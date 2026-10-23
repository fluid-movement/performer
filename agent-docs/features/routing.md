# Routing

Maps a source (a CV input, a CV output, or a MIDI message) to a parameter. Up to 16 routes
(`CONFIG_ROUTE_COUNT`) can be active at once. Routing a parameter takes it over: the UI shows it as
routed and refuses edits.

## Key source files

| Role | Path |
|---|---|
| Routes, targets, sources, `Routable<T>` | `model/Routing.{h,cpp}` |
| Runtime engine | `engine/RoutingEngine.{h,cpp}` |
| MIDI source matching | `engine/MidiUtils.h` |
| Routing page | `ui/pages/RoutingPage.{h,cpp}` |
| Route list rows | `ui/model/RouteListModel.h` |
| Open a parameter's route from its page | `ui/pages/TopPage.cpp` (`TopPage::editRoute`) |

## Data model

### `Routing::Route`

| Field | Meaning |
|---|---|
| `target` | what to control (`Routing::Target`); `None` = inactive route |
| `min`, `max` | normalized 0-1 range the source is scaled into, edited as target units |
| `tracks` | 8-bit track mask; only used by per-track targets |
| `source` | `None`, `CvIn1-4`, `CvOut1-8`, `Midi` |
| `cvSource` | voltage range used to normalize a CV source |
| `midiSource` | port/channel (`MidiSourceConfig`), event (CC Absolute, CC Relative, Pitch Bend, Note Momentary, Note Toggle, Note Velocity, Note Range), control number or note, note range |

### `Routing::Target` groups

| Group | Targets |
|---|---|
| Engine | Play, PlayToggle, Record, RecordToggle, TapTempo |
| Project | Tempo, Swing |
| PlayState | Mute, Fill, FillAmount, Pattern |
| Track | SlideTime, Octave, Transpose, Offset, Rotate, GateProbabilityBias, RetriggerProbabilityBias, LengthBias, NoteProbabilityBias |
| Sequence | FirstStep, LastStep, RunMode, Divisor, Scale, RootNote, CurrentRecordStep, Reseed, RestProbability2/4/8, SequenceFirstStep, SequenceLastStep, LowOctaveRange, HighOctaveRange, LengthModifier |

Targets serialize by a stable id (`targetSerialize`) and an unknown id reads back as `Target::None`,
so removing a target needs no migration (see `prep knowledge find routing`). Which track types honour
a target is decided by each track's `writeRouted`.

### `Routable<T>`

A property holds `base` (set by the UI) and `routed` (set by the engine). Getters call
`get(isRouted(target))`; setters take a `routed` flag. `Routing::isRouted(target, trackIndex)` is
true while an active route covers that target and track.

## Engine (`RoutingEngine::update`, every engine update)

1. `updateSources()`: CV sources are `range.normalize(cvInput.channel(i))` (or `cvOutput` for CvOut
   sources). MIDI sources are updated in `receiveMidi()`, called from `Engine::receiveMidi` before
   the track engines see a message; a matching message is consumed.
2. `updateSinks()`: for each active route, `value = min + source * (max - min)`. Engine targets go
   to `writeEngineTarget` (Play/Record toggle on edges, TapTempo on a rising edge); everything else
   goes to `Routing::writeTarget(target, tracks, value)`, which denormalizes the value and calls the
   owner's `writeRouted(target, int, float)`: `Project`, `PlayState`, or the selected tracks' track
   and (for sequence targets) all 16 of their sequences.
3. When a route's target or tracks change, the old target is un-routed and the new one marked with
   `Routing::setRouted`.

## UI (`RoutingPage`)

Reached with Page + Track 2 or from a parameter's context menu (ROUTE). It edits a scratch copy of
one route; footer **PREV / NEXT / INIT / LEARN / COMMIT**. COMMIT copies the edit into the route
unless `checkRouteConflict` finds another route on the same target and tracks. LEARN starts MIDI
learn ([midi-io](midi-io.md)) and fills the MIDI source from the next message. Shift + encoder
switches route. On the Tracks row the track buttons toggle membership directly.

## Change guide

| Task | Where |
|---|---|
| Add a routable target | `Routing::Target` and its name/serialize/range helpers in `model/Routing.{h,cpp}`; a `writeRouted` case in the owner (track, sequence, project or play state); group predicates if it is a new group |
| Make a property routable | wrap it in `Routable<T>`, use `get(isRouted(target))` in the getter, refuse edits when routed, add a `writeRouted` case |
| New source type | `Routing::Source`, `RoutingEngine::updateSources`, the route editor rows in `RouteListModel.h` |
| Route value scaling | `Routing::normalizedDefaultRange`, `denormalizeTargetValue`, `targetValueStep` |
