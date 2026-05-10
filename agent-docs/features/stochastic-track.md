# Stochastic Track

## Purpose
A probability-driven sequencer track where note/gate decisions are made at runtime using random processes. Each step defines a set of possible notes and probabilities rather than a fixed note. Supports reseeding for reproducible randomness.

## Key Source Files

| Role | Path |
|---|---|
| Data model (track settings) | `src/apps/sequencer/model/StochasticTrack.h/cpp` |
| Data model (sequence + steps) | `src/apps/sequencer/model/StochasticSequence.h/cpp` |
| Real-time engine | `src/apps/sequencer/engine/StochasticEngine.h/cpp` |
| Sequence display page | `src/apps/sequencer/ui/pages/StochasticSequencePage.h/cpp` |
| Sequence edit page | `src/apps/sequencer/ui/pages/StochasticSequenceEditPage.h/cpp` |
| UI list model | `src/apps/sequencer/ui/model/StochasticTrackListModel.h` |

## Data Model

### `StochasticTrack` (track-level settings)
- `_playMode` — `Types::PlayMode`
- `_fillMode` — `FillMode` enum: None | Gates | NextPattern | Condition
- `_cvUpdateMode` — `CvUpdateMode`: Gate | Always
- `_slideTime` — portamento time (routable)
- `_octave` — octave offset (routable)
- `_transpose` — transpose (routable)
- `_rotate` — step rotation (routable)
- `_gateProbabilityBias` — global gate probability offset (routable)
- `_retriggerProbabilityBias` — global retrigger probability offset (routable)
- `_lengthBias` — global length offset (routable)
- `_noteProbabilityBias` — global note variation bias (routable)
- `_sequences` — array of 17 `StochasticSequence`

### `StochasticSequence` (per-pattern data)
- `_seed` — RNG seed for reproducible sequences; `Reseed` routing target triggers re-randomization
- `_restProbability` — probability of rest (step skipped entirely)
- `_scale`, `_rootNote`, `_divisor`, `_runMode`, `_firstStep`, `_lastStep` — same semantics as NoteSequence

### `StochasticSequence::Step` layers
Similar to NoteSequence but weighted toward probabilistic choices:
| Layer | Description |
|---|---|
| Gate | Whether this step can trigger |
| GateProbability | Per-step gate probability |
| Retrigger / RetriggerProbability | Same as NoteTrack |
| Length / LengthVariationRange / LengthVariationProbability | Same as NoteTrack |
| Note | Base note (scale degree) |
| NoteVariationRange / NoteVariationProbability | Random note variation |
| Condition | Conditional trigger |
| RestProbability2 / RestProbability4 | Additional rest probability tiers |

## Engine Logic (`StochasticEngine`)

1. At startup and on `Reseed` trigger: initialize `Random` RNG with `_seed`.
2. On each clock tick: evaluate the current step.
3. Apply `restProbability` — if rest fires, skip the step entirely.
4. Apply `gateProbabilityBias` + step gate probability.
5. Pick note using `noteVariationRange`/`noteVariationProbability`.
6. Handle retrigger, length variation same as NoteTrackEngine.
7. Advance step via `SequenceState`.
8. Output CV + gate.

The key differentiator from NoteTrack: randomness is seeded and reproducible per sequence, and rest probability adds an additional layer of silence before individual gate probabilities.

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `StochasticSequencePage` | `ui/pages/StochasticSequencePage.h/cpp` | Step overview |
| `StochasticSequenceEditPage` | `ui/pages/StochasticSequenceEditPage.h/cpp` | Per-step editing |

## Change Guide

| Task | Where to look |
|---|---|
| Change reseed behavior | `StochasticEngine.cpp` reseed handler + `Routing.h` `Target::Reseed` |
| Add new rest probability tiers | `StochasticSequence.h` + `StochasticEngine.cpp` |
| Fix random number generation | `src/core/utils/Random.h` + `StochasticEngine.cpp` seed usage |
| Change per-step probability logic | `StochasticEngine.cpp` step evaluation section |
