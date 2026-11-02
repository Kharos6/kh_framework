#pragma once

// The injection half (rendering_integration_injection.hpp): the main depth's identity: its dimensions, MSAA, the DSV
// identity and the readopt. Never include it on its own.
namespace RenderIntegration {

// The adopted main depth's dimensions against a view's resource (the clear hook's once-per-frame confirmation of the
// pointer identity, and kh_fx_late_gt's check of the bound depth). True when they agree or the resource cannot be
// read (a read failure is not evidence).
inline bool kh_main_depth_dims_match(ID3D11DepthStencilView* dsv) {
    ID3D11Resource* khdm_res = nullptr;
    dsv->GetResource(&khdm_res);
    if (!khdm_res) return true;
    ID3D11Texture2D* khdm_tex = nullptr;
    bool khdm_ok = true;
    if (SUCCEEDED(khdm_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khdm_tex))) && khdm_tex) {
        D3D11_TEXTURE2D_DESC khdm_td = {};
        khdm_tex->GetDesc(&khdm_td);
        khdm_ok = khdm_td.Width == g_main_depth_w && khdm_td.Height == g_main_depth_h;
        khdm_tex->Release();
    }
    khdm_res->Release();
    return khdm_ok;
}
// KH_FSAA_SWITCH - a depth view over a multisampled texture of the adopted main depth's size.
inline bool kh_depth_msaa_scene(ID3D11DepthStencilView* kh1m_dsv) {
    ID3D11Resource* kh1m_res = nullptr;
    kh1m_dsv->GetResource(&kh1m_res);
    if (!kh1m_res) return false;
    ID3D11Texture2D* kh1m_tex = nullptr;
    bool kh1m_ms = false;
    const HRESULT kh1m_hr = kh1m_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&kh1m_tex));
    if (SUCCEEDED(kh1m_hr) && kh1m_tex) {
        D3D11_TEXTURE2D_DESC kh1m_td = {};
        kh1m_tex->GetDesc(&kh1m_td);
        kh1m_ms = kh1m_td.SampleDesc.Count > 1 && kh1m_td.Width == g_main_depth_w && kh1m_td.Height == g_main_depth_h;
        kh1m_tex->Release();
    }
    kh1m_res->Release();
    return kh1m_ms;
}

inline void* reorder_dsv_identity(ID3D11DepthStencilView* dsv) {
    void* id = nullptr;
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);

    if (res) {
        ID3D11Texture2D* tex = nullptr;

        if (SUCCEEDED(res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex))) && tex) {
            id = static_cast<void*>(tex);
            tex->Release();
        }

        res->Release();
    }

    return id;
}
// KH_DEPTH_READOPT - after a resolution or FSAA change the main depth identity goes stale, and a watchdog park
// re-adopting wherever the render thread happens to be can adopt the wrong depth. So the render thread re-adopts at its
// own scene resolve from a fingerprint no other pass has - a multisampled colour resolve with a same-size,
// same-sample-count depth view bound - and names the scene texture there. It replaces an identity only after
// KH_DEPTH_READOPT_RUN consecutive disagreeing resolves (at once when none exists). FSAA 1x has no scene resolve; the
// watchdog park remains the path there (a same-size stale identity: KH_WATCH_STALL).
// KH_SCENE_NAME: a resolve that agrees names the scene texture too - at once when none is named, else only when the
// named one has not been seen (named, or resolved here) for KH_DEPTH_READOPT_RUN cycles: the scene is resolved every
// cycle, so a name that live is never replaced, and one that is not keeps the render-thread flush off (every Draw3D
// parks) until it is.
static constexpr uint32_t KH_DEPTH_READOPT_RUN = 8u;
static uint32_t g_depth_readopt_run = 0;   // Render thread only.
inline void kh_main_depth_readopt(ID3D11DeviceContext* khdr_ctx, ID3D11Resource* khdr_src) {
    // KH_SCENE_NAME: the named scene texture resolved on the render thread is seen, whatever is bound.
    if (static_cast<void*>(khdr_src) == g_topo_scene_tex_id) g_topo_scene_seen_cycle = g_topo_cycles;
    const void* const khdr_cur = g_main_depth_identity.load(std::memory_order_relaxed);
    ID3D11Texture2D* khdr_st = nullptr;
    if (FAILED(khdr_src->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khdr_st))) || !khdr_st) return;
    D3D11_TEXTURE2D_DESC khdr_sd = {};
    khdr_st->GetDesc(&khdr_sd);
    khdr_st->Release();
    if (khdr_sd.SampleDesc.Count < 2) return;
    ID3D11DepthStencilView* khdr_dsv = nullptr;
    khdr_ctx->OMGetRenderTargets(0, nullptr, &khdr_dsv);
    if (!khdr_dsv) return;
    const void* const khdr_id = reorder_dsv_identity(khdr_dsv);
    ID3D11Resource* khdr_dr = nullptr;
    khdr_dsv->GetResource(&khdr_dr);
    khdr_dsv->Release();
    if (!khdr_dr) return;
    ID3D11Texture2D* khdr_dt = nullptr;
    const HRESULT khdr_hr = khdr_dr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khdr_dt));
    khdr_dr->Release();
    if (FAILED(khdr_hr) || !khdr_dt) return;
    D3D11_TEXTURE2D_DESC khdr_dd = {};
    khdr_dt->GetDesc(&khdr_dd);
    khdr_dt->Release();
    if (khdr_dd.Width != khdr_sd.Width || khdr_dd.Height != khdr_sd.Height ||
        khdr_dd.SampleDesc.Count != khdr_sd.SampleDesc.Count || !khdr_id) return;
    if (khdr_id == khdr_cur) {   // The scene resolve agrees.
        g_depth_readopt_run = 0;
        if (static_cast<void*>(khdr_src) != g_topo_scene_tex_id &&
            (!g_topo_scene_tex_id || g_topo_cycles - g_topo_scene_seen_cycle >= KH_DEPTH_READOPT_RUN)) {
            kh_scene_tex_name(khdr_src);   // KH_SCENE_NAME.
        }
        return;
    }
    if (khdr_cur != nullptr && ++g_depth_readopt_run < KH_DEPTH_READOPT_RUN) return;
    g_depth_readopt_run = 0;
    g_main_depth_w = khdr_dd.Width;
    g_main_depth_h = khdr_dd.Height;
    g_wrong_pass_streak = 0;
    kh_scene_tex_name(khdr_src);   // KH_SCENE_NAME.
    g_main_depth_identity.store(const_cast<void*>(khdr_id), std::memory_order_relaxed);
}
}