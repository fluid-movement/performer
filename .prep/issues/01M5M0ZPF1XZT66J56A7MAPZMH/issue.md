---
title: 'Stochastic: Free play mode is unimplemented'
kind: decision
tags:
  - engine
  - stochastic
priority: low
---

StochasticEngine.cpp: case Types::PlayMode::Free is an empty break (and Last too). Default is Aligned, so not urgent. Either implement Free (advance a free relative tick per tick and trigger on division boundaries, mirroring the old note free path) or remove playMode from StochasticTrack the way Arp did in Version49 (skip the byte on read, bump version).

Source: CONTEXT.md Active TODOs.

## Open questions
