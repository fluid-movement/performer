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
- `_muteMode` — what CV value to output when muted: LastValue | Zero (0V) | Min | Max
- `_shapeCurve` — steepness of the exponential at the low end of SHPE: Gentle | Medium | Snappy (Version51, default Medium)
- `_range` — output voltage range, **unipolar 0V..max** in 1V steps, 1V–5V (Version52, default 5V). Stored as `Types::VoltageRange` but clamped to the `Unipolar1V…Unipolar5V` entries; a bipolar value maps to the unipolar entry of the same voltage. `editRange` clamps as an `int` before converting — `VoltageRange` is `uint8_t` backed, so a negative would otherwise wrap to 255 and land on 5V.
- `_slideTime` — CV slew (routable)
- `_offset` — CV offset (routable via `Routing::Target::Offset`)
- `_rotate` — step rotation (routable)
- `_sequences` — array of 17 `CurveSequence` (16 patterns + 1 snapshot)

**Removed in Version52:** `_curveMin` / `_curveMax` and `_shapeProbabilityBias` /
`_gateProbabilityBias`, along with the `CurveMin` (39), `CurveMax` (40) and
`ShapeProbabilityBias` (22) routing targets. Nothing in the V1 engine ever read
them — they were leftovers from the pre-V1 per-step min/max model.
`GateProbabilityBias` (9) remains: Note, Arp and Stochastic tracks each own one.
Routes serialize targets by stable id, and `readEnum` falls back to
`Target::None` for an id it no longer recognizes, so old routes degrade
gracefully with no migration.

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
| Shape | SHPE | `_data0.shape` (7 bits) | 0–127 | exponential fall (0) → half sine (50) → flat hold (100) |
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

### Curve math — `CurveSequence::evalSegment(phase, shape, skew, expCurve)`

Skew warps `phase` so the peak lands at `phase == skew`. The contour is then built
around that peak using `u`, the normalized distance from it (0 at the peak, 1 at
the segment edges); `cos(u·π/2)` is identical to `sin(t·π)`.

- **shape 0–50** — blends an exponential fall into the half sine:
  `m·sine + (1-m)·(exp(-k·u) - e^-k)/(1 - e^-k)` with `m = shape·2` and
  `k = (1-m)·expCurve`. The normalization puts the fall at exactly 1.0 at the peak
  and 0.0 at the edges. Unlike any power of a sine — all of which have zero slope
  at the peak, and so can only make a rounded top — the exponential has a **cusp**
  there. That is what makes a sharp percussive envelope possible.
- **shape 50–100** — unchanged: `sine^p` with `p = (1 - (shape-0.5)·2)²`, widening
  the bump until the `p <= 0` guard returns a constant 1.0 (flat hold).

`expCurve` is the exponential steepness at shape 0, supplied by the track's
**Shape Curve** setting (`CurveTrack::shapeCurveExponent()`): Gentle 4, Medium 6,
Snappy 8. It has no effect at shape 50 and above.

The skew endpoints are handled exactly rather than clamped to an epsilon:
- **skew 0** — the segment starts at full amplitude and decays: a completely
  sharp attack, with SHPE choosing the decay contour.
- **skew 100** — the segment rises across its whole length and is cut off at the
  end: a sharp release.
- **skew 50 with a low SHPE** — a symmetric cusped spike, since SHPE shapes the
  rise and the fall equally.

`evalSegment` is duplicated in the design sandbox
(`agent-docs/ui-redesign/sandbox/src/routes/+page.svelte`) and the two must stay
identical.

## Engine Logic (`CurveTrackEngine`)

1. On each clock tick: determine the current segment via `SequenceState`.
2. Compute `fraction = (currentPulse + intra) / length`, clamped to 0..1 — phase 0
   is genuinely hit at the segment boundary, so a skew-0 edge reaches the DAC.
3. `amp = evalSegment(fraction, shapeNorm(), skewNorm(), _curveTrack.shapeCurveExponent())`.
4. `value = clamp(offsetNorm() + levelNorm() * amp, 0, 1)`, denormalized into the
   track's voltage `range` — **unipolar 0V…max**, so a segment idles at 0V.
5. Apply `slideTime` slew if non-zero, then the track `offset`.
6. Write the final CV value to `CvOutput` for this track's channel.
7. A short gate fires at each **segment boundary** — note this is unaffected by
   mirroring, so a mirrored sharp-attack segment has its transient at the end of
   the segment while the gate still marks the start.

### Mirrored playback

Travelling backwards plays each segment **mirrored**. The curve is a pure function
of phase, so the time reverse is just `1 - phase` — every curve has a mirrored twin
for free, with nothing extra stored:

```cpp
float phase = _sequenceState.direction() < 0 ? 1.f - fraction : fraction;
```

| Run mode | Behaviour |
|---|---|
| Forward | never mirrored |
| Backward | always mirrored, including the wrap from first back round to last |
| Pendulum | mirrored on the descending leg; the repeated endpoint plays forward once, then mirrored once |
| PingPong | mirrored on the descending leg |
| RandomWalk | mirrored whenever the walk steps back one |
| Random | never mirrored — it jumps arbitrary distances, so there is no direction of travel |

This relies on `SequenceState::direction()`, which until Version53 had **no callers
at all** and was only half maintained: PingPong, Pendulum and RandomWalk tracked it,
while Forward, Backward and Random never touched it, so Backward reported `+1` and a
stale `-1` could survive a run-mode change. Both free and aligned advance now set it
in every mode.

`_segmentFraction` is assigned the *mirrored* phase, so the play scanline on the
STEPS and Overview pages sweeps right-to-left when running backwards and sits on the
point of the drawn curve actually reaching the jack. The drawn curve itself is never
mirrored — it is the segment's definition, not a picture of the current traversal.

**Output range.** The hardware ceiling is ±5V, not 10V: `Calibration::CvOutput`
spans `MinVoltage = -5` to `MaxVoltage = 5` and `voltsToValue()` hard-clamps
there (the ideal DAC/opamp stage gives 5.17V / −5.25V). So `Range` offers 1V–5V.
Bipolar output is still reachable through the track `Offset` (±5.00V): Range
0..5V with Offset −2.50V gives −2.5V…+2.5V.

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

`src/apps/sequencer/tests/ui/curve_encoder_skew_test.py` — mirrored playback per run
mode, percent round trip,
1-unit-per-detent encoder behaviour, SHIFT coarse steps, multi-segment editing,
skew endpoints, the exponential low end of SHPE (cusp, convexity, continuity
across the midpoint), the Shape Curve setting on the DAC, and the
Version49 → Version50 → Version51 migrations.
