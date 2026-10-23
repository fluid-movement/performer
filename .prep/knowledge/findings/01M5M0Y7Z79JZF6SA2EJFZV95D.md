---
type: finding
title: 'Line-sequencer view: strengths and limits'
description: 'A horizontal line per active step encodes gate (present), pitch (vertical position) and gate length (width). It reads melody contour well but has under 2px per semitone for a two-octave range in 46px, small width differences are illegible, and beat structure disappears. Proposed fixes: use it on the NOTE tab only, add faint scale-degree rules, vary brightness by beat, and use a full-height scanline play cursor.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/NoteSequenceEditPage.*
---


