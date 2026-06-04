#include "ArpTrackEngine.h"

#include "Engine.h"
#include "Groove.h"
#include "Slide.h"
#include "SequenceUtils.h"

#include "core/Debug.h"
#include "core/utils/Random.h"
#include "core/math/Math.h"

#include "model/ArpSequence.h"
#include "model/Scale.h"

#include <algorithm>
#include <cstdint>

static Random rng;

// Bresenham Euclidean rhythm generator, canonical form.
// Rotates so the first hit is at index 0, then applies user rotation r.
// out[i] = true if a pulse falls on step i (length n).
static void computeEuclidean(bool *out, int n, int k, int r) {
    int bucket = 0;
    bool raw[16] = {};
    n = std::max(n, 1);
    k = std::min(k, n);
    for (int i = 0; i < n; i++) {
        bucket += k;
        if (bucket >= n) { bucket -= n; raw[i] = true; }
    }
    int firstHit = 0;
    if (k > 0) {
        for (int i = 0; i < n; i++) { if (raw[i]) { firstHit = i; break; } }
    }
    for (int i = 0; i < n; i++)
        out[i] = raw[((i + firstHit - r) % n + n) % n];
}

void ArpTrackEngine::reset() {
    _sequenceState.reset();
    _step = 0; _arpPhase = 0; _currentDegree = -1;
    _activity = false; _gateOutput = false;
    _cvOutput = _cvOutputTarget = 0.f;
    _slideActive = false;
    _gateQueue.clear(); _cvQueue.clear();
    if (_sequence) {
        recomputePatterns();
        rebuildNotePool();
    }
    changePattern();
}

void ArpTrackEngine::restart() {
    _sequenceState.reset();
    _step = 0; _arpPhase = 0; _currentDegree = -1;
}

void ArpTrackEngine::rebuildNotePool() {
    _activeNotes.clear();
    uint8_t mask = _sequence ? _sequence->degreeMask() : 0x01;
    for (int i = 0; i < 7; i++)
        if (mask & (1 << i)) _activeNotes.push_back(i);
}

void ArpTrackEngine::recomputePatterns() {
    if (!_sequence) return;
    const auto &seq = *_sequence;
    computeEuclidean(_rhythmPat, seq.rhythmN(), seq.rhythmK(), seq.rhythmR());
    computeEuclidean(_modPat,    seq.modN(),    seq.modK(),    seq.modR());
    _lastRhythmN = seq.rhythmN(); _lastRhythmK = seq.rhythmK(); _lastRhythmR = seq.rhythmR();
    _lastModN    = seq.modN();    _lastModK    = seq.modK();    _lastModR    = seq.modR();
}

float ArpTrackEngine::nextNoteCv() {
    const auto &seq = *_sequence;
    int n = int(_activeNotes.size());
    if (n == 0) return 0.f;
    int octaves = std::max(seq.arpOctaves(), 1);

    int noteIdx = 0;
    int octOff  = 0;

    switch (seq.arpOrder()) {
    case 1: { // DOWN
        int total = n * octaves;
        int phase = _arpPhase % total;
        noteIdx = (n - 1) - (phase % n);
        octOff  = octaves - 1 - (phase / n);
        _arpPhase = (_arpPhase + 1) % total;
        break;
    }
    case 2: { // UP-DN: bounce up then down, no repeated endpoint (period = 2*(n-1))
        int period = n > 1 ? 2 * (n - 1) : 1;
        int total  = period * octaves;
        int phase  = _arpPhase % total;
        int pos    = phase % period;
        noteIdx = pos < n ? pos : 2 * (n - 1) - pos;
        octOff  = phase / period;
        _arpPhase = (_arpPhase + 1) % total;
        break;
    }
    case 3: { // DN-UP: bounce down then up, no repeated endpoint
        int period = n > 1 ? 2 * (n - 1) : 1;
        int total  = period * octaves;
        int phase  = _arpPhase % total;
        int pos    = phase % period;
        noteIdx = pos < n ? (n - 1) - pos : pos - (n - 1);
        octOff  = phase / period;
        _arpPhase = (_arpPhase + 1) % total;
        break;
    }
    case 4: { // RAND: random note and octave each step
        noteIdx = int(rng.nextRange(n));
        octOff  = int(rng.nextRange(octaves));
        break;
    }
    case 5: { // CONV: converge from outside in (low, high, 2nd-low, 2nd-high, ...)
        int total = n * octaves;
        int phase = _arpPhase % total;
        int pos   = phase % n;
        octOff    = phase / n;
        noteIdx   = (pos % 2 == 0) ? pos / 2 : n - 1 - pos / 2;
        _arpPhase = (_arpPhase + 1) % total;
        break;
    }
    case 6: { // DIV: diverge from center out (mid, mid+1, mid-1, mid+2, mid-2, ...)
        int total = n * octaves;
        int phase = _arpPhase % total;
        int pos   = phase % n;
        octOff    = phase / n;
        int mid   = (n - 1) / 2;
        if (pos == 0)
            noteIdx = mid;
        else if (pos % 2 == 1)
            noteIdx = mid + (pos + 1) / 2;
        else
            noteIdx = mid - pos / 2;
        _arpPhase = (_arpPhase + 1) % total;
        break;
    }
    default: { // UP (0)
        int total = n * octaves;
        int phase = _arpPhase % total;
        noteIdx = phase % n;
        octOff  = phase / n;
        _arpPhase = (_arpPhase + 1) % total;
        break;
    }
    }

    int degree = _activeNotes[noteIdx];
    _currentDegree = degree;
    const auto &scale    = _model.project().selectedScale();
    int rootNote  = _model.project().rootNote();
    int trackOct  = _arpTrack.octave();
    int transpose = _arpTrack.transpose();
    int note = degree + (trackOct + octOff) * scale.notesPerOctave() + transpose;
    return scale.noteToVolts(note) + (scale.isChromatic() ? rootNote : 0) * (1.f / 12.f);
}

void ArpTrackEngine::triggerStep(uint32_t tick, uint32_t divisor) {
    if (!_sequence) return;
    const auto &seq = *_sequence;

    // Recompute patterns if params changed
    if (seq.rhythmN() != _lastRhythmN || seq.rhythmK() != _lastRhythmK || seq.rhythmR() != _lastRhythmR ||
        seq.modN() != _lastModN || seq.modK() != _lastModK || seq.modR() != _lastModR)
        recomputePatterns();

    int rn = std::max(seq.rhythmN(), 1);
    int mn = std::max(seq.modN(), 1);
    bool rhythmHit = _rhythmPat[_step % rn];
    bool modHit    = _modPat[_step % mn];
    bool overlap   = rhythmHit && modHit;

    _step = (_step + 1) % std::max(seq.arpLength(), 1);
    if (_step == 0) _arpPhase = 0;

    if (!rhythmHit) return;
    rebuildNotePool();
    if (_activeNotes.empty()) return;

    bool fires       = true;
    int  gateLenMult = 1;
    int  ratchets    = 1;

    switch (ArpSequence::ModMode(seq.modMode())) {
    case ArpSequence::ModMode::Off:     break;
    case ArpSequence::ModMode::Accent:  if (overlap) gateLenMult = 2; break;
    case ArpSequence::ModMode::Mask:    if (overlap) fires = false;   break;
    case ArpSequence::ModMode::Combine: if (!overlap) fires = false;  break;
    case ArpSequence::ModMode::Ratchet: if (overlap) ratchets = 2;   break;
    case ArpSequence::ModMode::Hold:    if (overlap) gateLenMult = 4; break;
    default: break;
    }
    if (!fires) return;

    float cv = nextNoteCv();

    uint32_t rawLen = (divisor * uint32_t((seq.rhythmGateLen() + 1) * 5)) / 100;
    rawLen = std::max(rawLen * uint32_t(gateLenMult), uint32_t(1));
    rawLen = std::min(rawLen, divisor - 1);

    for (int r = 0; r < ratchets; r++) {
        uint32_t onset = Groove::applySwing(tick + uint32_t(r) * (divisor / ratchets), swing());
        uint32_t len   = (ratchets > 1) ? rawLen / 2 : rawLen;
        _gateQueue.pushReplace({ onset,       true  });
        _gateQueue.pushReplace({ onset + len, false });
    }
    _cvQueue.push({ Groove::applySwing(tick, swing()), cv, false });

    _activity = true;
}

TrackEngine::TickResult ArpTrackEngine::tick(uint32_t tick) {
    ASSERT(_sequence != nullptr, "invalid sequence");
    const auto &sequence = *_sequence;
    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    if (linkData) {
        _linkData = *linkData;
        _sequenceState = *linkData->sequenceState;

        if (linkData->relativeTick % linkData->divisor == 0) {
            triggerStep(tick, linkData->divisor);
        }
    } else {
        uint32_t divisor = sequence.divisor() * (CONFIG_PPQN / CONFIG_SEQUENCE_PPQN);
        uint32_t resetDivisor = sequence.resetMeasure() * _engine.measureDivisor();
        uint32_t relativeTick = resetDivisor == 0 ? tick : tick % resetDivisor;

        if (int(_model.project().stepsToStop()) != 0 && int(relativeTick / divisor) == int(_model.project().stepsToStop())) {
            _engine.clockStop();
        }

        if (relativeTick == 0) {
            _step = 0;
            _arpPhase = 0;
            _gateQueue.clear();
            _cvQueue.clear();
        }

        switch (_arpTrack.playMode()) {
        case Types::PlayMode::Aligned:
            if (relativeTick % divisor == 0) {
                triggerStep(tick, divisor);
            }
            break;
        case Types::PlayMode::Free:  // not yet implemented
            break;
        case Types::PlayMode::Last:  // not yet implemented
            break;
        }

        _linkData.divisor = divisor;
        _linkData.relativeTick = relativeTick;
        _linkData.sequenceState = &_sequenceState;
    }

    auto &midiOutputEngine = _engine.midiOutputEngine();

    TickResult result = TickResult::NoUpdate;

    while (!_gateQueue.empty() && tick >= _gateQueue.front().tick) {
        if (!_monitorOverrideActive) {
            result |= TickResult::GateUpdate;
            _activity = _gateQueue.front().gate;
            _gateOutput = (!mute() || fill()) && _activity;
            midiOutputEngine.sendGate(_track.trackIndex(), _gateOutput);
        }
        _gateQueue.pop();
    }

    while (!_cvQueue.empty() && tick >= _cvQueue.front().tick) {
        if (!mute() || _arpTrack.cvUpdateMode() == ArpTrack::CvUpdateMode::Always) {
            if (!_monitorOverrideActive) {
                result |= TickResult::CvUpdate;
                _cvOutputTarget = _cvQueue.front().cv;
                _slideActive = _cvQueue.front().slide;
                midiOutputEngine.sendCv(_track.trackIndex(), _cvOutputTarget);
                midiOutputEngine.sendSlide(_track.trackIndex(), _slideActive);
            }
        }
        _cvQueue.pop();
    }

    return result;
}

void ArpTrackEngine::update(float dt) {
    if (_slideActive && _arpTrack.slideTime() > 0) {
        _cvOutput = Slide::applySlide(_cvOutput, _cvOutputTarget, _arpTrack.slideTime(), dt);
    } else {
        _cvOutput = _cvOutputTarget;
    }
}

void ArpTrackEngine::changePattern() {
    _sequence = &_arpTrack.sequence(pattern());
    if (_sequence) {
        recomputePatterns();
        rebuildNotePool();
    }
}

void ArpTrackEngine::monitorMidi(uint32_t /*tick*/, const MidiMessage &/*message*/) {
    // MIDI monitoring not used in Arp V2 — note pool is set via degreeMask
}

void ArpTrackEngine::clearMidiMonitoring() {
    // nothing to clear
}
