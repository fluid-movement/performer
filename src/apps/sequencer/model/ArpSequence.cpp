#include "ArpSequence.h"
#include "ProjectVersion.h"

#include "ModelUtils.h"
#include "Routing.h"

static const char *arpOrderNames[] = {
    "UP", "DOWN", "UP-DN", "DN-UP", "RAND", "CONV", "DIV"
};

const char *ArpSequence::layerName(Layer layer) {
    switch (layer) {
    case Layer::RhythmSteps:   return "STEPS";
    case Layer::RhythmGates:   return "GATES";
    case Layer::RhythmRotate:  return "ROTATE";
    case Layer::RhythmGateLen: return "GATE LEN";
    case Layer::ModSteps:      return "MOD STEPS";
    case Layer::ModPulses:     return "MOD PULSES";
    case Layer::ModRotate:     return "MOD ROT";
    case Layer::ModMode:       return "MOD MODE";
    case Layer::ArpOrder:      return "ORDER";
    case Layer::ArpOctaves:    return "OCTAVES";
    case Layer::ArpLength:     return "LENGTH";
    case Layer::Last:          break;
    }
    return nullptr;
}

const char *ArpSequence::modModeName(ModMode mode) {
    switch (mode) {
    case ModMode::Off:     return "OFF";
    case ModMode::Accent:  return "ACCENT";
    case ModMode::Mask:    return "MASK";
    case ModMode::Combine: return "COMBINE";
    case ModMode::Ratchet: return "RATCHET";
    case ModMode::Hold:    return "HOLD";
    case ModMode::Last:    break;
    }
    return "OFF";
}

void ArpSequence::printArpOrder(StringBuilder &str) const {
    int o = arpOrder();
    if (o >= 0 && o < 7)
        str(arpOrderNames[o]);
    else
        str("%d", o);
}

void ArpSequence::writeRouted(Routing::Target target, int intValue, float /*floatValue*/) {
    switch (target) {
    case Routing::Target::Divisor:
        setDivisor(intValue, true);
        break;
    default:
        break;
    }
}

void ArpSequence::clear() {
    setName("INIT");
    setDivisor(12);
    _resetMeasure   = 0;
    _degreeMask     = 0x01;
    _rhythmN        = 16; _rhythmK = 5; _rhythmR = 0; _rhythmGateLen = 7;
    _modN           = 12; _modK = 3;    _modR = 0;    _modMode = 0;
    _arpOrder       = 0;  _arpOctaves = 1; _arpLength = 16;
}

void ArpSequence::write(VersionedSerializedWriter &writer) const {
    writer.write(_divisor.base);
    writer.write(_resetMeasure);
    writer.write(_degreeMask);
    writer.write(_rhythmN);
    writer.write(_rhythmK);
    writer.write(_rhythmR);
    writer.write(_rhythmGateLen);
    writer.write(_modN);
    writer.write(_modK);
    writer.write(_modR);
    writer.write(_modMode);
    writer.write(_arpOrder);
    writer.write(_arpOctaves);
    writer.write(_arpLength);
    writer.write(_name, NameLength + 1);
    writer.write(_slot);
    writer.writeHash();
}

bool ArpSequence::read(VersionedSerializedReader &reader) {
    if (reader.dataVersion() < ProjectVersion::Version46) {
        // Old format: discard old data and use defaults
        clear();
        return true;
    }
    reader.read(_divisor.base);
    reader.read(_resetMeasure);
    reader.read(_degreeMask);
    reader.read(_rhythmN);
    reader.read(_rhythmK);
    reader.read(_rhythmR);
    reader.read(_rhythmGateLen);
    reader.read(_modN);
    reader.read(_modK);
    reader.read(_modR);
    reader.read(_modMode);
    reader.read(_arpOrder);
    reader.read(_arpOctaves);
    reader.read(_arpLength);
    reader.read(_name, NameLength + 1);
    reader.read(_slot);
    bool success = reader.checkHash();
    if (!success) {
        clear();
    }
    return success;
}
