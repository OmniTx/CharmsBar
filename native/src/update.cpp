#include "app.h"
#include <winhttp.h>
#include <cstdio>
#include <cstdlib>
#include <cctype>

namespace {

constexpr wchar_t UPDATE_JSON_URL[] = L"https://raw.githubusercontent.com/omnitx/CharmsBar/main/update.json";

using WinHttpOpen_t = HINTERNET(WINAPI*)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
using WinHttpConnect_t = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
using WinHttpOpenRequest_t = HINTERNET(WINAPI*)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
using WinHttpSendRequest_t = BOOL(WINAPI*)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
using WinHttpReceiveResponse_t = BOOL(WINAPI*)(HINTERNET, LPVOID);
using WinHttpQueryHeaders_t = BOOL(WINAPI*)(HINTERNET, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD);
using WinHttpQueryDataAvailable_t = BOOL(WINAPI*)(HINTERNET, LPDWORD);
using WinHttpReadData_t = BOOL(WINAPI*)(HINTERNET, LPVOID, DWORD, LPDWORD);
using WinHttpCloseHandle_t = BOOL(WINAPI*)(HINTERNET);
using WinHttpCrackUrl_t = BOOL(WINAPI*)(LPCWSTR, DWORD, DWORD, LPURL_COMPONENTSW);
using WinHttpSetOption_t = BOOL(WINAPI*)(HINTERNET, DWORD, LPVOID, DWORD);

struct WinApi {
    WinHttpOpen_t Open = nullptr;
    WinHttpConnect_t Connect = nullptr;
    WinHttpOpenRequest_t OpenRequest = nullptr;
    WinHttpSendRequest_t SendRequest = nullptr;
    WinHttpReceiveResponse_t ReceiveResponse = nullptr;
    WinHttpQueryHeaders_t QueryHeaders = nullptr;
    WinHttpQueryDataAvailable_t QueryDataAvailable = nullptr;
    WinHttpReadData_t ReadData = nullptr;
    WinHttpCloseHandle_t CloseHandle = nullptr;
    WinHttpCrackUrl_t CrackUrl = nullptr;
    WinHttpSetOption_t SetOption = nullptr;
    bool ok = false;
};

WinApi LoadWinHttp() {
    WinApi a;
    a.Open = DynFn<WinHttpOpen_t>(L"winhttp.dll", "WinHttpOpen");
    a.Connect = DynFn<WinHttpConnect_t>(L"winhttp.dll", "WinHttpConnect");
    a.OpenRequest = DynFn<WinHttpOpenRequest_t>(L"winhttp.dll", "WinHttpOpenRequest");
    a.SendRequest = DynFn<WinHttpSendRequest_t>(L"winhttp.dll", "WinHttpSendRequest");
    a.ReceiveResponse = DynFn<WinHttpReceiveResponse_t>(L"winhttp.dll", "WinHttpReceiveResponse");
    a.QueryHeaders = DynFn<WinHttpQueryHeaders_t>(L"winhttp.dll", "WinHttpQueryHeaders");
    a.QueryDataAvailable = DynFn<WinHttpQueryDataAvailable_t>(L"winhttp.dll", "WinHttpQueryDataAvailable");
    a.ReadData = DynFn<WinHttpReadData_t>(L"winhttp.dll", "WinHttpReadData");
    a.CloseHandle = DynFn<WinHttpCloseHandle_t>(L"winhttp.dll", "WinHttpCloseHandle");
    a.CrackUrl = DynFn<WinHttpCrackUrl_t>(L"winhttp.dll", "WinHttpCrackUrl");
    a.SetOption = DynFn<WinHttpSetOption_t>(L"winhttp.dll", "WinHttpSetOption");
    a.ok = a.Open && a.Connect && a.OpenRequest && a.SendRequest && a.ReceiveResponse &&
           a.QueryHeaders && a.QueryDataAvailable && a.ReadData && a.CloseHandle && a.CrackUrl && a.SetOption;
    return a;
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &w[0], n);
    return w;
}

std::string WideToNarrow(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
    return s;
}

bool HttpGet(const WinApi& a, const std::wstring& url, std::string& out, DWORD* status) {
    *status = 0;
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256]{};
    wchar_t path[2048]{};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 2048;
    if (!a.CrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &uc)) return false;

    HINTERNET ses = a.Open(L"CharmsBarNative", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) return false;
    HINTERNET con = a.Connect(ses, host, uc.nPort, 0);
    HINTERNET req = nullptr;
    if (con)
        req = a.OpenRequest(con, L"GET", path[0] ? path : L"/", nullptr, WINHTTP_NO_REFERER,
                            WINHTTP_DEFAULT_ACCEPT_TYPE,
                            (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);
    bool ok = false;
    if (req) {
        DWORD t = 5000;
        a.SetOption(req, WINHTTP_OPTION_RESOLVE_TIMEOUT, &t, sizeof(t));
        a.SetOption(req, WINHTTP_OPTION_CONNECT_TIMEOUT, &t, sizeof(t));
        a.SetOption(req, WINHTTP_OPTION_SEND_TIMEOUT, &t, sizeof(t));
        a.SetOption(req, WINHTTP_OPTION_RECEIVE_TIMEOUT, &t, sizeof(t));
        if (a.SendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            a.ReceiveResponse(req, nullptr)) {
            DWORD sz = sizeof(DWORD);
            a.QueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, status, &sz, WINHTTP_NO_HEADER_INDEX);
            DWORD avail = 0;
            while (a.QueryDataAvailable(req, &avail) && avail > 0) {
                std::vector<char> chunk(avail);
                DWORD read = 0;
                if (!a.ReadData(req, chunk.data(), avail, &read) || read == 0) break;
                out.append(chunk.data(), read);
            }
            ok = true;
        }
        a.CloseHandle(req);
    }
    if (con) a.CloseHandle(con);
    a.CloseHandle(ses);
    return ok;
}

bool JsonVal(const std::string& j, const char* key, std::string& out) {
    std::string pat = std::string("\"") + key + "\"";
    size_t p = j.find(pat);
    if (p == std::string::npos) return false;
    p = j.find(':', p + pat.size());
    if (p == std::string::npos) return false;
    ++p;
    while (p < j.size() && isspace(static_cast<unsigned char>(j[p]))) ++p;
    if (p >= j.size() || j[p] != '"') return false;
    ++p;
    std::string r;
    while (p < j.size() && j[p] != '"') {
        if (j[p] == '\\' && p + 1 < j.size()) { ++p; r += j[p]; }
        else r += j[p];
        ++p;
    }
    out = r;
    return true;
}

int CmpVersion(const std::string& a, const std::string& b) {
    long va = strtol(a.c_str(), nullptr, 10);
    long vb = strtol(b.c_str(), nullptr, 10);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

void DownloadAndSwap(HWND owner, const std::wstring& url) {
    WinApi a = LoadWinHttp();
    if (!a.ok) {
        MessageBoxW(owner, L"WinHTTP unavailable.", L"Update Failed", MB_ICONERROR);
        return;
    }
    std::string data;
    DWORD status = 0;
    if (!HttpGet(a, url, data, &status) || (status < 200 || status >= 300) || data.empty()) {
        MessageBoxW(owner, L"Download failed.", L"Update Failed", MB_ICONERROR);
        return;
    }

    std::wstring exePath = ExePath();
    std::wstring tempFile = ExeDirFile(L"update_temp.exe");
    HANDLE f = CreateFileW(tempFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) {
        MessageBoxW(owner, L"Cannot write update file.", L"Update Failed", MB_ICONERROR);
        return;
    }
    DWORD written = 0;
    WriteFile(f, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
    CloseHandle(f);

    size_t nameSplit = exePath.find_last_of(L'\\') + 1;
    std::string exeName = WideToNarrow(exePath.substr(nameSplit));
    std::string exePathN = WideToNarrow(exePath);
    std::string tempFileN = WideToNarrow(tempFile);
    std::wstring batPath = ExeDirFile(L"update.bat");

    char batch[4096]{};
    int n = sprintf_s(batch,
        "@echo off\r\ntimeout /t 1 /nobreak >nul\r\n:loop\r\ntasklist | find /i \"%s\" >nul\r\n"
        "if not errorlevel 1 (\r\n  timeout /t 1 /nobreak >nul\r\n  goto loop\r\n)\r\n"
        "move /y \"%s\" \"%s\" >nul\r\nstart \"\" \"%s\"\r\ndel \"%%~f0\" & exit\r\n",
        exeName.c_str(), tempFileN.c_str(), exePathN.c_str(), exePathN.c_str());
    if (n <= 0) {
        DeleteFileW(tempFile.c_str());
        MessageBoxW(owner, L"Cannot build update script.", L"Update Failed", MB_ICONERROR);
        return;
    }
    HANDLE bf = CreateFileW(batPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (bf == INVALID_HANDLE_VALUE) {
        DeleteFileW(tempFile.c_str());
        MessageBoxW(owner, L"Cannot write update script.", L"Update Failed", MB_ICONERROR);
        return;
    }
    DWORD bw = 0;
    WriteFile(bf, batch, static_cast<DWORD>(n), &bw, nullptr);
    CloseHandle(bf);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmd(batPath.begin(), batPath.end());
    cmd.push_back(L'\0');
    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                       nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        ExitProcess(0);
    }
    DeleteFileW(tempFile.c_str());
    MessageBoxW(owner, L"Failed to launch updater.", L"Update Failed", MB_ICONERROR);
}

}

namespace upd {

void CheckInteractive(HWND owner) {
    WinApi a = LoadWinHttp();
    if (!a.ok) {
        MessageBoxW(owner, L"WinHTTP unavailable.", L"Update Check", MB_ICONERROR);
        return;
    }
    std::wstring url = UPDATE_JSON_URL;
    url += L"?t=" + std::to_wstring(GetTickCount());
    std::string json;
    DWORD status = 0;
    if (!HttpGet(a, url, json, &status) || status != 200) {
        MessageBoxW(owner, (L"Failed to check for updates. HTTP Status: " + std::to_wstring(status)).c_str(),
                    L"Update Check", MB_ICONERROR);
        return;
    }
    std::string v, exeUrl, changelog;
    if (!JsonVal(json, "version", v) || v.empty()) {
        MessageBoxW(owner, L"Failed to parse update information.", L"Update Check", MB_ICONERROR);
        return;
    }
    JsonVal(json, "exe_url", exeUrl);
    JsonVal(json, "changelog", changelog);

    std::string cur = WideToNarrow(APP_VERSION);
    std::wstring curW = APP_VERSION;
    if (CmpVersion(v, cur) > 0) {
        std::wstring msg = L"A new version is available!\n\nCurrent: " + curW +
                           L"\nNew: " + Utf8ToWide(v) + L"\n\nChanges:\n" + Utf8ToWide(changelog) +
                           L"\n\nDo you want to update now?";
        int res = MessageBoxW(owner, msg.c_str(), L"Update Available", MB_YESNO | MB_ICONINFORMATION);
        if (res == IDYES && !exeUrl.empty())
            DownloadAndSwap(owner, Utf8ToWide(exeUrl));
    } else {
        MessageBoxW(owner, (L"You are using the latest version (" + curW + L").").c_str(),
                    L"Update Check", MB_ICONINFORMATION);
    }
}

}
