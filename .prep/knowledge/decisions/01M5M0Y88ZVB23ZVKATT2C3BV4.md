---
type: decision
title: 'Note steps: 5 tabs, 10 layers'
description: GATE (Gate, Step Length, Gate Length), RETRIG (Retrigger, Arp Retrigger), REPEAT (Repeat), NOTE (Note, Slide), COND (Condition, Retrigger Condition). 46 of 64 step bits used; _data1 has 18 spare bits. gate, slide and note kept their pre-Version54 bit positions so the migration copies them straight.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/NoteSequence.*
  - src/apps/sequencer/ui/pages/NoteSequenceEditPage.*
---


