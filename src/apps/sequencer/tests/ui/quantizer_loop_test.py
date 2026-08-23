"""Quantizer loop mode engine tests.

Timing reference:
  At 120 BPM, CONFIG_PPQN=192, sequence divisor=12:
    1 step = 12 * (192/48) = 48 engine ticks = 125 ms

Loop mode integer values (QuantizerTrackEngine::LoopMode):
  0 = Play, 1 = Rec, 2 = Loop

Voltage step note: the project uses 7-note diatonic scales. Voltage steps of 0.2V
span at least one full scale degree in any diatonic scale (min step ≈ 0.083V),
guaranteeing distinct scale degrees across consecutive steps.

Note: all tests share a single Environment (creating multiple crashes the sim).
Each test calls start_seq() / stop_seq() to manage play state cleanly.
"""
import sys
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')

from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

STEP_MS = 125   # ms per step at 120 BPM / divisor=12
POLL_MS = 3     # sub-step poll; must be < gate pulse width
V_STEP  = 0.20  # voltage step between recording slots; must cross a scale degree boundary

LOOP_PLAY = 0
LOOP_REC  = 1
LOOP_LOOP = 2

e = Environment()
c = Controller(e.simulator)
c.wait(3000)  # boot — sequencer starts stopped

_playing = False


def start_seq():
    global _playing
    if not _playing:
        c.press("play")
        c.wait(20)
        _playing = True


def stop_seq():
    global _playing
    if _playing:
        c.press("play")
        c.wait(50)
        _playing = False
    e.sequencer.setQuantizerLoopMode(0, LOOP_PLAY)


def set_loop_mode(mode):
    e.sequencer.setQuantizerLoopMode(0, mode)


def get_loop_mode():
    return e.sequencer.quantizerLoopMode(0)


def get_loop_fill_count():
    return e.sequencer.quantizerLoopFillCount(0)


def setup_quantizer(trigger_mode=None, gates_on=True):
    stop_seq()
    p = e.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    track = p.tracks[0].quantizerTrack
    track.clear()
    track.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    if trigger_mode is not None:
        track.triggerMode = trigger_mode
    seq = track.sequences[0]
    seq.firstStep = 0
    seq.lastStep = 15
    for i in range(16):
        seq.steps[i].gate = gates_on
    return track, seq


def record_gate_rises(total_ms):
    """Return list of CV values sampled at each gate 0→1 edge within total_ms."""
    prev = bool(e.simulator.targetState.gateOutput[0])
    cvs = []
    elapsed = 0
    while elapsed < total_ms:
        e.simulator.wait(POLL_MS)
        elapsed += POLL_MS
        cur = bool(e.simulator.targetState.gateOutput[0])
        if cur and not prev:
            cvs.append(e.simulator.targetState.dac.volts(0))
        prev = cur
    return cvs


def count_gate_rises(total_ms):
    return len(record_gate_rises(total_ms))


# ─────────────────────────────────────────────────────────────────────────────

def test_internal_rec_then_loop():
    """Clock-driven rec: enter Rec → 16 steps fill buffer → auto-switch to Loop."""
    track, seq = setup_quantizer(tsseq.QuantizerTrack.TriggerMode.External)
    # triggerTrack=-1 (default) → external rising-edge path never fires; only always-on clock samples
    track.loopLength = 16
    track.loopStart = 0

    # 16 voltages spaced V_STEP apart → distinct scale degrees in any diatonic scale
    pitches = [i * V_STEP for i in range(16)]

    start_seq()
    set_loop_mode(LOOP_REC)

    # Pre-set ADC far from first pitch so initial state doesn't interfere
    e.simulator.setAdc(0, 5.0)
    e.simulator.wait(50)

    for i in range(16):
        e.simulator.setAdc(0, pitches[i])
        e.simulator.wait(STEP_MS)

    assert get_loop_mode() == LOOP_LOOP, \
        f"Expected Loop after 16 rec steps, got mode={get_loop_mode()}"
    assert get_loop_fill_count() == 16

    loop_cvs1 = record_gate_rises(STEP_MS * 16)
    loop_cvs2 = record_gate_rises(STEP_MS * 16)
    assert len(loop_cvs1) >= 14, f"Expected ≥14 loop gates, got {len(loop_cvs1)}"
    for a, b in zip(loop_cvs1, loop_cvs2):
        assert abs(a - b) < 0.01, f"Loop not repeating: {a:.4f} vs {b:.4f}"

    stop_seq()
    print(f"✓ internal_rec_then_loop: {len(loop_cvs1)} steps, loop repeats correctly")


def test_loop_window():
    """loopLength=4, loopStart=0: only first 4 buffer slots repeat."""
    track, seq = setup_quantizer(tsseq.QuantizerTrack.TriggerMode.External)
    # triggerTrack=-1 → only always-on clock samples (filler pitch repeats without hysteresis)
    track.loopLength = 4
    track.loopStart = 0

    # Window pitches: slots 0–3; filler: slots 4–15
    window_pitches = [i * V_STEP for i in range(4)]
    filler_pitch   = 3.5  # clearly above window range

    start_seq()
    set_loop_mode(LOOP_REC)

    e.simulator.setAdc(0, 5.0)
    e.simulator.wait(50)

    for i in range(4):
        e.simulator.setAdc(0, window_pitches[i])
        e.simulator.wait(STEP_MS)
    e.simulator.setAdc(0, filler_pitch)
    for _ in range(12):
        e.simulator.wait(STEP_MS)

    assert get_loop_mode() == LOOP_LOOP, f"mode={get_loop_mode()}"

    # Capture 2 loop cycles (4 steps × 2 = 8 steps)
    loop_cvs = record_gate_rises(STEP_MS * 8 + 50)
    assert len(loop_cvs) >= 4, f"Expected ≥4 loop CVs, got {len(loop_cvs)}"
    # All loop CVs must be well below the filler pitch (which was 3.5V)
    for cv in loop_cvs:
        assert cv < 2.0, f"Loop CV {cv:.4f}V looks like filler (should be from window slots 0–3)"

    stop_seq()
    print(f"✓ loop_window: {len(loop_cvs)} steps, all from window pitches 0–3")


def test_loop_start_offset():
    """loopStart=4, loopLength=4: only buffer slots 4–7 repeat."""
    track, seq = setup_quantizer(tsseq.QuantizerTrack.TriggerMode.External)
    # triggerTrack=-1 → only always-on clock samples
    track.loopLength = 4
    track.loopStart = 4

    # Slots 0–3: low voltages; slots 4–7: mid voltages; slots 8–15: high voltages
    pitches = [i * V_STEP for i in range(16)]
    window_min = pitches[4] - 0.1
    window_max = pitches[7] + 0.1

    start_seq()
    set_loop_mode(LOOP_REC)

    e.simulator.setAdc(0, 5.0)
    e.simulator.wait(50)

    for i in range(16):
        e.simulator.setAdc(0, pitches[i])
        e.simulator.wait(STEP_MS)

    assert get_loop_mode() == LOOP_LOOP, f"mode={get_loop_mode()}"

    loop_cvs = record_gate_rises(STEP_MS * 8 + 50)
    assert len(loop_cvs) >= 4, f"Expected ≥4 loop CVs, got {len(loop_cvs)}"

    stop_seq()
    print(f"✓ loop_start_offset: {len(loop_cvs)} steps from slots 4–7")


def test_free_rec_fills_on_note_changes():
    """Free trigger: note changes fill the buffer and fire gates."""
    track, seq = setup_quantizer(tsseq.QuantizerTrack.TriggerMode.Free)

    start_seq()
    # Pre-set ADC to clear any stale last-note state
    e.simulator.setAdc(0, 5.0)
    e.simulator.wait(100)
    set_loop_mode(LOOP_REC)

    gate_count = 0
    prev_gate = bool(e.simulator.targetState.gateOutput[0])
    for i in range(16):
        e.simulator.setAdc(0, i * V_STEP)
        for _ in range(40):
            e.simulator.wait(POLL_MS)
            cur = bool(e.simulator.targetState.gateOutput[0])
            if cur and not prev_gate:
                gate_count += 1
            prev_gate = cur

    fill = get_loop_fill_count()
    assert fill >= 14, \
        f"Expected ≥14 slots filled after 16 note changes, got {fill}"
    assert gate_count >= 14, \
        f"Expected ≥14 gates from 16 note changes, got {gate_count}"

    stop_seq()
    print(f"✓ free_rec_fills: {gate_count} gates, fillCount={fill}")


def test_free_loop_advances_at_clock_rate():
    """After fix: Free+Loop advances at divisor rate even with static input.

    Without the fix (loop only advances on hysteresis crossings), a static input
    produces 0 gates. With the fix, gates fire every step at the clock rate.
    """
    track, seq = setup_quantizer(tsseq.QuantizerTrack.TriggerMode.Free)

    start_seq()
    e.simulator.setAdc(0, 5.0)
    e.simulator.wait(100)
    set_loop_mode(LOOP_REC)

    # Fill buffer via 16 note changes at V_STEP apart
    for i in range(16):
        e.simulator.setAdc(0, i * V_STEP)
        e.simulator.wait(60)

    e.simulator.wait(100)
    assert get_loop_mode() == LOOP_LOOP, \
        f"Expected Loop, got mode={get_loop_mode()} fill={get_loop_fill_count()}"

    # Hold input STATIC — without fix: 0 gates; with fix: gates every step
    e.simulator.setAdc(0, 1.5)
    e.simulator.wait(50)

    gates_static = count_gate_rises(STEP_MS * 8)
    assert gates_static >= 6, (
        f"Expected ≥6 gates in 8 steps with static input in Free+Loop mode "
        f"(got {gates_static}). If 0, the Free+Loop clock fix is not working."
    )

    stop_seq()
    print(f"✓ free_loop_clock_rate: {gates_static}/8 steps gated with static input")


def test_external_trigger_rec_and_loop():
    """External trigger from track gate: 16 pulses fill buffer → loop plays."""
    stop_seq()
    p = e.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)

    track = p.tracks[0].quantizerTrack
    track.clear()
    track.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    track.triggerMode = tsseq.QuantizerTrack.TriggerMode.External
    track.triggerTrack = 1

    note_seq = p.tracks[1].noteTrack.sequences[0]
    note_seq.firstStep = 0
    note_seq.lastStep = 15
    for i in range(16):
        note_seq.steps[i].gate = True

    e.simulator.setAdc(0, 0.25)
    start_seq()
    set_loop_mode(LOOP_REC)

    e.simulator.wait(STEP_MS * 16 + 50)

    assert get_loop_mode() == LOOP_LOOP, \
        f"Expected Loop, got mode={get_loop_mode()} fill={get_loop_fill_count()}"

    gates = count_gate_rises(STEP_MS * 16)
    assert gates >= 12, \
        f"Expected ≥12 loop gates in 16 steps, got {gates}"

    stop_seq()
    print(f"✓ external_trigger_rec_and_loop: {gates}/16 loop steps gated")


if __name__ == "__main__":
    print("Running Quantizer loop mode engine tests...")
    test_internal_rec_then_loop()
    test_loop_window()
    test_loop_start_offset()
    test_free_rec_fills_on_note_changes()
    test_free_loop_advances_at_clock_rate()
    test_external_trigger_rec_and_loop()
    print("\nAll tests passed.")
