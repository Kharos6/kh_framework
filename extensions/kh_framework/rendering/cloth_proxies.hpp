#pragma once

// The injection half (rendering_integration_injection.hpp): the cloth proxies (the SQF objects that carry a cloth's
// engine reads). Never include it on its own.
namespace RenderIntegration {

// CLOTH_PROXY - one local KH_HelperPhysics object per mesh with clothSimulation or chainSimulation on (both take the
// affectors its handlers add): spawned the first frame it is on, moved onto the mesh's position every frame
// (setPosWorld; world is ASL here), and deleted the frame the mesh is removed or turns both off. Script code elsewhere
// works with these objects; this side keeps each one in existence and in place and gives it its two event handlers at
// spawn (CLOTH_PROXY_EH_*).
//
// GAME THREAD ONLY, and unlocked for the same reason as g_affectors: the one writer outside the resets is
// cloth_proxy_step, from flush_frame (which also tests the table's emptiness before calling it), and nothing else
// touches the table but the mission and session resets (reset_retained_state, session_objects_reset) and
// rendering_integration_is_initialized. Every engine call is made in that step, BEFORE the park and holding no lock:
// createVehicleLocal can run the class's init event handlers and deleteVehicle its deletion handlers, and either may
// re-enter our SQF commands, which take g_draw_list_mutex (not recursive) - the rule attach_proxy_reap follows.
// Keyed by handle and owned by the mesh's creation stamp (RenderObject::seq), so a new object never inherits another's
// proxy.
static const char* const CLOTH_PROXY_TYPE = "KH_HelperPhysics";
struct ClothProxy {
    game_value obj;              // Nil until spawned.
    uint64_t   seq = 0;          // The mesh it serves.
    uint64_t   stamp = 0;        // The last step that found that mesh with cloth on.
    float      pos[3] = {};      // The mesh's position then, SQF [x, y, zASL].
    bool       failed = false;   // Creation returned null; not retried until the entry goes.
};
// EXIT_STATICS: never destructed (g_attach's rule - each entry holds its proxy object's SQF reference).
static std::unordered_map<std::string, ClothProxy>& g_cloth_proxy =
    *(new std::unordered_map<std::string, ClothProxy>());
// Every object with cloth or a chain on, as flush_frame's census found it under
// g_draw_list_mutex this frame. Game thread only.
struct ClothProxyWant { std::string h; uint64_t seq; float pos[3]; };
static std::vector<ClothProxyWant> g_cloth_proxy_want;
static uint64_t g_cloth_proxy_frame = 0;

// The two event handlers every proxy is given the moment it spawns: each turns a nearby explosion or shot into a short
// radial affector, and KH_var_physicsHandlers (the script side's own list) keeps the several proxies near one event
// from adding one affector each. Script text; the FiredNear body's closing brace for its outer 'if' was missing as
// supplied and is restored here.
static const char* const CLOTH_PROXY_EH_EXPLODED = R"KHEH(
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
static const char* const CLOTH_PROXY_EH_FIRED = R"KHEH(
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

inline bool cloth_proxy_eh_ready() {
    if (g_cloth_proxy_eh_state == 0) {
        g_cloth_proxy_eh_state = -1;
        try {
            g_cloth_proxy_eh_exploded = sqf::compile(CLOTH_PROXY_EH_EXPLODED);
            g_cloth_proxy_eh_fired = sqf::compile(CLOTH_PROXY_EH_FIRED);
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
inline void cloth_proxy_step() {
    KH_PROF_SCOPE(PROF_CLOTH_PROXY);   // PROF.
    const uint64_t f = ++g_cloth_proxy_frame;
    std::vector<game_value> dead;
    // Reconcile the table with the census. No engine call in this part.
    for (const ClothProxyWant& w : g_cloth_proxy_want) {
        ClothProxy& p = g_cloth_proxy[w.h];
        if (p.seq != w.seq) {   // A new entry, or another object under the handle.
            if (!p.obj.is_nil()) dead.push_back(p.obj);
            p = ClothProxy{};
            p.seq = w.seq;
        }
        p.stamp = f;
        memcpy(p.pos, w.pos, sizeof(p.pos));
    }
    for (auto it = g_cloth_proxy.begin(); it != g_cloth_proxy.end(); ) {
        if (it->second.stamp == f) { ++it; continue; }
        if (!it->second.obj.is_nil()) dead.push_back(it->second.obj);
        it = g_cloth_proxy.erase(it);
    }
    // The work is copied out first: a command re-entered during an engine
    // call must not be able to invalidate an iterator over the table.
    struct ClothProxyJob { std::string h; uint64_t seq; game_value obj; float pos[3]; bool failed; };
    std::vector<ClothProxyJob> jobs;
    jobs.reserve(g_cloth_proxy.size());
    for (const auto& kv : g_cloth_proxy) {
        jobs.push_back({ kv.first, kv.second.seq, kv.second.obj,
                         { kv.second.pos[0], kv.second.pos[1], kv.second.pos[2] },
                         kv.second.failed });
    }
    for (game_value& d : dead) {
        try { sqf::delete_vehicle(static_cast<object>(d)); } catch (...) {}
    }
    for (const ClothProxyJob& j : jobs) {
        try {
            // A proxy something else deleted is spawned again: the script side may rely on every cloth mesh having one.
            if (!j.obj.is_nil() && !sqf::is_null(static_cast<object>(j.obj))) {
                sqf::set_pos_world(static_cast<object>(j.obj),
                                   vector3(j.pos[0], j.pos[1], j.pos[2]));
                continue;
            }
            if (j.failed) continue;
            // Spawned at the origin and placed on the mesh below, this frame.
            const object o = sqf::create_vehicle_local(CLOTH_PROXY_TYPE, vector3(0.0f, 0.0f, 0.0f));
            auto it = g_cloth_proxy.find(j.h);
            const bool mine = it != g_cloth_proxy.end() && it->second.seq == j.seq;
            if (sqf::is_null(o)) {
                if (mine) it->second.failed = true;
                report_error_once_safe(std::string("KH RenderIntegration: cloth: could not create the physics "
                                                   "proxy (") + CLOTH_PROXY_TYPE +
                                       " missing from the loaded mods?)");
                continue;
            }
            if (!mine) {   // Its entry went while the spawn ran.
                sqf::delete_vehicle(o);
                continue;
            }
            it->second.obj = o;
            // Every spawn, a respawn included, before the proxy is placed.
            if (cloth_proxy_eh_ready()) {
                sqf::add_event_handler(o, "AmmoExplodedNear", g_cloth_proxy_eh_exploded);
                sqf::add_event_handler(o, "FiredNear", g_cloth_proxy_eh_fired);
            }
            sqf::set_pos_world(o, vector3(j.pos[0], j.pos[1], j.pos[2]));
        } catch (...) {}
    }
}
}