---
type: decision
title: Config detail pane draws what the parameter does, reusing engine math
description: 'The right 76px of a config page shows the selected parameter: a drawing (slide ramp, pulse train, pulse width, swing grid, curve segment, voltage/bipolar bar, keyboard with scale degrees, note range) or the value large, which also wraps long enum names. Drawings live in ui/painters/PreviewPainter and must call the engine''s own Slide::shape and CurveSequence::evalSegment rather than reimplementing them, so a change to the sound changes the picture. List occupies the left 164px: labels at x=8, values at x=60.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/painters/PreviewPainter.*
  - src/apps/sequencer/ui/pages/ListPage.*
---


