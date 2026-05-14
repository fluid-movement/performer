"""
Matrix tests for QuantizerTrack: all input sources × all trigger modes.

Coverage strategy:
  - All 4 CvIn channels verified (identical code path, different ADC index)
  - TrackCV tested on both sides of tick-order asymmetry:
      source_index < quantizer_index → current-frame CV (track already ticked)
      source_index > quantizer_index → last-frame CV (track hasn't ticked yet)
  - All 3 trigger modes exercised against each input source category
  - Edge cases: track provides both CV input AND external trigger,
    External trigger from low-index vs high-index track,
    TrackCV self-exclusion
"""

import sys
import gc
sys.path.insert(0, 'src/apps/sequencer/tests')
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')

from agent import Session
import testsim.sequencer as tsseq

STEP_MS = 125

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
# Parametric helpers
# ---------------------------------------------------------------------------

def setup_quantizer(p, track_idx, input_source, trigger_mode, trigger_track=1):
    """Configure track_idx as a Quantizer with given input/trigger."""
    p.setTrackMode(track_idx, tsseq.Track.TrackMode.Quantizer)
    qt = p.tracks[track_idx].quantizerTrack
    qt.inputSource = input_source
    qt.triggerMode = trigger_mode
    if trigger_mode == tsseq.QuantizerTrack.TriggerMode.External:
        qt.triggerTrack = trigger_track
    qt.octave = 0
    qt.transpose = 0

    if trigger_mode == tsseq.QuantizerTrack.TriggerMode.Internal:
        seq = p.tracks[track_idx].quantizerTrack.sequences[0]
        seq.firstStep = 0
        seq.lastStep = 3
        for i in range(4):
            seq.steps[i].gate = True

    p.scale = 0  # Ionian
    return qt


def setup_note_track_gate_source(p, track_idx):
    """Configure track_idx as a Note track that produces regular gate pulses."""
    p.setTrackMode(track_idx, tsseq.Track.TrackMode.Note)
    seq = p.tracks[track_idx].noteTrack.sequences[0]
    seq.scale = 0
    seq.firstStep = 0
    seq.lastStep = 3
    for i in range(4):
        seq.steps[i].gate = True
        seq.steps[i].note = 0


def run_cvin_free(s, adc_channel, input_source_enum, test_volts=0.5):
    """Free mode: set ADC channel, verify output quantizes correctly."""
    s.ctrl.adc(adc_channel, test_volts)
    s.wait(STEP_MS * 4)
    out = s.cv(0)
    expected = quantize_ionian(test_volts)
    assert abs(out - expected) < 0.03, (
        f"CvIn ch{adc_channel} Free: {test_volts}V -> {out:.4f}V, expected {expected:.4f}V"
    )
    return out, expected


# ---------------------------------------------------------------------------
# Section 1: All CvIn channels, all trigger modes
# ---------------------------------------------------------------------------

def test_cvin_all_channels_free():
    """All 4 CvIn channels produce correct output in Free mode."""
    print("Matrix 1: CvIn1–4 × Free — all channels read correctly")
    SOURCES = [
        (0, tsseq.QuantizerTrack.InputSource.CvIn1),
        (1, tsseq.QuantizerTrack.InputSource.CvIn2),
        (2, tsseq.QuantizerTrack.InputSource.CvIn3),
        (3, tsseq.QuantizerTrack.InputSource.CvIn4),
    ]
    for adc_ch, src_enum in SOURCES:
        s = Session()
        p = s.env.sequencer.model.project
        setup_quantizer(p, 0, src_enum, tsseq.QuantizerTrack.TriggerMode.Free)
        s.press("play")
        out, expected = run_cvin_free(s, adc_ch, src_enum, test_volts=0.5)
        s.press("play")
        s.close()
        del s, p; gc.collect()  # must destroy before creating next sim instance
        print(f"  CvIn{adc_ch+1}: 0.5V -> {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_cvin1_internal():
    """CvIn1 × Internal — samples ADC at gated step, not at arbitrary tick."""
    print("Matrix 2: CvIn1 × Internal")
    s = Session()
    p = s.env.sequencer.model.project
    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                    tsseq.QuantizerTrack.TriggerMode.Internal)
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(0)
    expected = quantize_ionian(0.5)
    s.press("play")
    s.close()
    assert abs(out - expected) < 0.03, f"CvIn1 Internal: {out:.4f}V != {expected:.4f}V"
    print(f"  Output {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_cvin1_external():
    """CvIn1 × External — samples ADC on rising gate from track 1."""
    print("Matrix 3: CvIn1 × External (trigger from track 1)")
    s = Session()
    p = s.env.sequencer.model.project
    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                    tsseq.QuantizerTrack.TriggerMode.External, trigger_track=1)
    setup_note_track_gate_source(p, 1)
    s.ctrl.adc(0, 0.25)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(0)
    expected = quantize_ionian(0.25)
    s.press("play")
    s.close()
    assert abs(out - expected) < 0.03, f"CvIn1 External: {out:.4f}V != {expected:.4f}V"
    print(f"  Output {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_cvin_channels_independent():
    """Different CvIn channels are independent — setting ch2 doesn't affect ch0."""
    print("Matrix 4: CvIn channels are independent (cross-channel isolation)")
    s = Session()
    p = s.env.sequencer.model.project
    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                    tsseq.QuantizerTrack.TriggerMode.Free)
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 3)
    # Set OTHER channels to very different voltages — should not affect ch0 output
    s.ctrl.adc(1, 2.0)
    s.ctrl.adc(2, -1.0)
    s.ctrl.adc(3, 3.5)
    s.wait(STEP_MS * 3)
    out = s.cv(0)
    s.press("play")
    s.close()
    expected = quantize_ionian(0.5)
    assert abs(out - expected) < 0.03, (
        f"Ch0 output {out:.4f}V changed due to other channels — not isolated!"
    )
    print(f"  Ch0 stable at {out:.4f}V despite ch1/2/3 set to different values OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Section 2: TrackCV input, all trigger modes
# ---------------------------------------------------------------------------

def _setup_cv_source_track(p, track_idx, note_degree):
    """Setup track_idx as a Note track holding a specific Ionian note degree."""
    p.setTrackMode(track_idx, tsseq.Track.TrackMode.Note)
    seq = p.tracks[track_idx].noteTrack.sequences[0]
    seq.scale = 0   # Ionian
    seq.firstStep = 0
    seq.lastStep = 0
    seq.steps[0].gate = True
    seq.steps[0].note = note_degree


def test_track_cv_source_free_higher_index():
    """TrackCV × Free: source track index > quantizer index (last-frame CV)."""
    print("Matrix 5: TrackCV (track 7) × Free — source has higher index than quantizer")
    # Quantizer on track 0 reads track 7's CV (last-frame due to tick order 0→7)
    s = Session()
    p = s.env.sequencer.model.project
    _setup_cv_source_track(p, 7, 3)   # track 7 outputs Ionian degree 3 = 640/1536 ≈ 0.4167V
    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.Track8,
                    tsseq.QuantizerTrack.TriggerMode.Free)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(0)
    s.press("play")
    s.close()
    expected = IONIAN_VOLTS[3]
    assert abs(out - expected) < 0.05, (
        f"Track 7→0 Free: got {out:.4f}V, expected {expected:.4f}V (Ionian deg 3)"
    )
    print(f"  Track 7 (deg 3={expected:.4f}V) -> quantizer track 0 output {out:.4f}V OK")
    print("  PASS\n")


def test_track_cv_source_free_lower_index():
    """TrackCV × Free: source track index < quantizer index (current-frame CV)."""
    print("Matrix 6: TrackCV (track 0) × Free — source has lower index than quantizer")
    # Quantizer on track 7 reads track 0's CV (current-frame, already ticked)
    s = Session()
    p = s.env.sequencer.model.project
    _setup_cv_source_track(p, 0, 5)   # track 0 outputs Ionian degree 5 = 1152/1536 ≈ 0.75V
    setup_quantizer(p, 7, tsseq.QuantizerTrack.InputSource.Track1,
                    tsseq.QuantizerTrack.TriggerMode.Free)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(7)
    s.press("play")
    s.close()
    expected = IONIAN_VOLTS[5]
    assert abs(out - expected) < 0.05, (
        f"Track 0→7 Free: got {out:.4f}V, expected {expected:.4f}V (Ionian deg 5)"
    )
    print(f"  Track 0 (deg 5={expected:.4f}V) -> quantizer track 7 output {out:.4f}V OK")
    print("  PASS\n")


def test_track_cv_source_internal():
    """TrackCV × Internal — quantizer on track 3 reads track 5, internal trigger."""
    print("Matrix 7: TrackCV (track 5) × Internal (quantizer on track 3)")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_cv_source_track(p, 5, 2)   # Ionian degree 2 = 512/1536 ≈ 0.3333V
    setup_quantizer(p, 3, tsseq.QuantizerTrack.InputSource.Track6,
                    tsseq.QuantizerTrack.TriggerMode.Internal)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(3)
    s.press("play")
    s.close()
    expected = IONIAN_VOLTS[2]
    assert abs(out - expected) < 0.05, (
        f"Track 5→3 Internal: got {out:.4f}V, expected {expected:.4f}V"
    )
    print(f"  Track 5 (deg 2={expected:.4f}V) -> quantizer track 3 output {out:.4f}V OK")
    print("  PASS\n")


def test_track_cv_source_external():
    """TrackCV × External — input from track 5, trigger from track 1."""
    print("Matrix 8: TrackCV (track 5) × External (trigger from track 1)")
    s = Session()
    p = s.env.sequencer.model.project
    _setup_cv_source_track(p, 5, 4)   # Ionian degree 4 = 896/1536 ≈ 0.5833V
    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.Track6,
                    tsseq.QuantizerTrack.TriggerMode.External, trigger_track=1)
    setup_note_track_gate_source(p, 1)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(0)
    s.press("play")
    s.close()
    expected = IONIAN_VOLTS[4]
    assert abs(out - expected) < 0.05, (
        f"Track 5 input + Track 1 trigger: got {out:.4f}V, expected {expected:.4f}V"
    )
    print(f"  Track 5 (deg 4={expected:.4f}V) -> quantizer output {out:.4f}V OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Section 3: Same track as input AND trigger
# ---------------------------------------------------------------------------

def test_same_track_as_input_and_external_trigger():
    """TrackCV × External where input source and trigger are the SAME track."""
    print("Matrix 9: Same track provides both CV input and External trigger")
    # Track 1 = Note track: its cvOutput is the pitch, its gateOutput is the trigger
    # Quantizer on track 0 reads track 1's CV AND triggers on track 1's gate
    s = Session()
    p = s.env.sequencer.model.project

    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.scale = 0
    seq1.firstStep = 0
    seq1.lastStep = 3
    for i in range(4):
        seq1.steps[i].gate = True
        seq1.steps[i].note = 3   # Ionian degree 3 every step

    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.Track2,
                    tsseq.QuantizerTrack.TriggerMode.External, trigger_track=1)

    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(0)
    s.press("play")
    s.close()

    expected = IONIAN_VOLTS[3]   # degree 3 = 640/1536
    assert abs(out - expected) < 0.05, (
        f"Same track CV+trigger: got {out:.4f}V, expected {expected:.4f}V"
    )
    print(f"  Track 1 as both input and trigger: output {out:.4f}V OK")
    print("  PASS\n")


def test_same_track_varying_notes():
    """Same-track input+trigger: quantizer tracks note changes across steps."""
    print("Matrix 10: Same track CV+trigger — note changes each step are tracked")
    s = Session()
    p = s.env.sequencer.model.project

    # Track 1 alternates between degree 0 and degree 6 every step
    p.setTrackMode(1, tsseq.Track.TrackMode.Note)
    seq1 = p.tracks[1].noteTrack.sequences[0]
    seq1.scale = 0
    seq1.firstStep = 0
    seq1.lastStep = 1
    seq1.steps[0].gate = True
    seq1.steps[0].note = 0   # 0V
    seq1.steps[1].gate = True
    seq1.steps[1].note = 6   # Ionian deg 6 ≈ 0.9167V

    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.Track2,
                    tsseq.QuantizerTrack.TriggerMode.External, trigger_track=1)

    s.press("play")
    # Sample quantizer output over 2 bars — should alternate between two distinct notes
    outputs = []
    for _ in range(24):
        s.wait(STEP_MS // 2)
        outputs.append(s.cv(0))

    s.press("play")
    s.close()

    # Check directly against raw outputs with tolerance (avoids DAC offset filtering issues)
    note0_seen = any(abs(v - IONIAN_VOLTS[0]) < 0.05 for v in outputs)
    note6_seen = any(abs(v - IONIAN_VOLTS[6]) < 0.05 for v in outputs)
    unique_notes = sorted(set(round(v, 2) for v in outputs))

    assert note0_seen and note6_seen, (
        f"Expected both note 0 ({IONIAN_VOLTS[0]:.4f}V) and note 6 ({IONIAN_VOLTS[6]:.4f}V) "
        f"in output. Saw: {sorted(unique_notes)}"
    )
    print(f"  Both note 0 and note 6 observed in output: {sorted(unique_notes)}")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Section 4: External trigger tick-order asymmetry
# ---------------------------------------------------------------------------

def test_external_trigger_from_higher_index_track():
    """External trigger from track with higher index than quantizer (last-frame gate)."""
    print("Matrix 11: External trigger from higher-index track (last-frame gate detection)")
    # Quantizer on track 0, trigger from track 7 (last-frame gate due to tick order)
    s = Session()
    p = s.env.sequencer.model.project
    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                    tsseq.QuantizerTrack.TriggerMode.External, trigger_track=7)
    setup_note_track_gate_source(p, 7)
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(0)
    s.press("play")
    s.close()
    expected = quantize_ionian(0.5)
    assert abs(out - expected) < 0.03, (
        f"External from track 7 (high index): {out:.4f}V != {expected:.4f}V — rising edge missed?"
    )
    print(f"  Trigger from track 7: output {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_external_trigger_from_lower_index_track():
    """External trigger from track with lower index than quantizer (current-frame gate)."""
    print("Matrix 12: External trigger from lower-index track (current-frame gate detection)")
    # Quantizer on track 7, trigger from track 0 (current-frame gate)
    s = Session()
    p = s.env.sequencer.model.project
    setup_quantizer(p, 7, tsseq.QuantizerTrack.InputSource.CvIn1,
                    tsseq.QuantizerTrack.TriggerMode.External, trigger_track=0)
    setup_note_track_gate_source(p, 0)
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = s.cv(7)
    s.press("play")
    s.close()
    expected = quantize_ionian(0.5)
    assert abs(out - expected) < 0.03, (
        f"External from track 0 (low index): {out:.4f}V != {expected:.4f}V"
    )
    print(f"  Trigger from track 0: output {out:.4f}V (expected {expected:.4f}V) OK")
    print("  PASS\n")


def test_external_trigger_timing_parity():
    """Both tick-order directions must produce the same output value (timing parity)."""
    print("Matrix 13: External trigger tick-order parity — high-index vs low-index trigger")
    results = {}
    for q_idx, trig_idx, label in [(0, 7, "q0<-trig7"), (7, 0, "q7<-trig0")]:
        s = Session()
        p = s.env.sequencer.model.project
        setup_quantizer(p, q_idx, tsseq.QuantizerTrack.InputSource.CvIn1,
                        tsseq.QuantizerTrack.TriggerMode.External, trigger_track=trig_idx)
        setup_note_track_gate_source(p, trig_idx)
        s.ctrl.adc(0, 0.5)
        s.press("play")
        s.wait(STEP_MS * 8)
        results[label] = s.cv(q_idx)
        s.press("play")
        s.close()
        del s, p; gc.collect()  # must destroy before creating next sim instance

    expected = quantize_ionian(0.5)
    for label, out in results.items():
        assert abs(out - expected) < 0.03, (
            f"{label}: {out:.4f}V != {expected:.4f}V"
        )
        print(f"  {label}: {out:.4f}V OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Section 5: TrackCV self-exclusion
# ---------------------------------------------------------------------------

def test_track_self_exclusion_returns_zero():
    """Input source = own track index must return 0V (guard in readInput)."""
    print("Matrix 14: TrackCV self-exclusion — own track as input returns 0V")
    s = Session()
    p = s.env.sequencer.model.project
    # Track 0 tries to read its own CV output — should get 0V
    setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.Track1,
                    tsseq.QuantizerTrack.TriggerMode.Free)
    s.press("play")
    s.wait(STEP_MS * 4)
    out = s.cv(0)
    s.press("play")
    s.close()
    # With 0V input → Ionian note 0 → 0V output
    assert abs(out) < 0.03, (
        f"Self-exclusion failed: own track as input produced {out:.4f}V (should be ~0V)"
    )
    print(f"  Own track as input -> 0V input -> {out:.4f}V output OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Section 6: Note-value correctness across all input-source × mode combos
# ---------------------------------------------------------------------------

def _quick_note_check(input_source_enum, trigger_mode_enum,
                      setup_extra_fn, get_cv_fn, label, test_volts=0.5):
    """
    Minimal correctness check: set up, play, check output ≈ quantize_ionian(test_volts).
    setup_extra_fn(p, s) called after quantizer setup but before play (e.g. setup gate source).
    get_cv_fn(s) returns the relevant cv output.
    """
    s = Session()
    p = s.env.sequencer.model.project
    qt = setup_quantizer(p, 0, input_source_enum, trigger_mode_enum, trigger_track=1)

    if setup_extra_fn:
        setup_extra_fn(p, s)

    s.ctrl.adc(0, test_volts)
    s.press("play")
    s.wait(STEP_MS * 8)
    out = get_cv_fn(s)
    s.press("play")
    s.close()

    expected = quantize_ionian(test_volts)
    ok = abs(out - expected) < 0.03
    return ok, out, expected, label


def test_all_cvin_all_modes_note_accuracy():
    """
    Full correctness sweep: CvIn1–4 × Free/Internal/External.
    12 combinations, all must quantize 0.5V to Ionian degree 3.
    Reuses one Session to avoid memory exhaustion from 12 sim instances.
    """
    print("Matrix 15: CvIn1–4 × Free/Internal/External — note accuracy sweep (12 combos)")

    cvin_sources = [
        ("CvIn1", tsseq.QuantizerTrack.InputSource.CvIn1, 0),
        ("CvIn2", tsseq.QuantizerTrack.InputSource.CvIn2, 1),
        ("CvIn3", tsseq.QuantizerTrack.InputSource.CvIn3, 2),
        ("CvIn4", tsseq.QuantizerTrack.InputSource.CvIn4, 3),
    ]
    trigger_modes = [
        ("Free",     tsseq.QuantizerTrack.TriggerMode.Free),
        ("Internal", tsseq.QuantizerTrack.TriggerMode.Internal),
        ("External", tsseq.QuantizerTrack.TriggerMode.External),
    ]

    s = Session()
    p = s.env.sequencer.model.project
    failures = []
    expected = quantize_ionian(0.5)

    for src_label, src_enum, adc_ch in cvin_sources:
        for mode_label, mode_enum in trigger_modes:
            # Reconfigure without creating a new session
            setup_quantizer(p, 0, src_enum, mode_enum, trigger_track=1)
            if mode_enum == tsseq.QuantizerTrack.TriggerMode.External:
                setup_note_track_gate_source(p, 1)
            s.ctrl.adc(adc_ch, 0.5)
            s.press("play")
            s.wait(STEP_MS * 6)
            out = s.cv(0)
            s.press("play")
            s.wait(50)  # brief settle between combos

            if abs(out - expected) < 0.03:
                print(f"  {src_label} × {mode_label}: {out:.4f}V OK")
            else:
                msg = f"{src_label} × {mode_label}: {out:.4f}V != {expected:.4f}V"
                print(f"  FAIL: {msg}")
                failures.append(msg)

    s.close()
    assert not failures, f"{len(failures)} combos failed:\n" + "\n".join(failures)
    print("  PASS\n")


def test_all_track_sources_all_modes_note_accuracy():
    """
    TrackCV sources × Free/Internal/External.
    Track2, Track5, Track8 × all 3 modes = 9 combos.
    Reuses one Session per track-source group.
    """
    print("Matrix 16: TrackCV × Free/Internal/External — note accuracy (9 combos)")

    # (input_track_idx, InputSource_enum, degree, expected_volts)
    track_cases = [
        (1, tsseq.QuantizerTrack.InputSource.Track2, 3, IONIAN_VOLTS[3]),
        (4, tsseq.QuantizerTrack.InputSource.Track5, 5, IONIAN_VOLTS[5]),
        (7, tsseq.QuantizerTrack.InputSource.Track8, 1, IONIAN_VOLTS[1]),
    ]
    trigger_modes = [
        ("Free",     tsseq.QuantizerTrack.TriggerMode.Free),
        ("Internal", tsseq.QuantizerTrack.TriggerMode.Internal),
        ("External", tsseq.QuantizerTrack.TriggerMode.External),
    ]

    failures = []
    for src_track_idx, src_enum, degree, expected in track_cases:
        # One session per source track; reconfigure trigger mode within it
        s = Session()
        p = s.env.sequencer.model.project
        _setup_cv_source_track(p, src_track_idx, degree)

        for mode_label, mode_enum in trigger_modes:
            setup_quantizer(p, 0, src_enum, mode_enum, trigger_track=2)
            if mode_enum == tsseq.QuantizerTrack.TriggerMode.External:
                setup_note_track_gate_source(p, 2)
            s.press("play")
            s.wait(STEP_MS * 6)
            out = s.cv(0)
            s.press("play")
            s.wait(50)

            label = f"Track{src_track_idx+1} × {mode_label}"
            if abs(out - expected) < 0.05:
                print(f"  {label}: {out:.4f}V OK")
            else:
                msg = f"{label}: {out:.4f}V != {expected:.4f}V"
                print(f"  FAIL: {msg}")
                failures.append(msg)

        s.close()
        del s, p; gc.collect()  # must destroy before creating next sim instance

    assert not failures, f"{len(failures)} combos failed:\n" + "\n".join(failures)
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Section 7: Input source switching mid-play
# ---------------------------------------------------------------------------

def test_input_source_switch_mid_play():
    """Switching input source while playing immediately reads new source."""
    print("Matrix 17: Input source switch mid-play — output follows new source")
    s = Session()
    p = s.env.sequencer.model.project
    qt = setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                         tsseq.QuantizerTrack.TriggerMode.Free)

    # Start: CvIn1 at 0.5V
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_cv = s.cv(0)
    expected_cv = quantize_ionian(0.5)

    # Setup CvIn2 at 1.0V, then switch source
    s.ctrl.adc(1, 1.0)
    s.wait(STEP_MS * 2)
    qt.inputSource = tsseq.QuantizerTrack.InputSource.CvIn2
    s.wait(STEP_MS * 4)
    out_after = s.cv(0)
    expected_after = quantize_ionian(1.0)

    s.press("play")
    s.close()

    assert abs(out_cv - expected_cv) < 0.03, f"Before switch: {out_cv:.4f}V"
    assert abs(out_after - expected_after) < 0.03, (
        f"After source switch to CvIn2: {out_after:.4f}V != {expected_after:.4f}V"
    )
    print(f"  CvIn1@0.5V -> {out_cv:.4f}V, then switch to CvIn2@1.0V -> {out_after:.4f}V OK")
    print("  PASS\n")


def test_trigger_mode_switch_mid_play():
    """Switching trigger mode while playing doesn't corrupt state."""
    print("Matrix 18: Trigger mode switch mid-play — no corrupted state")
    s = Session()
    p = s.env.sequencer.model.project
    qt = setup_quantizer(p, 0, tsseq.QuantizerTrack.InputSource.CvIn1,
                         tsseq.QuantizerTrack.TriggerMode.Free)
    s.ctrl.adc(0, 0.5)
    s.press("play")
    s.wait(STEP_MS * 4)
    out_free = s.cv(0)

    # Switch to Internal
    qt.triggerMode = tsseq.QuantizerTrack.TriggerMode.Internal
    s.wait(STEP_MS * 8)
    out_internal = s.cv(0)

    s.press("play")
    s.close()

    expected = quantize_ionian(0.5)
    assert abs(out_free - expected) < 0.03, f"Free mode output wrong: {out_free:.4f}V"
    assert abs(out_internal - expected) < 0.03, (
        f"After mode switch to Internal: {out_internal:.4f}V != {expected:.4f}V"
    )
    print(f"  Free: {out_free:.4f}V, switched to Internal: {out_internal:.4f}V OK")
    print("  PASS\n")


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

ALL_TESTS = [
    test_cvin_all_channels_free,
    test_cvin1_internal,
    test_cvin1_external,
    test_cvin_channels_independent,
    test_track_cv_source_free_higher_index,
    test_track_cv_source_free_lower_index,
    test_track_cv_source_internal,
    test_track_cv_source_external,
    test_same_track_as_input_and_external_trigger,
    test_same_track_varying_notes,
    test_external_trigger_from_higher_index_track,
    test_external_trigger_from_lower_index_track,
    test_external_trigger_timing_parity,
    test_track_self_exclusion_returns_zero,
    test_all_cvin_all_modes_note_accuracy,
    test_all_track_sources_all_modes_note_accuracy,
    test_input_source_switch_mid_play,
    test_trigger_mode_switch_mid_play,
]

if __name__ == "__main__":
    print("=== Quantizer Matrix Tests ===\n")
    failures = []
    for test_fn in ALL_TESTS:
        try:
            test_fn()
        except (AssertionError, Exception) as ex:
            print(f"  FAIL: {ex}\n")
            failures.append((test_fn.__name__, str(ex)))
        gc.collect()  # release C++ sim objects immediately between tests

    print(f"\n{'=' * 45}")
    if failures:
        print(f"FAILED {len(failures)}/{len(ALL_TESTS)}:")
        for name, msg in failures:
            print(f"  {name}: {msg}")
        sys.exit(1)
    else:
        print(f"All {len(ALL_TESTS)} matrix tests PASSED")
