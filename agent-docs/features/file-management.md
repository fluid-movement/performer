# File Management

Projects, user scales and single sequences are saved to a microSD card in numbered slots; the
settings (calibration and user settings) live in internal flash with an optional SD backup. The
formats are described in [serialization.md](serialization.md); this doc covers the storage and the
tasks around it.

## Key source files

| Role | Path |
|---|---|
| File manager (slots, read/write, tasks, volume state) | `model/FileManager.{h,cpp}` |
| File types and header | `model/FileDefs.h` |
| Settings and flash | `model/Settings.{h,cpp}`, `model/FlashReader.h`, `model/FlashWriter.h` |
| Filesystem layer | `src/core/fs/` (`FileSystem`, `File`, `FileReader`, `FileWriter`, `Directory`, `Volume`, `Error`) |
| FatFs library | `src/libs/ff/ff.h` |
| SD card driver (STM32) | `src/platform/stm32/drivers/SdCard.h` |
| Slot picker, busy screen, text entry | `ui/pages/FileSelectPage.{h,cpp}`, `ui/pages/BusyPage.{h,cpp}`, `ui/pages/TextInputPage.{h,cpp}` |
| Callers | `ui/pages/ProjectPage.cpp` (project save/load), `ui/pages/UserScalePage.cpp`, `ui/pages/SystemPage.cpp` (format, settings), `ui/pages/StartupPage.cpp` (auto-load) |

## Storage

- **SD card (FatFs)**: directories `PROJECTS` (`NNN.PRO`), `SCALES` (`NNN.SCA`) and `SEQS`
  (`NNN.NSQ`, `NNN.CSQ`, `NNN.ASQ`), 128 slots each (`FileSelectListModel` shows 128 rows). Slot
  numbering is 1-based on disk (`%03d`) and 0-based in code.
- **Last project**: `LAST.DAT` stores the slot of the last saved or loaded project;
  `StartupPage` reloads it at boot (`readLastProject`), which sets `Project::autoLoaded`.
- **Internal flash**: the settings, sector 3 at `0x0800C000` (`CONFIG_SETTINGS_FLASH_*` in
  `Config.h`). `Settings::writeToFlash` / `readFromFlash`; the SYSTEM page can also write them to or
  read them from an SD file.
- **Slot info**: `FileManager::slotInfo(type, slot, info)` returns `used` and the 8-character name
  from the file header, cached (`_cachedSlotInfos`) and invalidated on writes and card removal.

## `FileManager`

All static. Read/write pairs per type and slot (`writeProject(project, slot)`, `readProject`,
`writeUserScale`, `writeNoteSequence`, `writeCurveSequence`, `writeArpSequence` ...) plus path
overloads, `readLastProject`, `writeSettings` / `readSettings`, `format()`, `volumeAvailable()` /
`volumeMounted()`.

**File task**: long operations are queued with `FileManager::task(execute, result)` and run by
`FileManager::processTask()` on the file task (10 ms period, priority 1). `processTask` also polls
the card every second and mounts it. The UI pattern is: `Engine::suspend()`, show `BusyPage`, queue
the task, and in the result callback show a toast, close the busy page and `Engine::resume()`
(see `ProjectPage::saveProject`/`loadProject`).

## Change guide

| Task | Where |
|---|---|
| Add a file type | `FileType` in `FileDefs.h`, `fileTypeInfos` (keep index alignment), read/write pair in `FileManager`, a picker call |
| Change a stored format | [serialization.md](serialization.md) |
| SD detection or mount problems | `FileManager::processTask`, `src/core/fs/Volume.cpp`, `SdCard.h` |
| Corrupt file on load | `FileManager::readProject` result, `reader.checkHash()` in the model's `read` |
| Settings location | `CONFIG_SETTINGS_FLASH_SECTOR` / `ADDR` in `Config.h`, `Settings::writeToFlash` |
