# MIDI I/O

## Purpose
Handles MIDI input and output across hardware UART, USB-MIDI host, and virtual ports. Includes MIDI output mapping (send track events as MIDI notes), MIDI learn for parameter binding, and a MIDI monitor page.

## Key Source Files

| Role | Path |
|---|---|
| MIDI output data model | `src/apps/sequencer/model/MidiOutput.h/cpp` |
| MIDI output engine | `src/apps/sequencer/engine/MidiOutputEngine.h/cpp` |
| MIDI learn | `src/apps/sequencer/engine/MidiLearn.h/cpp` |
| CV-to-MIDI converter | `src/apps/sequencer/engine/CvGateToMidiConverter.h` |
| MIDI port abstraction | `src/apps/sequencer/engine/MidiPort.h` |
| MIDI message type | `src/core/midi/MidiMessage.h/cpp` |
| MIDI parser | `src/core/midi/MidiParser.h/cpp` |
| MIDI output page | `src/apps/sequencer/ui/pages/MidiOutputPage.h/cpp` |
| MIDI monitor page | `src/apps/sequencer/ui/pages/MonitorPage.h/cpp` |
| UART MIDI driver (STM32) | `src/platform/stm32/drivers/Midi.h` |
| USB MIDI driver (STM32) | `src/platform/stm32/drivers/UsbMidi.h` |

## Data Model (`MidiOutput`)

Each of the 16 `MidiOutput` slots defines one MIDI output mapping:
- `_event` — what event triggers output: `NoteOn`/`NoteOff`, `ControlChange`
- `_source` — which track and which parameter to read (e.g. Track 1, gate, note)
- `_port` — which MIDI port to send on (UART, USB-MIDI)
- `_channel` — MIDI channel (1–16)
- `_noteOffset` — note transposition for MIDI output
- `_arpeggiator` — optional arpeggiator applied before output

Stored as `std::array<MidiOutput, CONFIG_MIDI_OUTPUT_COUNT>` (16 slots) in `Project`.

## Engine Logic

### `MidiOutputEngine`
On every step event from any track engine:
1. Iterate all 16 `MidiOutput` slots.
2. For each slot whose source matches the current track event: build a `MidiMessage` (note-on/note-off/CC).
3. Write to the appropriate MIDI port via `Engine::sendMidi()`.

### `MidiLearn`
- Activated by the user pressing the MIDI learn button.
- Listens for incoming MIDI CC/note messages.
- Binds the received message to the currently selected routing target.
- Stored as a `Routing::Route` with `MidiCC`/`MidiNote` source type.
- Key file: `MidiLearn.h/cpp`.

### `CvGateToMidiConverter`
Converts CV+gate track output to MIDI note events. Used when a hardware CV output should also appear as MIDI. Tracks the gate state to generate note-on/note-off pairs.

### MIDI Ports
`MidiPort` enum: `Midi` (UART DIN), `UsbMidi` (USB host), `None`.

`Engine` owns both `Midi` and `UsbMidi` drivers. Incoming messages pass through `Engine::onMidiMessage()` which distributes them to:
- `RoutingEngine` (for routing/MIDI-learn sources)
- `MidiCvTrackEngine` instances (for MIDI/CV conversion)
- `Clock` (for MIDI clock sync)

## Constants
```cpp
CONFIG_MIDI_OUTPUT_COUNT  16   // number of MIDI output mapping slots
```

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `MidiOutputPage` | `ui/pages/MidiOutputPage.h/cpp` | Configure MIDI output slots |
| `MonitorPage` | `ui/pages/MonitorPage.h/cpp` | Live MIDI message display |

## Change Guide

| Task | Where to look |
|---|---|
| Add a new MIDI output event type | `MidiOutput.h` `Event` enum + `MidiOutputEngine.cpp` event handling |
| Change MIDI channel mapping | `MidiOutput.h` `_channel` field + `MidiOutputPage.cpp` |
| Fix MIDI note-off timing | `MidiOutputEngine.cpp` note-off scheduling |
| Add MIDI learn to a new parameter | `MidiLearn.h/cpp` + `Routing.h` target enum + `RoutingEngine.cpp` |
| Fix USB MIDI connectivity | `src/platform/stm32/drivers/UsbMidi.h` + `Engine.cpp` USB MIDI handlers |
| Debug incoming MIDI messages | `MonitorPage.cpp` + `Engine.cpp` `onMidiMessage()` |
