# UI State: Pages and Key Combos

What each page is, how to reach it, and its notable inputs. How the UI is built is in
[features/ui-system.md](features/ui-system.md); per-track Steps pages are described in the track
docs; the target redesign is [ui-redesign/00-ia.md](ui-redesign/00-ia.md). Keep this file to what
the code does today.

Naming: step buttons are `Step0..Step15` as in the code (hardware label = N+1); function keys are
F1-F5 (code `F0..F4`). Page + X means hold Page and press X.

## Physical surface

8 track buttons, 16 step buttons, 5 function keys, 8 global buttons (Play, Tempo, Pattern,
Performer, Left, Right, Shift, Page) and one encoder with a push. Shift and Page are modifiers.

## Global navigation (`TopPage`, `PageKeyMap`)

| Input | Goes to |
|---|---|
| Track N | selects track N |
| Page + Track 0 / 1 / 2 / 3 / 4 | Project / Layout / Routing / MIDI Output / User Scale |
| Page + Track 7 | System (after a confirmation) |
| Page + Step 0 | Steps edit page of the selected track type |
| Page + Step 1 | Sequence overview page of the selected track type |
| Page + Step 2 | Track page |
| Page + Step 3 | Song |
| Page + Step 7 | Monitor |
| Page + Left | Overview (all tracks) |
| Page + Tempo | Clock setup |
| Page + Pattern / Page + Performer | Pattern / Performer as a pinned page |
| Pattern / Performer (hold) | the same page as a modal overlay that closes on release |
| Tempo | Tempo page (modal) |
| Play | play/stop; Shift + Play uses the clock setup's shift mode; Page + Play toggles recording |
| Shift + Page, or double-tap Page | context menu of the current page |
| Page + Step 8-15 | quick edit overlay on Steps pages ([below](#quick-edit-and-common-steps-inputs)) |

Holding Page (without Shift) switches the LEDs to a page-select grid.

## Pages

| Page | Reach | Footer / notes |
|---|---|---|
| Project | Page + Track 0 | config list; context menu INIT, LOAD, SAVE, SAVE AS, ROUTE; Page + Step 15 starts the Asteroids easter egg |
| Layout | Page + Track 1 | MODE / LINK / GATE / CV (+ COMMIT): track mode, link, jack assignment |
| Track | Page + Step 2 | per-track config list with sections ([config-pages](features/config-pages.md)) |
| Routing | Page + Track 2 | PREV / NEXT / INIT / LEARN / COMMIT ([routing](features/routing.md)) |
| MIDI Output | Page + Track 3 | PREV / NEXT / INIT / COMMIT ([midi-io](features/midi-io.md)) |
| User Scale | Page + Track 4 | USER1-4 ([scales](features/scales.md)) |
| System | Page + Track 7 | CAL / UTILS / UPDATE / SETTINGS: calibration, format, firmware update, settings backup |
| Clock setup | Page + Tempo | config list ([clock-sync](features/clock-sync.md)) |
| Monitor | Page + Step 7 | CV IN / CV OUT / MIDI / STATS |
| Pattern | Pattern key | LATCH / SYNC / SNAP (REVERT) / COMMIT / CANCEL |
| Performer | Performer key | LATCH / SYNC / UNMUTE / FILL / CANCEL |
| Overview | Page + Left | all 8 tracks condensed, with quick edit for the selected track |
| Song | Page + Step 3 | CHAIN / ADD / REMOVE / DUPL / PLAY ([song-mode](features/song-mode.md)) |
| Tempo | Tempo key | modal; BPM, tap, nudge, swing ([clock-sync](features/clock-sync.md)) |
| Sequence overview (x5) | Page + Step 1 | config-style list for Note, Curve, Stochastic, Arp, Quantizer |
| Steps edit | Page + Step 0 | Note: GATE/RETRIG/REPEAT/NOTE/COND; Curve: SHPE/SKEW/LEN/LVL/OFST; Stochastic: NOTE/OCT/LEN/LOOP; Arp: NOTE/RHYTHM/MOD/ARP; Quantizer: GATE/SRCE/TRIG/TUNE/LOOP |
| MidiCv | Page + Step 0 / 1 | both open the Track page; there is no Steps page |
| Generator select / Generator | context menu GEN | [sequence-generators](features/sequence-generators.md) |
| File select, Text input, Confirmation, Busy | opened by other pages | slot picker, character picker (footer OK, CANCEL, BACKSPACE, DELETE, CLEAR), yes/no, blocking wait |
| Context menu, Quick edit | Shift + Page, Page + Step 8-15 | overlays |
| Startup, Intro, Asteroids | boot, optional, easter egg | `CONFIG_ENABLE_INTRO`, `CONFIG_ENABLE_ASTEROIDS` |

Config pages follow the modeless rules in [features/config-pages.md](features/config-pages.md):
the encoder edits the selected row, Left/Right move the cursor, F1-F5 jump to sections.

### Pattern page

Step N changes the playing pattern (Immediate by default; LATCH held latches; SYNC held syncs to the
next boundary; the `PatternChange` user setting can change the default). Shift + Step N sets the
edit pattern. Hold Track T + Step N changes track T only. Two steps held together chain patterns
into the song. SNAP creates a snapshot; REVERT/COMMIT apply or discard it.

### Performer page

Track N toggles mute; Shift + Track N solos. Step 8-15 held fills track (step - 8), Step 0-7 held
plus the encoder edits that track's fill amount. With no step held the encoder edits the tempo, and
pressing the encoder resets it to the project tempo.

## Quick edit and common Steps inputs

| Input | Effect |
|---|---|
| Page + Step 8-12 (Note, Stochastic, Quantizer) | quick edit FirstStep, LastStep, RunMode, Divisor, ResetMeasure |
| Page + Step 8-11 (Curve) | FirstStep, SegmentCount, RunMode, Divisor |
| Page + Step 11 / 12 (Arp) | Divisor, ResetMeasure |
| Page + Step 15 | toggle pattern-follow display (Note, Curve) |
| Page + Step 6 | undo to the last in-memory snapshot (Note, Quantizer); loop on/off (Stochastic) |
| Left / Right | change section (4 sections of 16 steps) |
| Step hold, or Shift + Step | immediate or persistent step selection (`StepSelection`); the two-step interval fill selects steps between two presses |
| Context menu | per track type: INIT, COPY, PASTE, DUPL, GEN where the page supports them |

While a quick edit is open, Left/Right and the encoder edit the value and releasing Page closes it.

## Known awkwardness

Discoverability gaps that remain: global config pages are only reachable through Page + Track
combos; quick edit slots are only hinted by lit step LEDs; undo and pattern follow are unlabelled
combos. The redesign docs record the intended fixes.
