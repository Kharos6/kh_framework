#pragma once

// The injection half (rendering_integration_injection.hpp): the main depth's identity: its dimensions, MSAA, the DSV
// identity and the readopt. Never include it on its own.
namespace RenderIntegration {

// The adopted main depth's dimensions against a view's resource (the clear hook's once-per-frame confirmation of the
// pointer identity, and fx_late_gt's check of the bound depth). True when they agree or the resource cannot be
// read (a read failure is not evidence).
inline bool main_depth_dims_match(ID3D11DepthStencilView* dsv) {
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    if (!res) return true;
    ID3D11Texture2D* tex = nullptr;
    bool ok = true;
    if (SUCCEEDED(res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex))) && tex) {
        D3D11_TEXTURE2D_DESC td = {};
        tex->GetDesc(&td);
        ok = td.Width == g_main_depth_w && td.Height == g_main_depth_h;
        tex->Release();
    }
    res->Release();
    return ok;
}
// FSAA_SWITCH - a depth view over a multisampled texture of the adopted main depth's size.
inline bool depth_msaa_scene(ID3D11DepthStencilView* dsv) {
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    if (!res) return false;
    ID3D11Texture2D* tex = nullptr;
    bool ms = false;
    const HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    if (SUCCEEDED(hr) && tex) {
        D3D11_TEXTURE2D_DESC td = {};
        tex->GetDesc(&td);
        ms = td.SampleDesc.Count > 1 && td.Width == g_main_depth_w && td.Height == g_main_depth_h;
        tex->Release();
    }
    res->Release();
    return ms;
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
// DEPTH_READOPT - after a resolution or FSAA change the main depth identity goes stale, and a watchdog park
// re-adopting wherever the render thread happens to be can adopt the wrong depth. So the render thread re-adopts at its
// own scene resolve from a fingerprint no other pass has - a multisampled colour resolve with a same-size,
// same-sample-count depth view bound - and names the scene texture there. It replaces an identity only after
// DEPTH_READOPT_RUN consecutive disagreeing resolves (at once when none exists). FSAA 1x has no scene resolve; the
// watchdog park remains the path there (a same-size stale identity: WATCH_STALL).
// SCENE_NAME: a resolve that agrees names the scene texture too - at once when none is named, else only when the
// named one has not been seen (named, or resolved here) for DEPTH_READOPT_RUN cycles: the scene is resolved every
// cycle, so a name that live is never replaced, and one that is not keeps the render-thread flush off (every Draw3D
// parks) until it is.
static constexpr uint32_t DEPTH_READOPT_RUN = 8u;
static uint32_t g_depth_readopt_run = 0;   // Render thread only.
inline void main_depth_readopt(ID3D11DeviceContext* ctx, ID3D11Resource* src) {
    // SCENE_NAME: the named scene texture resolved on the render thread is seen, whatever is bound.
    if (static_cast<void*>(src) == g_topo_scene_tex_id) g_topo_scene_seen_cycle = g_topo_cycles;
    const void* const cur = g_main_depth_identity.load(std::memory_order_relaxed);
    ID3D11Texture2D* st = nullptr;
    if (FAILED(src->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&st))) || !st) return;
    D3D11_TEXTURE2D_DESC sd = {};
    st->GetDesc(&sd);
    st->Release();
    if (sd.SampleDesc.Count < 2) return;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &dsv);
    if (!dsv) return;
    const void* const id = reorder_dsv_identity(dsv);
    ID3D11Resource* dr = nullptr;
    dsv->GetResource(&dr);
    dsv->Release();
    if (!dr) return;
    ID3D11Texture2D* dt = nullptr;
    const HRESULT hr = dr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&dt));
    dr->Release();
    if (FAILED(hr) || !dt) return;
    D3D11_TEXTURE2D_DESC dd = {};
    dt->GetDesc(&dd);
    dt->Release();
    if (dd.Width != sd.Width || dd.Height != sd.Height ||
        dd.SampleDesc.Count != sd.SampleDesc.Count || !id) return;
    if (id == cur) {   // The scene resolve agrees.
        g_depth_readopt_run = 0;
        if (static_cast<void*>(src) != g_topo_scene_tex_id &&
            (!g_topo_scene_tex_id || g_topo_cycles - g_topo_scene_seen_cycle >= DEPTH_READOPT_RUN)) {
            scene_tex_name(src);   // SCENE_NAME.
        }
        return;
    }
    if (cur != nullptr && ++g_depth_readopt_run < DEPTH_READOPT_RUN) return;
    g_depth_readopt_run = 0;
    g_main_depth_w = dd.Width;
    g_main_depth_h = dd.Height;
    g_wrong_pass_streak = 0;
    scene_tex_name(src);   // SCENE_NAME.
    g_main_depth_identity.store(const_cast<void*>(id), std::memory_order_relaxed);
}
}