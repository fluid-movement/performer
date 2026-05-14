import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../../../build/sim/release/src/apps/sequencer/python"))
sys.stdout.flush()

print("importing testsim..."); sys.stdout.flush()
from testsim import Environment
print("ok"); sys.stdout.flush()

from testframework.controller import Controller
print("controller imported"); sys.stdout.flush()

print("booting env..."); sys.stdout.flush()
e = Environment()
print("env created"); sys.stdout.flush()
c = Controller(e.simulator)
c.wait(3000)
print("booted"); sys.stdout.flush()
