#pragma once

// The shared head (rendering_integration.hpp): the instancing records (InstLane, InstKey, InstPlan), DepthSk
// and inst_sk_bind's declaration. Never include it on its own.
namespace RenderIntegration {
inline void inst_sk_bind(ID3D11DeviceContext* ctx, const SkinPoolChunk* c,
                         ID3D11ShaderResourceView*& bound);   // Defined in rendering/instancing.hpp.

// One InstLane per instance (16 B; HLSL VSInst, IA slot 1, TEXCOORD4 uint + TEXCOORD5 float3): the live-scene
// slot (the VS reads the record from t39), the LOD dither, the final alpha (envelope applied) and the material table
// base. Two dynamic rings, one per pass family (0 the flush's, 1 the injection's and the PIP's), discard on first
// use and wrap, NO_OVERWRITE per write. The sun-depth and DLS passes consume the same lane through a plain stream of
// their own (sun_instance_vb), the volume seam through another (seam_instance_vb): no ring - each rewritten whole
// (WRITE_DISCARD) for the set it draws next.
struct InstLane {
    uint32_t slot;
    float    dither;
    float    alpha;
    float    mat_base;
};
static_assert(sizeof(InstLane) == 16, "InstLane is one float4 (HLSL VSInst twin)");
static constexpr uint32_t INST_STRIDE = static_cast<uint32_t>(sizeof(InstLane));

// The bucket key: every object field a bucket's shared constant block or pipeline state is filled from. What varies
// per instance rides the record or the lane and is deliberately absent here.
struct InstKey {
    int      mesh;
    uint32_t pool;      // SKIN_POOL: the serial of the skin pool chunk the bucket pulls from (0 = the mesh's own
                        // buffer, bound on slot 0); inst_plan_build sets it from inst_eligible's chunk.
    uint64_t mats;      // 0 = untextured; else the set's page signature (MaterialSet::Gpu).
    int      blend;
    int      mode;
    int      lit;
    int      two_sided;
    int      part;
    uint32_t route;     // The pass's per-object route bits (2 near-gap) - CB lanes, PS and viewport. INST_RUN:
                        // the flush ORs the object's draw-order run in above them (<< 2), so a bucket never spans a
                        // draw it does not commute with.
    uint64_t aux;       // The object's dynamic-light block identity (dl_bucket_hash; 0 for an unlit object, never 0
                        // for a lit one): fill_dynlights_cb (dl_select) takes every pool light that reaches the
                        // mesh, uncapped (dl_ctl mode 3), so two lit objects share a bucket exactly when they would
                        // upload the same block, and the bucket uploads the representative's.
};

// One emitted instance range: a bucket's members at one LOD level, contiguous in the ring from lane 'first'
// (ring-absolute), 'n' lanes. src = the range's first lane in the plan's own stream (p.lanes / p.lane_d2), before
// the ring offset.
struct InstRange { int lvl; uint32_t first; uint32_t n; uint32_t src = 0; };
// INST_PART_ORDER: one lane of a translucent part's far-to-near walk (inst_part_lanes; inst_draw, part 2).
struct InstPartLane { float d2; uint32_t lane; int lvl; };

struct InstPlan {
    std::vector<int32_t>  batch_of;      // Per mesh index: bucket id, -1 = draws per object.
    std::vector<uint8_t>  done;          // Per mesh index: carried by a bucket (or culled).
    std::vector<uint32_t> batch_begin;   // Per bucket: first member in 'members'.
    std::vector<uint32_t> batch_n;       // Per bucket: member count.
    std::vector<const SkinPoolChunk*> batch_pool;   // SKIN_POOL, per bucket: the chunk its Sk twin pulls from
                                                      // (null = the mesh's own buffer).
    std::vector<uint32_t> members;       // Mesh indices, bucket-contiguous, visiting order within a
                                         // bucket.
    struct Ent { InstKey key; uint32_t rank; uint32_t idx; const SkinPoolChunk* pool_c; };
    std::vector<Ent>      ents;
    // The emission: per bucket the representative (the first visible member in visiting order; the loop draws the
    // bucket when it reaches it) and its ranges.
    std::vector<uint32_t> rep;           // Per bucket; SCENE_NONE = nothing visible.
    std::vector<uint32_t> range_begin;   // Per bucket: first range.
    std::vector<uint32_t> range_n;       // Per bucket: range count.
    std::vector<InstRange> ranges;
    std::vector<std::pair<uint64_t, uint32_t>> picked;   // Scratch: (lod << 32 | distance rank, mesh index) -
                                                         // near-to-far for a normal-blend bucket, far-to-near
                                                         // otherwise; bit 31 = the coarser half of a crossfade pair.
    std::vector<InstLane> lanes;       // Scratch: the pass's whole stream, one write.
    std::vector<float>    lane_d2;       // Scratch, per lane of 'lanes': its member's distance (the pass's
                                         // memo) - INST_PART_ORDER's sort key.
    std::vector<float>    lod_t;         // Scratch, per mesh index: the member's own crossfade fraction from its
                                         // one pick (0 = none).
    std::vector<int>      lod_l;         // Likewise, the level that pick drew (clamped; its crossfade partner is
                                         // lod_l + 1 where lod_t > 0). The injection's deferral reads both: the one
                                         // source of the bucket's rule.
};
// A run loop's vertex route: the pass's own VS and layout (vs / il, bound before the loop) for a run of one buffer,
// the Sk twin, layout_inst_sk and the chunk at VS t52 for a chunk's run. end() hands the loop's successor the pass's
// own pair, as the loop always left it. StateBackup holds t52.
struct DepthSk {
    ID3D11VertexShader* vs;
    ID3D11InputLayout*  il;
    ID3D11VertexShader* sk;
    const SkinPoolChunk* cur;
    ID3D11ShaderResourceView* srv;
    void use(ID3D11DeviceContext* ctx, const SkinPoolChunk* c) {
        if (c && !cur) {
            ctx->VSSetShader(sk, nullptr, 0);
            ctx->IASetInputLayout(g_res.layout_inst_sk);
        } else if (!c && cur) {
            ctx->VSSetShader(vs, nullptr, 0);
            ctx->IASetInputLayout(il);
        }
        inst_sk_bind(ctx, c, srv);
        cur = c;
    }
    void end(ID3D11DeviceContext* ctx) { use(ctx, nullptr); }
};
}