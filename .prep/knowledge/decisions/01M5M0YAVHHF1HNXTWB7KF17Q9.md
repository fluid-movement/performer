---
type: decision
title: Engine behaviour is verified with headless simulator scripts
description: Each track feature ships with a Python harness test under src/apps/sequencer/tests/ui/ (note_v2_test, curve_encoder_skew_test, arp_v2_test, quantizer_loop_test, stochastic_v2_test, config_nav_test, splash_test) built against the testsim target. Use rising-edge gate detection; wait(N) is N ms; at 120 BPM with divisor 12 one step is 125 ms.
date: "2026-10-23"
scope:
  - src/apps/sequencer/tests/**
  - src/apps/sequencer/python/**
---


