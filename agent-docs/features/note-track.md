# Note Track

A deterministic step sequencer: each of up to 64 steps per pattern has a note (a scale degree), a
gate and a set of timing parameters. One CV + gate pair per track. Randomness belongs to the
Stochastic track, not here. Current model: **Version54**.

Decisions behind this design (`prep knowledge find note`): the track is deterministic; step length
is a ratio of the divisor; Repeat extends a step while Retrigger subdivides it; the engine is a
slot scheduler; slide is a bounded TB-303 style ramp; no Aligned play mode and no CV update mode;
only the gate condition writes the `Pre`/`NotPre` chain; Gate Mode Retrigger vs Legato.

## Key source files

| Role | Path |
|---|---|
| Track settings | `model/NoteTrack.{h,cpp}` |
| Sequence + steps (bit-packed) | `model/NoteSequence.{h,cpp}` |
| Name, pattern follow | `model/BaseTrack.h`, `model/BaseTrackPatternFollow.h` |
| Engine (slot scheduler) | `engine/NoteTrackEngine.{h,cpp}` |
| Step position and run modes | `engine/SequenceState.{h,cpp}` |
| Slide ramp and shapes | `engine/Slide.h` |
| Step recording from MIDI | `engine/StepRecorder.h` |
| Steps overview page | `ui/pages/NoteSequencePage.{h,cpp}` |
| Steps edit page | `ui/pages/NoteSequenceEditPage.{h,cpp}` |
| Config list rows | `ui/model/NoteTrackListModel.h`, `ui/model/NoteSequenceListModel.h` |
| Tests | `tests/ui/note_v2_test.py` |

## Data model

### `NoteTrack` (per track)

- `fillMode` (None / Gates / NextPattern / Condition) and `fillMuted`
- `gateMode`: `Retrigger` (default; a 100% gate closes one tick early so the next slot can re-fire)
  or `Legato` (a 100% gate is held across the boundary and the next slot's gate-on is suppressed)
- `slideType`: Linear / Exponential (default) / Logarithmic
- `slideTime`: stored 1-100, interpreted as `value * 5` milliseconds (5-500 ms, default 12 = 60 ms);
  routable (`Routing::Target::SlideTime`)
- `octave`, `transpose`, `rotate`: routable
- `sequences`: 17 `NoteSequence` (16 patterns + 1 snapshot); name and pattern follow come from the base classes

There is no play mode and no CV update mode: the engine always advances freely, and a gate-off step
changes nothing.

### `NoteSequence` (per pattern)

`divisor` (routable), `resetMeasure`, `runMode`, `firstStep`, `lastStep` (0-63), 64 `Step`s, the
step-record cursor `currentRecordStep` (routable), and the displayed `section` (4 sections of 16).
Scale and root note are **project-global**, not per sequence (`prep knowledge find scale`).

### `NoteSequence::Step` layers

Ten layers on five tabs. 46 of 64 bits are used: `_data0` is full, `_data1` has 18 spare bits.

| Tab | Layer | Range | Storage | Meaning |
|---|---|---|---|---|
| GATE | Gate | bool | `_data0` bit 0 | whether the step sounds |
| GATE | StepLength | 0-19 | `_data0` 2-6 | index into the ratio table below |
| GATE | GateLength | 1-100 | `_data0` 7-13 | gate-high percentage **of the retrigger slice** |
| RETRIG | Retrigger | 1-8 | `_data0` 21-23 | slices the step body into N equal parts |
| RETRIG | ArpRetrigger | -15..+15 | `_data0` 27-31 | scale degrees added per slice; resets each repeat |
| REPEAT | Repeat | 1-8 | `_data0` 24-26 | replays the whole step body; the step lasts `stepTicks * repeat` |
| NOTE | Note | -63..+63 | `_data0` 14-20 | scale degree (not semitones) |
| NOTE | Slide | bool | `_data0` bit 1 | portamento **into** this note |
| COND | Condition | 0-127 | `_data1` 0-6 | gates the whole step (`Types::Condition`) |
| COND | RetriggerCondition | 0-127 | `_data1` 7-13 | when it fails, the step collapses to retrigger = 1 |

`gate`, `slide` and `note` kept their pre-Version54 bit positions so the migration copies them.

Step length ratios (`NoteSequence::stepLengthRatio`, default index 7 = `1/1`; the field is 5 bits,
so up to 32 entries): `1/8 1/6 1/4 1/3 1/2 2/3 3/4 1 5/4 4/3 3/2 2 5/2 3 4 5 6 8 12 16`.
`stepLengthTicks(index, divisor) = max(2, divisor * num / den)`.

## Engine (`NoteTrackEngine`): the slot scheduler

A step is `repeat * retrigger` equally spaced **slots**, scheduled one at a time (never expanded into
the queue up front, which used to overflow `SortedQueue`).

1. `tick()` calls `advanceSlot()` whenever `tick >= _nextSlotTick`.
2. At slot 0, `beginStep()` latches everything decided once per step: `SequenceState::advanceFree`
   picks the step, the gate condition is evaluated (and advances the `Pre`/`NotPre` chain), the
   retrigger condition is evaluated (read-only on that chain), and `_stepTotalTicks`, `_totalSlots`,
   `_retrigCount` and `_baseNote` are computed.
3. Slot onsets derive from the step start: `slotOnset(k) = _stepStartTick + _stepTotalTicks * k / _totalSlots`.
4. Each slot pushes at most 2 gate events and 1 CV event. In Retrigger gate mode the gate-off is
   clamped to `slotLen - 1`.
5. Arp retrigger: note = `_baseNote + (_slotIndex % _retrigCount) * arpRetrigger`. The slide flag
   rides only on slot 0, so intra-step arp jumps snap rather than glide.
6. `Slide::Ramp` is advanced in `update(dt)` at about 1 kHz. `Slide::shape` is normalized
   (`shape(0) == 0`, `shape(1) == 1`) so the ramp always arrives. `Slide::applySlide` (a one-pole lag)
   remains only for `CurveTrackEngine`'s slew limiter and `MidiCvTrackEngine`'s legato portamento.

## UI

Edit page (`NoteSequenceEditPage`), footer tabs **GATE / RETRIG / REPEAT / NOTE / COND**:

| Input | Effect |
|---|---|
| F1-F5 | switch tab; pressing the active tab again cycles the tab's layers (GATE: Gate, StepLength, GateLength; NOTE: Note, Slide; RETRIG and COND: first/second) |
| Shift + F1-F5 | jump to the tab's secondary layer (GateLength, ArpRetrigger, Repeat, Slide, RetriggerCondition) |
| Step button | on the Gate layer toggles the gate; on other layers, select/hold steps; double-tap toggles the gate |
| Step select (hold, or Shift for persist) + encoder | edit the active layer of the selected steps |
| Encoder with no selection | walk to the next layer within the tab |
| Encoder press | toggle the gate on the selection |
| F-key held + Step | set that step's note to octave 1-5 of the scale |
| Left / Right | change section; with Shift shift the steps (or the record cursor while recording) |
| Shift + F3 with a selection | tie the selected notes |
| Page + Step 8-12 | quick edit FirstStep, LastStep, RunMode, Divisor, ResetMeasure |
| Page + Step 15 | toggle pattern follow display |
| Page + Step 6 | undo to the last in-memory snapshot of the sequence |
| context menu (Shift + Page) | INIT, COPY, PASTE, DUPL, GEN (see [sequence-generators](sequence-generators.md)) |

Values are read from the per-step strip; there is no detail popup. F1-F5 are the five function keys
(the code calls them F0-F4). General page conventions:
[ui-state.md](../ui-state.md), [config-pages.md](config-pages.md) for the track's settings page.

## Change guide

| Task | Where |
|---|---|
| Add a per-step parameter | `Layer` enum, bit-field in `Step`, `layerName/Range/DefaultValue`, `layerValue/setLayerValue`, `clear()` in `model/NoteSequence.h`; handle in `engine/NoteTrackEngine.cpp`; rendering and a `switchLayer` slot in `ui/pages/NoteSequenceEditPage.cpp`; bind it in `python/project.cpp`; bump `model/ProjectVersion.h` ([serialization](serialization.md)) |
| Add a step-length ratio | `NoteSequence::stepLengthRatio` table and `StepLengthCount` (max 32) |
| Change step timing, retrigger, repeat | `NoteTrackEngine::beginStep` and `advanceSlot` |
| Change slide | `engine/Slide.h` (`shape`, `Ramp`) and `applySlideUpdate` / `drainCvQueue` in `engine/TrackEngineHelpers.h` |
| Add a fill mode | `NoteTrack::FillMode` and the fill handling in `NoteTrackEngine.cpp` |
| Add a run mode | `RunMode` in `model/Types.h` and `engine/SequenceState.{h,cpp}` |
| Fix step recording | `engine/StepRecorder.h` |

Tests: `tests/ui/note_v2_test.py` (36 checks). See [testing.md](../testing.md).
