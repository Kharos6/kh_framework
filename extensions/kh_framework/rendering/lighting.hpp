#pragma once

// The feature half (rendering_integration_features.hpp): the lighting constants: the dynamic lights' selection, packing
// and ring, the frame and object lighting fills. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/dls_select.hpp (declared here for the uses below).
inline int dls_slot_of(const DlPoolLight& l);


// Dynamic lights are baseline: no mode, no off switch. The pool is merged in absolute world space (dlCtl.x = 3 in the
// shader). g_dl_intensity_bits scales the whole sum (dlGlobal.w); no command sets it, so it holds 1.0, which the
// session resets restore.
static std::atomic<uint32_t> g_dl_intensity_bits{0x3F800000u};   // Float bits, default 1.0.

// DL_RING: the dynamic lights' ring buffer, its view, capacity and cursor.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// The ring's buffer is released by release_shadow_device_state before the session reset nulls it.
// Session reset: session_globals_reset resets every member.
struct DlRingState {
    // DL_RING - the per-draw light records, uncapped. A dynamic structured buffer of float4 (t40, HLSL khDlRecs)
    // each lit draw appends its selection to, in the CB ring's shape (upload_obj_cb): NO_OVERWRITE into space no
    // append since the last DISCARD has used, DISCARD at the wrap. A device without NO_OVERWRITE on dynamic SRV buffers
    // DISCARDs every append and writes at 0. The buffer doubles whenever one draw's records do not fit, so no count is
    // ever refused. A draw reads its records at dl_first.x; the region stays valid until the next append that DISCARDs,
    // and every fill site draws its object before it fills the next one - the invariant that makes a DISCARD safe.
    // Render thread, or the game thread under a park (the fill sites' rule) - or inside FX_LATE's chain-only flush,
    // which reaches it through the dynamicLightFog block on the game thread under dlf_select's premise (the engine
    // never drives its context from both threads at once). Device objects: released by release_shadow_device_state
    // (dlr_release).
    ID3D11Buffer*             buf = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    uint32_t cap = 0;            // Capacity, float4 elements.
    uint32_t cursor = 0;         // Next free element since the last DISCARD.
    bool     fresh = true;       // The next map DISCARDs.
    int      nooverwrite = -1;   // Device cap: -1 unknown, 0 no, 1 yes.
};
static DlRingState g_dlr;

inline void dlr_release() {
    KH_SAFE_RELEASE(g_dlr.srv);
    KH_SAFE_RELEASE(g_dlr.buf);
    g_dlr.cap = 0;
    g_dlr.cursor = 0;
    g_dlr.fresh = true;
    g_dlr.nooverwrite = -1;
}

// Append n4 float4s and bind the ring at PS t40. first = where they start. False = no buffer or a failed map:
// the caller draws without lights.
inline bool dlr_append(ID3D11DeviceContext* ctx, const float* src, uint32_t n4,
                       uint32_t& first) {
    first = 0;
    if (!ctx || !src || n4 == 0 || n4 > (1u << 26)) return false;
    if (!g_dlr.buf || g_dlr.cap < n4) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) return false;
        if (g_dlr.nooverwrite < 0) {
            D3D11_FEATURE_DATA_D3D11_OPTIONS o = {};
            g_dlr.nooverwrite = (SUCCEEDED(dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &o,
                sizeof(o))) &&
                                 o.MapNoOverwriteOnDynamicBufferSRV) ? 1 : 0;
        }
        uint32_t cap = g_dlr.cap > 1024u ? g_dlr.cap : 1024u;
        while (cap < n4) cap *= 2u;
        KH_SAFE_RELEASE(g_dlr.srv);
        KH_SAFE_RELEASE(g_dlr.buf);
        g_dlr.cap = 0;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = cap * 16u;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = 16u;
        if (SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &g_dlr.buf)) && g_dlr.buf) {
            D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
            sd.Format = DXGI_FORMAT_UNKNOWN;
            sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            sd.Buffer.FirstElement = 0;
            sd.Buffer.NumElements = cap;
            if (FAILED(dev->CreateShaderResourceView(g_dlr.buf, &sd, &g_dlr.srv)) || !g_dlr.srv) {
                g_dlr.srv = nullptr;
                KH_SAFE_RELEASE(g_dlr.buf);
            }
        } else {
            g_dlr.buf = nullptr;
        }
        dev->Release();
        if (!g_dlr.srv) return false;
        g_dlr.cap = cap;
        g_dlr.cursor = 0;
        g_dlr.fresh = true;
    }
    uint32_t at = 0;
    D3D11_MAP mode = D3D11_MAP_WRITE_DISCARD;
    if (g_dlr.nooverwrite == 1) {
        if (g_dlr.fresh || g_dlr.cursor + n4 > g_dlr.cap) {
            g_dlr.cursor = 0;   // Wrap: rename, never stall.
        } else {
            mode = D3D11_MAP_WRITE_NO_OVERWRITE;
        }
        at = g_dlr.cursor;
    }
    D3D11_MAPPED_SUBRESOURCE m = {};
    if (FAILED(ctx->Map(g_dlr.buf, 0, mode, 0, &m)) || !m.pData) {
        g_dlr.fresh = true;   // Re-DISCARD next time rather than trust a region never written.
        return false;
    }
    memcpy(static_cast<uint8_t*>(m.pData) + static_cast<size_t>(at) * 16u, src,
           static_cast<size_t>(n4) * 16u);
    ctx->Unmap(g_dlr.buf, 0);
    g_dlr.fresh = false;
    if (g_dlr.nooverwrite == 1) g_dlr.cursor = at + n4;
    if (!bc_on(ctx) || !t_kh_bc.k40 || t_kh_bc.v40 != g_dlr.srv) {   // BIND_FILTER.
        ctx->PSSetShaderResources(40, 1, &g_dlr.srv);
        if (bc_on(ctx)) { t_kh_bc.v40 = g_dlr.srv; t_kh_bc.k40 = true; }
    }
    first = at;
    return true;
}

inline float dl_intensity() {
    const uint32_t bits = g_dl_intensity_bits.load(std::memory_order_relaxed);
    float v = 1.0f;
    memcpy(&v, &bits, sizeof(v));
    return v;
}

// The lights a mesh receives: every pool light whose hard-fade reach touches its bounding sphere, nearest first (a
// stable sort), then packed points first and spots after (the split the shader's pointN reads). No count cap: a light
// out of reach adds an exact zero anyway (the range fade), so the reach test is the whole cull, and one that reaches is
// never dropped for another. DL_NO_FADE: a light authored with no hard fade (its inverse width ~1e-20) has no such
// reach - the shader's fade term stays 1 at any distance - so it is taken for every mesh, as dls_fill_dl_world packs
// it for the world ("unbounded"). Empty when the pool is empty or stale. Returns the distance scale the test used.
inline float dl_select(const RenderObject& o, std::vector<uint32_t>& sel, uint32_t& pn) {
    sel.clear();
    pn = 0;
    const float scale = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f)
                      ? g_dl.main_scale : 1.0f;
    if (g_dl.pool_n == 0) return scale;
    if (steady_now_ms() - g_dl.pool_stamp > DL_POOL_STALE_MS) return scale;
    // Mesh center + half diagonal, engine axes (SQF [x,y,zASL] -> engine [x, zASL, y]). NEARZ_BOUNDS: the diagonal
    // of the box around everything the mesh may draw (bounds_size_of: a cloth's reach, a decimated level, a vertex
    // stage's reach, the residual), each axis grown by its rotation row's scale past 1 - a light reaching only a part
    // past the script's size lights it too. A builtin with none of those and unit rows keeps the script's size.
    const float c[3] = { o.pos[0], o.pos[2], o.pos[1] };
    float sz[3];
    bounds_size_of(o, sz);
    if (o.rotated) {
        static const int ax[3] = { 0, 2, 1 };   // Row j (engine-local axis j) -> its SQF size axis.
        for (int j = 0; j < 3; ++j) {
            const float* const w = &o.rot_m[j * 3];
            const float n = sqrtf(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
            if (n > 1.0f) sz[ax[j]] *= n;
        }
    }
    const float half = 0.5f * sqrtf(sz[0] * sz[0] +
                                    sz[1] * sz[1] +
                                    sz[2] * sz[2]);
    static thread_local std::vector<std::pair<float, uint32_t>> d;   // Scratch.
    d.clear();
    for (uint32_t i = 0; i < g_dl.pool_n; ++i) {
        const DlPoolLight& pl = g_dl.pool[i];
        const float dx = pl.rec[0] - c[0];
        const float dy = pl.rec[1] - c[1];
        const float dz = pl.rec[2] - c[2];
        const float dd = sqrtf(dx * dx + dy * dy + dz * dz);
        // hard-fade reach: fade zeroes at dist*scale >= start + 1/invW. None (DL_NO_FADE): unbounded - do not cut at
        // the fade start alone: it drops every mesh past it that the light still lights.
        const float inv = pl.rec[21];
        if (inv > 1.0e-6f) {
            const float reach = (pl.rec[20] + 1.0f / inv) / scale;
            if (dd > reach + half) continue;
        }
        d.push_back({ dd, i });
    }
    std::stable_sort(d.begin(), d.end(),
                     [](const std::pair<float, uint32_t>& a, const std::pair<float, uint32_t>& b) {
                         return a.first < b.first;
                     });
    for (int pass = 0; pass < 2; ++pass) {
        for (const auto& e : d) {
            if ((g_dl.pool[e.second].spot != 0) != (pass == 1)) continue;
            sel.push_back(e.second);
            if (pass == 0) ++pn;
        }
    }
    return scale;
}

// A selection's records as the shader reads them: the engine's 6-float4 cb11
// record verbatim, with OUR shadow slot (+1; 0 = none) in [5].z - DL_SHADOW.
inline void dl_pack(const std::vector<uint32_t>& sel, std::vector<float>& out) {
    out.resize(sel.size() * 24u);
    for (size_t k = 0; k < sel.size(); ++k) {
        const DlPoolLight& pl = g_dl.pool[sel[k]];
        memcpy(&out[k * 24u], pl.rec, DL_LIGHT_BYTES);
        out[k * 24u + 22u] = static_cast<float>(dls_slot_of(pl) + 1);
    }
}

inline void dl_fill_ctl(ConstantData& cbd, uint32_t n, uint32_t pn, float scale) {
    cbd.dl_ctl[0] = 3.0f;
    cbd.dl_ctl[1] = static_cast<float>(pn);
    cbd.dl_ctl[2] = static_cast<float>(n - pn);
    cbd.dl_ctl[3] = scale;

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

    cbd.dl_global[3] = dl_intensity();
}

inline void fill_dynlights_cb(ID3D11DeviceContext* ctx, ConstantData& cbd, const RenderObject& o) {
    if (g_dl.pool_n != 0 && steady_now_ms() - g_dl.pool_stamp > DL_POOL_STALE_MS) {
        stat_inc(g_stats.dl_stale_fills);   // DL_DIAG: this draw gets no dynamic light.
    }
    static thread_local std::vector<uint32_t> sel;   // Scratch.
    static thread_local std::vector<float> rec;      // Scratch.
    uint32_t pn = 0;
    const float scale = dl_select(o, sel, pn);
    if (sel.empty()) return;
    dl_pack(sel, rec);
    const uint32_t n = static_cast<uint32_t>(sel.size());
    uint32_t first = 0;
    if (!dlr_append(ctx, rec.data(), n * 6u, first)) {
        stat_inc(g_stats.dl_ring_fails);   // DL_DIAG.
        return;
    }
    if (stats_on() && n > g_stats.dl_mesh_lights_max) g_stats.dl_mesh_lights_max = n;   // DL_DIAG.
    memcpy(&cbd.dl_first[0], &first, sizeof(first));
    cbd.dl_first[1] = 0.0f;
    cbd.dl_first[2] = 0.0f;
    cbd.dl_first[3] = 0.0f;
    dl_fill_ctl(cbd, n, pn, scale);
}
inline void fill_lighting_frame_cb(ConstantData& cbd) {
    cbd.lighting1[0] = g_sun_dir_engine[0];
    cbd.lighting1[1] = g_sun_dir_engine[1];
    cbd.lighting1[2] = g_sun_dir_engine[2];
    cbd.lighting1[3] = g_sun_valid ? 1.0f : 0.0f;   // Structural: the sun is published (see LIT_FRESH_ERR's note).
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

    const bool map_fresh = g_sun_maps.sun_map_valid && g_sun_maps.sun_map_time >= 0.0f &&
        effect_time_seconds() - g_sun_maps.sun_map_time < SUN_MAP_SELF_MAX_AGE_S;

    // One origin for every sun matrix in this template, set before any of them fills.
    cbd.sun_origin[0] = g_sun_maps.sun_anchor_now[0];
    cbd.sun_origin[1] = g_sun_maps.sun_anchor_now[1];
    cbd.sun_origin[2] = g_sun_maps.sun_anchor_now[2];

    {
        if (g_vmir.mask_time >= 0.0f && g_vmir.w > 0 && effect_time_seconds() - g_vmir.mask_time < 0.5f) {
            cbd.mir_meta[0] = 1.0f;
            cbd.mir_meta[1] = static_cast<float>(g_vmir.w);
            cbd.mir_meta[2] = static_cast<float>(g_vmir.h);
        }
        if (g_sun_maps.sun_anchor_now[0] != 0.0f || g_sun_maps.sun_anchor_now[1] != 0.0f ||
             g_sun_maps.sun_anchor_now[2] != 0.0f) {
            cbd.mir_meta[3] = fminf(fmaxf(
                g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
        }
    }

    if (map_fresh) {
        // The union matrix may be older than this flush's anchor (hash-skip frames): rebase it algebraically so every
        // matrix in this block shares cbd.sun_origin.
        fill_sun_tiers_cb(cbd);
        fill_sun_pf_lanes(cbd);
    } else {
        // DL_SHADOW: the light maps need no sun (dls_select and dls_frame run without one), so their lanes
        // fill whether or not the sun's do - as the world pass fills its own (dls_world_pass_body).
        dls_fill_cb(cbd);
    }

    if (!g_atlas_frame_cur && !g_atlas_frame_prev) {
        return;
    }

    // BAND_META: the compare sign, the bias and the atlas size are the band receive's lanes too
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

    uint32_t order[LIVE_MAX_CASCADES];
    uint32_t n = 0;
    float owpt[LIVE_MAX_CASCADES];

    for (uint32_t i = 0; i < g_ls.count && n < LIVE_MAX_CASCADES; ++i) {
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

    // No sun published yet this session (g_sun_valid never falls once set: LIT_FRESH_ERR's note): the zeroed
    // count short-circuits the shader.
    if (!g_sun_valid) return;

    // shadow_meta[0] (the count) is written last, after the table lands.
    uint32_t out_i = 0;

    // shadowMeta2 is no frame lane (x = the chain's near-z marker arm, y = the object's view-distance cut, z = its
    // depth-Off flag, w = a vertex stage run's first vertex or the chain's VM_SEE arm): zero here. Each entry's
    // camera origin is folded into its translation, so the shader reads absolute world positions and per-entry
    // staleness cannot misregister.

    for (uint32_t i = 0; i < n && out_i < LIVE_MAX_CASCADES; ++i, ++out_i) {
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
inline void fill_local_band_cb(ConstantData& cbd, const RenderObject& o) {
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
inline uint64_t dl_bucket_hash(const RenderObject& o) {
    if (!o.lit) return 0;
    static thread_local ConstantData c;
    static thread_local std::vector<uint32_t> sel;   // Scratch.
    static thread_local std::vector<float> rec;      // Scratch.
    memset(c.dl_ctl, 0, sizeof(c.dl_ctl));
    memset(c.dl_global, 0, sizeof(c.dl_global));
    // fill_dynlights_cb's lanes and records without the ring append (the ring position is not the block's identity).
    uint32_t pn = 0;
    const float scale = dl_select(o, sel, pn);
    rec.clear();
    if (!sel.empty()) {
        dl_pack(sel, rec);
        dl_fill_ctl(c, static_cast<uint32_t>(sel.size()), pn, scale);
    }
    uint64_t h = CryptoGenerator::FNV1A64_OFFSET;
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(c.dl_ctl), sizeof(c.dl_ctl));
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(c.dl_global), sizeof(c.dl_global));
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(rec.data()),
                                        rec.size() * sizeof(float));
    return h | 1u;
}
inline void fill_lighting_obj_cb(ID3D11DeviceContext* ctx, ConstantData& cbd, const RenderObject& o) {
    cbd.lighting0[0] = o.lit ? 1.0f : 0.0f;
    cbd.lighting0[1] = (o.receive_shadow && !g_objbuf_shoff) ? 0.0f : 1.0f;   // SHADOW_SWITCH / SHADOW_OFF.
    cbd.lighting0[2] = o.light_ambient;
    cbd.lighting0[3] = o.light_diffuse;
    cbd.shadow_meta2[0] = 0.0f;   // NEARZ_MARK: the effect chain arms its own passes; 0 everywhere else.
    // 0 when never read - the shader treats 0 as no cut. An object is sliced at the engine's object view distance.
    cbd.shadow_meta2[1] = g_obj_vis.load(std::memory_order_relaxed);
    // VOL_WITNESS: 1 = a depth-Off overlay, which the seam never draws into the footprint the stencil copy is
    // counted against, so it has no witness. Never bucketed (inst_eligible), so the per-object lane is its own.
    cbd.shadow_meta2[2] = (o.mode == DepthMode::Off) ? 1.0f : 0.0f;
    fill_user_obj_cb(cbd, o);   // USER_LANES.
    if (o.lit) fill_dynlights_cb(ctx, cbd, o);
}
}