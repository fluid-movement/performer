"""Minimal crash diagnostic — pinpoint where SIGSEGV occurs."""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../../../build/sim/release/src/apps/sequencer/python"))

from testsim import Environment
from testframework.controller import Controller

print("Step 1: boot")
e = Environment()
c = Controller(e.simulator)
c.wait(3000)
print("  ok")

print("Step 2: set track mode to Note")
p = e.simulator.project
p.setTrackMode(0, 1)  # 1 = Note
c.wait(200)
print("  ok")

print("Step 3: get sequence reference")
seq = p.tracks[0].noteTrack.sequence(0)
print(f"  seq={seq}")

print("Step 4: set firstStep/lastStep")
seq.firstStep = 0
seq.lastStep = 3
print("  ok")

print("Step 5: set individual steps (no caching)")
seq.steps[0].gate = True; seq.steps[0].note = 0
print("  step 0 ok")
seq.steps[1].gate = True; seq.steps[1].note = 12
print("  step 1 ok")
seq.steps[2].gate = True; seq.steps[2].note = 24
print("  step 2 ok")
seq.steps[3].gate = True; seq.steps[3].note = 12
print("  step 3 ok")

print("Step 6: verify notes before scale change")
for i in range(4):
    print(f"  step[{i}].note = {seq.steps[i].note}")

print("Step 7: set scale to Semitones (0)")
seq.scale = 0
print("  ok")

print("Step 8: start playback")
c.press("play")
c.wait(500)
print("  ok")

print("Step 9: single CV read")
v = e.simulator.targetState.dac.volts(0)
print(f"  cv[0] = {v}")

print("Step 10: 5 quick waits with CV reads")
for i in range(5):
    e.simulator.wait(100)
    v = e.simulator.targetState.dac.volts(0)
    print(f"  [{i}] cv[0] = {v:.3f}")

print("Step 11: switch scale to Major (1)")
seq.scale = 1
print("  ok")

print("Step 12: verify notes after scale change")
for i in range(4):
    print(f"  step[{i}].note = {seq.steps[i].note}")

print("Step 13: 5 more waits with CV reads")
for i in range(5):
    e.simulator.wait(100)
    v = e.simulator.targetState.dac.volts(0)
    print(f"  [{i}] cv[0] = {v:.3f}")

print("ALL DONE — no crash")
