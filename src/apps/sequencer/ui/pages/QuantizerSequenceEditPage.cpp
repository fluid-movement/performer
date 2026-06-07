#include "QuantizerSequenceEditPage.h"

#include "Pages.h"

#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"

#include "engine/QuantizerTrackEngine.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"

static const char *functionNames[] = { "GATE", "SRCE", "TRIG", "TUNE", "LOOP" };

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
    SequenceEditPageBase(manager, context)
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
    case 4: drawLoopTab(canvas, track);     break;
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
    const int src = int(track.inputSource());

    // CV IN section
    canvas.setBlendMode(BlendMode::Set);
    canvas.setFont(Font::Tiny);
    canvas.setColor(Color::Low);
    canvas.drawText(4, 13, "CV IN");

    static const char *cvLabels[] = { "CV1", "CV2", "CV3", "CV4" };
    for (int i = 0; i < 4; ++i) {
        const int bx  = i * 64 + 4;
        const int bw  = 56;
        const bool sel = (src == i);
        canvas.setBlendMode(BlendMode::Set);
        if (sel) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(bx, 17, bw, 17);
            canvas.setBlendMode(BlendMode::Sub);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(bx, 17, bw, 17);
            canvas.setColor(Color::Medium);
        }
        canvas.setFont(Font::Small);
        canvas.drawText(bx + (bw - canvas.textWidth(cvLabels[i])) / 2, 28, cvLabels[i]);
    }

    // TRACK section
    canvas.setBlendMode(BlendMode::Set);
    canvas.setFont(Font::Tiny);
    canvas.setColor(Color::Low);
    canvas.drawText(4, 38, "TRACK");

    // 8 chips centered: totalW = 8*28 + 7*2 = 238, startX = (256-238)/2 = 9
    const int trkW = 28, trkGap = 2, trkX0 = 9;
    for (int t = 0; t < 8; ++t) {
        const int bx  = trkX0 + t * (trkW + trkGap);
        const bool sel = (src == t + 4);
        canvas.setBlendMode(BlendMode::Set);
        if (sel) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(bx, 41, trkW, 11);
            canvas.setBlendMode(BlendMode::Sub);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(bx, 41, trkW, 11);
            canvas.setColor(Color::Medium);
        }
        FixedStringBuilder<4> lbl("T%d", t + 1);
        canvas.drawText(bx + (trkW - canvas.textWidth(lbl)) / 2, 49, lbl);
    }
    canvas.setBlendMode(BlendMode::Set);
}

void QuantizerSequenceEditPage::drawTriggerTab(Canvas &canvas, const QuantizerTrack &track) {
    const int srcIdx = track.triggerSourceIndex();
    // 0=Free, 1–8=External(track), 9–12=CvGate(cv)
    const int modeChip = (srcIdx == 0) ? 0 : (srcIdx <= 8) ? 1 : 2;

    // Row 1: 3 mode chips — FREE / INT / EXT
    static const char *modeNames[] = { "FREE", "INT", "EXT" };
    const int colW = 256 / 3;
    for (int i = 0; i < 3; ++i) {
        const int bx  = i * colW + 4;
        const int bw  = colW - 8;
        const int cx  = i * colW + colW / 2;
        canvas.setBlendMode(BlendMode::Set);
        if (i == modeChip) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(bx, 17, bw, 14);
            canvas.setBlendMode(BlendMode::Sub);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(bx, 17, bw, 14);
            canvas.setColor(Color::Medium);
        }
        canvas.setFont(Font::Tiny);
        canvas.drawText(cx - canvas.textWidth(modeNames[i]) / 2, 27, modeNames[i]);
    }

    // Separator
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Low);
    canvas.hline(0, 34, 256);

    // Row 2a: INT selected — 8 track chips T1–T8
    if (modeChip == 1) {
        const int selTrack = srcIdx - 1;  // 0–7
        const int trkW = 28, trkGap = 2, trkX0 = 9;
        for (int t = 0; t < 8; ++t) {
            const int bx  = trkX0 + t * (trkW + trkGap);
            const bool sel = (t == selTrack);
            canvas.setBlendMode(BlendMode::Set);
            if (sel) {
                canvas.setColor(Color::Bright);
                canvas.fillRect(bx, 38, trkW, 14);
                canvas.setBlendMode(BlendMode::Sub);
            } else {
                canvas.setColor(Color::Low);
                canvas.drawRect(bx, 38, trkW, 14);
                canvas.setColor(Color::Medium);
            }
            canvas.setFont(Font::Tiny);
            FixedStringBuilder<4> lbl("T%d", t + 1);
            canvas.drawText(bx + (trkW - canvas.textWidth(lbl)) / 2, 48, lbl);
        }
        canvas.setBlendMode(BlendMode::Set);
    }

    // Row 2b: EXT selected — 4 CV chips CV1–CV4
    if (modeChip == 2) {
        const int selCv = srcIdx - 9;  // 0–3
        static const char *cvLabels[] = { "CV1", "CV2", "CV3", "CV4" };
        for (int i = 0; i < 4; ++i) {
            const int bx  = i * 64 + 4;
            const int bw  = 56;
            const bool sel = (i == selCv);
            canvas.setBlendMode(BlendMode::Set);
            if (sel) {
                canvas.setColor(Color::Bright);
                canvas.fillRect(bx, 38, bw, 14);
                canvas.setBlendMode(BlendMode::Sub);
            } else {
                canvas.setColor(Color::Low);
                canvas.drawRect(bx, 38, bw, 14);
                canvas.setColor(Color::Medium);
            }
            canvas.setFont(Font::Tiny);
            canvas.drawText(bx + (bw - canvas.textWidth(cvLabels[i])) / 2, 48, cvLabels[i]);
        }
        canvas.setBlendMode(BlendMode::Set);
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

void QuantizerSequenceEditPage::drawLoopTab(Canvas &canvas, const QuantizerTrack &track) {
    auto &trackEngine = _engine.selectedTrackEngine().as<QuantizerTrackEngine>();
    const int fillCount  = trackEngine.loopFillCount();
    const int playSlot   = trackEngine.loopPlaySlot();
    const int loopStart  = track.loopStart();
    const int loopLen    = track.loopLength();
    const auto loopMode  = trackEngine.loopMode();

    // Step cells: 16 buffer slots showing fill + active window + play cursor
    canvas.setBlendMode(BlendMode::Set);
    const int SW = Width / StepCount;   // 16px
    const int cellY = 10, cellH = 12;

    for (int i = 0; i < StepCount; ++i) {
        const bool captured = i < fillCount;
        const bool inRange  = ((i - loopStart + StepCount) % StepCount) < loopLen;
        const bool isPlay = (i == playSlot);

        if (isPlay) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(i * SW + 1, cellY, SW - 2, cellH);
        } else if (inRange && captured) {
            canvas.setColor(Color::Low);
            canvas.fillRect(i * SW + 1, cellY, SW - 2, cellH);
        } else if (captured) {
            canvas.setColor(Color::Low);
            canvas.drawRect(i * SW + 1, cellY, SW - 2, cellH);
        }
    }

    // Separator
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Low);
    canvas.hline(0, 26, Width);

    // Transport + param buttons: [PLAY] [REC/LOOP] [LEN N] [START N]
    const int BTN_W = 64, BTN_Y = 29, BTN_H = 23;

    struct BtnSpec { const char *label; const char *sub; bool active; };
    FixedStringBuilder<8> lenStr("%d", loopLen);
    FixedStringBuilder<8> startStr("%d", loopStart + 1);

    BtnSpec btns[4] = {
        { "PLAY", nullptr, loopMode == QuantizerTrackEngine::LoopMode::Play },
        { "REC",  nullptr, loopMode != QuantizerTrackEngine::LoopMode::Play },
        { "LEN",   lenStr,   bool((_heldSteps >> 0) & 1) },
        { "START", startStr, bool((_heldSteps >> 1) & 1) },
    };

    for (int i = 0; i < 4; ++i) {
        const auto &btn = btns[i];
        const int bx = i * BTN_W + 2;
        const int bw = BTN_W - 4;
        const int cx = i * BTN_W + BTN_W / 2;

        canvas.setBlendMode(BlendMode::Set);
        if (btn.active) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(bx, BTN_Y, bw, BTN_H);
            canvas.setBlendMode(BlendMode::Sub);
            canvas.setColor(Color::Bright);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(bx, BTN_Y, bw, BTN_H);
        }
        canvas.setFont(Font::Tiny);
        if (btn.sub != nullptr) {
            canvas.drawText(cx - canvas.textWidth(btn.label) / 2, BTN_Y + 7, btn.label);
            canvas.setFont(Font::Small);
            canvas.drawText(cx - canvas.textWidth(btn.sub) / 2, BTN_Y + 19, btn.sub);
            canvas.setFont(Font::Tiny);
        } else {
            canvas.drawText(cx - canvas.textWidth(btn.label) / 2, BTN_Y + BTN_H / 2 + 2, btn.label);
        }
        canvas.setBlendMode(BlendMode::Set);
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
        case 1: // SRCE: free-scroll, no param buttons
            break;
        case 2: // TRIG: free-scroll only, no param buttons
            break;
        case 3: // TUNE: 2 param buttons (octave, transpose)
            if (i < 2) { bool h = (_heldSteps >> i) & 1; red = h; green = !h; }
            break;
        case 4: // LOOP: 4 transport buttons (steps 0-3)
            if (i < 4) {
                using LM = QuantizerTrackEngine::LoopMode;
                const auto mode = trackEngine.loopMode();
                if (i == 0) { green = (mode == LM::Play); }
                else if (i == 1) { green = (mode == LM::Loop); red = (mode == LM::Rec); }
                else if (i == 2) { red = bool((_heldSteps >> 0) & 1); green = !red; }
                else if (i == 3) { red = bool((_heldSteps >> 1) & 1); green = !red; }
            }
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
        } else if (_activeTab == 4) {
            // Steps 2-3 → LEN/START param hold (bits 0-1 of _heldSteps)
            int si = key.step();
            if (si == 2 || si == 3) {
                _heldSteps |= (1 << (si - 2));
            }
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
        } else if (_activeTab == 4) {
            int si = key.step();
            if (si == 2 || si == 3) {
                _heldSteps &= ~(1 << (si - 2));
            }
        } else {
            int stepIndex = key.step();
            if (stepIndex < maxForTab()) {
                _heldSteps &= ~(1 << stepIndex);
            }
        }
    }
}

void QuantizerSequenceEditPage::keyPress(KeyPressEvent &event) {
    if (handleCommonKeyPress(event)) return;
    const auto &key = event.key();
    auto &sequence = _project.selectedQuantizerSequence();

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

    if (key.isStep() && _activeTab == 4) {
        auto &trackEngine = _engine.selectedTrackEngine().as<QuantizerTrackEngine>();
        using LM = QuantizerTrackEngine::LoopMode;
        const int si = key.step();
        if (si == 0) {
            trackEngine.setLoopMode(LM::Play);
        } else if (si == 1) {
            trackEngine.setLoopMode(LM::Rec);
        }
        // Steps 2-3 are hold-only params (LEN/START); all step presses consumed in LOOP tab.
        event.consume();
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

    bool shift = globalKeyState()[Key::Shift];
    auto &track = _project.selectedTrack().quantizerTrack();

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

    // SRCE: free-scroll encoder
    if (_activeTab == 1) {
        track.editInputSource(event.value(), shift);
        event.consume();
        return;
    }

    // TRIG: unified free-scroll through all 13 trigger source options
    if (_activeTab == 2) {
        track.editTriggerSource(event.value(), shift);
        event.consume();
        return;
    }

    // LOOP tab: hold step 2 (LEN) or step 3 (START) + encoder
    if (_activeTab == 4) {
        if (_heldSteps == 0) return;
        if ((_heldSteps >> 0) & 1) track.editLoopLength(event.value(), shift);
        if ((_heldSteps >> 1) & 1) {
            track.editLoopStart(event.value(), shift);
            // Reset play position so the loop restarts cleanly from the new window start.
            auto &eng = _engine.selectedTrackEngine().as<QuantizerTrackEngine>();
            if (eng.loopMode() == QuantizerTrackEngine::LoopMode::Loop) {
                eng.setLoopMode(QuantizerTrackEngine::LoopMode::Loop);
            }
        }
        event.consume();
        return;
    }

    // TUNE and beyond: require hold-and-turn
    if (_heldSteps == 0) return;

    for (int i = 0; i < maxForTab(); ++i) {
        if (!(_heldSteps & (1 << i))) continue;
        switch (_activeTab) {
        case 3:
            if (i == 0) track.editOctave(event.value(), shift);
            if (i == 1) track.editTranspose(event.value(), shift);
            break;
        }
    }
    event.consume();
}

void QuantizerSequenceEditPage::switchTab(int fKey) {
    _activeTab = clamp(fKey, 0, 4);
    _heldSteps = 0;
    _cursor    = 0;
}

int QuantizerSequenceEditPage::maxForTab() const {
    switch (_activeTab) {
    case 0: return 16;  // GATE: all 16 steps
    case 1: return 0;   // SRCE: free-scroll encoder, no param buttons
    case 2: return 0;   // TRIG: free-scroll only, no param buttons
    case 3: return 2;   // TUNE: hold Step 0/1 for octave/transpose
    case 4: return 0;   // LOOP: handled specially in keyDown/keyUp
    default: return 0;
    }
}

const ContextMenuModel::Item *QuantizerSequenceEditPage::contextItems() const {
    return qSeqContextMenuItems;
}

int QuantizerSequenceEditPage::contextActionCount() const {
    return int(QSeqContextAction::Last);
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
