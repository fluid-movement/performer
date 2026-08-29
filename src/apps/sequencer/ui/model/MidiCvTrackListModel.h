#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "ui/painters/PreviewPainter.h"

#include "model/MidiCvTrack.h"

class MidiCvTrackListModel : public RoutableListModel {
public:
    void setTrack(MidiCvTrack &track) {
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
        static const char *names[] = { "PORT", "VOIC", "MOD", "ARP" };
        str(names[section]);
    }

    virtual int rowSection(int row) const override {
        switch (Item(row)) {
        case TrackName:
            return 0;
        case Source:
            return 0;
        case Voices:
            return 1;
        case VoiceConfig:
            return 1;
        case NotePriority:
            return 1;
        case LowNote:
            return 1;
        case HighNote:
            return 1;
        case PitchBendRange:
            return 2;
        case ModulationRange:
            return 2;
        case Retrigger:
            return 2;
        case SlideTime:
            return 2;
        case Transpose:
            return 2;
        case ArpeggiatorEnabled:
            return 3;
        case ArpeggiatorHold:
            return 3;
        case ArpeggiatorMode:
            return 3;
        case ArpeggiatorDivisor:
            return 3;
        case ArpeggiatorGateLength:
            return 3;
        case ArpeggiatorOctaves:
            return 3;
        default:
            return 0;
        }
    }

    virtual bool hasPreview(int row) const override {
        switch (Item(row)) {
        case LowNote: case HighNote: case SlideTime: case Transpose:
            return true;
        default:
            return false;
        }
    }

    virtual void drawPreview(Canvas &canvas, int row, int x, int y, int w, int h) const override {
        switch (Item(row)) {
        case LowNote:
        case HighNote:
            PreviewPainter::drawNoteRange(canvas, x, y, w, h, _track->lowNote(), _track->highNote());
            break;
        case SlideTime:
            PreviewPainter::drawSlideRamp(canvas, x, y, w, h, Types::SlideType::Linear, _track->slideTime() * 5);
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
        case Transpose:
            return Routing::Target::Transpose;
        default:
            return Routing::Target::None;
        }
    }

private:
    enum Item {
        TrackName,
        Source,
        Voices,
        VoiceConfig,
        NotePriority,
        LowNote,
        HighNote,
        PitchBendRange,
        ModulationRange,
        Retrigger,
        SlideTime,
        Transpose,
        ArpeggiatorEnabled,
        ArpeggiatorHold,
        ArpeggiatorMode,
        ArpeggiatorDivisor,
        ArpeggiatorGateLength,
        ArpeggiatorOctaves,
        Last
    };

    static const char *itemName(Item item) {
        switch (item) {
        case TrackName:             return "NAME";
        case Source:                return "SOURCE";
        case Voices:                return "VOICES";
        case VoiceConfig:           return "VOICECFG";
        case NotePriority:          return "PRIORITY";
        case LowNote:               return "LOW NOTE";
        case HighNote:              return "HI NOTE";
        case PitchBendRange:        return "P. BEND";
        case ModulationRange:       return "MOD RNG";
        case Retrigger:             return "RETRIG";
        case SlideTime:             return "SLIDE TM";
        case Transpose:             return "TRANSP";
        case ArpeggiatorEnabled:    return "ARP";
        case ArpeggiatorHold:       return "HOLD";
        case ArpeggiatorMode:       return "MODE";
        case ArpeggiatorDivisor:    return "DIVISOR";
        case ArpeggiatorGateLength: return "GATE LEN";
        case ArpeggiatorOctaves:    return "OCTAVES";
        case Last:                  break;
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
        case Source:
            _track->source().print(str);
            break;
        case Voices:
            _track->printVoices(str);
            break;
        case VoiceConfig:
            _track->printVoiceConfig(str);
            break;
        case NotePriority:
            _track->printNotePriority(str);
            break;
        case LowNote:
            _track->printLowNote(str);
            break;
        case HighNote:
            _track->printHighNote(str);
            break;
        case PitchBendRange:
            _track->printPitchBendRange(str);
            break;
        case ModulationRange:
            _track->printModulationRange(str);
            break;
        case Retrigger:
            _track->printRetrigger(str);
            break;
        case SlideTime:
            _track->printSlideTime(str);
            break;
        case Transpose:
            _track->printTranspose(str);
            break;
        case ArpeggiatorEnabled:
            arpeggiator.printEnabled(str);
            break;
        case ArpeggiatorHold:
            arpeggiator.printHold(str);
            break;
        case ArpeggiatorMode:
            arpeggiator.printMode(str);
            break;
        case ArpeggiatorDivisor:
            arpeggiator.printDivisor(str);
            break;
        case ArpeggiatorGateLength:
            arpeggiator.printGateLength(str);
            break;
        case ArpeggiatorOctaves:
            arpeggiator.printOctaves(str);
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
        case Source:
            _track->source().edit(value, shift);
            break;
        case Voices:
            _track->editVoices(value, shift);
            break;
        case VoiceConfig:
            _track->editVoiceConfig(value, shift);
            break;
        case NotePriority:
            _track->editNotePriority(value, shift);
            break;
        case LowNote:
            _track->editLowNote(value, shift);
            break;
        case HighNote:
            _track->editHighNote(value, shift);
            break;
        case PitchBendRange:
            _track->editPitchBendRange(value, shift);
            break;
        case ModulationRange:
            _track->editModulationRange(value, shift);
            break;
        case Retrigger:
            _track->editRetrigger(value, shift);
            break;
        case SlideTime:
            _track->editSlideTime(value, shift);
            break;
        case Transpose:
            _track->editTranspose(value, shift);
            break;
        case ArpeggiatorEnabled:
            arpeggiator.editEnabled(value, shift);
            break;
        case ArpeggiatorHold:
            arpeggiator.editHold(value, shift);
            break;
        case ArpeggiatorMode:
            arpeggiator.editMode(value, shift);
            break;
        case ArpeggiatorDivisor:
            arpeggiator.editDivisor(value, shift);
            break;
        case ArpeggiatorGateLength:
            arpeggiator.editGateLength(value, shift);
            break;
        case ArpeggiatorOctaves:
            arpeggiator.editOctaves(value, shift);
            break;
        case Last:
            break;
        }
    }

    
    MidiCvTrack *_track;
};
