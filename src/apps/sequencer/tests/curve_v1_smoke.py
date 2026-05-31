import sys
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')
from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

e = Environment()
c = Controller(e.simulator)
c.wait(3000)

p = e.sequencer.model.project
p.setTrackMode(0, tsseq.Track.TrackMode.Curve)
seq = p.tracks[0].curveTrack.sequences[0]

seq.segmentCount = 3
seq.steps[0].length = 2; seq.steps[0].shapeNorm = 0.1; seq.steps[0].levelNorm = 0.9
seq.steps[1].length = 4; seq.steps[1].shapeNorm = 0.5; seq.steps[1].levelNorm = 0.6
seq.steps[2].length = 2; seq.steps[2].shapeNorm = 0.9; seq.steps[2].levelNorm = 0.8

c.press("play")
e.simulator.wait(500)
print(f"CV={e.simulator.targetState.dac.volts(0):.3f}V  Gate={bool(e.simulator.targetState.gateOutput[0])}")
e.simulator.screenshot("/tmp/curve_v1.png")
print("Screenshot saved to /tmp/curve_v1.png")
