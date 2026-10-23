---
title: Find the best documentation structure for agents, then update agent-docs
kind: code
tags:
  - docs
priority: high
---

## Requirement

Thoroughly analyze what documentation structure serves coding agents best in this project, then restructure and update `agent-docs/` to match. The project is large (about 570 C++ files across five layers plus a Python sim harness and a Svelte UI sandbox), so agents need feature and code documentation to find their way. That documentation lives in `agent-docs/`. prep holds only work (issues) and decisions/findings (knowledge). It is not a place to keep feature or code documentation.

### Analysis (do first, record the outcome as a prep finding)

- How agents actually use the docs: entry point (CLAUDE.md → `agent-docs/index.md`), what they read before searching code, what they need per task type (engine change, UI page, serialization, tests).
- Accuracy: check every doc against the current code. Known stale or wrong content:
  - `features/stochastic-track.md` describes the pre-V2 per-step model.
  - `features/logic-track.md` covers a track type that was deleted.
  - `features/serialization.md` describes a tag-based format; the code uses version-gated reads (`reader.read(field, VersionN)`, `skip<T>`).
  - `architecture.md` and `index.md` still list LogicTrack and `Arpeggiator.h` in places.
  - `ui-state.md` and `ui-redesign/05-current-ui-functions.md` describe the old UI and overlap each other.
  - `ui-redesign/06-track-type-inventory.md` is superseded for every track type except MidiCv.
  - `curve-track.md`, `01-design-language.md` and `02-sandbox-status.md` reference `agent-docs/ui-redesign/sandbox`, which moved to `tools/ui-sandbox`.
- Structure: per-feature docs versus fewer, larger ones; what belongs in index/architecture; whether `features-log.md` (history) earns its place next to git and CHANGELOG/RELEASE-NOTES; where the UI redesign docs belong now that design work is mostly shipped.
- Rot prevention: how docs stay current (for example a Definition of Done item, a Stop-hook reminder, or "Change Guide" sections kept small and verified).
- Overlap with prep: the 57 knowledge records created on 2026-10-23 were partly written as feature documentation (for example "Note steps: 5 tabs, 10 layers", "Curve track is a segment assembler", "Quantizer V2: 5 tabs..."). Audit them. Keep the ones that are decisions or findings. Move descriptive content into the docs and supersede or drop the rest, using prep commands only. The docs should link to decisions rather than restate their reasons.

### Update

Restructure and rewrite `agent-docs/` according to the analysis, fix stale content and moved paths, and update CLAUDE.md's instructions to match.

### Settled with the user (2026-10-23)

- CONTEXT.md, PLAN.md, TODO.md and bugs.md stay retired. Do not restore them; prep issues and knowledge replace them.
- Where the UI redesign docs (00-ia, 01-design-language, 02-sandbox-status, 08-design-ideas) live is for the analysis to decide; record the choice with prep decide.
- Decision "Feature and code documentation lives in agent-docs, not in prep knowledge" applies.
- The uncommitted working-tree edits to `agent-docs/features-log.md` and `agent-docs/ui-redesign/01-design-language.md` (boot splash entry and brand-screen exemption) belong to the user's in-progress splash work, which is not committed. Treat that content as part of the docs.

## Open questions
