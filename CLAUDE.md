# Instructions
- Work is tracked with prep (`.prep/`): `prep next` for what to do, `prep list` for all open issues.
- Before changing code, run `prep knowledge find <area>` (e.g. `curve`, `slide`, `ui`, `serialization`):
  the design language, track-type decisions and known pitfalls live there, scoped to code paths.
- The UI design sandbox is `tools/ui-sandbox/` (Svelte; the user runs the dev server). Its
  iteration process lives in `tools/ui-sandbox/iteration/` and is started with `/iteration`.
- Everyday commands are in the `justfile` (`just` lists them): `just test [name]`, `just sim`,
  `just firmware`, `just sandbox`.

# Simulator Test Harness

The simulator can be driven headlessly from Python for fast, reproducible debugging.

## Build

```bash
just testsim          # or: just test <name> to build and run tests/ui/<name>_test.py
```

## Entry point

`src/apps/sequencer/tests/agent.py` — thin `Session` wrapper with helpers for all inputs and outputs. Run scripts from the project root:

```bash
python src/apps/sequencer/tests/my_script.py
```

## Key paths

| What | Where |
|---|---|
| Python bindings | `src/apps/sequencer/python/` (`simulator.cpp`, `project.cpp`, `sequencer.cpp`) |
| Built `.so` | `build/sim/release/src/apps/sequencer/python/testsim.*.so` |
| Controller (button map, press/wait/encoder) | `src/apps/sequencer/tests/testframework/controller.py` |
| Existing test scripts | `src/apps/sequencer/tests/ui/` |

## Accessing state

```python
import sys; sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

e = Environment()
c = Controller(e.simulator)
c.wait(3000)  # boot

p = e.sequencer.model.project
p.setTrackMode(0, tsseq.Track.TrackMode.Note)

seq = p.tracks[0].noteTrack.sequences[0]
seq.firstStep = 0
seq.lastStep = 3
seq.steps[0].gate = True
seq.steps[0].note = 7    # stored as scale-degree index

c.press("play")
e.simulator.wait(100)
cv   = e.simulator.targetState.dac.volts(0)   # volts, 0V=C4, 1V=C5
gate = bool(e.simulator.targetState.gateOutput[0])
led  = e.simulator.targetState.led.get(8)      # (red, green)
e.sim.screenshot("/tmp/out.png")
```

## Note encoding

Scale and root note are **project-global** (`project.scale` / `project.rootNote`), not per-sequence.

`step.note` stores a **scale-degree index**, not a semitone offset. There is **no chromatic scale**
in `model/Scale.cpp` — every entry is a 7-note mode, and index 0 is "Maj: Ionian":

- Ionian (index 0, 7 degrees): degrees 0..6 = 0, 2, 4, 5, 7, 9, 11 semitones; degree 7 → C5 (1.0 V)

So a step's degree 1 is **2 semitones**, not 1. Use `Scale::noteToVolts` / `noteFromVolts` to convert.