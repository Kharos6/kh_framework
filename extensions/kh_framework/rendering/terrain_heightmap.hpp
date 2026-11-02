#pragma once

// The feature half (rendering_integration_features.hpp): the terrain heightmap (KH_THM): its lanes and upload. Never
// include it on its own.
namespace RenderIntegration {

// Heightfield GPU upload at the flush once the CPU grid is complete; one
// immutable texture per ingest. kh_thm_upload owns the flag.
inline std::string kh_thm_upload_body(ID3D11Device* dev) {

    KH_SAFE_RELEASE(g_res.thm_srv);
    KH_SAFE_RELEASE(g_res.thm_tex);
    g_thm_valid = false;
    // The burial snapshot certifies THIS texture, so it dies with it here, not by luck through the early returns below.
    g_voc_thm_valid = false;

    if (g_thm_w == 0 || g_thm_h == 0 ||
        g_thm_data.size() < static_cast<size_t>(g_thm_w) * g_thm_h) {
        return "heightfield incomplete";
    }
    // Caller: flush_locked (a park or the render thread), so the live meta, read only by the render thread's drawers,
    // is published with plain stores (never beside one of them).

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = g_thm_w;
    td.Height = g_thm_h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = g_thm_data.data();
    init.SysMemPitch = g_thm_w * sizeof(float);
    HRESULT hr = dev->CreateTexture2D(&td, &init, &g_res.thm_tex);
    if (FAILED(hr)) return "create heightfield tex " + hr_str(hr);
    hr = dev->CreateShaderResourceView(g_res.thm_tex, nullptr, &g_res.thm_srv);

    if (FAILED(hr)) {
        KH_SAFE_RELEASE(g_res.thm_tex);
        return "create heightfield SRV " + hr_str(hr);
    }

    g_thml_origin[0] = g_thm_origin[0];
    g_thml_origin[1] = g_thm_origin[1];
    g_thml_cell = g_thm_cell;
    g_thml_split = g_thm_split;   // KH_THM_EXACT.
    g_thml_w = g_thm_w;
    g_thml_h = g_thm_h;
    // The nodes and their coarse max, of exactly the grid just uploaded (this body runs on the render thread or under
    // the park, never beside the culler's reader; the autobuild's staging reaches it through the g_thm_dirty hand-off,
    // kh_thm_upload). A failure leaves the snapshot invalid = refuse every occluder.
    try {
        const UINT khvb_cw = (g_thm_w + KH_VOC_THM_DS - 1) / KH_VOC_THM_DS;
        const UINT khvb_ch = (g_thm_h + KH_VOC_THM_DS - 1) / KH_VOC_THM_DS;
        g_voc_thm_max.assign(static_cast<size_t>(khvb_cw) * khvb_ch, -1.0e6f);
        for (UINT khvb_y = 0; khvb_y < g_thm_h; ++khvb_y) {
            const float* khvb_src = g_thm_data.data() +
                                    static_cast<size_t>(khvb_y) * g_thm_w;
            float* khvb_dst = g_voc_thm_max.data() +
                              static_cast<size_t>(khvb_y / KH_VOC_THM_DS) * khvb_cw;
            for (UINT khvb_x = 0; khvb_x < g_thm_w; ++khvb_x) {
                float& khvb_s = khvb_dst[khvb_x / KH_VOC_THM_DS];
                if (khvb_src[khvb_x] > khvb_s) khvb_s = khvb_src[khvb_x];
            }
        }
        g_voc_thm_fine.assign(g_thm_data.begin(),
                              g_thm_data.begin() + static_cast<std::ptrdiff_t>(static_cast<size_t>(g_thm_w) * g_thm_h));
        g_voc_thm_w = khvb_cw;
        g_voc_thm_fw = g_thm_w;
        g_voc_thm_fh = g_thm_h;
        g_voc_thm_origin[0] = g_thm_origin[0];
        g_voc_thm_origin[1] = g_thm_origin[1];
        g_voc_thm_cell = g_thm_cell;
        g_voc_thm_valid = true;
    } catch (...) {
        g_voc_thm_valid = false;
    }
    g_thm_valid = true;
    return "";
}

// KH_NO_PARK_THM - the flag is cleared after the body's last read of the
// staging grid (the autobuild resumes writing the moment it drops).
inline std::string kh_thm_upload(ID3D11Device* dev) {
    // Acquire / release: the flag hands the staging grid over and back (the autobuild writes it before the flag's
    // raise and again once it sees it drop).
    if (!g_thm_dirty.load(std::memory_order_acquire)) return "";
    std::string khtu_err = kh_thm_upload_body(dev);
    g_thm_dirty.store(false, std::memory_order_release);
    return khtu_err;
}

// KH_THM_DEVRESET - Resources::release (a device reset, a failed first build, a teardown), as it frees thm_tex /
// thm_srv: the live flags go with the texture (every reader takes its no-heightfield path; the burial snapshot
// certified that texture), and a live one asks for a re-upload of the retained staging grid (g_thm_reupload).
inline void kh_thm_device_lost() {
    if (g_thm_valid.exchange(false, std::memory_order_relaxed)) {
        g_thm_reupload.store(true, std::memory_order_relaxed);
    }
    g_voc_thm_valid = false;
}
}