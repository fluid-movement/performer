# Architecture

PER|FORMER is a C++ embedded firmware for a eurorack hardware sequencer (STM32F405, 168MHz, 1MB flash, 192KB RAM). The codebase is structured in 5 horizontal layers that communicate in strictly controlled ways.

---

## Layer Diagram

```
┌─────────────────────────────────────────────────────────┐
│  UI Layer                                               │
│  src/apps/sequencer/ui/                                 │
│  Page-based display, button/encoder input, LED state    │
├─────────────────────────────────────────────────────────┤
│  Engine Layer                                           │
│  src/apps/sequencer/engine/                             │
│  Real-time clock, track engines, routing, MIDI output   │
├─────────────────────────────────────────────────────────┤
│  Model Layer                                            │
│  src/apps/sequencer/model/                              │
│  All persisted data: Project, Track, Sequence, Settings │
├─────────────────────────────────────────────────────────┤
│  Platform / Drivers                                     │
│  src/platform/stm32/  (hardware) or src/platform/sim/   │
│  ADC, DAC, LCD, MIDI, USB, SD card, button matrix, LEDs │
├─────────────────────────────────────────────────────────┤
│  Core Libraries                                         │
│  src/core/                                              │
│  FS, graphics canvas, math, MIDI protocol, utilities    │
└─────────────────────────────────────────────────────────┘
```

---

## Layer Communication

| From → To | Mechanism |
|---|---|
| UI → Model | Direct read/write of `Project` properties via `model.project()` |
| UI → Engine | Method calls on `Engine` (lock/suspend, playback control) |
| Engine → Model | Direct read of `Project`; write via `writeRouted()` for routing targets |
| Engine → Drivers | Direct calls to `CvOutput`, `GateOutput`, `Midi`, `UsbMidi`, `Dio` |
| Drivers → Engine | Interrupt callbacks (clock ticks, MIDI bytes received) |
| Engine → UI | `MessageHandler` callback for toast notifications |
| UI ↔ Engine | `Engine::lock()` / `Engine::suspend()` guard critical sections |

There is **no event bus** — communication is direct and synchronous except for FreeRTOS queues used to pass input events from the driver task to the UI task.

---

## FreeRTOS Task Architecture

| Task | Priority | Stack | Period | Role |
|---|---|---|---|---|
| Driver task | 5 (highest) | 1 KB | 1ms | Reads ADC, scans button matrix, drives LEDs, feeds clock |
| Engine task | 4 | 4 KB | Clock-driven | Processes clock ticks, runs all track engines, writes DAC/gates |
| USB host task | 3 | 2 KB | Event-driven | Handles USB MIDI connect/disconnect, message receive |
| UI task | 2 | 4 KB | 20ms (50fps) | Renders display, processes input events, updates LEDs |
| File I/O task | 1 | 4 KB | On-demand | SD card read/write (FatFs operations) |
| Profiler task | 0 (lowest) | 2 KB | Idle | CPU utilization tracking |

Critical section: UI reads model → must `Engine::lock()` briefly to avoid data races. File operations: `Engine::suspend()` + `Engine::resume()` around long SD operations.

---

## Polymorphic Track System

A `Track` object is a tagged union (`Container<NoteTrack, CurveTrack, MidiCvTrack, StochasticTrack, LogicTrack, ArpTrack>` — see `src/core/utils/Container.h`). The `TrackMode` enum selects the active type. Switching mode resets all track data.

Each track type has a corresponding engine class:

| Track Type | Model | Engine |
|---|---|---|
| Note | `model/NoteTrack.h` | `engine/NoteTrackEngine.h` |
| Curve | `model/CurveTrack.h` | `engine/CurveTrackEngine.h` |
| MIDI/CV | `model/MidiCvTrack.h` | `engine/MidiCvTrackEngine.h` |
| Stochastic | `model/StochasticTrack.h` | `engine/StochasticEngine.h` |
| Logic | `model/LogicTrack.h` | `engine/LogicTrackEngine.h` |
| Arp | `model/ArpTrack.h` | `engine/ArpTrackEngine.h` |

`Engine` holds an `std::array<TrackEngineContainer, 8>` where each element is a `Container<>` over all engine types. The active engine type is kept in sync with the track's `TrackMode`.

---

## Data Flow: Clock Tick → CV Output

```
ClockTimer IRQ (168MHz timer)
  → Clock::onClockTick()
    → Engine::onClockOutput()  [Clock::Listener]
      → For each of 8 tracks:
          TrackEngine::tick(ticks, ppqn)
            SequenceState::advance()     ← determines current step
            [evaluate step data from NoteSequence/CurveSequence/...]
            CvOutput::setVoltage(ch, v)  ← 16-bit DAC
            GateOutput::set(ch, gate)    ← 74HC595 shift register
      → RoutingEngine::update()          ← apply CV/MIDI modulation
      → MidiOutputEngine::update()       ← send MIDI notes out
```

---

## Routable Properties

Many model properties (tempo, octave, transpose, etc.) are wrapped in `Routable<T>` (defined in `model/Routing.h`). This type stores two values: `base` (user-set) and `routed` (set by `RoutingEngine`). The getter `get(bool isRouted)` returns the routed value when a route is active, otherwise the base value. When writing from the UI, `routed=false`; when writing from `RoutingEngine`, `routed=true`.

---

## Key Config Constants

All constants in `src/apps/sequencer/Config.h` and `src/SystemConfig.h`:

```cpp
// Firmware version
CONFIG_VERSION_MAJOR       0
CONFIG_VERSION_MINOR       3
CONFIG_VERSION_REVISION    2

// CPU + system
CONFIG_CPU_FREQUENCY       168000000   // Hz
CONFIG_TICK_FREQUENCY      1000        // ms-based system tick

// Clock resolution
CONFIG_PPQN                192         // master ticks per quarter note
CONFIG_SEQUENCE_PPQN       48          // sequence step resolution (4:1 ratio)

// Sequencer capacity
CONFIG_TRACK_COUNT         8
CONFIG_PATTERN_COUNT       16
CONFIG_STEP_COUNT          64
CONFIG_SONG_SLOT_COUNT     64
CONFIG_CHANNEL_COUNT       8           // CV+gate output pairs
CONFIG_CV_INPUT_CHANNELS   4
CONFIG_CV_OUTPUT_CHANNELS  8

// Modulation
CONFIG_ROUTE_COUNT         16          // max simultaneous routing slots
CONFIG_MIDI_OUTPUT_COUNT   16          // max MIDI output mappings

// Scales
CONFIG_USER_SCALE_COUNT    4
CONFIG_USER_SCALE_SIZE     32

// Display
CONFIG_LCD_WIDTH           256
CONFIG_LCD_HEIGHT          64
CONFIG_DEFAULT_UI_FPS      50

// Storage
CONFIG_SETTINGS_FLASH_SECTOR  3
CONFIG_SETTINGS_FLASH_ADDR    0x0800C000
```

---

## Entry Points

| Target | File | Description |
|---|---|---|
| Hardware firmware | `src/apps/sequencer/Sequencer.cpp` | `main()` — initializes all drivers, creates Model/Engine/Ui, starts FreeRTOS |
| Simulator | `src/apps/sequencer/SequencerSim.cpp` | Same core, simulated drivers, NanoVG frontend |
| Bootloader | `src/apps/bootloader/` | Minimal app that loads `UPDATE.DAT` from SD and flashes firmware |
| Hardware tester | `src/apps/tester/` | Tests individual hardware peripherals |

---

## How Features Connect

```
Project (model)
├── ClockSetup → Clock (engine) → ClockTimer (driver)
├── Track[0..7]
│     ├── NoteTrack → NoteTrackEngine → CvOutput + GateOutput
│     ├── CurveTrack → CurveTrackEngine → CvOutput
│     ├── MidiCvTrack → MidiCvTrackEngine ← Midi/UsbMidi (driver)
│     ├── StochasticTrack → StochasticEngine → CvOutput + GateOutput
│     ├── LogicTrack → LogicTrackEngine → GateOutput (reads other engines)
│     └── ArpTrack → ArpTrackEngine → CvOutput + GateOutput
├── Song → PlayState → Engine (slot advance logic)
├── Routing → RoutingEngine ← CvInput (ADC) + Midi/UsbMidi
├── MidiOutput[0..15] → MidiOutputEngine → Midi/UsbMidi
└── UserScale[0..3] → Scale lookup in TrackEngines
```

---

## Simulator vs Hardware

The codebase compiles for two targets by selecting the `src/platform/` layer:

| | STM32 | Simulator |
|---|---|---|
| RTOS | FreeRTOS | Threads (pthreads) |
| Display | SPI OLED (256×64) | NanoVG window |
| ADC/DAC | Hardware peripherals | Simulated sliders/knobs |
| MIDI | UART + libopencm3 | RtMidi |
| Audio | — | SoLoud (for gate click preview) |
| Build | `cmake -DPLATFORM=stm32` | `cmake -DPLATFORM=sim` |

All `src/apps/` and `src/core/` code is platform-agnostic.
