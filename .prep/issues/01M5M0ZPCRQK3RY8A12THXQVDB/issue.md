---
title: 'Arp/Stochastic slideTime is dead: wire it or remove it'
kind: decision
tags:
  - engine
  - arp
  - stochastic
  - slide
---

Both engines push every CV event with slide = false (ArpTrackEngine.cpp:193, StochasticEngine.cpp:322 and :397), so the track-level slideTime setting does nothing. Decide whether to add a slide flag (e.g. a per-note slide probability or always-slide option) or remove the setting from both tracks (serialization change, version bump).

Source: CONTEXT.md Active TODOs.

## Open questions
