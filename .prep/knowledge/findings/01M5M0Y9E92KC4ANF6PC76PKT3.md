---
type: finding
title: SequenceState::direction() must be set by every run mode
description: Until Version53 direction() had no callers and only PingPong, Pendulum and RandomWalk maintained it, so Backward reported +1 and a stale -1 survived run-mode changes. Both calculateNextStepFree and calculateNextStepAligned now set it in every mode; keep it that way when adding run modes.
date: "2026-10-23"
scope:
  - src/apps/sequencer/engine/SequenceState.*
---


