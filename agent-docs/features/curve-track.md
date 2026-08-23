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

- `_segmentCount` — number of active segments (Version42)

### `CurveSequence::Step` layers — V1 segment assembler

A step is a **segment**, not a fixed-width step. Segments chain and loop; the loop
length is the sum of their lengths. Step buttons select segments.

| Layer | Tab | Stored in | Raw range | Meaning |
|---|---|---|---|---|
| Shape | SHPE | `_data0.shape` (7 bits) | 0–127 | spike (0) → half sine (50) → flat hold (100) |
| Skew | SKEW | `_data0.shapeVariation` (7 bits) | 0–127 | position of the peak within the segment |
| Length | LEN | `_data1.gate` (4 bits) | 1–16 | segment length in pulses |
| Level | LVL | `_data0.max` (8 bits) | 0–255 | peak height above the offset |
| Offset | OFST | `_data0.min` (8 bits) | 0–255 | value the segment starts and ends at |

The legacy `Min` / `Max` / `ShapeVariation` names survive as bitfield names only —
the V1 model reinterprets them as offset / level / skew.

**Editing domain (Version50).** The UI edits and displays these as integers 0–100
via `shapePercent()` / `skewPercent()` / `levelPercent()` / `offsetPercent()`.
One encoder detent moves the value by exactly 1, SHIFT by 10. The percent
round trip is exact in both directions (integer math, raw spacing > 1), which is
what makes stepping symmetric — turning back lands on the original value.
`shapeNorm()` etc. remain the 0.0–1.0 domain used by the engine and the curve math.

Shape and skew were 6-bit before Version50; the migration in `Step::read()`
rescales old values (`raw * 127 / 63`) and moves skew from bit 6 to bit 7.

### Curve math — `CurveSequence::evalSegment(phase, shape, skew)`

Skew warps `phase` so the sine peak lands at `phase == skew`; shape raises the
sine to a power (8 at shape 0, 1 at shape 50, → 0 at shape 100, where the
`p <= 0` guard returns a constant 1.0 = flat hold).

The endpoints are handled exactly rather than clamped to an epsilon:
- **skew 0** — the segment starts at full amplitude and decays: a completely
  sharp attack, with SHPE choosing the decay contour.
- **skew 100** — the segment rises across its whole length and is cut off at the
  end: a sharp release.

`evalSegment` is duplicated in the design sandbox
(`agent-docs/ui-redesign/sandbox/src/routes/+page.svelte`) and the two must stay
identical.

## Engine Logic (`CurveTrackEngine`)

1. On each clock tick: determine the current segment via `SequenceState`.
2. Compute `fraction = (currentPulse + intra) / length`, clamped to 0..1 — phase 0
   is genuinely hit at the segment boundary, so a skew-0 edge reaches the DAC.
3. `amp = evalSegment(fraction, shapeNorm(), skewNorm())`.
4. `value = clamp(offsetNorm() + levelNorm() * amp, 0, 1)`, denormalized into the
   sequence's voltage `range`.
5. Apply `slideTime` slew if non-zero, then the track `offset`.
6. Write the final CV value to `CvOutput` for this track's channel.
7. No gate output — this track does not drive `GateOutput`.

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `CurveSequencePage` | `ui/pages/CurveSequencePage.h/cpp` | Overview display of curve steps |
| `CurveSequenceEditPage` | `ui/pages/CurveSequenceEditPage.h/cpp` | Segment editing: SHPE / SKEW / LEN / LVL / OFST tabs, proportional-width curve display, per-tab indicator strip |

## Change Guide

| Task | Where to look |
|---|---|
| Change the curve math | `CurveSequence::evalSegment` in `model/CurveSequence.h` — and mirror it in the sandbox `+page.svelte` |
| Change encoder granularity | `CurveSequenceEditPage::encoder` and the duplicate in `OverviewPage.cpp`; the percent accessors live in `CurveSequence.h` |
| Widen a step field | `Step::_data0` bitfield layout + `Shape`/`Min`/`Max` typedefs, `layerRange()`/`layerDefaultValue()`, and a migration in `Step::read()` |
| Change mute behavior | `CurveTrack.h` `MuteMode` enum + `CurveTrackEngine.cpp` mute handling |
| Add a new fill mode | `CurveTrack.h` `FillMode` enum + `CurveTrackEngine.cpp` |
| Fix CV range/scaling | `CurveTrackEngine.cpp` output scaling + `CurveSequence.h` `_range` field |
| Fix curve recording | `src/apps/sequencer/engine/CurveRecorder.h/cpp` |

## Tests

`src/apps/sequencer/tests/ui/curve_encoder_skew_test.py` — percent round trip,
1-unit-per-detent encoder behaviour, SHIFT coarse steps, multi-segment editing,
skew endpoints (curve math and DAC output), and the Version49 → Version50 migration.
