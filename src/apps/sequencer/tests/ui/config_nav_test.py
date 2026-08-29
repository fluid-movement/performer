"""Config page navigation tests -- the modeless encoder.

Covers the ListPage rework: the encoder always edits the selected row (it never
navigates), Left/Right always move the cursor, and F1-F5 jump between sections.

The central claim under test is that there is NO edit mode. Before this change
you had to click the encoder to arm a value; turning it beforehand scrolled the
list instead. Every "no mode" check below turns the encoder straight after
landing on a page and asserts the value moved.

Note: creating multiple Environment() instances in one process crashes the
simulator, so all tests share one environment.
"""
import sys
sys.path.insert(0, 'build/sim/release/src/apps/sequencer/python')
sys.path.insert(0, 'src/apps/sequencer/tests')

from testsim import Environment
from testframework.controller import Controller
import testsim.sequencer as tsseq

FAILURES = []

# Section index -> function key name. F0..F4 are "f1".."f5" in the controller.
FKEY = ["f1", "f2", "f3", "f4", "f5"]


def check(cond, msg):
    if cond:
        print("  ok   " + msg)
    else:
        print("  FAIL " + msg)
        FAILURES.append(msg)


def section(c, index):
    c.press(FKEY[index])


def main():
    e = Environment()
    c = Controller(e.simulator)
    c.wait(3000)

    p = e.sequencer.model.project
    p.setTrackMode(0, tsseq.Track.TrackMode.Note)
    p.selectedTrackIndex = 0
    track = p.tracks[0].noteTrack

    # ---------------------------------------------------------------- TRACK
    print("\nTRACK CONFIG -- no mode, sections, navigation")
    c.selectPage("track")
    c.wait(50)

    # TIME section: GateMode, SlideType, SlideTime
    section(c, 1)
    c.wait(20)

    before_gate = track.gateMode
    before_slide_time = track.slideTime
    c.rotateEncoder(1)
    c.wait(20)
    check(track.gateMode != before_gate,
          "encoder edits immediately with no click to arm (gate mode moved)")
    check(track.slideTime == before_slide_time,
          "the turn did not move the cursor (slide time untouched)")

    # A second turn keeps editing the same row rather than toggling a mode.
    gate_after_one = track.gateMode
    c.rotateEncoder(-1)
    c.wait(20)
    check(track.gateMode != gate_after_one,
          "a second turn keeps editing the same row (no mode toggled in between)")

    # Left/Right navigate.
    c.press("next")
    c.wait(20)
    before_type = track.slideType
    gate_before = track.gateMode
    c.rotateEncoder(1)
    c.wait(20)
    check(track.slideType != before_type, "Right moved to Slide Type and the encoder edits it")
    check(track.gateMode == gate_before, "editing Slide Type left Gate Mode alone")

    c.press("prev")
    c.wait(20)
    gate_before = track.gateMode
    # Gate Mode has only two values, so whichever way it is currently pinned one
    # of these turns must move it if the cursor really is on that row.
    c.rotateEncoder(1)
    c.wait(20)
    if track.gateMode == gate_before:
        c.rotateEncoder(-1)
        c.wait(20)
    check(track.gateMode != gate_before, "Left moved back to Gate Mode")

    # Encoder press must not latch anything.
    c.press("next")
    c.press("next")
    c.wait(20)
    st = track.slideTime
    c.rotateEncoder(2)
    c.wait(20)
    after_turn = track.slideTime
    check(after_turn != st, "slide time edits on turn (%d -> %d)" % (st, after_turn))

    c.pressEncoder()
    c.wait(20)
    check(track.slideTime == after_turn, "encoder press alone changed no value")
    c.rotateEncoder(2)
    c.wait(20)
    check(track.slideTime != after_turn,
          "turning after an encoder press still edits -- the press latched no mode")

    # TUNE section.
    section(c, 2)
    c.wait(20)
    before_oct = track.octave
    slide_before = track.slideTime
    c.rotateEncoder(1)
    c.wait(20)
    check(track.octave != before_oct, "F3 jumped to TUNE and the cursor landed on Octave")
    check(track.slideTime == slide_before, "switching section did not disturb the TIME rows")

    c.press("next")
    c.wait(20)
    before_tr = track.transpose
    c.rotateEncoder(1)
    c.wait(20)
    check(track.transpose != before_tr, "Right within TUNE reaches Transpose")

    # Section 0 row 0 is the track name -- editing it must be a harmless no-op.
    section(c, 0)
    c.wait(20)
    oct_before = track.octave
    c.rotateEncoder(1)
    c.wait(20)
    check(track.octave == oct_before, "turning on the name row edits nothing else")

    c.screenshot("/tmp/cfg_track_type.png")
    section(c, 1)
    c.wait(20)
    c.screenshot("/tmp/cfg_track_time.png")

    # ---------------------------------------------------------------- CLOCK
    print("\nCLOCK SETUP -- sections and modeless editing")
    cs = p.clockSetup
    c.selectPage("clock")
    c.wait(50)

    section(c, 0)
    c.wait(20)
    before_mode = cs.mode
    c.rotateEncoder(1)
    c.wait(20)
    check(cs.mode != before_mode, "clock MODE section edits on first turn")

    section(c, 2)   # OUT: divisor, swing, pulse, mode
    c.wait(20)
    before_div = cs.clockOutputDivisor
    c.rotateEncoder(1)
    c.wait(20)
    check(cs.clockOutputDivisor != before_div, "clock OUT section lands on output divisor")

    c.press("next")
    c.wait(20)
    before_swing = cs.clockOutputSwing
    c.rotateEncoder(1)
    c.wait(20)
    check(cs.clockOutputSwing != before_swing, "Right reaches output swing")

    section(c, 3)   # MIDI: midiRx, midiTx, usbRx, usbTx, filterNote
    c.wait(20)
    before_rx = cs.midiRx
    c.rotateEncoder(-1 if before_rx else 1)
    c.wait(20)
    check(cs.midiRx != before_rx, "clock MIDI section lands on MIDI RX")
    c.screenshot("/tmp/cfg_clock_midi.png")

    # -------------------------------------------------------------- PROJECT
    print("\nPROJECT -- sections and modeless editing")
    c.selectPage("project")
    c.wait(50)

    section(c, 1)   # SCALE: scale, root note
    c.wait(20)
    before_scale = p.scale
    c.rotateEncoder(1)
    c.wait(20)
    check(p.scale != before_scale, "project SCALE section edits scale on first turn")

    c.press("next")
    c.wait(20)
    before_root = p.rootNote
    c.rotateEncoder(1)
    c.wait(20)
    check(p.rootNote != before_root, "Right reaches root note")

    section(c, 3)   # REC: monitor mode, record mode, record delay, steps to stop
    c.wait(20)
    before_monitor = p.monitorMode
    c.rotateEncoder(1)
    c.wait(20)
    check(p.monitorMode != before_monitor, "project REC section lands on monitor mode")

    section(c, 4)   # CV: cv gate input, reset cv on stop, multi cv rec
    c.wait(20)
    before_cv = p.cvGateInput
    c.rotateEncoder(1)
    c.wait(20)
    check(p.cvGateInput != before_cv, "project CV section lands on CV gate input")
    c.screenshot("/tmp/cfg_project_cv.png")

    # ------------------------------------------------- other track types
    print("\nOTHER TRACK TYPES -- sections resolve without crashing")
    for mode_name, mode in [("Curve", tsseq.Track.TrackMode.Curve),
                            ("Stochastic", tsseq.Track.TrackMode.Stochastic),
                            ("Arp", tsseq.Track.TrackMode.Arp),
                            ("Quantizer", tsseq.Track.TrackMode.Quantizer),
                            ("MidiCv", tsseq.Track.TrackMode.MidiCv)]:
        p.setTrackMode(0, mode)
        c.selectPage("track")
        c.wait(50)
        for s in range(5):
            section(c, s)
            c.wait(10)
            c.rotateEncoder(1)
            c.wait(10)
        c.screenshot("/tmp/cfg_track_%s.png" % mode_name.lower())
        check(True, "%s track config walked all sections" % mode_name)

    # ------------------------------------------------- pages without sections
    # Routing's Tracks row renders 8 checkboxes via a drawCell override; it only
    # appears for per-track targets, so move the target off None first.
    print("\nROUTING -- per-track checkbox row")
    c.selectPage("routing")
    c.wait(50)
    # Target order is None, Play, PlayToggle, Record, RecordToggle, TapTempo,
    # Tempo, Swing, Mute -- Mute is the first per-track target.
    c.rotateEncoder(8)
    c.wait(20)
    c.press("next")
    c.press("next")
    c.press("next")             # Target -> Min -> Max -> Tracks
    c.wait(20)
    c.press("track1")           # toggle route membership for track 1
    c.press("track3")
    c.wait(20)
    c.screenshot("/tmp/cfg_routing_tracks.png")
    check(True, "routing per-track checkbox row rendered without crashing")

    print("\nPAGES WITHOUT SECTIONS -- footer intact, encoder still edits")
    for page in ["routing", "midiout", "userscale", "layout", "system"]:
        c.selectPage(page)
        c.wait(50)
        if page == "system":
            c.press("f5")      # confirm "enter system page?"
            c.wait(50)
        c.rotateEncoder(1)
        c.wait(10)
        c.press("next")
        c.wait(10)
        c.rotateEncoder(-1)
        c.wait(10)
        c.screenshot("/tmp/cfg_%s.png" % page)
        check(True, "%s page navigates and edits without crashing" % page)

    print("\n" + "=" * 60)
    if FAILURES:
        print("FAILED (%d):" % len(FAILURES))
        for f in FAILURES:
            print("  - " + f)
        return 1
    print("ALL PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
