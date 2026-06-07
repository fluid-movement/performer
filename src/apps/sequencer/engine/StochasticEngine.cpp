#include "StochasticEngine.h"

#include "Engine.h"
#include "TrackEngineHelpers.h"
#include "Groove.h"
#include "SequenceState.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/StochasticSequence.h"
#include "model/Scale.h"
#include <algorithm>

static Random rng;
static Random mutationRng;

// Draw a scale degree (0–6) from the sequence's degreeProb distribution.
// Returns -1 if all weights are 0.
static int drawDegree(const StochasticSequence &sequence) {
    int sum = 0;
    for (int i = 0; i < 7; i++) sum += sequence.degreeProb(i);
    if (sum == 0) return -1;
    int rnd = rng.nextRange(sum);
    for (int i = 0; i < 7; i++) {
        int w = sequence.degreeProb(i);
        if (rnd < w) return i;
        rnd -= w;
    }
    return -1;
}

// Draw an octave index (0–4 → offset -2..+2). Returns 2 (centre) if all weights are 0.
static int drawOctave(const StochasticSequence &sequence) {
    int sum = 0;
    for (int i = 0; i < 5; i++) sum += sequence.octaveProb(i);
    if (sum == 0) return 2;
    int rnd = rng.nextRange(sum);
    for (int i = 0; i < 5; i++) {
        int w = sequence.octaveProb(i);
        if (rnd < w) return i;
        rnd -= w;
    }
    return 2;
}

// Draw a duration index (0–5). Returns 2 (quarter note) if all weights are 0.
static int drawDuration(const StochasticSequence &sequence) {
    int sum = 0;
    for (int i = 0; i < 6; i++) sum += sequence.durationProb(i);
    if (sum == 0) return 2;
    int rnd = rng.nextRange(sum);
    for (int i = 0; i < 6; i++) {
        int w = sequence.durationProb(i);
        if (rnd < w) return i;
        rnd -= w;
    }
    return 2;
}

// Duration index → step length in ticks. index 2 = quarter note = 1 divisor.
static const int DURATION_MULTIPLIERS[6] = { 1, 2, 4, 8, 16, 32 };

static uint32_t durationToTicks(int durationIndex, uint32_t divisor) {
    return (divisor / 4) * DURATION_MULTIPLIERS[durationIndex];
}

// Convert scale degree + octave offset to CV voltage.
static float degreeToCv(int degreeIndex, int octaveOffset, const Scale &scale, int rootNote, int trackOctave, int transpose) {
    int note = degreeIndex + (trackOctave + octaveOffset) * scale.notesPerOctave() + transpose;
    return scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
}

void StochasticEngine::reset() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
    _currentDegreeIndex = -1;
    _prevCondition = false;
    _activity = false;
    _gateOutput = false;
    _slideActive = false;
    _loopPhaseOffset = 0;
    _lastSeqLen = 0;
    if (_model.project().resetCvOnStop()) {
        _cvOutput = 0.f;
        _cvOutputTarget = 0.f;
    }
    _gateQueue.clear();
    _cvQueue.clear();
    _recordHistory.clear();
    rng = Random(time(NULL));
    changePattern();
}

void StochasticEngine::restart() {
    _freeRelativeTick = 0;
    _sequenceState.reset();
    _currentStep = -1;
    _currentDegreeIndex = -1;
    rng = Random(time(NULL));
}

TrackEngine::TickResult StochasticEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");
    const auto &sequence = *_sequence;
    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    if (linkData) {
        _linkData = *linkData;
        _sequenceState = *linkData->sequenceState;

        if (linkData->relativeTick % linkData->divisor == 0) {
            recordStep(tick, linkData->divisor);
            triggerStep(tick, linkData->divisor);
        }
    } else {
        uint32_t divisor, relativeTick;
        computeClockParams(sequence, tick, _engine.measureDivisor(), divisor, relativeTick);

        if (pastStepsToStop(int(_model.project().stepsToStop()), relativeTick, divisor)) {
            _engine.clockStop();
        }

        if (relativeTick == 0) {
            reset();
            _currentStageRepeat = 1;
        }
        const auto &sequence = *_sequence;

        switch (_stochasticTrack.playMode()) {
        case Types::PlayMode::Aligned: {
            if (relativeTick % divisor == 0) {
                if (sequence.useLoop()) {
                    _sequenceState.calculateNextStepAligned(
                        relativeTick / divisor,
                        sequence.runMode(),
                        sequence.sequenceFirstStep(),
                        sequence.sequenceLastStep(),
                        rng
                    );
                    triggerStep(tick, divisor, true);
                } else {
                    _sequenceState.advanceAligned(relativeTick / divisor, sequence.runMode(), 0, sequence.bufferLoopLength() - 1, rng);
                    triggerStep(tick, divisor);
                }
            }
        }
            break;
        case Types::PlayMode::Free:
            // Not yet implemented. When added: advance _freeRelativeTick per tick,
            // call triggerStep() on division boundaries (mirror NoteTrackEngine::Free path).
            break;
        case Types::PlayMode::Last:
            break;
        }

        updateLinkData(divisor, relativeTick, &_sequenceState);
    }

    auto &midiOutputEngine = _engine.midiOutputEngine();

    TickResult result = TickResult::NoUpdate;

    drainGateQueue(tick, _gateQueue, _activity, _gateOutput,
        _monitorOverrideActive, mute(), fill(), midiOutputEngine, _track.trackIndex(), result);
    drainCvQueue(tick, _cvQueue, _cvOutputTarget, _slideActive,
        _monitorOverrideActive, mute(),
        _stochasticTrack.cvUpdateMode() == StochasticTrack::CvUpdateMode::Always,
        midiOutputEngine, _track.trackIndex(), result);

    return result;
}

void StochasticEngine::update(float dt) {
    bool running = _engine.state().running();

    auto sendToMidiOutputEngine = [this] (bool gate, float cv = 0.f) {
        auto &midiOutputEngine = _engine.midiOutputEngine();
        midiOutputEngine.sendGate(_track.trackIndex(), gate);
        if (gate) {
            midiOutputEngine.sendCv(_track.trackIndex(), cv);
            midiOutputEngine.sendSlide(_track.trackIndex(), false);
        }
    };

    auto clearOverride = [&] () {
        if (_monitorOverrideActive) {
            _activity = _gateOutput = false;
            _monitorOverrideActive = false;
            sendToMidiOutputEngine(false);
        }
    };

    // No step monitoring in V2 (no per-step notes)
    bool liveMonitoring =
        (_model.project().monitorMode() == Types::MonitorMode::Always) ||
        (_model.project().monitorMode() == Types::MonitorMode::Stopped && !running);

    if (liveMonitoring && _recordHistory.isNoteActive()) {
        const auto &scale = _model.project().selectedScale();
        int rootNote = _model.project().rootNote();
        int octave = _stochasticTrack.octave();
        int transpose = _stochasticTrack.transpose();
        int note = octave * scale.notesPerOctave() + transpose;
        float cv = scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
        _cvOutputTarget = cv;
        _activity = _gateOutput = true;
        _monitorOverrideActive = true;
        sendToMidiOutputEngine(true, cv);
    } else {
        clearOverride();
    }

    applySlideUpdate(_cvOutput, _cvOutputTarget, _slideActive, _stochasticTrack.slideTime(), dt);
}

void StochasticEngine::changePattern() {
    _sequence = &_stochasticTrack.sequence(pattern());
    _fillSequence = &_stochasticTrack.sequence(std::min(pattern() + 1, CONFIG_PATTERN_COUNT - 1));
}

void StochasticEngine::monitorMidi(uint32_t tick, const MidiMessage &message) {
    _recordHistory.write(tick, message);
}

void StochasticEngine::clearMidiMonitoring() {
    _recordHistory.clear();
}

void StochasticEngine::setMonitorStep(int /*index*/) {
    // No per-step monitoring in V2
}

void StochasticEngine::triggerStep(uint32_t tick, uint32_t divisor, bool forNextStep) {
    int octave = _stochasticTrack.octave();
    int transpose = _stochasticTrack.transpose();

    bool fillStep = fill() && (rng.nextRange(100) < uint32_t(fillAmount()));
    bool useFillGates = fillStep && _stochasticTrack.fillMode() == StochasticTrack::FillMode::Gates;
    bool useFillSequence = fillStep && _stochasticTrack.fillMode() == StochasticTrack::FillMode::NextPattern;

    auto &sequence = *_sequence;
    const auto &evalSequence = useFillSequence ? *_fillSequence : *_sequence;

    uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
    uint32_t relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;
    auto absoluteStep = relativeTick / divisor;

    _index = absoluteStep % sequence.sequenceLength();

    uint32_t stepTick = tick;
    bool stepGate = false;
    float noteValue = 0.f;
    uint32_t stepLength = durationToTicks(2, divisor);  // default: quarter note

    // Reseed in non-loop mode
    if (!sequence.useLoop() && sequence.reseed() && !sequence.isEmpty()) {
        rng = Random(time(NULL));
        sequence.setReseed(0, false);
    }

    // Clear loop: reset buffer and return to free-running mode
    if (sequence.clearLoop()) {
        if (_index == 0) {
            _lockedSteps.clear();
            sequence.setClearLoop(false);
            sequence.setUseLoop(false);
            _sequenceState.reset();
            _index = -1;
        }
    }

    // Phase 1: fill buffer or free-running
    if (!sequence.useLoop() || int(_lockedSteps.size()) < sequence.bufferLoopLength()) {
        // Automatic density: gate probability = average active-bar height / max height.
        // Normalising by numNonZero means one bar at full always gives 100% density.
        int sum = 0, numNonZero = 0;
        for (int i = 0; i < 7; i++) {
            int w = evalSequence.degreeProb(i);
            sum += w;
            if (w > 0) numNonZero++;
        }
        int denom = numNonZero * 15;
        stepGate = useFillGates || (sum > 0 && int(rng.nextRange(denom)) < sum);

        int degreeIdx = -1;
        int octaveIdx = 2;
        if (stepGate) {
            degreeIdx = drawDegree(evalSequence);
            if (degreeIdx < 0) { stepGate = false; }
        }

        if (stepGate) {
            octaveIdx = drawOctave(evalSequence);
            int octaveOffset = octaveIdx - 2;  // map 0-4 → -2..+2
            const auto &scale = _model.project().selectedScale();
            int rootNote = _model.project().rootNote();
            noteValue = degreeToCv(degreeIdx, octaveOffset, scale, rootNote, octave, transpose);
            _currentDegreeIndex = degreeIdx;
            _currentStep = degreeIdx;
        }

        int durIdx = drawDuration(evalSequence);
        stepLength = durationToTicks(durIdx, divisor);

        if (int(_lockedSteps.size()) < sequence.bufferLoopLength()) {
            _lockedSteps.push_back(StochasticLoopStep(degreeIdx, octaveIdx, stepGate, stepLength));
        }

        if (stepGate) {
            _gateQueue.pushReplace({ Groove::applySwing(stepTick, swing()), true });
            _gateQueue.pushReplace({ Groove::applySwing(stepTick + stepLength, swing()), false });
        }
        if (stepGate || _stochasticTrack.cvUpdateMode() == StochasticTrack::CvUpdateMode::Always) {
            _cvQueue.push({ Groove::applySwing(stepTick, swing()), noteValue, false });
        }
        return;
    }

    // Phase 2: loop replay
    if (sequence.useLoop() && int(_lockedSteps.size()) >= sequence.bufferLoopLength()) {
        int seqLen = sequence.loopLength();

        // When loop length changes, anchor the phase so the loop restarts from position 0.
        if (seqLen != _lastSeqLen) {
            _loopPhaseOffset = absoluteStep;
            _lastSeqLen = seqLen;
        }

        _index = (absoluteStep - _loopPhaseOffset) % seqLen;

        // Wrap-around buffer access: firstStep is a phase offset into the 16-step ring.
        int bufIdx = (sequence.sequenceFirstStep() + _index) % int(_lockedSteps.size());

        const auto &lockedStep = _lockedSteps.at(bufIdx);
        stepGate = lockedStep.gate();
        stepLength = lockedStep.stepLength();
        int degreeIdx = lockedStep.degreeIndex();
        if (stepGate) {
            int octaveOffset = lockedStep.octaveIndex() - 2;
            const auto &scale = _model.project().selectedScale();
            int rootNote = _model.project().rootNote();
            noteValue = degreeToCv(degreeIdx, octaveOffset, scale, rootNote, octave, transpose);
        }

        // Mutation: re-draw this buffer slot with probability loopChance/15
        if (sequence.loopChance() > 0 && int(mutationRng.nextRange(15)) < sequence.loopChance()) {
            int sum = 0, numNonZero = 0;
            for (int i = 0; i < 7; i++) {
                int w = sequence.degreeProb(i);
                sum += w;
                if (w > 0) numNonZero++;
            }
            int denom = numNonZero * 15;
            bool newGate = sum > 0 && int(rng.nextRange(denom)) < sum;

            int newDegreeIdx = -1;
            int newOctaveIdx = 2;
            float newNote = 0.f;
            if (newGate) {
                newDegreeIdx = drawDegree(sequence);
                if (newDegreeIdx < 0) newGate = false;
            }
            if (newGate) {
                newOctaveIdx = drawOctave(sequence);
                int octaveOffset = newOctaveIdx - 2;
                const auto &scale = _model.project().selectedScale();
                int rootNote = _model.project().rootNote();
                newNote = degreeToCv(newDegreeIdx, octaveOffset, scale, rootNote, octave, transpose);
            }
            int newDurIdx = drawDuration(sequence);
            uint32_t newLen = durationToTicks(newDurIdx, divisor);

            _lockedSteps[bufIdx] =
                StochasticLoopStep(newDegreeIdx, newOctaveIdx, newGate, newLen);

            stepGate = newGate;
            noteValue = newNote;
            stepLength = newLen;
            degreeIdx = newDegreeIdx;
        }

        if (stepGate) {
            _currentDegreeIndex = degreeIdx;
            _currentStep = degreeIdx;
            _gateQueue.pushReplace({ Groove::applySwing(stepTick, swing()), true });
            _gateQueue.pushReplace({ Groove::applySwing(stepTick + stepLength, swing()), false });
        }
        if (stepGate || _stochasticTrack.cvUpdateMode() == StochasticTrack::CvUpdateMode::Always) {
            _cvQueue.push({ Groove::applySwing(stepTick, swing()), noteValue, false });
        }
    }
}

void StochasticEngine::triggerStep(uint32_t tick, uint32_t divisor) {
    triggerStep(tick, divisor, false);
}

void StochasticEngine::recordStep(uint32_t /*tick*/, uint32_t /*divisor*/) {
    // No MIDI step recording in V2 (no per-step note grid)
}

