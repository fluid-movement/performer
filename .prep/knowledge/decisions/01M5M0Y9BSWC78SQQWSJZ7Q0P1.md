---
type: decision
title: Curve plays segments mirrored when travelling backwards
description: 'phase = direction() < 0 ? 1 - fraction : fraction. Backward mirrors everything including the wrap; Pendulum/PingPong mirror the descending leg; RandomWalk mirrors when it steps back; Random never mirrors. The play scanline uses the mirrored phase; the drawn curve is never mirrored; the gate still fires at the segment start. A rule based on step deltas was rejected: it breaks on Backward''s wrap and Pendulum''s repeated endpoint. Version53.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/engine/CurveTrackEngine.*
  - src/apps/sequencer/engine/SequenceState.*
---


