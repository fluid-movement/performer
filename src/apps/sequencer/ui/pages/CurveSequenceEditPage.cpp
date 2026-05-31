#include "CurveSequenceEditPage.h"

#include "Pages.h"

#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"

#include "core/utils/StringBuilder.h"

#include <cmath>

enum class ContextAction {
    Init,
    Copy,
    Paste,
    Duplicate,
    Generate,
    Last
};

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT" },
    { "COPY" },
    { "PASTE" },
    { "DUPL" },
    { "GEN" },
};

enum class Function {
    Shape  = 0,
    Skew   = 1,
    Length = 2,
    Level  = 3,
    Offset = 4,
};

static const char *functionNames[] = { "SHPE", "SKEW", "LEN", "LVL", "OFST", nullptr };

static const CurveSequenceListModel::Item quickEditItems[8] = {
    CurveSequenceListModel::Item::Divisor,
    CurveSequenceListModel::Item::SegmentCount,
    CurveSequenceListModel::Item::Last,
    CurveSequenceListModel::Item::Last,
    CurveSequenceListModel::Item::Last,
    CurveSequenceListModel::Item::Last,
    CurveSequenceListModel::Item::Last,
    CurveSequenceListModel::Item::Last
};


CurveSequenceEditPage::CurveSequenceEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
    _stepSelection.setStepCompare([this] (int a, int b) {
        auto layer = _project.selectedCurveSequenceLayer();
        const auto &sequence = _project.selectedCurveSequence();
        return sequence.step(a).layerValue(layer) == sequence.step(b).layerValue(layer);
    });
}

void CurveSequenceEditPage::enter() {
}

void CurveSequenceEditPage::exit() {
}

void CurveSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    auto &track = _project.selectedTrack().curveTrack();
    const auto pattern_follow = track.patternFollow();
    const char* pf_repr = Types::patternFollowShortRepresentation(pattern_follow);

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", pf_repr);
    WindowPainter::drawActiveFunction(canvas, CurveSequence::layerName(layer()));
    static const char *actionLabelsActive[]   = { "DEL", "DUPL", "RSET", nullptr, nullptr, nullptr };
    static const char *actionLabelsInactive[] = { "ADD", nullptr, nullptr, nullptr, nullptr, nullptr };
    int selMode = selectionMode();
    const char **footerLabels = functionNames;
    int activeF = activeFunctionKey();
    if (selMode == 1) { footerLabels = actionLabelsActive;   activeF = -1; }
    if (selMode == 2) { footerLabels = actionLabelsInactive; activeF = -1; }

    WindowPainter::drawFooter(canvas, footerLabels, pageKeyState(), activeF);

    auto &trackEngine = _engine.selectedTrackEngine().as<CurveTrackEngine>();
    auto &sequence = _project.selectedCurveSequence();
    int currentSegment = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    float currentFraction = trackEngine.currentStepFraction();

    int segCount = sequence.segmentCount();

    // Layout constants (128 × 64 canvas)
    const int curveY = 18;
    const int curveH = 24;
    const int indY   = 44;  // indicator strip separator y
    const int indContent = 45;

    // Compute total loop length and per-segment pixel widths
    int totalLength = 0;
    for (int i = 0; i < segCount; ++i) {
        totalLength += sequence.step(i).length();
    }
    if (totalLength <= 0) totalLength = 1;

    int segX[CONFIG_STEP_COUNT];
    int segW[CONFIG_STEP_COUNT];
    int cumLen = 0;
    int xAccum = 0;
    for (int i = 0; i < segCount; ++i) {
        segX[i] = xAccum;
        cumLen += sequence.step(i).length();
        int nextX = int(std::round(float(cumLen) / float(totalLength) * Width));
        segW[i] = std::max(1, nextX - xAccum);
        xAccum = nextX;
    }

    // 1. Segment boundary vertical lines (only when a step is held)
    if (_stepSelection.any()) {
        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(Color::Low);
        for (int i = 1; i < segCount; ++i) {
            canvas.vline(segX[i], curveY, curveH);
        }
    }

    // 2. Segment number labels
    for (int i = 0; i < segCount; ++i) {
        bool isCursor = _stepSelection[i];
        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(isCursor ? Color::Bright : Color::Low);
        FixedStringBuilder<4> str("%d", i + 1);
        int textW = canvas.textWidth(str);
        canvas.drawText(segX[i] + (segW[i] - textW + 1) / 2, 14, str);
    }

    // 3. Per-segment curves
    for (int i = 0; i < segCount; ++i) {
        const auto &step = sequence.step(i);
        bool isCursor = _stepSelection[i];

        canvas.setBlendMode(BlendMode::Add);
        canvas.setColor(isCursor ? Color::Bright : Color::Low);

        float shp = step.shapeNorm();
        float skw = step.skewNorm();
        float lvl = step.levelNorm();
        float off = step.offsetNorm();

        int w = segW[i];
        int x0 = segX[i];

        int prevPY = -1;
        for (int px = 0; px < w; ++px) {
            float phase = w > 1 ? float(px) / float(w - 1) : 0.5f;
            float amp = CurveSequence::evalSegment(phase, shp, skw);
            float val = off + lvl * amp;
            if (val < 0.f) val = 0.f;
            if (val > 1.f) val = 1.f;
            int py = curveY + curveH - 1 - int(std::round(val * (curveH - 1)));
            if (prevPY >= 0 && prevPY != py) {
                canvas.line(x0 + px - 1, prevPY, x0 + px, py);
            } else {
                canvas.point(x0 + px, py);
            }
            prevPY = py;
        }
    }

    // 4. Play scanline
    if (currentSegment >= 0 && currentSegment < segCount) {
        int playX = segX[currentSegment] + std::min(int(std::round(currentFraction * segW[currentSegment])), segW[currentSegment] - 1);
        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(Color::Bright);
        canvas.vline(playX, curveY, curveH);
    }

    // 5. Indicator strip separator
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Low);
    canvas.hline(0, indY, Width);

    // 6. Indicator strip per tab
    auto activeLayer = layer();
    for (int i = 0; i < segCount; ++i) {
        const auto &step = sequence.step(i);
        canvas.setBlendMode(BlendMode::Set);
        canvas.setColor(Color::Bright);

        switch (activeLayer) {
        case Layer::Shape: {
            int val = int(std::round(step.shapeNorm() * 100.f));
            if (segW[i] >= 8) {
                FixedStringBuilder<8> str("%d", val);
                int tw = canvas.textWidth(str);
                canvas.drawText(segX[i] + (segW[i] - tw) / 2, indContent, str);
            }
            break;
        }
        case Layer::Skew: {
            int tickX = segX[i] + std::min(int(std::round(step.skewNorm() * segW[i])), segW[i] - 1);
            canvas.vline(tickX, indContent, 4);
            break;
        }
        case Layer::Length: {
            FixedStringBuilder<8> str("%d", step.length());
            int tw = canvas.textWidth(str);
            if (segW[i] >= tw + 2) {
                canvas.drawText(segX[i] + (segW[i] - tw) / 2, indContent, str);
            }
            break;
        }
        case Layer::Level: {
            int val = int(std::round(step.levelNorm() * 100.f));
            if (segW[i] >= 8) {
                FixedStringBuilder<8> str("%d", val);
                int tw = canvas.textWidth(str);
                canvas.drawText(segX[i] + (segW[i] - tw) / 2, indContent, str);
            }
            break;
        }
        case Layer::Offset: {
            int val = int(std::round(step.offsetNorm() * 100.f));
            if (segW[i] >= 8) {
                FixedStringBuilder<8> str("%d", val);
                int tw = canvas.textWidth(str);
                canvas.drawText(segX[i] + (segW[i] - tw) / 2, indContent, str);
            }
            break;
        }
        case Layer::Last:
            break;
        }
    }
}

void CurveSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<CurveTrackEngine>();
    const auto &sequence = _project.selectedCurveSequence();
    int currentSegment = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    int segCount = sequence.segmentCount();

    for (int i = 0; i < 16; ++i) {
        if (i >= segCount) {
            leds.set(MatrixMap::fromStep(i), false, false);
        } else {
            bool isCursor   = _stepSelection[i];
            bool isPlaying  = (i == currentSegment);
            // red = playing, amber = cursor (red+green), green = normal
            bool red   = isPlaying || isCursor;
            bool green = isCursor || (!isPlaying && i < segCount);
            leds.set(MatrixMap::fromStep(i), red, green);
        }
    }

    // show quick edit keys
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        for (int i = 0; i < 8; ++i) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            leds.set(index, false, quickEditItems[i] != CurveSequenceListModel::Item::Last);
            leds.mask(index);
        }
    }
}

void CurveSequenceEditPage::keyDown(KeyEvent &event) {
    _stepSelection.keyDown(event, 0);
}

void CurveSequenceEditPage::keyUp(KeyEvent &event) {
    _stepSelection.keyUp(event, 0);
}

void CurveSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &track = _project.selectedTrack().curveTrack();

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
        if (key.is(Key::Step15)) {
            track.togglePatternFollowDisplay();
        } else {
            quickEdit(key.quickEdit());
        }
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        return;
    }

    _stepSelection.keyPress(event, 0);

    if (key.isFunction()) {
        if (_stepSelection.any()) {
            int mode = selectionMode();
            if (mode == 1) {
                handleActiveSegmentAction(key.function());
                event.consume();
                return;
            } else if (mode == 2) {
                if (key.function() == 0)
                    _project.selectedCurveSequence().addSegment();
                event.consume();
                return;
            }
            // mode == 3 (mixed active+inactive): fall through to switchLayer
        }
        switchLayer(key.function());
        event.consume();
    }

    if (key.isEncoder()) {
        track.setPatternFollowDisplay(false);
        event.consume();
    }
}

void CurveSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedCurveSequence();

    if (!_stepSelection.any()) {
        return;
    }

    bool shift = globalKeyState()[Key::Shift];

    for (size_t i = 0; i < size_t(sequence.segmentCount()); ++i) {
        if (_stepSelection[i]) {
            auto &step = sequence.step(i);
            switch (layer()) {
            case Layer::Shape: {
                float delta = event.value() * (shift ? 0.01f : 0.05f);
                step.setShapeNorm(clamp(step.shapeNorm() + delta, 0.f, 1.f));
                break;
            }
            case Layer::Skew: {
                float delta = event.value() * (shift ? 0.01f : 0.05f);
                step.setSkewNorm(clamp(step.skewNorm() + delta, 0.f, 1.f));
                break;
            }
            case Layer::Length:
                step.setLength(step.length() + event.value());
                break;
            case Layer::Level: {
                float delta = event.value() * (shift ? 0.01f : 0.05f);
                step.setLevelNorm(clamp(step.levelNorm() + delta, 0.f, 1.f));
                break;
            }
            case Layer::Offset: {
                float delta = event.value() * (shift ? 0.01f : 0.05f);
                step.setOffsetNorm(clamp(step.offsetNorm() + delta, 0.f, 1.f));
                break;
            }
            case Layer::Last:
                break;
            }
        }
    }

    event.consume();
}

void CurveSequenceEditPage::switchLayer(int functionKey) {
    switch (Function(functionKey)) {
    case Function::Shape:
        setLayer(Layer::Shape);
        break;
    case Function::Skew:
        setLayer(Layer::Skew);
        break;
    case Function::Length:
        setLayer(Layer::Length);
        break;
    case Function::Level:
        setLayer(Layer::Level);
        break;
    case Function::Offset:
        setLayer(Layer::Offset);
        break;
    }
}

int CurveSequenceEditPage::activeFunctionKey() {
    switch (layer()) {
    case Layer::Shape:  return 0;
    case Layer::Skew:   return 1;
    case Layer::Length: return 2;
    case Layer::Level:  return 3;
    case Layer::Offset: return 4;
    case Layer::Last:   break;
    }
    return -1;
}

void CurveSequenceEditPage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); },
        doubleClick
    ));
}

void CurveSequenceEditPage::contextAction(int index) {
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
    case ContextAction::Duplicate:
        duplicateSequence();
        break;
    case ContextAction::Generate:
        generateSequence();
        break;
    case ContextAction::Last:
        break;
    }
}

bool CurveSequenceEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteCurveSequenceSteps();
    default:
        return true;
    }
}

void CurveSequenceEditPage::initSequence() {
    _project.selectedCurveSequence().clearSteps();
    showMessage("STEPS INITIALIZED");
}

void CurveSequenceEditPage::copySequence() {
    _model.clipBoard().copyCurveSequenceSteps(_project.selectedCurveSequence(), _stepSelection.selected());
    showMessage("STEPS COPIED");
}

void CurveSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteCurveSequenceSteps(_project.selectedCurveSequence(), _stepSelection.selected());
    showMessage("STEPS PASTED");
}

void CurveSequenceEditPage::duplicateSequence() {
    _project.selectedCurveSequence().duplicateSteps();
    showMessage("STEPS DUPLICATED");
}

void CurveSequenceEditPage::generateSequence() {
    _manager.pages().generatorSelect.show([this] (bool success, Generator::Mode mode) {
        if (success) {
            auto builder = _builderContainer.create<CurveSequenceBuilder>(_project.selectedCurveSequence(), layer());
            if (_stepSelection.none()) {
                _stepSelection.selectAll();
            }
            auto generator = Generator::execute(mode, *builder, _stepSelection.selected());
            if (generator) {
                _manager.pages().generator.show(generator, &_stepSelection);
            }
        }
    });
}

int CurveSequenceEditPage::selectionMode() const {
    if (!_stepSelection.any()) return 0;
    int segCount = _project.selectedCurveSequence().segmentCount();
    bool hasActive = false, hasInactive = false;
    for (int i = 0; i < 16; ++i) {
        if (_stepSelection[i]) {
            if (i < segCount) hasActive = true;
            else hasInactive = true;
        }
    }
    if (hasActive && hasInactive) return 3;
    if (hasActive) return 1;
    return 2;
}

void CurveSequenceEditPage::handleActiveSegmentAction(int f) {
    auto &seq = _project.selectedCurveSequence();
    // collect active selected indices descending to avoid index shifts
    std::array<int, 16> indices;
    int count = 0;
    for (int i = 15; i >= 0; --i) {
        if (_stepSelection[i] && i < seq.segmentCount())
            indices[count++] = i;
    }
    for (int k = 0; k < count; ++k) {
        int i = indices[k];
        switch (f) {
        case 0: seq.deleteSegment(i);    break;
        case 1: seq.duplicateSegment(i); break;
        case 2: seq.resetSegment(i);     break;
        }
    }
}

void CurveSequenceEditPage::quickEdit(int index) {
    _listModel.setSequence(&_project.selectedCurveSequence());
    if (quickEditItems[index] != CurveSequenceListModel::Item::Last) {
        _manager.pages().quickEdit.show(_listModel, int(quickEditItems[index]));
    }
}
