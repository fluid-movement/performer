# Serialization and Versioning

Projects, user scales, single sequences and the settings are saved as **positional binary blobs**
with a single global version number and a trailing checksum. There are no field tags: a reader must
consume exactly the bytes the writer produced for that version. Compatibility is built from
version-gated reads. The rules are a decision (`prep knowledge find serialization`, "Breaking model
changes bump ProjectVersion and migrate in read()").

## Key source files

| Role | Path |
|---|---|
| Writer and reader | `src/core/io/VersionedSerializedWriter.h`, `src/core/io/VersionedSerializedReader.h` |
| Array helpers | `model/Serialize.h` (`writeArray`, `readArray`) |
| Version history | `model/ProjectVersion.h` |
| File header and types | `model/FileDefs.h` |
| Save/load orchestration | `model/FileManager.cpp` |
| Settings (flash and SD) | `model/Settings.{h,cpp}`, `model/FlashWriter.h`, `model/FlashReader.h` |
| Python save at an older version | `python/project.cpp` (`Project.save(filename, version)`) |

## File layout

```
FileHeader   (packed, 10 bytes): FileType (1), version byte (written as 0), name (8)
uint32       data version  (the writer's version, written by VersionedSerializedWriter's constructor)
...          the model, field by field, in a fixed order
uint32       FNV hash of every byte written between the version and here (writeHash)
```

`FileType`: Project 0, UserScale 1, NoteSequence 2, CurveSequence 3, ArpSequence 5, Settings 255.
Files live on the SD card as `PROJECTS/NNN.PRO`, `SCALES/NNN.SCA`, `SEQS/NNN.NSQ|CSQ|ASQ` (slot 4 was
the removed LogicSequence and is kept for index alignment).

## Writer and reader API

- `write(value)` writes `sizeof(value)` raw bytes and feeds the hash. `writeEnum(e, serializeFn)`
  writes a stable id from a `switch`, so enum reordering cannot corrupt files (`Track::TrackMode`,
  `Routing::Target`). `writeHash()` ends the blob.
- `read(value, addedInVersion = 0)` reads only when `dataVersion() >= addedInVersion`; otherwise
  `value` keeps its `clear()` default. `readEnum`, `readAs<ReadT>(value, addedInVersion)` (read a
  narrower stored type into a wider field) follow the same rule.
- `skip<T>(addedInVersion, removedInVersion)` / `skip(len, added, removed)` consume bytes of a field
  that existed only in `[added, removed)`. Use it for removed fields in the middle of a blob.
- `readerVersion()` is `ProjectVersion::Latest` when loading; `writerVersion()` on the writer lets
  `write()` emit older formats (the Python harness saves with an explicit `version` to test
  migrations). Model `write()` methods therefore have `writerVersion()` branches that write zeroed
  placeholders for fields older formats still expect (Note, Curve).
- `backupHash()` / `restoreHash()` and `checkHash()`: a few track `read()`s back up the hash
  and restore it for formats older than Version23; `checkHash()` compares the stored hash.

`FileManager::readProject` constructs the reader with `ProjectVersion::Latest`, reads, and returns
`fs::INVALID_CHECKSUM` when `checkHash()` fails (the project is then cleared). A file written by a
newer firmware is not rejected up front: gates for its newer fields pass, the layout no longer
matches and the checksum normally fails.

## Call tree (project)

```
FileManager::writeProject -> FileHeader + VersionedSerializedWriter(ProjectVersion::Latest)
  Project::write: name, tempo, swing, time signature, sync measure, scale, root note, monitor/record/
    MIDI settings, cvGateInput, curveCvInput
    ClockSetup::write
    Track::write x8          trackMode (writeEnum), linkTrack, then the active type's write()
      NoteTrack/CurveTrack/... write -> sequences x17 -> steps
    cvOutputTracks, gateOutputTracks
    Song::write, PlayState::write, Routing::write, MidiOutput::write
    UserScale x4
    selected track and pattern, resetCvOnStop, useMultiCv
    writeHash
```

## Versions

`ProjectVersion` has an entry per format change with a comment saying what changed; `Latest` is
derived. Current latest is **Version54** (Note V2). Recent history: 40-48 Quantizer, 41 per-sequence
scale removed, 44-45 Stochastic V2, 46 Arp V2, 49 Arp play mode removed, 50-53 Curve V1 refinements,
54 Note V2.

Settings use their own constant, `Settings::Version` (currently 1), and a different header
(`SETTINGS`). They live in internal flash (`CONFIG_SETTINGS_FLASH_SECTOR` 3 at
`CONFIG_SETTINGS_FLASH_ADDR` 0x0800C000 in `Config.h`, via `FlashWriter`/`FlashReader`) and can be
backed up to or restored from an SD file on the SYSTEM page. Changing the settings layout needs a
`Settings::Version` bump and gated reads in the same way.

## Adding or changing stored data

1. Add a `VersionN` entry (with a comment) to `model/ProjectVersion.h`.
2. New field: write it unconditionally in `write()`, read it with `reader.read(field, VersionN)`,
   and give it its default in `clear()`.
3. Removed field: keep it in `write()` only if older versions must still be emitted (placeholder),
   read it with `reader.skip<T>(0, VersionN)` at its old position. A removed `Routable<T>` was
   written whole (both values), not `.base`.
4. Changed meaning: convert in `read()` when `dataVersion() < VersionN` (see the Version50 curve
   rescale in `CurveSequence::Step::read`, the Version53 to 54 Note migration).
5. Bind new fields in `python/project.cpp` and add a migration test: save with the previous version
   number, load, assert ([testing.md](../testing.md)).
6. A clean break without migration is acceptable before outside users (Curve V1, Stochastic V2 and
   Arp V2 did that) but say so in the change.
7. Routing targets and track modes use stable ids: removing one needs no migration, and unknown ids
   read back as the first value (`Target::None`, `TrackMode::Note`).

## Change guide

| Task | Where |
|---|---|
| Add or remove a model field | the owner's `write`/`read` and `clear`, `model/ProjectVersion.h` |
| Debug a load failure | `FileManager::readProject` result, the owner's `read()` gates, `reader.checkHash()` |
| Change file naming or slot layout | `fileTypeInfos` and `slotPath` in `model/FileManager.cpp`, `model/FileDefs.h` |
| Change settings layout | `Settings::write`/`read`, `Settings::Version`, `Calibration`/`UserSettings` write/read |
