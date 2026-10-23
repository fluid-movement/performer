---
type: decision
title: Step notes are scale degrees; scale and root are project-global
description: 'step.note() stores a scale-degree index, never a semitone. Scale and root note live on the project only (per-sequence scale removed in Version41). Changing the scale never remaps step notes: the same degree pattern plays in the new harmonic context, which makes sweeping the scale a sound-design move. Never use Scale::isNotePresent on a degree (it expects a chromatic semitone), and never assume Scale::get(0) is chromatic.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/Scale.*
  - src/apps/sequencer/model/Project.h
  - src/apps/sequencer/model/*Sequence.*
  - src/apps/sequencer/engine/**
---


