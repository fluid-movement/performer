# Quantizer Track

`TrackMode::Quantizer` (serialized id 6) reads a CV voltage, quantizes it to the project's global
scale, holds the result until the next trigger, and outputs it on the track's CV jack with a short
gate pulse on the gate jack at every commit. It also records and loops the notes it commits.
Current model: **Version48**.

Decisions behind this design (`prep knowledge find quantizer`): the five-tab layout and the unified
13-position trigger source; the always-on sequence clock and the loop rules; the input-stability
constants (IIR, hysteresis, sample delay) and why those values.

## Key source files

| Role | Path |
|---|---|
| Track settings | `model/QuantizerTrack.{h,cpp}` |
| Gate lane (a `NoteSequence`, only `gate` is used) | `model/NoteSequence.h` |
| Engine | `engine/QuantizerTrackEngine.{h,cpp}` |
| Overview page (config list) | `ui/pages/QuantizerSequencePage.{h,cpp}` |
| Edit page (5 tabs) | `ui/pages/QuantizerSequenceEditPage.{h,cpp}` |
| Config list rows | `ui/model/QuantizerTrackListModel.h` |
| Tests | `tests/ui/quantizer_loop_test.py` (maintained); `tests/verify_quantizer*.py` are older ad hoc scripts |

## Data model (`QuantizerTrack`)

| Field | Meaning |
|---|---|
| `inputSource` | CvIn1-4 (hardware CV inputs) or Track1-8 (another track's CV output 0); a track cannot read itself (reads 0 V) |
| `triggerMode` + `triggerTrack` | surfaced as one `TriggerSource` scroll of 13 positions: Free, External + T1..T8 (rising gate of that track), CvGate + CV1..CV4 (rising edge of a CV input, Schmitt thresholds 2.5 V / 1.0 V). `Internal` is deprecated: old data loads as Free |
| `octave` (-10..10), `transpose` (-100..100) | applied after quantizing, in scale degrees: `octave * notesPerOctave + transpose` |
| `loopLength` (1-16), `loopStart` (0-15) | the replay window into the 16-slot loop buffer (Version47) |
| `sequences` | 17 gate-lane `NoteSequence`s; first/last step, run mode, divisor, sections work as on a Note track |

Scale and root note are the project's; the quantizer has no per-track scale. The loop buffer and the
Play/Rec/Loop mode are engine runtime state, not stored in the project.

## Engine (`QuantizerTrackEngine::tick`)

Every tick:

1. `readInput()`: pick the source voltage, then the IIR filter (`InputFilterAlpha` 0.10; jumps over
   `FilterSnapVolts` 50 mV bypass it, so real pitch changes are instant and only sub-note jitter is
   smoothed).
2. A pending delayed sample commits first: after `SampleDelayTicks` (4 ticks, about 10 ms at
   120 BPM) so an externally clocked CV can settle.
3. **Sequence clock (always runs)**: on each divisor boundary `SequenceState::advanceAligned`
   moves the step so the GATE tab playhead always shows and other tracks can use this track's gate.
   For non-Free trigger modes (and for Free in Loop mode) a gated step, or any step in Loop mode,
   schedules a delayed sample. Free mode in Rec/Play does not use the clock to sample: hysteresis is
   its only commit path, which stops Rec double-filling.
4. Trigger mode:
   - **Free**: commit when the quantized note changes and the input moved at least
     `HysteresisVolts` (0.012 V) from the last committed note; also re-commit when the transposition
     or the scale changed under the current note. Skipped in Loop mode.
   - **External / CvGate**: on a rising edge schedule a delayed sample. Selecting a new trigger
     track or CV channel absorbs the current level so a HIGH input does not fire spuriously. In Loop
     mode these do not sample; the loop advances from the clock only.
5. **`commit(qNote)`**: remembers the un-transposed note and its volts (hysteresis reference). In
   Rec mode it stores the note in the loop buffer and switches to Loop when 16 are stored. In Loop
   mode the output is `loopBuffer[(loopStart + loopIndex) % 16]` and `loopIndex` advances modulo
   `loopLength`. The CV is `scale.noteToVolts(note + transposition)`, the gate goes high for
   `PulseLengthTicks` (`CONFIG_PPQN / 24` = 8 ticks, about 21 ms at 120 BPM), and CV and gate update
   together.

The loop buffer holds un-transposed notes, so octave and transpose edits apply live during playback.

## UI

`QuantizerSequenceEditPage`, footer tabs **GATE / SRCE / TRIG / TUNE / LOOP**.

| Tab | Display | Input |
|---|---|---|
| GATE | 16-step gate grid with playhead | step buttons toggle gates; step select and the encoder set the gate on a selection; Left/Right change section, with Shift they shift the steps; Page + Step 6 undoes |
| SRCE | two chip rows: CvIn1-4, Track1-8 | free-scroll encoder wraps over the 12 sources |
| TRIG | the 13 trigger positions | free-scroll encoder |
| TUNE | octave (left) and transpose (right), bipolar bars with numbers | hold Step 0 (octave) or Step 1 (transpose) and turn |
| LOOP | PLAY, REC, LEN, START | Step 0 = play, Step 1 = record; hold Step 2 (LEN) or Step 3 (START) and turn |

Step numbering in these docs is the code's: the first step button is Step 0 (hardware label 1).
Page + Step 8-12 quick-edit the gate lane's FirstStep, LastStep, RunMode, Divisor, ResetMeasure.
Selected chips are drawn as a Bright fill with the label knocked out via `BlendMode::Sub`
(never `Color::None`; see `prep knowledge find inverted`).

## Change guide

| Task | Where |
|---|---|
| Change stability behaviour | the constants in `engine/QuantizerTrackEngine.h` (see the stability decision first) |
| Add a trigger source | `QuantizerTrack::TriggerMode`, `triggerSourceIndex` / `setTriggerSourceIndex` / `printTriggerSource`, the `tick()` switch, the TRIG tab drawing |
| Change loop behaviour | `QuantizerTrackEngine::commit`, `setLoopMode`, `loopPlaySlot`; the LOOP tab in the edit page |
| Add a track field | `QuantizerTrack.{h,cpp}` `write`/`read` (new `ProjectVersion`, [serialization](serialization.md)), `python/project.cpp` |

Tests: `tests/ui/quantizer_loop_test.py` covers clock-driven, free and external record and loop,
the loop window, start offset, and clock-rate loop (see [testing.md](../testing.md)).
