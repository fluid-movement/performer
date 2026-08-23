# Project Context

## Current Focus

**Stochastic V2, Arp V2, and Quantizer V2 C++ all shipped.** Stochastic (Version45): SIG-inspired 7-degree probability bars, OCT/LEN/LOOP tabs, Turing Machine loop mutation. Arp (Version46): euclidean + note-pool model, 4 tabs (NOTE/RHYTHM/MOD/ARP), UP/DOWN/UP-DN/DN-UP/RAND arp orders. Quantizer (Version47/48): 5 tabs (GATE/SRCE/TRIG/TUNE/LOOP), unified 13-position trigger source scroll, CvGate trigger mode, loop record/playback, always-on sequence clock.

Next: **UI redesign** — MidiCv track. Stochastic hardware bar ghosting (SSD1322 column coupling) remains an open visual issue.

## Active TODOs

- [ ] Stochastic — hardware bar ghosting: SSD1322 column capacitive coupling causes bars to "bleed" into each other visually. Confirmed NOT a software rendering bug (simulator is clean). Bars thinned to 18 px (colW/2) to reduce coupling — may need further mitigation or acceptance.
- [ ] Stochastic — fix engine bug: `Free` play mode is unimplemented (empty break in `StochasticEngine.cpp`); default is `Aligned` so not urgent
- [ ] Arp — remove `playMode` from ArpTrack entirely: `Free`/`Last` are unimplemented, `Aligned` is the only behavior, and the edit case in `ArpTrackListModel` is already a no-op. Removing it touches `ArpTrack.h/cpp` (field + serialization), `ArpTrackListModel.h` (3 switch cases), and `ArpTrackEngine.cpp` (flatten the play mode switch in `tick()`). Breaking serialization change — bump project version.
- [ ] Arp — version bump needed: euclidean canonical-rotation change (session 2025-06-04) altered the meaning of non-zero `rhythmR`/`modR`. Any Version46 project with non-zero rotation values will sound different. Bump to Version47 with migration that compensates for the `firstHit` offset, or accept the break and document it.
- [ ] Arp — refactor: `computeEuclidean` (engine) and `buildPat` (display) are identical; extract to a shared inline header to prevent future drift.
- [ ] G7 — Continue UI redesign: MidiCv (Quantizer done)
- [ ] Iterate — Run tastemaker iterations with consolidate cadence against each trunk

## Decisions made (Arp track V2) — DONE, C++ SHIPPED

- **V2 is the active model.** Old `Arpeggiator.h/cpp` per-step model replaced entirely (breaking change; Version46).
- Note pool: 7-bit `degreeMask` selects active scale degrees (not per-step notes).
- Two independent euclidean generators: **rhythm** (drives gate on/off) and **mod** (secondary pattern for accent/mask/ratchet/hold).
- Arp engine: UP / DOWN / UP-DN / DN-UP / RAND implemented; CONV / DIV fall back to UP (not yet implemented).
- UI tabs: **NOTE / RHYTHM / MOD / ARP** (4 tabs, `ArpSequenceEditPage`).
- No MIDI input — note pool is set via degree mask only.
- Play modes (Free/Last) unimplemented and to be removed — see backlog in Active TODOs.
- **Euclidean patterns use canonical form** (first hit at step 0, user rotation applied on top). Both `computeEuclidean` in the engine and `buildPat` in the display use identical logic — display and engine are in sync.
- **MOD display visual bug fixed**: `buildPat` now matches engine formula exactly; previously display auto-rotated patterns to canonical form independently, causing COMBINE/MASK overlap cells to disagree with actual gate output.
- **Selected param cell inverted text**: `drawParamBar` uses `BlendMode::Sub` to knock text out of bright fill — `Color::None` was painting solid rectangles.
- **Engine tests**: `src/apps/sequencer/tests/ui/arp_v2_test.py` — covers OFF / COMBINE / MASK modes and polyrhythmic overlap using rising-edge gate detection. `wait(N)` = N ms; 1 step = 125 ms at 120 BPM / divisor=12.

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
- **Version50**: SHPE/SKEW widened 6→7 bits (0-127); all five params edit in an integer 0-100 percent domain, 1 unit per detent, SHIFT = x10 coarse. Migration in `Step::read()` rescales Version49 values.
- **Skew reaches its extremes**: `evalSegment` no longer clamps to [0.02, 0.98] — skew 0 = instant attack, skew 100 = instant release. SKEW indicator tick scales by `segW - 1` so it lands on the segment edges.
- **Engine tests**: `src/apps/sequencer/tests/ui/curve_encoder_skew_test.py` — 8 sections, all pass.

## Decisions made (Quantizer track Steps page) — DONE, C++ SHIPPED (Version47/48)

- **V2 is the active design.** 5 tabs: **GATE / SRCE / TRIG / TUNE / LOOP**.
- GATE tab: 16-step gate grid (identical to trunk visual). `_stepSelection` handles multi-step editing. Always-on clock advances step playhead regardless of trigger mode.
- SRCE tab: 2-row chip selector — CvIn1–4 (top row), Track1–8 (bottom row). Free-scroll encoder wraps modularly. `editInputSource()` called directly.
- TRIG tab: unified 13-position scroll — FREE → INT+T1..T8 → EXT+CV1..CV4. Single `TriggerSource` row in `QuantizerTrackListModel`. `CvGate` trigger mode added (Version48). Migration: old Internal → Free on load. Spurious-edge absorption: `_lastCvChannel` guard on CvGate (mirrors `_lastTriggerTrack` on External) prevents false fires on mode/channel switch.
- TUNE tab: 2-column — OCTAVE left, TRANSPOSE right. Bipolar bar + numeric value. Hold Step 0 / Step 1 + encoder to edit.
- LOOP tab: loop record/playback. Buffer stores un-transposed note indices (octave/transpose applied live at playback). Free+Loop advances at clock rate. External/CvGate don't fire samplePending in Loop mode — loop advances from clock only, preventing double-advance of `_loopIndex`.
- **Always-on clock**: sequence clock fires `_samplePending` (not gate directly) every tick for all modes — gate/CV remain atomic via `commit()`. Free mode does NOT get clock-driven samplePending outside Loop (hysteresis is the sole commit path; prevents Rec double-fill).
- **Inverted text fix**: chip labels use `fillRect(Bright)` → `setBlendMode(Sub)` → `drawText`. `Color::None` produces solid black rectangles — always use `BlendMode::Sub`.
- Scale/RootNote are global project parameters — no per-track scale on Quantizer.
- **Engine tests**: `src/apps/sequencer/tests/ui/quantizer_loop_test.py` — 6 tests covering internal/free/external rec+loop, window, start offset, clock-rate loop. All pass.
- Quantizer UI page: `src/apps/sequencer/ui/pages/QuantizerSequenceEditPage.cpp`

## Decisions made (Stochastic track Steps page) — IN PROGRESS

- TRUNK reproduced faithfully in sandbox: 12 steps at 16px each, **centered** (x = i×16 + 32), 32px margins each side
- TRUNK tabs: GATE / RETRIG / LEN / NOTE / COND
- GATE tab shows GateProbability bars (not note names) — the defining visual of the stochastic track
- V1 sketch also present: full-width probability fill cells
- **V2 now the active direction** — SIG (Stochastic Inspiration Generator) inspired:
  - 7 scale-degree bars (not 12 chromatic, not step positions) — bar height = selection probability
  - Automatic density: sum of bar heights drives rest density implicitly (no separate rest param)
  - **Final tabs: NOTE / OCT / LEN / LOOP** (ART dropped — Performer has no envelope output, SIG's articulation doesn't translate)
  - NOTE tab: 7 vertical bars, degree labels 1–7, cursor = inverted column, playing = bright outline
  - Header matches TRUNK form (`fwDrawHeader`) — layer name shows current tab
  - OCT tab: 5 centered bars (-2 to +2), "0" column marked as default
  - LEN tab: 6 duration probability bars (1/16 through 2)
  - LOOP tab: Turing Machine-style mutation chance — step buttons set loop length (1–16), encoder sets `loopChance` (0=locked, 15=fully random). Each locked step has a per-step chance of being re-drawn from the probability distribution rather than replayed. Separate mutation RNG stream.
- **Engine has loop buffer** (`_lockedSteps: std::vector<StochasticLoopStep>` in `StochasticEngine`). Loop on/off via `useLoop()`.
- **Loop length is now stored as `_loopLength` (Version45)**, decoupled from `firstStep`/`lastStep`. Phase 2 engine accesses buffer with wrapping: `(firstStep + index) % 16`. This means changing `firstStep` shifts the loop's phase within the 16-step buffer without changing how many steps play.
- **Engine bug fixed**: `noteVariationProbability` defaults to 0, causing `sum==0` early-return → silence. Fixed in `triggerStep()`: when sum==0 fall back to equal weighting.
- **Bar rendering**: bars are 18 px wide (colW/2), centered in their 36 px column slot. Full column (36 px) is cleared each frame to prevent header-line bleed into inter-bar gaps.
- Performer stochastic track is directly based on the SIG module (confirmed from manual review)

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
| **Stochastic LOOP firmware plan** | `PLAN.md` |
| **Stochastic LOOP task list** | `TODO.md` |
| Stochastic engine | `src/apps/sequencer/engine/StochasticEngine.cpp` |
| Stochastic model | `src/apps/sequencer/model/StochasticSequence.h/.cpp` |
| Stochastic UI page | `src/apps/sequencer/ui/pages/StochasticSequenceEditPage.cpp` |
| Arp engine | `src/apps/sequencer/engine/ArpTrackEngine.cpp` |
| Arp UI page | `src/apps/sequencer/ui/pages/ArpSequenceEditPage.cpp` |
| Arp engine tests | `src/apps/sequencer/tests/ui/arp_v2_test.py` |
| Quantizer UI page | `src/apps/sequencer/ui/pages/QuantizerSequenceEditPage.cpp` |
