#pragma once

// The injection half (rendering_integration_injection.hpp): the flush (flush_locked). Never include it on its own.
namespace RenderIntegration {


// rt (NO_PARK): true for the unparked flush - the render thread's, from its scene-resolve hook, and
// FX_LATE's chain-only flush; false for a park's (park_run: bootstrap, watchdog - its deferred mission destroy
// returns before any flush). The expiry erase stays the park's (flush_frame's census erases otherwise).
// SESSION_RESET: hoisted out of the function below so the session reset reaches it.
static bool g_flush_fx_arb_prev = false;

// ---- flush_locked's stages ------------------------------------------------------------------------------------------
// flush_locked (at the end of this file) reads as its stage list. The stages that share no state are free functions
// (flush_*, each takes what it reads); from the landing verdicts on, one flush's working state is a FlushPass and the
// stages are its methods. Every stage runs on the flush's thread: the render thread at its scene resolve, the game
// thread under a park (rt false), or - FX_LATE's chain-only run (rt true) - whichever thread issues the
// resolve that runs the owed chain (fx_late_gt: the game thread's, outside any park).

// Stage 1 - housekeeping (the scene flush only: a chain-only flush, FX_LATE, skips it).
inline void flush_housekeeping(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    // Land a pending publish, an import's or a generated mesh's (MESH_GEN_PUB, EA_FBX_NO_PARK: never parked; the
    // objects naming either wait out of the live scene until then, SCENE_UNPUB). This body runs either under a park
    // held end to end or on the render thread itself (which no park can then stall mid-body), so the publish and the VB
    // top-up share one serialized window. Ahead of every skip below on purpose: a frame the flush declines still lands
    // it.
    mesh_publish_land(dev);
    // LOD_BOUNDS (a backstop: SCENE_UNPUB syncs no such object): an object synced while its mesh was
    // unpublished was gridded at mesh 0's bounds (the id clamps); its cell radius is taken again once its own
    // every-level bounds read - whichever publish landed the id: the one above or the injection's
    // (mesh_publish_land), each of which publishes every id appended before it.
    if (g_scene_unpub_min < mesh_count()) {
        const uint32_t from = g_scene_unpub_min, to = mesh_count();
        uint32_t left = 0xFFFFFFFFu;   // The lowest id still unpublished.
        for (uint32_t s = 0; s < g_scene.objs.size(); ++s) {
            if (!g_scene.alive[s] || g_scene.cell[s] == SCENE_CELL_NONE) continue;
            const int m = g_scene.objs[s].mesh;
            if (m < 0 || static_cast<uint32_t>(m) < from) continue;
            if (static_cast<uint32_t>(m) >= to) {
                if (static_cast<uint32_t>(m) < left) left = static_cast<uint32_t>(m);
                continue;
            }
            scene_grid_remove(s);
            scene_grid_insert(s, g_scene.objs[s]);
        }
        g_scene_unpub_min = left;
    }
    mesh_grave_sweep();   // MESH_FREE: the release wall drains here.
    mesh_gc();
    material_gc();   // MAT_POOL.
    tex_cache_gc();   // TEX_GC.
    // NO_PARK_THM - the heightfield upload belongs to the frame's only
    // flush; the flag is the atomic handoff thm_upload owns.
    if (g_thm.dirty.load(std::memory_order_relaxed)) {
        std::string err = thm_upload(dev);
        ensure_ok("terrain heightmap", err);
    }
    // CLOTH: create, upload and retire the per-instance vertex buffers (in this block for the reason above).
    // UPLOAD_PRED: no StateBackup stands here, and the uploads make GPU writes D3D11 predicates - the skin pool's
    // trim / grow copies, a first install's UpdateSubresource into a stream-output buffer, the vertex stage's prime
    // copy (uvs_copy_src). One the engine left bound could skip them, and a pool's old chunk is released right
    // after its copy: every pooled binding of the mesh would draw unwritten bytes. Set aside for the three and put
    // back, as skin_gpu_move does for its own copy (redundant, harmless). Nothing in them relies on it:
    // their draws (uvs_step's) are bracketed by a StateBackup, which clears it anyway.
    // PRED_ASIDE: through the struct - a throw inside the three hands the predicate back and releases the
    // reference on the unwind (a hand-rolled pair leaves it cleared and leaks the reference).
    PredAside pa;
    if (ctx) pa.begin(ctx);
    cloth_upload(ctx, dev);
    skin_upload(ctx, dev);   // SKEL: after the cloth, which empties the slot tables first.
    uvs_upload(ctx, dev);    // KH_USER_VS: last - its source is whatever those two left in the slot.
    pa.end();   // UPLOAD_PRED: handed back (GetPredication's reference released).
}

// DL_FALLBACK_HARVEST: a cycle no accepted injection harvested (a rescue or anomalous cycle, a far-phase
// refusal, an injection that returned early, no trigger at all; a trigger with nothing of ours to draw harvests
// there, DL_TRIGGER_HARVEST) still merges its span here, before this flush's late draws fill their lights. The
// same harvest the injection runs - presence, TTL and authoring unchanged - so a light lives as long as on an
// accepted cycle (one harvest longer at most, where the next injection bridges this span:
// DL_SPAN_BRIDGE); without it the pool aged past DL_POOL_STALE_MS and every mesh lost every dynamic light
// until an injection landed again. Render thread only: g_dl is render-thread state.
inline void flush_dl_fallback_harvest(ID3D11DeviceContext* ctx, bool rt, bool chain_only) {
    if (rt && !chain_only && dl_cpu_wanted() &&   // DLF_DEMAND: a lit mesh / caster or the fog.
        g_dlcap.harvest_cycle != g_topo_cycles) {
        g_dlcap.harvest_cycle = g_topo_cycles;
        dynlights_harvest_arena(ctx, false);   // Resolve to resolve: no bridge.
        g_dlcap.harvest_flush_last = true;        // DL_SPAN_BRIDGE: the next injection's span is short.
        stat_inc(g_stats.dl_harvests_flush);   // DL_DIAG.
    }
}

// Stage 2 - the frame publications and stamps every scene flush makes, ahead of its entry tests.
inline void flush_publish_frame(bool chain_only) {
    // Consumers of these plain stores run on the storing thread or are stalled by a park, so they never see a store
    // mid-write. Publish this frame's staged world lighting.
    if (!chain_only) publish_world_lighting();   // FX_LATE: once, by the scene flush.

    // The lighting block AND the sun direction are published at the main depth
    // clear, not here: nothing on the flush arbitrates either.

    // Flush stamps: the serial keys the miss latch; the opaque store lifts the no-flush sentinel the clear re-arms
    // (rescue_window_open tests only that - the count itself is compared nowhere).
    if (!chain_only) {   // FX_LATE: the stamps are the scene flush's.
        g_cc_flush_serial.fetch_add(1, std::memory_order_relaxed);
        g_cc_flush_opaques.store(g_ro.opaque_draws, std::memory_order_relaxed);
    }
}

// NO_PARK_LATE: a cycle the injection did not take draws its meshes late in this flush, so this is the cycle's
// last consume point for their transforms (g_attach_final_step). A park's late draw is stepped by park_run.
// Placed ahead of the entry tests, so a declined flush still advances the transforms the game thread reads next
// (cloth sync, collider gather).
inline void flush_step_late(ID3D11DeviceContext* ctx, bool rt, bool chain_only) {
    if (rt && !g_ro.injected && !chain_only) {
        {
            FinalStepScope fl_final;
            attach_step(ctx);
        }
        uvs_step(ctx);   // KH_USER_VS: this cycle's late draws, when no injection stepped it.
        // SUN_RESTEP, the injection's twin: this flush's late meshes
        // self-shadow from the map the cast fire built before this step moved them.
        if (g_sun_restep_cycle == g_topo_cycles) g_sun_maps.sun_map_rendered_frame = false;
    }
}

// Stage 3 - the main-depth gate: the bound DSV must be the main scene's (adopted here). false = nothing of ours this
// flush (no DSV, or another pass's depth: a park re-parks on g_flush_wrong_pass).
inline bool flush_adopt_main_depth(ID3D11DeviceContext* ctx, bool rt) {
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &dsv);

    if (!dsv) return false;

    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    dsv->Release();

    if (!res) return false;

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    res->Release();

    if (FAILED(hr) || !tex) return false;

    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    void* identity = static_cast<void*>(tex);
    tex->Release();

    const bool adopt = (g_main_depth_identity == nullptr) ||
                       (static_cast<uint64_t>(td.Width) * td.Height >
                        static_cast<uint64_t>(g_main_depth_w) * g_main_depth_h) ||
                       (g_wrong_pass_streak >= WRONG_PASS_READOPT);

    if (adopt) {
        g_main_depth_identity = identity;
        g_main_depth_w = td.Width;
        g_main_depth_h = td.Height;
        g_wrong_pass_streak = 0;
    } else if (identity != g_main_depth_identity) {
        g_wrong_pass_streak++;
        // WRONG_PASS_REPARK: nothing of ours this park - the caller re-parks. The park is the only caller with a
        // re-park loop, so the render-thread path leaves the flag alone rather than store across threads for a
        // reader it does not have; its own retry is the next cycle's resolve.
        if (!rt) g_flush_wrong_pass = true;
        return false;
    } else {
        g_wrong_pass_streak = 0;
    }
    // td is the adopted-or-matched main scene depth, read in every flush with work at any FSAA state.
    g_scene_depth_samples.store(td.SampleDesc.Count, std::memory_order_relaxed);
    // This site sees every transition at any FSAA, in every flush (either thread's, chain-only runs too). The
    // wipe compares against g_res.depth_sample_count; the store after it keeps both trigger sites deduped.
    msaa_toggle_wipe(td.SampleDesc.Count);
    g_res.depth_sample_count = td.SampleDesc.Count;
    return true;
}

// Stage 4 - the device resources (a deferred ensure is not a failed one: the frame still ends, under the white
// placeholder while the shaders compile). false = no flush this frame.
inline bool flush_ensure_resources(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    const std::string res_err = ensure_resources(dev);
    // The hold is armed on the deferred branch and released here, on the first frame the gate passes.
    const bool hold_now = res_err == SHADERS_COMPILING;
    if (hold_now != g_acq_hold.load(std::memory_order_relaxed)) {
        g_acq_hold.store(hold_now, std::memory_order_relaxed);
        // Everything the chain accumulated while the flush was down belongs to an era with no reference behind it,
        // so the release hands the first live frame a cold chain, not a poisoned one.
        if (!hold_now) {
            g_ls.view_src_valid = false;
            g_ls.vc_n = 0;
            g_ls.pub_rej_streak = 0;
            g_ls.frame_view_valid = false;
            g_ls.prev_view_valid = false;
            g_ring_best.valid = false;
            g_view_drop.relock_wide_cycle = 0;
            g_ls.wipe_deferred = true;

            for (int b = 0; b < 8; ++b) {
                g_ls.band[b].valid = false;
                g_ls.band[b].pending_view = false;
            }
        }
    }
    if (!ensure_ok("flush resources", res_err)) {
        if (res_err == SHADERS_COMPILING) {
            skin_cull_catchup(ctx);   // SKIN_CULL: the placeholder draws every visible mesh.
            white_draw(dev, ctx);
        }
        return false;
    }
    return true;
}

// The rain lens's tracker (g_rain.vel: the camera's smoothed view-space velocity; g_rain.phase: its integrated clock -
// a RainLens pass's fxParams1, fill_fx_params_cb), from the flush's camera: run between FlushPass::adopt_camera and
// FlushPass::measure_viewport.
inline void flush_track_rain(const float cam[3], const RVExtBridge::ProjectionViewTransform& pv) {
    const uint64_t rm_now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    if (g_rain.prev_valid) {
        const float dt = static_cast<float>(rm_now - g_rain.prev_us) * 1e-6f;

        if (dt > 0.0005f && dt < 0.5f) {
            const float dx = cam[0] - g_rain.prev_cam[0];
            const float dy = cam[1] - g_rain.prev_cam[1];
            const float dz = cam[2] - g_rain.prev_cam[2];
            const float step2 = dx * dx + dy * dy + dz * dz;

            if (step2 < 100.0f * 100.0f) {
                const float a = 1.0f - expf(-dt / 0.15f);

                for (int k = 0; k < 3; ++k) {
                    const float v = (dx * pv.view[0][k] +
                                     dy * pv.view[1][k] +
                                     dz * pv.view[2][k]) / dt;
                    g_rain.vel[k] += (v - g_rain.vel[k]) * a;
                }
            } else {
                g_rain.vel[0] = g_rain.vel[1] = g_rain.vel[2] = 0.0f;
            }

            const float fk = fminf(fmaxf(g_rain.vel[2], 0.0f) / 20.0f, 1.0f);
            const float ud = fminf(fmaxf(-g_rain.vel[1] * 0.06f, 0.0f), 1.0f);
            g_rain.phase += static_cast<double>(dt) *
                            (1.0 + static_cast<double>(fk) * 2.2 + static_cast<double>(ud));
        }
    }

    g_rain.prev_cam[0] = cam[0];
    g_rain.prev_cam[1] = cam[1];
    g_rain.prev_cam[2] = cam[2];
    g_rain.prev_us = rm_now;
    g_rain.prev_valid = true;
}

// FlushPass - one run of flush_locked from the landing verdicts on: the verdicts, the camera, the draw list and the
// draw-loop trackers its stages hand each other. Lives on the flush's stack for one call; owned by the flush's thread
// (the stage list's note above). The members keep the names the locals had (the comments across the renderer cite
// them), grouped by the stage whose code declared the local (a later stage may set it first); a member with no
// initialiser is written before any read, as the local was.
struct FlushPass {
    // ---- inputs (flush_locked's arguments and its per-call scratch, kept across calls for capacity only) ----
    ID3D11Device*        dev;
    ID3D11DeviceContext* ctx;
    bool                rt;           // Not a park's flush (NO_PARK; the stage list's note).
    bool                chain_only;   // FX_LATE's chain-only flush.
    std::vector<RenderObject>& meshes;      // This flush's late mesh draws (the walk's copies).
    std::vector<std::pair<uint64_t, RenderObject>>& fullscreen;   // Key = creation seq.
    std::vector<uint32_t>& order;       // The draw order over meshes (order_draws).
    InstPlan&          inst_plan;     // Buckets, flush edition.

    FlushPass(ID3D11Device* fp_dev, ID3D11DeviceContext* fp_ctx, bool fp_rt, bool fp_chain_only,
              std::vector<RenderObject>& fp_meshes, std::vector<std::pair<uint64_t, RenderObject>>& fp_fullscreen,
              std::vector<uint32_t>& fp_order, InstPlan& fp_inst_plan)
        : dev(fp_dev), ctx(fp_ctx), rt(fp_rt), chain_only(fp_chain_only), meshes(fp_meshes),
          fullscreen(fp_fullscreen), order(fp_order), inst_plan(fp_inst_plan) {}
    FlushPass(const FlushPass&) = delete;
    FlushPass& operator=(const FlushPass&) = delete;

    // ---- judge_landing: did the injection land in the cycle these late draws go into? ----
    float snapshot_now = 0.0f;                 // The effect clock the walk's lifetimes are judged at.
    bool injected_since_last_flush = false;   // NO_PARK_LANDED: the landing witness.
    bool cycle_landed = false;            // FLUSH_CYCLE_WITNESS: this cycle's own flag.
    bool comp_healthy = false;                // The flush stands down for the composite meshes.
    // ---- collect_scene: the walk ----
    uint32_t comp_eligible = 0;
    bool any_lit = false;   // sun-map carry: a lit mesh exists this frame.
    bool any_mesh = false;   // Any non-fullscreen consumer for the occlusion snapshot exists this frame.
    // ---- decide_late_work ----
    bool infront_tpl = false;             // INFRONT_TPL: the view-model slice is owed this cycle's template.
    bool nothing_late = false;            // No mesh and no fullscreen pass to draw late.
    // ---- pick_projection: the camera and the depth pair the late draws encode with ----
    RVExtBridge::ProjectionViewTransform pv = {};
    float view_proj[4][4];
    // ---- build_inverse: the reconstruction frame of the depth readers ----
    float inv_view_proj[4][4] = {};
    float fx_cam[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool has_inverse = false;
    LocalCull lcull;
    // ---- adopt_camera / measure_viewport ----
    float cam[3];
    bool engcam = false;                  // ENGCAM: cam is the engine's own camera.
    float screen_w = 0, screen_h = 0;
    // ---- prepare_effects: capability gating ----
    float now = 0.0f;
    double now_d = 0.0;
    bool  cap_defer = false;              // CAP_DEFER: the scene capture is left to the chain.
    bool effects_ready = false;
    bool depth_fx_ready = false;
    // ---- draw: the pass state (bind_pass_state) ----
    OmSave* om_p = nullptr;              // draw()'s OM save, which dsv_swap moves the depth view through.
    bool om_captured = false;
    bool dsv_ro = false;
    UINT stride = sizeof(MeshVertex), offset = 0;
    VbRef bound_vb;   // CLOTH: seeded by bind_pass_state (SKIN_POOL: the pair).
    ID3D11ShaderResourceView* ps_srvs[2] = {};   // t0 scene capture / t1 depth pair (bind_pass_state).
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ID3D11RasterizerState* bound_rs = nullptr;
    // Set just before the mesh pass (the count), cleared by the loop's first perceptual capture failing
    // (FX_CAP_LAZY).
    bool perceptual = false;
    bool mm_cap = false;   // BLEND_MINMAX: this object's own capture is bound at t3 (set per object);
    bool mm_rep = false;   // and it draws with a replacing blend (a multisampled target).
    // ---- judge_far_arb ----
    bool fx_arb_this = false;
    bool fx_arb_frame = false;
    // ---- build_frame_cb ----
    ConstantData cbf = {};
    bool rebase_on = false;
    bool frame_ok = false;
    // ---- judge_near_gap ----
    float mesh_vp_lo = 0.0f, mesh_vp_hi = 0.0f;   // The mesh viewport's depth range (trigger or scene).
    float nz_near = 0.0f;
    bool nz_on = false;
    float nz_gap_lo = 0.0f;
    bool nearz_draw = false;   // per-mesh verdict (written by upload_cb).
    // Buckets, flush edition: the pass verdict (twins + object buffer present) every bucket decision on this pass
    // reads, and the slot-1 bind latch. Twin at the injection.
    bool inst_on = false;
    bool inst_vb_bound = false;
    // Hoisted per-object CB: the textured per-submesh path refills the mat lanes and uploads after upload_cb returns
    // (TX_ONE_UPLOAD: a textured object's only upload), so the object slice must outlive the call.
    ConstantData obj_cbd = {};
    // GLOW_PYR, effect meshes: their source is the scene capture at t0, which the loop's re-captures rewrite
    // (FX_CAP_ORDER; cap_gen counts the captures this flush takes after the effects' own). A glow mesh
    // builds glow_pyr[0] once per capture and threshold (the key below) and upload_cb arms its lanes
    // (glow_draw); the chain after the meshes builds its own.
    uint64_t cap_gen = 0;
    uint64_t glow_gen = ~0ull;
    bool glow_b = false, glow_ok = false;
    float glow_t = 0.0f;
    const GlowPyr* glow_draw = nullptr;
    // ANA_PYR, effect meshes: glow_pyr[0]'s rule for anamorphic's one-axis pyramid, per axis - built once per
    // capture and threshold, its lanes armed by upload_cb (ana_draw).
    uint64_t ana_gen[2] = { ~0ull, ~0ull };
    bool ana_ok[2] = { false, false };
    float ana_t[2] = { 0.0f, 0.0f };
    const AnaPyr* ana_draw = nullptr;
    ID3D11ShaderResourceView* utex[MAT_USER_MAPS] = {};   // FX_TEX: an effect mesh's set (upload_cb).
    bool t43 = false;   // FX_TEX_ALL: utex's pages are bound (the last effect mesh drawn had a set).
    // ---- build_visibility ----
    CullSet cull;
    // ---- order_draws ----
    uint32_t perc_count = 0;
    size_t def_n = 0;
    size_t solid_n = 0;                    // order[solid_n] is the translucent tail's first.
    bool ssao_done = false;
    // ---- prepare_draw_loop: the loop's bind trackers ----
    int ps_cat = -1;   // 0 solid / 1 effect / 2 arb / 3 textured / 4 textured arb
                           // (FLUSH_NEARZ).
    ID3D11VertexShader* bound_vs = nullptr;
    ID3D11InputLayout* bound_il = nullptr;
    ID3D11ShaderResourceView* bound_sk = nullptr;   // SKIN_POOL: VS t52 (inst_sk_bind).
    // FX_CAP_ORDER: the scene capture (g_res.scene_srv - t0 of the effect meshes, t3 of the perceptual
    // translucents) is the picture when it was taken, and a reader writes over the live target (an effect mesh
    // effect(capture) at its alpha, a perceptual translucent its mix opaque), erasing every draw of ours since then
    // under its silhouette. Two flags, cleared by every capture: cap_stale (a draw of ours since the capture) is
    // read before a perceptual translucent, cap_stale_ne (one that was not an effect mesh) before an effect mesh;
    // a stale flag takes the capture again first. An effect mesh over an effect mesh keeps the capture. The SSAO post
    // at the tail's start is a draw of ours (its apply darkens the solids).
    // FX_CAP_LAZY: cap_stale starts true; the effect meshes' capture is the setup's, taken above (cap_defer
    // is false whenever the flush has an effect mesh, so the chain's deferred copy is never theirs).
    bool cap_stale = true;
    bool cap_stale_ne = false;
    bool cap_t3 = false;   // FX_CAP_LAZY: a capture taken by this loop stands at t3 (any of the sites below).
    int srv_cat = -1;   // 0 = comp-depth t0, 1 = scene/depth pair, 2 = null t0 (no snapshot).
    int bound_bm = -1;   // Blend mode last set.
    ID3D11DepthStencilState* bound_dss = nullptr;
    UINT bound_ref = 0;   // SSAO_SEE_THROUGH: the stencil reference bound with bound_dss.
    UINT n_saved_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    D3D11_VIEWPORT mesh_vp = {};   // The mesh viewport (hoisted: FLUSH_NEARZ swaps against it per
                                  // Draw).
    SsaoPass ssao;
    D3D11_VIEWPORT nz_vp = {};
    int vp_bound = 0;
    // ---- prepare_wsee_sort ----
    int   ws_mode = 0;
    float ws_dec[4] = {};
    // Q, taken at the run's first part that composites (0 = not yet, 1 = taken, -1 = refused: the run's
    // composite modes take mode 1).
    int ws_q = 0;

    // ---- the stages, in flush_locked's order ----
    void judge_landing();
    void collect_scene();
    void render_shadow_maps();
    bool decide_late_work();          // false: nothing to draw or snapshot for.
    bool pick_projection();           // false: no usable matrices.
    void take_occlusion_snapshot();
    void build_inverse();
    void adopt_camera();
    void measure_viewport();
    void publish_frame_template();    // INFRONT_TPL: the template alone, nothing drawn.
    bool prepare_effects();           // false: everything dropped (and the template published if owed).
    void draw();                      // The mesh pass and the chain, under one StateBackup.
    // ---- draw()'s steps ----
    void bind_pass_state();
    void judge_far_arb();
    void build_frame_cb();
    void judge_near_gap();
    void build_visibility();
    void order_draws();
    void plan_buckets();
    void prepare_draw_loop();
    void prepare_wsee_sort();
    void draw_object(size_t ix);  // One iteration of the mesh loop.
    void finish_meshes();
    void run_chain();
    // ---- draw_object's steps ----
    void at_tail_start(size_t ix);
    void refresh_capture_for_perceptual(bool o_perc);
    void refresh_capture_for_effect(const RenderObject& o);
    void prepare_pyramids(const RenderObject& o, ID3D11PixelShader* ufx, bool upyr);
    void bind_object_shaders(const RenderObject& o, ID3D11PixelShader* ufx, int ps_want, bool tx_on);
    void bind_object_srvs(const RenderObject& o);
    ID3D11DepthStencilState* bind_object_blend_depth(const RenderObject& o);   // Returns the bound depth state.
    // ---- helpers the stages share (the lambdas the function body held) ----
    // Depth-sampling effects swap the read-only DSV in (draw()'s note at om).
    void dsv_swap(bool ro);
    // Asked by upload_cb for the object and by the bucket plan's key (inst_plan_build) for every candidate member.
    bool nz_verdict(const RenderObject& o);
    // defer non-null = build only (deferred chain draw - the caller uploads the returned slice at flush time; the
    // frame slice already went up at the pass build - or a textured object, whose slice draw_textured uploads per
    // submesh: TX_ONE_UPLOAD).
    bool upload_cb(const RenderObject& o, bool chain_pass, ConstantData* defer = nullptr);
    // The flush's one visibility truth (frustum only - occlusion culling is an injection concern: only there does the
    // pass-wide depth-write contract make a cull provably output-identical). The INST_RUN cut, the material ensure,
    // the plan's emit and the per-object loop all call vis; memoised per object.
    bool vis_calc(const RenderObject& o);
    bool vis(const RenderObject& o);
    static bool transl(const RenderObject& o);   // A translucent draw (the note at its definition).
    void vp_select(int want);   // 0 = the mesh viewport, 1 = the near-gap (prepare_draw_loop's note).
};

inline void FlushPass::judge_landing() {
    snapshot_now = effect_time_seconds();
    // A park's verdict: per flush_frame call, not per park (see g_flush_landed_this_frame).
    // NO_PARK_LANDED: the render-thread flush takes its own witness - a landing in the cycle it draws into.
    // g_flush_landed_this_frame is the game thread's verdict against its own previous call, which drifts from the
    // render thread's cycles; using it redrew composite meshes late over the engine's later draws, or adopted the
    // previous landing's view. A chain-only flush takes the same witness (its chain uses the view and encode chosen
    // from it), with the published cycle counter. A park's flush keeps the game-thread verdict.
    const uint64_t land_c = chain_only ? g_topo_cycles_pub.load(std::memory_order_relaxed) : g_topo_cycles;
    injected_since_last_flush = rt
                              ? g_composite_land_cycle == land_c
                              : g_flush_landed_this_frame;
    // FLUSH_CYCLE_WITNESS: whether the cycle these late draws go into has landed. The park stalls the render thread
    // inside that cycle, and the render-thread flush runs inside it, so the cycle's own flag (reset at its clear, set
    // at its landing) is definite here; the serial delta is not (it usually names the previous frame). A park landing
    // ahead of a healthy cycle's landing draws late and the injection draws again: identical overdraw.
    cycle_landed = g_ro.injected;
    // A frame that saw an anomalous cycle, or that never injected, gets its meshes drawn late here as well - identical
    // overdraw where the injection survived. The repaint term below was meant to add "or whose world redrew after the
    // injection" (fill-in where a foreign pass erased it), and it CANNOT fire: opaques_since_inject is incremented only
    // past reorder_pre_draw's return on g_ro.injected and is zeroed at the landing, so it is 0 whenever
    // cycle_landed is true, and when cycle_landed is false comp_healthy is already false without it. A later
    // partition is covered instead by the injection's own re-arm, which clears g_ro.injected: PART_NEAR before
    // drawing (it defers to the camera partition), the anomalous rescue after a draw (a later attempt lands)
    // (proj_try_window has none: see there). Counting opaques past the landing would make this term live, at
    // the price of a late redraw of every composite mesh on any frame with 32 opaques after the landing - measure that
    // before changing it.
    const bool repainted_since_inject = g_ro.opaques_since_inject >= 32;
    const bool anomaly_this_frame = g_ro.anomaly_seen;
    g_ro.anomaly_seen = false;
    // inj_healthy: the injection path is healthy and landed since the last flush; comp_healthy adds the cycle
    // witness and is the draw's verdict (may this flush stand down for the composite meshes). A park that lands ahead
    // of its cycle's landing draws late.
    const bool inj_healthy = composite_path_healthy() && injected_since_last_flush &&
                             !repainted_since_inject && !anomaly_this_frame;
    comp_healthy = inj_healthy && cycle_landed;
    // SKIN_CULL: a park draws under a view no step decided for, and may build the light maps - what was culled is
    // skinned first, before the walk below copies the boxes.
    if (!rt && !chain_only) skin_cull_catchup(ctx);
}

inline void FlushPass::collect_scene() {

    meshes.clear();
    fullscreen.clear();

    static std::vector<uint32_t> f_expired;   // SCENE: slots to erase after the walk.
    f_expired.clear();
    // WSEE: whether the injection's deferred copies joined meshes (appended after the walk).
    // WSEE_DEFER_FLAG: the tail sort below finds them by RenderObject::ws_dcopy - mesh_order permutes meshes,
    // so an index range over the appended copies named whichever objects sorted last (the depth-Off overlays, the
    // farthest translucents), drew those first and left the copies in the tail.
    bool wd_any = false;

    {
        scene_sync();   // SCENE: live walk; the copies below are of visible objects only.
        for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
            if (!g_scene.alive[i]) continue;
            const RenderObject& live = g_scene.objs[i];
            bool expired = false;
            const float env = lifetime_envelope(live, snapshot_now, expired);

            if (expired) {
                // The erase belongs to flush_frame's census (game thread).
                if (rt) continue;
                f_expired.push_back(i);
                continue;
            }
            // Sun-map demand: a visible lit receiver or any shadow-active object (casterOnly too).
            if ((obj_vis(live) && live.lit && !live.fullscreen &&   // SHOW_INFRONT.
                 live.effect != static_cast<int>(EffectId::Ssgi) &&   // MESH_SSGI: draws nothing.
                 live.effect != static_cast<int>(EffectId::DynLightFog) &&   // DLF: nor this.
                 (!obj_in_front(live) || g_vm_slice_live.load(std::memory_order_relaxed))) ||   // INFRONT.
                shadow_active(live)) any_lit = true;
            // INFRONT: the view-model slice draws it (infront_inject); it casts above. An effect mesh ignores
            // the flag (obj_in_front) and draws here.
            if (obj_in_front(live)) continue;

            if (obj_shown(live)) {   // SHOW_INFRONT: drawn by the main camera's passes.
                RenderObject o = live;
                o.color[3] *= env;   // Envelope = universal intensity.
                // FX_LATE: the scene flush drew the meshes. Every other visible mesh the injection does not own
                // (effect meshes, depth-Off, blended, fading, or any mesh on a cycle whose injection bailed) draws late
                // in this flush.
                if (chain_only && !o.fullscreen) continue;
                if (!o.fullscreen && fsaa_world_standdown()) { continue; }
                // MESH_SSGI: an effect mesh set to "ssgi" draws nothing. The gather is a scene-chain pass (its
                // grid factor, radiance seed and resolve exist only there); the PIP and the UI lane skip it too.
                if (!o.fullscreen && o.effect == static_cast<int>(EffectId::Ssgi)) continue;
                // DLF: an effect mesh set to dynamicLightFog likewise (its gather is the scene chain's).
                if (!o.fullscreen && o.effect == static_cast<int>(EffectId::DynLightFog)) continue;
                if (is_composite_eligible(o)) ++comp_eligible;
                if (!o.fullscreen) any_mesh = true;

                if (o.fullscreen) {
                    // Scene and both semantics run here (the pre-tonemap chain); only UI-mode passes are excluded.
                    if (!o.ui_only) fullscreen.emplace_back(o.seq, o);
                } else if (comp_healthy && is_composite_eligible(o)) {
                    // Composited meshes are drawn pre-translucent by the reorder hook; the flush stands down for them
                    // while injections are happening, blend submeshes included (the injection draws the translucent
                    // part in its own tail).
                    //
                    // A park that lands ahead of a healthy cycle's landing (comp_healthy false, inj_healthy true)
                    // draws every eligible mesh late, attached ones included, and the injection draws them again:
                    // identical overdraw for a mesh the render thread already sampled this cycle, and for one it has
                    // not, the game thread's sample under the injection's - one frame of motion apart, one frame. That
                    // is preferred to leaving the mesh to an injection that may bail (a frame with no draw at all), and
                    // the AO term stays single: the stencil mark is per pixel and per drawer, so whichever draw wins a
                    // pixel takes it once.
                } else {
                    if (is_composite_eligible(o)) {
                        const uint64_t ms_fs = g_cc_flush_serial.load(std::memory_order_relaxed);

                        if (g_ms_flush_serial != ms_fs) {
                            g_ms_flush_serial = ms_fs;
                            g_carry_pending_serial.store(ms_fs, std::memory_order_relaxed);
                        }
                    }
                    // An object the flush owns this frame with a blend material splits in two: its opaque part draws in
                    // place, its translucent part joins the sorted tail. Effect meshes keep their whole-object draw.
                    // MAT_PART_OFF: a depth-Off overlay splits too - both parts take dss_off (the loop's depth
                    // state names mode Off ahead of the part), and its glass sorts in the tail as a translucent
                    // overlay does; the opaque part first where both sort there (PART_ORDER).
                    if (o.effect == 0 && obj_has_blend(o)) {
                        meshes.push_back(o);
                        meshes.back().draw_part = 2;
                        meshes.push_back(o);
                        meshes.back().draw_part = 1;
                    } else {
                        meshes.push_back(o);
                    }
                }
            }
        }

        // WSEE: the injection's deferred translucent parts (g_wsee.defer_cycle's note), taken by this cycle's
        // first scene flush: where it stands down for the injection's meshes (comp_healthy), their copies join the
        // tail as draw_part 1 entries - each only while its slot still holds that object with that mesh and material
        // set (the live one keeps both alive through this flush's GC: mesh_gc, material_gc ran above) -;
        // where it draws them itself, their parts come with them (the split in the walk above) and the copies go.
        if (!chain_only && g_wsee.defer_cycle == g_topo_cycles) {
            if (comp_healthy) {
                for (const RenderObject& wd : g_wsee.defer_objs) {
                    if (wd.slot >= g_scene.objs.size() || !g_scene.alive[wd.slot]) continue;
                    const RenderObject& wl = g_scene.objs[wd.slot];
                    if (wl.seq != wd.seq || wl.mesh != wd.mesh ||
                        wl.materials != wd.materials) continue;
                    // A copy draws from its hold entry (wsee_hold_ref): a copy entry from the hold's bytes, an
                    // own entry (a static part, or one drawn from its mesh's buffer at the injection) from the mesh's
                    // own buffer at the injection's level. Void - the hold released or remade since (a switch to one
                    // sample, the device), or an own entry's mesh buffer gone - the copy is dropped.
                    if (wd.ws_hold != 0u && !wsee_hold_ref(wd)) continue;
                    meshes.push_back(wd);
                    wd_any = true;
                }
                if (wd_any) g_wsee.defer_taken = g_topo_cycles;   // Drawn once the loop runs (_drawn).
            }
            g_wsee.defer_cycle = ~0ull;
            g_wsee.defer_objs.clear();
        }

        // SKIN_CULL: the late draws are the copies above, under this flush's own view: a culled binding among
        // them (a flag changed since the step, or no landing) takes this stash's pose first, its copies the box.
        if (!chain_only) skin_cull_catchup(ctx, &meshes);
        if (chain_only) any_lit = false;   // FX_LATE: nothing to cast for; the scene flush prepared it.
        if (!rt && !f_expired.empty()) {   // The flush owns the erasure (the others skip expired). Park only.
            std::lock_guard<std::mutex> g(g_draw_list_mutex);
            for (uint32_t s : f_expired) {
                if (s >= g_scene_stage.size() || !g_scene_stage[s]) continue;
                auto it = g_draw_list.find(g_scene_handle[s]);
                if (it == g_draw_list.end() || it->second.slot != s) continue;
                scene_stage_remove(it->second);
                attach_drop(it->first);   // ATTACH.
                g_draw_list.erase(it);
            }
        }
    }
}

// No engine draw interleaves here (render thread inside the resolve hook, or a park). dls_select renders nothing
// (g_dls_frame holds it to once a cycle); render_sun_depth below saves its own RT and viewport state, and
// g_sun_maps.sun_map_rendered_frame makes it a no-op once this cycle's map is built (the cast fire's or the
// injection's) unless reopened (SUN_GEN below, SUN_RESTEP).
inline void FlushPass::render_shadow_maps() {
    dls_select();   // DL_SHADOW twin 2/2 (frame-guarded; free if twin 1 already ran).
    if (!injected_since_last_flush && any_lit) {
        // SUN_GEN: the injection's twin - the scene this flush holds (SCENE_HOLD, above) against the one the
        // cast fire's ladder was built from.
        // SUN_GEN_CAP.
        if (g_sun_maps.sun_map_valid && g_scene.gen_cast != g_sun_gen.map_gen && sun_gen_reopen_ok()) {
            g_sun_maps.sun_map_rendered_frame = false;
        }
        render_sun_depth(ctx);
        dls_frame(ctx);   // DLS_FRAME twin 2/2 (frame-guarded; free if twin 1 already ran).
    }
}

inline bool FlushPass::decide_late_work() {
    // INFRONT_TPL: the view-model slice shades under the frame template the injection or this flush publishes
    // (infront_inject: none, no draw). A cycle neither published in - inFront meshes alone, none of them lit and
    // casting (so the injection returned before its template), nothing late here - left the slice with no template,
    // or with the last one while the world's meshes stood. Then this flush runs up to the template, publishes it
    // and returns (the exit before the effect setup). Only such a cycle reaches that exit (infront_tpl): every
    // other nothing-late flush returns ahead of it (the return below, or the one after the occlusion snapshot), so
    // a template the injection published stands. The same lanes either way - fx_frame_template being the
    // injection's twin (RENDER_FLUSH), and the slice's own fill sets what a slice draw reads differently.
    infront_tpl = g_infront.wanted.load(std::memory_order_relaxed) &&
                  (!g_pip.tpl_valid || g_pip.tpl_cycle != g_topo_cycles);
    nothing_late = meshes.empty() && fullscreen.empty();
    if (nothing_late && comp_eligible == 0 && !infront_tpl) {
        // Nothing to draw or snapshot for; the injection (healthy, per comp_healthy) picks up whatever turns visible.
        return false;
    }

    std::sort(fullscreen.begin(), fullscreen.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    return true;
}

inline bool FlushPass::pick_projection() {

    const bool sparse_live = !cycle_landed &&   // FLUSH_CYCLE_WITNESS: the cycle in flight.
        g_ro.opaque_draws < REORDER_MIN_OPAQUE_DRAWS / 8;

    if (sparse_live && RVExtBridge::get_projection_view_transform(pv)) {
    } else if (g_ro.cycle_pv_valid) {
        pv = g_ro.cycle_pv;   // LATCH_FOLD: the boundary camera, never a held one.
    } else if (!RVExtBridge::get_projection_view_transform(pv)) {
        return false;
    }

    // Cycle state can be read mid-reset from another thread; never transform
    // with a degenerate projection - the live bridge repairs.
    if (fabsf(pv.projection[2][2]) < 1e-6f) {
        RVExtBridge::ProjectionViewTransform live_fix = {};
        if (!RVExtBridge::get_projection_view_transform(live_fix)) {  return false; }
        pv = live_fix;
    }

    // Registration repaint is meaningful only when an injected image exists
    // this frame; the window is the staleness bound on the landing's stamp.
    if (injected_since_last_flush &&
        g_inj_pub.view_ms != 0 && steady_now_ms() - g_inj_pub.view_ms < 100) {
        memcpy(&pv.view[0][0], g_inj_pub.view, sizeof(g_inj_pub.view));
    } else {
        adopt_frame_view(pv);
    }

    const bool slot_band = g_ro.slot_near_live >= CAM_NEAR_MIN &&
                           g_ro.slot_near_live <= CAM_NEAR_MAX &&
                           enc_far(g_ro.slot_m22, g_ro.slot_m32) <= ENC_FAR_MAX;

    const bool pub_valid = g_inj_pub.enc_ms != 0 && g_inj_pub.enc_near > 0.0f;
    bool inj_pair = pub_valid && injected_since_last_flush;
    bool latch_far = false;

    if (!inj_pair && pub_valid) {
        const float pubv_near = fabsf(pv.projection[2][2]) > 1e-9f
                              ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        const bool near_agree = pubv_near >= CAM_NEAR_MIN &&
                                pubv_near <= CAM_NEAR_MAX &&
                                fabsf(pubv_near - g_inj_pub.enc_near) <= 0.25f * pubv_near;

        if (near_agree) {
            const float pv_m221 = pv.projection[2][2] - 1.0f;
            const float pub_m221 = g_inj_pub.enc_m22 - 1.0f;
            const bool pv_far_sane =
                enc_far(pv.projection[2][2], pv.projection[3][2]) <= ENC_FAR_MAX;
            const bool far_agree = !pv_far_sane ||
                fabsf(pub_m221 - pv_m221) <=
                0.25f * fmaxf(fabsf(pv_m221), 1e-6f);

            if (far_agree) {
                inj_pair = true;
            } else {
                latch_far = true;
            }
        }
    }

    // LATE_WORLD_PAIR - a render-thread flush drawing a cycle the injection did not land in, on a frame drawn under
    // one pair, takes that pair: the world phase's latch (this cycle's g_ro.world_m22 / m32 - the pair the engine's
    // last opaque world draw rendered under, which wrote the depth these draws test against), the injection's own
    // first choice. Measured: in sparse views the injection never ran (6 to 65 engine opaques a frame), and the
    // publication rung took a pair published a frame to 52 s before whose near was within its 25 % of the camera's
    // (6.51 m against 6.29, 3.65 against 4.52). The engine's near plane moves with the view, so the mesh was encoded
    // nearer or farther than the terrain about it - a slice through it that moved with the camera, in front of the
    // terrain or behind it. The latch replaces exactly that: an earlier landing's publication (inj_pair or
    // latch_far, chosen against it) in a cycle that has not landed. Everything else stands - a landed cycle, a
    // session no injection has published in (post effects alone), a park's flush, a cycle with no latch, and a
    // partitioned frame: the latch's near past 1.5 x the camera's (PART_NEAR), whose depth holds more than one
    // encode, so that no one pair is right for a late draw.
    bool world_pair = false;
    if (rt && !injected_since_last_flush && pub_valid && (inj_pair || latch_far) &&
        g_ro.world_pair_valid) {
        const float wp_near = fabsf(g_ro.world_m22) > 1e-9f ? (-g_ro.world_m32 / g_ro.world_m22) : -1.0f;
        const float wp_cam = fabsf(pv.projection[2][2]) > 1e-9f
                           ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        world_pair = wp_near > 0.0f && wp_cam > 1e-4f && wp_near <= 1.5f * wp_cam;
    }

    if (world_pair) {
        pv.projection[2][2] = g_ro.world_m22;
        pv.projection[3][2] = g_ro.world_m32;
    } else if (inj_pair) {
        pv.projection[2][2] = g_inj_pub.enc_m22;
        pv.projection[3][2] = g_inj_pub.enc_m32;
    } else if (latch_far) {
        // Far drifted under an agreeing near: the latch pair is the encode this flush - deliberately ahead of the slot
        // rung, whose flush-time value is the partition ambiguity the publish path exists to avoid.
    } else if (slot_band) {
        pv.projection[2][2] = g_ro.slot_m22;
        pv.projection[3][2] = g_ro.slot_m32;
    } else if (g_slot_keep.near_m > 0.0f && g_slot_keep.ms != 0 &&
               steady_now_ms() - g_slot_keep.ms < 250) {
        // Slot silent this cycle: the keep is the poison shield, but an in-band pv that disagrees with it is fresher
        // (the moving-camera silent frame) and wins - same arbitration as the injection side.
        const float pv_near = fabsf(pv.projection[2][2]) > 1e-9f
                            ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        const bool pv_in_band = pv_near >= CAM_NEAR_MIN && pv_near <= CAM_NEAR_MAX;
        const bool keep_agrees = pv_in_band &&
            fabsf(pv_near - g_slot_keep.near_m) <= 0.25f * pv_near;

        if (keep_agrees || !pv_in_band) {
            pv.projection[2][2] = g_slot_keep.m22;
            pv.projection[3][2] = g_slot_keep.m32;
        }
    }

    mul_4x4(pv.view, pv.projection, view_proj);
    return true;
}

inline void FlushPass::take_occlusion_snapshot() {

    // Verdict computed per mesh in upload_cb; PS pick at the loop's category switch.

    // Occlusion snapshot. Every consumer is mesh-side (the injection guard, the flush PSMain guard/arb, the t2
    // effect-mesh arbitration - all freshness-gated and fail-safe to the stock path); the fullscreen chain reads the
    // live depth SRV, never this.
    if (any_mesh) snapshot_take(dev, ctx, pv);
}

inline void FlushPass::build_inverse() {

    bool need_inverse = false;

    for (const auto& o : meshes) {
        if (o.localized || o.effect == static_cast<int>(EffectId::Pulse) ||
            o.effect == static_cast<int>(EffectId::Fogscatter) ||
            o.effect == static_cast<int>(EffectId::DynLightFog) ||   // DLF: its rays.
            o.effect == EFFECT_CUSTOM) { need_inverse = true; break; }
    }

    if (!need_inverse) {
        for (const auto& f : fullscreen) {
            if (f.second.localized || f.second.effect == static_cast<int>(EffectId::Pulse) ||
                f.second.effect == static_cast<int>(EffectId::Fogscatter) ||
                f.second.effect == static_cast<int>(EffectId::DynLightFog) ||   // DLF: its rays.
                f.second.effect == EFFECT_CUSTOM) { need_inverse = true; break; }
        }
    }

    // The inverse serves KhWorldPosFenced / KhWorldPosRelFenced, which read the engine depth, so it inverts the view
    // that RENDERED that depth: the cycle's boundary camera - the flush's own pv may be the injection's view a frame
    // apart, which shimmers every localized mask's edge. The inverse is of the rotation-only view; the camera rides in
    // fxCam (an absolute inverse is kilometres cancelling per pixel in fp32).
    // FX_FRAME_ADOPT: that camera as the meshes take it - under the engine's own frame rotation when it is fresh
    // (adopt_frame_view, on a copy), its position through the honest inverse (cam_of). The transpose
    // (extract_camera_pos) is the inverse only of an exactly orthonormal rotation: the translation carries the
    // camera's map position, kilometres, so a rotation a little off it misplaces the camera by metres, by an amount
    // that turns with the camera - every reconstructed point, the dynamic lights' fog's included, slid over the scene
    // in the Zeus camera until a camera reset.
    float dvp[4][4];
    bool dvp_pair_engine = false;   // LOCAL_CULL: dvp's projection carries the engine's pair.
    if (g_ro.cycle_pv_valid && !g_ro.cycle_pv_stale && fabsf(g_ro.cycle_pv.projection[2][2]) > 1.0e-6f) {
        RVExtBridge::ProjectionViewTransform dpv = g_ro.cycle_pv;
        adopt_frame_view(dpv);   // The view only; the projection stays the cycle's.
        float dproj[4][4];
        memcpy(dproj, dpv.projection, sizeof(dproj));
        {   // FX_ENGINE_PAIR: the pair the passes linearise with.
            // FX_PAIR_HOLD: through the hold guard (fx_depth_pair_guard's note) - this flush's verdict,
            // which the chain's passes replay (fx_chain_run), so the inverse and every reader take one pair.
            // The local cull's witness stays the raw engine pair (local_culled). The range lanes go in as the
            // passes carry them (the guard adopts all four). FX_PAIR_ONE: on both routes - with no engine pair
            // the passes carry this flush's own pair (upload_cb: pv's projection), and the inverse must invert
            // through the pair every reconstruction encoded with.
            ConstantData pg = {};
            pg.depth_params[0] = g_ro.engine_proj_valid ? g_ro.engine_m22 : pv.projection[2][2];
            pg.depth_params[1] = g_ro.engine_proj_valid ? g_ro.engine_m32 : pv.projection[3][2];
            fill_depth_range_cb(pg);
            fx_depth_pair_guard(pg, g_cc_flush_serial.load(std::memory_order_relaxed));
            dproj[2][2] = pg.depth_params[0];
            dproj[3][2] = pg.depth_params[1];
            dvp_pair_engine = g_ro.engine_proj_valid;   // LOCAL_CULL: the engine's pair alone.
        }
        float dview[4][4];
        memcpy(dview, dpv.view, sizeof(dview));
        dview[3][0] = dview[3][1] = dview[3][2] = 0.0f;
        dview[3][3] = 1.0f;
        mul_4x4(dview, dproj, dvp);
        cam_of(dpv.view, fx_cam);
        fx_cam[3] = 1.0f;
    } else {
        memcpy(dvp, view_proj, sizeof(dvp));   // Absolute; fxCam stays disarmed.
    }
    has_inverse = need_inverse && inverse_4x4(dvp, inv_view_proj);
    // Only objects that need the inverse consult has_inverse, so skipping it when none does changes nothing.
    // LOCAL_CULL: the frame the chain's localized passes reconstruct in - camera-relative, built with the
    // engine's pair (the pair those passes linearise with); otherwise nothing is culled.
    if (has_inverse && g_ro.engine_proj_valid && fx_cam[3] > 0.5f &&
        dvp_pair_engine) {
        local_cull_make(lcull, dvp, fx_cam, true, g_ro.engine_m22, g_ro.engine_m32);
    }
}

inline void FlushPass::adopt_camera() {
    cam_of(pv.view, cam);   // FX_FRAME_ADOPT: the honest inverse, as fxCam and the injection take it.
    // Taken here, before the rain tracker and every other consumer, so the pass is internally coherent.
    engcam = engcam_consume(pv.view, cam);
    // FOG_PROBE: a cycle the injection recorded no camera for (none landed, or it returned with nothing to inject)
    // takes this pass's. A cycle it did record keeps the injection's.
    if (g_ls_cam_cycle != g_topo_cycles) {
        g_ls.cam[0] = cam[0];
        g_ls.cam[1] = cam[1];
        g_ls.cam[2] = cam[2];
        g_ls_cam_cycle = g_topo_cycles;
    }
}

inline void FlushPass::measure_viewport() {

    // Screen dimensions for the effect shader. The entry viewport's depth range is neither used nor learned
    // (FLUSH_DEPTH_RANGE, upload_cb; SCENE_VP_TRIG): a park can land in the view model's slice or a fullscreen
    // pass's [0, 1].
    {
        UINT n_vp = 1;
        D3D11_VIEWPORT vp = {};
        ctx->RSGetViewports(&n_vp, &vp);

        if (n_vp >= 1) {
            screen_w = vp.Width;
            screen_h = vp.Height;
        }
    }

    // The pass's own pixels-per-tangent, published here for both draw loops. LOD_ONE_SCALE: lod_publish, the
    // injection's definition too; on the render thread (the scene viewport bound) an unusable pair publishes 0
    // (every level-0 pick). LOD_PARK_KEEP: a park publishes only at the injection's last pass height, or while
    // the injection has not published within the last two cycles; otherwise, and for an unusable pair, the last
    // value stands.
    const bool lod_inj_live = g_lod.inj_cycle != ~0ull && g_topo_cycles - g_lod.inj_cycle <= 2u;
    if (rt || !lod_inj_live || screen_h == g_lod.inj_h) {
        if (!lod_publish(pv.projection[1][1], screen_h) && rt) {
            g_lod.proj_px = 0.0f;
            g_lod.zoom = 1.0f;   // LOD_ZOOM.
        }
    }
}

inline void FlushPass::publish_frame_template() {
    ConstantData tcb = {};
    fx_frame_template(tcb, pv, view_proj, inv_view_proj, fx_cam, engcam, cam);
    fill_user_cam_cb(tcb, cam, pv.view);   // USER_LANES.
    g_pip.tpl = tcb;
    g_pip.tpl_valid = true;
    g_pip.tpl_cycle = g_topo_cycles;
}

inline bool FlushPass::prepare_effects() {

    now = effect_time_seconds();
    now_d = effect_time_seconds_d();

    // Capability gating per frame: effects need the scene capture and the effect shader; depth effects additionally
    // need the depth SRV in the PS, which requires the read-only DSV swap.
    auto needs_depth = [](const RenderObject& o) { return fx_needs_depth(o); };   // NEARZ_READER.

    bool any_effect = !fullscreen.empty();
    bool any_depth_fx = false;
    // CAP_DEFER - with no mesh of this flush's own (none to draw, none composite-eligible: so no SSAO term either),
    // the scene capture has one reader, the chain, and nothing draws into the scene between here and the chain (the
    // sun pyramids' conversion draws into its own targets and restores the OM; the DSV swap moves the depth view
    // only). So the copy is taken where the chain takes its own after meshes (fx_chain_run's capture): the
    // same picture from the same target. A chain this flush hands to FX_LATE then costs no copy here (the late
    // flush takes its own), which saves a full resolve a frame on an MSAA scene. Not under a predicate the engine left
    // bound: the copy here runs under it, the chain's after StateBackup has cleared it (a copy the predicate skipped
    // left the chain the last capture; deferred, it would get this frame's). So with one bound the copy stays here.
    // Nothing between here and the setup's copy sets predication (only StateBackup does).
    // CAP_DEFER_FXM: with meshes but none with an effect (effect != 0, the t0 = capture category, counted before
    // any demotion to 0), the copy here has no reader at all: every mesh draw binds t0 by its category first (the
    // snapshot or null), a perceptual translucent takes its own capture in the loop (FX_CAP_LAZY: the first of
    // them) and t3 holds only captures taken since, a lighten / darken mesh copies its own (mm_capture), the
    // pyramids are an effect mesh's, the see-through record's B / Q / C are other textures, the DLS world pass ran
    // before the flush, and the chain takes its own after meshes (capture: !meshes.empty()). So it is left out
    // the same way; the checks run.
    bool fxm_any = false;
    for (const auto& fm : meshes) if (fm.effect != 0) { fxm_any = true; break; }
    cap_defer = meshes.empty() ? comp_eligible == 0 : !fxm_any;
    if (cap_defer) {
        ID3D11Predicate* cap_pred = nullptr;
        BOOL cap_pred_v = FALSE;
        ctx->GetPredication(&cap_pred, &cap_pred_v);
        if (cap_pred) {
            cap_defer = false;
            cap_pred->Release();
        }
    }

    for (const auto& o : meshes) {
        if (o.effect > 0) any_effect = true;
        // A mode-Off solid does not count here: its guard reads the snapshot at t0 (PSMain, armed by fxParams1.x < 1e8,
        // which upload_cb disarms without a fresh snapshot) - never t1, never the effect unit, never the scene capture.
        if (needs_depth(o)) {
            any_effect = true;
            any_depth_fx = true;
        }
    }

    for (const auto& f : fullscreen) if (needs_depth(f.second)) any_depth_fx = true;
    if (any_effect) {
        UINT dw = 0, dh = 0;

        {
            std::string fx_err = ensure_depth_srv(dev, ctx, &dw, &dh);
            if (fx_err.empty()) fx_err = ensure_effect_shader(dev);
            if (fx_err.empty()) {   // CAP_DEFER: the checks always, the copy below or at the chain.
                fx_err = cap_defer ? ensure_scene_capture(dev, ctx, false) : scene_capture_timed(dev, ctx);
            }
            effects_ready = fx_err.empty();
            // Reported once per distinct message.
            if (!effects_ready) ensure_ok("effect setup", fx_err);   // ENSURE_ONCE.
        }

        depth_fx_ready = effects_ready && any_depth_fx &&
                         g_res.depth_srv && g_res.depth_dsv_ro;

        // Same demotion, same fail direction.
        if (!has_inverse) {
            for (auto& o : meshes)
                if (o.effect == static_cast<int>(EffectId::Pulse) ||
                    o.effect == static_cast<int>(EffectId::Fogscatter) ||
                    o.effect == static_cast<int>(EffectId::DynLightFog)) o.effect = 0;   // DLF.
            for (auto& f : fullscreen)
                if (f.second.effect == static_cast<int>(EffectId::Pulse) ||
                    f.second.effect == static_cast<int>(EffectId::Fogscatter) ||
                    f.second.effect == static_cast<int>(EffectId::DynLightFog)) f.second.effect = 0;   // DLF.
        }

        // FX_DEPTH_DROP: without the depth SRV, the depth readers whose picture needs it stand down for the frame
        // rather than read an unbound t1 (every pixel would decode at about the near plane). One rule,
        // fx_depth_drop: every fx_needs_depth reader except an unbanded, unlocalized outline (its luma edges
        // still draw) or user effect (it may read no depth). A fullscreen pass is dropped here; an effect MESH is
        // skipped by the draw loop below (the same predicate), not demoted to a solid. The UI lane drops EVERY depth
        // reader without its depth SRV (flush_ui_locked's wants_depth). The fog records a user effect sees mirror the
        // chain: the fog drops there.
        if (!depth_fx_ready) {
            for (auto& f : fullscreen) if (fx_depth_drop(f.second)) f.second.effect = 0;
        }

        if (!effects_ready) {
            // Demote everything to solid for this frame; fullscreen passes
            // without an effect are meaningless - drop them.
            for (auto& o : meshes) o.effect = 0;
            fullscreen.clear();
            if (meshes.empty()) {
                // INFRONT_TPL: an inFront-only cycle whose passes were all dropped here still owes the slice
                // this cycle's template - the template-only exit's publish (nothing drawn, nothing bound yet).
                if (infront_tpl) publish_frame_template();
                return false;
            }
        }
    }
    return true;
}

// The mesh pass and the chain. Its scope objects (the state backup, the bind filter, slot 0, the OM save, the replay
// scope) are this function's locals in the order the single function declared them, so they unwind as they did.
inline void FlushPass::draw() {
    StateBackup backup;
    backup.capture(ctx);
    BindScope bc(ctx);   // BIND_FILTER.
    OmSlot0 s0;   // MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    s0.open(ctx);
    objbuf_sync(ctx);   // OBJBUF: the flush's buckets index the records too.
    objbuf_bind(ctx);
    tex_async_pump(ctx, dev);   // TEX_ASYNC: before any base is fixed or lane written.

    // Depth-sampling effects: the read-only DSV is swapped in so the live depth SRV may sit at PS t1 while depth
    // testing continues. Per draw, not per phase: it lands when the scene/depth pair is bound and the real DSV returns
    // for any depth-writing solid, so a TestWrite mesh keeps its write on frames that also carry a depth effect. The
    // depth SRV is never bound while the writable DSV is (t1 = atlas on the solid category).
    OmSave om;
    om_p = &om;   // dsv_swap's save, alive to this function's end.

    bind_pass_state();   // The pipeline, the constant buffers and every texture the pass reads.
    judge_far_arb();     // The ARB twin's analytic clamp for effect meshes (hysteresis with the injection).

    // REPLAY_SCISSOR: the scene was synced again above, so what this flush draws can differ from what the replay's
    // rectangles were made from; the merged textures are its own only where they cover it.
    if (g_rp.mir_pend) {   // MIR_REPLAY: as the injection (this pass's projection is final above).
        for (const RenderObject& o : meshes) {
            if (rp_mir_reads(o, pv.projection, cam)) { rp_mir_finish(ctx); break; }
        }
    }
    bool rp_vol = false, rp_mir = false;
    rp_covered(meshes, rp_vol, rp_mir);
    RpPassScope rp_scope(rp_vol, rp_mir);
    build_frame_cb();    // The frame template (and the PIP's, when no injection published one).
    judge_near_gap();    // FLUSH_NEARZ: the near-gap route's gate.
    inst_on = inst_twins_on();   // Buckets, flush edition: the pass verdict (the members' note).
    build_visibility();  // MESH_ORDER and the frustum cull (vis).
    order_draws();       // Solids, then the far-to-near translucent tail (WSEE's deferred parts first).
    plan_buckets();      // INST_RUN / BUCKETS.
    prepare_draw_loop(); // The bind trackers, the viewports, the SSAO pass.
    prepare_wsee_sort(); // WSEE: the tail's sort mode and depth decode.

    for (size_t ix = 0; ix < order.size(); ++ix) draw_object(ix);
    finish_meshes();     // The SSAO post when no tail ran it; the viewport and rasterizer back.
    run_chain();         // The fullscreen chain (or FX_LATE's hand-off).

    if (om_captured) {
        // t1 may still hold the live depth SRV (the chain's, or a pair consumer's last bind): down before
        // the writable DSV returns, as the tail and the SSAO site do - never both bound at once.
        ID3D11ShaderResourceView* t1_off = nullptr;
        ctx->PSSetShaderResources(1, 1, &t1_off);
        om.restore(ctx);
    }
    om.release();
    backup.restore(ctx);
}

inline void FlushPass::dsv_swap(bool ro) {
    if (!depth_fx_ready || ro == dsv_ro) return;
    if (!om_captured) { om_p->capture(ctx); om_captured = true; }
    if (ro) om_p->set_with_dsv(ctx, g_res.depth_dsv_ro);
    else         om_p->restore(ctx);
    dsv_ro = ro;
}

inline void FlushPass::bind_pass_state() {
    bound_vb.vb = g_res.mesh_vb.empty() ? nullptr : g_res.mesh_vb[0];
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &g_res.mesh_vb[0], &stride, &offset);
    ctx->IASetIndexBuffer(g_res.mesh_ib[0], DXGI_FORMAT_R32_UINT, 0);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    // Twin: the injection path carries the identical narrowing.
    ID3D11Buffer* cbs[2] = { g_res.constant_buffer, g_res.frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    // s0 for the textured per-submesh path; StateBackup's sampler slot restores.
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);

    ps_srvs[0] = effects_ready ? g_res.scene_srv : nullptr;
    ps_srvs[1] = depth_fx_ready ? g_res.depth_srv : nullptr;
    // t1 here (FSAA off: a single-channel R24/R32 SRV over the live depth) and the PIP depth copy both land under the
    // effect unit's Texture2D<float2> depthTex; the arb snapshot (R32G32) under Texture2D<float> khArbSnap. The shader
    // reads .x only and the runtime does not validate return-type width; only the debug layer would warn.

    // t1 stays empty until the first pair consumer swaps the DSV (dsv_swap): the writable DSV is bound here.
    ID3D11ShaderResourceView* srvs_init[2] = { ps_srvs[0], nullptr };
    ctx->PSSetShaderResources(0, 2, srvs_init);
    // Flush-time proven depth snapshot for the effect-mesh arbitration (t2, inside the StateBackup saved range;
    // PSEffect reads it only behind the local1.z gate).
    if (g_res.comp_depth_srv && g_res.comp_depth_samples == 1) {
        ctx->PSSetShaderResources(2, 1, &g_res.comp_depth_srv);
    }
    // Private sun-depth map (self term in PSMain), rendered earlier this frame (the cast fire, the injection, or this
    // flush's render_sun_depth above - a park's included) - t11, inside StateBackup's saved range.
    sun_pf_convert(ctx);   // Pyramids convert here (proven binds).
    if (g_sun_maps.sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
    if (g_sun_maps.sun2_map_valid && g_res.sun2_srv) ctx->PSSetShaderResources(25, 1, &g_res.sun2_srv);
    if (g_sun_maps.sun3_map_valid && g_res.sun3_srv) ctx->PSSetShaderResources(26, 1, &g_res.sun3_srv);
    if (g_sun_maps.sun4_map_valid && g_res.sun4_srv) ctx->PSSetShaderResources(27, 1, &g_res.sun4_srv);
    if (g_sun_maps.sun5_map_valid && g_res.sun5_srv) ctx->PSSetShaderResources(32, 1, &g_res.sun5_srv);
    if (g_sun_maps.sun2_map_valid && g_sun_maps.sun_pf_valid[0] && g_res.sun_pf_srv[0]) ctx->PSSetShaderResources(29,
        1, &g_res.sun_pf_srv[0]);
    if (g_sun_maps.sun3_map_valid && g_sun_maps.sun_pf_valid[1] && g_res.sun_pf_srv[1]) ctx->PSSetShaderResources(30,
        1, &g_res.sun_pf_srv[1]);
    if (g_sun_maps.sun4_map_valid && g_sun_maps.sun_pf_valid[2] && g_res.sun_pf_srv[2]) ctx->PSSetShaderResources(31,
        1, &g_res.sun_pf_srv[2]);
    // FAR_PF.
    if (g_sun_maps.sun5_map_valid && g_sun_maps.sun_pf_valid[3] && g_res.sun_pf_srv[3]) ctx->PSSetShaderResources(20,
        1, &g_res.sun_pf_srv[3]);
    ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // SHADOW_GATHER: null = the default state, also clamp.
    // Analytic terrain heightfield (t10, saved range) - the flush twin.
    if (g_thm.valid && g_res.thm_srv) ctx->PSSetShaderResources(10, 1, &g_res.thm_srv);
    // The light depth array at t36, twin of the injection's bind and inside this pass's StateBackup range.
    if (g_res.dls_srv) {
        ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
    }
    for (UINT bslot = 0; bslot < 8; ++bslot) {
        if (g_ls.band[bslot].valid && g_ls.band[bslot].srv) {
            const UINT treg = bslot < 6 ? 4 + bslot : 12 + (bslot - 6);
            ctx->PSSetShaderResources(treg, 1, &g_ls.band[bslot].srv);
        }
    }
    ctx->OMSetBlendState(g_res.blend_modes[0], bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
    bound_rs = g_res.rasterizer;
}

inline void FlushPass::judge_far_arb() {
    // Same floor/hold hysteresis on the flush's final encode near; requires the same-flush snapshot and the live
    // heightfield. Fail-safe: either down => stock path.
    const float arb_near = fabsf(pv.projection[2][2]) > 1e-9f
                         ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
    const bool arb_snap_ok = g_res.comp_depth_srv && g_res.comp_depth_samples == 1 &&
                             g_res.comp_depth_time >= 0.0f &&
                             now - g_res.comp_depth_time < 0.5f;
    // The floor applies here too: the twin must agree with the injection or the flush repaint re-takes the step.
    const bool arb_near_ok = arb_near >= FAR_ARB_NEAR_MIN || (g_flush_fx_arb_prev && arb_near >= FAR_ARB_NEAR_HOLD);
    fx_arb_frame = arb_snap_ok && g_thm.valid && arb_near_ok;

    g_flush_fx_arb_prev = fx_arb_frame;
}

inline void FlushPass::build_frame_cb() {
    // Deliberate twin of the injection build (cbf); the per-object draw
    // tails of the two mesh paths are twins by the same decision.
    rebase_on = fx_frame_template(cbf, pv, view_proj, inv_view_proj, fx_cam, engcam, cam);
    fill_user_cam_cb(cbf, cam, pv.view);   // USER_LANES.
    // The PIP paths (pip_inject, pip_run_fx) shade from a frame template the mesh injection publishes; a cycle with
    // no injection (no meshes) would leave them none. The flush's frame CB is the same template one publisher later,
    // and the PIP disarms what differs. The injection's copy, when there is one this cycle, stands.
    if (!g_pip.tpl_valid || g_pip.tpl_cycle != g_topo_cycles) {
        g_pip.tpl = cbf;
        g_pip.tpl_valid = true;
        g_pip.tpl_cycle = g_topo_cycles;
    }
    frame_ok = upload_frame_cb(ctx, g_res.frame_cb, cbf);
}

inline void FlushPass::judge_near_gap() {
    // The near-gap route, flush edition: the same gate, per-mesh verdict (its drawn box inside near + 2 m -
    // NEARZ_BOUNDS, get_drawn - never the far route), lanes (fxMeta.x = near, fxMeta.y = the gap floor), viewport
    // swap (MinDepth = floor for that draw) and pixel variant (the ARB twin, whose SV_Depth ramp spreads the gap
    // fragments from the floor up to the viewport min).
    mesh_vp_lo = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
    mesh_vp_hi = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
    const float nz_A = pv.projection[2][2];
    const float nz_B = pv.projection[3][2];
    nz_near = (nz_A > 1.0e-6f) ? (-nz_B / nz_A) : 0.0f;
    nz_on = g_res.ps_composite_arb != nullptr && nz_near > CAM_NEAR_MIN && nz_near < 50.0f &&
                mesh_vp_lo > 1.0e-5f && mesh_vp_lo < mesh_vp_hi && mesh_vp_hi <= 1.0f;
    nz_gap_lo = mesh_vp_lo * NEARZ_GAP_FRAC;
}

inline bool FlushPass::nz_verdict(const RenderObject& o) {
    return nz_on &&
           g_dist_memo_flush.get_drawn(o, cam) < (nz_near + 2.0f) * (nz_near + 2.0f);
}

inline bool FlushPass::upload_cb(const RenderObject& o, bool chain_pass, ConstantData* defer) {
    nearz_draw = false;   // FLUSH_NEARZ: per-call default; solid section may set it.
    ConstantData& cbd = defer ? *defer : obj_cbd;
    cbd = cbf;   // CB split: frame template (matrices ride for the echoes).
    cbd.center_size[0] = o.pos[0];
    cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
    cbd.center_size[2] = o.pos[1];
    cbd.center_size[3] = chain_pass ? 1.0f : 0.0f;
    // Per-object rebase arm mirrors the pass verdict (one without the other
    // is the wrong-space landmine). Chain passes carry it inertly.
    float res[3];   // POS_RES: the object's live residual.
    obj_pos_res(o, res);
    if (rebase_on) fill_center_rel(cbd, cam, res);
    cbd.size_axes[0] = o.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y].
    cbd.size_axes[1] = o.size[2];
    cbd.size_axes[2] = o.size[1];
    cbd.size_axes[3] = static_cast<float>(o.blend_mode);
    fill_obj_rot(cbd, o.rot_m);
    // A normal-blend translucent is never bucketed, so the perceptual arm
    // is decided by the object alone. FX_PERC_FX: never an effect mesh - PSEffect reads neither blendCtl.x
    // nor t3 as the scene (perc_count's rule).
    cbd.blend_ctl[0] = ((perceptual && !o.fullscreen && o.draw_part != 1 && o.effect == 0 &&
                         o.blend_mode == 0 && o.color[3] < 0.999f) ||
                        (mm_cap && blend_minmax_exact(o))) ? 1.0f : 0.0f;   // MAT_BLEND parts:
                                                                                   // Never perceptual.
    // blend_ctl[3] is the crossfade dither; written here so it is never inherited from the previous object's fade.
    cbd.blend_ctl[3] = 0.0f;
    fill_fx_params_cb(cbd, o);
    if (!chain_pass && glow_draw) glow_lanes(cbd, glow_draw);   // GLOW_PYR.
    if (!chain_pass && ana_draw) ana_lanes(cbd, ana_draw);   // ANA_PYR.

    if (o.effect == static_cast<int>(EffectId::Fogscatter)) {
        memset(cbd.fx1, 0, sizeof(cbd.fx1));
        memset(cbd.fx2, 0, sizeof(cbd.fx2));
    }

    // Solid meshes repurpose fx0.xyz (unused at effect 0) for the camera,
    // which SolidMask's band term measures against.
    if (o.effect == 0) {
        cbd.fx0[0] = cam[0];
        cbd.fx0[1] = cam[1];
        cbd.fx0[2] = cam[2];

    }

    cbd.fx_meta[0] = static_cast<float>(o.effect);
    cbd.fx_meta[1] = fx_time(o, now_d);
    cbd.fx_meta[2] = screen_w;
    cbd.fx_meta[3] = screen_h;
    cbd.depth_params[0] = pv.projection[2][2];
    cbd.depth_params[1] = pv.projection[3][2];
    // FLUSH_DEPTH_RANGE: the range the engine wrote the scene's depth through and every mesh viewport of this
    // flush draws in (mesh_vp_lo / hi, the same trigger-or-scene pair), as the injection fills it - the ARB
    // twin's SV_Depth and LinDepth decode with it. Not the entry viewport's range: a park landing in the view
    // model's slice would put every near-gap fragment at the gap floor, in front of the world.
    fill_depth_range_cb(cbd);

    // depthParams stays the live-depth pair for LinDepth/WorldPos; local0/local1.zw carry the guard's channel once
    // the masks have written.

    // Without a fresh snapshot: guard stands down.
    if (o.effect == 0) {
        // per-mesh near-gap verdict (nz_verdict; FLUSH_NEARZ).
        nearz_draw = !chain_pass && nz_verdict(o);
        const bool snap_fresh = (o.mode == DepthMode::Off) &&
                                g_res.comp_depth_srv &&
                                g_res.comp_depth_samples == 1 &&
                                g_res.comp_depth_time >= 0.0f &&
                                now - g_res.comp_depth_time < 0.5f;

        if (snap_fresh) {
            const bool measured = g_ro.engine_proj_valid;

            if (measured) {
                cbd.depth_params[0] = g_ro.engine_m22;
                cbd.depth_params[1] = g_ro.engine_m32;
            }

            if (g_ro.slot_near_live >= CAM_NEAR_MIN &&
                g_ro.slot_near_live <= CAM_NEAR_MAX &&
                enc_far(g_ro.slot_m22, g_ro.slot_m32) <= ENC_FAR_MAX) {
                cbd.depth_params[0] = g_ro.slot_m22;
                cbd.depth_params[1] = g_ro.slot_m32;
            }

            fill_depth_range_cb(cbd);
            // Twin: both CB editions carry it.
            cbd.fx_meta[2] = static_cast<float>(g_res.comp_depth_w);
            cbd.fx_meta[3] = static_cast<float>(g_res.comp_depth_h);

            const bool exact_flush = measured ||
                (g_ro.slot_near_live >= CAM_NEAR_MIN &&
                 g_ro.slot_near_live <= CAM_NEAR_MAX &&
                 enc_far(g_ro.slot_m22, g_ro.slot_m32) <= ENC_FAR_MAX);
            cbd.fx1[0] = exact_flush ? COMPOSITE_GUARD_BASE_MEASURED : COMPOSITE_GUARD_BASE;
            cbd.fx1[1] = exact_flush ? COMPOSITE_GUARD_REL_MEASURED : COMPOSITE_GUARD_REL;
        } else {
            cbd.fx1[0] = 1e9f;
            cbd.fx1[1] = 0.0f;
        }
        // FLUSH_ARB_LANES: fxParams1.zw are the ARB twin's analytic clamp (the far route's cap and contact
        // band; zero = no terrain pull), which fill_fx_params_cb filled from the object's params[6..7] - a
        // solid given seven or more params pulled its depth near terrain on this route alone. The injection's
        // near-gap route leaves them zero; so does this one. PSMain never reads them.
        cbd.fx1[2] = 0.0f;
        cbd.fx1[3] = 0.0f;
        // The effect id / time this pair carries for effect meshes is never read by a solid draw.
        if (nearz_draw) {
            cbd.fx_meta[0] =
                nz_near;
            cbd.fx_meta[1] = nz_gap_lo;
        }
        // Armed once at the pass frame build (both arms; the terrain lane
        // never depends on the depth lane's arming state).
    }
    fill_local_band_cb(cbd, o);
    fx_arb_this = false;

    if (o.effect > 0 && fx_arb_frame && !o.fullscreen) {
        float m22 = pv.projection[2][2];
        float m32 = pv.projection[3][2];

        if (g_ro.engine_proj_valid) { m22 = g_ro.engine_m22; m32 = g_ro.engine_m32; }
        if (g_ro.slot_near_live >= CAM_NEAR_MIN &&
            g_ro.slot_near_live <= CAM_NEAR_MAX &&
            enc_far(g_ro.slot_m22, g_ro.slot_m32) <= ENC_FAR_MAX) {
            m22 = g_ro.slot_m22; m32 = g_ro.slot_m32;
        }
        cbd.local0[0] = m22;
        cbd.local0[1] = m32;
        cbd.local0[2] = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
        cbd.local0[3] = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
        cbd.local1[2] = FAR_ARB_GUARD_BASE;   // Arm flag + base, one lane.
        cbd.local1[3] = FAR_ARB_GUARD_REL;
        fx_arb_this = true;
    }
    fill_lighting_obj_cb(ctx, cbd, o);
    if (!chain_pass && fx_tex_on(o)) fx_tex_fill(dev, ctx, cbd, o, utex);   // FX_TEX(_ALL).
    // A failed frame upload stands every draw down: fail dark, never stale-frame draws.
    if (defer) return frame_ok;
    return frame_ok && upload_obj_cb(ctx, g_res.constant_buffer, cbd);
}

inline void FlushPass::build_visibility() {
    // Front-to-back lets each opaque lay depth that the ones behind it then
    // fail, so an occluded fragment never enters the shadow ladder.
    {   // MESH_ORDER: key sort, same order, one distance per object.
        mesh_order(meshes, cam, g_dist_memo_flush);   // The memo begins here, seeded; the verdicts, the
                                                         // emit, the tail sort and the pick read it.
    }

    cull_build(view_proj, cam, cull);
    cull.back_on = true;   // CULL_BACK: twin of the injection.
    g_vis_memo_flush.begin(meshes);
}

inline bool FlushPass::vis_calc(const RenderObject& o) {
    float cc[3];
    obj_center_engine(o, cc);
    float bs[3];
    bounds_size_of(o, bs);   // CAST_REFIT: the cull's size; the LOD pick keeps size.
    return mesh_visible(cull, cc, lod_radius_of(bs, o.rot_m, o.rotated));
}

inline bool FlushPass::vis(const RenderObject& fv_o) {
    return g_vis_memo_flush.get(fv_o, [this](const RenderObject& fc_o) { return vis_calc(fc_o); });
}

// A translucent draw is a whole translucent object on normal blend or an object's blend part (draw_part 1). One
// predicate for the sort; the capture demand below is the whole-object case only.
// FX_PERC_FX: a fading effect mesh (normal blend, alpha below 0.999: a duration fade or a colour alpha) sorts
// into the tail with it - it writes no depth, and the tail's far-to-near order is the one its alpha blend wants;
// kept there so a fade's start moves nothing - but is no perceptual translucent (below, o_perc, blend_ctl.x).
inline bool FlushPass::transl(const RenderObject& o) {
    return !o.fullscreen && (o.draw_part == 1 || (o.blend_mode == 0 && o.color[3] < 0.999f));
}

inline void FlushPass::order_draws() {

    // Blend parts are hardware-blended, never perceptual: the perceptual composite lerps against the capture and writes
    // opaque, so overlapping fragments of one draw (the near and far walls of a tube) do not accumulate. A whole
    // translucent object is one hull composited once; a material part is exactly the multi-layer case.
    // FX_PERC_FX: nor is an effect mesh - PSEffect reads neither blendCtl.x nor t3 as the scene (its t3 is the
    // glow / anamorphic pyramid, bound per object), so counting one armed a capture and a t3 bind it never read: one
    // scene resolve a frame per fading effect mesh past the first. Its own source, the capture at t0, is retaken by
    // FX_CAP_ORDER's rule in the loop.
    for (const auto& o : meshes) {
        if (!o.fullscreen && o.draw_part != 1 && o.effect == 0 && o.blend_mode == 0 && o.color[3] < 0.999f)
            // A normal-blend translucent is never bucketed, so the perceptual demand counts objects directly.
            perc_count++;
    }

    // Sorting is per draw, not per triangle: a self-overlapping translucent
    // still relies on the ordered two-sided pass below.
    order.clear();
    size_t tail_n = 0;   // SSAO: where the translucent tail begins.
    // WSEE: the injection's deferred parts (ws_dcopy: WSEE_DEFER_FLAG) draw first, far to near, ahead of this
    // flush's own draws (its solids, effect meshes and tail drew after them when the injection drew them, and still
    // do); they count as tail entries for the sort (front) and the T / NW replay (ws_tail).
    {
        static std::vector<std::pair<float, uint32_t>> tail;
        static std::vector<std::pair<float, uint32_t>> front;
        static std::vector<uint32_t> sol;
        tail.clear();
        front.clear();
        sol.clear();
        for (uint32_t oi = 0; oi < static_cast<uint32_t>(meshes.size()); ++oi) {
            const bool dc = meshes[oi].ws_dcopy != 0u;   // WSEE_DEFER_FLAG.
            if (transl(meshes[oi])) {
                (dc ? front : tail).emplace_back(g_dist_memo_flush.get(meshes[oi], cam), oi);
            } else {
                sol.push_back(oi);
            }
        }
        const auto far_first = [](const std::pair<float, uint32_t>& a, const std::pair<float, uint32_t>& b) {
            return a.first > b.first;
        };
        std::stable_sort(tail.begin(), tail.end(), far_first);
        std::stable_sort(front.begin(), front.end(), far_first);
        tail_n = tail.size();
        def_n = front.size();
        for (const auto& t : front) order.push_back(t.second);
        order.insert(order.end(), sol.begin(), sol.end());
        for (const auto& t : tail) order.push_back(t.second);
    }
    // SSAO: the late-drawn solids' term runs once, before the first translucent (the tail above), so no blend draw
    // takes it; after the loop when there is none. On the WRITABLE DSV: a pair consumer swaps in the read-only view,
    // whose stencil discards the eraser's ZERO, and a mark that survives the park is taken again by the next injection
    // at pixels it never painted.
    solid_n = order.size() - tail_n;
}

inline void FlushPass::plan_buckets() {
    // INST_RUN: a bucket draws every member at its representative (the first visible one in order; for a
    // blend mode other than normal the farthest), so whatever order puts between two members is drawn after all
    // of them. Only a draw on the same commutative blend (additive, multiply, screen, lighten / darken at alpha 1;
    // testing depth without writing it, reading no capture) gives the same picture in either order. So the solid
    // section is cut into runs in drawing order: a run is a stretch of visible draws on one commutative blend, each
    // breaking draw a run of its own (a blended mesh writing depth buckets alone: a bucket's ranges are level-major).
    // The run rides the key's route above its near-gap bit, so a bucket never spans a draw it does not commute with.
    // Normal-blend opaque solids (ord_group 0) keep run 0; a culled object cuts nothing.
    static std::vector<uint32_t> inst_run;
    if (inst_on) {
        inst_run.assign(meshes.size(), 0u);
        uint32_t run_id = 0u;
        int run_bm = -1;   // The open run's blend mode; -1 = none open.
        for (size_t run_x = 0; run_x < solid_n; ++run_x) {
            const uint32_t run_i = order[run_x];
            const RenderObject& run_o = meshes[run_i];
            if (ord_group(run_o) == 0u) continue;
            if (!run_o.fullscreen && !vis(run_o)) continue;
            const bool run_c = run_o.blend_mode != 0 && run_o.effect == 0 && !run_o.fullscreen &&
                               run_o.draw_part != 1 && run_o.mode == DepthMode::TestOnly &&
                               !blend_minmax_exact(run_o);
            if (!run_c || run_o.blend_mode != run_bm) ++run_id;
            inst_run[run_i] = run_id;
            run_bm = run_c ? run_o.blend_mode : -1;
        }
    }

    if (inst_on) {
        const auto vis_fn = [this](const RenderObject& o) { return vis(o); };
        mat_ensure_all(ctx, dev, meshes, vis_fn);   // MAT_TABLE: bases fixed before any lane is written.
        inst_plan_build(inst_plan, meshes, order.data(), order.size(),
                        [&](const RenderObject& o, InstKey& k) {
                            const size_t i = static_cast<size_t>(&o - meshes.data());
                            k.route = (nz_verdict(o) ? 2u : 0u) |
                                      (inst_run[i] << 2);   // INST_RUN.
                            k.aux = dl_bucket_hash(o);
                        });
        inst_plan_emit(ctx, dev, inst_plan, meshes, cam, 0, vis_fn,
                       g_dist_memo_flush);   // BUCKETS: the cull walk, once.
    }
}

inline void FlushPass::prepare_draw_loop() {
    perceptual = perc_count > 0;
    // FX_CAP_LAZY: the perceptual translucents' first capture is taken in the loop by the first of them
    // (cap_stale starts true), not here: a draw of ours ahead of it (a deferred part, a solid, an effect mesh,
    // the SSAO apply) makes a capture taken here stale, and FX_CAP_ORDER's rule takes it again before that
    // translucent - two scene resolves a flush where one serves (every flush with any draw of ours ahead of its first
    // perceptual translucent). Its failure stands the perceptual composite down for the flush (cap_t3
    // false: no capture of this flush is at t3 yet). The per-draw re-capture before a perceptual translucent or an
    // effect mesh is FX_CAP_ORDER's (the loop below).

    bound_vs = g_res.vs;
    bound_il = g_res.input_layout;

    ctx->RSGetViewports(&n_saved_vp, saved_vp);

    {
        if (n_saved_vp >= 1) {
            mesh_vp = saved_vp[0];
        } else {
            mesh_vp.Width = static_cast<FLOAT>(g_main_depth_w);
            mesh_vp.Height = static_cast<FLOAT>(g_main_depth_h);
        }

        mesh_vp.MinDepth = mesh_vp_lo;   // (one truth with the near-gap gate above).
        mesh_vp.MaxDepth = mesh_vp_hi;
        ctx->RSSetViewports(1, &mesh_vp);
    }
    // SSAO: this park draws the injection's meshes late (comp_healthy false), so it is a drawer and takes the term
    // for what it marks - its own depth writes alone (SSAO_MARK), whichever side of the landing it fell on.
    // Translucents and effect meshes write no depth and leave no mark.
    if (!comp_healthy && comp_eligible > 0 && (ao_strength() > 0.0f || nzm_wanted())) {   // NEARZ_MARK.
        ssao_rect(pv, ssao.rect);
        ssao_pre(dev, ctx, ssao, pv.projection, mesh_vp.MinDepth, mesh_vp.MaxDepth);
        ssao.nz_near = nz_on ? nz_near : 0.0f;   // SSAO_NEARZ: as the injection.
        ssao.nz_gap = nz_gap_lo;
    }
    // The gap viewport (MinDepth = the floor the routed draw's ramp starts from) and the per-draw swap tracker - the
    // injection's vp_select, flush edition: 0 = the mesh viewport, 1 = the near-gap.
    nz_vp = mesh_vp;
    if (nz_on) nz_vp.MinDepth = nz_gap_lo;
}

inline void FlushPass::vp_select(int want) {
    if (want == vp_bound) return;
    ctx->RSSetViewports(1, want == 1 ? &nz_vp : &mesh_vp);
    vp_bound = want;
}

inline void FlushPass::prepare_wsee_sort() {
    // WSEE: this run's sort mode for the tail's parts (wsee_sort_mode) and the main depth's decode the parts'
    // tests take (fx_fog[0] on a sorted draw: the pair and the world range, fill_depth_range_cb's; a fragment's own
    // view depth, SV_Position.w, needs no encode). The pair is this flush's own (pv: the injection's encode, which our
    // solids in S were written with and which mode 0's hardware test compares against); mode 0 where the engine's
    // latched pair's near differs from this pair's by more than 2e-3 of it, relative (wsee_pair_split; a partitioned
    // cycle: S and a later partition's G = 1 pixels hold raws of two encodes, which no single decode reads).
    const bool ws_any = def_n > 0 || order.size() > solid_n;
    ws_mode = ws_any ? wsee_sort_mode(ctx, pv.projection[2][2], pv.projection[3][2]) : 0;
    ws_dec[0] = pv.projection[2][2];
    ws_dec[1] = pv.projection[3][2];
    ws_dec[2] = 0.0f;
    ws_dec[3] = 1.0f;
    if (ws_mode > 0) {
        ConstantData ws_rg = {};
        fill_depth_range_cb(ws_rg);
        ws_dec[2] = ws_rg.depth_params[2];
        ws_dec[3] = ws_rg.depth_params[3];
    }
}

// One iteration of the mesh loop (order[ix]); a `return` here is the loop's `continue`. Its scope objects
// (the min / max capture bind, the pyramid binds, the sort scope) are locals, so they unwind at the iteration's end as
// they did.
inline void FlushPass::draw_object(size_t ix) {
    const uint32_t oi = order[ix];
    at_tail_start(ix);   // WSEE / SSAO: the tail's first draw (solid_n).
    const RenderObject& o = meshes[oi];
    // Carried by its bucket's draw at the representative, or culled by the
    // emission (never an overlay: mode Off is not eligible).
    if (inst_on && inst_plan.done[oi]) return;
    // Fullscreen passes have no world extent and are never culled. Twin at the injection.
    if (!o.fullscreen) {
        // vis is the emit's twin, so an emitted bucket representative always draws here.
        if (!vis(o)) {
            return;
        }
    }
    // FX_DEPTH_DROP: an effect mesh whose picture needs the depth, on a frame without the depth SRV, draws
    // nothing (fx_depth_drop - the chain's rule) rather than read an unbound t1.
    if (!depth_fx_ready && o.effect > 0 && fx_depth_drop(o)) return;
    // Before upload_cb (which may skip): a skipped mesh drew nothing, so no refresh is owed. Blend parts: hardware
    // blend, never perceptual. FX_PERC_FX: nor effect meshes.
    const bool o_perc = transl(o) && o.draw_part != 1 && o.effect == 0;
    // BLEND_MINMAX: a lighten / darken mesh composites against its own copy of the scene as it stands right
    // before it, bound at t3 for this object's draws only (mmb puts t3 back at the iteration's end). The shared
    // capture - t3 of the perceptual translucents, t0 of the effect meshes - is not touched, so what they show does
    // not depend on where a lighten / darken mesh falls in the order. On a multisampled target the draw replaces
    // (mm_rep; the twins write alpha 1), as the perceptual composite does: MIN / MAX would apply per sample
    // against samples the resolved copy averaged. Where the mesh is opaque over a scene edge that keeps the pixel's
    // value, not each sample's max / min of it, and a self-overlapping mesh keeps its last face there.
    mm_cap = false;
    mm_rep = false;
    MmBind mmb;
    if (blend_minmax_exact(o)) {
        bool ms = false;
        if (mm_capture(dev, ctx, ms) && mmb.bind(ctx, g_res.mm_srv)) {
            mm_cap = true;
            mm_rep = ms;
        }
    }

    refresh_capture_for_perceptual(o_perc);   // FX_CAP_ORDER / FX_CAP_LAZY.
    // Custom effect (user .hlsl): resolve the compiled PS first - a failed or missing compile skips the draw
    // (reported once) rather than feeding the sentinel id into the builtin switch.
    ID3D11PixelShader* ufx = nullptr;
    bool upyr = false;   // FX_USER.

    if (o.effect == EFFECT_CUSTOM) {
        ufx = user_fx_ps(dev, fx_shader_of(o), &upyr);
        if (!ufx) return;
    }

    if (o.effect == EFFECT_LUT) {
        ID3D11ShaderResourceView* lut = user_lut_srv(dev, fx_shader_of(o));
        if (!lut) return;
        ctx->PSSetShaderResources(19, 1, &lut);
    }

    refresh_capture_for_effect(o);   // FX_CAP_ORDER: an effect mesh's source.
    prepare_pyramids(o, ufx, upyr);   // GLOW_PYR / ANA_PYR.
    // Textured routing (flush edition): textured solids take the PSMain/VSMain twins + the 4-element layout. Twins
    // missing => untextured fallback.
    const MaterialSet* txm = obj_textured(o);
    const bool tx_on = txm != nullptr &&
        g_res.ps_tex && g_res.vs_tex && g_res.layout_tex && g_res.mat_sampler;
    // TX_ONE_UPLOAD: textured, the slice is built here and goes up per submesh in draw_textured (the
    // build-only form: upload_cb fills obj_cbd and returns the frame verdict). Twin: the injection.
    if (!(tx_on ? upload_cb(o, false, &obj_cbd) : upload_cb(o, false))) return;
    GlowBind gb;   // GLOW_PYR: t3 / s2 for this object's draws, put back at the iteration's end.
    if (glow_draw) gb.bind(ctx, *glow_draw);
    AnaBind ab;   // ANA_PYR: t3 / t6, likewise.
    if (ana_draw) ab.bind(ctx, *ana_draw);
    // A translucent part exists only through the textured twins - untextured, the mesh has no submesh table to
    // split and no sampled alpha, so the part is skipped rather than drawn whole.
    if (o.draw_part == 1 && !tx_on) {  return; }
    // A near-gap draw takes the ARB twin - textured (ps_comp_arb_tex) or plain (ps_composite_arb).
    const bool nz_tex_ok = tx_on && g_res.ps_comp_arb_tex != nullptr;
    const bool nz_this = nearz_draw && (tx_on ? nz_tex_ok : true);
    const bool arb_this = nz_this;
    const int ps_want = o.effect > 0 ? 1 : (arb_this ? (tx_on ? 4 : 2) : (tx_on ? 3 : 0));

    bind_object_shaders(o, ufx, ps_want, tx_on);

    bind_object_srvs(o);

    ID3D11DepthStencilState* dss = bind_object_blend_depth(o);
    // A blend part on a two-sided object takes the ordered pass regardless
    // of the object colour's alpha - interiors first, exteriors over them.
    const bool ts_ordered = o.two_sided &&
                            ((o.blend_mode == 0 && o.color[3] < 0.999f) || o.draw_part == 1) &&
                            g_res.rasterizer_front != nullptr;
    ID3D11RasterizerState* want_rs =
        ts_ordered ? g_res.rasterizer_front :
        o.two_sided    ? g_res.rasterizer : g_res.rasterizer_cull;

    if (want_rs != bound_rs) {
        ctx->RSSetState(want_rs);
        bound_rs = want_rs;
    }

    vp_select(nz_this ? 1 : 0);   // FLUSH_NEARZ.

    const int mid = mesh_id_clamp(o.mesh);

    // WSEE: an injection's deferred copy draws from its hold entry (wsee_hold_ref: a copy entry the vertices
    // the injection drew with, an own entry - a static part, or one drawn from its mesh's buffer at the injection
    // - the mesh's own buffer, whatever the slot holds now) at the injection's level and crossfade
    // (wsee_hold_lod: a copy holds those levels' translucent vertices alone), not a pick of its own. The
    // append above dropped a void entry (the hold gone, or an own entry's mesh buffer gone).
    const VbRef vb = o.ws_hold != 0u ? wsee_hold_ref(o) : mesh_vb_ref(mid, o.slot);   // CLOTH.
    if (vb != bound_vb) {
        ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
        ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
        bound_vb = vb;
    }

    const MeshDef& md = mesh_def(mid);
    int lod = 0;
    float lodt = 0.0f;
    // A locked instance never picks - level 0, no fade. Twin at the injection.
    if (o.ws_hold != 0u) {
        wsee_hold_lod(o, lod, lodt);   // WSEE: the injection's pick.
    } else if (!lod_locked(o)) {   // LOD_SIM_SLOT.
        lod_pick(md, lod_radius(o), sqrtf(g_dist_memo_flush.get(o, cam)), lod, lodt);
    }
    const bool lodx = lodt > 0.0f;
    // Buckets, flush edition: this object is its bucket's representative. Its CB is up (textured: built, uploaded
    // per submesh by the draw) and its PS/dss/blend bound; swap in the instanced VS + layout, bind the lane ring
    // once per pass, draw the emitted ranges (every member visible at emission, sharing the route by key, at its
    // own LOD level, crossfade included). A two-sided bucket draws CullNone. Twin at the injection.
    // WSEE: the object's geometry as one call, so the tail's re-draws (wsee_tail, below) are this same
    // draw - wrep: such a re-draw, which counts nothing (a bucket an earlier call unbatched stays unbatched).
    auto geo = [&](bool wrep) {
        int lod_iters = lodx ? 2 : 1;

        if (inst_on && inst_plan.batch_of[oi] >= 0) {
            // SKIN_POOL: a skin pool chunk's bucket pulls (the injection's twin).
            const SkinPoolChunk* const ipc = inst_bucket_pool(inst_plan, oi);
            ID3D11VertexShader* ivs = ipc ? (tx_on ? g_res.vs_inst_sk_tex : g_res.vs_inst_sk)
                                                  : (tx_on ? g_res.vs_inst_tex : g_res.vs_inst);
            ID3D11InputLayout*  iil = ipc ? g_res.layout_inst_sk
                                                  : (tx_on ? g_res.layout_inst_tex : g_res.layout_inst);

            if (ivs && iil) {
                if (ivs != bound_vs) { ctx->VSSetShader(ivs, nullptr, 0); bound_vs = ivs; }
                if (iil != bound_il) { ctx->IASetInputLayout(iil); bound_il = iil; }
                inst_sk_bind(ctx, ipc, bound_sk);

                if (!inst_vb_bound) {
                    const UINT is = INST_STRIDE, io = 0;
                    ctx->IASetVertexBuffers(1, 1, &g_res.inst_vb[0], &is, &io);
                    inst_vb_bound = true;
                }

                if (o.two_sided && !ts_ordered && bound_rs != g_res.rasterizer) {
                    ctx->RSSetState(g_res.rasterizer);
                    bound_rs = g_res.rasterizer;
                }

                uint32_t in = 0;
                const uint32_t id = inst_draw(ctx, dev, inst_plan, meshes, oi,   // BUCKETS.
                    obj_cbd, g_res.constant_buffer, tx_on ? txm : nullptr, ts_ordered, bound_rs,
                    arb_this ? 2 : 0, arb_this ? g_res.ps_comp_arb_tex : g_res.ps_tex,
                    1, in);   // Part 1: a bucketed object is never a part copy (inst_eligible).
                if (tx_on && !wrep) stat_add(g_stats.textured_draws, id);
                if (ipc && !wrep) stat_add(g_stats.skin_inst, in);   // SKIN_POOL.
                lod_iters = 0;   // The batch drew; no per-object level walk.
            } else {
                inst_unbatch(inst_plan, oi);   // INST_UNBATCH.
            }
        }

        for (int li = 0; li < lod_iters; ++li) {
            const int lvl = lod + li;

            if (lodx) {
                obj_cbd.blend_ctl[3] = lod_dither(lodt, li != 0);
                // TX_ONE_UPLOAD: textured, draw_textured uploads the dither with each submesh.
                if (!tx_on && !upload_obj_cb(ctx, g_res.constant_buffer, obj_cbd)) break;
            }

            if (tx_on) {
                const uint32_t txd = draw_textured(ctx, dev, obj_cbd, g_res.constant_buffer,
                                                   *txm, mid, ts_ordered, bound_rs,
                                                   arb_this ? 2 : 0,   // The arb variant's user twin.
                                                   arb_this ? g_res.ps_comp_arb_tex : g_res.ps_tex,
                                                   lvl,
                                                   o.draw_part == 1 ? 2 : 1);   // MAT_BLEND part.
                if (!wrep) stat_add(g_stats.textured_draws, txd);
            } else {
                UINT ls = 0, lc = 0;
                mesh_lod_range(md, lvl, ls, lc);

                // The ordered contract restarts per level: iteration 1 left the cull rasterizer bound, and
                // interiors must precede exteriors for this level too.
                if (ts_ordered && bound_rs != g_res.rasterizer_front) {
                    ctx->RSSetState(g_res.rasterizer_front);
                    bound_rs = g_res.rasterizer_front;
                }

                ctx->DrawIndexed(lc, ls, 0);

                if (ts_ordered) {   // Pass 2: front faces over the interior.
                    ctx->RSSetState(g_res.rasterizer_cull);
                    bound_rs = g_res.rasterizer_cull;
                    ctx->DrawIndexed(lc, ls, 0);
                }
            }
        }
    };
    // WSEE: a translucent part in a sorting run draws after the game's layers, composited under
    // the nearer ones (cb.hlsl KhWseeMeshCut / KhWseeMeshOut on PSMain - the textured twin, its user-material
    // twins, PSWhite in place of a pending one; the composite unit never takes the lane): the tail's
    // normal-blend parts that test without writing (the injection's deferred ones among them), no near-gap
    // route. The lanes go into the object's slice, which draw_textured uploads per submesh: fx_tex[1].z = the
    // mode, fx_fog[0] = the decode (both zero on every other mesh draw - upload_cb rebuilds the slice from the
    // zeroed template each object, and the replay below zeroes them). The capture is checked again per part
    // (wsee_tail_owed): a depth-writing draw earlier in this run taints it (the loop's own check, at the dss
    // selection), and the record no longer holds the opaque depth then.
    const bool ws_tail = ix < def_n || ix >= solid_n;   // The deferred front parts too.
    const bool ws_elig = ws_tail && o.draw_part == 1 && tx_on && dss == g_res.dss_test &&
                         o.effect == 0 && o.blend_mode == 0 && !mm_rep && !o.fullscreen &&
                         o.mode != DepthMode::Off && !nz_this;
    int ws_sort_this = (ws_elig && ws_mode > 0 && wsee_tail_owed()) ? ws_mode : 0;
    if (ws_sort_this >= 2 && ws_q == 0) {
        // WSEE_BWANT: the composite reads B / B2 (KhWseeMeshCut) - its want from the next clear, and
        // without them this cycle the run's parts take mode 1, as without Q.
        g_wsee.col_want_cycle.store(g_topo_cycles, std::memory_order_relaxed);
        const bool ws_col = g_wsee.col_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
        ws_q = (ws_col && wsee_q_ensure(dev) && wsee_q_capture(ctx)) ? 1 : -1;
    }
    if (ws_sort_this >= 2 && ws_q < 0) ws_sort_this = 1;
    WseeSortScope wss;
    if (ws_sort_this > 0) {
        if (wss.begin(ctx, ws_sort_this)) {
            obj_cbd.fx_tex[1][2] = static_cast<float>(ws_sort_this);
            obj_cbd.fx_tex[1][3] = wss.b2m ? 8.0f : 0.0f;   // WSEE_B2M: bit 3, the B2 moment.
            if (wss.od) obj_cbd.fx_tex[1][3] += 32.0f;   // Bit 5, D3 at t65.
            if (wss.pb) obj_cbd.fx_tex[1][3] += 64.0f;   // PSB: bit 6, P0 / P1 / N / N2 at t66 - t69.
            if (wss.pe) obj_cbd.fx_tex[1][3] += 128.0f;   // PSB: bit 7, the old record's NW unread.
            if (wss.np) obj_cbd.fx_tex[1][3] += 256.0f;   // WSEE_NOPRE: bit 8, pre derived;
            if (wss.s2) obj_cbd.fx_tex[1][3] += 512.0f;   // bit 9, S2 at t62.
            memcpy(obj_cbd.fx_fog[0], ws_dec, sizeof(ws_dec));
        } else {
            ws_sort_this = 0;
        }
    }
    geo(false);
    wss.end();
    // WSEE: our translucent tail into the cycle's capture, as an engine blended no-write draw goes (the
    // WSEE note): past the solids (and the injection's deferred parts, drawn ahead of them: ws_tail),
    // testing the depth without writing it, on normal blend,
    // no effect mesh (unarmed; its output is the effect's, not a coverage), no depth-Off overlay, no near-gap
    // route (below the world's range, where no pixel is covered). The T pass needs the coverage as the alpha: a
    // perceptual translucent writes its own mix opaque, so its arm (blend_ctl.x) is dropped for that pass and put
    // back after - an untextured draw uploads it here, a textured one with each submesh (draw_textured); NW's
    // pass reads only the depth and the pixel shader's discards, so it takes the T pass's constants as they are.
    if (ws_tail && dss == g_res.dss_test && o.effect == 0 && o.blend_mode == 0 && !mm_rep &&
        !o.fullscreen && o.mode != DepthMode::Off && !nz_this && wsee_tail_owed()) {
        const float wpc = obj_cbd.blend_ctl[0];
        bool wup = true;   // A failed upload draws neither pass (T would take the opaque mix).
        // A sorted part re-draws as the plain tail does - the hardware test, its lanes off (the T pass binds T,
        // which the sorted draw's pixel shader reads at t54; the plain output's alpha is the same coverage).
        obj_cbd.fx_tex[1][2] = 0.0f;
        obj_cbd.fx_tex[1][3] = 0.0f;
        memset(obj_cbd.fx_fog[0], 0, sizeof(obj_cbd.fx_fog[0]));
        const bool wok = wsee_tail(ctx, [&](bool wt) {
            if (wt) {
                obj_cbd.blend_ctl[0] = 0.0f;
                if (!tx_on) wup = upload_obj_cb(ctx, g_res.constant_buffer, obj_cbd);
            }
            if (wup) geo(true);
        });
        obj_cbd.blend_ctl[0] = wpc;
        if (!wok || !wup) wsee_taint(g_topo_cycles);   // T / NW would miss it.
    }

    obj_cbd.blend_ctl[3] = 0.0f;   // Never leaks into the next object.
    // FX_CAP_ORDER: this object drew (every skip above left before here) - a bucket's draw at its
    // representative drew the members with it, which are of its kind.
    cap_stale = true;
    if (o.effect == 0) cap_stale_ne = true;
}

inline void FlushPass::at_tail_start(size_t ix) {
    // WSEE: past the injection's deferred parts, the scene capture this flush's effect meshes or perceptual
    // translucents read is taken again, so they see our parts behind them as they did when the injection drew
    // those - by FX_CAP_ORDER's flags, which every drawn part sets (the loop's end), before the first reader
    // (FX_CAP_LAZY: not here, where a solid or the SSAO apply between would stale a capture taken now again).
    // WSEE: the flush's own tail takes a fresh Q - its solid section (effect meshes, other
    // blends) drew since the deferred parts' one.
    if (ix == solid_n && def_n > 0 && ws_q == 1) ws_q = 0;
    if (ix == solid_n && !ssao_done) {   // SSAO: the tail starts here.
        ssao_done = true;
        if (ssao.on) {
            // t1 down before the writable DSV comes back (a live depth SRV bound against its writable view is
            // unbound by the runtime silently, and the category tracker would not know); the next draw rebinds its
            // pair whole.
            ID3D11ShaderResourceView* t1_none = nullptr;
            ctx->PSSetShaderResources(1, 1, &t1_none);
            dsv_swap(false);
            ssao_post(ctx, ssao);
            srv_cat = -1;
            if (ssao.term) {   // FX_CAP_ORDER: the term's apply drew over our solids.
                cap_stale = true;
                cap_stale_ne = true;
            }
        }
    }
}

inline void FlushPass::refresh_capture_for_perceptual(bool o_perc) {
    // FX_CAP_ORDER: a perceptual translucent composites against the scene as it stands - a draw of ours
    // since the capture (a translucent, a blended mesh, a part, a fading effect mesh, the SSAO post) is in it.
    // FX_CAP_LAZY: the flush's first (cap_stale starts true); its failure before any capture stood at t3
    // stands the perceptual composite down for the flush - this translucent and the rest draw plain (blend_ctl.x,
    // read by upload_cb below); a later failure keeps the capture that stands.
    if (perceptual && o_perc && cap_stale) {
        ++cap_gen;   // GLOW_PYR.
        if (scene_capture_timed(dev, ctx).empty()) {
            ctx->PSSetShaderResources(3, 1, &g_res.scene_srv);
            srv_cat = -1;   // t0/t1 pair may have been rebound; force re-swap below.
            cap_stale = false;
            cap_stale_ne = false;
            cap_t3 = true;
        } else if (!cap_t3) {
            perceptual = false;
        }
    }
}

inline void FlushPass::refresh_capture_for_effect(const RenderObject& o) {
    // FX_CAP_ORDER: an effect mesh's source (t0) is the scene as it stands - a blended mesh, a part, a
    // translucent or a solid of this flush drawn since the capture is in it (an effect mesh is not: see the
    // flags' note). Past the skips above (a skipped mesh reads nothing), before the pyramids built from it.
    if (o.effect > 0 && !o.fullscreen && cap_stale_ne) {
        ++cap_gen;   // GLOW_PYR: the pyramids follow the capture.
        if (scene_capture_timed(dev, ctx).empty()) {
            ctx->PSSetShaderResources(3, 1, &g_res.scene_srv);
            srv_cat = -1;   // t0/t1 pair may have been rebound; force re-swap below.
            cap_stale = false;
            cap_stale_ne = false;
            cap_t3 = true;   // FX_CAP_LAZY.
        }
    }
}

inline void FlushPass::prepare_pyramids(const RenderObject& o, ID3D11PixelShader* ufx, bool upyr) {
    // GLOW_PYR: before upload_cb, whose slice the build's own upload would replace.
    glow_draw = nullptr;
    if ((!ufx && glow_effect(o.effect)) || (ufx && upyr)) {   // FX_USER: or a user mesh's.
        const bool b = glow_bright(o.effect);
        const float t = b ? o.fx[0] : 0.0f;
        if (glow_gen != cap_gen || glow_b != b || glow_t != t) {
            glow_ok = glow_build(dev, ctx, g_res.glow_pyr[0], ps_srvs[0], screen_w, screen_h, 0.0f,
                                 o.effect, o.fx[0], g_res.constant_buffer);
            glow_gen = cap_gen;
            glow_b = b;
            glow_t = t;
        }
        if (glow_ok) glow_draw = &g_res.glow_pyr[0];
    }
    ana_draw = nullptr;   // ANA_PYR: the same rule, per axis.
    if (!ufx && o.effect == static_cast<int>(EffectId::Anamorphic)) {
        const int ax = o.fx[4] > 0.5f ? 1 : 0;   // fxParams1.x (fill_fx_params_cb).
        const float t = o.fx[0];
        if (ana_gen[ax] != cap_gen || ana_t[ax] != t) {
            ana_ok[ax] = ana_build(dev, ctx, g_res.ana_pyr[0][ax], ps_srvs[0], screen_w, screen_h,
                                   0.0f, ax, t, g_res.constant_buffer);
            ana_gen[ax] = cap_gen;
            ana_t[ax] = t;
        }
        if (ana_ok[ax]) ana_draw = &g_res.ana_pyr[0][ax];
    }
}

inline void FlushPass::bind_object_shaders(const RenderObject& o, ID3D11PixelShader* ufx, int ps_want,
                                          bool tx_on) {
    if (ufx) {
        // Custom PS per object: bind unconditionally and poison the
        // category so the next builtin object rebinds its own.
        ctx->PSSetShader(ufx, nullptr, 0);
        ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // FX_USER: s1 for every user effect draw.
        fx_tex_bind(ctx, utex);   // FX_TEX: this object's (upload_cb filled them); off at the next one.
        t43 = true;
        ps_cat = -1;
    } else {
        if (ps_want != ps_cat) {
            ctx->PSSetShader(ps_want == 1 ? g_res.ps_effect
                           : ps_want == 2 ? g_res.ps_composite_arb
                           : ps_want == 3 ? g_res.ps_tex
                           : ps_want == 4 ? g_res.ps_comp_arb_tex
                                              : g_res.ps, nullptr, 0);
            ps_cat = ps_want;
        }
        // FX_TEX_ALL: per object, outside the category cache (builtin effect meshes share one category): a
        // builtin effect mesh with a set binds its pages; any other object takes the last one's off (a user
        // mesh's included). A textured solid binds its own maps there afterwards.
        if (o.effect > 0 && o.fx_tex) {
            fx_tex_bind(ctx, utex);
            t43 = true;
        } else if (t43) {
            fx_tex_unbind(ctx);
            t43 = false;
        }
    }
    {
        ID3D11VertexShader* want_vs = tx_on ? g_res.vs_tex : g_res.vs;
        if (want_vs != bound_vs) {
            ctx->VSSetShader(want_vs, nullptr, 0);
            bound_vs = want_vs;
        }
        ID3D11InputLayout* want_il = tx_on ? g_res.layout_tex : g_res.input_layout;
        if (want_il != bound_il) {
            ctx->IASetInputLayout(want_il);
            bound_il = want_il;
        }
    }
}

inline void FlushPass::bind_object_srvs(const RenderObject& o) {
    // PSMain (the flush's solid PS; PSComposite on the near-gap ARB route) reads depth at t0; the flush's default
    // pair puts scene colour there (for effect shaders). Every solid mesh swaps the proven depth snapshot into t0
    // for its draw; the pair returns for effect meshes.
    // FLUSH_NOSNAP: a solid with no snapshot (category 2) takes the same binds with t0 null (read as no
    // scene; its guard stands down, fx1[0] = 1e9) - the pair put the live depth (or null) at t1, where the
    // receive Gathers the atlas (INJ_ATLAS twin).
    const int srv_want = o.effect != 0 ? 1 : (g_res.comp_depth_srv ? 0 : 2);

    if (srv_want != srv_cat) {
        if (srv_want != 1) {
            ID3D11ShaderResourceView* const t0 = srv_want == 0 ? g_res.comp_depth_srv : nullptr;
            ctx->PSSetShaderResources(0, 1, &t0);

            // The pre-resolve snapshot at t21, the post-resolve one at t22 (khShadowPre / khShadowPost). Twin: the
            // injection carries the identical block.
            if (svs_sten_on()) {
                if (g_svs.pre_srv) {
                    ctx->PSSetShaderResources(21, 1, &g_svs.pre_srv);
                }

                if (g_svs.post_srv) {
                    ctx->PSSetShaderResources(22, 1, &g_svs.post_srv);
                }
            }
            // The volume copy's stencil plane (t24), on its own arm.
            if (svs_vol_ready()) {
                ID3D11ShaderResourceView* const sten =
                    rp_use_merged() ? g_rp.msten_srv : g_svs.vol_sten_srv;   // VOL_REPLAY.
                ctx->PSSetShaderResources(24, 1, &sten);
                // VOL_WITNESS: its depth plane (t23). Twin: the injection.
                if (g_svs.vol_depth_srv) ctx->PSSetShaderResources(23, 1, &g_svs.vol_depth_srv);
                // VOL_FOOT: its footprint mask (t33). Twin: the injection.
                ID3D11ShaderResourceView* const foot = rp_use_merged() ? g_rp.mfoot_srv : svs_foot_bound();
                if (foot) ctx->PSSetShaderResources(33, 1, &foot);
            }
            if (g_vmir.srv && g_vmir.mask_time >= 0.0f) {
                ID3D11ShaderResourceView* const mir = rp_mir_srv();   // MIR_REPLAY.
                ctx->PSSetShaderResources(28, 1, &mir);
            }
            // Null when no atlas exists - the shader's shadowMeta.x = 0 short-circuit protects, and a stale depth
            // SRV must never masquerade as the atlas.
            ID3D11ShaderResourceView* atlas =
                shadow_live_ensure_srv() ? g_ls.atlas_srv : nullptr;
            ctx->PSSetShaderResources(1, 1, &atlas);
        } else {
            dsv_swap(true);   // Read-only depth before the depth SRV lands at t1.
            ctx->PSSetShaderResources(0, 2, ps_srvs);
        }
        srv_cat = srv_want;
    }
}

inline ID3D11DepthStencilState* FlushPass::bind_object_blend_depth(const RenderObject& o) {
    const int bm_want = mm_rep ? 0 : o.blend_mode;   // BLEND_MINMAX: the replacing draw.
    if (bm_want != bound_bm) {
        ctx->OMSetBlendState(g_res.blend_modes[bm_want], bf, 0xFFFFFFFF);
        bound_bm = bm_want;
    }

    // Effect meshes never write depth. The injection depth-writes every composite-eligible solid by contract.
    // SSAO_MARK: a depth write marks stencil 1 ONLY while this park's own post is still ahead of it
    // (ssao.on, the tail not yet reached), because the post's eraser is the only thing that clears a mark. A
    // TestWrite draw on a park with no post (the injection owns the eligible meshes, or nothing is eligible), or
    // one in the tail past the post, would leave a mark standing for the next injection's pass to take its term at
    // - pixels it never painted.
    const bool mark = ssao.on && !ssao_done;
    ID3D11DepthStencilState* const dss_w = mark ? g_res.dss_test_write_mark : g_res.dss_test_write;
    // SSAO_SEE_THROUGH: a draw on a blend mode that shows the scene behind it (additive, multiply, screen,
    // lighten, darken) marks 3, not 1. The term reads 1 alone (KhSaOurs, the apply's mask), so it leaves such a
    // pixel as it leaves the engine's and does not darken what is seen through the mesh; the near-z marker
    // and the eraser read bit 0 and still take it. Normal blend marks 1: its translucents draw in the
    // tail past the post (ssao_done), and the injection draws composite-eligible solids alone.
    const UINT mark_ref = (o.blend_mode == 0) ? 1u : 3u;
    ID3D11DepthStencilState* dss =
        (fx_arb_this)                ? g_res.dss_off :
        (o.mode == DepthMode::Off)       ? g_res.dss_off :
        (o.draw_part == 1)               ? g_res.dss_test :   // MAT_BLEND: a translucent
                                                              // Part never writes depth.
        (o.effect != 0)                  ? g_res.dss_test :   // FX_NODEPTH: in any mode.
        (o.mode == DepthMode::TestWrite) ? dss_w :
        is_composite_eligible(o)         ? dss_w :
                                           g_res.dss_test;

    // Read-only DSV reconciliation: a pair consumer (depth at t1) cannot write the depth it reads, so its write arm
    // is a plain test; a solid that writes takes the real DSV back (t1 is the atlas there, no hazard).
    if (dsv_ro && dss == dss_w) {
        if (srv_cat == 1) dss = g_res.dss_test;
        else                  dsv_swap(false);
    }
    // WSEE: a solid writes the main depth (late). WSEE_FLUSHW_PRED: noted where it follows a landed capture
    // or falls in a cycle the hold stood down.
    if (dss == dss_w) {
        const uint64_t wc = g_topo_cycles;
        wsee_taint(wc);
        if (g_wsee.ok_cycle.load(std::memory_order_relaxed) == wc ||
            g_wsee_flushw_off.load(std::memory_order_relaxed) == wc) {
            g_wsee_flushw_cycle.store(wc, std::memory_order_relaxed);
        }
    }

    const UINT ref = (dss == g_res.dss_test_write_mark) ? mark_ref : 0u;   // SSAO_SEE_THROUGH.
    if (dss != bound_dss || ref != bound_ref) {
        ctx->OMSetDepthStencilState(dss, ref);
        bound_dss = dss;
        bound_ref = ref;
    }
    return dss;
}

inline void FlushPass::finish_meshes() {
    if (g_wsee.defer_taken == g_topo_cycles) g_wsee.defer_drawn = g_topo_cycles;   // WSEE: the copies drew.

    if (!ssao_done && ssao.on) {   // SSAO: no translucent tail - after the last solid.
        ID3D11ShaderResourceView* t1_none = nullptr;   // As at the tail site; the chain rebinds t1 itself.
        ctx->PSSetShaderResources(1, 1, &t1_none);
        dsv_swap(false);
        ssao_post(ctx, ssao);
        srv_cat = -1;
    }

    // Restore the parked pass's own viewport (the depth-range override covered only the mesh draws).
    if (n_saved_vp > 0) ctx->RSSetViewports(n_saved_vp, saved_vp);

    // Chain passes below assume the ambient (CullNone) rasterizer.
    if (bound_rs != g_res.rasterizer) ctx->RSSetState(g_res.rasterizer);
}

inline void FlushPass::run_chain() {
    // Exactly one chain per cycle, whichever thread. A park that lands in a cycle the render thread already flushed
    // (g_rtshadow_cycle) draws no chain, and the render thread draws none when a park drew the chain first
    // (g_fx_park_cycle); otherwise the effects doubled for that frame.
    const bool chain_owed = (!rt || chain_only || g_fx_park_cycle != g_topo_cycles) &&
                            (rt || g_rtshadow_cycle.load(std::memory_order_relaxed) != g_topo_cycles);
    if (!fullscreen.empty() && effects_ready && chain_owed) {
        if (rt && !chain_only && fx_late_take()) {
            // FX_LATE: owed to a later resolve, past the view model.
            vmsee_arm(dev, fullscreen);   // VM_SEE: a depth reader there sees past its translucent planes.
            wsee_arm(dev, fullscreen);    // WSEE: and through the world's blended layers.
        } else {
            if (chain_only) g_fx_late.ran_cycle.store(g_topo_cycles_pub.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            // PARK_CHAIN_STAMP: stamped here, not at entry - a park that returned early (no DSV, wrong pass)
            // or had no chain to run left the render thread's chain refused for a cycle nobody drew it in.
            if (!rt) g_fx_park_cycle = g_topo_cycles;
            // CHAIN_RT0_DOC: the run takes the bound RT0 as the scene (its capture and its final write), as this
            // flush's mesh draws do; at a resolve it is not compared with the resolve's source. Were the two to
            // differ, the chain would read and write RT0 and the resolved frame would carry none of it; a refusal
            // keyed on the scene's name (a fingerprint's, SCENE_NAME) would turn the chain off wherever that name
            // is wrong and RT0 right.
            dsv_swap(true);   // The chain samples live depth; its OM save must hold the read-only view.
            auto needs_depth = [](const RenderObject& o) { return fx_needs_depth(o); };   // NEARZ_READER.
            auto upload_fn = [this](const RenderObject& fu_o, bool chain, ConstantData* defer) {
                return upload_cb(fu_o, chain, defer);
            };
            fx_chain_run(dev, ctx, fullscreen, bf, ps_srvs[1], !meshes.empty() || cap_defer,   // CAP_DEFER.
                         upload_fn, needs_depth, now_d, &lcull);   // LOCAL_CULL; FUSE_CLOCK.
        }
    }
}

inline void flush_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool rt, bool chain_only) {
    KH_PROF_SCOPE(PROF_FLUSH);
    KH_GPU_SCOPE(ctx, GPU_FLUSH);   // PROF.
    // A park that runs the post chain stamps its cycle (g_fx_park_cycle, at the chain below): the render-thread flush
    // draws no chain in it. The housekeeping block: a render-thread reader must not see a resized slot table, a
    // released buffer or a half-written Map; with the flush on the render thread that reader is this thread, so the
    // block belongs to the frame's only flush.
    if (!chain_only) flush_housekeeping(dev, ctx);   // FX_LATE: the scene flush did the housekeeping.
    flush_dl_fallback_harvest(ctx, rt, chain_only);   // DL_FALLBACK_HARVEST.
    flush_publish_frame(chain_only);   // The staged world lighting and the flush stamps.
    flush_step_late(ctx, rt, chain_only);   // NO_PARK_LATE.

    bool has_objects;

    scene_sync();   // SCENE.
    has_objects = g_scene.alive_n > 0;

    // Require a DSV and require it to be the main scene's depth, not a PiP/mirror/UAV sub-pass. Drawing into a sub-pass
    // misses the visible frame and churns the depth SRV cache against the wrong resource.

    if (!flush_adopt_main_depth(ctx, rt)) return;

    if (!flush_ensure_resources(dev, ctx)) return;   // A deferred ensure is not a failed one. The frame still ends.
    user_shader_reap();
    user_shader_pump(dev);
    // The scene-flush ordinal is the GAME THREAD's clock - the UI phase machine arms on it advancing once per frame
    // ahead of the UI pass, and flush_ui_locked reads a jump of two as a dark driver and skips every pass - so
    // flush_frame bumps it, never this body.

    if (!has_objects) return;

    // Snapshot the visible objects, draw without holding the mutex. Meshes and fullscreen passes are split; fullscreen
    // passes run last, in creation order so chained effects compose deterministically.
    // The pass's scratch, kept across calls so they never allocate per frame (each stage clears, fills or rebuilds
    // what it uses before reading it).
    static std::vector<RenderObject> meshes;
    static std::vector<std::pair<uint64_t, RenderObject>> fullscreen;   // Key = creation seq.
    static InstPlan inst_plan;
    static std::vector<uint32_t> order;
    FlushPass pass(dev, ctx, rt, chain_only, meshes, fullscreen, order, inst_plan);
    pass.judge_landing();   // Did the injection land in the cycle these late draws go into (comp_healthy)?
    pass.collect_scene();   // The walk: the late meshes and fullscreen passes; a park erases the expired.

    // SCENE_HOLD: from here to the flush's end the scene stays the one the walk above copied - the passes below
    // re-synced it (dls_caster_dist, render_sun_depth, dls_frame), and a stage the game thread wrote meanwhile
    // put the buckets' records (objbuf_sync, below) ahead of the copies they draw: a moved mesh culled at its old
    // pose and drawn at its new one, a reused slot's record under another object's mesh.
    const SceneHold scene_hold;

    pass.render_shadow_maps();   // dls_select; with no landing, the sun and light-shadow maps (SUN_GEN).
    if (!pass.decide_late_work()) return;   // Nothing to draw or snapshot for (INFRONT_TPL aside).
    if (!pass.pick_projection()) return;    // The camera and the depth pair the late draws encode with.
    pass.take_occlusion_snapshot();
    if (pass.nothing_late && !pass.infront_tpl) {
        return;
    }
    pass.build_inverse();      // The depth readers' reconstruction frame (FX_FRAME_ADOPT) and LOCAL_CULL.
    pass.adopt_camera();       // cam through the honest inverse; ENGCAM; FOG_PROBE's camera.
    flush_track_rain(pass.cam, pass.pv);
    pass.measure_viewport();   // The screen size; LOD_ONE_SCALE's publish.
    if (pass.nothing_late) {   // INFRONT_TPL (decide_late_work): the template alone, nothing drawn.
        pass.publish_frame_template();
        return;
    }
    if (!pass.prepare_effects()) return;   // Capability gating: the capture, the depth SRV, the demotions.
    pass.draw();   // The mesh pass, the SSAO term and the chain, under one StateBackup.
}
}