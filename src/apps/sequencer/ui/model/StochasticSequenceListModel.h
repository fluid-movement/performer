#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/StochasticSequence.h"

class StochasticSequenceListModel : public RoutableListModel {
public:
    // Config-page-visible items come first (rows() returns ConfigPageRows).
    // Items after ConfigPageRows are quick-edit only.
    enum Item {
        RestProbability2  = 0,
        RestProbability4  = 1,
        RestProbability8  = 2,
        LowOctaveRange    = 3,
        HighOctaveRange   = 4,
        LengthModifier    = 5,
        ConfigPageRows    = 6,  // config page shows rows 0-5
        // quick-edit only:
        RunMode           = 6,
        Divisor           = 7,
        ResetMeasure      = 8,
        SequenceFirstStep = 9,
        SequenceLastStep  = 10,
        Last              = 11,
    };

    StochasticSequenceListModel() {}

    void setSequence(StochasticSequence *sequence) {
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
            return _sequence->firstStep();
        case SequenceLastStep:
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
        case SequenceFirstStep:
            return _sequence->setFirstStep(index);
        case SequenceLastStep:
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
        case RestProbability2:
            return Routing::Target::RestProbability2;
        case RestProbability4:
            return Routing::Target::RestProbability4;
        case RestProbability8:
            return Routing::Target::RestProbability8;
        case LowOctaveRange:
            return Routing::Target::LowOctaveRange;
        case HighOctaveRange:
            return Routing::Target::HighOctaveRange;
        case LengthModifier:
            return Routing::Target::LengthModifier;
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
        case RestProbability2:  return "Rest Prob. 2";
        case RestProbability4:  return "Rest Prob. 4";
        case RestProbability8:  return "Rest Prob. 8";
        case LowOctaveRange:    return "L Oct Range";
        case HighOctaveRange:   return "H Oct Range";
        case LengthModifier:    return "Length Mod";
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
        case RestProbability2:
            _sequence->printRestProbability2(str);
            break;
        case RestProbability4:
            _sequence->printRestProbability4(str);
            break;
        case RestProbability8:
            _sequence->printRestProbability8(str);
            break;
        case LowOctaveRange:
            _sequence->printLowOctaveRange(str);
            break;
        case HighOctaveRange:
            _sequence->printHighOctaveRange(str);
            break;
        case LengthModifier:
            _sequence->printLengthModifier(str);
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
        case SequenceFirstStep:
            _sequence->printFirstStep(str);
            break;
        case SequenceLastStep:
            _sequence->printLastStep(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case RestProbability2:
            _sequence->editRestProbability2(value, shift);
            break;
        case RestProbability4:
            _sequence->editRestProbability4(value, shift);
            break;
        case RestProbability8:
            _sequence->editRestProbability8(value, shift);
            break;
        case LowOctaveRange:
            _sequence->editLowOctaveRange(value, shift);
            break;
        case HighOctaveRange:
            _sequence->editHighOctaveRange(value, shift);
            break;
        case LengthModifier:
            _sequence->editLengthModifier(value, shift);
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
        case SequenceFirstStep:
            _sequence->editFirstStep(value, shift);
            break;
        case SequenceLastStep:
            _sequence->editLastStep(value, shift);
            break;
        case Last:
            break;
        }
    }

    StochasticSequence *_sequence = nullptr;
};
