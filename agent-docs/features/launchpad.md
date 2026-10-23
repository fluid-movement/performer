# Launchpad Integration

## Purpose
Support for Novation Launchpad MIDI controllers connected via USB host. The Launchpad provides a 8×8 pad grid + buttons as an alternative hardware interface, with color-coded LEDs for visual feedback. Four Launchpad variants are supported.

## Key Source Files

| Role | Path |
|---|---|
| Controller manager | `src/apps/sequencer/ui/controllers/launchpad/LaunchpadController.h/cpp` |
| Base device class | `src/apps/sequencer/ui/controllers/launchpad/LaunchpadDevice.h/cpp` |
| Launchpad MK2 | `src/apps/sequencer/ui/controllers/launchpad/Mk2Device.h/cpp` |
| Launchpad MK3 | `src/apps/sequencer/ui/controllers/launchpad/Mk3Device.h/cpp` |
| Launchpad Pro | `src/apps/sequencer/ui/controllers/launchpad/ProDevice.h/cpp` |
| Launchpad Pro MK3 | `src/apps/sequencer/ui/controllers/launchpad/ProMk3Device.h/cpp` |
| Controller interface | `src/apps/sequencer/ui/Controller.h/cpp` |
| Controller manager (top-level) | `src/apps/sequencer/ui/ControllerManager.h/cpp` |
| USB MIDI driver | `src/platform/stm32/drivers/UsbMidi.h` |

## Architecture

### Device Detection
When a USB MIDI device connects, `Engine` fires `UsbMidiConnectHandler` with vendor/product IDs. `ControllerManager` matches IDs against known Launchpad product IDs and instantiates the appropriate `XxxDevice`.

Known Launchpad USB IDs are hardcoded in the device files (e.g. `Mk2Device.cpp`).

### `LaunchpadDevice` (base class)
Defines the interface for all Launchpad variants:
- `connect()` / `disconnect()` — called on USB connect/disconnect
- `update()` — called on every UI tick; reads page state, updates pad LEDs
- `onMidi(MidiMessage)` — receives MIDI from the Launchpad (pad presses)
- `setLed(row, col, color)` — set a pad's LED color
- Translates pad presses into `KeyEvent`/`EncoderEvent` for `Ui`

### `LaunchpadController`
Instantiates the correct device type and forwards events between it and the `Ui` system.

### Per-Variant Differences
| Variant | File | Notes |
|---|---|---|
| MK2 | `Mk2Device.h/cpp` | SysEx LED color, 8×8 pads |
| MK3 | `Mk3Device.h/cpp` | SysEx LED color, slightly different layout |
| Pro | `ProDevice.h/cpp` | Larger grid (10×10 including side buttons), extended SysEx |
| Pro MK3 | `ProMk3Device.h/cpp` | Latest Pro generation, uses different SysEx format |

Each variant has its own `VendorId`/`ProductId` constants for USB detection.

### LED Feedback
Pages call `LedPainter` which knows about the Launchpad grid layout. `LedPainter` translates internal LED indices to Launchpad pad coordinates and calls `setLed()` on the device.

### MIDI Communication
The Launchpad communicates over USB MIDI. Pad LED colors are set via SysEx messages (device-specific format). Pad presses arrive as MIDI note-on/note-off. Mode switching (programmer mode) is set via SysEx on connect.

## Change Guide

| Task | Where to look |
|---|---|
| Add support for a new Launchpad model | Create `XxxDevice.h/cpp` extending `LaunchpadDevice`, add USB VID/PID, register in `ControllerManager.cpp` |
| Fix Launchpad not detected on connect | `ControllerManager.cpp` USB connect handler + VID/PID constants in device files |
| Fix wrong pad colors | `LedPainter.cpp` Launchpad coordinate mapping + `XxxDevice.cpp` `setLed()` SysEx format |
| Fix pad input not working | `XxxDevice.cpp` `onMidi()` handler — check note-on/off mapping to KeyEvents |
| Change what pads do in a page | The page's `updateLeds()` and `keyDown()` methods, plus `LedPainter` helper mappings |
