#include "CurveSequence.h"
#include "ProjectVersion.h"
#include "ModelUtils.h"

Types::LayerRange CurveSequence::layerRange(Layer layer) {
    switch (layer) {
    case Layer::Shape:
    case Layer::Skew:
        return { 0, 127 };
    case Layer::Length:
        return { 1, 16 };
    case Layer::Level:
    case Layer::Offset:
        return { 0, 255 };
    case Layer::Last:
        break;
    }

    return { 0, 0 };
}

int CurveSequence::layerDefaultValue(Layer layer)
{
    switch (layer) {
    case Layer::Shape:
        return 64;
    case Layer::Skew:
        return 64;
    case Layer::Length:
        return 2;
    case Layer::Level:
        return 255;
    case Layer::Offset:
        return 0;
    case Layer::Last:
        break;
    }

    return 0;
}

int CurveSequence::Step::layerValue(Layer layer) const {
    switch (layer) {
    case Layer::Shape:  return _data0.shape;
    case Layer::Skew:   return _data0.shapeVariation;
    case Layer::Length: return length();
    case Layer::Level:  return _data0.max;
    case Layer::Offset: return _data0.min;
    case Layer::Last:   break;
    }
    return 0;
}

void CurveSequence::Step::setLayerValue(Layer layer, int value) {
    switch (layer) {
    case Layer::Shape:  _data0.shape = Shape::clamp(value); break;
    case Layer::Skew:   _data0.shapeVariation = Shape::clamp(value); break;
    case Layer::Length: setLength(value); break;
    case Layer::Level:  _data0.max = Max::clamp(value); break;
    case Layer::Offset: _data0.min = Min::clamp(value); break;
    case Layer::Last:   break;
    }
}

void CurveSequence::Step::clear() {
    _data0.raw = 0;
    _data1.raw = 0;
    setShapePercent(50);
    setSkewPercent(50);
    setOffsetPercent(0);
    setLevelPercent(100);
    setLength(2);
}

void CurveSequence::Step::write(VersionedSerializedWriter &writer) const {
    writer.write(_data0);
    writer.write(_data1);
}

void CurveSequence::Step::read(VersionedSerializedReader &reader) {
    if (reader.dataVersion() < ProjectVersion::Version15) {
        uint8_t shape, min, max;
        reader.read(shape);
        reader.read(min);
        reader.read(max);
        _data0.shape = shape;
        _data0.min = min;
        _data0.max = max;

        if (reader.dataVersion() < ProjectVersion::Version14) {
            if (_data0.shape <= 1) {
                _data0.shape = (_data0.shape + 1) % 2;
            }
        }
    } else {
        reader.read(_data0);
        reader.read(_data1);

        if (reader.dataVersion() < ProjectVersion::Version50) {
            // shape and skew widened from 6 to 7 bits, shifting skew's bit position.
            // Capture the old fields before rewriting, since the layouts overlap.
            uint32_t old = _data0.raw;
            int oldShape = old & 0x3f;
            int oldSkew  = (old >> 6) & 0x3f;
            int oldMin   = (old >> 16) & 0xff;
            int oldMax   = (old >> 24) & 0xff;
            _data0.raw = 0;
            _data0.shape          = (oldShape * 127 + 31) / 63;
            _data0.shapeVariation = (oldSkew  * 127 + 31) / 63;
            _data0.min = oldMin;
            _data0.max = oldMax;
        }
    }
}

void CurveSequence::writeRouted(Routing::Target target, int intValue, float floatValue) {
    switch (target) {
    case Routing::Target::Divisor:
        setDivisor(intValue, true);
        break;
    case Routing::Target::RunMode:
        setRunMode(Types::RunMode(intValue), true);
        break;
    case Routing::Target::FirstStep:
        setFirstStep(intValue, true);
        break;
    case Routing::Target::LastStep:
        setLastStep(intValue, true);
        break;
    default:
        break;
    }
}

void CurveSequence::clear() {
    setName("INIT");
    setRange(Types::VoltageRange::Bipolar5V);
    setDivisor(12);
    setResetMeasure(0);
    setRunMode(Types::RunMode::Forward);
    setFirstStep(0);
    setLastStep(15);
    _segmentCount = 1;

    clearSteps();
}

void CurveSequence::clearStepsSelected(const std::bitset<CONFIG_STEP_COUNT> &selected) {
    if (selected.any()) {
        for (size_t i = 0; i < CONFIG_STEP_COUNT; ++i) {
            if (selected[i]) {
                _steps[i].clear();
            }
    }
    } else {
        clearSteps();
    }
}

void CurveSequence::clearSteps() {
    for (auto &step : _steps) {
        step.clear();
    }
}

void CurveSequence::deleteSegment(int idx) {
    int n = segmentCount();
    if (idx < 0 || idx >= n) return;
    for (int i = idx; i < n - 1; ++i)
        _steps[i] = _steps[i + 1];
    _steps[n - 1].clear();
    setSegmentCount(n - 1);
}

void CurveSequence::duplicateSegment(int idx) {
    int n = segmentCount();
    if (idx < 0 || idx >= n || n >= 16) return;
    for (int i = n - 1; i > idx; --i)
        _steps[i + 1] = _steps[i];
    _steps[idx + 1] = _steps[idx];
    setSegmentCount(n + 1);
}

void CurveSequence::resetSegment(int idx) {
    if (idx < 0 || idx >= segmentCount()) return;
    _steps[idx].clear();
}

void CurveSequence::addSegment() {
    int n = segmentCount();
    if (n >= 16) return;
    _steps[n].clear();
    setSegmentCount(n + 1);
}

bool CurveSequence::isEdited() const {
    auto clearStep = Step();
    for (const auto &step : _steps) {
        if (step != clearStep) {
            return true;
        }
    }
    return false;
}

void CurveSequence::setShapes(std::initializer_list<int> shapes) {
    size_t step = 0;
    for (auto shape : shapes) {
        if (step < _steps.size()) {
            _steps[step++].setShape(shape);
        }
    }
}

void CurveSequence::shiftSteps(const std::bitset<CONFIG_STEP_COUNT> &selected, int direction) {
    if (selected.any()) {
        ModelUtils::shiftSteps(_steps, selected, firstStep(), lastStep(), direction);
    } else {
        ModelUtils::shiftSteps(_steps, firstStep(), lastStep(), direction);
    }
}

void CurveSequence::duplicateSteps() {
    ModelUtils::duplicateSteps(_steps, firstStep(), lastStep());
    setLastStep(lastStep() + (lastStep() - firstStep() + 1));
}

void CurveSequence::write(VersionedSerializedWriter &writer) const {
    writer.write(_range);
    writer.write(_divisor.base);
    writer.write(_resetMeasure);
    writer.write(_runMode.base);
    writer.write(_firstStep.base);
    writer.write(_lastStep.base);

    writeArray(writer, _steps);
    writer.write(_name, NameLength + 1);
    writer.write(_slot);
    writer.write(_segmentCount);
    writer.writeHash();
}

bool CurveSequence::read(VersionedSerializedReader &reader) {
    reader.read(_range);
    if (reader.dataVersion() < ProjectVersion::Version10) {
        reader.readAs<uint8_t>(_divisor.base);
    } else {
        reader.read(_divisor.base);
    }
    reader.read(_resetMeasure);
    reader.read(_runMode.base);
    reader.read(_firstStep.base);
    reader.read(_lastStep.base);

    readArray(reader, _steps);
    if (reader.dataVersion() >= ProjectVersion::Version35) {
        reader.read(_name, NameLength + 1, ProjectVersion::Version35);
        reader.read(_slot);
        if (reader.dataVersion() >= ProjectVersion::Version42) {
            reader.read(_segmentCount);
        } else {
            _segmentCount = 8;
        }
        bool success = reader.checkHash();
        if (!success) {
            clear();
        }
        return success;
    } else {
        return true;
    }
}
