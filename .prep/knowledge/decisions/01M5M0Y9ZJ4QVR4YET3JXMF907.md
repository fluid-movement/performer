---
type: decision
title: 'Quantizer V2: 5 tabs and a unified 13-position trigger source'
description: Tabs GATE / SRCE / TRIG / TUNE / LOOP. TRIG is one scroll FREE -> INT+T1..T8 -> EXT+CV1..CV4 backed by a single TriggerSource list row (editTriggerSource/printTriggerSource). CvGate mode (Version48) fires on a CV input rising edge. Old Internal mode loads as Free. Spurious-edge guards _lastCvChannel (CvGate) and _lastTriggerTrack (External) absorb a HIGH input on mode or channel switch. Scale/root are global; the quantizer has no per-track scale.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/QuantizerTrack.*
  - src/apps/sequencer/engine/QuantizerTrackEngine.*
  - src/apps/sequencer/ui/pages/QuantizerSequenceEditPage.*
  - src/apps/sequencer/ui/model/QuantizerTrackListModel.h
---


