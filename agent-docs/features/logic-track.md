# Logic Track

## Purpose
A gate-processing track that applies boolean logic operations to other tracks' gate outputs. Does not generate its own notes — it computes a gate output by combining gate inputs from other tracks using per-step logic operators (AND, OR, XOR, NOT, etc.). Can be used to drive CV/gate outputs or to gate other tracks via NoteTrack's `logicTrack` setting.

## Key Source Files

| Role | Path |
|---|---|
| Data model (track settings) | `src/apps/sequencer/model/LogicTrack.h/cpp` |
| Data model (sequence + steps) | `src/apps/sequencer/model/LogicSequence.h/cpp` |
| Real-time engine | `src/apps/sequencer/engine/LogicTrackEngine.h/cpp` |
| Sequence display page | `src/apps/sequencer/ui/pages/LogicSequencePage.h/cpp` |
| Sequence edit page | `src/apps/sequencer/ui/pages/LogicSequenceEditPage.h/cpp` |
| UI list model | `src/apps/sequencer/ui/model/LogicTrackListModel.h` |

## Data Model

### `LogicTrack` (track-level settings)
- `_playMode` — `Types::PlayMode`
- `_fillMode` — `FillMode` enum
- `_gateInputTrack1`, `_gateInputTrack2` — track indices (0–7) for the two gate inputs used in logic operations
- `_sequences` — array of 17 `LogicSequence`

### `LogicSequence::Step` layers
| Layer | Description |
|---|---|
| GateLogic | Logic operator: And, Or, Xor, Not (of input 1), Not (of input 2), FlipFlop, Latch, ... |
| Gate | Override: force gate on/off |
| GateProbability | Probability of gate output |
| Length | Gate output length |
| Condition | Conditional trigger |

## Engine Logic (`LogicTrackEngine`)

1. On each clock tick: read gate states from `_gateInputTrack1` and `_gateInputTrack2` track engines.
2. Apply the step's `GateLogic` operator to produce a combined gate value.
3. Apply `gateProbability` and `condition`.
4. Output the resulting gate to `GateOutput` for this track's channel.
5. No CV output — logic tracks only drive gates.

Other tracks reference this track via `NoteTrack::_logicTrack` — the NoteTrackEngine ANDs its own gate output with this track's output.

## UI Pages

| Page | Path | Purpose |
|---|---|---|
| `LogicSequencePage` | `ui/pages/LogicSequencePage.h/cpp` | Step overview |
| `LogicSequenceEditPage` | `ui/pages/LogicSequenceEditPage.h/cpp` | Per-step logic operator editing |

## Change Guide

| Task | Where to look |
|---|---|
| Add a new logic operator | `LogicSequence.h` — `GateLogic` enum + `LogicTrackEngine.cpp` operator evaluation |
| Change which tracks can be gate inputs | `LogicTrack.h` `_gateInputTrack1/2` + `LogicTrackEngine.cpp` input read logic |
| Fix gate output timing | `LogicTrackEngine.cpp` gate output section |
| Connect LogicTrack to NoteTrack gating | `NoteTrack.h` `_logicTrack`/`_logicTrackInput` + `NoteTrackEngine.cpp` logic gate ANDing |
