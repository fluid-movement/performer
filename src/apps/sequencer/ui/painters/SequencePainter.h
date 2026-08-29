#pragma once

#include "core/gfx/Canvas.h"
#include "model/NoteSequence.h"
#include "model/StochasticSequence.h"
#include "model/ArpSequence.h"



class SequencePainter {
public:
    static void drawLoopStart(Canvas &canvas, int x, int y, int w);
    static void drawLoopEnd(Canvas &canvas, int x, int y, int w);
    static void drawRetrigger(Canvas &canvas, int x, int y, int w, int h, int retrigger, int maxRetrigger);
    static void drawLength(Canvas &canvas, int x, int y, int w, int h, int length, int maxLength);
    static void drawSlide(Canvas &canvas, int x, int y, int w, int h, bool active);

    static void drawSequenceProgress(Canvas &canvas, int x, int y, int w, int h, float progress);
};
