#pragma once

// The injection half (rendering_integration_injection.hpp): the draw, state, resolve and clear hooks, the hook install
// and the flush frame template. Never include it on its own.
namespace RenderIntegration {

// The scene resolve: our resolve-time work, then the engine's resolve, passed through with its own arguments. Ours:
// the PSB_ZB guard's trip at a resolve into the main depth (whatever the source); the depth re-adoption; the
// slice's deferred tail where it is armed (INFRONT); the late chain where it is owed (FX_LATE); and on the render
// thread, once a cycle, the dynamic-light world pass and the flush. The format argument is unused: the passes key on
// the source identity (PSB_ZB on the destination's).
static void STDMETHODCALLTYPE hooked_resolvesubresource(ID3D11DeviceContext* self, ID3D11Resource* dst, UINT dst_sub, ID3D11Resource* src, UINT src_sub, DXGI_FORMAT fmt) {
    try {
    KH_PROF_SCOPE(PROF_HOOK_RESOLVE);   // PROF_HOOKS: the DLS world pass and the flush included.
    psb_zb_dst(self, dst);   // PSB_ZB: a resolve into the main depth ends Zb.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed)) {
        if (src && reorder_on_render_thread() && !g_ro.in_injection &&
            !g_flush_active.load(std::memory_order_relaxed)) {
            main_depth_readopt(self, src);   // Before anything below keys on the identities.
        }
        if (src && g_topo_scene_tex_id && static_cast<void*>(src) == g_topo_scene_tex_id) {
            // INFRONT: the slice's deferred tail, armed at the hands colour pass - this resolve is past it (the
            // cycle's first precedes the pass; the late chain's follows it) - drawn ahead of the chain and the
            // flush, so the resolved picture and the chain's hold the parts. Either thread (fx_late_gt's rule).
            if (g_infront.tail_armed.load(std::memory_order_relaxed) && !g_ro.in_injection &&
                !g_flush_active.load(std::memory_order_relaxed)) {
                infront_tail_take(self);
            }
            if (!reorder_on_render_thread() && !g_flush_active.load(std::memory_order_relaxed)) {
                fx_late_gt(self);   // FX_LATE: tests every frame; runs an owed chain.
            } else if (reorder_on_render_thread() && !g_ro.in_injection &&
                       !g_flush_active.load(std::memory_order_relaxed) &&
                       g_rtshadow_cycle.load(std::memory_order_relaxed) == g_topo_cycles) {
                fx_late_gt(self, true);   // FX_LATE: a render-thread resolve after this cycle's flush.
            }
            // The dynamic-light world pass, on the render thread once a cycle (a second resolve would multiply twice
            // and darken the frame): the cycle's first resolve, ahead of the view model, so the world multiply does
            // not reach the hands and weapon - and ahead of the flush below, so the flush's late draws and chain take
            // this cycle's pass. EA_DLS_ONE: one latch site (the flush branch carried a second copy).
            if (reorder_on_render_thread() && !g_ro.in_injection && g_dls_world_cycle != g_topo_cycles) {
                g_dls_world_cycle = g_topo_cycles;
                dls_world_pass(self);
            }
            // The flush, on the render thread, once a cycle and never inside our injection - and only while the game
            // thread has work, the park's own condition (flush_frame returns before it on an empty draw list), so an
            // empty list runs no housekeeping or GC.
            if (reorder_on_render_thread() && !g_ro.in_injection &&
                g_track_wanted.load(std::memory_order_relaxed) &&
                g_rtshadow_cycle.load(std::memory_order_relaxed) != g_topo_cycles) {
                g_rtshadow_cycle.store(g_topo_cycles, std::memory_order_relaxed);
                ID3D11Device* dev = RVExtBridge::get_d3d_device();
                if (dev) {
                    // in_injection is raised as the world pass raises it, so the flush's draws are not counted as
                    // engine draws.
                    const bool prev = g_ro.in_injection;
                    g_ro.in_injection = true;
                    flush_locked(dev, self, true);
                    g_ro.in_injection = prev;
                }
            }
        }
    }

    } catch (...) { hook_except(); }
    g_orig_resolvesubresource(self, dst, dst_sub, src, src_sub, fmt);
}

static void STDMETHODCALLTYPE hooked_pssetshaderresources(ID3D11DeviceContext* self, UINT start, UINT n, ID3D11ShaderResourceView* const* srvs) {
    try {
    KH_PROF_SCOPE(PROF_HOOK_PSSRV);   // PROF_HOOKS.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&   // Context first.
        reorder_on_render_thread() &&
        !g_ro.in_injection && g_ls.atlas_tex && srvs) {
        // The engine binds the shadow atlas at t15 (from its own resolve shader) - inspect only that slot.
        // EA_ATLAS_T15: the view's resource is read on every such bind (a view pointer alone is no identity: a
        // released view's address can be reused), so the verdict is that one compare - no view cache in front of it.
        if (start <= 15 && start + n > 15) {
            ID3D11ShaderResourceView* srv = srvs[15 - start];

            if (srv) {
                ID3D11Resource* res = nullptr;
                srv->GetResource(&res);

                if (res) {
                    if (res == static_cast<ID3D11Resource*>(g_ls.atlas_tex)) g_mask.atlas_bound = true;
                    res->Release();
                }
            }
        }
    }

    } catch (...) { hook_except(); }
    g_orig_pssetshaderresources(self, start, n, srvs);
}

static void STDMETHODCALLTYPE hooked_draw_indexed(ID3D11DeviceContext* self, UINT ic, UINT sil, INT bvl) {
    try { reorder_pre_draw(self); } catch (...) { hook_except(); }
    try { psb_zb_draw(self); } catch (...) { hook_except(); }   // PSB_ZB: the guard's one call a draw.
    {
        KH_PROF_SCOPE(PROF_DRAW_RECORD);   // PROF_HOOKS.
        rp_record(self, 1, ic, sil, 0, bvl, 0);   // VOL_REPLAY: engine state after our pre-draw work.
    }
    g_orig_draw_indexed(self, ic, sil, bvl);
    try {
    KH_PROF_TSC(PROF_DRAW_POST);   // PROF_HOOKS: mirror re-issue (with its driver draw), UI quad, bracket.
    // VM_SEE: the view model's translucent draw again, into the transmittance alone (the flag is set for the
    // tracked context's own draw by its pre-draw; the context test first, as the mirror's). Not one of our own draws
    // (in_injection / the flush flag - the pre-draw's own gate): the injection reorder_pre_draw may run after the
    // flag was set draws through these hooks first, and the flag is the engine draw's, whose post comes after them.
    // Ahead of the mirror's block, which may throw: nothing here does, so the flag never outlives its draw.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && g_vmsee.pending && !g_ro.in_injection &&
        !g_flush_active.load(std::memory_order_relaxed)) {
        g_vmsee.pending = false;
        if (vmsee_t_begin(self)) {
            g_orig_draw_indexed(self, ic, sil, bvl);
            vmsee_t_end(self);
        }
    }
    // WSEE: the engine's draw again, into T (and NW, for a blended draw writing no depth) or T's late-opaque flag -
    // the kind wsee_pre_draw marked (wsee_post; the VM_SEE block's rules: the context first, never one of
    // our own draws, nothing here throws).
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && g_wsee.pending != WSEE_RE_NONE &&
        !g_ro.in_injection && !g_flush_active.load(std::memory_order_relaxed)) {
        const uint8_t k = g_wsee.pending;
        g_wsee.pending = WSEE_RE_NONE;
        KH_PROF_TSC(PROF_WSEE_POST);   // WSEE: the re-issues' CPU alone (nested in drawPost).
        wsee_post(self, k, [&]() { g_orig_draw_indexed(self, ic, sil, bvl); });
    }
    // Mirror re-issue; the flag is set for the tracked context's own draw. The context test first: g_vmir.pending is a
    // plain render-thread bool, and this hook runs on every thread that draws through a shared vtable. Not one of our
    // own draws either (VMIR_OWN_DRAW, the vmsee block's rule): the pre-draw that armed the flag can still issue
    // draws of its own after the survey (the in-front seam, a mask-cast fire), whose post would take the flag for the
    // engine's counting draw and re-issue ours in its place (vmir_patch_b2 holds the flag down around its own draw
    // for the same reason). The engine's draw runs with the flag down, so it is unaffected.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        g_vmir.pending && !g_ro.in_injection) {
        g_vmir.pending = false;
        if (vmir_begin(self)) {
            KH_GPU_SCOPE(self, GPU_VMIR);   // PROF: the mirror's re-issued draw and the restore.
            g_orig_draw_indexed(self, ic, sil, bvl);
            vmir_end(self);
        }
    }
    ui_quad_post(self);
    svs_bracket_post(self);
    } catch (...) { hook_except(); }
}

static void STDMETHODCALLTYPE hooked_draw(ID3D11DeviceContext* self, UINT vc, UINT svl) {
    try { reorder_pre_draw(self); } catch (...) { hook_except(); }
    try { psb_zb_draw(self); } catch (...) { hook_except(); }   // PSB_ZB: the guard's one call a draw.
    {
        KH_PROF_SCOPE(PROF_DRAW_RECORD);   // PROF_HOOKS.
        rp_record(self, 2, vc, svl, 0, 0, 0);   // VOL_REPLAY: engine state after our pre-draw work.
    }
    g_orig_draw(self, vc, svl);
    try {
    KH_PROF_TSC(PROF_DRAW_POST);   // PROF_HOOKS: mirror re-issue (with its driver draw), UI quad, bracket.
    // VM_SEE: hooked_draw_indexed's block and rules.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && g_vmsee.pending && !g_ro.in_injection &&
        !g_flush_active.load(std::memory_order_relaxed)) {
        g_vmsee.pending = false;
        if (vmsee_t_begin(self)) {
            g_orig_draw(self, vc, svl);
            vmsee_t_end(self);
        }
    }
    // WSEE: hooked_draw_indexed's block and rules (wsee_post).
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && g_wsee.pending != WSEE_RE_NONE &&
        !g_ro.in_injection && !g_flush_active.load(std::memory_order_relaxed)) {
        const uint8_t k = g_wsee.pending;
        g_wsee.pending = WSEE_RE_NONE;
        KH_PROF_TSC(PROF_WSEE_POST);   // WSEE: the re-issues' CPU alone (nested in drawPost).
        wsee_post(self, k, [&]() { g_orig_draw(self, vc, svl); });
    }
    // Mirror re-issue (VMIR_OWN_DRAW): hooked_draw_indexed's block and rules.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        g_vmir.pending && !g_ro.in_injection) {
        g_vmir.pending = false;
        if (vmir_begin(self)) {
            KH_GPU_SCOPE(self, GPU_VMIR);   // PROF: the mirror's re-issued draw and the restore.
            g_orig_draw(self, vc, svl);
            vmir_end(self);
        }
    }
    ui_quad_post(self);
    svs_bracket_post(self);
    } catch (...) { hook_except(); }
}

static void STDMETHODCALLTYPE hooked_draw_indexed_instanced(ID3D11DeviceContext* self, UINT icpi, UINT ic, UINT sil, INT bvl, UINT sil2) {
    try { reorder_pre_draw(self); } catch (...) { hook_except(); }
    try { psb_zb_draw(self); } catch (...) { hook_except(); }   // PSB_ZB: the guard's one call a draw.
    {
        KH_PROF_SCOPE(PROF_DRAW_RECORD);   // PROF_HOOKS.
        rp_record(self, 3, icpi, ic, sil, bvl, sil2);   // VOL_REPLAY: engine state after our pre-draw work.
    }
    g_orig_draw_indexed_instanced(self, icpi, ic, sil, bvl, sil2);
    try {
    KH_PROF_TSC(PROF_DRAW_POST);   // PROF_HOOKS: mirror re-issue (with its driver draw), UI quad, bracket.
    // VM_SEE: hooked_draw_indexed's block and rules.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && g_vmsee.pending && !g_ro.in_injection &&
        !g_flush_active.load(std::memory_order_relaxed)) {
        g_vmsee.pending = false;
        if (vmsee_t_begin(self)) {
            g_orig_draw_indexed_instanced(self, icpi, ic, sil, bvl, sil2);
            vmsee_t_end(self);
        }
    }
    // WSEE: hooked_draw_indexed's block and rules (wsee_post).
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && g_wsee.pending != WSEE_RE_NONE &&
        !g_ro.in_injection && !g_flush_active.load(std::memory_order_relaxed)) {
        const uint8_t k = g_wsee.pending;
        g_wsee.pending = WSEE_RE_NONE;
        KH_PROF_TSC(PROF_WSEE_POST);   // WSEE: the re-issues' CPU alone (nested in drawPost).
        wsee_post(self, k, [&]() { g_orig_draw_indexed_instanced(self, icpi, ic, sil, bvl, sil2); });
    }
    // Mirror re-issue (VMIR_OWN_DRAW): hooked_draw_indexed's block and rules.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        g_vmir.pending && !g_ro.in_injection) {
        g_vmir.pending = false;
        if (vmir_begin(self)) {
            KH_GPU_SCOPE(self, GPU_VMIR);   // PROF: the mirror's re-issued draw and the restore.
            g_orig_draw_indexed_instanced(self, icpi, ic, sil, bvl, sil2);
            vmir_end(self);
        }
    }
    ui_quad_post(self);
    svs_bracket_post(self);
    } catch (...) { hook_except(); }
}

static void STDMETHODCALLTYPE hooked_draw_instanced(ID3D11DeviceContext* self, UINT vcpi, UINT ic, UINT svl, UINT sil) {
    try { reorder_pre_draw(self); } catch (...) { hook_except(); }
    try { psb_zb_draw(self); } catch (...) { hook_except(); }   // PSB_ZB: the guard's one call a draw.
    {
        KH_PROF_SCOPE(PROF_DRAW_RECORD);   // PROF_HOOKS.
        rp_record(self, 4, vcpi, ic, svl, 0, sil);   // VOL_REPLAY: engine state after our pre-draw work.
    }
    g_orig_draw_instanced(self, vcpi, ic, svl, sil);
    try {
    KH_PROF_TSC(PROF_DRAW_POST);   // PROF_HOOKS: mirror re-issue (with its driver draw), UI quad, bracket.
    // VM_SEE: hooked_draw_indexed's block and rules.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && g_vmsee.pending && !g_ro.in_injection &&
        !g_flush_active.load(std::memory_order_relaxed)) {
        g_vmsee.pending = false;
        if (vmsee_t_begin(self)) {
            g_orig_draw_instanced(self, vcpi, ic, svl, sil);
            vmsee_t_end(self);
        }
    }
    // WSEE: hooked_draw_indexed's block and rules (wsee_post).
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && g_wsee.pending != WSEE_RE_NONE &&
        !g_ro.in_injection && !g_flush_active.load(std::memory_order_relaxed)) {
        const uint8_t k = g_wsee.pending;
        g_wsee.pending = WSEE_RE_NONE;
        KH_PROF_TSC(PROF_WSEE_POST);   // WSEE: the re-issues' CPU alone (nested in drawPost).
        wsee_post(self, k, [&]() { g_orig_draw_instanced(self, vcpi, ic, svl, sil); });
    }
    // Mirror re-issue (VMIR_OWN_DRAW): hooked_draw_indexed's block and rules.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        g_vmir.pending && !g_ro.in_injection) {
        g_vmir.pending = false;
        if (vmir_begin(self)) {
            KH_GPU_SCOPE(self, GPU_VMIR);   // PROF: the mirror's re-issued draw and the restore.
            g_orig_draw_instanced(self, vcpi, ic, svl, sil);
            vmir_end(self);
        }
    }
    ui_quad_post(self);
    svs_bracket_post(self);
    } catch (...) { hook_except(); }
}

// HOOK_DESC_CACHE: the two OM state hooks read one bit of an immutable state object's desc on every engine bind (the
// depth state's hook, WSEE, a second: verdict2; and verdict3, the stencil test). The verdict is cached by object;
// the object is AddRef'd while cached, so its address cannot be recycled under the key (a foreign object at the same
// address is the one failure a pointer cache has). Render thread only; released with the device (the hooks' other weak
// identities go in release_shadow_device_state).
static constexpr uint32_t HDC_N = 32;
struct HookDescEntry { IUnknown* obj; bool verdict; bool verdict2; bool verdict3; };
struct HookDescCache {
    HookDescEntry e[HDC_N] = {};
    uint32_t n = 0;
    uint32_t next = 0;   // Round-robin victim once full.
    bool find(IUnknown* o, bool& v, bool* v2 = nullptr, bool* v3 = nullptr) const {
        for (uint32_t i = 0; i < n; ++i) {
            if (e[i].obj == o) {
                v = e[i].verdict;
                if (v2) *v2 = e[i].verdict2;
                if (v3) *v3 = e[i].verdict3;
                return true;
            }
        }
        return false;
    }
    void put(IUnknown* o, bool v, bool v2 = false, bool v3 = false) {
        uint32_t i;
        if (n < HDC_N) { i = n++; }
        else { i = next; next = (next + 1) % HDC_N; if (e[i].obj) e[i].obj->Release(); }
        o->AddRef();
        e[i].obj = o;
        e[i].verdict = v;
        e[i].verdict2 = v2;
        e[i].verdict3 = v3;
    }
    void release() {
        for (uint32_t i = 0; i < n; ++i) { if (e[i].obj) e[i].obj->Release(); e[i].obj = nullptr; }
        n = 0;
        next = 0;
    }
};
static HookDescCache g_hdc_blend;
static HookDescCache g_hdc_dss;
inline void hook_desc_cache_release() {
    g_hdc_blend.release();
    g_hdc_dss.release();
}

static void STDMETHODCALLTYPE hooked_omset_blendstate(ID3D11DeviceContext* self, ID3D11BlendState* bs, const FLOAT bf[4], UINT mask) {
    try {
    KH_PROF_SCOPE(PROF_HOOK_OMBLEND);   // PROF_HOOKS.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        !g_flush_active && !g_ui_mask_injecting &&   // own-state exclusion by flags.
        reorder_on_render_thread()) {
        if (!bs) {
            g_ro.blend_translucent = false;   // Default blend state: opaque.
        } else if (!g_hdc_blend.find(bs, g_ro.blend_translucent)) {   // HOOK_DESC_CACHE.
            D3D11_BLEND_DESC d = {};
            bs->GetDesc(&d);
            // Translucent = blending enabled with a destination term: covers
            // alpha (INV_SRC_ALPHA), additive (one) and the rest.
            g_ro.blend_translucent = d.RenderTarget[0].BlendEnable &&
                                     d.RenderTarget[0].DestBlend != D3D11_BLEND_ZERO;
            g_hdc_blend.put(bs, g_ro.blend_translucent);
        }
    }

    } catch (...) { hook_except(); }
    g_orig_omset_blendstate(self, bs, bf, mask);
}

static void STDMETHODCALLTYPE hooked_omset_depthstencil(ID3D11DeviceContext* self, ID3D11DepthStencilState* dss, UINT ref) {
    try {
    KH_PROF_SCOPE(PROF_HOOK_OMDSS);   // PROF_HOOKS.
    psb_zb_dss(self, dss);   // PSB_ZB: every caller's (the draws' guard reads it).
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        !g_flush_active && !g_ui_mask_injecting &&   // own-state exclusion by flags.
        reorder_on_render_thread()) {
        if (!dss) {
            g_ro.dss_nowrite = false;   // Default dss: depth test + write.
            g_ro.dss_write = true;
            g_ro.dss_stencil = false;   // WSEE: the default tests no stencil.
        } else if (!g_hdc_dss.find(dss, g_ro.dss_nowrite, &g_ro.dss_write,
                                   &g_ro.dss_stencil)) {   // HOOK_DESC_CACHE.
            D3D11_DEPTH_STENCIL_DESC d = {};
            dss->GetDesc(&d);
            g_ro.dss_nowrite = d.DepthEnable && d.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ZERO;
            g_ro.dss_write = d.DepthEnable && d.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL;   // WSEE.
            g_ro.dss_stencil = d.StencilEnable != FALSE;   // WSEE: a no-write layer testing stencil is left out.
            g_hdc_dss.put(dss, g_ro.dss_nowrite, g_ro.dss_write, g_ro.dss_stencil);
        }
    }

    } catch (...) { hook_except(); }
    g_orig_omset_depthstencil(self, dss, ref);
}

static void STDMETHODCALLTYPE hooked_omset_rendertargets(ID3D11DeviceContext* self, UINT n, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv) {
    try {
    KH_PROF_SCOPE(PROF_HOOK_OMRT);   // PROF_HOOKS.
    // Foreign contexts fall straight through.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        !g_flush_active && !g_ui_mask_injecting &&   // own-state exclusion by flags.
        reorder_on_render_thread()) {
        g_ro.dsv_main = dsv && g_main_depth_identity &&
                        reorder_dsv_identity(dsv) == g_main_depth_identity;
        // INFRONT: the bind that leaves the hands colour pass's targets (RT0 or the depth view changes; the
        // pointers compared, pip_track_targets' leave rule) draws the slice's deferred tail into them first -
        // they are still bound, this hook runs ahead of the engine's call. TWIN: hooked_omset_rts_and_uavs.
        if (g_infront.tail_armed.load(std::memory_order_relaxed) &&
            (dsv != g_infront.tail_dsv || n < 1 || !rtvs || rtvs[0] != g_infront.tail_rtv)) {
            infront_tail_take(self);
        }
        pip_track_targets(self, n, rtvs, dsv);   // PIP (+ PIP_FX at a leave).
        const bool was_on_atlas = g_ls.phase_on_atlas;
        shadow_track_targets(self, dsv, g_ro.dsv_main, n, rtvs);

        if (!was_on_atlas && g_ls.phase_on_atlas && g_ls.resolve_seen_since_cast) {
            // Clearing it here keeps the atlas-exit-to-boundary window of the next cycle from re-firing the previous
            // cycle's frozen inputs into the new cycle's mask.
            g_ls.resolve_seen_since_cast = false;
        }

        if (was_on_atlas && !g_ls.phase_on_atlas) {
            if (g_ro.injected) g_fire.post_scene_cycle = g_topo_cycles;   // FIRE_NEXT_MASK.
            g_ls.resolve_window = 4;
        }

        mask_classify_rt(n, rtvs);

    }

    } catch (...) { hook_except(); }
    g_orig_omset_rendertargets(self, n, rtvs, dsv);
}

static void STDMETHODCALLTYPE hooked_omset_rts_and_uavs(ID3D11DeviceContext* self, UINT n, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv, UINT uav_start, UINT n_uavs, ID3D11UnorderedAccessView* const* uavs, const UINT* counts) {
    try {
    KH_PROF_SCOPE(PROF_HOOK_OMRTUAV);   // PROF_HOOKS.
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        !g_flush_active && !g_ui_mask_injecting &&   // own-state exclusion by flags.
        reorder_on_render_thread() &&
        n != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL) {
        // The DSV changes through this path too (n == keep leaves the current bindings alone).
        g_ro.dsv_main = dsv && g_main_depth_identity &&
                        reorder_dsv_identity(dsv) == g_main_depth_identity;
        // INFRONT: the leave of the hands colour pass's targets (TWIN: hooked_omset_rendertargets).
        if (g_infront.tail_armed.load(std::memory_order_relaxed) &&
            (dsv != g_infront.tail_dsv || n < 1 || !rtvs || rtvs[0] != g_infront.tail_rtv)) {
            infront_tail_take(self);
        }
        pip_track_targets(self, n, rtvs, dsv);   // PIP (+ PIP_FX at a leave).
        shadow_track_targets(self, dsv, g_ro.dsv_main, n, rtvs);

    }
    // RP_WRITE_SPLIT: the draws after this bind can write a UAV's buffer, and a buffer bound as a UAV is no draw's
    // vertex / index buffer meanwhile (D3D11 nulls an input bound while it is an output) - so a pixel-stage UAV write
    // into a buffer a record read follows a bind here, which splits ahead of it. One GetResource per UAV, only past
    // the gate (an open pass, a buffer listed); a call the runtime drops (past the UAV slots) writes nothing.
    if (n_uavs != D3D11_KEEP_UNORDERED_ACCESS_VIEWS && uavs && uav_start < D3D11_1_UAV_SLOT_COUNT &&
        n_uavs <= D3D11_1_UAV_SLOT_COUNT - uav_start && rp_split_armed(self) && g_rp_rec.dynset.n != 0u) {
        for (UINT i = 0; i < n_uavs; ++i) {
            if (!uavs[i]) continue;
            ID3D11Resource* r = nullptr;
            uavs[i]->GetResource(&r);
            if (!r) continue;
            rp_on_write(self, r, false);
            r->Release();
        }
    }

    } catch (...) { hook_except(); }
    g_orig_omset_rts_and_uavs(self, n, rtvs, dsv, uav_start, n_uavs, uavs, counts);
}

static void STDMETHODCALLTYPE hooked_clear_depthstencil(ID3D11DeviceContext* self, ID3D11DepthStencilView* dsv, UINT flags, FLOAT depth, UINT8 stencil) {
    try {   // VOL_REPLAY - under a try of its own (every other hook's rp_foreign call is under a try too).
        if (dsv && g_svs.vol_src_id && self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
            reorder_dsv_identity(dsv) == g_svs.vol_src_id) {
            rp_foreign(0);   // Inside a pass: refused. Before one: this frame's buffer starts clean.
            g_rp_rec.vol_clear_seq = g_svs.frame_seq;
        }
    } catch (...) { hook_except(); }
    KH_PROF_SCOPE(PROF_HOOK_CLEAR);   // PROF.
    try {
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        // Own-state exclusion: the flush's sun / DLS clears are ours.
        !g_flush_active.load(std::memory_order_relaxed) &&
        dsv && (flags & D3D11_CLEAR_DEPTH)) {
        g_depth_clear_serial++;   // SVS_SKIP: any depth clear invalidates a cached seam inject.
        g_watch.clears.fetch_add(1u, std::memory_order_relaxed);   // WATCH_STALL.
        // The identity is a pointer the engine may recycle (a render-resolution change recreates the scene targets with
        // no device reset): the match is confirmed against the adopted dimensions once per frame here, and a mismatch
        // drops it so the next park re-adopts in either direction at once instead of after the wrong-pass streak.
        bool main = g_main_depth_identity && reorder_dsv_identity(dsv) == g_main_depth_identity;
        if (main && !main_depth_dims_match(dsv)) {
            g_main_depth_identity = nullptr;
            main = false;
        }
        // FSAA_SWITCH: a single-sample identity (FSAA 1x) while a multisampled depth of its size is cleared - FSAA
        // was turned on at runtime. The identity may still match the game thread's own clear, which keeps naming the
        // game thread (it never resolves: no DEPTH_READOPT), and only the park's wrong-pass streak would re-adopt
        // (WRONG_PASS_READOPT parks): back to the bootstrap instead, and the next park adopts at once (the MSAA
        // mission start's path). Inert while a multisampled depth is adopted (fsaa_world_standdown is false then).
        if (!main && g_main_depth_identity && fsaa_world_standdown() && depth_msaa_scene(dsv)) {
            g_main_depth_identity = nullptr;
            g_reorder_render_tid.store(0, std::memory_order_relaxed);
        }
        if (!main) pip_on_clear();   // PIP.
        if (main) {
            // The thread that clears the main scene depth is named the render thread for the tracking gate: at MSAA
            // the engine's render thread; at FSAA 1x the game thread (measured), so the gate keeps the real render
            // thread's mesh routes off there (the "By design" note at fsaa_world_standdown).
            g_reorder_render_tid.store(GetCurrentThreadId(), std::memory_order_relaxed);
            sun_size_latch();

            g_boundary_t = effect_time_seconds();
            volume_seam_frame_reset();   // One injection per frame.
            rp_img_clear();   // REPLAY_CPUCB: images are one frame's (the volume pass is at its end).
            infront_frame_reset();   // INFRONT: the slice's two injections re-arm.

            // The no-flush sentinel, re-armed per frame: rescue_window_open stays shut until this frame's flush
            // stamps its opaque count (only the sentinel is tested; the count is compared nowhere).
            g_cc_flush_opaques.store(0xFFFFFFFFu, std::memory_order_relaxed);

            g_topo_cycles++;
            g_topo_cycles_pub.store(g_topo_cycles, std::memory_order_relaxed);   // ATTACH_CYCLE: the park's step reads it.
            if constexpr (WSEE_ON) {   // No per-clear see-through work with the system built out.
                KH_PROF_TSC(PROF_WSEE_CLEAR);   // wseeClear, the three calls timed whole.
                wsee_cycle_note();   // WSEE: whether this cycle captures.
                psv_cycle_head(self);   // PSV: the queued variants made (device Create* alone).
                wsee_gput_head(self, g_topo_cycles, g_wsee.on);   // WSEE_GPUT: its frame closed, read, opened.
            }
            mat_anim_stamp(g_topo_cycles);   // UV_ANIM: the cycle's time, while a clock-driven entry exists.
            gts_clear_note(gts_qpc());   // ATTACH_GT_SNAP: this cycle's boundary and half the frame period.
            prof_fold(self);   // PROF: the ended cycle publishes; the next GPU bracket opens.

            // A main scene depth clear marks the new frame: injection re-arms and the phase evidence resets.
            blk_owner_frame();   // LIGHT_BASE: the ended frame's base lighting publishes.
            sun_raw_frame();   // SUN_RAW: the ended frame's sun direction publishes.
            g_ro.injected = false;
            // Census: fold the ended cycle, open the next.
            g_ro.frame_cycles++;
            g_ro.opaque_draws = 0;
            g_ro.inject_attempts = 0;
            g_rescue_frame_count = 0;   // Rescue re-arm budget renews per frame.
            g_atlas_frame_prev = g_atlas_frame_cur;   // Atlas liveness rolls per frame.
            g_atlas_frame_cur = false;
            g_ro.cycle_vp_valid = false;
            g_ro.trig_vp_valid = false;
            g_ro.engine_proj_valid = false;
            g_ro.slot_near_live = -1.0f;
            g_ro.world_pair_valid = false;   // world-phase pair latch is per-cycle.

            if (g_sun_jump_pending) {
                // A raw publish stepped past SRAW_JUMP_DEG. The band seals are NOT wiped (each re-pairs at its
                // next capture).
                g_sun_jump_pending = false;

                // Do NOT reset sweep_settle here: it counts coherent sweeps of the ENGINE'S mask stream, which a sun
                // change does not break, and the reset cost a frame of world cast (flicker). Do NOT clear map validity:
                // the mask cast fire builds this frame's map itself ahead of its freshness gate (mask_cast_engine's
                // once-per-frame render_sun_depth), and it has no analytic fallback to take (the AABB slab path is
                // gone: EA_SLAB_DEAD). The union's hash already carries the sun; zeroing it only makes the re-fit
                // unconditional.
                g_sun_maps.sun_map_hash = 0;
            }

            shadow_live_frame_reset();
            shadow_view_prewarm();   // Lock can exist before the first mesh does.
            g_sun_maps.sun_map_rendered_frame = false;
            if (g_fire.lock) { g_fire.lock->Release(); g_fire.lock = nullptr; }
            g_fire.lock_valid = false;   // New frame: first fire re-freezes.
            if (g_ls.resolve_seen_since_cast) {
                g_ls.resolve_seen_since_cast = false;
            }

            // Learn unconditionally from the frame's own final count.
            if (g_fire.count_frame > 0) {
                g_fire.clamp_target = g_fire.count_frame;
            }

            g_fire.clamp_painted = 0;
            g_fire.count_frame = 0;   // Fire census rolls per cycle (the clamp's last-fire pick).
            g_mask_cast_fired = false;   // New frame: one analytic pass allowed.
            // Do not arm at the boundary: this frame's gated capture has not run yet, so the fire would use the
            // previous frame's depth snapshot - on partitioned look-down frames near the mesh that stale close-range
            // depth MIN-darkened the screen.
            g_mask_cast_arm = false;
            g_ls.view_published_this_frame = false;

            RVExtBridge::ProjectionViewTransform pv = {};

            if (RVExtBridge::get_projection_view_transform(pv)) {
                g_boundary_pv_valid = true;
                // Adopt the bridge PV at the clear (see LATCH_JUMP_M).
                float cam[3];
                cam_of(pv.view, cam);
                g_cam_step_m = g_latch_cam_valid
                             ? sqrtf(latch_cam_dist_sq(cam, g_latch_cam)) : 0.0f;
                g_ro.cycle_pv = pv;
                g_ro.cycle_pv_valid = true;
                g_ro.cycle_pv_stale = false;
                g_latch_cam[0] = cam[0];
                g_latch_cam[1] = cam[1];
                g_latch_cam[2] = cam[2];
                g_latch_cam_valid = true;
                simlod_publish(cam, pv.projection[1][1], g_boundary_t);   // SIM_LOD: the game thread's view.

            } else {
                g_ro.cycle_pv_stale = true;
                g_boundary_pv_valid = false;   // No clear-time sample this cycle.
            }

        }
    }

    } catch (...) { hook_except(); }
    // PSB_ZB: after the cycle step (whatever it threw) - a clear of the main depth that starts no cycle ends Zb.
    try { psb_zb_clear(self, dsv, flags); } catch (...) { hook_except(); }
    g_orig_clear_depthstencil(self, dsv, flags, depth, stencil);
}

inline void reorder_hook_fail_round(const char* what) {
    ++g_reorder_hook.fail_count;

    if (g_reorder_hook.fail_count == 1) {
        g_reorder_hook.retry_ms = steady_now_ms() + 1000;
        return;
    }

    if (g_reorder_hook.fail_count == 2) {
        g_reorder_hook.retry_ms = steady_now_ms() + 10000;
        return;
    }

    g_reorder_hook.failed = true;
    report_error(std::string("KH RenderIntegration: context hooks: ") + what + " (phase "
                 + std::to_string(g_reorder_hook.fail_phase) + ", spec "
                 + std::to_string(g_reorder_hook.fail_slot) + ", MH status "
                 + std::to_string(g_reorder_hook.mh_status) + ", attempt "
                 + std::to_string(g_reorder_hook.fail_count)
                 + "); composited rendering disabled");
}

inline void ensure_reorder_hook() {
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    if (ctx) g_reorder_target_ctx.store(static_cast<void*>(ctx), std::memory_order_relaxed);
    if (ctx && g_reorder_hook.active.load(std::memory_order_acquire) &&
        g_reorder_hook.vt && *reinterpret_cast<void***>(ctx) != g_reorder_hook.vt) {
        if (g_reorder_hook.vt_drift++ == 0) {
            // EA_ERR_CLASS: another overlay re-hooked the context - a warning (game thread: a plain log line).
            sqf::diag_log("KH RenderIntegration: context hooks: context vtable changed under installed hooks; "
                          "composited draws miss the hooks if it points at other code "
                          "(reported once per process)");
        }
    }
    if (g_reorder_hook.active.load(std::memory_order_acquire) || g_reorder_hook.failed) return;
    if (!ctx) return;   // Bridge not ready: retry next flush.
    if (g_reorder_hook.retry_ms != 0 &&
        steady_now_ms() < g_reorder_hook.retry_ms) return;

    if (!ensure_minhook()) {
        g_reorder_hook.fail_phase = 1;
        g_reorder_hook.fail_slot = -1;
        g_reorder_hook.mh_status = -1;   // Framework ensure_minhook is bool-only.
        reorder_hook_fail_round("MinHook init failed");
        return;
    }

    void** vt = *reinterpret_cast<void***>(ctx);
    struct HookSpec { int slot; void* detour; void** original; };

    const HookSpec specs[] = {
        { VT_PSSETSHADERRESOURCES, reinterpret_cast<void*>(&hooked_pssetshaderresources),  reinterpret_cast<void**>(&g_orig_pssetshaderresources) },
        { VT_DRAWINDEXED,          reinterpret_cast<void*>(&hooked_draw_indexed),           reinterpret_cast<void**>(&g_orig_draw_indexed) },
        { VT_DRAW,                 reinterpret_cast<void*>(&hooked_draw),                   reinterpret_cast<void**>(&g_orig_draw) },
        { VT_DRAWINDEXEDINSTANCED, reinterpret_cast<void*>(&hooked_draw_indexed_instanced), reinterpret_cast<void**>(&g_orig_draw_indexed_instanced) },
        { VT_DRAWINSTANCED,        reinterpret_cast<void*>(&hooked_draw_instanced),         reinterpret_cast<void**>(&g_orig_draw_instanced) },
        { VT_OMSETRENDERTARGETS,   reinterpret_cast<void*>(&hooked_omset_rendertargets),    reinterpret_cast<void**>(&g_orig_omset_rendertargets) },
        { VT_OMSETRTS_AND_UAVS,    reinterpret_cast<void*>(&hooked_omset_rts_and_uavs),     reinterpret_cast<void**>(&g_orig_omset_rts_and_uavs) },
        { VT_OMSETBLENDSTATE,      reinterpret_cast<void*>(&hooked_omset_blendstate),       reinterpret_cast<void**>(&g_orig_omset_blendstate) },
        { VT_OMSETDEPTHSTENCIL,    reinterpret_cast<void*>(&hooked_omset_depthstencil),     reinterpret_cast<void**>(&g_orig_omset_depthstencil) },
        { VT_CLEARDEPTHSTENCIL,    reinterpret_cast<void*>(&hooked_clear_depthstencil),     reinterpret_cast<void**>(&g_orig_clear_depthstencil) },
        { VT_MAP,                  reinterpret_cast<void*>(&hooked_map),                     reinterpret_cast<void**>(&g_orig_map) },
        { VT_UNMAP,                reinterpret_cast<void*>(&hooked_unmap),                   reinterpret_cast<void**>(&g_orig_unmap) },
        { VT_UPDATESUBRESOURCE,    reinterpret_cast<void*>(&hooked_updatesubresource),      reinterpret_cast<void**>(&g_orig_updatesubresource) },
        { VT_COPYRESOURCE,         reinterpret_cast<void*>(&hooked_copyresource),           reinterpret_cast<void**>(&g_orig_copyresource) },   // PIP_FX_COPY.
        { VT_RESOLVESUBRESOURCE,   reinterpret_cast<void*>(&hooked_resolvesubresource),     reinterpret_cast<void**>(&g_orig_resolvesubresource) },
    };

    size_t created = 0;
    MH_STATUS st = MH_OK;   // The failing call's status, published.

    for (const auto& s : specs) {
        st = MH_CreateHook(vt[s.slot], s.detour, s.original);
        if (st != MH_OK) break;
        ++created;
    }

    if (created == sizeof(specs) / sizeof(specs[0])) {
        size_t enabled = 0;

        for (const auto& s : specs) {
            st = MH_EnableHook(vt[s.slot]);
            if (st != MH_OK) break;
            ++enabled;
        }

        if (enabled == created) {
            g_reorder_hook.vt = static_cast<void*>(vt);
            g_reorder_hook.fail_count = 0;   // A success clears the ladder.
            g_reorder_hook.retry_ms = 0;
            g_reorder_hook.active.store(true, std::memory_order_release);
            rp_extra_hooks(ctx, vt);   // VOL_REPLAY: optional, after the core hooks.
            return;
        }

        g_reorder_hook.fail_phase = 3;
        g_reorder_hook.fail_slot = static_cast<int32_t>(enabled);
        for (size_t i = 0; i < enabled; ++i) MH_DisableHook(vt[specs[i].slot]);
    } else {
        g_reorder_hook.fail_phase = 2;
        g_reorder_hook.fail_slot = static_cast<int32_t>(created);
    }

    g_reorder_hook.mh_status = static_cast<int32_t>(st);
    // EA_HOOK_ORIG_NULL: MH_CreateHook stored each created hook's trampoline in its g_orig_* and MH_RemoveHook frees
    // it - nulled with it, as rp_extra_hooks does, so the direct callers' null tests (the skin / user-VS SO draws'
    // g_orig_draw, WSEE's g_orig_resolvesubresource) take the plain call instead of a freed trampoline.
    for (size_t i = 0; i < created; ++i) {
        MH_RemoveHook(vt[specs[i].slot]);
        *specs[i].original = nullptr;
    }
    reorder_hook_fail_round("draw hook install failed");
}

// RENDER_FLUSH: the flush's per-pass frame template (view / inverse / fxCam, the rebase verdict, the lighting frame
// lanes, the band table, the sun and fog lanes, the heightfield lanes, the bucket pass lanes); returns the rebase
// verdict (rebase_on). The PIP template publish and the upload stay at the call site; the injection builds its
// twin (cbf) inline.
inline bool fx_frame_template(ConstantData& cbf, const RVExtBridge::ProjectionViewTransform& pv,
                              const float view_proj[4][4], const float inv_view_proj[4][4],
                              const float fx_cam[4], bool engcam, const float cam[3]) {
    memcpy(cbf.view_proj, view_proj, sizeof(cbf.view_proj));
    memcpy(cbf.inv_view_proj, inv_view_proj, sizeof(cbf.inv_view_proj));
    memcpy(cbf.fx_cam, fx_cam, sizeof(cbf.fx_cam));   // FX_CAM_REL.
    const bool rebase_on = engcam
                         ? rebase_vp_engcam(cbf, pv.view, pv.projection)
                         : rebase_vp(cbf, cam);   // DDX_REBASE.
    fill_lighting_frame_cb(cbf);
    {
        bool band_any = false;
        // Finest-first: order slots by near edge ascending, so the shader's first containing band holds the finest
        // content in the engine's overlap zones. bandBorder.w = 1 + the physical texture slot to sample; 0 = invalid.
        int order[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

        for (int a = 0; a < 7; ++a) {
            for (int b2 = a + 1; b2 < 8; ++b2) {
                const float na = g_ls.band[order[a]].valid ? g_ls.band[order[a]].border[0] : 1e9f;
                const float nb = g_ls.band[order[b2]].valid ? g_ls.band[order[b2]].border[0] : 1e9f;

                if (nb < na) {
                    const int t = order[a];
                    order[a] = order[b2];
                    order[b2] = t;
                }
            }
        }

        float prev_far = -1e9f;

        for (int b = 0; b < 8; ++b) {
            const auto& bs = g_ls.band[order[b]];
            cbf.band_border[b][3] = 0.0f;
            if (!bs.valid || !bs.srv) { continue; }

            // Absent beats offset: the receive term stands down for RECEIVE_WARMUP_S after the first capture.
            if (!g_ls.receive_warmed) {
                if (g_ls.first_capture_time < 0.0f ||
                    effect_time_seconds() - g_ls.first_capture_time < RECEIVE_WARMUP_S) {
                    continue;
                }
                g_ls.receive_warmed = true;
            }

            if (!recv_health()) { continue; }

            // Spawn-window guard: a pending band whose provisional view is the bridge fallback pairs cross-convention;
            // the band re-enters the moment its seal completes.
            if (bs.pending_view && bs.vcol_bridge) {  continue; }

            // A pending band's vcol belongs to the epoch before the sm sitting next to it.
            if (bs.pending_view) {
                continue;
            }

            // Sanity: an inverted, negative or past-4000 m border is no cascade - the band is dropped until a resolve
            // reseals it.
            if (!(bs.border[1] > bs.border[0]) || bs.border[0] < 0.0f ||
                bs.border[1] > 4000.0f) {
                g_ls.band[order[b]].valid = false;   // The next resolve reseals sanely.
                continue;
            }

            // Twin dedupe: near inside the previous band's range and far not meaningfully beyond it = a partition
            // duplicate; two twins alternate seal content and the shader's winner flip-flops.
            if (bs.border[0] < prev_far - 0.5f && bs.border[1] < prev_far * 1.25f + 1.0f) { continue; }
            prev_far = bs.border[1];

            memcpy(cbf.band_mat[b * 3 + 0], bs.sm + 0, 16);
            memcpy(cbf.band_mat[b * 3 + 1], bs.sm + 4, 16);
            memcpy(cbf.band_mat[b * 3 + 2], bs.sm + 8, 16);
            memcpy(cbf.band_view[b * 3 + 0], bs.vcol + 0, 16);
            memcpy(cbf.band_view[b * 3 + 1], bs.vcol + 4, 16);
            memcpy(cbf.band_view[b * 3 + 2], bs.vcol + 8, 16);
            cbf.band_border[b][0] = bs.border[0];
            cbf.band_border[b][1] = bs.border[1];
            cbf.band_border[b][2] = bs.border[2];
            cbf.band_border[b][3] = 1.0f + static_cast<float>(order[b]);
            band_any = true;
        }

        if (band_any && g_sun_valid &&
            (g_atlas_frame_cur || g_atlas_frame_prev)) {   // Published sun + live atlas.
            cbf.mask_meta[0] = 1.0f;   // Stream liveness: the receive.
        }   // Stands down before the first publish or while the cascade stream is dead.
        cbf.mask_meta[3] = svs_unit_on() ? 1.0f : 0.0f;
        // The volume transport arm and the copy's dimensions, flush twin.
        fill_sten_reproj(cbf, pv.projection);

        // Below the fog layer the shaders split the ray at the layer (fogBelow.y = 1, armed by every mesh fill) and
        // take the below-layer part through fogBelow.x (and the fogUw colour where fogUw.w arms it); KhHazeT's
        // below-layer stand-down (fogBelow.y < 0.5) is never selected from this fill.
        const float cam_y =
            (g_ls.cam[0] * g_ls.cam[0] + g_ls.cam[1] * g_ls.cam[1] +
             g_ls.cam[2] * g_ls.cam[2] >= 1.0f) ? g_ls.cam[1]
          : (cam[0] * cam[0] + cam[1] * cam[1] + cam[2] * cam[2] >= 1.0f)
             ? cam[1] : 0.0f;
        // The TRUE camera altitude, unclamped. The shaders arm their below-layer branch on camY < fogSkyCol.w
        // (static.hlsl, composite.hlsl), so a lane clamped at the layer would arm that branch nowhere: the ray is
        // split shader-side, not here.
        const float fog_cam_y = cam_y;
        // fogBelow: x = the engine's below-layer extinction (block nb[51]), y = armed. z is written 0 here: it is the
        // cast fire's lane (CAST_SNAP_FACING, read by PSMaskCast alone), which the mesh shaders never read; w is
        // never written.
        cbf.fog_below[0] = (g_light_probe.hits > 0 && g_light_probe.meta == 40)   // PROBE_META40.
                         ? g_light_probe.nb[51] : 0.0f;
        cbf.fog_below[1] = 1.0f;
        cbf.fog_below[2] = 0.0f;
        const bool fog_on = g_fog_valid && g_fog[0] > 1e-4f;
        const bool eng_ok = g_light_probe.hits > 0 && g_light_probe.meta == 40;
        fill_haze(cbf);

        if (fog_on || cbf.haze_pars[3] >= 0.5f || eng_ok) {
            cbf.fog_params[0] = fog_on ? g_fog[0] : 0.0f;
            cbf.fog_params[1] = g_fog[1];
            cbf.fog_params[2] = g_fog[2];
            cbf.fog_params[3] = fog_on ? 1.0f : 0.0f;

            // Fog is decoupled from the block lock, so a degenerate staged decay or the pre-lock cold window does not
            // leave the mesh unfogged in a fogged world.
            if (g_light_probe.hits > 0 && g_light_probe.meta == 40) {
                const float* e = g_light_probe.nb + 36;

                for (int c = 0; c < 3; ++c) cbf.fog_color[c] = e[c];

                // Engine transmittance terms from the block mirror: the density
                // scale, the linear ramp's end + inverse range.
                cbf.fog_engine[0] = g_light_probe.nb[41];
                cbf.fog_engine[1] = g_light_probe.nb[48];
                cbf.fog_engine[2] = g_light_probe.nb[49];
                const bool dec_fresh =
                    g_light_probe.hits > 0 &&
                    (effect_time_seconds() - g_light_probe.mirror_confirm) < 0.5f;   // DEC_MIRROR_CLOCK.
                const bool dec_eng = dec_fresh && g_light_probe.nb[40] >= 0.0f && g_light_probe.nb[40] < 1.0f;
                if (dec_eng) {
                    cbf.fog_params[1] = g_light_probe.nb[40];
                }
                const bool terms = g_light_probe.nb[48] > 1.0f &&
                                   g_light_probe.nb[49] > 0.0f &&
                                   g_light_probe.nb[41] >= 0.0f;
                const float rp_end = g_light_probe.nb[48];
                const float rp_inv = g_light_probe.nb[49];
                const float rp_prod = rp_end * rp_inv;
                const bool rp_incoh = terms && rp_prod < 1.0f;
                const bool ramp_off = rp_incoh;
                cbf.fog_engine[3] = !terms ? 0.0f : (ramp_off ? 2.0f : 1.0f);
            } else {
                cbf.fog_color[0] = 1.0f;
                cbf.fog_color[1] = 1.0f;
                cbf.fog_color[2] = 1.0f;
            }

            if (g_sky_probe.hits > 0) {
                cbf.fog_sky[0] = 1.0f;
                cbf.fog_sky[1] = 1.0f;
                cbf.fog_sky[2] = 1.0f;
                cbf.fog_sky[3] = 1.0f;
                cbf.fog_sky_col[0] = g_sky_probe.nb[4];
                cbf.fog_sky_col[1] = g_sky_probe.nb[5];
                cbf.fog_sky_col[2] = g_sky_probe.nb[6];
                // The below-layer convergence colour: nb[28..30] = the sky fog colour, nb[68..70] = its elevation
                // gradient. Pure captures, inert unless the shader is below the layer.
                cbf.fog_uw[0] = g_sky_probe.nb[28];
                cbf.fog_uw[1] = g_sky_probe.nb[29];
                cbf.fog_uw[2] = g_sky_probe.nb[30];
                cbf.fog_uw[3] =
                    1.0f;
                cbf.fog_uw_grad[0] = g_sky_probe.nb[68];
                cbf.fog_uw_grad[1] = g_sky_probe.nb[69];
                cbf.fog_uw_grad[2] = g_sky_probe.nb[70];
            }

            // Twin at the injection fill and in dlsw_fog_lanes (DLSW_FOG_FILL). Camera altitude (engine Y-up),
            // unclamped - see fog_cam_y.
            cbf.fog_color[3] = fog_cam_y;
        }
    }
    // Heightfield lanes, both arms: solids always carry them; effect meshes read them only behind the local1.z arb arm,
    // so the always-live fill is inert for unarmed effects; chain passes never arm.
    fill_occ(cbf);
    // OBJBUF pass lanes (twin of the injection's).
    cbf.pass_cam[0] = cam[0]; cbf.pass_cam[1] = cam[1]; cbf.pass_cam[2] = cam[2];
    cbf.pass_cam[3] = rebase_on ? 1.0f : 0.0f;
    cbf.pass_obj[0] = g_obj_vis.load(std::memory_order_relaxed);
    return rebase_on;
}
}