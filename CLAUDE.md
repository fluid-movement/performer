@CONTEXT.md

# Instructions
- always read the agent-docs before searching code, you will find features and code faster that way
- check `agent-docs/features-log.md` for a list of features built in this project — it links to detailed feature docs in `agent-docs/features/`

# Simulator Test Harness

The simulator can be driven headlessly from Python for fast, reproducible debugging.

## Build

```bash
cd build/sim/release && make -j testsim
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
seq.scale = 0        # 0 = Semitones, 1 = Major, etc.
seq.firstStep = 0
seq.lastStep = 3
seq.steps[0].gate = True
seq.steps[0].note = 12   # stored as scale-degree index

c.press("play")
e.simulator.wait(100)
cv   = e.simulator.targetState.dac.volts(0)   # volts, 0V=C4, 1V=C5
gate = bool(e.simulator.targetState.gateOutput[0])
led  = e.simulator.targetState.led.get(8)      # (red, green)
e.sim.screenshot("/tmp/out.png")
```

## Note encoding

`step.note` stores a **scale-degree index**, not a semitone offset:
- Semitones (index 0, 12 degrees): note 12 → C5 (1.0 V)
- Major (index 1, 7 degrees): note 7 → C5 (1.0 V), note 12 → A5 (1.75 V)

Use `Scale::noteToVolts` / `noteFromVolts` to convert between scales — `setScale()` on `NoteSequence` and `ArpSequence` does this automatically.