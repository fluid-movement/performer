#pragma once

#include "SequenceEditPageBase.h"

#include "ui/StepSelection.h"
#include "ui/model/CurveSequenceListModel.h"

#include "engine/generators/SequenceBuilder.h"

#include "core/utils/Container.h"

class CurveSequenceEditPage : public SequenceEditPageBase {
public:
    CurveSequenceEditPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;

private:
    typedef CurveSequence::Layer Layer;

    static const int StepCount = 16;

    int stepOffset() const { return _project.selectedCurveSequence().section() * StepCount; }

    void switchLayer(int functionKey);
    int activeFunctionKey() override;

    const ContextMenuModel::Item *contextItems() const override;
    int contextActionCount() const override;
    void contextAction(int index) override;
    bool contextActionEnabled(int index) const override;

    void initSequence();
    void copySequence();
    void pasteSequence();
    void duplicateSequence();
    void generateSequence();

    void quickEdit(int index);

    int selectionMode() const;
    void handleActiveSegmentAction(int f);

    CurveSequence::Layer layer() const { return _project.selectedCurveSequenceLayer(); }
    void setLayer(CurveSequence::Layer layer) { _project.setSelectedCurveSequenceLayer(layer); }

    ContextMenu _contextMenu;

    CurveSequenceListModel _listModel;

    StepSelection<CONFIG_STEP_COUNT> _stepSelection;

    Container<CurveSequenceBuilder> _builderContainer;

    CurveSequence _inMemorySequence;
};
