---
title: 'Arp: implement CONV and DIV arp orders'
kind: code
tags:
  - engine
  - arp
priority: low
---

arpOrder 5 (CONV, converge) and 6 (DIV, diverge) fall back to UP in ArpTrackEngine::nextNoteCv(). Implement them: converge alternates outside-in (lowest, highest, 2nd lowest, 2nd highest...), diverge inside-out. Extend arp_v2_test.py.

Source: agent-docs/features/arp-track.md.

## Open questions
