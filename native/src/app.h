#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <map>

#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUNDSMALL
#define DWMWCP_ROUNDSMALL 2
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMSBT_TRANSIENTWINDOW
#define DWMSBT_TRANSIENTWINDOW 2
#endif

#include "config.h"
#include "volume.h"
#include "actions.h"
#include "bar.h"
#include "settings.h"
#include "update.h"

constexpr wchar_t APP_VERSION[]  = L"24";
constexpr wchar_t APP_MUTEX[]    = L"CharmsBarNative_Instance";
constexpr wchar_t APP_TASKNAME[] = L"CharmsBarAutoStart";

extern HINSTANCE g_hInst;
extern HWND      g_hBar;
extern HWND      g_hMsg;
extern HWND      g_hSettings;
extern bool      g_barVisible;
extern bool      g_openedByHover;
extern UINT      g_dpi;
extern std::wstring g_toggleHotkey;

struct AppConfig {
    std::wstring searchProvider = L"Windows";
    std::wstring everythingPath = L"C:\\Program Files\\Everything 1.5a\\Everything.exe";
    bool         launchOnBoot   = false;
    std::wstring backgroundMode = L"Dark";
    std::wstring customColor    = L"101010";
    std::wstring bgImage;
    std::wstring gradientStr    = L"2E3192,1BFFFF";
    std::wstring forceText      = L"Auto";
    std::wstring volumeStyle    = L"Slider";
    std::wstring appAName = L"Notepad";        std::wstring appAPath = L"notepad.exe";
    std::wstring appBName = L"File Explorer";  std::wstring appBPath = L"explorer.exe";
    std::wstring appCName = L"Cmd";            std::wstring appCPath = L"cmd.exe";
};
extern AppConfig g_cfg;

int      SS(int logicalPx);
COLORREF HexToRgb(const std::wstring& hex);
COLORREF ContrastText(COLORREF bg);
COLORREF Blend(COLORREF a, COLORREF b, float t);
HFONT    GetCachedFontPt(int pt, int weight, bool light, const wchar_t* family);
bool     IsWin11();
bool     IsForegroundFullScreen();
void     MonitorUnderPoint(const POINT& pt, RECT* rcMon);
std::wstring UserName();
std::wstring ExeDirFile(const wchar_t* fileName);
std::wstring ExePath();

template <typename FN>
FN DynFn(const wchar_t* dll, const char* proc) {
    HMODULE h = GetModuleHandleW(dll);
    if (!h) h = LoadLibraryW(dll);
    return h ? reinterpret_cast<FN>(reinterpret_cast<void*>(GetProcAddress(h, proc))) : nullptr;
}
