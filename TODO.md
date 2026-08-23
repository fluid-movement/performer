# Stochastic Track V2 — Task List

Work through these in order. Each phase builds on the last. The LOOP tab and `loopChance`
parameter are already shipped — start from Phase 1 below.

---

## Phase 0 — Pre-flight checks

- [x] Confirm `BlendMode::Sub` exists in `src/libs/canvas/Canvas.h` (needed for NOTE tab cursor)
- [x] Find the correct API to convert scale degree + octave offset → CV voltage
      (look at how `NoteTrackEngine` does it — search for `Scale::note` or `noteToVolts`)
- [x] Check `StochasticTrack` bias fields: decide which survive V2
      (`noteProbabilityBias` → degree draw; `lengthBias` → duration draw; `retriggerProbabilityBias` → drop)

---

## Phase 1 — Model: replace Step class + old fields ✅

### `src/apps/sequencer/model/StochasticSequence.h`

- [x] Remove the `Step` inner class entirely
- [x] Remove `typedef std::array<Step, CONFIG_STEP_COUNT> StepArray`
- [x] Remove private fields: `_restProbability2/4/8`, `_lowOctaveRange`, `_highOctaveRange`,
      `_lengthModifier`, `_firstStep`, `_lastStep`
- [x] Remove all accessor methods for deleted fields
- [x] Add private fields: `_degreeProb[7]`, `_octaveProb[5]`, `_durationProb[6]`
- [x] Add accessors: `degreeProb/setDegreeProb/editDegreeProb/printDegreeProb` (same for oct/dur)
- [x] Remove step-array utilities
- [x] Remove `_steps` private member

### `src/apps/sequencer/model/StochasticSequence.cpp`

- [x] Remove `Step::clear()`, `Step::write()`, `Step::read()` implementations
- [x] In `clear()`: reset new arrays with correct defaults
- [x] In `write()`: write new arrays, remove old fields
- [x] In `read()`: skip old fields (via `skip()`), read new arrays at Version44

### `src/apps/sequencer/model/ProjectVersion.h`

- [x] Add `Version44 = 44`

---

## Phase 2 — Engine: replace Phase 1 draw logic ✅

### `src/apps/sequencer/engine/StochasticEngine.h`

- [x] Simplify `StochasticLoopStep`: drop `Step _step` + `retrigger`; keep `_degreeIndex, _gate, _noteValue, _stepLength`
- [x] Remove `evalRestProbability()` and `sampleStepIndex()` from public interface
- [x] New draw helpers: `drawDegree()`, `drawOctave()`, `drawDuration()` (file-static in .cpp)

### `src/apps/sequencer/engine/StochasticEngine.cpp`

- [x] Remove `evalStepGate`, `evalStepRetrigger`, `evalStepLength`, `evalStepNote`, `evalRestProbability`, `sampleStepIndex`
- [x] Add `drawDegree/drawOctave/drawDuration` + `degreeToCv` static helpers
- [x] Rewrite Phase 1 with automatic density check
- [x] Remove `_skips` member and rest-skip logic
- [x] Simplify Phase 2 loop replay (no retrigger)
- [x] Update mutation block to use new draw helpers

---

## Phase 3 — UI page: new NOTE/OCT/LEN tab painters ✅

### `src/apps/sequencer/ui/pages/StochasticSequenceEditPage.h`

- [x] Remove `Layer` typedef, `_showDetail`, `_stepSelection`, `_keyPressEventTracker`
- [x] Add `_activeTab`, `_cursor`, `_heldStep`

### `src/apps/sequencer/ui/pages/StochasticSequenceEditPage.cpp`

- [x] New `functionNames[]` = `{"NOTE", "OCT", "LEN", "LOOP"}`
- [x] Implement `drawNoteTab()`, `drawOctTab()`, `drawLenTab()` (prob bar painters)
- [x] `drawLoopTab()` unchanged
- [x] Rewrite `keyPress()` and `encoder()` for hold-and-edit UX
- [x] Update `updateLeds()`

---

## Phase 4 — Build and test ✅

- [x] Build `testsim` target: `cd build/sim/release && make -j testsim` — passes
- [x] Python tests at `src/apps/sequencer/tests/ui/stochastic_v2_test.py`:
  - [x] `test_all_degrees_zero_no_gate` — all rests when degreeProb=0
  - [x] `test_uniform_degree_probs` — dense gates + CV in range
  - [x] `test_loop_mode_fires_gates` — loop mode continues firing gates
  - [x] `test_loop_mutation` — loopChance=15 causes variation
- [ ] Take simulator screenshot of NOTE / OCT / LEN / LOOP tabs (manual step)
- [ ] Verify hold-step + encoder edits correctly (manual step)
