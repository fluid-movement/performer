---
type: decision
title: 'Quantizer input stability: IIR + hysteresis + delayed sampling'
description: InputFilterAlpha 0.10 IIR, HysteresisVolts 0.012 (~14% of a semitone, Free mode), SampleDelayTicks 4 (~10 ms) before committing in clocked modes. Every scale step is >= 83 mV and FilterSnapVolts is 50 mV, so real pitch changes bypass the IIR; it only damps sub-note jitter. Alpha 0.10 beat 0.25 for mid-frequency interference (stable to +-30 mV p-p at 33 Hz vs +-10 mV). Noise of +-30 mV or more near a boundary cannot be guaranteed stable; recommend hardware filtering.
date: "2026-10-23"
scope:
  - src/apps/sequencer/engine/QuantizerTrackEngine.*
---


