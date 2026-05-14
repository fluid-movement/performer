# UI State Reference

UX-focused reference for the performer sequencer. Covers every page, every button combo, and every held-button overlay. Complements the architecture doc at [`features/ui-system.md`](features/ui-system.md), which covers rendering, PageManager, and Page base classes.

---

## 1. Physical Input Surface

### Buttons

| Group | Buttons | Notes |
|---|---|---|
| Track | Track0..Track7 | Top row, 8 buttons |
| Step | Step0..Step15 | Bottom two rows, 16 buttons |
| Function | F0..F4 | Below display, 5 buttons |
| Global | Play, Tempo, Pattern, Performer, Left, Right, Shift, Page | 8 global buttons |
| Encoder | push + rotate | Single encoder |

### Modifier keys

- **Shift** (`Key::Shift`, code 30) — held while pressing another key
- **Page** (`Key::Page`, code 31) — held to navigate or enter quick-edit mode

### Composite key predicates (defined in `Key.h`)

| Predicate | Meaning |
|---|---|
| `isTrackSelect()` | Track pressed without Page held |
| `isPageSelect()` | Page + Track (→ pages 0–7) or Page + Step 0–7 (→ pages 8–15) |
| `isQuickEdit()` | Page + Step 8–15 (quickEdit index = step − 8) |
| `isContextMenu()` | Shift+Page or Page+Shift (either order) |

---

## 2. Global Navigation Map

Handled by `TopPage::keyPress` (`src/apps/sequencer/ui/pages/TopPage.cpp`).

| Trigger | Effect |
|---|---|
| **Track N** | Select track N (without Page held) |
| **Page + Tempo** | Clock Setup page |
| **Page + Pattern** | Pattern page (non-modal) |
| **Page + Performer** | Performer page (non-modal) |
| **Page + Left** | Overview page |
| **Page + Track0** | Project page |
| **Page + Track1** | Layout page |
| **Page + Track2** | Routing page |
| **Page + Track3** | MIDI Output page |
| **Page + Track4** | User Scale page |
| **Page + Track7** | System page (confirmation dialog first) |
| **Page + Step0** | Sequence Edit page (track-type dependent) |
| **Page + Step1** | Sequence page (track-type dependent) |
| **Page + Step2** | Track page |
| **Page + Step3** | Song page |
| **Page + Step7** | Monitor page |
| **Page + Step8–14** | Quick-Edit overlay for that parameter slot |
| **Page + Step15** | Action depends on current edit page (see per-page entries) |
| **Play** | Toggle play/stop |
| **Shift + Play** | Toggle play/stop with shift modifier (alternate sync behavior) |
| **Page + Play** | Toggle recording |
| **Tempo** | Open Tempo page (modal) |
| **Pattern** (without Page, not already on Pattern page) | Pattern page as **modal overlay** — closes on release |
| **Performer** (without Page, not already on Performer page) | Performer page as **modal overlay** — closes on release |
| **Shift + Page** (or **Page + Shift**) | Open current page's context menu |

### `Page` held — LED behavior

While Page is held (no Shift), `LedPainter::drawSelectedPage` replaces the normal track-gate LED display with a page-select grid showing the current page. On edit pages (NoteSequenceEdit, OverviewPage), Step8–15 LEDs also light up green for available quick-edit slots.

---

## 3. Page Inventory

### 3a. Global / settings pages

These share a common **ListPage** interaction model: Left/Right edit the focused row's value; Shift makes the step coarse (×10); encoder navigates rows; encoder pressed while turning = coarse. All open a context menu via **Shift+Page** or double-tap **Page**.

---

#### ProjectPage
**Path:** `src/apps/sequencer/ui/pages/ProjectPage.cpp`  
**Reach:** Page + Track0

**On screen:** Project name, global tempo, default time signature, default scale / root note.

**Key interactions:**
- Encoder press on row 0 → open text input for project name
- Encoder press on row 5 → set selected scale
- Page + Step15 → Asteroids easter egg
- Context menu: **INIT / LOAD / SAVE / SAVE AS / ROUTE**
  - LOAD and SAVE require SD card mounted
  - ROUTE jumps to Routing page for the focused row's routing target

---

#### LayoutPage
**Path:** `src/apps/sequencer/ui/pages/LayoutPage.cpp`  
**Reach:** Page + Track1

**On screen:** CV output assignment, gate output assignment, MIDI program source per track.

**Key interactions:** Standard ListPage (Left/Right/encoder). Context menu via Shift+Page.

---

#### TrackPage
**Path:** `src/apps/sequencer/ui/pages/TrackPage.cpp`  
**Reach:** Page + Step2

**On screen:** Track type selector (Note/Curve/Stochastic/Logic/Arp/Quantizer/MidiCv), track name, link target, fill mode, play mode, slide time, and track-type-specific settings.

**Key interactions:** Standard ListPage. Changing track type rebuilds the settings list.

---

#### RoutingPage
**Path:** `src/apps/sequencer/ui/pages/RoutingPage.cpp`  
**Reach:** Page + Track2

**On screen:** Up to 16 routing slots. Each route: source → target → track mask + depth.

**Key interactions:** Standard ListPage. Context menu to add/remove/clear routes.

---

#### MidiOutputPage
**Path:** `src/apps/sequencer/ui/pages/MidiOutputPage.cpp`  
**Reach:** Page + Track3

**On screen:** 8 MIDI output slots — channel, program, note source, velocity source.

**Key interactions:** Standard ListPage.

---

#### UserScalePage
**Path:** `src/apps/sequencer/ui/pages/UserScalePage.cpp`  
**Reach:** Page + Track4

**On screen:** 4 user scale slots. Each: name + 12-bit interval bitmap.

**Key interactions:** Standard ListPage. Encoder press on name row → text input.

---

#### SystemPage
**Path:** `src/apps/sequencer/ui/pages/SystemPage.cpp`  
**Reach:** Page + Track7 (requires confirmation)

**On screen:** Calibration, MIDI port config, update/reset utilities.

**Key interactions:** Standard ListPage. Destructive operations show a confirmation dialog.

---

#### ClockSetupPage
**Path:** `src/apps/sequencer/ui/pages/ClockSetupPage.cpp`  
**Reach:** Page + Tempo

**On screen:** Clock source (internal/external), PPQN, swing, sync pulse config.

**Key interactions:** Standard ListPage.

---

#### MonitorPage
**Path:** `src/apps/sequencer/ui/pages/MonitorPage.cpp`  
**Reach:** Page + Step7

**On screen:** Live MIDI/CV input monitor, note/CC/gate activity per port.

**Key interactions:** Read-only display. Any key closes.

---

### 3b. Performance pages

---

#### PatternPage
**Path:** `src/apps/sequencer/ui/pages/PatternPage.cpp`  
**Reach:** Page + Pattern (non-modal); Pattern key alone (modal — auto-closes on release)

**On screen:** 8 track columns. Each column shows: track number, a 16-pattern grid (current = bright, requested = dim), current pattern label. Sync progress bar at top when a synced change is pending.

**Function keys:** F0=LATCH · F1=SYNC · F2=SNAP (or REVERT when snapshot active) · F3=COMMIT (when snapshot active) · F4=CANCEL (when pending requests)

**Key interactions:**

| Combo | Effect |
|---|---|
| Step N | Change playing pattern to N (execute type = Immediate / Latched / Synced depending on held F keys and PatternChange user setting) |
| Shift + Step N | Set **edit** pattern to N (affects which pattern is shown in the sequence edit pages) |
| Step A + Step B (simultaneously) | Chain patterns A and B into song mode |
| Track N held + Step M | Change pattern M for track N only |
| F0 (LATCH) held | Changes become latched (committed when F0 released) |
| F1 (SYNC) held | Changes become synced to next measure boundary |
| F2 (SNAP) | Create a snapshot of current patterns |
| F2 (REVERT) | Revert to snapshot, optionally targeting a pattern slot |
| F3 (COMMIT) | Commit snapshot changes |
| F4 (CANCEL) | Cancel pending pattern requests |
| Left / Right | Edit selected pattern index |
| Encoder | Edit selected pattern index; pressed = coarse |
| Shift + Page | Context menu: INIT / COPY / PASTE / DUP / SAVE |

**LED behavior:** Step LEDs show active patterns (bright) and pending patterns (dim). Shift held → LEDs show edit pattern. Snapshot active → LEDs show snapshot target pattern.

---

#### PerformerPage
**Path:** `src/apps/sequencer/ui/pages/PerformerPage.cpp`  
**Reach:** Page + Performer (non-modal); Performer key alone (modal — auto-closes when Performer released and no latching)

**On screen:** 8 track columns. Each: track number (bright when fill active), activity rect, mute state, sequence progress bar, fill-amount bar.

**Function keys:** F0=LATCH · F1=SYNC · F2=UNMUTE · F3=FILL · F4=CANCEL (when pending)

**Key interactions:**

| Combo | Effect |
|---|---|
| Track N | Toggle mute on track N |
| Shift + Track N | Toggle solo on track N |
| F0 (LATCH) held | Mute changes become latched |
| F1 (SYNC) held | Mute changes become synced |
| F2 (UNMUTE) | Unmute all tracks (respects latch/sync mode) |
| F3 (FILL) held | Enable fill on all tracks |
| Step 8–15 held | Enable fill on corresponding track (track = step − 8); combines with Shift for "hold" fill |
| Step 0–7 held + encoder | Edit fill amount for that track |
| Encoder (no step held) | Edit project tempo |
| Encoder press (no step held) | Reset tempo to saved project value |
| F4 (CANCEL) | Cancel pending mute requests |

---

#### OverviewPage
**Path:** `src/apps/sequencer/ui/pages/OverviewPage.cpp`  
**Reach:** Page + Left

**On screen:** All 8 tracks in condensed view — track type icon, 16-step gate pattern (current step highlighted), CV output value, mute state. Quantizer tracks show their input CV source.

**Key interactions:**

| Combo | Effect |
|---|---|
| Track N | Select track N |
| Page + Step 8–14 | Quick-edit overlay for current track's sequence parameter (same items as NoteSequenceEdit) |
| Page + Step 15 | Toggle pattern-follow display; on Arp tracks: toggle MIDI keyboard mode |
| Shift + Page | Context menu |

**Quick-edit item table (by track type):**

| Slot | Note/Logic/Stochastic | Curve | Arp |
|---|---|---|---|
| Step8 | FirstStep | FirstStep | — |
| Step9 | LastStep | LastStep | — |
| Step10 | RunMode | RunMode | — |
| Step11 | Divisor | Divisor | Divisor |
| Step12 | ResetMeasure | ResetMeasure | ResetMeasure |
| Step13 | Scale | Range | Scale |
| Step14 | RootNote | — | RootNote |

---

#### SongPage
**Path:** `src/apps/sequencer/ui/pages/SongPage.cpp`  
**Reach:** Page + Step3

**On screen:** Song slot table — slot index, repeat count, T1–T8 pattern (or M if muted). Playback cursor, beat counter, and slot progress bar when playing.

**Function keys:** F0=CHAIN · F1=ADD (Shift+F1=INSERT before) · F2=REMOVE · F3=DUPL · F4=PLAY/STOP

**Key interactions:**

| Combo | Effect |
|---|---|
| F1 (ADD) | Append a new slot at the end |
| Shift + F1 (INSERT) | Insert a slot before the selected slot |
| F2 (REMOVE) | Remove selected slot |
| F3 (DUPL) | Duplicate selected slot |
| F4 (PLAY) | Play song from selected slot |
| Shift + F4 (PLAY) | Play song synced (if clock running) |
| F4 (STOP) | Stop song playback |
| Step N | Set all-track pattern for selected slot to N |
| Track T held + Step N | Set pattern for track T in selected slot to N |
| Track T held + encoder press | Toggle track T mute in selected slot |
| Shift + Step N | Set repeat count for selected slot to N+1 (1–16) |
| Left / Right | Move selected slot cursor |
| Shift + Left / Right | Swap selected slot with neighbour |
| Encoder | Move selected slot cursor |
| Shift + Encoder | Edit repeat count for selected slot |
| Track T held + encoder | Edit track T's pattern in selected slot |
| Encoder press (no track held) | Play song from selected slot |
| F0 (CHAIN) held + Step N | Chain pattern N into a new slot and start playing |
| Shift + Page | Context menu: INIT |

---

### 3c. Sequence pages (per track type)

Each track type has a **Sequence page** (list/overview of the sequence) and a **Sequence Edit page** (step editing). Both are reached via Page+Step1 and Page+Step0 respectively. The active page depends on the selected track's type (`TrackMode`).

---

#### NoteSequencePage
**Path:** `src/apps/sequencer/ui/pages/NoteSequencePage.cpp`  
**Reach:** Page + Step1 (Note track)

**On screen:** Parameter list view of the selected Note sequence (FirstStep, LastStep, RunMode, Divisor, Scale, RootNote, etc.).

**Key interactions:** Standard ListPage. Context menu via Shift+Page.

---

#### NoteSequenceEditPage
**Path:** `src/apps/sequencer/ui/pages/NoteSequenceEditPage.cpp`  
**Reach:** Page + Step0 (Note track)

**On screen:** 16-step grid with loop-point markers. Each step shows: gate box (filled = active), current-step highlight, per-layer value bar or symbol below. Header shows pattern-follow flag. Footer shows active layer family (GATE/RETRIG/LENGTH/NOTE/COND).

**Function key families (F0–F4):**

| Key | Family | Layers cycled on repeated press |
|---|---|---|
| F0 | GATE | Gate → GateOffset → GateProbability → Gate |
| F1 | RETRIG | Retrigger → RetriggerProbability → StageRepeats → StageRepeatsMode (Free mode only) → Retrigger |
| F2 | LENGTH | Length → LengthVariationRange → LengthVariationProbability → Length |
| F3 | NOTE | Note → NoteVariationRange → NoteVariationProbability → Slide → BypassScale → Note |
| F4 | COND | Condition |

**Shift + Function key (jump to sub-layer):**

| Combo | Effect |
|---|---|
| Shift + F0 | Jump to Gate layer |
| Shift + F1 | Jump to StageRepeats (Free mode only) |
| Shift + F2 | Jump to StageRepeatsMode (Free mode only); **if steps selected**: tie notes (set all to max length and same pitch) |
| Shift + F3 | Jump to Slide layer |
| Shift + F4 | Jump to Condition layer |

**Function key + Step (octave assign):**

| Combo | Effect |
|---|---|
| F0 held + Step N | Set step N note to 1 × notesPerOctave |
| F1 held + Step N | Set step N note to 2 × notesPerOctave |
| F2 held + Step N | Set step N note to 3 × notesPerOctave |
| F3 held + Step N | Set step N note to 4 × notesPerOctave |
| F4 held + Step N | Set step N note to 5 × notesPerOctave |

**Step interactions:**

| Combo | Effect |
|---|---|
| Step N (Gate layer) | Toggle gate on step N |
| Step N double-tap (other layers) | Toggle gate on step N |
| Hold Step N | Select step N (Immediate mode — selection clears on release) |
| Shift + Step N | Enter **Persist** mode, select step N (stays selected after release) |
| Hold Step A + hold Step B | Select both steps (Immediate mode with two held keys) |

**Step selection — persist mode operations:**

| Combo | Effect |
|---|---|
| Shift double-tap (no selection) | Select all steps |
| Shift double-tap (with selection in Persist) | Clear selection |
| Double-tap Step N (Persist mode, no other step held) | Select all steps with equal layer value (`selectEqualSteps`) |
| Two steps held + double-tap one (Persist mode) | Select steps at regular intervals between the two held steps |

**Encoder:**

| State | Effect |
|---|---|
| No selection | Cycle to next layer within current family |
| Steps selected | Edit layer value for all selected steps |
| Shift held + selection + chromatic scale (Note layer) | Transpose selected notes by one octave |

**Encoder press:** Toggle gate on selected steps (set all on; if all were already on, set all off).

**Navigation:**

| Combo | Effect |
|---|---|
| Left | Scroll to previous section (sections 0–3, 16 steps each) |
| Right | Scroll to next section |
| Shift + Left | Shift selected steps left by 1 within loop bounds; or move record cursor left |
| Shift + Right | Shift selected steps right by 1 within loop bounds; or move record cursor right |

**Quick-edit (Page + Step8–15):**

| Key | Parameter |
|---|---|
| Page + Step8 | FirstStep |
| Page + Step9 | LastStep |
| Page + Step10 | RunMode |
| Page + Step11 | Divisor |
| Page + Step12 | ResetMeasure |
| Page + Step13 | Scale |
| Page + Step14 | RootNote |
| Page + Step15 | Toggle pattern-follow display |

**Other:**

| Combo | Effect |
|---|---|
| Page + Step6 | **Undo** — revert sequence to snapshot taken on page enter (or last gate/note edit) |
| Shift + Page | Context menu: INIT / COPY / PASTE / DUPL / GEN |
| Page double-tap | Context menu (same as above) |
| MIDI note-on (Note layer, steps selected, not recording) | Set note on selected steps + enable gate |

**Detail overlay:** When encoder is turned with steps selected (non-Gate layer), a detail popup shows the current value for the first selected step. Disappears after 500 ms (Persist mode) or immediately on selection clear.

---

#### CurveSequencePage / CurveSequenceEditPage
**Path:** `src/apps/sequencer/ui/pages/CurveSequence*.cpp`  
**Reach:** Page + Step1/Step0 (Curve track)

Similar to NoteSequenceEditPage with different layers: Shape, Min, Max, Gate, Condition. F0–F4 map to these families. Holding a function key while turning the encoder edits that function's parameter directly (without switching to that layer). Context menu: INIT / COPY / PASTE / DUPL / GEN.

---

#### StochasticSequencePage / StochasticSequenceEditPage
**Path:** `src/apps/sequencer/ui/pages/StochasticSequence*.cpp`  
**Reach:** Page + Step1/Step0 (Stochastic track)

Step grid with probability weights. F0–F4 = Gate/Length/Note/Probability/Condition. Same step-selection and quick-edit mechanics as NoteSequenceEditPage.

---

#### LogicSequencePage / LogicSequenceEditPage
**Path:** `src/apps/sequencer/ui/pages/LogicSequence*.cpp`  
**Reach:** Page + Step1/Step0 (Logic track)

Boolean logic track: input sources A/B, operator (AND/OR/XOR/…), gate output. ListPage-style sequence params. Edit page shows gate steps.

---

#### ArpSequencePage / ArpSequenceEditPage
**Path:** `src/apps/sequencer/ui/pages/ArpSequence*.cpp`  
**Reach:** Page + Step1/Step0 (Arp track)

Arpeggiator configuration (mode, hold, octave range, pattern). Page + Step15 on OverviewPage toggles MIDI keyboard input mode for live note selection.

---

#### QuantizerSequencePage / QuantizerSequenceEditPage
**Path:** `src/apps/sequencer/ui/pages/QuantizerSequence*.cpp`  
**Reach:** Page + Step1/Step0 (Quantizer track)

**On screen (Sequence page):** CV input source, trigger mode (Free/Internal/External), trigger source track, octave offset, transpose. Standard ListPage.

**On screen (Edit page):** When trigger mode = Internal, shows a gate-lane step editor (uses NoteSequence internally). F0 = gate layer. Same step-selection, quick-edit, and undo mechanics as NoteSequenceEditPage.

---

### 3d. Modal / utility pages

---

#### TempoPage
**Path:** `src/apps/sequencer/ui/pages/TempoPage.cpp`  
**Reach:** Tempo key (always modal)

**On screen:** Large BPM display, tap tempo indicator.

| Combo | Effect |
|---|---|
| Encoder | ±1 BPM |
| Shift + Encoder | ±10 BPM |
| Encoder press (while turning) | ±0.1 BPM |
| Any key | Tap tempo |

---

#### ContextMenuPage
**Path:** `src/apps/sequencer/ui/pages/ContextMenuPage.cpp`

Floating action menu pushed onto the page stack. F keys select items; releases when Shift+Page is released.

---

#### QuickEditPage
**Path:** `src/apps/sequencer/ui/pages/QuickEditPage.cpp`  
**Reach:** Page + Step8–14 from edit pages / OverviewPage

**On screen:** Parameter name + current value + bar.

| Combo | Effect |
|---|---|
| Left / Right | Edit value (±1) |
| Shift + Left / Right | Edit value coarse (±10) |
| Encoder | Edit value |
| Release Page | Close page |

---

#### GeneratorSelectPage / GeneratorPage
**Path:** `src/apps/sequencer/ui/pages/Generator*.cpp`  
**Reach:** Context menu → GEN on sequence edit pages

Select a generator mode (Euclidean, Random, etc.), then preview and apply to selected or all steps.

---

#### FileSelectPage
**Path:** `src/apps/sequencer/ui/pages/FileSelectPage.cpp`

SD card file browser. Used by Project LOAD/SAVE AS. Encoder navigates slots, encoder press or F confirm, any other key cancels.

---

#### TextInputPage
**Path:** `src/apps/sequencer/ui/pages/TextInputPage.cpp`

On-screen character picker. Step keys select character groups; encoder scrolls characters; F4 confirms, F3 deletes, F0 cancels.

---

#### ConfirmationPage
**Path:** `src/apps/sequencer/ui/pages/ConfirmationPage.cpp`

Yes/No prompt. F4 = Yes, any other key = No.

---

#### BusyPage
**Path:** `src/apps/sequencer/ui/pages/BusyPage.cpp`

Blocking "please wait" screen shown during SD/flash operations. No input accepted.

---

### 3e. Misc pages

| Page | Notes |
|---|---|
| `StartupPage` | Boot screen, shown during firmware init |
| `IntroPage` | Optional splash screen (`CONFIG_ENABLE_INTRO`) |
| `AsteroidsPage` | Easter egg game — reach via Page+Step15 on ProjectPage (`CONFIG_ENABLE_ASTEROIDS`) |

---

## 4. Held-Button & Overlay Catalogue

These are the most "hidden" affordances — behaviors triggered by holding one button while pressing another.

| Held button | Pressed / turned | Page(s) | Effect |
|---|---|---|---|
| **Page** (no Shift) | — | All | LEDs switch to page-select grid |
| **Page** | Step 8–14 | Edit pages, Overview | Open QuickEditPage for that parameter slot |
| **Page** | Step 15 | NoteSequenceEdit, Overview | Toggle pattern-follow display |
| **Page** | Step 15 | OverviewPage (Arp track) | Toggle MIDI keyboard mode |
| **Page** | Step 15 | ProjectPage | Asteroids easter egg |
| **Page** | Step 6 | NoteSequenceEdit | Undo sequence to last snapshot |
| **Page** | Play | Any | Toggle recording |
| **Pattern** | (hold) | Any (except Pattern) | Pattern page modal overlay — closes on Pattern release |
| **Performer** | (hold) | Any (except Performer) | Performer page modal overlay — closes on Performer release |
| **Shift** | Step N (Immediate mode) | Edit pages | Switch to **Persist** selection mode, select step N |
| **Shift** | Step N (Persist mode) | Edit pages | Switch back to **Immediate** mode |
| **Shift** | Shift double-tap | Edit pages (Persist) | Select all (if none selected) / clear selection |
| **Shift** | Encoder | Note layer (chromatic) | Transpose selected steps by ±1 octave |
| **Shift** | Encoder | ListPage pages | Coarse step (×10) |
| **Shift** | F2 (with selection) | NoteSequenceEdit | Tie selected notes |
| **Step 0–7** (held) | Encoder | PerformerPage | Edit fill amount for that track |
| **Step 8–15** (held) | — | PerformerPage | Enable fill on track (step − 8) |
| **Step 8–15** + **Shift** | — | PerformerPage | Hold fill (sustained, not momentary) |
| **F0–F4** (held) | Encoder | CurveSequenceEdit | Edit that function's parameter directly |
| **Track T** (held) | Step N | PatternPage | Change track T's pattern only |
| **Track T** (held) | Step N | SongPage | Set track T's pattern in selected slot |
| **Track T** (held) | Encoder | SongPage | Edit track T's pattern in selected slot |
| **Track T** (held) | Encoder press | SongPage | Toggle mute for track T in selected slot |
| **Shift** (held) | — | PatternPage | LEDs show edit pattern instead of active patterns |
| Two **Step** keys (held) | — | PatternPage | Chain two patterns into song |
| Two **Step** keys (Persist, double-tap) | — | Edit pages | Select steps at regular intervals between the two keys |
| F0 (LATCH, held) | Step / Track | PatternPage, PerformerPage | Changes become latched until F0 release |
| F1 (SYNC, held) | Step / Track | PatternPage, PerformerPage | Changes become synced to measure boundary |

---

## 5. Cross-Reference Index

Alphabetical combo → page(s) where it applies.

| Combo | Page | Effect |
|---|---|---|
| Double-tap Page | Any page with context menu | Open context menu |
| Encoder (no selection) | NoteSequenceEdit | Cycle layer within family |
| Encoder (with selection) | NoteSequenceEdit | Edit layer value for selected steps |
| Encoder press | NoteSequenceEdit | Toggle gate on selection |
| Encoder press | SongPage (no track held) | Play song from selected slot |
| Encoder press + Shift | SongPage | (N/A — separate combos) |
| F0–F4 | NoteSequenceEdit | Select/cycle layer family |
| F0–F4 held + Step N | NoteSequenceEdit | Set step N note to octave N |
| F0 (LATCH) held | PatternPage, PerformerPage | Latch mode |
| F1 (SYNC) held | PatternPage, PerformerPage | Sync mode |
| F2 (SNAP/REVERT) | PatternPage | Snapshot create or revert |
| F3 (COMMIT) | PatternPage | Commit snapshot |
| F4 (CANCEL) | PatternPage, PerformerPage | Cancel pending requests |
| Left / Right | NoteSequenceEdit | Scroll section |
| Left / Right | PatternPage, SongPage, QuickEditPage, ListPage | Edit / navigate |
| Page + Left | Any | Overview page |
| Page + Pattern | Any | Pattern page (non-modal) |
| Page + Performer | Any | Performer page (non-modal) |
| Page + Play | Any | Toggle recording |
| Page + Step0 | Any | Sequence Edit page |
| Page + Step1 | Any | Sequence page |
| Page + Step2 | Any | Track page |
| Page + Step3 | Any | Song page |
| Page + Step6 | NoteSequenceEdit | Undo sequence |
| Page + Step7 | Any | Monitor page |
| Page + Step8–14 | NoteSequenceEdit, OverviewPage | QuickEditPage |
| Page + Step15 | NoteSequenceEdit, OverviewPage | Toggle follow; Arp: keyboard mode |
| Page + Step15 | ProjectPage | Asteroids easter egg |
| Page + Tempo | Any | Clock Setup page |
| Page + Track0 | Any | Project page |
| Page + Track1 | Any | Layout page |
| Page + Track2 | Any | Routing page |
| Page + Track3 | Any | MIDI Output page |
| Page + Track4 | Any | User Scale page |
| Page + Track7 | Any | System page (with confirmation) |
| Pattern (hold) | Any | Modal PatternPage |
| Performer (hold) | Any | Modal PerformerPage |
| Play | Any | Toggle play |
| Shift + Encoder | NoteSequenceEdit (Note+chromatic) | Octave transpose |
| Shift + Encoder | ListPage | Coarse edit |
| Shift + F0 | NoteSequenceEdit | Jump to Gate layer |
| Shift + F1 | NoteSequenceEdit | Jump to StageRepeats (Free mode) |
| Shift + F2 (+ selection) | NoteSequenceEdit | Tie notes |
| Shift + F3 | NoteSequenceEdit | Jump to Slide layer |
| Shift + F4 | NoteSequenceEdit | Jump to Condition layer |
| Shift + Left | NoteSequenceEdit | Shift steps left |
| Shift + Left/Right | SongPage | Swap slots |
| Shift + Page | Any | Context menu |
| Shift + Play | Any | Toggle play (alt/sync mode) |
| Shift + Step | NoteSequenceEdit | Persist-mode step selection |
| Shift + Step | PatternPage | Set edit pattern |
| Shift + Step | SongPage | Set slot repeat count |
| Shift + Track | PerformerPage | Solo track |
| Step (held) | NoteSequenceEdit | Immediate-mode step selection |
| Step 0–7 + Encoder | PerformerPage | Edit fill amount |
| Step 8–15 (held) | PerformerPage | Enable fill |
| Step N | NoteSequenceEdit (Gate) | Toggle gate |
| Step N | PatternPage | Change playing pattern |
| Step N | SongPage | Set slot pattern |
| Step N | SongPage (F0 Chain held) | Chain pattern into song |
| Tempo | Any | Tempo page |
| Track N | Any | Select track N |
| Track N | PerformerPage | Toggle mute |

---

## 6. Observations for Redesign

The following surfaced during documentation. These are observations, not decisions.

### Buried global config pages
Project, Layout, Routing, MidiOutput, UserScale, and System are all reachable only via **Page + Track** modifier. There is no direct, labelled path from any normal working view. A user who doesn't know the key map can't discover these pages.

### ListPage monoculture for config
Project, Layout, Track, Routing, MidiOutput, UserScale all use the same ListPage interaction (Left/Right scroll, Shift coarse, encoder). They have no shortcuts or structure beyond a flat scrollable list. Any setting requires navigating to the right page → finding the right row → editing. No visual hierarchy.

### Inconsistent Shift+Encoder semantics
- NoteSequenceEdit (Note layer, chromatic): Shift+encoder = ±1 octave
- ListPage pages: Shift+encoder = coarse step (×10)
- TempoPage: Shift+encoder = ×10 BPM (consistent with ListPage but different context)

### Hidden affordances
None of these are discoverable from the UI:
- `Page+Step6` = undo (NoteSequenceEdit) — no label, no LED
- `Page+Step15` = pattern-follow toggle — no label until you've read the header status flag
- `F*+Step` = set note to octave — unlabelled
- Asteroids via `Page+Step15` on ProjectPage
- Two-step chain in PatternPage
- Two-step interval fill in step selection (Persist mode)

### Pattern/Performer modal vs. non-modal duality
Both Pattern and Performer can be reached as a modal overlay (hold the button) or as a pinned page (Page+key). This is a useful feature but creates implicit state — modal closes on release, non-modal stays. Easy to accidentally stay on the wrong version.

### Sequence sections require Left/Right to navigate
64-step sequences are accessed as four 16-step sections. Navigating sections requires pressing Left/Right, with no visual indication of total sections or jump shortcuts. Pattern follow helps but must be explicitly toggled.

### Quick-edit items require knowing step numbers
`Page+Step8..14` = sequence parameters. The only indication is which step LEDs light up green when Page is held. There's no label association. Users must memorize the slot-to-parameter mapping.
