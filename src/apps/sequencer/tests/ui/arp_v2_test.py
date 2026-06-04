"""Arp V2 engine tests — validates euclidean modMode gate logic.

Timing reference:
  wait(N) = N milliseconds of simulated time
  At 120 BPM, CONFIG_PPQN=192, sequence divisor=12:
    1 step = 12 * (192/48) = 48 engine ticks = 125 ms
  rhythmGateLen=6 → rawLen = (48*35)//100 = 16 ticks ≈ 41 ms  (> POLL_MS)

Note: creating multiple Environment() instances in one Python process crashes the
simulator. All tests share a single environment, changing seq params between runs.
"""
import sys
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')

from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

STEP_MS  = 125   # ms per sequence step at 120 BPM / divisor=12
POLL_MS  = 3     # poll interval; must be < gate length (≈41 ms)

# ModMode integer values (ArpSequence::ModMode enum)
MOD_OFF     = 0
MOD_ACCENT  = 1
MOD_MASK    = 2
MOD_COMBINE = 3
MOD_RATCHET = 4
MOD_HOLD    = 5


def compute_euclidean(n, k, r=0):
    """Mirror of ArpTrackEngine::computeEuclidean — canonical form, first hit at index 0."""
    raw = [False] * n
    bucket = 0
    for i in range(n):
        bucket += k
        if bucket >= n:
            bucket -= n
            raw[i] = True
    first_hit = next((i for i, v in enumerate(raw) if v), 0)
    return [raw[((i + first_hit - r) % n + n) % n] for i in range(n)]


def record_cv_at_rises(e, total_ms):
    """Collect the CV value at each gate 0→1 rising edge within total_ms."""
    prev    = bool(e.simulator.targetState.gateOutput[0])
    cvs     = []
    elapsed = 0
    while elapsed < total_ms:
        step = min(POLL_MS, total_ms - elapsed)
        e.simulator.wait(step)
        elapsed += step
        curr = bool(e.simulator.targetState.gateOutput[0])
        if curr and not prev:
            cvs.append(e.simulator.targetState.dac.volts(0))
        prev = curr
    return cvs


def cvs_close(a, b, tol=0.005):
    return len(a) == len(b) and all(abs(x - y) < tol for x, y in zip(a, b))


def count_gate_rises(e, total_ms):
    """Count gate 0→1 rising edges over total_ms milliseconds.

    Rising-edge counting is robust to gate length and step-grid alignment.
    POLL_MS must be shorter than the gate duration.
    """
    prev    = bool(e.simulator.targetState.gateOutput[0])
    rises   = 0
    elapsed = 0
    while elapsed < total_ms:
        step = min(POLL_MS, total_ms - elapsed)
        e.simulator.wait(step)
        elapsed += step
        curr = bool(e.simulator.targetState.gateOutput[0])
        if curr and not prev:
            rises += 1
        prev = curr
    return rises


def set_seq(seq, rn, rk, rr, mn, mk, mr, mod_mode):
    seq.rhythmN = rn; seq.rhythmK = rk; seq.rhythmR = rr
    seq.modN = mn;    seq.modK = mk;    seq.modR = mr
    seq.modMode = mod_mode
    seq.arpLength = max(rn, mn)


def count_for_mode(e, seq, rn, rk, rr, mn, mk, mr, mod_mode, cycles=5):
    """Set mode params, warm up one cycle, then count rising edges over `cycles` cycles."""
    set_seq(seq, rn, rk, rr, mn, mk, mr, mod_mode)
    e.simulator.wait(max(rn, mn) * STEP_MS)          # warmup — let new params take effect
    return count_gate_rises(e, max(rn, mn) * STEP_MS * cycles)


# ---------------------------------------------------------------------------
# Test setup
#   Rhythm E(4,8,0): canonical → hits at indices 0,2,4,6
#   Mod    E(2,8,0): canonical → hits at indices 0,4
#   Overlaps: steps 0, 4
#   OFF: 4 gates/cycle  |  COMBINE: 2  |  MASK: 2
# ---------------------------------------------------------------------------

RN, RK, RR = 8, 4, 0
MN, MK, MR = 8, 2, 0
CYCLES = 5


def run_all_tests(e, seq):

    def test_expected_patterns():
        """Verify Python compute_euclidean matches expected hit positions."""
        rhythm = compute_euclidean(RN, RK, RR)
        mod    = compute_euclidean(MN, MK, MR)
        assert rhythm == [True,False,True,False,True,False,True,False], f"rhythm: {rhythm}"
        assert mod    == [True,False,False,False,True,False,False,False], f"mod: {mod}"
        print("✓ expected_patterns: canonical E(4,8) hits at 0,2,4,6 | E(2,8) hits at 0,4")

    def test_mod_off_mode():
        """modMode=OFF: all rhythm hits fire → 4 gates per cycle."""
        rises = count_for_mode(e, seq, RN, RK, RR, MN, MK, MR, MOD_OFF, CYCLES)
        assert rises == 4 * CYCLES, f"OFF: expected {4*CYCLES}, got {rises}"
        print(f"✓ mod_off_mode: {rises} rises over {CYCLES} cycles (expected {4*CYCLES})")

    def test_mod_combine_mode():
        """modMode=COMBINE: only overlapping steps fire → 2 gates per cycle."""
        rises = count_for_mode(e, seq, RN, RK, RR, MN, MK, MR, MOD_COMBINE, CYCLES)
        assert rises == 2 * CYCLES, f"COMBINE: expected {2*CYCLES}, got {rises}"
        print(f"✓ mod_combine_mode: {rises} rises over {CYCLES} cycles (expected {2*CYCLES})")

    def test_mod_mask_mode():
        """modMode=MASK: non-overlapping rhythm hits fire → 2 gates per cycle."""
        rises = count_for_mode(e, seq, RN, RK, RR, MN, MK, MR, MOD_MASK, CYCLES)
        assert rises == 2 * CYCLES, f"MASK: expected {2*CYCLES}, got {rises}"
        print(f"✓ mod_mask_mode: {rises} rises over {CYCLES} cycles (expected {2*CYCLES})")

    def test_combine_plus_mask_equals_off():
        """COMBINE + MASK = OFF (partition sanity check)."""
        g_comb = count_for_mode(e, seq, RN, RK, RR, MN, MK, MR, MOD_COMBINE, CYCLES)
        g_mask = count_for_mode(e, seq, RN, RK, RR, MN, MK, MR, MOD_MASK,    CYCLES)
        g_off  = count_for_mode(e, seq, RN, RK, RR, MN, MK, MR, MOD_OFF,     CYCLES)
        assert g_comb + g_mask == g_off, f"COMBINE({g_comb})+MASK({g_mask}) != OFF({g_off})"
        print(f"✓ combine_plus_mask_equals_off: {g_comb}+{g_mask}={g_off}")

    def test_mod_combine_different_lengths():
        """COMBINE with polyrhythmic (different N) patterns; overlap count matches prediction."""
        rn, rk, rr = 8, 4, 0
        mn, mk, mr = 6, 3, 0
        rhythm     = compute_euclidean(rn, rk, rr)
        mod6       = compute_euclidean(mn, mk, mr)
        per_cycle  = sum(1 for i in range(rn) if rhythm[i] and mod6[i % mn])
        rises      = count_for_mode(e, seq, rn, rk, rr, mn, mk, mr, MOD_COMBINE, CYCLES)
        expected   = per_cycle * CYCLES
        assert rises == expected, f"COMBINE poly: expected {expected} ({per_cycle}/cycle), got {rises}"
        print(f"✓ mod_combine_different_lengths: {rises} rises over {CYCLES} cycles (expected {expected})")

    def test_arp_orders():
        """Verify each arp order produces the expected note sequence.

        Uses 4 active degrees (degreeMask=0x0F) and all-hits rhythm so every
        step fires.  Chunks start at an arbitrary phase offset (we can't
        guarantee the recording window starts at cycle position 0), so
        correctness is checked via cyclic-rotation matching:
          - Deterministic modes: all chunks of cycle_len CVs are identical
            AND the chunk is a cyclic rotation of the expected degree sequence.
          - RAND: every CV is in the reference set.

        Expected degree sequences (n=4, cyclic):
          UP:    0 1 2 3
          DOWN:  3 2 1 0
          UP-DN: 0 1 2 3 2 1   (period = 2*(n-1) = 6)
          DN-UP: 3 2 1 0 1 2
          CONV:  0 3 1 2       (outside→in)
          DIV:   1 2 0 3       (center→out, mid = (n-1)/2 = 1)
        """
        N = 4
        BOUNCE = 2 * (N - 1)  # 6 — period for UP-DN / DN-UP

        orig_mask  = seq.degreeMask
        orig_order = seq.arpOrder
        orig_rn, orig_rk = seq.rhythmN, seq.rhythmK

        seq.degreeMask    = 0x0F   # degrees 0-3
        seq.rhythmN       = N
        seq.rhythmK       = N      # every step is a hit
        seq.rhythmR       = 0
        seq.modMode       = 0

        def icr(chunk, expected, tol=0.005):
            """Is chunk a cyclic rotation of expected?"""
            n = len(expected)
            if len(chunk) != n:
                return False
            for shift in range(n):
                if all(abs(chunk[j] - expected[(shift + j) % n]) < tol
                       for j in range(n)):
                    return True
            return False

        def get_chunks(order, cycle_len, n_chunks=4):
            """Switch to order, warmup, then collect n_chunks complete cycles."""
            seq.arpOrder  = order
            seq.arpLength = cycle_len
            e.simulator.wait(cycle_len * STEP_MS * 2)   # 2-cycle warmup
            # Record enough to guarantee n_chunks full cycles
            cvs = record_cv_at_rises(e, cycle_len * STEP_MS * (n_chunks + 2))
            assert len(cvs) >= cycle_len * n_chunks, (
                f"order {order}: only {len(cvs)} CVs (need {cycle_len*n_chunks})"
            )
            return [cvs[i*cycle_len:(i+1)*cycle_len] for i in range(n_chunks)]

        # ── UP ──────────────────────────────────────────────────────────────
        chunks = get_chunks(0, N)
        for i in range(1, len(chunks)):
            assert cvs_close(chunks[0], chunks[i]), f"UP: chunk {i} differs"
        cv_map = sorted(chunks[0])   # [cv_deg0, cv_deg1, cv_deg2, cv_deg3]
        assert icr(chunks[0], cv_map), f"UP not cyclic-ascending: {chunks[0]}"
        print(f"  UP ✓  cv_map={[round(v,3) for v in cv_map]}")

        def check(name, order, cycle_len, expected_degrees):
            chunks = get_chunks(order, cycle_len)
            for i in range(1, len(chunks)):
                assert cvs_close(chunks[0], chunks[i]), f"{name}: chunk {i} differs"
            exp = [cv_map[d] for d in expected_degrees]
            assert icr(chunks[0], exp), (
                f"{name}: {[round(v,3) for v in chunks[0]]} "
                f"not a cyclic rotation of {[round(v,3) for v in exp]}"
            )
            print(f"  {name} ✓")

        check("DOWN",  1, N,      [3, 2, 1, 0])
        check("UP-DN", 2, BOUNCE, [0, 1, 2, 3, 2, 1])
        check("DN-UP", 3, BOUNCE, [3, 2, 1, 0, 1, 2])
        check("CONV",  5, N,      [0, 3, 1, 2])
        check("DIV",   6, N,      [1, 2, 0, 3])

        # RAND: all CVs must be members of cv_map; no determinism requirement
        seq.arpOrder  = 4
        seq.arpLength = N
        e.simulator.wait(N * STEP_MS * 2)
        rand_cvs = record_cv_at_rises(e, N * STEP_MS * 6)
        for cv in rand_cvs:
            assert any(abs(cv - r) < 0.01 for r in cv_map), (
                f"RAND: CV {cv:.4f} not in reference set"
            )
        print("  RAND ✓")

        seq.degreeMask = orig_mask
        seq.arpOrder   = orig_order
        seq.rhythmN    = orig_rn
        seq.rhythmK    = orig_rk
        print("✓ test_arp_orders: all 7 modes pass")

    test_expected_patterns()
    test_mod_off_mode()
    test_mod_combine_mode()
    test_mod_mask_mode()
    test_combine_plus_mask_equals_off()
    test_mod_combine_different_lengths()
    test_arp_orders()


if __name__ == "__main__":
    print("Running Arp V2 engine tests...")

    e = Environment()
    c = Controller(e.simulator)
    c.wait(3000)

    p = e.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Arp)
    track = p.tracks[0].arpTrack
    seq   = track.sequences[0]

    seq.degreeMask    = 0x01   # only degree 0 active
    seq.arpOctaves    = 1
    seq.rhythmGateLen = 6      # ≈41 ms gate, well above POLL_MS

    c.press("play")
    e.simulator.wait(RN * STEP_MS)  # initial warmup

    run_all_tests(e, seq)
    print("\nAll tests passed.")
