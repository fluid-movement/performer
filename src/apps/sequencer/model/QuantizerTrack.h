#pragma once

#include "BaseTrackPatternFollow.h"
#include "Config.h"
#include "Types.h"
#include "NoteSequence.h"
#include "Serialize.h"
#include "Routing.h"
#include "FileDefs.h"
#include "core/utils/StringUtils.h"
#include "BaseTrack.h"
#include <cstdint>


class QuantizerTrack : public BaseTrack, public BaseTrackPatternFollow {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    typedef std::array<NoteSequence, CONFIG_PATTERN_COUNT + CONFIG_SNAPSHOT_COUNT> NoteSequenceArray;

    // InputSource: 0-3 = CV In 1-4, 4-11 = Track 1-8
    enum class InputSource : uint8_t {
        CvIn1 = 0,
        CvIn2,
        CvIn3,
        CvIn4,
        Track1,
        Track2,
        Track3,
        Track4,
        Track5,
        Track6,
        Track7,
        Track8,
        Last
    };

    static const char *inputSourceName(InputSource source) {
        switch (source) {
        case InputSource::CvIn1:  return "CV In 1";
        case InputSource::CvIn2:  return "CV In 2";
        case InputSource::CvIn3:  return "CV In 3";
        case InputSource::CvIn4:  return "CV In 4";
        case InputSource::Track1: return "Track 1";
        case InputSource::Track2: return "Track 2";
        case InputSource::Track3: return "Track 3";
        case InputSource::Track4: return "Track 4";
        case InputSource::Track5: return "Track 5";
        case InputSource::Track6: return "Track 6";
        case InputSource::Track7: return "Track 7";
        case InputSource::Track8: return "Track 8";
        case InputSource::Last:   break;
        }
        return nullptr;
    }

    static bool inputSourceIsCvIn(InputSource source) {
        return int(source) < 4;
    }

    static int inputSourceCvIndex(InputSource source) {
        return int(source);
    }

    static int inputSourceTrackIndex(InputSource source) {
        return int(source) - 4;
    }

    // TriggerMode
    enum class TriggerMode : uint8_t {
        Free,
        Internal,
        External,
        Last
    };

    static const char *triggerModeName(TriggerMode mode) {
        switch (mode) {
        case TriggerMode::Free:     return "Free";
        case TriggerMode::Internal: return "Internal";
        case TriggerMode::External: return "External";
        case TriggerMode::Last:     break;
        }
        return nullptr;
    }

    //----------------------------------------
    // Properties
    //----------------------------------------

    const int trackIndex() const { return _trackIndex; }

    // inputSource

    InputSource inputSource() const { return _inputSource; }
    void setInputSource(InputSource source) {
        _inputSource = ModelUtils::clampedEnum(source);
    }

    void editInputSource(int value, bool shift) {
        setInputSource(ModelUtils::adjustedEnum(inputSource(), value));
    }

    void printInputSource(StringBuilder &str) const {
        str(inputSourceName(inputSource()));
    }

    // triggerMode

    TriggerMode triggerMode() const { return _triggerMode; }
    void setTriggerMode(TriggerMode mode) {
        _triggerMode = ModelUtils::clampedEnum(mode);
    }

    void editTriggerMode(int value, bool shift) {
        setTriggerMode(ModelUtils::adjustedEnum(triggerMode(), value));
    }

    void printTriggerMode(StringBuilder &str) const {
        str(triggerModeName(triggerMode()));
    }

    // triggerTrack: which track provides the external gate (-1 = none, 0-7 = track index)

    int triggerTrack() const { return _triggerTrack; }
    void setTriggerTrack(int track) {
        _triggerTrack = clamp(track, -1, CONFIG_TRACK_COUNT - 1);
    }

    void editTriggerTrack(int value, bool shift) {
        setTriggerTrack(triggerTrack() + value);
    }

    void printTriggerTrack(StringBuilder &str) const {
        if (_triggerTrack < 0) {
            str("None");
        } else {
            str("Track %d", _triggerTrack + 1);
        }
    }

    // octave

    int octave() const { return _octave.get(isRouted(Routing::Target::Octave)); }
    void setOctave(int octave, bool routed = false) {
        _octave.set(clamp(octave, -10, 10), routed);
    }

    void editOctave(int value, bool shift) {
        if (!isRouted(Routing::Target::Octave)) {
            setOctave(octave() + value);
        }
    }

    void printOctave(StringBuilder &str) const {
        printRouted(str, Routing::Target::Octave);
        str("%+d", octave());
    }

    // transpose

    int transpose() const { return _transpose.get(isRouted(Routing::Target::Transpose)); }
    void setTranspose(int transpose, bool routed = false) {
        _transpose.set(clamp(transpose, -100, 100), routed);
    }

    void editTranspose(int value, bool shift) {
        if (!isRouted(Routing::Target::Transpose)) {
            setTranspose(transpose() + value);
        }
    }

    void printTranspose(StringBuilder &str) const {
        printRouted(str, Routing::Target::Transpose);
        str("%+d", transpose());
    }

    // sequences

    const NoteSequenceArray &sequences() const { return _sequences; }
          NoteSequenceArray &sequences()       { return _sequences; }

    const NoteSequence &sequence(int index) const { return _sequences[index]; }
          NoteSequence &sequence(int index)       { return _sequences[index]; }

    //----------------------------------------
    // Routing
    //----------------------------------------

    inline bool isRouted(Routing::Target target) const { return Routing::isRouted(target, _trackIndex); }
    inline void printRouted(StringBuilder &str, Routing::Target target) const { Routing::printRouted(str, target, _trackIndex); }
    void writeRouted(Routing::Target target, int intValue, float floatValue);

    //----------------------------------------
    // Methods
    //----------------------------------------

    QuantizerTrack() { clear(); }

    void clear();

    void write(VersionedSerializedWriter &writer) const;
    void read(VersionedSerializedReader &reader);

private:
    void setTrackIndex(int trackIndex) {
        _trackIndex = trackIndex;
        for (auto &sequence : _sequences) {
            sequence.setTrackIndex(trackIndex);
        }
    }

    int8_t _trackIndex = -1;
    InputSource _inputSource;
    TriggerMode _triggerMode;
    int8_t _triggerTrack;
    Routable<int8_t> _octave;
    Routable<int8_t> _transpose;

    NoteSequenceArray _sequences;

    friend class Track;
};
