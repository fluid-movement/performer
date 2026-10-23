# Launchpad Integration

Novation Launchpads connected to the USB host port act as an extra surface: an 8x8 pad grid plus
function and scene buttons, with colour LEDs mirroring sequencer state. The controller has three
modes (`Sequence`, `Pattern`, `Performer`) that reproduce the step, pattern and mute/fill workflows.

## Key source files

| Role | Path |
|---|---|
| Controller interface | `src/apps/sequencer/ui/Controller.{h,cpp}` (`ControllerInfo`) |
| Detection and dispatch | `src/apps/sequencer/ui/ControllerManager.{h,cpp}` |
| Launchpad logic (modes, buttons, drawing) | `src/apps/sequencer/ui/controllers/launchpad/LaunchpadController.{h,cpp}` |
| Device base class (generic) | `src/apps/sequencer/ui/controllers/launchpad/LaunchpadDevice.{h,cpp}` |
| Device variants | `LaunchpadMk2Device`, `LaunchpadMk3Device`, `LaunchpadProDevice`, `LaunchpadProMk3Device` (`.h/.cpp` in the same directory) |
| USB-MIDI | `src/platform/stm32/drivers/UsbMidi.h`, `UsbH.h`; connect handling in `engine/Engine.cpp` (`usbMidiConnect`) |

## How it works

- **Detection**: when a USB-MIDI device connects, `Engine::usbMidiConnect` calls the connect
  handler; `ControllerManager::findController(vendorId, productId)` matches a table in
  `ControllerManager.h` (vendor 0x1235): Launchpad S (0x0020), Mini Mk1 (0x0036), Mini Mk2 (0x0037),
  Mk2 (0x0069), Pro (0x0051), Mini Mk3 (0x0113), X (0x0103), Pro Mk3 (0x0123). `Engine` also sets a
  flag so the UI knows a Launchpad is connected (`isLaunchpadConnected`).
- **Device choice**: `LaunchpadController`'s constructor picks the device class by product id: Mk2
  (0x0069), Pro (0x0051), Mk3 (Mini Mk3 and X), Pro Mk3; anything else gets the generic
  `LaunchpadDevice`. Devices differ in how they set LEDs and enter programmer mode (SysEx).
- **Grid**: `LaunchpadDevice` is 8x8 pads plus a scene column (`SceneRow` 8) and a function row
  (`FunctionRow` 9). `setLed(row, col, color)` / `syncLeds()` push LED state; incoming pad presses
  come through `recvMidi` as button events.
- **Controller**: `update()` redraws for the current mode every UI tick; `recvMidi` translates pads
  into sequencer actions (toggle gates, select pattern, mute, fill, follow mode, run mode, first and
  last step). The button map is the `BUTTON(...)` table at the top of `LaunchpadController.cpp`.
  Controllers get MIDI before the engine (`Engine::receiveMidi` hands it to the receive handler
  first).
- `Track::patternFollow`'s display toggle (Page + Step 15) behaves differently when a Launchpad is
  connected.

## Change guide

| Task | Where |
|---|---|
| Support a new Launchpad model | add the vendor/product ids to `controllerInfos` in `ui/ControllerManager.h`, a device class deriving `LaunchpadDevice`, and a branch in the `LaunchpadController` constructor |
| Not detected | the id table in `ControllerManager.h`, `Engine::usbMidiConnect` |
| Wrong pad colours | the device's `setLed`, the colour mapping in `LaunchpadController.cpp` |
| Change what pads do | the mode's `*Button`/draw functions in `LaunchpadController.cpp` |
