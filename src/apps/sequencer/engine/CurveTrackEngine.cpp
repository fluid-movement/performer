#include "CurveTrackEngine.h"

#include "Engine.h"
#include "Groove.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/math/Math.h"

#include "model/Types.h"

#include <cmath>

void CurveTrackEngine::reset() {
    _sequenceState.reset();
    _currentSegment  = -1;
    _currentPulse    = 0;
    _loopLength      = 0;
    _loopPulse       = -1;
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
    _loopPulse       = -1;
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
        uint32_t divisor = sequence.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
        uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
        uint32_t relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;

        if (int(_model.project().stepsToStop()) != 0 && int(relativeTick / divisor) == int(_model.project().stepsToStop())) {
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

        _linkData.divisor = divisor;
        _linkData.relativeTick = relativeTick;
        _linkData.sequenceState = &_sequenceState;
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
    _fillSequence = &_curveTrack.sequence(std::min(pattern() + 1, CONFIG_PATTERN_COUNT - 1));

    _loopLength = 0;
    for (int i = 0; i < _sequence->segmentCount(); ++i) {
        _loopLength += _sequence->step(i).length();
    }
    if (_loopLength <= 0) {
        _loopLength = 1;
    }
}

void CurveTrackEngine::advancePulse(uint32_t tick, uint32_t divisor) {
    // Recompute in case segments were edited live
    _loopLength = 0;
    for (int i = 0; i < _sequence->segmentCount(); ++i) {
        _loopLength += _sequence->step(i).length();
    }
    if (_loopLength <= 0) _loopLength = 1;

    _loopPulse = (_loopPulse + 1) % _loopLength;

    int pulse = 0;
    for (int i = 0; i < _sequence->segmentCount(); ++i) {
        int len = _sequence->step(i).length();
        if (_loopPulse < pulse + len) {
            bool segmentStart = (_loopPulse == pulse);
            _currentSegment  = i;
            _currentPulse    = _loopPulse - pulse;
            _segmentFraction = float(_currentPulse) / float(len);
            if (segmentStart) {
                _gateQueue.pushReplace({ Groove::applySwing(tick, swing()), true });
                _gateQueue.pushReplace({ Groove::applySwing(tick + divisor / 8, swing()), false });
            }
            break;
        }
        pulse += len;
    }
}

void CurveTrackEngine::updateOutput(uint32_t relativeTick, uint32_t divisor) {
    if (_currentSegment < 0) {
        return;
    }

    float intra = float(relativeTick % divisor) / float(divisor);
    _loopProgress = _loopLength > 0
        ? clamp((float(_loopPulse) + intra) / float(_loopLength), 0.f, 1.f)
        : 0.f;

    const auto &range = Types::voltageRangeInfo(_sequence->range());

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
        _segmentFraction = fraction;
        float amp   = CurveSequence::evalSegment(fraction, step.shapeNorm(), step.skewNorm());
        float value = clamp(step.offsetNorm() + step.levelNorm() * amp, 0.f, 1.f);
        _cvOutputTarget = range.denormalize(value);
    }

    _engine.midiOutputEngine().sendCv(_track.trackIndex(), _cvOutputTarget);
}
