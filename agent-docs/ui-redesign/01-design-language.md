# Design Language — Decided

The rules every page and variation must follow. Evaluate any new draw function against this doc. If a rule blocks a good idea, update the rule here first — don't silently break it.

---

## Brightness

**3 levels only.** No mid-tones, no gradients.

| Level | Canvas value | Semantic |
|---|---|---|
| Bright | `Color.Bright` (15) | Focused / active / in-use. The thing the user is acting on right now. |
| Low | `Color.MediumLow` (5) | Context / structural / present but not the point. Labels, inactive steps, grid outlines. |
| Off | `Color.None` (0) | Absent. True black. Do not draw dim background fills for "inactive" — absence IS the information. |

**Never use** Color.Low (3), Color.Medium (7), or Color.MediumBright (10) in new designs. The three-level discipline is what keeps the screen calm.

Exception: inverted cells (Bright fill + off content) technically use 0 as a foreground value — that is allowed and is not a fourth brightness level.

---

## Typography

Two fonts. Pick the right one; do not mix within a logical unit.

| Font | Canvas constant | Line height | When to use |
|---|---|---|---|
| Tiny | `Font.Tiny` (tiny5x5) | 6px | Everything: labels, values, step data, footers. Default. |
| Normal | `Font.Normal` (ati8x8) | 10px | Single dominant value only — e.g., BPM in Tempo modal, large step value in TE2 style. Never for labels or multi-line content. |

**Label abbreviation rules:**
- 4 characters maximum for F-key tab labels and column headers. Abbreviate hard: GATE, NOTE, COND, SYNC, FILL, LATCH, SNAP, DIV, TRIG, STEP, RUN, RST, LEN, OCT, TRN.
- 6 characters maximum for row labels in list/config pages: SCALE, CLOCK, ROUTE, RESET, SLIDE.
- No trailing colons. No title-case. All-caps only.
- When in doubt, use the DX7/Elektron abbreviation if it exists. Don't invent new ones.

---

## Spacing

**Base unit: 4px.** All dimensions are multiples of 4px where possible.

| Element | Dimension | Notes |
|---|---|---|
| Footer height | 9px | 1px top border (Low) + 8px content (6px font, 1px padding top/bottom) |
| Optional header height | 8px | 6px font + 1px top + 1px bottom border (Low) |
| Step cell (outer) | 16×12px | 256px / 16 = 16px wide exactly. Height: adjust per page (min 8px). |
| Step cell (inner) | 14×10px | 1px border all sides |
| Gutter between logical zones | 2px | e.g., between track rows, between step grid and content area |
| Minimum touch target (for readability) | 8px | Nothing important should be smaller than this |

With footer only (no header): **55px content area** (64 − 9).  
With header + footer: **47px content area** (64 − 8 − 9).  
Full bleed (no chrome): **64px** — for pages that earn it.

---

## Step grid

The 16-step grid is the dominant visual element on all Track Edit pages and appears in compact form on Dashboard.

**Cell states:**

| State | Fill | Outline |
|---|---|---|
| Gate on, no cursor | `fillRect(Bright)` | none |
| Gate on, cursor | `fillRect(Bright)`, 1px inner border `Color.None` | inverted inner ring |
| Gate off, no cursor | none | 1px rect `Low` |
| Gate off, cursor | `fillRect(Bright)`, content `Color.None` | inverted (Bright fill, dark content) |

**Beat markers:** Steps 0, 4, 8, 12 have a 2px-tall top border instead of 1px. Same color (Low for off-steps). This signals the 4-beat grid without adding a new brightness level.

**Playback cursor:** The currently-playing step gets a `fillRect` at Bright regardless of gate state. If gate is also on, the fill just stays Bright (no difference needed — the LED on the button communicates the beat position).

---

## Selection / focus convention

**Inverted cell/row = active selection.** Everywhere.

- Fill the cell/row with `fillRect(Color.Bright)`.
- Draw all content (text, icons) inside with `setColor(Color.None)` (value 0).
- This produces dark glyphs on a bright background — maximum contrast, unambiguous.

This is the one convention that must be visually identical across all pages. A user learns "bright fill = my cursor is here" once, and it works everywhere.

---

## Footer

Every page has a footer. The footer is the bottom 9px of the display.

```
y=55: 1px horizontal rule at Low
y=56–63: content area (8px)
```

**F-key tab layout:** 5 equal cells across 256px = ~51px per cell (cells 0–3 = 51px, cell 4 = 52px).

| State | Appearance |
|---|---|
| Active tab | `fillRect(Bright)` for full cell, label in `Color.None` (inverted) |
| Inactive tab | Label only, in `Low`. No fill. |
| Disabled / unused tab | `—` label in `Low`. |

Label is always centered horizontally and vertically within the cell using `drawTextAligned`.

**What goes in the footer:**
- Primary: layer family tabs for the current page (F0–F3 = content, F4 = action or tab).
- Secondary: page-specific actions on pages without tabs.
- Never: top-level navigation destinations (those are Page+X shortcuts, not footer items).

---

## Header (optional)

When used, the header is the top 8px of the display.

```
y=0–6: content area (7px — fits Font.Tiny plus 1px top padding)
y=7: 1px horizontal rule at Low
```

Content: track number / track type glyph / page context on the left; record/play state glyph on the right. All in Low. Nothing Bright — the header is structural, never the focus.

**When to omit:** If the page's step grid or content area communicates context sufficiently. Omitting the header gives 8px back to content. Try without first.

---

## Encoder

- **The encoder never navigates on config pages.** Every turn changes a value. There is no edit
  mode. (Shipped 2026-08-29.)
  - **F1-F5 jump between sections; pressing the tab you are already on walks the cursor down that
    section and wraps.** One finger does both jobs -- it is already on the button. Left/Right
    remain as an alternative.
  - This applies wherever an F-key acts as a **tab** (a highlighted active state), including pages
    that implement their own footer rather than declaring model sections: Layout, System, User
    Scale. It does not apply where F-keys are **actions** (PREV/NEXT/INIT, CANCEL/OK).
  - Exception: single-column models (pickers, where the row *is* the value) move the cursor,
    because that *is* changing the value. `ListModel::columns()` is the discriminant.
- **Encoder press is never a mode toggle.** It may be a one-way descent into a sub-page (name
  entry, picker confirm) -- a mode you can see, because the screen changed.
- **On grid pages, value change is: hold the target button + rotate encoder.** The held button
  selects what changes. This suits pages with a spatial button-to-target mapping (steps). It is
  *not* used on config pages: hunting for the right button in the 3x8 field to reach a named
  setting is fiddlier than walking a list with Left/Right.
- **Idle rotate (no button held):**
  - Config pages: edits the selected row (see above).
  - Grid pages (Track Edit, Dashboard): no-op unless the current page defines a clear "most important single value" (e.g., selected step's note value on Note track). If ambiguous — no-op.

---

## Animation

**Only motion that carries information.** No transitions, no decorative movement.

Allowed:
- **Playback cursor** — the active step position moves forward each clock tick.
- **Beat pulse** — a brief (1-frame) brightness change on beat 1 to accent the downbeat. Subtle.
- **Record state** — record indicator blinks at the quarter-note rate when recording.
- **LED changes** — LEDs update in real time with sequencer state.

Not allowed:
- Page transitions (cuts only — no slides or fades).
- Value change animations (encoder turns update immediately).
- Idle/ambient animations (anything that moves with no user input or playback state).

---

## LEDs

The 32 bi-color LEDs are designed alongside the screen. Every variation must specify LED state, not just screen state.

**Color semantics (consistent across all pages):**

| LED color | Meaning |
|---|---|
| Green | Active / playing / on |
| Red | Muted / stopped / error / destructive pending |
| Amber (red + green) | Queued / requested / will change at next sync point |
| Off | Inactive / not present / nothing here |
| Blinking green | Recording |
| Blinking red | Waiting for external sync |

**Config pages** light the LEFT/RIGHT LEDs (`Key::Left`/`Key::Right`, matrix indices 28/29) green
when that button will move the cursor, off at the ends of the section. These buttons carry no
on-screen label, so their LEDs are the only affordance saying row navigation exists.

> **The 5 function keys have no LEDs.** The matrix is 32 entries (8 track + 16 step + 8 global);
> `Key::F0` is already index 32, so writing an F-key LED is out of bounds. F-key state must be
> communicated on screen (inverted footer tab), never by an LED.

**Per-button LED assignments (defaults, pages may override):**

| Button group | Default LED meaning |
|---|---|
| Track 0–7 | Green = active/selected track. Red = muted. Off = present but not focused. |
| Step 0–15 | Depends on page. On Track Edit gate layer: green = gate on. On Perform: current pattern green, queued pattern amber. |
| Play | Green (playing), blinking green (recording), off (stopped) |
| Pattern / Performer | Green when the corresponding page is open non-modally |
| Page | Off normally; lights during Page-hold to show navigation slots |

**LED design process:** For each page variation, write a companion `ledState_*` function or object that maps the 32 LEDs to their colors. This runs alongside the draw function in the sandbox.

---

## Rule promotion

Rules in this document may originate from the iteration feedback ledger (`07-iteration-feedback.md`). A "Liked" entry in the ledger promotes here when it has been reinforced in a second iteration without dissent. Once promoted, the ledger entry is deleted.

If you want to challenge or update a rule, change it here first — then note why in the ledger.

---

## Dos and don'ts

**Do:**
- Draw nothing for "off" states. Blank space is cheap and communicates absence.
- Use inverted fill consistently for the cursor/selection.
- Abbreviate labels to 4 chars. If it doesn't fit in 4 chars, abbreviate harder.
- Think about the LED state every time you think about the screen.

**Don't:**
- Use `Color.Low (3)`, `Color.Medium (7)`, or `Color.MediumBright (10)` — these are not in the palette.
- Use `Font.Normal` for labels or lists — it's for single dominant values only.
- Draw decorative separator lines beyond the footer and header rules.
- Animate anything that isn't playback state or user-triggered feedback.
- Design the screen without also designing the LEDs.
