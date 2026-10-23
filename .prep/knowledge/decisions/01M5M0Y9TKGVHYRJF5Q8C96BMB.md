---
type: decision
title: 'Arp track V2: degree-mask note pool + two euclidean generators'
description: 7-bit degreeMask selects active scale degrees (no per-step notes, no MIDI input). A rhythm euclidean (N/K/R + gate length) drives gates; a mod euclidean (N/K/R + mode Off/Accent/Mask/Combine/Ratchet/Hold) acts on rhythm hits. Orders UP/DOWN/UP-DN/DN-UP/RAND, with bounce modes using period 2*(n-1) so endpoints do not repeat. Tabs NOTE / RHYTHM / MOD / ARP. Version46, no migration from the old per-step model. Arp has no play mode (removed in Version49; only Aligned ever worked).
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/ArpSequence.*
  - src/apps/sequencer/model/ArpTrack.*
  - src/apps/sequencer/engine/ArpTrackEngine.*
  - src/apps/sequencer/ui/pages/ArpSequenceEditPage.*
---


