# MIDI/CV Track

Converts incoming MIDI into CV and gate in real time. Up to 8 polyphonic voices, selectable note
priority, pitch bend, velocity and pressure outputs, an optional arpeggiator and portamento for
mono. There is no sequence or step data: the track is driven entirely by MIDI input, not by the
tick.

This is the one track type without a Steps page. A design for it exists but is unbuilt:
[ui-redesign/midicv-page-brief.md](../ui-redesign/midicv-page-brief.md).

## Key source files

| Role | Path |
|---|---|
| Track settings | `model/MidiCvTrack.{h,cpp}` |
| Arpeggiator settings (also used elsewhere in the model) | `model/Arpeggiator.{h,cpp}` |
| Engine | `engine/MidiCvTrackEngine.{h,cpp}` |
| Arpeggiator engine | `engine/ArpeggiatorEngine.{h,cpp}` |
| Source matching (port + channel) | `engine/MidiUtils.h`, `engine/MidiPort.h` |
| Config list rows | `ui/model/MidiCvTrackListModel.h` |

## Data model (`MidiCvTrack`)

| Field | Meaning |
|---|---|
| `source` | `MidiSourceConfig`: MIDI port and channel to listen to |
| `voices` | 1-8 |
| `voiceConfig` | which signals each voice outputs: Pitch, Velocity, Pitch+Vel, Pitch+Vel+Press |
| `notePriority` | LastNote, FirstNote, LowestNote, HighestNote (decides which notes win voices) |
| `lowNote` / `highNote` | notes outside the range are ignored |
| `pitchBendRange` | 0-48 semitones |
| `modulationRange` | voltage range for velocity and pressure |
| `retrigger` | drop the gate for `RetriggerDelay` (2 system ticks) on a new note |
| `slideTime` | portamento, mono only (`voices == 1`) |
| `transpose` | semitones, routable |
| `arpeggiator` | enabled, hold, mode (PlayOrder, Up, Down, UpDown, DownUp, UpAndDown, DownAndUp, Converge, Diverge, Random), divisor, gate length, octaves |

## Engine (`MidiCvTrackEngine`)

1. `receiveMidi()` (called by `Engine::receiveMidi` for every track): drops notes outside
   `lowNote..highNote`, then for matching port and channel handles note on/off, key pressure,
   channel pressure and pitch bend. With the arpeggiator enabled, notes go to `ArpeggiatorEngine`
   instead and its events become voices; the arpeggiator also runs when the clock is stopped.
2. Voices: `addVoice` takes a free voice (or overrides the last), `sortVoices` orders active voices
   by note priority and allocates physical outputs round-robin, stealing the lowest priority
   output when all are busy. Mono with an existing active voice activates slide.
3. Outputs: gate `i` follows voice `i` (`!mute && active`, dropped briefly when retrigger is on).
   CV output `index` maps to `signal = index / voices`, `voice = index % voices`:
   - Pitch: `(note + transpose - 60) / 12` V plus pitch bend (`bend * range / (12 * 8192)`); for a
     single voice, the slid `_pitchCvOutput`
   - Velocity: `valueToCv(velocity)` in `modulationRange`
   - Pressure: key pressure plus channel pressure, in `modulationRange`
4. Slide uses the old one-pole `Slide::applySlide`, not the bounded `Slide::Ramp` the Note track
   uses; moving it to `Ramp` is an open TODO.

## Change guide

| Task | Where |
|---|---|
| Voice allocation or priority | `MidiCvTrackEngine::addVoice`, `sortVoices` |
| Add a voice signal or config | `MidiCvTrack::VoiceSignal`, `VoiceConfig`, `voiceConfigInfos` in `model/MidiCvTrack.cpp`, `cvOutput()` in the engine |
| Pitch bend range or scaling | `MidiCvTrack::pitchBendRange`, `MidiCvTrackEngine::pitchBendToCv` |
| MIDI channel or port filtering | `MidiUtils::matchSource`, `MidiCvTrack::source` |
| Arpeggiator behaviour | `engine/ArpeggiatorEngine.cpp` and `model/Arpeggiator.h` |
| Build a Steps-style page | the brief linked above, then [ui-system](ui-system.md) |
