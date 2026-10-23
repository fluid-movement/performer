---
title: 'docs-check: accept extension-less source paths'
kind: code
tags:
  - docs
  - tooling
priority: low
---

scripts/check-agent-docs.py (run by just docs-check) reports a backticked path without an extension, such as `ui/painters/LogoPainter`, as dead even when ui/painters/LogoPainter.h or .cpp exists. Docs commonly name a class by its stem. Accept a backticked path whose stem resolves to an existing file with a source extension (.h, .cpp, .py, .ts, .svelte); keep reporting stems that resolve to nothing. Agreed with the user 2026-10-23.

## Open questions
