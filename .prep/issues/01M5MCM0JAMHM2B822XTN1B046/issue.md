---
title: 'Dead track settings: remove or wire them'
kind: decision
tags:
  - engine
  - arp
  - stochastic
  - curve
priority: low
---

Settings that exist in the model and UI but that no engine reads: the Arp and Stochastic probability/length bias settings, the Arp track's embedded Arpeggiator and midiKeyboard flag, CurveTrack.curveCvInput, and Stochastic rotate. Decide per setting whether to wire it into the engine or remove it (removal is a serialization change: skip the bytes on read, bump ProjectVersion, keep writerVersion() branches; routing targets degrade to Target::None by stable id). Related: 01M5M0ZPCRQK3RY8A12THXQVDB covers slideTime on Arp/Stochastic, which is dead for the same reason. agent-docs feature docs list these as unwired.

Source: documentation pass for 01M5M1Z71R13P7S9TY1SW2QM69 (finding 01M5M4SW25RBH4Z67M2QW16EX1).

## Open questions
