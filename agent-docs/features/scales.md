# Scales & User Scales

## Purpose
Musical scale system used by note-generating track types (NoteTrack, StochasticTrack, ArpTrack). Each sequence has a `scale` and `rootNote` setting that maps integer step note values to actual chromatic pitch offsets. Users can define up to 4 custom scales stored in the project.

## Key Source Files

| Role | Path |
|---|---|
| Built-in scales | `src/apps/sequencer/model/Scale.h/cpp` |
| User-defined scales | `src/apps/sequencer/model/UserScale.h/cpp` |
| Scale used in sequences | `src/apps/sequencer/model/NoteSequence.h` (`_scale`, `_rootNote` fields) |
| User scale page | `src/apps/sequencer/ui/pages/UserScalePage.h/cpp` |
| UI list model | `src/apps/sequencer/ui/model/UserScaleListModel.h` |

## Data Model

### `Scale` (built-in, read-only)
Defines a chromatic interval pattern:
- `_name` — display name (e.g. "Major", "Dorian", "Pentatonic Minor")
- `_octave` — number of semitones per octave (always 12 for standard scales)
- `_notes` — array of semitone offsets for each degree (e.g. `Maj: Ionian` = `{0,2,4,5,7,9,11}`)
- `_count` — number of degrees in the scale

`Scale` provides:
- `noteToVolts(int degree, int rootNote)` — converts a scale degree + root to a voltage value (1V/octave, 0V = C4)
- `isChromatic()` — true for the chromatic scale (all 12 semitones, no filtering)

Built-in scales are defined as a static array in `Scale.cpp`. As of the 2026-05 modal refactor, index 0 is `Maj: Ionian` (the chromatic/Semitones scale was removed). **Scale indices are stored raw in project files — reordering is a breaking change.** The 2026-05 refactor was a deliberate clean break; old projects will load with shifted scale indices.

Changing a sequence's scale does **not** remap step notes — `step.note()` is preserved as a raw degree+octave integer. The same degree pattern produces different pitches under different scales; this is intentional and supports scale-sweeping as a sound-design move.

### `UserScale` (user-defined, up to 4)
Same structure as `Scale` but user-editable:
- Up to `CONFIG_USER_SCALE_SIZE = 32` note entries
- Stored in `Project::_userScales[4]`
- Serialized/deserialized with the project

### Scale selection in sequences
`NoteSequence`, `StochasticSequence`, `ArpSequence` each have:
- `_scale` — index into combined list: 0–N = built-in scales; N+1..N+4 = user scales
- `_rootNote` — chromatic root (0=C, 1=C#, ..., 11=B)

Negative `_scale` index typically means "use track default" or "inherit from project".

Also routable: `Routing::Target::Scale` and `Routing::Target::RootNote` can be externally controlled.

### Step `BypassScale` flag
In `NoteSequence::Step`, the `BypassScale` layer flag causes the step's note value to be treated as a raw chromatic note (direct MIDI note number) instead of a scale degree. Useful for chromatic passages within a diatonic sequence.

## Constants
```cpp
CONFIG_USER_SCALE_COUNT  4    // max user-defined scales
CONFIG_USER_SCALE_SIZE   32   // max notes per user scale
```

## UI Page (`UserScalePage`)

Allows editing user scale note entries:
- Select one of 4 user scale slots
- Add/remove/edit semitone offsets per degree
- Name the scale

## Change Guide

| Task | Where to look |
|---|---|
| Add a new built-in scale | `Scale.cpp` — add entry to the static scale array |
| Change scale-to-voltage conversion | `Scale.cpp` `noteToVolts()` |
| Change user scale storage size | `Config.h` `CONFIG_USER_SCALE_SIZE` + `UserScale.h` array |
| Add a new user scale slot | `Config.h` `CONFIG_USER_SCALE_COUNT` + `Project.h` `_userScales` array |
| Fix wrong pitch output for a scale | `Scale.cpp` interval array for that scale + `NoteTrackEngine.cpp` scale lookup call |
| Make scale routable | Already routable via `Routing::Target::Scale` — see `RoutingEngine.cpp` |
