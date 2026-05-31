# Project Context

## Current Focus

Stochastic track UI redesign — design session in progress.
Curve V1 C++ implementation is complete and shipped (shape math, engine, UI page).
Sandbox now has the Stochastic TRUNK (faithful firmware port) and a V1 sketch as starting points.
Next: iterate on the Stochastic design from the TRUNK base.

## Active TODOs

- [ ] Stochastic — iterate on design from TRUNK base (`PG+STP2` in sandbox)
- [ ] G7 — Continue UI redesign: Arp → Quantizer → MidiCv after Stochastic converges
- [ ] Iterate — Run tastemaker iterations with consolidate cadence against each trunk

## Decisions made (Note track Steps page) — DONE

- User converged on TRUNK (firmware port) as the base. V1/V2/V3 experiments remain as alternatives.
- Final tabs: GATE / RETRIG / LEN / NOTE / COND (matching firmware)
- Sandbox has Note Track Edit tab with TRUNK + V1/V2/V3 variations

## Decisions made (Curve track Steps page) — DONE, C++ SHIPPED

- **V1 is the chosen design.** TRUNK dropped entirely (breaking change to project files; acceptable).
- V1 = segment assembler model (`drawCurveV1`), tabs: SHPE / SKEW / LEN / LVL / OFST
- V1 model: N variable-length segments chain and loop; loop length = sum of segment lengths (implicit)
- Each segment: shape (spike→sine→square via power fn), skew (peak position), length (pulses), level, offset
- Shape math: `evalSegment(phase, shape, skew)` — skewed half-sine with power-function morphing
- **Max shape = perfect flat hold** (block/step sequencer mode): guard `p <= 0 → return 1.0f`
- Output: unipolar bumps (offset → offset+level → offset per segment); gate = trigger at segment start
- Step buttons select segments (not steps); up to 16 segments
- V1 display: segment numbers row, proportional-width curve, indicator strip per tab, play scanline
- `evalSegment` in sandbox kept in sync with C++ formula

## Decisions made (Stochastic track Steps page) — IN PROGRESS

- TRUNK reproduced faithfully in sandbox: 12 steps at 16px each, **centered** (x = i×16 + 32), 32px margins each side
- TRUNK tabs: GATE / RETRIG / LEN / NOTE / COND
- GATE tab shows GateProbability bars (not note names) — the defining visual of the stochastic track
- V1 sketch also present: full-width probability fill cells — to be iterated from TRUNK base
- Design challenge: communicate per-step probability with only 3 brightness levels

## Backlog

- [ ] Port remaining finalized page designs from sandbox to C++ painters (after each track converges)
- [ ] LED state design: specify LED state for each finalized page variation
- [ ] Track-level "humanize" parameter (replaces per-step length/note variation)
- [ ] Song page design (deferred until other pages settle)
- [ ] Dashboard interaction model: read-only vs mute-toggle (revisit after Perform + Track Edit settle)

## Key Pointers

| What | Where |
|---|---|
| **UI redesign status** (read first for UI work) | `agent-docs/ui-redesign/02-sandbox-status.md` |
| Sandbox page (active design file) | `agent-docs/ui-redesign/sandbox/src/routes/+page.svelte` |
| Design language rubric | `agent-docs/ui-redesign/01-design-language.md` |
| **Design ideas / emerging principles** | `agent-docs/ui-redesign/08-design-ideas.md` |
| Track-type parameter inventory | `agent-docs/ui-redesign/06-track-type-inventory.md` |
| IA decisions | `agent-docs/ui-redesign/00-ia.md` |
| Iteration feedback ledger | `agent-docs/ui-redesign/07-iteration-feedback.md` |
| Feature log | `agent-docs/features-log.md` |
