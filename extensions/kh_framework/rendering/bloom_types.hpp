#pragma once

// The shared head (rendering_integration.hpp): the glow and anamorphic pyramids (KhGlowPyr, KhAnaPyr). Never include it
// on its own.
namespace RenderIntegration {

// KH_GLOW_PYR - one glow pass's pyramid (cb.hlsl's KhGlowTap reads it, effect.hlsl's PSGlowSeed / PSGlowDown make it;
// kh_glow_build). Level 0 is half the frame, padded up to a multiple of 2^(KH_GLOW_LEVELS - 1) so every level halves
// exactly and one uv addresses them all; R16G16B16A16_FLOAT, for the scene's HDR bright passes. Resources holds one
// per route that draws glows (glow_pyr): [0] the scene flush (its effect meshes, then its chain), [1] the UI pass,
// [2] the PIP. Each is made at its first use, remade when its frame changes size, released with the device.
static constexpr int KH_GLOW_LEVELS = 8;   // Texels of 2 .. 256 frame pixels.
struct KhGlowPyr {
    ID3D11Texture2D*          tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;   // Every level: the pass's taps (t3).
    ID3D11RenderTargetView*   rtv[KH_GLOW_LEVELS] = {};
    ID3D11ShaderResourceView* lvl_srv[KH_GLOW_LEVELS] = {};   // One level each: the next level's source.
    UINT                      w = 0, h = 0;   // Level 0.
    void release() {
        for (int khgp_i = 0; khgp_i < KH_GLOW_LEVELS; ++khgp_i) {
            KH_SAFE_RELEASE(rtv[khgp_i]);
            KH_SAFE_RELEASE(lvl_srv[khgp_i]);
        }
        KH_SAFE_RELEASE(srv);
        KH_SAFE_RELEASE(tex);
        w = h = 0;
    }
};
struct KhAnaPyr {
    ID3D11Texture2D*          tex[2] = {};
    ID3D11ShaderResourceView* srv[2] = {};   // The pass's taps (t3 / t6); the other level's source while building.
    ID3D11RenderTargetView*   rtv[2] = {};
    UINT                      fw = 0, fh = 0;   // The frame it is made for.
    void release() {
        for (int khap_i = 0; khap_i < 2; ++khap_i) {
            KH_SAFE_RELEASE(rtv[khap_i]);
            KH_SAFE_RELEASE(srv[khap_i]);
            KH_SAFE_RELEASE(tex[khap_i]);
        }
        fw = fh = 0;
    }
};
}