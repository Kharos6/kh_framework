#pragma once

// The injection half (rendering_integration_injection.hpp): the flush (flush_locked). Never include it on its own.
namespace RenderIntegration {


// khfl_rt (KH_NO_PARK): true for the unparked flush - the render thread's, from its scene-resolve hook, and
// KH_FX_LATE's chain-only flush; false for a park's (kh_park_run: bootstrap, watchdog - its deferred mission destroy
// returns before any flush). The expiry erase stays the park's (flush_frame's census erases otherwise).
// KH_SESSION_RESET: hoisted out of the function below so the session reset reaches it.
static bool g_flush_fx_arb_prev = false;
inline void flush_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool khfl_rt, bool khfl_chain_only) {
    KH_PROF_SCOPE(KHP_FLUSH);
    KH_GPU_SCOPE(ctx, KHG_FLUSH);   // KH_PROF.
    // A park that runs the post chain stamps its cycle (g_fx_park_cycle, at the chain below): the render-thread flush
    // draws no chain in it. The housekeeping block: a render-thread reader must not see a resized slot table, a
    // released buffer or a half-written Map; with the flush on the render thread that reader is this thread, so the
    // block belongs to the frame's only flush.
    if (!khfl_chain_only) {   // KH_FX_LATE: the scene flush did the housekeeping.
    // Land a publish kh_fbx_register could not park for, or a generated mesh's (KH_MESH_GEN_PUB: never parked; the
    // objects naming either wait out of the live scene until then, KH_SCENE_UNPUB). This body runs either under a park
    // held end to end or on the render thread itself (which no park can then stall mid-body), so the publish and the VB
    // top-up share one serialized window. Ahead of every skip below on purpose: a frame the flush declines still lands
    // it.
    kh_mesh_publish_land(dev);
    // KH_LOD_BOUNDS (a backstop: KH_SCENE_UNPUB syncs no such object): an object synced while its mesh was
    // unpublished was gridded at mesh 0's bounds (the id clamps); its cell radius is taken again once its own
    // every-level bounds read - whichever publish landed the id: the one above, the injection's (kh_mesh_publish_land),
    // or a kh_fbx_register since, which publishes every id appended before its own.
    if (g_scene_unpub_min < mesh_count()) {
        const uint32_t khmp_from = g_scene_unpub_min, khmp_to = mesh_count();
        uint32_t khmp_left = 0xFFFFFFFFu;   // The lowest id still unpublished.
        for (uint32_t khmp_s = 0; khmp_s < g_scene.objs.size(); ++khmp_s) {
            if (!g_scene.alive[khmp_s] || g_scene.cell[khmp_s] == KH_SCENE_CELL_NONE) continue;
            const int khmp_m = g_scene.objs[khmp_s].mesh;
            if (khmp_m < 0 || static_cast<uint32_t>(khmp_m) < khmp_from) continue;
            if (static_cast<uint32_t>(khmp_m) >= khmp_to) {
                if (static_cast<uint32_t>(khmp_m) < khmp_left) khmp_left = static_cast<uint32_t>(khmp_m);
                continue;
            }
            kh_scene_grid_remove(khmp_s);
            kh_scene_grid_insert(khmp_s, g_scene.objs[khmp_s]);
        }
        g_scene_unpub_min = khmp_left;
    }
    kh_mesh_grave_sweep();   // KH_MESH_FREE: the release wall drains here.
    kh_mesh_gc();
    kh_material_gc();   // KH_MAT_POOL.
    kh_tex_cache_gc();   // KH_TEX_GC.
    // KH_NO_PARK_THM - the heightfield upload belongs to the frame's only
    // flush; the flag is the atomic handoff kh_thm_upload owns.
    if (g_thm_dirty.load(std::memory_order_relaxed)) {
        std::string kht_err = kh_thm_upload(dev);
        kh_ensure_ok("terrain heightmap", kht_err);
    }
    // KH_CLOTH: create, upload and retire the per-instance vertex buffers (in this block for the reason above).
    // KH_UPLOAD_PRED: no StateBackup stands here, and the uploads make GPU writes D3D11 predicates - the skin pool's
    // trim / grow copies, a first install's UpdateSubresource into a stream-output buffer, the vertex stage's prime
    // copy (kh_uvs_copy_src). One the engine left bound could skip them, and a pool's old chunk is released right
    // after its copy: every pooled binding of the mesh would draw unwritten bytes. Set aside for the three and put
    // back, as kh_skin_gpu_move does for its own copy (redundant, harmless). Nothing in them relies on it:
    // their draws (kh_uvs_step's) are bracketed by a StateBackup, which clears it anyway.
    // KH_PRED_ASIDE: through the struct - a throw inside the three hands the predicate back and releases the
    // reference on the unwind (a hand-rolled pair leaves it cleared and leaks the reference).
    KhPredAside khfl_pa;
    if (ctx) khfl_pa.begin(ctx);
    kh_cloth_upload(ctx, dev);
    kh_skin_upload(ctx, dev);   // KH_SKEL: after the cloth, which empties the slot tables first.
    kh_uvs_upload(ctx, dev);    // KH_USER_VS: last - its source is whatever those two left in the slot.
    khfl_pa.end();   // KH_UPLOAD_PRED: handed back (GetPredication's reference released).
    }   // End of the scene flush's housekeeping.
    // KH_DL_FALLBACK_HARVEST: a cycle no accepted injection harvested (a rescue or anomalous cycle, a far-phase
    // refusal, an injection that returned early, no trigger at all; a trigger with nothing of ours to draw harvests
    // there, KH_DL_TRIGGER_HARVEST) still merges its span here, before this flush's late draws fill their lights. The
    // same harvest the injection runs - presence, TTL and authoring unchanged - so a light lives as long as on an
    // accepted cycle (one harvest longer at most, where the next injection bridges this span:
    // KH_DL_SPAN_BRIDGE); without it the pool aged past KH_DL_POOL_STALE_MS and every mesh lost every dynamic light
    // until an injection landed again. Render thread only: g_dl is render-thread state.
    if (khfl_rt && !khfl_chain_only && kh_dl_cpu_wanted() &&   // KH_DLF_DEMAND: a lit mesh / caster or the fog.
        g_dl_harvest_cycle != g_topo_cycles) {
        g_dl_harvest_cycle = g_topo_cycles;
        dynlights_harvest_arena(ctx, false);   // Resolve to resolve: no bridge.
        g_dl_harvest_flush_last = true;        // KH_DL_SPAN_BRIDGE: the next injection's span is short.
        kh_stat(g_stats.dl_harvests_flush);   // KH_DL_DIAG.
    }
    // Consumers of these plain stores run on the storing thread or are stalled by a park, so they never see a store
    // mid-write. Publish this frame's staged world lighting.
    if (!khfl_chain_only) publish_world_lighting();   // KH_FX_LATE: once, by the scene flush.

    // The lighting block AND the sun direction are published at the main depth
    // clear, not here: nothing on the flush arbitrates either.

    // Flush stamps: the serial keys the miss latch; the opaque store lifts the no-flush sentinel the clear re-arms
    // (kh_rescue_window_open tests only that - the count itself is compared nowhere).
    if (!khfl_chain_only) {   // KH_FX_LATE: the stamps are the scene flush's.
        g_cc_flush_serial.fetch_add(1, std::memory_order_relaxed);
        g_cc_flush_opaques.store(g_ro.opaque_draws, std::memory_order_relaxed);
    }

    // KH_NO_PARK_LATE: a cycle the injection did not take draws its meshes late in this flush, so this is the cycle's
    // last consume point for their transforms (g_attach_final_step). A park's late draw is stepped by kh_park_run.
    // Placed ahead of the entry tests, so a declined flush still advances the transforms the game thread reads next
    // (cloth sync, collider gather).
    if (khfl_rt && !g_ro.injected && !khfl_chain_only) {
        {
            KhFinalStepScope khfl_final;
            kh_attach_step(ctx);
        }
        kh_uvs_step(ctx);   // KH_USER_VS: this cycle's late draws, when no injection stepped it.
        // KH_SUN_RESTEP, the injection's twin: this flush's late meshes
        // self-shadow from the map the cast fire built before this step moved them.
        if (g_sun_restep_cycle == g_topo_cycles) g_sun_map_rendered_frame = false;
    }

    bool has_objects;

    kh_scene_sync();   // KH_SCENE.
    has_objects = g_scene.alive_n > 0;

    // Require a DSV and require it to be the main scene's depth, not a PiP/mirror/UAV sub-pass. Drawing into a sub-pass
    // misses the visible frame and churns the depth SRV cache against the wrong resource.

    {
        ID3D11DepthStencilView* dsv = nullptr;
        ctx->OMGetRenderTargets(0, nullptr, &dsv);

        if (!dsv) return;

        ID3D11Resource* res = nullptr;
        dsv->GetResource(&res);
        dsv->Release();

        if (!res) return;

        ID3D11Texture2D* tex = nullptr;
        HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
        res->Release();

        if (FAILED(hr) || !tex) return;

        D3D11_TEXTURE2D_DESC td = {};
        tex->GetDesc(&td);
        void* identity = static_cast<void*>(tex);
        tex->Release();

        const bool adopt = (g_main_depth_identity == nullptr) ||
                           (static_cast<uint64_t>(td.Width) * td.Height >
                            static_cast<uint64_t>(g_main_depth_w) * g_main_depth_h) ||
                           (g_wrong_pass_streak >= KH_WRONG_PASS_READOPT);

        if (adopt) {
            g_main_depth_identity = identity;
            g_main_depth_w = td.Width;
            g_main_depth_h = td.Height;
            g_wrong_pass_streak = 0;
        } else if (identity != g_main_depth_identity) {
            g_wrong_pass_streak++;
            // KH_WRONG_PASS_REPARK: nothing of ours this park - the caller re-parks. The park is the only caller with a
            // re-park loop, so the render-thread path leaves the flag alone rather than store across threads for a
            // reader it does not have; its own retry is the next cycle's resolve.
            if (!khfl_rt) g_flush_wrong_pass = true;
            return;
        } else {
            g_wrong_pass_streak = 0;
        }
        // td is the adopted-or-matched main scene depth, read in every flush with work at any FSAA state.
        g_scene_depth_samples.store(td.SampleDesc.Count, std::memory_order_relaxed);
        // This site sees every transition at any FSAA, in every flush (either thread's, chain-only runs too). The
        // wipe compares against g_res.depth_sample_count; the store after it keeps both trigger sites deduped.
        kh_msaa_toggle_wipe(td.SampleDesc.Count);
        g_res.depth_sample_count = td.SampleDesc.Count;
    }

    {   // A deferred ensure is not a failed one. The frame still ends.
        const std::string khf_res_err = ensure_resources(dev);
        // The hold is armed on the deferred branch and released here, on the first frame the gate passes.
        const bool khf_hold_now = khf_res_err == KH_SHADERS_COMPILING;
        if (khf_hold_now != g_acq_hold.load(std::memory_order_relaxed)) {
            g_acq_hold.store(khf_hold_now, std::memory_order_relaxed);
            // Everything the chain accumulated while the flush was down belongs to an era with no reference behind it,
            // so the release hands the first live frame a cold chain, not a poisoned one.
            if (!khf_hold_now) {
                g_ls.view_src_valid = false;
                g_ls.vc_n = 0;
                g_ls.pub_rej_streak = 0;
                g_ls.frame_view_valid = false;
                g_ls.prev_view_valid = false;
                g_ls.alt_view_valid = false;
                g_ring_best_valid = false;
                g_view_relock_wide_cycle = 0;
                                g_ls.wipe_deferred = true;

                for (int khf_b = 0; khf_b < 8; ++khf_b) {
                    g_ls.band[khf_b].valid = false;
                    g_ls.band[khf_b].pending_view = false;
                }
            }
        }
        if (!kh_ensure_ok("flush resources", khf_res_err)) {
            if (khf_res_err == KH_SHADERS_COMPILING) {
                kh_skin_cull_catchup(ctx);   // KH_SKIN_CULL: the placeholder draws every visible mesh.
                kh_white_draw(dev, ctx);
            }
            return;
        }
    }
    kh_user_shader_reap();
    kh_user_shader_pump(dev);
    // The scene-flush ordinal is the GAME THREAD's clock - the UI phase machine arms on it advancing once per frame
    // ahead of the UI pass, and flush_ui_locked reads a jump of two as a dark driver and skips every pass - so
    // flush_frame bumps it, never this body.

    if (!has_objects) return;

    // Snapshot the visible objects, draw without holding the mutex. Meshes and fullscreen passes are split; fullscreen
    // passes run last, in creation order so chained effects compose deterministically.
    const float snapshot_now = effect_time_seconds();
    // A park's verdict: per flush_frame call, not per park (see g_flush_landed_this_frame).
    // KH_NO_PARK_LANDED: the render-thread flush takes its own witness - a landing in the cycle it draws into.
    // g_flush_landed_this_frame is the game thread's verdict against its own previous call, which drifts from the
    // render thread's cycles; using it redrew composite meshes late over the engine's later draws, or adopted the
    // previous landing's view. A chain-only flush takes the same witness (its chain uses the view and encode chosen
    // from it), with the published cycle counter. A park's flush keeps the game-thread verdict.
    const uint64_t khf_land_c = khfl_chain_only ? g_topo_cycles_pub.load(std::memory_order_relaxed) : g_topo_cycles;
    const bool injected_since_last_flush = khfl_rt
                                         ? g_composite_land_cycle == khf_land_c
                                         : g_flush_landed_this_frame;
    // KH_FLUSH_CYCLE_WITNESS: whether the cycle these late draws go into has landed. The park stalls the render thread
    // inside that cycle, and the render-thread flush runs inside it, so the cycle's own flag (reset at its clear, set
    // at its landing) is definite here; the serial delta is not (it usually names the previous frame). A park landing
    // ahead of a healthy cycle's landing draws late and the injection draws again: identical overdraw.
    const bool khf_cycle_landed = g_ro.injected;
    // A frame that saw an anomalous cycle, or that never injected, gets its meshes drawn late here as well - identical
    // overdraw where the injection survived. The repaint term below was meant to add "or whose world redrew after the
    // injection" (fill-in where a foreign pass erased it), and it CANNOT fire: opaques_since_inject is incremented only
    // past reorder_pre_draw's return on g_ro.injected and is zeroed at the landing, so it is 0 whenever
    // khf_cycle_landed is true, and when khf_cycle_landed is false comp_healthy is already false without it. A later
    // partition is covered instead by the injection's own re-arm, which clears g_ro.injected: KH_PART_NEAR before
    // drawing (it defers to the camera partition), the anomalous rescue after a draw (a later attempt lands)
    // (proj_try_window's re-arm never runs: see there). Counting opaques past the landing would make this term live, at
    // the price of a late redraw of every composite mesh on any frame with 32 opaques after the landing - measure that
    // before changing it.
    const bool repainted_since_inject = g_ro.opaques_since_inject >= 32;
    const bool anomaly_this_frame = g_ro.anomaly_seen;
    g_ro.anomaly_seen = false;
    // khf_inj_healthy: the injection path is healthy and landed since the last flush; comp_healthy adds the cycle
    // witness and is the draw's verdict (may this flush stand down for the composite meshes). A park that lands ahead
    // of its cycle's landing draws late.
    const bool khf_inj_healthy = composite_path_healthy() && injected_since_last_flush &&
                                 !repainted_since_inject && !anomaly_this_frame;
    const bool comp_healthy = khf_inj_healthy && khf_cycle_landed;
    // KH_SKIN_CULL: a park draws under a view no step decided for, and may build the light maps - what was culled is
    // skinned first, before the walk below copies the boxes.
    if (!khfl_rt && !khfl_chain_only) kh_skin_cull_catchup(ctx);

    uint32_t khf_comp_eligible = 0;
    bool khf_any_lit = false;   // sun-map carry: a lit mesh exists this frame.
    bool khf_any_mesh = false;   // Any non-fullscreen consumer for the occlusion snapshot exists this frame.

    static std::vector<RenderObject> meshes;
    static std::vector<std::pair<uint64_t, RenderObject>> fullscreen;   // Key = creation seq.
    meshes.clear();
    fullscreen.clear();

    static std::vector<uint32_t> khf_expired;   // KH_SCENE: slots to erase after the walk.
    khf_expired.clear();
    // KH_WSEE: whether the injection's deferred copies joined meshes (appended after the walk).
    // KH_WSEE_DEFER_FLAG: the tail sort below finds them by RenderObject::ws_dcopy - kh_mesh_order permutes meshes,
    // so an index range over the appended copies named whichever objects sorted last (the depth-Off overlays, the
    // farthest translucents), drew those first and left the copies in the tail.
    bool khf_wd_any = false;

    {
        kh_scene_sync();   // KH_SCENE: live walk; the copies below are of visible objects only.
        for (uint32_t khsc_i = 0; khsc_i < g_scene.objs.size(); ++khsc_i) {
            if (!g_scene.alive[khsc_i]) continue;
            const RenderObject& khf_live = g_scene.objs[khsc_i];
            bool expired = false;
            const float env = lifetime_envelope(khf_live, snapshot_now, expired);

            if (expired) {
                // The erase belongs to flush_frame's census (game thread).
                if (khfl_rt) continue;
                khf_expired.push_back(khsc_i);
                continue;
            }
            // Sun-map demand: a visible lit receiver or any shadow-active object (casterOnly too).
            if ((kh_obj_vis(khf_live) && khf_live.lit && !khf_live.fullscreen &&   // KH_SHOW_INFRONT.
                 khf_live.effect != static_cast<int>(EffectId::Ssgi) &&   // KH_MESH_SSGI: draws nothing.
                 khf_live.effect != static_cast<int>(EffectId::DynLightFog) &&   // KH_DLF: nor this.
                 (!kh_in_front(khf_live) || g_vm_slice_live.load(std::memory_order_relaxed))) ||   // KH_INFRONT.
                kh_shadow_active(khf_live)) khf_any_lit = true;
            // KH_INFRONT: the view-model slice draws it (kh_infront_inject); it casts above. An effect mesh ignores
            // the flag (kh_in_front) and draws here.
            if (kh_in_front(khf_live)) continue;

            if (kh_obj_shown(khf_live)) {   // KH_SHOW_INFRONT: drawn by the main camera's passes.
                RenderObject o = khf_live;
                o.color[3] *= env;   // Envelope = universal intensity.
                // KH_FX_LATE: the scene flush drew the meshes. Every other visible mesh the injection does not own
                // (effect meshes, depth-Off, blended, fading, or any mesh on a cycle whose injection bailed) draws late
                // in this flush.
                if (khfl_chain_only && !o.fullscreen) continue;
                if (!o.fullscreen && kh_fsaa_world_standdown()) { continue; }
                // KH_MESH_SSGI: an effect mesh set to "ssgi" draws nothing. The gather is a scene-chain pass (its
                // grid factor, radiance seed and resolve exist only there); the PIP and the UI lane skip it too.
                if (!o.fullscreen && o.effect == static_cast<int>(EffectId::Ssgi)) continue;
                // KH_DLF: an effect mesh set to dynamicLightFog likewise (its gather is the scene chain's).
                if (!o.fullscreen && o.effect == static_cast<int>(EffectId::DynLightFog)) continue;
                if (is_composite_eligible(o)) ++khf_comp_eligible;
                if (!o.fullscreen) khf_any_mesh = true;

                if (o.fullscreen) {
                    // Scene and both semantics run here (the pre-tonemap chain); only UI-mode passes are excluded.
                    if (!o.ui_only) fullscreen.emplace_back(o.seq, o);
                } else if (comp_healthy && is_composite_eligible(o)) {
                    // Composited meshes are drawn pre-translucent by the reorder hook; the flush stands down for them
                    // while injections are happening, blend submeshes included (the injection draws the translucent
                    // part in its own tail).
                    //
                    // A park that lands ahead of a healthy cycle's landing (comp_healthy false, khf_inj_healthy true)
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
                    // KH_MAT_PART_OFF: a depth-Off overlay splits too - both parts take dss_off (the loop's depth
                    // state names mode Off ahead of the part), and its glass sorts in the tail as a translucent
                    // overlay does; the opaque part first where both sort there (KH_PART_ORDER).
                    if (o.effect == 0 && kh_obj_has_blend(o)) {
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

        // KH_WSEE: the injection's deferred translucent parts (g_wsee_defer_cycle's note), taken by this cycle's
        // first scene flush: where it stands down for the injection's meshes (comp_healthy), their copies join the
        // tail as draw_part 1 entries - each only while its slot still holds that object with that mesh and material
        // set (the live one keeps both alive through this flush's GC: kh_mesh_gc, kh_material_gc ran above) -;
        // where it draws them itself, their parts come with them (the split in the walk above) and the copies go.
        if (!khfl_chain_only && g_wsee_defer_cycle == g_topo_cycles) {
            if (comp_healthy) {
                for (const RenderObject& khf_wd : g_wsee_defer_objs) {
                    if (khf_wd.slot >= g_scene.objs.size() || !g_scene.alive[khf_wd.slot]) continue;
                    const RenderObject& khf_wl = g_scene.objs[khf_wd.slot];
                    if (khf_wl.seq != khf_wd.seq || khf_wl.mesh != khf_wd.mesh ||
                        khf_wl.materials != khf_wd.materials) continue;
                    // A copy draws from its hold entry (kh_wsee_hold_ref): a copy entry from the hold's bytes, an
                    // own entry (a static part, or one drawn from its mesh's buffer at the injection) from the mesh's
                    // own buffer at the injection's level. Void - the hold released or remade since (a switch to one
                    // sample, the device), or an own entry's mesh buffer gone - the copy is dropped.
                    if (khf_wd.ws_hold != 0u && !kh_wsee_hold_ref(khf_wd)) continue;
                    meshes.push_back(khf_wd);
                    khf_wd_any = true;
                }
                if (khf_wd_any) g_wsee_defer_taken = g_topo_cycles;   // Drawn once the loop runs (_drawn).
            }
            g_wsee_defer_cycle = ~0ull;
            g_wsee_defer_objs.clear();
        }

        // KH_SKIN_CULL: the late draws are the copies above, under this flush's own view: a culled binding among
        // them (a flag changed since the step, or no landing) takes this stash's pose first, its copies the box.
        if (!khfl_chain_only) kh_skin_cull_catchup(ctx, &meshes);
        if (khfl_chain_only) khf_any_lit = false;   // KH_FX_LATE: nothing to cast for; the scene flush prepared it.
        if (!khfl_rt && !khf_expired.empty()) {   // The flush owns the erasure (the others skip expired). Park only.
            std::lock_guard<std::mutex> g(g_draw_list_mutex);
            for (uint32_t khfe_s : khf_expired) {
                if (khfe_s >= g_scene_stage.size() || !g_scene_stage[khfe_s]) continue;
                auto khfe_it = g_draw_list.find(g_scene_handle[khfe_s]);
                if (khfe_it == g_draw_list.end() || khfe_it->second.slot != khfe_s) continue;
                kh_scene_stage_remove(khfe_it->second);
                kh_attach_drop(khfe_it->first);   // KH_ATTACH.
                g_draw_list.erase(khfe_it);
            }
        }
    }

    // KH_SCENE_HOLD: from here to the flush's end the scene stays the one the walk above copied - the passes below
    // re-synced it (kh_dls_caster_dist, render_sun_depth, kh_dls_frame), and a stage the game thread wrote meanwhile
    // put the buckets' records (kh_objbuf_sync, below) ahead of the copies they draw: a moved mesh culled at its old
    // pose and drawn at its new one, a reused slot's record under another object's mesh.
    const KhSceneHold khf_scene_hold;

    // No engine draw interleaves here (render thread inside the resolve hook, or a park). kh_dls_select renders
    // nothing (g_dls_frame holds it to once a cycle); render_sun_depth below saves its own RT and viewport state, and
    // g_sun_map_rendered_frame makes it a no-op once this cycle's map is built (the cast fire's or the injection's)
    // unless reopened (KH_SUN_GEN below, KH_SUN_RESTEP).
    kh_dls_select();   // KH_DL_SHADOW twin 2/2 (frame-guarded; free if twin 1 already ran).
    if (!injected_since_last_flush && khf_any_lit) {
        // KH_SUN_GEN: the injection's twin - the scene this flush holds (KH_SCENE_HOLD, above) against the one the
        // cast fire's ladder was built from.
        if (g_sun_map_valid && g_scene.gen_cast != g_sun_map_gen && kh_sun_gen_reopen_ok()) {   // KH_SUN_GEN_CAP.
            g_sun_map_rendered_frame = false;
        }
        render_sun_depth(ctx);
        kh_dls_frame(ctx);   // KH_DLS_FRAME twin 2/2 (frame-guarded; free if twin 1 already ran).
    }

    // KH_INFRONT_TPL: the view-model slice shades under the frame template the injection or this flush publishes
    // (kh_infront_inject: none, no draw). A cycle neither published in - inFront meshes alone, none of them lit and
    // casting (so the injection returned before its template), nothing late here - left the slice with no template,
    // or with the last one while the world's meshes stood. Then this flush runs up to the template, publishes it
    // and returns (the exit before the effect setup). Only such a cycle reaches that exit (khf_infront_tpl): every
    // other nothing-late flush returns ahead of it (the return below, or the one after the occlusion snapshot), so
    // a template the injection published stands. The same lanes either way - kh_fx_frame_template being the
    // injection's twin (KH_RENDER_FLUSH), and the slice's own fill sets what a slice draw reads differently.
    const bool khf_infront_tpl = g_infront_wanted.load(std::memory_order_relaxed) &&
                                 (!g_pip.tpl_valid || g_pip.tpl_cycle != g_topo_cycles);
    const bool khs_nothing_late = meshes.empty() && fullscreen.empty();
    if (khs_nothing_late && khf_comp_eligible == 0 && !khf_infront_tpl) {
        // Nothing to draw or snapshot for; the injection (healthy, per comp_healthy) picks up whatever turns visible.
        return;
    }

    std::sort(fullscreen.begin(), fullscreen.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    RVExtBridge::ProjectionViewTransform pv = {};

    const bool khf_sparse_live = !khf_cycle_landed &&   // KH_FLUSH_CYCLE_WITNESS: the cycle in flight.
        g_ro.opaque_draws < KH_REORDER_MIN_OPAQUE_DRAWS / 8;

    if (khf_sparse_live && RVExtBridge::get_projection_view_transform(pv)) {
    } else if (g_ro.cycle_pv_valid) {
        pv = g_ro.cycle_pv;   // KH_LATCH_FOLD: the boundary camera, never a held one.
    } else if (!RVExtBridge::get_projection_view_transform(pv)) {
        return;
    }

    // Cycle state can be read mid-reset from another thread; never transform
    // with a degenerate projection - the live bridge repairs.
    if (fabsf(pv.projection[2][2]) < 1e-6f) {
        RVExtBridge::ProjectionViewTransform live_fix = {};
        if (!RVExtBridge::get_projection_view_transform(live_fix)) {  return; }
        pv = live_fix;
    }

    // Registration repaint is meaningful only when an injected image exists
    // this frame; the window is the staleness bound on the landing's stamp.
    if (injected_since_last_flush &&
        g_inj_view_ms != 0 && steady_now_ms() - g_inj_view_ms < 100) {
        memcpy(&pv.view[0][0], g_inj_view, sizeof(g_inj_view));
    } else {
        kh_adopt_frame_view(pv);
    }

    const bool khf_slot_band = g_ro.slot_near_live >= KH_CAM_NEAR_MIN &&
                               g_ro.slot_near_live <= KH_CAM_NEAR_MAX &&
                               kh_enc_far(g_ro.slot_m22, g_ro.slot_m32) <= KH_ENC_FAR_MAX;

    const bool khf_pub_valid = g_inj_enc_ms != 0 && g_inj_enc_near > 0.0f;
    bool khf_inj_pair = khf_pub_valid && injected_since_last_flush;
    bool khf_latch_far = false;

    if (!khf_inj_pair && khf_pub_valid) {
        const float khf_pubv_near = fabsf(pv.projection[2][2]) > 1e-9f
                                  ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        const bool khf_near_agree = khf_pubv_near >= KH_CAM_NEAR_MIN &&
                                    khf_pubv_near <= KH_CAM_NEAR_MAX &&
                                    fabsf(khf_pubv_near - g_inj_enc_near) <= 0.25f * khf_pubv_near;

        if (khf_near_agree) {
            const float khf_pv_m221 = pv.projection[2][2] - 1.0f;
            const float khf_pub_m221 = g_inj_enc_m22 - 1.0f;
            const bool khf_pv_far_sane =
                kh_enc_far(pv.projection[2][2], pv.projection[3][2]) <= KH_ENC_FAR_MAX;
            const bool khf_far_agree = !khf_pv_far_sane ||
                fabsf(khf_pub_m221 - khf_pv_m221) <=
                0.25f * fmaxf(fabsf(khf_pv_m221), 1e-6f);

            if (khf_far_agree) {
                khf_inj_pair = true;
            } else {
                khf_latch_far = true;
            }
        }
    }

    // KH_LATE_WORLD_PAIR - a render-thread flush drawing a cycle the injection did not land in, on a frame drawn under
    // one pair, takes that pair: the world phase's latch (this cycle's g_ro.world_m22 / m32 - the pair the engine's
    // last opaque world draw rendered under, which wrote the depth these draws test against), the injection's own
    // first choice. Measured: in sparse views the injection never ran (6 to 65 engine opaques a frame), and the
    // publication rung took a pair published a frame to 52 s before whose near was within its 25 % of the camera's
    // (6.51 m against 6.29, 3.65 against 4.52). The engine's near plane moves with the view, so the mesh was encoded
    // nearer or farther than the terrain about it - a slice through it that moved with the camera, in front of the
    // terrain or behind it. The latch replaces exactly that: an earlier landing's publication (khf_inj_pair or
    // khf_latch_far, chosen against it) in a cycle that has not landed. Everything else stands - a landed cycle, a
    // session no injection has published in (post effects alone), a park's flush, a cycle with no latch, and a
    // partitioned frame: the latch's near past 1.5 x the camera's (KH_PART_NEAR), whose depth holds more than one
    // encode, so that no one pair is right for a late draw.
    bool khf_world_pair = false;
    if (khfl_rt && !injected_since_last_flush && khf_pub_valid && (khf_inj_pair || khf_latch_far) &&
        g_ro.world_pair_valid) {
        const float khf_wp_near = fabsf(g_ro.world_m22) > 1e-9f ? (-g_ro.world_m32 / g_ro.world_m22) : -1.0f;
        const float khf_wp_cam = fabsf(pv.projection[2][2]) > 1e-9f
                               ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        khf_world_pair = khf_wp_near > 0.0f && khf_wp_cam > 1e-4f && khf_wp_near <= 1.5f * khf_wp_cam;
    }

    if (khf_world_pair) {
        pv.projection[2][2] = g_ro.world_m22;
        pv.projection[3][2] = g_ro.world_m32;
    } else if (khf_inj_pair) {
        pv.projection[2][2] = g_inj_enc_m22;
        pv.projection[3][2] = g_inj_enc_m32;
    } else if (khf_latch_far) {
        // Far drifted under an agreeing near: the latch pair is the encode this flush - deliberately ahead of the slot
        // rung, whose flush-time value is the partition ambiguity the publish path exists to avoid.
    } else if (khf_slot_band) {
        pv.projection[2][2] = g_ro.slot_m22;
        pv.projection[3][2] = g_ro.slot_m32;
    } else if (g_slot_keep_near > 0.0f && g_slot_keep_ms != 0 &&
               steady_now_ms() - g_slot_keep_ms < 250) {
        // Slot silent this cycle: the keep is the poison shield, but an in-band pv that disagrees with it is fresher
        // (the moving-camera silent frame) and wins - same arbitration as the injection side.
        const float khf_pv_near = fabsf(pv.projection[2][2]) > 1e-9f
                                ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        const bool khf_pv_in_band = khf_pv_near >= KH_CAM_NEAR_MIN && khf_pv_near <= KH_CAM_NEAR_MAX;
        const bool khf_keep_agrees = khf_pv_in_band &&
            fabsf(khf_pv_near - g_slot_keep_near) <= 0.25f * khf_pv_near;

        if (khf_keep_agrees || !khf_pv_in_band) {
            pv.projection[2][2] = g_slot_keep_m22;
            pv.projection[3][2] = g_slot_keep_m32;
        }
    }

    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);

    // Verdict computed per mesh in upload_cb; PS pick at the loop's category switch.

    // Occlusion snapshot. Every consumer is mesh-side (the injection guard, the flush PSMain guard/arb, the t2
    // effect-mesh arbitration - all freshness-gated and fail-safe to the stock path); the fullscreen chain reads the
    // live depth SRV, never this.
    if (khf_any_mesh) kh_snapshot_take(dev, ctx, pv);

    if (khs_nothing_late && !khf_infront_tpl) {
        return;
    }

    bool need_inverse = false;

    for (const auto& o : meshes) {
        if (o.localized || o.effect == static_cast<int>(EffectId::Pulse) ||
            o.effect == static_cast<int>(EffectId::Fogscatter) ||
            o.effect == static_cast<int>(EffectId::DynLightFog) ||   // KH_DLF: its rays.
            o.effect == KH_EFFECT_CUSTOM) { need_inverse = true; break; }
    }

    if (!need_inverse) {
        for (const auto& f : fullscreen) {
            if (f.second.localized || f.second.effect == static_cast<int>(EffectId::Pulse) ||
                f.second.effect == static_cast<int>(EffectId::Fogscatter) ||
                f.second.effect == static_cast<int>(EffectId::DynLightFog) ||   // KH_DLF: its rays.
                f.second.effect == KH_EFFECT_CUSTOM) { need_inverse = true; break; }
        }
    }

    float inv_view_proj[4][4] = {};
    // The inverse serves KhWorldPosFenced / KhWorldPosRelFenced, which read the engine depth, so it inverts the view
    // that RENDERED that depth: the cycle's boundary camera - the flush's own pv may be the injection's view a frame
    // apart, which shimmers every localized mask's edge. The inverse is of the rotation-only view; the camera rides in
    // fxCam (an absolute inverse is kilometres cancelling per pixel in fp32).
    // KH_FX_FRAME_ADOPT: that camera as the meshes take it - under the engine's own frame rotation when it is fresh
    // (kh_adopt_frame_view, on a copy), its position through the honest inverse (kh_cam_of). The transpose
    // (extract_camera_pos) is the inverse only of an exactly orthonormal rotation: the translation carries the
    // camera's map position, kilometres, so a rotation a little off it misplaces the camera by metres, by an amount
    // that turns with the camera - every reconstructed point, the dynamic lights' fog's included, slid over the scene
    // in the Zeus camera until a camera reset.
    float khf_dvp[4][4];
    float khf_fx_cam[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool khf_dvp_pair_engine = false;   // KH_LOCAL_CULL: khf_dvp's projection carries the engine's pair.
    if (g_ro.cycle_pv_valid && !g_ro.cycle_pv_stale && fabsf(g_ro.cycle_pv.projection[2][2]) > 1.0e-6f) {
        RVExtBridge::ProjectionViewTransform khf_dpv = g_ro.cycle_pv;
        kh_adopt_frame_view(khf_dpv);   // The view only; the projection stays the cycle's.
        float khf_dproj[4][4];
        memcpy(khf_dproj, khf_dpv.projection, sizeof(khf_dproj));
        {   // KH_FX_ENGINE_PAIR: the pair the passes linearise with.
            // KH_FX_PAIR_HOLD: through the hold guard (kh_fx_depth_pair_guard's note) - this flush's verdict,
            // which the chain's passes replay (kh_fx_chain_run), so the inverse and every reader take one pair.
            // The local cull's witness stays the raw engine pair (kh_local_culled). The range lanes go in as the
            // passes carry them (the guard adopts all four). KH_FX_PAIR_ONE: on both routes - with no engine pair
            // the passes carry this flush's own pair (upload_cb: pv's projection), and the inverse must invert
            // through the pair every reconstruction encoded with.
            ConstantData khf_pg = {};
            khf_pg.depth_params[0] = g_ro.engine_proj_valid ? g_ro.engine_m22 : pv.projection[2][2];
            khf_pg.depth_params[1] = g_ro.engine_proj_valid ? g_ro.engine_m32 : pv.projection[3][2];
            kh_fill_depth_range_cb(khf_pg);
            kh_fx_depth_pair_guard(khf_pg, g_cc_flush_serial.load(std::memory_order_relaxed));
            khf_dproj[2][2] = khf_pg.depth_params[0];
            khf_dproj[3][2] = khf_pg.depth_params[1];
            khf_dvp_pair_engine = g_ro.engine_proj_valid;   // KH_LOCAL_CULL: the engine's pair alone.
        }
        float khf_dview[4][4];
        memcpy(khf_dview, khf_dpv.view, sizeof(khf_dview));
        khf_dview[3][0] = khf_dview[3][1] = khf_dview[3][2] = 0.0f;
        khf_dview[3][3] = 1.0f;
        mul_4x4(khf_dview, khf_dproj, khf_dvp);
        kh_cam_of(khf_dpv.view, khf_fx_cam);
        khf_fx_cam[3] = 1.0f;
    } else {
        memcpy(khf_dvp, view_proj, sizeof(khf_dvp));   // Absolute; fxCam stays disarmed.
    }
    const bool has_inverse = need_inverse && inverse_4x4(khf_dvp, inv_view_proj);
    // Only objects that need the inverse consult has_inverse, so skipping it when none does changes nothing.
    // KH_LOCAL_CULL: the frame the chain's localized passes reconstruct in - camera-relative, built with the
    // engine's pair (the pair those passes linearise with); otherwise nothing is culled.
    KhLocalCull khf_lcull;
    if (has_inverse && g_ro.engine_proj_valid && khf_fx_cam[3] > 0.5f &&
        khf_dvp_pair_engine) {
        kh_local_cull_make(khf_lcull, khf_dvp, khf_fx_cam, true, g_ro.engine_m22, g_ro.engine_m32);
    }
    float cam[3];
    kh_cam_of(pv.view, cam);   // KH_FX_FRAME_ADOPT: the honest inverse, as fxCam and the injection take it.
    // Taken here, before the rain tracker and every other consumer, so the pass is internally coherent.
    const bool khf_engcam = kh_engcam_consume(pv.view, cam);
    // KH_FOG_PROBE: a cycle the injection recorded no camera for (none landed, or it returned with nothing to inject)
    // takes this pass's. A cycle it did record keeps the injection's.
    if (g_ls_cam_cycle != g_topo_cycles) {
        g_ls.cam[0] = cam[0];
        g_ls.cam[1] = cam[1];
        g_ls.cam[2] = cam[2];
        g_ls_cam_cycle = g_topo_cycles;
    }

    {
        const uint64_t khrm_now = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());

        if (g_rain_prev_valid) {
            const float khrm_dt = static_cast<float>(khrm_now - g_rain_prev_us) * 1e-6f;

            if (khrm_dt > 0.0005f && khrm_dt < 0.5f) {
                const float khrm_dx = cam[0] - g_rain_prev_cam[0];
                const float khrm_dy = cam[1] - g_rain_prev_cam[1];
                const float khrm_dz = cam[2] - g_rain_prev_cam[2];
                const float khrm_step2 = khrm_dx * khrm_dx + khrm_dy * khrm_dy + khrm_dz * khrm_dz;

                if (khrm_step2 < 100.0f * 100.0f) {
                    const float khrm_a = 1.0f - expf(-khrm_dt / 0.15f);

                    for (int khrm_k = 0; khrm_k < 3; ++khrm_k) {
                        const float khrm_v = (khrm_dx * pv.view[0][khrm_k] +
                                              khrm_dy * pv.view[1][khrm_k] +
                                              khrm_dz * pv.view[2][khrm_k]) / khrm_dt;
                        g_rain_vel[khrm_k] += (khrm_v - g_rain_vel[khrm_k]) * khrm_a;
                    }
                } else {
                    g_rain_vel[0] = g_rain_vel[1] = g_rain_vel[2] = 0.0f;
                }

                const float khrm_fk = fminf(fmaxf(g_rain_vel[2], 0.0f) / 20.0f, 1.0f);
                const float khrm_ud = fminf(fmaxf(-g_rain_vel[1] * 0.06f, 0.0f), 1.0f);
                g_rain_phase += static_cast<double>(khrm_dt) * (1.0 + static_cast<double>(khrm_fk) * 2.2 + static_cast<double>(khrm_ud));
            }
        }

        g_rain_prev_cam[0] = cam[0];
        g_rain_prev_cam[1] = cam[1];
        g_rain_prev_cam[2] = cam[2];
        g_rain_prev_us = khrm_now;
        g_rain_prev_valid = true;
    }

    // Screen dimensions for the effect shader. The entry viewport's depth range is neither used nor learned
    // (KH_FLUSH_DEPTH_RANGE, upload_cb; KH_SCENE_VP_TRIG): a park can land in the view model's slice or a fullscreen
    // pass's [0, 1].
    float screen_w = 0, screen_h = 0;
    {
        UINT n_vp = 1;
        D3D11_VIEWPORT vp = {};
        ctx->RSGetViewports(&n_vp, &vp);

        if (n_vp >= 1) {
            screen_w = vp.Width;
            screen_h = vp.Height;
        }
    }

    // The pass's own pixels-per-tangent, published here for both draw loops. KH_LOD_ONE_SCALE: kh_lod_publish, the
    // injection's definition too; on the render thread (the scene viewport bound) an unusable pair publishes 0
    // (every level-0 pick). KH_LOD_PARK_KEEP: a park publishes only at the injection's last pass height, or while
    // the injection has not published within the last two cycles; otherwise, and for an unusable pair, the last
    // value stands.
    const bool khf_lod_inj_live = g_lod_inj_cycle != ~0ull && g_topo_cycles - g_lod_inj_cycle <= 2u;
    if (khfl_rt || !khf_lod_inj_live || screen_h == g_lod_inj_h) {
        if (!kh_lod_publish(pv.projection[1][1], screen_h) && khfl_rt) {
            g_lod_proj_px = 0.0f;
            g_lod_zoom = 1.0f;   // KH_LOD_ZOOM.
        }
    }

    if (khs_nothing_late) {   // KH_INFRONT_TPL (above): the template alone - the full flush's publish, nothing drawn.
        ConstantData khf_tcb = {};
        kh_fx_frame_template(khf_tcb, pv, view_proj, inv_view_proj, khf_fx_cam, khf_engcam, cam);
        kh_fill_user_cam_cb(khf_tcb, cam, pv.view);   // KH_USER_LANES.
        g_pip.tpl = khf_tcb;
        g_pip.tpl_valid = true;
        g_pip.tpl_cycle = g_topo_cycles;
        return;
    }

    const float now = effect_time_seconds();
    const double now_d = effect_time_seconds_d();

    // Capability gating per frame: effects need the scene capture and the effect shader; depth effects additionally
    // need the depth SRV in the PS, which requires the read-only DSV swap.
    auto needs_depth = [](const RenderObject& o) { return kh_fx_needs_depth(o); };   // KH_NEARZ_READER.

    bool any_effect = !fullscreen.empty();
    bool any_depth_fx = false;
    // KH_CAP_DEFER - with no mesh of this flush's own (none to draw, none composite-eligible: so no SSAO term either),
    // the scene capture has one reader, the chain, and nothing draws into the scene between here and the chain (the
    // sun pyramids' conversion draws into its own targets and restores the OM; the DSV swap moves the depth view
    // only). So the copy is taken where the chain takes its own after meshes (kh_fx_chain_run's khfc_capture): the
    // same picture from the same target. A chain this flush hands to KH_FX_LATE then costs no copy here (the late
    // flush takes its own), which saves a full resolve a frame on an MSAA scene. Not under a predicate the engine left
    // bound: the copy here runs under it, the chain's after StateBackup has cleared it (a copy the predicate skipped
    // left the chain the last capture; deferred, it would get this frame's). So with one bound the copy stays here.
    // Nothing between here and the setup's copy sets predication (only StateBackup does).
    // KH_CAP_DEFER_FXM: with meshes but none with an effect (effect != 0, the t0 = capture category, counted before
    // any demotion to 0), the copy here has no reader at all: every mesh draw binds t0 by its category first (the
    // snapshot or null), a perceptual translucent takes its own capture in the loop (KH_FX_CAP_LAZY: the first of
    // them) and t3 holds only captures taken since, a lighten / darken mesh copies its own (kh_mm_capture), the
    // pyramids are an effect mesh's, the see-through record's B / Q / C are other textures, the DLS world pass ran
    // before the flush, and the chain takes its own after meshes (khfc_capture: !meshes.empty()). So it is left out
    // the same way; the checks run.
    bool khf_fxm_any = false;
    for (const auto& khf_fm : meshes) if (khf_fm.effect != 0) { khf_fxm_any = true; break; }
    bool khf_cap_defer = meshes.empty() ? khf_comp_eligible == 0 : !khf_fxm_any;
    if (khf_cap_defer) {
        ID3D11Predicate* khf_cap_pred = nullptr;
        BOOL khf_cap_pred_v = FALSE;
        ctx->GetPredication(&khf_cap_pred, &khf_cap_pred_v);
        if (khf_cap_pred) {
            khf_cap_defer = false;
            khf_cap_pred->Release();
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
    bool effects_ready = false;
    bool depth_fx_ready = false;
    if (any_effect) {
        UINT dw = 0, dh = 0;

        {
            std::string khf_fx_err = ensure_depth_srv(dev, ctx, &dw, &dh);
            if (khf_fx_err.empty()) khf_fx_err = ensure_effect_shader(dev);
            if (khf_fx_err.empty()) {   // KH_CAP_DEFER: the checks always, the copy below or at the chain.
                khf_fx_err = khf_cap_defer ? ensure_scene_capture(dev, ctx, false) : kh_scene_capture_timed(dev, ctx);
            }
            effects_ready = khf_fx_err.empty();
            // Reported once per distinct message.
            if (!effects_ready) kh_ensure_ok("effect setup", khf_fx_err);   // KH_ENSURE_ONCE.
        }

        depth_fx_ready = effects_ready && any_depth_fx &&
                         g_res.depth_srv && g_res.depth_dsv_ro;

        // Same demotion, same fail direction.
        if (!has_inverse) {
            for (auto& o : meshes)
                if (o.effect == static_cast<int>(EffectId::Pulse) ||
                    o.effect == static_cast<int>(EffectId::Fogscatter) ||
                    o.effect == static_cast<int>(EffectId::DynLightFog)) o.effect = 0;   // KH_DLF.
            for (auto& f : fullscreen)
                if (f.second.effect == static_cast<int>(EffectId::Pulse) ||
                    f.second.effect == static_cast<int>(EffectId::Fogscatter) ||
                    f.second.effect == static_cast<int>(EffectId::DynLightFog)) f.second.effect = 0;   // KH_DLF.
        }

        // KH_FX_DEPTH_DROP: without the depth SRV, the depth readers whose picture needs it stand down for the frame
        // rather than read an unbound t1 (every pixel would decode at about the near plane). One rule,
        // kh_fx_depth_drop: every kh_fx_needs_depth reader except an unbanded, unlocalized outline (its luma edges
        // still draw) or user effect (it may read no depth). A fullscreen pass is dropped here; an effect MESH is
        // skipped by the draw loop below (the same predicate), not demoted to a solid. The UI lane drops EVERY depth
        // reader without its depth SRV (flush_ui_locked's wants_depth). The fog records a user effect sees mirror the
        // chain: the fog drops there.
        if (!depth_fx_ready) {
            for (auto& f : fullscreen) if (kh_fx_depth_drop(f.second)) f.second.effect = 0;
        }

        if (!effects_ready) {
            // Demote everything to solid for this frame; fullscreen passes
            // without an effect are meaningless - drop them.
            for (auto& o : meshes) o.effect = 0;
            fullscreen.clear();
            if (meshes.empty()) {
                // KH_INFRONT_TPL: an inFront-only cycle whose passes were all dropped here still owes the slice
                // this cycle's template - the template-only exit's publish (nothing drawn, nothing bound yet).
                if (khf_infront_tpl) {
                    ConstantData khf_tcb = {};
                    kh_fx_frame_template(khf_tcb, pv, view_proj, inv_view_proj, khf_fx_cam, khf_engcam, cam);
                    kh_fill_user_cam_cb(khf_tcb, cam, pv.view);   // KH_USER_LANES.
                    g_pip.tpl = khf_tcb;
                    g_pip.tpl_valid = true;
                    g_pip.tpl_cycle = g_topo_cycles;
                }
                return;
            }
        }
    }

    StateBackup backup;
    backup.capture(ctx);
    KhBindScope khf_bc(ctx);   // KH_BIND_FILTER.
    KhOmSlot0 khf_s0;   // KH_MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    khf_s0.open(ctx);
    kh_objbuf_sync(ctx);   // KH_OBJBUF: the flush's buckets index the records too.
    kh_objbuf_bind(ctx);
    kh_tex_async_pump(ctx, dev);   // KH_TEX_ASYNC: before any base is fixed or lane written.

    // Depth-sampling effects: the read-only DSV is swapped in so the live depth SRV may sit at PS t1 while depth
    // testing continues. Per draw, not per phase: it lands when the scene/depth pair is bound and the real DSV returns
    // for any depth-writing solid, so a TestWrite mesh keeps its write on frames that also carry a depth effect. The
    // depth SRV is never bound while the writable DSV is (t1 = atlas on the solid category).
    KhOmSave khf_om;
    bool khf_om_captured = false;
    bool khf_dsv_ro = false;
    auto khf_dsv_swap = [&](bool khfd_ro) {
        if (!depth_fx_ready || khfd_ro == khf_dsv_ro) return;
        if (!khf_om_captured) { khf_om.capture(ctx); khf_om_captured = true; }
        if (khfd_ro) khf_om.set_with_dsv(ctx, g_res.depth_dsv_ro);
        else         khf_om.restore(ctx);
        khf_dsv_ro = khfd_ro;
    };

    UINT stride = sizeof(MeshVertex), offset = 0;
    KhVbRef bound_vb;   // KH_CLOTH: seeded on the bind below (KH_SKIN_POOL: the pair).
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
    ID3D11Buffer* khf_cbs[2] = { g_res.constant_buffer, g_res.frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khf_cbs);
    ctx->PSSetConstantBuffers(0, 2, khf_cbs);
    // s0 for the textured per-submesh path; StateBackup's sampler slot restores.
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);

    ID3D11ShaderResourceView* ps_srvs[2] = {
        effects_ready ? g_res.scene_srv : nullptr,
        depth_fx_ready ? g_res.depth_srv : nullptr
    };
    // t1 here (FSAA off: a single-channel R24/R32 SRV over the live depth) and the PIP depth copy both land under the
    // effect unit's Texture2D<float2> depthTex; the arb snapshot (R32G32) under Texture2D<float> khArbSnap. The shader
    // reads .x only and the runtime does not validate return-type width; only the debug layer would warn.

    // t1 stays empty until the first pair consumer swaps the DSV (khf_dsv_swap): the writable DSV is bound here.
    ID3D11ShaderResourceView* khf_srvs_init[2] = { ps_srvs[0], nullptr };
    ctx->PSSetShaderResources(0, 2, khf_srvs_init);
    // Flush-time proven depth snapshot for the effect-mesh arbitration (t2, inside the StateBackup saved range;
    // PSEffect reads it only behind the local1.z gate).
    if (g_res.comp_depth_srv && g_res.comp_depth_samples == 1) {
        ctx->PSSetShaderResources(2, 1, &g_res.comp_depth_srv);
    }
    // Private sun-depth map (self term in PSMain), rendered earlier this frame (the cast fire, the injection, or this
    // flush's render_sun_depth above - a park's included) - t11, inside StateBackup's saved range.
    kh_sun_pf_convert(ctx);   // Pyramids convert here (proven binds).
    if (g_sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
    if (g_sun2_map_valid && g_res.sun2_srv) ctx->PSSetShaderResources(25, 1, &g_res.sun2_srv);
    if (g_sun3_map_valid && g_res.sun3_srv) ctx->PSSetShaderResources(26, 1, &g_res.sun3_srv);
    if (g_sun4_map_valid && g_res.sun4_srv) ctx->PSSetShaderResources(27, 1, &g_res.sun4_srv);
    if (g_sun5_map_valid && g_res.sun5_srv) ctx->PSSetShaderResources(32, 1, &g_res.sun5_srv);
    if (g_sun2_map_valid && g_sun_pf_valid[0] && g_res.sun_pf_srv[0]) ctx->PSSetShaderResources(29, 1, &g_res.sun_pf_srv[0]);
    if (g_sun3_map_valid && g_sun_pf_valid[1] && g_res.sun_pf_srv[1]) ctx->PSSetShaderResources(30, 1, &g_res.sun_pf_srv[1]);
    if (g_sun4_map_valid && g_sun_pf_valid[2] && g_res.sun_pf_srv[2]) ctx->PSSetShaderResources(31, 1, &g_res.sun_pf_srv[2]);
    if (g_sun5_map_valid && g_sun_pf_valid[3] && g_res.sun_pf_srv[3]) ctx->PSSetShaderResources(20, 1, &g_res.sun_pf_srv[3]);   // KH_FAR_PF.
    ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_SHADOW_GATHER: null = the default state, also clamp.
    // Analytic terrain heightfield (t10, saved range) - the flush twin.
    if (g_thm_valid && g_res.thm_srv) ctx->PSSetShaderResources(10, 1, &g_res.thm_srv);
    // The light depth array at t36, twin of the injection's bind and inside this pass's StateBackup range.
    if (g_res.dls_srv) {
        ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
    }
    for (UINT khf_bslot = 0; khf_bslot < 8; ++khf_bslot) {
        if (g_ls.band[khf_bslot].valid && g_ls.band[khf_bslot].srv) {
            const UINT khf_treg = khf_bslot < 6 ? 4 + khf_bslot : 12 + (khf_bslot - 6);
            ctx->PSSetShaderResources(khf_treg, 1, &g_ls.band[khf_bslot].srv);
        }
    }
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(g_res.blend_modes[0], bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
    ID3D11RasterizerState* khr_bound_rs = g_res.rasterizer;
    // Declared before the upload lambda so its [&] capture can see it; set just before the mesh pass (the count),
    // cleared by the loop's first perceptual capture failing (KH_FX_CAP_LAZY).
    bool khf_perceptual = false;
    bool khf_mm_cap = false;   // KH_BLEND_MINMAX: this object's own capture is bound at t3 (set per object);
    bool khf_mm_rep = false;   // and it draws with a replacing blend (a multisampled target).

    // Same floor/hold hysteresis on the flush's final encode near; requires the same-flush snapshot and the live
    // heightfield. Fail-safe: either down => stock path.
    bool khf_fx_arb_this = false;
    const float khf_arb_near = fabsf(pv.projection[2][2]) > 1e-9f
                             ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
    const bool khf_arb_snap_ok = g_res.comp_depth_srv && g_res.comp_depth_samples == 1 &&
                                 g_res.comp_depth_time >= 0.0f &&
                                 now - g_res.comp_depth_time < 0.5f;
    // The floor applies here too: the twin must agree with the injection or the flush repaint re-takes the step.
    const bool khf_arb_near_ok = khf_arb_near >= KH_FAR_ARB_NEAR_MIN || (g_flush_fx_arb_prev && khf_arb_near >= KH_FAR_ARB_NEAR_HOLD);
    const bool khf_fx_arb_frame = khf_arb_snap_ok && g_thm_valid && khf_arb_near_ok;

    g_flush_fx_arb_prev = khf_fx_arb_frame;

    // KH_REPLAY_SCISSOR: the scene was synced again above, so what this flush draws can differ from what the replay's
    // rectangles were made from; the merged textures are its own only where they cover it.
    if (g_rp_mir_pend) {   // KH_MIR_REPLAY: as the injection (this pass's projection is final above).
        for (const RenderObject& o : meshes) {
            if (kh_rp_mir_reads(o, pv.projection, cam)) { kh_rp_mir_finish(ctx); break; }
        }
    }
    bool khf_rp_vol = false, khf_rp_mir = false;
    kh_rp_covered(meshes, khf_rp_vol, khf_rp_mir);
    KhRpPassScope khf_rp_scope(khf_rp_vol, khf_rp_mir);
    // Deliberate twin of the injection build (khr_cbf); the per-object draw
    // tails of the two mesh paths are twins by the same decision.
    ConstantData khf_cbf = {};
    const bool khf_rebase_on = kh_fx_frame_template(khf_cbf, pv, view_proj, inv_view_proj, khf_fx_cam, khf_engcam, cam);
    kh_fill_user_cam_cb(khf_cbf, cam, pv.view);   // KH_USER_LANES.
    // The PIP paths (kh_pip_inject, kh_pip_fx) shade from a frame template the mesh injection publishes; a cycle with
    // no injection (no meshes) would leave them none. The flush's frame CB is the same template one publisher later,
    // and the PIP disarms what differs. The injection's copy, when there is one this cycle, stands.
    if (!g_pip.tpl_valid || g_pip.tpl_cycle != g_topo_cycles) {
        g_pip.tpl = khf_cbf;
        g_pip.tpl_valid = true;
        g_pip.tpl_cycle = g_topo_cycles;
    }
    const bool khf_frame_ok = kh_upload_frame_cb(ctx, g_res.frame_cb, khf_cbf);

    // The near-gap route, flush edition: the same gate, per-mesh verdict (its drawn box inside near + 2 m -
    // KH_NEARZ_BOUNDS, get_drawn - never the far route), lanes (fxMeta.x = near, fxMeta.y = the gap floor), viewport
    // swap (MinDepth = floor for that draw) and pixel variant (the ARB twin, whose SV_Depth ramp spreads the gap
    // fragments from the floor up to the viewport min).
    const float khf_mesh_vp_lo = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
    const float khf_mesh_vp_hi = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
    const float khf_nz_A = pv.projection[2][2];
    const float khf_nz_B = pv.projection[3][2];
    const float khf_nz_near = (khf_nz_A > 1.0e-6f) ? (-khf_nz_B / khf_nz_A) : 0.0f;
    const bool khf_nz_on = g_res.ps_composite_arb != nullptr && khf_nz_near > KH_CAM_NEAR_MIN && khf_nz_near < 50.0f && khf_mesh_vp_lo > 1.0e-5f && khf_mesh_vp_lo < khf_mesh_vp_hi && khf_mesh_vp_hi <= 1.0f;
    const float khf_nz_gap_lo = khf_mesh_vp_lo * KH_NEARZ_GAP_FRAC;
    // Asked by upload_cb for the object and by the bucket plan's key (kh_inst_plan_build) for every candidate member.
    auto khf_nz_verdict = [&](const RenderObject& khnv_o) -> bool {
        return khf_nz_on &&
               g_dist_memo_flush.get_drawn(khnv_o, cam) < (khf_nz_near + 2.0f) * (khf_nz_near + 2.0f);
    };
    bool khf_nearz_draw = false;   // per-mesh verdict (written by upload_cb).
    // Buckets, flush edition: the pass verdict (twins + object buffer present) every bucket decision on this pass
    // reads, and the slot-1 bind latch. Twin at the injection.
    const bool khf_inst_on = kh_inst_twins_on();
    static KhInstPlan khf_inst_plan;
    bool khf_inst_vb_bound = false;
    // Hoisted per-object CB: the textured per-submesh path refills the mat lanes and uploads after upload_cb returns
    // (KH_TX_ONE_UPLOAD: a textured object's only upload), so the object slice must outlive the lambda.
    ConstantData khf_obj_cbd = {};
    // KH_GLOW_PYR, effect meshes: their source is the scene capture at t0, which the loop's re-captures rewrite
    // (KH_FX_CAP_ORDER; khf_cap_gen counts the captures this flush takes after the effects' own). A glow mesh
    // builds glow_pyr[0] once per capture and threshold (the key below) and upload_cb arms its lanes
    // (khf_glow_draw); the chain after the meshes builds its own.
    uint64_t khf_cap_gen = 0;
    uint64_t khf_glow_gen = ~0ull;
    bool khf_glow_b = false, khf_glow_ok = false;
    float khf_glow_t = 0.0f;
    const KhGlowPyr* khf_glow_draw = nullptr;
    // KH_ANA_PYR, effect meshes: glow_pyr[0]'s rule for anamorphic's one-axis pyramid, per axis - built once per
    // capture and threshold, its lanes armed by upload_cb (khf_ana_draw).
    uint64_t khf_ana_gen[2] = { ~0ull, ~0ull };
    bool khf_ana_ok[2] = { false, false };
    float khf_ana_t[2] = { 0.0f, 0.0f };
    const KhAnaPyr* khf_ana_draw = nullptr;
    ID3D11ShaderResourceView* khf_utex[KH_MAT_USER_MAPS] = {};   // KH_FX_TEX: an effect mesh's set (upload_cb).
    bool khf_t43 = false;   // KH_FX_TEX_ALL: khf_utex's pages are bound (the last effect mesh drawn had a set).
    // khf_defer non-null = build only (deferred chain draw - the caller uploads the returned slice at flush time; the
    // frame slice already went up at the pass build - or a textured object, whose slice kh_draw_textured uploads per
    // submesh: KH_TX_ONE_UPLOAD).
    auto upload_cb = [&](const RenderObject& o, bool chain_pass,
                         ConstantData* khf_defer = nullptr) -> bool {
        khf_nearz_draw = false;   // KH_FLUSH_NEARZ: per-call default; solid section may set it.
        ConstantData& cbd = khf_defer ? *khf_defer : khf_obj_cbd;
        cbd = khf_cbf;   // CB split: frame template (matrices ride for the echoes).
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        cbd.center_size[3] = chain_pass ? 1.0f : 0.0f;
        // Per-object rebase arm mirrors the pass verdict (one without the other
        // is the wrong-space landmine). Chain passes carry it inertly.
        float khf_res[3];   // KH_POS_RES: the object's live residual.
        kh_pos_res(o, khf_res);
        if (khf_rebase_on) kh_fill_center_rel(cbd, cam, khf_res);
        cbd.size_axes[0] = o.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y].
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        kh_fill_obj_rot(cbd, o.rot_m);
        // A normal-blend translucent is never bucketed, so the perceptual arm
        // is decided by the object alone. KH_FX_PERC_FX: never an effect mesh - PSEffect reads neither blendCtl.x
        // nor t3 as the scene (khf_perc_count's rule).
        cbd.blend_ctl[0] = ((khf_perceptual && !o.fullscreen && o.draw_part != 1 && o.effect == 0 &&
                             o.blend_mode == 0 && o.color[3] < 0.999f) ||
                            (khf_mm_cap && kh_blend_minmax_exact(o))) ? 1.0f : 0.0f;   // KH_MAT_BLEND parts:
                                                                                       // Never perceptual.
        // blend_ctl[3] is the crossfade dither; written here so it is never inherited from the previous object's fade.
        cbd.blend_ctl[3] = 0.0f;
        kh_fill_fx_params_cb(cbd, o);
        if (!chain_pass && khf_glow_draw) kh_glow_lanes(cbd, khf_glow_draw);   // KH_GLOW_PYR.
        if (!chain_pass && khf_ana_draw) kh_ana_lanes(cbd, khf_ana_draw);   // KH_ANA_PYR.

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
        cbd.fx_meta[1] = kh_fx_time(o, now_d);
        cbd.fx_meta[2] = screen_w;
        cbd.fx_meta[3] = screen_h;
        cbd.depth_params[0] = pv.projection[2][2];
        cbd.depth_params[1] = pv.projection[3][2];
        // KH_FLUSH_DEPTH_RANGE: the range the engine wrote the scene's depth through and every mesh viewport of this
        // flush draws in (khf_mesh_vp_lo / hi, the same trigger-or-scene pair), as the injection fills it - the ARB
        // twin's SV_Depth and LinDepth decode with it. Not the entry viewport's range: a park landing in the view
        // model's slice would put every near-gap fragment at the gap floor, in front of the world.
        kh_fill_depth_range_cb(cbd);

        // depthParams stays the live-depth pair for LinDepth/WorldPos; local0/local1.zw carry the guard's channel once
        // the masks have written.

        // Without a fresh snapshot: guard stands down.
        if (o.effect == 0) {
            // per-mesh near-gap verdict (khf_nz_verdict; KH_FLUSH_NEARZ).
            khf_nearz_draw = !chain_pass && khf_nz_verdict(o);
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

                if (g_ro.slot_near_live >= KH_CAM_NEAR_MIN &&
                    g_ro.slot_near_live <= KH_CAM_NEAR_MAX &&
                    kh_enc_far(g_ro.slot_m22, g_ro.slot_m32) <= KH_ENC_FAR_MAX) {
                    cbd.depth_params[0] = g_ro.slot_m22;
                    cbd.depth_params[1] = g_ro.slot_m32;
                }

                kh_fill_depth_range_cb(cbd);
                // Twin: both CB editions carry it.
                                cbd.fx_meta[2] = static_cast<float>(g_res.comp_depth_w);
                cbd.fx_meta[3] = static_cast<float>(g_res.comp_depth_h);

                const bool exact_flush = measured ||
                    (g_ro.slot_near_live >= KH_CAM_NEAR_MIN &&
                     g_ro.slot_near_live <= KH_CAM_NEAR_MAX &&
                     kh_enc_far(g_ro.slot_m22, g_ro.slot_m32) <= KH_ENC_FAR_MAX);
                cbd.fx1[0] = exact_flush ? KH_COMPOSITE_GUARD_BASE_MEASURED : KH_COMPOSITE_GUARD_BASE;
                cbd.fx1[1] = exact_flush ? KH_COMPOSITE_GUARD_REL_MEASURED : KH_COMPOSITE_GUARD_REL;
            } else {
                cbd.fx1[0] = 1e9f;
                cbd.fx1[1] = 0.0f;
            }
            // KH_FLUSH_ARB_LANES: fxParams1.zw are the ARB twin's analytic clamp (the far route's cap and contact
            // band; zero = no terrain pull), which kh_fill_fx_params_cb filled from the object's params[6..7] - a
            // solid given seven or more params pulled its depth near terrain on this route alone. The injection's
            // near-gap route leaves them zero; so does this one. PSMain never reads them.
            cbd.fx1[2] = 0.0f;
            cbd.fx1[3] = 0.0f;
            // The effect id / time this pair carries for effect meshes is never read by a solid draw.
            if (khf_nearz_draw) {
                cbd.fx_meta[0] =
                    khf_nz_near;
                cbd.fx_meta[1] = khf_nz_gap_lo;
            }
            // Armed once at the pass frame build (both arms; the terrain lane
            // never depends on the depth lane's arming state).
        }
        kh_fill_local_band_cb(cbd, o);
        khf_fx_arb_this = false;

        if (o.effect > 0 && khf_fx_arb_frame && !o.fullscreen) {
            float kha_m22 = pv.projection[2][2];
            float kha_m32 = pv.projection[3][2];

            if (g_ro.engine_proj_valid) { kha_m22 = g_ro.engine_m22; kha_m32 = g_ro.engine_m32; }
            if (g_ro.slot_near_live >= KH_CAM_NEAR_MIN &&
                g_ro.slot_near_live <= KH_CAM_NEAR_MAX &&
                kh_enc_far(g_ro.slot_m22, g_ro.slot_m32) <= KH_ENC_FAR_MAX) { kha_m22 = g_ro.slot_m22; kha_m32 = g_ro.slot_m32; }
            cbd.local0[0] = kha_m22;
            cbd.local0[1] = kha_m32;
            cbd.local0[2] = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
            cbd.local0[3] = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
            cbd.local1[2] = KH_FAR_ARB_GUARD_BASE;   // Arm flag + base, one lane.
            cbd.local1[3] = KH_FAR_ARB_GUARD_REL;
            khf_fx_arb_this = true;
        }
        fill_lighting_obj_cb(ctx, cbd, o);
        if (!chain_pass && kh_fx_tex_on(o)) kh_fx_tex_fill(dev, ctx, cbd, o, khf_utex);   // KH_FX_TEX(_ALL).
        // A failed frame upload stands every draw down: fail dark, never stale-frame draws.
        if (khf_defer) return khf_frame_ok;
        return khf_frame_ok && kh_upload_obj_cb(ctx, g_res.constant_buffer, cbd);
    };

    // Front-to-back lets each opaque lay depth that the ones behind it then
    // fail, so an occluded fragment never enters the shadow ladder.
    {   // KH_MESH_ORDER: key sort, same order, one distance per object.
        kh_mesh_order(meshes, cam, g_dist_memo_flush);   // The memo begins here, seeded; the verdicts, the
                                                         // emit, the tail sort and the pick read it.
    }

    KhCullSet khf_cull;
    kh_cull_build(view_proj, cam, khf_cull);
    khf_cull.back_on = true;   // KH_CULL_BACK: twin of the injection.
    // The flush's one visibility truth (frustum only - occlusion culling is an injection concern: only there does the
    // pass-wide depth-write contract make a cull provably output-identical). The KH_INST_RUN cut, the material ensure,
    // the plan's emit and the per-object loop all call this lambda; memoised per object.
    g_vis_memo_flush.begin(meshes);
    auto khf_vis_calc = [&](const RenderObject& khfv_o) -> bool {
        float khfv_cc[3];
        kh_obj_center_engine(khfv_o, khfv_cc);
        float khfv_bs[3];
        kh_bounds_size_of(khfv_o, khfv_bs);   // KH_CAST_REFIT: the cull's size; the LOD pick keeps size.
        return kh_mesh_visible(khf_cull, khfv_cc, kh_lod_radius_of(khfv_bs, khfv_o.rot_m, khfv_o.rotated));
    };
    auto khf_vis = [&](const RenderObject& khfv_o) -> bool {
        return g_vis_memo_flush.get(khfv_o, khf_vis_calc);
    };

    uint32_t khf_perc_count = 0;

    // A translucent draw is a whole translucent object on normal blend or an object's blend part (draw_part 1). One
    // predicate for the sort; the capture demand below is the whole-object case only.
    // KH_FX_PERC_FX: a fading effect mesh (normal blend, alpha below 0.999: a duration fade or a colour alpha) sorts
    // into the tail with it - it writes no depth, and the tail's far-to-near order is the one its alpha blend wants;
    // kept there so a fade's start moves nothing - but is no perceptual translucent (below, khf_o_perc, blend_ctl.x).
    auto khf_transl = [](const RenderObject& o) -> bool {
        return !o.fullscreen && (o.draw_part == 1 || (o.blend_mode == 0 && o.color[3] < 0.999f));
    };
    // Blend parts are hardware-blended, never perceptual: the perceptual composite lerps against the capture and writes
    // opaque, so overlapping fragments of one draw (the near and far walls of a tube) do not accumulate. A whole
    // translucent object is one hull composited once; a material part is exactly the multi-layer case.
    // KH_FX_PERC_FX: nor is an effect mesh - PSEffect reads neither blendCtl.x nor t3 as the scene (its t3 is the
    // glow / anamorphic pyramid, bound per object), so counting one armed a capture and a t3 bind it never read: one
    // scene resolve a frame per fading effect mesh past the first. Its own source, the capture at t0, is retaken by
    // KH_FX_CAP_ORDER's rule in the loop.
    for (const auto& o : meshes) {
        if (!o.fullscreen && o.draw_part != 1 && o.effect == 0 && o.blend_mode == 0 && o.color[3] < 0.999f)
            // A normal-blend translucent is never bucketed, so the perceptual demand counts objects directly.
            khf_perc_count++;
    }

    // Sorting is per draw, not per triangle: a self-overlapping translucent
    // still relies on the ordered two-sided pass below.
    static std::vector<uint32_t> khf_order;
    khf_order.clear();
    size_t khf_tail_n = 0;   // KH_SSAO: where the translucent tail begins.
    // KH_WSEE: the injection's deferred parts (ws_dcopy: KH_WSEE_DEFER_FLAG) draw first, far to near, ahead of this
    // flush's own draws (its solids, effect meshes and tail drew after them when the injection drew them, and still
    // do); they count as tail entries for the sort (khf_front) and the T / NW replay (khf_ws_tail).
    size_t khf_def_n = 0;
    {
        static std::vector<std::pair<float, uint32_t>> khf_tail;
        static std::vector<std::pair<float, uint32_t>> khf_front;
        static std::vector<uint32_t> khf_sol;
        khf_tail.clear();
        khf_front.clear();
        khf_sol.clear();
        for (uint32_t khf_oi = 0; khf_oi < static_cast<uint32_t>(meshes.size()); ++khf_oi) {
            const bool khf_dc = meshes[khf_oi].ws_dcopy != 0u;   // KH_WSEE_DEFER_FLAG.
            if (khf_transl(meshes[khf_oi])) {
                (khf_dc ? khf_front : khf_tail).emplace_back(g_dist_memo_flush.get(meshes[khf_oi], cam), khf_oi);
            } else {
                khf_sol.push_back(khf_oi);
            }
        }
        const auto khf_far_first = [](const std::pair<float, uint32_t>& a, const std::pair<float, uint32_t>& b) {
            return a.first > b.first;
        };
        std::stable_sort(khf_tail.begin(), khf_tail.end(), khf_far_first);
        std::stable_sort(khf_front.begin(), khf_front.end(), khf_far_first);
        khf_tail_n = khf_tail.size();
        khf_def_n = khf_front.size();
        for (const auto& khf_t : khf_front) khf_order.push_back(khf_t.second);
        khf_order.insert(khf_order.end(), khf_sol.begin(), khf_sol.end());
        for (const auto& khf_t : khf_tail) khf_order.push_back(khf_t.second);
    }
    // KH_SSAO: the late-drawn solids' term runs once, before the first translucent (the tail above), so no blend draw
    // takes it; after the loop when there is none. On the WRITABLE DSV: a pair consumer swaps in the read-only view,
    // whose stencil discards the eraser's ZERO, and a mark that survives the park is taken again by the next injection
    // at pixels it never painted.
    const size_t khf_solid_n = khf_order.size() - khf_tail_n;
    bool khf_ssao_done = false;

    // KH_INST_RUN: a bucket draws every member at its representative (the first visible one in khf_order; for a
    // blend mode other than normal the farthest), so whatever khf_order puts between two members is drawn after all
    // of them. Only a draw on the same commutative blend (additive, multiply, screen, lighten / darken at alpha 1;
    // testing depth without writing it, reading no capture) gives the same picture in either order. So the solid
    // section is cut into runs in drawing order: a run is a stretch of visible draws on one commutative blend, each
    // breaking draw a run of its own (a blended mesh writing depth buckets alone: a bucket's ranges are level-major).
    // The run rides the key's route above its near-gap bit, so a bucket never spans a draw it does not commute with.
    // Normal-blend opaque solids (kh_ord_group 0) keep run 0; a culled object cuts nothing.
    static std::vector<uint32_t> khf_inst_run;
    if (khf_inst_on) {
        khf_inst_run.assign(meshes.size(), 0u);
        uint32_t khf_run_id = 0u;
        int khf_run_bm = -1;   // The open run's blend mode; -1 = none open.
        for (size_t khf_run_x = 0; khf_run_x < khf_solid_n; ++khf_run_x) {
            const uint32_t khf_run_i = khf_order[khf_run_x];
            const RenderObject& khf_run_o = meshes[khf_run_i];
            if (kh_ord_group(khf_run_o) == 0u) continue;
            if (!khf_run_o.fullscreen && !khf_vis(khf_run_o)) continue;
            const bool khf_run_c = khf_run_o.blend_mode != 0 && khf_run_o.effect == 0 && !khf_run_o.fullscreen &&
                                   khf_run_o.draw_part != 1 && khf_run_o.mode == DepthMode::TestOnly &&
                                   !kh_blend_minmax_exact(khf_run_o);
            if (!khf_run_c || khf_run_o.blend_mode != khf_run_bm) ++khf_run_id;
            khf_inst_run[khf_run_i] = khf_run_id;
            khf_run_bm = khf_run_c ? khf_run_o.blend_mode : -1;
        }
    }

    if (khf_inst_on) {
        kh_mat_ensure_all(ctx, dev, meshes, khf_vis);   // KH_MAT_TABLE: bases fixed before any lane is written.
        kh_inst_plan_build(khf_inst_plan, meshes, khf_order.data(), khf_order.size(),
                           [&](const RenderObject& khfp_o, KhInstKey& khfp_k) {
                               const size_t khfp_i = static_cast<size_t>(&khfp_o - meshes.data());
                               khfp_k.route = (khf_nz_verdict(khfp_o) ? 2u : 0u) |
                                              (khf_inst_run[khfp_i] << 2);   // KH_INST_RUN.
                               khfp_k.aux = kh_dl_bucket_hash(khfp_o);
                           });
        kh_inst_plan_emit(ctx, dev, khf_inst_plan, meshes, cam, 0, khf_vis, g_dist_memo_flush);   // KH_BUCKETS: the cull walk, once.
    }

    khf_perceptual = khf_perc_count > 0;
    // KH_FX_CAP_LAZY: the perceptual translucents' first capture is taken in the loop by the first of them
    // (khf_cap_stale starts true), not here: a draw of ours ahead of it (a deferred part, a solid, an effect mesh,
    // the SSAO apply) makes a capture taken here stale, and KH_FX_CAP_ORDER's rule takes it again before that
    // translucent - two scene resolves a flush where one serves (every flush with any draw of ours ahead of its first
    // perceptual translucent). Its failure stands the perceptual composite down for the flush (khf_cap_t3
    // false: no capture of this flush is at t3 yet). The per-draw re-capture before a perceptual translucent or an
    // effect mesh is KH_FX_CAP_ORDER's (the loop below).

    int khf_ps_cat = -1;   // 0 solid / 1 effect / 2 arb / 3 textured / 4 textured arb
                           // (KH_FLUSH_NEARZ).
    ID3D11VertexShader* khf_bound_vs = g_res.vs;
    ID3D11InputLayout* khf_bound_il = g_res.input_layout;
    ID3D11ShaderResourceView* khf_bound_sk = nullptr;   // KH_SKIN_POOL: VS t52 (kh_inst_sk_bind).
    // KH_FX_CAP_ORDER: the scene capture (g_res.scene_srv - t0 of the effect meshes, t3 of the perceptual
    // translucents) is the picture when it was taken, and a reader writes over the live target (an effect mesh
    // effect(capture) at its alpha, a perceptual translucent its mix opaque), erasing every draw of ours since then
    // under its silhouette. Two flags, cleared by every capture: khf_cap_stale (a draw of ours since the capture) is
    // read before a perceptual translucent, khf_cap_stale_ne (one that was not an effect mesh) before an effect mesh;
    // a stale flag takes the capture again first. An effect mesh over an effect mesh keeps the capture. The SSAO post
    // at the tail's start is a draw of ours (its apply darkens the solids).
    // KH_FX_CAP_LAZY: khf_cap_stale starts true; the effect meshes' capture is the setup's, taken above (khf_cap_defer
    // is false whenever the flush has an effect mesh, so the chain's deferred copy is never theirs).
    bool khf_cap_stale = true;
    bool khf_cap_stale_ne = false;
    bool khf_cap_t3 = false;   // KH_FX_CAP_LAZY: a capture taken by this loop stands at t3 (any of the sites below).
    int khf_srv_cat = -1;   // 0 = comp-depth t0, 1 = scene/depth pair, 2 = null t0 (no snapshot).
    int khf_bound_bm = -1;   // Blend mode last set.
    ID3D11DepthStencilState* khf_bound_dss = nullptr;
    UINT khf_bound_ref = 0;   // KH_SSAO_SEE_THROUGH: the stencil reference bound with khf_bound_dss.

    UINT khf_n_saved_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT khf_saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&khf_n_saved_vp, khf_saved_vp);

    D3D11_VIEWPORT khf_vp = {};   // The mesh viewport (hoisted: KH_FLUSH_NEARZ swaps against it per
                                  // Draw).
    {
        if (khf_n_saved_vp >= 1) {
            khf_vp = khf_saved_vp[0];
        } else {
            khf_vp.Width = static_cast<FLOAT>(g_main_depth_w);
            khf_vp.Height = static_cast<FLOAT>(g_main_depth_h);
        }

        khf_vp.MinDepth = khf_mesh_vp_lo;   // (one truth with the near-gap gate above).
        khf_vp.MaxDepth = khf_mesh_vp_hi;
        ctx->RSSetViewports(1, &khf_vp);
    }
    // KH_SSAO: this park draws the injection's meshes late (comp_healthy false), so it is a drawer and takes the term
    // for what it marks - its own depth writes alone (KH_SSAO_MARK), whichever side of the landing it fell on.
    // Translucents and effect meshes write no depth and leave no mark.
    KhSsaoPass khf_ssao;
    if (!comp_healthy && khf_comp_eligible > 0 && (kh_ao_strength() > 0.0f || kh_nzm_wanted())) {   // KH_NEARZ_MARK.
        kh_ssao_rect(pv, khf_ssao.rect);
        kh_ssao_pre(dev, ctx, khf_ssao, pv.projection, khf_vp.MinDepth, khf_vp.MaxDepth);
        khf_ssao.nz_near = khf_nz_on ? khf_nz_near : 0.0f;   // KH_SSAO_NEARZ: as the injection.
        khf_ssao.nz_gap = khf_nz_gap_lo;
    }
    // The gap viewport (MinDepth = the floor the routed draw's ramp starts from) and the per-draw swap tracker - the
    // injection's khr_vp_select, flush edition: 0 = the mesh viewport, 1 = the near-gap.
    D3D11_VIEWPORT khf_nz_vp = khf_vp;
    if (khf_nz_on) khf_nz_vp.MinDepth = khf_nz_gap_lo;
    int khf_vp_bound = 0;
    auto khf_vp_select = [&](int khfv_want) {
        if (khfv_want == khf_vp_bound) return;
        ctx->RSSetViewports(1, khfv_want == 1 ? &khf_nz_vp : &khf_vp);
        khf_vp_bound = khfv_want;
    };

    // KH_WSEE: this run's sort mode for the tail's parts (kh_wsee_sort_mode) and the main depth's decode the parts'
    // tests take (fx_fog[0] on a sorted draw: the pair and the world range, kh_fill_depth_range_cb's; a fragment's own
    // view depth, SV_Position.w, needs no encode). The pair is this flush's own (pv: the injection's encode, which our
    // solids in S were written with and which mode 0's hardware test compares against); mode 0 where the engine's
    // latched pair's near differs from this pair's by more than 2e-3 of it, relative (kh_wsee_pair_split; a partitioned
    // cycle: S and a later partition's G = 1 pixels hold raws of two encodes, which no single decode reads).
    const bool khf_ws_any = khf_def_n > 0 || khf_order.size() > khf_solid_n;
    const int khf_ws_mode = khf_ws_any ? kh_wsee_sort_mode(ctx, pv.projection[2][2], pv.projection[3][2]) : 0;
    float khf_ws_dec[4] = { pv.projection[2][2], pv.projection[3][2], 0.0f, 1.0f };
    if (khf_ws_mode > 0) {
        ConstantData khf_ws_rg = {};
        kh_fill_depth_range_cb(khf_ws_rg);
        khf_ws_dec[2] = khf_ws_rg.depth_params[2];
        khf_ws_dec[3] = khf_ws_rg.depth_params[3];
    }
    // Q, taken at the run's first part that composites (0 = not yet, 1 = taken, -1 = refused: the run's
    // composite modes take mode 1).
    int khf_ws_q = 0;

    for (size_t khf_ix = 0; khf_ix < khf_order.size(); ++khf_ix) {
        const uint32_t khf_oi = khf_order[khf_ix];
        // KH_WSEE: past the injection's deferred parts, the scene capture this flush's effect meshes or perceptual
        // translucents read is taken again, so they see our parts behind them as they did when the injection drew
        // those - by KH_FX_CAP_ORDER's flags, which every drawn part sets (the loop's end), before the first reader
        // (KH_FX_CAP_LAZY: not here, where a solid or the SSAO apply between would stale a capture taken now again).
        // KH_WSEE: the flush's own tail takes a fresh Q - its solid section (effect meshes, other
        // blends) drew since the deferred parts' one.
        if (khf_ix == khf_solid_n && khf_def_n > 0 && khf_ws_q == 1) khf_ws_q = 0;
        if (khf_ix == khf_solid_n && !khf_ssao_done) {   // KH_SSAO: the tail starts here.
            khf_ssao_done = true;
            if (khf_ssao.on) {
                // t1 down before the writable DSV comes back (a live depth SRV bound against its writable view is
                // unbound by the runtime silently, and the category tracker would not know); the next draw rebinds its
                // pair whole.
                ID3D11ShaderResourceView* khf_t1_none = nullptr;
                ctx->PSSetShaderResources(1, 1, &khf_t1_none);
                khf_dsv_swap(false);
                kh_ssao_post(ctx, khf_ssao);
                khf_srv_cat = -1;
                if (khf_ssao.term) {   // KH_FX_CAP_ORDER: the term's apply drew over our solids.
                    khf_cap_stale = true;
                    khf_cap_stale_ne = true;
                }
            }
        }
        const RenderObject& o = meshes[khf_oi];
        // Carried by its bucket's draw at the representative, or culled by the
        // emission (never an overlay: mode Off is not eligible).
        if (khf_inst_on && khf_inst_plan.done[khf_oi]) continue;
        // Fullscreen passes have no world extent and are never culled. Twin at the injection.
        if (!o.fullscreen) {
            // khf_vis is the emit's twin, so an emitted bucket representative always draws here.
            if (!khf_vis(o)) {
                continue;
            }
        }
        // KH_FX_DEPTH_DROP: an effect mesh whose picture needs the depth, on a frame without the depth SRV, draws
        // nothing (kh_fx_depth_drop - the chain's rule) rather than read an unbound t1.
        if (!depth_fx_ready && o.effect > 0 && kh_fx_depth_drop(o)) continue;
        // Before upload_cb (which may skip): a skipped mesh drew nothing, so no refresh is owed. Blend parts: hardware
        // blend, never perceptual. KH_FX_PERC_FX: nor effect meshes.
        const bool khf_o_perc = khf_transl(o) && o.draw_part != 1 && o.effect == 0;
        // KH_BLEND_MINMAX: a lighten / darken mesh composites against its own copy of the scene as it stands right
        // before it, bound at t3 for this object's draws only (khf_mmb puts t3 back at the iteration's end). The shared
        // capture - t3 of the perceptual translucents, t0 of the effect meshes - is not touched, so what they show does
        // not depend on where a lighten / darken mesh falls in the order. On a multisampled target the draw replaces
        // (khf_mm_rep; the twins write alpha 1), as the perceptual composite does: MIN / MAX would apply per sample
        // against samples the resolved copy averaged. Where the mesh is opaque over a scene edge that keeps the pixel's
        // value, not each sample's max / min of it, and a self-overlapping mesh keeps its last face there.
        khf_mm_cap = false;
        khf_mm_rep = false;
        KhMmBind khf_mmb;
        if (kh_blend_minmax_exact(o)) {
            bool khfm_ms = false;
            if (kh_mm_capture(dev, ctx, khfm_ms) && khf_mmb.bind(ctx, g_res.mm_srv)) {
                khf_mm_cap = true;
                khf_mm_rep = khfm_ms;
            }
        }

        // KH_FX_CAP_ORDER: a perceptual translucent composites against the scene as it stands - a draw of ours
        // since the capture (a translucent, a blended mesh, a part, a fading effect mesh, the SSAO post) is in it.
        // KH_FX_CAP_LAZY: the flush's first (khf_cap_stale starts true); its failure before any capture stood at t3
        // stands the perceptual composite down for the flush - this translucent and the rest draw plain (blend_ctl.x,
        // read by upload_cb below); a later failure keeps the capture that stands.
        if (khf_perceptual && khf_o_perc && khf_cap_stale) {
            ++khf_cap_gen;   // KH_GLOW_PYR.
            if (kh_scene_capture_timed(dev, ctx).empty()) {
                ctx->PSSetShaderResources(3, 1, &g_res.scene_srv);
                khf_srv_cat = -1;   // t0/t1 pair may have been rebound; force re-swap below.
                khf_cap_stale = false;
                khf_cap_stale_ne = false;
                khf_cap_t3 = true;
            } else if (!khf_cap_t3) {
                khf_perceptual = false;
            }
        }
        // Custom effect (user .hlsl): resolve the compiled PS first - a failed or missing compile skips the draw
        // (reported once) rather than feeding the sentinel id into the builtin switch.
        ID3D11PixelShader* khf_ufx = nullptr;
        bool khf_upyr = false;   // KH_FX_USER.

        if (o.effect == KH_EFFECT_CUSTOM) {
            khf_ufx = kh_user_fx_ps(dev, kh_fx_shader_of(o), &khf_upyr);
            if (!khf_ufx) continue;
        }

        if (o.effect == KH_EFFECT_LUT) {
            ID3D11ShaderResourceView* khf_lut = kh_user_lut_srv(dev, kh_fx_shader_of(o));
            if (!khf_lut) continue;
            ctx->PSSetShaderResources(19, 1, &khf_lut);
        }

        // KH_FX_CAP_ORDER: an effect mesh's source (t0) is the scene as it stands - a blended mesh, a part, a
        // translucent or a solid of this flush drawn since the capture is in it (an effect mesh is not: see the
        // flags' note). Past the skips above (a skipped mesh reads nothing), before the pyramids built from it.
        if (o.effect > 0 && !o.fullscreen && khf_cap_stale_ne) {
            ++khf_cap_gen;   // KH_GLOW_PYR: the pyramids follow the capture.
            if (kh_scene_capture_timed(dev, ctx).empty()) {
                ctx->PSSetShaderResources(3, 1, &g_res.scene_srv);
                khf_srv_cat = -1;   // t0/t1 pair may have been rebound; force re-swap below.
                khf_cap_stale = false;
                khf_cap_stale_ne = false;
                khf_cap_t3 = true;   // KH_FX_CAP_LAZY.
            }
        }
        // KH_GLOW_PYR: before upload_cb, whose slice the build's own upload would replace.
        khf_glow_draw = nullptr;
        if ((!khf_ufx && kh_glow_effect(o.effect)) || (khf_ufx && khf_upyr)) {   // KH_FX_USER: or a user mesh's.
            const bool khfg_b = kh_glow_bright(o.effect);
            const float khfg_t = khfg_b ? o.fx[0] : 0.0f;
            if (khf_glow_gen != khf_cap_gen || khf_glow_b != khfg_b || khf_glow_t != khfg_t) {
                khf_glow_ok = kh_glow_build(dev, ctx, g_res.glow_pyr[0], ps_srvs[0], screen_w, screen_h, 0.0f,
                                            o.effect, o.fx[0], g_res.constant_buffer);
                khf_glow_gen = khf_cap_gen;
                khf_glow_b = khfg_b;
                khf_glow_t = khfg_t;
            }
            if (khf_glow_ok) khf_glow_draw = &g_res.glow_pyr[0];
        }
        khf_ana_draw = nullptr;   // KH_ANA_PYR: the same rule, per axis.
        if (!khf_ufx && o.effect == static_cast<int>(EffectId::Anamorphic)) {
            const int khfa_ax = o.fx[4] > 0.5f ? 1 : 0;   // fxParams1.x (kh_fill_fx_params_cb).
            const float khfa_t = o.fx[0];
            if (khf_ana_gen[khfa_ax] != khf_cap_gen || khf_ana_t[khfa_ax] != khfa_t) {
                khf_ana_ok[khfa_ax] = kh_ana_build(dev, ctx, g_res.ana_pyr[0][khfa_ax], ps_srvs[0], screen_w, screen_h,
                                                   0.0f, khfa_ax, khfa_t, g_res.constant_buffer);
                khf_ana_gen[khfa_ax] = khf_cap_gen;
                khf_ana_t[khfa_ax] = khfa_t;
            }
            if (khf_ana_ok[khfa_ax]) khf_ana_draw = &g_res.ana_pyr[0][khfa_ax];
        }
        // Textured routing (flush edition): textured solids take the PSMain/VSMain twins + the 4-element layout. Twins
        // missing => untextured fallback.
        const KhMaterialSet* khf_txm = kh_obj_textured(o);
        const bool khf_tx_on = khf_txm != nullptr &&
            g_res.ps_tex && g_res.vs_tex && g_res.layout_tex && g_res.mat_sampler;
        // KH_TX_ONE_UPLOAD: textured, the slice is built here and goes up per submesh in kh_draw_textured (the
        // build-only form: upload_cb fills khf_obj_cbd and returns the frame verdict). Twin: the injection.
        if (!(khf_tx_on ? upload_cb(o, false, &khf_obj_cbd) : upload_cb(o, false))) continue;
        KhGlowBind khf_gb;   // KH_GLOW_PYR: t3 / s2 for this object's draws, put back at the iteration's end.
        if (khf_glow_draw) khf_gb.bind(ctx, *khf_glow_draw);
        KhAnaBind khf_ab;   // KH_ANA_PYR: t3 / t6, likewise.
        if (khf_ana_draw) khf_ab.bind(ctx, *khf_ana_draw);
        // A translucent part exists only through the textured twins - untextured, the mesh has no submesh table to
        // split and no sampled alpha, so the part is skipped rather than drawn whole.
        if (o.draw_part == 1 && !khf_tx_on) {  continue; }
        // A near-gap draw takes the ARB twin - textured (ps_comp_arb_tex) or plain (ps_composite_arb).
        const bool khf_nz_tex_ok = khf_tx_on && g_res.ps_comp_arb_tex != nullptr;
        const bool khf_nz_this = khf_nearz_draw && (khf_tx_on ? khf_nz_tex_ok : true);
        const bool khf_arb_this = khf_nz_this;
        const int khf_ps_want = o.effect > 0 ? 1 : (khf_arb_this ? (khf_tx_on ? 4 : 2) : (khf_tx_on ? 3 : 0));

        if (khf_ufx) {
            // Custom PS per object: bind unconditionally and poison the
            // category so the next builtin object rebinds its own.
            ctx->PSSetShader(khf_ufx, nullptr, 0);
            ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_FX_USER: s1 for every user effect draw.
            kh_fx_tex_bind(ctx, khf_utex);   // KH_FX_TEX: this object's (upload_cb filled them); off at the next one.
            khf_t43 = true;
            khf_ps_cat = -1;
        } else {
            if (khf_ps_want != khf_ps_cat) {
                ctx->PSSetShader(khf_ps_want == 1 ? g_res.ps_effect
                               : khf_ps_want == 2 ? g_res.ps_composite_arb
                               : khf_ps_want == 3 ? g_res.ps_tex
                               : khf_ps_want == 4 ? g_res.ps_comp_arb_tex
                                                  : g_res.ps, nullptr, 0);
                khf_ps_cat = khf_ps_want;
            }
            // KH_FX_TEX_ALL: per object, outside the category cache (builtin effect meshes share one category): a
            // builtin effect mesh with a set binds its pages; any other object takes the last one's off (a user
            // mesh's included). A textured solid binds its own maps there afterwards.
            if (o.effect > 0 && o.fx_tex) {
                kh_fx_tex_bind(ctx, khf_utex);
                khf_t43 = true;
            } else if (khf_t43) {
                kh_fx_tex_unbind(ctx);
                khf_t43 = false;
            }
        }
        {
            ID3D11VertexShader* khf_want_vs = khf_tx_on ? g_res.vs_tex : g_res.vs;
            if (khf_want_vs != khf_bound_vs) {
                ctx->VSSetShader(khf_want_vs, nullptr, 0);
                khf_bound_vs = khf_want_vs;
            }
            ID3D11InputLayout* khf_want_il = khf_tx_on ? g_res.layout_tex : g_res.input_layout;
            if (khf_want_il != khf_bound_il) {
                ctx->IASetInputLayout(khf_want_il);
                khf_bound_il = khf_want_il;
            }
        }

        // PSMain (the flush's solid PS; PSComposite on the near-gap ARB route) reads depth at t0; the flush's default
        // pair puts scene colour there (for effect shaders). Every solid mesh swaps the proven depth snapshot into t0
        // for its draw; the pair returns for effect meshes.
        // KH_FLUSH_NOSNAP: a solid with no snapshot (category 2) takes the same binds with t0 null (read as no
        // scene; its guard stands down, fx1[0] = 1e9) - the pair put the live depth (or null) at t1, where the
        // receive Gathers the atlas (KH_INJ_ATLAS twin).
        const int khf_srv_want = o.effect != 0 ? 1 : (g_res.comp_depth_srv ? 0 : 2);

        if (khf_srv_want != khf_srv_cat) {
            if (khf_srv_want != 1) {
                ID3D11ShaderResourceView* const khf_t0 = khf_srv_want == 0 ? g_res.comp_depth_srv : nullptr;
                ctx->PSSetShaderResources(0, 1, &khf_t0);

                // The pre-resolve snapshot at t21, the post-resolve one at t22 (khShadowPre / khShadowPost). Twin: the
                // injection carries the identical block.
                if (kh_svs_sten_on()) {
                    if (g_svs_pre_srv) {
                        ctx->PSSetShaderResources(21, 1, &g_svs_pre_srv);
                    }

                    if (g_svs_post_srv) {
                        ctx->PSSetShaderResources(22, 1, &g_svs_post_srv);
                    }
                }
                // The volume copy's stencil plane (t24), on its own arm.
                if (kh_svs_vol_ready()) {
                    ID3D11ShaderResourceView* const khf_sten = kh_rp_use_merged() ? g_rp_msten_srv : g_svs_vol_sten_srv;   // KH_VOL_REPLAY.
                    ctx->PSSetShaderResources(24, 1, &khf_sten);
                    // KH_VOL_WITNESS: its depth plane (t23). Twin: the injection.
                    if (g_svs_vol_depth_srv) ctx->PSSetShaderResources(23, 1, &g_svs_vol_depth_srv);
                    // KH_VOL_FOOT: its footprint mask (t33). Twin: the injection.
                    ID3D11ShaderResourceView* const khf_foot = kh_rp_use_merged() ? g_rp_mfoot_srv : kh_svs_foot_bound();
                    if (khf_foot) ctx->PSSetShaderResources(33, 1, &khf_foot);
                }
                if (g_vmir_srv && g_vmir_mask_time >= 0.0f) {
                    ID3D11ShaderResourceView* const khf_mir = kh_rp_mir_srv();   // KH_MIR_REPLAY.
                    ctx->PSSetShaderResources(28, 1, &khf_mir);
                }
                // Null when no atlas exists - the shader's shadowMeta.x = 0 short-circuit protects, and a stale depth
                // SRV must never masquerade as the atlas.
                ID3D11ShaderResourceView* khf_atlas =
                    shadow_live_ensure_srv() ? g_ls.atlas_srv : nullptr;
                ctx->PSSetShaderResources(1, 1, &khf_atlas);
            } else {
                khf_dsv_swap(true);   // Read-only depth before the depth SRV lands at t1.
                ctx->PSSetShaderResources(0, 2, ps_srvs);
            }
            khf_srv_cat = khf_srv_want;
        }

        const int khf_bm_want = khf_mm_rep ? 0 : o.blend_mode;   // KH_BLEND_MINMAX: the replacing draw.
        if (khf_bm_want != khf_bound_bm) {
            ctx->OMSetBlendState(g_res.blend_modes[khf_bm_want], bf, 0xFFFFFFFF);
            khf_bound_bm = khf_bm_want;
        }

        // Effect meshes never write depth. The injection depth-writes every composite-eligible solid by contract.
        // KH_SSAO_MARK: a depth write marks stencil 1 ONLY while this park's own post is still ahead of it
        // (khf_ssao.on, the tail not yet reached), because the post's eraser is the only thing that clears a mark. A
        // TestWrite draw on a park with no post (the injection owns the eligible meshes, or nothing is eligible), or
        // one in the tail past the post, would leave a mark standing for the next injection's pass to take its term at
        // - pixels it never painted.
        const bool khf_mark = khf_ssao.on && !khf_ssao_done;
        ID3D11DepthStencilState* const khf_dss_w = khf_mark ? g_res.dss_test_write_mark : g_res.dss_test_write;
        // KH_SSAO_SEE_THROUGH: a draw on a blend mode that shows the scene behind it (additive, multiply, screen,
        // lighten, darken) marks 3, not 1. The term reads 1 alone (KhSaOurs, the apply's mask), so it leaves such a
        // pixel as it leaves the engine's and does not darken what is seen through the mesh; the near-z marker
        // and the eraser read bit 0 and still take it. Normal blend marks 1: its translucents draw in the
        // tail past the post (khf_ssao_done), and the injection draws composite-eligible solids alone.
        const UINT khf_mark_ref = (o.blend_mode == 0) ? 1u : 3u;
        ID3D11DepthStencilState* dss =
            (khf_fx_arb_this)                ? g_res.dss_off :
            (o.mode == DepthMode::Off)       ? g_res.dss_off :
            (o.draw_part == 1)               ? g_res.dss_test :   // KH_MAT_BLEND: a translucent
                                                                  // Part never writes depth.
            (o.effect != 0)                  ? g_res.dss_test :   // KH_FX_NODEPTH: in any mode.
            (o.mode == DepthMode::TestWrite) ? khf_dss_w :
            is_composite_eligible(o)         ? khf_dss_w :
                                               g_res.dss_test;

        // Read-only DSV reconciliation: a pair consumer (depth at t1) cannot write the depth it reads, so its write arm
        // is a plain test; a solid that writes takes the real DSV back (t1 is the atlas there, no hazard).
        if (khf_dsv_ro && dss == khf_dss_w) {
            if (khf_srv_cat == 1) dss = g_res.dss_test;
            else                  khf_dsv_swap(false);
        }
        // KH_WSEE: a solid writes the main depth (late). KH_WSEE_FLUSHW_PRED: noted where it follows a landed capture
        // or falls in a cycle the hold stood down.
        if (dss == khf_dss_w) {
            const uint64_t khf_wc = g_topo_cycles;
            kh_wsee_taint(khf_wc);
            if (g_wsee_ok_cycle.load(std::memory_order_relaxed) == khf_wc ||
                g_wsee_flushw_off.load(std::memory_order_relaxed) == khf_wc) {
                g_wsee_flushw_cycle.store(khf_wc, std::memory_order_relaxed);
            }
        }

        const UINT khf_ref = (dss == g_res.dss_test_write_mark) ? khf_mark_ref : 0u;   // KH_SSAO_SEE_THROUGH.
        if (dss != khf_bound_dss || khf_ref != khf_bound_ref) {
            ctx->OMSetDepthStencilState(dss, khf_ref);
            khf_bound_dss = dss;
            khf_bound_ref = khf_ref;
        }
        // A blend part on a two-sided object takes the ordered pass regardless
        // of the object colour's alpha - interiors first, exteriors over them.
        const bool khf_ts_ordered = o.two_sided &&
                                    ((o.blend_mode == 0 && o.color[3] < 0.999f) || o.draw_part == 1) &&
                                    g_res.rasterizer_front != nullptr;
        ID3D11RasterizerState* khr_want_rs =
            khf_ts_ordered ? g_res.rasterizer_front :
            o.two_sided    ? g_res.rasterizer : g_res.rasterizer_cull;

        if (khr_want_rs != khr_bound_rs) {
            ctx->RSSetState(khr_want_rs);
            khr_bound_rs = khr_want_rs;
        }

        khf_vp_select(khf_nz_this ? 1 : 0);   // KH_FLUSH_NEARZ.

        const int mid = mesh_id_clamp(o.mesh);

        // KH_WSEE: an injection's deferred copy draws from its hold entry (kh_wsee_hold_ref: a copy entry the vertices
        // the injection drew with, an own entry - a static part, or one drawn from its mesh's buffer at the injection
        // - the mesh's own buffer, whatever the slot holds now) at the injection's level and crossfade
        // (kh_wsee_hold_lod: a copy holds those levels' translucent vertices alone), not a pick of its own. The
        // append above dropped a void entry (the hold gone, or an own entry's mesh buffer gone).
        const KhVbRef khb_vb = o.ws_hold != 0u ? kh_wsee_hold_ref(o) : kh_mesh_vb_ref(mid, o.slot);   // KH_CLOTH.
        if (khb_vb != bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &khb_vb.vb, &stride, &khb_vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = khb_vb;
        }

        const MeshDef& khf_md = mesh_def(mid);
        int khf_lod = 0;
        float khf_lodt = 0.0f;
        // A locked instance never picks - level 0, no fade. Twin at the injection.
        if (o.ws_hold != 0u) {
            kh_wsee_hold_lod(o, khf_lod, khf_lodt);   // KH_WSEE: the injection's pick.
        } else if (!kh_lod_locked(o)) {   // KH_LOD_SIM_SLOT.
            kh_lod_pick(khf_md, kh_lod_radius(o), sqrtf(g_dist_memo_flush.get(o, cam)), khf_lod, khf_lodt);
        }
        const bool khf_lodx = khf_lodt > 0.0f;
        // Buckets, flush edition: this object is its bucket's representative. Its CB is up (textured: built, uploaded
        // per submesh by the draw) and its PS/dss/blend bound; swap in the instanced VS + layout, bind the lane ring
        // once per pass, draw the emitted ranges (every member visible at emission, sharing the route by key, at its
        // own LOD level, crossfade included). A two-sided bucket draws CullNone. Twin at the injection.
        // KH_WSEE: the object's geometry as one call, so the tail's re-draws (kh_wsee_tail, below) are this same
        // draw - khf_wrep: such a re-draw, which counts nothing (a bucket an earlier call unbatched stays unbatched).
        auto khf_geo = [&](bool khf_wrep) {
            int khf_lod_iters = khf_lodx ? 2 : 1;

            if (khf_inst_on && khf_inst_plan.batch_of[khf_oi] >= 0) {
                // KH_SKIN_POOL: a skin pool chunk's bucket pulls (the injection's twin).
                const KhSkinPoolChunk* const khf_ipc = kh_inst_bucket_pool(khf_inst_plan, khf_oi);
                ID3D11VertexShader* khf_ivs = khf_ipc ? (khf_tx_on ? g_res.vs_inst_sk_tex : g_res.vs_inst_sk)
                                                      : (khf_tx_on ? g_res.vs_inst_tex : g_res.vs_inst);
                ID3D11InputLayout*  khf_iil = khf_ipc ? g_res.layout_inst_sk
                                                      : (khf_tx_on ? g_res.layout_inst_tex : g_res.layout_inst);

                if (khf_ivs && khf_iil) {
                    if (khf_ivs != khf_bound_vs) { ctx->VSSetShader(khf_ivs, nullptr, 0); khf_bound_vs = khf_ivs; }
                    if (khf_iil != khf_bound_il) { ctx->IASetInputLayout(khf_iil); khf_bound_il = khf_iil; }
                    kh_inst_sk_bind(ctx, khf_ipc, khf_bound_sk);

                    if (!khf_inst_vb_bound) {
                        const UINT khf_is = KH_INST_STRIDE, khf_io = 0;
                        ctx->IASetVertexBuffers(1, 1, &g_res.inst_vb[0], &khf_is, &khf_io);
                        khf_inst_vb_bound = true;
                    }

                    if (o.two_sided && !khf_ts_ordered && khr_bound_rs != g_res.rasterizer) {
                        ctx->RSSetState(g_res.rasterizer);
                        khr_bound_rs = g_res.rasterizer;
                    }

                    uint32_t khf_in = 0;
                    const uint32_t khf_id = kh_inst_draw(ctx, dev, khf_inst_plan, meshes, khf_oi,   // KH_BUCKETS.
                        khf_obj_cbd, g_res.constant_buffer, khf_tx_on ? khf_txm : nullptr, khf_ts_ordered, khr_bound_rs,
                        khf_arb_this ? 2 : 0, khf_arb_this ? g_res.ps_comp_arb_tex : g_res.ps_tex,
                        o.draw_part == 1 ? 2 : 1, khf_in);
                    if (khf_tx_on && !khf_wrep) kh_stat_add(g_stats.textured_draws, khf_id);
                    if (khf_ipc && !khf_wrep) kh_stat_add(g_stats.skin_inst, khf_in);   // KH_SKIN_POOL.
                    khf_lod_iters = 0;   // The batch drew; no per-object level walk.
                } else {
                    kh_inst_unbatch(khf_inst_plan, khf_oi);   // KH_INST_UNBATCH.
                }
            }

            for (int khf_li = 0; khf_li < khf_lod_iters; ++khf_li) {
                const int khf_lvl = khf_lod + khf_li;

                if (khf_lodx) {
                    khf_obj_cbd.blend_ctl[3] = kh_lod_dither(khf_lodt, khf_li != 0);
                    // KH_TX_ONE_UPLOAD: textured, kh_draw_textured uploads the dither with each submesh.
                    if (!khf_tx_on && !kh_upload_obj_cb(ctx, g_res.constant_buffer, khf_obj_cbd)) break;
                }

                if (khf_tx_on) {
                    const uint32_t khf_txd = kh_draw_textured(ctx, dev, khf_obj_cbd, g_res.constant_buffer,
                                                              *khf_txm, mid, khf_ts_ordered, khr_bound_rs,
                                                              khf_arb_this ? 2 : 0,   // The arb variant's user twin.
                                                              khf_arb_this ? g_res.ps_comp_arb_tex : g_res.ps_tex,
                                                              khf_lvl,
                                                              o.draw_part == 1 ? 2 : 1);   // KH_MAT_BLEND part.
                    if (!khf_wrep) kh_stat_add(g_stats.textured_draws, khf_txd);
                } else {
                    UINT khf_ls = 0, khf_lc = 0;
                    mesh_lod_range(khf_md, khf_lvl, khf_ls, khf_lc);

                    // The ordered contract restarts per level: iteration 1 left the cull rasterizer bound, and
                    // interiors must precede exteriors for this level too.
                    if (khf_ts_ordered && khr_bound_rs != g_res.rasterizer_front) {
                        ctx->RSSetState(g_res.rasterizer_front);
                        khr_bound_rs = g_res.rasterizer_front;
                    }

                    ctx->DrawIndexed(khf_lc, khf_ls, 0);

                    if (khf_ts_ordered) {   // Pass 2: front faces over the interior.
                        ctx->RSSetState(g_res.rasterizer_cull);
                        khr_bound_rs = g_res.rasterizer_cull;
                        ctx->DrawIndexed(khf_lc, khf_ls, 0);
                    }
                }
            }
        };
        // KH_WSEE: a translucent part in a sorting run draws after the game's layers, composited under
        // the nearer ones (cb.hlsl KhWseeMeshCut / KhWseeMeshOut on PSMain - the textured twin, its user-material
        // twins, PSWhite in place of a pending one; the composite unit never takes the lane): the tail's
        // normal-blend parts that test without writing (the injection's deferred ones among them), no near-gap
        // route. The lanes go into the object's slice, which kh_draw_textured uploads per submesh: fx_tex[1].z = the
        // mode, fx_fog[0] = the decode (both zero on every other mesh draw - upload_cb rebuilds the slice from the
        // zeroed template each object, and the replay below zeroes them). The capture is checked again per part
        // (kh_wsee_tail_owed): a depth-writing draw earlier in this run taints it (the loop's own check, at the dss
        // selection), and the record no longer holds the opaque depth then.
        const bool khf_ws_tail = khf_ix < khf_def_n || khf_ix >= khf_solid_n;   // The deferred front parts too.
        const bool khf_ws_elig = khf_ws_tail && o.draw_part == 1 && khf_tx_on && dss == g_res.dss_test &&
                                 o.effect == 0 && o.blend_mode == 0 && !khf_mm_rep && !o.fullscreen &&
                                 o.mode != DepthMode::Off && !khf_nz_this;
        int khf_ws_sort_this = (khf_ws_elig && khf_ws_mode > 0 && kh_wsee_tail_owed()) ? khf_ws_mode : 0;
        if (khf_ws_sort_this >= 2 && khf_ws_q == 0) {
            // KH_WSEE_BWANT: the composite reads B / B2 (KhWseeMeshCut) - its want from the next clear, and
            // without them this cycle the run's parts take mode 1, as without Q.
            g_wsee_col_want_cycle.store(g_topo_cycles, std::memory_order_relaxed);
            const bool khf_ws_col = g_wsee_col_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
            khf_ws_q = (khf_ws_col && kh_wsee_q_ensure(dev) && kh_wsee_q_capture(ctx)) ? 1 : -1;
        }
        if (khf_ws_sort_this >= 2 && khf_ws_q < 0) khf_ws_sort_this = 1;
        KhWseeSortScope khf_wss;
        if (khf_ws_sort_this > 0) {
            if (khf_wss.begin(ctx, khf_ws_sort_this)) {
                khf_obj_cbd.fx_tex[1][2] = static_cast<float>(khf_ws_sort_this);
                khf_obj_cbd.fx_tex[1][3] = khf_wss.b2m ? 8.0f : 0.0f;   // KH_WSEE_B2M: bit 3, the B2 moment.
                if (khf_wss.od) khf_obj_cbd.fx_tex[1][3] += 32.0f;   // Bit 5, D3 at t65.
                if (khf_wss.pb) khf_obj_cbd.fx_tex[1][3] += 64.0f;   // KH_PSB: bit 6, P0 / P1 / N / N2 at t66 - t69.
                if (khf_wss.pe) khf_obj_cbd.fx_tex[1][3] += 128.0f;   // KH_PSB: bit 7, the old record's NW unread.
                if (khf_wss.np) khf_obj_cbd.fx_tex[1][3] += 256.0f;   // KH_WSEE_NOPRE: bit 8, pre derived;
                if (khf_wss.s2) khf_obj_cbd.fx_tex[1][3] += 512.0f;   // bit 9, S2 at t62.
                memcpy(khf_obj_cbd.fx_fog[0], khf_ws_dec, sizeof(khf_ws_dec));
            } else {
                khf_ws_sort_this = 0;
            }
        }
        khf_geo(false);
        khf_wss.end();
        // KH_WSEE: our translucent tail into the cycle's capture, as an engine blended no-write draw goes (the
        // KH_WSEE note): past the solids (and the injection's deferred parts, drawn ahead of them: khf_ws_tail),
        // testing the depth without writing it, on normal blend,
        // no effect mesh (unarmed; its output is the effect's, not a coverage), no depth-Off overlay, no near-gap
        // route (below the world's range, where no pixel is covered). The T pass needs the coverage as the alpha: a
        // perceptual translucent writes its own mix opaque, so its arm (blend_ctl.x) is dropped for that pass and put
        // back after - an untextured draw uploads it here, a textured one with each submesh (kh_draw_textured); NW's
        // pass reads only the depth and the pixel shader's discards, so it takes the T pass's constants as they are.
        if (khf_ws_tail && dss == g_res.dss_test && o.effect == 0 && o.blend_mode == 0 && !khf_mm_rep &&
            !o.fullscreen && o.mode != DepthMode::Off && !khf_nz_this && kh_wsee_tail_owed()) {
            const float khf_wpc = khf_obj_cbd.blend_ctl[0];
            bool khf_wup = true;   // A failed upload draws neither pass (T would take the opaque mix).
            // A sorted part re-draws as the plain tail does - the hardware test, its lanes off (the T pass binds T,
            // which the sorted draw's pixel shader reads at t54; the plain output's alpha is the same coverage).
            khf_obj_cbd.fx_tex[1][2] = 0.0f;
            khf_obj_cbd.fx_tex[1][3] = 0.0f;
            memset(khf_obj_cbd.fx_fog[0], 0, sizeof(khf_obj_cbd.fx_fog[0]));
            const bool khf_wok = kh_wsee_tail(ctx, [&](bool khf_wt) {
                if (khf_wt) {
                    khf_obj_cbd.blend_ctl[0] = 0.0f;
                    if (!khf_tx_on) khf_wup = kh_upload_obj_cb(ctx, g_res.constant_buffer, khf_obj_cbd);
                }
                if (khf_wup) khf_geo(true);
            });
            khf_obj_cbd.blend_ctl[0] = khf_wpc;
            if (!khf_wok || !khf_wup) kh_wsee_taint(g_topo_cycles);   // T / NW would miss it.
        }

        khf_obj_cbd.blend_ctl[3] = 0.0f;   // Never leaks into the next object.
        // KH_FX_CAP_ORDER: this object drew (every skip above left before here) - a bucket's draw at its
        // representative drew the members with it, which are of its kind.
        khf_cap_stale = true;
        if (o.effect == 0) khf_cap_stale_ne = true;
    }
    if (g_wsee_defer_taken == g_topo_cycles) g_wsee_defer_drawn = g_topo_cycles;   // KH_WSEE: the copies drew.

    if (!khf_ssao_done && khf_ssao.on) {   // KH_SSAO: no translucent tail - after the last solid.
        ID3D11ShaderResourceView* khf_t1_none = nullptr;   // As at the tail site; the chain rebinds t1 itself.
        ctx->PSSetShaderResources(1, 1, &khf_t1_none);
        khf_dsv_swap(false);
        kh_ssao_post(ctx, khf_ssao);
        khf_srv_cat = -1;
    }

    // Restore the parked pass's own viewport (the depth-range override covered only the mesh draws).
    if (khf_n_saved_vp > 0) ctx->RSSetViewports(khf_n_saved_vp, khf_saved_vp);

    // Chain passes below assume the ambient (CullNone) rasterizer.
    if (khr_bound_rs != g_res.rasterizer) ctx->RSSetState(g_res.rasterizer);

    // Exactly one chain per cycle, whichever thread. A park that lands in a cycle the render thread already flushed
    // (g_rtshadow_cycle) draws no chain, and the render thread draws none when a park drew the chain first
    // (g_fx_park_cycle); otherwise the effects doubled for that frame.
    const bool khfl_chain_owed = (!khfl_rt || khfl_chain_only || g_fx_park_cycle != g_topo_cycles) &&
                                 (khfl_rt || g_rtshadow_cycle.load(std::memory_order_relaxed) != g_topo_cycles);
    if (!fullscreen.empty() && effects_ready && khfl_chain_owed) {
        if (khfl_rt && !khfl_chain_only && kh_fx_late_take()) {
            // KH_FX_LATE: owed to a later resolve, past the view model.
            kh_vmsee_arm(dev, fullscreen);   // KH_VM_SEE: a depth reader there sees past its translucent planes.
            kh_wsee_arm(dev, fullscreen);    // KH_WSEE: and through the world's blended layers.
        } else {
            if (khfl_chain_only) g_fx_late_ran_cycle.store(g_topo_cycles_pub.load(std::memory_order_relaxed), std::memory_order_relaxed);
            // KH_PARK_CHAIN_STAMP: stamped here, not at entry - a park that returned early (no DSV, wrong pass)
            // or had no chain to run left the render thread's chain refused for a cycle nobody drew it in.
            if (!khfl_rt) g_fx_park_cycle = g_topo_cycles;
            // KH_CHAIN_RT0_DOC: the run takes the bound RT0 as the scene (its capture and its final write), as this
            // flush's mesh draws do; at a resolve it is not compared with the resolve's source. Were the two to
            // differ, the chain would read and write RT0 and the resolved frame would carry none of it; a refusal
            // keyed on the scene's name (a fingerprint's, KH_SCENE_NAME) would turn the chain off wherever that name
            // is wrong and RT0 right.
            khf_dsv_swap(true);   // The chain samples live depth; its OM save must hold the read-only view.
            kh_fx_chain_run(dev, ctx, fullscreen, bf, ps_srvs[1], !meshes.empty() || khf_cap_defer,   // KH_CAP_DEFER.
                            upload_cb, needs_depth, now_d, &khf_lcull);   // KH_LOCAL_CULL; KH_FUSE_CLOCK.
        }
    }

    if (khf_om_captured) {
        // t1 may still hold the live depth SRV (the chain's, or a pair consumer's last bind): down before
        // the writable DSV returns, as the tail and the SSAO site do - never both bound at once.
        ID3D11ShaderResourceView* khf_t1_off = nullptr;
        ctx->PSSetShaderResources(1, 1, &khf_t1_off);
        khf_om.restore(ctx);
    }
    khf_om.release();
    backup.restore(ctx);
}
}