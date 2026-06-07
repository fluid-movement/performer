#pragma once

#include "TrackEngine.h"
#include "SequenceState.h"
#include "SortedQueue.h"
#include "Groove.h"
#include "model/QuantizerTrack.h"
#include "model/NoteSequence.h"

class QuantizerTrackEngine : public TrackEngine {
public:
    QuantizerTrackEngine(Engine &engine, Model &model, Track &track, const TrackEngine *linkedTrackEngine) :
        TrackEngine(engine, model, track, linkedTrackEngine),
        _quantizerTrack(track.quantizerTrack())
    {
        reset();
    }

    virtual Track::TrackMode trackMode() const override { return Track::TrackMode::Quantizer; }

    virtual void reset() override;
    virtual void restart() override;
    virtual TickResult tick(uint32_t tick) override;
    virtual void update(float dt) override;

    virtual void changePattern() override;

    virtual void monitorMidi(uint32_t tick, const MidiMessage &message) override {}
    virtual void clearMidiMonitoring() override {}

    enum class LoopMode { Play, Rec, Loop };

    LoopMode loopMode() const { return _loopMode; }
    void setLoopMode(LoopMode mode);

    int loopFillCount() const { return _loopFillCount; }
    int loopPlaySlot() const;

    virtual bool activity() const override { return _gateOutput; }
    virtual bool gateOutput(int index) const override { return _gateOutput; }
    virtual float cvOutput(int index) const override { return _cvOutput; }
    virtual float sequenceProgress() const override {
        if (_currentStep < 0) return 0.f;
        int range = _sequence->lastStep() - _sequence->firstStep();
        return range <= 0 ? 0.f : float(_currentStep - _sequence->firstStep()) / range;
    }

    const NoteSequence &sequence() const { return *_sequence; }
    bool isActiveSequence(const NoteSequence &sequence) const { return &sequence == _sequence; }

    int currentStep() const { return _currentStep; }

private:
    float readInput() const;

    QuantizerTrack &_quantizerTrack;

    NoteSequence *_sequence = nullptr;

    SequenceState _sequenceState;
    int _currentStep = -1;

    int _lastQNote = INT32_MIN;
    float _lastQVolts = 0.f;
    int _lastTransposition = INT32_MIN;
    bool _lastSourceGate = false;
    int _lastTriggerTrack = -1;
    int _lastCvChannel    = -1;  // absorbs spurious edges on CvGate channel change

    // IIR low-pass on raw ADC input
    mutable float _filteredInput = 0.f;
    mutable bool _filterInit = false;

    // Delayed post-trigger sampling (Internal / External modes)
    bool _samplePending = false;
    uint32_t _sampleTick = 0;

    // Loop state (runtime only, not persisted)
    static constexpr int LoopBufferSize = 16;
    LoopMode _loopMode = LoopMode::Play;
    int   _loopBuffer[LoopBufferSize]{};  // un-transposed note indices; transposition applied at playback
    int   _loopFillCount = 0;
    int   _loopIndex = 0;

    bool _gateOutput = false;
    float _cvOutput = 0.f;
    uint32_t _pulseTick = 0;

    static constexpr uint32_t PulseLengthTicks = CONFIG_PPQN / 24;
    // α=0.1 → ~26ms TC. All note-boundary crossings (≥83mV) trigger the snap path, so
    // reducing α has zero effect on pitch-change latency while improving suppression of
    // systematic mid-frequency noise (stable up to ±41mV at 33Hz vs ±17mV at α=0.25).
    static constexpr float InputFilterAlpha = 0.10f;   // ~26ms TC at 120BPM
    static constexpr float FilterSnapVolts  = 0.05f;   // jumps >50mV bypass IIR and snap instantly
    static constexpr float HysteresisVolts  = 0.012f;  // ~14% of semitone
    static constexpr uint32_t SampleDelayTicks = 4;    // ~10ms at 120BPM
    // Eurorack gate thresholds for CvGate trigger mode
    static constexpr float CvGateThresholdHigh = 2.5f;
    static constexpr float CvGateThresholdLow  = 1.0f;
};
