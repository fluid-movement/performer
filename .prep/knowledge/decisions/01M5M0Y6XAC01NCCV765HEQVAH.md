---
type: decision
title: Step grid cell states and beat markers
description: 'Gate on: Bright fill. Gate on + cursor: Bright fill with 1px inner ring knocked out. Gate off: 1px MediumLow outline. Gate off + cursor: inverted. Steps 0/4/8/12 get a 2px top border to mark beats without a new brightness level. The playing step is Bright regardless of gate state.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/*SequenceEditPage.*
  - src/apps/sequencer/ui/painters/SequencePainter.*
---


