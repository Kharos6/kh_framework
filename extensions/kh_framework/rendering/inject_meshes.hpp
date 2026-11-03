#pragma once

// The injection half (rendering_integration_injection.hpp): the injection site (inject_composited_meshes) and its haze
// lanes. Never include it on its own.
namespace RenderIntegration {

// Engine distance haze fill (one truth; the injection, the flush's fx_frame_template and the world pass's
// dlsw_fog_lanes call it). Modes 1 and 3 share the case; any other mode is one this code has never seen and it
// stands down.
inline void fill_haze(ConstantData& cb) {
    cb.haze_pars[0] = 0.0f;
    cb.haze_pars[1] = 0.0f;
    cb.haze_pars[2] = 0.0f;
    cb.haze_pars[3] = 0.0f;
    cb.fog_sky_col[3] = 0.0f;

    // PROBE_META40: the light lanes below (nb[44], nb[50], nb[53]) sit at those indices only in a mirror whose
    // lock has the full 40-float head (meta == 40), the test the fog colour and engine terms take.
    if (g_sky_probe.hits == 0 || g_light_probe.hits == 0 || g_light_probe.meta != 40) return;

    // fog-model selector: the lane holds an integer, reinterpreted.
    uint32_t mode = 0;
    memcpy(&mode, &g_light_probe.nb[44], sizeof(mode));
    if (mode != 1u && mode != 3u) {  return; }

    const float ref = g_sky_probe.nb[56];
    const float den = g_sky_probe.nb[57];
    const float fal = g_sky_probe.nb[58];

    // Finite, positive, and inside the range an atmosphere can occupy. Absent beats wrong.
    if (!(den > 0.0f && den < 1.0f) ||
        !(fal > 1.0e-6f && fal < 1.0e-1f) ||
        !(ref > -1.0e4f && ref < 1.0e4f)) {  return; }

    cb.haze_pars[0] = ref;
    cb.haze_pars[1] = den;
    cb.haze_pars[2] = fal;
    cb.haze_pars[3] = 1.0f;
    // The layer the integral branches on, carried in the spare colour lane.
    cb.fog_sky_col[3] = g_light_probe.nb[50] + g_light_probe.nb[53];
}

// No owner prepass: the KH_ARB_DEPTH variant writes SV_Depth, which disables early-Z, so LESS_EQUAL cannot order two of
// our meshes against each other on far-arb frames. The software depth prepass was retired: its rasterization into a
// clone of the MSAA engine DSV cost more than any overdraw it saved (VIS_OCC already culls the fully hidden
// majority). Do not restore it unless it is armed on measured overlap, never on a mesh count. Conservative depth output
// cannot help (the clamp pulls NEARER). What remains: ordinary overdraw, and on far-arb frames a mesh whose clamp
// pulled it further than a neighbour's can win a pixel it is behind (both within FAR_ARB_CONTACT_H of the terrain,
// both behind it, overlapping). If it shows, bound the clamp so our meshes cannot reorder each other.
// SESSION_RESET: at file scope so the session reset reaches it.
static bool g_inj_arb_prev_armed = false;   // Injection site is render-thread-only.

// ---- inject_composited_meshes' stages -------------------------------------------------------------------------------
// inject_composited_meshes (at the end of this file) reads as its stage list; one injection's working state is a
// InjectPass and the stages are its methods. Render thread only: the injection runs inside the engine's main pass, at
// the reorder hook's trigger (reorder_trigger).

// Per-mesh route verdict of the injection (InjectPass::route_of).
struct InjRoute { bool nearz; };

// BLEND_PART_INJ - the translucent part draws where the solid draws.
struct InjPart {
    uint32_t oi;
    ConstantData cbd;   // The object's slice as its solid drew; mat lanes refilled per submesh by
                        // the draw, light lanes re-appended at the tail (DL_RING).
    ID3D11PixelShader* ps;
    ID3D11VertexShader* vs;
    ID3D11InputLayout* il;
    const SkinPoolChunk* pool = nullptr;   // SKIN_POOL: a bucket part's chunk (its Sk twin's t52).
    int ctx;   // draw_textured's pipeline variant (user-shader twin selection).
    int lod;
    float lodt;
    bool lodx;
    int vpsel;   // The viewport this object drew under (vp_select's code).
    bool ts_ordered;
    bool inst;   // The part is a bucket's: inst_draw part 2, one tail entry per lane (INST_PART_LANE).
    float dist2;
    bool ws_defer = false;   // WSEE: left to the flush's tail (its objects noted; not drawn here).
};
// The far-to-near order, sorted as entries naming a record (a stable_sort on the records themselves wants an
// aligned_storage temporary MSVC refuses; the records never move). INST_PART_LANE: a bucket's part is one entry
// per lane (lane = its ring lane, lvl its level, d2 its member's distance), so each lane takes its own place among
// the other parts - the bucket drew all of them at its representative's place; a per-object part is one entry (lane
// ~0u, d2 = its dist2).
struct InjPartEnt { uint32_t part; uint32_t lane; int lvl; float d2; };

// InjectPass - one run of inject_composited_meshes: the eligible set, the camera and depth pair, the pass verdicts, the
// frame template and the draw trackers its stages hand each other. Lives on the render thread's stack for one call.
// The members keep the names the locals had (the comments across the renderer cite them), grouped by the stage that
// first sets them; a member with no initialiser is written by its stage before any read, as the local was.
struct InjectPass {
    // ---- inputs (the context and the per-call scratch, kept across calls for capacity only) ----
    ID3D11DeviceContext* ctx;
    std::vector<RenderObject>& meshes;     // This pass's draws (cull_candidates' copies).
    std::vector<RenderObject>& occx;   // OCC_FORCE: declared occluders this pass does not draw.
    std::vector<uint32_t>& elig;       // SCENE: every eligible slot, pre-cull independent.
    InstPlan& inst_plan;             // Buckets, injection edition.
    VisOcc& occ;                     // The occlusion cull (build_cull).
    std::vector<InjPart>& parts;     // BLEND_PART_INJ: the staged translucent parts.

    InjectPass(ID3D11DeviceContext* ip_ctx, std::vector<RenderObject>& ip_meshes,
               std::vector<RenderObject>& ip_occx, std::vector<uint32_t>& ip_elig, InstPlan& ip_inst_plan,
               VisOcc& ip_occ, std::vector<InjPart>& ip_parts)
        : ctx(ip_ctx), meshes(ip_meshes), occx(ip_occx), elig(ip_elig),
          inst_plan(ip_inst_plan), occ(ip_occ), parts(ip_parts) {}
    InjectPass(const InjectPass&) = delete;
    InjectPass& operator=(const InjectPass&) = delete;

    // ---- walk_eligible ----
    float snapshot_now = 0.0f;   // The effect clock the eligibility (lifetimes, envelopes) is judged at.
    uint32_t elig_n = 0;
    bool     any_caster = false;
    uint64_t elig_gen = 0;   // OBJBUF_LATE: the scene generation elig was taken at.
    // ---- ensure_device ----
    ID3D11Device* dev = nullptr;
    // ---- pick_projection / choose_depth_pair: the camera and the depth pair the pass encodes with ----
    RVExtBridge::ProjectionViewTransform pv = {};
    float latch_view[16];
    bool measured = false;   // The engine's own projection pair was measured this cycle.
    // The telltale's snapshot (choose_depth_pair): every decision below reads these, not g_ro.
    float snap_slot_near = 0.0f;
    float snap_slot_m22 = 0.0f;
    float snap_slot_m32 = 0.0f;
    bool snap_world_valid = false;
    float snap_world_m22 = 0.0f;
    float snap_world_m32 = 0.0f;
    float cam_near = 0.0f;   // PART_NEAR: the camera's own near, before any pair replaced it.
    bool far_phase = false;   // Far pair is this phase's truth (set by check_partition, publish_encode).
    // ---- check_anomaly ----
    bool anomalous = false;   // A proven locator with a silent slot: a rescue, not an injection.
    // ---- publish_encode ----
    float view_proj[4][4];
    // ---- adopt_camera ----
    float cam[3];
    bool engcam = false;   // ENGCAM: cam is the engine's own camera.
    // ---- draw: setup_viewport / setup_ssao / judge_guards ----
    UINT n_saved_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    D3D11_VIEWPORT pass_vp = {};   // near-gap: filled by setup_viewport.
    SsaoPass ssao;
    bool guard = false;   // The composite guard: a fresh snapshot and the composite shader.
    bool far_arb = false;   // FAR_ARB: the far route's analytic arbitration is armed.
    // ---- bind_pass_state ----
    UINT stride = sizeof(MeshVertex), offset = 0;
    VbRef bound_vb;   // CLOTH: seeded by bind_pass_state (SKIN_POOL: the pair).
    ID3D11PixelShader* bound_ps = nullptr;
    ID3D11VertexShader* bound_vs = nullptr;
    ID3D11InputLayout* bound_il = nullptr;
    ID3D11ShaderResourceView* bound_sk = nullptr;   // SKIN_POOL: VS t52 (inst_sk_bind).
    float acc_far = 0.0f;   // The pass pair's encode far (enc_far).
    float nz_near = 0.0f;
    bool nz_on = false;
    D3D11_VIEWPORT nz_vp = {};
    float nz_gap_lo = 0.0f;
    // The per-draw viewport swap state (bound_rs pattern): 0 = the pass's, 1 = the near-gap.
    int vp_bound = 0;
    // ---- bind_depth_state ----
    ID3D11RasterizerState* bound_rs = nullptr;
    int bound_bm = -1;
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    // ---- build_frame_cb ----
    ConstantData cbf = {};
    bool rebase_on = false;
    bool frame_ok = false;
    // ---- build_cull ----
    CullSet cull;
    bool inst_on = false;
    bool inst_vb_bound = false;

    // ---- the stages, in inject_composited_meshes' order ----
    void walk_eligible();
    bool stamp_and_triage();     // false: nothing to draw (the harvest or the re-arm done if owed).
    bool ensure_device();        // false: no device or resources.
    void render_shadow_maps();
    void replay_and_resync();
    bool pick_projection();      // false: no usable matrices anywhere this frame.
    void choose_depth_pair();
    bool check_partition();      // false: PART_NEAR - stay armed for the camera partition.
    bool check_anomaly();        // false: an anomalous cycle with the rescue window shut.
    void publish_encode();
    void supervise_view_source();
    void adopt_camera();
    void cull_candidates();
    void order_meshes();
    void draw();                 // The mesh pass and the parts tail, under one StateBackup.
    // ---- draw()'s steps ----
    void setup_viewport();
    void setup_ssao();
    void judge_guards();
    void bind_pass_state();
    void bind_depth_state();
    void build_frame_cb();
    void fill_bands_and_fog_lanes();
    void publish_lod_scale();
    void build_cull();
    void build_visibility();
    void draw_object(size_t oi);   // One iteration of the mesh loop.
    void defer_parts_to_flush();
    void draw_parts_tail();
    void stamp_landing();
    // ---- draw_object's steps ----
    void bind_object_shaders(bool tx_on, bool tx_arb, bool comp_vs);
    void fill_object_guard(ConstantData& cbd, bool tx_arb, bool mesh_nearz);
    // ---- helpers the stages share (the lambdas the function body held) ----
    bool eligible(const RenderObject& o);   // Eligible for this pass (walk_eligible, replay_and_resync).
    // Per-mesh route verdict (the near-z gap; the far route is the pass's, far_arb), factored out so the bucket
    // plan keys every member through the same code the representative is judged by (route is part of the key).
    InjRoute route_of(const RenderObject& o);
    // Why an object is not drawn: 0 it is, 1 outside the view, 2 hidden by the
    // occlusion cull. The verdict is 'is it 0'; OCC_STATS counts the 2s.
    uint8_t vis_code(const RenderObject& o);
    bool vis_calc(const RenderObject& o);
    bool vis(const RenderObject& o);   // The pass's one visibility truth (build_visibility's note).
    void vp_select(int want);
    bool dok(const RenderObject& do_p, int dvp);   // A part the flush can composite (WSEE).
};

inline void InjectPass::walk_eligible() {
    snapshot_now = effect_time_seconds();

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_trigger < 0.0f) {
        g_mask.cold_first_trigger = static_cast<float>(snapshot_now - g_mask.cold_t0);
    }
    meshes.clear();
    occx.clear();
    elig.clear();
    // The eligible set is walked twice - once whole (the "anything eligible" verdicts, the near-clearance minimum -
    // that list taken again after the replay's late syncs when the scene moved, OBJBUF_LATE), once through the grid
    // so the copies below are only of objects whose cell can meet this cycle's frustum. An injection with every
    // eligible mesh off-screen still lands (serial + stats), which the flush's stand-down keys on.
    {
        // ATTACH: placed HERE, immediately before the sync that consumes it, on the thread that is about to draw. Do
        // not move it earlier: measured, the game thread wrote a newer scene mid-frame on 51.4% of injections (752 of
        // 1,464, never more than one generation) - a coin flip between this frame's transform and the next one's, which
        // is jitter. At the consume point there is no window: the injection draws what this step just placed, from the
        // cycle's Draw3D sample of the frame the engine is drawing (gts_for_cycle).
        {
            FinalStepScope fs_final;   // ATTACH_FINAL: the last step that can still change this cycle's draw.
            attach_step(ctx);   // SKEL_DRAW: this step skins the skeletal bindings too.
        }
        uvs_step(ctx);   // KH_USER_VS: a no-op for what the cast fire's step evaluated, but for a source this step just re-posed.
        // SUN_RESTEP: rebuild from this step's poses.
        if (g_sun_restep_cycle == g_topo_cycles) g_sun_maps.sun_map_rendered_frame = false;
        // MESH_GEN_PUB: a pending publish (a generated mesh, an import - EA_FBX_NO_PARK) lands here, before the
        // sync that would hold the objects naming it (SCENE_UNPUB) - so a new or rebuilt mesh draws this cycle,
        // with the pose this step just placed. The render thread, between our draws: the flush's own window (its
        // housekeeping lands the same way), and ensure_resources below tops up the same buffers here on every
        // injection that draws.
        mesh_publish_land(RVExtBridge::get_d3d_device());
        scene_sync();
        objbuf_sync(ctx);   // OBJBUF: the records this pass's buckets index.
    }
    // OBJBUF_LATE: the scene generation this list was taken at, and the test it was taken by - the list is taken
    // again after the replay's late syncs when the scene moved on.
    elig_gen = g_scene.gen;

    {
        for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
            if (!g_scene.alive[i]) continue;
            const RenderObject& o = g_scene.objs[i];

            if (shadow_active(o)) any_caster = true;   // SHADOW_ACTIVE: casterOnly counts.
            if (!eligible(o)) continue;
            ++elig_n;
            elig.push_back(i);
        }

        if (elig_n > 0 && g_mask.cold_t0 >= 0.0 && g_mask.cold_first_stage < 0.0f) {
            g_mask.cold_first_stage = static_cast<float>(snapshot_now - g_mask.cold_t0);
        }
    }
}

inline bool InjectPass::eligible(const RenderObject& o) {
    if (!obj_shown(o) || !is_composite_eligible(o)) return false;   // SHOW_INFRONT.
    bool expired = false;
    const float env = lifetime_envelope(o, snapshot_now, expired);
    if (expired) return false;   // The Draw3D flush owns the erasure.
    // Translucents never take the mid-frame slot. Measure before extending the SV_Depth clamp to PSMain.
    return o.blend_mode == 0 && o.color[3] * env >= 0.999f;
}

// The path is stamped alive; with nothing eligible and no caster the pass ends here: on an anomalous cycle with the
// rescue window shut, the re-arm; with it open, nothing more; otherwise the light pool's harvest
// (DL_TRIGGER_HARVEST) when the cycle's camera is valid.
inline bool InjectPass::stamp_and_triage() {

    // The path is alive even when the eligible set is momentarily empty: stamping here keeps the flush from flapping
    // over.
    g_composite_last_inject_ms.store(steady_now_ms(), std::memory_order_relaxed);
    g_wsee_inj_elig = elig_n > 0;   // WSEE: the capture gate's eligibility (this walk's verdict).
    if (elig_n == 0) {
        if (!any_caster) {
            // DL_TRIGGER_HARVEST: nothing to draw, but the dynamic-light pool is still harvested at this trigger,
            // inside the main pass, as an injection harvests it (dynlights_acquire, below). Do not harvest it at the
            // scene flush (DL_FALLBACK_HARVEST) instead: that span starts after the frame's resolve, its routine
            // windows (the span's first uploads) are other passes' lists than the main pass's after the trigger, and a
            // light the latter alone carries is re-sighted and not rewritten - a flashlight froze at some angles until
            // one of our meshes was in the scene (the main pass's variant was all but never placed). An anomalous cycle
            // is the injection's (anomalous below): re-armed for a later partition while the rescue window is shut,
            // a rescue harvests nothing. Unlike an injection with a mesh near the camera (PART_NEAR), a far
            // partition's trigger is not refused - as with our meshes farther off. The view is the cycle's
            // (dynlights_acquire keeps it for the trigger mirror only).
            if (g_proj_locator_ever && g_ro.slot_near_live <= 0.0f) {
                if (!rescue_window_open()) {
                    g_ro.injected = false;
                    g_ro.inject_attempts = 0;
                }
                return false;
            }
            if (g_ro.cycle_pv_valid) dynlights_acquire(ctx, g_ro.cycle_pv.view);
            return false;
        }
    }
    return true;
}

inline bool InjectPass::ensure_device() {
    dev = RVExtBridge::get_d3d_device();
    if (!dev) {  return false; }
    if (!ensure_ok("injection resources", ensure_resources(dev))) {  return false; }
    return true;
}

inline void InjectPass::render_shadow_maps() {
    dls_select();   // DL_SHADOW twin 1/2 - ahead of the sun, never inside it.
    // SUN_GEN: the syncs above (this step's, dls_select's) took a scene the cast fire's ladder was not built
    // from - rebuild it from what this pass draws (over standing maps only: none, nothing to redraw).
    // SUN_GEN_CAP.
    if (g_sun_maps.sun_map_valid && g_scene.gen_cast != g_sun_gen.map_gen && sun_gen_reopen_ok()) {
        g_sun_maps.sun_map_rendered_frame = false;
    }
    render_sun_depth(ctx);
    dls_frame(ctx);   // DLS_FRAME twin 1/2 - after the sun, never inside it.
}

inline void InjectPass::replay_and_resync() {
    rp_replay(ctx);   // VOL_REPLAY: before the meshes bind t24 / t33.
    // OBJBUF_LATE: the replay re-syncs the scene (RP_PRESYNC and infront_seam_inject, which upload no
    // records; volume_seam_inject), so a stage written since the sync above is in g_scene but not yet in the
    // records the bucket draws index (t39) - one frame at the old pose, or a zero / a dead object's record in a new
    // or reused slot. O(dirty); nothing when nothing moved. The eligible list the near-clearance test walks is taken
    // again with them: an object the late sync added, removed or moved into a freed slot is measured as it is drawn.
    objbuf_sync(ctx);
    if (g_scene.gen != elig_gen) {
        elig.clear();
        for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
            if (g_scene.alive[i] && eligible(g_scene.objs[i])) elig.push_back(i);
        }
        // The count the landing stamp reads (below): the set this pass draws from, as the list now is.
        elig_n = static_cast<uint32_t>(elig.size());
    }
}

inline bool InjectPass::pick_projection() {

    if (g_ro.cycle_pv_valid) {
        pv = g_ro.cycle_pv;

        RVExtBridge::ProjectionViewTransform live = {};

        if (RVExtBridge::get_projection_view_transform(live)) {
            // Render thread throughout: the boundary camera is written on this same thread.
            float lc[3], vc[3];
            cam_of(pv.view, lc);
            cam_of(live.view, vc);
            const float d2 = latch_cam_dist_sq(lc, vc);

            // A cycle whose clear-time fetch succeeded keeps its own sample (pv already); the mid-frame live fetch is
            // only the fallback for a cycle whose boundary fetch failed.
            if (d2 > LATCH_JUMP_M * LATCH_JUMP_M && !g_boundary_pv_valid) {
                memcpy(&pv.view[0][0], &live.view[0][0], sizeof(pv.view));
            }
            const float m22_latch = pv.projection[2][2];
            const float m32_latch = pv.projection[3][2];
            const float m22_live = live.projection[2][2];
            const float m32_live = live.projection[3][2];
            const float n_latch = (fabsf(m22_latch) > 1e-9f) ? (-m32_latch / m22_latch) : 0.0f;
            const float n_live  = (fabsf(m22_live)  > 1e-9f) ? (-m32_live  / m22_live)  : 0.0f;

            // On disagreement the latch is the suspect: on sky-heavy frames it belongs to a partition/aux pass whose
            // huge near plane slices the injected mesh.
            if (fabsf(n_latch - n_live) > 1e-4f ||
                (enc_far(m22_latch, m32_latch) > ENC_FAR_MAX &&
                 enc_far(m22_live, m32_live) <= ENC_FAR_MAX)) {
                pv.projection[2][2] = m22_live;
                pv.projection[3][2] = m32_live;
            }

            // The live fetch's scale terms are frame k+1's and the cycle latch's are frame k's; the seam has taken the
            // live pair, so the footprint and the visible box must rasterise at the same zoom or they diverge radially
            // by the per-frame FOV delta.
            const float lx = g_ro.cycle_pv.projection[0][0];
            const float ly = g_ro.cycle_pv.projection[1][1];
            const float sx = live.projection[0][0];
            const float sy = live.projection[1][1];
            bool  ok = sx == sx && sy == sy &&
                       lx > 1.0e-6f && ly > 1.0e-6f &&
                       sx > 1.0e-6f && sy > 1.0e-6f;

            if (ok) {
                const float rx = sx / lx;
                const float ry = sy / ly;
                const float hi = rx > ry ? rx : ry;
                // Aspect preserved, to 1 pct of the larger ratio.
                if (fabsf(rx - ry) > 0.01f * hi) ok = false;
                // And inside the foreign-window line, both ways.
                if (hi > 4.0f || hi < 0.25f) ok = false;
            }

            if (ok) {
                pv.projection[0][0] = sx;
                pv.projection[1][1] = sy;
            }
        }
    } else if (RVExtBridge::get_projection_view_transform(pv)) {
        g_last_pv = pv;
        g_last_pv_ms = steady_now_ms();
    } else if (g_last_pv_ms != 0 && steady_now_ms() - g_last_pv_ms < 250) {
        pv = g_last_pv;
    } else {
        return false;   // No usable matrices anywhere this frame.
    }

    return true;
}

inline void InjectPass::choose_depth_pair() {
    memcpy(latch_view, &pv.view[0][0], sizeof(latch_view));
    // Do not adopt the seam's rotation rows here: at altitude the seam pass runs before the cycle's clear and its view
    // is the previous frame's, and adopting it hands every mesh a one-frame rotation error (measured 4.8 deg at mouse
    // speed, distance-scaled). The pass draws with the cycle's own view; the seam's mask may trail a frame at its edge.
    adopt_frame_view(pv);

    // The meshes then rasterize with the true dynamic near plane of the frame being rendered - the one scalar the
    // bridge can never pin down - and the guard reconstructs scene distance with the same truth.
    measured = g_ro.engine_proj_valid;

    // State snapshot: injection decisions read the telltale once. The render thread's upload and pre-draw hooks write
    // this state; the snapshot keeps every decision below self-consistent.
    snap_slot_near = g_ro.slot_near_live;
    snap_slot_m22 = g_ro.slot_m22;
    snap_slot_m32 = g_ro.slot_m32;
    snap_world_valid = g_ro.world_pair_valid;
    snap_world_m22 = g_ro.world_m22;
    snap_world_m32 = g_ro.world_m32;

    // PART_NEAR: the camera's own near - the cycle's camera (the clear's latch, its pair checked against the
    // live one above), before any pair below replaces it: the partition our meshes belong in.
    cam_near = fabsf(pv.projection[2][2]) > 1e-9f
             ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;

    if (measured) {
        pv.projection[2][2] = g_ro.engine_m22;
        pv.projection[3][2] = g_ro.engine_m32;
    }

    const bool keep_fresh = g_slot_keep.near_m > 0.0f && g_slot_keep.ms != 0 &&
                            steady_now_ms() - g_slot_keep.ms < 250;

    const bool slot_band = snap_slot_near >= INJ_SLOT_NEAR_MIN;
    const bool slot_sane_far = enc_far(snap_slot_m22, snap_slot_m32) <= ENC_FAR_MAX;

    if (snap_world_valid) {
        // Parity by construction: the pair the world opaques we depth-test against last rendered under.
        pv.projection[2][2] = snap_world_m22;
        pv.projection[3][2] = snap_world_m32;
    } else if (snap_slot_near > 0.0f && slot_band && slot_sane_far) {
        pv.projection[2][2] = snap_slot_m22;
        pv.projection[3][2] = snap_slot_m32;
    } else if (keep_fresh) {
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
}

inline bool InjectPass::check_partition() {
    // A following camera partition still gets its own injection via the partition-change re-arm.
    // PART_NEAR: judged on the near this injection would draw with - the pair just taken - against the camera's. Do
    // not judge it on the slot: the world-phase latch keeps the last opaque draw's pair, which can be a far partition's
    // (a view with no object near the camera: 10 m, measured) while the slot has already moved on to the camera's;
    // judged on the slot, the rule passed and the injection drew with the far near, a near mesh wholly in the near-gap
    // in front of everything.
    const float eff_near = fabsf(pv.projection[2][2]) > 1e-9f
                         ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
    if (snap_slot_near > 0.0f && cam_near > 1e-4f &&
        enc_far(pv.projection[2][2], pv.projection[3][2]) <= ENC_FAR_MAX &&   // Celestial-class: never.
        eff_near > 1.5f * cam_near) {
        float r_cam[3];
        cam_of(pv.view, r_cam);
        float min_clear = 1e9f;

        for (uint32_t slot : elig) {   // SCENE: every eligible mesh, pre-cull independent.
            // NEARZ_BOUNDS: the nearest the box around everything the mesh may draw comes to the camera
            // (bounds_size_of: a cloth's reach, a decimated level, a vertex stage's reach, the residual; a
            // scaled rotation) - the script's size alone passed such a mesh as clear, drawn in the near-gap.
            const float d = sqrtf(bounds_dist_sq(g_scene.objs[slot], r_cam));

            if (d < min_clear) min_clear = d;
        }

        if (min_clear < 1.5f * eff_near) {
            // A repaint-invoked call landing here gets its one-shot refunded,
            // so a genuine repaint later the same cycle still fires.
            g_ro.injected = false;   // Stay armed for the camera partition.
            g_ro.inject_attempts = 0;
            return false;
        }

        far_phase = true;   // Arms the analytic arbitration below.
    }
    return true;
}

inline bool InjectPass::check_anomaly() {
    // Anomalous cycle: a proven locator with a silent slot means the engine is
    // rendering a pass that never published a camera-class projection.
    anomalous = g_proj_locator_ever && snap_slot_near <= 0.0f;

    if (anomalous) g_ro.anomaly_seen = true;   // Flush carries this frame.

    if (anomalous) {
        // Failure direction: an even-later partition erases it again - never a new artifact on healthy frames.
        if (!rescue_window_open()) {
            g_ro.injected = false;
            g_ro.inject_attempts = 0;
            return false;
        }
        // Rescue proceeds: anomalous stays true, so the view-source supervisor quarantine below holds and the
        // end-of-draw bookkeeping counts a rescue, not an injection.
    }
    return true;
}

// The encode pair and view the flush takes (LATE_WORLD_PAIR's publication rung), the far-route hysteresis, the
// light harvest of an accepted injection, and the pass's view_proj.
inline void InjectPass::publish_encode() {
    const float inject_near = fabsf(pv.projection[2][2]) > 1e-9f
                            ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;

    // Below the far-phase and anomalous skip returns, so an aborted attempt never re-publishes over a real landing's
    // pair; re-published by any later proceeding attempt this frame, so the flush carries the last painter's encode.
    g_inj_pub.enc_m22 = pv.projection[2][2];
    g_inj_pub.enc_m32 = pv.projection[3][2];
    g_inj_pub.enc_near = inject_near;
    g_inj_pub.enc_ms = steady_now_ms();
    memcpy(g_inj_pub.view, &pv.view[0][0], sizeof(g_inj_pub.view));
    g_inj_pub.view_ms = g_inj_pub.enc_ms;


    if (inject_near >= FAR_ARB_NEAR_MIN ||
        (g_inj_arb_prev_armed && inject_near >= FAR_ARB_NEAR_HOLD)) {
        far_phase = true;
    }

    // Accepted injections only. Rescue cycles are quarantined: a foreign pass's
    // slot bindings are exactly the poison the CPU validation keeps out.
    if (!anomalous) dynlights_acquire(ctx, pv.view);

    mul_4x4(pv.view, pv.projection, view_proj);
}

inline void InjectPass::supervise_view_source() {
    // Supervise the view source: lock the upload location whose contents equal this frame's pre-adoption latch
    // (latch_view - not the adopted view the meshes draw with: a wrong lock must not self-certify through its own
    // publishes). Never on anomalous cycles - a supervisor taught by them mis-locks (world-sliding shadows).
    if (!anomalous) {
        const float* truth = latch_view;
        const uint32_t vcn = g_ls.vc_n < 16 ? g_ls.vc_n : 16;
        // Age-bounded so a matrix harvested before the camera moved cannot sit here winning.
        const bool ok = ring_best_fresh();   // RING_BEST_AGE.

        if (!g_ls.view_src_valid && (vcn > 0 || ok)) {
            // Argmin, not first-passing: pick the closest candidate (by its rotation's error) and report its error.
            int best_i = -1, best_k = 0;
            float best_rot = 1e9f;   // DDX_BEST_TR: the rotation's error alone.

            for (uint32_t i = 0; i < vcn; ++i) {
                const float* m = g_ls.vc_ring[i].m;

                for (int orient = 1; orient <= 2; ++orient) {
                    float e_rot = 0.0f;

                    for (int r = 0; r < 3; ++r) {
                        for (int c = 0; c < 3; ++c) {
                            const float mv = orient == 1 ? m[r * 4 + c] : m[c * 4 + r];
                            e_rot += fabsf(mv - truth[r * 4 + c]);
                        }
                    }

                    if (e_rot < best_rot) {
                        best_rot = e_rot;
                        best_i = static_cast<int>(i);
                        best_k = orient;
                    }
                }
            }

            // Copied into ring slot 0 so every downstream consumer (convention classification, tmag gate, cold confirm,
            // adoption) runs on the identical code path.
            if (ok) {
                float e = 0.0f;

                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) {
                        const float v = (g_ring_best.orient == 1)
                            ? g_ring_best.m[r * 4 + c] : g_ring_best.m[c * 4 + r];
                        e += fabsf(v - truth[r * 4 + c]);
                    }
                }

                if (best_i < 0 || e < best_rot) {
                    g_ls.vc_ring[0].res =
                        static_cast<ID3D11Resource*>(const_cast<void*>(g_ring_best.res));
                    g_ls.vc_ring[0].off = g_ring_best.off;
                    memcpy(g_ls.vc_ring[0].m, g_ring_best.m, 64);
                    best_i = 0;
                    best_k = static_cast<int>(g_ring_best.orient);
                    best_rot = e;
                }
            }

            // Lock bar: the cold lock has no reference and accepts 0.05; re-locks must beat 0.02.
            const float lock_bar = g_ls.view_locks == 0 ? 0.05f : 0.02f;

            const bool exact_lock = best_rot < 1e-3f;

            if (best_i >= 0 && best_rot < lock_bar && !exact_lock &&
                g_ls.view_wait_since <= 0.0f) {
                g_ls.view_wait_since = effect_time_seconds();
            }

            const bool wait_expired = g_ls.view_wait_since > 0.0f &&
                effect_time_seconds() - g_ls.view_wait_since > 2.0f;

            if (best_i >= 0 && best_rot < lock_bar && (exact_lock || wait_expired)) {
                g_ls.view_wait_since = 0.0f;
                // Convention classification.
                const float* m = g_ls.vc_ring[best_i].m;
                float tmag = 0.0f;

                for (int c = 0; c < 3; ++c) {
                    const float tv = best_k == 1 ? m[12 + c] : m[c * 4 + 3];
                    tmag += fabsf(tv);
                }

                const bool relative = tmag < 10.0f;

                // Re-locks keep the tighter bar and skip the cold double-confirmation (a lock dropped mid-flight must
                // not wait an extra frame); the exact-or-2 s wait above still applies to them.
                bool cold_confirmed = true;

                if (g_ls.view_locks == 0) {
                    cold_confirmed = g_ls.vc_ring[best_i].res == g_coldlock_cand_res &&
                                     g_ls.vc_ring[best_i].off == g_coldlock_cand_off;
                    g_coldlock_cand_res = g_ls.vc_ring[best_i].res;
                    g_coldlock_cand_off = g_ls.vc_ring[best_i].off;
                }

                if (relative && cold_confirmed) {
                    g_ls.view_src_res = g_ls.vc_ring[best_i].res;
                    g_ls.view_src_off = g_ls.vc_ring[best_i].off;
                    g_ls.view_src_orient = best_k;
                    g_ls.view_src_relative = true;
                    g_ls.view_src_valid = true;
                    view_lock_acquired();
                    g_ring_best.valid = false;

                    g_ls.view_src_miss = 0;
                    g_ls.pub_ok_since_lock = false;
                    g_ls.view_locks++;
                    g_lock_churn_ms_v = steady_now_ms();   // lock-settle: fresh lock, hold the cast a beat.
                    g_ls.wipe_deferred = true;
                }
            }
        }

        g_ls.vc_n = 0;   // Fresh ring per injection: stale entries can't win.

        if (g_ls.view_src_valid) {
            // Health: rotation agreement is the signal (the hybrid's translation is bridge-derived by construction).
            bool healthy = false;

            if (g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f) {
                float e_rot = 0.0f;

                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) {
                        e_rot += fabsf(g_ls.frame_view[r * 4 + c] - truth[r * 4 + c]);
                    }
                }

                // The publish-side family filter still guards at 0.35.
                healthy = e_rot < 0.20f;
            }

            if (healthy) {
                g_ls.view_src_miss = 0;
            } else if (++g_ls.view_src_miss > 90) {
                g_view_drop.path = 2;
                g_view_drop.relock_wide_cycle = 0;
                g_ring_best.valid = false;
                g_ls.view_src_valid = false;
                g_ls.vc_n = 0;
            }
        }
    }
}

inline void InjectPass::adopt_camera() {

    // The camera through the honest inverse of the view the pass draws with, never the transpose.
    cam_of(pv.view, cam);
    engcam = engcam_consume(pv.view, cam);
    g_ls.cam[0] = cam[0];
    g_ls.cam[1] = cam[1];
    g_ls.cam[2] = cam[2];
    g_ls_cam_cycle = g_topo_cycles;   // FOG_PROBE.
}

inline void InjectPass::cull_candidates() {
    static std::vector<uint32_t> cand;   // SCENE: grid pre-cull candidates.
    // The grid pre-cull, against the view this pass will draw with (pv.view is final here) and the same cam the precise
    // test below is built from. The eligibility predicate is the staging walk's, against the same snapshot_now. An
    // invalid cull set returns every cell.
    {
        CullSet pre;
        float pre_vp[4][4];
        mul_4x4(pv.view, pv.projection, pre_vp);
        cull_build(pre_vp, cam, pre);
        scene_grid_query(pre, cand);

        for (uint32_t slot : cand) {
            if (!g_scene.alive[slot]) continue;
            const RenderObject& o = g_scene.objs[slot];
            bool expired = false;
            // OCC_FORCE: a declared occluder this pass will not draw still
            // hides (the cull's second list) while it lives. SHOW_INFRONT: eligible's test.
            if (!obj_shown(o) || !is_composite_eligible(o)) {
                if (o.occluder && !o.fullscreen) {
                    (void)lifetime_envelope(o, snapshot_now, expired);
                    if (!expired) occx.push_back(o);
                }
                continue;
            }
            const float env = lifetime_envelope(o, snapshot_now, expired);
            if (expired) continue;
            if (!(o.blend_mode == 0 && o.color[3] * env >= 0.999f)) {
                if (o.occluder) occx.push_back(o);
                continue;
            }
            meshes.push_back(o);
            meshes.back().color[3] *= env;
        }
    }
}

inline void InjectPass::order_meshes() {
    // Our meshes in mesh_order's order by the camera's distance to each
    // object's world bounds: opaque solids front to back, the rest back to front.
    {   // MESH_ORDER: key sort, same order, one distance per object.
        mesh_order(meshes, cam, g_dist_memo_inj);   // The memo begins here, seeded; the route verdicts,
                                                       // the emit, the pick and the parts read it.
    }
    // REPLAY_SCISSOR: the merged textures are this pass's where the replay drew for these meshes (the replay's own
    // world seam re-syncs the scene).
    if (g_rp.mir_pend) {   // MIR_REPLAY: this pass reads the mirror if a mesh of it does, under its own near.
        for (const RenderObject& o : meshes) {
            if (rp_mir_reads(o, pv.projection, cam)) { rp_mir_finish(ctx); break; }
        }
    }
}

// The mesh pass and the parts tail. Its scope objects (the replay scope, the state backup, the bind filter, slot 0) are
// this function's locals in the order the single function declared them, so they unwind as they did; the landing's
// bookkeeping runs after the backup's restore, inside them, as before.
inline void InjectPass::draw() {
    bool rp_vol = false, rp_mir = false;
    rp_covered(meshes, rp_vol, rp_mir);
    RpPassScope rp_scope(rp_vol, rp_mir);

    StateBackup backup;
    backup.capture(ctx);
    BindScope bc(ctx);   // BIND_FILTER.
    OmSlot0 s0;   // MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    s0.open(ctx);
    objbuf_bind(ctx);   // OBJBUF: VS t39 for every bucket draw (restored with the backup).
    setup_viewport();     // The pass viewport in the world's depth range.
    setup_ssao();         // SSAO: the pass's resources and lanes.
    judge_guards();       // The composite guard and the far route (FAR_ARB_SNAP).
    bind_pass_state();    // Shaders, the near-gap route, the constant buffers and every texture the pass reads.
    bind_depth_state();   // LESS_EQUAL + write (SSAO_MARK), the rasterizer.
    build_frame_cb();     // The frame template (and the PIP's).
    publish_lod_scale();  // LOD_ONE_SCALE.
    build_cull();         // The frustum, the route verdict, the occlusion cull.
    build_visibility();   // Every object's verdict (INJ_FORK) and the bucket plan.
    parts.clear();
    for (size_t oi = 0; oi < meshes.size(); ++oi) draw_object(oi);

    // SSAO: the term and its multiply, at our pixels, before any translucent part blends over them (its own
    // StateBackup / OmSave hand the loop's state back, so the tail's trackers stay true).
    ssao_post(ctx, ssao);

    defer_parts_to_flush();   // WSEE: the capture; the parts the flush's tail draws.
    draw_parts_tail();        // BLEND_PART_INJ: every staged part, far to near.

    backup.restore(ctx);   // The engine's viewports too (saved_vp is the same capture, taken after it).
    stamp_landing();
}

inline void InjectPass::setup_viewport() {
    // The meshes' stored depth must be encoded through the same range the surrounding scene geometry used, or every
    // comparison skews - worst for near fragments, where a min-depth offset is a huge world-space error.
    ctx->RSGetViewports(&n_saved_vp, saved_vp);
    {
        D3D11_VIEWPORT vp = {};

        if (n_saved_vp >= 1) {
            vp = saved_vp[0];
        } else {
            vp.Width = static_cast<FLOAT>(g_main_depth_w);
            vp.Height = static_cast<FLOAT>(g_main_depth_h);
        }

        vp.MinDepth = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
        vp.MaxDepth = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
        ctx->RSSetViewports(1, &vp);
        pass_vp = vp;   // near-gap: the world-range viewport.
    }   // pass-scope for the per-draw gap swap.
}

inline void InjectPass::setup_ssao() {
    // SSAO: the pass's resources and lanes, before the first of our draws; the post pass runs after the last
    // depth-writing one (ahead of the parts tail, which writes no depth and must not take the solids' term). pv and
    // pass_vp are what the solids below draw with.
    if (ao_strength() > 0.0f || nzm_wanted()) {   // NEARZ_MARK: the marker needs the mark too.
        ssao_rect(pv, ssao.rect);
        ssao_pre(dev, ctx, ssao, pv.projection, pass_vp.MinDepth, pass_vp.MaxDepth);
    }
}

inline void InjectPass::judge_guards() {
    const uint64_t inj_now = steady_now_ms();
    const bool snap_ok = g_res.comp_depth_srv && g_res.comp_depth_samples == 1 &&
                         g_snap.serial != 0 && g_snap.ms != 0 &&
                         (inj_now - g_snap.ms) < SNAP_MAX_AGE_MS;
    guard = snap_ok && ensure_ok("composite shader", ensure_composite_shader(dev));

    // Fail-safe direction: heightfield or arb shader down => stock LESS_EQUAL path (the known bite, never a new
    // artifact).
    // FAR_ARB_SNAP: and no usable snapshot (snap_ok) - the arb PS reads t0, which is the engine's texture when
    // no snapshot binds there. The flush's twin requires arb_snap_ok.
    far_arb = far_phase && g_thm.valid && snap_ok &&
              ensure_ok("composite shader", ensure_composite_shader(dev)) &&
              g_res.ps_composite_arb != nullptr;

    g_inj_arb_prev_armed = far_arb;
}

inline void InjectPass::bind_pass_state() {
    bound_vb.vb = g_res.mesh_vb.empty() ? nullptr : g_res.mesh_vb[0];
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &g_res.mesh_vb[0], &stride, &offset);
    ctx->IASetIndexBuffer(g_res.mesh_ib[0], DXGI_FORMAT_R32_UINT, 0);
    ctx->VSSetShader((guard || far_arb) ? g_res.vs_composite : g_res.vs, nullptr, 0);
    ctx->PSSetShader(far_arb ? g_res.ps_composite_arb
                     : guard     ? g_res.ps_composite
                                 : g_res.ps, nullptr, 0);
    // SV_Depth replaces the rasterized depth entirely, so the shared per-frame viewProj stays untouched: the override
    // is two per-object scalars plus the PS pick.
    bound_ps = far_arb ? g_res.ps_composite_arb
             : guard      ? g_res.ps_composite
                              : g_res.ps;
    bound_vs = (guard || far_arb) ? g_res.vs_composite : g_res.vs;
    bound_il = g_res.input_layout;
    // s0 for the textured per-submesh path; StateBackup's sampler slot restores.
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    acc_far = enc_far(pv.projection[2][2], pv.projection[3][2]);
    // Fail direction: disarmed = the stock path. Arms only with the arb shader compiled, a sane near, and a real
    // sub-floor gap to open (MinDepth 0 has none).
    const float nz_A = pv.projection[2][2];
    const float nz_B = pv.projection[3][2];
    nz_near = (nz_A > 1.0e-6f) ? (-nz_B / nz_A) : 0.0f;
    nz_on = g_res.ps_composite_arb != nullptr && nz_near > CAM_NEAR_MIN && nz_near < 50.0f &&
                pass_vp.MinDepth > 1.0e-5f && pass_vp.MinDepth < pass_vp.MaxDepth &&
                pass_vp.MaxDepth <= 1.0f;
    nz_vp = pass_vp;
    nz_gap_lo = pass_vp.MinDepth * NEARZ_GAP_FRAC;
    if (nz_on) nz_vp.MinDepth = nz_gap_lo;
    ssao.nz_near = nz_on ? nz_near : 0.0f;   // SSAO_NEARZ: the gap this pass's routed draws fill.
    ssao.nz_gap = nz_gap_lo;
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    // Binding probe: sky rows (fog) and the view harvest (lock survival). Runs
    // when either wants; idles when both are satisfied.
    if ((g_fog_valid && g_fog[0] > 1e-4f) || !g_ls.view_src_valid) skybind_step(ctx);

    // Twin: the flush path carries the identical narrowing.
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    // INJ_T0: the depth snapshot or null at t0, every pass (inside StateBackup's saved range). PSComposite Loads t0
    // with no lane gating it, and the near-gap route draws the arb twin whether or not a snapshot exists
    // (FAR_ARB_SNAP gates the far route alone) - left unbound, the first frame of a session or device and every
    // frame after a failed snapshot create would read whatever view the engine had at t0. Such a draw has its guard
    // stood down (fx1[0] = 1e9, fx1.zw = 0, snapCam.w = 0), so the value is discarded; a null view reads 0, the guard's
    // "no scene". Twin: the flush binds the snapshot or null per solid (FLUSH_NOSNAP).
    {
        ID3D11ShaderResourceView* const t0 = g_res.comp_depth_srv;
        ctx->PSSetShaderResources(0, 1, &t0);
    }
    // Bound together whenever any consumer is armed: the ratio needs both textures, and a half-bound pair is the
    // silent-wrong-answer shape this feature avoids.
    if (svs_sten_on()) {
        if (g_svs.pre_srv) {
            ctx->PSSetShaderResources(21, 1, &g_svs.pre_srv);
        }

        if (g_svs.post_srv) {
            ctx->PSSetShaderResources(22, 1, &g_svs.post_srv);
        }
    }
    // The volume copy's stencil plane (t24), on its own arm; inside
    // StateBackup's saved range. Twin: the flush carries the identical block.
    if (svs_vol_ready()) {
        // VOL_REPLAY.
        ID3D11ShaderResourceView* const sten = rp_use_merged() ? g_rp.msten_srv : g_svs.vol_sten_srv;
        ctx->PSSetShaderResources(24, 1, &sten);
        // VOL_WITNESS: its depth plane (t23; inside StateBackup's range).
        if (g_svs.vol_depth_srv) ctx->PSSetShaderResources(23, 1, &g_svs.vol_depth_srv);
        // VOL_FOOT: its footprint mask (t33; inside StateBackup's range).
        ID3D11ShaderResourceView* const foot = rp_use_merged() ? g_rp.mfoot_srv : svs_foot_bound();
        if (foot) ctx->PSSetShaderResources(33, 1, &foot);
    }
    // The mirror stencil at t28 (inside StateBackup's saved range). mirMeta.x = 0 short-circuits the shader, so an
    // absent mask never reads a stale bind. Twin: the flush carries the identical block.
    if (g_vmir.srv && g_vmir.mask_time >= 0.0f) {
        ID3D11ShaderResourceView* const mir = rp_mir_srv();   // MIR_REPLAY.
        ctx->PSSetShaderResources(28, 1, &mir);
    }
    // Shadow atlas for the per-pixel map compare (shadowMeta.x = 0 short-circuits the shader when the table is empty).
    // t1 is inside StateBackup's saved range. INJ_ATLAS: the atlas or null, every pass - the table
    // (fill_lighting_frame_cb) is armed whether or not this bind lands, and left unbound the receive would read
    // whatever the engine had at t1 (a frame without a fresh snapshot, or the composite shader down). Twin: the flush
    // and the in-front slice bind the same way.
    {
        ID3D11ShaderResourceView* const atlas = shadow_live_ensure_srv() ? g_ls.atlas_srv : nullptr;
        ctx->PSSetShaderResources(1, 1, &atlas);
    }
    for (UINT bslot = 0; bslot < 8; ++bslot) {
        if (g_ls.band[bslot].valid && g_ls.band[bslot].srv) {
            const UINT treg = bslot < 6 ? 4 + bslot : 12 + (bslot - 6);
            ctx->PSSetShaderResources(treg, 1, &g_ls.band[bslot].srv);
        }
    }
    // Private sun-depth map for the self term (t11, inside the saved range).
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
    // Analytic terrain heightfield (t10, saved range; see thmParams).
    if (g_thm.valid && g_res.thm_srv) ctx->PSSetShaderResources(10, 1, &g_res.thm_srv);
    // The light depth array at t36, bound by the pass that reads it (twin: the flush). Bound whenever it exists -
    // per-light validity lives in dls_meta[i].w, so an unbound slot would be a second way to say 'no shadow' and the
    // two could disagree.
    if (g_res.dls_srv) {
        ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
    }
}

inline void InjectPass::vp_select(int want) {
    if (want == vp_bound) return;
    ctx->RSSetViewports(1, want == 1 ? &nz_vp : &pass_vp);
    vp_bound = want;
}

inline void InjectPass::bind_depth_state() {
    // The injection fills blendCtl.x = 0 unconditionally; PSComposite's blendCtl.x branch stays cold. The engine's
    // translucents must be able to depth-reject against these meshes.

    // LESS_EQUAL + write for both arms: the honest raw-space test is the object authority; the arb variant beats
    // terrain LOD via its SV_Depth clamp.
    // SSAO_MARK: our depth writes mark stencil 1 only when this pass's post (ssao_post, its eraser) runs after
    // them; a mark no eraser follows would stand for the next drawer's pass.
    ID3D11DepthStencilState* const dss_w = ssao.on ? g_res.dss_test_write_mark : g_res.dss_test_write;
    ctx->OMSetDepthStencilState(dss_w, ssao.on ? 1u : 0u);
    // WSEE: our solids write the main depth from here (late: past the capture).
    wsee_taint(g_topo_cycles);
    ctx->RSSetState(g_res.rasterizer);
    bound_rs = g_res.rasterizer;
}

inline void InjectPass::build_frame_cb() {
    // Per-pass frame build (injection edition). Each object starts from this template; only the object slice uploads
    // per draw. Twin of the flush build (cbf); this copy also interleaves the g_band_tab_* and sun-range
    // publications.
    memcpy(cbf.view_proj, view_proj, sizeof(cbf.view_proj));

    // cam is deliberately not always pv.view's own camera (ENGCAM: engcam_consume may hand back the engine's
    // own camera - the frame's latch, the seam's take where volume_seam_inject consumed first), and the fold
    // cancels against fill_center_rel for any origin.
    rebase_on = engcam
              ? rebase_vp_engcam(cbf, pv.view, pv.projection)
              : rebase_vp(cbf, cam);   // DDX_REBASE.
    fill_lighting_frame_cb(cbf);
    // Snapshot reprojection frame fields (arb PS consumer; every other pass ignores them).
    if (g_snap.vp_valid && far_arb) {
        memcpy(cbf.snap_vp, g_snap.vp, sizeof(cbf.snap_vp));
        cbf.snap_meta[0] = g_snap.pair[0];
        cbf.snap_meta[1] = g_snap.pair[1];
        cbf.snap_meta[2] = g_snap.vp_lo;
        cbf.snap_meta[3] = g_snap.vp_hi;
        cbf.snap_cam[0] = g_snap.cam[0];
        cbf.snap_cam[1] = g_snap.cam[1];
        cbf.snap_cam[2] = g_snap.cam[2];
        cbf.snap_cam[3] = 1.0f;
    } else {
        cbf.snap_cam[3] = 0.0f;   // The zeroed template already says so; explicit.
    }
    fill_bands_and_fog_lanes();   // The engine's shadow bands, the receive arm, the fog and haze lanes.
    // Heightfield lanes, both arms - the terrain lane never depends on the depth lane's arming state.
    fill_occ(cbf);
    // Pass lanes: the rebase camera the bucket VS subtracts (armed exactly when
    // the per-object path arms centerRel) and the object view-distance cut.
    cbf.pass_cam[0] = cam[0]; cbf.pass_cam[1] = cam[1]; cbf.pass_cam[2] = cam[2];
    cbf.pass_cam[3] = rebase_on ? 1.0f : 0.0f;
    cbf.pass_obj[0] = g_obj_vis.load(std::memory_order_relaxed);
    fill_user_cam_cb(cbf, cam, pv.view);   // USER_LANES.
    frame_ok = upload_frame_cb(ctx, g_res.composite_frame_cb, cbf);
    if (frame_ok) {   // PIP: the next frame's PIP passes draw under this template.
        g_pip.tpl = cbf;
        g_pip.tpl_valid = true;
        g_pip.tpl_cycle = g_topo_cycles;
    }
}

inline void InjectPass::fill_bands_and_fog_lanes() {
    bool band_any = false;
    // Finest-first: order slots by near edge ascending, so the shader's first containing band holds the finest
    // content in the engine's overlap zones. bandBorder.w encodes valid + which physical texture slot to sample (1
    // + index).
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
        // BAND_TAB_FILL: the table is this fill's, not the session's. Do not carry it across fills: a position
        // written once and then skipped keeps its old far, and the max below (src 2, which shuts the video option
        // out) can then only grow - a lowered shadow distance never reaches our range while a dedupe twin or a
        // cascade the engine stopped sealing holds a slot. The far is the engine's cascade border, so it is taken
        // from every sane valid band, committed to the lanes or not: our own receive gating (warm-up, health, a
        // pending view, the twin dedupe) must not flap the range. A fill with no sane band leaves the previous
        // value and src standing (the max stays -1 and fails the guard).
        g_band_tab_far[b] = -1.0f;
        if (!bs.valid || !bs.srv) { continue; }
        if (bs.border[1] > bs.border[0] && bs.border[0] >= 0.0f && bs.border[1] <= 4000.0f) {
            g_band_tab_far[b] = bs.border[1];   // The sanity test below, in the positive.
        }

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

        // Sanity first (a band failing it is dropped until its next resolve); then the twin dedupe: near inside the
        // previous band's range and far not meaningfully beyond it = a partition duplicate of the same range; two
        // twins alternate seal content and the shader's winner flip-flops.
        if (!(bs.border[1] > bs.border[0]) || bs.border[0] < 0.0f ||
            bs.border[1] > 4000.0f) {
            g_ls.band[order[b]].valid = false;   // The next resolve reseals sanely.
            continue;
        }

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

    {   // The far border of the engine's cascades (BAND_TAB_FILL): the max over this fill's table.
        float mf = -1.0f;
        for (int i = 0; i < 8; ++i) {
            if (g_band_tab_far[i] > mf) mf = g_band_tab_far[i];
        }
        if (mf > 8.0f && mf < 1.0e5f && mf == mf) {
            const float prev = g_sun_range.load(std::memory_order_relaxed);
            if (!(fabsf(mf - prev) <= 0.005f * prev)) {
                g_sun_range.store(mf, std::memory_order_relaxed);
            }
            g_sun_setup.sun_range_src.store(2, std::memory_order_relaxed);
        }
    }

    if (band_any && g_sun_valid &&
        (g_atlas_frame_cur || g_atlas_frame_prev)) {   // Published sun + live atlas.
        cbf.mask_meta[0] = 1.0f;   // Stream liveness: the receive.
    }
    cbf.mask_meta[3] = svs_unit_on() ? 1.0f : 0.0f;
    // The volume transport arm and the copy's dimensions, for the visible mesh.
    fill_sten_reproj(cbf, pv.projection);

    // The fog-lane fill below: TWIN of the flush's (fx_frame_template)
    // and of dlsw_fog_lanes (DLSW_FOG_FILL).
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

        // Camera altitude (engine Y-up), unclamped - see fog_cam_y.
        cbf.fog_color[3] = fog_cam_y;
    }
}

inline void InjectPass::publish_lod_scale() {
    // Publish the pass's pixels-per-tangent here too: on a frame the injection draws and the flush never reaches its
    // own publish, g_lod.proj_px would stay 0 and every object would draw at level 0. LOD_ONE_SCALE: the flush's
    // definition - this pass's own viewport height (pass_vp, what its meshes draw with), not the adopted depth
    // texture's; an unusable pair keeps the last value.
    if (lod_publish(pv.projection[1][1], pass_vp.Height)) {   // LOD_PARK_KEEP: the flush's park test.
        g_lod.inj_h = pass_vp.Height;
        g_lod.inj_cycle = g_topo_cycles;
    }
}

inline void InjectPass::build_cull() {
    cull_build(cbf.view_proj, cam, cull);
    cull.back_on = true;   // CULL_BACK: the colour pass never draws what is behind the camera.
    // Buckets, injection edition: the pass verdict, the plan + emission (natural order) and the slot-1 bind latch. This
    // pass routes buckets through the composite instanced twins, which compile non-fatally; absent either twin the
    // whole pass draws per object (a missing twin would strand a bucket's members). The flush's gate stays
    // inst_twins_on(): the static twins are all-or-nothing.
    inst_on = inst_twins_on() &&
              g_res.vs_comp_inst != nullptr && g_res.vs_comp_inst_tex != nullptr;
    // Finished background loads land here, before any base is fixed or lane written (a placement bumps the page
    // generation and the ensure below re-fixes every base).
    tex_async_pump(ctx, dev);
    // Occlusion cull, injection edition: this pass depth-writes every solid, so a hidden object provably contributed
    // zero pixels (from THM_MIN_DIST_M, none the heightfield shows: VOC_BURIAL). SV_Depth routes are excluded on
    // both sides; on far-arb frames the terrain clamp may pull any fragment up to the guard nearer, so that is a
    // margin. The cap is metres of VIEW DEPTH while the grid compares EUCLIDEAN distances, so the margin carries the
    // frustum-corner secant sqrt(1 + tan^2(fovx/2) + tan^2(fovy/2)) from the pass projection; unreadable scales read as
    // 'never cull'.
    float occ_margin = 0.0f;
    if (far_arb) {
        const float opx = fabsf(pv.projection[0][0]);
        const float opy = fabsf(pv.projection[1][1]);
        if (opx > 1.0e-6f && opy > 1.0e-6f) {
            const float otx = 1.0f / opx;
            const float oty = 1.0f / opy;
            occ_margin = (FAR_ARB_GUARD_BASE + 1.0f) *
                         sqrtf(1.0f + otx * otx + oty * oty);
        } else {
            occ_margin = 1.0e18f;
        }
    }
    vis_occ_build(occ, meshes, occx, cbf.view_proj, cull, cam,
                  occ_margin,
                  acc_far, g_obj_vis.load(std::memory_order_relaxed),
                  // VOC_FLOOR_FLOAT: the floor is a height (vis_occ_build reads a float); a bool return would
                  // make REFUSE, NONE and every floor 1.0f.
                  [&](const RenderObject& o) -> float {
                      const InjRoute r = route_of(o);
                      return r.nearz ? VOC_FLOOR_REFUSE : voc_floor(o);
                  });
}

inline InjRoute InjectPass::route_of(const RenderObject& o) {
    InjRoute r;
    // For z >= near the encode equals the world mapping to a few D24 steps (float32 re-map; ARB_VP_FLOOR),
    // so routing on/off pops nothing but a fragment coplanar with the world's. NEARZ_BOUNDS: the drawn box
    // decides - what a fragment nearer than the near plane can come from (the script's box can hold less).
    r.nearz = nz_on &&
        g_dist_memo_inj.get_drawn(o, cam) < (nz_near + 2.0f) * (nz_near + 2.0f);
    return r;
}

inline void InjectPass::build_visibility() {
    // The pass's one visibility truth: the material ensure, the plan's emit and the per-object loop all call this same
    // function (vis), so a bucket's representative can never be admitted by one test and dropped by another, and
    // every emitted lane's set was ensured. Memoised per object per pass.
    g_vis_memo_inj.begin(meshes);
    // INJ_FORK: every object's verdict now, across the pool, into the memo's own marks (the same calc every consumer
    // below would run through get). The calc reads the cull set, the occupancy grid and the distance memo - all built
    // and seeded above - and writes its own index only.
    // OCC_STATS: armed, each index also records whether the occlusion cull hid it - its own slot, as the marks are -
    // summed after the join.
    static std::vector<uint8_t> occ_hid;
    const bool occ_count = stats_on();
    if (occ_count) occ_hid.assign(meshes.size(), 0u);
    fork_each(static_cast<uint32_t>(meshes.size()), [&](uint32_t i) {
        const uint8_t c = vis_code(meshes[i]);
        g_vis_memo_inj.mark[i] = c == 0u ? 1u : 2u;
        if (occ_count) occ_hid[i] = c == 2u ? 1u : 0u;
    });
    if (occ_count) {
        uint64_t occ_n = 0;
        for (const uint8_t h : occ_hid) occ_n += h;
        stat_add(g_stats.vis_occluded, occ_n);
    }
    if (inst_on) {
        const auto vis_fn = [this](const RenderObject& o) { return vis(o); };
        mat_ensure_all(ctx, dev, meshes, vis_fn);   // MAT_TABLE: bases fixed before any lane is written.
        inst_plan_build(inst_plan, meshes, nullptr, meshes.size(),
                        [&](const RenderObject& o, InstKey& k) {
                            const InjRoute r = route_of(o);
                            k.route = r.nearz ? 2u : 0u;
                            k.aux = dl_bucket_hash(o);
                        });
        // The cull walk, once, against the pass's own frustum; the whole stream lands in the ring here.
        inst_plan_emit(ctx, dev, inst_plan, meshes, cam, 1, vis_fn, g_dist_memo_inj);
    }
}

inline uint8_t InjectPass::vis_code(const RenderObject& o) {
    float cc[3];
    obj_center_engine(o, cc);
    float bs[3];
    bounds_size_of(o, bs);   // CAST_REFIT: the cull's size; the LOD pick keeps size.
    if (!mesh_visible(cull, cc, lod_radius_of(bs, o.rot_m, o.rotated))) return 1u;
    if (occ.on) {
        const InjRoute r = route_of(o);
        if (!r.nearz && vis_occ_hidden(occ, o)) return 2u;
    }
    return 0u;
}

inline bool InjectPass::vis_calc(const RenderObject& o) { return vis_code(o) == 0u; }

inline bool InjectPass::vis(const RenderObject& rv_o) {
    return g_vis_memo_inj.get(rv_o, [this](const RenderObject& rc_o) { return vis_calc(rc_o); });
}

// One iteration of the mesh loop (meshes[oi]); a `return` here is the loop's `continue`.
inline void InjectPass::draw_object(size_t oi) {
    const RenderObject& o = meshes[oi];
    // Carried by its bucket's draw at the representative, or culled by the
    // emission's walk (only the representative stays undone).
    if (inst_on && inst_plan.done[oi]) return;
    ConstantData cbd = cbf;
    cbd.fx_meta[2] = static_cast<float>(g_main_depth_w);
    cbd.fx_meta[3] = static_cast<float>(g_main_depth_h);
    cbd.center_size[0] = o.pos[0];
    cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
    cbd.center_size[2] = o.pos[1];
    // Per-object rebase arm mirrors the pass verdict (one without the other is the wrong-space landmine).
    float res[3];   // POS_RES: the object's live residual.
    obj_pos_res(o, res);
    if (rebase_on) fill_center_rel(cbd, cam, res);
    cbd.size_axes[0] = o.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y].
    cbd.size_axes[1] = o.size[2];
    cbd.size_axes[2] = o.size[1];
    cbd.size_axes[3] = static_cast<float>(o.blend_mode);
    fill_obj_rot(cbd, o.rot_m);
    // blendCtl.x: never on this pass (see the pass head).
    cbd.blend_ctl[0] = 0.0f;
    // blend_ctl[3] is the crossfade dither; written here so it is never inherited from the previous object's fade.
    cbd.blend_ctl[3] = 0.0f;
    memcpy(cbd.color, o.color, sizeof(cbd.color));
    // The camera (engine space) serves both the guard's true fragment
    // distance and the solid band mask, so it is filled regardless.
    cbd.fx0[0] = cam[0];
    cbd.fx0[1] = cam[1];
    cbd.fx0[2] = cam[2];
    // per-mesh route verdicts (route_of, above the loop).
    const InjRoute rt = route_of(o);
    const bool mesh_nearz = rt.nearz;
    // Textured routing: a material-carrying solid takes the textured twin of exactly the variant this draw would
    // have used (plain / guard / arb).
    const MaterialSet* txm = obj_textured(o);
    const bool tx_arb = far_arb || mesh_nearz;
    const bool comp_vs = guard || tx_arb;
    const bool tx_on = txm != nullptr &&
        g_res.layout_tex && g_res.mat_sampler &&
        (tx_arb ? g_res.ps_comp_arb_tex != nullptr :
         guard      ? g_res.ps_comp_tex != nullptr :
                      g_res.ps_tex != nullptr) &&
        (comp_vs ? g_res.vs_comp_tex != nullptr
                     : g_res.vs_tex != nullptr);
    bind_object_shaders(tx_on, tx_arb, comp_vs);
    // Band / local-volume mask inputs (same conversion as the flush).
    fill_local_band_cb(cbd, o);
    fill_lighting_obj_cb(ctx, cbd, o);
    fill_object_guard(cbd, tx_arb, mesh_nearz);
    if (!o.fullscreen) {
        // The same vis the plan's emit ran, so a bucket representative that was emitted always draws here.
        if (!vis(o)) {  return; }
    }

    // TX_ONE_UPLOAD: a textured object's slice goes up per submesh inside draw_textured (with that submesh's
    // material lanes, before its draw) on every route it draws by - per object and by bucket alike - and no draw
    // reads b0 between here and there, so an object-level upload here would never be read. Untextured draws read
    // this one. Twins: the flush and the PIP.
    // A failed pass-level frame upload (frame_ok) stands every draw down - fail dark, never stale-frame draws.
    // Object slice only (the frame slice went up before the loop).
    if (!frame_ok || (!tx_on && !upload_obj_cb(ctx, g_res.composite_cb, cbd))) return;
    // twoSided semantics must survive arb frames: a flat D3D11_CULL_BACK force erases interior faces there. Any shader
    // kind (a scene-read effect alpha-lerps on normal blend just like PSMain).
    const bool ts_ordered = o.two_sided &&
                            o.blend_mode == 0 && o.color[3] < 0.999f &&
                            g_res.rasterizer_front != nullptr;
    ID3D11RasterizerState* want_rs =
        ts_ordered ? g_res.rasterizer_front :
        o.two_sided    ? g_res.rasterizer : g_res.rasterizer_cull;

    if (want_rs != bound_rs) {
        ctx->RSSetState(want_rs);
        bound_rs = want_rs;
    }

    vp_select(mesh_nearz ? 1 : 0);   // The near-gap swap.

    const int mid = mesh_id_clamp(o.mesh);

    const VbRef vb = mesh_vb_ref(mid, o.slot);   // CLOTH.
    if (vb != bound_vb) {
        ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
        ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
        bound_vb = vb;
    }

    if (o.blend_mode != bound_bm) {
        ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);
        bound_bm = o.blend_mode;
    }

    // The pick reads g_lod.proj_px (lod_publish above, else the last value kept); only while it is unpublished
    // (0) does every object select level 0 rather than guess a projection scale.
    const MeshDef& md = mesh_def(mid);
    int lod = 0;
    float lodt = 0.0f;
    // A locked instance never picks - level 0, no fade. Twin at the flush.
    if (!lod_locked(o)) {   // LOD_SIM_SLOT.
        lod_pick(md, lod_radius(o), sqrtf(g_dist_memo_inj.get(o, cam)), lod, lodt);
    }
    const bool lodx = lodt > 0.0f;

    // This object is its bucket's representative (see the flush twin). The VS is the composite instanced pair when
    // this draw took the composite VS, the static pair otherwise. The bucket's ranges were emitted before the loop,
    // so the per-object LOD walk runs zero iterations. Members share the route verdict by key.
    int lod_iters = lodx ? 2 : 1;

    if (inst_on && inst_plan.batch_of[oi] >= 0) {
        // SKIN_POOL: a skin pool chunk's bucket pulls each member's own pose (the Sk twins: no slot-0
        // vertices, the chunk at t52).
        const SkinPoolChunk* const ipc = inst_bucket_pool(inst_plan, static_cast<uint32_t>(oi));
        ID3D11VertexShader* ivs = ipc
            ? (tx_on ? (comp_vs ? g_res.vs_comp_inst_sk_tex : g_res.vs_inst_sk_tex)
                         : (comp_vs ? g_res.vs_comp_inst_sk     : g_res.vs_inst_sk))
            : (tx_on ? (comp_vs ? g_res.vs_comp_inst_tex : g_res.vs_inst_tex)
                         : (comp_vs ? g_res.vs_comp_inst     : g_res.vs_inst));
        ID3D11InputLayout*  iil = ipc ? g_res.layout_inst_sk
                                              : (tx_on ? g_res.layout_inst_tex : g_res.layout_inst);

        if (ivs && iil) {
            if (ivs != bound_vs) { ctx->VSSetShader(ivs, nullptr, 0); bound_vs = ivs; }
            if (iil != bound_il) { ctx->IASetInputLayout(iil); bound_il = iil; }
            inst_sk_bind(ctx, ipc, bound_sk);

            if (!inst_vb_bound) {
                const UINT is = INST_STRIDE, io = 0;
                ctx->IASetVertexBuffers(1, 1, &g_res.inst_vb[1], &is, &io);
                inst_vb_bound = true;
            }

            if (o.two_sided && !ts_ordered && bound_rs != g_res.rasterizer) {
                ctx->RSSetState(g_res.rasterizer);
                bound_rs = g_res.rasterizer;
            }

            // The bucket's ranges were emitted before the loop (cull, LOD
            // and route verdicts are in the plan), so this draws them.
            uint32_t in = 0;
            const uint32_t id = inst_draw(ctx, dev, inst_plan, meshes, static_cast<uint32_t>(oi),
                cbd, g_res.composite_cb, tx_on ? txm : nullptr, ts_ordered,
                bound_rs, tx_arb ? 2 : (guard ? 1 : 0), bound_ps, 1, in);
            stat_add(g_stats.composite_meshes, id);
            if (tx_on) stat_add(g_stats.textured_draws, id);
            if (ipc) stat_add(g_stats.skin_inst, in);   // SKIN_POOL.
            // The batch's translucent part: the same gathered set, part 2, staged for the tail like a per-object
            // part - each lane its own tail entry, far-to-near among every other part (INST_PART_LANE; dist2 is
            // unused for it). It must not draw here: the AO term (ssao_post) multiplies every marked pixel, and
            // a blend part drawn ahead of it sits on its own solid's mark and takes the solid's term.
            if (tx_on && txm && in > 0 && obj_has_blend(o)) {
                InjPart p;
                p.oi = static_cast<uint32_t>(oi);
                p.cbd = cbd;
                p.ps = bound_ps;
                p.vs = ivs;
                p.il = iil;
                p.pool = ipc;   // SKIN_POOL.
                p.ctx = tx_arb ? 2 : (guard ? 1 : 0);
                p.lod = 0;
                p.lodt = 0.0f;
                p.lodx = false;   // The plan's ranges carry their own levels.
                p.vpsel = mesh_nearz ? 1 : 0;
                p.ts_ordered = o.two_sided && g_res.rasterizer_front != nullptr;
                p.inst = true;
                p.dist2 = g_dist_memo_inj.get(o, cam);
                parts.push_back(p);
            }
            lod_iters = 0;   // The batch drew; no per-object level walk.
        } else {
            inst_unbatch(inst_plan, static_cast<uint32_t>(oi));   // INST_UNBATCH.
        }
    }

    for (int li = 0; li < lod_iters; ++li) {
        const int lvl = lod + li;

        if (lodx) {
            cbd.blend_ctl[3] = lod_dither(lodt, li != 0);
            // TX_ONE_UPLOAD: textured, draw_textured uploads the dither with each submesh.
            if (!tx_on && !upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
        }

        if (tx_on) {
            // Per-submesh textured draws. Part 1 - the solid texels; the object's translucent part is staged below
            // and drawn in the parts tail (BLEND_PART_INJ).
            const uint32_t txd = draw_textured(ctx, dev, cbd, g_res.composite_cb,
                                               *txm, mid, ts_ordered, bound_rs,
                                               tx_arb ? 2 : (guard ? 1 : 0), bound_ps,
                                               lvl, 1);
            stat_add(g_stats.composite_meshes, txd);
            stat_add(g_stats.textured_draws, txd);
        } else {
            UINT ls = 0, lc = 0;
            mesh_lod_range(md, lvl, ls, lc);

            if (ts_ordered && bound_rs != g_res.rasterizer_front) {
                ctx->RSSetState(g_res.rasterizer_front);
                bound_rs = g_res.rasterizer_front;
            }

            ctx->DrawIndexed(lc, ls, 0);
            stat_inc(g_stats.composite_meshes);

            if (ts_ordered) {   // Pass 2: front faces over the interior.
                ctx->RSSetState(g_res.rasterizer_cull);
                bound_rs = g_res.rasterizer_cull;
                ctx->DrawIndexed(lc, ls, 0);
                stat_inc(g_stats.composite_meshes);
            }
        }
    }
    // The solid draw's set, for the parts tail.
    ID3D11PixelShader*  if_ps = bound_ps;
    ID3D11VertexShader* if_vs = bound_vs;
    ID3D11InputLayout*  if_il = bound_il;

    cbd.blend_ctl[3] = 0.0f;   // Never leaks into the next object.

    // Stage this object's translucent part for the tail. The CB is the slice the solid draw uploaded; the dither
    // lane is cleared (the light lanes are re-appended at the tail).
    if (lod_iters > 0 && tx_on && txm && obj_has_blend(o)) {
        InjPart p;
        p.oi = static_cast<uint32_t>(oi);
        p.cbd = cbd;
        p.ps = if_ps;
        p.vs = if_vs;
        p.il = if_il;
        p.ctx = tx_arb ? 2 : (guard ? 1 : 0);
        p.lod = lod;
        p.lodt = lodt;
        p.lodx = lodx;
        p.vpsel = mesh_nearz ? 1 : 0;
        p.ts_ordered = o.two_sided && g_res.rasterizer_front != nullptr;
        p.inst = false;
        p.dist2 = g_dist_memo_inj.get(o, cam);
        parts.push_back(p);
    }
}

inline void InjectPass::bind_object_shaders(bool tx_on, bool tx_arb, bool comp_vs) {
    ID3D11PixelShader* want_ps =
        tx_on ? (tx_arb ? g_res.ps_comp_arb_tex
                 : guard    ? g_res.ps_comp_tex
                                : g_res.ps_tex)
        : tx_arb ? g_res.ps_composite_arb
        : guard                             ? g_res.ps_composite
                                            : g_res.ps;
    if (want_ps != bound_ps) {
        ctx->PSSetShader(want_ps, nullptr, 0);
        bound_ps = want_ps;
    }
    ID3D11VertexShader* want_vs =
        tx_on ? (comp_vs ? g_res.vs_comp_tex : g_res.vs_tex)
                  : (comp_vs ? g_res.vs_composite : g_res.vs);
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

inline void InjectPass::fill_object_guard(ConstantData& cbd, bool tx_arb, bool mesh_nearz) {
    if (guard || tx_arb) {   // The pair and range, for the cut and the lane.
        // Guard inputs: reconstruction coefficients + the encode range of
        // the copied depth, the copy's pixel dimensions, and the margins.
        cbd.depth_params[0] = pv.projection[2][2];
        cbd.depth_params[1] = pv.projection[3][2];
        fill_depth_range_cb(cbd);
        // Twin: both CB editions carry it. The copy's dimensions only when the copy is what the draw reads; a
        // routed draw without one keeps the scene's (the mask pixel clamps read these lanes too).
        if (guard) {
            cbd.fx_meta[2] = static_cast<float>(g_res.comp_depth_w);
            cbd.fx_meta[3] = static_cast<float>(g_res.comp_depth_h);
        }

        const bool exact_encode = measured || snap_slot_near > 0.0f;
        // The classic tiers return only as the no-heightfield terrain backstop.
        if (far_arb) {
            // fxParams1.zw carry the analytic depth clamp: offset cap (the LOD margin) + the contact-band height
            // its ramp closes over; thm owns terrain in the same pass.
            cbd.fx1[0] = 1e9f;
            cbd.fx1[1] = 0.0f;
            cbd.fx1[2] = FAR_ARB_GUARD_BASE;
            cbd.fx1[3] = far_arb_contact();
        } else if (g_thm.valid || !guard) {
            // A live heightfield (it owns terrain), or a near-gap routed draw without the guard (no fresh snapshot,
            // or no composite shader; a far-routed draw takes the branch above): the guard stands down - the same
            // contract as the no-copy branch below (zero margins against an absent or stale depth is the
            // invisible-geometry landmine).
            cbd.fx1[0] = 1e9f;
            cbd.fx1[1] = 0.0f;
        } else {
            cbd.fx1[0] = exact_encode ? COMPOSITE_GUARD_BASE_MEASURED : COMPOSITE_GUARD_BASE;
            cbd.fx1[1] = exact_encode ? COMPOSITE_GUARD_REL_MEASURED : COMPOSITE_GUARD_REL;
        }
        if (mesh_nearz) {
            cbd.fx_meta[0] =
                nz_near;
            cbd.fx_meta[1] = nz_gap_lo;
        }
        g_inj_pub.dp[0] = cbd.depth_params[0];
        g_inj_pub.dp[1] = cbd.depth_params[1];
        g_inj_pub.dp[2] = cbd.depth_params[2];
        g_inj_pub.dp[3] = cbd.depth_params[3];
        g_inj_pub.dp_valid = true;
    } else {
        // No depth copy this frame: the fallback PS carries the same guard contract, so stand it down explicitly -
        // zero margins with invalid depth is the invisible-geometry landmine.
        cbd.fx1[0] = 1e9f;
    }
}

inline void InjectPass::defer_parts_to_flush() {
    // WSEE: the capture here (wsee_inj_capture: after our solids and their AO, before our first translucent
    // part), in the world's own injection alone (inj_world: not an anomalous rescue, the engine's viewport the
    // world's). Then, where the cycle sorts (wsee_defer_ok; not a far phase - PART_NEAR: the camera partition
    // draws after us under another pair - and no partitioned pair, wsee_pair_split), the parts whose geometry the
    // flush can draw as the injection drew it (wsee_defer_group: the mesh buffer named, own, or the vertices held;
    // a bucket's part only with every visible member so, a skin pool bucket too) are noted as copies and not drawn
    // here - only a part the flush can composite (normal blend, the mesh viewport, no effect, not depth-Off:
    // ws_sort_this's terms).
    const bool inj_world = !anomalous && n_saved_vp >= 1 && reorder_world_vp(saved_vp[0]);
    wsee_inj_capture(ctx, inj_world);
    if (!parts.empty() && inj_world && !far_phase &&
        !wsee_pair_split(pv.projection[2][2], pv.projection[3][2]) && wsee_defer_ok(dev, ctx)) {
        static std::vector<const RenderObject*> dms;   // One part's objects (wsee_defer_group),
        static std::vector<int> dlv;                     // their levels and crossfades.
        static std::vector<float> dtv;
        for (InjPart& d : parts) {
            if (!dok(meshes[d.oi], d.vpsel)) continue;
            if (!d.inst) {
                // The object's geometry held, or its mesh buffer named, at the levels this part would have drawn
                // here; refused, the part stays here.
                const RenderObject* const d1 = &meshes[d.oi];
                const int dl = d.lod;
                const float dt = d.lodx ? d.lodt : 0.0f;
                d.ws_defer = wsee_defer_group(dev, ctx, &d1, &dl, &dt, 1u);
                continue;
            }
            // A skin pool bucket too (d.pool): its members are drawn from their own slices, which are held.
            if (d.oi >= inst_plan.batch_of.size() || inst_plan.batch_of[d.oi] < 0) continue;
            const uint32_t db = static_cast<uint32_t>(inst_plan.batch_of[d.oi]);
            if (db >= inst_plan.batch_begin.size() || db >= inst_plan.batch_n.size()) continue;
            const uint32_t d0 = inst_plan.batch_begin[db], dn = inst_plan.batch_n[db];
            if (static_cast<size_t>(d0) + dn > inst_plan.members.size()) continue;
            bool dall = true;
            dms.clear();
            dlv.clear();
            dtv.clear();
            for (uint32_t dk = 0; dk < dn && dall; ++dk) {
                const uint32_t dm = inst_plan.members[d0 + dk];
                if (dm >= meshes.size()) { dall = false; break; }
                const bool dvis = dm < g_vis_memo_inj.mark.size() && g_vis_memo_inj.mark[dm] == 1u;
                if (!dvis) continue;
                if (!dok(meshes[dm], d.vpsel)) {
                    dall = false;
                    continue;
                }
                dms.push_back(&meshes[dm]);
                // The member's level and crossfade as the bucket's emission picked and drew them
                // (inst_plan_emit's lod_l / lod_t - one source, no second copy of its rule).
                const int dl = dm < inst_plan.lod_l.size() ? inst_plan.lod_l[dm] : 0;
                const float dt = dm < inst_plan.lod_t.size() ? inst_plan.lod_t[dm] : 0.0f;
                dlv.push_back(dl);
                dtv.push_back(dt);
            }
            if (!dall) continue;
            // The visible members all or none, each with its entry.
            d.ws_defer = wsee_defer_group(dev, ctx, dms.data(), dlv.data(), dtv.data(),
                                          dms.size());
        }
    }
}

inline bool InjectPass::dok(const RenderObject& do_p, int dvp) {
    return do_p.blend_mode == 0 && dvp == 0 && do_p.effect == 0 && !do_p.fullscreen &&
           do_p.mode != DepthMode::Off;
}

// The tail: every staged part, far to near, under the exact state its solid drew with, dss_test, the object's blend
// state, interiors-then-exteriors when twoSided, else back-face culled. The LOD crossfade is honoured. dss goes back to
// the plain write state afterwards - never the marking one: the eraser has run (SSAO_MARK), so a mark left past it
// would reach the next drawer's apply.
inline void InjectPass::draw_parts_tail() {
    static std::vector<InjPartEnt> parts_order;
    static std::vector<InstPartLane> part_lanes;   // Scratch: one bucket's lanes (inst_part_lanes).
    if (!parts.empty()) {
        parts_order.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(parts.size()); ++i) {
            if (parts[i].ws_defer) continue;   // WSEE: the flush's tail draws it.
            if (!parts[i].inst) {
                parts_order.push_back({ i, ~0u, 0, parts[i].dist2 });
                continue;
            }
            inst_part_lanes(inst_plan, parts[i].oi, part_lanes);   // INST_PART_LANE.
            for (const InstPartLane& l : part_lanes) {
                parts_order.push_back({ i, l.lane, l.lvl, l.d2 });
            }
        }
        std::stable_sort(parts_order.begin(), parts_order.end(),
                         [](const InjPartEnt& a, const InjPartEnt& b) { return a.d2 > b.d2; });
        ctx->OMSetDepthStencilState(g_res.dss_test, 0);
        uint32_t lights = ~0u;   // INST_PART_LANE: the part whose lights the ring holds last (below).

        for (const InjPartEnt& e : parts_order) {
            InjPart& p = parts[e.part];
            const RenderObject& o = meshes[p.oi];
            const MaterialSet* txm = obj_textured(o);
            if (!txm) continue;
            if (p.ps != bound_ps) { ctx->PSSetShader(p.ps, nullptr, 0); bound_ps = p.ps; }
            if (p.vs != bound_vs) { ctx->VSSetShader(p.vs, nullptr, 0); bound_vs = p.vs; }
            if (p.il != bound_il) { ctx->IASetInputLayout(p.il); bound_il = p.il; }

            vp_select(p.vpsel);

            if (o.blend_mode != bound_bm) {
                ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);
                bound_bm = o.blend_mode;
            }

            if (!p.ts_ordered && bound_rs != g_res.rasterizer_cull) {
                ctx->RSSetState(g_res.rasterizer_cull);
                bound_rs = g_res.rasterizer_cull;
            }

            const int mid = mesh_id_clamp(o.mesh);

            const VbRef vb = mesh_vb_ref(mid, o.slot);   // CLOTH.
            if (vb != bound_vb) {
                ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
                bound_vb = vb;
            }

            // DL_RING: a ring region is this object's only until the next append, and every later object
            // appended after its solid - a device without NO_OVERWRITE reuses offset 0 on every append, and a wrap or
            // growth renames the ring. So the part appends its lights again here, next to its own draws: the same pool
            // as the solid's fill (this pass), so the same set; the lanes cleared first, as a fresh template carries
            // them, so a failed append draws unlit. A bucket's part takes its representative's, as its solid did.
            // INST_PART_LANE: a bucket's lanes drawn one after another append once - nothing appends between them; a
            // lane drawn after another part's entry appends its part's lights again (one selection per run of the
            // part's lanes).
            if (e.part != lights) {
                memset(p.cbd.dl_ctl, 0, sizeof(p.cbd.dl_ctl));
                memset(p.cbd.dl_first, 0, sizeof(p.cbd.dl_first));
                if (o.lit) fill_dynlights_cb(ctx, p.cbd, o);
                lights = e.part;
            }

            // WSEE: the part's draw as one call, so the re-draws into T and NW (wsee_tail, below) are this same
            // draw - rep: such a re-draw, which counts nothing.
            auto geo = [&](bool rep) {
                if (p.inst) {
                    // This lane of the bucket (the instance VB at slot 1 is still bound: nothing in this pass unbinds
                    // it, and ssao_post's StateBackup put back what it found).
                    inst_sk_bind(ctx, p.pool, bound_sk);   // SKIN_POOL: its chunk (t52 is in the backup).
                    uint32_t in = 0;
                    const InstPartLane one = { e.d2, e.lane, e.lvl };   // INST_PART_LANE.
                    const uint32_t n = inst_draw(ctx, dev, inst_plan, meshes, p.oi, p.cbd,
                                                 g_res.composite_cb, txm, p.ts_ordered, bound_rs,
                                                 p.ctx, p.ps, 2, in, &one);
                    if (rep) return;
                    stat_add(g_stats.composite_meshes, n);
                    stat_add(g_stats.textured_draws, n);
                    // SKIN_POOL: instanced lanes (the flush draws its parts per object: none counted there).
                    if (p.pool) stat_add(g_stats.skin_inst, in);
                    return;
                }
                for (int li = 0; li < (p.lodx ? 2 : 1); ++li) {
                    const int lvl = p.lod + li;
                    p.cbd.blend_ctl[3] = p.lodx ? lod_dither(p.lodt, li != 0) : 0.0f;
                    // draw_textured uploads the slice per submesh (the dither rides along).
                    const uint32_t n = draw_textured(ctx, dev, p.cbd, g_res.composite_cb, *txm, mid,
                                                     p.ts_ordered, bound_rs, p.ctx, p.ps, lvl, 2);
                    if (rep) continue;
                    stat_add(g_stats.composite_meshes, n);
                    stat_add(g_stats.textured_draws, n);
                }
            };
            geo(false);
            // WSEE: a part drawn here while this cycle's capture stands goes into T and NW as the flush's tail
            // goes (wsee_tail): normal blend, the mesh viewport (the near-gap route lies below the world's range).
            // Its pixel shader writes the coverage as straight alpha here (blendCtl.x is 0 on this pass).
            // A refused re-draw (the bound depth not the capture's) taints, as the flush's does.
            if (o.blend_mode == 0 && p.vpsel == 0 && wsee_tail_owed()) {
                if (!wsee_tail(ctx, [&](bool) { geo(true); })) {
                    wsee_taint(g_topo_cycles);   // T and NW would miss our layer.
                }
            }
        }

        ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);   // Plain write: the eraser is behind us.
    }
}

// Rescue or landing: the cycle's bookkeeping after the pass (the flush's landing witness, the stats, the cold-start
// metric).
inline void InjectPass::stamp_landing() {
    if (anomalous) {
        // Not an injection for the hybrid guarantee's serial (the next flush must judge the next frame on its own
        // injection, not this frame's rescue) and not a cold-start metric.

        g_rescue_frame_count++;

        if (g_rescue_frame_count < RESCUE_MAX_PER_FRAME) {
            g_ro.injected = false;
            g_ro.inject_attempts = 0;
        }
    } else if (elig_n > 0) {   // SCENE: landed = eligible set non-empty, pre-cull aside.
        stat_inc(g_stats.composite_injections);
        g_composite_inject_serial.fetch_add(1, std::memory_order_relaxed);
        g_composite_land_cycle = g_topo_cycles;   // NO_PARK_LANDED.
    }
    g_ro.opaques_since_inject = 0;   // The flush's repaint check counts from this landing.

    if (!anomalous && g_mask.cold_t0 >= 0.0 && g_mask.cold_first_inject < 0.0f) {
        g_mask.cold_first_inject = static_cast<float>(effect_time_seconds() - g_mask.cold_t0);
    }
}

inline void inject_composited_meshes(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_INJECT);
    KH_GPU_SCOPE(ctx, GPU_INJECT);   // PROF.
    if (fsaa_world_standdown()) {  return; }
    // The pass's scratch, kept across calls so they never allocate per frame (each stage clears, fills or rebuilds
    // what it uses before reading it).
    static std::vector<RenderObject> meshes;
    static std::vector<RenderObject> occx;   // OCC_FORCE: declared occluders this pass does not draw.
    static std::vector<uint32_t> elig;   // SCENE: every eligible slot, pre-cull independent.
    static InstPlan inst_plan;
    static VisOcc occ;
    static std::vector<InjPart> parts;
    InjectPass pass(ctx, meshes, occx, elig, inst_plan, occ, parts);
    pass.walk_eligible();                    // ATTACH's final step, the publish, the sync; the eligible set.
    if (!pass.stamp_and_triage()) return;    // Nothing eligible, no caster: the pass ends there.
    if (!pass.ensure_device()) return;
    pass.render_shadow_maps();               // dls_select, the sun ladder (SUN_GEN), the light maps.
    pass.replay_and_resync();                // VOL_REPLAY; OBJBUF_LATE's re-sync and eligible list.
    if (!pass.pick_projection()) return;     // The cycle's camera (the latch, the live fetch, the last kept).
    pass.choose_depth_pair();                // The adopted view; the depth pair this pass encodes with.
    if (!pass.check_partition()) return;     // PART_NEAR: a mesh too near for this partition's near plane.
    if (!pass.check_anomaly()) return;       // An anomalous cycle outside the rescue window.
    pass.publish_encode();                   // The pair and view the flush takes; the far route; the lights.
    pass.supervise_view_source();            // The live-shadow view source's lock and health.
    pass.adopt_camera();                     // cam through the honest inverse; ENGCAM; FOG_PROBE's camera.
    pass.cull_candidates();                  // The grid pre-cull: this pass's draws and OCC_FORCE occluders.
    pass.order_meshes();                     // MESH_ORDER; MIR_REPLAY's finish.
    pass.draw();                             // The mesh pass and the parts tail, under one StateBackup.
}
}