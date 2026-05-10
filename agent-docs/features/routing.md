# Routing

## Purpose
A modulation routing system that maps CV inputs, MIDI CC, or other sources to project/track/sequence parameters. Up to 16 routes can be active simultaneously. Enables real-time external control of nearly any parameter without MIDI learn.

## Key Source Files

| Role | Path |
|---|---|
| Data model | `src/apps/sequencer/model/Routing.h/cpp` |
| Runtime routing engine | `src/apps/sequencer/engine/RoutingEngine.h/cpp` |
| Routing page | `src/apps/sequencer/ui/pages/RoutingPage.h/cpp` |
| UI list model | `src/apps/sequencer/ui/model/RoutingListModel.h` |

## Data Model (`Routing`)

### `Routing::Route`
Each route has:
- `_target` — what parameter to control (`Routing::Target` enum, see below)
- `_tracks` — bitmask of which tracks this route applies to (8-bit, one bit per track)
- `_min`, `_max` — output range scaling (normalize source value into this range)
- `_source` — `Routing::Source` variant (CvInput, MidiCC, MidiPitchBend, MidiChannelPressure, MidiNoteMomentary, MidiNoteToggle, MidiNoteVelocity)

Stored as array of 16 `Route` objects in `Project` via `Routing`.

### `Routing::Target` enum (key targets)
```
Engine targets:    Play, PlayToggle, Record, RecordToggle, TapTempo
Project targets:   Tempo, Swing
PlayState targets: Mute, Fill, FillAmount, Pattern
Track targets:     SlideTime, Octave, Transpose, Offset, Rotate,
                   GateProbabilityBias, RetriggerProbabilityBias, LengthBias,
                   NoteProbabilityBias, ShapeProbabilityBias, CurveMin, CurveMax
Sequence targets:  FirstStep, LastStep, RunMode, Divisor, Scale, RootNote,
                   CurrentRecordStep, Reseed, RestProbability2, RestProbability4, ...
```

### `Routing::Source` types
- `CvInput` — one of 4 analog CV inputs (`CONFIG_CV_INPUT_CHANNELS = 4`)
- `MidiCC` — MIDI control change on a port+channel+CC number
- `MidiPitchBend` — pitch bend value
- `MidiChannelPressure` — aftertouch
- `MidiNoteMomentary` / `MidiNoteToggle` / `MidiNoteVelocity` — MIDI note events as control source

### `Routable<T>` template
Many model properties are wrapped in `Routable<T>` (defined in `Routing.h`). This stores both a base value and a routed value, and `get(bool isRouted)` returns the routed value when the routing engine is active, or the base value otherwise. This is why the `isRouted()` check appears throughout model property getters.

## Engine Logic (`RoutingEngine`)

On every engine update tick:
1. Read current source values: CV input voltages from `CvInput`, MIDI CC values from MIDI message buffer.
2. For each active `Route`:
   a. Read the source value.
   b. Scale/clamp to the route's `_min`/`_max` range.
   c. Write the result to the target parameter using `Routing::writeRouted(target, trackMask, value)`.
3. `writeRouted()` calls the appropriate model setter (e.g. `project.setTempo()`, `track.setOctave()`) with the `routed=true` flag.

## UI Page (`RoutingPage`)

Displays a list of all 16 routes. Each entry shows: source → target, track mask, min/max range. The user can add/remove/configure routes here.

## Change Guide

| Task | Where to look |
|---|---|
| Add a new routable target | `Routing.h` `Target` enum + `RoutingEngine.cpp` `writeRouted()` switch + model setter |
| Add a new source type | `Routing.h` `Source` variant + `RoutingEngine.cpp` source read section |
| Change how CV inputs are read | `src/apps/sequencer/engine/CvInput.h/cpp` + `RoutingEngine.cpp` |
| Fix route value scaling | `RoutingEngine.cpp` min/max normalization section |
| Make a model property routable | Wrap the field in `Routable<T>`, add `isRouted()` check in getter, add `Target` enum entry, update `RoutingEngine::writeRouted()` |
