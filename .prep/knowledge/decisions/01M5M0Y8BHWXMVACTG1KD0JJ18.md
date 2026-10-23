---
type: decision
title: Note step length is a ratio of the divisor
description: Step Length indexes a 20-entry table 1/8 1/6 1/4 1/3 1/2 2/3 3/4 1 5/4 4/3 3/2 2 5/2 3 4 5 6 8 12 16 (default index 7 = 1/1); stepLengthTicks = max(2, divisor*num/den). A step can be shorter or longer than a clock pulse. The field is 5 bits, so up to 32 entries.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/NoteSequence.*
---


