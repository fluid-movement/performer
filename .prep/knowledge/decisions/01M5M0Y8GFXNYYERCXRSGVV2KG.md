---
type: decision
title: Only the gate condition writes the Pre/NotPre chain
description: Retrigger Condition uses the same Types::Condition set and, when it fails, collapses the step to retrigger = 1, but it only reads the Pre/NotPre chain. Two writers would make Pre meaningless.
date: "2026-10-23"
scope:
  - src/apps/sequencer/engine/NoteTrackEngine.*
---


