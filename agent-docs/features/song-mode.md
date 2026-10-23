# Song Mode

An arrangement layer on top of patterns. A `Song` is a list of up to 64 slots; each slot gives a
pattern for each of the 8 tracks, per-track mute flags and a repeat count. Playing the song steps
through the slots, switching all tracks' patterns at measure boundaries.

The song page design is deferred in the UI redesign (`prep knowledge find song`); it keeps its
current structure.

## Key source files

| Role | Path |
|---|---|
| Song and slots | `model/Song.{h,cpp}` |
| Song position, play/stop requests | `model/PlayState.{h,cpp}` (`SongState`) |
| Advance logic | `engine/Engine.cpp` (`updatePlayState`, `startSong`, `stopSong`) |
| Page and painter | `ui/pages/SongPage.{h,cpp}`, `ui/painters/SongPainter.{h,cpp}` |

## Data model

`Song`: `std::array<Slot, CONFIG_SONG_SLOT_COUNT>` (64) and `slotCount`. Operations: `chainPattern`,
`insertSlot`, `removeSlot`, `duplicateSlot`, `swapSlot`, `setPattern` (all tracks, or one),
`editPattern`, `setMute` / `toggleMute`, `setRepeats` / `editRepeats`, `trackHasMutes`.

`Song::Slot`: `patterns` (4 bits per track, 0-15), `mutes` (bit per track) and `repeats` (1-128).

`PlayState::SongState` (persisted with the project): `playing`, `currentSlot`, `currentRepeat`,
`requestedSlot`, and immediate / synced / latched play and stop requests.

## Engine

`Engine::updatePlayState`:

- A play request (`PlayState::playSong(slot, executeType)`) loads the slot (`activateSongSlot`: sets
  each track's pattern, and sets mutes only for tracks that have any song mutes), sets the current
  slot and repeat 0, starts the clock if it is stopped.
- On each measure boundary (`tick % measureDivisor() == 0`) while playing: when
  `currentRepeat + 1 >= repeats` the song moves to the next slot (wrapping to slot 0), activates it
  and restarts all track engines; otherwise it increments the repeat. A program change for the next
  slot is sent when MIDI program changes are enabled and all tracks share one pattern.
- The song stops when the slot index becomes invalid, or when the user changes patterns by hand.
- `startSong` / `stopSong` tie the song to the transport when the user setting `SyncSong` is on.

## UI (`SongPage`, Page + Step 3)

Footer **CHAIN / ADD (INSERT with Shift) / REMOVE / DUPL / PLAY (STOP)**. Context menu: INIT.

| Input | Effect |
|---|---|
| Left / Right, encoder | move the selected slot; with Shift on Left/Right swap with the neighbour |
| Shift + encoder | edit the slot's repeats |
| Step N | set the pattern for all tracks in the slot (N = pattern 0-15) |
| hold Track T + Step N | set track T's pattern in the slot; hold Track T + encoder edits it |
| hold Track T + encoder press | toggle track T's mute in the slot |
| Shift + Step N | set repeats to N + 1 |
| F1 held + Step N (CHAIN) | append a slot for pattern N and start playing |
| PLAY / encoder press | play from the selected slot (Shift: synced to the next boundary when the clock runs) |

## Change guide

| Task | Where |
|---|---|
| Song length | `CONFIG_SONG_SLOT_COUNT` in `Config.h` and the `Song` slot array |
| Repeat behaviour | `Song::Slot::setRepeats`, the advance block in `Engine::updatePlayState` |
| Pattern advance timing | `Engine::updatePlayState` (`handleSongAdvance`), `PlayState` |
| Per-slot tempo | a new `Slot` field, serialization ([serialization](serialization.md)) and an engine tempo change |
