#pragma once

#include "TrackEngine.h"
#include "SequenceState.h"
#include "Slide.h"
#include "SortedQueue.h"
#include "Groove.h"
#include "RecordHistory.h"
#include "model/NoteSequence.h"
#include "StepRecorder.h"

class NoteTrackEngine : public TrackEngine {
public:
    NoteTrackEngine(Engine &engine, Model &model, Track &track, const TrackEngine *linkedTrackEngine) :
        TrackEngine(engine, model, track, linkedTrackEngine),
        _noteTrack(track.noteTrack())
    {
        reset();
    }

    virtual Track::TrackMode trackMode() const override { return Track::TrackMode::Note; }

    virtual void reset() override;
    virtual void restart() override;
    virtual TickResult tick(uint32_t tick) override;
    virtual void update(float dt) override;

    virtual void changePattern() override;

    virtual void monitorMidi(uint32_t tick, const MidiMessage &message) override;
    virtual void clearMidiMonitoring() override;

    virtual bool activity() const override { return _activity; }
    virtual bool gateOutput(int index) const override { return _gateOutput; }
    virtual float cvOutput(int index) const override { return _cvOutput; }
    virtual float sequenceProgress() const override {
        return _currentStep < 0 ? 0.f : float(_currentStep - _sequence->firstStep()) / (_sequence->lastStep() - _sequence->firstStep());
    }

    const NoteSequence &sequence() const { return *_sequence; }
    bool isActiveSequence(const NoteSequence &sequence) const { return &sequence == _sequence; }

    int currentStep() const { return _currentStep; }
    int currentRecordStep() const { return _stepRecorder.stepIndex(); }
    void setCurrentRecordStep(int value) {
        _stepRecorder.setStepIndex(value);
    }

    void setMonitorStep(int index);

    SequenceState sequenceState() {
        return _sequenceState;
    }


private:
    // Schedule the events for one retrigger slice; begins a new step when the
    // slot index wraps back to zero.
    void advanceSlot(uint32_t tick, uint32_t divisor);
    void beginStep(uint32_t tick, uint32_t divisor);
    uint32_t slotOnset(int slot) const;
    void recordStep(uint32_t tick, uint32_t divisor);
    int noteFromMidiNote(uint8_t midiNote) const;

    bool fill() const {
        return (_noteTrack.fillMuted() || !TrackEngine::mute()) ? TrackEngine::fill() : false;
    }

    NoteTrack &_noteTrack;

    NoteSequence *_sequence;
    const NoteSequence *_fillSequence;

    SequenceState _sequenceState;
    int _currentStep;
    bool _prevCondition;

    // --- slot scheduler ---------------------------------------------------
    // A step is divided into repeat x retrigger equally spaced slots. Onsets are
    // derived from the step start rather than accumulated, so rounding error
    // stays bounded and the final slot lands exactly on the step end.
    uint32_t _stepStartTick = 0;
    uint32_t _stepTotalTicks = 0;
    uint32_t _nextSlotTick = 0;
    int _slotIndex = 0;
    int _totalSlots = 1;
    int _retrigCount = 1;
    int _baseNote = 0;
    int _arpRetrigger = 0;
    int _gateLength = 100;
    bool _stepGate = false;
    bool _stepSlide = false;
    bool _legatoHeld = false;
    bool _started = false;

    int _monitorStepIndex = -1;

    RecordHistory _recordHistory;
    bool _monitorOverrideActive = false;
    StepRecorder _stepRecorder;

    bool _activity;
    bool _gateOutput;
    float _cvOutput;
    float _cvOutputTarget;
    Slide::Ramp _slideRamp;

    struct Gate {
        uint32_t tick;
        bool gate;
    };

    struct GateCompare {
        bool operator()(const Gate &a, const Gate &b) {
            return a.tick < b.tick;
        }
    };

    SortedQueue<Gate, 16, GateCompare> _gateQueue;

    struct Cv {
        uint32_t tick;
        float cv;
        bool slide;
    };

    struct CvCompare {
        bool operator()(const Cv &a, const Cv &b) {
            return a.tick < b.tick;
        }
    };

    SortedQueue<Cv, 16, CvCompare> _cvQueue;
};
