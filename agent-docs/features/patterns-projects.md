# Projects, Tracks and Patterns

A `Project` is the top-level data container. It holds 8 tracks, each with 16 patterns plus one
snapshot slot, the project-level settings, the song, routing, MIDI outputs and the play state.
Switching a track's pattern switches which of its sequences plays; the others are kept.

## Key source files

| Role | Path |
|---|---|
| Project | `model/Project.{h,cpp}` |
| Track (polymorphic) | `model/Track.{h,cpp}`, `model/BaseTrack.h`, `model/BaseTrackPatternFollow.h` |
| Play state (patterns, mutes, fills, song position) | `model/PlayState.{h,cpp}` |
| Model (Project + Settings + clipboard) | `model/Model.{h,cpp}`, `model/ClipBoard.{h,cpp}` |
| Project page (settings list) | `ui/pages/ProjectPage.{h,cpp}`, `ui/model/ProjectListModel.h` |
| Track page (type, link, per-track settings) | `ui/pages/TrackPage.{h,cpp}`, `ui/model/*TrackListModel.h` |
| Pattern page, Performer page, Overview page | `ui/pages/PatternPage.{h,cpp}`, `ui/pages/PerformerPage.{h,cpp}`, `ui/pages/OverviewPage.{h,cpp}` |
| Layout page (jack assignment) | `ui/pages/LayoutPage.{h,cpp}` |

## `Project`

Persisted: name, tempo and swing (routable), time signature, sync measure, `scale` and `rootNote`
(global, see [scales](scales.md)), monitor, record and MIDI input modes, MIDI integration mode and
program offset, `cvGateInput`, `curveCvInput`, `ClockSetup`, the 8 tracks, `cvOutputTracks` and
`gateOutputTracks` (which track feeds each jack), `Song`, `PlayState`, `Routing`, `MidiOutput`, the
4 user scales, `selectedTrackIndex`, `selectedPatternIndex`, `resetCvOnStop`, `useMultiCv`.
Other settings: steps-to-stop, record delay. Runtime only: `slot` (the SD card slot it came from),
`autoLoaded`.

`Project::watch(handler)` observes `ProjectCleared`, `ProjectRead`, `TrackModeChanged`,
`SelectedTrackIndexChanged`, `SelectedPatternIndexChanged`. Helpers such as `selectedNoteSequence()`
return the sequence of the selected track and pattern; while a snapshot is active the "selected"
pattern is the snapshot slot.

## `Track`

A `Container<NoteTrack, CurveTrack, MidiCvTrack, StochasticTrack, ArpTrack, QuantizerTrack>` plus
the `TrackMode`, a `linkTrack` (a track can follow another's step position; engines read the
linked engine's `linkData`) and the track index. `setTrackMode` resets the track. Typed access:
`noteTrack()`, `curveTrack()` and so on (an assertion fires for the wrong mode); engines use
`TrackEngine::as<T>()`. `TrackMode` ids on
disk are stable; unknown ids (the removed Logic track) load as Note. See
[serialization](serialization.md).

## Patterns and `PlayState`

Each track type holds `CONFIG_PATTERN_COUNT + CONFIG_SNAPSHOT_COUNT` = 17 sequences. `PlayState`
(persisted with the project) holds per track a `TrackState`: current and requested pattern, mute,
fill and fill amount, plus pending requests, and a `SongState` ([song-mode](song-mode.md)).

Changes can be requested with an `ExecuteType`: **Immediate**, **Synced** (at the next sync
boundary, `Engine::syncDivisor`) or **Latched** (queued until committed). API: `muteTrack`,
`soloTrack`, `fillTrack`, `selectTrackPattern`, `selectPattern`, snapshots (`createSnapshot`,
`revertSnapshot`, `commitSnapshot`), `cancelMuteRequests` / `cancelPatternRequests`, `playSong`.
`Engine::updatePlayState` consumes the requests on ticks.

## UI

- **PatternPage** (Pattern key; Page + Pattern for the non-modal version): per-track pattern grid;
  footer LATCH / SYNC / SNAP (REVERT) / COMMIT / CANCEL.
- **PerformerPage** (Performer key): mutes, solo, fills; footer LATCH / SYNC / UNMUTE / FILL / CANCEL.
- **OverviewPage** (Page + Left): all tracks at a glance.
- **ProjectPage** (Page + Track 0) and **TrackPage** (Page + Step 2): config lists
  ([config-pages](config-pages.md)). Details of each page's keys: [ui-state.md](../ui-state.md).

## Change guide

| Task | Where |
|---|---|
| Add a project-level field | `model/Project.h` accessors, `clear()`, `write()`/`read()` in `Project.cpp` with a new `ProjectVersion`, a row in `ProjectListModel.h`, a binding in `python/project.cpp` ([serialization](serialization.md)) |
| Add a track type | `model/Track.h` (`TrackMode`, container args, serialize id, `write`/`read`), `engine/Engine.h` (engine container), a model, engine and pages, `ui/pages/Pages.h`, `TopPage` page selection, list model, routing `writeRouted` cases |
| Change pattern switching | `PlayState::selectTrackPattern` and `Engine::updatePlayState` |
| Change jack mapping | `Project::cvOutputTracks` / `gateOutputTracks`, `Engine::updateTrackOutputs`, `LayoutPage` |
