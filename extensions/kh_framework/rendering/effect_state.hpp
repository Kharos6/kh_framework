#pragma once

// The shared head (rendering_integration.hpp): the effect state both halves read (the SSGI scale, SSAO's strength,
// radius and pass record). Never include it on its own.
namespace RenderIntegration {

// XTHREAD_ATOMIC: written by setSsgiScale (game thread), read by ensure_fx_chain (the chain's thread: the render
// thread, a park, or the game thread's late chain - FX_LATE).
static std::atomic<float> g_ssgi_scale{ 0.5f };
static std::atomic<uint32_t> g_ao_strength_bits{ 0x3F800000u };   // Float bits, default 1.0; 0 = off.
static std::atomic<uint32_t> g_ao_dist_bits{ 0x3F000000u };       // The sample radius (m), default 0.5.
// The pass's parameters, filled by ssao_pre and read by ssao_post in the same drawer's pass (a member of the
// injection's or the flush's pass object, InjectPass / FlushPass, which lives for one call).
struct SsaoPass {
    bool        on = false;      // The resources stand; the post pass may run.
    D3D11_RECT  rect = {};       // The pixel rectangle both passes draw over.
    float       proj[4] = {};    // m00, m11, m22, m32.
    float       vp_lo = 0.0f, vp_hi = 1.0f;
    UINT        w = 0, h = 0;    // The main depth's dimensions.
    // SSAO_NEARZ: the drawer's near-z route - its near (0 = the pass routes nothing) and the gap floor its routed
    // draws ramp the fragments nearer than that near from, up to vp_lo. Set by the drawer before ssao_post.
    float       nz_near = 0.0f, nz_gap = 0.0f;
    bool        term = false;    // NEARZ_MARK: the SSAO term runs (off: the mark serves the marker alone).
    bool        nzm = false;     // NEARZ_MARK: the marker has a reader this cycle (nzm_wanted).
};
}