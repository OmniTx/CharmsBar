#include "app.h"
#include <objbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

namespace {

IAudioEndpointVolume* g_epv = nullptr;
bool g_comInit = false;
bool g_failed = false;

bool Ensure() {
    if (g_epv) return true;
    if (g_failed) return false;

    auto coInit = DynFn<decltype(&CoInitializeEx)>(L"ole32.dll", "CoInitializeEx");
    auto coCreate = DynFn<decltype(&CoCreateInstance)>(L"ole32.dll", "CoCreateInstance");
    if (!coInit || !coCreate) { g_failed = true; return false; }

    if (!g_comInit) {
        HRESULT hrInit = coInit(nullptr, COINIT_APARTMENTTHREADED);
        g_comInit = SUCCEEDED(hrInit) || hrInit == RPC_E_CHANGED_MODE;
    }

    IMMDeviceEnumerator* de = nullptr;
    HRESULT hr = coCreate(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&de));
    if (FAILED(hr) || !de) { g_failed = true; return false; }

    IMMDevice* dev = nullptr;
    hr = de->GetDefaultAudioEndpoint(eRender, eMultimedia, &dev);
    de->Release();
    if (FAILED(hr) || !dev) { g_failed = true; return false; }

    hr = dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                       reinterpret_cast<void**>(&g_epv));
    dev->Release();
    if (FAILED(hr) || !g_epv) { g_epv = nullptr; g_failed = true; return false; }
    return true;
}

}

namespace volume {

bool Init() { return Ensure(); }
bool Available() { return Ensure(); }

int GetVolume() {
    float v = 0.f;
    if (Ensure()) g_epv->GetMasterVolumeLevelScalar(&v);
    return static_cast<int>(v * 100.f + 0.5f);
}

bool GetMute() {
    BOOL m = FALSE;
    if (Ensure()) g_epv->GetMute(&m);
    return m != FALSE;
}

void SetVolume(int v) {
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    if (Ensure()) {
        if (GetMute() && v > 0) g_epv->SetMute(FALSE, nullptr);
        g_epv->SetMasterVolumeLevelScalar(v / 100.f, nullptr);
    }
}

void ToggleMute() {
    if (Ensure()) {
        BOOL m = FALSE;
        g_epv->GetMute(&m);
        g_epv->SetMute(!m, nullptr);
    }
}

}
