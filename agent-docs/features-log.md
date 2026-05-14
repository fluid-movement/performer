# Features Log

Append-only log of shipped features. Newest first. Links to detailed agent-docs where available.

---

## UI reference

For a full map of all pages, button combos, and held-button overlays, see [`agent-docs/ui-state.md`](../ui-state.md).

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
