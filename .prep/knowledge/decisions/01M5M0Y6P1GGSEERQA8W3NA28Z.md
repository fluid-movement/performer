---
type: decision
title: Inverted fill is the one selection convention, everywhere
description: Selection/cursor = fillRect(Bright) with content drawn knocked out. Draw the knocked-out text with setBlendMode(BlendMode::Sub) after the bright fill; drawing with Color::None paints solid black rectangles instead of glyph cutouts. Must look identical on every page so the user learns 'bright fill = my cursor' once.
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/**
---


