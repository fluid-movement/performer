"""
Agent harness for headless simulator scripting.

Usage:
    from agent import Session
    s = Session()
    s.press("play"); s.wait(500)
    print(s.cv(0))         # CV volts on channel 0
    print(s.gate(0))       # gate state on channel 0
    print(s.led(8))        # (red, green) for step 1 LED
    s.screenshot("/tmp/out.png")
    s.close()
"""

import os
import sys

_module_path = os.path.normpath(
    os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "../../../../build/sim/release/src/apps/sequencer/python"
    )
)
sys.path.insert(0, _module_path)

from testsim import Environment
from testframework.controller import Controller


class Session:
    """Thin wrapper combining Environment + Controller + state-read helpers."""

    def __init__(self, boot_ms: int = 3000):
        self.env = Environment()
        self.sim = self.env.simulator
        self.ctrl = Controller(self.sim)
        self.ctrl.wait(boot_ms)

    # ------------------------------------------------------------------
    # Input helpers (delegate to Controller)
    # ------------------------------------------------------------------

    def press(self, button: str, pre: int = 10, post: int = 10) -> "Session":
        self.ctrl.press(button, pre, post)
        return self

    def down(self, button: str) -> "Session":
        self.ctrl.down(button)
        return self

    def up(self, button: str) -> "Session":
        self.ctrl.up(button)
        return self

    def wait(self, ms: int) -> "Session":
        self.ctrl.wait(ms)
        return self

    def rotate(self, steps: int, fast: bool = False) -> "Session":
        self.ctrl.rotateEncoder(steps, fast)
        return self

    def adc(self, channel: int, volts: float) -> "Session":
        self.ctrl.adc(channel, volts)
        return self

    # ------------------------------------------------------------------
    # Output helpers (read TargetState)
    # ------------------------------------------------------------------

    def cv(self, channel: int) -> float:
        """CV voltage on DAC channel (0-7). 0V = C4, +1V = C5."""
        return self.sim.targetState.dac.volts(channel)

    def cv_raw(self, channel: int) -> int:
        """Raw 16-bit DAC code on channel (0-7)."""
        return self.sim.targetState.dac[channel]

    def gate(self, channel: int) -> bool:
        """Gate output state on channel (0-7)."""
        return bool(self.sim.targetState.gateOutput[channel])

    def led(self, index: int) -> tuple:
        """(red: bool, green: bool) for LED at index."""
        return self.sim.targetState.led.get(index)

    def button_state(self, index: int) -> bool:
        """Current button press state."""
        return bool(self.sim.targetState.button[index])

    def all_cv(self) -> list:
        """CV volts for all 8 DAC channels."""
        return [self.sim.targetState.dac.volts(i) for i in range(8)]

    def all_gates(self) -> list:
        """Gate state (bool) for all 8 channels."""
        return [bool(self.sim.targetState.gateOutput[i]) for i in range(8)]

    def lcd_bytes(self) -> bytes:
        """Raw LCD framebuffer: 256*64 bytes, each = pixel brightness."""
        return self.sim.targetState.lcd.bytes()

    def screenshot(self, path: str) -> "Session":
        """Save PNG screenshot to path."""
        self.sim.screenshot(path)
        return self

    # ------------------------------------------------------------------
    # Trace helpers
    # ------------------------------------------------------------------

    def record_trace(self, ms: int, path: str) -> str:
        """Run for ms milliseconds while recording, save trace text to path."""
        from testsim.simulator import TargetTrace
        trace = TargetTrace()
        self.ctrl.wait(ms)
        trace.saveToText(path)
        return path

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def close(self):
        self.ctrl = None
        self.sim = None
        self.env = None
