#pragma once

#include "Config.h"

#include "RoutableListModel.h"

#include "model/ArpSequence.h"

class ArpSequenceListModel : public RoutableListModel {
public:
    enum Item {
        Name           = 0,
        Divisor        = 1,
        ResetMeasure   = 2,
        ArpOrder       = 3,
        ArpOctaves     = 4,
        ArpLength      = 5,
        ConfigPageRows = 6,
        Last           = 6,
    };

    ArpSequenceListModel() {}

    void setSequence(ArpSequence *sequence) {
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
        case Divisor:
            return Routing::Target::Divisor;
        default:
            return Routing::Target::None;
        }
    }

    void setSelectedScale(int /*defaultScale*/, bool /*force*/ = false) override {}

private:
    static const char *itemName(Item item) {
        switch (item) {
        case Name:         return "Name";
        case Divisor:      return "Divisor";
        case ResetMeasure: return "Reset Measure";
        case ArpOrder:     return "Arp Order";
        case ArpOctaves:   return "Arp Octaves";
        case ArpLength:    return "Arp Length";
        case Last:         break;
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
        case Divisor:
            _sequence->printDivisor(str);
            break;
        case ResetMeasure:
            _sequence->printResetMeasure(str);
            break;
        case ArpOrder:
            _sequence->printArpOrder(str);
            break;
        case ArpOctaves:
            _sequence->printArpOctaves(str);
            break;
        case ArpLength:
            _sequence->printArpLength(str);
            break;
        case Last:
            break;
        }
    }

    void editValue(Item item, int value, bool shift) {
        switch (item) {
        case Name:
            break;
        case Divisor:
            _sequence->editDivisor(value, shift);
            break;
        case ResetMeasure:
            _sequence->editResetMeasure(value, shift);
            break;
        case ArpOrder:
            _sequence->editArpOrder(value, shift);
            break;
        case ArpOctaves:
            _sequence->editArpOctaves(value, shift);
            break;
        case ArpLength:
            _sequence->editArpLength(value, shift);
            break;
        case Last:
            break;
        }
    }

    ArpSequence *_sequence = nullptr;
};
