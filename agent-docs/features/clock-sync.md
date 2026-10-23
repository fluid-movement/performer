# Clock & Sync

## Purpose
The master clock drives all sequence playback. It supports internal (master) mode, three external slave modes (analog, MIDI, USB-MIDI), and auto-detection. Tap tempo and tempo nudge allow live BPM adjustment. The clock outputs signals to analog clock output and MIDI clock.

## Key Source Files

| Role | Path |
|---|---|
| Clock engine | `src/apps/sequencer/engine/Clock.h/cpp` |
| Clock setup data model | `src/apps/sequencer/model/ClockSetup.h/cpp` |
| Tap tempo | `src/apps/sequencer/engine/TapTempo.h` |
| Tempo nudge | `src/apps/sequencer/engine/NudgeTempo.h` |
| Low-level timer driver | `src/platform/stm32/drivers/ClockTimer.h` |
| Tempo page | `src/apps/sequencer/ui/pages/TempoPage.h/cpp` |
| Clock setup page | `src/apps/sequencer/ui/pages/ClockSetupPage.h/cpp` |
| UI list model | `src/apps/sequencer/ui/model/ClockSetupListModel.h` |

## Data Model (`ClockSetup`)

- `_mode` — `ClockSetup::Mode`: Auto | Master | Slave
- `_slaveDivisor` — divisor when in analog slave mode (e.g. 1 = 1PPQN input)
- `_masterBpm` — master BPM (1–1000 float)
- `_clockInputMode` — which input acts as slave: Analog | MIDI | USB-MIDI
- `_clockOutputMode` — clock output: Disabled | Analog | MIDI | USB-MIDI
- `_clockOutputDivisor` — output clock divisor
- `_clockOutputPulse` — output clock pulse width (in ticks)
- `_clockOutputSwing` — swing applied to clock output

## `Clock` Class

```
Mode: Auto | Master | Slave
State: Idle | MasterRunning | SlaveRunning
```

Key methods:
- `masterStart()` / `masterStop()` / `masterContinue()` / `masterReset()` — playback control
- `setMasterBpm(float)` — change BPM
- `slaveConfigure(int slave, int divisor, bool enabled)` — configure external clock input
- `slaveTick(int slave)` — receive external clock pulse
- `slaveHandleMidi(int slave, uint8_t msg)` — receive MIDI clock byte
- `outputConfigure(int divisor, int pulse)` — configure clock output
- `outputConfigureSwing(int swing)` — set swing on output

Clock implements `ClockTimer::Listener` — `ClockTimer` fires interrupts at `CONFIG_PPQN` × BPM rate and calls back into `Clock`.

## Constants
```cpp
CONFIG_PPQN          192   // Master clock resolution (ticks per quarter note)
CONFIG_SEQUENCE_PPQN  48   // Sequence step resolution (ticks per quarter note)
CONFIG_TICK_FREQUENCY 1000 // System tick (1ms)
```

Sequence divisors relate steps to clock ticks. A divisor of `CONFIG_PPQN / CONFIG_SEQUENCE_PPQN = 4` means 4 master ticks per sequence tick.

## Engine Integration

`Engine` owns the `Clock` object and implements `Clock::Listener`:
- `onClockOutput(OutputState)` — sends analog clock pulses to `Dio`/`GateOutput`
- `onClockMidi(uint8_t)` — sends MIDI clock bytes to `Midi`/`UsbMidi`

On each `slaveTick`, `Clock` computes the incoming BPM using a `MovingAverage` filter and adjusts timing.

`TapTempo` accumulates button-press timestamps and calls `Clock::setMasterBpm()`.
`NudgeTempo` applies a temporary BPM offset for live swing/rush during performance.

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `TempoPage` | `ui/pages/TempoPage.h/cpp` | Display/edit BPM, tap tempo, nudge |
| `ClockSetupPage` | `ui/pages/ClockSetupPage.h/cpp` | Configure mode, divisors, output |

## Change Guide

| Task | Where to look |
|---|---|
| Change BPM range | `Project.h` `setTempo()` clamp (1–1000) + `ClockSetup.h` |
| Add a new clock source type | `Clock.h` `Mode` + `ClockSetup.h` `Mode` + `Engine.cpp` slave setup |
| Fix analog clock input jitter | `Clock.cpp` `MovingAverage` filter on slave BPM + `slaveTick()` |
| Change clock output pulse width | `Clock.cpp` `outputConfigure()` + `ClockTimer` driver |
| Fix tap tempo behavior | `src/apps/sequencer/engine/TapTempo.h` |
| Fix MIDI clock sync | `Clock.cpp` `slaveHandleMidi()` + `Engine.cpp` MIDI receive handler |
