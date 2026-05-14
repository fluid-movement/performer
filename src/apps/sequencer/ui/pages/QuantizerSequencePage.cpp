#include "QuantizerSequencePage.h"

#include "ListPage.h"
#include "Pages.h"

#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"

#include "core/utils/StringBuilder.h"

enum class QuantizerSeqContextAction {
    Init,
    Copy,
    Paste,
    Duplicate,
    Route,
    Last
};

enum class QuantizerSeqSaveContextAction {
    Load,
    Save,
    SaveAs,
    Last
};

static const ContextMenuModel::Item quantizerSeqContextMenuItems[] = {
    { "INIT" },
    { "COPY" },
    { "PASTE" },
    { "DUPL" },
    { "ROUTE" },
};

static const ContextMenuModel::Item quantizerSeqSaveContextMenuItems[] = {
    { "LOAD" },
    { "SAVE" },
    { "SAVE AS"},
};


QuantizerSequencePage::QuantizerSequencePage(PageManager &manager, PageContext &context) :
    ListPage(manager, context, _listModel)
{}

void QuantizerSequencePage::enter() {
    _listModel.setSequence(&_project.selectedQuantizerSequence());
}

void QuantizerSequencePage::exit() {
    _listModel.setSequence(nullptr);
}

void QuantizerSequencePage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "SEQUENCE");
    WindowPainter::drawActiveFunction(canvas, Track::trackModeName(_project.selectedTrack().trackMode()));
    WindowPainter::drawFooter(canvas);

    ListPage::draw(canvas);
}

void QuantizerSequencePage::updateLeds(Leds &leds) {
    ListPage::updateLeds(leds);
}

void QuantizerSequencePage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.shiftModifier() && event.count() == 2) {
        saveContextShow();
        event.consume();
        return;
    }

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

    if (key.pageModifier()) {
        return;
    }

    if (key.is(Key::Encoder) && selectedRow() == 0) {
        _manager.pages().textInput.show("NAME:", _project.selectedQuantizerSequence().name(), NoteSequence::NameLength, [this] (bool result, const char *text) {
            if (result) {
                _project.selectedQuantizerSequence().setName(text);
            }
        });
        return;
    }

    if (!event.consumed()) {
        ListPage::keyPress(event);
    }
    if (key.isEncoder()) {
        auto row = ListPage::selectedRow();
        if (row == 6) {
            _listModel.setSelectedScale(_project.scale());
        }
    }
}

void QuantizerSequencePage::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        quantizerSeqContextMenuItems,
        int(QuantizerSeqContextAction::Last),
        [&] (int index) { contextAction(index); },
        [&] (int index) { return contextActionEnabled(index); },
        doubleClick
    ));
}

void QuantizerSequencePage::saveContextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        quantizerSeqSaveContextMenuItems,
        int(QuantizerSeqSaveContextAction::Last),
        [&] (int index) { saveContextAction(index); },
        [&] (int index) { return true; },
        doubleClick
    ));
}

void QuantizerSequencePage::contextAction(int index) {
    switch (QuantizerSeqContextAction(index)) {
    case QuantizerSeqContextAction::Init:
        initSequence();
        break;
    case QuantizerSeqContextAction::Copy:
        copySequence();
        break;
    case QuantizerSeqContextAction::Paste:
        pasteSequence();
        break;
    case QuantizerSeqContextAction::Duplicate:
        duplicateSequence();
        break;
    case QuantizerSeqContextAction::Route:
        initRoute();
        break;
    case QuantizerSeqContextAction::Last:
        break;
    }
}

void QuantizerSequencePage::saveContextAction(int index) {
    switch (QuantizerSeqSaveContextAction(index)) {
    case QuantizerSeqSaveContextAction::Load:
        loadSequence();
        break;
    case QuantizerSeqSaveContextAction::Save:
        saveSequence();
        break;
    case QuantizerSeqSaveContextAction::SaveAs:
        saveAsSequence();
        break;
    case QuantizerSeqSaveContextAction::Last:
        break;
    }
}

bool QuantizerSequencePage::contextActionEnabled(int index) const {
    switch (QuantizerSeqContextAction(index)) {
    case QuantizerSeqContextAction::Paste:
        return _model.clipBoard().canPasteNoteSequence();
    case QuantizerSeqContextAction::Route:
        return _listModel.routingTarget(selectedRow()) != Routing::Target::None;
    default:
        return true;
    }
}

void QuantizerSequencePage::initSequence() {
    _project.selectedQuantizerSequence().clear();
    showMessage("SEQUENCE INITIALIZED");
}

void QuantizerSequencePage::copySequence() {
    _model.clipBoard().copyNoteSequence(_project.selectedQuantizerSequence());
    showMessage("SEQUENCE COPIED");
}

void QuantizerSequencePage::pasteSequence() {
    _model.clipBoard().pasteNoteSequence(_project.selectedQuantizerSequence());
    showMessage("SEQUENCE PASTED");
}

void QuantizerSequencePage::duplicateSequence() {
    if (_project.selectedTrack().duplicatePattern(_project.selectedPatternIndex())) {
        showMessage("SEQUENCE DUPLICATED");
    }
}

void QuantizerSequencePage::initRoute() {
    _manager.pages().top.editRoute(_listModel.routingTarget(selectedRow()), _project.selectedTrackIndex());
}

void QuantizerSequencePage::loadSequence() {
    _manager.pages().fileSelect.show("LOAD SEQUENCE", FileType::NoteSequence, _project.selectedQuantizerSequence().slotAssigned() ? _project.selectedQuantizerSequence().slot() : 0, false, [this] (bool result, int slot) {
        if (result) {
            _manager.pages().confirmation.show("ARE YOU SURE?", [this, slot] (bool result) {
                if (result) {
                    loadSequenceFromSlot(slot);
                }
            });
        }
    });
}

void QuantizerSequencePage::saveSequence() {
    if (!_project.selectedQuantizerSequence().slotAssigned() || sizeof(_project.selectedQuantizerSequence().name()) == 0) {
        saveAsSequence();
        return;
    }
    saveSequenceToSlot(_project.selectedQuantizerSequence().slot());
    showMessage("SEQUENCE SAVED");
}

void QuantizerSequencePage::saveAsSequence() {
    _manager.pages().fileSelect.show("SAVE SEQUENCE", FileType::NoteSequence, _project.selectedQuantizerSequence().slotAssigned() ? _project.selectedQuantizerSequence().slot() : 0, true, [this] (bool result, int slot) {
        if (result) {
            if (FileManager::slotUsed(FileType::NoteSequence, slot)) {
                _manager.pages().confirmation.show("ARE YOU SURE?", [this, slot] (bool result) {
                    if (result) {
                        saveSequenceToSlot(slot);
                    }
                });
            } else {
                saveSequenceToSlot(slot);
            }
        }
    });
}

void QuantizerSequencePage::saveSequenceToSlot(int slot) {
    _manager.pages().busy.show("SAVING SEQUENCE ...");

    FileManager::task([this, slot] () {
        return FileManager::writeNoteSequence(_project.selectedQuantizerSequence(), slot);
    }, [this] (fs::Error result) {
        if (result == fs::OK) {
            showMessage("SEQUENCE SAVED");
        } else {
            showMessage(FixedStringBuilder<32>("FAILED (%s)", fs::errorToString(result)));
        }
        _manager.pages().busy.close();
        _engine.resume();
    });
}

void QuantizerSequencePage::loadSequenceFromSlot(int slot) {
    _manager.pages().busy.show("LOADING SEQUENCE ...");

    FileManager::task([this, slot] () {
        return FileManager::readNoteSequence(_project.selectedQuantizerSequence(), slot);
    }, [this] (fs::Error result) {
        if (result == fs::OK) {
            showMessage("SEQUENCE LOADED");
        } else if (result == fs::INVALID_CHECKSUM) {
            showMessage("INVALID SEQUENCE FILE");
        } else {
            showMessage(FixedStringBuilder<32>("FAILED (%s)", fs::errorToString(result)));
        }
        _manager.pages().busy.close();
        _engine.resume();
    });
}
