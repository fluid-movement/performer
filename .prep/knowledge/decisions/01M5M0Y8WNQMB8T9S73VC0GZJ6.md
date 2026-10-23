---
type: decision
title: 'Gate Mode: Retrigger by default, Legato holds 100% gates across the boundary'
description: At 100% gate length, Legato holds the gate across the step boundary and suppresses the next slot's gate-on; Retrigger (default) closes it one tick early so it can re-fire.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/NoteTrack.*
  - src/apps/sequencer/engine/NoteTrackEngine.*
---


