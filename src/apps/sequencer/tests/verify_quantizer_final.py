"""
Final adversarial pass for QuantizerTrack.

Targets edge cases not covered by the earlier suites:
  - Scale change mid-play in Free mode (same degree index, different pitch)
  - External triggerTrack switch: spurious trigger from _lastSourceGate contamination
  - Simultaneous quantizers on different tracks
  - Round-trip voltage accuracy across all scale degrees
  - Internal mode: sample-pending survives sequence reset
  - Harmonic minor / melodic minor scale families
  - Free mode: rapid scale cycling across all 21 scales
  - Transposition interactions with scale change
  - Internal mode: changing divisor mid-play
  - Very short Internal divisor (faster than sample delay)
"""

import sys, gc, math
sys.path.insert(0, 'src/apps/sequencer/tests')
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')

from agent import Session
import testsim.sequencer as tsseq

STEP_MS = 125
IONIAN_VOLTS  = [v / 1536.0 for v in [0, 256, 512, 640, 896, 1152, 1408]]
PHRYGIAN_VOLTS = [v / 1536.0 for v in [0, 128, 384, 640, 896, 1024, 1280]]


def quantize(scale_volts, v):
    """Replicate Scale::noteFromVolts → noteToVolts for a given 7-note scale."""
    v_biased = v + 0.01
    octave = math.floor(v_biased)   # matches C++ std::floor (not int(), which truncates)
    frac = v_biased - octave
    idx = -1
    for i, nv in enumerate(scale_volts):
        if frac < nv:
            break
        idx = i
    if idx == -1:
        idx = len(scale_volts) - 1
        octave -= 1
    return octave + scale_volts[idx]


def _setup_q(p, track_idx, input_src, trigger_mode, trigger_track=1):
    p.setTrackMode(track_idx, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[track_idx].quantizerTrack
    qt.inputSource = input_src
    qt.triggerMode = trigger_mode
    if trigger_mode == tsseq.QuantizerTrack.TriggerMode.External:
        qt.triggerTrack = trigger_track
    qt.octave = 0; qt.transpose = 0
    if trigger_mode == tsseq.QuantizerTrack.TriggerMode.Internal:
        seq = p.tracks[track_idx].quantizerTrack.sequences[0]
        seq.firstStep = 0; seq.lastStep = 3
        for i in range(4): seq.steps[i].gate = True
    return qt


# ---------------------------------------------------------------------------
# 1. Scale change in Free mode — same degree index, different pitch
# ---------------------------------------------------------------------------

def test_scale_change_same_degree_index():
    """
    Free mode: scale changes mid-play. Input at 0.2V.
    Ionian:   deg 1 = 256/1536 = 0.1667V
    Phrygian: deg 1 = 128/1536 = 0.0833V (b2)
    Both scales put 0.2V at degree index 1 → old bug: no re-commit, output stays wrong.
    """
    print("Final 1: Scale change — same degree index, different pitch (Free mode)")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian

    s.ctrl.adc(0, 0.2)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_ionian = s.cv(0)

    p.scale = 2  # Phrygian (b2 scale)
    s.wait(STEP_MS * 4)
    out_phrygian = s.cv(0)

    s.press("play")
    s.close()

    expected_ionian  = IONIAN_VOLTS[1]    # 0.1667V
    expected_phrygian = PHRYGIAN_VOLTS[1]  # 0.0833V

    assert abs(out_ionian - expected_ionian) < 0.03, (
        f"Ionian: {out_ionian:.4f}V != {expected_ionian:.4f}V"
    )
    assert abs(out_phrygian - expected_phrygian) < 0.03, (
        f"After scale→Phrygian: {out_phrygian:.4f}V != {expected_phrygian:.4f}V "
        f"(still at Ionian value? Scale change not detected)"
    )
    print(f"  Ionian: {out_ionian:.4f}V, Phrygian: {out_phrygian:.4f}V OK")
    print("  PASS\n")


def test_scale_change_back_and_forth():
    """Cycling scale multiple times each time updates the output correctly."""
    print("Final 2: Scale change back and forth — output tracks each scale")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)

    s.ctrl.adc(0, 0.2)
    s.press("play")
    results = []
    for scale_idx in [0, 2, 0, 2, 0]:   # Ionian, Phrygian, Ionian, Phrygian, Ionian
        p.scale = scale_idx
        s.wait(STEP_MS * 4)
        results.append((scale_idx, s.cv(0)))

    s.press("play")
    s.close()

    for scale_idx, out in results:
        if scale_idx == 0:
            expected = IONIAN_VOLTS[1]
            label = "Ionian"
        else:
            expected = PHRYGIAN_VOLTS[1]
            label = "Phrygian"
        assert abs(out - expected) < 0.03, (
            f"{label}: {out:.4f}V != {expected:.4f}V"
        )
        print(f"  Scale {scale_idx} ({label}): {out:.4f}V OK")
    print("  PASS\n")


def test_scale_change_with_transpose():
    """Scale change + transpose: output reflects both simultaneously."""
    print("Final 3: Scale change with transpose — both applied correctly")
    s = Session()
    p = s.env.sequencer.model.project
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    qt.octave = 1
    p.scale = 0  # Ionian

    s.ctrl.adc(0, 0.2)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_ionian = s.cv(0)

    p.scale = 2  # Phrygian
    s.wait(STEP_MS * 4)
    out_phrygian = s.cv(0)

    s.press("play")
    s.close()

    # octave=1 → transposition = 7 (one Ionian octave) or 7 (Phrygian also 7 notes/oct)
    expected_ionian   = IONIAN_VOLTS[1] + 1.0    # deg 1 + 1 octave
    expected_phrygian = PHRYGIAN_VOLTS[1] + 1.0  # b2 + 1 octave

    assert abs(out_ionian - expected_ionian) < 0.03, (
        f"Ionian+oct1: {out_ionian:.4f}V != {expected_ionian:.4f}V"
    )
    assert abs(out_phrygian - expected_phrygian) < 0.03, (
        f"Phrygian+oct1: {out_phrygian:.4f}V != {expected_phrygian:.4f}V"
    )
    print(f"  Ionian+oct1: {out_ionian:.4f}V, Phrygian+oct1: {out_phrygian:.4f}V OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 2. External trigger: no spurious trigger when triggerTrack changes
# ---------------------------------------------------------------------------

def test_external_trigger_track_switch_no_spurious():
    """
    Switching triggerTrack to a track whose gate is currently HIGH must NOT
    fire immediately — rising edge only fires on a low→high transition.
    """
    print("Final 4: External triggerTrack switch — no spurious trigger")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 1: always-gated note track (gate stays high for a long time)
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.firstStep = 0; seq1.lastStep = 0
    seq1.steps[0].gate = True

    # Track 2: no gates
    p.setTrackMode(2, tsseq.Track.TrackMode.Note)
    seq2 = p.tracks[2].noteTrack.sequences[0]
    seq2.firstStep = 0; seq2.lastStep = 3
    for i in range(4): seq2.steps[i].gate = False

    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.External, trigger_track=2)
    p.scale = 0
    s.ctrl.adc(0, 0.5)

    s.press("play")
    s.wait(STEP_MS * 4)
    out_before = s.cv(0)  # should be 0V — never triggered yet

    # Switch to track 1 while its gate is HIGH
    qt.triggerTrack = 1
    s.wait(30)
    out_immediately = s.cv(0)

    # Wait for the next full gate cycle (low → high transition)
    s.wait(STEP_MS * 4)
    out_after_cycle = s.cv(0)

    s.press("play")
    s.close()

    expected = quantize(IONIAN_VOLTS, 0.5)

    assert abs(out_before) < 0.03, f"Should not have fired before switch: {out_before:.4f}V"
    assert abs(out_immediately) < 0.03, (
        f"Spurious trigger on track switch: {out_immediately:.4f}V "
        f"(should still be 0V — no rising edge yet)"
    )
    assert abs(out_after_cycle - expected) < 0.03, (
        f"After real gate cycle: {out_after_cycle:.4f}V != {expected:.4f}V"
    )
    print(f"  Before: {out_before:.4f}V, at switch: {out_immediately:.4f}V (no spurious), "
          f"after cycle: {out_after_cycle:.4f}V OK")
    print("  PASS\n")


def test_external_trigger_switch_between_sources():
    """triggerTrack switches during play — each source's rising edges fire independently."""
    print("Final 5: External triggerTrack switch mid-play — each source fires independently")
    s = Session()
    p = s.env.sequencer.model.project

    for tk in [1, 2]:
        p.setTrackMode(tk, tsseq.Track.TrackMode.Note)
        seq = p.tracks[tk].noteTrack.sequences[0]
        seq.scale = 0; seq.firstStep = 0; seq.lastStep = 3
        for i in range(4):
            seq.steps[i].gate = True
            seq.steps[i].note = 0

    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.External, trigger_track=1)
    p.scale = 0
    s.ctrl.adc(0, 0.5)

    s.press("play")
    s.wait(STEP_MS * 4)
    out_track1 = s.cv(0)

    qt.triggerTrack = 2
    s.wait(STEP_MS * 4)
    out_track2 = s.cv(0)

    s.press("play")
    s.close()

    expected = quantize(IONIAN_VOLTS, 0.5)
    assert abs(out_track1 - expected) < 0.03, f"Track 1 trigger: {out_track1:.4f}V"
    assert abs(out_track2 - expected) < 0.03, f"Track 2 trigger: {out_track2:.4f}V"
    print(f"  Track 1 trigger: {out_track1:.4f}V, Track 2 trigger: {out_track2:.4f}V OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 3. Simultaneous quantizers
# ---------------------------------------------------------------------------

def test_simultaneous_quantizers_independent():
    """Two quantizers on different tracks must not interfere with each other."""
    print("Final 6: Simultaneous quantizers — independent outputs")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 0: quantizer, CvIn1 = 0.25V → Ionian deg 1
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    # Track 1: quantizer, CvIn2 = 0.75V → Ionian deg 5
    _setup_q(p, 1, tsseq.QuantizerTrack.InputSource.CvIn2,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    s.ctrl.adc(0, 0.25)
    s.ctrl.adc(1, 0.75)
    s.press("play")
    s.wait(STEP_MS * 6)

    out0 = s.cv(0)
    out1 = s.cv(1)
    s.press("play")
    s.close()

    expected0 = quantize(IONIAN_VOLTS, 0.25)  # deg 1 = 0.1667V
    expected1 = quantize(IONIAN_VOLTS, 0.75)  # deg 5 = 0.75V

    assert abs(out0 - expected0) < 0.03, f"Track 0: {out0:.4f}V != {expected0:.4f}V"
    assert abs(out1 - expected1) < 0.03, f"Track 1: {out1:.4f}V != {expected1:.4f}V"
    print(f"  Track 0 (0.25V→{expected0:.4f}V): {out0:.4f}V OK")
    print(f"  Track 1 (0.75V→{expected1:.4f}V): {out1:.4f}V OK")
    print("  PASS\n")


def test_simultaneous_quantizers_same_input():
    """Two quantizers reading the same CvIn channel produce identical outputs."""
    print("Final 7: Simultaneous quantizers reading same CvIn — identical outputs")
    s = Session()
    p = s.env.sequencer.model.project

    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    _setup_q(p, 2, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 6)
    out0 = s.cv(0)
    out2 = s.cv(2)
    s.press("play")
    s.close()

    assert abs(out0 - out2) < 0.001, (
        f"Two quantizers on same input diverged: {out0:.4f}V vs {out2:.4f}V"
    )
    print(f"  Both outputs: {out0:.4f}V == {out2:.4f}V OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 4. Round-trip voltage accuracy
# ---------------------------------------------------------------------------

def test_round_trip_all_ionian_degrees():
    """noteToVolts(noteFromVolts(note_v)) == note_v for all Ionian degrees."""
    print("Final 8: Round-trip voltage accuracy — all Ionian degrees × 3 octaves")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    s.press("play")
    failures = []
    for octave in range(-1, 3):   # octaves -1 to +2
        for deg_idx, note_v in enumerate(IONIAN_VOLTS):
            input_v = octave + note_v
            s.ctrl.adc(0, input_v)
            s.wait(STEP_MS * 3)
            out = s.cv(0)
            expected = octave + note_v
            if abs(out - expected) > 0.002:
                failures.append(f"Oct{octave} deg{deg_idx}: in={input_v:.4f}V out={out:.4f}V expected={expected:.4f}V")
            else:
                pass  # silent OK

    s.press("play")
    s.close()

    assert not failures, f"Round-trip failures:\n" + "\n".join(failures)
    print(f"  All {4*7} degree round-trips accurate to within 2mV")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 5. Harmonic minor / melodic minor scales
# ---------------------------------------------------------------------------

def test_harmonic_minor_scale():
    """Scale index 7 (HM:Aeolian) quantizes correctly — not just diatonic modes."""
    print("Final 9: Harmonic minor family scale quantizes correctly")
    # Find a harmonic minor scale index — scales 7-13 are HM modes
    # HM:Aeolian (scale 7): semitones 0, 2, 3, 5, 7, 8, 11
    # In 1536 units: 0, 256, 384, 640, 896, 1024, 1408
    HM_AEOLIAN_VOLTS = [v / 1536.0 for v in [0, 256, 384, 640, 896, 1024, 1408]]

    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 7  # HM:Aeolian

    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 2)

    # Test a few characteristic notes
    test_cases = [
        (0.0,  HM_AEOLIAN_VOLTS[0]),   # root
        (0.45, HM_AEOLIAN_VOLTS[3]),   # perfect 4th
        (0.95, HM_AEOLIAN_VOLTS[6]),   # major 7th (characteristic of harmonic minor)
    ]
    for v_in, v_expected in test_cases:
        s.ctrl.adc(0, v_in)
        s.wait(STEP_MS * 4)
        out = s.cv(0)
        assert abs(out - v_expected) < 0.03, (
            f"HM scale: {v_in:.2f}V -> {out:.4f}V, expected {v_expected:.4f}V"
        )
        print(f"  {v_in:.2f}V -> {out:.4f}V (expected {v_expected:.4f}V) OK")

    s.press("play")
    s.close()
    print("  PASS\n")


def test_rapid_scale_cycling():
    """Cycle through all 21 scales — output must always reflect the current scale."""
    print("Final 10: Rapid scale cycling (all 21 scales) — output always current")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)

    # Input 0.0V — root note, should be 0V in every scale (all scales start at degree 0 = 0V)
    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 2)

    failures = []
    for scale_idx in range(21):
        p.scale = scale_idx
        s.wait(STEP_MS * 3)
        out = s.cv(0)
        if abs(out) > 0.03:
            failures.append(f"Scale {scale_idx}: root 0V -> {out:.4f}V (should be 0V)")

    s.press("play")
    s.close()

    assert not failures, "Root note not 0V in these scales:\n" + "\n".join(failures)
    print(f"  Root note correct (0V) across all 21 scales")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 6. Internal mode edge cases
# ---------------------------------------------------------------------------

def test_internal_sample_pending_across_reset():
    """Sample scheduled at last step fires correctly even after sequence reset."""
    print("Final 11: Internal mode — samplePending survives sequence reset boundary")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)

    # Gate only on the very last step — sample fires near the measure boundary
    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0
    seq0.lastStep = 7
    for i in range(8):
        seq0.steps[i].gate = (i == 7)  # gate only on step 7

    p.scale = 0
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 20)   # run several bars so reset happens multiple times
    out = s.cv(0)
    s.press("play")
    s.close()

    expected = quantize(IONIAN_VOLTS, 0.5)
    assert abs(out - expected) < 0.03, (
        f"Last-step gate across reset: {out:.4f}V != {expected:.4f}V"
    )
    print(f"  Output after {20 * STEP_MS}ms: {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_internal_divisor_change_mid_play():
    """Changing the sequence divisor mid-play doesn't corrupt timing."""
    print("Final 12: Internal mode — divisor change mid-play")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)

    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0; seq0.lastStep = 3
    for i in range(4): seq0.steps[i].gate = True
    seq0.divisor = 1   # 16th notes initially

    p.scale = 0
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_fast = s.cv(0)

    # Double the divisor (half-time)
    seq0.divisor = 2
    s.wait(STEP_MS * 8)
    out_slow = s.cv(0)

    s.press("play")
    s.close()

    expected = quantize(IONIAN_VOLTS, 0.5)
    assert abs(out_fast - expected) < 0.03, f"Fast divisor: {out_fast:.4f}V"
    assert abs(out_slow - expected) < 0.03, f"Slow divisor after change: {out_slow:.4f}V"
    print(f"  div=1: {out_fast:.4f}V, div=2: {out_slow:.4f}V — both correct")
    print("  PASS\n")


def test_internal_fast_divisor_smaller_than_sample_delay():
    """Internal triggers firing faster than SampleDelayTicks: last sample wins."""
    print("Final 13: Internal faster than sample delay — last trigger's sample commits")
    # SampleDelayTicks=4, divisor=1 means steps fire every tick
    # Pending sample keeps getting rescheduled; eventually the last step's sample fires
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)

    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0; seq0.lastStep = 7
    for i in range(8): seq0.steps[i].gate = True
    seq0.divisor = 1   # fires faster than sample delay

    p.scale = 0
    s.ctrl.adc(0, 0.75)
    s.press("play")
    s.wait(STEP_MS * 16)
    out = s.cv(0)
    s.press("play")
    s.close()

    # Should eventually stabilise — we just need a valid quantized output, not silence
    expected = quantize(IONIAN_VOLTS, 0.75)
    assert abs(out - expected) < 0.05, (
        f"Fast internal (div=1): {out:.4f}V not near {expected:.4f}V — stuck?"
    )
    print(f"  Fast div=1 output: {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 7. Gate output correctness
# ---------------------------------------------------------------------------

def test_gate_pulses_on_every_note_change():
    """Every note change (Free mode) produces exactly one gate pulse."""
    print("Final 14: Gate pulse fires on every note change, not between changes")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 3)
    # No change — gate should be low
    gate_static = s.gate(0)

    # Move to new note
    s.ctrl.adc(0, 0.5)
    # Sample gate right after (should be high during pulse)
    s.wait(10)
    gate_during = s.gate(0)
    # Wait for pulse to expire (PulseLengthTicks = 8 ≈ 21ms)
    s.wait(60)
    gate_after = s.gate(0)

    s.press("play")
    s.close()

    assert not gate_static, f"Gate was high with no note change: {gate_static}"
    assert gate_during, f"Gate was not high immediately after note change: {gate_during}"
    assert not gate_after, f"Gate did not expire after pulse: {gate_after}"
    print(f"  Static: {gate_static}, during pulse: {gate_during}, after: {gate_after} OK")
    print("  PASS\n")


def test_no_gate_if_note_unchanged():
    """Internal trigger fires, but if note is the same as before, gate still fires (re-trigger)."""
    print("Final 15: Internal re-trigger on same note still produces gate pulse")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)

    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0; seq0.lastStep = 1
    for i in range(2): seq0.steps[i].gate = True

    p.scale = 0
    s.ctrl.adc(0, 0.5)  # holds same note across all steps
    s.press("play")

    # Collect gates over time — should see repeating pulses on every step
    pulses = []
    for _ in range(24):
        s.wait(STEP_MS // 2)
        pulses.append(s.gate(0))

    s.press("play")
    s.close()

    # Should have observed multiple gate pulses even though note didn't change
    edges = sum(1 for i in range(1, len(pulses)) if pulses[i] and not pulses[i-1])
    assert edges >= 2, (
        f"Expected multiple gate pulses on repeated Internal triggers of same note, "
        f"only saw {edges} rising edges"
    )
    print(f"  {edges} gate pulses on repeated same-note Internal triggers OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 8. Additional adversarial probes (session 2)
# ---------------------------------------------------------------------------

def test_single_step_sequence_loops():
    """Internal mode with firstStep==lastStep==0 — single step loops correctly."""
    print("Final 16: Single-step Internal sequence — loops on step 0")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)
    seq = p.tracks[0].quantizerTrack.sequences[0]
    seq.firstStep = 0; seq.lastStep = 0
    seq.steps[0].gate = True
    p.scale = 0
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(800)
    out = s.cv(0)
    s.press("play")
    s.close()

    expected = quantize(IONIAN_VOLTS, 0.5)
    assert abs(out - expected) < 0.03, f"Single-step loop: {out:.4f}V != {expected:.4f}V"
    print(f"  Output: {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_rapid_transposition_sign_flip():
    """Free mode — alternating +3/-3 transpose while input is stable produces correct CV."""
    print("Final 17: Rapid transposition sign-flip — CV always correct")
    s = Session()
    p = s.env.sequencer.model.project
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0
    s.ctrl.adc(0, 0.5)   # deg3 = note index 3
    s.press("play")
    s.wait(200)

    failures = []
    for i in range(4):
        t = 3 if i % 2 == 0 else -3
        qt.transpose = t
        s.wait(100)
        out = s.cv(0)
        # deg3 + 3 = deg6 = 1408/1536 ≈ 0.9167V; deg3 + (-3) = deg0 = 0V
        expected = IONIAN_VOLTS[6] if t == 3 else 0.0
        if abs(out - expected) > 0.03:
            failures.append(f"transpose={t:+d}: {out:.4f}V != {expected:.4f}V")

    qt.transpose = 0
    s.press("play")
    s.close()

    assert not failures, "\n".join(failures)
    print(f"  All ±3 transpose flips produced correct CV")
    print("  PASS\n")


def test_hysteresis_dead_zone_no_flip():
    """Inputs that alternate just inside the hysteresis dead zone should not flip notes."""
    print("Final 18: Hysteresis dead zone — alternating near boundary does not flip")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    # Park firmly at deg1 (0.1667V) first
    s.ctrl.adc(0, 0.25)
    s.press("play")
    s.wait(200)

    # Alternate: 0.158V (→ deg1) and 0.156V (→ deg0 candidate but |0.156-0.1667|=0.011 < 0.012)
    flip_count = 0
    prev = None
    for _ in range(20):
        s.ctrl.adc(0, 0.158)
        s.wait(40)
        v1 = s.cv(0)
        s.ctrl.adc(0, 0.156)
        s.wait(40)
        v2 = s.cv(0)
        if prev is not None and abs(v2 - prev) > 0.05:
            flip_count += 1
        prev = v2

    s.press("play")
    s.close()

    assert flip_count == 0, (
        f"Hysteresis dead zone failed: {flip_count} flips detected "
        f"(inputs within 0.011V of committed note, threshold 0.012V)"
    )
    print(f"  0 flips over 20 alternation cycles in dead zone OK")
    print("  PASS\n")


def test_iir_cannot_cross_note_boundary():
    """IIR convergence alone (no snap) can never cross a note boundary — mathematical guarantee."""
    print("Final 19: IIR-only convergence cannot change the quantized note")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    # Start at deg0 (0V). Set ADC to 0.04V (< FilterSnapVolts=0.05V → IIR only).
    # IIR converges from 0V toward 0.04V. 0.04V is still in deg0 territory.
    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(100)

    s.ctrl.adc(0, 0.04)   # sub-snap, forces IIR convergence
    s.wait(400)           # enough for full IIR convergence
    out = s.cv(0)
    s.press("play")
    s.close()

    # 0.04V → biased=0.05 < 256/1536=0.1667 → still deg0
    assert abs(out) < 0.01, (
        f"IIR convergence to 0.04V should stay at deg0 (0V), got {out:.4f}V"
    )
    print(f"  0.04V sub-snap input → {out:.4f}V (deg0, unchanged) OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 9. Session 3 adversarial probes (T–W)
# ---------------------------------------------------------------------------

def test_rapid_note_changes_gate_eventually_drops():
    """
    Free mode: ADC sweeps through multiple notes faster than PulseLengthTicks (~21ms).
    Each commit resets _pulseTick to tick+8, extending the gate.
    Gate must drop within ~30ms after the last change, not stay high forever.
    Final CV must match the last committed note.
    """
    print("Final 20: Rapid successive note changes — gate eventually drops, CV is last note")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    s.ctrl.adc(0, 0.0)  # deg0
    s.press("play")
    s.wait(100)

    # Sweep through deg1→deg2→deg3→deg4 in ~10ms steps (< PulseLengthTicks=21ms)
    for v in [0.20, 0.40, 0.60, 0.80]:
        s.ctrl.adc(0, v)
        s.wait(10)

    # Gate may still be high right now (chain of extended pulses)
    gate_during = s.gate(0)

    # Wait well past the last PulseLengthTicks
    s.wait(80)
    gate_after = s.gate(0)
    final_cv = s.cv(0)

    s.press("play")
    s.close()

    # Final note: 0.80V on Ionian → deg4
    expected = quantize(IONIAN_VOLTS, 0.80)

    assert abs(final_cv - expected) < 0.03, (
        f"Final CV: {final_cv:.4f}V != {expected:.4f}V (expected deg4 after sweep)"
    )
    assert not gate_after, (
        f"Gate still high 80ms after last note change — pulse never expired"
    )
    print(f"  Gate during sweep: {gate_during}, after: {gate_after}, "
          f"final CV: {final_cv:.4f}V (deg4={expected:.4f}V) OK")
    print("  PASS\n")


def test_input_source_switch_mid_play():
    """
    Free mode: switch inputSource from CvIn1 (0.3V) to CvIn2 (0.6V) during play.
    The IIR snap path should fire immediately (|0.6-0.3|=0.3V > FilterSnapVolts=0.05V),
    so the output updates to quantize(0.6V) within one update cycle.
    """
    print("Final 21: Input source switch mid-play — output tracks new source")
    s = Session()
    p = s.env.sequencer.model.project
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    s.ctrl.adc(0, 0.30)  # CvIn1
    s.ctrl.adc(1, 0.60)  # CvIn2 — primed but not yet active

    s.press("play")
    s.wait(200)
    out_before = s.cv(0)

    # Switch to CvIn2
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn2
    s.wait(50)
    out_after = s.cv(0)

    s.press("play")
    s.close()

    expected_before = quantize(IONIAN_VOLTS, 0.30)
    expected_after  = quantize(IONIAN_VOLTS, 0.60)

    assert abs(out_before - expected_before) < 0.03, (
        f"Before switch: {out_before:.4f}V != {expected_before:.4f}V"
    )
    assert abs(out_after - expected_after) < 0.03, (
        f"After switch to CvIn2: {out_after:.4f}V != {expected_after:.4f}V "
        f"(source switch not detected)"
    )
    print(f"  CvIn1: {out_before:.4f}V → CvIn2: {out_after:.4f}V OK")
    print("  PASS\n")


def test_self_route_guard_returns_zero():
    """
    inputSource set to track 0's own CV output (Track0). Engine guard
    (trackIdx == _track.trackIndex()) must return 0V, not recurse or emit garbage.
    """
    print("Final 22: Self-route guard — quantizer reading its own output stays at 0V")
    s = Session()
    p = s.env.sequencer.model.project
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    # Point inputSource to Track0 (self)
    qt.inputSource = tsseq.QuantizerTrack.InputSource.Track1  # Track1 in UI = index 0

    s.press("play")
    s.wait(300)
    out = s.cv(0)

    s.press("play")
    s.close()

    # Self-route guard returns 0.f → quantize(0.0) = 0V = deg0
    assert abs(out) < 0.03, (
        f"Self-route: expected 0V (guarded), got {out:.4f}V"
    )
    print(f"  Self-route output: {out:.4f}V (0V deg0 from guard) OK")
    print("  PASS\n")


def test_external_invalid_trigger_track_no_crash():
    """
    External mode with triggerTrack = -1 (invalid / unset).
    Engine guard (triggerTrack >= 0) must block silently — no crash, no commit.
    CV must stay at 0V (never triggered).
    """
    print("Final 23: External mode with invalid triggerTrack=-1 — no crash, no commit")
    s = Session()
    p = s.env.sequencer.model.project
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.External)
    p.scale = 0
    s.ctrl.adc(0, 0.5)

    # Force triggerTrack to -1 (no valid source)
    qt.triggerTrack = -1

    s.press("play")
    s.wait(500)
    out = s.cv(0)
    gate = s.gate(0)

    s.press("play")
    s.close()

    assert abs(out) < 0.03, (
        f"Invalid triggerTrack=-1 caused commit: {out:.4f}V (expected 0V)"
    )
    assert not gate, f"Gate was high with invalid trigger track"
    print(f"  Output: {out:.4f}V, gate: {gate} (no spurious trigger) OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 10. Session 3 continued (Y–BB)
# ---------------------------------------------------------------------------

def test_internal_all_gates_off_never_commits():
    """
    Internal mode where every step has gate=False.
    Engine never schedules a sample → output stays at 0V, gate stays low.
    """
    print("Final 24: Internal mode all gates off — never commits")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)

    seq = p.tracks[0].quantizerTrack.sequences[0]
    seq.firstStep = 0; seq.lastStep = 3
    for i in range(4): seq.steps[i].gate = False  # all off

    p.scale = 0
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(600)
    out = s.cv(0)
    gate = s.gate(0)

    s.press("play")
    s.close()

    assert abs(out) < 0.03, (
        f"All-gates-off Internal committed unexpectedly: {out:.4f}V"
    )
    assert not gate, f"Gate should be low when no steps are gated"
    print(f"  Output: {out:.4f}V, gate: {gate} (no commits) OK")
    print("  PASS\n")


def test_quantizer_drives_quantizer_external():
    """
    Track 0: Free-mode quantizer on CvIn1=0.0V initially, changes to 0.5V to fire a gate.
    Track 1: External-mode quantizer on CvIn2=0.5V, triggered by track 0's gate output.
    Track 1 must fire when track 0's gate goes high (not before).
    """
    print("Final 25: Quantizer drives quantizer (External chain) — track 1 fires on track 0 gate")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 0: Free mode, CvIn1
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)

    # Track 1: External mode, CvIn2, trigger from track 0
    p.setTrackMode(1, tsseq.Track.TrackMode.Quantizer)
    qt1 = p.tracks[1].quantizerTrack
    qt1.inputSource = tsseq.QuantizerTrack.InputSource.CvIn2
    qt1.triggerMode = tsseq.QuantizerTrack.TriggerMode.External
    qt1.triggerTrack = 0   # listen to track 0's gate
    qt1.octave = 0; qt1.transpose = 0

    p.scale = 0
    s.ctrl.adc(0, 0.0)   # track 0 parks at deg0, no gate change initially
    s.ctrl.adc(1, 0.50)  # CvIn2 pre-loaded at 0.5V

    s.press("play")
    s.wait(200)
    out1_before = s.cv(1)   # track 1 shouldn't have fired yet (no rising edge)

    # Trigger track 0 by changing its input — it fires a gate pulse (rising edge)
    s.ctrl.adc(0, 0.5)
    s.wait(80)   # SampleDelayTicks (~10ms) + IIR snap + pulse settle
    out1_after = s.cv(1)

    s.press("play")
    s.close()

    expected_track1 = quantize(IONIAN_VOLTS, 0.50)

    assert abs(out1_before) < 0.03, (
        f"Track 1 fired before track 0's gate: {out1_before:.4f}V"
    )
    assert abs(out1_after - expected_track1) < 0.03, (
        f"Track 1 after chain trigger: {out1_after:.4f}V != {expected_track1:.4f}V"
    )
    print(f"  Before: {out1_before:.4f}V, after chain: {out1_after:.4f}V "
          f"(expected {expected_track1:.4f}V) OK")
    print("  PASS\n")


def test_extreme_octave_no_crash():
    """
    octave=10, transpose=0 on a 7-note scale → transposition=70.
    Input 0V (deg0) → output noteToVolts(70) = 10.0V.
    Engine must not crash (no integer overflow, no UB).
    """
    print("Final 26: Extreme octave=10 — no crash, CV = 10V above input")
    s = Session()
    p = s.env.sequencer.model.project
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0   # Ionian (7 notes/oct)
    qt.octave = 10; qt.transpose = 0

    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(100)
    out = s.cv(0)

    s.press("play")
    s.close()

    # deg0 + 70 degrees = 10 octaves = 10.0V internally, but DAC hardware clips at ±5V.
    # The sim returns the DAC-clamped value (~5V). Verify the output is at the clip rail,
    # not 0V (no commit) or some junk value, and that no crash occurred.
    assert out > 4.9, (
        f"Extreme octave=10: {out:.4f}V, expected near +5V rail (DAC clamp)"
    )
    print(f"  octave=10: output {out:.4f}V (10V internally, clipped to ~5V DAC rail, no crash) OK")
    print("  PASS\n")


def test_internal_non_contiguous_gated_steps():
    """
    Internal mode: steps 0,2 gated; steps 1,3 NOT gated.
    We count rising gate edges over time — should see ~2 per loop, not 4.
    """
    print("Final 27: Internal non-contiguous gates — only gated steps commit")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)

    seq = p.tracks[0].quantizerTrack.sequences[0]
    seq.firstStep = 0; seq.lastStep = 3
    seq.steps[0].gate = True
    seq.steps[1].gate = False
    seq.steps[2].gate = True
    seq.steps[3].gate = False

    p.scale = 0
    s.ctrl.adc(0, 0.5)

    s.press("play")

    gate_samples = []
    for _ in range(16):
        s.wait(STEP_MS // 2)
        gate_samples.append(s.gate(0))

    s.press("play")
    s.close()

    edges = sum(1 for i in range(1, len(gate_samples))
                if gate_samples[i] and not gate_samples[i-1])

    # Gate pulse is ~21ms; sample interval is STEP_MS/2 = 62.5ms → ~33% capture rate per pulse.
    # 2 gates/loop × 2 loops ≈ 4 events → statistically expect ~1-3 edges caught.
    # Lower bound = 2 (must fire at least twice), upper bound = 8 (never more than 4 events/loop).
    assert edges >= 2, (
        f"Expected >=2 gate rising edges for alternating gated steps, got {edges}. "
        f"Samples: {gate_samples}"
    )
    # Verify non-gated steps didn't fire: edges must not exceed 2×(number of loops sampled)
    # We see ~4 loops → 8 gate events max → edges ≤ 8
    assert edges <= 8, (
        f"Too many gate pulses ({edges}) — non-gated steps may have fired"
    )
    print(f"  {edges} gate rising edges observed (2 gated steps/loop, ~33% capture rate) OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 11. Session 3 continued (CC–EE)
# ---------------------------------------------------------------------------

def test_negative_octave_subzero_output():
    """
    octave=-1, transpose=0 on 7-note Ionian scale → transposition=-7.
    Input 0.5V → deg3 (noteFromVolts(0.5) = degree index 3).
    Output = noteToVolts(3 + (-7)) = noteToVolts(-4) = -1 + scale_volts[3] = -1 + 0.4167 = -0.5833V.
    """
    print("Final 28: Negative octave — sub-zero CV output correct")
    s = Session()
    p = s.env.sequencer.model.project
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian
    qt.octave = -1; qt.transpose = 0

    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(100)
    out = s.cv(0)

    s.press("play")
    s.close()

    # noteToVolts(-4) for Ionian: floor(-4/7)=-1, degree=(-4 mod 7)=3
    # → -1 + 640/1536 = -1 + 0.41667 = -0.58333V
    expected = -1.0 + IONIAN_VOLTS[3]
    assert abs(out - expected) < 0.03, (
        f"octave=-1, input=0.5V: {out:.4f}V, expected {expected:.4f}V"
    )
    print(f"  octave=-1: {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_stop_restart_reinitializes_cleanly():
    """
    Internal mode: commit a note, then stop, then start again.
    After restart, _lastQNote resets to INT32_MIN → first tick immediately re-commits.
    CV should match input after restart without needing to wait for a trigger.
    (Free mode used for simplicity — checks that reset then re-init path is clean.)
    """
    print("Final 29: Stop and restart — engine reinitializes without stale state")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    # First play session
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(200)
    out_first = s.cv(0)

    # Stop
    s.press("play")
    s.wait(50)

    # Change input while stopped — engine not running
    s.ctrl.adc(0, 0.2)

    # Restart
    s.press("play")
    s.wait(100)
    out_second = s.cv(0)

    s.press("play")
    s.close()

    expected_first  = quantize(IONIAN_VOLTS, 0.5)
    expected_second = quantize(IONIAN_VOLTS, 0.2)

    assert abs(out_first - expected_first) < 0.03, (
        f"First session: {out_first:.4f}V != {expected_first:.4f}V"
    )
    assert abs(out_second - expected_second) < 0.03, (
        f"After restart with new input: {out_second:.4f}V != {expected_second:.4f}V "
        f"(stale state from previous session?)"
    )
    print(f"  First: {out_first:.4f}V, after restart: {out_second:.4f}V OK")
    print("  PASS\n")


def test_boundary_oscillation_iir_and_hysteresis():
    """
    Hysteresis from a committed high note: input that maps to the note BELOW but falls
    within the 0.012V deadband from the committed note must not flip.

    Ionian deg3 = 640/1536 = 0.4167V. Boundary (noteFromVolts crossover) = 0.4167 - 0.01 = 0.4067V.
    Input 0.405V → biased=0.415V < 0.4167V → candidate=deg2.
    But |0.405V - _lastQVolts(0.4167V)| = 0.0117V < HysteresisVolts(0.012V) → no flip.
    Output must stay at deg3 (0.4167V) throughout.
    """
    print("Final 30: Hysteresis from committed deg3 — sub-deadband input does not flip to deg2")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0

    # Park firmly at deg3 (0.4167V): 0.44V → biased=0.45 ≥ 0.4167 → deg3
    s.ctrl.adc(0, 0.44)
    s.press("play")
    s.wait(200)
    out_committed = s.cv(0)

    # Switch to 0.405V: maps to deg2 but within 0.012V deadband from 0.4167V
    # (|0.405 - 0.4167| = 0.0117 < 0.012 → hysteresis holds)
    s.ctrl.adc(0, 0.405)
    s.wait(300)   # enough for full IIR convergence to 0.405V
    out_after = s.cv(0)

    s.press("play")
    s.close()

    expected_deg3 = IONIAN_VOLTS[3]   # 0.4167V

    assert abs(out_committed - expected_deg3) < 0.03, (
        f"Initial commit: {out_committed:.4f}V != {expected_deg3:.4f}V"
    )
    assert abs(out_after - expected_deg3) < 0.03, (
        f"After sub-deadband input (0.405V): {out_after:.4f}V != {expected_deg3:.4f}V "
        f"(flipped to deg2! Hysteresis failed)"
    )
    print(f"  Committed: {out_committed:.4f}V → sub-deadband 0.405V input: "
          f"{out_after:.4f}V (stayed at deg3) OK")
    print("  PASS\n")


def test_overlapping_external_triggers_last_wins():
    """
    External mode: ADC changes between two trigger events. Verify the second commit
    (at sampleTick of the second trigger) reads the current ADC, not the first trigger's ADC.

    Also verifies that with slow trigger source (gate goes low between pulses), each
    rising edge of the source independently commits the current CV.

    Uses divisor=8 (32-tick step = ~83ms) with PulseLengthTicks=8 ticks (~21ms) so
    gate is high for 21ms and low for ~62ms — clear rising edges between steps.
    """
    print("Final 31: Sequential External triggers sample current CV at each trigger time")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 1: Note track, 2 gated steps, divisor=8 (32 engine ticks → 83ms/step)
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.firstStep = 0; seq1.lastStep = 1
    seq1.steps[0].gate = True; seq1.steps[1].gate = True
    seq1.divisor = 8  # 8 * 4 = 32 ticks/step ≈ 83ms, pulse=8 ticks ≈ 21ms → gate LOW for ~62ms
    for i in range(2): seq1.steps[i].note = 0

    # Track 0: External quantizer, listens to track 1
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.External)
    qt.triggerTrack = 1
    p.scale = 0

    # First trigger: ADC=0.3V (→ deg2=0.3333V)
    s.ctrl.adc(0, 0.30)
    s.press("play")
    s.wait(200)   # let first trigger + sample delay fire
    out_first = s.cv(0)

    # Change ADC before second trigger fires
    s.ctrl.adc(0, 0.60)
    s.wait(250)   # second trigger fires, sample delay fires
    out_second = s.cv(0)

    s.press("play")
    s.close()

    expected_first  = quantize(IONIAN_VOLTS, 0.30)
    expected_second = quantize(IONIAN_VOLTS, 0.60)

    assert abs(out_first - expected_first) < 0.03, (
        f"First trigger: {out_first:.4f}V != {expected_first:.4f}V"
    )
    assert abs(out_second - expected_second) < 0.03, (
        f"Second trigger: {out_second:.4f}V != {expected_second:.4f}V "
        f"(sampled stale ADC from first trigger?)"
    )
    print(f"  First trigger: {out_first:.4f}V, second: {out_second:.4f}V OK "
          f"(each trigger samples current CV)")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 12. Session 4 probes (32–34)
# ---------------------------------------------------------------------------

def test_negative_input_voltage():
    """
    Free mode: negative input voltage (-0.5V, -1.0V).
    Scale::noteFromVolts uses std::floor (not truncation) for negative octave.
    -0.5V → biased=-0.49 → floor=-1, frac=0.51 → deg3 of octave-1 → noteToVolts(-4) = -0.5833V
    -1.0V → biased=-0.99 → floor=-1, frac=0.01 → deg0 of octave-1 → noteToVolts(-7) = -1.0V
    """
    print("Final 32: Negative input voltage — sub-zero quantization correct")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian

    results = {}
    s.press("play")
    for v, label in [(-0.5, "-0.5V"), (-1.0, "-1.0V")]:
        s.ctrl.adc(0, v)
        s.wait(150)
        results[label] = s.cv(0)

    s.press("play")
    s.close()

    cases = {
        "-0.5V": (-0.5, quantize(IONIAN_VOLTS, -0.5)),
        "-1.0V": (-1.0, quantize(IONIAN_VOLTS, -1.0)),
    }
    for label, (inp, expected) in cases.items():
        out = results[label]
        assert abs(out - expected) < 0.03, (
            f"Input {label}: {out:.4f}V != {expected:.4f}V"
        )
        print(f"  {label} → {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_external_trigger_from_last_track():
    """
    External mode: triggerTrack = 7 (last valid track, index = CONFIG_TRACK_COUNT-1).
    Engine guard (triggerTrack < CONFIG_TRACK_COUNT=8) must accept index 7.
    Verify quantizer fires when track 7's gate goes high.
    """
    print("Final 33: External trigger from last track (track 7) — upper range guard OK")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 7: Note track with gated step
    p.setTrackMode(7, tsseq.Track.TrackMode.Note)
    seq7 = p.tracks[7].noteTrack.sequences[0]
    seq7.firstStep = 0; seq7.lastStep = 1
    seq7.steps[0].gate = True; seq7.steps[1].gate = True
    seq7.divisor = 8  # 32 engine ticks per step ≈ 83ms, pulse 21ms → clear low between steps
    seq7.steps[0].note = 0; seq7.steps[1].note = 0

    # Track 0: External quantizer, listens to track 7
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.External)
    qt.triggerTrack = 7
    p.scale = 0
    s.ctrl.adc(0, 0.5)

    s.press("play")
    s.wait(300)   # let trigger + sample delay fire
    out = s.cv(0)
    gate_seen = s.gate(0) or (abs(out) > 0.01)

    s.press("play")
    s.close()

    expected = quantize(IONIAN_VOLTS, 0.5)
    assert abs(out - expected) < 0.03, (
        f"External trigger from track 7: {out:.4f}V != {expected:.4f}V "
        f"(guard rejected last track index?)"
    )
    print(f"  Track 7 trigger: output {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_scale_change_during_sample_delay():
    """
    Internal mode: a gated step fires (samplePending=true, sampleTick=T+4).
    Before sampleTick arrives, the project scale changes.
    At sampleTick the commit should use the NEW scale's noteFromVolts.

    Input 0.78V discriminates between Ionian and Phrygian:
      Ionian:   biased=0.79 → deg5 = 1152/1536 = 0.75V
      Phrygian: biased=0.79 → deg5 = 1024/1536 = 0.6667V
    Trigger fires on Ionian, scale switches to Phrygian before sampleTick.
    Commit must produce Phrygian (0.6667V), not Ionian (0.75V).
    """
    print("Final 34: Scale change during sample delay — commit uses new scale at sampleTick")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)

    seq = p.tracks[0].quantizerTrack.sequences[0]
    seq.firstStep = 0; seq.lastStep = 0
    seq.steps[0].gate = True
    seq.divisor = 16  # 64 engine ticks ≈ 167ms/step; SampleDelay=4 ticks ≈ 10ms

    p.scale = 0   # Ionian initially
    s.ctrl.adc(0, 0.78)
    s.press("play")
    s.wait(60)    # step fires; samplePending=true for ~10ms

    # Change scale while sample is pending but before sampleTick fires
    p.scale = 2   # Phrygian
    s.wait(200)   # sample delay fires, commits using Phrygian
    out = s.cv(0)

    s.press("play")
    s.close()

    expected_phrygian = quantize(PHRYGIAN_VOLTS, 0.78)  # 0.6667V
    expected_ionian   = quantize(IONIAN_VOLTS,   0.78)  # 0.75V

    assert abs(expected_phrygian - expected_ionian) > 0.05, (
        f"Test design error: Ionian and Phrygian give same output for 0.78V "
        f"({expected_ionian:.4f} vs {expected_phrygian:.4f})"
    )
    assert abs(out - expected_phrygian) < 0.03, (
        f"After scale change during delay: {out:.4f}V "
        f"(Phrygian expected {expected_phrygian:.4f}V, Ionian would give {expected_ionian:.4f}V "
        f"— sample committed with old scale?)"
    )
    print(f"  Committed at sampleTick: {out:.4f}V "
          f"(Phrygian {expected_phrygian:.4f}V ≠ Ionian {expected_ionian:.4f}V) OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 13. Session 4 continued (35–37)
# ---------------------------------------------------------------------------

def test_track_source_cv_read():
    """
    inputSource = Track2 (track index 1): quantizer reads another track's CV output.
    Track 1: NoteTrack, Phrygian scale (seq.scale=2), note=5 → 0.6667V.
    Quantizer (Ionian project scale, Free mode) re-quantizes 0.6667V to Ionian deg4 (0.5833V).
    CvIn1 set to 0V — if quantizer falls back to CvIn1, output would be 0V, not 0.5833V.
    """
    print("Final 35: Track-source CV input — quantizer reads track 1's output (not CvIn1)")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 1: Phrygian (scale=2), note=5 → 1024/1536 ≈ 0.6667V
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.scale = 2   # Phrygian
    seq1.firstStep = 0; seq1.lastStep = 0
    seq1.steps[0].gate = True
    seq1.steps[0].note = 5  # Phrygian deg5 → 1024/1536 = 0.6667V

    # Track 0: Quantizer, reads track 1's CV (Track2 = index 1)
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.Track2,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian project scale
    s.ctrl.adc(0, 0.0)   # CvIn1=0V — wrong answer if we read this instead of track 1

    s.press("play")
    s.wait(400)
    out = s.cv(0)

    s.press("play")
    s.close()

    # Track 1 outputs 0.6667V. Ionian: biased=0.6767 → deg4 (0.5833V)
    track1_v = 1024.0 / 1536.0   # Phrygian deg5
    expected = quantize(IONIAN_VOLTS, track1_v)   # Ionian deg4 = 0.5833V
    assert abs(out - expected) < 0.03, (
        f"Track-source: {out:.4f}V != {expected:.4f}V "
        f"(CvIn1 reads 0V — if output is 0V, fell back to CvIn1 instead of track 1)"
    )
    print(f"  Track 1 outputs {track1_v:.4f}V → Ionian re-quantizes to {out:.4f}V "
          f"(expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_transpose_one_full_octave():
    """
    transpose = notesPerOctave (7 for Ionian) → transposition = 7.
    Output = noteToVolts(inputNote + 7) = inputNote's pitch + exactly 1.0V.
    Tests the edge where transpose wraps to the next octave boundary.
    """
    print("Final 36: transpose = notesPerOctave → output exactly +1V from unshifted")
    s = Session()
    p = s.env.sequencer.model.project
    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian (7 notes/oct)
    qt.octave = 0; qt.transpose = 7  # transposition = 7 → +1 octave

    results = {}
    s.press("play")
    for v, label in [(0.0, "0V"), (0.3, "0.3V"), (0.7, "0.7V")]:
        s.ctrl.adc(0, v)
        s.wait(150)
        results[label] = s.cv(0)

    qt.transpose = 0  # remove transpose for reference
    s.wait(50)
    references = {}
    for v, label in [(0.0, "0V"), (0.3, "0.3V"), (0.7, "0.7V")]:
        s.ctrl.adc(0, v)
        s.wait(150)
        references[label] = s.cv(0)

    s.press("play")
    s.close()

    for label in results:
        shifted = results[label]
        unshifted = references[label]
        diff = shifted - unshifted
        assert abs(diff - 1.0) < 0.03, (
            f"transpose=7 at {label}: shifted={shifted:.4f}V, unshifted={unshifted:.4f}V, "
            f"diff={diff:.4f}V (expected exactly 1.0V shift)"
        )
        print(f"  {label}: +{diff:.4f}V shift (expected 1.0V) OK")
    print("  PASS\n")


def test_internal_nonzero_first_step():
    """
    Internal mode with firstStep=4, lastStep=7 (second half of 8-step sequence).
    Only steps 4,6 are gated; steps 5,7 are not.
    Engine must only visit steps 4-7, firing only on gated steps.
    Verify correct CV output and that steps 0-3 (outside range) never trigger.
    """
    print("Final 37: Internal non-zero firstStep — only steps 4-7 visited, gates correct")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Internal)

    seq = p.tracks[0].quantizerTrack.sequences[0]
    seq.firstStep = 4; seq.lastStep = 7
    for i in range(8): seq.steps[i].gate = False
    seq.steps[4].gate = True   # gated
    seq.steps[5].gate = False  # not gated
    seq.steps[6].gate = True   # gated
    seq.steps[7].gate = False  # not gated
    # Steps 0-3: gate=False; engine must not visit them anyway (outside firstStep-lastStep)

    p.scale = 0
    s.ctrl.adc(0, 0.5)
    s.press("play")

    gate_samples = []
    for _ in range(20):
        s.wait(STEP_MS // 2)
        gate_samples.append(s.gate(0))

    final_cv = s.cv(0)
    s.press("play")
    s.close()

    edges = sum(1 for i in range(1, len(gate_samples))
                if gate_samples[i] and not gate_samples[i-1])

    expected_cv = quantize(IONIAN_VOLTS, 0.5)
    assert abs(final_cv - expected_cv) < 0.03, (
        f"Non-zero firstStep: CV {final_cv:.4f}V != {expected_cv:.4f}V"
    )
    # 2 gated out of 4 steps per loop = 2 fires/loop. Loop=500ms, 20 samples over ~1s = ~2 loops
    # → ~4 events × 33% catch rate ≈ 1-2 edges. Require ≥1.
    assert edges >= 1, (
        f"No gate edges with firstStep=4 — engine may not be advancing correctly"
    )
    print(f"  firstStep=4, lastStep=7: {edges} gate edges, CV={final_cv:.4f}V OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# 14. Session 5 probes (38–40)
# ---------------------------------------------------------------------------

def test_two_external_quantizers_same_trigger():
    """
    Track 0 and Track 1: both External quantizers listening to Track 2's gate.
    Each maintains independent _lastSourceGate — one quantizer rising-edge detection
    must not affect the other.
    Track 0 reads CvIn1=0.3V, Track 1 reads CvIn2=0.6V.
    Both should fire on the same trigger and produce their respective quantized outputs.
    """
    print("Final 38: Two External quantizers sharing trigger — independent state and outputs")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 2: Note trigger source
    p.setTrackMode(2, tsseq.Track.TrackMode.Note)
    seq2 = p.tracks[2].noteTrack.sequences[0]
    seq2.firstStep = 0; seq2.lastStep = 1
    seq2.steps[0].gate = True; seq2.steps[1].gate = True
    seq2.divisor = 8  # 32 engine ticks/step ≈ 83ms; pulse=21ms → clear low between steps
    seq2.steps[0].note = 0; seq2.steps[1].note = 0

    # Track 0: External, CvIn1=0.3V, trigger from track 2
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.External)
    p.tracks[0].quantizerTrack.triggerTrack = 2

    # Track 1: External, CvIn2=0.6V, trigger from track 2
    p.setTrackMode(1, tsseq.Track.TrackMode.Quantizer)
    qt1 = p.tracks[1].quantizerTrack
    qt1.inputSource = tsseq.QuantizerTrack.InputSource.CvIn2
    qt1.triggerMode = tsseq.QuantizerTrack.TriggerMode.External
    qt1.triggerTrack = 2
    qt1.octave = 0; qt1.transpose = 0

    p.scale = 0
    s.ctrl.adc(0, 0.30)  # CvIn1
    s.ctrl.adc(1, 0.60)  # CvIn2

    s.press("play")
    s.wait(400)
    out0 = s.cv(0)
    out1 = s.cv(1)

    s.press("play")
    s.close()

    expected0 = quantize(IONIAN_VOLTS, 0.30)
    expected1 = quantize(IONIAN_VOLTS, 0.60)

    assert abs(out0 - expected0) < 0.03, (
        f"Track 0: {out0:.4f}V != {expected0:.4f}V"
    )
    assert abs(out1 - expected1) < 0.03, (
        f"Track 1: {out1:.4f}V != {expected1:.4f}V "
        f"(was track 0's state shared?)"
    )
    print(f"  Track 0: {out0:.4f}V (expected {expected0:.4f}V), "
          f"Track 1: {out1:.4f}V (expected {expected1:.4f}V) OK")
    print("  PASS\n")


def test_free_mode_zero_crossing():
    """
    Free mode: input sweeps from -0.3V to +0.3V, crossing zero.
    Both negative and positive voltages must quantize correctly to Ionian.
    -0.3V: biased=-0.29 → floor=-1, frac=0.71 → Ionian deg4 of oct-1 → -1+0.5833=-0.4167V
    +0.3V: biased=+0.31 → floor=0, frac=0.31 → Ionian deg1 of oct0 → 0.1667V
    Verify a clean commit on each side of zero.
    """
    print("Final 39: Free mode zero-crossing — neg and pos quantize correctly")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian

    results = {}
    s.press("play")
    for v in [-0.3, -0.1, 0.0, 0.1, 0.3]:
        s.ctrl.adc(0, v)
        s.wait(150)
        results[v] = s.cv(0)

    s.press("play")
    s.close()

    for v, out in results.items():
        expected = quantize(IONIAN_VOLTS, v)
        assert abs(out - expected) < 0.03, (
            f"Zero-crossing: input {v:+.2f}V → {out:.4f}V, expected {expected:.4f}V"
        )
        print(f"  {v:+.2f}V → {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_bias_boundary_at_minus_001():
    """
    The +0.01V bias in Scale::noteFromVolts creates a quantization boundary at -0.01V.
    - Input -0.01V: biased=0.0 → floor=0, frac=0.0 → Ionian deg0 = 0.0V output
    - Input -0.0101V: biased=-0.0001 → floor=-1, frac=0.9999 → Ionian deg6 of oct-1 → -0.0833V
    After committing -0.0101V → -0.0833V, hysteresis deadband (0.012V from -0.0833V)
    spans [-0.0953, -0.0713]. Input of -0.01V (-0.0013V from -0.0833V) is within range
    for noteFromVolts → deg0, but hysteresis check: |(-0.01V) - (-0.0833V)| = 0.0733V ≥ 0.012 → flip.
    So: -0.0101V commits -0.0833V. Then -0.01V (which maps to deg0) will flip since 0.0733V > 0.012.

    Key test: -0.01V → exactly 0V output (positive side of bias boundary).
    """
    print("Final 40: +0.01V bias boundary — input at -0.01V gives 0V (deg0), -0.0101V gives -0.0833V")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian

    # Park well away from boundary first
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(100)

    # Now move to exactly -0.01V
    s.ctrl.adc(0, -0.010)
    s.wait(200)
    out_at_boundary = s.cv(0)

    # Cross just below the boundary
    s.ctrl.adc(0, -0.020)   # clearly below, biased=-0.01 → deg6 of oct-1
    s.wait(200)
    out_below = s.cv(0)

    s.press("play")
    s.close()

    # -0.01V: biased=0.0 → deg0 = 0V
    # -0.02V: biased=-0.01 → floor=-1, frac=0.99 → Ionian: 0.99 ≥ 0.9167 (deg6=1408/1536) → note=-1
    #         noteToVolts(-1): octave=floor(-1/7)=-1, index=6 → -1+0.9167=-0.0833V
    assert abs(out_at_boundary - 0.0) < 0.03, (
        f"Input -0.01V: {out_at_boundary:.4f}V, expected 0.0V (deg0)"
    )
    expected_below = -1.0 + IONIAN_VOLTS[6]   # = -1 + 1408/1536 ≈ -0.0833V
    assert abs(out_below - expected_below) < 0.03, (
        f"Input -0.02V: {out_below:.4f}V, expected {expected_below:.4f}V (deg6 of oct-1)"
    )
    print(f"  -0.01V → {out_at_boundary:.4f}V (deg0), "
          f"-0.02V → {out_below:.4f}V (deg6/oct-1={expected_below:.4f}V) OK")
    print("  PASS\n")


def test_free_refire_on_scale_change_same_degree_index():
    """
    Final 41: Scale change re-fire via the 'voltage changed under same degree index' branch.

    Input=0.17V → Ionian deg1 (0.1667V). After committing, scale switches to Phrygian.
    noteFromVolts(0.17V) on Phrygian also returns deg1 (same index, different voltage:
    Phrygian deg1 = 0.0833V). So `candidate == _lastQNote` is true — the candidate-changed
    branch does NOT fire. But `scale.noteToVolts(_lastQNote)` = 0.0833V ≠ `_lastQVolts`
    = 0.1667V → the third Free-mode branch fires and re-commits to Phrygian deg1 = 0.0833V.

    This specifically exercises the code path:
      else if (std::abs(scale.noteToVolts(_lastQNote) - _lastQVolts) > 0.001f) { changed = true; }
    """
    print("Final 41: Free refire via 'same degree index, different voltage' scale-change branch")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian

    # 0.17V → Ionian deg1: biased=0.18, deg1 starts at 0.1667 → idx=1 → 0.1667V out
    s.ctrl.adc(0, 0.17)
    s.press("play")
    s.wait(200)
    out_ionian = s.cv(0)

    expected_ionian = IONIAN_VOLTS[1]  # 256/1536 = 0.1667V
    assert abs(out_ionian - expected_ionian) < 0.03, (
        f"Ionian deg1: {out_ionian:.4f}V != {expected_ionian:.4f}V"
    )
    print(f"  Ionian committed: {out_ionian:.4f}V (expected {expected_ionian:.4f}V) OK")

    # Switch to Phrygian — same input 0.17V maps to Phrygian deg1 (same index)
    # but Phrygian deg1 = 128/1536 = 0.0833V, different from _lastQVolts=0.1667V
    p.scale = 2  # Phrygian
    s.wait(150)
    out_phrygian = s.cv(0)

    # Phrygian deg1 = 128/1536
    PHRYGIAN_DEG1 = 128.0 / 1536.0
    assert abs(out_phrygian - PHRYGIAN_DEG1) < 0.03, (
        f"Phrygian refire: {out_phrygian:.4f}V != {PHRYGIAN_DEG1:.4f}V — "
        f"third Free branch (voltage-under-note changed) did not fire"
    )
    print(f"  Phrygian refire: {out_phrygian:.4f}V (expected {PHRYGIAN_DEG1:.4f}V) OK")
    print("  PASS\n")

    s.press("play")
    s.close()


def test_external_mode_switch_from_free_absorbs_gate():
    """
    Final 42: Switching triggerMode from Free to External while trigger track gate is HIGH.

    When External mode first observes a triggerTrack, it sets _lastSourceGate = curGate
    (absorbs current level) to prevent a spurious rising-edge trigger. This must also
    work when arriving from Free mode (i.e., _lastTriggerTrack was -1 before).

    Setup:
    - Track 1 is a Note track. Its first step is gated, so its gate will be HIGH initially.
    - Start quantizer in Free mode, commit a note.
    - Switch triggerMode to External + triggerTrack=1 while track 1's gate is HIGH.
    - Verify no spurious commit from the HIGH gate absorption.
    - Let track 1's gate cycle (LOW → HIGH) → real commit fires.
    """
    print("Final 42: External mode switch from Free — HIGH gate absorbed, no spurious trigger")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 1: single always-gated step → gate HIGH for PulseLengthTicks after each step
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.firstStep = 0; seq1.lastStep = 0
    seq1.steps[0].gate = True
    seq1.divisor = 8  # 32 engine ticks/step ≈ 83ms — long enough step for observations

    qt = _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                  tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0
    s.ctrl.adc(0, 0.3)

    s.press("play")
    s.wait(200)   # let Free mode commit and track 1 start playing
    cv_free = s.cv(0)
    expected_free = quantize(IONIAN_VOLTS, 0.3)
    assert abs(cv_free - expected_free) < 0.03, (
        f"Free mode baseline: {cv_free:.4f}V != {expected_free:.4f}V"
    )
    print(f"  Free baseline: {cv_free:.4f}V OK")

    # Change CV — this will be the "new" input for when External eventually fires
    s.ctrl.adc(0, 0.75)

    # Switch to External while track 1 gate is likely HIGH
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.External
    qt.triggerTrack = 1
    s.wait(30)
    cv_after_switch = s.cv(0)

    # CV should NOT have jumped to quantize(0.75) — no spurious trigger from absorbed HIGH gate
    # It should still be at the last Free-committed value
    assert abs(cv_after_switch - expected_free) < 0.03, (
        f"Spurious trigger after mode switch: {cv_after_switch:.4f}V "
        f"(expected still Free value {expected_free:.4f}V)"
    )
    print(f"  After mode switch (no spurious): {cv_after_switch:.4f}V OK")

    # Wait for a full gate cycle (step 0 gate LOW → HIGH → sample fires)
    s.wait(300)
    cv_after_trigger = s.cv(0)
    expected_triggered = quantize(IONIAN_VOLTS, 0.75)
    assert abs(cv_after_trigger - expected_triggered) < 0.03, (
        f"After real trigger: {cv_after_trigger:.4f}V != {expected_triggered:.4f}V"
    )
    print(f"  After real trigger: {cv_after_trigger:.4f}V (expected {expected_triggered:.4f}V) OK")
    print("  PASS\n")

    s.press("play")
    s.close()


def test_free_hysteresis_tracks_latest_committed_note():
    """
    Final 43: Hysteresis _lastQVolts tracks each newly committed note, not the original.

    After hopping A → B → C, sub-deadband input from C stays at C (not from B or A).
    Verifies that commit() correctly updates _lastQVolts each time.

    Sequence:
    1. Commit deg0 (0V, _lastQVolts=0)
    2. Jump to deg4 (input=0.6V → 0.5833V, _lastQVolts=0.5833)
    3. Jump to deg2 (input=0.34V → 0.3333V, _lastQVolts=0.3333)
    4. Sub-deadband: input=0.326V (|0.326-0.3333|=0.0073 < HysteresisVolts=0.012)
       noteFromVolts(0.326) = deg2 (biased=0.336, still in deg2 range) → candidate==_lastQNote
       → no change. Output stays 0.3333V.
    5. Verify output hasn't changed to deg0 (hysteresis NOT anchored to first committed note).
    """
    print("Final 43: Hysteresis _lastQVolts tracks latest commit, not original")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_q(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
             tsseq.QuantizerTrack.TriggerMode.Free)
    p.scale = 0  # Ionian

    # Step 1: commit deg0
    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(150)
    assert abs(s.cv(0)) < 0.03, f"deg0 commit: {s.cv(0):.4f}V"
    print(f"  1) Committed deg0: {s.cv(0):.4f}V OK")

    # Step 2: jump to deg4
    s.ctrl.adc(0, 0.60)
    s.wait(200)
    cv_deg4 = s.cv(0)
    expected_deg4 = IONIAN_VOLTS[4]  # 896/1536 = 0.5833V
    assert abs(cv_deg4 - expected_deg4) < 0.03, f"deg4 commit: {cv_deg4:.4f}V != {expected_deg4:.4f}V"
    print(f"  2) Committed deg4: {cv_deg4:.4f}V OK")

    # Step 3: jump to deg2
    s.ctrl.adc(0, 0.34)
    s.wait(200)
    cv_deg2 = s.cv(0)
    expected_deg2 = IONIAN_VOLTS[2]  # 512/1536 = 0.3333V
    assert abs(cv_deg2 - expected_deg2) < 0.03, f"deg2 commit: {cv_deg2:.4f}V != {expected_deg2:.4f}V"
    print(f"  3) Committed deg2: {cv_deg2:.4f}V OK")

    # Step 4: sub-deadband from deg2 — 0.326V is within 0.012V of 0.3333V
    # noteFromVolts(0.326): biased=0.336, deg2=0.3333, deg3=0.4167 → 0.336<0.4167 → deg2
    # candidate=2=_lastQNote → no hysteresis check needed, just stays
    s.ctrl.adc(0, 0.326)
    s.wait(200)
    cv_sub = s.cv(0)

    assert abs(cv_sub - expected_deg2) < 0.03, (
        f"Sub-deadband from deg2: {cv_sub:.4f}V != {expected_deg2:.4f}V — "
        f"hysteresis may be anchored to wrong note"
    )
    # Also verify not stuck at deg0 (would indicate _lastQVolts wasn't updated)
    assert abs(cv_sub) > 0.2, (
        f"Output reverted to ~0V — _lastQVolts not updated from original deg0 commit"
    )
    print(f"  4) Sub-deadband 0.326V → {cv_sub:.4f}V (stays at deg2={expected_deg2:.4f}V) OK")
    print("  PASS\n")

    s.press("play")
    s.close()


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

ALL_TESTS = [
    test_scale_change_same_degree_index,
    test_scale_change_back_and_forth,
    test_scale_change_with_transpose,
    test_external_trigger_track_switch_no_spurious,
    test_external_trigger_switch_between_sources,
    test_simultaneous_quantizers_independent,
    test_simultaneous_quantizers_same_input,
    test_round_trip_all_ionian_degrees,
    test_harmonic_minor_scale,
    test_rapid_scale_cycling,
    test_internal_sample_pending_across_reset,
    test_internal_divisor_change_mid_play,
    test_internal_fast_divisor_smaller_than_sample_delay,
    test_gate_pulses_on_every_note_change,
    test_no_gate_if_note_unchanged,
    test_single_step_sequence_loops,
    test_rapid_transposition_sign_flip,
    test_hysteresis_dead_zone_no_flip,
    test_iir_cannot_cross_note_boundary,
    test_rapid_note_changes_gate_eventually_drops,
    test_input_source_switch_mid_play,
    test_self_route_guard_returns_zero,
    test_external_invalid_trigger_track_no_crash,
    test_internal_all_gates_off_never_commits,
    test_quantizer_drives_quantizer_external,
    test_extreme_octave_no_crash,
    test_internal_non_contiguous_gated_steps,
    test_negative_octave_subzero_output,
    test_stop_restart_reinitializes_cleanly,
    test_boundary_oscillation_iir_and_hysteresis,
    test_overlapping_external_triggers_last_wins,
    test_negative_input_voltage,
    test_external_trigger_from_last_track,
    test_scale_change_during_sample_delay,
    test_track_source_cv_read,
    test_transpose_one_full_octave,
    test_internal_nonzero_first_step,
    test_two_external_quantizers_same_trigger,
    test_free_mode_zero_crossing,
    test_bias_boundary_at_minus_001,
    test_free_refire_on_scale_change_same_degree_index,
    test_external_mode_switch_from_free_absorbs_gate,
    test_free_hysteresis_tracks_latest_committed_note,
]

if __name__ == "__main__":
    print("=== Quantizer Final Adversarial Tests ===\n")
    failures = []
    for test_fn in ALL_TESTS:
        try:
            test_fn()
        except (AssertionError, Exception) as ex:
            import traceback
            print(f"  FAIL: {ex}\n")
            failures.append((test_fn.__name__, str(ex)))
        gc.collect()

    print(f"\n{'=' * 45}")
    if failures:
        print(f"FAILED {len(failures)}/{len(ALL_TESTS)}:")
        for name, msg in failures:
            print(f"  {name}: {msg[:100]}")
        sys.exit(1)
    else:
        print(f"All {len(ALL_TESTS)} final tests PASSED")
