#pragma once

// The shared head (rendering_integration.hpp): the live scene grid (Scene, SceneCell, g_scene) and the shadows-off
// flag. Never include it on its own.
namespace RenderIntegration {
static constexpr float    SCENE_CELL_M = 64.0f;   // Grid cell edge, engine x / z.
// A slot with no cell (a fullscreen pass). Not SCENE_NONE: widened to a key, 0xFFFFFFFF is the cell x 0, z -1
// (SQF x in [0, 64), y in [-64, 0)). x = INT32_MIN is no cell: scene_cell_key keeps every index within 2^30.
static constexpr uint64_t SCENE_CELL_NONE = 0x8000000000000000ull;

struct SceneCell {
    std::vector<uint32_t> slots;
    float ymin, ymax, rmax;   // Loose bounds (grow only; reset when the cell empties).
};

struct Scene {
    std::vector<RenderObject> objs;   // By slot.
    std::vector<uint8_t>      alive;
    std::vector<uint64_t>     cell;   // Grid key per slot (SCENE_CELL_NONE for none).
    std::unordered_map<uint64_t, SceneCell> grid;   // Mesh objects only, by centre cell.
    uint32_t alive_n = 0;
    uint64_t gen = 0;   // Bumps on every sync that changed anything.
    // SUN_GEN: bumps on every sync that took an object able to cast into the sun ladder (before or after the
    // change - scene_cast_kind), so an edit of anything else never re-opens the ladder.
    uint64_t gen_cast = 0;
};
static Scene g_scene;
// LOD_BOUNDS: the lowest mesh id an object was gridded under while that id was unpublished - its cell radius read
// mesh 0's bounds (the id clamps); 0xFFFFFFFF = none. With g_scene: scene_grid_insert lowers it, flush_locked's
// housekeeping grids those objects again once the id reads published. A backstop since SCENE_UNPUB: scene_sync
// takes no object whose mesh is unpublished, so none is gridded so.
static uint32_t g_scene_unpub_min = 0xFFFFFFFFu;
// Staging side (under g_draw_list_mutex).
static std::vector<RenderObject*> g_scene_stage;   // Slot -> map value (nullptr = removed).
static std::vector<std::string>   g_scene_handle;   // Slot -> handle (the flush's expiry erase).
static std::atomic<bool> g_shadows_off{ false };   // Game thread writes; every thread reads.
}