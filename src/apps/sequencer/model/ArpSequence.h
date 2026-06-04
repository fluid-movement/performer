#pragma once

#include "Config.h"
#include "Serialize.h"
#include "ModelUtils.h"
#include "Types.h"
#include "Scale.h"
#include "Routing.h"
#include "FileDefs.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"
#include "core/utils/StringUtils.h"

#include <cstdint>

class ArpSequence {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    enum class Layer {
        RhythmSteps, RhythmGates, RhythmRotate, RhythmGateLen,
        ModSteps, ModPulses, ModRotate, ModMode,
        ArpOrder, ArpOctaves, ArpLength,
        Last
    };

    enum class ModMode { Off = 0, Accent, Mask, Combine, Ratchet, Hold, Last };

    static const char *layerName(Layer layer);
    static const char *modModeName(ModMode mode);

    static constexpr size_t NameLength = FileHeader::NameLength;

    //----------------------------------------
    // Properties
    //----------------------------------------

    // slot

    int slot() const { return _slot; }
    void setSlot(int slot) { _slot = slot; }
    bool slotAssigned() const { return _slot != uint8_t(-1); }

    // name

    const char *name() const { return _name; }
    void setName(const char *name) {
        StringUtils::copy(_name, name, sizeof(_name));
    }

    // trackIndex

    int trackIndex() const { return _trackIndex; }

    // divisor

    int divisor() const { return _divisor.get(isRouted(Routing::Target::Divisor)); }
    void setDivisor(int divisor, bool routed = false) {
        _divisor.set(ModelUtils::clampDivisor(divisor), routed);
    }

    int indexedDivisor() const { return ModelUtils::divisorToIndex(divisor()); }
    void setIndexedDivisor(int index) {
        int d = ModelUtils::indexToDivisor(index);
        if (d > 0) setDivisor(d);
    }

    void editDivisor(int value, bool shift) {
        if (!isRouted(Routing::Target::Divisor))
            setDivisor(ModelUtils::adjustedByDivisor(divisor(), value, shift));
    }

    void printDivisor(StringBuilder &str) const {
        printRouted(str, Routing::Target::Divisor);
        ModelUtils::printDivisor(str, divisor());
    }

    // resetMeasure

    int resetMeasure() const { return _resetMeasure; }
    void setResetMeasure(int resetMeasure) {
        _resetMeasure = clamp(resetMeasure, 0, 128);
    }

    void editResetMeasure(int value, bool shift) {
        setResetMeasure(ModelUtils::adjustedByPowerOfTwo(resetMeasure(), value, shift));
    }

    void printResetMeasure(StringBuilder &str) const {
        if (resetMeasure() == 0) {
            str("off");
        } else {
            str("%d %s", resetMeasure(), resetMeasure() > 1 ? "bars" : "bar");
        }
    }

    // degreeMask (bits 0-6, one bit per scale degree)

    uint8_t degreeMask() const { return _degreeMask; }
    void setDegreeMask(uint8_t m) { _degreeMask = m & 0x7F; }
    void toggleDegree(int i) { if (i >= 0 && i < 7) _degreeMask ^= (1 << i); }
    bool isDegreeActive(int i) const { return i >= 0 && i < 7 && ((_degreeMask >> i) & 1); }

    // rhythmN (1-16)

    int rhythmN() const { return _rhythmN; }
    void setRhythmN(int v) {
        _rhythmN = clamp(v, 1, 16);
        if (_rhythmK > _rhythmN) _rhythmK = _rhythmN;
    }
    void editRhythmN(int delta, bool /*shift*/) { setRhythmN(rhythmN() + delta); }
    void printRhythmN(StringBuilder &str) const { str("%d", rhythmN()); }

    // rhythmK (0-rhythmN)

    int rhythmK() const { return _rhythmK; }
    void setRhythmK(int v) { _rhythmK = clamp(v, 0, int(_rhythmN)); }
    void editRhythmK(int delta, bool /*shift*/) { setRhythmK(rhythmK() + delta); }
    void printRhythmK(StringBuilder &str) const { str("%d", rhythmK()); }

    // rhythmR (0-15)

    int rhythmR() const { return _rhythmR; }
    void setRhythmR(int v) { _rhythmR = clamp(v, 0, 15); }
    void editRhythmR(int delta, bool /*shift*/) { setRhythmR(rhythmR() + delta); }
    void printRhythmR(StringBuilder &str) const { str("%d", rhythmR()); }

    // rhythmGateLen (0-19, prints as percentage: (val+1)*5 %)

    int rhythmGateLen() const { return _rhythmGateLen; }
    void setRhythmGateLen(int v) { _rhythmGateLen = clamp(v, 0, 19); }
    void editRhythmGateLen(int delta, bool /*shift*/) { setRhythmGateLen(rhythmGateLen() + delta); }
    void printRhythmGateLen(StringBuilder &str) const { str("%d%%", (rhythmGateLen() + 1) * 5); }

    // modN (1-16)

    int modN() const { return _modN; }
    void setModN(int v) {
        _modN = clamp(v, 1, 16);
        if (_modK > _modN) _modK = _modN;
    }
    void editModN(int delta, bool /*shift*/) { setModN(modN() + delta); }
    void printModN(StringBuilder &str) const { str("%d", modN()); }

    // modK (0-modN)

    int modK() const { return _modK; }
    void setModK(int v) { _modK = clamp(v, 0, int(_modN)); }
    void editModK(int delta, bool /*shift*/) { setModK(modK() + delta); }
    void printModK(StringBuilder &str) const { str("%d", modK()); }

    // modR (0-15)

    int modR() const { return _modR; }
    void setModR(int v) { _modR = clamp(v, 0, 15); }
    void editModR(int delta, bool /*shift*/) { setModR(modR() + delta); }
    void printModR(StringBuilder &str) const { str("%d", modR()); }

    // modMode (ModMode enum)

    int modMode() const { return _modMode; }
    void setModMode(int v) { _modMode = clamp(v, 0, int(ModMode::Last) - 1); }
    void editModMode(int delta, bool /*shift*/) { setModMode(modMode() + delta); }
    void printModMode(StringBuilder &str) const { str(modModeName(ModMode(_modMode))); }

    // arpOrder (0-6)

    int arpOrder() const { return _arpOrder; }
    void setArpOrder(int v) { _arpOrder = clamp(v, 0, 6); }
    void editArpOrder(int delta, bool /*shift*/) { setArpOrder(arpOrder() + delta); }
    void printArpOrder(StringBuilder &str) const;

    // arpOctaves (1-4)

    int arpOctaves() const { return _arpOctaves; }
    void setArpOctaves(int v) { _arpOctaves = clamp(v, 1, 4); }
    void editArpOctaves(int delta, bool /*shift*/) { setArpOctaves(arpOctaves() + delta); }
    void printArpOctaves(StringBuilder &str) const { str("%d", arpOctaves()); }

    // arpLength (1-64)

    int arpLength() const { return _arpLength; }
    void setArpLength(int v) { _arpLength = clamp(v, 1, 64); }
    void editArpLength(int delta, bool /*shift*/) { setArpLength(arpLength() + delta); }
    void printArpLength(StringBuilder &str) const { str("%d", arpLength()); }

    //----------------------------------------
    // Routing
    //----------------------------------------

    inline bool isRouted(Routing::Target target) const { return Routing::isRouted(target, _trackIndex); }
    inline void printRouted(StringBuilder &str, Routing::Target target) const { Routing::printRouted(str, target, _trackIndex); }
    void writeRouted(Routing::Target target, int intValue, float floatValue);

    //----------------------------------------
    // Methods
    //----------------------------------------

    ArpSequence() { clear(); }

    void clear();

    void write(VersionedSerializedWriter &writer) const;
    bool read(VersionedSerializedReader &reader);

private:
    void setTrackIndex(int trackIndex) { _trackIndex = trackIndex; }

    uint8_t _slot = uint8_t(-1);
    char _name[NameLength + 1];
    int8_t _trackIndex = -1;
    Routable<uint16_t> _divisor;
    uint8_t _resetMeasure = 0;

    // note pool
    uint8_t _degreeMask = 0x01;    // bit 0 = degree 1 active by default

    // rhythm euclidean
    uint8_t _rhythmN       = 16;
    uint8_t _rhythmK       = 5;
    uint8_t _rhythmR       = 0;
    uint8_t _rhythmGateLen = 7;    // 40% (value 7 → (7+1)*5 = 40%)

    // mod euclidean
    uint8_t _modN    = 12;
    uint8_t _modK    = 3;
    uint8_t _modR    = 0;
    uint8_t _modMode = 0;          // ModMode::Off

    // arp engine
    uint8_t _arpOrder   = 0;       // UP
    uint8_t _arpOctaves = 1;       // 1-4
    uint8_t _arpLength  = 16;      // 1-64

    friend class ArpTrack;
};
