# Features Log

Append-only log of shipped features. Newest first. Links to detailed agent-docs where available.

---

## UI reference

For a full map of all pages, button combos, and held-button overlays, see [`agent-docs/ui-state.md`](../ui-state.md).

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
