---
type: decision
title: Breaking model changes bump ProjectVersion and migrate in read()
description: Each serialization change adds a ProjectVersion entry and version-gated reads (reader.read(field, VersionN), reader.skip<T>(0, VersionN) for removed mid-blob fields). write() keeps writerVersion() branches emitting zeroed placeholders for older formats, because the Python harness writes old versions to test the read migrations. Removed Routable<float> fields were written whole (8 bytes), not .base. Clean breaks without migration are acceptable before outside users (Curve V1, Stochastic V2, Arp V2 did that), but say so in the change.
date: "2026-10-23"
scope:
  - src/apps/sequencer/model/ProjectVersion.h
  - src/apps/sequencer/model/**
---


