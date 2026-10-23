---
type: finding
title: Routing targets degrade gracefully when removed
description: Routes serialize targets by a stable id (targetSerialize) and readEnum falls back to Target::None for an unknown id, so deleting a routing target needs no migration.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/Routing.*
---


