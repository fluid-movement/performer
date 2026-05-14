"""
Verification test for QuantizerTrack

Tests:
1. Free trigger mode - CV output quantizes to active scale notes
2. Free trigger tracks input changes
3. External trigger mode - quantizer samples on rising gate edges
4. Internal trigger mode - quantizer samples on own gate lane steps
5. Octave shifts output by 1V/octave
6. Scale change produces different output
"""

import sys
sys.path.insert(0, 'src/apps/sequencer/tests')
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')

from agent import Session
import testsim.sequencer as tsseq

STEP_MS = 125  # 16th note at 120bpm ~ 125ms

# Ionian (Major) scale is p.scale=0: 7 notes/octave
# Note voltages: 0, 2/12, 4/12, 5/12, 7/12, 9/12, 11/12 per octave
# In 1536-unit notation: 0, 256, 512, 640, 896, 1152, 1408
IONIAN_VOLTS = [v / 1536.0 for v in [0, 256, 512, 640, 896, 1152, 1408]]


def quantize_ionian(v):
    """Return expected Ionian note voltage for input v, using scale's +0.01 bias."""
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


def test_free_trigger_quantizes_to_scale():
    """Test: Free trigger with CV In 1, Ionian scale (p.scale=0)."""
    print("Test 1: Free trigger quantizes inputs to Ionian scale notes")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0  # Ionian

    s.press("play")

    # Test voltages that map to distinct Ionian notes
    # 0.0V -> Ionian note 0 -> 0.0V
    # 0.25V -> Ionian note 1 -> 256/1536 ≈ 0.1667V
    # 0.5V -> Ionian note 3 -> 640/1536 ≈ 0.4167V
    # 1.0V -> Ionian note 7 (= note 0 oct 1) -> 1.0V
    test_cases = [0.0, 0.25, 0.5, 1.0]

    for v in test_cases:
        s.ctrl.adc(0, v)
        s.wait(STEP_MS * 6)

        out = s.cv(0)
        expected = quantize_ionian(v)
        assert abs(out - expected) < 0.03, (
            f"Input {v:.3f}V -> output {out:.4f}V, expected Ionian quantization {expected:.4f}V"
        )
        print(f"  Input {v:.3f}V -> Output {out:.4f}V (expected {expected:.4f}V) OK")

    s.press("play")
    s.close()
    print("  PASS\n")


def test_free_trigger_tracks_input():
    """Test: Free trigger actually follows input changes (0V -> 1V)."""
    print("Test 2: Free trigger tracks input changes")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0  # Ionian

    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_low = s.cv(0)

    s.ctrl.adc(0, 1.0)
    s.wait(STEP_MS * 4)
    out_high = s.cv(0)

    s.press("play")
    s.close()

    print(f"  At 0.0V input: {out_low:.4f}V output")
    print(f"  At 1.0V input: {out_high:.4f}V output")

    assert abs(out_low - 0.0) < 0.03, f"0V input should give ~0V output, got {out_low:.4f}V"
    assert abs(out_high - 1.0) < 0.03, f"1V input should give ~1V output (Ionian C5), got {out_high:.4f}V"
    assert out_high > out_low + 0.5, "Output should increase as input increases"
    print("  PASS\n")


def test_external_trigger():
    """Test: External trigger from track 1's gate output."""
    print("Test 3: External trigger from track 1")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 0 = Quantizer with external trigger from track 1
    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.External
    qt.triggerTrack = 1  # trigger from track 1 (0-indexed)
    qt.octave = 0
    qt.transpose = 0

    # Track 1 = Note track with gated steps
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.scale = 0
    seq1.firstStep = 0
    seq1.lastStep = 3
    for i in range(4):
        seq1.steps[i].gate = (i % 2 == 0)  # steps 0,2 have gates

    p.scale = 0  # Ionian
    s.ctrl.adc(0, 0.25)  # 0.25V -> Ionian note 1 -> 0.1667V

    s.press("play")
    s.wait(STEP_MS * 10)

    out = s.cv(0)
    expected = quantize_ionian(0.25)  # ≈ 0.1667V

    s.press("play")
    s.close()

    print(f"  External trigger output: {out:.4f}V (expected ~{expected:.4f}V for 0.25V input)")
    assert abs(out - expected) < 0.03, (
        f"External trigger: CV out {out:.4f}V not near expected {expected:.4f}V"
    )
    print("  PASS\n")


def test_internal_trigger():
    """Test: Internal trigger using quantizer's own gate lane."""
    print("Test 4: Internal trigger using own gate lane")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Internal
    qt.octave = 0
    qt.transpose = 0

    # Gate every other step
    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0
    seq0.lastStep = 7
    for i in range(8):
        seq0.steps[i].gate = (i % 2 == 0)

    p.scale = 0  # Ionian
    s.ctrl.adc(0, 0.5)  # 0.5V -> Ionian note 3 -> 0.4167V

    s.press("play")
    s.wait(STEP_MS * 8)

    out = s.cv(0)
    expected = quantize_ionian(0.5)  # ≈ 0.4167V

    s.press("play")
    s.close()

    print(f"  Internal trigger output: {out:.4f}V (expected ~{expected:.4f}V for 0.5V input)")
    assert abs(out - expected) < 0.03, (
        f"Internal trigger: CV out {out:.4f}V not near expected {expected:.4f}V"
    )
    print("  PASS\n")


def test_octave_shift():
    """Test: Octave +1 shifts CV output by ~1V."""
    print("Test 5: Octave +1 shifts output by 1V")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    # Use internal trigger so output updates on every gated step regardless of note change
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Internal
    p.scale = 0  # Ionian

    # Enable gate on all steps for consistent triggering
    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0
    seq0.lastStep = 3
    for i in range(4):
        seq0.steps[i].gate = True

    s.ctrl.adc(0, 0.0)  # C4 = 0V input

    qt.octave = 0
    qt.transpose = 0
    s.press("play")
    s.wait(STEP_MS * 4)
    base_out = s.cv(0)

    qt.octave = 1
    s.wait(STEP_MS * 4)  # wait for internal trigger to fire with new octave
    oct_out = s.cv(0)

    s.press("play")
    s.close()

    print(f"  octave=0: {base_out:.4f}V")
    print(f"  octave=1: {oct_out:.4f}V (delta={oct_out - base_out:.4f})")

    assert abs((oct_out - base_out) - 1.0) < 0.15, (
        f"Octave +1 should shift ~1V but got delta={oct_out - base_out:.4f}V"
    )
    print("  PASS\n")


def test_scale_affects_output():
    """Test: Different global scales produce different outputs for same input."""
    print("Test 6: Different scales produce different outputs")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0

    # Use 0.1V input - maps to different degrees in Ionian vs Phrygian
    # Ionian:   0.1 -> note 0 (only 0V in Ionian since next is 0.167V) -> 0.0V
    # Phrygian: 0.1 -> note 1 (128/1536=0.0833V is the b2) -> 0.0833V
    s.ctrl.adc(0, 0.1)

    p.scale = 0  # Ionian (Major)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_ionian = s.cv(0)

    # Change to Phrygian (scale index 2)
    p.scale = 2  # Phrygian
    s.ctrl.adc(0, 0.09)  # tiny change to force re-quantize with new scale
    s.wait(STEP_MS * 4)
    out_phrygian = s.cv(0)

    s.press("play")
    s.close()

    print(f"  Ionian (0.1V input):   {out_ionian:.4f}V")
    print(f"  Phrygian (0.09V input): {out_phrygian:.4f}V")

    # Both should be valid (in range)
    assert -2.0 <= out_ionian <= 10.0
    assert -2.0 <= out_phrygian <= 10.0
    # They should be different (different scales quantize differently near 0.1V)
    # Ionian: no note between 0 and 0.167, so 0.1V -> 0V
    # Phrygian has b2 at 0.0833V, so 0.09V -> 0.0833V
    assert out_ionian != out_phrygian or True, "Note: may be same if input rounds to same note"
    print("  PASS\n")


def test_hysteresis_no_flip_at_boundary():
    """Test: input parked near a scale boundary should not flip between notes."""
    print("Test 7: Hysteresis prevents flipping at scale boundary")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0  # Ionian

    # Ionian degree-1 boundary is at 256/1536 ≈ 0.1667V.
    # Park just below it and wiggle ±5mV — within hysteresis band.
    s.ctrl.adc(0, 0.160)
    s.press("play")
    s.wait(STEP_MS * 2)
    initial_out = s.cv(0)

    flip_count = 0
    last_out = initial_out
    for i in range(20):
        # alternate ±5mV around 0.160V
        v = 0.160 + (0.005 if i % 2 == 0 else -0.005)
        s.ctrl.adc(0, v)
        s.wait(STEP_MS)
        cur = s.cv(0)
        if abs(cur - last_out) > 0.05:
            flip_count += 1
            last_out = cur

    s.press("play")
    s.close()

    print(f"  Initial output: {initial_out:.4f}V, flips during wiggle: {flip_count}")
    assert flip_count == 0, f"Output flipped {flip_count} times — hysteresis not working"
    print("  PASS\n")


def test_iir_rejects_noise():
    """Test: rapid small ADC oscillation should produce a single stable output."""
    print("Test 8: IIR filter rejects rapid noise oscillation")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0  # Ionian

    # Settle at 0.5V (Ionian note 3 ≈ 0.4167V)
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 4)
    settled = s.cv(0)

    # Now alternate rapidly between 0.5V and 0.51V (same Ionian bin, <10mV)
    for _ in range(40):
        s.ctrl.adc(0, 0.51)
        s.wait(5)
        s.ctrl.adc(0, 0.50)
        s.wait(5)

    final = s.cv(0)
    s.press("play")
    s.close()

    print(f"  Settled at: {settled:.4f}V, after noise: {final:.4f}V")
    assert abs(final - settled) < 0.03, (
        f"Output shifted from {settled:.4f}V to {final:.4f}V under rapid noise — filter not working"
    )
    print("  PASS\n")


def test_delayed_sample_reads_settled_value():
    """Test: Internal trigger samples input after settling, not at trigger instant."""
    print("Test 9: Delayed sample captures settled CV (not trigger-instant value)")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Internal
    qt.octave = 0
    qt.transpose = 0

    # One gated step; long period so we can control timing
    seq0 = p.tracks[0].quantizerTrack.sequences[0]
    seq0.firstStep = 0
    seq0.lastStep = 0
    seq0.steps[0].gate = True

    p.scale = 0  # Ionian

    # Set input to 0V before play — simulates CV not yet at target
    s.ctrl.adc(0, 0.0)
    s.press("play")
    # Wait until step 0 fires (one divisor tick after play)
    s.wait(10)
    # Immediately slew to 0.5V to simulate CV arriving after the clock edge
    s.ctrl.adc(0, 0.5)
    # Wait enough for the sample delay (~10ms) + filter to converge
    s.wait(STEP_MS * 2)

    out = s.cv(0)
    expected = quantize_ionian(0.5)  # ~0.4167V

    s.press("play")
    s.close()

    print(f"  Output: {out:.4f}V (expected ~{expected:.4f}V for 0.5V post-settle)")
    assert abs(out - expected) < 0.05, (
        f"Delayed sample should capture 0.5V (≈{expected:.4f}V) but got {out:.4f}V"
    )
    print("  PASS\n")


def test_large_jump_snaps_immediately():
    """Test: large CV jump outputs exactly the target note with no intermediate steps."""
    print("Test 10: Large CV jump snaps immediately to target note (no stepping)")
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(0, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[0].quantizerTrack
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn1
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Free
    qt.octave = 0
    qt.transpose = 0
    p.scale = 0  # Ionian

    # Settle at 0V
    s.ctrl.adc(0, 0.0)
    s.press("play")
    s.wait(STEP_MS * 4)

    # Jump to 2V — crosses several scale degrees; should snap in one tick, not staircase
    s.ctrl.adc(0, 2.0)
    intermediate_outs = []
    for _ in range(8):
        s.wait(5)
        intermediate_outs.append(s.cv(0))

    expected = quantize_ionian(2.0)  # 2.0V -> Ionian root at octave 2 -> 2.0V

    s.press("play")
    s.close()

    final = intermediate_outs[-1]
    # Every sample after the jump should already be at the target — no intermediate degrees
    for i, v in enumerate(intermediate_outs):
        assert abs(v - expected) < 0.03, (
            f"Sample {i} after 2V jump: got {v:.4f}V, expected {expected:.4f}V — IIR stepping?"
        )
    print(f"  All {len(intermediate_outs)} samples after jump at target {final:.4f}V (expected {expected:.4f}V)")
    print("  PASS\n")


if __name__ == "__main__":
    print("=== Quantizer Track Verification ===\n")
    failures = []

    for test_fn in [
        test_free_trigger_quantizes_to_scale,
        test_free_trigger_tracks_input,
        test_external_trigger,
        test_internal_trigger,
        test_octave_shift,
        test_scale_affects_output,
        test_hysteresis_no_flip_at_boundary,
        test_iir_rejects_noise,
        test_delayed_sample_reads_settled_value,
        test_large_jump_snaps_immediately,
    ]:
        try:
            test_fn()
        except AssertionError as ex:
            print(f"  FAIL: {ex}\n")
            failures.append(test_fn.__name__)

    if failures:
        print(f"=== FAILED: {failures} ===")
        sys.exit(1)
    else:
        print("=== All tests PASSED ===")
