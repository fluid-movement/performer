#include "ArpSequenceEditPage.h"

#include "Pages.h"

#include "model/ArpSequence.h"
#include "engine/ArpTrackEngine.h"
#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"
#include "ui/MatrixMap.h"

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

static const char *functionNames[] = { "NOTE", "RHYTHM", "MOD", "ARP", nullptr };

static const ArpSequenceListModel::Item quickEditItems[8] = {
    ArpSequenceListModel::Item::Last,
    ArpSequenceListModel::Item::Last,
    ArpSequenceListModel::Item::Last,
    ArpSequenceListModel::Item::Divisor,
    ArpSequenceListModel::Item::ResetMeasure,
    ArpSequenceListModel::Item::Last,
    ArpSequenceListModel::Item::Last,
    ArpSequenceListModel::Item::Last
};

ArpSequenceEditPage::ArpSequenceEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

void ArpSequenceEditPage::enter() {
    _activeTab = 0;
    _cursor = 0;
    _heldSteps = 0;
}

void ArpSequenceEditPage::exit() {
}

int ArpSequenceEditPage::maxForTab() const {
    switch (_activeTab) {
    case 0: return 7;   // NOTE: 7 degrees
    case 1: return 4;   // RHYTHM: N/K/R/GATELEN
    case 2: return 4;   // MOD: N/K/R/MODE
    case 3: return 3;   // ARP: ORDER/OCTAVES/LENGTH
    default: return 0;
    }
}

void ArpSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    auto &sequence = _project.selectedArpSequence();

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", nullptr);
    WindowPainter::drawActiveFunction(canvas, functionNames[_activeTab]);
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    switch (_activeTab) {
    case 0: drawNoteTab(canvas, sequence);   break;
    case 1: drawRhythmTab(canvas, sequence); break;
    case 2: drawModTab(canvas, sequence);    break;
    case 3: drawArpTab(canvas, sequence);    break;
    }
}

// Draw 7 degree boxes centered on screen
void ArpSequenceEditPage::drawNoteTab(Canvas &canvas, const ArpSequence &sequence) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
    int currentDegree = trackEngine.isActiveSequence(sequence) ? trackEngine.currentDegree() : -1;

    const int boxW   = 22;
    const int boxH   = 22;
    const int colW   = 36;
    const int xOff   = 2;
    const int boxY   = 14;
    const int labelY = boxY + boxH + 9;

    canvas.setFont(Font::Tiny);

    for (int i = 0; i < 7; i++) {
        int x  = xOff + i * colW;
        int bx = x + (colW - boxW) / 2;

        bool active  = sequence.isDegreeActive(i);
        bool held    = (_heldSteps >> i) & 1;
        bool playing = (i == currentDegree);

        canvas.setBlendMode(BlendMode::Set);

        if (active) {
            canvas.setColor(held ? Color::Bright : Color::Medium);
            canvas.fillRect(bx, boxY, boxW, boxH);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(bx, boxY, boxW, boxH);
        }

        // Play indicator: small tick below box
        if (playing) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(x + colW / 2 - 2, boxY + boxH + 3, 4, 2);
        }

        // Degree label
        canvas.setColor(Color::MediumLow);
        FixedStringBuilder<4> str("%d", i + 1);
        canvas.drawText(x + (colW - canvas.textWidth(str)) / 2, labelY, str);
    }
}

// Draw both euclidean patterns, one above the other
void ArpSequenceEditPage::drawCombinedEuclidean(Canvas &canvas, const ArpSequence &sequence, int playStep) {
    const int cw   = 16;   // cell width, matches sandbox
    const int rh   = 13;   // row height
    const int r1   = 8;    // rhythm row Y
    const int r2   = 23;   // mod row Y

    // Generate euclidean pattern with auto-rotation so first hit is at index 0
    auto buildPat = [](bool *out, int n, int k, int r) {
        n = std::max(n, 1);
        k = std::min(k, n);
        bool raw[16] = {};
        int bucket = 0;
        for (int i = 0; i < n; i++) {
            bucket += k;
            if (bucket >= n) { bucket -= n; raw[i] = true; }
        }
        int firstHit = 0;
        if (k > 0) {
            for (int i = 0; i < n; i++) { if (raw[i]) { firstHit = i; break; } }
        }
        for (int i = 0; i < n; i++)
            out[i] = raw[((i + firstHit - r) % n + n) % n];
    };

    int rn = std::max(sequence.rhythmN(), 1);
    int mn = std::max(sequence.modN(), 1);

    bool rhythmPat[16] = {};
    bool modPat[16]    = {};
    buildPat(rhythmPat, rn, sequence.rhythmK(), sequence.rhythmR());
    buildPat(modPat,    mn, sequence.modK(),    sequence.modR());

    int modMode = sequence.modMode();

    canvas.setBlendMode(BlendMode::Set);

    // Dim separator between rows
    canvas.setColor(Color::Low);
    canvas.hline(0, r2 - 1, 256);

    for (int i = 0; i < 16; i++) {
        int x       = i * cw;
        bool hit    = rhythmPat[i % rn];
        bool mod    = modPat[i % mn];
        bool overlap = hit && mod;

        // Row 2: mod pattern — plain fill
        if (mod) {
            canvas.setColor(Color::Low);
            canvas.fillRect(x + 2, r2, 12, rh);
        }

        if (!hit) continue;

        bool border  = false;
        bool fires   = true;
        bool ratchet = false;

        switch (modMode) {
        case 0: break;                                           // OFF
        case 1: border = overlap; break;                         // ACCENT
        case 2: border = overlap; fires = !overlap; break;       // MASK
        case 3: border = !overlap; fires = overlap; break;       // COMBINE
        case 4: border = overlap; ratchet = overlap; break;      // RATCHET
        case 5: border = overlap; break;                         // HOLD
        default: break;
        }

        canvas.setColor(Color::MediumBright);
        if (border)
            canvas.drawRect(x + 2, r1, 12, rh);
        if (fires) {
            if (ratchet) {
                canvas.fillRect(x + 4, r1 + 2, 3, rh - 4);
                canvas.fillRect(x + 9, r1 + 2, 3, rh - 4);
            } else if (border) {
                canvas.fillRect(x + 4, r1 + 2, 8, rh - 4);
            } else {
                canvas.fillRect(x + 2, r1, 12, rh);
            }
        }
    }

    // Play cursor: bright tick below row 2
    if (playStep >= 0) {
        int px = playStep * cw;
        canvas.setColor(Color::Bright);
        canvas.fillRect(px + 5, r2 + rh + 1, cw - 10, 2);
    }
}

// Draw parameter bar: equal-width columns with label and value
void ArpSequenceEditPage::drawParamBar(Canvas &canvas, const char *labels[], const char *values[], int count, int selIdx) {
    const int barY  = 38;
    const int barH  = 16;
    const int cellW = 256 / count;

    canvas.setFont(Font::Tiny);
    canvas.setBlendMode(BlendMode::Set);

    // Separator line above bar
    canvas.setColor(Color::Low);
    canvas.hline(0, barY - 1, 256);

    for (int i = 0; i < count; i++) {
        int x   = i * cellW;
        bool sel = (i == selIdx) || ((_heldSteps >> i) & 1);

        if (sel) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(x, barY, cellW, barH);
        }

        // Left divider (skip first cell)
        if (i > 0) {
            canvas.setColor(sel ? Color::Bright : Color::Low);
            canvas.vline(x, barY, barH);
        }

        // Label — use Sub blend when selected so text knocks out of the bright fill
        canvas.setBlendMode(sel ? BlendMode::Sub : BlendMode::Set);
        canvas.setColor(Color::Low);
        canvas.drawText(x + (cellW - canvas.textWidth(labels[i])) / 2, barY + 5, labels[i]);
        // Value
        canvas.setColor(Color::Bright);
        canvas.drawText(x + (cellW - canvas.textWidth(values[i])) / 2, barY + 13, values[i]);
        canvas.setBlendMode(BlendMode::Set);
    }
}

void ArpSequenceEditPage::drawRhythmTab(Canvas &canvas, const ArpSequence &sequence) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
    int playStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    drawCombinedEuclidean(canvas, sequence, playStep);

    static const char *labels[] = { "STEPS", "GATES", "ROTATE", "GATELEN" };
    FixedStringBuilder<8> v0, v1, v2, v3;
    sequence.printRhythmN(v0);
    sequence.printRhythmK(v1);
    sequence.printRhythmR(v2);
    sequence.printRhythmGateLen(v3);
    const char *values[] = { v0, v1, v2, v3 };

    int selIdx = -1;
    for (int i = 0; i < 4; i++) {
        if ((_heldSteps >> i) & 1) { selIdx = i; break; }
    }

    drawParamBar(canvas, labels, values, 4, selIdx);
}

void ArpSequenceEditPage::drawModTab(Canvas &canvas, const ArpSequence &sequence) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
    int playStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    drawCombinedEuclidean(canvas, sequence, playStep);

    static const char *labels[] = { "STEPS", "PULSES", "ROTATE", "MODE" };
    FixedStringBuilder<8> v0, v1, v2, v3;
    sequence.printModN(v0);
    sequence.printModK(v1);
    sequence.printModR(v2);
    sequence.printModMode(v3);
    const char *values[] = { v0, v1, v2, v3 };

    int selIdx = -1;
    for (int i = 0; i < 4; i++) {
        if ((_heldSteps >> i) & 1) { selIdx = i; break; }
    }

    drawParamBar(canvas, labels, values, 4, selIdx);
}

void ArpSequenceEditPage::drawArpTab(Canvas &canvas, const ArpSequence &sequence) {
    const int colW = 256 / 3;  // 85px per column

    bool sel0 = (_heldSteps >> 0) & 1;
    bool sel1 = (_heldSteps >> 1) & 1;
    bool sel2 = (_heldSteps >> 2) & 1;

    canvas.setBlendMode(BlendMode::Set);

    // Column dividers
    canvas.setColor(Color::Low);
    canvas.vline(colW,     8, 46);
    canvas.vline(colW * 2, 8, 46);

    // ── Column 0: PLAY ORDER ──────────────────────────────────────────────
    {
        int cx = colW / 2;
        canvas.setFont(Font::Tiny);
        canvas.setColor(Color::Low);
        canvas.drawText(cx - canvas.textWidth("PLAY ORDER") / 2, 16, "PLAY ORDER");
        FixedStringBuilder<8> orderStr;
        sequence.printArpOrder(orderStr);
        canvas.setFont(Font::Small);
        canvas.setColor(sel0 ? Color::Bright : Color::Medium);
        canvas.drawText(cx - canvas.textWidth(orderStr) / 2, 36, orderStr);
        canvas.setFont(Font::Tiny);
    }

    // ── Column 1: OCTAVES ─────────────────────────────────────────────────
    {
        int cx    = colW + colW / 2;
        int octs  = sequence.arpOctaves();
        canvas.setFont(Font::Tiny);
        canvas.setColor(Color::Low);
        canvas.drawText(cx - canvas.textWidth("OCTAVES") / 2, 16, "OCTAVES");

        // 4-segment bar: each segment 10×10 with 2px gap
        const int segW = 10, segH = 10, gap = 2;
        int totalW = 4 * segW + 3 * gap;
        int sx = cx - totalW / 2;
        for (int o = 0; o < 4; o++) {
            canvas.setColor(o < octs
                ? (sel1 ? Color::Bright : Color::MediumBright)
                : Color::Low);
            canvas.fillRect(sx + o * (segW + gap), 22, segW, segH);
        }

        FixedStringBuilder<8> octStr("%d oct", octs);
        canvas.setColor(sel1 ? Color::Bright : Color::Medium);
        canvas.drawText(cx - canvas.textWidth(octStr) / 2, 40, octStr);
    }

    // ── Column 2: LENGTH ──────────────────────────────────────────────────
    {
        int cx = colW * 2 + colW / 2;
        canvas.setFont(Font::Tiny);
        canvas.setColor(Color::Low);
        canvas.drawText(cx - canvas.textWidth("LENGTH") / 2, 16, "LENGTH");
        FixedStringBuilder<8> lenStr;
        sequence.printArpLength(lenStr);
        canvas.setFont(Font::Small);
        canvas.setColor(sel2 ? Color::Bright : Color::Medium);
        canvas.drawText(cx - canvas.textWidth(lenStr) / 2, 36, lenStr);
        canvas.setFont(Font::Tiny);
    }
}

void ArpSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<ArpTrackEngine>();
    const auto &sequence = _project.selectedArpSequence();
    int currentDegree = trackEngine.isActiveSequence(sequence) ? trackEngine.currentDegree() : -1;

    for (int i = 0; i < 16; i++) {
        bool red   = false;
        bool green = false;

        switch (_activeTab) {
        case 0: // NOTE: degrees
            if (i < 7) {
                bool held = (_heldSteps >> i) & 1;
                red   = (i == currentDegree) || held;
                green = !red && sequence.isDegreeActive(i);
            }
            break;
        case 1: // RHYTHM: 4 params
        case 2: // MOD: 4 params
            if (i < 4) {
                bool held = (_heldSteps >> i) & 1;
                red   = held;
                green = !held;
            }
            break;
        case 3: // ARP: 3 params
            if (i < 3) {
                bool held = (_heldSteps >> i) & 1;
                red   = held;
                green = !held;
            }
            break;
        }

        leds.set(MatrixMap::fromStep(i), red, green);
    }

    // F-key LEDs
    for (int i = 0; i < 4; i++) {
        // function keys are handled by drawFooter; nothing extra needed
    }

    // quick edit keys
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        for (int i = 0; i < 8; i++) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            leds.set(index, false, quickEditItems[i] != ArpSequenceListModel::Item::Last);
            leds.mask(index);
        }
    }
}

void ArpSequenceEditPage::keyDown(KeyEvent &event) {
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

void ArpSequenceEditPage::keyUp(KeyEvent &event) {
    const auto &key = event.key();
    if (key.isStep()) {
        int stepIndex = key.step();
        int max = maxForTab();
        if (stepIndex < max) {
            _heldSteps &= ~(1 << stepIndex);
        }
    }
}

void ArpSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedArpSequence();

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
        if (quickEditItems[key.quickEdit()] != ArpSequenceListModel::Item::Last) {
            _manager.pages().quickEdit.show(_listModel, int(quickEditItems[key.quickEdit()]));
        }
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        return;
    }

    if (key.isStep()) {
        int stepIndex = key.step();
        if (_activeTab == 0 && stepIndex < 7) {
            // NOTE tab: toggle degree
            sequence.toggleDegree(stepIndex);
            event.consume();
        }
        // For other tabs, hold-and-turn is handled via keyDown/encoder
    }

    if (key.isFunction()) {
        switchTab(key.function());
        event.consume();
    }
}

void ArpSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedArpSequence();

    if (_activeTab == 0) {
        // NOTE tab: no encoder action
        return;
    }

    if (_heldSteps == 0) return;

    int max = maxForTab();
    bool shift = globalKeyState()[Key::Shift];

    for (int i = 0; i < max; i++) {
        if (!(_heldSteps & (1 << i))) continue;
        switch (_activeTab) {
        case 1: // RHYTHM
            if (i == 0) sequence.editRhythmN(event.value(), shift);
            if (i == 1) sequence.editRhythmK(event.value(), shift);
            if (i == 2) sequence.editRhythmR(event.value(), shift);
            if (i == 3) sequence.editRhythmGateLen(event.value(), shift);
            break;
        case 2: // MOD
            if (i == 0) sequence.editModN(event.value(), shift);
            if (i == 1) sequence.editModK(event.value(), shift);
            if (i == 2) sequence.editModR(event.value(), shift);
            if (i == 3) sequence.editModMode(event.value(), shift);
            break;
        case 3: // ARP
            if (i == 0) sequence.editArpOrder(event.value(), shift);
            if (i == 1) sequence.editArpOctaves(event.value(), shift);
            if (i == 2) sequence.editArpLength(event.value(), shift);
            break;
        }
    }
    event.consume();
}

void ArpSequenceEditPage::midi(MidiEvent &/*event*/) {
    // No MIDI step recording in Arp V2
}

void ArpSequenceEditPage::switchTab(int fKey) {
    _activeTab = clamp(fKey, 0, 3);
    _heldSteps = 0;
    _cursor = 0;
}

int ArpSequenceEditPage::activeFunctionKey() {
    return _activeTab;
}

void ArpSequenceEditPage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); }, doubleClick
    ));
}

void ArpSequenceEditPage::contextAction(int index) {
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

bool ArpSequenceEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteArpSequenceSteps();
    default:
        return true;
    }
}

void ArpSequenceEditPage::initSequence() {
    _project.selectedArpSequence().clear();
    showMessage("SEQUENCE INITIALIZED");
}

void ArpSequenceEditPage::copySequence() {
    _model.clipBoard().copyArpSequenceSteps(_project.selectedArpSequence(), {});
    showMessage("SEQUENCE COPIED");
}

void ArpSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteArpSequenceSteps(_project.selectedArpSequence(), {});
    showMessage("SEQUENCE PASTED");
}
