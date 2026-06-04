#include "StochasticSequenceEditPage.h"

#include "Pages.h"

#include "model/StochasticSequence.h"
#include "engine/StochasticEngine.h"
#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"

enum class ContextAction {
    Init,
    Copy,
    Paste,
    Last
};

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT" },
    { "COPY" },
    { "PASTE" },
};

static const char *functionNames[] = { "NOTE", "OCT", "LEN", "LOOP", nullptr };

static const StochasticSequenceListModel::Item quickEditItems[8] = {
    StochasticSequenceListModel::Item::SequenceFirstStep,
    StochasticSequenceListModel::Item::SequenceLastStep,
    StochasticSequenceListModel::Item::RunMode,
    StochasticSequenceListModel::Item::Divisor,
    StochasticSequenceListModel::Item::ResetMeasure,
    StochasticSequenceListModel::Item::Last,
    StochasticSequenceListModel::Item::Last,
    StochasticSequenceListModel::Item::Last
};

StochasticSequenceEditPage::StochasticSequenceEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

void StochasticSequenceEditPage::enter() {
    auto &sequence = _project.selectedStochasticSequence();
    sequence.setMessage(StochasticSequence::Message::None);
    _activeTab = 0;
    _cursor = 0;
    _heldSteps = 0;
}

void StochasticSequenceEditPage::exit() {
    _engine.selectedTrackEngine().as<StochasticEngine>().setMonitorStep(-1);
}

void StochasticSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    auto &sequence = _project.selectedStochasticSequence();
    displayMessage(sequence);

    const char *modeFlags = nullptr;
    if (sequence.useLoop()) modeFlags = "L";

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", modeFlags);
    WindowPainter::drawActiveFunction(canvas, functionNames[_activeTab]);
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    switch (_activeTab) {
    case 0: drawNoteTab(canvas, sequence); break;
    case 1: drawOctTab(canvas, sequence);  break;
    case 2: drawLenTab(canvas, sequence);  break;
    case 3: drawLoopTab(canvas, sequence); break;
    }
}

// Draw a probability bar. isHeld = button held (white), isPlaying = currently sounding (underline).
static void drawProbBar(Canvas &canvas, int x, int barFloor, int barW, int colW,
                        int barHMax, int prob, bool isHeld, bool isPlaying) {
    const int barH = prob * barHMax / 15;
    const int barX = x + (colW - barW) / 2;  // center bar within its column slot

    canvas.setBlendMode(BlendMode::Set);

    // Clear the full column width so the header divider cannot leak through gap pixels.
    canvas.setColor(Color::None);
    canvas.fillRect(x, barFloor - barHMax, colW, barHMax);

    if (barH > 0) {
        canvas.setColor(isHeld ? Color::Bright : Color::Medium);
        canvas.fillRect(barX, barFloor - barH, barW, barH);
    }

    if (isPlaying) {
        canvas.setColor(Color::Bright);
        canvas.hline(barX, barFloor, barW);
    }
}

void StochasticSequenceEditPage::drawNoteTab(Canvas &canvas, const StochasticSequence &sequence) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
    int currentDegree = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    const int barFloor = 48;
    const int barHMax  = 39;
    const int colW     = 36;
    const int xOff     = 2;
    const int labelY   = 53;

    canvas.setFont(Font::Tiny);

    for (int i = 0; i < 7; i++) {
        int x = xOff + i * colW;
        drawProbBar(canvas, x, barFloor, colW / 2, colW, barHMax,
                    sequence.degreeProb(i), (_heldSteps >> i) & 1, i == currentDegree);
        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(Color::Bright);
        FixedStringBuilder<4> str("%d", i + 1);
        canvas.drawText(x + (colW - canvas.textWidth(str)) / 2, labelY, str);
    }
}

void StochasticSequenceEditPage::drawOctTab(Canvas &canvas, const StochasticSequence &sequence) {
    const int barFloor = 48;
    const int barHMax  = 39;
    const int colW     = 36;
    const int xOff     = (256 - 5 * colW) / 2;  // 38
    const int labelY   = 53;

    static const char *labels[] = { "-2", "-1", " 0", "+1", "+2" };
    canvas.setFont(Font::Tiny);

    for (int i = 0; i < 5; i++) {
        int x = xOff + i * colW;
        drawProbBar(canvas, x, barFloor, colW / 2, colW, barHMax,
                    sequence.octaveProb(i), (_heldSteps >> i) & 1, false);

        // Tick mark for centre octave (i == 2) — match bar width and centering
        if (i == 2) {
            canvas.setBlendMode(BlendMode::Set);
            canvas.setColor(Color::Medium);
            canvas.hline(x + colW / 4, barFloor - barHMax, colW / 2);
        }

        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(Color::Bright);
        canvas.drawText(x + (colW - canvas.textWidth(labels[i])) / 2, labelY, labels[i]);
    }
}

void StochasticSequenceEditPage::drawLenTab(Canvas &canvas, const StochasticSequence &sequence) {
    const int barFloor = 48;
    const int barHMax  = 39;
    const int colW     = 36;
    const int xOff     = (256 - 6 * colW) / 2;  // 20
    const int labelY   = 53;

    static const char *labels[] = { "1/16", "1/8", "1/4", "1/2", "1", "2" };
    canvas.setFont(Font::Tiny);

    for (int i = 0; i < 6; i++) {
        int x = xOff + i * colW;
        drawProbBar(canvas, x, barFloor, colW / 2, colW, barHMax,
                    sequence.durationProb(i), (_heldSteps >> i) & 1, false);
        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(Color::Bright);
        canvas.drawText(x + (colW - canvas.textWidth(labels[i])) / 2, labelY, labels[i]);
    }
}

void StochasticSequenceEditPage::drawLoopTab(Canvas &canvas, const StochasticSequence &sequence) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
    bool isActive = trackEngine.isActiveSequence(sequence);
    auto lockedSteps = isActive ? trackEngine.lockedSteps() : std::vector<StochasticLoopStep>{};
    int currentIdx = isActive ? trackEngine.currentIndex() : -1;

    const int cY = 10;
    const int cH = 45;

    // Chance box (left)
    const int boxX = 1;
    const int boxW = 50;
    canvas.setColor(Color::Bright);
    canvas.drawRect(boxX, cY, boxW, cH);

    canvas.setFont(Font::Tiny);
    canvas.setColor(Color::Low);
    const char *chcLbl = "CHC";
    canvas.drawText(boxX + (boxW - canvas.textWidth(chcLbl)) / 2, cY + 7, chcLbl);

    const int barX = boxX + 12;
    const int barW = 26;
    const int barY = cY + 11;
    const int barH = 22;
    canvas.setColor(Color::Low);
    canvas.drawRect(barX, barY, barW, barH);
    int fillH = (sequence.loopChance() * (barH - 2)) / 15;
    if (fillH > 0) {
        canvas.setColor(Color::Bright);
        canvas.fillRect(barX + 1, barY + barH - 1 - fillH, barW - 2, fillH);
    }

    FixedStringBuilder<8> pct;
    pct("%d%%", (sequence.loopChance() * 100) / 15);
    canvas.setColor(Color::Bright);
    canvas.drawText(boxX + (boxW - canvas.textWidth(pct)) / 2, cY + 38, pct);

    // Step grid (right): 8 cols × 2 rows
    const int gridX = 56;
    const int colW  = 25;
    const int rowH  = 20;
    const int rowGap = 4;
    const int row0Y = cY + 1;
    const int row1Y = row0Y + rowH + rowGap;
    int firstStep = sequence.sequenceFirstStep();
    int loopLen = sequence.loopLength();

    for (int i = 0; i < loopLen; i++) {
        int col = i % 8;
        int row = i / 8;
        int x = gridX + col * colW;
        int y = (row == 0) ? row0Y : row1Y;

        bool gate = false;
        int bufIdx = (firstStep + i) % int(lockedSteps.size());
        if (!lockedSteps.empty()) {
            gate = lockedSteps.at(bufIdx).gate();
        }
        bool isCurrent = (i == currentIdx);

        canvas.setBlendMode(BlendMode::Set);
        if (isCurrent) {
            if (gate) {
                canvas.setColor(Color::Bright);
                canvas.fillRect(x + 1, y + 1, colW - 2, rowH - 2);
            } else {
                canvas.setColor(Color::Bright);
                canvas.drawRect(x + 1, y + 1, colW - 2, rowH - 2);
            }
        } else if (gate) {
            canvas.setColor(Color::Medium);
            canvas.fillRect(x + 1, y + 1, colW - 2, rowH - 2);
        }
    }
}

void StochasticSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<StochasticEngine>();
    const auto &sequence = _project.selectedStochasticSequence();
    int currentDegree = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    int tabCount = 0;
    switch (_activeTab) {
    case 0: tabCount = 7; break;
    case 1: tabCount = 5; break;
    case 2: tabCount = 6; break;
    case 3: tabCount = 12; break;
    }

    for (int i = 0; i < 16; i++) {
        bool red = false;
        bool green = false;

        if (_activeTab == 0 && i < 7) {
            bool held = (_heldSteps >> i) & 1;
            red   = (i == currentDegree) || held;
            green = !red && (sequence.degreeProb(i) > 0);
        } else if (_activeTab == 1 && i < 5) {
            bool held = (_heldSteps >> i) & 1;
            red   = held;
            green = !held && (sequence.octaveProb(i) > 0);
        } else if (_activeTab == 2 && i < 6) {
            bool held = (_heldSteps >> i) & 1;
            red   = held;
            green = !held && (sequence.durationProb(i) > 0);
        } else if (_activeTab == 3 && i < 16) {
            // Loop tab: show active loop length
            int loopLen = sequence.loopLength();
            red   = (i == loopLen - 1);
            green = (i < loopLen - 1);
        } else {
            (void)tabCount;
        }

        leds.set(MatrixMap::fromStep(i), red, green);
    }

    // quick edit keys
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        for (int i = 0; i < 8; i++) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            leds.set(index, false, quickEditItems[i] != StochasticSequenceListModel::Item::Last);
            leds.mask(index);
        }
    }
}

void StochasticSequenceEditPage::keyDown(KeyEvent &event) {
    const auto &key = event.key();
    if (key.isStep()) {
        int stepIndex = key.step();
        int max = maxForTab();
        if (max > 0 && stepIndex < max) {
            _heldSteps |= (1 << stepIndex);
            _cursor = clamp(stepIndex, 0, max - 1);
        }
    }
}

void StochasticSequenceEditPage::keyUp(KeyEvent &event) {
    const auto &key = event.key();
    if (key.isStep()) {
        int stepIndex = key.step();
        if (stepIndex < maxForTab()) {
            _heldSteps &= ~(1 << stepIndex);
        }
    }
}

void StochasticSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedStochasticSequence();

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
        _listModel.setSequence(&sequence);
        if (quickEditItems[key.quickEdit()] != StochasticSequenceListModel::Item::Last) {
            _manager.pages().quickEdit.show(_listModel, int(quickEditItems[key.quickEdit()]));
        }
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        if (key.is(Key::Step5)) {
            showMessage("Loop cleared");
            sequence.setClearLoop(true);
            event.consume();
        }
        if (key.is(Key::Step6)) {
            if (sequence.useLoop()) showMessage("Loop off");
            else showMessage("Loop on");
            sequence.setUseLoop();
            event.consume();
        }
        return;
    }

    if (key.isStep()) {
        int stepIndex = key.step();
        if (_activeTab == 3) {
            // LOOP tab: step buttons set loop length (button n → length n+1, wraps around buffer)
            sequence.setLoopLength(stepIndex + 1);
            event.consume();
        }
        // Other tabs: handled in keyDown/keyUp for hold-and-edit
    }

    if (key.isFunction()) {
        switchTab(key.function());
        event.consume();
    }
}

void StochasticSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedStochasticSequence();
    bool shift = globalKeyState()[Key::Shift];

    if (_activeTab == 3) {
        // LOOP tab: encoder adjusts loopChance
        sequence.editLoopChance(event.value(), shift);
        event.consume();
        return;
    }

    if (_heldSteps != 0) {
        int max = maxForTab();
        for (int i = 0; i < max; i++) {
            if (_heldSteps & (1 << i)) {
                switch (_activeTab) {
                case 0: sequence.editDegreeProb(i, event.value(), shift); break;
                case 1: sequence.editOctaveProb(i, event.value(), shift); break;
                case 2: sequence.editDurationProb(i, event.value(), shift); break;
                }
            }
        }
        event.consume();
    }
}

void StochasticSequenceEditPage::midi(MidiEvent &/*event*/) {
    // No MIDI step recording in V2
}

void StochasticSequenceEditPage::switchTab(int functionKey) {
    _activeTab = clamp(functionKey, 0, 3);
    int tabMax = (_activeTab == 0) ? 6 : (_activeTab == 1) ? 4 : (_activeTab == 2) ? 5 : 15;
    _cursor = clamp(_cursor, 0, tabMax);
    _heldSteps = 0;
}

int StochasticSequenceEditPage::activeFunctionKey() {
    return _activeTab;
}

void StochasticSequenceEditPage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); }, doubleClick
    ));
}

void StochasticSequenceEditPage::contextAction(int index) {
    switch (ContextAction(index)) {
    case ContextAction::Init:
        initSequence();
        break;
    case ContextAction::Copy:
        copySequence();
        break;
    case ContextAction::Paste:
        pasteSequence();
        break;
    case ContextAction::Last:
        break;
    }
}

bool StochasticSequenceEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteStochasticSequenceSteps();
    default:
        return true;
    }
}

void StochasticSequenceEditPage::initSequence() {
    _project.selectedStochasticSequence().clear();
    showMessage("SEQUENCE INITIALIZED");
}

void StochasticSequenceEditPage::copySequence() {
    _model.clipBoard().copyStochasticSequenceSteps(_project.selectedStochasticSequence(), {});
    showMessage("SEQUENCE COPIED");
}

void StochasticSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteStochasticSequenceSteps(_project.selectedStochasticSequence(), {});
    showMessage("SEQUENCE PASTED");
}

void StochasticSequenceEditPage::displayMessage(StochasticSequence &sequence) {
    if (sequence.message() != StochasticSequence::Message::None) {
        FixedStringBuilder<16> str;
        switch (sequence.message()) {
        case StochasticSequence::Message::LoopOn:    str("Loop On");      break;
        case StochasticSequence::Message::LoopOff:   str("Loop Off");     break;
        case StochasticSequence::Message::Cleared:   str("Loop cleared"); break;
        case StochasticSequence::Message::ReSeed:    str("Reseed");       break;
        default: break;
        }
        showMessage(str);
        sequence.setMessage(StochasticSequence::Message::None);
    }
}
