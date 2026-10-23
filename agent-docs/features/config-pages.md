# Config (List) Pages

Every settings page is a `ListPage` driven by a `ListModel`: Track, Project, Clock setup, Layout,
Routing, MIDI output, User scale, System, the Sequence overview pages of the five step track types,
and the file and generator pickers. All 15 `ListPage` subclasses share one interaction model.
Rationale: `prep knowledge find config` (modeless pages, sections, the detail pane).

## Key source files

| Role | Path |
|---|---|
| The page | `src/apps/sequencer/ui/pages/ListPage.{h,cpp}` |
| Model interface | `src/apps/sequencer/ui/model/ListModel.h` |
| Models (one per page or track type) | `src/apps/sequencer/ui/model/*ListModel.h` |
| Preview drawings | `src/apps/sequencer/ui/painters/PreviewPainter.{h,cpp}` |
| Tests | `src/apps/sequencer/tests/ui/config_nav_test.py` |

Subclasses: `ProjectPage`, `LayoutPage`, `TrackPage`, `ClockSetupPage`, `RoutingPage`,
`MidiOutputPage`, `UserScalePage`, `SystemPage`, `NoteSequencePage`, `CurveSequencePage`,
`StochasticSequencePage`, `ArpSequencePage`, `QuantizerSequencePage`, `FileSelectPage`,
`GeneratorSelectPage`.

## Interaction (modeless)

- The **encoder always edits the selected row's value** (`ListModel::edit(row, 1, value, coarse)`;
  pressing while turning, or holding Shift, is the coarse step). It never navigates. Single-column
  models (pickers) are the exception: there the encoder moves the cursor.
- **Left / Right move the cursor** and clamp (no wrap). Their LEDs light when pressing them will
  move the cursor.
- **F1-F5 jump to a section** when the model has sections; **pressing the active section's key
  again advances the cursor down that section and wraps**.
- The encoder *press* is left to the subclass (name entry, confirmation, sub-pages).

## Layout

`LineCount` 6 rows of 7 px (`Font::Tiny`), label at x = 8, value at x = 60, list width 164 px, a
divider, and a **detail pane** 76 px wide at x = 176. Row labels are at most 8 characters, ALL CAPS.
The pane shows the selected row's preview if it has one, otherwise its value drawn large (wrapping
long enum names onto two lines).

## `ListModel` API

`rows()`, `columns()`, `cell(row, column, str)`, `edit(row, column, value, shift)`; indexed pickers
(`indexedCount`, `indexed`, `setIndexed`); **sections** (`sections()`, `sectionName()`,
`rowSection(row)`, defaulted to none; keyed per row so the `enum Item` order need not change; keep
sections at 6 rows or fewer so nothing scrolls); **previews** (`hasPreview(row)`,
`drawPreview(canvas, row, x, y, w, h)`).

Sections are defined on the six track list models plus Project and Clock.

## Previews (`PreviewPainter`)

Drawings: `drawSlideRamp`, `drawPulseTrain`, `drawPulseWidth`, `drawSwingGrid`, `drawBeatGrid`,
`drawCurveShape`, `drawVoltageBar`, `drawBipolarBar`, `drawKeyboard` (scale degrees lit),
`drawNoteRange`. They must call the engine's own math (`Slide::shape`, `CurveSequence::evalSegment`)
rather than reimplement it, so a change to the sound changes the picture.

## Change guide

| Task | Where |
|---|---|
| Add a setting to a page | the model's `enum Item`, `rows`/`cell`/`edit`, label of 8 characters or fewer, section mapping if the model has sections, then the underlying model field ([serialization](serialization.md)) |
| Add a preview | a drawing in `PreviewPainter`, `hasPreview`/`drawPreview` in the model |
| Add sections to a model | override `sections`, `sectionName`, `rowSection` |
| Page-specific footer or actions | the page's `draw`/`keyPress`, calling `ListPage::keyPress` last |
