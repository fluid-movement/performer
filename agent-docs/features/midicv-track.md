# MIDI/CV Track

## Purpose
Converts incoming MIDI notes to CV+gate outputs in real time. Supports polyphony (up to 8 voices), voice allocation strategies, and can output pitch, velocity, and aftertouch pressure as separate CV values. No step sequencer — purely a MIDI-to-CV converter track.

## Key Source Files

| Role | Path |
|---|---|
| Data model | `src/apps/sequencer/model/MidiCvTrack.h/cpp` |
| Arpeggiator (optional) | `src/apps/sequencer/model/Arpeggiator.h/cpp` |
| Real-time engine | `src/apps/sequencer/engine/MidiCvTrackEngine.h/cpp` |
| MIDI port abstraction | `src/apps/sequencer/engine/MidiPort.h` |
| UI list model | `src/apps/sequencer/ui/model/MidiCvTrackListModel.h` |

## Data Model

### `MidiCvTrack` (track-level settings)
- `_source` — `MidiSourceConfig`: which MIDI port + channel to receive from
- `_voices` — number of polyphonic voices (1–8)
- `_voiceConfig` — `VoiceConfig` enum:
  - `Pitch` — one CV output per voice (pitch only)
  - `Velocity` — output velocity as CV
  - `PitchVelocity` — pitch + velocity on separate CV outs
  - `PitchVelocityPressure` — pitch + velocity + aftertouch
- `_notePriority` — `NotePriority` enum: LastNote | FirstNote | LowestNote | HighestNote
- `_lowNote`, `_highNote` — MIDI note range filter
- `_pitchBend` — pitch bend range in semitones
- `_modulationRange` — mod wheel CV output range
- `_retrigger` — retrigger behavior on note overlap
- `_arpeggiator` — embedded `Arpeggiator` config (optional arpeggio processing)
- `_transpose` — semitone offset

### `VoiceSignal` enum
`Pitch`, `Velocity`, `Pressure` — used internally to route signals to the right CV output channels.

## Engine Logic (`MidiCvTrackEngine`)

1. Subscribes to MIDI note-on/note-off/pitchbend/aftertouch messages on the configured port+channel.
2. Voice allocation: assigns incoming notes to voice slots respecting `voices` and `notePriority`.
3. For each active voice: compute CV pitch (1V/octave), apply `pitchBend`, `transpose`.
4. Optionally route velocity and pressure to additional CV outputs based on `voiceConfig`.
5. If `arpeggiator` is enabled: feed active notes into the `Arpeggiator`, drive CV+gate from arp output.
6. Write CV values to `CvOutput` and gate states to `GateOutput` for assigned channels.

This track does not use `SequenceState` or patterns — it is entirely event-driven by MIDI input.

## Change Guide

| Task | Where to look |
|---|---|
| Change voice allocation logic | `MidiCvTrackEngine.cpp` voice allocator section |
| Add a new voice signal type | `MidiCvTrack.h` `VoiceSignal`/`VoiceConfig` enums + `MidiCvTrackEngine.cpp` routing |
| Change pitch bend range behavior | `MidiCvTrack.h` `_pitchBend` + `MidiCvTrackEngine.cpp` pitch bend application |
| Fix MIDI channel filtering | `MidiCvTrackEngine.cpp` MIDI receive handler + `MidiSourceConfig` |
| Enable/disable arpeggiator on this track | `MidiCvTrack.h` `_arpeggiator` + `MidiCvTrackEngine.cpp` arp integration |
