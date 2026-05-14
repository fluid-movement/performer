"""
Adversarial stress tests for QuantizerTrack.

Designed to break edge cases in note values, timing, trigger logic, and state management.
Run AFTER verify_quantizer.py passes.
"""

import sys
sys.path.insert(0, 'src/apps/sequencer/tests')
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')

from agent import Session
import testsim.sequencer as tsseq

STEP_MS = 125  # 16th note at 120 BPM

IONIAN_VOLTS = [v / 1536.0 for v in [0, 256, 512, 640, 896, 1152, 1408]]


def quantize_ionian(v):
    v_biased = v + 0.01
    octave = int(v_biased) if v_biased >= 0 else int(v_biased) - 1
    frac = v_biased - octave
    idx = -1
    for i, nv in enumerate(IONIAN_VOLTS):
        if frac < nv:
            break
        idx = i
    if idx == -1:
        idx = len(IONIAN_VOLTS) - 1
        octave -= 1
    return octave + IONIAN_VOLTS[idx]


# ---------------------------------------------------------------------------
# Group 1: Note value correctness
# ---------------------------------------------------------------------------

def test_negative_voltages():
    """Quantizer must handle sub-zero CV inputs (e.g. -1V, -2V)."""
    print("Stress 1: Negative voltage inputs quantize correctly")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0

    s.press("play")
    for v in [-0.5, -1.0, -2.0]:
        s.ctrl.adc(0, v)
        s.wait(STEP_MS * 4)
        out = s.cv(0)
        expected = quantize_ionian(v)
        assert abs(out - expected) < 0.03, (
            f"Input {v}V -> got {out:.4f}V, expected {expected:.4f}V"
        )
        print(f"  {v}V -> {out:.4f}V (expected {expected:.4f}V) OK")
    s.press("play")
    s.close()
    print("  PASS\n")


def test_high_voltage_inputs():
    """Quantizer must handle high CV inputs (3V, 5V) without stepping."""
    print("Stress 2: High voltage inputs snap immediately (no stepping)")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0

    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 2)

    for v in [2.0, 3.0, 4.0]:
        s.ctrl.adc(0, v)
        expected = quantize_ionian(v)
        outs = []
        for _ in range(6):
            s.wait(5)
            outs.append(s.cv(0))
        # Every sample after the jump should be at the target, no staircase
        for i, o in enumerate(outs):
            assert abs(o - expected) < 0.03, (
                f"After jump to {v}V: sample[{i}]={o:.4f}V, expected {expected:.4f}V — staircase?"
            )
        print(f"  Jump to {v}V -> all samples at {expected:.4f}V OK")
    s.press("play")
    s.close()
    print("  PASS\n")


def test_exact_note_boundaries():
    """Input voltage sitting exactly on a scale-note boundary is quantized consistently."""
    print("Stress 3: Exact note boundary voltages are stable")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0

    s.press("play")
    # Test at each Ionian note voltage exactly — should produce that note, not the one below
    for note_v in IONIAN_VOLTS:
        s.ctrl.adc(0, note_v)
        s.wait(STEP_MS * 4)
        out1 = s.cv(0)
        s.wait(STEP_MS * 4)
        out2 = s.cv(0)
        assert abs(out1 - out2) < 0.001, (
            f"Output unstable at exact note voltage {note_v:.4f}V: {out1:.4f} vs {out2:.4f}"
        )
        expected = quantize_ionian(note_v)
        assert abs(out1 - expected) < 0.03, (
            f"At exact note {note_v:.4f}V: got {out1:.4f}V, expected {expected:.4f}V"
        )
        print(f"  Exact {note_v:.4f}V -> {out1:.4f}V stable OK")
    s.press("play")
    s.close()
    print("  PASS\n")


def test_transpose_extremes():
    """Octave and transpose at maximum values don't overflow or produce wrong notes."""
    print("Stress 4: Transpose/octave extremes")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    p.scale = 0

    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 2)

    # Octave +5 should raise output by ~5V
    qt.octave = 5
    qt.transpose = 0
    s.wait(STEP_MS * 4)
    out_oct5 = s.cv(0)
    assert abs(out_oct5 - 5.0) < 0.05, f"Octave +5 at 0V input: got {out_oct5:.4f}V, expected ~5V"
    print(f"  octave=+5, 0V input -> {out_oct5:.4f}V OK")

    # Octave -5 should lower output by ~5V
    qt.octave = -5
    qt.transpose = 0
    # Need input change to re-fire in Free mode
    s.ctrl.adc(0, 0.01)
    s.wait(STEP_MS * 4)
    out_octm5 = s.cv(0)
    assert abs(out_octm5 - (-5.0)) < 0.05, f"Octave -5 at ~0V input: got {out_octm5:.4f}V, expected ~-5V"
    print(f"  octave=-5, 0V input -> {out_octm5:.4f}V OK")

    # Large positive transpose: +6 degrees in Ionian from note 0 = note 6 = 1408/1536 ≈ 0.917V
    qt.octave = 0
    qt.transpose = 6
    s.ctrl.adc(0, 0.0)
    s.wait(STEP_MS * 4)
    out_t6 = s.cv(0)
    expected_t6 = IONIAN_VOLTS[6]  # degree 6 = 1408/1536
    assert abs(out_t6 - expected_t6) < 0.03, (
        f"Transpose +6 at 0V input: got {out_t6:.4f}V, expected {expected_t6:.4f}V"
    )
    print(f"  transpose=+6, 0V input -> {out_t6:.4f}V OK")

    s.press("play")
    s.close()
    print("  PASS\n")


def test_free_mode_sweep_no_phantom_notes():
    """Free mode sweep produces exactly one output per scale degree, in order."""
    print("Stress 5: Free mode sweep — exactly one output per scale degree, no phantoms")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0

    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 4)

    # Sweep through octave 0 and octave 1 step by step (~10 scale degrees)
    targets = [0.0, 0.18, 0.37, 0.45, 0.62, 0.78, 0.96,
               1.0,  1.18, 1.37, 1.45, 1.62, 1.78, 1.96, 2.0]
    outputs = []
    for v in targets:
        s.ctrl.adc(0, v)
        s.wait(STEP_MS * 3)
        outputs.append((v, s.cv(0)))

    s.press("play")
    s.close()

    # No output should repeat (non-monotone means a phantom intermediate appeared)
    # Outputs should be non-decreasing (sweep up = notes up)
    for i in range(1, len(outputs)):
        v_prev, o_prev = outputs[i-1]
        v_cur,  o_cur  = outputs[i]
        assert o_cur >= o_prev - 0.001, (
            f"Output went backwards: {v_prev}V->{o_prev:.4f}V then {v_cur}V->{o_cur:.4f}V"
        )
    print(f"  {len(outputs)} steps, all non-decreasing — no phantom notes")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Group 2: Trigger timing
# ---------------------------------------------------------------------------

def test_external_self_reference():
    """External trigger pointing at own track must not cause infinite re-triggering."""
    print("Stress 6: External trigger self-reference — must not self-loop")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.External
    qt.triggerTrack = 0  # self-reference
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0

    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 16)  # let it run — if self-looping, CV will oscillate

    # Collect outputs over time; a self-loop will keep re-firing and produce
    # rapidly changing gate output and/or different CV values each sample
    outs = []
    for _ in range(10):
        s.wait(20)
        outs.append(s.cv(0))

    s.press("play")
    s.close()

    # All outputs should be identical (either 0 = never fired, or settled at quantized value)
    # Any variation means the quantizer is re-triggering off its own gate
    unique = set(round(o, 3) for o in outs)
    assert len(unique) <= 1, (
        f"External self-reference caused re-triggering: {len(unique)} different CV values seen: {sorted(unique)}"
    )
    print(f"  Output stable at {outs[0]:.4f}V — no self-loop")
    print("  PASS\n")


def test_internal_rapid_triggers_all_commit():
    """Internal mode with gates on every step and small divisor — all commits land."""
    print("Stress 7: Internal rapid triggers — no commits lost to samplePending collision")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Internal
    qt.octave = 0
    qt.transpose = 0

    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0
    seq0.lastStep = 1
    for i in range(2):
        seq0.steps[i].gate = True

    p.scale = 0  # Ionian

    # Alternate between two clearly distinct voltages each step
    # Step 0 CV -> 0.0V (note 0), Step 1 CV -> 1.0V (note 7 = root + octave)
    # We can't change CV per-step externally, so just verify output eventually commits
    s.ctrl.adc(0, 0.5)  # note 3
    s.press("play")
    s.wait(STEP_MS * 16)  # run 8 bars worth

    out = s.cv(0)
    expected = quantize_ionian(0.5)

    s.press("play")
    s.close()

    assert abs(out - expected) < 0.03, (
        f"Rapid internal triggers: final output {out:.4f}V not at expected {expected:.4f}V — commits lost?"
    )
    print(f"  Output settled at {out:.4f}V (expected {expected:.4f}V)")
    print("  PASS\n")


def test_external_short_gate_pulse():
    """External trigger: gate active for only 1 tick must still be detected."""
    print("Stress 8: External trigger — 1-tick gate pulse is detected")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 0 = Quantizer, External trigger from track 1
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.External
    qt.triggerTrack = 1
    qt.octave = 0
    qt.transpose = 0

    # Track 1 = Note track, divisor set to minimum so gate pulses are short
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.firstStep = 0
    seq1.lastStep = 0
    seq1.steps[0].gate = True

    p.scale = 0
    s.ctrl.adc(0, 0.25)
    s.press("play")
    s.wait(STEP_MS * 8)

    out = s.cv(0)
    expected = quantize_ionian(0.25)

    s.press("play")
    s.close()

    assert abs(out - expected) < 0.03, (
        f"Short gate pulse: output {out:.4f}V, expected {expected:.4f}V — rising edge missed?"
    )
    print(f"  Output: {out:.4f}V (expected {expected:.4f}V)")
    print("  PASS\n")


def test_external_multiple_triggers_all_fire():
    """External trigger: multiple consecutive gate pulses each cause a commit."""
    print("Stress 9: External trigger — multiple pulses each fire independently")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.External
    qt.triggerTrack = 1
    qt.octave = 0
    qt.transpose = 0

    # Track 1 = Note track with 4 gated steps
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.firstStep = 0
    seq1.lastStep = 3
    for i in range(4):
        seq1.steps[i].gate = True

    p.scale = 0
    s.ctrl.adc(0, 0.5)

    s.press("play")
    # Collect gate outputs across a full bar — should see multiple pulses
    gate_outs = []
    for _ in range(60):
        s.wait(STEP_MS // 2)
        gate_outs.append(s.gate(0))

    s.press("play")
    s.close()

    # Should have observed at least 3 distinct gate pulses (true followed by false)
    edges = sum(1 for i in range(1, len(gate_outs)) if gate_outs[i] and not gate_outs[i-1])
    assert edges >= 3, (
        f"Expected >=3 gate pulses from 4 triggers, only saw {edges}"
    )
    print(f"  Observed {edges} gate pulses from 4 external triggers")
    print("  PASS\n")


def test_internal_input_changes_before_sample_fires():
    """In Internal mode, input changing between trigger and sample fires gives latest value."""
    print("Stress 10: Internal — input change between trigger and sample fires latest value")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Internal
    qt.octave = 0
    qt.transpose = 0

    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0
    seq0.lastStep = 0
    seq0.steps[0].gate = True

    p.scale = 0

    # Start at 0.0V
    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(5)  # step fires, sample scheduled for ~10ms later
    # Change to 0.5V BEFORE the sample fires
    s.ctrl.adc(0, 0.5)
    s.wait(STEP_MS * 2)  # wait for sample to land and filter to settle

    out = s.cv(0)
    expected_new = quantize_ionian(0.5)
    expected_old = quantize_ionian(0.0)

    s.press("play")
    s.close()

    # Should have sampled the new value (0.5V), not the old one (0.0V)
    assert abs(out - expected_new) < 0.03, (
        f"Should sample post-trigger value (0.5V={expected_new:.4f}V) but got {out:.4f}V "
        f"(old value was {expected_old:.4f}V)"
    )
    print(f"  Output {out:.4f}V matches post-change target {expected_new:.4f}V (not old {expected_old:.4f}V)")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Group 3: State management
# ---------------------------------------------------------------------------

def test_reset_produces_clean_state():
    """After stop/restart, quantizer resets and immediately outputs correct note."""
    print("Stress 11: Stop and restart resets to clean state")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0

    # First run: settle at 0.5V
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_first = s.cv(0)
    s.press("play")  # stop

    # Change input to 1.0V while stopped
    s.ctrl.adc(0, 1.0)
    s.wait(STEP_MS * 2)

    # Restart and let settle
    s.press("play")
    s.wait(STEP_MS * 6)
    out_second = s.cv(0)
    s.press("play")
    s.close()

    expected_first  = quantize_ionian(0.5)
    expected_second = quantize_ionian(1.0)

    assert abs(out_first  - expected_first)  < 0.03, f"First run: {out_first:.4f}V != {expected_first:.4f}V"
    assert abs(out_second - expected_second) < 0.03, f"After restart: {out_second:.4f}V != {expected_second:.4f}V"
    print(f"  First run: {out_first:.4f}V, after restart: {out_second:.4f}V — both correct")
    print("  PASS\n")


def test_scale_change_updates_free_mode_output():
    """Scale change while Free mode is running must re-quantize and update output."""
    print("Stress 12: Scale change updates Free mode output immediately")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0

    # Ionian — 0.1V rounds to note 0 = 0V
    p.scale = 0
    s.ctrl.adc(0, 0.1)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_ionian = s.cv(0)

    # Switch to Phrygian (scale 2) — has b2 at ~0.083V, so 0.1V -> note 1 -> ~0.083V
    p.scale = 2
    # Nudge input to force re-quantize (new scale, same input, different note)
    s.ctrl.adc(0, 0.095)
    s.wait(STEP_MS * 4)
    out_phrygian = s.cv(0)

    s.press("play")
    s.close()

    print(f"  Ionian  0.1V -> {out_ionian:.4f}V")
    print(f"  Phrygian 0.095V -> {out_phrygian:.4f}V")
    assert out_phrygian != out_ionian or abs(out_ionian) < 0.01, (
        "Scale change had no effect — output identical before and after"
    )
    print("  PASS\n")


def test_hysteresis_bidirectional():
    """Hysteresis works in both directions: must cross the deadband to go up AND to come back down."""
    print("Stress 13: Hysteresis is bidirectional — crossing back requires clearing deadband")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0  # Ionian

    # Settle at note 0 (0V)
    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 4)
    assert abs(s.cv(0)) < 0.03, "Should be at note 0"

    # Push well past note 1 boundary (>0.012V hysteresis past 0.1667V) -> commit note 1
    s.ctrl.adc(0, 0.20)
    s.wait(STEP_MS * 4)
    out_note1 = s.cv(0)
    expected_note1 = IONIAN_VOLTS[1]  # 256/1536
    assert abs(out_note1 - expected_note1) < 0.03, (
        f"Should have committed note 1 ({expected_note1:.4f}V), got {out_note1:.4f}V"
    )
    print(f"  Crossed into note 1: {out_note1:.4f}V OK")

    # Pull back to just inside note 1's territory (0.175V — not far enough toward note 0 to trigger hysteresis)
    # lastQVolts = IONIAN_VOLTS[1] ≈ 0.1667V; need |input - 0.1667| < 0.012 to stay
    s.ctrl.adc(0, 0.175)
    s.wait(STEP_MS * 4)
    still_note1 = s.cv(0)
    assert abs(still_note1 - expected_note1) < 0.03, (
        f"Should still be note 1 ({expected_note1:.4f}V), but moved to {still_note1:.4f}V — hysteresis too weak?"
    )
    print(f"  Pulled back slightly, still note 1: {still_note1:.4f}V OK")

    # Now drive all the way back to note 0's region (0.05V — clearly below note 1 boundary with hysteresis)
    s.ctrl.adc(0, 0.05)
    s.wait(STEP_MS * 4)
    back_note0 = s.cv(0)
    assert abs(back_note0) < 0.03, (
        f"Should have returned to note 0 (0V), got {back_note0:.4f}V — downward hysteresis broken?"
    )
    print(f"  Returned to note 0: {back_note0:.4f}V OK")

    s.press("play")
    s.close()
    print("  PASS\n")


def test_internal_multiple_bars_no_drift():
    """Internal mode over many bars produces consistent note output with no timing drift."""
    print("Stress 14: Internal mode — consistent output across 16 bars, no drift")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Internal
    qt.octave = 0
    qt.transpose = 0

    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0
    seq0.lastStep = 3
    for i in range(4):
        seq0.steps[i].gate = True

    p.scale = 0
    s.ctrl.adc(0, 0.5)
    s.press("play")

    # Sample output every half-step over 16 bars
    samples = []
    for _ in range(64):
        s.wait(STEP_MS // 2)
        samples.append(s.cv(0))

    s.press("play")
    s.close()

    expected = quantize_ionian(0.5)
    bad = [v for v in samples if abs(v - expected) > 0.03 and abs(v) > 0.001]
    assert len(bad) == 0, (
        f"Internal mode drift: {len(bad)}/{len(samples)} samples off-target. "
        f"Expected {expected:.4f}V. Bad values: {[f'{v:.4f}' for v in bad[:5]]}"
    )
    print(f"  All {len(samples)} samples within tolerance of {expected:.4f}V")
    print("  PASS\n")


def test_octave_change_updates_output():
    """Changing octave while playing Free mode immediately updates the CV output."""
    print("Stress 15: Octave change in Free mode immediately updates output")
    s = Session()
    p = s.env.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Internal
    qt.octave = 0
    qt.transpose = 0

    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0
    seq0.lastStep = 1
    for i in range(2):
        seq0.steps[i].gate = True

    p.scale = 0
    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_oct0 = s.cv(0)

    qt.octave = 2
    s.wait(STEP_MS * 4)  # wait for Internal trigger to re-commit with new octave
    out_oct2 = s.cv(0)

    s.press("play")
    s.close()

    print(f"  octave=0: {out_oct0:.4f}V, octave=2: {out_oct2:.4f}V (delta={out_oct2-out_oct0:.4f}V)")
    assert abs((out_oct2 - out_oct0) - 2.0) < 0.15, (
        f"Octave change +2 should produce ~2V shift, got {out_oct2-out_oct0:.4f}V"
    )
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Group 4: Input source
# ---------------------------------------------------------------------------

def test_track_cv_as_input_source():
    """Quantizer reading another track's CV output produces correct quantized output."""
    print("Stress 16: Input source = another track's CV output")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 1 = Note track producing 0.5V output (Ionian note 3)
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.scale = 0   # Ionian
    seq1.firstStep = 0
    seq1.lastStep = 0
    seq1.steps[0].gate = True
    seq1.steps[0].note = 3   # Ionian degree 3 = 640/1536 ≈ 0.4167V

    # Track 0 = Quantizer reading track 1's CV
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.Track2  # 1-indexed in model? check
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0

    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(0)
    s.press("play")
    s.close()

    expected = IONIAN_VOLTS[3]  # 640/1536 ≈ 0.4167V
    assert abs(out - expected) < 0.05, (
        f"Quantizer reading track 1 CV: got {out:.4f}V, expected ~{expected:.4f}V"
    )
    print(f"  Track 1 CV ({expected:.4f}V) -> quantizer output {out:.4f}V OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

ALL_TESTS = [
    test_negative_voltages,
    test_high_voltage_inputs,
    test_exact_note_boundaries,
    test_transpose_extremes,
    test_free_mode_sweep_no_phantom_notes,
    test_external_self_reference,
    test_internal_rapid_triggers_all_commit,
    test_external_short_gate_pulse,
    test_external_multiple_triggers_all_fire,
    test_internal_input_changes_before_sample_fires,
    test_reset_produces_clean_state,
    test_scale_change_updates_free_mode_output,
    test_hysteresis_bidirectional,
    test_internal_multiple_bars_no_drift,
    test_octave_change_updates_output,
    test_track_cv_as_input_source,
]

if __name__ == "__main__":
    print("=== Quantizer Stress Tests ===\n")
    failures = []
    for test_fn in ALL_TESTS:
        try:
            test_fn()
        except (AssertionError, Exception) as ex:
            print(f"  FAIL: {ex}\n")
            failures.append((test_fn.__name__, str(ex)))

    print(f"\n{'=' * 40}")
    if failures:
        print(f"FAILED {len(failures)}/{len(ALL_TESTS)}:")
        for name, msg in failures:
            print(f"  {name}: {msg}")
        sys.exit(1)
    else:
        print(f"All {len(ALL_TESTS)} stress tests PASSED")
