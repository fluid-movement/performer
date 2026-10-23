---
type: decision
title: Curve parameters edit in an integer 0-100 percent domain
description: 'All five curve params edit via shapePercent()/skewPercent()/levelPercent()/offsetPercent(): 1 unit per detent, SHIFT = x10 coarse (the firmware-wide coarse convention). The percent round trip is exact, which keeps stepping symmetric. Why: the old 0.05f float step plus re-quantization stepped +3/-2 raw. The same encoder logic is duplicated in OverviewPage and must be changed in both.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/CurveSequence.h
  - src/apps/sequencer/ui/pages/CurveSequenceEditPage.*
  - src/apps/sequencer/ui/pages/OverviewPage.*
---


