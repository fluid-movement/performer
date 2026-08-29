#pragma once

#include "ListModel.h"

#include "ui/painters/PreviewPainter.h"

#include "model/ClockSetup.h"

class ClockSetupListModel : public ListModel {
public:
    ClockSetupListModel(ClockSetup &clockSetup) :
        _clockSetup(clockSetup)
    {}

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
        static const char *names[] = { "MODE", "IN", "OUT", "MIDI" };
        str(names[section]);
    }

    virtual int rowSection(int row) const override {
        switch (Item(row)) {
        case Mode:
            return 0;
        case ShiftMode:
            return 0;
        case ClockInputDivisor:
            return 1;
        case ClockInputMode:
            return 1;
        case ClockOutputDivisor:
            return 2;
        case ClockOutputSwing:
            return 2;
        case ClockOutputPulse:
            return 2;
        case ClockOutputMode:
            return 2;
        case MidiRx:
            return 3;
        case MidiTx:
            return 3;
        case UsbRx:
            return 3;
        case UsbTx:
            return 3;
        case FilterNote:
            return 3;
        default:
            return 0;
        }
    }

    virtual bool hasPreview(int row) const override {
        switch (Item(row)) {
        case ClockInputDivisor: case ClockOutputDivisor: case ClockOutputPulse:
            return true;
        default:
            return false;
        }
    }

    virtual void drawPreview(Canvas &canvas, int row, int x, int y, int w, int h) const override {
        switch (Item(row)) {
        case ClockInputDivisor:
            PreviewPainter::drawPulseTrain(canvas, x, y, w, h, pulsesPerBar(_clockSetup.clockInputDivisor()));
            break;
        case ClockOutputDivisor:
            PreviewPainter::drawPulseTrain(canvas, x, y, w, h, pulsesPerBar(_clockSetup.clockOutputDivisor()));
            break;
        case ClockOutputPulse:
            // Pulse width against a nominal 100 ms window.
            PreviewPainter::drawPulseWidth(canvas, x, y, w, h, _clockSetup.clockOutputPulse() / 100.f);
            break;
        default:
            break;
        }
    }

    static int pulsesPerBar(int divisor) {
        return divisor > 0 ? clamp((CONFIG_PPQN * 4) / divisor, 1, 16) : 1;
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


private:
    enum Item {
        Mode,
        ShiftMode,
        ClockInputDivisor,
        ClockInputMode,
        ClockOutputDivisor,
        ClockOutputSwing,
        ClockOutputPulse,
        ClockOutputMode,
        MidiRx,
        MidiTx,
        UsbRx,
        UsbTx,
        FilterNote,
        Last
    };

    static const char *itemName(Item item) {
        switch (item) {
        case Mode:              return "MODE";
        case ShiftMode:         return "SHIFT";
        case ClockInputDivisor: return "IN DIV";
        case ClockInputMode:    return "IN MODE";
        case ClockOutputDivisor:return "OUT DIV";
        case ClockOutputSwing:  return "OUT SWNG";
        case ClockOutputPulse:  return "OUT PLSE";
        case ClockOutputMode:   return "OUT MODE";
        case MidiRx:            return "MIDI RX";
        case MidiTx:            return "MIDI TX";
        case UsbRx:             return "USB RX";
        case UsbTx:             return "USB TX";
        case FilterNote:        return "FILT NOT";
        case Last:              break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        switch (item) {
        case Mode:
            _clockSetup.printMode(str);
            break;
        case ShiftMode:
            _clockSetup.printShiftMode(str);
            break;
        case ClockInputDivisor:
            _clockSetup.printClockInputDivisor(str);
            break;
        case ClockInputMode:
            _clockSetup.printClockInputMode(str);
            break;
        case ClockOutputDivisor:
            _clockSetup.printClockOutputDivisor(str);
            break;
        case ClockOutputSwing:
            _clockSetup.printClockOutputSwing(str);
            break;
        case ClockOutputPulse:
            _clockSetup.printClockOutputPulse(str);
            break;
        case ClockOutputMode:
            _clockSetup.printClockOutputMode(str);
            break;
        case MidiRx:
            _clockSetup.printMidiRx(str);
            break;
        case MidiTx:
            _clockSetup.printMidiTx(str);
            break;
        case UsbRx:
            _clockSetup.printUsbRx(str);
            break;
        case UsbTx:
            _clockSetup.printUsbTx(str);
            break;
        case FilterNote:
            _clockSetup.printFilterNote(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case Mode:
            _clockSetup.editMode(value, shift);
            break;
        case ShiftMode:
            _clockSetup.editShiftMode(value, shift);
            break;
        case ClockInputDivisor:
            _clockSetup.editClockInputDivisor(value, shift);
            break;
        case ClockInputMode:
            _clockSetup.editClockInputMode(value, shift);
            break;
        case ClockOutputDivisor:
            _clockSetup.editClockOutputDivisor(value, shift);
            break;
        case ClockOutputSwing:
            _clockSetup.editClockOutputSwing(value, shift);
            break;
        case ClockOutputPulse:
            _clockSetup.editClockOutputPulse(value, shift);
            break;
        case ClockOutputMode:
            _clockSetup.editClockOutputMode(value, shift);
            break;
        case MidiRx:
            _clockSetup.editMidiRx(value, shift);
            break;
        case MidiTx:
            _clockSetup.editMidiTx(value, shift);
            break;
        case UsbRx:
            _clockSetup.editUsbRx(value, shift);
            break;
        case UsbTx:
            _clockSetup.editUsbTx(value, shift);
            break;
        case FilterNote:
            _clockSetup.editFilterNote(value, shift);
            break;
        case Last:
            break;
        }
    }

    ClockSetup &_clockSetup;
};
