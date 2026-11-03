#pragma once

// The injection half (rendering_integration_injection.hpp): the resource hooks: Map / Unmap, the copy and update hooks
// and the extra draw hooks. Never include it on its own.
namespace RenderIntegration {

static HRESULT STDMETHODCALLTYPE hooked_map(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub, D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE* mapped) {
    KH_PROF_SCOPE(PROF_HOOK_MAP);   // PROF (MAP_BRANCH: from the top - the replay calls are the hook's cost).
    const bool on = prof_scope_PROF_HOOK_MAP.on;   // MAP_BRANCH: the scope's armed snapshot.
    int rp = 0;   // MAP_BRANCH: rp_on_map's branch.
    bool lk = false;   // MAP_BRANCH: the forget took the mutex.
    // REPLAY_SPLIT: before the discard reaches the runtime. A split that threw (its pool's growth) refuses
    // the pass, as rp_record's catch does - left armed, the draws kept reading the buffer the discard rewrites.
    try {
        rp = rp_on_map(self, res, type);
    } catch (...) {
        if (g_rp_rec.ok) stat_inc(g_rp_rec.c.refused);   // RP_THROW: counted once, as every refusal.
        g_rp_rec.ok = false;
    }
    try { lk = rp_cb_forget(res); } catch (...) {}   // REPLAY_CBREUSE: any map may write.
    if (on) {   // MAP_BRANCH.
        fun_any(FUNNEL_HK_MAP_N, 1u);
        if (lk) fun_any(FUNNEL_HK_RP_LOCK, 1u);
        if (rp != 0) fun_add(FUNNEL_HK_RP_CHECK, 1u);   // Past the split's render-thread gate: plain.
        if (rp == 2) fun_add(FUNNEL_HK_RP_SPLIT, 1u);
    }
    if (sub == 0 && type == D3D11_MAP_WRITE_DISCARD && res &&   // MAP_PREFETCH: hints, before the driver's Map.
        self == g_reorder_target_ctx.load(std::memory_order_relaxed) && reorder_on_render_thread() &&
        !g_ro.in_injection && g_track_wanted.load(std::memory_order_relaxed)) {
        map_prefetch(res);
    }
    const uint64_t t0 = prof_drv_t0(prof_scope_PROF_HOOK_MAP);   // PROF_DRV.
    const HRESULT hr = g_orig_map(self, res, sub, type, flags, mapped);
    prof_drv(PROF_DRV_MAP, t0);
    // MAP_REDIRECT: a record of an earlier mapping of this buffer, here, never reached its Unmap.
    if (SUCCEEDED(hr) && sub == 0 && g_redir_live.load(std::memory_order_relaxed) != 0u) {
        redir_drop(static_cast<void*>(self), res);
        if (on) fun_any(FUNNEL_HK_RD_DROP, 1u);   // MAP_BRANCH.
    }

    try {
    if (SUCCEEDED(hr) && mapped && mapped->pData && sub == 0 &&
        (type == D3D11_MAP_WRITE_DISCARD || type == D3D11_MAP_WRITE_NO_OVERWRITE) &&
        upload_hook_wanted(self, res)) {
        if (on) fun_add(FUNNEL_HK_ADMIT, 1u);   // MAP_BRANCH (past the gate's render-thread test: plain).
        const uint32_t bytes = proj_upload_byte_width(res);

        if (bytes != 0) {
            ProjPendingMap* slot = nullptr;

            for (auto& p : g_proj_pending) {
                if (p.res == res) { slot = &p; break; }
                if (!p.res && !slot) slot = &p;
            }

            if (slot) {
                if (on) fun_add(FUNNEL_HK_PENDING, 1u);   // MAP_BRANCH.
                slot->res = res;
                slot->data = mapped->pData;
                slot->bytes = bytes;
                slot->discard = type == D3D11_MAP_WRITE_DISCARD;   // MAP_MIX.
                slot->redir = false;
                if (slot->discard) {   // MAP_REDIRECT: the engine writes our buffer (redir_open swaps it in).
                    uint8_t* const b = redir_open(static_cast<void*>(self), res, mapped, bytes, on);
                    if (b) {
                        slot->data = b;
                        slot->redir = true;
                    }
                    // PROF_FUNNEL, on the hook scope's snapshot: g_prof.up_on is still the previous capture's.
                    if (on) {
                        if (b) { fun_add(FUNNEL_REDIR_N, 1u); fun_add(FUNNEL_REDIR_B, bytes); }
                        else fun_add(FUNNEL_REDIR_REFUSED, 1u);
                    }
                }
            } else if (on) {
                fun_add(FUNNEL_HK_NOSLOT, 1u);   // MAP_BRANCH: every slot held.
            }
        }
    }
    } catch (...) { hook_except(); }

    return hr;
}

static void STDMETHODCALLTYPE hooked_unmap(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub) {
    KH_PROF_SCOPE(PROF_HOOK_MAP);   // PROF.
    const bool on = prof_scope_PROF_HOOK_MAP.on;   // MAP_BRANCH: the scope's armed snapshot.
    if (on) fun_any(FUNNEL_HK_UNMAP_N, 1u);   // MAP_BRANCH.
    bool cap = false;   // UPLOAD_LAZY: this call captured.
    bool cached = false;   // MAP_REDIRECT: this call raised g_wc_cached_src.
    try {
    if (sub == 0 && self == g_reorder_target_ctx.load(std::memory_order_relaxed)) {
        if (on) fun_any(FUNNEL_HK_UN_WALK, 1u);   // MAP_BRANCH.
        for (auto& p : g_proj_pending) {
            if (p.res == res) {
                // The engine has finished writing; the pointer is valid until the original Unmap below runs. One read
                // into the scratch - of the mapped memory (the WC read), or, a redirected map's (MAP_REDIRECT), of
                // our cached buffer, plainly and only while this context's record still holds it (a stale slot is
                // skipped); the scanners walk what the capture returns (the buffer's whole shadow when the light
                // capture takes it, else the scratch).
                // PENDING_TAKE: the slot is emptied before anything below can throw - left set, a later Unmap of
                // this buffer (whose Map the gate turned down) would read the pointer of an ended mapping.
                if (on) fun_any(FUNNEL_HK_UN_HIT, 1u);   // MAP_BRANCH.
                const ProjPendingMap us_p = p;
                p = ProjPendingMap{};
                // PENDING_GATE: this Unmap ends the slot's mapping whatever the gate says now (a thread-id
                // reset or hand-over between the Map and here left the slot holding the pointer of an ended
                // mapping). Only the capture is render-thread-only (pairs with the map gate above).
                if (!reorder_on_render_thread() || g_ro.in_injection) break;
                if (us_p.redir && !redir_holds(static_cast<void*>(self), res, us_p.data)) break;
                uint32_t n = us_p.bytes;
                cap = true;
                if (us_p.redir) { g_wc_cached_src = true; cached = true; }   // MAP_REDIRECT.
                const void* d = upload_capture(res, us_p.data, n);   // DL_CPU.
                if (g_prof.up_on) fun_map_mix(us_p.discard, us_p.bytes);   // MAP_MIX (the capture armed it).
                upload_scan(res, d, n);
                KH_PROF_SUB(PROF_SC_RPIMG, g_prof.up_on, g_prof.up_t);   // PROF_UPLOAD.
                rp_img_note(res, d, n);   // REPLAY_CPUCB: before the source is unmapped.
                break;
            }
        }
    }

    } catch (...) { hook_except(); }
    if (cap) upload_capture_end();   // Before the source is unmapped.
    if (cached) g_wc_cached_src = false;   // MAP_REDIRECT.
    // MAP_REDIRECT: the engine's bytes to the driver's pointer, whatever the capture did (any thread: the record
    // is this context's or none).
    if (sub == 0 && g_redir_live.load(std::memory_order_relaxed) != 0u) {
        redir_close(static_cast<void*>(self), res);
        if (on) fun_any(FUNNEL_HK_RD_CLOSE, 1u);   // MAP_BRANCH.
    }
    const uint64_t t0 = prof_drv_t0(prof_scope_PROF_HOOK_MAP);   // PROF_DRV.
    g_orig_unmap(self, res, sub);
    prof_drv(PROF_DRV_UNMAP, t0);
}

// WSEE_XDRAW - DrawAuto and both indirect draws never reach wsee_pre_draw and have no re-issue: one writing the
// main depth while this cycle's capture stands (landed, untainted, the chain not yet drawn; the pre-draw's own-draw
// and thread gates, the view model's slice left out) would be a late opaque or a layer the record misses, so it
// taints. Render thread, the engine's context; a few loads and compares, one RSGetViewports only past them.
inline void wsee_xdraw(ID3D11DeviceContext* ctx) {
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed)) return;
    const uint64_t c = g_topo_cycles;
    if (!g_wsee.on || g_wsee.ok_cycle.load(std::memory_order_relaxed) != c ||
        g_wsee.taint_cycle.load(std::memory_order_relaxed) == c ||
        g_wsee.used_cycle.load(std::memory_order_relaxed) == c) return;
    if (g_ro.in_injection || !g_ro.dsv_main || !g_ro.dss_write || g_flush_active.load(std::memory_order_relaxed) ||
        !g_track_wanted.load(std::memory_order_relaxed) || !reorder_on_render_thread()) return;
    UINT n = 1;
    D3D11_VIEWPORT vp = {};
    ctx->RSGetViewports(&n, &vp);
    if (n < 1 || infront_vp_is_slice(vp)) return;
    wsee_taint(c);
}
static void STDMETHODCALLTYPE hooked_draw_auto(ID3D11DeviceContext* self) {   // VOL_REPLAY.
    {
        KH_PROF_SCOPE(PROF_HOOK_MISC);   // PROF_HOOKS.
        try { if (rp_engine_call(self) && g_svs.vol_dsv_bound) rp_foreign(5); } catch (...) { hook_except(); }
        try { psb_zb_draw(self); } catch (...) { hook_except(); }   // PSB_ZB: the core hooks' twin.
        try { wsee_xdraw(self); } catch (...) { hook_except(); }   // WSEE_XDRAW.
    }
    g_orig_draw_auto(self);
}
static void STDMETHODCALLTYPE hooked_draw_indexed_inst_indirect(ID3D11DeviceContext* self, ID3D11Buffer* args, UINT off) {
    {
        KH_PROF_SCOPE(PROF_HOOK_MISC);   // PROF_HOOKS.
        try { if (rp_engine_call(self) && g_svs.vol_dsv_bound) rp_foreign(5); } catch (...) { hook_except(); }
        try { psb_zb_draw(self); } catch (...) { hook_except(); }   // PSB_ZB: the core hooks' twin.
        try { wsee_xdraw(self); } catch (...) { hook_except(); }   // WSEE_XDRAW.
    }
    g_orig_draw_indexed_inst_indirect(self, args, off);
}
static void STDMETHODCALLTYPE hooked_draw_inst_indirect(ID3D11DeviceContext* self, ID3D11Buffer* args, UINT off) {
    {
        KH_PROF_SCOPE(PROF_HOOK_MISC);   // PROF_HOOKS.
        try { if (rp_engine_call(self) && g_svs.vol_dsv_bound) rp_foreign(5); } catch (...) { hook_except(); }
        try { psb_zb_draw(self); } catch (...) { hook_except(); }   // PSB_ZB: the core hooks' twin.
        try { wsee_xdraw(self); } catch (...) { hook_except(); }   // WSEE_XDRAW.
    }
    g_orig_draw_inst_indirect(self, args, off);
}
static void STDMETHODCALLTYPE hooked_copy_region(ID3D11DeviceContext* self, ID3D11Resource* dst, UINT dsub, UINT x, UINT y,
                                                 UINT z, ID3D11Resource* src, UINT ssub, const D3D11_BOX* box) {
    try {
        KH_PROF_SCOPE(PROF_HOOK_COPYREGION);   // PROF_HOOKS.
        psb_zb_dst(self, dst);   // PSB_ZB: a copy into the main depth ends Zb.
        rp_cb_forget(dst);   // REPLAY_CBREUSE.
        if (rp_engine_call(self) && dst && g_svs.vol_src &&
            static_cast<void*>(dst) == static_cast<void*>(g_svs.vol_src))
            rp_foreign(6);
        rp_on_write(self, dst, false);   // RP_WRITE_SPLIT.
    } catch (...) { hook_except(); }
    g_orig_copy_region(self, dst, dsub, x, y, z, src, ssub, box);
}
// REPLAY_CBREUSE: the writers the reuse must see, beyond the core hooks. CopySubresourceRegion1 also refuses a pass
// it writes the volume buffer in, as its context-0 twin above does.
static void STDMETHODCALLTYPE hooked_copy_structure_count(ID3D11DeviceContext* self, ID3D11Buffer* dst, UINT off,
                                                          ID3D11UnorderedAccessView* src) {
    {
        KH_PROF_SCOPE(PROF_HOOK_MISC);   // PROF_HOOKS.
        try { rp_cb_forget(dst); } catch (...) { hook_except(); }
        rp_on_write(self, dst, false);   // RP_WRITE_SPLIT.
    }
    g_orig_copy_structure_count(self, dst, off, src);
}
static void STDMETHODCALLTYPE hooked_copy_region1(ID3D11DeviceContext1* self, ID3D11Resource* dst, UINT dsub, UINT x,
                                                  UINT y, UINT z, ID3D11Resource* src, UINT ssub, const D3D11_BOX* box,
                                                  UINT flags) {
    try {
        KH_PROF_SCOPE(PROF_HOOK_COPYREGION);   // PROF_HOOKS.
        psb_zb_dst(self, dst);   // PSB_ZB: hooked_copy_region's twin.
        rp_cb_forget(dst);
        if (rp_engine_call(self) && dst && g_svs.vol_src &&
            static_cast<void*>(dst) == static_cast<void*>(g_svs.vol_src))
            rp_foreign(6);
        rp_on_write(self, dst, (flags & D3D11_COPY_NO_OVERWRITE) != 0);   // RP_WRITE_SPLIT.
    } catch (...) { hook_except(); }
    g_orig_copy_region1(self, dst, dsub, x, y, z, src, ssub, box, flags);
}
static void STDMETHODCALLTYPE hooked_update_subresource1(ID3D11DeviceContext1* self, ID3D11Resource* res, UINT sub,
                                                         const D3D11_BOX* box, const void* data, UINT rp, UINT dp,
                                                         UINT flags) {
    {
        KH_PROF_SCOPE(PROF_HOOK_MISC);   // PROF_HOOKS.
        try { rp_cb_forget(res); } catch (...) { hook_except(); }
        try { psb_zb_dst(self, res); } catch (...) { hook_except(); }   // PSB_ZB: hooked_updatesubresource's.
        rp_on_write(self, res, (flags & D3D11_COPY_NO_OVERWRITE) != 0);   // RP_WRITE_SPLIT.
    }
    g_orig_update_subresource1(self, res, sub, box, data, rp, dp, flags);
}
static void STDMETHODCALLTYPE hooked_execute_command_list(ID3D11DeviceContext* self, ID3D11CommandList* cl, BOOL restore) {
    {
        KH_PROF_SCOPE(PROF_HOOK_MISC);   // PROF_HOOKS.
        try { if (rp_engine_call(self)) rp_foreign(4); } catch (...) { hook_except(); }
        try { rp_img_clear(); } catch (...) { hook_except(); }   // REPLAY_CPUCB: it can write any buffer.
        try { psb_zb_list(self); } catch (...) { hook_except(); }   // PSB_ZB: and the main depth.
    }
    g_orig_execute_command_list(self, cl, restore);
}
// Installed one by one after the core table succeeded: a failure leaves that one off and never touches the core hooks.
inline void rp_extra_hooks(ID3D11DeviceContext* ctx, void** vt) {
    struct Sp { int slot; void* det; void** orig; };
    const Sp sp[] = {
        { VT_DRAWAUTO,             reinterpret_cast<void*>(&hooked_draw_auto),                  reinterpret_cast<void**>(&g_orig_draw_auto) },
        { VT_DRAWINDEXEDINSTIND,   reinterpret_cast<void*>(&hooked_draw_indexed_inst_indirect), reinterpret_cast<void**>(&g_orig_draw_indexed_inst_indirect) },
        { VT_DRAWINSTIND,          reinterpret_cast<void*>(&hooked_draw_inst_indirect),         reinterpret_cast<void**>(&g_orig_draw_inst_indirect) },
        { VT_COPYSUBRESOURCEREGION, reinterpret_cast<void*>(&hooked_copy_region),               reinterpret_cast<void**>(&g_orig_copy_region) },
        { VT_EXECUTECOMMANDLIST,   reinterpret_cast<void*>(&hooked_execute_command_list),       reinterpret_cast<void**>(&g_orig_execute_command_list) },
        { VT_COPYSTRUCTURECOUNT,   reinterpret_cast<void*>(&hooked_copy_structure_count),       reinterpret_cast<void**>(&g_orig_copy_structure_count) },
    };
    for (const auto& x : sp) {
        if (*x.orig) continue;   // Already in place (a later install round).
        if (MH_CreateHook(vt[x.slot], x.det, x.orig) != MH_OK) { *x.orig = nullptr; continue; }
        if (MH_EnableHook(vt[x.slot]) != MH_OK) { MH_RemoveHook(vt[x.slot]); *x.orig = nullptr; continue; }
    }
    // REPLAY_CBREUSE: the context-1 writers, from that interface's own table - a context without it has neither
    // (and a runtime's engine cannot call them). Reuse only while every writer the context has is hooked.
    bool c1 = true;
    ID3D11DeviceContext1* ctx1 = nullptr;
    if (ctx && SUCCEEDED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&ctx1))) &&
        ctx1) {
        void** vt1 = *reinterpret_cast<void***>(ctx1);
        const Sp sp1[] = {
            { VT1_COPYSUBRESOURCEREGION1, reinterpret_cast<void*>(&hooked_copy_region1),        reinterpret_cast<void**>(&g_orig_copy_region1) },
            { VT1_UPDATESUBRESOURCE1,     reinterpret_cast<void*>(&hooked_update_subresource1), reinterpret_cast<void**>(&g_orig_update_subresource1) },
        };
        for (const auto& x : sp1) {
            if (*x.orig) continue;
            if (MH_CreateHook(vt1[x.slot], x.det, x.orig) != MH_OK) { *x.orig = nullptr; continue; }
            if (MH_EnableHook(vt1[x.slot]) != MH_OK) { MH_RemoveHook(vt1[x.slot]); *x.orig = nullptr; continue; }
        }
        c1 = g_orig_copy_region1 && g_orig_update_subresource1;
        ctx1->Release();
    }
    g_rp_rec.cbreuse_on.store(c1 && g_orig_copy_region && g_orig_copy_structure_count &&
                              g_orig_execute_command_list,   // A command list writes too: its hook drops the images.
                              std::memory_order_relaxed);
}
// PIP_FX_COPY: the engine copies the PIP colour target out with CopyResource (mid-pass for its translucents, and
// after the last draw for its post pass, which overwrites the target from that copy). The passes run here, before the
// copy, so the copy carries them; the first copy of a cycle is the engine's opaque boundary. RT0 is compared by
// resource, not by view.
static void STDMETHODCALLTYPE hooked_copyresource(ID3D11DeviceContext* self, ID3D11Resource* dst, ID3D11Resource* src) {
    // RP_WRITE_SPLIT: ahead of this hook's profile scope - a split's own copy enters this hook (as ours).
    rp_on_write(self, dst, false);
    {
    KH_PROF_SCOPE(PROF_HOOK_COPYRES);   // PROF_HOOKS: ours, up to the driver's call.
    try { rp_cb_forget(dst); } catch (...) {}   // REPLAY_CBREUSE.
    try { psb_zb_dst(self, dst); } catch (...) { hook_except(); }   // PSB_ZB: a copy into the main depth.
    try {   // VOL_REPLAY: a copy into the volume buffer inside a pass.
        if (rp_engine_call(self) && dst && g_svs.vol_src &&
            static_cast<void*>(dst) == static_cast<void*>(g_svs.vol_src))
            rp_foreign(6);
    } catch (...) {}
    try {
    if (src && g_pip.on && !g_pip.fx_done && g_pip.opaques >= PIP_MIN_OPAQUES &&
        self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        reorder_on_render_thread() && !g_ro.in_injection &&
        !g_flush_active.load(std::memory_order_relaxed) &&
        g_track_wanted.load(std::memory_order_relaxed)) {
        ID3D11RenderTargetView* rtv = nullptr;
        self->OMGetRenderTargets(1, &rtv, nullptr);
        if (rtv) {
            ID3D11Resource* res = nullptr;
            rtv->GetResource(&res);
            rtv->Release();
            if (res) {
                const bool pip = res == src;
                res->Release();
                if (pip) pip_run_fx(self);
            }
        }
    }
    } catch (...) { hook_except(); }
    }
    g_orig_copyresource(self, dst, src);
}

static void STDMETHODCALLTYPE hooked_updatesubresource(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub, const D3D11_BOX* dst_box, const void* data, UINT row_pitch, UINT depth_pitch) {
    KH_PROF_SCOPE(PROF_HOOK_UPDSUB);   // PROF_HOOKS: the whole hook (the driver's share: drvUpdSub).
    try { rp_cb_forget(res); } catch (...) {}   // REPLAY_CBREUSE.
    try { psb_zb_dst(self, res); } catch (...) { hook_except(); }   // PSB_ZB: refused by D3D11 on a depth.
    rp_on_write(self, res, false);   // RP_WRITE_SPLIT.
    bool cap = false;   // UPLOAD_LAZY: this call captured.
    try {
    if (sub == 0 && !dst_box && data &&
        upload_hook_wanted(self, res)) {
        const uint32_t bytes = proj_upload_byte_width(res);

        if (bytes != 0) {
            uint32_t n = bytes;
            cap = true;
            const void* d = upload_capture(res, data, n);   // DL_CPU.
            if (g_prof.up_on) fun_add(FUNNEL_UPDSUB, 1u);   // PROF_FUNNEL.
            upload_scan(res, d, n);
            KH_PROF_SUB(PROF_SC_RPIMG, g_prof.up_on, g_prof.up_t);   // PROF_UPLOAD.
            rp_img_note(res, d, n, self);   // REPLAY_CPUCB: the caller's data is the whole buffer.
        }
    }

    } catch (...) { hook_except(); }
    if (cap) upload_capture_end();   // Before the caller's data is returned.
    const uint64_t t0 = prof_drv_t0(prof_scope_PROF_HOOK_UPDSUB);   // PROF_DRV.
    g_orig_updatesubresource(self, res, sub, dst_box, data, row_pitch, depth_pitch);
    prof_drv(PROF_DRV_UPDSUB, t0);
}
}