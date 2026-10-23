---
type: decision
title: Euclidean patterns use canonical form, shared by engine and display
description: 'computeEuclidean puts the first hit at step 0 and applies the user''s rotation on top. The engine and ArpSequenceEditPage both call the single implementation in engine/EuclideanGenerator.h (the test arp_v2_test.py mirrors it). Why: the display used to auto-rotate independently, so MASK/COMBINE overlap cells disagreed with the real gate output.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/engine/EuclideanGenerator.h
  - src/apps/sequencer/engine/ArpTrackEngine.*
  - src/apps/sequencer/ui/pages/ArpSequenceEditPage.*
---


