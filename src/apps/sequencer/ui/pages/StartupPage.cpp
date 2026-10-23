#include "StartupPage.h"

#include "ui/pages/Pages.h"
#include "ui/painters/LogoPainter.h"

#include "model/FileManager.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"

#include <algorithm>
#include <cmath>

// Vertical layout. The wordmark's x-span is the alignment grid for everything else on
// the screen, so the credit line, the EDITION lockup and the progress bar all sit on it.
enum {
    CreditBaselineY = 7,
    WordmarkY       = 12,
    ReflectionY     = WordmarkY + LogoPainter::CapHeight,
    EditionBaseline = 49,
    ProgressY       = 57,
    ProgressHeight  = 2,
};

static const char *WordmarkText = CONFIG_EDITION_NAME;

StartupPage::StartupPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
    _startTicks = os::ticks();
}

void StartupPage::draw(Canvas &canvas) {
    if (_state == State::Initial) {
        _state = State::Loading;
        _engine.suspend();
        FileManager::task([this] () {
            return FileManager::readLastProject(_model.project());
        }, [this] (fs::Error result) {
            _engine.resume();
            _state = State::Ready;
        });
    }

    if (relTime() > 1.f && _state == State::Ready) {
        close();
    }

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::None);
    canvas.fill();

    int wordmarkWidth = LogoPainter::wordmarkWidth(WordmarkText);
    int left = (Width - wordmarkWidth) / 2;

    // Credit line: whose shoulders this stands on, deliberately quiet.
    canvas.setFont(Font::Tiny);
    int markWidth = LogoPainter::drawPerformerMark(canvas, left, CreditBaselineY);
    canvas.setColor(Color::MediumLow);
    LogoPainter::drawTrackedText(canvas, left + markWidth + 9, CreditBaselineY, "SEQUENCER", 7);

    FixedStringBuilder<16> version("v%d.%d.%d", CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR, CONFIG_VERSION_REVISION);
    canvas.setColor(Color::Low);
    canvas.drawText(left + wordmarkWidth - canvas.textWidth(version), CreditBaselineY, version);

    // The edition itself.
    LogoPainter::drawWordmark(canvas, left, WordmarkY, WordmarkText, sweep());
    LogoPainter::drawReflection(canvas, left, ReflectionY, WordmarkText);

    canvas.setColor(Color::MediumLow);
    int editionWidth = LogoPainter::trackedTextWidth("EDITION", 14);
    LogoPainter::drawTrackedText(canvas, (Width - editionWidth) / 2, EditionBaseline, "EDITION", 14);

    // Progress is a hairline now -- the sweep across the wordmark carries the sense of
    // something happening.
    canvas.setColor(Color::Low);
    canvas.fillRect(left, ProgressY, wordmarkWidth, ProgressHeight);
    canvas.setColor(Color::Bright);
    canvas.fillRect(left, ProgressY, int(std::min(1.f, relTime()) * wordmarkWidth), ProgressHeight);
}

void StartupPage::updateLeds(Leds &leds) {
    leds.clear();

    // Step LEDs mirror the specular sweep travelling across the wordmark.
    int head = int(sweep() * 16.f);
    for (int i = 0; i < 16; ++i) {
        bool on = i == head;
        leds.set(MatrixMap::fromStep(i), on, on);
    }

    // Track LEDs are the coarse progress meter.
    int progress = int(std::min(1.f, relTime()) * 8.f);
    for (int i = 0; i < 8; ++i) {
        bool active = i < progress;
        leds.set(MatrixMap::fromTrack(i), active, active);
    }
}

float StartupPage::time() const {
    return (os::ticks() - _startTicks) / float(os::time::ms(1000));
}

float StartupPage::sweep() const {
    // Keeps shimmering if the card is slow, instead of going dead once relTime() > 1.
    return std::fmod(time() / SweepPeriod, 1.f);
}
