---
type: decision
title: 'Design principle: absence is information'
description: 'Draw nothing for off states: no dim placeholders, ghost borders or greyed cells. Everything drawn then means something. Mark only what varies: gate presence is boolean, so its mark is small and uniform; length, probability and pitch vary, so their marks vary. Negative space aids scanning; visual weight is not a proxy for importance.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/**
---


