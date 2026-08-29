#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "ui/painters/PreviewPainter.h"

#include "model/ArpTrack.h"

class ArpTrackListModel : public RoutableListModel {
public:
    enum Item {
        TrackName,
        FillMode,
        FillMuted,
        CvUpdateMode,
        SlideTime,
        Octave,
        Transpose,
        GateProbabilityBias,
        RetriggerProbabilityBias,
        LengthBias,
        NoteProbabilityBias,
        PatternFollow,
        ArpeggiatorMode,
        ArpeggiatorHold,
        ArpeggiatorOctaves,
        ArpeggiatorGateLength,
        ArpeggiatorDivisor,
        Last
    };

    ArpTrackListModel() {
        for (int i = 0; i< 8; ++i) {
            _selectedTrack[i] = -1;
        }
    }

    void setTrack(ArpTrack &track) {
        _track = &track;
    }

    virtual int rows() const override {
        return Last;
    }

    virtual int columns() const override {
        return 2;
    }

    virtual int sections() const override {
        return 5;
    }

    virtual void sectionName(int section, StringBuilder &str) const override {
        static const char *names[] = { "TYPE", "TIME", "TUNE", "BIAS", "ARP" };
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
        case CvUpdateMode:
            return 0;
        case PatternFollow:
            return 0;
        case SlideTime:
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
        case ArpeggiatorMode:
            return 4;
        case ArpeggiatorHold:
            return 4;
        case ArpeggiatorOctaves:
            return 4;
        case ArpeggiatorGateLength:
            return 4;
        case ArpeggiatorDivisor:
            return 4;
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
    static const char *itemName(Item item) {
        switch (item) {
        case TrackName: return "NAME";
        case FillMode:  return "FILL";
        case FillMuted: return "FILLMUTE";
        case CvUpdateMode:  return "CV UPD";
        case SlideTime: return "SLIDE TM";
        case Octave:    return "OCTAVE";
        case Transpose: return "TRANSP";
        case GateProbabilityBias: return "GATE BIA";
        case RetriggerProbabilityBias: return "RTRG BIA";
        case LengthBias: return "LEN BIAS";
        case NoteProbabilityBias: return "NOTE BIA";
        case PatternFollow: return "PAT FOLL";
        case ArpeggiatorMode:       return "MODE";
        case ArpeggiatorHold:       return "HOLD";
        case ArpeggiatorOctaves:    return "OCTAVES";
        case ArpeggiatorDivisor:    return "RATE";
        case ArpeggiatorGateLength: return "GATE LEN";
        case Last:      break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        const auto &arpeggiator = _track->arpeggiator();
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
        case PatternFollow:
            _track->printPatternFollow(str);
            break;
        case ArpeggiatorMode:
            arpeggiator.printMode(str);
            break;
         case ArpeggiatorHold:
            arpeggiator.printHold(str);
            break;
        case ArpeggiatorOctaves:
            arpeggiator.printOctaves(str);
            break;
        case ArpeggiatorDivisor:
            arpeggiator.printDivisor(str);
            break;
        case ArpeggiatorGateLength:
            arpeggiator.printGateLength(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        auto &arpeggiator = _track->arpeggiator();
        switch (item) {

        case TrackName:
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
        case PatternFollow:
            _track->editPatternFollow(value, shift);
            break;
        case ArpeggiatorMode:
            arpeggiator.editMode(value, shift);
            break;
         case ArpeggiatorHold:
            arpeggiator.editHold(value, shift);
            break;
        case ArpeggiatorOctaves:
            arpeggiator.editOctaves(value, shift);
            break;
        case ArpeggiatorDivisor:
            arpeggiator.editDivisor(value, shift);
            break;
        case ArpeggiatorGateLength:
            arpeggiator.editGateLength(value, shift);
            break;
        case Last:
            break;
        }
    }


    ArpTrack *_track;

    int _selectedTrack[8];
};
