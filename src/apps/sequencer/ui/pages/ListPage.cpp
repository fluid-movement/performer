#include "ListPage.h"

#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"

#include "core/math/Math.h"
#include "core/utils/StringBuilder.h"

ListPage::ListPage(PageManager &manager, PageContext &context, ListModel &listModel) :
    BasePage(manager, context)
{
    setListModel(listModel);
}

void ListPage::setListModel(ListModel &listModel) {
    // Careful: this runs from the constructor, where the base class is built
    // before the derived page's model member exists. Only reset plain state
    // here -- do not call into the model. enter() does the first real read.
    _listModel = &listModel;
    _activeSection = 0;
    _displayIndex = 0;
    _selectedRow = 0;
}

void ListPage::show() {
    BasePage::show();
}

void ListPage::enter() {
    scrollTo(indexInSection(_selectedRow));
}

void ListPage::exit() {
}

int ListPage::rowsInSection() const {
    int count = 0;
    for (int row = 0; row < _listModel->rows(); ++row) {
        if (_listModel->rowSection(row) == _activeSection) {
            ++count;
        }
    }
    return count;
}

int ListPage::sectionRowAt(int index) const {
    int count = 0;
    for (int row = 0; row < _listModel->rows(); ++row) {
        if (_listModel->rowSection(row) == _activeSection) {
            if (count == index) {
                return row;
            }
            ++count;
        }
    }
    return -1;
}

int ListPage::indexInSection(int row) const {
    int count = 0;
    for (int i = 0; i < _listModel->rows(); ++i) {
        if (_listModel->rowSection(i) == _activeSection) {
            if (i == row) {
                return count;
            }
            ++count;
        }
    }
    return 0;
}

void ListPage::draw(Canvas &canvas) {
    int count = rowsInSection();

    int displayIndex = _displayIndex;
    if (displayIndex + LineCount > count) {
        displayIndex = std::max(0, count - LineCount);
    }

    bool singleColumn = _listModel->columns() < 2;

    for (int i = 0; i < LineCount; ++i) {
        int index = displayIndex + i;
        if (index >= count) {
            break;
        }
        int row = sectionRowAt(index);
        int y = ContentTop + i * LineHeight;

        // Inverted fill for the selected row -- the one selection convention
        // shared by every page in the redesign.
        if (row == _selectedRow) {
            canvas.setBlendMode(BlendMode::Set);
            canvas.setColor(Color::Bright);
            canvas.fillRect(0, y, Width, LineHeight);
        }

        if (singleColumn) {
            drawCell(canvas, row, 0, 8, y, Width - 16, LineHeight);
        } else {
            drawCell(canvas, row, 0, 8, y, 84, LineHeight);
            drawCell(canvas, row, 1, 96, y, Width - 104, LineHeight);
        }
    }

    WindowPainter::drawScrollbar(canvas, Width - 6, ContentTop, 4, LineCount * LineHeight, count, LineCount, displayIndex);
}

void ListPage::drawSectionFooter(Canvas &canvas) {
    int sections = _listModel->sections();
    if (sections <= 0) {
        WindowPainter::drawFooter(canvas);
        return;
    }

    FixedStringBuilder<8> names[CONFIG_FUNCTION_KEY_COUNT];
    const char *namePtrs[CONFIG_FUNCTION_KEY_COUNT];
    for (int i = 0; i < CONFIG_FUNCTION_KEY_COUNT; ++i) {
        if (i < sections) {
            _listModel->sectionName(i, names[i]);
            namePtrs[i] = names[i];
        } else {
            namePtrs[i] = nullptr;
        }
    }

    WindowPainter::drawFooter(canvas, namePtrs, pageKeyState(), _activeSection);
}

void ListPage::updateLeds(Leds &leds) {
    // The function keys have no LEDs on this hardware -- the LED matrix covers
    // the 8 track, 16 step and 8 global buttons only (indices 0..31, where
    // Key::F0 is already 32). The active section is communicated by the
    // inverted footer tab instead.
}

void ListPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.pageModifier()) {
        return;
    }

    if (key.isFunction() && _listModel->sections() > 0) {
        int section = key.function();
        if (section < _listModel->sections()) {
            setActiveSection(section);
        }
        event.consume();
        return;
    }

    if (key.isLeft()) {
        moveSelection(-1);
        event.consume();
    } else if (key.isRight()) {
        moveSelection(1);
        event.consume();
    }

    // The encoder press is deliberately not handled here: pages use it to
    // descend into sub-pages (name entry, calibration, confirm) and it must not
    // be swallowed by the list.
}

void ListPage::encoder(EncoderEvent &event) {
    if (_listModel->columns() < 2) {
        // Single-column model: the row is the value.
        moveSelection(event.value());
    } else {
        _listModel->edit(_selectedRow, 1, event.value(), event.pressed() | globalKeyState()[Key::Shift]);
    }
    event.consume();
}

void ListPage::setSelectedRow(int selectedRow) {
    if (_listModel->rows() == 0) {
        return;
    }

    int row = clamp(selectedRow, 0, _listModel->rows() - 1);

    // Absolute selection may target a row outside the active section (pages jump
    // straight to a row after editing). Follow the row rather than refusing it.
    int section = _listModel->rowSection(row);
    if (section != _activeSection) {
        _activeSection = section;
        _displayIndex = 0;
    }

    _selectedRow = row;
    scrollTo(indexInSection(row));
}

void ListPage::moveSelection(int delta) {
    int count = rowsInSection();
    if (count == 0) {
        return;
    }
    int index = clamp(indexInSection(_selectedRow) + delta, 0, count - 1);
    int row = sectionRowAt(index);
    if (row >= 0) {
        _selectedRow = row;
        scrollTo(index);
    }
}

void ListPage::setActiveSection(int section) {
    if (section == _activeSection) {
        return;
    }
    _activeSection = section;
    int row = sectionRowAt(0);
    _selectedRow = row >= 0 ? row : 0;
    _displayIndex = 0;
}

void ListPage::setTopRow(int row) {
    _displayIndex = std::min(row, std::max(0, rowsInSection() - LineCount));
}

void ListPage::drawCell(Canvas &canvas, int row, int column, int x, int y, int w, int h) {
    FixedStringBuilder<32> str;
    _listModel->cell(row, column, str);
    canvas.setFont(Font::Tiny);
    // Selected rows are drawn as dark text knocked out of the bright fill.
    canvas.setBlendMode(row == _selectedRow ? BlendMode::Sub : BlendMode::Set);
    canvas.setColor(row == _selectedRow ? Color::Bright : Color::MediumLow);
    canvas.drawText(x, y + 6, str);
}

void ListPage::scrollTo(int index) {
    if (index < _displayIndex) {
        _displayIndex = index;
    } else if (index >= _displayIndex + LineCount) {
        _displayIndex = index - (LineCount - 1);
    }
}
