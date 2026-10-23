# Testing

Engine and UI behaviour is verified headlessly: the sequencer is built as a Python module
(`testsim`) and driven from scripts that set up a project, press buttons, advance simulated time and
read the DAC, gates, LEDs and LCD. See `prep knowledge find headless` for why this is the standard.

## Commands

| Command | Does |
|---|---|
| `just testsim` | builds `testsim` into `build/sim/release/src/apps/sequencer/python/` (configures CMake on first run) |
| `just test` | builds, then runs every `src/apps/sequencer/tests/ui/*_test.py`; fails if any fail |
| `just test note_v2` | builds and runs `tests/ui/note_v2_test.py` only |
| `just docs-check` | checks agent-docs for dead links and paths (not a code test) |

If the build directory is missing: `cmake --fresh -S . -B build/sim/release -DCMAKE_BUILD_TYPE=Release -DPLATFORM=sim`.
Run scripts from the project root; they add `build/sim/release/src/apps/sequencer/python` and
`src/apps/sequencer/tests` to `sys.path` themselves.

## Anatomy of a test

```python
import sys
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')
from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

e = Environment()
c = Controller(e.simulator)
c.wait(3000)                              # let the boot sequence finish
p = e.sequencer.model.project
p.setTrackMode(0, tsseq.Track.TrackMode.Note)
seq = p.tracks[0].noteTrack.sequences[0]
seq.steps[0].gate = True
c.press("play"); e.simulator.wait(100)
cv   = e.simulator.targetState.dac.volts(0)        # volts, 0 V = C4, 1 V = C5
gate = bool(e.simulator.targetState.gateOutput[0])
led  = e.simulator.targetState.led.get(8)          # (red, green)
c.screenshot("out")                                # PNG into the directory set by c.screenshotDir(...)
```

`src/apps/sequencer/tests/agent.py` wraps the same thing as a chainable `Session`
(`s.press("play").wait(500)`, `s.cv(0)`, `s.gate(0)`, `s.led(8)`, `s.screenshot(path)`,
`s.record_trace(ms, path)`) for interactive debugging.

Where things live:

| What | Where |
|---|---|
| Python bindings | `src/apps/sequencer/python/` (`simulator.cpp`, `project.cpp`, `sequencer.cpp`, `core.cpp`, `testsim.cpp`) |
| Button names, `press`/`wait`/`encoder` | `src/apps/sequencer/tests/testframework/controller.py` |
| Maintained tests | `src/apps/sequencer/tests/ui/*_test.py` |
| C++ unit tests (ctest) | `src/tests/unit/` (`TestCurve.cpp`, `TestScale.cpp`, core utils) |

## The maintained tests

| Test | Covers |
|---|---|
| `tests/ui/note_v2_test.py` | Note track: step length ratios, gate length, retrigger, arp retrigger, repeat, legato, slide arrival and curve, retrigger condition, queue-overflow regression, gate-off inertness, Version53 to 54 migration |
| `tests/ui/curve_encoder_skew_test.py` | Curve track: mirrored playback per run mode, percent encoder (1 per detent, SHIFT x10), skew endpoints, exponential SHPE, Shape Curve, Version49 to 51 migrations |
| `tests/ui/stochastic_v2_test.py` | Stochastic track: probability distributions, all-zero degrees stay silent, loop replay and mutation |
| `tests/ui/arp_v2_test.py` | Arp track: rhythm and mod euclidean, OFF / COMBINE / MASK, polyrhythmic overlap |
| `tests/ui/quantizer_loop_test.py` | Quantizer loop record/playback across trigger modes, window, start offset, clock-rate loop |
| `tests/ui/config_nav_test.py` | Config pages: modeless encoder, section jumps, active-tab walk, Left/Right LEDs |

`tests/ui/project.py` and the `tests/verify_*.py`, `tests/diag*.py` scripts are older ad hoc
scripts that `just test` does not run; treat them as examples, not as a safety net.

## Pitfalls

- **One `Environment()` per process.** Creating several crashes the simulator; tests share one.
- **Time is simulated.** `simulator.wait(N)` advances N ms. At 120 BPM with divisor 12 one step is
  125 ms. Use rising-edge detection on gates, not level sampling.
- **Pybind setters with defaulted `routed` arguments** raise `TypeError` when bound directly; wrap
  them in a lambda in `python/project.cpp` (`prep knowledge find pybind`).
- **A new model field needs a binding** in `python/project.cpp` before a test can set it.
- **Old formats.** `project.save(filename, version)` writes older `ProjectVersion`s (the
  `writerVersion()` branches in the model make this work), so migrations are tested by saving an old
  version and loading it. See [features/serialization.md](features/serialization.md).
- **Step notes are scale degrees**, and the default scale is Ionian, so degree 1 is 2 semitones
  (`prep knowledge find scale`).
- **Boot takes about 3 s** of simulated time; wait it out before pressing buttons.

## Adding a test

Put `<name>_test.py` in `src/apps/sequencer/tests/ui/` (so `just test <name>` and `just test` find
it), write plain `test_*` functions that `assert` (or a `check()` helper that exits non-zero), call them
from `if __name__ == "__main__":`, and list the test in the table above.
