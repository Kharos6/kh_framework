#pragma once

// The feature half (rendering_integration_features.hpp): our sun shadow render (render_sun_depth): the camera pick, the
// admission and the generation caps. Never include it on its own.
namespace RenderIntegration {
// SKIN_CULL: render_sun_depth's camera and caster admission, shared with the skin cull (skin_cull_pass) so the
// two cannot drift (TWIN by construction). sun_cam_pick: the camera (cam_e; false = camera-less, which admits
// every shadow-active object).
inline bool sun_cam_pick(float cam_e[3]) {
    bool cam_valid = false;
    cam_e[0] = cam_e[1] = cam_e[2] = 0.0f;
    // SUN_CAM_LATCH: this camera is the anchor, the caster cylinder, the tier windows and the range fade's centre.
    // Do not prefer the raw bridge sample to the latch: it is one step ahead of the frame at ground and foreign (up to
    // 156 m) at altitude, so the whole shadow domain sits that far off the camera the meshes draw from - invisible
    // while shadows stay under their casters, a rim that leads the camera once a low sun stretches them out to it.
    // The injection's own take: the cycle latch, with the bridge admitted over it only when the clear-time fetch
    // failed and the latch is more than LATCH_JUMP_M behind it (inject_composited_meshes' rule). With no cycle
    // latch: the bridge, else g_ls.cam; none of them = camera-less.
    {
        RVExtBridge::ProjectionViewTransform cpv = {};
        const bool live = RVExtBridge::get_projection_view_transform(cpv);

        if (g_ro.cycle_pv_valid) {
            cam_of(g_ro.cycle_pv.view, cam_e);
            cam_valid = true;
            if (live && !g_boundary_pv_valid) {
                float lc[3];
                cam_of(cpv.view, lc);
                if (latch_cam_dist_sq(cam_e, lc) > LATCH_JUMP_M * LATCH_JUMP_M) {
                    cam_e[0] = lc[0]; cam_e[1] = lc[1]; cam_e[2] = lc[2];
                }
            }
        } else if (live) {
            cam_of(cpv.view, cam_e);
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
// the camera cam_e (a valid one) and the clamped range rad.
inline bool sun_admits(const RenderObject& o, const float cam_e[3], float rad) {
    const float ce0[3] = { o.pos[0], o.pos[2], o.pos[1] };
    // LOD_BOUNDS: the world box around everything it may draw - the cylinder and the drop below are in world axes.
    float he0[3];
    world_half_extents(o, he0);
    const float dx = ce0[0] - cam_e[0];
    const float dy = ce0[1] - cam_e[1];
    const float dz = ce0[2] - cam_e[2];
    const float hd = sqrtf(he0[0] * he0[0] + he0[1] * he0[1] + he0[2] * he0[2]);
    const float lim = rad + hd;

    // Cylinder, matching KhSunRangeFade: a sphere refuses casters whose shadow lands on ground the shader still asks
    // about.
    if (dx * dx + dz * dz > lim * lim || fabsf(dy) > lim) {
        // The sole guard is the sy > 1e-4 horizon test below (a validity floor, not a length cap); the reach feeds
        // only this eligibility test, never the map fit.
        bool in = false;

        if (g_sun_valid) {   // The reach rescue needs a published sun.
            const float sx = g_sun_dir_engine[0];
            const float sy = g_sun_dir_engine[1];
            const float sz = g_sun_dir_engine[2];
            const float hl = sqrtf(sx * sx + sz * sz);

            if (hl > 1.0e-4f && sy > 1.0e-4f) {
                float drop = (ce0[1] - he0[1]) - cam_e[1];
                if (drop < 0.0f) drop = 0.0f;
                const float k = hl / sy;   // Down-sun metres per metre
                                                          // of drop.
                const float ux = -sx / hl;   // Shadow runs opposite.
                const float uz = -sz / hl;
                // + lim: without it the ray stops 2 * he.y below the camera and every receiver deeper than that is
                // invisible to the test (at a low sun, (camera AGL) * k of missing reach - the shadow pop-in). A
                // receiver inside the shadow view distance is at most lim below the camera, so this is the tight bound.
                const float span = 2.0f * he0[1] + drop + lim;
                const float ay = dy - he0[1];   // Camera -> caster base, vertical.
                const float h = dx * ux + dz * uz;
                // The shadow is a band, not a line, and on a cylinder the horizontal and vertical tests are
                // independent given s, so the exact answer is an interval intersection. Horizontal: |q + u*k*s| <=
                // lim is a quadratic in s; no root means never in range.
                const float qa = k * k;
                const float qb = 2.0f * k * h;
                const float qc = dx * dx + dz * dz - lim * lim;
                const float disc = qb * qb - 4.0f * qa * qc;
                if (disc >= 0.0f && qa > 1.0e-12f) {
                    const float rt = sqrtf(disc);
                    float lo = (-qb - rt) / (2.0f * qa);
                    float hi = (-qb + rt) / (2.0f * qa);
                    // Vertical: at s the band spans [ay - s, ay - s + 2he], so it reaches the camera's +-lim window
                    // exactly on this range.
                    if (lo < 0.0f) lo = 0.0f;
                    if (lo < ay - lim) lo = ay - lim;
                    if (hi > span) hi = span;
                    const float vhi = ay + 2.0f * he0[1] + lim;
                    if (hi > vhi) hi = vhi;
                    in = lo <= hi;
                }
            }
        }
        return in;
    }
    return true;
}

// SUN_GEN: the scene generation the sun ladder was built from, and the reopen cap's window (sun_gen_reopen_ok).
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets every member.
struct SunGenState {
    // SUN_GEN - the caster generation (g_scene.gen_cast: syncs that took an object able to cast) the ladder was last
    // built from, stamped at render_sun_depth's head and again at its own sync. The cast fire builds the ladder first
    // in a cycle and the once-per-frame flag then holds it, so a game-thread edit of a caster (updateRender3D) that the
    // injection's or the flush's later sync took would draw at its new pose over maps of its old one - a mesh moving
    // down-sun reads its own previous surface as an occluder. Those passes re-open the flag when the caster generation
    // moved over standing maps (SUN_RESTEP's rule). The cost: nothing on a cycle with no caster taken in between;
    // with one, the caster walk runs again, and where the caster set's hash changed the union and every tier redraw
    // there (each tier key carries the whole caster hash) - the draw the next cycle's fire would have made, unless a
    // caster is written again before it (then one more that cycle).
    uint64_t map_gen = 0;
    uint64_t win_start  = 0;
    uint32_t win_n      = 0;
    uint64_t hold_until = 0;
};
static SunGenState g_sun_gen;
// SUN_GEN_CAP - the re-open's cost bound. A caster written every frame (a scripted mover) would hit the window on
// most cycles and pay a second ladder a frame without end. The re-opens are counted over a window of
// SUN_GEN_CAP_WIN cycles; past SUN_GEN_CAP_N in one window they are suspended for SUN_GEN_CAP_HOLD cycles,
// during which such a frame keeps the cast fire's ladder - a steady one-frame lag for the hold rather than exact on
// some frames and a frame late on the others (a lag that comes and goes is worse than a steady one). A re-open
// refused under the hold keeps the hold at least CAP_WIN cycles past it, so the hold ends only after a window with
// no re-open asked: a caster written every frame stays on the fire's ladder for as long as it is written (not CAP_N
// exact frames in every CAP_HOLD + CAP_N), and an occasional edit (a mesh shown, moved once) is corrected once the
// writing stops. Bound: at most CAP_N extra ladders per window of CAP_WIN cycles - up to a quarter of frames at the
// defaults, from a caster written about every fourth frame, which never passes CAP_N in a window and so never trips
// the hold (CAP_N per CAP_N + CAP_HOLD cycles, ~2.6 %, holds only once the hold has tripped).
// Render thread (or the game thread under a park, which stalls it); reset with g_sun_gen.map_gen. SUN_RESTEP's own
// re-open (a followed object the final step moved) is not counted.
static constexpr uint64_t SUN_GEN_CAP_WIN  = 64;
static constexpr uint32_t SUN_GEN_CAP_N    = 16;
static constexpr uint64_t SUN_GEN_CAP_HOLD = 600;
// True when a SUN_GEN re-open may run this cycle (counts it); false under the hold.
inline bool sun_gen_reopen_ok() {
    const uint64_t c = g_topo_cycles;
    if (c < g_sun_gen.hold_until) {
        const uint64_t e = c + SUN_GEN_CAP_WIN;   // The hold ends a quiet window after the last ask.
        if (e > g_sun_gen.hold_until) g_sun_gen.hold_until = e;
        return false;
    }
    if (c < g_sun_gen.win_start || c - g_sun_gen.win_start >= SUN_GEN_CAP_WIN) {
        g_sun_gen.win_start = c;
        g_sun_gen.win_n = 0;
    }
    if (g_sun_gen.win_n >= SUN_GEN_CAP_N) {
        g_sun_gen.hold_until = c + SUN_GEN_CAP_HOLD;
        g_sun_gen.win_n = 0;
        return false;
    }
    ++g_sun_gen.win_n;
    return true;
}

inline bool render_sun_depth(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_SUN_LADDER);
    if (g_sun_maps.sun_map_rendered_frame) return g_sun_maps.sun_map_valid;
    g_sun_maps.sun_map_rendered_frame = true;
    g_sun_gen.map_gen = g_scene.gen_cast;   // SUN_GEN: as this call finds it (the sync below restamps it).
    // PROF: the GPU stamp past the once-per-frame return. A zone reports its last run, and a frame calls this
    // twice (the cast fire, then the injection): stamped at the head, gSunLadder was the second call's return.
    KH_GPU_SCOPE(ctx, GPU_SUN_LADDER);

    float cam_e[3] = { 0.0f, 0.0f, 0.0f };
    const bool cam_valid = sun_cam_pick(cam_e);   // SKIN_CULL: the shared pick.

    // DLS_LOD_CAM: the light maps' level camera, here - ahead of the no-sun returns below, as the light
    // maps need no sun - and zero when camera-less, as the anchor below takes it.
    memcpy(g_sun_setup.sun_cam_now, cam_e, sizeof(g_sun_setup.sun_cam_now));

    // Same clamp as every other g_sun_range consumer; hoisted once, read by every caster test.
    const float sr_rad = fminf(fmaxf(g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
    // SKIN_CULL: the steps culled against the range they read; one the game thread moved since (the video
    // options) may admit what they culled, which is skinned first - ahead of the no-sun returns too, as the light
    // maps (dls_frame, after this call) reach by the range with or without a sun.
    if (ctx && g_skin_cull_any && (sr_rad != g_skin_cull_rad_lo || sr_rad != g_skin_cull_rad_hi)) {
        skin_cull_catchup(ctx);
    }

    // No published sun: no self / cast map. Without a sky witness validity drops here so frozen consumers stand down
    // too; with one the call returns at the `!sun` test below (the witness carries no map - sun_axis_witnessed),
    // where the flags are already down: g_sun_valid clears only at the session resets, which drop them as well.
    if (!g_sun_valid && !sun_axis_witnessed()) {
        g_sun_maps.sun_map_valid = false;
        g_sun_maps.sun2_map_valid = false;
        g_sun_maps.sun3_map_valid = false;
        g_sun_maps.sun4_map_valid = false;
        g_sun_maps.sun5_map_valid = false;
        return false;
    }
    if (!ctx) return false;

    // The maps render from the live derivation - the axis the engine's own cascades are built from, the reference the
    // eye compares our shadow against.
    const float* sun = shadow_sun();

    // SUN_AXIS_FOLD: the maps render from the publish itself.
    if (!sun) return false;

    // Camera-less flushes anchor at the origin (world-absolute).
    if (cam_valid) {
        g_sun_maps.sun_anchor_now[0] = cam_e[0];
        g_sun_maps.sun_anchor_now[1] = cam_e[1];
        g_sun_maps.sun_anchor_now[2] = cam_e[2];
    } else {
        g_sun_maps.sun_anchor_now[0] = 0.0f; g_sun_maps.sun_anchor_now[1] = 0.0f; g_sun_maps.sun_anchor_now[2] = 0.0f;
    }

    static std::vector<SunCaster> casters;   // Render-thread scratch (the record is at namespace
                                             // scope: CAST_ALPHA).
    casters.clear();

    {
        scene_sync();   // SCENE: live walk by reference, no copy.
        g_sun_gen.map_gen = g_scene.gen_cast;   // SUN_GEN: the scene these maps are built from.
        objbuf_sync(ctx);   // OBJBUF: the records every tier's stream indexes.
        for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
            if (!g_scene.alive[i]) continue;
            const RenderObject& o = g_scene.objs[i];
            if (!shadow_active(o)) continue;   // SHADOW_ACTIVE.

            if (cam_valid && !sun_admits(o, cam_e, sr_rad)) continue;   // SKIN_CULL: the shared admission.
            casters.push_back(sun_caster_of(o, cam_e));   // SHADOW_LOD_DRAWN.
        }
    }

    // SHADOW_SWITCH: no caster, no map. A lit receiver with castShadow off is not a caster, so receivers can
    // outlive every caster; left claimed, the maps would keep the last caster's shadow on them. Every claim goes (the
    // next frame with a caster renders afresh).
    if (casters.empty()) { sun_ladder_forget(); return false; }

    // The hash commits only at render success below: a failed render never lets a stale map vouch for new inputs.
    uint64_t input_hash = CryptoGenerator::FNV1A64_OFFSET;
    uint64_t caster_key = 0;   // SUN_TIER_KEY: the caster hash (+ TEX_LAND), set at the union hash.
    bool     did[4] = { false, false, false, false };   // SUN_TIER_KEY: tiers that rendered this call.
    auto render_cams = [&]() -> void {
        if (!cam_valid || casters.empty()) {
            g_sun_maps.sun2_map_valid = false;
            g_sun_maps.sun3_map_valid = false;
            g_sun_maps.sun4_map_valid = false;
            g_sun_maps.sun5_map_valid = false;
            return;
        }

        ID3D11Device* sh_dev = nullptr;
        ctx->GetDevice(&sh_dev);
        if (!sh_dev) {
            g_sun_maps.sun2_map_valid = false;
            g_sun_maps.sun3_map_valid = false;
            g_sun_maps.sun4_map_valid = false;
            g_sun_maps.sun5_map_valid = false;
            return;
        }
        // Per-band ensure: a band whose resources fail is stood down alone (meta zeroed, both consumers fall through
        // to the next tier). SUN_OBJ: no instancing requirement - a band draws per object without it
        // (sun_draw_obj_t).
        const bool core = g_res.vs && g_res.input_layout;
        const bool ok0 = core && ensure_sun_depth2(sh_dev);
        const bool ok1 = core && ensure_sun_depth3(sh_dev);
        const bool ok2 = core && ensure_sun_depth4(sh_dev);
        const bool ok3 = core && ensure_sun_depth5(sh_dev);
        // The moment pyramids + convert pair. Non-fatal: a band whose pyramid is missing or short stays classic
        // (sun_pf_convert commits a whole chain only - PF_WHOLE), and the next ensure makes what is missing.
        ensure_sun_pf(sh_dev);
        sh_dev->Release();
        if (!ok0) g_sun_maps.sun2_map_valid = false;
        if (!ok1) g_sun_maps.sun3_map_valid = false;
        if (!ok2) g_sun_maps.sun4_map_valid = false;
        if (!ok3) g_sun_maps.sun5_map_valid = false;
        if (!ok0 && !ok1 && !ok2 && !ok3) return;

        // Sun basis, once for all bands (they share the sun; only the footprint differs).
        const float sh_f[3] = { -sun[0], -sun[1], -sun[2] };
        float up[3] = { 0.0f, 1.0f, 0.0f };
        if (fabsf(sh_f[1]) > 0.99f) { up[0] = 1.0f; up[1] = 0.0f; }
        float sh_r3[3] = {
            up[1] * sh_f[2] - up[2] * sh_f[1],
            up[2] * sh_f[0] - up[0] * sh_f[2],
            up[0] * sh_f[1] - up[1] * sh_f[0],
        };
        const float sh_rl = sqrtf(sh_r3[0] * sh_r3[0] +
                                  sh_r3[1] * sh_r3[1] +
                                  sh_r3[2] * sh_r3[2]);
        if (sh_rl < 1e-6f) {
            g_sun_maps.sun2_map_valid = false;
            g_sun_maps.sun3_map_valid = false;
            g_sun_maps.sun4_map_valid = false;
            g_sun_maps.sun5_map_valid = false;
            return;
        }
        sh_r3[0] /= sh_rl; sh_r3[1] /= sh_rl; sh_r3[2] /= sh_rl;
        const float sh_u3[3] = {
            sh_f[1] * sh_r3[2] - sh_f[2] * sh_r3[1],
            sh_f[2] * sh_r3[0] - sh_f[0] * sh_r3[2],
            sh_f[0] * sh_r3[1] - sh_f[1] * sh_r3[0],
        };

        // State envelope: its own on every call (the union's call from inside its own bracket, which this nests in;
        // the hash-skip call alone) - one envelope for all bands.
        StateBackup bk;
        OmSave sh_om;
        UINT nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_VIEWPORT ovp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        const bool pinj = g_ro.in_injection;

        ID3D11Buffer* ovb1 = nullptr;
        UINT ovb1_stride = 0, ovb1_offset = 0;

        {
            g_ro.in_injection = true;
            bk.capture(ctx);
            // VB slot 1 is saved explicitly, as the union pass saves it (StateBackup covers slots 0 and 1 as well),
            // so our mesh + instance buffers never leak into the engine's next draws on a skip frame.
            ctx->IAGetVertexBuffers(1, 1, &ovb1, &ovb1_stride, &ovb1_offset);
            sh_om.capture(ctx);   // Full OM set (8 RTVs + DSV).
            ctx->RSGetViewports(&nvp, ovp);
            ctx->IASetInputLayout(g_res.layout_sundepth);
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            sun_bind_opaque(ctx);
            ctx->GSSetShader(nullptr, nullptr, 0);
            ctx->HSSetShader(nullptr, nullptr, 0);
            ctx->DSSetShader(nullptr, nullptr, 0);
            ID3D11Buffer* sh_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
            ctx->VSSetConstantBuffers(0, 2, sh_cbs);
            // SUN_PS_CB: the PS stage too. Twin: the base map below.
            ctx->PSSetConstantBuffers(0, 2, sh_cbs);
            ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
            // Twin of the union site through sun_rs_pick: the tiers bind the clamp twin (a null twin falls through).
            {
                ID3D11RasterizerState* rs = sun_rs_pick(true);
                ctx->RSSetState(rs);
            }
        }

        BindScope sh_bc(ctx);   // BIND_FILTER: the tiers' alpha casters (nested in the union's, or alone).

        // The hero body, one band per call; outputs land in the band's own statics through the reference parameters.
        auto one = [&](float half, float dep, UINT size,
                       ID3D11DepthStencilView* dsv,
                       float (&out_vp)[4][4], float& out_bias,
                       float& out_cbias,   // CAST_BIAS_CAP: the cast chain's own.
                       float& out_hd, bool& out_valid,
                       uint64_t& out_renders,
                       int pf_idx,
                       bool& out_pf) -> void {
            double dctr[3] = { 0.0, 0.0, 0.0 };
            bool   sh_dsnap = false;

            // The window moves in whole texels, so a moving camera never makes stored depth crawl under the compare.
            const float r2 = half * 1.02f;
            const float sh_tex = (2.0f * r2) / static_cast<float>(size);
            float sh_ctr[3] = { cam_e[0], cam_e[1], cam_e[2] };
            const double dr3[3] = { sh_r3[0], sh_r3[1], sh_r3[2] };
            const double du3[3] = { sh_u3[0], sh_u3[1], sh_u3[2] };
            const double dc[3] = { sh_ctr[0], sh_ctr[1], sh_ctr[2] };
            const double dtex = static_cast<double>(sh_tex);
            const double dpr = dc[0] * dr3[0] + dc[1] * dr3[1] + dc[2] * dr3[2];
            const double dpu = dc[0] * du3[0] + dc[1] * du3[1] + dc[2] * du3[2];
            const double dsr = floor(dpr / dtex) * dtex - dpr;
            const double dsu = floor(dpu / dtex) * dtex - dpu;
            // The snapped centre stays double into the eye and the translation row - collapsing to float here
            // re-quantises at the very ulp this block exists to beat.
            dctr[0] = dc[0] + dsr * dr3[0] + dsu * du3[0];
            dctr[1] = dc[1] + dsr * dr3[1] + dsu * du3[1];
            dctr[2] = dc[2] + dsr * dr3[2] + dsu * du3[2];
            sh_dsnap = true;
            sh_ctr[0] = static_cast<float>(dctr[0]);
            sh_ctr[1] = static_cast<float>(dctr[1]);
            sh_ctr[2] = static_cast<float>(dctr[2]);

            // SUN_TIER_KEY: everything below is a function of these (the caster key carries TEX_LAND for the
            // material maps' readiness). TIER_KEY_HELD: taken twice - here for the skip test, and again past the
            // reach latch below, whose value it hashes, so the key a render stores is the one the next call with
            // these inputs takes (the latch as this call leaves it); stored as found, that call missed and drew the
            // same map once more.
            auto make = [&]() -> uint64_t {
                uint64_t tk_k = CryptoGenerator::FNV1A64_OFFSET;
                auto tk_f = [&tk_k](const void* p, size_t n) {
                    tk_k = CryptoGenerator::fnv1a64_update(tk_k, p, n);
                };
                tk_f(&caster_key, sizeof(caster_key));
                tk_f(sun, 12);
                tk_f(dctr, sizeof(dctr));
                tk_f(&half, sizeof(half));
                tk_f(&dep, sizeof(dep));
                tk_f(&size, sizeof(size));
                tk_f(g_sun_maps.sun_anchor_now, sizeof(g_sun_maps.sun_anchor_now));
                tk_f(&g_sun_setup.sun_fwd_held[pf_idx], sizeof(g_sun_setup.sun_fwd_held[pf_idx]));
                tk_f(&g_res.sun_instance_cap, sizeof(g_res.sun_instance_cap));
                if (tk_k == 0) tk_k = 1;
                return tk_k;
            };
            uint64_t tk = make();

            if (out_valid &&
                g_sun_setup.sun_tier_key_ok[pf_idx] && g_sun_setup.sun_tier_key[pf_idx] == tk) {
                return;   // The standing map is this call's map.
            }

            g_sun_setup.sun_tier_key_ok[pf_idx] = false;   // Proven again by a completed render.
            out_valid = false;   // Proven true only by a completed render.
            out_pf = false;   // Proven true only by a completed convert.

            static std::vector<SunCaster> set;
            set.clear();
            float fwd = dep;

            for (const auto& c : casters) {
                const float dx[3] = { c.pos[0] - sh_ctr[0],
                                      c.pos[2] - sh_ctr[1],
                                      c.pos[1] - sh_ctr[2] };
                const float sh_hl[3] = { c.bsize[0] * 0.5f, c.bsize[2] * 0.5f, c.bsize[1] * 0.5f };   // CAST_REFIT.
                float sh_he[3];
                rot_half_extents(sh_hl, c.rot, c.rotated, sh_he);
                const float er = sqrtf(sh_he[0] * sh_he[0] +
                                       sh_he[1] * sh_he[1] +
                                       sh_he[2] * sh_he[2]);

                // Sun-axis split: ds is how far up-sun the caster sits (sun[] points toward the sun), lat
                // the offset the ortho clips on (per-axis max).
                const float ds = dx[0] * sun[0] + dx[1] * sun[1] +
                                 dx[2] * sun[2];
                // Band admission norm: the footprint is a square in the sun plane (+-r2 along r3 and u3), so the
                // lateral clip uses the per-axis max norm - a radial test misses casters in the corners whose
                // silhouettes land inside the window. er over-bounds the per-axis extent conservatively.
                const float lu = fabsf(dx[0] * sh_r3[0] +
                                       dx[1] * sh_r3[1] +
                                       dx[2] * sh_r3[2]);
                const float lv = fabsf(dx[0] * sh_u3[0] +
                                       dx[1] * sh_u3[1] +
                                       dx[2] * sh_u3[2]);
                const float lat = fmaxf(lu, lv);
                if (lat > r2 + er) continue;   // Ortho clips it laterally.
                if (ds > sr_rad + er) {
                    float slim = sr_rad + er;
                    if (g_sun_valid) {   // The reach rescue needs a published sun (twin).
                        const float ssx = g_sun_dir_engine[0];
                        const float ssy = g_sun_dir_engine[1];
                        const float ssz = g_sun_dir_engine[2];
                        const float shl = sqrtf(ssx * ssx +
                                                ssz * ssz);
                        if (shl > 1.0e-4f && ssy > 1.0e-4f) {
                            float sdrop = dx[1] - sh_he[1];
                            if (sdrop < 0.0f) sdrop = 0.0f;
                            // + sr_rad inside the drop: the throw must reach the RECEIVER, i.e. the ground up to
                            // the shadow view distance below the camera; without it the bound is short by (camera
                            // AGL) / sin(elevation) and the caster is dropped while its shadow lands inside the
                            // window. Twin of the eligibility span above.
                            const float slen = sr_rad + er
                                             + (sdrop + sr_rad
                                                + 2.0f * er) / ssy;
                            if (slen > slim) slim = slen;
                        }
                    }
                    if (ds > slim) continue;
                }
                if (ds < -(dep) - er) continue;
                set.push_back(c);
                const float need = ds + er;
                if (need > fwd) fwd = need;
            }

            // Entered only above the floor, so a band whose window was never stretched is bit-exact. fwd is a
            // maximum over every admitted caster and sizes d2v: this is how much one member costs all the
            // others in depth precision.
            if (fwd > dep) {
                float& hold = g_sun_setup.sun_fwd_held[pf_idx];

                if (hold >= fwd && hold <= fwd * BAND_FWD_DROP) {
                    fwd = hold;
                } else {
                    hold = fwd * BAND_FWD_SLACK;
                    fwd = hold;
                }
            } else {
                g_sun_setup.sun_fwd_held[pf_idx] = -1.0f;   // Never leave a stale latch.
            }
            tk = make();   // TIER_KEY_HELD: the latch as this call leaves it.

            const float rdep = fwd;
            const float d2v = fwd + dep + 2.0f;
            const float sh_eye[3] = {
                sh_ctr[0] + sun[0] * (rdep + 1.0f),
                sh_ctr[1] + sun[1] * (rdep + 1.0f),
                sh_ctr[2] + sun[2] * (rdep + 1.0f),
            };
            const double deye[3] = {
                (sh_dsnap ? dctr[0] : static_cast<double>(sh_ctr[0]))
                    + static_cast<double>(sun[0]) * (static_cast<double>(rdep) + 1.0),
                (sh_dsnap ? dctr[1] : static_cast<double>(sh_ctr[1]))
                    + static_cast<double>(sun[1]) * (static_cast<double>(rdep) + 1.0),
                (sh_dsnap ? dctr[2] : static_cast<double>(sh_ctr[2]))
                    + static_cast<double>(sun[2]) * (static_cast<double>(rdep) + 1.0),
            };
            float sh_view[4][4] = {};
            sh_view[0][0] = sh_r3[0]; sh_view[0][1] = sh_u3[0]; sh_view[0][2] = sh_f[0];
            sh_view[1][0] = sh_r3[1]; sh_view[1][1] = sh_u3[1]; sh_view[1][2] = sh_f[1];
            sh_view[2][0] = sh_r3[2]; sh_view[2][1] = sh_u3[2]; sh_view[2][2] = sh_f[2];
            const double da[3] = {
                (sh_dsnap ? deye[0] : static_cast<double>(sh_eye[0]))
                    - static_cast<double>(g_sun_maps.sun_anchor_now[0]),
                (sh_dsnap ? deye[1] : static_cast<double>(sh_eye[1]))
                    - static_cast<double>(g_sun_maps.sun_anchor_now[1]),
                (sh_dsnap ? deye[2] : static_cast<double>(sh_eye[2]))
                    - static_cast<double>(g_sun_maps.sun_anchor_now[2]),
            };
            sh_view[3][0] = static_cast<float>(-(da[0] * sh_r3[0] + da[1] * sh_r3[1] + da[2] * sh_r3[2]));
            sh_view[3][1] = static_cast<float>(-(da[0] * sh_u3[0] + da[1] * sh_u3[1] + da[2] * sh_u3[2]));
            sh_view[3][2] = static_cast<float>(-(da[0] * sh_f[0] + da[1] * sh_f[1] + da[2] * sh_f[2]));
            sh_view[3][3] = 1.0f;
            float sh_proj[4][4] = {};
            sh_proj[0][0] = 1.0f / r2;
            sh_proj[1][1] = 1.0f / r2;
            sh_proj[2][2] = 1.0f / d2v;
            sh_proj[3][3] = 1.0f;
            float sh_lvp[4][4];
            mul_4x4(sh_view, sh_proj, sh_lvp);

            const float bcx = fabsf(sh_ctr[0]) > fabsf(sh_ctr[1]) ? fabsf(sh_ctr[0]) : fabsf(sh_ctr[1]);
            const float bcm = bcx > fabsf(sh_ctr[2]) ? bcx : fabsf(sh_ctr[2]);
            const float bulp = nextafterf(bcm, 3.0e38f) - bcm;
            const float bfl = 4.0f * bulp;
            // The texel term is a multiple of the tier's own texel; the ulp term is a floor under it.
            const float bk = BAND_BIAS_TEXELS;
            const float bbw = sh_tex * bk > bfl ? sh_tex * bk : bfl;
            // The same quantity, capped in metres for the cast chain only.
            const float sh_ccap = CAST_BIAS_CAP_M;
            const float sh_ctw = sh_tex * bk < sh_ccap ? sh_tex * bk : sh_ccap;
            const float cbw = sh_ctw > bfl ? sh_ctw : bfl;

            // Membership is the ortho's own lateral clip, never a positional subset of it.
            if (set.empty()) return;   // Valid stays false.
            // SUN_OBJ: instanced while the union's stream holds the band's casters; per object otherwise (no
            // instancing resources, a set past the stream, a failed map).
            const bool inst = sun_inst_on() && set.size() <= static_cast<size_t>(g_res.sun_instance_cap);

            ConstantData obj = {};
            ConstantData sh_cbf = {};
            memcpy(sh_cbf.view_proj, sh_lvp, sizeof(sh_cbf.view_proj));
            // The matrix above is anchor-relative; VSSunDepth subtracts the same anchor from every vertex.
            sh_cbf.sun_origin[0] = g_sun_maps.sun_anchor_now[0];
            sh_cbf.sun_origin[1] = g_sun_maps.sun_anchor_now[1];
            sh_cbf.sun_origin[2] = g_sun_maps.sun_anchor_now[2];

            if (upload_frame_cb(ctx, g_res.composite_frame_cb, sh_cbf) &&
                upload_obj_cb(ctx, g_res.composite_cb, obj)) {
                ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
                ctx->OMSetRenderTargets(0, nullptr, dsv);
                {
                    D3D11_VIEWPORT sh_vp = {};
                    sh_vp.Width = static_cast<FLOAT>(size);
                    sh_vp.Height = static_cast<FLOAT>(size);
                    sh_vp.MaxDepth = 1.0f;
                    ctx->RSSetViewports(1, &sh_vp);
                }

                if (g_skin_pool_any) {   // SKIN_POOL: a chunk's casters together, for its runs.
                    for (const SunCaster& kc : set) cast_key_put(kc.mesh, kc.slot);
                    std::sort(set.begin(), set.end(),
                              [](const SunCaster& a, const SunCaster& b) {
                                  if (a.mesh != b.mesh) return a.mesh < b.mesh;
                                  if (a.lod != b.lod) return a.lod < b.lod;   // SHADOW_LOD_DRAWN.
                                  return cast_key_at(a.slot) < cast_key_at(b.slot);
                              });
                } else {
                    std::sort(set.begin(), set.end(),
                              [](const SunCaster& a, const SunCaster& b) {
                                  return a.mesh != b.mesh ? a.mesh < b.mesh : a.lod < b.lod;   // SHADOW_LOD_DRAWN.
                              });
                }
                D3D11_MAPPED_SUBRESOURCE sh_im = {};
                bool sh_drawn = false;

                if (inst && SUCCEEDED(ctx->Map(g_res.sun_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &sh_im))) {
                    ctx->IASetInputLayout(g_res.layout_sundepth);   // SUN_OBJ: a band before may have drawn per
                    sun_bind_opaque(ctx);                         // object.
                    InstLane* sh_outp = static_cast<InstLane*>(sh_im.pData);   // OBJBUF: slot + alpha.

                    for (const auto& c : set) {
                        sh_outp->slot = c.slot;
                        sh_outp->dither = 0.0f;
                        sh_outp->alpha = c.alpha;   // CAST_ALPHA (unread by VSSunDepth).
                        sh_outp->mat_base = 0.0f;
                        ++sh_outp;
                    }

                    ctx->Unmap(g_res.sun_instance_vb, 0);
                    objbuf_bind(ctx);   // OBJBUF: the tier's VS reads the records.

                    ID3D11Buffer* sh_vbs[2] = { nullptr, g_res.sun_instance_vb };
                    UINT sh_strides[2] = { sizeof(MeshVertex), INST_STRIDE };
                    UINT sh_offsets[2] = { 0, 0 };
                    size_t sh_first = 0;
                    DepthSk sk = { g_res.vs_sundepth, g_res.layout_sundepth, g_res.vs_sundepth_sk, nullptr,
                                   nullptr };

                    while (sh_first < set.size()) {
                        if (set[sh_first].alpha_caster) { ++sh_first; continue; }
                        const int sh_mid = set[sh_first].mesh;
                        const int lod = set[sh_first].lod;   // SHADOW_LOD_DRAWN.
                        size_t sh_last = sh_first + 1;
                        const VbRef vb = mesh_vb_ref(sh_mid, set[sh_first].slot);   // CLOTH.
                        const SkinPoolChunk* const pc =   // SKIN_POOL.
                            cast_pool_of(sh_mid, set[sh_first].slot, g_res.vs_sundepth_sk);
                        while (sh_last < set.size() && set[sh_last].mesh == sh_mid &&
                               set[sh_last].lod == lod &&
                               cast_run_joins(vb, pc, set[sh_last].mesh, set[sh_last].slot,
                                              g_res.vs_sundepth_sk) &&
                               !set[sh_last].alpha_caster) ++sh_last;
                        sk.use(ctx, pc);
                        sh_vbs[0] = vb.vb;
                        sh_offsets[0] = vb.off;   // SKIN_POOL.
                        ctx->IASetVertexBuffers(0, 2, sh_vbs, sh_strides, sh_offsets);
                        ctx->IASetIndexBuffer(g_res.mesh_ib[sh_mid], DXGI_FORMAT_R32_UINT, 0);
                        UINT ls = 0, lc = 0;
                        mesh_lod_range_of(sh_mid, lod, ls, lc);   // SHADOW_LOD_DRAWN.
                        ctx->DrawIndexedInstanced(lc,
                                                  static_cast<UINT>(sh_last - sh_first), ls, 0,
                                                  static_cast<UINT>(sh_first));
                        sh_first = sh_last;
                    }
                    sk.end(ctx);   // SKIN_POOL: the tier's own pair again.

                    // CAST_ALPHA: the alpha casters, this tier - a failed upload leaves the map incomplete.
                    sh_drawn = sun_draw_alpha(ctx, set, obj, g_res.sun_instance_vb);
                } else {
                    sh_drawn = sun_draw_obj_t(ctx, set, g_sun_maps.sun_anchor_now);   // SUN_OBJ.
                }
                if (sh_drawn) {
                    memcpy(out_vp, sh_lvp, sizeof(out_vp));
                    out_bias = bbw / d2v;
                    out_cbias = cbw / d2v;
                    out_hd = half;
                    out_valid = true;
                    out_renders++;
                    // The band renders here; freshness is all this site marks (the pyramid convert happens where the
                    // maps are bound: sun_pf_convert at the injection's and the flush's proven binds, and in the
                    // PIP and the in-front pass).
                    out_pf = out_valid;   // Fresh map => convert due.
                    g_sun_setup.sun_pf_fresh[pf_idx] = true;   // SUN_TIER_KEY.
                    g_sun_setup.sun_tier_key[pf_idx] = tk;
                    g_sun_setup.sun_tier_key_ok[pf_idx] = true;
                    did[pf_idx] = true;
                    {   // PCSS_CELL: the window centre sits on whole texels of the world lattice (dpr /
                        // dpu over the texel, floored), so a texel index t of this map is lattice index
                        // t - size / 2 + the centre's along r3, and t - size / 2 - the centre's along u3 (the map's v
                        // runs down u3). The size is even (a power of two).
                        const double lr = floor(dpr / dtex);
                        const double lu = floor(dpu / dtex);
                        const double lh = static_cast<double>(size / 2u);
                        uint32_t* const lo = g_sun_setup.sun_tier_lat[pf_idx];
                        // A window off every sane range (never, with a finite camera) takes offset 0: its noise
                        // follows the window.
                        const bool lfin = fabs(lr) < 1.0e15 && fabs(lu) < 1.0e15;
                        lo[0] = lfin ? static_cast<uint32_t>(static_cast<int64_t>(lr - lh)) : 0u;
                        lo[1] = lfin ? static_cast<uint32_t>(static_cast<int64_t>(-lu - lh)) : 0u;
                    }
                }
            }
        };

        float cr = 0.0f;

        for (const auto& ls_c : casters) {
            const float ls_hl[3] = { ls_c.bsize[0] * 0.5f, ls_c.bsize[2] * 0.5f,
                                     ls_c.bsize[1] * 0.5f };   // CAST_REFIT.
            float ls_he[3];
            rot_half_extents(ls_hl, ls_c.rot, ls_c.rotated, ls_he);
            const float r = sqrtf(ls_he[0] * ls_he[0] + ls_he[1] * ls_he[1] +
                                  ls_he[2] * ls_he[2]);
            if (r > cr) cr = r;
        }

        float s = sr_rad / 200.0f;
        if (s < 1.0f) s = 1.0f;
        if (s > 5.0f) s = 5.0f;
        // The ensures above guarantee each texture is the wanted size; record it as the rendered size for the CB fills.
        if (ok0) g_sun_setup.sun2_map_size = sun_hero_size();
        if (ok1) g_sun_setup.sun3_map_size = sun_mid_size();
        if (ok2) g_sun_setup.sun4_map_size = sun_out_size();
        if (ok3) g_sun_setup.sun5_map_size = sun_far_size();
        if (ok0) one(SUN_HERO_HALF, SUN_HERO_DEPTH, sun_hero_size(),
                     g_res.sun2_dsv, g_sun_setup.sun2_map_vp, g_sun_setup.sun2_map_bias,
                     g_sun_maps.sun2_cast_bias,
                     g_sun_setup.sun2_half_diag, g_sun_maps.sun2_map_valid,
                     g_sun_setup.sun2_renders,
                     0,
                     g_sun_maps.sun_pf_valid[0]);
        if (ok1) one(SUN_MID_HALF * s, SUN_MID_DEPTH * s,
                     sun_mid_size(),
                     g_res.sun3_dsv, g_sun_setup.sun3_map_vp, g_sun_setup.sun3_map_bias,
                     g_sun_maps.sun3_cast_bias,
                     g_sun_setup.sun3_half_diag, g_sun_maps.sun3_map_valid,
                     g_sun_setup.sun3_renders,
                     1,
                     g_sun_maps.sun_pf_valid[1]);
        if (ok2) one(SUN_OUT_HALF * s, SUN_OUT_DEPTH * s,
                     sun_out_size(),
                     g_res.sun4_dsv, g_sun_setup.sun4_map_vp, g_sun_setup.sun4_map_bias,
                     g_sun_maps.sun4_cast_bias,
                     g_sun_setup.sun4_half_diag, g_sun_maps.sun4_map_valid,
                     g_sun_setup.sun4_renders,
                     2,
                     g_sun_maps.sun_pf_valid[2]);
        // The far tier converts a moment pyramid like the camera tiers (sparse taps shimmer under a moving sun).
        // Domain: half = range * (sin e + cos e), depth = 6x range - the fade's cylinder projects to that max-norm in
        // the sun plane; plain 'range' under-covers in-range receivers. The far tier answers its whole window (cb.hlsl:
        // KhCastTier/KhSelfTier 'last'). The factor is quantized to 1/8 steps: read live it resizes the window and its
        // snap grid every frame the sun moves.
        const float far_f = fabsf(sun[1]) + sqrtf(fmaxf(1.0f - sun[1] * sun[1], 0.0f));
        const float far_half = sr_rad * (ceilf(far_f * 8.0f) * 0.125f);
        if (ok3) one(far_half, sr_rad * 6.0f, sun_far_size(),
                     g_res.sun5_dsv, g_sun_setup.sun5_map_vp, g_sun_setup.sun5_map_bias,
                     g_sun_maps.sun5_cast_bias,
                     g_sun_setup.sun5_half_diag, g_sun_maps.sun5_map_valid,
                     g_sun_setup.sun5_renders,
                     3,
                     g_sun_maps.sun_pf_valid[3]);
        // The anchor a tier's stored vp is relative to is the one it RENDERED
        // with; a skipped tier keeps its stamp (the fill rebases).
        if (ok0 && did[0]) memcpy(g_sun_setup.sun_tier_anchor[0], g_sun_maps.sun_anchor_now,
            sizeof(g_sun_maps.sun_anchor_now));
        if (ok1 && did[1]) memcpy(g_sun_setup.sun_tier_anchor[1], g_sun_maps.sun_anchor_now,
            sizeof(g_sun_maps.sun_anchor_now));
        if (ok2 && did[2]) memcpy(g_sun_setup.sun_tier_anchor[2], g_sun_maps.sun_anchor_now,
            sizeof(g_sun_maps.sun_anchor_now));
        if (ok3 && did[3]) memcpy(g_sun_setup.sun_tier_anchor[3], g_sun_maps.sun_anchor_now,
            sizeof(g_sun_maps.sun_anchor_now));

        {
            sh_om.restore(ctx);
            if (nvp > 0) ctx->RSSetViewports(nvp, ovp);
            ctx->IASetVertexBuffers(1, 1, &ovb1, &ovb1_stride, &ovb1_offset);
            KH_SAFE_RELEASE(ovb1);
            bk.restore(ctx);
            sh_om.release();
            g_ro.in_injection = pinj;
        }
    };

    uint64_t caster_hash = 0;

    {
        auto fnv = [&input_hash](const void* p, size_t n) {
            input_hash = CryptoGenerator::fnv1a64_update(input_hash, p, n);
        };
        bool tex_alpha = false;   // TEX_LAND: a caster draws through its material's maps.
        bool uv_anim = false;   // UV_ANIM: an alpha caster's coverage moves with the clock.

        for (const auto& c : casters) {
            tex_alpha |= c.alpha_caster && c.mats;
            uv_anim |= c.alpha_caster && c.mats && c.mats->uv_anim_alpha;
            fnv(c.pos, sizeof(c.pos));
            fnv(c.res, sizeof(c.res));   // POS_RES: a centre that moved finer than pos re-renders too.
            fnv(c.size, sizeof(c.size));
            fnv(c.rot, sizeof(c.rot));   // Rotating a caster re-renders.
            fnv(&c.mesh, sizeof(c.mesh));
            fnv(&c.alpha, sizeof(c.alpha));   // CAST_ALPHA: a fade re-renders (1/64 steps).
            fnv(&c.lod, sizeof(c.lod));   // SHADOW_LOD_DRAWN: a level change re-renders.
            const uint64_t mh = c.mats ? c.mats->hash : 0ull;
            fnv(&mh, sizeof(mh));   // A material change re-renders.
            const uint8_t ac = c.alpha_caster ? 1 : 0;
            fnv(&ac, 1);
            // CLOTH: a cloth caster's geometry moves while its transform holds still, so its shape key changes (a
            // skeletal mesh's too, from its consume-point step).
            const uint32_t cg = cloth_gen_of(c.slot);
            fnv(&cg, sizeof(cg));
        }

        caster_hash = input_hash;
        // TEX_LAND: into the union and tier keys, not the maturity hash (a landing is no caster change).
        if (tex_alpha) fnv(&g_tex_land_serial, sizeof(g_tex_land_serial));
        if (uv_anim) {   // UV_ANIM: likewise - a new key each cycle (mat_anim_cycle_key).
            const uint64_t ac = mat_anim_cycle_key();
            fnv(&ac, sizeof(ac));
        }
        caster_key = input_hash;   // SUN_TIER_KEY.
        fnv(sun, 12);
        if (cam_valid) {
            const int32_t cell[3] = {
                static_cast<int32_t>(floorf(cam_e[0] * 0.5f)),
                static_cast<int32_t>(floorf(cam_e[1] * 0.5f)),
                static_cast<int32_t>(floorf(cam_e[2] * 0.5f)),
            };
            fnv(cell, sizeof(cell));
            // SUN_RNG_HASH: the clamped range every consumer reads (sr_rad) - the raw one is unbounded above
            // (the video options' reading), and its float -> int conversion past 2^31 undefined.
            const int32_t rng = static_cast<int32_t>(sr_rad);
            fnv(&rng, sizeof(rng));
        }

        // No ensure_resources on this path: the hash commits only past the res_ok return below, after a render through
        // a successful ensure. So a release that empties g_res.mesh_vb must zero g_sun_maps.sun_map_hash with it -
        // Resources::release does (sun_ladder_forget), which makes this return safe from mask_cast_engine, the one
        // caller with no ensure_resources ahead of it. Do not pair the clear with release_shadow_device_state instead:
        // ensure_resources releases on its own failure paths.
        if (g_sun_maps.sun_map_valid && input_hash == g_sun_maps.sun_map_hash) {
            g_sun_maps.sun_map_time = effect_time_seconds();
            render_cams();   // Camera bands render every call.
            return true;
        }
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) return false;
    const bool res_ok = ensure_ok("sun shadow: sun-depth resources", ensure_resources(dev)) && ensure_sun_depth(dev);
    bool instanced = res_ok && g_res.vs_sundepth && g_res.layout_sundepth && g_res.obj_srv;   // OBJBUF.

    // SUN_INST_CAP: one 16-byte InstLane per caster (centre, extents and rotation live in the record buffer the
    // lane's slot indexes); no stream, the per-object route (SUN_OBJ).
    if (instanced && !sun_inst_ensure(dev, casters.size())) instanced = false;

    dev->Release();
    if (!res_ok) {
        // SUN_OBJ: no map fit to read is left - a failed re-create has released the union (sun_map_ensure frees
        // before it allocates), and a failed ensure_resources leaves it drawn for inputs this call no longer has (the
        // hash above did not match). SUN_LADDER_DROP: and no tier either - they render only past this return
        // (render_cams), so each would stand as last drawn, for casters and a camera this call no longer has,
        // while its own flag armed its lanes. The ladder's one authority drops them all with the hash (the tier keys
        // too, so a recovered call renders every tier fresh).
        sun_ladder_forget();
        return false;
    }

    // Combined caster AABB, engine axes (SQF [x, y, zASL] -> [x, zASL, y]).
    float mn[3] = { 1e18f, 1e18f, 1e18f };
    float mx[3] = { -1e18f, -1e18f, -1e18f };

    for (const auto& c : casters) {
        const float ce[3] = { c.pos[0], c.pos[2], c.pos[1] };
        const float hl[3] = { c.bsize[0] * 0.5f, c.bsize[2] * 0.5f, c.bsize[1] * 0.5f };   // CAST_REFIT.
        float he[3];
        rot_half_extents(hl, c.rot, c.rotated, he);

        for (int k = 0; k < 3; ++k) {
            if (ce[k] - he[k] < mn[k]) mn[k] = ce[k] - he[k];
            if (ce[k] + he[k] > mx[k]) mx[k] = ce[k] + he[k];
        }
    }

    float ctr[3] = { (mn[0] + mx[0]) * 0.5f, (mn[1] + mx[1]) * 0.5f, (mn[2] + mx[2]) * 0.5f };
    float hf[3] = { (mx[0] - mn[0]) * 0.5f, (mx[1] - mn[1]) * 0.5f, (mx[2] - mn[2]) * 0.5f };

    g_sun_setup.sun_map_cam_anchor = false;   // Re-derived on every re-fit; skip frames keep the committed
                                    // value.
    if (cam_valid) {
        g_sun_setup.sun_map_cam_anchor = true;
        ctr[0] = floorf(cam_e[0] * 0.5f) * 2.0f + 1.0f;
        ctr[1] = floorf(cam_e[1] * 0.5f) * 2.0f + 1.0f;
        ctr[2] = floorf(cam_e[2] * 0.5f) * 2.0f + 1.0f;
        // EA_SUN_RNG_ONE: the call's one clamped range (sr_rad), never a second read of the atomic - a game-thread
        // store between the reads would fit the union to one range and admit / cull its casters by another.
        hf[0] = hf[1] = hf[2] = sr_rad / 1.7320508f;
        constexpr float SUN_GROW_CAP = 3500.0f;   // enclosing-radius ceiling (~7 km span).
        const float h = SUN_GROW_CAP / 1.7320508f;

        for (int sg_k = 0; sg_k < 3; ++sg_k) {
            const float cl = fmaxf(mn[sg_k], ctr[sg_k] - h);
            const float ch = fminf(mx[sg_k], ctr[sg_k] + h);
            if (cl > ch) continue;   // Capped caster box empty on this axis.
            float lo = ctr[sg_k] - hf[sg_k];
            float hi = ctr[sg_k] + hf[sg_k];
            if (cl < lo) lo = cl;
            if (ch > hi) hi = ch;
            ctr[sg_k] = (lo + hi) * 0.5f;
            hf[sg_k] = (hi - lo) * 0.5f;
        }

    }
    const float rad_raw = sqrtf(hf[0] * hf[0] + hf[1] * hf[1] + hf[2] * hf[2]);
    float rad_use = rad_raw;
    if (g_sun_setup.sun_map_cam_anchor) {
        constexpr float SUN_UNION_R_SLACK = 1.10f;   // Headroom taken on acquisition.
        constexpr float SUN_UNION_R_DROP  = 1.25f;   // re-latch tighter past this.
        if (g_sun_setup.sun_union_rad_held >= rad_raw &&
            g_sun_setup.sun_union_rad_held <= rad_raw * SUN_UNION_R_DROP) {
            rad_use = g_sun_setup.sun_union_rad_held;
        } else {
            g_sun_setup.sun_union_rad_held = rad_raw * SUN_UNION_R_SLACK;
            rad_use = g_sun_setup.sun_union_rad_held;
        }
    }
    const float sf_rad = rad_use;
    const float R_dep = sf_rad + 1.0f;
    // R is refined (only ever downward) after the sun basis exists below - it needs the sun plane to measure in.
    float R = sf_rad > 1.0e-4f ? sf_rad * 1.02f : R_dep;

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

    if (g_sun_setup.sun_map_cam_anchor) {
        const float rng = sr_rad;   // EA_SUN_RNG_ONE.
        // Camera window term: its cell centre's lateral offset from the committed centre, plus the range.
        const float cc[3] = {
            floorf(cam_e[0] * 0.5f) * 2.0f + 1.0f,
            floorf(cam_e[1] * 0.5f) * 2.0f + 1.0f,
            floorf(cam_e[2] * 0.5f) * 2.0f + 1.0f,
        };
        const float cd[3] = { cc[0] - ctr[0], cc[1] - ctr[1],
                              cc[2] - ctr[2] };
        const float cs = cd[0] * sun[0] + cd[1] * sun[1] +
                         cd[2] * sun[2];
        const float cl2 = cd[0] * cd[0] + cd[1] * cd[1] +
                          cd[2] * cd[2] - cs * cs;
        // The camera's cylinder reaches sqrt(2) * range radially in the sun plane, not 'range'.
        float need = (cl2 > 0.0f ? sqrtf(cl2) : 0.0f) + rng * 1.4142136f;

        for (const auto& sl_c : casters) {
            const float sl_ce[3] = { sl_c.pos[0], sl_c.pos[2], sl_c.pos[1] };
            const float sl_hl[3] = { sl_c.bsize[0] * 0.5f, sl_c.bsize[2] * 0.5f,
                                     sl_c.bsize[1] * 0.5f };   // CAST_REFIT.
            float sl_he[3];
            rot_half_extents(sl_hl, sl_c.rot, sl_c.rotated, sl_he);
            const float er = sqrtf(sl_he[0] * sl_he[0] +
                                   sl_he[1] * sl_he[1] +
                                   sl_he[2] * sl_he[2]);
            const float d[3] = { sl_ce[0] - ctr[0], sl_ce[1] - ctr[1],
                                 sl_ce[2] - ctr[2] };
            const float ds = d[0] * sun[0] + d[1] * sun[1] +
                             d[2] * sun[2];
            const float l2 = d[0] * d[0] + d[1] * d[1] +
                             d[2] * d[2] - ds * ds;
            const float lat = (l2 > 0.0f ? sqrtf(l2) : 0.0f) + er;
            if (lat > need) need = lat;
        }

        if (need > sf_rad) need = sf_rad;   // Never coarser than the sphere.
        float use = need;
        constexpr float SUN_UNION_L_SLACK = 1.10f;
        constexpr float SUN_UNION_L_DROP  = 1.25f;
        if (g_sun_setup.sun_union_lat_held >= need &&
            g_sun_setup.sun_union_lat_held <= need * SUN_UNION_L_DROP) {
            use = g_sun_setup.sun_union_lat_held;
        } else {
            g_sun_setup.sun_union_lat_held = need * SUN_UNION_L_SLACK;
            use = g_sun_setup.sun_union_lat_held;
        }
        // Apply only when it refines - the latch slack can push the held value past the sphere in dense-lateral
        // scenes.
        if (use > 1.0e-4f && use * 1.02f < R) R = use * 1.02f;
    }

    const double ut_tex = (2.0 * static_cast<double>(R)) /
                          static_cast<double>(sun_depth_size());
    // UNION_DSNAP: the snapped centre stays double into the eye and the translation row, as the tiers keep
    // theirs (dctr) - collapsing it to float re-quantised the window at the ulp the snap exists to beat.
    double dn[3] = { 0.0, 0.0, 0.0 };
    bool ut_dsnap = false;
    if (ut_tex > 1.0e-9) {
        const double ut_c[3] = { ctr[0], ctr[1], ctr[2] };
        const double r[3] = { r3[0], r3[1], r3[2] };
        const double u[3] = { u3[0], u3[1], u3[2] };
        const double pr = ut_c[0] * r[0] + ut_c[1] * r[1] +
                          ut_c[2] * r[2];
        const double pu = ut_c[0] * u[0] + ut_c[1] * u[1] +
                          ut_c[2] * u[2];
        const double sr = floor(pr / ut_tex) * ut_tex - pr;
        const double su = floor(pu / ut_tex) * ut_tex - pu;
        const double ut_n[3] = {
            ut_c[0] + sr * r[0] + su * u[0],
            ut_c[1] + sr * r[1] + su * u[1],
            ut_c[2] + sr * r[2] + su * u[2],
        };
        dn[0] = ut_n[0];
        dn[1] = ut_n[1];
        dn[2] = ut_n[2];
        ut_dsnap = true;
        ctr[0] = static_cast<float>(ut_n[0]);
        ctr[1] = static_cast<float>(ut_n[1]);
        ctr[2] = static_cast<float>(ut_n[2]);
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
    // wpos, so the fp products shrink from world magnitude to fit magnitude. UNION_DSNAP: the eye and its offset
    // from the anchor in double from the snapped centre (the tiers' deye / da), the float eye where nothing
    // snapped; one rounding at the end.
    double se_a[3];
    for (int se_k = 0; se_k < 3; ++se_k) {
        const double e = ut_dsnap
            ? dn[se_k] + static_cast<double>(sun[se_k]) * (static_cast<double>(R_dep) + 1.0)
            : static_cast<double>(eye[se_k]);
        se_a[se_k] = e - static_cast<double>(g_sun_maps.sun_anchor_now[se_k]);
    }
    view[3][0] = static_cast<float>(-(se_a[0] * r3[0] + se_a[1] * r3[1] + se_a[2] * r3[2]));
    view[3][1] = static_cast<float>(-(se_a[0] * u3[0] + se_a[1] * u3[1] + se_a[2] * u3[2]));
    view[3][2] = static_cast<float>(-(se_a[0] * f[0] + se_a[1] * f[1] + se_a[2] * f[2]));
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
    const float texel_world = (2.0f * R) / static_cast<float>(sun_depth_size());
    // The floor is fp32 world precision, not depth-buffer precision.
    const float cx = fabsf(ctr[0]) > fabsf(ctr[1]) ? fabsf(ctr[0]) : fabsf(ctr[1]);
    const float cm = cx > fabsf(ctr[2]) ? cx : fabsf(ctr[2]);
    const float ulp = nextafterf(cm, 3.0e38f) - cm;
    const float sb_floor = 4.0f * ulp;
    const float bias_world = texel_world * 0.5f > sb_floor ? texel_world * 0.5f : sb_floor;
    // The union's cast bias, capped in metres like the four tiers, so the far->union step reads the same bias on
    // both sides.
    const float su_ccap = CAST_BIAS_CAP_M;
    const float su_ctw = texel_world * 0.5f < su_ccap ? texel_world * 0.5f : su_ccap;
    const float cast_bias_world = su_ctw > sb_floor ? su_ctw : sb_floor;

    // depth-only pass: full save/restore, own clear, recursion off.
    const bool prev_inj = g_ro.in_injection;
    g_ro.in_injection = true;
    StateBackup backup;
    backup.capture(ctx);
    BindScope sdp_bc(ctx);   // BIND_FILTER.
    ID3D11Buffer* old_vb1 = nullptr;
    UINT old_vb1_stride = 0, old_vb1_offset = 0;
    ctx->IAGetVertexBuffers(1, 1, &old_vb1, &old_vb1_stride, &old_vb1_offset);
    OmSave sd_om;   // Full OM set (8 RTVs + DSV).
    sd_om.capture(ctx);
    UINT old_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&old_nvp, old_vps);

    g_sun_setup.sun_map_size = sun_depth_size();   // SUN_LADDER: the union's rendered size.
    ctx->ClearDepthStencilView(g_res.sun_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    ctx->OMSetRenderTargets(0, nullptr, g_res.sun_dsv);

    {
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<FLOAT>(sun_depth_size());
        vp.Height = static_cast<FLOAT>(sun_depth_size());
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
    }

    ctx->IASetInputLayout(instanced ? g_res.layout_sundepth : g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    if (instanced) {
        sun_bind_opaque(ctx);
    } else {
        ctx->VSSetShader(g_res.vs, nullptr, 0);
        ctx->PSSetShader(nullptr, nullptr, 0);
    }
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ID3D11Buffer* sd_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, sd_cbs);
    ctx->PSSetConstantBuffers(0, 2, sd_cbs);   // SUN_PS_CB: twin of the tier block's bind.
    ConstantData sd_cbf = {};
    memcpy(sd_cbf.view_proj, lvp, sizeof(sd_cbf.view_proj));
    sd_cbf.sun_origin[0] = g_sun_maps.sun_anchor_now[0];
    sd_cbf.sun_origin[1] = g_sun_maps.sun_anchor_now[1];
    sd_cbf.sun_origin[2] = g_sun_maps.sun_anchor_now[2];
    const bool frame_ok = upload_frame_cb(ctx, g_res.composite_frame_cb, sd_cbf);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    {
        // A large slope bias leaks light through thin walls (the back face is millimetres behind the front); open
        // meshes store no back face at all. Twin of the band pass through sun_rs_pick. The union stays on the clip
        // form (clamp false): its ortho volume is fitted around the caster set, so the fitting argument holds here
        // only.
        ctx->RSSetState(sun_rs_pick(false));
    }
    bool inst_drawn = false;   // SUN_OBJ: the stream took the casters.
    bool sd_drawn = false;        // SUN_OBJ: every caster reached the map (the commit below waits on it).
    if (instanced) {
        if (g_skin_pool_any) {   // SKIN_POOL: a chunk's casters together, for its runs.
            for (const SunCaster& kc : casters) cast_key_put(kc.mesh, kc.slot);
            std::sort(casters.begin(), casters.end(),
                      [](const SunCaster& a, const SunCaster& b) {
                          if (a.mesh != b.mesh) return a.mesh < b.mesh;
                          if (a.lod != b.lod) return a.lod < b.lod;   // SHADOW_LOD_DRAWN.
                          return cast_key_at(a.slot) < cast_key_at(b.slot);
                      });
        } else {
            std::sort(casters.begin(), casters.end(),
                      [](const SunCaster& a, const SunCaster& b) {
                          return a.mesh != b.mesh ? a.mesh < b.mesh : a.lod < b.lod;   // SHADOW_LOD_DRAWN.
                      });
        }

        bool filled = false;
        D3D11_MAPPED_SUBRESOURCE im = {};

        if (SUCCEEDED(ctx->Map(g_res.sun_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) {
            InstLane* outp = static_cast<InstLane*>(im.pData);   // OBJBUF: slot + alpha.

            for (const auto& c : casters) {
                outp->slot = c.slot;
                outp->dither = 0.0f;
                outp->alpha = c.alpha;   // CAST_ALPHA (unread by VSSunDepth).
                outp->mat_base = 0.0f;
                ++outp;
            }

            ctx->Unmap(g_res.sun_instance_vb, 0);
            objbuf_bind(ctx);   // OBJBUF.
            filled = true;
        }

        ConstantData obj = {};   // Zeroed object slice (unread by VSSunDepth).

        if (filled && frame_ok && upload_obj_cb(ctx, g_res.composite_cb, obj)) {

            ID3D11Buffer* vbs[2] = { nullptr, g_res.sun_instance_vb };
            UINT strides[2] = { sizeof(MeshVertex), INST_STRIDE };
            UINT offsets[2] = { 0, 0 };
            size_t first = 0;
            DepthSk sk = { g_res.vs_sundepth, g_res.layout_sundepth, g_res.vs_sundepth_sk, nullptr, nullptr };

            while (first < casters.size()) {
                // An alpha caster is not in an opaque run.
                if (casters[first].alpha_caster) { ++first; continue; }
                const int mid = casters[first].mesh;
                const int lod = casters[first].lod;   // SHADOW_LOD_DRAWN.
                const VbRef vb = mesh_vb_ref(mid, casters[first].slot);   // CLOTH.
                const SkinPoolChunk* const pc =   // SKIN_POOL.
                    cast_pool_of(mid, casters[first].slot, g_res.vs_sundepth_sk);
                size_t last = first + 1;
                while (last < casters.size() && casters[last].mesh == mid && casters[last].lod == lod &&
                       cast_run_joins(vb, pc, casters[last].mesh, casters[last].slot,
                                      g_res.vs_sundepth_sk) &&
                       !casters[last].alpha_caster) ++last;
                sk.use(ctx, pc);
                vbs[0] = vb.vb;
                offsets[0] = vb.off;   // SKIN_POOL.
                ctx->IASetVertexBuffers(0, 2, vbs, strides, offsets);
                ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
                // The mask cast is a casting pass exactly as the sun depth is: same shadow LOD.
                UINT ls = 0, lc = 0;
                mesh_lod_range_of(mid, lod, ls, lc);   // SHADOW_LOD_DRAWN.
                ctx->DrawIndexedInstanced(lc,
                                          static_cast<UINT>(last - first), ls, 0,
                                          static_cast<UINT>(first));
                first = last;
            }
            sk.end(ctx);   // SKIN_POOL: the union's own pair again.

            sd_drawn = sun_draw_alpha(ctx, casters, obj, g_res.sun_instance_vb);
        }
        inst_drawn = filled;
    }
    // SUN_OBJ: the per-object route when the stream could not take the casters (no instancing resources, a
    // failed map) - the same map, never an empty one committed as valid.
    if (!inst_drawn && frame_ok) sd_drawn = sun_draw_obj_t(ctx, casters, g_sun_maps.sun_anchor_now);

    render_cams();

    sd_om.restore(ctx);
    if (old_nvp > 0) ctx->RSSetViewports(old_nvp, old_vps);
    ctx->IASetVertexBuffers(1, 1, &old_vb1, &old_vb1_stride, &old_vb1_offset);
    KH_SAFE_RELEASE(old_vb1);
    backup.restore(ctx);
    sd_om.release();
    g_ro.in_injection = prev_inj;

    // Every in-map caster for the fire's per-pixel reach test (engine axes; <= 16 ride the CB list, more ride the
    // t2 structured-buffer twin).
    g_sun_maps.sun_locals.clear();

    for (const auto& c : casters) {
        const size_t sl_b = g_sun_maps.sun_locals.size();
        g_sun_maps.sun_locals.resize(sl_b + 6);
        float* lb = g_sun_maps.sun_locals.data() + sl_b;
        lb[0] = c.pos[0];
        lb[1] = c.pos[2];   // SQF -> engine axes.
        lb[2] = c.pos[1];
        const float hl[3] = { c.bsize[0] * 0.5f, c.bsize[2] * 0.5f, c.bsize[1] * 0.5f };   // CAST_REFIT.
        rot_half_extents(hl, c.rot, c.rotated, lb + 3);
    }

    g_sun_maps.sun_local_count = static_cast<int>(casters.size());
    ++g_sun_maps.sun_locals_serial;   // CASTOCC_KEYSER.

    // SUN_OBJ: a map cleared above and not drawn whole (a failed constant upload) is never committed: validity
    // drops with the hash, as a tier's does, and the next call renders afresh.
    if (!sd_drawn) {
        g_sun_maps.sun_map_valid = false;
        g_sun_maps.sun_map_hash = 0;
        return false;
    }

    memcpy(g_sun_setup.sun_map_vp, lvp, sizeof(g_sun_setup.sun_map_vp));
    memcpy(g_sun_setup.sun_map_anchor, g_sun_maps.sun_anchor_now, sizeof(g_sun_setup.sun_map_anchor));
    g_sun_maps.sun_map_hash = input_hash;   // commit-on-success only (see the skip above).
    g_sun_setup.sun_map_bias = bias_world / D;
    g_sun_maps.sun_map_cast_bias = cast_bias_world / D;
    g_sun_maps.sun_map_bounds[0] = ctr[0];
    g_sun_maps.sun_map_bounds[1] = ctr[1];
    g_sun_maps.sun_map_bounds[2] = ctr[2];
    g_sun_maps.sun_map_bounds[3] = hf[0];
    g_sun_maps.sun_map_bounds[4] = hf[1];
    g_sun_maps.sun_map_bounds[5] = hf[2];
    g_sun_maps.sun_map_valid = true;
    g_sun_maps.sun_map_time = effect_time_seconds();

    // A sub-settle sun wobble re-renders the map (float-exact hash) but must not de-mature it.
    {
        const float dot = sun[0] * g_sun_maps.sun_mat_sun[0] +
                          sun[1] * g_sun_maps.sun_mat_sun[1] +
                          sun[2] * g_sun_maps.sun_mat_sun[2];

        {
            const bool sm_new = g_sun_maps.sun_map_render_time < 0.0f;
            const bool cchg = caster_hash != g_sun_maps.sun_mat_chash;
            const bool big = dot < 0.99862953f;
            const bool drift = dot < 0.99996192f;
            const bool calm = drift && !big && sky_static();

            // calm (above): the roll already covers a sub-3-degree sun drift while the sky witness is static:
            // proven wobble, must not disarm the far-plane presence test. ccalm: a caster change on a standing
            // map (not new) under a sun that moved less than the drift threshold (0.5 degrees), sky static - kept
            // mature likewise.
            const bool ccalm = cchg && !sm_new && !big && !drift && sky_static();

            if (sm_new || (cchg && !ccalm) || big ||
                (drift && !calm)) {

                g_sun_maps.sun_map_render_time = g_sun_maps.sun_map_time;
                g_sun_maps.sun_mat_chash = caster_hash;
                g_sun_maps.sun_mat_sun[0] = sun[0];
                g_sun_maps.sun_mat_sun[1] = sun[1];
                g_sun_maps.sun_mat_sun[2] = sun[2];
            } else if (calm || ccalm) {
                g_sun_maps.sun_mat_chash = caster_hash;   // A no-op when only the sun moved (casters unchanged).
                g_sun_maps.sun_mat_sun[0] = sun[0];   // Set too: a calm drift re-anchors the sun reference.
                g_sun_maps.sun_mat_sun[1] = sun[1];
                g_sun_maps.sun_mat_sun[2] = sun[2];
            }
        }
    }

    return true;
}
}