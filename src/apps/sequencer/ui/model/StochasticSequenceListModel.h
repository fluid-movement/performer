#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/StochasticSequence.h"

class StochasticSequenceListModel : public RoutableListModel {
public:
    enum Item {
        // No config-page visible items in V2 — all prob editing is on the STEPS page
        ConfigPageRows    = 0,
        // quick-edit only:
        RunMode           = 0,
        Divisor           = 1,
        ResetMeasure      = 2,
        SequenceFirstStep = 3,
        SequenceLastStep  = 4,
        Last              = 5,
    };

    StochasticSequenceListModel() {}

    void setSequence(StochasticSequence *sequence) {
        _sequence = sequence;
    }

    virtual int rows() const override {
        return 0;  // no config-page rows in V2
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
        case SequenceFirstStep:
        case SequenceLastStep:
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
        case SequenceFirstStep:
            return _sequence->sequenceFirstStep();
        case SequenceLastStep:
            return _sequence->sequenceLastStep();
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
        case SequenceFirstStep:
            return _sequence->setSequenceFirstStep(index);
        case SequenceLastStep:
            return _sequence->setSequenceLastStep(index);
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
        case RunMode:           return "Run Mode";
        case Divisor:           return "Divisor";
        case ResetMeasure:      return "Reset Measure";
        case SequenceFirstStep: return "First Step";
        case SequenceLastStep:  return "Last Step";
        case Last:              break;
        }
        return nullptr;
    }

    void formatName(Item item, StringBuilder &str) const {
        str(itemName(item));
    }

    void formatValue(Item item, StringBuilder &str) const {
        switch (item) {
        case RunMode:
            _sequence->printRunMode(str);
            break;
        case Divisor:
            _sequence->printDivisor(str);
            break;
        case ResetMeasure:
            _sequence->printResetMeasure(str);
            break;
        case SequenceFirstStep:
            _sequence->printSequenceFirstStep(str);
            break;
        case SequenceLastStep:
            _sequence->printSequenceLastStep(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case RunMode:
            _sequence->editRunMode(value, shift);
            break;
        case Divisor:
            _sequence->editDivisor(value, shift);
            break;
        case ResetMeasure:
            _sequence->editResetMeasure(value, shift);
            break;
        case SequenceFirstStep:
            _sequence->editSequenceFirstStep(value, shift);
            break;
        case SequenceLastStep:
            _sequence->editSequenceLastStep(value, shift);
            break;
        case Last:
            break;
        }
    }

    StochasticSequence *_sequence = nullptr;
};
