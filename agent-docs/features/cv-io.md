# CV and Gate I/O

4 CV inputs (ADC), 8 CV outputs (DAC), 8 gate outputs (shift register). CV inputs feed routing,
Quantizer tracks, the CV/gate-to-MIDI converter and the monitor page. CV outputs carry each track's
pitch or modulation, and each output has its own calibration table.

## Key source files

| Role | Path |
|---|---|
| CV input | `engine/CvInput.{h,cpp}` |
| CV output | `engine/CvOutput.{h,cpp}` |
| Calibration (per output) | `model/Calibration.{h,cpp}`, stored in `model/Settings.h` |
| Track to jack mapping and test overrides | `engine/Engine.cpp` (`updateTrackOutputs`, `updateOverrides`), `model/Project.h` (`cvOutputTracks`, `gateOutputTracks`) |
| Calibration and utilities UI | `ui/pages/SystemPage.{h,cpp}`, mapping UI `ui/pages/LayoutPage.{h,cpp}` |
| Live values | `ui/pages/MonitorPage.{h,cpp}` |
| ADC / DAC / gate drivers (STM32) | `src/platform/stm32/drivers/Adc.h`, `Dac.h`, `GateOutput.h`, `ShiftRegister.h` |
| CV/gate to MIDI | `engine/CvGateToMidiConverter.h` ([midi-io](midi-io.md)) |

Channel counts: `CONFIG_CV_INPUT_CHANNELS` 4 and `CONFIG_CV_OUTPUT_CHANNELS` / `CONFIG_CHANNEL_COUNT`
8 in `src/apps/sequencer/Config.h`; `CONFIG_ADC_CHANNELS` 4 and `CONFIG_DAC_CHANNELS` 8 in
`src/SystemConfig.h`. The DAC is a DAC8568 (`Dac::Type`); outputs are 16-bit.

## `CvInput`

`update()` runs once per engine update and converts raw ADC counts to volts with
`5 - counts / 6553.5` (inverted input stage, about +5 V to -5 V). There is no input calibration or
filtering in this class; consumers that need stability filter themselves (the Quantizer does).
`channel(i)` returns volts.

## `CvOutput`

Tracks write volts with `setChannel(i, volts)` (1 V per octave, 0 V = C4); `update()` converts each
channel with `Calibration::CvOutput::voltsToValue` and writes the DAC. `channel(i)` reads the
last value back (used by routing and Quantizer sources).

## Calibration

`Calibration` holds one `CvOutput` table per output: `ItemCount` entries, one per volt from
`MinVoltage` (-5) to `MaxVoltage` (+5), each a 15-bit DAC value plus a user-defined flag. Values
between entries interpolate linearly; volts outside +-5 are clamped, which is why **no range above
5 V should be offered**. Entries not marked user-defined follow the ideal DAC/op-amp line
(0 -> +5.17 V, 32768 -> -5.25 V). Calibration lives in `Settings` (flash / SD backup), not in the
project. The SYSTEM page (Page + Track 7, behind a confirmation) edits it: F1 CAL, with AUTO resetting
an entry to the ideal value.

## Track to jack mapping

`Project::cvOutputTracks` and `gateOutputTracks` say which track feeds each of the 8 jacks (default:
track N to jack N; edited on the Layout page). `Engine::updateTrackOutputs()` walks the jacks and
asks the assigned track engine for `cvOutput(k)` / `gateOutput(k)`, with `k` counting how many jacks
that track already supplies. Most tracks supply one CV and one gate; a MIDI/CV track supplies one per
voice and signal ([midicv-track](midicv-track.md)). `SystemPage` overrides all outputs while it is open
(`Engine::setCvOutputOverride`), for calibration and tests.

## Gate outputs

`GateOutput::setGate(i, value)` drives the 74HC595 shift-register chain; gate timing is decided by
the track engines (queued gate on/off events in ticks), not by the driver.

## Change guide

| Task | Where |
|---|---|
| Voltage to DAC mapping | `Calibration::CvOutput::voltsToValue`, `defaultItemValue`, `CvOutput::update` |
| CV input scaling or noise | `CvInput::update` (add filtering here, or in the consumer) |
| Add an input channel | `Config.h`, `SystemConfig.h`, `Adc.h` driver, `CvInput` |
| Calibration procedure | `SystemPage.cpp` (calibration mode), `model/Calibration.{h,cpp}` |
| Jack assignment | `Project::cvOutputTracks`, `Engine::updateTrackOutputs`, `ui/pages/LayoutPage.cpp` |
