#pragma once

#include "BasePage.h"

#include "ui/StepSelection.h"
#include "ui/model/NoteSequenceListModel.h"
#include "ui/model/QuantizerTrackListModel.h"
#include "ui/KeyPressEventTracker.h"

#include "core/utils/Container.h"

class QuantizerSequenceEditPage : public BasePage {
public:
    QuantizerSequenceEditPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;
    virtual void midi(MidiEvent &event) override {}

private:
    static const int StepCount = 16;

    int stepOffset() const { return _project.selectedQuantizerSequence().section() * StepCount; }

    int maxForTab() const;

    void drawGateTab(Canvas &canvas, const NoteSequence &sequence);
    void drawSourceTab(Canvas &canvas, const QuantizerTrack &track);
    void drawTriggerTab(Canvas &canvas, const QuantizerTrack &track);
    void drawTuneTab(Canvas &canvas, const QuantizerTrack &track);

    void switchTab(int fKey);
    int activeFunctionKey();

    void contextShow(bool doubleClick = false);
    void contextAction(int index);
    bool contextActionEnabled(int index) const;

    void initSequence();
    void copySequence();
    void pasteSequence();
    void duplicateSequence();

    void quickEdit(int index);

    bool allSelectedStepsActive() const;
    void setSelectedStepsGate(bool gate);

    KeyPressEventTracker _keyPressEventTracker;

    NoteSequenceListModel _listModel;
    QuantizerTrackListModel _trackListModel;

    StepSelection<CONFIG_STEP_COUNT> _stepSelection;

    NoteSequence _inMemorySequence;

    int     _activeTab  = 0;
    uint8_t _heldSteps  = 0;
    int     _cursor     = 0;
};
