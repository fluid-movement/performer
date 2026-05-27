# UI Redesign — Sandbox Status & Handoff

This document is the primary handoff for the UI redesign work. Point new sessions here.
Related: `00-ia.md` (information architecture decisions).

---

## What we are building

A **Svelte 5 web sandbox** at `agent-docs/ui-redesign/sandbox/` that emulates the firmware's
256×64 mono OLED canvas API in the browser. All UI design iteration happens here before
any C++ code is written.

**Why:** The firmware's Python sim harness requires a full C++ build cycle. The web sandbox
lets us iterate on visual design in seconds. Every component in the sandbox uses only
operations that map 1:1 to the real `Canvas` C++ API, so mockups translate directly.

---

## Running the sandbox

The user runs the dev server themselves (`vp run dev` or equivalent). Do not start it.
The app lives at `http://localhost:5173` (default Vite port) once running.

**Stack:** SvelteKit 2 / Svelte 5 / Vite / TypeScript — scaffolded at
`agent-docs/ui-redesign/sandbox/`.

---

## Sandbox structure

```
sandbox/src/
├── lib/
│   ├── canvas/
│   │   ├── Canvas.ts          # TypeScript mirror of src/core/gfx/Canvas.h
│   │   ├── FrameBuffer.ts     # 256×64 Uint8Array, Set/Add/Sub blend ops
│   │   ├── Display.svelte     # Renders framebuffer → <canvas> at N× scale
│   │   └── fonts/
│   │       ├── types.ts       # BitmapFont / BitmapFontGlyph interfaces
│   │       ├── tiny5x5.ts     # Ported from src/core/gfx/fonts/tiny5x5.h
│   │       └── ati8x8.ts      # Ported from src/core/gfx/fonts/ati8x8.h
│   ├── components/            # (empty — next phase)
│   └── pages/                 # (empty — next phase)
└── routes/
    └── +page.svelte           # Single page: catalog (left) + click dummy (right)
```

### Canvas.ts API surface

Mirrors `src/core/gfx/Canvas.h` exactly. Key methods:

| Method | Notes |
|---|---|
| `setColor(Color)` / `setColorValue(n)` | Color enum: None=0, Low=3, MediumLow=5, Medium=7, MediumBright=10, Bright=15 |
| `setBrightness(float)` | Modulates all subsequent colors |
| `setFont(Font)` | Font.Tiny (tiny5x5, 6px) or Font.Normal (ati8x8, 10px) |
| `setBlendMode(BlendMode)` | Set / Add / Sub |
| `point(x, y)` | |
| `hline(x, y, w)` / `vline(x, y, h)` | |
| `line(x0, y0, x1, y1)` | Xiaolin Wu antialiased |
| `rect(x, y, w, h)` | Outline |
| `fillRect(x, y, w, h)` | Filled |
| `drawBitmap(x, y, w, h, bitmap, offset)` | 1bpp, LSB-first |
| `drawBitmap4bit(...)` | 4bpp nibble-per-pixel |
| `drawText(x, y, str)` | x/y is baseline |
| `drawTextAligned(x, y, w, h, hAlign, vAlign, str)` | Aligned within box |
| `textWidth(str)` / `textHeight()` | Measurement |

### Display.svelte

```svelte
<Display draw={(c: Canvas) => { /* draw here */ }} scale={3} />
```

`draw` is called every time it changes. `scale` multiplies the 256×64 framebuffer for
screen readability (use 2 for catalog items, 3 for the main click dummy).
Renders via `putImageData` with `image-rendering: pixelated`. Brightness 0–15 → grayscale
via `value * 17`.

---

## Current page in +page.svelte

**Layout:** Two-column grid.
- **Left column:** Static catalog — all pages rendered at scale=2, stacked vertically.
- **Right column:** Live click dummy — one Display at scale=3, page-nav buttons to switch
  between pages, hardware button row (Step 1–16, Track 1–8, F0–F4, global keys).

**Pages currently sketched (rough pass, not final):**

| Page | State | Notes |
|---|---|---|
| Dashboard | Rough | Status strip + 8 track rows + step gate pattern |
| Perform | Rough | 8 track columns, pattern grid 4×4, mute state |
| Track Edit (Note) | Rough | 16-step grid, gate layer, step cursor |
| Track Edit (Curve) | Rough | Same grid, bar-height = curve value |
| Track Config | Rough | List of per-track settings, encoder-selected |
| Song | Rough | Slot rows × 8 track pattern assignments + repeat count |
| Settings | Rough | Two-panel: section list left + detail right |
| Tempo modal | Rough | Large BPM value (Normal font), tap/nudge hints |
| Quick Edit overlay | Rough | Overlay on Track Edit: FirstStep/LastStep/Mode/Div/Reset |

All draw functions live inline in `+page.svelte` for now. When a page stabilises it should
move to `src/lib/pages/<PageName>.ts` as a plain function `(c: Canvas) => void`.

---

## Design decisions locked (2026-05-21)

Full record in `00-ia.md`. Key changes from original plan:

| Decision | Locked value |
|---|---|
| F-key footer | **Always present** on every page. All 5 F-keys free per page. Primary use: tabs within a page. |
| Encoder press | **Removed.** Pattern = "hold button + rotate encoder." Idle rotate = page-dependent cursor or no-op. |
| Track Edit | **5 per-type pages** (not one adaptive). Share footer conventions. Header optional. |
| Brightness palette | **3 levels only** — Bright (15) / Low (5) / off. See `01-design-language.md`. |
| Selection convention | **Inverted fill** everywhere — `fillRect(Bright)` + content in `Color.None`. |
| BPM | Tempo modal only. Not on persistent display. |
| Dashboard | Home page, read-only for now. |
| Song | **Deferred** until other pages settle. |
| LEDs | **First-class design surface.** Each variation must specify LED state. LedPanel in sandbox. |
| Shift modifier | Keep current load. |
| Animation | Purposeful only — play cursor, beat pulse. No transitions, no idle motion. |
| Tastemakers | M8 and Squarp Hapax promoted to next priority. |

Full design language rules in `01-design-language.md`. Track-type parameter inventory in `06-track-type-inventory.md`.

---

## Sandbox structure (updated)

```
sandbox/src/
├── lib/
│   ├── canvas/
│   │   ├── Canvas.ts
│   │   ├── FrameBuffer.ts
│   │   ├── Display.svelte
│   │   ├── LedPanel.svelte    ← NEW: renders 32 LED states alongside buttons
│   │   ├── LedState.ts        ← NEW: LedState type + defaultLeds()
│   │   └── fonts/
│   └── components/            (empty — populated as pages stabilise)
└── routes/
    └── +page.svelte           Catalog + click dummy + hardware buttons + LED panel
```

**Variation signature:**
```typescript
interface Variation {
  label: string;
  draw: (c: Canvas) => void;
  leds?: () => LedState;   // optional; defaults to all-off
}
```

**Trunk variations** (index 0 in their page, labeled `★ TRUNK`):
- Dashboard: `★ TRUNK — 3-level, inverted focus row`
- Perform: `★ TRUNK — 8 rows, 3-level, footer`
- Track Edit: trunks per-type, deferred to G7 design sessions

---

## Open questions (resolved)

| Question | Resolution |
|---|---|
| Dashboard interactions — read-only or mute-toggle? | Read-only for now; revisit after Perform and Track Edit settle. |
| Perform layout — 4×4 grid too dense? | Row-based layout (8 rows × 1 track) adopted in trunk. |
| Track Config — list or F-key tabs? | F-key tabs. Sections visible in footer; encoder scrolls within section. |
| Settings — two-panel OK? | Kept. One page with sections (encoder scrolls, F-keys jump). |
| Song layout density? | Deferred. Design after other pages settle. |
| Quick Edit discoverability? | Keep as hidden combo + Page-hold legend. Not promoted to tab. |

---

## Next steps

1. ✅ G1 — Patch `00-ia.md` (done)
2. ✅ G2 — Write `01-design-language.md` (done)
3. ✅ G3 — Track-type parameter inventory `06-track-type-inventory.md` (done)
4. ✅ G4 — LED mocking in sandbox (`LedState.ts`, `LedPanel.svelte`) (done)
5. ✅ G5 — Trunk picks for Dashboard and Perform (done)
6. ✅ G6 — Update docs, revise iteration prompt (done)
7. ✅ G8 — Iteration process: ledger (`07-iteration-feedback.md`), `prompt-iteration.md`, cycle with review + capture (done)
8. **G7** — Per-track-type design sessions: Note → Curve → Stochastic → Arp → Quantizer → MidiCv
9. **Iterate** — Run tastemaker iterations with consolidate cadence against the trunk; review after each

---

## Key source files to know

| What | Where |
|---|---|
| Canvas C++ API | `src/core/gfx/Canvas.{h,cpp}` |
| Blit operations (Set/Add/Sub) | `src/core/gfx/Blit.h` |
| Font headers | `src/core/gfx/fonts/tiny5x5.h`, `ati8x8.h` |
| Current pages (78 files) | `src/apps/sequencer/ui/pages/` |
| Current painters | `src/apps/sequencer/ui/painters/` |
| Python sim harness | `src/apps/sequencer/tests/agent.py` |
| IA decisions | `agent-docs/ui-redesign/00-ia.md` |
| Design language rubric | `agent-docs/ui-redesign/01-design-language.md` |
| Iteration feedback ledger | `agent-docs/ui-redesign/07-iteration-feedback.md` |
| Iteration agent prompt | `agent-docs/ui-redesign/prompt-iteration.md` |
| This document | `agent-docs/ui-redesign/02-sandbox-status.md` |
