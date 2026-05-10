# Arp Track (Arpeggiator Track)

## Purpose
A track that generates arpeggiated CV+gate sequences. Notes are fed into an `Arpeggiator` object which produces stepped output according to arp pattern, octave range, and gate settings. Can receive notes from MIDI input or from a static note set defined in the sequence.

## Key Source Files

| Role | Path |
|---|---|
| Data model (track settings) | `src/apps/sequencer/model/ArpTrack.h/cpp` |
| Data model (sequence + steps) | `src/apps/sequencer/model/ArpSequence.h/cpp` |
| Arpeggiator logic | `src/apps/sequencer/model/Arpeggiator.h/cpp` |
| Real-time engine | `src/apps/sequencer/engine/ArpTrackEngine.h/cpp` |
| Sequence display page | `src/apps/sequencer/ui/pages/ArpSequencePage.h/cpp` |
| Sequence edit page | `src/apps/sequencer/ui/pages/ArpSequenceEditPage.h/cpp` |
| UI list model | `src/apps/sequencer/ui/model/ArpTrackListModel.h` |

## Data Model

### `ArpTrack` (track-level settings)
- `_playMode` — `Types::PlayMode`
- `_fillMode` — `FillMode` enum: None | Gates | NextPattern | Condition
- `_fillMuted` — mute on fill
- `_cvUpdateMode` — Gate | Always
- `_slideTime` — portamento time (routable)
- `_octave` — octave offset (routable)
- `_transpose` — semitone transpose (routable)
- `_rotate` — step rotation (routable)
- `_gateProbabilityBias`, `_retriggerProbabilityBias`, `_lengthBias`, `_noteProbabilityBias` — global biases (all routable)
- `_sequences` — array of 17 `ArpSequence`

### `Arpeggiator` (embedded in ArpTrack or ArpSequence)
Defines the arpeggio behavior:
- `_hold` — hold mode (sustain active notes)
- `_mode` — arp pattern: Up, Down, UpDown, DownUp, UpAndDown, DownAndUp, Converge, Diverge, Random, ...
- `_divisor` — clock divisor for arp rate
- `_gateLength` — gate length for each arp note
- `_octaves` — number of octaves to span

### `ArpSequence::Step` layers
Same structure as NoteSequence step layers (Gate, GateProbability, Retrigger, Length, Note, NoteVariationRange, NoteVariationProbability, Condition, StageRepeats, StageRepeatsMode, Slide, BypassScale).

## Engine Logic (`ArpTrackEngine`)

1. Accept note input: either from MIDI (live played notes) or from static step notes in the sequence.
2. Pass the active note set into the `Arpeggiator` engine.
3. On each clock tick (divided by `arp.divisor`): advance the arp position, get the next note from the Arpeggiator's pattern.
4. Apply octave range expansion to cycle through octaves.
5. Apply transpose, scale mapping.
6. Output CV + gate at the arp rate.

The `Arpeggiator` class (`model/Arpeggiator.h/cpp`) contains the arp ordering algorithms (Up/Down/etc.) and is shared between `ArpTrack` and `MidiCvTrack`.

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `ArpSequencePage` | `ui/pages/ArpSequencePage.h/cpp` | Step overview |
| `ArpSequenceEditPage` | `ui/pages/ArpSequenceEditPage.h/cpp` | Per-step editing |

## Change Guide

| Task | Where to look |
|---|---|
| Add a new arp pattern mode | `src/apps/sequencer/model/Arpeggiator.h` — mode enum + `Arpeggiator.cpp` — pattern generation |
| Change octave expansion behavior | `Arpeggiator.cpp` octave cycling logic |
| Fix MIDI note input to arp | `ArpTrackEngine.cpp` MIDI note receive handler |
| Add new track-level parameters | `ArpTrack.h` properties section + serialization in `ArpTrack.cpp` |
