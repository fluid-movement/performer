---
type: decision
title: 'UI brightness: three levels only (Bright 15, Low/MediumLow 5, off)'
description: 'Every operational page uses only Color::Bright (focused/active), Color::MediumLow (context, labels, structure) and Color::None (absent). Color::Low (3), Medium (7) and MediumBright (10) are not in the palette; no gradients, no dim fills for inactive things. Inverted cells (Bright fill, None content) are not a fourth level. Why: three-level discipline keeps the 256x64 OLED calm and every mark unambiguous (Norns-derived). To change a rule, supersede this record first rather than silently breaking it.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/**
---


