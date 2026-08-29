#pragma once

#include "BasePage.h"

#include "ui/model/ListModel.h"

// Modeless list page.
//
// There is no edit mode: turning the encoder always changes the thing the page
// is about. For two-column models that means editing the selected row's value;
// for single-column models (pickers, where the row *is* the value) it means
// moving the cursor. Left/Right always navigate rows, and the encoder press is
// left untouched so pages can use it to descend into a sub-page.
class ListPage : public BasePage {
public:
    ListPage(PageManager &manager, PageContext &context, ListModel &listModel);

    void setListModel(ListModel &listModel);

    virtual void show() override;

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;

    int selectedRow() const { return _selectedRow; }
    void setSelectedRow(int selectedRow);

    int activeSection() const { return _activeSection; }
    void setActiveSection(int section);

    void setTopRow(int row);

protected:
    virtual void drawCell(Canvas &canvas, int row, int column, int x, int y, int w, int h);

    // Draws the section tabs into the footer. Does nothing when the model
    // declares no sections, leaving the footer free for the page's own keys.
    void drawSectionFooter(Canvas &canvas);

    // Right pane: the selected row blown up, with a preview when available.
    void drawDetailPanel(Canvas &canvas);

    static constexpr int LineHeight = 7;
    static constexpr int LineCount = 6;
    static constexpr int ContentTop = 10;
    static constexpr int ContentHeight = LineCount * LineHeight;   // 42

    // Two-pane split. The list keeps the left 164px; the right pane shows the
    // selected parameter -- a drawing of what it does when the model provides
    // one, otherwise its value at a size the list row cannot afford.
    static constexpr int ListWidth = 164;
    static constexpr int DividerX = 170;
    static constexpr int PanelX = 176;
    static constexpr int PanelWidth = 76;
    static constexpr int LabelX = 8;
    static constexpr int ValueX = 60;

private:
    // Rows are filtered by the active section. Models without sections report
    // section 0 for every row, so the unfiltered case falls out for free.
    int rowsInSection() const;
    int sectionRowAt(int index) const;
    int indexInSection(int row) const;

    void advanceSelection();
    void moveSelection(int delta);
    void scrollTo(int index);

    ListModel *_listModel;
    int _selectedRow = 0;
    int _displayIndex = 0;
    int _activeSection = 0;
};
