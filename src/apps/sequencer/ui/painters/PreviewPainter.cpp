#include "PreviewPainter.h"

#include "engine/Slide.h"
#include "model/CurveSequence.h"
#include "model/Scale.h"

#include "core/math/Math.h"

#include <algorithm>
#include <cmath>

void PreviewPainter::drawSlideRamp(Canvas &canvas, int x, int y, int w, int h, Types::SlideType type, int slideTimeMs) {
    // Baseline and target, so the shape is read against where it starts and ends.
    canvas.setColor(Color::MediumLow);
    canvas.hline(x, y + h - 1, w);
    canvas.hline(x, y, w);

    // A longer slide takes more of the window; the rest is the note held at
    // pitch. This is what makes slide time legible rather than just a number.
    float span = clamp(slideTimeMs / 500.f, 0.05f, 1.f);
    int rampW = std::max(2, int(w * span));

    canvas.setColor(Color::Bright);
    int prevY = y + h - 1;
    for (int i = 0; i <= rampW; ++i) {
        float t = rampW > 0 ? float(i) / rampW : 1.f;
        float v = Slide::shape(type, t);
        int py = y + h - 1 - int(v * (h - 1));
        if (i > 0) {
            canvas.vline(x + i, std::min(prevY, py), std::abs(py - prevY) + 1);
        }
        prevY = py;
    }
    if (rampW < w) {
        canvas.hline(x + rampW, y, w - rampW);
    }
}

void PreviewPainter::drawPulseTrain(Canvas &canvas, int x, int y, int w, int h, int pulses) {
    canvas.setColor(Color::MediumLow);
    canvas.hline(x, y + h - 1, w);

    pulses = clamp(pulses, 1, 16);
    canvas.setColor(Color::Bright);
    for (int i = 0; i < pulses; ++i) {
        int px = x + (w * i) / pulses;
        int pw = std::max(1, (w / pulses) / 2);
        canvas.fillRect(px, y + 2, pw, h - 3);
    }
}

void PreviewPainter::drawPulseWidth(Canvas &canvas, int x, int y, int w, int h, float dutyFraction) {
    canvas.setColor(Color::MediumLow);
    canvas.hline(x, y + h - 1, w);

    int pw = clamp(int(w * dutyFraction), 1, w);
    canvas.setColor(Color::Bright);
    canvas.fillRect(x, y + 2, pw, h - 3);
}

void PreviewPainter::drawSwingGrid(Canvas &canvas, int x, int y, int w, int h, int swing) {
    canvas.setColor(Color::MediumLow);
    canvas.hline(x, y + h - 1, w);

    // swing is a percentage where 50 is straight; push the off-beats late by
    // the excess so the grid visibly leans.
    float bias = clamp((swing - 50) / 100.f, 0.f, 0.5f);
    const int pairs = 4;
    canvas.setColor(Color::Bright);
    for (int i = 0; i < pairs * 2; ++i) {
        float slot = float(i) / (pairs * 2);
        if (i & 1) {
            slot += bias / pairs;
        }
        int px = x + int(slot * w);
        canvas.fillRect(px, y + 2, 2, h - 3);
    }
}

void PreviewPainter::drawBeatGrid(Canvas &canvas, int x, int y, int w, int h, int beats, int division) {
    canvas.setColor(Color::MediumLow);
    canvas.hline(x, y + h - 1, w);

    beats = clamp(beats, 1, 16);
    for (int i = 0; i < beats; ++i) {
        int px = x + (w * i) / beats;
        // The downbeat is taller, which is the whole point of a time signature.
        canvas.setColor(i == 0 ? Color::Bright : Color::MediumLow);
        int bh = i == 0 ? h - 2 : (h - 2) / 2;
        canvas.fillRect(px, y + h - 1 - bh, 2, bh);
    }
    (void)division;
}

void PreviewPainter::drawCurveShape(Canvas &canvas, int x, int y, int w, int h, float shape, float skew, float expCurve) {
    canvas.setColor(Color::MediumLow);
    canvas.hline(x, y + h - 1, w);

    canvas.setColor(Color::Bright);
    int prevY = y + h - 1;
    for (int i = 0; i < w; ++i) {
        float phase = w > 1 ? float(i) / (w - 1) : 0.f;
        float v = CurveSequence::evalSegment(phase, shape, skew, expCurve);
        int py = y + h - 1 - int(clamp(v, 0.f, 1.f) * (h - 1));
        if (i > 0) {
            canvas.vline(x + i, std::min(prevY, py), std::abs(py - prevY) + 1);
        }
        prevY = py;
    }
}

void PreviewPainter::drawVoltageBar(Canvas &canvas, int x, int y, int w, int h, float volts, float maxVolts) {
    int barY = y + h / 2 - 3;
    canvas.setColor(Color::MediumLow);
    canvas.drawRect(x, barY, w, 7);

    float f = maxVolts > 0.f ? clamp(volts / maxVolts, 0.f, 1.f) : 0.f;
    canvas.setColor(Color::Bright);
    canvas.fillRect(x + 1, barY + 1, std::max(1, int((w - 2) * f)), 5);
}

void PreviewPainter::drawBipolarBar(Canvas &canvas, int x, int y, int w, int h, float value, float range) {
    int barY = y + h / 2 - 3;
    int cx = x + w / 2;
    canvas.setColor(Color::MediumLow);
    canvas.drawRect(x, barY, w, 7);
    canvas.vline(cx, barY, 7);

    float f = range > 0.f ? clamp(value / range, -1.f, 1.f) : 0.f;
    int len = int((w / 2 - 1) * std::fabs(f));
    if (len > 0) {
        canvas.setColor(Color::Bright);
        canvas.fillRect(f >= 0.f ? cx : cx - len, barY + 1, len, 5);
    }
}

void PreviewPainter::drawKeyboard(Canvas &canvas, int x, int y, int w, int h, int rootSemitone, int litSemitone, int scaleIndex) {
    // Seven white keys spanning the width, with the five black keys sitting on
    // the boundaries between them.
    static const int whiteSemitone[7] = { 0, 2, 4, 5, 7, 9, 11 };
    static const bool blackAfter[7]   = { true, true, false, true, true, true, false };

    int kw = w / 7;
    int kh = std::min(h - 2, 18);
    int ky = y + (h - kh) / 2;

    auto isLit = [&] (int semitone) {
        if (litSemitone >= 0 && ((semitone - rootSemitone + 120) % 12) == ((litSemitone + 120) % 12)) {
            return true;
        }
        if (scaleIndex >= 0) {
            const Scale &scale = Scale::get(scaleIndex);
            return scale.isNotePresent((semitone - rootSemitone + 120) % 12);
        }
        return false;
    };

    for (int i = 0; i < 7; ++i) {
        int kx = x + i * kw;
        canvas.setColor(Color::MediumLow);
        canvas.drawRect(kx, ky, kw, kh);
        if (isLit(whiteSemitone[i])) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(kx + 1, ky + 1, kw - 2, kh - 2);
        }
    }

    for (int i = 0; i < 7; ++i) {
        if (!blackAfter[i]) {
            continue;
        }
        int bx = x + i * kw + kw - kw / 3;
        int bh = kh / 2;
        canvas.setColor(isLit(whiteSemitone[i] + 1) ? Color::Bright : Color::None);
        canvas.fillRect(bx, ky, (kw * 2) / 3, bh);
        canvas.setColor(Color::MediumLow);
        canvas.drawRect(bx, ky, (kw * 2) / 3, bh);
    }
}

void PreviewPainter::drawNoteRange(Canvas &canvas, int x, int y, int w, int h, int lowNote, int highNote) {
    canvas.setColor(Color::MediumLow);
    canvas.drawRect(x, y + h / 2 - 4, w, 9);

    int lo = clamp(lowNote, 0, 127);
    int hi = clamp(highNote, 0, 127);
    if (hi < lo) {
        std::swap(lo, hi);
    }
    int x0 = x + 1 + ((w - 2) * lo) / 127;
    int x1 = x + 1 + ((w - 2) * hi) / 127;
    canvas.setColor(Color::Bright);
    canvas.fillRect(x0, y + h / 2 - 3, std::max(1, x1 - x0), 7);
}
