---
title: 'Bug: switching a track to Arp resets or alters the sequence'
kind: code
tags:
  - bug
  - arp
---

Reported: switching a track to the Arp track mode resets the sequence to step 1, or does something with the steps. Needs reproduction in the simulator first (python harness), then a fix. Note that changing TrackMode reinitializes the track's data by design; check whether the issue is the expected reset or a playhead/state bug in ArpTrackEngine on mode switch.

Source: bugs.md.

## Open questions
