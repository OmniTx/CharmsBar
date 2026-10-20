#pragma once

namespace actions {
void RunSearch();
void RunClipboard();
void RunStart();
void RunDevices();
void RunSettings();
void RunSnip();
void RunCalc();
void RunTaskMgr();
void RunApp(const std::wstring& path, bool elevate);
void MediaPrev();
void MediaPlayPause();
void MediaNext();
void ShowPowerMenu();
void ApplyStartupTask(bool enable);
void OpenLink(const wchar_t* url);
void CenterActiveWindow();
void HotCornerTick();
}
