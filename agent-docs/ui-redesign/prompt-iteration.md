# Design Iteration — Agent Instructions

You are running one iteration of the PER|FORMER UI design work.
Read this file fully before doing anything else.

---

## What you are doing

You study real hardware/software UIs ("tastemakers"), extract concrete design principles,
and write new draw function variations into the sandbox. One iteration = one focused unit
of work, then a hand-off to the user for review.

This is an **evolutionary** process: good ideas survive into subsequent iterations, bad ones
are dropped. Your job is to generate raw material, not to pick winners unilaterally.

---

## Entry point

This file is read via the `/iteration` slash command, which instructs the agent to read the state files first. If reading this file directly, read these first:

1. `07-iteration-feedback.md` — Avoid section = hard constraints
2. `01-design-language.md` — full rubric
3. `04-design-log.md` — current state

---

## The iteration cycle

```
1. Plan       — state in 1–2 lines what this iteration will study/produce
2. Work       — produce variations (Explore or Consolidate, see below)
3. Hand-off   — 2–4 sentence summary of what's new; agent's own opinion (standouts + concerns)
4. Review     — user discusses freely
5. Capture    — agent writes short-form entries to 07-iteration-feedback.md
6. Promote    — if a "Liked" entry has been reinforced across two iterations, promote to
                01-design-language.md and delete from the ledger
```

**Hard stop after step 3.** Do not continue to a second tastemaker or second pass without
a review. One focused unit, then hand-off.

---

## Iteration types

Each iteration is either **Explore** or **Consolidate**. Default cadence: 2 Explore, then
1 Consolidate, then repeat.

Check `04-design-log.md` to see the last turn type. If the last 2 turns were Explore,
the next must be Consolidate.

---

### Explore iteration

1. **Read state.** Open `04-design-log.md`. Note `next_tastemaker` and focus pages.
2. **Research.** Run 2–4 searches for the current tastemaker. Extract **2–3 concrete,
   actionable principles** (specific decisions, not adjectives).
3. **Write variations.** For each focus page, write one new draw function + one `leds*()`
   function in `+page.svelte`. Follow conventions below.
4. **Register.** Add each variation (with `leds`) to the matching `VARIATIONS[pageId]` array.
5. **Log.** Append a log entry to `04-design-log.md`.
6. **Hand off.** Tell the user what's new (2–4 sentences). Name your standout and your concern.

---

### Consolidate iteration

Goal: pick a winner from the latest variations and update the trunk.

1. **Read state.** Open `04-design-log.md`. Review the last 2–3 variations added for
   each focus page.
2. **Evaluate against `01-design-language.md` and `07-iteration-feedback.md`.** Score each
   variation: does it use only Bright/Low/off? Is selection inverted? Is the footer present
   and correct? Does the LED state make sense? Does it violate any "Avoid" entries?
3. **Pick a challenger.** For each page, identify the variation that best satisfies the
   rubric AND advances the design beyond the current trunk.
4. **Update the trunk.** If the challenger beats the current trunk, replace `drawDash_trunk`
   / `drawPerf_trunk` (or the relevant trunk function) with the challenger's logic and
   update the trunk's label. If no challenger beats the trunk, note why in the log.
5. **Prune.** Remove variations that clearly don't work (or mark them as dropped).
6. **Log.** Append a consolidate-turn entry to `04-design-log.md`.
7. **Hand off.** 2–4 sentence summary of what changed and what held its ground.

---

## After the review: ledger capture (step 5 of cycle)

This step runs **after the user has responded**, before the next iteration starts.

Write entries to `07-iteration-feedback.md`:
- One "Liked" entry per pattern the user affirmed.
- One "Avoid" entry per pattern the user rejected or flagged.

Format: `date — rule. Why: one-line reason.` Quote the user's reasoning; don't paraphrase.

If a "Liked" entry already exists and the user has now affirmed it a second time, promote it
to the appropriate section of `01-design-language.md` and delete the ledger entry.

---

## Code conventions

**File to edit:** `agent-docs/ui-redesign/sandbox/src/routes/+page.svelte`

**Naming:** `drawPageId_SourceN` / `ledsPageId_SourceN` — e.g. `drawDash_Hapax4`, `ledsDash_Hapax4`.  
**Variation label:** `'Hapax 4 — one-phrase description'`

**Where to add:** draw function just before `// ── routing`; variation entry in `VARIATIONS[pageId]`.

For Canvas API surface, helper functions (`hdr`, `stepBar`), and LED state shape, read:
- `agent-docs/ui-redesign/02-sandbox-status.md` → "Canvas.ts API surface" and "Sandbox structure"

Design language rules summary (full detail in `01-design-language.md`):
- Brightness: Bright (15) / Low (5) / off ONLY
- Selection = `fillRect(Bright)` + content in `Color.None` (inverted)
- Footer always present (9px at y=55). Active tab = inverted. Label max 4 chars.
- Font.Normal for single dominant values only. Font.Tiny everywhere else.
- No decorative separator lines beyond header/footer rules.
- Violations = the variation cannot become a trunk.

---

## Tastemakers

Current queue and progress live in `04-design-log.md` (top section).

Full tastemaker list with search queries: see `04-design-log.md` → Iterations section, or the original queue below for reference.

| # | Source | Status | Search queries |
|---|---|---|---|
| 1–3 | Elektron, TE, Norns | done | — |
| 4 | **Squarp Hapax** | next | `Squarp Hapax sequencer screen`, `Hapax OLED UI`, `Hapax sequencer interface layout` |
| 5 | **Dirtywave M8** | queued | `Dirtywave M8 tracker screen`, `M8 OLED interface`, `M8 tracker UI layout` |
| 6 | Polyend Tracker | queued | `Polyend Tracker screen UI`, `Polyend Tracker OLED interface` |
| 7 | Roland MC-101/707 | queued | `Roland MC-101 screen`, `MC-707 clip launcher UI` |

Use `WebSearch` and `WebFetch`. Read alt text, captions, forum posts — you cannot render images directly.

---

## Log entry format

Append to `04-design-log.md` after each turn:

```markdown
### Turn N — [Source name]

**Principles extracted:**
- [specific decision 1]
- [specific decision 2]
- [specific decision 3]

**Variations added:**
- `drawDash_SourceN` → Dashboard: [one-line description]
- `drawEdit_SourceN` → Track Edit: [one-line description]

**References used:**
- [URL or search query that yielded useful material]
```

---

## Iteration summary format

After completing the work phase, write to the log and hand off to the user:

```markdown
## Iteration [N] summary — [date]

**Type:** Explore / Consolidate
**Tastemakers covered:** [list]
**Total new variations:** N

**Standouts (agent opinion):**
- [variation name] — [why it's interesting]

**Concerns:**
- [anything that might violate design language or feel off]
```

Then tell the user what's new in 2–4 sentences. Name your standout. Name your concern.
Wait for their response before starting the next iteration.

---

## Project context

- **Hardware:** 256×64 mono OLED, 8 Track + 16 Step + 5 F + 8 Global buttons, 1 encoder.
- **Navigation:** Page+X combos. F-keys = in-page tabs. No top-level nav in footer.
- **BPM** lives in Tempo modal only — not on persistent display.
- Full sandbox setup + Canvas API: `02-sandbox-status.md`
- IA decisions: `00-ia.md`
- Track-type parameter inventory: `06-track-type-inventory.md`
