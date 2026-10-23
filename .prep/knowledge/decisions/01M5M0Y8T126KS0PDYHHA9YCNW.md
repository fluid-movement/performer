---
type: decision
title: Note slide is a bounded, constant-time ramp (TB-303 style)
description: 'Portamento uses Slide::Ramp + Slide::shape, normalized so shape(0)=0 and shape(1)=1 for Linear, Exponential (default, RC/303) and Logarithmic, so arrival is guaranteed. slideTime is milliseconds (value*5, 5-500 ms, default 60 ms). Why: the old one-pole (tau = 2*(slideTime/100)^2 s) never converged, covering about 22% of the interval in a 1/16 step at 120 BPM; the TB-303 glide is constant-time (~55 ms), exponential and bounded. Slide::applySlide stays for CurveTrackEngine''s slew limiter, which genuinely wants a lag that never settles.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/engine/Slide.h
  - src/apps/sequencer/engine/NoteTrackEngine.*
  - src/apps/sequencer/engine/TrackEngineHelpers.h
---


