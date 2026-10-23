# Arp Track (V2 — Euclidean + Note Pool)

## Purpose

Generates arpeggiated CV+gate output. A 7-bit **degree mask** selects which scale degrees are active; two independent **euclidean rhythm generators** (rhythm and mod) drive gate density and articulation; an **arp engine** cycles through the active notes in a chosen order across one or more octaves.

Shipped as Version46. The old per-step / `Arpeggiator.h` model is gone.

## Key Source Files

| Role | Path |
|---|---|
| Sequence data model | `src/apps/sequencer/model/ArpSequence.h/cpp` |
| Track-level settings | `src/apps/sequencer/model/ArpTrack.h/cpp` |
| Real-time engine | `src/apps/sequencer/engine/ArpTrackEngine.h/cpp` |
| Sequence edit page | `src/apps/sequencer/ui/pages/ArpSequenceEditPage.h/cpp` |
| Sequence overview page | `src/apps/sequencer/ui/pages/ArpSequencePage.h/cpp` |
| Track list model (track settings UI) | `src/apps/sequencer/ui/model/ArpTrackListModel.h` |
| Sequence list model | `src/apps/sequencer/ui/model/ArpSequenceListModel.h` |
| Clipboard | `src/apps/sequencer/model/ClipBoard.cpp` — `copyArpSequence` / `pasteArpSequence` |

## Data Model

### `ArpSequence` (per-pattern, 17 slots)

**Note pool**
- `_degreeMask` — 7-bit mask, bit 0 = degree 1. At least one bit should be set; engine handles empty pool gracefully (silent).

**Rhythm euclidean** (drives gate on/off)
- `_rhythmN` — steps (1–16)
- `_rhythmK` — pulses (0–N)
- `_rhythmR` — rotation (0–15)
- `_rhythmGateLen` — gate length (0–19); prints as `(val+1)*5 %`, default 7 → 40%

**Mod euclidean** (secondary pattern, modifies rhythm hits)
- `_modN` — steps (1–16)
- `_modK` — pulses (0–N)
- `_modR` — rotation (0–15)
- `_modMode` — `ModMode` enum: Off / Accent / Mask / Combine / Ratchet / Hold

**Arp engine**
- `_arpOrder` — 0–6: UP / DOWN / UP-DN / DN-UP / RAND / CONV† / DIV†
- `_arpOctaves` — 1–4
- `_arpLength` — sequence loop length in steps (1–64)

† CONV and DIV fall back to UP (not yet implemented).

**Common**
- `_divisor` — clock divisor (routable)
- `_resetMeasure` — auto-reset period in bars (0 = off)

### `ArpTrack` (track-level, shared across patterns)

- `_playMode` — `Types::PlayMode` (Aligned only; Free/Last unimplemented and to be removed — see CONTEXT.md backlog)
- `_fillMode` — None / Gates / NextPattern / Condition
- `_fillMuted` — mute on fill
- `_cvUpdateMode` — Gate / Always
- `_slideTime`, `_octave`, `_transpose` — all routable
- `_sequences[17]` — array of `ArpSequence`

## Engine Logic (`ArpTrackEngine`)

On each clock tick (Aligned play mode only):

1. **Pattern change detection** — lazy-recompute `_rhythmPat[16]` / `_modPat[16]` if any euclidean param changed since last tick (Bresenham algorithm in `computeEuclidean()`).
2. **Rhythm gate** — check `_rhythmPat[_step % rhythmN]`. If no hit, skip. Step wraps at `arpLength`.
3. **Rebuild note pool** — `rebuildNotePool()` scans `degreeMask` into `_activeNotes` vector. Early return if empty.
4. **Mod gate** — check `_modPat[_step % modN]`. The `overlap` flag (rhythm hit ∧ mod hit) drives `ModMode`:
   - `Off` — no effect
   - `Accent` — gate length ×2
   - `Mask` — suppress note on overlap
   - `Combine` — suppress note on non-overlap (AND logic)
   - `Ratchet` — fire two notes per step on overlap
   - `Hold` — gate length ×4
5. **Next note** — `nextNoteCv()` maps `_arpPhase` → degree index → scale degree → CV voltage. Phase advances per order:
   - UP / DOWN: period = `n × octaves`, linear
   - UP-DN / DN-UP: period = `(2*(n-1)) × octaves`, bounce (no repeated endpoints)
   - RAND: random degree and octave, no phase advance
6. **Queue** — gate on/off events pushed to `SortedQueue<Gate>` with swing applied; CV pushed to `SortedQueue<Cv>`. Both drained in `tick()`.

## UI Pages

### `ArpSequenceEditPage` — 4 tabs

| Tab | Index | Controls |
|---|---|---|
| NOTE | 0 | 7 step buttons toggle degree mask; cursor selects; encoder does nothing |
| RHYTHM | 1 | Encoders: N / K / R / GATELEN |
| MOD | 2 | Encoders: N / K / R / MODE |
| ARP | 3 | Encoders: ORDER / OCTAVES / LENGTH |

### `ArpSequencePage`

Overview/navigation page. Step buttons show active degrees. No per-step editing.

## Serialization

`ArpSequence::read()` returns default values for `version < 46` (Version46 = euclidean+note-pool model). No migration of old per-step data needed — the old layout was incompatible.

## Change Guide

| Task | Where |
|---|---|
| Add a new arp order | `ArpSequence.h` — bump clamp to 0–N; `ArpTrackEngine.cpp` — add case in `nextNoteCv()` switch |
| Add a new mod mode | `ArpSequence.h` — `ModMode` enum; `ArpTrackEngine.cpp` — add case in `triggerStep()` switch |
| Change gate length formula | `ArpTrackEngine.cpp:triggerStep()` — `rawLen` calculation |
| Add new sequence parameters | `ArpSequence.h` fields + `ArpSequence.cpp` read/write + bump `ProjectVersion.h` |
| Remove play mode (backlog) | `ArpTrack.h/cpp`, `ArpTrackListModel.h`, `ArpTrackEngine.cpp:tick()` — see CONTEXT.md |
