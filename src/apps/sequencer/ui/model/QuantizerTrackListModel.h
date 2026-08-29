#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/QuantizerTrack.h"

class QuantizerTrackListModel : public RoutableListModel {
public:
    enum Item {
        TrackName,
        InputSource,
        TriggerSource,
        Octave,
        Transpose,
        PatternFollow,
        Last
    };

    void setTrack(QuantizerTrack &track) {
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
        static const char *names[] = { "TYPE", "SRCE", "TUNE" };
        str(names[section]);
    }

    virtual int rowSection(int row) const override {
        switch (Item(row)) {
        case TrackName:
            return 0;
        case PatternFollow:
            return 0;
        case InputSource:
            return 1;
        case TriggerSource:
            return 1;
        case Octave:
            return 2;
        case Transpose:
            return 2;
        default:
            return 0;
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
        case Octave:
            return Routing::Target::Octave;
        case Transpose:
            return Routing::Target::Transpose;
        default:
            return Routing::Target::None;
        }
    }

private:
    static const char *itemName(Item item) {
        switch (item) {
        case TrackName:     return "Name";
        case InputSource:   return "Input Source";
        case TriggerSource: return "Trigger Source";
        case Octave:        return "Octave";
        case Transpose:     return "Transpose";
        case PatternFollow: return "Pattern Follow";
        case Last:          break;
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
        case InputSource:
            _track->printInputSource(str);
            break;
        case TriggerSource:
            _track->printTriggerSource(str);
            break;
        case Octave:
            _track->printOctave(str);
            break;
        case Transpose:
            _track->printTranspose(str);
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
        case InputSource:
            _track->editInputSource(value, shift);
            break;
        case TriggerSource:
            _track->editTriggerSource(value, shift);
            break;
        case Octave:
            _track->editOctave(value, shift);
            break;
        case Transpose:
            _track->editTranspose(value, shift);
            break;
        case PatternFollow:
            _track->editPatternFollow(value, shift);
            break;
        case Last:
            break;
        }
    }


    QuantizerTrack *_track = nullptr;
};
