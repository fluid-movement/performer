---
type: decision
title: Encoder press is never a mode toggle; grid pages edit by hold-button + turn
description: Encoder press may only be a one-way descent into a visible sub-page (name entry, picker confirm). On grid pages (Track Edit, Dashboard) a value changes by holding the target button and turning the encoder; idle rotate there is a no-op unless the page has one unambiguous most-important value. Hold+turn is not used on config pages, where walking a list beats hunting for a button in the 3x8 field.
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/**
---


