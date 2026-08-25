# Features Log

Append-only log of shipped features. Newest first. Links to detailed agent-docs where available.

---

## UI reference

For a full map of all pages, button combos, and held-button overlays, see [`agent-docs/ui-state.md`](../ui-state.md).

---

## 2026-08-24 — Curve Track: mirrored playback when running backwards (Version53)

With Backward, PingPong or Pendulum the curve track played its segments in reverse *order*, but each segment was still traversed left-to-right — so a sharp-attack envelope stayed a sharp-attack envelope on the way back and the sequence never actually sounded reversed.

**The mirror is free.** The output is a pure function of a phase, so the time reverse is just `1 - phase`; every curve has a mirrored twin with nothing extra stored:

```cpp
float phase = _sequenceState.direction() < 0 ? 1.f - fraction : fraction;
```

| Run mode | Behaviour |
|---|---|
| Forward | never mirrored |
| Backward | always mirrored, including the wrap from first back round to last |
| Pendulum | mirrored on the descending leg; the repeated endpoint plays forward once then mirrored once |
| PingPong | mirrored on the descending leg |
| RandomWalk | mirrored whenever the walk steps back one |
| Random | never mirrored — arbitrary jump distances, so no direction of travel |

**The prerequisite was fixing `SequenceState::direction()`**, which had **no callers anywhere** and showed it: PingPong, Pendulum and RandomWalk maintained it, while Forward, Backward and Random never touched it. Since `reset()` sets it to 1, Backward reported `+1`, and a stale `-1` from Backward or a Pendulum descent would survive a run-mode change and mirror everything forever. Both `calculateNextStepFree` and `calculateNextStepAligned` now set it in every mode; the aligned Pendulum/PingPong branches derive it from `absoluteStep` the same way `_nextStep` is, instead of a toggle keyed off the previous step.

A rule based on the raw step transition (`step == prevStep - 1`) was rejected: it breaks on Backward's wrap (a +n−1 jump in index terms that is still backwards travel, leaving one segment per cycle unmirrored) and on Pendulum's repeated endpoint (delta 0, so the transition cannot say which leg you are on).

`_segmentFraction` is assigned the mirrored phase, so the play scanline sweeps **right-to-left** when reversed and stays on the point of the drawn curve actually reaching the jack. The drawn curve is never mirrored — it is the segment's definition.

**Blast radius is narrow:** a symmetric curve (SKEW 50) mirrors to itself, so only skewed segments change at all. There is a test asserting exactly that.

**Not changed:** the gate still fires at the segment boundary, so a mirrored sharp-attack segment has its transient at the *end* while the gate marks the start.

**Also removed:** `playMode` and `fillMode` on `CurveTrack` — dead like the Version52 removals, since the engine always calls `advanceFree` and `_fillSequence` was assigned but never read. The CurveTrack-local `FillMode` enum goes with them; `Types::PlayMode` stays (Note/Stochastic use it). Read becomes `reader.skip<uint8_t>(0, Version53)` twice, with zeroed placeholders on write for older formats — note `CurveTrack::write` now needs two separate legacy flags, one per version boundary.

**Also fixed:** `CurveSequence`'s `divisor` / `runMode` / `firstStep` / `lastStep` python properties were bound straight to setters carrying a defaulted `routed` argument that pybind does not honour, so assigning to them raised `TypeError` — same bug as the CurveTrack properties fixed in Version52. Now wrapped in lambdas.

**Tests:** `curve_encoder_skew_test.py` grew to 94 checks — instant-edge counting per run mode (Forward all rises / Backward all falls / Pendulum and PingPong mixed), the stale-direction case, Random staying forward, symmetric curves being unaffected, and the removals.

**Files changed:** `SequenceState.h/cpp`, `CurveTrackEngine.h/cpp`, `CurveTrack.h/cpp`, `ProjectVersion.h`, `CurveTrackListModel.h`, `python/project.cpp`.

---

## 2026-08-24 — Curve Track: unipolar Range on the track, dead settings removed (Version52)

**The output range was effectively invisible.** `CurveSequence::_range` existed and the engine used it, but the Sequence page shows only `Name` (`rows()` returns `ConfigPageRows` = 1) and `Range` is not in the STEPS quick-edit map — the only way to reach it was the Overview page's quick-edit overlay. Its default was `Bipolar5V`, so a curve segment idled at **−5V** and peaked at +5V, a poor fit for a V1 model that produces unipolar bumps by construction.

**Range is now a per-track, unipolar setting.** Moved from `CurveSequence` to `CurveTrack` and shown on the Track page under Shape Curve as `Range  0..5V`, editable 1V–5V in 1V steps. The hardware ceiling is **±5V, not 10V** — `Calibration::CvOutput` spans −5…+5 and `voltsToValue()` hard-clamps there. Bipolar output stays reachable through the track `Offset` (±5.00V): Range 0..5V with Offset −2.50V gives −2.5V…+2.5V.

Stored as `Types::VoltageRange` clamped to the `Unipolar1V…Unipolar5V` entries; a bipolar value maps to the unipolar entry of the same voltage. `editRange` clamps as an `int` before converting to the enum — `VoltageRange` is `uint8_t` backed, so a negative wrapped to 255 and landed on 5V instead of stopping at 1V (caught by the new test).

**Dead weight removed.** `_curveMin` / `_curveMax` and `_shapeProbabilityBias` / `_gateProbabilityBias` are gone from `CurveTrack`, together with the `CurveMin` (39), `CurveMax` (40) and `ShapeProbabilityBias` (22) routing targets and the dead `CurveSequence::Step::gateProbability` accessors. Nothing in the V1 engine ever read any of them. **`GateProbabilityBias` (9) stays** — Note, Arp and Stochastic tracks each own one, and `NoteTrackEngine.cpp:379` reads it.

**No migrations needed.** Routes serialize targets by stable id via `targetSerialize`, and `readEnum` falls back to `Target::None` for an unrecognized id, so old routes pointing at ids 22/39/40 degrade gracefully; all 25 remaining targets were verified to round-trip. The removed track fields sit mid-blob, so reads became `reader.skip<>(…, Version52)` — note `_min`/`_max` were written as whole `Routable<float>` values (8 bytes each), not `.base` like every other field. The writer still emits zeroed placeholders when asked for a pre-Version52 format, so the Version49/50/51 migration tests keep working.

**Also fixed:** the `slideTime` / `offset` / `rotate` python properties were bound directly to setters carrying a defaulted `routed` argument, which pybind does not honour — assigning to them raised `TypeError`. Now wrapped in lambdas.

**Tests:** `curve_encoder_skew_test.py` grew to 83 checks — unipolar 0V/5V and 0V/1V on the DAC, negative output via the track offset, bipolar values clamping, the encoder stopping at both ends, and Version49/51 projects defaulting to 5V.

**Files changed:** `CurveTrack.h/cpp`, `CurveSequence.h/cpp`, `ProjectVersion.h`, `Routing.h/cpp`, `CurveTrackEngine.cpp`, `CurveTrackListModel.h`, `CurveSequenceListModel.h`, `OverviewPage.cpp`, `LaunchpadController.cpp`, `python/project.cpp`.

---

## 2026-08-23 — Curve Track: exponential fall at the low end of SHPE (Version51)

The segment contour was `sin(t·π)^p`. Every power of a sine has **zero slope at the peak** (`d/dt sin^p = p·sin^(p-1)·cos`, which vanishes where `cos = 0`), so the top was always rounded — at SHPE 0 the value was still 99.6% after 2% of the segment and 53% at the quarter point. That bell meant the sharp-attack envelopes unlocked by the Version50 skew work still decayed like a bell rather than a pluck.

**New shape family.** `evalSegment` now builds the contour around `u`, the normalized distance from the peak (`cos(u·π/2)` is identical to `sin(t·π)`), so the two halves of the range can use different families:
- **SHPE 0–50** blends an exponential fall into the half sine — `m·sine + (1-m)·(exp(-k·u) - e^-k)/(1 - e^-k)`, `m = shape·2`, `k = (1-m)·expCurve`. The normalization pins the fall to exactly 1.0 at the peak and 0.0 at the edges, so segments still join cleanly. The exponential has a **cusp** at the peak, which is the whole point.
- **SHPE 50–100** is untouched, and SHPE 50 is bit-for-bit the same half sine as before.
- The exponential applies to both sides of the peak, so SKEW keeps meaning "peak position": SKEW 0 gives an exponential decay, SKEW 100 a reverse swell, SKEW 50 a symmetric cusped spike.

**`Shape Curve` track setting (Version51).** Steepness is per-track and lives on the Track page below Mute Mode: Gentle (k=4) / Medium (k=6, default) / Snappy (k=8). Stored on `CurveTrack`, threaded to `evalSegment` as a plain `float` so the curve math keeps no dependency on `CurveTrack`. Pre-Version51 projects default to Medium via `clear()`, no explicit migration.

`CurveTrack::write` now guards the new field on `writer.writerVersion()` so that writing at an older version produces a file an older reader can consume — the test harness relies on this to exercise the read migrations.

**Sound change:** existing projects with low SHPE values sound different. That is the intent of the change; storage is untouched.

**Tests:** `curve_encoder_skew_test.py` grew to 64 checks — cusp detection (`1 - eval(0.02, 0, 0) > 0.05`, where the old bell gave 0.004), convex front-loaded decay, exact endpoints at every skew, continuity across SHPE 50, SHPE 50 still matching `sin(t·π)`, output within [0,1] over the whole parameter grid, the three steepness settings ordering correctly, the setting reaching the DAC, and Version49/50 projects defaulting to Medium.

**Files changed:** `CurveSequence.h`, `CurveTrack.h/cpp`, `ProjectVersion.h`, `CurveTrackListModel.h`, `CurveTrackEngine.cpp`, `CurveSequenceEditPage.cpp`, `python/project.cpp`, sandbox `+page.svelte`.

---

## 2026-08-23 — Curve Track: 1-unit encoder steps, full-range skew (Version50)

Two usability fixes on the Curve track STEPS page.

**Encoder granularity.** The page edited in normalized float space with a hard-coded `0.05f` per detent while the readout printed `norm * 100`, so every click jumped ~5 display units. `setXNorm()` re-quantized with `int(v*N + 0.5f)`, making the step asymmetric (+3 raw up, −2 raw down on SHPE/SKEW) — turning back did not return to the original value.

- All UI editing and readouts moved to an integer **percent domain** (`shapePercent()` / `skewPercent()` / `levelPercent()` / `offsetPercent()` on `CurveSequence::Step`). One detent = 1 unit, SHIFT = 10 (matching the coarse-modifier convention used elsewhere in the firmware; SHIFT was previously the *fine* modifier on this page only).
- Percent round trips are exact in both directions, so stepping is symmetric.
- SHPE/SKEW widened from 6 to 7 bits (0–127) by reclaiming the unused 4-bit `shapeVariationProbability` field — 64 values could not back a 0–100 scale. `Step::read()` migrates Version49 projects (`raw * 127 / 63`, skew moves from bit 6 to bit 7).
- The same encoder logic is duplicated in `OverviewPage`; it was updated too and gained the SHIFT branch it never had.

**Full-range skew.** `evalSegment` clamped skew to `[0.02, 0.98]` as a divide-by-zero guard, so an envelope always ramped up over the first 2% and could never start at full level; 6-bit storage also made raw {0,1} and {62,63} degenerate pairs, wasting a detent at each end.

- The clamp is replaced by exact handling of the endpoints: **skew 0** starts the segment at full amplitude and decays (completely sharp attack, SHPE picks the decay contour); **skew 100** rises across the segment and cuts off at the end (sharp release).
- The SKEW indicator tick now scales by `segW - 1`, so it lands on the segment edges at 0 and 100 instead of over-reporting while the curve had stopped moving.

**Tests:** `src/apps/sequencer/tests/ui/curve_encoder_skew_test.py` — 8 sections covering the percent round trip, per-detent stepping, symmetry, SHIFT, multi-segment editing, skew endpoints (curve math + DAC edge detection), and the Version49 migration. All pass.

**Files changed:** `CurveSequence.h/cpp`, `ProjectVersion.h`, `CurveSequenceEditPage.cpp`, `OverviewPage.cpp`, `python/project.cpp`, sandbox `+page.svelte`, `agent-docs/features/curve-track.md`.

**Known pre-existing issue (not fixed):** `OverviewPage.cpp:168` still renders the curve overview strip through the legacy `Curve::function(step.shape())` table, i.e. it reads the V1 shape percent as an old curve-type enum index.

---

## 2026-06-07 — Quantizer Track V2: trigger source scroll, CvGate, loop mode (Version47/48)

Expanded Quantizer from V1 (4 tabs, 3 trigger modes) to V2 (5 tabs, 13 trigger sources, loop record/playback).

**Trigger redesign (Version48):**
- TRIG tab replaced 3-chip FREE/INT/EXT with a unified 13-position scroll: FREE → INT+T1..T8 → EXT+CV1..CV4
- New `CvGate` trigger mode — rising edge on a CV input (threshold-based) fires a sample
- `QuantizerTrackListModel` collapsed two broken rows (`TriggerMode` + `TriggerTrack`) into one unified `TriggerSource` row using `editTriggerSource()` / `printTriggerSource()`
- Spurious-edge absorption: `_lastCvChannel` guard on CvGate entry (mirrors `_lastTriggerTrack` on External) — prevents false fires when mode or channel changes
- Migration: old projects with Internal trigger mode → Free on load

**Always-on sequence clock:**
- Clock advancement moved out of the deprecated Internal trigger case — fires `_samplePending` every tick for all trigger modes
- Gate/CV remain atomic via `commit()`; gate fires `SampleDelayTicks` (~10ms) after step boundary
- Free mode does NOT get clock-driven samplePending outside Loop (hysteresis is sole commit path; prevents Rec double-fill)

**Loop mode (LOOP tab, Version47):**
- Buffer stores un-transposed note indices; octave/transpose applied live at playback
- Free+Loop advances at clock rate
- External/CvGate do NOT fire samplePending in Loop — loop advances from clock only, preventing double-advance of `_loopIndex`
- `loopLength` and `loopStart` model params added

**Engine tests:** `src/apps/sequencer/tests/ui/quantizer_loop_test.py` — 6 tests (internal/free/external rec+loop, window, start-offset, clock-rate loop). All pass.

**Files changed:** `QuantizerTrackEngine.h/cpp`, `QuantizerTrack.h/cpp`, `ProjectVersion.h`, `QuantizerSequenceEditPage.h/cpp`, `QuantizerTrackListModel.h`, `python/sequencer.cpp`, `python/project.cpp`.

---

## 2026-06-04 — Arp Track V2: euclidean + note-pool model (Version46)

Full redesign of the Arp track. Old `Arpeggiator.h/cpp` per-step grid replaced by a degree-mask + dual-euclidean model. See [`agent-docs/features/arp-track.md`](features/arp-track.md) for full details.

**Model changes (`ArpSequence`, Version46):**
- `degreeMask` (7-bit) — selects active scale degrees; replaces per-step note grid
- `rhythmN/K/R` + `rhythmGateLen` — euclidean rhythm generator drives gate on/off
- `modN/K/R` + `modMode` — secondary euclidean pattern with 6 mod modes: Off / Accent / Mask / Combine / Ratchet / Hold
- `arpOrder` (0–4 implemented: UP / DOWN / UP-DN / DN-UP / RAND), `arpOctaves`, `arpLength`

**Engine (`ArpTrackEngine`):**
- Bresenham euclidean precomputed into `_rhythmPat`/`_modPat`; lazy-recomputed on param change
- Note pool rebuilt from `degreeMask` each rhythm hit
- `nextNoteCv()` cycles `_arpPhase` through pool × octaves; bounce modes use period `2*(n-1)` with no repeated endpoints
- No MIDI input — note pool is degree-mask only

**UI:** 4-tab `ArpSequenceEditPage`: NOTE / RHYTHM / MOD / ARP

**Files changed:** `ArpSequence.h/cpp`, `ArpTrackEngine.h/cpp`, `ArpSequenceEditPage.h/cpp`, `ArpSequencePage.cpp`, `ArpSequenceListModel.h`, `LaunchpadController.h/cpp`, `ClipBoard.cpp`, `ProjectVersion.h`, `python/project.cpp`.

---

## 2026-05-14 — Fix: Arp & Stochastic tracks now use scale degrees

Arp and Stochastic tracks were stuck displaying and rendering as C Major regardless of the project scale. Two root causes:

1. `scale.isNotePresent(step.note())` was called throughout `ArpSequenceEditPage` and `StochasticSequenceEditPage` to dim/brighten steps. `isNotePresent` interprets its argument as a **chromatic semitone**, but `step.note()` stores a **scale-degree index**. Only degree values 0, 2, 4, 5, 7, 9, 11 (the Ionian pattern) appeared bright.

2. `bypassScale` paths grabbed `Scale::get(0)` — formerly the chromatic scale, now `Maj:Ionian` after the 2026-05 modal-scale refactor. All note-name rendering via `bypassScale.noteName(...)` produced Ionian degree names regardless of the project scale.

**Fix:**
- Removed `BypassScale` entirely from Arp and Stochastic tracks (engine, model `Step::clear()`, and all UI rendering). Step notes are always scale degrees.
- Replaced all `isNotePresent` dimming with `Color::Bright` (degrees are always valid in the project scale).
- Replaced all `bypassScale.noteName(...)` calls with `scale.noteName(...)` using the project scale.
- Simplified `ArpTrackEngine::evalStepNote`'s variation walk: removed the chromatic `isNotePresent` loop; offset now applied directly in degree space.
- Encoder shift+turn now jumps by `scale.notesPerOctave()` degrees (was gated on the now-removed chromatic scale check).
- Added `Layer::Note` to `StochasticSequence` so users can directly edit step degrees with the encoder (previously only settable via MIDI).
- `LaunchpadController` keyboard views for Arp/Stochastic now only light scale-degree keys; non-scale chromatic positions are unlit.

**Files changed:** `ArpTrackEngine.cpp`, `StochasticEngine.cpp`, `ArpSequence.cpp`, `StochasticSequence.h/.cpp`, `ArpSequenceEditPage.cpp`, `StochasticSequenceEditPage.cpp`, `LaunchpadController.cpp`.

---

## 2026-05-14 — Fix: scale selection stuck on Ionian after Tempo row removal

`ProjectPage.cpp` had a hardcoded `row == 5` to detect encoder-press on the Scale row and commit the scale preview to the project. Removing the `Tempo` row in phase 1 shifted Scale from row 5 to row 4, so the commit never fired and the scale was permanently stuck on Ionian (index 0).

Fix: made `ProjectListModel::Item` enum public and changed the check to `row == ProjectListModel::Scale` so it tracks the enum value rather than a magic number.

---

## 2026-05-14 — UI simplification: remove Logic track type (phase 2)

Removed the Logic track type entirely. Logic was a gate-masking sequencer that combined two Note track gate streams via AND/OR/XOR/NAND/XOR/RandomInput boolean logic — musically niche and the least-used track type.

**Deleted:** `LogicTrack.h/.cpp`, `LogicSequence.h/.cpp`, `LogicTrackEngine.h/.cpp`, `LogicSequenceEditPage.h/.cpp`, `LogicSequencePage.h/.cpp`, `LogicSequenceListModel.h`, `LogicTrackListModel.h`.

**Removed** all Logic cases from: `Track.h`, `Engine.h/.cpp`, `Project.h`, `NoteTrack.h/.cpp` (logicTrack/logicTrackInput fields), `TrackPage`, `TopPage`, `OverviewPage`, `Pages.h`, `LaunchpadController`, `SequencePainter`, `ClipBoard`, `FileManager`, `Routing`, Python bindings, CMakeLists.

**Serialization:** Old projects with `TrackMode::Logic` (serialized as `4`) remap to Note on load.

**TrackModeListModel:** Removed the `row < 2` guard that blocked Logic from tracks 0–1. All remaining track types (including Quantizer) are now selectable on every track.

---

## 2026-05-14 — UI simplification: remove redundant configuration (phase 1)

First pass of a broader UI redesign. Removed configuration rows that already have a dedicated hardware affordance, and eliminated per-sequence scale/root-note entirely.

**ProjectPage** — removed the `Tempo` row. Tempo is edited by holding the hardware Tempo button; the page row was redundant.

**Per-sequence scale and root-note** — removed from the data model (`NoteSequence`, `ArpSequence`, `LogicSequence`, `StochasticSequence`). The project-level scale/root is now the single source of truth for all sequences. All engines, edit pages, OverviewPage, and LaunchpadController updated to read `_project.selectedScale()` / `_project.rootNote()` directly. Serialization version bumped to **Version41**; old project files skip the dropped bytes on load.

**Sequence config pages** — removed rows that are duplicated by the Page+Step8–12 quick-edit overlay on each track's Edit page. After this change, `NoteSequencePage`, `CurveSequencePage`, `LogicSequencePage`, and `QuantizerSequencePage` show only `Name`. `StochasticSequencePage` and `ArpSequencePage` keep their unique rows (RestProb 2/4/8, Oct Range, Length Mod, and for Arp also Divisor/ResetMeasure).

Quick-edit overlays (Step13/14, previously Scale/RootNote) now show no-op `—` for all sequence types.

**Key files changed:** `ProjectListModel.h`, `{Note,Arp,Logic,Stochastic}Sequence.{h,cpp}`, `{Note,Arp,Logic,Stochastic,Curve}SequenceListModel.h`, `{Note,Arp,Logic,Stochastic,Quantizer,Curve}SequenceEditPage.cpp`, `OverviewPage.cpp`, `LaunchpadController.cpp`, `ProjectVersion.h`, Python bindings `project.cpp`.

---

## 2026-05-13 — Quantizer track mode

New `Quantizer` track type. Samples an input CV (from one of 4 hardware CV inputs or another track's CV output), quantizes to the project's global scale, and holds the value until re-triggered. Three trigger modes: **Free** (fires whenever the quantized degree changes), **Internal** (own gate sequencer lane, reuses NoteSequence), **External** (rising edge of another track's gate). Track-level octave and transpose applied after quantize. Outputs quantized CV on the track's CV jack; ~5ms pulse on the gate jack on each note change. The gate lane is a full NoteSequence, giving access to all existing menus (first/last step, run mode, divisor, scale via shift shortcuts). Especially useful with a Curve track as input for generative pitch sequences.

See: [`agent-docs/features/quantizer-track.md`](features/quantizer-track.md)

---

## 2026-05-11 — Scale preview while scrolling

Scale changes now apply immediately as you turn the encoder through the scale list, so you can hear the harmonic context shift in real time before committing. Previously the scale only updated on the second encoder press. Also fixed an out-of-bounds read in all four sequence list models (`_scales` array was sized 23, now 32, to accommodate 21 built-in + 4 user scales + Default). Affected: `NoteSequenceListModel`, `ArpSequenceListModel`, `LogicSequenceListModel`, `StochasticSequenceListModel`.

---

## 2026-05-11 — Fix: project-level scale change no longer remaps step degrees

Removed the remaining volt round-trip from `Project::setScale`. Previously, changing the **project default scale** (the global scale on the PROJECT page) would re-encode notes for every sequence using the default scale (`seq.scale == -1`), using a downward-search algorithm that could snap degrees down (e.g., degree 2 → 1 when moving from a flat-2 mode to Ionian). Per-sequence `setScale` was fixed earlier (commit `d5096f57` + working-tree changes to `NoteSequence`, `ArpSequence`, `LogicSequence`), but the project-level path in `Project.h` was missed. Now `step.note()` is untouched regardless of which layer the scale change originates from.

---

## 2026-05-11 — Fix: scale-change no longer remaps step degrees (per-sequence)

Removed the volt round-trip from `NoteSequence::setScale`, `ArpSequence::setScale`, and `LogicSequence::setScale`. Previously, switching scale would re-encode each step's note by converting to volts (old scale) and back (new scale), causing degrees to snap downward when the new scale had wider intervals at low degrees. Now `step.note()` is left untouched on scale change — the same degree pattern plays in the new harmonic context. `StochasticSequence::setScale` already had this behavior.

See: [`agent-docs/features/scales.md`](features/scales.md)

---

## 2026-05-11 — Modal scale refactor

Replaced the mixed bag of 20 built-in scales with 21 modal scales: 7 diatonic modes, 7 harmonic-minor modes, 7 melodic-minor modes. Semitones and Voltage removed. Every picker entry is now a 7-degree mode, matching the scale-degree step storage introduced in the previous feature. Labels prefixed by parent scale family (`Maj:`, `HM:`, `MM:`). Clean break — saved projects load with shifted scale indices.

See: [`agent-docs/features/scales.md`](features/scales.md)

---

## 2026-05-10 — Scale degrees as step note storage (commit `83be884c`)

`step.note` now stores a **scale-degree index** rather than a raw semitone offset. Changing the sequence's scale remaps all steps to preserve pitch (using `Scale::noteToVolts` / `noteFromVolts`). This makes sweeping the scale picker a first-class sound-design move — the step pattern stays, the harmonic context shifts.

See: [`agent-docs/features/scales.md`](features/scales.md)

---

## 2026-05-10 — Scale-change pitch preservation (commit `d5096f57`)

`NoteSequence::setScale` and `ArpSequence::setScale` now re-encode all step notes when the scale changes, so existing pitches are preserved as closely as possible rather than remapping raw degree indices.

---

## 2026-05-10 — Headless Python simulator harness (commit `0710978e`)

Added a Python `Session` wrapper (`src/apps/sequencer/tests/agent.py`) for driving the simulator headlessly. Enables fast, reproducible debugging without the hardware. Build target: `testsim`. See `CLAUDE.md` for full usage.
