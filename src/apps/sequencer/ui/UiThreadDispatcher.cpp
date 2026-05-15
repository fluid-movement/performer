#include "UiThreadDispatcher.h"

#include "Ui.h"

void UiThreadDispatcher::dispatchToMainThread(std::function<void()> task) {
    Ui::dispatchToMainThread(task);
}