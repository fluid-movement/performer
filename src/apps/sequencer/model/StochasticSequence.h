#pragma once

#include "Config.h"
#include "Serialize.h"
#include "ModelUtils.h"
#include "Types.h"
#include "Scale.h"
#include "Routing.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"

#include <cstdint>

class StochasticSequence {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    enum class Layer {
        Note = 0,
        Oct  = 1,
        Len  = 2,
        Loop = 3,
        Last
    };

    static const char *layerName(Layer layer) {
        switch (layer) {
        case Layer::Note: return "NOTE";
        case Layer::Oct:  return "OCT";
        case Layer::Len:  return "LEN";
        case Layer::Loop: return "LOOP";
        case Layer::Last: break;
        }
        return nullptr;
    }

    static Types::LayerRange layerRange(Layer /*layer*/) {
        return { 0, 15 };
    }

    static int layerDefaultValue(Layer /*layer*/) {
        return 8;
    }

    enum Message {
        None,
        LoopOn,
        LoopOff,
        Cleared,
        ReSeed,
    };

    //----------------------------------------
    // Properties
    //----------------------------------------

    // trackIndex

    int trackIndex() const { return _trackIndex; }

    // divisor

    int divisor() const { return _divisor.get(isRouted(Routing::Target::Divisor)); }
    void setDivisor(int divisor, bool routed = false) {
        _divisor.set(ModelUtils::clampDivisor(divisor), routed);
    }

    int indexedDivisor() const { return ModelUtils::divisorToIndex(divisor()); }
    void setIndexedDivisor(int index) {
        int divisor = ModelUtils::indexToDivisor(index);
        if (divisor > 0) {
            setDivisor(divisor);
        }
    }

    void editDivisor(int value, bool shift) {
        if (!isRouted(Routing::Target::Divisor)) {
            setDivisor(ModelUtils::adjustedByDivisor(divisor(), value, shift));
        }
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

    // runMode

    Types::RunMode runMode() const { return _runMode.get(isRouted(Routing::Target::RunMode)); }
    void setRunMode(Types::RunMode runMode, bool routed = false) {
        _runMode.set(ModelUtils::clampedEnum(runMode), routed);
    }

    void editRunMode(int value, bool shift) {
        if (!isRouted(Routing::Target::RunMode)) {
            setRunMode(ModelUtils::adjustedEnum(runMode(), value));
        }
    }

    void printRunMode(StringBuilder &str) const {
        printRouted(str, Routing::Target::RunMode);
        str(Types::runModeName(runMode()));
    }

    // reseed

    bool reseed() const {
        return _reseed.get(isRouted(Routing::Target::Reseed));
    }

    void setReseed(int r, bool routed = false) {
        if (!isRouted(Routing::Target::Reseed)) {
            _reseed.set(r, routed);
        }
        if (reseed()) {
            setMessage(Message::ReSeed);
        }
    }

    // sequence loop last step

    int sequenceLastStep() const {
        return std::max(sequenceFirstStep(), int(_sequenceLastStep.get(isRouted(Routing::Target::SequenceLastStep))));
    }

    void setSequenceLastStep(int lastStep, bool routed = false) {
         _sequenceLastStep.set(clamp(lastStep, sequenceFirstStep(), CONFIG_STEP_COUNT - 1), routed);
    }

    void editSequenceLastStep(int value, bool shift) {
        if (shift) {
            offsetSequenceFirstAndLastStep(value);
        } else if (!isRouted(Routing::Target::SequenceLastStep)) {
            setSequenceLastStep(sequenceLastStep() + value);
        }
    }

    void printSequenceLastStep(StringBuilder &str) const {
        printRouted(str, Routing::Target::SequenceLastStep);
        str("%d", sequenceLastStep()+1);
    }

    // sequence loop first step

     int sequenceFirstStep() const {
        return _sequenceFirstStep.get(isRouted(Routing::Target::SequenceFirstStep));
    }

    void setSequenceFirstStep(int firstStep, bool routed = false) {
        _sequenceFirstStep.set(clamp(firstStep, 0, sequenceLastStep()), routed);
    }

    void editSequenceFirstStep(int value, bool shift) {
        if (shift) {
            offsetSequenceFirstAndLastStep(value);
        } else if (!isRouted(Routing::Target::FirstStep)) {
            setSequenceFirstStep(sequenceFirstStep() + value);
        }
    }

    void printSequenceFirstStep(StringBuilder &str) const {
        printRouted(str, Routing::Target::SequenceFirstStep);
        str("%d", sequenceFirstStep()+1);
    }

    // sequence length

    int sequenceLength() {
        return _sequenceLastStep.base - _sequenceFirstStep.base + 1;
    }

    // buffer loop length

    int bufferLoopLength() const {
        return CONFIG_STEP_COUNT;
    }

    // use loop

    void setUseLoop() {
        _useLoop = !_useLoop;
        if (_useLoop) {
            setMessage(Message::LoopOn);
        } else {
            setMessage(Message::LoopOff);
        }
    }

    void setUseLoop(bool value) {
        _useLoop = value;
    }

    bool useLoop() {
        return _useLoop;
    }

    const bool useLoop() const { return _useLoop;}

    // clear loop

    void setClearLoop(bool clearLoop) {
        _clearLoop = clearLoop;
        if (_clearLoop) {
            setMessage(Message::Cleared);
        }
    }

    bool clearLoop() {
        return _clearLoop;
    }

    const bool clearLoop() const { return _clearLoop; }

    // loopChance

    int loopChance() const { return _loopChance; }
    void setLoopChance(int chance) {
        _loopChance = clamp(chance, 0, 15);
    }
    void editLoopChance(int value, bool shift) {
        setLoopChance(loopChance() + value * (shift ? 4 : 1));
    }
    void printLoopChance(StringBuilder &str) const {
        str("%d", loopChance());
    }

    // loopLength: how many buffer steps to replay per loop cycle (wraps around the 16-step buffer)

    int loopLength() const { return _loopLength; }
    void setLoopLength(int len) { _loopLength = clamp(len, 1, CONFIG_STEP_COUNT); }
    void editLoopLength(int value, bool shift) { setLoopLength(loopLength() + value * (shift ? 4 : 1)); }

    // degreeProb: scale degree selection probability, 7 degrees (0–6), each 0–15

    int degreeProb(int i) const { return _degreeProb[i]; }
    void setDegreeProb(int i, int v) {
        _degreeProb[i] = clamp(v, 0, 15);
    }
    void editDegreeProb(int i, int v, bool shift) {
        setDegreeProb(i, degreeProb(i) + v * (shift ? 4 : 1));
    }
    void printDegreeProb(int i, StringBuilder &str) const {
        str("%d", degreeProb(i));
    }

    // octaveProb: octave-offset probability, 5 slots (-2..+2), each 0–15

    int octaveProb(int i) const { return _octaveProb[i]; }
    void setOctaveProb(int i, int v) {
        _octaveProb[i] = clamp(v, 0, 15);
    }
    void editOctaveProb(int i, int v, bool shift) {
        setOctaveProb(i, octaveProb(i) + v * (shift ? 4 : 1));
    }
    void printOctaveProb(int i, StringBuilder &str) const {
        str("%d", octaveProb(i));
    }

    // durationProb: note duration probability, 6 slots (1/16..2 bars), each 0–15

    int durationProb(int i) const { return _durationProb[i]; }
    void setDurationProb(int i, int v) {
        _durationProb[i] = clamp(v, 0, 15);
    }
    void editDurationProb(int i, int v, bool shift) {
        setDurationProb(i, durationProb(i) + v * (shift ? 4 : 1));
    }
    void printDurationProb(int i, StringBuilder &str) const {
        str("%d", durationProb(i));
    }

    // message

    Message message() {
        return _message;
    }

    void setMessage(Message message) {
        _message = message;
    }

    const bool isEmpty() const {
        for (int i = 0; i < 7; i++) {
            if (_degreeProb[i] > 0) return false;
        }
        return true;
    }

    //----------------------------------------
    // Routing
    //----------------------------------------

    inline bool isRouted(Routing::Target target) const { return Routing::isRouted(target, _trackIndex); }
    inline void printRouted(StringBuilder &str, Routing::Target target) const { Routing::printRouted(str, target, _trackIndex); }
    void writeRouted(Routing::Target target, int intValue, float floatValue);

    //----------------------------------------
    // Methods
    //----------------------------------------

    StochasticSequence() { clear(); }

    void clear();

    void write(VersionedSerializedWriter &writer) const;
    void read(VersionedSerializedReader &reader);

private:
    void setTrackIndex(int trackIndex) { _trackIndex = trackIndex; }

    void offsetSequenceFirstAndLastStep(int value) {
        value = clamp(value, -sequenceFirstStep(), CONFIG_STEP_COUNT - 1 - sequenceLastStep());
        if (value > 0) {
            editSequenceLastStep(value, false);
            editSequenceFirstStep(value, false);
        } else {
            editSequenceFirstStep(value, false);
            editSequenceLastStep(value, false);
        }
    }

    int8_t _trackIndex = -1;
    Routable<uint16_t> _divisor;
    uint8_t _resetMeasure;
    Routable<Types::RunMode> _runMode;
    Routable<bool> _reseed;

    Routable<uint8_t> _sequenceLastStep;
    Routable<uint8_t> _sequenceFirstStep;

    uint8_t _loopChance = 0;
    uint8_t _loopLength = CONFIG_STEP_COUNT;

    bool _useLoop = false;
    bool _clearLoop = false;

    Message _message = Message::None;

    uint8_t _degreeProb[7];
    uint8_t _octaveProb[5];
    uint8_t _durationProb[6];

    friend class StochasticTrack;
};
