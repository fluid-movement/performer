"""Curve track: 1-unit encoder steps and full-range skew.

Covers the two behaviours changed in ProjectVersion50:
  - all 0..100 parameters step by exactly 1 per encoder detent (10 with SHIFT),
    symmetrically (up then down returns to the original value)
  - skew 0 / 100 produce a genuinely instant attack / release

Run from the project root:
    python src/apps/sequencer/tests/ui/curve_encoder_skew_test.py
"""

import math
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


# --- 6b. the low end of shape is an exponential fall, not a bell -----------
print("\n6b. shape 0 falls exponentially (cusp at the peak, not a rounded top)")

# The defining difference from the old sin^8 family: every power of a sine has
# zero slope at the peak, so it can only ever produce a rounded top. The old
# curve dropped 0.004 over the first 2% of the segment; an exponential drops ~0.11.
drop = 1.0 - evalSegment(0.02, 0.0, 0.0)
check("shape 0 has a cusp at the peak", drop > 0.05,
      "dropped only %.4f over the first 2%%" % drop)
rise = 1.0 - evalSegment(0.98, 0.0, 1.0)
check("shape 0 cusps at the peak with skew 100 too", rise > 0.05,
      "rose only %.4f over the last 2%%" % rise)

# a front-loaded decay: each successive drop is smaller than the last, unlike the
# bell, which was nearly flat and then plunged
steps = [evalSegment(i / 32.0, 0.0, 0.0) for i in range(33)]
drops = [a - b for a, b in zip(steps, steps[1:])]
check("shape 0 decays monotonically", all(d > 0 for d in drops))
check("shape 0 decay is front loaded (convex)",
      all(b <= a + 1e-6 for a, b in zip(drops, drops[1:])),
      "drops: %s" % [round(d, 4) for d in drops[:6]])

# segments must still join cleanly: exactly 1 at the peak, exactly 0 at the edges.
# The upper shapes matter here too: sine underflows at the segment edge and a small
# exponent would turn a tiny positive into a visibly non-zero value.
for shape in (0.0, 0.25, 0.5, 0.75, 0.9):
    check("shape %.2f: peak is 1.0 and edges are 0.0 at skew 0" % shape,
          abs(evalSegment(0.0, shape, 0.0) - 1.0) < 1e-5
          and evalSegment(1.0, shape, 0.0) < 1e-5)
    check("shape %.2f: peak is 1.0 and edges are 0.0 at skew 100" % shape,
          abs(evalSegment(1.0, shape, 1.0) - 1.0) < 1e-5
          and evalSegment(0.0, shape, 1.0) < 1e-5)
    check("shape %.2f: peak is 1.0 and edges are 0.0 at skew 50" % shape,
          abs(evalSegment(0.5, shape, 0.5) - 1.0) < 1e-5
          and evalSegment(0.0, shape, 0.5) < 1e-5
          and evalSegment(1.0, shape, 0.5) < 1e-5)

# the two halves of the shape range must meet without a discontinuity
check("shape is continuous across the midpoint",
      all(abs(evalSegment(i / 16.0, 0.4999, 0.3) - evalSegment(i / 16.0, 0.5001, 0.3)) < 1e-3
          for i in range(17)))

# shape 50 is still exactly the half sine
check("shape 50 is still the half sine",
      all(abs(evalSegment(i / 16.0, 0.5, 0.5) - math.sin((i / 16.0) * math.pi)) < 1e-5
          for i in range(17)))

# nothing leaves the unit range anywhere in the parameter space
grid = [evalSegment(i / 20.0, sh / 10.0, sk / 10.0, k)
        for i in range(21) for sh in range(11) for sk in range(11) for k in (4.0, 6.0, 8.0)]
check("output stays within 0..1 everywhere",
      all(-1e-6 <= v <= 1.0 + 1e-6 for v in grid),
      "range %f..%f" % (min(grid), max(grid)))

# the Shape Curve setting orders the decays
gentle, medium, snappy = (evalSegment(0.1, 0.0, 0.0, k) for k in (4.0, 6.0, 8.0))
check("Gentle / Medium / Snappy give progressively steeper falls",
      gentle > medium > snappy,
      "got %.3f / %.3f / %.3f" % (gentle, medium, snappy))
check("the steepness setting has no effect at shape 50",
      abs(evalSegment(0.25, 0.5, 0.0, 4.0) - evalSegment(0.25, 0.5, 0.0, 8.0)) < 1e-6)


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


# --- 7b. the Shape Curve track setting reaches the engine ------------------
print("\n7b. the Shape Curve setting changes the CV output")

def sample_shape_curve(setting):
    track.shapeCurve = setting
    seq.segmentCount = 1
    s = seq.steps[0]
    s.length = 16
    s.shapePercent = 0
    s.skewPercent = 0
    s.levelPercent = 100
    s.offsetPercent = 0
    e.simulator.wait(2200)         # let the current segment finish
    samples = []
    for _ in range(200):
        e.simulator.wait(10)
        samples.append(e.simulator.targetState.dac.volts(0))
    return samples

track = p.tracks[0].curveTrack
ShapeCurve = tsseq.CurveTrack.ShapeCurve

gentle = sample_shape_curve(ShapeCurve.Gentle)
snappy = sample_shape_curve(ShapeCurve.Snappy)

# align both runs on their peak, then compare the same point in the decay
def after_peak(samples, offset):
    return samples[samples.index(max(samples)) + offset]

check("Snappy decays faster than Gentle on the DAC",
      after_peak(snappy, 20) < after_peak(gentle, 20),
      "snappy %.3fV vs gentle %.3fV" % (after_peak(snappy, 20), after_peak(gentle, 20)))

track.shapeCurve = ShapeCurve.Medium
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

check("a Version49 project defaults to Shape Curve Medium",
      loaded.tracks[0].curveTrack.shapeCurve == ShapeCurve.Medium,
      "got %s" % loaded.tracks[0].curveTrack.shapeCurve)

# a project saved before Version51 predates the setting and must default too
track.shapeCurve = ShapeCurve.Snappy
p.save(path, 50)
v50 = tsseq.Project()
v50.load(path)
check("a Version50 project defaults to Shape Curve Medium",
      v50.tracks[0].curveTrack.shapeCurve == ShapeCurve.Medium,
      "got %s" % v50.tracks[0].curveTrack.shapeCurve)

# a current-version project must round trip with no migration applied
p.save(path)
current = tsseq.Project()
current.load(path)
same = current.tracks[0].curveTrack.sequences[0].steps[0]
check("Version51 round trips unchanged",
      (same.layerValue(Layer.Shape), same.layerValue(Layer.Skew),
       same.offsetPercent, same.levelPercent) ==
      (marker.layerValue(Layer.Shape), marker.layerValue(Layer.Skew),
       MARKER_OFFSET, MARKER_LEVEL))
check("Shape Curve survives save and load",
      current.tracks[0].curveTrack.shapeCurve == ShapeCurve.Snappy,
      "got %s" % current.tracks[0].curveTrack.shapeCurve)

track.shapeCurve = ShapeCurve.Medium
os.remove(path)


# --- 9. the output is unipolar 0V..Range -----------------------------------
print("\n9. Range sets a unipolar 0V..max output")

VoltageRange = tsseq.Types.VoltageRange

def sample_range(voltage_range, offset=0):
    track.range = voltage_range
    track.offset = offset
    seq.segmentCount = 1
    s = seq.steps[0]
    s.length = 16
    s.shapePercent = 50
    s.skewPercent = 50
    s.levelPercent = 100
    s.offsetPercent = 0
    e.simulator.wait(2200)          # let the current segment finish
    samples = []
    for _ in range(220):
        e.simulator.wait(10)
        samples.append(e.simulator.targetState.dac.volts(0))
    return min(samples), max(samples)

c.press("play")
e.simulator.wait(200)

# before this change the same setup swung -5V..+5V
lo, hi = sample_range(VoltageRange.Unipolar5V)
check("Range 5V troughs at 0V", abs(lo) < 0.2, "got %.3fV" % lo)
check("Range 5V peaks at 5V", abs(hi - 5.0) < 0.2, "got %.3fV" % hi)

lo, hi = sample_range(VoltageRange.Unipolar1V)
check("Range 1V troughs at 0V", abs(lo) < 0.1, "got %.3fV" % lo)
check("Range 1V peaks at 1V", abs(hi - 1.0) < 0.1, "got %.3fV" % hi)

# bipolar output is still reachable through the track offset
lo, hi = sample_range(VoltageRange.Unipolar5V, offset=-250)
check("offset -2.50V still reaches negative", abs(lo + 2.5) < 0.2, "got %.3fV" % lo)
check("offset -2.50V peaks at +2.5V", abs(hi - 2.5) < 0.2, "got %.3fV" % hi)

track.offset = 0
c.press("play")
e.simulator.wait(100)

# the track is unipolar only: bipolar values are clamped away
track.range = VoltageRange.Bipolar5V
check("bipolar clamps to unipolar", track.range == VoltageRange.Unipolar5V,
      "got %s" % track.range)
track.range = VoltageRange.Bipolar1V
check("bipolar 1V clamps to unipolar 1V", track.range == VoltageRange.Unipolar1V,
      "got %s" % track.range)

# and the encoder stops at both ends
c.selectPage("track"); c.wait(60)
# Track page rows: Name, Mute Mode, Shape Curve, Range, ...
c.rotateEncoder(3); c.wait(60)          # select the Range row
c.pressEncoder(); c.wait(60)            # enter edit
track.range = VoltageRange.Unipolar1V
c.rotateEncoder(-3); c.wait(60)
check("Range stops at 1V", track.range == VoltageRange.Unipolar1V, "got %s" % track.range)
c.rotateEncoder(9); c.wait(60)
check("Range stops at 5V", track.range == VoltageRange.Unipolar5V, "got %s" % track.range)
c.pressEncoder(); c.wait(60)
c.selectPage("steps"); c.wait(60)


# --- 10. Range survives serialization -------------------------------------
print("\n10. Range round trips and older projects default to 5V")

path2 = os.path.join(tempfile.mkdtemp(), "range.pro")
track.range = VoltageRange.Unipolar2V
p.save(path2)
reloaded = tsseq.Project()
reloaded.load(path2)
check("Range survives save and load",
      reloaded.tracks[0].curveTrack.range == VoltageRange.Unipolar2V,
      "got %s" % reloaded.tracks[0].curveTrack.range)

for version in (49, 51):
    p.save(path2, version)
    old = tsseq.Project()
    old.load(path2)
    check("a Version%d project defaults to Range 5V" % version,
          old.tracks[0].curveTrack.range == VoltageRange.Unipolar5V,
          "got %s" % old.tracks[0].curveTrack.range)

track.range = VoltageRange.Unipolar5V
os.remove(path2)


# --- 11. running backwards plays the curves mirrored -----------------------
print("\n11. backwards run modes play each segment mirrored")

RunMode = tsseq.Types.RunMode

# A strongly asymmetric segment is the discriminator: SKEW 0 slams to full level
# and decays, so forward playback shows one instant *rise* per segment and no
# instant falls. Mirrored, that is exactly inverted.
def edges(mode, segments=4, ms=6000):
    seq.runMode = mode
    seq.segmentCount = segments
    for i in range(segments):
        st = seq.steps[i]
        st.length = 8
        st.shapePercent = 0
        st.skewPercent = 0
        st.levelPercent = 100
        st.offsetPercent = 0
    e.simulator.wait(1500)              # let the current segment finish
    v = []
    for _ in range(ms // 10):
        e.simulator.wait(10)
        v.append(e.simulator.targetState.dac.volts(0))
    span = max(v) - min(v)
    jumps = [b - a for a, b in zip(v, v[1:])]
    rises = sum(1 for x in jumps if x > 0.5 * span)
    falls = sum(1 for x in jumps if x < -0.5 * span)
    return rises, falls, span

track.range = VoltageRange.Unipolar5V
track.offset = 0
c.press("play")
e.simulator.wait(200)

rises, falls, span = edges(RunMode.Forward)
check("Forward plays every segment forward", rises > 0 and falls == 0,
      "%d instant rises, %d instant falls" % (rises, falls))

# every segment must be mirrored, including the one played straight after the
# wrap from the first segment back round to the last
rises, falls, span = edges(RunMode.Backward)
check("Backward mirrors every segment, wrap included", falls > 0 and rises == 0,
      "%d instant rises, %d instant falls" % (rises, falls))

# the two-directional modes mix both: forward on the way out, mirrored on the way back
for name, mode in (("Pendulum", RunMode.Pendulum), ("PingPong", RunMode.PingPong)):
    rises, falls, span = edges(mode)
    check("%s mirrors only the descending leg" % name, rises > 0 and falls > 0,
          "%d instant rises, %d instant falls" % (rises, falls))

# a stale direction must not leak across a run mode change
edges(RunMode.Backward)
rises, falls, span = edges(RunMode.Forward)
check("Backward then Forward leaves nothing mirrored", rises > 0 and falls == 0,
      "%d instant rises, %d instant falls" % (rises, falls))

# random jumps by arbitrary distances, so there is no direction of travel
rises, falls, span = edges(RunMode.Random)
check("Random never mirrors", falls == 0,
      "%d instant rises, %d instant falls" % (rises, falls))

# a symmetric curve mirrors to itself: this bounds the blast radius of the change
def samples_symmetric(mode, ms=4000):
    seq.runMode = mode
    seq.segmentCount = 4
    for i in range(4):
        st = seq.steps[i]
        st.length = 8
        st.shapePercent = 50
        st.skewPercent = 50
        st.levelPercent = 100
        st.offsetPercent = 0
    e.simulator.wait(1500)
    v = []
    for _ in range(ms // 10):
        e.simulator.wait(10)
        v.append(e.simulator.targetState.dac.volts(0))
    return v

fwd = samples_symmetric(RunMode.Forward)
bwd = samples_symmetric(RunMode.Backward)
check("a symmetric curve is unaffected by direction",
      abs(max(fwd) - max(bwd)) < 0.1 and abs(min(fwd) - min(bwd)) < 0.1,
      "forward %.2f..%.2fV vs backward %.2f..%.2fV"
      % (min(fwd), max(fwd), min(bwd), max(bwd)))

seq.runMode = RunMode.Forward
c.press("play")
e.simulator.wait(100)


# --- 12. Play Mode / Fill Mode are gone ------------------------------------
print("\n12. the dead curve track settings are removed")

for name in ("playMode", "fillMode"):
    check("CurveTrack no longer exposes %s" % name, not hasattr(track, name))
check("CurveTrack.FillMode enum is gone", not hasattr(tsseq.CurveTrack, "FillMode"))

path3 = os.path.join(tempfile.mkdtemp(), "v52.pro")
track.range = VoltageRange.Unipolar3V
p.save(path3, 52)
v52 = tsseq.Project()
v52.load(path3)
check("a Version52 project still loads", v52.tracks[0].curveTrack.range == VoltageRange.Unipolar3V,
      "got %s" % v52.tracks[0].curveTrack.range)
track.range = VoltageRange.Unipolar5V
os.remove(path3)


# ---------------------------------------------------------------------------

print("\n%s" % ("-" * 60))
if failures:
    print("%d FAILED: %s" % (len(failures), ", ".join(failures)))
    sys.exit(1)
print("all checks passed")
