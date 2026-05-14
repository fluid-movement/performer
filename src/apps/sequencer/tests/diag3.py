import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../../../build/sim/release/src/apps/sequencer/python"))

from testsim import Environment

print("booting..."); sys.stdout.flush()
e = Environment()
print("env ok"); sys.stdout.flush()

print("wait 100ms..."); sys.stdout.flush()
e.simulator.wait(100)
print("ok"); sys.stdout.flush()

print("wait 200ms..."); sys.stdout.flush()
e.simulator.wait(200)
print("ok"); sys.stdout.flush()

print("wait 500ms..."); sys.stdout.flush()
e.simulator.wait(500)
print("ok"); sys.stdout.flush()

print("wait 1000ms..."); sys.stdout.flush()
e.simulator.wait(1000)
print("ok"); sys.stdout.flush()

print("wait 2000ms..."); sys.stdout.flush()
e.simulator.wait(2000)
print("ok — all waits done"); sys.stdout.flush()
