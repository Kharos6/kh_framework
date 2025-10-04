#include "framework.hpp"
#include "cryptography.hpp"
#include "kh_data.hpp"
#include "lua_integration.hpp"
#include "sqf_integration.hpp"

using namespace intercept;
using namespace intercept::types;

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start() {
    initialize_sqf_integration();
    initialize_lua_state();
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = false;
    game["postInit"] = false;
    game["frame"] = g_game_frame;
    game["time"] = g_game_time;
    game["server"] = sqf::is_server();
    game["dedicated"] = sqf::is_dedicated();
    game["headless"] = (!(sqf::is_server()) && !(sqf::has_interface()));
    game["player"] = sqf::has_interface();
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    mission["active"] = false;
    KHDataManager::instance().initialize();
    sqf::diag_log("KH Framework - Pre-start");
}

void intercept::pre_init() {
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = true;
    game["postInit"] = false;
    game["server"] = sqf::is_server();
    game["dedicated"] = sqf::is_dedicated();
    game["headless"] = (!(sqf::is_server()) && !(sqf::has_interface()));
    game["player"] = sqf::has_interface();
    g_mission_time = 0.0f;
    g_mission_frame = 0;
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    mission["active"] = true;
    initialize_terrain_matrix();
    clean_lua_state();
    KHDataManager::instance().flush_all();
    sqf::diag_log("KH Framework - Pre-init");
}

void intercept::post_init() {
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    game["preInit"] = false;
    game["postInit"] = true;
    sqf::diag_log("KH Framework - Post-init");
}

void intercept::on_frame() {
    LuaStackGuard guard(*g_lua_state);
    float current_delta = sqf::diag_delta_time();
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    g_game_time += current_delta;
    g_game_frame++;
    game["frame"] = g_game_frame;
    game["time"] = g_game_time;
    g_mission_time += current_delta;
    g_mission_frame++;
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    LuaFunctions::update_scheduler();
}

void intercept::mission_ended() {
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = false;
    game["postInit"] = false;
    g_mission_time = 0.0f;
    g_mission_frame = 0;
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    mission["active"] = false;
    clean_lua_state();
    KHDataManager::instance().flush_all();
    sqf::diag_log("KH Framework - Mission End");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}