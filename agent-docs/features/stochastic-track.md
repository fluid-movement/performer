# Stochastic Track

A generative track modelled on the SIG (Stochastic Inspiration Generator) module. There are no
per-step values: each sequence holds three global **probability distributions** (scale degree,
octave, duration) that are sampled on every step, plus a Turing Machine style **loop** that replays
and mutates what was drawn. Current model: **Version45** (the old per-step model was removed in
Version44 with no migration; old data is skipped on load).

Decisions behind this design (`prep knowledge find stochastic`): SIG-inspired distributions;
Turing-machine loop with its own mutation RNG; hardware bar ghosting (a finding about the
SSD1322 display, relevant to anyone drawing wide adjacent bars).

## Key source files

| Role | Path |
|---|---|
| Track settings | `model/StochasticTrack.{h,cpp}` |
| Sequence: distributions, loop | `model/StochasticSequence.{h,cpp}` |
| Engine | `engine/StochasticEngine.{h,cpp}` |
| Overview page (config-style list) | `ui/pages/StochasticSequencePage.{h,cpp}` |
| Edit page (bars, tabs) | `ui/pages/StochasticSequenceEditPage.{h,cpp}` |
| Config list rows | `ui/model/StochasticTrackListModel.h`, `ui/model/StochasticSequenceListModel.h` |
| Tests | `tests/ui/stochastic_v2_test.py` |

## Data model

### `StochasticSequence` (per pattern)

| Field | Meaning |
|---|---|
| `degreeProb[7]` | selection weight of each scale degree, 0-15 each |
| `octaveProb[5]` | weight of octave offset -2..+2, 0-15 each |
| `durationProb[6]` | weight of step duration 1/16, 1/8, 1/4, 1/2, 1, 2, 0-15 each |
| `sequenceFirstStep` / `sequenceLastStep` | the buffer window; `bufferLoopLength = last - first + 1` (default 0..15 = 16) |
| `loopLength` | steps replayed per loop cycle (1-64, default 64), stored since Version45 |
| `loopChance` | 0 = locked loop, 15 = every replayed slot re-drawn (default 15) |
| `useLoop` | loop on/off (default on); `clearLoop` is a one-shot request |
| `reseed` | routable (`Routing::Target::Reseed`); re-seeds the RNG when not looping |
| `divisor` (routable), `resetMeasure`, `runMode` | as other tracks |

Defaults: degree 1 weight 8 (others 0), octave 0 weight 15, duration 1/4 weight 15 and 1/2 weight 8.

### `StochasticTrack`

`playMode` (only `Aligned` does anything; `Free` and `Last` are unimplemented), `fillMode` (None,
Gates, NextPattern, Condition), `fillMuted`, `cvUpdateMode` (Gate / Always), `slideTime`, `octave`,
`transpose`, `rotate` (stored; the engine does not read it), and four probability biases that still
appear on the config page but are not stored and not read. Slide time is passed to the CV drain,
but the engine never sets the slide flag, so it has no audible effect (a known gap).

## Engine (`StochasticEngine`)

On each divisor boundary `triggerStep` runs one of two phases.

**Phase 1: free-running / filling the buffer** (loop off, or buffer not yet full)
1. Density: with `n` non-zero degree weights summing to `sum`, the step fires when
   `rand(n*15) < sum`. One bar at 15 is always 100%; adding or raising bars changes density. If all
   degree weights are 0 the track is silent (covered by `test_all_degrees_zero_no_gate`).
2. Draw a degree, an octave and a duration (if all octave or duration weights are 0, the centre
   octave and quarter-note duration are used). Duration ticks = `(divisor/4) * {1,2,4,8,16,32}[idx]`.
3. Note = `scale.noteToVolts(degree + (trackOctave + octaveOffset) * notesPerOctave + transpose)`,
   plus the root note for chromatic scales. Gate on at the swung step tick, off after the duration.
4. While looping, each drawn step is appended to `_lockedSteps` until `bufferLoopLength` is reached.

**Phase 2: loop replay** (loop on and buffer full)
1. `_index = (absoluteStep - _loopPhaseOffset) % loopLength`; the slot is
   `(sequenceFirstStep + _index) % buffer size`, so `firstStep` shifts the loop's phase without
   changing its length. Changing `loopLength` re-anchors the phase.
2. With probability `loopChance/15` (separate `mutationRng`) the slot is re-drawn from the current
   distributions and written back to the buffer.
3. Fill: `Gates` forces gates, `NextPattern` evaluates the distributions of pattern + 1.

`Clear loop` empties the buffer on the next index 0 and turns the loop off. The RNG is re-seeded
from the clock on `reset()` and `restart()`.

## UI

`StochasticSequenceEditPage`, footer tabs **NOTE / OCT / LEN / LOOP**.

| Tab | Display | Edit |
|---|---|---|
| NOTE | 7 vertical bars, degree labels 1-7, cursor = inverted column, playing degree outlined | hold Step 0-6 and turn the encoder; Shift = x4 |
| OCT | 5 bars, labels -2..+2 | same, Step 0-4 |
| LEN | 6 bars, labels 1/16..2 | same, Step 0-5 |
| LOOP | loop length, chance (`CHC`, shown as percent) | Step n sets the loop length to n+1; encoder sets loop chance |

Bars are 18 px wide centred in 36 px column slots and the full slot is cleared every frame, to
limit display column coupling (see the ghosting finding).

| Input | Effect |
|---|---|
| Page + Step 6 | loop on/off (toast "Loop on/off") |
| Page + Step 5 | clear loop (toast "Loop cleared") |
| Page + Step 8-12 | quick edit FirstStep, LastStep, RunMode, Divisor, ResetMeasure |
| Context menu | INIT, COPY, PASTE (edit page); INIT, COPY, PASTE, DUPL, ROUTE (overview page) |

There is no MIDI step recording on this track.

## Change guide

| Task | Where |
|---|---|
| Change how a step is drawn | `StochasticEngine.cpp`: `drawDegree`, `drawOctave`, `drawDuration`, `triggerStep` |
| Change loop or mutation | `StochasticEngine::triggerStep` phase 2 and `StochasticSequence` loop fields |
| Add a distribution | arrays in `StochasticSequence.h`, `write`/`read` (new `ProjectVersion`), a tab in `StochasticSequenceEditPage.cpp`, python binding in `python/project.cpp` |
| Implement play mode `Free` | the empty `case Types::PlayMode::Free` in `StochasticEngine::tick` |
| Wire slide or the biases | engine currently ignores them; `drainCvQueue` already takes `slideTime` |
| Reseed behaviour | `StochasticEngine::triggerStep` reseed block and `Routing::Target::Reseed` |

Tests: `tests/ui/stochastic_v2_test.py` (see [testing.md](../testing.md)).
