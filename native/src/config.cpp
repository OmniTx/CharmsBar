#include "app.h"

static std::wstring IniPath() {
    return ExeDirFile(L"CharmsSettings.ini");
}

static std::wstring ReadStr(const wchar_t* sec, const wchar_t* key, const wchar_t* def) {
    wchar_t buf[1024]{};
    GetPrivateProfileStringW(sec, key, def, buf, 1024, IniPath().c_str());
    return buf;
}

static bool ReadBool(const wchar_t* sec, const wchar_t* key, bool def) {
    return ReadStr(sec, key, def ? L"1" : L"0") == L"1";
}

static void WriteStr(const wchar_t* sec, const wchar_t* key, const std::wstring& v) {
    WritePrivateProfileStringW(sec, key, v.c_str(), IniPath().c_str());
}

void config::Load() {
    g_cfg.searchProvider = ReadStr(L"Settings", L"SearchProvider", g_cfg.searchProvider.c_str());
    g_cfg.everythingPath = ReadStr(L"Paths", L"EverythingPath", g_cfg.everythingPath.c_str());
    g_cfg.launchOnBoot   = ReadBool(L"Settings", L"LaunchOnBoot", false);
    g_cfg.backgroundMode = ReadStr(L"Theme", L"BackgroundMode", g_cfg.backgroundMode.c_str());
    g_cfg.customColor    = ReadStr(L"Theme", L"CustomColor", g_cfg.customColor.c_str());
    g_cfg.bgImage        = ReadStr(L"Theme", L"BgImage", L"");
    g_cfg.gradientStr    = ReadStr(L"Theme", L"GradientStr", g_cfg.gradientStr.c_str());
    g_cfg.forceText      = ReadStr(L"Theme", L"ForceText", g_cfg.forceText.c_str());
    g_cfg.volumeStyle    = ReadStr(L"Settings", L"VolumeStyle", g_cfg.volumeStyle.c_str());
    g_cfg.appAName = ReadStr(L"AppA", L"Name", g_cfg.appAName.c_str());
    g_cfg.appAPath = ReadStr(L"AppA", L"Path", g_cfg.appAPath.c_str());
    g_cfg.appBName = ReadStr(L"AppB", L"Name", g_cfg.appBName.c_str());
    g_cfg.appBPath = ReadStr(L"AppB", L"Path", g_cfg.appBPath.c_str());
    g_cfg.appCName = ReadStr(L"AppC", L"Name", g_cfg.appCName.c_str());
    g_cfg.appCPath = ReadStr(L"AppC", L"Path", g_cfg.appCPath.c_str());
}

void config::Save() {
    WriteStr(L"Settings", L"LaunchOnBoot", g_cfg.launchOnBoot ? L"1" : L"0");
    WriteStr(L"Settings", L"VolumeStyle", g_cfg.volumeStyle);
    WriteStr(L"Settings", L"SearchProvider", g_cfg.searchProvider);
    WriteStr(L"Paths", L"EverythingPath", g_cfg.everythingPath);
    WriteStr(L"Theme", L"BackgroundMode", g_cfg.backgroundMode);
    WriteStr(L"Theme", L"CustomColor", g_cfg.customColor);
    WriteStr(L"Theme", L"BgImage", g_cfg.bgImage);
    WriteStr(L"Theme", L"GradientStr", g_cfg.gradientStr);
    WriteStr(L"Theme", L"ForceText", g_cfg.forceText);
    WriteStr(L"AppA", L"Name", g_cfg.appAName); WriteStr(L"AppA", L"Path", g_cfg.appAPath);
    WriteStr(L"AppB", L"Name", g_cfg.appBName); WriteStr(L"AppB", L"Path", g_cfg.appBPath);
    WriteStr(L"AppC", L"Name", g_cfg.appCName); WriteStr(L"AppC", L"Path", g_cfg.appCPath);
}
