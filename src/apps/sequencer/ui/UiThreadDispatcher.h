#pragma once

#include <functional>

class UiThreadDispatcher {
public:
    static void dispatchToMainThread(std::function<void()> task);
};
