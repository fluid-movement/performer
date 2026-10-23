#!/usr/bin/env python3
"""Generates the glyph table in src/apps/sequencer/ui/painters/LogoPainter.cpp.

The "FLUID MOVEMENT" wordmark is a rounded geometric display face in the spirit of
the Daft Punk *Discovery* lettering: one uniform stroke weight, round terminals,
generous counters. Rather than hand-plotting pixels, every letter is described as a
union of capsules (thick line segments with round caps) and elliptical arcs, then
rasterised with 4x4 supersampling. That keeps the weight identical across the whole
alphabet and makes the curves actually curve.

Run from the repo root:

    python3 resources/generate-logo.py

and paste the output over the glyph table in LogoPainter.cpp.
"""

import math

CAP_HEIGHT = 22
STROKE = 4.0
R = STROKE / 2.0          # capsule radius
TOP = R                   # centre-line of the cap stroke
BOT = CAP_HEIGHT - R      # centre-line of the baseline stroke
SS = 4                    # supersample factor per axis


def arc(cx, cy, rx, ry, a0, a1, steps=48):
    """Polyline along an ellipse. Angles in radians, y points down."""
    pts = []
    for i in range(steps + 1):
        a = a0 + (a1 - a0) * i / steps
        pts.append((cx + rx * math.cos(a), cy + ry * math.sin(a)))
    return [(pts[i], pts[i + 1]) for i in range(len(pts) - 1)]


def seg(x0, y0, x1, y1):
    return [((x0, y0), (x1, y1))]


HALF = math.pi / 2

# Each glyph: (advance width in px, list of stroke segments).
#
# Stroke centres for anything with a flat edge sit on whole pixels, so a 4px stroke
# lands exactly on column/row boundaries instead of smearing a half-covered pixel down
# one side. Diagonals and arcs are free to sit anywhere.
GLYPHS = {
    'F': (13, seg(2, TOP, 2, BOT)          # stem
              + seg(2, TOP, 11, TOP)       # cap arm
              + seg(2, 11, 9, 11)),        # waist arm

    'L': (12, seg(2, TOP, 2, BOT)
              + seg(2, BOT, 10, BOT)),

    'U': (14, seg(2, TOP, 2, 13)
              + seg(12, TOP, 12, 13)
              + arc(7, 13, 5, 7, 0, math.pi)),

    'I': (5, seg(2, TOP, 2, BOT)),

    'D': (15, seg(2, TOP, 2, BOT)
              + seg(2, TOP, 7, TOP)
              + seg(2, BOT, 7, BOT)
              + arc(7, 11, 6, 9, -HALF, HALF)),

    'M': (22, seg(2, TOP, 2, BOT)
              + seg(20, TOP, 20, BOT)
              + seg(2, TOP, 11, 13)
              + seg(11, 13, 20, TOP)),

    'O': (16, arc(8, 11, 6, 9, 0, 2 * math.pi, 96)),

    'V': (18, seg(2, TOP, 9, BOT)
              + seg(9, BOT, 16, TOP)),

    'E': (13, seg(2, TOP, 2, BOT)
              + seg(2, TOP, 11, TOP)
              + seg(2, 11, 9, 11)
              + seg(2, BOT, 11, BOT)),

    'N': (16, seg(2, TOP, 2, BOT)
              + seg(14, TOP, 14, BOT)
              + seg(2, TOP, 14, BOT)),

    'T': (14, seg(2, TOP, 12, TOP)
              + seg(7, TOP, 7, BOT)),
}

ORDER = "FLUIDMOVENT"


def dist_point_segment(px, py, a, b):
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    l2 = dx * dx + dy * dy
    if l2 == 0.0:
        t = 0.0
    else:
        t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / l2))
    qx, qy = ax + t * dx, ay + t * dy
    return math.hypot(px - qx, py - qy)


def rasterize(width, segments):
    rows = []
    for y in range(CAP_HEIGHT):
        bits = 0
        for x in range(width):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    px = x + (sx + 0.5) / SS
                    py = y + (sy + 0.5) / SS
                    for a, b in segments:
                        if dist_point_segment(px, py, a, b) <= R:
                            hits += 1
                            break
            if hits * 2 >= SS * SS:
                bits |= 1 << (width - 1 - x)
        rows.append(bits)
    return rows


def main():
    out = []
    for ch in ORDER:
        width, segments = GLYPHS[ch]
        rows = rasterize(width, segments)
        art = ["".join('#' if r & (1 << (width - 1 - x)) else '.' for x in range(width))
               for r in rows]
        out.append((ch, width, rows, art))

    print("// clang-format off")
    print("static const LogoGlyph glyphs[] = {")
    for ch, width, rows, art in out:
        print("    // '%s'" % ch)
        print("    { %d, {" % width)
        for r, a in zip(rows, art):
            print("        0x%05x,  // %s" % (r, a))
        print("    } },")
    print("};")
    print("// clang-format on")
    print()
    print("// advance widths: " + ", ".join("%s=%d" % (c, w) for c, w, _, _ in out))
    for name in ("FLUID", "MOVEMENT"):
        total = sum(GLYPHS[c][0] for c in name) + 2 * (len(name) - 1)
        print("// '%s' = %d px" % (name, total))


if __name__ == '__main__':
    main()
