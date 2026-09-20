// skin.hlsl - KH_SKIN_GPU: a skeletal binding's vertices, skinned on the GPU. A
// STANDALONE unit (cb.hlsl is not prefixed): one vertex shader and no pixel
// stage. C++ (kh_skin_so_batch) draws a mesh's REST STREAM as a point list
// through VSSkinSo with stream output bound and rasterisation off, so every
// vertex lands in the binding's own vertex buffer as one MeshVertex - the
// same 48 bytes (position, normal, uv, tangent) kh_skin_verts writes on the
// CPU. Every pass that draws the mesh binds that buffer as it always has:
// no other shader knows skinning exists.
//
// TWIN of kh_skin_verts (rendering_integration.hpp) - the same sums in the
// same order. Change one and change the other.
//
// The rest stream (C++ twin KhSkinSoVert, 80 bytes, built by
// kh_skin_so_stream): the AUTHORED position (metres), the authored normal
// (n / native_ext), the covariant tangent (t * native_ext) with its
// handedness, the uv, four bone indices and four weights. A weight the CPU
// path would skip (not above zero, or naming a bone past the palette) is
// stored as zero, so the loop below needs no bone-count test.
//
// The palette: three float4 per bone, row c = (M[0+c], M[3+c], M[6+c],
// M[9+c]) of kh_skin_affine_of's 12-float map, so v' = v * M + T is a dot and
// an add per axis. The weight left under one goes to the root (the identity).
//
// The box: the pose is stored in its own box, as the CPU path stores it
// (KhVsCore scales a position by size, divides a normal by it and multiplies
// a tangent by it). The box is C++'s bound of this pose (kh_skin_so_bound);
// whatever box is handed in, the drawn position is root + skinned * R.

#define KH_SKIN_SO_BONES 256   // C++ twin KH_SKIN_SO_BONES.

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

VSOutSkin VSSkinSo(VSInSkin i)
{
    float3 khsk_pp = float3(0.0f, 0.0f, 0.0f);
    float3 khsk_nn = float3(0.0f, 0.0f, 0.0f);
    float3 khsk_tt = float3(0.0f, 0.0f, 0.0f);
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
            khsk_nn += khsk_w * float3(dot(i.n, khsk_r0.xyz), dot(i.n, khsk_r1.xyz), dot(i.n, khsk_r2.xyz));
            khsk_tt += khsk_w * float3(dot(i.t.xyz, khsk_r0.xyz), dot(i.t.xyz, khsk_r1.xyz), dot(i.t.xyz, khsk_r2.xyz));
            khsk_ws += khsk_w;
        }
    }
    const float khsk_rest = (khsk_ws < 1.0f) ? (1.0f - khsk_ws) : 0.0f;   // The weight left to the root.
    khsk_pp += khsk_rest * i.p;
    khsk_nn += khsk_rest * i.n;
    khsk_tt += khsk_rest * i.t.xyz;
    VSOutSkin o;
    o.pos = (khsk_pp - khskCtr.xyz) / khskSize.xyz;
    o.nrm = khsk_nn * khskSize.xyz;
    o.tan = float4(khsk_tt / khskSize.xyz, i.t.w);   // The chart's handedness does not deform.
    o.uv = i.uv;
    return o;
}
