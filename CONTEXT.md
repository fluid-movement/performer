# Project Context

## Current Focus

UI redesign — building a Svelte 5 web sandbox that emulates the firmware's 256×64 mono OLED
canvas API for fast design iteration. All UI work happens in the sandbox before any C++ changes.
We are in the G7 phase: per-track-type design sessions for each of the 6 track types.

## Active TODOs

- [ ] G7 — Per-track-type design sessions: Note → Curve → Stochastic → Arp → Quantizer → MidiCv
- [ ] Iterate — Run tastemaker iterations with consolidate cadence against each trunk; review after each

## Backlog

- [ ] Port finalized page designs from sandbox to C++ painters (`src/apps/sequencer/ui/painters/`)
- [ ] LED state design: specify LED state for each finalized page variation
- [ ] Song page design (deferred until other pages settle)
- [ ] Dashboard interaction model: read-only vs mute-toggle (revisit after Perform + Track Edit settle)

## Key Pointers

| What | Where |
|---|---|
| **UI redesign status** (read first for UI work) | `agent-docs/ui-redesign/02-sandbox-status.md` |
| Sandbox page (active design file) | `agent-docs/ui-redesign/sandbox/src/routes/+page.svelte` |
| IA decisions | `agent-docs/ui-redesign/00-ia.md` |
| Design language rubric | `agent-docs/ui-redesign/01-design-language.md` |
| Iteration feedback ledger | `agent-docs/ui-redesign/07-iteration-feedback.md` |
| Track-type parameter inventory | `agent-docs/ui-redesign/06-track-type-inventory.md` |
| Feature log | `agent-docs/features-log.md` |
| Agent docs index | `agent-docs/index.md` |
