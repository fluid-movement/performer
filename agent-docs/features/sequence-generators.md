# Sequence Generators

## Purpose
Algorithmic tools that fill a sequence with generated step data. Accessed via the generator page, they can produce Euclidean rhythms, random patterns, or other algorithmic sequences and write the result into the active sequence. Non-destructive preview before committing.

## Key Source Files

| Role | Path |
|---|---|
| Base generator interface | `src/apps/sequencer/engine/generators/Generator.h/cpp` |
| Euclidean generator | `src/apps/sequencer/engine/generators/EuclideanGenerator.h/cpp` |
| Random generator | `src/apps/sequencer/engine/generators/RandomGenerator.h/cpp` |
| Rhythm utilities | `src/apps/sequencer/engine/generators/Rhythm.h/cpp` |
| Rhythm string parser | `src/apps/sequencer/engine/generators/RhythmString.h` |
| Sequence builder helper | `src/apps/sequencer/engine/generators/SequenceBuilder.h` |
| Generator select page | `src/apps/sequencer/ui/pages/GeneratorSelectPage.h/cpp` |
| Generator page | `src/apps/sequencer/ui/pages/GeneratorPage.h/cpp` |

## Architecture

### `Generator` (base class)
Abstract interface with:
- `name()` — display name
- `edit(layer, value, shift)` — modify generator parameter
- `print(layer, str)` — format parameter for display
- `generate(SequenceBuilder &)` — produce output by calling `SequenceBuilder` methods
- `revert()` — restore original sequence before generation

### `EuclideanGenerator`
Generates a Euclidean rhythm (Bjorklund algorithm):
- Parameters: `Steps` (total steps), `Pulses` (active steps), `Offset` (rotation)
- Sets gate on/off per step according to the Euclidean distribution
- Uses `Rhythm.h` for the actual Bjorklund computation

### `RandomGenerator`
Fills steps with random gate/note values:
- Parameters: `Steps`, `GateProbability`, `NoteMin`, `NoteMax`, `NoteScale`

### `SequenceBuilder`
A helper passed to `Generator::generate()`. Abstracts writing to the underlying sequence type (NoteSequence, CurveSequence, etc.):
- `setGate(step, bool)` — set gate on a step
- `setNote(step, int)` — set note on a step
- `setLength(step, int)` — set length on a step

### `RhythmString`
Parses a string like `"x.x.x..x"` into a gate pattern for generator input.

## UI Flow

1. User navigates to `GeneratorSelectPage` — chooses Euclidean or Random.
2. `GeneratorPage` opens, showing current generator parameters.
3. User adjusts parameters; preview is applied to the sequence in real time.
4. Confirm: generator writes final values. Cancel: `revert()` restores original.

## Change Guide

| Task | Where to look |
|---|---|
| Add a new generator | Create `XxxGenerator.h/cpp` extending `Generator`, register in `GeneratorSelectPage.cpp` |
| Fix Euclidean distribution | `EuclideanGenerator.cpp` + `Rhythm.cpp` Bjorklund algorithm |
| Add a new generatable parameter | `SequenceBuilder.h` new setter + generator implementations |
| Fix generator preview/revert | `Generator.cpp` `revert()` + `GeneratorPage.cpp` |
