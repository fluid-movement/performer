---
type: finding
title: SortedQueue usable capacity is Capacity-1
description: SortedQueue::size() is (_write - _read) % Capacity, so a full queue reads as empty. The old note engine pushed 16 events for RETRIG 7 + LENGTH 15 and the step went silent. Never push a whole step's events at once into a 16-slot queue; the note slot scheduler makes overflow structurally impossible.
date: "2026-10-23"
scope:
  - src/apps/sequencer/engine/SortedQueue.h
  - src/apps/sequencer/engine/*TrackEngine.*
  - src/apps/sequencer/engine/StochasticEngine.*
---


