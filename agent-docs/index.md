# PER|FORMER Agent Documentation

PER|FORMER is C++ firmware for a eurorack sequencer (STM32F405, 168 MHz). It runs 8 tracks of six
types (Note, Curve, MidiCv, Stochastic, Arp, Quantizer), 16 patterns per track plus a snapshot, a
64-slot song, 8 CV/gate outputs, 4 CV inputs, MIDI and USB-MIDI, and a 256x64 mono OLED with 32
bi-colour LEDs, 5 function keys and one encoder. The same code builds for hardware and for a
desktop simulator that doubles as the headless test harness.

This directory is the **feature and code documentation**: how things work and where the code lives.
Decisions and findings (why, and what not to redo) live in prep knowledge, not here.

---

## How to use these docs

1. Find the feature below and read its doc *before* searching code. Each feature doc ends with a
   **Change Guide** naming the files to touch.
2. Run `prep knowledge find <word>` (e.g. `note`, `curve`, `slide`, `serialization`, `ui`) for the
   decisions and findings that apply. Feature docs link to decisions by title instead of restating
   them.
3. Read the code. Paths in these docs are relative to `src/apps/sequencer/` unless they start with
   `src/`, `tools/`, `scripts/` or `agent-docs/`.

### By kind of change

| You are... | Read first |
|---|---|
| Changing how a track type sounds (engine) | the track's feature doc, then [architecture.md](architecture.md) (data flow) and [testing.md](testing.md) |
| Adding or changing a stored field | [features/serialization.md](features/serialization.md), then the owning model doc. Bump `model/ProjectVersion.h` |
| Editing a track's Steps page | the track's feature doc (tabs, keys), [features/ui-system.md](features/ui-system.md), [ui-state.md](ui-state.md), [ui-redesign/01-design-language.md](ui-redesign/01-design-language.md) |
| Editing a config (list) page or its preview pane | [features/config-pages.md](features/config-pages.md) |
| Adding a routable parameter | [features/routing.md](features/routing.md) |
| Touching clock, tempo, sync | [features/clock-sync.md](features/clock-sync.md) |
| Writing or fixing a test | [testing.md](testing.md) |
| Designing UI (sandbox, mockups) | [ui-redesign/00-ia.md](ui-redesign/00-ia.md), [ui-redesign/01-design-language.md](ui-redesign/01-design-language.md), `tools/ui-sandbox/README.md` |
| Wondering when something shipped | [features-log.md](features-log.md) (history), or `git log` |

---

## Feature index

### Track types

| Feature | Doc | Main sources |
|---|---|---|
| Note track (Version54 model) | [features/note-track.md](features/note-track.md) | `model/NoteTrack.h`, `model/NoteSequence.h`, `engine/NoteTrackEngine.h` |
| Curve track (segment assembler) | [features/curve-track.md](features/curve-track.md) | `model/CurveTrack.h`, `model/CurveSequence.h`, `engine/CurveTrackEngine.h` |
| Stochastic track (probability bars) | [features/stochastic-track.md](features/stochastic-track.md) | `model/StochasticSequence.h`, `engine/StochasticEngine.h` |
| Arp track (euclidean + note pool) | [features/arp-track.md](features/arp-track.md) | `model/ArpSequence.h`, `engine/ArpTrackEngine.h` |
| Quantizer track | [features/quantizer-track.md](features/quantizer-track.md) | `model/QuantizerTrack.h`, `engine/QuantizerTrackEngine.h` |
| MIDI/CV track | [features/midicv-track.md](features/midicv-track.md) | `model/MidiCvTrack.h`, `engine/MidiCvTrackEngine.h` |

### Engine and model subsystems

| Feature | Doc | Main sources |
|---|---|---|
| Clock, tempo, sync | [features/clock-sync.md](features/clock-sync.md) | `engine/Clock.h`, `model/ClockSetup.h`, `engine/Engine.h` |
| Routing (CV/MIDI to parameters) | [features/routing.md](features/routing.md) | `model/Routing.h`, `engine/RoutingEngine.h` |
| MIDI I/O, MIDI learn | [features/midi-io.md](features/midi-io.md) | `model/MidiOutput.h`, `engine/MidiOutputEngine.h`, `engine/MidiLearn.h` |
| CV in/out and calibration | [features/cv-io.md](features/cv-io.md) | `engine/CvInput.h`, `engine/CvOutput.h`, `model/Calibration.h` |
| Scales | [features/scales.md](features/scales.md) | `model/Scale.h`, `model/UserScale.h` |
| Projects, tracks, patterns | [features/patterns-projects.md](features/patterns-projects.md) | `model/Project.h`, `model/Track.h`, `model/PlayState.h` |
| Song mode | [features/song-mode.md](features/song-mode.md) | `model/Song.h`, `model/PlayState.h`, `ui/pages/SongPage.h` |
| Sequence generators | [features/sequence-generators.md](features/sequence-generators.md) | `engine/generators/Generator.h` |
| Serialization and versioning | [features/serialization.md](features/serialization.md) | `src/core/io/VersionedSerializedWriter.h`, `model/ProjectVersion.h`, `model/Project.cpp` |
| Files, SD card, settings | [features/file-management.md](features/file-management.md) | `model/FileManager.h`, `model/Settings.h` |

### UI

| Feature | Doc |
|---|---|
| UI system: pages, painters, LEDs, controllers | [features/ui-system.md](features/ui-system.md) |
| Config (list) pages, sections, previews | [features/config-pages.md](features/config-pages.md) |
| Launchpad | [features/launchpad.md](features/launchpad.md) |
| Page map, key combos, overlays | [ui-state.md](ui-state.md) |

---

## Cross-cutting docs

| Doc | What it holds |
|---|---|
| [architecture.md](architecture.md) | Layers, tasks, locking, tick data flow, `Routable<T>`, config constants |
| [testing.md](testing.md) | The headless simulator harness, the test list, how to write a test |
| [ui-state.md](ui-state.md) | Every page: how to reach it, footer tabs, notable key combos |
| [features-log.md](features-log.md) | History of shipped features, newest first. Not a source for current behaviour |

### UI design (reference for anyone changing how a page looks)

| Doc | What it holds |
|---|---|
| [ui-redesign/00-ia.md](ui-redesign/00-ia.md) | The target information architecture (tiers, page map). Partly built; see its status note |
| [ui-redesign/01-design-language.md](ui-redesign/01-design-language.md) | The rubric: brightness, type, footer, grid, LED conventions |
| [ui-redesign/08-design-ideas.md](ui-redesign/08-design-ideas.md) | Emerging principles and the line-sequencer experiment |
| [ui-redesign/midicv-page-brief.md](ui-redesign/midicv-page-brief.md) | The one unbuilt Track Edit page (MidiCv): brief and parameters |

The Svelte sandbox where designs are drafted is `tools/ui-sandbox/` (see its `README.md`); its
iteration process is in `tools/ui-sandbox/iteration/` and is started with `/iteration`.

---

## Build, run, test

Everything goes through the `justfile` (`just` lists recipes): `just sim`, `just test [name]`,
`just testsim`, `just firmware`, `just sandbox`, `just docs-check`.

| Target | Entry point |
|---|---|
| STM32 firmware | `src/apps/sequencer/Sequencer.cpp` |
| Simulator | `src/apps/sequencer/SequencerSim.cpp` |
| Python test bindings (`testsim`) | `src/apps/sequencer/python/testsim.cpp` |
| Bootloader | `src/apps/bootloader/` |
| Hardware tester | `src/apps/tester/` |
| C++ unit tests | `src/tests/unit/` |

Constants live in `src/apps/sequencer/Config.h` (track/pattern/step counts, PPQN, version) and
`src/SystemConfig.h` (CPU, IRQ priorities, hardware channel counts).

---

## Keeping these docs current

- A change that alters behaviour, file locations or a stored format updates the matching doc in the
  same change. This is a Definition of Done item (`prep dod`).
- Run `just docs-check` after editing docs: it fails on dead links, dead code paths and docs that
  `index.md` does not link.
- Docs describe the present. History belongs in git and `features-log.md`; reasons belong in prep
  decisions. If a doc starts explaining *why* at length, record a decision and link it.
- Add a new doc to the tables above, or the check fails.
