#!/usr/bin/env python3
"""Captures and checks the boot splash.

Every other harness waits the splash out (agent.py boots with boot_ms=3000), so this is
the only place the first two seconds are looked at. Run from the repo root:

    python src/apps/sequencer/tests/ui/splash_test.py [outdir]
"""

import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'build/sim/release/src/apps/sequencer/python'))
sys.path.insert(0, os.path.join(ROOT, 'src/apps/sequencer/tests'))

from testsim import Environment            # noqa: E402
from testframework.controller import Controller  # noqa: E402

OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/splash'

WIDTH, HEIGHT = 256, 64
WORDMARK_TOP, WORDMARK_BOTTOM = 12, 33
REFLECTION_TOP, REFLECTION_BOTTOM = 34, 39
PROGRESS_Y = 57

failures = []


def check(name, ok, detail=''):
    print('%-46s %s %s' % (name, 'PASS' if ok else 'FAIL', detail))
    if not ok:
        failures.append(name)


def pixels(sim):
    """Framebuffer as [y][x] brightness in the hardware's 0..15 range."""
    raw = sim.targetState.lcd.bytes()
    return [[min(15, raw[y * WIDTH + x]) for x in range(WIDTH)] for y in range(HEIGHT)]


def band(px, top, bottom):
    return [v for row in px[top:bottom + 1] for v in row]


def main():
    os.makedirs(OUT, exist_ok=True)
    e = Environment()
    c = Controller(e.simulator)

    frames = {}
    elapsed = 0
    for ms in (60, 300, 700, 1100, 1500, 1900):
        c.wait(ms - elapsed)
        elapsed = ms
        e.simulator.screenshot(os.path.join(OUT, 'splash-%04d.png' % ms))
        frames[ms] = pixels(e.simulator)

    mid = frames[1100]

    # The chrome ramp has to survive the framebuffer round-trip, not collapse to a stencil.
    levels = set(band(mid, WORDMARK_TOP, WORDMARK_BOTTOM))
    check('wordmark uses more than 3 grey levels', len(levels) > 3, sorted(levels))

    # Reflection present, and clearly dimmer than the letters above it.
    wordmark_ink = [v for v in band(mid, WORDMARK_TOP, WORDMARK_BOTTOM) if v > 0]
    reflection_ink = [v for v in band(mid, REFLECTION_TOP, REFLECTION_BOTTOM) if v > 0]
    check('reflection is drawn', len(reflection_ink) > 40, '%d px' % len(reflection_ink))
    check('reflection is dimmer than the wordmark',
          bool(reflection_ink) and max(reflection_ink) < max(wordmark_ink),
          '%s vs %s' % (max(reflection_ink or [0]), max(wordmark_ink or [0])))

    # The specular sweep has to actually move.
    def brightest_column(px):
        cols = [(sum(px[y][x] for y in range(WORDMARK_TOP, WORDMARK_BOTTOM + 1)), x)
                for x in range(WIDTH)]
        return max(cols)[1]

    early, late = brightest_column(frames[300]), brightest_column(frames[1500])
    check('specular sweep moves left to right', late > early, '%d -> %d' % (early, late))

    # Nothing runs off the edges.
    check('no ink in the top scanline', not any(frames[1100][0]))
    check('no ink in the bottom scanline', not any(frames[1100][HEIGHT - 1]))
    check('no ink in column 0', not any(row[0] for row in frames[1100]))
    check('no ink in the last column', not any(row[WIDTH - 1] for row in frames[1100]))

    # Progress bar grows and shares the wordmark's left edge.
    def bar_fill(px):
        return sum(1 for v in px[PROGRESS_Y] if v >= 12)

    check('progress bar grows', bar_fill(frames[1900]) > bar_fill(frames[300]),
          '%d -> %d' % (bar_fill(frames[300]), bar_fill(frames[1900])))

    ink_columns = [x for x in range(WIDTH)
                   if any(frames[1100][y][x] for y in range(WORDMARK_TOP, WORDMARK_BOTTOM + 1))]
    bar_columns = [x for x in range(WIDTH) if frames[1100][PROGRESS_Y][x]]
    check('bar and wordmark share a left edge', min(bar_columns) == min(ink_columns),
          'bar %d, wordmark %d' % (min(bar_columns), min(ink_columns)))

    # The splash still gets out of the way.
    c.wait(2000)
    after = pixels(e.simulator)
    check('splash closes', after != frames[1900])

    print('\nwrote %s' % OUT)
    if failures:
        print('%d FAILED: %s' % (len(failures), ', '.join(failures)))
        return 1
    print('all checks passed')
    return 0


if __name__ == '__main__':
    sys.exit(main())
