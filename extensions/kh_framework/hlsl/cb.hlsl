// cb.hlsl - the shared prefix, concatenated by C++ (no #include) ahead of every unit but the standalone ones
// (depth_resolve, sunpf, vmir_cs, ssao, skin). Any edit changes every including unit's shader cache key.
// KH_WSEE_PSB_LS: log2 of the one pass's scale s (s = 2; the TWIN of C++ KH_PSB_LS and PSPsbZb's KH_PSB_ZB_LS) -
// read by both sections' record texel (KhWseePsbAt / KhWseeMsPsbAt).
#define KH_WSEE_PSB_LS 1

cbuffer CBObj : register(b0)
{
    float4 centerSize;   // xyz = world center (engine space), w = edge length.
    // FP32 jitter rebase: xyz = center minus the pass's rebase origin (the camera); w = 1 arms the rebased vertex
    // transform, whose viewProj is then the rebased matrix. w = 0 keeps the absolute path.
    float4 centerRel;
    float4 color;
    float4 fxParams0;   // Effect parameters [0..3].
    float4 fxParams1;   // Effect parameters [4..7].
    float4 fxMeta;   // x = effect id, y = time (s), z = screen width, w = screen height.
    float4 depthParams;   // x = proj m22, y = proj m32, z = viewport MinDepth, w = MaxDepth.
    float4 sizeAxes;   // xyz = mesh edge lengths (engine axes), w = blend mode id.
    float4 localParams0;   // xyz = mask center (engine space), w = shape (0 sphere, 1 cube).
    float4 localParams1;   // x = falloff (normalized to mean radius), y = localized flag.
    float4 localRadii;   // xyz = mask radii (engine axes).
    float4 bandParams;   // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w =
                         // Banded flag.
    // x = lit flag, y = 1 when receiveShadow is off (KH_SHADOW_SWITCH) or every shadow is (KH_SHADOW_OFF), z =
    // ambient fraction, w = diffuse fraction (y / z / w read through KhObjLanesCb / KhObjLoad).
    float4 lighting0;
    // x = 1 on an effect-chain pass whose near-plane marker (t37, the KH_FX_UNIT section)
    // is this cycle's (KH_NEARZ_MARK), else 0.
    float4 shadowMeta2;   // y = object view-distance cut; z = 1 for a
                          // depth-Off overlay (KH_VOL_WITNESS), else 0; w = the first
                          // vertex of the run VSUserSo is drawing (KH_USER_VS's pass),
                          // or on an effect-chain pass the KH_VM_SEE arm (1 = the depth
                          // behind the view model's translucent planes, t34; 2 = also
                          // their transmittance, t41); 0 everywhere else.
    // Engine-axes rotation rows (row-vector): world = center + local.x*R0 + local.y*R1 + local.z*R2.
    float4 objRot0;
    float4 objRot1;
    float4 objRot2;   // objRot0.w = 1 marks a filled matrix; 0 (the zeroed default) reads as identity.
    // x = 1: normal-blend translucent mesh with the scene capture bound (the flush's fill; the injection writes 0) -
    // the packing composites in Reinhard space against t3 and writes opaque; or a lighten / darken mesh with its own
    // capture bound (KH_BLEND_MINMAX) - the exact lerp against t3. w = the LOD crossfade dither. y unread. z =
    // KH_CUTOUT_A2C: 1 keeps a cutout's hard clip on a multisampled target (the view-model slice), else 0
    // (KhCutoutCover).
    float4 blendCtl;
    // x = this draw's material table index (base + submesh slot; non-instanced VS), y = the submesh slot (the
    // instanced VS adds it to the instance's own base), z = 1 when x is uniform for every instance, w = per-draw
    // alpha-mode override (>= 0 replaces the table's mode; 3 = the opaque part of a blend split). On every untextured
    // fill all four are zero, and read only as arms: y (the scene chain's fog scatter / sun flare) or z (the UI
    // lane's coverage probe) on an effect pass that drew its side value first (KH_FX_SIDE, effect.hlsl); x (the
    // frame's) or w (1 the SSGI grid's, 2 dynamicLightFog's) on a scene-chain pass a linear-depth copy serves
    // (KH_FX_LINZ, effect.hlsl).
    float4 matCtl;
    float4 fuseMeta;
    float4 fuseStage[12];   // [4s] = effect, blend, age (KH_FUSE_CLOCK), 0; then p0, p1, colour. C++ fuse_stage.
    float4 fxParams2;   // Effect parameters [8..11] (C++ twin fx2).
    // x = mode (0 off, 1 camera-relative world, 2 view space, 3 absolute pool),
    // y = point count, z = spot count, w = global distance scale (cb10[2].x).
    float4 dlCtl;
    // xyz = global diffuse multiplier (cb10[3]); w = the global intensity (C++ g_dl_intensity_bits: no command sets it,
    // so 1), applied to the whole DynLights sum.
    float4 dlGlobal;
    // View matrix columns (world->view rotation) for mode 2, captured with the light list.
    float4 dlView[3];
    // KH_DL_RING: x = where this draw's light records start in khDlRecs (t40),
    // as uint bits - read it through KhDlRec, never directly. C++ twin dl_first.
    float4 dlFirst;
    // KH_USER_LANES: x = the session clock (s), y = this object's creation on
    // it (s). Read through the KhUser* accessors below. C++ twin user_obj.
    float4 khUserObj;
    // KH_FX_TEX: an effect's own textures - 1 + each one's layer in its page ([0].xyzw = user0 .. user3, [1].xy =
    // user4 / user5; 0 = absent or still loading). Zero on every draw but a user effect's and (KH_FX_TEX_ALL) a
    // builtin effect pass's with textures. Read through KhUserTex* (the KH_FX_UNIT section). C++ twin fx_tex. [1].z =
    // KH_WSEE's arm (1, the only value an effect pass takes; the KH_FX_UNIT section's KH_WSEE note): 0 on every draw
    // but an armed scene-chain effect pass's - its side draws (fog scatter's per-pixel fog, the sun flare's
    // visibility, the SSGI and dynamicLightFog grids) and the run's agreement mask (PSWseeAgree) carry the pass's
    // lanes, so they read it too. C++ twin fx_tex[1][2]. [1].w = KH_WSEE's bound views where [1].z arms (read as
    // (uint)(w + 0.5): the bits are KhWseeAvail's table), 0 on every other draw. C++ twin fx_tex[1][3]. KH_WSEE on a
    // sorted mesh draw (no effect): [1].z = the sorted tail's mode (1 = the occlusion test against the layer record,
    // the plain blend; 2 = that and the layer-aware composite; 3 = the composite on a scene target that is not a
    // float format, its inputs held to [0, 1] - KhWseeMeshMode, C++ kh_wsee_sort_mode), 0 on every other mesh draw;
    // [1].w bit 3 = the B2 moment (khMsB2 bound in modes 2 and 3), bit 5 = khMsD3 bound (every mode), bits 6 / 7
    // KH_PSB, bits 8 / 9 KH_WSEE_NOPRE (khMsS2 bound in every mode); 0 elsewhere.
    float4 khFxTexLay[2];
    // KH_FX_FOGREC: the scene's KH fog passes as a user effect sees them - up to two records [startDist, endDist,
    // skyAmount, opacity] (opacity 0 = none): the two the builtin fog scatter takes (the fullscreen distance-fog
    // passes, neither localized nor banded nor additive, multiply or screen, the two most opaque). Filled for a user
    // effect in the scene chain and (KH_DLF_FOGREC) for dynamicLightFog's pass and its side draws (effect.hlsl
    // KhDlfPassMedium; its fxParams2.xy say whether each record's pass draws before it); zero on every other draw (an
    // effect mesh, the UI lane and a picture-in-picture - whose passes are all localized - see the engine's fog
    // alone, as the builtin's do). Read through KhFxFogAt / KhFxFogPasses (the KH_FX_UNIT section). C++ twin fx_fog.
    // KH_WSEE on a sorted mesh draw ([1].z of khFxTexLay above 0): [0] = the main depth's decode (the engine's m22,
    // m32, then the world's viewport MinDepth, MaxDepth; KhWseeMeshMeters), zero elsewhere.
    float4 khFxFog[2];
};

 cbuffer CBFrame : register(b1)
{
    row_major float4x4 viewProj;   // Rebased on the two mesh passes (see centerRel); absolute elsewhere.
    row_major float4x4 invViewProj;   // Clip -> world (row-vector convention). Camera-relative
                                      // when fxCam.w = 1 (KhWorldPosFenced and KhFsFog add
                                      // fxCam.xyz back; KhWorldPosRelFenced's readers take
                                      // camera-relative points instead); absolute when fxCam.w = 0.
    float4 lighting1;   // xyz = unit vector toward the sun/moon (engine axes), w = lighting valid flag.
    // rgb = the engine's sun colour in HDR scene units ((1,1,1) before the first publish), w = shadow-map strength.
    float4 lighting2;
    float4 shadowMeta;   // x = cascade count, y = depth compare sign, z = bias, w = atlas size
                         // (px).
    float4 shadowTiles[8];   // Per-cascade atlas rect in UV: x0, y0, x1, y1.
    float4 shadowMats[24];   // Per-cascade world->atlasUV+depth 4x3: 3 rows of [a, b, c, t].
    // View-paired bands: the engine's decoded receiver contract. bandView rows are the view matrix columns
    // (world->view, row-vector convention): viewPos.k = dot(float4(wpos,1), bandView[slot*3+k]). bandMat rows are
    // PSC_ShadowmapMatrix rows: uvz.k = dot(float4(vp,1), row).
    float4 bandMat[24];
    float4 bandView[24];
    float4 bandBorder[8];   // x = near, y = far, z = fade, w = 0 invalid / 1+texIndex.
    // The mask cast's lanes (PSMaskCast, static.hlsl): our shadow painted into the engine's screen-space shadow mask
    // from the engine view the fire reconstructs the scene through. castView[0].xyz = that view's row 3 (translation)
    // and castMat[0..2].xyz its inverse rotation laid out for KhCastWorld's row dots - the cofactor inverse frozen
    // with the view, or the view's own rows 0..2 when none was (the same for an orthonormal view); castView[1] =
    // (fov.x, fov.y, maskW, maskH); castView[2] = the frozen sun (xyz) and strength (w). castMat[0..2].w =
    // KH_CAST_OCC's occupancy grid: cells per metre (> 0 arms it, with localityMeta.w) and the grid's low x and z
    // corner. The DLS world pass (PSDlsWorld) reads castMat.xyz and castView[0..1] the same way, and castView[2].xyz
    // as the camera its reconstruction came from.
    float4 castMat[3];
    float4 castView[3];
    // x = view-paired band table valid; w = the unit-stencil arm (C++ twin
    // mask_meta[3], kh_svs_unit_on). y/z unwritten.
    float4 maskMeta;
    float4 fogParams;   // x = fogValue, y = fogDecay, z = fogBase, w = enable.
    float4 fogColor;   // rgb = fog color, w = camera altitude ASL.
    row_major float4x4 sunVP;   // World -> private sun-depth clip (row-vector).
    float4 sunMeta;   // x = valid, y = map size (px), z = compare bias, w = strength.
    // x = pair count, y = t2 list armed, z = sun map older than 0.5 s (the filtered-compare gate), w = KH_OCC_N, the
    // cast-occupancy grid's edge in texels (0, unwritten, leaves the grid unarmed).
    float4 localityMeta;
    float4 locality[32];   // [2i] = center.xyz (engine), [2i+1] = half extents.xyz.
    // rgb = engine ambient colour (HDR scene units) from the located lighting
    // block, or (1,1,1) before the first lock; w = engine-mode flag.
    float4 lightAmb;
    // The engine's own fog terms from the located block: x = density scale (lane 41), y = fog end distance (lane 48),
    // z = inverse ramp range (lane 49), w = terms valid.
    float4 fogEngine;
    // fogSky: the fog colour's view-elevation gains (x nadir, y horizon, z zenith), w = armed - filled (1, 1, 1, 1)
    // once the sky CB matched (in the fill's fog block), so the target is fogSkyCol.rgb. fogSkyCol.rgb: the sky
    // CB's row 1. KH_FOG_SKY_DOC.
    float4 fogSky;
    float4 fogSkyCol;
    float4 hazePars;   // Engine distance haze, the sky CB's row 14.
    // x = origin world X, y = origin world Z (SQF y), z = node spacing (m: the terrain grid's own where the
    // framework's terrain matrix is the source, a whole multiple of it past KH_THM_MAX_SIDE nodes a side), w =
    // enabled and texture valid (0 off) as 1 + the split code: the engine's triangle split of each cell class,
    // one base-3 digit per class (KhThmHeight, KH_THM_EXACT).
    float4 thmParams;
    // x = width, y = height (nodes: the texture's texels), z = the terrain band (m, KH_THM_BIAS_M: the discard's
    // clearance and PSMaskCast's snap), w = the view distance (m, KH_THM_MIN_DIST_M) from which the clearance march
    // runs. Written with thmParams by kh_fill_occ; both stay zero with no heightfield.
    float4 thmMeta;
    // zw = the volume copy's own dims (not fxMeta.zw). xy = the copy's depth
    // encode pair (m22, m32), KH_VOL_WITNESS (zero = no witness).
    float4 stenVol;
    // x = transport arm; z = KhVsCore vertex path selector (3 = the seam prepass); y/w = the copy's viewport depth
    // range (min, max), KH_VOL_WITNESS (armed while w > y).
    float4 stenVol2;
    row_major float4x4 sunVP2;   // World -> hero sun-depth clip.
    float4 sunMeta2;   // x = valid, y = size, z = bias, w = half-diag.
    row_major float4x4 sunVP3;   // World -> mid-band sun-depth clip (t26).
    float4 sunMeta3;   // x = valid, y = size, z = bias, w = half-diag.
    row_major float4x4 sunVP4;   // World -> outer-band sun-depth clip (t27).
    float4 sunMeta4;   // x = valid, y = size, z = bias, w = half-diag.
    // mirMeta: x = mirror mask valid (1; 2 = the mirror answers the whole surface, the view-model slice's fill), yz =
    // mask dims, w = the sun shadow range (m; C++ twin mir_meta[3], clamp(g_sun_range, 8, 1000)) that KhSunRangeFade
    // fades every sun shadow out over - 0 = no fade, not a free lane. sunOrigin: the anchor every sunVP* is relative
    // to (subtract from wpos before transforming; zero = world-absolute); w = the far tier's prefilter arm.
    float4 mirMeta;
    float4 sunOrigin;
    // x = engine below-layer extinction, y = armed; z = the cast fire's
    // frozen-depth ulp, relative (KH_CAST_SNAP_FACING, PSMaskCast only).
    float4 fogBelow;
    // Below the fog layer the engine converges on a different colour: fogUw.rgb = cb0[7], the sky fog colour, shaped
    // by the cb0[17] elevation gradient in fogUwGrad.xyz.
    float4 fogUw;
    float4 fogUwGrad;
    row_major float4x4 snapVp;
    float4 snapMeta;
    float4 snapCam;
    // xyz = the per-tier prefilter arms (hero / mid / outer); the far tier's arm is sunOrigin.w. w unread.
    float4 sunPf;
    // World -> far-band sun-depth clip (t32) + x = valid, y = size, z = bias, w = half-diag.
    row_major float4x4 sunVP5;
    float4 sunMeta5;
    // The world cast's own compare bias per tier (x/y/z/w = hero / mid / outer / far, normalized by that tier's own
    // d2v), separate from sunMeta*.z, which the self kernel also reads and which may not move. Zero = fall back to
    // that tier's sunMeta*.z, which every mesh fill site leaves.
    float4 sunCastBias;
    // x = the union's cast bias (normalized by its own D); 0 = fall back to sunMeta.z.
    float4 sunCastBias2;
    // C++ twins dls_meta / dls_ctl / dls_face_slice / dls_spot_vp.
    float4 dlsMeta[8];     // xyz = light world position, w = far plane (0 = no map).
    float4 dlsCtl[8];      // x = spot flag, y = bias const (m), z = spot slice, w = bias slope.
    // slot * 6 + face. x = slice (-1 = none), y = normal-offset arm (replicated per light).
    float4 dlsFaceSlice[48];
    row_major float4x4 dlsSpotVP[8];
    // The dynamic-light shadows obey the shadow view distance like the sun's. xyz = the pass camera (engine axes), w
    // = clamp(shadowVisibility, 8, 1000); 0 = no fade. Written by kh_dls_fill_cb.
    float4 dlsRange;
    // khPass.xyz = the pass's rebase camera (engine axes), w = 1 arms the bucket vertex path's rebase (twin of
    // centerRel.w). khPassObj.x = the engine object view distance (the cut for bucket instances); yzw free. Zero
    // wherever no bucket draws.
    float4 khPass;
    float4 khPassObj;
    // xyz = the camera invViewProj is relative to, w = 1 arms it (KhWorldPosFenced
    // and KhFsFog add xyz after the reconstruction; KhWorldPosRelFenced's readers
    // take camera-relative points instead); w = 0 = invViewProj is absolute.
    float4 fxCam;
    // KH_USER_LANES: the pass camera - xyz position (engine axes, absolute), w = 1 when filled; the rows are its
    // right, up and forward axes. Read through the KhUser* accessors below. C++ twins user_cam / user_cam_rot.
    float4 khUserCam;
    float4 khUserCamRot[3];
    // KH_VOL_ZOOM (C++ twin sten_vol3): this pass's raster -> the volume copy's, per axis copy = raster * xy + zw
    // (the copy's seam frame drew under its own projection - a zoom between the two frames moves every surface on
    // screen). x = 0 = identity.
    float4 stenVol3;
    // KH_VOL_FOOT (C++ twin sten_vol4): x = 1 when khVolFoot (t33) holds the
    // footprint mask of the copy's seam frame; yzw unused.
    float4 stenVol4;
    // KH_PCSS_CELL (C++ twin sun_lat): each camera tier's texel lattice offset as uint bits (asuint) - hero xy, mid
    // zw; outer xy, far zw. A texel index of that tier plus it is the world-fixed lattice KhPcssRot hashes.
    float4 sunLat0;
    float4 sunLat1;
    // KH_CB_DERIVED (C++ twins sun_col / cast_cam): values every pixel used to rebuild from the matrices above,
    // formed once per upload by kh_upload_frame_cb from the very matrices this upload carries, so they cannot
    // drift from them. sunCol: each sun map's texel and depth scales - the lengths of its matrix's first
    // (x: the texel direction) and third (z: the depth direction) columns, xyz rows only: [0].xy the union
    // (sunVP), [0].zw the hero (sunVP2), [1].xy the mid (sunVP3), [1].zw the outer (sunVP4), [2].xy the far
    // (sunVP5); [2].zw zero. castCam.xyz: the frozen mask-cast camera, castMat's rows dotted with -castView[0]
    // (read by PSMaskCast only; castMat / castView mean other things on other passes, where it is unread).
    float4 sunCol[3];
    float4 castCam;
};
 
cbuffer CBEngView : register(b2)
{
    float4 engBlk[15];
};

cbuffer CBEngView1 : register(b3)
{
    float4 engBlk1[15];
};

cbuffer CBEngView2 : register(b4)
{
    float4 engBlk2[15];
};

// The object record buffer (C++ twin KhObjRec, 7 float4), one per live-scene slot, read by every bucket vertex shader
// through the lane's slot (VSInst.islot). Engine axes. size.w = the object's creation on the session clock
// (KH_USER_LANES), pos.w = 1 when receiveShadow is off (KH_SHADOW_SWITCH) or every shadow is (KH_SHADOW_OFF), rot0.w
// = 1 (filled), rot1.w = lit ambient fraction, rot2.w = lit diffuse fraction; col carries no lifetime envelope (the
// lane's alpha does). res.xyz = KH_POS_RES, the centre's part finer than pos's float: every relative centre a record
// forms (against khPass, against sunOrigin) adds it; res.w = KH_SKIN_POOL, a skeletal binding's vertex base in its
// skin pool chunk (an exact integer; 0 for every other object), read only by the Sk twins through KhSkPull.
struct KhObjRec { float4 pos; float4 size; float4 rot0; float4 rot1; float4 rot2; float4 col; float4 res; };
StructuredBuffer<KhObjRec> khObjs : register(t39);

// KH_DL_RING: the draw's dynamic-light records, 6 float4 per light (the engine's cb11 record: [0] position, [1] spot
// axis + cone threshold, [2] diffuse + cone scale, [3] ambient + cone exponent, [4] offset + (a0,a1,a2), [5] fade
// start + inverse width + OUR shadow slot + 1), points first then spots, dlCtl.y + dlCtl.z of them - no count cap.
// Every read is gated on those counts (dlCtl.x >= 0.5 and the index below them): past them the ring holds another
// draw's records. Bound at PS t40 by the C++ append (kh_dlr_append), inside StateBackup's saved range.
StructuredBuffer<float4> khDlRecs : register(t40);
float4 KhDlRec(int khdr_i)
{
    return khDlRecs[asuint(dlFirst.x) + (uint)khdr_i];
}

// The per-object lanes a bucket varies per instance and the CB carries per draw. Filled by the vertex shader into two
// flat interpolants (VSOut.iobj0/1) and loaded by every mesh pixel shader at entry (KhObjLoad); the lighting / far
// contract / dither reads use these, never lighting0.zw, shadowMeta2.y or blendCtl.w directly. The passes with no
// VSOut read the CB itself: PSDlsMask, the one reader of blendCtl.w, and the DLS world pass, whose KhDlsWorldFactor
// reads lighting0.z / .w (kh_dls_world_pass fills both with 1).
static float khObjAmb = 0.0f;      // lighting0.z twin: the scale of the engine's ambient (lightAmb) on this mesh.
static float khObjDif = 0.0f;      // lighting0.w twin: the scale of the engine's sun (lighting2), the n.L term.
static float khObjCut = 0.0f;      // shadowMeta2.y twin: object view-distance cut (m, 0 = off).
static float khObjDither = 0.0f;   // blendCtl.w twin: the LOD crossfade dither for this draw.
// KH_SHADOW_SWITCH: lighting0.y / KhObjRec pos.w twin - 1 = receiveShadow off, or every shadow off (KH_SHADOW_OFF:
// the video options' shadow view distance at 5 m or less): every received shadow term reads 1 (the mesh shaders' sun
// block, DynLights, KhDynLightsPBR). 0 where nothing loaded it.
static float khObjNoRecv = 0.0f;
// KH_USER_LANES: khUserObj.y twin - this object's (or bucket instance's) creation on the session clock. The sentinel
// means 'not loaded': KhUserTime then reads the CB, which is this object's on every per-object draw.
static float khObjBirth = -1.0e30f;
void KhObjLoad(float4 khol_a, float4 khol_b)
{
    khObjAmb = khol_a.x;
    khObjDif = khol_a.y;
    khObjBirth = khol_a.z;
    khObjCut = khol_a.w;
    khObjDither = khol_b.y;
    khObjNoRecv = khol_b.x;   // KH_SHADOW_SWITCH.
}
// The LOD crossfade's per-pixel cut, one body for every pass that draws a fading level (the two colour twins, and the
// dlsw mask's PSDlsMask / PSDlsMaskA - KH_DLSW_MASK_DITHER): +v = the finer level keeps the pixels below v, -v = the
// coarser keeps the rest. Complementary at every pixel, whichever pass asks.
void KhLodDitherCut(float2 khld_px, float khld_v)
{
    if (khld_v == 0.0f) return;
    float khld_h = frac(52.9829189f * frac(dot(khld_px, float2(0.06711056f, 0.00583715f))));
    if (khld_v > 0.0f) { if (khld_h >= khld_v) discard; }
    else if (khld_h < -khld_v) discard;
}
// The vertex side: the CB lanes (per-object draws)...
void KhObjLanesCb(out float4 khoc_a, out float4 khoc_b)
{
    khoc_a = float4(lighting0.z, lighting0.w, khUserObj.y, shadowMeta2.y);   // z = creation (KH_USER_LANES).
    khoc_b = float4(lighting0.y, blendCtl.w, 0.0f, 0.0f);   // x = KH_SHADOW_SWITCH; z / w unused.
}
// ...or the record + lane (bucket draws); the cut is the pass's object view
// distance. khor_slot (the instance's scene slot) is unread; the parameter stays
// so the two bucket vertex shaders keep one call shape.
void KhObjLanesRec(KhObjRec khor_r, float khor_dither, uint khor_slot, out float4 khor_a, out float4 khor_b)
{
    khor_a = float4(khor_r.rot1.w, khor_r.rot2.w, khor_r.size.w, khPassObj.x);   // z = creation (KH_USER_LANES).
    khor_b = float4(khor_r.pos.w, khor_dither, 0.0f, 0.0f);   // x = KH_SHADOW_SWITCH; z / w unused.
}

// ===========================================================================
// USER SHADER CONTRACT (KH_USER_LANES). What a user .hlsl may rely on: everything a custom shader can plug into is in
// THIS file, as a declaration or in the text below; nothing of the contract lives anywhere else. Where the file goes:
// Documents\Arma 3\kh_framework\rendering, else any loaded mod's 'rendering' folder (searched in that order; the
// script names the file relative to it). Save it as UTF-8 or ANSI: UTF-16 text (Notepad's "Unicode") or a NUL byte is
// refused with a report (KH_USER_UTF16). The first use compiles it in the background; until it lands a material draws
// flat white (the white placeholder) and an effect pass is skipped. A compile error is reported with fxc's own
// message - its line numbers your file's own (KH_USER_LINE) - once per distinct message (KH_USER_ERR_ONCE): each
// compiled variant (below) remembers its own failure, and variants failing with the same message report it once. A
// failed variant stays failed - not compiled again - until the mission restarts or the device resets
// (KH_USER_FAIL_STAYS): fix the file, then restart the mission. Each compiled variant reads the file at its own first
// use, and every one reads it again after a device reset (alt-tab in exclusive fullscreen, a video-settings change;
// whether the file defines a vertex stage is read once per mission): an edit made mid-mission reaches some variants
// and not others - restart the mission to pick it up everywhere. The compile runs on worker threads, and a material's
// is not quick (KH_USER_END_NOWAIT): its unit is this file, yours and the whole builtin mesh shader (static.hlsl, or
// composite + composite2), so each variant costs about what that shader does - a five-line KhUserShade measured 49 s
// for one variant under fxc (the builtin twins: a minute or two). An effect's unit (this file and yours alone) and a
// vertex stage's take about a second. Variants asked for in the same few frames compile side by side, up to a core
// each; one asked for while a batch is in flight waits for that whole batch, then compiles. Neither a device reset
// nor a mission end waits for a user compile: it runs on, detached, and until it ends every mesh of ours draws the
// white placeholder and no post-processing runs - the next mission's first frames included. A finished unit is cached
// on disk (Documents\Arma 3\kh_framework\cache), keyed by the unit's whole text, its defines and the compile flags:
// an unchanged file compiles once per machine, each edit once more. To spare players the wait, ship the .khsc files
// in your mod's 'cache' folder (read after Documents, never written): run the release until the placeholder gives
// way, drawing every .hlsl material and effect it ships in each context it draws in (a user effect with FSAA both off
// and on: it compiles per depth sample count, MSAA_DEPTH), then copy the .khsc files from
// Documents\Arma 3\kh_framework\cache (best emptied first). Every variable at file scope must be
// static (static const for a constant): a plain global (float k = 0.5;) is a shader constant - its initializer is
// ignored and it reads whatever constant buffer its register holds, one of ours or the engine's. Declare no cbuffer:
// nothing fills one. Bound every loop by a literal, or clamp a count taken from a parameter: a draw that runs for
// about two seconds of GPU time makes Windows reset the driver, which the game does not survive.
//
// ---- MATERIAL shaders (a material whose shader is a .hlsl path) ----------
// Placed after this file and BEFORE the builtin pixel shaders, and compiled three times - the flush's twin
// (static.hlsl) and the injection's guard and arbitration twins (composite) - with KH_TEXTURED = KH_USER_MAT =
// KH_RECEIVE_TEX = 1. It may call anything in this file outside the KH_FX_UNIT section (an effect's: a material
// compile does not define it); it cannot call into static.hlsl / composite2.hlsl (they come after it). It defines
//     float3 KhUserShade(KhMatSurf s, float3 wpos, float3 n, float smf)
// (the older five-parameter form with an `out float3 ambient` does not compile - fxc's message is reported once and
// the submesh draws the white placeholder until the file is updated and the mission restarted). While the compile is
// pending, or if it fails (reported once), the submesh draws flat unlit white instead (the white placeholder, with
// the default material); the builtin PBR shades it only if the placeholder itself was not built.
//   s     the material sampled at the pixel's UV - after the material's texture animation (KhMatUv), KhUserUv() - by
//         KhSampleMatG; the object colour already tints s.albedo. s.alpha is informational: coverage is decided
//         around the call (below).
//   wpos  the world position (engine axes: x east, y up, z north; absolute m - a float of world size: steps of 2 - 4
//         mm 16 - 40 km from the origin, which a pattern keyed on it shows).
//   n     the shading normal the builtin uses: unit, normal map applied, and
//         reversed on the back face of a two-sided mesh.
//   smf   the sun shadow factor, 0 dark - 1 lit: the received world shadows and the private self-shadow, already
//         min-combined - multiply the sun term by it once and do not stack another sun shadow on it. With
//         receiveShadow off, or every mesh's shadows off (a shadow view distance of 5 m or less in the video
//         options), it is 1 wherever the face turns to the sun.
//   RETURN the lit colour in the engine's linear HDR scene units (the units of lighting2 and lightAmb), before fog:
//         finite and not negative - it is held to [0, 1e30], a NaN reads 0 and an inf 1e30 (KH_USER_FINITE); the 16F
//         scene stores at most its largest finite value, 65504 (the store saturates). Around the call the builtin
//         applies fog, the coverage (the material's cutout / blend alpha times the object colour's alpha) and the
//         object's blend mode. Coverage is that alpha's: a discard or clip in KhUserShade removes the pixel from this
//         colour draw alone - the shadows the mesh casts, its cast onto the world, an inFront mesh's depth prepass
//         and its stencil footprint (KH_SEAM_USER_CLIP: the surface the engine's shadow volumes are counted against)
//         keep the whole surface. A cutout material's edge is decided before the call: on a multisampled scene it
//         takes alpha to coverage (KH_CUTOUT_A2C, KhCutoutCover), so an edge pixel may reach only some of its
//         samples; the shading runs once per pixel. A blend material's translucent part (KhUserMatAlphaMode() 2) in a
//         sorted tail - a cycle whose see-through record landed on a multisampled scene (not PIP) - is drawn after
//         the game's blended layers with no hardware depth test: around the call the builtin tests the pixel per
//         sample against the see-through record (KH_WSEE, KhWseeMeshCut - discarded where every sample lies behind an
//         opaque, the kept samples its coverage) and composites the colour under the layers in front of it
//         (KhWseeMeshOut; on a non-float scene target, mode 3, colour and alpha held to [0, 1] first); on any other
//         cycle it draws depth-tested by the hardware. Nothing in KhUserShade changes this: return the straight lit
//         colour as on any draw.
// The builtin's own recipe is the reference to copy or wrap: KhApplyPBR(s, wpos, n, smf) is what KhUserShade
// replaces, and ApplyLighting (untextured) documents the combine. Rules its pieces follow:
//   - khObjAmb / khObjDif are this object's lit [ambient, diffuse] (loaded at the PS entry, per instance on a bucket
//     draw): scales, 0 or more, of the engine's ambient (lightAmb) and of its sun's direct n.L term (lighting2)
//     (KH_LIT_SCALES). Scale the ambient and the direct sun by them (the builtin's overcast lobe, sky light, takes
//     khObjAmb); the dynamic lights take neither. Never read lighting0.zw directly.
//   - DynLights(wpos, n) / KhDynLightsPBR(..., occ) carry every dynamic light that reaches the mesh, with their own
//     shadows (none with receiveShadow off or every mesh's shadows off); add them as they are. The n passed drives
//     their N.L; their shadow lookup takes the geometric normal (KH_DLS_GEOM_N). DynLights, KhDynLightsPBR and
//     KhApplyPBR take a screen gradient (fwidth(wpos), KH_USER_LIGHT_GRAD): call them in uniform flow, not inside a
//     loop with a per-pixel break.
//   - Ambient occlusion is NOT a shader term: KH_SSAO multiplies the whole colour of a depth-writing solid's pixels
//     after the draw, at the sample. s.occ is the material's occlusion map, for the ambient only (as KhPbrAmbient
//     applies it, and KhDynLightsPBR's occ argument to each lamp's ambient - KhApplyPBR passes s.occ).
//   - The ambient (KhPbrAmbient, and each lamp's ambient in KhDynLightsPBR) is the split sum's: its env-BRDF, F82 and
//     dome tables (KH_ENV_LUT / KH_ENV_F82 / KH_DOME_LUT: 544, 512 and 768 of the shader's 4096
//     immediate-constant-buffer float4s) take at most 1824, those a shader's calls reach (KH_ICB_DOC): KhApplyPBR all
//     three; KhDynLightsPBR alone no dome, 1056 or less. For an ambient of your own (another sky, a dome from a user
//     map): KhPbrAmbSplit(N.V, rough, F0) is its specular response E to a unit radiance (the builtin's diffuse keeps
//     (1 - metal)(1 - E)), KhEnvBrdfLut(N.V, rough) the (A, B) behind it, KhEnvBrdfC(N.V, rough) the arma model's F82
//     term (KH_ENV_F82) and KhDomeSkyShare(R.y, rough) the sky's share of the dome along R - pure functions;
//     KhPbrAmbSplit shades the arma model (s.model = 1, the conductor Fresnel of fresnelNK) only after a KhApplyPBR
//     call in the same pixel set it, Schlick's form before.
//   - Emissive is not added for you: KhApplyPBR adds s.emissive last.
//   - Highlights (KH_GGX_EXACT, KH_SMITH_HC): KhGGXSpec is the exact GGX single-scatter lobe at every roughness (its
//     D and the height-correlated Smith G2), held to 0.045 at the low end. KhApplyPBR and KhDynLightsPBR multiply
//     every lobe by the multiple-scattering compensation Fms (KH_MS_DIRECT; KhPbrAmbSplitF(N.V, rough, F0, fms)
//     returns it beside E): a lobe of your own takes it the same way. A glossy surface takes a small, bright sun or
//     lamp highlight - a white metal's at 0.045 peaks at ~2e4 (sun) x the light's colour head-on, ~1.3e5 at N.V 0.15
//     and more toward grazing (~2e6 at N.V 0.01), as spec x Fms x pi x N.L before khObjDif. KhApplyPBR widens the
//     sun's lobe by the sun's disc (KH_SUN_DISC) and KhDynLightsPBR a lamp's by a 0.05 m emitter at its distance
//     (KH_DL_RADIUS: a lamp 10 m off widens as the sun, 1 m off ~1 % of a point's peak, 100 m off 0.85 of it). Every
//     route's colour then loses only NaN, inf and negatives (KH_PBR_FINITE, [0, 1e30]; RETURN above); the 16F scene
//     stores at most 65504.
//   - Specular anti-aliasing (KH_SPEC_AA): s arrives with rough and gloss widened (s.aaA2, below) by the normal map's
//     filtering and the screen-space variance of n, so a lobe that narrow does not sparkle where the normal turns
//     faster than the pixels sample it. Shading with a normal of your own m (a second normal map, parallax), add its
//     variance in uniform flow: s = KhSpecAA(s, m) (= KhMatWiden(s, KhSpecAAKernel(m))) - on top of n's, which errs
//     toward the wider. A roughness or gloss you set yourself takes none: KhRoughWiden(rough, s.aaA2) is the
//     builtin's widening of it (gloss: 1 - KhRoughWiden(1 - gloss, s.aaA2)). The map's own value, un-widened, is
//     (s.rough^4 - s.aaA2)^(1/4) while s.rough is below 1 (the widening is held to 1; no helper inverts it).
// The material's inputs (KH_USER_SLOTS) - EVERY map slot, routing input and param there is belongs to a .hlsl
// material. The script side (updateRender3D "material"), one entry per submesh selector:
//     [selector, "my.hlsl", [[path, slot, routing?], ...], [[key, value], ...]]
//   selector  a submesh name, its index, or -1 / "*" for every submesh.
//   slot      where the map goes - KhMatFetch's slot number, its colour space:
//     "diffuse" | "albedo"   0  sRGB
//     "normal"               1  linear
//     "orm"  or  "as"        2  linear   ONE slot under two names: give one.
//     "emissive"             3  sRGB
//     "specular" or "smdi"   4  sRGB as "specular", linear as "smdi" (its
//                               channels are data). ONE slot: give one.
//     "speccolor"            5  sRGB
//     "user0" .. "user5"        yours alone (KH_USER_TEX below); their third
//                               element is "srgb" | "linear", not a routing.
//   routing   [[input, channel], ...] - which channel ("r" "g" "b" "a") of THIS map feeds a surface input:
//             "occlusion", "roughness", "metallic" (or "metalness"), "alpha", "gloss", "specular". Unrouted, an input
//             takes its map's own convention:
//               occlusion  orm.r - as.g when the slot was named "as"
//               roughness  orm.g - none under "as" (the roughness param)
//               metallic   orm.b - none under "as" (the metalness param)
//               alpha      diffuse.a
//               gloss      specular.a - smdi.b when named "smdi"
//               specular   none (1) - smdi.g when named "smdi"
//             A route reads its map as the slot is decoded: the r / g / b of an sRGB slot (diffuse, emissive,
//             specular, speccolor) arrive decoded from sRGB, so route data from orm / as, smdi, normal or an alpha
//             (never decoded).
//   params    "basecolor" [r,g,b], "roughness", "metalness" (or "metallic"), "glossiness", "specularColor" [r,g,b],
//             "fresnel" [N,K] (or an rvmat's "fresnel(N,K)" string), "emissiveIntensity", "normalStrength", "cutoff",
//             "alphaMode" "opaque" | "cutout" | "blend"; and the VERTEX stage's "vertexBound", "vertexStatic"
//             (below). Defaults: basecolor 1,1,1; roughness 0.8; metalness 0; glossiness 0.2; specularColor 1,1,1;
//             fresnel 1.5,0; the two strengths 1; cutoff 0.5; opaque.
//             Texture animation (KH_UV_ANIM; every shader kind takes them, the builtin "pbr" / "arma" too): "uvScale"
//             (number or [u, v]), "uvOffset" [u, v], "uvScroll" [u, v] per second, "uvRotation" (degrees, about the
//             tile's centre), "uvRotationSpeed" (degrees per second), "uvWarp" [amplitude, frequency, speed],
//             "flipbook" [columns, rows, fps, frames?]. The lookup (KhMatUv):
//               uv' = KhUvAffine(uv, A, c), A = S * R(angle) - c holds the
//                     centre, the offset and the scroll (wrapped to [0, 1));
//               then KhUvWarp(uv', amplitude, frequency, phase) - the ripple
//                     is tied to the surface, the picture scrolls through it;
//               then KhUvFlip(uv', [columns, rows], frame).
//             Every map of the slot moves together (user0 .. user5 included), and so do the coverage verdicts and the
//             shadows: a cutout or blend material's holes and casts follow its picture (its object then redraws the
//             shadows it is in every frame). The clock is the renderer's session clock - real time, running while the
//             game is paused or time-accelerated - so objects sharing a material move in step; the phases are formed
//             in double, once a frame. Normal maps turn and stretch with the picture (KhUvNrm, inside KhSampleMat);
//             the warp does not bend them.
// What they arrive as - the fields of s (KhMatSurf, from KhSampleMat):
//   albedo    diffuse.rgb (1 with no map) x basecolor x the object colour.
//   alpha     the routed "alpha" (1 unrouted).
//   nrmT      the tangent-space normal, xy x normalStrength (0,0,1, no map).
//   occ, rough, metal   the routed inputs; with no map behind a route, 1, the roughness param and the metalness param
//             - rough widened by KH_SPEC_AA (aaA2).
//   emissive  emissive.rgb x emissiveIntensity (0 with no map).
//   specF0    slot 4's rgb as sampled (0 with no map); specOn = 1 when that slot holds a "specular" map - the builtin
//             then shades spec-gloss from specF0 and gloss (roughness 1 - gloss: KH_GLOSS_ROUGH). 0 under "smdi" (its
//             rgb is no F0).
//   gloss     the routed "gloss"; with no map behind the route, slot 4's alpha where that slot holds a "specular" map
//             (SMDI.b under "smdi"), else 0 - take KhUserMatGlossiness() then (the builtin arma model does) - widened
//             by KH_SPEC_AA as 1 - rough is (aaA2).
//   specTint  the routed "specular" (1 unrouted) x speccolor.rgb (1 with no map) x the specularColor param.
//   fresnelNK the fresnel param.
//   model     0. Set it to 1 before KhApplyPBR(s, ...) to shade with the arma model - spec-gloss from gloss, specTint
//             as F0, the conductor Fresnel of fresnelNK - instead of metal-rough. KH_GLOSS_ROUGH: spec-gloss takes
//             roughness = 1 - gloss (floored at 0.045, as every roughness); there is no specularPower.
//   aaA2      KH_SPEC_AA: the GGX alpha^2 (alpha = roughness^2) added to rough and gloss - the normal map's filtering
//             (KhToksvigA2, in KhSampleMatG, png / tga / bmp normal maps: a re-sample carries its own) plus the
//             variance of n over the pixel (KhSpecAAKernel, added by the builtin before KhUserShade). 0 on a flat,
//             unmapped surface up close.
// The raw values behind them: KhUserMatHas(slot) - the map resolved and is loaded - and KhUserMatBaseColor /
// Roughness / Metalness / EmissiveIntensity / NormalStrength / Cutoff / AlphaMode / SpecularColor / Glossiness /
// Fresnel(), each the script's param (or its default). KhUserMatAlphaMode() is 0 opaque, 1 cutout, 2 blend - and 3 on
// a blend material's depth-writing draw (its solid texels - the routed alpha's mip 0 texel 0.9 or more - drawn with
// the opaques at alpha 1; the rest draw in the translucent tail as 2).
// Re-sampling the material (animation): KhUserUv() is the uv the builtin sampled - the pixel's, after the material's
// own texture animation when it has one (KhUserUvRaw() is the mesh's, KhUserUvDx() / KhUserUvDy() the gradients the
// builtin used) - so KhSampleMatG(KhUserUv() + offset, KhUserUvDx(), KhUserUvDy()) re-samples the whole surface
// (anywhere: the gradients are handed in) and KhUserTexGrad(i, KhUserUv(), KhUserUvDx(), KhUserUvDy()) one of its own
// maps. The implicit forms - KhSampleMat(uv) (in uniform flow), KhUserTex(i, uv), KhMatFetch(slot, uv) - take the
// gradients of their uv, which is right for a material without a flipbook; with one, KhUserUv() jumps between cells
// at every whole uv (a face's edge, a tile's) and an implicit read there drops to a coarse mip - a seam: use the
// gradient forms. KhMatFetchTexel(slot, uv) is the unfiltered texel at mip 0 (the slot numbers above). Drive the
// offset from KhUserTime(), or use the KH_UV_ANIM helpers (KhUvAffine / KhUvWarp / KhUvFlip). KhUserUv() lives in the
// animated space: an offset worked out in the mesh's uv (parallax from KhUserTangent / KhUserBitangent, a vertex
// stage's) belongs on KhUserUvRaw() - KhMatUv(KhUserUvRaw() + offset, dx, dy) applies the material's animation to it
// and returns the gradients (in uniform flow). What KhUserShade does moves the COLOUR only: the coverage and the
// shadows follow the builtin's uv - give the material the texture-animation params above to move those too.
// Textures of its own (KH_USER_TEX): the material's texture slots user0 .. user5 (script: [path, "user0", "srgb" |
// "linear"], linear by default) are this shader's alone - no builtin input reads them. Read them with
//     KhUserTex(i, uv)             filtered (matSamp, the material sampler)
//     KhUserTexLod(i, uv, lod)     at a mip level (loops, parallax steps)
//     KhUserTexGrad(i, uv, dx, dy) with explicit derivatives
//     KhUserTexValid(i)            the map resolved and is loaded
//     KhUserTexSize(i)             its size in texels (0 when absent)
// with i a literal 0 - 5 (a non-literal i samples all six and selects). An absent or still-loading map reads zero.
// Declare no texture or sampler of your own: nothing binds a register a user shader declares. One declared without a
// register takes a free one - possibly one of ours the unit does not read (khArbSnap's t2, another route's picture) -
// and reads whatever the route left bound there; fxc reports a clash with one of ours only when both are read.
// The surface frame (KH_USER_FRAME), the builtin's own:
//     KhUserGeomNormal()  the interpolated vertex normal, unit, reversed on the back face of a two-sided mesh (n is
//                         it with the normal map applied)
//     KhUserTangent() / KhUserBitangent()  the orthonormal tangent and
//                         bitangent (zero where the mesh has no usable tangent)
//     KhUserPerturb(t)    normalize(T * t.x + B * t.y + N * t.z) for a tangent-space normal t - with the material's
//                         own normal map (s.nrmT) it reproduces n exactly; N alone where there is no tangent
//     KhUserFrontFace()   false on the back face of a two-sided mesh.
// View direction in tangent space (parallax): with v = KhUserViewDir(wpos), float3(dot(v, T), dot(v, B), dot(v, N)).
//
// ---- The VERTEX stage of a material shader (KH_USER_VS) -------------------
// Optional. A material .hlsl that also defines
//     void KhUserVertex(inout KhUserVtx v)
// deforms the vertices of the submeshes it is assigned to (it still defines KhUserShade; "return KhApplyPBR(s, wpos,
// n, smf);" keeps the builtin look). It is a DEFORM function, not a free vertex shader: it moves the mesh's own
// vertices, in the object's space, and everything downstream - the builtin vertex transform, every colour pass, every
// shadow and depth pass - then draws the deformed mesh as if it had been authored so. (A cloth collider does not see
// it: physics stays on the CPU.) It is evaluated ONCE per frame for all of them (C++ kh_uvs_step: a stream-output
// pass through VSUserSo, static.hlsl), which is what keeps a shadow attached to what casts it.
//   v.pos      the vertex, object space, METRES: x right, y up, z forward, the object's centre at 0 - before the
//              object's rotation and position (world = KhUserVtxWorld(v.pos)). Write it. (It is stored in the
//              object's box, so an axis the script set the object's size to ZERO on cannot be displaced along; an
//              imported flat mesh has 0.1 mm there, which is enough.)
//   v.nrm      the unit normal, object space. Write it if the deformation
//              turns the surface (it need not stay unit length).
//   v.tan      the unit tangent, object space (zero where the mesh has none),
//              and v.tanSign its handedness. Write v.tan with v.nrm.
//   v.uv       the texture coordinate. Writable: it is what the pixel stage receives (KhUserUvRaw; KhUserUv once the
//              material's texture animation, if any, has moved it), the one channel from this stage to that.
//   v.rest     READ: v.pos as authored (or as skinned / simulated this frame -
//              the stage runs after both), whatever you have written to v.pos.
//   v.id       READ: the vertex's index in the mesh's vertex array - stable
//              across frames and LOD levels; hash it for per-vertex variation.
// What it may read: KhUserTime() (the object's age - the animation clock), KhUserSessionTime(), the object lanes
// (centerSize.xyz the centre, sizeAxes.xyz the edge lengths, objRot0..2 / KhObjRows the rotation rows, color), the
// KhUserVtx* helpers below, and the MAIN view's camera (KhUserCamera*, KhUserViewDir) - the one camera there is at
// that point: a PIP view and every shadow map draw the same deformed mesh, so a vertex turned toward the camera faces
// the main view in all of them. NOT there: the material table and every texture - no map is bound to the vertex stage
// and the table is not loaded, so KhUserMat* read zero, and KhUserTex / KhSampleMat / KhMatFetch must not be called
// from it at all (a filtered Sample does not compile in a vertex shader); the lighting and fog lanes (zero;
// lighting1.w = 0 says so); khObjAmb / khObjDif.
// The script's two params, with the material's others:
//   "vertexBound"   metres: the farthest KhUserVertex moves any vertex from v.rest. The deformation happens on the
//              GPU, so this promise is all the culling, the LOD radius, the sun-map fit and the cast have: every
//              bounds test pads the object by it. Too small and the mesh is culled at the screen edge while still in
//              view, or its shadow is clipped; too large costs only a looser fit. It is not enforced: a vertex moved
//              past it is drawn where it lands. Default 0 - right only for a stage that stays inside the box.
//   "vertexStatic"  1: the result depends on nothing that changes by itself (no clock, no camera) - it is then
//              evaluated again only when the object moves, turns, resizes, changes colour or material, and its shadow
//              maps are not redrawn every frame. Default 0. (Unread over a skinned or simulated mesh, which moves.)
// Its envelope: an object with a vertex stage is drawn per object (it leaves the instanced buckets, as a cloth does)
// and owns one vertex buffer of the mesh's size; a vertex shared by two submeshes belongs to the first of them; there
// are no interpolants of your own from this stage to KhUserShade (v.uv is the channel). While the stage compiles the
// mesh draws undeformed; if it fails (reported once) it stays so.
// A material shader (and its vertex stage) reads no scene colour and no depth of its own - the scene's picture and
// depth are bound only where the guards read them - so a refraction or an edge softened against the scene cannot be
// written in one; an effect mesh can (the EFFECT section).
//
// ---- EFFECT shaders (a fullscreen pass or effect mesh given a .hlsl path) --
// The script side: the object's "effect" property is the .hlsl path (where a builtin effect's name or id would go)
// and its "params" property the twelve numbers that arrive as fxParams0..2. It defines
//     float4 PSEffect(VSOut i) : SV_Target
// Compiled as this file plus the user file, with MSAA_DEPTH set to the depth it reads (the scene's; 0 in a
// picture-in-picture, whose depth is a single-sample copy) and KH_FX_UNIT = 1: the KH_FX_UNIT section at the end of
// this file - the builtin effects' own declarations and helpers, the text their shader is built on - is in it. Do not
// declare what it declares, and declare no texture or sampler of your own: the registers are assigned by this file,
// and one you declare may collide with one of ours (fxc reports it only once both are read) or read whatever the
// route left bound there.
// Where it runs: the scene chain, the UI lane, effect meshes and - a localized
// pass, as the builtins' - a picture-in-picture (never the injection).
// Use only the KH_FX_UNIT section's helpers and the lanes named here: the rest of this file (the mesh shading, the
// sun / DLS shadow functions, KhSceneMeters) compiles in an effect, but its resources are not bound for one.
// Bound for it, as for the builtin effects (the section declares each):
//   sceneColor (t0)  the pass's source. SampleScene(px) reads it clamped to the frame (and premultiplied in the UI
//                    spill lane); KhSampleSceneBil(p) at a fractional position p (pixel centres at k + 0.5, as
//                    i.pos.xy), bilinear over four SampleScene taps - the texel itself at a centre - as every builtin
//                    displacement effect (chromatic aberration, distortion, the rain lens, the CRT) reads: a tap at
//                    the nearest whole pixel steps a pixel at a time (KH_FX_BILINEAR).
//   depthTex (t1)    the depth the pass sees. On the UI (centerSize.w above 1) it is at the 3D render's size, which
//                    differs from the frame's at a Sampling other than 100 %: LoadDepthRaw / LoadDepthPS / KhLinZ map
//                    the pixel (KH_UI_DEPTH_SCALE), a direct depthTex.Load does not. LoadDepthPS(px) is the builtins'
//                    read: the raw depth (LoadDepthRaw), with our near-plane marker applied where one of our meshes
//                    drew nearer than the near plane, and the scene's depth where the view model's translucent planes
//                    hid it (KH_VM_SEE, khVmSee below; KH_WSEE's khWs* below leave it the front depth). LinDepth(raw)
//                    = metres along the view axis (1e9 for the far plane and the sky); KhLinZ(px) =
//                    LinDepth(LoadDepthPS(px)) (a builtin pass may read a copy of the frame's; a user pass converts);
//                    KhWorldPosFenced(px, uv, d) = the world position (engine axes, absolute) at the pixel's centre
//                    (uv = i.pos.xy / fxMeta.zw) and d its view depth (LinDepth's), held short of the far plane
//                    (0.999 x KhEncFence()); KhgVpos(float2(px), d, fxMeta.zw, m.x, m.y), m = KhFxProjScale(), = the
//                    view-space position at view depth d.
//   khNzMark (t37)   that marker (armed by shadowMeta2.x in the scene chain only - never an effect mesh, the PIP or
//                    the UI lane; LoadDepthPS reads it).
//   khVmSee (t34) / khVmT (t41)  KH_VM_SEE: the depth behind the first-person view model's translucent planes (a 3D
//                    scope's eyepiece, a sight's glass) and their transmittance. Armed (KhVmSeeArmed(),
//                    shadowMeta2.w) in the scene chain past the view model alone: MSAA, the viewport's MinDepth above
//                    0 (depthParams.z), a cycle whose view model drew a translucent plane and in which a pass reads
//                    the depth - a user effect counts when its file calls LoadDepthPS(...), LoadDepthRaw, KhLinZ,
//                    KhWorldPosFenced, KhWorldPosRelFenced, KhSunFlareVis or a KhVmSee helper (the whole name
//                    followed by a parenthesis, outside a comment: "#define D LoadDepthPS" and D(px) is not a call of
//                    it), or names depthTex, khNzMark, khLinZ, one of these views or an identifier beginning KhWsee or
//                    khWs (the scan is C++ kh_user_src_reads_depth), or is localized or banded (its mask reads the
//                    depth). Then LoadDepthPS returns the scene's depth there; KhVmSeeCovered(px)
//                    says a plane wrote its own over it; KhVmSeeT(px) is their transmittance (1 elsewhere and
//                    unarmed). The builtin fogs, fog scatter, dynamicLightFog and SSGI also fade by it (KhFxFinish);
//                    a user effect that puts a medium or light into the scene does that itself, before KhFxFinish:
//                    outc = KhVmSeeFade(scene, outc, px). The UI lane reads the depth at Present, where LoadDepthPS
//                    returns the planes' near depth, as for the builtins; at 1x (and with MSAA while the late chain
//                    is refused or backed off) the chain runs before the view model draws - no planes in the depth
//                    yet - and a picture-in-picture has no view model.
//   khWsS (t53) / khWsT (t54) / khWsB (t55) / khWsR (t56) / khWsN (t57) /
//   khWsE (t58) / khWsC (t59, MSAA)  KH_WSEE: the world's blended draws (a wire fence, foliage, glass, particles,
//                    smoke) - the raw depth behind them, their transmittance and the late opaque flag, the scene
//                    colour behind them, the chain's input, the nearest raw depth of the ones that write none, the
//                    samples' classes (KH_WSEE_EDGE_COST: a uint word, KhWseeClassWord) and the scene's sample
//                    colours (the KH_FX_UNIT section's KH_WSEE note). khWsC (and KhWseeClassC, which loads it) is
//                    bound only where KhWseeAvail() bit 1 is set - the pass's own draw, not the run's last pass where
//                    the scene cannot take the copy back, nor where the scene's sample view is missing; elsewhere it
//                    loads 0: test the bit, and without it take the pixel's colour (SampleScene) as effect 13 does.
//                    Armed (KhWseeArmed()) in the scene chain alone, in a cycle whose capture landed, for a user
//                    effect that is localized or banded or whose file names KhSunFlareVis or an identifier beginning
//                    KhWsee or khWs - these helpers and views (C++ kh_user_src_uses_wsee: the scan is textual, any
//                    such identifier outside a comment arms it - on a #define line, under #if 0, in a string, a name
//                    of your own with that prefix; over-arming costs the capture, never the picture - and only a name
//                    assembled by ## escapes it. An effect reading the depth otherwise alone is not armed - it would
//                    see the front depth either way, and every armed pass costs the capture). LoadDepthPS stays the
//                    depth buffer's (the layers' that write it). KhWseeCovered(px) says layers cover px; KhWseeT(px)
//                    is their transmittance (1 elsewhere and unarmed - and where the covering layers are fully
//                    transparent there: covered at T = 1 shows only what is behind them); KhWseeFrontRaw(px) /
//                    KhWseeBehindRaw(px) the nearest layer's raw depth and the raw depth behind them (each
//                    LoadDepthPS's where none covers). T = KhWseeAt(px, LoadDepthPS(px), o) gives them from one read:
//                    o = (front raw, behind raw, G), covered exactly where o.y > o.x (o = (r, r, 0) and T = 1 where
//                    not; T = 1 can be covered). G is 0 where the colour behind the layers is the capture's; 1 where
//                    a surface was drawn over the scene after the cycle's first fence-type layer - the depth holds
//                    it, or a layer drawn over it later (the behind raw is then that surface's own depth); 0.5 where
//                    such a surface was drawn before that layer and khWsB2 was taken (KhWseeAvail() bit 3: its colour
//                    is in khWsB2, its depth the behind; 1 without it) - so a test for a late surface is o.z > 0.25,
//                    not o.z > 0.5. A depth-derived quantity Q is then T Q(behind) + (1 - T) Q(front), as the
//                    builtins take it. With MSAA a pixel's samples may differ (a leaf's edge): KhWseeCount(px, r, g)
//                    is 1 (the per-pixel rule above) or, where they disagree, the samples to take (the depth's count,
//                    at most KH_WSEE_NS; g = the pixel's depth gradient, an out), and KhWseeSampleAt(px, r, k, n, g,
//                    o) the rule on sample k (k < n = the count; n = 1 is KhWseeAt) - Q is then the mean over them.
//                    KhWseeMix(px, r, q, z, e) does all of it as the builtins do - where khWsE's class word stands it
//                    takes one pair per class of samples (one T texel, one surface within a few per cent of one
//                    depth), weighed by the class's members (KhWseeCountW): the cheaper approximation of that mean,
//                    not the loop over every sample - (a statement; brace it under an unbraced if / else): on entry q
//                    holds Q at r = LoadDepthPS(px) of that same px, e is an expression of the float z (a view depth,
//                    LinDepth's; z a name unused in the scope - the macro declares it) giving Q there, free of
//                    gradients (Load / SampleLevel: it runs in a loop) - evaluated only at the other depths; px, r
//                    and q plain variables; on exit q is the blend (q as it was unarmed and where nothing covers and
//                    the samples agree; under layers passing 98 % or more with the depth buffer at the behind,
//                    Q(behind) alone - KH_WSEE_FAINT, within 2 % of the blend, e not evaluated). Two such masks
//                    multiplied average each alone, not their product (edges only). Where several layer groups cover
//                    a pixel (the fence-type layers, the particle-type ones drawn before / after the cycle's second
//                    snapshot, each split by its distance from the surface behind) KhWseeGroups(px, z, t) gives them
//                    one by one - their count n, raw depths nearest first and transmittances - to compose front to
//                    back (KhWseeMix does): (1 - t.x) Q(z.x) + t.x (1 - t.y) Q(z.y) + t.x t.y (1 - t.z) Q(z.z) + t.x
//                    t.y t.z Q(behind). n may exceed 3 (up to 7): z / t hold the three nearest, KhWseeGroups7(px, z,
//                    t, z2, t2) all of them (groups 0 - 3 in z / t as float4, 4 - 6 in z2 / t2; KhWseeGroups5 the
//                    five nearest); an absent group sits at the behind with t 1, so the sum above over all seven is
//                    right at any n. The colour behind the layers: KhWseeBehindColor(px, scene, behind, t) (below, in
//                    the section) gives it as the distance fog takes it - never read khWsB / khWsB2 raw: they are the
//                    capture's colours, older than every chain pass before this one, and hold no colour behind a
//                    surface drawn over the scene after the capture, nor the dynamic-light shadows our meshes cast on
//                    the world (applied after). The colours are taken only from the cycle after a pass naming these
//                    identifiers first runs: such a pass added draws its first frame unarmed (every helper here at
//                    its unarmed value). KhFxFinish already blends its localization and band masks so (KhFxLocalMask
//                    / KhFxBandMask give the masks alone); an effect's own use of the depth does it itself. Never
//                    armed on an effect mesh, the UI lane or a picture-in-picture. khWsC, KhWseeClassWord and the
//                    sample-level helpers (KhWseeNs / Grad / Same / Disagree / ClassC) exist only under MSAA_DEPTH 1
//                    - guard a read with #if MSAA_DEPTH (the same file compiles for a picture-in-picture, or with
//                    antialiasing off, at MSAA_DEPTH 0: fxc then reports an undeclared identifier and the pass is
//                    skipped); KhWseeCount / SampleAt / Mix / Groups exist in both (count 1 at MSAA_DEPTH 0). KH_PSB:
//                    the particle layers' record is split over khWsT's B / A and khWsN beside khWsP0 / khWsP1 /
//                    khWsNb / khWsNb2 (t66 - t69, the one pass's, at 1 / 2 the frame: KhWseePsbAt), and a pre late
//                    opaque is derived (KhWseeAvail() bit 8, KH_WSEE_NOPRE) - read the record (T's G included)
//                    through these helpers, not raw.
//   khArbSnap (t2)   an effect mesh's arbitration snapshot (KhFxBegin reads it).
//   khsgTex (t3) / khsgSamp (s2)  the pass's source pre-filtered: the pyramid the builtin glows read, built for a
//                    pass whose file calls KhGlowTap or KhGlowTapA (not thresholded). KhGlowTap(pc, f) reads it over
//                    a footprint of f px at the full-res position pc (centres at + 0.5), KhGlowTapA along an axis;
//                    KhGlowOn() is false while none is armed, and KhGlowSel(direct, pyramid, KhGlowMix(spacing))
//                    blends as the builtins do (direct taps a pixel apart or less, the pyramid from two).
//   khPfSamp (s1)    linear, clamp, every mip - bound for every user effect draw.
//   khUserMap0..5 (t43 - t48) / khFxUserSamp (s0)  the effect's own textures: the script's "textures" (updatePostFX,
//                    or updateRender3D for an effect mesh), user0 .. user5, "srgb" for colour data or "linear" (the
//                    default). Read them as a material shader reads its own: KhUserTex(i, uv) filtered (anisotropic,
//                    wrap), KhUserTexLod(i, uv, lod), KhUserTexGrad(i, uv, dx, dy), KhUserTexValid(i) (resolved and
//                    loaded), KhUserTexSize(i) (texels; 0 when absent), with i a literal 0 - 5 (a non-literal i
//                    samples all six and selects). An absent or still-loading texture reads zero. To move one
//                    (scroll, turn, ripple, a flipbook), transform the uv with the KH_UV_ANIM helpers (KhUvAffine /
//                    KhUvWarp / KhUvFlip) driven by fxMeta.y, the pass's age.
// Helpers there, besides those: KhUiCov(px) (the UI coverage, in the UI lane), Luma (BT.709), KhFxPx() (the factor
// from a 1080-row frame to this one - the builtins' pixel sizes are 1080-row pixels), KhEncFence() (the far plane's
// distance from the encode pair, clamped to [500, 100000] m - a view depth past 0.98 of it is the sky's),
// KhFxProjScale() (the projection's x / y scales), KhHashF / Hash / Hash2 (integer hashes of a float2 - exact at any
// magnitude; a time seed goes in KhHashF's integer lane), KhSin / KhCos (sin / cos reduced to one period - for a
// growing argument), KhGrainGc (the builtin grain's noise), KhSqfToEngine(p) (a position from the params, SQF [x, y,
// zASL], in engine axes: p.xzy - the C++ converts the builtin Pulse's and SunFlare's itself; a user's arrive as
// sent), the KhVmSee and KhWsee helpers (above), KhWorldPosRelAt(uv, z, df) (KhWorldPosFenced's point less fxCam's
// camera, at a given LINEAR view depth z - LinDepth(KhWseeBehindRaw(px)), say, not the raw value; df = z held short
// of the far plane), and:
//   fog          KhFxFogAt(px, d) = the fog the builtin fog scatter sees at px (0 none - 1 all; d its view depth,
//                KhLinZ(px), 1e9 the sky): the engine's height fog (KhFxFogEngine(float2(px), d, fxMeta.zw, m.x,
//                m.y), its sky integrated along the view ray, with the engine's view-distance fade) and the scene's
//                distance-fog passes (KhFxFogPasses over the khFxFog records, filled in the scene chain), as
//                independent media. Not the engine's haze (KhHazeT) or the below-layer term the mesh fog adds. The UI
//                lane's fog lanes are zero (KhFxFogEngine returns 0 there; lighting1.w = 0 says the same of the sun).
//   scatter      KhFsOwnW(s, rm), KhFsSourceFog(ss, sc, sd, cd), KhFsNearFloor(fx, ss, sr, rk, sd, cd): fog scatter's
//                rules for a pixel that spreads its fog's share s over a cone of radius rm s - its own weight, the
//                fog a source scatters into it with (a farther one, the sky behind a ridge, takes the receiver's) and
//                the floor a nearer one puts on its own term, so a silhouette against the sky bleeds (their note
//                below; effect3.hlsl's effect 23 is the worked example).
//   sun          KhFxSunSpot(dir, spos, sp) (a direction - lighting1.xyz for the sun or moon - projected as a point
//                at infinity: false behind the camera or well off screen), KhSunFlareVis(sp) (how much sky shows at
//                it, 25 depth taps - through KH_WSEE's layers by their transmittance), KhSunFlareEdge(spos) (its fade
//                leaving the frame).
// The dynamic-light lanes (dlCtl, dlGlobal, dlView, dlFirst) are filled for a lit object alone: an effect mesh is lit
// by default (updateRender3D's "lit"), and DynLights then returns the lights that reach it; a fullscreen pass is not
// lit and reads zero. The builtin rain lens's own lanes are its alone.
// The builtin's first and last lines, opt-in (one body with its own shader):
//     KhFxBegin(i);                               call it first
//     return KhFxFinish(SampleScene(px), outc, i.pos.xy);
// KhFxBegin loads the object lanes (KhObjLoad) and, for an effect mesh, takes the depth gate, the view-distance cut
// and the far-frame arbitration. KhFxFinish applies what the builtin applies to its result: the localization mask and
// its inverse, the band mask, opacity (color.a) and the blend mode, and packs the output for the composite the pass
// is in (under additive, multiply and screen, and on an effect mesh under normal, the masks scale the opacity, so the
// picture outside them is left as it was - KH_LOCAL_BLEND, KH_FX_MASK_ALPHA). KhFxLocalMask(px) / KhFxBandMask(px)
// are those two masks alone, as KhFxFinish forms them (the inverse applied; through KH_WSEE's layers where they
// cover) - 1 on a pass that is not localized / not banded - for a term shaped before KhFxFinish, or a pass that
// returns raw (KH_FX_MASK_ACC). Without KhFxBegin / KhFxFinish the output is written as returned - the rest of this
// section - and the shader should still begin with KhObjLoad(i.iobj0, i.iobj1).
//   fxMeta     x = effect id, y = the object's age in seconds (formed in double: the most precise clock), zw = the
//              target size in px. uv = i.pos.xy / fxMeta.zw.
//   fxParams0..2  the script's twelve effect parameters, in order (a world
//              position among them arrives in SQF axes: KhSqfToEngine).
//   color      the object colour; color.a is the script's opacity (a
//              fullscreen pass whose color.a <= 0.001 is skipped undrawn).
//   sizeAxes.w the blend mode (0 normal, 1 additive, 2 multiply, 3 screen, 4 lighten, 5 darken).
//   centerSize.w  which composite the pass is in:
//     > 0.5 A FULLSCREEN pass. Opacity, blend mode, the localization mask (localParams0/1, localRadii) and the band
//           mask (bandParams) are NOT applied for a user shader - KhFxFinish applies them as the builtin does.
//           1.0 = the scene chain (and a picture-in-picture's localized pass): t0 is the engine's HDR scene colour at
//           its scene resolve, before the engine's own post-processing and tonemap, and the output REPLACES the
//           pixel's colour (no hardware blend), in the same linear HDR units. Write every pixel - return
//           SampleScene(px) to leave one as it was - and never discard or clip: the chain's targets are not cleared,
//           so a discarded pixel shows an older frame's or the pre-chain picture. Return alpha 1 (KhFxFinish does):
//           the scene chain's last pass writes the alpha into the engine's target too (a picture-in-picture's passes
//           write the colour alone: its alpha stays the engine's). Return finite, non-negative colour: a NaN or inf
//           runs on into the later passes that read the pixel and into the game's own post-processing.
//           1.25 = the UI phase (a pass the script set to affect the UI): t0 is the finished display frame,
//           post-tonemap, UI included (0 - 1), its alpha the UI coverage, and the hardware lerps the frame toward the
//           output by that coverage (rgb only). With the script's UI spill, t0's rgb is premultiplied by that
//           coverage, sizeAxes.w reads 0 (normal) and the hardware ADDS the output to the frame's uncovered share -
//           output + frame * (1 - coverage) - so it replaces the UI where the UI is opaque and spills past it.
//     < 0.5 An EFFECT MESH. The output is hardware-blended by the blend mode; return it packed as the builtin's final
//           lines do (normal: rgb, alpha = opacity; additive / screen: rgb * opacity, 1; multiply: lerp(1, rgb,
//           opacity), 1; lighten / darken: lerp(scene, rgb, opacity), 1) - or return KhFxFinish, which does. The
//           builtin's first lines are not applied for you either (KhFxBegin applies them): its depth gate against
//           depthParams, the object's view-distance cut (discard past khObjCut, set by KhObjLoad) and the far-frame
//           arbitration - without them an effect mesh draws past the engine's object view distance.
//
// ---- Both kinds: the accessors below. -------------------------------------
// They read lanes every pass that can run a user shader fills (the mesh flush and injection, the PIP and view-model
// passes, the scene and UI effect chains, the PIP's effect passes).
//   KhUserTime()          the object's age: seconds since it was created - the animation clock. Per object, bucket
//                         instances included. Formed in float on a clock that restarts with each mission, so it
//                         resolves ~0.5 ms after an hour of mission (an effect's fxMeta.y is exact).
//   KhUserSessionTime()   seconds since the mission's render clock started; one
//                         value for every object - a shared clock.
//   KhUserCameraValid()   true when the camera lanes below are filled.
//   KhUserCameraPos()     the pass camera's position (engine axes, absolute). For a PIP pass it is the PIP camera.
//   KhUserCameraRight() / KhUserCameraUp() / KhUserCameraForward()
//                         the pass camera's axes (engine axes, unit length);
//                         Forward is the direction it looks.
//   KhUserViewDir(wpos)   unit vector from wpos toward the camera.
//   KhUserUv()            MATERIAL only: the pixel's texture UV, as the builtin sampled it (after the material's
//                         texture animation); KhUserUvRaw() the mesh's own, KhUserUvDx() / KhUserUvDy() its
//                         gradients.
//   KhUvMatrix / KhUvAffine / KhUvWarp / KhUvFlip / KhUvNrm   both: texture-
//                         coordinate animation (KH_UV_ANIM, below).
//   KhUserPixel()         MATERIAL only: the pixel's position in the target, px (an effect has i.pos.xy).
//   KhUserTex*            both: a material's user0 .. user5 (the MATERIAL
//                         section), an effect's "textures" (the EFFECT section).
//   KhUserMat* / KhUserGeomNormal / KhUserTangent / KhUserBitangent /
//   KhUserPerturb / KhUserFrontFace   MATERIAL only: see the MATERIAL section.
//   KhUserVtxWorld(p) / KhUserVtxWorldDir(d) / KhUserVtxObjectDir(d)
//                         object space <-> world, for the VERTEX stage.
// Object lanes (per-object draws: the drawn object; KH_INST_DOC: on a bucket draw - one call drawing many objects of
// one mesh and material - they are one member's, the bucket's representative, for every instance, and no accessor
// returns them per instance): centerSize.xyz its centre (engine axes), sizeAxes.xyz its edge lengths (engine axes),
// objRot0..2.xyz its rotation rows (objRot0.w = 1 when set, identity otherwise); the CB's color likewise. A material
// whose look depends on them varies wrongly across a bucket. Per instance a material has KhUserTime() (each one's
// age), khObjAmb / khObjDif, the pixel's own position, normal, tangent and UV, and the colour the builtin applies
// (the instance's).
// Scene lanes: lighting1.xyz the unit vector toward the sun or moon (w = 1 valid), lighting2.rgb its colour and
// lightAmb.rgb the ambient (HDR scene units).
// ===========================================================================
float  KhUserSessionTime()   { return khUserObj.x; }
float  KhUserTime()          { return max(khUserObj.x - (khObjBirth > -1.0e29f ? khObjBirth : khUserObj.y), 0.0f); }
bool   KhUserCameraValid()   { return khUserCam.w >= 0.5f; }
float3 KhUserCameraPos()     { return khUserCam.xyz; }
float3 KhUserCameraRight()   { return khUserCamRot[0].xyz; }
float3 KhUserCameraUp()      { return khUserCamRot[1].xyz; }
float3 KhUserCameraForward() { return khUserCamRot[2].xyz; }
float3 KhUserViewDir(float3 khuv_wpos)
{
    const float3 khuv_d = khUserCam.xyz - khuv_wpos;
    return khuv_d * rsqrt(max(dot(khuv_d, khuv_d), 1.0e-12f));
}
// Set by PSMain / PSComposite immediately before KhUserShade (TWIN).
static float2 khUserUvPs = float2(0.0f, 0.0f);
static float2 khUserPxPs = float2(0.0f, 0.0f);
static float2 khUserUvRawPs = float2(0.0f, 0.0f);   // KH_UV_ANIM: the mesh's own uv.
static float4 khUserUvGPs = float4(0.0f, 0.0f, 0.0f, 0.0f);   // KH_UV_ANIM: KhUserUv's ddx (xy) and ddy (zw).
float2 KhUserUv()    { return khUserUvPs; }
float2 KhUserPixel() { return khUserPxPs; }
float2 KhUserUvRaw() { return khUserUvRawPs; }
float2 KhUserUvDx()  { return khUserUvGPs.xy; }
float2 KhUserUvDy()  { return khUserUvGPs.zw; }
// KH_USER_VS: object space (metres, the object's centre at 0) to world and a direction likewise - the builtin
// transform's own arithmetic (KhVsCore), for a vertex stage that needs to know where it is (wind, a world-space
// wave). The object lanes are the drawn object's on every per-object draw, which the vertex stage's pass is. Absolute
// floats (wpos's precision): a round trip back to object space (minus centerSize) lands on that world step.
float3 KhUserVtxWorld(float3 khvw_p)
{
    if (objRot0.w < 0.5f) return centerSize.xyz + khvw_p;
    return centerSize.xyz + khvw_p.x * objRot0.xyz + khvw_p.y * objRot1.xyz + khvw_p.z * objRot2.xyz;
}
float3 KhUserVtxWorldDir(float3 khvw_d)
{
    if (objRot0.w < 0.5f) return khvw_d;
    return khvw_d.x * objRot0.xyz + khvw_d.y * objRot1.xyz + khvw_d.z * objRot2.xyz;
}
// World direction back to object space (the rows are orthonormal).
float3 KhUserVtxObjectDir(float3 khvw_d)
{
    if (objRot0.w < 0.5f) return khvw_d;
    return float3(dot(khvw_d, objRot0.xyz), dot(khvw_d, objRot1.xyz), dot(khvw_d, objRot2.xyz));
}
// KH_UV_ANIM - texture-coordinate animation. The materials' (the script's uv* / flipbook params, KhMatUv) is built
// from these; any shader may use them on its own coordinates - a user or builtin effect moving its KhUserTex maps by
// fxMeta.y, a material's KhUserShade. Pure functions of their arguments, in every unit.
//   KhUvMatrix(scale, radians)  A = S * R as float4 (a00, a01, a10, a11) - the lookup turns by the angle (the picture
//                               the other way), then scales.
//   KhUvAffine(uv, A, c)        A * uv + c. About the tile's centre: c = 0.5 - KhUvAffine(0.5, A, 0) + offset.
//   KhUvWarp(uv, amp, freq, ph) uv + amp * sin(2 pi (freq * uv.yx + ph)): a
//                               ripple across each axis; ph its phase per axis.
//   KhUvFlip(uv, grid, frame)   the uv inside cell 'frame' (0 ..) of a grid.x x grid.y sheet, row by row from v = 0.
//                               Sample it with the gradients of uv divided by grid (no mip seam at the cells' edges).
//   KhUvNrm(xy, A)              a tangent-space normal's xy, sampled at A * uv + c, in the surface's own frame: A^T *
//                               xy / sqrt|det A| - the bumps turn, mirror and stretch with the picture (the slope of
//                               the surface it draws, A^T * xy); a uniform tiling does not steepen them; a
//                               non-uniform one steepens them along its denser axis and flattens them along the
//                               other, by the root of the scales' ratio.
// Precision: a float time t resolves t x 6e-8 s (2 ms after 10 hours), and a motion of rate x t inherits that step
// times the rate - frac(rate * t) keeps the arguments small but cannot win the lost resolution back. fxMeta.y is the
// pass's own age (formed in double, then float), so it stays fine for a pass that is not hours old; the material
// params form theirs in double (C++ kh_uv_anim_lanes) and never coarsen.
float4 KhUvMatrix(float2 khuv_s, float khuv_a)
{
    float khuv_sn, khuv_cs;
    sincos(khuv_a, khuv_sn, khuv_cs);
    return float4(khuv_s.x * khuv_cs, -khuv_s.x * khuv_sn, khuv_s.y * khuv_sn, khuv_s.y * khuv_cs);
}
float2 KhUvAffine(float2 khuv_uv, float4 khuv_m, float2 khuv_c)
{
    return float2(khuv_m.x * khuv_uv.x + khuv_m.y * khuv_uv.y, khuv_m.z * khuv_uv.x + khuv_m.w * khuv_uv.y) + khuv_c;
}
float2 KhUvWarp(float2 khuv_uv, float khuv_amp, float khuv_f, float2 khuv_ph)
{
    return khuv_uv + khuv_amp * sin(6.28318531f * frac(khuv_f * khuv_uv.yx + khuv_ph));
}
float2 KhUvFlip(float2 khuv_uv, float2 khuv_g, float khuv_fr)
{
    const float khuv_i = floor(khuv_fr);
    const float khuv_r = floor((khuv_i + 0.5f) / khuv_g.x);
    const float2 khuv_c = float2(khuv_i - khuv_r * khuv_g.x, khuv_r);
    // Held 1e-6 inside the cell's edges (a 60th of a texel on a 16384-texel sheet): frac of a uv a hair below a whole
    // number rounds to 1.0 in fp32, and the sum and the division round either way - each lands a texel index on the
    // neighbouring frame (the last column's on 1.0: the first column's, once wrapped).
    return clamp((frac(khuv_uv) + khuv_c) / khuv_g, khuv_c / khuv_g + 1.0e-6f, (khuv_c + 1.0f) / khuv_g - 1.0e-6f);
}
float2 KhUvNrm(float2 khuv_xy, float4 khuv_m)
{
    const float khuv_d = abs(khuv_m.x * khuv_m.w - khuv_m.y * khuv_m.z);
    return float2(khuv_m.x * khuv_xy.x + khuv_m.z * khuv_xy.y, khuv_m.y * khuv_xy.x + khuv_m.w * khuv_xy.y)
         * rsqrt(max(khuv_d, 1.0e-12f));
}
#if KH_USER_MAT
// KH_USER_FRAME: set by PSMain / PSComposite where they build the mapped normal, before KhUserShade (TWIN).
static float3 khUserNPs = float3(0.0f, 0.0f, 1.0f);
static float3 khUserTPs = float3(0.0f, 0.0f, 0.0f);
static float3 khUserBPs = float3(0.0f, 0.0f, 0.0f);
static float  khUserFacePs = 1.0f;
float3 KhUserGeomNormal() { return khUserNPs; }
float3 KhUserTangent()    { return khUserTPs; }
float3 KhUserBitangent()  { return khUserBPs; }
bool   KhUserFrontFace()  { return khUserFacePs > 0.0f; }
float3 KhUserPerturb(float3 khup_t)
{
    if (dot(khUserTPs, khUserTPs) <= 0.0f) return khUserNPs;
    return normalize(khUserTPs * khup_t.x + khUserBPs * khup_t.y + khUserNPs * khup_t.z);
}
// KH_USER_VS: what KhUserVertex receives and hands back (the VERTEX stage section of the USER SHADER CONTRACT).
// Declared for every material compile, so the pixel twins parse a file that defines the stage.
struct KhUserVtx {
    float3 pos;       // Object space, metres. WRITE.
    float3 nrm;       // Object space, unit on entry. WRITE.
    float3 tan;       // Object space, unit on entry (zero = none). WRITE.
    float  tanSign;   // The tangent's handedness.
    float2 uv;        // WRITE: the pixel stage's UV.
    float3 rest;      // READ: pos on entry.
    uint   id;        // READ: the vertex's index in the mesh's vertex array.
};
#endif

#define KH_RPDB_GC_M 0.008f
#define KH_HERO_TEXEL_M 0.001f
#define KH_SLOPE_TW(khtw) (min((khtw), KH_HERO_TEXEL_M))
#define KH_RPDB_GC(khtw) (min(8.0f * (khtw), KH_RPDB_GC_M))

float KhSceneMeters(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    if (denom > -1e-7f) return 1e9f;
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

// Heightfield occlusion is marched camera->fragment: per-pixel, temporally
// stable, altitude- and LOD-independent. Do not swap in a screen-space variant.
Texture2D<float> terrainHeightTex : register(t10);
// KH_SHADOW_GATHER's sampler (its note is at KhPfMu below), declared here because KhThmHeight reads through it.
SamplerState khPfSamp : register(s1);   // Linear-clamp.

// KH_THM_EXACT - the terrain height (ASL metres) at engine-space (x, z), as the engine's own ground has it: between
// the nodes, two flat triangles per cell, split along the diagonal kh_cloth_ground_prepare measured for the cell's
// class ((i & 1) | (j & 1) << 1 of its first node's indices; 0 = the diagonal from (i, j) to (i + 1, j + 1), 1 = the
// one from (i + 1, j) to (i, j + 1), 2 = a bilinear fit where the split is not known - a grid not at the matrix's
// native spacing, or a split not recognised), carried as base-3 digit k of thmParams.w - 1. C++ twin:
// kh_cloth_ground_plane. Returns -1e6 outside the heightfield (= no occlusion evidence). KH_THM_EDGE: data up to
// the far node lines, exclusive - g < thmMeta - 1 (integers) keeps int(g) + 1 inside the texture with no margin;
// the twin also takes the node line itself (it clamps the cell), a set of measure zero.
float KhThmHeight(float2 xz)
{
    float2 g = (xz - thmParams.xy) / max(thmParams.z, 1e-3f);
    if (g.x < 0.0f || g.y < 0.0f ||
        g.x >= thmMeta.x - 1.0f || g.y >= thmMeta.y - 1.0f) return -1.0e6f;
    int2 c0 = int2(g);
    float2 f = g - float2(c0);
    // KH_SHADOW_GATHER's form: the 2 x 2 block (c0, c0 + 1) in one fetch, sampled at the corner its texels share,
    // half a texel from every boundary. The texture is exactly thmMeta.xy texels (kh_thm_upload_body), and the
    // test above keeps c0 + 1 inside it, so the addressing mode never engages: the texels, and the lerp, are the
    // four Loads' bit for bit.
    const float4 khth_g = terrainHeightTex.Gather(khPfSamp, (float2(c0) + 1.0f) / thmMeta.xy);
    const float khth_h00 = khth_g.w, khth_h10 = khth_g.z, khth_h01 = khth_g.x, khth_h11 = khth_g.y;
    const uint2 khth_p = uint2(c0) & 1u;
    const uint khth_cls = khth_p.x | (khth_p.y << 1);
    const float khth_d = khth_cls == 0u ? 1.0f : (khth_cls == 1u ? 3.0f : (khth_cls == 2u ? 9.0f : 27.0f));
    // Each floor takes an integer plus a half over its divisor, a quotient at least 0.5 / 27 from any integer:
    // D3D11 divides to 2.5 ulp (x * rcp(d) on most parts), and an exact integer quotient (12 / 3, 9 / 9 - the
    // code of every alternating split) could land just under it and drop a digit.
    const float khth_q = floor((max(thmParams.w - 1.0f, 0.0f) + 0.5f) / khth_d);
    const float khth_sp = khth_q - 3.0f * floor((khth_q + 0.5f) / 3.0f);
    if (khth_sp < 0.5f) {   // The diagonal from (i, j) to (i + 1, j + 1).
        return f.x >= f.y ? khth_h00 + (khth_h10 - khth_h00) * f.x + (khth_h11 - khth_h10) * f.y
                          : khth_h00 + (khth_h11 - khth_h01) * f.x + (khth_h01 - khth_h00) * f.y;
    }
    if (khth_sp < 1.5f) {   // The diagonal from (i + 1, j) to (i, j + 1).
        return f.x + f.y <= 1.0f
             ? khth_h00 + (khth_h10 - khth_h00) * f.x + (khth_h01 - khth_h00) * f.y
             : khth_h11 + (khth_h11 - khth_h01) * (f.x - 1.0f) + (khth_h11 - khth_h10) * (f.y - 1.0f);
    }
    return lerp(lerp(khth_g.w, khth_g.z, f.x), lerp(khth_g.x, khth_g.y, f.x), f.y);
}

// KH_THM_EXACT - the scale the heightfield's tolerances were tuned at: the cell of a grid decimated to at most 1025
// nodes a side (stride (n + 1024) / 1025 over the n nodes a side), whatever the heightfield's own spacing. They
// absorb what the engine's own drawn terrain departs from its exact ground (its distant LOD), which a finer
// heightfield does not shrink: the march's end skip and its sub-cell slack, the arbitration's terrain tolerance.
float KhThmTolCell()
{
    return thmParams.z * floor((thmMeta.x + 1024.0f) / 1025.0f);
}

// Both callers discard the fragment when clearance + 1.5 tolerance cells (KhThmTolCell) falls below -thmMeta.z;
// the running minimum can only fall, so the march stops once that test holds (the callers' own expression, so the
// decision is identical). PSComposite's arbitration cap (KH_ARB_DEPTH) reads the value too; a fragment the march
// stopped early for is discarded (occ) before anything the cap computed is used.
float KhThmClearance(float3 cam, float3 wp)
{
    float mc = 1.0e9f;
    float len = distance(cam, wp);
    if (len < 1.0f) return mc;
    float skip = max(2.0f * KhThmTolCell(), 25.0f);
    float t0 = saturate(skip / len);
    float t1 = 1.0f - saturate(skip / len);
    if (t1 <= t0) return mc;

    [loop] for (int s = 0; s < 96; ++s) {
        float t = lerp(t0, t1, ((float)s + 0.5f) / 96.0f);
        float3 p = lerp(cam, wp, t);
        float h = KhThmHeight(p.xz);
        if (h > -1.0e5f) mc = min(mc, p.y - h);
        if (mc + 1.5f * KhThmTolCell() < -thmMeta.z) break;   // Decided: the callers discard.
    }

    return mc;
}

// Shared object rotation (see objRot0): identity while unfilled, so only the
// mesh-transform fill sites carry the matrix.
float3 KhRotate(float3 p)
{
    if (objRot0.w < 0.5f) return p;
    return p.x * objRot0.xyz + p.y * objRot1.xyz + p.z * objRot2.xyz;
}

Texture2D<float> khSunDepth : register(t11);
Texture2D<float> khSunDepth2 : register(t25);   // Hero sun map (KH_SUN_HERO_MAP).
Texture2D<float> khSunDepth3 : register(t26);   // Mid cascade band (KH_SUN_CASCADE).
Texture2D<float> khSunDepth4 : register(t27);   // Outer cascade band (KH_SUN_CASCADE).
Texture2D<float> khSunDepth5 : register(t32);   // Far band (KH_SUN_FAR_BAND; t28-t31 taken).
// KH_SELF_PREFILTER: per-band moment pyramids (mu, E[z^2]) at half resolution +
// mips; the self kernel samples them under minification.
Texture2D<float2> khSunPf2 : register(t29);   // Hero.
Texture2D<float2> khSunPf3 : register(t30);
Texture2D<float2> khSunPf4 : register(t31);   // Outer.
Texture2D<float2> khSunPf5 : register(t20);   // Far (KH_FAR_PF; t20 was free in every unit).
// KH_SHADOW_GATHER: s1 (khPfSamp, declared above KhThmHeight) is also the shadow compares' Gather sampler
// (KhSunBilinT, KhSelfTapT, KhDlsBilin). Gather returns the 2 x 2 texels a bilinear sample would read, in one
// fetch, whatever the filter; only the address mode matters, and it is CLAMP - samp_pf's, or the default state a
// null binding invokes. Every pass that runs those kernels binds it: the mesh passes (injection, flush, PIP,
// inFront), the mask cast and the DLS world pass. KH_GATHER_NO_LOD: Gather computes no level of detail and takes
// no derivatives - it reads the view's most detailed mip on every path, in divergent flow too (these maps are
// single-mip views besides). It is also KhPfMu's sampler (a trilinear read of the moment pyramids at an explicit
// level) and the Gather of KhThmHeight, ShadowMapSample and KhBandTap4 - all on those same passes.
float2 KhPfMu(Texture2D<float2> khpb_t, float2 khpb_uv, float khpb_base, float khpb_lod)
{
    // The moments (mu, E[z^2]) at lod, filtered trilinearly: one hardware sample through s1 (samp_pf is
    // MIN_MAG_MIP_LINEAR, clamp - and a null s1 is the default state, the same) over the pyramid's full mip
    // chain (R32G32_FLOAT, filterable). The textbook read for a moment map: moments filter linearly. khpb_base
    // (the pyramid's size) is kept in the signature for its callers; the hardware takes each level's own size.
    return khpb_t.SampleLevel(khPfSamp, khpb_uv, max(khpb_lod, 0.0f)).xy;
}
Texture2D<float4> khShadowPre : register(t21);
Texture2D<float4> khShadowPost : register(t22);

#define KH_PRIME_V   0.99607843f

int2 KhMaskPx(float2 khsp_xy)
{
    return clamp(int2(khsp_xy), int2(0, 0),
                 int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
}

// Guard the denominator, do not clamp it.
float KhStenRatio(float khsp_post, float khsp_pre)
{
    if (khsp_pre <= 0.02f) return 1.0f;
    return saturate(khsp_post / khsp_pre);
}

float KhStenTerm(float2 khsp_xy)
{
    int2 khsp_p = KhMaskPx(khsp_xy);
    float khsp_post = khShadowPost.Load(int3(khsp_p, 0)).r;
    float khsp_pre  = khShadowPre.Load(int3(khsp_p, 0)).r;
    return KhStenRatio(khsp_post, khsp_pre);
}

Texture2D<uint2> khVolSten  : register(t24);
// KH_VOL_WITNESS: the same copy's depth plane (R24_UNORM_X8).
Texture2D<float> khVolDepth : register(t23);
// KH_VOL_FOOT: our footprint's view distance (m) at the copy's seam frame, drawn
// by the seam with no depth test; 1e30 = none of our surfaces there.
Texture2D<float> khVolFoot : register(t33);
// Any non-zero count reads as shadowed here. KhVolShadowed (below) also refuses
// a count of 128 or more (a wrapped count), so the two differ there.
Texture2D<uint2> khMirSten  : register(t28);
float KhMirUnit(float2 khmu_px, float khmu_w, float khmu_h)
{
    // KH_MIR_ZOOM: the mirror is drawn in the volume copy's raster (the seam's projection), so this pixel is looked
    // up through the same map as the count (stenVol3, KhVolTerm): identity when the projections agree.
    if (stenVol3.x > 0.0f) khmu_px = khmu_px * stenVol3.xy + stenVol3.zw;
    int2 khmu_p = int2(clamp(khmu_px.x, 0.0f, khmu_w - 1.0f),
                       clamp(khmu_px.y, 0.0f, khmu_h - 1.0f));
    return (khMirSten.Load(int3(khmu_p, 0)).g != 0u) ? 0.0f : 1.0f;
}
// The fade starts at KH_SUN_FADE_START of the range and completes at 99.5% (the 0.5% guard completes before the
// per-caster cliff at R), so every contract built on the 0.995 R rim (tier windows, grid domain, eligibility)
// holds; a short band at the rim pops long shadow tips in and out under camera motion.
#define KH_SUN_FADE_START 0.72f
float KhSunRangeFadeAt(float3 khrf_p, float3 khrf_o)
{
    if (mirMeta.w < 0.5f) return 1.0f;
    float3 khrf_v = khrf_p - khrf_o;
    float  khrf_fh = 1.0f - smoothstep(KH_SUN_FADE_START * mirMeta.w, 0.995f * mirMeta.w,
                                       length(khrf_v.xz));
    float  khrf_fy = 1.0f - smoothstep(KH_SUN_FADE_START * mirMeta.w, 0.995f * mirMeta.w,
                                       abs(khrf_v.y));
    return min(khrf_fh, khrf_fy);
}
float KhSunRangeFade(float3 khrf_p)
{
    if (mirMeta.w < 0.5f) return 1.0f;
    // The domain is a vertical cylinder around the camera, not a sphere: from altitude a sphere spends its reach
    // vertically. Horizontal and vertical fade separately over the same radius; the min keeps the rim soft on both.
    return KhSunRangeFadeAt(khrf_p, sunOrigin.xyz);
}

int2 KhVolPx(float2 khvp_xy)
{
    return clamp(int2(khvp_xy), int2(0, 0),
                 int2((int)stenVol.z - 1, (int)stenVol.w - 1));
}

// The stencil count at a texel. x24_TYPELESS_G8_UINT puts stencil in .g.
uint KhVolCount(int2 khvc_p)
{
    return khVolSten.Load(int3(khvc_p, 0)).g;
}

bool KhVolShadowed(uint khvd_c)
{
    return khvd_c != 0u && khvd_c < 128u;
}

// KH_VOL_WITNESS. The copy is read a frame late (the injection precedes the engine's counting draws), so at a
// pixel this surface did not cover then, the count is another surface's - the body behind a swinging cape, in its
// own shadow, shows through the cape's moving edge. The witness: the distance our surface stood at at the pixel in
// the copy's frame (KhVolWitnessDist - the KH_VOL_FOOT mask, else the copy's depth decoded) must be this
// fragment's view distance within KH_VOL_WITNESS_M or KH_VOL_WITNESS_REL of it (the far arbitration guard's relative
// margin, C++ KH_FAR_ARB_GUARD_REL). A failed witness takes the mean of the ring of neighbours KH_VOL_WITNESS_R pixels
// out that pass it (the surface's own counts beside the uncovered edge), then of the ring twice as far (an edge that
// moved further in the frame), else lit: a count that belongs to no part of this surface is never used.
static const float KH_VOL_WITNESS_M = 0.05f;
static const float KH_VOL_WITNESS_REL = 0.02f;
static const int   KH_VOL_WITNESS_R = 2;
// The distance the witness compares against at a copy pixel (m), <= 0 = no surface. KH_VOL_FOOT: our own
// footprint mask when the copy has one - no encode to decode, so a moving near plane cannot shift it; otherwise
// the copy's depth plane decoded through the seam's encode (stenVol.xy, stenVol2.yw). No finite distance (at or
// past infinity) = no surface. A cleared depth texel (1.0) decodes to the far plane's distance only where the
// range's MaxDepth is 1; under the world range [0.011, 0.999] its ndc exceeds 1, so it decodes to no surface where
// f / n exceeds ~989 (near 0.2, far 12000), else to a distance past the far plane (near 5, far 1600: ~2363 m) - a
// fragment short of the far never matches it either way.
float KhVolWitnessDist(int2 khvw_p)
{
    if (stenVol4.x >= 0.5f) {
        const float khvw_f = khVolFoot.Load(int3(khvw_p, 0));
        return (khvw_f > 0.0f && khvw_f < 1.0e29f) ? khvw_f : -1.0f;
    }
    const float khvw_ndc = (khVolDepth.Load(int3(khvw_p, 0)) - stenVol2.y) / (stenVol2.w - stenVol2.y);
    const float khvw_den = khvw_ndc - stenVol.x;
    if (khvw_den > -1.0e-7f) return -1.0f;
    return stenVol.y / khvw_den;
}
bool KhVolWitness(int2 khvw_p, float khvw_z)
{
    const float khvw_d = KhVolWitnessDist(khvw_p);
    return khvw_d > 0.0f &&
           abs(khvw_d - khvw_z) <= max(KH_VOL_WITNESS_M, KH_VOL_WITNESS_REL * khvw_z);
}

// The eight neighbours khvr_r pixels out: the lit sum and count of those that pass the witness.
void KhVolRing(int2 khvr_p, float khvr_z, int khvr_r, inout float khvr_acc, inout float khvr_n)
{
    const int2 khvr_mx = int2((int)stenVol.z - 1, (int)stenVol.w - 1);
    [unroll] for (int khvr_k = 0; khvr_k < 8; ++khvr_k) {
        const int2 khvr_d = (khvr_k == 0) ? int2( 1,  0) : (khvr_k == 1) ? int2(-1,  0)
                          : (khvr_k == 2) ? int2( 0,  1) : (khvr_k == 3) ? int2( 0, -1)
                          : (khvr_k == 4) ? int2( 1,  1) : (khvr_k == 5) ? int2(-1,  1)
                          : (khvr_k == 6) ? int2( 1, -1) :                 int2(-1, -1);
        const int2 khvr_q = clamp(khvr_p + khvr_d * khvr_r, int2(0, 0), khvr_mx);
        if (KhVolWitness(khvr_q, khvr_z)) {
            khvr_acc += KhVolShadowed(KhVolCount(khvr_q)) ? 0.0f : 1.0f;
            khvr_n += 1.0f;
        }
    }
}

// khvt_z = the fragment's view distance (SV_Position.w); <= 0 = no witness (a texel that wrote no depth).
float KhVolTerm(float2 khvt_raster, float khvt_z)
{
    // KH_VOL_ZOOM: this pixel's view ray in the copy's pixel grid. View depth does not change with the projection,
    // so khvt_z stands as it is.
    const float2 khvt_c = (stenVol3.x > 0.0f) ? khvt_raster * stenVol3.xy + stenVol3.zw : khvt_raster;
    const int2 khvt_p = KhVolPx(khvt_c);
    // Armed with a witness source: the footprint mask (KH_VOL_FOOT) or the depth plane's encode.
    // KH_VOL_WITNESS_BRANCH: a branch, not || - fxc evaluates every operand, so the witness's Load would run on
    // every pixel, unarmed and depth-less ones too. The witness takes no gradient.
    bool khvt_own = true;
    [branch] if (khvt_z > 0.0f && (stenVol4.x >= 0.5f || stenVol2.w > stenVol2.y))
        khvt_own = KhVolWitness(khvt_p, khvt_z);
    if (khvt_own) {
        return KhVolShadowed(KhVolCount(khvt_p)) ? 0.0f : 1.0f;
    }
    float khvt_acc = 0.0f;
    float khvt_n = 0.0f;
    KhVolRing(khvt_p, khvt_z, KH_VOL_WITNESS_R, khvt_acc, khvt_n);
    if (khvt_n <= 0.0f) KhVolRing(khvt_p, khvt_z, 2 * KH_VOL_WITNESS_R, khvt_acc, khvt_n);
    return khvt_n > 0.0f ? khvt_acc / khvt_n : 1.0f;
}

float KhStenUnit(float2 khsu_raster, float khsu_z)
{
    if (stenVol2.x >= 0.5f) return KhVolTerm(khsu_raster, khsu_z);
    return KhStenTerm(khsu_raster);
}

// Every sun bilinear compare, ring and kernel. Floor, not truncate (int2() rounds toward zero while frac is
// x - floor(x), so they disagree wherever tx < 0), and the map's edge texel answers for every texel past it on both
// sides (KhSelfTapT's and KhDlsBilin's rule too): a texel past the map read as 0 is the nearest depth = occluded. The
// wide kernel samples up to KH_PCSS_RMAX texels from a uv inside the window; the ring's widest reach is khcw_sp (<= 4)
// texels past a uv guarded to 0.002 / 0.998 - 3.7 texels inside a 4096 map, 0.4 OUTSIDE a 2048 one - so the edge rule
// is what keeps a lower KH_SUN_*_BASE from drawing an occluded rim along every tier window edge. The wide kernel needs
// both rules at every map size; the ring needs them only at the rim, and only below the shipped 4096. Here the edge
// rule is the Gather sampler's CLAMP addressing; it clamps no index.
// KH_SHADOW_GATHER: the 2 x 2 block (lo, lo + 1 on each axis) in one Gather at the corner its four texels share,
// (lo + 1) / size - half a texel from every texel boundary, so the hardware selects exactly lo and lo + 1 on each
// axis. CLAMP repeats the edge texel as a per-index clamp to [0, size - 1] would, and the texels come back unfiltered,
// so every compare and weight is the four-Load form's, bit for bit. Components: w = (lo.x, lo.y), z = (lo.x + 1,
// lo.y), x = (lo.x, lo.y + 1), y = (lo.x + 1, lo.y + 1). size must be the texture's own width: every sun map is
// created at its size lane (kh_sun_map_ensure), and the DLS kernel takes GetDimensions'.
float KhSunBilinT(Texture2D<float> khcb_m, float khcb_sz, float2 uv, float z)
{
    float2 tx = uv * khcb_sz - 0.5f;
    float2 f = frac(tx);
    const float2 khcb_lo = floor(tx);
    const float4 khcb_g = khcb_m.Gather(khPfSamp, (khcb_lo + 1.0f) / khcb_sz);   // KH_SHADOW_GATHER.
    float o00 = (z > khcb_g.w) ? 1.0f : 0.0f;
    float o10 = (z > khcb_g.z) ? 1.0f : 0.0f;
    float o01 = (z > khcb_g.x) ? 1.0f : 0.0f;
    float o11 = (z > khcb_g.y) ? 1.0f : 0.0f;
    return lerp(lerp(o00, o10, f.x), lerp(o01, o11, f.x), f.y);
}

float KhSunSoftT(Texture2D<float> khcs_m, float khcs_sz, float2 uv, float z)
{
    float khcs_o = 0.75f / max(khcs_sz, 1.0f);
    return (KhSunBilinT(khcs_m, khcs_sz, uv, z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2( khcs_o, 0.0f), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(-khcs_o, 0.0f), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(0.0f,  khcs_o), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(0.0f, -khcs_o), z)) * 0.2f;
}

float SunShadowCompareSoft(float2 uv, float z)  { return KhSunSoftT(khSunDepth, sunMeta.y, uv, z); }

// The cast chain's contact ring (KhSunPcssWT answers with it where the penumbra fits): a 3 x 3 ring of bilinear
// compares at +-khcw_sp texels. The spread follows the receiver footprint (clamped 1..4), so the edge is averaged
// over the texels it covers and a one-texel re-rasterization shift under a moving sun is a fraction of the transition.
float KhSunSoftWT(Texture2D<float> khcw_m, float khcw_sz, float2 uv, float z, float khcw_sp)
{
    float khcw_o = khcw_sp / max(khcw_sz, 1.0f);
    float khcw_a = KhSunBilinT(khcw_m, khcw_sz, uv, z);
    [unroll] for (int khcw_j = -1; khcw_j <= 1; ++khcw_j) {
        [unroll] for (int khcw_i = -1; khcw_i <= 1; ++khcw_i) {
            if (khcw_i == 0 && khcw_j == 0) continue;
            khcw_a += KhSunBilinT(khcw_m, khcw_sz, uv + float2((float)khcw_i, (float)khcw_j) * khcw_o, z);
        }
    }
    return khcw_a / 9.0f;
}

// KH_PCSS: percentage-closer soft shadows on every sun compare, both chains and the union. The sun is a disc of
// KH_SUN_DIAM_TAN angular diameter, so a caster's penumbra widens by that fraction of the occluder-to-receiver
// distance along the light: a 0.5 m post at 100 m casts a ~0.9 m penumbra and its shadow dissolves, a 30 m wall at
// 1 km keeps a solid core under a ~9 m soft rim. That is what stops a grazing sun stretching small shadows to the
// horizon - no distance cutoff, no shadow-range term. The blocker search averages every occluder within the widest
// penumbra the tier's depth window allows (the centre texel included, so a point under a caster is never missed),
// which keeps overlapping casters on one continuous estimate; a tap-count fade by caster distance cannot. Where the
// penumbra fits the contact ring the ring answers unchanged.
#define KH_SUN_DIAM_TAN 0.0093f   // tan(0.53 deg): the sun's (and the moon's) angular diameter.
#define KH_PCSS_RMAX 128.0f       // Search and kernel radius cap, tier texels.
static const float2 KH_PCSS_DISC[16] = {
    float2(-0.94201624f, -0.39906216f), float2( 0.94558609f, -0.76890725f),
    float2(-0.09418410f, -0.92938870f), float2( 0.34495938f,  0.29387760f),
    float2(-0.91588581f,  0.45771432f), float2(-0.81544232f, -0.87912464f),
    float2(-0.38277543f,  0.27676845f), float2( 0.97484398f,  0.75648379f),
    float2( 0.44323325f, -0.97511554f), float2( 0.53742981f, -0.47373420f),
    float2(-0.26496911f, -0.41893023f), float2( 0.79197514f,  0.19090188f),
    float2(-0.24188840f,  0.99706507f), float2(-0.81409955f,  0.91437590f),
    float2( 0.19984126f,  0.78641367f), float2( 0.14383161f, -0.14100790f)
};
// KH_PCSS_CELL: the disc is rotated per cell (interleaved gradient noise over the cell index). A cell is the map's
// texel split 2^k ways per axis, k the least that keeps a cell within the receiver's pixel footprint (khpr_fw = fwidth
// of the texel coordinate, its smaller axis; texels no larger than a pixel keep k = 0), so the rotation changes from
// pixel to pixel however far the map's texels are magnified (one rotation across a whole magnified texel draws the
// disc noise as blocks of that texel's size). The index is the world-fixed lattice (the texel index + khpr_lat:
// sunLat* for a camera tier, 0 for the union, whose window moves only at a re-fit), so the pattern stays with the
// world when the tier's window re-snaps; it changes where k does (a footprint crossing a power of two). The 4096-cell
// period keeps the noise's float arithmetic exact.
#define KH_PCSS_CELL_KMAX 8.0f
float2 KhPcssRot(float2 khpr_t, float2 khpr_fw, float2 khpr_lat)
{
    const float khpr_f = max(min(khpr_fw.x, khpr_fw.y), 1.0e-6f);   // NaN / 0 take the finest cell.
    const float khpr_k = clamp(ceil(-log2(khpr_f)), 0.0f, KH_PCSS_CELL_KMAX);
    const float2 khpr_i = floor(khpr_t);
    const uint2 khpr_c = ((uint2(int2(khpr_i)) + asuint(khpr_lat)) << (uint)khpr_k)
                       + uint2(floor((khpr_t - khpr_i) * exp2(khpr_k)));
    const float2 khpr_g = float2(khpr_c & 4095u);
    float khpr_a = 6.2831853f * frac(52.9829189f * frac(dot(khpr_g, float2(0.06711056f, 0.00583715f))));
    return float2(cos(khpr_a), sin(khpr_a));
}
float2 KhPcssTap(int khpt_i, float2 khpt_rot)
{
    float2 khpt_d = KH_PCSS_DISC[khpt_i];
    return float2(khpt_d.x * khpt_rot.x - khpt_d.y * khpt_rot.y,
                  khpt_d.x * khpt_rot.y + khpt_d.y * khpt_rot.x);
}
// Penumbra radius in tier texels: half the sun-disc spread over the occluder-to-receiver distance (depth units /
// khpz_iD = metres; / khpz_tw = texels). With khpz_zb = 0 it is the widest penumbra the window allows, the blocker
// search radius.
float KhPcssRadius(float khpz_z, float khpz_zb, float khpz_tw, float khpz_iD)
{
    return 0.5f * KH_SUN_DIAM_TAN * (khpz_z - khpz_zb) / max(khpz_iD, 1.0e-9f) / max(khpz_tw, 1.0e-9f);
}
// The blocker search: the mean depth of the taps in front of the (biased) receiver within khsb_r texels, centre
// included. -1 = nothing between the light and the point.
float KhSunBlockerZ(Texture2D<float> khsb_m, float khsb_sz, float2 khsb_uv, float khsb_z, float khsb_r, float2 khsb_rot)
{
    int2 khsb_mx = int2((int)khsb_sz - 1, (int)khsb_sz - 1);
    float2 khsb_t = khsb_uv * khsb_sz;
    float khsb_acc = 0.0f;
    float khsb_n = 0.0f;
    float khsb_c = khsb_m.Load(int3(clamp(int2(khsb_t), int2(0, 0), khsb_mx), 0));
    if (khsb_c < khsb_z) { khsb_acc += khsb_c; khsb_n += 1.0f; }
    [unroll] for (int khsb_i = 0; khsb_i < 16; ++khsb_i) {
        int2 khsb_q = clamp(int2(khsb_t + KhPcssTap(khsb_i, khsb_rot) * khsb_r), int2(0, 0), khsb_mx);
        float khsb_s = khsb_m.Load(int3(khsb_q, 0));
        if (khsb_s < khsb_z) { khsb_acc += khsb_s; khsb_n += 1.0f; }
    }
    return khsb_n > 0.5f ? khsb_acc / khsb_n : -1.0f;
}
// The cast chain's compare: blocker search, then the contact ring unchanged where the penumbra fits it, else the
// disc of bilinear compares at the penumbra radius. khpw_z is the biased receiver depth; khpw_sp the ring spread
// (the contact threshold); khpw_tw / khpw_iD the tier's texel (m) and depth units per metre; khpw_fw / khpw_lat the
// receiver's texel footprint and the map's lattice offset (KhPcssRot).
float KhSunPcssWT(Texture2D<float> khpw_m, float khpw_sz, float2 khpw_uv, float khpw_z, float khpw_sp,
                  float khpw_tw, float khpw_iD, float2 khpw_fw, float2 khpw_lat)
{
    // The widest penumbra this window allows at all: at or under the ring spread no blocker can take the kernel
    // past the ring, so the ring answers and the search is skipped (a search that found nothing would answer
    // 'lit'; the ring's compares over the same texels are the contact verdict).
    float khpw_rw = KhPcssRadius(khpw_z, 0.0f, khpw_tw, khpw_iD);
    if (khpw_rw <= khpw_sp) return KhSunSoftWT(khpw_m, khpw_sz, khpw_uv, khpw_z, khpw_sp);
    float2 khpw_rot = KhPcssRot(khpw_uv * khpw_sz, khpw_fw, khpw_lat);
    float khpw_rs = min(khpw_rw, KH_PCSS_RMAX);
    float khpw_zb = KhSunBlockerZ(khpw_m, khpw_sz, khpw_uv, khpw_z, khpw_rs, khpw_rot);
    if (khpw_zb < 0.0f) return 0.0f;
    float khpw_r = KhPcssRadius(khpw_z, khpw_zb, khpw_tw, khpw_iD);
    if (khpw_r <= khpw_sp) return KhSunSoftWT(khpw_m, khpw_sz, khpw_uv, khpw_z, khpw_sp);   // Contact: as before.
    khpw_r = min(khpw_r, KH_PCSS_RMAX) / khpw_sz;
    float khpw_acc = 0.0f;
    [unroll] for (int khpw_i = 0; khpw_i < 16; ++khpw_i) {
        khpw_acc += KhSunBilinT(khpw_m, khpw_sz, khpw_uv + KhPcssTap(khpw_i, khpw_rot) * khpw_r, khpw_z);
    }
    return khpw_acc * (1.0f / 16.0f);
}

// The tier-blend weight over the outer window edge. One curve for every blend site (the cast chain's KhCastTier, the
// self kernel's KhSelfTier).
float KhTbW(float khtw_e)
{
    return 1.0f - smoothstep(0.75f, 0.98f, khtw_e);
}
// KH_TIER_BLEND: a finer tier that carried a partial verdict blends with the coarser tier resolving it by the finer
// tier's edge weight.
float KhTbBlend(float khtd_c, float khtd_f, float khtd_w)
{
    return lerp(khtd_c, khtd_f, khtd_w);
}

// KH_SUN_GRAD: every gradient the two sun chains read, taken by the pixel shader in uniform control flow and handed
// down. The chains branch per pixel (a tier's window test, the carry's early returns, the mesh shaders' N.L gate) and
// a gradient taken past such a branch reads lanes of its quad that left it (undefined in D3D11). Each value is the
// expression the chain would take in place, evaluated before any such branch. A lane whose map is off (a CB flag:
// uniform) is 0. The cast chain's (SunShadowOcclusion): the footprint of each camera tier's texel coordinate, then the
// union's (KhCastTier's khC_fw, the union's rotation).
struct KhSunCastGrad {
    float2 fw2;   // Hero.
    float2 fw3;   // Mid.
    float2 fw4;   // Outer.
    float2 fw5;   // Far.
    float2 fwu;   // The union.
};
float2 KhSunCastFw(float3 khsf_r, float4x4 khsf_vp, float4 khsf_meta)
{
    if (khsf_meta.x < 0.5f) return float2(0.0f, 0.0f);
    float4 khsf_c = mul(float4(khsf_r, 1.0f), khsf_vp);
    float2 khsf_u = float2(0.5f + 0.5f * khsf_c.x, 0.5f - 0.5f * khsf_c.y);
    return fwidth(khsf_u * khsf_meta.y);
}
KhSunCastGrad KhSunCastGradAt(float3 wpos)
{
    KhSunCastGrad khsc_g;
    const float3 khsc_r = wpos - sunOrigin.xyz;
    khsc_g.fw2 = KhSunCastFw(khsc_r, sunVP2, sunMeta2);
    khsc_g.fw3 = KhSunCastFw(khsc_r, sunVP3, sunMeta3);
    khsc_g.fw4 = KhSunCastFw(khsc_r, sunVP4, sunMeta4);
    khsc_g.fw5 = KhSunCastFw(khsc_r, sunVP5, sunMeta5);
    khsc_g.fwu = KhSunCastFw(khsc_r, sunVP, sunMeta);
    return khsc_g;
}
// The self chain's (SunShadowOcclusionSelf): length(fwidth(n)) and the receiver's half footprint (khgs), then each
// tier's and the union's texel footprint at the normal-offset point the chain samples (KhSelfTier's khT_fw, the
// union's khsr_fw). Zero where the ladder is never consulted.
struct KhSunSelfGrad {
    float  fwn;   // length(fwidth(n)).
    float  gs;    // 0.5 * length(fwidth(wrel)): the chain's khgs.
    float2 fw2;   // Hero.
    float2 fw3;   // Mid.
    float2 fw4;   // Outer.
    float2 fw5;   // Far.
    float2 fwu;   // The union.
};
float2 KhSunSelfFw(float3 khsg_wr, float3 khsg_n, float khsg_k, float4x4 khsg_vp, float4 khsg_meta,
                   float khsg_iR0)   // KH_CB_DERIVED: this map's sunCol texel scale.
{
    if (khsg_meta.x < 0.5f) return float2(0.0f, 0.0f);
    float khsg_no = khsg_k * 2.0f / (max(khsg_meta.y, 1.0f) * max(khsg_iR0, 1e-6f));
    float4 khsg_c = mul(float4(khsg_wr + khsg_n * khsg_no, 1.0f), khsg_vp);
    float2 khsg_uv = float2(0.5f + 0.5f * khsg_c.x, 0.5f - 0.5f * khsg_c.y);
    return fwidth(khsg_uv * khsg_meta.y);
}
KhSunSelfGrad KhSunSelfGradAt(float3 wrel, float3 nrm)
{
    KhSunSelfGrad khss_g = (KhSunSelfGrad)0;
    // The ladder's uniform exits: no map (SunShadowOcclusionSelf) and zero strength (SunShadowFactorSelf,
    // KH_SHADOW_STRENGTH_SKIP), so both stay free.
    if (sunMeta.x < 0.5f || saturate(sunMeta.w) <= 0.0f) return khss_g;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    khss_g.fwn = length(fwidth(n));
    khss_g.gs = 0.5f * length(fwidth(wrel));
    float khno_k = min(1.0f + 2.0f * sqrt(saturate(1.0f - ndl * ndl))
                              / max(ndl, 0.15f), 2.0f)
                 * saturate(1.0f - 5.0f * khss_g.fwn);   // SunShadowOcclusionSelf's.
    khss_g.fw2 = KhSunSelfFw(wrel, n, khno_k, sunVP2, sunMeta2, sunCol[0].z);
    khss_g.fw3 = KhSunSelfFw(wrel, n, khno_k, sunVP3, sunMeta3, sunCol[1].x);
    khss_g.fw4 = KhSunSelfFw(wrel, n, khno_k, sunVP4, sunMeta4, sunCol[1].z);
    khss_g.fw5 = KhSunSelfFw(wrel, n, khno_k, sunVP5, sunMeta5, sunCol[2].x);
    khss_g.fwu = KhSunSelfFw(wrel, n, khno_k, sunVP, sunMeta, sunCol[0].x);
    return khss_g;
}

// One cast tier for the four camera-anchored bands (hero/mid/outer/far). false = fall through with the carry state
// updated in place. TWIN: KhSelfTier's ladder.
float KhCastTier(Texture2D<float> khC_map, float4x4 khC_vp, float4 khC_meta, float3 khC_r,
                 float khC_cb,   // This tier's cast bias; 0 = meta.z.
                 float2 khC_lat,   // KH_PCSS_CELL: this tier's lattice offset (sunLat*).
                 float2 khC_fw,    // KH_SUN_GRAD: fwidth of this tier's texel coordinate.
                 float2 khC_len,   // KH_CB_DERIVED: this tier's sunCol pair (texel scale, depth scale).
                 bool khC_last,   // Last camera tier - no edge carry.
                 inout float khtb_occ, inout float khtb_w, out bool khC_done)
{
    khC_done = false;
    if (khC_meta.x >= 0.5f) {
        float4 khC_c = mul(float4(khC_r, 1.0f), khC_vp);
        float2 khC_u = float2(0.5f + 0.5f * khC_c.x, 0.5f - 0.5f * khC_c.y);
        // The receiver footprint in this tier's texels (KH_SUN_GRAD: khC_fw).
        float  khC_sp = clamp(0.5f * max(khC_fw.x, khC_fw.y), 1.0f, 4.0f);
        if (khC_u.x > 0.002f && khC_u.x < 0.998f &&
            khC_u.y > 0.002f && khC_u.y < 0.998f &&
            khC_c.z > 0.0f && khC_c.z < 1.0f) {
            // khC_meta.z is texel-priced (1 mm .. 100 mm across the ladder) and on the cast chain this line is the
            // whole bias, so the far tier would push every world shadow bias / tan(elevation) sun-ward; the capped
            // khC_cb stops that.
            float khC_b = khC_cb > 0.0f ? khC_cb : khC_meta.z;
            // KH_PCSS: the tier's texel (m) and depth units per metre, as KhSelfTier prices them.
            float khC_iR = khC_len.x;
            float khC_iD = khC_len.y;
            float khC_tw = 2.0f / (max(khC_meta.y, 1.0f) * max(khC_iR, 1e-6f));
            float khC_o = KhSunPcssWT(khC_map, khC_meta.y, khC_u, khC_c.z - khC_b, khC_sp, khC_tw, khC_iD,
                                      khC_fw, khC_lat);
            // Lit authoritative: the tier map is complete.
            if (khtb_occ >= 0.0f) { khC_done = true; return KhTbBlend(khC_o, khtb_occ, khtb_w); }
            if (khC_last) { khC_done = true; return khC_o; }   // The window holds the whole domain.
            float khC_e = max(abs(khC_u.x - 0.5f), abs(khC_u.y - 0.5f)) * 2.0f;
            float khC_w = KhTbW(khC_e);
            if (khC_w >= 0.9999f) { khC_done = true; return khC_o; }
            khtb_occ = khC_o; khtb_w = khC_w;   // Carry into the next tier.
        }
    }
    return 0.0f;
}

float SunShadowOcclusion(float3 wpos, KhSunCastGrad khsc_g)   // KH_SUN_GRAD: KhSunCastGradAt(wpos).
{
    if (sunMeta.x < 0.5f) return 0.0f;
    // TWIN: the self chain (SunShadowOcclusionSelf) carries the identical shape.
    float khtb_occ = -1.0f;
    float khtb_w = 0.0f;
    // The admission test (KhCastTier's window: uv inside 0.002 .. 0.998, depth inside (0, 1)) is a hard threshold on
    // position: one ulp outside it the band is skipped entirely (no return, no carry, no weight) and the next tier
    // answers at full strength.
    {
        const float3 khc_r = wpos - sunOrigin.xyz;
        bool khc_done;
        float khc_v;
        khc_v = KhCastTier(khSunDepth2, sunVP2, sunMeta2, khc_r, sunCastBias.x, sunLat0.xy, khsc_g.fw2, sunCol[0].zw, false,
                           khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth3, sunVP3, sunMeta3, khc_r, sunCastBias.y, sunLat0.zw, khsc_g.fw3, sunCol[1].xy, false,
                           khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth4, sunVP4, sunMeta4, khc_r, sunCastBias.z, sunLat1.xy, khsc_g.fw4, sunCol[1].zw, false,
                           khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth5, sunVP5, sunMeta5, khc_r, sunCastBias.w, sunLat1.zw, khsc_g.fw5, sunCol[2].xy, true,
                           khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
    }
    float4 c = mul(float4(wpos - sunOrigin.xyz, 1.0f), sunVP);   // Ortho: w = 1 (sunVP is relative to sunOrigin).
    float2 uv = float2(0.5f + 0.5f * c.x, 0.5f - 0.5f * c.y);
    // KH_TIER_BLEND: a carried band verdict resolves against whatever the union
    // answers - including its rejections (0 = lit).
    if (uv.x <= 0.001f || uv.x >= 0.999f || uv.y <= 0.001f || uv.y >= 0.999f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    if (c.z <= 0.0f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;

    const float khcu_b = sunCastBias2.x > 0.0f ? sunCastBias2.x : sunMeta.z;

    // Past the map's far plane: the far compare (filtered), which a carried band verdict resolves too.
    if (c.z >= 1.0f) {
        float khcu_far = 0.0f;   // KH_PCSS_SKIP_BRANCH's rule: a branch, not ?: (fxc runs both arms).
        [branch] if (localityMeta.z >= 0.5f) khcu_far = SunShadowCompareSoft(uv, 1.0f - khcu_b);
        return (khtb_occ >= 0.0f) ? KhTbBlend(khcu_far, khtb_occ, khtb_w) : khcu_far;
    }

    // KH_TIER_BLEND: the union answer resolves any carried band edge.
    // KH_PCSS on the union too (the 0.75-texel diamond is its contact ring).
    float khcu_iR = sunCol[0].x;   // KH_CB_DERIVED.
    float khcu_iD = sunCol[0].y;
    float khcu_tw = 2.0f / (max(sunMeta.y, 1.0f) * max(khcu_iR, 1e-6f));
    float khtb_un;
    {
        float2 khcu_rot = KhPcssRot(uv * sunMeta.y, khsc_g.fwu, float2(0.0f, 0.0f));   // KH_PCSS_CELL, KH_SUN_GRAD.
        float khcu_zr = c.z - khcu_b;
        float khcu_rw = KhPcssRadius(khcu_zr, 0.0f, khcu_tw, khcu_iD);   // KhSunPcssWT's skip: the ring spread is 1 here.
        float khcu_rs = min(max(khcu_rw, 1.0f), KH_PCSS_RMAX);
        // KH_PCSS_SKIP_BRANCH: a branch, not ?: - fxc evaluates both arms of a ternary, so the skip ran the
        // search (17 Loads) on every pixel. Load-only, so no gradient is taken inside; the value is the ternary's.
        float khcu_zb = 0.0f;
        [branch] if (!(khcu_rw <= 1.0f))
            khcu_zb = KhSunBlockerZ(khSunDepth, sunMeta.y, uv, khcu_zr, khcu_rs, khcu_rot);
        float khcu_r = khcu_zb < 0.0f ? 0.0f : KhPcssRadius(khcu_zr, khcu_zb, khcu_tw, khcu_iD);
        if (khcu_zb < 0.0f) {
            khtb_un = 0.0f;
        } else if (khcu_r <= 1.0f) {
            khtb_un = SunShadowCompareSoft(uv, khcu_zr);   // Contact: as before.
        } else {
            khcu_r = min(khcu_r, KH_PCSS_RMAX) / sunMeta.y;
            khtb_un = 0.0f;
            [unroll] for (int khcu_i = 0; khcu_i < 16; ++khcu_i) {
                khtb_un += KhSunBilinT(khSunDepth, sunMeta.y, uv + KhPcssTap(khcu_i, khcu_rot) * khcu_r, khcu_zr);
            }
            khtb_un *= 1.0f / 16.0f;
        }
    }
    return (khtb_occ >= 0.0f) ? KhTbBlend(khtb_un, khtb_occ, khtb_w) : khtb_un;
}

// One soft-compare tap for the self term: the 2 x 2 block's compares, each against the receiver plane extrapolated
// to its texel, bilinearly weighted (KhSelfTier's 3 x 3 ring and KhSelfPcssT's disc are built from it). One tap body
// for all five maps (fxc resolves a resource parameter at inlining). The texel is kept to the map on BOTH sides
// (khst_sz = the map edge in texels): a footprint ring past an edge reads the edge texel, never an out-of-range 0
// (= the nearest depth = occluded) - KhDlsBilin's rule. The fetch has it from the Gather sampler's CLAMP addressing;
// the index clamp below puts each tap's depth gradient (khst_d) at the texel the fetch returned.
float KhSelfTapT(Texture2D<float> khst_m, float khst_sz, float2 khst_t, float2 khst_g, float khst_z, float khst_b, float khst_w, float2 khst_o)
{
    float2 khst_tc = khst_t + khst_o - 0.5f;   // Fractional offsets land on the corners.
    float2 khst_f0 = floor(khst_tc);
    float2 khst_fr = khst_tc - khst_f0;
    int2   khst_p0 = int2(khst_f0);
    int2   khst_mx = int2((int)khst_sz - 1, (int)khst_sz - 1);
    // KH_SHADOW_GATHER: the block's four texels in one fetch, reordered to this loop's tap order (0, 0) (1, 0)
    // (0, 1) (1, 1).
    const float4 khst_gv = khst_m.Gather(khPfSamp, (khst_f0 + 1.0f) / khst_sz);
    const float4 khst_gt = float4(khst_gv.w, khst_gv.z, khst_gv.x, khst_gv.y);
    float4 khst_c;
    [unroll] for (int khst_k = 0; khst_k < 4; ++khst_k) {
        int2 khst_q = clamp(khst_p0 + int2(khst_k & 1, khst_k >> 1), int2(0, 0), khst_mx);
        float2 khst_d = (float2(khst_q) + 0.5f) - khst_t;
        float khst_e = khst_z + khst_d.x * khst_g.x + khst_d.y * khst_g.y - khst_b;
        float khst_s = khst_gt[khst_k];
        khst_c[khst_k] = saturate((khst_e - khst_s) / max(khst_w, 1.0e-9f) + 0.5f);
    }
    return lerp(lerp(khst_c.x, khst_c.y, khst_fr.x),
                lerp(khst_c.z, khst_c.w, khst_fr.x), khst_fr.y);
}

// KH_PCSS, the self chain's blocker search: point loads compared against the receiver plane extrapolated to each tap
// (KhSelfTapT's rule), centre included. -1 = nothing between the light and the point. A receiver's own curved surface
// reads as a blocker at a tiny distance, so a small penumbra results and the contact ring answers; the wide kernel is
// reached only by genuinely distant casters.
float KhSelfBlockerZ(Texture2D<float> khbz_m, float khbz_sz, float2 khbz_t, float2 khbz_g, float khbz_z, float khbz_b, float khbz_r, float2 khbz_rot)
{
    int2 khbz_mx = int2((int)khbz_sz - 1, (int)khbz_sz - 1);
    float khbz_acc = 0.0f;
    float khbz_n = 0.0f;
    [unroll] for (int khbz_i = 0; khbz_i < 17; ++khbz_i) {
        float2 khbz_o = KhPcssTap(min(khbz_i, 15), khbz_rot) * khbz_r * (khbz_i < 16 ? 1.0f : 0.0f);   // Tap 16 = the centre.
        int2 khbz_q = clamp(int2(khbz_t + khbz_o), int2(0, 0), khbz_mx);
        float2 khbz_d = (float2(khbz_q) + 0.5f) - khbz_t;
        float khbz_e = khbz_z + khbz_d.x * khbz_g.x + khbz_d.y * khbz_g.y - khbz_b;
        float khbz_s = khbz_m.Load(int3(khbz_q, 0));
        if (khbz_s < khbz_e) { khbz_acc += khbz_s; khbz_n += 1.0f; }
    }
    return khbz_n > 0.5f ? khbz_acc / khbz_n : -1.0f;
}
// The self chain's wide kernel: the disc of soft compares at khsp_r texels, the slope bias grown by the radius (the
// textbook bias-with-filter-size, so a receiver tilted to the light does not shadow itself across the disc).
float KhSelfPcssT(Texture2D<float> khsp_m, float khsp_sz, float2 khsp_t, float2 khsp_g, float khsp_z, float khsp_b, float khsp_bsl, float khsp_w, float khsp_r, float2 khsp_rot)
{
    float khsp_bb = khsp_b + khsp_bsl * khsp_r;
    float khsp_acc = 0.0f;
    [unroll] for (int khsp_i = 0; khsp_i < 16; ++khsp_i) {
        khsp_acc += KhSelfTapT(khsp_m, khsp_sz, khsp_t, khsp_g, khsp_z, khsp_bb, khsp_w, KhPcssTap(khsp_i, khsp_rot) * khsp_r);
    }
    return khsp_acc * (1.0f / 16.0f);
}

// One cascade of the self ladder: receiver-plane gradient (damped by fwidth(n)), normal-offset sampling, a
// tier-proportional bias floor plus the hero-priced slope term, the KH_PCSS blocker search, then either the wide
// penumbra disc or the contact path (a 3x3 footprint-spread pcf ring with the prefilter mean / variance blend where
// the footprint exceeds a texel), and the tier-blend carry at the window edge. The window test IS the selection: a
// point inside this tier's window takes this tier's verdict, lit included - a lit verdict let fall through to a
// coarser tier paints the coarse tier's fatter shadow rim outside the fine tier's correct one.
float KhSelfTier(Texture2D<float> khT_map, Texture2D<float2> khT_pf, float4x4 khT_vp, float4 khT_meta,
                 float khT_pfArm,
                 float2 khT_lat,   // KH_PCSS_CELL: this tier's lattice offset (sunLat*).
                 float2 khT_fw,    // KH_SUN_GRAD: fwidth of this tier's texel coordinate.
                 float khT_fwn,    // KH_SUN_GRAD: length(fwidth(n)).
                 float2 khT_len,   // KH_CB_DERIVED: this tier's sunCol pair (texel scale, depth scale).
                 bool khT_last,   // Last camera tier - no edge carry.
                 float3 khwr, float3 n, float ndl, float khno_k, float khgs,
                 inout float khtb_occ, inout float khtb_w, out bool khT_done)
{
    khT_done = false;
    if (khT_meta.x >= 0.5f) {
        float khT_iR0 = khT_len.x;
        float khT_no = khno_k * 2.0f / (max(khT_meta.y, 1.0f) * max(khT_iR0, 1e-6f));
        float4 khT_c = mul(float4(khwr + n * khT_no, 1.0f), khT_vp);
        float2 khT_uv = float2(0.5f + 0.5f * khT_c.x, 0.5f - 0.5f * khT_c.y);

        if (khT_uv.x > 0.002f && khT_uv.x < 0.998f &&
            khT_uv.y > 0.002f && khT_uv.y < 0.998f &&
            khT_c.z > 0.0f && khT_c.z < 1.0f) {
            float3 khT_cr = float3(khT_vp[0].x, khT_vp[1].x, khT_vp[2].x);
            float3 khT_cu = float3(khT_vp[0].y, khT_vp[1].y, khT_vp[2].y);
            float khT_iR = khT_len.x;   // length(khT_cr), from the lane.
            float khT_iD = khT_len.y;
            float khT_tw = 2.0f / (max(khT_meta.y, 1.0f) * max(khT_iR, 1e-6f));
            float khT_k = khT_tw * khT_iD / max(ndl, 0.02f)
                        * saturate(1.0f - 3.0f * khT_fwn);   // Damped gradient (KH_SUN_GRAD).
            float khT_gc = KH_RPDB_GC(khT_tw) * khT_iD;
            float2 khT_g = clamp(
                float2( dot(n, khT_cr / max(khT_iR, 1e-9f)) * khT_k,
                       -dot(n, khT_cu / max(khT_iR, 1e-9f)) * khT_k),
                -khT_gc, khT_gc);
            float2 khT_t = khT_uv * khT_meta.y;
            float khT_tan = clamp(sqrt(saturate(1.0f - ndl * ndl)) / max(ndl, 0.02f), 1.0f, 1.0e4f);
            float khT_b = max(khT_meta.z, 1.5f * khT_tw * khT_iD)   // Tier-proportional floor.
                        + 0.35f * khT_tan * KH_SLOPE_TW(khT_tw) * khT_iD;
            float khT_gs = min(khgs, 4.0f * khT_tw);
            khT_b += khT_gs * khT_iD;
            // khT_fw: fwidth(khT_t), taken in uniform control flow (KH_SUN_GRAD), read by both paths.
            float khT_sp = (khgs > 4.0f * khT_tw)
                         ? 1.0f
                         : clamp(0.5f * max(khT_fw.x, khT_fw.y), 1.0f, 8.0f);
            float khT_sw = max(2.0f * khT_gs, khT_tw) * khT_iD;   // Clamped slack.
            // KH_PCSS: the blocker search over the widest penumbra this tier's window allows, then the contact ring
            // unchanged where the penumbra fits it, else the disc at the penumbra radius (never narrower than the
            // receiver footprint, the minification the prefilter blend serves on the ring path).
            float2 khT_rot = KhPcssRot(khT_t, khT_fw, khT_lat);
            // KhSunPcssWT's skip: a window whose widest penumbra fits the ring spread takes the ring without the
            // search (khT_zb = the receiver's own depth, 'a blocker at no distance': khT_pr = 0, the ring path).
            float khT_rw = KhPcssRadius(khT_c.z, 0.0f, khT_tw, khT_iD);
            float khT_rs = min(max(khT_rw, khT_sp), KH_PCSS_RMAX);
            float khT_zb = khT_c.z;   // KH_PCSS_SKIP_BRANCH (the cast chain's note).
            [branch] if (!(khT_rw <= khT_sp))
                khT_zb = KhSelfBlockerZ(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_rs, khT_rot);
            float khT_pr = khT_zb < 0.0f ? 0.0f : KhPcssRadius(khT_c.z, khT_zb, khT_tw, khT_iD);
            float khT_res;
            [branch] if (khT_zb < 0.0f) {
                khT_res = 0.0f;   // Nothing between the light and the point.
            } else if (khT_pr > khT_sp) {
                float khT_bsl = 0.35f * khT_tan * KH_SLOPE_TW(khT_tw) * khT_iD;
                float khT_kr = min(max(khT_pr, 0.5f * max(khT_fw.x, khT_fw.y)), KH_PCSS_RMAX);
                khT_res = KhSelfPcssT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_bsl, khT_sw, khT_kr, khT_rot);
            } else {
                // The centre and the four edge neighbours first; where those five agree the diagonals cannot move the
                // nine-tap mean, so they are skipped. The skip is exact while the taps overlap (the cross reaches sp
                // texels out, the diagonals sp * sqrt(2)): past sp = 1 a diagonal edge can leave every cross tap lit
                // while a corner is shadowed. The gate is sp <= 1.5 all the same - that miss is accepted up to there
                // (sp is 1 where the world footprint passes 4 texels - grazing or distant receivers; footprints up to 3
                // texels keep the early-out); wider spreads take all nine taps.
                float khT_ctr = KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0,  0));
                float khT_cr4 = KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0, -1) * khT_sp)
                              + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1,  0) * khT_sp)
                              + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1,  0) * khT_sp)
                              + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0,  1) * khT_sp);
                float khT_un = khT_ctr + khT_cr4;
                [branch] if (khT_sp <= 1.5f &&
                             (khT_un >= 4.9995f || khT_un <= 0.0005f)) {
                    khT_res = khT_ctr;   // Unanimous: the mean IS the centre.
                } else {
                    float khT_rng = khT_cr4
                                  + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1, -1) * khT_sp)
                                  + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1, -1) * khT_sp)
                                  + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1,  1) * khT_sp)
                                  + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1,  1) * khT_sp);
                    khT_res = (khT_ctr + khT_rng) / 9.0f;
                }
                if (khT_pfArm >= 0.5f) {
                    float khT_ft = max(khT_fw.x, khT_fw.y);
                    float khT_pw = (khgs > 4.0f * khT_tw)
                                 ? 0.0f : smoothstep(1.0f, 2.0f, khT_ft);
                    if (khT_pw > 0.001f) {
                        float khT_lod = log2(max(khT_ft * 0.5f, 1.0f));
                        float2 khT_mv = KhPfMu(khT_pf, khT_uv, khT_meta.y * 0.5f, khT_lod);
                        float khT_vd = (khT_c.z - khT_b) - khT_mv.x;
                        float khT_ww = max(khT_ft, 2.0f) * khT_tw * khT_iD;
                        float khT_var = khT_mv.y - khT_mv.x * khT_mv.x;
                        float khT_s2 = (khT_var >= 2.5e-7f ? max(khT_var, khT_ww * khT_ww)
                                                           : khT_ww * khT_ww);   // Ramp.
                        float khT_vv = khT_vd <= 0.0f ? 1.0f
                                     : saturate(khT_s2 / (khT_s2 + khT_vd * khT_vd));
                        float khT_pfo = 1.0f - saturate((khT_vv - 0.4f) / 0.6f);
                        // khT_ga is the 'both read lit, do not invent shadow' floor, as a weight (a hard test steps
                        // the blend across an iso-contour - a halo tracking the penumbra).
                        float khT_ga = max(smoothstep(0.0f, 0.002f, khT_res),
                                           smoothstep(0.10f, 0.30f, khT_pfo));
                        // The prefilter may not brighten the taps beyond their own uncertainty: Chebyshev's p_max is
                        // an UPPER bound on the lit fraction, tight for a caster edge against open sky, loose where
                        // two casters at different depths share the footprint (their depth spread is variance with
                        // no lit texel behind it - VSM light bleed, a bright line along every shadow-meets-shadow
                        // boundary). Unanimous taps take nothing brighter; disagreeing ones keep the weight
                        // (4 r (1 - r), continuous). Darkening is untouched (a thin caster the taps straddle). Past
                        // the spread clamp the bound relaxes.
                        float khT_wt = khT_pw * khT_ga;
                        if (khT_pfo < khT_res) {
                            float khT_unc = max(4.0f * khT_res * (1.0f - khT_res),
                                                smoothstep(16.0f, 32.0f, khT_ft));
                            khT_wt *= khT_unc;
                        }
                        khT_res = lerp(khT_res, khT_pfo, khT_wt);
                    }
                }
            }   // End of the contact-ring path.
            // Unconditional: the point is inside this tier's window, so this tier's verdict stands whatever it is.
            // Only the window EDGE consults the next tier, through the same carry the cast chain uses.
            if (khtb_occ >= 0.0f) { khT_done = true; return KhTbBlend(khT_res, khtb_occ, khtb_w); }
            if (khT_last) { khT_done = true; return khT_res; }   // The window holds the whole domain.
            float khT_e = max(abs(khT_uv.x - 0.5f), abs(khT_uv.y - 0.5f)) * 2.0f;
            float khT_bw = KhTbW(khT_e);
            if (khT_bw >= 0.9999f) { khT_done = true; return khT_res; }
            khtb_occ = khT_res; khtb_w = khT_bw;   // Edge band: blend with the coarser tier.
        }
    }
    return 0.0f;
}

// The self term: hero, mid, outer, far tiers, then the union map. TWIN: SunShadowOcclusion (the cast chain),
// identical carry shape.
float SunShadowOcclusionSelf(float3 wrel, float3 nrm, KhSunSelfGrad khsg)   // KH_SUN_GRAD: KhSunSelfGradAt.
{
    if (sunMeta.x < 0.5f) return 0.0f;
    float3 khwr = wrel;
    float khgs = khsg.gs;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float khno_k = min(1.0f + 2.0f * sqrt(saturate(1.0f - ndl * ndl))
                              / max(ndl, 0.15f), 2.0f)
                 * saturate(1.0f - 5.0f * khsg.fwn);
    float khtb_occ = -1.0f;
    float khtb_w = 0.0f;
    bool  khT_done = false;
    float khT_v;
    khT_v = KhSelfTier(khSunDepth2, khSunPf2, sunVP2, sunMeta2, sunPf.x, sunLat0.xy, khsg.fw2, khsg.fwn, sunCol[0].zw, false,
                       khwr, n, ndl, khno_k, khgs,
                       khtb_occ, khtb_w, khT_done);
    if (khT_done) return khT_v;
    khT_v = KhSelfTier(khSunDepth3, khSunPf3, sunVP3, sunMeta3, sunPf.y, sunLat0.zw, khsg.fw3, khsg.fwn, sunCol[1].xy, false,
                       khwr, n, ndl, khno_k, khgs,
                       khtb_occ, khtb_w, khT_done);
    if (khT_done) return khT_v;
    khT_v = KhSelfTier(khSunDepth4, khSunPf4, sunVP4, sunMeta4, sunPf.z, sunLat1.xy, khsg.fw4, khsg.fwn, sunCol[1].zw, false,
                       khwr, n, ndl, khno_k, khgs,
                       khtb_occ, khtb_w, khT_done);
    if (khT_done) return khT_v;
    if (sunMeta5.x >= 0.5f) {
        khT_v = KhSelfTier(khSunDepth5, khSunPf5, sunVP5, sunMeta5, sunOrigin.w, sunLat1.zw, khsg.fw5, khsg.fwn, sunCol[2].xy,
                           true,   // Last tier; the far tier's prefilter arm (KH_FAR_PF) is sunOrigin.w, above.
                           khwr, n, ndl, khno_k, khgs,
                           khtb_occ, khtb_w, khT_done);
        if (khT_done) return khT_v;
    }
    float khsr_iR0 = sunCol[0].x;   // KH_CB_DERIVED.
    float khsr_no = khno_k * 2.0f / (max(sunMeta.y, 1.0f) * max(khsr_iR0, 1e-6f));
    float4 khsr_c = mul(float4(khwr + n * khsr_no, 1.0f), sunVP);   // Ortho: w = 1.
    float2 khsr_uv = float2(0.5f + 0.5f * khsr_c.x, 0.5f - 0.5f * khsr_c.y);
    if (khsr_uv.x <= 0.002f || khsr_uv.x >= 0.998f ||
        khsr_uv.y <= 0.002f || khsr_uv.y >= 0.998f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    if (khsr_c.z <= 0.0f || khsr_c.z >= 1.0f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    float3 khsr_cr = float3(sunVP[0].x, sunVP[1].x, sunVP[2].x);
    float3 khsr_cu = float3(sunVP[0].y, sunVP[1].y, sunVP[2].y);
    float khsr_iR = sunCol[0].x;   // length(khsr_cr), from the lane.
    float khsr_iD = sunCol[0].y;
    float khsr_tw = 2.0f / (max(sunMeta.y, 1.0f) * max(khsr_iR, 1e-6f));
    float khsr_k = khsr_tw * khsr_iD / max(ndl, 0.02f)
                 * saturate(1.0f - 3.0f * khsg.fwn);
    float khsr_gc = KH_RPDB_GC(khsr_tw) * khsr_iD;   // KH_RPDB_WORLD_CLAMP. TWIN: KhSelfTier's khT_gc.
    float2 khsr_g = clamp(
        float2( dot(n, khsr_cr / max(khsr_iR, 1e-9f)) * khsr_k,
               -dot(n, khsr_cu / max(khsr_iR, 1e-9f)) * khsr_k),
        -khsr_gc, khsr_gc);
    float2 khsr_t = khsr_uv * sunMeta.y;
    float khsr_tan = clamp(sqrt(saturate(1.0f - ndl * ndl)) / max(ndl, 0.02f),
                           1.0f, 1.0e4f);
    float khsr_b = max(sunMeta.z, 1.5f * khsr_tw * khsr_iD)   // Tier-proportional floor.
                 + 0.35f * khsr_tan * KH_SLOPE_TW(khsr_tw) * khsr_iD;   // KH_SLOPE_WORLD. TWIN: KhSelfTier's khT_b.
    float khsr_gs = min(khgs, 4.0f * khsr_tw);
    khsr_b += khsr_gs * khsr_iD;
    float khsr_w = saturate(0.001f / max(khsr_tw, 1e-6f));
    float2 khsr_fw = khsg.fwu;   // KH_SUN_GRAD: fwidth(khsr_t), taken in uniform control flow.
    float khsr_sp = (khgs > 4.0f * khsr_tw)
                ? 1.0f
                : clamp(0.5f * max(khsr_fw.x, khsr_fw.y), 1.0f, 8.0f);
    float khsr_sw = max(2.0f * khsr_gs, khsr_tw) * khsr_iD;   // Clamped slack.
    float khsr_fd = 1.0f;
    if (sunMeta.x >= 1.5f) {
        float khsr_e = max(abs(khsr_uv.x - 0.5f), abs(khsr_uv.y - 0.5f)) * 2.0f;
        khsr_fd = 1.0f - smoothstep(0.85f, 0.98f, khsr_e);
    }
    // KH_PCSS on the union, the same shape as the tiers.
    float2 khsr_rot = KhPcssRot(khsr_t, khsr_fw, float2(0.0f, 0.0f));   // KH_PCSS_CELL.
    float khsr_rw = KhPcssRadius(khsr_c.z, 0.0f, khsr_tw, khsr_iD);   // KhSelfTier's skip.
    float khsr_rs = min(max(khsr_rw, khsr_sp), KH_PCSS_RMAX);
    float khsr_zb = khsr_c.z;   // KH_PCSS_SKIP_BRANCH (the cast chain's note).
    [branch] if (!(khsr_rw <= khsr_sp))
        khsr_zb = KhSelfBlockerZ(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_rs, khsr_rot);
    float khsr_pr = khsr_zb < 0.0f ? 0.0f : KhPcssRadius(khsr_c.z, khsr_zb, khsr_tw, khsr_iD);
    float khsr_res;
    [branch] if (khsr_zb < 0.0f) {
        khsr_res = 0.0f;
    } else if (khsr_pr > khsr_sp) {
        float khsr_bsl = 0.35f * khsr_tan * KH_SLOPE_TW(khsr_tw) * khsr_iD;
        float khsr_kr = min(max(khsr_pr, 0.5f * max(khsr_fw.x, khsr_fw.y)), KH_PCSS_RMAX);
        khsr_res = KhSelfPcssT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_bsl, khsr_sw, khsr_kr, khsr_rot) * khsr_fd;
    } else {
        float khsr_ctr = KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0,  0));
        float khsr_rng = KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1, -1) * khsr_sp)
                       + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0, -1) * khsr_sp)
                       + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1, -1) * khsr_sp)
                       + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1,  0) * khsr_sp)
                       + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1,  0) * khsr_sp)
                       + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1,  1) * khsr_sp)
                       + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0,  1) * khsr_sp)
                       + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1,  1) * khsr_sp);
        khsr_res = (khsr_ctr + khsr_w * khsr_rng)
                 * (1.0f / (1.0f + 8.0f * khsr_w)) * khsr_fd;
    }   // End of the contact-ring path.
    // The union is reached only when no tier window held the point; its verdict resolves any carried tier-edge
    // blend and stands.
    return (khtb_occ >= 0.0f) ? KhTbBlend(khsr_res, khtb_occ, khtb_w) : khsr_res;
}

float SunShadowFactorSelf(float3 wpos, float3 wrel, float3 nrm, KhSunSelfGrad khsg)
{
    // KH_SHADOW_STRENGTH_SKIP: sunMeta.w is g_shadow_map_strength, a CB scalar, so this branch is UNIFORM (the
    // ladder takes no gradient of its own: KH_SUN_GRAD, khsg). At strength 0 the product is an exact 1 - 0 whatever
    // the ladder answers, and the ladder is the costliest term in the mesh shaders (four tiers plus the union map,
    // each a KH_PCSS blocker search and up to 16 taps); an HLSL multiply evaluates both sides, so this gate is what
    // makes the zero free.
    if (saturate(sunMeta.w) <= 0.0f) return 1.0f;
    return 1.0f - SunShadowOcclusionSelf(wrel, nrm, khsg) * saturate(sunMeta.w)
                * KhSunRangeFade(wpos);
}

// Our own near clip: 0.05 m sits below the engine's floor of 0.07, so it is never the binding constraint.
static const float KH_OWN_NEAR = 0.05f;

// C++ twin KH_RP_STEN_FADE: the mirror replay runs only for a mesh that can reach this band (KH_MIR_BAND) - change
// both together.
static const float KH_STEN_FADE = 1.35f;

// KH_INFRONT: the view-model slice's projection has a near of 0.01 (measured), below our floor, so there the floor
// takes the engine's own near (the pass's depthParams pair) and stays non-binding. An invalid pair (no perspective
// m32) keeps the floor alone.
void ClipOwnNear(float khon_w)
{
    float khon_n = KH_OWN_NEAR;
    if (depthParams.y < -1.0e-3f && depthParams.x > 1.0e-6f)
        khon_n = min(khon_n, -depthParams.y / depthParams.x);
    if (khon_w < khon_n) discard;
}

void ClipEdgeSliver(float3 wpos)
{
    // This clip trusts fxParams0.xyz as the camera. The failure direction is 'fireflies possible', never 'geometry
    // lost'.
    if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return;
    float3 khes_dx = ddx(wpos);
    float3 khes_dy = ddy(wpos);
    // Same failure direction: stand the clip down when either derivative sits within ~4 ulps of the coordinate
    // magnitude (2^-23 = 1.2e-7; 16 = 4 ulps squared).
    float khes_ulp = max(max(abs(wpos.x), abs(wpos.y)), abs(wpos.z)) * 1.2e-7f;
    float khes_q2 = khes_ulp * khes_ulp * 16.0f;
    if (dot(khes_dx, khes_dx) < khes_q2 || dot(khes_dy, khes_dy) < khes_q2) return;
    // Close-range stand-down: standing near a face's own plane puts a large real face at true grazing incidence,
    // and a firefly test built for distant degenerate slivers must not eat close-up geometry at the sliver's angle.
    float3 khes_v = fxParams0.xyz - wpos;
    float khes_vl = length(khes_v);
    if (khes_vl < 10.0f) return;
    float3 khes_fn = cross(khes_dx, khes_dy);
    float khes_nv = abs(dot(khes_fn, khes_v / max(khes_vl, 1.0e-6f)))
                  / max(length(khes_fn), 1.0e-12f);
    float khes_deg = length(khes_fn)
                   / max(length(khes_dx) * length(khes_dy), 1.0e-20f);
    if (khes_deg < 0.05f) clip(khes_nv - 0.005f);
}

// A mesh's localization / band mask tests its interpolated absolute wpos, so far from the origin the mask's
// edge steps with wpos's float (2 - 4 mm at 16 - 40 km; seen only with falloff ~0, where the edge crosses a
// surface near the camera). The effect passes and Pulse test the camera-relative point (KH_LOCAL_REL); the same
// here would need a camera-relative position out of the mesh vertex stage. Not covered.
float SolidMask(float3 wpos)
{
    float m = 1.0f;

    if (bandParams.w > 0.5f)
    {
        float d = distance(wpos, fxParams0.xyz);
        float fall = max(bandParams.z, 0.01f);
        float bm = smoothstep(bandParams.x - fall, bandParams.x, d);
        if (bandParams.y > 0.0f)
            bm *= 1.0f - smoothstep(bandParams.y, bandParams.y + fall, d);
        m *= bm;
    }

    if (localParams1.y > 0.5f)
    {
        float3 nd3 = abs(wpos - localParams0.xyz) / max(localRadii.xyz, 0.01f);
        float nd = (localParams0.w > 0.5f)
                 ? max(nd3.x, max(nd3.y, nd3.z))   // Cube (Chebyshev).
                 : length(nd3);   // Sphere/ellipsoid.
        m *= 1.0f - smoothstep(1.0f, 1.0f + max(localParams1.x, 0.001f), nd);
    }

    return m;
}

// KH_DL_SHADOW - the dynamic-light shadow term. The cube needs no matrix: KH_DLS_BASIS is an axis permutation with
// signs, so for a light-relative point the face is the dominant axis, the depth is that component, and the other two
// are the uv. Every failure path returns 1.0: the worst a bug can do is fail to darken.

static const float KH_DLS_AMB_KEEP = 0.35f;
// The slope term is priced in texels, not metres, so it scales with range as the depth error it covers does: one map
// texel (khd_texel) added to the bias where the normal-offset arm is on (KhDlsShadow), beside the 1.5-texel
// receiver-normal offset (one texel for the receiver's, half for the bilinear footprint). KH_DLS_GRAD_TEXELS clamps
// the gradient, in texel-widths of depth per texel of lateral step.
static const float KH_DLS_GRAD_TEXELS = 8.0f;
// The HLSL twin of kh_dls_near, row for row: the receiver's (a, c) must be built to the same near the map was
// rasterised with. The three constants are the C++ ones and change with them.
static const float KH_DLS_NEAR_M     = 0.05f;
static const float KH_DLS_NEAR_RATIO = 300.0f;
static const float KH_DLS_NEAR_CAP_M = 0.25f;
float KhDlsNear(float khn_far)
{
    return min(max(khn_far / KH_DLS_NEAR_RATIO, KH_DLS_NEAR_M), KH_DLS_NEAR_CAP_M);
}
// Receiver-distance fade at the shadow view distance, the light maps' twin of KhSunRangeFade; without it a lamp's
// shadow on the ground draws at any range while the sun's stops at shadowVisibility. A sphere about dlsRange.xyz
// fading over 0.94 - 0.995 R (the sun's is a cylinder, from KH_SUN_FADE_START).
float KhDlsRangeFade(float3 khrf_p)
{
    if (dlsRange.w <= 0.0f) return 1.0f;
    const float khrf_d = length(khrf_p - dlsRange.xyz);
    return 1.0f - smoothstep(0.94f * dlsRange.w, 0.995f * dlsRange.w, khrf_d);
}
// Bias derivation: the maps are D32_FLOAT (kh_dls_ensure_maps), stored as a + c/z with a = f/(f-n) and
// c = -nf/(f-n), so d(stored)/dz = nf/((f-n) z^2) and one float ulp near stored ~ 1 is 2^-24.

Texture2DArray<float> khDlsMaps : register(t36);

// Parity with the sun filter: KhSunBilinT / KhSunSoftWT on an array slice - same bilinear weights, same 3 x 3
// footprint ring, same 9-tap average, same rule that a depth map is compared BEFORE it is filtered. The compare is
// built PER TAP: a single projected reference z is only correct for a receiver parallel to the map plane; on a tilted
// one the true depth differs at every texel of the footprint, so (as KhSelfTapT does) the reference is corrected per
// tap by d.x*g.x + d.y*g.y. khb_zb = the receiver's BIASED face-axis depth in metres; khb_tc = the RECEIVER's own uv
// in texels (never the filter's offset tap: the gradient origin must stay put while the footprint moves); khb_g =
// metres of face-axis depth per texel along +u / +v; a/c/near = the face projection, applied per tap.
float KhDlsBilin(float khb_sz, float2 uv, float khb_slice,
                 float khb_zb, float2 khb_tc, float2 khb_g,
                 float khb_a, float khb_c, float khb_near)
{
    float2 tx = uv * khb_sz - 0.5f;
    float2 f = frac(tx);
    // Floor, not truncate (KhSunBilinT's rule): truncated, at the outermost texel of every cube face a tap's khb_d
    // below would be taken at the neighbouring texel, not the one the Gather (which floors tx itself) returned.
    int2 p0 = int2(floor(tx));
    int khb_mx = (int)khb_sz - 1;
    // KH_SHADOW_GATHER: the block's four texels in one fetch. khb_slice is a whole slice below the array's size
    // (KhDlsFaceUV refuses a negative one; kh_dls_render assigns slices only below dls_slices), where Gather's
    // rounded index and the Load's truncated one are the same slice. Tap order 0..3 = (0,0) (1,0) (0,1) (1,1).
    const float4 khb_gv = khDlsMaps.Gather(khPfSamp, float3((floor(tx) + 1.0f) / khb_sz, khb_slice));
    const float4 khb_gt = float4(khb_gv.w, khb_gv.z, khb_gv.x, khb_gv.y);
    float4 khb_o;
    [unroll] for (int khb_k = 0; khb_k < 4; ++khb_k) {
        int2 khb_q = clamp(p0 + int2(khb_k & 1, khb_k >> 1),
                           int2(0, 0), int2(khb_mx, khb_mx));
        // Texel centres sit at integer + 0.5 and khb_tc is in the same convention, so khb_d is a signed count of
        // texels.
        float2 khb_d = (float2(khb_q) + 0.5f) - khb_tc;
        float khb_zq = khb_zb + khb_d.x * khb_g.x + khb_d.y * khb_g.y;
        float khb_ref = khb_a + khb_c / max(khb_zq, khb_near);
        khb_o[khb_k] = (khb_ref > khb_gt[khb_k]) ? 1.0f : 0.0f;
    }
    return lerp(lerp(khb_o.x, khb_o.y, f.x), lerp(khb_o.z, khb_o.w, f.x), f.y);
}
 
// The 3 x 3 footprint ring (the sun cast's KhSunSoftWT shape). khs_zb / khs_g / a / c / near replace a single
// pre-projected z (the compare is built per tap); khs_tc is computed ONCE here from the unoffset receiver uv and
// handed to every tap - the ring moves the FOOTPRINT and must not move the gradient's origin. khs_sp is the receiver
// footprint in map texels, clamped 1..4, priced by the caller from fwidth(wpos) before any per-pixel branch.
float KhDlsSoft(float khs_sz, float2 uv, float khs_slice,
                float khs_zb, float2 khs_g,
                float khs_a, float khs_c, float khs_near, float khs_sp)
{
    const float2 khs_tc = uv * khs_sz;
    float khs_o = khs_sp / max(khs_sz, 1.0f);
    float khs_acc = KhDlsBilin(khs_sz, uv, khs_slice, khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near);
    [unroll] for (int khs_j = -1; khs_j <= 1; ++khs_j) {
        [unroll] for (int khs_i = -1; khs_i <= 1; ++khs_i) {
            if (khs_i == 0 && khs_j == 0) continue;
            khs_acc += KhDlsBilin(khs_sz, uv + float2((float)khs_i, (float)khs_j) * khs_o, khs_slice,
                                  khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near);
        }
    }
    return khs_acc / 9.0f;
}

// khd_nrm (KhDlsShadow) is the receiver's world normal - the geometric one where the caller has it (KH_DLS_GEOM_N),
// since the offset and the plane gradient model the true surface. A mesh shadowing itself is the common case (a lamp
// beside a prop), and a self-compare with only a constant bias acnes wherever the surface slants away; this kernel
// takes the sun kernel's three defences (receiver-normal offset, texel-priced slope term, receiver-plane gradient).

// The face, slice and uv selection, shared so any second reader asks the SAME question the shadow lookup asks.
// Returns false where the lookup would answer "lit" without sampling. Also returns the projection axes the
// receiver-plane gradient needs, from the selection itself so they cannot drift from it: khf_r / khf_u = the
// projection's right / up axes, light-relative (world-parallel: dlsMeta.xyz is a translation); khf_sx = the lateral
// scale (1 for a cube face, 1/tan(fov/2) for a spot; one texel spans 2*z/(size*sx) metres). khf_sel is the UNOFFSET
// light-relative position and the only input to the face choice; khf_p (offset) is what gets projected - choosing the
// face from the offset position lets a receiver within 1.5 texels of an edge flip to the neighbouring face and land at
// its rim (a lit seam along every face boundary). The spot branch never selects and ignores khf_sel.
bool KhDlsFaceUV(int khf_slot, float3 khf_p, float3 khf_sel, float khf_near, float khf_far,
                 out float2 khf_uv, out float khf_z, out float khf_slice,
                 out float3 khf_r, out float3 khf_u, out float khf_sx)
{
    khf_uv = float2(0.0f, 0.0f);
    khf_z = 0.0f;
    khf_slice = -1.0f;
    khf_r = float3(1.0f, 0.0f, 0.0f);
    khf_u = float3(0.0f, 1.0f, 0.0f);
    khf_sx = 1.0f;

    if (dlsCtl[khf_slot].x >= 0.5f) {
        float4 khf_cl = mul(float4(khf_p, 1.0f), dlsSpotVP[khf_slot]);
        if (khf_cl.w <= 1.0e-6f) return false;          // Behind the spot.
        khf_uv = float2(0.5f + 0.5f * khf_cl.x / khf_cl.w,
                        0.5f - 0.5f * khf_cl.y / khf_cl.w);
        khf_z = khf_cl.w;
        khf_slice = dlsCtl[khf_slot].z;
        // Row-vector convention: clip.x = dot(p, column x), and kh_dls_spot_vp builds column x as right * s, so the
        // column's direction is the right axis and its length the lateral scale.
        const float3 khf_cx = float3(dlsSpotVP[khf_slot][0].x,
                                     dlsSpotVP[khf_slot][1].x,
                                     dlsSpotVP[khf_slot][2].x);
        const float3 khf_cy = float3(dlsSpotVP[khf_slot][0].y,
                                     dlsSpotVP[khf_slot][1].y,
                                     dlsSpotVP[khf_slot][2].y);
        khf_sx = max(length(khf_cx), 1.0e-6f);
        khf_r = khf_cx / khf_sx;
        khf_u = khf_cy / max(length(khf_cy), 1.0e-6f);
    } else {
        // Dominant axis = the cube face; st is that face's (right, up) pair from KH_DLS_BASIS (the C++ table's
        // twin). The face is chosen from khf_sel; z, st and the axes are read off khf_p on that face.
        const float3 khf_ap = abs(khf_sel);
        float2 khf_st;
        int khf_face;
        // khf_r / khf_u: khf_st's permutation as vectors, KH_DLS_BASIS row for row.
        if (khf_ap.x >= khf_ap.y && khf_ap.x >= khf_ap.z) {
            const bool khf_pos = khf_sel.x > 0.0f;
            khf_z = khf_pos ? khf_p.x : -khf_p.x;
            khf_face = khf_pos ? 0 : 1;
            khf_st = khf_pos ? float2(-khf_p.z, khf_p.y)
                             : float2( khf_p.z, khf_p.y);
            khf_r = float3(0.0f, 0.0f, khf_pos ? -1.0f : 1.0f);
            khf_u = float3(0.0f, 1.0f, 0.0f);
        } else if (khf_ap.y >= khf_ap.z) {
            const bool khf_pos = khf_sel.y > 0.0f;
            khf_z = khf_pos ? khf_p.y : -khf_p.y;
            khf_face = khf_pos ? 2 : 3;
            khf_st = khf_pos ? float2(khf_p.x, -khf_p.z)
                             : float2(khf_p.x,  khf_p.z);
            khf_r = float3(1.0f, 0.0f, 0.0f);
            khf_u = float3(0.0f, 0.0f, khf_pos ? -1.0f : 1.0f);
        } else {
            const bool khf_pos = khf_sel.z > 0.0f;
            khf_z = khf_pos ? khf_p.z : -khf_p.z;
            khf_face = khf_pos ? 4 : 5;
            khf_st = khf_pos ? float2( khf_p.x, khf_p.y)
                             : float2(-khf_p.x, khf_p.y);
            khf_r = float3(khf_pos ? 1.0f : -1.0f, 0.0f, 0.0f);
            khf_u = float3(0.0f, 1.0f, 0.0f);
        }
        if (khf_z <= khf_near) return false;            // Inside the near plane.
        khf_uv = float2(0.5f + 0.5f * khf_st.x / khf_z,
                        0.5f - 0.5f * khf_st.y / khf_z);
        khf_slice = dlsFaceSlice[khf_slot * 6 + khf_face].x;
    }

    if (khf_slice < 0.0f) return false;                 // This face has no slice.
    if (khf_z >= khf_far) return false;                 // Past the light's reach.
    // The cube must not refuse the outermost texel of a face (it has no guard band, and the boundary would answer
    // lit one texel wide on every edge). The spot keeps the refusal: outside its one frustum there is no map, and lit
    // is right there.
    if (dlsCtl[khf_slot].x >= 0.5f) {
        if (khf_uv.x <= 0.001f || khf_uv.x >= 0.999f ||
            khf_uv.y <= 0.001f || khf_uv.y >= 0.999f) return false;
    } else {
        khf_uv = saturate(khf_uv);
    }
    return true;
}

// The receiver-plane depth gradient: the bias has to cover how much the receiver's depth CHANGES ACROSS THE FILTER
// FOOTPRINT, and on a plane depth is an affine function of map position, so it is corrected exactly instead of
// covered. The face maps u = 0.5 + 0.5*sx*(p.r)/z, v = 0.5 - 0.5*sx*(p.u)/z, and the receiver plane is n.p = d;
// solving for z gives dz/du = -2 (n.r) z^2 / (sx d), dz/dv = +2 (n.u) z^2 / (sx d), per texel the same over the map
// size. Clamped, not floored: d goes to zero as the receiver turns edge-on and the plane genuinely spans unbounded
// depth inside one texel, so the gradient is capped at KH_DLS_GRAD_TEXELS (the KH_RPDB_GC idiom) and that sliver is
// allowed to acne. The divide is guarded by MAGNITUDE, keeping d's sign (negative facing the light, positive facing
// away; flipping it would tilt the correction the wrong way on back faces).
float2 KhDlsGrad(float3 khg_p, float3 khg_n, float3 khg_r, float3 khg_u,
                 float khg_z, float khg_sz, float khg_sx, float khg_texel)
{
    const float khg_d = dot(khg_n, khg_p);
    const float khg_ad = max(abs(khg_d), 1.0e-4f);
    const float khg_sd = (khg_d < 0.0f) ? -khg_ad : khg_ad;
    const float khg_k = (khg_z * khg_z)
                      / (max(khg_sz, 1.0f) * max(khg_sx, 1.0e-6f) * khg_sd);
    const float2 khg_g = float2(-2.0f * dot(khg_n, khg_r),
                                 2.0f * dot(khg_n, khg_u)) * khg_k;
    const float khg_c = KH_DLS_GRAD_TEXELS * khg_texel;
    return clamp(khg_g, -khg_c, khg_c);
}
 
// khd_zunc: the receiver's own depth uncertainty in metres, added to the bias. Every caller passes 0: a mesh's
// interpolated geometry is exact, and the world pass (KhDlsWorldFactor from PSDlsWorld) passes 0 too.
// khd_fwp: the receiver's world footprint per screen pixel, length(fwidth(wpos)), priced by the caller BEFORE its
// light loop (a gradient inside a loop with a break does not compile).
float KhDlsShadow(int khd_slot, float3 khd_wpos, float3 khd_nrm, float khd_zunc, float khd_fwp)
{
    if (khd_slot < 0 || khd_slot > 7) return 1.0f;
    float4 khd_meta = dlsMeta[khd_slot];
    if (khd_meta.w <= 0.0f) return 1.0f;               // No map for this light.
    const float khd_rf = KhDlsRangeFade(khd_wpos);
    if (khd_rf <= 0.0f) return 1.0f;

    // Face-axis distance before the offset, to price the texel: one texel of a 90 degree face at distance z is
    // 2z / size (a spot's: KH_DLS_SPOT_TEXEL).
    const float3 khd_p0 = khd_wpos - khd_meta.xyz;
    const float  khd_z0 = max(max(abs(khd_p0.x), abs(khd_p0.y)), abs(khd_p0.z));
    uint khd_mw, khd_mh, khd_me;
    khDlsMaps.GetDimensions(khd_mw, khd_mh, khd_me);
    // An unbound array reports zero dimensions; the texel price would then divide by max(0, 1) and displace the
    // receiver three times its own distance from the light (1.5 texels of 2z). Answer lit.
    if (khd_mw < 2u) return 1.0f;
    // KH_DLS_SPOT_TEXEL: a spot's map is one frustum of lateral scale sx = 1/tan(fov/2), so its texel at depth z
    // along the axis is 2z / (size * sx) (KhDlsFaceUV's rule); priced as a cube face, the offset, the texel bias and
    // KhDlsGrad's cap would be sized from the wrong texel (sx is 0.18 - 16.7). z is the spot's clip.w (0 behind it: no
    // offset, and KhDlsFaceUV answers lit) and sx the length of column x of the matrix the lookup projects with. A
    // cube keeps z0 and sx = 1: the same division, bit for bit.
    float khd_tz = khd_z0;
    float khd_tsx = 1.0f;
    [branch] if (dlsCtl[khd_slot].x >= 0.5f) {
        khd_tz = max(dot(float4(khd_p0, 1.0f), float4(dlsSpotVP[khd_slot][0].w, dlsSpotVP[khd_slot][1].w,
                                                     dlsSpotVP[khd_slot][2].w, dlsSpotVP[khd_slot][3].w)), 0.0f);
        khd_tsx = max(length(float3(dlsSpotVP[khd_slot][0].x, dlsSpotVP[khd_slot][1].x,
                                    dlsSpotVP[khd_slot][2].x)), 1.0e-3f);
    }
    const float  khd_texel = 2.0f * khd_tz / (max((float)khd_mw, 1.0f) * khd_tsx);   // One map texel (m).
    const int    khd_fbase = khd_slot * 6;
    const bool   khd_off_on = dlsFaceSlice[khd_fbase].y >= 0.5f;
    const float3 khd_n = normalize(khd_nrm);
    // 1.5 texels: one to clear the texel the receiver sits in, half for the bilinear footprint.
    const float3 khd_p = khd_off_on ? (khd_p0 + khd_n * (khd_texel * 1.5f)) : khd_p0;
    const float  khd_far = khd_meta.w;
    const float  khd_near = KhDlsNear(khd_far);   // KhDlsNear twin of kh_dls_near.
    const float  khd_a = khd_far / (khd_far - khd_near);
    const float  khd_c = -khd_near * khd_far / (khd_far - khd_near);

    float2 khd_uv;
    float  khd_z;      // Receiver distance along the face axis, in metres.
    float  khd_slice;
    float3 khd_fr, khd_fu;   // The projection's axes and lateral scale, for the receiver-plane gradient.
    float  khd_fsx;
    if (!KhDlsFaceUV(khd_slot, khd_p, khd_p0, khd_near, khd_far, khd_uv, khd_z, khd_slice,
                     khd_fr, khd_fu, khd_fsx)) {
        return 1.0f;
    }

    const float  khd_b = dlsCtl[khd_slot].y + dlsCtl[khd_slot].w * khd_z
                       + (khd_off_on ? khd_texel : 0.0f) + khd_zunc;
    const float2 khd_g = KhDlsGrad(khd_p, khd_n, khd_fr, khd_fu, khd_z,
                                   (float)khd_mw, khd_fsx, khd_texel);

    // The sun cast's 3 x 3 footprint ring (KhSunSoftWT's shape) on an array slice (KhDlsSoft) returns occlusion in
    // the sun's convention (1 = blocked); this kernel's contract is lit, so it is inverted once here. Footprint in
    // map texels: khd_texel is already one map texel, the lateral scale included (KH_DLS_SPOT_TEXEL).
    const float khd_sp = clamp(0.5f * khd_fwp / max(khd_texel, 1.0e-6f), 1.0f, 4.0f);
    const float khd_occ = KhDlsSoft((float)khd_mw, khd_uv, khd_slice,
                                    khd_z - khd_b, khd_g,
                                    khd_a, khd_c, khd_near, khd_sp);
    return saturate(1.0f - khd_occ * khd_rf);   // Thinned by the range fade, not cut.
}
 
// KH_DLS_GEOM_N: the receiver normal the two mesh light loops hand KhDlsShadow - the GEOMETRIC (interpolated,
// face-reversed) normal, as the sun self term's khBiasN. A normal map's tilt is not the surface the map sees: fed to
// the offset and KhDlsGrad it moves every tap's reference depth and mottles the shadow after the map. Set by PSMain /
// PSComposite on the textured route before the shading call (TWIN); zero = unset, the loop's own nrm is used (the
// untextured route, whose nrm is already the geometric one).
static float3 khDlsGeomN = float3(0.0f, 0.0f, 0.0f);

// Every dynamic light that reaches this point, summed (KH_DL_RING: no count cap): each light's diffuse N.L term,
// scaled by its own shadow from our meshes' maps (KhDlsShadow), plus its per-light ambient (KH_DLS_AMB_KEEP of which
// stays in full shadow), attenuated, the sum scaled by dlGlobal.w. HDR light in the engine's scene units, added to
// the ambient and sun by ApplyLighting (the untextured combine); the textured path's twin is KhDynLightsPBR.
float3 DynLights(float3 wpos, float3 nrm)
{
    if (dlCtl.x < 0.5f) return float3(0.0f, 0.0f, 0.0f);
    int pointN = (int)dlCtl.y;
    // KH_DL_RING: no count cap.
    int totalN = pointN + (int)dlCtl.z;
    float3 n = normalize(nrm);
    float3 p;

    if (dlCtl.x >= 2.5f) {
        // Mode 3: absolute world positions (engine axes); the CPU merged the pool in world space.
        p = wpos;
    } else {
        // Modes 1/2: camera-origin decodes. fxParams0.xyz carries the camera on the solid-mesh paths (the
        // ClipEdgeSliver contract); a zeroed camera cannot decode camera-origin lights, so stand down.
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return float3(0.0f, 0.0f, 0.0f);
        p = wpos - fxParams0.xyz;

        if (dlCtl.x >= 1.5f) {
            p = float3(dot(p, dlView[0].xyz), dot(p, dlView[1].xyz), dot(p, dlView[2].xyz));
            n = float3(dot(n, dlView[0].xyz), dot(n, dlView[1].xyz), dot(n, dlView[2].xyz));
        }
    }

    float3 acc = float3(0.0f, 0.0f, 0.0f);
    const float khs_fwp = length(fwidth(wpos));   // Footprint priced outside the loop.
    // KH_DLS_GEOM_N: the shadow lookup's normal; n (shading) keeps the N.L term.
    const float3 khs_rn = (dot(khDlsGeomN, khDlsGeomN) > 0.25f) ? khDlsGeomN : nrm;

    [loop] for (int i = 0; i < totalN; ++i) {
        int b = i * 6;
        float3 L = KhDlRec(b + 0).xyz - p;
        float dist = length(L);
        L /= dist + 1e-4f;
        float d = max(dist * dlCtl.w - KhDlRec(b + 4).x, 0.0f);
        float att = saturate(1.0f / (dot(KhDlRec(b + 4).yzw, float3(1.0f, d, d * d)) + 1e-4f));
        att *= 1.0f - saturate((dist * dlCtl.w - KhDlRec(b + 5).x) * KhDlRec(b + 5).y);

        if (i >= pointN) {
            // Spot cone: the engine's log/mul/exp pow; the (c > 0) guard stands
            // in for log(0) = -inf -> exp -> 0, and dodges the pow(0, 0) NaN a
            // degenerate exponent would mint.
            float c = saturate((dot(-KhDlRec(b + 1).xyz, L) - KhDlRec(b + 1).w) * KhDlRec(b + 2).w);
            att *= (c > 0.0f) ? pow(c, KhDlRec(b + 3).w) : 0.0f;
        }
        // KH_DL_ATT_SKIP: both cut-offs above are HARD zeros (the range fade is 1 - saturate(...) past its width,
        // the cone the (c > 0) select), so everything below is multiplied by an exact 0 and the light cannot reach
        // this pixel; skipping it skips KhDlsShadow, 9 Gathers (KhDlsSoft's 9 KhDlsBilin). TWIN: KhDlsWorldFactor's
        // guard. No gradient is skipped: khs_fwp is priced before the loop, and Gather takes none (KH_GATHER_NO_LOD).
        if (att <= 0.0f) continue;

        // The shadow scales the directional term in full and the per-light ambient KhDlRec(b + 3) only in part
        // (KH_DLS_AMB_KEEP, below): that ambient is the away-facing glow that makes A3 lights read on surfaces facing
        // away, and a surface in shadow is still partly inside that glow.
        // A dim light casts a faint shadow and a bright one a hard shadow for free.
        // KH_SHADOW_SWITCH: a mesh with receiveShadow off takes no light's shadow.
        // KH_NORECV_BRANCH: a branch, not ?: - fxc evaluates both arms of a ternary, so the lookup (9 Gathers)
        // would run for such a mesh too. Nothing under it takes a gradient (KH_DL_ATT_SKIP's note).
        float khs_sh = 1.0f;
        [branch] if (!(khObjNoRecv >= 0.5f)) {
            khs_sh = KhDlsShadow((int)KhDlRec(b + 5).z - 1, wpos, khs_rn, 0.0f, khs_fwp);
        }
        // A pure-ambient light must still cast: KH_DLS_AMB_KEEP of the per-light ambient stays in full shadow.
        const float khs_amb = lerp(KH_DLS_AMB_KEEP, 1.0f, khs_sh);
        float ndl = max(dot(n, L), 0.0f);
        acc += (dlGlobal.xyz * KhDlRec(b + 2).xyz * ndl * khs_sh
              + KhDlRec(b + 3).xyz * khs_amb) * att;
    }

    return acc * dlGlobal.w;
}

// KH_MESH_O1 - the two outputs of the mesh pixel shaders (PSMain, PSComposite, PSWhite). o1 is the sorted tail's
// dual-source blend operand (KhWseeMeshOut, modes 2 and 3: the C++ bs_wsee_sort reads it as SRC1_COLOR /
// SRC1_ALPHA) and 0 on every other draw, where slot 1 is unbound and the output is dropped (a route that draws them
// under the engine's binding binds slot 0 alone first - KhOmSlot0 - so it never lands in an engine target).
struct KhMeshOut { float4 c : SV_Target0; float4 o1 : SV_Target1; };
// KH_MESH_O1: the colour alone - o1 zero.
KhMeshOut KhMeshOutC(float4 khmo_c)
{
    KhMeshOut khmo_o;
    khmo_o.c = khmo_c;
    khmo_o.o1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    return khmo_o;
}

// KH_WSEE - the sorted tail parts (the flush's normal-blend parts that test the depth without writing it, the
// injection's deferred ones among them) draw after the game's layers, and PSMain (its textured twin, a user
// material's static twin, PSWhite in their place) tests and composites them against the cycle's record (the
// KH_FX_UNIT section's KH_WSEE note). The mesh unit's views (declared below; C++ KhWseeSortScope binds them): khMsS
// (S), khMsT (T), khMsB (B), khMsZ (the live main depth, MS), khMsN (NWpre), khMsQ (Q: the scene resolved before the
// run's first compositing part), khMsN2 (NWpost), khMsB2, khMsS2, khMsD3, khMsP0 / P1 / Nb / Nb2 - the FX unit's
// record. Armed by khFxTexLay[1].z (KhWseeMeshMode: 1 = the occlusion test with the plain blend; 2 = the test and
// the layer-aware composite under the dual-source state on a float target; 3 = the same on any other target, colour
// and alpha held to [0, 1]; C++ kh_wsee_sort_mode) on such a part's draw alone - 0 on every other mesh draw, where
// nothing here loads and the output is the colour alone (KhMeshOutC). The decode is khFxFog[0] (m22, m32, the
// world's MinDepth / MaxDepth). The C++ binds no depth test (the main depth holds the fence's transparent texels,
// which would hide our glass), so:
//   OCCLUSION (KhWseeMeshCut, per sample k): the opaque raw o_k = for a late opaque (T.G at k) its live depth - D3
//   where D3 stands, at sample 0 and at k > 0 whose sample 0 is late too; for a derived pre sample 0 (S2 != S in the
//   world's range, bit 8 / 9; the live r0 < S without S2) and its G = 0 samples on its surface (KhWseeMsSame) S2
//   where B2 or D3 stands, else the live depth; for a G = 0 sample with no fence-type layer of its own (T.R = 1) its
//   live depth (KH_WSEE_SURF_BEHIND: another surface's sample at an edge, or sample 0 written after the snapshot by
//   a draw the record holds nothing of - late then, G = 1); else S.
//   The fragment is discarded at k where o_k lies below the world's range (in front: the slice, our near gap) or
//   SV_Position.w lies past o_k in metres by more than KH_WSEE_MS_REL relative or KH_WSEE_MS_ZTOL (8 D24 steps at
//   o), whichever is larger (the two encodes' error, a coplanar part). The kept set goes out as SV_Coverage (khMsCov,
//   ANDed with the raster's); empty = discard. 2 n + 1 loads a fragment at the least (D3 at a replayed sample 0; S2
//   and KhWseeMsGrad's four for a derived pre one). The layer verdict (front, Tr, background) is sample 0's for the
//   pixel.
//   COMPOSITE (KhWseeMeshOut, modes 2 / 3): the nearest layer f = min(live, NWpre, NWpost) (NW positions through
//   KhWseeMsNwPos); a layer stands where f < o and f is in the world's range. In front of the fragment the target
//   becomes dest + Tr a (c - X), Tr = Tw Tpre Tpost (held by the stacking cap Tr X <= Q per channel, KhWseeMsCap),
//   X = B - or B2 with Tr = Tw Tpost where B2 was taken (bit 3) and no pre layer lies in front of the fragment (every
//   pre layer is then in B2). Written without a negative source: o0 = (Tr a c, 0), o1 = (saturate(1 - Tr a X /
//   max(Q, KH_WSEE_MS_QMIN)), 1), dest taken as Q - exact where no tail part drew the pixel after Q was taken (an
//   earlier part leaves out off by -d Tr a X / Q); per sample the factor is the pixel's, so the resolve is exact.
//   Over a late opaque (G marks it, but a derived pre one with the B2 form; X would be O, which no snapshot holds):
//   o1 = (1 - Tr a, 1), dest for the surface - off by Tr a (P - (1 - Tr) O), small where the layers in front are
//   faint. No layer, or the nearest behind the fragment: the plain over, o0 = (a c, a), o1 = 1 - a. Mode 1: the
//   test with the plain blend.
//   Light-adding layers (in NW, not in T) keep the composite exact (each layer is affine in what lies behind). Not in
//   the record (taken as absent): screen, multiply and the like.
// Not covered: layers both in front of and behind the fragment at one pixel (Tr is all of theirs); our own parts
// overlapping (an earlier sorted part is in T / NW but not in B); the layer verdict at sample 0 only; a late opaque
// a depth-writing layer drew over afterwards where D3 does not stand; engine layers the record does not place are
// covered at a by our later tail parts; an opaque drawn after a nearer fence-type layer at a sample (it failed the
// engine's own test there: in no depth and no record).
#if !KH_FX_UNIT
Texture2D<float> khMsS : register(t53);
Texture2DMS<float4> khMsT : register(t54);   // RGBA - Tw, G, Tpre, Tpost.
Texture2D<float4> khMsB : register(t55);
Texture2DMS<float> khMsZ : register(t56);
Texture2D<float> khMsN : register(t57);
Texture2D<float4> khMsQ : register(t58);
Texture2D<float> khMsN2 : register(t59);   // NWpost (khMsN is NWpre).
Texture2D<float4> khMsB2 : register(t60);   // B2, bound in modes 2 / 3 where khFxTexLay[1].w bit 3 (its moment) is set.
Texture2D<float> khMsS2 : register(t62);    // S2 (the depth at the B2 moment), bound where bit 9 says.
// KH_WSEE_OQ2: a post late opaque's D3 is resolved from its own interpolated depth (PSWseeD3Res).
Texture2D<float> khMsD3 : register(t65);    // D3 (a late opaque's own depth), where bit 5 says it stands.
// KH_PSB (the FX section's note at khWsP0): the one pass's record (KH_PSB_PHASE: Tpre in P0's R, Tpost in P1's,
// NWpre in N, NWpost in N2) where khFxTexLay[1].w bit 6 says a re-issue drew into it; bit 7: the old record's NW
// holds its clear (not loaded). The sorted part reads each phase as the product of both records' T and the nearer of
// both NWs, the read-time reset at sample 0's late opaque (a phase whose nearest one-pass layer lies behind the
// opaque raw o: 1).
Texture2D<float4> khMsP0 : register(t66);
Texture2D<float4> khMsP1 : register(t67);   // KH_PSB_PHASE: the post phase's.
Texture2D<float> khMsNb : register(t68);
Texture2D<float> khMsNb2 : register(t69);   // KH_PSB_PHASE: NWpost.
// KH_PSB_LORES (twin of the FX unit's KhWseePsbAt): the one pass draws at 1 / 2 the frame a side (KH_WSEE_PSB_LS), so
// the record's texel for pixel px is px >> 1, clamped to floor(W / 2) - 1 (W = khMsS's size): the one pass certainly
// writes that texel, while the texel past it may be undrawn or partial at an odd W (KH_PSB_LORES_EDGE) - the frame's
// last W mod 2 pixels read their neighbour block's texel in place of the clear.
int2 KhWseeMsPsbAt(int2 khmp_px) {
    const int khmp_ls = KH_WSEE_PSB_LS;   // s = 2 (C++ KH_PSB_LS).
    uint khmp_w, khmp_h;
    khMsS.GetDimensions(khmp_w, khmp_h);   // KH_PSB_LORES_EDGE.
    return min(khmp_px >> khmp_ls, max((int2((int)khmp_w, (int)khmp_h) >> khmp_ls) - 1, int2(0, 0)));
}
#define KH_WSEE_MS_REL 2.0e-3f
#define KH_WSEE_MS_QMIN 1.0e-6f   // Q's floor in the divisor (Q = 0: B's share is 0 there; the factor saturates).
#define KH_WSEE_MS_ZTOL (8.0f / 16777216.0f)
// KhWseeMeshCut's verdict for KhWseeMeshOut (one pixel's: per invocation).
static bool khMsFront = false;   // The nearest layer lies in front of the fragment (modes 2 / 3).
static float khMsTr = 1.0f;   // T at sample 0 where khMsFront (Tw Tpre Tpost; capped against B and Q).
static float3 khMsBg = float3(0.0f, 0.0f, 0.0f);   // B where khMsFront (or B2, T then Tw Tpost).
static float3 khMsQv = float3(1.0f, 1.0f, 1.0f);   // Q where khMsFront.
static bool khMsLate = false;   // khMsFront over a late opaque (G = 1): dest stands for what lies behind.
// Bit k set where the fragment lies in front of sample k's opaque - the caller's SV_Coverage.
// Every bit unarmed (the raster's coverage stands).
static uint khMsCov = 0xFFFFFFFFu;
#define KH_WSEE_MS_NS 32u   // The samples tested at most - SV_Coverage's width, D3D11's sample cap.
float KhWseeMeshMode() { return khFxTexLay[1].z; }
// The view depth (m) of a raw main-depth value - LinDepth's rule on khFxFog[0] (about the far plane's distance at
// raw == MaxDepth, 1e9 past it).
float KhWseeMeshMeters(float khmm_raw)
{
    const float khmm_n = (khmm_raw - khFxFog[0].z) / max(khFxFog[0].w - khFxFog[0].z, 1e-6f);
    const float khmm_d = khmm_n - khFxFog[0].x;
    if (khmm_d > -1e-7f) return 1e9f;
    const float khmm_m = khFxFog[0].y / khmm_d;
    return khmm_m > 0.0f ? khmm_m : 1e9f;
}
// KH_WSEE - the stacking cap (the FX section's KhWseeCap, the mesh unit's twin): a layer record cannot show more
// of khmp_x (the colour behind the layers) than the pixel khmp_p holds - T held to khmp_p / khmp_x per channel where
// khmp_x exceeds KH_WSEE_CAP_EPS. A consistent record is unchanged.
#define KH_WSEE_CAP_EPS (1.0f / 1024.0f)
float KhWseeMsCap(float khmp_t, float3 khmp_p, float3 khmp_x)
{
    [unroll] for (int khmp_i = 0; khmp_i < 3; ++khmp_i) {
        if (khmp_x[khmp_i] > KH_WSEE_CAP_EPS) khmp_t = min(khmp_t, max(khmp_p[khmp_i], 0.0f) / khmp_x[khmp_i]);
    }
    return khmp_t;
}
// An NW raw khmn_n as the layer's POSITION - KhWseeNwPos's twin (its note; C++ KhWseeSurfVp's push, Hi =
// khFxFog[0].w here): the pushed value itself still decides absorption (n' against the opaque); only an n' at or
// before Hi is taken back; no layer (NW's clear, 1) stays 1. KH_WSEE_SURF_TOL: C++'s twin.
#define KH_WSEE_SURF_TOL (0.02f)
float KhWseeMsNwPos(float khmn_n)
{
    const bool khmn_b = khmn_n < 1.0f && khmn_n <= khFxFog[0].w;   // A pushed value (<= Hi).
    return khmn_b ? (khmn_n - KH_WSEE_SURF_TOL * khFxFog[0].w) / (1.0f - KH_WSEE_SURF_TOL) : khmn_n;
}
// One sample's occlusion test for the fragment's view depth khmk_w (m) against the sample's
// opaque raw khmk_o: false below the world's range, or where the fragment lies past o's metres by more than
// KH_WSEE_MS_REL relative or eight D24 steps at o (KH_WSEE_MS_ZTOL; the larger).
bool KhWseeMsKeep(float khmk_o, float khmk_w)
{
    if (khmk_o < khFxFog[0].z) return false;
    const float khmk_zo = KhWseeMeshMeters(khmk_o);
    const float khmk_zu = KhWseeMeshMeters(min(khmk_o + KH_WSEE_MS_ZTOL, 1.0f));
    return !(khmk_w > max(khmk_zo * (1.0f + KH_WSEE_MS_REL), khmk_zu));
}
// KH_WSEE_NOPRE: whether sample khms_k's raw khms_rk lies on sample 0's surface (raw khms_r0, screen gradient khms_g)
// - the FX unit's KhWseeSame / KhWseeGrad (their twins: the same rule on the same live depth, khMsZ here, its
// KH_WSEE_ZTOL KH_WSEE_MS_ZTOL's value), so KhWseeMeshCut's per-sample pre flags are the chain's.
float2 KhWseeMsGrad(int2 px, float khmg_r0)
{
    uint khmg_w, khmg_h, khmg_n;
    khMsZ.GetDimensions(khmg_w, khmg_h, khmg_n);
    float2 khmg_g = float2(0.0f, 0.0f);
    [unroll] for (int khmg_a = 0; khmg_a < 2; ++khmg_a) {
        const int2 khmg_e = khmg_a == 0 ? int2(1, 0) : int2(0, 1);
        const int  khmg_p = khmg_a == 0 ? px.x : px.y;
        const int  khmg_m = (int)(khmg_a == 0 ? khmg_w : khmg_h);
        const bool khmg_hf = khmg_p + 1 < khmg_m;
        const bool khmg_hb = khmg_p > 0;
        const float khmg_f = khMsZ.Load(px + khmg_e, 0) - khmg_r0;   // Off the frame: unused (selected away).
        const float khmg_b = khmg_r0 - khMsZ.Load(px - khmg_e, 0);
        float khmg_d = 0.0f;
        if (khmg_hf && khmg_hb) khmg_d = abs(khmg_f) <= abs(khmg_b) ? khmg_f : khmg_b;
        else if (khmg_hf) khmg_d = khmg_f;
        else if (khmg_hb) khmg_d = khmg_b;
        if (khmg_a == 0) khmg_g.x = khmg_d;
        else khmg_g.y = khmg_d;
    }
    return khmg_g;
}
bool KhWseeMsSame(uint khms_k, float khms_r0, float khms_rk, float2 khms_g)
{
    const float2 khms_d = khMsZ.GetSamplePosition((int)khms_k) - khMsZ.GetSamplePosition(0);
    const float  khms_p = khms_r0 + dot(khms_g, khms_d);
    return abs(khms_rk - khms_p) <= KH_WSEE_MS_ZTOL + 0.5f * dot(abs(khms_g), abs(khms_d));
}
// The occlusion test and the layer verdict at the fragment khmc_p (SV_Position: xy the pixel, w its view depth).
// Discards; armed alone. The test per sample into khMsCov, the discard where no sample keeps the fragment; the layer
// verdict sample 0's.
void KhWseeMeshCut(float4 khmc_p)
{
    [branch] if (KhWseeMeshMode() > 0.5f) {
        const int2 khmc_px = int2(khmc_p.xy);
        const float4 khmc_t = khMsT.Load(khmc_px, 0);
        const float khmc_z = khMsZ.Load(khmc_px, 0);
        bool       khmc_g = khmc_t.y > 0.25f;   // A replayed late opaque (G = 1; T holds no 0.5).
        // A derived pre one (KH_WSEE_NOPRE, below; bit 3 the B2 moment, S2 bound - B2 too in modes 2 / 3): its depth is
        // S2's - a fence drawn over it since holds the live depth there.
        const bool khmc_b2 = ((uint)(khFxTexLay[1].w + 0.5f) & 8u) != 0u;
        bool       khmc_pre = false;   // Set by the derived path alone (T's G is 0 or 1).
        // D3 stands (bit 5): a late opaque's opaque raw is its own depth.
        const bool khmc_d3 = ((uint)(khFxTexLay[1].w + 0.5f) & 32u) != 0u;
        const float khmc_s = khMsS.Load(int3(khmc_px, 0));   // Once: every G = 0 sample's opaque (sample 0's S).
        float khmc_o = khmc_g ? khmc_z : khmc_s;
        [branch] if (khmc_d3 && khmc_g) khmc_o = khMsD3.Load(int3(khmc_px, 0));
        // KH_WSEE_NOPRE (the FX section's note at khWsS2): bit 8 - this cycle's pre late opaques were not replayed;
        // sample 0 is pre where S2 (bit 9, bound in every mode; else the live depth) differs from S and no post
        // opaque owns it, and reads as a replayed one did: its opaque raw S2 where B2 or D3 stands, else the live
        // depth; the loop's G = 0 samples on its surface likewise (below).
        const uint khmc_av0 = (uint)(khFxTexLay[1].w + 0.5f);
        const bool khmc_d2 = (khmc_av0 & 512u) != 0u && (khmc_b2 || khmc_d3);   // A derived opaque's raw is S2.
        bool  khmc_dp = false;
        float khmc_dr = khmc_z;
        [branch] if (!khmc_g && (khmc_av0 & 256u) != 0u) {
            // KhWseeRule's twin test - S2 only in the world's range, there S2 != S; else
            // the live depth before S; the reference itself in the world's range (the slice's raw: no late opaque).
            bool khmc_dq = khmc_z < khmc_s;
            [branch] if ((khmc_av0 & 512u) != 0u) {
                const float khmc_s2 = khMsS2.Load(int3(khmc_px.x + (int)(khmc_t.x > 2.0f), khmc_px.y, 0));
                const bool  khmc_dw = khmc_s2 >= khFxFog[0].z;
                khmc_dq = khmc_dw ? (khmc_s2 != khmc_s && khmc_s >= khFxFog[0].z) : khmc_dq;
                khmc_dr = khmc_dw ? khmc_s2 : khmc_dr;
            }
            khmc_dp = khmc_dq && khmc_dr >= khFxFog[0].z;
            [branch] if (khmc_dp) {
                khmc_g = true;
                khmc_pre = khmc_b2;
                khmc_o = khmc_d2 ? khmc_dr : khmc_z;
            }
        }
        // KH_WSEE_SURF_BEHIND (KhWseeRule's twin): sample 0's live depth nearer than its snapshot, no late opaque
        // replayed and no fence-type layer at the sample (T.R = 1) - a depth-writing draw after the capture the
        // record holds nothing of stands at z: the opaque raw is z and the surface is late (G = 1: dest stands for
        // it in the composite; B is behind it, not behind the layers). With S instead, a part behind such a surface
        // (a sea surface the replay missed) is kept and drawn over it.
        [branch] if (!khmc_g && khmc_t.x >= 1.0f && khmc_z < khmc_s) {
            khmc_g = true;
            khmc_o = khmc_z;
        }
        bool   khmc_gh = false;   // KH_WSEE_NOPRE: the gradient taken (KhWseeMsSame's), once a fragment.
        float2 khmc_gr = float2(0.0f, 0.0f);
        // Sample 0's verdict is bit 0; samples 1 .. n - 1 (n the live depth's count) each take their own T and live
        // depth, S the pixel's (the loop loads no S2: the derived path above did). A G = 1 sample k takes its live
        // depth - KhWseeRule's rule, so the fog and the glass agree at an edge - or, with D3, sample 0's D3 where
        // sample 0 is a replayed (G = 1) opaque (khmc_o holds it: no load; a derived one has G 0 in T).
        uint khmc_w, khmc_h, khmc_n;
        khMsZ.GetDimensions(khmc_w, khmc_h, khmc_n);
        khmc_n = clamp(khmc_n, 1u, KH_WSEE_MS_NS);
        uint khmc_m = KhWseeMsKeep(khmc_o, khmc_p.w) ? 1u : 0u;
        [loop] for (uint khmc_k = 1u; khmc_k < khmc_n; ++khmc_k) {
            const float4 khmc_tk = khMsT.Load(khmc_px, (int)khmc_k);
            const float  khmc_zk = khMsZ.Load(khmc_px, (int)khmc_k);
            const bool   khmc_gk = khmc_tk.y > 0.25f;
            // KH_WSEE_SURF_BEHIND: S is sample 0's snapshot alone - a G = 0 sample with no fence-type layer of its
            // own (T.R = 1) takes its live depth (its own surface: another one at an edge, sample 0's where they
            // agree - z_k = S then - or a surface written after the snapshot); a fence-type texel keeps S
            // (the main depth holds its transparent texels, which would hide a part behind them).
            float khmc_ok = (khmc_gk || khmc_tk.x >= 1.0f) ? khmc_zk : khmc_s;
            [branch] if (khmc_d3) {   // A replayed sample 0 (G 1) - a derived one has G 0 in T.
                if (khmc_gk && khmc_t.y > 0.25f) khmc_ok = khmc_o;
            }
            // KH_WSEE_NOPRE: a G = 0 sample under a derived pre sample 0 is pre too where it lies on sample 0's
            // surface - KhWseeMsSame, the chain's KhWseeSame on the same depth (so the fog and the glass agree at an
            // edge); its gradient once a fragment, its address carrying a raw-derived 0 (fxc keeps the loads here).
            [branch] if (khmc_dp && khmc_tk.y <= 0.25f) {
                bool khmc_on = khmc_zk == khmc_z;
                [branch] if (!khmc_on) {
                    [branch] if (!khmc_gh) khmc_gr = KhWseeMsGrad(khmc_px + int2((int)(khmc_zk > 2.0f), 0), khmc_z);
                    khmc_gh = true;
                    khmc_on = KhWseeMsSame(khmc_k, khmc_z, khmc_zk, khmc_gr);
                }
                if (khmc_on) khmc_ok = khmc_d2 ? khmc_o : khmc_zk;
            }
            if (KhWseeMsKeep(khmc_ok, khmc_p.w)) khmc_m |= 1u << khmc_k;
        }
        khMsCov = khmc_m;
        if (khmc_m == 0u) discard;
        [branch] if (KhWseeMeshMode() > 1.5f) {
            // G = 1: N alone can be nearer. Both phases' nearest particle-type layer. KH_PSB: of both records
            // (bit 7 clear: the old one's; bit 6: the one pass's).
            const uint khmc_av = (uint)(khFxTexLay[1].w + 0.5f);
            float2 khmc_nn = float2(1.0f, 1.0f);
            float2 khmc_nb = float2(1.0f, 1.0f);
            float  khmc_no = 1.0f;   // KH_WSEE_NOPRE: the old record's NWpre alone (its read-time reset's).
            [branch] if ((khmc_av & 128u) == 0u) {
                khmc_nn = float2(khMsN.Load(int3(khmc_px, 0)), khMsN2.Load(int3(khmc_px, 0)));
                khmc_no = khmc_nn.x;
            }
            [branch] if ((khmc_av & 64u) != 0u) {
                const int2 khmc_bp = KhWseeMsPsbAt(khmc_px);   // KH_PSB_LORES: the record's texel.
                khmc_nb = float2(khMsNb.Load(int3(khmc_bp, 0)), khMsNb2.Load(int3(khmc_bp, 0)));   // KH_PSB_PHASE.
                khmc_nn = min(khmc_nn, khmc_nb);
            }
            const float khmc_np = khmc_nn.x;
            const float khmc_nq = khmc_nn.y;
            const float khmc_f = min(khmc_z, min(khmc_np, khmc_nq));
            [branch] if (khmc_f < khmc_o && khmc_f >= khFxFog[0].z) {
                // The nearest layer's position - an NW in front of the opaque taken back from its push
                // (KhWseeMsNwPos); one at or behind it (absorbed) is no layer here. The pre layer's gated position
                // once (khmc_pp); the B2 choice below reads the same.
                const float khmc_pp = khmc_np < khmc_o ? KhWseeMsNwPos(khmc_np) : 1.0f;
                const float khmc_fp = min(khmc_z, min(khmc_pp, khmc_nq < khmc_o ? KhWseeMsNwPos(khmc_nq) : 1.0f));
                [branch] if (KhWseeMeshMeters(khmc_fp) < khmc_p.w) {
                    khMsFront = true;
                    // KH_PSB: the one pass's Tpre / Tpost at the pixel, the read-time reset over sample 0's late
                    // opaque (its raw o), times the old record's (bit 6 clear: 1 - the old record's value bit for bit).
                    float2 khmc_q = float2(1.0f, 1.0f);
                    [branch] if ((khmc_av & 64u) != 0u) {
                        const int2 khmc_qb = KhWseeMsPsbAt(khmc_px);   // KH_PSB_LORES: the record's texel.
                        const int3 khmc_qa = int3(khmc_qb.x + (int)(khmc_t.x > 2.0f), khmc_qb.y, 0);
                        khmc_q = float2(khMsP0.Load(khmc_qa).x, khMsP1.Load(khmc_qa).x);   // KH_PSB_PHASE.
                        khmc_q = float2(khmc_g && khmc_nb.x > khmc_o ? 1.0f : khmc_q.x,
                                        khmc_g && khmc_nb.y > khmc_o ? 1.0f : khmc_q.y);
                    }
                    // KH_WSEE_NOPRE: a derived pre sample 0 - the replay's reset of the old record's Tpre at read time.
                    const float khmc_tz = (khmc_dp && khmc_no > khmc_o ? 1.0f : khmc_t.z) * khmc_q.x,
                                khmc_tw = khmc_t.w * khmc_q.y;
                    khMsTr = khmc_t.x * khmc_tz * khmc_tw;   // Every layer's.
                    // No pre layer in front of the fragment and B2 taken: B2 behind, the fence-type and post layers'
                    // T (also over a derived pre late opaque: B2 holds its colour); a late opaque with a pre layer in
                    // front, or one drawn after B2, takes dest for what lies behind. The pre layer's position is
                    // khmc_pp's gated one: an NWpre absorbed by the opaque is no layer in front here either (taken
                    // back from the push it lies up to 2 % before the opaque and would refuse B2 to a part there).
                    const bool khmc_b2x = khmc_b2 && !(KhWseeMeshMeters(khmc_pp) < khmc_p.w);
                    khMsLate = khmc_g && !(khmc_pre && khmc_b2x);
                    [branch] if (!khMsLate) {
                        khMsQv = khMsQ.Load(int3(khmc_px, 0)).rgb;
                        [branch] if (khmc_b2x) {
                            khMsBg = khMsB2.Load(int3(khmc_px, 0)).rgb;
                            khMsTr = khmc_t.x * khmc_tw;   // KH_PSB: both records' post phase.
                        } else {
                            khMsBg = khMsB.Load(int3(khmc_px, 0)).rgb;
                        }
                        khMsTr = KhWseeMsCap(khMsTr, khMsQv, khMsBg);   // The stacking cap (Q holds T X).
                    }
                }
            }
        }
    }
}
// A sorted part's output for its straight colour khmw_c and alpha khmw_a (PSMain's normal-blend return): the colour
// alone (KhMeshOutC) unarmed and in mode 1; in modes 2 and 3 the dual-source pair above (SV_Target0 = o0,
// SV_Target1 = o1).
KhMeshOut KhWseeMeshOut(float3 khmw_c, float khmw_a)
{
    KhMeshOut khmw_o = KhMeshOutC(float4(khmw_c, khmw_a));
    [branch] if (KhWseeMeshMode() > 1.5f) {
        float3 khmw_k = khmw_c;
        float khmw_q = khmw_a;
        if (KhWseeMeshMode() > 2.5f) {   // Mode 3: the inputs the plain blend's would be held to.
            khmw_k = saturate(khmw_k);
            khmw_q = saturate(khmw_q);
        }
        if (khMsFront) {
            const float3 khmw_ta = (khMsTr * khmw_q).xxx;
            khmw_o.c = float4(khmw_ta * khmw_k, 0.0f);
            if (khMsLate) {   // G = 1: dest for the late surface behind the layers.
                khmw_o.o1 = float4(1.0f - khmw_ta, 1.0f);
            } else {
                khmw_o.o1 = float4(saturate(1.0f - khmw_ta * khMsBg / max(khMsQv, KH_WSEE_MS_QMIN)), 1.0f);
            }
        } else {
            khmw_o.c = float4(khmw_k * khmw_q, khmw_q);
            khmw_o.o1 = (1.0f - khmw_q).xxxx;
        }
    }
    return khmw_o;
}
#endif

// smf: per-pixel shadow factor from the caller (received world shadows and the private sun-depth self term,
// min-combined - they answer the same question at different granularities and must not stack).

// Returns HDR: neither clamped nor tone-mapped here (the one Reinhard mix is PSMain / PSComposite's blendCtl.x
// composite).
float3 ApplyLighting(float3 base, float3 wpos, float3 nrm, float smf)
{
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return base;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float3 direct = lighting2.rgb * (ndl * khObjDif * smf);   // Receive + self term (min-combined upstream).
    // Ambient occlusion is not a term here: KH_SSAO multiplies the drawer's result at the pixel (PSSsaoApply,
    // ssao.hlsl) after the draw.
    return base * (lightAmb.rgb * khObjAmb + direct + DynLights(wpos, nrm));
}

// The material lanes every reader names as matParams0..3. In the textured variants KhMatLoad fills them from the
// table entry; the untextured variants never load and read them at zero (the mirror-mask gate in PSMain /
// PSComposite reads matParams0.y in both).
static float4 matParams0 = 0.0f, matParams1 = 0.0f, matParams2 = 0.0f, matParams3 = 0.0f;

#if KH_TEXTURED
// Every material map is a layer of a texture page (a Texture2DArray of textures sharing width, height, format and
// mip count); the page is bound per draw at t14-t18 and t42, the layer comes from the material table entry.
// Materials whose maps live in the same pages batch across one instanced draw. The arma model keeps its AS map in
// the orm page and its SMDI map in the specular page (both read linear there - kh_tex_slot_srgb), and SPECCOLOR alone
// takes the sixth, at t42 (t33 is KH_VOL_FOOT's mask, t34 is read by the replay merges in static.hlsl). A user
// (.hlsl) material takes all six under either naming (KH_USER_SLOTS: its specular page is read linear when the
// script named it "smdi"); its own maps (KH_USER_TEX) follow at t43-t48. StateBackup saves t0-t69 (KH_WSEE's
// t53 - t69 included: the C++ KH_WSEE note's REGISTERS).
Texture2DArray<float4> matDiffuse  : register(t14);
Texture2DArray<float4> matNormal   : register(t15);
Texture2DArray<float4> matOrm      : register(t16);
Texture2DArray<float4> matEmissive : register(t17);
Texture2DArray<float4> matSpecular : register(t18);
Texture2DArray<float4> matSpecColor : register(t42);
SamplerState matSamp : register(s0);

// One entry per material-set slot, C++ twin KhGpuMat (13 float4). p0..p3 = the matParams0..3 lanes (map-bound
// flags, alpha mode, cutoff, normal strength / base colour, roughness / metalness, emissive intensity, occ route,
// rough route / metal route, alpha route, gloss route, spec workflow); lay0 = diffuse/normal/orm/emissive layers,
// lay1.x = specular, lay1.y = speccolor; p4 / p5 = matParams4 / 5, the arma model's lanes (specular colour,
// glossiness fallback / fresnel N, K, specular route, model): filled for the arma model and, KH_USER_SLOTS, for a
// user material (whose model lane stays 0); zero for pbr but the specular route (p5.z), which is -1 (unrouted);
// lay1.zw and lay2 = KH_USER_TEX's user0..user5 layers. p0.x's bits 0-5 are the six maps' bound flags, bits 6-11 the
// user maps'. KH_UV_ANIM: bit 12 = the uv transform, 13 its warp, 14 the flipbook; an0 = its matrix A, an1 =
// (translation c, warp phase), an2 = (warp amplitude, frequency), an3 = (columns, rows, frame) - C++
// kh_uv_anim_lanes, once a cycle. Bit 15 = KH_SPEC_AA's Toksvig arm (KhToksvigA2).
struct KhGpuMat {
    float4 p0; float4 p1; float4 p2; float4 p3; float4 lay0; float4 lay1; float4 p4; float4 p5; float4 lay2;
    float4 an0; float4 an1; float4 an2; float4 an3;
};
StructuredBuffer<KhGpuMat> khMats : register(t38);

// The per-pixel material lanes. KhMatLoad fills them from the table entry once per pixel (the index rides the VS
// interpolant, flat per draw or per instance).
static float4 khMatLay0 = 0.0f, khMatLay1 = 0.0f, khMatLay2 = 0.0f;
static float4 matParams4 = 0.0f, matParams5 = 0.0f;
static uint khMatIx = 0;   // KH_UV_ANIM: the entry KhMatLoad read (its animation lanes load where a flag needs them).
void KhMatLoad(uint khml_ix)
{
    KhGpuMat khml_m = khMats[khml_ix];
    matParams0 = khml_m.p0;
    matParams1 = khml_m.p1;
    matParams2 = khml_m.p2;
    matParams3 = khml_m.p3;
    khMatLay0 = khml_m.lay0;
    khMatLay1 = khml_m.lay1;
    khMatLay2 = khml_m.lay2;   // KH_USER_TEX.
    matParams4 = khml_m.p4;
    matParams5 = khml_m.p5;
    khMatIx = khml_ix;   // KH_UV_ANIM.
    if (matCtl.w >= 0.0f) matParams0.y = matCtl.w;   // The draw's alpha-mode override.
}
// KH_UV_ANIM: the uv this material's maps are read at, and its gradients - every reader takes it (the colour twins,
// the blend split's texel verdict, the alpha casters), so colour, coverage and shadows agree. Call after KhMatLoad,
// in uniform flow (the gradients are taken here, after the branches reconverge; the flags are per draw or per
// instance, and a quad never spans two). Unanimated (bits 12 - 14 clear) it returns uv and ddx / ddy of it. The
// flipbook's gradients are the continuous uv's over the sheet: no mip seam at a cell edge. The lanes are read under
// their flags (an unanimated material loads none of them).
float2 KhMatUv(float2 khmu_uv, out float2 khmu_dx, out float2 khmu_dy)
{
    const uint khmu_f = (uint)(int)matParams0.x;
    float2 khmu_c = khmu_uv;
    [branch] if (khmu_f & 4096u) {
        const float4 khmu_a1 = khMats[khMatIx].an1;
        khmu_c = KhUvAffine(khmu_uv, khMats[khMatIx].an0, khmu_a1.xy);
        [branch] if (khmu_f & 8192u) {
            const float4 khmu_a2 = khMats[khMatIx].an2;
            khmu_c = KhUvWarp(khmu_c, khmu_a2.x, khmu_a2.y, khmu_a1.zw);
        }
    }
    khmu_dx = ddx(khmu_c);
    khmu_dy = ddy(khmu_c);
    [branch] if (khmu_f & 16384u) {
        const float4 khmu_a3 = khMats[khMatIx].an3;
        khmu_dx /= khmu_a3.xy;
        khmu_dy /= khmu_a3.xy;
        khmu_c = KhUvFlip(khmu_c, khmu_a3.xy, khmu_a3.z);
    }
    return khmu_c;
}
float KhMatLayer(int slot)
{
    if (slot == 0) return khMatLay0.x;
    if (slot == 1) return khMatLay0.y;
    if (slot == 2) return khMatLay0.z;
    if (slot == 3) return khMatLay0.w;
    if (slot == 4) return khMatLay1.x;
    return khMatLay1.y;
}

float4 KhMatFetch(int slot, float2 uv)
{
    float3 khmf_c = float3(uv, KhMatLayer(slot));
    if (slot == 0) return matDiffuse.Sample(matSamp, khmf_c);
    if (slot == 1) return matNormal.Sample(matSamp, khmf_c);
    if (slot == 2) return matOrm.Sample(matSamp, khmf_c);
    if (slot == 3) return matEmissive.Sample(matSamp, khmf_c);
    if (slot == 4) return matSpecular.Sample(matSamp, khmf_c);
    return matSpecColor.Sample(matSamp, khmf_c);
}

#if KH_USER_MAT
// KH_USER_TEX: a user material's own maps (the texture slots user0..user5), pages bound per draw at t43-t48 by
// kh_bind_material (null for every other material). See the USER SHADER CONTRACT. The selects are ?: chains, not
// flow, so a filtered sample never sits in divergent control flow; with a literal index fxc drops the other five.
Texture2DArray<float4> khUserMap0 : register(t43);
Texture2DArray<float4> khUserMap1 : register(t44);
Texture2DArray<float4> khUserMap2 : register(t45);
Texture2DArray<float4> khUserMap3 : register(t46);
Texture2DArray<float4> khUserMap4 : register(t47);
Texture2DArray<float4> khUserMap5 : register(t48);
float KhUserTexLayer(int khut_i)
{
    return khut_i == 0 ? khMatLay1.z : khut_i == 1 ? khMatLay1.w
         : khut_i == 2 ? khMatLay2.x : khut_i == 3 ? khMatLay2.y
         : khut_i == 4 ? khMatLay2.z : khMatLay2.w;
}
bool KhUserTexValid(int khut_i)
{
    return khut_i >= 0 && khut_i < 6 && ((((int)matParams0.x) >> (6 + khut_i)) & 1) != 0;
}
float4 KhUserTex(int khut_i, float2 khut_uv)
{
    const float3 khut_c = float3(khut_uv, KhUserTexLayer(khut_i));
    return khut_i == 0 ? khUserMap0.Sample(matSamp, khut_c)
         : khut_i == 1 ? khUserMap1.Sample(matSamp, khut_c)
         : khut_i == 2 ? khUserMap2.Sample(matSamp, khut_c)
         : khut_i == 3 ? khUserMap3.Sample(matSamp, khut_c)
         : khut_i == 4 ? khUserMap4.Sample(matSamp, khut_c)
                       : khUserMap5.Sample(matSamp, khut_c);
}
float4 KhUserTexLod(int khut_i, float2 khut_uv, float khut_lod)
{
    const float3 khut_c = float3(khut_uv, KhUserTexLayer(khut_i));
    return khut_i == 0 ? khUserMap0.SampleLevel(matSamp, khut_c, khut_lod)
         : khut_i == 1 ? khUserMap1.SampleLevel(matSamp, khut_c, khut_lod)
         : khut_i == 2 ? khUserMap2.SampleLevel(matSamp, khut_c, khut_lod)
         : khut_i == 3 ? khUserMap3.SampleLevel(matSamp, khut_c, khut_lod)
         : khut_i == 4 ? khUserMap4.SampleLevel(matSamp, khut_c, khut_lod)
                       : khUserMap5.SampleLevel(matSamp, khut_c, khut_lod);
}
float4 KhUserTexGrad(int khut_i, float2 khut_uv, float2 khut_dx, float2 khut_dy)
{
    const float3 khut_c = float3(khut_uv, KhUserTexLayer(khut_i));
    return khut_i == 0 ? khUserMap0.SampleGrad(matSamp, khut_c, khut_dx, khut_dy)
         : khut_i == 1 ? khUserMap1.SampleGrad(matSamp, khut_c, khut_dx, khut_dy)
         : khut_i == 2 ? khUserMap2.SampleGrad(matSamp, khut_c, khut_dx, khut_dy)
         : khut_i == 3 ? khUserMap3.SampleGrad(matSamp, khut_c, khut_dx, khut_dy)
         : khut_i == 4 ? khUserMap4.SampleGrad(matSamp, khut_c, khut_dx, khut_dy)
                       : khUserMap5.SampleGrad(matSamp, khut_c, khut_dx, khut_dy);
}
float2 KhUserTexSize(int khut_i)
{
    if (!KhUserTexValid(khut_i)) return float2(0.0f, 0.0f);
    uint khut_w = 0, khut_h = 0, khut_n = 0;
    if (khut_i == 0)      khUserMap0.GetDimensions(khut_w, khut_h, khut_n);
    else if (khut_i == 1) khUserMap1.GetDimensions(khut_w, khut_h, khut_n);
    else if (khut_i == 2) khUserMap2.GetDimensions(khut_w, khut_h, khut_n);
    else if (khut_i == 3) khUserMap3.GetDimensions(khut_w, khut_h, khut_n);
    else if (khut_i == 4) khUserMap4.GetDimensions(khut_w, khut_h, khut_n);
    else                  khUserMap5.GetDimensions(khut_w, khut_h, khut_n);
    return float2((float)khut_w, (float)khut_h);   // A page holds maps of one size.
}
// KH_USER_SLOTS: the material's six maps and the script's params, as the table holds them for this pixel (KhMatLoad
// runs before KhUserShade). See the USER SHADER CONTRACT. KhUserMatHas: i = KhMatFetch's slot (0 diffuse, 1 normal,
// 2 orm / as, 3 emissive, 4 specular / smdi, 5 speccolor).
bool   KhUserMatHas(int khum_i)     { return khum_i >= 0 && khum_i < 6 && ((((int)matParams0.x) >> khum_i) & 1) != 0; }
float3 KhUserMatBaseColor()         { return matParams1.xyz; }
float  KhUserMatRoughness()         { return matParams1.w; }
float  KhUserMatMetalness()         { return matParams2.x; }
float  KhUserMatEmissiveIntensity() { return matParams2.y; }
float  KhUserMatNormalStrength()    { return matParams0.w; }
float  KhUserMatCutoff()            { return matParams0.z; }
int    KhUserMatAlphaMode()         { return (int)matParams0.y; }   // 0 opaque, 1 cutout, 2 blend, 3 = a blend material's opaque part.
float3 KhUserMatSpecularColor()     { return matParams4.xyz; }
float  KhUserMatGlossiness()        { return matParams4.w; }
float2 KhUserMatFresnel()           { return matParams5.xy; }       // (N, K).
#endif

float KhMatRoute(float route, float fallback, float2 uv)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatFetch(r >> 2, uv);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

// The blend split classifies by the texel's own alpha: a point fetch at mip 0 through the same route, never the
// filtered sample. The filtered alpha averages a transparent neighbour into an opaque texel wherever the footprint
// touches the window (the rim, wider the coarser the mip), so such texels would leave the opaque pass and the part
// pass could not cover them - bites through a solid hull. (A uv seam picks no coarse mip: KH_MAT_GRAD's gradients
// are the triangle's own, taken before KH_UV_ANIM's cell wrap.)
// The texel under t = frac(uv) of a w x h level. frac of a uv a hair below zero rounds to exactly 1.0 in fp32, one past
// the last texel, where a Load returns zero (alpha 0: a solid texel read as clear); the index stops at the last one.
int2 KhMatTexelIx(float2 t, uint w, uint h)
{
    return min(int2(t * float2(w, h)), int2((int)w - 1, (int)h - 1));
}
float4 KhMatFetchTexel(int slot, float2 uv)
{
    uint kmt_w, kmt_h, kmt_n;
    float2 kmt_t = frac(uv);   // The sampler wraps; so does this.
    int kmt_l = (int)KhMatLayer(slot);
    if (slot == 0) { matDiffuse.GetDimensions(kmt_w, kmt_h, kmt_n);  return matDiffuse.Load(int4(KhMatTexelIx(kmt_t, kmt_w, kmt_h), kmt_l, 0)); }
    if (slot == 1) { matNormal.GetDimensions(kmt_w, kmt_h, kmt_n);   return matNormal.Load(int4(KhMatTexelIx(kmt_t, kmt_w, kmt_h), kmt_l, 0)); }
    if (slot == 2) { matOrm.GetDimensions(kmt_w, kmt_h, kmt_n);      return matOrm.Load(int4(KhMatTexelIx(kmt_t, kmt_w, kmt_h), kmt_l, 0)); }
    if (slot == 3) { matEmissive.GetDimensions(kmt_w, kmt_h, kmt_n); return matEmissive.Load(int4(KhMatTexelIx(kmt_t, kmt_w, kmt_h), kmt_l, 0)); }
    if (slot == 4) { matSpecular.GetDimensions(kmt_w, kmt_h, kmt_n); return matSpecular.Load(int4(KhMatTexelIx(kmt_t, kmt_w, kmt_h), kmt_l, 0)); }
    matSpecColor.GetDimensions(kmt_w, kmt_h, kmt_n);
    return matSpecColor.Load(int4(KhMatTexelIx(kmt_t, kmt_w, kmt_h), kmt_l, 0));
}

// KH_MAT_GRAD: KhMatFetch / KhMatRoute with the uv gradients handed in - SampleGrad, a filtered read that is legal
// in divergent flow (as SampleLevel is) and keeps the implicit sample's footprint, so the slot chain below is a real
// branch: one fetch per call, not KhMatFetch's six (fxc hoists every arm of a chain of implicit-derivative samples).
// The builtin casters' alpha reads (PSSunDepthA, PSInjDepthA, PSDlsMaskA) take their ddx / ddy of the uv at entry, in
// uniform flow. KhMatFetch and KhMatRoute stay as they are: they are the user contract's.
float4 KhMatFetchG(int slot, float2 uv, float2 dx, float2 dy)
{
    const float3 khmg_c = float3(uv, KhMatLayer(slot));
    float4 khmg_r;   // One exit (fxc's control-path rule), each arm a real branch.
    [branch] switch (slot) {
        case 0:  khmg_r = matDiffuse.SampleGrad(matSamp, khmg_c, dx, dy);   break;
        case 1:  khmg_r = matNormal.SampleGrad(matSamp, khmg_c, dx, dy);    break;
        case 2:  khmg_r = matOrm.SampleGrad(matSamp, khmg_c, dx, dy);       break;
        case 3:  khmg_r = matEmissive.SampleGrad(matSamp, khmg_c, dx, dy);  break;
        case 4:  khmg_r = matSpecular.SampleGrad(matSamp, khmg_c, dx, dy);  break;
        default: khmg_r = matSpecColor.SampleGrad(matSamp, khmg_c, dx, dy); break;
    }
    return khmg_r;
}
float KhMatRouteG(float route, float fallback, float2 uv, float2 dx, float2 dy)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatFetchG(r >> 2, uv, dx, dy);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

float KhMatRouteTexel(float route, float fallback, float2 uv)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatFetchTexel(r >> 2, uv);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

// The matParams / layer lanes are per-pixel to the compiler (KhMatLoad fills them from khMats[matIx], an
// interpolant it cannot prove uniform), so every branch on them is divergent flow as far as fxc knows, and an
// implicit-derivative filtered sample has no derivative there: fxc hoists every arm of such a slot chain (one
// fetch per slot per KhMatFetch call, ~29 per textured pixel measured with five slots). KhSampleMat therefore
// takes the uv's gradients once, in its caller's flow, and fetches with SampleGrad under real branches - only the
// slots this material reads (KhMatNeed), each once (KH_MAT_GRAD). KhMatFetchTexel's chain is Loads (no derivative);
// KhMatFetch / KhMatRoute are the user contract's.
struct KhMatTaps { float4 t0; float4 t1; float4 t2; float4 t3; float4 t4; float4 t5; };

// Slot select over a taken tap set - a chain of movc, no fetch, no flow. Slot numbering is KhMatFetch's.
float4 KhMatPick(KhMatTaps khmp, int slot)
{
    return slot == 0 ? khmp.t0
         : slot == 1 ? khmp.t1
         : slot == 2 ? khmp.t2
         : slot == 3 ? khmp.t3
         : slot == 4 ? khmp.t4
                     : khmp.t5;
}

// KhMatRoute's decode over an already-taken tap set: same lane encoding (slot = r >> 2, channel = r & 3), same
// negative-is-unrouted fallback, no fetch.
float KhMatRouteTap(KhMatTaps khmr, float route, float fallback)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatPick(khmr, r >> 2);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

struct KhMatSurf {
    float3 albedo; float alpha; float3 nrmT; float occ; float rough;
    float metal; float3 emissive; float3 specF0; float gloss; float specOn;
    // The arma model (model 1; 0 for pbr and user materials): the specular intensity x SPECCOLOR x specularColor,
    // and the fresnel (N, K). Filled for a user material too (KH_USER_SLOTS), which may set model itself.
    float model; float3 specTint; float2 fresnelNK;
    // KH_SPEC_AA: the GGX alpha^2 already added to rough and gloss (KhMatWiden) - the normal map's filtering
    // (KhToksvigA2, in KhSampleMatG) and, in the builtin's s, the shading normal's screen-space variance.
    float aaA2;
};

// KH_SPEC_AA: a roughness widened by a2add in GGX alpha^2 (alpha = rough^2): (rough^4 + a2add)^(1/4), held to
// [0, 1] (KhApplyPBR's saturate); a2add <= 0 returns it as it is. Pure.
float KhRoughWiden(float khrw_r, float khrw_a2)
{
    const float khrw_s = saturate(khrw_r);
    const float khrw_q = khrw_s * khrw_s;
    return khrw_a2 > 0.0f ? sqrt(sqrt(saturate(khrw_q * khrw_q + khrw_a2))) : khrw_r;
}
// KH_SPEC_AA: s with a2add more alpha^2 on both lanes a shading reads - rough, and gloss as 1 - rough (KH_GLOSS_ROUGH)
// - so the widening holds whichever workflow shades; aaA2 keeps the sum.
KhMatSurf KhMatWiden(KhMatSurf s, float a2add)
{
    s.rough = KhRoughWiden(s.rough, a2add);
    s.gloss = a2add > 0.0f ? 1.0f - KhRoughWiden(1.0f - s.gloss, a2add) : s.gloss;
    s.aaA2 += max(a2add, 0.0f);
    return s;
}
// KH_SPEC_AA (geometric specular anti-aliasing; Kaplanyan et al. 2016, Tokuyoshi and Kaplanyan 2019 / 2021, JCGT
// 10(2) Listing 5): the alpha^2 that the variance of the normal n over the pixel's footprint adds, from its screen
// derivatives - 2 sigma^2 (|ddx n|^2 + |ddy n|^2), sigma^2 = 1 / (2 pi) the pixel filter's variance in pixels (a
// Gaussian of peak 1: their choice over 0.25, closer to a box and less over-blurred), clamped at kappa = 0.18
// (theirs: the derivative estimate's error past it). A highlight whose normal turns faster than the pixel grid
// samples it (curvature at range, a normal map's detail near a pixel) widens until the pixel's samples agree instead
// of sparkling. ddx / ddy: call in uniform control flow (a quad's four pixels all reach it).
float KhSpecAAKernel(float3 khsa_n)
{
    const float3 khsa_dx = ddx(khsa_n);
    const float3 khsa_dy = ddy(khsa_n);
    return min(2.0f * 0.15915494f * (dot(khsa_dx, khsa_dx) + dot(khsa_dy, khsa_dy)), 0.18f);
}
KhMatSurf KhSpecAA(KhMatSurf s, float3 n) { return KhMatWiden(s, KhSpecAAKernel(n)); }

// One tap set for the whole surface; the flag guards and the six routes are selects over it. The guards decide what
// an absent map means (white diffuse, flat normal, no emissive, no specular), not whether a fetch happens.
// KH_MAT_GRAD: the slots a material reads - bit k = slot k. Its flags' maps (1 diffuse, 2 normal, 8 emissive,
// 16 specular, 32 specular colour; the orm map is read through routes only) and every route's slot (a route is
// slot * 4 + channel, < 0 = unrouted). A slot outside the mask is not fetched and reads zero, which no reader
// sees: every consumer below is gated by the same flag or route that puts the slot in the mask.
uint KhMatNeed()
{
    const uint khmn_f = (uint)(int)matParams0.x;
    uint khmn_m = khmn_f & 59u;   // Flag bits 1 / 2 / 8 / 16 / 32 are slots 0 / 1 / 3 / 4 / 5's own bits.
    const float khmn_r[6] = { matParams3.y, matParams2.z, matParams2.w, matParams3.x, matParams3.z, matParams5.z };
    [unroll] for (int khmn_i = 0; khmn_i < 6; ++khmn_i) {
        const int khmn_s = (int)khmn_r[khmn_i];
        if (khmn_s >= 0) khmn_m |= 1u << (uint)min(khmn_s >> 2, 5);
    }
    return khmn_m;
}

// KH_SPEC_AA (Toksvig 2005, "Mipmapping Normal Maps"): the GGX alpha^2 the normal map's own filtering adds. A
// filtered read of unit normals is shorter the more they spread: variance (1 - L) / L for its length L, alpha^2 = 2 x
// that (the same 2 sigma^2 as KhSpecAAKernel). The mips are box averages not renormalised (kh_tex_build_mips), so L
// holds it - armed by p0.x bit 15 for a png / tga / bmp map alone (kh_mat_gpu_fill, by extension: a .dds / .paa's own
// levels may be renormalised or block-compressed, and the levels KH_DDS_MIP_FILL builds below them are not told
// apart). khtk_n is the decoded texel as read (t1 * 2 - 1), before
// normalStrength and KhUvNrm change its length; their effect on the spread scales the variance instead (s^2, and
// |M|_F^2 / 2 for KhUvNrm's M = A^T / sqrt|det A|). KH_TOKSVIG_Q = 1 / 128 of the deficit is 8-bit quantisation's (up
// to sqrt(3) / 255), not spread; the deficit counts as far as the read is minified (saturate(lod), the level the 16x
// anisotropic sampler reads - KH_ANISO_16). A jpg is not armed (its error passes 1 / 128): prefer png normal maps.
static const float KH_TOKSVIG_Q = 0.0078125f;
float KhToksvigA2(float3 khtk_n, float2 khtk_dx, float2 khtk_dy)
{
    uint khtk_w, khtk_h, khtk_e;
    matNormal.GetDimensions(khtk_w, khtk_h, khtk_e);
    const float2 khtk_wh = float2((float)khtk_w, (float)khtk_h);
    const float2 khtk_ex = khtk_dx * khtk_wh;
    const float2 khtk_ey = khtk_dy * khtk_wh;
    const float khtk_px = dot(khtk_ex, khtk_ex);
    const float khtk_py = dot(khtk_ey, khtk_ey);
    const float khtk_lod = 0.5f * log2(max(min(khtk_px, khtk_py), max(khtk_px, khtk_py) * (1.0f / 256.0f)));   // 16^2.
    const float khtk_len = length(khtk_n);
    const float khtk_var = max(1.0f - khtk_len - KH_TOKSVIG_Q, 0.0f) / max(khtk_len, 1.0e-4f);
    float khtk_k2 = matParams0.w * matParams0.w;   // normalStrength scales the slopes.
    [branch] if (((int)matParams0.x) & 4096) {   // KH_UV_ANIM: KhUvNrm's matrix (KhSampleMatG's).
        const float4 khtk_a = khMats[khMatIx].an0;
        khtk_k2 *= 0.5f * dot(khtk_a, khtk_a) / max(abs(khtk_a.x * khtk_a.w - khtk_a.y * khtk_a.z), 1.0e-12f);
    }
    return 2.0f * khtk_var * khtk_k2 * saturate(khtk_lod);
}

// KH_UV_ANIM: the body takes the gradients from its caller (KhMatUv's, for the builtin readers); uv is in the
// material's animated space, so a turned or mirrored material's normal map is turned back by KhUvNrm (bit 12).
// KH_SPEC_AA: rough and gloss arrive widened by the map's filtering (KhToksvigA2; aaA2 holds it).
KhMatSurf KhSampleMatG(float2 uv, float2 khsm_dx, float2 khsm_dy)
{
    KhMatSurf s;
    const uint khsm_need = KhMatNeed();
    KhMatTaps khsm_m = (KhMatTaps)0;
    [branch] if (khsm_need & 1u)  khsm_m.t0 = matDiffuse.SampleGrad(matSamp, float3(uv, khMatLay0.x), khsm_dx, khsm_dy);
    [branch] if (khsm_need & 2u)  khsm_m.t1 = matNormal.SampleGrad(matSamp, float3(uv, khMatLay0.y), khsm_dx, khsm_dy);
    [branch] if (khsm_need & 4u)  khsm_m.t2 = matOrm.SampleGrad(matSamp, float3(uv, khMatLay0.z), khsm_dx, khsm_dy);
    [branch] if (khsm_need & 8u)  khsm_m.t3 = matEmissive.SampleGrad(matSamp, float3(uv, khMatLay0.w), khsm_dx, khsm_dy);
    [branch] if (khsm_need & 16u) khsm_m.t4 = matSpecular.SampleGrad(matSamp, float3(uv, khMatLay1.x), khsm_dx, khsm_dy);
    [branch] if (khsm_need & 32u) khsm_m.t5 = matSpecColor.SampleGrad(matSamp, float3(uv, khMatLay1.y), khsm_dx, khsm_dy);
    int flags = (int)matParams0.x;
    float4 dif = (flags & 1) ? khsm_m.t0 : float4(1.0f, 1.0f, 1.0f, 1.0f);
    s.albedo = dif.rgb * matParams1.xyz;
    s.alpha = KhMatRouteTap(khsm_m, matParams3.y, 1.0f);
    // A normal map is read DirectX-style - green pointing down the picture, as Arma's own _nohq - and decoded as
    // stored: an OpenGL-style map (Blender's bakes, Substance's "OpenGL" export) needs its green inverted by the
    // author. Do not flip it here: every _nohq would read its bumps as dents along that axis.
    s.nrmT = (flags & 2) ? (khsm_m.t1.xyz * 2.0f - 1.0f) : float3(0.0f, 0.0f, 1.0f);
    float khsm_tk = 0.0f;   // KH_SPEC_AA: the map's Toksvig term, from the texel as decoded (before the two below).
    [branch] if ((flags & 32770) == 32770) khsm_tk = KhToksvigA2(s.nrmT, khsm_dx, khsm_dy);   // Bit 15: armed.
    [branch] if (flags & 4096) s.nrmT.xy = KhUvNrm(s.nrmT.xy, khMats[khMatIx].an0);   // KH_UV_ANIM (not the warp).
    s.nrmT.xy *= matParams0.w;
    s.occ = KhMatRouteTap(khsm_m, matParams2.z, 1.0f);
    s.rough = KhMatRouteTap(khsm_m, matParams2.w, matParams1.w);
    s.metal = KhMatRouteTap(khsm_m, matParams3.x, matParams2.x);
    s.emissive = ((flags & 8) ? khsm_m.t3.rgb : float3(0.0f, 0.0f, 0.0f)) * matParams2.y;
    s.specOn = matParams3.w;
    float4 spc = (flags & 16) ? khsm_m.t4 : float4(0.0f, 0.0f, 0.0f, 0.0f);
    s.specF0 = spc.rgb;
    // The arma model's routes default, C++ side, to AS.g (occ above), SMDI.b (gloss) and SMDI.g (specular);
    // unrouted, gloss falls back to the glossiness param and specular to 1. Its spec workflow lane and metalness are
    // zero and its metal route is -1 (unrouted), so metal reads 0 above.
    s.model = matParams5.w;
    s.gloss = KhMatRouteTap(khsm_m, matParams3.z, s.model >= 0.5f ? matParams4.w : spc.a);
    const float3 khsm_sc = (flags & 32) ? khsm_m.t5.rgb : float3(1.0f, 1.0f, 1.0f);
    s.specTint = KhMatRouteTap(khsm_m, matParams5.z, 1.0f) * khsm_sc * matParams4.xyz;
    s.fresnelNK = matParams5.xy;
    s.aaA2 = 0.0f;
    return KhMatWiden(s, khsm_tk);   // KH_SPEC_AA.
}
// The user contract's: the gradients of uv here (KH_MAT_GRAD: where the implicit sample took them), so call it in
// uniform flow. uv is a coordinate in the material's animated space (KhUserUv()'s).
KhMatSurf KhSampleMat(float2 uv)
{
    return KhSampleMatG(uv, ddx(uv), ddy(uv));
}

// KH_CUTOUT_A2C - a cutout material's coverage in the colour twins (PSMain / PSComposite; their SV_Coverage),
// called for a cutout pixel alone (matParams0.y in [0.5, 1.5)) with the routed alpha khcc_a. A single-sample
// target, or a draw that asks for the hard edge (blendCtl.z: the view-model slice, whose depth prepass clips at
// the cutoff and culls the world under it - a sample its colour then left out would show no world), keeps the
// clip at the cutoff, at full coverage. A multisampled one takes alpha to coverage on a sharpened alpha (Golus
// 2017, "Anti-aliased Alpha Test"): s = (a - cutoff) / fwidth(a) + 0.5, held to [0, 1] - the edge one pixel wide,
// centred on the cutoff - and covers round(s x n) of the n samples, the low ones (sample 0 first, so a pixel with
// any sample of ours is ours at sample 0 - the SSAO term's test); none = discarded. The mask is a function of the
// alpha and its screen gradient alone: no dither, no clock, steady on a still view. The hardware ANDs it with the
// raster coverage; the depth, the stencil mark (KH_SSAO_MARK) and both targets follow it per sample, and the
// written alpha stays the opaque contract's 1. fwidth is taken inside the cutout branch: its condition is a
// material lane, one value per draw or per instance - uniform in a quad. The casters, the seam footprint, the dlsw
// mask and the slice prepass keep their clip.
uint KhCutoutCover(float khcc_a)
{
    const float khcc_d = khcc_a - matParams0.z;
    const uint khcc_n = GetRenderTargetSampleCount();
    if (khcc_n <= 1u || blendCtl.z >= 0.5f) {
        clip(khcc_d);   // The hard edge.
        return 0xFFFFFFFFu;
    }
    const float khcc_s = saturate(khcc_d / max(fwidth(khcc_a), 1.0e-4f) + 0.5f);
    const uint khcc_k = min((uint)(khcc_s * (float)khcc_n + 0.5f), khcc_n);
    if (khcc_k == 0u) discard;   // No sample: killed before the shading, as the clip is.
    return khcc_k >= khcc_n ? 0xFFFFFFFFu : ((1u << khcc_k) - 1u);
}

// The arma model's Fresnel: the reflectance of a conductor of complex index N + iK at incidence cos = khfn_c, the
// mean of the s and p polarisations - the function Arma's procedural "fresnel(N,K)" texture tabulates, term for term
// (AA, BB, FS, FP with sin S tan S = sin^2 S / cos S). 1 at grazing for every N, K but N = 1, K = 0 (no interface:
// 0, except at the cosine floor below, where fp32 rounds 1 - c^2 to 1 and it reads about 0.75);
// ((N-1)^2+K^2)/((N+1)^2+K^2) head-on. The cosine is floored so tan S stays finite at grazing, and AA^2 + BB^2 so
// that N = K = 0 head-on (0/0 in the original, a NaN in TexView) reads its limit, 1.
float KhFresnelNK(float khfn_c, float khfn_n, float khfn_k)
{
    const float c = clamp(khfn_c, 1.0e-4f, 1.0f);
    const float s2 = 1.0f - c * c;
    const float n2 = khfn_n * khfn_n;
    const float k2 = khfn_k * khfn_k;
    const float t0 = n2 - k2 - s2;
    const float ab2 = max(sqrt(t0 * t0 + 4.0f * n2 * k2), 1.0e-10f);   // AA^2 + BB^2.
    const float a = sqrt(max(0.5f * (ab2 + t0), 0.0f));  // AA.
    const float fs = (ab2 - 2.0f * a * c + c * c) / max(ab2 + 2.0f * a * c + c * c, 1.0e-12f);
    const float st = s2 / c;   // sin S tan S.
    const float fp = fs * (ab2 - 2.0f * a * st + st * st) / max(ab2 + 2.0f * a * st + st * st, 1.0e-12f);
    return saturate(0.5f * (fs + fp));
}

// This pixel's Fresnel form: x = N, y = K, z >= 0.5 = the arma model's conductor curve, which KhApplyPBR arms for
// its surface. Unarmed, every reader keeps Schlick on F0.
static float4 khFrNK = 0.0f;

// Shared GGX core (Cook-Torrance D * G * F / (4 ndv ndl)) for the sun term (KhApplyPBR) and the engine dynamic
// lights (KhDynLightsPBR). Armed (khFrNK), F0 is the arma model's specular tint and F the tint times the conductor
// curve at V.H. A branch, not a ternary: fxc evaluates both sides of ?:, and this runs once per lobe per light, so
// the other kinds would pay for the curve on every one; khFrNK is per material, so the branch is coherent.
// KH_PBR_PI: every light colour here is pi-folded - the colour C a white Lambert surface facing it returns
// (diffuse = albedo * C * N.L, i.e. the BRDF albedo / pi under an irradiance of pi * C * N.L). KhGGXSpec is the
// specular BRDF itself (D G F / (4 N.L N.V), D carrying its 1 / pi), so a punctual light's lobe is
// KhGGXSpec * pi * C * N.L - the same irradiance the diffuse takes. The split-sum ambient (KhPbrAmbient) reads
// radiance through the environment BRDF and takes no pi.
static const float KH_PBR_PI = 3.14159265f;
// KH_GGX_EXACT: D is the GGX (Trowbridge-Reitz) distribution a2 / (pi dd^2), dd = 1 - (N.H)^2 (1 - a2), a2 = rough^4,
// exact for every roughness a caller can pass: rough is held to KH_ROUGH_MIN here (KhApplyPBR's floor, the env-BRDF
// table's first row), so a user's KhDynLightsPBR / KhGGXSpec call below it shades as 0.045 does. 1 - (N.H)^2 is
// taken as |N x H|^2: at 0.045 the lobe's centre lies within 1 - (N.H)^2 < a2 = 4.1e-6, about 70 fp32 ulps of 1,
// where 1 - ndh * ndh cancels (up to 22 % error in D there, measured); the cross product carries none. A half vector
// at or below the surface keeps dd = 1 (D = a2 / pi). The floor below cannot bite for unit N and H: dd >= a2 >=
// 4.1e-6, so pi dd^2 >= 5.3e-11. Do not floor pi dd^2 at 1e-6: that caps D at a2 x 1e6 and cuts the lobe for every
// roughness below 0.154 (1.45 % of the energy kept at 0.045, 32 % at 0.1 - a dim, wide sun / lamp highlight on
// glossy surfaces instead of a bright, tight one).
static const float KH_ROUGH_MIN = 0.045f;
static const float KH_SUN_DISC_A = 0.00155f;   // KH_SUN_DISC: tan(0.2665 deg) / 3, the sun's alpha term (KhApplyPBR).
// KH_DL_RADIUS: a lamp is an emitter of this radius (m), not a point - the same rule as the sun's disc (Karis 2013
// eq. 10, alpha' = saturate(alpha + radius / (3 distance)), D normalised, so the highlight keeps its energy and
// spreads over the emitter's reflection). The engine's light records carry no size, so one radius stands for every
// lamp: 0.05 m (a bulb, a headlight's reflector, a torch head). At 10 m it widens as the sun does (0.00167); at 1 m
// a 0.045 surface's peak falls to ~1 % of a point's, at 100 m to 0.85 (its glint still saturates the 16F scene;
// only lamps within a few metres soften visibly). KhDynLightsPBRE's lobe alone; the per-light ambient and the
// diffuse take no size. A user lobe shading a lamp of its own may widen the same way:
// sqrt(min(rough * rough + KH_DL_RADIUS_M / (3 distance), 1)).
static const float KH_DL_RADIUS_M = 0.05f;
float3 KhGGXSpec(float3 n, float3 v, float3 l, float rough, float3 F0, out float3 outF)
{
    float3 h = normalize(l + v);
    float ndl = saturate(dot(n, l));
    float ndv = max(dot(n, v), 1.0e-4f);
    float ndh = saturate(dot(n, h));
    float vdh = saturate(dot(v, h));
    rough = max(rough, KH_ROUGH_MIN);   // KH_GGX_EXACT.
    float aa = rough * rough;
    float a2 = aa * aa;
    const float3 khgx_c = cross(n, h);   // KH_GGX_EXACT: |N x H|^2 = 1 - (N.H)^2 without the cancellation.
    float dd = (ndh > 0.0f ? dot(khgx_c, khgx_c) : 1.0f) + ndh * ndh * a2;
    float D = a2 / max(3.14159265f * dd * dd, 1.0e-20f);
    // KH_SMITH_HC: the masking-shadowing is the exact height-correlated Smith G2 of GGX, 1 / (1 + L(v) + L(l)),
    // L(c) = (sqrt(1 + a2 (1 - c^2) / c^2) - 1) / 2 (Heitz 2014; the Smith microsurface's own, which the random walk
    // of Heitz 2016 reproduces; Filament's V_SmithGGXCorrelated), folded with the 1 / (4 N.V N.L) into one exact
    // visibility term. The env-BRDF table integrates the same G2 (KH_ENV_LUT). N.V and N.L are floored at 1e-4 and
    // each root is at least alpha >= 0.002, so the denominator is 4e-7 or more.
    float gl = max(ndl, 1.0e-4f);
    float khgx_vis = 0.5f / (gl * sqrt(ndv * ndv * (1.0f - a2) + a2) + ndv * sqrt(gl * gl * (1.0f - a2) + a2));
    [branch] if (khFrNK.z >= 0.5f) {
        outF = F0 * KhFresnelNK(vdh, khFrNK.x, khFrNK.y);
    } else {
        outF = F0 + (1.0f - F0) * pow(1.0f - vdh, 5.0f);
    }
    return D * khgx_vis * outF;   // KH_SMITH_HC.
}

// KH_ENV_LUT / KH_ENV_F82 / KH_DOME_LUT: the split-sum's precomputed integrals, as immediate constant buffers
// (static const arrays: fxc and DXC place them in the shader itself - no slot, no resource, no C++). Generated
// offline by kh_gen_lut.cpp (a standalone tool, kh_dq_tools.zip dq/lut/, not part of the build; 2048^2 / 512^2
// stratified samples an env / dome node, integration error <= 2.4e-4 / 1.3e-4).
// Grids (the generator's constants - change both together):
//   roughness  17 nodes, rough = 0.045 + 0.955 t^2, t = j / 16 (denser where the lobe narrows); below 0.045
//              (KhApplyPBR's floor) the first row holds.
//   KH_ENVBRDF_LUT  Karis 2013's IntegrateBRDF (F = F0 A + B; GGX D with alpha = rough^2, the shader's) with the
//              height-correlated Smith G2 = 1 / (1 + L(v) + L(l)), the direct lobes' (KH_SMITH_HC), 33 N.V nodes,
//              N.V = 0.004 + 0.996 t^2, t = i / 32; below N.V 0.004 the first column holds (as a texture LUT's
//              clamp does). Pair-packed: element [j * 32 + i] = (A, B at node i, A, B at node i + 1) of row j.
//              544 float4, 8704 bytes. Bilinear error against the integral <= 0.0070 (worst at a cell midpoint,
//              N.V 0.016, rough 0.068; <= 0.0059 for N.V >= 0.05).
//   KH_ENVBRDF_C  KH_ENV_F82: C, the integral of (v.h)(1 - v.h)^6 under the same G2 (Hoffman 2019's F82 term), on
//              the same grid. Corner-packed: element [j * 32 + i] = (C(i, j), C(i + 1, j), C(i, j + 1),
//              C(i + 1, j + 1)). 512 float4, 8192 bytes. Bilinear error <= 0.00055 (N.V 0.010, rough 0.091;
//              <= 0.00044 for N.V >= 0.05): <= 0.0045 of an arma E at the largest F82 weight (N 1, K 0).
//   KH_DOME_LUT  the dome's SKY SHARE prefiltered along R (Karis' prefilter, N = V = R, weights N.L) - the
//              weight, not radiance: KhPbrAmbient's dome is amb x lerp(KH_PBR_GROUND, 1, share), linear in the
//              share, so the table holds for any ambient colour and any KH_PBR_GROUND. It bakes the horizon,
//              share = smoothstep(-0.15, 0.15, l.y): REGENERATE (kh_gen_lut.cpp's kh_sstep) if those edges change.
//              49 R.y nodes, R.y = u |u|, u = -1 + i / 24 (denser at the horizon). Corner-packed: element
//              [j * 48 + i] = (P(i, j), P(i + 1, j), P(i, j + 1), P(i + 1, j + 1)). 768 float4, 12288 bytes.
//              Bilinear error <= 0.0057 (worst at a cell midpoint, R.y -0.125, rough 0.068): <= 0.0037 of the
//              ambient after the (1 - KH_PBR_GROUND) = 0.65 scale.
//   All three: 1824 float4 (29184 bytes) of the 4096-float4 immediate constant buffer; a shader that reads none
//   keeps none.
static const float KH_LUT_RMIN = 0.045f;
static const float KH_ENVLUT_NMIN = 0.004f;
static const float4 KH_ENVBRDF_LUT[544] = {
    0.03331f, 0.87945f, 0.03578f, 0.89322f, 0.03578f, 0.89322f, 0.04624f, 0.91671f, 0.04624f, 0.91671f, 0.06695f,
    0.91793f, 0.06695f, 0.91793f, 0.09726f, 0.89649f, 0.09726f, 0.89649f, 0.13599f, 0.86114f, 0.13599f, 0.86114f,
    0.18199f, 0.81654f, 0.18199f, 0.81654f, 0.23407f, 0.76512f, 0.23407f, 0.76512f, 0.29098f, 0.70853f, 0.29098f,
    0.70853f, 0.35142f, 0.64829f, 0.35142f, 0.64829f, 0.41407f, 0.58572f, 0.41407f, 0.58572f, 0.47762f, 0.52225f,
    0.47762f, 0.52225f, 0.54080f, 0.45915f, 0.54080f, 0.45915f, 0.60238f, 0.39758f, 0.60238f, 0.39758f, 0.66129f,
    0.33868f, 0.66129f, 0.33868f, 0.71657f, 0.28341f, 0.71657f, 0.28341f, 0.76744f, 0.23255f, 0.76744f, 0.23255f,
    0.81327f, 0.18671f, 0.81327f, 0.18671f, 0.85369f, 0.14631f, 0.85369f, 0.14631f, 0.88847f, 0.11152f, 0.88847f,
    0.11152f, 0.91763f, 0.08237f, 0.91763f, 0.08237f, 0.94135f, 0.05864f, 0.94135f, 0.05864f, 0.96001f, 0.03998f,
    0.96001f, 0.03998f, 0.97412f, 0.02588f, 0.97412f, 0.02588f, 0.98428f, 0.01572f, 0.98428f, 0.01572f, 0.99119f,
    0.00881f, 0.99119f, 0.00881f, 0.99555f, 0.00445f, 0.99555f, 0.00445f, 0.99805f, 0.00195f, 0.99805f, 0.00195f,
    0.99931f, 0.00069f, 0.99931f, 0.00069f, 0.99982f, 0.00018f, 0.99982f, 0.00018f, 0.99997f, 0.00003f, 0.99997f,
    0.00003f, 1.00000f, 0.00000f, 1.00000f, 0.00000f, 1.00000f, 0.00000f, 0.03731f, 0.86565f, 0.03916f, 0.87772f,
    0.03916f, 0.87772f, 0.04850f, 0.90373f, 0.04850f, 0.90373f, 0.06842f, 0.91089f, 0.06842f, 0.91089f, 0.09825f,
    0.89303f, 0.09825f, 0.89303f, 0.13668f, 0.85930f, 0.13668f, 0.85930f, 0.18249f, 0.81547f, 0.18249f, 0.81547f,
    0.23443f, 0.76444f, 0.23443f, 0.76444f, 0.29124f, 0.70808f, 0.29124f, 0.70808f, 0.35160f, 0.64796f, 0.35160f,
    0.64796f, 0.41421f, 0.58552f, 0.41421f, 0.58552f, 0.47771f, 0.52209f, 0.47771f, 0.52209f, 0.54085f, 0.45902f,
    0.54085f, 0.45902f, 0.60243f, 0.39752f, 0.60243f, 0.39752f, 0.66133f, 0.33864f, 0.66133f, 0.33864f, 0.71659f,
    0.28338f, 0.71659f, 0.28338f, 0.76745f, 0.23253f, 0.76745f, 0.23253f, 0.81328f, 0.18671f, 0.81328f, 0.18671f,
    0.85368f, 0.14630f, 0.85368f, 0.14630f, 0.88847f, 0.11152f, 0.88847f, 0.11152f, 0.91762f, 0.08237f, 0.91762f,
    0.08237f, 0.94135f, 0.05865f, 0.94135f, 0.05865f, 0.96001f, 0.03999f, 0.96001f, 0.03999f, 0.97411f, 0.02588f,
    0.97411f, 0.02588f, 0.98428f, 0.01572f, 0.98428f, 0.01572f, 0.99119f, 0.00881f, 0.99119f, 0.00881f, 0.99555f,
    0.00445f, 0.99555f, 0.00445f, 0.99805f, 0.00195f, 0.99805f, 0.00195f, 0.99930f, 0.00069f, 0.99930f, 0.00069f,
    0.99982f, 0.00018f, 0.99982f, 0.00018f, 0.99997f, 0.00003f, 0.99997f, 0.00003f, 1.00000f, 0.00000f, 1.00000f,
    0.00000f, 1.00000f, 0.00000f, 0.05289f, 0.83857f, 0.05281f, 0.84193f, 0.05281f, 0.84193f, 0.05805f, 0.86240f,
    0.05805f, 0.86240f, 0.07460f, 0.88231f, 0.07460f, 0.88231f, 0.10239f, 0.87748f, 0.10239f, 0.87748f, 0.13958f,
    0.85087f, 0.13958f, 0.85087f, 0.18457f, 0.81055f, 0.18457f, 0.81055f, 0.23594f, 0.76136f, 0.23594f, 0.76136f,
    0.29234f, 0.70606f, 0.29234f, 0.70606f, 0.35241f, 0.64658f, 0.35241f, 0.64658f, 0.41479f, 0.58455f, 0.41479f,
    0.58455f, 0.47813f, 0.52142f, 0.47813f, 0.52142f, 0.54114f, 0.45855f, 0.54114f, 0.45855f, 0.60261f, 0.39716f,
    0.60261f, 0.39716f, 0.66143f, 0.33840f, 0.66143f, 0.33840f, 0.71665f, 0.28324f, 0.71665f, 0.28324f, 0.76749f,
    0.23246f, 0.76749f, 0.23246f, 0.81329f, 0.18667f, 0.81329f, 0.18667f, 0.85368f, 0.14629f, 0.85368f, 0.14629f,
    0.88845f, 0.11153f, 0.88845f, 0.11153f, 0.91760f, 0.08238f, 0.91760f, 0.08238f, 0.94133f, 0.05866f, 0.94133f,
    0.05866f, 0.95999f, 0.04000f, 0.95999f, 0.04000f, 0.97409f, 0.02590f, 0.97409f, 0.02590f, 0.98426f, 0.01573f,
    0.98426f, 0.01573f, 0.99118f, 0.00882f, 0.99118f, 0.00882f, 0.99554f, 0.00445f, 0.99554f, 0.00445f, 0.99805f,
    0.00195f, 0.99805f, 0.00195f, 0.99930f, 0.00070f, 0.99930f, 0.00070f, 0.99982f, 0.00018f, 0.99982f, 0.00018f,
    0.99997f, 0.00003f, 0.99997f, 0.00003f, 1.00000f, 0.00000f, 1.00000f, 0.00000f, 1.00000f, 0.00000f, 0.08826f,
    0.81767f, 0.08569f, 0.81143f, 0.08569f, 0.81143f, 0.08385f, 0.80894f, 0.08385f, 0.80894f, 0.09233f, 0.82356f,
    0.09233f, 0.82356f, 0.11427f, 0.83448f, 0.11427f, 0.83448f, 0.14783f, 0.82433f, 0.14783f, 0.82433f, 0.19049f,
    0.79449f, 0.19049f, 0.79449f, 0.24027f, 0.75129f, 0.24027f, 0.75129f, 0.29552f, 0.69946f, 0.29552f, 0.69946f,
    0.35475f, 0.64212f, 0.35475f, 0.64212f, 0.41648f, 0.58146f, 0.41648f, 0.58146f, 0.47934f, 0.51927f, 0.47934f,
    0.51927f, 0.54198f, 0.45704f, 0.54198f, 0.45704f, 0.60316f, 0.39612f, 0.60316f, 0.39612f, 0.66178f, 0.33770f,
    0.66178f, 0.33770f, 0.71683f, 0.28277f, 0.71683f, 0.28277f, 0.76754f, 0.23217f, 0.76754f, 0.23217f, 0.81326f,
    0.18650f, 0.81326f, 0.18650f, 0.85360f, 0.14621f, 0.85360f, 0.14621f, 0.88834f, 0.11151f, 0.88834f, 0.11151f,
    0.91751f, 0.08241f, 0.91751f, 0.08241f, 0.94125f, 0.05871f, 0.94125f, 0.05871f, 0.95991f, 0.04005f, 0.95991f,
    0.04005f, 0.97403f, 0.02594f, 0.97403f, 0.02594f, 0.98421f, 0.01577f, 0.98421f, 0.01577f, 0.99114f, 0.00885f,
    0.99114f, 0.00885f, 0.99551f, 0.00447f, 0.99551f, 0.00447f, 0.99803f, 0.00196f, 0.99803f, 0.00196f, 0.99929f,
    0.00070f, 0.99929f, 0.00070f, 0.99981f, 0.00018f, 0.99981f, 0.00018f, 0.99997f, 0.00003f, 0.99997f, 0.00003f,
    1.00000f, 0.00000f, 1.00000f, 0.00000f, 1.00000f, 0.00000f, 0.14819f, 0.78701f, 0.14431f, 0.77967f, 0.14431f,
    0.77967f, 0.13660f, 0.76466f, 0.13660f, 0.76466f, 0.13422f, 0.75735f, 0.13422f, 0.75735f, 0.14450f, 0.76170f,
    0.14450f, 0.76170f, 0.16897f, 0.76399f, 0.16897f, 0.76399f, 0.20548f, 0.75155f, 0.20548f, 0.75155f, 0.25115f,
    0.72235f, 0.25115f, 0.72235f, 0.30353f, 0.68005f, 0.30353f, 0.68005f, 0.36064f, 0.62891f, 0.36064f, 0.62891f,
    0.42078f, 0.57235f, 0.42078f, 0.57235f, 0.48241f, 0.51291f, 0.48241f, 0.51291f, 0.54411f, 0.45261f, 0.54411f,
    0.45261f, 0.60458f, 0.39306f, 0.60458f, 0.39306f, 0.66264f, 0.33562f, 0.66264f, 0.33562f, 0.71728f, 0.28141f,
    0.71728f, 0.28141f, 0.76768f, 0.23131f, 0.76768f, 0.23131f, 0.81321f, 0.18601f, 0.81321f, 0.18601f, 0.85341f,
    0.14598f, 0.85341f, 0.14598f, 0.88807f, 0.11144f, 0.88807f, 0.11144f, 0.91717f, 0.08243f, 0.91717f, 0.08243f,
    0.94091f, 0.05879f, 0.94091f, 0.05879f, 0.95958f, 0.04015f, 0.95958f, 0.04015f, 0.97373f, 0.02604f, 0.97373f,
    0.02604f, 0.98395f, 0.01586f, 0.98395f, 0.01586f, 0.99092f, 0.00892f, 0.99092f, 0.00892f, 0.99536f, 0.00452f,
    0.99536f, 0.00452f, 0.99796f, 0.00200f, 0.99796f, 0.00200f, 0.99925f, 0.00072f, 0.99925f, 0.00072f, 0.99979f,
    0.00019f, 0.99979f, 0.00019f, 0.99996f, 0.00003f, 0.99996f, 0.00003f, 0.99999f, 0.00000f, 0.99999f, 0.00000f,
    1.00000f, 0.00000f, 0.23047f, 0.72884f, 0.22673f, 0.72402f, 0.22673f, 0.72402f, 0.21742f, 0.71128f, 0.21742f,
    0.71128f, 0.20808f, 0.69617f, 0.20808f, 0.69617f, 0.20593f, 0.68554f, 0.20593f, 0.68554f, 0.21609f, 0.68057f,
    0.21609f, 0.68057f, 0.23993f, 0.67475f, 0.23993f, 0.67475f, 0.27593f, 0.66026f, 0.27593f, 0.66026f, 0.32136f,
    0.63352f, 0.32136f, 0.63352f, 0.37351f, 0.59531f, 0.37351f, 0.59531f, 0.43001f, 0.54844f, 0.43001f, 0.54844f,
    0.48890f, 0.49601f, 0.48890f, 0.49601f, 0.54850f, 0.44073f, 0.54850f, 0.44073f, 0.60735f, 0.38479f, 0.60735f,
    0.38479f, 0.66418f, 0.32995f, 0.66418f, 0.32995f, 0.71790f, 0.27763f, 0.71790f, 0.27763f, 0.76762f, 0.22889f,
    0.76762f, 0.22889f, 0.81267f, 0.18457f, 0.81267f, 0.18457f, 0.85256f, 0.14521f, 0.85256f, 0.14521f, 0.88704f,
    0.11112f, 0.88704f, 0.11112f, 0.91606f, 0.08240f, 0.91606f, 0.08240f, 0.93978f, 0.05892f, 0.93978f, 0.05892f,
    0.95852f, 0.04037f, 0.95852f, 0.04037f, 0.97275f, 0.02627f, 0.97275f, 0.02627f, 0.98309f, 0.01607f, 0.98309f,
    0.01607f, 0.99015f, 0.00908f, 0.99015f, 0.00908f, 0.99466f, 0.00464f, 0.99466f, 0.00464f, 0.99732f, 0.00207f,
    0.99732f, 0.00207f, 0.99868f, 0.00076f, 0.99868f, 0.00076f, 0.99926f, 0.00021f, 0.99926f, 0.00021f, 0.99945f,
    0.00004f, 0.99945f, 0.00004f, 0.99949f, 0.00000f, 0.99949f, 0.00000f, 0.99951f, 0.00000f, 0.32918f, 0.64534f,
    0.32623f, 0.64255f, 0.32623f, 0.64255f, 0.31828f, 0.63449f, 0.31828f, 0.63449f, 0.30791f, 0.62242f, 0.30791f,
    0.62242f, 0.29936f, 0.60874f, 0.29936f, 0.60874f, 0.29755f, 0.59604f, 0.29755f, 0.59604f, 0.30621f, 0.58486f,
    0.30621f, 0.58486f, 0.32686f, 0.57276f, 0.32686f, 0.57276f, 0.35891f, 0.55591f, 0.35891f, 0.55591f, 0.40040f,
    0.53145f, 0.40040f, 0.53145f, 0.44885f, 0.49865f, 0.44885f, 0.49865f, 0.50175f, 0.45859f, 0.50175f, 0.45859f,
    0.55691f, 0.41333f, 0.55691f, 0.41333f, 0.61244f, 0.36516f, 0.61244f, 0.36516f, 0.66679f, 0.31620f, 0.66679f,
    0.31620f, 0.71868f, 0.26825f, 0.71868f, 0.26825f, 0.76708f, 0.22273f, 0.76708f, 0.22273f, 0.81120f, 0.18074f,
    0.81120f, 0.18074f, 0.85051f, 0.14302f, 0.85051f, 0.14302f, 0.88466f, 0.11007f, 0.88466f, 0.11007f, 0.91357f,
    0.08208f, 0.91357f, 0.08208f, 0.93732f, 0.05903f, 0.93732f, 0.05903f, 0.95621f, 0.04071f, 0.95621f, 0.04071f,
    0.97066f, 0.02669f, 0.97066f, 0.02669f, 0.98123f, 0.01647f, 0.98123f, 0.01647f, 0.98856f, 0.00942f, 0.98856f,
    0.00942f, 0.99332f, 0.00489f, 0.99332f, 0.00489f, 0.99617f, 0.00223f, 0.99617f, 0.00223f, 0.99770f, 0.00085f,
    0.99770f, 0.00085f, 0.99844f, 0.00025f, 0.99844f, 0.00025f, 0.99874f, 0.00005f, 0.99874f, 0.00005f, 0.99892f,
    0.00001f, 0.99892f, 0.00001f, 0.99900f, 0.00000f, 0.43672f, 0.54674f, 0.43461f, 0.54503f, 0.43461f, 0.54503f,
    0.42865f, 0.53996f, 0.42865f, 0.53996f, 0.42010f, 0.53174f, 0.42010f, 0.53174f, 0.41097f, 0.52088f, 0.41097f,
    0.52088f, 0.40410f, 0.50826f, 0.40410f, 0.50826f, 0.40262f, 0.49476f, 0.40262f, 0.49476f, 0.40914f, 0.48063f,
    0.40914f, 0.48063f, 0.42519f, 0.46516f, 0.42519f, 0.46516f, 0.45090f, 0.44685f, 0.45090f, 0.44685f, 0.48525f,
    0.42424f, 0.48525f, 0.42424f, 0.52647f, 0.39654f, 0.52647f, 0.39654f, 0.57245f, 0.36389f, 0.57245f, 0.36389f,
    0.62102f, 0.32728f, 0.62102f, 0.32728f, 0.67023f, 0.28819f, 0.67023f, 0.28819f, 0.71841f, 0.24824f, 0.71841f,
    0.24824f, 0.76422f, 0.20897f, 0.76422f, 0.20897f, 0.80662f, 0.17171f, 0.80662f, 0.17171f, 0.84486f, 0.13747f,
    0.84486f, 0.13747f, 0.87848f, 0.10698f, 0.87848f, 0.10698f, 0.90723f, 0.08067f, 0.90723f, 0.08067f, 0.93113f,
    0.05869f, 0.93113f, 0.05869f, 0.95037f, 0.04097f, 0.95037f, 0.04097f, 0.96530f, 0.02724f, 0.96530f, 0.02724f,
    0.97642f, 0.01709f, 0.97642f, 0.01709f, 0.98431f, 0.00998f, 0.98431f, 0.00998f, 0.98961f, 0.00532f, 0.98961f,
    0.00532f, 0.99292f, 0.00253f, 0.99292f, 0.00253f, 0.99486f, 0.00103f, 0.99486f, 0.00103f, 0.99590f, 0.00034f,
    0.99590f, 0.00034f, 0.99646f, 0.00008f, 0.99646f, 0.00008f, 0.99678f, 0.00002f, 0.99678f, 0.00002f, 0.99696f,
    0.00000f, 0.54440f, 0.44406f, 0.54289f, 0.44288f, 0.54289f, 0.44288f, 0.53867f, 0.43937f, 0.53867f, 0.43937f,
    0.53230f, 0.43354f, 0.53230f, 0.43354f, 0.52482f, 0.42550f, 0.52482f, 0.42550f, 0.51757f, 0.41543f, 0.51757f,
    0.41543f, 0.51233f, 0.40362f, 0.51233f, 0.40362f, 0.51098f, 0.39035f, 0.51098f, 0.39035f, 0.51527f, 0.37577f,
    0.51527f, 0.37577f, 0.52643f, 0.35971f, 0.52643f, 0.35971f, 0.54492f, 0.34177f, 0.54492f, 0.34177f, 0.57048f,
    0.32143f, 0.57048f, 0.32143f, 0.60214f, 0.29833f, 0.60214f, 0.29833f, 0.63846f, 0.27247f, 0.63846f, 0.27247f,
    0.67778f, 0.24426f, 0.67778f, 0.24426f, 0.71839f, 0.21446f, 0.71839f, 0.21446f, 0.75869f, 0.18409f, 0.75869f,
    0.18409f, 0.79732f, 0.15421f, 0.79732f, 0.15421f, 0.83321f, 0.12582f, 0.83321f, 0.12582f, 0.86558f, 0.09977f,
    0.86558f, 0.09977f, 0.89393f, 0.07665f, 0.89393f, 0.07665f, 0.91803f, 0.05685f, 0.91803f, 0.05685f, 0.93791f,
    0.04051f, 0.93791f, 0.04051f, 0.95376f, 0.02756f, 0.95376f, 0.02756f, 0.96595f, 0.01775f, 0.96595f, 0.01775f,
    0.97495f, 0.01070f, 0.97495f, 0.01070f, 0.98132f, 0.00595f, 0.98132f, 0.00595f, 0.98562f, 0.00298f, 0.98562f,
    0.00298f, 0.98840f, 0.00131f, 0.98840f, 0.00131f, 0.99016f, 0.00049f, 0.99016f, 0.00049f, 0.99128f, 0.00015f,
    0.99128f, 0.00015f, 0.99205f, 0.00003f, 0.99205f, 0.00003f, 0.99265f, 0.00000f, 0.64418f, 0.34686f, 0.64305f,
    0.34596f, 0.64305f, 0.34596f, 0.63987f, 0.34327f, 0.63987f, 0.34327f, 0.63504f, 0.33882f, 0.63504f, 0.33882f,
    0.62913f, 0.33265f, 0.62913f, 0.33265f, 0.62284f, 0.32480f, 0.62284f, 0.32480f, 0.61704f, 0.31537f, 0.61704f,
    0.31537f, 0.61276f, 0.30448f, 0.61276f, 0.30448f, 0.61111f, 0.29224f, 0.61111f, 0.29224f, 0.61315f, 0.27875f,
    0.61315f, 0.27875f, 0.61973f, 0.26406f, 0.61973f, 0.26406f, 0.63137f, 0.24811f, 0.63137f, 0.24811f, 0.64814f,
    0.23085f, 0.64814f, 0.23085f, 0.66969f, 0.21223f, 0.66969f, 0.21223f, 0.69526f, 0.19233f, 0.69526f, 0.19233f,
    0.72382f, 0.17138f, 0.72382f, 0.17138f, 0.75419f, 0.14979f, 0.75419f, 0.14979f, 0.78512f, 0.12808f, 0.78512f,
    0.12808f, 0.81547f, 0.10687f, 0.81547f, 0.10687f, 0.84422f, 0.08679f, 0.84422f, 0.08679f, 0.87058f, 0.06840f,
    0.87058f, 0.06840f, 0.89402f, 0.05212f, 0.89402f, 0.05212f, 0.91421f, 0.03823f, 0.91421f, 0.03823f, 0.93108f,
    0.02685f, 0.93108f, 0.02685f, 0.94473f, 0.01793f, 0.94473f, 0.01793f, 0.95544f, 0.01128f, 0.95544f, 0.01128f,
    0.96358f, 0.00660f, 0.96358f, 0.00660f, 0.96960f, 0.00353f, 0.96960f, 0.00353f, 0.97395f, 0.00169f, 0.97395f,
    0.00169f, 0.97708f, 0.00070f, 0.97708f, 0.00070f, 0.97937f, 0.00024f, 0.97937f, 0.00024f, 0.98111f, 0.00007f,
    0.98111f, 0.00007f, 0.98251f, 0.00001f, 0.73005f, 0.26199f, 0.72905f, 0.26125f, 0.72905f, 0.26125f, 0.72624f,
    0.25907f, 0.72624f, 0.25907f, 0.72202f, 0.25550f, 0.72202f, 0.25550f, 0.71681f, 0.25057f, 0.71681f, 0.25057f,
    0.71107f, 0.24435f, 0.71107f, 0.24435f, 0.70524f, 0.23690f, 0.70524f, 0.23690f, 0.69988f, 0.22828f, 0.69988f,
    0.22828f, 0.69558f, 0.21859f, 0.69558f, 0.21859f, 0.69294f, 0.20792f, 0.69294f, 0.20792f, 0.69258f, 0.19636f,
    0.69258f, 0.19636f, 0.69503f, 0.18401f, 0.69503f, 0.18401f, 0.70069f, 0.17093f, 0.70069f, 0.17093f, 0.70972f,
    0.15722f, 0.70972f, 0.15722f, 0.72209f, 0.14295f, 0.72209f, 0.14295f, 0.73751f, 0.12825f, 0.73751f, 0.12825f,
    0.75550f, 0.11329f, 0.75550f, 0.11329f, 0.77538f, 0.09829f, 0.77538f, 0.09829f, 0.79642f, 0.08355f, 0.79642f,
    0.08355f, 0.81782f, 0.06937f, 0.81782f, 0.06937f, 0.83884f, 0.05609f, 0.83884f, 0.05609f, 0.85882f, 0.04400f,
    0.85882f, 0.04400f, 0.87722f, 0.03335f, 0.87722f, 0.03335f, 0.89369f, 0.02431f, 0.89369f, 0.02431f, 0.90802f,
    0.01693f, 0.90802f, 0.01693f, 0.92018f, 0.01117f, 0.92018f, 0.01117f, 0.93024f, 0.00692f, 0.93024f, 0.00692f,
    0.93842f, 0.00397f, 0.93842f, 0.00397f, 0.94499f, 0.00206f, 0.94499f, 0.00206f, 0.95024f, 0.00095f, 0.95024f,
    0.00095f, 0.95447f, 0.00037f, 0.95447f, 0.00037f, 0.95795f, 0.00011f, 0.95795f, 0.00011f, 0.96087f, 0.00002f,
    0.79892f, 0.19296f, 0.79783f, 0.19234f, 0.79783f, 0.19234f, 0.79480f, 0.19052f, 0.79480f, 0.19052f, 0.79032f,
    0.18757f, 0.79032f, 0.18757f, 0.78480f, 0.18356f, 0.78480f, 0.18356f, 0.77862f, 0.17856f, 0.77862f, 0.17856f,
    0.77213f, 0.17263f, 0.77213f, 0.17263f, 0.76566f, 0.16585f, 0.76566f, 0.16585f, 0.75952f, 0.15830f, 0.75952f,
    0.15830f, 0.75407f, 0.15006f, 0.75407f, 0.15006f, 0.74964f, 0.14123f, 0.74964f, 0.14123f, 0.74658f, 0.13189f,
    0.74658f, 0.13189f, 0.74517f, 0.12215f, 0.74517f, 0.12215f, 0.74568f, 0.11210f, 0.74568f, 0.11210f, 0.74827f,
    0.10184f, 0.74827f, 0.10184f, 0.75301f, 0.09147f, 0.75301f, 0.09147f, 0.75984f, 0.08111f, 0.75984f, 0.08111f,
    0.76861f, 0.07086f, 0.76861f, 0.07086f, 0.77904f, 0.06088f, 0.77904f, 0.06088f, 0.79078f, 0.05131f, 0.79078f,
    0.05131f, 0.80344f, 0.04229f, 0.80344f, 0.04229f, 0.81657f, 0.03398f, 0.81657f, 0.03398f, 0.82977f, 0.02652f,
    0.82977f, 0.02652f, 0.84264f, 0.02000f, 0.84264f, 0.02000f, 0.85487f, 0.01450f, 0.85487f, 0.01450f, 0.86623f,
    0.01003f, 0.86623f, 0.01003f, 0.87658f, 0.00657f, 0.87658f, 0.00657f, 0.88584f, 0.00402f, 0.88584f, 0.00402f,
    0.89405f, 0.00226f, 0.89405f, 0.00226f, 0.90129f, 0.00113f, 0.90129f, 0.00113f, 0.90766f, 0.00049f, 0.90766f,
    0.00049f, 0.91329f, 0.00016f, 0.91329f, 0.00016f, 0.91831f, 0.00003f, 0.85055f, 0.14020f, 0.84920f, 0.13967f,
    0.84920f, 0.13967f, 0.84547f, 0.13810f, 0.84547f, 0.13810f, 0.83995f, 0.13563f, 0.83995f, 0.13563f, 0.83317f,
    0.13231f, 0.83317f, 0.13231f, 0.82556f, 0.12823f, 0.82556f, 0.12823f, 0.81743f, 0.12347f, 0.81743f, 0.12347f,
    0.80906f, 0.11810f, 0.80906f, 0.11810f, 0.80069f, 0.11221f, 0.80069f, 0.11221f, 0.79256f, 0.10587f, 0.79256f,
    0.10587f, 0.78486f, 0.09917f, 0.78486f, 0.09917f, 0.77779f, 0.09218f, 0.77779f, 0.09218f, 0.77154f, 0.08500f,
    0.77154f, 0.08500f, 0.76629f, 0.07770f, 0.76629f, 0.07770f, 0.76216f, 0.07036f, 0.76216f, 0.07036f, 0.75930f,
    0.06306f, 0.75930f, 0.06306f, 0.75777f, 0.05588f, 0.75777f, 0.05588f, 0.75761f, 0.04889f, 0.75761f, 0.04889f,
    0.75881f, 0.04217f, 0.75881f, 0.04217f, 0.76131f, 0.03579f, 0.76131f, 0.03579f, 0.76500f, 0.02983f, 0.76500f,
    0.02983f, 0.76973f, 0.02433f, 0.76973f, 0.02433f, 0.77534f, 0.01937f, 0.77534f, 0.01937f, 0.78163f, 0.01499f,
    0.78163f, 0.01499f, 0.78842f, 0.01122f, 0.78842f, 0.01122f, 0.79551f, 0.00807f, 0.79551f, 0.00807f, 0.80275f,
    0.00553f, 0.80275f, 0.00553f, 0.80999f, 0.00358f, 0.80999f, 0.00358f, 0.81714f, 0.00215f, 0.81714f, 0.00215f,
    0.82412f, 0.00116f, 0.82412f, 0.00116f, 0.83088f, 0.00055f, 0.83088f, 0.00055f, 0.83741f, 0.00020f, 0.83741f,
    0.00020f, 0.84370f, 0.00004f, 0.88683f, 0.10193f, 0.88507f, 0.10146f, 0.88507f, 0.10146f, 0.88018f, 0.10008f,
    0.88018f, 0.10008f, 0.87297f, 0.09792f, 0.87297f, 0.09792f, 0.86411f, 0.09511f, 0.86411f, 0.09511f, 0.85411f,
    0.09171f, 0.85411f, 0.09171f, 0.84335f, 0.08782f, 0.84335f, 0.08782f, 0.83214f, 0.08350f, 0.83214f, 0.08350f,
    0.82071f, 0.07885f, 0.82071f, 0.07885f, 0.80926f, 0.07392f, 0.80926f, 0.07392f, 0.79797f, 0.06880f, 0.79797f,
    0.06880f, 0.78697f, 0.06354f, 0.78697f, 0.06354f, 0.77641f, 0.05823f, 0.77641f, 0.05823f, 0.76639f, 0.05291f,
    0.76639f, 0.05291f, 0.75702f, 0.04766f, 0.75702f, 0.04766f, 0.74838f, 0.04251f, 0.74838f, 0.04251f, 0.74054f,
    0.03753f, 0.74054f, 0.03753f, 0.73357f, 0.03275f, 0.73357f, 0.03275f, 0.72752f, 0.02823f, 0.72752f, 0.02823f,
    0.72242f, 0.02399f, 0.72242f, 0.02399f, 0.71828f, 0.02007f, 0.71828f, 0.02007f, 0.71510f, 0.01649f, 0.71510f,
    0.01649f, 0.71287f, 0.01327f, 0.71287f, 0.01327f, 0.71153f, 0.01043f, 0.71153f, 0.01043f, 0.71107f, 0.00797f,
    0.71107f, 0.00797f, 0.71140f, 0.00589f, 0.71140f, 0.00589f, 0.71249f, 0.00418f, 0.71249f, 0.00418f, 0.71426f,
    0.00281f, 0.71426f, 0.00281f, 0.71664f, 0.00178f, 0.71664f, 0.00178f, 0.71959f, 0.00103f, 0.71959f, 0.00103f,
    0.72304f, 0.00052f, 0.72304f, 0.00052f, 0.72694f, 0.00021f, 0.72694f, 0.00021f, 0.73123f, 0.00005f, 0.91066f,
    0.07529f, 0.90833f, 0.07485f, 0.90833f, 0.07485f, 0.90190f, 0.07359f, 0.90190f, 0.07359f, 0.89240f, 0.07163f,
    0.89240f, 0.07163f, 0.88070f, 0.06916f, 0.88070f, 0.06916f, 0.86747f, 0.06623f, 0.86747f, 0.06623f, 0.85317f,
    0.06294f, 0.85317f, 0.06294f, 0.83818f, 0.05938f, 0.83818f, 0.05938f, 0.82277f, 0.05561f, 0.82277f, 0.05561f,
    0.80717f, 0.05169f, 0.80717f, 0.05169f, 0.79156f, 0.04770f, 0.79156f, 0.04770f, 0.77610f, 0.04369f, 0.77610f,
    0.04369f, 0.76091f, 0.03970f, 0.76091f, 0.03970f, 0.74607f, 0.03578f, 0.74607f, 0.03578f, 0.73168f, 0.03198f,
    0.73168f, 0.03198f, 0.71780f, 0.02831f, 0.71780f, 0.02831f, 0.70449f, 0.02483f, 0.70449f, 0.02483f, 0.69180f,
    0.02155f, 0.69180f, 0.02155f, 0.67976f, 0.01848f, 0.67976f, 0.01848f, 0.66840f, 0.01566f, 0.66840f, 0.01566f,
    0.65777f, 0.01308f, 0.65777f, 0.01308f, 0.64787f, 0.01075f, 0.64787f, 0.01075f, 0.63872f, 0.00869f, 0.63872f,
    0.00869f, 0.63033f, 0.00687f, 0.63033f, 0.00687f, 0.62272f, 0.00531f, 0.62272f, 0.00531f, 0.61588f, 0.00398f,
    0.61588f, 0.00398f, 0.60982f, 0.00288f, 0.60982f, 0.00288f, 0.60452f, 0.00200f, 0.60452f, 0.00200f, 0.59998f,
    0.00131f, 0.59998f, 0.00131f, 0.59620f, 0.00079f, 0.59620f, 0.00079f, 0.59315f, 0.00043f, 0.59315f, 0.00043f,
    0.59083f, 0.00019f, 0.59083f, 0.00019f, 0.58921f, 0.00005f, 0.92505f, 0.05728f, 0.92203f, 0.05684f, 0.92203f,
    0.05684f, 0.91367f, 0.05562f, 0.91367f, 0.05562f, 0.90132f, 0.05378f, 0.90132f, 0.05378f, 0.88612f, 0.05148f,
    0.88612f, 0.05148f, 0.86888f, 0.04885f, 0.86888f, 0.04885f, 0.85023f, 0.04597f, 0.85023f, 0.04597f, 0.83062f,
    0.04292f, 0.83062f, 0.04292f, 0.81040f, 0.03976f, 0.81040f, 0.03976f, 0.78987f, 0.03657f, 0.78987f, 0.03657f,
    0.76923f, 0.03337f, 0.76923f, 0.03337f, 0.74866f, 0.03023f, 0.74866f, 0.03023f, 0.72832f, 0.02717f, 0.72832f,
    0.02717f, 0.70829f, 0.02423f, 0.70829f, 0.02423f, 0.68869f, 0.02143f, 0.68869f, 0.02143f, 0.66956f, 0.01879f,
    0.66956f, 0.01879f, 0.65097f, 0.01632f, 0.65097f, 0.01632f, 0.63296f, 0.01404f, 0.63296f, 0.01404f, 0.61555f,
    0.01195f, 0.61555f, 0.01195f, 0.59877f, 0.01005f, 0.59877f, 0.01005f, 0.58263f, 0.00835f, 0.58263f, 0.00835f,
    0.56715f, 0.00684f, 0.56715f, 0.00684f, 0.55232f, 0.00551f, 0.55232f, 0.00551f, 0.53815f, 0.00436f, 0.53815f,
    0.00436f, 0.52465f, 0.00338f, 0.52465f, 0.00338f, 0.51180f, 0.00255f, 0.51180f, 0.00255f, 0.49961f, 0.00187f,
    0.49961f, 0.00187f, 0.48807f, 0.00132f, 0.48807f, 0.00132f, 0.47717f, 0.00088f, 0.47717f, 0.00088f, 0.46691f,
    0.00055f, 0.46691f, 0.00055f, 0.45727f, 0.00031f, 0.45727f, 0.00031f, 0.44826f, 0.00015f, 0.44826f, 0.00015f,
    0.43985f, 0.00004f, 0.93261f, 0.04530f, 0.92877f, 0.04485f, 0.92877f, 0.04485f, 0.91813f, 0.04361f, 0.91813f,
    0.04361f, 0.90242f, 0.04179f, 0.90242f, 0.04179f, 0.88309f, 0.03957f, 0.88309f, 0.03957f, 0.86118f, 0.03710f,
    0.86118f, 0.03710f, 0.83748f, 0.03447f, 0.83748f, 0.03447f, 0.81257f, 0.03175f, 0.81257f, 0.03175f, 0.78690f,
    0.02902f, 0.78690f, 0.02902f, 0.76083f, 0.02633f, 0.76083f, 0.02633f, 0.73463f, 0.02370f, 0.73463f, 0.02370f,
    0.70854f, 0.02117f, 0.70854f, 0.02117f, 0.68271f, 0.01877f, 0.68271f, 0.01877f, 0.65731f, 0.01652f, 0.65731f,
    0.01652f, 0.63242f, 0.01442f, 0.63242f, 0.01442f, 0.60813f, 0.01248f, 0.60813f, 0.01248f, 0.58451f, 0.01071f,
    0.58451f, 0.01071f, 0.56160f, 0.00911f, 0.56160f, 0.00911f, 0.53944f, 0.00767f, 0.53944f, 0.00767f, 0.51803f,
    0.00639f, 0.51803f, 0.00639f, 0.49740f, 0.00526f, 0.49740f, 0.00526f, 0.47754f, 0.00428f, 0.47754f, 0.00428f,
    0.45846f, 0.00343f, 0.45846f, 0.00343f, 0.44014f, 0.00270f, 0.44014f, 0.00270f, 0.42257f, 0.00209f, 0.42257f,
    0.00209f, 0.40573f, 0.00158f, 0.40573f, 0.00158f, 0.38962f, 0.00116f, 0.38962f, 0.00116f, 0.37420f, 0.00082f,
    0.37420f, 0.00082f, 0.35946f, 0.00056f, 0.35946f, 0.00056f, 0.34537f, 0.00036f, 0.34537f, 0.00036f, 0.33192f,
    0.00021f, 0.33192f, 0.00021f, 0.31908f, 0.00010f, 0.31908f, 0.00010f, 0.30682f, 0.00003f
};
static const float4 KH_ENVBRDF_C[512] = {
    0.00587f, 0.00643f, 0.00642f, 0.00690f, 0.00643f, 0.00855f, 0.00690f, 0.00885f, 0.00855f, 0.01244f, 0.00885f,
    0.01261f, 0.01244f, 0.01775f, 0.01261f, 0.01786f, 0.01775f, 0.02406f, 0.01786f, 0.02412f, 0.02406f, 0.03085f,
    0.02412f, 0.03089f, 0.03085f, 0.03764f, 0.03089f, 0.03765f, 0.03764f, 0.04393f, 0.03765f, 0.04393f, 0.04393f,
    0.04928f, 0.04393f, 0.04928f, 0.04928f, 0.05334f, 0.04928f, 0.05333f, 0.05334f, 0.05583f, 0.05333f, 0.05582f,
    0.05583f, 0.05662f, 0.05582f, 0.05661f, 0.05662f, 0.05567f, 0.05661f, 0.05566f, 0.05567f, 0.05308f, 0.05566f,
    0.05307f, 0.05308f, 0.04907f, 0.05307f, 0.04907f, 0.04907f, 0.04394f, 0.04907f, 0.04394f, 0.04394f, 0.03805f,
    0.04394f, 0.03804f, 0.03805f, 0.03179f, 0.03804f, 0.03178f, 0.03179f, 0.02554f, 0.03178f, 0.02554f, 0.02554f,
    0.01965f, 0.02554f, 0.01965f, 0.01965f, 0.01440f, 0.01965f, 0.01440f, 0.01440f, 0.00997f, 0.01440f, 0.00997f,
    0.00997f, 0.00646f, 0.00997f, 0.00646f, 0.00646f, 0.00386f, 0.00646f, 0.00387f, 0.00386f, 0.00209f, 0.00387f,
    0.00209f, 0.00209f, 0.00100f, 0.00209f, 0.00100f, 0.00100f, 0.00040f, 0.00100f, 0.00040f, 0.00040f, 0.00012f,
    0.00040f, 0.00012f, 0.00012f, 0.00003f, 0.00012f, 0.00003f, 0.00003f, 0.00000f, 0.00003f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00642f, 0.00690f, 0.00850f, 0.00871f,
    0.00690f, 0.00885f, 0.00871f, 0.01007f, 0.00885f, 0.01261f, 0.01007f, 0.01335f, 0.01261f, 0.01786f, 0.01335f,
    0.01830f, 0.01786f, 0.02412f, 0.01830f, 0.02438f, 0.02412f, 0.03089f, 0.02438f, 0.03102f, 0.03089f, 0.03765f,
    0.03102f, 0.03771f, 0.03765f, 0.04393f, 0.03771f, 0.04394f, 0.04393f, 0.04928f, 0.04394f, 0.04926f, 0.04928f,
    0.05333f, 0.04926f, 0.05329f, 0.05333f, 0.05582f, 0.05329f, 0.05577f, 0.05582f, 0.05661f, 0.05577f, 0.05655f,
    0.05661f, 0.05566f, 0.05655f, 0.05561f, 0.05566f, 0.05307f, 0.05561f, 0.05303f, 0.05307f, 0.04907f, 0.05303f,
    0.04903f, 0.04907f, 0.04394f, 0.04903f, 0.04391f, 0.04394f, 0.03804f, 0.04391f, 0.03803f, 0.03804f, 0.03178f,
    0.03803f, 0.03177f, 0.03178f, 0.02554f, 0.03177f, 0.02553f, 0.02554f, 0.01965f, 0.02553f, 0.01965f, 0.01965f,
    0.01440f, 0.01965f, 0.01440f, 0.01440f, 0.00997f, 0.01440f, 0.00997f, 0.00997f, 0.00646f, 0.00997f, 0.00646f,
    0.00646f, 0.00387f, 0.00646f, 0.00387f, 0.00387f, 0.00209f, 0.00387f, 0.00209f, 0.00209f, 0.00100f, 0.00209f,
    0.00100f, 0.00100f, 0.00040f, 0.00100f, 0.00040f, 0.00040f, 0.00012f, 0.00040f, 0.00012f, 0.00012f, 0.00003f,
    0.00012f, 0.00003f, 0.00003f, 0.00000f, 0.00003f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00850f, 0.00871f, 0.01285f, 0.01276f, 0.00871f, 0.01007f, 0.01276f, 0.01319f,
    0.01007f, 0.01335f, 0.01319f, 0.01533f, 0.01335f, 0.01830f, 0.01533f, 0.01946f, 0.01830f, 0.02438f, 0.01946f,
    0.02503f, 0.02438f, 0.03102f, 0.02503f, 0.03137f, 0.03102f, 0.03771f, 0.03137f, 0.03785f, 0.03771f, 0.04394f,
    0.03785f, 0.04394f, 0.04394f, 0.04926f, 0.04394f, 0.04917f, 0.04926f, 0.05329f, 0.04917f, 0.05316f, 0.05329f,
    0.05577f, 0.05316f, 0.05562f, 0.05577f, 0.05655f, 0.05562f, 0.05640f, 0.05655f, 0.05561f, 0.05640f, 0.05546f,
    0.05561f, 0.05303f, 0.05546f, 0.05290f, 0.05303f, 0.04903f, 0.05290f, 0.04892f, 0.04903f, 0.04391f, 0.04892f,
    0.04383f, 0.04391f, 0.03803f, 0.04383f, 0.03797f, 0.03803f, 0.03177f, 0.03797f, 0.03173f, 0.03177f, 0.02553f,
    0.03173f, 0.02551f, 0.02553f, 0.01965f, 0.02551f, 0.01964f, 0.01965f, 0.01440f, 0.01964f, 0.01440f, 0.01440f,
    0.00997f, 0.01440f, 0.00998f, 0.00997f, 0.00646f, 0.00998f, 0.00647f, 0.00646f, 0.00387f, 0.00647f, 0.00388f,
    0.00387f, 0.00209f, 0.00388f, 0.00210f, 0.00209f, 0.00100f, 0.00210f, 0.00100f, 0.00100f, 0.00040f, 0.00100f,
    0.00040f, 0.00040f, 0.00012f, 0.00040f, 0.00013f, 0.00012f, 0.00003f, 0.00013f, 0.00003f, 0.00003f, 0.00000f,
    0.00003f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.01285f,
    0.01276f, 0.01925f, 0.01905f, 0.01276f, 0.01319f, 0.01905f, 0.01886f, 0.01319f, 0.01533f, 0.01886f, 0.01964f,
    0.01533f, 0.01946f, 0.01964f, 0.02221f, 0.01946f, 0.02503f, 0.02221f, 0.02655f, 0.02503f, 0.03137f, 0.02655f,
    0.03208f, 0.03137f, 0.03785f, 0.03208f, 0.03807f, 0.03785f, 0.04394f, 0.03807f, 0.04384f, 0.04394f, 0.04917f,
    0.04384f, 0.04888f, 0.04917f, 0.05316f, 0.04888f, 0.05276f, 0.05316f, 0.05562f, 0.05276f, 0.05517f, 0.05562f,
    0.05640f, 0.05517f, 0.05595f, 0.05640f, 0.05546f, 0.05595f, 0.05504f, 0.05546f, 0.05290f, 0.05504f, 0.05254f,
    0.05290f, 0.04892f, 0.05254f, 0.04862f, 0.04892f, 0.04383f, 0.04862f, 0.04359f, 0.04383f, 0.03797f, 0.04359f,
    0.03780f, 0.03797f, 0.03173f, 0.03780f, 0.03162f, 0.03173f, 0.02551f, 0.03162f, 0.02545f, 0.02551f, 0.01964f,
    0.02545f, 0.01961f, 0.01964f, 0.01440f, 0.01961f, 0.01440f, 0.01440f, 0.00998f, 0.01440f, 0.01000f, 0.00998f,
    0.00647f, 0.01000f, 0.00650f, 0.00647f, 0.00388f, 0.00650f, 0.00390f, 0.00388f, 0.00210f, 0.00390f, 0.00212f,
    0.00210f, 0.00100f, 0.00212f, 0.00102f, 0.00100f, 0.00040f, 0.00102f, 0.00041f, 0.00040f, 0.00013f, 0.00041f,
    0.00013f, 0.00013f, 0.00003f, 0.00013f, 0.00003f, 0.00003f, 0.00000f, 0.00003f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.01925f, 0.01905f, 0.02636f, 0.02622f, 0.01905f,
    0.01886f, 0.02622f, 0.02596f, 0.01886f, 0.01964f, 0.02596f, 0.02604f, 0.01964f, 0.02221f, 0.02604f, 0.02712f,
    0.02221f, 0.02655f, 0.02712f, 0.02965f, 0.02655f, 0.03208f, 0.02965f, 0.03356f, 0.03208f, 0.03807f, 0.03356f,
    0.03837f, 0.03807f, 0.04384f, 0.03837f, 0.04341f, 0.04384f, 0.04888f, 0.04341f, 0.04801f, 0.04888f, 0.05276f,
    0.04801f, 0.05166f, 0.05276f, 0.05517f, 0.05166f, 0.05400f, 0.05517f, 0.05595f, 0.05400f, 0.05480f, 0.05595f,
    0.05504f, 0.05480f, 0.05398f, 0.05504f, 0.05254f, 0.05398f, 0.05161f, 0.05254f, 0.04862f, 0.05161f, 0.04786f,
    0.04862f, 0.04359f, 0.04786f, 0.04300f, 0.04359f, 0.03780f, 0.04300f, 0.03737f, 0.03780f, 0.03162f, 0.03737f,
    0.03133f, 0.03162f, 0.02545f, 0.03133f, 0.02528f, 0.02545f, 0.01961f, 0.02528f, 0.01954f, 0.01961f, 0.01440f,
    0.01954f, 0.01439f, 0.01440f, 0.01000f, 0.01439f, 0.01002f, 0.01000f, 0.00650f, 0.01002f, 0.00654f, 0.00650f,
    0.00390f, 0.00654f, 0.00395f, 0.00390f, 0.00212f, 0.00395f, 0.00216f, 0.00212f, 0.00102f, 0.00216f, 0.00105f,
    0.00102f, 0.00041f, 0.00105f, 0.00043f, 0.00041f, 0.00013f, 0.00043f, 0.00014f, 0.00013f, 0.00003f, 0.00014f,
    0.00003f, 0.00003f, 0.00000f, 0.00003f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.02636f, 0.02622f, 0.03251f, 0.03247f, 0.02622f, 0.02596f, 0.03247f, 0.03237f, 0.02596f,
    0.02604f, 0.03237f, 0.03238f, 0.02604f, 0.02712f, 0.03238f, 0.03280f, 0.02712f, 0.02965f, 0.03280f, 0.03395f,
    0.02965f, 0.03356f, 0.03395f, 0.03605f, 0.03356f, 0.03837f, 0.03605f, 0.03905f, 0.03837f, 0.04341f, 0.03905f,
    0.04261f, 0.04341f, 0.04801f, 0.04261f, 0.04621f, 0.04801f, 0.05166f, 0.04621f, 0.04930f, 0.05166f, 0.05400f,
    0.04930f, 0.05142f, 0.05400f, 0.05480f, 0.05142f, 0.05225f, 0.05480f, 0.05398f, 0.05225f, 0.05163f, 0.05398f,
    0.05161f, 0.05163f, 0.04955f, 0.05161f, 0.04786f, 0.04955f, 0.04615f, 0.04786f, 0.04300f, 0.04615f, 0.04165f,
    0.04300f, 0.03737f, 0.04165f, 0.03637f, 0.03737f, 0.03133f, 0.03637f, 0.03065f, 0.03133f, 0.02528f, 0.03065f,
    0.02486f, 0.02528f, 0.01954f, 0.02486f, 0.01933f, 0.01954f, 0.01439f, 0.01933f, 0.01433f, 0.01439f, 0.01002f,
    0.01433f, 0.01006f, 0.01002f, 0.00654f, 0.01006f, 0.00663f, 0.00654f, 0.00395f, 0.00663f, 0.00405f, 0.00395f,
    0.00216f, 0.00405f, 0.00225f, 0.00216f, 0.00105f, 0.00225f, 0.00111f, 0.00105f, 0.00043f, 0.00111f, 0.00047f,
    0.00043f, 0.00014f, 0.00047f, 0.00016f, 0.00014f, 0.00003f, 0.00016f, 0.00004f, 0.00003f, 0.00000f, 0.00004f,
    0.00001f, 0.00000f, 0.00000f, 0.00001f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.03251f, 0.03247f,
    0.03636f, 0.03637f, 0.03247f, 0.03237f, 0.03637f, 0.03640f, 0.03237f, 0.03238f, 0.03640f, 0.03649f, 0.03238f,
    0.03280f, 0.03649f, 0.03674f, 0.03280f, 0.03395f, 0.03674f, 0.03727f, 0.03395f, 0.03605f, 0.03727f, 0.03822f,
    0.03605f, 0.03905f, 0.03822f, 0.03965f, 0.03905f, 0.04261f, 0.03965f, 0.04152f, 0.04261f, 0.04621f, 0.04152f,
    0.04361f, 0.04621f, 0.04930f, 0.04361f, 0.04559f, 0.04930f, 0.05142f, 0.04559f, 0.04708f, 0.05142f, 0.05225f,
    0.04708f, 0.04772f, 0.05225f, 0.05163f, 0.04772f, 0.04728f, 0.05163f, 0.04955f, 0.04728f, 0.04563f, 0.04955f,
    0.04615f, 0.04563f, 0.04280f, 0.04615f, 0.04165f, 0.04280f, 0.03895f, 0.04165f, 0.03637f, 0.03895f, 0.03431f,
    0.03637f, 0.03065f, 0.03431f, 0.02919f, 0.03065f, 0.02486f, 0.02919f, 0.02391f, 0.02486f, 0.01933f, 0.02391f,
    0.01880f, 0.01933f, 0.01433f, 0.01880f, 0.01411f, 0.01433f, 0.01006f, 0.01411f, 0.01004f, 0.01006f, 0.00663f,
    0.01004f, 0.00673f, 0.00663f, 0.00405f, 0.00673f, 0.00419f, 0.00405f, 0.00225f, 0.00419f, 0.00239f, 0.00225f,
    0.00111f, 0.00239f, 0.00123f, 0.00111f, 0.00047f, 0.00123f, 0.00055f, 0.00047f, 0.00016f, 0.00055f, 0.00021f,
    0.00016f, 0.00004f, 0.00021f, 0.00006f, 0.00004f, 0.00001f, 0.00006f, 0.00001f, 0.00001f, 0.00000f, 0.00001f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.03636f, 0.03637f, 0.03717f, 0.03719f, 0.03637f, 0.03640f,
    0.03719f, 0.03725f, 0.03640f, 0.03649f, 0.03725f, 0.03735f, 0.03649f, 0.03674f, 0.03735f, 0.03752f, 0.03674f,
    0.03727f, 0.03752f, 0.03778f, 0.03727f, 0.03822f, 0.03778f, 0.03816f, 0.03822f, 0.03965f, 0.03816f, 0.03869f,
    0.03965f, 0.04152f, 0.03869f, 0.03938f, 0.04152f, 0.04361f, 0.03938f, 0.04016f, 0.04361f, 0.04559f, 0.04016f,
    0.04091f, 0.04559f, 0.04708f, 0.04091f, 0.04144f, 0.04708f, 0.04772f, 0.04144f, 0.04154f, 0.04772f, 0.04728f,
    0.04154f, 0.04099f, 0.04728f, 0.04563f, 0.04099f, 0.03965f, 0.04563f, 0.04280f, 0.03965f, 0.03745f, 0.04280f,
    0.03895f, 0.03745f, 0.03442f, 0.03895f, 0.03431f, 0.03442f, 0.03070f, 0.03431f, 0.02919f, 0.03070f, 0.02649f,
    0.02919f, 0.02391f, 0.02649f, 0.02205f, 0.02391f, 0.01880f, 0.02205f, 0.01764f, 0.01880f, 0.01411f, 0.01764f,
    0.01349f, 0.01411f, 0.01004f, 0.01349f, 0.00982f, 0.01004f, 0.00673f, 0.00982f, 0.00675f, 0.00673f, 0.00419f,
    0.00675f, 0.00434f, 0.00419f, 0.00239f, 0.00434f, 0.00258f, 0.00239f, 0.00123f, 0.00258f, 0.00139f, 0.00123f,
    0.00055f, 0.00139f, 0.00067f, 0.00055f, 0.00021f, 0.00067f, 0.00028f, 0.00021f, 0.00006f, 0.00028f, 0.00010f,
    0.00006f, 0.00001f, 0.00010f, 0.00003f, 0.00001f, 0.00000f, 0.00003f, 0.00001f, 0.00000f, 0.00000f, 0.00001f,
    0.00000f, 0.03717f, 0.03719f, 0.03502f, 0.03503f, 0.03719f, 0.03725f, 0.03503f, 0.03506f, 0.03725f, 0.03735f,
    0.03506f, 0.03510f, 0.03735f, 0.03752f, 0.03510f, 0.03515f, 0.03752f, 0.03778f, 0.03515f, 0.03521f, 0.03778f,
    0.03816f, 0.03521f, 0.03527f, 0.03816f, 0.03869f, 0.03527f, 0.03533f, 0.03869f, 0.03938f, 0.03533f, 0.03538f,
    0.03938f, 0.04016f, 0.03538f, 0.03538f, 0.04016f, 0.04091f, 0.03538f, 0.03530f, 0.04091f, 0.04144f, 0.03530f,
    0.03507f, 0.04144f, 0.04154f, 0.03507f, 0.03458f, 0.04154f, 0.04099f, 0.03458f, 0.03375f, 0.04099f, 0.03965f,
    0.03375f, 0.03248f, 0.03965f, 0.03745f, 0.03248f, 0.03070f, 0.03745f, 0.03442f, 0.03070f, 0.02840f, 0.03442f,
    0.03070f, 0.02840f, 0.02561f, 0.03070f, 0.02649f, 0.02561f, 0.02244f, 0.02649f, 0.02205f, 0.02244f, 0.01903f,
    0.02205f, 0.01764f, 0.01903f, 0.01557f, 0.01764f, 0.01349f, 0.01557f, 0.01222f, 0.01349f, 0.00982f, 0.01222f,
    0.00916f, 0.00982f, 0.00675f, 0.00916f, 0.00652f, 0.00675f, 0.00434f, 0.00652f, 0.00436f, 0.00434f, 0.00258f,
    0.00436f, 0.00272f, 0.00258f, 0.00139f, 0.00272f, 0.00156f, 0.00139f, 0.00067f, 0.00156f, 0.00081f, 0.00067f,
    0.00028f, 0.00081f, 0.00037f, 0.00028f, 0.00010f, 0.00037f, 0.00015f, 0.00010f, 0.00003f, 0.00015f, 0.00005f,
    0.00003f, 0.00001f, 0.00005f, 0.00001f, 0.00001f, 0.00000f, 0.00001f, 0.00000f, 0.03502f, 0.03503f, 0.03074f,
    0.03073f, 0.03503f, 0.03506f, 0.03073f, 0.03070f, 0.03506f, 0.03510f, 0.03070f, 0.03065f, 0.03510f, 0.03515f,
    0.03065f, 0.03057f, 0.03515f, 0.03521f, 0.03057f, 0.03046f, 0.03521f, 0.03527f, 0.03046f, 0.03031f, 0.03527f,
    0.03533f, 0.03031f, 0.03010f, 0.03533f, 0.03538f, 0.03010f, 0.02982f, 0.03538f, 0.03538f, 0.02982f, 0.02945f,
    0.03538f, 0.03530f, 0.02945f, 0.02896f, 0.03530f, 0.03507f, 0.02896f, 0.02833f, 0.03507f, 0.03458f, 0.02833f,
    0.02754f, 0.03458f, 0.03375f, 0.02754f, 0.02653f, 0.03375f, 0.03248f, 0.02653f, 0.02528f, 0.03248f, 0.03070f,
    0.02528f, 0.02377f, 0.03070f, 0.02840f, 0.02377f, 0.02197f, 0.02840f, 0.02561f, 0.02197f, 0.01991f, 0.02561f,
    0.02244f, 0.01991f, 0.01763f, 0.02244f, 0.01903f, 0.01763f, 0.01519f, 0.01903f, 0.01557f, 0.01519f, 0.01269f,
    0.01557f, 0.01222f, 0.01269f, 0.01023f, 0.01222f, 0.00916f, 0.01023f, 0.00792f, 0.00916f, 0.00652f, 0.00792f,
    0.00585f, 0.00652f, 0.00436f, 0.00585f, 0.00410f, 0.00436f, 0.00272f, 0.00410f, 0.00270f, 0.00272f, 0.00156f,
    0.00270f, 0.00165f, 0.00156f, 0.00081f, 0.00165f, 0.00093f, 0.00081f, 0.00037f, 0.00093f, 0.00047f, 0.00037f,
    0.00015f, 0.00047f, 0.00021f, 0.00015f, 0.00005f, 0.00021f, 0.00008f, 0.00005f, 0.00001f, 0.00008f, 0.00002f,
    0.00001f, 0.00000f, 0.00002f, 0.00000f, 0.03074f, 0.03073f, 0.02545f, 0.02542f, 0.03073f, 0.03070f, 0.02542f,
    0.02535f, 0.03070f, 0.03065f, 0.02535f, 0.02522f, 0.03065f, 0.03057f, 0.02522f, 0.02505f, 0.03057f, 0.03046f,
    0.02505f, 0.02482f, 0.03046f, 0.03031f, 0.02482f, 0.02452f, 0.03031f, 0.03010f, 0.02452f, 0.02416f, 0.03010f,
    0.02982f, 0.02416f, 0.02372f, 0.02982f, 0.02945f, 0.02372f, 0.02319f, 0.02945f, 0.02896f, 0.02319f, 0.02256f,
    0.02896f, 0.02833f, 0.02256f, 0.02182f, 0.02833f, 0.02754f, 0.02182f, 0.02095f, 0.02754f, 0.02653f, 0.02095f,
    0.01996f, 0.02653f, 0.02528f, 0.01996f, 0.01883f, 0.02528f, 0.02377f, 0.01883f, 0.01755f, 0.02377f, 0.02197f,
    0.01755f, 0.01615f, 0.02197f, 0.01991f, 0.01615f, 0.01462f, 0.01991f, 0.01763f, 0.01462f, 0.01298f, 0.01763f,
    0.01519f, 0.01298f, 0.01129f, 0.01519f, 0.01269f, 0.01129f, 0.00956f, 0.01269f, 0.01023f, 0.00956f, 0.00787f,
    0.01023f, 0.00792f, 0.00787f, 0.00626f, 0.00792f, 0.00585f, 0.00626f, 0.00479f, 0.00585f, 0.00410f, 0.00479f,
    0.00350f, 0.00410f, 0.00270f, 0.00350f, 0.00243f, 0.00270f, 0.00165f, 0.00243f, 0.00158f, 0.00165f, 0.00093f,
    0.00158f, 0.00095f, 0.00093f, 0.00047f, 0.00095f, 0.00052f, 0.00047f, 0.00021f, 0.00052f, 0.00025f, 0.00021f,
    0.00008f, 0.00025f, 0.00010f, 0.00008f, 0.00002f, 0.00010f, 0.00003f, 0.00002f, 0.00000f, 0.00003f, 0.00000f,
    0.02545f, 0.02542f, 0.02020f, 0.02016f, 0.02542f, 0.02535f, 0.02016f, 0.02006f, 0.02535f, 0.02522f, 0.02006f,
    0.01988f, 0.02522f, 0.02505f, 0.01988f, 0.01965f, 0.02505f, 0.02482f, 0.01965f, 0.01935f, 0.02482f, 0.02452f,
    0.01935f, 0.01899f, 0.02452f, 0.02416f, 0.01899f, 0.01856f, 0.02416f, 0.02372f, 0.01856f, 0.01807f, 0.02372f,
    0.02319f, 0.01807f, 0.01750f, 0.02319f, 0.02256f, 0.01750f, 0.01686f, 0.02256f, 0.02182f, 0.01686f, 0.01613f,
    0.02182f, 0.02095f, 0.01613f, 0.01533f, 0.02095f, 0.01996f, 0.01533f, 0.01445f, 0.01996f, 0.01883f, 0.01445f,
    0.01350f, 0.01883f, 0.01755f, 0.01350f, 0.01248f, 0.01755f, 0.01615f, 0.01248f, 0.01140f, 0.01615f, 0.01462f,
    0.01140f, 0.01027f, 0.01462f, 0.01298f, 0.01027f, 0.00911f, 0.01298f, 0.01129f, 0.00911f, 0.00793f, 0.01129f,
    0.00956f, 0.00793f, 0.00677f, 0.00956f, 0.00787f, 0.00677f, 0.00564f, 0.00787f, 0.00626f, 0.00564f, 0.00456f,
    0.00626f, 0.00479f, 0.00456f, 0.00358f, 0.00479f, 0.00350f, 0.00358f, 0.00270f, 0.00350f, 0.00243f, 0.00270f,
    0.00195f, 0.00243f, 0.00158f, 0.00195f, 0.00133f, 0.00158f, 0.00095f, 0.00133f, 0.00085f, 0.00095f, 0.00052f,
    0.00085f, 0.00050f, 0.00052f, 0.00025f, 0.00050f, 0.00026f, 0.00025f, 0.00010f, 0.00026f, 0.00012f, 0.00010f,
    0.00003f, 0.00012f, 0.00004f, 0.00003f, 0.00000f, 0.00004f, 0.00001f, 0.02020f, 0.02016f, 0.01566f, 0.01562f,
    0.02016f, 0.02006f, 0.01562f, 0.01549f, 0.02006f, 0.01988f, 0.01549f, 0.01530f, 0.01988f, 0.01965f, 0.01530f,
    0.01503f, 0.01965f, 0.01935f, 0.01503f, 0.01471f, 0.01935f, 0.01899f, 0.01471f, 0.01433f, 0.01899f, 0.01856f,
    0.01433f, 0.01389f, 0.01856f, 0.01807f, 0.01389f, 0.01339f, 0.01807f, 0.01750f, 0.01339f, 0.01285f, 0.01750f,
    0.01686f, 0.01285f, 0.01225f, 0.01686f, 0.01613f, 0.01225f, 0.01161f, 0.01613f, 0.01533f, 0.01161f, 0.01092f,
    0.01533f, 0.01445f, 0.01092f, 0.01019f, 0.01445f, 0.01350f, 0.01019f, 0.00942f, 0.01350f, 0.01248f, 0.00942f,
    0.00863f, 0.01248f, 0.01140f, 0.00863f, 0.00782f, 0.01140f, 0.01027f, 0.00782f, 0.00699f, 0.01027f, 0.00911f,
    0.00699f, 0.00617f, 0.00911f, 0.00793f, 0.00617f, 0.00536f, 0.00793f, 0.00677f, 0.00536f, 0.00458f, 0.00677f,
    0.00564f, 0.00458f, 0.00383f, 0.00564f, 0.00456f, 0.00383f, 0.00313f, 0.00456f, 0.00358f, 0.00313f, 0.00249f,
    0.00358f, 0.00270f, 0.00249f, 0.00191f, 0.00270f, 0.00195f, 0.00191f, 0.00142f, 0.00195f, 0.00133f, 0.00142f,
    0.00101f, 0.00133f, 0.00085f, 0.00101f, 0.00067f, 0.00085f, 0.00050f, 0.00067f, 0.00042f, 0.00050f, 0.00026f,
    0.00042f, 0.00024f, 0.00026f, 0.00012f, 0.00024f, 0.00011f, 0.00012f, 0.00004f, 0.00011f, 0.00004f, 0.00004f,
    0.00001f, 0.00004f, 0.00001f, 0.01566f, 0.01562f, 0.01210f, 0.01205f, 0.01562f, 0.01549f, 0.01205f, 0.01192f,
    0.01549f, 0.01530f, 0.01192f, 0.01171f, 0.01530f, 0.01503f, 0.01171f, 0.01143f, 0.01503f, 0.01471f, 0.01143f,
    0.01110f, 0.01471f, 0.01433f, 0.01110f, 0.01072f, 0.01433f, 0.01389f, 0.01072f, 0.01029f, 0.01389f, 0.01339f,
    0.01029f, 0.00983f, 0.01339f, 0.01285f, 0.00983f, 0.00933f, 0.01285f, 0.01225f, 0.00933f, 0.00880f, 0.01225f,
    0.01161f, 0.00880f, 0.00824f, 0.01161f, 0.01092f, 0.00824f, 0.00767f, 0.01092f, 0.01019f, 0.00767f, 0.00707f,
    0.01019f, 0.00942f, 0.00707f, 0.00647f, 0.00942f, 0.00863f, 0.00647f, 0.00586f, 0.00863f, 0.00782f, 0.00586f,
    0.00526f, 0.00782f, 0.00699f, 0.00526f, 0.00466f, 0.00699f, 0.00617f, 0.00466f, 0.00408f, 0.00617f, 0.00536f,
    0.00408f, 0.00353f, 0.00536f, 0.00458f, 0.00353f, 0.00300f, 0.00458f, 0.00383f, 0.00300f, 0.00250f, 0.00383f,
    0.00313f, 0.00250f, 0.00205f, 0.00313f, 0.00249f, 0.00205f, 0.00164f, 0.00249f, 0.00191f, 0.00164f, 0.00127f,
    0.00191f, 0.00142f, 0.00127f, 0.00096f, 0.00142f, 0.00101f, 0.00096f, 0.00069f, 0.00101f, 0.00067f, 0.00069f,
    0.00048f, 0.00067f, 0.00042f, 0.00048f, 0.00031f, 0.00042f, 0.00024f, 0.00031f, 0.00018f, 0.00024f, 0.00011f,
    0.00018f, 0.00009f, 0.00011f, 0.00004f, 0.00009f, 0.00004f, 0.00004f, 0.00001f, 0.00004f, 0.00001f, 0.01210f,
    0.01205f, 0.00949f, 0.00944f, 0.01205f, 0.01192f, 0.00944f, 0.00929f, 0.01192f, 0.01171f, 0.00929f, 0.00907f,
    0.01171f, 0.01143f, 0.00907f, 0.00878f, 0.01143f, 0.01110f, 0.00878f, 0.00845f, 0.01110f, 0.01072f, 0.00845f,
    0.00808f, 0.01072f, 0.01029f, 0.00808f, 0.00767f, 0.01029f, 0.00983f, 0.00767f, 0.00724f, 0.00983f, 0.00933f,
    0.00724f, 0.00679f, 0.00933f, 0.00880f, 0.00679f, 0.00633f, 0.00880f, 0.00824f, 0.00633f, 0.00585f, 0.00824f,
    0.00767f, 0.00585f, 0.00537f, 0.00767f, 0.00707f, 0.00537f, 0.00489f, 0.00707f, 0.00647f, 0.00489f, 0.00442f,
    0.00647f, 0.00586f, 0.00442f, 0.00395f, 0.00586f, 0.00526f, 0.00395f, 0.00350f, 0.00526f, 0.00466f, 0.00350f,
    0.00307f, 0.00466f, 0.00408f, 0.00307f, 0.00266f, 0.00408f, 0.00353f, 0.00266f, 0.00228f, 0.00353f, 0.00300f,
    0.00228f, 0.00192f, 0.00300f, 0.00250f, 0.00192f, 0.00160f, 0.00250f, 0.00205f, 0.00160f, 0.00130f, 0.00205f,
    0.00164f, 0.00130f, 0.00104f, 0.00164f, 0.00127f, 0.00104f, 0.00081f, 0.00127f, 0.00096f, 0.00081f, 0.00061f,
    0.00096f, 0.00069f, 0.00061f, 0.00045f, 0.00069f, 0.00048f, 0.00045f, 0.00031f, 0.00048f, 0.00031f, 0.00031f,
    0.00021f, 0.00031f, 0.00018f, 0.00021f, 0.00013f, 0.00018f, 0.00009f, 0.00013f, 0.00007f, 0.00009f, 0.00004f,
    0.00007f, 0.00003f, 0.00004f, 0.00001f, 0.00003f, 0.00001f, 0.00949f, 0.00944f, 0.00765f, 0.00760f, 0.00944f,
    0.00929f, 0.00760f, 0.00744f, 0.00929f, 0.00907f, 0.00744f, 0.00720f, 0.00907f, 0.00878f, 0.00720f, 0.00691f,
    0.00878f, 0.00845f, 0.00691f, 0.00657f, 0.00845f, 0.00808f, 0.00657f, 0.00621f, 0.00808f, 0.00767f, 0.00621f,
    0.00582f, 0.00767f, 0.00724f, 0.00582f, 0.00542f, 0.00724f, 0.00679f, 0.00542f, 0.00500f, 0.00679f, 0.00633f,
    0.00500f, 0.00459f, 0.00633f, 0.00585f, 0.00459f, 0.00418f, 0.00585f, 0.00537f, 0.00418f, 0.00378f, 0.00537f,
    0.00489f, 0.00378f, 0.00339f, 0.00489f, 0.00442f, 0.00339f, 0.00302f, 0.00442f, 0.00395f, 0.00302f, 0.00266f,
    0.00395f, 0.00350f, 0.00266f, 0.00233f, 0.00350f, 0.00307f, 0.00233f, 0.00201f, 0.00307f, 0.00266f, 0.00201f,
    0.00172f, 0.00266f, 0.00228f, 0.00172f, 0.00146f, 0.00228f, 0.00192f, 0.00146f, 0.00122f, 0.00192f, 0.00160f,
    0.00122f, 0.00100f, 0.00160f, 0.00130f, 0.00100f, 0.00081f, 0.00130f, 0.00104f, 0.00081f, 0.00064f, 0.00104f,
    0.00081f, 0.00064f, 0.00050f, 0.00081f, 0.00061f, 0.00050f, 0.00038f, 0.00061f, 0.00045f, 0.00038f, 0.00028f,
    0.00045f, 0.00031f, 0.00028f, 0.00020f, 0.00031f, 0.00021f, 0.00020f, 0.00013f, 0.00021f, 0.00013f, 0.00013f,
    0.00008f, 0.00013f, 0.00007f, 0.00008f, 0.00005f, 0.00007f, 0.00003f, 0.00005f, 0.00002f, 0.00003f, 0.00001f,
    0.00002f, 0.00001f
};
static const float4 KH_DOME_LUT[768] = {
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00001f, 0.00000f, 0.00002f, 0.00001f, 0.00005f, 0.00002f, 0.00009f, 0.00005f, 0.00019f, 0.00009f,
    0.00029f, 0.00019f, 0.00418f, 0.00029f, 0.00460f, 0.00418f, 0.04736f, 0.00460f, 0.04779f, 0.04736f, 0.12132f,
    0.04779f, 0.12168f, 0.12132f, 0.20639f, 0.12168f, 0.20667f, 0.20639f, 0.28961f, 0.20667f, 0.28981f, 0.28961f,
    0.36307f, 0.28981f, 0.36320f, 0.36307f, 0.42237f, 0.36320f, 0.42244f, 0.42237f, 0.46539f, 0.42244f, 0.46543f,
    0.46539f, 0.49134f, 0.46543f, 0.49135f, 0.49134f, 0.50000f, 0.49135f, 0.50000f, 0.50000f, 0.50866f, 0.50000f,
    0.50865f, 0.50866f, 0.53461f, 0.50865f, 0.53457f, 0.53461f, 0.57763f, 0.53457f, 0.57756f, 0.57763f, 0.63693f,
    0.57756f, 0.63680f, 0.63693f, 0.71039f, 0.63680f, 0.71019f, 0.71039f, 0.79361f, 0.71019f, 0.79333f, 0.79361f,
    0.87868f, 0.79333f, 0.87832f, 0.87868f, 0.95264f, 0.87832f, 0.95221f, 0.95264f, 0.99582f, 0.95221f, 0.99540f,
    0.99582f, 0.99981f, 0.99540f, 0.99971f, 0.99981f, 0.99995f, 0.99971f, 0.99991f, 0.99995f, 0.99999f, 0.99991f,
    0.99998f, 0.99999f, 1.00000f, 0.99998f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00001f, 0.00000f, 0.00000f, 0.00001f, 0.00005f, 0.00000f, 0.00002f, 0.00005f, 0.00015f, 0.00002f, 0.00009f,
    0.00015f, 0.00033f, 0.00009f, 0.00029f, 0.00033f, 0.00079f, 0.00029f, 0.00460f, 0.00079f, 0.00634f, 0.00460f,
    0.04779f, 0.00634f, 0.04962f, 0.04779f, 0.12168f, 0.04962f, 0.12320f, 0.12168f, 0.20667f, 0.12320f, 0.20783f,
    0.20667f, 0.28981f, 0.20783f, 0.29063f, 0.28981f, 0.36320f, 0.29063f, 0.36372f, 0.36320f, 0.42244f, 0.36372f,
    0.42274f, 0.42244f, 0.46543f, 0.42274f, 0.46556f, 0.46543f, 0.49135f, 0.46556f, 0.49138f, 0.49135f, 0.50000f,
    0.49138f, 0.50000f, 0.50000f, 0.50865f, 0.50000f, 0.50862f, 0.50865f, 0.53457f, 0.50862f, 0.53444f, 0.53457f,
    0.57756f, 0.53444f, 0.57726f, 0.57756f, 0.63680f, 0.57726f, 0.63628f, 0.63680f, 0.71019f, 0.63628f, 0.70937f,
    0.71019f, 0.79333f, 0.70937f, 0.79217f, 0.79333f, 0.87832f, 0.79217f, 0.87680f, 0.87832f, 0.95221f, 0.87680f,
    0.95038f, 0.95221f, 0.99540f, 0.95038f, 0.99366f, 0.99540f, 0.99971f, 0.99366f, 0.99921f, 0.99971f, 0.99991f,
    0.99921f, 0.99967f, 0.99991f, 0.99998f, 0.99967f, 0.99985f, 0.99998f, 1.00000f, 0.99985f, 0.99995f, 1.00000f,
    1.00000f, 0.99995f, 0.99999f, 1.00000f, 1.00000f, 0.99999f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00004f, 0.00000f, 0.00000f,
    0.00004f, 0.00013f, 0.00000f, 0.00001f, 0.00013f, 0.00026f, 0.00001f, 0.00005f, 0.00026f, 0.00044f, 0.00005f,
    0.00015f, 0.00044f, 0.00071f, 0.00015f, 0.00033f, 0.00071f, 0.00119f, 0.00033f, 0.00079f, 0.00119f, 0.00243f,
    0.00079f, 0.00634f, 0.00243f, 0.01093f, 0.00634f, 0.04962f, 0.01093f, 0.05462f, 0.04962f, 0.12320f, 0.05462f,
    0.12742f, 0.12320f, 0.20783f, 0.12742f, 0.21107f, 0.20783f, 0.29063f, 0.21107f, 0.29294f, 0.29063f, 0.36372f,
    0.29294f, 0.36522f, 0.36372f, 0.42274f, 0.36522f, 0.42358f, 0.42274f, 0.46556f, 0.42358f, 0.46594f, 0.46556f,
    0.49138f, 0.46594f, 0.49148f, 0.49138f, 0.50000f, 0.49148f, 0.50000f, 0.50000f, 0.50862f, 0.50000f, 0.50852f,
    0.50862f, 0.53444f, 0.50852f, 0.53406f, 0.53444f, 0.57726f, 0.53406f, 0.57642f, 0.57726f, 0.63628f, 0.57642f,
    0.63478f, 0.63628f, 0.70937f, 0.63478f, 0.70706f, 0.70937f, 0.79217f, 0.70706f, 0.78893f, 0.79217f, 0.87680f,
    0.78893f, 0.87258f, 0.87680f, 0.95038f, 0.87258f, 0.94538f, 0.95038f, 0.99366f, 0.94538f, 0.98907f, 0.99366f,
    0.99921f, 0.98907f, 0.99757f, 0.99921f, 0.99967f, 0.99757f, 0.99881f, 0.99967f, 0.99985f, 0.99881f, 0.99929f,
    0.99985f, 0.99995f, 0.99929f, 0.99956f, 0.99995f, 0.99999f, 0.99956f, 0.99974f, 0.99999f, 1.00000f, 0.99974f,
    0.99987f, 1.00000f, 1.00000f, 0.99987f, 0.99996f, 1.00000f, 1.00000f, 0.99996f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00001f, 0.00000f, 0.00000f,
    0.00001f, 0.00011f, 0.00000f, 0.00000f, 0.00011f, 0.00024f, 0.00000f, 0.00000f, 0.00024f, 0.00036f, 0.00000f,
    0.00004f, 0.00036f, 0.00048f, 0.00004f, 0.00013f, 0.00048f, 0.00066f, 0.00013f, 0.00026f, 0.00066f, 0.00094f,
    0.00026f, 0.00044f, 0.00094f, 0.00140f, 0.00044f, 0.00071f, 0.00140f, 0.00217f, 0.00071f, 0.00119f, 0.00217f,
    0.00365f, 0.00119f, 0.00243f, 0.00365f, 0.00724f, 0.00243f, 0.01093f, 0.00724f, 0.02143f, 0.01093f, 0.05462f,
    0.02143f, 0.06645f, 0.05462f, 0.12742f, 0.06645f, 0.13763f, 0.12742f, 0.21107f, 0.13763f, 0.21901f, 0.21107f,
    0.29294f, 0.21901f, 0.29862f, 0.29294f, 0.36522f, 0.29862f, 0.36891f, 0.36522f, 0.42358f, 0.36891f, 0.42567f,
    0.42358f, 0.46594f, 0.42567f, 0.46687f, 0.46594f, 0.49148f, 0.46687f, 0.49171f, 0.49148f, 0.50000f, 0.49171f,
    0.50000f, 0.50000f, 0.50852f, 0.50000f, 0.50829f, 0.50852f, 0.53406f, 0.50829f, 0.53313f, 0.53406f, 0.57642f,
    0.53313f, 0.57433f, 0.57642f, 0.63478f, 0.57433f, 0.63109f, 0.63478f, 0.70706f, 0.63109f, 0.70138f, 0.70706f,
    0.78893f, 0.70138f, 0.78099f, 0.78893f, 0.87258f, 0.78099f, 0.86237f, 0.87258f, 0.94538f, 0.86237f, 0.93355f,
    0.94538f, 0.98907f, 0.93355f, 0.97857f, 0.98907f, 0.99757f, 0.97857f, 0.99276f, 0.99757f, 0.99881f, 0.99276f,
    0.99635f, 0.99881f, 0.99929f, 0.99635f, 0.99783f, 0.99929f, 0.99956f, 0.99783f, 0.99860f, 0.99956f, 0.99974f,
    0.99860f, 0.99906f, 0.99974f, 0.99987f, 0.99906f, 0.99934f, 0.99987f, 0.99996f, 0.99934f, 0.99952f, 0.99996f,
    1.00000f, 0.99952f, 0.99964f, 1.00000f, 1.00000f, 0.99964f, 0.99976f, 1.00000f, 1.00000f, 0.99976f, 0.99989f,
    1.00000f, 1.00000f, 0.99989f, 0.99999f, 1.00000f, 1.00000f, 0.99999f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
    1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 0.00000f, 0.00000f,
    0.00000f, 0.00002f, 0.00000f, 0.00000f, 0.00002f, 0.00017f, 0.00000f, 0.00000f, 0.00017f, 0.00026f, 0.00000f,
    0.00001f, 0.00026f, 0.00031f, 0.00001f, 0.00011f, 0.00031f, 0.00044f, 0.00011f, 0.00024f, 0.00044f, 0.00063f,
    0.00024f, 0.00036f, 0.00063f, 0.00090f, 0.00036f, 0.00048f, 0.00090f, 0.00128f, 0.00048f, 0.00066f, 0.00128f,
    0.00184f, 0.00066f, 0.00094f, 0.00184f, 0.00269f, 0.00094f, 0.00140f, 0.00269f, 0.00404f, 0.00140f, 0.00217f,
    0.00404f, 0.00629f, 0.00217f, 0.00365f, 0.00629f, 0.01047f, 0.00365f, 0.00724f, 0.01047f, 0.01948f, 0.00724f,
    0.02143f, 0.01948f, 0.04220f, 0.02143f, 0.06645f, 0.04220f, 0.08989f, 0.06645f, 0.13763f, 0.08989f, 0.15843f,
    0.13763f, 0.21901f, 0.15843f, 0.23543f, 0.21901f, 0.29862f, 0.23543f, 0.31045f, 0.29862f, 0.36891f, 0.31045f,
    0.37662f, 0.36891f, 0.42567f, 0.37662f, 0.43005f, 0.42567f, 0.46687f, 0.43005f, 0.46882f, 0.46687f, 0.49171f,
    0.46882f, 0.49220f, 0.49171f, 0.50000f, 0.49220f, 0.50000f, 0.50000f, 0.50829f, 0.50000f, 0.50780f, 0.50829f,
    0.53313f, 0.50780f, 0.53118f, 0.53313f, 0.57433f, 0.53118f, 0.56995f, 0.57433f, 0.63109f, 0.56995f, 0.62338f,
    0.63109f, 0.70138f, 0.62338f, 0.68955f, 0.70138f, 0.78099f, 0.68955f, 0.76457f, 0.78099f, 0.86237f, 0.76457f,
    0.84157f, 0.86237f, 0.93355f, 0.84157f, 0.91011f, 0.93355f, 0.97857f, 0.91011f, 0.95780f, 0.97857f, 0.99276f,
    0.95780f, 0.98052f, 0.99276f, 0.99635f, 0.98052f, 0.98953f, 0.99635f, 0.99783f, 0.98953f, 0.99371f, 0.99783f,
    0.99860f, 0.99371f, 0.99596f, 0.99860f, 0.99906f, 0.99596f, 0.99731f, 0.99906f, 0.99934f, 0.99731f, 0.99816f,
    0.99934f, 0.99952f, 0.99816f, 0.99872f, 0.99952f, 0.99964f, 0.99872f, 0.99910f, 0.99964f, 0.99976f, 0.99910f,
    0.99937f, 0.99976f, 0.99989f, 0.99937f, 0.99956f, 0.99989f, 0.99999f, 0.99956f, 0.99969f, 0.99999f, 1.00000f,
    0.99969f, 0.99974f, 1.00000f, 1.00000f, 0.99974f, 0.99983f, 1.00000f, 1.00000f, 0.99983f, 0.99998f, 1.00000f,
    1.00000f, 0.99998f, 1.00000f, 0.00000f, 0.00002f, 0.00000f, 0.00001f, 0.00002f, 0.00017f, 0.00001f, 0.00015f,
    0.00017f, 0.00026f, 0.00015f, 0.00034f, 0.00026f, 0.00031f, 0.00034f, 0.00060f, 0.00031f, 0.00044f, 0.00060f,
    0.00097f, 0.00044f, 0.00063f, 0.00097f, 0.00150f, 0.00063f, 0.00090f, 0.00150f, 0.00225f, 0.00090f, 0.00128f,
    0.00225f, 0.00331f, 0.00128f, 0.00184f, 0.00331f, 0.00487f, 0.00184f, 0.00269f, 0.00487f, 0.00720f, 0.00269f,
    0.00404f, 0.00720f, 0.01080f, 0.00404f, 0.00629f, 0.01080f, 0.01664f, 0.00629f, 0.01047f, 0.01664f, 0.02664f,
    0.01047f, 0.01948f, 0.02664f, 0.04477f, 0.01948f, 0.04220f, 0.04477f, 0.07735f, 0.04220f, 0.08989f, 0.07735f,
    0.12819f, 0.08989f, 0.15843f, 0.12819f, 0.19303f, 0.15843f, 0.23543f, 0.19303f, 0.26317f, 0.23543f, 0.31045f,
    0.26317f, 0.33063f, 0.31045f, 0.37662f, 0.33063f, 0.38985f, 0.37662f, 0.43005f, 0.38985f, 0.43757f, 0.43005f,
    0.46882f, 0.43757f, 0.47218f, 0.46882f, 0.49220f, 0.47218f, 0.49304f, 0.49220f, 0.50000f, 0.49304f, 0.50000f,
    0.50000f, 0.50780f, 0.50000f, 0.50696f, 0.50780f, 0.53118f, 0.50696f, 0.52782f, 0.53118f, 0.56995f, 0.52782f,
    0.56243f, 0.56995f, 0.62338f, 0.56243f, 0.61015f, 0.62338f, 0.68955f, 0.61015f, 0.66937f, 0.68955f, 0.76457f,
    0.66937f, 0.73683f, 0.76457f, 0.84157f, 0.73683f, 0.80697f, 0.84157f, 0.91011f, 0.80697f, 0.87181f, 0.91011f,
    0.95780f, 0.87181f, 0.92265f, 0.95780f, 0.98052f, 0.92265f, 0.95523f, 0.98052f, 0.98953f, 0.95523f, 0.97336f,
    0.98953f, 0.99371f, 0.97336f, 0.98336f, 0.99371f, 0.99596f, 0.98336f, 0.98920f, 0.99596f, 0.99731f, 0.98920f,
    0.99280f, 0.99731f, 0.99816f, 0.99280f, 0.99513f, 0.99816f, 0.99872f, 0.99513f, 0.99669f, 0.99872f, 0.99910f,
    0.99669f, 0.99775f, 0.99910f, 0.99937f, 0.99775f, 0.99850f, 0.99937f, 0.99956f, 0.99850f, 0.99903f, 0.99956f,
    0.99969f, 0.99903f, 0.99940f, 0.99969f, 0.99974f, 0.99940f, 0.99966f, 0.99974f, 0.99983f, 0.99966f, 0.99985f,
    0.99983f, 0.99998f, 0.99985f, 0.99999f, 0.99998f, 1.00000f, 0.99999f, 1.00000f, 0.00000f, 0.00001f, 0.00003f,
    0.00021f, 0.00001f, 0.00015f, 0.00021f, 0.00054f, 0.00015f, 0.00034f, 0.00054f, 0.00103f, 0.00034f, 0.00060f,
    0.00103f, 0.00172f, 0.00060f, 0.00097f, 0.00172f, 0.00269f, 0.00097f, 0.00150f, 0.00269f, 0.00404f, 0.00150f,
    0.00225f, 0.00404f, 0.00593f, 0.00225f, 0.00331f, 0.00593f, 0.00862f, 0.00331f, 0.00487f, 0.00862f, 0.01247f,
    0.00487f, 0.00720f, 0.01247f, 0.01807f, 0.00720f, 0.01080f, 0.01807f, 0.02638f, 0.01080f, 0.01664f, 0.02638f,
    0.03894f, 0.01664f, 0.02664f, 0.03894f, 0.05811f, 0.02664f, 0.04477f, 0.05811f, 0.08692f, 0.04477f, 0.07735f,
    0.08692f, 0.12778f, 0.07735f, 0.12819f, 0.12778f, 0.18010f, 0.12819f, 0.19303f, 0.18010f, 0.23973f, 0.19303f,
    0.26317f, 0.23973f, 0.30087f, 0.26317f, 0.33063f, 0.30087f, 0.35824f, 0.33063f, 0.38985f, 0.35824f, 0.40802f,
    0.38985f, 0.43757f, 0.40802f, 0.44792f, 0.43757f, 0.47218f, 0.44792f, 0.47680f, 0.47218f, 0.49304f, 0.47680f,
    0.49420f, 0.49304f, 0.50000f, 0.49420f, 0.50000f, 0.50000f, 0.50696f, 0.50000f, 0.50580f, 0.50696f, 0.52782f,
    0.50580f, 0.52320f, 0.52782f, 0.56243f, 0.52320f, 0.55208f, 0.56243f, 0.61015f, 0.55208f, 0.59198f, 0.61015f,
    0.66937f, 0.59198f, 0.64176f, 0.66937f, 0.73683f, 0.64176f, 0.69913f, 0.73683f, 0.80697f, 0.69913f, 0.76027f,
    0.80697f, 0.87181f, 0.76027f, 0.81990f, 0.87181f, 0.92265f, 0.81990f, 0.87222f, 0.92265f, 0.95523f, 0.87222f,
    0.91308f, 0.95523f, 0.97336f, 0.91308f, 0.94189f, 0.97336f, 0.98336f, 0.94189f, 0.96106f, 0.98336f, 0.98920f,
    0.96106f, 0.97362f, 0.98920f, 0.99280f, 0.97362f, 0.98193f, 0.99280f, 0.99513f, 0.98193f, 0.98753f, 0.99513f,
    0.99669f, 0.98753f, 0.99138f, 0.99669f, 0.99775f, 0.99138f, 0.99407f, 0.99775f, 0.99850f, 0.99407f, 0.99596f,
    0.99850f, 0.99903f, 0.99596f, 0.99731f, 0.99903f, 0.99940f, 0.99731f, 0.99828f, 0.99940f, 0.99966f, 0.99828f,
    0.99897f, 0.99966f, 0.99985f, 0.99897f, 0.99946f, 0.99985f, 0.99999f, 0.99946f, 0.99979f, 0.99999f, 1.00000f,
    0.99979f, 0.99997f, 0.00003f, 0.00021f, 0.00002f, 0.00053f, 0.00021f, 0.00054f, 0.00053f, 0.00133f, 0.00054f,
    0.00103f, 0.00133f, 0.00250f, 0.00103f, 0.00172f, 0.00250f, 0.00415f, 0.00172f, 0.00269f, 0.00415f, 0.00643f,
    0.00269f, 0.00404f, 0.00643f, 0.00957f, 0.00404f, 0.00593f, 0.00957f, 0.01389f, 0.00593f, 0.00862f, 0.01389f,
    0.01986f, 0.00862f, 0.01247f, 0.01986f, 0.02811f, 0.01247f, 0.01807f, 0.02811f, 0.03955f, 0.01807f, 0.02638f,
    0.03955f, 0.05538f, 0.02638f, 0.03894f, 0.05538f, 0.07706f, 0.03894f, 0.05811f, 0.07706f, 0.10606f, 0.05811f,
    0.08692f, 0.10606f, 0.14323f, 0.08692f, 0.12778f, 0.14323f, 0.18808f, 0.12778f, 0.18010f, 0.18808f, 0.23842f,
    0.18010f, 0.23973f, 0.23842f, 0.29087f, 0.23973f, 0.30087f, 0.29087f, 0.34180f, 0.30087f, 0.35824f, 0.34180f,
    0.38813f, 0.35824f, 0.40802f, 0.38813f, 0.42768f, 0.40802f, 0.44792f, 0.42768f, 0.45913f, 0.44792f, 0.47680f,
    0.45913f, 0.48180f, 0.47680f, 0.49420f, 0.48180f, 0.49545f, 0.49420f, 0.50000f, 0.49545f, 0.50000f, 0.50000f,
    0.50580f, 0.50000f, 0.50455f, 0.50580f, 0.52320f, 0.50455f, 0.51820f, 0.52320f, 0.55208f, 0.51820f, 0.54087f,
    0.55208f, 0.59198f, 0.54087f, 0.57232f, 0.59198f, 0.64176f, 0.57232f, 0.61187f, 0.64176f, 0.69913f, 0.61187f,
    0.65820f, 0.69913f, 0.76027f, 0.65820f, 0.70913f, 0.76027f, 0.81990f, 0.70913f, 0.76158f, 0.81990f, 0.87222f,
    0.76158f, 0.81192f, 0.87222f, 0.91308f, 0.81192f, 0.85677f, 0.91308f, 0.94189f, 0.85677f, 0.89394f, 0.94189f,
    0.96106f, 0.89394f, 0.92294f, 0.96106f, 0.97362f, 0.92294f, 0.94462f, 0.97362f, 0.98193f, 0.94462f, 0.96045f,
    0.98193f, 0.98753f, 0.96045f, 0.97189f, 0.98753f, 0.99138f, 0.97189f, 0.98014f, 0.99138f, 0.99407f, 0.98014f,
    0.98611f, 0.99407f, 0.99596f, 0.98611f, 0.99043f, 0.99596f, 0.99731f, 0.99043f, 0.99357f, 0.99731f, 0.99828f,
    0.99357f, 0.99585f, 0.99828f, 0.99897f, 0.99585f, 0.99750f, 0.99897f, 0.99946f, 0.99750f, 0.99867f, 0.99946f,
    0.99979f, 0.99867f, 0.99947f, 0.99979f, 0.99997f, 0.99947f, 0.99998f, 0.00002f, 0.00053f, 0.00004f, 0.00119f,
    0.00053f, 0.00133f, 0.00119f, 0.00301f, 0.00133f, 0.00250f, 0.00301f, 0.00559f, 0.00250f, 0.00415f, 0.00559f,
    0.00917f, 0.00415f, 0.00643f, 0.00917f, 0.01402f, 0.00643f, 0.00957f, 0.01402f, 0.02052f, 0.00957f, 0.01389f,
    0.02052f, 0.02917f, 0.01389f, 0.01986f, 0.02917f, 0.04058f, 0.01986f, 0.02811f, 0.04058f, 0.05551f, 0.02811f,
    0.03955f, 0.05551f, 0.07476f, 0.03955f, 0.05538f, 0.07476f, 0.09911f, 0.05538f, 0.07706f, 0.09911f, 0.12910f,
    0.07706f, 0.10606f, 0.12910f, 0.16470f, 0.10606f, 0.14323f, 0.16470f, 0.20516f, 0.14323f, 0.18808f, 0.20516f,
    0.24892f, 0.18808f, 0.23842f, 0.24892f, 0.29389f, 0.23842f, 0.29087f, 0.29389f, 0.33780f, 0.29087f, 0.34180f,
    0.33780f, 0.37861f, 0.34180f, 0.38813f, 0.37861f, 0.41474f, 0.38813f, 0.42768f, 0.41474f, 0.44508f, 0.42768f,
    0.45913f, 0.44508f, 0.46902f, 0.45913f, 0.48180f, 0.46902f, 0.48621f, 0.48180f, 0.49545f, 0.48621f, 0.49655f,
    0.49545f, 0.50000f, 0.49655f, 0.50000f, 0.50000f, 0.50455f, 0.50000f, 0.50345f, 0.50455f, 0.51820f, 0.50345f,
    0.51379f, 0.51820f, 0.54087f, 0.51379f, 0.53098f, 0.54087f, 0.57232f, 0.53098f, 0.55492f, 0.57232f, 0.61187f,
    0.55492f, 0.58526f, 0.61187f, 0.65820f, 0.58526f, 0.62139f, 0.65820f, 0.70913f, 0.62139f, 0.66220f, 0.70913f,
    0.76158f, 0.66220f, 0.70611f, 0.76158f, 0.81192f, 0.70611f, 0.75108f, 0.81192f, 0.85677f, 0.75108f, 0.79484f,
    0.85677f, 0.89394f, 0.79484f, 0.83530f, 0.89394f, 0.92294f, 0.83530f, 0.87090f, 0.92294f, 0.94462f, 0.87090f,
    0.90089f, 0.94462f, 0.96045f, 0.90089f, 0.92524f, 0.96045f, 0.97189f, 0.92524f, 0.94449f, 0.97189f, 0.98014f,
    0.94449f, 0.95942f, 0.98014f, 0.98611f, 0.95942f, 0.97083f, 0.98611f, 0.99043f, 0.97083f, 0.97948f, 0.99043f,
    0.99357f, 0.97948f, 0.98598f, 0.99357f, 0.99585f, 0.98598f, 0.99083f, 0.99585f, 0.99750f, 0.99083f, 0.99441f,
    0.99750f, 0.99867f, 0.99441f, 0.99699f, 0.99867f, 0.99947f, 0.99699f, 0.99881f, 0.99947f, 0.99998f, 0.99881f,
    0.99996f, 0.00004f, 0.00119f, 0.00009f, 0.00255f, 0.00119f, 0.00301f, 0.00255f, 0.00631f, 0.00301f, 0.00559f,
    0.00631f, 0.01155f, 0.00559f, 0.00917f, 0.01155f, 0.01855f, 0.00917f, 0.01402f, 0.01855f, 0.02771f, 0.01402f,
    0.02052f, 0.02771f, 0.03947f, 0.02052f, 0.02917f, 0.03947f, 0.05430f, 0.02917f, 0.04058f, 0.05430f, 0.07270f,
    0.04058f, 0.05551f, 0.07270f, 0.09508f, 0.05551f, 0.07476f, 0.09508f, 0.12165f, 0.07476f, 0.09911f, 0.12165f,
    0.15236f, 0.09911f, 0.12910f, 0.15236f, 0.18676f, 0.12910f, 0.16470f, 0.18676f, 0.22401f, 0.16470f, 0.20516f,
    0.22401f, 0.26290f, 0.20516f, 0.24892f, 0.26290f, 0.30204f, 0.24892f, 0.29389f, 0.30204f, 0.34000f, 0.29389f,
    0.33780f, 0.34000f, 0.37552f, 0.33780f, 0.37861f, 0.37552f, 0.40757f, 0.37861f, 0.41474f, 0.40757f, 0.43539f,
    0.41474f, 0.44508f, 0.43539f, 0.45850f, 0.44508f, 0.46902f, 0.45850f, 0.47661f, 0.46902f, 0.48621f, 0.47661f,
    0.48960f, 0.48621f, 0.49655f, 0.48960f, 0.49740f, 0.49655f, 0.50000f, 0.49740f, 0.50000f, 0.50000f, 0.50345f,
    0.50000f, 0.50260f, 0.50345f, 0.51379f, 0.50260f, 0.51040f, 0.51379f, 0.53098f, 0.51040f, 0.52339f, 0.53098f,
    0.55492f, 0.52339f, 0.54150f, 0.55492f, 0.58526f, 0.54150f, 0.56461f, 0.58526f, 0.62139f, 0.56461f, 0.59243f,
    0.62139f, 0.66220f, 0.59243f, 0.62448f, 0.66220f, 0.70611f, 0.62448f, 0.66000f, 0.70611f, 0.75108f, 0.66000f,
    0.69796f, 0.75108f, 0.79484f, 0.69796f, 0.73710f, 0.79484f, 0.83530f, 0.73710f, 0.77599f, 0.83530f, 0.87090f,
    0.77599f, 0.81324f, 0.87090f, 0.90089f, 0.81324f, 0.84764f, 0.90089f, 0.92524f, 0.84764f, 0.87835f, 0.92524f,
    0.94449f, 0.87835f, 0.90492f, 0.94449f, 0.95942f, 0.90492f, 0.92730f, 0.95942f, 0.97083f, 0.92730f, 0.94570f,
    0.97083f, 0.97948f, 0.94570f, 0.96053f, 0.97948f, 0.98598f, 0.96053f, 0.97229f, 0.98598f, 0.99083f, 0.97229f,
    0.98145f, 0.99083f, 0.99441f, 0.98145f, 0.98845f, 0.99441f, 0.99699f, 0.98845f, 0.99369f, 0.99699f, 0.99881f,
    0.99369f, 0.99745f, 0.99881f, 0.99996f, 0.99745f, 0.99991f, 0.00009f, 0.00255f, 0.00018f, 0.00507f, 0.00255f,
    0.00631f, 0.00507f, 0.01225f, 0.00631f, 0.01155f, 0.01225f, 0.02180f, 0.01155f, 0.01855f, 0.02180f, 0.03399f,
    0.01855f, 0.02771f, 0.03399f, 0.04909f, 0.02771f, 0.03947f, 0.04909f, 0.06736f, 0.03947f, 0.05430f, 0.06736f,
    0.08896f, 0.05430f, 0.07270f, 0.08896f, 0.11392f, 0.07270f, 0.09508f, 0.11392f, 0.14206f, 0.09508f, 0.12165f,
    0.14206f, 0.17301f, 0.12165f, 0.15236f, 0.17301f, 0.20617f, 0.15236f, 0.18676f, 0.20617f, 0.24075f, 0.18676f,
    0.22401f, 0.24075f, 0.27584f, 0.22401f, 0.26290f, 0.27584f, 0.31049f, 0.26290f, 0.30204f, 0.31049f, 0.34381f,
    0.30204f, 0.34000f, 0.34381f, 0.37501f, 0.34000f, 0.37552f, 0.37501f, 0.40345f, 0.37552f, 0.40757f, 0.40345f,
    0.42864f, 0.40757f, 0.43539f, 0.42864f, 0.45027f, 0.43539f, 0.45850f, 0.45027f, 0.46811f, 0.45850f, 0.47661f,
    0.46811f, 0.48204f, 0.47661f, 0.48960f, 0.48204f, 0.49202f, 0.48960f, 0.49740f, 0.49202f, 0.49800f, 0.49740f,
    0.50000f, 0.49800f, 0.50000f, 0.50000f, 0.50260f, 0.50000f, 0.50200f, 0.50260f, 0.51040f, 0.50200f, 0.50798f,
    0.51040f, 0.52339f, 0.50798f, 0.51796f, 0.52339f, 0.54150f, 0.51796f, 0.53189f, 0.54150f, 0.56461f, 0.53189f,
    0.54973f, 0.56461f, 0.59243f, 0.54973f, 0.57136f, 0.59243f, 0.62448f, 0.57136f, 0.59655f, 0.62448f, 0.66000f,
    0.59655f, 0.62499f, 0.66000f, 0.69796f, 0.62499f, 0.65619f, 0.69796f, 0.73710f, 0.65619f, 0.68951f, 0.73710f,
    0.77599f, 0.68951f, 0.72416f, 0.77599f, 0.81324f, 0.72416f, 0.75925f, 0.81324f, 0.84764f, 0.75925f, 0.79383f,
    0.84764f, 0.87835f, 0.79383f, 0.82699f, 0.87835f, 0.90492f, 0.82699f, 0.85794f, 0.90492f, 0.92730f, 0.85794f,
    0.88608f, 0.92730f, 0.94570f, 0.88608f, 0.91104f, 0.94570f, 0.96053f, 0.91104f, 0.93264f, 0.96053f, 0.97229f,
    0.93264f, 0.95091f, 0.97229f, 0.98145f, 0.95091f, 0.96601f, 0.98145f, 0.98845f, 0.96601f, 0.97820f, 0.98845f,
    0.99369f, 0.97820f, 0.98775f, 0.99369f, 0.99745f, 0.98775f, 0.99493f, 0.99745f, 0.99991f, 0.99493f, 0.99982f,
    0.00018f, 0.00507f, 0.00035f, 0.00935f, 0.00507f, 0.01225f, 0.00935f, 0.02174f, 0.01225f, 0.02180f, 0.02174f,
    0.03726f, 0.02180f, 0.03399f, 0.03726f, 0.05585f, 0.03399f, 0.04909f, 0.05585f, 0.07746f, 0.04909f, 0.06736f,
    0.07746f, 0.10192f, 0.06736f, 0.08896f, 0.10192f, 0.12896f, 0.08896f, 0.11392f, 0.12896f, 0.15818f, 0.11392f,
    0.14206f, 0.15818f, 0.18907f, 0.14206f, 0.17301f, 0.18907f, 0.22103f, 0.17301f, 0.20617f, 0.22103f, 0.25342f,
    0.20617f, 0.24075f, 0.25342f, 0.28557f, 0.24075f, 0.27584f, 0.28557f, 0.31686f, 0.27584f, 0.31049f, 0.31686f,
    0.34671f, 0.31049f, 0.34381f, 0.34671f, 0.37465f, 0.34381f, 0.37501f, 0.37465f, 0.40027f, 0.37501f, 0.40345f,
    0.40027f, 0.42327f, 0.40345f, 0.42864f, 0.42327f, 0.44345f, 0.42864f, 0.45027f, 0.44345f, 0.46065f, 0.45027f,
    0.46811f, 0.46065f, 0.47479f, 0.46811f, 0.48204f, 0.47479f, 0.48581f, 0.48204f, 0.49202f, 0.48581f, 0.49369f,
    0.49202f, 0.49800f, 0.49369f, 0.49842f, 0.49800f, 0.50000f, 0.49842f, 0.50000f, 0.50000f, 0.50200f, 0.50000f,
    0.50158f, 0.50200f, 0.50798f, 0.50158f, 0.50631f, 0.50798f, 0.51796f, 0.50631f, 0.51419f, 0.51796f, 0.53189f,
    0.51419f, 0.52521f, 0.53189f, 0.54973f, 0.52521f, 0.53935f, 0.54973f, 0.57136f, 0.53935f, 0.55655f, 0.57136f,
    0.59655f, 0.55655f, 0.57673f, 0.59655f, 0.62499f, 0.57673f, 0.59973f, 0.62499f, 0.65619f, 0.59973f, 0.62535f,
    0.65619f, 0.68951f, 0.62535f, 0.65329f, 0.68951f, 0.72416f, 0.65329f, 0.68314f, 0.72416f, 0.75925f, 0.68314f,
    0.71443f, 0.75925f, 0.79383f, 0.71443f, 0.74658f, 0.79383f, 0.82699f, 0.74658f, 0.77897f, 0.82699f, 0.85794f,
    0.77897f, 0.81093f, 0.85794f, 0.88608f, 0.81093f, 0.84182f, 0.88608f, 0.91104f, 0.84182f, 0.87104f, 0.91104f,
    0.93264f, 0.87104f, 0.89808f, 0.93264f, 0.95091f, 0.89808f, 0.92254f, 0.95091f, 0.96601f, 0.92254f, 0.94415f,
    0.96601f, 0.97820f, 0.94415f, 0.96274f, 0.97820f, 0.98775f, 0.96274f, 0.97826f, 0.98775f, 0.99493f, 0.97826f,
    0.99065f, 0.99493f, 0.99982f, 0.99065f, 0.99965f, 0.00035f, 0.00935f, 0.00063f, 0.01579f, 0.00935f, 0.02174f,
    0.01579f, 0.03495f, 0.02174f, 0.03726f, 0.03495f, 0.05725f, 0.03726f, 0.05585f, 0.05725f, 0.08221f, 0.05585f,
    0.07746f, 0.08221f, 0.10937f, 0.07746f, 0.10192f, 0.10937f, 0.13826f, 0.10192f, 0.12896f, 0.13826f, 0.16838f,
    0.12896f, 0.15818f, 0.16838f, 0.19922f, 0.15818f, 0.18907f, 0.19922f, 0.23028f, 0.18907f, 0.22103f, 0.23028f,
    0.26106f, 0.22103f, 0.25342f, 0.26106f, 0.29111f, 0.25342f, 0.28557f, 0.29111f, 0.32002f, 0.28557f, 0.31686f,
    0.32002f, 0.34744f, 0.31686f, 0.34671f, 0.34744f, 0.37306f, 0.34671f, 0.37465f, 0.37306f, 0.39666f, 0.37465f,
    0.40027f, 0.39666f, 0.41806f, 0.40027f, 0.42327f, 0.41806f, 0.43710f, 0.42327f, 0.44345f, 0.43710f, 0.45372f,
    0.44345f, 0.46065f, 0.45372f, 0.46783f, 0.46065f, 0.47479f, 0.46783f, 0.47940f, 0.47479f, 0.48581f, 0.47940f,
    0.48841f, 0.48581f, 0.49369f, 0.48841f, 0.49485f, 0.49369f, 0.49842f, 0.49485f, 0.49871f, 0.49842f, 0.50000f,
    0.49871f, 0.50000f, 0.50000f, 0.50158f, 0.50000f, 0.50129f, 0.50158f, 0.50631f, 0.50129f, 0.50515f, 0.50631f,
    0.51419f, 0.50515f, 0.51159f, 0.51419f, 0.52521f, 0.51159f, 0.52060f, 0.52521f, 0.53935f, 0.52060f, 0.53217f,
    0.53935f, 0.55655f, 0.53217f, 0.54628f, 0.55655f, 0.57673f, 0.54628f, 0.56290f, 0.57673f, 0.59973f, 0.56290f,
    0.58194f, 0.59973f, 0.62535f, 0.58194f, 0.60334f, 0.62535f, 0.65329f, 0.60334f, 0.62694f, 0.65329f, 0.68314f,
    0.62694f, 0.65256f, 0.68314f, 0.71443f, 0.65256f, 0.67998f, 0.71443f, 0.74658f, 0.67998f, 0.70889f, 0.74658f,
    0.77897f, 0.70889f, 0.73894f, 0.77897f, 0.81093f, 0.73894f, 0.76972f, 0.81093f, 0.84182f, 0.76972f, 0.80078f,
    0.84182f, 0.87104f, 0.80078f, 0.83162f, 0.87104f, 0.89808f, 0.83162f, 0.86174f, 0.89808f, 0.92254f, 0.86174f,
    0.89063f, 0.92254f, 0.94415f, 0.89063f, 0.91779f, 0.94415f, 0.96274f, 0.91779f, 0.94275f, 0.96274f, 0.97826f,
    0.94275f, 0.96505f, 0.97826f, 0.99065f, 0.96505f, 0.98421f, 0.99065f, 0.99965f, 0.98421f, 0.99937f, 0.00063f,
    0.01579f, 0.00105f, 0.02419f, 0.01579f, 0.03495f, 0.02419f, 0.05074f, 0.03495f, 0.05725f, 0.05074f, 0.07940f,
    0.05725f, 0.08221f, 0.07940f, 0.10946f, 0.08221f, 0.10937f, 0.10946f, 0.14034f, 0.10937f, 0.13826f, 0.14034f,
    0.17157f, 0.13826f, 0.16838f, 0.17157f, 0.20270f, 0.16838f, 0.19922f, 0.20270f, 0.23336f, 0.19922f, 0.23028f,
    0.23336f, 0.26321f, 0.23028f, 0.26106f, 0.26321f, 0.29197f, 0.26106f, 0.29111f, 0.29197f, 0.31939f, 0.29111f,
    0.32002f, 0.31939f, 0.34526f, 0.32002f, 0.34744f, 0.34526f, 0.36942f, 0.34744f, 0.37306f, 0.36942f, 0.39173f,
    0.37306f, 0.39666f, 0.39173f, 0.41209f, 0.39666f, 0.41806f, 0.41209f, 0.43042f, 0.41806f, 0.43710f, 0.43042f,
    0.44667f, 0.43710f, 0.45372f, 0.44667f, 0.46079f, 0.45372f, 0.46783f, 0.46079f, 0.47275f, 0.46783f, 0.47940f,
    0.47275f, 0.48256f, 0.47940f, 0.48841f, 0.48256f, 0.49019f, 0.48841f, 0.49485f, 0.49019f, 0.49564f, 0.49485f,
    0.49871f, 0.49564f, 0.49891f, 0.49871f, 0.50000f, 0.49891f, 0.50000f, 0.50000f, 0.50129f, 0.50000f, 0.50109f,
    0.50129f, 0.50515f, 0.50109f, 0.50436f, 0.50515f, 0.51159f, 0.50436f, 0.50981f, 0.51159f, 0.52060f, 0.50981f,
    0.51744f, 0.52060f, 0.53217f, 0.51744f, 0.52725f, 0.53217f, 0.54628f, 0.52725f, 0.53921f, 0.54628f, 0.56290f,
    0.53921f, 0.55333f, 0.56290f, 0.58194f, 0.55333f, 0.56958f, 0.58194f, 0.60334f, 0.56958f, 0.58791f, 0.60334f,
    0.62694f, 0.58791f, 0.60827f, 0.62694f, 0.65256f, 0.60827f, 0.63058f, 0.65256f, 0.67998f, 0.63058f, 0.65474f,
    0.67998f, 0.70889f, 0.65474f, 0.68061f, 0.70889f, 0.73894f, 0.68061f, 0.70803f, 0.73894f, 0.76972f, 0.70803f,
    0.73679f, 0.76972f, 0.80078f, 0.73679f, 0.76664f, 0.80078f, 0.83162f, 0.76664f, 0.79730f, 0.83162f, 0.86174f,
    0.79730f, 0.82843f, 0.86174f, 0.89063f, 0.82843f, 0.85966f, 0.89063f, 0.91779f, 0.85966f, 0.89054f, 0.91779f,
    0.94275f, 0.89054f, 0.92060f, 0.94275f, 0.96505f, 0.92060f, 0.94926f, 0.96505f, 0.98421f, 0.94926f, 0.97581f,
    0.98421f, 0.99937f, 0.97581f, 0.99895f, 0.00105f, 0.02419f, 0.00161f, 0.03363f, 0.02419f, 0.05074f, 0.03363f,
    0.06701f, 0.05074f, 0.07940f, 0.06701f, 0.10070f, 0.07940f, 0.10946f, 0.10070f, 0.13416f, 0.10946f, 0.14034f,
    0.13416f, 0.16703f, 0.14034f, 0.17157f, 0.16703f, 0.19904f, 0.17157f, 0.20270f, 0.19904f, 0.22996f, 0.20270f,
    0.23336f, 0.22996f, 0.25963f, 0.23336f, 0.26321f, 0.25963f, 0.28789f, 0.26321f, 0.29197f, 0.28789f, 0.31462f,
    0.29197f, 0.31939f, 0.31462f, 0.33974f, 0.31939f, 0.34526f, 0.33974f, 0.36316f, 0.34526f, 0.36942f, 0.36316f,
    0.38483f, 0.36942f, 0.39173f, 0.38483f, 0.40470f, 0.39173f, 0.41209f, 0.40470f, 0.42275f, 0.41209f, 0.43042f,
    0.42275f, 0.43893f, 0.43042f, 0.44667f, 0.43893f, 0.45322f, 0.44667f, 0.46079f, 0.45322f, 0.46562f, 0.46079f,
    0.47275f, 0.46562f, 0.47612f, 0.47275f, 0.48256f, 0.47612f, 0.48472f, 0.48256f, 0.49019f, 0.48472f, 0.49140f,
    0.49019f, 0.49564f, 0.49140f, 0.49618f, 0.49564f, 0.49891f, 0.49618f, 0.49904f, 0.49891f, 0.50000f, 0.49904f,
    0.50000f, 0.50000f, 0.50109f, 0.50000f, 0.50096f, 0.50109f, 0.50436f, 0.50096f, 0.50382f, 0.50436f, 0.50981f,
    0.50382f, 0.50860f, 0.50981f, 0.51744f, 0.50860f, 0.51528f, 0.51744f, 0.52725f, 0.51528f, 0.52388f, 0.52725f,
    0.53921f, 0.52388f, 0.53438f, 0.53921f, 0.55333f, 0.53438f, 0.54678f, 0.55333f, 0.56958f, 0.54678f, 0.56107f,
    0.56958f, 0.58791f, 0.56107f, 0.57725f, 0.58791f, 0.60827f, 0.57725f, 0.59530f, 0.60827f, 0.63058f, 0.59530f,
    0.61517f, 0.63058f, 0.65474f, 0.61517f, 0.63684f, 0.65474f, 0.68061f, 0.63684f, 0.66026f, 0.68061f, 0.70803f,
    0.66026f, 0.68538f, 0.70803f, 0.73679f, 0.68538f, 0.71211f, 0.73679f, 0.76664f, 0.71211f, 0.74037f, 0.76664f,
    0.79730f, 0.74037f, 0.77004f, 0.79730f, 0.82843f, 0.77004f, 0.80096f, 0.82843f, 0.85966f, 0.80096f, 0.83297f,
    0.85966f, 0.89054f, 0.83297f, 0.86584f, 0.89054f, 0.92060f, 0.86584f, 0.89930f, 0.92060f, 0.94926f, 0.89930f,
    0.93299f, 0.94926f, 0.97581f, 0.93299f, 0.96637f, 0.97581f, 0.99895f, 0.96637f, 0.99839f, 0.00161f, 0.03363f,
    0.00225f, 0.04287f, 0.03363f, 0.06701f, 0.04287f, 0.08175f, 0.06701f, 0.10070f, 0.08175f, 0.11891f, 0.10070f,
    0.13416f, 0.11891f, 0.15434f, 0.13416f, 0.16703f, 0.15434f, 0.18804f, 0.16703f, 0.19904f, 0.18804f, 0.22002f,
    0.19904f, 0.22996f, 0.22002f, 0.25026f, 0.22996f, 0.25963f, 0.25026f, 0.27878f, 0.25963f, 0.28789f, 0.27878f,
    0.30557f, 0.28789f, 0.31462f, 0.30557f, 0.33063f, 0.31462f, 0.33974f, 0.33063f, 0.35396f, 0.33974f, 0.36316f,
    0.35396f, 0.37556f, 0.36316f, 0.38483f, 0.37556f, 0.39544f, 0.38483f, 0.40470f, 0.39544f, 0.41359f, 0.40470f,
    0.42275f, 0.41359f, 0.43003f, 0.42275f, 0.43893f, 0.43003f, 0.44473f, 0.43893f, 0.45322f, 0.44473f, 0.45768f,
    0.45322f, 0.46562f, 0.45768f, 0.46891f, 0.46562f, 0.47612f, 0.46891f, 0.47841f, 0.47612f, 0.48472f, 0.47841f,
    0.48618f, 0.48472f, 0.49140f, 0.48618f, 0.49223f, 0.49140f, 0.49618f, 0.49223f, 0.49655f, 0.49618f, 0.49904f,
    0.49655f, 0.49914f, 0.49904f, 0.50000f, 0.49914f, 0.50000f, 0.50000f, 0.50096f, 0.50000f, 0.50086f, 0.50096f,
    0.50382f, 0.50086f, 0.50345f, 0.50382f, 0.50860f, 0.50345f, 0.50777f, 0.50860f, 0.51528f, 0.50777f, 0.51382f,
    0.51528f, 0.52388f, 0.51382f, 0.52159f, 0.52388f, 0.53438f, 0.52159f, 0.53109f, 0.53438f, 0.54678f, 0.53109f,
    0.54232f, 0.54678f, 0.56107f, 0.54232f, 0.55527f, 0.56107f, 0.57725f, 0.55527f, 0.56997f, 0.57725f, 0.59530f,
    0.56997f, 0.58641f, 0.59530f, 0.61517f, 0.58641f, 0.60456f, 0.61517f, 0.63684f, 0.60456f, 0.62444f, 0.63684f,
    0.66026f, 0.62444f, 0.64604f, 0.66026f, 0.68538f, 0.64604f, 0.66937f, 0.68538f, 0.71211f, 0.66937f, 0.69443f,
    0.71211f, 0.74037f, 0.69443f, 0.72122f, 0.74037f, 0.77004f, 0.72122f, 0.74974f, 0.77004f, 0.80096f, 0.74974f,
    0.77998f, 0.80096f, 0.83297f, 0.77998f, 0.81196f, 0.83297f, 0.86584f, 0.81196f, 0.84566f, 0.86584f, 0.89930f,
    0.84566f, 0.88109f, 0.89930f, 0.93299f, 0.88109f, 0.91825f, 0.93299f, 0.96637f, 0.91825f, 0.95713f, 0.96637f,
    0.99839f, 0.95713f, 0.99775f
};
// KH_ENV_LUT: (A, B) at (N.V, rough), bilinear; every index clamped (NaN reads the first node: saturate).
float2 KhEnvBrdfLut(float khel_ndv, float khel_rough)
{
    const float khel_x = sqrt(saturate((khel_ndv - KH_ENVLUT_NMIN) * (1.0f / (1.0f - KH_ENVLUT_NMIN)))) * 32.0f;
    const float khel_y = sqrt(saturate((khel_rough - KH_LUT_RMIN) * (1.0f / (1.0f - KH_LUT_RMIN)))) * 16.0f;
    const int   khel_ix = min((int)khel_x, 31);
    const int   khel_iy = min((int)khel_y, 15);
    const float khel_tx = khel_x - (float)khel_ix;
    const float khel_ty = khel_y - (float)khel_iy;
    const float4 khel_a = KH_ENVBRDF_LUT[khel_iy * 32 + khel_ix];
    const float4 khel_b = KH_ENVBRDF_LUT[khel_iy * 32 + 32 + khel_ix];
    return lerp(lerp(khel_a.xy, khel_a.zw, khel_tx), lerp(khel_b.xy, khel_b.zw, khel_tx), khel_ty);
}
// KH_ENV_F82: the F82 integral C at (N.V, rough), bilinear on KhEnvBrdfLut's grid and clamps.
float KhEnvBrdfC(float khec_ndv, float khec_rough)
{
    const float khec_x = sqrt(saturate((khec_ndv - KH_ENVLUT_NMIN) * (1.0f / (1.0f - KH_ENVLUT_NMIN)))) * 32.0f;
    const float khec_y = sqrt(saturate((khec_rough - KH_LUT_RMIN) * (1.0f / (1.0f - KH_LUT_RMIN)))) * 16.0f;
    const int   khec_ix = min((int)khec_x, 31);
    const int   khec_iy = min((int)khec_y, 15);
    const float4 khec_q = KH_ENVBRDF_C[khec_iy * 32 + khec_ix];
    return lerp(lerp(khec_q.x, khec_q.y, khec_x - (float)khec_ix), lerp(khec_q.z, khec_q.w, khec_x - (float)khec_ix),
                khec_y - (float)khec_iy);
}
// KH_DOME_LUT: the dome's sky share prefiltered along R at (R.y, rough), bilinear; every index clamped.
float KhDomeSkyShare(float khdq_ry, float khdq_rough)
{
    const float khdq_c = clamp(khdq_ry, -1.0f, 1.0f);
    const float khdq_u = (khdq_c < 0.0f) ? -sqrt(-khdq_c) : sqrt(khdq_c);
    const float khdq_x = saturate(0.5f * khdq_u + 0.5f) * 48.0f;
    const float khdq_y = sqrt(saturate((khdq_rough - KH_LUT_RMIN) * (1.0f / (1.0f - KH_LUT_RMIN)))) * 16.0f;
    const int   khdq_ix = min((int)khdq_x, 47);
    const int   khdq_iy = min((int)khdq_y, 15);
    const float khdq_tx = khdq_x - (float)khdq_ix;
    const float khdq_ty = khdq_y - (float)khdq_iy;
    const float4 khdq_q = KH_DOME_LUT[khdq_iy * 48 + khdq_ix];
    return lerp(lerp(khdq_q.x, khdq_q.y, khdq_tx), lerp(khdq_q.z, khdq_q.w, khdq_tx), khdq_ty);
}

// KH_AMB_MS: the split-sum ambient's response to a unit radiance, shared by the sky ambient (KhPbrAmbient) and
// each lamp's ambient (KhDynLightsPBR): returns the specular albedo E, and both callers' diffuse keeps 1 - E
// (KH_AMB_KD). E is the environment BRDF (F0 * A + B; KH_ENV_LUT, the integral itself) times the
// multiple-scattering energy compensation (Turquin 2019; Fdez-Aguera 2019): Ess = A + B is the single-scatter
// albedo of a white (F = 1) lobe, and Fms = 1 + F0 (1 / Ess - 1) gives back what the lobe loses to
// inter-reflection between microfacets, so a white metal reflects a uniform sky whole at every roughness.
// KH_MS_DIRECT: Fms belongs to the surface (its roughness, its F0, the view's N.V), not to the ambient: every direct
// GGX lobe takes it too - the sun's, the overcast lobe's and each lamp's (KhGGXSpec x Fms), as Turquin 2019,
// Kulla-Conty 2017 and Filament apply it to every light; KhPbrAmbSplitF returns it beside E (priced once per pixel,
// KH_AMB_E_ONCE). KhGGXSpec itself stays the single-scatter lobe. Ess is 0.307 or more over the table (rough 1,
// N.V 1), so the 0.25 floor never binds. F0 is held to [0, 1] in Fms, the published form's range: an arma tint
// above 1 compensates as a white metal does.
// KH_AMB_MS_REF: checked against the Smith microsurface's brute-force multiple scattering (Heitz 2016's random walk)
// for F0 0.04 - 1, gold and copper, rough 0.25 - 1: Turquin's form is the closest of the published ones on this
// table. Do not switch to Fdez-Aguera 2019's FmsEms = Ems FssEss Favg / (1 - Favg Ems) (darker: gold's green at
// rough 1, head-on, 0.510; the walk 0.584; this form 0.642) or Kulla-Conty's Eavg form (a third table; darker too,
// most away from head-on). Residual on the height-correlated table (KH_SMITH_HC), for F0 0.04 - 1, gold and copper at
// rough 0.045 - 1: from rough 0.8 on the F0-linear colour term errs bright head-on (gold's green +0.058 at rough 1);
// below it E is within -0.018 of the walk for N.V >= 0.2, and reads dark at N.V 0.05 (down to -0.046, a dielectric at
// rough 0.2).
float3 KhPbrAmbSplitF(float khms_ndv, float khms_rough, float3 khms_F0, out float3 khms_fmsOut)
{
    const float2 khms_AB = KhEnvBrdfLut(khms_ndv, khms_rough);   // KH_ENV_LUT (A, B).
    // Armed (khFrNK), F0 is the arma tint, and the tint scales the whole conductor curve, the grazing part included
    // (the direct term's tint x curve form). KH_ENV_F82: the curve is integrated in Hoffman 2019's F82 form,
    // r + (1 - r)(1 - c)^5 - a c (1 - c)^6: r its head-on value, a set so that the form meets the curve at c = 1/7
    // (82 deg, Hoffman's fit point) as well; Schlick's form alone (a = 0) misses the curve's dip below it toward
    // grazing (near 82 deg; a conductor's, and a weak interface's - N near 1 - whose curve stays far under Schlick's).
    // It integrates to r A + B - a C (C: KhEnvBrdfC). A branch: khFrNK is per material. KH_F82_SELECT: the term is
    // kept only where the F82 form is closer to the curve than Schlick's at c = 1/4 (75.5 deg), uniform per
    // material. A dielectric-like curve (N about 1.35 - 2.3, small K; fresnel(1.5, 0), the arma default, among
    // them) lies above Schlick's at mid angles, where the term - it lowers the form wherever a > 0, as for these -
    // pulls the wrong way: unconditionally it makes such a material's E worse against the walk (Heitz 2016) at 2 / 3
    // of the points. Over 198 materials (N 0.15 - 3.2, K 0 - 7.3) the test picks the form with the lower mean error
    // against the exact single-scatter integral of the curve for 186 (the 12 misses lose <= 0.0028) and against the
    // walk for 42 of the 45 walked; of the test points swept (1/7 - 0.4), 1/4 leaves the fewest single-scatter nodes
    // worse. No material's mean error exceeds the earlier baseline's (Schlick's form on its own separable table)
    // against either reference; node by node, 6.7 % of the single-scatter nodes (by <= 0.053) and 16 % of the walk's
    // (by <= 0.045) read worse than that baseline - glossy glancing nodes, where its table's under-integration
    // offsets Schlick's overshoot.
    const bool   khms_nk = khFrNK.z >= 0.5f;
    float  khms_f0 = 0.0f;
    float  khms_c = 0.0f;
    float  khms_fa = 0.0f;
    [branch] if (khms_nk) {
        khms_f0 = KhFresnelNK(1.0f, khFrNK.x, khFrNK.y);
        const float khms_f82 = KhFresnelNK(0.14285715f, khFrNK.x, khFrNK.y);   // c = 1/7.
        // a = (Schlick(r) at 1/7 - the curve there) / ((1/7)(6/7)^6); (6/7)^5 = 0.46266437.
        const float khms_a82 = (khms_f0 + (1.0f - khms_f0) * 0.46266437f - khms_f82) * 17.651385f;
        // KH_F82_SELECT: the curve, Schlick's form and the F82 form at c = 1/4; (3/4)^5 = 0.2373046875,
        // (1/4)(3/4)^6 = 0.04449462890625 (both exact in fp32).
        const float khms_c4 = KhFresnelNK(0.25f, khFrNK.x, khFrNK.y);
        const float khms_s4 = khms_f0 + (1.0f - khms_f0) * 0.2373046875f;
        khms_fa = (abs(khms_c4 - (khms_s4 - khms_a82 * 0.04449462890625f)) < abs(khms_c4 - khms_s4)) ? khms_a82 : 0.0f;
        khms_c = KhEnvBrdfC(khms_ndv, khms_rough);
    }
    // KH_ENVBRDF_FLOOR: the table's A, B and C are integrals of non-negative terms (>= 0 at every node, so
    // between them), but the F82 form dips below zero between grazing and head-on where the curve is near 0
    // (N near 1, K 0), and a user may pass a negative F0 or tint: E is held to 0 or more.
    const float3 khms_envBRDF = max(khms_nk ? khms_F0 * (khms_f0 * khms_AB.x + khms_AB.y - khms_fa * khms_c)
                                            : khms_F0 * khms_AB.x + khms_AB.y, 0.0f);
    // KH_AMB_MS: the head-on reflectance (the arma model's tint x curve) and the compensation.
    const float  khms_ess = max(khms_AB.x + khms_AB.y, 0.25f);
    const float3 khms_F0n = saturate(khms_nk ? khms_F0 * khms_f0 : khms_F0);
    const float3 khms_Fms = 1.0f + khms_F0n * (1.0f / khms_ess - 1.0f);
    khms_fmsOut = khms_Fms;   // KH_MS_DIRECT.
    return khms_envBRDF * khms_Fms;
}
// KH_MS_DIRECT: E alone (the public form).
float3 KhPbrAmbSplit(float khms_ndv, float khms_rough, float3 khms_F0)
{
    float3 khms_fms;
    return KhPbrAmbSplitF(khms_ndv, khms_rough, khms_F0, khms_fms);
}

// Engine dynamic lights on the PBR path: modes 1 camera-relative world / 2 view space / 3 absolute, offset +
// a0/a1/a2 attenuation on the scaled distance, the spot cone log/mul/exp pow with its degenerate-exponent guard, and
// the hard range fade. KH_DL_AMB_OCC: occ is the material's occlusion (s.occ), for the per-light ambient alone, as
// KhPbrAmbient takes it for the sky ambient; the direct terms do not take it. KH_AMB_E_ONCE: envE is the ambient
// response E and envFms the lobes' compensation (KhPbrAmbSplitF at this pixel's N.V, KH_MS_DIRECT) when envKnown -
// KhApplyPBR prices them once for the sky, the sun and the lamps, their N and V being the same - else they are
// priced here. KhDynLightsPBR below is the public form.
float3 KhDynLightsPBRE(float3 wpos, float3 nrm, float3 albedo, float3 F0, float rough, float metal, float occ,
                       float3 envE, float3 envFms, bool envKnown)
{
    if (dlCtl.x < 0.5f) return float3(0.0f, 0.0f, 0.0f);
    int pointN = (int)dlCtl.y;
    // KH_DL_RING: no count cap.
    int totalN = pointN + (int)dlCtl.z;
    float3 n = normalize(nrm);
    float3 p;
    float specOn = 1.0f;
    float3 v = float3(0.0f, 0.0f, 1.0f);

    if (dlCtl.x >= 2.5f) {
        // Mode 3: absolute world positions (engine axes); the view vector alone consumes fxParams0 (zeroed camera =
        // diffuse-only).
        p = wpos;
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) specOn = 0.0f;
        else v = normalize(fxParams0.xyz - wpos);
    } else {
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return float3(0.0f, 0.0f, 0.0f);
        p = wpos - fxParams0.xyz;

        if (dlCtl.x >= 1.5f) {
            p = float3(dot(p, dlView[0].xyz), dot(p, dlView[1].xyz), dot(p, dlView[2].xyz));
            n = float3(dot(n, dlView[0].xyz), dot(n, dlView[1].xyz), dot(n, dlView[2].xyz));
        }

        // Camera-at-origin: the fragment sits at p, the camera at 0.
        v = -p / max(length(p), 1.0e-4f);
    }

    float kdM = 1.0f - saturate(metal);
    // KH_DL_AMB_SPEC: a lamp's ambient is a uniform radiance (no direction, no N.L), so it takes the split-sum
    // under a uniform environment: the env-BRDF specular with its compensation (KH_AMB_MS), and the diffuse keeps
    // what that specular (E, the weight returned) does not reflect, kdM x (1 - E) (Fdez-Aguera 2019's kD = 1 - the
    // specular albedo; the sky ambient's KH_AMB_KD): a white dielectric's lamp ambient stays 1 at every angle and
    // roughness. Without a view (mode 3, zeroed camera) there is no lobe and the diffuse stays whole - albedo itself,
    // a metal's included, as KhPbrAmbientE's no-view branch keeps it (KH_DL_AMB_METAL: kdM there would zero a
    // metal's lamp ambient). Priced once per pixel.
    float3 khs_ambK = albedo * (specOn >= 0.5f ? kdM : 1.0f);
    float3 khs_fms = envFms;   // KH_MS_DIRECT: read with the lobes, which run only with a view (specOn).
    if (specOn >= 0.5f) {
        float3 khs_envMs = envE;   // KH_AMB_E_ONCE.
        if (!envKnown) khs_envMs = KhPbrAmbSplitF(saturate(dot(n, v)), rough, F0, khs_fms);
        khs_ambK = khs_ambK * saturate(1.0f - khs_envMs) + khs_envMs;
    }
    khs_ambK *= occ;   // KH_DL_AMB_OCC: the material's occlusion scales ambient light, diffuse and specular.
    float3 acc = float3(0.0f, 0.0f, 0.0f);
    const float khs_fwp = length(fwidth(wpos));   // Footprint priced outside the loop.
    // KH_DLS_GEOM_N: the shadow lookup's normal; n (shading) keeps the N.L term.
    const float3 khs_rn = (dot(khDlsGeomN, khDlsGeomN) > 0.25f) ? khDlsGeomN : nrm;

    [loop] for (int i = 0; i < totalN; ++i) {
        int b = i * 6;
        float3 L = KhDlRec(b + 0).xyz - p;
        float dist = length(L);
        L /= dist + 1e-4f;
        float d = max(dist * dlCtl.w - KhDlRec(b + 4).x, 0.0f);
        float att = saturate(1.0f / (dot(KhDlRec(b + 4).yzw, float3(1.0f, d, d * d)) + 1e-4f));
        att *= 1.0f - saturate((dist * dlCtl.w - KhDlRec(b + 5).x) * KhDlRec(b + 5).y);

        if (i >= pointN) {
            // Spot cone (DynLights' note): the (c > 0) guard stands in for log(0) and dodges the pow(0, 0) NaN.
            float c = saturate((dot(-KhDlRec(b + 1).xyz, L) - KhDlRec(b + 1).w) * KhDlRec(b + 2).w);
            att *= (c > 0.0f) ? pow(c, KhDlRec(b + 3).w) : 0.0f;
        }
        // KH_DL_ATT_SKIP: twin of the DynLights and KhDlsWorldFactor guard.
        if (att <= 0.0f) continue;

        // Twin of the DynLights site. Folding the term into diffI shadows the specular lobe with it (KhGGXSpec is
        // scaled by diffI * pi, KH_PBR_PI): a highlight from a blocked light goes with the light. The per-light
        // ambient stays outside.
        float khs_sh = 1.0f;   // KH_SHADOW_SWITCH, as DynLights (KH_NORECV_BRANCH).
        [branch] if (!(khObjNoRecv >= 0.5f)) {
            khs_sh = KhDlsShadow((int)KhDlRec(b + 5).z - 1, wpos, khs_rn, 0.0f, khs_fwp);
        }
        // Twin of the DynLights site.
        const float khs_amb = lerp(KH_DLS_AMB_KEEP, 1.0f, khs_sh);
        float ndl = max(dot(n, L), 0.0f);
        float3 diffI = dlGlobal.xyz * KhDlRec(b + 2).xyz * ndl * khs_sh;
        // KH_DL_KD_F: the diffuse keeps what the lobe does not reflect, kdM x (1 - F) for every model - KhApplyPBR's
        // sun rule, (1 - F)(1 - metal). The arma model has no metal lane, so there F alone takes a conductor's
        // diffuse (fresnel(1.3, 7) reflects ~90 % head-on). Without a view (mode 3, zeroed camera) there is no lobe
        // and no F, and the diffuse stays whole.
        float3 khsKd = kdM;
        float3 khsSpec = float3(0.0f, 0.0f, 0.0f);

        // KH_DL_NDL_SKIP: specOn is uniform; ndl = max(dot, 0) is an exact zero on a face turned from this light, so
        // diffI and BOTH terms the lobe feeds (khsSpec = GGX * diffI, diffuse = diffI * khsKd) vanish whatever F is -
        // KH_SUN_SKIP_DARK on the per-light side, sparing the conductor Fresnel's two square roots and the
        // D/G/divide. The per-light ambient below is not scaled by ndl and takes no gate.
        if (specOn >= 0.5f && ndl > 0.0f) {
            float3 khsF;
            // KH_DL_RADIUS: the lobe widened by the emitter's angular radius at this distance (dist: metres to the
            // light, before dlCtl.w's attenuation scale) - KhApplyPBR's KH_SUN_DISC rule for a lamp.
            const float khsRw = sqrt(min(rough * rough + KH_DL_RADIUS_M / (3.0f * max(dist, 1.0e-3f)), 1.0f));
            khsSpec = KhGGXSpec(n, v, L, khsRw, F0, khsF)
                    * (khs_fms * diffI * KH_PBR_PI);   // KH_PBR_PI; KH_MS_DIRECT: the compensation.
            khsKd *= saturate(1.0f - khsF);   // KH_DL_KD_F. The arma tint may exceed 1.
        }

        // KH_DL_AMB_METAL / KH_DL_AMB_SPEC: the per-light ambient takes khs_ambK (priced above): kdM on the diffuse (a
        // metal has no diffuse albedo; its albedo lane is its F0 colour) times what the env-BRDF specular leaves, plus
        // that specular.
        acc += (albedo * (diffI * khsKd) + KhDlRec(b + 3).xyz * (khs_amb * khs_ambK) + khsSpec) * att;
    }

    return acc * dlGlobal.w;
}
// KH_DL_AMB_OCC: the public form with the occlusion; E is priced inside.
float3 KhDynLightsPBR(float3 wpos, float3 nrm, float3 albedo, float3 F0, float rough, float metal, float occ)
{
    return KhDynLightsPBRE(wpos, nrm, albedo, F0, rough, metal, occ, float3(0.0f, 0.0f, 0.0f),
                           float3(1.0f, 1.0f, 1.0f), false);
}

// The split-sum ambient: specular from the analytic dome (there is no reflection target), (1 - metal) x (1 - E) on the
// diffuse (KH_AMB_KD) - a metal has no diffuse albedo, so without it a metal goes flat grey-brown once the direct
// term dies (shadow, overcast, dusk).
static const float KH_PBR_GROUND = 0.35f;
// KH_AMB_E_ONCE: the body, given E (khpa_envBRDF: KhPbrAmbSplit at this N.V, the environment BRDF with its
// multiple-scattering compensation, KH_AMB_MS); KhPbrAmbient below is the public form.
float3 KhPbrAmbientE(float3 khpa_n, float3 khpa_v, bool khpa_vOk, float khpa_rough,
                     float khpa_metal, float3 khpa_albedo,
                     float3 khpa_amb, float khpa_occ, float3 khpa_envBRDF)
{
    const float3 khpa_ambo = khpa_amb * khpa_occ;
    if (!khpa_vOk) return khpa_albedo * khpa_ambo;

    // KH_DOME_LUT: the dome prefiltered along R by this roughness's GGX lobe (the split sum's prefiltered radiance,
    // tabulated): the dome itself when smooth, the hemisphere irradiance 0.35 + 0.65 (1 + R.y) / 2 at rough 1 - not a
    // blur toward the dome's mean (0.675 for every R). KH_AMB_MS: the diffuse below reads the flat ambient on purpose -
    // the engine's own (albedo x lightAmb on its surfaces, ApplyLighting's twin); the dome is only the reflection's
    // sky / ground split, and the two are not one irradiance model.
    const float3 khpa_R = reflect(-khpa_v, khpa_n);
    const float3 khpa_env = khpa_amb * lerp(KH_PBR_GROUND, 1.0f, KhDomeSkyShare(khpa_R.y, khpa_rough));

    // KH_AMB_KD: the diffuse keeps 1 - E (Fdez-Aguera 2019's kD; Filament's): a white dielectric under a uniform
    // ambient returns 1 at every N.V and roughness. Do not use the roughness-aware Schlick (1 - Famb): it outgrows E at
    // grazing and darkens silhouettes (0.66 at N.V 0, rough 0.5 under a uniform ambient).
    const float3 khpa_diff = khpa_albedo * khpa_ambo * (1.0f - khpa_metal) * saturate(1.0f - khpa_envBRDF);
    const float3 khpa_spec = khpa_env * khpa_envBRDF * khpa_occ;
    return khpa_diff + khpa_spec;
}
float3 KhPbrAmbient(float3 khpa_n, float3 khpa_v, bool khpa_vOk, float khpa_rough,
                    float3 khpa_F0, float khpa_metal, float3 khpa_albedo,
                    float3 khpa_amb, float khpa_occ)
{
    float3 khpa_E = float3(0.0f, 0.0f, 0.0f);   // KH_AMB_E_ONCE: no view, no specular - E is not read.
    if (khpa_vOk) khpa_E = KhPbrAmbSplit(saturate(dot(khpa_n, khpa_v)), khpa_rough, khpa_F0);
    return KhPbrAmbientE(khpa_n, khpa_v, khpa_vOk, khpa_rough, khpa_metal, khpa_albedo, khpa_amb, khpa_occ, khpa_E);
}

    // Compact GGX (Cook-Torrance specular + Lambert diffuse).
float3 KhApplyPBR(KhMatSurf m, float3 wpos, float3 n, float smf)
{
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return m.albedo * m.occ + m.emissive;
    // Arma model: spec-gloss with its own tint and the conductor Fresnel (khFrNK), armed for every GGX lobe and the
    // ambient.
    const bool khar_nk = m.model >= 0.5f;
    khFrNK = khar_nk ? float4(max(m.fresnelNK, float2(0.0f, 0.0f)), 1.0f, 0.0f) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    const bool khar_sg = m.specOn >= 0.5f || khar_nk;   // Spec-gloss: rough from gloss, no metal.
    float rough = khar_sg ? saturate(1.0f - m.gloss) : saturate(m.rough);
    rough = max(rough, KH_ROUGH_MIN);   // KH_ROUGH_MIN_ONE: KhGGXSpec's floor and the env-BRDF table's first row.
    float3 F0 = khar_nk ? max(m.specTint, float3(0.0f, 0.0f, 0.0f))
              : m.specOn >= 0.5f ? m.specF0
              : lerp(float3(0.04f, 0.04f, 0.04f), m.albedo, saturate(m.metal));
    float metal = khar_sg ? 0.0f : saturate(m.metal);
    float3 l = lighting1.xyz;
    float3 v = normalize(fxParams0.xyz - wpos);
    // KH_AMB_E_ONCE: E and the lobes' compensation Fms (KH_MS_DIRECT) priced once for the sky's ambient, the sun's two
    // lobes and every lamp (one N, V and roughness - the surface's; the sun disc's and the overcast lobe's widening
    // shape the light, not the microsurface). E is read only with a view, when both are priced; without one Fms = 1.
    const bool  khov_vOk = dot(fxParams0.xyz, fxParams0.xyz) >= 1.0f;
    float3 khae_E = float3(0.0f, 0.0f, 0.0f);
    float3 khae_Fms = float3(1.0f, 1.0f, 1.0f);
    if (khov_vOk) khae_E = KhPbrAmbSplitF(saturate(dot(n, v)), rough, F0, khae_Fms);
    float ndl = saturate(dot(n, l));
    // Both sun terms (direct, the overcast lobe below) carry ndl * smf; at 0 the GGX lobes feeding only them are
    // skipped. Exact.
    const bool khsd_lit = ndl * smf > 0.0f;
    float3 direct = float3(0.0f, 0.0f, 0.0f);
    if (khsd_lit) {
        float3 F;
        // KH_SUN_DISC: the sun is a disc - its lobe widened by the disc's angular radius (Karis 2013, "Real Shading in
        // Unreal Engine 4", eq. 10: alpha' = saturate(alpha + sourceRadius / (3 distance)), alpha = rough^2; at
        // infinity sourceRadius / distance = tan(theta_s), so alpha' = alpha + tan(0.2665 deg) / 3 = alpha + 0.00155).
        // D stays normalised: the highlight keeps its energy over the disc's reflection, its peak (alpha / alpha')^2 of
        // a point's - 0.32 at roughness 0.045, 0.75 at 0.1, 0.93 at 0.2, 0.99 at 0.5. The lamps take the same rule at
        // KH_DL_RADIUS_M (KhDynLightsPBRE).
        float3 spec = KhGGXSpec(n, v, l, sqrt(min(rough * rough + KH_SUN_DISC_A, 1.0f)), F0, F);
        float3 kd = saturate(1.0f - F) * (1.0f - metal);   // The arma tint may exceed 1.
        direct = lighting2.rgb * (khObjDif * ndl * smf)
               * (kd * m.albedo + spec * (khae_Fms * KH_PBR_PI));   // KH_PBR_PI; KH_MS_DIRECT.
    }
    // The overcast lobe: under cloud the sun colour falls to a fraction of the ambient and the lobe above is gone, yet
    // the bright patch of cloud around the sun still puts one broad, soft highlight on a metal - the same GGX toward
    // lighting1, roughness floored (an area light), radiance the ambient's, weighted by how dim the sun is against it.
    const float khov_sun = max(lighting2.r, max(lighting2.g, lighting2.b));
    const float khov_amb = max(lightAmb.r, max(lightAmb.g, lightAmb.b));
    const float khov_w = (lightAmb.w >= 0.5f && khov_vOk)
                       ? saturate(1.0f - khov_sun / max(khov_amb, 1.0e-4f)) : 0.0f;
    float3 khov = float3(0.0f, 0.0f, 0.0f);
    if (khsd_lit && khov_w > 0.0f) {   // KH_SUN_SKIP_DARK.
        float3 khov_F;
        khov = KhGGXSpec(n, v, l, max(rough, 0.6f), F0, khov_F) * khae_Fms   // KH_MS_DIRECT.
             * (lightAmb.rgb * khObjAmb) * (khov_w * ndl * smf * KH_PBR_PI);   // KH_PBR_PI: a punctual lobe too.
    }
    // The split-sum ambient replaces albedo * amb; smf is not passed (the sky is not shadowed by our casters). The
    // material's own occlusion map scales the ambient alone - the sky's here, each lamp's in KhDynLightsPBR
    // (KH_DL_AMB_OCC); KH_SSAO multiplies the whole colour after the draw, at the sample. KH_AMB_E_ONCE: E, Fms above.
    const float3 amb = KhPbrAmbientE(n, v, khov_vOk, rough, metal, m.albedo,
                                     lightAmb.rgb * khObjAmb, m.occ, khae_E);
    return amb + KhDynLightsPBRE(wpos, n, m.albedo, F0, rough, metal, m.occ, khae_E, khae_Fms, true)
         + direct + khov + m.emissive;
}
#endif

// KH_DLS_WORLD: the world receives our meshes' dynamic-light shadows. Returns a per-channel MULTIPLY FACTOR for an
// already-shaded world pixel (1 = leave alone, < 1 = this much of its light one of our meshes blocked); the caller
// emits it under a dest*src blend at the scene resolve. A factor because the engine's lit shaders are forward, out =
// albedo * (skyAmb*shadowMask + sun*NdL + Ldyn), with no albedo target to read; in out' = out * (1 - Lblocked/Ltotal)
// the albedo divides out. THE NUMERATOR IS EXACT: the per-light loop is the engine's own dynamic light function
// instruction for instruction (cb11 stride 6, the same offset table, the same 1e-4 guards, the same log/mul/exp cone
// pow, ambient accumulated without N.L). Five copies of that loop exist - DynLights, KhDynLightsPBRE and this one in
// this file, KhDlfAtt in effect.hlsl, kh_dls_atten in the C++ (the distance terms alone, no cone); they differ in what
// they ACCUMULATE, not in how they attenuate, so an edit to the attenuation or the cone in one is an edit to all five.
// Offsets: [b+0] position, [b+1] spot axis + cone threshold, [b+2] diffuse + cone scale, [b+3] ambient + cone exponent,
// [b+4] offset + (a0,a1,a2), [b+5] fade start + inverse width + OUR SHADOW SLOT. THE DENOMINATOR IS GENEROUS: Ltotal
// adds the engine's sky ambient and sun with the sun's own shadow assumed fully lit, so every error fails toward
// UNDER-darkening; in daylight the factor goes to 1. EVERY FAILURE PATH RETURNS 1.0. khw_nrel: the caller's confidence
// in khw_nrm, 0..1 (the one caller, KhDlsWorldMain - the world pass body PSDlsWorld and PSDlsWorldFog share - passes
// the agreement between two baselines of its depth-derived plane, which collapses to the view axis inside
// quantisation plateaus; a caller with an exact normal would pass 1); where the normal is not resolved N.L falls back
// to 1 (the facing prior) on BOTH the numerator's diffuse and the denominator's sun. khw_fwp: length(fwidth(khw_wpos)),
// priced by the caller before any per-pixel exit (KH_DLSW_GRAD).
float3 KhDlsWorldFactor(float3 khw_wpos, float3 khw_nrm, float khw_zunc,
                        float khw_nrel, float khw_fwp)
{
    if (dlCtl.x < 0.5f) return float3(1.0f, 1.0f, 1.0f);
    // Mode 3 only: modes 1/2 decode light positions relative to the camera through lanes this pass does not fill.
    if (dlCtl.x < 2.5f) return float3(1.0f, 1.0f, 1.0f);

    const int khw_pointN = (int)dlCtl.y;
    const int khw_totalN = khw_pointN + (int)dlCtl.z;   // Every light the fill packed (KH_DL_RING).
    const float3 khw_n = normalize(khw_nrm);

    float3 khw_dyn = float3(0.0f, 0.0f, 0.0f);       // What the lights add here.
    float3 khw_blocked = float3(0.0f, 0.0f, 0.0f);   // And how much we take back.

    [loop] for (int khw_i = 0; khw_i < khw_totalN; ++khw_i) {
        const int khw_b = khw_i * 6;
        float3 khw_L = KhDlRec(khw_b + 0).xyz - khw_wpos;
        const float khw_dist = length(khw_L);
        khw_L /= khw_dist + 1e-4f;
        const float khw_d = max(khw_dist * dlCtl.w - KhDlRec(khw_b + 4).x, 0.0f);
        float khw_att = saturate(1.0f / (dot(KhDlRec(khw_b + 4).yzw,
                                             float3(1.0f, khw_d, khw_d * khw_d)) + 1e-4f));
        khw_att *= 1.0f - saturate((khw_dist * dlCtl.w - KhDlRec(khw_b + 5).x)
                                   * KhDlRec(khw_b + 5).y);
        if (khw_i >= khw_pointN) {
            const float khw_c = saturate((dot(-KhDlRec(khw_b + 1).xyz, khw_L)
                                          - KhDlRec(khw_b + 1).w) * KhDlRec(khw_b + 2).w);
            khw_att *= (khw_c > 0.0f) ? pow(khw_c, KhDlRec(khw_b + 3).w) : 0.0f;
        }
        if (khw_att <= 0.0f) continue;

        const float  khw_ndl = lerp(1.0f, max(dot(khw_n, khw_L), 0.0f), khw_nrel);
        const float3 khw_diff = dlGlobal.xyz * KhDlRec(khw_b + 2).xyz * khw_ndl;
        const float3 khw_amb = KhDlRec(khw_b + 3).xyz;
        khw_dyn += (khw_diff + khw_amb) * khw_att;

        // The slot lane (the C++ kh_dls_fill_dl_world writes it: our map's slot + 1); 0 = this light casts no shadow
        // (the zeroed default).
        const int khw_slot = (int)KhDlRec(khw_b + 5).z - 1;
        if (khw_slot < 0) continue;
        const float khw_sh = KhDlsShadow(khw_slot, khw_wpos, khw_n, khw_zunc, khw_fwp);

        if (khw_sh >= 1.0f) continue;   // Fully lit: nothing blocked.
        // Exactly DynLights' shading with the shadow applied, the partial ambient included (KH_DLS_AMB_KEEP); the
        // difference is the blocked share.
        const float3 khw_shaded = (khw_diff * khw_sh
                                 + khw_amb * lerp(KH_DLS_AMB_KEEP, 1.0f, khw_sh)) * khw_att;
        khw_blocked += max((khw_diff + khw_amb) * khw_att - khw_shaded, 0.0f);
    }

    // DynLights' trailing global scale on both sides: the ratio is scale-invariant.
    khw_dyn *= dlGlobal.w;
    khw_blocked *= dlGlobal.w;
    if (dot(khw_blocked, float3(1.0f, 1.0f, 1.0f)) <= 0.0f) return float3(1.0f, 1.0f, 1.0f);

    // Ltotal: the engine's own combine, sun shadow assumed lit (see above).
    const float3 khw_sky = lightAmb.rgb * lighting0.z;
    // The numerator's N.L weighting: at khw_nrel 0 full sun, overstating Ltotal (fails toward under-darkening).
    const float3 khw_sun = lighting2.rgb *
        (lerp(1.0f, max(dot(khw_n, lighting1.xyz), 0.0f), khw_nrel) * lighting0.w);
    const float3 khw_total = khw_dyn + khw_sky + khw_sun;

    return saturate(1.0f - khw_blocked / max(khw_total, 1e-4f));
}
 
struct VSIn  { float3 pos : POSITION; float3 nrm : NORMAL;
#if KH_TEXTURED
    float2 uv : TEXCOORD0; float4 tan : TANGENT;   // 48-byte lanes (layout_tex).
#endif
};
struct VSOut { float4 pos : SV_Position; float3 wpos : TEXCOORD0; float3 nrm : TEXCOORD1;
    // Anchor-relative position, metres-scale through the interpolators (world-absolute wpos at ~7.5 km rounds at
    // +-0.49 mm per frame under camera motion - half a hero texel). The self chain samples with this; wpos stays for
    // every absolute consumer.
    float3 wrel : TEXCOORD4;
    // The object colour rides the interpolators so the pixel twins read one source whether the draw was per object
    // (the VS copies the CB lane) or a batch (the instance lane). Flat across a triangle by construction.
    float4 icol : TEXCOORD5;
    // The per-object lanes (KhObjLoad at every mesh PS entry). Flat per draw or per instance.
    nointerpolation float4 iobj0 : TEXCOORD7;   // amb, dif, creation (KH_USER_LANES), cut.
    nointerpolation float4 iobj1 : TEXCOORD8;   // no-receive (KH_SHADOW_SWITCH), dither, 0, 0.
#if KH_TEXTURED
    float2 uv : TEXCOORD2; float4 tanw : TEXCOORD3;   // World tangent + handedness.
    nointerpolation uint matIx : TEXCOORD6;   // KH_MAT_TABLE: this draw's / instance's entry.
#endif
};

// The shared mesh vertex transform (the sun-depth, mirror, dlsw mask and white vertex shaders do their own). The
// per-object lanes (centre, rebase-relative centre + arm, edge lengths, rotation rows) come from the CB on a
// per-object draw and, on a batch draw, from the object record (khObjs, t39) at the slot the instance lane names (the
// wrapper forms the rebase-relative centre as (pos - khPass) + res); the five entry points (VSMain / VSMainInst,
// VSComposite / VSCompositeInst, and the seam's VSSeamInst) are wrappers that only choose the source. VSInst = the
// 16-byte lane (C++ twin KhInstLane): record slot, LOD dither, final colour alpha, material table base.
struct VSInst {
    uint   islot : TEXCOORD4;
    float3 ilane : TEXCOORD5;    // x = dither, y = alpha, z = material base.
};

// KH_SKIN_POOL - a skin pool chunk (C++ KhSkinPoolChunk): the GPU-skinned poses of many skeletal bindings of one mesh,
// each its whole MeshVertex array (48 bytes: pos 0, nrm 12, uv 24, tan 32) from its own vertex base, viewed as uint4 -
// three elements a vertex. An Sk twin (static.hlsl, composite2.hlsl) pulls its instance's vertex at the record's base
// (KhObjRec res.w) plus the vertex's index, then runs the twin it wraps unchanged (asfloat yields the bits the input
// assembler would fetch from the same bytes for a 32-bit float element), so every instance draws exactly its own
// slice's pose and the bucket draws them all in one call. Every bucket draw passes BaseVertexLocation 0, so SV_VertexID
// is the index's value. VSInstSk is the twins' one input: the lane (VSInst's two elements on slot 1; C++
// layout_inst_sk) and the vertex id - no slot-0 element.
Buffer<uint4> khSkinPool : register(t52);
struct VSInstSk {
    uint   islot : TEXCOORD4;
    float3 ilane : TEXCOORD5;
    uint   vid   : SV_VertexID;
};
VSIn KhSkPull(uint khsp_vid, float khsp_base)
{
    const uint khsp_e = ((uint)khsp_base + khsp_vid) * 3u;
    const uint4 khsp_a = khSkinPool.Load((int)khsp_e);
    const uint4 khsp_b = khSkinPool.Load((int)(khsp_e + 1u));
    VSIn i;
    i.pos = asfloat(khsp_a.xyz);                            // MeshVertex bytes 0 - 11.
    i.nrm = asfloat(uint3(khsp_a.w, khsp_b.x, khsp_b.y));   // 12 - 23.
#if KH_TEXTURED
    const uint4 khsp_c = khSkinPool.Load((int)(khsp_e + 2u));
    i.uv = asfloat(khsp_b.zw);                              // 24 - 31.
    i.tan = asfloat(khsp_c);                                // 32 - 47.
#endif
    return i;
}
VSInst KhSkLane(VSInstSk s)
{
    VSInst n;
    n.islot = s.islot;
    n.ilane = s.ilane;
    return n;
}

float3 KhRotateR(float3 p, float3 r0, float3 r1, float3 r2)
{
    return p.x * r0 + p.y * r1 + p.z * r2;
}

void KhObjRows(out float3 r0, out float3 r1, out float3 r2)
{
    if (objRot0.w < 0.5f) {
        r0 = float3(1.0f, 0.0f, 0.0f);
        r1 = float3(0.0f, 1.0f, 0.0f);
        r2 = float3(0.0f, 0.0f, 1.0f);
    } else {
        r0 = objRot0.xyz;
        r1 = objRot1.xyz;
        r2 = objRot2.xyz;
    }
}

void KhVsCore(float3 khvc_lp, float3 khvc_ln, float3 khvc_ctr, float3 khvc_rel, float khvc_relArm,
              float3 khvc_size, float3 khvc_r0, float3 khvc_r1, float3 khvc_r2,
              out float4 khvc_opos, out float3 khvc_owpos, out float3 khvc_owrel, out float3 khvc_onrm)
{
    float3 wp = khvc_ctr + KhRotateR(khvc_lp * khvc_size, khvc_r0, khvc_r1, khvc_r2);
    // FP32 jitter rebase (centerRel): armed, the camera-relative position goes through the rebased viewProj and the
    // world-absolute fp32 cancellation never enters the position path.
    float3 khvTp = (khvc_relArm > 0.5f)
                 ? (khvc_rel + KhRotateR(khvc_lp * khvc_size, khvc_r0, khvc_r1, khvc_r2))
                 : wp;
    // stenVol2.z selects the vertex path: 1 = the absolute position through the engine VP, 2 = the rebased one, 3 = the
    // seam prepass (the engine-view z remap). Only 3 is written; 1/2 are arms, not the default - a camera-relative
    // engine cb2 would displace an absolute wp by the whole camera vector.
    float4x4 khEngVP = float4x4(engBlk[0], engBlk[1], engBlk[2], engBlk[3]);
    float3   khEngP  = (stenVol2.z >= 1.5f) ? khvTp : wp;
    float4   khClip  = mul(float4(khvTp, 1.0f), viewProj);

    if (stenVol2.z >= 2.5f) {
        float3 khC2 = float3(engBlk[0].z, engBlk[1].z, engBlk[2].z);
        float3 khC3 = float3(engBlk[0].w, engBlk[1].w, engBlk[2].w);
        float  khD3 = dot(khC3, khC3);

        if (khD3 > 1.0e-12f) {
            float khM22 = dot(khC2, khC3) / khD3;
            float khM32 = engBlk[3].z - engBlk[3].w * khM22;
            khClip.z = khM22 * khClip.w + khM32;
        }
        khvc_opos = khClip;
    } else {
        khvc_opos = (stenVol2.z >= 0.5f) ? mul(float4(khEngP, 1.0f), khEngVP)
                                         : khClip;
    }
    // The pop at max view distance is enforced per fragment in the PS (far contract block), not here.
    khvc_owpos = wp;
    // KH_SELF_REL_INTERP: the position against the sun anchor, which the sun matrices subtract. KH_POS_RES: armed, it
    // is built from the camera-relative position (the residual already in it) and the pass camera's offset from the
    // anchor - both small, so wp's world-scale rounding never reaches the self-shadow lookup (VSSunDepth builds its
    // casters the same way). Every armed pass that reads it (the colour passes: injection, flush, PIP, view-model
    // slice) fills khPass with the camera it rebases on; the volume and prime passes arm without reading it.
    khvc_owrel = (khvc_relArm > 0.5f) ? (khvTp + (khPass.xyz - sunOrigin.xyz)) : (wp - sunOrigin.xyz);
    // Non-uniform scale: normals take the inverse scale, then the object rotation (the inverse-transpose of
    // scale-then-rotate for orthonormal R; kh_set_rotation). KH_NRM_FLOOR: the floor only guards a zero axis (1e-12:
    // n / size stays far inside float range); every drawn axis divides by its own size. Do not raise it: a floor of
    // 1e-4 (0.1 mm) gave any axis drawn thinner than that the wrong weight (a flat import, KH_IMPORT_FLAT, drawn at a
    // size below 1 - the tangent of its bent normals' tilt then scaled by 1 / that size).
    khvc_onrm = normalize(KhRotateR(khvc_ln / max(khvc_size, float3(1e-12f, 1e-12f, 1e-12f)),
                                    khvc_r0, khvc_r1, khvc_r2));
}

// The effect unit's depthTex owns t1. KH_RECEIVE_TEX is passed by the white, static and composite compiles only
// (kh_white_ensure, ensure_resources' static and composite tables, ensure_composite_shader, kh_user_mat_ps's three
// material twins, kh_user_mat_vs's vertex stage). EVERY compile of the effect unit (ensure_resources' prewarm rows and
// its four kh_ps_optional pyramid entries, ensure_effect_shader, kh_pip_fx_shader, kh_user_fx_ps's cb + user post-FX)
// passes MSAA_DEPTH and KH_FX_UNIT, never KH_RECEIVE_TEX: that keeps shadowAtlas off depthTex at t1. white.hlsl
// declares no register of its own, so the white unit takes the block below without a collision.
#ifdef KH_RECEIVE_TEX
// The engine's shadow atlas (its cascade passes' depth), sampled with the engine's own per-cascade world->atlasUV+depth
// transforms harvested from its constant uploads - the same compare the engine performs for its own geometry.
Texture2D<float> shadowAtlas : register(t1);
Texture2D<float> shadowBand0 : register(t4);
Texture2D<float> shadowBand1 : register(t5);
Texture2D<float> shadowBand2 : register(t6);
Texture2D<float> shadowBand3 : register(t7);
Texture2D<float> shadowBand4 : register(t8);
Texture2D<float> shadowBand5 : register(t9);
Texture2D<float> shadowBand6 : register(t12);   // Slots 6-7 (t10 is the terrain heightfield).
Texture2D<float> shadowBand7 : register(t13);

// Takes ABSOLUTE world positions: the engine renders camera-relative and its sampling transforms consume that space,
// so fill_lighting_frame_cb folds each entry's own camera origin into its translation (t' = t - M.cam) before the table
// lands here; a camera-relative position would be folded twice.
void ShadowMapSample(float3 rel, out int cascade, out float occluded)
{
    cascade = -1;
    occluded = 0.0f;
    int n = (int)shadowMeta.x;
    if (n <= 0) return;

    for (int c = 0; c < n; ++c) {
        float4 r0 = shadowMats[c * 3 + 0];
        float4 r1 = shadowMats[c * 3 + 1];
        float4 r2 = shadowMats[c * 3 + 2];
        float u = dot(r0.xyz, rel) + r0.w;
        float v = dot(r1.xyz, rel) + r1.w;
        float z = dot(r2.xyz, rel) + r2.w;

        float4 t = shadowTiles[c];
        // Tiles are sorted finest-first; the first cascade whose tile and depth range contain the point decides.
        if (u < t.x || u > t.z || v < t.y || v > t.w) continue;
        if (z <= 0.001f || z >= 0.999f) continue;

        float px1 = 1.0f / shadowMeta.w;
        float2 uv = clamp(float2(u, v), t.xy + px1, t.zw - px1);
        float2 fpx = uv * shadowMeta.w - 0.5f;
        int2 p0 = int2(floor(fpx));
        float2 fr = frac(fpx);
        // KH_SHADOW_GATHER: the texels p0 .. p0 + 1 in one fetch, at the corner they share over the atlas's own
        // size (so the texels are the Loads' whatever shadowMeta.w says); the tile clamp above keeps all four
        // inside the atlas, so the result is the four Loads' bit for bit. w = p0, z = +x, x = +y, y = +xy.
        uint khsa_w, khsa_h;
        shadowAtlas.GetDimensions(khsa_w, khsa_h);
        const float4 khsa_g = shadowAtlas.Gather(khPfSamp, (float2(p0) + 1.0f) / float2(khsa_w, khsa_h));
        float d00 = khsa_g.w;
        float d10 = khsa_g.z;
        float d01 = khsa_g.x;
        float d11 = khsa_g.y;
        float o00 = ((z - d00) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o10 = ((z - d10) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o01 = ((z - d01) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o11 = ((z - d11) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        cascade = c;
        occluded = lerp(lerp(o00, o10, fr.x), lerp(o01, o11, fr.x), fr.y);
        return;
    }
}

float ShadowMapFactor(float3 rel)
{
    // KH_SHADOW_STRENGTH_SKIP: lighting2.w is g_shadow_map_strength, a CB scalar, so this is a UNIFORM branch. Not a
    // rare case: BOTH PIP fills (kh_pip_fx's and the PIP mesh injection's) zero lighting2[3] on purpose, because
    // shadowAtlas is the MAIN view's and is not bound in a PIP pass; without this gate every PIP mesh pixel walked the
    // cascade table and Gathered from an UNBOUND t1 (reads fully occluded) only to multiply by zero. The result is
    // bit-identical either way.
    if (saturate(lighting2.w) <= 0.0f) return 1.0f;
    int cascade;
    float occluded;
    ShadowMapSample(rel, cascade, occluded);
    return 1.0f - occluded * saturate(lighting2.w);
}

// Bilinear compare per tap (the engine's sample_c equivalent): the 2x2 weighted compare resolves every dither cell of
// the atlas's dithered foliage to its coverage fraction. KhBandTap4 is the compare against ONE named band texture; the
// texture selection lives in BandCmpBilin's branch.
float KhBandTap4(Texture2D<float> khbt_m, float2 pos, float z)
{
    float2 f = pos - 0.5f;
    int2 p0 = int2(floor(f));
    float2 fr = frac(f);
    // KH_SHADOW_GATHER: the four texels in one fetch (w = p0, z = +x, x = +y, y = +xy) over the band's own size.
    // Inside the map they are the four Loads' exactly. At the rim (a tap reaching past the edge, which a small
    // band map can do - the disc reaches 1.3 texels past a uv guarded to 0.001) CLAMP returns the edge texel
    // where the Load returned 0, the nearest depth and so a false occlusion: the rule our own maps keep.
    uint khbt_w, khbt_h;
    khbt_m.GetDimensions(khbt_w, khbt_h);
    const float4 khbt_g = khbt_m.Gather(khPfSamp, (float2(p0) + 1.0f) / float2(khbt_w, khbt_h));
    float b00 = ((z - khbt_g.w) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b10 = ((z - khbt_g.z) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b01 = ((z - khbt_g.x) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b11 = ((z - khbt_g.y) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    return lerp(lerp(b00, b10, fr.x), lerp(b01, b11, fr.x), fr.y);
}

// The selection is a BRANCH, not a select: fxc flattens `?:` over the eight band textures into all eight fetches per
// tap (it cannot index a texture object). [branch] on a value uniform across the band costs one taken arm.
float BandCmpBilin(int t, float2 pos, float z)
{
    [branch] if (t == 0) return KhBandTap4(shadowBand0, pos, z);
    [branch] if (t == 1) return KhBandTap4(shadowBand1, pos, z);
    [branch] if (t == 2) return KhBandTap4(shadowBand2, pos, z);
    [branch] if (t == 3) return KhBandTap4(shadowBand3, pos, z);
    [branch] if (t == 4) return KhBandTap4(shadowBand4, pos, z);
    [branch] if (t == 5) return KhBandTap4(shadowBand5, pos, z);
    [branch] if (t == 6) return KhBandTap4(shadowBand6, pos, z);
    return KhBandTap4(shadowBand7, pos, z);
}

float ShadowBandFactor(float3 wpos)
{
    // KH_SHADOW_STRENGTH_SKIP on the band path: the tail multiplies by the same saturate(lighting2.w), so at strength
    // 0 this spares 4 to 16 Gathers (one per tap) through the eight-arm band select for an exact 1. UNIFORM, so the
    // loops below keep whole-warp flow.
    if (saturate(lighting2.w) <= 0.0f) return 1.0f;
    // No [unroll] on these loops (X4575 in this unit); the 'done' flag plus a plain break is the early-out shape - no
    // continue. Slots arrive finest-first; the first containing band wins; bandBorder.w-1 names the physical texture.
    float4 p = float4(wpos, 1.0f);
    float occ = -1.0f;
    float khbf_vz = 0.0f;   // The winning band's view-z, for the far fade.
    int done = 0;

    for (int s = 0; s < 8; ++s) {
        if (done != 0) break;
        if (bandBorder[s].w < 0.5f) { done = 0; }
        else {
            float vz = dot(p, bandView[s * 3 + 2]);

            if (vz >= bandBorder[s].x && vz < bandBorder[s].y) {
                float4 vp4 = float4(dot(p, bandView[s * 3 + 0]), dot(p, bandView[s * 3 + 1]), vz, 1.0f);
                float u = dot(vp4, bandMat[s * 3 + 0]);
                float v = dot(vp4, bandMat[s * 3 + 1]);
                float z = dot(vp4, bandMat[s * 3 + 2]);

                if (u > 0.001f && u < 0.999f && v > 0.001f && v < 0.999f && z > 0.001f && z < 0.999f) {
                    int t = (int)(bandBorder[s].w + 0.5f) - 1;   // w = 1 + texIndex.
                    float2 khbp_hz = frac(wpos.xz * 0.015625f) * 64.0f;
                    float ang = frac(sin(dot(khbp_hz, float2(12.9898f, 78.233f))) * 43758.5469f) * 6.2831853f;
                    float ca = cos(ang);
                    float sa = sin(ang);
                    const float r = 1.3f;
                    float2 base = float2(u, v) * shadowMeta.w;
                    float acc = 0.0f;

                    for (int k = 0; k < 4; ++k) {
                        float2 d0 =
                            (k == 0) ? float2( 0.974844f, 0.756484f)
                          : (k == 1) ? float2(-0.814100f, 0.914376f)
                          : (k == 2) ? float2( 0.945586f,-0.768907f)
                                     : float2(-0.815442f,-0.879125f);
                        // Unrotated; the 12 refinement taps below take the rotation.
                        acc += BandCmpBilin(t, base + d0 * r, z);
                    }

                    if (acc >= 3.999f || acc <= 0.001f) {
                        occ = acc >= 3.999f ? 1.0f : 0.0f;   // Decisive: engine's early-out.
                    } else {
                        for (int k2 = 0; k2 < 12; ++k2) {
                            float2 d0 =
                                (k2 ==  0) ? float2( 0.443233f,-0.975116f)
                              : (k2 ==  1) ? float2(-0.241888f, 0.997065f)
                              : (k2 ==  2) ? float2(-0.915886f, 0.457714f)
                              : (k2 ==  3) ? float2(-0.942016f,-0.399062f)
                              : (k2 ==  4) ? float2(-0.094184f,-0.929389f)
                              : (k2 ==  5) ? float2( 0.791975f, 0.190902f)
                              : (k2 ==  6) ? float2( 0.199841f, 0.786414f)
                              : (k2 ==  7) ? float2( 0.537430f,-0.473734f)
                              : (k2 ==  8) ? float2(-0.264969f,-0.418930f)
                              : (k2 ==  9) ? float2(-0.382775f, 0.276768f)
                              : (k2 == 10) ? float2( 0.344959f, 0.293878f)
                                           : float2( 0.143832f,-0.141008f);
                            float2 off = float2(d0.x * ca - d0.y * sa, d0.x * sa + d0.y * ca);
                            acc += BandCmpBilin(t, base + off * r, z);
                        }

                        occ = acc * 0.0625f;   // /16, the engine's own weight.
                    }

                    khbf_vz = vz;
                    done = 1;
                }
            }
        }
    }

    if (occ < 0.0f) return 1.0f;   // No band covers this depth: lit.
    // Fade at the shadow view distance (without it the shadow snaps in at the cascade's far edge under motion).
    float khbf_mf = 0.0f;
    float khbf_mn = 0.0f;
    for (int khbf_s = 0; khbf_s < 8; ++khbf_s) {
        if (bandBorder[khbf_s].w >= 0.5f && bandBorder[khbf_s].y > khbf_mf) {
            khbf_mf = bandBorder[khbf_s].y;
            khbf_mn = bandBorder[khbf_s].x;
        }
    }
    float khbf_d = khbf_vz;
    if (dot(fxParams0.xyz, fxParams0.xyz) > 1.0f) khbf_d = distance(wpos, fxParams0.xyz);
    if (khbf_mf > 0.0f) {
        occ *= saturate((khbf_mf - khbf_d) / max(0.164f * (khbf_mf - khbf_mn), 1.0f));
    }
    return 1.0f - occ * saturate(lighting2.w);
}
#endif

// Engine distance haze (PSC_HazePars, sky CB row 14): the blue distance wash world geometry carries.
float KhHazeT(float khaz_d, float khaz_wposY, float khaz_camY, float khaz_layerY)
{
    if (hazePars.w < 0.5f) return 1.0f;
    if (khaz_camY < khaz_layerY) {
        if (fogBelow.y < 0.5f) return 1.0f;   // Stand-down below the layer.
        float khaz_bf = saturate((khaz_layerY - khaz_camY) /
                                 (max(khaz_wposY - khaz_camY, 0.0f) + 1.0e-5f));
        float khaz_sa = khaz_d * (1.0f - khaz_bf);   // Path above the layer.
        if (khaz_sa <= 0.0f) return 1.0f;
        float khaz_dn = khaz_sa + 1.0e-5f;
        float khaz_a2 = (abs(khaz_wposY - khaz_layerY) / khaz_dn) * hazePars.z;
        float khaz_I2 = abs(khaz_a2) < 1.0e-12f
                      ? khaz_sa
                      : (1.0f - exp(-khaz_sa * khaz_a2)) / khaz_a2;
        // minY = min(layerY, fragY) - the export's "min r3.z, r1.y, v8.y".
        float khaz_b2 = exp(-(min(khaz_layerY, khaz_wposY) - hazePars.x) *
                            hazePars.z) * hazePars.y;
        return min(exp(-khaz_I2 * khaz_b2), 1.0f);
    }

    float khaz_t   = saturate((khaz_camY - khaz_layerY) /
                              (max(khaz_camY - khaz_wposY, 0.0f) + 1.0e-5f));
    float khaz_s   = khaz_d * khaz_t;   // Path above the layer.
    float khaz_den = khaz_s + 1.0e-5f;
    float khaz_hi  = max(khaz_layerY, khaz_wposY);
    float khaz_a   = ((khaz_hi - khaz_camY) / khaz_den) * hazePars.z;   // Signed.
    // Integral of exp(-a x) over [0, s]; the a -> 0 limit is s itself.
    float khaz_I   = abs(khaz_a) < 1.0e-12f
                   ? khaz_s
                   : (1.0f - exp(-khaz_s * khaz_a)) / khaz_a;
    float khaz_b   = exp(-(khaz_camY - hazePars.x) * hazePars.z) * hazePars.y;

    return min(exp(-khaz_I * khaz_b), 1.0f);
}
// ===========================================================================
// KH_FX_UNIT - the effect unit's own declarations and helpers, compiled into the builtin effect shader (effect.hlsl,
// effect2.hlsl and effect3.hlsl follow this file) and into every user effect shader (the USER SHADER CONTRACT's EFFECT
// section) - one text for both, so a user effect reads the scene, the depth (the near-plane marker, KH_VM_SEE and
// KH_WSEE included), the pyramid and the fog (the engine's, and the scene's fog passes where khFxFog is filled) as the
// builtins do, scatters by fog scatter's own rules, and can begin and finish its pixel with the builtin's own lines
// (KhFxBegin / KhFxFinish, below). What stays the builtins' is built for one pass and bound around it: the side values
// (fog scatter's per-pixel fog, the sun flare's visibility, the UI probe), the linear-depth copies, the SSGI /
// dynamicLightFog grids, the anamorphic atlas and the LUT. Fenced: the mesh units declare other resources at t0 - t3
// (static.hlsl, composite.hlsl).
// ===========================================================================
#if KH_FX_UNIT
Texture2D<float4> sceneColor : register(t0);
Texture2D<float4> khsgTex : register(t3);
// s2, not s1: khPfSamp holds s1 (declared earlier in this file), so a second s1 fails the compile with X4509 the moment
// any entry reaches a helper that uses it (the shadow compares' Gather, KH_SHADOW_GATHER). C++ twin (KH_FX_SAMP_S2).
// Bound only for the draws that read it, the prior binding put back: the SSGI pass's linear clamp (its seed, gather
// and resolve), or a glow pass's anisotropic clamp (KH_GLOW_PYR, C++ KhGlowBind); for a user effect only when its
// pyramid is.
SamplerState khsgSamp : register(s2);

// KH_UI_DEPTH_SCALE: the UI lane (centerSize.w above 1: 1.25 / 2 / 3) draws in back-buffer pixels (fxMeta.zw is
// the back buffer's size) while t1 is the scene's depth at the 3D render size - display x Sampling. Where the
// two differ the pixel is mapped by the ratio (the 3D picture fills the back buffer): its centre, scaled, then
// truncated. Equal sizes give px back exactly ((px + 0.5) x a ratio within an ulp of 1). Every other route
// draws in the depth's own pixels and keeps px. Every reader of t1 goes through LoadDepthRaw but KhOlRawT (effect.hlsl:
// the UI lane's taps, at depth texels it maps itself) and the KH_WSEE helpers' direct loads (never armed on the UI
// lane).
#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t1);
int2 KhDepthPx(int2 px)
{
    [branch] if (centerSize.w > 1.125f) {
        uint khdp_w, khdp_h, khdp_n;
        depthTex.GetDimensions(khdp_w, khdp_h, khdp_n);
        if (khdp_w > 0u && fxMeta.z >= 1.0f && fxMeta.w >= 1.0f)
            px = int2((float2(px) + 0.5f) * float2((float)khdp_w / fxMeta.z, (float)khdp_h / fxMeta.w));
    }
    return px;
}
float LoadDepthRaw(int2 px) { return depthTex.Load(KhDepthPx(px), 0); }
#else
// The live depth, not the snapshot: a single-channel view (the flush's depth_srv, the PIP's pip_depth_srv) under a
// float2 declaration; LoadDepthRaw reads .x alone (the C++ note at flush_locked's ps_srvs).
Texture2D<float2> depthTex : register(t1);
int2 KhDepthPx(int2 px)
{
    [branch] if (centerSize.w > 1.125f) {
        uint khdp_w, khdp_h;
        depthTex.GetDimensions(khdp_w, khdp_h);
        if (khdp_w > 0u && fxMeta.z >= 1.0f && fxMeta.w >= 1.0f)
            px = int2((float2(px) + 0.5f) * float2((float)khdp_w / fxMeta.z, (float)khdp_h / fxMeta.w));
    }
    return px;
}
float LoadDepthRaw(int2 px) { return depthTex.Load(int3(KhDepthPx(px), 0)).x; }
#endif
// KH_NEARZ_MARK: our near-z fragments nearer than the near plane are written into a gap below the viewport's MinDepth
// that this pair reads as a flat wall at the near distance. The marker (t37, armed by shadowMeta2.x) holds (raw, true
// distance) where a drawer of ours wrote one this cycle; where its raw is still the live raw, that distance comes back
// re-encoded through this pass's own pair, so LinDepth (and every reader of this load) gets the true one. Any other
// pixel - the hands drawn over since, the world - loads as it is (but for KH_VM_SEE, below).
Texture2D<float2> khNzMark : register(t37);
// KH_VM_SEE: past the view model (the scene chain at a later resolve, KH_FX_LATE - MSAA), a sight's translucent planes
// (a 3D scope's eyepiece over its whole aperture, a holosight's glass) have written their own near depth over the scene
// they show, so every reader here saw a wall a few centimetres away. khVmSee holds the raw depth (sample 0,
// LoadDepthRaw's) as the view model's first translucent draw found it - the world, or the view model's opaque parts,
// which its depth prepass has drawn by then - and khVmT the planes' transmittance (1 = none: the product of 1 - alpha
// over the ones that blend over with it). Armed by shadowMeta2.w: 1 = LoadDepthPS takes the snapshot where the live raw
// is below the world's range (depthParams.z) and the snapshot's is in it - after the marker, whose pixels (and our
// gap's, below the range in the snapshot too) keep theirs; 2 = KhFxFinish also fades the pass to the scene there by the
// transmittance (KhVmSeeT, below). Armed only with depthParams.z above 0, so an unbound view (null: loads 0) or a pixel
// off the texture applies neither.
Texture2D<float> khVmSee : register(t34);
Texture2D<float> khVmT : register(t41);
// KH_WSEE - the record the chain reads (C++: the KH_WSEE design note in rendering_integration_injection.hpp holds the
// targets, the pass table, the register map and the lane bits; below, this unit's registers). The world's blended
// draws (DestBlend INV_SRC_ALPHA, no A2C: fences, foliage, glass, particles) leave the scene behind them in the colour
// while the depth holds their own (fence-type) or nothing (particle-type). C++ captures at the cycle's first such draw:
// khWsS the raw depth then (sample 0), khWsB the scene colour then (resolved only where a colour reader wants it,
// KH_WSEE_BWANT); khWsT (RGBA8 at the depth's sample count) cleared (1, 0, 1, 1) and khWsN / khWsN2 (1x raw) cleared 1.
// Each fence-type draw is replayed into T.R (dest *= 1 - alpha, EQUAL: the samples it wrote); each particle-type draw
// into T.B (pre) or T.A (post, after the cycle's B2 moment) under LESS_EQUAL and depth-only into N / N2 (LESS, write:
// the nearest such layer); each late opaque drawn after the B2 moment resets T to (1, 1, 1, 1) at the samples it wrote
// (G = 1: S is stale there) - one drawn before it is derived (KH_WSEE_NOPRE, the note at khWsS2); our translucent tail
// likewise. A light-adding draw (DestBlend ONE) goes into N alone (its light is in S beyond T B, fogged at the front);
// a test-off over-blend into T untested. Multiply, screen, subtractive, MIN / MAX stay out. khWsR is the chain's input
// capture, khWsE the class word (KhWseeClassWord), khWsC (MS) the scene's sample colours at the chain's capture, khWsB2
// / khWsS2 the colour and depth at the B2 moment (bit 3 / 9), khWsTp / khWsTf the split (bit 4), khWsD3 the late
// opaques' own depth (bit 5), khWsP0 / P1 / Nb / Nb2 the one pass's record (bits 6 / 7). KhWseeAvail() lists the bits.
// PER PIXEL (KhWseeRule / KhWseeAt): r = LoadDepthPS's raw (the marker and KH_VM_SEE applied), n = min(N, N2) (as
// positions: KhWseeNwPos), s = S, (Tw, G, Tpre, Tpost) = T at sample 0. front f = min(r, n); behind b = G ? D3 (bit 5;
// else r) : s (S2 for a derived pre; r, G = 1, for a sample off sample 0's surface or a sample 0 written after the
// snapshot by a draw the record holds nothing of, where Tw = 1 - KH_WSEE_SURF_BEHIND); COVERED = armed, f >=
// depthParams.z (the world's range) and b > f. No threshold on Tr: Tr = 1 with b > f is a fully transparent layer. A
// covered pixel blends what the pass derives from depth, Q = Tr Q(b) + (1 - Tr) Q(f) with Tr = Tw Tpre Tpost - or, with
// two or more layer groups, front to back by group (below): KhFxFinish's localization and band masks, Pulse's ring, the
// sun flare's taps, fog scatter's share (KhFsFogW), dynamicLightFog's march (one march to b, a second sum cut at f) and
// the distance fog (13). SSGI keeps the front depth. Uncovered pixels are unchanged; every KH_WSEE load sits behind the
// arm.
// THE DISTANCE FOG (13), exact where G = 0: S = Tr B + E (E the layers' own light): out = Tq Fb2 + (1 - f_front)
// max(S - Tq B2', 0) + (1 - Tq) f_front g with Fb2 = Tpre lerp(B', g, f_b) + (1 - f_pre) max(B2' - Tpre B', 0) +
// (1 - Tpre) f_pre g, Tq = Tw Tpost, f_front = f(min(r, NWpost)), f_pre = f(NWpre) where it lies before the behind,
// else f_b, B' = max(B + (S - R), 0), B2' likewise (the earlier chain passes' change given to the background); without
// B2 one stage over B with T = the product. Exact for one layer a stage in the first colour-changing pass. G = 1 and
// KH_VM_SEE's planes: the factor blend lerp(S, g, Tr f(b) + (1 - Tr) f(f)). STACKING CAP (KhWseeCap, wherever B or B2
// is read): per channel the record cannot show more background than the pixel holds, S >= T X, so T is held to
// min_c S_c / X_c over X_c > KH_WSEE_CAP_EPS (it hides the symptom of one draw's layers overlapping; per sample against
// the sample's own colour).
// PER SAMPLE (MSAA_DEPTH; KhWseeCount / KhWseeSampleAt / KhWseeMix): where a pixel's samples disagree - a T texel
// differs bit for bit, or a raw lies off sample 0's surface (KhWseeSame: a plane's samples differ bit for bit, so
// each is predicted from sample 0 by the pixel's raw-depth gradient) - khWsE says so where bound (bit 0), else the
// inline test (KhWseeDisagree); the masks, Pulse, the flare taps, fog scatter's share and the distance fog take each
// sample k < min(count, KH_WSEE_NS) by the rule on its own r_k and T_k (n, s the pixel's) and average - the builtins
// one pair a class, weighted by its members (KhWseeCountW); KhWseeCount (the user contract) every sample. S is sample
// 0's, so a G = 0 sample is covered only with evidence of its own (Tr_k < 1, or n < r_k) or where it lies on sample
// 0's surface and sample 0 holds a depth-writing layer (s > r_0). The distance fog is exact per sample with khWsC
// (bit 1): o_k on C_k' = C_k + (S - R), out = mean_k o_k. dynamicLightFog and SSGI stay per pixel (sample 0). The
// marker and KH_VM_SEE are sample-0 constructs: a pixel LoadDepthPS changed keeps the per-pixel path.
// ARMED by khFxTexLay[1].z = 1 (uniform per draw, only on the fullscreen lane: centerSize.w = 1) - an armed scene-chain
// pass, its side draws and its agreement mask (C++ kh_fx_chain_run). LoadDepthPS stays the depth buffer's; the scene
// chain draws in the depth's own pixels (KhDepthPx).
// LAYER GROUPS (khWsGz / khWsGt, khWsGz2 / khWsGt2; KhWseeGroupsSet8): the fence-type layers (Tw at r), and per
// phase the part at its NW position (Tm), the 25 % band's part (Tx at KhWseeNearRaw: the band's middle) and the
// 100 % band's (Tx2 at KhWseeNearRaw2) - the pre phase measured from the behind, the post from r - or, where r lies
// beyond D (KhWseeFarRaw) the far part (Tx at max(D, the phase's NW)) where Tf >= T - 1.5 / 255 (Tf has no occlusion
// test). Near: Tm = Tp held to [T, 1], Tx = T / Tm (Tp's layers are a subset of T's: same draws, fewer fragments);
// Tm2 = Tp2 held to [Tm, 1], Tx2 = Tm / Tm2; far: Tx = Tf held to [T, 1], Tm = T / Tx. A part within one UNORM8 step
// of none is no group. Up to seven groups; with Tp = T and Tf = 1 only the three NW groups stand. Two or more: Q =
// (1 - T1) Q(z1) + T1 (1 - T2) Q(z2) + ... + prod T_i Q(behind) (KhWseeMix, dynamicLightFog's cuts, effect 13's
// factor, KhWseeFogMean, KhWseeGroups7); one: the two-depth blend with the group's own depth as the front where that
// lies behind min(r, n) (KhWseeOneFront: a faded sprite leaves T = 1 but sets NW); none: the front.
// NOT COVERED: one draw's layers overlapping one another (T counts each fragment, N the nearest); two layers of one
// phase both off any surface share the nearer's depth (one NW a phase); a fence-type layer over a late opaque surface
// not coplanar with it where D3 does not stand (behind = r: uncovered); a particle-type layer drawn after B2 but
// behind a later fence (post: fogged at the fence); at a per-sample pixel a fully transparent writer's texel off
// sample 0's surface (uncovered), and a sample within KhWseeSame's tolerance of a layer at sample 0 (a contact line);
// a pass both localized and banded multiplies the two averaged masks; layers over the cleared depth are never
// absorbed by the 2 % push; the effect meshes (unarmed); FSAA 1x (the capture does not land).
Texture2D<float> khWsS : register(t53);
#if MSAA_DEPTH
Texture2DMS<float4> khWsT : register(t54);   // RGBA - Tw, G, Tpre, Tpost.
#else
Texture2D<float4> khWsT : register(t54);
#endif
Texture2D<float4> khWsB : register(t55);
Texture2D<float4> khWsR : register(t56);
Texture2D<float> khWsN : register(t57);   // NWpre.
Texture2D<uint> khWsE : register(t58);   // KH_WSEE_EDGE_COST: the class word (KhWseeClassWord).
#if MSAA_DEPTH
Texture2DMS<float4> khWsC : register(t59);
#endif
Texture2D<float4> khWsB2 : register(t60);   // B2, where KhWseeAvail() bit 3 says it was taken.
Texture2D<float> khWsN2 : register(t61);    // NWpost.
Texture2D<float> khWsS2 : register(t62);    // S2 (the B2 moment's depth), bound where KhWseeAvail() bit 9 says.
// KH_WSEE_NOPRE: a late opaque marked before the B2 attempt is not replayed; where bit 8 stands the rule derives it.
// Sample 0 is pre where S2 != S (bit 9: S2 bound, taken at the B2 moment whether B2 landed; only in the world's range
// - the view-model slice writes the depth untracked, so there and without S2 the live r0 < S decides) and no post
// opaque owns it (G 0); a sample k > 0 where sample 0 is and it lies on sample 0's surface (KhWseeSame). It reads as
// a replayed pre sample did: behind S2 where B2 (bit 3) or D3 (bit 5) stands, else the live depth; G 0.5 with B2, else
// 1; the old record's pre channels (Tpre, Tp's R / B) 1 where its NWpre lies behind that behind - the replay's EQUAL
// reset at read time (KH_PSB's rule for its own record). Differs from a replay: a pre layer drawn before the opaque
// and lying in front of it is kept (and any of its phase behind it drawn before it); an off-plane edge sample reads
// G = 0 with S behind; an opaque that wrote S's own value at sample 0 is no pre sample; a test-off over-blend after
// a pre opaque and behind it is reset; the class word reads T's bytes, which no replay reset (a pixel whose Tpre
// differed between samples under a pre opaque reads more classes); a depth write the hooks do not track between the
// capture and the B2 moment reads as a pre late opaque (the premise: only tracked late opaques write the depth
// there). Bit 8 clear: the rule without this derivation.
// Bit 4. Tp: the particle-type layers counting only fragments NOT within the near band of the surface behind them
// (R pre, G post; B / A the 100 % band), at the depth's sample count; Tf: the fragments beyond D (R pre, G post), 1x.
#if MSAA_DEPTH
Texture2DMS<float4> khWsTp : register(t63);
#else
Texture2D<float4> khWsTp : register(t63);
#endif
Texture2D<float2> khWsTf : register(t64);
// D3 (bit 5): the main depth's sample 0 raw at the last time a late opaque owned sample 0 (no value of this cycle
// elsewhere: never read there); a post one's is resolved from its own interpolated depth z (KH_WSEE_OQ2,
// PSWseeD3Res: the live sample 0 where it lies within KH_WSEE_D3_TOL of z, else z).
Texture2D<float> khWsD3 : register(t65);
// KH_PSB - the one pass's record (C++ kh_psb_post), where bit 6 says a re-issue drew into it: a particle-type draw
// whose shader has the one-pass variant is drawn ONCE, single sample, its five passes' depth verdicts made in its
// shader at the block's centre against the largest sample 0 of its 2 x 2 block (KH_PSB_LORES below), into khWsP0 (t66)
// and khWsNb (t68) for a pre layer, khWsP1 / khWsNb2 (t67 / t69) for a post one (KH_PSB_PHASE): P = (T, the 25 %
// band's, the 100 % band's, Tf's) of the phase, N its NW (the same pushed raws, MIN). Each channel is dest *= 1 - a
// under INV_SRC_COLOR or, the alpha channel, INV_SRC_ALPHA - the same factor, so the same byte GIVEN that the blend
// unit rounds both alike (D3D11 defines one formula and promises nothing of the precision; failing it, an LSB).
// Draws the one pass cannot take (no variant, a partial sample mask, a depth bias) and our own tail draw the replay
// passes into T, Tp, Tf and N - "the old record"; bit 7 says it holds no particle-type layer this cycle (its N / Tp /
// Tf not loaded). Each phase's channel is the product of both records' and its NW the nearer (min), so every consumer
// below the rule's loads is unchanged; bit 6 clear = the old record bit for bit.
// The late-opaque reset as a READ-TIME RULE: P is single sample and gets no reset, so at a sample G marks a phase
// whose nearest one-pass layer (Nb, pushed) lies behind the sample's behind b reads 1 in every one-pass channel of it
// (every layer of the phase would fail its own test against b). Exact where the phase's layers all lie behind the
// opaque or were all drawn after it; a layer drawn before it and lying in front is kept (with any of its phase behind
// it drawn before it). The Tf channel is reset too (harmless: a reset phase reads T = 1, and the far branch then
// gives Tx = Tm = 1). The old record keeps the replay's reset. Else: the one pass's channels are the pixel's at every
// sample, its band verdicts the block's, its pushes the shader's (z m + a: ulps from the viewport shift's), and it
// rasterizes at one sample (a sprite edge covers the pixels whose centre it covers).
// The alpha is the engine's own instructions' (KH_PSV's alpha variant: the same rounding caveat).
Texture2D<float4> khWsP0 : register(t66);
Texture2D<float4> khWsP1 : register(t67);
Texture2D<float> khWsNb : register(t68);
Texture2D<float> khWsNb2 : register(t69);   // KH_PSB_PHASE: NWpost.
// KH_PSB_LORES (twin of the mesh unit's KhWseeMsPsbAt): the one pass draws at 1 / 2 the frame a side (KH_WSEE_PSB_LS),
// so the record's texel for pixel px is px >> 1, clamped to floor(W / 2) - 1 (W = khWsS's size): the one pass
// certainly writes that texel, while the texel past it may be undrawn or partial at an odd W (KH_PSB_LORES_EDGE) -
// the frame's last W mod 2 pixels read their neighbour block's texel in place of the clear.
int2 KhWseePsbAt(int2 khpa_px) {
    const int khpa_ls = KH_WSEE_PSB_LS;   // s = 2 (C++ KH_PSB_LS).
    uint khpa_w, khpa_h;
    khWsS.GetDimensions(khpa_w, khpa_h);   // KH_PSB_LORES_EDGE.
    return min(khpa_px >> khpa_ls, max((int2((int)khpa_w, (int)khpa_h) >> khpa_ls) - 1, int2(0, 0)));
}
// The stacking cap's floor (KhWseeCap) - a channel of the colour behind the layers at or below it caps nothing.
#define KH_WSEE_CAP_EPS (1.0f / 1024.0f)
// An NW raw as the layer's POSITION (twin KhWseeMsNwPos): the NW passes are drawn pushed back (C++ KhWseeSurfVp:
// n' = (1 - TOL) n + TOL Hi, Hi the viewport's MaxDepth = depthParams.w where shifted), so n = (n' - TOL Hi) /
// (1 - TOL). KhWseeRule keeps the pushed value wherever it DECIDES (the range test, n' against r, the evidence rule)
// and takes this wherever NW is a depth (the front, khWsLn, the groups). The clear (1) stays 1; only an n' <= Hi is
// taken back (one past it was drawn in another range and reads at most 2.04 % near). TWIN: C++ KH_WSEE_SURF_TOL.
#define KH_WSEE_SURF_TOL (0.02f)
float KhWseeNwPos(float khwn_n)
{
    const bool khwn_b = khwn_n < 1.0f && khwn_n <= depthParams.w;   // A pushed value (<= Hi).
    return khwn_b ? (khwn_n - KH_WSEE_SURF_TOL * depthParams.w) / (1.0f - KH_WSEE_SURF_TOL) : khwn_n;
}
// The most samples a pixel takes (the per-sample loops' bound; the rest of a larger count are left out). The loops'
// attribute: without MSAA the count is 1 (a [loop] of one iteration draws fxc's X3557).
#define KH_WSEE_NS 8u
// KH_WSEE_FAINT - a pair covered by layers passing this much of the light or more, the depth buffer already at the
// behind (o.y == r: no depth-writing layer among them), takes Q at r alone: the blend T Q(behind) + (1 - T) Q(front) is
// then within (1 - T) - under 2 % - of the front / behind difference of Q(r), and every evaluation of Q at another
// depth (the front, the groups) is spared: thin rain, drizzle, a faint haze sprite cost the consumers nothing; smoke,
// foliage and glass keep the blend. One rule for every reader: KhWseeMix (fog scatter, the masks, Pulse, the sun flare,
// user effects through it), effect 13's own loop, and dynamicLightFog's split marker (effect3.hlsl KH_DLF_WS_FAINT, the
// same constant). The record itself is untouched: the sorted mesh cut and the colour helpers read it whole.
#define KH_WSEE_FAINT_T 0.98f
#if MSAA_DEPTH
#define KH_WSEE_LOOP [loop]
#else
#define KH_WSEE_LOOP [unroll]
#endif
// KH_FX_LEAN: an entry compiled with KH_FX_LEAN 1 runs no record maths - C++ draws such a build only where none can
// run, so its value is the full build's: fog scatter's PSFsSplitS and dynamicLightFog's PSDlfComp on an unlocalized,
// unbanded pass (their record code is KhFxFinish's mask blocks alone, which such a pass skips), PSEffect on an unarmed
// builtin pass (KH_FX_LEAN_PASS) and dynamicLightFog's gather on an unarmed pass or a grid texel PSDlfClass leaves 0
// (KH_DLF_SPLIT); the record's registers are declared but unreferenced in it (fxc keeps no binding for them).
#if KH_FX_LEAN
bool KhWseeArmed() { return false; }
#else
bool KhWseeArmed() { return khFxTexLay[1].z > 0.5f && centerSize.w > 0.875f && centerSize.w < 1.125f; }
#endif
// The last covered verdict's own record for a caller that needs more than the product (effect 13): the T texel (Tw,
// G, Tpre, Tpost), (NWpre, NWpost) and the sample's raw. Set by KhWseeRule where it covers; read right after it.
static float4 khWsLx = float4(1.0f, 0.0f, 1.0f, 1.0f);
static float2 khWsLn = float2(1.0f, 1.0f);
static float khWsLr = 1.0f;
// The last covered verdict's layer groups (KhWseeRule; read right after it): khWsGn how many stand (T < 1), khWsGz
// their raw depths nearest first (each held to the behind), khWsGt their transmittances; an absent group sits at the
// behind with T = 1 (weight 0). Composed front to back by KhWseeMix (the KH_WSEE note above: LAYER GROUPS).
static uint khWsGn = 0u;
static float3 khWsGz = float3(1.0f, 1.0f, 1.0f);
static float3 khWsGt = float3(1.0f, 1.0f, 1.0f);
// The split's groups (bit 4; KhWseeGroupsSet8): up to seven, the four farthest in khWsGz2 / khWsGt2; khWsLs = (Tm pre,
// Tx pre, Tm post, Tx post) at khWsLzx, khWsLs2 = (Tx2 pre, Tx2 post) at khWsLzx2. khWsSp false: the loops run over
// the three NW groups alone.
static bool khWsSp = false;
static float4 khWsGz2 = float4(1.0f, 1.0f, 1.0f, 1.0f);
static float4 khWsGt2 = float4(1.0f, 1.0f, 1.0f, 1.0f);
static float4 khWsLs = float4(1.0f, 1.0f, 1.0f, 1.0f);
static float2 khWsLzx = float2(1.0f, 1.0f);   // (pre, post) - each phase's own.
static float2 khWsLs2 = float2(1.0f, 1.0f);   // (pre, post).
static float2 khWsLzx2 = float2(1.0f, 1.0f);
// The groups of one covered verdict (khwg_x the T texel, khwg_nn = (NWpre, NWpost), khwg_r the sample's raw, khwg_b
// the behind) into khWsGn / khWsGz / khWsGt, sorted by depth.
void KhWseeGroupsSet(float4 khwg_x, float2 khwg_nn, float khwg_r, float khwg_b)
{
    float3 khwg_t = float3(khwg_x.x, khwg_x.z, khwg_x.w);
    float3 khwg_z = min(float3(khwg_r, khwg_nn.x, khwg_nn.y), khwg_b.xxx);
    uint khwg_n = 0u;
    [unroll] for (int khwg_i = 0; khwg_i < 3; ++khwg_i) {
        if (khwg_t[khwg_i] < 1.0f) {
            ++khwg_n;
        } else {
            khwg_t[khwg_i] = 1.0f;
            khwg_z[khwg_i] = khwg_b;
        }
    }
    float2 khwg_s;
    if (khwg_z.x > khwg_z.y) { khwg_s = float2(khwg_z.x, khwg_t.x); khwg_z.x = khwg_z.y; khwg_t.x = khwg_t.y;
                               khwg_z.y = khwg_s.x; khwg_t.y = khwg_s.y; }
    if (khwg_z.y > khwg_z.z) { khwg_s = float2(khwg_z.y, khwg_t.y); khwg_z.y = khwg_z.z; khwg_t.y = khwg_t.z;
                               khwg_z.z = khwg_s.x; khwg_t.z = khwg_s.y; }
    if (khwg_z.x > khwg_z.y) { khwg_s = float2(khwg_z.x, khwg_t.x); khwg_z.x = khwg_z.y; khwg_t.x = khwg_t.y;
                               khwg_z.y = khwg_s.x; khwg_t.y = khwg_s.y; }
    khWsGn = khwg_n;
    khWsGz = khwg_z;
    khWsGt = khwg_t;
}
// Group khwq_i's (0 - 2) raw depth and transmittance (a select chain: no indexed static).
float KhWseeGroupZ(uint khwq_i) { return khwq_i == 0u ? khWsGz.x : (khwq_i == 1u ? khWsGz.y : khWsGz.z); }
float KhWseeGroupT(uint khwq_i) { return khwq_i == 0u ? khWsGt.x : (khwq_i == 1u ? khWsGt.y : khWsGt.z); }
// Group khwq_i of 0 - 6 (3 - 6: the split's four farther groups, khWsGz2 / khWsGt2) - the consumers' bound 7.
float KhWseeGroupZ7(uint khwq_i)
{
    return khwq_i < 3u ? KhWseeGroupZ(khwq_i)
         : (khwq_i == 3u ? khWsGz2.x : (khwq_i == 4u ? khWsGz2.y : (khwq_i == 5u ? khWsGz2.z : khWsGz2.w)));
}
float KhWseeGroupT7(uint khwq_i)
{
    return khwq_i < 3u ? KhWseeGroupT(khwq_i)
         : (khwq_i == 3u ? khWsGt2.x : (khwq_i == 4u ? khWsGt2.y : (khwq_i == 5u ? khWsGt2.z : khWsGt2.w)));
}
// The near band's middle for a surface at raw khwb_b: a fragment fails the near push's LESS_EQUAL against khwb_b
// where its raw z > (khwb_b - TOL Hi) / (1 - TOL) (Hi = the world range's MaxDepth, depthParams.w - C++ pushes only
// viewports of that Max), so the band is (that, khwb_b]; its raw middle is the harmonic middle in distance - khwb_b's
// distance / 1.125 near the camera (a push of 25 %). KH_WSEE_NEAR_TOL: C++'s twin (0.2: s / (1 - s) = 25 %).
#define KH_WSEE_NEAR_TOL (0.2f)
float KhWseeNearRaw(float khwb_b)
{
    const float khwb_lo = max((khwb_b - KH_WSEE_NEAR_TOL * depthParams.w) / (1.0f - KH_WSEE_NEAR_TOL), depthParams.z);
    return 0.5f * (khwb_lo + khwb_b);
}
// The 100 % band's middle for a surface at raw khwb_b: the same at KH_WSEE_NEAR_TOL2 (C++'s twin, 0.5: s / (1 - s) =
// 100 %) - the band runs from half the surface's distance to the surface, its raw middle the harmonic middle in
// distance, two thirds of the surface's distance near the camera (the whole band's middle, one constant).
#define KH_WSEE_NEAR_TOL2 (0.5f)
float KhWseeNearRaw2(float khwb_b)
{
    const float khwb_lo = max((khwb_b - KH_WSEE_NEAR_TOL2 * depthParams.w) / (1.0f - KH_WSEE_NEAR_TOL2),
                              depthParams.z);
    return 0.5f * (khwb_lo + khwb_b);
}
// The far distance D's raw (C++ kh_wsee_far_raw's twin): KH_WSEE_FAR_FRAC of the far plane of the pair the chain
// decodes with (depthParams.x = m22: ndc = m22 + (1 - m22) / FRAC), in the world range.
#define KH_WSEE_FAR_FRAC (0.25f)
float KhWseeFarRaw()
{
    return depthParams.z + (depthParams.w - depthParams.z) *
           (depthParams.x + (1.0f - depthParams.x) / KH_WSEE_FAR_FRAC);
}
// KhWseeGroupsSet8: each phase's opacity split into Tm, Tx and Tx2 as the KH_WSEE note's LAYER GROUPS gives.
// KH_WSEE_FARNW: the far part sits at max(D, the phase's NW) - Tf's fragments lie beyond D and at or beyond the
// phase's NW, so the farther is the nearest bound the record gives; NW at or before D gives D. The far split only
// where Tf >= T - 1.5 / 255: Tf has no occlusion test, so a Tf below T holds fragments T lacks (hidden behind far
// terrain or absorbed by the 2 % push), which would move the visible layers' opacity to D; that phase keeps one
// group. T = Tf = 0 exactly (an opaque far record) reads Tm = 1: the phase stands at the far part. With Tp = T and
// Tf = 1 the four extra parts are absent and the three NW groups are the unsplit record's bit for bit.
void KhWseeGroupsSet8(float4 khwg_x, float2 khwg_nn, float khwg_r, float khwg_b, float4 khwg_tp, float2 khwg_tf)
{
    float2 khwg_tm = khwg_x.zw;
    float2 khwg_tx = float2(1.0f, 1.0f);
    float2 khwg_tx2 = float2(1.0f, 1.0f);   // The 100 % band's part (absent on the far branch).
    float2 khwg_zx;
    float2 khwg_zx2 = float2(khwg_b, khwg_b);
    const float khwg_d = KhWseeFarRaw();
    // The far split on the live depth: r beyond D (the sky, far terrain - no depth writer nearer than D at the
    // sample, so nothing of the far part lies hidden behind a nearer surface; a fence or a window against the sky
    // keeps the near split). Not covered: Tf is per pixel while T is per sample (far silhouettes may disagree); a
    // near-surface layer on a surface beyond D is not split; under a nearer layer of the phase (NW before D) the far
    // part stays at D - the record holds no depth of the far part's own (Tf's pass writes none).
    const bool khwg_far = khwg_r > khwg_d && khwg_d > depthParams.z && khwg_d < depthParams.w;
    if (khwg_far) {
        khwg_zx = max(float2(khwg_d, khwg_d), khwg_nn);   // KH_WSEE_FARNW.
        [unroll] for (int khwg_f = 0; khwg_f < 2; ++khwg_f) {
            const float khwg_tt = khwg_x[2 + khwg_f];
            const float khwg_fx = min(max(khwg_tf[khwg_f], khwg_tt), 1.0f);
            if (khwg_tf[khwg_f] >= khwg_tt - 1.5f / 255.0f && khwg_fx < 1.0f - 1.5f / 255.0f) {
                khwg_tx[khwg_f] = khwg_fx;
                khwg_tm[khwg_f] = khwg_fx > 0.0f ? khwg_tt / khwg_fx : 1.0f;
            }
        }
    } else {
        // Each phase's band is measured from the depth its layers were tested against: the pre phase's from the
        // behind (S, or S2 under a pre late opaque), the post phase's (and our tail's) from the live depth r.
        khwg_zx = float2(KhWseeNearRaw(khwg_b), KhWseeNearRaw(khwg_r));
        khwg_zx2 = float2(KhWseeNearRaw2(khwg_b), KhWseeNearRaw2(khwg_r));   // The same references.
        [unroll] for (int khwg_q = 0; khwg_q < 2; ++khwg_q) {
            const float khwg_tt = khwg_x[2 + khwg_q];
            const float khwg_pm = min(max(khwg_tp[khwg_q], khwg_tt), 1.0f);
            const float khwg_px = khwg_pm > 0.0f ? khwg_tt / khwg_pm : 1.0f;
            // A near part within one UNORM8 step of none (Tp within 1.5 / 255 of T, or Tx within 1.5 / 255 of 1) is
            // no group: a colour and an alpha blend unit that round a step apart make no phantom group.
            if (khwg_pm - khwg_tt > 1.5f / 255.0f && khwg_px < 1.0f - 1.5f / 255.0f) {
                khwg_tm[khwg_q] = khwg_pm;
                khwg_tx[khwg_q] = khwg_px;
            }
            // The 100 % band (Tp's B / A, a subset of R / G) splits the part at NW once more: Tm2 = Tp2 held to
            // [Tm, 1] stays at the NW position, Tx2 = Tm / Tm2 goes to the band's middle - under the same guard.
            const float khwg_pm2 = min(max(khwg_tp[2 + khwg_q], khwg_tm[khwg_q]), 1.0f);
            const float khwg_px2 = khwg_pm2 > 0.0f ? khwg_tm[khwg_q] / khwg_pm2 : 1.0f;
            if (khwg_pm2 - khwg_tm[khwg_q] > 1.5f / 255.0f && khwg_px2 < 1.0f - 1.5f / 255.0f) {
                khwg_tx2[khwg_q] = khwg_px2;
                khwg_tm[khwg_q] = khwg_pm2;
            }
        }
    }
    khWsSp = true;
    khWsLs = float4(khwg_tm.x, khwg_tx.x, khwg_tm.y, khwg_tx.y);
    khWsLzx = khwg_zx;
    khWsLs2 = khwg_tx2;
    khWsLzx2 = khwg_zx2;
    // The three NW groups sorted by KhWseeGroupsSet, then the four band / far parts inserted each after every group at
    // or before it - a stable insertion in selects alone. Do not use a compare-exchange network here: it cost fxc tens
    // of seconds of PSEffect's build over the KhWseeRule sites.
    KhWseeGroupsSet(float4(khwg_x.x, khwg_x.y, khwg_tm.x, khwg_tm.y), khwg_nn, khwg_r, khwg_b);
    float3 khwg_z = khWsGz, khwg_t = khWsGt;
    const bool  khwg_p3 = khwg_tx.x < 1.0f, khwg_p4 = khwg_tx.y < 1.0f;
    const float khwg_z3 = khwg_p3 ? min(khwg_zx.x, khwg_b) : khwg_b, khwg_t3 = khwg_p3 ? khwg_tx.x : 1.0f;
    const float khwg_z4 = khwg_p4 ? min(khwg_zx.y, khwg_b) : khwg_b, khwg_t4 = khwg_p4 ? khwg_tx.y : 1.0f;
    // The pre phase's near / far part into the three: four (q).
    const bool  khwg_a0 = khwg_z.x <= khwg_z3, khwg_a1 = khwg_z.y <= khwg_z3, khwg_a2 = khwg_z.z <= khwg_z3;
    const float4 khwg_qz = float4(khwg_a0 ? khwg_z.x : khwg_z3, khwg_a1 ? khwg_z.y : (khwg_a0 ? khwg_z3 : khwg_z.x),
                                  khwg_a2 ? khwg_z.z : (khwg_a1 ? khwg_z3 : khwg_z.y), khwg_a2 ? khwg_z3 : khwg_z.z);
    const float4 khwg_qt = float4(khwg_a0 ? khwg_t.x : khwg_t3, khwg_a1 ? khwg_t.y : (khwg_a0 ? khwg_t3 : khwg_t.x),
                                  khwg_a2 ? khwg_t.z : (khwg_a1 ? khwg_t3 : khwg_t.y), khwg_a2 ? khwg_t3 : khwg_t.z);
    // The post phase's into q: five (r).
    const bool khwg_b0 = khwg_qz.x <= khwg_z4, khwg_b1 = khwg_qz.y <= khwg_z4, khwg_b2 = khwg_qz.z <= khwg_z4,
               khwg_b3 = khwg_qz.w <= khwg_z4;
    const float khwg_rz0 = khwg_b0 ? khwg_qz.x : khwg_z4,
                khwg_rz1 = khwg_b1 ? khwg_qz.y : (khwg_b0 ? khwg_z4 : khwg_qz.x),
                khwg_rz2 = khwg_b2 ? khwg_qz.z : (khwg_b1 ? khwg_z4 : khwg_qz.y),
                khwg_rz3 = khwg_b3 ? khwg_qz.w : (khwg_b2 ? khwg_z4 : khwg_qz.z),
                khwg_rz4 = khwg_b3 ? khwg_z4 : khwg_qz.w;
    const float khwg_rt0 = khwg_b0 ? khwg_qt.x : khwg_t4,
                khwg_rt1 = khwg_b1 ? khwg_qt.y : (khwg_b0 ? khwg_t4 : khwg_qt.x),
                khwg_rt2 = khwg_b2 ? khwg_qt.z : (khwg_b1 ? khwg_t4 : khwg_qt.y),
                khwg_rt3 = khwg_b3 ? khwg_qt.w : (khwg_b2 ? khwg_t4 : khwg_qt.z),
                khwg_rt4 = khwg_b3 ? khwg_t4 : khwg_qt.w;
    // The 100 % band parts into the five: six (s), then seven - the same stable insertion; the three nearest into
    // khWsGz / khWsGt, the four farthest into khWsGz2 / khWsGt2.
    const bool  khwg_p5 = khwg_tx2.x < 1.0f, khwg_p6 = khwg_tx2.y < 1.0f;
    const float khwg_z5 = khwg_p5 ? min(khwg_zx2.x, khwg_b) : khwg_b, khwg_t5 = khwg_p5 ? khwg_tx2.x : 1.0f;
    const float khwg_z6 = khwg_p6 ? min(khwg_zx2.y, khwg_b) : khwg_b, khwg_t6 = khwg_p6 ? khwg_tx2.y : 1.0f;
    const bool khwg_c0 = khwg_rz0 <= khwg_z5, khwg_c1 = khwg_rz1 <= khwg_z5, khwg_c2 = khwg_rz2 <= khwg_z5,
               khwg_c3 = khwg_rz3 <= khwg_z5, khwg_c4 = khwg_rz4 <= khwg_z5;
    const float khwg_sz0 = khwg_c0 ? khwg_rz0 : khwg_z5, khwg_sz1 = khwg_c1 ? khwg_rz1 : (khwg_c0 ? khwg_z5 : khwg_rz0),
                khwg_sz2 = khwg_c2 ? khwg_rz2 : (khwg_c1 ? khwg_z5 : khwg_rz1),
                khwg_sz3 = khwg_c3 ? khwg_rz3 : (khwg_c2 ? khwg_z5 : khwg_rz2),
                khwg_sz4 = khwg_c4 ? khwg_rz4 : (khwg_c3 ? khwg_z5 : khwg_rz3), khwg_sz5 = khwg_c4 ? khwg_z5 : khwg_rz4;
    const float khwg_st0 = khwg_c0 ? khwg_rt0 : khwg_t5, khwg_st1 = khwg_c1 ? khwg_rt1 : (khwg_c0 ? khwg_t5 : khwg_rt0),
                khwg_st2 = khwg_c2 ? khwg_rt2 : (khwg_c1 ? khwg_t5 : khwg_rt1),
                khwg_st3 = khwg_c3 ? khwg_rt3 : (khwg_c2 ? khwg_t5 : khwg_rt2),
                khwg_st4 = khwg_c4 ? khwg_rt4 : (khwg_c3 ? khwg_t5 : khwg_rt3), khwg_st5 = khwg_c4 ? khwg_t5 : khwg_rt4;
    const bool khwg_d0 = khwg_sz0 <= khwg_z6, khwg_d1 = khwg_sz1 <= khwg_z6, khwg_d2 = khwg_sz2 <= khwg_z6,
               khwg_d3 = khwg_sz3 <= khwg_z6, khwg_d4 = khwg_sz4 <= khwg_z6, khwg_d5 = khwg_sz5 <= khwg_z6;
    khWsGz = float3(khwg_d0 ? khwg_sz0 : khwg_z6, khwg_d1 ? khwg_sz1 : (khwg_d0 ? khwg_z6 : khwg_sz0),
                    khwg_d2 ? khwg_sz2 : (khwg_d1 ? khwg_z6 : khwg_sz1));
    khWsGt = float3(khwg_d0 ? khwg_st0 : khwg_t6, khwg_d1 ? khwg_st1 : (khwg_d0 ? khwg_t6 : khwg_st0),
                    khwg_d2 ? khwg_st2 : (khwg_d1 ? khwg_t6 : khwg_st1));
    khWsGz2 = float4(khwg_d3 ? khwg_sz3 : (khwg_d2 ? khwg_z6 : khwg_sz2),
                     khwg_d4 ? khwg_sz4 : (khwg_d3 ? khwg_z6 : khwg_sz3),
                     khwg_d5 ? khwg_sz5 : (khwg_d4 ? khwg_z6 : khwg_sz4), khwg_d5 ? khwg_z6 : khwg_sz5);
    khWsGt2 = float4(khwg_d3 ? khwg_st3 : (khwg_d2 ? khwg_t6 : khwg_st2),
                     khwg_d4 ? khwg_st4 : (khwg_d3 ? khwg_t6 : khwg_st3),
                     khwg_d5 ? khwg_st5 : (khwg_d4 ? khwg_t6 : khwg_st4), khwg_d5 ? khwg_t6 : khwg_st5);
    khWsGn += (khwg_p3 ? 1u : 0u) + (khwg_p4 ? 1u : 0u) + (khwg_p5 ? 1u : 0u) + (khwg_p6 ? 1u : 0u);
}
// The stacking cap (the KH_WSEE note: STACKING CAP), khwc_p the pixel, khwc_x the colour behind the layers. An
// over-reported record (one draw's layers overlapping: the replay counts the nearest fragment per sample alone) then
// shows no more background than the pixel has; a consistent one is unchanged.
float KhWseeCap(float khwc_t, float3 khwc_p, float3 khwc_x)
{
    [unroll] for (int khwc_i = 0; khwc_i < 3; ++khwc_i) {
        if (khwc_x[khwc_i] > KH_WSEE_CAP_EPS) khwc_t = min(khwc_t, max(khwc_p[khwc_i], 0.0f) / khwc_x[khwc_i]);
    }
    return khwc_t;
}
// The views bound beside the arm (khFxTexLay[1].w; an unbound view loads 0, which the shader cannot tell from data):
// bit 0 khWsE, 1 khWsC, 2 the per-sample path allowed (clear: every pixel takes the per-pixel rule), 3 khWsB2 and
// khWsS2 (the B2 moment taken), 4 khWsTp / khWsTf, 5 khWsD3, 6 the one pass's record (t66 - t69), 7 the old record
// holds no particle-type layer (khWsN / N2 / Tp / Tf not loaded), 8 a pre late opaque was not replayed (derived, the
// note at khWsS2), 9 khWsS2 bound as its reference. Bits 10 - 11 unused. Read only where armed. C++ twin fx_tex[1][3].
uint KhWseeAvail() { return (uint)(khFxTexLay[1].w + 0.5f); }
// KH_WSEE: the rule on one sample's raw khwr_r and T's sample khwr_k - Tr returned, khwr_o = (front raw, behind raw, G:
// 0, 1 a late opaque, 0.5 a derived pre with S2 behind) where covered (khwr_o.y > khwr_o.x), (khwr_r, khwr_r, 0) and 1
// where not. S holds sample 0's snapshot alone (raw khwr_r0), so for another sample s > r_k is no evidence of a layer
// (a geometry edge drawn before the snapshot, a tilted surface's slope): the KH_WSEE note's PER SAMPLE evidence rule,
// khwr_own = on sample 0's surface (KhWseeSame); off it s is no behind either (KH_WSEE_SURF_BEHIND, below). The
// per-pixel rule passes khwr_own true and khwr_r0 = khwr_r. Loads: N and N2 (bit 7 clear) and the one pass's Nb / Nb2
// (bit 6), then S (the front in the world's range), then T only where a layer can cover (n < r, or s > r) - 3 on most
// pixels with one record, 5 with both; D3, S2, the one pass's P0 / P1 and the split's texels and T's sample 0 (k > 0)
// behind T, their address carrying a T-derived 0 (khwr_z: Tw <= 1, but not provably so
// to fxc) so fxc cannot speculate them beside S in the sample-loop sites (KhWseeMix, effect 13: there it loads S beside
// N before the loop, loop-invariant). Each load behind the test before it (KH_VMSEE_NEST: nested ifs - fxc evaluates
// both operands of &&). Callers hold the arm.
// TWIN (KH_DLF_SPLIT): effect3.hlsl's KhDlfWsMaybe takes this function's NW loads and its two outer tests (the range
// test, n < r || s > r) as the superset of its covered set - an edit to either is made in both.
float KhWseeRule(int2 px, float khwr_r, uint khwr_k, bool khwr_own, float khwr_r0, out float3 khwr_o)
{
    float khwr_t = 1.0f;
    khwr_o = float3(khwr_r, khwr_r, 0.0f);
    khWsGn = 0u;   // No groups unless covered (below).
    khWsSp = false;   // No split likewise (KhWseeGroupsSet8 sets it).
    float2 khwr_nn = float2(1.0f, 1.0f);
    float2 khwr_nb = float2(1.0f, 1.0f);   // KH_PSB: the one pass's NW alone (the read-time rule's).
    float  khwr_no = 1.0f;   // KH_WSEE_NOPRE: the old record's NWpre alone (its read-time reset's).
    {
        // KH_PSB: the old record's NW where it holds a layer (bit 7 clear), the one pass's where it drew (bit 6) -
        // each phase's nearest is the nearer of the two.
        [branch] if ((KhWseeAvail() & 128u) == 0u) {
            khwr_nn = float2(khWsN.Load(int3(px, 0)), khWsN2.Load(int3(px, 0)));
            khwr_no = khwr_nn.x;   // KH_WSEE_NOPRE.
        }
        [branch] if ((KhWseeAvail() & 64u) != 0u) {
            const int2 khwr_bp = KhWseePsbAt(px);   // KH_PSB_LORES: the record's texel.
            khwr_nb = float2(khWsNb.Load(int3(khwr_bp, 0)), khWsNb2.Load(int3(khwr_bp, 0)));   // KH_PSB_PHASE.
            khwr_nn = min(khwr_nn, khwr_nb);
        }
    }
    const float khwr_n = min(khwr_nn.x, khwr_nn.y);
    const float khwr_f = min(khwr_r, khwr_n);
    [branch] if (khwr_f >= depthParams.z) {
        const float khwr_s = khWsS.Load(int3(px, 0));
        [branch] if (khwr_n < khwr_r || khwr_s > khwr_r) {
#if MSAA_DEPTH
            float4 khwr_x = khWsT.Load(px, (int)khwr_k);   // KH_PSB: its particle channels take P0's below.
#else
            float4 khwr_x = khWsT.Load(int3(px, 0));
#endif
            bool        khwr_g = khwr_x.y > 0.25f;   // A replayed late opaque (G = 1; T holds no 0.5).
            float khwr_b = khwr_g ? khwr_r : khwr_s;
            float khwr_gz = khwr_g ? 1.0f : 0.0f;
            // A late opaque (G): behind = D3 at sample 0 (bit 5) - at k > 0 only where sample 0 is late too (its G
            // loaded once; the address khwr_z keeps fxc from hoisting it) - else the live depth.
            [branch] if (khwr_g) {
                const int khwr_z = (int)(khwr_x.x > 2.0f);   // Tw <= 1: 0, but not to fxc.
                const bool khwr_d3 = (KhWseeAvail() & 32u) != 0u;
                bool khwr_p0 = true;
#if MSAA_DEPTH
                [branch] if (khwr_k != 0u && khwr_d3) {
                    const float khwr_g0 = khWsT.Load(px, khwr_z).y;
                    khwr_p0 = khwr_g0 > 0.25f;   // G is 0 or 1 in T.
                }
#endif
                [branch] if (khwr_d3 && khwr_p0) khwr_b = khWsD3.Load(int3(px, khwr_z));
            }
            // KH_WSEE_NOPRE (bit 8; the note at khWsS2): sample 0 is pre where S2 != S (bit 9) - without S2 the live
            // sample 0 before S - and G is 0; k > 0 where sample 0 is and khwr_own. Then behind = S2 where B2 or D3
            // stands else the live depth, G = 0.5 with B2 else 1, and the old record's pre phase (Tpre here, Tp's pre
            // bands below) reads 1 where its NWpre lies behind that behind - the replay's reset at read time.
            bool khwr_ro = false;
            [branch] if (!khwr_g && (KhWseeAvail() & 256u) != 0u) {
                const int khwr_dz = (int)(khwr_x.x > 2.0f);   // Tw <= 1: 0, but not to fxc.
                // S2 is the reference only where it lies in the world's range (the view-model slice draws untracked:
                // its raw in S2 is no late opaque's) and there any change marks sample 0 (S2 != S); else the live r0
                // (LoadDepthPS's world depth at a KH_VM_SEE pixel) with r0 < S (its substitutes may lie beyond S).
                float khwr_dr = khwr_r0;
                bool  khwr_dp = khwr_r0 < khwr_s;
                [branch] if ((KhWseeAvail() & 512u) != 0u) {
                    const float khwr_s2 = khWsS2.Load(int3(px + int2(khwr_dz, 0), 0));
                    const bool  khwr_dw = khwr_s2 >= depthParams.z;
                    khwr_dp = khwr_dw ? (khwr_s2 != khwr_s && khwr_s >= depthParams.z) : khwr_dp;
                    khwr_dr = khwr_dw ? khwr_s2 : khwr_dr;
                }
                khwr_dp = khwr_dp && khwr_own && khwr_dr >= depthParams.z;
#if MSAA_DEPTH
                [branch] if (khwr_dp && khwr_k != 0u) khwr_dp = khWsT.Load(px, khwr_dz).y <= 0.25f;
#endif
                [branch] if (khwr_dp) {
                    khwr_g = true;
                    khwr_b = ((KhWseeAvail() & 512u) != 0u && (KhWseeAvail() & 40u) != 0u) ? khwr_dr : khwr_r;
                    khwr_gz = (KhWseeAvail() & 8u) != 0u ? 0.5f : 1.0f;
                    khwr_ro = khwr_no > khwr_b;
                    khwr_x.z = khwr_ro ? 1.0f : khwr_x.z;
                }
            }
            // KH_WSEE_SURF_BEHIND: two samples whose behind is the live depth, taken as a late opaque without D3 (G =
            // 1: the factor blend, never the colour formula against B - B is not the colour behind their layers).
            // (a) A sample off sample 0's surface (!khwr_own): S holds sample 0's snapshot alone, so s is another
            // surface's depth, no behind of this one; with s as its behind (on the layers' evidence, a rain streak's
            // Tr < 1) every geometry edge under a faint layer took the other side's fog - a bright or dark seam. (b)
            // Sample 0 itself (k = 0: its snapshot is its own bit for bit; another sample of its surface would differ
            // from s by its slope) with its live depth nearer than its snapshot, no late opaque replayed and no
            // fence-type layer at the sample (Tw = 1): a depth-writing draw after the capture the record holds nothing
            // of (a late opaque whose replay was refused or failed, or a qualifying draw that wrote depth where its
            // replay left no T) stands at r and the snapshot is behind IT, not behind the layers; with behind = s and
            // the colour formula the stacking cap, against a B the pixel no longer shows, took the stack for opaque and
            // put its light at the nearest layer (a rain streak before such a sea surface took the streak's fog, not
            // the sea's). A fence-type layer at the sample (Tw < 1) keeps s in both cases: it is that layer over the
            // snapshot, as recorded. TWIN: KhWseeMeshCut's opaque raw (the glass and the fog agree at an edge and over
            // such a surface).
            [branch] if (!khwr_g && khwr_x.x >= 1.0f && (!khwr_own || (khwr_k == 0u && khwr_s > khwr_r))) {
                khwr_g = true;
                khwr_b = khwr_r;
                khwr_gz = 1.0f;
            }
            // KH_PSB (the note at khWsP0): the one pass's channels at the pixel (bit 6), the read-time reset at a
            // late-opaque sample (a phase whose nearest one-pass layer lies behind the behind: 1), then multiplied
            // into the old record's - Tpre / Tpost here, the bands and Tf at the split below.
            float4 khwr_q0 = float4(1.0f, 1.0f, 1.0f, 1.0f);
            float4 khwr_q1 = float4(1.0f, 1.0f, 1.0f, 1.0f);
            [branch] if ((KhWseeAvail() & 64u) != 0u) {
                const int khwr_qz = (int)(khwr_x.x > 2.0f);   // Tw <= 1: 0, but not to fxc.
                // KH_PSB_PHASE: (Tpre, Tpost, the 25 % band's pre / post) and (the 100 % band's, Tf's) from the two
                // phases' targets.
                const int2 khwr_qb = KhWseePsbAt(px);   // KH_PSB_LORES: the record's texel.
                const float4 khwr_qp = khWsP0.Load(int3(khwr_qb + int2(khwr_qz, 0), 0));
                const float4 khwr_qq = khWsP1.Load(int3(khwr_qb + int2(khwr_qz, 0), 0));
                khwr_q0 = float4(khwr_qp.x, khwr_qq.x, khwr_qp.y, khwr_qq.y);
                [branch] if ((KhWseeAvail() & 16u) != 0u) khwr_q1 = float4(khwr_qp.z, khwr_qq.z, khwr_qp.w, khwr_qq.w);
                const bool khwr_rp = khwr_g && khwr_nb.x > khwr_b;   // The pre phase's layers all behind.
                const bool khwr_rq = khwr_g && khwr_nb.y > khwr_b;   // The post phase's.
                khwr_q0 = float4(khwr_rp ? 1.0f : khwr_q0.x, khwr_rq ? 1.0f : khwr_q0.y,
                                 khwr_rp ? 1.0f : khwr_q0.z, khwr_rq ? 1.0f : khwr_q0.w);
                khwr_q1 = float4(khwr_rp ? 1.0f : khwr_q1.x, khwr_rq ? 1.0f : khwr_q1.y,
                                 khwr_rp ? 1.0f : khwr_q1.z, khwr_rq ? 1.0f : khwr_q1.w);
                khwr_x.zw = khwr_x.zw * khwr_q0.xy;
            }
            // The transmittance, Tw Tpre Tpost.
            const float khwr_tp = khwr_x.x * khwr_x.z * khwr_x.w;
            if (khwr_b > khwr_f) {
                if (khwr_g || khwr_n < khwr_r || khwr_tp < 1.0f || (khwr_own && khwr_s > khwr_r0)) {
                    // NW as a position (KhWseeNwPos) - the front out, khWsLn and the groups' depths; the
                    // pushed value decided above.
                    const float2 khwr_np = float2(KhWseeNwPos(khwr_nn.x), KhWseeNwPos(khwr_nn.y));
                    const float  khwr_fp = khwr_n < khwr_r ? min(khwr_np.x, khwr_np.y) : khwr_r;
                    khwr_o = float3(khwr_fp, khwr_b, khwr_gz);
                    khwr_t = khwr_tp;
                    khWsLx = khwr_x;
                    khWsLn = khwr_np;
                    khWsLr = khwr_r;
                    // The layer groups; with the split (bit 4) the sample's Tp and the pixel's Tf too - loaded here
                    // alone (covered), their address carrying a T-derived 0.
                    {
                        [branch] if ((KhWseeAvail() & 16u) != 0u) {
                            const int khwr_lz = (int)(khwr_x.x > 2.0f);   // Tw <= 1: 0, but not to fxc.
                            float4 khwr_lp = float4(1.0f, 1.0f, 1.0f, 1.0f);   // KH_PSB: the old record's where it
                            float2 khwr_lf = float2(1.0f, 1.0f);                // holds a layer (bit 7 clear).
                            [branch] if ((KhWseeAvail() & 128u) == 0u) {
#if MSAA_DEPTH
                                khwr_lp = khWsTp.Load(px + int2(khwr_lz, 0), (int)khwr_k);   // RGBA.
#else
                                khwr_lp = khWsTp.Load(int3(px + int2(khwr_lz, 0), 0));
#endif
                                // KH_WSEE_NOPRE: the replay's reset of Tp's pre bands, at read time (above).
                                khwr_lp = khwr_ro ? float4(1.0f, khwr_lp.y, 1.0f, khwr_lp.w) : khwr_lp;
                                khwr_lf = khWsTf.Load(int3(px + int2(khwr_lz, 0), 0));
                            }
                            khwr_lp = khwr_lp * float4(khwr_q0.zw, khwr_q1.xy);   // KH_PSB: times the one pass's.
                            khwr_lf = khwr_lf * khwr_q1.zw;
                            KhWseeGroupsSet8(khwr_x, khwr_np, khwr_r, khwr_b, khwr_lp, khwr_lf);
                        } else {
                            KhWseeGroupsSet(khwr_x, khwr_np, khwr_r, khwr_b);
                        }
                    }
                }
            }
        }
    }
    return khwr_t;
}
// KH_WSEE: the per-pixel rule at px given its front raw khwa_r (LoadDepthPS's) - the layers' transmittance Tr returned
// (1 where not covered), khwa_o = (front raw, behind raw, G: 1 = a late opaque surface - the behind raw its own depth
// where D3 stands (bit 5), else the live depth; 0.5 = one drawn before the cycle's first fence-type layer with B2 taken
// - KhWseeRule) where covered, which is exactly where khwa_o.y > khwa_o.x; (khwa_r, khwa_r, 0) elsewhere and unarmed.
// The loads only when armed (uniform lanes alone). KH_WSEE_FXC: Tr comes back as the value, not as a second out beside
// a bool - that shape stopped d3dcompiler_47 on PSEffect with "internal error: compilation aborted unexpectedly".
float KhWseeAt(int2 px, float khwa_r, out float3 khwa_o)
{
    float khwa_t = 1.0f;
    khwa_o = float3(khwa_r, khwa_r, 0.0f);
    [branch] if (KhWseeArmed()) {
        khwa_t = KhWseeRule(px, khwa_r, 0u, true, khwa_r, khwa_o);
    }
    return khwa_t;
}
// KhWseeOneFront - with exactly one layer group its opacity lies at ITS depth (khWsGz.x, held to the behind) where that
// lies behind the front: max(khWsGz.x, front). T is a UNORM8 blend, so a sprite with a < 1 / 510 at a pixel leaves T =
// 1 while NW (discard-based) holds its depth: under such a faded sprite the one-group mix Tr Q(behind) + (1 - Tr)
// Q(front) charged the group's whole opacity at the sprite's depth. A part the split places nearer than its layer (a
// band part for a pane in the band's farther half) keeps the layer's own front; slot 0 is the present group unless its
// depth tied the behind (every slot then sits at the behind, behind the front: the mix an identity either way). None
// or two and more: the front. Not covered: a pane in no band under such a sprite (one NW a phase). khwof_o =
// KhWseeRule's (front, behind, G).
float KhWseeOneFront(float3 khwof_o)
{
    return khWsGn == 1u ? max(khWsGz.x, khwof_o.x) : khwof_o.x;
}
#if MSAA_DEPTH
// The depth's sample count, at most KH_WSEE_NS.
uint KhWseeNs()
{
    uint khwz_w, khwz_h, khwz_n;
    depthTex.GetDimensions(khwz_w, khwz_h, khwz_n);
    return clamp(khwz_n, 1u, KH_WSEE_NS);
}
// KH_WSEE_SAME: whether sample khwq_k (raw khwq_rk) lies on sample 0's surface (raw khwq_r0) - a heuristic. At MSAA
// each sample's depth is the primitive's at that sample's position, so a tilted surface's samples are never bit for
// bit equal; across a plane the raw depth (z / w) is affine in screen x, y, so sample k is predicted from sample 0 by
// the pixel's raw-depth gradient khwq_g (KhWseeGrad) times the offset between their positions (GetSamplePosition, in
// pixels) and is the same surface within KH_WSEE_ZTOL raw plus half the predicted change (creases, curvature, the
// gradient's own rounding). Simulated at 4K / D24 and D32F / 4x and 8x: planes to 80 deg and spheres read "same",
// edges whose depth jumps 0.5 % or more read "different" in all but about 0.1 % (features under 2 px up to 4 %); a
// missed edge keeps the per-pixel path. Assumes the standard sample patterns GetSamplePosition reports; a driver
// reporting no positions falls back to the KH_WSEE_ZTOL floor.
#define KH_WSEE_ZTOL (8.0f / 16777216.0f)
// The pixel's raw-depth screen gradient (per pixel) from its four neighbours' sample-0 raws: per axis the one-sided
// difference of the smaller magnitude (edge-aware: a neighbour across an edge differs by the jump), at the frame's
// edge the one there is (0 with neither). KH_WSEE_NOPRE: its and KhWseeSame's mesh-unit twins KhWseeMsGrad / MsSame.
float2 KhWseeGrad(int2 px, float khwg_r0)
{
    uint khwg_w, khwg_h, khwg_n;
    depthTex.GetDimensions(khwg_w, khwg_h, khwg_n);
    float2 khwg_g = float2(0.0f, 0.0f);
    [unroll] for (int khwg_a = 0; khwg_a < 2; ++khwg_a) {
        const int2 khwg_e = khwg_a == 0 ? int2(1, 0) : int2(0, 1);
        const int  khwg_p = khwg_a == 0 ? px.x : px.y;
        const int  khwg_m = (int)(khwg_a == 0 ? khwg_w : khwg_h);
        const bool khwg_hf = khwg_p + 1 < khwg_m;
        const bool khwg_hb = khwg_p > 0;
        const float khwg_f = depthTex.Load(px + khwg_e, 0) - khwg_r0;   // Off the frame: unused (selected away).
        const float khwg_b = khwg_r0 - depthTex.Load(px - khwg_e, 0);
        float khwg_d = 0.0f;
        if (khwg_hf && khwg_hb) khwg_d = abs(khwg_f) <= abs(khwg_b) ? khwg_f : khwg_b;
        else if (khwg_hf) khwg_d = khwg_f;
        else if (khwg_hb) khwg_d = khwg_b;
        if (khwg_a == 0) khwg_g.x = khwg_d;
        else khwg_g.y = khwg_d;
    }
    return khwg_g;
}
bool KhWseeSame(uint khwq_k, float khwq_r0, float khwq_rk, float2 khwq_g)
{
    const float2 khwq_d = depthTex.GetSamplePosition((int)khwq_k) - depthTex.GetSamplePosition(0);
    const float  khwq_p = khwq_r0 + dot(khwq_g, khwq_d);
    return abs(khwq_rk - khwq_p) <= KH_WSEE_ZTOL + 0.5f * dot(abs(khwq_g), abs(khwq_d));
}
// How far px's samples (the first KhWseeNs) disagree: 2 - a T texel's Tw or G other than sample 0's, bit for bit, or
// a raw depth off sample 0's surface (KhWseeSame - its gradient only where some raw differs bit for bit, so a pixel
// parallel to the image costs nothing more); 1 - only Tpre or Tpost differ; 0 - they agree. The search stops once
// the level reaches khwd_stop (KhWseeDisagree: 1).
uint KhWseeDisagreeLv(int2 px, uint khwd_stop)
{
    const uint   khwd_n = KhWseeNs();
    const float  khwd_r = depthTex.Load(px, 0);
    const float4 khwd_t = khWsT.Load(px, 0);   // Every channel.
    uint khwd_d = 0u;
    bool khwd_z = false;
    [loop] for (uint khwd_k = 1u; khwd_k < khwd_n; ++khwd_k) {
        const float4 khwd_tk = khWsT.Load(px, (int)khwd_k);
        if (any(khwd_tk.xy != khwd_t.xy)) {
            khwd_d = 2u;
            break;
        }
        if (any(khwd_tk.zw != khwd_t.zw)) {
            khwd_d = 1u;
            if (khwd_stop <= 1u) break;
        }
        if (depthTex.Load(px, (int)khwd_k) != khwd_r) khwd_z = true;
    }
    [branch] if (khwd_d < khwd_stop && khwd_z) {
        const float2 khwd_g = KhWseeGrad(px, khwd_r);
        [loop] for (uint khwd_j = 1u; khwd_j < khwd_n; ++khwd_j) {
            if (!KhWseeSame(khwd_j, khwd_r, depthTex.Load(px, (int)khwd_j), khwd_g)) {
                khwd_d = 2u;
                break;
            }
        }
    }
    return khwd_d;
}
// The inline test where khWsE is not bound.
bool KhWseeDisagree(int2 px)
{
    return KhWseeDisagreeLv(px, 1u) >= 1u;
}
// KH_WSEE_EDGE_COST - THE CLASS WORD. khWsE (t58) holds, per pixel, the classes of its KhWseeNs samples (PSWseeAgree,
// once a chain run): a class = the samples sharing their T texel (all four bytes) and their side of sample 0's surface
// (own: the raw is sample 0's bit for bit or KhWseeSame); an off-surface sample joins an off-surface class only where
// its raw is near the class's first (KH_WSEE_CLS_REL below). Up to four classes in order of first appearance: bits
// 0 - 15 sample k's class (2 bits at 2k; sample 0 is class 0), bits 16 - 18 the own flag of classes 1 - 3, bits
// 20 - 21 the class count - 1, bit 30 (KH_WSEE_CW_L2) the level-2 verdict (Tw or G other than sample 0's, or a raw off
// its surface), bit 31 (KH_WSEE_CW_OV) more than four classes (the other bits mean nothing; consumers take every
// sample). The word is 0 exactly where KhWseeDisagreeLv's level is 0 (its verdicts bit for bit: the nearness test
// only splits classes), so a pass's samples disagree where it is nonzero. Loads: 2 per sample, the gradient's 4 only
// once a raw differs (its address carries a raw-derived 0 so fxc cannot hoist them onto every pixel). The pass
// discards where the word is 0, so the stencil C++ binds with it marks exactly the pixels a consumer may take per
// sample.
#define KH_WSEE_CW_L2 0x40000000u
#define KH_WSEE_CW_OV 0x80000000u
// KH_WSEE_CLS_REL: an off-surface raw r joins a class of first raw r_c where |r - r_c| <= KH_WSEE_ZTOL +
// KH_WSEE_CLS_REL |r_c - r_inf| (r_inf the raw at infinity in the pass's range): the raw is affine in 1 / d, so the
// members lie within about 3 % of the first one's distance to a km, widening to the depth's resolution floor beyond;
// the sky's cleared raw and a ridge stay apart except within a few % of a finite far plane. Below depthParams.z (the
// view-model slice, another projection) it is no distance window. ALU only; only off-surface samples run it.
#define KH_WSEE_CLS_REL (1.0f / 32.0f)
uint KhWseeClassWord(int2 px)
{
    const uint   khcw_n = KhWseeNs();
    const float  khcw_r0 = depthTex.Load(px, 0);
    const float4 khcw_t0 = khWsT.Load(px, 0);
    const uint4  khcw_b0 = (uint4)(khcw_t0 * 255.0f + 0.5f);   // UNORM8: the stored byte exactly.
    uint4  khcw_key = uint4(khcw_b0.x | (khcw_b0.y << 8) | (khcw_b0.z << 16) | (khcw_b0.w << 24), 0u, 0u, 0u);
    float4 khcw_rc = float4(khcw_r0, 0.0f, 0.0f, 0.0f);   // Each class's first raw.
    const float khcw_ri = depthParams.z + (depthParams.w - depthParams.z) * depthParams.x;
    uint   khcw_own = 1u;   // Bit c: class c lies on sample 0's surface.
    uint   khcw_nc = 1u;
    uint   khcw_map = 0u;
    uint   khcw_eq = 0u;    // Bit k: sample k's raw is sample 0's bit for bit.
    bool   khcw_ov = false;
    bool   khcw_l2 = false;
    bool   khcw_z = false;
    float2 khcw_g = float2(0.0f, 0.0f);
    // KH_WSEE_EFAST: every T texel and raw loaded together (unrolled, the index held to the count), word 0 returned
    // where every T texel is sample 0's and every raw is sample 0's or on its surface (one class, own, no level 2);
    // elsewhere the loop below runs. No early return (fxc's X4000 at the loop): khcw_fz marks word 0 and the
    // loop runs only without it. At 2x / 4x the samples past the count reload the last one (a cached texel).
    bool khcw_fz = false;
    {
        bool  khcw_ft = true, khcw_fd = false;
        float khcw_fr[KH_WSEE_NS];
        khcw_fr[0] = khcw_r0;
        [unroll] for (uint khcw_fk = 1u; khcw_fk < KH_WSEE_NS; ++khcw_fk) {
            const bool   khcw_fi = khcw_fk < khcw_n;
            const float4 khcw_ftk = khWsT.Load(px, (int)min(khcw_fk, khcw_n - 1u));
            const float  khcw_frk = depthTex.Load(px, (int)min(khcw_fk, khcw_n - 1u));
            khcw_fr[khcw_fk] = khcw_frk;
            khcw_ft = khcw_ft && (!khcw_fi || all(khcw_ftk == khcw_t0));
            khcw_fd = khcw_fd || (khcw_fi && khcw_frk != khcw_r0);
        }
        [branch] if (khcw_ft) {
            khcw_fz = !khcw_fd;
            [branch] if (khcw_fd) {
                // The gradient's address carries a raw-derived 0 (as in the loop below; a raw is at most 1).
                const float2 khcw_fg = KhWseeGrad(px + int2((int)(khcw_r0 > 2.0f), 0), khcw_r0);
                bool khcw_fo = true;
                [unroll] for (uint khcw_fj = 1u; khcw_fj < KH_WSEE_NS; ++khcw_fj) {
                    khcw_fo = khcw_fo && (khcw_fj >= khcw_n ||
                                          KhWseeSame(khcw_fj, khcw_r0, khcw_fr[khcw_fj], khcw_fg));
                }
                khcw_fz = khcw_fo;
            }
        }
    }
    uint khcw_w = 0u;   // One return - the fast path's word 0, else the loop's word below.
    [branch] if (!khcw_fz) {
        [loop] for (uint khcw_k = 1u; khcw_k < khcw_n; ++khcw_k) {
            const float4 khcw_tk = khWsT.Load(px, (int)khcw_k);
            const float  khcw_rk = depthTex.Load(px, (int)khcw_k);
            if (any(khcw_tk.xy != khcw_t0.xy)) khcw_l2 = true;
            bool khcw_on = true;
            [branch] if (khcw_rk != khcw_r0) {
                // The address's offset is 0 (a raw is at most 1) but not to fxc - the loads stay in this branch.
                [branch] if (!khcw_z) khcw_g = KhWseeGrad(px + int2((int)(khcw_rk > 2.0f), 0), khcw_r0);
                khcw_z = true;
                khcw_on = KhWseeSame(khcw_k, khcw_r0, khcw_rk, khcw_g);
                if (!khcw_on) khcw_l2 = true;
            } else {
                khcw_eq |= 1u << khcw_k;
            }
            const uint4 khcw_bk = (uint4)(khcw_tk * 255.0f + 0.5f);
            const uint  khcw_kk = khcw_bk.x | (khcw_bk.y << 8) | (khcw_bk.z << 16) | (khcw_bk.w << 24);
            const uint  khcw_o = khcw_on ? 1u : 0u;
            uint khcw_c = 4u;
            // An on-surface sample matches an on-surface class by its key alone; only an off-surface one pays the
            // nearness test (fxc evaluates both operands of ||, so an agreeing pixel's loop carries none of it).
            [branch] if (khcw_on) {
                [unroll] for (uint khcw_j = 0u; khcw_j < 4u; ++khcw_j) {
                    const bool khcw_m = khcw_key[khcw_j] == khcw_kk && ((khcw_own >> khcw_j) & 1u) == 1u;
                    if (khcw_c == 4u && khcw_j < khcw_nc && khcw_m) khcw_c = khcw_j;
                }
            } else {
                [unroll] for (uint khcw_j1 = 0u; khcw_j1 < 4u; ++khcw_j1) {
                    const bool khcw_nr = abs(khcw_rk - khcw_rc[khcw_j1]) <=   // Near the class's first raw.
                                         KH_WSEE_ZTOL + KH_WSEE_CLS_REL * abs(khcw_rc[khcw_j1] - khcw_ri);
                    const bool khcw_m = khcw_key[khcw_j1] == khcw_kk && ((khcw_own >> khcw_j1) & 1u) == 0u && khcw_nr;
                    if (khcw_c == 4u && khcw_j1 < khcw_nc && khcw_m) khcw_c = khcw_j1;
                }
            }
            if (khcw_c == 4u) {
                khcw_ov = khcw_ov || khcw_nc >= 4u;
                khcw_c = min(khcw_nc, 3u);
                [unroll] for (uint khcw_i = 1u; khcw_i < 4u; ++khcw_i) {
                    if (khcw_i == khcw_nc) {
                        khcw_key[khcw_i] = khcw_kk;
                        khcw_rc[khcw_i] = khcw_rk;
                    }
                }
                khcw_own |= khcw_nc < 4u ? (khcw_o << khcw_nc) : 0u;
                khcw_nc = min(khcw_nc + 1u, 4u);
            }
            khcw_map |= khcw_c << (2u * khcw_k);
        }
        // KhWseeDisagreeLv's second loop for the samples whose raw is sample 0's bit for bit (its KhWseeSame on every
        // sample once some raw differs): the same call on the same values, no load.
        [branch] if (khcw_z && !khcw_l2) {
            [loop] for (uint khcw_j2 = 1u; khcw_j2 < khcw_n; ++khcw_j2) {
                [branch] if (((khcw_eq >> khcw_j2) & 1u) != 0u) {
                    if (!KhWseeSame(khcw_j2, khcw_r0, khcw_r0, khcw_g)) khcw_l2 = true;
                }
            }
        }
        const uint khcw_lv = khcw_l2 ? KH_WSEE_CW_L2 : 0u;
        khcw_w = khcw_ov ? (KH_WSEE_CW_OV | khcw_lv)
                         : (khcw_lv | ((khcw_nc - 1u) << 20) | (((khcw_own >> 1) & 7u) << 16) | (khcw_map & 0xFFFFu));
    }
    return khcw_w;
}
#endif
// KH_WSEE per sample: how many depth pairs px takes given its front raw khwn_r (LoadDepthPS's) - 1 (unarmed, no MSAA,
// the samples agree, or the marker / KH_VM_SEE changed sample 0 - both sample-0 constructs) or KhWseeNs where they
// disagree; khwn_g = the pixel's raw-depth gradient for KhWseeSampleAt (0 on the per-pixel path), taken here outside
// the caller's sample loop (inside it fxc hoists its four loads onto every pixel). The fast path reads khWsE alone
// where bound.
// KH_WSEE_EDGE_COST - KhWseeCountW (the builtins): where khWsE's word stands (bit 0) and is nonzero, the pixel's
// CLASSES (1 - 4; khwn_w = the word, no gradient) in place of its samples; past four classes or without the word every
// sample (khwn_w = KH_WSEE_CW_OV, the gradient taken); khwn_w = 0 on the per-pixel path. KhWseeSampleAtW takes class
// k's first member (raw, T texel, own flag), KhWseeClassM its member count (weight members / KhWseeNs, KhWseeDivW): the
// rule and Q once per class. The value moves only within a class (its members take the first member's raw and Tp).
// KhWseeCount (the user contract) keeps every sample where they disagree.
uint KhWseeCountW(int2 px, float khwn_r, out float2 khwn_g, out uint khwn_w)
{
    uint khwn_c = 1u;
    khwn_g = float2(0.0f, 0.0f);
    khwn_w = 0u;
#if MSAA_DEPTH
    [branch] if (KhWseeArmed()) {
        [branch] if ((KhWseeAvail() & 4u) != 0u) {   // Bit 2: per sample allowed (KH_WSEE_EPIX clears it).
            uint khwn_e;
            // khWsE is the class word: nonzero where the samples disagree (KhWseeClassWord); the inline test reads as
            // past four classes.
            [branch] if ((KhWseeAvail() & 1u) != 0u) {
                khwn_e = khWsE.Load(int3(px, 0));
            } else {
                khwn_e = KhWseeDisagree(px) ? KH_WSEE_CW_OV : 0u;
            }
            [branch] if (khwn_e != 0u) {
                [branch] if (khwn_r == depthTex.Load(px, 0)) {
                    [branch] if ((khwn_e & KH_WSEE_CW_OV) != 0u) {
                        khwn_c = KhWseeNs();
                        khwn_g = KhWseeGrad(px, khwn_r);
                        khwn_w = KH_WSEE_CW_OV;
                    } else {
                        khwn_c = ((khwn_e >> 20) & 3u) + 1u;
                        khwn_w = khwn_e;
                    }
                }
            }
        }
    }
#endif
    return khwn_c;
}
uint KhWseeCount(int2 px, float khwn_r, out float2 khwn_g)
{
    uint khwn_w;
    uint khwn_c = KhWseeCountW(px, khwn_r, khwn_g, khwn_w);
#if MSAA_DEPTH
    [branch] if (khwn_w != 0u && (khwn_w & KH_WSEE_CW_OV) == 0u) {   // KH_WSEE_EDGE_COST: every sample here.
        khwn_c = KhWseeNs();
        khwn_g = KhWseeGrad(px, khwn_r);
    }
#endif
    return khwn_c;
}
// KH_WSEE_EDGE_COST: class khcs_c of word khcs_w - bit 2k set where sample k (k < KhWseeNs) is a member; its
// member count (KhWseeClassM, as a weight: 1 outside the class form - every sample, or the per-pixel rule) and its
// first member (KhWseeClassRep); KhWseeDivW the weights' sum (KhWseeNs in the class form, else the pair count).
uint KhWseeClassSet(uint khcs_w, uint khcs_c)
{
#if MSAA_DEPTH
    const uint khcs_x = (khcs_w & 0xFFFFu) ^ (khcs_c * 0x5555u);   // 0 in the pairs that hold class c.
    return ~(khcs_x | (khcs_x >> 1)) & 0x5555u & ((1u << (2u * KhWseeNs())) - 1u);
#else
    return 1u;
#endif
}
float KhWseeClassM(uint khcs_w, uint khcs_c)
{
    float khcs_m = 1.0f;
#if MSAA_DEPTH
    [branch] if (khcs_w != 0u && (khcs_w & KH_WSEE_CW_OV) == 0u) {
        khcs_m = (float)countbits(KhWseeClassSet(khcs_w, khcs_c));
    }
#endif
    return khcs_m;
}
uint KhWseeClassRep(uint khcs_w, uint khcs_c) { return firstbitlow(KhWseeClassSet(khcs_w, khcs_c)) >> 1; }
float KhWseeDivW(uint khcs_w, uint khcs_n)
{
    float khcs_d = (float)khcs_n;
#if MSAA_DEPTH
    [branch] if (khcs_w != 0u && (khcs_w & KH_WSEE_CW_OV) == 0u) khcs_d = (float)KhWseeNs();
#endif
    return khcs_d;
}
#if MSAA_DEPTH
// KH_WSEE_EDGE_COST: the scene's sample colours (khWsC) for pair khcc_c - in the class form the mean of the
// class's members' (exact for the affine parts of effect 13's per-sample rule; its clamp and cap then see the mean),
// else sample khcc_c's own.
float3 KhWseeClassC(int2 px, uint khcc_w, uint khcc_c)
{
    float3 khcc_s = float3(0.0f, 0.0f, 0.0f);
    [branch] if (khcc_w != 0u && (khcc_w & KH_WSEE_CW_OV) == 0u) {
        const uint khcc_set = KhWseeClassSet(khcc_w, khcc_c);
        [loop] for (uint khcc_k = 0u; khcc_k < KhWseeNs(); ++khcc_k) {
            [branch] if (((khcc_set >> (2u * khcc_k)) & 1u) != 0u) khcc_s += khWsC.Load(px, (int)khcc_k).rgb;
        }
        khcc_s /= (float)countbits(khcc_set);
    } else {
        khcc_s = khWsC.Load(px, (int)khcc_c).rgb;
    }
    return khcc_s;
}
#endif
// The pair of px's sample khws_k of khws_n with gradient khws_g (KhWseeCount's): KhWseeAt's out and value on that
// sample's own raw and T texel (n and s the pixel's; khws_r is sample 0's raw on this path, KhWseeCount having checked
// that LoadDepthPS left it as it is; the sample shares sample 0's layer verdict where KhWseeSame); khws_n = 1 is
// KhWseeAt itself. One body for both: a per-pixel case inside a caller's sample loop would be loop-invariant and fxc
// would hoist KhWseeRule's loads out of their branches (the T load then on every pixel). KhWseeSampleAtW: in the class
// form (khws_w KhWseeCountW's word) khws_k is a class - its first member's raw and T texel and the class's own flag
// from the word (no KhWseeSame); else (khws_w 0 or KH_WSEE_CW_OV) KhWseeSampleAt itself.
float KhWseeSampleAtW(int2 px, float khws_r, uint khws_k, uint khws_n, uint khws_w, float2 khws_g, out float3 khws_o)
{
    float khws_t = 1.0f;
    khws_o = float3(khws_r, khws_r, 0.0f);
#if MSAA_DEPTH
    uint  khws_s = khws_n > 1u ? khws_k : 0u;
    float khws_rk = khws_r;
    bool  khws_own = true;
    [branch] if (khws_w != 0u && (khws_w & KH_WSEE_CW_OV) == 0u) {
        khws_s = KhWseeClassRep(khws_w, khws_k);
        khws_own = khws_k == 0u || ((khws_w >> (15u + khws_k)) & 1u) != 0u;
        khws_rk = depthTex.Load(px, (int)khws_s);
    } else {
        [branch] if (khws_n > 1u) {
            khws_rk = depthTex.Load(px, (int)khws_s);
            khws_own = khws_rk == khws_r;
            [branch] if (!khws_own) khws_own = KhWseeSame(khws_s, khws_r, khws_rk, khws_g);
        }
    }
    [branch] if (KhWseeArmed()) {
        khws_t = KhWseeRule(px, khws_rk, khws_s, khws_own, khws_r, khws_o);
    }
#else
    khws_t = KhWseeAt(px, khws_r, khws_o);
#endif
    return khws_t;
}
float KhWseeSampleAt(int2 px, float khws_r, uint khws_k, uint khws_n, float2 khws_g, out float3 khws_o)
{
    return KhWseeSampleAtW(px, khws_r, khws_k, khws_n, 0u, khws_g, khws_o);
}
// KhWseeMix - the two-depth blend of a quantity, per sample where px's samples disagree. khwm_q holds Q at the front
// raw khwm_r; khwm_e is an expression of the float khwm_z (a view depth, LinDepth's) giving Q there. Each pair takes Tr
// Q(behind) + (1 - Tr) Q(front) - or the front-to-back group composition where two or more groups cover it, one group's
// at its own depth (KhWseeOneFront) - and khwm_q becomes their mean (the classes' weighted mean with khWsE:
// KhWseeClassM / KhWseeDivW); left exactly as it was unarmed and where nothing covers and the samples agree (an
// uncovered pair takes Q(r_k)). One-entry caches keep khwm_e to the depths not yet seen, so the per-pixel path
// evaluates it at most once per covered pixel at the behind and once at a no-write layer's front, plus once per
// further layer group whose depth is neither (KhWseeOneFront's group where one stands, each KhWseeGroupZ7 group where
// two or more do). KH_WSEE_FAINT (khwm_fn): a pair covered by faint layers alone, the behind the depth buffer's, takes
// Q at r and evaluates nothing (the constant's note at KH_WSEE_FAINT_T). Contract: khwm_r must be
// LoadDepthPS(khwm_px) of the same pixel; khwm_z a name unused in the scope (the macro declares it); khwm_e free of
// gradients (Load / SampleLevel, no Sample or ddx: it runs inside a [loop] and [branch]es); khwm_px, khwm_r, khwm_q
// plain variables (read several times). A statement ending in a block: brace it as the body of an unbraced if / else.
// One loop over all seven groups (khWsGz / khWsGz2) so the caller's expression is not repeated.
#define KhWseeMix(khwm_px, khwm_r, khwm_q, khwm_z, khwm_e) \
    [branch] if (KhWseeArmed()) { \
        float2 khwm_g; \
        uint khwm_w; \
        const uint khwm_n = KhWseeCountW(khwm_px, khwm_r, khwm_g, khwm_w); \
        float khwm_lf = khwm_r, khwm_qf = khwm_q, khwm_lb = khwm_r, khwm_qb = khwm_q, khwm_a = 0.0f; \
        bool khwm_c = khwm_n > 1u || khwm_w != 0u; \
        KH_WSEE_LOOP for (uint khwm_k = 0u; khwm_k < khwm_n; ++khwm_k) { \
            float3 khwm_o; \
            const float khwm_t = KhWseeSampleAtW(khwm_px, khwm_r, khwm_k, khwm_n, khwm_w, khwm_g, khwm_o); \
            const bool khwm_fn = khwm_o.y > khwm_o.x && khwm_t >= KH_WSEE_FAINT_T && khwm_o.y == khwm_r; \
            float khwm_v = khwm_q; \
            [branch] if (!khwm_fn) { \
            [branch] if (khwm_o.x != khwm_lf) { \
                khwm_lf = khwm_o.x; \
                const float khwm_z = LinDepth(khwm_o.x); \
                khwm_qf = (khwm_e); \
            } \
            khwm_v = khwm_qf; \
            [branch] if (khwm_o.y > khwm_o.x) { \
                khwm_c = true; \
                [branch] if (khwm_o.y != khwm_lb) { \
                    khwm_lb = khwm_o.y; \
                    const float khwm_z = LinDepth(khwm_o.y); \
                    khwm_qb = (khwm_e); \
                } \
                [branch] if (khWsGn >= 2u) { \
                    float khwm_gv = 0.0f, khwm_gp = 1.0f; \
                    [loop] for (uint khwm_i = 0u; khwm_i < (khWsSp ? 7u : 3u); ++khwm_i) { \
                        const float khwm_gt = KhWseeGroupT7(khwm_i); \
                        [branch] if (khwm_gt < 1.0f) { \
                            const float khwm_gr = KhWseeGroupZ7(khwm_i); \
                            float khwm_gq = khwm_qb; \
                            [branch] if (khwm_gr == khwm_lf) { \
                                khwm_gq = khwm_qf; \
                            } else { \
                                [branch] if (khwm_gr != khwm_lb) { \
                                    const float khwm_z = LinDepth(khwm_gr); \
                                    khwm_gq = (khwm_e); \
                                } \
                            } \
                            khwm_gv += khwm_gp * (1.0f - khwm_gt) * khwm_gq; \
                            khwm_gp *= khwm_gt; \
                        } \
                    } \
                    khwm_v = khwm_gv + khwm_gp * khwm_qb; \
                } else { \
                    const float khwm_of = KhWseeOneFront(khwm_o); \
                    float khwm_q1 = khwm_qb; \
                    [branch] if (khwm_of == khwm_lf) { \
                        khwm_q1 = khwm_qf; \
                    } else { \
                        [branch] if (khwm_of != khwm_lb) { \
                            const float khwm_z = LinDepth(khwm_of); \
                            khwm_q1 = (khwm_e); \
                        } \
                    } \
                    khwm_v = khwm_t * khwm_qb + (1.0f - khwm_t) * khwm_q1; \
                } \
            } \
            } \
            khwm_a += KhWseeClassM(khwm_w, khwm_k) * khwm_v; \
        } \
        if (khwm_c) khwm_q = khwm_a / KhWseeDivW(khwm_w, khwm_n); \
    }
float LoadDepthPS(int2 px)
{
    const float khnz_r = LoadDepthRaw(px);
    if (shadowMeta2.x > 0.5f) {   // TWIN (KH_DLF_SPLIT): effect3.hlsl KhDlfWsMaybe's mark test.
        const float2 khnz_m = khNzMark.Load(int3(px, 0));
        if (khnz_m.x > 0.0f && khnz_m.x == khnz_r && khnz_m.y > 0.0f) {
            return depthParams.z + (depthParams.w - depthParams.z) * (depthParams.x + depthParams.y / khnz_m.y);
        }
    }
    [branch] if (shadowMeta2.w > 0.5f && khnz_r < depthParams.z) {   // KH_VM_SEE.
        const float khvs_s = khVmSee.Load(int3(px, 0));
        if (khvs_s >= depthParams.z) return khvs_s;
    }
    return khnz_r;
}
// KH_VM_SEE: the view model's translucent planes' transmittance at px where the snapshot holds the scene - 1
// elsewhere and on an unarmed pass. KhFxFinish fades a builtin medium pass by it (shadowMeta2.w 2, KhVmSeeFade).
float KhVmSeeT(int2 px)
{
    float khvt_t = 1.0f;
    [branch] if (shadowMeta2.w > 0.5f) {
        if (khVmSee.Load(int3(px, 0)) >= depthParams.z) khvt_t = khVmT.Load(int3(px, 0));
    }
    return khvt_t;
}
// KH_VM_SEE for any effect: the arm (this pass substitutes - the scene chain past the view model, in a cycle whose
// snapshot landed; not a first-person test), whether a view-model translucent plane wrote its own depth over the
// scene at px (LoadDepthPS returns the snapshot there), and the builtin medium passes' fade, guarded so a pixel no
// plane covers keeps outc exactly.
bool KhVmSeeArmed() { return shadowMeta2.w > 0.5f; }
bool KhVmSeeCovered(int2 px)
{
    bool khvc_c = false;
    // KH_VMSEE_NEST: nested as KhVmSeeT is - fxc evaluates both operands of &&, so the depth fetch ran unarmed too.
    [branch] if (shadowMeta2.w > 0.5f) {
        if (LoadDepthRaw(px) < depthParams.z) khvc_c = khVmSee.Load(int3(px, 0)) >= depthParams.z;
    }
    return khvc_c;
}
float3 KhVmSeeFade(float3 scene, float3 outc, int2 px)
{
    const float khvf_t = KhVmSeeT(px);
    if (khvf_t < 1.0f) outc = lerp(scene, outc, khvf_t);
    return outc;
}
// KH_WSEE for any effect (KhWseeArmed above), the per-pixel rule (sample 0): whether the world's blended layers cover
// px, their transmittance there (1 elsewhere and unarmed; covered layers can be fully transparent: 1 too), the
// nearest layer's raw depth and the raw depth behind them (each the front raw - LoadDepthPS's - where not covered).
bool KhWseeCovered(int2 px)
{
    bool khwc_c = false;
    [branch] if (KhWseeArmed()) {
        float3 khwc_o;
        KhWseeAt(px, LoadDepthPS(px), khwc_o);
        khwc_c = khwc_o.y > khwc_o.x;
    }
    return khwc_c;
}
float KhWseeT(int2 px)
{
    float khwt_t = 1.0f;
    [branch] if (KhWseeArmed()) {
        float3 khwt_o;
        khwt_t = KhWseeAt(px, LoadDepthPS(px), khwt_o);
    }
    return khwt_t;
}
float KhWseeFrontRaw(int2 px)
{
    const float khwf_r = LoadDepthPS(px);
    float3 khwf_o = float3(khwf_r, khwf_r, 0.0f);
    [branch] if (KhWseeArmed()) {
        KhWseeAt(px, khwf_r, khwf_o);
    }
    return khwf_o.x;
}
float KhWseeBehindRaw(int2 px)
{
    const float khwb_r = LoadDepthPS(px);
    float3 khwb_o = float3(khwb_r, khwb_r, 0.0f);
    [branch] if (KhWseeArmed()) {
        KhWseeAt(px, khwb_r, khwb_o);
    }
    return khwb_o.y;
}
// The layer groups covering px (per pixel, sample 0; khWsGz / khWsGt's note): how many (2 or more, up to 7 - the
// three nearest here; 1: one group - the blend by KhWseeT between KhWseeFrontRaw and KhWseeBehindRaw is exact for it
// where khwu_z.x is the front, else between khwu_z.x and the behind; 0: uncovered or unarmed), their raw depths
// nearest first (khwu_z) and transmittances (khwu_t; an absent group: the behind's raw at T 1). Two or more compose
// front to back: (1 - t.x) Q(z.x) + t.x (1 - t.y) Q(z.y) + t.x t.y (1 - t.z) Q(z.z) + t.x t.y t.z Q(behind) - as
// KhWseeMix does per sample.
uint KhWseeGroups(int2 px, out float3 khwu_z, out float3 khwu_t)
{
    const float khwu_r = LoadDepthPS(px);
    uint khwu_n = 0u;
    khwu_z = khwu_r.xxx;
    khwu_t = float3(1.0f, 1.0f, 1.0f);
    [branch] if (KhWseeArmed()) {
        float3 khwu_o;
        KhWseeAt(px, khwu_r, khwu_o);
        if (khwu_o.y > khwu_o.x) {
            khwu_n = khWsGn;
            khwu_z = khwu_n > 0u ? khWsGz : khwu_o.yyy;
            khwu_t = khwu_n > 0u ? khWsGt : float3(1.0f, 1.0f, 1.0f);
        }
    }
    return khwu_n;   // Up to 7 (KhWseeGroups5 / KhWseeGroups7 give more); the three nearest.
}
// KhWseeGroups with five groups: groups 0 - 3 in khwu_z / khwu_t, group 4 in khwu_g4 (depth, T) - the five NEAREST of
// up to seven (fence-type, and per phase the part at its NW position, the 100 % band's and the 25 % band's or far
// part; KhWseeGroups7 gives all).
uint KhWseeGroups5(int2 px, out float4 khwu_z, out float4 khwu_t, out float2 khwu_g4)
{
    const float khwu_r = LoadDepthPS(px);
    uint khwu_n = 0u;
    khwu_z = khwu_r.xxxx;
    khwu_t = float4(1.0f, 1.0f, 1.0f, 1.0f);
    khwu_g4 = float2(khwu_r, 1.0f);
    [branch] if (KhWseeArmed()) {
        float3 khwu_o;
        KhWseeAt(px, khwu_r, khwu_o);
        if (khwu_o.y > khwu_o.x) {
            khwu_n = khWsGn;
            khwu_z = khwu_n > 0u ? float4(khWsGz, khWsSp ? khWsGz2.x : khwu_o.y) : khwu_o.yyyy;
            khwu_t = khwu_n > 0u ? float4(khWsGt, khWsSp ? khWsGt2.x : 1.0f) : float4(1.0f, 1.0f, 1.0f, 1.0f);
            khwu_g4 = (khwu_n > 0u && khWsSp) ? float2(khWsGz2.y, khWsGt2.y) : float2(khwu_o.y, 1.0f);
        }
    }
    return khwu_n;
}
// KhWseeGroups with all seven groups (fence-type, and per phase the part at its NW position, the 100 % band's and the
// 25 % band's or far part): groups 0 - 3 in khwu_z / khwu_t, 4 - 6 in khwu_z2 / khwu_t2 (an absent group: the
// behind's raw at T 1).
uint KhWseeGroups7(int2 px, out float4 khwu_z, out float4 khwu_t, out float3 khwu_z2, out float3 khwu_t2)
{
    const float khwu_r = LoadDepthPS(px);
    uint khwu_n = 0u;
    khwu_z = khwu_r.xxxx;
    khwu_t = float4(1.0f, 1.0f, 1.0f, 1.0f);
    khwu_z2 = khwu_r.xxx;
    khwu_t2 = float3(1.0f, 1.0f, 1.0f);
    [branch] if (KhWseeArmed()) {
        float3 khwu_o;
        KhWseeAt(px, khwu_r, khwu_o);
        if (khwu_o.y > khwu_o.x) {
            khwu_n = khWsGn;
            khwu_z = khwu_n > 0u ? float4(khWsGz, khWsSp ? khWsGz2.x : khwu_o.y) : khwu_o.yyyy;
            khwu_t = khwu_n > 0u ? float4(khWsGt, khWsSp ? khWsGt2.x : 1.0f) : float4(1.0f, 1.0f, 1.0f, 1.0f);
            khwu_z2 = (khwu_n > 0u && khWsSp) ? khWsGz2.yzw : khwu_o.yyy;
            khwu_t2 = (khwu_n > 0u && khWsSp) ? khWsGt2.yzw : float3(1.0f, 1.0f, 1.0f);
        }
    }
    return khwu_n;
}
// KH_WSEE_BEHIND - the scene colour behind px's layers (the per-pixel rule, sample 0) and their transmittance over
// it, as the distance fog's exact stages take them at a pixel whose samples agree (TWIN: effect.hlsl's effect 13, its
// per-sample loop with the sample's own colour as the cap's pixel; the formulas here are its sample-0 ones): the
// capture's colour plus this pass's change since it (scene - R, the chain's input), held >= 0 - B2's where a late
// opaque was drawn before the cycle's first fence-type layer (G = 0.5: its colour is in B2), else B's - and the
// transmittance held by the stacking cap (KhWseeCap): at G = 0.5 Tw Tpost alone against the pixel over B2'; else
// with B2 (bit 3) the product of the two stages' caps (Tw Tpost against the pixel over B2'; Tpre against B2' over
// B'), else the whole product against the pixel over B'. true where
// covered and the colour behind is known; false - behind = scene, t = 1 - unarmed, uncovered, where a surface was
// drawn over the scene after the cycle's first fence-type layer (G = 1: no colour behind it was taken) and where
// KH_VM_SEE's planes cover px (the pixel holds their light): blend by depth alone there, as effect 13's factor blend
// and KhWseeMix do. Every load sits behind the arm and the covered test; scene is the pass's source at px
// (SampleScene). A file calling it names a KhWsee identifier, so it is a colour reader (C++ kh_wsee_col_reader):
// armed only in a cycle whose capture resolved B (B2 with it where bit 3 stands), unarmed in the cycle it first asks.
// LIMITATION: B / B2 are taken before the C++ multiplies the scene by our dynamic-light world shadow
// (kh_dls_world_pass), so a world surface our meshes shadow from a dynamic light reads unshadowed behind a layer.
bool KhWseeBehindColor(int2 px, float3 scene, out float3 behind, out float t)
{
    bool khwb_k = false;
    behind = scene;
    t = 1.0f;
    [branch] if (KhWseeArmed()) {
        float3 khwb_o;
        const float khwb_tr = KhWseeAt(px, LoadDepthPS(px), khwb_o);
        [branch] if (khwb_o.y > khwb_o.x && khwb_o.z < 0.75f) {
            [branch] if (KhVmSeeT(px) >= 1.0f) {
                const float3 khwb_d = scene - khWsR.Load(int3(px, 0)).rgb;   // S - R.
                const float4 khwb_x = khWsLx;   // The rule's T texel (Tw, G, Tpre, Tpost).
                [branch] if (khwb_o.z > 0.25f) {
                    behind = max(khWsB2.Load(int3(px, 0)).rgb + khwb_d, 0.0f);
                    t = KhWseeCap(khwb_x.x * khwb_x.w, scene, behind);
                } else {
                    behind = max(khWsB.Load(int3(px, 0)).rgb + khwb_d, 0.0f);
                    [branch] if ((KhWseeAvail() & 8u) != 0u) {
                        const float3 khwb_b2 = max(khWsB2.Load(int3(px, 0)).rgb + khwb_d, 0.0f);
                        t = KhWseeCap(khwb_x.x * khwb_x.w, scene, khwb_b2) * KhWseeCap(khwb_x.z, khwb_b2, behind);
                    } else {
                        t = KhWseeCap(khwb_tr, scene, behind);
                    }
                }
                khwb_k = true;
            }
        }
    }
    return khwb_k;
}

Texture2D<float> khArbSnap : register(t2);

float3 SampleScene(int2 px)
{
    px = clamp(px, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    float4 khss = sceneColor.Load(int3(px, 0));
    if (centerSize.w > 2.5f) return khss.rgb * khss.a;
    return khss.rgb;
}

// KH_FX_BILINEAR - the scene at a fractional pixel position p (pixel centres at k + 0.5, as SV_Position's), the
// four SampleScene texels around it weighted bilinearly (each tap clamped to the frame, the UI spill lane's
// premultiplication per tap). Displacement effects need it: at the nearest whole pixel a shift below half a pixel is
// none and a growing one steps a pixel at a time (chromatic aberration's ring line, distortion's staircases). At a
// pixel centre the texel itself, exactly: an axis with no fraction takes its first texel by a select, not lerp(a, b,
// 0) - an Inf or NaN neighbour would make that NaN. Four loads a tap, for one.
float3 KhSampleSceneBil(float2 p)
{
    const float2 khsb_q = p - 0.5f;
    const float2 khsb_i = floor(khsb_q);
    const float2 khsb_f = khsb_q - khsb_i;
    const int2 khsb_p = int2(khsb_i);
    const float3 khsb_a = SampleScene(khsb_p);
    const float3 khsb_c = SampleScene(khsb_p + int2(0, 1));
    const float3 khsb_t = khsb_f.x > 0.0f ? lerp(khsb_a, SampleScene(khsb_p + int2(1, 0)), khsb_f.x) : khsb_a;
    const float3 khsb_u = khsb_f.x > 0.0f ? lerp(khsb_c, SampleScene(khsb_p + int2(1, 1)), khsb_f.x) : khsb_c;
    return khsb_f.y > 0.0f ? lerp(khsb_t, khsb_u, khsb_f.y) : khsb_t;
}

// Meaningless in the scene phase - callers gate on the flag.
float KhUiCov(int2 px)
{
    px = clamp(px, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    return sceneColor.Load(int3(px, 0)).a;
}

float Luma(float3 c) { return dot(c, float3(0.2126f, 0.7152f, 0.0722f)); }   // BT.709.

// KH_FX_PX_REF: every size a builtin effect gives in pixels - its "...Px" parameters and the fixed pixel spans inside
// the effects below - is in pixels of a 1080-row frame; this is the factor to the pass's own frame (fxMeta.w, the
// height of the picture it draws), so a look holds at any resolution. Exactly 1 at 1080 rows, and every use multiplies
// it onto the finished pixel quantity, so there each quantity is the unscaled one (fxc, not IEEE-strict, may reorder
// a product: a rounding step on an offset at most). In real pixels stay: sharpen's 1-px neighbourhood (the pixels of
// the frame the pass draws: the display's on the UI, the 3D render's on the scene - they differ at a Sampling other
// than 100 %), anti-aliasing widths (the CRT tube's feather), per-pixel dither and sampling noise (deband's grain, the
// interleaved rotations), and the SSGI chain (its radius is in metres; its resolve works on the gather's own grid).
float KhFxPx() { return max(fxMeta.w, 1.0f) / 1080.0f; }

// KH_GLOW_PYR - the glows' and blurs' pre-filtered picture. C++ kh_glow_build makes one per pass, of that pass's own
// source: level 0 is half the frame (PSGlowSeed), each further level half the one above (PSGlowDown), each texel a
// 4 x 4 tent (1 3 3 1) over what it covers - padded past the frame (edge-clamped) so every level halves exactly and
// one uv addresses them all. The bright-pass effects (bloom 8, halation 12, lens flare 14) take it through their
// threshold, subtracted before the average as their direct taps subtract it; the UI spill lane's premultiply rides
// SampleScene. The pass reads it at t3 (khsgTex) through khsgSamp (s2: anisotropic, clamp).
// fuseMeta.y > 0.5 arms it; fuseMeta.zw is the full-resolution extent level 0 covers (twice its size, not less than
// the frame). A tap reads the level whose texel matches its footprint - the spacing to the next tap - so a sparse
// tap pattern samples a picture already averaged over the gaps between its taps: no copies of the scene at the
// tap spacing, no grain from a jittered pattern. Disarmed (no pyramid for the pass) every effect takes its direct
// taps. (Anamorphic reads a pyramid of its own, halved along its streak alone: KH_ANA_PYR.)
bool KhGlowOn() { return fuseMeta.y > 0.5f; }

// Each footprint is taken a quarter past the tap spacing: bilinear reads of a level are not shift-invariant, so at
// the bare spacing a lone bright pixel under the 5 x 5 bloom still ripples by up to ~18% of its peak; at 1.25 by
// about a tenth, the glow's spread then ~15% past the direct taps' (~10% at the bare spacing).
static const float KH_GLOW_FP = 1.25f;

// pc: a full-resolution position (pixel centres at +0.5). The footprint is fa pixels along the unit axis ax and fp
// across it; hardware anisotropic filtering takes the long side (up to 16:1, then a coarser level).
float3 KhGlowTapA(float2 pc, float2 ax, float fa, float fp)
{
    const float2 khga_e = max(fuseMeta.zw, float2(1.0f, 1.0f));
    const float2 khga_g = KH_GLOW_FP / khga_e;
    return khsgTex.SampleGrad(khsgSamp, pc / khga_e, ax * (fa * khga_g), float2(-ax.y, ax.x) * (fp * khga_g)).rgb;
}
float3 KhGlowTap(float2 pc, float f) { return KhGlowTapA(pc, float2(1.0f, 0.0f), f, f); }

// The pyramid's share of taps sp pixels apart: none at a pixel or less (adjacent pixels leave no gap: the direct taps),
// all from two, linear between. KhGlowSel takes the direct result d unless the share is positive.
float KhGlowMix(float sp) { return KhGlowOn() ? saturate(sp - 1.0f) : 0.0f; }
float3 KhGlowSel(float3 d, float3 p, float m) { return m <= 0.0f ? d : (m >= 1.0f ? p : lerp(d, p, m)); }
float KhGlowSel(float d, float p, float m) { return m <= 0.0f ? d : (m >= 1.0f ? p : lerp(d, p, m)); }

float LinDepth(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    // At / beyond the far plane the denominator crosses zero and flips sign; sky pixels sit at the depth-clear value
    // (1.0), beyond the viewport range entirely - everything past the far plane is "very far".
    if (denom > -1e-7f) return 1e9f;
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

// KH_FX_LINZ (effect.hlsl's note): LinDepth(LoadDepthPS(p)), or the frame's copy of it at t7 where a builtin pass
// armed one (matCtl.x - zero for a user pass, which converts). Unarmed (one value for the whole draw: a uniform
// branch), the conversion alone - t7 untouched, whatever the route left there (effect meshes and the PIP reach this
// too). Armed, the copy is loaded ahead of the in-range branch (texel 0 past the frame, its value unused), which only
// a read past the frame takes. Branches, not ?: (which evaluates both arms); the conversion written once.
Texture2D<float> khLinZ : register(t7);
float KhLinZ(int2 khlz_p)
{
    float khlz_z = 0.0f;
    bool  khlz_cv = true;   // Convert in place.
    [branch] if (matCtl.x > 0.5f) {
        const bool khlz_in = khlz_p.x >= 0 && khlz_p.y >= 0 &&
                             khlz_p.x < (int)fxMeta.z && khlz_p.y < (int)fxMeta.w;
        khlz_z = khLinZ.Load(int3(khlz_in ? khlz_p : int2(0, 0), 0));
        khlz_cv = !khlz_in;
    }
    [branch] if (khlz_cv) khlz_z = LinDepth(LoadDepthPS(khlz_p));
    return khlz_z;
}

// KH_HASH - integer hashing (pcg3d, Jarzynski & Olano, "Hash Functions for GPU Rendering", JCGT 2020) in place of
// frac(sin(dot(p, k)) * 43758.5453). D3D11 specifies sin only on [-100 pi, 100 pi]; the sine hash fed it ~1e5 from
// a pixel position and, through the time-seeded callers (fxMeta.y is seconds since the object's creation and never
// wraps), without bound - vendor-defined noise, and after hours of grain adjacent cells collided outright. The
// input's bits are hashed exactly, so equal inputs give equal values (every caller's structure holds) and distinct
// ones stay distinct at any magnitude; a time seed rides as its own integer lane (KhHashF). Output in [0, 1).
uint3 KhPcg3(uint3 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    return v;
}
float KhHashF(float2 p, uint s) { return (float)(KhPcg3(uint3(asuint(p), s)).x >> 8) * (1.0f / 16777216.0f); }
float Hash(float2 p) { return KhHashF(p, 0u); }
// KH_HASH's range rule for the periodic terms: sin / cos of an argument that grows with time, reduced to one
// period first (the same value, inside the specified range).
float KhSin(float x) { return sin(6.2831853f * frac(x * 0.15915494f)); }
float KhCos(float x) { return cos(6.2831853f * frac(x * 0.15915494f)); }

float2 Hash2(float2 p)
{
    return float2(Hash(p), Hash(p + float2(41.13f, 7.77f)));
}

// The encode's far fence in metres (depthParams' far plane, held to [500, 100000]; 20000 where the encode has none).
// Per-pixel world position below: the depth buffer + inverse view-projection, the view depth clamped to 0.999 x the
// fence.
float KhEncFence()
{
    float khef_den = 1.0f - depthParams.x;
    float khef_far = khef_den < -1.0e-7f ? depthParams.y / khef_den : 20000.0f;
    return clamp(khef_far, 500.0f, 100000.0f);
}

// KH_LOCAL_REL: the same reconstruction without the camera added - relative to fxCam when it is armed, absolute (as
// KhWorldPosFenced) when it is not. KhWorldPosFenced = this + fxCam. KH_NEAR_RECON: the view depth is re-encoded as it
// is down to a millimetre (KhDlfRel's floor; the divide is never at risk - a depth decodes at about the near plane or
// farther). Do not floor at 1 m: that put every surface nearer than a metre (the first-person hands and weapon, a wall
// the player stands against) a metre out along its ray, so a localized volume, a Pulse ring or a user effect took it
// as standing there. KH_WSEE: KhWorldPosRelAt is its body at a given LINEAR view depth khwr_z (LinDepth's) - the fence
// and the KH_NEAR_RECON floor as here - so a two-depth pass reconstructs the point behind the world's blended layers.
float3 KhWorldPosRelAt(float2 uv, float khwr_z, out float khwr_d)
{
    khwr_d = min(khwr_z, KhEncFence() * 0.999f);
    float4 khwr_nd = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f,
                            depthParams.x + depthParams.y / max(khwr_d, 1.0e-3f), 1.0f);   // KH_NEAR_RECON.
    float4 khwr_wp = mul(khwr_nd, invViewProj);
    return khwr_wp.xyz / khwr_wp.w;
}
float3 KhWorldPosRelFenced(int2 px, float2 uv, out float khwr_d)
{
    return KhWorldPosRelAt(uv, LinDepth(LoadDepthPS(px)), khwr_d);
}

float3 KhWorldPosFenced(int2 px, float2 uv, out float khwf_d)
{
    float khwf_raw = LinDepth(LoadDepthPS(px));
    khwf_d = min(khwf_raw, KhEncFence() * 0.999f);
    float4 khwf_nd = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f,
                            depthParams.x + depthParams.y / max(khwf_d, 1.0e-3f), 1.0f);   // KH_NEAR_RECON.
    float4 khwf_wp = mul(khwf_nd, invViewProj);
    // With fxCam armed the inverse is of a rotation-only view (camera at the origin) - small numbers, exact in fp32 -
    // and the camera is added here, once. An absolute inverse mixes the camera's kilometres into every entry and
    // cancels them per pixel, which jitters the reconstruction by centimetres frame to frame.
    return khwf_wp.xyz / khwf_wp.w + (fxCam.w > 0.5f ? fxCam.xyz : float3(0.0f, 0.0f, 0.0f));
}

// The projection's x / y scales (KhgVpos's vp_m00 / vp_m11, KhFxFogEngine's fs_m00 / fs_m11): the lengths of
// viewProj's first and second columns over its xyz rows (a rebase moves only row 3).
float2 KhFxProjScale()
{
    return float2(max(length(float3(viewProj[0].x, viewProj[1].x, viewProj[2].x)), 1e-6f),
                  max(length(float3(viewProj[0].y, viewProj[1].y, viewProj[2].y)), 1e-6f));
}

// Pure function of its arguments; reads no CB.
float3 KhgVpos(float2 vp_px, float vp_d, float2 vp_res, float vp_m00, float vp_m11)
{
    float2 vp_uv = (vp_px + 0.5f) / vp_res;
    return float3((vp_uv.x * 2.0f - 1.0f) * vp_d / vp_m00,
                  (1.0f - vp_uv.y * 2.0f) * vp_d / vp_m11,
                  vp_d);
}

// KH_FX_USER: the engine's own fog at a pixel - the share of its light the engine's fog replaces (0 none - 1 all), from
// the view distance fs_d (LinDepth), the frame fs_res (fxMeta.zw) and the projection's two scales fs_m00 / fs_m11
// (KhFxProjScale()). Fog scatter's engine term (KhFsFog adds the KH fog passes to it, KhFxFogPasses) - one body.
// KH_FOG_SKY: the sky (a depth past the far fence) and, with the engine's fog-end ramp, whatever lies past the fog end
// (where the game shows no geometry, only sky and clouds) take the height fog integrated along the view ray to infinity
// instead of the ramp's full fog: looking up from above the fog shows a clear sky, from inside it a fogged one, and
// toward the horizon the path grows without bound (fully fogged).
float KhFxFogEngine(float2 fs_px, float fs_d, float2 fs_res, float fs_m00, float fs_m11)
{
    float fs_s = 0.0f;

    if (fogParams.w >= 0.5f && depthParams.y < -1.0e-3f)
    {
        // The encode pair's far fence (the m32 gate above guarantees the standard-z shape: 1 - m22 < 0, m32 < 0 ->
        // positive fence).
        float fs_fden = 1.0f - depthParams.x;
        float fs_far = fs_fden < -1.0e-7f ? depthParams.y / fs_fden : 20000.0f;
        float fs_fence = clamp(fs_far, 500.0f, 100000.0f) * 0.999f;
        float fs_de = min(fs_d, fs_fence);
        float fs_distM = length(KhgVpos(fs_px, fs_de, fs_res, fs_m00, fs_m11));
        // Height at the clamped distance: fs_de's analytic ndcZ, immune to the beyond-far mirror and raw-depth teeter.
        float2 fs_uv = (fs_px + 0.5f) / fs_res;
        float4 fs_nd = float4(fs_uv.x * 2.0f - 1.0f, 1.0f - fs_uv.y * 2.0f,
                              depthParams.x + depthParams.y / max(fs_de, 1.0e-3f), 1.0f);   // KH_NEAR_RECON.
        float4 fs_wp = mul(fs_nd, invViewProj);
        // With fxCam armed the inverse is camera-relative and the height comes back relative too; fogColor.w is the
        // camera's absolute altitude, so the camera's own is added here, as the position is.
        float fs_hgt = fs_wp.y / fs_wp.w + (fxCam.w > 0.5f ? fxCam.y : 0.0f);
        float fs_camY = fogColor.w;
        float fs_tr;
        // KH_FOG_SKY: the density model is the branch's own - the engine terms' (fogEngine.x at sea level, falling
        // off with fogParams.y above it, flat below) or the fallback's (fogParams.x * 0.0153 up to fogParams.z,
        // falling off above) - integrated from the camera along the ray's rise fs_sin per metre: the flat part up to
        // the base over fs_sin, the falloff's exp(-f h0) / (f fs_sin). A level or falling ray, or no falloff: no end.
        const bool fs_sky = fs_d >= fs_fence ||
                            (fogEngine.w >= 0.5f && fogEngine.w < 1.5f && fs_distM >= fogEngine.y);

        if (fs_sky)
        {
            float fs_sin = (fs_hgt - fs_camY) / max(fs_distM, 1.0e-4f);
            float fs_f = fogParams.y;
            float fs_base = fogEngine.w >= 0.5f ? 0.0f : fogParams.z;
            float fs_sig = fogEngine.w >= 0.5f ? fogEngine.x : fogParams.x * 0.0153f;
            if (fs_sig <= 0.0f)
                fs_tr = 1.0f;
            else if (fs_sin <= 1.0e-4f || fs_f <= 1.0e-6f)
                fs_tr = 0.0f;
            else
                fs_tr = exp(-fs_sig * (max(fs_base - fs_camY, 0.0f)
                                       + exp(-fs_f * max(fs_camY - fs_base, 0.0f)) / fs_f) / fs_sin);
        }
        else if (fogEngine.w >= 0.5f)
        {
            float fs_ramp = fogEngine.w >= 1.5f
                          ? 1.0f
                          : saturate((fogEngine.y - fs_distM) * fogEngine.z);
            float fs_dh = abs(fs_hgt - fs_camY);
            float fs_k = fogParams.y * fs_dh / max(fs_distM, 1.0e-4f);
            float fs_integ = fs_k < 1.0e-6f ? fs_distM : (1.0f - exp(-fs_distM * fs_k)) / fs_k;
            float fs_minY = min(fs_hgt, fs_camY);
            // KH_FOG_EXP_CAP (PSComposite's note): a zero density read 1 - saturate(NaN) = full fog. TWIN.
            fs_tr = fs_ramp * exp(-fs_integ * fogEngine.x * exp(min(-fogParams.y * max(fs_minY, 0.0f), 60.0f)));
        }
        else
        {
            float fs_dens = fogParams.x * exp(-fogParams.y * max(fs_hgt - fogParams.z, 0.0f));
            fs_tr = exp(-fs_distM * fs_dens * 0.0153f);
        }

        fs_s = 1.0f - saturate(fs_tr);
    }
    return fs_s;
}

// KH_FX_FOGREC: the KH fog passes (effect 13's twin, effect.hlsl's KhFxDistFogF - the same linear-depth ramp and
// skyAmount rule; edit both or neither), two records [startDist, endDist, skyAmount, opacity] (opacity 0 = none), each
// combined with the fog fraction fs_s as an independent medium; unsaturated. KhFsFog's loop (fxParams1 / 2, the
// builtin fog scatter's system lanes) and KhFxFogAt's (khFxFog). One depth per call: KH_WSEE's two are its
// caller's blend (fog scatter's KhFsFogW; effect 13 blends its twin KhFxDistFogF the same way). KH_DLF_FOGREC: a third
// twin, effect.hlsl's KhDlfPassF (one record's factor and its derivative, dynamicLightFog's medium).
float KhFxFogPasses(float fs_s, float fs_d, float4 fs_r0, float4 fs_r1)
{
    float fs_fence = KhEncFence();

    [unroll] for (int fs_i = 0; fs_i < 2; ++fs_i)
    {
        float4 fs_e = fs_i == 0 ? fs_r0 : fs_r1;
        if (fs_e.w > 0.001f)
        {
            float fs_f = saturate((fs_d - fs_e.x) / max(fs_e.y - fs_e.x, 1.0f));
            float fs_w = saturate((min(fs_d, fs_fence) - fs_fence * 0.98f)
                                  / max(fs_fence * 0.019f, 1.0f));
            fs_f = lerp(fs_f, saturate(fs_e.z), fs_w);
            if (fs_d > 1e8f) fs_f = saturate(fs_e.z);   // Belt (the feather already lands here).
            fs_s = 1.0f - (1.0f - fs_s) * (1.0f - fs_f * fs_e.w);
        }
    }

    return fs_s;
}
// The fog the builtin fog scatter sees at px (KhFsFog's twin, with the user's records): 0 none - 1 all, d its view
// depth. The engine's fog and the scene's fog passes where khFxFog is filled (the scene chain).
float KhFxFogAt(int2 px, float d)
{
    const float2 khfa_m = KhFxProjScale();
    float khfa_s = KhFxFogEngine(float2(px), d, float2(fxMeta.z, fxMeta.w), khfa_m.x, khfa_m.y);
    khfa_s = KhFxFogPasses(khfa_s, d, khFxFog[0], khFxFog[1]);
    return saturate(khfa_s);
}

// KH_FS_SELF / KH_FS_SKY - the rules of a screen-space scatter in which each pixel spreads its fog's share s of its
// light over a cone of radius rm s px (fog scatter, effect 23 in effect3.hlsl - the worked example: its taps, their
// weights and the pyramid's footprints):
//   KhFsOwnW(s, rm)  the receiver's own weight: its unscattered share (1 - s) and its own pixel's share of its cone,
//                    3 / (pi max(rm s, 1)^2) (a cone of radius r and height 1 holds pi r^2 / 3; the taps sample the
//                    cone around the pixel - 1 px out at least - so the pixel is not weighted as a tap).
//   KhFsSourceFog(ss, sc, sd, cd)  the fog a source at view depth sd (its own fog ss) scatters with into a receiver at
//                    cd (its own sc): a FARTHER source's light reaches the receiver through the receiver's own column,
//                    so sc - the sky behind a fogged ridge glows onto it; a nearer one its own.
//   KhFsNearFloor(fx, ss, sr, rk, sd, cd)  for a source that reaches (sr, its distance in px, below rk = rm x its
//                    fog): the most fog a NEARER source puts on the receiver's ray (it runs beside that column and
//                    crossed that fog too), tapered over the source's disc. Start fx at sc; after the taps, where
//                    fx > sc, add min(KhFsOwnW(fx, rm) - KhFsOwnW(sc, rm), 0) to the receiver's weight (scene x it
//                    to the sum) - so a sky or past-the-fog-end receiver beside a fogged ridge bleeds.
float KhFsOwnW(float s, float rm)
{
    const float khfo_r = max(rm * s, 1.0f);
    return (1.0f - s) + s * 0.95492966f / (khfo_r * khfo_r);
}
float KhFsSourceFog(float ss, float sc, float sd, float cd)
{
    float khfg_s = ss;
    if (sd > cd) khfg_s = sc;
    return khfg_s;
}
float KhFsNearFloor(float fx, float ss, float sr, float rk, float sd, float cd)
{
    float khfn_x = fx;
    if (sd < cd) khfn_x = max(fx, ss * (1.0f - sr / rk));
    return khfn_x;
}

// A position from the script's params, SQF [x, y, zASL], in engine axes (east, up, north).
float3 KhSqfToEngine(float3 p) { return p.xzy; }

// A source at infinity in direction dir (engine axes: the sun flare's fxParams0.xyz, or lighting1.xyz for the sun /
// moon): the direction projects as a point at infinity (w = 0). False = the source is behind the camera or well off
// screen; spos its uv, sp its pixel (clamped to the frame edge: one past the last pixel at the right / bottom edge).
bool KhFxSunSpot(float3 khss_dir, out float2 spos, out int2 sp)
{
    spos = float2(0.0f, 0.0f);
    sp = int2(0, 0);
    const float4 clip = mul(float4(khss_dir, 0.0f), viewProj);
    if (!(clip.w > 0.01f)) return false;
    const float2 sndc = clip.xy / clip.w;
    if (!all(abs(sndc) < 1.3f)) return false;
    spos = float2(sndc.x * 0.5f + 0.5f, 0.5f - sndc.y * 0.5f);
    sp = int2(saturate(spos) * float2(fxMeta.z, fxMeta.w));
    return true;
}
// Its visibility: depth occlusion at the source (sky = visible, geometry = blocked) over 25 taps around it, the same
// for every pixel of the pass (the builtin sun flare draws it once where it can, KH_FX_SIDE). A tap off the frame is
// absent information, left out of the average (fog scatter's rule): a Load there reads 0, the near plane, which counted
// as blocked and dimmed a sun standing near an edge. KhFxSunSpot clamps the source to the frame edge (one past it), so
// some taps always land inside; with none (a frame a few pixels wide) it reads 0. KH_WSEE: a tap the world's blended
// layers cover sees the sky through them by their transmittance - Tr x the behind depth's verdict + (1 - Tr) x the
// front's (a fence or foliage against the sun does not block it whole), per sample where the tap's pixel's samples
// disagree (KhWseeMix). Armed, the taps are taken again in one loop - not 25 unrolled copies of KhWseeMix's - in the
// same order, so an uncovered tap adds the same value. The scene chain draws this once (side 29); where that side draw
// is unavailable an armed pass's every pixel pays the 25 taps twice and 25 KhWseeMix (3 - 4 loads each, more at a
// disagreeing tap).
// One tap's verdict at view depth khsf_d (khsf_f = KhEncFence()): the far fence's feather, 1 = sky.
float KhSunFlareTap(float khsf_d, float khsf_f)
{
    return saturate((min(khsf_d, khsf_f) - khsf_f * 0.98f)
                    / max(khsf_f * 0.019f, 1.0f));
}
float KhSunFlareVis(int2 sp)
{
    float vis = 0.0f;
    float khsf_n = 0.0f;
    const float khsf_f = KhEncFence();
    [unroll] for (int oy = -2; oy <= 2; ++oy)
    [unroll] for (int ox = -2; ox <= 2; ++ox)
    {
        // KH_FX_PX_REF: nearest whole px.
        const int2 khsf_p = sp + int2(ox, oy) * max((int)floor(3.0f * KhFxPx() + 0.5f), 1);
        if (khsf_p.x < 0 || khsf_p.y < 0 || khsf_p.x >= (int)fxMeta.z || khsf_p.y >= (int)fxMeta.w) continue;
        vis += KhSunFlareTap(LinDepth(LoadDepthPS(khsf_p)), khsf_f);
        khsf_n += 1.0f;
    }
    [branch] if (KhWseeArmed()) {   // KH_WSEE.
        const int khsf_st = max((int)floor(3.0f * KhFxPx() + 0.5f), 1);
        float khsf_w = 0.0f;
        [loop] for (uint khsf_i = 0u; khsf_i < 25u; ++khsf_i)
        {
            const int2 khsf_p = sp + (int2(uint2(khsf_i % 5u, khsf_i / 5u)) - 2) * khsf_st;
            if (khsf_p.x < 0 || khsf_p.y < 0 || khsf_p.x >= (int)fxMeta.z || khsf_p.y >= (int)fxMeta.w) continue;
            const float khsf_r = LoadDepthPS(khsf_p);
            float khsf_v = KhSunFlareTap(LinDepth(khsf_r), khsf_f);
            KhWseeMix(khsf_p, khsf_r, khsf_v, khsf_z, KhSunFlareTap(khsf_z, khsf_f));
            khsf_w += khsf_v;
        }
        vis = khsf_w;
    }
    return khsf_n > 0.0f ? vis / khsf_n : 0.0f;
}
// KH_FLARE_EDGE: the source's fade as it leaves the frame - 1 on screen, falling linearly with the farther axis to 0
// where KhFxSunSpot gives up (1.3 NDC). Past the edge the source is clamped to it and its visibility is the
// border's, so without the fade the flare held that level out to 1.3 and then vanished in one frame.
float KhSunFlareEdge(float2 spos)
{
    const float2 khse_n = abs(spos * 2.0f - 1.0f);
    return saturate((1.3f - max(khse_n.x, khse_n.y)) / 0.3f);
}

// Point-op pass fusion. CPU twin: the chain loops' gate (kh_fuse_appendable, then kh_fuse_append) admits the fusible
// set {1 invert, 2 colorgrade, 3 vignette, 5 grain}, never localized / banded / with textures (KH_FX_TEX_ALL) / spill /
// LUT / custom, so this path carries no masks.
// KH_GRAIN_SIZE: film grain's noise layer - smooth (smoothstep-bilinear) value noise at grain-cell coordinates gp,
// three independent lanes [luma, red, blue] from one hash per lattice corner (pcg3d's three words). khgv_o shifts the
// lattice so two layers never hash the same corner. Each lane in [0, 1), mean 1/2.
float3 KhGrainVn(float2 gp, float khgv_o, uint f)
{
    const float2 ip = floor(gp) + khgv_o;
    float2 fp = frac(gp);
    fp = fp * fp * (3.0f - 2.0f * fp);
    const float3 n00 = (float3)(KhPcg3(uint3(asuint(ip), f)) >> 8u);
    const float3 n10 = (float3)(KhPcg3(uint3(asuint(ip + float2(1, 0)), f)) >> 8u);
    const float3 n01 = (float3)(KhPcg3(uint3(asuint(ip + float2(0, 1)), f)) >> 8u);
    const float3 n11 = (float3)(KhPcg3(uint3(asuint(ip + float2(1, 1)), f)) >> 8u);
    return lerp(lerp(n00, n10, fp.x), lerp(n01, n11, fp.x), fp.y) * (1.0f / 16777216.0f);
}
// Film grain's noise (effect 5 and KhFusePoint's id 5 - one body): smooth value noise over grainSizePx cells plus
// a finer octave over cells 2.13 times smaller, triangular-ish and signed, optional chroma. p0 = [amount, fps,
// grainSizePx, lumaResponse], chroma = the chroma lane. KH_HASH: the frame (time quantized to fps) is the hash's
// integer lane, the cells' own coordinates the other two. KH_GRAIN_SIZE: both octaves over cells of grainSizePx x
// KhFxPx() pixels, so the grain is one picture at any resolution. The gain 0.5929 = 0.5 (the two-octave mean) x
// sqrt(1901 / 1352) keeps the 1080-row amplitude the defaults were tuned at with a per-pixel hash octave (variance
// 1/12, against this value noise's pointwise (1/12)(26/35)^2).
float3 KhGrainGc(float2 pos, float t, float4 p0, float chroma)
{
    const float fps = max(p0.y, 1.0f);
    const uint f = (uint)floor(t * fps);
    const float2 gp = pos / (max(p0.z, 1.0f) * KhFxPx());   // KH_FX_PX_REF.
    const float3 n = KhGrainVn(gp, 0.0f, f) + KhGrainVn(gp * 2.13f, 0.5f, f);   // Coarse + fine octave.
    const float3 gl = (n - 1.0f) * 0.5929f;   // Triangular-ish, signed, zero-mean: lanes [luma, red, blue].
    float3 gc = gl.xxx;
    if (chroma > 0.001f)
        gc = lerp(gc, gl.yxz, chroma);   // Red and blue from their own lanes, green the luma's.
    return gc;
}

float3 KhFusePoint(int id, float3 c, float2 uv, float2 pos, float t,
                   float4 p0, float4 p1, float4 col)
{
    if (id == 1) return (1.0f - saturate(c)) * col.rgb;
    if (id == 2)
    {
        float3 g = c * p0.z;   // KH_GRADE_TINT: effect 2's order - the tint after the saturation.
        float l = Luma(g);
        g = lerp(l.xxx, g, p0.x) * col.rgb;
        g = (g - 0.5f) * p0.y + 0.5f;
        return pow(max(g, 0.0f), max(p0.w, 1.0e-4f));   // Effect 2's twin: see its gamma floor.
    }
    if (id == 3)
    {
        float d = distance(uv, float2(0.5f, 0.5f)) * 1.4142f;
        float v = smoothstep(p0.x, p0.x + max(p0.y, 1e-3f), d);
        return lerp(c, col.rgb, v);
    }
    if (id == 5)
    {
        const float3 gc = KhGrainGc(pos, t, p0, p1.x);   // Effect 5's twin: one body.
        float luma = saturate(Luma(c));
        float resp = lerp(1.0f, 4.0f * luma * (1.0f - luma) * 0.9f + 0.1f, p0.w);
        return max(c + gc * p0.x * resp, 0.0f);   // KH_GRAIN_FLOOR (effect 5's note). TWIN.
    }
    return c;
}

// Fused-stage composite: the packing tail's blend algebra over the running value, plus - write-window lanes only - the
// coverage destination mask (KH_LOCAL_BLEND: a lerp in its pre-composite position under normal / lighten / darken, the
// opacity's factor under the other three). KH_FUSE_CLOCK: each stage on its own clock, fm.z = its pass's age
// (kh_fuse_append) - the fxMeta.y it takes unfused, so a fused grain draws the frame its own pass would.
float3 KhFuseTail(float3 v, float cov, bool uiLane, float2 uv, float2 pos)
{
    int n = (int)fuseMeta.x;
    [loop] for (int s = 0; s < n; ++s)
    {
        float4 fm = fuseStage[s * 4];
        float4 fcol = fuseStage[s * 4 + 3];
        float3 c = KhFusePoint((int)fm.x, v, uv, pos, fm.z,
                               fuseStage[s * 4 + 1], fuseStage[s * 4 + 2], fcol);
        float a = fcol.w;
        int bm = (int)fm.y;
        if (uiLane)   // KH_LOCAL_BLEND: additive, multiply and screen take the coverage on the opacity.
        {
            if (bm == 1 || bm == 2 || bm == 3) a *= cov;
            else c = lerp(v, c, cov);
        }
        float3 mixed = lerp(v, c, a);
        if (bm == 1)      v = v + c * a;
        else if (bm == 2) v = v * lerp(float3(1.0f, 1.0f, 1.0f), c, a);
        else if (bm == 3) v = v + c * a - v * c * a;
        else if (bm == 4) v = max(v, mixed);
        else if (bm == 5) v = min(v, mixed);
        else              v = mixed;
    }
    return v;
}

// KH_FX_TEX - a user effect's own textures (the script's "textures", user0 .. user5; C++ kh_fx_tex_fill): each a layer
// of a texture page bound per draw at t43 + i (null when absent), its layer in khFxTexLay (1 + layer; 0 = absent or
// still loading), read through s0 (anisotropic, wrap - the material sampler). The material shaders' KH_USER_TEX
// functions, by the same names and rules: the selects are ?: chains, not flow, so a filtered sample never sits in
// divergent control flow; with a literal index fxc drops the other five.
// KH_FX_TEX_ALL: a BUILTIN effect's pass binds them too when its object names some (the same script property, on every
// route; none of today's builtins reads them). Builtin code reads them with KhUserTexLod / KhUserTexGrad only:
// PSEffect dispatches through if-chains fxc cannot prove uniform, where an implicit-derivative sample has no
// derivative. For a fullscreen pass at uv = i.pos.xy / fxMeta.zw the gradients are dx = float2(1 / fxMeta.z, 0),
// dy = float2(0, 1 / fxMeta.w); fxMeta.y (the pass's age) drives an animation (the KH_UV_ANIM helpers, KhUvAffine /
// KhUvWarp / KhUvFlip).
Texture2DArray<float4> khUserMap0 : register(t43);
Texture2DArray<float4> khUserMap1 : register(t44);
Texture2DArray<float4> khUserMap2 : register(t45);
Texture2DArray<float4> khUserMap3 : register(t46);
Texture2DArray<float4> khUserMap4 : register(t47);
Texture2DArray<float4> khUserMap5 : register(t48);
SamplerState khFxUserSamp : register(s0);
float KhFxTexLane(int khut_i)
{
    return khut_i == 0 ? khFxTexLay[0].x : khut_i == 1 ? khFxTexLay[0].y
         : khut_i == 2 ? khFxTexLay[0].z : khut_i == 3 ? khFxTexLay[0].w
         : khut_i == 4 ? khFxTexLay[1].x : khFxTexLay[1].y;
}
float KhUserTexLayer(int khut_i) { return max(KhFxTexLane(khut_i) - 1.0f, 0.0f); }
bool KhUserTexValid(int khut_i)
{
    return khut_i >= 0 && khut_i < 6 && KhFxTexLane(khut_i) > 0.5f;
}
float4 KhUserTex(int khut_i, float2 khut_uv)
{
    const float3 khut_c = float3(khut_uv, KhUserTexLayer(khut_i));
    return khut_i == 0 ? khUserMap0.Sample(khFxUserSamp, khut_c)
         : khut_i == 1 ? khUserMap1.Sample(khFxUserSamp, khut_c)
         : khut_i == 2 ? khUserMap2.Sample(khFxUserSamp, khut_c)
         : khut_i == 3 ? khUserMap3.Sample(khFxUserSamp, khut_c)
         : khut_i == 4 ? khUserMap4.Sample(khFxUserSamp, khut_c)
                       : khUserMap5.Sample(khFxUserSamp, khut_c);
}
float4 KhUserTexLod(int khut_i, float2 khut_uv, float khut_lod)
{
    const float3 khut_c = float3(khut_uv, KhUserTexLayer(khut_i));
    return khut_i == 0 ? khUserMap0.SampleLevel(khFxUserSamp, khut_c, khut_lod)
         : khut_i == 1 ? khUserMap1.SampleLevel(khFxUserSamp, khut_c, khut_lod)
         : khut_i == 2 ? khUserMap2.SampleLevel(khFxUserSamp, khut_c, khut_lod)
         : khut_i == 3 ? khUserMap3.SampleLevel(khFxUserSamp, khut_c, khut_lod)
         : khut_i == 4 ? khUserMap4.SampleLevel(khFxUserSamp, khut_c, khut_lod)
                       : khUserMap5.SampleLevel(khFxUserSamp, khut_c, khut_lod);
}
float4 KhUserTexGrad(int khut_i, float2 khut_uv, float2 khut_dx, float2 khut_dy)
{
    const float3 khut_c = float3(khut_uv, KhUserTexLayer(khut_i));
    return khut_i == 0 ? khUserMap0.SampleGrad(khFxUserSamp, khut_c, khut_dx, khut_dy)
         : khut_i == 1 ? khUserMap1.SampleGrad(khFxUserSamp, khut_c, khut_dx, khut_dy)
         : khut_i == 2 ? khUserMap2.SampleGrad(khFxUserSamp, khut_c, khut_dx, khut_dy)
         : khut_i == 3 ? khUserMap3.SampleGrad(khFxUserSamp, khut_c, khut_dx, khut_dy)
         : khut_i == 4 ? khUserMap4.SampleGrad(khFxUserSamp, khut_c, khut_dx, khut_dy)
                       : khUserMap5.SampleGrad(khFxUserSamp, khut_c, khut_dx, khut_dy);
}
float2 KhUserTexSize(int khut_i)
{
    if (!KhUserTexValid(khut_i)) return float2(0.0f, 0.0f);
    uint khut_w = 0, khut_h = 0, khut_n = 0;
    if (khut_i == 0)      khUserMap0.GetDimensions(khut_w, khut_h, khut_n);
    else if (khut_i == 1) khUserMap1.GetDimensions(khut_w, khut_h, khut_n);
    else if (khut_i == 2) khUserMap2.GetDimensions(khut_w, khut_h, khut_n);
    else if (khut_i == 3) khUserMap3.GetDimensions(khut_w, khut_h, khut_n);
    else if (khut_i == 4) khUserMap4.GetDimensions(khut_w, khut_h, khut_n);
    else                  khUserMap5.GetDimensions(khut_w, khut_h, khut_n);
    return float2((float)khut_w, (float)khut_h);   // A page holds maps of one size.
}

// KH_FX_USER - the builtin effect's first and last lines, one body with its PSEffect (which calls both) and a user
// effect's opt-in. KhFxBegin: the object's lanes (KhObjLoad), the depth gate an effect mesh takes against the scene
// pair (a fullscreen pass passes it by construction), the object's view-distance cut (khObjCut) and an effect mesh's
// far-frame arbitration (localParams1.z arms it: the snapshot at t2, and the terrain). Call it first.
void KhFxBegin(VSOut i)
{
    KhObjLoad(i.iobj0, i.iobj1);   // KH_OBJBUF: effect meshes draw per object (the CB lanes).
    // Fullscreen passes are inert by construction (w = 1 -> ndc far below 1), and zeroed / degenerate depthParams
    // stand the test down via the m32 gate.
    if (depthParams.y < -1.0e-3f &&
        depthParams.x + depthParams.y / max(i.pos.w, 1.0e-4f) > 1.0f) discard;
    if (khObjCut > 0.0f && i.pos.w > khObjCut) discard;

    // Far-frame analytic arbitration (flush/effect edition).
    if (localParams1.z >= 0.5f) {
        int2 khaPx = clamp(int2(i.pos.xy), int2(0, 0),
                           int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
        float khaRaw = khArbSnap.Load(int3(khaPx, 0));

        if (khaRaw > 0.000001f && khaRaw < 0.999999f) {
            float khaNdc = (khaRaw - localParams0.z) / max(localParams0.w - localParams0.z, 1e-6f);
            float khaDen = khaNdc - localParams0.x;
            float khaScene = (khaDen > -1e-7f) ? 1.0e9f : localParams0.y / khaDen;
            if (khaScene <= 0.0f) khaScene = 1.0e9f;
            if (i.pos.w > khaScene * (1.0f + localParams1.w) + localParams1.z) discard;
        }

        if (thmParams.w >= 0.5f) {
            float khaHe = KhThmHeight(i.wpos.xz);
            if (khaHe > -1.0e5f && (i.wpos.y - khaHe) < -thmMeta.z) discard;
        }
    }
}

// KH_WSEE: KhFxFinish's localization and band masks at a view depth z (LinDepth's) - each block's body after its depth
// read, so the front mask is the unblended arithmetic - and a covered pixel takes Tr x the mask behind the world's
// blended layers + (1 - Tr) x the front's, per sample where its samples disagree (KhWseeMix). khlm_c = the centre
// relative to the camera (KH_LOCAL_REL). The inverse (localRadii.w) is applied after the blend: linear (and an average
// of masks: affine), so either order gives the same.
float KhFxLocalMaskAt(float3 khlm_c, float2 uv, float khlm_z)
{
    float khlm_d;
    float3 nd3 = abs(KhWorldPosRelAt(uv, khlm_z, khlm_d) - khlm_c) / max(localRadii.xyz, 0.01f);
    // Normalized distance: 1.0 = the mask surface (ellipsoid or box).
    float nd = (localParams0.w > 0.5f)
             ? max(nd3.x, max(nd3.y, nd3.z))   // Cube (Chebyshev).
             : length(nd3);   // Sphere/ellipsoid.
    return 1.0f - smoothstep(1.0f, 1.0f + max(localParams1.x, 0.001f), nd);
}
float KhFxBandMaskAt(int2 px, float khbr_z)
{
    // KH_BAND_RADIAL: the distance of the surface the pixel shows - its view depth taken out along its ray, as the
    // reference says ("metres from the camera") and as the mesh twin measures (SolidMask); the view depth alone reaches
    // ~1.7 x farther toward the screen's sides at a 16:9 field of view. The sky stays the sky. (Distance fog 13 and the
    // fog passes keep the view depth, as the mesh fog does: deliberate.)
    const float2 khbr_m = KhFxProjScale();
    float d = khbr_z > 1.0e8f ? khbr_z : length(KhgVpos(float2(px), khbr_z, fxMeta.zw, khbr_m.x, khbr_m.y));
    float fall = max(bandParams.z, 0.01f);
    float mask = smoothstep(bandParams.x - fall, bandParams.x, d);
    if (bandParams.y > 0.0f)
        mask *= 1.0f - smoothstep(bandParams.y, bandParams.y + fall, d);
    return mask;
}
// KH_FX_MASK_ACC - the pass's localization mask (its inverse applied) and band mask at px, as KhFxFinish forms them
// (TWIN: its two mask blocks, with uv = i.pos.xy / fxMeta.zw - px + 0.5 is i.pos.xy at a pixel centre): at both depths
// through the world's blended layers where KH_WSEE covers, per sample where the samples disagree. 1 on a pass that
// is not localized / not banded. Every depth read behind the lane test, as KhFxFinish's.
float KhFxLocalMask(int2 px)
{
    float khlm_m = 1.0f;
    [branch] if (localParams1.y > 0.5f) {
        precise float3 khlm_c = localParams0.xyz - (fxCam.w > 0.5f ? fxCam.xyz : float3(0.0f, 0.0f, 0.0f));
        const float2 khlm_uv = (float2(px) + 0.5f) / float2(fxMeta.z, fxMeta.w);
        const float khlm_r = LoadDepthPS(px);
        khlm_m = KhFxLocalMaskAt(khlm_c, khlm_uv, LinDepth(khlm_r));
        KhWseeMix(px, khlm_r, khlm_m, khlm_z, KhFxLocalMaskAt(khlm_c, khlm_uv, khlm_z));
        if (localRadii.w >= 0.5f) khlm_m = 1.0f - khlm_m;
    }
    return khlm_m;
}
float KhFxBandMask(int2 px)
{
    float khbm_m = 1.0f;
    [branch] if (bandParams.w > 0.5f) {
        const float khbm_r = LoadDepthPS(px);
        khbm_m = KhFxBandMaskAt(px, LinDepth(khbm_r));
        KhWseeMix(px, khbm_r, khbm_m, khbm_z, KhFxBandMaskAt(px, khbm_z));
    }
    return khbm_m;
}

// KhFxFinish: the pixel finished as the builtin finishes its own - for a builtin medium pass, the fade through the view
// model's translucent planes (KH_VM_SEE: shadowMeta2.w 2, never a user effect's) - the localization mask (and its
// inverse), the band mask (each at both depths where KH_WSEE covers, per sample where its samples disagree), the UI
// lane's coverage, opacity (color.a) and the blend mode, the fused stages, then the packing its composite wants (a
// fullscreen pass writes the composite; an effect mesh returns what the hardware blend expects). scene = the pass's
// source at the pixel (SampleScene), outc = the effect's result, pos = i.pos.xy. Return it.
float4 KhFxFinish(float3 scene, float3 outc, float2 pos)
{
    const int2 px = int2(pos);
    const float2 uv = pos / float2(fxMeta.z, fxMeta.w);
    int bm = (int)sizeAxes.w;

    // KH_LOCAL_BLEND - every mask below (KH_VM_SEE's transmittance, the localization mask and its inverse, the band
    // mask, the UI coverage) is how much of the pass reaches the pixel. Under normal (a chain pass's), lighten and
    // darken it lerps outc toward the scene, which the blend then leaves as it was. Under additive, multiply and screen
    // that lerp would blend the scene into itself where the mask is 0 (an additive local pass at opacity 0.5 brightened
    // the whole picture 1.5x outside its volume), so for those three the masks scale the opacity instead (khlb_m): the
    // picture is untouched where a mask is 0 and blended at the pass's own opacity where it is 1. khlb_m stays exactly
    // 1 under the other three. KhFuseTail takes the UI coverage by the same rule; fused stages carry no other mask
    // (kh_fuse_appendable).
    // KH_FX_MASK_ALPHA - on an effect MESH under normal blend (centerSize.w 0, bm 0) the masks scale khlb_m as well,
    // and the mesh packing below puts it on the alpha the hardware blend reads: the lerp of outc toward `scene` would
    // write the capture back where a mask is 0 (over everything drawn since it was taken, at the draw's opacity). Where
    // the capture equals the pixel the two forms agree for every mask value (lerp(d, lerp(d, o, m), a) = lerp(d, o,
    // a m)); where a mask is 0 the pixel is left as it was. The chain (centerSize.w 1), the UI lane and the other mesh
    // modes keep their expressions. TWIN: the chain's masks and KhFuseTail's coverage rule are KH_LOCAL_BLEND's; the
    // UI lane's coverage is the chain's.
    const bool khlb_scale = bm == 1 || bm == 2 || bm == 3 || (bm == 0 && centerSize.w < 0.5f);
    float khlb_m = 1.0f;

    // KH_VM_SEE (shadowMeta2.w 2: a builtin medium pass): where the snapshot holds the scene, the pass shows through
    // the view model's translucent planes by their transmittance - faded to the scene (which carries the planes' own
    // colour) as they cover it. First, so the masks, the opacity, the blend and the fused stages compose with it as
    // with any mask - and only where a plane covers (T < 1), so every other pixel keeps outc exactly (KhVmSeeFade).
    [branch] if (shadowMeta2.w > 1.5f)
    {
        if (khlb_scale)
        {
            const float khlb_t = KhVmSeeT(px);   // KH_LOCAL_BLEND: the transmittance on the opacity.
            if (khlb_t < 1.0f) khlb_m *= khlb_t;
        }
        else outc = KhVmSeeFade(scene, outc, px);
    }

    if (localParams1.y > 0.5f)
    {
        // KH_LOCAL_REL: the camera-relative point against the centre made relative to the camera. Do not add the
        // camera's kilometres to the point first (KhWorldPosFenced): that rounds it to the world's float step (2 - 4 mm
        // far from the origin), which a sharp falloff turns into a crawling staircase. centre - camera of two nearby
        // floats is exact (precise keeps it from being reassociated).
        precise float3 khlm_c = localParams0.xyz - (fxCam.w > 0.5f ? fxCam.xyz : float3(0.0f, 0.0f, 0.0f));
        const float khlm_r = LoadDepthPS(px);
        float mask = KhFxLocalMaskAt(khlm_c, uv, LinDepth(khlm_r));
        // KH_WSEE: behind the world's blended layers by their transmittance (per sample where they disagree).
        KhWseeMix(px, khlm_r, mask, khlm_z, KhFxLocalMaskAt(khlm_c, uv, khlm_z));
        // localRadii.w >= 0.5 complements the mask - the effect reaches everything except the volume, falloff band and
        // sky included. C++ twin local_radii[3] (addLocalPostFX 'inverse').
        if (localRadii.w >= 0.5f) mask = 1.0f - mask;
        if (khlb_scale) khlb_m *= mask;   // KH_LOCAL_BLEND.
        else outc = lerp(scene, outc, mask);
    }

    // Camera-distance band mask: full strength within [min, max], fading over 'falloff' metres at both edges; max <= 0
    // = unbounded far (sky included). Multiplies with the localization mask.
    if (bandParams.w > 0.5f)
    {
        // KH_BAND_RADIAL (KhFxBandMaskAt): the distance of the surface the pixel shows, along its ray.
        const float khbr_r = LoadDepthPS(px);
        float mask = KhFxBandMaskAt(px, LinDepth(khbr_r));
        // KH_WSEE: behind the world's blended layers by their transmittance (per sample where they disagree).
        KhWseeMix(px, khbr_r, mask, khbr_z, KhFxBandMaskAt(px, khbr_z));
        if (khlb_scale) khlb_m *= mask;   // KH_LOCAL_BLEND.
        else outc = lerp(scene, outc, mask);
    }

    // UI-coverage destination mask (write-window masked lane, centerSize.w = 2): the effect fades smoothly off the UI.
    if (centerSize.w > 1.5f && centerSize.w < 2.5f)
    {
        const float khlb_cov = KhUiCov(px);
        if (khlb_scale) khlb_m *= khlb_cov;   // KH_LOCAL_BLEND.
        else outc = lerp(scene, outc, khlb_cov);   // Spill = w 3, excluded.
    }

    // KH_BLEND_AS_IS: the blend takes the pass's whole result - for a glow or edge pass (bloom, halation, lens flare,
    // anamorphic, sunflare, pulse, outline) outc already holds the picture: additive bloom at opacity 1 doubles it,
    // multiply squares it - and multiply / screen are the display formulas taken as they stand on the linear HDR scene
    // (a colour past 1 brightens under multiply; screen with a value past 1 comes out darker, even below 0).
    // Deliberate - scripts are tuned to it; do not adapt them to HDR.
    if (centerSize.w > 0.5f)
    {
        float a = color.a * khlb_m;   // KH_LOCAL_BLEND: khlb_m is 1 under normal, lighten and darken.
        float3 mixed = lerp(scene, outc, a);
        float3 comp;
        if (bm == 1)      comp = scene + outc * a;   // Additive.
        else if (bm == 2) comp = scene * lerp(float3(1.0f, 1.0f, 1.0f), outc, a);   // Multiply.
        else if (bm == 3) comp = scene + outc * a - scene * outc * a;   // Screen.
        else if (bm == 4) comp = max(scene, mixed);   // Lighten.
        else if (bm == 5) comp = min(scene, mixed);   // Darken.
         else              comp = mixed;   // Normal.

        if (centerSize.w > 1.5f) {
            float4 khuRaw = sceneColor.Load(int3(clamp(px, int2(0, 0),
                int2((int)fxMeta.z - 1, (int)fxMeta.w - 1)), 0));
            if (centerSize.w > 2.5f)
                comp += khuRaw.rgb * (1.0f - khuRaw.a);
            comp = KhFuseTail(comp, khuRaw.a, true, uv, pos);
            return float4(comp, khuRaw.a);   // Coverage passthrough.
        }

        comp = KhFuseTail(comp, 1.0f, false, uv, pos);   // The chain lanes (scene, PIP: 1) and the UI phase (1.25).
        return float4(comp, 1.0f);
    }

    // Blend-mode output packing (meshes: hardware blend against the live framebuffer; intensity pre-applied where blend
    // factors cannot express it).
    if (bm == 1 || bm == 3) return float4(outc * (color.a * khlb_m), 1.0f);   // Additive, screen (KH_LOCAL_BLEND).
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), outc, color.a * khlb_m), 1.0f);   // Multiply.
    if (bm == 4 || bm == 5) return float4(lerp(scene, outc, color.a), 1.0f);   // Lighten, darken (MAX/MIN op).
    return float4(outc, color.a * khlb_m);   // Normal (alpha lerp); KH_FX_MASK_ALPHA: the masks on the alpha.
}
#endif
