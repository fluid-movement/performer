#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "ui/painters/PreviewPainter.h"

#include "model/StochasticTrack.h"

class StochasticTrackListModel : public RoutableListModel {
public:
    void setTrack(StochasticTrack &track) {
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
        static const char *names[] = { "TYPE", "TIME", "TUNE", "BIAS" };
        str(names[section]);
    }

    virtual int rowSection(int row) const override {
        switch (Item(row)) {
        case TrackName:
            return 0;
        case PlayMode:
            return 0;
        case FillMode:
            return 0;
        case FillMuted:
            return 0;
        case CvUpdateMode:
            return 0;
        case SlideTime:
            return 1;
        case Rotate:
            return 1;
        case Octave:
            return 2;
        case Transpose:
            return 2;
        case GateProbabilityBias:
            return 3;
        case RetriggerProbabilityBias:
            return 3;
        case LengthBias:
            return 3;
        case NoteProbabilityBias:
            return 3;
        default:
            return 0;
        }
    }

    virtual bool hasPreview(int row) const override {
        switch (Item(row)) {
        case SlideTime: case Octave: case Transpose:
            return true;
        default:
            return false;
        }
    }

    virtual void drawPreview(Canvas &canvas, int row, int x, int y, int w, int h) const override {
        switch (Item(row)) {
        case SlideTime:
            PreviewPainter::drawSlideRamp(canvas, x, y, w, h, Types::SlideType::Linear, _track->slideTime() * 5);
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
        case GateProbabilityBias:
            return Routing::Target::GateProbabilityBias;
        case RetriggerProbabilityBias:
            return Routing::Target::RetriggerProbabilityBias;
        case LengthBias:
            return Routing::Target::LengthBias;
        case NoteProbabilityBias:
            return Routing::Target::NoteProbabilityBias;
        default:
            return Routing::Target::None;
        }
    }

private:
    enum Item {
        TrackName,
        PlayMode,
        FillMode,
        FillMuted,
        CvUpdateMode,
        SlideTime,
        Octave,
        Transpose,
        Rotate,
        GateProbabilityBias,
        RetriggerProbabilityBias,
        LengthBias,
        NoteProbabilityBias,
        Last
    };

    static const char *itemName(Item item) {
        switch (item) {
        case TrackName: return "NAME";
        case PlayMode:  return "PLAY";
        case FillMode:  return "FILL";
        case FillMuted: return "FILLMUTE";
        case CvUpdateMode:  return "CV UPD";
        case SlideTime: return "SLIDE TM";
        case Octave:    return "OCTAVE";
        case Transpose: return "TRANSP";
        case Rotate:    return "ROTATE";
        case GateProbabilityBias: return "GATE BIA";
        case RetriggerProbabilityBias: return "RTRG BIA";
        case LengthBias: return "LEN BIAS";
        case NoteProbabilityBias: return "NOTE BIA";
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
        case PlayMode:
            _track->printPlayMode(str);
            break;
        case FillMode:
            _track->printFillMode(str);
            break;
        case FillMuted:
            _track->printFillMuted(str);
            break;
        case CvUpdateMode:
            _track->printCvUpdateMode(str);
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
        case GateProbabilityBias:
            _track->printGateProbabilityBias(str);
            break;
        case RetriggerProbabilityBias:
            _track->printRetriggerProbabilityBias(str);
            break;
        case LengthBias:
            _track->printLengthBias(str);
            break;
        case NoteProbabilityBias:
            _track->printNoteProbabilityBias(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {

        case TrackName:
            break;
        case PlayMode:
            //_track->editPlayMode(value, shift);
            break;
        case FillMode:
            _track->editFillMode(value, shift);
            break;
        case FillMuted:
            _track->editFillMuted(value, shift);
            break;
        case CvUpdateMode:
            _track->editCvUpdateMode(value, shift);
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
        case GateProbabilityBias:
            _track->editGateProbabilityBias(value, shift);
            break;
        case RetriggerProbabilityBias:
            _track->editRetriggerProbabilityBias(value, shift);
            break;
        case LengthBias:
            _track->editLengthBias(value, shift);
            break;
        case NoteProbabilityBias:
            _track->editNoteProbabilityBias(value, shift);
            break;
        case Last:
            break;
        }
    }



    StochasticTrack *_track;
};
