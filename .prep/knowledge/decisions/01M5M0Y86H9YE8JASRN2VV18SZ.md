---
type: decision
title: Note track is deterministic; randomness belongs to the Stochastic track
description: Note V2 (Version54) deleted gate probability, gate offset, retrigger probability, length/note variation range and probability, Bypass Scale and Stage Repeat Mode from steps, and play mode, CV update mode and the four bias parameters from the track. Do not reintroduce per-step randomness on the Note track. The detail popup is gone; values are read from the per-step strip.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/NoteSequence.*
  - src/apps/sequencer/model/NoteTrack.*
  - src/apps/sequencer/engine/NoteTrackEngine.*
---


