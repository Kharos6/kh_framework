#pragma once

using namespace intercept;
using namespace intercept::sqf;
using namespace intercept::types;

// GAME DATA TYPES
#define GDT_SCALAR game_data_type::SCALAR
#define GDT_BOOL game_data_type::BOOL
#define GDT_ARRAY game_data_type::ARRAY
#define GDT_STRING game_data_type::STRING
#define GDT_NOTHING game_data_type::NOTHING
#define GDT_ANY game_data_type::ANY
#define GDT_NAMESPACE game_data_type::NAMESPACE
#define GDT_NaN game_data_type::NaN
#define GDT_CODE game_data_type::CODE
#define GDT_OBJECT game_data_type::OBJECT
#define GDT_SIDE game_data_type::SIDE
#define GDT_GROUP game_data_type::GROUP
#define GDT_TEXT game_data_type::TEXT
#define GDT_SCRIPT game_data_type::SCRIPT
#define GDT_TARGET game_data_type::TARGET
#define GDT_CONFIG game_data_type::CONFIG
#define GDT_DISPLAY game_data_type::DISPLAY
#define GDT_CONTROL game_data_type::CONTROL
#define GDT_NETOBJECT game_data_type::NetObject
#define GDT_SUBGROUP game_data_type::SUBGROUP
#define GDT_TEAM_MEMBER game_data_type::TEAM_MEMBER
#define GDT_TASK game_data_type::TASK
#define GDT_DIARY_RECORD game_data_type::DIARY_RECORD
#define GDT_LOCATION game_data_type::LOCATION
#define GDT_HASHMAP game_data_type::HASHMAP

// STATIC CAST MACROS - DIRECT GAME VALUE CASTS
#define CAST_BOOL(x) static_cast<bool>(x)
#define CAST_FLOAT(x) static_cast<float>(x)
#define CAST_INT(x) static_cast<int>(x)
#define CAST_STRING(x) static_cast<std::string>(x)
#define CAST_R_STRING(x) static_cast<r_string>(x)
#define CAST_VECTOR3(x) static_cast<vector3>(x)
#define CAST_VECTOR2(x) static_cast<vector2>(x)
#define CAST_DIARY_RECORD(x) static_cast<diary_record>(x)
#define CAST_OBJECT(x) static_cast<object>(x)
#define CAST_SIDE(x) static_cast<side>(x)
#define CAST_GROUP(x) static_cast<group>(x)
#define CAST_DISPLAY(x) static_cast<display>(x)
#define CAST_CONTROL(x) static_cast<control>(x)
#define CAST_LOCATION(x) static_cast<location>(x)
#define CAST_TASK(x) static_cast<task>(x)
#define CAST_TEAM_MEMBER(x) static_cast<team_member>(x)
#define CAST_NAMESPACE(x) static_cast<rv_namespace>(x)
#define CAST_CODE(x) static_cast<code>(x)
#define CAST_SCRIPT(x) static_cast<script>(x)
#define CAST_TEXT(x) static_cast<rv_text>(x)
#define CAST_CONFIG(x) static_cast<config>(x)
#define CAST_NETOBJECT(x) static_cast<netobject>(x)
#define CAST_SUBGROUP(x) static_cast<subgroup>(x)
#define CAST_ARRAY(gv) gv.to_array()
#define CAST_HASHMAP(gv) gv.to_hashmap()

// TYPE CONVERSION MACROS - LUA VALUE TO GAME VALUE OR STANDARD VALUE
#define LUA_TO_BOOL(x) (IS_WRAPPER(x) ? CAST_BOOL(GET_WRAPPER_VALUE(x)) : (x).as<bool>())
#define LUA_TO_FLOAT(x) (IS_WRAPPER(x) ? CAST_FLOAT(GET_WRAPPER_VALUE(x)) : (x).as<float>())
#define LUA_TO_INT(x) (IS_WRAPPER(x) ? CAST_INT(GET_WRAPPER_VALUE(x)) : (x).as<int>())
#define LUA_TO_STRING(x) (IS_WRAPPER(x) ? CAST_STRING(GET_WRAPPER_VALUE(x)) : (x).as<std::string>())
#define LUA_TO_R_STRING(x) r_string((x).as<std::string>())
#define LUA_TO_OBJECT(x) CAST_OBJECT(GET_WRAPPER_VALUE(x))
#define LUA_TO_SIDE(x) CAST_SIDE(GET_WRAPPER_VALUE(x))
#define LUA_TO_GROUP(x) CAST_GROUP(GET_WRAPPER_VALUE(x))
#define LUA_TO_DISPLAY(x) CAST_DISPLAY(GET_WRAPPER_VALUE(x))
#define LUA_TO_CONTROL(x) CAST_CONTROL(GET_WRAPPER_VALUE(x))
#define LUA_TO_LOCATION(x) CAST_LOCATION(GET_WRAPPER_VALUE(x))
#define LUA_TO_TASK(x) CAST_TASK(GET_WRAPPER_VALUE(x))
#define LUA_TO_TEAM_MEMBER(x) CAST_TEAM_MEMBER(GET_WRAPPER_VALUE(x))
#define LUA_TO_NAMESPACE(x) CAST_NAMESPACE(GET_WRAPPER_VALUE(x))
#define LUA_TO_CODE(x) CAST_CODE(GET_WRAPPER_VALUE(x))
#define LUA_TO_SCRIPT(x) CAST_SCRIPT(GET_WRAPPER_VALUE(x))
#define LUA_TO_TEXT(x) CAST_TEXT(GET_WRAPPER_VALUE(x))
#define LUA_TO_CONFIG(x) CAST_CONFIG(GET_WRAPPER_VALUE(x))
#define LUA_TO_TARGET(x) CAST_TARGET(GET_WRAPPER_VALUE(x))
#define LUA_TO_NETOBJECT(x) CAST_NETOBJECT(GET_WRAPPER_VALUE(x))
#define LUA_TO_SUBGROUP(x) CAST_SUBGROUP(GET_WRAPPER_VALUE(x))
#define LUA_TO_DIARY_RECORD(x) CAST_DIARY_RECORD(GET_WRAPPER_VALUE(x))
#define LUA_TO_GAME_VALUE(x) (IS_WRAPPER(x) ? GET_WRAPPER_VALUE(x) : convert_lua_to_game_value(x))
#define LUA_TO_VECTOR3(x) CAST_VECTOR3(convert_lua_to_game_value(x))
#define LUA_TO_VECTOR2(x) CAST_VECTOR2(convert_lua_to_game_value(x))
#define LUA_TO_ARRAY(x) convert_lua_to_game_value(x).to_array()
#define LUA_TO_HASHMAP(x) convert_lua_to_game_value(x).to_hashmap()
#define LUA_OPT_TO_BOOL(opt, default_val) ((opt) ? LUA_TO_BOOL(*(opt)) : (default_val))
#define LUA_OPT_TO_FLOAT(opt, default_val) ((opt) ? LUA_TO_FLOAT(*(opt)) : (default_val))
#define LUA_OPT_TO_INT(opt, default_val) ((opt) ? LUA_TO_INT(*(opt)) : (default_val))
#define LUA_OPT_TO_STRING(opt, default_val) ((opt) ? LUA_TO_STRING(*(opt)) : (default_val))
#define LUA_OPT_TO_R_STRING(opt, default_val) ((opt) ? LUA_TO_R_STRING(*(opt)) : (default_val))
#define LUA_OPT_TO_OBJECT(opt, default_val) ((opt) ? LUA_TO_OBJECT(*(opt)) : (default_val))
#define LUA_OPT_TO_SIDE(opt, default_val) ((opt) ? LUA_TO_SIDE(*(opt)) : (default_val))
#define LUA_OPT_TO_GROUP(opt, default_val) ((opt) ? LUA_TO_GROUP(*(opt)) : (default_val))
#define LUA_OPT_TO_DISPLAY(opt, default_val) ((opt) ? LUA_TO_DISPLAY(*(opt)) : (default_val))
#define LUA_OPT_TO_CONTROL(opt, default_val) ((opt) ? LUA_TO_CONTROL(*(opt)) : (default_val))
#define LUA_OPT_TO_LOCATION(opt, default_val) ((opt) ? LUA_TO_LOCATION(*(opt)) : (default_val))
#define LUA_OPT_TO_TASK(opt, default_val) ((opt) ? LUA_TO_TASK(*(opt)) : (default_val))
#define LUA_OPT_TO_TEAM_MEMBER(opt, default_val) ((opt) ? LUA_TO_TEAM_MEMBER(*(opt)) : (default_val))
#define LUA_OPT_TO_NAMESPACE(opt, default_val) ((opt) ? LUA_TO_NAMESPACE(*(opt)) : (default_val))
#define LUA_OPT_TO_CODE(opt, default_val) ((opt) ? LUA_TO_CODE(*(opt)) : (default_val))
#define LUA_OPT_TO_SCRIPT(opt, default_val) ((opt) ? LUA_TO_SCRIPT(*(opt)) : (default_val))
#define LUA_OPT_TO_TEXT(opt, default_val) ((opt) ? LUA_TO_TEXT(*(opt)) : (default_val))
#define LUA_OPT_TO_CONFIG(opt, default_val) ((opt) ? LUA_TO_CONFIG(*(opt)) : (default_val))
#define LUA_OPT_TO_TARGET(opt, default_val) ((opt) ? LUA_TO_TARGET(*(opt)) : (default_val))
#define LUA_OPT_TO_NETOBJECT(opt, default_val) ((opt) ? LUA_TO_NETOBJECT(*(opt)) : (default_val))
#define LUA_OPT_TO_SUBGROUP(opt, default_val) ((opt) ? LUA_TO_SUBGROUP(*(opt)) : (default_val))
#define LUA_OPT_TO_DIARY_RECORD(opt, default_val) ((opt) ? LUA_TO_DIARY_RECORD(*(opt)) : (default_val))
#define LUA_OPT_TO_GAME_VALUE(opt, default_val) ((opt) ? LUA_TO_GAME_VALUE(*(opt)) : (default_val))
#define LUA_OPT_TO_VECTOR3(opt, default_val) ((opt) ? LUA_TO_VECTOR3(*(opt)) : (default_val))
#define LUA_OPT_TO_VECTOR2(opt, default_val) ((opt) ? LUA_TO_VECTOR2(*(opt)) : (default_val))
#define LUA_OPT_TO_ARRAY(opt, default_val) ((opt) ? LUA_TO_ARRAY(*(opt)) : (default_val))
#define LUA_OPT_TO_HASHMAP(opt, default_val) ((opt) ? LUA_TO_HASHMAP(*(opt)) : (default_val))

// GENERIC VALUE TO LUA
#define GV_TO_LUA(x) convert_game_value_to_lua(x)

// TYPE HELPER MACROS
#define IS_WRAPPER(x) (x).is<GameValueWrapper>()
#define GET_WRAPPER_VALUE(x) (x).as<GameValueWrapper>().value

#define TYPE_OF(x) \
(IS_WRAPPER(x) ? GET_WRAPPER_VALUE(x).type_enum() : \
    (x.is<bool>() ? game_data_type::BOOL : \
    (x.is<float>() || x.is<int>() ? game_data_type::SCALAR : \
    (x.is<std::string>() ? game_data_type::STRING : \
    (x.is<sol::table>() ? game_data_type::ARRAY : \
        game_data_type::NOTHING)))))

// REGISTRATION MACROS - RETURN VALUE
#define REG_SQF_CMD_0(lua_name, name) \
    sqf_table[lua_name] = []() -> sol::object { \
        return GV_TO_LUA(name()); \
    }

#define REG_SQF_CMD_1(lua_name, name, p1_conv) \
    sqf_table[lua_name] = [](sol::object p1) -> sol::object { \
        return GV_TO_LUA(name(p1_conv(p1))); \
    }

#define REG_SQF_CMD_2(lua_name, name, p1_conv, p2_conv) \
    sqf_table[lua_name] = [](sol::object p1, sol::object p2) -> sol::object { \
        return GV_TO_LUA(name(p1_conv(p1), p2_conv(p2))); \
    }


// REGISTRATION MACROS - VOID RETURN
#define REG_SQF_CMD_0_VOID(lua_name, name) \
    sqf_table[lua_name] = []() -> sol::object { \
        name(); \
        return sol::nil; \
    }

#define REG_SQF_CMD_1_VOID(lua_name, name, p1_conv) \
    sqf_table[lua_name] = [](sol::object p1) -> sol::object { \
        name(p1_conv(p1)); \
        return sol::nil; \
    }

#define REG_SQF_CMD_2_VOID(lua_name, name, p1_conv, p2_conv) \
    sqf_table[lua_name] = [](sol::object p1, sol::object p2) -> sol::object { \
        name(p1_conv(p1), p2_conv(p2)); \
        return sol::nil; \
    }

/*
    rv_cargo, rv_particle_shape, rv_particle_array, rv_particle_random, rv_shot_parents, rv_action_params, rv_query_target, 
    rv_cursor_object_params, rv_vehicle_role, rv_pp_effect, rv_lnb_array, rv_ct_list, curator_selected_return, 
    rv_group_icon, rv_task_custom_data, rv_task_variables, rv_selection_params, and rv_hc_group_params don't have game_value constructors.
*/

// Registrations
REG_SQF_CMD_0("accTime", acc_time);
REG_SQF_CMD_1("admin", admin, LUA_TO_INT);
REG_SQF_CMD_2_VOID("addRating", add_rating, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("addScore", add_score, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("addScoreSide", add_score_side, LUA_TO_SIDE, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("addVehicle", add_vehicle, LUA_TO_GROUP, LUA_TO_OBJECT);
REG_SQF_CMD_1("agent", agent, LUA_TO_TEAM_MEMBER);
REG_SQF_CMD_0("agents", agents);
REG_SQF_CMD_1("AGLToASL", agl_to_asl, LUA_TO_VECTOR3);
REG_SQF_CMD_1("aimPos", aim_pos, LUA_TO_OBJECT);
REG_SQF_CMD_1("airDensityRTD", air_density_rtd, LUA_TO_FLOAT);
REG_SQF_CMD_1("airplaneThrottle", airplane_throttle, LUA_TO_OBJECT);
REG_SQF_CMD_1("alive", alive, LUA_TO_OBJECT);
REG_SQF_CMD_0("allEnv3DSoundSources", all_env_3d_sound_sources);
REG_SQF_CMD_2_VOID("allowDamage", allow_damage, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("allowDammage", allow_dammage, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1("allowedService", allowed_service, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("allowService", allow_service, LUA_TO_OBJECT, LUA_TO_INT);
REG_SQF_CMD_2_VOID("allowSprint", allow_sprint, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1("angularVelocity", angular_velocity, LUA_TO_OBJECT);
REG_SQF_CMD_1("angularVelocityModelSpace", angular_velocity_model_space, LUA_TO_OBJECT);
REG_SQF_CMD_0("armoryPoints", armory_points);
REG_SQF_CMD_1("ASLToAGL", asl_to_agl, LUA_TO_VECTOR3);
REG_SQF_CMD_1("ASLToATL", asl_to_atl, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("assignAsCommander", assign_as_commander, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("assignAsCargo", assign_as_cargo, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("assignAsDriver", assign_as_driver, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("assignCurator", assign_curator, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_1("assignedCommander", assigned_commander, LUA_TO_OBJECT);
REG_SQF_CMD_1("assignedDriver", assigned_driver, LUA_TO_OBJECT);
REG_SQF_CMD_1("assignedGroup", assigned_group, LUA_TO_OBJECT);
REG_SQF_CMD_1("assignedGunner", assigned_gunner, LUA_TO_OBJECT);
REG_SQF_CMD_1("assignedTarget", assigned_target, LUA_TO_OBJECT);
REG_SQF_CMD_1("assignedTeam", assigned_team, LUA_TO_OBJECT);
REG_SQF_CMD_1("assignedVehicle", assigned_vehicle, LUA_TO_OBJECT);
REG_SQF_CMD_1("attachedObject", attached_object, LUA_TO_LOCATION);
REG_SQF_CMD_2_VOID("attachObject", attach_object, LUA_TO_LOCATION, LUA_TO_OBJECT);
REG_SQF_CMD_1("attachedTo", attached_to, LUA_TO_OBJECT);
REG_SQF_CMD_1("ATLToASL", atl_to_asl, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("awake", awake, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1("backpackContainer", backpack_container, LUA_TO_OBJECT);
REG_SQF_CMD_1("behaviour", behaviour, LUA_TO_OBJECT);
REG_SQF_CMD_0("blufor", blufor);
REG_SQF_CMD_1("boundingCenter", bounding_center, LUA_TO_OBJECT);
REG_SQF_CMD_1("brakesDisabled", brakes_disabled, LUA_TO_OBJECT);
REG_SQF_CMD_0("briefingName", briefing_name);
REG_SQF_CMD_2("buildingExit", building_exit, LUA_TO_OBJECT, LUA_TO_INT);
REG_SQF_CMD_0("cadetMode", cadet_mode);
REG_SQF_CMD_1_VOID("calculatePlayerVisibilityByFriendly", calculate_player_visibility_by_friendly, LUA_TO_BOOL);
REG_SQF_CMD_0("campaignConfigFile", campaign_config_file);
REG_SQF_CMD_1("canDeployWeapon", can_deploy_weapon, LUA_TO_OBJECT);
REG_SQF_CMD_1("canFire", can_fire, LUA_TO_OBJECT);
REG_SQF_CMD_1("canMove", can_move, LUA_TO_OBJECT);
REG_SQF_CMD_1("canStand", can_stand, LUA_TO_OBJECT);
REG_SQF_CMD_0("canSuspend", can_suspend);
REG_SQF_CMD_1("canTriggerDynamicSimulation", can_trigger_dynamic_simulation, LUA_TO_OBJECT);
REG_SQF_CMD_1("canUnloadInCombat", can_unload_in_combat, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("cancelSimpleTaskDestination", cancel_simple_task_destination, LUA_TO_TASK);
REG_SQF_CMD_1("captive", captive, LUA_TO_OBJECT);
REG_SQF_CMD_1("captiveNum", captive_num, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("camCommit", cam_commit, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1("camCommitted", cam_committed, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("camCommitPrepared", cam_commit_prepared, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("camDestroy", cam_destroy, LUA_TO_OBJECT);
REG_SQF_CMD_1("cameraInterest", camera_interest, LUA_TO_OBJECT);
REG_SQF_CMD_0("cameraOn", camera_on);
REG_SQF_CMD_0("cameraView", camera_view);
REG_SQF_CMD_2_VOID("camPrepareBank", cam_prepare_bank, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("camPrepareDir", cam_prepare_dir, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("camPrepareDive", cam_prepare_dive, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("camPrepareFov", cam_prepare_fov, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("camPreparePos", cam_prepare_pos, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("camPrepareRelPos", cam_prepare_rel_pos, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("camPreload", cam_preload, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1("camPreloaded", cam_preloaded, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("camSetBank", cam_set_bank, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("camSetDir", cam_set_dir, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("camSetDive", cam_set_dive, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("camSetFov", cam_set_fov, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("camSetPos", cam_set_pos, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("camSetRelativePos", cam_set_relative_pos, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_1("camTarget", cam_target, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("camUseNVG", cam_use_nvg, LUA_TO_BOOL);
REG_SQF_CMD_0("cheatsEnabled", cheats_enabled);
REG_SQF_CMD_0("civilian", civilian);
REG_SQF_CMD_1_VOID("clearAllItemsFromBackpack", clear_all_items_from_backpack, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("clearBackpackCargo", clear_backpack_cargo, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("clearBackpackCargoGlobal", clear_backpack_cargo_global, LUA_TO_OBJECT);
REG_SQF_CMD_0_VOID("clearForcesRTD", clear_forces_rtd);
REG_SQF_CMD_1_VOID("clearGroupIcons", clear_group_icons, LUA_TO_GROUP);
REG_SQF_CMD_0_VOID("clearItemPool", clear_item_pool);
REG_SQF_CMD_1_VOID("clearItemCargo", clear_item_cargo, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("clearItemCargoGlobal", clear_item_cargo_global, LUA_TO_OBJECT);
REG_SQF_CMD_0_VOID("clearMagazinePool", clear_magazine_pool);
REG_SQF_CMD_1_VOID("clearMagazineCargo", clear_magazine_cargo, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("clearMagazineCargoGlobal", clear_magazine_cargo_global, LUA_TO_OBJECT);
REG_SQF_CMD_0_VOID("clearRadio", clear_radio);
REG_SQF_CMD_0_VOID("clearWeaponPool", clear_weapon_pool);
REG_SQF_CMD_1_VOID("clearWeaponCargo", clear_weapon_cargo, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("clearWeaponCargoGlobal", clear_weapon_cargo_global, LUA_TO_OBJECT);
REG_SQF_CMD_0("clientOwner", client_owner);
REG_SQF_CMD_1("collectiveRTD", collective_rtd, LUA_TO_OBJECT);
REG_SQF_CMD_1("commander", commander, LUA_TO_OBJECT);
REG_SQF_CMD_0("commandingMenu", commanding_menu);
REG_SQF_CMD_1("completedFsm", completed_fsm, LUA_TO_FLOAT);
REG_SQF_CMD_0("configFile", config_file);
REG_SQF_CMD_1("configName", config_name, LUA_TO_CONFIG);
REG_SQF_CMD_0("configNull", config_null);
REG_SQF_CMD_1("configOf", config_of, LUA_TO_OBJECT);
REG_SQF_CMD_1("configSourceMod", config_source_mod, LUA_TO_CONFIG);
REG_SQF_CMD_2("connectTerminalToUAV", connect_terminal_to_uav, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_1("conversationDisabled", conversation_disabled, LUA_TO_OBJECT);
REG_SQF_CMD_0("copyFromClipboard", copy_from_clipboard);
REG_SQF_CMD_0("curatorCamera", curator_camera);
REG_SQF_CMD_1("curatorCameraAreaCeiling", curator_camera_area_ceiling, LUA_TO_OBJECT);
REG_SQF_CMD_1("curatorEditingAreaType", curator_editing_area_type, LUA_TO_OBJECT);
REG_SQF_CMD_0("curatorMouseOver", curator_mouse_over);
REG_SQF_CMD_1("curatorPoints", curator_points, LUA_TO_OBJECT);
REG_SQF_CMD_1("curatorWaypointCost", curator_waypoint_cost, LUA_TO_OBJECT);
REG_SQF_CMD_0("currentChannel", current_channel);
REG_SQF_CMD_1("currentCommand", current_command, LUA_TO_OBJECT);
REG_SQF_CMD_0("currentNamespace", current_namespace);
REG_SQF_CMD_1("currentPilot", current_pilot, LUA_TO_OBJECT);
REG_SQF_CMD_1("currentTask", current_task, LUA_TO_OBJECT);
REG_SQF_CMD_0("cursorObject", cursor_object);
REG_SQF_CMD_0("cursorTarget", cursor_target);
REG_SQF_CMD_1("damage", damage, LUA_TO_OBJECT);
REG_SQF_CMD_0("daytime", daytime);
REG_SQF_CMD_1_VOID("deleteCollection", delete_collection, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("deleteGroupWhenEmpty", delete_group_when_empty, LUA_TO_GROUP, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("deleteGroup", delete_group, LUA_TO_GROUP);
REG_SQF_CMD_1_VOID("deleteLocation", delete_location, LUA_TO_LOCATION);
REG_SQF_CMD_1_VOID("deleteSite", delete_site, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("deleteTeam", delete_team, LUA_TO_TEAM_MEMBER);
REG_SQF_CMD_1_VOID("detach", detach, LUA_TO_OBJECT);
REG_SQF_CMD_0("didJIP", did_jip);
REG_SQF_CMD_1("didJIPOwner", did_jipowner, LUA_TO_OBJECT);
REG_SQF_CMD_0("difficulty", difficulty);
REG_SQF_CMD_0("difficultyEnabledRTD", difficulty_enabled_rtd);
REG_SQF_CMD_2_VOID("disableBrakes", disable_brakes, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("disableCollisionWith", disable_collision_with, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("disableConversation", disable_conversation, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_0_VOID("disableDebriefingStats", disable_debriefing_stats);
REG_SQF_CMD_1_VOID("disableDynamicSimulationSystem", enable_dynamic_simulation_system, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("disableNVGEquipment", disable_nvgequipment, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("disableRemoteSensors", disable_remote_sensors, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("disableTIEquipment", disable_tiequipment, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("disableUserInput", disable_user_input, LUA_TO_BOOL);
REG_SQF_CMD_0("distributionRegion", distribution_region);
REG_SQF_CMD_1("driver", driver, LUA_TO_OBJECT);
REG_SQF_CMD_0("dynamicSimulationSystemEnabled", dynamic_simulation_system_enabled);
REG_SQF_CMD_0("east", east);
REG_SQF_CMD_1("effectiveCommander", effective_commander, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("enableAimPrecision", enable_aim_precision, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableAttack", enable_attack, LUA_TO_GROUP, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableAutoStartUpRTD", enable_auto_start_up_rtd, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2("enableAutoTrimRTD", enable_auto_trim_rtd, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("enableCamShake", enable_cam_shake, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("enableCaustics", enable_caustics, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableChannel", enable_channel, LUA_TO_FLOAT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableCollisionWith", enable_collision_with, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("enableCopilot", enable_copilot, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("enableDynamicSimulationSystem", enable_dynamic_simulation_system, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableFatigue", enable_fatigue, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableMimics", enable_mimics, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("enableRadio", enable_radio, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableReload", enable_reload, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableRopeAttach", enable_rope_attach, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("enableSatNormalOnDetail", enable_sat_normal_on_detail, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableSimulation", enable_simulation, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableSimulationGlobal", enable_simulation_global, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableStamina", enable_stamina, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("enableTraffic", enable_traffic, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("enableVehicleCargo", enable_vehicle_cargo, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_0("endl", endl);
REG_SQF_CMD_2_VOID("engineOn", engine_on, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_0("environmentVolume", environment_volume);
REG_SQF_CMD_0("estimatedEndServerTime", estimated_end_server_time);
REG_SQF_CMD_1_VOID("estimatedTimeLeft", estimated_time_left, LUA_TO_FLOAT);
REG_SQF_CMD_0_VOID("exit", exit);
REG_SQF_CMD_1("eyeDirection", eye_direction, LUA_TO_OBJECT);
REG_SQF_CMD_1("eyePos", eye_pos, LUA_TO_OBJECT);
REG_SQF_CMD_1("face", face, LUA_TO_OBJECT);
REG_SQF_CMD_1("faction", faction, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("fadeEnvironment", fade_environment, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("fadeMusic", fade_music, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("fadeRadio", fade_radio, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("fadeSound", fade_sound, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("fadeSpeech", fade_speech, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("fillWeaponsFromPool", fill_weapons_from_pool, LUA_TO_OBJECT);
REG_SQF_CMD_0_VOID("finishMissionInit", finish_mission_init);
REG_SQF_CMD_1("finite", finite, LUA_TO_FLOAT);
REG_SQF_CMD_1("firstBackpack", first_backpack, LUA_TO_OBJECT);
REG_SQF_CMD_1("flag", flag, LUA_TO_OBJECT);
REG_SQF_CMD_1("flagAnimationPhase", flag_animation_phase, LUA_TO_OBJECT);
REG_SQF_CMD_1("flagOwner", flag_owner, LUA_TO_OBJECT);
REG_SQF_CMD_1("flagSide", flag_side, LUA_TO_OBJECT);
REG_SQF_CMD_1("fleeing", fleeing, LUA_TO_OBJECT);
REG_SQF_CMD_0("focusOn", focus_on);
REG_SQF_CMD_0("fog", fog);
REG_SQF_CMD_0("fogForecast", fog_forecast);
REG_SQF_CMD_0_VOID("forceEnd", force_end);
REG_SQF_CMD_2_VOID("forceFollowRoad", force_follow_road, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1("forceHitpointsDamageSync", force_hitpoints_damage_sync, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("forceRespawn", force_respawn, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("forceSpeed", force_speed, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("forceWalk", force_walk, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_0_VOID("forceWeatherChange", force_weather_change);
REG_SQF_CMD_1("formationDirection", formation_direction, LUA_TO_OBJECT);
REG_SQF_CMD_1("formationLeader", formation_leader, LUA_TO_OBJECT);
REG_SQF_CMD_1("formationPosition", formation_position, LUA_TO_OBJECT);
REG_SQF_CMD_1("formLeader", form_leader, LUA_TO_OBJECT);
REG_SQF_CMD_0("freeLook", free_look);
REG_SQF_CMD_1("fromEditor", from_editor, LUA_TO_TEAM_MEMBER);
REG_SQF_CMD_1("fuel", fuel, LUA_TO_OBJECT);
REG_SQF_CMD_1("gearIDCAmmoCount", gear_idcammo_count, LUA_TO_FLOAT);
REG_SQF_CMD_1("getAimingCoef", get_aiming_coef, LUA_TO_OBJECT);
REG_SQF_CMD_1("getAmmoCargo", get_ammo_cargo, LUA_TO_OBJECT);
REG_SQF_CMD_1("getAnimAimPrecision", get_anim_aim_precision, LUA_TO_OBJECT);
REG_SQF_CMD_1("getAnimSpeedCoef", get_anim_speed_coef, LUA_TO_OBJECT);
REG_SQF_CMD_1("getArray", get_array, LUA_TO_CONFIG);
REG_SQF_CMD_1("getAssignedCuratorLogic", get_assigned_curator_logic, LUA_TO_OBJECT);
REG_SQF_CMD_1("getAssignedCuratorUnit", get_assigned_curator_unit, LUA_TO_OBJECT);
REG_SQF_CMD_1("getAttackTarget", get_attack_target, LUA_TO_OBJECT);
REG_SQF_CMD_1("getBleedingRemaining", get_bleeding_remaining, LUA_TO_OBJECT);
REG_SQF_CMD_1("getBurningValue", get_burning_value, LUA_TO_OBJECT);
REG_SQF_CMD_0("getCalculatePlayerVisibilityByFriendly", get_calculate_player_visibility_by_friendly);
REG_SQF_CMD_1("getCameraViewDirection", get_camera_view_direction, LUA_TO_OBJECT);
REG_SQF_CMD_2("getCargoIndex", get_cargo_index, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_0("getClientStateNumber", get_client_state_number);
REG_SQF_CMD_1("getConnectedUAV", get_connected_uav, LUA_TO_OBJECT);
REG_SQF_CMD_1("getConnectedUAVUnit", get_connected_uav_unit, LUA_TO_OBJECT);
REG_SQF_CMD_1("getCorpse", get_corpse, LUA_TO_OBJECT);
REG_SQF_CMD_1("getCustomAimCoef", get_custom_aim_coef, LUA_TO_OBJECT);
REG_SQF_CMD_1("getDammage", get_dammage, LUA_TO_OBJECT);
REG_SQF_CMD_1("getDiverState", get_diver_state, LUA_TO_OBJECT);
REG_SQF_CMD_1("getDLCUsageTime", get_dlcusage_time, LUA_TO_FLOAT);
REG_SQF_CMD_0("get3DENCamera", get_eden_camera);
REG_SQF_CMD_0("getElevationOffset", getelevationoffset);
REG_SQF_CMD_1("getFatigue", get_fatigue, LUA_TO_OBJECT);
REG_SQF_CMD_1("getForcedSpeed", get_forced_speed, LUA_TO_OBJECT);
REG_SQF_CMD_2("getFriend", get_friend, LUA_TO_SIDE, LUA_TO_SIDE);
REG_SQF_CMD_1("getFuelCargo", get_fuel_cargo, LUA_TO_OBJECT);
REG_SQF_CMD_1("getFuelConsumptionCoef", get_fuel_consumption_coef, LUA_TO_OBJECT);
REG_SQF_CMD_1("getGroup", get_group, LUA_TO_OBJECT);
REG_SQF_CMD_1("getLeaning", get_leaning, LUA_TO_OBJECT);
REG_SQF_CMD_1("getMass", get_mass, LUA_TO_OBJECT);
REG_SQF_CMD_0("getMissionVersion", get_mission_version);
REG_SQF_CMD_0("getMusicPlayedTime", get_music_played_time);
REG_SQF_CMD_1("getNumber", get_number, LUA_TO_CONFIG);
REG_SQF_CMD_1("getObjectDLC", get_object_dlc, LUA_TO_OBJECT);
REG_SQF_CMD_1("getObjectFOV", get_object_fov, LUA_TO_OBJECT);
REG_SQF_CMD_1("getObjectScale", get_object_scale, LUA_TO_OBJECT);
REG_SQF_CMD_1("getObjectType", get_object_type, LUA_TO_OBJECT);
REG_SQF_CMD_1("getOxygenRemaining", get_oxygen_remaining, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPilotCameraDirection", get_pilot_camera_direction, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPilotCameraOpticsMode", get_pilot_camera_optics_mode, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPilotCameraPosition", get_pilot_camera_position, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPilotCameraRotation", get_pilot_camera_rotation, LUA_TO_OBJECT);
REG_SQF_CMD_0("getPipViewDistance", get_pip_view_distance);
REG_SQF_CMD_1("getPlayerChannel", get_player_channel, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPlayerVonVolume", get_player_von_volume, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPosASL", get_pos_asl, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPosASLVisual", get_pos_asl_visual, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPosASLW", get_pos_aslw, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPosATL", get_pos_atl, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPosATLVisual", get_pos_atl_visual, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPosVisual", get_pos_visual, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPosWorld", get_pos_world, LUA_TO_OBJECT);
REG_SQF_CMD_1("getPosWorldVisual", get_pos_world_visual, LUA_TO_OBJECT);
REG_SQF_CMD_0("getRemoteSensorsDisabled", get_remote_sensors_disabled);
REG_SQF_CMD_1("getRepairCargo", get_repair_cargo, LUA_TO_OBJECT);
REG_SQF_CMD_1("getRotorBrakeRTD", get_rotor_brake_rtd, LUA_TO_OBJECT);
REG_SQF_CMD_0("getShadowDistance", get_shadow_distance);
REG_SQF_CMD_1("getSlingLoad", get_sling_load, LUA_TO_OBJECT);
REG_SQF_CMD_1("getStamina", get_stamina, LUA_TO_OBJECT);
REG_SQF_CMD_1("getSuppression", get_suppression, LUA_TO_OBJECT);
REG_SQF_CMD_0("getTerrainGrid", get_terrain_grid);
REG_SQF_CMD_1("getTerrainHeight", get_terrain_height, LUA_TO_VECTOR2);
REG_SQF_CMD_1("getText", get_text, LUA_TO_CONFIG);
REG_SQF_CMD_1("getTextRaw", get_text_raw, LUA_TO_CONFIG);
REG_SQF_CMD_0("getTide", get_tide);
REG_SQF_CMD_0("getTIParameters", get_ti_parameters);
REG_SQF_CMD_0("getTotalDLCUsageTime", get_total_dlc_usage_time);
REG_SQF_CMD_1("getTowParent", get_tow_parent, LUA_TO_OBJECT);
REG_SQF_CMD_0("getVideoOptions", get_video_options);
REG_SQF_CMD_1("getWaterFillPercentage", get_water_fill_percentage, LUA_TO_OBJECT);
REG_SQF_CMD_1("getWaterLeakiness", get_water_leakiness, LUA_TO_OBJECT);
REG_SQF_CMD_1("getWeaponSway", get_weapon_sway, LUA_TO_OBJECT);
REG_SQF_CMD_1("getWingsOrientationRTD", get_wings_orientation_rtd, LUA_TO_OBJECT);
REG_SQF_CMD_1("getWingsPositionRTD", get_wings_position_rtd, LUA_TO_OBJECT);
REG_SQF_CMD_0("grpNull", grp_null);
REG_SQF_CMD_1("groupGet", group_get, LUA_TO_OBJECT);
REG_SQF_CMD_0("groupIconSelectable", group_icon_selectable);
REG_SQF_CMD_0("groupIconsVisible", group_icons_visible);
REG_SQF_CMD_1("groupOwner", group_owner, LUA_TO_GROUP);
REG_SQF_CMD_1("groupSelectedUnits", group_selected_units, LUA_TO_OBJECT);
REG_SQF_CMD_1("gunner", gunner, LUA_TO_OBJECT);
REG_SQF_CMD_0("gusts", gusts);
REG_SQF_CMD_1("handsHit", hands_hit, LUA_TO_OBJECT);
REG_SQF_CMD_1("hasPilotCamera", has_pilot_camera, LUA_TO_OBJECT);
REG_SQF_CMD_0("hasInterface", has_interface);
REG_SQF_CMD_1("hasCustomFace", has_custom_face, LUA_TO_OBJECT);
REG_SQF_CMD_1("hashValue", hash_value, LUA_TO_GAME_VALUE);
REG_SQF_CMD_1_VOID("hideBody", hide_body, LUA_TO_OBJECT);
REG_SQF_CMD_0("humidity", humidity);
REG_SQF_CMD_1("importance", importance, LUA_TO_LOCATION);
REG_SQF_CMD_1("incapacitatedState", incapacitated_state, LUA_TO_OBJECT);
REG_SQF_CMD_0("independent", independent);
REG_SQF_CMD_2_VOID("inflame", inflame, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1("inflamed", inflamed, LUA_TO_OBJECT);
REG_SQF_CMD_1("inheritsFrom", inherits_from, LUA_TO_CONFIG);
REG_SQF_CMD_0_VOID("initAmbientLife", init_ambient_life);
REG_SQF_CMD_1("inputController", input_controller, LUA_TO_FLOAT);
REG_SQF_CMD_1("insideBuilding", inside_building, LUA_TO_OBJECT);
REG_SQF_CMD_0("is3DENPreview", is_3den_preview);
REG_SQF_CMD_1("isAbleToBreathe", is_able_to_breathe, LUA_TO_OBJECT);
REG_SQF_CMD_1("isAgent", is_agent, LUA_TO_TEAM_MEMBER);
REG_SQF_CMD_1("isAimPrecisionEnabled", is_aim_precision_enabled, LUA_TO_OBJECT);
REG_SQF_CMD_1("isAISteeringComponentEnabled", is_ai_steering_component_enabled, LUA_TO_OBJECT);
REG_SQF_CMD_1("isAllowedCrewInImmobile", is_allowed_crew_in_immobile, LUA_TO_OBJECT);
REG_SQF_CMD_1("isArray", is_array, LUA_TO_CONFIG);
REG_SQF_CMD_1("isAutoHoverOn", is_auto_hover_on, LUA_TO_OBJECT);
REG_SQF_CMD_0("isAutotest", is_autotest);
REG_SQF_CMD_1("isAutonomous", is_autonomous, LUA_TO_OBJECT);
REG_SQF_CMD_1("isAutoTrimOnRTD", is_auto_trim_on_rtd, LUA_TO_OBJECT);
REG_SQF_CMD_1("isAwake", is_awake, LUA_TO_OBJECT);
REG_SQF_CMD_1("isBleeding", is_bleeding, LUA_TO_OBJECT);
REG_SQF_CMD_1("isBurning", is_burning, LUA_TO_OBJECT);
REG_SQF_CMD_1("isClass", is_class, LUA_TO_CONFIG);
REG_SQF_CMD_1("isCollisionLightOn", is_collision_light_on, LUA_TO_OBJECT);
REG_SQF_CMD_1("isCopilotEnabled", is_copilot_enabled, LUA_TO_OBJECT);
REG_SQF_CMD_1("isDamageAllowed", is_damage_allowed, LUA_TO_OBJECT);
REG_SQF_CMD_0("isDedicated", is_dedicated);
REG_SQF_CMD_0("is3DEN", is_eden);
REG_SQF_CMD_0("is3DENMultiplayer", is_eden_multiplayer);
REG_SQF_CMD_1("isEngineOn", is_engine_on, LUA_TO_OBJECT);
REG_SQF_CMD_2("isEqualTo", is_equal_to, LUA_TO_GAME_VALUE, LUA_TO_GAME_VALUE);
REG_SQF_CMD_2("isEqualType", is_equal_type, LUA_TO_GAME_VALUE, LUA_TO_GAME_VALUE);
REG_SQF_CMD_2("isEqualTypeAll", is_equal_type_all, LUA_TO_GAME_VALUE, LUA_TO_GAME_VALUE);
REG_SQF_CMD_2("isEqualTypeAny", is_equal_type_any, LUA_TO_GAME_VALUE, LUA_TO_GAME_VALUE);
REG_SQF_CMD_2("isEqualTypeArray", is_equal_type_array, LUA_TO_GAME_VALUE, LUA_TO_GAME_VALUE);
REG_SQF_CMD_2("isEqualTypeParams", is_equal_type_params, LUA_TO_GAME_VALUE, LUA_TO_GAME_VALUE);
REG_SQF_CMD_0("isFilePatchingEnabled", is_filepatching_enabled);
REG_SQF_CMD_1("isFinal", is_final, LUA_TO_GAME_VALUE);
REG_SQF_CMD_1("isForcedWalk", is_forced_walk, LUA_TO_OBJECT);
REG_SQF_CMD_1("isFormationLeader", is_formation_leader, LUA_TO_OBJECT);
REG_SQF_CMD_0("isGameFocused", is_game_focused);
REG_SQF_CMD_0("isGamePaused", is_game_paused);
REG_SQF_CMD_1("isGroupDeletedWhenEmpty", is_group_deleted_when_empty, LUA_TO_GROUP);
REG_SQF_CMD_1("isHidden", is_hidden, LUA_TO_OBJECT);
REG_SQF_CMD_1("isInRemainsCollector", is_in_remains_collector, LUA_TO_OBJECT);
REG_SQF_CMD_0("isInstructorFigureEnabled", is_instructor_figure_enabled);
REG_SQF_CMD_1("isManualFire", is_manual_fire, LUA_TO_OBJECT);
REG_SQF_CMD_1("isMarkedForCollection", is_marked_for_collection, LUA_TO_OBJECT);
REG_SQF_CMD_0("isMissionProfileNamespaceLoaded", is_mission_profile_namespace_loaded);
REG_SQF_CMD_0("isMultiplayer", is_multiplayer);
REG_SQF_CMD_0("isMultiplayerSolo", is_multiplayer_solo);
REG_SQF_CMD_1("isNumber", is_number, LUA_TO_CONFIG);
REG_SQF_CMD_1("isObjectHidden", is_object_hidden, LUA_TO_OBJECT);
REG_SQF_CMD_1("isObjectRTD", is_object_rtd, LUA_TO_OBJECT);
REG_SQF_CMD_1("isRemoteControlling", is_remote_controlling, LUA_TO_OBJECT);
REG_SQF_CMD_0("isRemoteExecuted", is_remote_executed);
REG_SQF_CMD_0("isRemoteExecutedJIP", is_remote_executed_jip);
REG_SQF_CMD_0("isSaving", is_saving);
REG_SQF_CMD_0("isServer", is_server);
REG_SQF_CMD_1("isSimpleObject", is_simple_object, LUA_TO_OBJECT);
REG_SQF_CMD_1("isSprintAllowed", is_sprint_allowed, LUA_TO_OBJECT);
REG_SQF_CMD_1("isStaminaEnabled", is_stamina_enabled, LUA_TO_OBJECT);
REG_SQF_CMD_0("isSteamMission", is_steam_mission);
REG_SQF_CMD_0("isSteamOverlayEnabled", is_steam_overlay_enabled);
REG_SQF_CMD_0("isStreamFriendlyUIEnabled", is_stream_friendly_ui_enabled);
REG_SQF_CMD_0("isStressDamageEnabled", is_stress_damage_enabled);
REG_SQF_CMD_1("isText", is_text, LUA_TO_CONFIG);
REG_SQF_CMD_1("isTouchingGround", is_touching_ground, LUA_TO_OBJECT);
REG_SQF_CMD_1("isTurnedOut", is_turned_out, LUA_TO_OBJECT);
REG_SQF_CMD_1("isUAVConnected", is_uavconnected, LUA_TO_OBJECT);
REG_SQF_CMD_0("isUsingAISteeringComponent", is_using_ai_steering_component);
REG_SQF_CMD_1("isVehicleCargo", is_vehicle_cargo, LUA_TO_OBJECT);
REG_SQF_CMD_1("isVehicleRadarOn", is_vehicle_radar_on, LUA_TO_OBJECT);
REG_SQF_CMD_1("isWalking", is_walking, LUA_TO_OBJECT);
REG_SQF_CMD_1("keyImage", key_image, LUA_TO_FLOAT);
REG_SQF_CMD_1("keyName", key_name, LUA_TO_FLOAT);
REG_SQF_CMD_1("landResult", land_result, LUA_TO_OBJECT);
REG_SQF_CMD_1("laserTarget", laser_target, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("lightDetachObject", light_detach_object, LUA_TO_OBJECT);
REG_SQF_CMD_0("lightnings", lightnings);
REG_SQF_CMD_2_VOID("limitSpeed", limit_speed, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_0("lineBreak", line_break);
REG_SQF_CMD_1("lifeState", life_state, LUA_TO_OBJECT);
REG_SQF_CMD_1("load", load, LUA_TO_OBJECT);
REG_SQF_CMD_1("loadAbs", load_abs, LUA_TO_OBJECT);
REG_SQF_CMD_1("loadBackpack", load_backpack, LUA_TO_OBJECT);
REG_SQF_CMD_0_VOID("loadGame", load_game);
REG_SQF_CMD_1("loadUniform", load_uniform, LUA_TO_OBJECT);
REG_SQF_CMD_1("loadVest", load_vest, LUA_TO_OBJECT);
REG_SQF_CMD_0("localNamespace", local_namespace);
REG_SQF_CMD_1("locked", locked, LUA_TO_OBJECT);
REG_SQF_CMD_1("lockedDriver", locked_driver, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("lockDriver", lock_driver, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1("lockIdentity", lock_identity, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("lockInventory", lock_inventory, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1("lockedInventory", locked_inventory, LUA_TO_OBJECT);
REG_SQF_CMD_1("locationPosition", location_position, LUA_TO_LOCATION);
REG_SQF_CMD_0("locationNull", location_null);
REG_SQF_CMD_0_VOID("logEntities", log_entities);
REG_SQF_CMD_0("markAsFinishedOnSteam", mark_as_finished_on_steam);
REG_SQF_CMD_1("maxLoad", max_load, LUA_TO_OBJECT);
REG_SQF_CMD_2("mineDetectedBy", mine_detected_by, LUA_TO_OBJECT, LUA_TO_SIDE);
REG_SQF_CMD_1("mineActive", mine_active, LUA_TO_OBJECT);
REG_SQF_CMD_0("missionConfigFile", mission_config_file);
REG_SQF_CMD_0("missionDifficulty", mission_difficulty);
REG_SQF_CMD_0("missionEnd", mission_end);
REG_SQF_CMD_0("missionName", mission_name);
REG_SQF_CMD_0("missionNamespace", mission_namespace);
REG_SQF_CMD_0("missionNameSource", mission_name_source);
REG_SQF_CMD_0("missionProfileNamespace", mission_profile_namespace);
REG_SQF_CMD_1("missileTarget", missile_target, LUA_TO_OBJECT);
REG_SQF_CMD_1("missileTargetPos", missile_target_pos, LUA_TO_OBJECT);
REG_SQF_CMD_2("modelToWorld", model_to_world, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2("modelToWorldVisual", model_to_world_visual, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2("modelToWorldVisualWorld", model_to_world_visual_world, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2("modelToWorldWorld", model_to_world_world, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_0("moonIntensity", moon_intensity);
REG_SQF_CMD_1("morale", morale, LUA_TO_OBJECT);
REG_SQF_CMD_2("moveInAny", move_in_any, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("moveInCargo", move_in_cargo, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("moveInCommander", move_in_commander, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("moveInDriver", move_in_driver, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("moveInGunner", move_in_gunner, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_1("moveTime", move_time, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("moveTo", move_to, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_1("moveToCompleted", move_to_completed, LUA_TO_OBJECT);
REG_SQF_CMD_1("moveToFailed", move_to_failed, LUA_TO_OBJECT);
REG_SQF_CMD_0("musicVolume", music_volume);
REG_SQF_CMD_1("needReload", need_reload, LUA_TO_OBJECT);
REG_SQF_CMD_1("needService", need_service, LUA_TO_OBJECT);
REG_SQF_CMD_0("nextWeatherChange", next_weather_change);
REG_SQF_CMD_1("numberOfEnginesRTD", number_of_engines_rtd, LUA_TO_OBJECT);
REG_SQF_CMD_0("objNull", obj_null);
REG_SQF_CMD_1("objectParent", object_parent, LUA_TO_OBJECT);
REG_SQF_CMD_0_VOID("openCuratorInterface", open_curator_interface);
REG_SQF_CMD_0("opfor", opfor);
REG_SQF_CMD_0("overcast", overcast);
REG_SQF_CMD_0("overcastForecast", overcast_forecast);
REG_SQF_CMD_1("owner", owner, LUA_TO_OBJECT);
REG_SQF_CMD_0("parsingNamespace", parsing_namespace);
REG_SQF_CMD_0("particlesQuality", particles_quality);
REG_SQF_CMD_1_VOID("pickWeaponPool", pick_weapon_pool, LUA_TO_OBJECT);
REG_SQF_CMD_0("player", player);
REG_SQF_CMD_0("playerRespawnTime", player_respawn_time);
REG_SQF_CMD_0("playerSide", player_side);
REG_SQF_CMD_0_VOID("playerTargetLock", player_target_lock);
REG_SQF_CMD_1("playableSlotsNumber", playable_slots_number, LUA_TO_SIDE);
REG_SQF_CMD_1("playersNumber", players_number, LUA_TO_SIDE);
REG_SQF_CMD_1("pose", pose, LUA_TO_OBJECT);
REG_SQF_CMD_1("positionCameraToWorld", position_camera_to_world, LUA_TO_VECTOR3);
REG_SQF_CMD_1("precision", precision, LUA_TO_OBJECT);
REG_SQF_CMD_1("preloadCamera", preload_camera, LUA_TO_VECTOR3);
REG_SQF_CMD_1("priority", priority, LUA_TO_TASK);
REG_SQF_CMD_0("profileName", profile_name);
REG_SQF_CMD_0("profileNamespace", profile_namespace);
REG_SQF_CMD_0("profileNameSteam", profile_namesteam);
REG_SQF_CMD_1_VOID("publicVariable", public_variable, LUA_TO_STRING);
REG_SQF_CMD_2_VOID("publicVariableClient", public_variable_client, LUA_TO_FLOAT, LUA_TO_STRING);
REG_SQF_CMD_1_VOID("publicVariableServer", public_variable_server, LUA_TO_STRING);
REG_SQF_CMD_1_VOID("putWeaponPool", put_weapon_pool, LUA_TO_OBJECT);
REG_SQF_CMD_0("radioEnabled", radio_enabled);
REG_SQF_CMD_0("radioVolume", radio_volume);
REG_SQF_CMD_0("rain", rain);
REG_SQF_CMD_0("rainbow", rainbow);
REG_SQF_CMD_1("rank", rank, LUA_TO_OBJECT);
REG_SQF_CMD_1("rankId", rank_id, LUA_TO_OBJECT);
REG_SQF_CMD_1("rating", rating, LUA_TO_OBJECT);
REG_SQF_CMD_1("rectangular", rectangular, LUA_TO_LOCATION);
REG_SQF_CMD_1("reloadEnabled", reload_enabled, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("removeAction", remove_action, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("removeAllActions", remove_all_actions, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllBinocularItems", remove_all_binocular_items, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllContainers", remove_all_containers, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllCuratorAddons", remove_all_curator_addons, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllCuratorCameraAreas", remove_all_curator_camera_areas, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllCuratorEditingAreas", remove_all_curator_editing_areas, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllHandgunItems", remove_all_handgun_items, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllItems", remove_all_items, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllItemsWithMagazines", remove_all_items_with_magazines, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllPrimaryWeaponItems", remove_all_primary_weapon_items, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllSecondaryWeaponItems", remove_all_secondary_weapon_items, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeAllWeapons", remove_all_weapons, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeBackpack", remove_backpack, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeBackpackGlobal", remove_backpack_global, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("removeCuratorCameraArea", remove_curator_camera_area, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("removeCuratorEditingArea", remove_curator_editing_area, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("removeGoggles", remove_goggles, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("removeGroupIcon", remove_group_icon, LUA_TO_GROUP, LUA_TO_INT);
REG_SQF_CMD_1_VOID("removeHeadgear", remove_headgear, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("removeOwnedMine", remove_owned_mine, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_0("remoteExecutedOwner", remote_executed_owner);
REG_SQF_CMD_1("remoteControlled", remote_controlled, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("remoteControl", remote_control, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeSwitchableUnit", remove_switchable_unit, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("removeTeamMember", remove_team_member, LUA_TO_TEAM_MEMBER, LUA_TO_TEAM_MEMBER);
REG_SQF_CMD_1_VOID("removeUniform", remove_uniform, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("removeVest", remove_vest, LUA_TO_OBJECT);
REG_SQF_CMD_0("resistance", resistance);
REG_SQF_CMD_0_VOID("resetCamShake", reset_cam_shake);
REG_SQF_CMD_1_VOID("resetSubgroupDirection", reset_subgroup_direction, LUA_TO_OBJECT);
REG_SQF_CMD_0("reversedMouseY", reversed_mouse_y);
REG_SQF_CMD_2_VOID("revealMine", reveal_mine, LUA_TO_SIDE, LUA_TO_OBJECT);
REG_SQF_CMD_1("ropeLength", rope_length, LUA_TO_OBJECT);
REG_SQF_CMD_1("ropeUnwound", rope_unwound, LUA_TO_OBJECT);
REG_SQF_CMD_0_VOID("runInitScript", run_init_script);
REG_SQF_CMD_0_VOID("save3DENPreferences", save_3den_preferences);
REG_SQF_CMD_0_VOID("saveGame", save_game);
REG_SQF_CMD_0_VOID("saveJoysticks", save_joysticks);
REG_SQF_CMD_0("saveMissionProfileNamespace", save_mission_profile_namespace);
REG_SQF_CMD_0_VOID("saveProfileNamespace", save_profile_namespace);
REG_SQF_CMD_0("savingEnabled", saving_enabled);
REG_SQF_CMD_1("score", score, LUA_TO_OBJECT);
REG_SQF_CMD_1("scoreSide", score_side, LUA_TO_SIDE);
REG_SQF_CMD_0("scriptNull", script_null);
REG_SQF_CMD_1("scriptDone", script_done, LUA_TO_SCRIPT);
REG_SQF_CMD_1("scudState", scud_state, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("selectLeader", select_leader, LUA_TO_GROUP, LUA_TO_OBJECT);
REG_SQF_CMD_1("selectMax", select_max, LUA_TO_GAME_VALUE);
REG_SQF_CMD_1("selectMin", select_min, LUA_TO_GAME_VALUE);
REG_SQF_CMD_0_VOID("selectNoPlayer", select_no_player);
REG_SQF_CMD_1_VOID("selectPlayer", select_player, LUA_TO_OBJECT);
REG_SQF_CMD_0("sentencesEnabled", sentences_enabled);
REG_SQF_CMD_0("serverNamespace", server_namespace);
REG_SQF_CMD_0("serverTime", server_time);
REG_SQF_CMD_1_VOID("setAccTime", set_acc_time, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setActualCollectiveRTD", set_actual_collective_rtd, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setAmmoCargo", set_ammo_cargo, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setAnimSpeedCoef", set_anim_speed_coef, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setAngularVelocity", set_angular_velocity, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setAngularVelocityModelSpace", set_angular_velocity_model_space, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_1_VOID("setAperture", set_aperture, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("setArmoryPoints", set_armory_points, LUA_TO_FLOAT);
REG_SQF_CMD_2("setAutonomous", set_autonomous, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setBleedingRemaining", set_bleeding_remaining, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setCameraInterest", set_camera_interest, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setCollisionLight", set_collision_light, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setConvoySeperation", set_convoy_seperation, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1("setCurrentChannel", set_current_channel, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setCurrentTask", set_current_task, LUA_TO_OBJECT, LUA_TO_TASK);
REG_SQF_CMD_2_VOID("setCuratorCameraAreaCeiling", set_curator_camera_area_ceiling, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setCuratorEditingAreaType", set_curator_editing_area_type, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setCuratorWaypointCost", set_curator_waypoint_cost, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setCustomAimCoef", set_custom_aim_coef, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setCustomWeightRTD", set_custom_weight_rtd, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setDefaultCamera", set_default_camera, LUA_TO_VECTOR3, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setDetailMapBlendPars", set_detail_map_blend_pars, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setDir", set_dir, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setDirection", set_direction, LUA_TO_LOCATION, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setDropInterval", set_drop_interval, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setEffectiveCommander", set_effective_commander, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("setFaceAnimation", set_faceanimation, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setFatigue", set_fatigue, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setFlagOwner", set_flag_owner, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("setFlagSide", set_flag_side, LUA_TO_OBJECT, LUA_TO_SIDE);
REG_SQF_CMD_2_VOID("setFuel", set_fuel, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setFuelCargo", set_fuel_cargo, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setFuelConsumptionCoef", set_fuel_consumption_coef, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("setGroupIconsSelectable", set_group_icons_selectable, LUA_TO_BOOL);
REG_SQF_CMD_2("setGroupOwner", set_group_owner, LUA_TO_GROUP, LUA_TO_INT);
REG_SQF_CMD_2_VOID("setGusts", set_gusts, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("setHorizonParallaxCoef", set_horizon_parallax_coef, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("setHumidity", set_humidity, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setImportance", set_importance, LUA_TO_LOCATION, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setLeader", set_leader, LUA_TO_TEAM_MEMBER, LUA_TO_TEAM_MEMBER);
REG_SQF_CMD_2_VOID("setLightBrightness", set_light_brightness, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setLightDayLight", set_light_day_light, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setLightFlareMaxDistance", set_light_flare_max_distance, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setLightFlareSize", set_light_flare_size, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setLightIntensity", set_light_intensity, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setLightIR", set_light_ir, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setLightnings", set_lightnings, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setLightUseFlare", set_light_use_flare, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setLocalWindParams", set_local_wind_params, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setMaxLoad", set_max_load, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setMissileTargetPos", set_missile_target_pos, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setObjectScale", set_object_scale, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setOvercast", set_overcast, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setOwner", set_owner, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setOxygenRemaining", set_oxygen_remaining, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setPhysicsCollisionFlag", set_physics_collision_flag, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2("setPilotCameraOpticsMode", set_pilot_camera_optics_mode, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setPilotLight", set_pilot_light, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setPitch", set_pitch, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("setPlayable", set_playable, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("setPlayerRespawnTime", set_player_respawn_time, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setPlayerVonVolume", set_player_von_volume, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setPos", set_pos, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setPosASL", set_pos_asl, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setPosASL2", set_pos_asl2, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setPosASLW", set_pos_aslw, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setPosATL", set_pos_atl, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setPosition", set_position, LUA_TO_LOCATION, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setPosWorld", set_pos_world, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setRainbow", set_rainbow, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setRandomLip", set_random_lip, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setRectangular", set_rectangular, LUA_TO_LOCATION, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setRepairCargo", set_repair_cargo, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setRotorBrakeRTD", set_rotor_brake_rtd, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("setShadowDistance", set_shadow_distance, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setSide", set_side, LUA_TO_LOCATION, LUA_TO_SIDE);
REG_SQF_CMD_2_VOID("setSimpleTaskAlwaysVisible", set_simple_task_always_visible, LUA_TO_TASK, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setSimpleTaskDestination", set_simple_task_destination, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_1_VOID("setSimulWeatherLayers", set_simul_weather_layers, LUA_TO_FLOAT);
REG_SQF_CMD_2("setSlingLoad", set_sling_load, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("setStamina", set_stamina, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setSuppression", set_suppression, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("setSystemOfUnits", set_system_of_units, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setTaskMarkerOffset", set_task_marker_offset, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_1_VOID("setTerrainGrid", set_terrain_grid, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setTIParameter", set_ti_parameter, LUA_TO_STRING, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("setTimeMultiplier", set_time_multiplier, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setTowParent", set_tow_parent, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("setTrafficDistance", set_traffic_distance, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setUnconscious", set_unconscious, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_2_VOID("setUnitAbility", set_unit_ability, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setUnitFreefallHeight", set_unit_freefall_height, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setUnitRecoilCoefficient", set_unit_recoil_coefficient, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setVectorDir", set_vector_dir, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setVectorUp", set_vector_up, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setVehicleAmmo", set_vehicle_ammo, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setVehicleAmmoDef", set_vehicle_ammo_def, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setVehicleArmor", set_vehicle_armor, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2("setVehicleCargo", set_vehicle_cargo, LUA_TO_OBJECT, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("setVehicleId", set_vehicle_id, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setVelocity", set_velocity, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("setVelocityModelSpace", set_velocity_model_space, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_1_VOID("setViewDistance", set_view_distance, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setWaterFillPercentage", set_water_fill_percentage, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setWaterLeakiness", set_water_leakiness, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setWaves", set_waves, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setWindForce", set_wind_force, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("setWindStr", set_wind_str, LUA_TO_FLOAT, LUA_TO_FLOAT);
REG_SQF_CMD_1_VOID("showChat", show_chat, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("showCuratorCompass", show_curator_compass, LUA_TO_BOOL);
REG_SQF_CMD_0("shownChat", shown_chat);
REG_SQF_CMD_0("shownCuratorCompass", shown_curatorcompass);
REG_SQF_CMD_0("shownSubtitles", shown_subtitles);
REG_SQF_CMD_0("sideAmbientLife", side_ambient_life);
REG_SQF_CMD_0("sideEmpty", side_empty);
REG_SQF_CMD_0("sideEnemy", side_enemy);
REG_SQF_CMD_0("sideFriendly", side_friendly);
REG_SQF_CMD_1("sideGet", side_get, LUA_TO_GROUP);
REG_SQF_CMD_0("sideLogic", side_logic);
REG_SQF_CMD_0("sideUnknown", side_unknown);
REG_SQF_CMD_1("simulationEnabled", simulation_enabled, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("simulCloudDensity", simul_cloud_density, LUA_TO_VECTOR3);
REG_SQF_CMD_1("simulInClouds", simul_in_clouds, LUA_TO_VECTOR3);
REG_SQF_CMD_0_VOID("simulWeatherSync", simul_weather_sync);
REG_SQF_CMD_1("size", size, LUA_TO_LOCATION);
REG_SQF_CMD_1_VOID("skipTime", skip_time, LUA_TO_FLOAT);
REG_SQF_CMD_0("slingLoadAssistantShown", sling_load_assistant_shown);
REG_SQF_CMD_1("someAmmo", some_ammo, LUA_TO_OBJECT);
REG_SQF_CMD_0("soundVolume", sound_volume);
REG_SQF_CMD_1("speaker", speaker, LUA_TO_OBJECT);
REG_SQF_CMD_0("speechVolume", speech_volume);
REG_SQF_CMD_1("speed", speed, LUA_TO_OBJECT);
REG_SQF_CMD_1("stance", stance, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("stop", stop, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("stopEngineRTD", stop_engine_rtd, LUA_TO_OBJECT);
REG_SQF_CMD_1("stopped", stopped, LUA_TO_OBJECT);
REG_SQF_CMD_1("str", str, LUA_TO_GAME_VALUE);
REG_SQF_CMD_0("sunOrMoon", sun_or_moon);
REG_SQF_CMD_2_VOID("suppressFor", suppress_for, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_2_VOID("swimInDepth", swim_in_depth, LUA_TO_OBJECT, LUA_TO_FLOAT);
REG_SQF_CMD_0("systemOfUnits", system_of_units);
REG_SQF_CMD_1("taskAlwaysVisible", task_always_visible, LUA_TO_TASK);
REG_SQF_CMD_1("taskCompleted", task_completed, LUA_TO_TASK);
REG_SQF_CMD_1("taskDestination", task_destination, LUA_TO_TASK);
REG_SQF_CMD_1("taskMarkerOffset", task_marker_offset, LUA_TO_OBJECT);
REG_SQF_CMD_0("taskNull", task_null);
REG_SQF_CMD_1("taskParent", task_parent, LUA_TO_TASK);
REG_SQF_CMD_1("teamName", team_name, LUA_TO_TEAM_MEMBER);
REG_SQF_CMD_0("teamMemberNull", team_member_null);
REG_SQF_CMD_0_VOID("teamSwitch", team_switch);
REG_SQF_CMD_0("teamSwitchEnabled", team_switch_enabled);
REG_SQF_CMD_1("teamType", team_type, LUA_TO_TEAM_MEMBER);
REG_SQF_CMD_2("terrainIntersect", terrain_intersect, LUA_TO_VECTOR3, LUA_TO_VECTOR3);
REG_SQF_CMD_2("terrainIntersectASL", terrain_intersect_asl, LUA_TO_VECTOR3, LUA_TO_VECTOR3);
REG_SQF_CMD_2("terrainIntersectAtASL", terrain_intersect_at_asl, LUA_TO_VECTOR3, LUA_TO_VECTOR3);
REG_SQF_CMD_1("text", text, LUA_TO_LOCATION);
REG_SQF_CMD_0("time", time);
REG_SQF_CMD_0("timeMultiplier", time_multiplier);
REG_SQF_CMD_1("toUpper", to_upper, LUA_TO_STRING);
REG_SQF_CMD_2_VOID("addTorque", add_torque, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2_VOID("triggerDynamicSimulation", trigger_dynamic_simulation, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("triggerAmmo", trigger_ammo, LUA_TO_OBJECT);
REG_SQF_CMD_1("typeOf", type_of, LUA_TO_OBJECT);
REG_SQF_CMD_0("uiNamespace", ui_namespace);
REG_SQF_CMD_1("unitAimPosition", unit_aim_position, LUA_TO_OBJECT);
REG_SQF_CMD_1("unitAimPositionVisual", unit_aim_position_visual, LUA_TO_OBJECT);
REG_SQF_CMD_1("unitBackpack", unit_backpack, LUA_TO_OBJECT);
REG_SQF_CMD_1("unitCombatMode", unit_combat_mode, LUA_TO_OBJECT);
REG_SQF_CMD_1("unitIsUAV", unit_is_uav, LUA_TO_OBJECT);
REG_SQF_CMD_1("unitPos", unit_pos, LUA_TO_OBJECT);
REG_SQF_CMD_1("unitReady", unit_ready, LUA_TO_OBJECT);
REG_SQF_CMD_1("unitRecoilCoefficient", unit_recoil_coefficient, LUA_TO_OBJECT);
REG_SQF_CMD_1("uniformContainer", uniform_container, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("unassignCurator", unassign_curator, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("unassignTeam", unassign_team, LUA_TO_OBJECT);
REG_SQF_CMD_1_VOID("unassignVehicle", unassign_vehicle, LUA_TO_OBJECT);
REG_SQF_CMD_1("underwater", underwater, LUA_TO_OBJECT);
REG_SQF_CMD_2_VOID("useAudioTimeForMoves", use_audio_time_for_moves, LUA_TO_OBJECT, LUA_TO_BOOL);
REG_SQF_CMD_1_VOID("useAIOperMapObstructionTest", use_ai_oper_map_obstruction_test, LUA_TO_BOOL);
REG_SQF_CMD_0("userInputDisabled", user_input_disabled);
REG_SQF_CMD_1("vehicle", vehicle, LUA_TO_OBJECT);
REG_SQF_CMD_1("vehicleCargoEnabled", vehicle_cargo_enabled, LUA_TO_OBJECT);
REG_SQF_CMD_1("vehicleReceiveRemoteTargets", vehicle_receive_remote_targets, LUA_TO_OBJECT);
REG_SQF_CMD_1("vehicleReportOwnPosition", vehicle_report_own_position, LUA_TO_OBJECT);
REG_SQF_CMD_1("vehicleReportRemoteTargets", vehicle_report_remote_targets, LUA_TO_OBJECT);
REG_SQF_CMD_1("vehicleVarName", vehicle_var_name, LUA_TO_OBJECT);
REG_SQF_CMD_1("velocity", velocity, LUA_TO_OBJECT);
REG_SQF_CMD_1("velocityModelSpace", velocity_model_space, LUA_TO_OBJECT);
REG_SQF_CMD_1("vestContainer", vest_container, LUA_TO_OBJECT);
REG_SQF_CMD_1("vectorDir", vector_dir, LUA_TO_OBJECT);
REG_SQF_CMD_1("vectorDirVisual", vector_dir_visual, LUA_TO_OBJECT);
REG_SQF_CMD_2("vectorModelToWorld", vector_model_to_world, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2("vectorModelToWorldVisual", vector_model_to_world_visual, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_1("vectorSide", vector_side, LUA_TO_OBJECT);
REG_SQF_CMD_1("vectorSideVisual", vector_side_visual, LUA_TO_OBJECT);
REG_SQF_CMD_1("vectorUp", vector_up, LUA_TO_OBJECT);
REG_SQF_CMD_1("vectorUpVisual", vector_up_visual, LUA_TO_OBJECT);
REG_SQF_CMD_2("vectorWorldToModel", vector_world_to_model, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2("vectorWorldToModelVisual", vector_world_to_model_visual, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_0("viewDistance", view_distance);
REG_SQF_CMD_1("visiblePosition", visible_position, LUA_TO_OBJECT);
REG_SQF_CMD_1("visiblePositionASL", visible_position_asl, LUA_TO_OBJECT);
REG_SQF_CMD_1("waterDamaged", water_damaged, LUA_TO_OBJECT);
REG_SQF_CMD_0("waves", waves);
REG_SQF_CMD_1("weaponInertia", weapon_inertia, LUA_TO_OBJECT);
REG_SQF_CMD_1("weaponLowered", weapon_lowered, LUA_TO_OBJECT);
REG_SQF_CMD_0("west", west);
REG_SQF_CMD_0("wind", wind);
REG_SQF_CMD_0("windDir", wind_dir);
REG_SQF_CMD_0("windStr", wind_str);
REG_SQF_CMD_2("worldToModel", world_to_model, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_2("worldToModelVisual", world_to_model_visual, LUA_TO_OBJECT, LUA_TO_VECTOR3);
REG_SQF_CMD_0("worldSize", world_size);

sqf_table["call"] = [](sol::object code_obj, sol::optional<sol::object> args) -> sol::object {
    code compiled;

    if (code_obj.get_type() == sol::type::userdata) {
        sol::optional<GameValueWrapper> wrapper = code_obj.as<sol::optional<GameValueWrapper>>();
        
        if (wrapper && wrapper->value.type_enum() == game_data_type::CODE) {
            compiled = wrapper->value;
            
            if (!args) {
                return GV_TO_LUA(sqf::call2(compiled));
            } else {
                return GV_TO_LUA(sqf::call2(compiled, LUA_TO_GAME_VALUE(*args)));
            }
        }
    }
    
    std::string code_or_func_str = code_obj.as<std::string>();
    std::string key;

    if (!args) {
        key = code_or_func_str;
    } else {
        key = code_or_func_str + "_";
    }                   
    
    if (code_or_func_str.find(' ') == std::string::npos && code_or_func_str.find(';') == std::string::npos) {
        if (!args) {
            auto cache_it = g_sqf_function_cache.find(key);
            
            if (cache_it != g_sqf_function_cache.end()) {
                compiled = cache_it->second;
            } else {
                compiled = sqf::compile("setReturnValue (call " + code_or_func_str + ");");
                g_sqf_function_cache[key] = compiled;
            }
            
            return GV_TO_LUA(raw_call_sqf_native(compiled));
        } else {
            auto cache_it = g_sqf_function_cache.find(key);
            
            if (cache_it != g_sqf_function_cache.end()) {
                compiled = cache_it->second;
            } else {
                compiled = sqf::compile("setReturnValue (getCallArguments call " + code_or_func_str + ");");
                g_sqf_function_cache[key] = compiled;
            }
            
            return GV_TO_LUA(raw_call_sqf_args_native(compiled, LUA_TO_GAME_VALUE(*args)));
        }
    } else {
        if (!args) {
            auto cache_it = g_sqf_function_cache.find(key);
            
            if (cache_it != g_sqf_function_cache.end()) {
                compiled = cache_it->second;
            } else {
                compiled = sqf::compile("setReturnValue (call {" + code_or_func_str + "});");
                g_sqf_function_cache[key] = compiled;
            }
            
            return GV_TO_LUA(raw_call_sqf_native(compiled));
        } else {
            auto cache_it = g_sqf_function_cache.find(key);
            
            if (cache_it != g_sqf_function_cache.end()) {
                compiled = cache_it->second;
            } else {
                compiled = sqf::compile("setReturnValue (getCallArguments call {" + code_or_func_str + "});");
                g_sqf_function_cache[key] = compiled;
            }
            
            return GV_TO_LUA(raw_call_sqf_args_native(compiled, LUA_TO_GAME_VALUE(*args)));
        }
    }
    
    return sol::nil;
};

sqf_table["combatBehaviour"] = [](sol::object target) -> sol::object {
    if (TYPE_OF(target) == GDT_OBJECT) {
        return GV_TO_LUA(combat_behaviour(LUA_TO_OBJECT(target)));
    } else {
        return GV_TO_LUA(combat_behaviour(LUA_TO_GROUP(target)));
    }
};

sqf_table["commandSuppressiveFire"] = [](sol::object unit, sol::object target) -> sol::object {
    if (TYPE_OF(target) == GDT_OBJECT) {
        command_suppressive_fire(LUA_TO_OBJECT(unit), LUA_TO_OBJECT(target));
    } else {
        command_suppressive_fire(LUA_TO_OBJECT(unit), LUA_TO_VECTOR3(target));
    }

    return sol::nil;
};

sqf_table["createGroup"] = [](sol::object arg_side, sol::optional<sol::object> delete_when_empty) -> sol::object {
    if (!delete_when_empty) {
        return GV_TO_LUA(create_group(LUA_TO_SIDE(arg_side)));
    } else {
        return GV_TO_LUA(create_group(LUA_TO_SIDE(arg_side), LUA_TO_BOOL(*delete_when_empty)));
    }
};

sqf_table["createVehicleCrew"] = [](sol::object p1, sol::optional<sol::object> p2) -> sol::object {
    if (!p2) {
        return GV_TO_LUA(create_vehicle_crew(LUA_TO_OBJECT(p1)));
    } else {
        if (TYPE_OF(p1) == GDT_GROUP) {
            return GV_TO_LUA(create_vehicle_crew(LUA_TO_GROUP(p1), LUA_TO_OBJECT(*p2)));
        } else {
            return GV_TO_LUA(create_vehicle_crew(LUA_TO_SIDE(p1), LUA_TO_OBJECT(*p2)));
        }
    }
};

sqf_table["dynamicSimulationEnabled"] = [](sol::object entity) -> sol::object {
    if (TYPE_OF(entity) == GDT_OBJECT) {
        return GV_TO_LUA(dynamic_simulation_enabled(LUA_TO_OBJECT(entity)));
    } else {
        return GV_TO_LUA(dynamic_simulation_enabled(LUA_TO_GROUP(entity)));
    }
};

sqf_table["enableDynamicSimulation"] = [](sol::object entity, sol::object enable) -> sol::object {
    if (TYPE_OF(entity) == GDT_OBJECT) {
        enable_dynamic_simulation(LUA_TO_OBJECT(entity), LUA_TO_BOOL(enable));
    } else {
        enable_dynamic_simulation(LUA_TO_GROUP(entity), LUA_TO_BOOL(enable));
    }

    return sol::nil;
};

sqf_table["forgetTarget"] = [](sol::object source, sol::object target) -> sol::object {
    if (TYPE_OF(source) == GDT_OBJECT) {
        forget_target(LUA_TO_OBJECT(source), LUA_TO_OBJECT(target));
    } else {
        forget_target(LUA_TO_GROUP(source), LUA_TO_OBJECT(target));
    }

    return sol::nil;
};

sqf_table["formation"] = [](sol::object target) -> sol::object {
    switch (TYPE_OF(target)) {
        case GDT_OBJECT:
            return GV_TO_LUA(formation(LUA_TO_OBJECT(target)));
        case GDT_GROUP:
            return GV_TO_LUA(formation(LUA_TO_GROUP(target)));
        case GDT_TEAM_MEMBER:
            return GV_TO_LUA(formation(LUA_TO_TEAM_MEMBER(target)));
    }

    return sol::nil;
};

sqf_table["getRelDir"] = [](sol::object p1, sol::object p2) -> sol::object {
    if (TYPE_OF(p2) == GDT_OBJECT) {
        return GV_TO_LUA(get_rel_dir(LUA_TO_OBJECT(p1), LUA_TO_OBJECT(p2)));
    } else {
        return GV_TO_LUA(get_rel_dir(LUA_TO_OBJECT(p1), LUA_TO_VECTOR3(p2)));
    }
};

sqf_table["getTerrainHeightASL"] = [](sol::object pos) -> sol::object {
    game_value pos_gv = LUA_TO_GAME_VALUE(pos);

    if (pos_gv.size() == 2) {
        return GV_TO_LUA(get_terrain_height_asl(CAST_VECTOR2(pos_gv)));
    } else {
        return GV_TO_LUA(get_terrain_height_asl(CAST_VECTOR3(pos_gv)));
    }
};

sqf_table["getUnitLoadout"] = [](sol::object source) -> sol::object {
    switch (TYPE_OF(source)) {
        case GDT_OBJECT:
            return GV_TO_LUA(get_unit_loadout(LUA_TO_OBJECT(source)));
        case GDT_CONFIG:
            return GV_TO_LUA(get_unit_loadout(LUA_TO_CONFIG(source)));
        case GDT_STRING:
            return GV_TO_LUA(get_unit_loadout(LUA_TO_STRING(source)));
    }

    return sol::nil;
};

sqf_table["groupId"] = [](sol::object entity) -> sol::object {
    if (TYPE_OF(entity) == GDT_OBJECT) {
        return GV_TO_LUA(group_id(LUA_TO_OBJECT(entity)));
    } else {
        return GV_TO_LUA(group_id(LUA_TO_GROUP(entity)));
    }
};

sqf_table["hideObject"] = [](sol::object obj, sol::optional<sol::object> hide) -> sol::object {
    if (!hide) {
        hide_object(LUA_TO_OBJECT(obj));
    } else {
        hide_object(LUA_TO_OBJECT(obj), LUA_TO_BOOL(*hide));
    }

    return sol::nil;
};

sqf_table["hideObjectGlobal"] = [](sol::object obj, sol::optional<sol::object> hide) -> sol::object {
    if (!hide) {
        hide_object_global(LUA_TO_OBJECT(obj));
    } else {
        hide_object_global(LUA_TO_OBJECT(obj), LUA_TO_BOOL(*hide));
    }
    
    return sol::nil;
};

sqf_table["isNull"] = [](sol::object obj) -> sol::object {
    switch (TYPE_OF(obj)) {
        case GDT_OBJECT:
            return GV_TO_LUA(is_null(LUA_TO_OBJECT(obj)));
        case GDT_CONTROL:
            return GV_TO_LUA(is_null(LUA_TO_CONTROL(obj)));
        case GDT_DISPLAY:
            return GV_TO_LUA(is_null(LUA_TO_DISPLAY(obj)));
        case GDT_SCRIPT:
            return GV_TO_LUA(is_null(LUA_TO_SCRIPT(obj)));
        case GDT_TASK:
            return GV_TO_LUA(is_null(LUA_TO_TASK(obj)));
        case GDT_CONFIG:
            return GV_TO_LUA(is_null(LUA_TO_CONFIG(obj)));
        case GDT_GROUP:
            return GV_TO_LUA(is_null(LUA_TO_GROUP(obj)));
        case GDT_LOCATION:
            return GV_TO_LUA(is_null(LUA_TO_LOCATION(obj)));
    }

    return sol::nil;
};

sqf_table["isOnRoad"] = [](sol::object pos) -> sol::object {
    if (TYPE_OF(pos) == GDT_OBJECT) {
        return GV_TO_LUA(is_on_road(LUA_TO_OBJECT(pos)));
    } else {
        return GV_TO_LUA(is_on_road(LUA_TO_VECTOR3(pos)));
    }
};

sqf_table["isWeaponDeployed"] = [](sol::object obj, sol::optional<sol::object> on_ground) -> sol::object {
    if (!on_ground) {
        return GV_TO_LUA(is_weapon_deployed(LUA_TO_OBJECT(obj)));
    } else {
        return GV_TO_LUA(is_weapon_deployed(LUA_TO_OBJECT(obj), LUA_TO_BOOL(*on_ground)));
    }
};

sqf_table["knowsAbout"] = [](sol::object source, sol::object target) -> sol::object {
    switch (TYPE_OF(source)) {
        case GDT_OBJECT:
            return GV_TO_LUA(knows_about(LUA_TO_OBJECT(source), LUA_TO_OBJECT(target)));
        case GDT_GROUP:
            return GV_TO_LUA(knows_about(LUA_TO_GROUP(source), LUA_TO_OBJECT(target)));
        case GDT_SIDE:
            return GV_TO_LUA(knows_about(LUA_TO_SIDE(source), LUA_TO_OBJECT(target)));
    }

    return sol::nil;
};

sqf_table["leader"] = [](sol::object entity) -> sol::object {
    switch (TYPE_OF(entity)) {
        case GDT_GROUP:
            return GV_TO_LUA(leader(LUA_TO_GROUP(entity)));
        case GDT_TEAM_MEMBER:
            return GV_TO_LUA(leader(LUA_TO_TEAM_MEMBER(entity)));
        case GDT_OBJECT:
            return GV_TO_LUA(leader(LUA_TO_OBJECT(entity)));
    }

    return sol::nil;
};

sqf_table["local"] = [](sol::object entity) -> sol::object {
    if (TYPE_OF(entity) == GDT_OBJECT) {
        return GV_TO_LUA(local(LUA_TO_OBJECT(entity)));
    } else {
        return GV_TO_LUA(local(LUA_TO_GROUP(entity)));
    }
};

sqf_table["lock"] = [](sol::object obj, sol::object value) -> sol::object {
    if (value.is<bool>()) {
        lock(LUA_TO_OBJECT(obj), LUA_TO_BOOL(value));
    } else {
        lock(LUA_TO_OBJECT(obj), LUA_TO_FLOAT(value));
    }

    return sol::nil;
};

sqf_table["move"] = [](sol::object entity, sol::object pos) -> sol::object {
    if (TYPE_OF(entity) == GDT_OBJECT) {
        move(LUA_TO_OBJECT(entity), LUA_TO_VECTOR3(pos));
    } else {
        move(LUA_TO_GROUP(entity), LUA_TO_VECTOR3(pos));
    }

    return sol::nil;
};

sqf_table["moveOut"] = [](sol::object p1, sol::optional<sol::object> p2) -> sol::object {
    if (!p2) {
        move_out(LUA_TO_OBJECT(p1));
    } else {
        move_out(LUA_TO_OBJECT(p1), LUA_TO_OBJECT(*p2));
    }

    return sol::nil;
};

sqf_table["screenToWorld"] = [](sol::object p1, sol::optional<sol::object> p2) -> sol::object {
    if (!p2) {
        return GV_TO_LUA(screen_to_world(LUA_TO_VECTOR2(p1)));
    } else {
        return GV_TO_LUA(screen_to_world(LUA_TO_OBJECT(p1), LUA_TO_VECTOR2(*p2)));
    }
};

sqf_table["screenToWorldDirection"] = [](sol::object p1, sol::optional<sol::object> p2) -> sol::object {
    if (!p2) {
        return GV_TO_LUA(screen_to_world_direction(LUA_TO_VECTOR2(p1)));
    } else {
        return GV_TO_LUA(screen_to_world_direction(LUA_TO_OBJECT(p1), LUA_TO_VECTOR2(*p2)));
    }
};

sqf_table["setAirportSide"] = [](sol::object p1, sol::object side_arg) -> sol::object {
    if (p1.is<float>() || p1.is<int>()) {
        set_airport_side(LUA_TO_FLOAT(p1), LUA_TO_SIDE(side_arg));
    } else {
        set_airport_side(LUA_TO_OBJECT(p1), LUA_TO_SIDE(side_arg));
    }

    return sol::nil;
};

sqf_table["setCaptive"] = [](sol::object obj, sol::object status) -> sol::object {
    if (status.is<bool>()) {
        set_captive(LUA_TO_OBJECT(obj), LUA_TO_BOOL(status));
    } else {
        set_captive(LUA_TO_OBJECT(obj), LUA_TO_FLOAT(status));
    }

    return sol::nil;
};

sqf_table["setObjectViewDistance"] = [](sol::object distance, sol::optional<sol::object> shadow) -> sol::object {
    if (!shadow) {
        set_object_view_distance(LUA_TO_FLOAT(distance));
    } else {
        set_object_view_distance(LUA_TO_FLOAT(distance), LUA_TO_FLOAT(*shadow));
    }

    return sol::nil;
};

sqf_table["speedMode"] = [](sol::object target) -> sol::object {
    if (TYPE_OF(target) == GDT_OBJECT) {
        return GV_TO_LUA(speed_mode(LUA_TO_OBJECT(target)));
    } else {
        return GV_TO_LUA(speed_mode(LUA_TO_GROUP(target)));
    }
};

sqf_table["surfaceIsWater"] = [](sol::object pos) -> sol::object {
    game_value pos_gv = LUA_TO_GAME_VALUE(pos);

    if (pos_gv.size() == 2) {
        return GV_TO_LUA(surface_is_water(CAST_VECTOR2(pos_gv)));
    } else {
        return GV_TO_LUA(surface_is_water(CAST_VECTOR3(pos_gv)));
    }
};

sqf_table["surfaceNormal"] = [](sol::object pos) -> sol::object {
    game_value pos_gv = LUA_TO_GAME_VALUE(pos);

    if (pos_gv.size() == 2) {
        return GV_TO_LUA(surface_normal(CAST_VECTOR2(pos_gv)));
    } else {
        return GV_TO_LUA(surface_normal(CAST_VECTOR3(pos_gv)));
    }
};

sqf_table["useAISteeringComponent"] = [](sol::object p1, sol::optional<sol::object> p2) -> sol::object {
    if (!p2) {
        use_ai_steering_component(LUA_TO_BOOL(p1));
    } else {
        use_ai_steering_component(LUA_TO_OBJECT(p1), LUA_TO_BOOL(*p2));
    }

    return sol::nil;
};