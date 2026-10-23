---
type: decision
title: Repeat extends a step, Retrigger subdivides it
description: Repeat (1-8) replays the whole step body including retriggers and arp offsets, so the step lasts stepTicks*repeat (Metropolix stage repeats). Retrigger (1-8) slices that body. Arp Retrigger (-15..+15) adds scale degrees per slice and resets each repeat. Gate Length is a percentage of the retrigger slice, not the step, because measured against the step retriggered gates would overlap.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/NoteSequence.*
  - src/apps/sequencer/engine/NoteTrackEngine.*
---


