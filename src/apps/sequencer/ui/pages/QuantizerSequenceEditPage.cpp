#include "QuantizerSequenceEditPage.h"

#include "Pages.h"

#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"

#include "engine/QuantizerTrackEngine.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"

enum class QSeqContextAction {
    Init,
    Copy,
    Paste,
    Duplicate,
    Last
};

static const ContextMenuModel::Item qSeqContextMenuItems[] = {
    { "INIT" },
    { "COPY" },
    { "PASTE" },
    { "DUPL" },
};

static const NoteSequenceListModel::Item quickEditItems[8] = {
    NoteSequenceListModel::Item::FirstStep,
    NoteSequenceListModel::Item::LastStep,
    NoteSequenceListModel::Item::RunMode,
    NoteSequenceListModel::Item::Divisor,
    NoteSequenceListModel::Item::ResetMeasure,
    NoteSequenceListModel::Item::Scale,
    NoteSequenceListModel::Item::RootNote,
    NoteSequenceListModel::Item::Last
};

QuantizerSequenceEditPage::QuantizerSequenceEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
    _stepSelection.setStepCompare([this] (int a, int b) {
        const auto &sequence = _project.selectedQuantizerSequence();
        return sequence.step(a).gate() == sequence.step(b).gate();
    });
}

void QuantizerSequenceEditPage::enter() {
    _inMemorySequence = _project.selectedQuantizerSequence();
}

void QuantizerSequenceEditPage::exit() {
}

void QuantizerSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    const auto &track = _project.selectedTrack().quantizerTrack();
    const char* pf_repr = Types::patternFollowShortRepresentation(track.patternFollow());

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", pf_repr);
    WindowPainter::drawActiveFunction(canvas, "GATE");
    WindowPainter::drawFooter(canvas);

    auto &trackEngine = _engine.selectedTrackEngine().as<QuantizerTrackEngine>();
    auto &sequence = _project.selectedQuantizerSequence();

    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    const int stepWidth = Width / StepCount;
    const int stepOffset = this->stepOffset();
    const int loopY = 16;

    // Section auto-scroll
    if (_engine.state().running() && currentStep >= 0) {
        int section_no = currentStep / StepCount;
        if (section_no != sequence.section()) {
            sequence.setSecion(section_no);
        }
    }

    // loop points
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);
    SequencePainter::drawLoopStart(canvas, (sequence.firstStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);
    SequencePainter::drawLoopEnd(canvas, (sequence.lastStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);

    for (int i = 0; i < StepCount; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);

        int x = i * stepWidth;
        int y = 20;

        // loop connector
        if (stepIndex > sequence.firstStep() && stepIndex <= sequence.lastStep()) {
            canvas.setColor(Color::Bright);
            canvas.point(x, loopY);
        }

        // step index
        {
            canvas.setColor(_stepSelection[stepIndex] ? Color::Bright : Color::Medium);
            FixedStringBuilder<8> str("%d", stepIndex + 1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y - 2, str);
        }

        // gate box
        canvas.setColor(stepIndex == currentStep ? Color::Bright : Color::Medium);
        canvas.drawRect(x + 2, y + 2, stepWidth - 4, stepWidth - 4);
        if (step.gate()) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(x + 4, y + 4, stepWidth - 8, stepWidth - 8);
        }
    }
}

void QuantizerSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<QuantizerTrackEngine>();
    auto &sequence = _project.selectedQuantizerSequence();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    for (int i = 0; i < 16; ++i) {
        int stepIndex = stepOffset() + i;
        bool red = (stepIndex == currentStep) || _stepSelection[stepIndex];
        bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection[stepIndex]);
        leds.set(MatrixMap::fromStep(i), red, green);
    }

    LedPainter::drawSelectedSequenceSection(leds, sequence.section());

    // quick edit key indicators
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        for (int i = 0; i < 8; ++i) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            leds.set(index, false, quickEditItems[i] != NoteSequenceListModel::Item::Last);
            leds.mask(index);
        }
    }
}

void QuantizerSequenceEditPage::keyDown(KeyEvent &event) {
    _stepSelection.keyDown(event, stepOffset());
}

void QuantizerSequenceEditPage::keyUp(KeyEvent &event) {
    _stepSelection.keyUp(event, stepOffset());
}

void QuantizerSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedQuantizerSequence();

    if (key.isContextMenu()) {
        contextShow();
        event.consume();
        return;
    }
    if (key.pageModifier() && event.count() == 2) {
        contextShow(true);
        event.consume();
        return;
    }

    if (key.isQuickEdit()) {
        _inMemorySequence = _project.selectedQuantizerSequence();
        quickEdit(key.quickEdit());
        event.consume();
        return;
    }

    if (key.pageModifier() && key.is(Key::Step6)) {
        _project.setSelectedQuantizerSequence(_inMemorySequence);
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        return;
    }

    _stepSelection.keyPress(event, stepOffset());

    if (!key.shiftModifier() && key.isStep()) {
        int stepIndex = stepOffset() + key.step();
        _inMemorySequence = _project.selectedQuantizerSequence();
        sequence.step(stepIndex).toggleGate();
        event.consume();
    }

    KeyPressEvent keyPressEvent = _keyPressEventTracker.process(key);
    if (!key.shiftModifier() && key.isStep() && keyPressEvent.count() == 2) {
        // double-press already handled above
        event.consume();
    }

    if (key.isEncoder()) {
        _inMemorySequence = _project.selectedQuantizerSequence();
        if (!_stepSelection.any() || allSelectedStepsActive()) {
            setSelectedStepsGate(!allSelectedStepsActive());
        } else {
            setSelectedStepsGate(true);
        }
        event.consume();
    }

    if (key.isLeft()) {
        if (key.shiftModifier()) {
            _inMemorySequence = _project.selectedQuantizerSequence();
            sequence.shiftSteps(_stepSelection.selected(), -1);
            _stepSelection.shiftLeft(sequence.firstStep(), sequence.lastStep() + 1);
        } else {
            sequence.setSecion(std::max(0, sequence.section() - 1));
        }
        event.consume();
    }

    if (key.isRight()) {
        if (key.shiftModifier()) {
            _inMemorySequence = _project.selectedQuantizerSequence();
            sequence.shiftSteps(_stepSelection.selected(), 1);
            _stepSelection.shiftRight(sequence.firstStep(), sequence.lastStep() + 1);
        } else {
            sequence.setSecion(std::min(3, sequence.section() + 1));
        }
        event.consume();
    }
}

void QuantizerSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedQuantizerSequence();

    if (!_stepSelection.any()) {
        return;
    }

    for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            sequence.step(stepIndex).setGate(event.value() > 0);
        }
    }

    event.consume();
}

void QuantizerSequenceEditPage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        qSeqContextMenuItems,
        int(QSeqContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); },
        doubleClick
    ));
}

void QuantizerSequenceEditPage::contextAction(int index) {
    switch (QSeqContextAction(index)) {
    case QSeqContextAction::Init:
        initSequence();
        break;
    case QSeqContextAction::Copy:
        copySequence();
        break;
    case QSeqContextAction::Paste:
        pasteSequence();
        break;
    case QSeqContextAction::Duplicate:
        duplicateSequence();
        break;
    case QSeqContextAction::Last:
        break;
    }
}

bool QuantizerSequenceEditPage::contextActionEnabled(int index) const {
    switch (QSeqContextAction(index)) {
    case QSeqContextAction::Paste:
        return _model.clipBoard().canPasteNoteSequence();
    default:
        return true;
    }
}

void QuantizerSequenceEditPage::initSequence() {
    _project.selectedQuantizerSequence().clear();
    showMessage("SEQUENCE INITIALIZED");
}

void QuantizerSequenceEditPage::copySequence() {
    _model.clipBoard().copyNoteSequence(_project.selectedQuantizerSequence());
    showMessage("SEQUENCE COPIED");
}

void QuantizerSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteNoteSequence(_project.selectedQuantizerSequence());
    showMessage("SEQUENCE PASTED");
}

void QuantizerSequenceEditPage::duplicateSequence() {
    if (_project.selectedTrack().duplicatePattern(_project.selectedPatternIndex())) {
        showMessage("SEQUENCE DUPLICATED");
    }
}

void QuantizerSequenceEditPage::quickEdit(int index) {
    _listModel.setSequence(&_project.selectedQuantizerSequence());
    if (quickEditItems[index] != NoteSequenceListModel::Item::Last) {
        _manager.pages().quickEdit.show(_listModel, int(quickEditItems[index]));
    }
}

bool QuantizerSequenceEditPage::allSelectedStepsActive() const {
    const auto &sequence = _project.selectedQuantizerSequence();
    for (size_t i = 0; i < sequence.steps().size(); ++i) {
        if (_stepSelection[i] && !sequence.step(i).gate()) {
            return false;
        }
    }
    return true;
}

void QuantizerSequenceEditPage::setSelectedStepsGate(bool gate) {
    auto &sequence = _project.selectedQuantizerSequence();
    for (size_t i = 0; i < sequence.steps().size(); ++i) {
        if (_stepSelection[i]) {
            sequence.step(i).setGate(gate);
        }
    }
}
