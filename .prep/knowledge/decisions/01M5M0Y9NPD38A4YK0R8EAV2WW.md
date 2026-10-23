---
type: decision
title: 'Stochastic LOOP: Turing-machine style mutation with its own RNG'
description: Step buttons set loop length (1-16), the encoder sets loopChance (0 = locked, 15 = fully random); each replayed step has a per-step chance of being redrawn from the current distributions. Mutation uses a separate mutationRng so it does not disturb the sequence RNG stream. Loop length is stored as _loopLength (Version45), decoupled from firstStep/lastStep; the buffer is read as (firstStep + index) % 16, so firstStep shifts phase without changing length.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/StochasticSequence.*
  - src/apps/sequencer/engine/StochasticEngine.*
---


