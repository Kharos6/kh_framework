// skin.hlsl - KH_SKIN_GPU: a skeletal binding's vertices, skinned on the GPU. A
// STANDALONE unit (cb.hlsl is not prefixed): one vertex shader and no pixel
// stage. C++ (kh_skin_so_batch) draws a mesh's REST STREAM as a point list
// through VSSkinSo with stream output bound and rasterisation off, so every
// streamed vertex lands in the binding's vertex buffer as one MeshVertex -
// the same 48 bytes (position, normal, uv, tangent) kh_skin_verts writes on the
// CPU. Every binding streams the whole rest stream, locked or not: every LOD
// level skins (KH_LOD_SKIN; KH_SKIN_LVL0 retired). KH_SKIN_POOL: that buffer is
// the binding's slice of its mesh's pool chunk, streamed at the slice's offset
// (or a buffer of its own when the pool cannot give one). Every pass that draws
// the mesh binds it at that offset - a mesh under a vertex stage (KH_USER_VS)
// draws the stage's deformed copy of it instead - and the instanced routes read
// the same bytes through the chunk's view (the Sk twins' KhSkPull, cb.hlsl): no
// other shader skins.
//
// TWIN of kh_skin_verts (rendering_integration_features.hpp) - the same sums in the
// same order. Change one and change the other.
//
// The rest stream (C++ twin KhSkinSoVert, 80 bytes, built by
// kh_skin_so_stream): the AUTHORED position (metres), the authored normal
// (n / native_ext), the covariant tangent (t * native_ext) with its
// handedness, the uv, four bone indices and four weights. A weight the CPU
// path would skip (not above zero, or naming a bone past the skeleton) is
// stored as zero, so the loop below needs no bone-count test.
// KH_SKIN_SO_PAL: a bone index is the bone's row in the mesh's palette (C++
// KhSkinSup::pal - the bones some weight names, in bone order), not its
// skeleton index: kh_skin_so_cb_fill writes bone pal[j]'s map at row j, so the
// map is the one kh_skin_verts applies.
//
// The palette: three float4 per bone, row c = (M[0+c], M[3+c], M[6+c],
// M[9+c]) of kh_skin_affine_of's 12-float map, so v' = v * M + T is a dot and
// an add per axis. The weight left under one goes to the root (the identity).
//
// KH_SKIN_SO_BIG: KH_SKIN_SO_BONES is the palette's size in this compile - 256
// as written (C++ KH_SKIN_SO_BONES_S, the small class: CBSkin 770 float4), 1364
// when C++ compiles the large class (KH_SKIN_SO_DEFS_L; C++ KH_SKIN_SO_BONES,
// CBSkin 4094 float4, the most one cbuffer holds). C++ hands a mesh to the
// smallest class its palette fits and keeps a bigger one on the CPU. The
// clamp in the loop is unreachable (SKN-9): every stored index is below the
// palette's size, which is at most this class's count; it only keeps a read
// inside khskBone. These counts, CBSkin's ByteWidths and the refusal sites
// change as one.
//
// The box: the pose is stored in its own box, as the CPU path stores it
// (KhVsCore scales a position by size, divides a normal by it and multiplies
// a tangent by it). The box is C++'s bound of this pose (kh_skin_so_bound);
// whatever box is handed in, the drawn position is root + skinned * R.

#ifndef KH_SKIN_SO_BONES
#define KH_SKIN_SO_BONES 256   // C++ twin KH_SKIN_SO_BONES_S; the large class defines it (KH_SKIN_SO_BIG above).
#endif

cbuffer CBSkin : register(b0)
{
    float4 khskCtr;    // xyz = the box centre (metres, the authored frame); w unread.
    float4 khskSize;   // xyz = the box edge lengths (metres); w unread.
    float4 khskBone[KH_SKIN_SO_BONES * 3];
};

struct VSInSkin {
    float3 p  : POSITION;
    float3 n  : NORMAL;
    float2 uv : TEXCOORD0;
    float4 t  : TANGENT;
    uint4  b  : BLENDINDICES;
    float4 w  : BLENDWEIGHT;
};
// The stream-output declaration (kh_skin_so_ensure) names these four
// semantics in this order: MeshVertex's lanes at offsets 0 / 12 / 24 / 32.
struct VSOutSkin {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv  : TEXCOORD0;
    float4 tan : TANGENT;
};

// KH_SKIN_NRM_COF - TWIN of kh_skin_nrm_cof (rendering_integration_features.hpp), term for term: the normal by
// the blended map's inverse transpose. b0 / b1 / b2: the blended map B's rows (the root's share on the diagonal); n:
// the authored normal. Returns n . cof(B) turned by det(B)'s sign at the length of n . B, or n . B itself where the
// cofactor's squared length is outside (1e-30, 1e30) or n . B's is 1e30 or more (the select drops the other arm's
// inf / NaN).
float3 KhSkinNrmCof(float3 b0, float3 b1, float3 b2, float3 n)
{
    const float3 khsn_nn = float3(dot(n, b0), dot(n, b1), dot(n, b2));
    const float3 khsn_x0 = cross(b1, b2);
    const float3 khsn_x1 = cross(b2, b0);
    const float3 khsn_x2 = cross(b0, b1);
    const float3 khsn_c = float3(dot(n, khsn_x0), dot(n, khsn_x1), dot(n, khsn_x2));
    const float khsn_lc = dot(khsn_c, khsn_c);
    const float khsn_la = dot(khsn_nn, khsn_nn);
    const bool khsn_ok = khsn_lc > 1.0e-30f && khsn_lc < 1.0e30f && khsn_la < 1.0e30f;
    const float khsn_det = dot(b0, khsn_x0);
    const float khsn_g = ((khsn_det >= 0.0f) ? 1.0f : -1.0f) * (sqrt(khsn_la) / sqrt(khsn_lc));
    return khsn_ok ? khsn_c * khsn_g : khsn_nn;
}

VSOutSkin VSSkinSo(VSInSkin i)
{
    float3 khsk_pp = float3(0.0f, 0.0f, 0.0f);
    float3 khsk_tt = float3(0.0f, 0.0f, 0.0f);
    float3 khsk_b0 = float3(0.0f, 0.0f, 0.0f);   // KH_SKIN_NRM_COF: the blended map's rows.
    float3 khsk_b1 = float3(0.0f, 0.0f, 0.0f);
    float3 khsk_b2 = float3(0.0f, 0.0f, 0.0f);
    float  khsk_ws = 0.0f;
    [unroll] for (int k = 0; k < 4; ++k) {
        const float khsk_w = i.w[k];
        if (khsk_w > 0.0f) {
            const uint khsk_o = min(i.b[k], (uint)(KH_SKIN_SO_BONES - 1)) * 3u;
            const float4 khsk_r0 = khskBone[khsk_o];
            const float4 khsk_r1 = khskBone[khsk_o + 1u];
            const float4 khsk_r2 = khskBone[khsk_o + 2u];
            khsk_pp += khsk_w * float3(dot(i.p, khsk_r0.xyz) + khsk_r0.w,
                                       dot(i.p, khsk_r1.xyz) + khsk_r1.w,
                                       dot(i.p, khsk_r2.xyz) + khsk_r2.w);
            khsk_tt += khsk_w * float3(dot(i.t.xyz, khsk_r0.xyz), dot(i.t.xyz, khsk_r1.xyz), dot(i.t.xyz, khsk_r2.xyz));
            khsk_b0 += khsk_w * khsk_r0.xyz;   // KH_SKIN_NRM_COF.
            khsk_b1 += khsk_w * khsk_r1.xyz;
            khsk_b2 += khsk_w * khsk_r2.xyz;
            khsk_ws += khsk_w;
        }
    }
    const float khsk_rest = (khsk_ws < 1.0f) ? (1.0f - khsk_ws) : 0.0f;   // The weight left to the root.
    khsk_pp += khsk_rest * i.p;
    khsk_tt += khsk_rest * i.t.xyz;
    khsk_b0.x += khsk_rest;   // KH_SKIN_NRM_COF: the root's share, the identity.
    khsk_b1.y += khsk_rest;
    khsk_b2.z += khsk_rest;
    const float3 khsk_nn = KhSkinNrmCof(khsk_b0, khsk_b1, khsk_b2, i.n);
    VSOutSkin o;
    o.pos = (khsk_pp - khskCtr.xyz) / khskSize.xyz;
    o.nrm = khsk_nn * khskSize.xyz;
    o.tan = float4(khsk_tt / khskSize.xyz, i.t.w);   // The chart's handedness does not deform.
    o.uv = i.uv;
    return o;
}
