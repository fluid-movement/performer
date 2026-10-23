# Arp Track

Generates arpeggiated CV + gate. A 7-bit **degree mask** selects which scale degrees are in the note
pool; a **rhythm** euclidean pattern decides when a note fires; a second **mod** euclidean pattern
modifies the rhythm hits (accent, mask, ratchet, hold); an **arp order** walks the pool across 1-4
octaves. There are no per-step values and no MIDI input. Current model: **Version46** (older
sequences reset to defaults on load; there is no migration).

Decisions behind this design (`prep knowledge find arp` and `prep knowledge find euclid`): degree
mask plus two euclidean generators; euclidean patterns use canonical form shared by engine and
display; the play mode was removed in Version49.

## Key source files

| Role | Path |
|---|---|
| Sequence data | `model/ArpSequence.{h,cpp}` |
| Track settings | `model/ArpTrack.{h,cpp}` |
| Engine | `engine/ArpTrackEngine.{h,cpp}` |
| Shared euclidean generator | `engine/EuclideanGenerator.h` (function `computeEuclidean`) |
| Edit page | `ui/pages/ArpSequenceEditPage.{h,cpp}` |
| Overview page | `ui/pages/ArpSequencePage.{h,cpp}` |
| Config list rows | `ui/model/ArpTrackListModel.h`, `ui/model/ArpSequenceListModel.h` |
| Clipboard | `model/ClipBoard.cpp` (`copyArpSequence`, `pasteArpSequence`) |
| Tests | `tests/ui/arp_v2_test.py` |

`model/Arpeggiator.h` and `engine/ArpeggiatorEngine.h` are **not** this track's arpeggiator: they are
the MIDI/CV track's ([midicv-track](midicv-track.md)). `ArpTrack` still embeds an `Arpeggiator`
and the config page still lists its rows, but the Arp engine never reads it.

## Data model

### `ArpSequence` (per pattern, 17 per track)

| Field | Range | Meaning |
|---|---|---|
| `degreeMask` | 7 bits | bit 0 = scale degree 1; default only degree 1 |
| `rhythmN` / `rhythmK` / `rhythmR` | 1-16 / 0-N / 0-15 | rhythm euclidean: steps, pulses, rotation (defaults 16, 5, 0) |
| `rhythmGateLen` | 0-19 | gate length `(v+1)*5 %` of the step, default 7 = 40% |
| `modN` / `modK` / `modR` | 1-16 / 0-N / 0-15 | mod euclidean (defaults 12, 3, 0) |
| `modMode` | enum | Off, Accent, Mask, Combine, Ratchet, Hold |
| `arpOrder` | 0-6 | UP, DOWN, UP-DN, DN-UP, RAND, CONV, DIV |
| `arpOctaves` | 1-4 | |
| `arpLength` | 1-64 | steps before the step counter and arp phase wrap |
| `divisor` (routable), `resetMeasure`, `name`, `slot` | | |

### `ArpTrack`

`fillMode`, `fillMuted`, `cvUpdateMode` (Gate / Always), `slideTime` (passed to the CV drain; the
engine never sets the slide flag, so it has no audible effect), `octave`, `transpose`, pattern
follow. Four probability biases, the embedded `arpeggiator` and a `midiKeyboard` flag exist but the
engine ignores them. There is no play mode: the engine only works aligned to the clock.

## Engine (`ArpTrackEngine`)

On each divisor boundary (`triggerStep`):

1. If any euclidean parameter changed, rebuild `_rhythmPat` / `_modPat` with `computeEuclidean`
   (Bresenham, canonical form: first hit at index 0, user rotation applied on top).
2. `rhythmHit = _rhythmPat[_step % rhythmN]`, `modHit = _modPat[_step % modN]`,
   `overlap = rhythmHit && modHit`. `_step` wraps at `arpLength` (and resets the arp phase).
3. No rhythm hit, or an empty note pool: nothing fires. The pool is rebuilt from `degreeMask`.
4. Mod mode: Accent doubles the gate on overlap; Mask suppresses the note on overlap; Combine fires
   only on overlap; Ratchet fires two half-length gates on overlap; Hold quadruples the gate on overlap.
5. `nextNoteCv()` picks the note: UP/DOWN period `n * octaves`; UP-DN/DN-UP bounce with period
   `2*(n-1)` per octave so endpoints do not repeat; RAND draws note and octave; CONV converges from
   the outside in; DIV diverges from the centre out.
6. Gate on/off and the CV go into the sorted queues with swing, and are drained in `tick()`.

A linked track (pattern follow / link) takes its step from the linked engine's `linkData`.

## UI

`ArpSequenceEditPage`, footer tabs **NOTE / RHYTHM / MOD / ARP**:

| Tab | Controls |
|---|---|
| NOTE | Step 0-6 toggle the degrees; the encoder does nothing |
| RHYTHM | hold Step 0-3 and turn the encoder: N, K, R, GATE LEN |
| MOD | hold Step 0-3: N, K, R, MODE |
| ARP | hold Step 0-2: ORDER, OCTAVES, LENGTH |

Page + Step 11 and Page + Step 12 quick-edit Divisor and ResetMeasure. The overview
page (`ArpSequencePage`) is a config-style list for the sequence.

## Change guide

| Task | Where |
|---|---|
| Add an arp order | `ArpSequence::setArpOrder` clamp and `arpOrderNames` in `model/ArpSequence.cpp`; a case in `ArpTrackEngine::nextNoteCv` |
| Add a mod mode | `ArpSequence::ModMode`; a case in `ArpTrackEngine::triggerStep` |
| Change the gate length formula | `ArpTrackEngine::triggerStep` (`rawLen`) and `printRhythmGateLen` |
| Add a sequence parameter | field and accessors in `model/ArpSequence.h`, `write`/`read`, new `ProjectVersion` ([serialization](serialization.md)), `python/project.cpp`, the edit page |
| Remove the dead track settings | `ArpTrack.{h,cpp}`, `ArpTrackListModel.h`; a version bump |

Tests: `tests/ui/arp_v2_test.py` (see [testing.md](../testing.md)); its euclidean helper mirrors
`computeEuclidean`.
