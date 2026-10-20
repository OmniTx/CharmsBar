#include "app.h"

namespace {

constexpr int HK_TOGGLE = 1;
constexpr int HK_CENTER = 2;
constexpr int TIMER_CORNER = 1;

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

using SetProcessDpiAwarenessContext_t = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);

LRESULT CALLBACK MsgProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_HOTKEY:
        if (wp == HK_TOGGLE) {
            if (!IsForegroundFullScreen()) bar::Toggle(false);
        } else if (wp == HK_CENTER) {
            actions::CenterActiveWindow();
        }
        return 0;
    case WM_TIMER:
        if (wp == TIMER_CORNER) actions::HotCornerTick();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(h, msg, wp, lp);
    }
}

HWND CreateMsgWindow() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = MsgProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = L"CharmsBarNativeMsg";
    RegisterClassW(&wc);
    return CreateWindowExW(0, wc.lpszClassName, L"", WS_POPUP, 0, 0, 0, 0,
                           nullptr, nullptr, g_hInst, nullptr);
}

}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    g_hInst = hInst;
    auto setDpiCtx = DynFn<SetProcessDpiAwarenessContext_t>(L"user32.dll", "SetProcessDpiAwarenessContext");
    if (setDpiCtx) setDpiCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HANDLE mutex = CreateMutexW(nullptr, TRUE, APP_MUTEX);
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    config::Load();
    volume::Init();
    bar::CreateOnce();
    g_hMsg = CreateMsgWindow();
    if (!g_hMsg) return 0;

    RegisterHotKey(g_hMsg, HK_TOGGLE, MOD_WIN | MOD_NOREPEAT, 'C');
    RegisterHotKey(g_hMsg, HK_CENTER, MOD_WIN | MOD_ALT | MOD_NOREPEAT, 'C');
    SetTimer(g_hMsg, TIMER_CORNER, 100, nullptr);

    MSG m{};
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
