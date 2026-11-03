#pragma once

// The shared head (rendering_integration.hpp): the material types (Material, MaterialSet, the texture maps,
// UvAnim, FxTexSet) and the texture bind cache. Never include it on its own.
namespace RenderIntegration {
inline void fx_tex_unbind(ID3D11DeviceContext* ctx);   // Defined in rendering/user_shaders.hpp.

struct MaterialMap {
    std::string path;   // Resolved disk path ("" = slot empty).
    // USER_TEX: the colour space a user slot is sampled in (the script's "srgb"; default linear). The builtin slots
    // take tex_slot_srgb's rule and never set it.
    bool srgb = false;
};

// Map slots per material: the builtin six (pages bound at t14-t18 and t42), then USER_TEX's user0..user5 (slots
// 6-11, a .hlsl material only; pages bound at t43-t48, read by the user shader alone through KhUserTex).
static constexpr int MAT_MAPS_BUILTIN = 6;
static constexpr int MAT_USER_MAPS = 6;
static constexpr int MAT_MAPS = MAT_MAPS_BUILTIN + MAT_USER_MAPS;

// BIND_FILTER - what bind_material and dlr_append last bound (the material pages at t14 - t18, t42 and
// t43 - t48, the material table at t38; the light ring at t40), so a bind of the views a slot already holds is
// skipped. Materials share their pages by texture class (MAT_PAGES) and the table and the ring are one view
// each, so consecutive submesh draws mostly rebound identical pointers: four runtime calls per submesh, one per lit
// object. A skipped call would have bound what the slot holds: the pipeline is exactly as it was. The record is
// believed only inside a BindScope (opened after the state capture of the injection, the
// flush, the PIP, the in-front slice and seam, the sun's union and tiers, the DLS faces, the seams and the dlsw mask;
// its thread and context; it begins and ends knowing nothing) and is forgotten by every other writer of those slots:
// StateBackup::restore (t0 - t69), RpSave's restore (t0 - t127), the replay's draws (t0 - t127), mask_cast_engine's
// restore (t0 - t32) and fx_tex_bind / unbind (t43 - t48); every other PSSetShaderResources in the renderer binds
// other slots, and none of these views is ever a render target or UAV (whose bind would unbind it). A recorded view
// is bound, so the context's reference keeps it alive: no new view can take its address while the record names it.
// Outside a scope, or on another context, both functions bind as always. Per thread: a pass and its scope run on one
// thread.
struct BindCache {
    ID3D11DeviceContext* ctx = nullptr;   // The outermost scope's context; null outside every scope.
    int depth = 0;                        // Open scopes on this thread.
    bool k14 = false, k42 = false, k43 = false, k38 = false, k40 = false;   // The range's views are known.
    ID3D11ShaderResourceView* v14[5] = {};
    ID3D11ShaderResourceView* v42 = nullptr;
    ID3D11ShaderResourceView* v43[MAT_USER_MAPS] = {};
    ID3D11ShaderResourceView* v38 = nullptr;
    ID3D11ShaderResourceView* v40 = nullptr;
};
static thread_local BindCache t_kh_bc;
inline void bc_forget() noexcept {
    t_kh_bc.k14 = t_kh_bc.k42 = t_kh_bc.k43 = t_kh_bc.k38 = t_kh_bc.k40 = false;
}
inline bool bc_on(const ID3D11DeviceContext* ctx) noexcept {
    return t_kh_bc.depth > 0 && t_kh_bc.ctx == ctx;
}
struct BindScope {
    explicit BindScope(ID3D11DeviceContext* ctx) noexcept {
        if (t_kh_bc.depth++ == 0) t_kh_bc.ctx = ctx;
        bc_forget();
    }
    ~BindScope() {
        bc_forget();
        if (--t_kh_bc.depth == 0) t_kh_bc.ctx = nullptr;
    }
    BindScope(const BindScope&) = delete;
    BindScope& operator=(const BindScope&) = delete;
};

// UV_ANIM - a material slot's texture-coordinate animation, the script's uv* / flipbook params (every kind:
// pbr, arma, .hlsl). It moves every map of the slot together - the builtin six and the user maps - and every reader
// of them: the colour twins, the coverage verdicts and every alpha caster (KhMatUv, cb.hlsl). The lookup is
//     uv' = S * R(rotation + rotationSpeed * t) * (uv - 0.5) + 0.5 + offset + scroll * t
// then the warp (a ripple across each axis, tied to the surface), then the flipbook cell; t is the session clock -
// objects sharing a material animate in step. Defaults leave the uv as it is.
struct UvAnim {
    float scale[2] = { 1.0f, 1.0f };    // "uvScale": repeats per uv unit (negative mirrors).
    float offset[2] = { 0.0f, 0.0f };   // "uvOffset".
    float scroll[2] = { 0.0f, 0.0f };   // "uvScroll": uv units per second.
    float rot = 0.0f;                   // "uvRotation": degrees, about the tile's centre (0.5, 0.5).
    float rot_speed = 0.0f;             // "uvRotationSpeed": degrees per second.
    float warp[3] = { 0.0f, 0.0f, 0.0f };   // "uvWarp": amplitude (uv units), frequency (per uv unit), speed (/s).
    float flip[4] = { 1.0f, 1.0f, 0.0f, 1.0f };   // "flipbook": columns, rows, frames per second, frames.
};

struct Material {
    bool used = false;   // Slot carries an assignment.
    int shader = 0;   // 0 = "pbr"; 1 = custom user.hlsl (user_shader); 2 = "arma" (builtin).
    std::string user_shader;   // Resolved.hlsl path when shader == 1 ("" otherwise).
    // Diffuse, normal, orm, emissive, specular, speccolor. The arma model keeps its AS map in the orm slot and its SMDI
    // map in the specular slot, and takes speccolor; a .hlsl material takes all six under either naming
    // (USER_SLOTS, map_as / map_smdi below).
    MaterialMap maps[MAT_MAPS];
    // base_color, spec_color and RenderObject::color are LINEAR multipliers (glTF's convention), never sRGB-decoded:
    // 0.5 is sRGB 0.735. Scripts rely on it.
    float base_color[3] = { 1.0f, 1.0f, 1.0f };
    float roughness = 0.8f;
    float metalness = 0.0f;
    float emissive_intensity = 1.0f;
    float normal_strength = 1.0f;
    float cutoff = 0.5f;
    // 0 opaque, 1 cutout, 2 blend. A nonzero mode on any used slot makes the object an alpha caster
    // (mat_set_has_alpha; CAST_ALPHA / FOOTPRINT_ALPHA, where those are on): its sun, DLS and seam casts
    // clip on the material's alpha.
    int alpha_mode = 0;
    // User channel routes (slot*4+chan; -1 = slot-convention default)
    int route_occ = -1, route_rough = -1, route_metal = -1;
    int route_alpha = -1, route_gloss = -1;
    // The arma model. Its defaults make a map-less arma material shade as a map-less pbr one: fresnel (1.5, 0) is 0.04
    // head-on, pbr's dielectric F0, and glossiness 0.2 is pbr's roughness 0.8.
    int   route_spec = -1;                            // Specular intensity (default SMDI.g).
    float spec_color[3] = { 1.0f, 1.0f, 1.0f };       // Tints the specular (x SPECCOLOR).
    float glossiness = 0.2f;                          // Gloss with no SMDI map (0 matte - 1 mirror).
    float fresnel_n = 1.5f;                           // Refractive index N.
    float fresnel_k = 0.0f;                           // Absorption coefficient K.
    // USER_SLOTS - a .hlsl material may give its orm slot as "as" and its specular slot as "smdi" (the arma model's
    // maps: the same two slots). The name says what the slot holds: SMDI's channels are data, so the specular slot is
    // then read linear (tex_map_srgb), and the default routes are the named map's own (mat_gpu_fill). False for
    // "pbr"; unread for "arma", whose two slots hold AS and SMDI by definition.
    bool map_as = false;
    bool map_smdi = false;
    // KH_USER_VS - the .hlsl defines KhUserVertex (user_vertex_scan, at the parse): its vertices are deformed by
    // it, once a cycle (uvs_step). vertex_bound ("vertexBound"): the script's promise of how far it moves any
    // vertex (m); the CPU cannot see the deformation, so every bounds test pads by it (bounds_size_of).
    // vertex_static ("vertexStatic"): the result depends on nothing that changes by itself (no clock, no camera), so
    // it is evaluated again only when its inputs move.
    bool  user_vertex = false;
    float vertex_bound = 0.0f;
    bool  vertex_static = false;
    // UV_ANIM: the animation and what the parse derived from it (uv_anim_derive): uv_anim = any of it differs
    // from the defaults, uv_anim_time = some of it moves with the clock.
    UvAnim uv;
    bool uv_anim = false;
    bool uv_anim_time = false;
};

struct MaterialSet {
    std::vector<Material> slots;
    bool any = false;   // At least one used slot.
    // KH_USER_VS: some slot has a vertex stage; the largest vertex_bound among them; every one of them is
    // vertex_static. Derived at the parse.
    bool  vertex_any = false;
    float vertex_bound = 0.0f;
    bool  vertex_static = false;
    // UV_ANIM: some used slot with an alpha mode and an alpha source (a routed alpha, or a diffuse map) animates
    // with the clock: its casters' shadow and seam keys then change every cycle (mat_anim_cycle_key). Derived at
    // the parse.
    bool  uv_anim_alpha = false;
    // Content hash (material_set_hash): the MAT_POOL intern key and the instancing batch key. Covers every field
    // bind_material and draw_textured read.
    uint64_t hash = 0;
    // MAT_TABLE: the set's GPU state, filled by mat_gpu_ensure at first draw and reset by the table rebuild;
    // mutable because the set is shared and const to its holders. Written by the render thread or a park only.
    // MAT_GPU_NOCOPY: a copy of a set starts with none of it and never reads it - the game thread copies a live pool
    // set (apply_material_update's copy-on-write base) while the render thread may be filling this very state
    // (pages.assign), and the copy's state is discarded anyway (material_intern resets it).
    struct Gpu {
        int base = -1;   // First table entry (one per slot); -1 = not uploaded.
        uint64_t page_sig = 0;   // Hash over the pages every slot's maps live in, each slot's shader kind and alpha
                                 // mode (and a .hlsl slot's path): the material half of the instancing batch key.
        struct PageSet { ID3D11ShaderResourceView* p[MAT_MAPS]; };
        std::vector<PageSet> pages;   // Per slot, t14-t18, t42 and t43-t48.
        Gpu() = default;
        Gpu(const Gpu&) {}
        Gpu& operator=(const Gpu&) {
            base = -1;
            page_sig = 0;
            pages.clear();
            return *this;
        }
    };
    mutable Gpu gpu;
};

// FX_TEX: an effect's own textures (the script's "textures", user0 .. user5) as one immutable, interned set - its
// paths (interned, nullptr = none) and bit i = user<i> is colour (sRGB) - so an object carries one pointer
// (RenderObject::fx_tex, nullptr = no textures). Interned for the process as the paths are (one per distinct set;
// never released), on the game thread (the SQF parser); readers dereference an immutable record.
struct FxTexSet {
    const std::string* path[6] = {};
    uint8_t srgb = 0;
};
// TEX_LAND: bumps when tex_async_pump places a pending texture. A cutout / blend caster drew solid
// while its map was pending; the shadow skip keys (sun union and tiers, DLS, seam) mix this in while a
// textured alpha caster is in their set, so the next pass redraws with the map. Not g_tex_page_gen, which
// also bumps on frees and grows that change no map. tex_resolve's synchronous placement needs no bump:
// it places inside the ensure of the draw that first resolves the map. Same serialized window as
// g_tex_page_gen (render thread, or the game thread under the park).
static uint32_t g_tex_land_serial = 0;
// Defined in rendering/user_shaders.hpp.
inline void fx_tex_bind(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* const srv[MAT_USER_MAPS]);
// The pages bound for a scope and taken off at its end, on every exit: the scene chain's pass (its side draws return
// early on a failed upload) and the UI lane's deferred builtin pass.
struct FxTexScope {
    ID3D11DeviceContext* ctx = nullptr;
    FxTexScope() = default;
    FxTexScope(const FxTexScope&) = delete;
    FxTexScope& operator=(const FxTexScope&) = delete;
    void bind(ID3D11DeviceContext* ts_ctx, ID3D11ShaderResourceView* const srv[MAT_USER_MAPS]) {
        fx_tex_bind(ts_ctx, srv);
        ctx = ts_ctx;
    }
    ~FxTexScope() { if (ctx) fx_tex_unbind(ctx); }
};
}