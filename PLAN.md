# Stochastic Track V2 — Full Refactor Plan

## What we're building

A complete replacement of the stochastic track with a SIG (Stochastic Inspiration Generator)-inspired
design. The TRUNK model (12-step chromatic grid with per-step gate/note/retrigger/condition fields) is
dropped entirely. The new model is built around three global probability distributions — scale degrees,
octave offsets, and note durations — that the engine samples on every step.

**User-facing result:**
- NOTE tab: 7 vertical probability bars, one per scale degree; height = selection probability
- OCT tab: 5 probability bars for octave offset (-2 to +2)
- LEN tab: 6 probability bars for duration (1/16 → 2 bars)
- LOOP tab: Turing-machine mutation (already shipped)
- Hold a step button + turn encoder to edit that bar's probability (same UX pattern as the rest of the module)
- Automatic density: rest probability is implicit — derived from the total probability mass of the degree bars

---

## What was already done (LOOP mode)

- `loopChance` parameter added to `StochasticSequence` (serialized at ProjectVersion 43)
- Engine mutation logic: on each loop-playback step, `mutationRng.nextRange(15) < loopChance` triggers
  a re-draw of that buffer slot from the current distribution
- `drawLoopTab()` added to `StochasticSequenceEditPage`
- F4 toggles the LOOP tab view; step buttons 1–12 set loop length; encoder sets `loopChance`
- `sampleStepIndex()` extracted as a shared helper; `clamp(p, -1, Max)` bug fixed to `clamp(p, 0, Max)`
- `mutationRng` separated from main `rng` to protect the sequence RNG stream

---

## Breaking changes

- Old stochastic sequences will load with default V2 values (all degree bars at 8/15, octave centred,
  medium length). There is no migration. This is the same approach taken for Curve V1.
- A new `ProjectVersion` (Version44) marks the V2 model fields.
- `_steps` array, `_restProbabilityX` fields, `_lengthModifier`, `_lowOctaveRange`, `_highOctaveRange`,
  `_firstStep`, `_lastStep` are all removed from the serialised model.

---

## New model schema

### Fields to **keep** (behaviour unchanged)

| Field | Purpose |
|---|---|
| `_divisor` | Step clock divider |
| `_resetMeasure` | Bar-aligned reset |
| `_runMode` | Forward / reverse / random / etc. |
| `_reseed` | Re-randomise seed on next trigger |
| `_sequenceFirstStep` / `_sequenceLastStep` | Loop window within buffer |
| `_useLoop` / `_clearLoop` | Loop mode on/off |
| `_loopChance` | Mutation probability (0–15) |

### Fields to **add**

```cpp
uint8_t _degreeProb[7];    // note probability for scale degrees 1–7 (each 0–15)
uint8_t _octaveProb[5];    // octave-offset probability (-2, -1, 0, +1, +2) (each 0–15)
uint8_t _durationProb[6];  // note duration probability (1/16, 1/8, 1/4, 1/2, 1, 2) (each 0–15)
```

Default values on `clear()`:
- `_degreeProb`: all 8 (medium probability across all degrees)
- `_octaveProb`: {0, 0, 15, 0, 0} (centre octave only)
- `_durationProb`: {0, 0, 15, 8, 0, 0} (mostly quarter notes, some half notes)

### Fields to **remove**

- `_restProbability2`, `_restProbability4`, `_restProbability8`
- `_lowOctaveRange`, `_highOctaveRange`
- `_lengthModifier`
- `_firstStep`, `_lastStep` (TRUNK sequence window, not loop window — different fields)
- Entire `Step` inner class and `_steps: StepArray`

### Accessor pattern for new arrays

Follow the same pattern as `loopChance()`:

```cpp
int degreeProb(int degree) const { return _degreeProb[degree]; }
void setDegreeProb(int degree, int prob) {
    _degreeProb[degree] = clamp(prob, 0, 15);
}
void editDegreeProb(int degree, int value, bool shift) {
    setDegreeProb(degree, degreeProb(degree) + value * (shift ? 4 : 1));
}
void printDegreeProb(int degree, StringBuilder &str) const {
    str("%d", degreeProb(degree));
}
// Same pattern for octaveProb(int i) and durationProb(int i)
```

---

## Engine rewrite (`StochasticEngine.cpp/h`)

### Remove entirely

- `evalStepGate()` — replaced by automatic density
- `evalStepRetrigger()` — retrigger dropped in V2
- `evalStepLength()` — replaced by `drawDuration()`
- `evalStepNote()` — replaced by `drawNote()`
- `evalRestProbability()` — replaced by automatic density
- `_skips` member variable — the skip-rest mechanism was driven by `evalRestProbability`

### Simplify `StochasticLoopStep`

Drop the `StochasticSequence::Step _step` member. V2 replay only needs:

```cpp
class StochasticLoopStep {
    int _degreeIndex;    // which degree (0–6) was drawn
    bool _gate;          // did gate fire
    float _noteValue;    // pre-computed CV output
    uint32_t _stepLength; // pre-computed ticks
};
```

### New draw helpers (static functions)

```cpp
// Draw a scale degree (0–6) by probability. Returns -1 if all probs are 0.
static int drawDegree(const StochasticSequence &sequence);

// Draw an octave index (0–4 mapping to -2..+2). Returns 2 (centre) if all probs are 0.
static int drawOctave(const StochasticSequence &sequence);

// Draw a duration index (0–5). Returns 2 (quarter note) if all probs are 0.
static int drawDuration(const StochasticSequence &sequence);
```

All three use the same `getNextWeightedPitch()` mechanism: build a probability vector from the
sequence's `degreeProb`/`octaveProb`/`durationProb` arrays and call `getNextWeightedPitch`.

### Automatic density

Gate fires when the random draw "lands in the probability mass". Implementation:

```cpp
// automatic density: rest probability is implicit from total degree probability mass
int sum = 0;
for (int i = 0; i < 7; i++) sum += sequence.degreeProb(i);
bool gate = rng.nextRange(7 * 15) < uint32_t(sum);
```

When all degree bars are at 15 (max): `sum = 105`, always gates.  
When all at 0: `sum = 0`, always rests (returns early).  
At intermediate: rests proportional to remaining probability mass.

### Duration mapping (index → ticks)

```cpp
static const int DURATION_MULTIPLIERS[6] = {
    1, 2, 4, 8, 16, 32   // multipliers of (divisor / 4)
};
// stepLength = (divisor / 4) * DURATION_MULTIPLIERS[durationIndex]
// Index 0 → 1/16 note, 2 → 1/4 note (=1 step), 5 → 2 bars
```

### Phase 1 rewrite (buffer fill)

```
drawDegree()  → degreeIndex (0–6)
drawOctave()  → octaveIndex (0–4)  → octaveOffset = octaveIndex - 2
drawDuration() → durationIndex (0–5) → stepLength in ticks
automatic density check → gate bool
degree + octave + selectedScale + rootNote + transpose → noteValue (float, volts)
append StochasticLoopStep(degreeIndex, gate, noteValue, stepLength) to _lockedSteps
```

### Phase 2 simplification (loop replay)

The locked step already has all computed values. Replay is trivial:
```
gate      = lockedStep.gate()
noteValue = lockedStep.noteValue()
stepLength = lockedStep.stepLength()
// no retrigger, no condition eval
```

Mutation uses the same draw helpers (`drawDegree/Octave/Duration`) but with `mutationRng` for the trigger roll (already correct architecture from LOOP mode work).

### Helpers to remove from `StochasticEngine.h`

- `evalRestProbability()` (public, becomes unreachable)
- `sampleStepIndex()` — replaced by `drawDegree()`; remove the declaration

---

## UI page rewrite (`StochasticSequenceEditPage.cpp/h`)

### Tab layout

```cpp
static const char *functionNames[] = { "NOTE", "OCT", "LEN", "LOOP" };

enum class Tab { Note = 0, Oct = 1, Len = 2, Loop = 3 };
```

There are only 4 tabs. `_loopTabActive` flag stays for LOOP tab. Replace the `Layer` enum usage with a
simple tab index: `_activeTab` (0–3) stored on the page. The `setLayer()` / `layer()` calls can be
dropped entirely — they were for per-step layer selection which no longer exists.

### Input: step buttons

| Tab | Step button 1–N | Effect |
|---|---|---|
| NOTE | 1–7 | Set `_cursor = stepIndex`; hold + encoder adjusts `degreeProb[_cursor]` |
| OCT | 1–5 | Set `_cursor = stepIndex`; hold + encoder adjusts `octaveProb[_cursor]` |
| LEN | 1–6 | Set `_cursor = stepIndex`; hold + encoder adjusts `durationProb[_cursor]` |
| LOOP | 1–12 | Set `sequenceLastStep = stepIndex` (sets loop length) |

Hold-and-edit UX: register button-down, track held index in `_heldStep`. Encoder turns while button
held → edit that slot. On button-up, clear `_heldStep`. This is the standard Performer UX pattern.

### Input: encoder (no step held)

| Tab | Encoder |
|---|---|
| NOTE | Move cursor left/right through degrees |
| OCT | Move cursor left/right through octave slots |
| LEN | Move cursor left/right through duration slots |
| LOOP | Edit `loopChance` (already implemented) |

### Draw: NOTE tab

Port directly from `drawStochasticV2(tab === 0)` in the sandbox:

- 7 columns, each 36px wide, starting at x=2
- Column height = `degreeProb[i] * 39 / 15` px, drawn bottom-up from y=48
- Cursor column: bright background (`fillRect`), bar drawn via Sub blend
- Playing degree: bright outline rect around the column
- Degree labels "1"–"7" at y=53

Canvas API matches sandbox 1:1. `BlendMode::Sub` is available in the firmware canvas.

### Draw: OCT tab

Port from `drawStochasticV2(tab === 1)`:

- 5 columns, 36px wide, centred: `xOff = (256 - 5*36) / 2 = 38`
- Same bar geometry as NOTE (barFloor=48, barTop=10)
- Column 2 (index 2, "0") has a `MediumLow` tick mark at `barTop`
- Labels: "-2", "-1", " 0", "+1", "+2" at y=53

### Draw: LEN tab

Port from `drawStochasticV2(tab === 2)`:

- 6 columns, 36px wide, centred: `xOff = (256 - 6*36) / 2 = 20`
- Same bar geometry
- Labels: "1/16", "1/8", "1/4", "1/2", "1", "2" at y=53

### Draw: LOOP tab

Already implemented via `drawLoopTab()`. No change needed.

### LED state

```cpp
// NOTE tab LEDs: steps 1–7 map to degrees 0–6
for (int i = 0; i < 7; i++) {
    if (i == currentDegree) leds.step[i] = LedColor::Amber;       // playing
    else if (i == _cursor)  leds.step[i] = LedColor::Amber;       // cursor
    else if (sequence.degreeProb(i) > 0) leds.step[i] = LedColor::Green;
    else leds.step[i] = LedColor::Off;
}
```

OCT and LEN tabs: same pattern for their respective step counts (5 and 6).

---

## Serialization

### New version

```cpp
// stochastic track V2: replace per-step model with degree/octave/duration probability arrays
Version44 = 44,
```

### `write()`

Remove writes for: `_restProbabilityX`, `_lowOctaveRange`, `_highOctaveRange`, `_lengthModifier`,
`_firstStep`, `_lastStep`, `_steps` array.

Add writes for: `_degreeProb[7]`, `_octaveProb[5]`, `_durationProb[6]`.

### `read()`

Reads for removed fields are gated with their original version guards — they still read from old project
files but discard into a dummy variable so the byte stream stays aligned. The new arrays are read with
`reader.read(_degreeProb, ProjectVersion::Version44)` etc. — missing on old files → stays at default.

---

## Files to modify

| File | Change |
|---|---|
| `model/StochasticSequence.h` | Replace Step class + fields; add degreeProb/octaveProb/durationProb |
| `model/StochasticSequence.cpp` | clear(), write(), read() |
| `model/ProjectVersion.h` | Add Version44 |
| `engine/StochasticEngine.h` | Simplify StochasticLoopStep; remove old declarations |
| `engine/StochasticEngine.cpp` | Replace Phase 1, simplify Phase 2, new draw helpers |
| `ui/pages/StochasticSequenceEditPage.h` | New tab/cursor state; remove Layer references |
| `ui/pages/StochasticSequenceEditPage.cpp` | Full draw/input rewrite for NOTE/OCT/LEN tabs |

---

## Things to verify before starting

1. **`BlendMode::Sub`** — confirm it exists in the Canvas API used by the firmware painter.
   Check `src/libs/canvas/Canvas.h` or similar.

2. **`Scale::noteToVolts` / degree → semitone** — confirm the correct API call for converting
   scale degree + octave offset to a CV voltage. The note track engine uses `Scale::note()` — check
   how it maps a degree index to a semitone and then to volts.

3. **`StochasticTrack` bias fields** — `_stochasticTrack.noteProbabilityBias()`,
   `gateProbabilityBias()`, `lengthBias()`, `retriggerProbabilityBias()` are track-level CV-modulation
   inputs. Retrigger bias can be removed. The others need reassessment: `noteProbabilityBias` may apply
   to the degree distribution; `lengthBias` may apply to the duration distribution. Decide before
   touching the engine.

4. **`bufferLoopLength()`** — this is derived from `_sequenceLastStep - _sequenceFirstStep + 1`. It
   is unchanged by this refactor. Confirm the loop buffer size semantics still make sense with V2 step
   lengths that can span multiple clock ticks.
