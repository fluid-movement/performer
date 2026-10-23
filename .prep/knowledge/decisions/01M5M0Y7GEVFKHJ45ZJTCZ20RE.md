---
type: decision
title: 'Information architecture: three tiers, about 20 pages'
description: 'Jam tier: Dashboard (home, Performer key; 8 track rows; read-only for now; no BPM) and Perform (Pattern key; replaces PatternPage+PerformerPage; patterns, mutes, fills, hold Step A+B quick chain; footer LATCH/SYNC/SNAP/FILL). Track Edit is 5 type-specific pages (Note, Curve, Stochastic, Arp, Quantizer) plus MidiCv, sharing footer-as-tabs, step selection, quick-edit overlay (Page+Step8-12: FirstStep, LastStep, RunMode, Divisor, ResetMeasure, with a Page-hold legend) and undo (Page+Step6). Set-and-forget tier: Track Config, Settings (Project, Clock, Routing, MIDI, Scales, Layout, System), Song. BPM lives only in the Tempo modal. Page+X shortcuts stay as expert navigation. Decided 2026-05-21.'
date: "2026-10-23"
scope:
  - src/apps/sequencer/ui/pages/**
---


