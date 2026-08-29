#pragma once

#include "Config.h"
#include "Bitfield.h"
#include "Serialize.h"
#include "ModelUtils.h"
#include "Types.h"
#include "Scale.h"
#include "Routing.h"
#include "FileDefs.h"
#include "os/os.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"
#include "core/utils/StringUtils.h"

#include <array>
#include <bitset>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <map>

class NoteSequence {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    typedef UnsignedValue<5> StepLength;    // index into StepLengthRatios
    typedef UnsignedValue<7> GateLength;    // 1..100 percent of the retrigger slice
    typedef UnsignedValue<3> Retrigger;     // stored 0..7 -> 1..8 slices
    typedef UnsignedValue<3> Repeat;        // stored 0..7 -> 1..8 repeats
    typedef SignedValue<5> ArpRetrigger;    // -15..+15 scale degrees per slice
    typedef SignedValue<7> Note;
    typedef UnsignedValue<7> Condition;

    static_assert(int(Types::Condition::Last) <= Condition::Max + 1, "Condition enum does not fit");

    // Step length expressed as a ratio against the sequence divisor. Sub-pulse
    // entries give triplets/rolls, multiples give steps spanning several pulses.
    struct StepLengthRatio {
        uint8_t num;
        uint8_t den;
    };

    static constexpr int StepLengthCount = 20;
    static constexpr int StepLengthDefault = 7;     // ratio 1/1

    static const StepLengthRatio &stepLengthRatio(int index) {
        static const StepLengthRatio ratios[StepLengthCount] = {
            { 1, 8 }, { 1, 6 }, { 1, 4 }, { 1, 3 }, { 1, 2 }, { 2, 3 }, { 3, 4 },
            { 1, 1 },
            { 5, 4 }, { 4, 3 }, { 3, 2 }, { 2, 1 }, { 5, 2 }, { 3, 1 }, { 4, 1 },
            { 5, 1 }, { 6, 1 }, { 8, 1 }, { 12, 1 }, { 16, 1 },
        };
        return ratios[clamp(index, 0, StepLengthCount - 1)];
    }

    // step duration in engine ticks for the given divisor
    static uint32_t stepLengthTicks(int index, uint32_t divisor) {
        const auto &r = stepLengthRatio(index);
        uint32_t ticks = (divisor * r.num) / r.den;
        return ticks < 2 ? 2 : ticks;
    }

    static void printStepLength(StringBuilder &str, int index) {
        const auto &r = stepLengthRatio(index);
        if (r.den == 1) {
            str("%d", r.num);
        } else {
            str("%d/%d", r.num, r.den);
        }
    }

    enum class Layer {
        Gate,
        StepLength,
        GateLength,
        Retrigger,
        ArpRetrigger,
        Repeat,
        Note,
        Slide,
        Condition,
        RetriggerCondition,
        Last
    };

    static const char *layerName(Layer layer) {
        switch (layer) {
        case Layer::Gate:               return "GATE";
        case Layer::StepLength:         return "STEP LENGTH";
        case Layer::GateLength:         return "GATE LENGTH";
        case Layer::Retrigger:          return "RETRIG";
        case Layer::ArpRetrigger:       return "ARP RETRIG";
        case Layer::Repeat:             return "REPEAT";
        case Layer::Note:               return "NOTE";
        case Layer::Slide:              return "SLIDE";
        case Layer::Condition:          return "CONDITION";
        case Layer::RetriggerCondition: return "RETRIG COND";
        case Layer::Last:               break;
        }
        return nullptr;
    }

    static Types::LayerRange layerRange(Layer layer);
    static int layerDefaultValue(Layer layer);

    static constexpr size_t NameLength = FileHeader::NameLength;


    class Step {

    public:
        //----------------------------------------
        // Properties
        //----------------------------------------

        // gate

        bool gate() const { return _data0.gate ? true : false; }
        void setGate(bool gate) { _data0.gate = gate; }
        void toggleGate() { setGate(!gate()); }

        // slide

        bool slide() const { return _data0.slide ? true : false; }
        void setSlide(bool slide) { _data0.slide = slide; }
        void toggleSlide() { setSlide(!slide()); }

        // stepLength (index into StepLengthRatios)

        int stepLength() const { return _data0.stepLength; }
        void setStepLength(int stepLength) {
            _data0.stepLength = clamp(stepLength, 0, StepLengthCount - 1);
        }

        // gateLength (1..100 percent of the retrigger slice)

        int gateLength() const { return _data0.gateLength; }
        void setGateLength(int gateLength) {
            _data0.gateLength = clamp(gateLength, 1, 100);
        }

        // retrigger (1..8 slices per step)

        int retrigger() const { return _data0.retrigger + 1; }
        void setRetrigger(int retrigger) {
            _data0.retrigger = Retrigger::clamp(retrigger - 1);
        }

        // repeat (1..8 replays of the whole step body)

        int repeat() const { return _data0.repeat + 1; }
        void setRepeat(int repeat) {
            _data0.repeat = Repeat::clamp(repeat - 1);
        }

        // arpRetrigger (scale degrees added per retrigger slice)

        int arpRetrigger() const { return ArpRetrigger::Min + _data0.arpRetrigger; }
        void setArpRetrigger(int arpRetrigger) {
            _data0.arpRetrigger = ArpRetrigger::clamp(arpRetrigger) - ArpRetrigger::Min;
        }

        // note

        int note() const { return Note::Min + _data0.note; }
        void setNote(int note) {
            _data0.note = Note::clamp(note) - Note::Min;
        }

        // condition (gates the whole step)

        Types::Condition condition() const { return Types::Condition(int(_data1.condition)); }
        void setCondition(Types::Condition condition) {
            _data1.condition = int(ModelUtils::clampedEnum(condition));
        }

        // retriggerCondition (gates whether retrigger fires at all)

        Types::Condition retriggerCondition() const { return Types::Condition(int(_data1.retriggerCondition)); }
        void setRetriggerCondition(Types::Condition condition) {
            _data1.retriggerCondition = int(ModelUtils::clampedEnum(condition));
        }

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
            BitField<uint32_t, 0, 1> gate;
            BitField<uint32_t, 1, 1> slide;
            BitField<uint32_t, 2, StepLength::Bits> stepLength;
            BitField<uint32_t, 7, GateLength::Bits> gateLength;
            BitField<uint32_t, 14, Note::Bits> note;
            BitField<uint32_t, 21, Retrigger::Bits> retrigger;
            BitField<uint32_t, 24, Repeat::Bits> repeat;
            BitField<uint32_t, 27, ArpRetrigger::Bits> arpRetrigger;
            // full
        } _data0;
        union {
            uint32_t raw;
            BitField<uint32_t, 0, Condition::Bits> condition;
            BitField<uint32_t, 7, Condition::Bits> retriggerCondition;
            // 18 bits left
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

    int currentRecordStep() const {
        return _currentRecordStep.get(isRouted(Routing::Target::CurrentRecordStep)); 
    }
    
    void setCurrentRecordStep(int step, bool routed = false) {
        _currentRecordStep.set(clamp(step, firstStep(), CONFIG_STEP_COUNT - 1), routed);
    }    
    
    void editCurrentRecordStep(int value, bool shift) {
        if (!isRouted(Routing::Target::CurrentRecordStep)) {
            setCurrentRecordStep(currentRecordStep()+value);
        }
    }

    void printCurrentRecordStep(StringBuilder &str) const {
        printRouted(str, Routing::Target::CurrentRecordStep);
        str("%d", currentRecordStep() + 1);
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

    NoteSequence() { clear(); }

    void clear();
    void clearSteps();
    void clearStepsSelected(const std::bitset<CONFIG_STEP_COUNT> &selected);

    bool isEdited() const;

    void setGates(std::initializer_list<int> gates);
    void setNotes(std::initializer_list<int> notes);

    void shiftSteps(const std::bitset<CONFIG_STEP_COUNT> &selected, int direction);

    void duplicateSteps();

    void write(VersionedSerializedWriter &writer) const;
    bool read(VersionedSerializedReader &reader);

    int trackIndex() {
        return _trackIndex;
    }

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
    Routable<uint16_t> _divisor;
    uint8_t _resetMeasure;
    Routable<Types::RunMode> _runMode;
    Routable<uint8_t> _firstStep;
    Routable<uint8_t> _lastStep;

    Routable<uint8_t> _currentRecordStep;

    StepArray _steps;

    uint8_t _edited;

    int _section = 0;
    uint32_t _lastGateOff;
    uint8_t _gate;
    static constexpr uint32_t GateOnDelay = os::time::ms(5);

    friend class NoteTrack;
    friend class QuantizerTrack;
};
