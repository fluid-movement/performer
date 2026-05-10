# Plan: Scale-Degree Note Display

## Context
The user wants to enter and display notes as scale degrees (1, 5, 7, etc.) instead of absolute chromatic note names (C, D#, etc.). The goal: set a project-level scale once, enter scale degree numbers in the step editor, and changing the scale automatically re-harmonizes everything without re-entering notes.

## Key Finding: This Is Mostly Already Done in the Data Layer

The note storage in `NoteSequence` already stores notes as **scale-relative indices** (type `SignedValue<7>`, range -63 to +63), not as MIDI/chromatic numbers. The engine already applies the chosen scale to convert these indices to CV voltages at playback time. A project-level scale (`Project::scale()`) and per-sequence scale override already exist.

**Changing the project scale already re-harmonizes CV output without touching stored note data.** The feature is primarily a **UI display change**.

## Design Decisions

1. **Display format**: Keep the existing two-row layout — degree number on top (replacing the chromatic note name), octave number stays below unchanged.
   - e.g., currently shows `C` / `0` → will show `1` / `0`
   - e.g., currently shows `G` / `1` → will show `5` / `1`

2. **Conditional**: If the project scale is non-chromatic → show scale degrees. If the project scale is chromatic (default, index 0) → keep existing chromatic note name display as-is.

3. **Scope**: All track types that use note storage — Note, Stochastic, and Arp tracks.

## What Needs to Change

### 1. Note display in step editor pages (primary change)
**Files:**
- `src/apps/sequencer/ui/pages/NoteSequenceEditPage.cpp` (~line 225–246)
- `src/apps/sequencer/ui/pages/StochasticSequenceEditPage.cpp` (equivalent display section)
- `src/apps/sequencer/ui/pages/ArpSequenceEditPage.cpp` (equivalent display section)

Currently: resolves scale index → chromatic note name + octave (e.g., "C", octave "0")
Target: when project scale is non-chromatic → display 1-indexed scale degree + same octave

Degree calculation from scale-relative note index:
```cpp
// Get the scale used by this sequence
const Scale &scale = sequence.selectedScale(_project.scale());

if (!scale.isChromatic()) {
    int notesPerOctave = scale.notesPerOctave();
    int octave = floorDiv(note, notesPerOctave);      // same octave already shown below
    int degree = note - octave * notesPerOctave + 1;  // 1-indexed (1..notesPerOctave)
    // Display: degree as string in the "note name" slot, octave unchanged
} else {
    // existing chromatic note name display unchanged
}
```

The `bypassScale` path on individual steps always keeps the existing chromatic display regardless of project scale.

### 2. (Optional) Shared helper on Scale class
If the degree logic is needed across all three edit pages, add to `src/apps/sequencer/model/Scale.h`:
```cpp
// Returns {1-indexed degree, octave} for a note index
std::pair<int,int> noteToScaleDegree(int note) const;
```
Otherwise implement inline in each edit page to keep changes minimal.

## What Does NOT Need to Change
| Concern | Status |
|---|---|
| NoteSequence / StochasticSequence / ArpSequence data model | Already stores scale-relative indices |
| Serialization / file format | Unchanged — same values on disk |
| NoteTrackEngine / StochasticEngine / ArpTrackEngine CV conversion | Already applies scale at playback |
| Project/sequence scale settings and UI | Already exist and are accessible |
| Encoder input mechanics | Already increments in scale steps |
| MIDI note input → scale degree conversion | Already uses `scale.noteFromVolts()` |

## Critical Files
| File | Change |
|---|---|
| `src/apps/sequencer/ui/pages/NoteSequenceEditPage.cpp` | **Yes** — degree display logic |
| `src/apps/sequencer/ui/pages/StochasticSequenceEditPage.cpp` | **Yes** — same display change |
| `src/apps/sequencer/ui/pages/ArpSequenceEditPage.cpp` | **Yes** — same display change |
| `src/apps/sequencer/model/Scale.h` | **Possibly** — add degree helper |
| `src/apps/sequencer/model/NoteSequence.h` | No |
| `src/apps/sequencer/model/Project.h` | No |
| `src/apps/sequencer/engine/NoteTrackEngine.cpp` | No |

## Verification
1. Build simulator: `cd build/sim/debug && make -j sequencer`
2. Run: `./src/apps/sequencer/sequencer`
3. With project scale = chromatic (default): verify existing note name display is unchanged
4. Change project scale to a non-chromatic scale (e.g., Major)
5. Enter notes on a Note track — verify degree numbers (1–7) appear in the note name slot, octave number unchanged below
6. Change the project scale to a different scale — verify degree numbers stay the same (re-harmonization confirmed: CV output changes, displayed degrees don't)
7. Repeat steps 4–6 on a Stochastic and Arp track
8. Verify a step with `bypassScale` still shows chromatic note name regardless of scale
