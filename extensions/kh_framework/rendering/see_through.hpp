#pragma once

// The injection half (rendering_integration_injection.hpp): WSEE, the see-through record: the arm, the snapshot, the
// pre-draw, the re-issues and the deferral. Never include it on its own.
namespace RenderIntegration {


inline bool wsee_hold_made();   // With the hold, below.
inline void wsee_hold_drop();

// Defined in rendering/reorder_trigger.hpp (declared here for the uses below).
inline uint32_t reorder_min_opaques();
inline bool reorder_world_vp(const D3D11_VIEWPORT& vp);


// The chain's arm: WSEE_ON, and the cycle the chain draws (the published one, as vmsee_valid - the late chain
// may run on the game thread) is the one whose capture landed, untainted, its targets standing and made against the
// depth the chain reads (its texture and sample count).
inline bool wsee_valid() {
    if constexpr (!WSEE_ON) {   // Built out: no code left unreachable (C4702).
        return false;
    } else {
        const uint64_t c = g_topo_cycles_pub.load(std::memory_order_relaxed);
        return g_wsee.ok_cycle.load(std::memory_order_relaxed) == c &&
               g_wsee.taint_cycle.load(std::memory_order_relaxed) != c &&
               g_res.wsee_s_srv != nullptr && g_res.wsee_t_srv != nullptr && g_res.wsee_b_srv != nullptr &&
               g_res.wsee_n_srv != nullptr && g_res.wsee_n2_srv != nullptr && g_res.wsee_depth_id != nullptr &&
               g_res.wsee_depth_id == g_res.depth_res_identity && g_res.wsee_samples == g_res.depth_sample_count;
    }
}
// The cycle's verdict at the main depth's clear (render thread, after the cycle counter steps): WSEE_ON, a want at
// most WSEE_WANT_SPAN cycles old (one stamped later in the cycle - the scene flush's, the late chain's - counts from
// the next clear), no back-off, no WSEE_FLUSHW_PRED stand-down. The ended cycle's attempt is final here (its flush
// and any park ran inside it): landed untainted ends a run of waste, tainted or refused extends it, and
// WSEE_WASTE_RUN of them begin a back-off (the WSEE_FLUSHW_PRED hold where our flush's write ended the run).
// WSEE_IDLE_FREE cycles without a want release the targets: no chain binds them past this clear (a bind needs this
// cycle's capture, wsee_valid). Not called with the system built out.
inline void wsee_cycle_note() {
    const uint64_t c = g_topo_cycles;
    // WSEE_FLUSHW_PRED: whether the ended cycle's flush wrote the main depth after its capture (the stamp's note).
    const bool fwp = c >= 1u && g_wsee_flushw_cycle.load(std::memory_order_relaxed) == c - 1u;
    if (g_wsee.snap_cycle == c - 1u) {
        const bool used = g_wsee.ok_cycle.load(std::memory_order_relaxed) == c - 1u &&
                          g_wsee.taint_cycle.load(std::memory_order_relaxed) != c - 1u;
        // WSEE_FLAP: the verdict against the last attempted cycle's.
        if (c < g_wsee_flap_win_start || c - g_wsee_flap_win_start >= WSEE_FLAP_WIN) {
            g_wsee_flap_win_start = c;
            g_wsee_flap_n = 0;
        }
        if (g_wsee_flap_prev >= 0 && (g_wsee_flap_prev != 0) != used) ++g_wsee_flap_n;
        g_wsee_flap_prev = used ? 1 : 0;
        if (g_wsee_flap_n >= WSEE_FLAP_N) {
            g_wsee_flap_n = 0;
            g_wsee_flap_prev = -1;
            g_wsee_waste_n = 0;
            g_wsee_backoff_end = c + WSEE_BACKOFF;
        }
        if (used) {
            g_wsee_waste_n = 0;
            g_wsee_flushw_hold = false;   // WSEE_FLUSHW_PRED.
        } else if (++g_wsee_waste_n >= WSEE_WASTE_RUN) {
            g_wsee_waste_n = 0;
            if (fwp) {   // WSEE_FLUSHW_PRED: the run's cause is our flush's write - the hold, not the back-off.
                g_wsee_flushw_hold = true;
                g_wsee_flushw_probe = c - 1u;
            } else {
                g_wsee_flushw_hold = false;
                g_wsee_backoff_end = c + WSEE_BACKOFF;
            }
        } else if (g_wsee_flushw_hold && fwp) {
            g_wsee_flushw_probe = c - 1u;   // A probe (or a capture the hold let through) the write spoiled.
        }
    }
    const bool on = WSEE_ON;
    const uint64_t w = g_wsee.want_cycle.load(std::memory_order_relaxed);
    const bool want = w <= c && c - w <= WSEE_WANT_SPAN;
    g_wsee.on = on && want && c >= g_wsee_backoff_end;
    // WSEE_FLUSHW_PRED: under the hold, a cycle after a stamped one stands down but for the probe.
    if (g_wsee_flushw_hold && g_wsee.on && fwp && c - g_wsee_flushw_probe < WSEE_FLUSHW_PROBE) {
        g_wsee.on = false;
        g_wsee_flushw_off.store(c, std::memory_order_relaxed);
    }
    // The pinned depth view (g_res.wsee_dsv_pin's note) goes with a cycle that captures nothing or an identity no
    // longer the adopted depth's: its one reference keeps no stale depth texture past this clear.
    if (g_res.wsee_dsv_pin && (!g_wsee.on || g_res.wsee_depth_id != g_res.depth_res_identity)) {
        g_res.release_wsee_dsv_pin();
    }
    // The layer-depth split, one verdict a cycle: on unless a make was refused (wsee_ld_failed), whose targets go at
    // this clear.
    g_wsee.cyc_layer = on && !g_res.wsee_ld_failed;
    if (!g_wsee.cyc_layer && g_res.wsee_tp_tex) g_res.release_wsee_ld();
    // The geometry hold goes after WSEE_IDLE_FREE cycles without a copy into it (WSEE_HOLD_IDLE_COPY: the last
    // cycle a copy entry used the buffer or its run caches - an own entry uses neither; the targets' own idle span,
    // so deferrable parts that come and go do not remake it). The ended cycle's held copies were its flush's, or are
    // dropped with its record below.
    if (wsee_hold_made() &&
        (g_wsee.hold_copy_cycle > c || c - g_wsee.hold_copy_cycle > WSEE_IDLE_FREE)) {
        wsee_hold_drop();
    }
    // A loss: deferred copies no flush took, or a flush that appended them and bailed before its draw loop; either
    // stands the deferral down for WSEE_DEFER_BACKOFF cycles (a persistent cause - a scene resolve the flush
    // cannot use - would otherwise lose our parts every frame).
    bool lost = false;
    if (g_wsee.defer_cycle != ~0ull) {
        if (g_wsee.defer_cycle != c) lost = true;
        g_wsee.defer_cycle = ~0ull;
        g_wsee.defer_objs.clear();
    }
    if (g_wsee.defer_taken != ~0ull && g_wsee.defer_taken != g_wsee.defer_drawn) lost = true;
    // WSEE_DEFER_TAINT: a deferred cycle that ended tainted is a loss too - its copies drew at the flush under the
    // hardware test (wsee_sort_mode 0), after the engine's blended depth-writers, where the injection would have
    // drawn them before those; a persistent cause (a flush solid writing depth every frame) stands the deferral
    // down for WSEE_DEFER_BACKOFF cycles rather than every probing cycle of the capture's back-off.
    if (g_wsee.defer_taken == c - 1u &&
        g_wsee.taint_cycle.load(std::memory_order_relaxed) == c - 1u) lost = true;
    g_wsee.defer_taken = ~0ull;
    g_wsee.defer_drawn = ~0ull;
    if (lost) g_wsee.defer_back = c + WSEE_DEFER_BACKOFF;
    if ((g_res.wsee_s_tex || g_res.wsee_e_tex || g_res.wsee_c_srv) &&
        (!on || w > c || c - w > WSEE_IDLE_FREE)) {
        g_res.release_wsee_targets();
    }
}

// The scene flush's FX_LATE take (render thread, beside vmsee_arm): a pass the late chain will run that would
// take a mode (vmsee_arm's skips; wsee_mode_of) wants the capture, as the chain's own run does. WSEE_BWANT:
// and the colours B / B2 where such a pass is a colour reader (wsee_col_reader).
inline void wsee_arm(ID3D11Device* dev, const std::vector<std::pair<uint64_t, RenderObject>>& fs) {
    if constexpr (WSEE_ON) {   // The body whole: no code left unreachable with it out.
        for (const auto& f : fs) {
            const RenderObject& o = f.second;
            if (o.effect <= 0 || o.color[3] <= 0.001f || wsee_mode_of(o) == 0) continue;
            if (o.effect == static_cast<int>(EffectId::DynLightFog) &&
                (o.blend_mode == 0 || o.blend_mode == 4 || o.blend_mode == 5) &&
                (!(o.fx[0] > 0.0f) || !dlf_pool_live())) continue;
            if (o.effect == static_cast<int>(EffectId::Ssgi) && (!g_res.ps_ssgi_gather || !g_res.ps_ssgi_atrous)) {
                continue;
            }
            if (!wsee_ensure(dev)) return;
            g_wsee.want_cycle.store(g_topo_cycles, std::memory_order_relaxed);
            for (const auto& g : fs) {   // WSEE_BWANT.
                const RenderObject& r = g.second;
                if (r.effect > 0 && r.color[3] > 0.001f && wsee_col_reader(r) &&
                    wsee_mode_of(r) != 0) {
                    g_wsee.col_want_cycle.store(g_topo_cycles, std::memory_order_relaxed);
                    break;
                }
            }
            return;
        }
    }
}

// The far split's distance D: KH_WSEE_FAR_FRAC of the far plane of the engine's latched projection pair (m22, m32:
// ndc = m22 + m32 / d, far f = m32 / (1 - m22)), so D's ndc is m22 + (1 - m22) / KH_WSEE_FAR_FRAC - the pair alone.
// TWIN: cb.hlsl KhWseeFarRaw computes the same from depthParams.x. Its raw in the world range (lo, hi). False: no
// latched pair, or a value not inside the range (no Tf replay this cycle).
static constexpr float KH_WSEE_FAR_FRAC = 0.25f;
inline bool wsee_far_raw(float& raw, float& lo, float& hi) {
    if (!g_ro.engine_proj_valid) return false;
    world_depth_range(lo, hi);   // The world range the chain and the sort decode with, read now.
    const float m22 = g_ro.engine_m22;
    const float ndc = m22 + (1.0f - m22) / KH_WSEE_FAR_FRAC;
    raw = lo + ndc * (hi - lo);
    return std::isfinite(raw) && ndc > 0.0f && ndc < 1.0f;
}

// PSB_ZB - the guard over Zb's premise: the one pass's loop bound is exact while the main depth's texture (the one
// depth_srv views) holds at every pixel a sample 0 no farther than S. Each path that could leave it farther after the
// capture trips Zb for the rest of the cycle (psb_zb_trip; the loop is unbounded from there): a depth clear of a
// view of that texture that starts no new cycle (hooked_clear_depthstencil, after its cycle step); a draw on the
// engine's context while the bound depth state writes with GREATER / GREATER_EQUAL / NOT_EQUAL / ALWAYS and the bound
// depth view writes that texture (not READ_ONLY_DEPTH) - every draw entry (the four core hooks after
// reorder_pre_draw, DrawAuto, both indirect draws) looks at the live state while g_psb.zb_far says the bound state
// may; a CopyResource / CopySubresourceRegion(1) / ResolveSubresource / UpdateSubresource(1) into it; any
// ExecuteCommandList; depth_srv remade (depth_srv_gen). A LESS / LESS_EQUAL / EQUAL / NEVER write or a stencil-only
// write cannot (the depth plane is untouched). Zb is not built at a capture (as a trip) unless every one of those
// entries is hooked (g_rp_rec.cbreuse_on: CopySubresourceRegion, ExecuteCommandList and the context-1 writers). Assumed
// absent, as the record's other rules assume: SwapDeviceContextState bringing in such a state, Discard* of the main
// depth followed by draws without a clear, a tiled main depth. Every check runs only while Zb stands; render thread,
// the engine's context alone.
inline bool psb_zb_armed() {
    return g_psb.zb_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
}
inline void psb_zb_trip() {
    g_psb.zb_cycle.store(~0ull, std::memory_order_relaxed);
}
// A depth state that can write the depth farther (null: the default, LESS).
inline bool psb_zb_far_of(ID3D11DepthStencilState* s) {
    if (!s) return false;
    D3D11_DEPTH_STENCIL_DESC d = {};
    s->GetDesc(&d);
    return d.DepthEnable && d.DepthWriteMask != D3D11_DEPTH_WRITE_MASK_ZERO &&
           d.DepthFunc != D3D11_COMPARISON_LESS && d.DepthFunc != D3D11_COMPARISON_LESS_EQUAL &&
           d.DepthFunc != D3D11_COMPARISON_EQUAL && d.DepthFunc != D3D11_COMPARISON_NEVER;
}
// The resource is the main depth's texture (a texture first: GetType, no reference taken - the update hooks see
// every constant buffer).
inline bool psb_zb_is_depth(ID3D11Resource* r) {
    if (!r || !g_res.depth_res_identity) return false;
    if (static_cast<void*>(r) == g_res.depth_res_identity) return true;
    D3D11_RESOURCE_DIMENSION k = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    r->GetType(&k);
    if (k != D3D11_RESOURCE_DIMENSION_TEXTURE2D) return false;
    ID3D11Texture2D* t = nullptr;
    if (FAILED(r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&t))) || !t) {
        return false;
    }
    const bool m = static_cast<void*>(t) == g_res.depth_res_identity;
    t->Release();
    return m;
}
// The draw entries: the live depth state and view where the last bound state may write farther.
inline void psb_zb_draw(ID3D11DeviceContext* ctx) {
    // The context first (deferred-context threads never touch the render thread's globals).
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed) || !g_psb.zb_far || !psb_zb_armed()) return;
    ID3D11DepthStencilState* s = nullptr;
    UINT ref = 0u;
    ctx->OMGetDepthStencilState(&s, &ref);
    g_psb.zb_far = psb_zb_far_of(s);
    KH_SAFE_RELEASE(s);
    if (!g_psb.zb_far) return;
    ID3D11DepthStencilView* v = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &v);
    bool w = false;
    if (v && g_res.depth_res_identity) {
        D3D11_DEPTH_STENCIL_VIEW_DESC d = {};
        v->GetDesc(&d);
        w = (d.Flags & D3D11_DSV_READ_ONLY_DEPTH) == 0u &&
            reorder_dsv_identity(v) == g_res.depth_res_identity;
    }
    KH_SAFE_RELEASE(v);
    if (w) psb_zb_trip();
}
// OMSetDepthStencilState (any caller on the engine's context): while Zb stands, whether the state set can write
// farther; otherwise unknown (true - the first draw after the next capture looks at the live state).
// PSB_ZB_OWN: the re-issues' own fixed states take far_of's verdict without its GetDesc - none can write farther
// (dss_test LESS_EQUAL, dss_wsee_eq EQUAL and dss_wsee_far GREATER without a write, dss_wsee_nw LESS, dss_off depth
// off: their descs as made); held by g_res for the device's life, so no other object has their address.
inline void psb_zb_dss(ID3D11DeviceContext* ctx, ID3D11DepthStencilState* s) {
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed)) return;
    // PSB_ZB_DSS_ARMED: the five compares only while Zb stands; unarmed, the verdict is true (one store either way).
    if (!psb_zb_armed()) {
        g_psb.zb_far = true;
        return;
    }
    const bool own = s && (s == g_res.dss_test || s == g_res.dss_off ||
                           s == g_res.dss_wsee_nw || s == g_res.dss_wsee_eq ||
                           s == g_res.dss_wsee_far);   // PSB_ZB_OWN.
    g_psb.zb_far = !own && psb_zb_far_of(s);
}
// A clear (after the hook's cycle step), a copy / resolve / update destination, a command list.
inline void psb_zb_clear(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* v, UINT f) {
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed) || !v ||
        (f & D3D11_CLEAR_DEPTH) == 0u || !psb_zb_armed()) return;
    if (g_res.depth_res_identity && reorder_dsv_identity(v) == g_res.depth_res_identity) {
        psb_zb_trip();
    }
}
inline void psb_zb_dst(ID3D11DeviceContext* ctx, ID3D11Resource* r) {
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed) || !r || !psb_zb_armed()) return;
    if (psb_zb_is_depth(r)) psb_zb_trip();
}
inline void psb_zb_list(ID3D11DeviceContext* ctx) {
    if (ctx == g_reorder_target_ctx.load(std::memory_order_relaxed) && psb_zb_armed()) {
        psb_zb_trip();
    }
}

// The capture (vmsee_snapshot's pattern): sample 0 of the main depth into S (PSDepthS0), the scene target
// (rtv, the engine's RT0) resolved into B through the ORIGINAL resolve (the hook's render-thread work stays
// aside), T cleared (1, 0, 1, 1), NW / NW2 / Tp / Tf / P / N to 1, Tf's depth to D's raw, Zb drawn - everything
// under StateBackup (the engine's predicate set aside and handed back) and in_injection (the OM hooks do not take our
// binds for the engine's). Requires the bound depth = the texture depth_srv views and RT0 = the scene at its size and
// sample count; the targets are made or remade here (wsee_targets). False = not this cycle. WSEE_BWANT: B is
// resolved only with a colour want within WSEE_WANT_SPAN cycles (g_wsee.col_want_cycle; one stamped earlier this
// cycle counts); else B is left as it was, g_wsee.col_cycle not stamped and no reader binds it this cycle.
inline bool wsee_snapshot(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv) {
    KH_PROF_TSC(PROF_WSEE_CAP);   // wseeCapture, timed whole (once a cycle; armed alone).
    if (!rtv || !g_res.ps_vmsee_s0 || !g_res.bs_vmsee_t || !g_res.dss_test || !g_res.depth_srv ||
        !g_res.ps_wsee_one || !g_res.dss_wsee_nw || !g_res.dss_wsee_eq ||   // The re-issues' (wsee_ensure).
        !g_res.depth_dsv_ro || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off ||
        !g_orig_resolvesubresource) return false;
    const uint64_t cw = g_wsee.col_want_cycle.load(std::memory_order_relaxed);   // WSEE_BWANT.
    const bool col = cw <= g_topo_cycles && g_topo_cycles - cw <= WSEE_WANT_SPAN;
    const UINT sc = g_res.depth_sample_count;
    if (sc < 2 || g_res.ps_vmsee_samples != sc) return false;   // FSAA 1x: never (wsee_ensure).
    {
        ID3D11DepthStencilView* dsv = nullptr;
        ctx->OMGetRenderTargets(0, nullptr, &dsv);
        if (!dsv) return false;
        const bool same = g_res.depth_res_identity != nullptr &&
                          reorder_dsv_identity(dsv) == g_res.depth_res_identity;
        dsv->Release();
        if (!same) return false;
    }
    D3D11_RENDER_TARGET_VIEW_DESC vd = {};
    rtv->GetDesc(&vd);
    // WSEE: multisampled only, as B2, Q and C require - a TEXTURE2D view's texture has one sample and would fail
    // the count test below (and the targets' make ahead of it would have run at its format for nothing).
    if (vd.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2DMS) return false;
    ID3D11Texture2D* src = nullptr;
    {
        ID3D11Resource* r = nullptr;
        rtv->GetResource(&r);
        if (!r) return false;
        const HRESULT hr = r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&src));
        r->Release();
        if (FAILED(hr) || !src) return false;
    }
    D3D11_TEXTURE2D_DESC sd = {};
    src->GetDesc(&sd);
    bool tg = false;
    {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        tg = wsee_targets(dev, vd.Format);   // VMSEE_LAZY.
        if (dev) dev->Release();
    }
    if (!tg || g_res.wsee_samples != sc || sd.Width != g_res.wsee_w || sd.Height != g_res.wsee_h ||
        sd.SampleDesc.Count != sc || sd.MipLevels != 1 || sd.ArraySize != 1) {
        src->Release();
        return false;
    }
    // The layer-depth split's targets (this cycle's verdict: the split's shaders not refused, wsee_ld_failed).
    bool ld = false;
    if (g_wsee.cyc_layer) {
        ID3D11Device* ldev = nullptr;
        ctx->GetDevice(&ldev);
        ld = wsee_ld_ensure(ldev) && wsee_ld_targets(ldev);
        if (ldev) ldev->Release();
    }
    float fr = 1.0f, flo = 0.0f, fhi = 1.0f;
    const bool wn_far = ld && wsee_far_raw(fr, flo, fhi);
    // PSB: the one pass's record where a variant of it exists, at s = 1 << PSB_LS.
    bool pb = false;
    if (g_psb.have) {
        ID3D11Device* pdev = nullptr;
        ctx->GetDevice(&pdev);
        pb = psb_targets(pdev);
        if (pdev) pdev->Release();
    }
    // PSB_ZB: Zb this cycle - the record made, Zb and its shader made; built only where every writer its guard
    // watches is hooked (else a trip).
    bool zb = false;
    if (pb && g_res.wsee_zb_rtv && g_res.wsee_zb_srv && g_res.wsee_s_srv && g_res.ps_psb_zb) {
        zb = g_rp_rec.cbreuse_on.load(std::memory_order_relaxed) && g_orig_draw_auto &&
             g_orig_draw_indexed_inst_indirect && g_orig_draw_inst_indirect;
        if (!zb) psb_zb_trip();
    }
    // D3 stands this cycle (its views are made with S). Nothing is copied into it: a reader takes D3 only where a late
    // opaque owns sample 0, which this cycle's store or resolve wrote first.
    const bool od = g_res.wsee_d3_tex != nullptr && g_res.wsee_d3_uav != nullptr &&
                    g_res.wsee_d3_srv != nullptr;
    if (od) {   // WSEE_OQ2: D3m with D3 (device Create* alone; refused or absent - the store path).
        ID3D11Device* mdev = nullptr;
        ctx->GetDevice(&mdev);
        wsee_d3m_targets(mdev);
        if (mdev) mdev->Release();
    }
    {
        InjectionFlagScope fl;   // Ahead of the saves: it unwinds after their restore.
        g_ro.in_injection = true;
        KH_GPU_SCOPE(ctx, GPU_WSEE);   // PROF: S's depth pass, the resolve, the clears.
        WseeGputScope gt(ctx, WSEE_GPU_CAPTURE);   // WSEE_GPUT.
        StateBackup bk;
        bk.capture(ctx);
        OmSave om;
        om.capture(ctx);
        depth_s0_pass(ctx, g_res.wsee_s_rtv, g_res.wsee_w, g_res.wsee_h);   // EA_S0_PASS: S.
        if (zb) {   // PSB_ZB: Zb from the S just written, at the record's size (one fullscreen triangle).
            ID3D11ShaderResourceView* null = nullptr;
            D3D11_VIEWPORT zv = {};
            zv.Width = static_cast<FLOAT>((g_res.wsee_w + (1u << PSB_LS) - 1u) >> PSB_LS);
            zv.Height = static_cast<FLOAT>((g_res.wsee_h + (1u << PSB_LS) - 1u) >> PSB_LS);
            zv.MaxDepth = 1.0f;
            ctx->OMSetRenderTargets(1, &g_res.wsee_zb_rtv, nullptr);   // S's target off: its view is legal at t0.
            ctx->RSSetViewports(1, &zv);
            ctx->PSSetShader(g_res.ps_psb_zb, nullptr, 0);
            ctx->PSSetShaderResources(0, 1, &g_res.wsee_s_srv);
            ctx->Draw(3, 0);
            ctx->PSSetShaderResources(0, 1, &null);
        }
        // Multisampled always (1x is refused above), so a resolve - never a copy. WSEE_BWANT: where wanted.
        if (col) g_orig_resolvesubresource(ctx, g_res.wsee_b_tex, 0, src, 0, g_res.wsee_b_fmt);
        // T: R = 1 (nothing over), G = 0 (no late opaque), B = A = 1 (no particle-type layer, before or after B2).
        const FLOAT t0[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
        ctx->ClearRenderTargetView(g_res.wsee_t_rtv, t0);
        ctx->ClearDepthStencilView(g_res.wsee_n_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);   // NW: no layer yet.
        ctx->ClearDepthStencilView(g_res.wsee_n2_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);   // NWpost likewise.
        if (ld) {   // Tp and Tf: no layer yet; Tf's depth at D (none: Tf stays 1 this cycle).
            const FLOAT one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            ctx->ClearRenderTargetView(g_res.wsee_tp_rtv, one);
            ctx->ClearRenderTargetView(g_res.wsee_tf_rtv, one);
            ctx->ClearDepthStencilView(g_res.wsee_tfd_dsv, D3D11_CLEAR_DEPTH, wn_far ? fr : 1.0f, 0);
        }
        if (pb) {   // PSB: P0, P1, N - no layer yet (T's particle channels', Tp's, Tf's and NW's clears).
            const FLOAT pb1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            ctx->ClearRenderTargetView(g_res.wsee_p0_rtv, pb1);
            ctx->ClearRenderTargetView(g_res.wsee_p1_rtv, pb1);
            ctx->ClearRenderTargetView(g_res.wsee_nb_rtv, pb1);
            ctx->ClearRenderTargetView(g_res.wsee_nb2_rtv, pb1);   // PSB_PHASE: N2.
        }
        om.restore(ctx);
        om.release();
        bk.restore(ctx);
    }
    src->Release();
    // The pinned depth view matched the identity this replaces (g_res.wsee_dsv_pin's note).
    if (g_res.wsee_depth_id != g_res.depth_res_identity) g_res.release_wsee_dsv_pin();
    g_res.wsee_depth_id = g_res.depth_res_identity;
    if (ld) {   // This cycle's split stands (wsee_post, wsee_tail, the chain's t63 / t64).
        g_wsee.ld_far_ok = wn_far;
        g_wsee.ld_far_lo = flo;
        g_wsee.ld_far_hi = fhi;
        g_wsee.ld_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (pb) {   // PSB: this cycle's one-pass record stands (psb_on), D's raw for its Tf verdict.
        g_psb.far_raw = wn_far ? fr : 1.0f;
        g_psb.cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (zb) {   // PSB_ZB: Zb stands this cycle (psb_post) until its guard trips; the depth state unknown.
        g_psb.zb_gen = g_res.depth_srv_gen;
        g_psb.zb_far = true;
        g_psb.zb_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    } else {   // PSB_ZB: none from this capture, nor an earlier one's this cycle (its targets may be remade).
        g_psb.zb_cycle.store(~0ull, std::memory_order_relaxed);
    }
    if (od) {   // D3 stands this cycle (the replays' stores, the chain's and the sort's t65).
        g_wsee.od_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (col) {   // WSEE_BWANT: B stands this cycle (and B2 at its moment: wsee_b2_take).
        g_wsee.col_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    return true;
}

// The B2 moment: at the cycle's first fence-type draw (a qualifying blended draw that writes depth) after the
// capture, before it. S2 = the main depth's sample 0 raw then (PSDepthS0 into S2, as the capture makes S: the depth
// of the surfaces B2 shows, the pre late opaques among them), B2 = the scene target rtv resolved through the
// ORIGINAL resolve, only in a cycle whose capture resolved B (g_wsee.col_cycle, WSEE_BWANT; else B2 is left as it
// was). Particle-type layers after this moment are "post" (lane bit 3 says the split stands). S2's pass needs
// VM_SEE's depth pass at the live count and the capture's depth bound; RT0 must be the scene texture in B's view
// format at the capture's size and sample count. Under StateBackup (the predicate set aside), OmSave and
// in_injection; S2's pass binds S2 alone first, so the main depth's view is off the OM before its SRV goes to t0.
// False = no B2 this cycle (every particle-type layer counts as pre); S2 may still be drawn (below).
inline bool wsee_b2_take(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv) {
    KH_PROF_TSC(PROF_WSEE_CAP);   // wseeCapture, timed whole (once a cycle; armed alone).
    if (!ctx) return false;
    const bool s2 = g_res.wsee_s2_rtv && g_res.ps_vmsee_s0 && g_res.depth_srv && g_res.vs_fullscreen &&
                    g_res.rasterizer && g_res.dss_off && g_res.ps_vmsee_samples == g_res.depth_sample_count &&
                    g_res.wsee_depth_id && g_res.wsee_depth_id == g_res.depth_res_identity;
    // WSEE_NOPRE: a cycle that skipped a pre late opaque's replay needs S2 as the derivation's reference whatever
    // B2 does - S2's pass runs for it alone where B2 is refused (the readers bind S2 with lane bit 9); an S2 that
    // cannot be taken taints the cycle (a fence-type layer follows: the live depth is no reference any more).
    const bool np = g_wsee.nopre_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
    if (np && !s2) wsee_taint(g_topo_cycles);
    // WSEE_OQ: D3 stands complete once S2 is taken in such a cycle (wsee_d3_ok) - where this cycle's D3 stands.
    const bool sync = np && s2 && g_res.wsee_d3_tex && g_res.wsee_s2_tex &&
                      g_wsee.od_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
    ID3D11Texture2D* src = nullptr;
    if (s2 && rtv && g_res.wsee_b2_tex && g_orig_resolvesubresource && g_topo_scene_tex_id) {
        D3D11_RENDER_TARGET_VIEW_DESC vd = {};
        rtv->GetDesc(&vd);
        if (vd.Format == g_res.wsee_b_fmt && vd.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS) {
            ID3D11Resource* r = nullptr;
            rtv->GetResource(&r);
            if (r) {
                if (static_cast<void*>(r) != g_topo_scene_tex_id ||
                    FAILED(r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&src)))) {
                    src = nullptr;
                }
                r->Release();
            }
        }
    }
    bool ok = false;
    if (src) {
        D3D11_TEXTURE2D_DESC sd = {};
        src->GetDesc(&sd);
        ok = sd.Width == g_res.wsee_w && sd.Height == g_res.wsee_h &&
             sd.SampleDesc.Count == g_res.wsee_samples && sd.MipLevels == 1 && sd.ArraySize == 1;
    }
    if (ok || (np && s2)) {   // sync implies the second term.
        InjectionFlagScope fl;   // Ahead of the saves: it unwinds after their restore.
        g_ro.in_injection = true;
        KH_GPU_SCOPE(ctx, GPU_WSEE_B2);   // PROF: S2's depth pass and B2's resolve.
        WseeGputScope gt(ctx, WSEE_GPU_B2);   // WSEE_GPUT.
        StateBackup bk;
        bk.capture(ctx);
        OmSave om;
        om.capture(ctx);
        depth_s0_pass(ctx, g_res.wsee_s2_rtv, g_res.wsee_w, g_res.wsee_h);   // EA_S0_PASS: S2, as S.
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        if (sync) g_wsee.d3_sync_cycle.store(g_topo_cycles, std::memory_order_relaxed);   // WSEE_OQ.
        g_wsee.s2_cycle.store(g_topo_cycles, std::memory_order_relaxed);   // WSEE_NOPRE: S2 stands (bit 9).
        if (ok && g_wsee.col_cycle.load(std::memory_order_relaxed) == g_topo_cycles) {   // WSEE_BWANT.
            g_orig_resolvesubresource(ctx, g_res.wsee_b2_tex, 0, src, 0, g_res.wsee_b_fmt);
        }
        om.restore(ctx);
        om.release();
        bk.restore(ctx);
    }
    if (src) src->Release();
    return ok;
}

// WSEE_OQ2 - D3 from D3m, once before a reader binds D3 (the chain's t65, fx_chain_run; the sort scope's,
// WseeSortScope): where a post late opaque wrote D3m in cycle c since the last resolve (g_wsee.d3m_cycle), one
// fullscreen PSWseeD3Res into D3's RTV reading the depth's MS view (t0), T (t1) and D3m (t2) - depth_resolve.hlsl's
// note: the store's value where nothing moved sample 0 since the replay, the opaque's own depth where something did;
// untouched where sample 0's last late opaque was a pre one, a store-path one or none. Nothing pending: true, nothing
// drawn. False = pending but not drawable (a view gone, the depth not the capture's): the caller leaves D3 unbound.
// Under StateBackup (the predicate set aside) and OmSave; in_injection is set on the hooks' render thread alone
// (WSEE_D3M_RT: the hooks read it in their render-thread branches alone, which the game thread's late chain and a
// park never reach); D3 off t65 and the main depth's writable view off the OM before its view goes to t0. The
// reader's thread (render thread, a park that stalls it, or the game thread's late chain); at most two runs a cycle.
inline bool wsee_d3m_resolve(ID3D11DeviceContext* ctx, uint64_t c) {
    if (g_wsee.d3m_cycle.load(std::memory_order_relaxed) != c) return true;
    if (!ctx || !g_res.ps_wsee_d3_res || !g_res.wsee_d3_rtv || !g_res.wsee_d3m_srv || !g_res.wsee_t_srv ||
        !g_res.depth_srv || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off || !g_res.wsee_depth_id ||
        g_res.wsee_depth_id != g_res.depth_res_identity || g_res.wsee_samples != g_res.depth_sample_count) {
        return false;
    }
    {
        InjectionFlagScope fl;   // Ahead of the saves: it unwinds after their restore.
        if (reorder_on_render_thread()) g_ro.in_injection = true;   // WSEE_D3M_RT: render-thread state.
        WseeGputScope gt(ctx, WSEE_GPU_D3RES);   // WSEE_GPUT.
        StateBackup bk;
        bk.capture(ctx);
        OmSave om;
        om.capture(ctx);
        ID3D11ShaderResourceView* const n[3] = { nullptr, nullptr, nullptr };
        ctx->PSSetShaderResources(65, 1, n);   // D3 off its readers' slot before it is a target.
        ctx->OMSetRenderTargets(1, &g_res.wsee_d3_rtv, nullptr);
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<FLOAT>(g_res.wsee_w);
        vp.Height = static_cast<FLOAT>(g_res.wsee_h);
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
        ctx->RSSetState(g_res.rasterizer);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_wsee_d3_res, nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        ID3D11ShaderResourceView* const v[3] = { g_res.depth_srv, g_res.wsee_t_srv, g_res.wsee_d3m_srv };
        ctx->PSSetShaderResources(0, 3, v);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
        ctx->Draw(3, 0);
        ctx->PSSetShaderResources(0, 3, n);   // Off t0 - t2 before the engine's depth view is bound again.
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        om.restore(ctx);
        om.release();
        bk.restore(ctx);
    }
    g_wsee.d3m_cycle.store(~0ull, std::memory_order_relaxed);
    return true;
}

// Whether the bound depth view dsv is the capture's depth (its texture wsee_depth_id): the view that last
// matched is kept by one reference (g_res.wsee_dsv_pin), so the same pointer with the same identity is a match
// without a call (a view's texture never changes, a held address is no other's); anything else takes
// reorder_dsv_identity's path (GetResource / QueryInterface / Release) and a match replaces the pin. The caller tests
// a null view first: with no pin and no record standing null would equal both. Render thread.
inline bool wsee_dsv_is_depth(ID3D11DepthStencilView* dsv) {
    if (dsv == g_res.wsee_dsv_pin && g_res.wsee_dsv_pin_id == g_res.wsee_depth_id) return true;
    void* const id = reorder_dsv_identity(dsv);
    if (id != g_res.wsee_depth_id) return false;
    if (id) {
        if (dsv != g_res.wsee_dsv_pin) {
            KH_SAFE_RELEASE(g_res.wsee_dsv_pin);
            dsv->AddRef();
            g_res.wsee_dsv_pin = dsv;
        }
        g_res.wsee_dsv_pin_id = id;
    }
    return true;
}

// The late opaques a cycle replays (wsee_mark_opaque): past WSEE_OPAQUE_CAP the cycle is tainted - a capture
// that landed before most of the world would otherwise pay the world's vertex work again; the back-off then bounds
// it. The cap bounds work alone (a replay is drawn at its own hook's post, nothing stored). A pre mark is not
// replayed and does not count (WSEE_NOPRE). Render thread.
static constexpr uint32_t WSEE_OPAQUE_CAP = 4096u;
static uint32_t g_wsee_opq_n = 0;
static uint64_t g_wsee_opq_cycle = ~0ull;
// The capture gate's view of the injection (wsee_pre_draw): whether the last injection found composite-eligible
// meshes (inject_composited_meshes' elig_n; stamped there, one cycle behind at most).
static bool     g_wsee_inj_elig = false;
inline void wsee_mark_opaque(uint64_t c) {
    if (g_wsee_opq_cycle != c) {
        g_wsee_opq_cycle = c;
        g_wsee_opq_n = 0;
    }
    // Its phase: "pre" before the cycle's B2 attempt (B2 and S2 then hold this surface's colour and depth), else
    // "post" (G = 1). WSEE_NOPRE: a pre one is not replayed - the shaders derive its G, behind and reset from S2
    // against S (cb.hlsl's note at khWsS2); wsee_b2_take takes S2 for it, g_wsee.nopre_cycle stamps the cycle (lane
    // bit 8; D3's verdict too, wsee_d3_ok). The record must still be this depth's (else the replay's taint). A pre
    // mark in a cycle that then takes no B2 is still derived against the live depth and reads G = 1 (bit 3 clear).
    const bool post = g_wsee.b2_try == c;
    if (!post) {
        g_wsee.pending = WSEE_RE_NONE;
        g_wsee.pending_post = false;
        if (!g_res.wsee_depth_id || g_res.wsee_depth_id != g_res.depth_res_identity ||
            g_res.wsee_samples != g_res.depth_sample_count || !g_res.wsee_t_rtv) {
            wsee_taint(c);
            return;
        }
        g_wsee.nopre_cycle.store(c, std::memory_order_relaxed);
        return;
    }
    ++g_wsee_opq_n;
    if (g_wsee_opq_n > WSEE_OPAQUE_CAP) {
        wsee_taint(c);   // Once a cycle: the taint stands the rest of the cycle's draws down.
        return;
    }
    g_wsee.pending = WSEE_RE_OPAQUE;
    g_wsee.pending_post = true;
}

// reorder_pre_draw's call, wanted cycles only (render thread; a tracked main-depth draw of the engine, outside the
// flush and the view-model slice - the slice's draws lie below the world's range; nothing once the chain has drawn
// the cycle, g_wsee.used_cycle). Depth verdicts are the DSS hook's (dss_nowrite: tested, not written; dss_write:
// DepthEnable, mask ALL; the default state writes). A blended draw qualifies when its depth state writes or tests
// without writing, RT0 blends over (ADD, DestBlend INV_SRC_ALPHA, SrcBlend ONE / SRC_ALPHA / ZERO, colour written,
// sample mask != 0, no alpha-to-coverage) and RT0 is the scene texture (SCENE_NAME's; none named = nothing). The
// cycle's first qualifying draw takes the capture once the world pass has begun (the trigger's
// tests: reorder_world_vp, and the injection landed or reorder_min_opaques passed) and no injection can still
// come (the gate below; else the capture lands at the injection's tail); one before that neither captures nor
// counts. While the capture stands untainted: a qualifying draw is marked for its re-issue (wsee_post: T, and NW
// for a no-write one); any other draw that really writes depth is marked as a late opaque (wsee_mark_opaque: pre /
// post by the B2 attempt); an unblended draw writing the scene's colour with the depth test off taints. A blended
// draw writing no depth on the main depth into the scene texture, not testing stencil, that does not qualify is
// still re-issued (the block below): an over-blend with the test off into T untested and NW (RE_NOWRITE_OFF), a
// light-adding one (DestBlend ONE, a source factor that reads no destination), tested or not, into NW alone
// (RE_NWONLY). Any other blended draw with the test off, or one that tests without writing and does not qualify, is
// neither.
// The state is read only as far as the flags leave it open; a tainted or refused cycle reads no blended draw's.
inline void wsee_pre_draw(ID3D11DeviceContext* ctx) {
    const uint64_t c = g_topo_cycles;
    if (g_wsee.used_cycle.load(std::memory_order_relaxed) == c) return;   // The chain has drawn.
    const bool landed = g_wsee.ok_cycle.load(std::memory_order_relaxed) == c;
    const bool tainted = g_wsee.taint_cycle.load(std::memory_order_relaxed) == c;
    const bool write = g_ro.dss_write;
    // The bound viewports, the whole array once (g_wsee_pd_vp's note: the post takes it); the tests below read entry
    // 0 and the count.
    g_wsee_pd_vp_ok = false;
    const UINT& n = g_wsee_pd_vpn;
    const D3D11_VIEWPORT& vp = g_wsee_pd_vp[0];
    if (!g_ro.blend_translucent) {
        if (!landed || g_ro.dss_nowrite) return;
        // WSEE_TAINT_EARLY: a tainted cycle marks no late opaque and taints no further - nothing below changes
        // anything on it, so its four state reads a draw are skipped.
        if (tainted) return;
        wsee_pd_vp_read(ctx);
        if (n < 1 || infront_vp_is_slice(vp)) return;
        if (write) {
            wsee_mark_opaque(c);
            return;
        }
        {   // The depth test off: does it write colour (RT0's mask, the sample mask)?
            ID3D11BlendState* cbs = nullptr;
            FLOAT cbf[4] = {};
            UINT cmask = 0;
            ctx->OMGetBlendState(&cbs, cbf, &cmask);
            UINT wm = D3D11_COLOR_WRITE_ENABLE_ALL;   // Null: the default state, which writes all.
            if (cbs) {
                D3D11_BLEND_DESC cbd = {};
                cbs->GetDesc(&cbd);
                cbs->Release();
                wm = cbd.RenderTarget[0].RenderTargetWriteMask;
            }
            if ((wm & 7u) == 0 || cmask == 0) return;
        }
        ID3D11RenderTargetView* crtv = nullptr;
        ctx->OMGetRenderTargets(1, &crtv, nullptr);
        if (!crtv) return;
        const bool cscene = rtv_is_scene(crtv);   // EA_SCENE_RTV.
        crtv->Release();
        if (!cscene) return;
        wsee_taint(c);
        return;
    }
    if (tainted) return;
    if (!landed && g_wsee.snap_cycle == c) return;   // Attempted and refused: not this cycle.
    // The depth test off: no write and no capture; once the capture stands, a layer still (below).
    const bool test = write || g_ro.dss_nowrite;
    if (!test && !landed) return;
    wsee_pd_vp_read(ctx);
    if (n < 1 || infront_vp_is_slice(vp)) return;
    bool over = false;
    bool add = false;   // Light-adding: dest x ONE plus a source term that reads no dest.
    {
        ID3D11BlendState* bs = nullptr;
        FLOAT bf[4] = {};
        UINT mask = 0;
        ctx->OMGetBlendState(&bs, bf, &mask);
        if (bs) {   // Null is the default state, opaque (blend_translucent says otherwise only for a bound one).
            D3D11_BLEND_DESC bd = {};
            bs->GetDesc(&bd);
            bs->Release();
            const D3D11_RENDER_TARGET_BLEND_DESC& r = bd.RenderTarget[0];
            over = r.BlendEnable && r.BlendOp == D3D11_BLEND_OP_ADD &&
                   r.DestBlend == D3D11_BLEND_INV_SRC_ALPHA &&
                   (r.SrcBlend == D3D11_BLEND_ONE || r.SrcBlend == D3D11_BLEND_SRC_ALPHA ||
                    r.SrcBlend == D3D11_BLEND_ZERO) &&
                   (r.RenderTargetWriteMask & 7u) != 0 && mask != 0 &&
                   !bd.AlphaToCoverageEnable;   // A2C writes depth at covered samples alone.
            // The source factor must not read the destination (screen's INV_DEST_COLOR scales what lies behind by
            // a factor T does not hold).
            const D3D11_BLEND sb = r.SrcBlend;
            const bool sd = sb == D3D11_BLEND_DEST_COLOR || sb == D3D11_BLEND_INV_DEST_COLOR ||
                            sb == D3D11_BLEND_DEST_ALPHA || sb == D3D11_BLEND_INV_DEST_ALPHA ||
                            sb == D3D11_BLEND_SRC_ALPHA_SAT;
            add = r.BlendEnable && r.BlendOp == D3D11_BLEND_OP_ADD && !sd &&
                  r.DestBlend == D3D11_BLEND_ONE && (r.RenderTargetWriteMask & 7u) != 0 &&
                  mask != 0 && !bd.AlphaToCoverageEnable;
        }
    }
    ID3D11RenderTargetView* rtv = nullptr;
    bool scene = false;
    if (over || add) {
        ctx->OMGetRenderTargets(1, &rtv, nullptr);
        scene = rtv != nullptr && rtv_is_scene(rtv);   // EA_SCENE_RTV.
    }
    // A blended draw writing no depth over the capture's depth, once the capture stands: an over-blend drawn with
    // its depth test off goes into T untested (it covered every fragment it drew) and NW; a light-adding one
    // (DestBlend ONE, a source factor that reads no destination), tested or not, into NW alone - its light is in the
    // scene's colour beyond T B, so its depth is a front and T stays the over-blends' product. Multiply, screen,
    // subtractive and MIN / MAX stay out: a darkening layer at the front would lose its darkening in the fog (E = S -
    // T B' is held at 0 there), and a term that reads the destination scales what lies behind by a factor T does not
    // hold.
    if (!write && scene && landed && (!test || !over)) {
        bool dz = false;
        {
            ID3D11DepthStencilView* dsv = nullptr;
            ctx->OMGetRenderTargets(0, nullptr, &dsv);
            dz = dsv != nullptr && wsee_dsv_is_depth(dsv);
            if (dsv) dsv->Release();
        }
        if (rtv) rtv->Release();
        if (!dz) return;   // Not on the main depth: no layer of the world's picture.
        if (!over && !add) return;
        if (g_ro.dss_stencil) return;   // NW cannot carry its stencil mask: not recorded.
        g_wsee.pending = over ? WSEE_RE_NOWRITE_OFF : WSEE_RE_NWONLY;
        g_wsee.pending_post = g_wsee.b2_cycle.load(std::memory_order_relaxed) == c;   // Its phase.
        return;
    }
    if (!test || !over || !scene) {
        if (rtv) rtv->Release();
        // A depth write that is no qualifying layer is an opaque surface to the chain.
        if (landed && write) wsee_mark_opaque(c);
        return;
    }
    if (!landed) {
        // The world pass has begun (the trigger's own tests), or this draw neither captures nor counts (a far
        // partition's clouds, a blended draw ahead of the opaque wall).
        const bool world = reorder_world_vp(vp) &&
                           (g_ro.injected || g_ro.opaque_draws >= reorder_min_opaques());
        // Not while this cycle's injection can still come: none landed, attempts left (reorder_trigger's cap), the
        // last injection found eligible meshes (g_wsee_inj_elig), the cycle is not anomalous (inject_composited_meshes'
        // anomalous: that injection re-arms or rescues outside the world's viewport, and its tail takes no
        // capture) and the path is alive (composite_path_healthy; the clock read only once the other terms hold). Such
        // a draw passes uncaptured and the capture lands at the injection's tail (wsee_inj_capture, before its
        // deferral), with our solids in S and B - a capture here would be tainted by the injection's depth write. A
        // cycle no injection can reach captures here.
        const bool inj_due = !g_ro.injected && g_ro.inject_attempts < REORDER_MAX_INJECT_ATTEMPTS &&
                             g_wsee_inj_elig && !(g_proj_locator_ever && g_ro.slot_near_live <= 0.0f) &&
                             composite_path_healthy();
        if (world && !inj_due) {
            g_wsee.snap_cycle = c;
            if (wsee_snapshot(ctx, rtv)) g_wsee.ok_cycle.store(c, std::memory_order_relaxed);
        }
    }
    // The cycle's first fence-type layer (a qualifying blended draw that writes depth) once the capture stands: B2,
    // the scene it is drawn over, taken before it (once a cycle; refused, the cycle has none).
    if (write && g_wsee.ok_cycle.load(std::memory_order_relaxed) == c && g_wsee.b2_try != c) {
        g_wsee.b2_try = c;
        if (wsee_b2_take(ctx, rtv)) {
            g_wsee.b2_cycle.store(c, std::memory_order_relaxed);
        }
    }
    rtv->Release();
    if (g_wsee.ok_cycle.load(std::memory_order_relaxed) != c) return;
    // A no-write layer testing stencil is not re-issued: its NW pass cannot carry the mask (a depth writer's EQUAL
    // passes select its own samples whatever its stencil).
    if (!write && g_ro.dss_stencil) return;
    g_wsee.pending = write ? WSEE_RE_BLEND : WSEE_RE_NOWRITE;
    // A particle-type layer's phase: drawn after this cycle's B2 (Tpost / NWpost) or before it (Tpre / NWpre).
    g_wsee.pending_post = g_wsee.b2_cycle.load(std::memory_order_relaxed) == c;
}

inline void wsee_sv_release() {
    for (ID3D11RenderTargetView*& r : g_wsee_sv.rtvs) KH_SAFE_RELEASE(r);
    KH_SAFE_RELEASE(g_wsee_sv.dsv);
    KH_SAFE_RELEASE(g_wsee_sv.bs);
    KH_SAFE_RELEASE(g_wsee_sv.dss);
    KH_SAFE_RELEASE(g_wsee_sv.ps);
    g_wsee_sv.has_ps = false;
    KH_SAFE_RELEASE(g_wsee_sv.srv0);
    g_wsee_sv.has_srv0 = false;
    g_wsee_sv.d3m = false;   // WSEE_OQ2.
}

// The re-issues' passes (wsee_re_set's pass; TWIN of its binds), all under in_injection:
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
static constexpr int WSEE_PASS_T = 0, WSEE_PASS_NW = 1, WSEE_PASS_ONE = 2, WSEE_PASS_T_OFF = 3,
                     WSEE_PASS_T_EQ = 4;
//   PASS_TP / TP_OFF   Tp.R|G under dss_test / dss_off (the caller pushes the viewports by KH_WSEE_NEAR_TOL),
//               bs_wsee_ldpre / ldpost (T's particle factors: a fragment Tp and T both take is multiplied alike).
//   PASS_TF     Tf over wsee_tfd_dsv (cleared to D's raw), dss_wsee_far (GREATER, no write), every sample.
static constexpr int WSEE_PASS_TP = 5, WSEE_PASS_TP_OFF = 6, WSEE_PASS_TF = 7;
//   PASS_TP2 / TP2_OFF   the Tp passes again into Tp.B|A (bs_wsee_ldpre2 / ldpost2) at KH_WSEE_NEAR_TOL2.
static constexpr int WSEE_PASS_TP2 = 8, WSEE_PASS_TP2_OFF = 9;
// WSEE_GPUT: a pass's stamp kind - the particle-type passes' own; none for the one-pass kinds (a late opaque, a
// fence-type layer: their block's interval alone); any other pass its own catch-all.
inline uint32_t wsee_gput_pk(int pass) {
    if (pass == WSEE_PASS_ONE || pass == WSEE_PASS_T_EQ) return WSEE_GPU_N;
    if (pass == WSEE_PASS_T || pass == WSEE_PASS_T_OFF) return WSEE_GPU_P_T;
    if (pass == WSEE_PASS_NW) return WSEE_GPU_P_NW;
    if (pass == WSEE_PASS_TP || pass == WSEE_PASS_TP_OFF) return WSEE_GPU_P_TP;
    if (pass == WSEE_PASS_TP2 || pass == WSEE_PASS_TP2_OFF) return WSEE_GPU_P_TP2;
    if (pass == WSEE_PASS_TF) return WSEE_GPU_P_TF;
    return WSEE_GPU_P_OTHER;
}
// This cycle's split stands (the capture cleared Tp / Tf: g_wsee.ld_cycle) with its states (render thread).
inline bool wsee_ld_on() {
    return g_wsee.cyc_layer && g_wsee.ld_cycle.load(std::memory_order_relaxed) == g_topo_cycles &&
           g_res.wsee_tp_rtv && g_res.wsee_tf_rtv && g_res.wsee_tfd_dsv && g_res.bs_wsee_ldpre &&
           g_res.bs_wsee_ldpost && g_res.dss_wsee_far && g_res.ps_wsee_one_l &&
           g_res.bs_wsee_ldpre2 && g_res.bs_wsee_ldpost2 &&
           g_res.depth_dsv_ro && g_res.dss_test && g_res.dss_off;
}
// This cycle's D3 stands (stamped by the capture: g_wsee.od_cycle) with the views the late-opaque pass binds - D3's
// UAV (u3) and the main depth's multisampled view (PS t0, read beside depth_dsv_ro). Render thread (the re-issues)
// and the flush's (the sort scope: the render thread, or a park that stalls it).
inline bool wsee_od_on() {
    return g_wsee.od_cycle.load(std::memory_order_relaxed) == g_topo_cycles &&
           g_res.wsee_d3_uav && g_res.wsee_d3_srv && g_res.depth_srv;
}
// WSEE_OQ2: whether a post late opaque's replay takes the D3m path (no UAV, no depth view): D3m and its shaders
// stand, the engine's rasterizer neither biases depth nor turns depth clip off (with either the PS's interpolated z
// need not be the depth the engine wrote - D3D11 leaves it to the hardware), and the draw rasterizes solid triangles
// (WSEE_PXZ: the twins write the depth plane at sample 0 from the centre's z and derivatives, a plane over a
// triangle alone - a line / point / patch list, a bound GS or a wireframe fill takes the store path). The state is
// read once a post replay (RSGetState), its desc only when it is not the pinned one (g_res.wsee_rs_pin: one
// reference, so a reused address cannot pass for it; states are immutable). False: the store path. Render thread
// (wsee_re_save); one IAGetPrimitiveTopology and one GSGetShader a post replay.
inline bool wsee_d3m_path(ID3D11DeviceContext* ctx) {
    if (!g_res.wsee_d3m_rtv || !g_res.wsee_d3m_srv || !g_res.wsee_d3_rtv || !g_res.ps_wsee_one_z ||
        !g_res.ps_wsee_one_zl || !g_res.ps_wsee_d3_res) return false;
    D3D11_PRIMITIVE_TOPOLOGY tp = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ctx->IAGetPrimitiveTopology(&tp);
    ID3D11GeometryShader* gs = nullptr;
    ctx->GSGetShader(&gs, nullptr, nullptr);
    const bool gsb = gs != nullptr;
    KH_SAFE_RELEASE(gs);
    if (gsb ||
        (tp != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST && tp != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP &&
         tp != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ &&
         tp != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ)) {
        return false;
    }
    ID3D11RasterizerState* rs = nullptr;
    ctx->RSGetState(&rs);
    bool plain = true;   // Null: the default state - no bias, depth clip on, solid fill.
    if (rs) {
        if (rs == g_res.wsee_rs_pin) {
            plain = g_res.wsee_rs_pin_plain;
            rs->Release();
        } else {
            D3D11_RASTERIZER_DESC d = {};
            rs->GetDesc(&d);
            plain = d.DepthBias == 0 && d.SlopeScaledDepthBias == 0.0f &&
                    d.DepthClipEnable != FALSE &&
                    d.FillMode == D3D11_FILL_SOLID;   // WSEE_PXZ: no wireframe (lines: no plane).
            KH_SAFE_RELEASE(g_res.wsee_rs_pin);
            g_res.wsee_rs_pin = rs;   // The reference RSGetState took is the pin's.
            g_res.wsee_rs_pin_plain = plain;
        }
    }
    return plain;
}

// Whether this cycle's targets and states for a re-issue of kind k stand, made against the depth bound now.
inline bool wsee_re_ready(uint8_t k) {
    if (!g_res.wsee_depth_id || g_res.wsee_depth_id != g_res.depth_res_identity ||
        g_res.wsee_samples != g_res.depth_sample_count || !g_res.wsee_t_rtv || !g_res.depth_dsv_ro) return false;
    if (k == WSEE_RE_OPAQUE) {   // A post one always (no pre replay).
        return g_res.dss_wsee_eq && g_res.ps_wsee_one;
    }
    if (k == WSEE_RE_BLEND && !g_res.dss_wsee_eq) return false;   // Its T pass is EQUAL.
    // A particle-type kind also needs NWpost and the Tpre / Tpost states.
    if (k != WSEE_RE_BLEND && (!g_res.wsee_n2_dsv || !g_res.bs_wsee_tpre || !g_res.bs_wsee_tpost)) return false;
    if (k == WSEE_RE_NWONLY) return g_res.wsee_n_dsv && g_res.dss_wsee_nw;
    if (!g_res.dss_test || !g_res.bs_vmsee_t) return false;
    if (k == WSEE_RE_NOWRITE_OFF && !g_res.dss_off) return false;
    return (k != WSEE_RE_NOWRITE && k != WSEE_RE_NOWRITE_OFF) ||
           (g_res.wsee_n_dsv && g_res.dss_wsee_nw);
}

// A re-issue's save, once a post (render thread, wsee_post) whatever its passes: the RTVs and DSV, the blend state
// with its factor and sample mask, the depth state with its reference, and with ps (a late opaque) the pixel
// shader and PS t0 (the store path binds the depth's view there). Probed once: the bound depth must be wsee_depth_id's
// texture; refused (false, nothing kept) with a UAV or stream-output target bound (the second draw would write them
// again) or the PS bound with class instances (set back without them it would not be the engine's); the caller
// taints. wsee_re_restore sets back exactly this; scissor, rasterizer, other shaders, constants, views, input and
// predicate are not touched by the passes (the viewports are shifted and set back by WseeSurfVp itself; PSB's
// one pass, psb_post, sets the viewports, scissor rects, its constant-buffer and view slots and puts them back).
inline bool wsee_re_save(ID3D11DeviceContext* ctx, bool ps) {
    ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_wsee_sv.rtvs, &g_wsee_sv.dsv);
    bool refuse = !g_wsee_sv.dsv || !wsee_dsv_is_depth(g_wsee_sv.dsv);
    if (!refuse) {
        ID3D11UnorderedAccessView* uavs[8] = {};
        ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 8, uavs);   // UAV probe only.
        for (ID3D11UnorderedAccessView*& u : uavs) {
            if (u) refuse = true;
            KH_SAFE_RELEASE(u);
        }
    }
    if (!refuse) {
        ID3D11Buffer* so = nullptr;
        ctx->SOGetTargets(1, &so);
        if (so) {
            refuse = true;
            so->Release();
        }
    }
    if (!refuse && ps) {
        ID3D11ClassInstance* ci[8] = {};
        UINT cn = 8;
        ctx->PSGetShader(&g_wsee_sv.ps, ci, &cn);
        g_wsee_sv.has_ps = true;
        if (cn > 0) refuse = true;
        for (ID3D11ClassInstance*& c : ci) {
            if (c) refuse = true;
            KH_SAFE_RELEASE(c);
        }
        // The store path binds the main depth's view at t0; the D3m path binds neither (WSEE_OQ2).
        if (!refuse && wsee_od_on()) {
            g_wsee_sv.d3m = wsee_d3m_path(ctx);
            if (!g_wsee_sv.d3m) {
                ctx->PSGetShaderResources(0, 1, &g_wsee_sv.srv0);
                g_wsee_sv.has_srv0 = true;
            }
        }
    }
    if (refuse) {
        wsee_sv_release();
        return false;
    }
    ctx->OMGetBlendState(&g_wsee_sv.bs, g_wsee_sv.bf, &g_wsee_sv.mask);
    ctx->OMGetDepthStencilState(&g_wsee_sv.dss, &g_wsee_sv.ref);
    wsee_gput_begin(ctx, WSEE_GPU_N);   // WSEE_GPUT: the re-issue's block (its kind from its first pass).
    return true;
}

// One pass's state (after wsee_re_save; a post's passes follow each other without a restore between). Under
// in_injection, so the OM hooks do not take ours for the engine's. post picks the phase's target / channel
// (NWpost, T.A; else NWpre, T.B); the fence-type pass (PASS_T_EQ) keeps T.R. pv (PSV): the pass binds the
// engine shader's variant - nw (null: no pixel shader) for NW, alpha for every other pass; off or absent, the engine's
// shader as bound (wsee_re_restore sets it back where one was set here).
inline void wsee_re_set(ID3D11DeviceContext* ctx, int pass, bool post = false,
                        const PsvPick* pv = nullptr) {
    InjectionFlagScope fl;
    g_ro.in_injection = true;
    if (g_wsee_gput.blk >= 0) {   // WSEE_GPUT: the block's kind from its first pass; a stamp before a pass's own.
        wsee_gput_pass(ctx, pass == WSEE_PASS_ONE ? WSEE_GPU_OPQ_POST
                            : pass == WSEE_PASS_T_EQ ? WSEE_GPU_FENCE : WSEE_GPU_PART, wsee_gput_pk(pass));
    }
    if (pv && pv->on) {
        ctx->PSSetShader(pass == WSEE_PASS_NW ? pv->nw : pv->alpha, nullptr, 0);
    }
    const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (pass == WSEE_PASS_NW) {
        ctx->OMSetRenderTargets(0, nullptr, post ? g_res.wsee_n2_dsv : g_res.wsee_n_dsv);
        ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_wsee_nw, 0);
        return;
    }
    // The split's passes (wsee_ld_on stands: the caller's check).
    ID3D11BlendState* const lb = post ? g_res.bs_wsee_ldpost : g_res.bs_wsee_ldpre;
    if (pass == WSEE_PASS_TP || pass == WSEE_PASS_TP_OFF) {
        ctx->OMSetRenderTargets(1, &g_res.wsee_tp_rtv, g_res.depth_dsv_ro);
        ctx->OMSetBlendState(lb, bf, g_wsee_sv.mask);
        ctx->OMSetDepthStencilState(pass == WSEE_PASS_TP ? g_res.dss_test : g_res.dss_off, 0);
        return;
    }
    // The 100 % band's passes: Tp's B / A (the caller pushes by KH_WSEE_NEAR_TOL2).
    if (pass == WSEE_PASS_TP2 || pass == WSEE_PASS_TP2_OFF) {
        ctx->OMSetRenderTargets(1, &g_res.wsee_tp_rtv, g_res.depth_dsv_ro);
        ctx->OMSetBlendState(post ? g_res.bs_wsee_ldpost2 : g_res.bs_wsee_ldpre2, bf, g_wsee_sv.mask);
        ctx->OMSetDepthStencilState(pass == WSEE_PASS_TP2 ? g_res.dss_test : g_res.dss_off, 0);
        return;
    }
    if (pass == WSEE_PASS_TF) {
        ctx->OMSetRenderTargets(1, &g_res.wsee_tf_rtv, g_res.wsee_tfd_dsv);
        ctx->OMSetBlendState(lb, bf, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_wsee_far, 0);
        return;
    }
    // A late opaque's replay resets Tp with T (MRT; all four channels, no blend state) while the split stands, and
    // (PSB) only once this cycle's old record holds a Tp layer (PSB_TPA: g_psb.psa_tp_cycle, the first Tp pass of
    // wsee_post or wsee_tail; an NW-only layer writes none) - before that Tp holds its clear, which the reset
    // would write again. The one pass's record has no reset (the shaders' read-time rule). A cycle without the one
    // pass's record resets from its start.
    const bool l = pass == WSEE_PASS_ONE && wsee_ld_on() &&
                   (g_psb.cycle.load(std::memory_order_relaxed) != g_topo_cycles ||
                    g_psb.psa_tp_cycle.load(std::memory_order_relaxed) == g_topo_cycles);
    // WSEE_OQ2: a post opaque's third target D3m (null where not made; Tp's slot null without the split).
    ID3D11RenderTargetView* const rt[3] = { g_res.wsee_t_rtv, l ? g_res.wsee_tp_rtv : nullptr,
                                            g_res.wsee_d3m_rtv };
    // D3: the store path binds the UAV at u3 and the depth's MS view at t0 (after the read-only DSV is in place);
    // the D3m path (g_wsee_sv.d3m, wsee_d3m_path at the save) binds neither - its twin writes z into D3m, which a
    // reader resolves into D3 (wsee_d3m_resolve); the store twins write -1 there, so the resolve keeps the store.
    const bool od = pass == WSEE_PASS_ONE && wsee_od_on();
    const bool z = od && g_wsee_sv.d3m;
    if (od && !z) {
        ctx->OMSetRenderTargetsAndUnorderedAccessViews(3u, rt, g_res.depth_dsv_ro, 3u, 1u, &g_res.wsee_d3_uav,
                                                       nullptr);
        ctx->PSSetShaderResources(0, 1, &g_res.depth_srv);
    } else {
        ctx->OMSetRenderTargets(od ? 3u : l ? 2u : 1u, rt, g_res.depth_dsv_ro);
    }
    if (z) g_wsee.d3m_cycle.store(g_topo_cycles, std::memory_order_relaxed);   // WSEE_OQ2: a resolve owed.
    ID3D11BlendState* bs = nullptr;   // WSEE_PASS_ONE: none (it writes (1, 1, 1, 1)).
    if (pass == WSEE_PASS_T_EQ) bs = g_res.bs_vmsee_t;
    else if (pass == WSEE_PASS_T || pass == WSEE_PASS_T_OFF)
        bs = post ? g_res.bs_wsee_tpost : g_res.bs_wsee_tpre;
    ctx->OMSetBlendState(bs, bf, g_wsee_sv.mask);
    ctx->OMSetDepthStencilState(pass == WSEE_PASS_T ? g_res.dss_test
                                : pass == WSEE_PASS_T_OFF ? g_res.dss_off : g_res.dss_wsee_eq, 0);
    if (pass == WSEE_PASS_ONE) {   // G = 1; the D3m path's twins are PSWseeOneZ / PSWseeOneZL.
        if (l) {
            ctx->PSSetShader(z ? g_res.ps_wsee_one_zl : g_res.ps_wsee_one_l, nullptr, 0);
        } else {
            ctx->PSSetShader(z ? g_res.ps_wsee_one_z : g_res.ps_wsee_one, nullptr, 0);
        }
    }
}

inline void wsee_re_restore(ID3D11DeviceContext* ctx) {
    {
        InjectionFlagScope fl;   // The engine's own set back, which its trackers already saw.
        g_ro.in_injection = true;
        // The engine's t0 first: the main depth's view leaves the slot before its writable depth view is bound again
        // below (the runtime would otherwise null the slot itself); OMSetRenderTargets unbinds D3's UAV.
        if (g_wsee_sv.has_srv0) ctx->PSSetShaderResources(0, 1, &g_wsee_sv.srv0);
        ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_wsee_sv.rtvs, g_wsee_sv.dsv);
        ctx->OMSetBlendState(g_wsee_sv.bs, g_wsee_sv.bf, g_wsee_sv.mask);
        ctx->OMSetDepthStencilState(g_wsee_sv.dss, g_wsee_sv.ref);
        if (g_wsee_sv.has_ps) ctx->PSSetShader(g_wsee_sv.ps, nullptr, 0);
    }
    wsee_gput_end(ctx);   // WSEE_GPUT: the re-issue's block, its restore included.
    wsee_sv_release();
}

// WseeSurfVp - a blended no-write draw lying on a surface (a decal, a set-in window, smoke's foot) is the surface's,
// not a layer: the particle-type T / NW / Tp / Tp2 passes and our tail's are drawn with every bound viewport's MinDepth
// raised, Min' = Min + TOL (Max - Min), i.e. z' = z + TOL (Max - z) in the viewport's range - a fragment at view
// distance d is pushed back by at most TOL d / (1 - TOL) (2.04 % at 0.02), less toward the far plane. Against
// LESS_EQUAL a fragment within that of the surface behind it fails (T untouched) and its NW lands behind the surface:
// the pixel is the surface's. A layer meant to be separate but within the band (smoke hugging the ground) is fogged
// with the surface. Not shifted: the fence-type and late-opaque passes (EQUAL selects the samples the draw wrote) and
// a test-off T pass (its NW pass is). Guards (wsee_surf_ok, begin): the engine's depth test is not GREATER /
// GREATER_EQUAL, every bound viewport has a finite depth range and MaxDepth = the world range's Hi (wsee_surf_max,
// within 1e-6: the inverse below is exact only then; a draw in another range is unshifted and its NW position reads
// at most 2.04 % near). NW holds the pushed raw n' = (1 - TOL) n + TOL Hi; it decides (n' against r, the evidence
// rule) and the shaders take the position back, n = (n' - TOL Hi) / (1 - TOL) (cb.hlsl KhWseeNwPos / KhWseeMsNwPos,
// Hi = depthParams.w / khFxFog[0].w). An engine PS reading SV_Position.z (soft particles) sees the pushed depth in
// these passes only: a fade over F metres loses up to TOL d / F near a surface. The viewports go back exactly
// (RSSetViewports is not hooked). TWIN: cb.hlsl KH_WSEE_SURF_TOL 0.02.
static constexpr float KH_WSEE_SURF_TOL = 0.02f;
// The world range's MaxDepth the chain and the sort decode with (fill_depth_range_cb's rule, read at the draw).
inline float wsee_surf_max() {
    float lo = 0.0f, hi = 1.0f;
    world_depth_range(lo, hi);
    return hi;
}
struct WseeSurfVp {
    D3D11_VIEWPORT vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT           n = 0;
    bool           on = false;
    // The array shifted and set back: vp, read by begin, or the draw's kept array (keep: wsee_post's, read once by
    // its pre-draw - g_wsee_pd_vp's note); with one kept, begin reads nothing.
    const D3D11_VIEWPORT* src = vp;
    bool           kept = false;
    void keep(const D3D11_VIEWPORT* sv_vp, UINT sv_n) {
        src = sv_vp;
        n = sv_n;
        kept = true;
    }
    // The shift (the bound viewports read, kept, and set again raised); false = nothing changed (no viewport, one
    // without a depth range, or one whose MaxDepth is not the world range's, wsee_surf_max). tol: the push
    // (KH_WSEE_SURF_TOL; KH_WSEE_NEAR_TOL / NEAR_TOL2 for the Tp passes). A shift standing is kept (end first to
    // change the push).
    bool begin(ID3D11DeviceContext* ctx, float tol = KH_WSEE_SURF_TOL) {
        if (on) return true;
        if (!kept) {
            n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            ctx->RSGetViewports(&n, vp);
        }
        if (n == 0 || n > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) return false;
        const float wh = wsee_surf_max();
        D3D11_VIEWPORT s[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        for (UINT i = 0; i < n; ++i) {
            const float lo = src[i].MinDepth, hi = src[i].MaxDepth;
            if (!(hi > lo) || !std::isfinite(lo) || !std::isfinite(hi)) return false;
            if (!(fabsf(hi - wh) <= 1.0e-6f)) return false;   // The shaders' inverse needs Hi.
            s[i] = src[i];
            s[i].MinDepth = lo + tol * (hi - lo);
        }
        ctx->RSSetViewports(n, s);
        on = true;
        return true;
    }
    void end(ID3D11DeviceContext* ctx) {
        if (!on) return;
        ctx->RSSetViewports(n, src);
        on = false;
    }
};
// The near bands: WseeSurfVp's shift at 0.2 pushes a fragment at view distance d to d / (1 - 0.2 (1 - d / f))
// (1.25 d near the camera); at 0.5 to 2 d. A particle-type fragment that passes the 2 % test but not the 25 % one is
// in T and not in Tp.R|G (the chain charges it at the band's middle, cb.hlsl KhWseeNearRaw); one not passing the
// 100 % test is not in Tp.B|A (charged at KhWseeNearRaw2). TWINS: cb.hlsl KH_WSEE_NEAR_TOL 0.2, KH_WSEE_NEAR_TOL2 0.5.
static constexpr float KH_WSEE_NEAR_TOL = 0.2f;
static constexpr float KH_WSEE_NEAR_TOL2 = 0.5f;
// Every bound viewport is the world range the far raw D was computed in (g_wsee.ld_far_lo / hi, within 1e-6): the Tf
// pass compares raws of that range alone. k / kn: the draw's kept array (wsee_post's, g_wsee_pd_vp's
// note) in place of the read; null, the bound array is read (the tail's).
inline bool wsee_vp_world(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT* k = nullptr, UINT kn = 0) {
    D3D11_VIEWPORT rd[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT n = kn;
    const D3D11_VIEWPORT* vp = k;
    if (!k) {
        n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetViewports(&n, rd);
        vp = rd;
    }
    if (n == 0 || n > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) return false;
    for (UINT i = 0; i < n; ++i) {
        if (!(fabsf(vp[i].MinDepth - g_wsee.ld_far_lo) <= 1.0e-6f &&
              fabsf(vp[i].MaxDepth - g_wsee.ld_far_hi) <= 1.0e-6f)) return false;
    }
    return true;
}
// Every bound viewport lies at or beyond the world range's far side (MinDepth >= Hi within 1e-6, MaxDepth >=
// MinDepth; NaN fails): the engine's cloud deck is drawn in a [1, 1] range, so every fragment has z >= Hi > D's raw
// and the Tf pass's GREATER passes wherever it rasterizes - the layer is beyond D by construction. Accepted beside
// wsee_vp_world at wsee_post's Tf gate. A range partly before Hi (a far partition's [0.5, 1]) is not: a fragment
// of it may land before D.
inline bool wsee_vp_beyond(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT* k = nullptr, UINT kn = 0) {
    D3D11_VIEWPORT rd[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT n = kn;
    const D3D11_VIEWPORT* vp = k;
    if (!k) {
        n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetViewports(&n, rd);
        vp = rd;
    }
    if (n == 0 || n > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) return false;
    for (UINT i = 0; i < n; ++i) {
        if (!(vp[i].MinDepth >= g_wsee.ld_far_hi - 1.0e-6f &&
              vp[i].MaxDepth >= vp[i].MinDepth)) return false;
    }
    return true;
}
// The engine's depth state allows the shift (wsee_post, after wsee_re_save: the saved state): refused only on
// evidence of reversed depth - a GREATER / GREATER_EQUAL test; every other function or no test is no evidence either
// way, and the replays' own tests are ours (dss_test LESS_EQUAL, dss_wsee_nw LESS). Null is the default state (LESS).
inline bool wsee_surf_ok(ID3D11DepthStencilState* dss) {
    if (!dss) return true;
    D3D11_DEPTH_STENCIL_DESC d = {};
    dss->GetDesc(&d);
    return !d.DepthEnable || (d.DepthFunc != D3D11_COMPARISON_GREATER &&
                              d.DepthFunc != D3D11_COMPARISON_GREATER_EQUAL);
}

// PSB - this cycle's one-pass record stands: the capture cleared P0 / P1 / N / N2 (g_psb.cycle), the states and the
// main depth's multisampled view are there (render thread).
inline bool psb_on() {
    return g_psb.cycle.load(std::memory_order_relaxed) == g_topo_cycles && g_res.wsee_p0_rtv && g_res.wsee_p1_rtv &&
           g_res.wsee_nb_rtv && g_res.wsee_nb2_rtv && g_res.bs_psb[1] && g_res.psb_cb[7] && g_res.depth_srv;
}
// PSB: the bound rasterizer lets the variant's in-shader z stand for the hardware's - no depth bias (SV_Position.z
// does not carry it) and depth clip on (null = the default state); sc = whether it scissors. PSB_RSPIN: the
// state last asked is pinned with its verdicts (g_res.wsee_rs_pin_psb: one reference, so a reused address cannot pass
// for it; states are immutable) - GetDesc only for another state; released with that pin.
inline bool psb_rs_ok(ID3D11DeviceContext* ctx, bool* sc = nullptr) {
    if (sc) *sc = false;
    ID3D11RasterizerState* rs = nullptr;
    ctx->RSGetState(&rs);
    if (!rs) return true;
    if (rs == g_res.wsee_rs_pin_psb) {   // PSB_RSPIN.
        rs->Release();
    } else {
        D3D11_RASTERIZER_DESC d = {};
        rs->GetDesc(&d);
        KH_SAFE_RELEASE(g_res.wsee_rs_pin_psb);
        g_res.wsee_rs_pin_psb = rs;   // The reference RSGetState took is the pin's.
        g_res.wsee_rs_pin_psb_sc = d.ScissorEnable != FALSE;   // PSB_LORES.
        g_res.wsee_rs_pin_psb_ok = d.DepthBias == 0 && d.SlopeScaledDepthBias == 0.0f &&
                                   d.DepthClipEnable != FALSE;
    }
    if (sc) *sc = g_res.wsee_rs_pin_psb_sc;
    return g_res.wsee_rs_pin_psb_ok;
}
// PSB_LORES: a scissor edge e of the frame at 1 / s (s = 1 << ls): the first pixel of the scaled target
// whose centre, taken to the frame ((X + 1/2) s), lies at or past e - floor((e + s / 2 - 1) / s), so the scaled
// rect keeps exactly the block centres the frame's rect holds (the rasterizer's own pixel-centre rule).
inline LONG psb_sdiv(LONG e, UINT ls) {
    const long long s = 1ll << ls;
    const long long q = static_cast<long long>(e) + (s >> 1) - 1;
    return static_cast<LONG>(q >= 0 ? q / s : -((-q + s - 1) / s));
}
// PSB_TF_EPS - the one pass writes a fragment's Tf channels only where the block's largest live sample 0 r >= Dq,
// the float just above D - PSB_TF_EPS (D's raw as the capture took it, g_psb.far_raw; 0 where Tf is not drawn).
// Exact at sample 0: Tf is read by KhWseeGroupsSet8's far branch alone, only at a sample whose live depth r' at the
// flush exceeds D, and the main depth only comes nearer within a cycle (the engine tests LESS / LESS_EQUAL), so at a
// sample 0 r >= r' > D > Dq there and every fragment the old Tf pass wrote is written. At a sample k > 0 off its
// block's sample-0 surfaces (r'_k > D while r < Dq) the one pass writes no Tf where the old pass did; its T verdict
// (z_1 <= r) drops the same far fragments (PSB_LORES' block verdicts), so that sample reads no far layer in either
// channel. The margin covers D read as a constant by the shader and as a float here (1e-6 is ~17 ulp just below 1;
// D's raw lies below 1). Premises: the chain's D (KhWseeFarRaw) is the capture's
// within the margin (a depth-pair mismatch moves it), and the reader's depth is the live one or nearer (LoadDepthPS's
// VM_SEE substitution is farther: particles drawn after the view model's first translucent draw).
static constexpr float PSB_TF_EPS = 1.0e-6f;
// PSB - a particle-type re-issue (RE_NOWRITE / RE_NOWRITE_OFF; after wsee_re_save and psv_pick) drawn ONCE
// with its shader's one-pass variant in place of today's passes: no depth view bound, its phase's P / N pair (single
// sample, at 1 / s of the frame - PSB_LS: the viewports and, where the state scissors, the scissor rects set at 1 /
// s around the draw and back from the same arrays; psb_sdiv), the phase's blend state, the variant's constant buffer
// (which verdicts it makes and how: the five passes' own terms as wsee_post reads them, a fourth row for s and the
// reads - the patcher's note) and the depth's MS view at the variant's slots; Zb at its second slot with cb3.w while it
// stands this cycle. Its slots (the constant buffer's, the depth view's and, with Zb, Zb's) are put back here (the
// views before the restore binds the engine's depth: the runtime would null a view of a bound depth), the rest by the
// caller's restore. Verdicts: the pushes where WseeSurfVp's shift would stand (sfo, every bound viewport with a
// depth range whose MaxDepth is the world range's, taken as Hi), test-off for RE_NOWRITE_OFF, Tf where its pass would
// be drawn (the split on, a far raw, sfo, the world or beyond viewport) and r >= Dq (PSB_TF_EPS's note). False
// (nothing drawn or changed; the caller draws today's passes): no record this cycle, no variant, RE_NWONLY (its NW pass
// binds no pixel shader and costs less than the whole engine shader), the engine's sample mask not every sample, a
// depth bias or no depth clip (psb_rs_ok), no viewport or viewports of different depth ranges. For a particle-type
// draw this cannot take, an NW-only one included, the caller (wsee_post) sets g_psb.psa_cycle (lane bit 7 clears:
// every armed pixel loads the old record). No return or throw between its sets and their set-back.
template <class WseeDraw>
inline bool psb_post(ID3D11DeviceContext* ctx, uint8_t k, bool post, const PsvPick& pv,
                     bool vk, bool sfo, WseeDraw& draw) {
    if (!psb_on()) return false;
    if (k == WSEE_RE_NWONLY) return false;   // Its NW-only pass (no pixel shader), into the old record.
    const UINT ns = g_res.depth_sample_count;
    bool sci = false;                   // PSB_LORES: the engine's state scissors.
    const UINT all = ns >= 32u ? 0xFFFFFFFFu : ((1u << ns) - 1u);
    D3D11_VIEWPORT rd[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT n = g_wsee_pd_vpn;
    const D3D11_VIEWPORT* vp = g_wsee_pd_vp;
    if (!vk) {
        n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetViewports(&n, rd);
        vp = rd;
    }
    bool vok = n > 0u && n <= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    for (UINT i = 1u; i < n && vok; ++i) {
        vok = vp[i].MinDepth == vp[0].MinDepth && vp[i].MaxDepth == vp[0].MaxDepth;
    }
    // psb_rs_ok only past the first two terms (it pins a state).
    if (!pv.on || !pv.one || ns == 0u || (g_wsee_sv.mask & all) != all ||
        !psb_rs_ok(ctx, &sci) || !vok) {
        return false;
    }
    const float lo = vp[0].MinDepth, hi = vp[0].MaxDepth;
    const bool push = sfo && hi > lo && std::isfinite(lo) && std::isfinite(hi) &&
                      fabsf(hi - wsee_surf_max()) <= 1.0e-6f;
    const bool ld = wsee_ld_on();
    const bool off = k == WSEE_RE_NOWRITE_OFF;
    const bool tf = ld && g_wsee.ld_far_ok && sfo &&
                    (wsee_vp_world(ctx, vp, n) || wsee_vp_beyond(ctx, vp, n));
    // The verdict set's constant buffer, refilled where its Hi, D or sample count differ (bits, exactly).
    const int ix = (push ? 1 : 0) | (off ? 2 : 0) | (tf ? 4 : 0);
    // PSB_ZB: Zb bounds the loop where it stands this cycle (the capture built it and its guard has not
    // tripped; the depth view still the one S was read through - else a trip) and the variant has its slot.
    bool zb = g_res.wsee_zb_srv && pv.one_tz < 128u && psb_zb_armed();
    if (zb && g_res.depth_srv_gen != g_psb.zb_gen) {
        psb_zb_trip();
        zb = false;
    }
    uint32_t key[4] = { 0u, 0u, ns, PSB_LS | (zb ? 0x100u : 0u) };   // PSB_LORES / _ZB.
    memcpy(&key[0], &hi, 4);
    memcpy(&key[1], &g_psb.far_raw, 4);
    // PSB_CB_INJ: the refill under the injection flag with the pass's own binds below - an upload of ours, which
    // the UpdateSubresource hook otherwise scanned as an engine upload (upload_hook_wanted: not in_injection).
    InjectionFlagScope fl;
    g_ro.in_injection = true;
    if (!g_res.psb_cb_set[ix] || memcmp(g_res.psb_cb_key[ix], key, sizeof(key)) != 0) {
        static const float tol[3] = { KH_WSEE_SURF_TOL, KH_WSEE_NEAR_TOL, KH_WSEE_NEAR_TOL2 };
        float c[8] = {};
        for (int i = 0; i < 3; ++i) {
            c[i] = push ? 1.0f - tol[i] : 1.0f;      // m_i
            c[4 + i] = push ? tol[i] * hi : 0.0f;  // a_i
        }
        // Dq (PSB_TF_EPS's note) where Tf is drawn, else 0 - with the bands' mask off (the test-off kind)
        // the sample loop's bound is Dq alone, so that kind reads no sample where Tf is not drawn.
        c[3] = tf ? std::nextafter(g_psb.far_raw - PSB_TF_EPS, 2.0f) : 0.0f;
        c[7] = g_psb.far_raw;
        uint32_t u[16] = {};
        memcpy(u, c, sizeof(c));
        u[8] = off ? 0xFFFFFFFFu : 0u;
        u[9] = tf ? 0xFFFFFFFFu : 0u;
        u[10] = off ? 0u : 0xFFFFFFFFu;   // The bands' mask: their verdicts tested.
        // PSB_LORES: the reads - sample 0 of the block's s x s pixels (s = 1 << PSB_LS; the patcher's s = 1
        // arm, every sample of the pixel, is never taken); cb3: the patcher's note.
        u[11] = 1u << (2u * PSB_LS);
        u[12] = PSB_LS;
        u[13] = (1u << PSB_LS) - 1u;
        u[14] = 0u;
        u[15] = zb ? 0xFFFFFFFFu : 0u;   // PSB_ZB: cb3.w, Zb's flag.
        ctx->UpdateSubresource(g_res.psb_cb[ix], 0, nullptr, u, 0, 0);
        memcpy(g_res.psb_cb_key[ix], key, sizeof(key));
        g_res.psb_cb_set[ix] = true;
    }
    {
        ID3D11Buffer* ocb = nullptr;
        ID3D11ShaderResourceView* osrv = nullptr;
        ID3D11ShaderResourceView* ozb = nullptr;   // PSB_ZB: the engine's view at Zb's slot.
        // A D3D11.1 PSSetConstantBuffers1 offset / size at this slot would not survive the set-back (the engine's
        // next draw would read it from offset 0); the patcher takes the highest slot the shader leaves free, which the
        // engine is not known to bind.
        ctx->PSGetConstantBuffers(pv.one_cb, 1, &ocb);
        ctx->PSGetShaderResources(pv.one_t, 1, &osrv);
        // PSB_PHASE: the phase's pair alone - P0 / N pre, P1 / N2 post -, the split's channels written only while
        // it stands (bs_psb[1] takes T's alone; else nothing reads them: they keep their clear).
        ID3D11RenderTargetView* const rt[2] = { post ? g_res.wsee_p1_rtv : g_res.wsee_p0_rtv,
                                                post ? g_res.wsee_nb2_rtv : g_res.wsee_nb_rtv };
        ctx->OMSetRenderTargets(2, rt, nullptr);
        const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(g_res.bs_psb[ld ? 0 : 1], bf, 0xFFFFFFFF);
        ctx->PSSetShader(pv.one, nullptr, 0);
        ctx->PSSetConstantBuffers(pv.one_cb, 1, &g_res.psb_cb[ix]);
        // Legal: no depth view is bound. The engine's MS depth is read as a resource at every one-pass draw, between
        // the engine's own depth writes: a driver may decompress or wait at each write-to-read turn (not measured).
        ctx->PSSetShaderResources(pv.one_t, 1, &g_res.depth_srv);
        if (zb) {   // PSB_ZB: Zb at the variant's second slot (read only under cb3.w), set back below.
            ctx->PSGetShaderResources(pv.one_tz, 1, &ozb);
            ctx->PSSetShaderResources(pv.one_tz, 1, &g_res.wsee_zb_srv);
        }
        // PSB_LORES: the viewports (and the scissor rects, where the state scissors) at the targets'
        // scale - the same mapping of the scene onto 1 / s of the pixels (a pixel's centre lands on its block's
        // centre); set back below from the arrays they came from.
        D3D11_RECT po_or[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        UINT nr = 0;
        {   // s > 1 (PSB_LS).
            const float f = 1.0f / static_cast<float>(1u << PSB_LS);   // Exact: a power of two.
            D3D11_VIEWPORT sv[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
            for (UINT i = 0; i < n; ++i) {
                sv[i] = vp[i];
                sv[i].TopLeftX *= f;
                sv[i].TopLeftY *= f;
                sv[i].Width *= f;
                sv[i].Height *= f;
            }
            ctx->RSSetViewports(n, sv);
            if (sci) {
                nr = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
                ctx->RSGetScissorRects(&nr, po_or);
                if (nr > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) nr = 0;
                D3D11_RECT sr[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
                for (UINT i = 0; i < nr; ++i) {
                    sr[i].left = psb_sdiv(po_or[i].left, PSB_LS);
                    sr[i].top = psb_sdiv(po_or[i].top, PSB_LS);
                    sr[i].right = psb_sdiv(po_or[i].right, PSB_LS);
                    sr[i].bottom = psb_sdiv(po_or[i].bottom, PSB_LS);
                }
                if (nr > 0u) ctx->RSSetScissorRects(nr, sr);
            }
        }
        if (g_wsee_gput.blk >= 0) wsee_gput_pass(ctx, WSEE_GPU_PART, WSEE_GPU_P_ONE);   // WSEE_GPUT: the one pass.
        draw();
        ctx->RSSetViewports(n, vp);   // PSB_LORES: the engine's own again.
        if (nr > 0u) ctx->RSSetScissorRects(nr, po_or);
        if (zb) ctx->PSSetShaderResources(pv.one_tz, 1, &ozb);   // PSB_ZB.
        ctx->PSSetShaderResources(pv.one_t, 1, &osrv);
        ctx->PSSetConstantBuffers(pv.one_cb, 1, &ocb);
        KH_SAFE_RELEASE(ozb);
        KH_SAFE_RELEASE(osrv);
        KH_SAFE_RELEASE(ocb);
    }
    g_psb.drew_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    return true;
}

// The draw hooks' post (render thread, the tracked context's own engine draw, never ours - the hooks' gate): the
// engine's draw again (draw: the original, unhooked call with the hook's own arguments) for the kind its
// pre-draw marked - a fence-type layer into T; a particle-type one into T then NW (and Tp / Tp2 / Tf), or the one
// pass; a light-adding one into NW alone; a late opaque into T's flag (PASS_ONE). One save and one restore a post. A
// refused or impossible re-issue draws nothing and taints the cycle (T or NW would miss the layer). Nothing here
// throws.
template <class WseeDraw>
inline void wsee_post(ID3D11DeviceContext* ctx, uint8_t k, WseeDraw&& draw) {
    const bool opq = k == WSEE_RE_OPAQUE;
    // The viewports the pre-draw read for this draw (g_wsee_pd_vp's note), taken here - once.
    const bool vk = g_wsee_pd_vp_ok;
    g_wsee_pd_vp_ok = false;
    if (!wsee_re_ready(k) || !wsee_re_save(ctx, opq)) {
        wsee_taint(g_topo_cycles);
        return;
    }
    const bool nw = k == WSEE_RE_NOWRITE || k == WSEE_RE_NOWRITE_OFF ||
                    k == WSEE_RE_NWONLY;
    // An engine draw that tests stencil: a depth writer's passes are EQUAL against the depth it left (exactly the
    // samples it wrote, stencil included - nothing replayed); a kind with an NW pass cannot be recorded (NW's D32
    // holds no stencil, and it writes no depth to select by) and the pre-draw leaves it out by the DSS hook's latch,
    // which only the engine's binds set and none comes between a pre-draw and its post - so this block is a backstop,
    // read off the saved state when the latch says stencil.
    if (nw && g_ro.dss_stencil && g_wsee_sv.dss) {
        D3D11_DEPTH_STENCIL_DESC sd = {};
        g_wsee_sv.dss->GetDesc(&sd);
        if (sd.StencilEnable) {
            wsee_re_restore(ctx);
            return;
        }
    }
    // A particle-type layer's phase (the pre-draw's); a late opaque's too (wsee_mark_opaque).
    const bool post = (nw || opq) && g_wsee.pending_post;   // A late opaque's: always true.
    // PSV: a particle-type kind's passes draw the variants of the engine's pixel shader where they are made
    // (psv_pick; the shader it took is the save's - wsee_re_restore sets it back, as a late opaque's); none,
    // nothing is taken or set - today's passes.
    PsvPick pv;
    if (nw) {
        psv_pick(ctx, pv, false);
        if (pv.on) {
            g_wsee_sv.ps = pv.orig;
            g_wsee_sv.has_ps = true;
            pv.orig = nullptr;
        }
    }
    // A particle-type kind's depth-tested T pass and its NW pass take the surface shift (WseeSurfVp's note); the
    // test-off T pass (no test) and the depth writers' EQUAL passes do not; the test-off kind's NW pass does (its
    // position exact through the shaders' inverse). Set back before the restore.
    WseeSurfVp sv;
    if (vk) sv.keep(g_wsee_pd_vp, g_wsee_pd_vpn);   // The pre-draw's array: no read of its own.
    // The saved depth state's verdict, once (sfo; the Tf pass below takes it too).
    const bool sfo = nw && wsee_surf_ok(g_wsee_sv.dss);
    bool sok = sfo;
    // PSB: a particle-type kind in ONE draw where it can (psb_post; its passes' verdicts in the shader), else
    // today's passes below - into the old record, which the shaders then read beside the one pass's (g_psb.psa_cycle:
    // lane bit 7 clears, the chain and the sort load the old record; the late-opaque replays reset Tp from
    // g_psb.psa_tp_cycle, which the Tp passes below stamp).
    const bool one = nw && psb_post(ctx, k, post, pv, vk, sfo, draw);
    if (nw && !one && g_psb.psa_cycle.load(std::memory_order_relaxed) != g_topo_cycles) {
        g_psb.psa_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (k != WSEE_RE_NWONLY && !one) {
        if (sok && k == WSEE_RE_NOWRITE) sok = sv.begin(ctx);
        wsee_re_set(ctx, opq ? WSEE_PASS_ONE
                         : k == WSEE_RE_NOWRITE_OFF ? WSEE_PASS_T_OFF
                         : k == WSEE_RE_BLEND ? WSEE_PASS_T_EQ : WSEE_PASS_T, post,
                    &pv);   // PSV.
        draw();
    }
    if (nw && !one) {
        if (sok) sok = sv.begin(ctx);
        wsee_re_set(ctx, WSEE_PASS_NW, post, &pv);   // PSV: nw, or no pixel shader.
        draw();
    }
    // The layer-depth split - a kind with a T pass again into Tp (the depth-tested kind under the near push where the
    // surface shift stood - else unpushed: as its T pass, no near part; the test-off kind untested, as its T) and
    // into Tf (only where every viewport is the world range D was computed in, or lies at or beyond its far side -
    // wsee_vp_world / wsee_vp_beyond - and the depth is not reversed; else not drawn: Tf then lacks this
    // layer's far part, which only the far pixels read).
    if (nw && k != WSEE_RE_NWONLY && wsee_ld_on() && !one) {
        sv.end(ctx);
        // PSB_TPA: the replays reset Tp from here.
        g_psb.psa_tp_cycle.store(g_topo_cycles, std::memory_order_relaxed);
        bool np = false;
        if (k == WSEE_RE_NOWRITE) {
            if (sok) np = sv.begin(ctx, KH_WSEE_NEAR_TOL);
            wsee_re_set(ctx, WSEE_PASS_TP, post, &pv);   // PSV.
        } else {
            wsee_re_set(ctx, WSEE_PASS_TP_OFF, post, &pv);
        }
        draw();
        sv.end(ctx);
        // The same pass once more into Tp's B / A under the 100 % band's push (where the 25 % push stood - the same
        // shift test; else unpushed, as its T: no band part); the test-off kind untested.
        if (k == WSEE_RE_NOWRITE) {
            if (np) sv.begin(ctx, KH_WSEE_NEAR_TOL2);
            wsee_re_set(ctx, WSEE_PASS_TP2, post, &pv);   // PSV.
        } else {
            wsee_re_set(ctx, WSEE_PASS_TP2_OFF, post, &pv);
        }
        draw();
        sv.end(ctx);
        if (g_wsee.ld_far_ok && sfo &&
            (wsee_vp_world(ctx, vk ? g_wsee_pd_vp : nullptr, g_wsee_pd_vpn) ||
             wsee_vp_beyond(ctx, vk ? g_wsee_pd_vp : nullptr, g_wsee_pd_vpn))) {
            wsee_re_set(ctx, WSEE_PASS_TF, post, &pv);   // PSV.
            draw();
        }
    }
    // WseeSurfVp has no destructor - nothing between its begin above and this end may return or throw (the
    // stencil backstop returns before it exists).
    sv.end(ctx);
    wsee_re_restore(ctx);
}

// Our translucent tail (flush_locked's normal-blend translucents past its solids that test the depth without writing
// it - a whole translucent object, a blend material's part - and inject_composited_meshes' normal-blend parts drawn at
// the injection) is a blended no-write layer as the engine's particles are: owed into T and NW while this cycle's
// capture stands (landed, untainted, unused, the targets made against the depth the caller draws on). The caller's
// thread (render thread, or a park that stalls it: the cycle is g_topo_cycles, wsee_taint's).
inline bool wsee_tail_owed() {
    if constexpr (!WSEE_ON) {   // Built out: no code left unreachable (C4702).
        return false;
    } else {
        const uint64_t c = g_topo_cycles;
        return g_wsee.ok_cycle.load(std::memory_order_relaxed) == c &&
               g_wsee.taint_cycle.load(std::memory_order_relaxed) != c &&
               g_wsee.used_cycle.load(std::memory_order_relaxed) != c && g_res.wsee_t_rtv && g_res.wsee_n_dsv &&
               g_res.depth_dsv_ro && g_res.dss_test && g_res.bs_vmsee_t && g_res.dss_wsee_nw &&
               g_res.wsee_depth_id && g_res.wsee_n2_dsv && g_res.bs_wsee_tpre && g_res.bs_wsee_tpost &&
               g_res.wsee_depth_id == g_res.depth_res_identity && g_res.wsee_samples == g_res.depth_sample_count;
    }
}

// One owed tail draw (wsee_tail_owed first): the caller's draw again, draw(true) into T over depth_dsv_ro
// (dss_test; Tpre or Tpost *= 1 - a: the caller's PS writes its coverage as straight alpha - PSComposite / PSMain's
// normal-blend output, the perceptual arm dropped for it), then draw(false) depth-only into NW (dss_wsee_nw);
// and the Tp / Tp2 / Tf passes under the shifts; every sample. Saved and set back here (OmSave with UAVs, blend
// state / factor / mask, depth state / reference; WseeSurfVp the viewports) so the caller's bound-state trackers
// stay true. Two callers: flush_locked's tail (g_flush_active, so the OM hooks stand aside without in_injection)
// and inject_composited_meshes' tail (under in_injection, which they stand aside for). False = refused (the bound
// depth is not the capture's): nothing drawn, the caller taints.
template <class WseeTailDraw>
inline bool wsee_tail(ID3D11DeviceContext* ctx, WseeTailDraw&& draw) {
    OmSave om;
    om.capture(ctx);
    if (!om.dsv || reorder_dsv_identity(om.dsv) != g_res.wsee_depth_id) {
        om.release();   // Nothing changed: no restore owed (release disarms).
        return false;
    }
    WseeGputScope gt(ctx, WSEE_GPU_TAIL);   // WSEE_GPUT: its passes and its restore.
    ID3D11BlendState* bs = nullptr;
    FLOAT bf[4] = {};
    UINT mask = 0xFFFFFFFFu;
    ctx->OMGetBlendState(&bs, bf, &mask);
    ID3D11DepthStencilState* dss = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&dss, &ref);
    const FLOAT z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    // Our layer is particle-type (it writes no depth): drawn after this cycle's B2, into Tpost (T's A) and NWpost;
    // before it, into Tpre (T's B) and NWpre.
    const bool post = g_wsee.b2_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
    // Both passes take the surface shift, as the engine's particle-type re-issues do (WseeSurfVp's note): a part of
    // ours within KH_WSEE_SURF_TOL of the surface behind it (a decal mesh, a pane set in a wall) is that surface's for
    // the chain and the later sorted parts; our depth test is LESS_EQUAL. The caller's draws set no viewport
    // (draw_textured, inst_draw); one without a depth range or whose MaxDepth is not the world range's is
    // unshifted. WseeSurfVp has no destructor: no return or throw before sv.end. PSV: the bound shader is
    // the caller's builtin (draw_textured binds the builtin after a user material's shader; psv_pick's tail
    // entries); its alpha variant before every T / Tp / Tp2 / Tf draw, nw (or none) before NW's, the builtin back
    // below. No variant: the builtin as bound.
    PsvPick pv;
    psv_pick(ctx, pv, true);
    // PSB: our layer goes into the old record (the builtins have no one-pass variant: a second target, a depth or
    // coverage output) - the shaders read it this cycle, the later late-opaque replays reset it.
    if (g_psb.psa_cycle.load(std::memory_order_relaxed) != g_topo_cycles) {
        g_psb.psa_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    WseeSurfVp sv;
    const bool sok = sv.begin(ctx);
    ctx->OMSetRenderTargets(1, &g_res.wsee_t_rtv, g_res.depth_dsv_ro);
    ctx->OMSetBlendState(post ? g_res.bs_wsee_tpost : g_res.bs_wsee_tpre, z, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_res.dss_test, 0);
    if (pv.on) ctx->PSSetShader(pv.alpha, nullptr, 0);   // PSV.
    draw(true);
    ctx->OMSetRenderTargets(0, nullptr, post ? g_res.wsee_n2_dsv : g_res.wsee_n_dsv);
    ctx->OMSetBlendState(nullptr, z, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_res.dss_wsee_nw, 0);
    if (pv.on) ctx->PSSetShader(pv.nw, nullptr, 0);   // PSV.
    draw(false);
    sv.end(ctx);
    // Our layer into Tp (the near push where the surface shift stood, else unpushed as its T pass) and Tf (the
    // world-range viewport alone), as wsee_post does the engine's - else T would hold a layer Tp lacks and the
    // chain would take ours for a near-surface one.
    if (wsee_ld_on()) {
        // PSB_TPA: the replays reset Tp from here.
        g_psb.psa_tp_cycle.store(g_topo_cycles, std::memory_order_relaxed);
        if (sok) sv.begin(ctx, KH_WSEE_NEAR_TOL);
        ctx->OMSetRenderTargets(1, &g_res.wsee_tp_rtv, g_res.depth_dsv_ro);
        ctx->OMSetBlendState(post ? g_res.bs_wsee_ldpost : g_res.bs_wsee_ldpre, z, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_test, 0);
        if (pv.on) ctx->PSSetShader(pv.alpha, nullptr, 0);   // PSV.
        draw(true);
        sv.end(ctx);
        // And into Tp's B / A under the 100 % band's push, as wsee_post does the engine's.
        if (sok) sv.begin(ctx, KH_WSEE_NEAR_TOL2);
        ctx->OMSetBlendState(post ? g_res.bs_wsee_ldpost2 : g_res.bs_wsee_ldpre2, z, 0xFFFFFFFF);
        if (pv.on) ctx->PSSetShader(pv.alpha, nullptr, 0);   // PSV.
        draw(true);
        sv.end(ctx);
        if (g_wsee.ld_far_ok && wsee_vp_world(ctx)) {
            ctx->OMSetRenderTargets(1, &g_res.wsee_tf_rtv, g_res.wsee_tfd_dsv);
            ctx->OMSetDepthStencilState(g_res.dss_wsee_far, 0);
            if (pv.on) ctx->PSSetShader(pv.alpha, nullptr, 0);   // PSV.
            draw(true);
        }
    }
    om.restore(ctx);
    om.release();
    ctx->OMSetBlendState(bs, bf, mask);
    ctx->OMSetDepthStencilState(dss, ref);
    if (pv.on) ctx->PSSetShader(pv.orig, nullptr, 0);   // PSV: the builtin back (the flush's trackers).
    KH_SAFE_RELEASE(pv.orig);
    KH_SAFE_RELEASE(bs);
    KH_SAFE_RELEASE(dss);
    return true;
}

// The capture at the injection's translucent tail (inject_composited_meshes, after its solids and their AO, before
// its first translucent part): S and B hold our solids and none of our layers; the engine's later blended draws find
// it landed and only re-issue. In a wanted cycle not yet attempted (g_wsee.on), with the trigger's world viewport
// recorded (trig_vp_valid) and this injection the world's own (world: not an anomalous rescue, the engine's
// viewport at its draw a world one - trig_vp_valid alone is sticky across a re-armed attempt). RT0 must be the scene
// texture and the bound depth the main one (wsee_snapshot's checks). Render thread, under in_injection.
inline void wsee_inj_capture(ID3D11DeviceContext* ctx, bool world) {
    const uint64_t c = g_topo_cycles;
    if (!g_wsee.on || !world || !g_ro.trig_vp_valid || g_wsee.snap_cycle == c ||
        g_wsee.used_cycle.load(std::memory_order_relaxed) == c ||
        g_wsee.ok_cycle.load(std::memory_order_relaxed) == c ||
        g_wsee.taint_cycle.load(std::memory_order_relaxed) == c) return;
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return;
    if (rtv_is_scene(rtv)) {   // EA_SCENE_RTV.
        g_wsee.snap_cycle = c;
        if (wsee_snapshot(ctx, rtv)) g_wsee.ok_cycle.store(c, std::memory_order_relaxed);
    }
    rtv->Release();
}

// Whether the engine's latched pair's near differs from the pair given (pv: the flush's, or the injection's own
// encode) by more than the sort's relative tolerance (KH_WSEE_MS_REL, 2e-3: the decode's error in metres follows the
// near): a partitioned cycle, whose main depth holds raws of more than one encode. An unpartitioned cycle's pv is the
// engine's world pair by construction (snap_world), so the two agree to rounding. No latched pair: false. Used by
// the flush's sort mode and the injection's deferral gate.
inline bool wsee_pair_split(float m22, float m32) {
    if (!g_ro.engine_proj_valid) return false;
    const float pn = fabsf(m22) > 1e-9f ? -m32 / m22 : -1.0f;
    const float en = fabsf(g_ro.engine_m22) > 1e-9f ? -g_ro.engine_m32 / g_ro.engine_m22 : -1.0f;
    return !(pn > 0.0f) || !(en > 0.0f) || fabsf(en - pn) > 2.0e-3f * pn;
}

// What the sorted draw binds and blends with, all made: S, T, B, NWpre, NWpost and the main depth's view (MS: the
// mesh unit declares T and it as Texture2DMS; a landed capture implies it), no depth test, the normal blend and the
// layer-aware composite's dual-source state.
inline bool wsee_sort_ready() {
    return g_res.depth_srv && g_res.wsee_s_srv && g_res.wsee_t_srv && g_res.wsee_b_srv && g_res.wsee_n_srv &&
           g_res.wsee_n2_srv &&   // NWpost (t59 for the mesh unit).
           g_res.wsee_b_tex && g_res.dss_off && g_res.blend_modes[0] && g_res.bs_wsee_sort &&
           g_res.depth_sample_count >= 2;
}

// The composite's capture Q (Resources' note): made with B's desc (the scene target's view
// format, single sample), kept while it matches B; false = none (a refused make latches until the device goes).
inline bool wsee_q_ensure(ID3D11Device* dev) {
    if (!dev || !g_res.wsee_b_tex) return false;
    D3D11_TEXTURE2D_DESC bd = {};
    g_res.wsee_b_tex->GetDesc(&bd);
    if (g_res.wsee_q_tex && g_res.wsee_q_srv) {
        D3D11_TEXTURE2D_DESC qd = {};
        g_res.wsee_q_tex->GetDesc(&qd);
        if (qd.Width == bd.Width && qd.Height == bd.Height && qd.Format == bd.Format &&
            qd.SampleDesc.Count == 1) return true;
    }
    KH_SAFE_RELEASE(g_res.wsee_q_srv);
    KH_SAFE_RELEASE(g_res.wsee_q_tex);
    if (g_res.wsee_q_failed) return false;
    if (FAILED(dev->CreateTexture2D(&bd, nullptr, &g_res.wsee_q_tex)) || !g_res.wsee_q_tex ||
        FAILED(dev->CreateShaderResourceView(g_res.wsee_q_tex, nullptr, &g_res.wsee_q_srv)) || !g_res.wsee_q_srv) {
        KH_SAFE_RELEASE(g_res.wsee_q_srv);
        KH_SAFE_RELEASE(g_res.wsee_q_tex);
        g_res.wsee_q_failed = true;
        report_error_once_safe("KH RenderIntegration: world see-through: the sorted tail's scene capture could not "
                               "be made (our translucent parts draw where they did, before the game's later layers)");
        return false;
    }
    return true;
}

// The texture Q resolves from, given an RT0 (rtv): the scene texture
// (g_topo_scene_tex_id) in B's view format, multisampled at the capture's count and size - what the capture resolved
// into B. A reference back, or null.
inline ID3D11Texture2D* wsee_q_source(ID3D11RenderTargetView* rtv) {
    if (!rtv || !g_res.wsee_b_tex || g_topo_scene_tex_id == nullptr) return nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC vd = {};
    rtv->GetDesc(&vd);
    if (vd.Format != g_res.wsee_b_fmt || vd.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2DMS) return nullptr;
    ID3D11Resource* r = nullptr;
    rtv->GetResource(&r);
    if (!r) return nullptr;
    ID3D11Texture2D* t = nullptr;
    if (static_cast<void*>(r) != g_topo_scene_tex_id ||
        FAILED(r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&t)))) {
        t = nullptr;
    }
    r->Release();
    if (!t) return nullptr;
    D3D11_TEXTURE2D_DESC d = {};
    t->GetDesc(&d);
    if (d.Width != g_res.wsee_w || d.Height != g_res.wsee_h ||
        d.SampleDesc.Count != g_res.wsee_samples || d.MipLevels != 1 || d.ArraySize != 1) {
        t->Release();
        return nullptr;
    }
    return t;
}

// Q taken: the bound RT0 (the flush's scene target) resolved into it through the ORIGINAL resolve (wsee_snapshot's
// rule), with no target bound meanwhile (Q is off t58: the scope nulls it at its end). The flush is g_flush_active,
// so the OM hooks stand aside. False = not taken (nothing changed).
inline bool wsee_q_capture(ID3D11DeviceContext* ctx) {
    if (!ctx || !g_res.wsee_q_tex || !g_orig_resolvesubresource) return false;
    OmSave om;
    om.capture(ctx);
    ID3D11Texture2D* const src = wsee_q_source(om.rtv[0]);
    if (!src) {
        om.release();
        return false;
    }
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    g_orig_resolvesubresource(ctx, g_res.wsee_q_tex, 0, src, 0, g_res.wsee_b_fmt);
    om.restore(ctx);
    om.release();
    src->Release();
    return true;
}

// Whether the injection leaves its translucent parts to the flush's tail this cycle: the capture wanted and landed
// untainted and unused; the cycle's render-thread scene flush still to come (g_rtshadow_cycle: not yet run this
// cycle) and alive (flush_frame's test, a render-thread flush within two cycles: under a park regime - the bootstrap,
// WATCH_STALL, an unnamed scene texture - the park's flush is another cycle's and would drop the parts at the
// clear); every view and state made (wsee_sort_ready, the dual-source state included), Q made and the bound
// target one Q resolves (any scene format); no deferral back-off (g_wsee.defer_back). Else the parts draw at the
// injection and into T / NW. What can still change by the flush (a taint, the target) falls back to the flush's
// plain tail. The caller adds: the world's own injection, no far phase and no partitioned pair (wsee_pair_split).
inline bool wsee_defer_ok(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    const uint64_t c = g_topo_cycles;
    const uint64_t last = g_rtshadow_cycle.load(std::memory_order_relaxed);
    if (!(g_wsee.on && c >= g_wsee.defer_back &&
          g_wsee.ok_cycle.load(std::memory_order_relaxed) == c &&
          g_wsee.taint_cycle.load(std::memory_order_relaxed) != c &&
          g_wsee.used_cycle.load(std::memory_order_relaxed) != c && reorder_on_render_thread() &&
          g_track_wanted.load(std::memory_order_relaxed) &&
          last != c && last != ~0ull && c - last <= 2ull && wsee_sort_ready() &&
          wsee_q_ensure(dev))) return false;
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    ID3D11Texture2D* const src = wsee_q_source(rtv);
    if (rtv) rtv->Release();
    if (!src) return false;
    src->Release();
    return true;
}

// WSEE - one deferred object (its copy, as the injection drew it; draw_part 1: the flush draws the translucent
// part alone). The record is the cycle's (g_wsee.defer_cycle's note).
inline void wsee_defer_note(const RenderObject& o) {
    const uint64_t c = g_topo_cycles;
    if (g_wsee.defer_cycle != c) {
        g_wsee.defer_objs.clear();
        g_wsee.defer_cycle = c;
    }
    g_wsee.defer_objs.push_back(o);
    g_wsee.defer_objs.back().draw_part = 1;
    g_wsee.defer_objs.back().ws_dcopy = 1u;   // WSEE_DEFER_FLAG: the flush's tail knows the copy by this.
}

// Deferring a part whose geometry is the object's own (a skinned pose, a cloth / chain buffer, a vertex stage's
// output): those buffers are rewritten or recycled before the flush's tail (its uploads, pool grows and rebinds, the
// consume-point steps, the skin catch-up, a park's flush), so the part's vertices are COPIED at the injection, where
// its object is noted (after the cycle's steps and the solids that read them, before any later writer), by GPU copies
// on the immediate context: every later write, whatever thread or pass issues it, is ordered after them. Pinning the
// slot's buffer would not do (Map DISCARD renames its memory, stream output writes in place), nor holding the uploads
// back (every other pass draws from the slot). own: an object drawn from its mesh's own buffer needs no copy - its
// entry names that buffer and the flush binds it whatever the slot holds by then; WSEE_OWN_STATIC: a static object
// takes such an entry too (the slot may gain a cloth, pose or stage buffer before the flush's tail, and the entry
// carries the injection's level and crossfade). Only what the flush can draw is copied: the translucent submeshes
// (alpha mode 2) at the injection's LOD pick (wsee_hold_lod), as vertex runs (wsee_spans_of; gaps <=
// WSEE_RUN_GAP merged), each at its own place relative to base, the IA offset the flush binds (vertex v at base + 48
// v; DrawIndexed with base vertex 0 and the mesh's absolute indices, all inside the runs; base may lie on earlier
// entries' bytes, which no index of this entry reaches). The entry spans [off, off + bytes) = vmin .. vmax. Nothing
// translucent at those levels: own, no copy. A mesh whose index ranges cannot be read, or an object without a material
// set, copies the whole array.
struct WseeRun {
    uint32_t a = 0u;   // Vertices [a, b).
    uint32_t b = 0u;
};
struct WseeHoldPlan {
    int           mesh = -1;
    bool          own = false;
    ID3D11Buffer* src = nullptr;
    UINT          off = 0u;       // The source's vertex 0 (its slice's offset).
    int           lvl = 0;        // The injection's pick (wsee_hold_lod's).
    float         t = 0.0f;
    uint32_t      r0 = 0u;        // Its runs in g_wsee_run_plan [r0, r1).
    uint32_t      r1 = 0u;
    UINT          h0 = 0u;        // Its place in the hold (wsee_defer_group's layout): vmin at h0.
    UINT          base = 0u;
};
static constexpr UINT WSEE_HOLD_ALIGN = 256u;
static constexpr UINT WSEE_HOLD_MIN = 1u << 20;   // The hold buffer's first size (bytes).
static constexpr UINT WSEE_HOLD_CAP = 1u << 30;   // The hold's largest size (bytes).
static constexpr size_t WSEE_HOLD_MAX = 0xFFFFu;   // Entries a cycle (RenderObject::ws_hold's 16 bits).
static constexpr uint32_t WSEE_RUN_GAP = 256u;   // Vertices: a smaller gap is copied (12 KB) rather than split.
static std::vector<WseeRun> g_wsee_run_plan;   // The runs of wsee_defer_group's current plans.
// Per mesh (ids are never reused; a mesh's index stream never changes): the vertex runs of every (level, submesh)
// index range, levels 0 to lod_n in order, each level's submesh table in order (slot lvl_slot[level] + submesh); slot
// k's runs are runs[first[k], first[k + 1]). Built once from the CPU index stream against nv vertices (the GPU
// array's count); ok false: unreadable - the caller copies the whole array. Render thread; cleared with the hold
// (wsee_hold_drop).
struct WseeSpans {
    bool built = false;
    bool ok = false;
    uint32_t nv = 0u;
    std::vector<uint32_t> lvl_slot;
    std::vector<uint32_t> first;
    std::vector<WseeRun> runs;
};
static std::unordered_map<int, WseeSpans> g_wsee_spans;
inline void wsee_runs_merge(std::vector<WseeRun>& v, size_t from) {   // Sorted by a, merged in place.
    std::sort(v.begin() + static_cast<std::ptrdiff_t>(from), v.end(),
              [](const WseeRun& x, const WseeRun& y) { return x.a < y.a; });
    size_t o = from;
    for (size_t i = from; i < v.size(); ++i) {
        if (o > from && v[i].a <= v[o - 1u].b + WSEE_RUN_GAP) {
            if (v[i].b > v[o - 1u].b) v[o - 1u].b = v[i].b;
        } else {
            v[o++] = v[i];
        }
    }
    v.resize(o);
}
inline const WseeSpans& wsee_spans_of(int m, uint32_t nv) {
    WseeSpans& s = g_wsee_spans[m];
    if (s.built && s.nv == nv) return s;
    s = WseeSpans{};
    s.built = true;
    s.nv = nv;
    const MeshDef& d = mesh_def(m);
    const size_t in = d.indices.size();
    if (in == 0u || nv == 0u || d.lod_n > LOD_MAX) return s;
    static std::vector<uint32_t> seen;
    seen.assign(nv, 0u);
    uint32_t tick = 0u;
    for (int l = 0; l <= static_cast<int>(d.lod_n); ++l) {
        s.lvl_slot.push_back(static_cast<uint32_t>(s.first.size()));
        const std::vector<MeshSubmesh>& tab = l == 0 ? d.submeshes : d.lod_sub[l - 1];
        for (const MeshSubmesh& sm : tab) {
            s.first.push_back(static_cast<uint32_t>(s.runs.size()));
            if (sm.index_count == 0u) continue;
            if (sm.index_start > in || sm.index_count > in - sm.index_start) return s;
            ++tick;
            const size_t r0 = s.runs.size();
            for (uint32_t i = 0; i < sm.index_count; ++i) {
                const uint32_t v = d.indices[sm.index_start + i];
                if (v >= nv) return s;
                if (seen[v] == tick) continue;
                seen[v] = tick;
                s.runs.push_back({ v, v + 1u });
            }
            wsee_runs_merge(s.runs, r0);
        }
    }
    s.lvl_slot.push_back(static_cast<uint32_t>(s.first.size()));
    s.first.push_back(static_cast<uint32_t>(s.runs.size()));
    s.ok = true;
    return s;
}
// What holding o's geometry takes, at the injection's pick (lvl, and lvl + 1 where t > 0): the
// vertices its draws read now (mesh_vb_ref - the buffer and its slice's offset), the runs the flush can draw of them
// appended to g_wsee_run_plan; false = it cannot be held (no mesh buffer, a source smaller than the array, the hold
// itself).
inline bool wsee_hold_plan(const RenderObject& o, int lvl, float t, WseeHoldPlan& p) {
    p = WseeHoldPlan{};   // WSEE_OWN_STATIC: every object plans an entry (own where the slot is the mesh's).
    const int m = mesh_id_clamp(o.mesh);
    if (m < 0 || static_cast<size_t>(m) >= g_res.mesh_vb.size() || !g_res.mesh_vb[m]) return false;
    p.mesh = m;
    p.lvl = lvl;
    p.t = t > 0.0f ? t : 0.0f;
    const VbRef r = mesh_vb_ref(m, o.slot);
    if (r.vb == g_res.mesh_vb[m]) {
        p.own = true;
        return true;
    }
    if (!r.vb || r.vb == g_res.wsee_hold_vb) return false;
    D3D11_BUFFER_DESC md = {}, sd = {};
    g_res.mesh_vb[m]->GetDesc(&md);
    r.vb->GetDesc(&sd);
    if (md.ByteWidth < sizeof(MeshVertex) || sd.Usage == D3D11_USAGE_STAGING ||
        r.off > sd.ByteWidth || sd.ByteWidth - r.off < md.ByteWidth ||
        md.ByteWidth > WSEE_HOLD_CAP) return false;
    p.src = r.vb;
    p.off = r.off;
    const uint32_t nv = md.ByteWidth / static_cast<UINT>(sizeof(MeshVertex));
    p.r0 = static_cast<uint32_t>(g_wsee_run_plan.size());
    const WseeSpans& s = wsee_spans_of(m, nv);
    const MaterialSet* const ms = obj_textured(o);
    if (!s.ok || !ms) {
        g_wsee_run_plan.push_back({ 0u, nv });   // The whole array.
    } else {
        const MeshDef& d = mesh_def(m);
        for (int k = 0; k < (p.t > 0.0f ? 2 : 1); ++k) {
            const int l = mesh_lod_clamp(d, lvl + k);   // draw_textured's clamp.
            if (k == 1 && l == mesh_lod_clamp(d, lvl)) break;
            const uint32_t s0 = s.lvl_slot[static_cast<size_t>(l)];
            const uint32_t s1 = s.lvl_slot[static_cast<size_t>(l) + 1u];
            for (uint32_t si = 0; s0 + si < s1; ++si) {
                // draw_textured's part-2 test: a used slot of alpha mode 2 (an unused slot's default is opaque).
                if (si >= ms->slots.size() || !ms->slots[si].used ||
                    ms->slots[si].alpha_mode != 2) continue;
                const uint32_t k0 = s.first[s0 + si];
                const uint32_t k1 = s.first[s0 + si + 1u];
                for (uint32_t j = k0; j < k1; ++j) {
                    g_wsee_run_plan.push_back(s.runs[j]);
                }
            }
        }
        wsee_runs_merge(g_wsee_run_plan, p.r0);
    }
    p.r1 = static_cast<uint32_t>(g_wsee_run_plan.size());
    if (p.r1 == p.r0) {   // Nothing translucent at those levels: nothing the flush draws reads a vertex.
        p.own = true;
        p.src = nullptr;
    }
    return true;
}
inline void wsee_hold_begin() {   // The cycle's first hold drops the last cycle's entries.
    if (g_wsee.hold_cycle != g_topo_cycles) {
        g_wsee.hold_cycle = g_topo_cycles;
        g_wsee.hold.clear();
        g_wsee.hold_used = 0u;
    }
}
// Room in the hold up to end bytes (this cycle's entries kept: a growth copies them to the new buffer's start,
// ordered before anything later). False = refused (past WSEE_HOLD_CAP, or the device refused the buffer, now or
// before at this size or a smaller one: WSEE_HOLD_FAIL, Resources::wsee_hold_fail_cap).
inline bool wsee_hold_reserve(ID3D11Device* dev, ID3D11DeviceContext* ctx, UINT end) {
    wsee_hold_begin();
    if (end > WSEE_HOLD_CAP) return false;
    if (g_res.wsee_hold_vb && end <= g_res.wsee_hold_cap) return true;
    if (!dev) return false;
    UINT cap = g_res.wsee_hold_cap > WSEE_HOLD_MIN ? g_res.wsee_hold_cap : WSEE_HOLD_MIN;
    while (cap < end) cap *= 2u;
    // WSEE_HOLD_FAIL: a size the device refused (or a larger one) is not asked for every cycle;
    // WSEE_HOLD_FAIL_RETRY: the ask goes through once WSEE_HOLD_FAIL_RETRY cycles after the refusal (one
    // CreateBuffer per that many cycles while it keeps failing).
    if (g_res.wsee_hold_fail_cap != 0u && cap >= g_res.wsee_hold_fail_cap &&
        g_topo_cycles >= g_res.wsee_hold_fail_cycle &&   // A cycle count that started over: ask again.
        g_topo_cycles - g_res.wsee_hold_fail_cycle < WSEE_HOLD_FAIL_RETRY) return false;
    D3D11_BUFFER_DESC d = {};
    d.ByteWidth = cap;
    d.Usage = D3D11_USAGE_DEFAULT;
    d.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    ID3D11Buffer* vb = nullptr;
    if (FAILED(dev->CreateBuffer(&d, nullptr, &vb)) || !vb) {
        g_res.wsee_hold_fail_cap = cap;   // WSEE_HOLD_FAIL.
        g_res.wsee_hold_fail_cycle = g_topo_cycles;   // WSEE_HOLD_FAIL_RETRY.
        return false;
    }
    if (cap >= g_res.wsee_hold_fail_cap) {   // WSEE_HOLD_FAIL_RETRY: a retry at the refused size succeeded.
        g_res.wsee_hold_fail_cap = 0u;             // (A smaller size's success proves nothing about it: kept.)
        g_res.wsee_hold_fail_cycle = 0u;
    }
    if (g_res.wsee_hold_vb && g_wsee.hold_used > 0u) {
        const D3D11_BOX b = { 0u, 0u, 0u, g_wsee.hold_used, 1u, 1u };
        ctx->CopySubresourceRegion(vb, 0, 0, 0, 0, g_res.wsee_hold_vb, 0, &b);
    }
    if (!g_res.wsee_hold_vb) ++g_res.wsee_hold_gen;   // A new hold: entries of an older one are void.
    KH_SAFE_RELEASE(g_res.wsee_hold_vb);
    g_res.wsee_hold_vb = vb;
    g_res.wsee_hold_cap = cap;
    return true;
}
// One planned hold taken (wsee_defer_group laid it out and reserved the room, and checked the count): its runs
// copied, each to base + 48 a, and its entry, 1-based (the copy's ws_hold).
inline uint16_t wsee_hold_take(ID3D11DeviceContext* ctx, const WseeHoldPlan& p) {
    WseeHold h;
    h.mesh = p.mesh;
    h.own = p.own;
    h.gen = g_res.wsee_hold_gen;
    h.lvl = p.lvl;
    h.t = p.t;
    if (!p.own) {
        g_wsee.hold_copy_cycle = g_topo_cycles;   // WSEE_HOLD_IDLE_COPY: the buffer is in use this cycle.
        const UINT vs = static_cast<UINT>(sizeof(MeshVertex));
        h.base = p.base;
        h.off = p.h0;
        h.bytes = (g_wsee_run_plan[p.r1 - 1u].b - g_wsee_run_plan[p.r0].a) * vs;
        for (uint32_t j = p.r0; j < p.r1; ++j) {
            const WseeRun& r = g_wsee_run_plan[j];
            const D3D11_BOX b = { p.off + r.a * vs, 0u, 0u,
                                  p.off + r.b * vs, 1u, 1u };
            ctx->CopySubresourceRegion(g_res.wsee_hold_vb, 0, p.base + r.a * vs, 0, 0, p.src, 0,
                                       &b);
        }
        if (h.off + h.bytes > g_wsee.hold_used) g_wsee.hold_used = h.off + h.bytes;
    }
    g_wsee.hold.push_back(h);
    return static_cast<uint16_t>(g_wsee.hold.size());
}
// A held copy's entry (the flush): null when void - the hold released or remade since (g_res.wsee_hold_gen), another
// mesh's, or of another cycle (g_wsee.hold_cycle).
inline const WseeHold* wsee_hold_of(const RenderObject& o) {
    if (o.ws_hold == 0u || o.ws_hold > g_wsee.hold.size() || g_wsee.hold_cycle != g_topo_cycles) {
        return nullptr;
    }
    const WseeHold& h = g_wsee.hold[o.ws_hold - 1u];
    if (h.mesh != mesh_id_clamp(o.mesh) || h.mesh < 0 ||
        static_cast<size_t>(h.mesh) >= g_res.mesh_vb.size()) return nullptr;
    if (h.own) return &h;
    if (h.gen != g_res.wsee_hold_gen || !g_res.wsee_hold_vb || h.off > g_res.wsee_hold_cap ||
        g_res.wsee_hold_cap - h.off < h.bytes) return nullptr;
    return &h;
}
// The vertices a held copy draws from (mesh_vb_ref's stand-in for it): its mesh's own buffer, or the hold at the
// entry's base; null when the entry is void.
inline VbRef wsee_hold_ref(const RenderObject& o) {
    VbRef r;
    const WseeHold* const h = wsee_hold_of(o);
    if (!h) return r;
    if (h->own) {
        r.vb = g_res.mesh_vb[h->mesh];
        return r;
    }
    r.vb = g_res.wsee_hold_vb;
    r.off = h->base;
    return r;
}
// The level and crossfade a held copy draws at - the injection's pick, which its copy holds
// (level 0, no fade, for a void entry: the flush appends none).
inline void wsee_hold_lod(const RenderObject& o, int& lvl, float& t) {
    const WseeHold* const h = wsee_hold_of(o);
    lvl = h ? h->lvl : 0;
    t = h ? h->t : 0.0f;
}
// The hold and its run cache gone (wsee_cycle_note's idle rule) - each only where it was made (the hold's
// release also where only its refusal latch stands: WSEE_HOLD_FAIL).
inline bool wsee_hold_made() { return g_res.wsee_hold_vb != nullptr || !g_wsee_spans.empty(); }
inline void wsee_hold_drop() {
    if (g_res.wsee_hold_vb || g_res.wsee_hold_fail_cap != 0u) g_res.release_wsee_hold();
    if (!g_wsee_spans.empty()) g_wsee_spans.clear();
}
// One part's objects deferred together (a lone object, or a bucket's visible members), each at the injection's pick
// (lvl / t: its level and crossfade), each with an entry - its geometry held, or its mesh buffer named
// (own) - all or none. False: none deferred (a hold refused; the part draws at the injection, into T / NW).
inline bool wsee_defer_group(ID3D11Device* dev, ID3D11DeviceContext* ctx, const RenderObject* const* o,
                             const int* lvl, const float* t, size_t n) {
    static std::vector<WseeHoldPlan> p;
    p.resize(n);
    g_wsee_run_plan.clear();
    uint64_t refused = 0u, held = 0u;
    for (size_t i = 0; i < n; ++i) {
        if (!wsee_hold_plan(*o[i], lvl[i], t[i], p[i])) {
            ++refused;
            continue;
        }
        ++held;   // WSEE_OWN_STATIC: every planned object takes an entry (an own one no room).
    }
    if (refused == 0u && held > 0u) {
        wsee_hold_begin();
        // The layout, in take order: each copy's vmin at the first 256-aligned place past the last one's end and at
        // least 48 vmin bytes in (its base must not go below 0). A group of own entries alone lays out nothing and
        // makes no buffer (end stays g_wsee.hold_used).
        uint64_t end = g_wsee.hold_used;
        const uint64_t vs = sizeof(MeshVertex);
        for (WseeHoldPlan& q : p) {
            if (q.own) continue;
            const uint64_t lo = g_wsee_run_plan[q.r0].a * vs;
            const uint64_t sp = (g_wsee_run_plan[q.r1 - 1u].b - g_wsee_run_plan[q.r0].a) * vs;
            uint64_t h = end > lo ? end : lo;
            h = (h + (WSEE_HOLD_ALIGN - 1u)) & ~static_cast<uint64_t>(WSEE_HOLD_ALIGN - 1u);
            if (h + sp > WSEE_HOLD_CAP) {
                end = static_cast<uint64_t>(WSEE_HOLD_CAP) + 1u;
                break;
            }
            q.h0 = static_cast<UINT>(h);
            q.base = static_cast<UINT>(h - lo);
            end = h + sp;
        }
        if (g_wsee.hold.size() + held > WSEE_HOLD_MAX || end > WSEE_HOLD_CAP ||
            (end > g_wsee.hold_used && !wsee_hold_reserve(dev, ctx, static_cast<UINT>(end)))) {
            refused = held;
        }
    }
    if (refused > 0u) {
        // A refusal that built run caches stamps the hold's cycle, so the idle rule keeps them (wsee_cycle_note)
        // rather than dropping them at the next clear for a rebuild.
        if (!g_wsee_spans.empty()) {
            wsee_hold_begin();
            g_wsee.hold_copy_cycle = g_topo_cycles;   // WSEE_HOLD_IDLE_COPY.
        }
        return false;
    }
    for (size_t i = 0; i < n; ++i) {
        const uint16_t h = wsee_hold_take(ctx, p[i]);
        wsee_defer_note(*o[i]);
        g_wsee.defer_objs.back().ws_hold = h;
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
inline int wsee_sort_mode(ID3D11DeviceContext* ctx, float m22, float m32) {
    if (!wsee_tail_owed() || !g_res.depth_srv || !g_res.wsee_s_srv || !g_res.wsee_t_srv ||
        !g_res.wsee_b_srv || !g_res.wsee_n_srv || !g_res.wsee_n2_srv || !g_res.dss_off || !g_res.blend_modes[0] ||
        g_res.depth_sample_count < 2) return 0;   // The mesh unit declares T and the depth multisampled.
    // A partitioned cycle (wsee_pair_split) holds more than one encode in the main depth, which no one decode
    // reads: none.
    if (wsee_pair_split(m22, m32)) return 0;
    if (!g_res.bs_wsee_sort) return 1;
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return 0;
    D3D11_RENDER_TARGET_VIEW_DESC vd = {};
    rtv->GetDesc(&vd);
    rtv->Release();
    const DXGI_FORMAT f = vd.Format;
    // The float family by name; R32G32B32_FLOAT is never met - D3D11 gives the 96-bit formats no
    // multisampled render target, and the target here is drawn over the multisampled main depth (counts match).
    const bool sm_float = f == DXGI_FORMAT_R32G32B32A32_FLOAT || f == DXGI_FORMAT_R32G32B32_FLOAT ||
                          f == DXGI_FORMAT_R16G16B16A16_FLOAT || f == DXGI_FORMAT_R32G32_FLOAT ||
                          f == DXGI_FORMAT_R11G11B10_FLOAT || f == DXGI_FORMAT_R16G16_FLOAT ||
                          f == DXGI_FORMAT_R32_FLOAT || f == DXGI_FORMAT_R16_FLOAT;
    return sm_float ? 2 : 3;
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
// the next resolve into it), then sets back the targets (OmSave), blend and depth states. The flush is
// g_flush_active: the OM hooks stand aside. begin false = nothing changed (no target bound, or the bound depth is
// not the capture's): the caller draws as before.
struct WseeSortScope {
    OmSave                 om;
    ID3D11BlendState*        bs = nullptr;
    FLOAT                    bf[4] = {};
    UINT                     mask = 0xFFFFFFFFu;
    ID3D11DepthStencilState* dss = nullptr;
    UINT                     ref = 0;
    ID3D11DeviceContext*     ctx = nullptr;
    bool                     b2 = false;   // B2 bound at t60 (modes 2 / 3; b2m with it).
    // WSEE_B2M: this cycle's B2 moment stands (S2 taken with it: s2 too) - lane bit 3 in every mode, as the
    // chain's (KhWseeMeshCut's derived pre opaque then reads S2 behind, as KhWseeRule's khwr_b; mode 1 binds no B2 and
    // reads none).
    bool                     b2m = false;
    bool                     od = false;   // D3 bound at t65 (lane bit 5).
    bool                     pb = false;   // PSB: the one pass's record bound at t66 - t69 (lane bit 6).
    bool                     pe = false;   // PSB: the old record holds no particle-type layer (lane bit 7).
    bool                     np = false;   // WSEE_NOPRE: a pre late opaque skipped this cycle (lane bit 8).
    bool                     s2 = false;   // WSEE_NOPRE: S2 bound at t62, every mode (lane bit 9).
    WseeSortScope() = default;
    WseeSortScope(const WseeSortScope&) = delete;
    WseeSortScope& operator=(const WseeSortScope&) = delete;
    ~WseeSortScope() { end(); }
    bool begin(ID3D11DeviceContext* ss_ctx, int mode) {
        om.capture(ss_ctx);
        if (!om.rtv[0] || !om.dsv || reorder_dsv_identity(om.dsv) != g_res.wsee_depth_id) {
            om.release();   // Not the capture's depth (wsee_tail's rule): nothing changed.
            return false;
        }
        ctx = ss_ctx;
        ctx->OMGetBlendState(&bs, bf, &mask);
        ctx->OMGetDepthStencilState(&dss, &ref);
        ctx->OMSetRenderTargets(1, &om.rtv[0], g_res.depth_dsv_ro);
        const FLOAT z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(mode >= 2 ? g_res.bs_wsee_sort : g_res.blend_modes[0], z, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        b2m = g_res.wsee_b2_srv != nullptr && g_res.wsee_s2_srv != nullptr &&   // WSEE_B2M: the chain's b2.
              g_wsee.b2_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        b2 = mode >= 2 && b2m;
        ID3D11ShaderResourceView* const v[8] = { g_res.wsee_s_srv, g_res.wsee_t_srv, g_res.wsee_b_srv,
                                                 g_res.depth_srv, g_res.wsee_n_srv,
                                                 mode >= 2 ? g_res.wsee_q_srv : nullptr,
                                                 g_res.wsee_n2_srv,   // NWpost at t59.
                                                 b2 ? g_res.wsee_b2_srv : nullptr };   // B2 at t60.
        ctx->PSSetShaderResources(53, 8, v);
        // S2 (t62; cb.hlsl khMsS2): with B2, and (WSEE_NOPRE) in every mode wherever this cycle took it - the
        // derivation's reference for a skipped pre late opaque (bit 8).
        s2 = g_res.wsee_s2_srv != nullptr && g_wsee.s2_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        np = g_wsee.nopre_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        ID3D11ShaderResourceView* const ss_s2 = (b2 || s2) ? g_res.wsee_s2_srv : nullptr;
        ctx->PSSetShaderResources(62, 1, &ss_s2);
        // D3 (t65; cb.hlsl khMsD3): a late opaque's own depth, every mode (the occlusion test reads it).
        od = wsee_od_on() && wsee_d3_ok(g_topo_cycles);   // WSEE_OQ: complete only (its note).
        od = od && wsee_d3m_resolve(ctx, g_topo_cycles);   // WSEE_OQ2: the post opaques' D3m first.
        ID3D11ShaderResourceView* const d3 = od ? g_res.wsee_d3_srv : nullptr;
        ctx->PSSetShaderResources(65, 1, &d3);
        // PSB: P0, P1, N, N2 (t66 - t69; cb.hlsl khMsP0 / khMsNb) where a re-issue drew into them this cycle, and
        // whether the old record holds a particle-type layer (else its NW holds the clear: not read).
        pb = g_res.wsee_p0_srv && g_res.wsee_p1_srv && g_res.wsee_nb_srv && g_res.wsee_nb2_srv &&   // PSB_PHASE.
             g_psb.drew_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        // Bit 7 only while the one pass's record stands (else the old record is read whole).
        pe = g_psb.cycle.load(std::memory_order_relaxed) == g_topo_cycles &&
             g_psb.psa_cycle.load(std::memory_order_relaxed) != g_topo_cycles;
        ID3D11ShaderResourceView* const ss_pb[4] = { pb ? g_res.wsee_p0_srv : nullptr,
                                                     pb ? g_res.wsee_p1_srv : nullptr,
                                                     pb ? g_res.wsee_nb_srv : nullptr,
                                                     pb ? g_res.wsee_nb2_srv : nullptr };
        ctx->PSSetShaderResources(66, 4, ss_pb);
        return true;
    }
    void end() {
        if (!ctx) return;
        ID3D11ShaderResourceView* const n[8] = {};
        ctx->PSSetShaderResources(53, 8, n);
        ctx->PSSetShaderResources(62, 1, n);   // S2.
        ctx->PSSetShaderResources(65, 1, n);   // D3.
        ctx->PSSetShaderResources(66, 4, n);   // PSB: P0, P1, N, N2.
        om.restore(ctx);
        om.release();
        ctx->OMSetBlendState(bs, bf, mask);
        ctx->OMSetDepthStencilState(dss, ref);
        KH_SAFE_RELEASE(bs);
        KH_SAFE_RELEASE(dss);
        ctx = nullptr;
        b2 = false;
        b2m = false;   // WSEE_B2M.
        od = false;
        pb = false;   // PSB.
        pe = false;
        np = false;   // WSEE_NOPRE.
        s2 = false;
    }
};
}