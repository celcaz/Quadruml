#include "Windowing.h"

#include <QWindow>

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif

namespace Windowing {

void bringToFront(QWindow *window)
{
    if (!window)
        return;
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const HWND foreground = GetForegroundWindow();
    if (foreground != hwnd) {
        const DWORD foregroundThread = GetWindowThreadProcessId(foreground, nullptr);
        const DWORD thisThread = GetCurrentThreadId();
        const bool attached = foregroundThread != thisThread
                              && AttachThreadInput(foregroundThread, thisThread, TRUE);
        ShowWindow(hwnd, IsIconic(hwnd) ? SW_RESTORE : SW_SHOW);
        BringWindowToTop(hwnd);
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        if (attached)
            AttachThreadInput(foregroundThread, thisThread, FALSE);
    }
#endif
    window->requestActivate();
}

}
