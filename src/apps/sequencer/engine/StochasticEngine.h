#pragma once

#include "TrackEngine.h"
#include "SequenceState.h"
#include "SortedQueue.h"
#include "Groove.h"
#include "RecordHistory.h"
#include "model/StochasticSequence.h"
#include "StepRecorder.h"
#include <vector>

class StochasticLoopStep {
    public:
        StochasticLoopStep() {}
        StochasticLoopStep(int degreeIndex, int octaveIndex, bool gate, uint32_t stepLength) {
            _degreeIndex = degreeIndex;
            _octaveIndex = octaveIndex;
            _gate = gate;
            _stepLength = stepLength;
        }

        int degreeIndex() const { return _degreeIndex; }
        int octaveIndex() const { return _octaveIndex; }
        bool gate() const { return _gate; }
        uint32_t stepLength() const { return _stepLength; }

    private:
        int _degreeIndex = -1;
        int8_t _octaveIndex = 2;
        bool _gate = false;
        uint32_t _stepLength = 0;
};

class StochasticEngine : public TrackEngine {
public:
    StochasticEngine(Engine &engine, Model &model, Track &track, const TrackEngine *linkedTrackEngine) :
        TrackEngine(engine, model, track, linkedTrackEngine),
        _stochasticTrack(track.stochasticTrack())
    {
        reset();
    }

    virtual Track::TrackMode trackMode() const override { return Track::TrackMode::Stochastic; }

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
        int len = _sequence->bufferLoopLength();
        return (len > 0 && _index >= 0) ? float(_index % len) / float(len) : 0.f;
    }

    const StochasticSequence &sequence() const { return *_sequence; }
    bool isActiveSequence(const StochasticSequence &sequence) const { return &sequence == _sequence; }

    int currentStep() const { return _currentDegreeIndex; }
    int currentRecordStep() const { return _stepRecorder.stepIndex(); }
    int currentIndex() const { return _index; }

    void setMonitorStep(int index);
    Types::PlayMode playMode() const { return _stochasticTrack.playMode(); }

    std::vector<StochasticLoopStep> lockedSteps() const { return _lockedSteps; }

private:
    void triggerStep(uint32_t tick, uint32_t divisor, bool nextStep);
    void triggerStep(uint32_t tick, uint32_t divisor);
    void recordStep(uint32_t tick, uint32_t divisor);

    bool fill() const {
        return (_stochasticTrack.fillMuted() || !TrackEngine::mute()) ? TrackEngine::fill() : false;
    }

    std::vector<StochasticLoopStep> slicing(std::vector<StochasticLoopStep> &arr, int X, int Y) {
        auto start = arr.begin() + X;
        auto end = arr.begin() + Y + 1;
        std::vector<StochasticLoopStep> result(Y - X + 1);
        copy(start, end, result.begin());
        return result;
    }

    StochasticTrack &_stochasticTrack;

    StochasticSequence *_sequence;
    const StochasticSequence *_fillSequence;

    uint32_t _freeRelativeTick;
    SequenceState _sequenceState;
    int _currentStep;
    int _currentDegreeIndex;
    int _index;
    bool _prevCondition;

    uint32_t _loopPhaseOffset = 0;
    int _lastSeqLen = 0;

    RecordHistory _recordHistory;
    bool _monitorOverrideActive = false;
    StepRecorder _stepRecorder;

    bool _activity;
    bool _gateOutput;
    float _cvOutput;
    float _cvOutputTarget;
    bool _slideActive;
    unsigned int _currentStageRepeat;

    std::vector<StochasticLoopStep> _lockedSteps;

    struct Gate {
        uint32_t tick;
        bool gate;
    };

    struct GateCompare {
        bool operator()(const Gate &a, const Gate &b) { return a.tick < b.tick; }
    };

    SortedQueue<Gate, 16, GateCompare> _gateQueue;

    struct Cv {
        uint32_t tick;
        float cv;
        bool slide;
    };

    struct CvCompare {
        bool operator()(const Cv &a, const Cv &b) { return a.tick < b.tick; }
    };

    SortedQueue<Cv, 16, CvCompare> _cvQueue;
};
