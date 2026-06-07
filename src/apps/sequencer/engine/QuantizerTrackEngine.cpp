#include "QuantizerTrackEngine.h"

#include "Engine.h"
#include "TrackEngineHelpers.h"
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

void QuantizerTrackEngine::setLoopMode(LoopMode mode) {
    if (mode == LoopMode::Rec) {
        _loopFillCount = 0;
        _loopIndex = 0;
    } else if (mode == LoopMode::Loop) {
        _loopIndex = 0;
    }
    _loopMode = mode;
}

int QuantizerTrackEngine::loopPlaySlot() const {
    if (_loopMode != LoopMode::Loop) return -1;
    const int loopLen = std::max(1, _quantizerTrack.loopLength());
    // _loopIndex has already been advanced past the last-played slot, so step back one.
    const int lastIdx = (_loopIndex + loopLen - 1) % loopLen;
    return (_quantizerTrack.loopStart() + lastIdx) % LoopBufferSize;
}

void QuantizerTrackEngine::reset() {
    _sequenceState.reset();
    _currentStep = -1;
    _lastQNote = INT32_MIN;
    _lastQVolts = 0.f;
    _lastTransposition = INT32_MIN;
    _lastSourceGate = false;
    _lastTriggerTrack = -1;
    _lastCvChannel    = -1;
    _filterInit = false;
    _filteredInput = 0.f;
    _samplePending = false;
    _sampleTick = 0;
    _gateOutput = false;
    if (_model.project().resetCvOnStop()) {
        _cvOutput = 0.f;
    }
    _pulseTick = 0;
    _loopMode = LoopMode::Play;
    _loopFillCount = 0;
    _loopIndex = 0;

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

    // Helper: commit a quantized note to CV/gate output.
    // In Rec mode: capture into loop buffer and auto-switch to Loop when full.
    // In Loop mode: replay from buffer, ignoring the live qNote.
    auto commit = [&](int qNote) {
        _lastQNote = qNote;
        _lastQVolts = scale.noteToVolts(qNote);  // un-transposed, used for hysteresis
        _lastTransposition = transposition;

        if (_loopMode == LoopMode::Rec) {
            if (_loopFillCount < LoopBufferSize) {
                // Store un-transposed note index so octave/transpose edits apply during playback.
                _loopBuffer[_loopFillCount++] = qNote;
            }
            if (_loopFillCount >= LoopBufferSize) {
                _loopMode = LoopMode::Loop;
                _loopIndex = 0;
            }
            _cvOutput = scale.noteToVolts(qNote + transposition);
        } else if (_loopMode == LoopMode::Loop) {
            const int loopLen = std::max(1, _quantizerTrack.loopLength());
            const int slot    = (_quantizerTrack.loopStart() + _loopIndex) % LoopBufferSize;
            _cvOutput  = scale.noteToVolts(_loopBuffer[slot] + transposition);
            _loopIndex = (_loopIndex + 1) % loopLen;
        } else {
            _cvOutput = scale.noteToVolts(qNote + transposition);
        }

        _gateOutput = true;
        _pulseTick = tick + PulseLengthTicks;
        result |= TickResult::GateUpdate | TickResult::CvUpdate;
    };

    // Commit pending delayed sample first — prevents perpetual deferral when
    // divisor <= SampleDelayTicks (new trigger would overwrite sampleTick).
    if (_samplePending && tick >= _sampleTick) {
        commit(scale.noteFromVolts(inputVolts));
        _samplePending = false;
    }

    // ── Sequence clock (always runs regardless of trigger mode) ───────────────
    // Advances step display, link data, and fires gate output on active steps.
    // Having the clock always run means the GATE tab playhead is always visible
    // and this track can always serve as a trigger source for other tracks.
    const auto &sequence = *_sequence;
    uint32_t divisor, relativeTick;
    computeClockParams(sequence, tick, _engine.measureDivisor(), divisor, relativeTick);

    if (relativeTick == 0) {
        _sequenceState.reset();
    }
    if (relativeTick % divisor == 0) {
        const int absoluteStep = int(relativeTick / divisor);
        _sequenceState.advanceAligned(absoluteStep, sequence.runMode(), sequence.firstStep(), sequence.lastStep(), rng);
        _currentStep = _sequenceState.step();
        const auto &step = sequence.step(_currentStep);
        // Schedule a sample on active steps; Loop mode bypasses the gate pattern.
        // Gate fires via commit() so it is always atomic with the CV update.
        // Free trigger mode drives its own commits via hysteresis — the clock must not
        // add a second commit path or Rec mode would fill the buffer with duplicates.
        // Exception: Free+Loop still needs the clock to advance the loop (hysteresis
        // is disabled in Loop mode).
        const bool clockShouldSample =
            _quantizerTrack.triggerMode() != QuantizerTrack::TriggerMode::Free ||
            _loopMode == LoopMode::Loop;
        if (clockShouldSample && (step.gate() || _loopMode == LoopMode::Loop) && !_samplePending) {
            _samplePending = true;
            _sampleTick    = tick + SampleDelayTicks;
        }
    }
    updateLinkData(divisor, relativeTick, &_sequenceState);

    // ── Trigger mode: controls when the CV input is sampled ───────────────────
    switch (_quantizerTrack.triggerMode()) {
    case QuantizerTrack::TriggerMode::Free: {
        // In Loop mode skip hysteresis — loop advances via the sequence clock above.
        if (_loopMode != LoopMode::Loop) {
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
        }
        break;
    }

    case QuantizerTrack::TriggerMode::Internal:
        // Deprecated — step advancement now handled by the sequence clock above.
        break;

    case QuantizerTrack::TriggerMode::External: {
        int triggerTrack = _quantizerTrack.triggerTrack();
        if (triggerTrack >= 0 && triggerTrack < CONFIG_TRACK_COUNT) {
            bool curGate = _engine.trackEngine(triggerTrack).gateOutput(0);
            if (triggerTrack != _lastTriggerTrack) {
                // Source changed — absorb current level to prevent spurious rising edge.
                _lastSourceGate   = curGate;
                _lastTriggerTrack = triggerTrack;
            }
            // In Loop mode the sequence clock drives advancement; skip external sampling.
            if (_loopMode != LoopMode::Loop && curGate && !_lastSourceGate) {
                _samplePending = true;
                _sampleTick    = tick + SampleDelayTicks;
            }
            _lastSourceGate = curGate;
        }
        break;
    }

    case QuantizerTrack::TriggerMode::CvGate: {
        int cvCh = _quantizerTrack.triggerTrack();  // 0–3 = CV input index
        if (cvCh >= 0 && cvCh < 4) {
            float cvIn = _engine.cvInput().channel(cvCh);
            // Schmitt-trigger hysteresis: rise above High, fall below Low
            bool curGate = (cvIn > CvGateThresholdHigh) ||
                           (_lastSourceGate && cvIn > CvGateThresholdLow);
            if (cvCh != _lastCvChannel) {
                // Channel changed (or first entry from another mode) — absorb current level.
                _lastSourceGate = curGate;
                _lastCvChannel  = cvCh;
            }
            // In Loop mode the sequence clock drives advancement; skip external sampling.
            if (_loopMode != LoopMode::Loop && curGate && !_lastSourceGate) {
                _samplePending = true;
                _sampleTick    = tick + SampleDelayTicks;
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
