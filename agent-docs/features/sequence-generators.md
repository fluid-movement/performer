# Sequence Generators

Algorithmic tools that fill the **active layer** of a sequence: clear it (Init Layer), write a
Euclidean pattern, or write random values. They run from the context menu of the Note and Curve
edit pages (GEN), preview live on the sequence, and keep the result or clear the selection to revert.

(These are not the Arp track's euclidean patterns: that track has its own implementation,
`engine/EuclideanGenerator.h`, see [arp-track](arp-track.md).)

## Key source files

| Role | Path |
|---|---|
| Base class and `execute` factory | `engine/generators/Generator.{h,cpp}` |
| Euclidean | `engine/generators/EuclideanGenerator.{h,cpp}` |
| Random | `engine/generators/RandomGenerator.{h,cpp}` |
| Rhythm helpers, rhythm strings | `engine/generators/Rhythm.{h,cpp}`, `engine/generators/RhythmString.h` |
| Layer abstraction over a sequence | `engine/generators/SequenceBuilder.h` |
| Pickers | `ui/pages/GeneratorSelectPage.{h,cpp}`, `ui/pages/GeneratorPage.{h,cpp}`, `ui/model/GeneratorSelectListModel.h` |
| Callers | `ui/pages/NoteSequenceEditPage.cpp`, `ui/pages/CurveSequenceEditPage.cpp` (`generateSequence`) |

## Design

- `Generator::Mode`: `InitLayer`, `Euclidean`, `Random`. `Generator::execute(mode, builder,
  selectedSteps)` creates the generator in a static `Container` (or, for InitLayer, just clears the
  layer and returns nothing).
- A `Generator` has `paramCount` / `paramName` / `editParam` / `printParam`, `update()` (recompute
  into the builder) and `revert()` (restore the original).
  - Euclidean params: Steps, Beats, Offset (defaults 16, 4, 0).
  - Random params: Seed, Smooth, Bias, Scale (the selected steps only).
  - Parameters persist between uses in static `Params` structs.
- `SequenceBuilder` is the layer-agnostic interface the generators write through: `length`,
  `value(i)` / `setValue(i, v)` (a float within the layer's range), `originalValue`, `clearSteps`,
  `copyStep`, `clearLayer`, `revert`. `SequenceBuilderImpl<T>` adapts a concrete sequence and a
  `Layer`; typedefs exist for Note, Curve, Stochastic and Arp sequences (only Note and Curve edit
  pages call the generator).

## UI flow

1. Context menu, GEN: `GeneratorSelectPage` lists the modes.
2. `GeneratorPage` shows the generator's parameters as the footer. Hold F1-F5 and turn the encoder
   (pressed = coarse) to edit that parameter; every edit calls `update()` and the result shows live on
   the sequence. If no step was selected, all steps are used.
3. Shift + a step button re-runs `update()` (a new random roll); double-tapping Shift toggles between
   all steps and none (clearing reverts to the original values); Left/Right change the section.
   Leaving the page keeps what is on the sequence.

## Change guide

| Task | Where |
|---|---|
| Add a generator | a class deriving `Generator`, a `Mode` entry and name, a case in `Generator::execute` (and the `Container` type list), the select list is derived from `Mode` |
| Fix the Euclidean distribution | `EuclideanGenerator.cpp`, `Rhythm.cpp` |
| Support another layer type or sequence | `SequenceBuilderImpl<T>` needs `T::Layer`, `layerRange`, `layerDefaultValue`; add the call in that edit page |
| Preview and revert | `Generator::revert`, `GeneratorPage.cpp` |
