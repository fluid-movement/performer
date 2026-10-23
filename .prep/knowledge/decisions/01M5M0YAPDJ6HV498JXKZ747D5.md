---
type: decision
title: 'Boot splash: PER|FORMER credit line plus FLUID MOVEMENT EDITION chrome wordmark'
description: PER|FORMER SEQUENCER credits what existed; FLUID MOVEMENT / EDITION names what was built on top. The wordmark is a rounded geometric display face (Daft Punk Discovery treatment) with a vertical chrome ramp and a travelling specular sweep that the 16 step LEDs mirror; the loading bar is a 2px hairline. Glyphs come from resources/generate-logo.py (capsules + elliptical arcs, 4x4 supersampling) into LogoPainter.cpp; only the 11 letters of FLUID MOVEMENT exist, it is a logo renderer, not a font. splash_test.py is the only harness that does not wait the splash out.
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/StartupPage.*
  - src/apps/sequencer/ui/painters/LogoPainter.*
  - resources/generate-logo.py
---


