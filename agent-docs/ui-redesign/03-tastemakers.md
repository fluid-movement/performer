# Tastemakers

Sources for the evolutionary design loop. Each entry has: what to look for, where to find references, and which principles to extract per iteration.

The loop works through these in order, adding new variations to the sandbox. Progress is tracked in `04-design-log.md`.

---

## 1. Elektron (Digitakt / Syntakt / Octatrack)
**What to look for:** Information density on small screens, the grid/step language, parameter page layouts, contrast between active/inactive states, how they handle multi-layer editing without a touchscreen.  
**Search:** `Elektron Digitakt UI screen`, `Digitakt OLED interface`, `Elektron sequencer parameter page`  
**Principles to extract:** Selection highlight style, how they abbreviate labels, how the step grid relates to parameter editing.

## 2. Teenage Engineering (OP-1 / OP-Z)
**What to look for:** Bold typography on very small screens, color usage (even on mono screens, contrast creates hierarchy), playful iconography, how they communicate mode/state.  
**Search:** `OP-1 screen UI`, `OP-Z OLED display`, `Teenage Engineering interface design`  
**Principles to extract:** Typography scale ratios, how much whitespace they use, icon vs. text balance.

## 3. Monome (norns)
**What to look for:** Text-first minimalism, how they use brightness levels (not just on/off), script-based UI variety, calm information hierarchy.  
**Search:** `monome norns screen`, `norns script UI`, `norns OLED display examples`  
**Principles to extract:** Brightness usage as the primary design language, how they handle dense vs. sparse layouts.

## 4. Yamaha DX7 / vintage LCD synths
**What to look for:** Parameter display conventions from the 80s/90s, how they handled limited resolution gracefully, abbreviation systems that became standards (ENV, LFO, OP, etc.).  
**Search:** `Yamaha DX7 display`, `vintage synth LCD parameter screen`, `Korg M1 display UI`  
**Principles to extract:** Abbreviation standards, parameter grouping conventions, how limited resolution forced visual clarity.

---

## Future candidates (not yet in rotation)

- **Buchla / Serge** — patch-diagram thinking applied to screen UI
- **Squarp Pyramid** — another sequencer with good dense UI
- **Hapax (Squarp)** — modern Eurorack sequencer, OLED
- **Endorphin.es / Frap Tools** — minimal Eurorack parameter display
- **Roland MC-101/707** — clip-based sequencer UI on small screen
- **Ableton Push** — touchscreen sequencer, grid-heavy, good color language even on mono
- **Dirtywave M8** — handheld tracker, excellent OLED UI, very influential in this space
- **Polyend Tracker** — another handheld tracker, different layout approach

---

## What "extract principles" means in practice

For each tastemaker, the loop should produce a short note like:

> **Elektron turn 1:** Consistent use of inverted (filled) row for selected item. Labels always abbreviated to 4-6 chars. Step grid uses uniform cells — brightness/fill encodes gate, not size. No decorative elements.

These notes live in `04-design-log.md` and inform which draw functions get written.
