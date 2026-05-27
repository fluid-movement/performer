# Design Log

Tracks each loop iteration: what was studied, what principles were extracted, what variations were added, and what survived into the next round.

**Iteration state:** `next_tastemaker = 5` (Dirtywave M8) — Elektron, TE, Norns, Hapax covered  
**Current focus pages:** Note Track Edit (primary); Dashboard + Perform (secondary — 3 options each, stable)  
**Batch cadence:** 2 Explore turns → 1 Consolidate turn → repeat  
**Last turn type:** Explore (Hapax — Turn 4) + Note Track seed (3 initial options)  
**3-variation rule:** Each page shows exactly 3 numbered options (Option 1/2/3). New iterations replace the weakest. User references options by number.  
**Ledger:** `07-iteration-feedback.md` — read the "Avoid" section before starting any iteration.

---

## Iterations

### Turn 1 — Elektron (Digitakt / Syntakt)

**Principles extracted:**
- **Inverted row = active/selected**: active track/item gets a full-width Color.Bright fill; all content (text, step dots) drawn with Color.Low over it — high-contrast binary state, no ambiguity
- **4-char max label abbreviations**: TRIG, SRC, FLTR, AMP, LFO, MIDI — short enough to fit in a knob-grid cell without crowding
- **Step brightness encodes state + beat**: active trigger = Bright/MediumBright; every 4th step (beat 1) = slightly brighter or differently outlined; cursor/selected step = inverted (brightest fill)

**Variations added:**
- `drawDash_Elektron1` → Dashboard: inverted bright row for focused track, Low-colored content over it, beat-aware gate dots
- `drawEdit_Elektron1` → Track Edit: inverted cursor cell, outline-only for off-steps, brightness difference on beat positions
- `drawPerf_Elektron1` → Perform: inverted row for focused track, mini 16-slot pattern strip, active slot left bright in inverted row

**References used:**
- https://www.manualslib.com/manual/2166470/Elektron-Digitakt.html?page=16 — parameter inversion on selection
- https://www.soundonsound.com/reviews/elektron-digitakt — step brightness and playback cursor style
- Search: "Elektron Digitakt screen UI inverted selection highlight parameter knob mapping"

### Turn 2 — Teenage Engineering (OP-1 / OP-Z)

**Principles extracted:**
- **Large dominant value, tiny subordinate context**: the most important element is huge (Font.Normal or large bars); labels and metadata are nearly invisible (Color.Low, Font.Tiny). Extreme hierarchy, not uniform information density.
- **Graphic bar as primary encoding**: a horizontal fill bar communicates parameter value more immediately than a number. Use visual shapes (bar heights, fills) as primary, text as optional annotation.
- **Breathing room between sections**: separate logical zones with blank space and thin rules; don't pack content wall-to-wall. A half-full screen with clear zones beats a full screen with no hierarchy.

**Variations added:**
- `drawDash_TE2` → Dashboard: focused track gets large bottom-anchored bars (top 60%); all others compressed to a 3px dot strip (bottom 40%)
- `drawEdit_TE2` → Track Edit: tiny gate strip at top, large note name dominant in center, pitch bar below, tiny context label at bottom
- `drawPerf_TE2` → Perform: 8 equal columns, each shows track# tiny at top and pattern# in Font.Normal as the single dominant element

**References used:**
- https://teenage.engineering/products/op-1/original/overview — "vector based GUI, all graphics realtime"
- https://teenage.engineering/guides/op-1/original/synthesizer-mode — color-coded encoders, graphical not text-based
- Search: "Teenage Engineering OP-1 synthesizer screen display interface design"

### Turn 3 — Monome Norns

**Principles extracted:**
- **Brightness as the only differentiator**: use exactly 3 levels — Level 15 (Bright) for focus/active, Level 4 (≈ Color.Low) for inactive/context, and Level 0 (nothing drawn) for off. Don't use medium values as "also off but visible."
- **True black = off**: off-steps are not drawn at all (black background shows). The absence of light IS the information. Resist the urge to draw dim background fills — they add noise.
- **Calm, no-noise hierarchy**: no separator lines, no decorative rules. Brightness contrast alone creates hierarchy. The screen should feel serene, not busy.

**Variations added:**
- `drawDash_Norns3` → Dashboard: active steps drawn bright (focus) or dim (others); off-steps not drawn; focus track bright, others barely visible
- `drawEdit_Norns3` → Track Edit: large sparse cells (28×22px), on=bright filled, cursor-off=dim outline, off+no cursor=nothing drawn
- `drawPerf_Norns3` → Perform: 8 pattern numbers only, at 3 brightness levels (focus=Bright, active=Medium, muted=Low); no labels, no grids

**References used:**
- https://monome.org/docs/norns/study-2/ — 128×64 OLED, 16 brightness levels, state-based level changes
- https://llllllll.co/t/norns-display-gamma/44910 — "level 4 for dim, level 15 for active" as community standard
- Search: "norns OLED display brightness level visual hierarchy design"

---

## Batch 1 summary — 2026-05-21

**Turns completed:** 3
**Tastemakers covered:** Elektron (1), Teenage Engineering (2), Monome Norns (3)
**Total new variations:** 9 (3 per page × 3 pages: Dashboard, Track Edit, Perform)

**Standouts (agent opinion):**
- `drawEdit_Elektron1` — outlined off-steps + inverted cursor is very distinctive; the true inversion on the cursor step makes editing state immediately clear
- `drawDash_TE2` — the focus-track-bars-over-dot-strip layout is genuinely novel vs existing variants; shows the TE size-hierarchy principle clearly
- `drawEdit_Norns3` — the large sparse cells with true-black off-steps create a completely different feel; very readable at a glance

**Ready for user review.**

---

## Trunk variations (current best candidates — not yet user-confirmed)

These are written to the design language spec (`01-design-language.md`) and sit at
index 0 of their page's VARIATIONS array, labeled `★ TRUNK`. Challengers from the
loop must beat them to replace them.

| Page | Variation label | Core idea |
|---|---|---|
| Dashboard | `★ TRUNK — 3-level, inverted focus row` | Elektron inverted-row focus + Norns off=absent. Strict palette. Footer present. |
| Perform | `★ TRUNK — 8 rows, 3-level, footer` | 8-row layout, Bright = active pattern, Low = slot present, off = absent. Footer with LATCH/SYNC/SNAP/FILL. |

Track Edit trunk deferred — Track Edit is now 5 per-type pages; each gets its own trunk after the per-type design sessions (G7).

---

### Turn 4 — Squarp Hapax

**Principles extracted:**
- **Playback phase bar replaces gate dots**: Hapax's left screen shows a progress bar per track — how far through the current pattern playback has gone. This gives "where are we NOW" information vs gate-dots which show "what will play." Each track gets a wide horizontal fill bar; width = current step / total steps.
- **Track content type as single-char glyph**: Hapax uses icon notation for content type (note, automation, drum, MPE). Translated to PER|FORMER: compress 4-char type label to one char (N, C, S, A, Q) — frees horizontal space for the phase bar.
- **Muted tracks = absent bar**: Hapax's left screen only shows progress for playing tracks. Muted tracks show no bar — absence communicates the mute state without any explicit "MUTE" label.

**Variations added:**
- `drawDash_Hapax4` → Dashboard: 8 rows with wide phase bars; focused row inverted (dark cutout bar); muted rows bar-free; type as single char
- `drawPerf_Hapax4` → Perform: pat# + wide phase bar per track; same focused/muted/active logic; footer LTCH/SYNC/SNAP/FILL

**References used:**
- https://squarp.net/hapax/manual/modepattern/ — left screen progress bars for all 16 tracks, content type icons
- https://squarp.net/hapax/manual/modestep/ — viewport, piano roll, dual information zones
- Search: "Squarp Hapax sequencer screen UI layout OLED display interface design"

---

## Batch 2 summary — 2026-05-21

**Type:** Explore
**Tastemakers covered:** Hapax (4)
**Total new variations:** 2 (Dashboard + Perform)

**Standouts (agent opinion):**
- `drawDash_Hapax4` — the phase bar is semantically distinct from everything in Batch 1. It shows the sequencer's live temporal state rather than its compositional content. The mute=absent convention (no bar drawn for muted tracks) cleanly combines Norns' off=absent principle with semantic meaning.
- `drawPerf_Hapax4` — same phase principle applied to Perform. Compact pat# on the left + the wide bar makes the page read as a "timing dashboard" rather than a "pattern picker."

**Concerns:**
- `Color.None` (0) inside an inverted row (the dark cutout bar): this relies on Canvas writing zero-value pixels onto the bright fill. If Canvas's Set mode skips zero draws, the cutout won't appear. May need to verify in the sandbox.
- The phase bars all show the same playback position (PLAY_CURSOR=4, fixture is global). In real usage, each track would have its own playhead position. The design intent is clear but the fixture makes all bars identical.

---

## Survivors (user-approved variations)

*(populated after first sandbox review session)*

---

## Dropped

*(none yet)*
