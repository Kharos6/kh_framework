#pragma once

// The injection half (rendering_integration_injection.hpp): the cloth proxies (the SQF objects that carry a cloth's
// engine reads). Never include it on its own.
namespace RenderIntegration {

// KH_CLOTH_PROXY - one local KH_HelperPhysics object per mesh with clothSimulation or chainSimulation on (both take the
// affectors its handlers add): spawned the first frame it is on, moved onto the mesh's position every frame
// (setPosWorld; world is ASL here), and deleted the frame the mesh is removed or turns both off. Script code elsewhere
// works with these objects; this side keeps each one in existence and in place and gives it its two event handlers at
// spawn (KH_CLOTH_PROXY_EH_*).
//
// GAME THREAD ONLY, and unlocked for the same reason as g_affectors: the one writer outside the resets is
// kh_cloth_proxy_step, from flush_frame (which also tests the table's emptiness before calling it), and nothing else
// touches the table but the mission and session resets (reset_retained_state, kh_session_objects_reset) and
// rendering_integration_is_initialized. Every engine call is made in that step, BEFORE the park and holding no lock:
// createVehicleLocal can run the class's init event handlers and deleteVehicle its deletion handlers, and either may
// re-enter our SQF commands, which take g_draw_list_mutex (not recursive) - the rule kh_attach_proxy_reap follows.
// Keyed by handle and owned by the mesh's creation stamp (RenderObject::seq), so a new object never inherits another's
// proxy.
static const char* const KH_CLOTH_PROXY_TYPE = "KH_HelperPhysics";
struct KhClothProxy {
    game_value obj;              // Nil until spawned.
    uint64_t   seq = 0;          // The mesh it serves.
    uint64_t   stamp = 0;        // The last step that found that mesh with cloth on.
    float      pos[3] = {};      // The mesh's position then, SQF [x, y, zASL].
    bool       failed = false;   // Creation returned null; not retried until the entry goes.
};
// KH_EXIT_STATICS: never destructed (g_attach's rule - each entry holds its proxy object's SQF reference).
static std::unordered_map<std::string, KhClothProxy>& g_cloth_proxy =
    *(new std::unordered_map<std::string, KhClothProxy>());
// Every object with cloth or a chain on, as flush_frame's census found it under
// g_draw_list_mutex this frame. Game thread only.
struct KhClothProxyWant { std::string h; uint64_t seq; float pos[3]; };
static std::vector<KhClothProxyWant> g_cloth_proxy_want;
static uint64_t g_cloth_proxy_frame = 0;

// The two event handlers every proxy is given the moment it spawns: each turns a nearby explosion or shot into a short
// radial affector, and KH_var_physicsHandlers (the script side's own list) keeps the several proxies near one event
// from adding one affector each. Script text; the FiredNear body's closing brace for its outer 'if' was missing as
// supplied and is restored here.
static const char* const KH_CLOTH_PROXY_EH_EXPLODED = R"KHEH(
private _entity = param [0];
private _position = param [2];
private _ammo = param [4];
private _ammoConfig = configFile >> "CfgAmmo" >> _ammo;
private _radius = (getNumber (_ammoConfig >> "indirectHitRange")) max 0.1;

if (((getPosWorldVisual _entity) vectorDistance _position) <= _radius) then {
    if !([_position, _ammo] in KH_var_physicsHandlers) then {
        private _hit = getNumber (_ammoConfig >> "hit");
        addPhysicsAffector ["radial", [0.1, _position, _radius, _hit, (1 - (getNumber (_ammoConfig >> "explosive"))) max 0.5]];
        KH_var_physicsHandlers pushBackUnique [_position, _ammo];

        execute [
            [[_position, _ammo]],
            {
                params ["_physicsHandler"];

                if (_physicsHandler in KH_var_physicsHandlers) then {
                    KH_var_physicsHandlers deleteAt (KH_var_physicsHandlers find _physicsHandler);
                };
            },
            true,
            "0.1",
            false
        ];
    };
};
)KHEH";
static const char* const KH_CLOTH_PROXY_EH_FIRED = R"KHEH(
private _firer = param [1];
private _distance = param [2];
private _ammo = param [6];
private _ammoConfig = configFile >> "CfgAmmo" >> _ammo;
private _radius = (getNumber (_ammoConfig >> "caliber")) max 0.1;

if (_distance <= _radius) then {
    private _position = AGLToASL (unitAimPositionVisual _firer);

    if !([_position, _ammo] in KH_var_physicsHandlers) then {
        private _hit = getNumber (_ammoConfig >> "hit");
        addPhysicsAffector ["radial", [0.1, AGLToASL (unitAimPositionVisual _firer), _radius, _hit, (1 - (getNumber (_ammoConfig >> "explosive"))) max 0.5]];
        KH_var_physicsHandlers pushBackUnique [_position, _ammo];

        execute [
            [[_position, _ammo]],
            {
                params ["_physicsHandler"];

                if (_physicsHandler in KH_var_physicsHandlers) then {
                    KH_var_physicsHandlers deleteAt (KH_var_physicsHandlers find _physicsHandler);
                };
            },
            true,
            "0.1",
            false
        ];
    };
};
)KHEH";
// Compiled once, on the game thread, the first time a proxy spawns, and kept for the process like the sqf side's own
// compiled snippets. 0 = not yet tried, 1 = compiled, -1 = failed (reported once; proxies then spawn without handlers
// rather than not at all).
static code g_cloth_proxy_eh_exploded;
static code g_cloth_proxy_eh_fired;
static int  g_cloth_proxy_eh_state = 0;

inline bool kh_cloth_proxy_eh_ready() {
    if (g_cloth_proxy_eh_state == 0) {
        g_cloth_proxy_eh_state = -1;
        try {
            g_cloth_proxy_eh_exploded = sqf::compile(KH_CLOTH_PROXY_EH_EXPLODED);
            g_cloth_proxy_eh_fired = sqf::compile(KH_CLOTH_PROXY_EH_FIRED);
            if (!g_cloth_proxy_eh_exploded.is_nil() && !g_cloth_proxy_eh_fired.is_nil()) g_cloth_proxy_eh_state = 1;
        } catch (...) {}
        if (g_cloth_proxy_eh_state != 1) {
            report_error_once_safe("KH RenderIntegration: cloth: the physics proxy's event handlers did not compile; "
                                   "proxies spawn without them");
        }
    }
    return g_cloth_proxy_eh_state == 1;
}

// flush_frame, after its census and before the park.
inline void kh_cloth_proxy_step() {
    KH_PROF_SCOPE(KHP_CLOTH_PROXY);   // KH_PROF.
    const uint64_t khpx_f = ++g_cloth_proxy_frame;
    std::vector<game_value> khpx_dead;
    // Reconcile the table with the census. No engine call in this part.
    for (const KhClothProxyWant& khpx_w : g_cloth_proxy_want) {
        KhClothProxy& khpx_p = g_cloth_proxy[khpx_w.h];
        if (khpx_p.seq != khpx_w.seq) {   // A new entry, or another object under the handle.
            if (!khpx_p.obj.is_nil()) khpx_dead.push_back(khpx_p.obj);
            khpx_p = KhClothProxy{};
            khpx_p.seq = khpx_w.seq;
        }
        khpx_p.stamp = khpx_f;
        memcpy(khpx_p.pos, khpx_w.pos, sizeof(khpx_p.pos));
    }
    for (auto khpx_it = g_cloth_proxy.begin(); khpx_it != g_cloth_proxy.end(); ) {
        if (khpx_it->second.stamp == khpx_f) { ++khpx_it; continue; }
        if (!khpx_it->second.obj.is_nil()) khpx_dead.push_back(khpx_it->second.obj);
        khpx_it = g_cloth_proxy.erase(khpx_it);
    }
    // The work is copied out first: a command re-entered during an engine
    // call must not be able to invalidate an iterator over the table.
    struct KhClothProxyJob { std::string h; uint64_t seq; game_value obj; float pos[3]; bool failed; };
    std::vector<KhClothProxyJob> khpx_jobs;
    khpx_jobs.reserve(g_cloth_proxy.size());
    for (const auto& khpx_kv : g_cloth_proxy) {
        khpx_jobs.push_back({ khpx_kv.first, khpx_kv.second.seq, khpx_kv.second.obj,
                              { khpx_kv.second.pos[0], khpx_kv.second.pos[1], khpx_kv.second.pos[2] },
                              khpx_kv.second.failed });
    }
    for (game_value& khpx_d : khpx_dead) {
        try { sqf::delete_vehicle(static_cast<object>(khpx_d)); } catch (...) {}
    }
    for (const KhClothProxyJob& khpx_j : khpx_jobs) {
        try {
            // A proxy something else deleted is spawned again: the script side may rely on every cloth mesh having one.
            if (!khpx_j.obj.is_nil() && !sqf::is_null(static_cast<object>(khpx_j.obj))) {
                sqf::set_pos_world(static_cast<object>(khpx_j.obj),
                                   vector3(khpx_j.pos[0], khpx_j.pos[1], khpx_j.pos[2]));
                continue;
            }
            if (khpx_j.failed) continue;
            // Spawned at the origin and placed on the mesh below, this frame.
            const object khpx_o = sqf::create_vehicle_local(KH_CLOTH_PROXY_TYPE, vector3(0.0f, 0.0f, 0.0f));
            auto khpx_it = g_cloth_proxy.find(khpx_j.h);
            const bool khpx_mine = khpx_it != g_cloth_proxy.end() && khpx_it->second.seq == khpx_j.seq;
            if (sqf::is_null(khpx_o)) {
                if (khpx_mine) khpx_it->second.failed = true;
                report_error_once_safe(std::string("KH RenderIntegration: cloth: could not create the physics "
                                                   "proxy (") + KH_CLOTH_PROXY_TYPE +
                                       " missing from the loaded mods?)");
                continue;
            }
            if (!khpx_mine) {   // Its entry went while the spawn ran.
                sqf::delete_vehicle(khpx_o);
                continue;
            }
            khpx_it->second.obj = khpx_o;
            // Every spawn, a respawn included, before the proxy is placed.
            if (kh_cloth_proxy_eh_ready()) {
                sqf::add_event_handler(khpx_o, "AmmoExplodedNear", g_cloth_proxy_eh_exploded);
                sqf::add_event_handler(khpx_o, "FiredNear", g_cloth_proxy_eh_fired);
            }
            sqf::set_pos_world(khpx_o, vector3(khpx_j.pos[0], khpx_j.pos[1], khpx_j.pos[2]));
        } catch (...) {}
    }
}
}