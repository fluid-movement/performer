# Clock and Sync

The master clock drives all playback. It runs internally (master), or follows one of three external
sources (analog clock input, MIDI clock, USB-MIDI clock), with auto-detection. It also produces the
analog clock/reset outputs and MIDI clock out. Tempo, tap tempo and tempo nudge feed it.

## Key source files

| Role | Path |
|---|---|
| Clock (master/slave, tick queue, outputs) | `engine/Clock.{h,cpp}` |
| Clock settings (persisted) | `model/ClockSetup.{h,cpp}` |
| Tempo, swing, time signature | `model/Project.h`, `model/TimeSignature.h` |
| Glue: setup, I/O handlers, tap, nudge, tick loop | `engine/Engine.{h,cpp}` |
| Tap tempo, tempo nudge | `engine/TapTempo.h`, `engine/NudgeTempo.h` |
| Swing math | `engine/Groove.h` |
| Hardware timer | `src/platform/stm32/drivers/ClockTimer.h` (simulator: `src/platform/sim/drivers/ClockTimer.h`) |
| Tempo page (modal) | `ui/pages/TempoPage.{h,cpp}` |
| Clock setup page | `ui/pages/ClockSetupPage.{h,cpp}`, `ui/model/ClockSetupListModel.h` |

## Data model

`ClockSetup` (a `ListPage` model; `dirty` makes the engine reconfigure):

| Field | Values |
|---|---|
| `mode` | Auto, Master, Slave |
| `shiftMode` | Restart, Pause (what Shift + Play does) |
| `clockInputDivisor`, `clockInputMode` | analog input divisor; Reset, Run, StartStop (what the reset input means) |
| `clockOutputDivisor`, `clockOutputSwing`, `clockOutputPulse`, `clockOutputMode` | output divisor; swing on/off; pulse width; Reset or Run on the reset output |
| `midiRx`, `midiTx`, `usbRx`, `usbTx` | MIDI clock in/out per port |
| `filterNote` | ignore MIDI note input when editing notes on the Note edit page |

Tempo (`Project::tempo`, 1-1000, routable) and swing (`Project::swing`, routable) are project
properties, not part of `ClockSetup`.

## `Clock`

`Mode` Auto/Master/Slave; `State` Idle, MasterRunning, SlaveRunning. Key calls:
`masterStart/Stop/Continue/Reset`, `setMasterBpm`, `slaveConfigure(slave, divisor, enabled)`,
`slaveTick/Start/Stop/Continue/Reset`, `slaveHandleMidi`, `outputConfigure(divisor, pulse)`,
`outputConfigureSwing`, `checkTick(&tick)`, `checkEvent()`. `Clock` listens to the `ClockTimer`
interrupt, queues ticks at `CONFIG_PPQN` (192 per quarter), and the engine task drains them, see
[architecture.md](../architecture.md). Slave BPM is derived from tick spacing and low-pass filtered.

## Engine glue (`Engine`)

- `initClock()` registers the digital I/O handlers: a rising clock input calls
  `slaveTick(ClockSourceExternal)` (and starts the clock on the first pulse in Reset mode); the reset
  input is interpreted per `clockInputMode`.
- `updateClockSetup()` (every update; full reconfigure when dirty) sets the mode, configures the
  three slaves (external: `clockInputDivisor * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN)`; MIDI and USB:
  `CONFIG_PPQN / 24`, enabled by `midiRx` / `usbRx`) and the outputs.
- `onClockOutput()` drives the clock and reset/run jacks; `onClockMidi()` sends MIDI clock bytes to
  the ports enabled by `midiTx` / `usbTx`.
- `update()` sets `masterBpm = tempo * (1 + nudge * 0.1)`; tap tempo calls `tapTempoTap()`.

A step is `divisor` ticks of the 192 PPQN clock (`CONFIG_SEQUENCE_PPQN` is 48). `Groove::applySwing`
shifts off-beat ticks.

## UI

`TempoPage` (Tempo key, modal, closes on release): encoder changes BPM by 1; Shift x10; pressing while
turning x0.1; Play taps tempo; Left/Right nudge the tempo while held; holding Performer switches the
encoder to swing. `ClockSetupPage` (Page + Tempo) is a config list with sections
([config-pages](config-pages.md)).

## Change guide

| Task | Where |
|---|---|
| BPM range | `Project::setTempo` clamp |
| New clock source | `Clock::Mode`, `ClockSetup::Mode`, `Engine::updateClockSetup` and I/O handlers |
| Analog clock jitter | `Clock.cpp` slave tick handling and BPM filter |
| Clock output pulse | `Clock::outputConfigure`, `ClockSetup::clockOutputPulse` |
| Tap tempo | `engine/TapTempo.h` |
| MIDI clock sync | `Clock::slaveHandleMidi`, `Engine.cpp` MIDI receive and `onClockMidi` |
