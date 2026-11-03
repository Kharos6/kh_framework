#pragma once

// The feature half (rendering_integration_features.hpp): the per-object helpers: visibility, placement and rotation,
// bounds and distances, the shadow flags, the handle ids. Never include it on its own.
namespace RenderIntegration {

// KH_INFRONT_EFFECT: whether the view-model slice draws the object - the flag on a solid mesh. An effect mesh ignores
// inFront (RenderObject::in_front): the world routes draw it and it counts as no slice demand; a pass carries no
// inFront flag (the property is a mesh's). Every "drawn in the slice" test reads this, never the field; the tests
// already limited to solid meshes (kh_shadow_part, is_composite_eligible, the slice's own walk) read the field.
inline bool kh_in_front(const RenderObject& o) { return o.in_front && o.effect == 0; }
// KH_SHOW_INFRONT: visible as far as any view can draw it - visible false, and an inFront mesh with showInFront false
// (never drawn: the slice is first person by definition), are alike for every route. The test wherever visible is
// read for a superset or a demand (posing, the pre-filter, the replay rectangles, the mirror, the censuses); the PIP
// tests visible itself (is_composite_eligible already keeps inFront meshes out of it).
inline bool kh_obj_vis(const RenderObject& o) { return o.visible && (o.show_in_front || !kh_in_front(o)); }

// KH_POS_RES - an attached mesh's centre finer than a world float holds. The step composes it in double (the helper,
// the KH_PXY_SMOOTH delta, attachPosition, a skeletal mesh's centre) and stores the nearest floats in pos and the
// rest in pos_res (engine axes, at most half a float step); every draw that places the mesh adds pos_res where it
// forms a camera- or anchor-relative centre (kh_fill_center_rel, the object record's res lane), so the drawn position
// carries no world-scale rounding of ours. The residual belongs to the pos it was taken for (pos_res_at): any other
// writer that changes pos retires it without knowing it exists (one that stores the very floats it was taken for
// leaves it live - at most half a float step from what that writer meant). The world-absolute lanes (centerSize,
// the record's pos) keep pos alone; the bounds cover the residual (kh_bounds_size_of grows by it) and an occluder's
// box is pulled in by it (kh_vis_occ_build), so no test is off by it; every cache keyed on a caster's position keys
// on its residual too, and a simulation's frame and a collider enter exact (kh_cloth_sync).
inline void kh_pos_res(const RenderObject& khpr_o, float khpr_e[3]) {   // Engine axes; zero when retired.
    if (memcmp(khpr_o.pos, khpr_o.pos_res_at, sizeof(khpr_o.pos)) == 0) {
        memcpy(khpr_e, khpr_o.pos_res, sizeof(khpr_o.pos_res));
    } else {
        khpr_e[0] = khpr_e[1] = khpr_e[2] = 0.0f;
    }
}
// The exact centre (SQF order) into pos and its residual: true when either changed (the step's moved test).
inline bool kh_pos_set_exact(RenderObject& khpx_o, const double khpx_p[3]) {
    const float khpx_f[3] = { static_cast<float>(khpx_p[0]), static_cast<float>(khpx_p[1]),
                              static_cast<float>(khpx_p[2]) };
    const float khpx_r[3] = { static_cast<float>(khpx_p[0] - static_cast<double>(khpx_f[0])),   // Engine axes.
                              static_cast<float>(khpx_p[2] - static_cast<double>(khpx_f[2])),
                              static_cast<float>(khpx_p[1] - static_cast<double>(khpx_f[1])) };
    const bool khpx_ch = memcmp(khpx_o.pos, khpx_f, sizeof(khpx_f)) != 0 ||
                         memcmp(khpx_o.pos_res_at, khpx_f, sizeof(khpx_f)) != 0 ||
                         memcmp(khpx_o.pos_res, khpx_r, sizeof(khpx_r)) != 0;
    memcpy(khpx_o.pos, khpx_f, sizeof(khpx_f));
    memcpy(khpx_o.pos_res_at, khpx_f, sizeof(khpx_f));
    memcpy(khpx_o.pos_res, khpx_r, sizeof(khpx_r));
    return khpx_ch;
}
// The centre as the draws place it (pos plus a live residual), SQF order.
inline void kh_pos_exact(const RenderObject& khpe_o, double khpe_p[3]) {
    float khpe_e[3];
    kh_pos_res(khpe_o, khpe_e);
    khpe_p[0] = static_cast<double>(khpe_o.pos[0]) + static_cast<double>(khpe_e[0]);
    khpe_p[1] = static_cast<double>(khpe_o.pos[1]) + static_cast<double>(khpe_e[2]);
    khpe_p[2] = static_cast<double>(khpe_o.pos[2]) + static_cast<double>(khpe_e[1]);
}

// The matrix for a [pitch, yaw, roll] triple in ARMA degrees, in ENGINE axes. The one builder of an EULER rotation's
// matrix (the [vectorDir, vectorUp] form is kh_rotation_from_vectors'), at TWO call sites: kh_set_rotation below, and
// kh_rotation_m_from_gv in sqf_integration.hpp, the parser every other typed rotation (attachRotation, a chain's
// endRotation) takes its matrix from - so KH_ATTACH_OFFSET's attach-space rotation cannot drift from the object's.
inline void kh_rotation_matrix(float khr_m[9], float pitch_deg, float yaw_deg, float roll_deg) {
    if (pitch_deg == 0.0f && yaw_deg == 0.0f && roll_deg == 0.0f) {
        const float khr_id[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        memcpy(khr_m, khr_id, sizeof(khr_id));
        return;
    }

    const float khr_d2r = 0.017453292519943295f;
    const float cp = cosf(pitch_deg * khr_d2r), sp = sinf(pitch_deg * khr_d2r);
    const float cy = cosf(yaw_deg * khr_d2r),  sy = sinf(yaw_deg * khr_d2r);
    const float cr = cosf(roll_deg * khr_d2r), sr = sinf(roll_deg * khr_d2r);
    const float khr_roll[3][3]  = { { cr, 0.0f, -sr }, { 0.0f, 1.0f, 0.0f }, { sr, 0.0f, cr } };
    const float khr_pitch[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, cp, sp }, { 0.0f, -sp, cp } };
    const float khr_yaw[3][3]   = { { cy, -sy, 0.0f }, { sy, cy, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    float khr_rp[3][3];
    float khr_ma[3][3];

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            khr_rp[r][c] = khr_roll[r][0] * khr_pitch[0][c] +
                           khr_roll[r][1] * khr_pitch[1][c] +
                           khr_roll[r][2] * khr_pitch[2][c];
        }
    }

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            khr_ma[r][c] = khr_rp[r][0] * khr_yaw[0][c] +
                           khr_rp[r][1] * khr_yaw[1][c] +
                           khr_rp[r][2] * khr_yaw[2][c];
        }
    }

    // Arma -> engine similarity: engine index i reads arma index p(i), p = (0, 2, 1), its own inverse.
    const int khr_p[3] = { 0, 2, 1 };

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            khr_m[r * 3 + c] = khr_ma[khr_p[r]][khr_p[c]];
        }
    }
}

// rot / rotated are the euler the script typed; rot_m is the matrix. rotated follows the EULER here (a typed zero is
// not rotated); the attachment path has its own test, kh_attach_rotated, because a raw basis has no euler.
inline void kh_set_rotation(RenderObject& o, float pitch_deg, float yaw_deg, float roll_deg) {
    o.rot[0] = pitch_deg;
    o.rot[1] = yaw_deg;
    o.rot[2] = roll_deg;
    o.rotated = pitch_deg != 0.0f || yaw_deg != 0.0f || roll_deg != 0.0f;
    kh_rotation_matrix(o.rot_m, pitch_deg, yaw_deg, roll_deg);
}

// KH_ROT_VECTORS - the matrix for a [vectorDir, vectorUp] pair in SQF's axes (x east / right, y north / forward, z
// up), as setVectorDirAndUp takes it: the direction kept, the up made square to it, aside = up x dir. The rows are
// the ones kh_rotation_matrix and a raw read hand back (aside / up / dir, engine axes: x east, y up, z north). False
// for a zero, non-finite or (to within 0.06 degrees) parallel pair.
inline bool kh_rotation_from_vectors(float khrv_m[9], const float khrv_dir[3], const float khrv_up[3]) {
    float khrv_d[3] = { khrv_dir[0], khrv_dir[2], khrv_dir[1] };   // SQF -> engine axes.
    float khrv_u[3] = { khrv_up[0], khrv_up[2], khrv_up[1] };
    const float khrv_dl = kh_cloth_v3_len(khrv_d);
    const float khrv_ul = kh_cloth_v3_len(khrv_u);
    if (!(khrv_dl > 1.0e-6f && khrv_dl < 1.0e30f) || !(khrv_ul > 1.0e-6f && khrv_ul < 1.0e30f)) return false;
    for (int k = 0; k < 3; ++k) { khrv_d[k] /= khrv_dl; khrv_u[k] /= khrv_ul; }
    const float khrv_k = kh_cloth_v3_dot(khrv_u, khrv_d);
    for (int k = 0; k < 3; ++k) khrv_u[k] -= khrv_k * khrv_d[k];
    const float khrv_sl = kh_cloth_v3_len(khrv_u);
    if (!(khrv_sl >= 1.0e-3f)) return false;
    for (int k = 0; k < 3; ++k) khrv_u[k] /= khrv_sl;
    khrv_m[0] = khrv_u[1] * khrv_d[2] - khrv_u[2] * khrv_d[1];
    khrv_m[1] = khrv_u[2] * khrv_d[0] - khrv_u[0] * khrv_d[2];
    khrv_m[2] = khrv_u[0] * khrv_d[1] - khrv_u[1] * khrv_d[0];
    for (int k = 0; k < 3; ++k) { khrv_m[3 + k] = khrv_u[k]; khrv_m[6 + k] = khrv_d[k]; }
    return true;
}
// KH_ROT_VECTORS - kh_set_rotation's twin for the vector form: no euler (zero), the matrix as given, rotated unless
// it is exactly the identity (kh_attach_rotated's test).
inline void kh_set_rotation_rows(RenderObject& o, const float khsr_m[9]) {
    static const float khsr_id[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    o.rot[0] = 0.0f;
    o.rot[1] = 0.0f;
    o.rot[2] = 0.0f;
    memcpy(o.rot_m, khsr_m, sizeof(o.rot_m));
    o.rotated = memcmp(khsr_m, khsr_id, sizeof(khsr_id)) != 0;
}

// Enclosing half extents (engine axes) of a rotated, per-axis-scaled mesh cube: he_k = sum_j |R[j][k]| * hl_j, the
// OBB -> AABB bound - exact for axis-aligned rotations, conservative otherwise. hl = engine-local half extents.
inline void kh_rot_half_extents(const float hl[3], const float* rot_m, bool rotated, float he[3]) {
    if (!rotated) {
        he[0] = hl[0];
        he[1] = hl[1];
        he[2] = hl[2];
        return;
    }

    for (int k = 0; k < 3; ++k) {
        he[k] = fabsf(rot_m[k]) * hl[0] + fabsf(rot_m[3 + k]) * hl[1] + fabsf(rot_m[6 + k]) * hl[2];
    }
}

// KH_CAST_REFIT: the size every BOUNDS test takes - the script's, grown per axis to what a plain cloth's simulation
// reached (RenderObject::cast_size). KH_LOD_BOUNDS: then grown per axis, about the centre, to the mesh's every-level
// bounds (MeshDef::occ_hmn / occ_hmx, made for every mesh by kh_mesh_occ_prep): a decimated level can leave the unit
// box, and which level is drawn is not a bounds test's to know. A mesh inside the unit box (every builtin) is scaled
// by exactly 1; an import's level 0 can sit a rounding outside it (the squash's (p - ctr) / ext), carried too.
// KH_USER_VS: plus the vertex stage's declared reach on both sides of every axis (KhMaterialSet::vertex_bound) -
// the deformation happens on the GPU, so the script's promise is all a bounds test has. Untouched without one.
// KH_POS_RES: plus twice the residual's length on every axis, divided by the smallest row scale of a scaled
// rotation: the mesh is drawn at pos plus its residual, which may point anywhere, and the box about pos then holds
// all of it after any rotation. Untouched with no live residual.
inline void kh_bounds_size_of(const RenderObject& o, float khbs_sz[3]) {
    for (int k = 0; k < 3; ++k) khbs_sz[k] = o.cast_size[k] > o.size[k] ? o.cast_size[k] : o.size[k];
    {
        const MeshDef& khbs_md = mesh_def(mesh_id_clamp(o.mesh));
        static const int khbs_ax[3] = { 0, 2, 1 };   // SQF axis -> engine axis.
        for (int k = 0; k < 3; ++k) {
            const int khbs_e = khbs_ax[k];
            khbs_sz[k] *= 2.0f * fmaxf(fmaxf(-khbs_md.occ_hmn[khbs_e], khbs_md.occ_hmx[khbs_e]), 0.5f);
        }
    }
    if (o.materials && o.materials->vertex_any && o.materials->vertex_bound > 0.0f) {
        for (int k = 0; k < 3; ++k) khbs_sz[k] += 2.0f * o.materials->vertex_bound;
    }
    {   // KH_POS_RES.
        float khbs_r[3];
        kh_pos_res(o, khbs_r);
        const float khbs_rl = sqrtf(khbs_r[0] * khbs_r[0] + khbs_r[1] * khbs_r[1] + khbs_r[2] * khbs_r[2]);
        if (khbs_rl > 0.0f) {
            float khbs_g = 2.0f * khbs_rl;
            if (o.rotated) {
                for (int khbs_rr = 0; khbs_rr < 3; ++khbs_rr) {
                    const float* khbs_w = &o.rot_m[khbs_rr * 3];
                    const float khbs_n = sqrtf(khbs_w[0] * khbs_w[0] + khbs_w[1] * khbs_w[1] + khbs_w[2] * khbs_w[2]);
                    if (khbs_n > 1.0e-6f && khbs_n < 1.0f) khbs_g = fmaxf(khbs_g, 2.0f * khbs_rl / khbs_n);
                }
            }
            for (int k = 0; k < 3; ++k) khbs_sz[k] += khbs_g;
        }
    }
}
inline void kh_world_half_extents(const RenderObject& o, float he[3]) {
    float khwh_sz[3];
    kh_bounds_size_of(o, khwh_sz);   // KH_CAST_REFIT.
    const float hl[3] = { khwh_sz[0] * 0.5f, khwh_sz[2] * 0.5f, khwh_sz[1] * 0.5f };
    kh_rot_half_extents(hl, o.rot_m, o.rotated, he);
}


// The distance kh_mesh_dist_sq (below) takes, over a raw record: kh_drawn_lod (the level of the depth-only passes
// that re-draw the scene's meshes) and the dlsw mask (its KhDlswCaster, copied from a SunCaster) pick their LOD with
// it and must land on the level the injection drew.
inline float kh_mesh_dist_sq_of(const float* pos, const float* size, const float* rot_m,
                                bool rotated, const float cam[3]) {
    const float c[3] = { cam[0], cam[2], cam[1] };   // Engine -> SQF axes.
    const float hl[3] = { size[0] * 0.5f, size[2] * 0.5f, size[1] * 0.5f };
    float khs_he[3];
    kh_rot_half_extents(hl, rot_m, rotated, khs_he);
    const float hs[3] = { khs_he[0], khs_he[2], khs_he[1] };   // SQF axes.
    float acc = 0.0f;

    for (int k = 0; k < 3; ++k) {
        const float lo = pos[k] - hs[k];
        const float hi = pos[k] + hs[k];
        const float p = c[k] < lo ? lo : (c[k] > hi ? hi : c[k]);
        const float d = c[k] - p;
        acc += d * d;
    }

    return acc;
}

inline float kh_mesh_dist_sq(const RenderObject& o, const float cam[3]) {
    return kh_mesh_dist_sq_of(o.pos, o.size, o.rot_m, o.rotated, cam);
}
// KH_NEARZ_BOUNDS: the same distance to the box around everything the object may draw (kh_bounds_size_of's size).
// The near-z route (KhDistMemo::get_drawn, the injection's nearest clear) and the mirror (kh_rp_box_dist's band,
// kh_rp_mir_reads) take it; the LOD pick keeps kh_mesh_dist_sq (the script's size: KH_CAST_REFIT's rule).
inline float kh_bounds_dist_sq(const RenderObject& o, const float cam[3]) {
    float khbq_s[3];
    kh_bounds_size_of(o, khbq_s);
    return kh_mesh_dist_sq_of(o.pos, khbq_s, o.rot_m, o.rotated, cam);
}

// KH_SHADOW_OFF - a shadow view distance of KH_SHADOW_OFF_M or less (the video options' shadow view distance,
// getObjectViewDistance's second element, read by stage_video_options on the game thread whoever owns g_sun_range)
// turns every shadow of ours off: every mesh casts and receives as if castShadow and receiveShadow were both false
// (KH_SHADOW_SWITCH) - no caster in any gather or census (kh_cast_on, kh_shadow_active: no sun, far or DLS map, no
// world cast, no DLS world pass) and no received term (the khObjNoRecv lanes: the sun's bands / atlas, the stencil
// and mirror, the self ladder, the DLS shadows; the receive lanes read g_objbuf_shoff, the object records' own copy,
// kh_objbuf_sync). What serves only those terms stands down through a path it already has: the DLS light choice
// (kh_dls_select's allowDynamicShadows-off branch: no slot, so no map and no world pass); the volume copy
// (kh_svs_vol_copy's no-reader branch, which drops the primed flag); the mask snapshots and the prime
// (kh_svs_snap_wanted false); the volume seam's arm (kh_svs_inject_armed false: no world footprint in the engine's
// volume pass, so no recording, replay, mirror prepass, patch, mirror replay or census take) with the in-front seam
// and the replay's constant-buffer images (kh_rp_img_note). Every frame while off, kh_volume_seam_frame_reset
// leaves nothing of that path ready (copy, snapshots, mirror) and releases the last recorded pass. What stays is
// what non-shadow work reads: the engine-side capture (its cascade axes are the sun direction that lights every mesh
// - kh_sun_raw_note), the constant-buffer census (a blanket term of the upload funnel the lighting probes ride -
// kh_cbc_on), the dynamic lights' harvest (kh_dl_cpu_wanted: a lit mesh or a dynamicLightFog pass) and the
// engine-mask tracking the capture shares.
static constexpr float KH_SHADOW_OFF_M = 5.0f;
inline bool kh_shadows_off() { return g_shadows_off.load(std::memory_order_relaxed); }

// Handles are opaque strings from the framework's UID generator, prefixed "khr_"; command errors never carry the
// prefix, so callers distinguish success by it. String, not numeric: SQF scalars are 32-bit floats, integer-exact
// only to 2^24.
static uint64_t g_next_seq = 0;

inline std::string make_render_uid() {
    return std::string("khr_") + UIDGenerator::generate();
}

inline void kh_obj_center_engine(const RenderObject& khc_o, float khc_out[3]) {
    khc_out[0] = khc_o.pos[0];
    khc_out[1] = khc_o.pos[2];
    khc_out[2] = khc_o.pos[1];
}

// Casting is the object's, not the material's. The mirror prepass and the prime draw every caster whole; the sun
// tiers and DLS maps draw an alpha caster (colour alpha below 1, or a cutout / blend slot) through their alpha twins,
// an untextured one whole with its colour alpha, a textured one per submesh with its material (kh_sun_draw_alpha_t);
// the seam footprint leaves out a caster with colour alpha below 1 and draws a cutout / blend one per submesh
// through its alpha twin (kh_footprint_alpha_on). Without its alpha twins a pass has no alpha caster.

inline bool kh_obj_has_blend(const RenderObject& o) {
    if (!o.materials || !o.materials->any) return false;
    for (const KhMaterial& m : o.materials->slots) if (m.used && m.alpha_mode == 2) return true;
    return false;
}

// Material draw support (both loops share these).

inline void kh_apply_native_size(RenderObject& o) {
    // Staged lookup: a just-imported .fbx and a generated mesh, until the next injection or flush lands its publish
    // (KH_MESH_GEN_PUB, KH_EA_FBX_NO_PARK), are appended but not published; mesh_id_clamp would hand back the box. Game
    // thread only.
    const MeshDef* khn_sd = kh_mesh_def_staged(o.mesh);
    const MeshDef& md = khn_sd ? *khn_sd : mesh_def(0);

    for (int k = 0; k < 3; ++k) {
        // Reads size_mul and writes size, so running twice is a no-op.
        const float khn_m = o.size_mul[k] < 0.0f ? -o.size_mul[k] : o.size_mul[k];
        o.size[k] = md.native_size[k] * (khn_m > 0.0f ? khn_m : 1.0f);
        // KH_SIZE_FINITE: a finite multiplier past what a float carries times the mesh's own size is taken as 1,
        // as a zero or NaN one is (the size parser refuses a non-finite multiplier outright).
        if (!(fabsf(o.size[k]) <= 3.0e38f)) o.size[k] = md.native_size[k];
    }
}

// The textured-path gate shared by both loops: solid (effect 0), non-fullscreen, and carrying at least one used
// material slot.
inline const KhMaterialSet* kh_obj_textured(const RenderObject& o) {
    return (o.effect == 0 && !o.fullscreen && o.materials && o.materials->any)
         ? o.materials : nullptr;
}
// KH_SHOW_INFRONT: whether the main camera's passes draw the object - its colour and the pixels it owns (the seam
// footprints, the DLS mask). kh_obj_vis, and showInFront or a MEASURED view that is not first person: the cycle that
// last ended was watched and drew no view-model slice (one frame late at a view switch, as inFront's own casting
// is). Unmeasured - the first frames after such a mesh appears, turns visible or takes showInFront false with no
// detector running - it is held hidden, so the first-person camera never flashes it. The shadow follows it
// (kh_shadow_part, below): hidden this way, a mesh casts only if casterOnly, exactly as visible false. A PIP (another
// camera, never first person) tests visible - an inFront mesh is never in it, so that is kh_obj_vis there.
inline bool kh_obj_shown(const RenderObject& o) {
    if (!kh_obj_vis(o)) return false;
    if (o.show_in_front) return true;
    return g_vm_slice_known.load(std::memory_order_relaxed) && !g_vm_slice_live.load(std::memory_order_relaxed);
}
inline bool kh_shadow_part(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.lit && o.mode != DepthMode::Off &&
           (kh_obj_shown(o) || o.caster_only) &&   // KH_SHOW_INFRONT: drawn, or casterOnly.
           (!o.in_front || g_vm_slice_live.load(std::memory_order_relaxed));   // KH_INFRONT.
}
inline bool kh_cast_on(const RenderObject& o) {
    return o.cast_shadow && !kh_shadows_off();   // KH_SHADOW_SWITCH, KH_SHADOW_OFF.
}
// KH_DLSW_OWN: kh_shadow_part without its lit term - a world mesh that would take part but for being unlit. Pixel
// ownership only (the DLS world mask, kh_dls_frame); never a caster or receiver test.
inline bool kh_shadow_part_unlit(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.mode != DepthMode::Off &&
           (kh_obj_shown(o) || o.caster_only) &&   // KH_SHOW_INFRONT.
           (!o.in_front || g_vm_slice_live.load(std::memory_order_relaxed));   // KH_INFRONT.
}
inline bool kh_shadow_active(const RenderObject& o) {
    return kh_cast_on(o) && kh_shadow_part(o);
}
// KH_SKIN_CASTER_VM: kh_shadow_active without its inFront slice term - the maps take the object in some view. The
// posing gates' (kh_skin_pose_drawn, kh_uvs_step): equal to kh_shadow_active for every object but an inFront one.
inline bool kh_shadow_active_any_view(const RenderObject& o) {
    return kh_cast_on(o) && !o.fullscreen && o.effect == 0 && o.lit && o.mode != DepthMode::Off &&
           (kh_obj_shown(o) || o.caster_only);
}
}