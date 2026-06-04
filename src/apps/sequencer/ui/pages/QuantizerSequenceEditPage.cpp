#include "QuantizerSequenceEditPage.h"

#include "Pages.h"

#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"

#include "engine/QuantizerTrackEngine.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"

static const char *functionNames[] = { "GATE", "SRCE", "TRIG", "TUNE", nullptr };

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
    NoteSequenceListModel::Item::Last,
    NoteSequenceListModel::Item::Last,
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
    _activeTab  = 0;
    _heldSteps  = 0;
    _cursor     = 0;
}

void QuantizerSequenceEditPage::exit() {
}

void QuantizerSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    const auto &track = _project.selectedTrack().quantizerTrack();
    const char *pf_repr = Types::patternFollowShortRepresentation(track.patternFollow());

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", pf_repr);
    WindowPainter::drawActiveFunction(canvas, functionNames[_activeTab]);
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    const auto &sequence = _project.selectedQuantizerSequence();

    switch (_activeTab) {
    case 0: drawGateTab(canvas, sequence);  break;
    case 1: drawSourceTab(canvas, track);   break;
    case 2: drawTriggerTab(canvas, track);  break;
    case 3: drawTuneTab(canvas, track);     break;
    }
}

void QuantizerSequenceEditPage::drawGateTab(Canvas &canvas, const NoteSequence &sequence) {
    auto &trackEngine = _engine.selectedTrackEngine().as<QuantizerTrackEngine>();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    const int stepWidth = Width / StepCount;
    const int stepOffset = this->stepOffset();
    const int loopY = 16;

    // Section auto-scroll
    if (_engine.state().running() && currentStep >= 0) {
        int section_no = currentStep / StepCount;
        if (section_no != sequence.section()) {
            const_cast<NoteSequence &>(sequence).setSecion(section_no);
        }
    }

    // Loop points
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);
    SequencePainter::drawLoopStart(canvas, (sequence.firstStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);
    SequencePainter::drawLoopEnd(canvas, (sequence.lastStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);

    for (int i = 0; i < StepCount; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);

        int x = i * stepWidth;
        int y = 20;

        // Loop connector
        if (stepIndex > sequence.firstStep() && stepIndex <= sequence.lastStep()) {
            canvas.setColor(Color::Bright);
            canvas.point(x, loopY);
        }

        // Step index
        {
            canvas.setColor(_stepSelection[stepIndex] ? Color::Bright : Color::Medium);
            FixedStringBuilder<8> str("%d", stepIndex + 1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y - 2, str);
        }

        // Gate box
        canvas.setColor(stepIndex == currentStep ? Color::Bright : Color::Medium);
        canvas.drawRect(x + 2, y + 2, stepWidth - 4, stepWidth - 4);
        if (step.gate()) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(x + 4, y + 4, stepWidth - 8, stepWidth - 8);
        }
    }
}

void QuantizerSequenceEditPage::drawSourceTab(Canvas &canvas, const QuantizerTrack &track) {
    canvas.setBlendMode(BlendMode::Set);
    const int src = int(track.inputSource());

    // CV IN section
    canvas.setFont(Font::Tiny);
    canvas.setColor(Color::Low);
    canvas.drawText(4, 13, "CV IN");

    static const char *cvLabels[] = { "CV1", "CV2", "CV3", "CV4" };
    for (int i = 0; i < 4; ++i) {
        const int bx = i * 64 + 4;
        const int bw = 56;
        const bool sel = (src == i);
        if (sel) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(bx, 17, bw, 17);
            canvas.setColor(Color::None);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(bx, 17, bw, 17);
            canvas.setColor(Color::Medium);
        }
        canvas.setFont(Font::Small);
        canvas.drawText(bx + (bw - canvas.textWidth(cvLabels[i])) / 2, 28, cvLabels[i]);
        canvas.setFont(Font::Tiny);
    }

    // TRACK section
    canvas.setColor(Color::Low);
    canvas.drawText(4, 38, "TRACK");

    // 8 chips centered: totalW = 8*28 + 7*2 = 238, startX = (256-238)/2 = 9
    const int trkW = 28, trkGap = 2, trkX0 = 9;
    for (int t = 0; t < 8; ++t) {
        const int bx  = trkX0 + t * (trkW + trkGap);
        const bool sel = (src == t + 4);
        if (sel) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(bx, 41, trkW, 11);
            canvas.setColor(Color::None);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(bx, 41, trkW, 11);
            canvas.setColor(Color::Medium);
        }
        FixedStringBuilder<4> lbl("T%d", t + 1);
        canvas.drawText(bx + (trkW - canvas.textWidth(lbl)) / 2, 49, lbl);
    }
}

void QuantizerSequenceEditPage::drawTriggerTab(Canvas &canvas, const QuantizerTrack &track) {
    canvas.setBlendMode(BlendMode::Set);
    const int mode = int(track.triggerMode());
    const int colW = 256 / 3;

    // "TRIGGER MODE" centered label
    canvas.setFont(Font::Tiny);
    canvas.setColor(Color::Low);
    const char *modeHdr = "TRIGGER MODE";
    canvas.drawText((256 - canvas.textWidth(modeHdr)) / 2, 13, modeHdr);

    // 3 mode chips
    static const char *modeNames[] = { "FREE", "INT", "EXT" };
    for (int i = 0; i < 3; ++i) {
        const int bx  = i * colW + 4;
        const int bw  = colW - 8;
        const bool sel = (mode == i);
        if (sel) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(bx, 17, bw, 14);
            canvas.setColor(Color::None);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(bx, 17, bw, 14);
            canvas.setColor(Color::Medium);
        }
        const int cx = i * colW + colW / 2;
        canvas.drawText(cx - canvas.textWidth(modeNames[i]) / 2, 27, modeNames[i]);
    }

    // Separator
    canvas.setColor(Color::Low);
    canvas.hline(0, 34, 256);

    // Sub-selector: trigger track (INT only)
    if (track.triggerMode() == QuantizerTrack::TriggerMode::Internal) {
        canvas.setFont(Font::Tiny);
        canvas.setColor(Color::Low);
        canvas.drawText(4, 40, "TRIGGER TRACK");

        FixedStringBuilder<16> trkStr;
        track.printTriggerTrack(trkStr);

        bool sel = (_heldSteps >> 1) & 1;
        canvas.setFont(Font::Small);
        canvas.setColor(sel ? Color::Bright : Color::Medium);
        canvas.drawText((256 - canvas.textWidth(trkStr)) / 2, 51, trkStr);
        canvas.setFont(Font::Tiny);
    }
}

void QuantizerSequenceEditPage::drawTuneTab(Canvas &canvas, const QuantizerTrack &track) {
    canvas.setBlendMode(BlendMode::Set);

    // Column divider
    canvas.setColor(Color::Low);
    canvas.vline(128, 8, 46);

    // ── Column 0: OCTAVE ──────────────────────────────────────────────────────
    {
        const int cx  = 64;
        const bool sel = (_heldSteps >> 0) & 1;

        canvas.setFont(Font::Tiny);
        canvas.setColor(Color::Low);
        canvas.drawText(cx - canvas.textWidth("OCTAVE") / 2, 16, "OCTAVE");

        // Bipolar bar: x=24, w=80, h=5, range -10..+10
        const int bx = 24, bw = 80, by = 26, bh = 5;
        canvas.setColor(Color::Low);
        canvas.fillRect(bx, by, bw, bh);
        const int oct = track.octave();
        if (oct != 0) {
            const int mid = bx + bw / 2;
            const int px  = bw / 2 * oct / 10;
            canvas.setColor(sel ? Color::Bright : Color::Medium);
            if (px > 0) canvas.fillRect(mid, by, px, bh);
            else        canvas.fillRect(mid + px, by, -px, bh);
        }
        canvas.setColor(Color::None);
        canvas.vline(bx + bw / 2, by, bh);

        FixedStringBuilder<8> octStr;
        track.printOctave(octStr);
        canvas.setFont(Font::Small);
        canvas.setColor(sel ? Color::Bright : Color::Medium);
        canvas.drawText(cx - canvas.textWidth(octStr) / 2, 44, octStr);
        canvas.setFont(Font::Tiny);
    }

    // ── Column 1: TRANSPOSE ───────────────────────────────────────────────────
    {
        const int cx  = 192;
        const bool sel = (_heldSteps >> 1) & 1;

        canvas.setFont(Font::Tiny);
        canvas.setColor(Color::Low);
        canvas.drawText(cx - canvas.textWidth("TRANSPOSE") / 2, 16, "TRANSPOSE");

        // Bipolar bar: x=152, w=80, h=5, range -100..+100
        const int bx = 152, bw = 80, by = 26, bh = 5;
        canvas.setColor(Color::Low);
        canvas.fillRect(bx, by, bw, bh);
        const int trp = track.transpose();
        if (trp != 0) {
            const int mid = bx + bw / 2;
            const int px  = bw / 2 * trp / 100;
            canvas.setColor(sel ? Color::Bright : Color::Medium);
            if (px > 0) canvas.fillRect(mid, by, px, bh);
            else        canvas.fillRect(mid + px, by, -px, bh);
        }
        canvas.setColor(Color::None);
        canvas.vline(bx + bw / 2, by, bh);

        FixedStringBuilder<8> trpStr;
        track.printTranspose(trpStr);
        canvas.setFont(Font::Small);
        canvas.setColor(sel ? Color::Bright : Color::Medium);
        canvas.drawText(cx - canvas.textWidth(trpStr) / 2, 44, trpStr);
        canvas.setFont(Font::Tiny);
    }
}

void QuantizerSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<QuantizerTrackEngine>();
    auto &sequence = _project.selectedQuantizerSequence();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    for (int i = 0; i < 16; ++i) {
        bool red = false, green = false;

        switch (_activeTab) {
        case 0: {
            int stepIndex = stepOffset() + i;
            red   = (stepIndex == currentStep) || _stepSelection[stepIndex];
            green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection[stepIndex]);
            break;
        }
        case 1: // SRCE: 1 param
            if (i < 1) { bool h = (_heldSteps >> i) & 1; red = h; green = !h; }
            break;
        case 2: // TRIG: 2 params
        case 3: // TUNE: 2 params
            if (i < 2) { bool h = (_heldSteps >> i) & 1; red = h; green = !h; }
            break;
        }

        leds.set(MatrixMap::fromStep(i), red, green);
    }

    LedPainter::drawSelectedSequenceSection(leds, sequence.section());

    // Quick-edit key indicators
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
    const auto &key = event.key();
    if (key.isStep()) {
        if (_activeTab == 0) {
            _stepSelection.keyDown(event, stepOffset());
        } else {
            int stepIndex = key.step();
            int max = maxForTab();
            if (stepIndex < max) {
                _heldSteps |= (1 << stepIndex);
                _cursor = clamp(stepIndex, 0, max - 1);
            }
        }
    }
}

void QuantizerSequenceEditPage::keyUp(KeyEvent &event) {
    const auto &key = event.key();
    if (key.isStep()) {
        if (_activeTab == 0) {
            _stepSelection.keyUp(event, stepOffset());
        } else {
            int stepIndex = key.step();
            if (stepIndex < maxForTab()) {
                _heldSteps &= ~(1 << stepIndex);
            }
        }
    }
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

    if (key.isStep() && _activeTab == 0) {
        _stepSelection.keyPress(event, stepOffset());

        if (!key.shiftModifier()) {
            int stepIndex = stepOffset() + key.step();
            _inMemorySequence = _project.selectedQuantizerSequence();
            sequence.step(stepIndex).toggleGate();
            event.consume();
        }

        KeyPressEvent keyPressEvent = _keyPressEventTracker.process(key);
        if (!key.shiftModifier() && keyPressEvent.count() == 2) {
            event.consume();
        }
    }

    if (key.isEncoder() && _activeTab == 0) {
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

    if (key.isFunction()) {
        switchTab(key.function());
        event.consume();
    }
}

void QuantizerSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedQuantizerSequence();

    if (_activeTab == 0) {
        if (!_stepSelection.any()) {
            return;
        }
        for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
            if (_stepSelection[stepIndex]) {
                sequence.step(stepIndex).setGate(event.value() > 0);
            }
        }
        event.consume();
        return;
    }

    if (_heldSteps == 0) return;

    bool shift = globalKeyState()[Key::Shift];
    auto &track = _project.selectedTrack().quantizerTrack();

    for (int i = 0; i < maxForTab(); ++i) {
        if (!(_heldSteps & (1 << i))) continue;
        switch (_activeTab) {
        case 1:
            if (i == 0) track.editInputSource(event.value(), shift);
            break;
        case 2:
            if (i == 0) track.editTriggerMode(event.value(), shift);
            if (i == 1) track.editTriggerTrack(event.value(), shift);
            break;
        case 3:
            if (i == 0) track.editOctave(event.value(), shift);
            if (i == 1) track.editTranspose(event.value(), shift);
            break;
        }
    }
    event.consume();
}

void QuantizerSequenceEditPage::switchTab(int fKey) {
    _activeTab = clamp(fKey, 0, 3);
    _heldSteps = 0;
    _cursor    = 0;
}

int QuantizerSequenceEditPage::activeFunctionKey() {
    return _activeTab;
}

int QuantizerSequenceEditPage::maxForTab() const {
    switch (_activeTab) {
    case 0: return 16;
    case 1: return 1;
    case 2: return 2;
    case 3: return 2;
    default: return 0;
    }
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
