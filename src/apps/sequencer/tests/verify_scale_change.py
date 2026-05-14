"""
Verify that switching scale does NOT remap stored step degrees.
Specific regression: degrees 1 and 2 must not snap to 1 when switching
from a flat-2 mode (Phrygian, HM: Phryg Dom) to a natural-2 mode (Ionian, Lydian).

Run from project root: python src/apps/sequencer/tests/verify_scale_change.py
"""
import sys
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')

from agent import Session
import testsim.sequencer as tsseq

NUM_SCALES = 21

s = Session()
p = s.env.sequencer.model.project
p.setTrackMode(0, tsseq.Track.TrackMode.Note)
seq = p.tracks[0].noteTrack.sequences[0]
seq.rootNote = 0  # C
seq.firstStep = 0
seq.lastStep = 6

# Start in HM: Phryg Dom (index 11) — has a flat 2 (1 semitone).
# This is the mode where the bug was most visible.
seq.scale = 11
for i in range(7):
    seq.steps[i].gate = True
    seq.steps[i].note = i  # degrees 0..6

# Confirm setup
for i in range(7):
    assert seq.steps[i].note == i, f"Setup: step {i} note should be {i}"

print(f"Starting in HM: Phryg Dom — steps set to degrees 0-6")

# Switch through every scale and verify notes are unchanged
for scale_idx in range(NUM_SCALES):
    seq.scale = scale_idx
    for i in range(7):
        actual = seq.steps[i].note
        if actual != i:
            print(f"FAIL  scale {scale_idx}: step {i} expected degree {i}, got {actual}")
            sys.exit(1)

print(f"OK  degrees unchanged across all {NUM_SCALES} scales")

# Sanity check: pitch DOES change (proves the scale actually switched)
# Degree 1 in HM: Phryg Dom = 1 semitone = 1/12 V
# Degree 1 in Maj: Ionian = 2 semitones = 2/12 V
seq.firstStep = 0
seq.lastStep = 0
seq.steps[0].gate = True
seq.steps[0].note = 1   # degree 1

seq.scale = 11   # HM: Phryg Dom
s.press("play").wait(400)
cv_phrygian_d2 = s.cv(0)
s.press("play")

seq.scale = 0    # Maj: Ionian
s.press("play").wait(400)
cv_ionian_d2 = s.cv(0)
s.press("play")

assert abs(cv_phrygian_d2 - 1/12.0) < 0.01, f"HM: Phryg Dom d2 CV: expected {1/12:.4f}V got {cv_phrygian_d2:.4f}V"
assert abs(cv_ionian_d2 - 2/12.0) < 0.01, f"Maj: Ionian d2 CV: expected {2/12:.4f}V got {cv_ionian_d2:.4f}V"
print(f"OK  pitch changes correctly: Phryg Dom d2={cv_phrygian_d2:.4f}V, Ionian d2={cv_ionian_d2:.4f}V")

print("\nAll scale-change checks passed.")
