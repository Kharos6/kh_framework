#pragma once

// The injection half (rendering_integration_injection.hpp): the resource hooks: Map / Unmap, the copy and update hooks
// and the extra draw hooks. Never include it on its own.
namespace RenderIntegration {

static HRESULT STDMETHODCALLTYPE hooked_map(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub, D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE* mapped) {
    KH_PROF_SCOPE(KHP_HOOK_MAP);   // KH_PROF (KH_MAP_BRANCH: from the top - the replay calls are the hook's cost).
    const bool khmb_on = khps_scope_KHP_HOOK_MAP.khps_on;   // KH_MAP_BRANCH: the scope's armed snapshot.
    int khmb_rp = 0;   // KH_MAP_BRANCH: kh_rp_on_map's branch.
    bool khmb_lk = false;   // KH_MAP_BRANCH: the forget took the mutex.
    // KH_REPLAY_SPLIT: before the discard reaches the runtime. A split that threw (its pool's growth) refuses
    // the pass, as kh_rp_record's catch does - left armed, the draws kept reading the buffer the discard rewrites.
    try {
        khmb_rp = kh_rp_on_map(self, res, type);
    } catch (...) {
        if (g_rp_ok) kh_stat(g_rp_c.refused);   // KH_RP_THROW: counted once, as every refusal.
        g_rp_ok = false;
    }
    try { khmb_lk = kh_rp_cb_forget(res); } catch (...) {}   // KH_REPLAY_CBREUSE: any map may write.
    if (khmb_on) {   // KH_MAP_BRANCH.
        kh_fun_any(KHF_HK_MAP_N, 1u);
        if (khmb_lk) kh_fun_any(KHF_HK_RP_LOCK, 1u);
        if (khmb_rp != 0) kh_fun(KHF_HK_RP_CHECK, 1u);   // Past the split's render-thread gate: plain.
        if (khmb_rp == 2) kh_fun(KHF_HK_RP_SPLIT, 1u);
    }
    if (sub == 0 && type == D3D11_MAP_WRITE_DISCARD && res &&   // KH_MAP_PREFETCH: hints, before the driver's Map.
        self == g_reorder_target_ctx.load(std::memory_order_relaxed) && reorder_on_render_thread() &&
        !g_ro.in_injection && g_kh_track_wanted.load(std::memory_order_relaxed)) {
        kh_map_prefetch(res);
    }
    const uint64_t khdm_t0 = kh_prof_drv_t0(khps_scope_KHP_HOOK_MAP);   // KH_PROF_DRV.
    const HRESULT hr = g_orig_map(self, res, sub, type, flags, mapped);
    kh_prof_drv(KHP_DRV_MAP, khdm_t0);
    // KH_MAP_REDIRECT: a record of an earlier mapping of this buffer, here, never reached its Unmap.
    if (SUCCEEDED(hr) && sub == 0 && g_redir_live.load(std::memory_order_relaxed) != 0u) {
        kh_redir_drop(static_cast<void*>(self), res);
        if (khmb_on) kh_fun_any(KHF_HK_RD_DROP, 1u);   // KH_MAP_BRANCH.
    }

    try {
    if (SUCCEEDED(hr) && mapped && mapped->pData && sub == 0 &&
        (type == D3D11_MAP_WRITE_DISCARD || type == D3D11_MAP_WRITE_NO_OVERWRITE) &&
        kh_upload_hook_wanted(self, res)) {
        if (khmb_on) kh_fun(KHF_HK_ADMIT, 1u);   // KH_MAP_BRANCH (past the gate's render-thread test: plain).
        const uint32_t bytes = proj_upload_byte_width(res);

        if (bytes != 0) {
            ProjPendingMap* slot = nullptr;

            for (auto& p : g_proj_pending) {
                if (p.res == res) { slot = &p; break; }
                if (!p.res && !slot) slot = &p;
            }

            if (slot) {
                if (khmb_on) kh_fun(KHF_HK_PENDING, 1u);   // KH_MAP_BRANCH.
                slot->res = res;
                slot->data = mapped->pData;
                slot->bytes = bytes;
                slot->discard = type == D3D11_MAP_WRITE_DISCARD;   // KH_MAP_MIX.
                slot->redir = false;
                if (slot->discard) {   // KH_MAP_REDIRECT: the engine writes our buffer (kh_redir_open swaps it in).
                    uint8_t* const khrm_b = kh_redir_open(static_cast<void*>(self), res, mapped, bytes, khmb_on);
                    if (khrm_b) {
                        slot->data = khrm_b;
                        slot->redir = true;
                    }
                    // KH_PROF_FUNNEL, on the hook scope's snapshot: g_prof_up_on is still the previous capture's.
                    if (khmb_on) {
                        if (khrm_b) { kh_fun(KHF_REDIR_N, 1u); kh_fun(KHF_REDIR_B, bytes); }
                        else kh_fun(KHF_REDIR_REFUSED, 1u);
                    }
                }
            } else if (khmb_on) {
                kh_fun(KHF_HK_NOSLOT, 1u);   // KH_MAP_BRANCH: every slot held.
            }
        }
    }
    } catch (...) { kh_hook_except(); }

    return hr;
}

static void STDMETHODCALLTYPE hooked_unmap(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub) {
    KH_PROF_SCOPE(KHP_HOOK_MAP);   // KH_PROF.
    const bool khmb_on = khps_scope_KHP_HOOK_MAP.khps_on;   // KH_MAP_BRANCH: the scope's armed snapshot.
    if (khmb_on) kh_fun_any(KHF_HK_UNMAP_N, 1u);   // KH_MAP_BRANCH.
    bool khus_cap = false;   // KH_UPLOAD_LAZY: this call captured.
    bool khus_cached = false;   // KH_MAP_REDIRECT: this call raised g_wc_cached_src.
    try {
    if (sub == 0 && self == g_reorder_target_ctx.load(std::memory_order_relaxed)) {
        if (khmb_on) kh_fun_any(KHF_HK_UN_WALK, 1u);   // KH_MAP_BRANCH.
        for (auto& p : g_proj_pending) {
            if (p.res == res) {
                // The engine has finished writing; the pointer is valid until the original Unmap below runs. One read
                // into the scratch - of the mapped memory (the WC read), or, a redirected map's (KH_MAP_REDIRECT), of
                // our cached buffer, plainly and only while this context's record still holds it (a stale slot is
                // skipped); the scanners walk what the capture returns (the buffer's whole shadow when the light
                // capture takes it, else the scratch).
                // KH_PENDING_TAKE: the slot is emptied before anything below can throw - left set, a later Unmap of
                // this buffer (whose Map the gate turned down) would read the pointer of an ended mapping.
                if (khmb_on) kh_fun_any(KHF_HK_UN_HIT, 1u);   // KH_MAP_BRANCH.
                const ProjPendingMap khus_p = p;
                p = ProjPendingMap{};
                // KH_PENDING_GATE: this Unmap ends the slot's mapping whatever the gate says now (a thread-id
                // reset or hand-over between the Map and here left the slot holding the pointer of an ended
                // mapping). Only the capture is render-thread-only (pairs with the map gate above).
                if (!reorder_on_render_thread() || g_ro.in_injection) break;
                if (khus_p.redir && !kh_redir_holds(static_cast<void*>(self), res, khus_p.data)) break;
                uint32_t khus_n = khus_p.bytes;
                khus_cap = true;
                if (khus_p.redir) { g_wc_cached_src = true; khus_cached = true; }   // KH_MAP_REDIRECT.
                const void* khus_d = kh_upload_capture(res, khus_p.data, khus_n);   // KH_DL_CPU.
                if (g_prof_up_on) kh_fun_map_mix(khus_p.discard, khus_p.bytes);   // KH_MAP_MIX (the capture armed it).
                kh_upload_scan(res, khus_d, khus_n);
                KH_PROF_SUB(KHP_SC_RPIMG, g_prof_up_on, g_prof_up_t);   // KH_PROF_UPLOAD.
                kh_rp_img_note(res, khus_d, khus_n);   // KH_REPLAY_CPUCB: before the source is unmapped.
                break;
            }
        }
    }

    } catch (...) { kh_hook_except(); }
    if (khus_cap) kh_upload_capture_end();   // Before the source is unmapped.
    if (khus_cached) g_wc_cached_src = false;   // KH_MAP_REDIRECT.
    // KH_MAP_REDIRECT: the engine's bytes to the driver's pointer, whatever the capture did (any thread: the record
    // is this context's or none).
    if (sub == 0 && g_redir_live.load(std::memory_order_relaxed) != 0u) {
        kh_redir_close(static_cast<void*>(self), res);
        if (khmb_on) kh_fun_any(KHF_HK_RD_CLOSE, 1u);   // KH_MAP_BRANCH.
    }
    const uint64_t khdu_t0 = kh_prof_drv_t0(khps_scope_KHP_HOOK_MAP);   // KH_PROF_DRV.
    g_orig_unmap(self, res, sub);
    kh_prof_drv(KHP_DRV_UNMAP, khdu_t0);
}

// KH_WSEE_XDRAW - DrawAuto and both indirect draws never reach kh_wsee_pre_draw and have no re-issue: one writing the
// main depth while this cycle's capture stands (landed, untainted, the chain not yet drawn; the pre-draw's own-draw
// and thread gates, the view model's slice left out) would be a late opaque or a layer the record misses, so it
// taints. Render thread, the engine's context; a few loads and compares, one RSGetViewports only past them.
inline void kh_wsee_xdraw(ID3D11DeviceContext* ctx) {
    if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed)) return;
    const uint64_t khwx_c = g_topo_cycles;
    if (!g_wsee_on || g_wsee_ok_cycle.load(std::memory_order_relaxed) != khwx_c ||
        g_wsee_taint_cycle.load(std::memory_order_relaxed) == khwx_c ||
        g_wsee_used_cycle.load(std::memory_order_relaxed) == khwx_c) return;
    if (g_ro.in_injection || !g_ro.dsv_main || !g_ro.dss_write || g_kh_flush_active.load(std::memory_order_relaxed) ||
        !g_kh_track_wanted.load(std::memory_order_relaxed) || !reorder_on_render_thread()) return;
    UINT khwx_n = 1;
    D3D11_VIEWPORT khwx_vp = {};
    ctx->RSGetViewports(&khwx_n, &khwx_vp);
    if (khwx_n < 1 || kh_infront_vp_is_slice(khwx_vp)) return;
    kh_wsee_taint(khwx_c);
}
static void STDMETHODCALLTYPE hooked_draw_auto(ID3D11DeviceContext* self) {   // KH_VOL_REPLAY.
    {
        KH_PROF_SCOPE(KHP_HOOK_MISC);   // KH_PROF_HOOKS.
        try { if (kh_rp_engine_call(self) && g_svs_vol_dsv_bound) kh_rp_foreign(5); } catch (...) { kh_hook_except(); }
        try { kh_psb_zb_draw(self); } catch (...) { kh_hook_except(); }   // KH_PSB_ZB: the core hooks' twin.
        try { kh_wsee_xdraw(self); } catch (...) { kh_hook_except(); }   // KH_WSEE_XDRAW.
    }
    g_orig_draw_auto(self);
}
static void STDMETHODCALLTYPE hooked_draw_indexed_inst_indirect(ID3D11DeviceContext* self, ID3D11Buffer* args, UINT off) {
    {
        KH_PROF_SCOPE(KHP_HOOK_MISC);   // KH_PROF_HOOKS.
        try { if (kh_rp_engine_call(self) && g_svs_vol_dsv_bound) kh_rp_foreign(5); } catch (...) { kh_hook_except(); }
        try { kh_psb_zb_draw(self); } catch (...) { kh_hook_except(); }   // KH_PSB_ZB: the core hooks' twin.
        try { kh_wsee_xdraw(self); } catch (...) { kh_hook_except(); }   // KH_WSEE_XDRAW.
    }
    g_orig_draw_indexed_inst_indirect(self, args, off);
}
static void STDMETHODCALLTYPE hooked_draw_inst_indirect(ID3D11DeviceContext* self, ID3D11Buffer* args, UINT off) {
    {
        KH_PROF_SCOPE(KHP_HOOK_MISC);   // KH_PROF_HOOKS.
        try { if (kh_rp_engine_call(self) && g_svs_vol_dsv_bound) kh_rp_foreign(5); } catch (...) { kh_hook_except(); }
        try { kh_psb_zb_draw(self); } catch (...) { kh_hook_except(); }   // KH_PSB_ZB: the core hooks' twin.
        try { kh_wsee_xdraw(self); } catch (...) { kh_hook_except(); }   // KH_WSEE_XDRAW.
    }
    g_orig_draw_inst_indirect(self, args, off);
}
static void STDMETHODCALLTYPE hooked_copy_region(ID3D11DeviceContext* self, ID3D11Resource* dst, UINT dsub, UINT x, UINT y,
                                                 UINT z, ID3D11Resource* src, UINT ssub, const D3D11_BOX* box) {
    try {
        KH_PROF_SCOPE(KHP_HOOK_COPYREGION);   // KH_PROF_HOOKS.
        kh_psb_zb_dst(self, dst);   // KH_PSB_ZB: a copy into the main depth ends Zb.
        kh_rp_cb_forget(dst);   // KH_REPLAY_CBREUSE.
        if (kh_rp_engine_call(self) && dst && g_svs_vol_src && static_cast<void*>(dst) == static_cast<void*>(g_svs_vol_src))
            kh_rp_foreign(6);
        kh_rp_on_write(self, dst, false);   // KH_RP_WRITE_SPLIT.
    } catch (...) { kh_hook_except(); }
    g_orig_copy_region(self, dst, dsub, x, y, z, src, ssub, box);
}
// KH_REPLAY_CBREUSE: the writers the reuse must see, beyond the core hooks. CopySubresourceRegion1 also refuses a pass
// it writes the volume buffer in, as its context-0 twin above does.
static void STDMETHODCALLTYPE hooked_copy_structure_count(ID3D11DeviceContext* self, ID3D11Buffer* dst, UINT off,
                                                          ID3D11UnorderedAccessView* src) {
    {
        KH_PROF_SCOPE(KHP_HOOK_MISC);   // KH_PROF_HOOKS.
        try { kh_rp_cb_forget(dst); } catch (...) { kh_hook_except(); }
        kh_rp_on_write(self, dst, false);   // KH_RP_WRITE_SPLIT.
    }
    g_orig_copy_structure_count(self, dst, off, src);
}
static void STDMETHODCALLTYPE hooked_copy_region1(ID3D11DeviceContext1* self, ID3D11Resource* dst, UINT dsub, UINT x,
                                                  UINT y, UINT z, ID3D11Resource* src, UINT ssub, const D3D11_BOX* box,
                                                  UINT flags) {
    try {
        KH_PROF_SCOPE(KHP_HOOK_COPYREGION);   // KH_PROF_HOOKS.
        kh_psb_zb_dst(self, dst);   // KH_PSB_ZB: hooked_copy_region's twin.
        kh_rp_cb_forget(dst);
        if (kh_rp_engine_call(self) && dst && g_svs_vol_src && static_cast<void*>(dst) == static_cast<void*>(g_svs_vol_src))
            kh_rp_foreign(6);
        kh_rp_on_write(self, dst, (flags & D3D11_COPY_NO_OVERWRITE) != 0);   // KH_RP_WRITE_SPLIT.
    } catch (...) { kh_hook_except(); }
    g_orig_copy_region1(self, dst, dsub, x, y, z, src, ssub, box, flags);
}
static void STDMETHODCALLTYPE hooked_update_subresource1(ID3D11DeviceContext1* self, ID3D11Resource* res, UINT sub,
                                                         const D3D11_BOX* box, const void* data, UINT rp, UINT dp,
                                                         UINT flags) {
    {
        KH_PROF_SCOPE(KHP_HOOK_MISC);   // KH_PROF_HOOKS.
        try { kh_rp_cb_forget(res); } catch (...) { kh_hook_except(); }
        try { kh_psb_zb_dst(self, res); } catch (...) { kh_hook_except(); }   // KH_PSB_ZB: hooked_updatesubresource's.
        kh_rp_on_write(self, res, (flags & D3D11_COPY_NO_OVERWRITE) != 0);   // KH_RP_WRITE_SPLIT.
    }
    g_orig_update_subresource1(self, res, sub, box, data, rp, dp, flags);
}
static void STDMETHODCALLTYPE hooked_execute_command_list(ID3D11DeviceContext* self, ID3D11CommandList* cl, BOOL restore) {
    {
        KH_PROF_SCOPE(KHP_HOOK_MISC);   // KH_PROF_HOOKS.
        try { if (kh_rp_engine_call(self)) kh_rp_foreign(4); } catch (...) { kh_hook_except(); }
        try { kh_rp_img_clear(); } catch (...) { kh_hook_except(); }   // KH_REPLAY_CPUCB: it can write any buffer.
        try { kh_psb_zb_list(self); } catch (...) { kh_hook_except(); }   // KH_PSB_ZB: and the main depth.
    }
    g_orig_execute_command_list(self, cl, restore);
}
// Installed one by one after the core table succeeded: a failure leaves that one off and never touches the core hooks.
inline void kh_rp_extra_hooks(ID3D11DeviceContext* ctx, void** vt) {
    struct Sp { int slot; void* det; void** orig; };
    const Sp sp[] = {
        { KH_VT_DRAWAUTO,             reinterpret_cast<void*>(&hooked_draw_auto),                  reinterpret_cast<void**>(&g_orig_draw_auto) },
        { KH_VT_DRAWINDEXEDINSTIND,   reinterpret_cast<void*>(&hooked_draw_indexed_inst_indirect), reinterpret_cast<void**>(&g_orig_draw_indexed_inst_indirect) },
        { KH_VT_DRAWINSTIND,          reinterpret_cast<void*>(&hooked_draw_inst_indirect),         reinterpret_cast<void**>(&g_orig_draw_inst_indirect) },
        { KH_VT_COPYSUBRESOURCEREGION, reinterpret_cast<void*>(&hooked_copy_region),               reinterpret_cast<void**>(&g_orig_copy_region) },
        { KH_VT_EXECUTECOMMANDLIST,   reinterpret_cast<void*>(&hooked_execute_command_list),       reinterpret_cast<void**>(&g_orig_execute_command_list) },
        { KH_VT_COPYSTRUCTURECOUNT,   reinterpret_cast<void*>(&hooked_copy_structure_count),       reinterpret_cast<void**>(&g_orig_copy_structure_count) },
    };
    for (const auto& x : sp) {
        if (*x.orig) continue;   // Already in place (a later install round).
        if (MH_CreateHook(vt[x.slot], x.det, x.orig) != MH_OK) { *x.orig = nullptr; continue; }
        if (MH_EnableHook(vt[x.slot]) != MH_OK) { MH_RemoveHook(vt[x.slot]); *x.orig = nullptr; continue; }
    }
    // KH_REPLAY_CBREUSE: the context-1 writers, from that interface's own table - a context without it has neither
    // (and a runtime's engine cannot call them). Reuse only while every writer the context has is hooked.
    bool khxh_c1 = true;
    ID3D11DeviceContext1* khxh_ctx1 = nullptr;
    if (ctx && SUCCEEDED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&khxh_ctx1))) &&
        khxh_ctx1) {
        void** vt1 = *reinterpret_cast<void***>(khxh_ctx1);
        const Sp sp1[] = {
            { KH_VT1_COPYSUBRESOURCEREGION1, reinterpret_cast<void*>(&hooked_copy_region1),        reinterpret_cast<void**>(&g_orig_copy_region1) },
            { KH_VT1_UPDATESUBRESOURCE1,     reinterpret_cast<void*>(&hooked_update_subresource1), reinterpret_cast<void**>(&g_orig_update_subresource1) },
        };
        for (const auto& x : sp1) {
            if (*x.orig) continue;
            if (MH_CreateHook(vt1[x.slot], x.det, x.orig) != MH_OK) { *x.orig = nullptr; continue; }
            if (MH_EnableHook(vt1[x.slot]) != MH_OK) { MH_RemoveHook(vt1[x.slot]); *x.orig = nullptr; continue; }
        }
        khxh_c1 = g_orig_copy_region1 && g_orig_update_subresource1;
        khxh_ctx1->Release();
    }
    g_rp_cbreuse_on.store(khxh_c1 && g_orig_copy_region && g_orig_copy_structure_count &&
                          g_orig_execute_command_list,   // A command list writes too: its hook drops the images.
                          std::memory_order_relaxed);
}
// KH_PIP_FX_COPY: the engine copies the PIP colour target out with CopyResource (mid-pass for its translucents, and
// after the last draw for its post pass, which overwrites the target from that copy). The passes run here, before the
// copy, so the copy carries them; the first copy of a cycle is the engine's opaque boundary. RT0 is compared by
// resource, not by view.
static void STDMETHODCALLTYPE hooked_copyresource(ID3D11DeviceContext* self, ID3D11Resource* dst, ID3D11Resource* src) {
    // KH_RP_WRITE_SPLIT: ahead of this hook's profile scope - a split's own copy enters this hook (as ours).
    kh_rp_on_write(self, dst, false);
    {
    KH_PROF_SCOPE(KHP_HOOK_COPYRES);   // KH_PROF_HOOKS: ours, up to the driver's call.
    try { kh_rp_cb_forget(dst); } catch (...) {}   // KH_REPLAY_CBREUSE.
    try { kh_psb_zb_dst(self, dst); } catch (...) { kh_hook_except(); }   // KH_PSB_ZB: a copy into the main depth.
    try {   // KH_VOL_REPLAY: a copy into the volume buffer inside a pass.
        if (kh_rp_engine_call(self) && dst && g_svs_vol_src && static_cast<void*>(dst) == static_cast<void*>(g_svs_vol_src))
            kh_rp_foreign(6);
    } catch (...) {}
    try {
    if (src && g_pip.on && !g_pip.fx_done && g_pip.opaques >= KH_PIP_MIN_OPAQUES &&
        self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        reorder_on_render_thread() && !g_ro.in_injection &&
        !g_kh_flush_active.load(std::memory_order_relaxed) &&
        g_kh_track_wanted.load(std::memory_order_relaxed)) {
        ID3D11RenderTargetView* khcr_rtv = nullptr;
        self->OMGetRenderTargets(1, &khcr_rtv, nullptr);
        if (khcr_rtv) {
            ID3D11Resource* khcr_res = nullptr;
            khcr_rtv->GetResource(&khcr_res);
            khcr_rtv->Release();
            if (khcr_res) {
                const bool khcr_pip = khcr_res == src;
                khcr_res->Release();
                if (khcr_pip) kh_pip_fx(self);
            }
        }
    }
    } catch (...) { kh_hook_except(); }
    }
    g_orig_copyresource(self, dst, src);
}

static void STDMETHODCALLTYPE hooked_updatesubresource(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub, const D3D11_BOX* dst_box, const void* data, UINT row_pitch, UINT depth_pitch) {
    KH_PROF_SCOPE(KHP_HOOK_UPDSUB);   // KH_PROF_HOOKS: the whole hook (the driver's share: drvUpdSub).
    try { kh_rp_cb_forget(res); } catch (...) {}   // KH_REPLAY_CBREUSE.
    try { kh_psb_zb_dst(self, res); } catch (...) { kh_hook_except(); }   // KH_PSB_ZB: refused by D3D11 on a depth.
    kh_rp_on_write(self, res, false);   // KH_RP_WRITE_SPLIT.
    bool khus_cap = false;   // KH_UPLOAD_LAZY: this call captured.
    try {
    if (sub == 0 && !dst_box && data &&
        kh_upload_hook_wanted(self, res)) {
        const uint32_t bytes = proj_upload_byte_width(res);

        if (bytes != 0) {
            uint32_t khus_n = bytes;
            khus_cap = true;
            const void* khus_d = kh_upload_capture(res, data, khus_n);   // KH_DL_CPU.
            if (g_prof_up_on) kh_fun(KHF_UPDSUB, 1u);   // KH_PROF_FUNNEL.
            kh_upload_scan(res, khus_d, khus_n);
            KH_PROF_SUB(KHP_SC_RPIMG, g_prof_up_on, g_prof_up_t);   // KH_PROF_UPLOAD.
            kh_rp_img_note(res, khus_d, khus_n, self);   // KH_REPLAY_CPUCB: the caller's data is the whole buffer.
        }
    }

    } catch (...) { kh_hook_except(); }
    if (khus_cap) kh_upload_capture_end();   // Before the caller's data is returned.
    const uint64_t khdd_t0 = kh_prof_drv_t0(khps_scope_KHP_HOOK_UPDSUB);   // KH_PROF_DRV.
    g_orig_updatesubresource(self, res, sub, dst_box, data, row_pitch, depth_pitch);
    kh_prof_drv(KHP_DRV_UPDSUB, khdd_t0);
}
}