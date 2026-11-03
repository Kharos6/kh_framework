#pragma once

// The feature half (rendering_integration_features.hpp): the per-object helpers: visibility, placement and rotation,
// bounds and distances, the shadow flags, the handle ids. Never include it on its own.
namespace RenderIntegration {

// INFRONT_EFFECT: whether the view-model slice draws the object - the flag on a solid mesh. An effect mesh ignores
// inFront (RenderObject::in_front): the world routes draw it and it counts as no slice demand; a pass carries no
// inFront flag (the property is a mesh's). Every "drawn in the slice" test reads this, never the field; the tests
// already limited to solid meshes (shadow_part, is_composite_eligible, the slice's own walk) read the field.
inline bool obj_in_front(const RenderObject& o) { return o.in_front && o.effect == 0; }
// SHOW_INFRONT: visible as far as any view can draw it - visible false, and an inFront mesh with showInFront false
// (never drawn: the slice is first person by definition), are alike for every route. The test wherever visible is
// read for a superset or a demand (posing, the pre-filter, the replay rectangles, the mirror, the censuses); the PIP
// tests visible itself (is_composite_eligible already keeps inFront meshes out of it).
inline bool obj_vis(const RenderObject& o) { return o.visible && (o.show_in_front || !obj_in_front(o)); }

// POS_RES - an attached mesh's centre finer than a world float holds. The step composes it in double (the helper,
// the PXY_SMOOTH delta, attachPosition, a skeletal mesh's centre) and stores the nearest floats in pos and the
// rest in pos_res (engine axes, at most half a float step); every draw that places the mesh adds pos_res where it
// forms a camera- or anchor-relative centre (fill_center_rel, the object record's res lane), so the drawn position
// carries no world-scale rounding of ours. The residual belongs to the pos it was taken for (pos_res_at): any other
// writer that changes pos retires it without knowing it exists (one that stores the very floats it was taken for
// leaves it live - at most half a float step from what that writer meant). The world-absolute lanes (centerSize,
// the record's pos) keep pos alone; the bounds cover the residual (bounds_size_of grows by it) and an occluder's
// box is pulled in by it (vis_occ_build), so no test is off by it; every cache keyed on a caster's position keys
// on its residual too, and a simulation's frame and a collider enter exact (cloth_sync).
inline void obj_pos_res(const RenderObject& o, float e[3]) {   // Engine axes; zero when retired.
    if (memcmp(o.pos, o.pos_res_at, sizeof(o.pos)) == 0) {
        memcpy(e, o.pos_res, sizeof(o.pos_res));
    } else {
        e[0] = e[1] = e[2] = 0.0f;
    }
}
// The exact centre (SQF order) into pos and its residual: true when either changed (the step's moved test).
inline bool pos_set_exact(RenderObject& o, const double p[3]) {
    const float f[3] = { static_cast<float>(p[0]), static_cast<float>(p[1]),
                         static_cast<float>(p[2]) };
    const float r[3] = { static_cast<float>(p[0] - static_cast<double>(f[0])),   // Engine axes.
                         static_cast<float>(p[2] - static_cast<double>(f[2])),
                         static_cast<float>(p[1] - static_cast<double>(f[1])) };
    const bool ch = memcmp(o.pos, f, sizeof(f)) != 0 ||
                    memcmp(o.pos_res_at, f, sizeof(f)) != 0 ||
                    memcmp(o.pos_res, r, sizeof(r)) != 0;
    memcpy(o.pos, f, sizeof(f));
    memcpy(o.pos_res_at, f, sizeof(f));
    memcpy(o.pos_res, r, sizeof(r));
    return ch;
}
// The centre as the draws place it (pos plus a live residual), SQF order.
inline void pos_exact(const RenderObject& o, double p[3]) {
    float e[3];
    obj_pos_res(o, e);
    p[0] = static_cast<double>(o.pos[0]) + static_cast<double>(e[0]);
    p[1] = static_cast<double>(o.pos[1]) + static_cast<double>(e[2]);
    p[2] = static_cast<double>(o.pos[2]) + static_cast<double>(e[1]);
}

// The matrix for a [pitch, yaw, roll] triple in ARMA degrees, in ENGINE axes. The one builder of an EULER rotation's
// matrix (the [vectorDir, vectorUp] form is rotation_from_vectors'), at TWO call sites: set_rotation below, and
// kh_rotation_m_from_gv in sqf_integration.hpp, the parser every other typed rotation (attachRotation, a chain's
// endRotation) takes its matrix from - so ATTACH_OFFSET's attach-space rotation cannot drift from the object's.
inline void rotation_matrix(float m[9], float pitch_deg, float yaw_deg, float roll_deg) {
    if (pitch_deg == 0.0f && yaw_deg == 0.0f && roll_deg == 0.0f) {
        const float id[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        memcpy(m, id, sizeof(id));
        return;
    }

    const float d2r = 0.017453292519943295f;
    const float cp = cosf(pitch_deg * d2r), sp = sinf(pitch_deg * d2r);
    const float cy = cosf(yaw_deg * d2r),  sy = sinf(yaw_deg * d2r);
    const float cr = cosf(roll_deg * d2r), sr = sinf(roll_deg * d2r);
    const float roll[3][3]  = { { cr, 0.0f, -sr }, { 0.0f, 1.0f, 0.0f }, { sr, 0.0f, cr } };
    const float pitch[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, cp, sp }, { 0.0f, -sp, cp } };
    const float yaw[3][3]   = { { cy, -sy, 0.0f }, { sy, cy, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    float rp[3][3];
    float ma[3][3];

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            rp[r][c] = roll[r][0] * pitch[0][c] +
                       roll[r][1] * pitch[1][c] +
                       roll[r][2] * pitch[2][c];
        }
    }

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            ma[r][c] = rp[r][0] * yaw[0][c] +
                       rp[r][1] * yaw[1][c] +
                       rp[r][2] * yaw[2][c];
        }
    }

    // Arma -> engine similarity: engine index i reads arma index p(i), p = (0, 2, 1), its own inverse.
    const int p[3] = { 0, 2, 1 };

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            m[r * 3 + c] = ma[p[r]][p[c]];
        }
    }
}

// rot / rotated are the euler the script typed; rot_m is the matrix. rotated follows the EULER here (a typed zero is
// not rotated); the attachment path has its own test, attach_rotated, because a raw basis has no euler.
inline void set_rotation(RenderObject& o, float pitch_deg, float yaw_deg, float roll_deg) {
    o.rot[0] = pitch_deg;
    o.rot[1] = yaw_deg;
    o.rot[2] = roll_deg;
    o.rotated = pitch_deg != 0.0f || yaw_deg != 0.0f || roll_deg != 0.0f;
    rotation_matrix(o.rot_m, pitch_deg, yaw_deg, roll_deg);
}

// ROT_VECTORS - the matrix for a [vectorDir, vectorUp] pair in SQF's axes (x east / right, y north / forward, z
// up), as setVectorDirAndUp takes it: the direction kept, the up made square to it, aside = up x dir. The rows are
// the ones rotation_matrix and a raw read hand back (aside / up / dir, engine axes: x east, y up, z north). False
// for a zero, non-finite or (to within 0.06 degrees) parallel pair.
inline bool rotation_from_vectors(float m[9], const float dir[3], const float up[3]) {
    float d[3] = { dir[0], dir[2], dir[1] };   // SQF -> engine axes.
    float u[3] = { up[0], up[2], up[1] };
    const float dl = cloth_v3_len(d);
    const float ul = cloth_v3_len(u);
    if (!(dl > 1.0e-6f && dl < 1.0e30f) || !(ul > 1.0e-6f && ul < 1.0e30f)) return false;
    for (int k = 0; k < 3; ++k) { d[k] /= dl; u[k] /= ul; }
    const float rv_k = cloth_v3_dot(u, d);
    for (int k = 0; k < 3; ++k) u[k] -= rv_k * d[k];
    const float sl = cloth_v3_len(u);
    if (!(sl >= 1.0e-3f)) return false;
    for (int k = 0; k < 3; ++k) u[k] /= sl;
    m[0] = u[1] * d[2] - u[2] * d[1];
    m[1] = u[2] * d[0] - u[0] * d[2];
    m[2] = u[0] * d[1] - u[1] * d[0];
    for (int k = 0; k < 3; ++k) { m[3 + k] = u[k]; m[6 + k] = d[k]; }
    return true;
}
// ROT_VECTORS - set_rotation's twin for the vector form: no euler (zero), the matrix as given, rotated unless
// it is exactly the identity (attach_rotated's test).
inline void set_rotation_rows(RenderObject& o, const float m[9]) {
    static const float id[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    o.rot[0] = 0.0f;
    o.rot[1] = 0.0f;
    o.rot[2] = 0.0f;
    memcpy(o.rot_m, m, sizeof(o.rot_m));
    o.rotated = memcmp(m, id, sizeof(id)) != 0;
}

// Enclosing half extents (engine axes) of a rotated, per-axis-scaled mesh cube: he_k = sum_j |R[j][k]| * hl_j, the
// OBB -> AABB bound - exact for axis-aligned rotations, conservative otherwise. hl = engine-local half extents.
inline void rot_half_extents(const float hl[3], const float* rot_m, bool rotated, float he[3]) {
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

// CAST_REFIT: the size every BOUNDS test takes - the script's, grown per axis to what a plain cloth's simulation
// reached (RenderObject::cast_size). LOD_BOUNDS: then grown per axis, about the centre, to the mesh's every-level
// bounds (MeshDef::occ_hmn / occ_hmx, made for every mesh by mesh_occ_prep): a decimated level can leave the unit
// box, and which level is drawn is not a bounds test's to know. A mesh inside the unit box (every builtin) is scaled
// by exactly 1; an import's level 0 can sit a rounding outside it (the squash's (p - ctr) / ext), carried too.
// KH_USER_VS: plus the vertex stage's declared reach on both sides of every axis (MaterialSet::vertex_bound) -
// the deformation happens on the GPU, so the script's promise is all a bounds test has. Untouched without one.
// POS_RES: plus twice the residual's length on every axis, divided by the smallest row scale of a scaled
// rotation: the mesh is drawn at pos plus its residual, which may point anywhere, and the box about pos then holds
// all of it after any rotation. Untouched with no live residual.
inline void bounds_size_of(const RenderObject& o, float sz[3]) {
    for (int k = 0; k < 3; ++k) sz[k] = o.cast_size[k] > o.size[k] ? o.cast_size[k] : o.size[k];
    {
        const MeshDef& md = mesh_def(mesh_id_clamp(o.mesh));
        static const int ax[3] = { 0, 2, 1 };   // SQF axis -> engine axis.
        for (int k = 0; k < 3; ++k) {
            const int e = ax[k];
            sz[k] *= 2.0f * fmaxf(fmaxf(-md.occ_hmn[e], md.occ_hmx[e]), 0.5f);
        }
    }
    if (o.materials && o.materials->vertex_any && o.materials->vertex_bound > 0.0f) {
        for (int k = 0; k < 3; ++k) sz[k] += 2.0f * o.materials->vertex_bound;
    }
    {   // POS_RES.
        float r[3];
        obj_pos_res(o, r);
        const float rl = sqrtf(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
        if (rl > 0.0f) {
            float g = 2.0f * rl;
            if (o.rotated) {
                for (int rr = 0; rr < 3; ++rr) {
                    const float* w = &o.rot_m[rr * 3];
                    const float n = sqrtf(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
                    if (n > 1.0e-6f && n < 1.0f) g = fmaxf(g, 2.0f * rl / n);
                }
            }
            for (int k = 0; k < 3; ++k) sz[k] += g;
        }
    }
}
inline void world_half_extents(const RenderObject& o, float he[3]) {
    float sz[3];
    bounds_size_of(o, sz);   // CAST_REFIT.
    const float hl[3] = { sz[0] * 0.5f, sz[2] * 0.5f, sz[1] * 0.5f };
    rot_half_extents(hl, o.rot_m, o.rotated, he);
}


// The distance mesh_dist_sq (below) takes, over a raw record: drawn_lod (the level of the depth-only passes
// that re-draw the scene's meshes) and the dlsw mask (its DlswCaster, copied from a SunCaster) pick their LOD with
// it and must land on the level the injection drew.
inline float mesh_dist_sq_of(const float* pos, const float* size, const float* rot_m,
                             bool rotated, const float cam[3]) {
    const float c[3] = { cam[0], cam[2], cam[1] };   // Engine -> SQF axes.
    const float hl[3] = { size[0] * 0.5f, size[2] * 0.5f, size[1] * 0.5f };
    float he[3];
    rot_half_extents(hl, rot_m, rotated, he);
    const float hs[3] = { he[0], he[2], he[1] };   // SQF axes.
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

inline float mesh_dist_sq(const RenderObject& o, const float cam[3]) {
    return mesh_dist_sq_of(o.pos, o.size, o.rot_m, o.rotated, cam);
}
// NEARZ_BOUNDS: the same distance to the box around everything the object may draw (bounds_size_of's size).
// The near-z route (DistMemo::get_drawn, the injection's nearest clear) and the mirror (rp_box_dist's band,
// rp_mir_reads) take it; the LOD pick keeps mesh_dist_sq (the script's size: CAST_REFIT's rule).
inline float bounds_dist_sq(const RenderObject& o, const float cam[3]) {
    float s[3];
    bounds_size_of(o, s);
    return mesh_dist_sq_of(o.pos, s, o.rot_m, o.rotated, cam);
}

// SHADOW_OFF - a shadow view distance of SHADOW_OFF_M or less (the video options' shadow view distance,
// getObjectViewDistance's second element, read by stage_video_options on the game thread whoever owns g_sun_range)
// turns every shadow of ours off: every mesh casts and receives as if castShadow and receiveShadow were both false
// (SHADOW_SWITCH) - no caster in any gather or census (cast_on, shadow_active: no sun, far or DLS map, no
// world cast, no DLS world pass) and no received term (the khObjNoRecv lanes: the sun's bands / atlas, the stencil
// and mirror, the self ladder, the DLS shadows; the receive lanes read g_objbuf_shoff, the object records' own copy,
// objbuf_sync). What serves only those terms stands down through a path it already has: the DLS light choice
// (dls_select's allowDynamicShadows-off branch: no slot, so no map and no world pass); the volume copy
// (svs_vol_copy's no-reader branch, which drops the primed flag); the mask snapshots and the prime
// (svs_snap_wanted false); the volume seam's arm (svs_inject_armed false: no world footprint in the engine's
// volume pass, so no recording, replay, mirror prepass, patch, mirror replay or census take) with the in-front seam
// and the replay's constant-buffer images (rp_img_note). Every frame while off, volume_seam_frame_reset
// leaves nothing of that path ready (copy, snapshots, mirror) and releases the last recorded pass. What stays is
// what non-shadow work reads: the engine-side capture (its cascade axes are the sun direction that lights every mesh
// - sun_raw_note), the constant-buffer census (a blanket term of the upload funnel the lighting probes ride -
// cbc_on), the dynamic lights' harvest (dl_cpu_wanted: a lit mesh or a dynamicLightFog pass) and the
// engine-mask tracking the capture shares.
static constexpr float SHADOW_OFF_M = 5.0f;
inline bool shadows_off() { return g_shadows_off.load(std::memory_order_relaxed); }

// Handles are opaque strings from the framework's UID generator, prefixed "khr_"; command errors never carry the
// prefix, so callers distinguish success by it. String, not numeric: SQF scalars are 32-bit floats, integer-exact
// only to 2^24.
static uint64_t g_next_seq = 0;

inline std::string make_render_uid() {
    return std::string("khr_") + UIDGenerator::generate();
}

inline void obj_center_engine(const RenderObject& o, float out[3]) {
    out[0] = o.pos[0];
    out[1] = o.pos[2];
    out[2] = o.pos[1];
}

// Casting is the object's, not the material's. The mirror prepass and the prime draw every caster whole; the sun
// tiers and DLS maps draw an alpha caster (colour alpha below 1, or a cutout / blend slot) through their alpha twins,
// an untextured one whole with its colour alpha, a textured one per submesh with its material (sun_draw_alpha_t);
// the seam footprint leaves out a caster with colour alpha below 1 and draws a cutout / blend one per submesh
// through its alpha twin (footprint_alpha_on). Without its alpha twins a pass has no alpha caster.

inline bool obj_has_blend(const RenderObject& o) {
    if (!o.materials || !o.materials->any) return false;
    for (const Material& m : o.materials->slots) if (m.used && m.alpha_mode == 2) return true;
    return false;
}

// Material draw support (both loops share these).

inline void apply_native_size(RenderObject& o) {
    // Staged lookup: a just-imported .fbx and a generated mesh, until the next injection or flush lands its publish
    // (MESH_GEN_PUB, EA_FBX_NO_PARK), are appended but not published; mesh_id_clamp would hand back the box. Game
    // thread only.
    const MeshDef* sd = mesh_def_staged(o.mesh);
    const MeshDef& md = sd ? *sd : mesh_def(0);

    for (int k = 0; k < 3; ++k) {
        // Reads size_mul and writes size, so running twice is a no-op.
        const float m = o.size_mul[k] < 0.0f ? -o.size_mul[k] : o.size_mul[k];
        o.size[k] = md.native_size[k] * (m > 0.0f ? m : 1.0f);
        // SIZE_FINITE: a finite multiplier past what a float carries times the mesh's own size is taken as 1,
        // as a zero or NaN one is (the size parser refuses a non-finite multiplier outright).
        if (!(fabsf(o.size[k]) <= 3.0e38f)) o.size[k] = md.native_size[k];
    }
}

// The textured-path gate shared by both loops: solid (effect 0), non-fullscreen, and carrying at least one used
// material slot.
inline const MaterialSet* obj_textured(const RenderObject& o) {
    return (o.effect == 0 && !o.fullscreen && o.materials && o.materials->any)
         ? o.materials : nullptr;
}
// SHOW_INFRONT: whether the main camera's passes draw the object - its colour and the pixels it owns (the seam
// footprints, the DLS mask). obj_vis, and showInFront or a MEASURED view that is not first person: the cycle that
// last ended was watched and drew no view-model slice (one frame late at a view switch, as inFront's own casting
// is). Unmeasured - the first frames after such a mesh appears, turns visible or takes showInFront false with no
// detector running - it is held hidden, so the first-person camera never flashes it. The shadow follows it
// (shadow_part, below): hidden this way, a mesh casts only if casterOnly, exactly as visible false. A PIP (another
// camera, never first person) tests visible - an inFront mesh is never in it, so that is obj_vis there.
inline bool obj_shown(const RenderObject& o) {
    if (!obj_vis(o)) return false;
    if (o.show_in_front) return true;
    return g_vm_slice_known.load(std::memory_order_relaxed) && !g_vm_slice_live.load(std::memory_order_relaxed);
}
inline bool shadow_part(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.lit && o.mode != DepthMode::Off &&
           (obj_shown(o) || o.caster_only) &&   // SHOW_INFRONT: drawn, or casterOnly.
           (!o.in_front || g_vm_slice_live.load(std::memory_order_relaxed));   // INFRONT.
}
inline bool cast_on(const RenderObject& o) {
    return o.cast_shadow && !shadows_off();   // SHADOW_SWITCH, SHADOW_OFF.
}
// DLSW_OWN: shadow_part without its lit term - a world mesh that would take part but for being unlit. Pixel
// ownership only (the DLS world mask, dls_frame); never a caster or receiver test.
inline bool shadow_part_unlit(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.mode != DepthMode::Off &&
           (obj_shown(o) || o.caster_only) &&   // SHOW_INFRONT.
           (!o.in_front || g_vm_slice_live.load(std::memory_order_relaxed));   // INFRONT.
}
inline bool shadow_active(const RenderObject& o) {
    return cast_on(o) && shadow_part(o);
}
// SKIN_CASTER_VM: shadow_active without its inFront slice term - the maps take the object in some view. The
// posing gates' (skin_pose_drawn, uvs_step): equal to shadow_active for every object but an inFront one.
inline bool shadow_active_any_view(const RenderObject& o) {
    return cast_on(o) && !o.fullscreen && o.effect == 0 && o.lit && o.mode != DepthMode::Off &&
           (obj_shown(o) || o.caster_only);
}
}