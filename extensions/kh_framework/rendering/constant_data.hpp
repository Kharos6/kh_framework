#pragma once

// The shared head (rendering_integration.hpp): the constant-buffer layout (ConstantData), the HLSL unit ids and the CB
// offsetting flags. Never include it on its own.
namespace RenderIntegration {

// Shaders convention: row_major, clip = p * viewProj.

// HLSL sources are concatenated per unit; no #include, since the cache key hashes the assembled source. Rules: fxc
// has no linker pass, so a function body must precede its caller in the unit; cb.hlsl is the shared prefix (a helper
// both chains need goes there); twins move together (flush / injection mesh loops, PSMain / PSComposite / PSEffect,
// the HLSL and C++ sides of the CB, the two sun-verdict chains); any .hlsl edit changes that unit's cache key, so
// batch shader edits.
enum HlslRes : int {
    HLSL_CB = 0,          // The shared prefix (cb.hlsl).
    HLSL_WHITE,
    HLSL_STATIC,          // Static entry points (the mesh, sun-depth, seam, mirror, mask, DLS and replay-merge
                             // passes); some read the scene depth at t0 (PSMain's depth-off solids, PSMaskCast,
                             // PSDlsWorld / PSDlsWorldFog).
    HLSL_COMPOSITE,       // Injected path.
    HLSL_EFFECT,          // Effect uber-shader (t0 scene, t1 depth).
    HLSL_DEPTH_RESOLVE,
    HLSL_SUNPF,
    HLSL_VMIR_CS,         // Standalone b2 patch (MIR_DRAW: a draw despite the name).
    HLSL_SSAO,            // SSAO: the AO passes and the near-plane marker (standalone).
    HLSL_SKIN,            // SKIN_GPU: the stream-output skinning vertex shader (standalone).
    HLSL_N
};

struct alignas(16) ConstantData {
    // per-object block (b0).
    float center_size[4];
    float center_rel[4];
    float color[4];
    float fx0[4];   // Effect parameters [0..3]; on the mesh fill sites xyz = the camera.
    float fx1[4];
    float fx_meta[4];   // Effect id, time (s), screen width, screen height.
    float depth_params[4];
    float size_axes[4];   // Xyz = mesh edge lengths (engine axes), w = blend mode id.
    float local0[4];   // Xyz = mask center (engine space), w = shape (0 sphere, 1 cube).
    float local1[4];   // x = falloff (normalized to mean radius), y = localized flag.
    float local_radii[4];   // Xyz = mask radii (engine axes).
    float band0[4];   // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w =
                      // banded flag.
    float lighting0[4];   // x = lit flag, y = 1 receiveShadow off (SHADOW_SWITCH) or every shadow off
                          // (SHADOW_OFF, g_objbuf_shoff), z = ambient, w = diffuse.
    // x = 1 on an effect-chain pass whose near-plane marker (t37) is this cycle's (NEARZ_MARK), else 0; y =
    // object view-distance cut (m, 0 = off); z = 1 for a depth-Off overlay (VOL_WITNESS: no footprint, no
    // witness; fill_lighting_obj_cb), else 0; w = the first vertex of the run VSUserSo is drawing (KH_USER_VS's
    // pass), or on an effect-chain pass the VM_SEE arm (fx_chain_run: 1 = the depth behind the view model's
    // translucent planes, 2 = also their transmittance); 0 everywhere else.
    float shadow_meta2[4];
    float obj_rot[3][4];   // [0][3] = 1 marks filled; the zeroed default reads as identity
                           // in-shader, so an auxiliary fill that leaves it zero stays correct.
    float blend_ctl[4];   // x = perceptual-composite enable (the flush's fill only; the injection writes 0), w = the
                          // LOD crossfade dither; y unread; z = CUTOUT_A2C: 1 keeps a cutout's hard clip on a
                          // multisampled target (the view-model slice, infront_inject), 0 everywhere else (alpha
                          // to coverage, cb.hlsl KhCutoutCover).
    float mat_ctl[4];   // MAT_TABLE (HLSL twin matCtl): x = table index (base + slot) for the non-instanced VS,
                        // y = slot for the instanced VS, z = 1 when x holds for every instance, w = alpha-mode
                        // override (-1 none). The material lanes themselves live in the table (KhGpuMat). On an
                        // effect pass y / z are FX_SIDE's arms instead (the scene chain's fog scatter / sun
                        // flare, the UI lane's probe), zero unless the pass drew its side value first, and x / w
                        // FX_LINZ's (a scene-chain SSGI / dynamicLightFog / fog scatter pass's linear-depth
                        // copies), zero unless one serves the pass.
    float fuse_meta[4];   // x = the fused stages (fuse_append); y, zw = GLOW_PYR's arm and extent
                          // (glow_lanes), or on an anamorphic pass ANA_PYR's arm and frame (ana_lanes).
    float fuse_stage[12][4];   // Per stage s: [4s] = effect, blend, age (s; FUSE_CLOCK), 0; [4s + 1] /
                               // [4s + 2] = fx[0..3] / fx[4..7]; [4s + 3] = colour (fuse_append).
    // fx2 sits in the append region (append-only block discipline); HLSL twin fxParams2. fill_fx_params_cb copies
    // the object's params 8 - 11 into it (the flush's fills, the PIP's effect pass, the UI lane); the mesh fills of
    // the injection, the PIP and the in-front slice leave the template's zero; a fog-scatter object's fill zeroes it,
    // a fog-scatter pass (the scene chain's, the PIP's) packs the fog passes into it (fogscatter_pack),
    // dynamicLightFog's pass writes xy = whether each fog record's pass draws before it (dlf_fog_pack), and the
    // SSGI resolve writes an auto width into w.
    float fx2[4];
    float dl_ctl[4];   // x = mode (3 absolute pool, armed; 0 off - the only two values written), y = point count,
                       // z = spot count, w = distance scale.
    float dl_global[4];   // Xyz = global diffuse multiplier (cb10[3]); w = the global intensity (g_dl_intensity_bits,
                          // 1: no command sets it) - DynLights scales its whole sum by w, so a zero here is 'lights
                          // off', never a padding lane.
    float dl_view[3][4];   // Unused (HLSL dlView): the retired mode 2's lane, kept for the layout.
    // DL_RING: x = where this draw's light records start in the light ring (t40, HLSL khDlRecs), in float4
    // elements, as uint BITS (asuint in the shader; exact past 2^24); yzw unused (zero). The records themselves - the
    // engine's 6-float4 cb11 record per light, points first then spots, dl_ctl.y + dl_ctl.z of them - live in the
    // ring, not here, so a mesh takes every light that reaches it with no count cap (dl_select).
    float dl_first[4];
    // USER_LANES (HLSL twin khUserObj): x = the session clock (s, effect_time_seconds), y = this object's creation
    // on that clock (s, fx_t0) - x - y is the object's own age, the effects' fxMeta.y; zw unused (zero). Filled by
    // fill_user_obj_cb from fill_lighting_obj_cb, fill_fx_params_cb and uvs_step (every object fill of a
    // pass that can run a user shader); zero elsewhere.
    float user_obj[4];
    // FX_TEX (HLSL twin khFxTexLay): an effect's textures, 1 + each one's layer in its page ([0].xyzw =
    // user0 .. user3, [1].xy = user4 / user5; 0 = absent or still loading). Written by fx_tex_fill
    // on a user effect's draw and (FX_TEX_ALL, fx_tex_on) a builtin effect pass's with textures; zero elsewhere.
    // [1].z = WSEE's mode on a scene-chain pass and its side draws (fx_chain_run: 1 = armed; 0 = off - every
    // other draw: effect meshes, the UI lane, the PIP, a cycle without a valid capture) and a sorted part's mode on
    // its mesh draw (wsee_sort_mode: 1 = the occlusion test, 2 / 3 = and the layer-aware composite);
    // fx_tex_fill leaves it.
    // [1].w = WSEE's bound views on an armed scene-chain pass, read as (uint)(w + 0.5) (fx_chain_run; the shader
    // cannot test a null view): the bit table is the WSEE note's LANES paragraph (rendering_integration_injection.
    // hpp) and cb.hlsl's KhWseeAvail; bit 1 (C) is the pass's own draw's alone, never its side draws'; on a sorted
    // part's mesh draw bits 3, 5 - 9 alike (WseeSortScope); 0 on every other draw. fx_tex_fill leaves it too.
    float fx_tex[2][4];
    // FX_FOGREC (HLSL twin khFxFog): the scene's KH fog passes for a user effect - fog_records_pack's two records
    // [startDist, endDist, skyAmount, opacity] (opacity 0 = none), filled for a user effect in the scene chain and
    // (DLF_FOGREC, dlf_fog_pack) for dynamicLightFog's pass and its side draws; zero elsewhere. Last per-object
    // field: the per-frame block (b1) starts at view_proj.
    float fx_fog[2][4];
    float view_proj[4][4];   // Rebased (relative to the pass camera; for the sun-depth casters, to the map's origin) on
                             // the passes that arm center_rel.w through fill_center_rel: the flush and injection
                             // mesh passes, the PIP, the view-model slice, the volume and in-front seams, the stencil
                             // prime and DLS world masks, the sun-depth casters. Absolute at every other site.
    float inv_view_proj[4][4];   // Inverse of the live encode for the depth reconstruction: of the rotation-only
                                 // view when fx_cam.w = 1 (the shader adds fx_cam.xyz back), absolute when
                                 // fx_cam.w = 0 (FX_CAM_REL).
    float lighting1[4];   // Xyz = unit vector toward the light (engine axes), w = valid flag.
    float lighting2[4];   // Rgb = the engine's sun colour in HDR scene units, copied straight from the published
                          // lighting block (LIGHT_BASE; (1,1,1) before the first publish), w = shadow-map strength.
    float shadow_meta[4];   // Count, compare sign, bias, atlas size.
    float shadow_tiles[8][4];   // per-cascade atlas UV rects.
    float shadow_mats[24][4];   // per-cascade 4x3 sampling transforms.
    float band_mat[24][4];
    float band_view[24][4];
    float band_border[8][4];
    float cast_mat[3][4];
    float cast_view[3][4];
    // Bands (then the live table) are the receive truth; the mask machinery that remains serves the cast write only,
    // which is registration-exact by construction. x = view-paired band table valid; w = the unit-stencil arm
    // (svs_unit_on), read by the two colour twins. y/z unwritten.
    float mask_meta[4];
    // Fog + engine-mask receive.
    float fog_params[4];   // x = fogValue, y = fogDecay, z = fogBase, w = enable.
    float fog_color[4];   // Rgb = fog color approximation, w = camera altitude ASL (relative).
    // Private sun-depth map (mirrors sunVP/sunMeta in CBFrame).
    float sun_vp[4][4];   // row-vector world -> sun-depth clip.
    float sun_meta[4];   // x = valid, y = map size (px), z = compare bias (normalized depth units), w = shadow-map
                         // strength.
    // x = per-caster locality pair count; y = 1 -> the uncapped t2 list carries them; z = sun map older than 0.5 s
    // (the filtered-compare gate); w = OCC_N, the cast-occupancy grid's edge in texels, so PSMaskCast bounds its
    // Load against the CB rather than a literal; an unwritten 0 leaves the grid unarmed (PSMaskCast and
    // cast_scissor alike).
    float locality_meta[4];
    float locality[32][4];   // [2i] = center.xyz, [2i+1] = half extents.xyz.
    float light_amb[4];   // Rgb = engine ambient (HDR) or (1,1,1) cold; w = engine mode.
    float fog_engine[4];   // x = density scale (blk 41), y = fog end (blk 48), z = inv ramp range (blk 49),
                           // w = 0 invalid, 1 valid, 2 valid with the ramp off (end x inv range < 1).
    float fog_sky[4];   // The fog colour's view-elevation gradient: x nadir, y horizon, z zenith gain, w = armed.
                        // FOG_SKY_DOC: each fill writes (1, 1, 1, 1) once the sky probe has matched, inside its
                        // fog / haze / engine-terms block (gain 1: the fog target is fog_sky_col.rgb); the sky
                        // block's row 17 goes to fog_uw_grad.
    float fog_sky_col[4];   // Sky block row 1 (nb[4..6]): the above-layer fog target colour (FOG_SKY_DOC); w = the
                            // engine fog-layer altitude (the lighting block's row 12 z + row 13 y, fill_haze; 0
                            // unless the haze arms), the branch point KhHazeT integrates against.
    float haze_pars[4];   // Engine distance haze (sky CB row 14 = PSC_HazePars): x = reference altitude (m),
                          // y = density, z = height falloff (/m), w = armed.
    float thm_params[4];
    float thm_meta[4];
    // The volume transport. HLSL twins stenVol / stenVol2 (mirror contract, same relative slot). sten_vol: z/w = the
    // volume copy's own dimensions (not fx_meta.zw), x/y = the copy's depth encode pair (m22, m32) for VOL_WITNESS
    // (fill_sten_reproj; zero = no witness).
    float sten_vol[4];
    // x = transport arm (1 = read the volume stencil); z = KhVsCore vertex path selector (3 = the seam prepass);
    // y/w = the copy's viewport depth range (min, max) for VOL_WITNESS (zero = no witness).
    float sten_vol2[4];
    // Hero sun map - HLSL twins sunVP2 / sunMeta2 (mirror contract).
    float sun_vp2[4][4];   // World -> hero sun-depth clip (row-vector).
    float sun_meta2[4];   // x = valid, y = map size (px), z = compare bias, w = half-diag (m).
    // Frame data - one per pass.
    float sun_vp3[4][4];   // World -> mid-band sun-depth clip (t26).
    float sun_meta3[4];   // x = valid, y = map size (px), z = compare bias, w = half-diag (m).
    float sun_vp4[4][4];   // World -> outer-band sun-depth clip (t27).
    float sun_meta4[4];   // x = valid, y = map size (px), z = compare bias, w = half-diag (m).
    // x = mask valid this frame, y/z = mask dims (px). w is NOT free: it is the sun shadow range,
    // clamp(g_sun_range, 8, 1000) m, written by fill_lighting_frame_cb and by the cast fill whenever a sun anchor
    // exists. Every sun shadow fades out over the last 28% of it (KhSunRangeFade / KhSunRangeFadeAt, which return
    // 1.0 unfaded when the lane reads < 0.5), so zeroing this lane turns the range fade off, it does not free a slot.
    float mir_meta[4];
    float sun_origin[4];   // xyz = the sun anchor; w = the far tier's prefilter arm (FAR_PF).
    // x = engine below-layer extinction (block nb[51]), y = armed; z = the cast fire's frozen-depth ulp, relative
    // (CAST_SNAP_FACING, PSMaskCast only - both mesh fill sites write 0 and no mesh shader reads it); w free.
    float fog_below[4];
    float fog_uw[4];
    float fog_uw_grad[4];
    float snap_vp[4][4];
    float snap_meta[4];
    float snap_cam[4];
    float sun_pf[4];
    // HLSL twin appended identically.
    float sun_vp5[4][4];
    float sun_meta5[4];
    // World cast compare bias, one lane per tier plus the union (HLSL twins sunCastBias / sunCastBias2).
    // Deliberately not sun_meta*[2]: that lane feeds the self kernel too and may not move. Zero = the shader falls
    // back to sun_meta*[2], which every mesh fill site leaves here.
    float sun_cast_bias[4];    // Hero, mid, outer, far (normalized).
    float sun_cast_bias2[4];   // x = union (normalized).
    // No cube matrices live here, by design: only the spot frustum is arbitrary and therefore stored.
    // xyz = light world position (engine axes), w = far plane (m); w == 0 = no map, the shader answers lit.
    float dls_meta[8][4];
    float dls_ctl[8][4];     // x = 1 spot / 0 cube, y = compare bias constant (metres), z = the spot's slice,
                             // w = bias slope (per metre of range).
    // One float4 per face, indexed slot*6 + face. x = slice for this face (-1 = no slice: nothing occludes in that
    // direction). y = the receiver-normal offset arm, replicated across the light's six entries so it can be read
    // before the face is known. zw free.
    float dls_face_slice[48][4];
    float dls_spot_vp[8][4][4];
    // xyz = pass camera (engine axes), w = clamp(shadowVisibility, 8, 1000); 0 = no fade. Written by dls_fill_cb;
    // the DLS world pass then replaces xyz (w > 0) with its reconstruction camera (cast_view[2].xyz).
    float dls_range[4];
    // Bucket pass lanes (HLSL twins khPass / khPassObj). khPass.xyz = the pass's rebase camera (engine axes), w = 1
    // arms the instanced vertex path's rebase (the instanced VS subtracts the camera itself). khPassObj.x = the
    // engine object view distance (the object view-distance cut); yzw free. Zero on every fill site that draws no
    // bucket.
    float pass_cam[4];
    float pass_obj[4];
    // xyz = the camera the effect unit's inverse view-projection is relative to (engine axes), w = 1 arms it -
    // KhWorldPosFenced and KhFsFog add xyz back, KhWorldPosRelFenced's readers take camera-relative points. w = 0:
    // the inverse is absolute and nothing is added.
    float fx_cam[4];
    // USER_LANES (HLSL twins khUserCam / khUserCamRot): the pass camera. user_cam.xyz = its position (engine
    // axes, absolute), w = 1 when filled; user_cam_rot rows = its right, up and forward axes (engine axes, unit
    // length), w unused (zero). Filled by fill_user_cam_cb on every pass that can run a user shader (the mesh
    // flush and injection, the PIP and view-model passes, the scene and UI effect chains, uvs_step's vertex
    // stage); zero (w = 0) elsewhere.
    float user_cam[4];
    float user_cam_rot[3][4];
    // VOL_ZOOM (HLSL twin stenVol3): the raster map from this pass's projection to the volume copy's seam-frame
    // projection, per axis copy = raster * xy + zw. The copy was drawn under its seam frame's projection (the
    // injection reads it a frame late), and a zoom between that frame and this pass (a projection change, the view
    // unchanged) moves every surface on screen; through the map the count and the witness depth are read on this
    // pixel's own view ray. x = 0 = identity: zero-initialised fills, and sten_zoom_map's own zero (no measured
    // projection on either side, or one of the other handedness). The view-model slice takes its own map (it draws
    // under g_infront.proj: infront_inject). The PIP passes copy the template's map but zero maskMeta, so no stencil
    // is read there.
    float sten_vol3[4];
    // VOL_FOOT (HLSL twin stenVol4): x = 1 when the footprint mask of the copy's seam frame is bound at t33 - the
    // witness then compares against our own footprint's distance there instead of decoding the copy's depth plane.
    // yzw unused (zero). Zero = no mask (the depth-plane witness).
    float sten_vol4[4];
    // PCSS_CELL (HLSL twins sunLat0 / sunLat1): each camera tier's texel lattice offset, as uint BITS (asuint in
    // the shader; mod 2^32) - a texel index of that tier's map plus it is the world-fixed lattice (the floor of the
    // position over the texel along the sun plane's axes) that KhPcssRot hashes. [0] = hero xy, mid zw; [1] = outer
    // xy, far zw. Filled by fill_sun_tiers_cb from the render that made each map; a stood-down tier's pair is
    // unread (its meta is zero).
    float sun_lat[2][4];
    // CB_DERIVED (HLSL twins sunCol / castCam, cb.hlsl's note): never filled by hand - upload_frame_cb forms
    // them from this struct's own matrices at every frame upload (cb_derived), so they cannot drift from what
    // the upload carries. sun_col: each sun map's texel and depth scales, the lengths of its matrix's columns 0 and
    // 2 over rows 0 - 2 - [0] = union xy / hero zw, [1] = mid xy / outer zw, [2] = far xy (zw zero). cast_cam: the
    // frozen mask-cast camera, cast_mat's rows dotted with -cast_view[0].
    float sun_col[3][4];
    float cast_cam[4];
};
static bool     g_ctx1_null_first = false;
// A driver without offsetting drops SetConstantBuffers1, so StateBackup's offset-aware restore must not be attempted
// there or our buffer stays bound. False until ensure_resources reads the caps.
static bool     g_cb_offsetting = false;
}