---
type: decision
title: UI design iterates in the Svelte sandbox before any C++
description: 'tools/ui-sandbox is a SvelteKit/Svelte 5 app whose Canvas.ts mirrors src/core/gfx/Canvas.h 1:1 (same colours, fonts, Set/Add/Sub blend), so mockups translate directly to C++. Variations are {label, draw(c), leds()} in VARIATIONS[pageId] in src/routes/+page.svelte; the trunk sits at index 0 labelled TRUNK. The user runs the dev server themselves; agents do not start it. Why: the firmware sim needs a full C++ build per change; the sandbox iterates in seconds.'
date: "2026-10-23"
scope:
  - tools/ui-sandbox/**
---


