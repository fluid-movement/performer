---
type: decision
title: Logic track removed; old Logic tracks load as Note
description: Logic (gate-combining) was the least used, musically niche track type and was deleted 2026-05-14. TrackMode::Logic (serialized 4) remaps to Note on load. Every remaining track type is selectable on every track.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/Track.*
---


