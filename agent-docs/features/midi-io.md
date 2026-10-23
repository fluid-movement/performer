# MIDI I/O

MIDI arrives from the DIN port, USB-MIDI (host) and a virtual CV/gate port; it feeds routing, MIDI
learn, MIDI/CV tracks, note monitoring and recording, and program changes. MIDI leaves through 16
configurable output slots, plus MIDI clock ([clock-sync](clock-sync.md)).

## Key source files

| Role | Path |
|---|---|
| Output slots (persisted) | `model/MidiOutput.{h,cpp}`, `model/MidiConfig.h` (port + channel) |
| Output engine | `engine/MidiOutputEngine.{h,cpp}` |
| MIDI learn | `engine/MidiLearn.{h,cpp}` |
| CV/gate input to MIDI notes | `engine/CvGateToMidiConverter.h` |
| Ports | `engine/MidiPort.h` (`Midi`, `UsbMidi`, `CvGate`), `engine/MidiUtils.h` (`matchSource`) |
| Message type and parser | `src/core/midi/MidiMessage.{h,cpp}`, `src/core/midi/MidiParser.{h,cpp}` |
| Receive path and monitoring | `engine/Engine.cpp` (`receiveMidi`, `monitorMidi`) |
| Output page, monitor page | `ui/pages/MidiOutputPage.{h,cpp}`, `ui/pages/MonitorPage.{h,cpp}` |
| Drivers | `src/platform/stm32/drivers/Midi.h`, `UsbMidi.h`, `UsbH.h` |

## Input path (`Engine::receiveMidi`)

1. Drain the DIN and USB-MIDI receive queues; real-time and system messages are dropped (clock is
   handled by `Clock`). Also derive virtual notes from CV/gate inputs when `Project::cvGateInput`
   is `Cv1Cv2` or `Cv3Cv4` (port `CvGate`).
2. The UI controller handler (Launchpad) may consume the message first; only cable 0 continues.
3. Program changes select that pattern on all tracks when `Project::midiIntegrationMode` is
   ProgramChanges or Malekko (patterns wrap modulo 16), and are forwarded to the output.
4. `MidiLearn` sees the message (not from `CvGate`), then `RoutingEngine::receiveMidi`, then every
   track engine's `receiveMidi` (only MIDI/CV tracks consume).
5. Unconsumed messages go to **monitoring**: per `Project::midiInputMode` (Off, All, Source) the
   selected track's engine gets note on/off through `monitorMidi`, which is how notes are
   auditioned and recorded (`StepRecorder`).

## Output (`MidiOutputEngine`, `MidiOutput`)

Each `MidiOutput::Output` has a `target` (port + channel), an `event` (None, Note, ControlChange)
and sources:

- **Note**: `gateSource` (a track's gate), `noteSource` (a track's CV, or a fixed note 0-127),
  `velocitySource` (a track, or fixed 0-127).
- **ControlChange**: a `controlSource` (a track) and a control number.

Track engines push `sendGate(track, gate)`, `sendCv(track, cv)` and `sendSlide(track, slide)`;
`update()` turns them into messages. Tied notes (same note on and off in one update) are not
retriggered. Slide is sent as CC 65. CC output is rate-limited to about 50 events per second per
output. Program changes can be sent (`sendProgramChange`) and the engine includes Malekko Select
handshake messages. Changing an output's event or target resets its state.

## MIDI learn

`MidiLearn::start(callback)` calls back with the next message's port, channel, event and control
number or note; `RoutingPage` uses it to fill a route's MIDI source ([routing](routing.md)).

## UI

- `MidiOutputPage` (Page + Track 3): config list of the 16 slots, footer PREV / NEXT / INIT /
  COMMIT, same scratch-copy-and-commit flow as the routing page.
- `MonitorPage` (Page + Step 7): footer **CV IN / CV OUT / MIDI / STATS**; shows live CV, the last MIDI
  messages and task statistics.
- Project-level MIDI settings (input mode and source, integration, program offset) are on
  `ProjectPage`.

## Change guide

| Task | Where |
|---|---|
| New output event type | `MidiOutput::Output::Event` and its data union, `MidiOutputEngine::update`, the output list model |
| Note-off timing | `MidiOutputEngine.cpp` note request handling |
| Add MIDI learn to a parameter | `MidiLearn.{h,cpp}` result, `RoutingPage::assignMidiLearn` |
| New MIDI route event | `Routing::MidiSource::Event`, `RoutingEngine::receiveMidi` |
| USB MIDI connectivity | `src/platform/stm32/drivers/UsbMidi.h`, `UsbH.h`, the connect handler in `Engine.cpp` |
| Monitoring or recording | `Engine::monitorMidi`, the track engines' `monitorMidi`, `engine/StepRecorder.h` |
