#pragma once

#include "Config.h"
#include "Bitfield.h"
#include "Serialize.h"
#include "ModelUtils.h"
#include "Types.h"
#include "Curve.h"
#include "Routing.h"
#include "FileDefs.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"
#include "core/utils/StringUtils.h"

#include <array>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <initializer_list>

class CurveSequence {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    typedef UnsignedValue<7> Shape;
    typedef UnsignedValue<8> Min;
    typedef UnsignedValue<8> Max;
    typedef UnsignedValue<4> Gate;
    typedef UnsignedValue<3> GateProbability;

    enum class Layer {
        Shape,
        Skew,
        Length,
        Level,
        Offset,
        Last
    };

    static const char *layerName(Layer layer) {
        switch (layer) {
        case Layer::Shape:  return "SHPE";
        case Layer::Skew:   return "SKEW";
        case Layer::Length: return "LEN";
        case Layer::Level:  return "LVL";
        case Layer::Offset: return "OFST";
        case Layer::Last:   break;
        }
        return nullptr;
    }

    static constexpr size_t NameLength = FileHeader::NameLength;

    static Types::LayerRange layerRange(Layer layer);
    static int layerDefaultValue(Layer layer);

    static float evalSegment(float phase, float shape, float skew) {
        // skew warps phase so that the sine peak lands at phase == skew. The two
        // endpoints are handled exactly so that skew 0 gives an instant attack and
        // skew 1 an instant release, rather than ramping over a small epsilon.
        float sk = clamp(skew, 0.0f, 1.0f);
        float t;
        if (sk <= 0.0f) {
            t = 0.5f + phase * 0.5f;
        } else if (sk >= 1.0f) {
            t = phase * 0.5f;
        } else {
            t = phase < sk
                ? (phase / sk) * 0.5f
                : 0.5f + ((phase - sk) / (1.0f - sk)) * 0.5f;
        }
        float s = std::sin(t * float(M_PI));
        float p = shape <= 0.5f
            ? 1.0f + (1.0f - shape * 2.0f) * 7.0f
            : std::pow(1.0f - (shape - 0.5f) * 2.0f, 2.0f);
        return p <= 0.0f ? 1.0f : std::pow(std::max(0.0f, s), p);
    }

    class Step {
    public:
        //----------------------------------------
        // Properties
        //----------------------------------------

        // shape

        int shape() const { return _data0.shape; }
        void setShape(int shape) {
            _data0.shape = clamp(shape, 0, int(Curve::Last) - 1);
        }

        // min

        int min() const { return _data0.min; }
        void setMin(int min) {
            _data0.min = Min::clamp(min);
            _data0.max = std::max(max(), this->min());
        }

        float minNormalized() const { return float(min()) / Min::Max; }
        void setMinNormalized(float min) {
            setMin(int(std::round(min * Min::Max)));
        }

        // max

        int max() const { return _data0.max; }
        void setMax(int max) {
            _data0.max = Max::clamp(max);
            _data0.min = std::min(min(), this->max());
        }

        float maxNormalized() const { return float(max()) / Max::Max; }
        void setMaxNormalized(float max) {
            setMax(int(std::round(max * Max::Max)));
        }

        // gate

        int gate() const { return _data1.gate; }
        void setGate(int gate) {
            _data1.gate = Gate::clamp(gate);
        }

        // gateProbability

        int gateProbability() const { return _data1.gateProbability; }
        void setGateProbability(int gateProbability) {
            _data1.gateProbability = GateProbability::clamp(gateProbability);
        }

        // V1 accessors (segment assembler model)

        float shapeNorm() const { return float(_data0.shape) / Shape::Max; }
        void  setShapeNorm(float v) { _data0.shape = Shape::clamp(int(v * Shape::Max + 0.5f)); }

        int   skewRaw() const { return _data0.shapeVariation; }
        void  setSkewRaw(int v) { _data0.shapeVariation = Shape::clamp(v); }
        float skewNorm() const { return float(_data0.shapeVariation) / Shape::Max; }
        void  setSkewNorm(float v) { _data0.shapeVariation = Shape::clamp(int(v * Shape::Max + 0.5f)); }

        float offsetNorm() const { return float(_data0.min) / Min::Max; }
        void  setOffsetNorm(float v) { _data0.min = Min::clamp(int(v * Min::Max + 0.5f)); }

        float levelNorm() const { return float(_data0.max) / Max::Max; }
        void  setLevelNorm(float v) { _data0.max = Max::clamp(int(v * Max::Max + 0.5f)); }

        int   length() const { return _data1.gate + 1; }
        void  setLength(int len) { _data1.gate = clamp(len - 1, 0, 15); }

        // Percent accessors (0..100) - the domain the UI edits and displays in.
        // Integer math keeps the round trip exact in both directions, so stepping
        // by +/-1 is symmetric: up then down returns to the original value.

        static int   toPercent(int raw, int rawMax) { return (raw * 100 + rawMax / 2) / rawMax; }
        static int   fromPercent(int percent, int rawMax) { return (clamp(percent, 0, 100) * rawMax + 50) / 100; }

        int   shapePercent() const { return toPercent(_data0.shape, Shape::Max); }
        void  setShapePercent(int v) { _data0.shape = fromPercent(v, Shape::Max); }

        int   skewPercent() const { return toPercent(_data0.shapeVariation, Shape::Max); }
        void  setSkewPercent(int v) { _data0.shapeVariation = fromPercent(v, Shape::Max); }

        int   offsetPercent() const { return toPercent(_data0.min, Min::Max); }
        void  setOffsetPercent(int v) { _data0.min = fromPercent(v, Min::Max); }

        int   levelPercent() const { return toPercent(_data0.max, Max::Max); }
        void  setLevelPercent(int v) { _data0.max = fromPercent(v, Max::Max); }

        int layerValue(Layer layer) const;
        void setLayerValue(Layer layer, int value);

        //----------------------------------------
        // Methods
        //----------------------------------------

        Step() { clear(); }

        void clear();

        void write(VersionedSerializedWriter &writer) const;
        void read(VersionedSerializedReader &reader);

        bool operator==(const Step &other) const {
            return _data0.raw == other._data0.raw && _data1.raw == other._data1.raw;
        }

        bool operator!=(const Step &other) const {
            return !(*this == other);
        }

    private:
        union {
            uint32_t raw;
            BitField<uint32_t, 0, Shape::Bits> shape;
            BitField<uint32_t, 7, Shape::Bits> shapeVariation;
            // 2 bits left (bits 14-15)
            BitField<uint32_t, 16, Min::Bits> min;
            BitField<uint32_t, 24, Max::Bits> max;
        } _data0;
        union {
            uint16_t raw;
            BitField<uint16_t, 0, Gate::Bits> gate;
            BitField<uint16_t, 4, GateProbability::Bits> gateProbability;
            // 9 bits left
        } _data1;
    };

    typedef std::array<Step, CONFIG_STEP_COUNT> StepArray;

    //----------------------------------------
    // Properties
    //----------------------------------------

    // slot

    int slot() const { return _slot; }
    void setSlot(int slot) {
        _slot = slot;
    }
    bool slotAssigned() const {
        return _slot != uint8_t(-1);
    }

    // name

    const char *name() const { return _name; }
    void setName(const char *name) {
        StringUtils::copy(_name, name, sizeof(_name));
    }


    // trackIndex

    int trackIndex() const { return _trackIndex; }

    // range

    Types::VoltageRange range() const { return _range; }
    void setRange(Types::VoltageRange range) {
        _range = ModelUtils::clampedEnum(range);
    }

    void editRange(int value, bool shift) {
        setRange(ModelUtils::adjustedEnum(range(), value));
    }

    void printRange(StringBuilder &str) const {
        str(Types::voltageRangeName(range()));
    }

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

    // firstStep

    int firstStep() const {
        return _firstStep.get(isRouted(Routing::Target::FirstStep));
    }

    void setFirstStep(int firstStep, bool routed = false) {
        _firstStep.set(clamp(firstStep, 0, lastStep()), routed);
    }

    void editFirstStep(int value, bool shift) {
        if (shift) {
            offsetFirstAndLastStep(value);
        } else if (!isRouted(Routing::Target::FirstStep)) {
            setFirstStep(firstStep() + value);
        }
    }

    void printFirstStep(StringBuilder &str) const {
        printRouted(str, Routing::Target::FirstStep);
        str("%d", firstStep() + 1);
    }

    // lastStep

    int lastStep() const {
        // make sure last step is always >= first step even if stored value is invalid (due to routing changes)
        return std::max(firstStep(), int(_lastStep.get(isRouted(Routing::Target::LastStep))));
    }

    void setLastStep(int lastStep, bool routed = false) {
        _lastStep.set(clamp(lastStep, firstStep(), CONFIG_STEP_COUNT - 1), routed);
    }

    void editLastStep(int value, bool shift) {
        if (shift) {
            offsetFirstAndLastStep(value);
        } else if (!isRouted(Routing::Target::LastStep)) {
            setLastStep(lastStep() + value);
        }
    }

    void printLastStep(StringBuilder &str) const {
        printRouted(str, Routing::Target::LastStep);
        str("%d", lastStep() + 1);
    }

    // segmentCount (V1)

    int  segmentCount() const { return _segmentCount; }
    void setSegmentCount(int n) { _segmentCount = uint8_t(clamp(n, 1, 16)); }
    void editSegmentCount(int value, bool shift) { setSegmentCount(int(_segmentCount) + value); }

    void printSegmentCount(StringBuilder &str) const {
        str("%d", int(_segmentCount));
    }

    // steps

    const StepArray &steps() const { return _steps; }
          StepArray &steps()       { return _steps; }

    const Step &step(int index) const { return _steps[index]; }
          Step &step(int index)       { return _steps[index]; }

    //----------------------------------------
    // Routing
    //----------------------------------------

    inline bool isRouted(Routing::Target target) const { return Routing::isRouted(target, _trackIndex); }
    inline void printRouted(StringBuilder &str, Routing::Target target) const { Routing::printRouted(str, target, _trackIndex); }
    void writeRouted(Routing::Target target, int intValue, float floatValue);

    //----------------------------------------
    // Methods
    //----------------------------------------

    CurveSequence() { clear(); }

    void clear();
    void clearSteps();
    void clearStepsSelected(const std::bitset<CONFIG_STEP_COUNT> &selected);

    void deleteSegment(int idx);
    void duplicateSegment(int idx);
    void resetSegment(int idx);
    void addSegment();


    bool isEdited() const;

    void setShapes(std::initializer_list<int> shapes);

    void shiftSteps(const std::bitset<CONFIG_STEP_COUNT> &selected, int direction);

    void duplicateSteps();

    void write(VersionedSerializedWriter &writer) const;
    bool read(VersionedSerializedReader &reader);

    int section() { return _section; }
    const int section() const { return _section; }

    void setSecion(int section) {
        _section = section;
    }

private:
    void setTrackIndex(int trackIndex) { _trackIndex = trackIndex; }

    void offsetFirstAndLastStep(int value) {
        value = clamp(value, -firstStep(), CONFIG_STEP_COUNT - 1 - lastStep());
        if (value > 0) {
            editLastStep(value, false);
            editFirstStep(value, false);
        } else {
            editFirstStep(value, false);
            editLastStep(value, false);
        }
    }

    uint8_t _slot = uint8_t(-1);
    char _name[NameLength + 1];
    int8_t _trackIndex = -1;
    Types::VoltageRange _range;
    Routable<uint16_t> _divisor;
    uint8_t _resetMeasure;
    Routable<Types::RunMode> _runMode;
    Routable<uint8_t> _firstStep;
    Routable<uint8_t> _lastStep;

    StepArray _steps;

    uint8_t _segmentCount = 8;

    int _section = 0;

    friend class CurveTrack;
};
