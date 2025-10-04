#pragma once

using namespace intercept;
using namespace intercept::types;

sqf_table["diag_fps"] = []() -> float {
    return sqf::diag_fps();
};

sqf_table["diag_fpsMin"] = []() -> float {
    return sqf::diag_fpsmin();
};

sqf_table["diag_frameNo"] = []() -> float {
    return sqf::diag_frameno();
};

sqf_table["diag_tickTime"] = []() -> float {
    return sqf::diag_ticktime();
};

sqf_table["diag_log"] = [](sqf_string_const_ref text) -> sol::object {
    sqf::diag_log(text);
    return sol::nil;
};

sqf_table["throw"] = [](sqf_string_const_ref text) -> sol::object {
    sqf::throw_exception(text);
    return sol::nil;
};

sqf_table["publicVariable"] = [](sqf_string_const_ref var_name) -> sol::object {
    sqf::public_variable(var_name);
    return sol::nil;
};

sqf_table["publicVariableServer"] = [](sqf_string_const_ref var_name) -> sol::object {
    sqf::public_variable_server(var_name);
    return sol::nil;
};

sqf_table["publicVariableClient"] = [](float client_id, sqf_string_const_ref var_name) -> sol::object {
    sqf::public_variable_client(client_id, var_name);
    return sol::nil;
};

sqf_table["clientOwner"] = []() -> float {
    return sqf::client_owner();
};

sqf_table["isServer"] = []() -> bool {
    return sqf::is_server();
};

sqf_table["isDedicated"] = []() -> bool {
    return sqf::is_dedicated();
};

sqf_table["hasInterface"] = []() -> bool {
    return sqf::has_interface();
};

sqf_table["currentNamespace"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::current_namespace());
};

sqf_table["missionNamespace"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::mission_namespace());
};

sqf_table["profileNamespace"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::profile_namespace());
};

sqf_table["uiNamespace"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::ui_namespace());
};

sqf_table["parsingNamespace"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::parsing_namespace());
};

sqf_table["serverNamespace"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::server_namespace());
};

sqf_table["missionProfileNamespace"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::mission_profile_namespace());
};

sqf_table["missionName"] = []() -> std::string {
    return static_cast<std::string>(sqf::mission_name());
};

sqf_table["profileName"] = []() -> std::string {
    return static_cast<std::string>(sqf::profile_name());
};

sqf_table["profileNameSteam"] = []() -> std::string {
    return static_cast<std::string>(sqf::profile_namesteam());
};

sqf_table["blufor"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::blufor());
};

sqf_table["west"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::west());
};

sqf_table["opfor"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::opfor());
};

sqf_table["east"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::east());
};

sqf_table["resistance"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::resistance());
};

sqf_table["independent"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::independent());
};

sqf_table["civilian"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::civilian());
};

sqf_table["sideLogic"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::side_logic());
};

sqf_table["sideUnknown"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::side_unknown());
};

sqf_table["sideEnemy"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::side_enemy());
};

sqf_table["sideFriendly"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::side_friendly());
};

sqf_table["sideAmbientLife"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::side_ambient_life());
};

sqf_table["sideEmpty"] = []() -> sol::object {
    return convert_game_value_to_lua(sqf::side_empty());
};