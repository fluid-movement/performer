#pragma once

#include "BasePage.h"

#include "ui/model/ArpSequenceListModel.h"

class ArpSequenceEditPage : public BasePage {
public:
    ArpSequenceEditPage(PageManager &manager, PageContext &context);

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
    int maxForTab() const;

    void drawNoteTab(Canvas &canvas, const ArpSequence &sequence);
    void drawRhythmTab(Canvas &canvas, const ArpSequence &sequence);
    void drawModTab(Canvas &canvas, const ArpSequence &sequence);
    void drawArpTab(Canvas &canvas, const ArpSequence &sequence);
    void drawCombinedEuclidean(Canvas &canvas, const ArpSequence &sequence, int playStep);
    void drawParamBar(Canvas &canvas, const char *labels[], const char *values[], int count, int selIdx);

    void switchTab(int fKey);
    int activeFunctionKey();

    void contextShow(bool doubleClick = false);
    void contextAction(int index);
    bool contextActionEnabled(int index) const;

    void initSequence();
    void copySequence();
    void pasteSequence();

    int _activeTab  = 0;
    uint8_t _heldSteps = 0;
    int _cursor     = 0;

    ArpSequenceListModel _listModel;
};
