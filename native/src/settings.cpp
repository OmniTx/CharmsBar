#include "app.h"
#include <commdlg.h>

namespace {

constexpr int ID_BOOT = 101;
constexpr int ID_RADWIN = 102;
constexpr int ID_RADEVT = 103;
constexpr int ID_EDT_EVT = 104;
constexpr int ID_BTN_EVT = 105;
constexpr int ID_CMB_MODE = 201;
constexpr int ID_CMB_TEXT = 202;
constexpr int ID_CMB_VOL = 203;
constexpr int ID_EDT_HEX = 204;
constexpr int ID_EDT_GRAD = 205;
constexpr int ID_EDT_IMG = 206;
constexpr int ID_BTN_IMG = 207;
constexpr int ID_EDT_AN = 301;
constexpr int ID_EDT_AP = 302;
constexpr int ID_BTN_AP = 303;
constexpr int ID_EDT_BN = 304;
constexpr int ID_EDT_BP = 305;
constexpr int ID_BTN_BP = 306;
constexpr int ID_EDT_CN = 307;
constexpr int ID_EDT_CP = 308;
constexpr int ID_BTN_CP = 309;
constexpr int ID_BTN_UPD = 401;
constexpr int ID_BTN_SAVE = 900;

struct SDraw {
    RECT rc;
    std::wstring text;
    int pt;
    int weight;
    COLORREF color;
    UINT align;
    bool underline;
    int page;
};
struct SFrame {
    RECT rc;
    int page;
};

std::vector<SDraw> g_draws;
std::vector<SFrame> g_frames;
RECT g_navRects[4];
int g_navActive = 0;
int g_navHover = -1;
RECT g_linkRect{};

bool g_boot = false;
bool g_bootAtOpen = false;
bool g_searchWin = true;
bool g_navTrack = false;

HBRUSH g_brBg = nullptr;
HBRUSH g_brEdit = nullptr;
HBRUSH g_brNav = nullptr;
HFONT g_fontUi = nullptr;

HWND Ctl(int id) {
    return GetDlgItem(g_hSettings, id);
}

void AddChild(const wchar_t* cls, const wchar_t* text, DWORD style, DWORD ex, int id) {
    CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10,
                    g_hSettings, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_hInst, nullptr);
}

void MoveCtl(int id, RECT rc) {
    SetWindowPos(Ctl(id), nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
}

void Label(int page, int y, const wchar_t* text, COLORREF color) {
    g_draws.push_back({ { SS(175), SS(y), SS(430), SS(y + 20) }, text, 10, 400, color, DT_LEFT, false, page });
}

void Frame(int page, int x, int y, int w, int h, const wchar_t* title) {
    g_frames.push_back({ { SS(x), SS(y), SS(x + w), SS(y + h) }, page });
    g_draws.push_back({ { SS(x + 15), SS(y + 6), SS(x + w), SS(y + 24) }, title, 10, 700,
                        RGB(0xE8, 0xE8, 0xE8), DT_LEFT, false, page });
}

void ApplyFonts() {
    for (int id : { ID_BOOT, ID_RADWIN, ID_RADEVT, ID_EDT_EVT, ID_BTN_EVT, ID_CMB_MODE, ID_CMB_TEXT,
                    ID_CMB_VOL, ID_EDT_HEX, ID_EDT_GRAD, ID_EDT_IMG, ID_BTN_IMG, ID_EDT_AN, ID_EDT_AP,
                    ID_BTN_AP, ID_EDT_BN, ID_EDT_BP, ID_BTN_BP, ID_EDT_CN, ID_EDT_CP, ID_BTN_CP,
                    ID_BTN_UPD, ID_BTN_SAVE })
        SendMessageW(Ctl(id), WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);
}

void LayoutS(HWND hwnd) {
    g_dpi = GetDpiForWindow(hwnd);
    g_fontUi = GetCachedFontPt(10, 400, false, L"Segoe UI");
    g_draws.clear();
    g_frames.clear();

    ApplyFonts();

    for (int i = 0; i < 4; i++)
        g_navRects[i] = { 0, SS(50 + i * 45), SS(140), SS(50 + i * 45 + 40) };

    Frame(0, 160, 20, 300, 80, L"Startup");
    MoveCtl(ID_BOOT, { SS(175), SS(52), SS(435), SS(78) });

    Frame(0, 160, 115, 300, 110, L"Search Provider");
    MoveCtl(ID_RADWIN, { SS(175), SS(150), SS(305), SS(174) });
    MoveCtl(ID_RADEVT, { SS(315), SS(150), SS(445), SS(174) });
    Label(0, 188, L"Path:", RGB(0x80, 0xD0, 0xFF));
    MoveCtl(ID_EDT_EVT, { SS(220), SS(185), SS(370), SS(207) });
    MoveCtl(ID_BTN_EVT, { SS(376), SS(185), SS(404), SS(207) });

    Label(1, 35, L"Background Mode:", RGB(0xE8, 0xE8, 0xE8));
    MoveCtl(ID_CMB_MODE, { SS(280), SS(32), SS(430), SS(232) });
    Label(1, 70, L"Text Color:", RGB(0xE8, 0xE8, 0xE8));
    MoveCtl(ID_CMB_TEXT, { SS(280), SS(67), SS(430), SS(267) });
    Label(1, 105, L"Volume Control:", RGB(0xE8, 0xE8, 0xE8));
    MoveCtl(ID_CMB_VOL, { SS(280), SS(102), SS(430), SS(302) });

    Frame(1, 160, 140, 300, 135, L"Details");
    Label(1, 178, L"Solid Hex:", RGB(0x80, 0xD0, 0xFF));
    MoveCtl(ID_EDT_HEX, { SS(250), SS(175), SS(330), SS(197) });
    Label(1, 208, L"Gradient:", RGB(0x80, 0xD0, 0xFF));
    MoveCtl(ID_EDT_GRAD, { SS(250), SS(205), SS(390), SS(227) });
    Label(1, 238, L"Image Path:", RGB(0x80, 0xD0, 0xFF));
    MoveCtl(ID_EDT_IMG, { SS(250), SS(235), SS(365), SS(257) });
    MoveCtl(ID_BTN_IMG, { SS(371), SS(235), SS(399), SS(257) });

    g_draws.push_back({ { SS(160), SS(30), SS(460), SS(50) }, L"Configure Quick Launch Apps",
                        10, 400, RGB(0x80, 0xD0, 0xFF), DT_CENTER, false, 2 });
    struct AppRow { int idName; int idPath; int idBtn; int y; const wchar_t* lbl; };
    const AppRow rows[3] = {
        { ID_EDT_AN, ID_EDT_AP, ID_BTN_AP, 75, L"A:" },
        { ID_EDT_BN, ID_EDT_BP, ID_BTN_BP, 115, L"B:" },
        { ID_EDT_CN, ID_EDT_CP, ID_BTN_CP, 155, L"C:" },
    };
    for (const AppRow& r : rows) {
        g_draws.push_back({ { SS(160), SS(r.y), SS(185), SS(r.y + 22) }, r.lbl, 10, 400,
                            RGB(0xE8, 0xE8, 0xE8), DT_LEFT, false, 2 });
        MoveCtl(r.idName, { SS(185), SS(r.y), SS(255), SS(r.y + 22) });
        MoveCtl(r.idPath, { SS(262), SS(r.y), SS(397), SS(r.y + 22) });
        MoveCtl(r.idBtn, { SS(403), SS(r.y), SS(431), SS(r.y + 22) });
    }

    g_draws.push_back({ { SS(160), SS(45), SS(460), SS(75) }, L"Charms Bar Platinum", 16, 700,
                        RGB(0xFF, 0xFF, 0xFF), DT_CENTER, false, 3 });
    g_draws.push_back({ { SS(160), SS(80), SS(460), SS(102) },
                        std::wstring(L"Version ") + APP_VERSION, 11, 400, RGB(0x80, 0xD0, 0xFF), DT_CENTER, false, 3 });
    g_draws.push_back({ { SS(160), SS(125), SS(460), SS(145) }, L"Created by Imran Ahmed", 10, 400,
                        RGB(0xFF, 0xFF, 0xFF), DT_CENTER, false, 3 });
    g_linkRect = { SS(160), SS(150), SS(460), SS(170) };
    g_draws.push_back({ g_linkRect, L"github.com/omnitx", 10, 400, RGB(0x80, 0xC0, 0xFF), DT_CENTER, true, 3 });
    g_draws.push_back({ { SS(160), SS(185), SS(460), SS(200) }, L"(Google DeepMind)", 8, 400,
                        RGB(0x80, 0x80, 0x80), DT_CENTER, false, 3 });
    g_draws.push_back({ { SS(160), SS(206), SS(460), SS(221) },
                        std::wstring(L"Toggle hotkey: ") + g_toggleHotkey, 9, 400,
                        RGB(0xB0, 0xB0, 0xB0), DT_CENTER, false, 3 });
    MoveCtl(ID_BTN_UPD, { SS(230), SS(230), SS(390), SS(260) });

    MoveCtl(ID_BTN_SAVE, { SS(230), SS(400), SS(390), SS(435) });

    RECT rcWin{ 0, 0, SS(480), SS(460) };
    AdjustWindowRectEx(&rcWin, GetWindowLongW(hwnd, GWL_STYLE), TRUE, GetWindowLongW(hwnd, GWL_EXSTYLE));
    SetWindowPos(hwnd, nullptr, 0, 0, rcWin.right - rcWin.left, rcWin.bottom - rcWin.top,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
    InvalidateRect(hwnd, nullptr, TRUE);
}

void SwitchPage(int idx) {
    g_navActive = idx;
    const int pageCtl[4][6] = {
        { ID_BOOT, ID_RADWIN, ID_RADEVT, ID_EDT_EVT, ID_BTN_EVT, 0 },
        { ID_CMB_MODE, ID_CMB_TEXT, ID_CMB_VOL, ID_EDT_HEX, ID_EDT_GRAD, 0 },
        { ID_EDT_AN, ID_EDT_AP, ID_BTN_AP, ID_EDT_BN, ID_EDT_BP, 0 },
        { ID_BTN_UPD, 0, 0, 0, 0, 0 },
    };
    for (int p = 0; p < 4; p++) {
        for (int id : pageCtl[p]) {
            if (!id) continue;
            ShowWindow(Ctl(id), p == idx ? SW_SHOW : SW_HIDE);
        }
    }
    ShowWindow(Ctl(ID_EDT_IMG), idx == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(Ctl(ID_BTN_IMG), idx == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(Ctl(ID_EDT_CN), idx == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(Ctl(ID_EDT_CP), idx == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(Ctl(ID_BTN_CP), idx == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(Ctl(ID_BTN_SAVE), SW_SHOW);
    InvalidateRect(g_hSettings, nullptr, FALSE);
}

int NavHit(POINT pt) {
    for (int i = 0; i < 4; i++)
        if (PtInRect(&g_navRects[i], pt)) return i;
    return -1;
}

std::wstring ComboText(HWND cmb) {
    int i = static_cast<int>(SendMessageW(cmb, CB_GETCURSEL, 0, 0));
    if (i < 0) return L"";
    wchar_t buf[128]{};
    SendMessageW(cmb, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(buf));
    return buf;
}

std::wstring EditTxt(int id) {
    wchar_t buf[1024]{};
    GetWindowTextW(Ctl(id), buf, 1024);
    return buf;
}

void SaveAll() {
    g_cfg.launchOnBoot = g_boot;
    g_cfg.searchProvider = g_searchWin ? L"Windows" : L"Everything";
    g_cfg.everythingPath = EditTxt(ID_EDT_EVT);
    g_cfg.backgroundMode = ComboText(Ctl(ID_CMB_MODE));
    g_cfg.forceText = ComboText(Ctl(ID_CMB_TEXT));
    g_cfg.volumeStyle = ComboText(Ctl(ID_CMB_VOL));
    std::wstring hex = EditTxt(ID_EDT_HEX);
    if (!hex.empty() && hex[0] == L'#') hex.erase(0, 1);
    g_cfg.customColor = hex;
    g_cfg.gradientStr = EditTxt(ID_EDT_GRAD);
    g_cfg.bgImage = EditTxt(ID_EDT_IMG);
    g_cfg.appAName = EditTxt(ID_EDT_AN);
    g_cfg.appAPath = EditTxt(ID_EDT_AP);
    g_cfg.appBName = EditTxt(ID_EDT_BN);
    g_cfg.appBPath = EditTxt(ID_EDT_BP);
    g_cfg.appCName = EditTxt(ID_EDT_CN);
    g_cfg.appCPath = EditTxt(ID_EDT_CP);
    config::Save();
    if (g_boot != g_bootAtOpen) actions::ApplyStartupTask(g_boot);
    g_bootAtOpen = g_boot;
    bar::OnConfigChanged();
    DestroyWindow(g_hSettings);
}

std::wstring BrowseFile(const wchar_t* filter) {
    using GetOpenFileNameW_t = BOOL(WINAPI*)(LPOPENFILENAMEW);
    auto f = DynFn<GetOpenFileNameW_t>(L"comdlg32.dll", "GetOpenFileNameW");
    if (!f) return L"";
    wchar_t file[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hSettings;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (!f(&ofn)) return L"";
    return file;
}

void DrawOwnerButton(const DRAWITEMSTRUCT* dis) {
    HDC dc = dis->hDC;
    RECT rc = dis->rcItem;
    bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    COLORREF fill = pressed ? RGB(0x5A, 0x5A, 0x5A) : RGB(0x40, 0x40, 0x40);
    HBRUSH b = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0x60, 0x60, 0x60));
    HGDIOBJ ob = SelectObject(dc, b);
    HGDIOBJ op = SelectObject(dc, pen);
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, SS(8), SS(8));
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(b);
    DeleteObject(pen);
    wchar_t buf[128]{};
    int n = GetWindowTextW(dis->hwndItem, buf, 128);
    HFONT f = GetCachedFontPt(10, 600, false, L"Segoe UI");
    HGDIOBJ of = SelectObject(dc, f);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
    DrawTextW(dc, buf, n, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, of);
}

void DrawOwnerCheck(const DRAWITEMSTRUCT* dis, bool checked, bool radio) {
    HDC dc = dis->hDC;
    RECT rc = dis->rcItem;
    FillRect(dc, &rc, g_brBg);
    int bx = rc.left;
    int by = (rc.top + rc.bottom) / 2 - SS(8);
    int bs = SS(16);
    COLORREF accent = checked ? RGB(0x80, 0xD0, 0xFF) : RGB(0x99, 0x99, 0x99);
    HBRUSH fillB = CreateSolidBrush(RGB(0x40, 0x40, 0x40));
    HPEN pen = CreatePen(PS_SOLID, 1, accent);
    HGDIOBJ ob = SelectObject(dc, fillB);
    HGDIOBJ op = SelectObject(dc, pen);
    if (radio) {
        Ellipse(dc, bx, by, bx + bs, by + bs);
        if (checked) {
            HBRUSH dot = CreateSolidBrush(accent);
            HGDIOBJ od = SelectObject(dc, dot);
            int inset = SS(4);
            Ellipse(dc, bx + inset, by + inset, bx + bs - inset, by + bs - inset);
            SelectObject(dc, od);
            DeleteObject(dot);
        }
    } else {
        Rectangle(dc, bx, by, bx + bs, by + bs);
        if (checked) {
            HPEN cp = CreatePen(PS_SOLID, 2, accent);
            HGDIOBJ oc = SelectObject(dc, cp);
            MoveToEx(dc, bx + SS(3), by + SS(8), nullptr);
            LineTo(dc, bx + SS(7), by + SS(12));
            LineTo(dc, bx + SS(13), by + SS(4));
            SelectObject(dc, oc);
            DeleteObject(cp);
        }
    }
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(fillB);
    DeleteObject(pen);
    wchar_t buf[128]{};
    int n = GetWindowTextW(dis->hwndItem, buf, 128);
    HFONT f = GetCachedFontPt(10, 400, false, L"Segoe UI");
    HGDIOBJ of = SelectObject(dc, f);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
    RECT tr = { rc.left + SS(24), rc.top, rc.right, rc.bottom };
    DrawTextW(dc, buf, n, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, of);
}

LRESULT CALLBACK SettingsProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        int dark = 1;
        DwmSetWindowAttribute(h, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        return 0;
    }
    case WM_PAINT: {
        g_dpi = GetDpiForWindow(h);
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(h, &ps);
        RECT rc{};
        GetClientRect(h, &rc);
        FillRect(dc, &rc, g_brBg);
        RECT nav = { 0, 0, SS(140), rc.bottom };
        FillRect(dc, &nav, g_brNav);
        const wchar_t* navNames[4] = { L"General", L"Appearance", L"Apps", L"About" };
        for (int i = 0; i < 4; i++) {
            if (i == g_navActive) {
                HBRUSH ab = CreateSolidBrush(RGB(0x35, 0x35, 0x35));
                FillRect(dc, &g_navRects[i], ab);
                DeleteObject(ab);
            } else if (i == g_navHover) {
                HBRUSH ab = CreateSolidBrush(RGB(0x2A, 0x2A, 0x2A));
                FillRect(dc, &g_navRects[i], ab);
                DeleteObject(ab);
            }
            HFONT f = GetCachedFontPt(10, i == g_navActive ? 700 : 400, false, L"Segoe UI");
            HGDIOBJ of = SelectObject(dc, f);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
            RECT tr = g_navRects[i];
            DrawTextW(dc, navNames[i], -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, of);
        }
        HPEN fr = CreatePen(PS_SOLID, 1, RGB(0x55, 0x55, 0x55));
        HGDIOBJ op = SelectObject(dc, fr);
        HGDIOBJ ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
        for (const SFrame& f : g_frames)
            if (f.page == g_navActive)
                Rectangle(dc, f.rc.left, f.rc.top, f.rc.right, f.rc.bottom);
        SelectObject(dc, ob);
        SelectObject(dc, op);
        DeleteObject(fr);
        for (const SDraw& d : g_draws) {
            if (d.page != g_navActive) continue;
            HFONT f = GetCachedFontPt(d.pt, d.weight, false, L"Segoe UI");
            HGDIOBJ of = SelectObject(dc, f);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, d.color);
            if (d.underline) {
                RECT urc = d.rc;
                DrawTextW(dc, d.text.c_str(), -1, &urc, d.align | DT_VCENTER | DT_SINGLELINE);
                SIZE sz{};
                GetTextExtentPoint32W(dc, d.text.c_str(), static_cast<int>(d.text.size()), &sz);
                int tx = (urc.left + urc.right - sz.cx) / 2;
                int ty = (urc.top + urc.bottom) / 2 + sz.cy / 2 + 2;
                HPEN up = CreatePen(PS_SOLID, 1, d.color);
                HGDIOBJ ou = SelectObject(dc, up);
                MoveToEx(dc, tx, ty, nullptr);
                LineTo(dc, tx + sz.cx, ty);
                SelectObject(dc, ou);
                DeleteObject(up);
            } else {
                RECT nrc = d.rc;
                DrawTextW(dc, d.text.c_str(), -1, &nrc, d.align | DT_VCENTER | DT_SINGLELINE);
            }
            SelectObject(dc, of);
        }
        EndPaint(h, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DPICHANGED: {
        RECT* r = reinterpret_cast<RECT*>(lp);
        SetWindowPos(h, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        LayoutS(h);
        return 0;
    }
    case WM_MOUSEMOVE: {
        POINT pt{ static_cast<short>(LOWORD(lp)), static_cast<short>(HIWORD(lp)) };
        int n = NavHit(pt);
        if (n != g_navHover) {
            g_navHover = n;
            InvalidateRect(h, nullptr, FALSE);
        }
        if (!g_navTrack) {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = h;
            TrackMouseEvent(&tme);
            g_navTrack = true;
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        g_navTrack = false;
        if (g_navHover != -1) {
            g_navHover = -1;
            InvalidateRect(h, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONUP: {
        POINT pt{ static_cast<short>(LOWORD(lp)), static_cast<short>(HIWORD(lp)) };
        int n = NavHit(pt);
        if (n >= 0 && n != g_navActive) { SwitchPage(n); return 0; }
        if (PtInRect(&g_linkRect, pt) && g_navActive == 3) {
            actions::OpenLink(L"https://github.com/omnitx");
            return 0;
        }
        return 0;
    }
    case WM_DRAWITEM: {
        const DRAWITEMSTRUCT* dis = reinterpret_cast<const DRAWITEMSTRUCT*>(lp);
        switch (dis->CtlID) {
        case ID_BTN_SAVE: case ID_BTN_UPD: case ID_BTN_EVT: case ID_BTN_IMG:
        case ID_BTN_AP: case ID_BTN_BP: case ID_BTN_CP:
            DrawOwnerButton(dis);
            break;
        case ID_BOOT:
            DrawOwnerCheck(dis, g_boot, false);
            break;
        case ID_RADWIN:
            DrawOwnerCheck(dis, g_searchWin, true);
            break;
        case ID_RADEVT:
            DrawOwnerCheck(dis, !g_searchWin, true);
            break;
        default:
            break;
        }
        return TRUE;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp);
        int code = HIWORD(wp);
        if (code != BN_CLICKED) return 0;
        switch (id) {
        case ID_BOOT:
            g_boot = !g_boot;
            InvalidateRect(Ctl(ID_BOOT), nullptr, TRUE);
            break;
        case ID_RADWIN:
            g_searchWin = true;
            InvalidateRect(Ctl(ID_RADWIN), nullptr, TRUE);
            InvalidateRect(Ctl(ID_RADEVT), nullptr, TRUE);
            break;
        case ID_RADEVT:
            g_searchWin = false;
            InvalidateRect(Ctl(ID_RADWIN), nullptr, TRUE);
            InvalidateRect(Ctl(ID_RADEVT), nullptr, TRUE);
            break;
        case ID_BTN_EVT: {
            std::wstring f = BrowseFile(L"Executables (*.exe)\0*.exe\0All Files\0*.*\0");
            if (!f.empty()) SetWindowTextW(Ctl(ID_EDT_EVT), f.c_str());
            break;
        }
        case ID_BTN_IMG: {
            std::wstring f = BrowseFile(L"Images (*.jpg;*.png;*.bmp)\0*.jpg;*.png;*.bmp\0All Files\0*.*\0");
            if (!f.empty()) SetWindowTextW(Ctl(ID_EDT_IMG), f.c_str());
            break;
        }
        case ID_BTN_AP: case ID_BTN_BP: case ID_BTN_CP: {
            std::wstring f = BrowseFile(L"All Files\0*.*\0");
            if (!f.empty()) {
                int target = (id == ID_BTN_AP) ? ID_EDT_AP : (id == ID_BTN_BP) ? ID_EDT_BP : ID_EDT_CP;
                SetWindowTextW(Ctl(target), f.c_str());
            }
            break;
        }
        case ID_BTN_UPD:
            upd::CheckInteractive(h);
            break;
        case ID_BTN_SAVE:
            SaveAll();
            break;
        default:
            break;
        }
        return 0;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
        SetBkColor(dc, RGB(0x40, 0x40, 0x40));
        return reinterpret_cast<LRESULT>(g_brEdit);
    }
    case WM_CLOSE:
        DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        if (g_hSettings == h) g_hSettings = nullptr;
        return 0;
    default:
        return DefWindowProcW(h, msg, wp, lp);
    }
}

void CreateControls(HWND h) {
    AddChild(L"BUTTON", L"Run at Startup (No UAC)", BS_OWNERDRAW, 0, ID_BOOT);
    AddChild(L"BUTTON", L"Windows Search", BS_OWNERDRAW, 0, ID_RADWIN);
    AddChild(L"BUTTON", L"Everything Search", BS_OWNERDRAW, 0, ID_RADEVT);
    AddChild(L"EDIT", g_cfg.everythingPath.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_EVT);
    AddChild(L"BUTTON", L"...", BS_OWNERDRAW, 0, ID_BTN_EVT);
    AddChild(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 0, ID_CMB_MODE);
    AddChild(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 0, ID_CMB_TEXT);
    AddChild(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 0, ID_CMB_VOL);
    AddChild(L"EDIT", g_cfg.customColor.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_HEX);
    AddChild(L"EDIT", g_cfg.gradientStr.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_GRAD);
    AddChild(L"EDIT", g_cfg.bgImage.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_IMG);
    AddChild(L"BUTTON", L"...", BS_OWNERDRAW, 0, ID_BTN_IMG);
    AddChild(L"EDIT", g_cfg.appAName.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_AN);
    AddChild(L"EDIT", g_cfg.appAPath.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_AP);
    AddChild(L"BUTTON", L"...", BS_OWNERDRAW, 0, ID_BTN_AP);
    AddChild(L"EDIT", g_cfg.appBName.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_BN);
    AddChild(L"EDIT", g_cfg.appBPath.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_BP);
    AddChild(L"BUTTON", L"...", BS_OWNERDRAW, 0, ID_BTN_BP);
    AddChild(L"EDIT", g_cfg.appCName.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_CN);
    AddChild(L"EDIT", g_cfg.appCPath.c_str(), WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ID_EDT_CP);
    AddChild(L"BUTTON", L"...", BS_OWNERDRAW, 0, ID_BTN_CP);
    AddChild(L"BUTTON", L"Check for Updates", BS_OWNERDRAW, 0, ID_BTN_UPD);
    AddChild(L"BUTTON", L"Save & Apply", BS_OWNERDRAW, 0, ID_BTN_SAVE);

    const wchar_t* modes[5] = { L"Dark", L"Light", L"Custom", L"Image", L"Gradient" };
    for (int i = 0; i < 5; i++) SendMessageW(Ctl(ID_CMB_MODE), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(modes[i]));
    const wchar_t* texts[3] = { L"Auto", L"White", L"Black" };
    for (int i = 0; i < 3; i++) SendMessageW(Ctl(ID_CMB_TEXT), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(texts[i]));
    const wchar_t* vols[2] = { L"Buttons", L"Slider" };
    for (int i = 0; i < 2; i++) SendMessageW(Ctl(ID_CMB_VOL), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(vols[i]));

    auto selectCombo = [](int id, const std::wstring& v) {
        int i = static_cast<int>(SendMessageW(Ctl(id), CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(v.c_str())));
        SendMessageW(Ctl(id), CB_SETCURSEL, i >= 0 ? i : 0, 0);
    };
    selectCombo(ID_CMB_MODE, g_cfg.backgroundMode);
    selectCombo(ID_CMB_TEXT, g_cfg.forceText);
    selectCombo(ID_CMB_VOL, g_cfg.volumeStyle);
}

}

namespace settings {

void Open() {
    if (g_hSettings) {
        SetForegroundWindow(g_hSettings);
        return;
    }
    WNDCLASSW wc{};
    wc.lpfnWndProc = SettingsProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = L"CharmsBarNativeSettings";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    if (!g_brBg) g_brBg = CreateSolidBrush(RGB(0x20, 0x20, 0x20));
    if (!g_brEdit) g_brEdit = CreateSolidBrush(RGB(0x40, 0x40, 0x40));
    if (!g_brNav) g_brNav = CreateSolidBrush(RGB(0x15, 0x15, 0x15));

    POINT pt{};
    GetCursorPos(&pt);
    RECT rcMon{};
    MonitorUnderPoint(pt, &rcMon);
    int x = rcMon.left + (rcMon.right - rcMon.left - SS(480)) / 2;
    int y = rcMon.top + (rcMon.bottom - rcMon.top - SS(460)) / 2;

    g_hSettings = CreateWindowExW(0, L"CharmsBarNativeSettings", L"Charms Settings",
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                  x, y, SS(480), SS(460), nullptr, nullptr, g_hInst, nullptr);
    if (!g_hSettings) return;
    CreateControls(g_hSettings);
    g_boot = g_cfg.launchOnBoot;
    g_bootAtOpen = g_boot;
    g_searchWin = (g_cfg.searchProvider != L"Everything");
    g_navActive = 0;
    g_fontUi = GetCachedFontPt(10, 400, false, L"Segoe UI");
    LayoutS(g_hSettings);
    SwitchPage(0);
    ShowWindow(g_hSettings, SW_SHOW);
    SetForegroundWindow(g_hSettings);
}

}
