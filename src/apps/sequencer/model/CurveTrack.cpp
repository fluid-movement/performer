#include "CurveTrack.h"
#include "ProjectVersion.h"
#include "Routing.h"

void CurveTrack::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::SlideTime:
        setSlideTime(intValue, true);
        break;
    case Routing::Target::Offset:
        setOffset(intValue, true);
        break;
    case Routing::Target::Rotate:
        setRotate(intValue, true);
        break;
    default:
        break;
    }
}

void CurveTrack::clear() {
    setMuteMode(MuteMode::LastValue);
    setShapeCurve(ShapeCurve::Medium);
    setRange(Types::VoltageRange::Unipolar5V);
    setSlideTime(0);
    setOffset(0);
    setRotate(0);
    setCurveCvInput(Types::CurveCvInput::Off);

    for (auto &sequence : _sequences) {
        sequence.clear();
    }
}


void CurveTrack::write(VersionedSerializedWriter &writer) const {
    // Fields removed in Version52 are still emitted, as zeroed placeholders, when
    // writing an older format, so a file stamped with that version keeps that
    // version's layout. Production always writes ProjectVersion::Latest.
    const bool legacy52 = writer.writerVersion() < ProjectVersion::Version52;
    const bool legacy53 = writer.writerVersion() < ProjectVersion::Version53;
    const int8_t legacyBias = 0;
    const uint8_t legacyMode = 0;
    const Routable<float> legacyLevel = {};

    writer.write(_name, NameLength + 1);
    if (legacy53) {
        writer.write(legacyMode);       // was _playMode
        writer.write(legacyMode);       // was _fillMode
    }
    writer.write(_muteMode);
    writer.write(_slideTime.base);
    writer.write(_offset.base);
    writer.write(_rotate.base);
    if (legacy52) {
        writer.write(legacyBias);       // was _shapeProbabilityBias
        writer.write(legacyBias);       // was _gateProbabilityBias
    }
    writer.write(_curveCvInput);
    if (legacy52) {
        writer.write(legacyLevel);      // was _min
        writer.write(legacyLevel);      // was _max
    }
    writeArray(writer, _sequences);
    writer.write(_patternFollow);
    // guarded so that writing at an older version produces a file an older reader
    // can consume - the test harness uses this to exercise the read migrations
    if (writer.writerVersion() >= ProjectVersion::Version51) {
        writer.write(_shapeCurve);
    }
    if (writer.writerVersion() >= ProjectVersion::Version52) {
        writer.write(_range);
    }
}

void CurveTrack::read(VersionedSerializedReader &reader) {
    reader.read(_name, NameLength + 1, ProjectVersion::Version33);
    // _playMode and _fillMode were never read by the V1 engine (it always uses
    // advanceFree, and _fillSequence was never used) and were removed in Version53
    reader.skip<uint8_t>(0, ProjectVersion::Version53);
    reader.skip<uint8_t>(0, ProjectVersion::Version53);
    reader.read(_muteMode, ProjectVersion::Version22);
    reader.read(_slideTime.base, ProjectVersion::Version8);
    reader.read(_offset.base, ProjectVersion::Version28);
    reader.read(_rotate.base);
    // _shapeProbabilityBias and _gateProbabilityBias were never read by the V1
    // engine and were removed in Version52
    reader.skip<int8_t>(ProjectVersion::Version15, ProjectVersion::Version52);
    reader.skip<int8_t>(ProjectVersion::Version15, ProjectVersion::Version52);
    reader.read(_curveCvInput, ProjectVersion::Version36);
    // _min and _max likewise; note they were written as whole Routable<float>
    // values (8 bytes each), not as .base like every other field
    reader.skip<Routable<float>>(ProjectVersion::Version37, ProjectVersion::Version52);
    reader.skip<Routable<float>>(ProjectVersion::Version37, ProjectVersion::Version52);
    readArray(reader, _sequences);
    reader.read(_patternFollow, ProjectVersion::Version39);
    reader.read(_shapeCurve, ProjectVersion::Version51);
    reader.read(_range, ProjectVersion::Version52);
}
