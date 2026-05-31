# Project Context

## Current Focus

Curve track V1 (segment assembler) C++ implementation — in progress.
Decision: V1 converged on as the Curve track design. TRUNK dropped entirely. V1 is being
ported from the Svelte sandbox to C++ before continuing the broader UI redesign.
Full implementation spec in `PLAN.md` (project root); task checklist in `TODO.md`.

After Curve V1 ships, resume UI redesign: Stochastic → Arp → Quantizer → MidiCv.

## Active TODOs

- [ ] Curve V1 — implement C++ model, engine, and UI page (see `TODO.md`)
- [ ] G7 — Continue UI redesign: Stochastic → Arp → Quantizer → MidiCv after Curve V1 done
- [ ] Iterate — Run tastemaker iterations with consolidate cadence against each trunk

## Decisions made (Note track Steps page) — DONE

- User converged on TRUNK (firmware port) as the base. V1/V2/V3 experiments remain as alternatives.
- Final tabs: GATE / RETRIG / LEN / NOTE / COND (matching firmware)
- Sandbox has Note Track Edit tab with TRUNK + V1/V2/V3 variations

## Decisions made (Curve track Steps page) — CONVERGED ON V1

- **V1 is the chosen design.** TRUNK dropped entirely (breaking change to project files; acceptable).
- V1 = segment assembler model (`drawCurveV1`), tabs: SHPE / SKEW / LEN / LVL / OFST
- V1 model: N variable-length segments chain and loop; loop length = sum of segment lengths (implicit)
- Each segment: shape (spike→sine→square via power fn), skew (peak position), length (pulses), level, offset
- Shape math: `evalSegment(phase, shape, skew)` — skewed half-sine with power-function morphing
- Output: unipolar bumps (offset → offset+level → offset per segment); gate = trigger at segment start
- Step buttons select segments (not steps); up to 16 segments
- V1 display: segment numbers row, proportional-width curve, indicator strip per tab, play scanline
- Future: euclidean trigger sequencer per segment (not in V1 scope)

## Backlog

- [ ] Port remaining finalized page designs from sandbox to C++ painters (after Curve V1 done)
- [ ] LED state design: specify LED state for each finalized page variation
- [ ] Track-level "humanize" parameter (replaces per-step length/note variation)
- [ ] Song page design (deferred until other pages settle)
- [ ] Dashboard interaction model: read-only vs mute-toggle (revisit after Perform + Track Edit settle)

## Key Pointers

| What | Where |
|---|---|
| **Curve V1 implementation spec** | `PLAN.md` (project root) |
| **Curve V1 task checklist** | `TODO.md` (project root) |
| **UI redesign status** (read first for UI work) | `agent-docs/ui-redesign/02-sandbox-status.md` |
| Sandbox page (active design file) | `agent-docs/ui-redesign/sandbox/src/routes/+page.svelte` |
| Design language rubric | `agent-docs/ui-redesign/01-design-language.md` |
| **Design ideas / emerging principles** | `agent-docs/ui-redesign/08-design-ideas.md` |
| Track-type parameter inventory | `agent-docs/ui-redesign/06-track-type-inventory.md` |
| IA decisions | `agent-docs/ui-redesign/00-ia.md` |
| Iteration feedback ledger | `agent-docs/ui-redesign/07-iteration-feedback.md` |
| Feature log | `agent-docs/features-log.md` |
