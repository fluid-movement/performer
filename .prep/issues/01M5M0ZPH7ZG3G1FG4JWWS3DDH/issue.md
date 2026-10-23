---
title: 'Arp: decide what to do about the euclidean rotation change for Version46 projects'
kind: decision
tags:
  - arp
  - serialization
priority: low
---

The canonical-rotation change (session 2025-06-04: first hit at step 0, user rotation applied on top) changed the meaning of non-zero rhythmR/modR. A Version46 project with non-zero rotation now sounds different. Options: add a migration that compensates for the old firstHit offset (needs a version bump; Version47+ are already used by other changes, so it would be a new version gated on the arp read path), or accept the break and document it. Before outside users, accepting is consistent with earlier clean breaks.

Source: CONTEXT.md Active TODOs.

## Open questions
