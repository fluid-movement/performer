#include "QuantizerTrackEngine.h"

#include "Engine.h"
#include "SequenceUtils.h"

#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/Scale.h"

#include <climits>

static Random rng;

static int evalTransposition(const Scale &scale, int octave, int transpose) {
    return octave * scale.notesPerOctave() + transpose;
}

float QuantizerTrackEngine::readInput() const {
    const auto &src = _quantizerTrack.inputSource();
    float raw;
    if (QuantizerTrack::inputSourceIsCvIn(src)) {
        raw = _engine.cvInput().channel(QuantizerTrack::inputSourceCvIndex(src));
    } else {
        int trackIdx = QuantizerTrack::inputSourceTrackIndex(src);
        if (trackIdx >= 0 && trackIdx < CONFIG_TRACK_COUNT && trackIdx != _track.trackIndex()) {
            raw = _engine.trackEngine(trackIdx).cvOutput(0);
        } else {
            raw = 0.f;
        }
    }
    if (!_filterInit) { _filteredInput = raw; _filterInit = true; }
    else if (std::abs(raw - _filteredInput) > FilterSnapVolts) { _filteredInput = raw; }
    else _filteredInput = InputFilterAlpha * raw + (1.f - InputFilterAlpha) * _filteredInput;
    return _filteredInput;
}

void QuantizerTrackEngine::reset() {
    _sequenceState.reset();
    _currentStep = -1;
    _lastQNote = INT32_MIN;
    _lastQVolts = 0.f;
    _lastTransposition = INT32_MIN;
    _lastSourceGate = false;
    _lastTriggerTrack = -1;
    _filterInit = false;
    _filteredInput = 0.f;
    _samplePending = false;
    _sampleTick = 0;
    _gateOutput = false;
    if (_model.project().resetCvOnStop()) {
        _cvOutput = 0.f;
    }
    _pulseTick = 0;

    changePattern();
}

void QuantizerTrackEngine::restart() {
    _sequenceState.reset();
    _currentStep = -1;
    _samplePending = false;
}

TrackEngine::TickResult QuantizerTrackEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");

    const auto &scale = _model.project().selectedScale();
    // IIR-filtered input runs every tick regardless of trigger mode
    float inputVolts = readInput();

    TickResult result = TickResult::NoUpdate;

    int transposition = evalTransposition(scale, _quantizerTrack.octave(), _quantizerTrack.transpose());

    // Helper: commit a quantized note to CV/gate output
    auto commit = [&](int qNote) {
        _lastQNote = qNote;
        _lastQVolts = scale.noteToVolts(qNote);  // un-transposed, used for hysteresis
        _lastTransposition = transposition;
        _cvOutput = scale.noteToVolts(qNote + transposition);
        _gateOutput = true;
        _pulseTick = tick + PulseLengthTicks;
        result |= TickResult::GateUpdate | TickResult::CvUpdate;
    };

    // Commit delayed sample before processing new triggers — prevents perpetual
    // deferral when divisor <= SampleDelayTicks (new trigger would overwrite sampleTick
    // before the commit check ran).
    if (_samplePending && tick >= _sampleTick) {
        commit(scale.noteFromVolts(inputVolts));
        _samplePending = false;
    }

    switch (_quantizerTrack.triggerMode()) {
    case QuantizerTrack::TriggerMode::Free: {
        // Hysteresis: only commit if input has moved far enough from current note
        int candidate = scale.noteFromVolts(inputVolts);
        bool changed = false;
        if (_lastQNote == INT32_MIN) {
            changed = true;
        } else if (candidate != _lastQNote) {
            changed = std::abs(inputVolts - _lastQVolts) >= HysteresisVolts;
        } else if (transposition != _lastTransposition) {
            changed = true;
        } else if (std::abs(scale.noteToVolts(_lastQNote) - _lastQVolts) > 0.001f) {
            // Scale changed under the current note — re-commit at new scale's pitch
            changed = true;
        }
        if (changed) {
            commit(candidate);
        }
        break;
    }

    case QuantizerTrack::TriggerMode::Internal: {
        const auto &sequence = *_sequence;
        uint32_t divisor = sequence.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
        uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
        uint32_t relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;

        if (relativeTick == 0) {
            _sequenceState.reset();
        }

        if (relativeTick % divisor == 0) {
            int absoluteStep = int(relativeTick / divisor);
            _sequenceState.advanceAligned(absoluteStep, sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
            _currentStep = _sequenceState.step();
            const auto &step = sequence.step(_currentStep);
            if (step.gate()) {
                // Schedule delayed sample to let the input CV settle
                _samplePending = true;
                _sampleTick = tick + SampleDelayTicks;
            }
        }

        _linkData.divisor = divisor;
        _linkData.relativeTick = relativeTick;
        _linkData.sequenceState = &_sequenceState;
        break;
    }

    case QuantizerTrack::TriggerMode::External: {
        int triggerTrack = _quantizerTrack.triggerTrack();
        if (triggerTrack >= 0 && triggerTrack < CONFIG_TRACK_COUNT) {
            bool curGate = _engine.trackEngine(triggerTrack).gateOutput(0);
            if (triggerTrack != _lastTriggerTrack) {
                // Trigger source changed — absorb current gate level to prevent
                // spurious rising-edge on the first tick after the switch
                _lastSourceGate = curGate;
                _lastTriggerTrack = triggerTrack;
            }
            if (curGate && !_lastSourceGate) {
                _samplePending = true;
                _sampleTick = tick + SampleDelayTicks;
            }
            _lastSourceGate = curGate;
        }
        break;
    }

    case QuantizerTrack::TriggerMode::Last:
        break;
    }

    if (_gateOutput && tick >= _pulseTick) {
        _gateOutput = false;
        result |= TickResult::GateUpdate;
    }

    return result;
}

void QuantizerTrackEngine::update(float dt) {
    // No slide or monitoring needed for quantizer
}

void QuantizerTrackEngine::changePattern() {
    _sequence = &_quantizerTrack.sequence(pattern());
}
