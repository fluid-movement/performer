---
type: finding
title: Stochastic bar ghosting on hardware is SSD1322 column coupling
description: 'Probability bars bleed into each other on the real OLED but not in the simulator: SSD1322 column capacitive coupling, not a rendering bug. Bars were thinned to 18px (half the 36px column) and the full column is cleared each frame to stop header-line bleed into gaps. Wide bright adjacent columns are risky on this display.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/StochasticSequenceEditPage.*
---


