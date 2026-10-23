# UI System

A page-based UI on a 256x64 OLED with 32 bi-colour LEDs (8 track, 16 step, 8 global buttons), 5
function keys (no LEDs) and one encoder. `Ui` owns a `PageManager` (a stack of pages), a
`ControllerManager` (Launchpad) and a `MessageManager` (toasts). The UI task polls every
millisecond and redraws at the active page stack's fps (`CONFIG_DEFAULT_UI_FPS` = 50).

Design rules (brightness, type, footer, LEDs) are in
[ui-redesign/01-design-language.md](../ui-redesign/01-design-language.md) and the matching prep
decisions (`prep knowledge find ui`). The map of pages and key combos is [ui-state.md](../ui-state.md).

## Key source files

| Role | Path |
|---|---|
| UI loop | `src/apps/sequencer/ui/Ui.{h,cpp}` |
| Page stack | `src/apps/sequencer/ui/PageManager.{h,cpp}` |
| Page base | `src/apps/sequencer/ui/Page.{h,cpp}`, `ui/pages/BasePage.{h,cpp}` |
| Page registry (`struct Pages`) | `src/apps/sequencer/ui/pages/Pages.h` |
| Root page, page-select keys | `ui/pages/TopPage.{h,cpp}`, `ui/PageKeyMap.h` |
| Events, keys, matrix | `ui/Event.h`, `ui/Key.h`, `ui/MatrixMap.h`, `ui/KeyPressEventTracker.h` |
| Step selection (hold / persist) | `ui/StepSelection.h` |
| Shared Steps-page behaviour | `ui/pages/SequenceEditPageBase.{h,cpp}` |
| Painters | `ui/painters/WindowPainter.h`, `SequencePainter.h`, `SongPainter.h`, `PreviewPainter.h` (each with a `.cpp`) |
| LEDs | `ui/LedPainter.{h,cpp}`, `ui/Leds.h` |
| Toasts, screensaver | `ui/MessageManager.{h,cpp}`, `ui/Screensaver.{h,cpp}` |
| Graphics | `src/core/gfx/Canvas.{h,cpp}` |

## Mechanics

- **Events**: the driver task queues button and encoder events. `Ui` turns raw state into
  `KeyEvent` (down/up), `KeyPressEvent` (a press with a count, so double-taps are visible),
  `EncoderEvent` and `MidiEvent`, and `PageManager::dispatchEvent` offers each to the top page, then
  down the stack until one calls `event.consume()`. `TopPage` is the root and handles global keys.
- **Page**: override `enter`/`exit`, `draw(Canvas&)`, `updateLeds(Leds&)`, `keyDown`/`keyUp`/
  `keyPress`, `encoder`, `midi`; `isModal()`; `show()`/`close()` push and pop. Pages are
  constructed once in `struct Pages`.
- **Key names**: the code numbers step buttons `Step0..Step15` (hardware label = N+1) and function
  keys `F0..F4` (labelled F1-F5). Docs here use the code's step numbers and the labelled F1-F5.
  `Key` predicates: `isStep`, `isTrackSelect`, `isQuickEdit` (Page + Step 8-15), `isPageSelect`,
  `isContextMenu` (Shift + Page).
- **Page selection**: Page + a key maps through `PageKeyMap` to `TopPage::Mode` (`setMode`).
- **Common Steps-page logic**: `SequenceEditPageBase::handleCommonKeyPress` handles the context menu
  (Shift + Page, or double-tap Page) and global shortcuts; subclasses provide `contextItems()`,
  `contextAction()`. `BasePage::functionShortcuts` and `showMessage` are shared helpers.
- **Drawing**: pages draw into a `Canvas`; `WindowPainter` provides the header, active-function
  label and the 9 px footer of 5 tab cells. Selection is a Bright fill with the text knocked out using
  `BlendMode::Sub` (never `Color::None`).
- **LEDs**: pages set green/red per LED in `updateLeds`; `LedPainter` has shared patterns (track
  gates, selected page, sections, patterns, mutes, fills, quick-edit values). The function keys have
  no LEDs: show their state on screen. Holding Page switches the LEDs to a page-select grid.
- **Config pages** share one `ListPage`: [config-pages.md](config-pages.md).
- **Controllers**: `ControllerManager` forwards MIDI to a connected Launchpad first
  ([launchpad](launchpad.md)).
- **Boot splash**: `ui/pages/StartupPage.cpp`; the splash is exempt from the three-brightness rule
  (`prep knowledge find splash`, `prep knowledge find brand`).

## Change guide

| Task | Where |
|---|---|
| Add a page | `XxxPage.{h,cpp}` deriving `Page`/`BasePage`, a member in `struct Pages` (`Pages.h`), construct it, reach it from `TopPage` or a parent page; for a page key, `PageKeyMap.h` and `TopPage::setMode` |
| Add a track type's Steps page | derive from `SequenceEditPageBase`; model it on the nearest existing one ([note-track](note-track.md), [quantizer-track](quantizer-track.md)) |
| LED bug | `LedPainter.{h,cpp}`, the page's `updateLeds` |
| Change header or footer | `ui/painters/WindowPainter.cpp` |
| Screensaver | `ui/Screensaver.cpp`, `Ui.cpp` |
| Show a toast | `showMessage(...)` on any page |
| Display resolution | `CONFIG_LCD_*` in `src/SystemConfig.h` and the `Canvas` buffer |
