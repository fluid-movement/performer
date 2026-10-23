---
type: decision
title: Stochastic track V2 is SIG-inspired probability distributions
description: 'Three global distributions sampled every step: 7 scale-degree bars (NOTE), 5 octave bars -2..+2 (OCT), 6 duration bars 1/16..2 (LEN). Rest density is implicit: the total degree probability mass drives it, with no separate rest parameter. If all degree weights are 0 the engine falls back to equal weighting rather than going silent. Edit by holding a step and turning. Tabs NOTE / OCT / LEN / LOOP; SIG''s articulation (ART) was dropped because the module has no envelope output. No migration from the old per-step model (Version44).'
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/StochasticSequence.*
  - src/apps/sequencer/engine/StochasticEngine.*
  - src/apps/sequencer/ui/pages/StochasticSequenceEditPage.*
---


