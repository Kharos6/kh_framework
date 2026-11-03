#pragma once

// The shared head (rendering_integration.hpp): the pipeline state backups (StateBackup, OmSave, OmSlot0,
// PredAside) and the glow / anamorphic binds. Never include it on its own.
namespace RenderIntegration {
// The ring binds the same buffer every slice with only firstConstant moving - the shape the D3D11.1 docs warn about
// under command-list emulation, where an offset-only SetConstantBuffers1 may not apply and a null bind between two
// Set1 calls is the workaround. The trigger is a deferred context on a driver without native command lists; every
// upload here uses the immediate context.
inline ID3D11DeviceContext1* as_ctx1(ID3D11DeviceContext* ctx);

// Pipeline state snapshot / restore (only what we touch). Constant buffers are per-stage: b0/b1 of both VS and PS.
// Restores itself if the pass that captured it unwinds before its explicit restore (an exception inside a hook or
// the flush): the host's pipeline state is never left as ours, and no captured reference leaks. The explicit
// restore() is the normal path; the destructor is the exception path only. That holds for this struct, OmSave,
// PredAside and RpSave (RP_UNWIND); the hand-rolled brackets (mask_cast_engine, GlowSave,
// sun_pf_convert) restore on their explicit path alone (the predicate mask_cast_engine sets aside -
// PRED_ASIDE - and the OmSave the other two hold excepted).
struct StateBackup {
    ID3D11DeviceContext*     ctx_saved = nullptr;   // The context of the capture (the destructor's).
    bool                     armed = false;         // Captured and not yet restored.
    ID3D11InputLayout*       input_layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    // Two IA vertex slots: the instance stream binds slot 1. A slot we touch is a slot we save; widen in the same
    // edit that binds a third.
    ID3D11Buffer*            vb[2] = { nullptr, nullptr };
    UINT                     vb_stride[2] = { 0, 0 }, vb_offset[2] = { 0, 0 };
    ID3D11Buffer*            ib = nullptr;   // MESH_INDEXED: every mesh pass binds an IB.
    DXGI_FORMAT              ib_fmt = DXGI_FORMAT_UNKNOWN;
    UINT                     ib_offset = 0;
    ID3D11VertexShader*      vs = nullptr;
    ID3D11ClassInstance*     vs_insts[8] = {};
    UINT                     vs_inst_count = 8;
    ID3D11PixelShader*       ps = nullptr;
    ID3D11ClassInstance*     ps_insts[8] = {};
    UINT                     ps_inst_count = 8;
    ID3D11GeometryShader*    gs = nullptr;
    ID3D11HullShader*        hs = nullptr;
    ID3D11DomainShader*      ds = nullptr;
    // Our passes bind b0 and b1. The capture is offset-aware through context1: the engine binds its b0 pool as
    // windows, and a plain restore would hand each window back at offset zero. Widen these arrays and the offset
    // pairs in the same edit that re-arms a bind. b2-b4 are declared by every unit (CBEngView*) and read as the
    // engine left them: no pass under a StateBackup ever Sets them - their writers bracket themselves: vmir_begin
    // runs in the draw hook outside any backup with its own save / restore pair (g_vmir.sv_b2), and the replay's
    // draws (rp_issue_mir, rp_replay_b) run under RpSave, which holds b0 - b13. No pass of ours sets compute
    // state (MIR_DRAW: the b2 patch is a draw). A pass that Sets b2-b4 or CS state under a backup widens this
    // capture in the same edit.
    ID3D11Buffer*            vs_cbs[2] = { nullptr, nullptr };
    ID3D11Buffer*            ps_cbs[2] = { nullptr, nullptr };
    UINT                     vs_cb_first[2] = { 0, 0 }, vs_cb_num[2] = { 0, 0 };
    UINT                     ps_cb_first[2] = { 0, 0 }, ps_cb_num[2] = { 0, 0 };
    bool                     cb_offsets = false;
    // t19 is in the saved range on purpose: the effect unit binds a LUT there and never nulls it. Beyond the mesh
    // set: t20 far pyramid, t21 / t22 shadow pre / post, t24 volume stencil, t25-t27 hero / mid / outer sun maps,
    // t28 mirror stencil, t29-t31 their pyramids, t32 far band, t35 cast occupancy, t36 DLS array, t37 dlsw mask
    // (the DLS world pass) or the near-plane marker (the effect chain, NEARZ_MARK), t38 material table, t23 the
    // volume copy's depth, t40 the light ring, t42 SPECCOLOR, t43-t48 the USER_TEX pages (a user effect's too:
    // FX_TEX), t33 the VOL_FOOT mask (t39 free at the pixel stage - it is the object records' VS slot,
    // vs_srv39 below; t34 and t41 are the replay merges' - PSReplayMerge / PSReplayMergeMir, which bind and null
    // them, with t49-t51, under their own RpSave (t0-t127) - and the effect chain's VM_SEE snapshot /
    // transmittance, bound by fx_chain_run under the flush's backup; SSAO binds t0-t3 under its own StateBackup).
    // t53-t69: the WSEE record (the WSEE note's REGISTERS), bound by fx_chain_run under the flush's backup
    // and by the sort scope.
    ID3D11ShaderResourceView* ps_srvs[70] = {};   // t0..t69 (t53 - t69 the WSEE record: the note's REGISTERS).
    ID3D11ShaderResourceView* vs_srv39 = nullptr;   // OBJBUF: the object record buffer's VS slot.
    ID3D11ShaderResourceView* vs_srv52 = nullptr;   // SKIN_POOL: the Sk twins' chunk (inst_sk_bind).
    ID3D11SamplerState*      ps_samps[2] = {};   // s0 material, s1 the shadow Gather (SHADOW_GATHER).
    ID3D11DepthStencilState* dss = nullptr;
    UINT                     stencil_ref = 0;
    ID3D11BlendState*        blend = nullptr;
    FLOAT                    blend_factor[4] = {};
    UINT                     sample_mask = 0xFFFFFFFF;
    ID3D11RasterizerState*   rasterizer = nullptr;
    UINT                     scissor_n = 0;   // The engine's scissor rects, restored verbatim.
    D3D11_RECT               scissor[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    // The engine's viewports, restored like the scissors. One difference: a capture that found none bound restores
    // none rather than setting zero of them, so ours would survive that unwind - the engine has at least one bound
    // at every site we capture from. A pass's own viewport save stays (some hand the engine's back mid-pass) and
    // must be taken with the captured viewports bound - this restore runs last and would override it.
    UINT                     vp_n = 0;
    D3D11_VIEWPORT           vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    // A predicate set by the engine would predicate our draws too: cleared for the pass, handed back with its value.
    ID3D11Predicate*         predicate = nullptr;
    BOOL                     predicate_value = FALSE;

    StateBackup() = default;
    StateBackup(const StateBackup&) = delete;
    StateBackup& operator=(const StateBackup&) = delete;
    ~StateBackup() { if (armed && ctx_saved) restore(ctx_saved); }
    void capture(ID3D11DeviceContext* ctx) {
        ctx_saved = ctx;
        armed = true;
        ctx->IAGetInputLayout(&input_layout);
        ctx->IAGetPrimitiveTopology(&topology);
        ctx->IAGetVertexBuffers(0, 2, vb, vb_stride, vb_offset);
        ctx->IAGetIndexBuffer(&ib, &ib_fmt, &ib_offset);
        ctx->VSGetShader(&vs, vs_insts, &vs_inst_count);
        ctx->PSGetShader(&ps, ps_insts, &ps_inst_count);
        // A count past eight (more class instances than the engine is known to use) is clamped to the arrays.
        if (vs_inst_count > _countof(vs_insts)) vs_inst_count = _countof(vs_insts);
        if (ps_inst_count > _countof(ps_insts)) ps_inst_count = _countof(ps_insts);
        ctx->GSGetShader(&gs, nullptr, nullptr);
        ctx->HSGetShader(&hs, nullptr, nullptr);
        ctx->DSGetShader(&ds, nullptr, nullptr);
        // Offset-aware when the driver offsets and the context is 1.1; the plain pair otherwise, never both
        // (g_cb_offsetting: a driver without offsetting drops Set1, so a restore through it would leave our buffer
        // bound).
        ID3D11DeviceContext1* c1 = g_cb_offsetting ? as_ctx1(ctx) : nullptr;

        if (c1) {
            c1->VSGetConstantBuffers1(0, 2, vs_cbs, vs_cb_first, vs_cb_num);
            c1->PSGetConstantBuffers1(0, 2, ps_cbs, ps_cb_first, ps_cb_num);
            cb_offsets = true;
        } else {
            ctx->VSGetConstantBuffers(0, 2, vs_cbs);
            ctx->PSGetConstantBuffers(0, 2, ps_cbs);
            cb_offsets = false;
        }
        ctx->PSGetShaderResources(0, _countof(ps_srvs), ps_srvs);
        ctx->VSGetShaderResources(39, 1, &vs_srv39);   // OBJBUF.
        ctx->VSGetShaderResources(52, 1, &vs_srv52);   // SKIN_POOL.
        ctx->PSGetSamplers(0, _countof(ps_samps), ps_samps);
        ctx->OMGetDepthStencilState(&dss, &stencil_ref);
        ctx->OMGetBlendState(&blend, blend_factor, &sample_mask);
        ctx->RSGetState(&rasterizer);
        scissor_n = 0;
        ctx->RSGetScissorRects(&scissor_n, nullptr);
        if (scissor_n > _countof(scissor)) scissor_n = _countof(scissor);
        if (scissor_n) ctx->RSGetScissorRects(&scissor_n, scissor);
        vp_n = _countof(vp);
        ctx->RSGetViewports(&vp_n, vp);
        if (vp_n > _countof(vp)) vp_n = _countof(vp);
        ctx->GetPredication(&predicate, &predicate_value);
        if (predicate) ctx->SetPredication(nullptr, FALSE);
    }

    void restore(ID3D11DeviceContext* ctx) {
        ctx->IASetInputLayout(input_layout);
        ctx->IASetPrimitiveTopology(topology);
        ctx->IASetVertexBuffers(0, 2, vb, vb_stride, vb_offset);
        ctx->IASetIndexBuffer(ib, ib_fmt, ib_offset);
        ctx->VSSetShader(vs, vs_inst_count ? vs_insts : nullptr, vs_inst_count);
        ctx->PSSetShader(ps, ps_inst_count ? ps_insts : nullptr, ps_inst_count);
        // CLASS_INST: the GS / HS / DS come back without class instances (the capture takes none; the VS / PS
        // carry eight) - a shader of those stages the engine bound with interfaces would then not be its own.
        // Accepted: the engine is not known to use dynamic linkage at any stage; wsee_re_save, the one bracket
        // that tests for them, refuses on the pixel shader's rather than set it back without them.
        ctx->GSSetShader(gs, nullptr, 0);
        ctx->HSSetShader(hs, nullptr, 0);
        ctx->DSSetShader(ds, nullptr, 0);
        // Offset-aware restore, one slot at a time: a slot captured with a window goes back through Set1; an empty
        // slot, or a capture without context1, takes the plain setter.
        {
            ID3D11DeviceContext1* c1 = cb_offsets ? as_ctx1(ctx) : nullptr;

            for (UINT sb_i = 0; sb_i < 2; ++sb_i) {
                if (c1 && vs_cbs[sb_i] && vs_cb_num[sb_i] > 0) {
                    c1->VSSetConstantBuffers1(sb_i, 1, &vs_cbs[sb_i], &vs_cb_first[sb_i], &vs_cb_num[sb_i]);
                } else {
                    ctx->VSSetConstantBuffers(sb_i, 1, &vs_cbs[sb_i]);
                }

                if (c1 && ps_cbs[sb_i] && ps_cb_num[sb_i] > 0) {
                    c1->PSSetConstantBuffers1(sb_i, 1, &ps_cbs[sb_i], &ps_cb_first[sb_i], &ps_cb_num[sb_i]);
                } else {
                    ctx->PSSetConstantBuffers(sb_i, 1, &ps_cbs[sb_i]);
                }
            }
        }
        ctx->PSSetShaderResources(0, _countof(ps_srvs), ps_srvs);
        bc_forget();   // BIND_FILTER: t0 - t69 hold the capture's views again (ps_srvs: PSB_PHASE's t69).
        ctx->VSSetShaderResources(39, 1, &vs_srv39);   // OBJBUF.
        ctx->VSSetShaderResources(52, 1, &vs_srv52);   // SKIN_POOL.
        ctx->PSSetSamplers(0, _countof(ps_samps), ps_samps);
        ctx->OMSetDepthStencilState(dss, stencil_ref);
        ctx->OMSetBlendState(blend, blend_factor, sample_mask);
        ctx->RSSetState(rasterizer);
        ctx->RSSetScissorRects(scissor_n, scissor_n ? scissor : nullptr);
        if (vp_n > 0) ctx->RSSetViewports(vp_n, vp);
        if (predicate) ctx->SetPredication(predicate, predicate_value);
        armed = false;
        KH_SAFE_RELEASE(input_layout);
        KH_SAFE_RELEASE(vb[0]);
        KH_SAFE_RELEASE(vb[1]);
        KH_SAFE_RELEASE(ib);
        KH_SAFE_RELEASE(vs);
        for (UINT i = 0; i < vs_inst_count; ++i) KH_SAFE_RELEASE(vs_insts[i]);
        KH_SAFE_RELEASE(ps);
        for (UINT i = 0; i < ps_inst_count; ++i) KH_SAFE_RELEASE(ps_insts[i]);
        KH_SAFE_RELEASE(gs);
        KH_SAFE_RELEASE(hs);
        KH_SAFE_RELEASE(ds);
        KH_SAFE_RELEASE(vs_cbs[0]);
        KH_SAFE_RELEASE(vs_cbs[1]);
        KH_SAFE_RELEASE(ps_cbs[0]);
        KH_SAFE_RELEASE(ps_cbs[1]);
        for (UINT sb_i = 0; sb_i < _countof(ps_srvs); ++sb_i) KH_SAFE_RELEASE(ps_srvs[sb_i]);
        KH_SAFE_RELEASE(vs_srv39);
        KH_SAFE_RELEASE(vs_srv52);
        for (UINT j = 0; j < _countof(ps_samps); ++j) KH_SAFE_RELEASE(ps_samps[j]);
        KH_SAFE_RELEASE(dss);
        KH_SAFE_RELEASE(blend);
        KH_SAFE_RELEASE(rasterizer);
        KH_SAFE_RELEASE(predicate);
    }
};

// PRED_ASIDE: the engine's predicate set aside around a GPU command of ours that runs outside any StateBackup (a
// copy, a clear, a draw of ours in a hook or ahead of a pass's backup) and handed back with its value - a predicate
// the engine left bound skips predicated commands, ours included (UPLOAD_PRED's rule; StateBackup clears it for
// the passes it brackets). One GetPredication per begin; nothing is set when none is bound. The explicit end() is
// the normal path, the destructor the exception path (StateBackup's rule). Not for an engine draw re-issued at its
// own hook (vmsee_t_begin, wsee_post): those keep the engine's predicate on purpose, the one the draw itself
// took, so a draw it skipped is skipped in the record too.
struct PredAside {
    ID3D11DeviceContext* ctx = nullptr;
    ID3D11Predicate*     pred = nullptr;
    BOOL                 value = FALSE;
    PredAside() = default;
    PredAside(const PredAside&) = delete;
    PredAside& operator=(const PredAside&) = delete;
    ~PredAside() { end(); }
    void begin(ID3D11DeviceContext* pa_ctx) {
        ctx = pa_ctx;
        ctx->GetPredication(&pred, &value);
        if (pred) ctx->SetPredication(nullptr, FALSE);
    }
    void end() {   // Idempotent: a second call (the destructor's) finds nothing to hand back.
        if (!pred) return;
        ctx->SetPredication(pred, value);
        pred->Release();
        pred = nullptr;
    }
};

// OMSetRenderTargets(N, ..) nulls every slot it does not name, so a get/set pair carrying only slot 0 would drop an
// engine multi-target bind. Every flush-side target swap goes through this: full slot set in, full slot set out.
// The engine binds unordered-access views on the OM too (OMSetRenderTargetsAndUnorderedAccessViews is hooked), and
// a plain OMSetRenderTargets would drop them: the capture reads the UAVs behind the bound RTVs and the restore
// carries them back (counters kept). With no UAV bound, the set is the plain call. Self-restoring on unwind (see
// StateBackup); release() disarms.
struct OmSave {
    ID3D11RenderTargetView*    rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView*    dsv = nullptr;
    ID3D11UnorderedAccessView* uav[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    UINT                       uav_start = 0;   // = the bound RTV count; UAVs occupy the slots above it.
    UINT                       uav_n = 0;       // UAVs found (0 = the plain set).
    ID3D11DeviceContext*       ctx_saved = nullptr;
    bool                       armed = false;
    OmSave() = default;
    OmSave(const OmSave&) = delete;
    OmSave& operator=(const OmSave&) = delete;
    ~OmSave() { if (armed && ctx_saved) restore(ctx_saved); release(); }
    void capture(ID3D11DeviceContext* ctx) {
        ctx_saved = ctx;
        armed = true;
        ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, &dsv);
        uav_start = 0;
        uav_n = 0;
        for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) if (rtv[i]) uav_start = i + 1;
        if (uav_start < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) {
            // The Get writes from element 0 of the array it is handed, so the array is offset by the start slot to
            // keep slot i at uav[i] (set reads it there).
            ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, uav_start,
                                                           D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT - uav_start, uav + uav_start);
            for (UINT i = uav_start; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) if (uav[i]) uav_n = i + 1 - uav_start;
        }
    }
    void set(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* om_dsv) {
        if (uav_n == 0) {
            ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, om_dsv);
            return;
        }
        static const UINT keep[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u };
        ctx->OMSetRenderTargetsAndUnorderedAccessViews(uav_start, rtv, om_dsv, uav_start, uav_n, uav + uav_start, keep);
    }
    // Rebind the saved targets under a different depth view: only the DSV moves.
    void set_with_dsv(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* om_dsv) { set(ctx, om_dsv); }
    void restore(ID3D11DeviceContext* ctx) { set(ctx, dsv); }
    void release() {
        armed = false;
        for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
            KH_SAFE_RELEASE(rtv[i]);
            KH_SAFE_RELEASE(uav[i]);
        }
        KH_SAFE_RELEASE(dsv);
        uav_n = 0;
    }
};

// MESH_O1 - our mesh pixel shaders write a second output (SV_Target1, cb.hlsl KhMeshOut): the sorted tail's
// dual-source blend operand there, 0 on every other draw, where slot 1 is unbound and it is dropped. A route that
// draws them under the engine's binding opens this first: where the engine left a target past slot 0 or an OM UAV
// bound, the route's draws get its slot-0 target alone (the same depth view) and the engine's whole set comes back
// at the scope's end (the capture's destructor) - so nothing of ours lands in an engine target past slot 0. The
// common binding (one target, no UAV) is left as it is, at the cost of the capture's two reads.
struct OmSlot0 {
    OmSave save;
    void open(ID3D11DeviceContext* ctx) {
        save.capture(ctx);
        bool more = save.uav_n != 0;
        for (UINT k = 1; k < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++k) {
            if (save.rtv[k]) more = true;
        }
        if (!more) {
            save.release();
            return;
        }
        ctx->OMSetRenderTargets(1, &save.rtv[0], save.dsv);
    }
    // The engine's set back now (where open changed it) - for a route that drops its own hook exclusion
    // (g_ro.in_injection) by hand before its scope ends, so the hooks never see the restore as the engine's bind.
    void close(ID3D11DeviceContext* ctx) {
        if (save.armed) save.restore(ctx);
        save.release();
    }
};

// GLOW_PYR - one glow draw's t3 (the pyramid) and s2 (the anisotropic sampler), each put back as it was by
// unbind() or the scope's end (the SSGI resolve's FX_SAMP_S2 rule; StateBackup keeps s0 - s1 only).
struct GlowBind {
    ID3D11DeviceContext*      ctx = nullptr;
    ID3D11SamplerState*       s2 = nullptr;
    ID3D11ShaderResourceView* t3 = nullptr;
    GlowBind() = default;
    GlowBind(const GlowBind&) = delete;
    GlowBind& operator=(const GlowBind&) = delete;
    ~GlowBind() { unbind(); }
    void bind(ID3D11DeviceContext* gn_ctx, const GlowPyr& p) {
        if (ctx || !gn_ctx || !p.srv || !g_res.glow_sampler) return;
        ctx = gn_ctx;
        ctx->PSGetSamplers(2, 1, &s2);
        ctx->PSGetShaderResources(3, 1, &t3);
        ctx->PSSetSamplers(2, 1, &g_res.glow_sampler);
        ctx->PSSetShaderResources(3, 1, &p.srv);
    }
    void unbind() {
        if (!ctx) return;
        ctx->PSSetShaderResources(3, 1, &t3);
        ctx->PSSetSamplers(2, 1, &s2);
        KH_SAFE_RELEASE(t3);
        KH_SAFE_RELEASE(s2);
        ctx = nullptr;
    }
};

// ANA_PYR - one anamorphic draw's t3 / t6 (the even / odd atlases), each put back as it was by unbind() or the
// scope's end (GlowBind's rule). No sampler: the taps are Loads.
struct AnaBind {
    ID3D11DeviceContext*      ctx = nullptr;
    ID3D11ShaderResourceView* t3 = nullptr;
    ID3D11ShaderResourceView* t6 = nullptr;
    AnaBind() = default;
    AnaBind(const AnaBind&) = delete;
    AnaBind& operator=(const AnaBind&) = delete;
    ~AnaBind() { unbind(); }
    void bind(ID3D11DeviceContext* an_ctx, const AnaPyr& p) {
        if (ctx || !an_ctx || !p.srv[0] || !p.srv[1]) return;
        ctx = an_ctx;
        ctx->PSGetShaderResources(3, 1, &t3);
        ctx->PSGetShaderResources(6, 1, &t6);
        ctx->PSSetShaderResources(3, 1, &p.srv[0]);
        ctx->PSSetShaderResources(6, 1, &p.srv[1]);
    }
    void unbind() {
        if (!ctx) return;
        ctx->PSSetShaderResources(6, 1, &t6);
        ctx->PSSetShaderResources(3, 1, &t3);
        KH_SAFE_RELEASE(t6);
        KH_SAFE_RELEASE(t3);
        ctx = nullptr;
    }
};
}