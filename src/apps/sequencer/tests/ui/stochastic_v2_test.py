"""Stochastic V2 engine tests — validates probability-based note/gate generation."""
import sys
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')

from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

def setup_stochastic_track(p, track_index=0):
    p.setTrackMode(track_index, tsseq.Track.TrackMode.Stochastic)
    return p.tracks[track_index].stochasticTrack

def run_steps(e, c, n_steps, divisor=192):
    """Advance the simulator by n_steps steps."""
    for _ in range(n_steps):
        e.simulator.wait(divisor)

def test_uniform_degree_probs():
    """All degrees at equal probability → gate fires, CV within scale range."""
    e = Environment()
    c = Controller(e.simulator)
    c.wait(3000)

    p = e.sequencer.model.project
    track = setup_stochastic_track(p)
    seq = track.sequences[0]

    # All 7 degrees at max probability
    for i in range(7):
        seq.setDegreeProb(i, 15)
    # Centre octave only
    for i in range(5):
        seq.setOctaveProb(i, 15 if i == 2 else 0)
    # Quarter-note duration only
    for i in range(6):
        seq.setDurationProb(i, 15 if i == 2 else 0)

    c.press("play")
    gates_seen = 0
    for _ in range(32):
        e.simulator.wait(192)
        if e.simulator.targetState.gateOutput[0]:
            gates_seen += 1
            cv = e.simulator.targetState.dac.volts(0)
            # Semitone scale: degrees 0-6 map to 0..0.5V (notes 0-6 in a 12-TET scale @ C4)
            assert -2.0 <= cv <= 3.0, f"CV {cv:.3f}V out of expected range"

    # With all degrees at max, should gate very frequently
    assert gates_seen >= 20, f"Expected >= 20 gates in 32 steps, got {gates_seen}"
    print(f"✓ uniform_degree_probs: {gates_seen}/32 steps gated")

def test_all_degrees_zero_no_gate():
    """All degreeProb = 0 → automatic density = 0 → all rests."""
    e = Environment()
    c = Controller(e.simulator)
    c.wait(3000)

    p = e.sequencer.model.project
    track = setup_stochastic_track(p)
    seq = track.sequences[0]

    for i in range(7):
        seq.setDegreeProb(i, 0)

    c.press("play")
    gate_count = 0
    for _ in range(16):
        e.simulator.wait(192)
        if e.simulator.targetState.gateOutput[0]:
            gate_count += 1

    assert gate_count == 0, f"Expected 0 gates with all-zero degreeProb, got {gate_count}"
    print(f"✓ all_degrees_zero_no_gate: 0/16 gates (correct)")

def test_loop_mode_fires_gates():
    """With loop mode enabled and degreeProbs active, gates still fire."""
    e = Environment()
    c = Controller(e.simulator)
    c.wait(3000)

    p = e.sequencer.model.project
    track = setup_stochastic_track(p)
    seq = track.sequences[0]

    for i in range(7):
        seq.setDegreeProb(i, 15)
    for i in range(5):
        seq.setOctaveProb(i, 15 if i == 2 else 0)
    for i in range(6):
        seq.setDurationProb(i, 15 if i == 2 else 0)

    seq.loopChance = 0
    seq.sequenceLastStep = 15

    c.press("play")

    # Fill the buffer (16 steps)
    for _ in range(16):
        e.simulator.wait(192)

    # Enable loop mode
    seq.useLoop = True
    e.simulator.wait(10)

    # Run 16 more steps in loop mode
    gate_count = 0
    for _ in range(16):
        e.simulator.wait(192)
        if e.simulator.targetState.gateOutput[0]:
            gate_count += 1

    # All degrees at max prob → should gate on most steps
    assert gate_count >= 10, f"Expected ≥ 10 gates in 16 loop steps, got {gate_count}"
    print(f"✓ loop_mode_fires_gates: {gate_count}/16 steps gated in loop mode")

def test_loop_mutation():
    """loopChance=15 → output varies across loop repetitions."""
    e = Environment()
    c = Controller(e.simulator)
    c.wait(3000)

    p = e.sequencer.model.project
    track = setup_stochastic_track(p)
    seq = track.sequences[0]

    for i in range(7):
        seq.setDegreeProb(i, 8)
    for i in range(5):
        seq.setOctaveProb(i, 15 if i == 2 else 0)
    for i in range(6):
        seq.setDurationProb(i, 15 if i == 2 else 0)

    seq.loopChance = 15
    seq.sequenceLastStep = 15

    c.press("play")

    # Fill buffer (16 steps)
    for _ in range(16):
        e.simulator.wait(192)

    # Enable loop
    seq.useLoop = True
    e.simulator.wait(10)

    # Capture first loop repetition
    gate_rep1 = []
    for _ in range(16):
        e.simulator.wait(192)
        gate_rep1.append(bool(e.simulator.targetState.gateOutput[0]))

    # Play more repetitions and count differences
    diffs = 0
    for rep in range(4):
        gate_rep = []
        for _ in range(16):
            e.simulator.wait(192)
            gate_rep.append(bool(e.simulator.targetState.gateOutput[0]))
        if gate_rep != gate_rep1:
            diffs += 1

    # With loopChance=15 (max mutation), expect at least one different repetition
    assert diffs >= 1, f"Expected ≥ 1 different rep with loopChance=15, got 0 diffs"
    print(f"✓ loop_mutation: {diffs}/4 reps differed (expected ≥ 1)")

if __name__ == "__main__":
    print("Running Stochastic V2 engine tests...")
    test_all_degrees_zero_no_gate()
    test_uniform_degree_probs()
    test_loop_mode_fires_gates()
    test_loop_mutation()
    print("\nAll tests passed.")
