# Current UI — Function Reference

This document catalogues every page in the current PER|FORMER firmware: what it displays, what parameters it exposes, and exactly what every physical button combination does on that page. It is the **source-of-truth input for UI redesign** — design decisions should be evaluated against this function list, not aesthetics alone.

**Form follows function.** Read this before touching the sandbox.

Primary sources: `agent-docs/ui-state.md`, `agent-docs/features/*.md`.  
Code sources: `src/apps/sequencer/ui/pages/`, `src/apps/sequencer/model/`.

---

## 0. Physical Input Surface

### Button groups

| Group | Labels | Count | Notes |
|---|---|---|---|
| Track | Track0–Track7 | 8 | Top row — select active track |
| Step | Step0–Step15 | 16 | Bottom two rows — gate/note editing and navigation |
| Function | F0–F4 | 5 | Below display — context-sensitive per page |
| Global | Play, Tempo, Pattern, Performer, Left, Right, Shift, Page | 8 | Always active via `TopPage` |
| Encoder | rotate + press | 1 | Navigate lists and edit values |

### Modifier keys

| Key | Code | Role |
|---|---|---|
| **Shift** | 30 | Held with another key for alternate action |
| **Page** | 31 | Held for navigation; alone switches LED display to page-select grid |

### Composite predicates (`Key.h`)

| Predicate | Condition |
|---|---|
| `isTrackSelect()` | Track pressed, Page **not** held |
| `isPageSelect()` | Page + Track0–7 (pages 0–7) or Page + Step0–7 (pages 8–15) |
| `isQuickEdit()` | Page + Step8–15 (quick-edit index = step − 8) |
| `isContextMenu()` | Shift+Page or Page+Shift (either order) |

---

## 1. Global Navigation Map

Handled by `TopPage::keyPress` (`src/apps/sequencer/ui/pages/TopPage.cpp`).  
These combos work from **any page** unless noted.

| Combo | Destination / Effect |
|---|---|
| Track N | Select track N |
| Play | Toggle play / stop |
| Shift + Play | Toggle play / stop (alternate sync behavior) |
| Page + Play | Toggle recording |
| Tempo | Tempo page (modal) |
| Pattern (hold) | Pattern page (modal overlay — closes on release) |
| Page + Pattern | Pattern page (non-modal — stays open) |
| Performer (hold) | Performer page (modal overlay — closes on release) |
| Page + Performer | Performer page (non-modal) |
| Page + Left | Overview page |
| Page + Track0 | Project page |
| Page + Track1 | Layout page |
| Page + Track2 | Routing page |
| Page + Track3 | MIDI Output page |
| Page + Track4 | User Scale page |
| Page + Track7 | System page (with confirmation dialog) |
| Page + Step0 | Sequence Edit page (depends on active track type) |
| Page + Step1 | Sequence page (depends on active track type) |
| Page + Step2 | Track page |
| Page + Step3 | Song page |
| Page + Step7 | Monitor page |
| Page + Step8–14 | Quick-Edit overlay for parameter slot (step − 8) |
| Page + Step15 | Context-dependent (see per-page entries) |
| Shift + Page | Open current page's context menu |
| Page double-tap | Open current page's context menu |

---

## 2. Settings / Config Pages

These pages use the **ListPage** interaction model unless noted: Left/Right = ±1 on focused row; Shift + Left/Right = coarse ±10; encoder rotates rows; Shift + encoder = coarse; context menu via Shift+Page or Page double-tap.

---

### 2.1 Project Page

**Source:** `src/apps/sequencer/ui/pages/ProjectPage.cpp`  
**Reached by:** Page + Track0  
**Modal?** No

**Parameters displayed / editable**

| Parameter | Type / Range | Stored in |
|---|---|---|
| Project name | String (max ~16 chars) | `Project._name` |
| Tempo | 1.0–1000.0 BPM (float, routable) | `Project._tempo` |
| Swing | 0–100% (routable) | `Project._swing` |
| Time signature | Beats per bar | `Project._timeSignature` |
| Default scale | Built-in or user scale index | `Project` default scale |
| Default root note | 0–11 (C–B) | `Project` default root |
| Auto-load | On / Off | `Project._autoLoaded` |

**Buttons**

| Combo | Action |
|---|---|
| Encoder | Navigate rows |
| Left / Right | Edit focused row value ±1 |
| Shift + Left / Right | Edit focused row value ±10 |
| Encoder press on "Name" row | Open text input |
| Encoder press on "Scale" row | Open scale selector |
| Page + Step15 | Asteroids easter egg (`CONFIG_ENABLE_ASTEROIDS`) |
| Shift + Page | Context menu |

**Context menu (Shift+Page)**
- **INIT** — reset project to default state
- **LOAD** — load project from SD card (opens FileSelectPage)
- **SAVE** — save project to current SD slot
- **SAVE AS** — save to a new SD slot (opens FileSelectPage)
- **ROUTE** — jump to Routing page for the focused row's routing target

---

### 2.2 Layout Page

**Source:** `src/apps/sequencer/ui/pages/LayoutPage.cpp`  
**Reached by:** Page + Track1  
**Modal?** No

**Parameters displayed / editable**

| Parameter | Type | Notes |
|---|---|---|
| CV output assignment | Per-output: which track drives it | 8 CV outputs, any track |
| Gate output assignment | Per-output: which track drives it | 8 gate outputs, any track |
| MIDI program source | Per-track: which MIDI program to respond to | 8 tracks |

**Buttons:** Standard ListPage (encoder navigate, Left/Right ±1, Shift coarse).

---

### 2.3 Track Page

**Source:** `src/apps/sequencer/ui/pages/TrackPage.cpp`  
**Reached by:** Page + Step2  
**Modal?** No

**Parameters displayed / editable**

The list content changes based on the active track type. Common rows:

| Parameter | Type / Range | Notes |
|---|---|---|
| Track type | Note / Curve / Stochastic / Arp / Quantizer / MidiCv | Changing type resets track data |
| Track name | String | |
| Link target | Track 0–7 or None | For track linking / follow behavior |
| Play mode | Aligned / Free | When patterns are aligned to measures |
| Fill mode | Type-specific (see per-track sections) | |
| Slide time | 0–100% (routable) | Note/Stochastic/Arp tracks |

Type-specific rows are listed in their respective sections below (§4–§9).

**Buttons:** Standard ListPage. Changing track type live rebuilds the settings list.

---

### 2.4 Routing Page

**Source:** `src/apps/sequencer/ui/pages/RoutingPage.cpp`  
**Reached by:** Page + Track2  
**Modal?** No

**What it does:** Up to 16 modulation routes. Each route maps an input source → a target parameter, with a track mask and min/max range scaling.

**Parameters per route**

| Parameter | Options |
|---|---|
| Target | Play, Tempo, Swing, Mute, Fill, FillAmount, Pattern, SlideTime, Octave, Transpose, Offset, Rotate, GateProbabilityBias, RetriggerProbabilityBias, LengthBias, NoteProbabilityBias, ShapeProbabilityBias, CurveMin, CurveMax, FirstStep, LastStep, RunMode, Divisor, Scale, RootNote, Reseed, RestProbability, … |
| Track mask | Bitmask — which tracks the route applies to |
| Min / Max | Output range scaling |
| Source type | CV Input (1–4), MIDI CC, MIDI PitchBend, MIDI ChannelPressure, MIDI Note Momentary/Toggle/Velocity |
| Source details | Port, channel, CC number / note number |

**Buttons:** Standard ListPage. Context menu to add / remove / clear routes.

---

### 2.5 MIDI Output Page

**Source:** `src/apps/sequencer/ui/pages/MidiOutputPage.cpp`  
**Reached by:** Page + Track3  
**Modal?** No

**Parameters per slot (16 slots)**

| Parameter | Options |
|---|---|
| Event | NoteOn/NoteOff, ControlChange |
| Source | Track 0–7 + which parameter (gate, note, velocity, CV) |
| Port | UART DIN, USB-MIDI |
| Channel | 1–16 |
| Note offset | Transposition for MIDI output |
| Arpeggiator | Optional arp applied before output |

**Buttons:** Standard ListPage.

---

### 2.6 User Scale Page

**Source:** `src/apps/sequencer/ui/pages/UserScalePage.cpp`  
**Reached by:** Page + Track4  
**Modal?** No

**Parameters per slot (4 slots)**

| Parameter | Type | Notes |
|---|---|---|
| Name | String (up to ~16 chars) | Encoder press opens text input |
| Notes | Up to 32 semitone offset entries | Each defines one scale degree |

Scales are referenced by index from any sequence's scale field. Index 0–N = built-in; N+1–N+4 = user scales.

**Buttons:** Standard ListPage. Encoder press on name row → TextInputPage.

---

### 2.7 Clock Setup Page

**Source:** `src/apps/sequencer/ui/pages/ClockSetupPage.cpp`  
**Reached by:** Page + Tempo  
**Modal?** No

**Parameters**

| Parameter | Options |
|---|---|
| Mode | Auto / Master / Slave |
| Clock input mode | Analog / MIDI / USB-MIDI |
| Slave divisor | Clock pulses per quarter note (when in analog slave mode) |
| Clock output mode | Disabled / Analog / MIDI / USB-MIDI |
| Clock output divisor | Output clock division |
| Clock output pulse | Pulse width in ticks |
| Clock output swing | Swing % applied to output |

**Buttons:** Standard ListPage.

---

### 2.8 System Page

**Source:** `src/apps/sequencer/ui/pages/SystemPage.cpp`  
**Reached by:** Page + Track7 (requires confirmation)  
**Modal?** No

**Parameters / actions**

| Item | Effect |
|---|---|
| Calibration | Hardware CV calibration utility |
| MIDI port config | Set port-specific MIDI settings |
| Firmware update | Flash new firmware |
| Factory reset | Wipe all settings (with confirmation dialog) |

**Buttons:** Standard ListPage. Destructive operations show a ConfirmationPage first.

---

### 2.9 Monitor Page

**Source:** `src/apps/sequencer/ui/pages/MonitorPage.cpp`  
**Reached by:** Page + Step7  
**Modal?** No

**What it shows:** Live display of all incoming MIDI/CV activity — note events, CC values, gate signals, per port.

**Buttons:** Read-only display. Any key press closes the page.

---

## 3. Performance Pages

---

### 3.1 Overview Page

**Source:** `src/apps/sequencer/ui/pages/OverviewPage.cpp`  
**Reached by:** Page + Left  
**Modal?** No

**What it shows:** All 8 tracks at once — per track: track type icon, 16-step gate pattern (current step highlighted), current CV output value, mute state. Quantizer tracks show their CV input source.

**Buttons**

| Combo | Action |
|---|---|
| Track N | Select track N |
| Page + Step8 | Quick-edit: FirstStep (current track) |
| Page + Step9 | Quick-edit: LastStep |
| Page + Step10 | Quick-edit: RunMode |
| Page + Step11 | Quick-edit: Divisor |
| Page + Step12 | Quick-edit: ResetMeasure |
| Page + Step13 | Quick-edit: Range (Curve track only) |
| Page + Step15 | Toggle pattern-follow display; Arp track: toggle MIDI keyboard mode |
| Shift + Page | Context menu |

---

### 3.2 Pattern Page

**Source:** `src/apps/sequencer/ui/pages/PatternPage.cpp`  
**Reached by:** Page + Pattern (non-modal) or Pattern held (modal)  
**Modal?** Dual: holds as modal; Page+Pattern pins as non-modal

**What it shows:** 8 track columns × 16 pattern grid. Current active pattern = bright; pending/requested pattern = dim. Sync progress bar when a synced change is pending.

**Function keys**

| Key | Label | Action |
|---|---|---|
| F0 | LATCH | Hold = latch mode (changes commit on F0 release) |
| F1 | SYNC | Hold = sync mode (changes commit on next measure boundary) |
| F2 | SNAP | Create snapshot of current patterns; becomes REVERT when snapshot is active |
| F3 | COMMIT | Commit snapshot changes to current patterns |
| F4 | CANCEL | Cancel all pending pattern requests |

**Buttons**

| Combo | Action |
|---|---|
| Step N | Change playing pattern to N (immediate / latched / synced per F0/F1 state) |
| Shift + Step N | Set **edit** pattern to N (pattern displayed in sequence edit pages) |
| Step A + Step B (simultaneous) | Chain patterns A → B into song mode |
| Track T held + Step N | Change pattern N for track T only |
| F0 held | Latch mode (as above) |
| F1 held | Sync mode (as above) |
| F2 (REVERT, snapshot active) | Revert to snapshot; optionally target a specific pattern slot |
| F3 (COMMIT) | Commit snapshot |
| F4 (CANCEL) | Cancel pending requests |
| Left / Right | Edit selected pattern index |
| Encoder | Edit selected pattern index (press = coarse) |
| Shift (held) | LEDs show edit pattern instead of active patterns |
| Shift + Page | Context menu: INIT / COPY / PASTE / DUP / SAVE |

---

### 3.3 Performer Page

**Source:** `src/apps/sequencer/ui/pages/PerformerPage.cpp`  
**Reached by:** Page + Performer (non-modal) or Performer held (modal)  
**Modal?** Dual

**What it shows:** 8 track columns — track number (bright when fill active), activity rect, mute state, sequence progress bar, fill-amount bar.

**Function keys**

| Key | Label | Action |
|---|---|---|
| F0 | LATCH | Hold = latch mode for mute changes |
| F1 | SYNC | Hold = sync mode for mute changes |
| F2 | UNMUTE | Unmute all tracks (respects latch/sync) |
| F3 | FILL | Hold = enable fill on all tracks |
| F4 | CANCEL | Cancel pending mute requests |

**Buttons**

| Combo | Action |
|---|---|
| Track N | Toggle mute on track N |
| Shift + Track N | Toggle solo on track N |
| Step 8–15 held | Enable fill on track (step − 8) |
| Step 8–15 + Shift held | Hold fill (sustained, not momentary) |
| Step 0–7 held + encoder | Edit fill amount for that track (0–100%) |
| Encoder (no step held) | Edit project tempo |
| Encoder press (no step held) | Reset tempo to saved project value |
| F0 held | Latch mode |
| F1 held | Sync mode |
| F2 (UNMUTE) | Unmute all |
| F3 (FILL) held | Fill all tracks |

---

### 3.4 Song Page

**Source:** `src/apps/sequencer/ui/pages/SongPage.cpp`  
**Reached by:** Page + Step3  
**Modal?** No

**What it shows:** Song slot table — slot index, repeat count, per-track pattern assignments (T1–T8) or M if muted. Playback cursor, beat counter, slot progress bar when playing.

**Song data model:** 64 slots × (8 per-track pattern indices + 8 mute bits + 1 repeat count).

**Function keys**

| Key | Label | Action |
|---|---|---|
| F0 | CHAIN | Hold + Step N = chain pattern N into a new slot and start playing |
| F1 | ADD | Append new slot at the end |
| F1 + Shift | INSERT | Insert slot before the selected slot |
| F2 | REMOVE | Remove selected slot |
| F3 | DUPL | Duplicate selected slot |
| F4 | PLAY/STOP | Play song from selected slot / stop song |

**Buttons**

| Combo | Action |
|---|---|
| Step N | Set all-track pattern for selected slot to N (0–15) |
| Track T held + Step N | Set pattern for track T in selected slot to N |
| Track T held + encoder press | Toggle mute for track T in selected slot |
| Track T held + encoder | Edit track T's pattern in selected slot |
| Shift + Step N | Set repeat count for selected slot to N+1 (range 1–16) |
| Left / Right | Move selected slot cursor |
| Shift + Left / Right | Swap selected slot with neighbour |
| Encoder | Move selected slot cursor |
| Shift + Encoder | Edit repeat count for selected slot |
| Encoder press (no track held) | Play song from selected slot |
| F0 held + Step N | Chain pattern into new slot and start |
| Shift + F1 (INSERT) | Insert slot before current |
| Shift + F4 | Play song synced (if clock running) |
| Shift + Page | Context menu: INIT |

---

### 3.5 Tempo Page (modal)

**Source:** `src/apps/sequencer/ui/pages/TempoPage.cpp`  
**Reached by:** Tempo key (always modal — closes when Tempo released or confirmed)  
**Modal?** Yes

**What it shows:** Large BPM value (Font.Normal), tap tempo indicator.

**Parameters**

| Parameter | Range | Stored in |
|---|---|---|
| BPM | 1.0–1000.0 | `Project._tempo` (routable) |

**Buttons**

| Combo | Action |
|---|---|
| Encoder rotate | ±1 BPM |
| Shift + Encoder | ±10 BPM |
| Encoder press (while turning) | ±0.1 BPM fine |
| Any key | Tap tempo (accumulates timestamps → compute BPM) |

---

## 4. Note Track Pages

Note track: stepped CV+gate sequencer. Most parameter-rich track type.

---

### 4.1 Note Sequence Edit Page

**Source:** `src/apps/sequencer/ui/pages/NoteSequenceEditPage.cpp`  
**Reached by:** Page + Step0 (Note track selected)  
**Modal?** No

**What it shows:** 16-step grid with loop-point markers. Per step: gate box (filled = active), current-step highlight (playback cursor), per-layer value bar or symbol. Header: pattern-follow flag. Footer: active layer family.

---

#### Per-step layers

| Layer | Type / Range | Notes |
|---|---|---|
| Gate | bool | Whether the step triggers a note |
| GateProbability | 0–15 | Probability the gate fires (15 = always) |
| GateOffset | –8 to +7 | Sub-step timing offset |
| Slide | bool | Legato / portamento into next step |
| BypassScale | bool | Treat note as raw chromatic MIDI note, ignoring scale |
| Retrigger | 0–7 | Number of retriggers within the step |
| RetriggerProbability | 0–15 | Probability of retrigger firing |
| Length | 0–15 | Gate length (fraction of step duration) |
| LengthVariationRange | –8 to +7 | Random length variation range |
| LengthVariationProbability | 0–15 | Probability of length variation |
| Note | –64 to +63 | Scale degree (or raw MIDI if BypassScale) |
| NoteVariationRange | –64 to +63 | Random note variation range |
| NoteVariationProbability | 0–15 | Probability of note variation |
| Condition | 0–127 | Conditional trigger (fills, pattern-based conditions) |
| StageRepeats | 0–7 | How many times step repeats before advancing (Free mode) |
| StageRepeatsMode | 0–7 | How repeats affect gate/note on each repetition |

**Total: 16 layers per step.**

---

#### Function key families

| Key | Family | Layers cycled on repeated press |
|---|---|---|
| F0 | GATE | Gate → GateOffset → GateProbability → (back to Gate) |
| F1 | RETRIG | Retrigger → RetriggerProbability → StageRepeats → StageRepeatsMode (Free mode only) → (back) |
| F2 | LENGTH | Length → LengthVariationRange → LengthVariationProbability → (back) |
| F3 | NOTE | Note → NoteVariationRange → NoteVariationProbability → Slide → BypassScale → (back) |
| F4 | COND | Condition |

**Shift + F key (jump directly to sub-layer):**

| Combo | Effect |
|---|---|
| Shift + F0 | Jump to Gate layer |
| Shift + F1 | Jump to StageRepeats (Free mode only) |
| Shift + F2 | Jump to StageRepeatsMode (Free mode only); **if steps selected**: tie notes (set all to max length + same pitch) |
| Shift + F3 | Jump to Slide layer |
| Shift + F4 | Jump to Condition layer |

**F key + Step (set octave note):**

| Combo | Effect |
|---|---|
| F0 held + Step N | Set step N note = 1 × notesPerOctave |
| F1 held + Step N | Set step N note = 2 × notesPerOctave |
| F2 held + Step N | Set step N note = 3 × notesPerOctave |
| F3 held + Step N | Set step N note = 4 × notesPerOctave |
| F4 held + Step N | Set step N note = 5 × notesPerOctave |

#### Step interactions

| Combo | Action |
|---|---|
| Step N (Gate layer active) | Toggle gate on step N |
| Step N double-tap (other layers) | Toggle gate on step N |
| Hold Step N (Immediate mode) | Select step N (selection clears on release) |
| Shift + Step N | Enter Persist mode, select step N (stays selected after release) |
| Hold Step A + Hold Step B | Select both steps (Immediate mode) |

#### Step selection — Persist mode operations

| Combo | Effect |
|---|---|
| Shift double-tap (no selection) | Select all steps |
| Shift double-tap (selection active) | Clear selection |
| Double-tap Step N (Persist, no other step held) | Select all steps with equal layer value (`selectEqualSteps`) |
| Two steps held + double-tap one (Persist) | Select steps at regular intervals between the two held steps |

#### Encoder

| State | Effect |
|---|---|
| No steps selected | Cycle to next layer within current family |
| Steps selected | Edit layer value for all selected steps simultaneously |
| Shift held + selection + chromatic Note layer | Transpose selected notes ±1 octave |

**Encoder press:** Toggle gate on selected steps (set all on; if all already on, set all off).

#### Navigation

| Combo | Effect |
|---|---|
| Left | Scroll to previous 16-step section (0–3; total 64 steps) |
| Right | Scroll to next section |
| Shift + Left | Shift selected steps left by 1 within loop bounds |
| Shift + Right | Shift selected steps right by 1 within loop bounds |

#### Quick-edit (Page + Step8–15)

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

#### Other

| Combo | Effect |
|---|---|
| Page + Step6 | Undo — revert sequence to snapshot taken on page entry (or last gate/note edit) |
| Shift + Page | Context menu |
| Page double-tap | Context menu (same) |
| MIDI note-on (Note layer, steps selected, not recording) | Set note on selected steps + enable gate |

**Detail overlay:** When encoder is turned with steps selected (non-Gate layer), a detail popup shows current value for the first selected step. Disappears after 500ms (Persist mode) or immediately on selection clear.

**Context menu (Shift+Page)**
- **INIT** — reset sequence to default
- **COPY** — copy sequence to clipboard
- **PASTE** — paste from clipboard
- **DUPL** — duplicate sequence to another pattern slot
- **GEN** — open GeneratorSelectPage (Euclidean / Random)

---

### 4.2 Note Sequence Page (list view)

**Source:** `src/apps/sequencer/ui/pages/NoteSequencePage.cpp`  
**Reached by:** Page + Step1 (Note track)  
**Modal?** No

**Parameters displayed / editable**

| Parameter | Type / Range | Notes |
|---|---|---|
| FirstStep | 0–63 | Start of active step range |
| LastStep | 0–63 | End of active step range |
| RunMode | Forward / Backward / PingPong / Random / etc. | Sequence playback direction |
| Divisor | Clock divisor | Controls sequence tempo relative to master |
| ResetMeasure | 0–N measures | Reset period (0 = no auto-reset) |
| Scale | Built-in or user scale index | See §2.6 for scale list |
| RootNote | 0–11 (C–B) | Root of the selected scale |

**Buttons:** Standard ListPage. Context menu via Shift+Page.

---

### 4.3 Note Track Config (Track Page, Note type)

**Source:** `src/apps/sequencer/ui/pages/TrackPage.cpp` (Note list model: `NoteTrackListModel.h`)  
**Reached by:** Page + Step2 (Note track)

**Track-level parameters**

| Parameter | Type / Range | Notes |
|---|---|---|
| Track type | Note | Changing resets track |
| Play mode | Aligned / Free | |
| Fill mode | None / Gates / NextPattern / Condition | |
| Fill muted | On / Off | Mutes track during fill |
| CV update mode | Gate / Always | CV changes only on gate, or continuously |
| Slide time | 0–100% (routable) | Portamento duration |
| Octave | –10 to +10 (routable) | Octave offset applied to all steps |
| Transpose | –100 to +100 semitones (routable) | Semitone offset |
| Rotate | –64 to +64 steps (routable) | Rotates step sequence |
| Gate probability bias | (routable) | Shifts all gate probabilities |
| Retrigger probability bias | (routable) | Shifts all retrigger probabilities |
| Length bias | (routable) | Shifts all note lengths |
| Note probability bias | (routable) | Shifts all note variation probabilities |

---

## 5. Curve Track Pages

Curve track: CV automation/LFO track. No gate output — pure continuous CV modulation.

---

### 5.1 Curve Sequence Edit Page

**Source:** `src/apps/sequencer/ui/pages/CurveSequenceEditPage.cpp`  
**Reached by:** Page + Step0 (Curve track)

**What it shows:** 16-step grid where bar height represents the CV value. Header shows current layer. Each step has a curve shape drawn inside the bar.

#### Per-step layers

| Layer | Description |
|---|---|
| Shape | Curve shape: Linear, Exponential, Logarithmic, Sine, Square, etc. |
| Min | Minimum CV output value for this step |
| Max | Maximum CV output value for this step |
| ShapeVariation | Alternative shape applied probabilistically |
| ShapeVariationProbability | Probability of shape variation firing |

**Total: 5 layers per step.**

#### Function keys

| Key | Layer / effect |
|---|---|
| F0 | Shape layer |
| F1 | Min layer |
| F2 | Max layer |
| F3 | ShapeVariation layer |
| F4 | ShapeVariationProbability layer |

**F key held + encoder:** Edit that function's parameter directly without switching to that layer (unique to Curve edit page).

**Step interactions, selection, navigation, quick-edit, undo:** Same mechanics as NoteSequenceEditPage.

**Quick-edit slots:**

| Key | Parameter |
|---|---|
| Page + Step8 | FirstStep |
| Page + Step9 | LastStep |
| Page + Step10 | RunMode |
| Page + Step11 | Divisor |
| Page + Step12 | ResetMeasure |
| Page + Step13 | Range (CV output range: ±1V, ±2V, ±5V, 0–10V) |

**Context menu:** INIT / COPY / PASTE / DUPL / GEN

---

### 5.2 Curve Sequence Page (list view)

**Reached by:** Page + Step1 (Curve track)

**Parameters:** FirstStep, LastStep, RunMode, Divisor, ResetMeasure, Range (output voltage range).

**Buttons:** Standard ListPage.

---

### 5.3 Curve Track Config (Track Page, Curve type)

**Track-level parameters**

| Parameter | Type / Range | Notes |
|---|---|---|
| Track type | Curve | |
| Play mode | Aligned / Free | |
| Fill mode | None / Variation / NextPattern / Invert | |
| Mute mode | LastValue / Zero (0V) / Min / Max | Output when track is muted |
| Offset | CV offset (routable) | |
| Rotate | Step rotation (routable) | |
| Shape probability bias | (routable) | |
| Curve min | Output range minimum (routable) | |
| Curve max | Output range maximum (routable) | |

---

## 6. Stochastic Track Pages

Stochastic track: probability-driven sequencer. Notes and gates determined stochastically at runtime from weighted step definitions. Reproducible via seed.

---

### 6.1 Stochastic Sequence Edit Page

**Source:** `src/apps/sequencer/ui/pages/StochasticSequenceEditPage.cpp`  
**Reached by:** Page + Step0 (Stochastic track)

#### Per-step layers

| Layer | Description |
|---|---|
| Gate | Whether this step can trigger |
| GateProbability | Per-step gate probability |
| Retrigger | Retrigger count |
| RetriggerProbability | Probability of retrigger |
| Length | Gate length |
| LengthVariationRange | Random length variation range |
| LengthVariationProbability | Probability of length variation |
| Note | Base note (scale degree) |
| NoteVariationRange | Random note variation range |
| NoteVariationProbability | Probability of note variation |
| Condition | Conditional trigger |
| RestProbability2 | Additional rest tier (2x weight) |
| RestProbability4 | Additional rest tier (4x weight) |

#### Function keys

| Key | Family / Layer |
|---|---|
| F0 | Gate family |
| F1 | Length family |
| F2 | Note family |
| F3 | Probability / rest layers |
| F4 | Condition |

**Step interactions, selection, navigation, quick-edit:** Same as NoteSequenceEditPage.

**Context menu:** INIT / COPY / PASTE / DUPL / GEN

---

### 6.2 Stochastic Sequence Page (list view)

**Reached by:** Page + Step1 (Stochastic track)

**Parameters:** Seed (RNG reseed value), RestProbability (global rest chance), FirstStep, LastStep, RunMode, Divisor, ResetMeasure, Scale, RootNote.

**Buttons:** Standard ListPage.

---

### 6.3 Stochastic Track Config (Track Page, Stochastic type)

Same parameters as Note Track Config (§4.3) — playMode, fillMode, cvUpdateMode, slideTime, octave, transpose, rotate, biases.

---

## 7. Arp Track Pages

Arp track: arpeggiator. Takes a set of notes (from sequence steps or live MIDI input) and generates an arpeggiated CV+gate output.

---

### 7.1 Arp Sequence Edit Page

**Source:** `src/apps/sequencer/ui/pages/ArpSequenceEditPage.cpp`  
**Reached by:** Page + Step0 (Arp track)

#### Per-step layers

Same structure as NoteSequence:

| Layer | Description |
|---|---|
| Gate | Step gate |
| GateProbability | |
| Retrigger / RetriggerProbability | |
| Length / LengthVariationRange / LengthVariationProbability | |
| Note | Scale degree (note fed into arpeggiator) |
| NoteVariationRange / NoteVariationProbability | |
| Condition | |
| StageRepeats / StageRepeatsMode | |
| Slide / BypassScale | |

**Arpeggiator parameters (part of ArpSequence / ArpTrack):**

| Parameter | Options |
|---|---|
| Arp mode | Up / Down / UpDown / DownUp / UpAndDown / DownAndUp / Converge / Diverge / Random |
| Hold | On / Off — sustain active notes |
| Octaves | Number of octaves to span |
| Gate length | Gate length for each arp note |
| Divisor | Arp rate (clock divisor) |

**Step interactions, selection, navigation, quick-edit:** Same as NoteSequenceEditPage.

**Page + Step15 on OverviewPage (Arp track):** Toggle MIDI keyboard input mode (live note selection fed into arpeggiator).

**Context menu:** INIT / COPY / PASTE / DUPL / GEN

---

### 7.2 Arp Sequence Page (list view)

**Reached by:** Page + Step1 (Arp track)

**Parameters:** Arp mode, Hold, Octaves, GateLength, Divisor, FirstStep, LastStep, RunMode, ResetMeasure.

**Buttons:** Standard ListPage.

---

### 7.3 Arp Track Config (Track Page, Arp type)

Same parameters as Note Track Config — playMode, fillMode, fillMuted, cvUpdateMode, slideTime, octave, transpose, rotate, biases.

---

## 8. Quantizer Track Pages

Quantizer track: reads an external CV input, quantizes it to the active scale, holds until retriggered.

---

### 8.1 Quantizer Sequence Edit Page

**Source:** `src/apps/sequencer/ui/pages/QuantizerSequenceEditPage.cpp`  
**Reached by:** Page + Step0 (Quantizer track)

**What it shows:** A gate lane editor (uses embedded NoteSequence). Only the `gate` field per step matters — note values are ignored. Same visual as NoteSequenceEditPage without the NOTE/SLIDE/COND families.

**Function keys**

| Key | Layer |
|---|---|
| F0 | Gate (only layer) |

**Step, selection, navigation, quick-edit, undo:** Same mechanics as NoteSequenceEditPage.

---

### 8.2 Quantizer Sequence Page (list view)

**Reached by:** Page + Step1 (Quantizer track)

**Parameters**

| Parameter | Options |
|---|---|
| FirstStep | 0–63 |
| LastStep | 0–63 |
| RunMode | Forward / Backward / PingPong / Random |
| Divisor | Clock divisor |
| Scale | Scale index |
| RootNote | 0–11 |

**Buttons:** Standard ListPage.

---

### 8.3 Quantizer Track Config (Track Page, Quantizer type)

**Track-level parameters**

| Parameter | Options |
|---|---|
| Track type | Quantizer |
| Input source | CV In 1–4, or Track 1–8 (reads that track's CV output) |
| Trigger mode | Free (fires on pitch change) / Internal (own gate sequence) / External (another track's gate) |
| Trigger track | Which track to use for External trigger mode |
| Octave | ±10, applied after quantize |
| Transpose | ±100 scale degrees, applied after quantize |
| Pattern follow | On / Off |

**Engine output:** Quantized CV on CV out; ~21ms gate pulse on gate out for each note change.

---

## 9. MIDI/CV Track Page

MIDI/CV track: MIDI-to-CV converter. No step sequencer — event-driven by incoming MIDI notes.

---

### 9.1 MIDI/CV Track Config (Track Page, MidiCv type)

**Source:** `src/apps/sequencer/ui/model/MidiCvTrackListModel.h`  
**Reached by:** Page + Step2 (MidiCv track)

**Parameters**

| Parameter | Options |
|---|---|
| Track type | MidiCv |
| MIDI source port | UART DIN / USB-MIDI |
| MIDI source channel | 1–16 / Any |
| Voices | 1–8 polyphonic voices |
| Voice config | Pitch / Velocity / PitchVelocity / PitchVelocityPressure |
| Note priority | LastNote / FirstNote / LowestNote / HighestNote |
| Low note filter | MIDI note 0–127 |
| High note filter | MIDI note 0–127 |
| Pitch bend range | Semitones |
| Modulation range | CC01 (mod wheel) CV output range |
| Retrigger | Retrigger on note overlap |
| Transpose | Semitone offset |
| Arpeggiator: mode | Up / Down / UpDown / etc. |
| Arpeggiator: hold | On / Off |
| Arpeggiator: octaves | Number of octaves |
| Arpeggiator: gate length | Per-note gate |
| Arpeggiator: divisor | Arp rate |

**No sequence pages** — MidiCv track has no pattern/step editing.

**CV output signals:** Pitch (1V/oct), optionally Velocity and Pressure on additional CV outputs per `VoiceConfig`.

---

## 10. Modal / Utility Pages

---

### 10.1 Context Menu Page

**Source:** `src/apps/sequencer/ui/pages/ContextMenuPage.cpp`  
**Reached by:** Shift+Page or Page double-tap (from any page that has a context menu)  
**Modal?** Yes

**What it shows:** Floating action list. Items vary per parent page (see per-page entries above).

**Buttons**

| Combo | Action |
|---|---|
| F0–F4 | Select the corresponding menu item |
| Release Shift | Close menu (when opened via Shift+Page) |
| Any other key | Close without action |

---

### 10.2 Quick Edit Page (overlay)

**Source:** `src/apps/sequencer/ui/pages/QuickEditPage.cpp`  
**Reached by:** Page + Step8–14 from NoteSequenceEditPage or OverviewPage  
**Modal?** Yes (stays open while Page is held)

**What it shows:** Parameter name + current value + progress bar.

**Buttons**

| Combo | Action |
|---|---|
| Left / Right | Edit value ±1 |
| Shift + Left / Right | Edit value ±10 (coarse) |
| Encoder | Edit value |
| Release Page | Close page |

---

### 10.3 Generator Select Page

**Source:** `src/apps/sequencer/ui/pages/GeneratorSelectPage.cpp`  
**Reached by:** Context menu → GEN from any sequence edit page  
**Modal?** Yes

**What it shows:** List of available generators. Currently: **Euclidean**, **Random**.

**Buttons:** Encoder selects. F4 confirm / any other key cancel.

---

### 10.4 Generator Page

**Source:** `src/apps/sequencer/ui/pages/GeneratorPage.cpp`  
**Reached by:** Generator Select → choose a generator  
**Modal?** Yes

**What it shows:** Generator parameters. Preview applied to the sequence in real time.

**Euclidean generator parameters:**

| Parameter | Range | Effect |
|---|---|---|
| Steps | 1–64 | Total steps in generated pattern |
| Pulses | 0–Steps | Number of active (gate on) steps |
| Offset | 0–Steps | Rotation of the Euclidean pattern |

**Random generator parameters:**

| Parameter | Range | Effect |
|---|---|---|
| Steps | 1–64 | Total steps |
| Gate probability | 0–100% | How often gates fire |
| Note min | Scale degree | Minimum random note |
| Note max | Scale degree | Maximum random note |
| Note scale | Scale index | Scale for random notes |

**Buttons**

| Combo | Action |
|---|---|
| Left / Right | Edit focused parameter |
| Encoder | Navigate / edit |
| F4 | Confirm — write generated values to sequence |
| Any other key | Cancel — revert sequence to state before opening |

---

### 10.5 Text Input Page

**Source:** `src/apps/sequencer/ui/pages/TextInputPage.cpp`  
**Reached by:** Encoder press on a name row (ProjectPage, UserScalePage)  
**Modal?** Yes

**What it shows:** Character picker with cursor.

**Buttons**

| Combo | Action |
|---|---|
| Step keys | Select character group |
| Encoder | Scroll through characters |
| F4 | Confirm |
| F3 | Delete character |
| F0 | Cancel |

---

### 10.6 File Select Page

**Source:** `src/apps/sequencer/ui/pages/FileSelectPage.cpp`  
**Reached by:** Project LOAD or SAVE AS  
**Modal?** Yes

**What it shows:** SD card file browser — slot list with project names.

**Buttons**

| Combo | Action |
|---|---|
| Encoder | Navigate slots |
| Encoder press or F4 | Confirm selection |
| Any other key | Cancel |

---

### 10.7 Confirmation Page

**Source:** `src/apps/sequencer/ui/pages/ConfirmationPage.cpp`  
**Reached by:** Destructive actions (System page operations, Page+Track7 entry)  
**Modal?** Yes

**Buttons**

| Combo | Action |
|---|---|
| F4 | Yes / confirm |
| Any other key | No / cancel |

---

### 10.8 Busy Page

**Source:** `src/apps/sequencer/ui/pages/BusyPage.cpp`  
**Reached by:** Automatically during SD / flash operations  
**Modal?** Yes (blocks all input)

No interaction. Shown during file load/save. Disappears when operation completes.

---

### 10.9 Startup / Intro / Asteroids

| Page | Reach | Notes |
|---|---|---|
| `StartupPage` | Boot | Firmware init screen, no input |
| `IntroPage` | Boot (if `CONFIG_ENABLE_INTRO`) | Optional splash, any key skips |
| `AsteroidsPage` | Page+Step15 on ProjectPage (if `CONFIG_ENABLE_ASTEROIDS`) | Easter egg mini-game |

---

## 11. Held-Button Overlay Catalogue

The most hidden affordances — behaviors triggered by holding one button and pressing another.

| Held | Pressed / turned | Page(s) | Effect |
|---|---|---|---|
| **Page** | — | All | LEDs switch to page-select grid |
| **Page** | Step8–14 | Edit pages, Overview | Open QuickEditPage for that parameter |
| **Page** | Step15 | NoteSequenceEdit, Overview | Toggle pattern-follow display |
| **Page** | Step15 | Overview (Arp track) | Toggle MIDI keyboard mode |
| **Page** | Step15 | ProjectPage | Asteroids easter egg |
| **Page** | Step6 | NoteSequenceEdit | Undo sequence to last snapshot |
| **Page** | Play | Any | Toggle recording |
| **Pattern** | (hold) | Any except Pattern | Pattern page modal overlay |
| **Performer** | (hold) | Any except Performer | Performer page modal overlay |
| **Shift** | Step N (Immediate mode) | Edit pages | Switch to Persist selection, select step N |
| **Shift** | Shift double-tap | Edit pages (Persist) | Select all / clear selection |
| **Shift** | Encoder | Note layer (chromatic) | Transpose selected steps ±1 octave |
| **Shift** | Encoder | ListPage pages | Coarse step ×10 |
| **Shift** | F2 (with selection) | NoteSequenceEdit | Tie selected notes |
| **Step 0–7** | Encoder | PerformerPage | Edit fill amount for that track |
| **Step 8–15** | — | PerformerPage | Enable fill on track (step − 8) |
| **Step 8–15** + Shift | — | PerformerPage | Hold fill (sustained) |
| **F0–F4** | Encoder | CurveSequenceEdit | Edit that function's parameter directly |
| **Track T** | Step N | PatternPage | Change track T's pattern only |
| **Track T** | Step N | SongPage | Set track T's pattern in selected slot |
| **Track T** | Encoder | SongPage | Edit track T's pattern in selected slot |
| **Track T** | Encoder press | SongPage | Toggle mute for track T in selected slot |
| **Shift** | — | PatternPage | LEDs show edit pattern instead of active |
| Two **Step** keys | — | PatternPage | Chain two patterns into song |
| Two **Step** keys (Persist double-tap) | — | Edit pages | Select steps at regular intervals between them |
| **F0** (LATCH) | Step / Track | PatternPage, PerformerPage | Latch mode |
| **F1** (SYNC) | Step / Track | PatternPage, PerformerPage | Sync mode |

---

## 12. Cross-Reference: Combo → Page → Effect

Alphabetical by combo.

| Combo | Page | Effect |
|---|---|---|
| Double-tap Page | Any (with context menu) | Open context menu |
| Encoder | ListPage pages | Navigate rows |
| Encoder | NoteSequenceEdit (no selection) | Cycle layer within family |
| Encoder | NoteSequenceEdit (with selection) | Edit layer value for selected steps |
| Encoder press | NoteSequenceEdit | Toggle gate on selection |
| Encoder press | SongPage (no track held) | Play song from selected slot |
| Encoder press | TempoPage | Fine ±0.1 BPM |
| F0–F4 | NoteSequenceEdit | Select / cycle layer family |
| F0–F4 held + Step N | NoteSequenceEdit | Set step N note to octave N |
| F0 (LATCH) held | PatternPage, PerformerPage | Latch mode |
| F1 (SYNC) held | PatternPage, PerformerPage | Sync mode |
| F2 (SNAP) | PatternPage | Create snapshot |
| F2 (REVERT) | PatternPage | Revert to snapshot |
| F3 (COMMIT) | PatternPage | Commit snapshot |
| F4 (CANCEL) | PatternPage, PerformerPage | Cancel pending requests |
| Left / Right | NoteSequenceEdit | Scroll 16-step section |
| Left / Right | PatternPage, SongPage, QuickEditPage, ListPage | Edit / navigate |
| Page + Left | Any | Overview page |
| Page + Pattern | Any | Pattern page (non-modal) |
| Page + Performer | Any | Performer page (non-modal) |
| Page + Play | Any | Toggle recording |
| Page + Step0 | Any | Sequence Edit page (type-dependent) |
| Page + Step1 | Any | Sequence page (type-dependent) |
| Page + Step2 | Any | Track page |
| Page + Step3 | Any | Song page |
| Page + Step6 | NoteSequenceEdit | Undo sequence |
| Page + Step7 | Any | Monitor page |
| Page + Step8–14 | NoteSequenceEdit, OverviewPage | QuickEditPage for that parameter |
| Page + Step15 | NoteSequenceEdit, OverviewPage | Toggle pattern-follow |
| Page + Step15 | OverviewPage (Arp) | Toggle MIDI keyboard mode |
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
| Play | Any | Toggle play / stop |
| Shift + Encoder | NoteSequenceEdit (Note+chromatic) | Octave transpose |
| Shift + Encoder | ListPage, TempoPage | Coarse edit ×10 |
| Shift + F0 | NoteSequenceEdit | Jump to Gate layer |
| Shift + F1 | NoteSequenceEdit | Jump to StageRepeats (Free mode) |
| Shift + F2 + selection | NoteSequenceEdit | Tie notes |
| Shift + F3 | NoteSequenceEdit | Jump to Slide layer |
| Shift + F4 | NoteSequenceEdit | Jump to Condition layer |
| Shift + Left / Right | NoteSequenceEdit | Shift steps left / right |
| Shift + Left / Right | SongPage | Swap slots |
| Shift + Page | Any | Context menu |
| Shift + Play | Any | Toggle play (alt/sync mode) |
| Shift + Step | NoteSequenceEdit | Persist-mode step selection |
| Shift + Step | PatternPage | Set edit pattern |
| Shift + Step | SongPage | Set slot repeat count |
| Shift + Track | PerformerPage | Solo track |
| Step (held) | NoteSequenceEdit | Immediate-mode step selection |
| Step 0–7 + Encoder | PerformerPage | Edit fill amount |
| Step 8–15 held | PerformerPage | Enable fill |
| Step N | NoteSequenceEdit (Gate layer) | Toggle gate |
| Step N | PatternPage | Change playing pattern |
| Step N | SongPage | Set slot pattern |
| Tempo | Any | Tempo page (modal) |
| Track N | Any | Select track N |
| Track N | PerformerPage | Toggle mute |

---

## 13. Observations for Redesign

These are observations, not decisions. They point to friction in the current UI that the redesign should address.

### Buried config pages

Project, Layout, Routing, MIDI Output, User Scale, and System are all reachable **only** via Page + Track modifier. A user who doesn't know the key map cannot discover these pages. No on-screen label or LED hints their existence.

### ListPage monoculture for all config

Project, Layout, Track, Routing, MIDI Output, User Scale — all use the same flat scrollable list (Left/Right ±1, Shift coarse, encoder navigate). Any setting requires: know the page → navigate there → find the row → edit. No visual hierarchy, no grouping, no shortcuts to frequently-used rows.

### NoteSequenceEditPage is the densest page by far

16 layers × 64 steps across 4 sections. Users must memorize: F-key-to-family, cycle-order-within-family, Shift+F-key shortcuts, F-key+Step-for-octave, two step-selection modes (Immediate vs Persist), Page+Step6 undo, and 8 quick-edit slots. There is no on-screen legend for any of this.

### Inconsistent Shift+Encoder semantics

- NoteSequenceEdit (Note layer, chromatic): Shift+encoder = ±1 octave
- ListPage pages: Shift+encoder = coarse step ×10
- TempoPage: Shift+encoder = ×10 BPM

Same physical gesture does three different things depending on context. No visual indicator.

### Hidden affordances — nothing is labelled

None of the following are discoverable from the UI:
- `Page+Step6` = undo (NoteSequenceEdit) — no LED, no label
- `Page+Step15` = pattern-follow toggle — invisible until you read the header status flag
- `Fkey+Step` = set note to octave N — unlabelled
- Asteroids via `Page+Step15` on ProjectPage
- Two-step chain in PatternPage
- Two-step interval fill in step selection (Persist mode)
- `Page+Step6` undo even exists

### Pattern/Performer modal vs. non-modal duality

Both Pattern and Performer can be reached as a modal overlay (hold the button — closes on release) or as a pinned page (Page+key — stays open). This is powerful but creates implicit state: easy to accidentally stay on the wrong version. No visual distinction between the two states on screen.

### 64-step sections require Left/Right to navigate

Steps 17–64 are accessed by pressing Left/Right to scroll through sections 0–3. No visual indication of how many sections exist or which section is current (only a small position indicator). Pattern-follow reduces this burden but must be explicitly toggled.

### Quick-edit step numbers are arbitrary

`Page+Step8` = FirstStep, `Page+Step9` = LastStep, etc. The only discovery aid is which LEDs light green while Page is held. Users must memorize the slot-to-parameter mapping. Changing one parameter (e.g. Divisor) requires knowing it's always Step11 regardless of page context.

### Track-type config has no dedicated page

All per-track-type settings live in the generic TrackPage (Page+Step2) as a flat list. There is no visual indication that the list changes when you change track type, and no way to see both the sequence edit page and the track config simultaneously.

### No BPM on primary views

By design (locked in IA), BPM lives only in the Tempo modal. Users must press Tempo to check or adjust tempo — it can't be monitored passively during performance.
