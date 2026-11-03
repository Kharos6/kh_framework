#pragma once

// The feature half (rendering_integration_features.hpp): our sun shadow render (render_sun_depth): the camera pick, the
// admission and the generation caps. Never include it on its own.
namespace RenderIntegration {
// KH_SKIN_CULL: render_sun_depth's camera and caster admission, shared with the skin cull (kh_skin_cull_pass) so the
// two cannot drift (TWIN by construction). kh_sun_cam_pick: the camera (cam_e; false = camera-less, which admits
// every shadow-active object).
inline bool kh_sun_cam_pick(float cam_e[3]) {
    bool cam_valid = false;
    cam_e[0] = cam_e[1] = cam_e[2] = 0.0f;
    // KH_SUN_CAM_LATCH: this camera is the anchor, the caster cylinder, the tier windows and the range fade's centre.
    // Do not prefer the raw bridge sample to the latch: it is one step ahead of the frame at ground and foreign (up to
    // 156 m) at altitude, so the whole shadow domain sits that far off the camera the meshes draw from - invisible
    // while shadows stay under their casters, a rim that leads the camera once a low sun stretches them out to it.
    // The injection's own take: the cycle latch, with the bridge admitted over it only when the clear-time fetch
    // failed and the latch is more than KH_LATCH_JUMP_M behind it (inject_composited_meshes' rule). With no cycle
    // latch: the bridge, else g_ls.cam; none of them = camera-less.
    {
        RVExtBridge::ProjectionViewTransform cpv = {};
        const bool khsc_live = RVExtBridge::get_projection_view_transform(cpv);

        if (g_ro.cycle_pv_valid) {
            kh_cam_of(g_ro.cycle_pv.view, cam_e);
            cam_valid = true;
            if (khsc_live && !g_boundary_pv_valid) {
                float khsc_lc[3];
                kh_cam_of(cpv.view, khsc_lc);
                if (latch_cam_dist_sq(cam_e, khsc_lc) > KH_LATCH_JUMP_M * KH_LATCH_JUMP_M) {
                    cam_e[0] = khsc_lc[0]; cam_e[1] = khsc_lc[1]; cam_e[2] = khsc_lc[2];
                }
            }
        } else if (khsc_live) {
            kh_cam_of(cpv.view, cam_e);
            cam_valid = true;
        } else if (g_ls.cam[0] != 0.0f || g_ls.cam[1] != 0.0f || g_ls.cam[2] != 0.0f) {
            cam_e[0] = g_ls.cam[0];
            cam_e[1] = g_ls.cam[1];
            cam_e[2] = g_ls.cam[2];
            cam_valid = true;
        }
    }
    return cam_valid;
}
// The cylinder about the camera, else the down-sun reach rescue: whether a shadow-active object is a sun caster for
// the camera cam_e (a valid one) and the clamped range khsr_rad.
inline bool kh_sun_admits(const RenderObject& o, const float cam_e[3], float khsr_rad) {
    const float ce0[3] = { o.pos[0], o.pos[2], o.pos[1] };
    // KH_LOD_BOUNDS: the world box around everything it may draw - the cylinder and the drop below are in world axes.
    float he0[3];
    kh_world_half_extents(o, he0);
    const float dx = ce0[0] - cam_e[0];
    const float dy = ce0[1] - cam_e[1];
    const float dz = ce0[2] - cam_e[2];
    const float hd = sqrtf(he0[0] * he0[0] + he0[1] * he0[1] + he0[2] * he0[2]);
    const float lim = khsr_rad + hd;

    // Cylinder, matching KhSunRangeFade: a sphere refuses casters whose shadow lands on ground the shader still asks
    // about.
    if (dx * dx + dz * dz > lim * lim || fabsf(dy) > lim) {
        // The sole guard is the sy > 1e-4 horizon test below (a validity floor, not a length cap); the reach feeds
        // only this eligibility test, never the map fit.
        bool khsr_in = false;

        if (g_sun_valid) {   // The reach rescue needs a published sun.
            const float khsr_sx = g_sun_dir_engine[0];
            const float khsr_sy = g_sun_dir_engine[1];
            const float khsr_sz = g_sun_dir_engine[2];
            const float khsr_hl = sqrtf(khsr_sx * khsr_sx + khsr_sz * khsr_sz);

            if (khsr_hl > 1.0e-4f && khsr_sy > 1.0e-4f) {
                float khsr_drop = (ce0[1] - he0[1]) - cam_e[1];
                if (khsr_drop < 0.0f) khsr_drop = 0.0f;
                const float khsr_k = khsr_hl / khsr_sy;   // Down-sun metres per metre
                                                          // of drop.
                const float khsr_ux = -khsr_sx / khsr_hl;   // Shadow runs opposite.
                const float khsr_uz = -khsr_sz / khsr_hl;
                // + lim: without it the ray stops 2 * he.y below the camera and every receiver deeper than that is
                // invisible to the test (at a low sun, (camera AGL) * k of missing reach - the shadow pop-in). A
                // receiver inside the shadow view distance is at most lim below the camera, so this is the tight bound.
                const float khsr_span = 2.0f * he0[1] + khsr_drop + lim;
                const float khsr_ay = dy - he0[1];   // Camera -> caster base, vertical.
                const float khsr_h = dx * khsr_ux + dz * khsr_uz;
                // The shadow is a band, not a line, and on a cylinder the horizontal and vertical tests are
                // independent given s, so the exact answer is an interval intersection. Horizontal: |q + u*k*s| <=
                // lim is a quadratic in s; no root means never in range.
                const float khsr_qa = khsr_k * khsr_k;
                const float khsr_qb = 2.0f * khsr_k * khsr_h;
                const float khsr_qc = dx * dx + dz * dz - lim * lim;
                const float khsr_disc = khsr_qb * khsr_qb - 4.0f * khsr_qa * khsr_qc;
                if (khsr_disc >= 0.0f && khsr_qa > 1.0e-12f) {
                    const float khsr_rt = sqrtf(khsr_disc);
                    float khsr_lo = (-khsr_qb - khsr_rt) / (2.0f * khsr_qa);
                    float khsr_hi = (-khsr_qb + khsr_rt) / (2.0f * khsr_qa);
                    // Vertical: at s the band spans [ay - s, ay - s + 2he], so it reaches the camera's +-lim window
                    // exactly on this range.
                    if (khsr_lo < 0.0f) khsr_lo = 0.0f;
                    if (khsr_lo < khsr_ay - lim) khsr_lo = khsr_ay - lim;
                    if (khsr_hi > khsr_span) khsr_hi = khsr_span;
                    const float khsr_vhi = khsr_ay + 2.0f * he0[1] + lim;
                    if (khsr_hi > khsr_vhi) khsr_hi = khsr_vhi;
                    khsr_in = khsr_lo <= khsr_hi;
                }
            }
        }
        return khsr_in;
    }
    return true;
}

// KH_SUN_GEN - the caster generation (g_scene.gen_cast: syncs that took an object able to cast) the ladder was last
// built from, stamped at render_sun_depth's head and again at its own sync. The cast fire builds the ladder first
// in a cycle and the once-per-frame flag then holds it, so a game-thread edit of a caster (updateRender3D) that the
// injection's or the flush's later sync took would draw at its new pose over maps of its old one - a mesh moving
// down-sun reads its own previous surface as an occluder. Those passes re-open the flag when the caster generation
// moved over standing maps (KH_SUN_RESTEP's rule). The cost: nothing on a cycle with no caster taken in between;
// with one, the caster walk runs again, and where the caster set's hash changed the union and every tier redraw
// there (each tier key carries the whole caster hash) - the draw the next cycle's fire would have made, unless a
// caster is written again before it (then one more that cycle).
static uint64_t g_sun_map_gen = 0;
// KH_SUN_GEN_CAP - the re-open's cost bound. A caster written every frame (a scripted mover) would hit the window on
// most cycles and pay a second ladder a frame without end. The re-opens are counted over a window of
// KH_SUN_GEN_CAP_WIN cycles; past KH_SUN_GEN_CAP_N in one window they are suspended for KH_SUN_GEN_CAP_HOLD cycles,
// during which such a frame keeps the cast fire's ladder - a steady one-frame lag for the hold rather than exact on
// some frames and a frame late on the others (a lag that comes and goes is worse than a steady one). A re-open
// refused under the hold keeps the hold at least CAP_WIN cycles past it, so the hold ends only after a window with
// no re-open asked: a caster written every frame stays on the fire's ladder for as long as it is written (not CAP_N
// exact frames in every CAP_HOLD + CAP_N), and an occasional edit (a mesh shown, moved once) is corrected once the
// writing stops. Bound: at most CAP_N extra ladders per window of CAP_WIN cycles - up to a quarter of frames at the
// defaults, from a caster written about every fourth frame, which never passes CAP_N in a window and so never trips
// the hold (CAP_N per CAP_N + CAP_HOLD cycles, ~2.6 %, holds only once the hold has tripped).
// Render thread (or the game thread under a park, which stalls it); reset with g_sun_map_gen. KH_SUN_RESTEP's own
// re-open (a followed object the final step moved) is not counted.
static constexpr uint64_t KH_SUN_GEN_CAP_WIN  = 64;
static constexpr uint32_t KH_SUN_GEN_CAP_N    = 16;
static constexpr uint64_t KH_SUN_GEN_CAP_HOLD = 600;
static uint64_t g_sun_gen_win_start  = 0;
static uint32_t g_sun_gen_win_n      = 0;
static uint64_t g_sun_gen_hold_until = 0;
// True when a KH_SUN_GEN re-open may run this cycle (counts it); false under the hold.
inline bool kh_sun_gen_reopen_ok() {
    const uint64_t khsg_c = g_topo_cycles;
    if (khsg_c < g_sun_gen_hold_until) {
        const uint64_t khsg_e = khsg_c + KH_SUN_GEN_CAP_WIN;   // The hold ends a quiet window after the last ask.
        if (khsg_e > g_sun_gen_hold_until) g_sun_gen_hold_until = khsg_e;
        return false;
    }
    if (khsg_c < g_sun_gen_win_start || khsg_c - g_sun_gen_win_start >= KH_SUN_GEN_CAP_WIN) {
        g_sun_gen_win_start = khsg_c;
        g_sun_gen_win_n = 0;
    }
    if (g_sun_gen_win_n >= KH_SUN_GEN_CAP_N) {
        g_sun_gen_hold_until = khsg_c + KH_SUN_GEN_CAP_HOLD;
        g_sun_gen_win_n = 0;
        return false;
    }
    ++g_sun_gen_win_n;
    return true;
}

inline bool render_sun_depth(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(KHP_SUN_LADDER);
    if (g_sun_map_rendered_frame) return g_sun_map_valid;
    g_sun_map_rendered_frame = true;
    g_sun_map_gen = g_scene.gen_cast;   // KH_SUN_GEN: as this call finds it (the sync below restamps it).
    // KH_PROF: the GPU stamp past the once-per-frame return. A zone reports its last run, and a frame calls this
    // twice (the cast fire, then the injection): stamped at the head, gSunLadder was the second call's return.
    KH_GPU_SCOPE(ctx, KHG_SUN_LADDER);

    float cam_e[3] = { 0.0f, 0.0f, 0.0f };
    const bool cam_valid = kh_sun_cam_pick(cam_e);   // KH_SKIN_CULL: the shared pick.

    // KH_DLS_LOD_CAM: the light maps' level camera, here - ahead of the no-sun returns below, as the light
    // maps need no sun - and zero when camera-less, as the anchor below takes it.
    memcpy(g_sun_cam_now, cam_e, sizeof(g_sun_cam_now));

    // Same clamp as every other g_sun_range consumer; hoisted once, read by every caster test.
    const float khsr_rad = fminf(fmaxf(g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
    // KH_SKIN_CULL: the steps culled against the range they read; one the game thread moved since (the video
    // options) may admit what they culled, which is skinned first - ahead of the no-sun returns too, as the light
    // maps (kh_dls_frame, after this call) reach by the range with or without a sun.
    if (ctx && g_skin_cull_any && (khsr_rad != g_skin_cull_rad_lo || khsr_rad != g_skin_cull_rad_hi)) {
        kh_skin_cull_catchup(ctx);
    }

    // No published sun: no self / cast map. Without a sky witness validity drops here so frozen consumers stand down
    // too; with one the call returns at the `!sun` test below (the witness carries no map - kh_sun_axis_witnessed),
    // where the flags are already down: g_sun_valid clears only at the session resets, which drop them as well.
    if (!g_sun_valid && !kh_sun_axis_witnessed()) {
        g_sun_map_valid = false;
        g_sun2_map_valid = false;
        g_sun3_map_valid = false;
        g_sun4_map_valid = false;
        g_sun5_map_valid = false;
        return false;
    }
    if (!ctx) return false;

    // The maps render from the live derivation - the axis the engine's own cascades are built from, the reference the
    // eye compares our shadow against.
    const float* sun = kh_shadow_sun();

    // KH_SUN_AXIS_FOLD: the maps render from the publish itself.
    if (!sun) return false;

    // Camera-less flushes anchor at the origin (world-absolute).
    if (cam_valid) {
        g_sun_anchor_now[0] = cam_e[0];
        g_sun_anchor_now[1] = cam_e[1];
        g_sun_anchor_now[2] = cam_e[2];
    } else {
        g_sun_anchor_now[0] = 0.0f; g_sun_anchor_now[1] = 0.0f; g_sun_anchor_now[2] = 0.0f;
    }

    static std::vector<SunCaster> casters;   // Render-thread scratch (the record is at namespace
                                             // scope: KH_CAST_ALPHA).
    casters.clear();

    {
        kh_scene_sync();   // KH_SCENE: live walk by reference, no copy.
        g_sun_map_gen = g_scene.gen_cast;   // KH_SUN_GEN: the scene these maps are built from.
        kh_objbuf_sync(ctx);   // KH_OBJBUF: the records every tier's stream indexes.
        for (uint32_t khsc_i = 0; khsc_i < g_scene.objs.size(); ++khsc_i) {
            if (!g_scene.alive[khsc_i]) continue;
            const RenderObject& o = g_scene.objs[khsc_i];
            if (!kh_shadow_active(o)) continue;   // KH_SHADOW_ACTIVE.

            if (cam_valid && !kh_sun_admits(o, cam_e, khsr_rad)) continue;   // KH_SKIN_CULL: the shared admission.
            casters.push_back(kh_sun_caster_of(o, cam_e));   // KH_SHADOW_LOD_DRAWN.
        }
    }

    // KH_SHADOW_SWITCH: no caster, no map. A lit receiver with castShadow off is not a caster, so receivers can
    // outlive every caster; left claimed, the maps would keep the last caster's shadow on them. Every claim goes (the
    // next frame with a caster renders afresh).
    if (casters.empty()) { kh_sun_ladder_forget(); return false; }

    // The hash commits only at render success below: a failed render never lets a stale map vouch for new inputs.
    uint64_t input_hash = CryptoGenerator::FNV1A64_OFFSET;
    uint64_t khsh_caster_key = 0;   // KH_SUN_TIER_KEY: the caster hash (+ KH_TEX_LAND), set at the union hash.
    bool     khsh_did[4] = { false, false, false, false };   // KH_SUN_TIER_KEY: tiers that rendered this call.
    auto khsh_render_cams = [&]() -> void {
        if (!cam_valid || casters.empty()) {
            g_sun2_map_valid = false;
            g_sun3_map_valid = false;
            g_sun4_map_valid = false;
            g_sun5_map_valid = false;
            return;
        }

        ID3D11Device* khsh_dev = nullptr;
        ctx->GetDevice(&khsh_dev);
        if (!khsh_dev) {
            g_sun2_map_valid = false;
            g_sun3_map_valid = false;
            g_sun4_map_valid = false;
            g_sun5_map_valid = false;
            return;
        }
        // Per-band ensure: a band whose resources fail is stood down alone (meta zeroed, both consumers fall through
        // to the next tier). KH_SUN_OBJ: no instancing requirement - a band draws per object without it
        // (kh_sun_draw_obj_t).
        const bool khsh_core = g_res.vs && g_res.input_layout;
        const bool khsh_ok0 = khsh_core && ensure_sun_depth2(khsh_dev);
        const bool khsh_ok1 = khsh_core && ensure_sun_depth3(khsh_dev);
        const bool khsh_ok2 = khsh_core && ensure_sun_depth4(khsh_dev);
        const bool khsh_ok3 = khsh_core && ensure_sun_depth5(khsh_dev);
        // The moment pyramids + convert pair. Non-fatal: a band whose pyramid is missing or short stays classic
        // (kh_sun_pf_convert commits a whole chain only - KH_PF_WHOLE), and the next ensure makes what is missing.
        ensure_sun_pf(khsh_dev);
        khsh_dev->Release();
        if (!khsh_ok0) g_sun2_map_valid = false;
        if (!khsh_ok1) g_sun3_map_valid = false;
        if (!khsh_ok2) g_sun4_map_valid = false;
        if (!khsh_ok3) g_sun5_map_valid = false;
        if (!khsh_ok0 && !khsh_ok1 && !khsh_ok2 && !khsh_ok3) return;

        // Sun basis, once for all bands (they share the sun; only the footprint differs).
        const float khsh_f[3] = { -sun[0], -sun[1], -sun[2] };
        float khsh_up[3] = { 0.0f, 1.0f, 0.0f };
        if (fabsf(khsh_f[1]) > 0.99f) { khsh_up[0] = 1.0f; khsh_up[1] = 0.0f; }
        float khsh_r3[3] = {
            khsh_up[1] * khsh_f[2] - khsh_up[2] * khsh_f[1],
            khsh_up[2] * khsh_f[0] - khsh_up[0] * khsh_f[2],
            khsh_up[0] * khsh_f[1] - khsh_up[1] * khsh_f[0],
        };
        const float khsh_rl = sqrtf(khsh_r3[0] * khsh_r3[0] +
                                    khsh_r3[1] * khsh_r3[1] +
                                    khsh_r3[2] * khsh_r3[2]);
        if (khsh_rl < 1e-6f) {
            g_sun2_map_valid = false;
            g_sun3_map_valid = false;
            g_sun4_map_valid = false;
            g_sun5_map_valid = false;
            return;
        }
        khsh_r3[0] /= khsh_rl; khsh_r3[1] /= khsh_rl; khsh_r3[2] /= khsh_rl;
        const float khsh_u3[3] = {
            khsh_f[1] * khsh_r3[2] - khsh_f[2] * khsh_r3[1],
            khsh_f[2] * khsh_r3[0] - khsh_f[0] * khsh_r3[2],
            khsh_f[0] * khsh_r3[1] - khsh_f[1] * khsh_r3[0],
        };

        // State envelope: its own on every call (the union's call from inside its own bracket, which this nests in;
        // the hash-skip call alone) - one envelope for all bands.
        StateBackup khsh_bk;
        KhOmSave khsh_om;
        UINT khsh_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_VIEWPORT khsh_ovp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        const bool khsh_pinj = g_ro.in_injection;

        ID3D11Buffer* khsh_ovb1 = nullptr;
        UINT khsh_ovb1_stride = 0, khsh_ovb1_offset = 0;

        {
            g_ro.in_injection = true;
            khsh_bk.capture(ctx);
            // VB slot 1 is saved explicitly, as the union pass saves it (StateBackup covers slots 0 and 1 as well),
            // so our mesh + instance buffers never leak into the engine's next draws on a skip frame.
            ctx->IAGetVertexBuffers(1, 1, &khsh_ovb1, &khsh_ovb1_stride, &khsh_ovb1_offset);
            khsh_om.capture(ctx);   // Full OM set (8 RTVs + DSV).
            ctx->RSGetViewports(&khsh_nvp, khsh_ovp);
            ctx->IASetInputLayout(g_res.layout_sundepth);
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            kh_sun_bind_opaque(ctx);
            ctx->GSSetShader(nullptr, nullptr, 0);
            ctx->HSSetShader(nullptr, nullptr, 0);
            ctx->DSSetShader(nullptr, nullptr, 0);
            ID3D11Buffer* khsh_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
            ctx->VSSetConstantBuffers(0, 2, khsh_cbs);
            // KH_SUN_PS_CB: the PS stage too. Twin: the base map below.
            ctx->PSSetConstantBuffers(0, 2, khsh_cbs);
            ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
            // Twin of the union site through kh_sun_rs_pick: the tiers bind the clamp twin (a null twin falls through).
            {
                ID3D11RasterizerState* khsh_rs = kh_sun_rs_pick(true);
                ctx->RSSetState(khsh_rs);
            }
        }

        KhBindScope khsh_bc(ctx);   // KH_BIND_FILTER: the tiers' alpha casters (nested in the union's, or alone).

        // The hero body, one band per call; outputs land in the band's own statics through the reference parameters.
        auto khsh_one = [&](float khsh_half, float khsh_dep, UINT khsh_size,
                            ID3D11DepthStencilView* khsh_dsv,
                            float (&khsh_out_vp)[4][4], float& khsh_out_bias,
                            float& khsh_out_cbias,   // KH_CAST_BIAS_CAP: the cast chain's own.
                            float& khsh_out_hd, bool& khsh_out_valid,
                            uint64_t& khsh_out_renders,
                            int khsh_pf_idx,
                            bool& khsh_out_pf) -> void {
            double khsh_dctr[3] = { 0.0, 0.0, 0.0 };
            bool   khsh_dsnap = false;

            // The window moves in whole texels, so a moving camera never makes stored depth crawl under the compare.
            const float khsh_r2 = khsh_half * 1.02f;
            const float khsh_tex = (2.0f * khsh_r2) / static_cast<float>(khsh_size);
            float khsh_ctr[3] = { cam_e[0], cam_e[1], cam_e[2] };
            const double khsh_dr3[3] = { khsh_r3[0], khsh_r3[1], khsh_r3[2] };
            const double khsh_du3[3] = { khsh_u3[0], khsh_u3[1], khsh_u3[2] };
            const double khsh_dc[3] = { khsh_ctr[0], khsh_ctr[1], khsh_ctr[2] };
            const double khsh_dtex = static_cast<double>(khsh_tex);
            const double khsh_dpr = khsh_dc[0] * khsh_dr3[0] + khsh_dc[1] * khsh_dr3[1] + khsh_dc[2] * khsh_dr3[2];
            const double khsh_dpu = khsh_dc[0] * khsh_du3[0] + khsh_dc[1] * khsh_du3[1] + khsh_dc[2] * khsh_du3[2];
            const double khsh_dsr = floor(khsh_dpr / khsh_dtex) * khsh_dtex - khsh_dpr;
            const double khsh_dsu = floor(khsh_dpu / khsh_dtex) * khsh_dtex - khsh_dpu;
            // The snapped centre stays double into the eye and the translation row - collapsing to float here
            // re-quantises at the very ulp this block exists to beat.
            khsh_dctr[0] = khsh_dc[0] + khsh_dsr * khsh_dr3[0] + khsh_dsu * khsh_du3[0];
            khsh_dctr[1] = khsh_dc[1] + khsh_dsr * khsh_dr3[1] + khsh_dsu * khsh_du3[1];
            khsh_dctr[2] = khsh_dc[2] + khsh_dsr * khsh_dr3[2] + khsh_dsu * khsh_du3[2];
            khsh_dsnap = true;
            khsh_ctr[0] = static_cast<float>(khsh_dctr[0]);
            khsh_ctr[1] = static_cast<float>(khsh_dctr[1]);
            khsh_ctr[2] = static_cast<float>(khsh_dctr[2]);

            // KH_SUN_TIER_KEY: everything below is a function of these (the caster key carries KH_TEX_LAND for the
            // material maps' readiness). KH_TIER_KEY_HELD: taken twice - here for the skip test, and again past the
            // reach latch below, whose value it hashes, so the key a render stores is the one the next call with
            // these inputs takes (the latch as this call leaves it); stored as found, that call missed and drew the
            // same map once more.
            auto khtk_make = [&]() -> uint64_t {
                uint64_t khtk_k = CryptoGenerator::FNV1A64_OFFSET;
                auto khtk_f = [&khtk_k](const void* p, size_t n) {
                    khtk_k = CryptoGenerator::fnv1a64_update(khtk_k, p, n);
                };
                khtk_f(&khsh_caster_key, sizeof(khsh_caster_key));
                khtk_f(sun, 12);
                khtk_f(khsh_dctr, sizeof(khsh_dctr));
                khtk_f(&khsh_half, sizeof(khsh_half));
                khtk_f(&khsh_dep, sizeof(khsh_dep));
                khtk_f(&khsh_size, sizeof(khsh_size));
                khtk_f(g_sun_anchor_now, sizeof(g_sun_anchor_now));
                khtk_f(&g_sun_fwd_held[khsh_pf_idx], sizeof(g_sun_fwd_held[khsh_pf_idx]));
                khtk_f(&g_res.sun_instance_cap, sizeof(g_res.sun_instance_cap));
                if (khtk_k == 0) khtk_k = 1;
                return khtk_k;
            };
            uint64_t khtk = khtk_make();

            if (khsh_out_valid &&
                g_sun_tier_key_ok[khsh_pf_idx] && g_sun_tier_key[khsh_pf_idx] == khtk) {
                return;   // The standing map is this call's map.
            }

            g_sun_tier_key_ok[khsh_pf_idx] = false;   // Proven again by a completed render.
            khsh_out_valid = false;   // Proven true only by a completed render.
            khsh_out_pf = false;   // Proven true only by a completed convert.

            static std::vector<SunCaster> khsh_set;
            khsh_set.clear();
            float khsh_fwd = khsh_dep;

            for (const auto& c : casters) {
                const float khsh_dx[3] = { c.pos[0] - khsh_ctr[0],
                                           c.pos[2] - khsh_ctr[1],
                                           c.pos[1] - khsh_ctr[2] };
                const float khsh_hl[3] = { c.bsize[0] * 0.5f, c.bsize[2] * 0.5f, c.bsize[1] * 0.5f };   // KH_CAST_REFIT.
                float khsh_he[3];
                kh_rot_half_extents(khsh_hl, c.rot, c.rotated, khsh_he);
                const float khsh_er = sqrtf(khsh_he[0] * khsh_he[0] +
                                            khsh_he[1] * khsh_he[1] +
                                            khsh_he[2] * khsh_he[2]);

                // Sun-axis split: khsh_ds is how far up-sun the caster sits (sun[] points toward the sun), khsh_lat
                // the offset the ortho clips on (per-axis max).
                const float khsh_ds = khsh_dx[0] * sun[0] + khsh_dx[1] * sun[1] +
                                      khsh_dx[2] * sun[2];
                // Band admission norm: the footprint is a square in the sun plane (+-r2 along r3 and u3), so the
                // lateral clip uses the per-axis max norm - a radial test misses casters in the corners whose
                // silhouettes land inside the window. er over-bounds the per-axis extent conservatively.
                const float khsh_lu = fabsf(khsh_dx[0] * khsh_r3[0] +
                                            khsh_dx[1] * khsh_r3[1] +
                                            khsh_dx[2] * khsh_r3[2]);
                const float khsh_lv = fabsf(khsh_dx[0] * khsh_u3[0] +
                                            khsh_dx[1] * khsh_u3[1] +
                                            khsh_dx[2] * khsh_u3[2]);
                const float khsh_lat = fmaxf(khsh_lu, khsh_lv);
                if (khsh_lat > khsh_r2 + khsh_er) continue;   // Ortho clips it laterally.
                if (khsh_ds > khsr_rad + khsh_er) {
                    float khsh_slim = khsr_rad + khsh_er;
                    if (g_sun_valid) {   // The reach rescue needs a published sun (twin).
                        const float khsh_ssx = g_sun_dir_engine[0];
                        const float khsh_ssy = g_sun_dir_engine[1];
                        const float khsh_ssz = g_sun_dir_engine[2];
                        const float khsh_shl = sqrtf(khsh_ssx * khsh_ssx +
                                                     khsh_ssz * khsh_ssz);
                        if (khsh_shl > 1.0e-4f && khsh_ssy > 1.0e-4f) {
                            float khsh_sdrop = khsh_dx[1] - khsh_he[1];
                            if (khsh_sdrop < 0.0f) khsh_sdrop = 0.0f;
                            // + khsr_rad inside the drop: the throw must reach the RECEIVER, i.e. the ground up to
                            // the shadow view distance below the camera; without it the bound is short by (camera
                            // AGL) / sin(elevation) and the caster is dropped while its shadow lands inside the
                            // window. Twin of the eligibility span above.
                            const float khsh_slen = khsr_rad + khsh_er
                                                  + (khsh_sdrop + khsr_rad
                                                     + 2.0f * khsh_er) / khsh_ssy;
                            if (khsh_slen > khsh_slim) khsh_slim = khsh_slen;
                        }
                    }
                    if (khsh_ds > khsh_slim) continue;
                }
                if (khsh_ds < -(khsh_dep) - khsh_er) continue;
                khsh_set.push_back(c);
                const float khsh_need = khsh_ds + khsh_er;
                if (khsh_need > khsh_fwd) khsh_fwd = khsh_need;
            }

            // Entered only above the floor, so a band whose window was never stretched is bit-exact. khsh_fwd is a
            // maximum over every admitted caster and sizes khsh_d2v: this is how much one member costs all the
            // others in depth precision.
            if (khsh_fwd > khsh_dep) {
                float& khsh_hold = g_sun_fwd_held[khsh_pf_idx];

                if (khsh_hold >= khsh_fwd && khsh_hold <= khsh_fwd * KH_BAND_FWD_DROP) {
                    khsh_fwd = khsh_hold;
                } else {
                    khsh_hold = khsh_fwd * KH_BAND_FWD_SLACK;
                    khsh_fwd = khsh_hold;
                }
            } else {
                g_sun_fwd_held[khsh_pf_idx] = -1.0f;   // Never leave a stale latch.
            }
            khtk = khtk_make();   // KH_TIER_KEY_HELD: the latch as this call leaves it.

            const float khsh_rdep = khsh_fwd;
            const float khsh_d2v = khsh_fwd + khsh_dep + 2.0f;
            const float khsh_eye[3] = {
                khsh_ctr[0] + sun[0] * (khsh_rdep + 1.0f),
                khsh_ctr[1] + sun[1] * (khsh_rdep + 1.0f),
                khsh_ctr[2] + sun[2] * (khsh_rdep + 1.0f),
            };
            const double khsh_deye[3] = {
                (khsh_dsnap ? khsh_dctr[0] : static_cast<double>(khsh_ctr[0]))
                    + static_cast<double>(sun[0]) * (static_cast<double>(khsh_rdep) + 1.0),
                (khsh_dsnap ? khsh_dctr[1] : static_cast<double>(khsh_ctr[1]))
                    + static_cast<double>(sun[1]) * (static_cast<double>(khsh_rdep) + 1.0),
                (khsh_dsnap ? khsh_dctr[2] : static_cast<double>(khsh_ctr[2]))
                    + static_cast<double>(sun[2]) * (static_cast<double>(khsh_rdep) + 1.0),
            };
            float khsh_view[4][4] = {};
            khsh_view[0][0] = khsh_r3[0]; khsh_view[0][1] = khsh_u3[0]; khsh_view[0][2] = khsh_f[0];
            khsh_view[1][0] = khsh_r3[1]; khsh_view[1][1] = khsh_u3[1]; khsh_view[1][2] = khsh_f[1];
            khsh_view[2][0] = khsh_r3[2]; khsh_view[2][1] = khsh_u3[2]; khsh_view[2][2] = khsh_f[2];
            const double khsh_da[3] = {
                (khsh_dsnap ? khsh_deye[0] : static_cast<double>(khsh_eye[0]))
                    - static_cast<double>(g_sun_anchor_now[0]),
                (khsh_dsnap ? khsh_deye[1] : static_cast<double>(khsh_eye[1]))
                    - static_cast<double>(g_sun_anchor_now[1]),
                (khsh_dsnap ? khsh_deye[2] : static_cast<double>(khsh_eye[2]))
                    - static_cast<double>(g_sun_anchor_now[2]),
            };
            khsh_view[3][0] = static_cast<float>(-(khsh_da[0] * khsh_r3[0] + khsh_da[1] * khsh_r3[1] + khsh_da[2] * khsh_r3[2]));
            khsh_view[3][1] = static_cast<float>(-(khsh_da[0] * khsh_u3[0] + khsh_da[1] * khsh_u3[1] + khsh_da[2] * khsh_u3[2]));
            khsh_view[3][2] = static_cast<float>(-(khsh_da[0] * khsh_f[0] + khsh_da[1] * khsh_f[1] + khsh_da[2] * khsh_f[2]));
            khsh_view[3][3] = 1.0f;
            float khsh_proj[4][4] = {};
            khsh_proj[0][0] = 1.0f / khsh_r2;
            khsh_proj[1][1] = 1.0f / khsh_r2;
            khsh_proj[2][2] = 1.0f / khsh_d2v;
            khsh_proj[3][3] = 1.0f;
            float khsh_lvp[4][4];
            mul_4x4(khsh_view, khsh_proj, khsh_lvp);

            const float khsh_bcx = fabsf(khsh_ctr[0]) > fabsf(khsh_ctr[1]) ? fabsf(khsh_ctr[0]) : fabsf(khsh_ctr[1]);
            const float khsh_bcm = khsh_bcx > fabsf(khsh_ctr[2]) ? khsh_bcx : fabsf(khsh_ctr[2]);
            const float khsh_bulp = nextafterf(khsh_bcm, 3.0e38f) - khsh_bcm;
            const float khsh_bfl = 4.0f * khsh_bulp;
            // The texel term is a multiple of the tier's own texel; the ulp term is a floor under it.
            const float khsh_bk = KH_BAND_BIAS_TEXELS;
            const float khsh_bbw = khsh_tex * khsh_bk > khsh_bfl ? khsh_tex * khsh_bk : khsh_bfl;
            // The same quantity, capped in metres for the cast chain only.
            const float khsh_ccap = KH_CAST_BIAS_CAP_M;
            const float khsh_ctw = khsh_tex * khsh_bk < khsh_ccap ? khsh_tex * khsh_bk : khsh_ccap;
            const float khsh_cbw = khsh_ctw > khsh_bfl ? khsh_ctw : khsh_bfl;

            // Membership is the ortho's own lateral clip, never a positional subset of it.
            if (khsh_set.empty()) return;   // Valid stays false.
            // KH_SUN_OBJ: instanced while the union's stream holds the band's casters; per object otherwise (no
            // instancing resources, a set past the stream, a failed map).
            const bool khsh_inst = kh_sun_inst_on() && khsh_set.size() <= static_cast<size_t>(g_res.sun_instance_cap);

            ConstantData khsh_obj = {};
            ConstantData khsh_cbf = {};
            memcpy(khsh_cbf.view_proj, khsh_lvp, sizeof(khsh_cbf.view_proj));
            // The matrix above is anchor-relative; VSSunDepth subtracts the same anchor from every vertex.
            khsh_cbf.sun_origin[0] = g_sun_anchor_now[0];
            khsh_cbf.sun_origin[1] = g_sun_anchor_now[1];
            khsh_cbf.sun_origin[2] = g_sun_anchor_now[2];

            if (kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khsh_cbf) &&
                kh_upload_obj_cb(ctx, g_res.composite_cb, khsh_obj)) {
                ctx->ClearDepthStencilView(khsh_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
                ctx->OMSetRenderTargets(0, nullptr, khsh_dsv);
                {
                    D3D11_VIEWPORT khsh_vp = {};
                    khsh_vp.Width = static_cast<FLOAT>(khsh_size);
                    khsh_vp.Height = static_cast<FLOAT>(khsh_size);
                    khsh_vp.MaxDepth = 1.0f;
                    ctx->RSSetViewports(1, &khsh_vp);
                }

                if (g_skin_pool_any) {   // KH_SKIN_POOL: a chunk's casters together, for its runs.
                    for (const SunCaster& khsh_kc : khsh_set) kh_cast_key_put(khsh_kc.mesh, khsh_kc.slot);
                    std::sort(khsh_set.begin(), khsh_set.end(),
                              [](const SunCaster& a, const SunCaster& b) {
                                  if (a.mesh != b.mesh) return a.mesh < b.mesh;
                                  if (a.lod != b.lod) return a.lod < b.lod;   // KH_SHADOW_LOD_DRAWN.
                                  return kh_cast_key_at(a.slot) < kh_cast_key_at(b.slot);
                              });
                } else {
                    std::sort(khsh_set.begin(), khsh_set.end(),
                              [](const SunCaster& a, const SunCaster& b) {
                                  return a.mesh != b.mesh ? a.mesh < b.mesh : a.lod < b.lod;   // KH_SHADOW_LOD_DRAWN.
                              });
                }
                D3D11_MAPPED_SUBRESOURCE khsh_im = {};
                bool khsh_drawn = false;

                if (khsh_inst && SUCCEEDED(ctx->Map(g_res.sun_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khsh_im))) {
                    ctx->IASetInputLayout(g_res.layout_sundepth);   // KH_SUN_OBJ: a band before may have drawn per
                    kh_sun_bind_opaque(ctx);                         // object.
                    KhInstLane* khsh_outp = static_cast<KhInstLane*>(khsh_im.pData);   // KH_OBJBUF: slot + alpha.

                    for (const auto& c : khsh_set) {
                        khsh_outp->slot = c.slot;
                        khsh_outp->dither = 0.0f;
                        khsh_outp->alpha = c.alpha;   // KH_CAST_ALPHA (unread by VSSunDepth).
                        khsh_outp->mat_base = 0.0f;
                        ++khsh_outp;
                    }

                    ctx->Unmap(g_res.sun_instance_vb, 0);
                    kh_objbuf_bind(ctx);   // KH_OBJBUF: the tier's VS reads the records.

                    ID3D11Buffer* khsh_vbs[2] = { nullptr, g_res.sun_instance_vb };
                    UINT khsh_strides[2] = { sizeof(MeshVertex), KH_INST_STRIDE };
                    UINT khsh_offsets[2] = { 0, 0 };
                    size_t khsh_first = 0;
                    KhDepthSk khsh_sk = { g_res.vs_sundepth, g_res.layout_sundepth, g_res.vs_sundepth_sk, nullptr,
                                          nullptr };

                    while (khsh_first < khsh_set.size()) {
                        if (khsh_set[khsh_first].alpha_caster) { ++khsh_first; continue; }
                        const int khsh_mid = khsh_set[khsh_first].mesh;
                        const int khsh_lod = khsh_set[khsh_first].lod;   // KH_SHADOW_LOD_DRAWN.
                        size_t khsh_last = khsh_first + 1;
                        const KhVbRef khsh_vb = kh_mesh_vb_ref(khsh_mid, khsh_set[khsh_first].slot);   // KH_CLOTH.
                        const KhSkinPoolChunk* const khsh_pc =   // KH_SKIN_POOL.
                            kh_cast_pool_of(khsh_mid, khsh_set[khsh_first].slot, g_res.vs_sundepth_sk);
                        while (khsh_last < khsh_set.size() && khsh_set[khsh_last].mesh == khsh_mid &&
                               khsh_set[khsh_last].lod == khsh_lod &&
                               kh_cast_run_joins(khsh_vb, khsh_pc, khsh_set[khsh_last].mesh, khsh_set[khsh_last].slot,
                                                 g_res.vs_sundepth_sk) &&
                               !khsh_set[khsh_last].alpha_caster) ++khsh_last;
                        khsh_sk.use(ctx, khsh_pc);
                        khsh_vbs[0] = khsh_vb.vb;
                        khsh_offsets[0] = khsh_vb.off;   // KH_SKIN_POOL.
                        ctx->IASetVertexBuffers(0, 2, khsh_vbs, khsh_strides, khsh_offsets);
                        ctx->IASetIndexBuffer(g_res.mesh_ib[khsh_mid], DXGI_FORMAT_R32_UINT, 0);
                        UINT khsh_ls = 0, khsh_lc = 0;
                        mesh_lod_range_of(khsh_mid, khsh_lod, khsh_ls, khsh_lc);   // KH_SHADOW_LOD_DRAWN.
                        ctx->DrawIndexedInstanced(khsh_lc,
                                                  static_cast<UINT>(khsh_last - khsh_first), khsh_ls, 0,
                                                  static_cast<UINT>(khsh_first));
                        khsh_first = khsh_last;
                    }
                    khsh_sk.end(ctx);   // KH_SKIN_POOL: the tier's own pair again.

                    // KH_CAST_ALPHA: the alpha casters, this tier - a failed upload leaves the map incomplete.
                    khsh_drawn = kh_sun_draw_alpha(ctx, khsh_set, khsh_obj, g_res.sun_instance_vb);
                } else {
                    khsh_drawn = kh_sun_draw_obj_t(ctx, khsh_set, g_sun_anchor_now);   // KH_SUN_OBJ.
                }
                if (khsh_drawn) {
                    memcpy(khsh_out_vp, khsh_lvp, sizeof(khsh_out_vp));
                    khsh_out_bias = khsh_bbw / khsh_d2v;
                    khsh_out_cbias = khsh_cbw / khsh_d2v;
                    khsh_out_hd = khsh_half;
                    khsh_out_valid = true;
                    khsh_out_renders++;
                    // The band renders here; freshness is all this site marks (the pyramid convert happens where the
                    // maps are bound: kh_sun_pf_convert at the injection's and the flush's proven binds, and in the
                    // PIP and the in-front pass).
                    khsh_out_pf = khsh_out_valid;   // Fresh map => convert due.
                    g_sun_pf_fresh[khsh_pf_idx] = true;   // KH_SUN_TIER_KEY.
                    g_sun_tier_key[khsh_pf_idx] = khtk;
                    g_sun_tier_key_ok[khsh_pf_idx] = true;
                    khsh_did[khsh_pf_idx] = true;
                    {   // KH_PCSS_CELL: the window centre sits on whole texels of the world lattice (khsh_dpr /
                        // khsh_dpu over the texel, floored), so a texel index t of this map is lattice index
                        // t - size / 2 + the centre's along r3, and t - size / 2 - the centre's along u3 (the map's v
                        // runs down u3). The size is even (a power of two).
                        const double khsh_lr = floor(khsh_dpr / khsh_dtex);
                        const double khsh_lu = floor(khsh_dpu / khsh_dtex);
                        const double khsh_lh = static_cast<double>(khsh_size / 2u);
                        uint32_t* const khsh_lo = g_sun_tier_lat[khsh_pf_idx];
                        // A window off every sane range (never, with a finite camera) takes offset 0: its noise
                        // follows the window.
                        const bool khsh_lfin = fabs(khsh_lr) < 1.0e15 && fabs(khsh_lu) < 1.0e15;
                        khsh_lo[0] = khsh_lfin ? static_cast<uint32_t>(static_cast<int64_t>(khsh_lr - khsh_lh)) : 0u;
                        khsh_lo[1] = khsh_lfin ? static_cast<uint32_t>(static_cast<int64_t>(-khsh_lu - khsh_lh)) : 0u;
                    }
                }
            }
        };

        float khls_cr = 0.0f;

        for (const auto& khls_c : casters) {
            const float khls_hl[3] = { khls_c.bsize[0] * 0.5f, khls_c.bsize[2] * 0.5f,
                                       khls_c.bsize[1] * 0.5f };   // KH_CAST_REFIT.
            float khls_he[3];
            kh_rot_half_extents(khls_hl, khls_c.rot, khls_c.rotated, khls_he);
            const float khls_r = sqrtf(khls_he[0] * khls_he[0] + khls_he[1] * khls_he[1] +
                                       khls_he[2] * khls_he[2]);
            if (khls_r > khls_cr) khls_cr = khls_r;
        }

        float khls_s = khsr_rad / 200.0f;
        if (khls_s < 1.0f) khls_s = 1.0f;
        if (khls_s > 5.0f) khls_s = 5.0f;
        // The ensures above guarantee each texture is the wanted size; record it as the rendered size for the CB fills.
        if (khsh_ok0) g_sun2_map_size = kh_sun_hero_size();
        if (khsh_ok1) g_sun3_map_size = kh_sun_mid_size();
        if (khsh_ok2) g_sun4_map_size = kh_sun_out_size();
        if (khsh_ok3) g_sun5_map_size = kh_sun_far_size();
        if (khsh_ok0) khsh_one(KH_SUN_HERO_HALF, KH_SUN_HERO_DEPTH, kh_sun_hero_size(),
                               g_res.sun2_dsv, g_sun2_map_vp, g_sun2_map_bias,
                               g_sun2_cast_bias,
                               g_sun2_half_diag, g_sun2_map_valid,
                               g_sun2_renders,
                               0,
                               g_sun_pf_valid[0]);
        if (khsh_ok1) khsh_one(KH_SUN_MID_HALF * khls_s, KH_SUN_MID_DEPTH * khls_s,
                               kh_sun_mid_size(),
                               g_res.sun3_dsv, g_sun3_map_vp, g_sun3_map_bias,
                               g_sun3_cast_bias,
                               g_sun3_half_diag, g_sun3_map_valid,
                               g_sun3_renders,
                               1,
                               g_sun_pf_valid[1]);
        if (khsh_ok2) khsh_one(KH_SUN_OUT_HALF * khls_s, KH_SUN_OUT_DEPTH * khls_s,
                               kh_sun_out_size(),
                               g_res.sun4_dsv, g_sun4_map_vp, g_sun4_map_bias,
                               g_sun4_cast_bias,
                               g_sun4_half_diag, g_sun4_map_valid,
                               g_sun4_renders,
                               2,
                               g_sun_pf_valid[2]);
        // The far tier converts a moment pyramid like the camera tiers (sparse taps shimmer under a moving sun).
        // Domain: half = range * (sin e + cos e), depth = 6x range - the fade's cylinder projects to that max-norm in
        // the sun plane; plain 'range' under-covers in-range receivers. The far tier answers its whole window (cb.hlsl:
        // KhCastTier/KhSelfTier 'last'). The factor is quantized to 1/8 steps: read live it resizes the window and its
        // snap grid every frame the sun moves.
        const float khsh_far_f = fabsf(sun[1]) + sqrtf(fmaxf(1.0f - sun[1] * sun[1], 0.0f));
        const float khsh_far_half = khsr_rad * (ceilf(khsh_far_f * 8.0f) * 0.125f);
        if (khsh_ok3) khsh_one(khsh_far_half, khsr_rad * 6.0f, kh_sun_far_size(),
                               g_res.sun5_dsv, g_sun5_map_vp, g_sun5_map_bias,
                               g_sun5_cast_bias,
                               g_sun5_half_diag, g_sun5_map_valid,
                               g_sun5_renders,
                               3,
                               g_sun_pf_valid[3]);
        // The anchor a tier's stored vp is relative to is the one it RENDERED
        // with; a skipped tier keeps its stamp (the fill rebases).
        if (khsh_ok0 && khsh_did[0]) memcpy(g_sun_tier_anchor[0], g_sun_anchor_now, sizeof(g_sun_anchor_now));
        if (khsh_ok1 && khsh_did[1]) memcpy(g_sun_tier_anchor[1], g_sun_anchor_now, sizeof(g_sun_anchor_now));
        if (khsh_ok2 && khsh_did[2]) memcpy(g_sun_tier_anchor[2], g_sun_anchor_now, sizeof(g_sun_anchor_now));
        if (khsh_ok3 && khsh_did[3]) memcpy(g_sun_tier_anchor[3], g_sun_anchor_now, sizeof(g_sun_anchor_now));

        {
            khsh_om.restore(ctx);
            if (khsh_nvp > 0) ctx->RSSetViewports(khsh_nvp, khsh_ovp);
            ctx->IASetVertexBuffers(1, 1, &khsh_ovb1, &khsh_ovb1_stride, &khsh_ovb1_offset);
            KH_SAFE_RELEASE(khsh_ovb1);
            khsh_bk.restore(ctx);
            khsh_om.release();
            g_ro.in_injection = khsh_pinj;
        }
    };

    uint64_t khsm_caster_hash = 0;

    {
        auto fnv = [&input_hash](const void* p, size_t n) {
            input_hash = CryptoGenerator::fnv1a64_update(input_hash, p, n);
        };
        bool khsm_tex_alpha = false;   // KH_TEX_LAND: a caster draws through its material's maps.
        bool khsm_uv_anim = false;   // KH_UV_ANIM: an alpha caster's coverage moves with the clock.

        for (const auto& c : casters) {
            khsm_tex_alpha |= c.alpha_caster && c.mats;
            khsm_uv_anim |= c.alpha_caster && c.mats && c.mats->uv_anim_alpha;
            fnv(c.pos, sizeof(c.pos));
            fnv(c.res, sizeof(c.res));   // KH_POS_RES: a centre that moved finer than pos re-renders too.
            fnv(c.size, sizeof(c.size));
            fnv(c.rot, sizeof(c.rot));   // Rotating a caster re-renders.
            fnv(&c.mesh, sizeof(c.mesh));
            fnv(&c.alpha, sizeof(c.alpha));   // KH_CAST_ALPHA: a fade re-renders (1/64 steps).
            fnv(&c.lod, sizeof(c.lod));   // KH_SHADOW_LOD_DRAWN: a level change re-renders.
            const uint64_t khsc_mh = c.mats ? c.mats->hash : 0ull;
            fnv(&khsc_mh, sizeof(khsc_mh));   // A material change re-renders.
            const uint8_t khsc_ac = c.alpha_caster ? 1 : 0;
            fnv(&khsc_ac, 1);
            // KH_CLOTH: a cloth caster's geometry moves while its transform holds still, so its shape key changes (a
            // skeletal mesh's too, from its consume-point step).
            const uint32_t khsc_cg = kh_cloth_gen_of(c.slot);
            fnv(&khsc_cg, sizeof(khsc_cg));
        }

        khsm_caster_hash = input_hash;
        // KH_TEX_LAND: into the union and tier keys, not the maturity hash (a landing is no caster change).
        if (khsm_tex_alpha) fnv(&g_tex_land_serial, sizeof(g_tex_land_serial));
        if (khsm_uv_anim) {   // KH_UV_ANIM: likewise - a new key each cycle (kh_mat_anim_cycle_key).
            const uint64_t khsm_ac = kh_mat_anim_cycle_key();
            fnv(&khsm_ac, sizeof(khsm_ac));
        }
        khsh_caster_key = input_hash;   // KH_SUN_TIER_KEY.
        fnv(sun, 12);
        if (cam_valid) {
            const int32_t khsm_cell[3] = {
                static_cast<int32_t>(floorf(cam_e[0] * 0.5f)),
                static_cast<int32_t>(floorf(cam_e[1] * 0.5f)),
                static_cast<int32_t>(floorf(cam_e[2] * 0.5f)),
            };
            fnv(khsm_cell, sizeof(khsm_cell));
            // KH_SUN_RNG_HASH: the clamped range every consumer reads (khsr_rad) - the raw one is unbounded above
            // (the video options' reading), and its float -> int conversion past 2^31 undefined.
            const int32_t khsm_rng = static_cast<int32_t>(khsr_rad);
            fnv(&khsm_rng, sizeof(khsm_rng));
        }

        // No ensure_resources on this path: the hash commits only past the res_ok return below, after a render through
        // a successful ensure. So a release that empties g_res.mesh_vb must zero g_sun_map_hash with it -
        // Resources::release does (kh_sun_ladder_forget), which makes this return safe from mask_cast_engine, the one
        // caller with no ensure_resources ahead of it. Do not pair the clear with release_shadow_device_state instead:
        // ensure_resources releases on its own failure paths.
        if (g_sun_map_valid && input_hash == g_sun_map_hash) {
            g_sun_map_time = effect_time_seconds();
            khsh_render_cams();   // Camera bands render every call.
            return true;
        }
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) return false;
    const bool res_ok = kh_ensure_ok("sun shadow: sun-depth resources", ensure_resources(dev)) && ensure_sun_depth(dev);
    bool instanced = res_ok && g_res.vs_sundepth && g_res.layout_sundepth && g_res.obj_srv;   // KH_OBJBUF.

    // KH_SUN_INST_CAP: one 16-byte KhInstLane per caster (centre, extents and rotation live in the record buffer the
    // lane's slot indexes); no stream, the per-object route (KH_SUN_OBJ).
    if (instanced && !kh_sun_inst_ensure(dev, casters.size())) instanced = false;

    dev->Release();
    if (!res_ok) {
        // KH_SUN_OBJ: no map fit to read is left - a failed re-create has released the union (kh_sun_map_ensure frees
        // before it allocates), and a failed ensure_resources leaves it drawn for inputs this call no longer has (the
        // hash above did not match). KH_SUN_LADDER_DROP: and no tier either - they render only past this return
        // (khsh_render_cams), so each would stand as last drawn, for casters and a camera this call no longer has,
        // while its own flag armed its lanes. The ladder's one authority drops them all with the hash (the tier keys
        // too, so a recovered call renders every tier fresh).
        kh_sun_ladder_forget();
        return false;
    }

    // Combined caster AABB, engine axes (SQF [x, y, zASL] -> [x, zASL, y]).
    float mn[3] = { 1e18f, 1e18f, 1e18f };
    float mx[3] = { -1e18f, -1e18f, -1e18f };

    for (const auto& c : casters) {
        const float ce[3] = { c.pos[0], c.pos[2], c.pos[1] };
        const float hl[3] = { c.bsize[0] * 0.5f, c.bsize[2] * 0.5f, c.bsize[1] * 0.5f };   // KH_CAST_REFIT.
        float he[3];
        kh_rot_half_extents(hl, c.rot, c.rotated, he);

        for (int k = 0; k < 3; ++k) {
            if (ce[k] - he[k] < mn[k]) mn[k] = ce[k] - he[k];
            if (ce[k] + he[k] > mx[k]) mx[k] = ce[k] + he[k];
        }
    }

    float ctr[3] = { (mn[0] + mx[0]) * 0.5f, (mn[1] + mx[1]) * 0.5f, (mn[2] + mx[2]) * 0.5f };
    float hf[3] = { (mx[0] - mn[0]) * 0.5f, (mx[1] - mn[1]) * 0.5f, (mx[2] - mn[2]) * 0.5f };

    g_sun_map_cam_anchor = false;   // Re-derived on every re-fit; skip frames keep the committed
                                    // value.
    if (cam_valid) {
        g_sun_map_cam_anchor = true;
        ctr[0] = floorf(cam_e[0] * 0.5f) * 2.0f + 1.0f;
        ctr[1] = floorf(cam_e[1] * 0.5f) * 2.0f + 1.0f;
        ctr[2] = floorf(cam_e[2] * 0.5f) * 2.0f + 1.0f;
        // KH_EA_SUN_RNG_ONE: the call's one clamped range (khsr_rad), never a second read of the atomic - a game-thread
        // store between the reads would fit the union to one range and admit / cull its casters by another.
        hf[0] = hf[1] = hf[2] = khsr_rad / 1.7320508f;
        constexpr float KH_SUN_GROW_CAP = 3500.0f;   // enclosing-radius ceiling (~7 km span).
        const float khsg_h = KH_SUN_GROW_CAP / 1.7320508f;

        for (int khsg_k = 0; khsg_k < 3; ++khsg_k) {
            const float khsg_cl = fmaxf(mn[khsg_k], ctr[khsg_k] - khsg_h);
            const float khsg_ch = fminf(mx[khsg_k], ctr[khsg_k] + khsg_h);
            if (khsg_cl > khsg_ch) continue;   // Capped caster box empty on this axis.
            float khsg_lo = ctr[khsg_k] - hf[khsg_k];
            float khsg_hi = ctr[khsg_k] + hf[khsg_k];
            if (khsg_cl < khsg_lo) khsg_lo = khsg_cl;
            if (khsg_ch > khsg_hi) khsg_hi = khsg_ch;
            ctr[khsg_k] = (khsg_lo + khsg_hi) * 0.5f;
            hf[khsg_k] = (khsg_hi - khsg_lo) * 0.5f;
        }

    }
    const float khsf_rad_raw = sqrtf(hf[0] * hf[0] + hf[1] * hf[1] + hf[2] * hf[2]);
    float khsf_rad_use = khsf_rad_raw;
    if (g_sun_map_cam_anchor) {
        constexpr float KH_SUN_UNION_R_SLACK = 1.10f;   // Headroom taken on acquisition.
        constexpr float KH_SUN_UNION_R_DROP  = 1.25f;   // re-latch tighter past this.
        if (g_sun_union_rad_held >= khsf_rad_raw &&
            g_sun_union_rad_held <= khsf_rad_raw * KH_SUN_UNION_R_DROP) {
            khsf_rad_use = g_sun_union_rad_held;
        } else {
            g_sun_union_rad_held = khsf_rad_raw * KH_SUN_UNION_R_SLACK;
            khsf_rad_use = g_sun_union_rad_held;
        }
    }
    const float khsf_rad = khsf_rad_use;
    const float R_dep = khsf_rad + 1.0f;
    // R is refined (only ever downward) after the sun basis exists below - it needs the sun plane to measure in.
    float R = khsf_rad > 1.0e-4f ? khsf_rad * 1.02f : R_dep;

    // Ortho sun basis (row-vector convention). The light looks opposite the "toward the light" direction.
    const float f[3] = { -sun[0], -sun[1], -sun[2] };
    float up_hint[3] = { 0.0f, 1.0f, 0.0f };
    if (fabsf(f[1]) > 0.99f) { up_hint[0] = 1.0f; up_hint[1] = 0.0f; }

    float r3[3] = {
        up_hint[1] * f[2] - up_hint[2] * f[1],
        up_hint[2] * f[0] - up_hint[0] * f[2],
        up_hint[0] * f[1] - up_hint[1] * f[0],
    };

    const float rl = sqrtf(r3[0] * r3[0] + r3[1] * r3[1] + r3[2] * r3[2]);
    if (rl < 1e-6f) return false;
    r3[0] /= rl; r3[1] /= rl; r3[2] /= rl;

    const float u3[3] = {
        f[1] * r3[2] - f[2] * r3[1],
        f[2] * r3[0] - f[0] * r3[2],
        f[0] * r3[1] - f[1] * r3[0],
    };

    if (g_sun_map_cam_anchor) {
        const float khsl_rng = khsr_rad;   // KH_EA_SUN_RNG_ONE.
        // Camera window term: its cell centre's lateral offset from the committed centre, plus the range.
        const float khsl_cc[3] = {
            floorf(cam_e[0] * 0.5f) * 2.0f + 1.0f,
            floorf(cam_e[1] * 0.5f) * 2.0f + 1.0f,
            floorf(cam_e[2] * 0.5f) * 2.0f + 1.0f,
        };
        const float khsl_cd[3] = { khsl_cc[0] - ctr[0], khsl_cc[1] - ctr[1],
                                   khsl_cc[2] - ctr[2] };
        const float khsl_cs = khsl_cd[0] * sun[0] + khsl_cd[1] * sun[1] +
                              khsl_cd[2] * sun[2];
        const float khsl_cl2 = khsl_cd[0] * khsl_cd[0] + khsl_cd[1] * khsl_cd[1] +
                               khsl_cd[2] * khsl_cd[2] - khsl_cs * khsl_cs;
        // The camera's cylinder reaches sqrt(2) * range radially in the sun plane, not 'range'.
        float khsl_need = (khsl_cl2 > 0.0f ? sqrtf(khsl_cl2) : 0.0f) + khsl_rng * 1.4142136f;

        for (const auto& khsl_c : casters) {
            const float khsl_ce[3] = { khsl_c.pos[0], khsl_c.pos[2], khsl_c.pos[1] };
            const float khsl_hl[3] = { khsl_c.bsize[0] * 0.5f, khsl_c.bsize[2] * 0.5f,
                                       khsl_c.bsize[1] * 0.5f };   // KH_CAST_REFIT.
            float khsl_he[3];
            kh_rot_half_extents(khsl_hl, khsl_c.rot, khsl_c.rotated, khsl_he);
            const float khsl_er = sqrtf(khsl_he[0] * khsl_he[0] +
                                        khsl_he[1] * khsl_he[1] +
                                        khsl_he[2] * khsl_he[2]);
            const float khsl_d[3] = { khsl_ce[0] - ctr[0], khsl_ce[1] - ctr[1],
                                      khsl_ce[2] - ctr[2] };
            const float khsl_ds = khsl_d[0] * sun[0] + khsl_d[1] * sun[1] +
                                  khsl_d[2] * sun[2];
            const float khsl_l2 = khsl_d[0] * khsl_d[0] + khsl_d[1] * khsl_d[1] +
                                  khsl_d[2] * khsl_d[2] - khsl_ds * khsl_ds;
            const float khsl_lat = (khsl_l2 > 0.0f ? sqrtf(khsl_l2) : 0.0f) + khsl_er;
            if (khsl_lat > khsl_need) khsl_need = khsl_lat;
        }

        if (khsl_need > khsf_rad) khsl_need = khsf_rad;   // Never coarser than the sphere.
        float khsl_use = khsl_need;
        constexpr float KH_SUN_UNION_L_SLACK = 1.10f;
        constexpr float KH_SUN_UNION_L_DROP  = 1.25f;
        if (g_sun_union_lat_held >= khsl_need &&
            g_sun_union_lat_held <= khsl_need * KH_SUN_UNION_L_DROP) {
            khsl_use = g_sun_union_lat_held;
        } else {
            g_sun_union_lat_held = khsl_need * KH_SUN_UNION_L_SLACK;
            khsl_use = g_sun_union_lat_held;
        }
        // Apply only when it refines - the latch slack can push the held value past the sphere in dense-lateral
        // scenes.
        if (khsl_use > 1.0e-4f && khsl_use * 1.02f < R) R = khsl_use * 1.02f;
    }

    const double khut_tex = (2.0 * static_cast<double>(R)) /
                            static_cast<double>(kh_sun_depth_size());
    // KH_UNION_DSNAP: the snapped centre stays double into the eye and the translation row, as the tiers keep
    // theirs (khsh_dctr) - collapsing it to float re-quantised the window at the ulp the snap exists to beat.
    double khut_dn[3] = { 0.0, 0.0, 0.0 };
    bool khut_dsnap = false;
    if (khut_tex > 1.0e-9) {
        const double khut_c[3] = { ctr[0], ctr[1], ctr[2] };
        const double khut_r[3] = { r3[0], r3[1], r3[2] };
        const double khut_u[3] = { u3[0], u3[1], u3[2] };
        const double khut_pr = khut_c[0] * khut_r[0] + khut_c[1] * khut_r[1] +
                               khut_c[2] * khut_r[2];
        const double khut_pu = khut_c[0] * khut_u[0] + khut_c[1] * khut_u[1] +
                               khut_c[2] * khut_u[2];
        const double khut_sr = floor(khut_pr / khut_tex) * khut_tex - khut_pr;
        const double khut_su = floor(khut_pu / khut_tex) * khut_tex - khut_pu;
        const double khut_n[3] = {
            khut_c[0] + khut_sr * khut_r[0] + khut_su * khut_u[0],
            khut_c[1] + khut_sr * khut_r[1] + khut_su * khut_u[1],
            khut_c[2] + khut_sr * khut_r[2] + khut_su * khut_u[2],
        };
        khut_dn[0] = khut_n[0];
        khut_dn[1] = khut_n[1];
        khut_dn[2] = khut_n[2];
        khut_dsnap = true;
        ctr[0] = static_cast<float>(khut_n[0]);
        ctr[1] = static_cast<float>(khut_n[1]);
        ctr[2] = static_cast<float>(khut_n[2]);
    }
    const float eye[3] = {
        ctr[0] + sun[0] * (R_dep + 1.0f),
        ctr[1] + sun[1] * (R_dep + 1.0f),
        ctr[2] + sun[2] * (R_dep + 1.0f),
    };

    const float D = 2.0f * R_dep + 2.0f;   // view-axis range: casters sit in [1, 2R_dep+1].

    float view[4][4] = {};
    view[0][0] = r3[0]; view[0][1] = u3[0]; view[0][2] = f[0];
    view[1][0] = r3[1]; view[1][1] = u3[1]; view[1][2] = f[1];
    view[2][0] = r3[2]; view[2][1] = u3[2]; view[2][2] = f[2];
    // The translation row measures from the anchor, not the world origin - the shader subtracts the same anchor from
    // wpos, so the fp products shrink from world magnitude to fit magnitude. KH_UNION_DSNAP: the eye and its offset
    // from the anchor in double from the snapped centre (the tiers' khsh_deye / khsh_da), the float eye where nothing
    // snapped; one rounding at the end.
    double khse_a[3];
    for (int khse_k = 0; khse_k < 3; ++khse_k) {
        const double khse_e = khut_dsnap
            ? khut_dn[khse_k] + static_cast<double>(sun[khse_k]) * (static_cast<double>(R_dep) + 1.0)
            : static_cast<double>(eye[khse_k]);
        khse_a[khse_k] = khse_e - static_cast<double>(g_sun_anchor_now[khse_k]);
    }
    view[3][0] = static_cast<float>(-(khse_a[0] * r3[0] + khse_a[1] * r3[1] + khse_a[2] * r3[2]));
    view[3][1] = static_cast<float>(-(khse_a[0] * u3[0] + khse_a[1] * u3[1] + khse_a[2] * u3[2]));
    view[3][2] = static_cast<float>(-(khse_a[0] * f[0] + khse_a[1] * f[1] + khse_a[2] * f[2]));
    view[3][3] = 1.0f;

    float proj[4][4] = {};
    proj[0][0] = 1.0f / R;   // Lateral extent (no depth guard in it).
    proj[1][1] = 1.0f / R;
    proj[2][2] = 1.0f / D;   // z in [0, D] -> [0, 1].
    proj[3][3] = 1.0f;

    float lvp[4][4];
    mul_4x4(view, proj, lvp);

    // Compare bias, deliberately small: the world cast cannot acne (the ground is never in our map), and the self
    // term's acne is handled geometrically by the receiver-normal offset + grazing fade.
    const float texel_world = (2.0f * R) / static_cast<float>(kh_sun_depth_size());
    // The floor is fp32 world precision, not depth-buffer precision.
    const float khsb_cx = fabsf(ctr[0]) > fabsf(ctr[1]) ? fabsf(ctr[0]) : fabsf(ctr[1]);
    const float khsb_cm = khsb_cx > fabsf(ctr[2]) ? khsb_cx : fabsf(ctr[2]);
    const float khsb_ulp = nextafterf(khsb_cm, 3.0e38f) - khsb_cm;
    const float khsb_floor = 4.0f * khsb_ulp;
    const float bias_world = texel_world * 0.5f > khsb_floor ? texel_world * 0.5f : khsb_floor;
    // The union's cast bias, capped in metres like the four tiers, so the far->union step reads the same bias on
    // both sides.
    const float khsu_ccap = KH_CAST_BIAS_CAP_M;
    const float khsu_ctw = texel_world * 0.5f < khsu_ccap ? texel_world * 0.5f : khsu_ccap;
    const float cast_bias_world = khsu_ctw > khsb_floor ? khsu_ctw : khsb_floor;

    // depth-only pass: full save/restore, own clear, recursion off.
    const bool prev_inj = g_ro.in_injection;
    g_ro.in_injection = true;
    StateBackup backup;
    backup.capture(ctx);
    KhBindScope khsdp_bc(ctx);   // KH_BIND_FILTER.
    ID3D11Buffer* old_vb1 = nullptr;
    UINT old_vb1_stride = 0, old_vb1_offset = 0;
    ctx->IAGetVertexBuffers(1, 1, &old_vb1, &old_vb1_stride, &old_vb1_offset);
    KhOmSave khsd_om;   // Full OM set (8 RTVs + DSV).
    khsd_om.capture(ctx);
    UINT old_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&old_nvp, old_vps);

    g_sun_map_size = kh_sun_depth_size();   // KH_SUN_LADDER: the union's rendered size.
    ctx->ClearDepthStencilView(g_res.sun_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    ctx->OMSetRenderTargets(0, nullptr, g_res.sun_dsv);

    {
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<FLOAT>(kh_sun_depth_size());
        vp.Height = static_cast<FLOAT>(kh_sun_depth_size());
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
    }

    ctx->IASetInputLayout(instanced ? g_res.layout_sundepth : g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    if (instanced) {
        kh_sun_bind_opaque(ctx);
    } else {
        ctx->VSSetShader(g_res.vs, nullptr, 0);
        ctx->PSSetShader(nullptr, nullptr, 0);
    }
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ID3D11Buffer* khsd_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khsd_cbs);
    ctx->PSSetConstantBuffers(0, 2, khsd_cbs);   // KH_SUN_PS_CB: twin of the tier block's bind.
    ConstantData khsd_cbf = {};
    memcpy(khsd_cbf.view_proj, lvp, sizeof(khsd_cbf.view_proj));
    khsd_cbf.sun_origin[0] = g_sun_anchor_now[0];
    khsd_cbf.sun_origin[1] = g_sun_anchor_now[1];
    khsd_cbf.sun_origin[2] = g_sun_anchor_now[2];
    const bool khsd_frame_ok = kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khsd_cbf);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    {
        // A large slope bias leaks light through thin walls (the back face is millimetres behind the front); open
        // meshes store no back face at all. Twin of the band pass through kh_sun_rs_pick. The union stays on the clip
        // form (khrs_clamp false): its ortho volume is fitted around the caster set, so the fitting argument holds here
        // only.
        ctx->RSSetState(kh_sun_rs_pick(false));
    }
    bool khsd_inst_drawn = false;   // KH_SUN_OBJ: the stream took the casters.
    bool khsd_drawn = false;        // KH_SUN_OBJ: every caster reached the map (the commit below waits on it).
    if (instanced) {
        if (g_skin_pool_any) {   // KH_SKIN_POOL: a chunk's casters together, for its runs.
            for (const SunCaster& khmc_kc : casters) kh_cast_key_put(khmc_kc.mesh, khmc_kc.slot);
            std::sort(casters.begin(), casters.end(),
                      [](const SunCaster& a, const SunCaster& b) {
                          if (a.mesh != b.mesh) return a.mesh < b.mesh;
                          if (a.lod != b.lod) return a.lod < b.lod;   // KH_SHADOW_LOD_DRAWN.
                          return kh_cast_key_at(a.slot) < kh_cast_key_at(b.slot);
                      });
        } else {
            std::sort(casters.begin(), casters.end(),
                      [](const SunCaster& a, const SunCaster& b) {
                          return a.mesh != b.mesh ? a.mesh < b.mesh : a.lod < b.lod;   // KH_SHADOW_LOD_DRAWN.
                      });
        }

        bool filled = false;
        D3D11_MAPPED_SUBRESOURCE im = {};

        if (SUCCEEDED(ctx->Map(g_res.sun_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) {
            KhInstLane* outp = static_cast<KhInstLane*>(im.pData);   // KH_OBJBUF: slot + alpha.

            for (const auto& c : casters) {
                outp->slot = c.slot;
                outp->dither = 0.0f;
                outp->alpha = c.alpha;   // KH_CAST_ALPHA (unread by VSSunDepth).
                outp->mat_base = 0.0f;
                ++outp;
            }

            ctx->Unmap(g_res.sun_instance_vb, 0);
            kh_objbuf_bind(ctx);   // KH_OBJBUF.
            filled = true;
        }

        ConstantData khsd_obj = {};   // Zeroed object slice (unread by VSSunDepth).

        if (filled && khsd_frame_ok && kh_upload_obj_cb(ctx, g_res.composite_cb, khsd_obj)) {

            ID3D11Buffer* vbs[2] = { nullptr, g_res.sun_instance_vb };
            UINT strides[2] = { sizeof(MeshVertex), KH_INST_STRIDE };
            UINT offsets[2] = { 0, 0 };
            size_t first = 0;
            KhDepthSk khmc_sk = { g_res.vs_sundepth, g_res.layout_sundepth, g_res.vs_sundepth_sk, nullptr, nullptr };

            while (first < casters.size()) {
                // An alpha caster is not in an opaque run.
                if (casters[first].alpha_caster) { ++first; continue; }
                const int mid = casters[first].mesh;
                const int khmc_lod = casters[first].lod;   // KH_SHADOW_LOD_DRAWN.
                const KhVbRef khmc_vb = kh_mesh_vb_ref(mid, casters[first].slot);   // KH_CLOTH.
                const KhSkinPoolChunk* const khmc_pc =   // KH_SKIN_POOL.
                    kh_cast_pool_of(mid, casters[first].slot, g_res.vs_sundepth_sk);
                size_t last = first + 1;
                while (last < casters.size() && casters[last].mesh == mid && casters[last].lod == khmc_lod &&
                       kh_cast_run_joins(khmc_vb, khmc_pc, casters[last].mesh, casters[last].slot,
                                         g_res.vs_sundepth_sk) &&
                       !casters[last].alpha_caster) ++last;
                khmc_sk.use(ctx, khmc_pc);
                vbs[0] = khmc_vb.vb;
                offsets[0] = khmc_vb.off;   // KH_SKIN_POOL.
                ctx->IASetVertexBuffers(0, 2, vbs, strides, offsets);
                ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
                // The mask cast is a casting pass exactly as the sun depth is: same shadow LOD.
                UINT khmc_ls = 0, khmc_lc = 0;
                mesh_lod_range_of(mid, khmc_lod, khmc_ls, khmc_lc);   // KH_SHADOW_LOD_DRAWN.
                ctx->DrawIndexedInstanced(khmc_lc,
                                          static_cast<UINT>(last - first), khmc_ls, 0,
                                          static_cast<UINT>(first));
                first = last;
            }
            khmc_sk.end(ctx);   // KH_SKIN_POOL: the union's own pair again.

            khsd_drawn = kh_sun_draw_alpha(ctx, casters, khsd_obj, g_res.sun_instance_vb);
        }
        khsd_inst_drawn = filled;
    }
    // KH_SUN_OBJ: the per-object route when the stream could not take the casters (no instancing resources, a
    // failed map) - the same map, never an empty one committed as valid.
    if (!khsd_inst_drawn && khsd_frame_ok) khsd_drawn = kh_sun_draw_obj_t(ctx, casters, g_sun_anchor_now);

    khsh_render_cams();

    khsd_om.restore(ctx);
    if (old_nvp > 0) ctx->RSSetViewports(old_nvp, old_vps);
    ctx->IASetVertexBuffers(1, 1, &old_vb1, &old_vb1_stride, &old_vb1_offset);
    KH_SAFE_RELEASE(old_vb1);
    backup.restore(ctx);
    khsd_om.release();
    g_ro.in_injection = prev_inj;

    // Every in-map caster for the fire's per-pixel reach test (engine axes; <= 16 ride the CB list, more ride the
    // t2 structured-buffer twin).
    g_sun_locals.clear();

    for (const auto& c : casters) {
        const size_t khsl_b = g_sun_locals.size();
        g_sun_locals.resize(khsl_b + 6);
        float* lb = g_sun_locals.data() + khsl_b;
        lb[0] = c.pos[0];
        lb[1] = c.pos[2];   // SQF -> engine axes.
        lb[2] = c.pos[1];
        const float hl[3] = { c.bsize[0] * 0.5f, c.bsize[2] * 0.5f, c.bsize[1] * 0.5f };   // KH_CAST_REFIT.
        kh_rot_half_extents(hl, c.rot, c.rotated, lb + 3);
    }

    g_sun_local_count = static_cast<int>(casters.size());
    ++g_sun_locals_serial;   // KH_CASTOCC_KEYSER.

    // KH_SUN_OBJ: a map cleared above and not drawn whole (a failed constant upload) is never committed: validity
    // drops with the hash, as a tier's does, and the next call renders afresh.
    if (!khsd_drawn) {
        g_sun_map_valid = false;
        g_sun_map_hash = 0;
        return false;
    }

    memcpy(g_sun_map_vp, lvp, sizeof(g_sun_map_vp));
    memcpy(g_sun_map_anchor, g_sun_anchor_now, sizeof(g_sun_map_anchor));
    g_sun_map_hash = input_hash;   // commit-on-success only (see the skip above).
    g_sun_map_bias = bias_world / D;
    g_sun_map_cast_bias = cast_bias_world / D;
    g_sun_map_bounds[0] = ctr[0];
    g_sun_map_bounds[1] = ctr[1];
    g_sun_map_bounds[2] = ctr[2];
    g_sun_map_bounds[3] = hf[0];
    g_sun_map_bounds[4] = hf[1];
    g_sun_map_bounds[5] = hf[2];
    g_sun_map_valid = true;
    g_sun_map_time = effect_time_seconds();

    // A sub-settle sun wobble re-renders the map (float-exact hash) but must not de-mature it.
    {
        const float khsm_dot = sun[0] * g_sun_mat_sun[0] +
                               sun[1] * g_sun_mat_sun[1] +
                               sun[2] * g_sun_mat_sun[2];

        {
            const bool khsm_new = g_sun_map_render_time < 0.0f;
            const bool khsm_cchg = khsm_caster_hash != g_sun_mat_chash;
            const bool khsm_big = khsm_dot < 0.99862953f;
            const bool khsm_drift = khsm_dot < 0.99996192f;
            const bool khsm_calm = khsm_drift && !khsm_big && kh_sky_static();

            // khsm_calm (above): the roll already covers a sub-3-degree sun drift while the sky witness is static:
            // proven wobble, must not disarm the far-plane presence test. khsm_ccalm: a caster change on a standing
            // map (not new) under a sun that moved less than the drift threshold (0.5 degrees), sky static - kept
            // mature likewise.
            const bool khsm_ccalm = khsm_cchg && !khsm_new && !khsm_big && !khsm_drift && kh_sky_static();

            if (khsm_new || (khsm_cchg && !khsm_ccalm) || khsm_big ||
                (khsm_drift && !khsm_calm)) {

                g_sun_map_render_time = g_sun_map_time;
                g_sun_mat_chash = khsm_caster_hash;
                g_sun_mat_sun[0] = sun[0];
                g_sun_mat_sun[1] = sun[1];
                g_sun_mat_sun[2] = sun[2];
            } else if (khsm_calm || khsm_ccalm) {
                g_sun_mat_chash = khsm_caster_hash;   // A no-op when only the sun moved (casters unchanged).
                g_sun_mat_sun[0] = sun[0];   // Set too: a calm drift re-anchors the sun reference.
                g_sun_mat_sun[1] = sun[1];
                g_sun_mat_sun[2] = sun[2];
            }
        }
    }

    return true;
}
}