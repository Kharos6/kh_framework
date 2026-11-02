#pragma once

// The injection half (rendering_integration_injection.hpp): KH_WSEE, the see-through record: the arm, the snapshot, the
// pre-draw, the re-issues and the deferral. Never include it on its own.
namespace RenderIntegration {


inline bool kh_wsee_hold_made();   // With the hold, below.
inline void kh_wsee_hold_drop();

// Defined in rendering/reorder_trigger.hpp (declared here for the uses below).
inline uint32_t kh_reorder_min_opaques();
inline bool kh_reorder_world_vp(const D3D11_VIEWPORT& khrw_vp);


// The chain's arm: KH_WSEE_ON, and the cycle the chain draws (the published one, as kh_vmsee_valid - the late chain
// may run on the game thread) is the one whose capture landed, untainted, its targets standing and made against the
// depth the chain reads (its texture and sample count).
inline bool kh_wsee_valid() {
    if constexpr (!KH_WSEE_ON) {   // Built out: no code left unreachable (C4702).
        return false;
    } else {
        const uint64_t khwv_c = g_topo_cycles_pub.load(std::memory_order_relaxed);
        return g_wsee_ok_cycle.load(std::memory_order_relaxed) == khwv_c &&
               g_wsee_taint_cycle.load(std::memory_order_relaxed) != khwv_c &&
               g_res.wsee_s_srv != nullptr && g_res.wsee_t_srv != nullptr && g_res.wsee_b_srv != nullptr &&
               g_res.wsee_n_srv != nullptr && g_res.wsee_n2_srv != nullptr && g_res.wsee_depth_id != nullptr &&
               g_res.wsee_depth_id == g_res.depth_res_identity && g_res.wsee_samples == g_res.depth_sample_count;
    }
}
// The cycle's verdict at the main depth's clear (render thread, after the cycle counter steps): KH_WSEE_ON, a want at
// most KH_WSEE_WANT_SPAN cycles old (one stamped later in the cycle - the scene flush's, the late chain's - counts from
// the next clear), no back-off, no KH_WSEE_FLUSHW_PRED stand-down. The ended cycle's attempt is final here (its flush
// and any park ran inside it): landed untainted ends a run of waste, tainted or refused extends it, and
// KH_WSEE_WASTE_RUN of them begin a back-off (the KH_WSEE_FLUSHW_PRED hold where our flush's write ended the run).
// KH_WSEE_IDLE_FREE cycles without a want release the targets: no chain binds them past this clear (a bind needs this
// cycle's capture, kh_wsee_valid). Not called with the system built out.
inline void kh_wsee_cycle_note() {
    const uint64_t khwc_c = g_topo_cycles;
    // KH_WSEE_FLUSHW_PRED: whether the ended cycle's flush wrote the main depth after its capture (the stamp's note).
    const bool khwc_fwp = khwc_c >= 1u && g_wsee_flushw_cycle.load(std::memory_order_relaxed) == khwc_c - 1u;
    if (g_wsee_snap_cycle == khwc_c - 1u) {
        const bool khwc_used = g_wsee_ok_cycle.load(std::memory_order_relaxed) == khwc_c - 1u &&
                               g_wsee_taint_cycle.load(std::memory_order_relaxed) != khwc_c - 1u;
        // KH_WSEE_FLAP: the verdict against the last attempted cycle's.
        if (khwc_c < g_wsee_flap_win_start || khwc_c - g_wsee_flap_win_start >= KH_WSEE_FLAP_WIN) {
            g_wsee_flap_win_start = khwc_c;
            g_wsee_flap_n = 0;
        }
        if (g_wsee_flap_prev >= 0 && (g_wsee_flap_prev != 0) != khwc_used) ++g_wsee_flap_n;
        g_wsee_flap_prev = khwc_used ? 1 : 0;
        if (g_wsee_flap_n >= KH_WSEE_FLAP_N) {
            g_wsee_flap_n = 0;
            g_wsee_flap_prev = -1;
            g_wsee_waste_n = 0;
            g_wsee_backoff_end = khwc_c + KH_WSEE_BACKOFF;
        }
        if (khwc_used) {
            g_wsee_waste_n = 0;
            g_wsee_flushw_hold = false;   // KH_WSEE_FLUSHW_PRED.
        } else if (++g_wsee_waste_n >= KH_WSEE_WASTE_RUN) {
            g_wsee_waste_n = 0;
            if (khwc_fwp) {   // KH_WSEE_FLUSHW_PRED: the run's cause is our flush's write - the hold, not the back-off.
                g_wsee_flushw_hold = true;
                g_wsee_flushw_probe = khwc_c - 1u;
            } else {
                g_wsee_flushw_hold = false;
                g_wsee_backoff_end = khwc_c + KH_WSEE_BACKOFF;
            }
        } else if (g_wsee_flushw_hold && khwc_fwp) {
            g_wsee_flushw_probe = khwc_c - 1u;   // A probe (or a capture the hold let through) the write spoiled.
        }
    }
    const bool khwc_on = KH_WSEE_ON;
    const uint64_t khwc_w = g_wsee_want_cycle.load(std::memory_order_relaxed);
    const bool khwc_want = khwc_w <= khwc_c && khwc_c - khwc_w <= KH_WSEE_WANT_SPAN;
    g_wsee_on = khwc_on && khwc_want && khwc_c >= g_wsee_backoff_end;
    // KH_WSEE_FLUSHW_PRED: under the hold, a cycle after a stamped one stands down but for the probe.
    if (g_wsee_flushw_hold && g_wsee_on && khwc_fwp && khwc_c - g_wsee_flushw_probe < KH_WSEE_FLUSHW_PROBE) {
        g_wsee_on = false;
        g_wsee_flushw_off.store(khwc_c, std::memory_order_relaxed);
    }
    // The pinned depth view (g_res.wsee_dsv_pin's note) goes with a cycle that captures nothing or an identity no
    // longer the adopted depth's: its one reference keeps no stale depth texture past this clear.
    if (g_res.wsee_dsv_pin && (!g_wsee_on || g_res.wsee_depth_id != g_res.depth_res_identity)) {
        g_res.release_wsee_dsv_pin();
    }
    // The layer-depth split, one verdict a cycle: on unless a make was refused (wsee_ld_failed), whose targets go at
    // this clear.
    g_wsee_cyc_layer = khwc_on && !g_res.wsee_ld_failed;
    if (!g_wsee_cyc_layer && g_res.wsee_tp_tex) g_res.release_wsee_ld();
    // The geometry hold goes after KH_WSEE_IDLE_FREE cycles without a copy into it (KH_WSEE_HOLD_IDLE_COPY: the last
    // cycle a copy entry used the buffer or its run caches - an own entry uses neither; the targets' own idle span,
    // so deferrable parts that come and go do not remake it). The ended cycle's held copies were its flush's, or are
    // dropped with its record below.
    if (kh_wsee_hold_made() &&
        (g_wsee_hold_copy_cycle > khwc_c || khwc_c - g_wsee_hold_copy_cycle > KH_WSEE_IDLE_FREE)) {
        kh_wsee_hold_drop();
    }
    // A loss: deferred copies no flush took, or a flush that appended them and bailed before its draw loop; either
    // stands the deferral down for KH_WSEE_DEFER_BACKOFF cycles (a persistent cause - a scene resolve the flush
    // cannot use - would otherwise lose our parts every frame).
    bool khwc_lost = false;
    if (g_wsee_defer_cycle != ~0ull) {
        if (g_wsee_defer_cycle != khwc_c) khwc_lost = true;
        g_wsee_defer_cycle = ~0ull;
        g_wsee_defer_objs.clear();
    }
    if (g_wsee_defer_taken != ~0ull && g_wsee_defer_taken != g_wsee_defer_drawn) khwc_lost = true;
    // KH_WSEE_DEFER_TAINT: a deferred cycle that ended tainted is a loss too - its copies drew at the flush under the
    // hardware test (kh_wsee_sort_mode 0), after the engine's blended depth-writers, where the injection would have
    // drawn them before those; a persistent cause (a flush solid writing depth every frame) stands the deferral
    // down for KH_WSEE_DEFER_BACKOFF cycles rather than every probing cycle of the capture's back-off.
    if (g_wsee_defer_taken == khwc_c - 1u &&
        g_wsee_taint_cycle.load(std::memory_order_relaxed) == khwc_c - 1u) khwc_lost = true;
    g_wsee_defer_taken = ~0ull;
    g_wsee_defer_drawn = ~0ull;
    if (khwc_lost) g_wsee_defer_back = khwc_c + KH_WSEE_DEFER_BACKOFF;
    if ((g_res.wsee_s_tex || g_res.wsee_e_tex || g_res.wsee_c_srv) &&
        (!khwc_on || khwc_w > khwc_c || khwc_c - khwc_w > KH_WSEE_IDLE_FREE)) {
        g_res.release_wsee_targets();
    }
}

// The scene flush's KH_FX_LATE take (render thread, beside kh_vmsee_arm): a pass the late chain will run that would
// take a mode (kh_vmsee_arm's skips; kh_wsee_mode_of) wants the capture, as the chain's own run does. KH_WSEE_BWANT:
// and the colours B / B2 where such a pass is a colour reader (kh_wsee_col_reader).
inline void kh_wsee_arm(ID3D11Device* dev, const std::vector<std::pair<uint64_t, RenderObject>>& khwa_fs) {
    if constexpr (KH_WSEE_ON) {   // The body whole: no code left unreachable with it out.
        for (const auto& khwa_f : khwa_fs) {
            const RenderObject& khwa_o = khwa_f.second;
            if (khwa_o.effect <= 0 || khwa_o.color[3] <= 0.001f || kh_wsee_mode_of(khwa_o) == 0) continue;
            if (khwa_o.effect == static_cast<int>(EffectId::DynLightFog) &&
                (khwa_o.blend_mode == 0 || khwa_o.blend_mode == 4 || khwa_o.blend_mode == 5) &&
                (!(khwa_o.fx[0] > 0.0f) || !kh_dlf_pool_live())) continue;
            if (khwa_o.effect == static_cast<int>(EffectId::Ssgi) && (!g_res.ps_ssgi_gather || !g_res.ps_ssgi_atrous)) {
                continue;
            }
            if (!kh_wsee_ensure(dev)) return;
            g_wsee_want_cycle.store(g_topo_cycles, std::memory_order_relaxed);
            for (const auto& khwa_g : khwa_fs) {   // KH_WSEE_BWANT.
                const RenderObject& khwa_r = khwa_g.second;
                if (khwa_r.effect > 0 && khwa_r.color[3] > 0.001f && kh_wsee_col_reader(khwa_r) &&
                    kh_wsee_mode_of(khwa_r) != 0) {
                    g_wsee_col_want_cycle.store(g_topo_cycles, std::memory_order_relaxed);
                    break;
                }
            }
            return;
        }
    }
}

// The world range the chain and the sort decode with (kh_fill_depth_range_cb's rule, read now: the trigger draw's
// viewport, else the scene's; a range that is not one, the scene's).
inline void kh_wsee_world_range(float& khwr_lo, float& khwr_hi) {
    khwr_lo = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
    khwr_hi = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
    if (!(khwr_lo >= 0.0f && khwr_hi <= 1.0f && khwr_lo < khwr_hi)) {
        khwr_lo = g_scene_vp_min_d;
        khwr_hi = g_scene_vp_max_d;
    }
}
// The far split's distance D: KH_WSEE_FAR_FRAC of the far plane of the engine's latched projection pair (m22, m32:
// ndc = m22 + m32 / d, far f = m32 / (1 - m22)), so D's ndc is m22 + (1 - m22) / KH_WSEE_FAR_FRAC - the pair alone.
// TWIN: cb.hlsl KhWseeFarRaw computes the same from depthParams.x. Its raw in the world range (lo, hi). False: no
// latched pair, or a value not inside the range (no Tf replay this cycle).
static constexpr float KH_WSEE_FAR_FRAC = 0.25f;
inline bool kh_wsee_far_raw(float& khfr_raw, float& khfr_lo, float& khfr_hi) {
    if (!g_ro.engine_proj_valid) return false;
    kh_wsee_world_range(khfr_lo, khfr_hi);
    const float khfr_m22 = g_ro.engine_m22;
    const float khfr_ndc = khfr_m22 + (1.0f - khfr_m22) / KH_WSEE_FAR_FRAC;
    khfr_raw = khfr_lo + khfr_ndc * (khfr_hi - khfr_lo);
    return std::isfinite(khfr_raw) && khfr_ndc > 0.0f && khfr_ndc < 1.0f;
}

// KH_PSB_ZB - the guard over Zb's premise: the one pass's loop bound is exact while the main depth's texture (the one
// depth_srv views) holds at every pixel a sample 0 no farther than S. Each path that could leave it farther after the
// capture trips Zb for the rest of the cycle (kh_psb_zb_trip; the loop is unbounded from there): a depth clear of a
// view of that texture that starts no new cycle (hooked_clear_depthstencil, after its cycle step); a draw on the
// engine's context while the bound depth state writes with GREATER / GREATER_EQUAL / NOT_EQUAL / ALWAYS and the bound
// depth view writes that texture (not READ_ONLY_DEPTH) - every draw entry (the four core hooks after
// reorder_pre_draw, DrawAuto, both indirect draws) looks at the live state while g_psb_zb_far says the bound state
// may; a CopyResource / CopySubresourceRegion(1) / ResolveSubresource / UpdateSubresource(1) into it; any
// ExecuteCommandList; depth_srv remade (depth_srv_gen). A LESS / LESS_EQUAL / EQUAL / NEVER write or a stencil-only
// write cannot (the depth plane is untouched). Zb is not built at a capture (as a trip) unless every one of those
// entries is hooked (g_rp_cbreuse_on: CopySubresourceRegion, ExecuteCommandList and the context-1 writers). Assumed
// absent, as the record's other rules assume: SwapDeviceContextState bringing in such a state, Discard* of the main
// depth followed by draws without a clear, a tiled main depth. Every check runs only while Zb stands; render thread,
// the engine's context alone.
inline bool kh_psb_zb_armed() {
    return g_psb_zb_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
}
inline void kh_psb_zb_trip() {
    g_psb_zb_cycle.store(~0ull, std::memory_order_relaxed);
}
// A depth state that can write the depth farther (null: the default, LESS).
inline bool kh_psb_zb_far_of(ID3D11DepthStencilState* khzf_s) {
    if (!khzf_s) return false;
    D3D11_DEPTH_STENCIL_DESC khzf_d = {};
    khzf_s->GetDesc(&khzf_d);
    return khzf_d.DepthEnable && khzf_d.DepthWriteMask != D3D11_DEPTH_WRITE_MASK_ZERO &&
           khzf_d.DepthFunc != D3D11_COMPARISON_LESS && khzf_d.DepthFunc != D3D11_COMPARISON_LESS_EQUAL &&
           khzf_d.DepthFunc != D3D11_COMPARISON_EQUAL && khzf_d.DepthFunc != D3D11_COMPARISON_NEVER;
}
// The resource is the main depth's texture (a texture first: GetType, no reference taken - the update hooks see
// every constant buffer).
inline bool kh_psb_zb_is_depth(ID3D11Resource* khzd_r) {
    if (!khzd_r || !g_res.depth_res_identity) return false;
    if (static_cast<void*>(khzd_r) == g_res.depth_res_identity) return true;
    D3D11_RESOURCE_DIMENSION khzd_k = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    khzd_r->GetType(&khzd_k);
    if (khzd_k != D3D11_RESOURCE_DIMENSION_TEXTURE2D) return false;
    ID3D11Texture2D* khzd_t = nullptr;
    if (FAILED(khzd_r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khzd_t))) || !khzd_t) {
        return false;
    }
    const bool khzd_m = static_cast<void*>(khzd_t) == g_res.depth_res_identity;
    khzd_t->Release();
    return khzd_m;
}
// The draw entries: the live depth state and view where the last bound state may write farther.
inline void kh_psb_zb_draw(ID3D11DeviceContext* ctx) {
    // The context first (deferred-context threads never touch the render thread's globals).
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed) || !g_psb_zb_far || !kh_psb_zb_armed()) return;
    ID3D11DepthStencilState* khzw_s = nullptr;
    UINT khzw_ref = 0u;
    ctx->OMGetDepthStencilState(&khzw_s, &khzw_ref);
    g_psb_zb_far = kh_psb_zb_far_of(khzw_s);
    KH_SAFE_RELEASE(khzw_s);
    if (!g_psb_zb_far) return;
    ID3D11DepthStencilView* khzw_v = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &khzw_v);
    bool khzw_w = false;
    if (khzw_v && g_res.depth_res_identity) {
        D3D11_DEPTH_STENCIL_VIEW_DESC khzw_d = {};
        khzw_v->GetDesc(&khzw_d);
        khzw_w = (khzw_d.Flags & D3D11_DSV_READ_ONLY_DEPTH) == 0u &&
                 reorder_dsv_identity(khzw_v) == g_res.depth_res_identity;
    }
    KH_SAFE_RELEASE(khzw_v);
    if (khzw_w) kh_psb_zb_trip();
}
// OMSetDepthStencilState (any caller on the engine's context): while Zb stands, whether the state set can write
// farther; otherwise unknown (true - the first draw after the next capture looks at the live state).
// KH_PSB_ZB_OWN: the re-issues' own fixed states take far_of's verdict without its GetDesc - none can write farther
// (dss_test LESS_EQUAL, dss_wsee_eq EQUAL and dss_wsee_far GREATER without a write, dss_wsee_nw LESS, dss_off depth
// off: their descs as made); held by g_res for the device's life, so no other object has their address.
inline void kh_psb_zb_dss(ID3D11DeviceContext* ctx, ID3D11DepthStencilState* khzs_s) {
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed)) return;
    // KH_PSB_ZB_DSS_ARMED: the five compares only while Zb stands; unarmed, the verdict is true (one store either way).
    if (!kh_psb_zb_armed()) {
        g_psb_zb_far = true;
        return;
    }
    const bool khzs_own = khzs_s && (khzs_s == g_res.dss_test || khzs_s == g_res.dss_off ||
                                     khzs_s == g_res.dss_wsee_nw || khzs_s == g_res.dss_wsee_eq ||
                                     khzs_s == g_res.dss_wsee_far);   // KH_PSB_ZB_OWN.
    g_psb_zb_far = !khzs_own && kh_psb_zb_far_of(khzs_s);
}
// A clear (after the hook's cycle step), a copy / resolve / update destination, a command list.
inline void kh_psb_zb_clear(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* khzc_v, UINT khzc_f) {
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed) || !khzc_v ||
        (khzc_f & D3D11_CLEAR_DEPTH) == 0u || !kh_psb_zb_armed()) return;
    if (g_res.depth_res_identity && reorder_dsv_identity(khzc_v) == g_res.depth_res_identity) {
        kh_psb_zb_trip();
    }
}
inline void kh_psb_zb_dst(ID3D11DeviceContext* ctx, ID3D11Resource* khzt_r) {
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed) || !khzt_r || !kh_psb_zb_armed()) return;
    if (kh_psb_zb_is_depth(khzt_r)) kh_psb_zb_trip();
}
inline void kh_psb_zb_list(ID3D11DeviceContext* ctx) {
    if (ctx == g_reorder_target_ctx.load(std::memory_order_relaxed) && kh_psb_zb_armed()) {
        kh_psb_zb_trip();
    }
}

// The capture (kh_vmsee_snapshot's pattern): sample 0 of the main depth into S (PSDepthS0), the scene target
// (khwn_rtv, the engine's RT0) resolved into B through the ORIGINAL resolve (the hook's render-thread work stays
// aside), T cleared (1, 0, 1, 1), NW / NW2 / Tp / Tf / P / N to 1, Tf's depth to D's raw, Zb drawn - everything
// under StateBackup (the engine's predicate set aside and handed back) and in_injection (the OM hooks do not take our
// binds for the engine's). Requires the bound depth = the texture depth_srv views and RT0 = the scene at its size and
// sample count; the targets are made or remade here (kh_wsee_targets). False = not this cycle. KH_WSEE_BWANT: B is
// resolved only with a colour want within KH_WSEE_WANT_SPAN cycles (g_wsee_col_want_cycle; one stamped earlier this
// cycle counts); else B is left as it was, g_wsee_col_cycle not stamped and no reader binds it this cycle.
inline bool kh_wsee_snapshot(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* khwn_rtv) {
    KH_PROF_TSC(KHP_WSEE_CAP);   // wseeCapture, timed whole (once a cycle; armed alone).
    if (!khwn_rtv || !g_res.ps_vmsee_s0 || !g_res.bs_vmsee_t || !g_res.dss_test || !g_res.depth_srv ||
        !g_res.ps_wsee_one || !g_res.dss_wsee_nw || !g_res.dss_wsee_eq ||   // The re-issues' (kh_wsee_ensure).
        !g_res.depth_dsv_ro || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off ||
        !g_orig_resolvesubresource) return false;
    const uint64_t khwn_cw = g_wsee_col_want_cycle.load(std::memory_order_relaxed);   // KH_WSEE_BWANT.
    const bool khwn_col = khwn_cw <= g_topo_cycles && g_topo_cycles - khwn_cw <= KH_WSEE_WANT_SPAN;
    const UINT khwn_sc = g_res.depth_sample_count;
    if (khwn_sc < 2 || g_res.ps_vmsee_samples != khwn_sc) return false;   // FSAA 1x: never (kh_wsee_ensure).
    {
        ID3D11DepthStencilView* khwn_dsv = nullptr;
        ctx->OMGetRenderTargets(0, nullptr, &khwn_dsv);
        if (!khwn_dsv) return false;
        const bool khwn_same = g_res.depth_res_identity != nullptr &&
                               reorder_dsv_identity(khwn_dsv) == g_res.depth_res_identity;
        khwn_dsv->Release();
        if (!khwn_same) return false;
    }
    D3D11_RENDER_TARGET_VIEW_DESC khwn_vd = {};
    khwn_rtv->GetDesc(&khwn_vd);
    // KH_WSEE: multisampled only, as B2, Q and C require - a TEXTURE2D view's texture has one sample and would fail
    // the count test below (and the targets' make ahead of it would have run at its format for nothing).
    if (khwn_vd.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2DMS) return false;
    ID3D11Texture2D* khwn_src = nullptr;
    {
        ID3D11Resource* khwn_r = nullptr;
        khwn_rtv->GetResource(&khwn_r);
        if (!khwn_r) return false;
        const HRESULT khwn_hr = khwn_r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khwn_src));
        khwn_r->Release();
        if (FAILED(khwn_hr) || !khwn_src) return false;
    }
    D3D11_TEXTURE2D_DESC khwn_sd = {};
    khwn_src->GetDesc(&khwn_sd);
    bool khwn_tg = false;
    {
        ID3D11Device* khwn_dev = nullptr;
        ctx->GetDevice(&khwn_dev);
        khwn_tg = kh_wsee_targets(khwn_dev, khwn_vd.Format);   // KH_VMSEE_LAZY.
        if (khwn_dev) khwn_dev->Release();
    }
    if (!khwn_tg || g_res.wsee_samples != khwn_sc || khwn_sd.Width != g_res.wsee_w || khwn_sd.Height != g_res.wsee_h ||
        khwn_sd.SampleDesc.Count != khwn_sc || khwn_sd.MipLevels != 1 || khwn_sd.ArraySize != 1) {
        khwn_src->Release();
        return false;
    }
    // The layer-depth split's targets (this cycle's verdict: the split's shaders not refused, wsee_ld_failed).
    bool khwn_ld = false;
    if (g_wsee_cyc_layer) {
        ID3D11Device* khwn_ldev = nullptr;
        ctx->GetDevice(&khwn_ldev);
        khwn_ld = kh_wsee_ld_ensure(khwn_ldev) && kh_wsee_ld_targets(khwn_ldev);
        if (khwn_ldev) khwn_ldev->Release();
    }
    float khwn_fr = 1.0f, khwn_flo = 0.0f, khwn_fhi = 1.0f;
    const bool khwn_far = khwn_ld && kh_wsee_far_raw(khwn_fr, khwn_flo, khwn_fhi);
    // KH_PSB: the one pass's record where a variant of it exists, at s = 1 << KH_PSB_LS.
    bool khwn_pb = false;
    if (g_psb_have) {
        ID3D11Device* khwn_pdev = nullptr;
        ctx->GetDevice(&khwn_pdev);
        khwn_pb = kh_psb_targets(khwn_pdev);
        if (khwn_pdev) khwn_pdev->Release();
    }
    // KH_PSB_ZB: Zb this cycle - the record made, Zb and its shader made; built only where every writer its guard
    // watches is hooked (else a trip).
    bool khwn_zb = false;
    if (khwn_pb && g_res.wsee_zb_rtv && g_res.wsee_zb_srv && g_res.wsee_s_srv && g_res.ps_psb_zb) {
        khwn_zb = g_rp_cbreuse_on.load(std::memory_order_relaxed) && g_orig_draw_auto &&
                  g_orig_draw_indexed_inst_indirect && g_orig_draw_inst_indirect;
        if (!khwn_zb) kh_psb_zb_trip();
    }
    // D3 stands this cycle (its views are made with S). Nothing is copied into it: a reader takes D3 only where a late
    // opaque owns sample 0, which this cycle's store or resolve wrote first.
    const bool khwn_od = g_res.wsee_d3_tex != nullptr && g_res.wsee_d3_uav != nullptr &&
                         g_res.wsee_d3_srv != nullptr;
    if (khwn_od) {   // KH_WSEE_OQ2: D3m with D3 (device Create* alone; refused or absent - the store path).
        ID3D11Device* khwn_mdev = nullptr;
        ctx->GetDevice(&khwn_mdev);
        kh_wsee_d3m_targets(khwn_mdev);
        if (khwn_mdev) khwn_mdev->Release();
    }
    {
        KhInjectionFlagScope khwn_fl;   // Ahead of the saves: it unwinds after their restore.
        g_ro.in_injection = true;
        KH_GPU_SCOPE(ctx, KHG_WSEE);   // KH_PROF: S's depth pass, the resolve, the clears.
        KhWseeGputScope khwn_gt(ctx, KHWG_CAPTURE);   // KH_WSEE_GPUT.
        StateBackup khwn_bk;
        khwn_bk.capture(ctx);
        KhOmSave khwn_om;
        khwn_om.capture(ctx);
        D3D11_VIEWPORT khwn_vp = {};
        khwn_vp.Width = static_cast<FLOAT>(g_res.wsee_w);
        khwn_vp.Height = static_cast<FLOAT>(g_res.wsee_h);
        khwn_vp.MaxDepth = 1.0f;
        // Replacing the OM targets unbinds the main depth's view first, which makes its SRV legal at t0.
        ctx->OMSetRenderTargets(1, &g_res.wsee_s_rtv, nullptr);
        ctx->RSSetViewports(1, &khwn_vp);
        ctx->RSSetState(g_res.rasterizer);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_vmsee_s0, nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &g_res.depth_srv);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        const FLOAT khwn_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(nullptr, khwn_bf, 0xFFFFFFFF);
        ctx->Draw(3, 0);
        ID3D11ShaderResourceView* khwn_null = nullptr;
        ctx->PSSetShaderResources(0, 1, &khwn_null);   // Off t0 before the engine's depth view is bound again.
        if (khwn_zb) {   // KH_PSB_ZB: Zb from the S just written, at the record's size (one fullscreen triangle).
            D3D11_VIEWPORT khwn_zv = {};
            khwn_zv.Width = static_cast<FLOAT>((g_res.wsee_w + (1u << KH_PSB_LS) - 1u) >> KH_PSB_LS);
            khwn_zv.Height = static_cast<FLOAT>((g_res.wsee_h + (1u << KH_PSB_LS) - 1u) >> KH_PSB_LS);
            khwn_zv.MaxDepth = 1.0f;
            ctx->OMSetRenderTargets(1, &g_res.wsee_zb_rtv, nullptr);   // S's target off: its view is legal at t0.
            ctx->RSSetViewports(1, &khwn_zv);
            ctx->PSSetShader(g_res.ps_psb_zb, nullptr, 0);
            ctx->PSSetShaderResources(0, 1, &g_res.wsee_s_srv);
            ctx->Draw(3, 0);
            ctx->PSSetShaderResources(0, 1, &khwn_null);
        }
        // Multisampled always (1x is refused above), so a resolve - never a copy. KH_WSEE_BWANT: where wanted.
        if (khwn_col) g_orig_resolvesubresource(ctx, g_res.wsee_b_tex, 0, khwn_src, 0, g_res.wsee_b_fmt);
        // T: R = 1 (nothing over), G = 0 (no late opaque), B = A = 1 (no particle-type layer, before or after B2).
        const FLOAT khwn_t0[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
        ctx->ClearRenderTargetView(g_res.wsee_t_rtv, khwn_t0);
        ctx->ClearDepthStencilView(g_res.wsee_n_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);   // NW: no layer yet.
        ctx->ClearDepthStencilView(g_res.wsee_n2_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);   // NWpost likewise.
        if (khwn_ld) {   // Tp and Tf: no layer yet; Tf's depth at D (none: Tf stays 1 this cycle).
            const FLOAT khwn_one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            ctx->ClearRenderTargetView(g_res.wsee_tp_rtv, khwn_one);
            ctx->ClearRenderTargetView(g_res.wsee_tf_rtv, khwn_one);
            ctx->ClearDepthStencilView(g_res.wsee_tfd_dsv, D3D11_CLEAR_DEPTH, khwn_far ? khwn_fr : 1.0f, 0);
        }
        if (khwn_pb) {   // KH_PSB: P0, P1, N - no layer yet (T's particle channels', Tp's, Tf's and NW's clears).
            const FLOAT khwn_pb1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            ctx->ClearRenderTargetView(g_res.wsee_p0_rtv, khwn_pb1);
            ctx->ClearRenderTargetView(g_res.wsee_p1_rtv, khwn_pb1);
            ctx->ClearRenderTargetView(g_res.wsee_nb_rtv, khwn_pb1);
            ctx->ClearRenderTargetView(g_res.wsee_nb2_rtv, khwn_pb1);   // KH_PSB_PHASE: N2.
        }
        khwn_om.restore(ctx);
        khwn_om.release();
        khwn_bk.restore(ctx);
    }
    khwn_src->Release();
    // The pinned depth view matched the identity this replaces (g_res.wsee_dsv_pin's note).
    if (g_res.wsee_depth_id != g_res.depth_res_identity) g_res.release_wsee_dsv_pin();
    g_res.wsee_depth_id = g_res.depth_res_identity;
    if (khwn_ld) {   // This cycle's split stands (kh_wsee_post, kh_wsee_tail, the chain's t63 / t64).
        g_wsee_ld_far_ok = khwn_far;
        g_wsee_ld_far_lo = khwn_flo;
        g_wsee_ld_far_hi = khwn_fhi;
        g_wsee_ld_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (khwn_pb) {   // KH_PSB: this cycle's one-pass record stands (kh_psb_on), D's raw for its Tf verdict.
        g_psb_far_raw = khwn_far ? khwn_fr : 1.0f;
        g_psb_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (khwn_zb) {   // KH_PSB_ZB: Zb stands this cycle (kh_psb_post) until its guard trips; the depth state unknown.
        g_psb_zb_gen = g_res.depth_srv_gen;
        g_psb_zb_far = true;
        g_psb_zb_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    } else {   // KH_PSB_ZB: none from this capture, nor an earlier one's this cycle (its targets may be remade).
        g_psb_zb_cycle.store(~0ull, std::memory_order_relaxed);
    }
    if (khwn_od) {   // D3 stands this cycle (the replays' stores, the chain's and the sort's t65).
        g_wsee_od_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (khwn_col) {   // KH_WSEE_BWANT: B stands this cycle (and B2 at its moment: kh_wsee_b2_take).
        g_wsee_col_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    return true;
}

// The B2 moment: at the cycle's first fence-type draw (a qualifying blended draw that writes depth) after the
// capture, before it. S2 = the main depth's sample 0 raw then (PSDepthS0 into S2, as the capture makes S: the depth
// of the surfaces B2 shows, the pre late opaques among them), B2 = the scene target khwb_rtv resolved through the
// ORIGINAL resolve, only in a cycle whose capture resolved B (g_wsee_col_cycle, KH_WSEE_BWANT; else B2 is left as it
// was). Particle-type layers after this moment are "post" (lane bit 3 says the split stands). S2's pass needs
// KH_VM_SEE's depth pass at the live count and the capture's depth bound; RT0 must be the scene texture in B's view
// format at the capture's size and sample count. Under StateBackup (the predicate set aside), KhOmSave and
// in_injection; S2's pass binds S2 alone first, so the main depth's view is off the OM before its SRV goes to t0.
// False = no B2 this cycle (every particle-type layer counts as pre); S2 may still be drawn (below).
inline bool kh_wsee_b2_take(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* khwb_rtv) {
    KH_PROF_TSC(KHP_WSEE_CAP);   // wseeCapture, timed whole (once a cycle; armed alone).
    if (!ctx) return false;
    const bool khwb_s2 = g_res.wsee_s2_rtv && g_res.ps_vmsee_s0 && g_res.depth_srv && g_res.vs_fullscreen &&
                         g_res.rasterizer && g_res.dss_off && g_res.ps_vmsee_samples == g_res.depth_sample_count &&
                         g_res.wsee_depth_id && g_res.wsee_depth_id == g_res.depth_res_identity;
    // KH_WSEE_NOPRE: a cycle that skipped a pre late opaque's replay needs S2 as the derivation's reference whatever
    // B2 does - S2's pass runs for it alone where B2 is refused (the readers bind S2 with lane bit 9); an S2 that
    // cannot be taken taints the cycle (a fence-type layer follows: the live depth is no reference any more).
    const bool khwb_np = g_wsee_nopre_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
    if (khwb_np && !khwb_s2) kh_wsee_taint(g_topo_cycles);
    // KH_WSEE_OQ: D3 stands complete once S2 is taken in such a cycle (kh_wsee_d3_ok) - where this cycle's D3 stands.
    const bool khwb_sync = khwb_np && khwb_s2 && g_res.wsee_d3_tex && g_res.wsee_s2_tex &&
                           g_wsee_od_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
    ID3D11Texture2D* khwb_src = nullptr;
    if (khwb_s2 && khwb_rtv && g_res.wsee_b2_tex && g_orig_resolvesubresource && g_topo_scene_tex_id) {
        D3D11_RENDER_TARGET_VIEW_DESC khwb_vd = {};
        khwb_rtv->GetDesc(&khwb_vd);
        if (khwb_vd.Format == g_res.wsee_b_fmt && khwb_vd.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS) {
            ID3D11Resource* khwb_r = nullptr;
            khwb_rtv->GetResource(&khwb_r);
            if (khwb_r) {
                if (static_cast<void*>(khwb_r) != g_topo_scene_tex_id ||
                    FAILED(khwb_r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khwb_src)))) {
                    khwb_src = nullptr;
                }
                khwb_r->Release();
            }
        }
    }
    bool khwb_ok = false;
    if (khwb_src) {
        D3D11_TEXTURE2D_DESC khwb_sd = {};
        khwb_src->GetDesc(&khwb_sd);
        khwb_ok = khwb_sd.Width == g_res.wsee_w && khwb_sd.Height == g_res.wsee_h &&
                  khwb_sd.SampleDesc.Count == g_res.wsee_samples && khwb_sd.MipLevels == 1 && khwb_sd.ArraySize == 1;
    }
    if (khwb_ok || (khwb_np && khwb_s2)) {   // khwb_sync implies the second term.
        KhInjectionFlagScope khwb_fl;   // Ahead of the saves: it unwinds after their restore.
        g_ro.in_injection = true;
        KH_GPU_SCOPE(ctx, KHG_WSEE_B2);   // KH_PROF: S2's depth pass and B2's resolve.
        KhWseeGputScope khwb_gt(ctx, KHWG_B2);   // KH_WSEE_GPUT.
        StateBackup khwb_bk;
        khwb_bk.capture(ctx);
        KhOmSave khwb_om;
        khwb_om.capture(ctx);
        // S2: kh_wsee_snapshot's depth pass.
        D3D11_VIEWPORT khwb_vp = {};
        khwb_vp.Width = static_cast<FLOAT>(g_res.wsee_w);
        khwb_vp.Height = static_cast<FLOAT>(g_res.wsee_h);
        khwb_vp.MaxDepth = 1.0f;
        ctx->OMSetRenderTargets(1, &g_res.wsee_s2_rtv, nullptr);
        ctx->RSSetViewports(1, &khwb_vp);
        ctx->RSSetState(g_res.rasterizer);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_vmsee_s0, nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &g_res.depth_srv);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        const FLOAT khwb_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(nullptr, khwb_bf, 0xFFFFFFFF);
        ctx->Draw(3, 0);
        ID3D11ShaderResourceView* khwb_null = nullptr;
        ctx->PSSetShaderResources(0, 1, &khwb_null);   // Off t0 before the engine's depth view is bound again.
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        if (khwb_sync) g_wsee_d3_sync_cycle.store(g_topo_cycles, std::memory_order_relaxed);   // KH_WSEE_OQ.
        g_wsee_s2_cycle.store(g_topo_cycles, std::memory_order_relaxed);   // KH_WSEE_NOPRE: S2 stands (bit 9).
        if (khwb_ok && g_wsee_col_cycle.load(std::memory_order_relaxed) == g_topo_cycles) {   // KH_WSEE_BWANT.
            g_orig_resolvesubresource(ctx, g_res.wsee_b2_tex, 0, khwb_src, 0, g_res.wsee_b_fmt);
        }
        khwb_om.restore(ctx);
        khwb_om.release();
        khwb_bk.restore(ctx);
    }
    if (khwb_src) khwb_src->Release();
    return khwb_ok;
}

// KH_WSEE_OQ2 - D3 from D3m, once before a reader binds D3 (the chain's t65, kh_fx_chain_run; the sort scope's,
// KhWseeSortScope): where a post late opaque wrote D3m in cycle khdr_c since the last resolve (g_wsee_d3m_cycle), one
// fullscreen PSWseeD3Res into D3's RTV reading the depth's MS view (t0), T (t1) and D3m (t2) - depth_resolve.hlsl's
// note: the store's value where nothing moved sample 0 since the replay, the opaque's own depth where something did;
// untouched where sample 0's last late opaque was a pre one, a store-path one or none. Nothing pending: true, nothing
// drawn. False = pending but not drawable (a view gone, the depth not the capture's): the caller leaves D3 unbound.
// Under StateBackup (the predicate set aside) and KhOmSave; in_injection is set on the hooks' render thread alone
// (KH_WSEE_D3M_RT: the hooks read it in their render-thread branches alone, which the game thread's late chain and a
// park never reach); D3 off t65 and the main depth's writable view off the OM before its view goes to t0. The
// reader's thread (render thread, a park that stalls it, or the game thread's late chain); at most two runs a cycle.
inline bool kh_wsee_d3m_resolve(ID3D11DeviceContext* ctx, uint64_t khdr_c) {
    if (g_wsee_d3m_cycle.load(std::memory_order_relaxed) != khdr_c) return true;
    if (!ctx || !g_res.ps_wsee_d3_res || !g_res.wsee_d3_rtv || !g_res.wsee_d3m_srv || !g_res.wsee_t_srv ||
        !g_res.depth_srv || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off || !g_res.wsee_depth_id ||
        g_res.wsee_depth_id != g_res.depth_res_identity || g_res.wsee_samples != g_res.depth_sample_count) {
        return false;
    }
    {
        KhInjectionFlagScope khdr_fl;   // Ahead of the saves: it unwinds after their restore.
        if (reorder_on_render_thread()) g_ro.in_injection = true;   // KH_WSEE_D3M_RT: render-thread state.
        KhWseeGputScope khdr_gt(ctx, KHWG_D3RES);   // KH_WSEE_GPUT.
        StateBackup khdr_bk;
        khdr_bk.capture(ctx);
        KhOmSave khdr_om;
        khdr_om.capture(ctx);
        ID3D11ShaderResourceView* const khdr_n[3] = { nullptr, nullptr, nullptr };
        ctx->PSSetShaderResources(65, 1, khdr_n);   // D3 off its readers' slot before it is a target.
        ctx->OMSetRenderTargets(1, &g_res.wsee_d3_rtv, nullptr);
        D3D11_VIEWPORT khdr_vp = {};
        khdr_vp.Width = static_cast<FLOAT>(g_res.wsee_w);
        khdr_vp.Height = static_cast<FLOAT>(g_res.wsee_h);
        khdr_vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &khdr_vp);
        ctx->RSSetState(g_res.rasterizer);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_wsee_d3_res, nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        ID3D11ShaderResourceView* const khdr_v[3] = { g_res.depth_srv, g_res.wsee_t_srv, g_res.wsee_d3m_srv };
        ctx->PSSetShaderResources(0, 3, khdr_v);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        const FLOAT khdr_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(nullptr, khdr_bf, 0xFFFFFFFF);
        ctx->Draw(3, 0);
        ctx->PSSetShaderResources(0, 3, khdr_n);   // Off t0 - t2 before the engine's depth view is bound again.
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        khdr_om.restore(ctx);
        khdr_om.release();
        khdr_bk.restore(ctx);
    }
    g_wsee_d3m_cycle.store(~0ull, std::memory_order_relaxed);
    return true;
}

// Whether the bound depth view khwd_dsv is the capture's depth (its texture wsee_depth_id): the view that last
// matched is kept by one reference (g_res.wsee_dsv_pin), so the same pointer with the same identity is a match
// without a call (a view's texture never changes, a held address is no other's); anything else takes
// reorder_dsv_identity's path (GetResource / QueryInterface / Release) and a match replaces the pin. The caller tests
// a null view first: with no pin and no record standing null would equal both. Render thread.
inline bool kh_wsee_dsv_is_depth(ID3D11DepthStencilView* khwd_dsv) {
    if (khwd_dsv == g_res.wsee_dsv_pin && g_res.wsee_dsv_pin_id == g_res.wsee_depth_id) return true;
    void* const khwd_id = reorder_dsv_identity(khwd_dsv);
    if (khwd_id != g_res.wsee_depth_id) return false;
    if (khwd_id) {
        if (khwd_dsv != g_res.wsee_dsv_pin) {
            KH_SAFE_RELEASE(g_res.wsee_dsv_pin);
            khwd_dsv->AddRef();
            g_res.wsee_dsv_pin = khwd_dsv;
        }
        g_res.wsee_dsv_pin_id = khwd_id;
    }
    return true;
}

// The late opaques a cycle replays (kh_wsee_mark_opaque): past KH_WSEE_OPAQUE_CAP the cycle is tainted - a capture
// that landed before most of the world would otherwise pay the world's vertex work again; the back-off then bounds
// it. The cap bounds work alone (a replay is drawn at its own hook's post, nothing stored). A pre mark is not
// replayed and does not count (KH_WSEE_NOPRE). Render thread.
static constexpr uint32_t KH_WSEE_OPAQUE_CAP = 4096u;
static uint32_t g_wsee_opq_n = 0;
static uint64_t g_wsee_opq_cycle = ~0ull;
// The capture gate's view of the injection (kh_wsee_pre_draw): whether the last injection found composite-eligible
// meshes (inject_composited_meshes' khr_elig_n; stamped there, one cycle behind at most).
static bool     g_wsee_inj_elig = false;
inline void kh_wsee_mark_opaque(uint64_t khwm_c) {
    if (g_wsee_opq_cycle != khwm_c) {
        g_wsee_opq_cycle = khwm_c;
        g_wsee_opq_n = 0;
    }
    // Its phase: "pre" before the cycle's B2 attempt (B2 and S2 then hold this surface's colour and depth), else
    // "post" (G = 1). KH_WSEE_NOPRE: a pre one is not replayed - the shaders derive its G, behind and reset from S2
    // against S (cb.hlsl's note at khWsS2); kh_wsee_b2_take takes S2 for it, g_wsee_nopre_cycle stamps the cycle (lane
    // bit 8; D3's verdict too, kh_wsee_d3_ok). The record must still be this depth's (else the replay's taint). A pre
    // mark in a cycle that then takes no B2 is still derived against the live depth and reads G = 1 (bit 3 clear).
    const bool khwm_post = g_wsee_b2_try == khwm_c;
    if (!khwm_post) {
        g_wsee_pending = KH_WSEE_RE_NONE;
        g_wsee_pending_post = false;
        if (!g_res.wsee_depth_id || g_res.wsee_depth_id != g_res.depth_res_identity ||
            g_res.wsee_samples != g_res.depth_sample_count || !g_res.wsee_t_rtv) {
            kh_wsee_taint(khwm_c);
            return;
        }
        g_wsee_nopre_cycle.store(khwm_c, std::memory_order_relaxed);
        return;
    }
    ++g_wsee_opq_n;
    if (g_wsee_opq_n > KH_WSEE_OPAQUE_CAP) {
        kh_wsee_taint(khwm_c);   // Once a cycle: the taint stands the rest of the cycle's draws down.
        return;
    }
    g_wsee_pending = KH_WSEE_RE_OPAQUE;
    g_wsee_pending_post = true;
}

// reorder_pre_draw's call, wanted cycles only (render thread; a tracked main-depth draw of the engine, outside the
// flush and the view-model slice - the slice's draws lie below the world's range; nothing once the chain has drawn
// the cycle, g_wsee_used_cycle). Depth verdicts are the DSS hook's (dss_nowrite: tested, not written; dss_write:
// DepthEnable, mask ALL; the default state writes). A blended draw qualifies when its depth state writes or tests
// without writing, RT0 blends over (ADD, DestBlend INV_SRC_ALPHA, SrcBlend ONE / SRC_ALPHA / ZERO, colour written,
// sample mask != 0, no alpha-to-coverage) and RT0 is the scene texture (KH_SCENE_NAME's; none named = nothing). The
// cycle's first qualifying draw takes the capture once the world pass has begun (the trigger's
// tests: kh_reorder_world_vp, and the injection landed or kh_reorder_min_opaques passed) and no injection can still
// come (the gate below; else the capture lands at the injection's tail); one before that neither captures nor
// counts. While the capture stands untainted: a qualifying draw is marked for its re-issue (kh_wsee_post: T, and NW
// for a no-write one); any other draw that really writes depth is marked as a late opaque (kh_wsee_mark_opaque: pre /
// post by the B2 attempt); an unblended draw writing the scene's colour with the depth test off taints. A blended
// draw writing no depth on the main depth into the scene texture, not testing stencil, that does not qualify is
// still re-issued (the block below): an over-blend with the test off into T untested and NW (RE_NOWRITE_OFF), a
// light-adding one (DestBlend ONE, a source factor that reads no destination), tested or not, into NW alone
// (RE_NWONLY). Any other blended draw with the test off, or one that tests without writing and does not qualify, is
// neither.
// The state is read only as far as the flags leave it open; a tainted or refused cycle reads no blended draw's.
inline void kh_wsee_pre_draw(ID3D11DeviceContext* ctx) {
    const uint64_t khwp_c = g_topo_cycles;
    if (g_wsee_used_cycle.load(std::memory_order_relaxed) == khwp_c) return;   // The chain has drawn.
    const bool khwp_landed = g_wsee_ok_cycle.load(std::memory_order_relaxed) == khwp_c;
    const bool khwp_tainted = g_wsee_taint_cycle.load(std::memory_order_relaxed) == khwp_c;
    const bool khwp_write = g_ro.dss_write;
    // The bound viewports, the whole array once (g_wsee_pd_vp's note: the post takes it); the tests below read entry
    // 0 and the count.
    g_wsee_pd_vp_ok = false;
    const UINT& khwp_n = g_wsee_pd_vpn;
    const D3D11_VIEWPORT& khwp_vp = g_wsee_pd_vp[0];
    if (!g_ro.blend_translucent) {
        if (!khwp_landed || g_ro.dss_nowrite) return;
        // KH_WSEE_TAINT_EARLY: a tainted cycle marks no late opaque and taints no further - nothing below changes
        // anything on it, so its four state reads a draw are skipped.
        if (khwp_tainted) return;
        kh_wsee_pd_vp_read(ctx);
        if (khwp_n < 1 || kh_infront_vp_is_slice(khwp_vp)) return;
        if (khwp_write) {
            kh_wsee_mark_opaque(khwp_c);
            return;
        }
        {   // The depth test off: does it write colour (RT0's mask, the sample mask)?
            ID3D11BlendState* khwp_cbs = nullptr;
            FLOAT khwp_cbf[4] = {};
            UINT khwp_cmask = 0;
            ctx->OMGetBlendState(&khwp_cbs, khwp_cbf, &khwp_cmask);
            UINT khwp_wm = D3D11_COLOR_WRITE_ENABLE_ALL;   // Null: the default state, which writes all.
            if (khwp_cbs) {
                D3D11_BLEND_DESC khwp_cbd = {};
                khwp_cbs->GetDesc(&khwp_cbd);
                khwp_cbs->Release();
                khwp_wm = khwp_cbd.RenderTarget[0].RenderTargetWriteMask;
            }
            if ((khwp_wm & 7u) == 0 || khwp_cmask == 0) return;
        }
        ID3D11RenderTargetView* khwp_crtv = nullptr;
        ctx->OMGetRenderTargets(1, &khwp_crtv, nullptr);
        if (!khwp_crtv) return;
        ID3D11Resource* khwp_cres = nullptr;
        khwp_crtv->GetResource(&khwp_cres);
        khwp_crtv->Release();
        const bool khwp_cscene = khwp_cres != nullptr && g_topo_scene_tex_id != nullptr &&
                                 static_cast<void*>(khwp_cres) == g_topo_scene_tex_id;
        if (khwp_cres) khwp_cres->Release();
        if (!khwp_cscene) return;
        kh_wsee_taint(khwp_c);
        return;
    }
    if (khwp_tainted) return;
    if (!khwp_landed && g_wsee_snap_cycle == khwp_c) return;   // Attempted and refused: not this cycle.
    // The depth test off: no write and no capture; once the capture stands, a layer still (below).
    const bool khwp_test = khwp_write || g_ro.dss_nowrite;
    if (!khwp_test && !khwp_landed) return;
    kh_wsee_pd_vp_read(ctx);
    if (khwp_n < 1 || kh_infront_vp_is_slice(khwp_vp)) return;
    bool khwp_over = false;
    bool khwp_add = false;   // Light-adding: dest x ONE plus a source term that reads no dest.
    {
        ID3D11BlendState* khwp_bs = nullptr;
        FLOAT khwp_bf[4] = {};
        UINT khwp_mask = 0;
        ctx->OMGetBlendState(&khwp_bs, khwp_bf, &khwp_mask);
        if (khwp_bs) {   // Null is the default state, opaque (blend_translucent says otherwise only for a bound one).
            D3D11_BLEND_DESC khwp_bd = {};
            khwp_bs->GetDesc(&khwp_bd);
            khwp_bs->Release();
            const D3D11_RENDER_TARGET_BLEND_DESC& khwp_r = khwp_bd.RenderTarget[0];
            khwp_over = khwp_r.BlendEnable && khwp_r.BlendOp == D3D11_BLEND_OP_ADD &&
                        khwp_r.DestBlend == D3D11_BLEND_INV_SRC_ALPHA &&
                        (khwp_r.SrcBlend == D3D11_BLEND_ONE || khwp_r.SrcBlend == D3D11_BLEND_SRC_ALPHA ||
                         khwp_r.SrcBlend == D3D11_BLEND_ZERO) &&
                        (khwp_r.RenderTargetWriteMask & 7u) != 0 && khwp_mask != 0 &&
                        !khwp_bd.AlphaToCoverageEnable;   // A2C writes depth at covered samples alone.
            // The source factor must not read the destination (screen's INV_DEST_COLOR scales what lies behind by
            // a factor T does not hold).
            const D3D11_BLEND khwp_sb = khwp_r.SrcBlend;
            const bool khwp_sd = khwp_sb == D3D11_BLEND_DEST_COLOR || khwp_sb == D3D11_BLEND_INV_DEST_COLOR ||
                                 khwp_sb == D3D11_BLEND_DEST_ALPHA || khwp_sb == D3D11_BLEND_INV_DEST_ALPHA ||
                                 khwp_sb == D3D11_BLEND_SRC_ALPHA_SAT;
            khwp_add = khwp_r.BlendEnable && khwp_r.BlendOp == D3D11_BLEND_OP_ADD && !khwp_sd &&
                       khwp_r.DestBlend == D3D11_BLEND_ONE && (khwp_r.RenderTargetWriteMask & 7u) != 0 &&
                       khwp_mask != 0 && !khwp_bd.AlphaToCoverageEnable;
        }
    }
    ID3D11RenderTargetView* khwp_rtv = nullptr;
    bool khwp_scene = false;
    if (khwp_over || khwp_add) {
        ctx->OMGetRenderTargets(1, &khwp_rtv, nullptr);
        if (khwp_rtv) {
            ID3D11Resource* khwp_res = nullptr;
            khwp_rtv->GetResource(&khwp_res);
            khwp_scene = khwp_res != nullptr && g_topo_scene_tex_id != nullptr &&
                         static_cast<void*>(khwp_res) == g_topo_scene_tex_id;
            if (khwp_res) khwp_res->Release();
        }
    }
    // A blended draw writing no depth over the capture's depth, once the capture stands: an over-blend drawn with
    // its depth test off goes into T untested (it covered every fragment it drew) and NW; a light-adding one
    // (DestBlend ONE, a source factor that reads no destination), tested or not, into NW alone - its light is in the
    // scene's colour beyond T B, so its depth is a front and T stays the over-blends' product. Multiply, screen,
    // subtractive and MIN / MAX stay out: a darkening layer at the front would lose its darkening in the fog (E = S -
    // T B' is held at 0 there), and a term that reads the destination scales what lies behind by a factor T does not
    // hold.
    if (!khwp_write && khwp_scene && khwp_landed && (!khwp_test || !khwp_over)) {
        bool khwp_dz = false;
        {
            ID3D11DepthStencilView* khwp_dsv = nullptr;
            ctx->OMGetRenderTargets(0, nullptr, &khwp_dsv);
            khwp_dz = khwp_dsv != nullptr && kh_wsee_dsv_is_depth(khwp_dsv);
            if (khwp_dsv) khwp_dsv->Release();
        }
        if (khwp_rtv) khwp_rtv->Release();
        if (!khwp_dz) return;   // Not on the main depth: no layer of the world's picture.
        if (!khwp_over && !khwp_add) return;
        if (g_ro.dss_stencil) return;   // NW cannot carry its stencil mask: not recorded.
        g_wsee_pending = khwp_over ? KH_WSEE_RE_NOWRITE_OFF : KH_WSEE_RE_NWONLY;
        g_wsee_pending_post = g_wsee_b2_cycle.load(std::memory_order_relaxed) == khwp_c;   // Its phase.
        return;
    }
    if (!khwp_test || !khwp_over || !khwp_scene) {
        if (khwp_rtv) khwp_rtv->Release();
        // A depth write that is no qualifying layer is an opaque surface to the chain.
        if (khwp_landed && khwp_write) kh_wsee_mark_opaque(khwp_c);
        return;
    }
    if (!khwp_landed) {
        // The world pass has begun (the trigger's own tests), or this draw neither captures nor counts (a far
        // partition's clouds, a blended draw ahead of the opaque wall).
        const bool khwp_world = kh_reorder_world_vp(khwp_vp) &&
                                (g_ro.injected || g_ro.opaque_draws >= kh_reorder_min_opaques());
        // Not while this cycle's injection can still come: none landed, attempts left (kh_reorder_trigger's cap), the
        // last injection found eligible meshes (g_wsee_inj_elig), the cycle is not anomalous (inject_composited_meshes'
        // khr_anomalous: that injection re-arms or rescues outside the world's viewport, and its tail takes no
        // capture) and the path is alive (composite_path_healthy; the clock read only once the other terms hold). Such
        // a draw passes uncaptured and the capture lands at the injection's tail (kh_wsee_inj_capture, before its
        // deferral), with our solids in S and B - a capture here would be tainted by the injection's depth write. A
        // cycle no injection can reach captures here.
        const bool khwp_inj_due = !g_ro.injected && g_ro.inject_attempts < KH_REORDER_MAX_INJECT_ATTEMPTS &&
                                  g_wsee_inj_elig && !(g_proj_locator_ever && g_ro.slot_near_live <= 0.0f) &&
                                  composite_path_healthy();
        if (khwp_world && !khwp_inj_due) {
            g_wsee_snap_cycle = khwp_c;
            if (kh_wsee_snapshot(ctx, khwp_rtv)) g_wsee_ok_cycle.store(khwp_c, std::memory_order_relaxed);
        }
    }
    // The cycle's first fence-type layer (a qualifying blended draw that writes depth) once the capture stands: B2,
    // the scene it is drawn over, taken before it (once a cycle; refused, the cycle has none).
    if (khwp_write && g_wsee_ok_cycle.load(std::memory_order_relaxed) == khwp_c && g_wsee_b2_try != khwp_c) {
        g_wsee_b2_try = khwp_c;
        if (kh_wsee_b2_take(ctx, khwp_rtv)) {
            g_wsee_b2_cycle.store(khwp_c, std::memory_order_relaxed);
        }
    }
    khwp_rtv->Release();
    if (g_wsee_ok_cycle.load(std::memory_order_relaxed) != khwp_c) return;
    // A no-write layer testing stencil is not re-issued: its NW pass cannot carry the mask (a depth writer's EQUAL
    // passes select its own samples whatever its stencil).
    if (!khwp_write && g_ro.dss_stencil) return;
    g_wsee_pending = khwp_write ? KH_WSEE_RE_BLEND : KH_WSEE_RE_NOWRITE;
    // A particle-type layer's phase: drawn after this cycle's B2 (Tpost / NWpost) or before it (Tpre / NWpre).
    g_wsee_pending_post = g_wsee_b2_cycle.load(std::memory_order_relaxed) == khwp_c;
}

inline void kh_wsee_sv_release() {
    for (ID3D11RenderTargetView*& khwr_r : g_wsee_sv_rtvs) KH_SAFE_RELEASE(khwr_r);
    KH_SAFE_RELEASE(g_wsee_sv_dsv);
    KH_SAFE_RELEASE(g_wsee_sv_bs);
    KH_SAFE_RELEASE(g_wsee_sv_dss);
    KH_SAFE_RELEASE(g_wsee_sv_ps);
    g_wsee_sv_has_ps = false;
    KH_SAFE_RELEASE(g_wsee_sv_srv0);
    g_wsee_sv_has_srv0 = false;
    g_wsee_sv_d3m = false;   // KH_WSEE_OQ2.
}

// The re-issues' passes (kh_wsee_re_set's khwb_pass; TWIN of its binds), all under in_injection:
//   PASS_T      T over depth_dsv_ro, dss_test (LESS_EQUAL, no write), bs_wsee_tpre / tpost (T.B or .A *= 1 - a: the
//               particle-type layer's phase), the engine's sample mask (T is multisampled).
//   PASS_NW     no RTV, NW's (pre / post) DSV, dss_wsee_nw (LESS, write), no blend state, every sample (NW has one).
//   PASS_ONE    PSWseeOneZ / OneZL (or PSWseeOne / OneL on the store path; system values alone, so it links with any
//               vertex stage) over depth_dsv_ro, dss_wsee_eq (EQUAL, no write), no blend state (it writes
//               (1, 1, 1, 1)), the engine's sample mask; the store path binds the targets through
//               OMSetRenderTargetsAndUnorderedAccessViews with D3's UAV at u3 and the depth's MS view at PS t0 (legal
//               beside the read-only DSV, bound first); the restore's OMSetRenderTargets unbinds the UAV.
//   PASS_T_OFF  PASS_T with dss_off (an over-blend the engine drew untested: it covered every fragment it drew).
//   PASS_T_EQ   T.R *= 1 - a under dss_wsee_eq and bs_vmsee_t (a blended draw that writes depth): EQUAL passes exactly
//               the samples it wrote, its stencil or discard included, so nothing of its stencil is replayed; a draw
//               overlapping itself counts its nearest fragment a sample alone.
static constexpr int KH_WSEE_PASS_T = 0, KH_WSEE_PASS_NW = 1, KH_WSEE_PASS_ONE = 2, KH_WSEE_PASS_T_OFF = 3,
                     KH_WSEE_PASS_T_EQ = 4;
//   PASS_TP / TP_OFF   Tp.R|G under dss_test / dss_off (the caller pushes the viewports by KH_WSEE_NEAR_TOL),
//               bs_wsee_ldpre / ldpost (T's particle factors: a fragment Tp and T both take is multiplied alike).
//   PASS_TF     Tf over wsee_tfd_dsv (cleared to D's raw), dss_wsee_far (GREATER, no write), every sample.
static constexpr int KH_WSEE_PASS_TP = 5, KH_WSEE_PASS_TP_OFF = 6, KH_WSEE_PASS_TF = 7;
//   PASS_TP2 / TP2_OFF   the Tp passes again into Tp.B|A (bs_wsee_ldpre2 / ldpost2) at KH_WSEE_NEAR_TOL2.
static constexpr int KH_WSEE_PASS_TP2 = 8, KH_WSEE_PASS_TP2_OFF = 9;
// KH_WSEE_GPUT: a pass's stamp kind - the particle-type passes' own; none for the one-pass kinds (a late opaque, a
// fence-type layer: their block's interval alone); any other pass its own catch-all.
inline uint32_t kh_wsee_gput_pk(int khgk_pass) {
    if (khgk_pass == KH_WSEE_PASS_ONE || khgk_pass == KH_WSEE_PASS_T_EQ) return KHWG_N;
    if (khgk_pass == KH_WSEE_PASS_T || khgk_pass == KH_WSEE_PASS_T_OFF) return KHWG_P_T;
    if (khgk_pass == KH_WSEE_PASS_NW) return KHWG_P_NW;
    if (khgk_pass == KH_WSEE_PASS_TP || khgk_pass == KH_WSEE_PASS_TP_OFF) return KHWG_P_TP;
    if (khgk_pass == KH_WSEE_PASS_TP2 || khgk_pass == KH_WSEE_PASS_TP2_OFF) return KHWG_P_TP2;
    if (khgk_pass == KH_WSEE_PASS_TF) return KHWG_P_TF;
    return KHWG_P_OTHER;
}
// This cycle's split stands (the capture cleared Tp / Tf: g_wsee_ld_cycle) with its states (render thread).
inline bool kh_wsee_ld_on() {
    return g_wsee_cyc_layer && g_wsee_ld_cycle.load(std::memory_order_relaxed) == g_topo_cycles &&
           g_res.wsee_tp_rtv && g_res.wsee_tf_rtv && g_res.wsee_tfd_dsv && g_res.bs_wsee_ldpre &&
           g_res.bs_wsee_ldpost && g_res.dss_wsee_far && g_res.ps_wsee_one_l &&
           g_res.bs_wsee_ldpre2 && g_res.bs_wsee_ldpost2 &&
           g_res.depth_dsv_ro && g_res.dss_test && g_res.dss_off;
}
// This cycle's D3 stands (stamped by the capture: g_wsee_od_cycle) with the views the late-opaque pass binds - D3's
// UAV (u3) and the main depth's multisampled view (PS t0, read beside depth_dsv_ro). Render thread (the re-issues)
// and the flush's (the sort scope: the render thread, or a park that stalls it).
inline bool kh_wsee_od_on() {
    return g_wsee_od_cycle.load(std::memory_order_relaxed) == g_topo_cycles &&
           g_res.wsee_d3_uav && g_res.wsee_d3_srv && g_res.depth_srv;
}
// KH_WSEE_OQ2: whether a post late opaque's replay takes the D3m path (no UAV, no depth view): D3m and its shaders
// stand, the engine's rasterizer neither biases depth nor turns depth clip off (with either the PS's interpolated z
// need not be the depth the engine wrote - D3D11 leaves it to the hardware), and the draw rasterizes solid triangles
// (KH_WSEE_PXZ: the twins write the depth plane at sample 0 from the centre's z and derivatives, a plane over a
// triangle alone - a line / point / patch list, a bound GS or a wireframe fill takes the store path). The state is
// read once a post replay (RSGetState), its desc only when it is not the pinned one (g_res.wsee_rs_pin: one
// reference, so a reused address cannot pass for it; states are immutable). False: the store path. Render thread
// (kh_wsee_re_save); one IAGetPrimitiveTopology and one GSGetShader a post replay.
inline bool kh_wsee_d3m_path(ID3D11DeviceContext* ctx) {
    if (!g_res.wsee_d3m_rtv || !g_res.wsee_d3m_srv || !g_res.wsee_d3_rtv || !g_res.ps_wsee_one_z ||
        !g_res.ps_wsee_one_zl || !g_res.ps_wsee_d3_res) return false;
    D3D11_PRIMITIVE_TOPOLOGY khdp_tp = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ctx->IAGetPrimitiveTopology(&khdp_tp);
    ID3D11GeometryShader* khdp_gs = nullptr;
    ctx->GSGetShader(&khdp_gs, nullptr, nullptr);
    const bool khdp_gsb = khdp_gs != nullptr;
    KH_SAFE_RELEASE(khdp_gs);
    if (khdp_gsb ||
        (khdp_tp != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST && khdp_tp != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP &&
         khdp_tp != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ &&
         khdp_tp != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ)) {
        return false;
    }
    ID3D11RasterizerState* khdp_rs = nullptr;
    ctx->RSGetState(&khdp_rs);
    bool khdp_plain = true;   // Null: the default state - no bias, depth clip on, solid fill.
    if (khdp_rs) {
        if (khdp_rs == g_res.wsee_rs_pin) {
            khdp_plain = g_res.wsee_rs_pin_plain;
            khdp_rs->Release();
        } else {
            D3D11_RASTERIZER_DESC khdp_d = {};
            khdp_rs->GetDesc(&khdp_d);
            khdp_plain = khdp_d.DepthBias == 0 && khdp_d.SlopeScaledDepthBias == 0.0f &&
                         khdp_d.DepthClipEnable != FALSE &&
                         khdp_d.FillMode == D3D11_FILL_SOLID;   // KH_WSEE_PXZ: no wireframe (lines: no plane).
            KH_SAFE_RELEASE(g_res.wsee_rs_pin);
            g_res.wsee_rs_pin = khdp_rs;   // The reference RSGetState took is the pin's.
            g_res.wsee_rs_pin_plain = khdp_plain;
        }
    }
    return khdp_plain;
}

// Whether this cycle's targets and states for a re-issue of kind khwy_k stand, made against the depth bound now.
inline bool kh_wsee_re_ready(uint8_t khwy_k) {
    if (!g_res.wsee_depth_id || g_res.wsee_depth_id != g_res.depth_res_identity ||
        g_res.wsee_samples != g_res.depth_sample_count || !g_res.wsee_t_rtv || !g_res.depth_dsv_ro) return false;
    if (khwy_k == KH_WSEE_RE_OPAQUE) {   // A post one always (no pre replay).
        return g_res.dss_wsee_eq && g_res.ps_wsee_one;
    }
    if (khwy_k == KH_WSEE_RE_BLEND && !g_res.dss_wsee_eq) return false;   // Its T pass is EQUAL.
    // A particle-type kind also needs NWpost and the Tpre / Tpost states.
    if (khwy_k != KH_WSEE_RE_BLEND && (!g_res.wsee_n2_dsv || !g_res.bs_wsee_tpre || !g_res.bs_wsee_tpost)) return false;
    if (khwy_k == KH_WSEE_RE_NWONLY) return g_res.wsee_n_dsv && g_res.dss_wsee_nw;
    if (!g_res.dss_test || !g_res.bs_vmsee_t) return false;
    if (khwy_k == KH_WSEE_RE_NOWRITE_OFF && !g_res.dss_off) return false;
    return (khwy_k != KH_WSEE_RE_NOWRITE && khwy_k != KH_WSEE_RE_NOWRITE_OFF) ||
           (g_res.wsee_n_dsv && g_res.dss_wsee_nw);
}

// A re-issue's save, once a post (render thread, kh_wsee_post) whatever its passes: the RTVs and DSV, the blend state
// with its factor and sample mask, the depth state with its reference, and with khws_ps (a late opaque) the pixel
// shader and PS t0 (the store path binds the depth's view there). Probed once: the bound depth must be wsee_depth_id's
// texture; refused (false, nothing kept) with a UAV or stream-output target bound (the second draw would write them
// again) or the PS bound with class instances (set back without them it would not be the engine's); the caller
// taints. kh_wsee_re_restore sets back exactly this; scissor, rasterizer, other shaders, constants, views, input and
// predicate are not touched by the passes (the viewports are shifted and set back by KhWseeSurfVp itself; KH_PSB's
// one pass, kh_psb_post, sets the viewports, scissor rects, its constant-buffer and view slots and puts them back).
inline bool kh_wsee_re_save(ID3D11DeviceContext* ctx, bool khws_ps) {
    ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_wsee_sv_rtvs, &g_wsee_sv_dsv);
    bool khws_refuse = !g_wsee_sv_dsv || !kh_wsee_dsv_is_depth(g_wsee_sv_dsv);
    if (!khws_refuse) {
        ID3D11UnorderedAccessView* khws_uavs[8] = {};
        ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 8, khws_uavs);   // UAV probe only.
        for (ID3D11UnorderedAccessView*& khws_u : khws_uavs) {
            if (khws_u) khws_refuse = true;
            KH_SAFE_RELEASE(khws_u);
        }
    }
    if (!khws_refuse) {
        ID3D11Buffer* khws_so = nullptr;
        ctx->SOGetTargets(1, &khws_so);
        if (khws_so) {
            khws_refuse = true;
            khws_so->Release();
        }
    }
    if (!khws_refuse && khws_ps) {
        ID3D11ClassInstance* khws_ci[8] = {};
        UINT khws_cn = 8;
        ctx->PSGetShader(&g_wsee_sv_ps, khws_ci, &khws_cn);
        g_wsee_sv_has_ps = true;
        if (khws_cn > 0) khws_refuse = true;
        for (ID3D11ClassInstance*& khws_c : khws_ci) {
            if (khws_c) khws_refuse = true;
            KH_SAFE_RELEASE(khws_c);
        }
        // The store path binds the main depth's view at t0; the D3m path binds neither (KH_WSEE_OQ2).
        if (!khws_refuse && kh_wsee_od_on()) {
            g_wsee_sv_d3m = kh_wsee_d3m_path(ctx);
            if (!g_wsee_sv_d3m) {
                ctx->PSGetShaderResources(0, 1, &g_wsee_sv_srv0);
                g_wsee_sv_has_srv0 = true;
            }
        }
    }
    if (khws_refuse) {
        kh_wsee_sv_release();
        return false;
    }
    ctx->OMGetBlendState(&g_wsee_sv_bs, g_wsee_sv_bf, &g_wsee_sv_mask);
    ctx->OMGetDepthStencilState(&g_wsee_sv_dss, &g_wsee_sv_ref);
    kh_wsee_gput_begin(ctx, KHWG_N);   // KH_WSEE_GPUT: the re-issue's block (its kind from its first pass).
    return true;
}

// One pass's state (after kh_wsee_re_save; a post's passes follow each other without a restore between). Under
// in_injection, so the OM hooks do not take ours for the engine's. khwb_post picks the phase's target / channel
// (NWpost, T.A; else NWpre, T.B); the fence-type pass (PASS_T_EQ) keeps T.R. khwb_pv (KH_PSV): the pass binds the
// engine shader's variant - nw (null: no pixel shader) for NW, alpha for every other pass; off or absent, the engine's
// shader as bound (kh_wsee_re_restore sets it back where one was set here).
inline void kh_wsee_re_set(ID3D11DeviceContext* ctx, int khwb_pass, bool khwb_post = false,
                           const KhPsvPick* khwb_pv = nullptr) {
    KhInjectionFlagScope khwb_fl;
    g_ro.in_injection = true;
    if (g_wsee_gput.blk >= 0) {   // KH_WSEE_GPUT: the block's kind from its first pass; a stamp before a pass's own.
        kh_wsee_gput_pass(ctx, khwb_pass == KH_WSEE_PASS_ONE ? KHWG_OPQ_POST
                               : khwb_pass == KH_WSEE_PASS_T_EQ ? KHWG_FENCE : KHWG_PART, kh_wsee_gput_pk(khwb_pass));
    }
    if (khwb_pv && khwb_pv->on) {
        ctx->PSSetShader(khwb_pass == KH_WSEE_PASS_NW ? khwb_pv->nw : khwb_pv->alpha, nullptr, 0);
    }
    const FLOAT khwb_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (khwb_pass == KH_WSEE_PASS_NW) {
        ctx->OMSetRenderTargets(0, nullptr, khwb_post ? g_res.wsee_n2_dsv : g_res.wsee_n_dsv);
        ctx->OMSetBlendState(nullptr, khwb_bf, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_wsee_nw, 0);
        return;
    }
    // The split's passes (kh_wsee_ld_on stands: the caller's check).
    ID3D11BlendState* const khwb_lb = khwb_post ? g_res.bs_wsee_ldpost : g_res.bs_wsee_ldpre;
    if (khwb_pass == KH_WSEE_PASS_TP || khwb_pass == KH_WSEE_PASS_TP_OFF) {
        ctx->OMSetRenderTargets(1, &g_res.wsee_tp_rtv, g_res.depth_dsv_ro);
        ctx->OMSetBlendState(khwb_lb, khwb_bf, g_wsee_sv_mask);
        ctx->OMSetDepthStencilState(khwb_pass == KH_WSEE_PASS_TP ? g_res.dss_test : g_res.dss_off, 0);
        return;
    }
    // The 100 % band's passes: Tp's B / A (the caller pushes by KH_WSEE_NEAR_TOL2).
    if (khwb_pass == KH_WSEE_PASS_TP2 || khwb_pass == KH_WSEE_PASS_TP2_OFF) {
        ctx->OMSetRenderTargets(1, &g_res.wsee_tp_rtv, g_res.depth_dsv_ro);
        ctx->OMSetBlendState(khwb_post ? g_res.bs_wsee_ldpost2 : g_res.bs_wsee_ldpre2, khwb_bf, g_wsee_sv_mask);
        ctx->OMSetDepthStencilState(khwb_pass == KH_WSEE_PASS_TP2 ? g_res.dss_test : g_res.dss_off, 0);
        return;
    }
    if (khwb_pass == KH_WSEE_PASS_TF) {
        ctx->OMSetRenderTargets(1, &g_res.wsee_tf_rtv, g_res.wsee_tfd_dsv);
        ctx->OMSetBlendState(khwb_lb, khwb_bf, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_wsee_far, 0);
        return;
    }
    // A late opaque's replay resets Tp with T (MRT; all four channels, no blend state) while the split stands, and
    // (KH_PSB) only once this cycle's old record holds a Tp layer (KH_PSB_TPA: g_psa_tp_cycle, the first Tp pass of
    // kh_wsee_post or kh_wsee_tail; an NW-only layer writes none) - before that Tp holds its clear, which the reset
    // would write again. The one pass's record has no reset (the shaders' read-time rule). A cycle without the one
    // pass's record resets from its start.
    const bool khwb_l = khwb_pass == KH_WSEE_PASS_ONE && kh_wsee_ld_on() &&
                        (g_psb_cycle.load(std::memory_order_relaxed) != g_topo_cycles ||
                         g_psa_tp_cycle.load(std::memory_order_relaxed) == g_topo_cycles);
    // KH_WSEE_OQ2: a post opaque's third target D3m (null where not made; Tp's slot null without the split).
    ID3D11RenderTargetView* const khwb_rt[3] = { g_res.wsee_t_rtv, khwb_l ? g_res.wsee_tp_rtv : nullptr,
                                                 g_res.wsee_d3m_rtv };
    // D3: the store path binds the UAV at u3 and the depth's MS view at t0 (after the read-only DSV is in place);
    // the D3m path (g_wsee_sv_d3m, kh_wsee_d3m_path at the save) binds neither - its twin writes z into D3m, which a
    // reader resolves into D3 (kh_wsee_d3m_resolve); the store twins write -1 there, so the resolve keeps the store.
    const bool khwb_od = khwb_pass == KH_WSEE_PASS_ONE && kh_wsee_od_on();
    const bool khwb_z = khwb_od && g_wsee_sv_d3m;
    if (khwb_od && !khwb_z) {
        ctx->OMSetRenderTargetsAndUnorderedAccessViews(3u, khwb_rt, g_res.depth_dsv_ro, 3u, 1u, &g_res.wsee_d3_uav,
                                                       nullptr);
        ctx->PSSetShaderResources(0, 1, &g_res.depth_srv);
    } else {
        ctx->OMSetRenderTargets(khwb_od ? 3u : khwb_l ? 2u : 1u, khwb_rt, g_res.depth_dsv_ro);
    }
    if (khwb_z) g_wsee_d3m_cycle.store(g_topo_cycles, std::memory_order_relaxed);   // KH_WSEE_OQ2: a resolve owed.
    ID3D11BlendState* khwb_bs = nullptr;   // KH_WSEE_PASS_ONE: none (it writes (1, 1, 1, 1)).
    if (khwb_pass == KH_WSEE_PASS_T_EQ) khwb_bs = g_res.bs_vmsee_t;
    else if (khwb_pass == KH_WSEE_PASS_T || khwb_pass == KH_WSEE_PASS_T_OFF)
        khwb_bs = khwb_post ? g_res.bs_wsee_tpost : g_res.bs_wsee_tpre;
    ctx->OMSetBlendState(khwb_bs, khwb_bf, g_wsee_sv_mask);
    ctx->OMSetDepthStencilState(khwb_pass == KH_WSEE_PASS_T ? g_res.dss_test
                                : khwb_pass == KH_WSEE_PASS_T_OFF ? g_res.dss_off : g_res.dss_wsee_eq, 0);
    if (khwb_pass == KH_WSEE_PASS_ONE) {   // G = 1; the D3m path's twins are PSWseeOneZ / PSWseeOneZL.
        if (khwb_l) {
            ctx->PSSetShader(khwb_z ? g_res.ps_wsee_one_zl : g_res.ps_wsee_one_l, nullptr, 0);
        } else {
            ctx->PSSetShader(khwb_z ? g_res.ps_wsee_one_z : g_res.ps_wsee_one, nullptr, 0);
        }
    }
}

inline void kh_wsee_re_restore(ID3D11DeviceContext* ctx) {
    {
        KhInjectionFlagScope khwe_fl;   // The engine's own set back, which its trackers already saw.
        g_ro.in_injection = true;
        // The engine's t0 first: the main depth's view leaves the slot before its writable depth view is bound again
        // below (the runtime would otherwise null the slot itself); OMSetRenderTargets unbinds D3's UAV.
        if (g_wsee_sv_has_srv0) ctx->PSSetShaderResources(0, 1, &g_wsee_sv_srv0);
        ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_wsee_sv_rtvs, g_wsee_sv_dsv);
        ctx->OMSetBlendState(g_wsee_sv_bs, g_wsee_sv_bf, g_wsee_sv_mask);
        ctx->OMSetDepthStencilState(g_wsee_sv_dss, g_wsee_sv_ref);
        if (g_wsee_sv_has_ps) ctx->PSSetShader(g_wsee_sv_ps, nullptr, 0);
    }
    kh_wsee_gput_end(ctx);   // KH_WSEE_GPUT: the re-issue's block, its restore included.
    kh_wsee_sv_release();
}

// KhWseeSurfVp - a blended no-write draw lying on a surface (a decal, a set-in window, smoke's foot) is the surface's,
// not a layer: the particle-type T / NW / Tp / Tp2 passes and our tail's are drawn with every bound viewport's MinDepth
// raised, Min' = Min + TOL (Max - Min), i.e. z' = z + TOL (Max - z) in the viewport's range - a fragment at view
// distance d is pushed back by at most TOL d / (1 - TOL) (2.04 % at 0.02), less toward the far plane. Against
// LESS_EQUAL a fragment within that of the surface behind it fails (T untouched) and its NW lands behind the surface:
// the pixel is the surface's. A layer meant to be separate but within the band (smoke hugging the ground) is fogged
// with the surface. Not shifted: the fence-type and late-opaque passes (EQUAL selects the samples the draw wrote) and
// a test-off T pass (its NW pass is). Guards (kh_wsee_surf_ok, begin): the engine's depth test is not GREATER /
// GREATER_EQUAL, every bound viewport has a finite depth range and MaxDepth = the world range's Hi (kh_wsee_surf_max,
// within 1e-6: the inverse below is exact only then; a draw in another range is unshifted and its NW position reads
// at most 2.04 % near). NW holds the pushed raw n' = (1 - TOL) n + TOL Hi; it decides (n' against r, the evidence
// rule) and the shaders take the position back, n = (n' - TOL Hi) / (1 - TOL) (cb.hlsl KhWseeNwPos / KhWseeMsNwPos,
// Hi = depthParams.w / khFxFog[0].w). An engine PS reading SV_Position.z (soft particles) sees the pushed depth in
// these passes only: a fade over F metres loses up to TOL d / F near a surface. The viewports go back exactly
// (RSSetViewports is not hooked). TWIN: cb.hlsl KH_WSEE_SURF_TOL 0.02.
static constexpr float KH_WSEE_SURF_TOL = 0.02f;
// The world range's MaxDepth the chain and the sort decode with (kh_fill_depth_range_cb's rule, read at the draw).
inline float kh_wsee_surf_max() {
    float khsm_lo = 0.0f, khsm_hi = 1.0f;
    kh_wsee_world_range(khsm_lo, khsm_hi);
    return khsm_hi;
}
struct KhWseeSurfVp {
    D3D11_VIEWPORT vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT           n = 0;
    bool           on = false;
    // The array shifted and set back: vp, read by begin, or the draw's kept array (keep: kh_wsee_post's, read once by
    // its pre-draw - g_wsee_pd_vp's note); with one kept, begin reads nothing.
    const D3D11_VIEWPORT* src = vp;
    bool           kept = false;
    void keep(const D3D11_VIEWPORT* khsv_vp, UINT khsv_n) {
        src = khsv_vp;
        n = khsv_n;
        kept = true;
    }
    // The shift (the bound viewports read, kept, and set again raised); false = nothing changed (no viewport, one
    // without a depth range, or one whose MaxDepth is not the world range's, kh_wsee_surf_max). khsv_tol: the push
    // (KH_WSEE_SURF_TOL; KH_WSEE_NEAR_TOL / NEAR_TOL2 for the Tp passes). A shift standing is kept (end first to
    // change the push).
    bool begin(ID3D11DeviceContext* khsv_ctx, float khsv_tol = KH_WSEE_SURF_TOL) {
        if (on) return true;
        if (!kept) {
            n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            khsv_ctx->RSGetViewports(&n, vp);
        }
        if (n == 0 || n > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) return false;
        const float khsv_wh = kh_wsee_surf_max();
        D3D11_VIEWPORT khsv_s[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        for (UINT khsv_i = 0; khsv_i < n; ++khsv_i) {
            const float khsv_lo = src[khsv_i].MinDepth, khsv_hi = src[khsv_i].MaxDepth;
            if (!(khsv_hi > khsv_lo) || !std::isfinite(khsv_lo) || !std::isfinite(khsv_hi)) return false;
            if (!(fabsf(khsv_hi - khsv_wh) <= 1.0e-6f)) return false;   // The shaders' inverse needs Hi.
            khsv_s[khsv_i] = src[khsv_i];
            khsv_s[khsv_i].MinDepth = khsv_lo + khsv_tol * (khsv_hi - khsv_lo);
        }
        khsv_ctx->RSSetViewports(n, khsv_s);
        on = true;
        return true;
    }
    void end(ID3D11DeviceContext* khsv_ctx) {
        if (!on) return;
        khsv_ctx->RSSetViewports(n, src);
        on = false;
    }
};
// The near bands: KhWseeSurfVp's shift at 0.2 pushes a fragment at view distance d to d / (1 - 0.2 (1 - d / f))
// (1.25 d near the camera); at 0.5 to 2 d. A particle-type fragment that passes the 2 % test but not the 25 % one is
// in T and not in Tp.R|G (the chain charges it at the band's middle, cb.hlsl KhWseeNearRaw); one not passing the
// 100 % test is not in Tp.B|A (charged at KhWseeNearRaw2). TWINS: cb.hlsl KH_WSEE_NEAR_TOL 0.2, KH_WSEE_NEAR_TOL2 0.5.
static constexpr float KH_WSEE_NEAR_TOL = 0.2f;
static constexpr float KH_WSEE_NEAR_TOL2 = 0.5f;
// Every bound viewport is the world range the far raw D was computed in (g_wsee_ld_far_lo / hi, within 1e-6): the Tf
// pass compares raws of that range alone. khvw_k / khvw_kn: the draw's kept array (kh_wsee_post's, g_wsee_pd_vp's
// note) in place of the read; null, the bound array is read (the tail's).
inline bool kh_wsee_vp_world(ID3D11DeviceContext* khvw_ctx, const D3D11_VIEWPORT* khvw_k = nullptr, UINT khvw_kn = 0) {
    D3D11_VIEWPORT khvw_rd[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT khvw_n = khvw_kn;
    const D3D11_VIEWPORT* khvw_vp = khvw_k;
    if (!khvw_k) {
        khvw_n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        khvw_ctx->RSGetViewports(&khvw_n, khvw_rd);
        khvw_vp = khvw_rd;
    }
    if (khvw_n == 0 || khvw_n > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) return false;
    for (UINT khvw_i = 0; khvw_i < khvw_n; ++khvw_i) {
        if (!(fabsf(khvw_vp[khvw_i].MinDepth - g_wsee_ld_far_lo) <= 1.0e-6f &&
              fabsf(khvw_vp[khvw_i].MaxDepth - g_wsee_ld_far_hi) <= 1.0e-6f)) return false;
    }
    return true;
}
// Every bound viewport lies at or beyond the world range's far side (MinDepth >= Hi within 1e-6, MaxDepth >=
// MinDepth; NaN fails): the engine's cloud deck is drawn in a [1, 1] range, so every fragment has z >= Hi > D's raw
// and the Tf pass's GREATER passes wherever it rasterizes - the layer is beyond D by construction. Accepted beside
// kh_wsee_vp_world at kh_wsee_post's Tf gate. A range partly before Hi (a far partition's [0.5, 1]) is not: a fragment
// of it may land before D.
inline bool kh_wsee_vp_beyond(ID3D11DeviceContext* khvb_ctx, const D3D11_VIEWPORT* khvb_k = nullptr, UINT khvb_kn = 0) {
    D3D11_VIEWPORT khvb_rd[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT khvb_n = khvb_kn;
    const D3D11_VIEWPORT* khvb_vp = khvb_k;
    if (!khvb_k) {
        khvb_n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        khvb_ctx->RSGetViewports(&khvb_n, khvb_rd);
        khvb_vp = khvb_rd;
    }
    if (khvb_n == 0 || khvb_n > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) return false;
    for (UINT khvb_i = 0; khvb_i < khvb_n; ++khvb_i) {
        if (!(khvb_vp[khvb_i].MinDepth >= g_wsee_ld_far_hi - 1.0e-6f &&
              khvb_vp[khvb_i].MaxDepth >= khvb_vp[khvb_i].MinDepth)) return false;
    }
    return true;
}
// The engine's depth state allows the shift (kh_wsee_post, after kh_wsee_re_save: the saved state): refused only on
// evidence of reversed depth - a GREATER / GREATER_EQUAL test; every other function or no test is no evidence either
// way, and the replays' own tests are ours (dss_test LESS_EQUAL, dss_wsee_nw LESS). Null is the default state (LESS).
inline bool kh_wsee_surf_ok(ID3D11DepthStencilState* khso_dss) {
    if (!khso_dss) return true;
    D3D11_DEPTH_STENCIL_DESC khso_d = {};
    khso_dss->GetDesc(&khso_d);
    return !khso_d.DepthEnable || (khso_d.DepthFunc != D3D11_COMPARISON_GREATER &&
                                   khso_d.DepthFunc != D3D11_COMPARISON_GREATER_EQUAL);
}

// KH_PSB - this cycle's one-pass record stands: the capture cleared P0 / P1 / N / N2 (g_psb_cycle), the states and the
// main depth's multisampled view are there (render thread).
inline bool kh_psb_on() {
    return g_psb_cycle.load(std::memory_order_relaxed) == g_topo_cycles && g_res.wsee_p0_rtv && g_res.wsee_p1_rtv &&
           g_res.wsee_nb_rtv && g_res.wsee_nb2_rtv && g_res.bs_psb[1] && g_res.psb_cb[7] && g_res.depth_srv;
}
// KH_PSB: the bound rasterizer lets the variant's in-shader z stand for the hardware's - no depth bias (SV_Position.z
// does not carry it) and depth clip on (null = the default state); khpr_sc = whether it scissors. KH_PSB_RSPIN: the
// state last asked is pinned with its verdicts (g_res.wsee_rs_pin_psb: one reference, so a reused address cannot pass
// for it; states are immutable) - GetDesc only for another state; released with that pin.
inline bool kh_psb_rs_ok(ID3D11DeviceContext* ctx, bool* khpr_sc = nullptr) {
    if (khpr_sc) *khpr_sc = false;
    ID3D11RasterizerState* khpr_rs = nullptr;
    ctx->RSGetState(&khpr_rs);
    if (!khpr_rs) return true;
    if (khpr_rs == g_res.wsee_rs_pin_psb) {   // KH_PSB_RSPIN.
        khpr_rs->Release();
    } else {
        D3D11_RASTERIZER_DESC khpr_d = {};
        khpr_rs->GetDesc(&khpr_d);
        KH_SAFE_RELEASE(g_res.wsee_rs_pin_psb);
        g_res.wsee_rs_pin_psb = khpr_rs;   // The reference RSGetState took is the pin's.
        g_res.wsee_rs_pin_psb_sc = khpr_d.ScissorEnable != FALSE;   // KH_PSB_LORES.
        g_res.wsee_rs_pin_psb_ok = khpr_d.DepthBias == 0 && khpr_d.SlopeScaledDepthBias == 0.0f &&
                                   khpr_d.DepthClipEnable != FALSE;
    }
    if (khpr_sc) *khpr_sc = g_res.wsee_rs_pin_psb_sc;
    return g_res.wsee_rs_pin_psb_ok;
}
// KH_PSB_LORES: a scissor edge khps_e of the frame at 1 / s (s = 1 << khps_ls): the first pixel of the scaled target
// whose centre, taken to the frame ((X + 1/2) s), lies at or past khps_e - floor((e + s / 2 - 1) / s), so the scaled
// rect keeps exactly the block centres the frame's rect holds (the rasterizer's own pixel-centre rule).
inline LONG kh_psb_sdiv(LONG khps_e, UINT khps_ls) {
    const long long khps_s = 1ll << khps_ls;
    const long long khps_q = static_cast<long long>(khps_e) + (khps_s >> 1) - 1;
    return static_cast<LONG>(khps_q >= 0 ? khps_q / khps_s : -((-khps_q + khps_s - 1) / khps_s));
}
// KH_PSB_TF_EPS - the one pass writes a fragment's Tf channels only where the block's largest live sample 0 r >= Dq,
// the float just above D - KH_PSB_TF_EPS (D's raw as the capture took it, g_psb_far_raw; 0 where Tf is not drawn).
// Exact at sample 0: Tf is read by KhWseeGroupsSet8's far branch alone, only at a sample whose live depth r' at the
// flush exceeds D, and the main depth only comes nearer within a cycle (the engine tests LESS / LESS_EQUAL), so at a
// sample 0 r >= r' > D > Dq there and every fragment the old Tf pass wrote is written. At a sample k > 0 off its
// block's sample-0 surfaces (r'_k > D while r < Dq) the one pass writes no Tf where the old pass did; its T verdict
// (z_1 <= r) drops the same far fragments (KH_PSB_LORES' block verdicts), so that sample reads no far layer in either
// channel. The margin covers D read as a constant by the shader and as a float here (1e-6 is ~17 ulp just below 1;
// D's raw lies below 1). Premises: the chain's D (KhWseeFarRaw) is the capture's
// within the margin (a depth-pair mismatch moves it), and the reader's depth is the live one or nearer (LoadDepthPS's
// KH_VM_SEE substitution is farther: particles drawn after the view model's first translucent draw).
static constexpr float KH_PSB_TF_EPS = 1.0e-6f;
// KH_PSB - a particle-type re-issue (RE_NOWRITE / RE_NOWRITE_OFF; after kh_wsee_re_save and kh_psv_pick) drawn ONCE
// with its shader's one-pass variant in place of today's passes: no depth view bound, its phase's P / N pair (single
// sample, at 1 / s of the frame - KH_PSB_LS: the viewports and, where the state scissors, the scissor rects set at
// 1 / s around the draw and back from the same arrays; kh_psb_sdiv), the phase's blend state, the variant's constant
// buffer (which verdicts it makes and how: the five passes' own terms as kh_wsee_post reads them, a fourth row for s
// and the reads - the patcher's note) and the depth's MS view at the variant's slots; Zb at its second slot with
// cb3.w while it stands this cycle. Its slots (the constant buffer's, the depth view's and, with Zb, Zb's) are put
// back here (the views before the restore binds the engine's depth: the runtime would null a view of a bound depth),
// the rest by the caller's restore. Verdicts: the
// pushes where KhWseeSurfVp's shift would stand (khpo_sfo, every bound viewport with a depth range whose MaxDepth is
// the world range's, taken as Hi), test-off for RE_NOWRITE_OFF, Tf where its pass would be drawn (the split on, a far
// raw, khpo_sfo, the world or beyond viewport) and r >= Dq (KH_PSB_TF_EPS's note). False (nothing drawn or changed;
// the caller draws today's passes): no record this cycle, no variant, RE_NWONLY (its NW pass binds no pixel shader
// and costs less than the whole engine shader), the engine's sample mask not every sample, a depth bias or no depth
// clip (kh_psb_rs_ok), no viewport or viewports of different depth ranges. For a particle-type draw this cannot take,
// an NW-only one included, the caller (kh_wsee_post) sets g_psa_cycle (lane bit 7 clears: every armed pixel loads the
// old record). No return or throw between its sets and their set-back.
template <class KhWseeDraw>
inline bool kh_psb_post(ID3D11DeviceContext* ctx, uint8_t khpo_k, bool khpo_post, const KhPsvPick& khpo_pv,
                        bool khpo_vk, bool khpo_sfo, KhWseeDraw& khpo_draw) {
    if (!kh_psb_on()) return false;
    if (khpo_k == KH_WSEE_RE_NWONLY) return false;   // Its NW-only pass (no pixel shader), into the old record.
    const UINT khpo_ns = g_res.depth_sample_count;
    bool khpo_sci = false;                   // KH_PSB_LORES: the engine's state scissors.
    const UINT khpo_all = khpo_ns >= 32u ? 0xFFFFFFFFu : ((1u << khpo_ns) - 1u);
    D3D11_VIEWPORT khpo_rd[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT khpo_n = g_wsee_pd_vpn;
    const D3D11_VIEWPORT* khpo_vp = g_wsee_pd_vp;
    if (!khpo_vk) {
        khpo_n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetViewports(&khpo_n, khpo_rd);
        khpo_vp = khpo_rd;
    }
    bool khpo_vok = khpo_n > 0u && khpo_n <= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    for (UINT khpo_i = 1u; khpo_i < khpo_n && khpo_vok; ++khpo_i) {
        khpo_vok = khpo_vp[khpo_i].MinDepth == khpo_vp[0].MinDepth && khpo_vp[khpo_i].MaxDepth == khpo_vp[0].MaxDepth;
    }
    // kh_psb_rs_ok only past the first two terms (it pins a state).
    if (!khpo_pv.on || !khpo_pv.one || khpo_ns == 0u || (g_wsee_sv_mask & khpo_all) != khpo_all ||
        !kh_psb_rs_ok(ctx, &khpo_sci) || !khpo_vok) {
        return false;
    }
    const float khpo_lo = khpo_vp[0].MinDepth, khpo_hi = khpo_vp[0].MaxDepth;
    const bool khpo_push = khpo_sfo && khpo_hi > khpo_lo && std::isfinite(khpo_lo) && std::isfinite(khpo_hi) &&
                           fabsf(khpo_hi - kh_wsee_surf_max()) <= 1.0e-6f;
    const bool khpo_ld = kh_wsee_ld_on();
    const bool khpo_off = khpo_k == KH_WSEE_RE_NOWRITE_OFF;
    const bool khpo_tf = khpo_ld && g_wsee_ld_far_ok && khpo_sfo &&
                         (kh_wsee_vp_world(ctx, khpo_vp, khpo_n) || kh_wsee_vp_beyond(ctx, khpo_vp, khpo_n));
    // The verdict set's constant buffer, refilled where its Hi, D or sample count differ (bits, exactly).
    const int khpo_ix = (khpo_push ? 1 : 0) | (khpo_off ? 2 : 0) | (khpo_tf ? 4 : 0);
    // KH_PSB_ZB: Zb bounds the loop where it stands this cycle (the capture built it and its guard has not
    // tripped; the depth view still the one S was read through - else a trip) and the variant has its slot.
    bool khpo_zb = g_res.wsee_zb_srv && khpo_pv.one_tz < 128u && kh_psb_zb_armed();
    if (khpo_zb && g_res.depth_srv_gen != g_psb_zb_gen) {
        kh_psb_zb_trip();
        khpo_zb = false;
    }
    uint32_t khpo_key[4] = { 0u, 0u, khpo_ns, KH_PSB_LS | (khpo_zb ? 0x100u : 0u) };   // KH_PSB_LORES / _ZB.
    memcpy(&khpo_key[0], &khpo_hi, 4);
    memcpy(&khpo_key[1], &g_psb_far_raw, 4);
    // KH_PSB_CB_INJ: the refill under the injection flag with the pass's own binds below - an upload of ours, which
    // the UpdateSubresource hook otherwise scanned as an engine upload (kh_upload_hook_wanted: not in_injection).
    KhInjectionFlagScope khpo_fl;
    g_ro.in_injection = true;
    if (!g_res.psb_cb_set[khpo_ix] || memcmp(g_res.psb_cb_key[khpo_ix], khpo_key, sizeof(khpo_key)) != 0) {
        static const float khpo_tol[3] = { KH_WSEE_SURF_TOL, KH_WSEE_NEAR_TOL, KH_WSEE_NEAR_TOL2 };
        float khpo_c[8] = {};
        for (int khpo_i = 0; khpo_i < 3; ++khpo_i) {
            khpo_c[khpo_i] = khpo_push ? 1.0f - khpo_tol[khpo_i] : 1.0f;      // m_i
            khpo_c[4 + khpo_i] = khpo_push ? khpo_tol[khpo_i] * khpo_hi : 0.0f;  // a_i
        }
        // Dq (KH_PSB_TF_EPS's note) where Tf is drawn, else 0 - with the bands' mask off (the test-off kind)
        // the sample loop's bound is Dq alone, so that kind reads no sample where Tf is not drawn.
        khpo_c[3] = khpo_tf ? std::nextafter(g_psb_far_raw - KH_PSB_TF_EPS, 2.0f) : 0.0f;
        khpo_c[7] = g_psb_far_raw;
        uint32_t khpo_u[16] = {};
        memcpy(khpo_u, khpo_c, sizeof(khpo_c));
        khpo_u[8] = khpo_off ? 0xFFFFFFFFu : 0u;
        khpo_u[9] = khpo_tf ? 0xFFFFFFFFu : 0u;
        khpo_u[10] = khpo_off ? 0u : 0xFFFFFFFFu;   // The bands' mask: their verdicts tested.
        // KH_PSB_LORES: the reads - sample 0 of the block's s x s pixels (s = 1 << KH_PSB_LS; the patcher's s = 1
        // arm, every sample of the pixel, is never taken); cb3: the patcher's note.
        khpo_u[11] = 1u << (2u * KH_PSB_LS);
        khpo_u[12] = KH_PSB_LS;
        khpo_u[13] = (1u << KH_PSB_LS) - 1u;
        khpo_u[14] = 0u;
        khpo_u[15] = khpo_zb ? 0xFFFFFFFFu : 0u;   // KH_PSB_ZB: cb3.w, Zb's flag.
        ctx->UpdateSubresource(g_res.psb_cb[khpo_ix], 0, nullptr, khpo_u, 0, 0);
        memcpy(g_res.psb_cb_key[khpo_ix], khpo_key, sizeof(khpo_key));
        g_res.psb_cb_set[khpo_ix] = true;
    }
    {
        ID3D11Buffer* khpo_ocb = nullptr;
        ID3D11ShaderResourceView* khpo_osrv = nullptr;
        ID3D11ShaderResourceView* khpo_ozb = nullptr;   // KH_PSB_ZB: the engine's view at Zb's slot.
        // A D3D11.1 PSSetConstantBuffers1 offset / size at this slot would not survive the set-back (the engine's
        // next draw would read it from offset 0); the patcher takes the highest slot the shader leaves free, which the
        // engine is not known to bind.
        ctx->PSGetConstantBuffers(khpo_pv.one_cb, 1, &khpo_ocb);
        ctx->PSGetShaderResources(khpo_pv.one_t, 1, &khpo_osrv);
        // KH_PSB_PHASE: the phase's pair alone - P0 / N pre, P1 / N2 post -, the split's channels written only while
        // it stands (bs_psb[1] takes T's alone; else nothing reads them: they keep their clear).
        ID3D11RenderTargetView* const khpo_rt[2] = { khpo_post ? g_res.wsee_p1_rtv : g_res.wsee_p0_rtv,
                                                     khpo_post ? g_res.wsee_nb2_rtv : g_res.wsee_nb_rtv };
        ctx->OMSetRenderTargets(2, khpo_rt, nullptr);
        const FLOAT khpo_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(g_res.bs_psb[khpo_ld ? 0 : 1], khpo_bf, 0xFFFFFFFF);
        ctx->PSSetShader(khpo_pv.one, nullptr, 0);
        ctx->PSSetConstantBuffers(khpo_pv.one_cb, 1, &g_res.psb_cb[khpo_ix]);
        // Legal: no depth view is bound. The engine's MS depth is read as a resource at every one-pass draw, between
        // the engine's own depth writes: a driver may decompress or wait at each write-to-read turn (not measured).
        ctx->PSSetShaderResources(khpo_pv.one_t, 1, &g_res.depth_srv);
        if (khpo_zb) {   // KH_PSB_ZB: Zb at the variant's second slot (read only under cb3.w), set back below.
            ctx->PSGetShaderResources(khpo_pv.one_tz, 1, &khpo_ozb);
            ctx->PSSetShaderResources(khpo_pv.one_tz, 1, &g_res.wsee_zb_srv);
        }
        // KH_PSB_LORES: the viewports (and the scissor rects, where the state scissors) at the targets'
        // scale - the same mapping of the scene onto 1 / s of the pixels (a pixel's centre lands on its block's
        // centre); set back below from the arrays they came from.
        D3D11_RECT khpo_or[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        UINT khpo_nr = 0;
        {   // s > 1 (KH_PSB_LS).
            const float khpo_f = 1.0f / static_cast<float>(1u << KH_PSB_LS);   // Exact: a power of two.
            D3D11_VIEWPORT khpo_sv[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
            for (UINT khpo_i = 0; khpo_i < khpo_n; ++khpo_i) {
                khpo_sv[khpo_i] = khpo_vp[khpo_i];
                khpo_sv[khpo_i].TopLeftX *= khpo_f;
                khpo_sv[khpo_i].TopLeftY *= khpo_f;
                khpo_sv[khpo_i].Width *= khpo_f;
                khpo_sv[khpo_i].Height *= khpo_f;
            }
            ctx->RSSetViewports(khpo_n, khpo_sv);
            if (khpo_sci) {
                khpo_nr = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
                ctx->RSGetScissorRects(&khpo_nr, khpo_or);
                if (khpo_nr > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) khpo_nr = 0;
                D3D11_RECT khpo_sr[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
                for (UINT khpo_i = 0; khpo_i < khpo_nr; ++khpo_i) {
                    khpo_sr[khpo_i].left = kh_psb_sdiv(khpo_or[khpo_i].left, KH_PSB_LS);
                    khpo_sr[khpo_i].top = kh_psb_sdiv(khpo_or[khpo_i].top, KH_PSB_LS);
                    khpo_sr[khpo_i].right = kh_psb_sdiv(khpo_or[khpo_i].right, KH_PSB_LS);
                    khpo_sr[khpo_i].bottom = kh_psb_sdiv(khpo_or[khpo_i].bottom, KH_PSB_LS);
                }
                if (khpo_nr > 0u) ctx->RSSetScissorRects(khpo_nr, khpo_sr);
            }
        }
        if (g_wsee_gput.blk >= 0) kh_wsee_gput_pass(ctx, KHWG_PART, KHWG_P_ONE);   // KH_WSEE_GPUT: the one pass.
        khpo_draw();
        ctx->RSSetViewports(khpo_n, khpo_vp);   // KH_PSB_LORES: the engine's own again.
        if (khpo_nr > 0u) ctx->RSSetScissorRects(khpo_nr, khpo_or);
        if (khpo_zb) ctx->PSSetShaderResources(khpo_pv.one_tz, 1, &khpo_ozb);   // KH_PSB_ZB.
        ctx->PSSetShaderResources(khpo_pv.one_t, 1, &khpo_osrv);
        ctx->PSSetConstantBuffers(khpo_pv.one_cb, 1, &khpo_ocb);
        KH_SAFE_RELEASE(khpo_ozb);
        KH_SAFE_RELEASE(khpo_osrv);
        KH_SAFE_RELEASE(khpo_ocb);
    }
    g_psb_drew_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    return true;
}

// The draw hooks' post (render thread, the tracked context's own engine draw, never ours - the hooks' gate): the
// engine's draw again (khwq_draw: the original, unhooked call with the hook's own arguments) for the kind its
// pre-draw marked - a fence-type layer into T; a particle-type one into T then NW (and Tp / Tp2 / Tf), or the one
// pass; a light-adding one into NW alone; a late opaque into T's flag (PASS_ONE). One save and one restore a post. A
// refused or impossible re-issue draws nothing and taints the cycle (T or NW would miss the layer). Nothing here
// throws.
template <class KhWseeDraw>
inline void kh_wsee_post(ID3D11DeviceContext* ctx, uint8_t khwq_k, KhWseeDraw&& khwq_draw) {
    const bool khwq_opq = khwq_k == KH_WSEE_RE_OPAQUE;
    // The viewports the pre-draw read for this draw (g_wsee_pd_vp's note), taken here - once.
    const bool khwq_vk = g_wsee_pd_vp_ok;
    g_wsee_pd_vp_ok = false;
    if (!kh_wsee_re_ready(khwq_k) || !kh_wsee_re_save(ctx, khwq_opq)) {
        kh_wsee_taint(g_topo_cycles);
        return;
    }
    const bool khwq_nw = khwq_k == KH_WSEE_RE_NOWRITE || khwq_k == KH_WSEE_RE_NOWRITE_OFF ||
                         khwq_k == KH_WSEE_RE_NWONLY;
    // An engine draw that tests stencil: a depth writer's passes are EQUAL against the depth it left (exactly the
    // samples it wrote, stencil included - nothing replayed); a kind with an NW pass cannot be recorded (NW's D32
    // holds no stencil, and it writes no depth to select by) and the pre-draw leaves it out by the DSS hook's latch,
    // which only the engine's binds set and none comes between a pre-draw and its post - so this block is a backstop,
    // read off the saved state when the latch says stencil.
    if (khwq_nw && g_ro.dss_stencil && g_wsee_sv_dss) {
        D3D11_DEPTH_STENCIL_DESC khwq_sd = {};
        g_wsee_sv_dss->GetDesc(&khwq_sd);
        if (khwq_sd.StencilEnable) {
            kh_wsee_re_restore(ctx);
            return;
        }
    }
    // A particle-type layer's phase (the pre-draw's); a late opaque's too (kh_wsee_mark_opaque).
    const bool khwq_post = (khwq_nw || khwq_opq) && g_wsee_pending_post;   // A late opaque's: always true.
    // KH_PSV: a particle-type kind's passes draw the variants of the engine's pixel shader where they are made
    // (kh_psv_pick; the shader it took is the save's - kh_wsee_re_restore sets it back, as a late opaque's); none,
    // nothing is taken or set - today's passes.
    KhPsvPick khwq_pv;
    if (khwq_nw) {
        kh_psv_pick(ctx, khwq_pv, false);
        if (khwq_pv.on) {
            g_wsee_sv_ps = khwq_pv.orig;
            g_wsee_sv_has_ps = true;
            khwq_pv.orig = nullptr;
        }
    }
    // A particle-type kind's depth-tested T pass and its NW pass take the surface shift (KhWseeSurfVp's note); the
    // test-off T pass (no test) and the depth writers' EQUAL passes do not; the test-off kind's NW pass does (its
    // position exact through the shaders' inverse). Set back before the restore.
    KhWseeSurfVp khwq_sv;
    if (khwq_vk) khwq_sv.keep(g_wsee_pd_vp, g_wsee_pd_vpn);   // The pre-draw's array: no read of its own.
    // The saved depth state's verdict, once (khwq_sfo; the Tf pass below takes it too).
    const bool khwq_sfo = khwq_nw && kh_wsee_surf_ok(g_wsee_sv_dss);
    bool khwq_sok = khwq_sfo;
    // KH_PSB: a particle-type kind in ONE draw where it can (kh_psb_post; its passes' verdicts in the shader), else
    // today's passes below - into the old record, which the shaders then read beside the one pass's (g_psa_cycle:
    // lane bit 7 clears, the chain and the sort load the old record; the late-opaque replays reset Tp from
    // g_psa_tp_cycle, which the Tp passes below stamp).
    const bool khwq_one = khwq_nw && kh_psb_post(ctx, khwq_k, khwq_post, khwq_pv, khwq_vk, khwq_sfo, khwq_draw);
    if (khwq_nw && !khwq_one && g_psa_cycle.load(std::memory_order_relaxed) != g_topo_cycles) {
        g_psa_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (khwq_k != KH_WSEE_RE_NWONLY && !khwq_one) {
        if (khwq_sok && khwq_k == KH_WSEE_RE_NOWRITE) khwq_sok = khwq_sv.begin(ctx);
        kh_wsee_re_set(ctx, khwq_opq ? KH_WSEE_PASS_ONE
                            : khwq_k == KH_WSEE_RE_NOWRITE_OFF ? KH_WSEE_PASS_T_OFF
                            : khwq_k == KH_WSEE_RE_BLEND ? KH_WSEE_PASS_T_EQ : KH_WSEE_PASS_T, khwq_post,
                       &khwq_pv);   // KH_PSV.
        khwq_draw();
    }
    if (khwq_nw && !khwq_one) {
        if (khwq_sok) khwq_sok = khwq_sv.begin(ctx);
        kh_wsee_re_set(ctx, KH_WSEE_PASS_NW, khwq_post, &khwq_pv);   // KH_PSV: nw, or no pixel shader.
        khwq_draw();
    }
    // The layer-depth split - a kind with a T pass again into Tp (the depth-tested kind under the near push where the
    // surface shift stood - else unpushed: as its T pass, no near part; the test-off kind untested, as its T) and
    // into Tf (only where every viewport is the world range D was computed in, or lies at or beyond its far side -
    // kh_wsee_vp_world / kh_wsee_vp_beyond - and the depth is not reversed; else not drawn: Tf then lacks this
    // layer's far part, which only the far pixels read).
    if (khwq_nw && khwq_k != KH_WSEE_RE_NWONLY && kh_wsee_ld_on() && !khwq_one) {
        khwq_sv.end(ctx);
        g_psa_tp_cycle.store(g_topo_cycles, std::memory_order_relaxed);   // KH_PSB_TPA: the replays reset Tp from here.
        bool khwq_np = false;
        if (khwq_k == KH_WSEE_RE_NOWRITE) {
            if (khwq_sok) khwq_np = khwq_sv.begin(ctx, KH_WSEE_NEAR_TOL);
            kh_wsee_re_set(ctx, KH_WSEE_PASS_TP, khwq_post, &khwq_pv);   // KH_PSV.
        } else {
            kh_wsee_re_set(ctx, KH_WSEE_PASS_TP_OFF, khwq_post, &khwq_pv);
        }
        khwq_draw();
        khwq_sv.end(ctx);
        // The same pass once more into Tp's B / A under the 100 % band's push (where the 25 % push stood - the same
        // shift test; else unpushed, as its T: no band part); the test-off kind untested.
        if (khwq_k == KH_WSEE_RE_NOWRITE) {
            if (khwq_np) khwq_sv.begin(ctx, KH_WSEE_NEAR_TOL2);
            kh_wsee_re_set(ctx, KH_WSEE_PASS_TP2, khwq_post, &khwq_pv);   // KH_PSV.
        } else {
            kh_wsee_re_set(ctx, KH_WSEE_PASS_TP2_OFF, khwq_post, &khwq_pv);
        }
        khwq_draw();
        khwq_sv.end(ctx);
        if (g_wsee_ld_far_ok && khwq_sfo &&
            (kh_wsee_vp_world(ctx, khwq_vk ? g_wsee_pd_vp : nullptr, g_wsee_pd_vpn) ||
             kh_wsee_vp_beyond(ctx, khwq_vk ? g_wsee_pd_vp : nullptr, g_wsee_pd_vpn))) {
            kh_wsee_re_set(ctx, KH_WSEE_PASS_TF, khwq_post, &khwq_pv);   // KH_PSV.
            khwq_draw();
        }
    }
    // KhWseeSurfVp has no destructor - nothing between its begin above and this end may return or throw (the
    // stencil backstop returns before it exists).
    khwq_sv.end(ctx);
    kh_wsee_re_restore(ctx);
}

// Our translucent tail (flush_locked's normal-blend translucents past its solids that test the depth without writing
// it - a whole translucent object, a blend material's part - and inject_composited_meshes' normal-blend parts drawn at
// the injection) is a blended no-write layer as the engine's particles are: owed into T and NW while this cycle's
// capture stands (landed, untainted, unused, the targets made against the depth the caller draws on). The caller's
// thread (render thread, or a park that stalls it: the cycle is g_topo_cycles, kh_wsee_taint's).
inline bool kh_wsee_tail_owed() {
    if constexpr (!KH_WSEE_ON) {   // Built out: no code left unreachable (C4702).
        return false;
    } else {
        const uint64_t khwo_c = g_topo_cycles;
        return g_wsee_ok_cycle.load(std::memory_order_relaxed) == khwo_c &&
               g_wsee_taint_cycle.load(std::memory_order_relaxed) != khwo_c &&
               g_wsee_used_cycle.load(std::memory_order_relaxed) != khwo_c && g_res.wsee_t_rtv && g_res.wsee_n_dsv &&
               g_res.depth_dsv_ro && g_res.dss_test && g_res.bs_vmsee_t && g_res.dss_wsee_nw &&
               g_res.wsee_depth_id && g_res.wsee_n2_dsv && g_res.bs_wsee_tpre && g_res.bs_wsee_tpost &&
               g_res.wsee_depth_id == g_res.depth_res_identity && g_res.wsee_samples == g_res.depth_sample_count;
    }
}

// One owed tail draw (kh_wsee_tail_owed first): the caller's draw again, khwt_draw(true) into T over depth_dsv_ro
// (dss_test; Tpre or Tpost *= 1 - a: the caller's PS writes its coverage as straight alpha - PSComposite / PSMain's
// normal-blend output, the perceptual arm dropped for it), then khwt_draw(false) depth-only into NW (dss_wsee_nw);
// and the Tp / Tp2 / Tf passes under the shifts; every sample. Saved and set back here (KhOmSave with UAVs, blend
// state / factor / mask, depth state / reference; KhWseeSurfVp the viewports) so the caller's bound-state trackers
// stay true. Two callers: flush_locked's tail (g_kh_flush_active, so the OM hooks stand aside without in_injection)
// and inject_composited_meshes' tail (under in_injection, which they stand aside for). False = refused (the bound
// depth is not the capture's): nothing drawn, the caller taints.
template <class KhWseeTailDraw>
inline bool kh_wsee_tail(ID3D11DeviceContext* ctx, KhWseeTailDraw&& khwt_draw) {
    KhOmSave khwt_om;
    khwt_om.capture(ctx);
    if (!khwt_om.dsv || reorder_dsv_identity(khwt_om.dsv) != g_res.wsee_depth_id) {
        khwt_om.release();   // Nothing changed: no restore owed (release disarms).
        return false;
    }
    KhWseeGputScope khwt_gt(ctx, KHWG_TAIL);   // KH_WSEE_GPUT: its passes and its restore.
    ID3D11BlendState* khwt_bs = nullptr;
    FLOAT khwt_bf[4] = {};
    UINT khwt_mask = 0xFFFFFFFFu;
    ctx->OMGetBlendState(&khwt_bs, khwt_bf, &khwt_mask);
    ID3D11DepthStencilState* khwt_dss = nullptr;
    UINT khwt_ref = 0;
    ctx->OMGetDepthStencilState(&khwt_dss, &khwt_ref);
    const FLOAT khwt_z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    // Our layer is particle-type (it writes no depth): drawn after this cycle's B2, into Tpost (T's A) and NWpost;
    // before it, into Tpre (T's B) and NWpre.
    const bool khwt_post = g_wsee_b2_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
    // Both passes take the surface shift, as the engine's particle-type re-issues do (KhWseeSurfVp's note): a part of
    // ours within KH_WSEE_SURF_TOL of the surface behind it (a decal mesh, a pane set in a wall) is that surface's for
    // the chain and the later sorted parts; our depth test is LESS_EQUAL. The caller's draws set no viewport
    // (kh_draw_textured, kh_inst_draw); one without a depth range or whose MaxDepth is not the world range's is
    // unshifted. KhWseeSurfVp has no destructor: no return or throw before khwt_sv.end. KH_PSV: the bound shader is
    // the caller's builtin (kh_draw_textured binds the builtin after a user material's shader; kh_psv_pick's tail
    // entries); its alpha variant before every T / Tp / Tp2 / Tf draw, nw (or none) before NW's, the builtin back
    // below. No variant: the builtin as bound.
    KhPsvPick khwt_pv;
    kh_psv_pick(ctx, khwt_pv, true);
    // KH_PSB: our layer goes into the old record (the builtins have no one-pass variant: a second target, a depth or
    // coverage output) - the shaders read it this cycle, the later late-opaque replays reset it.
    if (g_psa_cycle.load(std::memory_order_relaxed) != g_topo_cycles) {
        g_psa_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    KhWseeSurfVp khwt_sv;
    const bool khwt_sok = khwt_sv.begin(ctx);
    ctx->OMSetRenderTargets(1, &g_res.wsee_t_rtv, g_res.depth_dsv_ro);
    ctx->OMSetBlendState(khwt_post ? g_res.bs_wsee_tpost : g_res.bs_wsee_tpre, khwt_z, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_res.dss_test, 0);
    if (khwt_pv.on) ctx->PSSetShader(khwt_pv.alpha, nullptr, 0);   // KH_PSV.
    khwt_draw(true);
    ctx->OMSetRenderTargets(0, nullptr, khwt_post ? g_res.wsee_n2_dsv : g_res.wsee_n_dsv);
    ctx->OMSetBlendState(nullptr, khwt_z, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_res.dss_wsee_nw, 0);
    if (khwt_pv.on) ctx->PSSetShader(khwt_pv.nw, nullptr, 0);   // KH_PSV.
    khwt_draw(false);
    khwt_sv.end(ctx);
    // Our layer into Tp (the near push where the surface shift stood, else unpushed as its T pass) and Tf (the
    // world-range viewport alone), as kh_wsee_post does the engine's - else T would hold a layer Tp lacks and the
    // chain would take ours for a near-surface one.
    if (kh_wsee_ld_on()) {
        g_psa_tp_cycle.store(g_topo_cycles, std::memory_order_relaxed);   // KH_PSB_TPA: the replays reset Tp from here.
        if (khwt_sok) khwt_sv.begin(ctx, KH_WSEE_NEAR_TOL);
        ctx->OMSetRenderTargets(1, &g_res.wsee_tp_rtv, g_res.depth_dsv_ro);
        ctx->OMSetBlendState(khwt_post ? g_res.bs_wsee_ldpost : g_res.bs_wsee_ldpre, khwt_z, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_test, 0);
        if (khwt_pv.on) ctx->PSSetShader(khwt_pv.alpha, nullptr, 0);   // KH_PSV.
        khwt_draw(true);
        khwt_sv.end(ctx);
        // And into Tp's B / A under the 100 % band's push, as kh_wsee_post does the engine's.
        if (khwt_sok) khwt_sv.begin(ctx, KH_WSEE_NEAR_TOL2);
        ctx->OMSetBlendState(khwt_post ? g_res.bs_wsee_ldpost2 : g_res.bs_wsee_ldpre2, khwt_z, 0xFFFFFFFF);
        if (khwt_pv.on) ctx->PSSetShader(khwt_pv.alpha, nullptr, 0);   // KH_PSV.
        khwt_draw(true);
        khwt_sv.end(ctx);
        if (g_wsee_ld_far_ok && kh_wsee_vp_world(ctx)) {
            ctx->OMSetRenderTargets(1, &g_res.wsee_tf_rtv, g_res.wsee_tfd_dsv);
            ctx->OMSetDepthStencilState(g_res.dss_wsee_far, 0);
            if (khwt_pv.on) ctx->PSSetShader(khwt_pv.alpha, nullptr, 0);   // KH_PSV.
            khwt_draw(true);
        }
    }
    khwt_om.restore(ctx);
    khwt_om.release();
    ctx->OMSetBlendState(khwt_bs, khwt_bf, khwt_mask);
    ctx->OMSetDepthStencilState(khwt_dss, khwt_ref);
    if (khwt_pv.on) ctx->PSSetShader(khwt_pv.orig, nullptr, 0);   // KH_PSV: the builtin back (the flush's trackers).
    KH_SAFE_RELEASE(khwt_pv.orig);
    KH_SAFE_RELEASE(khwt_bs);
    KH_SAFE_RELEASE(khwt_dss);
    return true;
}

// The capture at the injection's translucent tail (inject_composited_meshes, after its solids and their AO, before
// its first translucent part): S and B hold our solids and none of our layers; the engine's later blended draws find
// it landed and only re-issue. In a wanted cycle not yet attempted (g_wsee_on), with the trigger's world viewport
// recorded (trig_vp_valid) and this injection the world's own (khwi_world: not an anomalous rescue, the engine's
// viewport at its draw a world one - trig_vp_valid alone is sticky across a re-armed attempt). RT0 must be the scene
// texture and the bound depth the main one (kh_wsee_snapshot's checks). Render thread, under in_injection.
inline void kh_wsee_inj_capture(ID3D11DeviceContext* ctx, bool khwi_world) {
    const uint64_t khwi_c = g_topo_cycles;
    if (!g_wsee_on || !khwi_world || !g_ro.trig_vp_valid || g_wsee_snap_cycle == khwi_c ||
        g_wsee_used_cycle.load(std::memory_order_relaxed) == khwi_c ||
        g_wsee_ok_cycle.load(std::memory_order_relaxed) == khwi_c ||
        g_wsee_taint_cycle.load(std::memory_order_relaxed) == khwi_c) return;
    ID3D11RenderTargetView* khwi_rtv = nullptr;
    ctx->OMGetRenderTargets(1, &khwi_rtv, nullptr);
    if (!khwi_rtv) return;
    bool khwi_scene = false;
    {
        ID3D11Resource* khwi_res = nullptr;
        khwi_rtv->GetResource(&khwi_res);
        khwi_scene = khwi_res != nullptr && g_topo_scene_tex_id != nullptr &&
                     static_cast<void*>(khwi_res) == g_topo_scene_tex_id;
        if (khwi_res) khwi_res->Release();
    }
    if (khwi_scene) {
        g_wsee_snap_cycle = khwi_c;
        if (kh_wsee_snapshot(ctx, khwi_rtv)) g_wsee_ok_cycle.store(khwi_c, std::memory_order_relaxed);
    }
    khwi_rtv->Release();
}

// Whether the engine's latched pair's near differs from the pair given (pv: the flush's, or the injection's own
// encode) by more than the sort's relative tolerance (KH_WSEE_MS_REL, 2e-3: the decode's error in metres follows the
// near): a partitioned cycle, whose main depth holds raws of more than one encode. An unpartitioned cycle's pv is the
// engine's world pair by construction (snap_world), so the two agree to rounding. No latched pair: false. Used by
// the flush's sort mode and the injection's deferral gate.
inline bool kh_wsee_pair_split(float khps_m22, float khps_m32) {
    if (!g_ro.engine_proj_valid) return false;
    const float khps_pn = fabsf(khps_m22) > 1e-9f ? -khps_m32 / khps_m22 : -1.0f;
    const float khps_en = fabsf(g_ro.engine_m22) > 1e-9f ? -g_ro.engine_m32 / g_ro.engine_m22 : -1.0f;
    return !(khps_pn > 0.0f) || !(khps_en > 0.0f) || fabsf(khps_en - khps_pn) > 2.0e-3f * khps_pn;
}

// What the sorted draw binds and blends with, all made: S, T, B, NWpre, NWpost and the main depth's view (MS: the
// mesh unit declares T and it as Texture2DMS; a landed capture implies it), no depth test, the normal blend and the
// layer-aware composite's dual-source state.
inline bool kh_wsee_sort_ready() {
    return g_res.depth_srv && g_res.wsee_s_srv && g_res.wsee_t_srv && g_res.wsee_b_srv && g_res.wsee_n_srv &&
           g_res.wsee_n2_srv &&   // NWpost (t59 for the mesh unit).
           g_res.wsee_b_tex && g_res.dss_off && g_res.blend_modes[0] && g_res.bs_wsee_sort &&
           g_res.depth_sample_count >= 2;
}

// The composite's capture Q (Resources' note): made with B's desc (the scene target's view
// format, single sample), kept while it matches B; false = none (a refused make latches until the device goes).
inline bool kh_wsee_q_ensure(ID3D11Device* dev) {
    if (!dev || !g_res.wsee_b_tex) return false;
    D3D11_TEXTURE2D_DESC khwq_bd = {};
    g_res.wsee_b_tex->GetDesc(&khwq_bd);
    if (g_res.wsee_q_tex && g_res.wsee_q_srv) {
        D3D11_TEXTURE2D_DESC khwq_qd = {};
        g_res.wsee_q_tex->GetDesc(&khwq_qd);
        if (khwq_qd.Width == khwq_bd.Width && khwq_qd.Height == khwq_bd.Height && khwq_qd.Format == khwq_bd.Format &&
            khwq_qd.SampleDesc.Count == 1) return true;
    }
    KH_SAFE_RELEASE(g_res.wsee_q_srv);
    KH_SAFE_RELEASE(g_res.wsee_q_tex);
    if (g_res.wsee_q_failed) return false;
    if (FAILED(dev->CreateTexture2D(&khwq_bd, nullptr, &g_res.wsee_q_tex)) || !g_res.wsee_q_tex ||
        FAILED(dev->CreateShaderResourceView(g_res.wsee_q_tex, nullptr, &g_res.wsee_q_srv)) || !g_res.wsee_q_srv) {
        KH_SAFE_RELEASE(g_res.wsee_q_srv);
        KH_SAFE_RELEASE(g_res.wsee_q_tex);
        g_res.wsee_q_failed = true;
        report_error_once_safe("KH world see-through: the sorted tail's scene capture could not be made (our "
                               "translucent parts draw where they did, before the game's later layers)");
        return false;
    }
    return true;
}

// The texture Q resolves from, given an RT0 (khwq_rtv): the scene texture
// (g_topo_scene_tex_id) in B's view format, multisampled at the capture's count and size - what the capture resolved
// into B. A reference back, or null.
inline ID3D11Texture2D* kh_wsee_q_source(ID3D11RenderTargetView* khwq_rtv) {
    if (!khwq_rtv || !g_res.wsee_b_tex || g_topo_scene_tex_id == nullptr) return nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC khwq_vd = {};
    khwq_rtv->GetDesc(&khwq_vd);
    if (khwq_vd.Format != g_res.wsee_b_fmt || khwq_vd.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2DMS) return nullptr;
    ID3D11Resource* khwq_r = nullptr;
    khwq_rtv->GetResource(&khwq_r);
    if (!khwq_r) return nullptr;
    ID3D11Texture2D* khwq_t = nullptr;
    if (static_cast<void*>(khwq_r) != g_topo_scene_tex_id ||
        FAILED(khwq_r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khwq_t)))) {
        khwq_t = nullptr;
    }
    khwq_r->Release();
    if (!khwq_t) return nullptr;
    D3D11_TEXTURE2D_DESC khwq_d = {};
    khwq_t->GetDesc(&khwq_d);
    if (khwq_d.Width != g_res.wsee_w || khwq_d.Height != g_res.wsee_h ||
        khwq_d.SampleDesc.Count != g_res.wsee_samples || khwq_d.MipLevels != 1 || khwq_d.ArraySize != 1) {
        khwq_t->Release();
        return nullptr;
    }
    return khwq_t;
}

// Q taken: the bound RT0 (the flush's scene target) resolved into it through the ORIGINAL resolve (kh_wsee_snapshot's
// rule), with no target bound meanwhile (Q is off t58: the scope nulls it at its end). The flush is g_kh_flush_active,
// so the OM hooks stand aside. False = not taken (nothing changed).
inline bool kh_wsee_q_capture(ID3D11DeviceContext* ctx) {
    if (!ctx || !g_res.wsee_q_tex || !g_orig_resolvesubresource) return false;
    KhOmSave khwq_om;
    khwq_om.capture(ctx);
    ID3D11Texture2D* const khwq_src = kh_wsee_q_source(khwq_om.rtv[0]);
    if (!khwq_src) {
        khwq_om.release();
        return false;
    }
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    g_orig_resolvesubresource(ctx, g_res.wsee_q_tex, 0, khwq_src, 0, g_res.wsee_b_fmt);
    khwq_om.restore(ctx);
    khwq_om.release();
    khwq_src->Release();
    return true;
}

// Whether the injection leaves its translucent parts to the flush's tail this cycle: the capture wanted and landed
// untainted and unused; the cycle's render-thread scene flush still to come (g_rtshadow_cycle: not yet run this
// cycle) and alive (flush_frame's test, a render-thread flush within two cycles: under a park regime - the bootstrap,
// KH_WATCH_STALL, an unnamed scene texture - the park's flush is another cycle's and would drop the parts at the
// clear); every view and state made (kh_wsee_sort_ready, the dual-source state included), Q made and the bound
// target one Q resolves (any scene format); no deferral back-off (g_wsee_defer_back). Else the parts draw at the
// injection and into T / NW. What can still change by the flush (a taint, the target) falls back to the flush's
// plain tail. The caller adds: the world's own injection, no far phase and no partitioned pair (kh_wsee_pair_split).
inline bool kh_wsee_defer_ok(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    const uint64_t khwd_c = g_topo_cycles;
    const uint64_t khwd_last = g_rtshadow_cycle.load(std::memory_order_relaxed);
    if (!(g_wsee_on && khwd_c >= g_wsee_defer_back &&
          g_wsee_ok_cycle.load(std::memory_order_relaxed) == khwd_c &&
          g_wsee_taint_cycle.load(std::memory_order_relaxed) != khwd_c &&
          g_wsee_used_cycle.load(std::memory_order_relaxed) != khwd_c && reorder_on_render_thread() &&
          g_kh_track_wanted.load(std::memory_order_relaxed) &&
          khwd_last != khwd_c && khwd_last != ~0ull && khwd_c - khwd_last <= 2ull && kh_wsee_sort_ready() &&
          kh_wsee_q_ensure(dev))) return false;
    ID3D11RenderTargetView* khwd_rtv = nullptr;
    ctx->OMGetRenderTargets(1, &khwd_rtv, nullptr);
    ID3D11Texture2D* const khwd_src = kh_wsee_q_source(khwd_rtv);
    if (khwd_rtv) khwd_rtv->Release();
    if (!khwd_src) return false;
    khwd_src->Release();
    return true;
}

// KH_WSEE - one deferred object (its copy, as the injection drew it; draw_part 1: the flush draws the translucent
// part alone). The record is the cycle's (g_wsee_defer_cycle's note).
inline void kh_wsee_defer_note(const RenderObject& khwn_o) {
    const uint64_t khwn_c = g_topo_cycles;
    if (g_wsee_defer_cycle != khwn_c) {
        g_wsee_defer_objs.clear();
        g_wsee_defer_cycle = khwn_c;
    }
    g_wsee_defer_objs.push_back(khwn_o);
    g_wsee_defer_objs.back().draw_part = 1;
    g_wsee_defer_objs.back().ws_dcopy = 1u;   // KH_WSEE_DEFER_FLAG: the flush's tail knows the copy by this.
}

// Deferring a part whose geometry is the object's own (a skinned pose, a cloth / chain buffer, a vertex stage's
// output): those buffers are rewritten or recycled before the flush's tail (its uploads, pool grows and rebinds, the
// consume-point steps, the skin catch-up, a park's flush), so the part's vertices are COPIED at the injection, where
// its object is noted (after the cycle's steps and the solids that read them, before any later writer), by GPU copies
// on the immediate context: every later write, whatever thread or pass issues it, is ordered after them. Pinning the
// slot's buffer would not do (Map DISCARD renames its memory, stream output writes in place), nor holding the uploads
// back (every other pass draws from the slot). own: an object drawn from its mesh's own buffer needs no copy - its
// entry names that buffer and the flush binds it whatever the slot holds by then; KH_WSEE_OWN_STATIC: a static object
// takes such an entry too (the slot may gain a cloth, pose or stage buffer before the flush's tail, and the entry
// carries the injection's level and crossfade). Only what the flush can draw is copied: the translucent submeshes
// (alpha mode 2) at the injection's LOD pick (kh_wsee_hold_lod), as vertex runs (kh_wsee_spans_of; gaps <=
// KH_WSEE_RUN_GAP merged), each at its own place relative to base, the IA offset the flush binds (vertex v at base + 48
// v; DrawIndexed with base vertex 0 and the mesh's absolute indices, all inside the runs; base may lie on earlier
// entries' bytes, which no index of this entry reaches). The entry spans [off, off + bytes) = vmin .. vmax. Nothing
// translucent at those levels: own, no copy. A mesh whose index ranges cannot be read, or an object without a material
// set, copies the whole array.
struct KhWseeRun {
    uint32_t a = 0u;   // Vertices [a, b).
    uint32_t b = 0u;
};
struct KhWseeHoldPlan {
    int           mesh = -1;
    bool          own = false;
    ID3D11Buffer* src = nullptr;
    UINT          off = 0u;       // The source's vertex 0 (its slice's offset).
    int           lvl = 0;        // The injection's pick (kh_wsee_hold_lod's).
    float         t = 0.0f;
    uint32_t      r0 = 0u;        // Its runs in g_wsee_run_plan [r0, r1).
    uint32_t      r1 = 0u;
    UINT          h0 = 0u;        // Its place in the hold (kh_wsee_defer_group's layout): vmin at h0.
    UINT          base = 0u;
};
static constexpr UINT KH_WSEE_HOLD_ALIGN = 256u;
static constexpr UINT KH_WSEE_HOLD_MIN = 1u << 20;   // The hold buffer's first size (bytes).
static constexpr UINT KH_WSEE_HOLD_CAP = 1u << 30;   // The hold's largest size (bytes).
static constexpr size_t KH_WSEE_HOLD_MAX = 0xFFFFu;   // Entries a cycle (RenderObject::ws_hold's 16 bits).
static constexpr uint32_t KH_WSEE_RUN_GAP = 256u;   // Vertices: a smaller gap is copied (12 KB) rather than split.
static std::vector<KhWseeRun> g_wsee_run_plan;   // The runs of kh_wsee_defer_group's current plans.
// Per mesh (ids are never reused; a mesh's index stream never changes): the vertex runs of every (level, submesh)
// index range, levels 0 to lod_n in order, each level's submesh table in order (slot lvl_slot[level] + submesh); slot
// k's runs are runs[first[k], first[k + 1]). Built once from the CPU index stream against nv vertices (the GPU
// array's count); ok false: unreadable - the caller copies the whole array. Render thread; cleared with the hold
// (kh_wsee_hold_drop).
struct KhWseeSpans {
    bool built = false;
    bool ok = false;
    uint32_t nv = 0u;
    std::vector<uint32_t> lvl_slot;
    std::vector<uint32_t> first;
    std::vector<KhWseeRun> runs;
};
static std::unordered_map<int, KhWseeSpans> g_wsee_spans;
inline void kh_wsee_runs_merge(std::vector<KhWseeRun>& khrm_v, size_t khrm_from) {   // Sorted by a, merged in place.
    std::sort(khrm_v.begin() + static_cast<std::ptrdiff_t>(khrm_from), khrm_v.end(),
              [](const KhWseeRun& x, const KhWseeRun& y) { return x.a < y.a; });
    size_t khrm_o = khrm_from;
    for (size_t khrm_i = khrm_from; khrm_i < khrm_v.size(); ++khrm_i) {
        if (khrm_o > khrm_from && khrm_v[khrm_i].a <= khrm_v[khrm_o - 1u].b + KH_WSEE_RUN_GAP) {
            if (khrm_v[khrm_i].b > khrm_v[khrm_o - 1u].b) khrm_v[khrm_o - 1u].b = khrm_v[khrm_i].b;
        } else {
            khrm_v[khrm_o++] = khrm_v[khrm_i];
        }
    }
    khrm_v.resize(khrm_o);
}
inline const KhWseeSpans& kh_wsee_spans_of(int khws_m, uint32_t khws_nv) {
    KhWseeSpans& khws_s = g_wsee_spans[khws_m];
    if (khws_s.built && khws_s.nv == khws_nv) return khws_s;
    khws_s = KhWseeSpans{};
    khws_s.built = true;
    khws_s.nv = khws_nv;
    const MeshDef& khws_d = mesh_def(khws_m);
    const size_t khws_in = khws_d.indices.size();
    if (khws_in == 0u || khws_nv == 0u || khws_d.lod_n > KH_LOD_MAX) return khws_s;
    static std::vector<uint32_t> khws_seen;
    khws_seen.assign(khws_nv, 0u);
    uint32_t khws_tick = 0u;
    for (int khws_l = 0; khws_l <= static_cast<int>(khws_d.lod_n); ++khws_l) {
        khws_s.lvl_slot.push_back(static_cast<uint32_t>(khws_s.first.size()));
        const std::vector<MeshSubmesh>& khws_tab = khws_l == 0 ? khws_d.submeshes : khws_d.lod_sub[khws_l - 1];
        for (const MeshSubmesh& khws_sm : khws_tab) {
            khws_s.first.push_back(static_cast<uint32_t>(khws_s.runs.size()));
            if (khws_sm.index_count == 0u) continue;
            if (khws_sm.index_start > khws_in || khws_sm.index_count > khws_in - khws_sm.index_start) return khws_s;
            ++khws_tick;
            const size_t khws_r0 = khws_s.runs.size();
            for (uint32_t khws_i = 0; khws_i < khws_sm.index_count; ++khws_i) {
                const uint32_t khws_v = khws_d.indices[khws_sm.index_start + khws_i];
                if (khws_v >= khws_nv) return khws_s;
                if (khws_seen[khws_v] == khws_tick) continue;
                khws_seen[khws_v] = khws_tick;
                khws_s.runs.push_back({ khws_v, khws_v + 1u });
            }
            kh_wsee_runs_merge(khws_s.runs, khws_r0);
        }
    }
    khws_s.lvl_slot.push_back(static_cast<uint32_t>(khws_s.first.size()));
    khws_s.first.push_back(static_cast<uint32_t>(khws_s.runs.size()));
    khws_s.ok = true;
    return khws_s;
}
// What holding khwp_o's geometry takes, at the injection's pick (khwp_lvl, and khwp_lvl + 1 where khwp_t > 0): the
// vertices its draws read now (kh_mesh_vb_ref - the buffer and its slice's offset), the runs the flush can draw of them
// appended to g_wsee_run_plan; false = it cannot be held (no mesh buffer, a source smaller than the array, the hold
// itself).
inline bool kh_wsee_hold_plan(const RenderObject& khwp_o, int khwp_lvl, float khwp_t, KhWseeHoldPlan& khwp_p) {
    khwp_p = KhWseeHoldPlan{};   // KH_WSEE_OWN_STATIC: every object plans an entry (own where the slot is the mesh's).
    const int khwp_m = mesh_id_clamp(khwp_o.mesh);
    if (khwp_m < 0 || static_cast<size_t>(khwp_m) >= g_res.mesh_vb.size() || !g_res.mesh_vb[khwp_m]) return false;
    khwp_p.mesh = khwp_m;
    khwp_p.lvl = khwp_lvl;
    khwp_p.t = khwp_t > 0.0f ? khwp_t : 0.0f;
    const KhVbRef khwp_r = kh_mesh_vb_ref(khwp_m, khwp_o.slot);
    if (khwp_r.vb == g_res.mesh_vb[khwp_m]) {
        khwp_p.own = true;
        return true;
    }
    if (!khwp_r.vb || khwp_r.vb == g_res.wsee_hold_vb) return false;
    D3D11_BUFFER_DESC khwp_md = {}, khwp_sd = {};
    g_res.mesh_vb[khwp_m]->GetDesc(&khwp_md);
    khwp_r.vb->GetDesc(&khwp_sd);
    if (khwp_md.ByteWidth < sizeof(MeshVertex) || khwp_sd.Usage == D3D11_USAGE_STAGING ||
        khwp_r.off > khwp_sd.ByteWidth || khwp_sd.ByteWidth - khwp_r.off < khwp_md.ByteWidth ||
        khwp_md.ByteWidth > KH_WSEE_HOLD_CAP) return false;
    khwp_p.src = khwp_r.vb;
    khwp_p.off = khwp_r.off;
    const uint32_t khwp_nv = khwp_md.ByteWidth / static_cast<UINT>(sizeof(MeshVertex));
    khwp_p.r0 = static_cast<uint32_t>(g_wsee_run_plan.size());
    const KhWseeSpans& khwp_s = kh_wsee_spans_of(khwp_m, khwp_nv);
    const KhMaterialSet* const khwp_ms = kh_obj_textured(khwp_o);
    if (!khwp_s.ok || !khwp_ms) {
        g_wsee_run_plan.push_back({ 0u, khwp_nv });   // The whole array.
    } else {
        const MeshDef& khwp_d = mesh_def(khwp_m);
        for (int khwp_k = 0; khwp_k < (khwp_p.t > 0.0f ? 2 : 1); ++khwp_k) {
            const int khwp_l = mesh_lod_clamp(khwp_d, khwp_lvl + khwp_k);   // kh_draw_textured's clamp.
            if (khwp_k == 1 && khwp_l == mesh_lod_clamp(khwp_d, khwp_lvl)) break;
            const uint32_t khwp_s0 = khwp_s.lvl_slot[static_cast<size_t>(khwp_l)];
            const uint32_t khwp_s1 = khwp_s.lvl_slot[static_cast<size_t>(khwp_l) + 1u];
            for (uint32_t khwp_si = 0; khwp_s0 + khwp_si < khwp_s1; ++khwp_si) {
                // kh_draw_textured's part-2 test: a used slot of alpha mode 2 (an unused slot's default is opaque).
                if (khwp_si >= khwp_ms->slots.size() || !khwp_ms->slots[khwp_si].used ||
                    khwp_ms->slots[khwp_si].alpha_mode != 2) continue;
                const uint32_t khwp_k0 = khwp_s.first[khwp_s0 + khwp_si];
                const uint32_t khwp_k1 = khwp_s.first[khwp_s0 + khwp_si + 1u];
                for (uint32_t khwp_j = khwp_k0; khwp_j < khwp_k1; ++khwp_j) {
                    g_wsee_run_plan.push_back(khwp_s.runs[khwp_j]);
                }
            }
        }
        kh_wsee_runs_merge(g_wsee_run_plan, khwp_p.r0);
    }
    khwp_p.r1 = static_cast<uint32_t>(g_wsee_run_plan.size());
    if (khwp_p.r1 == khwp_p.r0) {   // Nothing translucent at those levels: nothing the flush draws reads a vertex.
        khwp_p.own = true;
        khwp_p.src = nullptr;
    }
    return true;
}
inline void kh_wsee_hold_begin() {   // The cycle's first hold drops the last cycle's entries.
    if (g_wsee_hold_cycle != g_topo_cycles) {
        g_wsee_hold_cycle = g_topo_cycles;
        g_wsee_hold.clear();
        g_wsee_hold_used = 0u;
    }
}
// Room in the hold up to khwr_end bytes (this cycle's entries kept: a growth copies them to the new buffer's start,
// ordered before anything later). False = refused (past KH_WSEE_HOLD_CAP, or the device refused the buffer, now or
// before at this size or a smaller one: KH_WSEE_HOLD_FAIL, Resources::wsee_hold_fail_cap).
inline bool kh_wsee_hold_reserve(ID3D11Device* dev, ID3D11DeviceContext* ctx, UINT khwr_end) {
    kh_wsee_hold_begin();
    if (khwr_end > KH_WSEE_HOLD_CAP) return false;
    if (g_res.wsee_hold_vb && khwr_end <= g_res.wsee_hold_cap) return true;
    if (!dev) return false;
    UINT khwr_cap = g_res.wsee_hold_cap > KH_WSEE_HOLD_MIN ? g_res.wsee_hold_cap : KH_WSEE_HOLD_MIN;
    while (khwr_cap < khwr_end) khwr_cap *= 2u;
    // KH_WSEE_HOLD_FAIL: a size the device refused (or a larger one) is not asked for every cycle;
    // KH_WSEE_HOLD_FAIL_RETRY: the ask goes through once KH_WSEE_HOLD_FAIL_RETRY cycles after the refusal (one
    // CreateBuffer per that many cycles while it keeps failing).
    if (g_res.wsee_hold_fail_cap != 0u && khwr_cap >= g_res.wsee_hold_fail_cap &&
        g_topo_cycles >= g_res.wsee_hold_fail_cycle &&   // A cycle count that started over: ask again.
        g_topo_cycles - g_res.wsee_hold_fail_cycle < KH_WSEE_HOLD_FAIL_RETRY) return false;
    D3D11_BUFFER_DESC khwr_d = {};
    khwr_d.ByteWidth = khwr_cap;
    khwr_d.Usage = D3D11_USAGE_DEFAULT;
    khwr_d.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    ID3D11Buffer* khwr_vb = nullptr;
    if (FAILED(dev->CreateBuffer(&khwr_d, nullptr, &khwr_vb)) || !khwr_vb) {
        g_res.wsee_hold_fail_cap = khwr_cap;   // KH_WSEE_HOLD_FAIL.
        g_res.wsee_hold_fail_cycle = g_topo_cycles;   // KH_WSEE_HOLD_FAIL_RETRY.
        return false;
    }
    if (khwr_cap >= g_res.wsee_hold_fail_cap) {   // KH_WSEE_HOLD_FAIL_RETRY: a retry at the refused size succeeded.
        g_res.wsee_hold_fail_cap = 0u;             // (A smaller size's success proves nothing about it: kept.)
        g_res.wsee_hold_fail_cycle = 0u;
    }
    if (g_res.wsee_hold_vb && g_wsee_hold_used > 0u) {
        const D3D11_BOX khwr_b = { 0u, 0u, 0u, g_wsee_hold_used, 1u, 1u };
        ctx->CopySubresourceRegion(khwr_vb, 0, 0, 0, 0, g_res.wsee_hold_vb, 0, &khwr_b);
    }
    if (!g_res.wsee_hold_vb) ++g_res.wsee_hold_gen;   // A new hold: entries of an older one are void.
    KH_SAFE_RELEASE(g_res.wsee_hold_vb);
    g_res.wsee_hold_vb = khwr_vb;
    g_res.wsee_hold_cap = khwr_cap;
    return true;
}
// One planned hold taken (kh_wsee_defer_group laid it out and reserved the room, and checked the count): its runs
// copied, each to base + 48 a, and its entry, 1-based (the copy's ws_hold).
inline uint16_t kh_wsee_hold_take(ID3D11DeviceContext* ctx, const KhWseeHoldPlan& khwt_p) {
    KhWseeHold khwt_h;
    khwt_h.mesh = khwt_p.mesh;
    khwt_h.own = khwt_p.own;
    khwt_h.gen = g_res.wsee_hold_gen;
    khwt_h.lvl = khwt_p.lvl;
    khwt_h.t = khwt_p.t;
    if (!khwt_p.own) {
        g_wsee_hold_copy_cycle = g_topo_cycles;   // KH_WSEE_HOLD_IDLE_COPY: the buffer is in use this cycle.
        const UINT khwt_vs = static_cast<UINT>(sizeof(MeshVertex));
        khwt_h.base = khwt_p.base;
        khwt_h.off = khwt_p.h0;
        khwt_h.bytes = (g_wsee_run_plan[khwt_p.r1 - 1u].b - g_wsee_run_plan[khwt_p.r0].a) * khwt_vs;
        for (uint32_t khwt_j = khwt_p.r0; khwt_j < khwt_p.r1; ++khwt_j) {
            const KhWseeRun& khwt_r = g_wsee_run_plan[khwt_j];
            const D3D11_BOX khwt_b = { khwt_p.off + khwt_r.a * khwt_vs, 0u, 0u,
                                       khwt_p.off + khwt_r.b * khwt_vs, 1u, 1u };
            ctx->CopySubresourceRegion(g_res.wsee_hold_vb, 0, khwt_p.base + khwt_r.a * khwt_vs, 0, 0, khwt_p.src, 0,
                                       &khwt_b);
        }
        if (khwt_h.off + khwt_h.bytes > g_wsee_hold_used) g_wsee_hold_used = khwt_h.off + khwt_h.bytes;
    }
    g_wsee_hold.push_back(khwt_h);
    return static_cast<uint16_t>(g_wsee_hold.size());
}
// A held copy's entry (the flush): null when void - the hold released or remade since (g_res.wsee_hold_gen), another
// mesh's, or of another cycle (g_wsee_hold_cycle).
inline const KhWseeHold* kh_wsee_hold_of(const RenderObject& khwh_o) {
    if (khwh_o.ws_hold == 0u || khwh_o.ws_hold > g_wsee_hold.size() || g_wsee_hold_cycle != g_topo_cycles) {
        return nullptr;
    }
    const KhWseeHold& khwh_h = g_wsee_hold[khwh_o.ws_hold - 1u];
    if (khwh_h.mesh != mesh_id_clamp(khwh_o.mesh) || khwh_h.mesh < 0 ||
        static_cast<size_t>(khwh_h.mesh) >= g_res.mesh_vb.size()) return nullptr;
    if (khwh_h.own) return &khwh_h;
    if (khwh_h.gen != g_res.wsee_hold_gen || !g_res.wsee_hold_vb || khwh_h.off > g_res.wsee_hold_cap ||
        g_res.wsee_hold_cap - khwh_h.off < khwh_h.bytes) return nullptr;
    return &khwh_h;
}
// The vertices a held copy draws from (kh_mesh_vb_ref's stand-in for it): its mesh's own buffer, or the hold at the
// entry's base; null when the entry is void.
inline KhVbRef kh_wsee_hold_ref(const RenderObject& khwh_o) {
    KhVbRef khwh_r;
    const KhWseeHold* const khwh_h = kh_wsee_hold_of(khwh_o);
    if (!khwh_h) return khwh_r;
    if (khwh_h->own) {
        khwh_r.vb = g_res.mesh_vb[khwh_h->mesh];
        return khwh_r;
    }
    khwh_r.vb = g_res.wsee_hold_vb;
    khwh_r.off = khwh_h->base;
    return khwh_r;
}
// The level and crossfade a held copy draws at - the injection's pick, which its copy holds
// (level 0, no fade, for a void entry: the flush appends none).
inline void kh_wsee_hold_lod(const RenderObject& khwl_o, int& khwl_lvl, float& khwl_t) {
    const KhWseeHold* const khwl_h = kh_wsee_hold_of(khwl_o);
    khwl_lvl = khwl_h ? khwl_h->lvl : 0;
    khwl_t = khwl_h ? khwl_h->t : 0.0f;
}
// The hold and its run cache gone (kh_wsee_cycle_note's idle rule) - each only where it was made (the hold's
// release also where only its refusal latch stands: KH_WSEE_HOLD_FAIL).
inline bool kh_wsee_hold_made() { return g_res.wsee_hold_vb != nullptr || !g_wsee_spans.empty(); }
inline void kh_wsee_hold_drop() {
    if (g_res.wsee_hold_vb || g_res.wsee_hold_fail_cap != 0u) g_res.release_wsee_hold();
    if (!g_wsee_spans.empty()) g_wsee_spans.clear();
}
// One part's objects deferred together (a lone object, or a bucket's visible members), each at the injection's pick
// (khwg_lvl / khwg_t: its level and crossfade), each with an entry - its geometry held, or its mesh buffer named
// (own) - all or none. False: none deferred (a hold refused; the part draws at the injection, into T / NW).
inline bool kh_wsee_defer_group(ID3D11Device* dev, ID3D11DeviceContext* ctx, const RenderObject* const* khwg_o,
                                const int* khwg_lvl, const float* khwg_t, size_t khwg_n) {
    static std::vector<KhWseeHoldPlan> khwg_p;
    khwg_p.resize(khwg_n);
    g_wsee_run_plan.clear();
    uint64_t khwg_refused = 0u, khwg_held = 0u;
    for (size_t khwg_i = 0; khwg_i < khwg_n; ++khwg_i) {
        if (!kh_wsee_hold_plan(*khwg_o[khwg_i], khwg_lvl[khwg_i], khwg_t[khwg_i], khwg_p[khwg_i])) {
            ++khwg_refused;
            continue;
        }
        ++khwg_held;   // KH_WSEE_OWN_STATIC: every planned object takes an entry (an own one no room).
    }
    if (khwg_refused == 0u && khwg_held > 0u) {
        kh_wsee_hold_begin();
        // The layout, in take order: each copy's vmin at the first 256-aligned place past the last one's end and at
        // least 48 vmin bytes in (its base must not go below 0). A group of own entries alone lays out nothing and
        // makes no buffer (khwg_end stays g_wsee_hold_used).
        uint64_t khwg_end = g_wsee_hold_used;
        const uint64_t khwg_vs = sizeof(MeshVertex);
        for (KhWseeHoldPlan& khwg_q : khwg_p) {
            if (khwg_q.own) continue;
            const uint64_t khwg_lo = g_wsee_run_plan[khwg_q.r0].a * khwg_vs;
            const uint64_t khwg_sp = (g_wsee_run_plan[khwg_q.r1 - 1u].b - g_wsee_run_plan[khwg_q.r0].a) * khwg_vs;
            uint64_t khwg_h = khwg_end > khwg_lo ? khwg_end : khwg_lo;
            khwg_h = (khwg_h + (KH_WSEE_HOLD_ALIGN - 1u)) & ~static_cast<uint64_t>(KH_WSEE_HOLD_ALIGN - 1u);
            if (khwg_h + khwg_sp > KH_WSEE_HOLD_CAP) {
                khwg_end = static_cast<uint64_t>(KH_WSEE_HOLD_CAP) + 1u;
                break;
            }
            khwg_q.h0 = static_cast<UINT>(khwg_h);
            khwg_q.base = static_cast<UINT>(khwg_h - khwg_lo);
            khwg_end = khwg_h + khwg_sp;
        }
        if (g_wsee_hold.size() + khwg_held > KH_WSEE_HOLD_MAX || khwg_end > KH_WSEE_HOLD_CAP ||
            (khwg_end > g_wsee_hold_used && !kh_wsee_hold_reserve(dev, ctx, static_cast<UINT>(khwg_end)))) {
            khwg_refused = khwg_held;
        }
    }
    if (khwg_refused > 0u) {
        // A refusal that built run caches stamps the hold's cycle, so the idle rule keeps them (kh_wsee_cycle_note)
        // rather than dropping them at the next clear for a rebuild.
        if (!g_wsee_spans.empty()) {
            kh_wsee_hold_begin();
            g_wsee_hold_copy_cycle = g_topo_cycles;   // KH_WSEE_HOLD_IDLE_COPY.
        }
        return false;
    }
    for (size_t khwg_i = 0; khwg_i < khwg_n; ++khwg_i) {
        const uint16_t khwg_h = kh_wsee_hold_take(ctx, khwg_p[khwg_i]);
        kh_wsee_defer_note(*khwg_o[khwg_i]);
        g_wsee_defer_objs.back().ws_hold = khwg_h;
    }
    return true;
}

// The flush's sort mode for this run's tail parts (flush_locked, once before its loop): 0 = none - the capture not
// standing (tainted, refused, off, used), a view missing or a partitioned cycle: the hardware depth test and the plain
// blend, into T / NW as the tail does; 1 = the shader's occlusion test against the record (no hardware test), plain
// blend - the dual-source state refused (the injection then defers nothing) or (the flush, per run) Q not taken; 2 =
// the test and the layer-aware composite (bs_wsee_sort) on a float scene target; 3 = the same on any other (the
// shader holds colour and alpha to [0, 1], as UNORM / SRGB targets clamp the blend's inputs). The format is the
// flush's RT0 view's.
inline int kh_wsee_sort_mode(ID3D11DeviceContext* ctx, float khsm_m22, float khsm_m32) {
    if (!kh_wsee_tail_owed() || !g_res.depth_srv || !g_res.wsee_s_srv || !g_res.wsee_t_srv ||
        !g_res.wsee_b_srv || !g_res.wsee_n_srv || !g_res.wsee_n2_srv || !g_res.dss_off || !g_res.blend_modes[0] ||
        g_res.depth_sample_count < 2) return 0;   // The mesh unit declares T and the depth multisampled.
    // A partitioned cycle (kh_wsee_pair_split) holds more than one encode in the main depth, which no one decode
    // reads: none.
    if (kh_wsee_pair_split(khsm_m22, khsm_m32)) return 0;
    if (!g_res.bs_wsee_sort) return 1;
    ID3D11RenderTargetView* khsm_rtv = nullptr;
    ctx->OMGetRenderTargets(1, &khsm_rtv, nullptr);
    if (!khsm_rtv) return 0;
    D3D11_RENDER_TARGET_VIEW_DESC khsm_vd = {};
    khsm_rtv->GetDesc(&khsm_vd);
    khsm_rtv->Release();
    const DXGI_FORMAT khsm_f = khsm_vd.Format;
    // The float family by name; R32G32B32_FLOAT is never met - D3D11 gives the 96-bit formats no
    // multisampled render target, and the target here is drawn over the multisampled main depth (counts match).
    const bool khsm_float = khsm_f == DXGI_FORMAT_R32G32B32A32_FLOAT || khsm_f == DXGI_FORMAT_R32G32B32_FLOAT ||
                            khsm_f == DXGI_FORMAT_R16G16B16A16_FLOAT || khsm_f == DXGI_FORMAT_R32G32_FLOAT ||
                            khsm_f == DXGI_FORMAT_R11G11B10_FLOAT || khsm_f == DXGI_FORMAT_R16G16_FLOAT ||
                            khsm_f == DXGI_FORMAT_R32_FLOAT || khsm_f == DXGI_FORMAT_R16_FLOAT;
    return khsm_float ? 2 : 3;
}

// One sorted part's colour draw: RT0 alone (a dual-source blend takes no second target) over depth_dsv_ro, dss_off
// (PSMain's own test against the record, cb.hlsl KhWseeMeshCut), the mode's blend (2 / 3 bs_wsee_sort; 1
// blend_modes[0]), the mask all and A2C off (the shader's per-sample verdict, SV_Coverage, is ANDed with the raster's
// alone). The mesh unit's views (cb.hlsl khMs*): t53 S, t54 T, t55 B, t56 the main depth's MS view (beside its
// read-only DSV), t57 NWpre, t58 Q (null in mode 1), t59 NWpost, t60 B2 (modes 2 / 3 where this cycle took it: b2;
// lane bit 3 = b2m, the B2 moment, in every mode - the chain's meaning), t62 S2 (every mode where taken: s2, bit 9;
// np = bit 8, a pre mark skipped), t65 D3 (every mode while it
// stands complete: od, bit 5), t66 - t69 P0 / P1 / N / N2 (pb, bit 6; pe, bit 7) - the caller's lanes. end (and the
// destructor) nulls the views first (the main depth's before the flush's writable view of it is bound again, Q before
// the next resolve into it), then sets back the targets (KhOmSave), blend and depth states. The flush is
// g_kh_flush_active: the OM hooks stand aside. begin false = nothing changed (no target bound, or the bound depth is
// not the capture's): the caller draws as before.
struct KhWseeSortScope {
    KhOmSave                 om;
    ID3D11BlendState*        bs = nullptr;
    FLOAT                    bf[4] = {};
    UINT                     mask = 0xFFFFFFFFu;
    ID3D11DepthStencilState* dss = nullptr;
    UINT                     ref = 0;
    ID3D11DeviceContext*     ctx = nullptr;
    bool                     b2 = false;   // B2 bound at t60 (modes 2 / 3; b2m with it).
    // KH_WSEE_B2M: this cycle's B2 moment stands (S2 taken with it: s2 too) - lane bit 3 in every mode, as the
    // chain's (KhWseeMeshCut's derived pre opaque then reads S2 behind, as KhWseeRule's khwr_b; mode 1 binds no B2 and
    // reads none).
    bool                     b2m = false;
    bool                     od = false;   // D3 bound at t65 (lane bit 5).
    bool                     pb = false;   // KH_PSB: the one pass's record bound at t66 - t69 (lane bit 6).
    bool                     pe = false;   // KH_PSB: the old record holds no particle-type layer (lane bit 7).
    bool                     np = false;   // KH_WSEE_NOPRE: a pre late opaque skipped this cycle (lane bit 8).
    bool                     s2 = false;   // KH_WSEE_NOPRE: S2 bound at t62, every mode (lane bit 9).
    KhWseeSortScope() = default;
    KhWseeSortScope(const KhWseeSortScope&) = delete;
    KhWseeSortScope& operator=(const KhWseeSortScope&) = delete;
    ~KhWseeSortScope() { end(); }
    bool begin(ID3D11DeviceContext* khss_ctx, int khss_mode) {
        om.capture(khss_ctx);
        if (!om.rtv[0] || !om.dsv || reorder_dsv_identity(om.dsv) != g_res.wsee_depth_id) {
            om.release();   // Not the capture's depth (kh_wsee_tail's rule): nothing changed.
            return false;
        }
        ctx = khss_ctx;
        ctx->OMGetBlendState(&bs, bf, &mask);
        ctx->OMGetDepthStencilState(&dss, &ref);
        ctx->OMSetRenderTargets(1, &om.rtv[0], g_res.depth_dsv_ro);
        const FLOAT khss_z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(khss_mode >= 2 ? g_res.bs_wsee_sort : g_res.blend_modes[0], khss_z, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        b2m = g_res.wsee_b2_srv != nullptr && g_res.wsee_s2_srv != nullptr &&   // KH_WSEE_B2M: the chain's khfc_b2.
              g_wsee_b2_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        b2 = khss_mode >= 2 && b2m;
        ID3D11ShaderResourceView* const khss_v[8] = { g_res.wsee_s_srv, g_res.wsee_t_srv, g_res.wsee_b_srv,
                                                      g_res.depth_srv, g_res.wsee_n_srv,
                                                      khss_mode >= 2 ? g_res.wsee_q_srv : nullptr,
                                                      g_res.wsee_n2_srv,   // NWpost at t59.
                                                      b2 ? g_res.wsee_b2_srv : nullptr };   // B2 at t60.
        ctx->PSSetShaderResources(53, 8, khss_v);
        // S2 (t62; cb.hlsl khMsS2): with B2, and (KH_WSEE_NOPRE) in every mode wherever this cycle took it - the
        // derivation's reference for a skipped pre late opaque (bit 8).
        s2 = g_res.wsee_s2_srv != nullptr && g_wsee_s2_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        np = g_wsee_nopre_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        ID3D11ShaderResourceView* const khss_s2 = (b2 || s2) ? g_res.wsee_s2_srv : nullptr;
        ctx->PSSetShaderResources(62, 1, &khss_s2);
        // D3 (t65; cb.hlsl khMsD3): a late opaque's own depth, every mode (the occlusion test reads it).
        od = kh_wsee_od_on() && kh_wsee_d3_ok(g_topo_cycles);   // KH_WSEE_OQ: complete only (its note).
        od = od && kh_wsee_d3m_resolve(ctx, g_topo_cycles);   // KH_WSEE_OQ2: the post opaques' D3m first.
        ID3D11ShaderResourceView* const khss_d3 = od ? g_res.wsee_d3_srv : nullptr;
        ctx->PSSetShaderResources(65, 1, &khss_d3);
        // KH_PSB: P0, P1, N, N2 (t66 - t69; cb.hlsl khMsP0 / khMsNb) where a re-issue drew into them this cycle, and
        // whether the old record holds a particle-type layer (else its NW holds the clear: not read).
        pb = g_res.wsee_p0_srv && g_res.wsee_p1_srv && g_res.wsee_nb_srv && g_res.wsee_nb2_srv &&   // KH_PSB_PHASE.
             g_psb_drew_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        // Bit 7 only while the one pass's record stands (else the old record is read whole).
        pe = g_psb_cycle.load(std::memory_order_relaxed) == g_topo_cycles &&
             g_psa_cycle.load(std::memory_order_relaxed) != g_topo_cycles;
        ID3D11ShaderResourceView* const khss_pb[4] = { pb ? g_res.wsee_p0_srv : nullptr,
                                                       pb ? g_res.wsee_p1_srv : nullptr,
                                                       pb ? g_res.wsee_nb_srv : nullptr,
                                                       pb ? g_res.wsee_nb2_srv : nullptr };
        ctx->PSSetShaderResources(66, 4, khss_pb);
        return true;
    }
    void end() {
        if (!ctx) return;
        ID3D11ShaderResourceView* const khss_n[8] = {};
        ctx->PSSetShaderResources(53, 8, khss_n);
        ctx->PSSetShaderResources(62, 1, khss_n);   // S2.
        ctx->PSSetShaderResources(65, 1, khss_n);   // D3.
        ctx->PSSetShaderResources(66, 4, khss_n);   // KH_PSB: P0, P1, N, N2.
        om.restore(ctx);
        om.release();
        ctx->OMSetBlendState(bs, bf, mask);
        ctx->OMSetDepthStencilState(dss, ref);
        KH_SAFE_RELEASE(bs);
        KH_SAFE_RELEASE(dss);
        ctx = nullptr;
        b2 = false;
        b2m = false;   // KH_WSEE_B2M.
        od = false;
        pb = false;   // KH_PSB.
        pe = false;
        np = false;   // KH_WSEE_NOPRE.
        s2 = false;
    }
};
}