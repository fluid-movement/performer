#pragma once

#include "BasePage.h"

class StartupPage : public BasePage {
public:
    StartupPage(PageManager &manager, PageContext &context);

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual bool isModal() const override { return true; }

private:
    enum class State {
        Initial,
        Loading,
        Ready,
    };

    float time() const;
    float relTime() const { return time() / LoadTime; }

    // Phase of the specular highlight, wrapped to [0,1). Independent of relTime() so
    // the logo keeps moving when a slow card holds the splash open past LoadTime.
    float sweep() const;

    static constexpr int LoadTime = 2;
    static constexpr float SweepPeriod = 2.f;

    uint32_t _startTicks;
    State _state = State::Initial;
};
