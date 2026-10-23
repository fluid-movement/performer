# Track Type Inventory

Per-type parameter tables for the redesigned Track Edit pages. Use this when designing each type's visual layout and F-key tab schema.

**Source:** `05-current-ui-functions.md`, §4–9.  
**Cuts applied:** Removes BypassScale, RetriggerProbability, NoteVariationRange/Probability per `00-ia.md`. StageRepeats/StageRepeatsMode restored (Metropolix-style beat repeats from mebitek fork).

---

## How to read this doc

Each section lists:
- **Per-step layers** — what can be stored/edited per step (these drive the F-key layer tabs)
- **Sequence-level parameters** — track-wide values for quick-edit overlay and Track Config
- **Recommended F-key tabs** — proposed new tab schema (4 tabs + F4 for a page action)
- **Design notes** — constraints and differences that affect the visual layout

---

## 1. Note Track

**What it is:** Stepped CV+gate sequencer. Most feature-rich type. Gates trigger notes; note values are scale degrees.

### Per-step layers (after cuts)

| Layer | Range | Notes |
|---|---|---|
| Gate | bool | Step fires a note |
| GateProbability | 0–15 | 15 = always fires |
| GateOffset | –8 to +7 | Sub-step timing offset |
| Retrigger | 0–7 | Retriggers within the step |
| Length | 0–15 | Gate length as fraction of step duration |
| Note | –64 to +63 | Scale degree index |
| Slide | bool | Legato / portamento into next step |
| Condition | 0–127 | Conditional trigger (fill conditions, pattern conditions) |
| StageRepeats | 1–8 | Times this step repeats before advancing (Metropolix-style) |
| StageRepeatsMode | enum | How repeats are spaced: Free / Triplets / Dotted / Bisect / Last / FirstLast |

**Cut from original:** BypassScale, RetriggerProbability, NoteVariationRange, NoteVariationProbability, LengthVariationRange, LengthVariationProbability.
**Note on variation params:** LengthVariation and NoteVariation are superseded by a planned track-level "humanize" setting that applies subtle random variation globally — no need to expose them per-step.

### Sequence-level parameters (quick-edit + Track Config)

| Parameter | Location |
|---|---|
| FirstStep / LastStep | Quick-edit (Page+Step8/9) |
| RunMode | Quick-edit (Page+Step10) |
| Divisor | Quick-edit (Page+Step11) |
| ResetMeasure | Quick-edit (Page+Step12) |
| Scale | Quick-edit (Page+Step13) |
| RootNote | Quick-edit (Page+Step14) |
| Octave, Transpose, Rotate | Track Config |
| Gate/Length/Note probability biases | Track Config |
| Fill mode, Play mode, Slide time | Track Config |

### Recommended F-key tabs

| Tab | Label | Layers cycled |
|---|---|---|
| F0 | GATE | Gate → GateProbability → GateOffset → Retrigger → Length |
| F1 | NOTE | Note → Slide |
| F2 | COND | Condition |
| F3 | REPT | StageRepeats → StageRepeatsMode |
| F4 | — | free |

**Rationale for changes from current firmware:**
- Length folded into GATE family — all gate/time parameters in one tab
- LEN tab eliminated (LengthVariation cut; plain Length doesn't need its own tab)
- LengthVariationRange/Prob and NoteVariationRange/Prob replaced by planned track-level humanize
- REPT tab for StageRepeats/StageRepeatsMode — no paging needed, all tabs first-class
- F4 free for a future action (pattern copy, context menu, etc.)

### Design notes

- The gate grid is the primary visual. Note values visible as height bars or text per step (TBD in design session).
- Step selection + encoder edit is the core interaction. Most time is spent here.
- Slide is step-level bool — could show as a glyph on the step cell rather than as a separate layer.
- 64-step sequences require Left/Right section navigation — this is a significant interaction to surface.

---

## 2. Curve Track

**What it is:** CV automation / LFO track. No gate output. Each step defines a CV curve shape between a min and max value. Purely continuous.

### Per-step layers

| Layer | Description |
|---|---|
| Shape | Curve shape: Linear, Exponential, Logarithmic, Sine, Square, Stairs, etc. |
| Min | Minimum CV output for this step |
| Max | Maximum CV output for this step |
| ShapeVariation | Alternate shape applied probabilistically |
| ShapeVariationProbability | Probability the alternate shape fires |

No cuts apply (NoteVariationRange/RetriggerProbability cuts don't affect Curve type).

### Sequence-level parameters

| Parameter | Location |
|---|---|
| FirstStep / LastStep | Quick-edit (Page+Step8/9) |
| RunMode | Quick-edit (Page+Step10) |
| Divisor | Quick-edit (Page+Step11) |
| ResetMeasure | Quick-edit (Page+Step12) |
| Range (output voltage range) | Quick-edit (Page+Step13) |
| Offset, Rotate | Track Config |
| Shape probability bias, Curve min/max | Track Config |
| Mute mode (LastValue / Zero / Min / Max) | Track Config |

### Recommended F-key tabs

| Tab | Label | Layers cycled |
|---|---|---|
| F0 | SHPE | Shape → ShapeVariation → ShapeVariationProbability |
| F1 | RNGE | Min → Max |
| F2 | — | (unused — may show curve summary or global CV range) |
| F3 | — | (unused) |
| F4 | (action — TBD) | |

Only 5 layers, 2 natural groupings. The extra F-keys can surface step-count or range config or remain empty in the initial design.

### Design notes

- **No gates.** The bar-height visualization IS the primary display — each step cell shows a bar from Min to Max with the curve shape drawn inside.
- No selection + note-editing flow. Encoder with step selected adjusts Min or Max depending on active layer.
- F key held + encoder = direct edit without switching layer (firmware behavior worth preserving).
- The curve shape glyph is the most distinctive visual element of this page. Give it space.
- Min/Max as a pair define each bar's range — consider showing both together on the step cell (bottom anchor = Min, top = Max, shape between).

---

## 3. Stochastic Track

**What it is:** Probability-weighted sequencer. Steps are templates that fire with configurable probability. Not deterministic — plays differently each time, reproducible via seed. Generative sibling of Note track.

### Per-step layers (after cuts)

| Layer | Range | Notes |
|---|---|---|
| Gate | bool | This step is available to fire |
| GateProbability | 0–15 | Likelihood this step fires when its turn comes |
| Length | 0–15 | Gate length |
| LengthVariationRange | –8 to +7 | |
| LengthVariationProbability | 0–15 | |
| Note | scale degree | Base note for this step |
| Condition | 0–127 | Conditional trigger |
| RestProbability2 | 0–8 | Extra rest probability at 2× weight |
| RestProbability4 | 0–8 | Extra rest probability at 4× weight |

**Cut from original:** RetriggerProbability, NoteVariationRange, NoteVariationProbability, Retrigger (no retrigger on stochastic), StageRepeats.

### Sequence-level parameters

| Parameter | Location |
|---|---|
| Seed | Quick-edit or dedicated tab (sets RNG seed for reproducibility) |
| RestProbability (global rest %) | Sequence-level; important live parameter |
| FirstStep / LastStep | Quick-edit |
| RunMode, Divisor, ResetMeasure | Quick-edit |
| Scale, RootNote | Quick-edit |
| Octave, Transpose, Rotate | Track Config |
| Fill mode, biases | Track Config |

### Recommended F-key tabs

| Tab | Label | Layers cycled |
|---|---|---|
| F0 | GATE | Gate → GateProbability |
| F1 | LEN | Length → LengthVariationRange → LengthVariationProbability |
| F2 | NOTE | Note |
| F3 | PROB | RestProbability2 → RestProbability4 → Condition |
| F4 | (action — TBD) | |

**Design note on RestProbability:** The global RestProbability (sequence-level) is a live performance knob — "how random/sparse is this track right now." It should be accessible via encoder+hold, not buried in Quick-Edit. Consider surfacing it as the idle-rotate value for this page.

### Design notes

- Conceptually similar to Note track but steps are seeds, not guarantees. The visual should communicate probability — perhaps gate cells drawn with varying weight or opacity... but wait, only 3 brightness levels. Use cell fill % (border vs. filled) to hint probability, not grey-out.
- RestProbability is a key live handle. Figure out its primary access in the design session.
- Seed display somewhere useful — shows reproducibility state.

---

## 4. Arp Track

**What it is:** Arpeggiator. Steps define a note pool and gate rhythm; the arp engine generates arpeggiated CV+gate from that pool. Has both step-level parameters (what notes / gates) and arp-engine parameters (how to arpeggiate).

### Per-step layers (after cuts)

Same as Note track (after cuts):

| Layer | Range |
|---|---|
| Gate | bool |
| GateProbability | 0–15 |
| GateOffset | –8 to +7 |
| Retrigger | 0–7 |
| Length | 0–15 |
| LengthVariationRange | –8 to +7 |
| LengthVariationProbability | 0–15 |
| Note | scale degree (fed into arp engine) |
| Slide | bool |
| Condition | 0–127 |

### Arp engine parameters (sequence-level, important for Track Edit)

| Parameter | Options |
|---|---|
| Arp mode | Up / Down / UpDown / DownUp / UpAndDown / DownAndUp / Converge / Diverge / Random |
| Hold | On / Off — sustain active notes |
| Octaves | 1–N octaves to span |
| GateLength | Gate length per arp note |
| Divisor | Arp rate |

These are sequence-level, not per-step — but they're central to how the arp sounds and are frequently adjusted. **They belong on a dedicated ARP tab in Track Edit**, not buried in Track Config.

### Recommended F-key tabs

| Tab | Label | Content |
|---|---|---|
| F0 | GATE | Gate → GateProbability → GateOffset → Retrigger |
| F1 | LEN | Length → LengthVariationRange → LengthVariationProbability |
| F2 | NOTE | Note → Slide |
| F3 | ARP | Arp mode / Hold / Octaves / GateLength — shown as a compact list in the grid area |
| F4 | (action — TBD) | |

**Condition** is less critical on Arp (it layers well but isn't as fundamental). Consider folding it into a context menu or GATE family.

### Design notes

- **The ARP tab is unique to this track type.** When F3 (ARP) is active, the 16-step grid could minimize and the arp parameters appear as a list/quick-edit area. Or the grid stays and arp params appear as an overlay column. Decide in design session.
- Arp mode is the primary "what kind of arp" selector. Should be quickly accessible — consider exposing as idle-rotate value or hold-track+rotate encoder.
- This track type has the most non-step content to show.

---

## 5. Quantizer Track

**What it is:** External CV quantizer. Reads an incoming CV signal, quantizes it to the active scale, and outputs it. Steps define a **gate rhythm** — when to sample and hold a new note. The pitch itself comes from outside.

### Per-step layers

| Layer | Description |
|---|---|
| Gate | Whether this step triggers a new sample (only meaningful layer) |

Note values are ignored — quantization handles pitch externally.

### Sequence-level parameters

| Parameter | Location |
|---|---|
| Input source | SRCE tab (CvIn1–4 or Track1–8) |
| Trigger mode | TRIG tab (Free / Internal / External) |
| Trigger track | TRIG tab sub-selector (for Internal trigger mode) |
| FirstStep, LastStep, RunMode, Divisor | Quick-edit |
| Octave, Transpose | TUNE tab (performance-oriented, routable) |
| PatternFollow | Track Config |

Note: Scale and RootNote are **global project parameters** — the Quantizer track has no per-track scale.

### Recommended F-key tabs

| Tab | Label | Content |
|---|---|---|
| F0 | GATE | Gate (only per-step layer) |
| F1 | SRCE | Input source: CvIn1–4 (top row) + Track1–8 (bottom row) — hold Step 0 + encoder |
| F2 | TRIG | Trigger mode chips (FREE/INT/EXT) + trigger track sub-selector when INT — hold Step 0/1 + encoder |
| F3 | TUNE | Octave + Transpose (2-column bipolar bar + value) — hold Step 0/1 + encoder |
| F4 | — | free |

**Rationale:** SRCE and TRIG are set-and-forget routing configuration. TUNE (Octave/Transpose) surfaces the most performance-oriented parameters since they're commonly adjusted live and are both routable via CV.

### Design notes

- **Only one step-layer.** Conceptually the simplest edit page — gate pattern plus three config tabs.
- The page should feel calm and spacious. Resist filling the extra space.
- SRCE two-row layout communicates the distinction between hardware CV inputs and internal track sources at a glance.
- TRIG shows the trigger track sub-selector only when INT mode is active — avoids showing irrelevant options.
- TUNE tab follows the Arp V2 ARP-tab column pattern: dim label at top, bipolar bar in middle, numeric value below.

---

## 6. MidiCv Track

**What it is:** MIDI-to-CV converter. No step sequencer — event-driven by incoming MIDI. Supports polyphonic voices, velocity, aftertouch, mod wheel, arpeggiator.

### No step grid.

MidiCv has no sequence or step parameters. The "Track Edit" page for MidiCv is instead a **real-time MIDI activity monitor** — shows what's incoming and how it's being converted.

### Real-time display content

| Element | Description |
|---|---|
| Active voices | Up to 8 voice slots: show note name + velocity bar (or off) |
| Incoming pitch | Last received note name |
| Pitch bend indicator | ±bend as a bar |
| Mod wheel | CC01 value as a bar |
| Port / channel | Current MIDI input config |

### Configuration (F-key tabs)

Since there's no editing loop, F-keys surface the most-changed config values:

| Tab | Label | Content |
|---|---|---|
| F0 | PORT | MIDI source port + channel |
| F1 | VOIC | Voices + voice config + note priority |
| F2 | ARP | Arp mode / hold / octaves / gate length / divisor (same as Arp track) |
| F3 | TUNE | Transpose, pitch bend range, mod range |
| F4 | (action — TBD) | |

### Design notes

- **No grid.** The voice display replaces the 16-step grid. Each voice is a row (up to 8 rows × ~7px each = 56px — fits in the content area).
- Encoder + any-held-button for config. Idle encoder: scrolls through voice rows or tabs.
- The activity monitor is live — needs animation (voice slots appear/disappear as MIDI notes arrive). This is purposeful animation.
- This track type uses the full content area for status rather than step editing.

---

## Summary table

| Track type | Step layers | F-key tabs | Unique design challenge |
|---|---|---|---|
| Note | 10 | GATE / NOTE / COND / REPT / — | Rich step editing, 64-step navigation, stage repeats |
| Curve | 5 | SHPE / RNGE / — / — | Bar-height CV visualization, curve shape glyph |
| Stochastic | 9 | GATE / LEN / NOTE / PROB | Communicating probability without grey levels |
| Arp | 10 + 5 arp params | GATE / LEN / NOTE / ARP | Arp tab shows non-step config in grid area |
| Quantizer | 1 | GATE / SRCE / TRIG / SCLE | Almost no step content; mostly config |
| MidiCv | 0 (no grid) | PORT / VOIC / ARP / TUNE | Live monitor replaces step grid |
