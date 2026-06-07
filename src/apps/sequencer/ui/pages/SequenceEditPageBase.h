#pragma once

#include "BasePage.h"

class SequenceEditPageBase : public BasePage {
public:
    SequenceEditPageBase(PageManager &manager, PageContext &context);

protected:
    int _activeTab = 0;

    virtual int activeFunctionKey();

    void contextShow(bool doubleClick = false);
    bool handleCommonKeyPress(KeyPressEvent &event);

private:
    virtual const ContextMenuModel::Item *contextItems() const = 0;
    virtual int contextActionCount() const = 0;
    virtual void contextAction(int index) = 0;
    virtual bool contextActionEnabled(int index) const = 0;
};
