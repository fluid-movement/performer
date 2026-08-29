"""Note track V2 engine tests.

Covers the slot scheduler (step length ratios, gate length, retrigger, arp
retrigger, repeat, legato) and the reworked portamento.

Timing reference:
  wait(N) = N milliseconds of simulated time
  At 120 BPM, CONFIG_PPQN=192, sequence divisor=12:
    1 step = 12 * (192/48) = 48 engine ticks = 125 ms

Note: creating multiple Environment() instances in one Python process crashes the
simulator. All tests share a single environment, changing params between runs.
"""
import os
import sys
import tempfile
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')

from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

STEP_MS = 125    # ms per divisor at 120 BPM / divisor=12
POLL_MS = 2

# StepLength ratio table indices (NoteSequence::stepLengthRatio)
SL = {'1/8':0, '1/6':1, '1/4':2, '1/3':3, '1/2':4, '2/3':5, '3/4':6,
      '1':7, '5/4':8, '4/3':9, '3/2':10, '2':11, '5/2':12, '3':13,
      '4':14, '5':15, '6':16, '8':17, '12':18, '16':19}

# Scale index 0 is "Maj: Ionian" (Scale.cpp) -- 7 notes per octave at these
# semitone offsets. There is no chromatic scale in the table, so a step's note
# is a degree in this scale, not a semitone.
IONIAN = [0, 2, 4, 5, 7, 9, 11]


def degree_volts(d):
    """CV a note-degree index produces under the Ionian scale, root C."""
    oct_, idx = divmod(d, 7)
    return (IONIAN[idx] + 12 * oct_) / 12.0


FAILURES = []


def check(cond, msg):
    if cond:
        print("  ok   " + msg)
    else:
        print("  FAIL " + msg)
        FAILURES.append(msg)


def gate(e):
    return bool(e.simulator.targetState.gateOutput[0])


def sample_edges(e, total_ms, poll=POLL_MS):
    """Return (rises, falls, cvs_at_rise) over total_ms."""
    prev = gate(e)
    rises = falls = 0
    cvs = []
    elapsed = 0
    while elapsed < total_ms:
        d = min(poll, total_ms - elapsed)
        e.simulator.wait(d)
        elapsed += d
        curr = gate(e)
        if curr and not prev:
            rises += 1
            cvs.append(e.simulator.targetState.dac.volts(0))
        elif prev and not curr:
            falls += 1
        prev = curr
    return rises, falls, cvs


def gate_high_ms(e, total_ms, poll=1):
    """Total time the gate spends high over total_ms."""
    high = 0
    elapsed = 0
    while elapsed < total_ms:
        e.simulator.wait(poll)
        elapsed += poll
        if gate(e):
            high += poll
    return high


def reset_steps(seq, n=4):
    for i in range(16):
        s = seq.steps[i]
        s.clear()
    seq.firstStep = 0
    seq.lastStep = n - 1
    for i in range(n):
        s = seq.steps[i]
        s.gate = True
        s.note = 0
        s.stepLength = SL['1']
        s.gateLength = 50
        s.retrigger = 1
        s.arpRetrigger = 0
        s.repeat = 1


def restart(c, e):
    """Stop, then start again from the top so measurements begin at a step edge."""
    c.press("play")
    e.simulator.wait(60)
    c.press("play")
    e.simulator.wait(20)


# ---------------------------------------------------------------------------

def main():
    e = Environment()
    c = Controller(e.simulator)
    c.wait(3000)

    p = e.sequencer.model.project
    p.scale = 0          # Ionian (major) -- see IONIAN below
    p.rootNote = 0
    p.setTrackMode(0, tsseq.Track.TrackMode.Note)
    track = p.tracks[0].noteTrack
    seq = track.sequences[0]
    seq.divisor = 12
    reset_steps(seq)

    c.press("play")
    e.simulator.wait(300)

    # -- 1. step length ratios ------------------------------------------------
    print("\n[1] step length ratios")
    for name, expect_rises in (('1', 8), ('1/2', 16), ('2', 4), ('4', 2)):
        reset_steps(seq)
        for i in range(4):
            seq.steps[i].stepLength = SL[name]
        e.simulator.wait(8 * STEP_MS)          # settle
        rises, _, _ = sample_edges(e, 8 * STEP_MS)
        check(abs(rises - expect_rises) <= 1,
              "stepLength %-4s -> %d rises in 8 pulses (expected ~%d)" % (name, rises, expect_rises))

    # -- 2. gate length -------------------------------------------------------
    print("\n[2] gate length as a percentage of the slice")
    for pct, lo, hi in ((25, 0.15, 0.35), (50, 0.40, 0.60), (90, 0.80, 1.0)):
        reset_steps(seq)
        for i in range(4):
            seq.steps[i].gateLength = pct
        e.simulator.wait(4 * STEP_MS)
        window = 8 * STEP_MS
        high = gate_high_ms(e, window)
        frac = high / window
        check(lo <= frac <= hi,
              "gateLength %3d%% -> gate high %.0f%% of the time" % (pct, frac * 100))

    # -- 3. retrigger ---------------------------------------------------------
    print("\n[3] retrigger slices the step")
    reset_steps(seq)
    for i in range(4):
        seq.steps[i].retrigger = 4
        seq.steps[i].gateLength = 40
    e.simulator.wait(4 * STEP_MS)
    rises, _, _ = sample_edges(e, 4 * STEP_MS)
    check(abs(rises - 16) <= 1, "retrig 4 @ len 1 -> %d rises in 4 pulses (expected ~16)" % rises)

    reset_steps(seq)
    for i in range(4):
        seq.steps[i].stepLength = SL['2']
        seq.steps[i].retrigger = 3
        seq.steps[i].gateLength = 40
    e.simulator.wait(4 * STEP_MS)
    rises, _, _ = sample_edges(e, 12 * STEP_MS)
    check(abs(rises - 18) <= 1,
          "retrig 3 @ len 2 -> %d rises in 12 pulses (expected ~18, triplets)" % rises)

    # -- 4. arp retrigger -----------------------------------------------------
    print("\n[4] arp retrigger walks the scale per slice")
    reset_steps(seq, n=1)
    s = seq.steps[0]
    s.retrigger = 3
    s.arpRetrigger = 1
    s.gateLength = 40
    s.note = 0
    e.simulator.wait(2 * STEP_MS)
    _, _, cvs = sample_edges(e, 3 * STEP_MS, poll=1)
    want = sorted(round(degree_volts(d), 3) for d in (0, 1, 2))
    got = sorted(set(round(v, 3) for v in cvs))
    check(got == want,
          "arp +1 -> degrees {0,1,2} = %s V (got %s)" % (want, got))

    s.arpRetrigger = -1
    e.simulator.wait(2 * STEP_MS)
    _, _, cvs = sample_edges(e, 3 * STEP_MS, poll=1)
    want = sorted(round(degree_volts(d), 3) for d in (0, -1, -2))
    got = sorted(set(round(v, 3) for v in cvs))
    check(got == want,
          "arp -1 -> degrees {0,-1,-2} = %s V (got %s)" % (want, got))

    # -- 5. repeat extends the step ------------------------------------------
    print("\n[5] repeat extends the step (Metropolix stage repeats)")
    reset_steps(seq, n=1)
    s = seq.steps[0]
    s.repeat = 3
    s.retrigger = 2
    s.gateLength = 40
    e.simulator.wait(3 * STEP_MS)
    rises, _, _ = sample_edges(e, 6 * STEP_MS)
    check(abs(rises - 12) <= 1,
          "repeat 3 x retrig 2 -> %d rises in 6 pulses (expected ~12)" % rises)

    # -- 6. legato ------------------------------------------------------------
    print("\n[6] legato holds the gate across the boundary")
    reset_steps(seq)
    for i in range(4):
        seq.steps[i].gateLength = 100
    track.gateMode = tsseq.NoteTrack.GateMode.Retrigger
    e.simulator.wait(4 * STEP_MS)
    r_ret, f_ret, _ = sample_edges(e, 8 * STEP_MS, poll=1)
    check(f_ret >= 4, "Retrigger mode @ 100%% -> %d falling edges (gate re-fires)" % f_ret)

    track.gateMode = tsseq.NoteTrack.GateMode.Legato
    e.simulator.wait(4 * STEP_MS)
    r_leg, f_leg, _ = sample_edges(e, 8 * STEP_MS, poll=1)
    check(f_leg < f_ret and r_leg < r_ret,
          "Legato mode @ 100%% -> %d rises / %d falls (vs %d / %d retrigger)"
          % (r_leg, f_leg, r_ret, f_ret))
    track.gateMode = tsseq.NoteTrack.GateMode.Retrigger

    # -- 7. slide arrives -----------------------------------------------------
    print("\n[7] slide reaches the target (regression: old one-pole never did)")
    reset_steps(seq, n=2)
    seq.steps[0].note = 0
    seq.steps[1].note = 7      # one octave up in the Ionian scale
    seq.steps[1].slide = True
    seq.steps[0].gateLength = 90
    seq.steps[1].gateLength = 90
    track.slideTime = 12       # 60 ms
    target = degree_volts(7)
    shapes = {}

    for name, stype in (("Linear", tsseq.Types.SlideType.Linear),
                        ("Exponential", tsseq.Types.SlideType.Exponential),
                        ("Logarithmic", tsseq.Types.SlideType.Logarithmic)):
        track.slideType = stype
        e.simulator.wait(4 * STEP_MS)
        # find the slide onset: wait for the CV to leave the low note
        prev = e.simulator.targetState.dac.volts(0)
        trace = []
        for _ in range(int(2 * STEP_MS)):
            e.simulator.wait(1)
            trace.append(e.simulator.targetState.dac.volts(0))
        peak = max(trace)
        check(abs(peak - target) < 0.01,
              "%-12s reaches %.4f V (target %.4f)" % (name, peak, target))

        # and it must settle there, not just graze it
        settled = [v for v in trace if abs(v - target) < 0.005]
        check(len(settled) > 20,
              "%-12s holds the target for %d ms" % (name, len(settled)))
        shapes[name] = trace

    # the three curves must be genuinely different in shape, not just endpoints:
    # exponential is fast out of the gate, logarithmic is slow.
    def frac_at(trace, ms_in):
        """How far through the glide the curve is, ms_in after it starts."""
        lo = min(trace)
        start = next((i for i, v in enumerate(trace) if v > lo + 0.01), None)
        if start is None or start + ms_in >= len(trace):
            return None
        return (trace[start + ms_in] - lo) / (target - lo)

    f_lin = frac_at(shapes["Linear"], 20)
    f_exp = frac_at(shapes["Exponential"], 20)
    f_log = frac_at(shapes["Logarithmic"], 20)
    if None in (f_lin, f_exp, f_log):
        check(False, "curve shape sampling failed")
    else:
        check(f_exp > f_lin > f_log,
              "20 ms into a 60 ms glide: exp %.2f > linear %.2f > log %.2f"
              % (f_exp, f_lin, f_log))

    # -- 8. retrigger condition ----------------------------------------------
    print("\n[8] retrigger condition collapses the step to one slice")
    reset_steps(seq, n=1)
    s = seq.steps[0]
    s.retrigger = 4
    s.gateLength = 40
    s.retriggerCondition = tsseq.Types.Condition.Off
    e.simulator.wait(2 * STEP_MS)
    on_rises, _, _ = sample_edges(e, 4 * STEP_MS)

    s.retriggerCondition = tsseq.Types.Condition.First   # only the first iteration
    e.simulator.wait(4 * STEP_MS)                        # move past iteration 0
    off_rises, _, _ = sample_edges(e, 4 * STEP_MS)
    check(off_rises < on_rises,
          "retrig cond failing -> %d rises vs %d when Off (collapses to 1 slice)"
          % (off_rises, on_rises))

    # -- 9. queue overflow regression ----------------------------------------
    print("\n[9] retrig 8 x repeat 8 still produces gates (old engine went silent)")
    reset_steps(seq, n=1)
    s = seq.steps[0]
    s.retrigger = 8
    s.repeat = 8
    s.gateLength = 100
    s.stepLength = SL['1']
    e.simulator.wait(4 * STEP_MS)
    rises, _, _ = sample_edges(e, 8 * STEP_MS, poll=1)
    check(rises > 0, "retrig 8 x repeat 8 -> %d rises (must not be 0)" % rises)

    # -- 10. gate off is inert ------------------------------------------------
    print("\n[10] a gate-off step changes nothing")
    reset_steps(seq, n=2)
    seq.steps[0].note = 0
    seq.steps[0].gate = True
    seq.steps[1].note = 12
    seq.steps[1].gate = False        # must not sound and must not move the CV
    e.simulator.wait(4 * STEP_MS)
    cvs_seen = set()
    for _ in range(4 * STEP_MS // 2):
        e.simulator.wait(2)
        cvs_seen.add(round(e.simulator.targetState.dac.volts(0), 3))
    check(all(abs(v - 0.0) < 0.01 for v in cvs_seen),
          "gate-off step leaves CV at %s (must stay on the sounding note)"
          % sorted(cvs_seen))

    # -- 11. Version53 -> Version54 migration ---------------------------------
    print("\n[11] a pre-Version54 project migrates")
    reset_steps(seq, n=4)
    m = seq.steps[0]
    m.gate = True
    m.slide = True
    m.note = 5
    m.gateLength = 50          # legacy length 7 round-trips back to 50%
    m.retrigger = 4
    m.repeat = 3
    m.condition = tsseq.Types.Condition.Fill
    # these three have no pre-Version54 representation and must come back default
    m.stepLength = SL['4']
    m.arpRetrigger = 6
    m.retriggerCondition = tsseq.Types.Condition.First
    track.slideTime = 40

    path = os.path.join(tempfile.mkdtemp(), "v53.pro")
    p.save(path, 53)

    loaded = tsseq.Project()
    loaded.load(path)
    lt = loaded.tracks[0].noteTrack
    ms = lt.sequences[0].steps[0]

    check(ms.gate and ms.slide, "gate and slide survive")
    check(ms.note == 5, "note survives (got %d)" % ms.note)
    check(ms.gateLength == 50, "gate length round-trips through the 4-bit duty (got %d)" % ms.gateLength)
    check(ms.retrigger == 4, "retrigger survives (got %d)" % ms.retrigger)
    check(ms.repeat == 3, "stage repeats land on repeat (got %d)" % ms.repeat)
    check(ms.condition == tsseq.Types.Condition.Fill, "condition survives")
    check(ms.stepLength == SL['1'], "step length defaults to 1/1 (got index %d)" % ms.stepLength)
    check(ms.arpRetrigger == 0, "arp retrigger defaults to 0 (got %d)" % ms.arpRetrigger)
    check(ms.retriggerCondition == tsseq.Types.Condition.Off, "retrigger condition defaults to Off")
    check(lt.slideTime == 12, "slide time resets to the 60 ms default (got %d)" % lt.slideTime)
    check(lt.slideType == tsseq.Types.SlideType.Exponential, "slide type defaults to Exponential")
    check(lt.gateMode == tsseq.NoteTrack.GateMode.Retrigger, "gate mode defaults to Retrigger")

    c.press("play")

    print("\n" + "=" * 60)
    if FAILURES:
        print("FAILED (%d):" % len(FAILURES))
        for f in FAILURES:
            print("  - " + f)
        return 1
    print("ALL PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
