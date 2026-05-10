# Patterns & Projects

## Purpose
A Project is the top-level data container. It holds 8 tracks, each with 16 patterns + 1 snapshot, global settings, song data, routing, MIDI output config, and user scales. Patterns are independent sequence states per track — switching patterns changes the active sequence without losing others.

## Key Source Files

| Role | Path |
|---|---|
| Project (top-level container) | `src/apps/sequencer/model/Project.h/cpp` |
| Track (polymorphic container) | `src/apps/sequencer/model/Track.h/cpp` |
| PlayState (runtime state) | `src/apps/sequencer/model/PlayState.h/cpp` |
| Model (wraps Project + Settings) | `src/apps/sequencer/model/Model.h/cpp` |
| File definitions | `src/apps/sequencer/model/FileDefs.h` |
| Project page | `src/apps/sequencer/ui/pages/ProjectPage.h/cpp` |
| Pattern page | `src/apps/sequencer/ui/pages/PatternPage.h/cpp` |
| Overview page | `src/apps/sequencer/ui/pages/OverviewPage.h/cpp` |

## Data Model

### `Project`
Top-level owned object, contains:
- `_name` — project name string (max `FileHeader::NameLength`)
- `_slot` — SD card slot number (which file it was loaded from)
- `_autoLoaded` — whether this project auto-loads on boot
- `_tempo` — BPM (1–1000 float, `Routable<float>`)
- `_swing` — swing amount (0–100, `Routable<uint8_t>`)
- `_timeSignature` — `TimeSignature` (beats per bar)
- `_clockSetup` — `ClockSetup` (clock mode + divisors)
- `_tracks` — `std::array<Track, 8>` (polymorphic track containers)
- `_song` — `Song` (64-slot arrangement)
- `_playState` — `PlayState` (runtime mute/pattern/fill state)
- `_cvOutputTrack` — mapping: which track drives each of the 8 CV outputs
- `_gateOutput` — mapping: which track drives each of the 8 gate outputs
- `_routing` — `Routing` (16 modulation routes)
- `_midiOutput` — array of 16 `MidiOutput` slots
- `_userScales` — array of 4 `UserScale`
- `_selectedTrackIndex`, `_selectedPatternIndex` — UI focus state

### `Track` (polymorphic container)
`Track` is a tagged-union container (`Container<NoteTrack, CurveTrack, MidiCvTrack, StochasticTrack, LogicTrack, ArpTrack>`) managed by `src/core/utils/Container.h`. The active track type is stored as a `TrackMode` enum. Switching mode resets the track data.

Key methods:
- `trackMode()` — current `TrackMode`
- `setTrackMode(TrackMode)` — switches type (clears data)
- `noteTrack()` / `curveTrack()` / etc. — access typed track (unsafe if wrong type)
- `as<T>()` — safe typed access

### `PlayState`
Runtime (not serialized, rebuilt on load):
- `_trackStates[8]` — per-track: current pattern index, mute, fill, pattern-change request
- `requestPatternChange(track, pattern, immediate)` — queues/applies a pattern switch
- `fill(track, active)` — activates fill mode

### `Model`
Wraps `Project` + `Settings`. Passed to `Engine` and `Ui`. Provides `model.project()` and `model.settings()` accessors. File load/save goes through `FileManager` not `Model` directly.

## Constants
```cpp
CONFIG_TRACK_COUNT    8    // tracks per project
CONFIG_PATTERN_COUNT  16   // patterns per track
CONFIG_SNAPSHOT_COUNT 1    // snapshot slots per track
CONFIG_STEP_COUNT     64   // steps per pattern
```

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `ProjectPage` | `ui/pages/ProjectPage.h/cpp` | Project name, save/load, auto-load toggle |
| `PatternPage` | `ui/pages/PatternPage.h/cpp` | Select active pattern per track |
| `OverviewPage` | `ui/pages/OverviewPage.h/cpp` | Visual overview of all 8 tracks |
| `TrackPage` | `ui/pages/TrackPage.h/cpp` | Per-track settings (mode, name) |

## Change Guide

| Task | Where to look |
|---|---|
| Add a new project-level field | `Project.h` property section + `Project.cpp` `clear()`/`write()`/`read()` + serialization version bump |
| Add a new track type | `Track.h` `TrackMode` enum + `Container<>` template args + new `XxxTrack.h/cpp` + new `XxxTrackEngine.h/cpp` + `Engine.h` includes + `Ui` page wiring |
| Change how pattern switching works | `PlayState.h/cpp` `requestPatternChange()` + `Engine.cpp` pattern change consumption |
| Change CV/gate output routing | `Project.h` `_cvOutputTrack`/`_gateOutput` arrays + `Engine.cpp` output mapping |
| Change project name length | `FileDefs.h` `FileHeader::NameLength` |
