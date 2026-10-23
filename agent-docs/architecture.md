# Architecture

PER|FORMER is C++ firmware for a eurorack sequencer (STM32F405, 168 MHz, 1 MB flash, 192 KB RAM).
The code is five layers; the same app code builds for hardware and for a desktop simulator by
swapping the platform layer.

---

## Layers

```
UI        src/apps/sequencer/ui/       pages, painters, LEDs, controllers (Launchpad)
Engine    src/apps/sequencer/engine/   clock, track engines, routing, MIDI out, CV/gate out
Model     src/apps/sequencer/model/    Project, tracks, sequences, settings, (de)serialization
Platform  src/platform/stm32|sim/      ADC, DAC, LCD, MIDI, USB, SD, buttons/LEDs, OS wrappers
Core      src/core/                    fs, gfx Canvas, math, MIDI parser, io, utils (Container)
```

`src/apps/` and `src/core/` are platform-agnostic. Other apps: `src/apps/bootloader/`,
`src/apps/tester/`. Unit tests are in `src/tests/unit/`.

| From to | Mechanism |
|---|---|
| UI to Model | direct reads and writes through `Project` (`model.project()`) |
| UI to Engine | method calls on `Engine`; `Engine::lock()` / `suspend()` guard critical sections |
| Engine to Model | reads `Project`; writes routed values through `writeRouted()` |
| Engine to hardware | `CvOutput`, `GateOutput`, `Midi`, `UsbMidi`, `Dio` |
| Engine to UI | `MessageHandler` callback for toasts; the UI polls engine state |

There is no event bus. Communication is direct and synchronous.

---

## Tasks (FreeRTOS on hardware)

Defined in `src/apps/sequencer/Sequencer.cpp`; priorities and stacks in `src/apps/sequencer/Config.h`.

| Task | Priority | Period | Does |
|---|---|---|---|
| driver | 5 | 1 ms | shift register, button/LED matrix, encoder scan |
| engine | 4 | 1 ms | `Engine::update()`: drains clock ticks, ticks track engines, writes outputs |
| usbh | 3 | 1 ms | USB host / USB-MIDI |
| ui | 2 | 1 ms poll | `Ui::update()`: input events, draws at the page's fps (50 by default), LEDs |
| file | 1 | 10 ms | `FileManager::processTask()`: SD card work |
| profiler | 0 | 5 s | task CPU dump |

**Locking.** The UI task must `Engine::lock()` briefly when it reads or writes model data the
engine is reading, and `Engine::suspend()` / `resume()` around long operations (file load/save).
On the simulator both functions pump `Engine::update()` themselves, because there is no second
task.

---

## The tick: clock to CV/gate

The hardware clock timer (or an external clock) produces ticks at `CONFIG_PPQN` (192 per quarter
note); `Clock` queues them. The engine task consumes them:

```
Engine::update()                      every 1 ms
  clock events (start/stop/continue/reset)
  tempo, clock setup, track setups, play state, CV inputs
  receiveMidi()                       routing, MIDI learn, MIDI/CV tracks, monitoring/recording
  routingEngine.update()
  while (clock.checkTick(&tick)):
      updatePlayState(true)           pattern changes, song advance, fills
      for each of the 8 tracks:       trackEngine->tick(tick)
          SequenceState advances the step; the engine queues gate and CV events
          if the CV changed: update(0), updateTrackOutputs(), routingEngine.update()
      midiOutputEngine.update() on tick 0
  trackEngine->update(dt) for each    slide ramps and other time-based smoothing
  midiOutputEngine.update()
  updateTrackOutputs()                maps track engines to physical CV/gate jacks (Project mapping)
  cvOutput.update(); gateOutput.update()   write DAC and shift registers
```

Sequence resolution is `CONFIG_SEQUENCE_PPQN` = 48; a step's length is its `divisor` in ticks of
the 192 PPQN clock. The Clock class itself is described in [features/clock-sync.md](features/clock-sync.md).

---

## Polymorphic tracks

`Track` (`model/Track.h`) holds a `Container<NoteTrack, CurveTrack, MidiCvTrack, StochasticTrack,
ArpTrack, QuantizerTrack>` (see `src/core/utils/Container.h`); `TrackMode` selects the active type
and switching mode resets the track's data. `Engine` holds one `TrackEngineContainer` per track, a
`Container` over the matching engines, kept in sync with `TrackMode`.

| Type | Model | Engine | Doc |
|---|---|---|---|
| Note | `model/NoteTrack.h` | `engine/NoteTrackEngine.h` | [note-track](features/note-track.md) |
| Curve | `model/CurveTrack.h` | `engine/CurveTrackEngine.h` | [curve-track](features/curve-track.md) |
| MidiCv | `model/MidiCvTrack.h` | `engine/MidiCvTrackEngine.h` | [midicv-track](features/midicv-track.md) |
| Stochastic | `model/StochasticTrack.h` | `engine/StochasticEngine.h` | [stochastic-track](features/stochastic-track.md) |
| Arp | `model/ArpTrack.h` | `engine/ArpTrackEngine.h` | [arp-track](features/arp-track.md) |
| Quantizer | `model/QuantizerTrack.h` | `engine/QuantizerTrackEngine.h` | [quantizer-track](features/quantizer-track.md) |

The serialized `TrackMode` ids are stable; the removed Logic track (id 4) loads as Note (see
`prep knowledge find logic`). Mode ids and the (de)serialization are in
[features/serialization.md](features/serialization.md).

Shared engine pieces: `engine/SequenceState.h` (step position, run modes), `engine/SequenceUtils.h`
and `engine/TrackEngineHelpers.h` (clock params, gate/CV queue draining), `engine/SortedQueue.h`
(event queues; usable capacity is one less than declared), `engine/Groove.h` (swing),
`engine/Slide.h` (slide ramps), `engine/generators/` (see [features/sequence-generators.md](features/sequence-generators.md)).

---

## Routable properties

Many model properties are `Routable<T>` (`model/Routing.h`): a `base` value set from the UI and a
`routed` value written by `RoutingEngine`. Getters call `get(isRouted(Target))`; UI edits write the
base value and are refused while the parameter is routed. Details in [features/routing.md](features/routing.md).

---

## How the model hangs together

```
Project
  ClockSetup, tempo, swing, time signature, scale, root note, MIDI/CV-gate input settings
  Track[8]  ->  one of the six track types, each with 16 patterns + 1 snapshot sequence
  cvOutputTracks[8], gateOutputTracks[8]   which track feeds which jack
  Song (64 slots), PlayState (patterns, mutes, fills, song position)
  Routing (16 routes), MidiOutput (16 outputs)
UserScale[4]   static, shared across the project (UserScale::userScales)
Settings       calibration + user settings, stored outside the project
```

Constants (`src/apps/sequencer/Config.h`): 8 tracks, 16 patterns, 64 steps, 64 song slots, 16
routes, 16 MIDI outputs, 4 user scales of 32 notes. Hardware channel counts are in
`src/SystemConfig.h`.

---

## Simulator and hardware

| | STM32 | Simulator |
|---|---|---|
| RTOS | FreeRTOS | host threads |
| Display | SPI OLED | NanoVG window |
| ADC/DAC | peripherals | simulated |
| MIDI | UART + USB host | RtMidi |
| Build | `just firmware` | `just sim` |

`testsim` is the same app built as a Python module for headless runs; see [testing.md](testing.md).
