# Curve Track

A CV shape generator. Steps are **segments** of variable length that chain and loop; each segment
draws one bump from an offset up to offset + level and back, shaped by `shape` and `skew`. Output
is **unipolar 0 V to the track's Range**, plus a short gate at each segment start. Current model:
**Version53**.

Decisions behind this design (`prep knowledge find curve`): curve track is a segment assembler;
parameters edit in a 0-100 percent domain; SHPE has an exponential fall at the low end and a flat
hold at the top; output is unipolar with Range 1-5 V; segments play mirrored when travelling
backwards; no play mode, fill mode, min/max or probability biases; the CV hardware ceiling is
5 V; `evalSegment` is duplicated in the UI sandbox.

## Key source files

| Role | Path |
|---|---|
| Track settings | `model/CurveTrack.{h,cpp}` |
| Sequence, segments, `evalSegment` | `model/CurveSequence.{h,cpp}` |
| Engine | `engine/CurveTrackEngine.{h,cpp}` |
| Recorder (declared, not wired in V1) | `engine/CurveRecorder.h` |
| Overview page | `ui/pages/CurveSequencePage.{h,cpp}` |
| Edit page | `ui/pages/CurveSequenceEditPage.{h,cpp}` |
| Same encoder logic, duplicated | `ui/pages/OverviewPage.cpp` |
| Config list rows | `ui/model/CurveTrackListModel.h`, `ui/model/CurveSequenceListModel.h` |
| Sandbox copy of the curve math | `tools/ui-sandbox/src/routes/+page.svelte` |
| Tests | `tests/ui/curve_encoder_skew_test.py` |

## Data model

### `CurveTrack`

- `muteMode`: what the CV does while muted: LastValue, Zero, Min, Max
- `shapeCurve`: steepness of the exponential at SHPE 0: Gentle 4, Medium 6 (default), Snappy 8 (Version51)
- `range`: output range, unipolar 0 V..max in 1 V steps, 1-5 V, default 5 V (Version52). Stored as
  `Types::VoltageRange`; bipolar values map to the unipolar entry of the same voltage; `editRange`
  clamps as `int` first because the enum is `uint8_t` backed
- `slideTime` (CV slew, routable), `offset` (volts, routable via `Routing::Target::Offset`), `rotate`
- `curveCvInput` (Off, Cv1-4): stored and editable, but nothing in the engine reads it
- `sequences`: 17 `CurveSequence`; pattern follow and name from the base classes

### `CurveSequence`

`divisor` (routable), `resetMeasure`, `runMode` (routable), `firstStep`, `lastStep` (routable),
64 `Step`s of which the first `segmentCount` (Version42) are
active, and the displayed `section`. `segmentCount` is 1-16 (default 8). The loop covers segments `0..segmentCount-1`; `firstStep`
rotates which segment starts.

### `CurveSequence::Step` layers

| Layer | Tab | Stored in | Raw range | Meaning |
|---|---|---|---|---|
| Shape | SHPE | `_data0.shape` (7 bits) | 0-127 | exponential fall (0), half sine (50), flat hold (100) |
| Skew | SKEW | `_data0.shapeVariation` (7 bits) | 0-127 | where the peak sits within the segment |
| Length | LEN | `_data1.gate` (4 bits) | 1-16 | segment length in divisor pulses |
| Level | LVL | `_data0.max` (8 bits) | 0-255 | peak height above the offset |
| Offset | OFST | `_data0.min` (8 bits) | 0-255 | value the segment starts and ends at |

The names `min` / `max` / `shapeVariation` survive as bit-field names only. The UI edits and shows
the five values as integers 0-100 (`shapePercent()`, `skewPercent()`, `levelPercent()`,
`offsetPercent()`); one detent is 1, Shift is 10. `shapeNorm()` and friends are the 0.0-1.0 domain
the engine uses.

### Curve math: `CurveSequence::evalSegment(phase, shape, skew, expCurve)`

Skew warps `phase` so the peak lands at `phase == skew`; `u` is the normalized distance from the
peak (0 at the peak, 1 at the edges).

- **shape 0-50**: `m*sine + (1-m)*fall` with `m = shape*2`, `k = (1-m)*expCurve` and
  `fall = (exp(-k*u) - e^-k) / (1 - e^-k)`, a cusp at the peak (percussive envelopes).
- **shape 50-100**: `sine^p`, `p = (1 - (shape-0.5)*2)^2`; `p <= 0` returns 1.0 (flat hold).
- Skew 0 is an instant attack, skew 100 an instant release; endpoints are exact, not clamped.

Any change here must be mirrored in the sandbox copy (`tools/ui-sandbox/src/routes/+page.svelte`).

## Engine (`CurveTrackEngine`)

1. `tick()`: `computeClockParams`, then on every divisor boundary `advancePulse()` moves within the
   current segment or, at its end, `SequenceState::advanceFree` picks the next one and queues the
   gate (length `divisor / 8` ticks, swung). `relativeTick == 0` resets.
2. `updateOutput()` each tick: `fraction = (pulse + intra) / length`, clamped to 0..1;
   `phase = direction() < 0 ? 1 - fraction : fraction`;
   `value = clamp(offsetNorm + levelNorm * evalSegment(phase, ...), 0, 1)`, denormalized into the
   range. Muted tracks write the mute-mode value instead.
3. `update(dt)`: applies `slideTime` with `Slide::applySlide` (a slew limiter) and adds the track
   `offset` (0 when muted). Output is sent to MIDI output as well.

**Mirrored playback**: running backwards plays each segment mirrored. Backward mirrors everything
including the wrap; Pendulum and PingPong mirror the descending leg; RandomWalk mirrors when it
steps back; Random never mirrors. The play scanline uses the mirrored phase; the drawn curve never
does; the gate still fires at the segment start. This relies on `SequenceState::direction()` being
set by every run mode (`prep knowledge find direction`).

**Range**: the hardware ceiling is +-5 V (`Calibration::CvOutput`), so Range offers 1-5 V. Bipolar
output comes from the track `Offset` (Range 5 V with Offset -2.50 V gives -2.5..+2.5 V).

## UI

`CurveSequenceEditPage`: segment numbers row, proportional-width curve, per-tab indicator strip,
play scanline. Footer tabs **SHPE / SKEW / LEN / LVL / OFST**.

| Input | Effect |
|---|---|
| Step buttons | select segments (a selection of only active segments, or only inactive ones, switches the footer to actions) |
| Encoder (with selection) | edit the active tab's value on all selected segments; Shift x10; nothing happens with no selection |
| F1-F5 | switch tab |
| footer actions on active segments | DEL, DUPL, RSET (F1-F3) |
| footer action on inactive segments | ADD (F1) |
| Page + Step 8-11 | quick edit FirstStep, SegmentCount, RunMode, Divisor |
| Page + Step 15 | toggle pattern follow display |

## Change guide

| Task | Where |
|---|---|
| Change the curve math | `CurveSequence::evalSegment`, and the sandbox copy |
| Change encoder granularity | `CurveSequenceEditPage::encoder` and the duplicate in `ui/pages/OverviewPage.cpp`; percent accessors in `model/CurveSequence.h` |
| Widen a step field | `Step::_data0` layout and typedefs, `layerRange`/`layerDefaultValue`, a migration in `Step::read()`, a `ProjectVersion` bump ([serialization](serialization.md)) |
| Change mute behaviour | `CurveTrack::MuteMode` and `CurveTrackEngine::updateOutput` |
| Change range or scaling | `CurveTrack::range`, `Types::voltageRangeInfo` in `model/Types.h`, `CurveTrackEngine::updateOutput` |

Tests: `tests/ui/curve_encoder_skew_test.py` (see [testing.md](../testing.md)).
