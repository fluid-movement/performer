# Song Mode

## Purpose
An arrangement/song sequencer layered on top of pattern playback. A Song is a list of up to 64 slots, where each slot specifies which pattern each of the 8 tracks should play, per-track mutes, and a repeat count. Song mode plays these slots sequentially, enabling full song arrangement without manual pattern switching.

## Key Source Files

| Role | Path |
|---|---|
| Data model | `src/apps/sequencer/model/Song.h/cpp` |
| PlayState (song/pattern state) | `src/apps/sequencer/model/PlayState.h/cpp` |
| Engine state | `src/apps/sequencer/engine/EngineState.h` |
| Song page | `src/apps/sequencer/ui/pages/SongPage.h/cpp` |
| Song painter | `src/apps/sequencer/ui/painters/SongPainter.h/cpp` |

## Data Model

### `Song`
- `_slots` — `std::array<Slot, CONFIG_SONG_SLOT_COUNT>` (64 slots)
- `chain()` — the active chain (ordered list of slot indices for playback)

### `Song::Slot`
- `_patterns` — packed uint32: 4 bits per track × 8 tracks = pattern index 0–15 per track
- `_mutes` — uint8: one bit per track (mute mask)
- `_repeats` — uint8: how many times this slot loops before advancing (1–128)

Key slot accessors:
- `pattern(int trackIndex)` — returns pattern index for a track (0–15)
- `mute(int trackIndex)` — returns mute state
- `repeats()` — returns repeat count

### `PlayState`
Tracks the runtime playback position:
- Current pattern per track
- Current song slot index
- Mute state per track
- Fill state per track
- Pattern change requests (queued vs. immediate)

`PlayState` is modified by both the UI (user requests) and the engine (advancing slots).

## Engine Integration

`EngineState` (header-only) holds flags for song mode active/inactive and current slot position. On each measure boundary (when a pattern completes its `repeats`):
1. Engine advances `PlayState` to the next song slot.
2. New per-track pattern indices are loaded from `Song::Slot`.
3. Mute states from the slot are applied.
4. Track engines pick up the new patterns on the next pattern boundary.

```cpp
CONFIG_SONG_SLOT_COUNT  64   // max song slots
CONFIG_PATTERN_COUNT    16   // patterns per track (slot pattern index range)
```

## UI Page (`SongPage`)

- Displays the song slot list.
- Each slot row shows 8 pattern indices + mute indicators + repeat count.
- User navigates slots with the encoder, edits per-track patterns/mutes inline.
- Play/stop controls activate song mode.
- `SongPainter` handles the graphical rendering.

## Change Guide

| Task | Where to look |
|---|---|
| Change max song length | `Config.h` `CONFIG_SONG_SLOT_COUNT` + `Song.h` array size |
| Change slot repeat behavior | `Song.h` `Slot::setRepeats()` + `Engine.cpp` slot advance logic |
| Fix pattern advance timing | `Engine.cpp` measure boundary detection + `PlayState.cpp` |
| Add per-slot tempo | `Song::Slot` would need new field + serialization + engine tempo change |
| Fix mute state in song mode | `Song::Slot._mutes` + `PlayState.cpp` mute application |
