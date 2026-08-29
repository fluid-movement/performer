#include "NoteTrack.h"
#include "ProjectVersion.h"
#include <string>
#include "core/utils/StringBuilder.h"


void NoteTrack::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::SlideTime:
        setSlideTime(intValue, true);
        break;
    case Routing::Target::Octave:
        setOctave(intValue, true);
        break;
    case Routing::Target::Transpose:
        setTranspose(intValue, true);
        break;
    case Routing::Target::Rotate:
        setRotate(intValue, true);
        break;
    default:
        break;
    }
}

void NoteTrack::clear() {
    setFillMode(FillMode::Gates);
    setFillMuted(true);
    setGateMode(GateMode::Retrigger);
    setSlideType(Types::SlideType::Exponential);
    setSlideTime(SlideTimeDefault);
    setOctave(0);
    setTranspose(0);
    setRotate(0);

    for (auto &sequence : _sequences) {
        sequence.clear();
    }
}


void NoteTrack::write(VersionedSerializedWriter &writer) const {
    // Fields removed in Version54 are still emitted, as zeroed placeholders, when
    // writing an older format, so a file stamped with that version keeps that
    // version's layout. Production always writes ProjectVersion::Latest.
    const bool legacy54 = writer.writerVersion() < ProjectVersion::Version54;
    const uint8_t legacyMode = 0;
    const int8_t legacyBias = 0;

    writer.write(_name, NameLength + 1);
    if (legacy54) {
        writer.write(legacyMode);       // was _playMode
    }
    writer.write(_fillMode);
    writer.write(_fillMuted);
    if (legacy54) {
        writer.write(legacyMode);       // was _cvUpdateMode
    } else {
        writer.write(_gateMode);
        writer.write(_slideType);
    }
    writer.write(_slideTime.base);
    writer.write(_octave.base);
    writer.write(_transpose.base);
    writer.write(_rotate.base);
    if (legacy54) {
        writer.write(legacyBias);       // was _gateProbabilityBias
        writer.write(legacyBias);       // was _retriggerProbabilityBias
        writer.write(legacyBias);       // was _lengthBias
        writer.write(legacyBias);       // was _noteProbabilityBias
    }
    writeArray(writer, _sequences);
    writer.write(_patternFollow);
}

void NoteTrack::read(VersionedSerializedReader &reader) {
    reader.backupHash();

    reader.read(_name, NameLength + 1, ProjectVersion::Version33);

    // _playMode removed in Version54 (variable step length is incompatible with
    // Types::PlayMode::Aligned -- the engine always advances freely now)
    reader.skip<Types::PlayMode>(0, ProjectVersion::Version54);
    reader.read(_fillMode);
    reader.read(_fillMuted, ProjectVersion::Version26);
    // _cvUpdateMode was added in Version4, removed in Version54 (a gate-off step
    // must not change the CV, so CvUpdateMode::Always no longer makes sense)
    reader.skip<uint8_t>(ProjectVersion::Version4, ProjectVersion::Version54);
    reader.read(_gateMode, ProjectVersion::Version54);
    reader.read(_slideType, ProjectVersion::Version54);
    reader.read(_slideTime.base);
    reader.read(_octave.base);
    reader.read(_transpose.base);
    reader.read(_rotate.base);
    // the four probability/length biases were removed in Version54 along with the
    // step parameters they biased
    reader.skip<int8_t>(0, ProjectVersion::Version54);
    reader.skip<int8_t>(0, ProjectVersion::Version54);
    reader.skip<int8_t>(0, ProjectVersion::Version54);
    reader.skip<int8_t>(0, ProjectVersion::Version54);
    // _logicTrack and _logicTrackInput were added in Version37, removed in Version41
    reader.skip<int8_t>(ProjectVersion::Version37, ProjectVersion::Version41);
    reader.skip<int8_t>(ProjectVersion::Version37, ProjectVersion::Version41);

    // There is a bug in previous firmware versions where writing the properties
    // of a note track did not update the hash value.
    if (reader.dataVersion() < ProjectVersion::Version23) {
        reader.restoreHash();
    }

    if (reader.dataVersion() < ProjectVersion::Version54) {
        // the old slide time parameterized an exponential lag with no defined
        // arrival time, so the stored value carries no meaning in the new model
        setSlideTime(SlideTimeDefault);
    }

    readArray(reader, _sequences);
    reader.read(_patternFollow, ProjectVersion::Version39);
}
