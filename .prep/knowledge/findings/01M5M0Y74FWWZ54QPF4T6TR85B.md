---
type: finding
title: The five function keys have no LEDs
description: The LED matrix is CONFIG_BLM_ROWS * CONFIG_BLM_COLS_LED = 32 entries covering 8 track, 16 step and 8 global buttons; Key::F0 is already index 32, so writing an F-key LED is an out-of-bounds write. F-key state must be shown on screen (inverted footer tab). Found 2026-08-29.
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/**
---


