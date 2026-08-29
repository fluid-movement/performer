#include "NoteSequence.h"
#include "ProjectVersion.h"

#include "ModelUtils.h"
#include "Types.h"
#include "Routing.h"

#include "os/os.h"
#include <cstdint>

Types::LayerRange NoteSequence::layerRange(Layer layer) {
    switch (layer) {
    case Layer::Gate:
    case Layer::Slide:
        return { 0, 1 };
    case Layer::StepLength:
        return { 0, StepLengthCount - 1 };
    case Layer::GateLength:
        return { 1, 100 };
    case Layer::Retrigger:
        return { 1, Retrigger::Max + 1 };
    case Layer::Repeat:
        return { 1, Repeat::Max + 1 };
    case Layer::ArpRetrigger:
        return { ArpRetrigger::Min, ArpRetrigger::Max };
    case Layer::Note:
        return { Note::Min, Note::Max };
    case Layer::Condition:
    case Layer::RetriggerCondition:
        return { 0, int(Types::Condition::Last) - 1 };
    case Layer::Last:
        break;
    }

    return { 0, 0 };
}

int NoteSequence::layerDefaultValue(Layer layer)
{
    NoteSequence::Step step;

    switch (layer) {
    case Layer::Gate:               return step.gate();
    case Layer::Slide:              return step.slide();
    case Layer::StepLength:         return step.stepLength();
    case Layer::GateLength:         return step.gateLength();
    case Layer::Retrigger:          return step.retrigger();
    case Layer::ArpRetrigger:       return step.arpRetrigger();
    case Layer::Repeat:             return step.repeat();
    case Layer::Note:               return step.note();
    case Layer::Condition:          return int(step.condition());
    case Layer::RetriggerCondition: return int(step.retriggerCondition());
    case Layer::Last:               break;
    }

    return 0;
}

int NoteSequence::Step::layerValue(Layer layer) const {
    switch (layer) {
    case Layer::Gate:               return gate() ? 1 : 0;
    case Layer::Slide:              return slide() ? 1 : 0;
    case Layer::StepLength:         return stepLength();
    case Layer::GateLength:         return gateLength();
    case Layer::Retrigger:          return retrigger();
    case Layer::ArpRetrigger:       return arpRetrigger();
    case Layer::Repeat:             return repeat();
    case Layer::Note:               return note();
    case Layer::Condition:          return int(condition());
    case Layer::RetriggerCondition: return int(retriggerCondition());
    case Layer::Last:               break;
    }

    return 0;
}

void NoteSequence::Step::setLayerValue(Layer layer, int value) {
    switch (layer) {
    case Layer::Gate:               setGate(bool(value)); break;
    case Layer::Slide:              setSlide(bool(value)); break;
    case Layer::StepLength:         setStepLength(value); break;
    case Layer::GateLength:         setGateLength(value); break;
    case Layer::Retrigger:          setRetrigger(value); break;
    case Layer::ArpRetrigger:       setArpRetrigger(value); break;
    case Layer::Repeat:             setRepeat(value); break;
    case Layer::Note:               setNote(value); break;
    case Layer::Condition:          setCondition(Types::Condition(value)); break;
    case Layer::RetriggerCondition: setRetriggerCondition(Types::Condition(value)); break;
    case Layer::Last:               break;
    }
}

void NoteSequence::Step::clear() {
    _data0.raw = 0;
    _data1.raw = 0;
    setGate(false);
    setSlide(false);
    setStepLength(StepLengthDefault);
    setGateLength(50);
    setRetrigger(1);
    setArpRetrigger(0);
    setRepeat(1);
    setNote(0);
    setCondition(Types::Condition::Off);
    setRetriggerCondition(Types::Condition::Off);
}

void NoteSequence::Step::write(VersionedSerializedWriter &writer) const {
    if (writer.writerVersion() >= ProjectVersion::Version54) {
        writer.write(_data0.raw);
        writer.write(_data1.raw);
        return;
    }

    // Emit the pre-Version54 (post-Version36) bit layout so that writing at an
    // older version produces a file an older reader can consume -- the test
    // harness uses this to exercise the read migration.
    int legacyLength = clamp((gateLength() * 16) / 100 - 1, 0, 15);

    uint32_t d0 = 0;
    d0 |= uint32_t(gate() ? 1 : 0) << 0;
    d0 |= uint32_t(slide() ? 1 : 0) << 1;
    d0 |= uint32_t(legacyLength & 0xF) << 2;
    d0 |= uint32_t(7) << 6;                             // lengthVariationRange = 0 (biased)
    d0 |= uint32_t(15) << 10;                           // lengthVariationProbability = max
    d0 |= uint32_t(_data0.note & 0x7F) << 14;           // same bias as the new field
    d0 |= uint32_t(63) << 21;                           // noteVariationRange = 0 (biased)
    d0 |= uint32_t(15) << 28;                           // noteVariationProbability = max

    uint32_t d1 = 0;
    d1 |= uint32_t(0) << 0;                             // bypassScale
    d1 |= uint32_t((retrigger() - 1) & 0x7) << 1;
    d1 |= uint32_t(15) << 4;                            // gateProbability = max
    d1 |= uint32_t(15) << 8;                            // retriggerProbability = max
    d1 |= uint32_t(7) << 12;                            // gateOffset = 0 (biased)
    d1 |= uint32_t(int(condition()) & 0x7F) << 16;
    d1 |= uint32_t((repeat() - 1) & 0x7) << 23;
    d1 |= uint32_t(0) << 26;                            // stageRepeatMode = Each

    writer.write(d0);
    writer.write(d1);
}

void NoteSequence::Step::read(VersionedSerializedReader &reader) {

    if (reader.dataVersion() >= ProjectVersion::Version54) {
        reader.read(_data0.raw);
        reader.read(_data1.raw);
        return;
    }

    // --- Version54 migration -------------------------------------------------
    // Read the legacy pair, normalize it to the post-Version36 bit layout, then
    // map the surviving fields into the new layout. Probability, variation, gate
    // offset, stage repeat mode and bypass scale no longer exist.

    uint32_t d0 = 0;
    uint32_t d1 = 0;

    if (reader.dataVersion() < ProjectVersion::Version27) {
        reader.read(d0);
        reader.readAs<uint16_t>(d1);
        if (reader.dataVersion() < ProjectVersion::Version5) {
            d1 &= 0x1f;
        }
    } else {
        reader.read(d0);
        reader.read(d1);
    }

    if (reader.dataVersion() < ProjectVersion::Version36) {
        // length grew 3->4 bits: shift everything above gate/slide up by one,
        // and open _data1 bit 0 for the bypassScale flag that used to live in
        // _data0 bit 31.
        d0 = (d0 & 0x3) | ((d0 & 0xFFFFFFFC) << 1);
        d1 = 0x7FFFFFFF & (d1 << 1);
    }

    // legacy field extraction (post-Version36 layout)
    bool legacyGate      = (d0 >> 0) & 0x1;
    bool legacySlide     = (d0 >> 1) & 0x1;
    int  legacyLength    = (d0 >> 2) & 0xF;
    int  legacyNote      = (d0 >> 14) & 0x7F;
    int  legacyRetrigger = (d1 >> 1) & 0x7;
    int  legacyCondition = (d1 >> 16) & 0x7F;
    int  legacyRepeats   = (d1 >> 23) & 0x7;

    if (reader.dataVersion() < ProjectVersion::Version12) {
        legacyCondition = 0;
    }

    clear();

    _data0.gate = legacyGate;
    _data0.slide = legacySlide;
    _data0.note = legacyNote;                       // same bit width and bias
    _data0.retrigger = legacyRetrigger;             // both store count-1
    _data0.repeat = legacyRepeats;                  // stage repeats stored count-1
    // old length was a 1/16..16/16 duty cycle of the step
    setGateLength(((legacyLength + 1) * 100) / 16);
    setCondition(Types::Condition(legacyCondition));
}

void NoteSequence::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::Divisor:
        setDivisor(intValue, true);
        break;
    case Routing::Target::RunMode:
        setRunMode(Types::RunMode(intValue), true);
        break;
    case Routing::Target::FirstStep:
        setFirstStep(intValue, true);
        break;
    case Routing::Target::LastStep:
        setLastStep(intValue, true);
        break;
    case Routing::Target::CurrentRecordStep:
        if (_gate) {
            if (floatValue < 2.f) {
                // gate off
                _gate = 0;
                _lastGateOff = os::ticks();
            }
        } else {
            if (floatValue > 3.f) {
                if (os::ticks() - _lastGateOff >= GateOnDelay) {
                    // gate on
                    _gate = 1;
                    setCurrentRecordStep(currentRecordStep()+1,true);
                }
            } else {
                _lastGateOff = os::ticks();
            }
        }
        break;
    default:
        break;
    }
}

void NoteSequence::clear() {
    setName("INIT");
    setDivisor(12);
    setResetMeasure(0);
    setRunMode(Types::RunMode::Forward);
    setFirstStep(0);
    setLastStep(15);
    setCurrentRecordStep(0);

    clearSteps();
}

void NoteSequence::clearSteps() {
    for (auto &step : _steps) {
        step.clear();
    }
}

void NoteSequence::clearStepsSelected(const std::bitset<CONFIG_STEP_COUNT> &selected) {
    if (selected.any()) {
        for (size_t i = 0; i < CONFIG_STEP_COUNT; ++i) {
            if (selected[i]) {
                _steps[i].clear();
            }
    }
    } else {
        clearSteps();
    }
}

bool NoteSequence::isEdited() const {
    auto clearStep = Step();
    for (const auto &step : _steps) {
        if (step != clearStep) {
            return true;
        }
    }
    return false;
}

void NoteSequence::setGates(std::initializer_list<int> gates) {
    size_t step = 0;
    for (auto gate : gates) {
        if (step < _steps.size()) {
            _steps[step++].setGate(gate);
        }
    }
}

void NoteSequence::setNotes(std::initializer_list<int> notes) {
    size_t step = 0;
    for (auto note : notes) {
        if (step < _steps.size()) {
            _steps[step++].setNote(note);
        }
    }
}

void NoteSequence::shiftSteps(const std::bitset<CONFIG_STEP_COUNT> &selected, int direction) {
    if (selected.any()) {
        ModelUtils::shiftSteps(_steps, selected, firstStep(), lastStep()+1, direction);
    } else {
        ModelUtils::shiftSteps(_steps, firstStep(), lastStep()+1, direction);
    }
}

void NoteSequence::duplicateSteps() {
    ModelUtils::duplicateSteps(_steps, firstStep(), lastStep());
    setLastStep(lastStep() + (lastStep() - firstStep() + 1));
}

void NoteSequence::write(VersionedSerializedWriter &writer) const {
    writer.write(_divisor.base);
    writer.write(_resetMeasure);
    writer.write(_runMode.base);
    writer.write(_firstStep.base);
    writer.write(_lastStep.base);

    writeArray(writer, _steps);
    writer.write(_name, NameLength + 1);
    writer.write(_slot);
    writer.writeHash();
}

bool NoteSequence::read(VersionedSerializedReader &reader) {
    if (reader.dataVersion() < ProjectVersion::Version41) {
        int8_t discardScale, discardRootNote;
        reader.read(discardScale);
        reader.read(discardRootNote);
    }
    if (reader.dataVersion() < ProjectVersion::Version10) {
        reader.readAs<uint8_t>(_divisor.base);
    } else {
        reader.read(_divisor.base);
    }
    reader.read(_resetMeasure);
    reader.read(_runMode.base);
    reader.read(_firstStep.base);
    reader.read(_lastStep.base);

    readArray(reader, _steps);

    if (reader.dataVersion() >= ProjectVersion::Version35) {
        reader.read(_name, NameLength + 1, ProjectVersion::Version35);
        reader.read(_slot);
        bool success = reader.checkHash();
        if (!success) {
            clear();
        }

        return success;
    } else {
        return true;
    }
}
