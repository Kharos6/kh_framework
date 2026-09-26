// cb.hlsl - the shared prefix, concatenated into every unit by C++ (no
// #include). Any edit changes every unit's shader cache key.

cbuffer CBObj : register(b0)
{
    float4 centerSize;   // xyz = world center (engine space), w = edge length.
    // FP32 jitter rebase: xyz = center minus the pass's rebase origin (the
    // camera); w = 1 arms the rebased vertex transform, whose viewProj is then
    // the rebased matrix. w = 0 keeps the absolute path.
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
    // x = lit flag, y = 1 when receiveShadow is off (KH_SHADOW_SWITCH) or every
    // shadow is (KH_SHADOW_OFF), z = ambient fraction, w = diffuse fraction
    // (y / z / w read through KhObjLanesCb / KhObjLoad).
    float4 lighting0;
    // x = 1 on an effect-chain pass whose near-plane marker (t37, the KH_FX_UNIT section)
    // is this cycle's (KH_NEARZ_MARK), else 0.
    float4 shadowMeta2;   // y = object view-distance cut; z = 1 for a
                          // depth-Off overlay (KH_VOL_WITNESS), else 0; w = the first
                          // vertex of the run VSUserSo is drawing (KH_USER_VS's pass
                          // alone; zero, and unread, everywhere else).
    // Engine-axes rotation rows (row-vector): world = center + local.x*R0 +
    // local.y*R1 + local.z*R2.
    float4 objRot0;
    float4 objRot1;
    float4 objRot2;   // objRot0.w = 1 marks a filled matrix; 0 (the zeroed default) reads as identity.
    // x = 1: normal-blend translucent mesh with the scene capture bound (the
    // flush's fill; the injection writes 0) - the packing composites in
    // Reinhard space against t3 and writes opaque. w = the LOD crossfade
    // dither. y / z unread.
    float4 blendCtl;
    // x = this draw's material table index (base + submesh slot; non-instanced
    // VS), y = the submesh slot (the instanced VS adds it to the instance's own
    // base), z = 1 when x is uniform for every instance, w = per-draw
    // alpha-mode override (>= 0 replaces the table's mode; 3 = the opaque part
    // of a blend split). On every untextured fill x and w are zero and unread,
    // and so are y and z except on an effect pass that drew its side value
    // first: there y (the scene chain's fog scatter / sun flare) or z (the UI
    // lane's coverage probe) arms that read (KH_FX_SIDE, effect.hlsl).
    float4 matCtl;
    float4 fuseMeta;
    float4 fuseStage[12];
    float4 fxParams2;   // Effect parameters [8..11] (C++ twin fx2).
    // x = mode (0 off, 1 camera-relative world, 2 view space, 3 absolute pool),
    // y = point count, z = spot count, w = global distance scale (cb10[2].x).
    float4 dlCtl;
    // xyz = global diffuse multiplier (cb10[3]); w = the script intensity,
    // applied to the whole DynLights sum.
    float4 dlGlobal;
    // View matrix columns (world->view rotation) for mode 2, captured with the
    // light list.
    float4 dlView[3];
    // KH_DL_RING: x = where this draw's light records start in khDlRecs (t40),
    // as uint bits - read it through KhDlRec, never directly. C++ twin dl_first.
    float4 dlFirst;
    // KH_USER_LANES: x = the session clock (s), y = this object's creation on
    // it (s). Read through the KhUser* accessors below. C++ twin user_obj.
    float4 khUserObj;
    // KH_FX_TEX: a user effect's own textures - 1 + each one's layer in its page
    // ([0].xyzw = user0 .. user3, [1].xy = user4 / user5; 0 = absent or still
    // loading; [1].zw unused). Zero on every draw but a user effect's. Read
    // through KhUserTex* (the KH_FX_UNIT section). C++ twin fx_tex.
    float4 khFxTexLay[2];
};

 cbuffer CBFrame : register(b1)
{
    row_major float4x4 viewProj;   // Rebased on the two mesh passes (see centerRel); absolute elsewhere.
    row_major float4x4 invViewProj;   // Clip -> world (row-vector convention). Camera-relative
                                      // when fxCam.w = 1 (every reader adds fxCam.xyz back:
                                      // KhWorldPosFenced, KhFsFog); absolute when fxCam.w = 0.
    float4 lighting1;   // xyz = unit vector toward the sun/moon (engine axes), w = lighting valid flag.
    // rgb = the engine's sun colour in HDR scene units ((1,1,1) before the
    // first publish), w = shadow-map strength.
    float4 lighting2;
    float4 shadowMeta;   // x = cascade count, y = depth compare sign, z = bias, w = atlas size
                         // (px).
    float4 shadowTiles[8];   // Per-cascade atlas rect in UV: x0, y0, x1, y1.
    float4 shadowMats[24];   // Per-cascade world->atlasUV+depth 4x3: 3 rows of [a, b, c, t].
    // View-paired bands: the engine's decoded receiver contract. bandView rows
    // are the view matrix columns (world->view, row-vector convention):
    // viewPos.k = dot(float4(wpos,1), bandView[slot*3+k]). bandMat rows are
    // PSC_ShadowmapMatrix rows: uvz.k = dot(float4(vp,1), row).
    float4 bandMat[24];
    float4 bandView[24];
    float4 bandBorder[8];   // x = near, y = far, z = fade, w = 0 invalid / 1+texIndex.
    // The mask cast's lanes (PSMaskCast, static.hlsl): our shadow painted into
    // the engine's screen-space shadow mask from the engine view the fire
    // reconstructs the scene through. castView[0].xyz = that view's row 3
    // (translation) and castMat[0..2].xyz its inverse rotation laid out for
    // KhCastWorld's row dots - the cofactor inverse frozen with the view, or the
    // view's own rows 0..2 when none was (the same for an orthonormal view);
    // castView[1] = (fov.x, fov.y, maskW, maskH); castView[2] = the frozen sun
    // (xyz) and strength (w). castMat[0..2].w = KH_CAST_OCC's occupancy grid:
    // cells per metre (> 0 arms it, with localityMeta.w) and the grid's low x
    // and z corner. The DLS world pass (PSDlsWorld) reads castMat.xyz and
    // castView[0..1] the same way, and castView[2].xyz as the camera its
    // reconstruction came from.
    float4 castMat[3];
    float4 castView[3];
    // x = view-paired band table valid; w = the unit-stencil arm (C++ twin
    // mask_meta[3], kh_svs_unit_on). y/z unwritten.
    float4 maskMeta;
    float4 fogParams;   // x = fogValue, y = fogDecay, z = fogBase, w = enable.
    float4 fogColor;   // rgb = fog color, w = camera altitude ASL.
    row_major float4x4 sunVP;   // World -> private sun-depth clip (row-vector).
    float4 sunMeta;   // x = valid, y = map size (px), z = compare bias, w = strength.
    // x = pair count, y = t2 list armed, z = sun map older than 0.5 s (the
    // filtered-compare gate), w = KH_OCC_N, the cast-occupancy grid's edge in
    // texels (0, unwritten, leaves the grid unarmed).
    float4 localityMeta;
    float4 locality[32];   // [2i] = center.xyz (engine), [2i+1] = half extents.xyz.
    // rgb = engine ambient colour (HDR scene units) from the located lighting
    // block, or (1,1,1) before the first lock; w = engine-mode flag.
    float4 lightAmb;
    // The engine's own fog terms from the located block: x = density scale
    // (lane 41), y = fog end distance (lane 48), z = inverse ramp range (lane
    // 49), w = terms valid.
    float4 fogEngine;
    float4 fogSky;   // The sky CB's view-elevation gradient control.
    float4 fogSkyCol;   // The sky CB's fog base colour (row 7).
    float4 hazePars;   // Engine distance haze, the sky CB's row 14.
    // x = origin world X, y = origin world Z (SQF y), z = node spacing (m: the terrain grid's own where the
    // framework's terrain matrix is the source, a whole multiple of it past KH_THM_MAX_SIDE nodes a side), w =
    // enabled and texture valid (0 off) as 1 + the split code: the engine's triangle split of each cell class,
    // one base-3 digit per class (KhThmHeight, KH_THM_EXACT).
    float4 thmParams;
    // x = width, y = height (nodes: the texture's texels), z = the terrain band (m,
    // KH_THM_BIAS_M: the discard's clearance and PSMaskCast's snap), w = the
    // view distance (m, KH_THM_MIN_DIST_M) from which the clearance march runs.
    // Written with thmParams by kh_fill_occ; both stay zero with no heightfield.
    float4 thmMeta;
    // zw = the volume copy's own dims (not fxMeta.zw). xy = the copy's depth
    // encode pair (m22, m32), KH_VOL_WITNESS (zero = no witness).
    float4 stenVol;
    // x = transport arm; z = KhVsCore vertex path selector (3 = the seam
    // prepass); y/w = the copy's viewport depth range (min, max), KH_VOL_WITNESS
    // (armed while w > y).
    float4 stenVol2;
    row_major float4x4 sunVP2;   // World -> hero sun-depth clip.
    float4 sunMeta2;   // x = valid, y = size, z = bias, w = half-diag.
    row_major float4x4 sunVP3;   // World -> mid-band sun-depth clip (t26).
    float4 sunMeta3;   // x = valid, y = size, z = bias, w = half-diag.
    row_major float4x4 sunVP4;   // World -> outer-band sun-depth clip (t27).
    float4 sunMeta4;   // x = valid, y = size, z = bias, w = half-diag.
    // mirMeta: x = mirror mask valid (1; 2 = the mirror answers the whole
    // surface, the view-model slice's fill), yz = mask dims, w = the sun shadow
    // range (m; C++ twin mir_meta[3], clamp(g_sun_range, 8, 1000)) that
    // KhSunRangeFade fades every sun shadow out over - 0 = no fade, not a
    // free lane. sunOrigin: the anchor
    // every sunVP* is relative to (subtract from wpos before transforming; zero
    // = world-absolute); w = the far tier's prefilter arm.
    float4 mirMeta;
    float4 sunOrigin;
    // x = engine below-layer extinction, y = armed; z = the cast fire's
    // frozen-depth ulp, relative (KH_CAST_SNAP_FACING, PSMaskCast only).
    float4 fogBelow;
    // Below the fog layer the engine converges on a different colour: fogUw.rgb
    // = cb0[7], the sky fog colour, shaped by the cb0[17] elevation gradient in
    // fogUwGrad.xyz.
    float4 fogUw;
    float4 fogUwGrad;
    row_major float4x4 snapVp;
    float4 snapMeta;
    float4 snapCam;
    // xyz = the per-tier prefilter arms (hero / mid / outer); the far tier's
    // arm is sunOrigin.w. w unread.
    float4 sunPf;
    // World -> far-band sun-depth clip (t32) + x = valid, y = size, z = bias, w
    // = half-diag.
    row_major float4x4 sunVP5;
    float4 sunMeta5;
    // The world cast's own compare bias per tier (x/y/z/w = hero / mid / outer
    // / far, normalized by that tier's own d2v), separate from sunMeta*.z,
    // which the self kernel also reads and which may not move. Zero = fall back
    // to that tier's sunMeta*.z, which every mesh fill site leaves.
    float4 sunCastBias;
    // x = the union's cast bias (normalized by its own D); 0 = fall back to
    // sunMeta.z.
    float4 sunCastBias2;
    // C++ twins dls_meta / dls_ctl / dls_face_slice / dls_spot_vp.
    float4 dlsMeta[8];     // xyz = light world position, w = far plane (0 = no map).
    float4 dlsCtl[8];      // x = spot flag, y = bias const (m), z = spot slice, w = bias slope.
    // slot * 6 + face. x = slice (-1 = none), y = normal-offset arm (replicated
    // per light).
    float4 dlsFaceSlice[48];
    row_major float4x4 dlsSpotVP[8];
    // The dynamic-light shadows obey the shadow view distance like the sun's.
    // xyz = the pass camera (engine axes), w = clamp(shadowVisibility, 8,
    // 1000); 0 = no fade. Written by kh_dls_fill_cb.
    float4 dlsRange;
    // khPass.xyz = the pass's rebase camera (engine axes), w = 1 arms the
    // bucket vertex path's rebase (twin of centerRel.w). khPassObj.x = the
    // engine object view distance (the cut for bucket instances);
    // yzw free. Zero wherever no bucket draws.
    float4 khPass;
    float4 khPassObj;
    // xyz = the camera invViewProj is relative to, w = 1 arms it (every reader
    // adds xyz after the reconstruction); w = 0 = invViewProj is absolute.
    float4 fxCam;
    // KH_USER_LANES: the pass camera - xyz position (engine axes, absolute),
    // w = 1 when filled; the rows are its right, up and forward axes. Read
    // through the KhUser* accessors below. C++ twins user_cam / user_cam_rot.
    float4 khUserCam;
    float4 khUserCamRot[3];
    // KH_VOL_ZOOM (C++ twin sten_vol3): this pass's raster -> the volume
    // copy's, per axis copy = raster * xy + zw (the copy's seam frame drew under
    // its own projection - a zoom between the two frames moves every surface
    // on screen). x = 0 = identity.
    float4 stenVol3;
    // KH_VOL_FOOT (C++ twin sten_vol4): x = 1 when khVolFoot (t33) holds the
    // footprint mask of the copy's seam frame; yzw unused.
    float4 stenVol4;
    // KH_PCSS_CELL (C++ twin sun_lat): each camera tier's texel lattice offset
    // as uint bits (asuint) - hero xy, mid zw; outer xy, far zw. A texel index
    // of that tier plus it is the world-fixed lattice KhPcssRot hashes.
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

// The object record buffer (C++ twin KhObjRec, 7 float4), one per live-scene
// slot, read by every bucket vertex shader through the lane's slot
// (VSInst.islot). Engine axes. size.w = the object's creation on the session
// clock (KH_USER_LANES), pos.w = 1 when receiveShadow is off (KH_SHADOW_SWITCH)
// or every shadow is (KH_SHADOW_OFF), rot0.w = 1 (filled),
// rot1.w = lit ambient fraction, rot2.w = lit diffuse fraction; col carries no
// lifetime envelope (the lane's alpha does). res.xyz = KH_POS_RES, the centre's
// part finer than pos's float: every relative centre a record forms (against
// khPass, against sunOrigin) adds it; res.w unread.
struct KhObjRec { float4 pos; float4 size; float4 rot0; float4 rot1; float4 rot2; float4 col; float4 res; };
StructuredBuffer<KhObjRec> khObjs : register(t39);

// KH_DL_RING: the draw's dynamic-light records, 6 float4 per light (the
// engine's cb11 record: [0] position, [1] spot axis + cone threshold, [2]
// diffuse + cone scale, [3] ambient + cone exponent, [4] offset + (a0,a1,a2),
// [5] fade start + inverse width + OUR shadow slot + 1), points first then
// spots, dlCtl.y + dlCtl.z of them - no count cap. Every read is gated on
// those counts (dlCtl.x >= 0.5 and the index below them): past them the ring
// holds another draw's records. Bound at PS t40 by the C++ append
// (kh_dlr_append), inside StateBackup's saved range.
StructuredBuffer<float4> khDlRecs : register(t40);
float4 KhDlRec(int khdr_i)
{
    return khDlRecs[asuint(dlFirst.x) + (uint)khdr_i];
}

// The per-object lanes a bucket varies per instance and the CB carries per
// draw. Filled by the vertex shader into two flat interpolants (VSOut.iobj0/1)
// and loaded by every mesh pixel shader at entry (KhObjLoad); the lighting /
// far contract / dither reads use these, never lighting0.zw,
// shadowMeta2.xy or blendCtl.w directly (PSDlsMask, a per-object-only shader
// with no VSOut, is the one reader of blendCtl.w itself).
static float khObjAmb = 0.0f;      // lighting0.z twin: base-colour fraction kept in shadow.
static float khObjDif = 0.0f;      // lighting0.w twin: n.L-scaled fraction.
static float khObjCut = 0.0f;      // shadowMeta2.y twin: object view-distance cut (m, 0 = off).
static float khObjDither = 0.0f;   // blendCtl.w twin: the LOD crossfade dither for this draw.
// KH_SHADOW_SWITCH: lighting0.y / KhObjRec pos.w twin - 1 = receiveShadow off,
// or every shadow off (KH_SHADOW_OFF: the video options' shadow view distance
// at 5 m or less): every received shadow term reads 1 (the mesh shaders' sun
// block, DynLights, KhDynLightsPBR). 0 where nothing loaded it.
static float khObjNoRecv = 0.0f;
// KH_USER_LANES: khUserObj.y twin - this object's (or bucket instance's)
// creation on the session clock. The sentinel means 'not loaded': KhUserTime
// then reads the CB, which is this object's on every per-object draw.
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
// The vertex side: the CB lanes (per-object draws)...
// The LOD crossfade's per-pixel cut, one body for every pass that draws a
// fading level (the two colour twins): +v = the
// finer level keeps the pixels below v, -v = the coarser keeps the rest.
// Complementary at every pixel, whichever pass asks.
void KhLodDitherCut(float2 khld_px, float khld_v)
{
    if (khld_v == 0.0f) return;
    float khld_h = frac(52.9829189f * frac(dot(khld_px, float2(0.06711056f, 0.00583715f))));
    if (khld_v > 0.0f) { if (khld_h >= khld_v) discard; }
    else if (khld_h < -khld_v) discard;
}
void KhObjLanesCb(out float4 khoc_a, out float4 khoc_b)
{
    khoc_a = float4(lighting0.z, lighting0.w, khUserObj.y, shadowMeta2.y);   // z = creation (KH_USER_LANES).
    khoc_b = float4(lighting0.y, blendCtl.w, 0.0f, 0.0f);   // x = KH_SHADOW_SWITCH; z / w unused.
}
// ...or the record + lane (bucket draws); the cut is the pass's object view
// distance. khor_slot is the instance's scene slot, unread since the
// distance-field AO (its one reader) was replaced by KH_SSAO; the parameter
// stays so the two bucket vertex shaders keep one call shape.
void KhObjLanesRec(KhObjRec khor_r, float khor_dither, uint khor_slot, out float4 khor_a, out float4 khor_b)
{
    khor_a = float4(khor_r.rot1.w, khor_r.rot2.w, khor_r.size.w, khPassObj.x);   // z = creation (KH_USER_LANES).
    khor_b = float4(khor_r.pos.w, khor_dither, 0.0f, 0.0f);   // x = KH_SHADOW_SWITCH; z / w unused.
}

// ===========================================================================
// USER SHADER CONTRACT (KH_USER_LANES). What a user .hlsl may rely on -
// everything a custom shader can plug into is in THIS file, as a declaration
// or in the text below; nothing of the contract lives anywhere else.
// Where the file goes: Documents\Arma 3\kh_framework\rendering, else any
// loaded mod's 'rendering' folder (searched in that order; the script names
// the file relative to it). The first use compiles it in the background - a
// material draws flat white and an effect pass is skipped until it lands - and
// the result is cached on disk; a compile error is reported once, with fxc's
// own message. A file is read once per mission: restart the mission to pick up
// an edit.
//
// ---- MATERIAL shaders (a material whose shader is a .hlsl path) ----------
// Placed after this file and BEFORE the builtin pixel shaders, and compiled
// three times - the flush's twin (static.hlsl) and the injection's guard and
// arbitration twins (composite) - with KH_TEXTURED = KH_USER_MAT =
// KH_RECEIVE_TEX = 1. It may call anything in this file; it cannot call into
// static.hlsl / composite2.hlsl (they come after it). It defines
//     float3 KhUserShade(KhMatSurf s, float3 wpos, float3 n, float smf)
// While the compile is pending, or if it fails (reported once), the submesh
// draws flat unlit white instead (the white placeholder, with the default
// material); the builtin PBR shades it only if the placeholder itself was not
// built.
//   s     the material sampled at the pixel's UV (KhSampleMat); the object
//         colour already tints s.albedo. s.alpha is informational: coverage is
//         decided around the call (below).
//   wpos  the world position (engine axes: x east, y up, z north; absolute m).
//   n     the shading normal the builtin uses: unit, normal map applied, and
//         reversed on the back face of a two-sided mesh.
//   smf   the sun shadow factor, 0 dark - 1 lit: the received world shadows
//         and the private self-shadow, already min-combined - multiply the sun
//         term by it once and do not stack another sun shadow on it. With
//         receiveShadow off, or every mesh's shadows off (a shadow view
//         distance of 5 m or less in the video options), it is 1 wherever the
//         face turns to the sun.
//   RETURN the lit colour in the engine's linear HDR scene units (the units of
//         lighting2 and lightAmb), before fog. Around the call the builtin
//         applies fog, the coverage (the material's cutout / blend alpha times
//         the object colour's alpha) and the object's blend mode - a user
//         shader cannot change coverage.
// The builtin's own recipe is the reference to copy or wrap: KhApplyPBR(s,
// wpos, n, smf) is what KhUserShade replaces, and ApplyLighting (untextured)
// documents the combine. Rules its pieces follow:
//   - khObjAmb / khObjDif are this object's ambient and sun-diffuse fractions
//     (loaded at the PS entry, per instance on a bucket draw). Scale the
//     ambient and the direct sun by them; never read lighting0.zw directly.
//   - DynLights(wpos, n) / KhDynLightsPBR(...) carry every dynamic light that
//     reaches the mesh, with their own shadows (none with receiveShadow off or
//     every mesh's shadows off); add them as they are.
//   - Ambient occlusion is NOT a shader term: KH_SSAO multiplies the result at
//     the pixel after the draw. s.occ is the material's occlusion map, for the
//     ambient only (as KhPbrAmbient applies it).
//   - Emissive is not added for you: KhApplyPBR adds s.emissive last.
// The material's inputs (KH_USER_SLOTS) - EVERY map slot, routing input and
// param there is belongs to a .hlsl material. The script side (updateRender3D
// "material"), one entry per submesh selector:
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
//   routing   [[input, channel], ...] - which channel ("r" "g" "b" "a") of THIS
//             map feeds a surface input: "occlusion", "roughness", "metallic"
//             (or "metalness"), "alpha", "gloss", "specular". Unrouted, an
//             input takes its map's own convention:
//               occlusion  orm.r - as.g when the slot was named "as"
//               roughness  orm.g - none under "as" (the roughness param)
//               metallic   orm.b - none under "as" (the metalness param)
//               alpha      diffuse.a
//               gloss      specular.a - smdi.b when named "smdi"
//               specular   none (1) - smdi.g when named "smdi"
//   params    "basecolor" [r,g,b], "roughness", "metalness" (or "metallic"),
//             "glossiness", "specularColor" [r,g,b], "fresnel" [N,K] (or an
//             rvmat's "fresnel(N,K)" string), "emissiveIntensity",
//             "normalStrength", "cutoff", "alphaMode" "opaque" | "cutout" |
//             "blend"; and the VERTEX stage's "vertexBound", "vertexStatic"
//             (below). Defaults: basecolor 1,1,1; roughness 0.8; metalness 0;
//             glossiness 0.2; specularColor 1,1,1; fresnel 1.5,0; the two
//             strengths 1; cutoff 0.5; opaque.
// What they arrive as - the fields of s (KhMatSurf, from KhSampleMat):
//   albedo    diffuse.rgb (1 with no map) x basecolor x the object colour.
//   alpha     the routed "alpha" (1 unrouted).
//   nrmT      the tangent-space normal, xy x normalStrength (0,0,1, no map).
//   occ, rough, metal   the routed inputs; with no map behind a route, 1, the
//             roughness param and the metalness param.
//   emissive  emissive.rgb x emissiveIntensity (0 with no map).
//   specF0    slot 4's rgb as sampled (0 with no map); specOn = 1 when that
//             slot holds a "specular" map - the builtin then shades spec-gloss
//             from specF0 and gloss. 0 under "smdi" (its rgb is no F0).
//   gloss     the routed "gloss"; 0 with no map behind it - take
//             KhUserMatGlossiness() then (the builtin arma model does).
//   specTint  the routed "specular" (1 unrouted) x speccolor.rgb (1 with no
//             map) x the specularColor param.
//   fresnelNK the fresnel param.
//   model     0. Set it to 1 before KhApplyPBR(s, ...) to shade with the arma
//             model - spec-gloss from gloss, specTint as F0, the conductor
//             Fresnel of fresnelNK - instead of metal-rough.
// The raw values behind them: KhUserMatHas(slot) - the map resolved and is
// loaded - and KhUserMatBaseColor / Roughness / Metalness /
// EmissiveIntensity / NormalStrength / Cutoff / AlphaMode / SpecularColor /
// Glossiness / Fresnel(), each the script's param (or its default).
// Re-sampling the material (animation): KhUserUv() is the pixel's UV, so
// KhSampleMat(KhUserUv() + offset) re-samples the whole surface and
// KhMatFetch(slot, uv) one map, filtered (the slot numbers above;
// KhMatFetchTexel(slot, uv) is the unfiltered texel at mip 0); drive the
// offset from KhUserTime().
// Textures of its own (KH_USER_TEX): the material's texture slots user0 ..
// user5 (script: [path, "user0", "srgb" | "linear"], linear by default) are
// this shader's alone - no builtin input reads them. Read them with
//     KhUserTex(i, uv)             filtered (matSamp, the material sampler)
//     KhUserTexLod(i, uv, lod)     at a mip level (loops, parallax steps)
//     KhUserTexGrad(i, uv, dx, dy) with explicit derivatives
//     KhUserTexValid(i)            the map resolved and is loaded
//     KhUserTexSize(i)             its size in texels (0 when absent)
// with i a literal 0 - 5 (a non-literal i samples all six and selects). An
// absent or still-loading map reads zero. Declare no texture or sampler of
// your own: nothing binds a register a user shader declares, and a taken
// register fails the compile.
// The surface frame (KH_USER_FRAME), the builtin's own:
//     KhUserGeomNormal()  the interpolated vertex normal, unit, reversed on the
//                         back face of a two-sided mesh (n is it with the
//                         normal map applied)
//     KhUserTangent() / KhUserBitangent()  the orthonormal tangent and
//                         bitangent (zero where the mesh has no usable tangent)
//     KhUserPerturb(t)    normalize(T * t.x + B * t.y + N * t.z) for a
//                         tangent-space normal t - with the material's own
//                         normal map (s.nrmT) it reproduces n exactly; N alone
//                         where there is no tangent
//     KhUserFrontFace()   false on the back face of a two-sided mesh.
// View direction in tangent space (parallax): with v = KhUserViewDir(wpos),
// float3(dot(v, T), dot(v, B), dot(v, N)).
//
// ---- The VERTEX stage of a material shader (KH_USER_VS) -------------------
// Optional. A material .hlsl that also defines
//     void KhUserVertex(inout KhUserVtx v)
// deforms the vertices of the submeshes it is assigned to (it still defines
// KhUserShade; "return KhApplyPBR(s, wpos, n, smf);" keeps the builtin look).
// It is a DEFORM function, not a free vertex shader: it moves the mesh's own
// vertices, in the object's space, and everything downstream - the builtin
// vertex transform, every colour pass, every shadow and depth pass - then
// draws the deformed mesh as if it had been authored so. (A cloth collider
// does not see it: physics stays on the CPU.) It is evaluated ONCE per frame
// for all of them (C++
// kh_uvs_step: a stream-output pass through VSUserSo, static.hlsl), which is
// what keeps a shadow attached to what casts it.
//   v.pos      the vertex, object space, METRES: x right, y up, z forward, the
//              object's centre at 0 - before the object's rotation and
//              position (world = KhUserVtxWorld(v.pos)). Write it. (It is
//              stored in the object's box, so an axis the script set the
//              object's size to ZERO on cannot be displaced along; an imported
//              flat mesh has 0.1 mm there, which is enough.)
//   v.nrm      the unit normal, object space. Write it if the deformation
//              turns the surface (it need not stay unit length).
//   v.tan      the unit tangent, object space (zero where the mesh has none),
//              and v.tanSign its handedness. Write v.tan with v.nrm.
//   v.uv       the texture coordinate. Writable: it is what the pixel stage
//              receives (KhUserUv), the one channel from this stage to that.
//   v.rest     READ: v.pos as authored (or as skinned / simulated this frame -
//              the stage runs after both), whatever you have written to v.pos.
//   v.id       READ: the vertex's index in the mesh's vertex array - stable
//              across frames and LOD levels; hash it for per-vertex variation.
// What it may read: KhUserTime() (the object's age - the animation clock),
// KhUserSessionTime(), the object lanes (centerSize.xyz the centre, sizeAxes.xyz
// the edge lengths, objRot0..2 / KhObjRows the rotation rows, color), the
// KhUserVtx* helpers below, and the MAIN view's camera (KhUserCamera*,
// KhUserViewDir) - the one camera there is at that point: a PIP view and every
// shadow map draw the same deformed mesh, so a vertex turned toward the camera
// faces the main view in all of them. NOT there: the material table and every
// texture - no map is bound to the vertex stage and the table is not loaded,
// so KhUserMat* read zero, and KhUserTex / KhSampleMat / KhMatFetch must not
// be called from it at all (a filtered Sample does not compile in a vertex
// shader); the lighting and fog lanes (zero; lighting1.w = 0 says so);
// khObjAmb / khObjDif.
// The script's two params, with the material's others:
//   "vertexBound"   metres: the farthest KhUserVertex moves any vertex from
//              v.rest. The deformation happens on the GPU, so this promise is
//              all the culling, the LOD radius, the sun-map fit and the cast
//              have: every bounds test pads the object by it. Too small and
//              the mesh is culled at the screen edge while still in view, or
//              its shadow is clipped; too large costs only a looser fit.
//              Default 0 - right only for a stage that stays inside the box.
//   "vertexStatic"  1: the result depends on nothing that changes by itself
//              (no clock, no camera) - it is then evaluated again only when
//              the object moves, turns, resizes, changes colour or material,
//              and its shadow maps are not redrawn every frame. Default 0.
//              (Unread over a skinned or simulated mesh, which moves.)
// Its envelope: an object with a vertex stage is drawn per object (it leaves
// the instanced buckets, as a cloth does) and owns one vertex buffer of the
// mesh's size; a vertex shared by two submeshes belongs to the first of them;
// there are no interpolants of your own from this stage to KhUserShade (v.uv
// is the channel). While the stage compiles the mesh draws undeformed; if it
// fails (reported once) it stays so.
//
// ---- EFFECT shaders (a fullscreen pass or effect mesh given a .hlsl path) --
// The script side: the object's "effect" property is the .hlsl path (where a
// builtin effect's name or id would go) and its "params" property the twelve
// numbers that arrive as fxParams0..2. It defines
//     float4 PSEffect(VSOut i) : SV_Target
// Compiled as this file plus the user file, with MSAA_DEPTH set to the depth
// it reads (the scene's; 0 in a picture-in-picture, whose depth is a
// single-sample copy) and KH_FX_UNIT = 1: the KH_FX_UNIT section at the end of
// this file - the builtin effects' own declarations and helpers, the text
// their shader is built on - is in it. Do not declare what it declares, and
// declare no texture or sampler of your own: nothing binds a register a user
// shader declares, and a taken register fails the compile.
// Where it runs: the scene chain, the UI lane, effect meshes and - a localized
// pass, as the builtins' - a picture-in-picture.
// Bound for it, as for the builtin effects (the section declares each):
//   sceneColor (t0)  the pass's source. SampleScene(px) reads it clamped to the
//                    frame (and premultiplied in the UI spill lane).
//   depthTex (t1)    the depth the pass sees. LoadDepthPS(px) is the builtins'
//                    read: the raw depth (LoadDepthRaw), with our near-plane
//                    marker applied where one of our meshes drew nearer than
//                    the near plane. LinDepth(raw) = metres along the view
//                    axis (1e9 for the far plane and the sky);
//                    KhWorldPosFenced(px, uv, d) = the world position (engine
//                    axes, absolute) and d its distance, fenced short of the
//                    far plane; KhgVpos = the view-space position.
//   khNzMark (t37)   that marker (armed by shadowMeta2.x; LoadDepthPS reads it).
//   khArbSnap (t2)   an effect mesh's arbitration snapshot (KhFxBegin reads it).
//   khsgTex (t3) / khsgSamp (s2)  the pass's source pre-filtered: the pyramid
//                    the builtin glows read, built for a pass whose file calls
//                    KhGlowTap or KhGlowTapA (not thresholded). KhGlowTap(pc, f)
//                    reads it over a footprint of f px at the full-res
//                    position pc (centres at + 0.5), KhGlowTapA along an axis;
//                    KhGlowOn() is false while none is armed, and
//                    KhGlowSel(direct, pyramid, KhGlowMix(spacing)) blends as
//                    the builtins do (direct taps a pixel apart or less, the
//                    pyramid from two).
//   khPfSamp (s1)    linear, clamp, every mip - bound for every user effect draw.
//   khUserMap0..5 (t43 - t48) / khFxUserSamp (s0)  the effect's own textures:
//                    the script's "textures" (updatePostFX, or updateRender3D
//                    for an effect mesh), user0 .. user5, "srgb" for colour
//                    data or "linear" (the default). Read them as a material
//                    shader reads its own: KhUserTex(i, uv) filtered
//                    (anisotropic, wrap), KhUserTexLod(i, uv, lod),
//                    KhUserTexGrad(i, uv, dx, dy), KhUserTexValid(i) (resolved
//                    and loaded), KhUserTexSize(i) (texels; 0 when absent),
//                    with i a literal 0 - 5 (a non-literal i samples all six
//                    and selects). An absent or still-loading texture reads
//                    zero.
// Helpers there, besides those: KhUiCov(px) (the UI coverage, in the UI lane),
// Luma (BT.709), KhFxPx() (the factor from a 1080-row frame to this one - the
// builtins' pixel sizes are 1080-row pixels), KhEncFence(), KhFxFogEngine
// (the engine's fog at a pixel), KhHashF / Hash / Hash2 (integer hashes of
// a float2 - exact at any magnitude; a time seed goes in KhHashF's integer
// lane), KhSin / KhCos (sin / cos reduced to one period - for a growing
// argument), KhGrainGc (the builtin grain's noise).
// The builtin's first and last lines, opt-in (one body with its own shader):
//     KhFxBegin(i);                               call it first
//     return KhFxFinish(SampleScene(px), outc, i.pos.xy);
// KhFxBegin loads the object lanes (KhObjLoad) and, for an effect mesh, takes
// the depth gate, the view-distance cut and the far-frame arbitration.
// KhFxFinish applies what the builtin applies to its result: the localization
// mask and its inverse, the band mask, opacity (color.a) and the blend mode,
// and packs the output for the composite the pass is in. Without them the
// output is written as returned - the rest of this section - and the shader
// should still begin with KhObjLoad(i.iobj0, i.iobj1).
//   fxMeta     x = effect id, y = the object's age in seconds (formed in
//              double: the most precise clock), zw = the target size in px.
//              uv = i.pos.xy / fxMeta.zw.
//   fxParams0..2  the script's twelve effect parameters, in order.
//   color      the object colour; color.a is the script's opacity (a
//              fullscreen pass whose color.a <= 0.001 is skipped undrawn).
//   sizeAxes.w the blend mode (0 normal, 1 additive, 2 multiply, 3 screen,
//              4 lighten, 5 darken).
//   centerSize.w  which composite the pass is in:
//     > 0.5 A FULLSCREEN pass. Opacity, blend mode, the localization mask
//           (localParams0/1, localRadii) and the band mask (bandParams) are
//           NOT applied for a user shader - KhFxFinish applies them as the
//           builtin does.
//           1.0 = the scene chain (and a picture-in-picture's localized pass):
//           t0 is the engine's HDR scene colour at its scene resolve, before
//           the engine's own post-processing and tonemap, and the output
//           REPLACES the pixel's colour (no hardware blend), in the same
//           linear HDR units.
//           1.25 = the UI phase (a pass the script set to affect the UI): t0
//           is the finished display frame, post-tonemap, UI included (0 - 1),
//           its alpha the UI coverage, and the hardware lerps the frame toward
//           the output by that coverage (rgb only). With the script's UI
//           spill, t0's rgb is premultiplied by that coverage, sizeAxes.w
//           reads 0 (normal) and the hardware ADDS the output to the frame's
//           uncovered share - output + frame * (1 - coverage) - so it replaces
//           the UI where the UI is opaque and spills past it.
//     < 0.5 An EFFECT MESH. The output is hardware-blended by the blend mode;
//           return it packed as the builtin's final lines do (normal: rgb,
//           alpha = opacity; additive / screen: rgb * opacity, 1; multiply:
//           lerp(1, rgb, opacity), 1; lighten / darken: lerp(scene, rgb,
//           opacity), 1) - or return KhFxFinish, which does. The builtin's
//           first lines are not applied for you either (KhFxBegin applies
//           them): its depth gate against depthParams, the object's
//           view-distance cut (discard past khObjCut, set by KhObjLoad) and
//           the far-frame arbitration - without them an effect mesh draws
//           past the engine's object view distance.
//
// ---- Both kinds: the accessors below. -------------------------------------
// They read lanes every pass that can run a user shader fills (the mesh flush
// and injection, the PIP and view-model passes, the scene and UI effect
// chains, the PIP's effect passes).
//   KhUserTime()          the object's age: seconds since it was created - the
//                         animation clock. Per object, bucket instances
//                         included. Formed in float on a clock that restarts
//                         with each mission, so it resolves ~0.5 ms after an
//                         hour of mission (an effect's fxMeta.y is exact).
//   KhUserSessionTime()   seconds since the mission's render clock started; one
//                         value for every object - a shared clock.
//   KhUserCameraValid()   true when the camera lanes below are filled.
//   KhUserCameraPos()     the pass camera's position (engine axes, absolute).
//                         For a PIP pass it is the PIP camera.
//   KhUserCameraRight() / KhUserCameraUp() / KhUserCameraForward()
//                         the pass camera's axes (engine axes, unit length);
//                         Forward is the direction it looks.
//   KhUserViewDir(wpos)   unit vector from wpos toward the camera.
//   KhUserUv()            MATERIAL only: the pixel's texture UV.
//   KhUserPixel()         MATERIAL only: the pixel's position in the target, px
//                         (an effect has i.pos.xy).
//   KhUserTex*            both: a material's user0 .. user5 (the MATERIAL
//                         section), an effect's "textures" (the EFFECT section).
//   KhUserMat* / KhUserGeomNormal / KhUserTangent / KhUserBitangent /
//   KhUserPerturb / KhUserFrontFace   MATERIAL only: see the MATERIAL section.
//   KhUserVtxWorld(p) / KhUserVtxWorldDir(d) / KhUserVtxObjectDir(d)
//                         object space <-> world, for the VERTEX stage.
// Object lanes (per-object draws: the drawn object; on a bucket draw these
// are the first instance's - use the accessors): centerSize.xyz its centre
// (engine axes), sizeAxes.xyz its edge lengths (engine axes), objRot0..2.xyz
// its rotation rows (objRot0.w = 1 when set, identity otherwise).
// Scene lanes: lighting1.xyz the unit vector toward the sun or moon (w = 1
// valid), lighting2.rgb its colour and lightAmb.rgb the ambient (HDR scene
// units).
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
float2 KhUserUv()    { return khUserUvPs; }
float2 KhUserPixel() { return khUserPxPs; }
// KH_USER_VS: object space (metres, the object's centre at 0) to world and a
// direction likewise - the builtin transform's own arithmetic (KhVsCore), for
// a vertex stage that needs to know where it is (wind, a world-space wave).
// The object lanes are the drawn object's on every per-object draw, which the
// vertex stage's pass is.
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
#if KH_USER_MAT
// KH_USER_FRAME: set by PSMain / PSComposite where they build the mapped
// normal, before KhUserShade (TWIN).
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
// KH_USER_VS: what KhUserVertex receives and hands back (the VERTEX stage
// section of the USER SHADER CONTRACT). Declared for every material compile,
// so the pixel twins parse a file that defines the stage.
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
// kh_cloth_ground_plane. Returns -1e6 outside the heightfield (= no occlusion evidence).
float KhThmHeight(float2 xz)
{
    float2 g = (xz - thmParams.xy) / max(thmParams.z, 1e-3f);
    if (g.x < 0.0f || g.y < 0.0f ||
        g.x > thmMeta.x - 1.001f || g.y > thmMeta.y - 1.001f) return -1.0e6f;
    int2 c0 = int2(g);
    float2 f = g - float2(c0);
    // KH_SHADOW_GATHER's form: the 2 x 2 block (c0, c0 + 1) in one fetch, sampled at the corner its texels share,
    // half a texel from every boundary. The texture is exactly thmMeta.xy texels (kh_thm_upload_body), and the
    // test above keeps c0 + 1 inside it, so the addressing mode never engages: the texels, and the lerp, are the
    // four Loads' bit for bit. Components: w = c0, z = c0 + (1, 0), x = c0 + (0, 1), y = c0 + (1, 1).
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

// KH_THM_EXACT - the scale the heightfield's tolerances were tuned at: the cell of the grid the heightfield was
// decimated to before it took the matrix whole (at most 1025 nodes a side, kh_thm_autobuild_step's old stride
// (n + 1024) / 1025 over the n nodes a side). They absorb what the engine's own drawn terrain departs from its
// exact ground (its distant LOD), which a finer heightfield does not shrink: the march's end skip and its sub-cell
// slack, the arbitration's terrain tolerance.
float KhThmTolCell()
{
    return thmParams.z * floor((thmMeta.x + 1024.0f) / 1025.0f);
}

// Both callers read the result only through one test - clearance + 1.5 tolerance cells (KhThmTolCell)
// below -thmMeta.z discards the fragment - and the running minimum can only
// fall, so once that test holds the remaining steps cannot change the
// outcome: the march stops there. The test is the callers' own expression,
// evaluated on the running value, so the decision is identical.
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
// KH_SHADOW_GATHER: s1 is also the shadow compares' Gather sampler (KhSunBilinT,
// KhSelfTapT, KhDlsBilin). Gather returns the 2 x 2 texels a bilinear sample
// would read, in one fetch, whatever the filter; only the address mode
// matters, and it is CLAMP - samp_pf's, or the default state a null binding
// invokes. Every pass that runs those kernels binds it: the mesh passes
// (injection, flush, PIP, inFront), the mask cast and the DLS world pass.
// Every map they read is a single-mip view, so the level is mip 0 on every
// path. It is also KhPfMu's sampler (a trilinear read of the moment pyramids at
// an explicit level) and the Gather of KhThmHeight, ShadowMapSample and
// KhBandTap4 - all on those same passes. (Declared above KhThmHeight.)
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
// Same shadowed semantics as KhVolShadowed's default arm (count != 0).
Texture2D<uint2> khMirSten  : register(t28);
float KhMirUnit(float2 khmu_px, float khmu_w, float khmu_h)
{
    // KH_MIR_ZOOM: the mirror is drawn in the volume copy's raster (the seam's
    // projection), so this pixel is looked up through the same map as the count
    // (stenVol3, KhVolTerm): identity when the projections agree.
    if (stenVol3.x > 0.0f) khmu_px = khmu_px * stenVol3.xy + stenVol3.zw;
    int2 khmu_p = int2(clamp(khmu_px.x, 0.0f, khmu_w - 1.0f),
                       clamp(khmu_px.y, 0.0f, khmu_h - 1.0f));
    return (khMirSten.Load(int3(khmu_p, 0)).g != 0u) ? 0.0f : 1.0f;
}
// The fade starts at KH_SUN_FADE_START of the range and completes at 99.5%, so
// every contract built on the 0.995 R rim (tier windows, grid domain,
// eligibility) holds; a short band at the rim pops long shadow tips in and out
// under camera motion.
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
    // The domain is a vertical cylinder around the camera, not a sphere: from
    // altitude a sphere spends its reach vertically. Horizontal and vertical
    // fade separately over the same radius; the min keeps the rim soft on both.
    // Full until KH_SUN_FADE_START, gone at 99.5% (the 0.5% guard completes
    // before the per-caster cliff at R).
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

// KH_VOL_WITNESS. The copy is read a frame late (the injection precedes the
// engine's counting draws), so at a pixel this surface did not cover then, the
// count is another surface's - the body behind a swinging cape, in its own
// shadow, shows through the cape's moving edge. The witness: the distance our
// surface stood at at the pixel in the copy's frame (KhVolWitnessDist - the
// KH_VOL_FOOT mask, else the copy's depth decoded) must be this fragment's view
// distance within KH_VOL_WITNESS_M or KH_VOL_WITNESS_REL of it (the measured
// guard's relative margin). A failed witness takes the mean of the ring of neighbours
// KH_VOL_WITNESS_R pixels out that pass it (the surface's own counts beside
// the uncovered edge), then of the ring twice as far (an edge that moved
// further in the frame), else lit: a count that belongs to no part of this
// surface is never used.
static const float KH_VOL_WITNESS_M = 0.05f;
static const float KH_VOL_WITNESS_REL = 0.02f;
static const int   KH_VOL_WITNESS_R = 2;
// The distance the witness compares against at a copy pixel (m), <= 0 = no
// surface. KH_VOL_FOOT: our own footprint mask when the copy has one - no
// encode to decode, so a moving near plane cannot shift it; otherwise the
// copy's depth plane decoded through the seam's encode (stenVol.xy,
// stenVol2.yw). No finite distance (at or past infinity) = no surface; the
// projection's far is finite (m22 > 1), so a cleared depth texel (1.0) decodes
// to the far plane's distance, which a fragment short of it never matches.
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

// The eight neighbours khvr_r pixels out: the lit sum and count of those that
// pass the witness.
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

// khvt_z = the fragment's view distance (SV_Position.w); <= 0 = no witness (a
// texel that wrote no depth).
float KhVolTerm(float2 khvt_raster, float khvt_z)
{
    // KH_VOL_ZOOM: this pixel's view ray in the copy's pixel grid. View depth
    // does not change with the projection, so khvt_z stands as it is.
    const float2 khvt_c = (stenVol3.x > 0.0f) ? khvt_raster * stenVol3.xy + stenVol3.zw : khvt_raster;
    const int2 khvt_p = KhVolPx(khvt_c);
    // Armed with a witness source: the footprint mask (KH_VOL_FOOT) or the
    // depth plane's encode.
    if (!(khvt_z > 0.0f) || !(stenVol4.x >= 0.5f || stenVol2.w > stenVol2.y) ||
        KhVolWitness(khvt_p, khvt_z)) {
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

// Every sun bilinear compare, ring and kernel. Floor, not truncate, and the
// map's edge texel answering for every texel past it, on both sides
// (KhSelfTapT's and KhDlsBilin's rule): int2() rounds toward zero while frac
// is x - floor(x), so they disagree wherever tx < 0, and a texel past the map
// read as 0 is the nearest depth = occluded. The wide kernel needs both - it
// samples up to KH_PCSS_RMAX texels from a uv inside the window. The ring
// needs them only at the rim, and only below the sizes shipped: its widest
// reach is khcw_sp (<= 4) texels past a uv guarded to 0.002 / 0.998, which
// lands 3.7 texels inside a 4096 map and 0.4 OUTSIDE a 2048 one. Nothing
// differs at 4096; this is what keeps a lower KH_SUN_*_BASE from drawing an
// occluded rim along every tier window edge. In the bilinear compare below
// the edge rule is the Gather sampler's CLAMP addressing; it clamps no index.
// KH_SHADOW_GATHER: the 2 x 2 block (lo, lo + 1 on each axis) in one Gather,
// sampled at the corner its four texels share, (lo + 1) / size: half a texel
// from every texel boundary, so the hardware selects exactly lo and lo + 1
// on each axis. CLAMP repeats the edge texel as the per-index clamp to
// [0, size - 1] did, and the texels come back unfiltered, so every compare
// and weight is the four-Load form's, bit for bit. Components: w = (lo.x,
// lo.y), z = (lo.x + 1, lo.y), x = (lo.x, lo.y + 1), y = (lo.x + 1, lo.y + 1).
// size must be the texture's own width: every sun map is created at its size
// lane (kh_sun_map_ensure), and the DLS kernel takes GetDimensions'.
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

// The cast chain's contact ring (KhSunPcssWT answers with it where the
// penumbra fits): a 3 x 3 ring of bilinear compares at +-khcw_sp
// texels. The spread follows the receiver footprint (clamped 1..4), so the edge
// is averaged over the texels it covers and a one-texel re-rasterization shift
// under a moving sun is a fraction of the transition.
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

// KH_PCSS: percentage-closer soft shadows on every sun compare, both chains
// and the union. The sun is a disc of KH_SUN_DIAM_TAN angular diameter, so a
// caster's penumbra widens by that fraction of the occluder-to-receiver
// distance along the light: a 0.5 m post at 100 m casts a ~0.9 m penumbra and
// its shadow dissolves, a 30 m wall at 1 km keeps a solid core under a ~9 m
// soft rim. That is what stops a grazing sun stretching small shadows to the
// horizon - no distance cutoff, no shadow-range term. The blocker search
// averages every occluder within the widest penumbra the tier's depth window
// allows (the centre texel included, so a point under a caster is never
// missed), which is what keeps overlapping casters on one continuous
// estimate; a tap-count fade by caster distance cannot do that. Where the
// penumbra fits the contact ring the ring answers unchanged, so contact
// shadows are byte-identical to before.
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
// KH_PCSS_CELL: the disc is rotated per cell (interleaved gradient noise over
// the cell index). A cell is the map's texel split 2^k ways per axis, k the
// least that keeps a cell within the receiver's pixel footprint (khpr_fw =
// fwidth of the texel coordinate, its smaller axis; texels no larger than a
// pixel keep k = 0), so the rotation changes from pixel to pixel however far
// the map's texels are magnified - one rotation across a whole magnified
// texel drew its disc noise as blocks of that texel's size. The index is the
// world-fixed lattice (the texel index + khpr_lat, sunLat* for a camera tier,
// 0 for the union, whose window moves only at a re-fit), so the pattern stays
// with the world when the tier's window re-snaps; it changes where k does (a
// footprint crossing a power of two). The 4096-cell period keeps the noise's
// float arithmetic exact.
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
// Penumbra radius in tier texels: half the sun-disc spread over the
// occluder-to-receiver distance (depth units / khpz_iD = metres; / khpz_tw =
// texels). With khpz_zb = 0 it is the widest penumbra the window allows,
// which is the blocker search radius.
float KhPcssRadius(float khpz_z, float khpz_zb, float khpz_tw, float khpz_iD)
{
    return 0.5f * KH_SUN_DIAM_TAN * (khpz_z - khpz_zb) / max(khpz_iD, 1.0e-9f) / max(khpz_tw, 1.0e-9f);
}
// The blocker search: the mean depth of the taps in front of the (biased)
// receiver within khsb_r texels, centre included. -1 = nothing between the
// light and the point.
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
// The cast chain's compare: blocker search, then the contact ring unchanged
// where the penumbra fits it, else the disc of bilinear compares at the
// penumbra radius. khpw_z is the biased receiver depth; khpw_sp the ring
// spread (the contact threshold); khpw_tw / khpw_iD the tier's texel (m) and
// depth units per metre; khpw_fw / khpw_lat the receiver's texel footprint and
// the map's lattice offset (KhPcssRot).
float KhSunPcssWT(Texture2D<float> khpw_m, float khpw_sz, float2 khpw_uv, float khpw_z, float khpw_sp,
                  float khpw_tw, float khpw_iD, float2 khpw_fw, float2 khpw_lat)
{
    // The widest penumbra this window allows at all: at or under the ring
    // spread no blocker can take the kernel past the ring, so the ring answers
    // and the search is skipped. (Where the search ran and found nothing it
    // answered 'lit'; the ring's compares over the same texels are the
    // contact verdict that stood before the search existed.)
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

// The tier-blend weight over the outer window edge. One curve for every blend
// site (cast chain, self kernel, contact carries).
float KhTbW(float khtw_e)
{
    return 1.0f - smoothstep(0.75f, 0.98f, khtw_e);
}
// KH_TIER_BLEND: a finer tier that carried a partial verdict blends with the
// coarser tier resolving it by the finer tier's edge weight.
float KhTbBlend(float khtd_c, float khtd_f, float khtd_w)
{
    return lerp(khtd_c, khtd_f, khtd_w);
}

// KH_SUN_GRAD: every gradient the two sun chains read, taken by the pixel shader
// in uniform control flow and handed down. The chains branch per pixel - a
// tier's window test, the carry's early returns, the mesh shaders' N.L gate -
// and a gradient taken past such a branch reads lanes of its quad that left it
// (undefined in D3D11). Each value is the expression the chain took in place,
// evaluated before any such branch, so a pixel whose quad stayed together reads
// what it always read. A lane whose map is off (a CB flag: uniform) is 0.
// The cast chain's (SunShadowOcclusion): the footprint of each camera tier's
// texel coordinate, then the union's (KhCastTier's khC_fw, the union's rotation).
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
// The self chain's (SunShadowOcclusionSelf): length(fwidth(n)) and the
// receiver's half footprint (khgs), then each tier's and the union's texel
// footprint at the normal-offset point the chain samples (KhSelfTier's khT_fw,
// the union's khsr_fw). Zero where the ladder is never consulted.
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
    // The ladder's uniform exits - no map (SunShadowOcclusionSelf) and zero
    // strength (SunShadowFactorSelf, KH_SHADOW_STRENGTH_SKIP) - so both stay free.
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

// One cast tier for the four camera-anchored bands (hero/mid/outer/far). false
// = fall through with the carry state updated in place. Twin contract with
// KhSelfTier's ladder.
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
        // The receiver footprint in this tier's texels (khC_fw), taken by the
        // pixel shader in uniform control flow (KH_SUN_GRAD).
        float  khC_sp = clamp(0.5f * max(khC_fw.x, khC_fw.y), 1.0f, 4.0f);
        if (khC_u.x > 0.002f && khC_u.x < 0.998f &&
            khC_u.y > 0.002f && khC_u.y < 0.998f &&
            khC_c.z > 0.0f && khC_c.z < 1.0f) {
            // khC_meta.z is texel-priced (1 mm .. 100 mm across the ladder) and
            // on the cast chain this line is the whole bias, so the far tier
            // would push every world shadow bias / tan(elevation) sun-ward; the
            // capped khC_cb is what stops that.
            float khC_b = khC_cb > 0.0f ? khC_cb : khC_meta.z;
            // KH_PCSS: the tier's texel (m) and depth units per metre from
            // the matrix, as KhSelfTier prices them.
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
    // Twin contract: the cast chain here and the self chain carry the identical
    // shape.
    float khtb_occ = -1.0f;
    float khtb_w = 0.0f;
    // The admission test is a hard threshold on occlusion: one ulp below it the
    // band is skipped entirely - no return, no carry, no weight - and the next
    // tier answers at full strength.
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
    float4 c = mul(float4(wpos - sunOrigin.xyz, 1.0f), sunVP);   // Ortho: w = 1 (KH_SUN_ANCHOR).
    float2 uv = float2(0.5f + 0.5f * c.x, 0.5f - 0.5f * c.y);
    // KH_TIER_BLEND: a carried band verdict resolves against whatever the union
    // answers - including its rejections (0 = lit).
    if (uv.x <= 0.001f || uv.x >= 0.999f || uv.y <= 0.001f || uv.y >= 0.999f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    if (c.z <= 0.0f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;

    const float khcu_b = sunCastBias2.x > 0.0f ? sunCastBias2.x : sunMeta.z;

    if (c.z >= 1.0f) return localityMeta.z >= 0.5f
                          ? SunShadowCompareSoft(uv, 1.0f - khcu_b) : 0.0f;   // Filtered.

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
        float khcu_zb = khcu_rw <= 1.0f ? 0.0f : KhSunBlockerZ(khSunDepth, sunMeta.y, uv, khcu_zr, khcu_rs, khcu_rot);
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

// Soft compare for the self term: five bilinear taps in a +/-0.75-texel diamond
// (~2.5-texel penumtra). One tap body for all five maps (fxc resolves a
// resource parameter at inlining). The texel is kept to the map on BOTH sides
// (khst_sz = the map edge in texels): a footprint ring past an edge reads the
// edge texel, never an out-of-range 0 (= the nearest depth = occluded) - the
// same rule KhDlsBilin applies. The fetch has it from the Gather sampler's
// CLAMP addressing; the index clamp below puts each tap's depth gradient
// (khst_d) at the texel the fetch returned.
float KhSelfTapT(Texture2D<float> khst_m, float khst_sz, float2 khst_t, float2 khst_g, float khst_z, float khst_b, float khst_w, float2 khst_o)
{
    float2 khst_tc = khst_t + khst_o - 0.5f;   // Fractional offsets land on the corners.
    float2 khst_f0 = floor(khst_tc);
    float2 khst_fr = khst_tc - khst_f0;
    int2   khst_p0 = int2(khst_f0);
    int2   khst_mx = int2((int)khst_sz - 1, (int)khst_sz - 1);
    // KH_SHADOW_GATHER (KhSunBilinT's note): the block's four texels in one
    // fetch, reordered to this loop's tap order (0, 0) (1, 0) (0, 1) (1, 1).
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

// KH_PCSS, the self chain's blocker search: point loads compared against the
// receiver plane extrapolated to each tap (KhSelfTapT's own rule), centre
// included. -1 = nothing between the light and the point. A receiver's own
// curved surface reads as a blocker at a tiny distance, so a small penumbra
// results and the contact ring answers - the wide kernel is reached only by
// genuinely distant casters.
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
// The self chain's wide kernel: the disc of soft compares at khsp_r texels,
// the slope bias grown by the radius (the textbook bias-with-filter-size, so a
// receiver tilted to the light does not shadow itself across the disc).
float KhSelfPcssT(Texture2D<float> khsp_m, float khsp_sz, float2 khsp_t, float2 khsp_g, float khsp_z, float khsp_b, float khsp_bsl, float khsp_w, float khsp_r, float2 khsp_rot)
{
    float khsp_bb = khsp_b + khsp_bsl * khsp_r;
    float khsp_acc = 0.0f;
    [unroll] for (int khsp_i = 0; khsp_i < 16; ++khsp_i) {
        khsp_acc += KhSelfTapT(khsp_m, khsp_sz, khsp_t, khsp_g, khsp_z, khsp_bb, khsp_w, KhPcssTap(khsp_i, khsp_rot) * khsp_r);
    }
    return khsp_acc * (1.0f / 16.0f);
}

// One cascade of the self ladder: receiver-plane gradient (damped by
// fwidth(n)), normal-offset sampling, a tier-proportional bias floor plus the
// hero-priced slope term, the KH_PCSS blocker search, then either the wide
// penumbra disc or the contact path - a 3x3 footprint-spread pcf ring with the
// prefilter (mean/variance) blend where the footprint exceeds a texel - and
// the tier-blend carry at the window edge. The window test IS the selection: a point inside
// this tier's window takes this tier's verdict, lit included - letting a lit
// verdict fall through to a coarser tier paints the coarse tier's fatter shadow
// rim outside the fine tier's correct one.
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
            // KH_PCSS: the blocker search over the widest penumbra this
            // tier's window allows, then the contact ring unchanged where the
            // penumbra fits it, else the disc at the penumbra radius (never
            // narrower than the receiver footprint, which is the minification
            // the prefilter blend served on the ring path).
            float2 khT_rot = KhPcssRot(khT_t, khT_fw, khT_lat);
            // KhSunPcssWT's skip: a window whose widest penumbra fits the ring
            // spread takes the ring without the search (khT_zb = the receiver's
            // own depth stands in for 'a blocker at no distance': khT_pr = 0,
            // the ring path).
            float khT_rw = KhPcssRadius(khT_c.z, 0.0f, khT_tw, khT_iD);
            float khT_rs = min(max(khT_rw, khT_sp), KH_PCSS_RMAX);
            float khT_zb = khT_rw <= khT_sp ? khT_c.z
                         : KhSelfBlockerZ(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_rs, khT_rot);
            float khT_pr = khT_zb < 0.0f ? 0.0f : KhPcssRadius(khT_c.z, khT_zb, khT_tw, khT_iD);
            float khT_res;
            [branch] if (khT_zb < 0.0f) {
                khT_res = 0.0f;   // Nothing between the light and the point.
            } else if (khT_pr > khT_sp) {
                float khT_bsl = 0.35f * khT_tan * KH_SLOPE_TW(khT_tw) * khT_iD;
                float khT_kr = min(max(khT_pr, 0.5f * max(khT_fw.x, khT_fw.y)), KH_PCSS_RMAX);
                khT_res = KhSelfPcssT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_bsl, khT_sw, khT_kr, khT_rot);
            } else {
                // The centre and the four edge neighbours first; where those five
                // agree the footprint is uniform and the diagonals cannot move the
                // nine-tap mean, so they are skipped. That holds only while the
                // taps overlap: the cross reaches sp texels out and the diagonals
                // sp * sqrt(2), so past sp = 1 a diagonal edge can leave every
                // cross tap lit while a corner is shadowed - hence the gate: the
                // early-out only at sp = 1, all nine taps on grazing and distant
                // footprints.
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
                        // khT_ga is the 'both read lit, do not invent shadow'
                        // floor, as a weight (a hard test steps the blend across an
                        // iso-contour - a halo tracking the penumbra).
                        float khT_ga = max(smoothstep(0.0f, 0.002f, khT_res),
                                           smoothstep(0.10f, 0.30f, khT_pfo));
                        // The prefilter may not brighten the taps beyond the taps'
                        // own uncertainty. Chebyshev's p_max is an UPPER bound on
                        // the lit fraction: tight for a caster edge against open
                        // sky, loose where two casters at different depths share
                        // the footprint (their depth spread is variance with no lit
                        // texel behind it - VSM light bleed, a bright line along
                        // every shadow-meets-shadow boundary). Where the nine taps
                        // are unanimous the prefilter has nothing brighter to add;
                        // where they disagree it keeps its weight (4 r (1 - r),
                        // continuous). Darkening is untouched (a thin caster the
                        // taps straddle). Past the spread clamp the bound relaxes.
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
            // Unconditional: the point is inside this tier's window, so this
            // tier's verdict stands whatever it is. Only the window EDGE
            // consults the next tier, through the same carry the cast chain
            // uses.
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

// The self term: hero, mid, outer, far tiers, then the union map. Twin contract
// with SunShadowOcclusion (the cast chain): identical carry shape.
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
                           true,   // KH_FAR_PF.
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
    float khsr_gc = KH_RPDB_GC(khsr_tw) * khsr_iD;   // KH_RPDB_WORLD_CLAMP twin.
    float2 khsr_g = clamp(
        float2( dot(n, khsr_cr / max(khsr_iR, 1e-9f)) * khsr_k,
               -dot(n, khsr_cu / max(khsr_iR, 1e-9f)) * khsr_k),
        -khsr_gc, khsr_gc);
    float2 khsr_t = khsr_uv * sunMeta.y;
    float khsr_tan = clamp(sqrt(saturate(1.0f - ndl * ndl)) / max(ndl, 0.02f),
                           1.0f, 1.0e4f);
    float khsr_b = max(sunMeta.z, 1.5f * khsr_tw * khsr_iD)   // Tier-proportional floor.
                 + 0.35f * khsr_tan * KH_SLOPE_TW(khsr_tw) * khsr_iD;   // KH_SLOPE_WORLD twin.
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
    float khsr_zb = khsr_rw <= khsr_sp ? khsr_c.z
                  : KhSelfBlockerZ(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_rs, khsr_rot);
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
    // The union is reached only when no tier window held the point; its verdict
    // resolves any carried tier-edge blend and stands.
    return (khtb_occ >= 0.0f) ? KhTbBlend(khsr_res, khtb_occ, khtb_w) : khsr_res;
}

float SunShadowFactorSelf(float3 wpos, float3 wrel, float3 nrm, KhSunSelfGrad khsg)
{
    // KH_SHADOW_STRENGTH_SKIP: sunMeta.w is g_shadow_map_strength, a CB scalar,
    // so this branch is UNIFORM - one scalar test, no divergence (the ladder
    // takes no gradient of its own: KH_SUN_GRAD, khsg). At strength 0 the
    // product is an exact 1 - 0 whatever the ladder answers, and the ladder is
    // the costliest term in the mesh shaders (the four tiers plus the union
    // map, each a KH_PCSS blocker search and up to 16 taps). An HLSL multiply
    // evaluates both sides (1.573); this is the gate that makes the zero free.
    if (saturate(sunMeta.w) <= 0.0f) return 1.0f;
    return 1.0f - SunShadowOcclusionSelf(wrel, nrm, khsg) * saturate(sunMeta.w)
                * KhSunRangeFade(wpos);
}

// Our own near clip: 0.05 m sits an order of magnitude below the engine's floor
// of 0.07, so it can never be the binding constraint.
static const float KH_OWN_NEAR = 0.05f;

// C++ twin KH_RP_STEN_FADE: the mirror replay runs only for a mesh that can
// reach this band (KH_MIR_BAND) - change both together.
static const float KH_STEN_FADE = 1.35f;

// KH_INFRONT: the view-model slice's projection has a near of 0.01 (measured),
// below our floor, so there the floor takes the engine's own near - the pass's
// depthParams pair - and stays non-binding, as it is everywhere else. An
// invalid pair (no perspective m32) keeps the floor alone.
void ClipOwnNear(float khon_w)
{
    float khon_n = KH_OWN_NEAR;
    if (depthParams.y < -1.0e-3f && depthParams.x > 1.0e-6f)
        khon_n = min(khon_n, -depthParams.y / depthParams.x);
    if (khon_w < khon_n) discard;
}

void ClipEdgeSliver(float3 wpos)
{
    // Fail-safe: this clip trusts fxParams0.xyz as the camera. The failure
    // direction is 'fireflies possible', never 'geometry lost'.
    if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return;
    float3 khes_dx = ddx(wpos);
    float3 khes_dy = ddy(wpos);
    // Same failure-direction rule: stand the clip down when either derivative
    // sits within ~4 ulps of the coordinate magnitude (2^-23 = 1.2e-7; 16 = 4
    // ulps squared).
    float khes_ulp = max(max(abs(wpos.x), abs(wpos.y)), abs(wpos.z)) * 1.2e-7f;
    float khes_q2 = khes_ulp * khes_ulp * 16.0f;
    if (dot(khes_dx, khes_dx) < khes_q2 || dot(khes_dy, khes_dy) < khes_q2) return;
    // Close-range stand-down: standing near a face's own plane puts a large
    // real face at true grazing incidence, and a firefly test built for distant
    // degenerate slivers must not eat close-up geometry that merely shares the
    // sliver's angle.
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

// KH_DL_SHADOW - the dynamic-light shadow term. The cube needs no matrix:
// KH_DLS_BASIS is an axis permutation with signs, so for a light-relative point
// the face is the dominant axis, the depth is that component, and the other two
// are the uv. Every failure path returns 1.0: the worst a bug can do is fail to
// darken.
 
static const float KH_DLS_AMB_KEEP = 0.35f;
// The slope term is priced in texels, not metres, so it scales with range as
// the depth error it covers does. 1.5 texels is the filter's own reach (one
// texel for the receiver's, half for the bilinear footprint), the same 1.5 the
// receiver-normal offset uses. KH_DLS_GRAD_TEXELS clamps the gradient, in
// texel-widths of depth per texel of lateral step.
static const float KH_DLS_GRAD_TEXELS = 8.0f;
// The HLSL twin of kh_dls_near, row for row: the receiver's (a, c) must be
// built to the same near the map was rasterised with. The three constants are
// the C++ ones and change with them.
static const float KH_DLS_NEAR_M     = 0.05f;
static const float KH_DLS_NEAR_RATIO = 300.0f;
static const float KH_DLS_NEAR_CAP_M = 0.25f;
float KhDlsNear(float khn_far)
{
    return min(max(khn_far / KH_DLS_NEAR_RATIO, KH_DLS_NEAR_M), KH_DLS_NEAR_CAP_M);
}
// Receiver-distance fade at the shadow view distance, the sun's own rule
// (KhSunRangeFade) on the light maps; without it a lamp's shadow on the ground
// draws at any range while the sun's stops at shadowVisibility.
float KhDlsRangeFade(float3 khrf_p)
{
    if (dlsRange.w <= 0.0f) return 1.0f;
    const float khrf_d = length(khrf_p - dlsRange.xyz);
    return 1.0f - smoothstep(0.94f * dlsRange.w, 0.995f * dlsRange.w, khrf_d);
}
// Bias derivation, so the next reader can check it: the maps are D32_FLOAT
// (kh_dls_ensure_maps), stored as a + c/z with a = f/(f-n) and c = -nf/(f-n),
// so d(stored)/dz = nf/((f-n) z^2) and one float ulp near stored ~ 1 is 2^-24.

Texture2DArray<float> khDlsMaps : register(t36);

// Parity with the sun filter: KhSunBilinT / KhSunSoftT with the texture swapped
// for an array slice - same bilinear weights, same 0.75-texel cross offset,
// same 5-tap average, same rule that a depth map is compared BEFORE it is
// filtered. The compare is built PER TAP: a single projected reference z is
// only correct for a receiver parallel to the map plane; on a tilted one the
// receiver's true depth differs at every texel of the footprint, so (as
// KhSelfTapT does) the reference is corrected per tap by d.x*g.x + d.y*g.y.
// khb_zb = the receiver's BIASED face-axis depth in metres; khb_tc = the
// RECEIVER's own uv in texels (never the filter's offset tap - the gradient
// origin must stay put while the footprint moves); khb_g = metres of face-axis
// depth per texel along +u / +v; a/c/near = the face projection, applied per
// tap.
float KhDlsBilin(float khb_sz, float2 uv, float khb_slice,
                 float khb_zb, float2 khb_tc, float2 khb_g,
                 float khb_a, float khb_c, float khb_near)
{
    float2 tx = uv * khb_sz - 0.5f;
    float2 f = frac(tx);
    // Floor, not truncate: int2 rounds toward zero while frac is x - floor(x),
    // so they disagree wherever tx < 0, and at the outermost texel of every
    // cube face a tap's khb_d below would be taken at the neighbouring texel,
    // not the one the Gather (which floors tx itself) returned.
    int2 p0 = int2(floor(tx));
    int khb_mx = (int)khb_sz - 1;
    // KH_SHADOW_GATHER (KhSunBilinT's note): the block's four texels in one
    // fetch. khb_slice is a whole slice below the array's size (KhDlsFaceUV
    // refuses a negative one; kh_dls_render assigns slices only below
    // dls_slices), where Gather's rounded index and the Load's truncated one
    // are the same slice. Tap order 0..3 = (0,0) (1,0) (0,1) (1,1), the
    // o00/o10/o01/o11 the lerp pair below consumes.
    const float4 khb_gv = khDlsMaps.Gather(khPfSamp, float3((floor(tx) + 1.0f) / khb_sz, khb_slice));
    const float4 khb_gt = float4(khb_gv.w, khb_gv.z, khb_gv.x, khb_gv.y);
    float4 khb_o;
    [unroll] for (int khb_k = 0; khb_k < 4; ++khb_k) {
        int2 khb_q = clamp(p0 + int2(khb_k & 1, khb_k >> 1),
                           int2(0, 0), int2(khb_mx, khb_mx));
        // Texel centres sit at integer + 0.5 in this convention, and khb_tc is
        // in the same one, so the difference is a signed count of texels.
        float2 khb_d = (float2(khb_q) + 0.5f) - khb_tc;
        float khb_zq = khb_zb + khb_d.x * khb_g.x + khb_d.y * khb_g.y;
        float khb_ref = khb_a + khb_c / max(khb_zq, khb_near);
        khb_o[khb_k] = (khb_ref > khb_gt[khb_k]) ? 1.0f : 0.0f;
    }
    return lerp(lerp(khb_o.x, khb_o.y, f.x), lerp(khb_o.z, khb_o.w, f.x), f.y);
}
 
// The 3 x 3 footprint ring (the sun cast's KhSunSoftWT shape). khs_zb / khs_g /
// a / c / near replace a single pre-projected z (the compare is built per tap);
// khs_tc is computed ONCE here from the unoffset receiver uv and handed to
// every tap - the ring moves the FOOTPRINT and must not move the gradient's
// origin. khs_sp is the receiver footprint in map texels, clamped 1..4, priced
// by the caller from fwidth(wpos) before any per-pixel branch.
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

// khd_nrm is the receiver's world normal. A mesh shadowing itself is the common
// case for a lamp beside a prop, and a self-compare with only a constant bias
// acnes wherever the surface slants away; this kernel takes the sun kernel's
// three defences (receiver-normal offset, texel-priced slope term,
// receiver-plane gradient).
 
// The face, slice and uv selection, shared so any second reader asks the SAME
// question the shadow lookup asks. Returns false where the lookup would answer
// "lit" without sampling. Also returns the projection axes the receiver-plane
// gradient needs, from the selection itself so they cannot drift from it: khf_r
// / khf_u = the projection's right / up axes, light-relative (world-parallel:
// dlsMeta.xyz is a translation); khf_sx = the lateral scale (1 for a cube face,
// 1/tan(fov/2) for a spot; one texel spans 2*z/(size*sx) metres). khf_sel is
// the UNOFFSET light-relative position and the only input to the face choice;
// khf_p (offset) is what gets projected - choosing the face from the offset
// position lets a receiver within 1.5 texels of an edge flip to the
// neighbouring face and land at its rim (a lit seam along every face boundary).
// The spot branch never selects and ignores khf_sel.
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
        // Row-vector convention: clip.x = dot(p, column x), and kh_dls_spot_vp
        // builds column x as right * s, so the column's direction is the right
        // axis and its length the lateral scale.
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
        // Dominant axis = the cube face; st is that face's (right, up) pair
        // from KH_DLS_BASIS (the C++ table's twin). The face is chosen from
        // khf_sel; z, st and the axes are read off khf_p on that face.
        const float3 khf_ap = abs(khf_sel);
        float2 khf_st;
        int khf_face;
        // khf_r / khf_u are the same permutation khf_st applies, written as
        // vectors, reproducing KH_DLS_BASIS row for row.
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
    // The cube must not refuse the outermost texel of a face (it has no guard
    // band, and the boundary would answer lit one texel wide on every edge).
    // The spot keeps the refusal: outside its one frustum there is no map, and
    // lit is right.
    if (dlsCtl[khf_slot].x >= 0.5f) {
        if (khf_uv.x <= 0.001f || khf_uv.x >= 0.999f ||
            khf_uv.y <= 0.001f || khf_uv.y >= 0.999f) return false;
    } else {
        khf_uv = saturate(khf_uv);
    }
    return true;
}

// The receiver-plane depth gradient: the bias has to cover how much the
// receiver's depth CHANGES ACROSS THE FILTER FOOTPRINT, and on a plane depth is
// an affine function of map position, so it is corrected exactly instead of
// covered. The cube face maps u = 0.5 + 0.5*sx*(p.r)/z, v = 0.5 -
// 0.5*sx*(p.u)/z, and the receiver plane is n.p = d; solving for z gives dz/du
// = -2 (n.r) z^2 / (sx d), dz/dv = +2 (n.u) z^2 / (sx d), per texel the same
// over the map size. Clamped, not floored: d goes to zero as the receiver turns
// edge-on and the plane genuinely spans unbounded depth inside one texel, so
// the gradient is capped at KH_DLS_GRAD_TEXELS (the KH_RPDB_GC idiom) and that
// sliver is allowed to acne. The divide is guarded by MAGNITUDE, keeping d's
// sign (negative facing the light, positive facing away; flipping it would tilt
// the correction the wrong way on back faces).
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
 
// khd_zunc is the receiver's own depth uncertainty in metres, added to the
// bias, supplied by the caller. Every caller passes 0: a mesh because
// interpolated geometry is exact, and the world pass (KhDlsWorldFactor from
// PSDlsWorld) as well.
// khd_fwp: the receiver's world footprint per screen pixel,
// length(fwidth(wpos)), priced by the caller BEFORE its light loop (a gradient
// inside a loop with a break does not compile).
float KhDlsShadow(int khd_slot, float3 khd_wpos, float3 khd_nrm, float khd_zunc, float khd_fwp)
{
    if (khd_slot < 0 || khd_slot > 7) return 1.0f;
    float4 khd_meta = dlsMeta[khd_slot];
    if (khd_meta.w <= 0.0f) return 1.0f;               // No map for this light.
    const float khd_rf = KhDlsRangeFade(khd_wpos);
    if (khd_rf <= 0.0f) return 1.0f;

    // Face-axis distance before the offset, to price the texel: one texel of a
    // 90 degree face at distance z is 2z / size.
    const float3 khd_p0 = khd_wpos - khd_meta.xyz;
    const float  khd_z0 = max(max(abs(khd_p0.x), abs(khd_p0.y)), abs(khd_p0.z));
    uint khd_mw, khd_mh, khd_me;
    khDlsMaps.GetDimensions(khd_mw, khd_mh, khd_me);
    // An unbound array reports zero dimensions; the texel price would then
    // divide by max(0, 1) and displace the receiver twice its own distance from
    // the light. Answer lit.
    if (khd_mw < 2u) return 1.0f;
    const float  khd_texel = 2.0f * khd_z0 / max((float)khd_mw, 1.0f);
    const int    khd_fbase = khd_slot * 6;
    const bool   khd_off_on = dlsFaceSlice[khd_fbase].y >= 0.5f;
    const float3 khd_n = normalize(khd_nrm);
    // 1.5 texels: one to clear the texel the receiver sits in, half for the
    // bilinear footprint.
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

    // The sun's own 5-tap soft compare on an array slice. Returns occlusion in
    // the sun's convention (1 = blocked); this kernel's contract is lit, so it
    // is inverted once here. Footprint in map texels at this receiver: a face
    // texel at distance z spans 2z / size, scaled by the projection's lateral.
    const float khd_sp = clamp(0.5f * khd_fwp / max(khd_texel / max(khd_fsx, 1.0e-3f), 1.0e-6f), 1.0f, 4.0f);
    const float khd_occ = KhDlsSoft((float)khd_mw, khd_uv, khd_slice,
                                    khd_z - khd_b, khd_g,
                                    khd_a, khd_c, khd_near, khd_sp);
    return saturate(1.0f - khd_occ * khd_rf);   // Thinned by the range fade, not cut.
}
 
// Every dynamic light that reaches this point, summed (KH_DL_RING: no count
// cap): each light's diffuse N.L term, scaled by its own shadow from our
// meshes' maps (KhDlsShadow), plus its per-light ambient (KH_DLS_AMB_KEEP of
// which stays in full shadow), attenuated, the sum scaled by dlGlobal.w. HDR
// light in the engine's scene units, added to the ambient and sun by
// ApplyLighting (the untextured combine); the textured path's twin is
// KhDynLightsPBR.
float3 DynLights(float3 wpos, float3 nrm)
{
    if (dlCtl.x < 0.5f) return float3(0.0f, 0.0f, 0.0f);
    int pointN = (int)dlCtl.y;
    // Every light that reaches this mesh (KH_DL_RING: no count cap).
    int totalN = pointN + (int)dlCtl.z;
    float3 n = normalize(nrm);
    float3 p;

    if (dlCtl.x >= 2.5f) {
        // Mode 3: absolute world positions (engine axes) - no camera dependency
        // at all; the CPU merged the pool in world space.
        p = wpos;
    } else {
        // Modes 1/2: camera-origin decodes. fxParams0.xyz carries the camera on
        // the solid-mesh paths (the ClipEdgeSliver contract); a zeroed camera
        // cannot decode camera-origin lights - stand down rather than shade
        // garbage.
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return float3(0.0f, 0.0f, 0.0f);
        p = wpos - fxParams0.xyz;

        if (dlCtl.x >= 1.5f) {
            p = float3(dot(p, dlView[0].xyz), dot(p, dlView[1].xyz), dot(p, dlView[2].xyz));
            n = float3(dot(n, dlView[0].xyz), dot(n, dlView[1].xyz), dot(n, dlView[2].xyz));
        }
    }

    float3 acc = float3(0.0f, 0.0f, 0.0f);
    const float khs_fwp = length(fwidth(wpos));   // Footprint priced outside the loop.

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
        // KH_DL_ATT_SKIP: both cut-offs above are HARD zeros - the range fade is
        // 1 - saturate(...) past its width, and the cone is the (c > 0) select -
        // so everything below is multiplied by an exact 0 and the light cannot
        // reach this pixel. Skipping it skips KhDlsShadow, which is 36 Loads
        // (KhDlsSoft's 9 x KhDlsBilin's 4). Twin: KhDlsWorldFactor, which has
        // carried this same guard since it was written; the two mesh loops did
        // not. No gradient is skipped with it: khs_fwp is priced before the
        // loop and KhDlsSoft Loads only.
        if (att <= 0.0f) continue;

        // The shadow scales the directional term only. KhDlRec(b + 3) is the
        // per-light ambient - the away-facing glow that makes A3 lights read on
        // surfaces facing away - and a surface in shadow is still inside that
        // glow. A dim light casts a faint shadow and a bright one a hard shadow
        // for free.
        // KH_SHADOW_SWITCH: a mesh with receiveShadow off takes no light's shadow.
        const float khs_sh = khObjNoRecv >= 0.5f ? 1.0f
                           : KhDlsShadow((int)KhDlRec(b + 5).z - 1, wpos, nrm, 0.0f, khs_fwp);
        // A pure-ambient light must still cast: a fraction of the per-light
        // ambient follows the shadow (KH_DLS_AMB_KEEP kept).
        const float khs_amb = lerp(KH_DLS_AMB_KEEP, 1.0f, khs_sh);
        float ndl = max(dot(n, L), 0.0f);
        acc += (dlGlobal.xyz * KhDlRec(b + 2).xyz * ndl * khs_sh
              + KhDlRec(b + 3).xyz * khs_amb) * att;
    }

    return acc * dlGlobal.w;
}

// smf: per-pixel shadow factor from the caller (received world shadows and the
// private sun-depth self term, min-combined - they answer the same question at
// different granularities and must not stack).
 
// Reinhard first, then the lift: x/(1+x) maps the whole HDR range into 0..1
// without discarding anything above 1 (a saturate before the lift flattens
// every flashlit surface to 1).
float3 ApplyLighting(float3 base, float3 wpos, float3 nrm, float smf)
{
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return base;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float3 direct = lighting2.rgb * (ndl * khObjDif * smf);   // Per-pixel receive + self term
                                                              // (min-combined upstream).
    // Ambient occlusion is not a term here: KH_SSAO multiplies the injected
    // pass's result at the pixel (PSSsaoApply, ssao.hlsl) after the draw.
    return base * (lightAmb.rgb * khObjAmb + direct + DynLights(wpos, nrm));
}

// The material lanes every reader names as matParams0..3. In the textured
// variants KhMatLoad fills them from the table entry; the untextured variants
// never load and read them at zero (the mirror-mask gate in PSMain /
// PSComposite reads matParams0.y in both).
static float4 matParams0 = 0.0f, matParams1 = 0.0f, matParams2 = 0.0f, matParams3 = 0.0f;

#if KH_TEXTURED
// Every material map is a layer of a texture page (a Texture2DArray of textures
// sharing width, height, format and mip count); the page is bound per draw at
// t14-t18 and t42, the layer comes from the material table entry. Materials
// whose maps live in the same pages batch across one instanced draw. The arma
// model keeps its AS map in the orm page and its SMDI map in the specular page
// (both read linear there - kh_tex_slot_srgb), and SPECCOLOR alone takes the
// sixth, at t42 (t33 is KH_VOL_FOOT's mask, t34 is read by the replay merges
// in static.hlsl). A user (.hlsl) material takes all six under either naming
// (KH_USER_SLOTS: its specular page is read linear when the script named it
// "smdi"); its own maps (KH_USER_TEX) follow at t43-t48. StateBackup saves
// t0-t48.
Texture2DArray<float4> matDiffuse  : register(t14);
Texture2DArray<float4> matNormal   : register(t15);
Texture2DArray<float4> matOrm      : register(t16);
Texture2DArray<float4> matEmissive : register(t17);
Texture2DArray<float4> matSpecular : register(t18);
Texture2DArray<float4> matSpecColor : register(t42);
SamplerState matSamp : register(s0);

// One entry per material-set slot, C++ twin KhGpuMat (9 float4). p0..p3 = the
// matParams0..3 lanes (map-bound flags, alpha mode, cutoff, normal strength /
// base colour, roughness / metalness, emissive intensity, occ route, rough
// route / metal route, alpha route, gloss route, spec workflow); lay0 =
// diffuse/normal/orm/emissive layers, lay1.x = specular, lay1.y = speccolor;
// p4 / p5 = matParams4 / 5, the arma model's lanes (specular colour,
// glossiness fallback / fresnel N, K, specular route, model): filled for the
// arma model and, KH_USER_SLOTS, for a user material (whose model lane stays
// 0); zero for pbr but the specular route (p5.z), which is -1 (unrouted);
// lay1.zw and lay2 = KH_USER_TEX's user0..user5 layers. p0.x's bits 0-5 are
// the six maps' bound flags, bits 6-11 the user maps'.
struct KhGpuMat { float4 p0; float4 p1; float4 p2; float4 p3; float4 lay0; float4 lay1; float4 p4; float4 p5; float4 lay2; };
StructuredBuffer<KhGpuMat> khMats : register(t38);

// The per-pixel material lanes. KhMatLoad fills them from the table entry once
// per pixel (the index rides the VS interpolant, flat per draw or per
// instance).
static float4 khMatLay0 = 0.0f, khMatLay1 = 0.0f, khMatLay2 = 0.0f;
static float4 matParams4 = 0.0f, matParams5 = 0.0f;
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
    if (matCtl.w >= 0.0f) matParams0.y = matCtl.w;   // The draw's alpha-mode override.
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
// KH_USER_TEX: a user material's own maps (the texture slots user0..user5),
// pages bound per draw at t43-t48 by kh_bind_material (null for every other
// material). See the USER SHADER CONTRACT. The selects are ?: chains, not
// flow, so a filtered sample never sits in divergent control flow; with a
// literal index fxc drops the other five.
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
// KH_USER_SLOTS: the material's six maps and the script's params, as the table
// holds them for this pixel (KhMatLoad runs before KhUserShade). See the USER
// SHADER CONTRACT. KhUserMatHas: i = KhMatFetch's slot (0 diffuse, 1 normal,
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

// The blend split classifies by the texel's own alpha: a point fetch at mip 0
// through the same route, never the filtered sample. The filtered alpha
// averages a transparent neighbour into an opaque texel wherever the footprint
// touches the window (the rim at distance, a uv seam where the hardware picks
// the coarsest mip), so such texels would leave the opaque pass and the part
// pass could not cover them - bites through a solid hull.
float4 KhMatFetchTexel(int slot, float2 uv)
{
    uint kmt_w, kmt_h, kmt_n;
    float2 kmt_t = frac(uv);   // The sampler wraps; so does this.
    int kmt_l = (int)KhMatLayer(slot);
    if (slot == 0) { matDiffuse.GetDimensions(kmt_w, kmt_h, kmt_n);  return matDiffuse.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    if (slot == 1) { matNormal.GetDimensions(kmt_w, kmt_h, kmt_n);   return matNormal.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    if (slot == 2) { matOrm.GetDimensions(kmt_w, kmt_h, kmt_n);      return matOrm.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    if (slot == 3) { matEmissive.GetDimensions(kmt_w, kmt_h, kmt_n); return matEmissive.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    if (slot == 4) { matSpecular.GetDimensions(kmt_w, kmt_h, kmt_n); return matSpecular.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    matSpecColor.GetDimensions(kmt_w, kmt_h, kmt_n);
    return matSpecColor.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0));
}

// KH_MAT_GRAD: KhMatFetch / KhMatRoute with the uv gradients handed in - SampleGrad, the one filtered read
// that is legal in divergent flow, so the slot chain below is a real branch: one fetch per call, not KhMatFetch's
// six (fxc hoists every arm of a chain of implicit-derivative samples). The builtin casters' alpha reads
// (PSSunDepthA, PSInjDepthA, PSDlsMaskA) take their ddx / ddy of the uv at entry, in uniform flow. KhMatFetch
// and KhMatRoute stay as they are: they are the user contract's.
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
// fetch per slot per KhMatFetch call, ~29 per textured pixel when it was measured with five slots). KhSampleMat
// therefore takes the uv's gradients once, in its caller's flow, and fetches with SampleGrad under real branches -
// only the slots this material reads (KhMatNeed), each once (KH_MAT_GRAD). KhMatFetchTexel's chain is Loads (no
// derivative) and is left as is; KhMatFetch / KhMatRoute are the user contract's, unchanged.
struct KhMatTaps { float4 t0; float4 t1; float4 t2; float4 t3; float4 t4; float4 t5; };

// Slot select over a taken tap set - a chain of movc, no fetch, no flow. Slot
// numbering is KhMatFetch's.
float4 KhMatPick(KhMatTaps khmp, int slot)
{
    return slot == 0 ? khmp.t0
         : slot == 1 ? khmp.t1
         : slot == 2 ? khmp.t2
         : slot == 3 ? khmp.t3
         : slot == 4 ? khmp.t4
                     : khmp.t5;
}

// KhMatRoute's decode over an already-taken tap set: same lane encoding (slot =
// r >> 2, channel = r & 3), same negative-is-unrouted fallback, no fetch.
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
    // The arma model (model 1; 0 for pbr and user materials): the specular
    // intensity x SPECCOLOR x specularColor, and the fresnel (N, K). Filled for
    // a user material too (KH_USER_SLOTS), which may set model itself.
    float model; float3 specTint; float2 fresnelNK;
};

// One tap set for the whole surface; the flag guards and the six routes are
// selects over it. The guards still decide what an absent map means (white
// diffuse, flat normal, no emissive, no specular), not whether a fetch happens.
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

KhMatSurf KhSampleMat(float2 uv)
{
    KhMatSurf s;
    // KH_MAT_GRAD: the gradients here, before any branch (this is where the implicit sample took them).
    const float2 khsm_dx = ddx(uv);
    const float2 khsm_dy = ddy(uv);
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
    s.nrmT = (flags & 2) ? (khsm_m.t1.xyz * 2.0f - 1.0f) : float3(0.0f, 0.0f, 1.0f);
    s.nrmT.xy *= matParams0.w;
    s.occ = KhMatRouteTap(khsm_m, matParams2.z, 1.0f);
    s.rough = KhMatRouteTap(khsm_m, matParams2.w, matParams1.w);
    s.metal = KhMatRouteTap(khsm_m, matParams3.x, matParams2.x);
    s.emissive = ((flags & 8) ? khsm_m.t3.rgb : float3(0.0f, 0.0f, 0.0f)) * matParams2.y;
    s.specOn = matParams3.w;
    float4 spc = (flags & 16) ? khsm_m.t4 : float4(0.0f, 0.0f, 0.0f, 0.0f);
    s.specF0 = spc.rgb;
    // The arma model's routes default, C++ side, to AS.g (occ above), SMDI.b
    // (gloss) and SMDI.g (specular); unrouted, gloss falls back to the
    // glossiness param and specular to 1. Its spec workflow lane, metalness
    // and metal route are zero, so metal reads 0 above.
    s.model = matParams5.w;
    s.gloss = KhMatRouteTap(khsm_m, matParams3.z, s.model >= 0.5f ? matParams4.w : spc.a);
    const float3 khsm_sc = (flags & 32) ? khsm_m.t5.rgb : float3(1.0f, 1.0f, 1.0f);
    s.specTint = KhMatRouteTap(khsm_m, matParams5.z, 1.0f) * khsm_sc * matParams4.xyz;
    s.fresnelNK = matParams5.xy;
    return s;
}

// The arma model's Fresnel: the reflectance of a conductor of complex index
// N + iK at incidence cos = khfn_c, the mean of the s and p polarisations -
// the function Arma's procedural "fresnel(N,K)" texture tabulates, term for
// term (AA, BB, FS, FP with sin S tan S = sin^2 S / cos S). 1 at grazing for
// every N, K; ((N-1)^2+K^2)/((N+1)^2+K^2) head-on. The cosine is floored so
// tan S stays finite at grazing, and AA^2 + BB^2 so that N = K = 0 head-on
// (0/0 in the original, a NaN in TexView) reads its limit, 1.
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

// This pixel's Fresnel form: x = N, y = K, z >= 0.5 = the arma model's conductor
// curve, which KhApplyPBR arms for its surface. Unarmed, every reader keeps
// Schlick on F0.
static float4 khFrNK = 0.0f;

// Shared compact GGX core (Cook-Torrance D * G * F / (4 ndv ndl)) for the sun
// term (KhApplyPBR) and the engine dynamic lights (KhDynLightsPBR). Armed
// (khFrNK), F0 is the arma model's specular tint and F the tint times the
// conductor curve at V.H. A branch, not a ternary: fxc evaluates both sides of
// ?:, and this runs once per lobe per light, so the other kinds would pay for
// the curve on every one. khFrNK is per material, so the branch is coherent.
// KH_PBR_PI: every light colour here is pi-folded - the colour C a white Lambert surface facing it returns
// (diffuse = albedo * C * N.L, i.e. the BRDF albedo / pi under an irradiance of pi * C * N.L). KhGGXSpec is the
// specular BRDF itself (D G F / (4 N.L N.V), D carrying its 1 / pi), so a punctual light's lobe is
// KhGGXSpec * pi * C * N.L - the same irradiance the diffuse takes. The split-sum ambient (KhPbrAmbient) reads
// radiance through the environment BRDF and takes no pi.
static const float KH_PBR_PI = 3.14159265f;
float3 KhGGXSpec(float3 n, float3 v, float3 l, float rough, float3 F0, out float3 outF)
{
    float3 h = normalize(l + v);
    float ndl = saturate(dot(n, l));
    float ndv = max(dot(n, v), 1.0e-4f);
    float ndh = saturate(dot(n, h));
    float vdh = saturate(dot(v, h));
    float aa = rough * rough;
    float a2 = aa * aa;
    float dd = ndh * ndh * (a2 - 1.0f) + 1.0f;
    float D = a2 / max(3.14159265f * dd * dd, 1.0e-6f);
    float kk = (rough + 1.0f) * (rough + 1.0f) * 0.125f;
    float gl = max(ndl, 1.0e-4f);
    float G = (ndv / (ndv * (1.0f - kk) + kk)) * (gl / (gl * (1.0f - kk) + kk));
    [branch] if (khFrNK.z >= 0.5f) {
        outF = F0 * KhFresnelNK(vdh, khFrNK.x, khFrNK.y);
    } else {
        outF = F0 + (1.0f - F0) * pow(1.0f - vdh, 5.0f);
    }
    return D * G * outF / max(4.0f * ndv * gl, 1.0e-4f);
}

// Engine dynamic lights on the PBR path: modes 1 camera-relative world / 2 view
// space / 3 absolute, offset + a0/a1/a2 attenuation on the scaled distance, the
// spot cone log/mul/exp pow with its degenerate-exponent guard, and the hard
// range fade.
float3 KhDynLightsPBR(float3 wpos, float3 nrm, float3 albedo, float3 F0, float rough, float metal)
{
    if (dlCtl.x < 0.5f) return float3(0.0f, 0.0f, 0.0f);
    int pointN = (int)dlCtl.y;
    // Every light that reaches this mesh (KH_DL_RING: no count cap).
    int totalN = pointN + (int)dlCtl.z;
    float3 n = normalize(nrm);
    float3 p;
    float specOn = 1.0f;
    float3 v = float3(0.0f, 0.0f, 1.0f);

    if (dlCtl.x >= 2.5f) {
        // Mode 3: absolute world positions (engine axes); the view vector alone
        // consumes fxParams0 (zeroed camera = diffuse-only).
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
    float3 acc = float3(0.0f, 0.0f, 0.0f);
    const float khs_fwp = length(fwidth(wpos));   // Footprint priced outside the loop.

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
        // KH_DL_ATT_SKIP: twin of the DynLights and KhDlsWorldFactor guard.
        if (att <= 0.0f) continue;

        // Twin of the DynLights site. Folding the term into diffI shadows the
        // specular lobe with it (KhGGXSpec is scaled by diffI * pi, KH_PBR_PI): a highlight
        // from a blocked light goes with the light. The per-light ambient stays
        // outside.
        const float khs_sh = khObjNoRecv >= 0.5f ? 1.0f   // KH_SHADOW_SWITCH, as DynLights.
                           : KhDlsShadow((int)KhDlRec(b + 5).z - 1, wpos, nrm, 0.0f, khs_fwp);
        // Twin of the DynLights site.
        const float khs_amb = lerp(KH_DLS_AMB_KEEP, 1.0f, khs_sh);
        float ndl = max(dot(n, L), 0.0f);
        float3 diffI = dlGlobal.xyz * KhDlRec(b + 2).xyz * ndl * khs_sh;
        // The diffuse keeps what the lobe does not reflect. Only the arma model
        // takes 1 - F here, as KhApplyPBR's sun does: it has no metal lane, so
        // F alone can take a conductor's diffuse (fresnel(1.3, 7) reflects ~90 %
        // head-on). pbr keeps kdM alone - its metals are zeroed by it, and its
        // dielectrics' ~4 % is the received look. Without a view (mode 3,
        // zeroed camera) there is no lobe and no F, and the diffuse stays whole.
        float3 khsKd = kdM;
        float3 khsSpec = float3(0.0f, 0.0f, 0.0f);

        // specOn is the mode verdict (uniform); ndl is per pixel and per light.
        // KH_DL_NDL_SKIP: ndl is max(dot, 0), a HARD zero on a face turned away
        // from this light, so diffI is an exact zero too and BOTH terms the lobe
        // feeds vanish whatever F comes back - khsSpec is GGX * diffI, and the
        // diffuse is diffI * khsKd. This is KhApplyPBR's khsd_lit rule
        // (KH_SUN_SKIP_DARK) on the per-light side, and it carries the conductor
        // Fresnel's three square roots and the D/G/divide with it. The per-light
        // ambient below does NOT take the gate: it is not scaled by ndl.
        if (specOn >= 0.5f && ndl > 0.0f) {
            float3 khsF;
            khsSpec = KhGGXSpec(n, v, L, rough, F0, khsF) * (diffI * KH_PBR_PI);   // KH_PBR_PI.
            if (khFrNK.z >= 0.5f) khsKd *= saturate(1.0f - khsF);   // The arma tint may exceed 1.
        }

        acc += (albedo * (diffI * khsKd + KhDlRec(b + 3).xyz * khs_amb) + khsSpec) * att;
    }

    return acc * dlGlobal.w;
}

// The split-sum ambient the direct term implies: ambient specular from the
// dome, (1 - metal) on the diffuse - a metal has no diffuse albedo, so without
// this it goes flat grey-brown the moment the direct term dies (shadow,
// overcast, dusk). There is no reflection target; the dome is analytic.
static const float KH_PBR_GROUND = 0.35f;
float3 KhPbrAmbient(float3 khpa_n, float3 khpa_v, bool khpa_vOk, float khpa_rough,
                    float3 khpa_F0, float khpa_metal, float3 khpa_albedo,
                    float3 khpa_amb, float khpa_occ)
{
    const float3 khpa_ambo = khpa_amb * khpa_occ;
    if (!khpa_vOk) return khpa_albedo * khpa_ambo;

    const float khpa_ndv = saturate(dot(khpa_n, khpa_v));
    // Karis env BRDF (F0 * A + B).
    const float4 khpa_c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
    const float4 khpa_c1 = float4(1.0f, 0.0425f, 1.04f, -0.04f);
    const float4 khpa_r = khpa_rough * khpa_c0 + khpa_c1;
    const float  khpa_a004 = min(khpa_r.x * khpa_r.x, exp2(-9.28f * khpa_ndv)) * khpa_r.x + khpa_r.y;
    const float2 khpa_AB = float2(-1.04f, 1.04f) * khpa_a004 + khpa_r.zw;
    // Armed (khFrNK), F0 is the arma tint: the conductor curve's head-on value
    // takes Schlick's F0 in the fit, and the tint scales the whole of it, the
    // grazing part included - the tint x curve form of the direct term.
    const bool   khpa_nk = khFrNK.z >= 0.5f;
    const float  khpa_f0 = khpa_nk ? KhFresnelNK(1.0f, khFrNK.x, khFrNK.y) : 0.0f;
    const float3 khpa_envBRDF = khpa_nk ? khpa_F0 * (khpa_f0 * khpa_AB.x + khpa_AB.y)
                                        : khpa_F0 * khpa_AB.x + khpa_AB.y;

    // The dome along the reflection vector, blurred toward its mean.
    const float3 khpa_R = reflect(-khpa_v, khpa_n);
    const float  khpa_sky = smoothstep(-0.15f, 0.15f, khpa_R.y);
    const float3 khpa_dome = khpa_amb * lerp(KH_PBR_GROUND, 1.0f, khpa_sky);
    const float3 khpa_mean = khpa_amb * (0.5f * (1.0f + KH_PBR_GROUND));
    const float3 khpa_env = lerp(khpa_dome, khpa_mean, khpa_rough * khpa_rough);

    // Roughness-aware Schlick at N.V: what the surface reflects of the sky is
    // what the diffuse does not get.
    const float3 khpa_Fr = max(float3(1.0f - khpa_rough, 1.0f - khpa_rough, 1.0f - khpa_rough), khpa_F0);
    float3 khpa_Famb = khpa_F0 + (khpa_Fr - khpa_F0) * pow(1.0f - khpa_ndv, 5.0f);
    if (khpa_nk) {
        // The same form on the conductor curve: its own rise from head-on
        // toward grazing, normalised to 0..1, stands in for Schlick's
        // (1 - ndv)^5, and roughness caps where it may reach exactly as
        // max(1 - rough, F0) does above.
        const float khpa_rise = saturate((KhFresnelNK(khpa_ndv, khFrNK.x, khFrNK.y) - khpa_f0) /
                                         max(1.0f - khpa_f0, 1.0e-4f));
        const float khpa_top = max(1.0f - khpa_rough, khpa_f0);
        khpa_Famb = khpa_F0 * (khpa_f0 + (khpa_top - khpa_f0) * khpa_rise);
    }

    const float3 khpa_diff = khpa_albedo * khpa_ambo * (1.0f - khpa_metal) * saturate(1.0f - khpa_Famb);
    const float3 khpa_spec = khpa_env * khpa_envBRDF * khpa_occ;
    return khpa_diff + khpa_spec;
}

    // Compact GGX (Cook-Torrance specular + Lambert diffuse).
float3 KhApplyPBR(KhMatSurf m, float3 wpos, float3 n, float smf)
{
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return m.albedo * m.occ + m.emissive;
    // The arma model: spec-gloss with its own tint and the conductor Fresnel,
    // armed for every GGX lobe and the ambient below (khFrNK).
    const bool khar_nk = m.model >= 0.5f;
    khFrNK = khar_nk ? float4(max(m.fresnelNK, float2(0.0f, 0.0f)), 1.0f, 0.0f) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    const bool khar_sg = m.specOn >= 0.5f || khar_nk;   // Spec-gloss: rough from gloss, no metal.
    float rough = khar_sg ? saturate(1.0f - m.gloss) : saturate(m.rough);
    rough = max(rough, 0.045f);
    float3 F0 = khar_nk ? max(m.specTint, float3(0.0f, 0.0f, 0.0f))
              : m.specOn >= 0.5f ? m.specF0
              : lerp(float3(0.04f, 0.04f, 0.04f), m.albedo, saturate(m.metal));
    float metal = khar_sg ? 0.0f : saturate(m.metal);
    float3 l = lighting1.xyz;
    float3 v = normalize(fxParams0.xyz - wpos);
    float ndl = saturate(dot(n, l));
    // Both sun terms (direct here, the overcast lobe below) carry ndl * smf;
    // when it is 0 the GGX lobes that feed only them are not evaluated. Exact.
    const bool khsd_lit = ndl * smf > 0.0f;
    float3 direct = float3(0.0f, 0.0f, 0.0f);
    if (khsd_lit) {
        float3 F;
        float3 spec = KhGGXSpec(n, v, l, rough, F0, F);
        float3 kd = saturate(1.0f - F) * (1.0f - metal);   // The arma tint may exceed 1.
        direct = lighting2.rgb * (khObjDif * ndl * smf) * (kd * m.albedo + spec * KH_PBR_PI);   // KH_PBR_PI.
    }
    // The sun through cloud is a wide highlight: under overcast the sun colour
    // falls to a fraction of the ambient and the GGX lobe above is gone, yet
    // the bright patch of cloud around the sun still puts one broad, soft
    // highlight on a metal. This is that patch: the same GGX toward lighting1,
    // roughness floored (an area light), radiance the ambient's, weighted by
    // how dim the sun is against the ambient.
    const bool  khov_vOk = dot(fxParams0.xyz, fxParams0.xyz) >= 1.0f;
    const float khov_sun = max(lighting2.r, max(lighting2.g, lighting2.b));
    const float khov_amb = max(lightAmb.r, max(lightAmb.g, lightAmb.b));
    const float khov_w = (lightAmb.w >= 0.5f && khov_vOk)
                       ? saturate(1.0f - khov_sun / max(khov_amb, 1.0e-4f)) : 0.0f;
    float3 khov = float3(0.0f, 0.0f, 0.0f);
    if (khsd_lit && khov_w > 0.0f) {   // KH_SUN_SKIP_DARK.
        float3 khov_F;
        khov = KhGGXSpec(n, v, l, max(rough, 0.6f), F0, khov_F)
             * (lightAmb.rgb * khObjAmb) * (khov_w * ndl * smf * KH_PBR_PI);   // KH_PBR_PI: a punctual lobe too.
    }
    // The split-sum ambient replaces albedo * amb. smf is not passed - the sky
    // is not shadowed by our casters. The material's own occlusion map scales
    // the ambient alone; the screen-space term (KH_SSAO) is applied after the
    // draw, at the pixel.
    const float3 amb = KhPbrAmbient(n, v, khov_vOk, rough, F0, metal, m.albedo,
                                    lightAmb.rgb * khObjAmb, m.occ);
    return amb + KhDynLightsPBR(wpos, n, m.albedo, F0, rough, metal) + direct + khov + m.emissive;
}
#endif

// KH_DLS_WORLD: the world receives our meshes' dynamic-light shadows. Returns a
// per-channel MULTIPLY FACTOR for an already-shaded world pixel (1 = leave
// alone, < 1 = this much of its light was blocked by one of our meshes); the
// caller emits it under a dest*src blend at the scene resolve. Why a factor:
// the engine's lit shaders are forward and end with out = albedo *
// (skyAmb*shadowMask + sun*NdL + Ldyn), and there is no albedo target to read;
// in ratio form out' = out * (1 - Lblocked/Ltotal) the albedo divides out. THE
// NUMERATOR IS EXACT: the per-light loop below is the engine's own dynamic
// light function instruction for instruction (cb11 stride 6, the same offset
// table, the same 1e-4 guards, the same log/mul/exp cone pow, ambient
// accumulated without N.L). This is the third copy of that loop in this file
// (beside DynLights and KhDynLightsPBR, plus kh_dls_atten in the C++); they
// differ in what they ACCUMULATE, not in how they attenuate, so any edit to the
// attenuation or the cone in one is an edit to all four. Offsets: [b+0]
// position, [b+1] spot axis + cone threshold, [b+2] diffuse + cone scale, [b+3]
// ambient + cone exponent, [b+4] offset + (a0,a1,a2), [b+5] fade start +
// inverse width + OUR SHADOW SLOT. THE DENOMINATOR IS GENEROUS: Ltotal adds the
// engine's sky ambient and sun with the sun's own shadow assumed fully lit, so
// every error fails toward UNDER-darkening; in daylight the factor correctly
// goes to 1. EVERY FAILURE PATH RETURNS 1.0. khw_nrel is the caller's
// confidence in khw_nrm, 0..1 (a mesh passes 1; the world pass passes the
// agreement between two baselines of its depth-derived plane, which collapses
// to the view axis inside quantisation plateaus). Where the normal is not
// resolved N.L falls back to 1 - the facing prior - on BOTH the numerator's
// diffuse and the denominator's sun. khw_fwp: length(fwidth(khw_wpos)),
// priced by the caller before any per-pixel exit (KH_DLSW_GRAD) - it reaches
// here past several.
float3 KhDlsWorldFactor(float3 khw_wpos, float3 khw_nrm, float khw_zunc,
                        float khw_nrel, float khw_fwp)
{
    if (dlCtl.x < 0.5f) return float3(1.0f, 1.0f, 1.0f);
    // Mode 3 only: modes 1/2 decode light positions relative to the camera
    // through lanes this pass does not fill.
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

        // The slot lane, written by kh_dls_fill_cb's twin in the C++. 0 = this
        // light casts no shadow (the zeroed default).
        const int khw_slot = (int)KhDlRec(khw_b + 5).z - 1;
        if (khw_slot < 0) continue;
        const float khw_sh = KhDlsShadow(khw_slot, khw_wpos, khw_n, khw_zunc, khw_fwp);

        if (khw_sh >= 1.0f) continue;   // Fully lit: nothing blocked; skip the arithmetic.
        // Exactly the shading DynLights would have produced with the shadow
        // applied, including the partial ambient (KH_DLS_AMB_KEEP). The
        // difference is the blocked share.
        const float3 khw_shaded = (khw_diff * khw_sh
                                 + khw_amb * lerp(KH_DLS_AMB_KEEP, 1.0f, khw_sh)) * khw_att;
        khw_blocked += max((khw_diff + khw_amb) * khw_att - khw_shaded, 0.0f);
    }

    // DynLights' trailing global scale on both sides, so the ratio is
    // scale-invariant.
    khw_dyn *= dlGlobal.w;
    khw_blocked *= dlGlobal.w;
    if (dot(khw_blocked, float3(1.0f, 1.0f, 1.0f)) <= 0.0f) return float3(1.0f, 1.0f, 1.0f);

    // Ltotal: the engine's own combine, sun shadow assumed lit (see above).
    const float3 khw_sky = lightAmb.rgb * lighting0.z;
    // Same N.L weighting as the numerator: at khw_nrel 0 this reads full sun,
    // which overstates Ltotal and fails toward under-darkening.
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
    // Anchor-relative position, metres-scale through the interpolators
    // (world-absolute wpos at ~7.5 km rounds at +-0.49 mm per frame under
    // camera motion - half a hero texel). The self chain samples with this;
    // wpos stays for every absolute consumer.
    float3 wrel : TEXCOORD4;
    // The object colour rides the interpolators so the pixel twins read one
    // source whether the draw was per object (the VS copies the CB lane) or a
    // batch (the instance lane). Flat across a triangle by construction.
    float4 icol : TEXCOORD5;
    // The per-object lanes (KhObjLoad at every mesh PS entry). Flat per draw or
    // per instance.
    nointerpolation float4 iobj0 : TEXCOORD7;   // amb, dif, creation (KH_USER_LANES), cut.
    nointerpolation float4 iobj1 : TEXCOORD8;   // no-receive (KH_SHADOW_SWITCH), dither, 0, 0.
#if KH_TEXTURED
    float2 uv : TEXCOORD2; float4 tanw : TEXCOORD3;   // World tangent + handedness.
    nointerpolation uint matIx : TEXCOORD6;   // KH_MAT_TABLE: this draw's / instance's entry.
#endif
};

// The one vertex transform. The per-object lanes (centre, rebase-relative
// centre + arm, edge lengths, rotation rows) come from the CB on a per-object
// draw and from the instance stream on a batch draw; everything below the lane
// read is identical, so the four entry points (VSMain / VSMainInst, VSComposite
// / VSCompositeInst) are wrappers that only choose the source. VSInst = the
// 16-byte lane (C++ twin KhInstLane): record slot, LOD dither, final colour
// alpha, material table base.
struct VSInst {
    uint   islot : TEXCOORD4;
    float3 ilane : TEXCOORD5;    // x = dither, y = alpha, z = material base.
};

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
    // FP32 jitter rebase (see centerRel): when armed, the camera-relative
    // position goes through the rebased viewProj and the world-absolute fp32
    // cancellation never enters the position path.
    float3 khvTp = (khvc_relArm > 0.5f)
                 ? (khvc_rel + KhRotateR(khvc_lp * khvc_size, khvc_r0, khvc_r1, khvc_r2))
                 : wp;
    // stenVol2.z selects the vertex path: 1 = the absolute position through the
    // engine VP, 2 = the rebased one, 3 = the seam prepass (the engine-view z
    // remap). Only 3 is written today; 1/2 are arms, not the default, because a
    // camera-relative engine cb2 would displace an absolute wp by the whole
    // camera vector.
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
    // The pop at max view distance is enforced per fragment in the
    // PS (far contract block) instead of here.
    khvc_owpos = wp;
    // KH_SELF_REL_INTERP: the position against the sun anchor, which the sun
    // matrices subtract. KH_POS_RES: armed, it is built from the camera-
    // relative position (the residual already in it) and the pass camera's
    // offset from the anchor - both small, so wp's world-scale rounding never
    // reaches the self-shadow lookup (VSSunDepth builds its casters the same
    // way). Every armed pass that reads it - the colour passes: injection,
    // flush, PIP, view-model slice - fills khPass with the camera it rebases
    // on; the volume and prime passes arm without reading it.
    khvc_owrel = (khvc_relArm > 0.5f) ? (khvTp + (khPass.xyz - sunOrigin.xyz)) : (wp - sunOrigin.xyz);
    // Per-axis scale is non-uniform: normals take the inverse scale, then the
    // object rotation (the inverse-transpose of scale-then-rotate for
    // orthonormal R - see kh_set_rotation).
    khvc_onrm = normalize(KhRotateR(khvc_ln / max(khvc_size, float3(1e-4f, 1e-4f, 1e-4f)), khvc_r0, khvc_r1, khvc_r2));
}

// The effect unit's depthTex owns t1. KH_RECEIVE_TEX is passed by the white,
// static and composite compiles - kh_white_ensure, ensure_resources'
// static and composite tables, ensure_composite_shader, kh_user_mat_ps's
// three material twins and kh_user_mat_vs's vertex stage - and by nothing
// else. EVERY compile of the effect unit (ensure_resources' prewarm pair,
// ensure_effect_shader, kh_pip_fx_shader) and kh_user_fx_ps's cb + user
// post-FX pass MSAA_DEPTH and KH_FX_UNIT, never KH_RECEIVE_TEX; that is what
// keeps shadowAtlas off depthTex at t1. white.hlsl
// declares no register of its own, so the white unit takes the block below
// without colliding with anything.
#ifdef KH_RECEIVE_TEX
// The engine's shadow atlas (the depth its cascade passes render into), sampled
// with the engine's own per-cascade world->atlasUV+depth transforms harvested
// from its constant uploads - the same compare the engine performs for its own
// geometry.
Texture2D<float> shadowAtlas : register(t1);
Texture2D<float> shadowBand0 : register(t4);
Texture2D<float> shadowBand1 : register(t5);
Texture2D<float> shadowBand2 : register(t6);
Texture2D<float> shadowBand3 : register(t7);
Texture2D<float> shadowBand4 : register(t8);
Texture2D<float> shadowBand5 : register(t9);
Texture2D<float> shadowBand6 : register(t12);   // Slots 6-7 (t10 is the terrain heightfield).
Texture2D<float> shadowBand7 : register(t13);

// Takes ABSOLUTE world positions: the engine renders camera-relative and its
// sampling transforms consume that space, so fill_lighting_frame_cb folds each
// entry's own camera origin into its translation (t' = t - M.cam) before the
// table lands here. A camera-relative position would be folded twice.
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
        // Tiles are sorted finest-first; the first cascade whose tile and depth
        // range contain the point decides.
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
    // KH_SHADOW_STRENGTH_SKIP: lighting2.w is g_shadow_map_strength, a CB
    // scalar, so this is a UNIFORM branch. It is not a rare case: BOTH PIP
    // fills (kh_pip_fx's and the PIP mesh injection's) zero lighting2[3] on
    // purpose, because shadowAtlas is the MAIN view's and is not bound in a PIP
    // pass - so without this gate every PIP mesh pixel walked the cascade table
    // and took a Gather from an UNBOUND t1 (which reads fully occluded) only
    // to multiply the answer by zero. Returning here skips the walk and the
    // unbound read alike; the result is bit-identical either way.
    if (saturate(lighting2.w) <= 0.0f) return 1.0f;
    int cascade;
    float occluded;
    ShadowMapSample(rel, cascade, occluded);
    return 1.0f - occluded * saturate(lighting2.w);
}

// Bilinear compare per tap (the engine's sample_c equivalent): the 2x2 weighted
// compare resolves every dither cell of the atlas's dithered foliage to its
// coverage fraction. KhBandTap4 is the compare against ONE named band texture;
// the texture selection lives in the single branch below.
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

// The selection is a BRANCH, not a select: fxc flattens `?:` over the eight
// band textures into all eight fetches per tap (it cannot index a texture
// object; measured as eight Load instructions when the taps were Loads - now
// each is a Gather). [branch] on a value uniform across the band costs one
// taken arm.
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
    // KH_SHADOW_STRENGTH_SKIP: the ShadowMapFactor gate, on the band path. The
    // tail multiplies by the same saturate(lighting2.w), so at strength 0 this
    // returns an exact 1 after 4 to 16 Gathers (one per tap) through the
    // eight-arm band select. UNIFORM, so the loops below keep whole-warp flow.
    if (saturate(lighting2.w) <= 0.0f) return 1.0f;
    // No [unroll] on these loops (X4575 in this unit); the 'done' flag plus a
    // plain break is the accepted early-out shape - no continue. Slots arrive
    // finest-first; the first containing band wins; bandBorder.w-1 names the
    // physical texture.
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
                        // The 12 refinement taps below keep the rotation where
                        // it belongs.
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
    // Fade at the shadow view distance (without it the shadow snaps into
    // existence at the cascade's far edge under motion).
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

// Engine distance haze (PSC_HazePars, sky CB row 14): the blue distance wash
// world geometry carries.
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
// KH_FX_UNIT - the effect unit's own declarations and helpers. Compiled into
// the builtin effect shader (effect.hlsl, effect2.hlsl and effect3.hlsl follow
// this file) and into every user effect shader (the USER SHADER CONTRACT's
// EFFECT section) - one text for both, so a
// user effect reads the scene, the depth, the pyramid and the fog as the
// builtins do, and can begin and finish its pixel with the builtin's own lines
// (KhFxBegin / KhFxFinish, below). Fenced: the mesh units declare other
// resources at t0 - t3 (static.hlsl, composite.hlsl).
// ===========================================================================
#if KH_FX_UNIT
Texture2D<float4> sceneColor : register(t0);
Texture2D<float4> khsgTex : register(t3);
// s2, not s1: khPfSamp holds s1 (declared earlier in this file), so a second s1
// fails the compile with X4509 the moment any entry reaches a helper that uses it
// (the shadow compares' Gather, KH_SHADOW_GATHER). C++ twin (KH_FX_SAMP_S2).
// Bound for a user effect only when its pyramid is (KhGlowBind).
// Bound only for the draws that read it, the prior binding put back: the SSGI resolve's linear clamp, or a glow
// pass's anisotropic clamp (KH_GLOW_PYR, C++ KhGlowBind).
SamplerState khsgSamp : register(s2);

#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t1);
float LoadDepthRaw(int2 px) { return depthTex.Load(px, 0); }
#else
// Two-plane snapshot; declaration only.
Texture2D<float2> depthTex : register(t1);
float LoadDepthRaw(int2 px) { return depthTex.Load(int3(px, 0)).x; }
#endif
// KH_NEARZ_MARK: our near-z fragments nearer than the near plane are written
// into a gap below the viewport's MinDepth that this pair reads as a flat wall
// at the near distance. The marker (t37, armed by shadowMeta2.x) holds (raw,
// true distance) where a drawer of ours wrote one this cycle; where its raw is
// still the live raw, that distance comes back re-encoded through this pass's
// own pair, so LinDepth (and every reader of this load) gets the true one. Any
// other pixel - the hands drawn over since, the world - loads as it is.
Texture2D<float2> khNzMark : register(t37);
float LoadDepthPS(int2 px)
{
    const float khnz_r = LoadDepthRaw(px);
    if (shadowMeta2.x > 0.5f) {
        const float2 khnz_m = khNzMark.Load(int3(px, 0));
        if (khnz_m.x > 0.0f && khnz_m.x == khnz_r && khnz_m.y > 0.0f) {
            return depthParams.z + (depthParams.w - depthParams.z) * (depthParams.x + depthParams.y / khnz_m.y);
        }
    }
    return khnz_r;
}

Texture2D<float> khArbSnap : register(t2);

float3 SampleScene(int2 px)
{
    px = clamp(px, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    float4 khss = sceneColor.Load(int3(px, 0));
    if (centerSize.w > 2.5f) return khss.rgb * khss.a;
    return khss.rgb;
}

// Meaningless in the scene phase - callers gate on the flag.
float KhUiCov(int2 px)
{
    px = clamp(px, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    return sceneColor.Load(int3(px, 0)).a;
}

float Luma(float3 c) { return dot(c, float3(0.2126f, 0.7152f, 0.0722f)); }   // BT.709.

// KH_FX_PX_REF: every size a builtin effect gives in pixels - its "...Px" parameters and the fixed pixel
// spans inside the effects below - is in pixels of a 1080-row frame; this is the factor to the pass's own
// frame (fxMeta.w, the height of the picture it draws), so a look holds at any resolution. Exactly 1 at 1080
// rows, and every use multiplies it onto the finished pixel quantity, so at 1080 rows each quantity is the
// unscaled one (up to fxc reordering a product - it is not IEEE-strict - a rounding step on an offset at
// most). What stays in real pixels: sharpen's 1-px neighbourhood (sharpening is of the display's pixels), anti-aliasing
// widths (the CRT tube's feather), per-pixel dither and sampling noise (deband's grain, the interleaved
// rotations), and the SSGI chain (its radius is in metres; its resolve works on the gather's own grid).
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
// taps, exactly as before. (Anamorphic reads a pyramid of its own, halved along its streak alone: KH_ANA_PYR.)
bool KhGlowOn() { return fuseMeta.y > 0.5f; }

// Each footprint is taken a quarter past the tap spacing: bilinear reads of a level are not shift-invariant, so at
// the bare spacing a lone bright pixel under the 5 x 5 bloom still ripples by up to ~18% of its peak; at 1.25 by
// about a tenth, the glow's spread then ~15% past the direct taps' (~10% at the bare spacing). H38 models it.
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

// The pyramid's share of taps sp pixels apart: none at a pixel or less (adjacent pixels leave no gap - the direct
// taps, exactly as before), all from two, linear between. KhGlowSel takes the direct result d unless the share
// is positive.
float KhGlowMix(float sp) { return KhGlowOn() ? saturate(sp - 1.0f) : 0.0f; }
float3 KhGlowSel(float3 d, float3 p, float m) { return m <= 0.0f ? d : (m >= 1.0f ? p : lerp(d, p, m)); }
float KhGlowSel(float d, float p, float m) { return m <= 0.0f ? d : (m >= 1.0f ? p : lerp(d, p, m)); }

float LinDepth(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    // At/beyond the far plane the denominator crosses zero and flips sign; sky
    // pixels sit at the depth-clear value (1.0), beyond the viewport range
    // entirely - treat everything past the far plane as "very far".
    if (denom > -1e-7f) return 1e9f;
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
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

// Per-pixel world position from the depth buffer + inverse view-projection.
float KhEncFence()
{
    float khef_den = 1.0f - depthParams.x;
    float khef_far = khef_den < -1.0e-7f ? depthParams.y / khef_den : 20000.0f;
    return clamp(khef_far, 500.0f, 100000.0f);
}

float3 KhWorldPosFenced(int2 px, float2 uv, out float khwf_d)
{
    float khwf_raw = LinDepth(LoadDepthPS(px));
    khwf_d = min(khwf_raw, KhEncFence() * 0.999f);
    float4 khwf_nd = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f,
                            depthParams.x + depthParams.y / max(khwf_d, 1.0f), 1.0f);
    float4 khwf_wp = mul(khwf_nd, invViewProj);
    // With fxCam armed the inverse is of a rotation-only view (camera at the
    // origin) - small numbers, exact in fp32 - and the camera is added here,
    // once. An absolute inverse mixes the camera's kilometres into every entry
    // and cancels them per pixel, which jitters the reconstruction by
    // centimetres frame to frame.
    return khwf_wp.xyz / khwf_wp.w + (fxCam.w > 0.5f ? fxCam.xyz : float3(0.0f, 0.0f, 0.0f));
}

// Pure function of its arguments; reads no CB.
float3 KhgVpos(float2 vp_px, float vp_d, float2 vp_res, float vp_m00, float vp_m11)
{
    float2 vp_uv = (vp_px + 0.5f) / vp_res;
    return float3((vp_uv.x * 2.0f - 1.0f) * vp_d / vp_m00,
                  (1.0f - vp_uv.y * 2.0f) * vp_d / vp_m11,
                  vp_d);
}

// KH_FX_USER: the engine's own fog at a pixel - the share of its light the
// engine's fog replaces (0 none - 1 all), from the view distance fs_d (LinDepth),
// the frame fs_res (fxMeta.zw) and the projection's two scales fs_m00 / fs_m11
// (the lengths of viewProj's first and second columns over its xyz rows, as
// fog scatter forms them). Fog scatter's engine term (KhFsFog adds the KH fog
// passes to it) - one body.
float KhFxFogEngine(float2 fs_px, float fs_d, float2 fs_res, float fs_m00, float fs_m11)
{
    float fs_s = 0.0f;

    if (fogParams.w >= 0.5f && depthParams.y < -1.0e-3f)
    {
        // The encode pair's far fence (m32 gate above guarantees the standard-z
        // shape: 1 - m22 < 0, m32 < 0 -> positive fence).
        float fs_fden = 1.0f - depthParams.x;
        float fs_far = fs_fden < -1.0e-7f ? depthParams.y / fs_fden : 20000.0f;
        float fs_de = min(fs_d, clamp(fs_far, 500.0f, 100000.0f) * 0.999f);
        float fs_distM = length(KhgVpos(fs_px, fs_de, fs_res, fs_m00, fs_m11));
        // Height at the clamped distance: the analytic ndcZ of fs_de, immune to
        // the beyond-far mirror and to raw-depth teeter by construction.
        float2 fs_uv = (fs_px + 0.5f) / fs_res;
        float4 fs_nd = float4(fs_uv.x * 2.0f - 1.0f, 1.0f - fs_uv.y * 2.0f,
                              depthParams.x + depthParams.y / max(fs_de, 1.0f), 1.0f);
        float4 fs_wp = mul(fs_nd, invViewProj);
        // With fxCam armed the inverse is camera-relative and the height comes
        // back relative too; fogColor.w is the camera's absolute altitude, so
        // the camera's own is added here, as the position is.
        float fs_hgt = fs_wp.y / fs_wp.w + (fxCam.w > 0.5f ? fxCam.y : 0.0f);
        float fs_camY = fogColor.w;
        float fs_tr;

        if (fogEngine.w >= 0.5f)
        {
            float fs_ramp = fogEngine.w >= 1.5f
                          ? 1.0f
                          : saturate((fogEngine.y - fs_distM) * fogEngine.z);
            float fs_dh = abs(fs_hgt - fs_camY);
            float fs_k = fogParams.y * fs_dh / max(fs_distM, 1.0e-4f);
            float fs_integ = fs_k < 1.0e-6f ? fs_distM : (1.0f - exp(-fs_distM * fs_k)) / fs_k;
            float fs_minY = min(fs_hgt, fs_camY);
            fs_tr = fs_ramp * exp(-fs_integ * fogEngine.x * exp(-fogParams.y * max(fs_minY, 0.0f)));
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

// Point-op pass fusion. CPU twin: the chain loops' pending append
// (kh_fuse_append) enforces the fusible set {1 invert, 2 colorgrade, 3
// vignette, 5 grain}, never localized / banded / spill / LUT / custom, so this
// path carries no masks.
// Film grain's noise (effect 5 and KhFusePoint's id 5 - one body): smooth value noise over grainSizePx cells,
// triangular-ish and signed, optional chroma. p0 = [amount, fps, grainSizePx, lumaResponse], chroma = the chroma
// lane. KH_HASH: the frame (time quantized to fps) is the hash's integer lane, the cells' own coordinates the
// other two, so no frame count or position reaches a float sum.
float3 KhGrainGc(float2 pos, float t, float4 p0, float chroma)
{
    const float fps = max(p0.y, 1.0f);
    const uint f = (uint)floor(t * fps);
    const float2 gp = pos / (max(p0.z, 1.0f) * KhFxPx());   // KH_FX_PX_REF.
    const float2 ip = floor(gp);
    float2 fp = frac(gp);
    fp = fp * fp * (3.0f - 2.0f * fp);
    const float n00 = KhHashF(ip, f);
    const float n10 = KhHashF(ip + float2(1, 0), f);
    const float n01 = KhHashF(ip + float2(0, 1), f);
    const float n11 = KhHashF(ip + float2(1, 1), f);
    const float nv = lerp(lerp(n00, n10, fp.x), lerp(n01, n11, fp.x), fp.y);
    const float nf = KhHashF(gp * 2.13f + 17.0f, f);
    const float g = (nv + nf) * 0.5f - 0.5f;   // Triangular-ish, signed.
    float3 gc = g.xxx;
    if (chroma > 0.001f)
    {
        const float gr = (lerp(KhHashF(ip + 31.0f, f), KhHashF(ip + float2(1, 1) + 31.0f, f), fp.x) + KhHashF(gp * 1.71f + 47.0f, f)) * 0.5f - 0.5f;
        const float gb = (lerp(KhHashF(ip + 73.0f, f), KhHashF(ip + float2(1, 1) + 73.0f, f), fp.x) + KhHashF(gp * 2.71f + 89.0f, f)) * 0.5f - 0.5f;
        gc = lerp(gc, float3(gr, g, gb), chroma);
    }
    return gc;
}

float3 KhFusePoint(int id, float3 c, float2 uv, float2 pos, float t,
                   float4 p0, float4 p1, float4 col)
{
    if (id == 1) return (1.0f - saturate(c)) * col.rgb;
    if (id == 2)
    {
        float3 g = c * col.rgb * p0.z;
        float l = Luma(g);
        g = lerp(l.xxx, g, p0.x);
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
        return c + gc * p0.x * resp;
    }
    return c;
}

// Fused-stage composite: the packing tail's blend algebra over the running
// value, plus - write-window lanes only - the coverage destination lerp in its
// pre-composite position.
float3 KhFuseTail(float3 v, float cov, bool uiLane, float2 uv, float2 pos, float t)
{
    int n = (int)fuseMeta.x;
    [loop] for (int s = 0; s < n; ++s)
    {
        float4 fm = fuseStage[s * 4];
        float4 fcol = fuseStage[s * 4 + 3];
        float3 c = KhFusePoint((int)fm.x, v, uv, pos, t,
                               fuseStage[s * 4 + 1], fuseStage[s * 4 + 2], fcol);
        if (uiLane) c = lerp(v, c, cov);
        float a = fcol.w;
        int bm = (int)fm.y;
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

// KH_FX_TEX - a user effect's own textures (the script's "textures", user0 ..
// user5; C++ kh_fx_tex_fill): each a layer of a texture page bound per draw at
// t43 + i (null when absent), its layer in khFxTexLay (1 + layer; 0 = absent
// or still loading), read through s0 (anisotropic, wrap - the material
// sampler). The material shaders' KH_USER_TEX functions, by the same names and
// rules: the selects are ?: chains, not flow, so a filtered sample never sits
// in divergent control flow; with a literal index fxc drops the other five.
// The builtin effects bind none of these and read none.
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

// KH_FX_USER - the builtin effect's first and last lines, one body with its
// PSEffect (which calls both) and a user effect's opt-in.
// KhFxBegin: the object's lanes (KhObjLoad), the depth gate an effect mesh
// takes against the scene pair (a fullscreen pass passes it by construction),
// the object's view-distance cut (khObjCut) and an effect mesh's far-frame
// arbitration (localParams1.z arms it: the snapshot at t2, and the terrain).
// Call it first.
void KhFxBegin(VSOut i)
{
    KhObjLoad(i.iobj0, i.iobj1);   // KH_OBJBUF: effect meshes draw per object (the CB lanes).
    // Fullscreen passes are inert by construction (w = 1 -> ndc far below 1),
    // and zeroed/degenerate depthParams stand the test down via the m32 gate.
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

// KhFxFinish: the pixel finished as the builtin finishes its own - the
// localization mask (and its inverse), the band mask, the UI lane's coverage,
// opacity (color.a) and the blend mode, the fused stages, then the packing
// its composite wants (a fullscreen pass writes the composite; an effect mesh
// returns what the hardware blend expects). scene = the pass's source at the
// pixel (SampleScene), outc = the effect's result, pos = i.pos.xy. Return it.
float4 KhFxFinish(float3 scene, float3 outc, float2 pos)
{
    const int2 px = int2(pos);
    const float2 uv = pos / float2(fxMeta.z, fxMeta.w);
    const float t = fxMeta.y;

    if (localParams1.y > 0.5f)
    {
        float khlm_d;
        float3 nd3 = abs(KhWorldPosFenced(px, uv, khlm_d) - localParams0.xyz) / max(localRadii.xyz, 0.01f);
        // Normalized distance: 1.0 = the mask surface (ellipsoid or mesh).
        float nd = (localParams0.w > 0.5f)
                 ? max(nd3.x, max(nd3.y, nd3.z))   // Cube (Chebyshev).
                 : length(nd3);   // Sphere/ellipsoid.
        float mask = 1.0f - smoothstep(1.0f, 1.0f + max(localParams1.x, 0.001f), nd);
        // localRadii.w >= 0.5 complements the mask - the effect reaches
        // everything except the volume, falloff band and sky included. C++ twin
        // local_radii[3] (addLocalPostFX 'inverse').
        if (localRadii.w >= 0.5f) mask = 1.0f - mask;
        outc = lerp(scene, outc, mask);
    }

    // Camera-distance band mask: full strength within [min, max], fading over
    // 'falloff' metres at both edges; max <= 0 = unbounded far (sky included).
    // Multiplies with the localization mask.
    if (bandParams.w > 0.5f)
    {
        float d = LinDepth(LoadDepthPS(px));
        float fall = max(bandParams.z, 0.01f);
        float mask = smoothstep(bandParams.x - fall, bandParams.x, d);
        if (bandParams.y > 0.0f)
            mask *= 1.0f - smoothstep(bandParams.y, bandParams.y + fall, d);
        outc = lerp(scene, outc, mask);
    }

    // UI-coverage destination mask (write-window masked lane, centerSize.w =
    // 2): the effect vanishes smoothly off the UI.
    if (centerSize.w > 1.5f && centerSize.w < 2.5f)
        outc = lerp(scene, outc, KhUiCov(px));   // Spill = w 3, excluded.

    int bm = (int)sizeAxes.w;
    if (centerSize.w > 0.5f)
    {
        float a = color.a;
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
            comp = KhFuseTail(comp, khuRaw.a, true, uv, pos, t);
            return float4(comp, khuRaw.a);   // Coverage passthrough.
        }

        comp = KhFuseTail(comp, 1.0f, false, uv, pos, t);   // (scene chain lane).
        return float4(comp, 1.0f);
    }

    // Blend-mode output packing (meshes: hardware blend against the live
    // framebuffer; intensity pre-applied where blend factors cannot express
    // it).
    if (bm == 1 || bm == 3) return float4(outc * color.a, 1.0f);   // Additive, screen.
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), outc, color.a), 1.0f);   // Multiply.
    if (bm == 4 || bm == 5) return float4(lerp(scene, outc, color.a), 1.0f);   // Lighten, darken (MAX/MIN op).
    return float4(outc, color.a);   // Normal (alpha lerp).
}
#endif
