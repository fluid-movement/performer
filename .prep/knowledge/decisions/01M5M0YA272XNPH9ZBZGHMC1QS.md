---
type: decision
title: Quantizer always-on clock and loop rules
description: 'The sequence clock sets _samplePending every tick in all modes and gate+CV commit atomically in commit() SampleDelayTicks after the boundary. Free mode gets no clock-driven sample outside Loop: hysteresis is its only commit path (prevents Rec double-fill). In Loop, External/CvGate do not set samplePending; the loop advances from the clock only, preventing double-advance of _loopIndex. The loop buffer stores un-transposed note indices; octave/transpose apply live at playback.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/engine/QuantizerTrackEngine.*
---


