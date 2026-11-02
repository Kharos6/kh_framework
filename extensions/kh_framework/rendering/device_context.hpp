#pragma once

// The injection half (rendering_integration_injection.hpp): the immediate context's ID3D11DeviceContext1 cache
// (kh_ctx1). Never include it on its own.
namespace RenderIntegration {

// Cached ID3D11DeviceContext1 for the immediate context. Callers (render thread, or the game thread in a park, Present
// or a game-thread resolve) never overlap.
static ID3D11DeviceContext1* g_kh_ctx1 = nullptr;
static void*                 g_kh_ctx1_for = nullptr;

// Unlocked cache: its callers are serialized by the park invariant. A third
// thread (a shader worker, for one) must not call this.
inline ID3D11DeviceContext1* kh_ctx1(ID3D11DeviceContext* ctx) {
    if (g_kh_ctx1 && g_kh_ctx1_for == static_cast<void*>(ctx)) return g_kh_ctx1;

    if (g_kh_ctx1) {
        g_kh_ctx1->Release();
        g_kh_ctx1 = nullptr;
    }

    ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_kh_ctx1));
    g_kh_ctx1_for = g_kh_ctx1 ? static_cast<void*>(ctx) : nullptr;
    // A deferred context whose driver reports no native command lists is the emulated runtime the offset-only bind can
    // fail on. Immediate contexts latch false. Once per context.
    g_kh_ctx1_null_first = false;

    if (g_kh_ctx1 && g_kh_ctx1->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED) {
        ID3D11Device* khc1_dev = nullptr;
        g_kh_ctx1->GetDevice(&khc1_dev);

        if (khc1_dev) {
            D3D11_FEATURE_DATA_THREADING khc1_thr = {};
            if (SUCCEEDED(khc1_dev->CheckFeatureSupport(D3D11_FEATURE_THREADING, &khc1_thr, sizeof(khc1_thr))) &&
                !khc1_thr.DriverCommandLists) g_kh_ctx1_null_first = true;
            khc1_dev->Release();
        }
    }
    return g_kh_ctx1;
}

inline void kh_ctx1_release() {
    if (g_kh_ctx1) {
        g_kh_ctx1->Release();
        g_kh_ctx1 = nullptr;
    }

    g_kh_ctx1_for = nullptr;
    g_kh_ctx1_null_first = false;
}
}