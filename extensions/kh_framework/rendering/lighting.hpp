#pragma once

// The feature half (rendering_integration_features.hpp): the lighting constants: the dynamic lights' selection, packing
// and ring, the frame and object lighting fills. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/dls_select.hpp (declared here for the uses below).
inline int kh_dls_slot_of(const DlPoolLight& khs_l);


// Dynamic lights are baseline: no mode, no off switch. The pool is merged in absolute world space (dlCtl.x = 3 in the
// shader). g_dl_intensity_bits scales the whole sum (dlGlobal.w); no command sets it, so it holds 1.0, which the
// session resets restore.
static std::atomic<uint32_t> g_dl_intensity_bits{0x3F800000u};   // Float bits, default 1.0.

// KH_DL_RING - the per-draw light records, uncapped. A dynamic structured buffer of float4 (t40, HLSL khDlRecs) each
// lit draw appends its selection to, in the CB ring's shape (kh_upload_obj_cb): NO_OVERWRITE into space no append since
// the last DISCARD has used, DISCARD at the wrap. A device without NO_OVERWRITE on dynamic SRV buffers DISCARDs every
// append and writes at 0. The buffer doubles whenever one draw's records do not fit, so no count is ever refused. A
// draw reads its records at dl_first.x; the region stays valid until the next append that DISCARDs, and every fill site
// draws its object before it fills the next one - the invariant that makes a DISCARD safe.
// Render thread, or the game thread under a park (the fill sites' rule) - or inside KH_FX_LATE's chain-only flush,
// which reaches it through the dynamicLightFog block on the game thread under kh_dlf_select's premise (the engine
// never drives its context from both threads at once).
// Device objects: released by release_shadow_device_state (kh_dlr_release).
static ID3D11Buffer*             g_dlr_buf = nullptr;
static ID3D11ShaderResourceView* g_dlr_srv = nullptr;
static uint32_t g_dlr_cap = 0;            // Capacity, float4 elements.
static uint32_t g_dlr_cursor = 0;         // Next free element since the last DISCARD.
static bool     g_dlr_fresh = true;       // The next map DISCARDs.
static int      g_dlr_nooverwrite = -1;   // Device cap: -1 unknown, 0 no, 1 yes.

inline void kh_dlr_release() {
    KH_SAFE_RELEASE(g_dlr_srv);
    KH_SAFE_RELEASE(g_dlr_buf);
    g_dlr_cap = 0;
    g_dlr_cursor = 0;
    g_dlr_fresh = true;
    g_dlr_nooverwrite = -1;
}

// Append khda_n4 float4s and bind the ring at PS t40. khda_first = where they start. False = no buffer or a failed map:
// the caller draws without lights.
inline bool kh_dlr_append(ID3D11DeviceContext* khda_ctx, const float* khda_src, uint32_t khda_n4,
                          uint32_t& khda_first) {
    khda_first = 0;
    if (!khda_ctx || !khda_src || khda_n4 == 0 || khda_n4 > (1u << 26)) return false;
    if (!g_dlr_buf || g_dlr_cap < khda_n4) {
        ID3D11Device* khda_dev = nullptr;
        khda_ctx->GetDevice(&khda_dev);
        if (!khda_dev) return false;
        if (g_dlr_nooverwrite < 0) {
            D3D11_FEATURE_DATA_D3D11_OPTIONS khda_o = {};
            g_dlr_nooverwrite = (SUCCEEDED(khda_dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &khda_o, sizeof(khda_o))) &&
                                 khda_o.MapNoOverwriteOnDynamicBufferSRV) ? 1 : 0;
        }
        uint32_t khda_cap = g_dlr_cap > 1024u ? g_dlr_cap : 1024u;
        while (khda_cap < khda_n4) khda_cap *= 2u;
        KH_SAFE_RELEASE(g_dlr_srv);
        KH_SAFE_RELEASE(g_dlr_buf);
        g_dlr_cap = 0;
        D3D11_BUFFER_DESC khda_bd = {};
        khda_bd.ByteWidth = khda_cap * 16u;
        khda_bd.Usage = D3D11_USAGE_DYNAMIC;
        khda_bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        khda_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        khda_bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        khda_bd.StructureByteStride = 16u;
        if (SUCCEEDED(khda_dev->CreateBuffer(&khda_bd, nullptr, &g_dlr_buf)) && g_dlr_buf) {
            D3D11_SHADER_RESOURCE_VIEW_DESC khda_sd = {};
            khda_sd.Format = DXGI_FORMAT_UNKNOWN;
            khda_sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            khda_sd.Buffer.FirstElement = 0;
            khda_sd.Buffer.NumElements = khda_cap;
            if (FAILED(khda_dev->CreateShaderResourceView(g_dlr_buf, &khda_sd, &g_dlr_srv)) || !g_dlr_srv) {
                g_dlr_srv = nullptr;
                KH_SAFE_RELEASE(g_dlr_buf);
            }
        } else {
            g_dlr_buf = nullptr;
        }
        khda_dev->Release();
        if (!g_dlr_srv) return false;
        g_dlr_cap = khda_cap;
        g_dlr_cursor = 0;
        g_dlr_fresh = true;
    }
    uint32_t khda_at = 0;
    D3D11_MAP khda_mode = D3D11_MAP_WRITE_DISCARD;
    if (g_dlr_nooverwrite == 1) {
        if (g_dlr_fresh || g_dlr_cursor + khda_n4 > g_dlr_cap) {
            g_dlr_cursor = 0;   // Wrap: rename, never stall.
        } else {
            khda_mode = D3D11_MAP_WRITE_NO_OVERWRITE;
        }
        khda_at = g_dlr_cursor;
    }
    D3D11_MAPPED_SUBRESOURCE khda_m = {};
    if (FAILED(khda_ctx->Map(g_dlr_buf, 0, khda_mode, 0, &khda_m)) || !khda_m.pData) {
        g_dlr_fresh = true;   // Re-DISCARD next time rather than trust a region never written.
        return false;
    }
    memcpy(static_cast<uint8_t*>(khda_m.pData) + static_cast<size_t>(khda_at) * 16u, khda_src,
           static_cast<size_t>(khda_n4) * 16u);
    khda_ctx->Unmap(g_dlr_buf, 0);
    g_dlr_fresh = false;
    if (g_dlr_nooverwrite == 1) g_dlr_cursor = khda_at + khda_n4;
    if (!kh_bc_on(khda_ctx) || !t_kh_bc.k40 || t_kh_bc.v40 != g_dlr_srv) {   // KH_BIND_FILTER.
        khda_ctx->PSSetShaderResources(40, 1, &g_dlr_srv);
        if (kh_bc_on(khda_ctx)) { t_kh_bc.v40 = g_dlr_srv; t_kh_bc.k40 = true; }
    }
    khda_first = khda_at;
    return true;
}

inline float kh_dl_intensity() {
    const uint32_t khdi_bits = g_dl_intensity_bits.load(std::memory_order_relaxed);
    float khdi_v = 1.0f;
    memcpy(&khdi_v, &khdi_bits, sizeof(khdi_v));
    return khdi_v;
}

// The lights a mesh receives: every pool light whose hard-fade reach touches its bounding sphere, nearest first (a
// stable sort), then packed points first and spots after (the split the shader's pointN reads). No count cap: a light
// out of reach adds an exact zero anyway (the range fade), so the reach test is the whole cull, and one that reaches is
// never dropped for another. KH_DL_NO_FADE: a light authored with no hard fade (its inverse width ~1e-20) has no such
// reach - the shader's fade term stays 1 at any distance - so it is taken for every mesh, as kh_dls_fill_dl_world packs
// it for the world ("unbounded"). Empty when the pool is empty or stale. Returns the distance scale the test used.
inline float kh_dl_select(const RenderObject& o, std::vector<uint32_t>& khds_sel, uint32_t& khds_pn) {
    khds_sel.clear();
    khds_pn = 0;
    const float khds_scale = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f)
                           ? g_dl.main_scale : 1.0f;
    if (g_dl.pool_n == 0) return khds_scale;
    if (steady_now_ms() - g_dl.pool_stamp > KH_DL_POOL_STALE_MS) return khds_scale;
    // Mesh center + half diagonal, engine axes (SQF [x,y,zASL] -> engine [x, zASL, y]). KH_NEARZ_BOUNDS: the diagonal
    // of the box around everything the mesh may draw (kh_bounds_size_of: a cloth's reach, a decimated level, a vertex
    // stage's reach, the residual), each axis grown by its rotation row's scale past 1 - a light reaching only a part
    // past the script's size lights it too. A builtin with none of those and unit rows keeps the script's size.
    const float khds_c[3] = { o.pos[0], o.pos[2], o.pos[1] };
    float khds_sz[3];
    kh_bounds_size_of(o, khds_sz);
    if (o.rotated) {
        static const int khds_ax[3] = { 0, 2, 1 };   // Row j (engine-local axis j) -> its SQF size axis.
        for (int khds_j = 0; khds_j < 3; ++khds_j) {
            const float* const khds_w = &o.rot_m[khds_j * 3];
            const float khds_n = sqrtf(khds_w[0] * khds_w[0] + khds_w[1] * khds_w[1] + khds_w[2] * khds_w[2]);
            if (khds_n > 1.0f) khds_sz[khds_ax[khds_j]] *= khds_n;
        }
    }
    const float khds_half = 0.5f * sqrtf(khds_sz[0] * khds_sz[0] +
                                         khds_sz[1] * khds_sz[1] +
                                         khds_sz[2] * khds_sz[2]);
    static thread_local std::vector<std::pair<float, uint32_t>> khds_d;   // Scratch.
    khds_d.clear();
    for (uint32_t i = 0; i < g_dl.pool_n; ++i) {
        const DlPoolLight& khds_pl = g_dl.pool[i];
        const float khds_dx = khds_pl.rec[0] - khds_c[0];
        const float khds_dy = khds_pl.rec[1] - khds_c[1];
        const float khds_dz = khds_pl.rec[2] - khds_c[2];
        const float khds_dd = sqrtf(khds_dx * khds_dx + khds_dy * khds_dy + khds_dz * khds_dz);
        // hard-fade reach: fade zeroes at dist*scale >= start + 1/invW. None (KH_DL_NO_FADE): unbounded - do not cut at
        // the fade start alone: it drops every mesh past it that the light still lights.
        const float khds_inv = khds_pl.rec[21];
        if (khds_inv > 1.0e-6f) {
            const float khds_reach = (khds_pl.rec[20] + 1.0f / khds_inv) / khds_scale;
            if (khds_dd > khds_reach + khds_half) continue;
        }
        khds_d.push_back({ khds_dd, i });
    }
    std::stable_sort(khds_d.begin(), khds_d.end(),
                     [](const std::pair<float, uint32_t>& khds_a, const std::pair<float, uint32_t>& khds_b) {
                         return khds_a.first < khds_b.first;
                     });
    for (int khds_pass = 0; khds_pass < 2; ++khds_pass) {
        for (const auto& khds_e : khds_d) {
            if ((g_dl.pool[khds_e.second].spot != 0) != (khds_pass == 1)) continue;
            khds_sel.push_back(khds_e.second);
            if (khds_pass == 0) ++khds_pn;
        }
    }
    return khds_scale;
}

// A selection's records as the shader reads them: the engine's 6-float4 cb11
// record verbatim, with OUR shadow slot (+1; 0 = none) in [5].z - KH_DL_SHADOW.
inline void kh_dl_pack(const std::vector<uint32_t>& khdp_sel, std::vector<float>& khdp_out) {
    khdp_out.resize(khdp_sel.size() * 24u);
    for (size_t k = 0; k < khdp_sel.size(); ++k) {
        const DlPoolLight& khdp_pl = g_dl.pool[khdp_sel[k]];
        memcpy(&khdp_out[k * 24u], khdp_pl.rec, KH_DL_LIGHT_BYTES);
        khdp_out[k * 24u + 22u] = static_cast<float>(kh_dls_slot_of(khdp_pl) + 1);
    }
}

inline void kh_dl_fill_ctl(ConstantData& cbd, uint32_t khdc_n, uint32_t khdc_pn, float khdc_scale) {
    cbd.dl_ctl[0] = 3.0f;
    cbd.dl_ctl[1] = static_cast<float>(khdc_pn);
    cbd.dl_ctl[2] = static_cast<float>(khdc_n - khdc_pn);
    cbd.dl_ctl[3] = khdc_scale;

    if (g_dl.main_ref_valid &&
        (g_dl.main_gdiff[0] > 0.0f || g_dl.main_gdiff[1] > 0.0f || g_dl.main_gdiff[2] > 0.0f)) {
        cbd.dl_global[0] = g_dl.main_gdiff[0];   // The main pass's cb10[3].
        cbd.dl_global[1] = g_dl.main_gdiff[1];
        cbd.dl_global[2] = g_dl.main_gdiff[2];
    } else {
        cbd.dl_global[0] = 1.0f;   // Cold / degenerate reference.
        cbd.dl_global[1] = 1.0f;
        cbd.dl_global[2] = 1.0f;
    }

    cbd.dl_global[3] = kh_dl_intensity();
}

inline void fill_dynlights_cb(ID3D11DeviceContext* ctx, ConstantData& cbd, const RenderObject& o) {
    if (g_dl.pool_n != 0 && steady_now_ms() - g_dl.pool_stamp > KH_DL_POOL_STALE_MS) {
        kh_stat(g_stats.dl_stale_fills);   // KH_DL_DIAG: this draw gets no dynamic light.
    }
    static thread_local std::vector<uint32_t> khd_sel;   // Scratch.
    static thread_local std::vector<float> khd_rec;      // Scratch.
    uint32_t khd_pn = 0;
    const float khd_scale = kh_dl_select(o, khd_sel, khd_pn);
    if (khd_sel.empty()) return;
    kh_dl_pack(khd_sel, khd_rec);
    const uint32_t khd_n = static_cast<uint32_t>(khd_sel.size());
    uint32_t khd_first = 0;
    if (!kh_dlr_append(ctx, khd_rec.data(), khd_n * 6u, khd_first)) {
        kh_stat(g_stats.dl_ring_fails);   // KH_DL_DIAG.
        return;
    }
    if (kh_stats_on() && khd_n > g_stats.dl_mesh_lights_max) g_stats.dl_mesh_lights_max = khd_n;   // KH_DL_DIAG.
    memcpy(&cbd.dl_first[0], &khd_first, sizeof(khd_first));
    cbd.dl_first[1] = 0.0f;
    cbd.dl_first[2] = 0.0f;
    cbd.dl_first[3] = 0.0f;
    kh_dl_fill_ctl(cbd, khd_n, khd_pn, khd_scale);
}
inline void fill_lighting_frame_cb(ConstantData& cbd) {
    cbd.lighting1[0] = g_sun_dir_engine[0];
    cbd.lighting1[1] = g_sun_dir_engine[1];
    cbd.lighting1[2] = g_sun_dir_engine[2];
    cbd.lighting1[3] = g_sun_valid ? 1.0f : 0.0f;   // Structural: the sun is published (see KH_LIT_FRESH_ERR's note).
    // The published block is the engine's own base lighting, republished every world frame. No override sits between it
    // and the shader: a zero sun is night.
    if (g_pub_block_valid) {
        cbd.lighting2[0] = g_pub_block_sun[0];
        cbd.lighting2[1] = g_pub_block_sun[1];
        cbd.lighting2[2] = g_pub_block_sun[2];
        cbd.light_amb[0] = g_pub_block_amb[0];
        cbd.light_amb[1] = g_pub_block_amb[1];
        cbd.light_amb[2] = g_pub_block_amb[2];
        cbd.light_amb[3] = 1.0f;
    } else {
        cbd.lighting2[0] = 1.0f;
        cbd.lighting2[1] = 1.0f;
        cbd.lighting2[2] = 1.0f;
        cbd.light_amb[0] = 1.0f;
        cbd.light_amb[1] = 1.0f;
        cbd.light_amb[2] = 1.0f;
        cbd.light_amb[3] = 0.0f;
    }
    cbd.lighting2[3] = g_shadow_map_strength;

    const bool khl_map_fresh = g_sun_map_valid && g_sun_map_time >= 0.0f &&
        effect_time_seconds() - g_sun_map_time < KH_SUN_MAP_SELF_MAX_AGE_S;

    // One origin for every sun matrix in this template, set before any of them fills.
    cbd.sun_origin[0] = g_sun_anchor_now[0];
    cbd.sun_origin[1] = g_sun_anchor_now[1];
    cbd.sun_origin[2] = g_sun_anchor_now[2];

    {
        if (g_vmir_mask_time >= 0.0f && g_vmir_w > 0 && effect_time_seconds() - g_vmir_mask_time < 0.5f) {
            cbd.mir_meta[0] = 1.0f;
            cbd.mir_meta[1] = static_cast<float>(g_vmir_w);
            cbd.mir_meta[2] = static_cast<float>(g_vmir_h);
        }
        if (g_sun_anchor_now[0] != 0.0f || g_sun_anchor_now[1] != 0.0f ||
             g_sun_anchor_now[2] != 0.0f) {
            cbd.mir_meta[3] = fminf(fmaxf(
                g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
        }
    }

    if (khl_map_fresh) {
        // The union matrix may be older than this flush's anchor (hash-skip frames): rebase it algebraically so every
        // matrix in this block shares cbd.sun_origin.
        kh_fill_sun_tiers_cb(cbd);
        kh_fill_sun_pf_lanes(cbd);
    } else {
        // KH_DL_SHADOW: the light maps need no sun (kh_dls_select and kh_dls_frame run without one), so their lanes
        // fill whether or not the sun's do - as the world pass fills its own (kh_dls_world_pass_body).
        kh_dls_fill_cb(cbd);
    }

    if (!g_atlas_frame_cur && !g_atlas_frame_prev) {
        return;
    }

    // KH_BAND_META: the compare sign, the bias and the atlas size are the band receive's lanes too
    // (ShadowBandFactor / KhBandTap4), which maskMeta.x arms on a valid band, a published sun and a live atlas -
    // not on the cascade table. Filled ahead of the table's gates: else a relock that empties the table while the
    // bands stand (the injection's miss-streak drop) leaves them zero, every band compare false and our meshes lit.
    // The count (below) still short-circuits the cascade path.
    cbd.shadow_meta[1] = g_shadow_map_sign;
    cbd.shadow_meta[2] = g_shadow_map_bias;
    cbd.shadow_meta[3] = static_cast<float>(g_ls.atlas_size);

    if (g_ls.count == 0 || g_ls.newest < 0) {
        return;
    }

    uint32_t order[KH_LIVE_MAX_CASCADES];
    uint32_t n = 0;
    float owpt[KH_LIVE_MAX_CASCADES];

    for (uint32_t i = 0; i < g_ls.count && n < KH_LIVE_MAX_CASCADES; ++i) {
        const LiveShadowEntry& le = g_ls.entries[i];
        const float ilen = sqrtf(le.m[0] * le.m[0] + le.m[4] * le.m[4] + le.m[8] * le.m[8]);
        const float texels = le.tile[2] > le.tile[0]
                           ? (le.tile[2] - le.tile[0]) * static_cast<float>(g_ls.atlas_size)
                           : 1.0f;
        const float wpt = ilen > 1e-9f ? (2.0f / ilen) / (texels > 1.0f ? texels : 1.0f) : 1e9f;

        uint32_t j = n;
        while (j > 0 && owpt[j - 1] > wpt) {
            owpt[j] = owpt[j - 1];
            order[j] = order[j - 1];
            --j;
        }

        owpt[j] = wpt;
        order[j] = i;
        ++n;
    }

    if (n == 0) return;

    // No sun published yet this session (g_sun_valid never falls once set: KH_LIT_FRESH_ERR's note): the zeroed
    // count short-circuits the shader.
    if (!g_sun_valid) return;

    // shadow_meta[0] (the count) is written last, after the table lands.
    uint32_t out_i = 0;

    // shadowMeta2 is no frame lane (x = the chain's near-z marker arm, y = the object's view-distance cut, z = its
    // depth-Off flag, w = a vertex stage run's first vertex or the chain's KH_VM_SEE arm): zero here. Each entry's
    // camera origin is folded into its translation, so the shader reads absolute world positions and per-entry
    // staleness cannot misregister.

    for (uint32_t i = 0; i < n && out_i < KH_LIVE_MAX_CASCADES; ++i, ++out_i) {
        const LiveShadowEntry& e = g_ls.entries[order[i]];
        memcpy(cbd.shadow_tiles[out_i], e.tile, sizeof(e.tile));

        for (int r = 0; r < 3; ++r) {
            float* dst = cbd.shadow_mats[out_i * 3 + r];
            const float* row = e.m + r * 4;
            dst[0] = row[0];
            dst[1] = row[1];
            dst[2] = row[2];
            // t' = t - M.row. cam_entry: uvz = M*(p - cam) + t = M*p + t'.
            dst[3] = row[3] - (row[0] * e.cam[0] + row[1] * e.cam[1] + row[2] * e.cam[2]);
        }
    }

    cbd.shadow_meta[0] = static_cast<float>(out_i);

}

// Object half: the localization mask and the camera-distance band lanes - the one fill of them on every route (the
// PIP's pass and mesh, the inFront slice, the injection, the flush, the UI lane).
inline void kh_fill_local_band_cb(ConstantData& cbd, const RenderObject& o) {
    cbd.local0[0] = o.pos[0];
    cbd.local0[1] = o.pos[2];
    cbd.local0[2] = o.pos[1];
    cbd.local0[3] = static_cast<float>(o.local_shape);
    cbd.local_radii[0] = o.local_radius[0];
    cbd.local_radii[1] = o.local_radius[2];
    cbd.local_radii[2] = o.local_radius[1];
    const float mean_r = (o.local_radius[0] + o.local_radius[1] + o.local_radius[2]) / 3.0f;
    cbd.local1[0] = o.local_falloff / (mean_r > 0.01f ? mean_r : 0.01f);
    cbd.local1[1] = o.localized ? 1.0f : 0.0f;
    cbd.local_radii[3] = o.local_inverse ? 1.0f : 0.0f;
    cbd.band0[0] = o.band_min;
    cbd.band0[1] = o.band_max;
    cbd.band0[2] = o.band_falloff;
    cbd.band0[3] = o.banded ? 1.0f : 0.0f;
}

// The identity of the dynamic-light block fill_dynlights_cb would upload for this object - a hash over the lanes it
// writes. 0 for an unlit object, never 0 for a lit one (even with no light selected); two lit objects bucket together
// exactly when they would upload the same block. Scratch is thread-local: a pass runs on one thread at a time.
inline uint64_t kh_dl_bucket_hash(const RenderObject& o) {
    if (!o.lit) return 0;
    static thread_local ConstantData khdh_c;
    static thread_local std::vector<uint32_t> khdh_sel;   // Scratch.
    static thread_local std::vector<float> khdh_rec;      // Scratch.
    memset(khdh_c.dl_ctl, 0, sizeof(khdh_c.dl_ctl));
    memset(khdh_c.dl_global, 0, sizeof(khdh_c.dl_global));
    memset(khdh_c.dl_view, 0, sizeof(khdh_c.dl_view));
    // fill_dynlights_cb's lanes and records without the ring append (the ring position is not the block's identity).
    uint32_t khdh_pn = 0;
    const float khdh_scale = kh_dl_select(o, khdh_sel, khdh_pn);
    khdh_rec.clear();
    if (!khdh_sel.empty()) {
        kh_dl_pack(khdh_sel, khdh_rec);
        kh_dl_fill_ctl(khdh_c, static_cast<uint32_t>(khdh_sel.size()), khdh_pn, khdh_scale);
    }
    uint64_t h = CryptoGenerator::FNV1A64_OFFSET;
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(khdh_c.dl_ctl), sizeof(khdh_c.dl_ctl));
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(khdh_c.dl_global), sizeof(khdh_c.dl_global));
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(khdh_c.dl_view), sizeof(khdh_c.dl_view));
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(khdh_rec.data()),
                                        khdh_rec.size() * sizeof(float));
    return h | 1u;
}
inline void fill_lighting_obj_cb(ID3D11DeviceContext* ctx, ConstantData& cbd, const RenderObject& o) {
    cbd.lighting0[0] = o.lit ? 1.0f : 0.0f;
    cbd.lighting0[1] = (o.receive_shadow && !g_objbuf_shoff) ? 0.0f : 1.0f;   // KH_SHADOW_SWITCH / KH_SHADOW_OFF.
    cbd.lighting0[2] = o.light_ambient;
    cbd.lighting0[3] = o.light_diffuse;
    cbd.shadow_meta2[0] = 0.0f;   // KH_NEARZ_MARK: the effect chain arms its own passes; 0 everywhere else.
    // 0 when never read - the shader treats 0 as no cut. An object is sliced at the engine's object view distance.
    cbd.shadow_meta2[1] = g_obj_vis.load(std::memory_order_relaxed);
    // KH_VOL_WITNESS: 1 = a depth-Off overlay, which the seam never draws into the footprint the stencil copy is
    // counted against, so it has no witness. Never bucketed (kh_inst_eligible), so the per-object lane is its own.
    cbd.shadow_meta2[2] = (o.mode == DepthMode::Off) ? 1.0f : 0.0f;
    kh_fill_user_obj_cb(cbd, o);   // KH_USER_LANES.
    if (o.lit) fill_dynlights_cb(ctx, cbd, o);
}
}