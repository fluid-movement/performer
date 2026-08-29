#pragma once

#include "model/Types.h"

#include "core/gfx/Canvas.h"

// Drawings of what a config parameter *does*, shown in the right pane of the
// config pages. The point is that turning the encoder changes a picture, not
// just a number -- a slide type is a curve, a divisor is a rhythm, swing is a
// grid that leans.
//
// Every function draws inside (x, y, w, h) and assumes the caller has already
// set the blend mode. Nothing here reads the model; callers pass plain values
// so the same drawing serves several parameters.
class PreviewPainter {
public:
    // Portamento shape over a bounded window. Uses the same Slide::shape() the
    // engine ramps with, so the drawing cannot drift from what you hear.
    static void drawSlideRamp(Canvas &canvas, int x, int y, int w, int h, Types::SlideType type, int slideTimeMs);

    // Evenly spaced pulses. `pulses` is how many fit in the window.
    static void drawPulseTrain(Canvas &canvas, int x, int y, int w, int h, int pulses);

    // A single pulse of the given width, as a fraction of the window.
    static void drawPulseWidth(Canvas &canvas, int x, int y, int w, int h, float dutyFraction);

    // Four beats with every second one pushed late by `swing` (50 = straight).
    static void drawSwingGrid(Canvas &canvas, int x, int y, int w, int h, int swing);

    // Numerator beats grouped against a bar line.
    static void drawBeatGrid(Canvas &canvas, int x, int y, int w, int h, int beats, int division);

    // One curve segment, using the same evalSegment() the engine renders.
    static void drawCurveShape(Canvas &canvas, int x, int y, int w, int h, float shape, float skew, float expCurve);

    // Unipolar bar with the voltage ceiling marked.
    static void drawVoltageBar(Canvas &canvas, int x, int y, int w, int h, float volts, float maxVolts);

    // Bipolar bar centred on zero.
    static void drawBipolarBar(Canvas &canvas, int x, int y, int w, int h, float value, float range);

    // One octave of keys. `litSemitone` highlights a single key (-1 for none);
    // `scaleIndex` lights every semitone belonging to that scale (-1 for none).
    static void drawKeyboard(Canvas &canvas, int x, int y, int w, int h, int rootSemitone, int litSemitone, int scaleIndex);

    // Keyboard span between two MIDI notes.
    static void drawNoteRange(Canvas &canvas, int x, int y, int w, int h, int lowNote, int highNote);
};
