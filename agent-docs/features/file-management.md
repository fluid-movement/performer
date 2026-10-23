# File Management

## Purpose
Handles saving and loading projects to/from MicroSD card, storing settings in internal STM32 flash, and managing the SD card file system. Provides the file browser UI.

## Key Source Files

| Role | Path |
|---|---|
| File manager | `src/apps/sequencer/model/FileManager.h/cpp` |
| File definitions (header format) | `src/apps/sequencer/model/FileDefs.h` |
| Flash storage (settings) | `src/apps/sequencer/model/FlashReader.h`, `FlashWriter.h` |
| VFS abstraction | `src/core/fs/FileSystem.h/cpp` |
| File I/O types | `src/core/fs/File.h/cpp`, `FileReader.h`, `FileWriter.h` |
| Directory ops | `src/core/fs/Directory.h` |
| Volume abstraction | `src/core/fs/Volume.h/cpp` |
| Serialization I/O | `src/core/io/VersionedSerializedReader.h`, `VersionedSerializedWriter.h` |
| File select page | `src/apps/sequencer/ui/pages/FileSelectPage.h/cpp` |
| Busy page | `src/apps/sequencer/ui/pages/BusyPage.h/cpp` |

## Architecture

### Storage Layers

**SD Card (FatFs)**
- FAT32 filesystem via `src/platform/stm32/libs/fatfs/`
- `Volume` wraps FatFs mount/unmount
- `FileSystem` provides path-based file operations
- Projects saved as binary files in a `/PROJECTS/` directory

**Internal Flash (Settings)**
- STM32 sector 3 at address `0x0800C000` (4KB)
- `FlashWriter`/`FlashReader` write/read raw binary
- Stores `Settings` object only (not full projects)

### `FileManager`
Orchestrates all file operations:
- `saveProject(Project&, int slot)` — serialize project to SD card slot file
- `loadProject(Project&, int slot)` — deserialize project from SD card
- `saveSettings(Settings&)` — write settings to flash
- `loadSettings(Settings&)` — read settings from flash
- `slotInfo(int slot)` — query file name, size, timestamp for a slot
- `formatDrive()` — format SD card
- File operations run in the `FILE_TASK` (priority 1) to avoid blocking the engine

### File Format (`FileDefs.h`)
Binary files begin with `FileHeader`:
- Magic number
- Version (major.minor.revision)
- Name (project name string, null-terminated)
- Checksum

The header is followed by versioned serialized data via `VersionedSerializedWriter/Reader`.

### Serialization Flow
```
Project::write(VersionedSerializedWriter&)
  → ClockSetup::write()
  → Track::write() × 8
    → NoteTrack::write() / CurveTrack::write() / ...
      → NoteSequence::write() × 17
  → Song::write()
  → Routing::write()
  → MidiOutput::write() × 16
  → UserScale::write() × 4
```

Each component is responsible for its own `write()`/`read()` methods. Versioned reader skips unknown fields for forward compatibility.

## Constants
```cpp
CONFIG_SETTINGS_FLASH_SECTOR  3
CONFIG_SETTINGS_FLASH_ADDR    0x0800C000
```

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `FileSelectPage` | `ui/pages/FileSelectPage.h/cpp` | Browse/select project slots, load/save/delete |
| `BusyPage` | `ui/pages/BusyPage.h/cpp` | Shown during file operations to block UI |

## Change Guide

| Task | Where to look |
|---|---|
| Add a new field to project files | Add to model class `write()`/`read()` methods + bump version if breaking |
| Change file storage location | `FileManager.cpp` path constants |
| Fix SD card detection | `src/platform/stm32/drivers/SdCard.h` + `Volume.cpp` |
| Fix file corruption on save | `FileManager.cpp` write + checksum validation in `FileDefs.h` |
| Add a new settings field | `src/apps/sequencer/model/Settings.h/cpp` `write()`/`read()` |
| Change flash storage sector | `Config.h` `CONFIG_SETTINGS_FLASH_SECTOR`/`CONFIG_SETTINGS_FLASH_ADDR` |
