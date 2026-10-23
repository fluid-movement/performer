# Scales

The scale system turns the note values stored in sequences into pitch. **Scale and root note are
project-global** (`Project::scale`, `Project::rootNote`); there is no per-sequence or per-track
scale. Note, Stochastic, Arp and Quantizer tracks all read `Project::selectedScale()`.

Decisions (`prep knowledge find scale`): there is no chromatic scale; step notes are scale degrees,
not semitones; changing the scale never remaps stored notes; scale indices are stored raw, so
reordering the table is a breaking change.

## Key source files

| Role | Path |
|---|---|
| `Scale` base class, `NoteScale`, `VoltScale`, `Scale::get` | `model/Scale.h` |
| The built-in table | `model/Scale.cpp` |
| User scales | `model/UserScale.{h,cpp}` |
| Project scale and root note | `model/Project.h` (`setScale`, `setRootNote`, `selectedScale`) |
| User scale page | `ui/pages/UserScalePage.{h,cpp}`, `ui/model/UserScaleListModel.h` |
| Tests | `src/tests/unit/sequencer/TestScale.cpp` |

## Model

`Scale` is an abstract class: `noteToVolts(note)`, `noteFromVolts(volts)`, `notesPerOctave()`,
`isChromatic()`, `noteName(...)`, `isNotePresent`, `noteIndex`.

- **Built-in scales** are `NoteScale`s: 21 seven-degree modes (7 major modes `Maj:`, 7 harmonic
  minor modes `HM:`, 7 melodic minor modes `MM:`). Index 0 is `Maj: Ionian`. Each degree is stored
  in 1/128 semitone units; `noteToVolts(note) = octave + notes[index] / 1536` where `octave` and
  `index` come from dividing `note` by the note count (so degree 7 is the next octave, 1.0 V up).
  `isChromatic()` is true for them, meaning the root note is added (`rootNote / 12` V) by the engines.
- **User scales** (`UserScale`, 4 of them in `UserScale::userScales`, a static array shared by the
  project, serialized with the project) have a mode, `Chromatic` (items are semitones 0-11 that are
  in the scale; the root note applies) or `Voltage` (items are millivolts, -5000..5000), a size (up to
  `CONFIG_USER_SCALE_SIZE` = 32), items and a name. `Scale::get(index)` returns the built-ins first, then the user scales, so
  `Scale::Count` is 21 + 4 and a project's scale index may point at a user scale.
- `VoltScale` (equal voltage steps) exists in `model/Scale.h` but is not in the built-in table.

`step.note()` is a degree in the current scale. Degree 1 of Ionian is 2 semitones. Use
`noteToVolts` / `noteFromVolts` to convert, never `isNotePresent` on a degree.

## Routing and UI

`Routing::Target::Scale` and `RootNote` can be routed. The project page edits scale and root note and
previews the scale as you scroll. `UserScalePage` (Page + Track 4): footer **USER1-4** picks the slot;
rows Name, Mode, Size, then the items; context menu INIT, COPY, PASTE, LOAD, SAVE (user scales are
also files, see [file-management](file-management.md)).

## Change guide

| Task | Where |
|---|---|
| Add a built-in scale | the `NOTE_SCALE(...)` table and `scales[]` in `model/Scale.cpp` (append; never reorder) |
| Change scale to voltage conversion | `NoteScale::noteToVolts` / `noteFromVolts` in `model/Scale.h` |
| User scale size or count | `CONFIG_USER_SCALE_SIZE` / `CONFIG_USER_SCALE_COUNT` in `Config.h` |
| Fix pitch for one scale | its entry in `model/Scale.cpp` |
