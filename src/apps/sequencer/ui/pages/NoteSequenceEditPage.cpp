#include "NoteSequenceEditPage.h"

#include "LayoutPage.h"
#include "Pages.h"

#include "model/NoteSequence.h"
#include "ui/LedPainter.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"

#include "model/Scale.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"
#include <bitset>
#include <iostream>

enum class ContextAction {
    Init,
    Copy,
    Paste,
    Duplicate,    Generate,
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
    Gate        = 0,
    Retrigger   = 1,
    Repeat      = 2,
    Note        = 3,
    Condition   = 4,
};

static const char *functionNames[] = { "GATE", "RETRIG", "REPEAT", "NOTE", "COND" };


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

NoteSequenceEditPage::NoteSequenceEditPage(PageManager &manager, PageContext &context) :
    SequenceEditPageBase(manager, context)
{
    _stepSelection.setStepCompare([this] (int a, int b) {
        auto layer = _project.selectedNoteSequenceLayer();
        const auto &sequence = _project.selectedNoteSequence();
        return sequence.step(a).layerValue(layer) == sequence.step(b).layerValue(layer);
    });
}

void NoteSequenceEditPage::enter() {
    updateMonitorStep();

    _inMemorySequence = _project.selectedNoteSequence();
}

void NoteSequenceEditPage::exit() {
    _engine.selectedTrackEngine().as<NoteTrackEngine>().setMonitorStep(-1);
}

void NoteSequenceEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    auto &track = _project.selectedTrack().noteTrack();

    /* Prepare flags shown before mode name (top right header) */
    const auto pattern_follow = track.patternFollow();
    const char* pf_repr = Types::patternFollowShortRepresentation(pattern_follow);

    WindowPainter::drawHeader(canvas, _model, _engine, "STEPS", pf_repr);

    WindowPainter::drawActiveFunction(canvas, NoteSequence::layerName(layer()));
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    auto &sequence = _project.selectedNoteSequence();
    const auto &scale = _project.selectedScale();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    if (trackEngine.currentRecordStep()!=-1) {
        trackEngine.setCurrentRecordStep(sequence.currentRecordStep());
    }
    int currentRecordStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentRecordStep() : -1;

    const int stepWidth = Width / StepCount;
    const int stepOffset = this->stepOffset();

    const int loopY = 16;

    // Track Pattern Section on the UI
    if (track.isPatternFollowDisplayOn() && _engine.state().running()) {
        bool section_change = bool((currentStep) % StepCount == 0); // StepCount is relative to screen
        int section_no = int((currentStep) / StepCount);
        if (section_change && section_no != sequence.section()) {
            sequence.setSecion(section_no);
        }
    }

    // draw loop points
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);
    SequencePainter::drawLoopStart(canvas, (sequence.firstStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);
    SequencePainter::drawLoopEnd(canvas, (sequence.lastStep() - stepOffset) * stepWidth + 1, loopY, stepWidth - 2);

    for (int i = 0; i < StepCount; ++i) {
        int stepIndex = stepOffset + i;
        const auto &step = sequence.step(stepIndex);

        int x = i * stepWidth;
        int y = 20;

        // loop
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

        // step gate
        canvas.setColor(stepIndex == currentStep ? Color::Bright : Color::Medium);
        canvas.drawRect(x + 2, y + 2, stepWidth - 4, stepWidth - 4);
        if (step.gate()) {
            canvas.setColor(_context.model.settings().userSettings().get<DimSequenceSetting>(SettingDimSequence)->getValue() ? Color::Low : Color::Bright);
            canvas.fillRect(x + 4, y + 4, stepWidth - 8, stepWidth - 8);
        }

        // record step
        if (stepIndex == currentRecordStep) {
            // draw circle
            canvas.setColor(step.gate() ? Color::None : Color::Bright);
            canvas.fillRect(x + 6, y + 6, stepWidth - 12, stepWidth - 12);
            canvas.setColor(Color::Medium);
            canvas.hline(x + 7, y + 5, 2);
            canvas.hline(x + 7, y + 10, 2);
            canvas.vline(x + 5, y + 7, 2);
            canvas.vline(x + 10, y + 7, 2);
        }

        switch (layer()) {
        case Layer::Gate:
            break;
        case Layer::StepLength: {
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;
            NoteSequence::printStepLength(str, step.stepLength());
            canvas.setFont(Font::Tiny);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 24, str);
            canvas.setFont(Font::Small);
            break;
        }
        case Layer::GateLength:
            SequencePainter::drawLength(
                canvas,
                x + 2, y + 18, stepWidth - 4, 6,
                step.gateLength(), 100
            );
            break;
        case Layer::Retrigger:
            SequencePainter::drawRetrigger(
                canvas,
                x, y + 18, stepWidth, 2,
                step.retrigger(), NoteSequence::Retrigger::Range
            );
            break;
        case Layer::ArpRetrigger: {
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str("%+d", step.arpRetrigger());
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            break;
        }
        case Layer::Repeat: {
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str("x%d", step.repeat());
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            break;
        }
        case Layer::Note: {
            int rootNote = _project.rootNote();
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;

            if (scale.isChromatic() && scale.notesPerOctave() < 12) {
                int note = step.note();
                int npo = scale.notesPerOctave();
                int octave = note / npo - (note % npo != 0 && note < 0 ? 1 : 0);
                int degree = note - octave * npo + 1;
                str("%d", degree);
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
                str.reset();
                str("%+d", octave);
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            } else {
                scale.noteName(str, step.note(), rootNote, Scale::Short1);
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
                str.reset();
                scale.noteName(str, step.note(), rootNote, Scale::Short2);
                canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            }
            break;
        }
        case Layer::Slide:
            SequencePainter::drawSlide(
                canvas,
                x + 4, y + 18, stepWidth - 8, 4,
                step.slide()
            );
            break;
        case Layer::Condition:
        case Layer::RetriggerCondition: {
            auto condition = layer() == Layer::Condition ? step.condition() : step.retriggerCondition();
            canvas.setColor(Color::Bright);
            FixedStringBuilder<8> str;
            Types::printCondition(str, condition, Types::ConditionFormat::Short1);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 20, str);
            str.reset();
            Types::printCondition(str, condition, Types::ConditionFormat::Short2);
            canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, y + 27, str);
            break;
        }
        case Layer::Last:
            break;
        }
    }
}

    // handle detail display

void NoteSequenceEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
    auto &sequence = _project.selectedNoteSequence();
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;

    for (int i = 0; i < 16; ++i) {
        int stepIndex = stepOffset() + i;
        bool red = (stepIndex == currentStep) || _stepSelection[stepIndex];
        bool green = (stepIndex != currentStep) && (sequence.step(stepIndex).gate() || _stepSelection[stepIndex]);
        leds.set(MatrixMap::fromStep(i), red, green);
    }

    LedPainter::drawSelectedSequenceSection(leds, sequence.section());

    // show quick edit keys
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        for (int i = 0; i < 8; ++i) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            leds.set(index, false, quickEditItems[i] != NoteSequenceListModel::Item::Last);
            leds.mask(index);
        }
        int index = MatrixMap::fromStep(15);
        leds.unmask(index);
        leds.set(index, false, true);
        leds.mask(index);
    }
}

void NoteSequenceEditPage::keyDown(KeyEvent &event) {
    _stepSelection.keyDown(event, stepOffset());
    updateMonitorStep();
}

void NoteSequenceEditPage::keyUp(KeyEvent &event) {
    _stepSelection.keyUp(event, stepOffset());
    updateMonitorStep();
}

void NoteSequenceEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedNoteSequence();
    auto &track = _project.selectedTrack().noteTrack();

    auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    if (handleCommonKeyPress(event)) return;

    if (key.isQuickEdit()) {
         if (key.is(Key::Step15)) {
            bool lpConnected = _engine.isLaunchpadConnected();

             track.togglePatternFollowDisplay(lpConnected);
        } else {
            _inMemorySequence = _project.selectedNoteSequence();
            quickEdit(key.quickEdit());
        }
        event.consume();
        return;
    }

    if (key.pageModifier() && key.is(Key::Step6)) {
        // undo function
        _project.setSelectedNoteSequence(_inMemorySequence);
        event.consume();
        return;
    }

    if (key.pageModifier()) {
        return;
    }


    if (key.isFunction()) {
        int v = 0;
        switch (key.code()) {
            case Key::F0:
                v=1;
                break;
            case Key::F1:
                v=2;
                break;
            case Key::F2:
                v=3;
                break;
            case Key::F3:
                v=4;
                break;
            case Key::F4:
                v=5;
                break;
        }
        for (int i=0; i<16; ++i) {
           if (key.state(i)) {
                const auto &scale = _project.selectedScale();
                int stepIndex = 0;
                if (i>=8) {
                    stepIndex = i -8;
                } else {
                    stepIndex = i+8;
                }
                sequence.step(stepIndex).setNote(scale.notesPerOctave()*v);
                event.consume();
                return;
                
           }
        }
        
    }
    _stepSelection.keyPress(event, stepOffset());
    updateMonitorStep();

    if (!key.shiftModifier() && key.isStep()) {
        int stepIndex = stepOffset() + key.step();
        switch (layer()) {
        case Layer::Gate:
            _inMemorySequence = _project.selectedNoteSequence();
            sequence.step(stepIndex).toggleGate();
            event.consume();
            break;
        default:
            break;
        }
    }

    KeyPressEvent keyPressEvent =_keyPressEventTracker.process(key);

    if (!key.shiftModifier() && key.isStep() && keyPressEvent.count() == 2) {
        int stepIndex = stepOffset() + key.step();
        if (layer() != Layer::Gate) {
            _inMemorySequence = _project.selectedNoteSequence();
            sequence.step(stepIndex).toggleGate();
            event.consume();
        }
    }

    if (key.isFunction()) {
        if(key.shiftModifier() && key.function() == 2 && _stepSelection.any()) {
            _inMemorySequence = _project.selectedNoteSequence();
            tieNotes();
            event.consume();
            return;
        }
        switchLayer(key.function(), key.shiftModifier());
        event.consume();
    }

    if (key.isEncoder()) {
        track.setPatternFollowDisplay(false);
        _inMemorySequence = _project.selectedNoteSequence();
        if (_stepSelection.any() && allSelectedStepsActive()) {
            setSelectedStepsGate(false);
        } else {
            setSelectedStepsGate(true);
        }
        event.consume();
    }


    if (key.isLeft()) {
        if (key.shiftModifier()) {
            if (trackEngine.currentRecordStep()!=-1) {
                if (Routing::isRouted(Routing::Target::CurrentRecordStep, _model.project().selectedTrackIndex())) {
                    sequence.setCurrentRecordStep(sequence.currentRecordStep()-1, true);
                } else {
                    sequence.setCurrentRecordStep(sequence.currentRecordStep()-1, false);
                }
            } else {
                _inMemorySequence = _project.selectedNoteSequence();
                sequence.shiftSteps(_stepSelection.selected(), -1);
                _stepSelection.shiftLeft(sequence.firstStep(), sequence.lastStep()+1);
            }
        } else {
             track.setPatternFollowDisplay(false);
             sequence.setSecion(std::max(0, sequence.section() - 1));
        }
        event.consume();
    }
    if (key.isRight()) {
        if (key.shiftModifier()) {
            if (trackEngine.currentRecordStep()!=-1) {
                if (Routing::isRouted(Routing::Target::CurrentRecordStep, _model.project().selectedTrackIndex())) {
                    sequence.setCurrentRecordStep(sequence.currentRecordStep()+1, true);
                } else {
                    sequence.setCurrentRecordStep(sequence.currentRecordStep()+1, false);
                }
                
            } else {
                _inMemorySequence = _project.selectedNoteSequence();
                sequence.shiftSteps(_stepSelection.selected(), 1);
                _stepSelection.shiftRight(sequence.firstStep(), sequence.lastStep()+1);
            }
        } else {
            track.setPatternFollowDisplay(false);
            sequence.setSecion(std::min(3, sequence.section() + 1));
        }
        event.consume();
    }
}

void NoteSequenceEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedNoteSequence();
    const auto &scale = _project.selectedScale();

    if (!_stepSelection.any())
    {
        // with nothing selected the encoder walks within the current tab's family
        switch (layer())
        {
        case Layer::Gate:
            setLayer(event.value() > 0 ? Layer::StepLength : Layer::GateLength);
            break;
        case Layer::StepLength:
            setLayer(event.value() > 0 ? Layer::GateLength : Layer::Gate);
            break;
        case Layer::GateLength:
            setLayer(event.value() > 0 ? Layer::Gate : Layer::StepLength);
            break;
        case Layer::Retrigger:
            setLayer(Layer::ArpRetrigger);
            break;
        case Layer::ArpRetrigger:
            setLayer(Layer::Retrigger);
            break;
        case Layer::Repeat:
            break;
        case Layer::Note:
            setLayer(Layer::Slide);
            break;
        case Layer::Slide:
            setLayer(Layer::Note);
            break;
        case Layer::Condition:
            setLayer(Layer::RetriggerCondition);
            break;
        case Layer::RetriggerCondition:
            setLayer(Layer::Condition);
            break;
        default:
            break;
        }
        return;
    }

    for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            auto &step = sequence.step(stepIndex);
            bool shift = globalKeyState()[Key::Shift];
            switch (layer()) {
            case Layer::Gate:
                step.setGate(event.value() > 0);
                break;
            case Layer::StepLength:
                step.setStepLength(step.stepLength() + event.value());
                break;
            case Layer::GateLength:
                // 1 per detent, 10 with shift
                step.setGateLength(step.gateLength() + event.value() * (shift ? 10 : 1));
                break;
            case Layer::Retrigger:
                step.setRetrigger(step.retrigger() + event.value());
                break;
            case Layer::ArpRetrigger:
                step.setArpRetrigger(step.arpRetrigger() + event.value());
                break;
            case Layer::Repeat:
                step.setRepeat(step.repeat() + event.value());
                break;
            case Layer::Note:
                step.setNote(step.note() + event.value() * ((shift && scale.isChromatic()) ? scale.notesPerOctave() : 1));
                updateMonitorStep();
                break;
            case Layer::Slide:
                step.setSlide(event.value() > 0);
                break;
            case Layer::Condition:
                step.setCondition(ModelUtils::adjustedEnum(step.condition(), event.value()));
                break;
            case Layer::RetriggerCondition:
                step.setRetriggerCondition(ModelUtils::adjustedEnum(step.retriggerCondition(), event.value()));
                break;
            case Layer::Last:
                break;
            }
        }
    }

    event.consume();
}

void NoteSequenceEditPage::midi(MidiEvent &event) {
    if (!_engine.recording() && layer() == Layer::Note && _stepSelection.any()) {
        if (_project.clockSetup().filterNote()) {
            return;
        }
        auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
        auto &sequence = _project.selectedNoteSequence();
        const auto &scale = _project.selectedScale();
        const auto &message = event.message();

        if (message.isNoteOn()) {
            float volts = (message.note() - 60) * (1.f / 12.f);
            int note = scale.noteFromVolts(volts);

            for (size_t stepIndex = 0; stepIndex < sequence.steps().size(); ++stepIndex) {
                if (_stepSelection[stepIndex]) {
                    auto &step = sequence.step(stepIndex);
                    step.setNote(note);
                    step.setGate(true);
                }
            }

            trackEngine.setMonitorStep(_stepSelection.first());
            updateMonitorStep();
        }
    }
}

void NoteSequenceEditPage::switchLayer(int functionKey, bool shift) {
    // shift jumps straight to the tab's secondary layer
    if (shift) {
        switch (Function(functionKey)) {
        case Function::Gate:
            setLayer(Layer::GateLength);
            break;
        case Function::Retrigger:
            setLayer(Layer::ArpRetrigger);
            break;
        case Function::Repeat:
            setLayer(Layer::Repeat);
            break;
        case Function::Note:
            setLayer(Layer::Slide);
            break;
        case Function::Condition:
            setLayer(Layer::RetriggerCondition);
            break;
        }
        return;
    }

    // repeated presses cycle within the tab
    switch (Function(functionKey)) {
    case Function::Gate:
        switch (layer()) {
        case Layer::Gate:       setLayer(Layer::StepLength); break;
        case Layer::StepLength: setLayer(Layer::GateLength); break;
        default:                setLayer(Layer::Gate); break;
        }
        break;
    case Function::Retrigger:
        switch (layer()) {
        case Layer::Retrigger:  setLayer(Layer::ArpRetrigger); break;
        default:                setLayer(Layer::Retrigger); break;
        }
        break;
    case Function::Repeat:
        setLayer(Layer::Repeat);
        break;
    case Function::Note:
        switch (layer()) {
        case Layer::Note:       setLayer(Layer::Slide); break;
        default:                setLayer(Layer::Note); break;
        }
        break;
    case Function::Condition:
        switch (layer()) {
        case Layer::Condition:  setLayer(Layer::RetriggerCondition); break;
        default:                setLayer(Layer::Condition); break;
        }
        break;
    }
}

int NoteSequenceEditPage::activeFunctionKey() {
    switch (layer()) {
    case Layer::Gate:
    case Layer::StepLength:
    case Layer::GateLength:
        return 0;
    case Layer::Retrigger:
    case Layer::ArpRetrigger:
        return 1;
    case Layer::Repeat:
        return 2;
    case Layer::Note:
    case Layer::Slide:
        return 3;
    case Layer::Condition:
    case Layer::RetriggerCondition:
        return 4;
    case Layer::Last:
        break;
    }

    return -1;
}

void NoteSequenceEditPage::updateMonitorStep() {
    auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();

    // TODO should we monitor an all layers not just note?
    if (layer() == Layer::Note && !_stepSelection.isPersisted() && _stepSelection.any()) {
        trackEngine.setMonitorStep(_stepSelection.first());
    } else {
        trackEngine.setMonitorStep(-1);
    }
}

const ContextMenuModel::Item *NoteSequenceEditPage::contextItems() const {
    return contextMenuItems;
}

int NoteSequenceEditPage::contextActionCount() const {
    return int(ContextAction::Last);
}

void NoteSequenceEditPage::contextAction(int index) {
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

bool NoteSequenceEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteNoteSequenceSteps();
    default:
        return true;
    }
}

void NoteSequenceEditPage::initSequence() {
    _project.selectedNoteSequence().clearStepsSelected(_stepSelection.selected());
    showMessage("STEPS INITIALIZED");
}

void NoteSequenceEditPage::copySequence() {
    _model.clipBoard().copyNoteSequenceSteps(_project.selectedNoteSequence(), _stepSelection.selected());
    showMessage("STEPS COPIED");
}

void NoteSequenceEditPage::pasteSequence() {
    _model.clipBoard().pasteNoteSequenceSteps(_project.selectedNoteSequence(), _stepSelection.selected());
    showMessage("STEPS PASTED");
}

void NoteSequenceEditPage::duplicateSequence() {
    _project.selectedNoteSequence().duplicateSteps();
    showMessage("STEPS DUPLICATED");
}


void NoteSequenceEditPage::tieNotes() {

    auto &sequence = _project.selectedNoteSequence();

    if (_stepSelection.any()) {
        int first=-1;
        int last=-1;

        for (size_t i = 0; i < sequence.steps().size(); ++i) {
            if (_stepSelection[i]) {
                if (first == -1 ) {
                    first = i;
                }
                last = i;
            }
        }

        for (int i = first; i <= last; i++) {
            sequence.step(i).setGate(true);
            if (i != last) {
                sequence.step(i).setGateLength(100);
                showMessage("NOTES TIED");
            }
            sequence.step(i).setNote(sequence.step(first).note());
        }
    }
}

void NoteSequenceEditPage::generateSequence() {
    _manager.pages().generatorSelect.show([this] (bool success, Generator::Mode mode) {
        if (success) {
            auto builder = _builderContainer.create<NoteSequenceBuilder>(_project.selectedNoteSequence(), layer());

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

void NoteSequenceEditPage::quickEdit(int index) {
    _listModel.setSequence(&_project.selectedNoteSequence());
    if (quickEditItems[index] != NoteSequenceListModel::Item::Last) {
        _manager.pages().quickEdit.show(_listModel, int(quickEditItems[index]));
    }
}

bool NoteSequenceEditPage::allSelectedStepsActive() const {
    const auto &sequence = _project.selectedNoteSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex] && !sequence.step(stepIndex).gate()) {
            return false;
        }
    }
    return true;
}

void NoteSequenceEditPage::setSelectedStepsGate(bool gate) {
    auto &sequence = _project.selectedNoteSequence();
    for (size_t stepIndex = 0; stepIndex < _stepSelection.size(); ++stepIndex) {
        if (_stepSelection[stepIndex]) {
            sequence.step(stepIndex).setGate(gate);
        }
    }
}