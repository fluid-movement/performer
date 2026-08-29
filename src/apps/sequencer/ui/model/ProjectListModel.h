#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "ui/painters/PreviewPainter.h"

#include "model/Project.h"

class ProjectListModel : public RoutableListModel {
public:
    ProjectListModel(Project &project) :
        _project(project)
    {
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
        static const char *names[] = { "PROJ", "SCALE", "MIDI", "REC", "CV" };
        str(names[section]);
    }

    virtual int rowSection(int row) const override {
        switch (Item(row)) {
        case Name:
            return 0;
        case Swing:
            return 0;
        case TimeSignature:
            return 0;
        case SyncMeasure:
            return 0;
        case Scale:
            return 1;
        case RootNote:
            return 1;
        case MidiInput:
            return 2;
        case MidiIntegrationMode:
            return 2;
        case MidiProgramOffset:
            return 2;
        case MonitorMode:
            return 3;
        case RecordMode:
            return 3;
        case RecordDelay:
            return 3;
        case StepsToStop:
            return 3;
        case CvGateInput:
            return 4;
        case ResetCvOnStop:
            return 4;
        case MultiCvRec:
            return 4;
        default:
            return 0;
        }
    }

    virtual bool hasPreview(int row) const override {
        switch (Item(row)) {
        case Swing: case Scale: case RootNote:
            return true;
        default:
            return false;
        }
    }

    virtual void drawPreview(Canvas &canvas, int row, int x, int y, int w, int h) const override {
        switch (Item(row)) {
        case Swing:
            PreviewPainter::drawSwingGrid(canvas, x, y, w, h, _project.swing());
            break;
        case Scale:
            PreviewPainter::drawKeyboard(canvas, x, y, w, h, _project.rootNote(), -1, _project.scale());
            break;
        case RootNote:
            PreviewPainter::drawKeyboard(canvas, x, y, w, h, 0, _project.rootNote(), -1);
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
        case Swing:
            return Routing::Target::Swing;
        default:
            return Routing::Target::None;
        }
    }

    void initScale() {
        _selectedScale = 0;
    }

    void resetScale() {
        _selectedScale = _project.scale();
    }

    enum Item {
        Name,
        Swing,
        TimeSignature,
        SyncMeasure,
        Scale,
        RootNote,
        MonitorMode,
        RecordMode,
        MidiInput,
        MidiIntegrationMode,
        MidiProgramOffset,
        CvGateInput,
        StepsToStop,
        RecordDelay,
        ResetCvOnStop,
        MultiCvRec,
        //CurveCvInput,
        Last
    };

    static const char *itemName(Item item) {
        switch (item) {
        case Name:              return "NAME";
        case Swing:             return "SWING";
        case TimeSignature:     return "TIME SIG";
        case SyncMeasure:       return "SYNC";
        case Scale:             return "SCALE";
        case RootNote:          return "ROOT";
        case MonitorMode:       return "MONITOR";
        case RecordMode:        return "RECORD";
        case MidiInput:         return "MIDI IN";
        case MidiIntegrationMode:   return "MIDI INT";
        case MidiProgramOffset:     return "PGM OFFS";
        case CvGateInput:       return "CV GATE";
        case StepsToStop:       return "STOP STP";
        case RecordDelay:       return "REC DLY";
        case ResetCvOnStop:     return "RESET CV";
        case MultiCvRec:        return "MULTI CV";
        //case CurveCvInput:      return "CURVE CV";
        case Last:              break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        switch (item) {
        case Name:
            str(_project.name());
            break;
        case Swing:
            _project.printSwing(str);
            break;
        case TimeSignature:
            _project.printTimeSignature(str);
            break;
        case SyncMeasure:
            _project.printSyncMeasure(str);
            break;
        case Scale:
            _project.printScale(str);
            break;
        case RootNote:
            _project.printRootNote(str);
            break;
        case MonitorMode:
            _project.printMonitorMode(str);
            break;
        case RecordMode:
            _project.printRecordMode(str);
            break;
        case MidiInput:
            _project.printMidiInput(str);
            break;
        case MidiIntegrationMode:
            _project.printMidiIntegrationMode(str);
            break;
        case MidiProgramOffset:
            _project.printMidiProgramOffset(str);
            break;
        case CvGateInput:
            _project.printCvGateInput(str);
            break;
        case StepsToStop:
            _project.printStepsToStop(str);
            break;
        case RecordDelay:
            _project.printRecordDelay(str);
            break;
        case ResetCvOnStop:
            _project.printResetCvOnStop(str);
            break;
        case MultiCvRec:
            _project.printUseMultiCvRec(str);
            break;
        //case CurveCvInput:
        //    _project.printCurveCvInput(str);
        //    break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case Name:
            break;
        case Swing:
            _project.editSwing(value, shift);
            break;
        case TimeSignature:
            _project.editTimeSignature(value, shift);
            break;
        case SyncMeasure:
            _project.editSyncMeasure(value, shift);
            break;
        case Scale:
            // Clamp against the real scale count; the old bound of 23 ran one
            // past the end of the 23-entry table.
            _selectedScale = clamp(_selectedScale + value, 0, Scale::Count - 1);
            _project.setScale(_selectedScale);
            break;
        case RootNote:
            _project.editRootNote(value, shift);
            break;
        case MonitorMode:
            _project.editMonitorMode(value, shift);
            break;
        case RecordMode:
            _project.editRecordMode(value, shift);
            break;
        case MidiInput:
            _project.editMidiInput(value, shift);
            break;
       case MidiIntegrationMode:
            _project.editMidiIntegrationMode(value, shift);
            break;
        case MidiProgramOffset:
            _project.editMidiProgramOffset(value, shift);
            break;
        case CvGateInput:
            _project.editCvGateInput(value, shift);
            break;
        case StepsToStop: 
            _project.editStepsToStop(value);
            break;
        case RecordDelay:
            _project.editRecordDelay(value);
            break;
        case ResetCvOnStop:
            _project.editResetCvOnStop(value);
            break;
        case MultiCvRec:
            _project.editUseMultiCvRec(value);
            break;
        //case CurveCvInput:
        //    _project.editCurveCvInput(value, shift);
        //    break;
        case Last:
            break;
        }
    
    }

    
    Project &_project;
    private:
        int _selectedScale = 0;
};
