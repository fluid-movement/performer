# PER|FORMER Agent Documentation

PER|FORMER is a C++ embedded firmware for a eurorack modular synthesizer sequencer. It runs on an STM32F405 microcontroller and provides 8 independent sequencer tracks with multiple track types, 16 patterns per project, 64-slot song arrangement, CV/gate outputs (8 channels), CV inputs (4 channels), MIDI I/O, and a 256×64 OLED display with button/encoder hardware interface.

**Version:** 0.3.2 | **CPU:** STM32F405 (168MHz) | **Source files:** ~572 C++ files

---

## Quick Start for Agents

- Start with [architecture.md](architecture.md) to understand the 5-layer structure and data flow.
- Find the feature you're changing in the table below.
- Each feature doc has a **Change Guide** section listing exactly which files to edit.

---

## Feature Index

| Feature | Doc | Primary Source Paths |
|---|---|---|
| Note Track | [features/note-track.md](features/note-track.md) | `model/NoteTrack.h`, `model/NoteSequence.h`, `engine/NoteTrackEngine.h` |
| Curve Track | [features/curve-track.md](features/curve-track.md) | `model/CurveTrack.h`, `model/CurveSequence.h`, `engine/CurveTrackEngine.h` |
| Stochastic Track | [features/stochastic-track.md](features/stochastic-track.md) | `model/StochasticTrack.h`, `model/StochasticSequence.h`, `engine/StochasticEngine.h` |
| Logic Track | [features/logic-track.md](features/logic-track.md) | `model/LogicTrack.h`, `model/LogicSequence.h`, `engine/LogicTrackEngine.h` |
| Arp Track | [features/arp-track.md](features/arp-track.md) | `model/ArpTrack.h`, `model/ArpSequence.h`, `model/Arpeggiator.h`, `engine/ArpTrackEngine.h` |
| MIDI/CV Track | [features/midicv-track.md](features/midicv-track.md) | `model/MidiCvTrack.h`, `engine/MidiCvTrackEngine.h` |
| Clock & Sync | [features/clock-sync.md](features/clock-sync.md) | `engine/Clock.h`, `model/ClockSetup.h`, `engine/TapTempo.h` |
| Routing (CV/MIDI modulation) | [features/routing.md](features/routing.md) | `model/Routing.h`, `engine/RoutingEngine.h` |
| MIDI I/O | [features/midi-io.md](features/midi-io.md) | `model/MidiOutput.h`, `engine/MidiOutputEngine.h`, `engine/MidiLearn.h` |
| Song Mode | [features/song-mode.md](features/song-mode.md) | `model/Song.h`, `model/PlayState.h`, `ui/pages/SongPage.h` |
| Patterns & Projects | [features/patterns-projects.md](features/patterns-projects.md) | `model/Project.h`, `model/Track.h`, `model/PlayState.h` |
| Sequence Generators | [features/sequence-generators.md](features/sequence-generators.md) | `engine/generators/EuclideanGenerator.h`, `engine/generators/RandomGenerator.h` |
| File Management | [features/file-management.md](features/file-management.md) | `model/FileManager.h`, `core/fs/`, `model/FileDefs.h` |
| UI System | [features/ui-system.md](features/ui-system.md) | `ui/Ui.h`, `ui/PageManager.h`, `ui/Page.h`, `ui/LedPainter.h` |
| Launchpad Integration | [features/launchpad.md](features/launchpad.md) | `ui/controllers/launchpad/LaunchpadController.h`, `Mk2Device.h`, `Mk3Device.h`, `ProDevice.h`, `ProMk3Device.h` |
| Scales & User Scales | [features/scales.md](features/scales.md) | `model/Scale.h`, `model/UserScale.h` |
| CV I/O & Calibration | [features/cv-io.md](features/cv-io.md) | `engine/CvInput.h`, `engine/CvOutput.h`, `model/Calibration.h`, `platform/stm32/drivers/Adc.h`, `Dac.h` |
| Serialization | [features/serialization.md](features/serialization.md) | `core/io/VersionedSerializedWriter.h`, `core/io/VersionedSerializedReader.h`, `model/Serialize.h` |

All paths above are relative to `src/apps/sequencer/` unless prefixed with `src/`.

---

## Build Targets & Entry Points

| Target | Entry Point | Command |
|---|---|---|
| STM32 firmware | `src/apps/sequencer/Sequencer.cpp` | `cmake -DPLATFORM=stm32 && make sequencer` |
| Simulator (macOS/Linux) | `src/apps/sequencer/SequencerSim.cpp` | `cmake -DPLATFORM=sim && make sequencer` |
| Bootloader | `src/apps/bootloader/` | `make bootloader` |
| Hardware tester | `src/apps/tester/` | `make tester` |
| Unit tests | `src/tests/unit/` | `make && ctest` |

Firmware output: `.bin` / `.hex` in `bin/`. Bootloader update file: `UPDATE.DAT`.

---

## Key Config Files

| File | Purpose |
|---|---|
| `src/apps/sequencer/Config.h` | All sequencer constants (track count, pattern count, PPQN, task priorities) |
| `src/SystemConfig.h` | CPU frequency, IRQ priorities, hardware pin counts |
| `CMakeLists.txt` (root) | Build system entry |

---

## Architecture Summary

See [architecture.md](architecture.md) for the full diagram. In brief:

```
UI (pages, LEDs, display)
  ↕ direct model access + Engine lock/suspend
Engine (clock, track engines, routing, MIDI out)
  ↕ direct model reads; clock/MIDI driver callbacks
Model (Project, Tracks, Sequences, Settings)
  ↕ serialized to/from SD card and flash
Platform drivers (ADC, DAC, OLED, UART MIDI, USB, SD, buttons)
Core libraries (FS, graphics canvas, math, MIDI parser)
```
