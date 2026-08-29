#include "NoteTrackEngine.h"

#include "Engine.h"
#include "TrackEngineHelpers.h"
#include "Groove.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/NoteSequence.h"
#include "model/Scale.h"
#include "ui/MatrixMap.h"
#include <climits>
#include <iostream>
#include <ctime>

static Random rng;

// evaluate if step gate is active
static bool evalStepGate(const NoteSequence::Step &step) {
    return step.gate();
}

// evaluate a step condition. Only the gate condition advances the Pre/NotPre
// chain -- the retrigger condition reads it without writing, otherwise two
// writers per step would make Pre meaningless.
static bool evalStepCondition(Types::Condition condition, int iteration, bool fill, bool &prevCondition, bool writeChain) {
    bool result = true;
    switch (condition) {
    case Types::Condition::Off:                             return true;
    case Types::Condition::Fill:        result = fill;      break;
    case Types::Condition::NotFill:     result = !fill;     break;
    case Types::Condition::Pre:                             return prevCondition;
    case Types::Condition::NotPre:                          return !prevCondition;
    case Types::Condition::First:       result = iteration == 0; break;
    case Types::Condition::NotFirst:    result = iteration != 0; break;
    default: {
        int index = int(condition);
        if (index >= int(Types::Condition::Loop) && index < int(Types::Condition::Last)) {
            auto loop = Types::conditionLoop(condition);
            result = iteration % loop.base == loop.offset;
            if (loop.invert) result = !result;
        }
        break;
    }
    }
    if (writeChain) {
        prevCondition = result;
    }
    return result;
}

// evaluate transposition
static int evalTransposition(const Scale &scale, int octave, int transpose) {
    return octave * scale.notesPerOctave() + transpose;
}

// evaluate note voltage for a given scale-degree index
static float noteToVolts(int note, const Scale &scale, int rootNote) {
    return scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
}

// evaluate note voltage
static float evalStepNote(const NoteSequence::Step &step, int probabilityBias, const Scale &scale, int rootNote, int octave, int transpose, bool useVariation = true) {
    (void)probabilityBias;
    (void)useVariation;
    return noteToVolts(step.note() + evalTransposition(scale, octave, transpose), scale, rootNote);
}

void NoteTrackEngine::reset() {
    _sequenceState.reset();
    _currentStep = -1;
    _prevCondition = false;
    _activity = false;
    _gateOutput = false;
    if (_model.project().resetCvOnStop()) {
        _cvOutput = 0.f;
        _cvOutputTarget = 0.f;
    }
    _slideRamp.reset();
    _stepStartTick = 0;
    _stepTotalTicks = 0;
    _nextSlotTick = 0;
    _slotIndex = 0;
    _totalSlots = 1;
    _retrigCount = 1;
    _stepGate = false;
    _legatoHeld = false;
    _started = false;
    _gateQueue.clear();
    _cvQueue.clear();
    //_recordHistory.clear();

    changePattern();
}

void NoteTrackEngine::restart() {
    _sequenceState.reset();
    _currentStep = -1;
    _slotIndex = 0;
    _legatoHeld = false;
    _started = false;
}

TrackEngine::TickResult NoteTrackEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");
    const auto &sequence = *_sequence;
    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    if (linkData) {
        // A linked track borrows the master's step boundaries. Its own step
        // length and repeat are ignored -- variable step duration under a
        // foreign clock has no well defined meaning -- but retrigger, arp
        // retrigger and gate length still shape what happens inside the step.
        _linkData = *linkData;
        _sequenceState = *linkData->sequenceState;

        if (linkData->relativeTick % linkData->divisor == 0) {
            int absoluteStep = int(linkData->relativeTick / linkData->divisor);
            if (absoluteStep == 0 || absoluteStep >= _model.project().recordDelay() + 1) {
                recordStep(tick + 1, linkData->divisor);
            }
            _slotIndex = 0;
            beginStep(tick, linkData->divisor);
            _stepTotalTicks = linkData->divisor;
            _totalSlots = _retrigCount;
            advanceSlot(tick, linkData->divisor);
        } else if (_started && tick >= _nextSlotTick && _slotIndex != 0) {
            advanceSlot(tick, linkData->divisor);
        }
    } else {
        uint32_t divisor, relativeTick;
        computeClockParams(sequence, tick, _engine.measureDivisor(), divisor, relativeTick);

        if (pastStepsToStop(int(_model.project().stepsToStop()), relativeTick, divisor)) {
            _engine.clockStop();
        }

        // handle reset measure
        if (relativeTick == 0) {
            reset();
        }

        // Variable step length rules out PlayMode::Aligned -- advanceAligned
        // derives the step index in closed form from relativeTick / divisor,
        // which cannot express steps of differing duration. The engine always
        // advances freely, as CurveTrackEngine does.
        if (!_started || tick >= _nextSlotTick) {
            advanceSlot(tick, divisor);
        }

        updateLinkData(divisor, relativeTick, &_sequenceState);
    }

    auto &midiOutputEngine = _engine.midiOutputEngine();

    TickResult result = TickResult::NoUpdate;

    drainGateQueue(tick, _gateQueue, _activity, _gateOutput,
        _monitorOverrideActive, mute(), fill(), midiOutputEngine, _track.trackIndex(), result);
    drainCvQueue(tick, _cvQueue, _cvOutputTarget, _cvOutput, _slideRamp,
        _monitorOverrideActive, mute(), false,
        _noteTrack.slideTimeMs() * 0.001f,
        midiOutputEngine, _track.trackIndex(), result);

    return result;
}

void NoteTrackEngine::update(float dt) {
    bool running = _engine.state().running();
    bool recording = _engine.state().recording();

    const auto &sequence = *_sequence;
    const auto &scale = _model.project().selectedScale();
    int rootNote = _model.project().rootNote();
    int octave = _noteTrack.octave();
    int transpose = _noteTrack.transpose();

    // enable/disable step recording mode
    if (recording && _model.project().recordMode() == Types::RecordMode::StepRecord) {
        if (!_stepRecorder.enabled()) {
            _stepRecorder.start(sequence);
        }
    } else {
        if (_stepRecorder.enabled()) {
            _stepRecorder.stop();
        }
    }

    // helper to send gate/cv from monitoring to midi output engine
    auto sendToMidiOutputEngine = [this] (bool gate, float cv = 0.f) {
        auto &midiOutputEngine = _engine.midiOutputEngine();
        midiOutputEngine.sendGate(_track.trackIndex(), gate);
        if (gate) {
            midiOutputEngine.sendCv(_track.trackIndex(), cv);
            midiOutputEngine.sendSlide(_track.trackIndex(), false);
        }
    };

    // set monitor override
    auto setOverride = [&] (float cv) {
        _cvOutputTarget = cv;
        _activity = _gateOutput = true;
        _monitorOverrideActive = true;
        // pass through to midi engine
        sendToMidiOutputEngine(true, cv);
    };

    // clear monitor override
    auto clearOverride = [&] () {
        if (_monitorOverrideActive) {
            _activity = _gateOutput = false;
            _monitorOverrideActive = false;
            sendToMidiOutputEngine(false);
        }
    };

    // check for step monitoring
    bool stepMonitoring = (!running && _monitorStepIndex >= 0);

    // check for live monitoring
    auto monitorMode = _model.project().monitorMode();
    bool liveMonitoring =
        (monitorMode == Types::MonitorMode::Always) ||
        (monitorMode == Types::MonitorMode::Stopped && !running);

    if (stepMonitoring) {
        const auto &step = sequence.step(_monitorStepIndex);
        setOverride(evalStepNote(step, 0, scale, rootNote, octave, transpose, false));
    } else if (liveMonitoring && _recordHistory.isNoteActive()) {
        int note = noteFromMidiNote(_recordHistory.activeNote()) + evalTransposition(scale, octave, transpose);
        setOverride(scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f));
    } else {
        clearOverride();
    }

    applySlideUpdate(_cvOutput, _cvOutputTarget, _slideRamp, _noteTrack.slideType(), dt);
}

void NoteTrackEngine::changePattern() {
    _sequence = &_noteTrack.sequence(pattern());
    _fillSequence = &_noteTrack.sequence(std::min(pattern() + 1, CONFIG_PATTERN_COUNT - 1));
}

void NoteTrackEngine::monitorMidi(uint32_t tick, const MidiMessage &message) {
    _recordHistory.write(tick, message);

    if (_engine.recording() && _model.project().recordMode() == Types::RecordMode::StepRecord) {
        _stepRecorder.process(message, *_sequence, [this] (int midiNote) { return noteFromMidiNote(midiNote); });
        if (Routing::isRouted(Routing::Target::CurrentRecordStep, _model.project().selectedTrackIndex())) {
            _sequence->setCurrentRecordStep(_stepRecorder.stepIndex(), true);
        } else {
            _sequence->setCurrentRecordStep(_stepRecorder.stepIndex(), false);
        }
    }
}

void NoteTrackEngine::clearMidiMonitoring() {
    _recordHistory.clear();
}

void NoteTrackEngine::setMonitorStep(int index) {
    _monitorStepIndex = (index >= 0 && index < CONFIG_STEP_COUNT) ? index : -1;

    // in step record mode, select step to start recording recording from
    if (_engine.recording() && _model.project().recordMode() == Types::RecordMode::StepRecord &&
        index >= _sequence->firstStep() && index <= _sequence->lastStep()) {
        _stepRecorder.setStepIndex(index);
    }
}
// Absolute tick of a slot onset, derived from the step start rather than
// accumulated, so rounding error stays bounded and slot _totalSlots lands
// exactly on the end of the step.
uint32_t NoteTrackEngine::slotOnset(int slot) const {
    return _stepStartTick + (_stepTotalTicks * uint32_t(slot)) / uint32_t(_totalSlots);
}

// Latch everything that is decided once per step: which step plays, whether it
// gates at all, how long it lasts and how it is subdivided.
void NoteTrackEngine::beginStep(uint32_t tick, uint32_t divisor) {
    int octave = _noteTrack.octave();
    int transpose = _noteTrack.transpose();
    int rotate = _noteTrack.rotate();

    bool fillStep = fill() && (rng.nextRange(100) < uint32_t(fillAmount()));
    bool useFillGates = fillStep && _noteTrack.fillMode() == NoteTrack::FillMode::Gates;
    bool useFillSequence = fillStep && _noteTrack.fillMode() == NoteTrack::FillMode::NextPattern;
    bool useFillCondition = fillStep && _noteTrack.fillMode() == NoteTrack::FillMode::Condition;

    const auto &sequence = *_sequence;
    const auto &evalSequence = useFillSequence ? *_fillSequence : *_sequence;

    _currentStep = SequenceUtils::rotateStep(_sequenceState.step(), sequence.firstStep(), sequence.lastStep(), rotate);
    if (_currentStep < 0) {
        _stepGate = false;
        _stepSlide = false;
        _retrigCount = 1;
        _totalSlots = 1;
        _stepTotalTicks = divisor;
        _stepStartTick = tick;
        return;
    }

    const auto &step = evalSequence.step(_currentStep);

    _stepGate = evalStepGate(step) || useFillGates;
    if (_stepGate) {
        _stepGate = evalStepCondition(step.condition(), _sequenceState.iteration(), useFillCondition, _prevCondition, true);
    }

    // A failing retrigger condition collapses the step to a single slice, so
    // the arp offset never gets applied either.
    bool retrigOn = evalStepCondition(step.retriggerCondition(), _sequenceState.iteration(), useFillCondition, _prevCondition, false);
    _retrigCount = retrigOn ? step.retrigger() : 1;

    int repeatCount = step.repeat();
    _totalSlots = std::max(1, _retrigCount * repeatCount);

    // repeats extend the step rather than subdividing it
    _stepTotalTicks = NoteSequence::stepLengthTicks(step.stepLength(), divisor) * uint32_t(repeatCount);
    _stepStartTick = tick;

    _gateLength = step.gateLength();
    _arpRetrigger = step.arpRetrigger();
    _stepSlide = step.slide();

    const auto &scale = _model.project().selectedScale();
    _baseNote = step.note() + evalTransposition(scale, octave, transpose);
}

void NoteTrackEngine::advanceSlot(uint32_t tick, uint32_t divisor) {
    if (_slotIndex == 0) {
        if (!_linkedTrackEngine) {
            const auto &sequence = *_sequence;
            _sequenceState.advanceFree(sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
            recordStep(tick, divisor);
            beginStep(tick, divisor);
        }
    }

    _started = true;

    uint32_t onset = slotOnset(_slotIndex);
    uint32_t nextOnset = slotOnset(_slotIndex + 1);
    uint32_t slotLen = nextOnset > onset ? nextOnset - onset : 1;

    bool legato = _noteTrack.gateMode() == NoteTrack::GateMode::Legato && _gateLength >= 100;

    if (_stepGate) {
        // Gate length is a percentage of the retrigger slice, not of the whole
        // step -- measured against the step it would make retriggered gates
        // overlap each other.
        uint32_t gateTicks = std::max(uint32_t(1), (slotLen * uint32_t(_gateLength)) / 100);
        if (!legato) {
            // The gate must close strictly before the next onset, otherwise the
            // gate-off and the next gate-on land on the same tick and the queue
            // collapses them into a permanently high gate.
            gateTicks = std::min(gateTicks, slotLen > 1 ? slotLen - 1 : slotLen);
        }

        if (!_legatoHeld) {
            _gateQueue.pushReplace({ Groove::applySwing(onset, swing()), true });
        }
        if (legato) {
            _legatoHeld = true;
        } else {
            _gateQueue.pushReplace({ Groove::applySwing(onset + gateTicks, swing()), false });
            _legatoHeld = false;
        }

        // Arp retrigger walks the scale once per repeat; the slide flag rides
        // only on the first slot so intra-step arp jumps snap rather than glide.
        int slice = _retrigCount > 0 ? (_slotIndex % _retrigCount) : 0;
        const auto &scale = _model.project().selectedScale();
        int rootNote = _model.project().rootNote();
        int note = NoteSequence::Note::clamp(_baseNote + slice * _arpRetrigger);
        bool slide = _stepSlide && _slotIndex == 0;

        _cvQueue.push({ Groove::applySwing(onset, swing()), noteToVolts(note, scale, rootNote), slide });
    } else if (_legatoHeld) {
        // a gate-off step ends any held legato and otherwise does nothing --
        // no gate, no note change for the whole duration of the step
        _gateQueue.pushReplace({ Groove::applySwing(onset, swing()), false });
        _legatoHeld = false;
    }

    _slotIndex++;
    if (_slotIndex >= _totalSlots) {
        _slotIndex = 0;
        _nextSlotTick = _stepStartTick + _stepTotalTicks;
    } else {
        _nextSlotTick = slotOnset(_slotIndex);
    }
}

void NoteTrackEngine::recordStep(uint32_t tick, uint32_t divisor) {

    if (!_engine.state().recording() || _model.project().recordMode() == Types::RecordMode::StepRecord || _sequenceState.prevStep()==-1) {
        return;
    }

    bool stepWritten = false;

    auto writeStep = [this, divisor, &stepWritten] (int stepIndex, int note, int lengthTicks) {
        auto &step = _sequence->step(stepIndex);
        int gateLength = clamp(int((lengthTicks * 100) / int(divisor)), 1, 100);

        step.setGate(true);
        step.setStepLength(NoteSequence::StepLengthDefault);
        step.setGateLength(gateLength);
        step.setRetrigger(1);
        step.setArpRetrigger(0);
        step.setRepeat(1);
        step.setNote(noteFromMidiNote(note));
        step.setCondition(Types::Condition::Off);
        step.setRetriggerCondition(Types::Condition::Off);

        stepWritten = true;
    };

    auto clearStep = [this] (int stepIndex) {
        auto &step = _sequence->step(stepIndex);

        step.clear();
    };

    uint32_t stepStart = tick - divisor;
    uint32_t stepEnd = tick;
    uint32_t margin = divisor / 2;

    for (size_t i = 0; i < _recordHistory.size(); ++i) {
        if (_recordHistory[i].type != RecordHistory::Type::NoteOn) {
            continue;
        }

        int note = _recordHistory[i].note;
        uint32_t noteStart = _recordHistory[i].tick;
        uint32_t noteEnd = i + 1 < _recordHistory.size() ? _recordHistory[i + 1].tick : tick;

        if (noteStart >= stepStart - margin && noteStart < stepStart + margin) {
            // note on during step start phase
            if (noteEnd >= stepEnd) {
                // note hold during step
                int length = std::min(noteEnd, stepEnd) - stepStart;
                writeStep(_sequenceState.prevStep(), note, length);
            } else {
                // note released during step
                int length = noteEnd - noteStart;
                writeStep(_sequenceState.prevStep(), note, length);
            }
        } else if (noteStart < stepStart && noteEnd > stepStart) {
            // note on during previous step
            int length = std::min(noteEnd, stepEnd) - stepStart;
            writeStep(_sequenceState.prevStep(), note, length);
        }
    }

    if (isSelected() && !stepWritten && _model.project().recordMode() == Types::RecordMode::Overwrite) {
        clearStep(_sequenceState.prevStep());
    }
}

int NoteTrackEngine::noteFromMidiNote(uint8_t midiNote) const {
    const auto &scale = _model.project().selectedScale();
    int rootNote = _model.project().rootNote();

    if (scale.isChromatic()) {
        return scale.noteFromVolts((midiNote - 60 - rootNote) * (1.f / 12.f));
    } else {
        return scale.noteFromVolts((midiNote - 60) * (1.f / 12.f));
    }
}
