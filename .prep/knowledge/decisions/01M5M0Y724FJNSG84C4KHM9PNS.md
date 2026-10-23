---
type: decision
title: LEDs are a first-class design surface with fixed colour semantics
description: 'Every page design specifies LED state alongside the screen. Green = active/playing/on; red = muted/stopped/error/destructive pending; amber (red+green) = queued, changes at next sync point; off = nothing here; blinking green = recording; blinking red = waiting for external sync. Defaults: track LEDs green = selected, red = muted; Play green playing / blinking green recording; Page lights during Page-hold.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/**
  - src/apps/sequencer/ui/LedPainter.*
---


