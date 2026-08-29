#pragma once

#include "core/utils/StringBuilder.h"
#include "core/gfx/Canvas.h"

class ListModel {
public:
    virtual int rows() const = 0;

    // Number of value columns.
    //   2 = label + value. Encoder edits the value, Left/Right navigate rows.
    //   1 = the row *is* the value (pickers). Encoder moves the cursor.
    virtual int columns() const = 0;

    virtual void cell(int row, int column, StringBuilder &str) const = 0;

    virtual void edit(int row, int column, int value, bool shift) = 0;

    virtual int indexedCount(int row) const { return 0; }
    virtual int indexed(int row) const { return -1; }
    virtual void setIndexed(int row, int index) {}

    // Optional grouping into F-key selectable sections. Returning 0 means the
    // model has no sections and all rows are shown at once. Sections are keyed
    // per row rather than by contiguous range so that `enum Item` orderings do
    // not have to be reshuffled.
    virtual int sections() const { return 0; }
    virtual void sectionName(int section, StringBuilder &str) const {}
    virtual int rowSection(int row) const { return 0; }

    // Optional drawing of what a parameter *does* -- a slide ramp, a pulse
    // train, a swung beat grid -- shown in the config page's right pane. Rows
    // without one fall back to the value drawn large.
    virtual bool hasPreview(int row) const { return false; }
    virtual void drawPreview(Canvas &canvas, int row, int x, int y, int w, int h) const {}

    virtual void setSelectedScale(int defaultScale, bool force = false) {}
};
