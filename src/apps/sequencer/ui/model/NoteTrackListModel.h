#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "ui/painters/PreviewPainter.h"

#include "model/NoteTrack.h"

class NoteTrackListModel : public RoutableListModel {
public:

    NoteTrackListModel() {}

    void setTrack(NoteTrack &track) {
        _track = &track;
    }

    virtual int rows() const override {
        return Last;
    }

    virtual int columns() const override {
        return 2;
    }

    virtual int sections() const override {
        return 3;
    }

    virtual void sectionName(int section, StringBuilder &str) const override {
        static const char *names[] = { "TYPE", "TIME", "TUNE" };
        str(names[section]);
    }

    virtual int rowSection(int row) const override {
        switch (Item(row)) {
        case TrackName:
            return 0;
        case FillMode:
            return 0;
        case FillMuted:
            return 0;
        case PatternFollow:
            return 0;
        case GateMode:
            return 1;
        case SlideType:
            return 1;
        case SlideTime:
            return 1;
        case Octave:
            return 2;
        case Transpose:
            return 2;
        case Rotate:
            return 2;
        default:
            return 0;
        }
    }

    virtual bool hasPreview(int row) const override {
        switch (Item(row)) {
        case SlideType: case SlideTime: case Octave: case Transpose:
            return true;
        default:
            return false;
        }
    }

    virtual void drawPreview(Canvas &canvas, int row, int x, int y, int w, int h) const override {
        switch (Item(row)) {
        case SlideType:
        case SlideTime:
            PreviewPainter::drawSlideRamp(canvas, x, y, w, h, _track->slideType(), _track->slideTimeMs());
            break;
        case Octave:
            PreviewPainter::drawBipolarBar(canvas, x, y, w, h, float(_track->octave()), 10.f);
            break;
        case Transpose:
            PreviewPainter::drawKeyboard(canvas, x, y, w, h, 0, _track->transpose(), -1);
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
        case Octave:
            return Routing::Target::Octave;
        case Transpose:
            return Routing::Target::Transpose;
        case Rotate:
            return Routing::Target::Rotate;
        default:
            return Routing::Target::None;
        }
    }

private:
    enum Item {
        TrackName,
        FillMode,
        FillMuted,
        GateMode,
        SlideType,
        SlideTime,
        Octave,
        Transpose,
        Rotate,
        PatternFollow,
        Last
    };

    static const char *itemName(Item item) {
        switch (item) {
        case TrackName: return "NAME";
        case FillMode:  return "FILL";
        case FillMuted: return "FILLMUTE";
        case GateMode:  return "GATE";
        case SlideType: return "SLIDE";
        case SlideTime: return "SLIDE TM";
        case Octave:    return "OCTAVE";
        case Transpose: return "TRANSP";
        case Rotate:    return "ROTATE";
        case PatternFollow: return "PAT FOLL";
        case Last:      break;
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
        case FillMode:
            _track->printFillMode(str);
            break;
        case FillMuted:
            _track->printFillMuted(str);
            break;
        case GateMode:
            _track->printGateMode(str);
            break;
        case SlideType:
            _track->printSlideType(str);
            break;
        case SlideTime:
            _track->printSlideTime(str);
            break;
        case Octave:
            _track->printOctave(str);
            break;
        case Transpose:
            _track->printTranspose(str);
            break;
        case Rotate:
            _track->printRotate(str);
            break;
        case PatternFollow:
            _track->printPatternFollow(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {

        case TrackName:
            break;
        case FillMode:
            _track->editFillMode(value, shift);
            break;
        case FillMuted:
            _track->editFillMuted(value, shift);
            break;
        case GateMode:
            _track->editGateMode(value, shift);
            break;
        case SlideType:
            _track->editSlideType(value, shift);
            break;
        case SlideTime:
            _track->editSlideTime(value, shift);
            break;
        case Octave:
            _track->editOctave(value, shift);
            break;
        case Transpose:
            _track->editTranspose(value, shift);
            break;
        case Rotate:
            _track->editRotate(value, shift);
            break;
        case PatternFollow:
            _track->editPatternFollow(value, shift);
            break;
        case Last:
            break;
        }
    }


    NoteTrack *_track;
};
