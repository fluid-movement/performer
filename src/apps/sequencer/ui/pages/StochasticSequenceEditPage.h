#pragma once

#include "SequenceEditPageBase.h"

#include "ui/model/StochasticSequenceListModel.h"

#include "core/utils/Container.h"

class StochasticSequenceEditPage : public SequenceEditPageBase {
public:
    StochasticSequenceEditPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;
    virtual void midi(MidiEvent &event) override;

private:
    void switchTab(int functionKey);

    void drawNoteTab(Canvas &canvas, const StochasticSequence &sequence);
    void drawOctTab(Canvas &canvas, const StochasticSequence &sequence);
    void drawLenTab(Canvas &canvas, const StochasticSequence &sequence);
    void drawLoopTab(Canvas &canvas, const StochasticSequence &sequence);

    const ContextMenuModel::Item *contextItems() const override;
    int contextActionCount() const override;
    void contextAction(int index) override;
    bool contextActionEnabled(int index) const override;

    void initSequence();
    void copySequence();
    void pasteSequence();

    void displayMessage(StochasticSequence &sequence);

    int _cursor = 0;
    uint8_t _heldSteps = 0;

    int maxForTab() const {
        switch (_activeTab) {
        case 0: return 7;
        case 1: return 5;
        case 2: return 6;
        default: return 0;
        }
    }

    StochasticSequenceListModel _listModel;
};
