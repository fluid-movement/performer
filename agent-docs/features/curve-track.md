# Curve Track

## Purpose
A CV automation track. Instead of note+gate steps it outputs smoothly interpolated CV values. No gate output — purely a CV modulation source useful for LFO-like curves, envelopes, or any continuously varying CV.

## Key Source Files

| Role | Path |
|---|---|
| Data model (track settings) | `src/apps/sequencer/model/CurveTrack.h/cpp` |
| Data model (sequence + steps) | `src/apps/sequencer/model/CurveSequence.h/cpp` |
| Real-time engine | `src/apps/sequencer/engine/CurveTrackEngine.h/cpp` |
| Curve recorder | `src/apps/sequencer/engine/CurveRecorder.h/cpp` |
| Sequence display page | `src/apps/sequencer/ui/pages/CurveSequencePage.h/cpp` |
| Sequence edit page | `src/apps/sequencer/ui/pages/CurveSequenceEditPage.h/cpp` |
| UI list model | `src/apps/sequencer/ui/model/CurveTrackListModel.h` |

## Data Model

### `CurveTrack` (track-level settings)
- `_playMode` — `Types::PlayMode`
- `_fillMode` — `FillMode` enum: None | Variation | NextPattern | Invert
- `_muteMode` — what CV value to output when muted: LastValue | Zero (0V) | Min | Max
- `_offset` — CV offset (routable via `Routing::Target::Offset`)
- `_rotate` — step rotation (routable)
- `_shapeProbabilityBias` — global shape variation bias (routable via `ShapeProbabilityBias`)
- `_curveMin`, `_curveMax` — output CV range min/max (routable)
- `_sequences` — array of 17 `CurveSequence` (16 patterns + 1 snapshot)

### `CurveSequence` (per-pattern data)
- `_scale` — output scale (maps step values to voltage range)
- `_divisor` — clock divisor
- `_resetMeasure` — measure-length reset period
- `_runMode` — `Types::RunMode`
- `_firstStep`, `_lastStep` — active range (0–63)
- `_range` — CV output range (e.g. ±1V, ±2V, ±5V, 0–10V)
- `_steps` — array of 64 `Step`

### `CurveSequence::Step` layers
| Layer | Description |
|---|---|
| Shape | Curve shape (linear, exponential, logarithmic, sine, etc.) |
| Min | Minimum CV value for this step |
| Max | Maximum CV value for this step |
| ShapeVariation | Alternative shape applied probabilistically |
| ShapeVariationProbability | Probability of shape variation |

## Engine Logic (`CurveTrackEngine`)

1. On each clock tick: determine current step position via `SequenceState`.
2. Compute interpolation position within the current step (fractional progress 0.0–1.0).
3. Apply the step's `Shape` curve function to the interpolation position.
4. Interpolate between `Min` and `Max` for the step.
5. Apply `shapeProbabilityBias` to potentially substitute the variation shape.
6. Apply `offset` and clamp to `curveMin`/`curveMax`.
7. Write final CV value to `CvOutput` for this track's channel.
8. No gate output — this track does not drive `GateOutput`.

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `CurveSequencePage` | `ui/pages/CurveSequencePage.h/cpp` | Overview display of curve steps |
| `CurveSequenceEditPage` | `ui/pages/CurveSequenceEditPage.h/cpp` | Per-step shape/min/max editing |

## Change Guide

| Task | Where to look |
|---|---|
| Add a new curve shape | `src/apps/sequencer/model/CurveSequence.h` — shape enum; `CurveTrackEngine.cpp` — shape evaluation |
| Change mute behavior | `CurveTrack.h` `MuteMode` enum + `CurveTrackEngine.cpp` mute handling |
| Add a new fill mode | `CurveTrack.h` `FillMode` enum + `CurveTrackEngine.cpp` |
| Fix CV range/scaling | `CurveTrackEngine.cpp` output scaling + `CurveSequence.h` `_range` field |
| Fix curve recording | `src/apps/sequencer/engine/CurveRecorder.h/cpp` |
