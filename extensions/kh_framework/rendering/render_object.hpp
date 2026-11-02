#pragma once

// The shared head (rendering_integration.hpp): RenderObject, DepthMode, KhVbRef, the effect ids, the draw list and
// RenderObject's layout checks. Never include it on its own.
namespace RenderIntegration {
inline KhClothParams kh_cloth_defaults();   // Defined in rendering/cloth_solver.hpp.

enum class DepthMode : int {
    TestOnly = 0,   // Default: occluded by scene, depth buffer untouched.
    TestWrite = 1,
    Off = 2,
};
// KH_SKIN_POOL - a vertex buffer and the byte offset its vertex array starts at. Every buffer but a skin pool chunk
// holds one vertex array from byte 0 (an object's own, or a mesh's, shared by its objects); a chunk (KhSkinPoolChunk)
// holds the skinned poses of many bindings of one mesh, each in its slice, so a chunk's pointer alone does not name a
// binding's geometry. Every bind (IASetVertexBuffers' offset), every cache of the bound buffer, every run predicate,
// signature and identity test that can meet a skin buffer takes the pair. A pointer compare against a buffer that is
// never a chunk (a cloth's, a chain's, a vertex stage's, a mesh's own) stays exact.
struct KhVbRef {
    ID3D11Buffer* vb = nullptr;
    UINT          off = 0u;
    explicit operator bool() const { return vb != nullptr; }
};

struct RenderObject {
    int   effect = 0;   // 0 = solid color; >0 = screen-space effect (see EffectId).
    // Effect parameters, effect-specific (set_effect_params); shorter script arrays keep zero/default tails.
    float fx[12] = {};
    const std::string* fx_shader = nullptr;   // Interned resolved user.hlsl path (nullptr = builtin; consumed only
                                              // when effect == KH_EFFECT_CUSTOM) - or the resolved .cube path when
                                              // effect == KH_EFFECT_LUT (same slot, same rule). kh_fx_shader_of.
    // KH_FX_TEX: an effect's own textures, the script's "textures" - an interned set (kh_fx_tex_intern),
    // nullptr = none. Bound for a user effect and (KH_FX_TEX_ALL, kh_fx_tex_on) for any effect pass that has a set;
    // a solid mesh's is stored and never read.
    const KhFxTexSet* fx_tex = nullptr;
    bool  fullscreen = false;   // True = fullscreen triangle (post-processing pass), size unused.
    // Fullscreen passes: true = render post-tonemap over the composited frame (UI included).
    bool  affect_ui = false;
    bool  ui_only = false;   // With affect_ui: mask the pass to the UI phase. Meaningless without affect_ui.
    bool  ui_spill = false;
    bool  localized = false;   // Fullscreen pass masked to a world-space sphere around pos.
    bool  local_inverse = false;   // KH_LOCAL_INVERSE: the mask complemented.
    float local_radius[3] = { 25.0f, 25.0f, 25.0f };   // Full-strength radii per SQF axis [x, y, z]
                                                       // (m).
    int   local_shape = 0;   // 0 = sphere/ellipsoid, 1 = cube/mesh mask.
    float local_falloff = 10.0f;   // fade-to-zero band beyond the radius (m).
    bool  banded = false;   // Effect confined to a camera-distance band.
    float band_min = 0.0f;   // Band start (m); fades in over band_falloff before it.
    float band_max = 0.0f;   // Band end (m); <= 0 = unbounded (includes sky).
    float band_falloff = 10.0f;   // Fade width at both band edges (m).
    float pos[3] = {};   // SQF coords [x, y, zASL]; converted at draw.
    // KH_POS_RES (kh_pos_res): engine axes, the exact centre minus pos, valid while pos is pos_res_at.
    float pos_res[3] = {};
    float pos_res_at[3] = {};
    float size[3] = { 1.0f, 1.0f, 1.0f };   // Resolved mesh edge lengths per SQF.
    float size_mul[3] = { 1.0f, 1.0f, 1.0f };
    float rot[3] = {};   // [pitch, yaw, roll] degrees, arma sense: yaw clockwise from north about z-up, pitch
                         // nose-up about x-east, roll clockwise about y-north; heading applied outermost. Zero
                         // when rot_m came from [vectorDir, vectorUp] (kh_set_rotation_rows): axes, not angles.
    float rot_m[9] = { 1.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 1.0f };
    bool  rotated = false;   // False = identity (skip the matrix math).
    // KH_CAST_REFIT: the edge lengths (SQF order, metres) a plain cloth's simulated geometry reached about the
    // object's centre in its last uploaded buffer, when they exceed size on any axis; 0 = none. Read only through
    // kh_bounds_size_of by bounds tests (culls, caster lists, the sun map fit, the reach lists); never a draw
    // transform (size stays the script's) and never the LOD pick. Written by kh_cloth_upload under g_draw_list_mutex.
    float cast_size[3] = { 0.0f, 0.0f, 0.0f };

    // KH_ATTACH_OFFSET - where the mesh sits INSIDE the frame it follows. Each is inert unless the MATCHING lane is
    // attached (no frame for an offset to be relative to: the literal position or rotation the script set stands);
    // the two are independent of each other as the two lanes are. attach_pos is SQF order [x, y, z] like every other
    // script-facing vector, read in the FOLLOWED OBJECT's own axes (x right, y forward, z up at that object's
    // identity) - not world axes, not the mesh's own rotated frame: an attach_rot never swings the attach_pos.
    // attach_rot is a rotation matrix in ENGINE axes, the same form and builder as rot_m (kh_rotation_matrix),
    // composed onto the followed object's basis as attach_rot * basis.
    // They live here and not in KhAttach because they are mesh properties that OUTLIVE an attachment (set, detach,
    // re-attach: it still holds) and a KhAttach entry is erased the moment both its lanes clear; here they also ride
    // the ordinary staged update-and-commit instead of a side-band write under a different lock. Cost: +48 bytes on
    // sizeof(RenderObject), paid on every staging copy the flush and the injection make, plus one bool test per lane
    // per step for the meshes that set neither.
    float attach_pos[3] = {};
    float attach_rot[9] = { 1.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 1.0f };
    bool  attach_pos_on = false;   // False = skip the compose entirely (the step's fast path).
    bool  attach_rot_on = false;
    // KH_SKEL: the position follows a parent with a skeletal binding ([object, true]). The mesh draws at its authored
    // coordinates relative to the parent's origin, its vertex groups following the parent's memory points through a
    // substituted vertex buffer - KH_SKIN_POOL: a slice of its mesh's skin pool, so it joins an instanced bucket with
    // the other bindings of that chunk (kh_inst_pool_of), each drawing its own pose. Set with the KhAttach entry;
    // cleared when the position is pointed elsewhere. While skel is set the skin owns size (the drawn pose's box, so
    // culling follows the animation) and skel_ctr (that box's centre in the authored frame): kh_skin_upload and the
    // consume-point steps set both with the bytes, under g_draw_list_mutex; a binding or mesh change resets them to
    // the rest box.
    bool  skel = false;
    float skel_ctr[3] = { 0.0f, 0.0f, 0.0f };
    // KH_SKIN_POOL: the vertex base of the skinned pose in its skin pool chunk (the slice's offset / 48; 0 for a
    // chunk's first slice and without one - the routes test it against the slot table's offset, never against 0), set
    // by kh_skin_upload under g_draw_list_mutex with the slot tables it installs, carried to the record's res.w
    // (kh_objrec_fill) for the Sk twins.
    uint32_t skin_vbase = 0u;

    bool  caster_only = false;
    // KH_SHADOW_SWITCH ("castShadow" / "receiveShadow"): cast_shadow false takes the object out of every caster list
    // (kh_shadow_active - casterOnly included); receive_shadow false zeroes every received term in the mesh shaders
    // (the khObjNoRecv lane: lighting0.y per draw, KhObjRec pos.w per instance).
    bool  cast_shadow = true;
    bool  receive_shadow = true;
    // KH_OCC_FORCE ("occluder"): the object's box - exactly its size, at its position and rotation - hides what is
    // behind it in the occlusion cull (KH_VIS_OCC) whether or not anything is drawn there (the script's promise);
    // false leaves the object to the automatic rule (KH_OCC_BOX).
    bool  occluder = false;
    // KH_INFRONT: drawn in the engine's view-model depth slice (the range the first-person weapon and hands occupy, in
    // front of the whole world and depth-sorted against them) by kh_infront_inject, never by the world colour passes.
    // It casts shadows like any mesh while a slice is drawn (g_vm_slice_live; the sun / DLS maps and the mask cast are
    // world-space passes, and only the seam footprint, a world-depth draw, leaves it out), takes no screen-space AO
    // (KH_SSAO runs in the world injection alone), and draws level 0 only (the two slice draws must write identical
    // depth). Effect meshes and fullscreen passes ignore it (kh_in_front). With no view-model slice in the frame
    // (third person) the mesh is not drawn, as the hands are not.
    bool  in_front = false;
    // KH_SHOW_INFRONT ("showInFront"): false hides the mesh from the main view while it is a first-person one (the
    // cycle that last ended drew the view-model slice, g_vm_slice_live); with in_front it is never drawn, exactly as
    // visible false. The main camera's draw and pixel-ownership routes test kh_obj_shown; the cost-side censuses and
    // the routes that must keep a superset (posing, the replay rectangles) test kh_obj_vis; the PIP tests visible
    // (another camera - and an inFront mesh is never in it).
    bool  show_in_front = true;
    bool  two_sided = true;   // addRender3D spawns false (the script-side default).
    // This instance always draws level 0: kh_lod_pick is skipped at both colour loops, so no crossfade.
    bool  lod_lock = false;
    int   blend_mode = 0;   // 0 normal, 1 additive, 2 multiply, 3 screen, 4 lighten, 5 darken.
    // KH_INST_DOC: a mesh kh_inst_eligible admits draws through a bucket - one DrawIndexedInstanced per LOD level its
    // members draw at (a crossfading member at two; textured, one per drawn submesh of the level; a blend material's
    // translucent part, one per member, KH_INST_PART_ORDER), per bucket key (KhInstKey: mesh, skin pool chunk,
    // material pages, blend, depth mode, lit, twoSided, part, the pass's route - in the flush also its draw-order run,
    // KH_INST_RUN - and the dynamic-light selection), from the object record buffer (t39) + a 16-byte instance lane.
    // The per-object CB path serves the rest: normal-blend translucents, effect meshes, fullscreen passes, depth Off,
    // cloth / chain / unpooled skeletal / vertex-stage meshes, exact lighten / darken, localized / banded meshes, a
    // translucent part copy, a slot drawing a substitute, and a bucket that would overflow the lane ring
    // (KH_INST_OVERFLOW). Materials: an interned pool-owned set; updates swap the pointer.
    const KhMaterialSet* materials = nullptr;
    // KH_SCENE: this object's slot in the live scene, assigned by add_render_object and carried through every copy the
    // script side makes.
    uint32_t slot = 0xFFFFFFFFu;
    int   mesh = 0;   // Mesh registry id 0 = "box"; published ids only - see mesh_id_clamp.
    float color[4] = { 1, 1, 1, 1 };
    bool  lit = false;
    float light_ambient = 0.40f;   // Scale of the engine's ambient light on this mesh (lit [ambient, diffuse]).
    float light_diffuse = 0.60f;   // Scale of the engine's sun on this mesh (the n.L term); neither scales a lamp.
    uint64_t seq = 0;   // Creation order (fullscreen pass chaining).
    // Transient, never stored: set on the flush's staging copies, and (1, the translucent part) on the injection's
    // deferred copies (kh_wsee_defer_note), whose part the flush's tail draws. Otherwise an injected object's part is
    // drawn by the injection; the flush stages a part only for objects it owns.
    uint8_t draw_part = 0;
    // KH_WSEE_DEFER_FLAG - transient, 1 on the injection's deferred copies alone (kh_wsee_defer_note), never stored:
    // the flush's tail recognises a copy by it after kh_mesh_order has permuted the meshes (an index range over the
    // appended copies would name the wrong objects past the sort). In draw_part's padding byte: layout unchanged.
    uint8_t ws_dcopy = 0u;
    // Transient, set only on the injection's deferred copies (kh_wsee_defer_group), never stored: 0, or 1 + the copy's
    // entry in the cycle's geometry hold (g_wsee_hold) - the flush's tail draws the part from the vertex array the
    // injection drew with (kh_wsee_hold_ref), not from the slot's buffer as it stands then. 16 bits, in draw_part's
    // padding: sizeof(RenderObject) unchanged (KH_WSEE_HOLD_MAX entries a cycle).
    uint16_t ws_hold = 0u;
    DepthMode mode = DepthMode::TestOnly;
    bool  visible = true;
    bool  timed = false;
    float birth_time = 0.0f;
    double fx_t0 = 0.0;   // Creation stamp of the session clock in double (kh_fx_time's origin); never re-armed.
    float fade_in = 0.0f;
    float hold_dur = 0.0f;
    float fade_out = 0.0f;

    // KH_CLOTH. The two roles are independent: a mesh may hold neither, either or both (a collider that is itself cloth
    // is legal and collides against its own rest shape - almost certainly what a script asking for it meant). The
    // parameters live here, not in the instance table, for the same reason attach_pos does: mesh properties that
    // OUTLIVE the simulation, so turning cloth off and on again keeps the tuning; the instance table holds the STATE,
    // which does not.
    bool cloth_sim = false;         // "clothSimulation": this mesh simulates.
    bool physics_collider = false;    // "physicsCollider": this mesh pushes cloth around.
    KhClothParams cloth = kh_cloth_defaults();
    float physics_col_margin = 0.0f;      // Collider surface offset (m), on top of the cloth's thickness.
    float physics_col_friction = 1.0f;    // Collider's half of the friction product.
    bool  physics_col_inside = false;     // Contain cloth rather than exclude it.
    bool  physics_col_convex = false;     // KH_COL_CONVEX: collide as the mesh's convex pieces' planes.
    // KH_CHAIN: "chainSimulation" - the mesh's skeleton simulates as a chain of rigid bodies (KhChainState). Only the
    // switch is here, for the passes that must treat the object's buffer as its own (instancing); the settings hold
    // game objects and live game-side (KhChainCfg). Exclusive with cloth_sim. While it is on, a plain object's size
    // is the chain's (kh_chain_rest_size, kh_cloth_upload) and the script's is kept in size_mul, as under a skeletal
    // binding.
    bool  chain_sim = false;
    // KH_SIM_LOD: "simulationLod" - the cloth / chain solver's level of detail (kh_simlod_share): on/off, and floor
    // (the least share of iterations / substeps, 0.05..1), full distance and min distance (m), full and min screen
    // share (of the screen height). On by default at these values, fitted to a 1 x 2 m cape (sphere ~1.1 m): whole
    // to 25 m, the floor from 100 m, and the shares are what that cape spans there at the default field of view
    // (m11 ~1.33) - 0.06 and 0.015.
    bool  sim_lod_on = true;
    float sim_lod[5] = { 0.25f, 25.0f, 100.0f, 0.06f, 0.015f };
};

// Effect ids shared by meshes (localized, clipped to the mesh's screen footprint) and fullscreen passes. Outline,
// pulse, distance fog, the sun flare, SSGI, fog scatter and dynamicLightFog sample the engine depth buffer, as does
// any localized or banded pass and every user .hlsl effect (it may sample depth; kh_fx_needs_depth counts them all).
// The numbers are script surface - an effect may be given by id (0 - 23, 31; effect_id_from_gv) - so never renumber
// one; a new builtin takes a free id and its name there.
enum class EffectId : int {
    Solid = 0, Invert = 1, ColorGrade = 2, Vignette = 3, Chromatic = 4,
    Grain = 5, Sharpen = 6, Blur = 7, Bloom = 8, Distortion = 9,
    Outline = 10, Pulse = 11, Halation = 12, Fog = 13,
    LensFlare = 14, Anamorphic = 15, SunFlare = 16, Glitch = 17,
    Clarity = 18, Deband = 19, RainLens = 20,
    // CRT tube simulation.
    Crt = 21,
    Ssgi = 22,
    Fogscatter = 23,
    // 24 - 30 are the chain's internal side ids (SSGI's 24 - 26, with 27 - its pyramid's level draw - retired by
    // KH_SSGI_SEED; KH_FX_SIDE's 28 - 30), refused to a script (effect_id_from_gv).
    DynLightFog = 31,   // KH_DLF.
};
// User effect sentinel: RenderObject.effect for a custom ".hlsl" pass (fx_shader carries the resolved path).
static constexpr int KH_EFFECT_CUSTOM = 100;
// LUT sentinel: a ".cube" 3D-LUT grade; fx_shader carries the resolved path. The lattice binds at t19 and the builtin
// uber shader applies it, so KH_EFFECT_CUSTOM resolve sites never see this id.
static constexpr int KH_EFFECT_LUT = 101;

// Rain state: written only inside the lock-held flush, read only at CB fill sites inside serialized windows; no
// atomics needed.
static float    g_rain_vel[3] = {};
// Integrated, never multiplied: accumulates dt * factor per flush and rides fxParams1.w; the shader multiplies only
// by the user's speed param.
static double   g_rain_phase = 0.0;   // Integrated in double: a float accumulator loses its dt to rounding after hours.

static std::unordered_map<std::string, RenderObject> g_draw_list;
static std::mutex g_draw_list_mutex;

// Every object is trivially copyable so the live-scene sync is a memcpy per dirty object; the string / material
// members are interned pointers.
static_assert(std::is_trivially_copyable<RenderObject>::value, "RenderObject must stay a POD snapshot (KH_SCENE)");
// ws_dcopy and ws_hold live in draw_part's padding, so RenderObject keeps its size and layout - draw_part, ws_dcopy,
// ws_hold, then mode at draw_part's next 4-byte boundary. RenderObject is not standard-layout: offsetof on it is
// conditionally supported (C++17); MSVC's and clang's both evaluate it at compile time (clang warns, silenced here).
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(RenderObject, ws_dcopy) == offsetof(RenderObject, draw_part) + 1u,
              "KH_WSEE_DEFER_FLAG: ws_dcopy sits in draw_part's padding");
static_assert(offsetof(RenderObject, ws_hold) == offsetof(RenderObject, draw_part) + 2u,
              "KH_WSEE: ws_hold sits in draw_part's padding");
static_assert(offsetof(RenderObject, mode) == offsetof(RenderObject, draw_part) + 4u,
              "KH_WSEE: ws_hold must not move mode (RenderObject's size and layout unchanged)");
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}