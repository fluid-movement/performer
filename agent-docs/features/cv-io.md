# CV I/O

## Purpose
Manages analog control voltage inputs (4 channels, 12-bit ADC) and outputs (8 channels, 16-bit DAC). CV inputs feed the routing system for real-time modulation. CV outputs carry the sequenced pitch and modulation values to analog synthesizer modules. Also includes gate outputs (8 channels, digital).

## Key Source Files

| Role | Path |
|---|---|
| CV input engine | `src/apps/sequencer/engine/CvInput.h/cpp` |
| CV output engine | `src/apps/sequencer/engine/CvOutput.h/cpp` |
| Calibration data model | `src/apps/sequencer/model/Calibration.h/cpp` |
| System page (calibration UI) | `src/apps/sequencer/ui/pages/SystemPage.h/cpp` |
| ADC driver (STM32) | `src/platform/stm32/drivers/Adc.h` |
| DAC driver (STM32) | `src/platform/stm32/drivers/Dac.h` |
| Gate output driver (STM32) | `src/platform/stm32/drivers/GateOutput.h` |
| Digital I/O driver (STM32) | `src/platform/stm32/drivers/Dio.h` |

## Hardware Specs
- **ADC:** 12-bit, 4 channels (`CONFIG_ADC_CHANNELS = 4`, `CONFIG_CV_INPUT_CHANNELS = 4`)
- **DAC:** 16-bit, 8 channels (`CONFIG_DAC_CHANNELS = 8`, `CONFIG_CV_OUTPUT_CHANNELS = 8`) — TI DAC8568 via SPI
- **Gate outputs:** 8 digital outputs (`CONFIG_CHANNEL_COUNT = 8`) via 74HC595 shift registers

## `CvInput`
Reads the 4 ADC channels on every driver task tick (1ms):
- Applies a moving average filter to reduce noise
- Converts raw ADC counts to voltage (calibrated via `Calibration`)
- Exposes `voltage(int channel)` used by `RoutingEngine` to read CV source values

## `CvOutput`
Writes 8 DAC channels:
- `setVoltage(int channel, float volts)` — convert volts to 16-bit DAC counts using `Calibration`
- Applies 1V/octave scaling for pitch outputs
- Called by each `TrackEngine` at step output time

## Calibration (`Calibration`)
Per-channel calibration data corrects DAC/ADC non-linearity:
- `_cvInputCalibration[4]` — per-channel input calibration (offset + scale)
- `_cvOutputCalibration[8]` — per-channel output calibration (two-point: 0V and octave reference)

Calibration is run from `SystemPage` which drives known voltages through the DAC and reads them back via the ADC. Stored in `Settings` (flash, not project file).

## Gate Outputs
Gates are driven by `GateOutput` driver via `ShiftRegister` chain (74HC595):
- `GateOutput::set(int channel, bool active)` — set gate high/low
- Called by `TrackEngine` instances at step boundaries
- Gate pulse width is determined by note length + `CONFIG_PPQN` tick counting

## CV-to-Gate-to-MIDI
`CvGateToMidiConverter` (`engine/CvGateToMidiConverter.h`) monitors CV and gate inputs and converts them to MIDI note events — used when a hardware module output should be re-emitted as MIDI.

## Constants
```cpp
CONFIG_CV_INPUT_CHANNELS   4    // ADC input channels
CONFIG_CV_OUTPUT_CHANNELS  8    // DAC output channels
CONFIG_CHANNEL_COUNT       8    // tracks (and gate outputs)
```

## Change Guide

| Task | Where to look |
|---|---|
| Change voltage-to-DAC mapping | `CvOutput.cpp` voltage scaling + `Calibration.h` calibration data |
| Fix CV input noise | `CvInput.cpp` moving average filter window size |
| Add a new CV input channel | `Config.h` `CONFIG_CV_INPUT_CHANNELS` + `Adc.h` driver + `CvInput.cpp` |
| Change calibration procedure | `SystemPage.cpp` calibration UI + `Calibration.h/cpp` |
| Fix gate pulse width | `TrackEngine` implementations + `Config.h` `CONFIG_PPQN` |
| Fix DAC channel mapping | `CvOutput.cpp` channel routing + `Project.h` `_cvOutputTrack` array |
