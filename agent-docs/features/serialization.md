# Serialization

## Purpose
Binary serialization system for persisting project data to SD card and settings to internal flash. Versioned to allow forward/backward compatibility when the data format changes across firmware versions.

## Key Source Files

| Role | Path |
|---|---|
| Versioned writer | `src/core/io/VersionedSerializedWriter.h` |
| Versioned reader | `src/core/io/VersionedSerializedReader.h` |
| Raw writer | `src/core/io/SerializedWriter.h` |
| Raw reader | `src/core/io/SerializedReader.h` |
| Serialize helpers | `src/apps/sequencer/model/Serialize.h` |
| File header + magic | `src/apps/sequencer/model/FileDefs.h` |
| Flash writer | `src/apps/sequencer/model/FlashWriter.h` |
| Flash reader | `src/apps/sequencer/model/FlashReader.h` |

## Format Overview

### File Header (`FileDefs.h`)
Every project file begins with `FileHeader`:
```cpp
struct FileHeader {
    uint32_t magic;          // CONFIG_VERSION_MAGIC = 0xfadebabe
    uint8_t  versionMajor;
    uint8_t  versionMinor;
    uint8_t  versionRevision;
    char     name[NameLength]; // project name
    uint32_t checksum;
};
```
The body after the header is a versioned binary blob.

### `VersionedSerializedWriter`
Wraps `SerializedWriter` with version-tagged sections:
- `write<T>(tag, value)` — writes a tag (uint16) + value
- Tags allow the reader to skip unknown fields
- Nested structures write with a length prefix

### `VersionedSerializedReader`
Reads back versioned data:
- `read<T>(tag, value)` — if tag matches, reads value; otherwise skips
- Unknown tags are silently ignored (forward compatibility)
- Missing tags leave the value at its default (backward compatibility)

### `Serialize.h`
Provides convenience macros/helpers used throughout model `write()`/`read()` methods:
- `writeValue(writer, field)` — write a simple scalar
- `writeEnum(writer, field)` — write an enum as integer
- `readValue(reader, field)` — read a scalar, ignore if tag not found
- etc.

## Versioning Strategy
- Version is embedded in `FileHeader` (major.minor.revision from `Config.h`)
- Model classes do NOT check the header version themselves — they rely on tag-based skipping
- Breaking changes require new tags (old readers skip them, new readers see defaults for missing old tags)
- The `CONFIG_VERSION_MAGIC` constant (`0xfadebabe`) guards against loading non-project files

## Flash Storage (Settings)
Settings use a simpler format:
- `FlashWriter` writes raw bytes directly to STM32 flash sector 3
- `FlashReader` reads them back
- Settings do not use the versioned tag format — format changes require care

## Serialization Call Tree
```
FileManager::saveProject()
  → FileWriter (core/fs)
    → VersionedSerializedWriter
      → Project::write()
          → ClockSetup::write()
          → Track::write() × 8 → [NoteTrack|CurveTrack|...]::write()
              → NoteSequence::write() × 17 → Step data
          → Song::write()
          → PlayState::write()
          → Routing::write()
          → MidiOutput::write() × 16
          → UserScale::write() × 4
```

## Change Guide

| Task | Where to look |
|---|---|
| Add a field to an existing model | Add `write<T>(TAG, field)` + `read<T>(TAG, field)` in the model's `write()`/`read()` methods — choose a new unique tag value |
| Change the file magic/version | `Config.h` version constants + `FileDefs.h` |
| Fix corrupt file loading | `FileManager.cpp` checksum validation + `FileDefs.h` `FileHeader` |
| Make settings versioned | Replace `FlashWriter`/`FlashReader` with `VersionedSerializedWriter`/`Reader` in Settings load/save |
| Debug what's in a project file | Add temporary debug output in the model `read()` methods or inspect with a hex editor matching `FileHeader` format |
