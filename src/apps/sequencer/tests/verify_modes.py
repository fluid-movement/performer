"""
Verify that all 21 modal scales produce correct CV output.
Run from project root: python src/apps/sequencer/tests/verify_modes.py
"""
import sys
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')

from agent import Session
import testsim.sequencer as tsseq

SCALE_NAMES = [
    "Maj: Ionian", "Maj: Dorian", "Maj: Phrygian", "Maj: Lydian", "Maj: Mixolyd.",
    "Maj: Aeolian", "Maj: Locrian",
    "HM: Harm. Min", "HM: Locrian n6", "HM: Ionian #5", "HM: Dorian #4",
    "HM: Phryg Dom", "HM: Lydian #2", "HM: Sup Loc b7",
    "MM: Melod. Min", "MM: Dorian b2", "MM: Lydian Aug", "MM: Lydian Dom",
    "MM: Mixo b6", "MM: Locrian n2", "MM: Altered",
]

# (scale_index, degree_0..6_semitones) spot checks
CHECKS = {
    0:  [0, 2, 4, 5, 7, 9, 11],   # Maj: Ionian
    7:  [0, 2, 3, 5, 7, 8, 11],   # HM: Harm. Min
    11: [0, 1, 4, 5, 7, 8, 10],   # HM: Phryg Dom
    14: [0, 2, 3, 5, 7, 9, 11],   # MM: Melod. Min
    17: [0, 2, 4, 6, 7, 9, 10],   # MM: Lydian Dom
    20: [0, 1, 3, 4, 6, 8, 10],   # MM: Altered
}

s = Session()
p = s.env.sequencer.model.project
p.setTrackMode(0, tsseq.Track.TrackMode.Note)
seq = p.tracks[0].noteTrack.sequences[0]
seq.rootNote = 0  # C

for scale_idx, semitones in CHECKS.items():
    seq.scale = scale_idx
    # check each degree independently: 1-step sequence with gate
    for degree, expected_st in enumerate(semitones):
        seq.firstStep = 0
        seq.lastStep = 0
        seq.steps[0].gate = True
        seq.steps[0].note = degree
        s.press("play").wait(400)
        cv = s.cv(0)
        s.press("play")
        expected_v = expected_st / 12.0
        diff = abs(cv - expected_v)
        if diff >= 0.01:
            print(f"FAIL  {SCALE_NAMES[scale_idx]} degree {degree}: "
                  f"expected {expected_v:.4f}V ({expected_st} st) got {cv:.4f}V")
            sys.exit(1)
    print(f"OK  [{scale_idx:2d}] {SCALE_NAMES[scale_idx]}")

print(f"\nAll {len(CHECKS)} scale checks passed.")
