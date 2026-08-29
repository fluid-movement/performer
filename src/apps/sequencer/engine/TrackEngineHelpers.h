#pragma once

#include "Config.h"
#include "MidiOutputEngine.h"
#include "Slide.h"
#include "TrackEngine.h"

#include <cstdint>

// ---------------------------------------------------------------------------
// Clock helpers
// ---------------------------------------------------------------------------

// Compute divisor and relativeTick from a sequence and current engine tick.
// measureDivisor comes from Engine::measureDivisor(); passed explicitly to
// avoid including Engine.h (which would create a circular dependency).
template<typename Seq>
inline void computeClockParams(const Seq &seq, uint32_t tick, uint32_t measureDivisor,
    uint32_t &divisor, uint32_t &relativeTick) {
    divisor = seq.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
    uint32_t resetDivisor = seq.resetMeasure() * measureDivisor;
    relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;
}

// Returns true when the sequencer should stop at the configured step limit.
inline bool pastStepsToStop(int stepsToStop, uint32_t relativeTick, uint32_t divisor) {
    return stepsToStop != 0 && int(relativeTick / divisor) == stepsToStop;
}

// ---------------------------------------------------------------------------
// Queue drain helpers (Note, Arp, Stochastic share these patterns)
// ---------------------------------------------------------------------------

// Drain one gate queue entry per tick. Gate half is 100% identical across
// all three queue-using engines — extracted here to prevent silent drift
// when mute/fill/monitor semantics change.
template<typename GateQueue>
inline void drainGateQueue(uint32_t tick, GateQueue &q,
    bool &activity, bool &gateOutput,
    bool monitorOverrideActive, bool muted, bool filled,
    MidiOutputEngine &midi, int trackIndex,
    TrackEngine::TickResult &result) {
    while (!q.empty() && tick >= q.front().tick) {
        if (!monitorOverrideActive) {
            result |= TrackEngine::TickResult::GateUpdate;
            activity = q.front().gate;
            gateOutput = (!muted || filled) && activity;
            midi.sendGate(trackIndex, gateOutput);
        }
        q.pop();
    }
}

// Drain one CV queue entry per tick. cvAlways = (track.cvUpdateMode() == Always),
// evaluated at the call site so no track-type dependency bleeds into this helper.
//
// A slide-flagged event arms the ramp from wherever the output currently sits;
// an unflagged event disarms it so the new note lands immediately. slideSeconds
// is the track's configured glide duration, evaluated at the call site.
template<typename CvQueue>
inline void drainCvQueue(uint32_t tick, CvQueue &q,
    float &cvOutputTarget, float cvOutput, Slide::Ramp &slideRamp,
    bool monitorOverrideActive, bool muted, bool cvAlways, float slideSeconds,
    MidiOutputEngine &midi, int trackIndex,
    TrackEngine::TickResult &result) {
    while (!q.empty() && tick >= q.front().tick) {
        if (!muted || cvAlways) {
            if (!monitorOverrideActive) {
                result |= TrackEngine::TickResult::CvUpdate;
                bool slide = q.front().slide;
                cvOutputTarget = q.front().cv;
                if (slide && slideSeconds > 0.f) {
                    slideRamp.start(cvOutput, slideSeconds);
                } else {
                    slideRamp.reset();
                }
                midi.sendCv(trackIndex, cvOutputTarget);
                midi.sendSlide(trackIndex, slide);
            }
        }
        q.pop();
    }
}

// ---------------------------------------------------------------------------
// Slide helper
// ---------------------------------------------------------------------------

// Advance the portamento ramp by dt. When the ramp is inactive this snaps the
// output to the target, so a non-sliding step lands immediately. CurveTrackEngine
// is excluded — it slews a continuous signal and stays on Slide::applySlide.
inline void applySlideUpdate(float &cvOutput, float cvOutputTarget,
    Slide::Ramp &slideRamp, Types::SlideType slideType, float dt) {
    cvOutput = slideRamp.eval(cvOutputTarget, dt, slideType);
}
