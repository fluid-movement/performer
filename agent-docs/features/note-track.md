# Note Track

## Purpose
The most commonly used track type. A step sequencer where each step has an independent
note, gate and a set of per-step **timing** parameters. Outputs one CV + gate pair per track.

**V2 (Version54)** replaced the old probability/variation parameters with timing ones: the
note track is now fully deterministic — randomness belongs to the Stochastic track — and gained
variable step length, gate length as a true percentage, Metropolix-style repeats, an arpeggiating
retrigger, and a portamento that actually reaches the target note.

## Key Source Files

| Role | Path |
|---|---|
| Data model (track settings) | `src/apps/sequencer/model/NoteTrack.h/cpp` |
| Data model (sequence + steps) | `src/apps/sequencer/model/NoteSequence.h/cpp` |
| Base track helpers | `src/apps/sequencer/model/BaseTrack.h`, `BaseTrackPatternFollow.h` |
| Real-time engine | `src/apps/sequencer/engine/NoteTrackEngine.h/cpp` |
| Step recorder | `src/apps/sequencer/engine/StepRecorder.h/cpp` |
| Slide helper | `src/apps/sequencer/engine/Slide.h` |
| Sequence display page | `src/apps/sequencer/ui/pages/NoteSequencePage.h/cpp` |
| Sequence edit page | `src/apps/sequencer/ui/pages/NoteSequenceEditPage.h/cpp` |
| UI list model | `src/apps/sequencer/ui/model/NoteTrackListModel.h` |

## Data Model

### `NoteTrack` (track-level settings, persisted per track)
- `_fillMode` — `FillMode` enum: None | Gates | NextPattern | Condition
- `_fillMuted` — whether fill mutes the track
- `_gateMode` — `GateMode`: Retrigger (gate closes on the boundary) or Legato
  (a 100% gate length is held across the boundary and the next slot does not re-fire)
- `_slideType` — `Types::SlideType`: Linear | Exponential (default, TB-303/RC) | Logarithmic
- `_slideTime` — portamento time, stored 1–100 and interpreted as `value * 5` **milliseconds**
  (5–500 ms; default 12 = 60 ms). Routable via `Routing::Target::SlideTime`.
- `_octave` — octave offset (–10 to +10, routable)
- `_transpose` — semitone transpose (–100 to +100, routable)
- `_rotate` — step rotation (–64 to +64, routable)
- `_sequences` — array of 17 `NoteSequence` (16 patterns + 1 snapshot)

There is **no `_playMode`**: variable step length is incompatible with `PlayMode::Aligned`,
whose `advanceAligned()` derives the step index in closed form from `relativeTick / divisor`.
The engine always advances freely, as `CurveTrackEngine` does.

### `NoteSequence` (per-pattern data)
- `_divisor` — clock divisor (controls playback speed)
- `_resetMeasure` — reset period in measures
- `_runMode` — `Types::RunMode` (Forward, Backward, PingPong, Random, etc.)
- `_firstStep`, `_lastStep` — active step range (0–63)
- `_steps` — array of 64 `Step`

### `NoteSequence::Step` layers (bit-packed per step)

Ten layers across five tabs. 46 of 64 bits used; `_data0` is full, `_data1` has 18 bits spare.

| Tab | Layer | Range | Bits | Description |
|---|---|---|---|---|
| GATE | Gate | bool | `_data0` 0 | Whether the step sounds at all |
| GATE | StepLength | 0–19 | `_data0` 2–6 | Index into `StepLengthRatios` — the step's duration as a ratio of the divisor |
| GATE | GateLength | 1–100 | `_data0` 7–13 | Gate-high percentage **of the retrigger slice** |
| RETRIG | Retrigger | 1–8 | `_data0` 21–23 | Slices the step into N equal parts |
| RETRIG | ArpRetrigger | –15…+15 | `_data0` 27–31 | Scale degrees added per slice; resets each repeat |
| REPEAT | Repeat | 1–8 | `_data0` 24–26 | Replays the whole step body; **extends** the step |
| NOTE | Note | –63…+63 | `_data0` 14–20 | Scale degree |
| NOTE | Slide | bool | `_data0` 1 | Portamento **into** this note |
| COND | Condition | 0–127 | `_data1` 0–6 | Gates the whole step (`Types::Condition`) |
| COND | RetriggerCondition | 0–127 | `_data1` 7–13 | When it fails, the step collapses to retrigger = 1 |

`gate`, `slide` and `note` deliberately keep their pre-Version54 bit positions so the migration
is a straight copy for those three.

**Step length ratio table** (`NoteSequence::stepLengthRatio`, default index 7 = `1/1`):

```
1/8  1/6  1/4  1/3  1/2  2/3  3/4   1   5/4  4/3
3/2   2   5/2   3    4    5    6    8    12   16
```

`stepLengthTicks(index, divisor) = max(2, divisor * num / den)`.

## Engine Logic (`NoteTrackEngine`) — the slot scheduler

A step is divided into `repeat * retrigger` equally spaced **slots**. The engine schedules one
slot at a time rather than expanding the whole step into the queue up front.

1. `tick()` calls `advanceSlot()` whenever `tick >= _nextSlotTick`.
2. At slot 0, `beginStep()` latches everything decided once per step: `SequenceState::advanceFree`
   picks the step; the gate condition is evaluated (and advances the `Pre`/`NotPre` chain); the
   retrigger condition is evaluated (read-only on that chain — two writers would make `Pre`
   meaningless); `_stepTotalTicks`, `_totalSlots`, `_retrigCount` and `_baseNote` are computed.
3. Slot onsets are derived from the step start, never accumulated:
   `slotOnset(k) = _stepStartTick + (_stepTotalTicks * k) / _totalSlots`.
   This keeps rounding error bounded and lands the final slot exactly on the step end.
4. Each slot pushes at most **2 gate + 1 CV event**. In Retrigger gate mode the gate-off is
   clamped to `slotLen - 1` so it cannot coincide with the next gate-on (which the queue would
   collapse into a permanently high gate).
5. Arp retrigger: note = `_baseNote + (_slotIndex % _retrigCount) * arpRetrigger`. The slide flag
   rides only on slot 0, so intra-step arp jumps snap rather than glide.
6. A gate-off step pushes nothing and changes no CV — "nothing happens for the duration".
7. `Slide::Ramp` (see below) is advanced in `update(dt)` at ~1 kHz.

Scheduling one slot at a time also **fixes a queue overflow**: the old engine pushed up to 16
events in one `triggerStep`, and `SortedQueue`'s usable capacity is 15 (`size()` is
`(_write - _read) % Capacity`), so RETRIG 7 + LENGTH 15 silently muted the step entirely.

Key class: `src/apps/sequencer/engine/SequenceState.h` — tracks current step position, handles all run modes.

## Slide (`engine/Slide.h`)

Two distinct mechanisms live here:

- **`Slide::applySlide`** — the original one-pole exponential lag. Never reaches its target, which
  is correct for smoothing a *continuous* signal; `CurveTrackEngine` still uses it as a slew limiter.
- **`Slide::Ramp` + `Slide::shape`** — the note-to-note portamento. A bounded ramp with
  `shape(0) == 0` and `shape(1) == 1` for every type, so it is **guaranteed to arrive** and then
  pins to the target.

The old note-track slide used `applySlide` with `tau = 2·(slideTime/100)²` **seconds** and no
reference to tempo or step duration; at `slideTime = 50` it covered ~22% of the interval during a
1/16 step at 120 BPM, so the pitch never actually landed on the programmed note.

Research on the TB-303 informed the replacement: its glide is **constant-time** (~55 ms regardless
of interval), RC/exponential shaped, and completes because the slide window is bounded. `Exponential`
reproduces that — measured 77% of the way through a 60 ms glide at 20 ms, versus 35% linear and 9%
logarithmic.

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `NoteSequencePage` | `ui/pages/NoteSequencePage.h/cpp` | Overview of steps in a pattern, LED display |
| `NoteSequenceEditPage` | `ui/pages/NoteSequenceEditPage.h/cpp` | Per-step layer editing (rotate encoder to cycle layers) |

The edit page cycles through `NoteSequence::Layer` values. Each layer maps to a different bit-field in `Step`.

## Configuration Constants
```cpp
CONFIG_TRACK_COUNT   8    // tracks total
CONFIG_PATTERN_COUNT 16   // patterns per track
CONFIG_STEP_COUNT    64   // steps per pattern
CONFIG_SEQUENCE_PPQN 48   // ticks per quarter note for sequence resolution
```

## Change Guide

| Task | Where to look |
|---|---|
| Add a new per-step parameter | Add a `Layer` enum value in `NoteSequence.h`, add bit-field storage in `Step` (`_data1` has 18 free bits), update `layerName()`, `layerRange()`, `layerDefaultValue()`, `layerValue()`/`setLayerValue()`, `clear()`, handle in `NoteTrackEngine.cpp`, add rendering + a `switchLayer` slot in `NoteSequenceEditPage.cpp`, and bind it in `python/project.cpp` |
| Add a step-length ratio | `NoteSequence::stepLengthRatio` table + bump `StepLengthCount` (the field is 5 bits, so up to 32 entries) |
| Change step timing / retrigger / repeat | `NoteTrackEngine::advanceSlot` and `beginStep` |
| Change slide/portamento | `engine/Slide.h` (`shape`/`Ramp`) + `TrackEngineHelpers::drainCvQueue` |
| Add a new fill mode | `NoteTrack.h` `FillMode` enum + `NoteTrackEngine.cpp` fill handling |
| Add a new run mode | `src/apps/sequencer/model/Types.h` `RunMode` enum + `SequenceState.h/cpp` |
| Fix step recording bugs | `src/apps/sequencer/engine/StepRecorder.h/cpp` |

## Tests

`src/apps/sequencer/tests/ui/note_v2_test.py` — 36 checks across 11 sections: step length ratios,
gate length, retrigger, arp retrigger, repeat, legato, slide arrival + curve shape, retrigger
condition, the queue-overflow regression, gate-off inertness, and the Version53→54 migration.

Note the project's default scale (index 0) is **Ionian**, not chromatic — a step's `note` is a
degree in that scale, so degree 1 is 2 semitones.
