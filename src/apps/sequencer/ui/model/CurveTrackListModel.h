#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "ui/painters/PreviewPainter.h"

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

    virtual int sections() const override {
        return 4;
    }

    virtual void sectionName(int section, StringBuilder &str) const override {
        static const char *names[] = { "TYPE", "SHPE", "TIME", "CV" };
        str(names[section]);
    }

    virtual int rowSection(int row) const override {
        switch (Item(row)) {
        case TrackName:
            return 0;
        case MuteMode:
            return 0;
        case PatternFollow:
            return 0;
        case ShapeCurve:
            return 1;
        case Range:
            return 1;
        case Offset:
            return 1;
        case SlideTime:
            return 2;
        case Rotate:
            return 2;
        case CurveCvInput:
            return 3;
        default:
            return 0;
        }
    }

    virtual bool hasPreview(int row) const override {
        switch (Item(row)) {
        case ShapeCurve: case Range: case Offset: case SlideTime:
            return true;
        default:
            return false;
        }
    }

    virtual void drawPreview(Canvas &canvas, int row, int x, int y, int w, int h) const override {
        switch (Item(row)) {
        case ShapeCurve:
            // shape 0 is where expCurve actually bites (see evalSegment), and a
            // fast attack makes it read as the percussive AD envelope this
            // setting exists to shape.
            PreviewPainter::drawCurveShape(canvas, x, y, w, h, 0.f, 0.2f, _track->shapeCurveExponent());
            break;
        case Range:
            PreviewPainter::drawVoltageBar(canvas, x, y, w, h,
                float(int(_track->range()) - int(Types::VoltageRange::Unipolar1V) + 1), 5.f);
            break;
        case Offset:
            PreviewPainter::drawBipolarBar(canvas, x, y, w, h, float(_track->offset()), 100.f);
            break;
        case SlideTime:
            PreviewPainter::drawSlideRamp(canvas, x, y, w, h, Types::SlideType::Linear, _track->slideTime() * 5);
            break;
        default:
            break;
        }
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
        case TrackName:             return "NAME";
        case MuteMode:              return "MUTE";
        case ShapeCurve:            return "SHAPE";
        case Range:                 return "RANGE";
        case SlideTime:             return "SLIDE TM";
        case Offset:                return "OFFSET";
        case Rotate:                return "ROTATE";
        case PatternFollow:         return "PAT FOLL";
        case CurveCvInput:          return "CURVE CV";
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


    CurveTrack *_track;
};
