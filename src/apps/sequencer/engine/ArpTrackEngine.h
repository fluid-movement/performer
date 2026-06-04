#pragma once

#include "TrackEngine.h"
#include "SequenceState.h"
#include "SortedQueue.h"
#include "Groove.h"
#include "model/ArpSequence.h"
#include <cstdint>
#include <vector>

class ArpTrackEngine : public TrackEngine {
public:
    ArpTrackEngine(Engine &engine, Model &model, Track &track, const TrackEngine *linkedTrackEngine) :
        TrackEngine(engine, model, track, linkedTrackEngine),
        _arpTrack(track.arpTrack()),
        _model(model)
    {
        reset();
    }

    virtual Track::TrackMode trackMode() const override { return Track::TrackMode::Arp; }

    virtual void reset() override;
    virtual void restart() override;
    virtual TickResult tick(uint32_t tick) override;
    virtual void update(float dt) override;

    virtual void changePattern() override;

    virtual void monitorMidi(uint32_t tick, const MidiMessage &message) override;
    virtual void clearMidiMonitoring() override;

    virtual const TrackLinkData *linkData() const override { return &_linkData; }

    virtual bool activity() const override { return _activity; }
    virtual bool gateOutput(int index) const override { return _gateOutput; }
    virtual float cvOutput(int index) const override { return _cvOutput; }
    virtual float sequenceProgress() const override {
        if (!_sequence) return 0.f;
        int len = std::max(_sequence->arpLength(), 1);
        return float(_step % len) / float(len);
    }

    const ArpSequence &sequence() const { return *_sequence; }
    bool isActiveSequence(const ArpSequence &sequence) const { return &sequence == _sequence; }

    int currentStep() const { return _step; }
    int currentDegree() const { return _currentDegree; }

    void setMonitorStep(int /*index*/) {}

private:
    void triggerStep(uint32_t tick, uint32_t divisor);
    float nextNoteCv();
    void rebuildNotePool();
    void recomputePatterns();

    bool fill() const {
        return (_arpTrack.fillMuted() || !TrackEngine::mute()) ? TrackEngine::fill() : false;
    }

    ArpTrack &_arpTrack;
    const Model &_model;

    TrackLinkData _linkData;

    ArpSequence *_sequence = nullptr;

    SequenceState _sequenceState;

    // Euclidean patterns (precomputed)
    bool _rhythmPat[16] = {};
    bool _modPat[16]    = {};
    uint8_t _lastRhythmN = 0, _lastRhythmK = 0, _lastRhythmR = 0;
    uint8_t _lastModN = 0,    _lastModK = 0,    _lastModR = 0;

    // Playback state
    int _step          = 0;   // absolute step counter, wraps at arpLength
    int _arpPhase      = 0;   // position in note pool cycle
    int _currentDegree = -1;  // last-played scale degree index (0–6), -1 = none

    // Note pool
    std::vector<int> _activeNotes;  // sorted degree indices

    // Output
    bool  _activity   = false;
    bool  _gateOutput = false;
    float _cvOutput   = 0.f;
    float _cvOutputTarget = 0.f;
    bool  _slideActive = false;

    bool _monitorOverrideActive = false;

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
