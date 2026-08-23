#include "QuantizerTrack.h"
#include "ProjectVersion.h"
#include "core/utils/StringBuilder.h"


void QuantizerTrack::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::Octave:
        setOctave(intValue, true);
        break;
    case Routing::Target::Transpose:
        setTranspose(intValue, true);
        break;
    default:
        break;
    }
}

void QuantizerTrack::clear() {
    setInputSource(InputSource::CvIn1);
    setTriggerMode(TriggerMode::Free);
    setTriggerTrack(-1);
    setOctave(0);
    setTranspose(0);
    setLoopLength(16);
    setLoopStart(0);

    for (auto &sequence : _sequences) {
        sequence.clear();
    }
}

void QuantizerTrack::write(VersionedSerializedWriter &writer) const {
    writer.write(_name, NameLength + 1);
    writer.write(_inputSource);
    writer.write(_triggerMode);
    writer.write(_triggerTrack);
    writer.write(_octave.base);
    writer.write(_transpose.base);
    writeArray(writer, _sequences);
    writer.write(_patternFollow);
    writer.write(_loopLength);
    writer.write(_loopStart);
}

void QuantizerTrack::read(VersionedSerializedReader &reader) {
    reader.read(_name, NameLength + 1, ProjectVersion::Version33);
    reader.read(_inputSource, ProjectVersion::Version40);
    reader.read(_triggerMode, ProjectVersion::Version40);
    reader.read(_triggerTrack, ProjectVersion::Version40);
    // Version48: Internal trigger mode removed; migrate old INT projects to Free.
    if (reader.dataVersion() < ProjectVersion::Version48 && _triggerMode == TriggerMode::Internal) {
        _triggerMode = TriggerMode::Free;
    }
    reader.read(_octave.base, ProjectVersion::Version40);
    reader.read(_transpose.base, ProjectVersion::Version40);
    readArray(reader, _sequences);
    reader.read(_patternFollow, ProjectVersion::Version40);
    reader.read(_loopLength, ProjectVersion::Version47);
    reader.read(_loopStart,  ProjectVersion::Version47);
}
