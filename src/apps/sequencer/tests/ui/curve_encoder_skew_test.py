"""Curve track: 1-unit encoder steps and full-range skew.

Covers the two behaviours changed in ProjectVersion50:
  - all 0..100 parameters step by exactly 1 per encoder detent (10 with SHIFT),
    symmetrically (up then down returns to the original value)
  - skew 0 / 100 produce a genuinely instant attack / release

Run from the project root:
    python src/apps/sequencer/tests/ui/curve_encoder_skew_test.py
"""

import os
import sys
import tempfile

sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')

from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

SHAPE_MAX = 127   # 7-bit shape/skew fields
LEVEL_MAX = 255   # 8-bit level/offset fields

# mirrors CurveSequence::Step::fromPercent / toPercent
def from_percent(percent, raw_max):
    return (max(0, min(100, percent)) * raw_max + 50) // 100

def to_percent(raw, raw_max):
    return (raw * 100 + raw_max // 2) // raw_max

failures = []

def check(name, cond, detail=""):
    if cond:
        print("  PASS  %s" % name)
    else:
        print("  FAIL  %s %s" % (name, detail))
        failures.append(name)


# ---------------------------------------------------------------------------

e = Environment()
c = Controller(e.simulator)
c.wait(3000)

p = e.sequencer.model.project
p.setTrackMode(0, tsseq.Track.TrackMode.Curve)
seq = p.tracks[0].curveTrack.sequences[0]
step0 = seq.steps[0]


# --- 1. percent round trip -------------------------------------------------
print("\n1. percent round trip is exact for every value 0..100")

for name in ("shapePercent", "skewPercent", "levelPercent", "offsetPercent"):
    bad = []
    for value in range(101):
        setattr(step0, name, value)
        if getattr(step0, name) != value:
            bad.append((value, getattr(step0, name)))
    check("%s round trips" % name, not bad, "mismatches: %s" % bad[:5])

# clamping at the ends
step0.levelPercent = -20
check("percent clamps low", step0.levelPercent == 0, "got %d" % step0.levelPercent)
step0.levelPercent = 180
check("percent clamps high", step0.levelPercent == 100, "got %d" % step0.levelPercent)


# --- 2. one unit per detent, via the UI ------------------------------------
print("\n2. encoder moves the value by exactly 1 per detent")

c.selectPage("steps")
c.wait(50)

seq.segmentCount = 4
for i in range(4):
    seq.steps[i].length = 4

FUNCTION_KEYS = {
    "shapePercent":  "f1",   # SHPE
    "skewPercent":   "f2",   # SKEW
    "levelPercent":  "f4",   # LVL
    "offsetPercent": "f5",   # OFST
}

def clear_selection():
    # a plain step press drops StepSelection back to Immediate mode and leaves
    # nothing selected, so the next function key is read as a tab switch
    c.press("step1")
    c.wait(20)

def select_tab(param):
    # the function key must be pressed with nothing selected, otherwise the page
    # reads it as a DEL / DUPL / RSET action on the selected segment
    clear_selection()
    c.press(FUNCTION_KEYS[param])
    c.wait(20)

def turn(param, detents, shift=False):
    """Hold segment 1 and rotate the encoder, returning the resulting value."""
    select_tab(param)
    # the step must go down before SHIFT: SHIFT held while a step key goes down
    # puts StepSelection into Persist mode, which keeps the segment selected
    # after release (StepSelection.h)
    c.down("step1")
    c.wait(20)
    if shift:
        c.down("shift")
        c.wait(20)
    c.rotateEncoder(detents)
    c.wait(20)
    if shift:
        c.up("shift")
    c.up("step1")
    c.wait(20)
    return getattr(seq.steps[0], param)

for param in FUNCTION_KEYS:
    setattr(seq.steps[0], param, 50)
    got = turn(param, 5)
    check("%s: +5 detents -> +5" % param, got == 55, "got %d" % got)

    setattr(seq.steps[0], param, 50)
    got = turn(param, -5)
    check("%s: -5 detents -> -5" % param, got == 45, "got %d" % got)


# --- 3. symmetry -----------------------------------------------------------
print("\n3. turning back returns to the original value")

for param in FUNCTION_KEYS:
    setattr(seq.steps[0], param, 37)
    turn(param, 7)
    got = turn(param, -7)
    check("%s: +7 then -7 returns to 37" % param, got == 37, "got %d" % got)


# --- 4. shift is the coarse (x10) modifier ---------------------------------
print("\n4. SHIFT + encoder moves by 10")

for param in FUNCTION_KEYS:
    setattr(seq.steps[0], param, 40)
    got = turn(param, 2, shift=True)
    check("%s: shift +2 detents -> +20" % param, got == 60, "got %d" % got)


# --- 5. multi-step editing keeps relative offsets --------------------------
print("\n5. editing several segments at once preserves their differences")

seq.steps[0].levelPercent = 20
seq.steps[1].levelPercent = 50
select_tab("levelPercent")
seq.steps[0].levelPercent = 20
seq.steps[1].levelPercent = 50
c.down("step1")
c.down("step2")
c.wait(20)
c.rotateEncoder(3)
c.wait(20)
c.up("step2")
c.up("step1")
c.wait(20)
check("both segments moved by +3",
      (seq.steps[0].levelPercent, seq.steps[1].levelPercent) == (23, 53),
      "got %d, %d" % (seq.steps[0].levelPercent, seq.steps[1].levelPercent))


# --- 6. skew reaches the extremes (curve math) -----------------------------
print("\n6. skew 0 / 100 give an instant attack / release")

evalSegment = tsseq.CurveSequence.evalSegment

# skew 0: full amplitude at the very start of the segment, decaying to 0
check("skew 0 starts at full level", abs(evalSegment(0.0, 0.5, 0.0) - 1.0) < 1e-5,
      "got %f" % evalSegment(0.0, 0.5, 0.0))
check("skew 0 ends at zero", evalSegment(1.0, 0.5, 0.0) < 1e-5,
      "got %f" % evalSegment(1.0, 0.5, 0.0))
check("skew 0 decays monotonically",
      all(evalSegment(i / 32.0, 0.5, 0.0) > evalSegment((i + 1) / 32.0, 0.5, 0.0)
          for i in range(32)))

# skew 1: rises across the segment and is cut off at the end
check("skew 100 starts at zero", evalSegment(0.0, 0.5, 1.0) < 1e-5,
      "got %f" % evalSegment(0.0, 0.5, 1.0))
check("skew 100 ends at full level", abs(evalSegment(1.0, 0.5, 1.0) - 1.0) < 1e-5,
      "got %f" % evalSegment(1.0, 0.5, 1.0))
check("skew 100 rises monotonically",
      all(evalSegment(i / 32.0, 0.5, 1.0) < evalSegment((i + 1) / 32.0, 0.5, 1.0)
          for i in range(32)))

# the smallest representable non-zero skew must still be well behaved
smallest = 1.0 / SHAPE_MAX
values = [evalSegment(i / 64.0, 0.5, smallest) for i in range(65)]
check("smallest non-zero skew stays in range",
      all(0.0 <= v <= 1.0 for v in values), "range %f..%f" % (min(values), max(values)))

# shape 100 is still the flat hold, at any skew
check("shape 100 holds flat",
      all(abs(evalSegment(i / 8.0, 1.0, s) - 1.0) < 1e-5
          for i in range(9) for s in (0.0, 0.5, 1.0)))


# --- 7. the sharp edge reaches the CV output -------------------------------
print("\n7. the instant edge is visible on the DAC")

def sample_cv(skew_percent, ms=4000, interval=10):
    seq.segmentCount = 1
    s = seq.steps[0]
    s.length = 16
    s.shapePercent = 50
    s.levelPercent = 100
    s.offsetPercent = 0
    s.skewPercent = skew_percent
    e.simulator.wait(400)          # let the current segment finish
    samples = []
    for _ in range(ms // interval):
        e.simulator.wait(interval)
        samples.append(e.simulator.targetState.dac.volts(0))
    return samples

c.press("play")
e.simulator.wait(200)

samples = sample_cv(0)
span = max(samples) - min(samples)
jumps = [b - a for a, b in zip(samples, samples[1:])]
check("skew 0 produces an instant rising edge", max(jumps) > 0.8 * span,
      "largest rise %.3f of span %.3f" % (max(jumps), span))
check("skew 0 has no instant falling edge", min(jumps) > -0.2 * span,
      "largest fall %.3f of span %.3f" % (min(jumps), span))

samples = sample_cv(100)
span = max(samples) - min(samples)
jumps = [b - a for a, b in zip(samples, samples[1:])]
check("skew 100 produces an instant falling edge", min(jumps) < -0.8 * span,
      "largest fall %.3f of span %.3f" % (min(jumps), span))
check("skew 100 has no instant rising edge", max(jumps) < 0.2 * span,
      "largest rise %.3f of span %.3f" % (max(jumps), span))

c.press("play")
e.simulator.wait(100)


# --- 8. Version49 projects migrate ----------------------------------------
print("\n8. a Version49 project loads with its curves intact")

# Old layout packed shape into bits 0-5 and skew into bits 6-11; the new one uses
# bits 0-6 and 7-13. Writing the old bit pattern through the new fields lets the
# project be saved with a Version49 stamp, which is what triggers the migration.
OLD_SHAPE, OLD_SKEW = 33, 20
OLD_WORD_LOW = OLD_SHAPE | (OLD_SKEW << 6)
MARKER_OFFSET, MARKER_LEVEL = 78, 44

Layer = tsseq.CurveSequence.Layer
marker = seq.steps[0]
marker.setLayerValue(Layer.Shape, OLD_WORD_LOW & 0x7f)
marker.setLayerValue(Layer.Skew, (OLD_WORD_LOW >> 7) & 0x7f)
marker.offsetPercent = MARKER_OFFSET
marker.levelPercent = MARKER_LEVEL

path = os.path.join(tempfile.mkdtemp(), "v49.pro")
p.save(path, 49)

# load into a standalone Project: swapping the model under the running engine
# trips an "invalid track mode" assert in TrackEngine
loaded = tsseq.Project()
loaded.load(path)
migrated = loaded.tracks[0].curveTrack.sequences[0].steps[0]

expected_shape_raw = (OLD_SHAPE * 127 + 31) // 63
expected_skew_raw = (OLD_SKEW * 127 + 31) // 63
old_shape_pct = round(OLD_SHAPE / 63.0 * 100)
old_skew_pct = round(OLD_SKEW / 63.0 * 100)

check("migrated shape raw matches the rescale",
      migrated.layerValue(Layer.Shape) == expected_shape_raw,
      "got %d, expected %d" % (migrated.layerValue(Layer.Shape), expected_shape_raw))
check("migrated skew raw matches the rescale",
      migrated.layerValue(Layer.Skew) == expected_skew_raw,
      "got %d, expected %d" % (migrated.layerValue(Layer.Skew), expected_skew_raw))
check("migrated shape keeps its old meaning",
      abs(migrated.shapePercent - old_shape_pct) <= 1,
      "got %d%%, old was %d%%" % (migrated.shapePercent, old_shape_pct))
check("migrated skew keeps its old meaning",
      abs(migrated.skewPercent - old_skew_pct) <= 1,
      "got %d%%, old was %d%%" % (migrated.skewPercent, old_skew_pct))
check("offset survives untouched", migrated.offsetPercent == MARKER_OFFSET,
      "got %d" % migrated.offsetPercent)
check("level survives untouched", migrated.levelPercent == MARKER_LEVEL,
      "got %d" % migrated.levelPercent)

# a current-version project must round trip with no migration applied
p.save(path)
current = tsseq.Project()
current.load(path)
same = current.tracks[0].curveTrack.sequences[0].steps[0]
check("Version50 round trips unchanged",
      (same.layerValue(Layer.Shape), same.layerValue(Layer.Skew),
       same.offsetPercent, same.levelPercent) ==
      (marker.layerValue(Layer.Shape), marker.layerValue(Layer.Skew),
       MARKER_OFFSET, MARKER_LEVEL))

os.remove(path)


# ---------------------------------------------------------------------------

print("\n%s" % ("-" * 60))
if failures:
    print("%d FAILED: %s" % (len(failures), ", ".join(failures)))
    sys.exit(1)
print("all checks passed")
