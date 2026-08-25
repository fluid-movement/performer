#include "CurveTrackEngine.h"

#include "Engine.h"
#include "TrackEngineHelpers.h"
#include "Groove.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/math/Math.h"
#include "core/utils/Random.h"

#include "model/Types.h"

#include <cmath>

static Random rng;

void CurveTrackEngine::reset() {
    _sequenceState.reset();
    _currentSegment  = -1;
    _currentPulse    = 0;
    _loopLength      = 0;
    _segmentFraction = 0.f;
    _activity = false;
    _gateOutput = false;

    _gateQueue.clear();

    changePattern();
}

void CurveTrackEngine::restart() {
    _sequenceState.reset();
    _currentSegment  = -1;
    _currentPulse    = 0;
    _segmentFraction = 0.f;
}

TrackEngine::TickResult CurveTrackEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");
    const auto &sequence = *_sequence;
    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    if (linkData) {
        _linkData = *linkData;
        _sequenceState = *linkData->sequenceState;

        // TODO: V1 recording not implemented

        if (linkData->relativeTick % linkData->divisor == 0) {
            advancePulse(tick, linkData->divisor);
        }

        updateOutput(linkData->relativeTick, linkData->divisor);
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

        // TODO: V1 recording not implemented

        if (relativeTick % divisor == 0) {
            advancePulse(tick, divisor);
        }

        updateOutput(relativeTick, divisor);

        updateLinkData(divisor, relativeTick, &_sequenceState);
    }

    TickResult result = TickResult::NoUpdate;

    while (!_gateQueue.empty() && tick >= _gateQueue.front().tick) {
        result |= TickResult::GateUpdate;
        _activity = _gateQueue.front().gate;
        _gateOutput = (!mute() || fill()) && _activity;
        _gateQueue.pop();

        _engine.midiOutputEngine().sendGate(_track.trackIndex(), _gateOutput);
    }

    return result;
}

void CurveTrackEngine::update(float dt) {
    if (!_engine.clockRunning()) {
        _cvOutputTarget = 0.f;
    }

    float offset = mute() ? 0.f : _curveTrack.offsetVolts();

    if (_curveTrack.slideTime() > 0) {
        _cvOutput = Slide::applySlide(_cvOutput, _cvOutputTarget + offset, _curveTrack.slideTime(), dt);
    } else {
        _cvOutput = _cvOutputTarget + offset;
    }
}

void CurveTrackEngine::changePattern() {
    _sequence = &_curveTrack.sequence(pattern());

    _loopLength = 0;
    for (int i = 0; i < _sequence->segmentCount(); ++i) {
        _loopLength += _sequence->step(i).length();
    }
    if (_loopLength <= 0) {
        _loopLength = 1;
    }
}

void CurveTrackEngine::advancePulse(uint32_t tick, uint32_t divisor) {
    const auto &seq = *_sequence;
    int count = seq.segmentCount();
    int first = clamp(seq.firstStep(), 0, count - 1);
    int last  = count - 1;
    auto mode = seq.runMode();

    if (_currentSegment < 0) {
        _sequenceState.advanceFree(mode, 0, last, rng);
        _currentSegment = (_sequenceState.step() + first) % count;
        _currentPulse   = 0;
        _gateQueue.pushReplace({ Groove::applySwing(tick, swing()), true });
        _gateQueue.pushReplace({ Groove::applySwing(tick + divisor / 8, swing()), false });
    } else {
        _currentPulse++;
        int segLen = seq.step(_currentSegment).length();
        if (_currentPulse >= segLen) {
            _sequenceState.advanceFree(mode, 0, last, rng);
            _currentSegment = (_sequenceState.step() + first) % count;
            _currentPulse   = 0;
            _gateQueue.pushReplace({ Groove::applySwing(tick, swing()), true });
            _gateQueue.pushReplace({ Groove::applySwing(tick + divisor / 8, swing()), false });
        }
    }

    // Recompute loop length (all segments) for progress display
    _loopLength = 0;
    for (int i = 0; i < count; ++i) _loopLength += seq.step(i).length();
    if (_loopLength <= 0) _loopLength = 1;
}

void CurveTrackEngine::updateOutput(uint32_t relativeTick, uint32_t divisor) {
    if (_currentSegment < 0) {
        return;
    }

    float intra = float(relativeTick % divisor) / float(divisor);
    int pulsesBeforeCurrent = 0;
    for (int i = 0; i < _currentSegment; ++i)
        pulsesBeforeCurrent += _sequence->step(i).length();
    _loopProgress = _loopLength > 0
        ? clamp((float(pulsesBeforeCurrent + _currentPulse) + intra) / float(_loopLength), 0.f, 1.f)
        : 0.f;

    const auto &range = Types::voltageRangeInfo(_curveTrack.range());

    if (mute()) {
        switch (_curveTrack.muteMode()) {
        case CurveTrack::MuteMode::LastValue:
            break;
        case CurveTrack::MuteMode::Zero:
            _cvOutputTarget = 0.f;
            break;
        case CurveTrack::MuteMode::Min:
            _cvOutputTarget = range.lo;
            break;
        case CurveTrack::MuteMode::Max:
            _cvOutputTarget = range.hi;
            break;
        case CurveTrack::MuteMode::Last:
            break;
        }
    } else {
        const auto &step = _sequence->step(_currentSegment);
        int len = step.length();
        float fraction = clamp((float(_currentPulse) + intra) / float(len), 0.f, 1.f);
        // Travelling backwards plays the segment mirrored. The curve is a pure
        // function of phase, so the time reverse is simply 1 - phase - every curve
        // has a mirrored twin for free. _segmentFraction gets the mirrored value so
        // the play scanline sweeps right to left and stays on the point of the drawn
        // curve that is actually reaching the jack.
        float phase = _sequenceState.direction() < 0 ? 1.f - fraction : fraction;
        _segmentFraction = phase;
        float amp   = CurveSequence::evalSegment(phase, step.shapeNorm(), step.skewNorm(), _curveTrack.shapeCurveExponent());
        float value = clamp(step.offsetNorm() + step.levelNorm() * amp, 0.f, 1.f);
        _cvOutputTarget = range.denormalize(value);
    }

    _engine.midiOutputEngine().sendCv(_track.trackIndex(), _cvOutputTarget);
}
