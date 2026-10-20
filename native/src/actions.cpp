#include "app.h"
#include <powrprof.h>

namespace {

void OpenUri(const wchar_t* uri) {
    ShellExecuteW(nullptr, L"open", uri, nullptr, nullptr, SW_SHOWNORMAL);
}

void SendAppCommand(int cmd) {
    HWND fg = GetForegroundWindow();
    if (!fg || fg == g_hBar || fg == g_hMsg) fg = GetShellWindow();
    if (fg) PostMessageW(fg, WM_APPCOMMAND, 0, static_cast<LPARAM>(cmd) << 16);
}

bool DoPrivilege(const wchar_t* privilege) {
    auto openTok = DynFn<decltype(&OpenProcessToken)>(L"advapi32.dll", "OpenProcessToken");
    auto lookup = DynFn<decltype(&LookupPrivilegeValueW)>(L"advapi32.dll", "LookupPrivilegeValueW");
    auto adjust = DynFn<decltype(&AdjustTokenPrivileges)>(L"advapi32.dll", "AdjustTokenPrivileges");
    if (!openTok || !lookup || !adjust) return false;
    HANDLE tok = nullptr;
    if (!openTok(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok)) return false;
    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    lookup(nullptr, privilege, &tp.Privileges[0].Luid);
    adjust(tok, FALSE, &tp, 0, nullptr, nullptr);
    CloseHandle(tok);
    return true;
}

void DoShutdown(UINT flags) {
    DoPrivilege(SE_SHUTDOWN_NAME);
    ExitWindowsEx(flags | EWX_FORCEIFHUNG, SHTDN_REASON_FLAG_PLANNED);
}

void DoSleep() {
    auto f = DynFn<decltype(&SetSuspendState)>(L"powrprof.dll", "SetSuspendState");
    if (f) f(FALSE, FALSE, FALSE);
}

}

namespace actions {

void RunSearch() {
    if (g_cfg.searchProvider == L"Everything") {
        if (GetFileAttributesW(g_cfg.everythingPath.c_str()) != INVALID_FILE_ATTRIBUTES)
            RunApp(g_cfg.everythingPath, false);
        else
            MessageBoxW(nullptr, L"Everything.exe not found.", L"Charms", MB_ICONERROR);
    } else {
        OpenUri(L"ms-search:");
    }
}

void RunClipboard() { OpenUri(L"ms-settings:clipboard"); }
void RunDevices() { OpenUri(L"ms-settings:connecteddevices"); }
void RunSettings() { OpenUri(L"ms-settings:"); }
void RunSnip() { OpenUri(L"ms-screenclip:"); }
void RunCalc() { RunApp(L"calc.exe", false); }
void RunTaskMgr() { RunApp(L"taskmgr.exe", false); }

void RunStart() {
    HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (tray) PostMessageW(tray, WM_SYSCOMMAND, SC_TASKLIST, 0);
}

void RunApp(const std::wstring& path, bool elevate) {
    ShellExecuteW(nullptr, elevate ? L"runas" : L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void MediaPrev() { SendAppCommand(APPCOMMAND_MEDIA_PREVIOUSTRACK); }
void MediaPlayPause() { SendAppCommand(APPCOMMAND_MEDIA_PLAY_PAUSE); }
void MediaNext() { SendAppCommand(APPCOMMAND_MEDIA_NEXTTRACK); }

void ShowPowerMenu() {
    HMENU m = CreatePopupMenu();
    if (!m) return;
    AppendMenuW(m, MF_STRING, 1, L"Sleep");
    AppendMenuW(m, MF_STRING, 2, L"Restart");
    AppendMenuW(m, MF_STRING, 3, L"Shut Down");
    HWND h = g_hBar;
    SetForegroundWindow(h);
    POINT pt{};
    GetCursorPos(&pt);
    int id = TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, h, nullptr);
    DestroyMenu(m);
    PostMessageW(h, WM_NULL, 0, 0);
    if (id == 1) DoSleep();
    else if (id == 2) DoShutdown(EWX_REBOOT);
    else if (id == 3) DoShutdown(EWX_SHUTDOWN | EWX_POWEROFF);
}

void ApplyStartupTask(bool enable) {
    std::wstring cmd;
    if (enable)
        cmd = L"schtasks /Create /TN \"" + std::wstring(APP_TASKNAME) + L"\" /TR \"\\\"" + ExePath() + L"\\\"\" /SC ONLOGON /F";
    else
        cmd = L"schtasks /Delete /TN \"" + std::wstring(APP_TASKNAME) + L"\" /F";
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(L'\0');
    if (CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                       nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

void OpenLink(const wchar_t* url) {
    ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

void CenterActiveWindow() {
    HWND h = GetForegroundWindow();
    if (!h || h == g_hBar || h == g_hMsg || h == g_hSettings) return;
    RECT wr{};
    GetWindowRect(h, &wr);
    HMONITOR hm = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hm, &mi);
    int w = wr.right - wr.left;
    int hh = wr.bottom - wr.top;
    int x = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - w) / 2;
    int y = mi.rcWork.top + ((mi.rcWork.bottom - mi.rcWork.top) - hh) / 2;
    SetWindowPos(h, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

void HotCornerTick() {
    static ULONGLONG cornerStart = 0;
    if (IsForegroundFullScreen()) {
        cornerStart = 0;
        return;
    }
    POINT pt{};
    GetCursorPos(&pt);
    RECT rcMon{};
    MonitorUnderPoint(pt, &rcMon);
    bool inCorner = pt.x >= rcMon.right - 10 && pt.y <= rcMon.top + 10;
    if (inCorner && !bar::Visible()) {
        ULONGLONG now = GetTickCount64();
        if (cornerStart == 0) {
            cornerStart = now;
        } else if (now - cornerStart > 400) {
            cornerStart = 0;
            bar::Show(true);
        }
    } else {
        cornerStart = 0;
        if (bar::Visible() && g_openedByHover) {
            RECT rc{};
            GetWindowRect(g_hBar, &rc);
            if (pt.x < rc.left || pt.y < rc.top || pt.y > rc.bottom)
                bar::Hide();
        }
    }
}

}
