---
type: decision
title: Note track has no Aligned play mode and no CV update mode
description: 'NoteTrack::_playMode was deleted: advanceAligned() derives the step index in closed form from relativeTick/divisor, which variable step length cannot express (the same trade Curve made in Version53). The engine always advances freely. CvUpdateMode was deleted because Always contradicts ''a gate-off step changes nothing''.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/NoteTrack.*
  - src/apps/sequencer/engine/NoteTrackEngine.*
---


