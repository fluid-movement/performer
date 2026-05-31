# Design Ideas

A living document. Not rules — those live in `01-design-language.md`. This captures emerging principles and patterns as they're discovered through design sessions. Append as ideas crystallise; revisit to promote strong ideas into the rules doc.

---

## Principle: Information in context

Show the gestalt by default. Surface exact values only when the user is actively editing that value — holding a step button and rotating the encoder. At all other times, the display communicates shape and structure, not numbers.

**Why:** The idle view is navigational. The edit view is precise. Mixing them produces clutter that serves neither goal. A musician scanning their sequence doesn't need to read "62 bpm" on every step — they need to see the contour. When they reach for a specific step and turn the encoder, *then* they need the number.

**Applies to:** every layer on every page. Pitch, gate length, probability, condition, stage repeats — all of these can be communicated gesturally when browsing and precisely when editing.

**Implementation pattern:**
- Idle / browsing: visual encoding only (line height, line width, fill density, etc.)
- Step held + encoder rotating: show exact value as a prominent overlay (large text, center-weighted)
- Releasing the step: overlay returns to gestural view

---

## Principle: The line sequencer

A horizontal line per active step encodes three dimensions simultaneously without any labels:

| Dimension | Encoding |
|---|---|
| Gate on/off | Line present / nothing drawn |
| Pitch | Vertical position (high note → near top, low note → near bottom) |
| Gate length | Line width (short gate → short line, full gate → nearly fills slot width) |

Inactive steps draw nothing — not a dim cell, not a border, nothing. The display at rest is silence punctuated by marks.

**Why:** This mirrors how a musician thinks about a sequence. Notes are events in time, not slots waiting to be filled. The visual weight of the display should match the sonic weight of the sequence — a sparse pattern is visually sparse; a dense pattern is visually dense.

**Cursor treatment:** A small vertical tick above the step slot indicates "you are here" regardless of gate state. It's the only structural element drawn for an inactive cursor step.

**Pitch range:** Map relative to the sequence's actual note range, not to an absolute MIDI range. If all notes are in a 4-semitone cluster, they should spread across most of the vertical space — the visual shows relative pitch contour, not absolute position.

### Known limitations and refinements

**Pitch resolution is the core tension.** The content area is 46px tall. A typical 2-octave sequence is 24 semitones — that's under 2px per semitone. C4 and D4 are visually indistinguishable at a glance. The view communicates macro contour (ascending, descending, wide leaps) well but not chromatic precision. Adaptive range mapping helps — a 5-note sequence gets ~9px per note — but introduces a new problem: the visual distance between lines now depends on what other notes are in the sequence, making the encoding *relative* rather than *absolute*. Moving to a different pattern, the same vertical height means a different pitch.

**Gate-length-as-width is subtle at small differences.** A 3px vs 13px line is clearly different. A 7px vs 9px line is not. This encoding works for conveying "staccato vs legato" at a macro level, not for programming gate lengths precisely.

**Beat structure disappears.** Cell-based views communicate 4/4 groupings silently through their spacing and beat markers. Floating lines have no inherent rhythm structure — a 16-step sequence looks like a 12-step one with rests unless you add structural markers, which fights the absence principle.

**Where it excels:** Melodic sequences with clear wide-interval motion (arpeggios, stepwise motion across an octave). The ascending staircase, the octave leap, the call-and-response shape — all read immediately. It is the best view for melody, not the best view for rhythm.

### Proposed refinements

1. **Reserve the line view for the NOTE tab.** The default GATE tab uses a conventional gate/bar view (reliable for rhythm programming). Switching to NOTE reveals the line view — the tab switch is itself the reveal. When you're thinking about rhythm, see cells. When you're thinking about melody, see lines.

2. **Add a scale degree grid on the NOTE tab.** Ultra-faint horizontal rules at each scale degree position. With 7 degrees in a major scale over 2 octaves (~6px per degree) the positions become unambiguous. A line sitting on a grid line is diatonic; between lines is chromatic. This resolves the pitch-precision problem without adding labels.

3. **Vary line brightness by beat position.** Steps 0 and 8 (beats 1 and 3) at full Bright; steps 4 and 12 (beats 2 and 4) at Low. Beat structure re-enters through brightness — no structural markers needed, no conflict with the absence principle.

4. **Play cursor as a vertical scanline.** A 1px full-height vertical line sweeping left to right as the sequence plays. This is immediately legible, more so than a small tick. The display feels alive during playback.

---

## Principle: Absence as information

Borrowed from Monome/Norns design sensibility: an unlit pixel communicates "off" with zero visual noise. Do not draw dim placeholders, ghost borders, or greyed-out cells for inactive steps.

**Why:** Dim elements still occupy visual attention. The eye must evaluate each one and dismiss it. True absence requires no evaluation — the eye finds what's present and ignores the void. This makes the signal-to-noise ratio perfect: everything drawn is information.

**Corollary:** When you do draw something, it always means something. The discipline of "only draw what's active" makes every mark unambiguous.

**Contrast with:** The cell-based step grid (Options 1–3 in the sandbox) which draws borders/outlines for every step. Those borders communicate "here is a slot" — useful when teaching new users that 16 steps exist. The line sequencer view trusts the user to know.

---

## Principle: Tab stability — tabs change the editing target, not the display

**The problem with tab-switching-as-display-change:** When switching F-key tabs completely reorganises the display (e.g., GATE shows gate cells, NOTE shows note names, COND shows condition values), the user experiences a context switch. Their eyes must re-parse the entire screen. Their mental model of the sequence — built up over the last few seconds of looking — is discarded. This is cognitive overhead with every tab press.

**The old firmware does this.** Pressing F1 (GATE) and F2 (NOTE) show completely different displays. It works, but it's a cost.

**The principle:** The base display never changes when switching tabs. What changes is:
1. Which footer tab is highlighted (always)
2. What the encoder edits when a step is held (always)
3. Optionally: a small *additive* overlay that aids editing of the active layer (sometimes)

The third point is the nuance: tabs *can* change the UI a little — and should, when it helps. Switching to NOTE can reveal the scale degree grid (since you're editing pitch, grid lines help). Switching to REPT can show tiny repeat counts above steps (since you're now editing repeats, that context is useful). These are additions on top of the stable base view, not replacements of it.

**Why this is better:** The user builds one mental model of the display and keeps it. Switching tabs feels like changing gear, not changing rooms. The sequence shape is always visible. The active tab just determines what happens when you reach for a step and turn.

**The synthesis:** Combine this with the line sequencer and layered reveal —
- Base layer (always): lines encoding gate/pitch/length
- Tab overlay (additive): scale grid on NOTE, repeat counts on REPT, faint probability shading on GATE
- Interaction layer (step held): exact value for the current tab's parameter

This gives a UI that is simultaneously more information-dense than the current firmware (three dimensions always visible), less cognitively demanding (stable base view, no context switches), and more precise when needed (exact values on demand).

---

## Pattern: Layered reveal

A single view can serve multiple information densities by revealing detail in layers:

1. **Base layer** (always visible): gestural encoding — lines, shapes, positions
2. **Tab overlay** (varies by active tab): small additive context that aids editing the current parameter — scale grid, repeat counts, probability shading. Never replaces the base view.
3. **Interaction layer** (step held): exact value for the selected parameter as a prominent overlay

Switching tabs shifts the overlay and the editing target. The base view does not change. Users build spatial memory for the sequence shape without the display reorganising under them.

**Contrast with current firmware:** tab switches are display replacements — each tab is a completely different page. The new model treats tabs as editing modes layered onto a persistent view.

---

## Principle: Negative space is not wasted space

The old firmware's Note Track Edit page uses small 12×12px gate squares inside a 16×50px column. Most of each column is empty. That reads clearly and feels calm.

Filling more of the available space does not produce more information — it produces more noise. A 44px-tall bright cell for a gate-on step is not "more informative" than a 12px one. It's heavier. The eye works harder because there's more lit area to process, yet the information content is identical: gate on.

**The mistake to avoid:** using visual weight as a proxy for importance. A step that fires on beat 1 is not more important than one on beat 3. The grid is not a progress bar. Filling cells proportionally to their "significance" has no semantic meaning and makes the display harder to read at a glance.

**The discipline:** mark only what needs to be marked. Use size and weight to encode *variation* in a value, not to indicate that a value is *present*. A gate-on indicator should be small and consistent. Length, probability, pitch — these vary, so their visual representations should vary. Gate presence does not vary (it's boolean), so its representation should be minimal and uniform.

**What the old firmware gets right:** the 12×12 gate square leaves 38px of vertical breathing room per step column. That space makes the layout scannable. The marks are small enough that the eye can group them into rhythm patterns without needing to process each individual cell's fill state.

**Implication for new designs:** a step grid made of small uniform indicators (not tall filled bars) with layer data shown as separate lightweight marks in the surrounding space will be more readable than a grid of tall filled cells, even if the tall cells carry more data per cell. Information per pixel is not the goal. Legibility is.
