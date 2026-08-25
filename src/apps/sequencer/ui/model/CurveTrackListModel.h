#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/CurveTrack.h"

class CurveTrackListModel : public RoutableListModel {
public:
    void setTrack(CurveTrack &track) {
        _track = &track;
    }

    virtual int rows() const override {
        return Last;
    }

    virtual int columns() const override {
        return 2;
    }

    virtual void cell(int row, int column, StringBuilder &str) const override {
        if (column == 0) {
            formatName(Item(row), str);
        } else if (column == 1) {
            formatValue(Item(row), str);
        }
    }

    virtual void edit(int row, int column, int value, bool shift) override {
        if (column == 1) {
            editValue(Item(row), value, shift);
        }
    }

    virtual Routing::Target routingTarget(int row) const override {
        switch (Item(row)) {
        case SlideTime:
            return Routing::Target::SlideTime;
        case Offset:
            return Routing::Target::Offset;
        case Rotate:
            return Routing::Target::Rotate;
        default:
            return Routing::Target::None;
        }
    }

private:
    enum Item {
        TrackName,
        MuteMode,
        ShapeCurve,
        Range,
        SlideTime,
        Offset,
        Rotate,
        PatternFollow,
        CurveCvInput,
        Last
    };

    static const char *itemName(Item item) {
        switch (item) {
        case TrackName:             return "Name";
        case MuteMode:              return "Mute Mode";
        case ShapeCurve:            return "Shape Curve";
        case Range:                 return "Range";
        case SlideTime:             return "Slide Time";
        case Offset:                return "Offset";
        case Rotate:                return "Rotate";
        case PatternFollow:         return "Pattern Follow";
        case CurveCvInput:          return "Curve CV Input";
        case Last:                  break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        switch (item) {
        case TrackName:
            str(_track->name());
            break;
        case MuteMode:
            _track->printMuteMode(str);
            break;
        case ShapeCurve:
            _track->printShapeCurve(str);
            break;
        case Range:
            _track->printRange(str);
            break;
        case SlideTime:
            _track->printSlideTime(str);
            break;
        case Offset:
            _track->printOffset(str);
            break;
        case Rotate:
            _track->printRotate(str);
            break;
        case PatternFollow:
            _track->printPatternFollow(str);
            break;
        case CurveCvInput:
            _track->printCurveCvInput(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case TrackName:
            break;
        case MuteMode:
            _track->editMuteMode(value, shift);
            break;
        case ShapeCurve:
            _track->editShapeCurve(value, shift);
            break;
        case Range:
            _track->editRange(value, shift);
            break;
        case SlideTime:
            _track->editSlideTime(value, shift);
            break;
        case Offset:
            _track->editOffset(value, shift);
            break;
        case Rotate:
            _track->editRotate(value, shift);
            break;
        case PatternFollow:
            _track->editPatternFollow(value, shift);
            break;
        case CurveCvInput:
            _track->editCurveCvInput(value, shift);
            break;
        case Last:
            break;
        }
    }

    virtual void setSelectedScale(int defaultScale, bool force = false) override {};

    CurveTrack *_track;
};
