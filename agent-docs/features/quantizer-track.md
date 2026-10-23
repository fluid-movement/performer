# Quantizer Track Mode

## Overview

`TrackMode::Quantizer` (serialization index 6) — a new track type that:

1. **Reads** an input CV voltage (hardware CV In 1–4, or another track's CV output)
2. **Quantizes** to the project's global scale via `Scale::noteFromVolts` / `Scale::noteToVolts`
3. **Holds** the quantized note until re-triggered
4. **Outputs** quantized CV on the track's CV jack; ~5ms pulse on the gate jack on each note change

## Feature Set

| Property | Options |
|---|---|
| Input source | CV In 1–4 (raw hardware ADC) or Track 1–8 (raw CV output) |
| Trigger mode | Free, Internal, External |
| Octave | ±10, applied after quantize |
| Transpose | ±100 scale degrees, applied after quantize |
| Gate lane | Full `NoteSequence` embedded in `QuantizerTrack` (gate field only used by engine) |

### Trigger Modes

- **Free**: fires whenever the quantized degree (`qNote`) changes from the previous tick
- **Internal**: fires on gated steps of the track's own embedded `NoteSequence` (runs like `LogicTrackEngine` internal mode, using `SequenceState` + `relativeTick`)
- **External**: fires on the rising edge of another track's `gateOutput(0)` (configurable via `triggerTrack`)

### Gate Lane

The gate lane is a standard `NoteSequence` reused from `NoteTrack`. Only the `step.gate()` field is honored; note values in the sequence are ignored. This gives full access to:
- First/last step, run mode, divisor, section navigation
- All shift-menu shortcuts (same muscle memory as Note tracks)

## File Map

| File | Purpose |
|---|---|
| `model/QuantizerTrack.h/.cpp` | Model: InputSource/TriggerMode enums, octave/transpose, embedded `NoteSequenceArray` |
| `engine/QuantizerTrackEngine.h/.cpp` | Engine: reads input, quantizes, fires, outputs CV/gate |
| `ui/model/QuantizerTrackListModel.h` | Track config list rows: input source, trigger mode, trigger track, octave, transpose, pattern follow |
| `ui/pages/QuantizerSequencePage.h/.cpp` | Sequence params page (first/last step, divisor, run mode, scale, root note) |
| `ui/pages/QuantizerSequenceEditPage.h/.cpp` | Gate step editor (gate toggle, section scroll, quick edit, shift menus) |

## Stability (IIR filter + hysteresis + delayed sampling)

Three techniques work together to eliminate jitter on hardware ADC:

| Technique | Constant | Value | Purpose |
|---|---|---|---|
| IIR low-pass | `InputFilterAlpha = 0.10` | ~26ms TC @120BPM | Smooths ADC noise before quantizing |
| Hysteresis | `HysteresisVolts = 0.012f` | ~14% of semitone | Prevents boundary flip in Free mode |
| Delayed sample | `SampleDelayTicks = 4` | ~10ms @120BPM | Lets externally-clocked CV settle before commit (Internal/External) |

Gate fires at *commit* time (after the settle delay), so CV and gate are always coherent.

**IIR and note-change latency**: all scale steps are ≥83mV; `FilterSnapVolts = 50mV`, so every pitch-changing input triggers the snap path and bypasses the IIR entirely. The IIR only affects sub-note ADC jitter. α=0.10 was chosen over 0.25 for better rejection of systematic mid-frequency interference (stable up to ±30mV p-p @ 33Hz vs ±10mV with α=0.25) with zero impact on musical response.

**Known noise floor**: if the CV input carries systematic noise ≥±30mV (peak) near a scale boundary at ~33Hz (e.g. power supply ripple), the IIR+hysteresis combination cannot guarantee stability. Hardware-side filtering of the CV input is recommended for noisy environments.

## Engine Logic (tick-level)

```
filteredV = IIR(readInput())   // always runs

switch triggerMode:
  Free:     candidate = noteFromVolts(filteredV)
            if |filteredV - _lastQVolts| >= HysteresisVolts (or first tick):
                commit(candidate)
  Internal: advance SequenceState
            if step.gate(): schedule sampleTick = tick + SampleDelayTicks
  External: curGate = trackEngine(triggerTrack).gateOutput(0)
            if curGate && !_lastSourceGate:
                schedule sampleTick = tick + SampleDelayTicks
            _lastSourceGate = curGate

if samplePending && tick >= sampleTick:
    commit(noteFromVolts(filteredV))
    samplePending = false

commit(qNote):
    _lastQNote = qNote
    _lastQVolts = noteToVolts(qNote)          // un-transposed, for hysteresis reference
    _cvOutput   = noteToVolts(qNote + transposition)
    _gateOutput = true
    _pulseTick  = tick + PulseLengthTicks     // CONFIG_PPQN / 24 = 8 ticks ≈ 21ms @120BPM

if _gateOutput && tick >= _pulseTick:
    _gateOutput = false
```

## Serialization

Added at `ProjectVersion::Version40 = 40`. All new fields guarded by `reader.dataVersionAtLeast(ProjectVersion::Version40)`.

`NoteSequence` declares `friend class QuantizerTrack` so `setTrackIndex` is accessible during pattern initialization.

## Verified by

`src/apps/sequencer/tests/verify_quantizer.py` — 9 headless tests (basic suite):
1. Free trigger quantizes to Ionian scale notes
2. Free trigger tracks input changes (0V → 1V)
3. External trigger fires on rising gate edge
4. Internal trigger fires on own gated steps
5. Octave +1 shifts output by ~1V
6. Different scales produce different outputs
7. Hysteresis: input parked at scale boundary does not flip
8. IIR filter: rapid ADC oscillation produces one stable output
9. Delayed sample: Internal trigger reads settled CV, not trigger-instant value

`src/apps/sequencer/tests/verify_quantizer_final.py` — 31 adversarial tests covering:
- Scale change mid-play (same degree index, back-and-forth, with transpose)
- External triggerTrack switch: no spurious trigger, independent sources
- Simultaneous quantizers (independent inputs and shared input)
- Round-trip accuracy: all Ionian degrees × 3 octaves
- Harmonic minor family scale quantization
- All 21 scales in rapid succession (Free mode)
- Internal: sample-pending across sequence reset, divisor change, fast divisor
- Gate pulse behavior: fires on every change, pulse expiry, re-trigger on same note
- Single-step Internal sequence looping
- Rapid transposition sign-flip
- Hysteresis dead zone (does not flip), sub-deadband from high note (stays put)
- IIR-only sub-snap convergence cannot cross a note boundary
- Rapid successive note changes: gate eventually drops after chain
- Input source switch mid-play: snap path fires immediately
- Self-route guard: own track input returns 0V
- External invalid triggerTrack (-1): no crash, no commit
- Internal all-gates-off: never commits
- Quantizer chained to quantizer (External): chain fires correctly
- Extreme octave=10: no crash, DAC clips at +5V rail
- Non-contiguous gated steps: only gated steps commit
- Negative octave: sub-zero CV correct
- Stop-and-restart: engine re-initializes without stale state
- Sequential External triggers: each trigger samples current CV independently
- Two External quantizers sharing same trigger track: independent _lastSourceGate per engine
- Free mode zero-crossing: negative and positive voltages quantize correctly
- +0.01V bias boundary: input at exactly -0.01V → 0V (deg0), -0.02V → deg6/oct-1
- Free mode refire via 'same degree index, different voltage' scale-change branch
- External mode switch from Free: HIGH gate absorbed, no spurious trigger
- Hysteresis _lastQVolts tracks latest committed note, not original
