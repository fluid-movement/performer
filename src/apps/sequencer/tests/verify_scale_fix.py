"""Verify that scale conversion preserves pitch when switching from Semitones to Major.

notes [0, 12, 24, 12] = C4, C5, C6, C5 in Semitones (12-degree).
After converting to Major (7-degree), they should become [0, 7, 14, 7] = same pitches.
CV readings during playback should match in both scales.
"""
import sys, os, faulthandler
faulthandler.enable()
sys.path.insert(0, os.path.join(os.path.dirname(__file__),
    "../../../../build/sim/release/src/apps/sequencer/python"))

from testsim import Environment
from testframework.controller import Controller

print("=== booting ===", flush=True)
e = Environment()
c = Controller(e.simulator)
c.wait(3000)

p = e.sequencer.model.project
import testsim.sequencer as tsseq
p.setTrackMode(0, tsseq.Track.TrackMode.Note)
c.wait(100)

seq = p.tracks[0].noteTrack.sequences[0]
seq.firstStep = 0
seq.lastStep = 3
seq.scale = 0  # Semitones

for i, n in enumerate([0, 12, 24, 12]):
    seq.steps[i].gate = True
    seq.steps[i].note = n

print("Notes before scale change:", [seq.steps[i].note for i in range(4)], flush=True)

# --- Play in Semitones, sample CV ---
c.press("play")
c.wait(200)

sem_cvs = []
for _ in range(20):
    e.simulator.wait(100)
    cv = e.simulator.targetState.dac.volts(0)
    gate = bool(e.simulator.targetState.gateOutput[0])
    if gate:
        sem_cvs.append(round(cv, 3))

print(f"Semitones CVs (gated): {sorted(set(sem_cvs))}", flush=True)

# --- Switch to Major mid-playback ---
seq.scale = 1  # Major

print("Notes after scale change:", [seq.steps[i].note for i in range(4)], flush=True)

major_cvs = []
for _ in range(20):
    e.simulator.wait(100)
    cv = e.simulator.targetState.dac.volts(0)
    gate = bool(e.simulator.targetState.gateOutput[0])
    if gate:
        major_cvs.append(round(cv, 3))

print(f"Major CVs (gated):     {sorted(set(major_cvs))}", flush=True)

# --- Stop ---
c.press("play")

sem_set = set(sem_cvs)
major_set = set(major_cvs)
print(f"\nSemitones unique CVs: {sem_set}", flush=True)
print(f"Major unique CVs:     {major_set}", flush=True)

if sem_set == major_set:
    print("\n✓ PASS: CVs match — scale conversion preserves pitch", flush=True)
elif not major_cvs:
    print("\n? No gated notes sampled in Major — try increasing sample count", flush=True)
else:
    print("\n✗ FAIL: CVs differ — scale conversion is broken", flush=True)
    print(f"  Semitones: {sem_set}", flush=True)
    print(f"  Major:     {major_set}", flush=True)
