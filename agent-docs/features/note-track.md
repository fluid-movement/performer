# Note Track

## Purpose
The most commonly used track type. A classic step sequencer where each step has an independent note, gate, and a rich set of per-step probability parameters. Outputs one CV + gate pair per track.

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
- `_playMode` — `Types::PlayMode` (e.g. Aligned, Free)
- `_fillMode` — `FillMode` enum: None | Gates | NextPattern | Condition
- `_fillMuted` — whether fill mutes the track
- `_cvUpdateMode` — `CvUpdateMode`: Gate (CV only changes on gate) or Always
- `_slideTime` — portamento time (0–100%, routable)
- `_octave` — octave offset (–10 to +10, routable)
- `_transpose` — semitone transpose (–100 to +100, routable)
- `_rotate` — step rotation (–64 to +64, routable)
- `_gateProbabilityBias` — global gate probability offset (routable)
- `_retriggerProbabilityBias` — global retrigger probability offset (routable)
- `_lengthBias` — global note length offset (routable)
- `_noteProbabilityBias` — global note variation probability offset (routable)
- `_logicTrack` — index (0–7) of a LogicTrack that gates this track, or –1 for none
- `_logicTrackInput` — which logic input to use (0 or 1), or –1
- `_sequences` — array of 17 `NoteSequence` (16 patterns + 1 snapshot)

### `NoteSequence` (per-pattern data)
- `_scale` — scale index
- `_rootNote` — root note (0–11)
- `_divisor` — clock divisor (controls playback speed)
- `_resetMeasure` — reset period in measures
- `_runMode` — `Types::RunMode` (Forward, Backward, PingPong, Random, etc.)
- `_firstStep`, `_lastStep` — active step range (0–63)
- `_steps` — array of 64 `Step`

### `NoteSequence::Step` layers (bit-packed per step)
| Layer | Type | Description |
|---|---|---|
| Gate | bool | Whether the step triggers |
| GateProbability | 0–15 | Probability the gate fires |
| GateOffset | –8 to +7 | Sub-step gate timing offset |
| Slide | bool | Legato/portamento to next note |
| BypassScale | bool | Play raw MIDI note, ignore scale |
| Retrigger | 0–7 | Number of retrigggers within the step |
| RetriggerProbability | 0–15 | Probability of retrigger |
| Length | 0–15 | Gate length (fraction of step) |
| LengthVariationRange | –8 to +7 | Random length variation range |
| LengthVariationProbability | 0–15 | Probability of length variation |
| Note | –64 to +63 | Scale degree (or raw MIDI if BypassScale) |
| NoteVariationRange | –64 to +63 | Random note variation range |
| NoteVariationProbability | 0–15 | Probability of note variation |
| Condition | 0–127 | Conditional trigger (see `Types::Condition`) |
| StageRepeats | 0–7 | How many times this step repeats before advancing |
| StageRepeatsMode | 0–7 | How repeats affect gate/note |

## Engine Logic (`NoteTrackEngine`)

1. On each clock tick: check if the current step's gate should fire, applying `gateProbabilityBias` and `Condition`.
2. If gate fires: read note, apply `NoteVariationRange`/`NoteVariationProbability`, apply `octave` + `transpose` + scale mapping.
3. Compute gate length from `Length` + `LengthVariationRange`/`LengthVariationProbability`.
4. Handle `Slide` — uses `Slide.h` to interpolate CV if slide is active.
5. Handle `Retrigger` — subdivides the step into multiple gate pulses.
6. Advance step via `SequenceState` respecting `runMode`, `firstStep`, `lastStep`, `rotate`.
7. If `logicTrack != -1`, gate output is ANDed with the referenced LogicTrack's gate output.
8. Outputs final CV to `CvOutput` and gate to `GateOutput` for this track's channel.

Key class: `src/apps/sequencer/engine/SequenceState.h` — tracks current step position, handles all run modes.

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
| Add a new per-step parameter | Add a `Layer` enum value in `NoteSequence.h`, add bit-field storage in `Step`, update `layerName()`, `layerRange()`, `layerDefaultValue()`, handle in `NoteTrackEngine.cpp`, add rendering in `NoteSequenceEditPage.cpp` |
| Change gate probability behavior | `NoteTrackEngine.cpp` — gate probability logic near step evaluation |
| Add a new fill mode | `NoteTrack.h` `FillMode` enum + `NoteTrackEngine.cpp` fill handling |
| Add a new run mode | `src/apps/sequencer/model/Types.h` `RunMode` enum + `SequenceState.h/cpp` |
| Change slide/portamento | `src/apps/sequencer/engine/Slide.h` + `NoteTrackEngine.cpp` |
| Change how logic track gating works | `NoteTrackEngine.cpp` bottom of step evaluation + `NoteTrack.h` `_logicTrack` field |
| Fix step recording bugs | `src/apps/sequencer/engine/StepRecorder.h/cpp` |
