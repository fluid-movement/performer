---
type: decision
title: Curve output is unipolar 0V..Range, Range 1V-5V on the track
description: Range lives on CurveTrack (Version52), restricted to Unipolar1V..Unipolar5V; bipolar values map to the unipolar entry of the same voltage. Bipolar output is reached through the track Offset (Range 0..5V with Offset -2.50V gives -2.5..+2.5V). editRange clamps as int before converting because VoltageRange is uint8_t backed and a negative would wrap to 5V.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/CurveTrack.*
  - src/apps/sequencer/engine/CurveTrackEngine.*
---


