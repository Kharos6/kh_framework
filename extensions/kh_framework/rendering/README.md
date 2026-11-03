# The rendering integration - a reader's guide

This folder holds the renderer: meshes, materials, cloth / chain / skinning / physics, post effects, lighting and shadow
maps, all drawn inside the game's own Direct3D 11 frame. Scripts drive it through SQF commands (Intercept); the renderer
reaches the frame by hooking the engine's D3D11 immediate context and by subscribing to the framework's Present detour.
This file explains how the code is laid out, which threads run it, how one frame flows through it, how things are named,
and what it does not do. Read it before the code.

## Contents

1. [Layout: where things live](#1-layout-where-things-live)
2. [Threads](#2-threads)
3. [The cycle: one frame, in order](#3-the-cycle-one-frame-in-order)
4. [Shadows: what is the engine's and what is ours](#4-shadows-what-is-the-engines-and-what-is-ours)
5. [Naming](#5-naming)
6. [Abbreviations](#6-abbreviations)
7. [The state map](#7-the-state-map)
8. [Limitations](#8-limitations)
9. [Before you change something](#9-before-you-change-something)

## 1. Layout: where things live

- **The hosts**, beside this folder: `rendering_integration.hpp` includes, in order, the shared head (the parts every
  feature reads: types, state, GPU resources), then `rendering_integration_features.hpp` (our own renderer: meshes,
  materials, physics, shaders, effects, lighting and shadows), then `rendering_integration_injection.hpp` (the bridge
  to the engine: the D3D hooks, the scanners of the engine's uploads and memory, the injection sites and the drivers),
  then the shared tail (the session lifecycle and the render-object API). Its head also holds the SQF COMMAND NOTES
  (the script surface's ranges, refusals, PLAYER_ONLY, MISSION RESET and SHADER_FIRST_RUN).
- **The parts**, in this folder: one feature each (`flush.hpp`, `inject_meshes.hpp`, `see_through.hpp`, ...). Each
  begins with a line naming its host and what it holds; none is included on its own. **The include order is
  load-bearing**: a part may use what the parts above it declare.
- **The shaders**, in `hlsl/`: embedded as RCDATA resources by `kh_shaders.rc` and compiled at run time (cached on
  disk as `khs_*.khsc`). `cb.hlsl` is shared by most units and holds the USER SHADER CONTRACT that user `.hlsl`
  files compile against.
- **The script surface**: `sqf_integration.hpp` (outside this folder) parses the SQF commands and calls the renderer;
  `getRenderStats`' fields are documented at `get_render_stats_sqf`.
- **The entry points** the rest of the DLL calls are the `rendering_integration_*` functions (`session_lifecycle.hpp`).
- **Where to start reading**: `flush_locked` (end of `flush.hpp`) and `inject_composited_meshes` (end of
  `inject_meshes.hpp`) read as their stage lists; each stage is a named function or a method of `FlushPass` /
  `InjectPass`.

## 2. Threads

- **The render thread** is whichever thread drives the engine's immediate context: every D3D hook (`hooked_*`,
  `draw_hooks.hpp` and `resource_hooks.hpp`) and the Present callback run on it.
- **The game thread** runs the SQF commands (`sqf_integration.hpp`), the Draw3D driver (`flush_frame`,
  `frame_drivers.hpp`) and the mission resets (`session_lifecycle.hpp`).
- **The park** is the game thread holding the graphics lock while the render thread is stalled inside a hook
  (`park_run`: the bootstrap, the watchdog, the mission end), so the game thread may run render-thread code then
  (`flush_locked` with `rt` false).
- **The engine also hands its context to the game thread** (for some resolves, where FX_LATE's chain runs; at FSAA 1x
  for the whole frame): a hook then runs there, and "the render thread" in a struct's comment means the thread inside
  the hook.
- **Worker threads**: the cloth / skin pool (`physics_workers.hpp`), the shader compiler (`shader_compiler.hpp`), the
  texture loader (`texture_loader.hpp`), the mesh cache writer (`disk_cache.hpp`).

A feature struct's comment names its owner thread and the members another thread touches.

## 3. The cycle: one frame, in order

A cycle runs from one main-depth clear to the next (`g_topo_cycles`, bumped in `hooked_clear_depthstencil`, which also
latches the cycle's camera: `g_ro.cycle_pv`). In a cycle, on the render thread:

1. **The main-depth clear**: the camera latch, the per-cycle resets.
2. **The cast fire** (`mask_cast_engine`, `mask_cast.hpp`): our meshes' shadow painted into the engine's screen-space
   shadow mask while the engine draws it.
3. **The injection** (`inject_composited_meshes`, `inject_meshes.hpp`): our opaque meshes drawn inside the engine's
   main pass, after its world opaques and ahead of its translucents (the reorder trigger, `reorder_trigger.hpp`).
4. **The flush** (`flush_locked`, `flush.hpp`) at the scene resolve: whatever the injection did not draw (effect
   meshes, blended and fading meshes, a cycle whose injection bailed), then the fullscreen effect chain - or that
   chain handed to a later resolve past the view model (FX_LATE, `late_chain.hpp`).
5. **At Present**, the UI lane (`flush_ui_locked`, `ui_flush.hpp`).

The fail direction everywhere: when a feature's inputs are missing or not trusted, it stands down - it draws nothing
new rather than something wrong.

## 4. Shadows: what is the engine's and what is ours

Two kinds of light cast shadows in this renderer, and they work differently because the game gives us different things
for each.

### 4.1 The sun

The game has sun shadows: shadow-map cascades, and a screen-space shadow mask its world shaders read. All three
directions are covered:

- **The world onto our meshes**: our mesh shaders read the engine's live cascades (`g_ls`, `shadow_capture.hpp` /
  `shadow_scan.hpp`), so a building's sun shadow falls on our meshes.
- **Our meshes onto our meshes**: our own sun shadow maps (`sun_shadow_maps.hpp`, `sun_shadow_render.hpp`), sampled in
  our mesh shaders - mesh to mesh and self-shadowing.
- **Our meshes onto the world**: the cast fire (`mask_cast.hpp`) paints our meshes' shadow into the engine's own
  screen-space shadow mask while the engine draws it, so the world's own shaders shade it.

### 4.2 Dynamic lights (lamps, flashlights, fires, ...)

**The game has no dynamic-light shadows at all.** Its point and spot lights light every surface in range with no
occlusion test, and it renders no shadow map for them - so there is nothing of the engine's to read. While shadow
information does not exist, the details of the lights themselves do: the engine uploads each draw's light list (the cb11
records: position, colour, cone, attenuation), and the renderer harvests them (`dynamic_lights_capture.hpp`,
`dynamic_lights_pool.hpp`) to light our meshes with the same lights.

The shadows are ours alone: for up to 8 of those lights at a time (`DLS_MAX`; ranked by how bright they arrive at the
camera, `dls_select.hpp`) the renderer draws its own shadow maps - a cube for a point light, one frustum for a spot -
holding **our meshes only** (`dls_render.hpp`). The engine's geometry is never in them, so, as in the game, a wall or
the terrain blocks no lamp - not for the world and not for our meshes. The maps are then applied in two different ways:

- **On our meshes** (DL_SHADOW, `KhDlsShadow` in `cb.hlsl`): inside our own shading, per light. Each light's diffuse and
  specular are scaled by that light's visibility in its map, and its per-light ambient (the glow that lights surfaces
  facing away) keeps 35 % in full shadow (`KH_DLS_AMB_KEEP`). Our shaders know the material (albedo, roughness, normal),
  so this is exact for what the maps hold: our meshes shadow each other and themselves. A mesh with `receiveShadow` off
  takes none.
- **On the world** (DLS_WORLD, `dls_world_pass.hpp`; `KhDlsWorldFactor` in `cb.hlsl`): the world's pixels were already
  shaded by the engine, which exposes no albedo or normal buffer to re-light them from. So at the scene resolve one
  fullscreen pass multiplies the scene colour by a factor: it rebuilds each pixel's world position from the engine's
  depth and estimates its normal from the depth field, recomputes every light's contribution with the engine's own
  dynamic-light function (exact), and divides what our meshes block by an estimate of all the light reaching that pixel
  (the sky's ambient, the sun - assumed unshadowed - and the lamps), which cancels the unknown albedo. It is an
  approximation that errs toward too little darkening: in daylight the factor goes to 1 and the shadow fades out, and a
  badly estimated normal shades a little wrong rather than shadowing the wrong place. Pixels where one of our meshes is
  the visible surface are skipped (they took their shadows in their own shading), and every failure leaves the pixel as
  the engine drew it.

## 5. Naming

- **`g_*`** are namespace-scope globals. A feature's state is one struct (`g_fire.lock`, `g_svs.mask_srv`): the
  struct's comment says what it is, who owns it and how the session reset treats it; its members keep the meaning their
  comments give.
- **No `kh` / `Kh` / `KH_` prefix on the renderer's own names**: the namespace (`RenderIntegration`) already says
  whose they are. The prefix stays only where a namespace does not reach:
  - the `#define` macros (`KH_SAFE_RELEASE`, `KH_PROF_SCOPE`, `KH_WSEE_BUILD`, ...: a macro ignores namespaces);
  - C++ names spelled exactly as in a shader (`KhObjRec`, `KhGpuMat`, `KH_DLS_NEAR_M`, ...: the twin greps across
    both languages);
  - the shader side's own names (HLSL identifiers, shader macros, `kh_shaders.rc`'s resource names: user shaders and
    the resource table depend on them);
  - the framework's and `sqf_integration.hpp`'s names (`kh_present_*`, `kh_param`, `kh_lower_copy`, `kh_rv_*`, ...);
  - text the DLL writes or reads: the log prefix (`KH RenderIntegration:`), the cache files (`.khsc` / `.khmc` /
    `.khtc`), the shader macro strings.
- **Locals are short** (`o`, `cam`, `from`). Where that name is refused in its function (a keyword or a Windows macro -
  `near`, `far`, `max` -, a libc name, a local of the same or an enclosing block, a member, a name the function also
  uses), a local keeps its old family letters (`cs_k`, `br_near`; a lone letter that would read as a Hungarian mark
  goes after the name: `far_m`). The few locals that shadow an outer name did so before the prefix was dropped too.
- **Tags.** An ALL-CAPS word in a comment is a tag: one mechanism, marked at every site that takes part in it - grep
  the tag to see them all. Its note is the comment that starts `<TAG> - ` (or "the `<TAG>` note" points at it). A tag
  spelled `KH_` is a shader-side name (`KH_USER_VS`, `KH_FX_LEAN`) or one of `sqf_integration.hpp`'s own
  (`KH_RV_FINITE`, `KH_REMOVE_ONE`).
- **Constants.** An ALL-CAPS `static constexpr` is a tunable or a limit, not a tag. Enumerator families: `PROF_`
  (profiler zones), `GPU_` (GPU zones), `FUNNEL_` (funnel counters), `WSEE_GPU_` (the see-through GPU timings), `LZO_`
  (the LZO decoder's states).
- **"Twin"**: two pieces of code that must agree (the injection's and the flush's per-object fills, a shader and its
  C++ mirror); change one, change the other.
- **"Stand down" / "fail direction"**: what a feature does when its inputs are missing - it draws nothing new rather
  than something wrong.

## 6. Abbreviations

| Abbreviation | Meaning |
|---|---|
| arb | the far arbiter: our meshes' analytic depth clamp against the terrain heightfield (FAR_ARB) |
| blko | the world-lighting owner (the published lighting block) |
| castocc | the cast occlusion (OCC) |
| cbs | the CB shadow: the dynamic-light harvest's copy of the engine's constant-buffer uploads |
| comp | composite: our meshes drawn into the engine's own pass (the injection), and their depth guard |
| dl / dlr | the engine's dynamic lights, harvested from its uploads (g_dl); dlr: their ring buffer (DL_RING) |
| dlf | dynamicLightFog (volumetric dynamic lights) |
| dls / dlsw | our shadow maps for the engine's dynamic lights (DLS); dlsw: their world pass |
| engcam | the engine camera scan (ENGCAM) |
| fire | the cast fire (mask_cast_engine) and its lock |
| gid | the Draw3D sample identity (GTS_IDENTITY) |
| gts | the Draw3D sample clock the attachments are stepped by (GTS) |
| hdc | the hooks' state-description cache (HOOK_DESC_CACHE) |
| infront / vm | the view-model slice: the hands and weapon, and showInFront meshes (INFRONT) |
| inj | the injection (inject_composited_meshes); g_inj_pub: what it published for the late readers |
| ls | the engine's live shadow cascades (g_ls) |
| mask | the engine's screen-space shadow mask (g_mask) |
| meshwriter / meshcache | the mesh cache's writer thread (disk_cache.hpp) and its session clock |
| nz / nearz | the near-gap route: our fragments nearer than the engine's near plane |
| nzm | the near-z marker (ssao.hpp) |
| pair | a depth encode pair: the projection's m22 / m32 |
| park | the game thread under the graphics lock, the render thread stalled |
| pip | picture-in-picture (PIP) |
| psb / psv | the see-through record's particle passes (PSB, with PSA's cycles) and its reduced engine particle shader (PSV) |
| rbuf / redir | the Map redirect's cached buffers and records (MAP_REDIRECT) |
| ro | the reorder hook's state (g_ro): the trigger, the cycle's latch, the engine pairs |
| rp | the volume replay (VOL_REPLAY) |
| rtp | the render-thread paint read (RT_PAINT_READ) |
| shpool / shbatch / shcache | the shader compiler: its worker pool, its user batches' abort / state, its cache trim and fail latch |
| simlod | the simulation level of detail of cloth and chains (SIM_LOD) |
| skysun | the sky's sun direction; sraw: the raw sun axis |
| slot | the projection slot: the engine upload the projection locator found the camera's projection in |
| snap | the composite depth snapshot the occlusion guard reads (snapshot_take) |
| ssgi | the SSGI pass's scale and size |
| svs | the volume seam (SVS); g_seam_take: its camera take |
| texldr | the texture loader thread |
| thm | the terrain heightmap (THM); thml: its uploaded layout; voc_thm: VOC's copy of it |
| tpl | the frame template: the frame constant buffer a pass's objects start from |
| upc / uwc | the upload capture and the upload width cache (upload_funnel_state.hpp) |
| uvs | the user vertex stage (KH_USER_VS) |
| vmir | the view-model mirror (VMIR); vmsee: the view-model see-through (VM_SEE) |
| voc | the visibility occluders (VOC) |
| wsee | the see-through record of the engine's translucent layers (WSEE) |

## 7. The state map

Each feature struct, its home part (where it is declared, beside its feature's code) and the other parts that read or
write it (code references; the session resets left out). A struct its home part alone uses is that part's private
state; one many parts use is a shared hub - change it with this list in hand. Regenerate it with the tools'
`statemap.py` when a feature moves.

| Instance | Type | Home part | Also used by |
|---|---|---|---|
| `g_gts` | `GtsState` | attachment_steps | chain_ends mask_cast |
| `g_rtp_c` | `RtpCounters` | attachment_steps | mask_cast sqf_integration |
| `g_castocc` | `CastOccState` | cast_occlusion | its home part alone |
| `g_cloth_fork` | `ClothFork` | cloth_solver | physics_workers |
| `g_fx_pair_guard` | `FxPairGuard` | composite_guard | ui_flush |
| `g_snap` | `SnapState` | composite_guard | inject_meshes snapshot |
| `g_inj_pub` | `InjPublishState` | composite_guard | dls_world_pass flush inject_meshes snapshot volume_seam |
| `g_lod` | `LodScaleState` | culling_state | flush inject_meshes mesh_lod |
| `g_dls_render` | `DlsRenderState` | dls_render | its home part alone |
| `g_hdc_blend` | `HookDescCache` | draw_hooks | its home part alone |
| `g_hdc_dss` | `HookDescCache` | draw_hooks | its home part alone |
| `g_vis_memo_inj` | `VisMemo` | draw_order | inject_meshes pip |
| `g_vis_memo_flush` | `VisMemo` | draw_order | flush |
| `g_dist_memo_inj` | `DistMemo` | draw_order | inject_meshes pip |
| `g_dist_memo_flush` | `DistMemo` | draw_order | flush |
| `g_dl` | `DynLightsState` | dynamic_light_state | dls_render dls_select dynamic_lights_capture dynamic_lights_pool lighting shadow_scan skin_cull sqf_integration |
| `g_dlcap` | `DlCaptureState` | dynamic_lights_capture_state | dynamic_lights_capture dynamic_lights_pool flush frame_drivers sqf_integration |
| `g_fast_clock` | `FastClock` | effect_clock | its home part alone |
| `g_engcam` | `EngCamState` | engine_camera | its home part alone |
| `g_skysun` | `SkySunState` | engine_lighting_state | engine_lighting shadow_locators |
| `g_lite_cand` | `LiteCandState` | engine_lighting_state | engine_lighting |
| `g_blko` | `BlockOwnerState` | engine_lighting_state | dls_world_pass engine_lighting reorder_trigger |
| `g_sraw` | `SunRawState` | engine_lighting_state | engine_lighting |
| `g_watch` | `WatchState` | flush_state | draw_hooks frame_drivers |
| `g_seam_take` | `SeamTakeState` | frame_state | skin_cull volume_seam |
| `g_res` | `Resources` | gpu_resources | 48 parts (a shared hub) |
| `g_reorder_hook` | `ReorderHookState` | hook_table | draw_hooks frame_drivers present_hook sqf_integration upload_funnel |
| `g_infront` | `InFrontState` | in_front | draw_hooks flush frame_drivers reorder_trigger view_model_seam |
| `g_fx_late` | `FxLateState` | late_chain | flush |
| `g_dlr` | `DlRingState` | lighting | its home part alone |
| `g_ls` | `LiveShadowState` | live_shadow_state | 18 parts (a shared hub) |
| `g_fire` | `FireState` | mask_cast_state | dls_world_pass draw_hooks mask_cast shadow_scan |
| `g_view_drop` | `ViewDropState` | mask_cast_state | flush inject_meshes shadow_scan |
| `g_mask` | `ShadowMaskState` | mask_cast_state | 10 parts (a shared hub) |
| `g_mat_gpu` | `MatGpuState` | material_binding | its home part alone |
| `g_ref_note_cen` | `RefNote` | mesh_cache | frame_drivers |
| `g_ref_note_gc` | `RefNote` | mesh_cache | asset_gc |
| `g_voc_thm` | `VocTerrainState` | occlusion_state | occlusion terrain_heightmap |
| `g_pip` | `PipState` | pip | dls_world_pass flush in_front inject_meshes reorder_trigger resource_hooks |
| `g_present` | `PresentState` | present_hook_state | flush_state present_hook sqf_integration ui_flush ui_mask |
| `g_prof` | `ProfState` | profiling | dynamic_lights_capture reorder_trigger resource_hooks sqf_integration upload_funnel |
| `g_wsee_gput` | `WseeGput` | profiling | see_through |
| `g_gpu_prof` | `GpuProf` | profiling | its home part alone |
| `g_slot_keep` | `SlotKeepState` | projection_locator | flush inject_meshes reorder_trigger shadow_locators |
| `g_proj_locator` | `ProjLocator` | projection_locator | shadow_scan upload_funnel |
| `g_rain` | `RainLensState` | render_object | effect_params flush |
| `g_ro` | `ReorderState` | reorder_state | 43 parts (a shared hub) |
| `g_rp_rec` | `ReplayRecorder` | replay | draw_hooks resource_hooks see_through sqf_integration |
| `g_rp` | `ReplayState` | replay_state | flush in_front inject_meshes replay view_model_seam volume_seam |
| `g_scene` | `Scene` | scene_state | 19 parts (a shared hub) |
| `g_wsee_sv` | `WseeSaveState` | see_through_shaders | see_through |
| `g_psv_books` | `PsvBooks` | see_through_shaders | its home part alone |
| `g_wsee` | `WseeCycleState` | see_through_state | draw_hooks flush post_chain reorder_trigger resource_hooks see_through |
| `g_psb` | `PsbState` | see_through_state | post_chain see_through see_through_shaders |
| `g_light_probe` | `CbColorProbe` | shadow_locators_state | dls_world_pass draw_hooks inject_meshes shadow_scan volume_seam |
| `g_sky_probe` | `CbColorProbe` | shadow_locators_state | dls_world_pass draw_hooks inject_meshes shadow_locators shadow_scan volume_seam |
| `g_skybind` | `SkyBindProbe` | shadow_locators_state | shadow_locators |
| `g_ring_best` | `RingBestState` | shadow_locators_state | flush inject_meshes shadow_locators shadow_scan |
| `g_sr` | `ShadowReconState` | shadow_scan | volume_seam |
| `g_cascbind` | `CascBindProbe` | shadow_scan | its home part alone |
| `g_simlod` | `SimLodView` | simulation_lod | cloth_sync |
| `g_stats` | `RenderStats` | stats | 17 parts (a shared hub) |
| `g_sun_setup` | `SunMapSetup` | sun_shadow_maps | dls_render frame_drivers inject_meshes sun_shadow_render |
| `g_sun_gen` | `SunGenState` | sun_shadow_render | flush inject_meshes |
| `g_sun_maps` | `SunMapState` | sun_shadow_state | 13 parts (a shared hub) |
| `g_thml` | `TerrainLayoutState` | terrain_heightmap_state | occlusion terrain_heightmap |
| `g_thm_auto` | `TerrainAutoState` | terrain_heightmap_state | terrain_autobuild |
| `g_thm` | `TerrainHeightState` | terrain_state | flush inject_meshes occlusion pip terrain_autobuild terrain_heightmap |
| `g_ui_mask` | `UiMask` | ui_mask_state | ui_flush ui_mask |
| `g_upc` | `UploadCap` | upload_funnel_state | upload_funnel |
| `g_user_shaders` | `UserShaderState` | user_shaders | its home part alone |
| `g_b2_snap` | `B2Snap` | view_model_mirror | its home part alone |
| `g_vmsee` | `VmseeState` | view_model_see_through | draw_hooks reorder_trigger |
| `g_vmir` | `VmirState` | view_model_state | 12 parts (a shared hub) |
| `g_svs` | `SeamState` | volume_seam_state | 12 parts (a shared hub) |

## 8. Limitations

### 8.1 FSAA 1x: no world meshes (by design)

At FSAA 1x (a single-sample main depth) every world mesh stands down - the injection and the flush alike - while
fullscreen post effects keep running (`fsaa_world_standdown`, `frame_state.hpp`).

**Why.** The integration was built around the structure of the game's multisampled frame, learned by measurement: the
engine's render thread clears the multisampled main depth (the clear hook takes that thread as "the render thread" and
starts a cycle there, and every mesh route runs only on it, keyed on the scene passes it sees), and the frame's scene
ends in a multisampled colour resolve with the main depth bound (where the flush runs, and where the main depth is
recognised again after a resolution or FSAA change, DEPTH_READOPT). The game drives a 1x frame differently (measured):

- the **game thread** clears the main depth, so the hooks take the game thread for the render thread, while the engine
  thread that actually draws the 1x scene stays outside every hook's render-thread gate - the injection, PIP, in-front
  and seam routes never run (measured: no injection or PIP injection at 1x), and the see-through capture and the
  engine shadow-cascade capture stand down with them;
- a single-sample target has **no scene resolve**, so there is nowhere for the render thread to flush or to re-adopt
  the main depth from.

Supporting 1x would mean mapping that other frame structure the way the multisampled one was mapped - which thread
draws which pass, where the main pass's opaques end and its translucents begin, which depth and colour targets to
anchor on, where a flush may run - in other words reading another large part of the game's renderer. That has not been
done; the mesh path is validated against the multisampled scene only, and standing down at 1x keeps it from drawing
anything wrong.

**What happens at 1x**: the render thread never flushes, so the watchdog parks the game thread every frame to run the
flush - the post effects. That flush draws no world mesh, the white placeholder included (its sun and dynamic-light maps
can still render, unread). Changing FSAA at run time, either way, returns the renderer to its bootstrap state, which
adopts the new main depth (WATCH_STALL, FSAA_SWITCH). FSAA 1x is not a test target.

### 8.2 Where it runs, and when a mission may use it

- **Players only.** Graphics run only where there is an interface (`hasInterface`; PLAYER_ONLY). On a dedicated
  server or a headless client every renderer command returns its empty value at once ('' / false / []): no parse, no
  report, nothing created.
- **Mission reset.** Every mission start clears the last mission's meshes, passes and physics affectors and puts the
  settings back to their defaults, in Intercept's pre-init. A command run earlier at the
  same start (a preInit function the game calls ahead of Intercept's) is cleared with the rest, unreported: the first
  commands must come from postInit or later (MISSION RESET).

### 8.3 Costs a user notices

- **The first start compiles the shaders** (SHADER_FIRST_RUN): some 130 units, about 18 minutes of one-core compile
  work spread over one worker per hardware thread - roughly 9.5 - 10 minutes on 2 threads, 5 on 4, 4 - 4.5 on 8 -
  under the white placeholder (no post-processing) until the last unit lands. Warm, it is a frame or more of
  placeholder at each mission start and device reset.
- **Any edit to a shader file recompiles on every player's machine**, comments included: the cache key hashes a unit's
  whole source, and `cb.hlsl` is part of most units and of every user shader. Pre-built caches a mod ships stop
  matching too; stale cache files stay on disk until the cache's LRU trim removes them.
- **FSAA 1x parks the game thread every frame** (8.1).

### 8.4 Dependence on the game's renderer

The renderer finds what it needs - the camera and its depth encode, the engine's dynamic lights and shadow cascades, its
screen-space shadow mask, the main pass's opaque / translucent boundary, the view model - by watching the engine's
D3D11 calls, constant-buffer uploads and memory. These are measured behaviours of the current game, not a documented
interface: a game update that changes its rendering can make a feature stand down (the fail direction: it draws
nothing new rather than something wrong) until the scanner is re-measured. The notes in the parts record what each
scanner was measured on.

### 8.5 Known gaps (documented, not fixed)

- **See-through layers and our dynamic-light world shadow** (LIMITATION in `cb.hlsl` at `KhWseeBehindColor`; the
  framework reference's section 15.2, See-through): the world pass (4.2) multiplies the scene after the see-through
  record has taken its copy of the colour behind the engine's translucent layers (B / B2). So under smoke or glass a
  world surface our meshes shadow from a dynamic light is rebuilt unshadowed, and our sorted glass over it darkens it
  further (held by the stacking cap). Shadows on our own meshes are unaffected. A fix needs render targets on B / B2 and
  a single-sample variant of the factor pass applied in the world pass.

### 8.6 Fixed limits

| Limit | Value | Where |
|---|---|---|
| Engine dynamic lights that get our shadow maps at once | 8 (`DLS_MAX`) | `dls_select.hpp` |
| Engine dynamic lights a mesh receives | uncapped (the engine's own per-draw lists carry at most 32) | `dynamic_light_state.hpp` |
| Mesh registry ids (built-in, imported and generated) | 1,048,576 for the process's life, never reused ("mesh registry full") | `mesh_store.hpp`, `mesh_generate.hpp` |
| User texture maps per `.hlsl` material or effect | 6 (`user0` - `user5`, `MAT_USER_MAPS`), beside the 6 built-in maps | `material_types.hpp` |
| Queued user-shader compile requests | 32 (`USER_REQ_MAX`; the getter retries next frame) | `user_shaders.hpp` |
| Instanced draws per pass | 65,536 lanes (`INST_CAP`); past that a bucket's members draw one by one - slower, nothing dropped | `instancing.hpp` |
| Disk cache budgets (LRU) | shaders 64 MB, meshes 1 GB, textures 2 GB | `disk_cache.hpp` |
| Distinct warnings logged per session | 256, then one notice | `core.hpp` |

## 9. Before you change something

- **Keep the include order** in the hosts: a part may use only what the parts above it declare.
- **A new global needs a session reset**: add its line to `session_globals_reset` (or to `session_objects_reset` /
  `session_scratch_reset`, per the note above `session_globals_reset`). A member added to a feature struct reset by
  one `reinit(<instance>)` line is reset with no line of its own.
- **Change a twin with its twin** (the injection's and the flush's fills, a shader and its C++ mirror); a tag's note
  lists its sites - grep the tag.
- **Name a struct's owner thread** in its comment, and the members another thread touches.
- **Shader edits cost every player a recompile** (8.3): batch them.
- **A change a script can see** (a command's arguments, defaults, clamps or results) is an edit to the separate SQF
  reference too (SQF COMMAND NOTES in `rendering_integration.hpp`); user shaders compile against `cb.hlsl`'s USER
  SHADER CONTRACT, which must not break.