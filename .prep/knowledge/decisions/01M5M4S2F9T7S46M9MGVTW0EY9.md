---
type: decision
title: Stochastic loop mutation uses its own RNG and a loop length decoupled from first/last step
description: Replayed slots are re-drawn with probability loopChance/15 using a separate mutationRng so the sequence RNG stream is undisturbed, and loopLength (Version45) is independent of firstStep/lastStep, which only shifts the loop's phase in the 16-step buffer. Tab controls are in agent-docs/features/stochastic-track.md.
date: "2026-10-23"
issue: 01M5M1Z71R13P7S9TY1SW2QM69
supersedes:
  - 01M5M0Y9NPD38A4YK0R8EAV2WW
---


