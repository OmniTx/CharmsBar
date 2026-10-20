#include "app.h"

HINSTANCE g_hInst = nullptr;
HWND      g_hBar = nullptr;
HWND      g_hMsg = nullptr;
HWND      g_hSettings = nullptr;
bool      g_barVisible = false;
bool      g_openedByHover = false;
UINT      g_dpi = 96;
AppConfig g_cfg;

int SS(int logicalPx) {
    return MulDiv(logicalPx, static_cast<int>(g_dpi), 96);
}

COLORREF HexToRgb(const std::wstring& hex) {
    std::wstring h = hex;
    if (!h.empty() && h[0] == L'#') h.erase(0, 1);
    if (h.size() != 6) return RGB(0, 0, 0);
    auto part = [](const std::wstring& s) { return static_cast<COLORREF>(wcstoul(s.c_str(), nullptr, 16)); };
    return RGB(part(h.substr(0, 2)), part(h.substr(2, 2)), part(h.substr(4, 2)));
}

COLORREF ContrastText(COLORREF bg) {
    int lum = (GetRValue(bg) * 299 + GetGValue(bg) * 587 + GetBValue(bg) * 114) / 1000;
    return lum >= 128 ? RGB(0, 0, 0) : RGB(255, 255, 255);
}

COLORREF Blend(COLORREF a, COLORREF b, float t) {
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    int r = static_cast<int>(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t);
    int g = static_cast<int>(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t);
    int bl = static_cast<int>(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
    return RGB(r, g, bl);
}

HFONT GetCachedFontPt(int pt, int weight, bool light, const wchar_t* family) {
    static std::map<std::wstring, HFONT> cache;
    std::wstring key = std::to_wstring(pt) + L"|" + std::to_wstring(weight) + L"|" +
                       (light ? L"1" : L"0") + L"|" + family + L"|" + std::to_wstring(static_cast<int>(g_dpi));
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    int h = -MulDiv(pt, static_cast<int>(g_dpi), 72);
    HFONT f = CreateFontW(h, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                          DEFAULT_PITCH, family);
    cache[key] = f;
    return f;
}

bool IsWin11() {
    static int cached = -1;
    if (cached < 0) {
        typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        auto f = DynFn<RtlGetVersionPtr>(L"ntdll.dll", "RtlGetVersion");
        RTL_OSVERSIONINFOW vi{};
        vi.dwOSVersionInfoSize = sizeof(vi);
        cached = (f && f(&vi) == 0 && vi.dwBuildNumber >= 22000) ? 1 : 0;
    }
    return cached == 1;
}

bool IsForegroundFullScreen() {
    HWND h = GetForegroundWindow();
    if (!h || h == g_hBar || h == g_hMsg) return false;
    wchar_t cls[64]{};
    GetClassNameW(h, cls, 64);
    if (!wcscmp(cls, L"Progman") || !wcscmp(cls, L"WorkerW")) return false;
    RECT wr{};
    GetWindowRect(h, &wr);
    HMONITOR hm = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hm, &mi);
    return wr.left <= mi.rcMonitor.left && wr.top <= mi.rcMonitor.top &&
           wr.right >= mi.rcMonitor.right && wr.bottom >= mi.rcMonitor.bottom;
}

void MonitorUnderPoint(const POINT& pt, RECT* rcMon) {
    HMONITOR hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hm, &mi);
    *rcMon = mi.rcMonitor;
}

std::wstring UserName() {
    static std::wstring name;
    if (name.empty()) {
        wchar_t buf[256]{};
        DWORD n = 256;
        auto f = DynFn<decltype(&GetUserNameW)>(L"advapi32.dll", "GetUserNameW");
        if (f && f(buf, &n)) name = buf;
        else name = L"User";
    }
    return name;
}

std::wstring ExePath() {
    wchar_t buf[MAX_PATH]{};
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::wstring(buf, n);
}

std::wstring ExeDirFile(const wchar_t* fileName) {
    std::wstring p = ExePath();
    size_t sl = p.find_last_of(L'\\');
    if (sl == std::wstring::npos) return fileName;
    return p.substr(0, sl + 1) + fileName;
}
