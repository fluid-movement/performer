#include "StochasticSequence.h"
#include "ProjectVersion.h"

#include "Routing.h"

void StochasticSequence::writeRouted(Routing::Target target, int intValue, float /*floatValue*/) {
    switch (target) {
    case Routing::Target::Divisor:
        setDivisor(intValue, true);
        break;
    case Routing::Target::RunMode:
        setRunMode(Types::RunMode(intValue), true);
        break;
    case Routing::Target::Reseed:
        setReseed(intValue, true);
        break;
    case Routing::Target::SequenceFirstStep:
        setSequenceFirstStep(intValue, true);
        break;
    case Routing::Target::SequenceLastStep:
        setSequenceLastStep(intValue, true);
        break;
    default:
        break;
    }
}

void StochasticSequence::clear() {
    setDivisor(12);
    setResetMeasure(0);
    setRunMode(Types::RunMode::Forward);
    setSequenceFirstStep(0);
    setSequenceLastStep(15);
    setUseLoop(true);
    setReseed(false);
    _loopChance = 15;
    _loopLength = CONFIG_STEP_COUNT;

    _degreeProb[0] = 8;
    for (int i = 1; i < 7; i++) _degreeProb[i] = 0;
    for (int i = 0; i < 5; i++) _octaveProb[i]   = (i == 2) ? 15 : 0;
    for (int i = 0; i < 6; i++) _durationProb[i] = (i == 2) ? 15 : (i == 3) ? 8 : 0;
}

void StochasticSequence::write(VersionedSerializedWriter &writer) const {
    writer.write(_divisor.base);
    writer.write(_resetMeasure);
    writer.write(_runMode.base);
    writer.write(_sequenceFirstStep);
    writer.write(_sequenceLastStep);
    writer.write(_loopChance);
    writer.write(_degreeProb, sizeof(_degreeProb));
    writer.write(_octaveProb, sizeof(_octaveProb));
    writer.write(_durationProb, sizeof(_durationProb));
    writer.write(_loopLength);
}

void StochasticSequence::read(VersionedSerializedReader &reader) {
    // Discard pre-V41 per-sequence scale/rootNote (2 × int8_t)
    reader.skip(2, 0, ProjectVersion::Version41);

    // divisor (pre-V10: 1 byte uint8, V10+: 2 bytes uint16)
    if (reader.dataVersion() < ProjectVersion::Version10) {
        reader.readAs<uint8_t>(_divisor.base);
    } else {
        reader.read(_divisor.base);
    }
    reader.read(_resetMeasure);
    reader.read(_runMode.base);

    // Discard firstStep + lastStep (always written until V44, now removed)
    reader.skip(2, 0, ProjectVersion::Version44);

    // Discard V36+ fields: restProb2/4/8, lengthModifier, lowOctave, highOctave
    // Each is Routable<int8_t> = 2 bytes × 6 = 12 bytes
    reader.skip(12, ProjectVersion::Version36, ProjectVersion::Version44);

    // Keep: sequenceFirstStep/Last (V37+)
    reader.read(_sequenceFirstStep, ProjectVersion::Version37);
    reader.read(_sequenceLastStep, ProjectVersion::Version37);

    // Keep: loopChance (V43+)
    reader.read(_loopChance, ProjectVersion::Version43);

    // Discard old steps array
    // Pre-V27: uint32 + uint16 = 6 bytes per step
    reader.skip(CONFIG_STEP_COUNT * 6, 0, ProjectVersion::Version27);
    // V27–V44: uint32 + uint32 = 8 bytes per step
    reader.skip(CONFIG_STEP_COUNT * 8, ProjectVersion::Version27, ProjectVersion::Version44);

    // New V44+ probability arrays
    reader.read(_degreeProb, sizeof(_degreeProb), ProjectVersion::Version44);
    reader.read(_octaveProb, sizeof(_octaveProb), ProjectVersion::Version44);
    reader.read(_durationProb, sizeof(_durationProb), ProjectVersion::Version44);

    // V45+: explicit loop length (decoupled from firstStep/lastStep)
    reader.read(_loopLength, ProjectVersion::Version45);
}
