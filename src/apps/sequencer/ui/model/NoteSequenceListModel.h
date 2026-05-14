#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/NoteSequence.h"

class NoteSequenceListModel : public RoutableListModel {
public:
    // Config-page-visible items come first (rows() returns ConfigPageRows).
    // Items after ConfigPageRows are quick-edit only (accessible by row index
    // from the edit page Page+Step8..12 overlay).
    enum Item {
        Name        = 0,
        ConfigPageRows = 1,  // only Name shows on the config page
        // quick-edit only:
        FirstStep   = 1,
        LastStep    = 2,
        RunMode     = 3,
        Divisor     = 4,
        ResetMeasure = 5,
        Last        = 6,
    };

    NoteSequenceListModel() {}

    void setSequence(NoteSequence *sequence) {
        _sequence = sequence;
    }

    virtual int rows() const override {
        return _sequence ? ConfigPageRows : 0;
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

    virtual int indexedCount(int row) const override {
        switch (Item(row)) {
        case FirstStep:
        case LastStep:
            return 16;
        case RunMode:
            return int(Types::RunMode::Last);
        case Divisor:
        case ResetMeasure:
            return 16;
        default:
            break;
        }
        return -1;
    }

    virtual int indexed(int row) const override {
        switch (Item(row)) {
        case FirstStep:
            return _sequence->firstStep();
        case LastStep:
            return _sequence->lastStep();
        case RunMode:
            return int(_sequence->runMode());
        case Divisor:
            return _sequence->indexedDivisor();
        case ResetMeasure:
            return _sequence->resetMeasure();
        default:
            break;
        }
        return -1;
    }

    virtual void setIndexed(int row, int index) override {
        switch (Item(row)) {
        case FirstStep:
            return _sequence->setFirstStep(index);
        case LastStep:
            return _sequence->setLastStep(index);
        case RunMode:
            return _sequence->setRunMode(Types::RunMode(index));
        case Divisor:
            return _sequence->setIndexedDivisor(index);
        case ResetMeasure:
            return _sequence->setResetMeasure(index);
        default:
            break;
        }
    }

    virtual Routing::Target routingTarget(int row) const override {
        switch (Item(row)) {
        case FirstStep:
            return Routing::Target::FirstStep;
        case LastStep:
            return Routing::Target::LastStep;
        case RunMode:
            return Routing::Target::RunMode;
        case Divisor:
            return Routing::Target::Divisor;
        default:
            return Routing::Target::None;
        }
    }

    void setSelectedScale(int defaultScale, bool force = false) override {}

private:
    static const char *itemName(Item item) {
        switch (item) {
        case Name:          return "Name";
        case FirstStep:     return "First Step";
        case LastStep:      return "Last Step";
        case RunMode:       return "Run Mode";
        case Divisor:       return "Divisor";
        case ResetMeasure:  return "Reset Measure";
        case Last:          break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        switch (item) {
        case Name:
            str(_sequence->name());
            break;
        case FirstStep:
            _sequence->printFirstStep(str);
            break;
        case LastStep:
            _sequence->printLastStep(str);
            break;
        case RunMode:
            _sequence->printRunMode(str);
            break;
        case Divisor:
            _sequence->printDivisor(str);
            break;
        case ResetMeasure:
            _sequence->printResetMeasure(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case Name:
            break;
        case FirstStep:
            _sequence->editFirstStep(value, shift);
            break;
        case LastStep:
            _sequence->editLastStep(value, shift);
            break;
        case RunMode:
            _sequence->editRunMode(value, shift);
            break;
        case Divisor:
            _sequence->editDivisor(value, shift);
            break;
        case ResetMeasure:
            _sequence->editResetMeasure(value, shift);
            break;
        case Last:
            break;
        }
    }

    NoteSequence *_sequence = nullptr;
};
