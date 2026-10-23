---
type: decision
title: 'Typography: Font::Tiny for everything, the large font only for one dominant value'
description: Font::Tiny (6px per char) is the default for labels, values, step data and footers. Font::Small (8px per char; called 'Font.Normal' in older design notes and the sandbox) is only for a single dominant value such as BPM in the Tempo modal, never for labels or lists. Labels are ALL CAPS, no trailing colons. F-key tab labels and column headers max 4 chars (GATE, NOTE, COND, SYNC, FILL, DIV, TRIG, LEN, OCT); config row labels max 8 chars. Prefer existing DX7/Elektron abbreviations over invented ones.
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/**
---


