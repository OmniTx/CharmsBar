#include "app.h"
#include <map>
#include <algorithm>
#include <cmath>
#include <objbase.h>
#include <dwmapi.h>
#include <initguid.h>
#include <wincodec.h>

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

namespace {

enum class Id { None, Search, Clipboard, Start, Devices, SettingsCharm,
                MediaPrev, MediaPlay, MediaNext,
                VolIcon, VolTrack, VolDown, VolUp,
                Snip, Calc, Task, AppA, AppB, AppC, Power, Gear };

constexpr int TIMER_CLOCK = 2;
constexpr int TIMER_RAINBOW = 3;
constexpr int TIMER_TIP = 4;
constexpr int TIMER_ANIM = 5;

constexpr wchar_t MDL2[] = L"Segoe MDL2 Assets";
constexpr wchar_t SEGOE[] = L"Segoe UI";
constexpr wchar_t SEGOE_LIGHT[] = L"Segoe UI Light";
constexpr wchar_t SEGOE_SEMIBOLD[] = L"Segoe UI Semibold";

struct GlyphDraw { RECT rc; wchar_t ch; int pt; int weight; COLORREF color; const wchar_t* family; };
struct TextDraw  { RECT rc; std::wstring text; int pt; int weight; COLORREF color; const wchar_t* family; UINT align; };
struct HitItem   { Id id; RECT rc; };
struct TipEntry  { Id id; std::wstring text; };

std::vector<GlyphDraw> g_glyphs;
std::vector<TextDraw> g_texts;
std::vector<HitItem> g_hits;
std::vector<RECT> g_dividers;
std::vector<TipEntry> g_tips;

Id g_hover = Id::None;
Id g_pressed = Id::None;
Id g_focusId = Id::None;
bool g_barFocus = false;
std::vector<Id> g_navOrder;
int g_animPhase = 0;
ULONGLONG g_animStart = 0;
int g_animFromX = 0, g_animToX = 0, g_animY = 0;
int g_w = 0, g_h = 0;
RECT g_rcMon{};
float g_hue = 0.f;
std::wstring g_timeStr, g_ampmStr, g_dateStr;
bool g_trackMouse = false;
int g_appTextIdx[3] = { -1, -1, -1 };
RECT g_rcVolTrack{};
RECT g_rcVolFill{};
int g_volTextIdx = -1;
int g_volIconIdx = -1;

COLORREF g_bg = RGB(0, 0, 0);
COLORREF g_text = RGB(255, 255, 255);
COLORREF g_sub = RGB(0xBB, 0xBB, 0xBB);
int g_bgAlpha = 235;
bool g_acrylic = false;
bool g_backdropOK = false;

HBITMAP g_hbmImage = nullptr;
std::wstring g_loadedImage;
int g_imgW = 0, g_imgH = 0;

HWND g_hTip = nullptr;

HDC g_dcBg = nullptr, g_dcFg = nullptr, g_dcMask = nullptr, g_dcOut = nullptr;
HBITMAP g_bmpBg = nullptr, g_bmpFg = nullptr, g_bmpMask = nullptr, g_bmpOut = nullptr;
BYTE* g_bitsBg = nullptr;
BYTE* g_bitsFg = nullptr;
BYTE* g_bitsMask = nullptr;
BYTE* g_bitsOut = nullptr;
int g_bufW = 0, g_bufH = 0;

short X16(LPARAM l) { return static_cast<short>(LOWORD(l)); }
short Y16(LPARAM l) { return static_cast<short>(HIWORD(l)); }

void UpdateAppColors();
void Compose();
void Layout();
void MoveFocus(int dir);
void StartAnim(bool show);
void TickAnim();

COLORREF Hsv(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.f - fabsf(fmodf(h / 60.f, 2.f) - 1.f));
    float m = v - c;
    float r = 0.f, g = 0.f, b = 0.f;
    if (h < 60.f)      { r = c; g = x; }
    else if (h < 120.f){ r = x; g = c; }
    else if (h < 180.f){ g = c; b = x; }
    else if (h < 240.f){ g = x; b = c; }
    else if (h < 300.f){ r = x; b = c; }
    else               { r = c; b = x; }
    return RGB(static_cast<COLORREF>((r + m) * 255.f),
               static_cast<COLORREF>((g + m) * 255.f),
               static_cast<COLORREF>((b + m) * 255.f));
}

std::wstring GetBatteryInfo() {
    SYSTEM_POWER_STATUS sps{};
    if (!GetSystemPowerStatus(&sps)) return L"";
    if (sps.BatteryFlag == 255 || sps.BatteryLifePercent == 255) return L"AC Power";
    wchar_t buf[40]{};
    swprintf_s(buf, L"%s%d%%", sps.ACLineStatus ? L"Charging " : L"Battery ", sps.BatteryLifePercent);
    return buf;
}

wchar_t VolGlyph() {
    if (volume::GetMute()) return static_cast<wchar_t>(0xE74F);
    int v = volume::GetVolume();
    if (v == 0) return static_cast<wchar_t>(0xE74F);
    if (v < 30) return static_cast<wchar_t>(0xE992);
    if (v < 70) return static_cast<wchar_t>(0xE993);
    return static_cast<wchar_t>(0xE767);
}

bool UpdateClockStrings() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    int h12 = st.wHour % 12;
    if (h12 == 0) h12 = 12;
    std::wstring t = std::to_wstring(h12) + (st.wMinute < 10 ? L":0" : L":") + std::to_wstring(st.wMinute);
    std::wstring ap = st.wHour < 12 ? L"AM" : L"PM";
    wchar_t db[64]{};
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, db, 64);
    std::wstring d = db;
    bool changed = (t != g_timeStr || ap != g_ampmStr || d != g_dateStr);
    g_timeStr = t;
    g_ampmStr = ap;
    g_dateStr = d;
    return changed;
}

void AddDivider(int y) {
    g_dividers.push_back({ SS(10), y, g_w - SS(10), y + 1 });
}

void AddTip(Id id, const std::wstring& tip) {
    g_tips.push_back({ id, tip });
}

HBITMAP DecodeImageFile(const std::wstring& path, int* outW, int* outH) {
    *outW = 0;
    *outH = 0;
    auto coInit = DynFn<decltype(&CoInitializeEx)>(L"ole32.dll", "CoInitializeEx");
    if (coInit) coInit(nullptr, COINIT_APARTMENTTHREADED);
    auto coCreate = DynFn<decltype(&CoCreateInstance)>(L"ole32.dll", "CoCreateInstance");
    if (!coCreate) return nullptr;

    static const CLSID kClsidWicFactory =
        { 0xcacaf262, 0x9370, 0x4615, { 0xa1, 0x3b, 0x9f, 0x55, 0x39, 0xda, 0x4c, 0x0a } };
    IWICImagingFactory* fac = nullptr;
    HBITMAP hbmp = nullptr;
    if (FAILED(coCreate(kClsidWicFactory, nullptr, CLSCTX_INPROC_SERVER,
                        __uuidof(IWICImagingFactory), reinterpret_cast<void**>(&fac))) || !fac)
        return nullptr;

    IWICBitmapDecoder* dec = nullptr;
    if (SUCCEEDED(fac->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand, &dec)) && dec) {
        IWICBitmapFrameDecode* frame = nullptr;
        if (SUCCEEDED(dec->GetFrame(0, &frame)) && frame) {
            IWICFormatConverter* conv = nullptr;
            if (SUCCEEDED(fac->CreateFormatConverter(&conv)) && conv) {
                if (SUCCEEDED(conv->Initialize(frame, GUID_WICPixelFormat32bppBGR,
                                               WICBitmapDitherTypeNone, nullptr, 0.0,
                                               WICBitmapPaletteTypeCustom))) {
                    UINT w = 0, h = 0;
                    conv->GetSize(&w, &h);
                    if (w > 0 && h > 0) {
                        BITMAPINFO bi{};
                        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                        bi.bmiHeader.biWidth = static_cast<LONG>(w);
                        bi.bmiHeader.biHeight = -static_cast<LONG>(h);
                        bi.bmiHeader.biPlanes = 1;
                        bi.bmiHeader.biBitCount = 32;
                        bi.bmiHeader.biCompression = BI_RGB;
                        BYTE* bits = nullptr;
                        hbmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS,
                                                reinterpret_cast<void**>(&bits), nullptr, 0);
                        if (hbmp && bits) {
                            if (FAILED(conv->CopyPixels(nullptr, w * 4, w * h * 4, bits))) {
                                DeleteObject(hbmp);
                                hbmp = nullptr;
                            } else {
                                *outW = static_cast<int>(w);
                                *outH = static_cast<int>(h);
                            }
                        }
                    }
                }
                conv->Release();
            }
            frame->Release();
        }
        dec->Release();
    }
    fac->Release();
    return hbmp;
}

void EnsureImage() {
    if (g_cfg.bgImage == g_loadedImage && g_hbmImage) return;
    if (g_hbmImage) { DeleteObject(g_hbmImage); g_hbmImage = nullptr; }
    g_loadedImage.clear();
    g_imgW = g_imgH = 0;
    if (g_cfg.bgImage.empty()) return;
    if (GetFileAttributesW(g_cfg.bgImage.c_str()) == INVALID_FILE_ATTRIBUTES) return;
    int w = 0, h = 0;
    HBITMAP bmp = DecodeImageFile(g_cfg.bgImage, &w, &h);
    if (!bmp) return;
    g_hbmImage = bmp;
    g_loadedImage = g_cfg.bgImage;
    g_imgW = w;
    g_imgH = h;
}

using GradientFill_t = BOOL(WINAPI*)(HDC, PTRIVERTEX, ULONG, PVOID, ULONG, ULONG);

void PaintBackground(HDC hdc) {
    RECT full = { 0, 0, g_w, g_h };
    const std::wstring& mode = g_cfg.backgroundMode;
    if (mode == L"Image" && g_hbmImage) {
        HBRUSH blk = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        FillRect(hdc, &full, blk);
        HDC idc = CreateCompatibleDC(hdc);
        HGDIOBJ old = SelectObject(idc, g_hbmImage);
        double sxA = static_cast<double>(g_w) / g_imgW;
        double syA = static_cast<double>(g_h) / g_imgH;
        double scale = (sxA > syA) ? sxA : syA;
        int sw = static_cast<int>(g_w / scale + 0.5);
        int sh = static_cast<int>(g_h / scale + 0.5);
        int sx = (g_imgW - sw) / 2;
        int sy = (g_imgH - sh) / 2;
        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchBlt(hdc, 0, 0, g_w, g_h, idc, sx, sy, sw, sh, SRCCOPY);
        SelectObject(idc, old);
        DeleteDC(idc);
        return;
    }
    if (mode == L"Gradient") {
        HBRUSH blk = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        FillRect(hdc, &full, blk);
        std::wstring gs = g_cfg.gradientStr;
        std::wstring clean;
        for (wchar_t c : gs) if (c != L' ' && c != L'#') clean += c;
        std::vector<COLORREF> cols;
        size_t start = 0;
        while (start <= clean.size()) {
            size_t comma = clean.find(L',', start);
            std::wstring tok = (comma == std::wstring::npos) ? clean.substr(start) : clean.substr(start, comma - start);
            if (tok.size() == 6) cols.push_back(HexToRgb(tok));
            if (comma == std::wstring::npos) break;
            start = comma + 1;
        }
        if (cols.size() < 2 && !cols.empty()) cols.push_back(cols[0]);
        if (cols.size() < 2) return;
        auto gradFill = DynFn<GradientFill_t>(L"msimg32.dll", "GradientFill");
        if (!gradFill) return;
        size_t segs = cols.size() - 1;
        for (size_t i = 0; i < segs; i++) {
            int y0 = static_cast<int>(static_cast<double>(g_h) * i / segs);
            int y1 = static_cast<int>(static_cast<double>(g_h) * (i + 1) / segs);
            if (y1 <= y0) y1 = y0 + 1;
            TRIVERTEX v[2] = {};
            v[0].x = 0; v[0].y = y0;
            v[0].Red = static_cast<USHORT>(GetRValue(cols[i]) << 8);
            v[0].Green = static_cast<USHORT>(GetGValue(cols[i]) << 8);
            v[0].Blue = static_cast<USHORT>(GetBValue(cols[i]) << 8);
            v[0].Alpha = 0xFFFF;
            v[1].x = g_w; v[1].y = y1;
            v[1].Red = static_cast<USHORT>(GetRValue(cols[i + 1]) << 8);
            v[1].Green = static_cast<USHORT>(GetGValue(cols[i + 1]) << 8);
            v[1].Blue = static_cast<USHORT>(GetBValue(cols[i + 1]) << 8);
            v[1].Alpha = 0xFFFF;
            GRADIENT_RECT gr{ 0, 1 };
            gradFill(hdc, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);
        }
        return;
    }
    HBRUSH b = CreateSolidBrush(g_bg);
    FillRect(hdc, &full, b);
    DeleteObject(b);
}

void EnsureBuffers() {
    if (g_bufW == g_w && g_bufH == g_h && g_dcOut) return;
    if (g_dcBg) { DeleteDC(g_dcBg); g_dcBg = nullptr; }
    if (g_dcFg) { DeleteDC(g_dcFg); g_dcFg = nullptr; }
    if (g_dcMask) { DeleteDC(g_dcMask); g_dcMask = nullptr; }
    if (g_dcOut) { DeleteDC(g_dcOut); g_dcOut = nullptr; }
    if (g_bmpBg) { DeleteObject(g_bmpBg); g_bmpBg = nullptr; }
    if (g_bmpFg) { DeleteObject(g_bmpFg); g_bmpFg = nullptr; }
    if (g_bmpMask) { DeleteObject(g_bmpMask); g_bmpMask = nullptr; }
    if (g_bmpOut) { DeleteObject(g_bmpOut); g_bmpOut = nullptr; }

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = g_w;
    bi.bmiHeader.biHeight = -g_h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    g_bmpBg = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_bitsBg), nullptr, 0);
    g_bmpFg = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_bitsFg), nullptr, 0);
    g_bmpMask = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_bitsMask), nullptr, 0);
    g_bmpOut = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_bitsOut), nullptr, 0);
    g_dcBg = CreateCompatibleDC(nullptr);   SelectObject(g_dcBg, g_bmpBg);
    g_dcFg = CreateCompatibleDC(nullptr);   SelectObject(g_dcFg, g_bmpFg);
    g_dcMask = CreateCompatibleDC(nullptr); SelectObject(g_dcMask, g_bmpMask);
    g_dcOut = CreateCompatibleDC(nullptr);  SelectObject(g_dcOut, g_bmpOut);
    g_bufW = g_w;
    g_bufH = g_h;
}

void DrawAll(HDC hdc, bool mask) {
    SetBkMode(hdc, TRANSPARENT);
    for (const GlyphDraw& gd : g_glyphs) {
        HFONT f = GetCachedFontPt(gd.pt, gd.weight, false, gd.family);
        HGDIOBJ of = SelectObject(hdc, f);
        SetTextColor(hdc, mask ? RGB(255, 255, 255) : gd.color);
        std::wstring s(1, gd.ch);
        RECT rc = gd.rc;
        DrawTextW(hdc, s.c_str(), 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, of);
    }
    for (const TextDraw& td : g_texts) {
        HFONT f = GetCachedFontPt(td.pt, td.weight, false, td.family);
        HGDIOBJ of = SelectObject(hdc, f);
        SetTextColor(hdc, mask ? RGB(255, 255, 255) : td.color);
        RECT rc = td.rc;
        DrawTextW(hdc, td.text.c_str(), static_cast<int>(td.text.size()), &rc,
                  td.align | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, of);
    }
}

RECT RectFor(Id id) {
    RECT r{ 0, 0, 0, 0 };
    bool first = true;
    for (const HitItem& h : g_hits) {
        if (h.id != id) continue;
        if (first) { r = h.rc; first = false; }
        else UnionRect(&r, &r, &h.rc);
    }
    return r;
}

void Compose() {
    if (!g_hBar || g_w <= 0 || g_h <= 0) return;
    EnsureBuffers();

    PaintBackground(g_dcBg);
    RECT hr{}, fr{}, pr{};
    bool hasHover = (g_hover != Id::None);
    if (hasHover) { hr = RectFor(g_hover); if (IsRectEmpty(&hr)) hasHover = false; }
    bool hasFocus = (g_barFocus && g_focusId != Id::None);
    if (hasFocus) { fr = RectFor(g_focusId); if (IsRectEmpty(&fr)) hasFocus = false; }
    bool hasPress = (g_pressed != Id::None);
    if (hasPress) { pr = RectFor(g_pressed); if (IsRectEmpty(&pr)) hasPress = false; }
    if (hasPress) {
        HBRUSH hb = CreateSolidBrush(Blend(g_bg, g_text, 0.30f));
        FillRect(g_dcBg, &pr, hb);
        DeleteObject(hb);
    } else {
        if (hasHover) {
            HBRUSH hb = CreateSolidBrush(Blend(g_bg, g_text, 0.16f));
            FillRect(g_dcBg, &hr, hb);
            DeleteObject(hb);
        }
        if (hasFocus) {
            HBRUSH hb = CreateSolidBrush(Blend(g_bg, g_text, 0.16f));
            FillRect(g_dcBg, &fr, hb);
            HBRUSH fb = CreateSolidBrush(Blend(g_bg, g_text, 0.45f));
            FrameRect(g_dcBg, &fr, fb);
            DeleteObject(hb);
            DeleteObject(fb);
        }
    }
    HBRUSH db = CreateSolidBrush(RGB(0x55, 0x55, 0x55));
    for (const RECT& d : g_dividers) FillRect(g_dcBg, &d, db);
    DeleteObject(db);

    if (g_rcVolTrack.right > g_rcVolTrack.left) {
        HBRUSH tb = CreateSolidBrush(RGB(0x33, 0x33, 0x33));
        FillRect(g_dcBg, &g_rcVolTrack, tb);
        DeleteObject(tb);
        HBRUSH fb = CreateSolidBrush(g_text);
        FillRect(g_dcBg, &g_rcVolFill, fb);
        DeleteObject(fb);
    }

    BitBlt(g_dcFg, 0, 0, g_w, g_h, g_dcBg, 0, 0, SRCCOPY);
    DrawAll(g_dcFg, false);

    RECT full = { 0, 0, g_w, g_h };
    HBRUSH blk = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    FillRect(g_dcMask, &full, blk);
    DrawAll(g_dcMask, true);

    int baseAlpha = g_acrylic ? 150 : g_bgAlpha;
    const int stride = g_w * 4;
    RECT acts[3];
    int nAct = 0;
    if (hasPress) acts[nAct++] = pr;
    else {
        if (hasHover) acts[nAct++] = hr;
        if (hasFocus) acts[nAct++] = fr;
    }
    for (int y = 0; y < g_h; y++) {
        const BYTE* mrow = g_bitsMask + y * stride;
        const BYTE* frow = g_bitsFg + y * stride;
        BYTE* orow = g_bitsOut + y * stride;
        for (int x = 0; x < g_w; x++) {
            const BYTE* m = mrow + x * 4;
            int mr = m[2], mg = m[1], mb = m[0];
            int mv = mr > mg ? (mr > mb ? mr : mb) : (mg > mb ? mg : mb);
            int a0 = baseAlpha;
            if (g_acrylic) {
                for (int i = 0; i < nAct; i++) {
                    if (x >= acts[i].left && x < acts[i].right && y >= acts[i].top && y < acts[i].bottom) {
                        a0 = 200;
                        break;
                    }
                }
            }
            int a = a0 + ((255 - a0) * mv) / 255;
            const BYTE* f = frow + x * 4;
            BYTE* o = orow + x * 4;
            o[0] = static_cast<BYTE>(f[0] * a / 255);
            o[1] = static_cast<BYTE>(f[1] * a / 255);
            o[2] = static_cast<BYTE>(f[2] * a / 255);
            o[3] = static_cast<BYTE>(a);
        }
    }

    HDC scr = GetDC(nullptr);
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    POINT ptZero = { 0, 0 };
    SIZE size = { g_w, g_h };
    UpdateLayeredWindow(g_hBar, scr, nullptr, &size, g_dcOut, &ptZero, 0, &bf, ULW_ALPHA);
    ReleaseDC(nullptr, scr);
}

void RefreshVolume() {
    int idx = 0;
    for (GlyphDraw& gd : g_glyphs) {
        if (idx == g_volIconIdx) gd.ch = VolGlyph();
        idx++;
    }
    if (g_volTextIdx >= 0 && g_volTextIdx < static_cast<int>(g_texts.size())) {
        int v = volume::GetVolume();
        g_texts[g_volTextIdx].text = std::to_wstring(v) + L"%";
    }
    int trackW = g_rcVolTrack.right - g_rcVolTrack.left;
    int fillW = trackW * volume::GetVolume() / 100;
    g_rcVolFill = { g_rcVolTrack.left, g_rcVolTrack.top, g_rcVolTrack.left + fillW, g_rcVolTrack.bottom };
}

void Layout() {
    g_glyphs.clear();
    g_texts.clear();
    g_hits.clear();
    g_dividers.clear();
    g_tips.clear();
    g_volTextIdx = -1;
    g_volIconIdx = -1;
    g_appTextIdx[0] = g_appTextIdx[1] = g_appTextIdx[2] = -1;

    const int W = SS(160);
    g_w = W;
    g_h = g_rcMon.bottom - g_rcMon.top;

    const std::wstring& mode = g_cfg.backgroundMode;
    if (mode == L"Image") EnsureImage();
    if (mode == L"Dark") {
        g_bg = RGB(0, 0, 0);
        g_text = RGB(255, 255, 255);
        g_sub = RGB(0xBB, 0xBB, 0xBB);
    } else if (mode == L"Light") {
        g_bg = RGB(0xF0, 0xF0, 0xF0);
        g_text = RGB(0, 0, 0);
        g_sub = RGB(0x55, 0x55, 0x55);
    } else {
        g_bg = (mode == L"Custom") ? HexToRgb(g_cfg.customColor) : RGB(0, 0, 0);
        if (g_cfg.forceText == L"Black") g_text = RGB(0, 0, 0);
        else if (g_cfg.forceText == L"White") g_text = RGB(255, 255, 255);
        else if (mode == L"Custom") g_text = ContrastText(g_bg);
        else g_text = RGB(255, 255, 255);
        g_sub = (g_text == RGB(255, 255, 255)) ? RGB(0xCC, 0xCC, 0xCC) : RGB(0x44, 0x44, 0x44);
    }
    g_acrylic = g_backdropOK && (mode == L"Dark" || mode == L"Light" || mode == L"Custom");
    if (mode == L"Image" || mode == L"Gradient") g_acrylic = false;

    int contentLogical = (g_cfg.volumeStyle == L"Slider") ? 643 : 653;
    int freeSpace = g_h - SS(180) - SS(contentLogical);
    int boost = 0;
    if (freeSpace > SS(40)) boost = (freeSpace - SS(20)) / 5;
    if (boost > SS(20)) boost = SS(20);

    int y = SS(10);
    g_texts.push_back({ { 0, y, W, y + SS(18) }, UserName(), 12, 400, g_text, SEGOE_SEMIBOLD, DT_CENTER });
    y += SS(20);
    g_texts.push_back({ { 0, y, W, y + SS(14) }, GetBatteryInfo(), 10, 400, g_sub, SEGOE, DT_CENTER });
    y += SS(14) + SS(8);
    y += boost;
    AddDivider(y);
    y += SS(11);

    auto AddCharm = [&](Id id, UINT code, const wchar_t* label, const wchar_t* tip) {
        RECT ri = { 0, y, W, y + SS(46) };
        g_glyphs.push_back({ ri, static_cast<wchar_t>(code), 24, 400, g_text, MDL2 });
        g_hits.push_back({ id, ri });
        y += SS(46);
        RECT rl = { 0, y, W, y + SS(14) };
        g_texts.push_back({ rl, label, 10, 400, g_text, SEGOE, DT_CENTER });
        g_hits.push_back({ id, rl });
        y += SS(14) + SS(8);
        AddTip(id, tip);
    };

    AddCharm(Id::Search, 0xE721, L"Search", L"Search System/Files");
    AddCharm(Id::Clipboard, 0xF0E3, L"Clipboard", L"Clipboard Settings");
    AddCharm(Id::Start, 0xE80F, L"Start", L"Open Start Menu");
    AddCharm(Id::Devices, 0xE772, L"Devices", L"Connected Devices");
    AddCharm(Id::SettingsCharm, 0xE713, L"Settings", L"Windows Settings");

    AddTip(Id::AppA, L"Launch " + g_cfg.appAName + L" (Shift+Click = Admin)");
    AddTip(Id::AppB, L"Launch " + g_cfg.appBName + L" (Shift+Click = Admin)");
    AddTip(Id::AppC, L"Launch " + g_cfg.appCName + L" (Shift+Click = Admin)");
    AddTip(Id::MediaPrev, L"Previous Track");
    AddTip(Id::MediaPlay, L"Play/Pause");
    AddTip(Id::MediaNext, L"Next Track");
    if (g_cfg.volumeStyle == L"Slider") {
        AddTip(Id::VolIcon, L"Toggle Mute");
        AddTip(Id::VolTrack, L"Click to Set Volume");
    } else {
        AddTip(Id::VolDown, L"Volume Down");
        AddTip(Id::VolUp, L"Volume Up");
    }
    AddTip(Id::Snip, L"Snipping Tool");
    AddTip(Id::Calc, L"Calculator");
    AddTip(Id::Task, L"Task Manager");
    AddTip(Id::Power, L"Power Options");
    if (!g_openedByHover) AddTip(Id::Gear, L"Charms Settings");

    g_navOrder.clear();
    g_navOrder = { Id::Search, Id::Clipboard, Id::Start, Id::Devices, Id::SettingsCharm,
                   Id::MediaPrev, Id::MediaPlay, Id::MediaNext };
    if (g_cfg.volumeStyle == L"Slider") {
        g_navOrder.push_back(Id::VolIcon);
    } else {
        g_navOrder.push_back(Id::VolDown);
        g_navOrder.push_back(Id::VolUp);
    }
    g_navOrder.push_back(Id::Snip);
    g_navOrder.push_back(Id::Calc);
    g_navOrder.push_back(Id::Task);
    g_navOrder.push_back(Id::AppA);
    g_navOrder.push_back(Id::AppB);
    g_navOrder.push_back(Id::AppC);
    g_navOrder.push_back(Id::Power);
    if (!g_openedByHover) g_navOrder.push_back(Id::Gear);

    y += boost;
    AddDivider(y);
    y += SS(11);

    g_texts.push_back({ { 0, y, W, y + SS(14) }, L"MEDIA", 10, 400, g_sub, SEGOE, DT_CENTER });
    y += SS(14) + SS(5);
    {
        int bw = SS(40), bh = SS(30);
        int sx = (W - bw * 3) / 2;
        const Id ids[3] = { Id::MediaPrev, Id::MediaPlay, Id::MediaNext };
        const UINT glyphs[3] = { 0xE892, 0xE768, 0xE893 };
        for (int i = 0; i < 3; i++) {
            RECT r = { sx + i * bw, y, sx + (i + 1) * bw, y + bh };
            g_glyphs.push_back({ r, static_cast<wchar_t>(glyphs[i]), 16, 400, g_text, MDL2 });
            g_hits.push_back({ ids[i], r });
        }
        y += bh;
    }

    y += SS(10);
    if (g_cfg.volumeStyle == L"Slider") {
        RECT ri = { SS(10), y, SS(10) + SS(30), y + SS(30) };
        g_glyphs.push_back({ ri, VolGlyph(), 12, 400, g_text, MDL2 });
        g_hits.push_back({ Id::VolIcon, ri });
        g_volIconIdx = static_cast<int>(g_glyphs.size()) - 1;

        g_rcVolTrack = { SS(45), y + SS(11), W - SS(40), y + SS(19) };
        g_hits.push_back({ Id::VolTrack, { SS(45), y, W - SS(40), y + SS(30) } });

        int v = volume::GetVolume();
        g_rcVolFill = { g_rcVolTrack.left, g_rcVolTrack.top,
                        g_rcVolTrack.left + (g_rcVolTrack.right - g_rcVolTrack.left) * v / 100,
                        g_rcVolTrack.bottom };
        g_texts.push_back({ { W - SS(38), y + SS(5), W - SS(3), y + SS(25) },
                            std::to_wstring(v) + L"%", 10, 400, g_text, SEGOE, DT_RIGHT });
        g_volTextIdx = static_cast<int>(g_texts.size()) - 1;
        y += SS(30);
    } else {
        RECT rd = { SS(20), y, SS(60), y + SS(40) };
        g_glyphs.push_back({ rd, static_cast<wchar_t>(0xE992), 12, 400, g_text, MDL2 });
        g_hits.push_back({ Id::VolDown, rd });
        int v = volume::GetVolume();
        g_texts.push_back({ { SS(60), y, SS(100), y + SS(40) }, std::to_wstring(v) + L"%",
                            10, 400, g_text, SEGOE, DT_CENTER });
        g_volTextIdx = static_cast<int>(g_texts.size()) - 1;
        RECT ru = { SS(100), y, SS(140), y + SS(40) };
        g_glyphs.push_back({ ru, static_cast<wchar_t>(0xE767), 12, 400, g_text, MDL2 });
        g_hits.push_back({ Id::VolUp, ru });
        g_rcVolTrack = { 0, 0, 0, 0 };
        y += SS(40);
    }

    y += boost;
    AddDivider(y);
    y += SS(11);

    g_texts.push_back({ { 0, y, W, y + SS(14) }, L"TOOLS", 10, 400, g_sub, SEGOE, DT_CENTER });
    y += SS(14) + SS(5);
    {
        int bw = SS(40), bh = SS(40);
        int sx = (W - bw * 3) / 2;
        const Id ids[3] = { Id::Snip, Id::Calc, Id::Task };
        const UINT glyphs[3] = { 0xE70F, 0xE8EF, 0xE9D9 };
        for (int i = 0; i < 3; i++) {
            RECT r = { sx + i * bw, y, sx + (i + 1) * bw, y + bh };
            g_glyphs.push_back({ r, static_cast<wchar_t>(glyphs[i]), 14, 400, g_text, MDL2 });
            g_hits.push_back({ ids[i], r });
        }
        y += bh;
    }

    y += boost;
    AddDivider(y);
    y += SS(11);

    g_texts.push_back({ { 0, y, W, y + SS(14) }, L"APPS", 10, 400, g_sub, SEGOE, DT_CENTER });
    y += SS(14) + SS(5);
    {
        int bw = SS(40), bh = SS(40);
        int sx = (W - bw * 3) / 2;
        const Id ids[3] = { Id::AppA, Id::AppB, Id::AppC };
        for (int i = 0; i < 3; i++) {
            RECT r = { sx + i * bw, y, sx + (i + 1) * bw, y + bh };
            g_texts.push_back({ r, std::wstring(1, static_cast<wchar_t>(L'A' + i)), 12, 700, g_text, SEGOE, DT_CENTER });
            g_appTextIdx[i] = static_cast<int>(g_texts.size()) - 1;
            g_hits.push_back({ ids[i], r });
        }
        y += bh;
    }

    int bottomY = g_h - SS(180);
    if (bottomY < y) bottomY = y;

    g_texts.push_back({ { 0, bottomY, W, bottomY + SS(40) }, g_timeStr, 26, 400, g_text, SEGOE_LIGHT, DT_CENTER });
    bottomY += SS(40);
    g_texts.push_back({ { 0, bottomY, W, bottomY + SS(16) }, g_ampmStr, 10, 400, g_text, SEGOE, DT_CENTER });
    bottomY += SS(16);
    g_texts.push_back({ { 0, bottomY + SS(5), W, bottomY + SS(19) }, g_dateStr, 10, 400, g_sub, SEGOE, DT_CENTER });
    bottomY += SS(19) + SS(10);

    RECT rp = { 0, bottomY, W, bottomY + SS(40) };
    g_glyphs.push_back({ rp, static_cast<wchar_t>(0xE7E8), 20, 400, g_text, MDL2 });
    g_hits.push_back({ Id::Power, rp });
    bottomY += SS(40);

    if (!g_openedByHover) {
        RECT rg = { W - SS(35), bottomY + SS(5), W - SS(5), bottomY + SS(35) };
        g_glyphs.push_back({ rg, static_cast<wchar_t>(0xE713), 12, 400, g_text, MDL2 });
        g_hits.push_back({ Id::Gear, rg });
    }

    UpdateAppColors();
}

void UpdateAppColors() {
    if (g_appTextIdx[0] >= 0) g_texts[g_appTextIdx[0]].color = Hsv(g_hue, 1.f, 1.f);
    if (g_appTextIdx[1] >= 0) g_texts[g_appTextIdx[1]].color = Hsv(fmodf(g_hue + 60.f, 360.f), 1.f, 1.f);
    if (g_appTextIdx[2] >= 0) g_texts[g_appTextIdx[2]].color = Hsv(fmodf(g_hue + 120.f, 360.f), 1.f, 1.f);
}

Id HitTest(const POINT& pt) {
    for (const HitItem& h : g_hits)
        if (PtInRect(&h.rc, pt)) return h.id;
    return Id::None;
}

std::wstring TipFor(Id id) {
    for (const TipEntry& t : g_tips)
        if (t.id == id) return t.text;
    return L"";
}

void TipHide() {
    if (g_hTip) ShowWindow(g_hTip, SW_HIDE);
    KillTimer(g_hBar, TIMER_TIP);
}

void TipShow(const std::wstring& text, const POINT& clientPt) {
    if (!g_hTip) return;
    UINT tipDpi = GetDpiForWindow(g_hTip);
    HFONT f = GetCachedFontPt(10, 400, false, SEGOE);
    HDC dc = GetDC(g_hTip);
    HGDIOBJ of = SelectObject(dc, f);
    SIZE sz{};
    GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &sz);
    SelectObject(dc, of);
    ReleaseDC(g_hTip, dc);
    int padX = MulDiv(8, static_cast<int>(tipDpi), 96);
    int padY = MulDiv(4, static_cast<int>(tipDpi), 96);
    int w = sz.cx + padX * 2;
    int h = sz.cy + padY * 2;
    POINT sp = clientPt;
    ClientToScreen(g_hBar, &sp);
    sp.x += SS(10);
    sp.y += SS(24);
    RECT mon = g_rcMon;
    if (sp.x + w > mon.right) sp.x = mon.right - w;
    if (sp.y + h > mon.bottom) sp.y = sp.y - h - SS(40);
    SetWindowPos(g_hTip, HWND_TOPMOST, sp.x, sp.y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    SetTimer(g_hBar, TIMER_TIP, 2000, nullptr);
}

LRESULT CALLBACK TipProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(h, &ps);
        RECT rc{};
        GetClientRect(h, &rc);
        HBRUSH b = CreateSolidBrush(RGB(0x2D, 0x2D, 0x2D));
        FillRect(dc, &rc, b);
        DeleteObject(b);
        HBRUSH fr = CreateSolidBrush(RGB(0x66, 0x66, 0x66));
        FrameRect(dc, &rc, fr);
        DeleteObject(fr);
        wchar_t buf[256]{};
        int n = GetWindowTextW(h, buf, 256);
        HFONT f = GetCachedFontPt(10, 400, false, SEGOE);
        HGDIOBJ of = SelectObject(dc, f);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(0xF0, 0xF0, 0xF0));
        DrawTextW(dc, buf, n, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, of);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    default:
        return DefWindowProcW(h, msg, wp, lp);
    }
}

void MoveFocus(int dir) {
    if (g_navOrder.empty()) return;
    int idx = 0;
    if (g_focusId != Id::None) {
        auto it = std::find(g_navOrder.begin(), g_navOrder.end(), g_focusId);
        if (it != g_navOrder.end()) idx = static_cast<int>(it - g_navOrder.begin());
    }
    idx = (idx + dir + static_cast<int>(g_navOrder.size())) % static_cast<int>(g_navOrder.size());
    g_focusId = g_navOrder[idx];
    Compose();
}

void TickAnim() {
    if (g_animPhase == 0) return;
    double dur = (g_animPhase == 1) ? 180.0 : 130.0;
    double t = static_cast<double>(GetTickCount64() - g_animStart) / dur;
    if (t > 1.0) t = 1.0;
    double e = (g_animPhase == 1) ? 1.0 - pow(1.0 - t, 3.0) : t * t * t;
    int x = g_animFromX + static_cast<int>((g_animToX - g_animFromX) * e);
    SetWindowPos(g_hBar, nullptr, x, g_animY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    if (t >= 1.0) {
        KillTimer(g_hBar, TIMER_ANIM);
        if (g_animPhase == 2) ShowWindow(g_hBar, SW_HIDE);
        g_animPhase = 0;
    }
}

void StartAnim(bool show) {
    KillTimer(g_hBar, TIMER_ANIM);
    g_animPhase = show ? 1 : 2;
    g_animStart = GetTickCount64();
    SetTimer(g_hBar, TIMER_ANIM, 10, nullptr);
    TickAnim();
}

void Activate(Id id, const POINT& pt);

LRESULT CALLBACK BarProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        BeginPaint(h, &ps);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE: {
        POINT pt{ X16(lp), Y16(lp) };
        Id id = HitTest(pt);
        if (id != g_hover) {
            g_hover = id;
            Compose();
            std::wstring tip = TipFor(id);
            if (!tip.empty()) TipShow(tip, pt);
            else TipHide();
        }
        if (!g_trackMouse) {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = h;
            TrackMouseEvent(&tme);
            g_trackMouse = true;
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        g_trackMouse = false;
        g_hover = Id::None;
        TipHide();
        Compose();
        return 0;
    case WM_LBUTTONDOWN:
        g_pressed = g_hover;
        SetCapture(h);
        return 0;
    case WM_LBUTTONUP: {
        ReleaseCapture();
        POINT pt{ X16(lp), Y16(lp) };
        Id id = HitTest(pt);
        bool click = (id != Id::None && id == g_pressed);
        g_pressed = Id::None;
        if (click) Activate(id, pt);
        return 0;
    }
    case WM_SETFOCUS:
        g_barFocus = true;
        Compose();
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) bar::Hide();
        else if (wp == VK_RETURN || wp == VK_SPACE) {
            if (g_focusId != Id::None) {
                RECT r = RectFor(g_focusId);
                POINT pt{ (r.left + r.right) / 2, (r.top + r.bottom) / 2 };
                Activate(g_focusId, pt);
            }
        }
        else if (wp == VK_TAB) MoveFocus((GetKeyState(VK_SHIFT) < 0) ? -1 : 1);
        else if (wp == VK_UP || wp == VK_LEFT) MoveFocus(-1);
        else if (wp == VK_DOWN || wp == VK_RIGHT) MoveFocus(1);
        return 0;
    case WM_KILLFOCUS:
        g_barFocus = false;
        g_focusId = Id::None;
        if (g_barVisible) bar::Hide();
        return 0;
    case WM_TIMER:
        if (wp == TIMER_CLOCK) {
            if (UpdateClockStrings()) Compose();
        } else if (wp == TIMER_RAINBOW) {
            g_hue = fmodf(g_hue + 4.f, 360.f);
            UpdateAppColors();
            Compose();
        } else if (wp == TIMER_TIP) {
            TipHide();
        } else if (wp == TIMER_ANIM) {
            TickAnim();
        }
        return 0;
    default:
        return DefWindowProcW(h, msg, wp, lp);
    }
}

void Activate(Id id, const POINT& pt) {
    switch (id) {
    case Id::Search:          bar::Hide(); actions::RunSearch(); break;
    case Id::Clipboard:       bar::Hide(); actions::RunClipboard(); break;
    case Id::Start:           bar::Hide(); actions::RunStart(); break;
    case Id::Devices:         bar::Hide(); actions::RunDevices(); break;
    case Id::SettingsCharm:   bar::Hide(); actions::RunSettings(); break;
    case Id::MediaPrev:       actions::MediaPrev(); break;
    case Id::MediaPlay:       actions::MediaPlayPause(); break;
    case Id::MediaNext:       actions::MediaNext(); break;
    case Id::VolIcon:         volume::ToggleMute(); RefreshVolume(); Compose(); break;
    case Id::VolDown:         volume::SetVolume(volume::GetVolume() - 5); RefreshVolume(); Compose(); break;
    case Id::VolUp:           volume::SetVolume(volume::GetVolume() + 5); RefreshVolume(); Compose(); break;
    case Id::VolTrack: {
        int tw = g_rcVolTrack.right - g_rcVolTrack.left;
        if (tw > 0) {
            int v = (pt.x - g_rcVolTrack.left) * 100 / tw;
            volume::SetVolume(v);
            RefreshVolume();
            Compose();
        }
        break;
    }
    case Id::Snip:            bar::Hide(); actions::RunSnip(); break;
    case Id::Calc:            bar::Hide(); actions::RunCalc(); break;
    case Id::Task:            bar::Hide(); actions::RunTaskMgr(); break;
    case Id::AppA:            bar::Hide(); actions::RunApp(g_cfg.appAPath, GetKeyState(VK_SHIFT) < 0); break;
    case Id::AppB:            bar::Hide(); actions::RunApp(g_cfg.appBPath, GetKeyState(VK_SHIFT) < 0); break;
    case Id::AppC:            bar::Hide(); actions::RunApp(g_cfg.appCPath, GetKeyState(VK_SHIFT) < 0); break;
    case Id::Power:           actions::ShowPowerMenu(); break;
    case Id::Gear:            bar::Hide(); settings::Open(); break;
    default: break;
    }
}

void ApplyDwm() {
    if (!IsWin11()) return;
    int dark = 1;
    DwmSetWindowAttribute(g_hBar, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    UINT pref = DWMWCP_ROUNDSMALL;
    DwmSetWindowAttribute(g_hBar, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
    int backdrop = DWMSBT_TRANSIENTWINDOW;
    HRESULT hr = DwmSetWindowAttribute(g_hBar, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
    if (SUCCEEDED(hr)) {
        MARGINS mg;
        mg.cxLeftWidth = -1; mg.cxRightWidth = -1; mg.cyTopHeight = -1; mg.cyBottomHeight = -1;
        DwmExtendFrameIntoClientArea(g_hBar, &mg);
        g_backdropOK = true;
    }
}

}

namespace bar {

void CreateOnce() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = BarProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = L"CharmsBarNative";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    WNDCLASSW wt{};
    wt.lpfnWndProc = TipProc;
    wt.hInstance = g_hInst;
    wt.lpszClassName = L"CharmsBarNativeTip";
    wt.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wt);

    g_hBar = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                             wc.lpszClassName, L"CharmsBar", WS_POPUP,
                             0, 0, 0, 0, nullptr, nullptr, g_hInst, nullptr);
    g_hTip = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                             wt.lpszClassName, L"", WS_POPUP,
                             0, 0, 0, 0, nullptr, nullptr, g_hInst, nullptr);
    ApplyDwm();
}

void Show(bool fromHover) {
    if (!g_hBar) return;
    if (g_animPhase != 0) {
        KillTimer(g_hBar, TIMER_ANIM);
        g_animPhase = 0;
        ShowWindow(g_hBar, SW_HIDE);
    }
    POINT pt{};
    GetCursorPos(&pt);
    MonitorUnderPoint(pt, &g_rcMon);
    g_openedByHover = fromHover;
    g_hover = Id::None;
    g_pressed = Id::None;
    g_focusId = Id::None;
    UpdateClockStrings();
    SetWindowPos(g_hBar, HWND_TOPMOST, g_rcMon.right - 8, g_rcMon.top, 8, 8, SWP_NOACTIVATE | SWP_HIDEWINDOW);
    g_dpi = GetDpiForWindow(g_hBar);
    if (g_dpi == 0) g_dpi = 96;
    Layout();
    g_animY = g_rcMon.top;
    g_animFromX = g_rcMon.right;
    g_animToX = g_rcMon.right - g_w;
    SetWindowPos(g_hBar, HWND_TOPMOST, g_animFromX, g_animY, g_w, g_h, SWP_NOACTIVATE);
    Compose();
    ShowWindow(g_hBar, SW_SHOWNOACTIVATE);
    StartAnim(true);
    if (!fromHover) {
        if (!g_navOrder.empty()) g_focusId = g_navOrder[0];
        SetForegroundWindow(g_hBar);
        SetFocus(g_hBar);
    }
    SetTimer(g_hBar, TIMER_CLOCK, 1000, nullptr);
    SetTimer(g_hBar, TIMER_RAINBOW, 50, nullptr);
    g_barVisible = true;
}

void Hide() {
    if (!g_barVisible) return;
    g_barVisible = false;
    KillTimer(g_hBar, TIMER_CLOCK);
    KillTimer(g_hBar, TIMER_RAINBOW);
    TipHide();
    g_hover = Id::None;
    g_pressed = Id::None;
    g_focusId = Id::None;
    g_barFocus = false;
    g_trackMouse = false;
    RECT rc{};
    GetWindowRect(g_hBar, &rc);
    g_animFromX = rc.left;
    g_animY = rc.top;
    g_animToX = g_rcMon.right;
    StartAnim(false);
}

void Toggle(bool fromHover) {
    if (g_barVisible) {
        if (!fromHover && g_openedByHover) {
            g_openedByHover = false;
            Layout();
            if (!g_navOrder.empty()) g_focusId = g_navOrder[0];
            SetForegroundWindow(g_hBar);
            SetFocus(g_hBar);
            Compose();
            return;
        }
        Hide();
        return;
    }
    Show(fromHover);
}

void OnConfigChanged() {
    if (!g_barVisible) return;
    Layout();
    Compose();
}

bool Visible() {
    return g_barVisible;
}

}
