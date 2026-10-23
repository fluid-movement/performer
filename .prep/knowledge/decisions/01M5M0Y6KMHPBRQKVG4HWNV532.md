---
type: decision
title: Brand screens are exempt from the three-brightness rule
description: 'The boot splash (StartupPage + ui/painters/LogoPainter) may use the full 16-level ramp: the chrome shading of the FLUID MOVEMENT wordmark is the design. It shows no operational state, is never read under time pressure and is on screen about two seconds per session. Every page the user works on stays on three levels. Decided 2026-08-29.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/StartupPage.*
  - src/apps/sequencer/ui/painters/LogoPainter.*
  - resources/generate-logo.py
---


