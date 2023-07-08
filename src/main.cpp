#include <Windows.h>
#include <memory>
#include <kthook/kthook.hpp>

typedef struct { float left, top, right, bottom; } CRect; 
typedef struct { unsigned char r, g, b, a; } CRGBA;


void memory_fill(void* addr, const int value, const int size) {
    DWORD newProtect;
    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &newProtect);
    memset(addr, value, size);
    VirtualProtect(addr, size, newProtect, &newProtect);
}

template<typename T>
T read_memory(void* addr) {
    DWORD newProtect;
    VirtualProtect(addr, sizeof(T), PAGE_EXECUTE_READWRITE, &newProtect);
    T value = *reinterpret_cast<T*>(addr);
    VirtualProtect(addr, sizeof(T), newProtect, &newProtect);
    return value;
}

using CTimerUpdateSignature = void(__cdecl*)();
using CRadarSpriteDrawSignature = void(__fastcall*)(void*, void*, CRect*, CRGBA*);

kthook::kthook_simple<CTimerUpdateSignature> CTimerHook{};
kthook::kthook_simple<CRadarSpriteDrawSignature> CRadarHook{};

int radardiscAddr[]{0x58A8C2, 0x58A96C, 0x58AA1A};

void CRadar2d__DrawSprite(const decltype(CRadarHook)& hook, void* this_, void* EDX, CRect* rect, CRGBA* color){
    rect->right += (rect->right - rect->left);
    rect->top += (rect->top - rect->bottom);
     
    color->r = 255;
    color->g = 255;
    color->b = 255;
    color->a = 255;
    hook.get_trampoline()(this_, EDX, rect, color);
}

void CTimer__Update(const decltype(CTimerHook)& hook){
    static bool init{};
    if (!init){

        for (int i : radardiscAddr){ memory_fill((void*)(i), 0x90, 16); } // nop radardisc render

        CRadarHook.set_dest(0x58A823);
        CRadarHook.set_cb(&CRadar2d__DrawSprite);
        CRadarHook.install();

        init = {true};
    }

    hook.get_trampoline()();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CTimerHook.set_dest(0x561B10);
        CTimerHook.set_cb(&CTimer__Update);
        CTimerHook.install();
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}