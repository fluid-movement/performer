---
title: 'Stochastic: hardware bar ghosting on the SSD1322'
kind: research
tags:
  - ui
  - stochastic
  - hardware
priority: low
---

On hardware the NOTE/OCT/LEN probability bars bleed into each other (SSD1322 column capacitive coupling); the simulator is clean. Bars are already thinned to 18px. Investigate further mitigation (narrower bars, MediumLow fill, gaps, different column layout) on the real device, or accept and close. See finding 'Stochastic bar ghosting on hardware is SSD1322 column coupling'.

Source: CONTEXT.md Active TODOs.

## Open questions
