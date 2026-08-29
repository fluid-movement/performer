#pragma once

#include "model/Types.h"

#include "core/math/Math.h"

#include <cmath>

namespace Slide {

// ---------------------------------------------------------------------------
// Slew limiter (continuous signals)
// ---------------------------------------------------------------------------

// First-order exponential lag with a wall-clock time constant. This never
// actually reaches the target, which is exactly what you want for smoothing a
// continuously varying signal — CurveTrackEngine uses it that way. It is NOT
// suitable for note-to-note portamento; see Ramp below.
static float applySlide(float current, float target, int slideTime, float dt) {
    float tau = slideTime / 100.f;
    tau = tau * tau * 2.f;
    float coeff = tau > 0.f ? std::exp(-1.f * dt / tau) : 0.f;
    return target + coeff * (current - target);
}

// ---------------------------------------------------------------------------
// Portamento ramp (note to note)
// ---------------------------------------------------------------------------

// Normalized shaping curve over a bounded window. shape(0) == 0 and
// shape(1) == 1 for every type, so a ramp driven by it is guaranteed to arrive
// at the target — the defining property the old one-pole lacked.
//
// Exponential is the TB-303 / RC character: fast at the start, decelerating
// into the target, which is what produces the sense of settling. Because the
// window is bounded the glide is constant-time — the same duration whether the
// interval is a semitone or two octaves, matching the 303.
inline float shape(Types::SlideType type, float t) {
    constexpr float K = 4.f;
    const float N = 1.f - std::exp(-K);
    switch (type) {
    case Types::SlideType::Exponential:
        return (1.f - std::exp(-K * t)) / N;
    case Types::SlideType::Logarithmic:
        return 1.f - (1.f - std::exp(-K * (1.f - t))) / N;
    default:
        return t;
    }
}

struct Ramp {
    float startCv = 0.f;
    float elapsed = 0.f;
    float duration = 0.f;   // seconds; 0 means "not sliding"

    void start(float from, float seconds) {
        startCv = from;
        elapsed = 0.f;
        duration = seconds > 0.f ? seconds : 0.f;
    }

    void reset() {
        elapsed = 0.f;
        duration = 0.f;
    }

    bool active() const { return duration > 0.f; }

    // Advance by dt and return the current output. Once the window closes the
    // ramp deactivates, so the output stays pinned to the target afterwards.
    float eval(float target, float dt) { return eval(target, dt, Types::SlideType::Linear); }

    float eval(float target, float dt, Types::SlideType type) {
        if (duration <= 0.f) {
            return target;
        }
        elapsed += dt;
        float t = clamp(elapsed / duration, 0.f, 1.f);
        if (t >= 1.f) {
            duration = 0.f;
            return target;
        }
        return startCv + (target - startCv) * shape(type, t);
    }
};

} // namespace Slide
