---
title: 'MidiCv: move legato portamento to Slide::Ramp'
kind: code
tags:
  - engine
  - midicv
  - slide
---

MidiCvTrackEngine.cpp:50 still uses the unbounded one-pole Slide::applySlide for legato-detected portamento, which never reaches the target note (the problem Note V2 fixed). Consider moving it to the bounded Slide::Ramp + Slide::shape like NoteTrackEngine, so the glide arrives. See decision 'Note slide is a bounded, constant-time ramp'.

Source: CONTEXT.md Active TODOs.

## Open questions
