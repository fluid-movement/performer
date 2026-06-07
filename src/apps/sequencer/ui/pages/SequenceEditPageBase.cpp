#include "SequenceEditPageBase.h"

SequenceEditPageBase::SequenceEditPageBase(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{}

int SequenceEditPageBase::activeFunctionKey() {
    return _activeTab;
}

void SequenceEditPageBase::contextShow(bool doubleClick) {
    showContextMenu(ContextMenu(
        contextItems(), contextActionCount(),
        [&](int index){ contextAction(index); },
        [&](int index){ return contextActionEnabled(index); }, doubleClick
    ));
}

bool SequenceEditPageBase::handleCommonKeyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    if (key.isContextMenu()) {
        contextShow();
        event.consume();
        return true;
    }
    if (key.pageModifier() && event.count() == 2) {
        contextShow(true);
        event.consume();
        return true;
    }
    return false;
}
