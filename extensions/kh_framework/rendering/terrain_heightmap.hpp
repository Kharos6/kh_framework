#pragma once

// The feature half (rendering_integration_features.hpp): the terrain heightmap (THM): its lanes and upload. Never
// include it on its own.
namespace RenderIntegration {

// Heightfield GPU upload at the flush once the CPU grid is complete; one
// immutable texture per ingest. thm_upload owns the flag.
inline std::string thm_upload_body(ID3D11Device* dev) {

    KH_SAFE_RELEASE(g_res.thm_srv);
    KH_SAFE_RELEASE(g_res.thm_tex);
    g_thm.valid = false;
    // The burial snapshot certifies THIS texture, so it dies with it here, not by luck through the early returns below.
    g_voc_thm.valid = false;

    if (g_thm.w == 0 || g_thm.h == 0 ||
        g_thm.data.size() < static_cast<size_t>(g_thm.w) * g_thm.h) {
        return "heightfield incomplete";
    }
    // Caller: flush_locked (a park or the render thread), so the live meta, read only by the render thread's drawers,
    // is published with plain stores (never beside one of them).

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = g_thm.w;
    td.Height = g_thm.h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = g_thm.data.data();
    init.SysMemPitch = g_thm.w * sizeof(float);
    HRESULT hr = dev->CreateTexture2D(&td, &init, &g_res.thm_tex);
    if (FAILED(hr)) return "create heightfield tex " + hr_str(hr);
    hr = dev->CreateShaderResourceView(g_res.thm_tex, nullptr, &g_res.thm_srv);

    if (FAILED(hr)) {
        KH_SAFE_RELEASE(g_res.thm_tex);
        return "create heightfield SRV " + hr_str(hr);
    }

    g_thml.origin[0] = g_thm.origin[0];
    g_thml.origin[1] = g_thm.origin[1];
    g_thml.cell = g_thm.cell;
    g_thml.split = g_thm.split;   // THM_EXACT.
    g_thml.w = g_thm.w;
    g_thml.h = g_thm.h;
    // The nodes and their coarse max, of exactly the grid just uploaded (this body runs on the render thread or under
    // the park, never beside the culler's reader; the autobuild's staging reaches it through the g_thm.dirty hand-off,
    // thm_upload). A failure leaves the snapshot invalid = refuse every occluder.
    try {
        const UINT cw = (g_thm.w + VOC_THM_DS - 1) / VOC_THM_DS;
        const UINT ch = (g_thm.h + VOC_THM_DS - 1) / VOC_THM_DS;
        g_voc_thm.max_grid.assign(static_cast<size_t>(cw) * ch, -1.0e6f);
        for (UINT y = 0; y < g_thm.h; ++y) {
            const float* src = g_thm.data.data() +
                               static_cast<size_t>(y) * g_thm.w;
            float* dst = g_voc_thm.max_grid.data() +
                         static_cast<size_t>(y / VOC_THM_DS) * cw;
            for (UINT x = 0; x < g_thm.w; ++x) {
                float& s = dst[x / VOC_THM_DS];
                if (src[x] > s) s = src[x];
            }
        }
        g_voc_thm.fine.assign(g_thm.data.begin(),
                              g_thm.data.begin() + static_cast<std::ptrdiff_t>(static_cast<size_t>(g_thm.w) * g_thm.h));
        g_voc_thm.w = cw;
        g_voc_thm.fw = g_thm.w;
        g_voc_thm.fh = g_thm.h;
        g_voc_thm.origin[0] = g_thm.origin[0];
        g_voc_thm.origin[1] = g_thm.origin[1];
        g_voc_thm.cell = g_thm.cell;
        g_voc_thm.valid = true;
    } catch (...) {
        g_voc_thm.valid = false;
    }
    g_thm.valid = true;
    return "";
}

// NO_PARK_THM - the flag is cleared after the body's last read of the
// staging grid (the autobuild resumes writing the moment it drops).
inline std::string thm_upload(ID3D11Device* dev) {
    // Acquire / release: the flag hands the staging grid over and back (the autobuild writes it before the flag's
    // raise and again once it sees it drop).
    if (!g_thm.dirty.load(std::memory_order_acquire)) return "";
    std::string err = thm_upload_body(dev);
    g_thm.dirty.store(false, std::memory_order_release);
    return err;
}

// THM_DEVRESET - Resources::release (a device reset, a failed first build, a teardown), as it frees thm_tex /
// thm_srv: the live flags go with the texture (every reader takes its no-heightfield path; the burial snapshot
// certified that texture), and a live one asks for a re-upload of the retained staging grid (g_thm.reupload).
inline void thm_device_lost() {
    if (g_thm.valid.exchange(false, std::memory_order_relaxed)) {
        g_thm.reupload.store(true, std::memory_order_relaxed);
    }
    g_voc_thm.valid = false;
}
}