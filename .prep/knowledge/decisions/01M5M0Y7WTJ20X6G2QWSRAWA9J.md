---
type: decision
title: 'Design principle: tabs change the editing target, not the display'
description: 'Switching F-key tabs keeps the base view stable; the tab changes which footer cell is highlighted, what hold+turn edits, and optionally a small additive overlay (scale-degree grid on NOTE, repeat counts on REPEAT). Layered reveal: base gestural layer always, tab overlay, then exact value while a step is held. Why: a full display swap per tab forces the user to re-parse the screen and discards their mental model.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/*SequenceEditPage.*
---


