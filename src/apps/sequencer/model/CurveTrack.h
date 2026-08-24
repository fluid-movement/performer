#pragma once

#include "BaseTrack.h"
#include "BaseTrackPatternFollow.h"
#include "Config.h"
#include "Types.h"
#include "CurveSequence.h"
#include "Serialize.h"
#include "Routing.h"
#include "FileDefs.h"
#include "core/utils/StringUtils.h"
#include <cstdint>

class CurveTrack : public BaseTrack, public BaseTrackPatternFollow {
public:
    //----------------------------------------
    // Types
    //----------------------------------------

    typedef std::array<CurveSequence, CONFIG_PATTERN_COUNT + CONFIG_SNAPSHOT_COUNT> CurveSequenceArray;

    // FillMode

    enum class FillMode : uint8_t {
        None,
        Variation,
        NextPattern,
        Invert,
        Last
    };

    static const char *fillModeName(FillMode fillMode) {
        switch (fillMode) {
        case FillMode::None:        return "None";
        case FillMode::Variation:   return "Variation";
        case FillMode::NextPattern: return "Next Pattern";
        case FillMode::Invert:      return "Invert";
        case FillMode::Last:        break;
        }
        return nullptr;
    }

    enum class MuteMode : uint8_t {
        LastValue,
        Zero,
        Min,
        Max,
        Last
    };

    static const char *muteModeName(MuteMode muteMode) {
        switch (muteMode) {
        case MuteMode::LastValue:   return "Last Value";
        case MuteMode::Zero:        return "0V";
        case MuteMode::Min:         return "Min";
        case MuteMode::Max:         return "Max";
        case MuteMode::Last:        break;
        }
        return nullptr;
    }

    // ShapeCurve - steepness of the exponential fall at the low end of SHPE

    enum class ShapeCurve : uint8_t {
        Gentle,
        Medium,
        Snappy,
        Last
    };

    static const char *shapeCurveName(ShapeCurve shapeCurve) {
        switch (shapeCurve) {
        case ShapeCurve::Gentle:    return "Gentle";
        case ShapeCurve::Medium:    return "Medium";
        case ShapeCurve::Snappy:    return "Snappy";
        case ShapeCurve::Last:      break;
        }
        return nullptr;
    }

    static float shapeCurveExponent(ShapeCurve shapeCurve) {
        switch (shapeCurve) {
        case ShapeCurve::Gentle:    return 4.f;
        case ShapeCurve::Medium:    return 6.f;
        case ShapeCurve::Snappy:    return 8.f;
        case ShapeCurve::Last:      break;
        }
        return 6.f;
    }

    //----------------------------------------
    // Properties
    //----------------------------------------

    // playMode

    Types::PlayMode playMode() const { return _playMode; }
    void setPlayMode(Types::PlayMode playMode) {
        _playMode = ModelUtils::clampedEnum(playMode);
    }

    void editPlayMode(int value, bool shift) {
        setPlayMode(ModelUtils::adjustedEnum(playMode(), value));
    }

    void printPlayMode(StringBuilder &str) const {
        str(Types::playModeName(playMode()));
    }

    // fillMode

    FillMode fillMode() const { return _fillMode; }
    void setFillMode(FillMode fillMode) {
        _fillMode = ModelUtils::clampedEnum(fillMode);
    }

    void editFillMode(int value, bool shift) {
        setFillMode(ModelUtils::adjustedEnum(fillMode(), value));
    }

    void printFillMode(StringBuilder &str) const {
        str(fillModeName(fillMode()));
    }

    // muteMode

    MuteMode muteMode() const { return _muteMode; }
    void setMuteMode(MuteMode muteMode) {
        _muteMode = ModelUtils::clampedEnum(muteMode);
    }

    void editMuteMode(int value, bool shift) {
        setMuteMode(ModelUtils::adjustedEnum(muteMode(), value));
    }

    void printMuteMode(StringBuilder &str) const {
        str(muteModeName(muteMode()));
    }

    // shapeCurve

    ShapeCurve shapeCurve() const { return _shapeCurve; }
    void setShapeCurve(ShapeCurve shapeCurve) {
        _shapeCurve = ModelUtils::clampedEnum(shapeCurve);
    }

    float shapeCurveExponent() const { return shapeCurveExponent(_shapeCurve); }

    void editShapeCurve(int value, bool shift) {
        setShapeCurve(ModelUtils::adjustedEnum(shapeCurve(), value));
    }

    void printShapeCurve(StringBuilder &str) const {
        str(shapeCurveName(shapeCurve()));
    }

    // range - output voltage range, unipolar 0V..max in 1V steps

    Types::VoltageRange range() const { return _range; }
    void setRange(Types::VoltageRange range) {
        // the curve track is unipolar only; a bipolar value maps to the unipolar
        // entry of the same voltage rather than clamping to the top of the enum
        int index = int(range);
        if (index >= int(Types::VoltageRange::Bipolar1V)) {
            index -= int(Types::VoltageRange::Bipolar1V);
        }
        _range = Types::VoltageRange(clamp(index,
            int(Types::VoltageRange::Unipolar1V), int(Types::VoltageRange::Unipolar5V)));
    }

    void editRange(int value, bool shift) {
        // clamp as an int: VoltageRange is uint8_t backed, so a negative value
        // would wrap to 255 before the clamp in setRange could see it
        setRange(Types::VoltageRange(clamp(int(range()) + value,
            int(Types::VoltageRange::Unipolar1V), int(Types::VoltageRange::Unipolar5V))));
    }

    void printRange(StringBuilder &str) const {
        str("0..%.0fV", Types::voltageRangeInfo(range()).hi);
    }

    // slideTime

    int slideTime() const { return _slideTime.get(isRouted(Routing::Target::SlideTime)); }
    void setSlideTime(int slideTime, bool routed = false) {
        _slideTime.set(clamp(slideTime, 0, 100), routed);
    }

    void editSlideTime(int value, bool shift) {
        if (!isRouted(Routing::Target::SlideTime)) {
            setSlideTime(ModelUtils::adjustedByStep(slideTime(), value, 5, !shift));
        }
    }

    void printSlideTime(StringBuilder &str) const {
        printRouted(str, Routing::Target::SlideTime);
        str("%d%%", slideTime());
    }

    // offset

    int offset() const { return _offset.get(isRouted(Routing::Target::Offset)); }
    float offsetVolts() const { return offset() * 0.01f; }
    void setOffset(int offset, bool routed = false) {
        _offset.set(clamp(offset, -500, 500), routed);
    }

    void editOffset(int value, bool shift) {
        if (!isRouted(Routing::Target::Offset)) {
            setOffset(offset() + value * (shift ? 100 : 1));
        }
    }

    void printOffset(StringBuilder &str) const {
        printRouted(str, Routing::Target::Offset);
        str("%+.2fV", offsetVolts());
    }

    // rotate

    int rotate() const { return _rotate.get(isRouted(Routing::Target::Rotate)); }
    void setRotate(int rotate, bool routed = false) {
        _rotate.set(clamp(rotate, -64, 64), routed);
    }

    void editRotate(int value, bool shift) {
        if (!isRouted(Routing::Target::Rotate)) {
            setRotate(rotate() + value);
        }
    }

    void printRotate(StringBuilder &str) const {
        printRouted(str, Routing::Target::Rotate);
        str("%+d", rotate());
    }

    // curveCvInput

    Types::CurveCvInput curveCvInput() const { return _curveCvInput; }
    void setCurveCvInput(Types::CurveCvInput curveCvInput) {
        _curveCvInput = ModelUtils::clampedEnum(curveCvInput);
    }

    void editCurveCvInput(int value, bool shift) {
        _curveCvInput = ModelUtils::adjustedEnum(_curveCvInput, value);
    }

    void printCurveCvInput(StringBuilder &str) const {
        str(Types::curveCvInput(_curveCvInput));
    }

    // sequences

    const CurveSequenceArray &sequences() const { return _sequences; }
          CurveSequenceArray &sequences()       { return _sequences; }

    const CurveSequence &sequence(int index) const { return _sequences[index]; }
          CurveSequence &sequence(int index)       { return _sequences[index]; }

    void setSequence(int index, CurveSequence seq) {
        _sequences[index] = seq;
    }

    //----------------------------------------
    // Routing
    //----------------------------------------

    inline bool isRouted(Routing::Target target) const { return Routing::isRouted(target, _trackIndex); }
    inline void printRouted(StringBuilder &str, Routing::Target target) const { Routing::printRouted(str, target, _trackIndex); }
    void writeRouted(Routing::Target target, int intValue, float floatValue);

    //----------------------------------------
    // Methods
    //----------------------------------------

    CurveTrack() { clear(); }

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
    Types::PlayMode _playMode;
    FillMode _fillMode;
    MuteMode _muteMode;
    ShapeCurve _shapeCurve;
    Types::VoltageRange _range;
    Routable<uint8_t> _slideTime;
    Routable<int16_t> _offset;
    Routable<int8_t> _rotate;

    Types::CurveCvInput _curveCvInput;

    CurveSequenceArray _sequences;

    friend class Track;
};
