---
type: decision
title: Curve track is a segment assembler
description: Steps are segments that chain and loop; loop length is the sum of segment lengths. Tabs SHPE / SKEW / LEN / LVL / OFST. Each segment rises from offset to offset+level and back, shaped by evalSegment(phase, shape, skew, expCurve); a short gate fires at each segment start. Step buttons select segments (up to 16). Legacy bitfield names Min/Max/ShapeVariation now mean offset/level/skew.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/CurveSequence.*
  - src/apps/sequencer/engine/CurveTrackEngine.*
  - src/apps/sequencer/ui/pages/CurveSequenceEditPage.*
---


