#pragma once

using namespace intercept;
using namespace intercept::types;

static registered_sqf_function _sqf_execute_lua_any_array;
static registered_sqf_function _sqf_execute_lua_any_string;
static registered_sqf_function _sqf_execute_lua_any_code;
static registered_sqf_function _sqf_execute_lua_string;
static registered_sqf_function _sqf_execute_lua_array;
static registered_sqf_function _sqf_execute_lua_code;
static registered_sqf_function _sqf_compile_lua_string_string;
static registered_sqf_function _sqf_compile_lua_string_code;
static registered_sqf_function _sqf_crypto_hash_string_string;
static registered_sqf_function _sqf_generate_uid;
static registered_sqf_function _sqf_get_epoch;
static registered_sqf_function _sqf_get_epoch_delta;
static registered_sqf_function _sqf_write_khdata_string_array;
static registered_sqf_function _sqf_read_khdata_string;
static registered_sqf_function _sqf_read_khdata_string_array;
static registered_sqf_function _sqf_read_khdata_string_string;
static registered_sqf_function _sqf_flush_khdata;
static registered_sqf_function _sqf_delete_khdata_file_string;
static registered_sqf_function _sqf_get_terrain_matrix;
static registered_sqf_function _sqf_emit_lua_variable_string;
static registered_sqf_function _sqf_emit_lua_variable_array;
static registered_sqf_function _sqf_lua_set_variable_array;
static registered_sqf_function _sqf_lua_get_variable_array;
static registered_sqf_function _sqf_lua_get_variable_string;
static registered_sqf_function _sqf_execute_any_code;
static registered_sqf_function _sqf_execute_any_array;
static registered_sqf_function _sqf_execute_any_string;
static registered_sqf_function _sqf_execute_code;
static registered_sqf_function _sqf_execute_string;
static registered_sqf_function _sqf_execute_array;
static registered_sqf_function _sqf_remove_handler_array;
static registered_sqf_function _sqf_generate_random_string_scalar;
static registered_sqf_function _sqf_generate_random_string_array_scalar;
static registered_sqf_function _sqf_trigger_lua_event_string;
static registered_sqf_function _sqf_trigger_lua_event_array;
static registered_sqf_function _sqf_trigger_lua_event_any_array;
static registered_sqf_function _sqf_trigger_lua_event_any_string;
static registered_sqf_function _sqf_set_return_value;
static registered_sqf_function _sqf_get_return_value;
static registered_sqf_function _sqf_set_call_arguments;
static registered_sqf_function _sqf_get_call_arguments;
static registered_sqf_function _sqf_get_rotation_euler_object;
static registered_sqf_function _sqf_get_rotation_euler_object_object;
static registered_sqf_function _sqf_set_rotation_euler;
static registered_sqf_function _sqf_vector_to_euler;
static registered_sqf_function _sqf_euler_to_vector;
static registered_sqf_function _sqf_euler_to_quaternion;
static registered_sqf_function _sqf_quaternion_to_euler;
static registered_sqf_function _sqf_vector_to_quaternion;
static registered_sqf_function _sqf_quaternion_to_vector;
static registered_sqf_function _sqf_quaternion_slerp;
static registered_sqf_function _sqf_quaternion_multiply;
static registered_sqf_function _sqf_get_rotation_quaternion_object;
static registered_sqf_function _sqf_get_rotation_quaternion_object_object;
static registered_sqf_function _sqf_set_rotation_quaternion;
static registered_sqf_function _sqf_axis_angle_to_quaternion;
static registered_sqf_function _sqf_quaternion_to_axis_angle;
static registered_sqf_function _sqf_initialize_ai;
static registered_sqf_function _sqf_stop_ai;
static registered_sqf_function _sqf_stop_all_ai;
static registered_sqf_function _sqf_is_ai_active;
static registered_sqf_function _sqf_is_ai_generating;
static registered_sqf_function _sqf_get_active_ai;
static registered_sqf_function _sqf_set_ai_model_string_string;
static registered_sqf_function _sqf_set_ai_model_string;
static registered_sqf_function _sqf_update_ai_system_prompt;
static registered_sqf_function _sqf_update_ai_master_prompt;
static registered_sqf_function _sqf_update_ai_user_prompt;
static registered_sqf_function _sqf_set_ai_parameters;
static registered_sqf_function _sqf_trigger_ai_inference;
static registered_sqf_function _sqf_set_ai_markers;
static registered_sqf_function _sqf_abort_ai_generation;
static registered_sqf_function _sqf_log_ai_generation;
static registered_sqf_function _sqf_reset_ai_context;
static registered_sqf_function _sqf_tts_load_model_string;
static registered_sqf_function _sqf_tts_load_model_string_array;
static registered_sqf_function _sqf_tts_speak;
static registered_sqf_function _sqf_tts_update_speaker;
static registered_sqf_function _sqf_tts_stop_speaker;
static registered_sqf_function _sqf_tts_is_playing;
static registered_sqf_function _sqf_tts_stop_all;
static registered_sqf_function _sqf_tts_is_initialized;
static registered_sqf_function _sqf_stt_load_model_string;
static registered_sqf_function _sqf_stt_load_model_string_array;
static registered_sqf_function _sqf_stt_is_initialized;
static registered_sqf_function _sqf_stt_is_capturing;
static registered_sqf_function _sqf_stt_start_capture;
static registered_sqf_function _sqf_stt_stop_capture;
static registered_sqf_function _sqf_html_create;
static registered_sqf_function _sqf_html_open;
static registered_sqf_function _sqf_html_close;
static registered_sqf_function _sqf_html_set_visible;
static registered_sqf_function _sqf_html_get_open;
static registered_sqf_function _sqf_html_is_initialized;
static registered_sqf_function _sqf_html_execute_js;
static registered_sqf_function _sqf_html_set_js_variable;
static registered_sqf_function _sqf_html_get_js_variable;
static registered_sqf_function _sqf_html_set_position;
static registered_sqf_function _sqf_html_set_opacity;
static registered_sqf_function _sqf_html_set_size;
static registered_sqf_function _sqf_html_set_z_order;
static registered_sqf_function _sqf_html_bring_to_front;
static registered_sqf_function _sqf_html_send_to_back;
static registered_sqf_function _sqf_html_reload;
static registered_sqf_function _sqf_kh_network_message_send_any_array;
static registered_sqf_function _sqf_kh_network_message_send_array;
static registered_sqf_function _sqf_kh_network_remove_jip;
static registered_sqf_function _sqf_kh_network_message_receive_string_array;
static registered_sqf_function _sqf_kh_network_message_receive_string_code;
static registered_sqf_function _sqf_kh_network_remove_handler;
static registered_sqf_function _sqf_kh_network_is_initialized;
static registered_sqf_function _sqf_kh_network_initialize;
static registered_sqf_function _sqf_kh_network_shutdown;
static registered_sqf_function _sqf_enable_network_logging;
static registered_sqf_function _sqf_kh_set_variable_namespace_array;
static registered_sqf_function _sqf_kh_set_variable_object_array;
static registered_sqf_function _sqf_kh_set_variable_group_array;
static registered_sqf_function _sqf_kh_set_variable_location_array;
static registered_sqf_function _sqf_kh_set_variable_display_array;
static registered_sqf_function _sqf_ts_connect;
static registered_sqf_function _sqf_ts_disconnect;
static registered_sqf_function _sqf_ts_apply_voice_effects;
static registered_sqf_function _sqf_ts_clear_voice_effects;
static registered_sqf_function _sqf_ts_is_initialized;
static registered_sqf_function _sqf_ts_is_plugin_active;
static registered_sqf_function _sqf_ts_is_connected;
static registered_sqf_function _sqf_ts_is_plugin_installed;
static registered_sqf_function _sqf_serialize_function_code;
static registered_sqf_function _sqf_serialize_function_string;
static registered_sqf_function _sqf_serialize_function_bool_code;
static registered_sqf_function _sqf_serialize_function_bool_string;
static registered_sqf_function _sqf_call_serialized_function;
static registered_sqf_function _sqf_curve_conversion;
static registered_sqf_function _sqf_inverse_curve_conversion;
static registered_sqf_function _sqf_vector_curve_conversion;
static registered_sqf_function _sqf_inverse_vector_curve_conversion;
static registered_sqf_function _sqf_curve_slope;
static registered_sqf_function _sqf_vector_curve_slope;
static registered_sqf_function _sqf_get_unit_yaw_speed;
static registered_sqf_function _sqf_process_execution;
static registered_sqf_function _sqf_trigger_cba_event_array;
static registered_sqf_function _sqf_process_cba_group_event;
static registered_sqf_function _sqf_process_cba_array_event;
static registered_sqf_function _sqf_process_cba_code_event;
static registered_sqf_function _sqf_sample_scene_depth_array;
static registered_sqf_function _sqf_gpu_visibility_array;
static registered_sqf_function _sqf_remove_render_handler_string;
static registered_sqf_function _sqf_queue_visibility_array;
static registered_sqf_function _sqf_get_visibility_results;
static registered_sqf_function _sqf_add_render3d_array;
static registered_sqf_function _sqf_update_render3d_array;
static registered_sqf_function _sqf_update_post_fx_array;
static registered_sqf_function _sqf_add_postfx_array;
static registered_sqf_function _sqf_add_local_postfx_array;
static registered_sqf_function _sqf_get_render_stats;
static registered_sqf_function _sqf_set_render_debug;
static registered_sqf_function _sqf_reset_render_stats;
static registered_sqf_function _sqf_set_ssgi_scale;
static registered_sqf_function _sqf_flush_ui_render;
static registered_sqf_function _sqf_dump_render_trace;
static registered_sqf_function _sqf_dump_dynamic_lights;

struct kh_command_variant {
    std::string source;
    code compiled;
    bool exists = false;
    bool compiled_ready = false;
};

struct kh_command_entry {
    std::string canonical_name;   // as reported by supportInfo, for error text
    kh_command_variant call_variant[3];
    kh_command_variant spawn_variant[3];
};

static std::unordered_map<std::string, kh_command_entry> g_sqf_command_map;
static bool g_sqf_command_map_initialized = false;
static const std::unordered_set<std::string> g_sqf_command_map_skip = {};

static std::string kh_lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return value;
}

// Only plain identifiers are callable by name; this filters out the symbolic
// operators supportInfo also reports (+, -, ==, #, !, &&,...), which would
// otherwise produce compile errors in the RPT for entries nobody can call.
static bool kh_command_name_is_callable(const std::string& name) {
    if (name.empty() || name.size() > 64) {
        return false;
    }

    const unsigned char first = static_cast<unsigned char>(name[0]);

    if (!std::isalpha(first) && first != '_') {
        return false;
    }

    for (const char raw : name) {
        const unsigned char c = static_cast<unsigned char>(raw);

        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }

    return true;
}

static void kh_register_command_variant(const std::string& name, int arity) {
    if (arity < 0 || arity > 2 || !kh_command_name_is_callable(name)) {
        return;
    }

    const std::string key = kh_lower_copy(name);

    if (g_sqf_command_map_skip.find(key) != g_sqf_command_map_skip.end()) {
        return;
    }

    kh_command_entry& entry = g_sqf_command_map[key];

    if (entry.canonical_name.empty()) {
        entry.canonical_name = name;
    }

    if (entry.call_variant[arity].exists) {
        return;   // supportInfo repeats each command once per type combination
    }

    std::string invocation;
    std::string spawn_invocation;

    switch (arity) {
        case 0:
            invocation = name;
            spawn_invocation = name;
            break;
        case 1:
            invocation = name + " _khRightArgument";
            spawn_invocation = name + " (_this select 0)";
            break;
        default:
            invocation = "_khLeftArgument " + name + " _khRightArgument";
            spawn_invocation = "(_this select 0) " + name + " (_this select 1)";
            break;
    }

    entry.call_variant[arity].exists = true;
    entry.call_variant[arity].source = "setReturnValue (" + invocation + ");";
    entry.spawn_variant[arity].exists = true;
    entry.spawn_variant[arity].source = spawn_invocation + ";";
}

// supportInfo entries look like: "n:time", "u:count ARRAY", "b:ARRAY select
// SCALAR", "t:OBJECT"
static void kh_parse_support_info_entry(const std::string& raw) {
    if (raw.size() < 3 || raw[1] != ':') {
        return;
    }

    const char kind = static_cast<char>(std::tolower(static_cast<unsigned char>(raw[0])));

    if (kind != 'n' && kind != 'u' && kind != 'b') {
        return;   // 't:' type entries and anything unexpected
    }

    std::vector<std::string> tokens;
    std::string current;

    for (size_t i = 2; i < raw.size(); ++i) {
        if (raw[i] == ' ' || raw[i] == '\t') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(raw[i]);
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    switch (kind) {
        case 'n':
            if (tokens.size() == 1) {
                kh_register_command_variant(tokens[0], 0);
            }

            break;
        case 'u':
            if (tokens.size() == 2) {
                kh_register_command_variant(tokens[0], 1);
            }

            break;
        default:
            if (tokens.size() == 3) {
                kh_register_command_variant(tokens[1], 2);
            }

            break;
    }
}

static bool kh_ensure_variant_compiled(kh_command_variant& variant) {
    if (!variant.exists) {
        return false;
    }

    if (!variant.compiled_ready) {
        variant.compiled = sqf::compile(variant.source.c_str());
        variant.compiled_ready = true;
    }

    return true;
}

// Idempotent: the engine's command set never changes, so the table survives
// mission restarts and is only ever built once.
static void populate_sqf_command_map() {
    if (g_sqf_command_map_initialized) {
        return;
    }

    g_sqf_command_map_initialized = true;

    try {
        const auto entries = sqf::support_info("");

        if (entries.empty()) {
            sqf::diag_log("KH Framework Extension - supportInfo returned no command list");
            return;
        }

        g_sqf_command_map.reserve(entries.size() / 2 + 16);

        for (const auto& entry : entries) {
            kh_parse_support_info_entry(std::string(entry.c_str()));
        }

        size_t compiled = 0;

        for (auto& pair : g_sqf_command_map) {
            for (int arity = 0; arity < 3; ++arity) {
                if (kh_ensure_variant_compiled(pair.second.call_variant[arity])) {
                    ++compiled;
                }
            }
        }

        sqf::diag_log("KH Framework Extension - Command table: "
                      + std::to_string(g_sqf_command_map.size()) + " commands, "
                      + std::to_string(compiled) + " variants");
    } catch (const std::exception& e) {
        sqf::diag_log("KH Framework Extension - Failed to build command table: " + std::string(e.what()));
    } catch (...) {
        sqf::diag_log("KH Framework Extension - Failed to build command table");
    }
}

static kh_command_entry* kh_find_command(const std::string& name) {
    if (g_sqf_command_map.empty()) {
        return nullptr;
    }

    const auto it = g_sqf_command_map.find(kh_lower_copy(name));
    return it == g_sqf_command_map.end() ? nullptr : &it->second;
}

static std::string kh_command_arity_description(const kh_command_entry& entry) {
    std::string out;

    for (int arity = 0; arity < 3; ++arity) {
        if (!entry.call_variant[arity].exists) {
            continue;
        }

        if (!out.empty()) {
            out += ", ";
        }

        out += std::to_string(arity);
    }

    return out.empty() ? std::string("none") : out;
}

static game_value execute_lua_sqf(game_value_parameter args, game_value_parameter code_or_function) {    
    try {
        LuaStackGuard guard(*g_lua_state);

        // Check if right argument is an array. If so, use built in execute
        if (code_or_function.type_enum() == game_data_type::ARRAY) {
            auto& arr = code_or_function.to_array();

            if (arr.empty()) {
                report_error("Remote execution requires at least function");
                return game_value();
            }

            game_value lua_code = arr[0];
            game_value target_gv = arr.size() > 1 ? arr[1] : game_value();
            game_value environment_gv = arr.size() > 2 ? arr[2] : game_value();
            game_value special_gv = arr.size() > 3 ? arr[3] : game_value();            
            std::string code_str;

            if (lua_code.type_enum() == game_data_type::CODE) {
                auto code_data = lua_code.get_as<game_data_code>();
                code_str = static_cast<std::string>(code_data->code_string);
            } else if (lua_code.type_enum() == game_data_type::STRING) {
                code_str = static_cast<std::string>(lua_code);
            } else {
                report_error("Bad code type");
                return game_value();
            }

            auto_array<game_value> sqf_params;
            sqf_params.push_back(std::move(args));
            sqf_params.push_back(game_value(code_str));
            sqf_params.push_back(target_gv);
            sqf_params.push_back(environment_gv);
            sqf_params.push_back(special_gv);
            return raw_call_sqf_args_native(g_compiled_sqf_execute_lua, game_value(std::move(sqf_params)));
        }

        sol::protected_function func;
        auto* source_ptr = code_or_function.data.get();
        uintptr_t source_key = reinterpret_cast<uintptr_t>(source_ptr);
        auto exec_it = source_ptr ? g_local_exec_cache.find(source_key) : g_local_exec_cache.end();

        if (exec_it != g_local_exec_cache.end()) {
            func = exec_it->second.func;
        } else {
            std::string code_str;

            if (code_or_function.type_enum() == game_data_type::CODE) {
                auto code_data = code_or_function.get_as<game_data_code>();
                code_str = static_cast<std::string>(code_data->code_string);
            } else {
                code_str = static_cast<std::string>(code_or_function);
            }

            // Bare global function name vs. arbitrary code.
            if (code_str.find(' ') == std::string::npos && code_str.find('(') == std::string::npos) {
                auto cache_it = g_call_cache.find(code_str);

                if (cache_it != g_call_cache.end()) {
                    func = cache_it->second.func;
                } else {
                    func = (*g_lua_state)[code_str];

                    if (!func.valid()) {
                        report_error("Function '" + code_str + "' not found");
                        return game_value();
                    }

                    g_call_cache[code_str] = {code_str, func, true};
                }
            } else {
                size_t code_hash = std::hash<std::string>{}(code_str);
                auto code_it = g_code_cache.find(code_hash);

                if (code_it != g_code_cache.end()) {
                    func = code_it->second;
                } else {
                    code_str = Lua_Compilation::preprocess_lua_operators(code_str);
                    sol::load_result load_res = g_lua_state->load("return function(...) " + code_str + " end");

                    if (!load_res.valid()) {
                        sol::error err = load_res;
                        report_error(std::string(err.what()));
                        return game_value();
                    }

                    sol::protected_function factory = load_res;
                    auto factory_result = factory();

                    if (!factory_result.valid()) {
                        sol::error err = factory_result;
                        report_error("Failed to create function: " + std::string(err.what()));
                        return game_value();
                    }

                    func = factory_result;
                    g_code_cache[code_hash] = func;
                }
            }

            if (source_ptr && g_local_exec_cache.size() < LUA_LOCAL_EXEC_CACHE_MAX) {
                g_local_exec_cache[source_key] = LuaLocalExecCache{ func, code_or_function };
            }
        }

        sol::protected_function_result result;

        if (args.type_enum() == game_data_type::ARRAY) {
            auto& arr = args.to_array();

            if (arr.empty()) {
                result = func();
            } else {
                std::vector<sol::object> arg_vec;
                arg_vec.reserve(arr.size());

                for (size_t i = 0; i < arr.size(); i++) {
                    arg_vec.push_back(convert_game_value_to_lua(arr[i]));
                }

                result = func(sol::as_args(arg_vec));
            }
        } else if (args.is_nil()) {
            result = func();
        } else {
            result = func(convert_game_value_to_lua(args));
        }

        if (!result.valid()) {
            sol::error err = result;
            report_error(std::string(err.what()));
            return game_value();
        }

        if (result.return_count() == 0) {
            return game_value();
        } else {
            return convert_lua_to_game_value(result.get<sol::object>());
        }
    } catch (const sol::error& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Unknown error occurred");
        return game_value();
    }
}

static game_value compile_lua_sqf(game_value_parameter name, game_value_parameter code) {    
    try {
        LuaStackGuard guard(*g_lua_state);
        std::string lua_code;

        // Handle CODE type or STRING type
        if (code.type_enum() == game_data_type::CODE) {
            auto code_data = code.get_as<game_data_code>();
            lua_code = static_cast<std::string>(code_data->code_string);
        } else {
            lua_code = static_cast<std::string>(code);
        }

        std::string lua_name = static_cast<std::string>(name);

        // Validate the Lua name
        if (lua_name.empty()) {
            report_error("Function name cannot be empty");
            return game_value();
        }

        auto result = Lua_Compilation::lua_compile(lua_code, lua_name);

        if (result.success) {
            if (!lua_name.empty() && result.function.valid()) {
                // Update call cache
                g_call_cache[lua_name] = {lua_name, result.function, true};
            }

            return game_value();
        } else {
            report_error(result.error_message);
            return game_value();
        }

    } catch (const sol::error& e) {
        report_error("Lua compilation - " + std::string(e.what()));
        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Unknown error during compilation");
        return game_value();
    }
}

static game_value crypto_hash_sqf(game_value_parameter type, game_value_parameter input) {
    try {
        std::string hash_type = static_cast<std::string>(type);
        std::string input_str = static_cast<std::string>(input);

        // Convert to lowercase for comparison
        std::transform(hash_type.begin(), hash_type.end(), hash_type.begin(), ::tolower);

        std::string result;
        if (hash_type == "md5") {
            result = CryptoGenerator::md5(input_str);
        } else if (hash_type == "sha1") {
            result = CryptoGenerator::sha1(input_str);
        } else if (hash_type == "sha256") {
            result = CryptoGenerator::sha256(input_str);
        } else if (hash_type == "sha512") {
            result = CryptoGenerator::sha512(input_str);
        } else if (hash_type == "fnv1a32") {
            result = CryptoGenerator::fnv1a32(input_str);
        } else if (hash_type == "fnv1a64") {
            result = CryptoGenerator::fnv1a64(input_str);
        } else if (hash_type == "crc32") {
            result = CryptoGenerator::crc32(input_str);
        } else if (hash_type == "xxhash32") {
            result = CryptoGenerator::xxhash32(input_str);
        } else if (hash_type == "adler32") {
            result = CryptoGenerator::adler32(input_str);
        } else if (hash_type == "djb2") {
            result = CryptoGenerator::djb2(input_str);
        } else if (hash_type == "sdbm") {
            result = CryptoGenerator::sdbm(input_str);
        } else {
            report_error("Unknown hash type: " + hash_type);
            return game_value();
        }

        return game_value(result);
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value("");
    }
}

static game_value generate_random_string_sqf(game_value_parameter options, game_value_parameter length) {
    try {
        int len = static_cast<int>(static_cast<float>(length));

        if (len <= 0) {
            report_error("Length must be greater than 0");
            return game_value();
        }

        bool use_numbers = true;
        bool use_letters = true;
        bool use_symbols = true;
        auto& arr = options.to_array();

        if (arr.size() >= 1 && !arr[0].is_nil()) {
            use_numbers = static_cast<bool>(arr[0]);
        }

        if (arr.size() >= 2 && !arr[1].is_nil()) {
            use_letters = static_cast<bool>(arr[1]);
        }

        if (arr.size() >= 3 && !arr[2].is_nil()) {
            use_symbols = static_cast<bool>(arr[2]);
        }

        // If all are explicitly false, default to all true
        if (!use_numbers && !use_letters && !use_symbols) {
            use_numbers = true;
            use_letters = true;
            use_symbols = true;
        }

        std::string result = RandomStringGenerator::generate(len, use_numbers, use_letters, use_symbols);
        return game_value(result);
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value("");
    }
}

static game_value generate_uid_sqf() {
    try {
        std::string uid = UIDGenerator::generate();
        return game_value(uid);
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value("");
    }
}

static game_value get_epoch_sqf() noexcept {
    try {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        double seconds = microseconds / 1000000.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << seconds;
        return game_value(oss.str());
    } catch (...) {
        return game_value("0");
    }
}

static game_value get_epoch_delta_sqf(game_value_parameter past_epoch_str) noexcept {
    try {
        std::string past_str = static_cast<std::string>(past_epoch_str);
        double past_epoch = std::stod(past_str);
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        double current_epoch = microseconds / 1000000.0;
        return game_value(static_cast<float>(current_epoch - past_epoch));
    } catch (...) {
        return game_value(0.0f);
    }
}

static game_value read_khdata_sqf(game_value_parameter filename, game_value_parameter var_param) {
    try {
        std::string file_str = static_cast<std::string>(filename);
        std::transform(file_str.begin(), file_str.end(), file_str.begin(), ::tolower);
        std::string var_str;
        game_value default_value;
        bool has_default = false;

        if (var_param.type_enum() == game_data_type::STRING) {
            var_str = static_cast<std::string>(var_param);
        } else if (var_param.type_enum() == game_data_type::ARRAY) {
            auto& arr = var_param.to_array();

            if (arr.empty() || arr[0].type_enum() != game_data_type::STRING) {
                report_error("Array must contain variable name as first element");
                return game_value();
            }

            var_str = static_cast<std::string>(arr[0]);

            if (arr.size() > 1) {
                default_value = arr[1];
                has_default = true;
            }
        }

        std::transform(var_str.begin(), var_str.end(), var_str.begin(), ::tolower);        
        auto* file = KHDataManager::instance().get_or_create_file(file_str);

        if (!file) {
            return has_default ? default_value : game_value();
        }

        game_value result = file->read_variable(var_str);

        // Return default value if result is nil and default was provided
        if (result.is_nil() && has_default) {
            return default_value;
        }

        return result;
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        return game_value();
    }
}

static game_value read_khdata_unary_sqf(game_value_parameter filename) {
    try {
        std::string file_str = static_cast<std::string>(filename);
        std::transform(file_str.begin(), file_str.end(), file_str.begin(), ::tolower);
        auto* file = KHDataManager::instance().get_or_create_file(file_str);

        if (!file) {
            return game_value(auto_array<game_value>());
        }

        auto names = file->get_variable_names();
        auto_array<game_value> arr;
        arr.reserve(names.size());

        for (const auto& name : names) {
            arr.push_back(game_value(name));
        }

        return game_value(std::move(arr));
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        return game_value();
    }
}

static game_value write_khdata_sqf(game_value_parameter filename, game_value_parameter params) {
    try {
        std::string file_str = static_cast<std::string>(filename);
        std::transform(file_str.begin(), file_str.end(), file_str.begin(), ::tolower);

        if (params.size() < 2) {
            report_error("Must be an array with at least the name and value");
            return game_value();
        }

        auto& arr = params.to_array();

        if (arr[0].type_enum() != game_data_type::STRING) {
            report_error("First array element must be variable name (string)");
            return game_value();
        }

        std::string var_name = static_cast<std::string>(arr[0]);
        std::transform(var_name.begin(), var_name.end(), var_name.begin(), ::tolower);
        game_value value = arr[1];
        game_value target = arr.size() > 2 ? arr[2] : game_value();
        game_value jip = arr.size() > 3 ? arr[3] : game_value();

        if (file_str.empty() || var_name.empty()) {
            report_error("Empty file name or variable name");
            return game_value();
        }

        // If target or jip is specified, trigger CBA event
        if (!((target.is_nil() || (target.type_enum() == game_data_type::BOOL && static_cast<bool>(target))) &&
            (jip.is_nil() || (jip.type_enum() == game_data_type::BOOL && !static_cast<bool>(jip))))) {   
            // Build CBA parameters: ["KH_eve_khDataWriteEmission", [file_str,
            // var_name, value], target, jip]
            auto_array<game_value> value_array;
            value_array.push_back(file_str);
            value_array.push_back(var_name);
            value_array.push_back(value);
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_khDataWriteEmission"));
            cba_params.push_back(game_value(std::move(value_array)));
            cba_params.push_back(target);
            cba_params.push_back(jip);
            trigger_cba_event_sqf(game_value(std::move(cba_params)));
            return game_value();
        } else {
            auto* file = KHDataManager::instance().get_or_create_file(file_str);

            if (!file) {
                report_error("Failed to access file");
                return game_value();
            }

            file->write_variable(var_name, value);
        }

        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value flush_khdata_sqf() {
    try {
        int count = KHDataManager::instance().flush_all();
        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value delete_khdata_file_sqf(game_value_parameter filename) {
    try {
        std::string file_str = static_cast<std::string>(filename);
        std::transform(file_str.begin(), file_str.end(), file_str.begin(), ::tolower);

        if (KHDataManager::instance().delete_file(file_str)) {
            return game_value();
        } else {
            report_error("Failed to delete file");
            return game_value();
        }
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value get_terrain_matrix_sqf() {
    try {
        // Ensure terrain matrix is initialized for current terrain
        initialize_terrain_matrix();

        if (g_terrain_matrix.empty()) {
            return game_value();
        }

        auto_array<game_value> matrix;
        matrix.reserve(g_terrain_matrix.size());

        for (const auto& row : g_terrain_matrix) {
            auto_array<game_value> sqf_row;
            sqf_row.reserve(row.size());

            for (float height : row) {
                sqf_row.push_back(game_value(height));
            }

            matrix.push_back(game_value(std::move(sqf_row)));
        }

        return game_value(std::move(matrix));
    } catch (...) {
        report_error("Failed to retrieve terrain matrix");
        return game_value(auto_array<game_value>());
    }
}

static game_value trigger_lua_event_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string event_name;
        game_value target;
        game_value jip;
        game_value event_args = left_arg;   // Left arg is the actual arguments

        // Parse right argument
        if (right_arg.type_enum() == game_data_type::STRING) {
            event_name = static_cast<std::string>(right_arg);
            target = game_value();
            jip = game_value();
        } else {
            // Must be array
            auto& arr = right_arg.to_array();

            if (arr.empty()) {
                report_error("Array cannot be empty");
                return game_value();
            }

            if (arr[0].type_enum() != game_data_type::STRING) {
                report_error("First array element must be event name (string)");
                return game_value();
            }

            event_name = static_cast<std::string>(arr[0]);
            target = arr.size() > 1 ? arr[1] : game_value();
            jip = arr.size() > 2 ? arr[2] : game_value();
        }

        // Check if we should use CBA event
        if (!((target.is_nil() || (target.type_enum() == game_data_type::BOOL && static_cast<bool>(target))) &&
            (jip.is_nil() || (jip.type_enum() == game_data_type::BOOL && !static_cast<bool>(jip))))) {         
            auto_array<game_value> cba_event_data;
            cba_event_data.push_back(game_value(event_name));
            cba_event_data.push_back(event_args);
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_luaEventTrigger"));
            cba_params.push_back(game_value(std::move(cba_event_data)));
            cba_params.push_back(target);
            cba_params.push_back(jip);
            trigger_cba_event_sqf(game_value(std::move(cba_params)));
        } else {
            LuaStackGuard guard(*g_lua_state);

            // Convert arguments and call trigger_event with raw Lua API
            sol::state& lua = *g_lua_state;
            lua_State* L = lua.lua_state();

            // Push function
            lua_getglobal(L, "event");
            lua_getfield(L, -1, "trigger");
            lua_remove(L, -2);   // Clean up event table from stack
            lua_pushstring(L, event_name.c_str());

            // Push target (arg 2) - always true for local
            lua_pushboolean(L, true);

            // Push jip (arg 3) - always false for local
            lua_pushboolean(L, false);

            // Now push event arguments (variadic args starting from arg 4)
            int arg_count = 3;   // event_name, target, jip

            if (event_args.type_enum() == game_data_type::ARRAY) {
                auto& args_arr = event_args.to_array();

                for (const auto& elem : args_arr) {
                    convert_game_value_to_lua(elem).push(L);
                    arg_count++;
                }
            } else if (!event_args.is_nil()) {
                convert_game_value_to_lua(event_args).push(L);
                arg_count++;
            }

            // Call the function
            if (lua_pcall(L, arg_count, 1, 0) != 0) {
                std::string err = lua_tostring(L, -1);
                lua_pop(L, 1);
                report_error("Failed to trigger event: " + err);
                return game_value();
            }

            sol::object result = sol::stack::pop<sol::object>(L);
            return convert_lua_to_game_value(result);
        }

        return game_value();
    } catch (const std::exception& e) {
        report_error("Failed to trigger event: " + std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Failed to trigger event");
        return game_value();
    }
}

static game_value emit_lua_variable_sqf(game_value_parameter params) {
    try {
        LuaStackGuard guard(*g_lua_state);
        sol::state& lua = *g_lua_state;
        std::string var_name;
        game_value emit_value;
        game_value target;
        game_value jip;

        if (params.type_enum() == game_data_type::STRING) {
            // Variable name, fetch from Lua
            var_name = static_cast<std::string>(params);

            // Get value from global
            sol::object lua_var = lua[var_name];

            if (lua_var.valid()) {
                emit_value = convert_lua_to_game_value(lua_var);
            } else {
                report_error("Lua global variable '" + var_name + "' not found");
                return game_value();
            }
        } else {
            // Must be array
            auto& arr = params.to_array();

            if (arr.empty() || arr[0].type_enum() != game_data_type::STRING) {
                report_error("First element must be variable name (string)");
                return game_value();
            }

            var_name = static_cast<std::string>(arr[0]);

            if (arr.size() == 1) {
                sol::object lua_var = lua[var_name];

                if (lua_var.valid()) {
                    emit_value = convert_lua_to_game_value(lua_var);
                } else {
                    report_error("Lua global variable '" + var_name + "' not found");
                    return game_value();
                }
            } else {
                // Value provided
                emit_value = arr[1];

                if (arr.size() > 2) {
                    target = arr[2];
                }

                if (arr.size() > 3) {
                    jip = arr[3];
                }
            }
        }

        auto_array<game_value> emission_data;
        emission_data.push_back(game_value(var_name));
        emission_data.push_back(emit_value);
        auto_array<game_value> cba_params;
        cba_params.push_back(game_value("KH_eve_luaVariableEmission"));
        cba_params.push_back(game_value(std::move(emission_data)));
        cba_params.push_back(target);
        cba_params.push_back(jip);
        trigger_cba_event_sqf(game_value(std::move(cba_params)));
        return game_value();
    } catch (const std::exception& e) {
        report_error("Failed to emit variable: " + std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Failed to emit variable");
        return game_value();
    }
}

static game_value lua_set_variable_sqf(game_value_parameter params) {
    try {
        LuaStackGuard guard(*g_lua_state);
        sol::state& lua = *g_lua_state;
        std::string var_name;
        game_value set_value;
        game_value target;
        game_value jip;

        auto& arr = params.to_array();

        if (arr.empty() || arr[0].type_enum() != game_data_type::STRING) {
            report_error("First element must be variable name (string)");
            return game_value();
        }

        var_name = static_cast<std::string>(arr[0]);

        if (arr.size() < 2) {
            report_error("Must provide at least name and value");
            return game_value();
        }

        set_value = arr[1];

        if (arr.size() > 2) {
            target = arr[2];
        }

        if (arr.size() > 3) {
            jip = arr[3];
        }

        if (var_name.empty()) {
            report_error("Variable name cannot be empty");
            return game_value();
        }

        // Check if we should use CBA event
        if (!((target.is_nil() || (target.type_enum() == game_data_type::BOOL && static_cast<bool>(target))) &&
            (jip.is_nil() || (jip.type_enum() == game_data_type::BOOL && !static_cast<bool>(jip))))) { 
            auto_array<game_value> emission_data;
            emission_data.push_back(game_value(var_name));
            emission_data.push_back(set_value);
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_luaVariableEmission"));
            cba_params.push_back(game_value(std::move(emission_data)));
            cba_params.push_back(target);
            cba_params.push_back(jip);
            trigger_cba_event_sqf(game_value(std::move(cba_params)));
            return game_value();
        } else {
            // Set directly in Lua global namespace
            lua[var_name] = convert_game_value_to_lua(set_value);
        }

        return game_value();
    } catch (const std::exception& e) {
        report_error("Failed to set Lua variable: " + std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Failed to set Lua variable");
        return game_value();
    }
}

static game_value lua_get_variable_sqf(game_value_parameter params) {
    try {
        LuaStackGuard guard(*g_lua_state);
        sol::state& lua = *g_lua_state;
        std::string var_name;
        game_value default_value;
        bool has_default = false;

        if (params.type_enum() == game_data_type::STRING) {
            var_name = static_cast<std::string>(params);
        } else {
            // Must be array
            auto& arr = params.to_array();

            if (arr.empty() || arr[0].type_enum() != game_data_type::STRING) {
                report_error("First element must be variable name (string)");
                return game_value();
            }

            var_name = static_cast<std::string>(arr[0]);

            if (arr.size() > 1) {
                default_value = arr[1];
                has_default = true;
            }
        }

        if (var_name.empty()) {
            report_error("Variable name cannot be empty");
            return game_value();
        }

        // Get value from Lua global
        sol::object lua_var = lua[var_name];

        // Check if variable exists and is not nil
        if (!lua_var.valid() || lua_var.get_type() == sol::type::nil) {
            if (has_default) {
                return default_value;
            }

            return game_value();
        }

        return convert_lua_to_game_value(lua_var);
    } catch (const std::exception& e) {
        report_error("Failed to get Lua variable: " + std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Failed to get Lua variable");
        return game_value();
    }
}

static game_value kh_execute_impl(game_value_parameter execute_params);

static game_value execute_sqf(game_value_parameter args, game_value_parameter code_or_function) {
    try {
        auto_array<game_value> params;

        if (code_or_function.type_enum() == game_data_type::ARRAY) {
            // Right arg is array: prepend left arg as first element
            params.push_back(args);
            auto& arr = code_or_function.to_array();
            params.insert(params.end(), arr.begin(), arr.end());
        } else {
            // Right arg is CODE or STRING: [arguments, code/string]
            params.push_back(args);
            params.push_back(code_or_function);
        }

        return kh_execute_impl(game_value(std::move(params)));
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("An unknown error occurred");
        return game_value();
    }
}

static game_value remove_handler_sqf(game_value_parameter handler_info) {
    try {        
        // Nest in outer array for _this call since the remover accepts array
        // in _this
        auto_array<game_value> nested;
        nested.push_back(handler_info);
        raw_call_sqf_args_native(g_compiled_sqf_remove_handler, game_value(std::move(nested)));
        return game_value();
    } catch (const std::exception& e) {
        report_error("Failed to remove handler: " + std::string(e.what()));
        return game_value();
    }
}

static game_value set_return_value_sqf(game_value_parameter value) noexcept {
    g_return_value = value;
    return game_value();
}

static game_value get_return_value_sqf() noexcept {
    return g_return_value;
}

static game_value set_call_arguments_sqf(game_value_parameter value) noexcept {
    g_call_arguments = value;
    return game_value();
}

static game_value get_call_arguments_sqf() noexcept {
    return g_call_arguments;
}

static game_value get_rotation_euler_sqf(game_value_parameter relative, game_value_parameter entity) {
    try {
        object obj = static_cast<object>(entity);

        auto make_zero_result = []() {
            auto_array<game_value> r;
            r.reserve(3);
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            return game_value(std::move(r));
        };

        vector3 dir = sqf::vector_dir(obj);
        vector3 up = sqf::vector_up(obj);

        // If relative object is not nil, calculate relative rotation
        if (!relative.is_nil()) {
            object rel_obj = static_cast<object>(relative);
            vector3 current_pos = sqf::get_pos_atl(obj);
            vector3 relative_pos = sqf::get_pos_atl(rel_obj);
            float dx = relative_pos.x - current_pos.x;
            float dy = relative_pos.y - current_pos.y;
            float dz = relative_pos.z - current_pos.z;
            float distance_horizontal = std::sqrt(dx * dx + dy * dy);
            float yaw = std::atan2(dy, dx);
            float pitch = std::atan2(dz, distance_horizontal);
            float cos_pitch = std::cos(pitch);
            float sin_pitch = std::sin(pitch);
            float cos_yaw = std::cos(yaw);
            float sin_yaw = std::sin(yaw);
            dir.x = cos_pitch * cos_yaw;
            dir.y = cos_pitch * sin_yaw;
            dir.z = sin_pitch;
            up.x = 0.0f;
            up.y = 0.0f;
            up.z = 1.0f;
        }

        // Normalize direction vector
        float dirX = dir.x;
        float dirY = dir.y;
        float dirZ = dir.z;
        float dirLen = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        if (dirLen < EPSILON) return make_zero_result();
        dirX /= dirLen; dirY /= dirLen; dirZ /= dirLen;

        // Normalize up vector
        float upX = up.x;
        float upY = up.y;
        float upZ = up.z;
        float upLen = std::sqrt(upX * upX + upY * upY + upZ * upZ);
        if (upLen < EPSILON) return make_zero_result();
        upX /= upLen; upY /= upLen; upZ /= upLen;

        // Right vector = dir ?? up
        float rightX = dirY * upZ - dirZ * upY;
        float rightY = dirZ * upX - dirX * upZ;
        float rightZ = dirX * upY - dirY * upX;
        float rightLen = std::sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
        if (rightLen < EPSILON) return make_zero_result();
        rightX /= rightLen; rightY /= rightLen; rightZ /= rightLen;

        // Re-orthogonalize up = right ?? dir
        upX = rightY * dirZ - rightZ * dirY;
        upY = rightZ * dirX - rightX * dirZ;
        upZ = rightX * dirY - rightY * dirX;
        float aroundX, aroundY, aroundZ;
        float cosRoll = std::sqrt(rightX * rightX + rightY * rightY);

        if (cosRoll > EPSILON) {
            aroundX = std::fmod(-std::atan2(dirZ, upZ) * RAD_TO_DEG + 360.0f, 360.0f);
            aroundY = std::fmod(std::atan2(rightZ, cosRoll) * RAD_TO_DEG + 360.0f, 360.0f);
            aroundZ = std::fmod(-std::atan2(rightY, rightX) * RAD_TO_DEG + 360.0f, 360.0f);
        } else {
            aroundX = 0.0f;
            aroundY = (rightZ > 0.0f) ? 90.0f : 270.0f;
            float coupled = std::atan2(-dirX, dirY);
            aroundZ = std::fmod(-coupled * RAD_TO_DEG + 360.0f, 360.0f);
        }

        auto_array<game_value> result;
        result.reserve(3);
        result.push_back(game_value(aroundX));
        result.push_back(game_value(aroundY));
        result.push_back(game_value(aroundZ));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to get rotation: " + std::string(e.what()));
        return game_value();
    }
}

static game_value set_rotation_euler_sqf(game_value_parameter entity, game_value_parameter rotation) {
    try {
        object obj = static_cast<object>(entity);
        auto& rot = rotation.to_array();

        if (rot.size() != 3) {
            report_error("Rotation must be an array of 3 elements [pitch, roll, yaw]");
            return game_value();
        }

        float aroundX = -static_cast<float>(rot[0]) * DEG_TO_RAD;
        float aroundY = -static_cast<float>(rot[1]) * DEG_TO_RAD;
        float aroundZ = -static_cast<float>(rot[2]) * DEG_TO_RAD;
        float dirX = 0.0f;
        float dirY = 1.0f;
        float dirZ = 0.0f;
        float upX = 0.0f;
        float upY = 0.0f;
        float upZ = 1.0f;

        if (std::abs(aroundX) > 0.0001f) {
            float cosX = std::cos(aroundX);
            float sinX = std::sin(aroundX);
            dirY = cosX;
            dirZ = sinX;
            upY = -sinX;
            upZ = cosX;
        }

        if (std::abs(aroundY) > 0.0001f) {
            float cosY = std::cos(aroundY);
            float sinY = std::sin(aroundY);
            dirX = dirZ * sinY;
            dirZ = dirZ * cosY;
            upX = upZ * sinY;
            upZ = upZ * cosY;
        }

        if (std::abs(aroundZ) > 0.0001f) {
            float cosZ = std::cos(aroundZ);
            float sinZ = std::sin(aroundZ);
            float dirXTemp = dirX;
            dirX = dirXTemp * cosZ - dirY * sinZ;
            dirY = dirY * cosZ + dirXTemp * sinZ;
            float upXTemp = upX;
            upX = upXTemp * cosZ - upY * sinZ;
            upY = upY * cosZ + upXTemp * sinZ;
        }

        vector3 dir(dirX, dirY, dirZ);
        vector3 up(upX, upY, upZ);
        sqf::set_vector_dir_and_up(obj, dir, up);
        return game_value();
    } catch (const std::exception& e) {
        report_error("Failed to set rotation: " + std::string(e.what()));
        return game_value();
    }
}

static game_value vector_to_euler_sqf(game_value_parameter vectors) {
    try {
        auto& vec_array = vectors.to_array();

        if (vec_array.size() != 2) {
            report_error("vectorToEuler requires an array of 2 vectors [[dirX, dirY, dirZ], [upX, upY, upZ]]");
            return game_value();
        }

        auto& dir_array = vec_array[0].to_array();
        auto& up_array = vec_array[1].to_array();

        if (dir_array.size() != 3 || up_array.size() != 3) {
            report_error("vectorToEuler requires three components per vector");
            return game_value();
        }

        auto make_zero_result = []() {
            auto_array<game_value> r;
            r.reserve(3);
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            return game_value(std::move(r));
        };

        float dirX = static_cast<float>(dir_array[0]);
        float dirY = static_cast<float>(dir_array[1]);
        float dirZ = static_cast<float>(dir_array[2]);
        float dirLen = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        if (dirLen < EPSILON) return make_zero_result();
        dirX /= dirLen; dirY /= dirLen; dirZ /= dirLen;

        // Extract and normalize up vector
        float upX = static_cast<float>(up_array[0]);
        float upY = static_cast<float>(up_array[1]);
        float upZ = static_cast<float>(up_array[2]);
        float upLen = std::sqrt(upX * upX + upY * upY + upZ * upZ);
        if (upLen < EPSILON) return make_zero_result();
        upX /= upLen; upY /= upLen; upZ /= upLen;

        // Right vector = dir ?? up
        float rightX = dirY * upZ - dirZ * upY;
        float rightY = dirZ * upX - dirX * upZ;
        float rightZ = dirX * upY - dirY * upX;
        float rightLen = std::sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
        if (rightLen < EPSILON) return make_zero_result();
        rightX /= rightLen; rightY /= rightLen; rightZ /= rightLen;
        upX = rightY * dirZ - rightZ * dirY;
        upY = rightZ * dirX - rightX * dirZ;
        upZ = rightX * dirY - rightY * dirX;
        float aroundX, aroundY, aroundZ;
        float cosRoll = std::sqrt(rightX * rightX + rightY * rightY);

        if (cosRoll > EPSILON) {
            // Normal case
            aroundX = std::fmod(-std::atan2(dirZ, upZ) * RAD_TO_DEG + 360.0f, 360.0f);
            aroundY = std::fmod(std::atan2(rightZ, cosRoll) * RAD_TO_DEG + 360.0f, 360.0f);
            aroundZ = std::fmod(-std::atan2(rightY, rightX) * RAD_TO_DEG + 360.0f, 360.0f);
        } else {
            // Gimbal lock
            aroundX = 0.0f;
            aroundY = (rightZ > 0.0f) ? 90.0f : 270.0f;
            float coupled = std::atan2(-dirX, dirY);
            aroundZ = std::fmod(-coupled * RAD_TO_DEG + 360.0f, 360.0f);
        }

        auto_array<game_value> result;
        result.reserve(3);
        result.push_back(game_value(aroundX));
        result.push_back(game_value(aroundY));
        result.push_back(game_value(aroundZ));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to convert vector to euler: " + std::string(e.what()));
        return game_value();
    }
}

static game_value euler_to_vector_sqf(game_value_parameter rotation) {
    try {
        auto& rot = rotation.to_array();

        if (rot.size() != 3) {
            report_error("eulerToVector requires an array of 3 elements [pitch, roll, yaw]");
            return game_value();
        }

        float aroundX = -static_cast<float>(rot[0]) * DEG_TO_RAD;
        float aroundY = -static_cast<float>(rot[1]) * DEG_TO_RAD;
        float aroundZ = -static_cast<float>(rot[2]) * DEG_TO_RAD;
        float dirX = 0.0f;
        float dirY = 1.0f;
        float dirZ = 0.0f;
        float upX = 0.0f;
        float upY = 0.0f;
        float upZ = 1.0f;

        if (std::abs(aroundX) > 0.0001f) {
            float cosX = std::cos(aroundX);
            float sinX = std::sin(aroundX);
            dirY = cosX;
            dirZ = sinX;
            upY = -sinX;
            upZ = cosX;
        }

        if (std::abs(aroundY) > 0.0001f) {
            float cosY = std::cos(aroundY);
            float sinY = std::sin(aroundY);
            dirX = dirZ * sinY;
            dirZ = dirZ * cosY;
            upX = upZ * sinY;
            upZ = upZ * cosY;
        }

        if (std::abs(aroundZ) > 0.0001f) {
            float cosZ = std::cos(aroundZ);
            float sinZ = std::sin(aroundZ);
            float dirXTemp = dirX;
            dirX = dirXTemp * cosZ - dirY * sinZ;
            dirY = dirY * cosZ + dirXTemp * sinZ;
            float upXTemp = upX;
            upX = upXTemp * cosZ - upY * sinZ;
            upY = upY * cosZ + upXTemp * sinZ;
        }

        auto_array<game_value> dir_array;
        dir_array.reserve(3);
        dir_array.push_back(game_value(dirX));
        dir_array.push_back(game_value(dirY));
        dir_array.push_back(game_value(dirZ));
        auto_array<game_value> up_array;
        up_array.reserve(3);
        up_array.push_back(game_value(upX));
        up_array.push_back(game_value(upY));
        up_array.push_back(game_value(upZ));
        auto_array<game_value> result;
        result.reserve(2);
        result.push_back(game_value(std::move(dir_array)));
        result.push_back(game_value(std::move(up_array)));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to convert euler to vector: " + std::string(e.what()));
        return game_value();
    }
}

static game_value euler_to_quaternion_sqf(game_value_parameter rotation) {
    try {
        auto& rot = rotation.to_array();

        if (rot.size() != 3) {
            report_error("eulerToQuaternion requires an array of 3 elements [pitch, roll, yaw]");
            return game_value();
        }

        // Negate to match eulerToVector convention: R_Z(-yaw) ?? R_Y(-roll)
        // ?? R_X(-pitch)
        float halfX = -static_cast<float>(rot[0]) * DEG_TO_RAD * 0.5f;
        float halfY = -static_cast<float>(rot[1]) * DEG_TO_RAD * 0.5f;
        float halfZ = -static_cast<float>(rot[2]) * DEG_TO_RAD * 0.5f;
        float cx = std::cos(halfX), sx = std::sin(halfX);
        float cy = std::cos(halfY), sy = std::sin(halfY);
        float cz = std::cos(halfZ), sz = std::sin(halfZ);

        // q = q_z * q_y * q_x (ZYX order)
        float w = cz * cy * cx + sz * sy * sx;
        float x = cz * cy * sx - sz * sy * cx;
        float y = cz * sy * cx + sz * cy * sx;
        float z = sz * cy * cx - cz * sy * sx;
        auto_array<game_value> result;
        result.reserve(4);
        result.push_back(game_value(w));
        result.push_back(game_value(x));
        result.push_back(game_value(y));
        result.push_back(game_value(z));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to convert euler to quaternion: " + std::string(e.what()));
        return game_value();
    }
}

static game_value quaternion_to_euler_sqf(game_value_parameter quat) {
    try {
        auto& q = quat.to_array();

        if (q.size() != 4) {
            report_error("quaternionToEuler requires an array of 4 elements [w, x, y, z]");
            return game_value();
        }

        float w = static_cast<float>(q[0]);
        float x = static_cast<float>(q[1]);
        float y = static_cast<float>(q[2]);
        float z = static_cast<float>(q[3]);

        // Normalize quaternion
        float len = std::sqrt(w * w + x * x + y * y + z * z);

        if (len < EPSILON) {
            auto_array<game_value> r;
            r.reserve(3);
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            return game_value(std::move(r));
        }

        w /= len; x /= len; y /= len; z /= len;

        // ZYX euler extraction These extract the angles of R_Z(-yaw)
        // R_Y(-roll) R_X(-pitch)
        float sinRoll = 2.0f * (w * y - z * x);
        float aroundX, aroundY, aroundZ;

        if (std::abs(sinRoll) > 1.0f - EPSILON) {
            // Gimbal lock ??? roll is ??90??
            aroundX = 0.0f;
            aroundY = (sinRoll < 0.0f) ? 90.0f : 270.0f;
            float coupled = std::atan2(2.0f * (x * y + w * z), 1.0f - 2.0f * (y * y + z * z));
            aroundZ = std::fmod(-coupled * RAD_TO_DEG + 360.0f, 360.0f);
        } else {
            // Normal case ??? negate extracted angles to undo the convention
            // negation
            float extractedX = std::atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
            float extractedY = std::asin(sinRoll);
            float extractedZ = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));
            aroundX = std::fmod(-extractedX * RAD_TO_DEG + 360.0f, 360.0f);
            aroundY = std::fmod(-extractedY * RAD_TO_DEG + 360.0f, 360.0f);
            aroundZ = std::fmod(-extractedZ * RAD_TO_DEG + 360.0f, 360.0f);
        }

        auto_array<game_value> result;
        result.reserve(3);
        result.push_back(game_value(aroundX));
        result.push_back(game_value(aroundY));
        result.push_back(game_value(aroundZ));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to convert quaternion to euler: " + std::string(e.what()));
        return game_value();
    }
}

static game_value vector_to_quaternion_sqf(game_value_parameter vectors) {
    try {
        auto& vec_array = vectors.to_array();

        if (vec_array.size() != 2) {
            report_error("vectorToQuaternion requires an array of 2 vectors [[dirX, dirY, dirZ], [upX, upY, upZ]]");
            return game_value();
        }

        auto& dir_array = vec_array[0].to_array();
        auto& up_array = vec_array[1].to_array();

        if (dir_array.size() != 3 || up_array.size() != 3) {
            report_error("vectorToQuaternion requires three components per vector");
            return game_value();
        }

        auto make_identity = []() {
            auto_array<game_value> r;
            r.reserve(4);
            r.push_back(game_value(1.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            return game_value(std::move(r));
        };

        // Normalize direction
        float dirX = static_cast<float>(dir_array[0]);
        float dirY = static_cast<float>(dir_array[1]);
        float dirZ = static_cast<float>(dir_array[2]);
        float dirLen = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        if (dirLen < EPSILON) return make_identity();
        dirX /= dirLen; dirY /= dirLen; dirZ /= dirLen;

        // Normalize up
        float upX = static_cast<float>(up_array[0]);
        float upY = static_cast<float>(up_array[1]);
        float upZ = static_cast<float>(up_array[2]);
        float upLen = std::sqrt(upX * upX + upY * upY + upZ * upZ);
        if (upLen < EPSILON) return make_identity();
        upX /= upLen; upY /= upLen; upZ /= upLen;

        // Right = dir ?? up
        float rightX = dirY * upZ - dirZ * upY;
        float rightY = dirZ * upX - dirX * upZ;
        float rightZ = dirX * upY - dirY * upX;
        float rightLen = std::sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
        if (rightLen < EPSILON) return make_identity();
        rightX /= rightLen; rightY /= rightLen; rightZ /= rightLen;

        // Re-orthogonalize up = right ?? dir
        upX = rightY * dirZ - rightZ * dirY;
        upY = rightZ * dirX - rightX * dirZ;
        upZ = rightX * dirY - rightY * dirX;

        // Rotation matrix columns: [right, dir, up] m00=rightX m01=dirX
        // m02=upX m10=rightY m11=dirY m12=upY m20=rightZ m21=dirZ m22=upZ
        // Shepperd's method
        float m00 = rightX, m11 = dirY, m22 = upZ;
        float trace = m00 + m11 + m22;
        float w, x, y, z;

        if (trace > 0.0f) {
            float s = std::sqrt(trace + 1.0f) * 2.0f;
            w = 0.25f * s;
            x = (dirZ - upY) / s;
            y = (upX - rightZ) / s;
            z = (rightY - dirX) / s;
        } else if (m00 > m11 && m00 > m22) {
            float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
            w = (dirZ - upY) / s;
            x = 0.25f * s;
            y = (rightY + dirX) / s;
            z = (upX + rightZ) / s;
        } else if (m11 > m22) {
            float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
            w = (upX - rightZ) / s;
            x = (rightY + dirX) / s;
            y = 0.25f * s;
            z = (dirZ + upY) / s;
        } else {
            float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
            w = (rightY - dirX) / s;
            x = (upX + rightZ) / s;
            y = (dirZ + upY) / s;
            z = 0.25f * s;
        }

        auto_array<game_value> result;
        result.reserve(4);
        result.push_back(game_value(w));
        result.push_back(game_value(x));
        result.push_back(game_value(y));
        result.push_back(game_value(z));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to convert vector to quaternion: " + std::string(e.what()));
        return game_value();
    }
}

static game_value quaternion_to_vector_sqf(game_value_parameter quat) {
    try {
        auto& q = quat.to_array();

        if (q.size() != 4) {
            report_error("quaternionToVector requires an array of 4 elements [w, x, y, z]");
            return game_value();
        }

        float w = static_cast<float>(q[0]);
        float x = static_cast<float>(q[1]);
        float y = static_cast<float>(q[2]);
        float z = static_cast<float>(q[3]);

        // Normalize
        float len = std::sqrt(w * w + x * x + y * y + z * z);

        if (len < EPSILON) {
            // Identity orientation
            auto_array<game_value> dir_arr;
            dir_arr.reserve(3);
            dir_arr.push_back(game_value(0.0f));
            dir_arr.push_back(game_value(1.0f));
            dir_arr.push_back(game_value(0.0f));
            auto_array<game_value> up_arr;
            up_arr.reserve(3);
            up_arr.push_back(game_value(0.0f));
            up_arr.push_back(game_value(0.0f));
            up_arr.push_back(game_value(1.0f));
            auto_array<game_value> result;
            result.reserve(2);
            result.push_back(game_value(std::move(dir_arr)));
            result.push_back(game_value(std::move(up_arr)));
            return game_value(std::move(result));
        }
        w /= len; x /= len; y /= len; z /= len;

        // Rotation matrix from quaternion dir = R * [0,1,0] (column 1 of
        // rotation matrix)
        float dirX = 2.0f * (x * y - w * z);
        float dirY = 1.0f - 2.0f * (x * x + z * z);
        float dirZ = 2.0f * (y * z + w * x);

        // up = R * [0,0,1] (column 2 of rotation matrix)
        float upX = 2.0f * (x * z + w * y);
        float upY = 2.0f * (y * z - w * x);
        float upZ = 1.0f - 2.0f * (x * x + y * y);
        auto_array<game_value> dir_arr;
        dir_arr.reserve(3);
        dir_arr.push_back(game_value(dirX));
        dir_arr.push_back(game_value(dirY));
        dir_arr.push_back(game_value(dirZ));
        auto_array<game_value> up_arr;
        up_arr.reserve(3);
        up_arr.push_back(game_value(upX));
        up_arr.push_back(game_value(upY));
        up_arr.push_back(game_value(upZ));
        auto_array<game_value> result;
        result.reserve(2);
        result.push_back(game_value(std::move(dir_arr)));
        result.push_back(game_value(std::move(up_arr)));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to convert quaternion to vector: " + std::string(e.what()));
        return game_value();
    }
}

static game_value quaternion_slerp_sqf(game_value_parameter right_arg) {
    try {
        auto& params = right_arg.to_array();

        if (params.size() != 3) {
            report_error("quaternionSlerp requires [quatA, quatB, t]");
            return game_value();
        }

        auto& qa = params[0].to_array();
        auto& qb = params[1].to_array();
        float t = static_cast<float>(params[2]);

        if (qa.size() != 4 || qb.size() != 4) {
            report_error("quaternionSlerp requires quaternions with 4 components each");
            return game_value();
        }

        float aw = static_cast<float>(qa[0]);
        float ax = static_cast<float>(qa[1]);
        float ay = static_cast<float>(qa[2]);
        float az = static_cast<float>(qa[3]);
        float bw = static_cast<float>(qb[0]);
        float bx = static_cast<float>(qb[1]);
        float by = static_cast<float>(qb[2]);
        float bz = static_cast<float>(qb[3]);

        // Dot product
        float dot = aw * bw + ax * bx + ay * by + az * bz;

        // Take shortest path
        if (dot < 0.0f) {
            bw = -bw; bx = -bx; by = -by; bz = -bz;
            dot = -dot;
        }

        float w, x, y, z;

        if (dot > 0.9995f) {
            // Very close ??? linear interpolation to avoid division by
            // near-zero sin
            w = aw + t * (bw - aw);
            x = ax + t * (bx - ax);
            y = ay + t * (by - ay);
            z = az + t * (bz - az);
        } else {
            float theta = std::acos(dot);
            float sinTheta = std::sin(theta);
            float wa = std::sin((1.0f - t) * theta) / sinTheta;
            float wb = std::sin(t * theta) / sinTheta;
            w = wa * aw + wb * bw;
            x = wa * ax + wb * bx;
            y = wa * ay + wb * by;
            z = wa * az + wb * bz;
        }

        // Normalize result
        float len = std::sqrt(w * w + x * x + y * y + z * z);

        if (len > 0.0001f) {
            w /= len; x /= len; y /= len; z /= len;
        }

        auto_array<game_value> result;
        result.reserve(4);
        result.push_back(game_value(w));
        result.push_back(game_value(x));
        result.push_back(game_value(y));
        result.push_back(game_value(z));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to slerp quaternions: " + std::string(e.what()));
        return game_value();
    }
}

static game_value quaternion_multiply_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        auto& qa = left_arg.to_array();
        auto& qb = right_arg.to_array();

        if (qa.size() != 4 || qb.size() != 4) {
            report_error("quaternionMultiply requires quaternions with 4 components each");
            return game_value();
        }

        float aw = static_cast<float>(qa[0]);
        float ax = static_cast<float>(qa[1]);
        float ay = static_cast<float>(qa[2]);
        float az = static_cast<float>(qa[3]);
        float bw = static_cast<float>(qb[0]);
        float bx = static_cast<float>(qb[1]);
        float by = static_cast<float>(qb[2]);
        float bz = static_cast<float>(qb[3]);
        float w = aw * bw - ax * bx - ay * by - az * bz;
        float x = aw * bx + ax * bw + ay * bz - az * by;
        float y = aw * by - ax * bz + ay * bw + az * bx;
        float z = aw * bz + ax * by - ay * bx + az * bw;
        auto_array<game_value> result;
        result.reserve(4);
        result.push_back(game_value(w));
        result.push_back(game_value(x));
        result.push_back(game_value(y));
        result.push_back(game_value(z));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to multiply quaternions: " + std::string(e.what()));
        return game_value();
    }
}

static game_value get_quaternion_rotation_sqf(game_value_parameter relative, game_value_parameter entity) {
    try {
        object obj = static_cast<object>(entity);

        auto make_identity = []() {
            auto_array<game_value> r;
            r.reserve(4);
            r.push_back(game_value(1.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            return game_value(std::move(r));
        };

        vector3 dir = sqf::vector_dir(obj);
        vector3 up = sqf::vector_up(obj);

        if (!relative.is_nil()) {
            object rel_obj = static_cast<object>(relative);
            vector3 current_pos = sqf::get_pos_atl(obj);
            vector3 relative_pos = sqf::get_pos_atl(rel_obj);
            float dx = relative_pos.x - current_pos.x;
            float dy = relative_pos.y - current_pos.y;
            float dz = relative_pos.z - current_pos.z;
            float distance_horizontal = std::sqrt(dx * dx + dy * dy);
            float yaw = std::atan2(dy, dx);
            float pitch = std::atan2(dz, distance_horizontal);
            float cos_pitch = std::cos(pitch);
            float sin_pitch = std::sin(pitch);
            float cos_yaw = std::cos(yaw);
            float sin_yaw = std::sin(yaw);
            dir.x = cos_pitch * cos_yaw;
            dir.y = cos_pitch * sin_yaw;
            dir.z = sin_pitch;
            up.x = 0.0f;
            up.y = 0.0f;
            up.z = 1.0f;
        }

        // Normalize direction
        float dirX = dir.x, dirY = dir.y, dirZ = dir.z;
        float dirLen = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        if (dirLen < EPSILON) return make_identity();
        dirX /= dirLen; dirY /= dirLen; dirZ /= dirLen;

        // Normalize up
        float upX = up.x, upY = up.y, upZ = up.z;
        float upLen = std::sqrt(upX * upX + upY * upY + upZ * upZ);
        if (upLen < EPSILON) return make_identity();
        upX /= upLen; upY /= upLen; upZ /= upLen;

        // Right = dir ?? up
        float rightX = dirY * upZ - dirZ * upY;
        float rightY = dirZ * upX - dirX * upZ;
        float rightZ = dirX * upY - dirY * upX;
        float rightLen = std::sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
        if (rightLen < EPSILON) return make_identity();
        rightX /= rightLen; rightY /= rightLen; rightZ /= rightLen;

        // Re-orthogonalize up = right ?? dir
        upX = rightY * dirZ - rightZ * dirY;
        upY = rightZ * dirX - rightX * dirZ;
        upZ = rightX * dirY - rightY * dirX;

        // Shepperd's method
        float m00 = rightX, m11 = dirY, m22 = upZ;
        float trace = m00 + m11 + m22;
        float w, x, y, z;

        if (trace > 0.0f) {
            float s = std::sqrt(trace + 1.0f) * 2.0f;
            w = 0.25f * s;
            x = (dirZ - upY) / s;
            y = (upX - rightZ) / s;
            z = (rightY - dirX) / s;
        } else if (m00 > m11 && m00 > m22) {
            float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
            w = (dirZ - upY) / s;
            x = 0.25f * s;
            y = (rightY + dirX) / s;
            z = (upX + rightZ) / s;
        } else if (m11 > m22) {
            float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
            w = (upX - rightZ) / s;
            x = (rightY + dirX) / s;
            y = 0.25f * s;
            z = (dirZ + upY) / s;
        } else {
            float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
            w = (rightY - dirX) / s;
            x = (upX + rightZ) / s;
            y = (dirZ + upY) / s;
            z = 0.25f * s;
        }

        auto_array<game_value> result;
        result.reserve(4);
        result.push_back(game_value(w));
        result.push_back(game_value(x));
        result.push_back(game_value(y));
        result.push_back(game_value(z));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to get quaternion rotation: " + std::string(e.what()));
        return game_value();
    }
}

static game_value set_quaternion_rotation_sqf(game_value_parameter entity, game_value_parameter quat) {
    try {
        object obj = static_cast<object>(entity);
        auto& q = quat.to_array();

        if (q.size() != 4) {
            report_error("setQuaternionRotation requires [w, x, y, z]");
            return game_value();
        }

        float w = static_cast<float>(q[0]);
        float x = static_cast<float>(q[1]);
        float y = static_cast<float>(q[2]);
        float z = static_cast<float>(q[3]);

        // Normalize
        float len = std::sqrt(w * w + x * x + y * y + z * z);

        if (len < EPSILON) {
            sqf::set_vector_dir_and_up(obj, vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f));
            return game_value();
        }

        w /= len; x /= len; y /= len; z /= len;

        // dir = R * [0,1,0]
        float dirX = 2.0f * (x * y - w * z);
        float dirY = 1.0f - 2.0f * (x * x + z * z);
        float dirZ = 2.0f * (y * z + w * x);

        // up = R * [0,0,1]
        float upX = 2.0f * (x * z + w * y);
        float upY = 2.0f * (y * z - w * x);
        float upZ = 1.0f - 2.0f * (x * x + y * y);
        sqf::set_vector_dir_and_up(obj, vector3(dirX, dirY, dirZ), vector3(upX, upY, upZ));
        return game_value();
    } catch (const std::exception& e) {
        report_error("Failed to set quaternion rotation: " + std::string(e.what()));
        return game_value();
    }
}

static game_value axis_angle_to_quaternion_sqf(game_value_parameter params) {
    try {
        auto& arr = params.to_array();

        if (arr.size() != 2) {
            report_error("axisAngleToQuaternion requires [[axisX, axisY, axisZ], angle]");
            return game_value();
        }

        auto& axis_array = arr[0].to_array();

        if (axis_array.size() != 3) {
            report_error("axisAngleToQuaternion requires axis with 3 components");
            return game_value();
        }

        float axisX = static_cast<float>(axis_array[0]);
        float axisY = static_cast<float>(axis_array[1]);
        float axisZ = static_cast<float>(axis_array[2]);
        float angle = static_cast<float>(arr[1]);

        // Normalize axis
        float axisLen = std::sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);

        if (axisLen < EPSILON) {
            // Zero axis ??? return identity
            auto_array<game_value> r;
            r.reserve(4);
            r.push_back(game_value(1.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            r.push_back(game_value(0.0f));
            return game_value(std::move(r));
        }

        axisX /= axisLen; axisY /= axisLen; axisZ /= axisLen;
        float halfAngle = angle * DEG_TO_RAD * 0.5f;
        float s = std::sin(halfAngle);
        float w = std::cos(halfAngle);
        float x = axisX * s;
        float y = axisY * s;
        float z = axisZ * s;
        auto_array<game_value> result;
        result.reserve(4);
        result.push_back(game_value(w));
        result.push_back(game_value(x));
        result.push_back(game_value(y));
        result.push_back(game_value(z));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to convert axis-angle to quaternion: " + std::string(e.what()));
        return game_value();
    }
}

static game_value quaternion_to_axis_angle_sqf(game_value_parameter quat) {
    try {
        auto& q = quat.to_array();

        if (q.size() != 4) {
            report_error("quaternionToAxisAngle requires [w, x, y, z]");
            return game_value();
        }

        float w = static_cast<float>(q[0]);
        float x = static_cast<float>(q[1]);
        float y = static_cast<float>(q[2]);
        float z = static_cast<float>(q[3]);

        // Normalize
        float len = std::sqrt(w * w + x * x + y * y + z * z);

        if (len < EPSILON) {
            auto_array<game_value> axis_arr;
            axis_arr.reserve(3);
            axis_arr.push_back(game_value(0.0f));
            axis_arr.push_back(game_value(1.0f));
            axis_arr.push_back(game_value(0.0f));
            auto_array<game_value> result;
            result.reserve(2);
            result.push_back(game_value(std::move(axis_arr)));
            result.push_back(game_value(0.0f));
            return game_value(std::move(result));
        }
        w /= len; x /= len; y /= len; z /= len;

        // Ensure w is positive so angle is in [0, 360)
        if (w < 0.0f) {
            w = -w; x = -x; y = -y; z = -z;
        }

        // Clamp w to avoid NaN from acos
        if (w > 1.0f) w = 1.0f;
        float halfAngle = std::acos(w);
        float angle = halfAngle * 2.0f * RAD_TO_DEG;
        float s = std::sin(halfAngle);
        float axisX, axisY, axisZ;

        if (s > EPSILON) {
            axisX = x / s;
            axisY = y / s;
            axisZ = z / s;
        } else {
            // Near-zero rotation ??? axis is arbitrary
            axisX = 0.0f;
            axisY = 1.0f;
            axisZ = 0.0f;
        }

        auto_array<game_value> axis_arr;
        axis_arr.reserve(3);
        axis_arr.push_back(game_value(axisX));
        axis_arr.push_back(game_value(axisY));
        axis_arr.push_back(game_value(axisZ));
        auto_array<game_value> result;
        result.reserve(2);
        result.push_back(game_value(std::move(axis_arr)));
        result.push_back(game_value(angle));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("Failed to convert quaternion to axis-angle: " + std::string(e.what()));
        return game_value();
    }
}

static game_value initialize_ai_sqf(game_value_parameter ai_name) {
    try {
        std::string name = ai_name;

        if (name.empty()) {
            report_error("KH - AI Framework: Error in initializeAi - Name cannot be empty");
            return game_value(false);
        }

        auto& framework = AIFramework::instance();
        bool success = framework.initialize_ai(name);        
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in initializeAi - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value stop_ai_sqf(game_value_parameter ai_name) {
    try {
        std::string name = ai_name;

        if (name.empty()) {
            report_error("KH - AI Framework: Error in stopAi - Name cannot be empty");
            return game_value(false);
        }

        auto& framework = AIFramework::instance();
        bool success = framework.stop_ai(name);        
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in stopAi - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value stop_all_ai_sqf() {
    try {
        auto& framework = AIFramework::instance();
        framework.stop_all();
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in stopAllAi - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value is_ai_active_sqf(game_value_parameter ai_name) {
    try {
        std::string name = ai_name;

        if (name.empty()) {
            report_error("KH - AI Framework: Error in isAiActive - Name cannot be empty");
            return game_value(false);
        }

        auto& framework = AIFramework::instance();
        return game_value(framework.is_ai_active(name));
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in isAiActive - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value is_ai_generating_sqf(game_value_parameter ai_name) {    
    try {
        std::string name = ai_name;

        if (name.empty()) {
            report_error("KH - AI Framework: Error in isAiGenerating - Name cannot be empty");
            return game_value(false);
        }

        bool is_generating = AIFramework::instance().is_ai_generating(name);
        return game_value(is_generating);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in isAiGenerating: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value get_active_ai_sqf() {
    try {
        auto& framework = AIFramework::instance();
        auto active_names = framework.get_active_ai_names();
        auto_array<game_value> result;

        for (const auto& name : active_names) {
            result.push_back(name);
        }

        return result;
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in getActiveAi - " + std::string(e.what()));
        return game_value();
    }
}

static game_value set_ai_model_sqf(game_value_parameter model) {
    try {
        std::string filename = model;

        if (filename.empty()) {
            report_error("KH - AI Framework: Error in setAiModel - Name cannot be empty");
            return game_value(false);
        }

        auto& framework = AIFramework::instance();
        framework.set_model_path(filename);
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in setAiModel - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value set_ai_instance_model_path_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string ai_name = left_arg;
        std::string filename = right_arg;

        if (ai_name.empty() || filename.empty()) {
            report_error("KH - AI Framework: Error in setAiModel - Both AI name and model filename must be provided");
            return game_value(false);
        }

        auto& framework = AIFramework::instance();
        bool success = framework.set_ai_model_path(ai_name, filename);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in setAiModelPath - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value update_ai_system_prompt_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string ai_name = left_arg;
        std::string prompt = right_arg;

        if (ai_name.empty()) {
            report_error("KH - AI Framework: Error in updateAiSystemPrompt - Name cannot be empty");
            return game_value(false);
        }

        auto& framework = AIFramework::instance();
        bool success = framework.update_system_prompt(ai_name, prompt);        
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in updateAiSystemPrompt - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value update_ai_master_prompt_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string ai_name = left_arg;
        std::string prompt = right_arg;

        if (ai_name.empty()) {
            report_error("KH - AI Framework: Error in updateAiMasterPrompt - Name cannot be empty");
            return game_value(false);
        }

        auto& framework = AIFramework::instance();
        bool success = framework.update_master_prompt(ai_name, prompt);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in updateAiMasterPrompt - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value update_ai_user_prompt_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string ai_name = left_arg;
        std::string prompt = right_arg;

        if (ai_name.empty()) {
            report_error("KH - AI Framework: Error in updateAiUserPrompt - Name cannot be empty");
            return game_value(false);
        }

        auto& framework = AIFramework::instance();
        bool success = framework.update_user_prompt(ai_name, prompt);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in updateAiUserPrompt - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value set_ai_parameters_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string ai_name = left_arg;
        std::vector<float> tensor_split;

        if (ai_name.empty()) {
            report_error("KH - AI Framework: Error in setAiParameters - Name cannot be empty");
            return game_value(false);
        }

        auto params = right_arg.to_array();

        if (params.size() != 25) {
            report_error("KH - AI Framework: setAiParameters requires exactly 25 parameters");
            return game_value(false);
        }

        int n_ctx = static_cast<int>((float)params[0]);
        int max_new_tokens = static_cast<int>((float)params[1]);
        float temperature = static_cast<float>(params[2]);
        int top_k = static_cast<int>((float)params[3]);
        float top_p = static_cast<float>(params[4]);
        float min_p = static_cast<float>(params[5]);
        float typical_p = static_cast<float>(params[6]);
        float repeat_penalty = static_cast<float>(params[7]);
        int repeat_last_n = static_cast<int>(params[8]);
        float presence_penalty = static_cast<float>(params[9]);
        float frequency_penalty = static_cast<float>(params[10]);
        int mirostat = static_cast<int>(params[11]);
        float mirostat_tau = static_cast<float>(params[12]);
        float mirostat_eta = static_cast<float>(params[13]);
        int seed = static_cast<uint32_t>(static_cast<int>(params[14]));
        int n_batch = static_cast<int>((float)params[15]);
        int n_ubatch = static_cast<int>((float)params[16]);
        int cpu_threads = static_cast<int>((float)params[17]);
        int cpu_threads_batch = static_cast<int>((float)params[18]);
        int gpu_layers = static_cast<int>((float)params[19]);
        bool flash_attention = static_cast<bool>(params[20]);
        bool offload_kv_cache = static_cast<bool>(params[21]);
        int main_gpu = static_cast<int>(params[22]);

        if (params[23].type_enum() == game_data_type::ARRAY) {
            auto& split_params = params[23].to_array();

            for (size_t i = 0; i < split_params.size(); i++) {
                if (split_params[i].type_enum() == game_data_type::SCALAR) {
                    tensor_split.push_back(static_cast<float>(split_params[i]));
                }
            }
        } else {
            report_error("KH - AI Framework: setAiParameters error: Tensor Split must be an array");
        }

        int split_mode = static_cast<int>(params[24]);

        bool result = AIFramework::instance().set_ai_parameters(
            ai_name, n_ctx, max_new_tokens, temperature, top_k, top_p,
            min_p, typical_p, repeat_penalty, repeat_last_n,
            presence_penalty, frequency_penalty, mirostat, mirostat_tau, mirostat_eta, seed,
            n_batch, n_ubatch, cpu_threads, cpu_threads_batch, gpu_layers,
            flash_attention, offload_kv_cache, main_gpu, tensor_split,
            split_mode
        );

        return game_value(result);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: setAiParameters error: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value trigger_ai_inference_sqf(game_value_parameter right_arg) {
    try {
        std::string ai_name = right_arg;

        if (ai_name.empty()) {
            report_error("KH - AI Framework: Error in triggerAiInference - Name cannot be empty");
            return game_value(false);
        }

        bool result = AIFramework::instance().trigger_ai_inference(ai_name);
        return game_value(result);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: triggerAiInference error: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value set_ai_markers_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string ai_name = left_arg;
        auto markers = right_arg.to_array();

        if (ai_name.empty()) {
            report_error("KH - AI Framework: Error in setAiMarkers - Name cannot be empty");
            return game_value(false);
        }

        if (markers.size() != 6) {
            report_error("KH - AI Framework: setAiMarkers requires exactly 6 markers: [systemStart, systemEnd, userStart, userEnd, assistantStart, assistantEnd]");
            return game_value(false);
        }

        std::string sys_start = static_cast<std::string>(markers[0]);
        std::string sys_end = static_cast<std::string>(markers[1]);
        std::string usr_start = static_cast<std::string>(markers[2]);
        std::string usr_end = static_cast<std::string>(markers[3]);
        std::string asst_start = static_cast<std::string>(markers[4]);
        std::string asst_end = static_cast<std::string>(markers[5]);        
        auto& framework = AIFramework::instance();
        bool success = framework.set_ai_markers(ai_name, sys_start, sys_end, usr_start, usr_end, asst_start, asst_end);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in setAiMarkers - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value abort_ai_generation_sqf(game_value_parameter right) {
    std::string ai_name = right;

    if (ai_name.empty()) {
        report_error("KH - AI Framework: Error in abortAiGeneration - Name cannot be empty");
        return game_value(false);
    }

    try {
        bool success = AIFramework::instance().abort_ai_generation(ai_name);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in abortAiGeneration - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value log_ai_generation_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    std::string ai_name = left_arg;

    if (ai_name.empty()) {
        report_error("KH - AI Framework: Error in logAiGeneration - Name cannot be empty");
        return game_value(false);
    }

    bool enabled = right_arg;

    try {        
        bool success = AIFramework::instance().set_ai_log_generation(ai_name, enabled);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in logAiGeneration - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value reset_ai_context_sqf(game_value_parameter right) {
    std::string ai_name = right;

    if (ai_name.empty()) {
        report_error("KH - AI Framework: Error in resetAiContext - Name cannot be empty");
        return game_value(false);
    }

    try {
        bool success = AIFramework::instance().reset_ai_context(ai_name);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - AI Framework: Error in resetAiContext - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value tts_load_model_sqf(game_value_parameter model) {
    try {
        std::string model_name = model;        
        bool success = TTSFramework::instance().load_model(model_name);

        if (!success) {
            report_error("KH - TTS Framework: Failed to load model: " + model_name);
        }

        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - TTS Framework: Error in ttsLoadModel - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value tts_load_model_with_config_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string model_name = left_arg;
        int num_threads = 4;
        float noise_scale = 0.667f;
        float noise_scale_w = 0.8f;
        float length_scale = 1.0f;

        if (right_arg.type_enum() == game_data_type::ARRAY) {
            try {
                auto config = right_arg.to_array();

                if (config.size() > 0 && !config[0].is_nil()) {
                    num_threads = static_cast<int>(static_cast<float>(config[0]));
                }

                if (config.size() > 1 && !config[1].is_nil()) {
                    noise_scale = static_cast<float>(config[1]);
                }

                if (config.size() > 2 && !config[2].is_nil()) {
                    noise_scale_w = static_cast<float>(config[2]);
                }

                if (config.size() > 3 && !config[3].is_nil()) {
                    length_scale = static_cast<float>(config[3]);
                }
            } catch (...) {
                // Use defaults if parsing fails
            }
        }

        bool success = TTSFramework::instance().load_model(
            model_name, num_threads, noise_scale, noise_scale_w, length_scale
        );

        if (!success) {
            report_error("KH - TTS Framework: Failed to load model: " + model_name);
        }

        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - TTS Framework: Error in ttsLoadModel - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value tts_speak_sqf(game_value_parameter params) {
    try {
        auto arr = params.to_array();

        if (arr.size() < 2) {
            report_error("KH - TTS Framework: ttsSpeak requires at least [speakerId, text]");
            return game_value(false);
        }

        std::string speaker_id = arr[0];
        std::string text = arr[1];
        float x = arr.size() > 2 ? static_cast<float>(arr[2]) : 0.0f;
        float y = arr.size() > 3 ? static_cast<float>(arr[3]) : 0.0f;
        float z = arr.size() > 4 ? static_cast<float>(arr[4]) : 0.0f;
        float volume = arr.size() > 5 ? static_cast<float>(arr[5]) : 1.0f;
        float speed = arr.size() > 6 ? static_cast<float>(arr[6]) : 1.0f;
        int sid = arr.size() > 7 ? static_cast<int>(static_cast<float>(arr[7])) : 0;
        auto effects = TTSFramework::parse_effects_from_args(arr, 8);

        if (speaker_id.empty()) {
            report_error("KH - TTS Framework: Speaker ID cannot be empty");
            return game_value(false);
        }

        if (text.empty()) {
            report_error("KH - TTS Framework: Text cannot be empty");
            return game_value(false);
        }

        bool success = TTSFramework::instance().speak(speaker_id, text, x, y, z, volume, speed, sid, effects);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - TTS Framework: Error in ttsSpeak - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value tts_update_speaker_sqf(game_value_parameter params) {
    try {
        auto arr = params.to_array();

        if (arr.size() < 5) {
            report_error("KH - TTS Framework: ttsUpdateSpeaker requires [speakerId, x, y, z, volume]");
            return game_value(false);
        }

        std::string speaker_id = arr[0];
        float x = static_cast<float>(arr[1]);
        float y = static_cast<float>(arr[2]);
        float z = static_cast<float>(arr[3]);
        float volume = static_cast<float>(arr[4]);
        auto effects = TTSFramework::parse_effects_from_args(arr, 5);

        if (speaker_id.empty()) {
            report_error("KH - TTS Framework: Speaker ID cannot be empty");
            return game_value(false);
        }

        bool success = TTSFramework::instance().update_speaker(speaker_id, x, y, z, volume, effects);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - TTS Framework: Error in ttsUpdateSpeaker - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value tts_stop_speaker_sqf(game_value_parameter speaker_id) {
    try {
        std::string id = speaker_id;

        if (id.empty()) {
            report_error("KH - TTS Framework: Speaker ID cannot be empty");
            return game_value(false);
        }

        bool success = TTSFramework::instance().stop_speaker(id);
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - TTS Framework: Error in ttsStopSpeaker - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value tts_is_playing_sqf(game_value_parameter speaker_id) {
    try {
        std::string id = speaker_id;

        if (id.empty()) {
            return game_value(false);
        }

        bool is_playing = TTSFramework::instance().is_playing(id);
        return game_value(is_playing);
    } catch (const std::exception& e) {
        report_error("KH - TTS Framework: Error in ttsIsPlaying - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value tts_stop_all_sqf() {
    try {
        TTSFramework::instance().stop_all();
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH - TTS Framework: Error in ttsStopAll - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value tts_is_initialized_sqf() {
    try {
        bool initialized = TTSFramework::instance().is_initialized();
        return game_value(initialized);
    } catch (const std::exception& e) {
        report_error("KH - TTS Framework: Error in ttsIsInitialized - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value stt_load_model_sqf(game_value_parameter model_name) {
    try {
        std::string model = model_name;
        return game_value(STTFramework::instance().load_model_public(model));
    } catch (const std::exception& e) {
        report_error("KH - STT Framework: sttLoadModel failed: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value stt_load_model_with_config_sqf(game_value_parameter model_name, game_value_parameter config) {
    try {
        std::string model = model_name;
        auto& config_arr = config.to_array();
        int threads = config_arr.size() > 0 ? static_cast<int>(static_cast<float>(config_arr[0])) : 4;
        return game_value(STTFramework::instance().load_model_public(model, threads));
    } catch (const std::exception& e) {
        report_error("KH - STT Framework: sttLoadModel failed: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value stt_is_initialized_sqf() {
    try {
        return game_value(STTFramework::instance().is_initialized_public());
    } catch (const std::exception& e) {
        report_error("KH - STT Framework: sttIsInitialized failed: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value stt_is_capturing_sqf() {
    try {
        return game_value(STTFramework::instance().is_capturing_audio_public());
    } catch (const std::exception& e) {
        report_error("KH - STT Framework: sttIsCapturing failed: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value stt_start_capture_sqf() {
    try {
        return game_value(STTFramework::instance().start_capture_public());
    } catch (const std::exception& e) {
        report_error("KH - STT Framework: sttStartCapture failed: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value stt_stop_capture_sqf() {
    try {
        return game_value(STTFramework::instance().stop_capture_public());
    } catch (const std::exception& e) {
        report_error("KH - STT Framework: sttStopCapture failed: " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_create_html_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string html_content = left_arg;

        if (html_content.empty()) {
            return game_value("");
        }

        int x = 0, y = 0, width = 0, height = 0;
        float opacity = 1.0f;
        auto& arr = right_arg.to_array();
        x = arr.size() > 0 ? static_cast<int>(static_cast<float>(arr[0])) : 0;
        y = arr.size() > 1 ? static_cast<int>(static_cast<float>(arr[1])) : 0;
        width = arr.size() > 2 ? static_cast<int>(static_cast<float>(arr[2])) : 0;
        height = arr.size() > 3 ? static_cast<int>(static_cast<float>(arr[3])) : 0;
        opacity = arr.size() > 4 ? static_cast<float>(arr[4]) : 1.0f;
        return game_value(UIFramework::instance().create_html(html_content, x, y, width, height, opacity));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlCreate - " + std::string(e.what()));
        return game_value("");
    }
}

static game_value ui_open_html_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string filename = left_arg;

        if (filename.empty()) {
            return game_value("");
        }

        int x = 0, y = 0, width = 0, height = 0;
        float opacity = 1.0f;

        if (right_arg.type_enum() == game_data_type::ARRAY) {
            auto& arr = right_arg.to_array();
            x = arr.size() > 0 ? static_cast<int>(static_cast<float>(arr[0])) : 0;
            y = arr.size() > 1 ? static_cast<int>(static_cast<float>(arr[1])) : 0;
            width = arr.size() > 2 ? static_cast<int>(static_cast<float>(arr[2])) : 0;
            height = arr.size() > 3 ? static_cast<int>(static_cast<float>(arr[3])) : 0;
            opacity = arr.size() > 4 ? static_cast<float>(arr[4]) : 1.0f;
        }

        return game_value(UIFramework::instance().open_html(filename, x, y, width, height, opacity));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlOpen - " + std::string(e.what()));
        return game_value("");
    }
}

static game_value ui_close_html_sqf(game_value_parameter args) {
    try {
        std::string doc_id = args;

        if (doc_id.empty()) {
            return game_value(false);
        }

        return game_value(UIFramework::instance().close_html(doc_id));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlClose - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_set_html_visible_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string doc_id = left_arg;

        if (doc_id.empty()) {
            return game_value(false);
        }

        bool visible = static_cast<bool>(right_arg);
        return game_value(UIFramework::instance().set_html_visible(doc_id, visible));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlSetVisible - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_get_open_documents_sqf() {
    try {
        auto docs = UIFramework::instance().get_open_documents();
        auto_array<game_value> result;
        result.reserve(docs.size());

        for (const auto& id : docs) {
            result.push_back(game_value(id));
        }

        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlGetOpenDocuments - " + std::string(e.what()));
        return game_value(auto_array<game_value>());
    }
}

static game_value ui_is_initialized_sqf() {
    try {
        return game_value(UIFramework::instance().is_initialized());
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlIsInitialized - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_execute_js_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string doc_id = left_arg;

        if (doc_id.empty()) {
            return game_value(false);
        }

        std::string script = right_arg;

        if (script.empty()) {
            return game_value(false);
        }

        return game_value(UIFramework::instance().execute_javascript(doc_id, script));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlExecuteJS - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_set_js_variable_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string doc_id = left_arg;

        if (doc_id.empty()) {
            return game_value(false);
        }

        if (right_arg.type_enum() != game_data_type::ARRAY) {
            return game_value(false);
        }

        auto& arr = right_arg.to_array();

        if (arr.size() < 2) {
            return game_value(false);
        }

        std::string var_name = arr[0];

        if (var_name.empty()) {
            return game_value(false);
        }

        std::string value_json = game_value_to_json(arr[1]);
        return game_value(UIFramework::instance().set_js_variable(doc_id, var_name, value_json));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlSetJsVariable - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_get_js_variable_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string doc_id = left_arg;

        if (doc_id.empty()) {
            return game_value();
        }

        std::string var_name = right_arg;

        if (var_name.empty()) {
            return game_value();
        }

        std::string json_result = UIFramework::instance().get_js_variable(doc_id, var_name);
        return json_to_game_value(json_result);
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlGetJsVariable - " + std::string(e.what()));
        return game_value();
    }
}

static game_value ui_set_position_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string doc_id = left_arg;

        if (doc_id.empty()) {
            return game_value(false);
        }

        if (right_arg.type_enum() != game_data_type::ARRAY) {
            return game_value(false);
        }

        auto& arr = right_arg.to_array();

        if (arr.size() < 2) {
            return game_value(false);
        }

        int x = static_cast<int>(static_cast<float>(arr[0]));
        int y = static_cast<int>(static_cast<float>(arr[1]));
        return game_value(UIFramework::instance().set_html_position(doc_id, x, y));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlSetPosition - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_set_opacity_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string doc_id = left_arg;

        if (doc_id.empty()) {
            return game_value(false);
        }

        float opacity = static_cast<float>(right_arg);
        return game_value(UIFramework::instance().set_html_opacity(doc_id, opacity));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlSetOpacity - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_set_size_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string doc_id = left_arg;

        if (doc_id.empty()) {
            return game_value(false);
        }

        if (right_arg.type_enum() != game_data_type::ARRAY) {
            return game_value(false);
        }

        auto& arr = right_arg.to_array();

        if (arr.size() < 2) {
            return game_value(false);
        }

        int width = static_cast<int>(static_cast<float>(arr[0]));
        int height = static_cast<int>(static_cast<float>(arr[1]));

        if (width <= 0 || height <= 0) {
            return game_value(false);
        }

        return game_value(UIFramework::instance().set_html_size(doc_id, width, height));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlSetSize - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_set_z_order_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string doc_id = left_arg;

        if (doc_id.empty()) {
            return game_value(false);
        }

        int z_order = static_cast<int>(static_cast<float>(right_arg));
        return game_value(UIFramework::instance().set_html_z_order(doc_id, z_order));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlSetZOrder - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_bring_to_front_sqf(game_value_parameter args) {
    try {
        std::string doc_id = args;

        if (doc_id.empty()) {
            return game_value(false);
        }

        return game_value(UIFramework::instance().bring_html_to_front(doc_id));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlBringToFront - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_send_to_back_sqf(game_value_parameter args) {
    try {
        std::string doc_id = args;

        if (doc_id.empty()) {
            return game_value(false);
        }

        return game_value(UIFramework::instance().send_html_to_back(doc_id));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlSendToBack - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ui_reload_html_sqf(game_value_parameter args) {
    try {
        std::string doc_id = args;

        if (doc_id.empty()) {
            return game_value("");
        }

        return game_value(UIFramework::instance().reload_html(doc_id));
    } catch (const std::exception& e) {
        report_error("KH - UI Framework: Error in htmlReload - " + std::string(e.what()));
        return game_value("");
    }
}

static game_value network_message_send_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        auto& arr = right_arg.to_array();

        if (arr.size() < 2) {
            report_error("KH Network: networkMessageSend requires [eventName, target]");
            return game_value(false);
        }

        std::string event_name = static_cast<std::string>(arr[0]);
        std::transform(event_name.begin(), event_name.end(), event_name.begin(), ::tolower);
        game_value target = arr[1];
        std::string jip_key = "";
        std::string dependency_net_id = "";
        bool dependency_is_group = false;
        bool unit_required = false;

        if (arr.size() > 2) {
            game_value jip_arg = arr[2];

            if (jip_arg.type_enum() == game_data_type::STRING) {
                jip_key = static_cast<std::string>(jip_arg);
            } else if (jip_arg.type_enum() == game_data_type::BOOL && static_cast<bool>(jip_arg)) {
                jip_key = UIDGenerator::generate();
            } else if (jip_arg.type_enum() == game_data_type::ARRAY) {
                // dependency can be object or group - will be stored as netId
                auto& jip_arr = jip_arg.to_array();

                if (!jip_arr.empty()) {
                    // [0] dependency (object or group; null/other = no
                    // dependency gating)
                    game_value dependency = jip_arr[0];

                    if (dependency.type_enum() == game_data_type::OBJECT) {
                        object dep_obj = static_cast<object>(dependency);

                        if (!sqf::is_null(dep_obj)) {
                            dependency_net_id = static_cast<std::string>(sqf::net_id(dep_obj));
                            dependency_is_group = false;
                        }
                    } else if (dependency.type_enum() == game_data_type::GROUP) {
                        group dep_grp = static_cast<group>(dependency);

                        if (!sqf::is_null(dep_grp)) {
                            dependency_net_id = static_cast<std::string>(sqf::net_id(dep_grp));
                            dependency_is_group = true;
                        }
                    }

                    // unitRequired
                    if (jip_arr.size() > 1 && jip_arr[1].type_enum() == game_data_type::BOOL) {
                        unit_required = static_cast<bool>(jip_arr[1]);
                    }

                    if (jip_arr.size() > 2 && jip_arr[2].type_enum() == game_data_type::STRING) {
                        std::string key_str = static_cast<std::string>(jip_arr[2]);
                        jip_key = key_str.empty() ? UIDGenerator::generate() : key_str;
                    } else {
                        jip_key = UIDGenerator::generate();
                    }
                }
            }
        }

        game_value message = left_arg;

        if (event_name.empty()) {
            report_error("KH Network: Event name cannot be empty");
            return game_value(false);
        }

        NetworkTargetType target_type;
        game_value target_data;
        bool target_is_code = (!target.is_nil() && target.type_enum() == game_data_type::CODE);

        // below stores the conditional wrapper instead (no
        // plain-then-replace).
        if (!jip_key.empty() && !target_is_code) {
            NetworkFramework::instance().store_jip_message(jip_key, event_name, message, static_cast<int>(sqf::client_owner()), dependency_net_id, dependency_is_group, unit_required);
        }

        if (target.is_nil()) {
            return (!jip_key.empty()) ? game_value(jip_key) : game_value(false);
        }

        auto type = target.type_enum();

        switch (type) {
            case game_data_type::SCALAR: {
                int client_id = static_cast<int>(static_cast<float>(target));

                if (client_id < 0) {
                    target_type = NetworkTargetType::CLIENT_ID_EXCLUDE;
                    target_data = target;
                } else {
                    bool success = NetworkFramework::instance().send_message(client_id, event_name, message);                    
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(success);
                }

                break;
            }

            case game_data_type::BOOL: {
                bool val = static_cast<bool>(target);

                if (!val) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::LOCAL_ONLY;
                break;
            }

            case game_data_type::OBJECT: {
                object target_obj = static_cast<object>(target);

                if (sqf::is_null(target_obj)) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::OBJECT_OWNER;
                target_data = target;
                break;
            }

            case game_data_type::GROUP: {
                group target_grp = static_cast<group>(target);

                if (sqf::is_null(target_grp)) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::GROUP_MEMBERS;
                target_data = target;
                break;
            }

            case game_data_type::TEAM_MEMBER: {
                game_value agent_obj = sqf::agent(target);

                if (agent_obj.is_nil() || sqf::is_null(static_cast<object>(agent_obj))) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::TEAM_MEMBER_OWNER;
                target_data = target;
                break;
            }

            case game_data_type::SIDE:
                target_type = NetworkTargetType::SIDE_MEMBERS;
                target_data = target;
                break;

            case game_data_type::LOCATION: {
                location target_loc = static_cast<location>(target);

                if (sqf::is_null(target_loc)) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::LOCATION_UNITS;
                target_data = target;
                break;
            }

            case game_data_type::CODE: {
                auto_array<game_value> cond_data;
                cond_data.push_back(target);
                cond_data.push_back(game_value(event_name));
                cond_data.push_back(message);
                game_value cond_payload(std::move(cond_data));

                bool success = NetworkFramework::instance().send_message_to_target(
                    NetworkTargetType::CODE_CONDITION,
                    game_value(),
                    NET_INTERNAL_CONDITIONAL_EVENT,
                    cond_payload
                );

                return (!jip_key.empty()) ? game_value(jip_key) : game_value(success);
            }

            case game_data_type::STRING: {
                std::string target_str = static_cast<std::string>(target);

                if (target_str.empty()) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                std::string target_upper = target_str;
                std::transform(target_upper.begin(), target_upper.end(), target_upper.begin(), ::toupper);

                if (target_upper == "SERVER") {
                    target_type = NetworkTargetType::STRING_SERVER;
                } else if (target_upper == "GLOBAL") {
                    target_type = NetworkTargetType::STRING_GLOBAL;
                } else if (target_upper == "LOCAL") {
                    target_type = NetworkTargetType::STRING_LOCAL;
                } else if (target_upper == "PLAYERS") {
                    target_type = NetworkTargetType::STRING_PLAYERS;
                } else if (target_upper == "REMOTE") {
                    target_type = NetworkTargetType::STRING_REMOTE;
                } else if (target_upper == "ADMIN") {
                    target_type = NetworkTargetType::STRING_ADMIN;
                } else if (target_upper == "HEADLESS") {
                    target_type = NetworkTargetType::STRING_HEADLESS;
                } else if (target_upper == "CURATORS") {
                    target_type = NetworkTargetType::STRING_CURATORS;
                } else {
                    target_type = NetworkTargetType::STRING_EXTENDED;
                    target_data = game_value(target_str);
                }

                break;
            }

            case game_data_type::ARRAY: {
                if (target.to_array().empty()) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::ARRAY_TARGETS;
                target_data = target;
                break;
            }

            default:
                return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
        }

        bool success = NetworkFramework::instance().send_message_to_target(
            target_type,
            target_data,
            event_name,
            message
        );

        return (!jip_key.empty()) ? game_value(jip_key) : game_value(success);
    } catch (const std::exception& e) {
        report_error("KH Network: Error in networkMessageSend - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value network_remove_jip_sqf(game_value_parameter jip_key_value) {
    try {        
        std::string jip_key = static_cast<std::string>(jip_key_value);

        if (jip_key.empty()) {
            report_error("KH Network: JIP key cannot be empty");
            return game_value(false);
        }

        NetworkFramework::instance().request_remove_jip(jip_key);
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH Network: Error in networkMessageRemoveJip - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value network_message_receive_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    auto fail = []() {
        auto_array<game_value> e;
        e.push_back(game_value(-1.0f));
        e.push_back(game_value(-1.0f));
        return game_value(std::move(e));
    };

    try {
        std::string event_name = static_cast<std::string>(left_arg);
        std::transform(event_name.begin(), event_name.end(), event_name.begin(), ::tolower);

        if (event_name.empty()) {
            report_error("KH Network: Event name cannot be empty");
            return fail();
        }

        game_value handler_args;
        code handler_function;

        if (right_arg.type_enum() == game_data_type::ARRAY) {
            auto& arr = right_arg.to_array();

            if (arr.size() < 2) {
                report_error("KH Network: networkMessageReceive requires [arguments, function] or just function");
                return fail();
            }

            handler_args = arr[0];

            if (arr[1].type_enum() == game_data_type::CODE) {
                handler_function = static_cast<code>(arr[1]);
            } else {
                report_error("KH Network: Handler must be code");
                return fail();
            }
        } else {
            handler_function = static_cast<code>(right_arg);
        }

        // Ensure network is initialized
        if (!NetworkFramework::instance().is_initialized()) {
            if (!NetworkFramework::instance().initialize()) {
                report_error("KH Network: Failed to initialize network framework");
                return fail();
            }
        }

        int handler_id = NetworkFramework::instance().add_message_handler(event_name, handler_function, handler_args);

        auto_array<game_value> result;
        result.push_back(game_value(static_cast<float>(handler_id)));
        result.push_back(game_value(static_cast<float>(sqf::client_owner())));
        return game_value(std::move(result));
    } catch (const std::exception& e) {
        report_error("KH Network: Error in networkMessageReceive - " + std::string(e.what()));
        return fail();
    }
}

static game_value network_is_initialized_sqf() {
    return game_value(NetworkFramework::instance().is_initialized());
}

static game_value network_remove_handler_sqf(game_value_parameter handler_value) {
    try {
        int handler_id = -1;
        int owner_id = -1;

        if (handler_value.type_enum() == game_data_type::ARRAY) {
            auto& a = handler_value.to_array();

            if (a.size() < 2) {
                report_error("KH Network: networkRemoveHandler requires [handlerId, owner]");
                return game_value(false);
            }

            handler_id = static_cast<int>(static_cast<float>(a[0]));
            owner_id = static_cast<int>(static_cast<float>(a[1]));
        } else {
            // Back-compat: a bare handler id targets a handler owned by this
            // machine.
            handler_id = static_cast<int>(static_cast<float>(handler_value));
            owner_id = static_cast<int>(sqf::client_owner());
        }

        if (handler_id < 0) {
            report_error("KH Network: Invalid handler ID");
            return game_value(false);
        }

        NetworkFramework::instance().request_remove_handler(handler_id, owner_id);
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH Network: Error in networkRemoveHandler - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value network_initialize_sqf() {
    try {
        network_pre_init();
        return game_value(true);
    } catch (...) {
        return game_value(false);
    }
}

static game_value network_shutdown_sqf() {
    try {
        NetworkFramework::instance().shutdown();
        return game_value(true);
    } catch (...) {
        return game_value(false);
    }
}

static game_value enable_network_logging_sqf(game_value_parameter enabled_value) {
    try {
        if (!sqf::is_server()) {
            return game_value(false);
        }

        bool enabled = static_cast<bool>(enabled_value);        
        NetworkFramework::instance().set_network_logging(enabled);
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH Network: Error in khNetworkLog - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value kh_set_variable_impl(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {        
        auto& arr = right_arg.to_array();

        if (arr.size() < 2) {
            report_error("KH SetVariable: Array must contain at least [name, value]");
            return game_value(false);
        }

        std::string var_name = static_cast<std::string>(arr[0]);
        std::transform(var_name.begin(), var_name.end(), var_name.begin(), ::tolower);
        game_value value = arr[1];
        game_value target = arr.size() > 2 ? arr[2] : game_value(true);

        if (var_name.empty()) {
            report_error("KH SetVariable: Variable name cannot be empty");
            return game_value(false);
        }

        std::string jip_key = "";
        std::string dependency_net_id = "";
        bool dependency_is_group = false;
        bool unit_required = false;

        if (arr.size() > 3) {
            game_value jip_arg = arr[3];

            if (jip_arg.type_enum() == game_data_type::STRING) {
                jip_key = static_cast<std::string>(jip_arg);
            } else if (jip_arg.type_enum() == game_data_type::BOOL && static_cast<bool>(jip_arg)) {
                jip_key = UIDGenerator::generate();
            } else if (jip_arg.type_enum() == game_data_type::ARRAY) {
                auto& jip_arr = jip_arg.to_array();

                if (!jip_arr.empty()) {
                    game_value dependency = jip_arr[0];

                    if (dependency.type_enum() == game_data_type::OBJECT) {
                        object dep_obj = static_cast<object>(dependency);

                        if (!sqf::is_null(dep_obj)) {
                            dependency_net_id = static_cast<std::string>(sqf::net_id(dep_obj));
                            dependency_is_group = false;
                        }
                    } else if (dependency.type_enum() == game_data_type::GROUP) {
                        group dep_grp = static_cast<group>(dependency);

                        if (!sqf::is_null(dep_grp)) {
                            dependency_net_id = static_cast<std::string>(sqf::net_id(dep_grp));
                            dependency_is_group = true;
                        }
                    }

                    // unitRequired
                    if (jip_arr.size() > 1 && jip_arr[1].type_enum() == game_data_type::BOOL) {
                        unit_required = static_cast<bool>(jip_arr[1]);
                    }

                    if (jip_arr.size() > 2 && jip_arr[2].type_enum() == game_data_type::STRING) {
                        std::string key_str = static_cast<std::string>(jip_arr[2]);
                        jip_key = key_str.empty() ? UIDGenerator::generate() : key_str;
                    } else {
                        jip_key = UIDGenerator::generate();
                    }
                }
            }
        }

        game_value ns_data = NetworkFramework::serialize_namespace_for_network(left_arg);
        auto_array<game_value> message_data;
        message_data.push_back(ns_data);
        message_data.push_back(game_value(var_name));
        message_data.push_back(value);
        game_value message(std::move(message_data));
        bool target_is_code = (!target.is_nil() && target.type_enum() == game_data_type::CODE);

        // Store the JIP message now, unless the target is CODE ??? the CODE
        // branch below stores the conditional wrapper instead (no
        // plain-then-replace).
        if (!jip_key.empty() && !target_is_code) {
            NetworkFramework::instance().store_jip_message(
                jip_key, 
                NET_INTERNAL_SET_VARIABLE_EVENT, 
                message, 
                static_cast<int>(sqf::client_owner()),
                dependency_net_id,
                dependency_is_group,
                unit_required
            );
        }

        NetworkTargetType target_type;
        game_value target_data;

        if (target.is_nil()) {
            return (!jip_key.empty()) ? game_value(jip_key) : game_value(false);
        }

        auto type = target.type_enum();

        switch (type) {
            case game_data_type::SCALAR: {
                int client_id = static_cast<int>(static_cast<float>(target));

                if (client_id < 0) {
                    target_type = NetworkTargetType::CLIENT_ID_EXCLUDE;
                    target_data = target;
                } else {
                    bool success = NetworkFramework::instance().send_message(
                        client_id, 
                        NET_INTERNAL_SET_VARIABLE_EVENT, 
                        message
                    );

                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(success);
                }

                break;
            }

            case game_data_type::BOOL: {
                bool val = static_cast<bool>(target);

                if (!val) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::LOCAL_ONLY;
                break;
            }

            case game_data_type::OBJECT: {
                object target_obj = static_cast<object>(target);

                if (sqf::is_null(target_obj)) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::OBJECT_OWNER;
                target_data = target;
                break;
            }

            case game_data_type::GROUP: {
                group target_grp = static_cast<group>(target);

                if (sqf::is_null(target_grp)) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::GROUP_MEMBERS;
                target_data = target;
                break;
            }

            case game_data_type::SIDE:
                target_type = NetworkTargetType::SIDE_MEMBERS;
                target_data = target;
                break;

            case game_data_type::LOCATION: {
                location target_loc = static_cast<location>(target);

                if (sqf::is_null(target_loc)) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::LOCATION_UNITS;
                target_data = target;
                break;
            }

            case game_data_type::CODE: {
                auto_array<game_value> cond_data;
                cond_data.push_back(target);
                cond_data.push_back(game_value(std::string(NET_INTERNAL_SET_VARIABLE_EVENT)));
                cond_data.push_back(message);
                game_value cond_payload(std::move(cond_data));

                bool success = NetworkFramework::instance().send_message_to_target(
                    NetworkTargetType::CODE_CONDITION,
                    game_value(),
                    NET_INTERNAL_CONDITIONAL_EVENT,
                    cond_payload
                );

                return (!jip_key.empty()) ? game_value(jip_key) : game_value(success);
            }

            case game_data_type::STRING: {
                std::string target_str = static_cast<std::string>(target);

                if (target_str.empty()) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                std::string target_upper = target_str;
                std::transform(target_upper.begin(), target_upper.end(), target_upper.begin(), ::toupper);

                if (target_upper == "SERVER") {
                    target_type = NetworkTargetType::STRING_SERVER;
                } else if (target_upper == "GLOBAL") {
                    target_type = NetworkTargetType::STRING_GLOBAL;
                } else if (target_upper == "LOCAL") {
                    target_type = NetworkTargetType::STRING_LOCAL;
                } else if (target_upper == "PLAYERS") {
                    target_type = NetworkTargetType::STRING_PLAYERS;
                } else if (target_upper == "REMOTE") {
                    target_type = NetworkTargetType::STRING_REMOTE;
                } else if (target_upper == "ADMIN") {
                    target_type = NetworkTargetType::STRING_ADMIN;
                } else if (target_upper == "HEADLESS") {
                    target_type = NetworkTargetType::STRING_HEADLESS;
                } else if (target_upper == "CURATORS") {
                    target_type = NetworkTargetType::STRING_CURATORS;
                } else {
                    target_type = NetworkTargetType::STRING_EXTENDED;
                    target_data = game_value(target_str);
                }

                break;
            }

            case game_data_type::ARRAY: {
                if (target.to_array().empty()) {
                    return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
                }

                target_type = NetworkTargetType::ARRAY_TARGETS;
                target_data = target;
                break;
            }

            default:
                return (!jip_key.empty()) ? game_value(jip_key) : game_value(true);
        }

        bool success = NetworkFramework::instance().send_message_to_target(
            target_type,
            target_data,
            NET_INTERNAL_SET_VARIABLE_EVENT,
            message
        );

        return (!jip_key.empty()) ? game_value(jip_key) : game_value(success);
    } catch (const std::exception& e) {
        report_error("KH SetVariable: Error - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ts_connect_sqf() {
    try {
        bool success = TeamspeakFramework::instance().initialize();

        if (success) {
            sqf::diag_log("KH - TeamSpeak: Connected to IPC");
        }

        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - TeamSpeak: Error in tsConnect - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ts_disconnect_sqf() {
    try {
        TeamspeakFramework::instance().cleanup();
        sqf::diag_log("KH - TeamSpeak: Disconnected from IPC");
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH - TeamSpeak: Error in tsDisconnect - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ts_apply_voice_effects_sqf(game_value_parameter params) {
    try {
        if (params.type_enum() != game_data_type::ARRAY) {
            report_error("KH - TeamSpeak: tsApplyVoiceEffects requires an array of effects");
            return game_value(false);
        }

        auto effects = TeamspeakFramework::parse_effects_from_args(params, 0);
        bool success = TeamspeakFramework::instance().apply_voice_effects(effects);        
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - TeamSpeak: Error in tsApplyVoiceEffects - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ts_clear_voice_effects_sqf() {
    try {
        bool success = TeamspeakFramework::instance().clear_voice_effects();        
        return game_value(success);
    } catch (const std::exception& e) {
        report_error("KH - TeamSpeak: Error in tsClearVoiceEffects - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ts_is_initialized_sqf() {
    try {
        return game_value(TeamspeakFramework::instance().is_initialized());
    } catch (const std::exception& e) {
        report_error("KH - TeamSpeak: Error in tsIsInitialized - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ts_is_plugin_active_sqf() {
    try {
        return game_value(TeamspeakFramework::instance().is_plugin_active());
    } catch (const std::exception& e) {
        report_error("KH - TeamSpeak: Error in tsIsPluginActive - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ts_is_connected_sqf() {
    try {
        return game_value(TeamspeakFramework::instance().is_connected());
    } catch (const std::exception& e) {
        report_error("KH - TeamSpeak: Error in tsIsConnected - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value ts_is_plugin_installed_sqf() {
    try {
        return game_value(TeamspeakFramework::is_plugin_installed());
    } catch (const std::exception& e) {
        report_error("KH - TeamSpeak: Error in tsIsPluginInstalled - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value serialize_function_impl(const game_value& function, bool is_public) {
    try {
        bool is_string = (function.type_enum() == game_data_type::STRING);
        std::string func_str;
        bool has_space = false;
        bool is_sqf_path = false;

        if (is_string) {
            func_str = static_cast<std::string>(function);

            // A native SQF command name is directly callable through
            // callSerializedFunction, so hand it straight back instead of
            // compiling it into a stored hash entry.
            if (kh_find_command(func_str) != nullptr) {
                return function;
            }

            has_space = (func_str.find(' ') != std::string::npos);
            is_sqf_path = (func_str.find(".sqf") != std::string::npos);

            // _parse == false: a bare global function name is returned
            // unchanged, no engine calls.
            if (!has_space && !is_sqf_path) {
                return function;
            }
        }

        rv_namespace ns = sqf::mission_namespace();
        std::string hash_str = static_cast<std::string>(sqf::hash_value(function));
        std::string public_marker = "kh_var_publicfunction_" + hash_str;

        auto compile_function = [&]() -> game_value {
            if (is_sqf_path && !has_space) {
                auto_array<game_value> cs;
                cs.push_back(function);
                cs.push_back(game_value(false));
                cs.push_back(game_value(std::string()));
                return game_value(sqf::compile_script(game_value(std::move(cs))));
            }

            return game_value(sqf::compile(func_str));
        };

        if (sqf::get_variable(ns, hash_str).is_nil()) {
            game_value compiled_function = is_string ? compile_function() : function;

            if (is_public && sqf::get_variable(ns, public_marker).is_nil()) {
                sqf::set_variable(ns, public_marker, game_value(true), game_value(true));
                sqf::set_variable(ns, hash_str, compiled_function, game_value(true));
            } else {
                sqf::set_variable(ns, hash_str, compiled_function);

                if (!sqf::is_server()) {
                    raw_call_sqf_args_native(g_compiled_kh_set_variable_generic, kh_make_array({game_value(ns), game_value(hash_str), game_value(compiled_function), game_value(2)}));
                }
            }
        } else if (is_public && sqf::get_variable(ns, public_marker).is_nil()) {
            sqf::set_variable(ns, public_marker, game_value(true), game_value(true));
            game_value compiled_function = is_string ? compile_function() : function;
            sqf::set_variable(ns, hash_str, compiled_function, game_value(true));
        }

        return game_value(hash_str);
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Failed to serialize function");
        return game_value();
    }
}

static game_value call_serialized_function_sqf(game_value_parameter arguments, game_value_parameter params) {
    try {
        auto& arr = params.to_array();
        game_value function = arr.size() > 0 ? arr[0] : game_value();
        bool unscheduled = arr.size() > 2 ? static_cast<bool>(arr[2]) : true;
        const bool args_nil = arguments.is_nil();
        static const r_string n_arguments("_thisarguments");
        static const r_string n_function("_thisfunction");
        auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;

        // Case 1: _function is CODE -> run it directly
        if (!function.is_nil() && function.type_enum() == game_data_type::CODE) {
            code fnc = static_cast<code>(function);

            if (unscheduled) {
                if (args_nil) {
                    game_state->set_local_variable(n_function, fnc);
                    return raw_call_sqf_native(g_compiled_sqf_generic_call);
                } else {
                    game_state->set_local_variable(n_arguments, arguments);
                    game_state->set_local_variable(n_function, fnc);
                    return raw_call_sqf_native(g_compiled_sqf_generic_call_args);
                }
            } else {
                if (args_nil) {
                    return sqf::spawn(game_value(auto_array<game_value>()), fnc);
                } else {
                    return sqf::spawn(arguments, fnc);
                }
            }
        }

        // Case 2: _function is a string name -> resolve from missionNamespace
        if (!function.is_nil() && function.type_enum() == game_data_type::STRING) {
            std::string fname = static_cast<std::string>(function);

            // Case 2a: native SQF command. Case-insensitive, O(1).
            kh_command_entry* entry = kh_find_command(fname);

            if (entry != nullptr) {
                static const r_string kh_n_left_argument("_khleftargument");
                static const r_string kh_n_right_argument("_khrightargument");
                int argument_count = 0;
                const auto_array<game_value>* argument_array = nullptr;

                if (!args_nil) {
                    if (arguments.type_enum() != game_data_type::ARRAY) {
                        report_error("KH Framework Extension - Command '" + entry->canonical_name
                                     + "' expects its arguments as an array");

                        return game_value();
                    }

                    argument_array = &arguments.to_array();
                    argument_count = static_cast<int>(argument_array->size());
                }

                if (argument_count > 2) {
                    report_error("KH Framework Extension - Command '" + entry->canonical_name
                                 + "' was given " + std::to_string(argument_count)
                                 + " arguments; SQF commands accept at most 2");

                    return game_value();
                }

                kh_command_variant& variant = unscheduled
                    ? entry->call_variant[argument_count]
                    : entry->spawn_variant[argument_count];

                if (!variant.exists) {
                    report_error("KH Framework Extension - Command '" + entry->canonical_name
                                 + "' has no variant taking " + std::to_string(argument_count)
                                 + " argument(s); supported argument counts: "
                                 + kh_command_arity_description(*entry));

                    return game_value();
                }

                kh_ensure_variant_compiled(variant);

                if (!unscheduled) {
                    return sqf::spawn(args_nil ? game_value(auto_array<game_value>()) : arguments,
                                      variant.compiled);
                }

                if (argument_count > 0) {
                    const auto_array<game_value>& args_ref = *argument_array;
                    game_state->set_local_variable(kh_n_right_argument, args_ref[argument_count - 1]);

                    if (argument_count == 2) {
                        game_state->set_local_variable(kh_n_left_argument, args_ref[0]);
                    }
                }

                return raw_call_sqf_native(variant.compiled);
            }

            // Case 2b: not a command -> resolve from missionNamespace
            // (unchanged)
            rv_namespace ns = sqf::mission_namespace();
            game_value stored = sqf::get_variable(ns, fname);

            if (!stored.is_nil() && stored.type_enum() == game_data_type::CODE) {
                code fnc = static_cast<code>(stored);

                if (unscheduled) {
                    if (args_nil) {
                        game_state->set_local_variable(n_function, fnc);
                        return raw_call_sqf_native(g_compiled_sqf_generic_call);
                    } else {
                        game_state->set_local_variable(n_arguments, arguments);
                        game_state->set_local_variable(n_function, fnc);
                        return raw_call_sqf_native(g_compiled_sqf_generic_call_args);
                    }
                } else {
                    if (args_nil) {
                        return sqf::spawn(game_value(auto_array<game_value>()), fnc);
                    } else {
                        return sqf::spawn(arguments, fnc);
                    }
                }
            }
        }

        // Case 3: fallback -> remote execute call
        game_value caller = arr.size() > 1 ? arr[1] : game_value(2);
        auto_array<game_value> special;
        special.push_back(game_value(std::string("CALLBACK")));
        auto_array<game_value> fn_wrap;
        fn_wrap.push_back(function);
        special.push_back(game_value(std::move(fn_wrap)));
        special.push_back(game_value(std::string("KH_fnc_retrieveSerializedFunction")));
        auto_array<game_value> inner;
        inner.push_back(arguments);
        inner.push_back(function);
        inner.push_back(caller);
        inner.push_back(game_value(unscheduled));
        auto_array<game_value> exec_args;
        exec_args.push_back(game_value(std::move(inner)));
        exec_args.push_back(game_value(std::string("KH_fnc_processRemoteSerializedFunction")));
        exec_args.push_back(game_value(std::string("SERVER")));
        exec_args.push_back(game_value(true));
        exec_args.push_back(game_value(std::move(special)));
        return kh_execute_impl(game_value(std::move(exec_args)));
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value kh_hashmap_get(const game_value& map, const game_value& key) {
    if (map.is_nil() || map.type_enum() != game_data_type::HASHMAP || key.type_enum() != game_data_type::STRING) {
        return game_value();
    }

    std::string key_str = static_cast<std::string>(key);
    auto& hashmap = map.to_hashmap();

    for (const auto& pair : hashmap) {
        if (pair.key.type_enum() == game_data_type::STRING && static_cast<std::string>(pair.key) == key_str) {
            return pair.value;
        }
    }

    return game_value();
}

static game_value kh_cba_local_event(const game_value& event_name, const game_value& arguments) {
    auto_array<game_value> event_params;
    event_params.push_back(event_name);
    event_params.push_back(arguments);
    return raw_call_sqf_args_native(g_compiled_kh_cba_local_event, game_value(std::move(event_params)));
}

static game_value kh_cba_server_event(const game_value& event_name, const game_value& arguments) {
    auto_array<game_value> event_params;
    event_params.push_back(event_name);
    event_params.push_back(arguments);
    return raw_call_sqf_args_native(g_compiled_kh_cba_server_event, game_value(std::move(event_params)));
}

static game_value kh_cba_owner_event(const game_value& event_name, const game_value& arguments, const game_value& owner_machine) {
    auto_array<game_value> event_params;
    event_params.push_back(event_name);
    event_params.push_back(arguments);
    event_params.push_back(owner_machine);
    return raw_call_sqf_args_native(g_compiled_kh_cba_owner_event, game_value(std::move(event_params)));
}

static game_value kh_cba_target_event(const game_value& event_name, const game_value& arguments, const game_value& event_target) {
    auto_array<game_value> event_params;
    event_params.push_back(event_name);
    event_params.push_back(arguments);
    event_params.push_back(event_target);
    return raw_call_sqf_args_native(g_compiled_kh_cba_target_event, game_value(std::move(event_params)));
}

// Native equivalent of SQF 'flatten'; nils are preserved as elements
static void kh_flatten_into(const game_value& value, auto_array<game_value>& out) {
    if (!value.is_nil() && value.type_enum() == game_data_type::ARRAY) {
        auto& arr = value.to_array();

        for (size_t i = 0; i < arr.size(); ++i) {
            kh_flatten_into(arr[i], out);
        }
    } else {
        out.push_back(value);
    }
}

// Dispatches a compiled dispatcher shim through the native KH_fnc_execute
// equivalent: execute [_arguments, _function, _target, true, false]
static game_value kh_cba_execute_remote(const game_value& execute_arguments, const code& function, const char* execute_target) {
    auto_array<game_value> exec_args;
    exec_args.push_back(execute_arguments);
    exec_args.push_back(game_value(function));
    exec_args.push_back(game_value(std::string(execute_target)));
    exec_args.push_back(game_value(true));
    exec_args.push_back(game_value(false));
    return kh_execute_impl(game_value(std::move(exec_args)));
}

static game_value kh_cba_owner_event_broadcast(const game_value& event_name, const game_value& arguments, const char* machines_variable, bool has_excluded_machine, float excluded_machine) {
    game_value machines = sqf::get_variable(sqf::mission_namespace(), machines_variable);

    if (!machines.is_nil() && machines.type_enum() == game_data_type::ARRAY) {
        auto& machine_list = machines.to_array();

        for (size_t i = 0; i < machine_list.size(); ++i) {
            if (machine_list[i].is_nil()) {
                continue;
            }

            if (has_excluded_machine && machine_list[i].type_enum() == game_data_type::SCALAR && static_cast<float>(machine_list[i]) == excluded_machine) {
                continue;
            }

            kh_cba_owner_event(event_name, arguments, machine_list[i]);
        }
    }

    return game_value();
}

static game_value kh_cba_dispatch_machine(const game_value& event_name, const game_value& arguments, const game_value& machine) {
    if (machine.type_enum() == game_data_type::SCALAR && static_cast<float>(machine) == static_cast<float>(sqf::client_owner())) {
        return kh_cba_local_event(event_name, arguments);
    }

    return kh_cba_owner_event(event_name, arguments, machine);
}

// Fires CBA_fnc_targetEvent at the subset of KH_var_allPlayerUnits matching
// the predicate; the event fires unconditionally, even on an empty selection,
// mirroring the original
template <typename Predicate>
static game_value kh_cba_target_event_player_units(const game_value& event_name, const game_value& arguments, Predicate predicate) {
    auto_array<game_value> selected_units;
    game_value player_units = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits");

    if (!player_units.is_nil() && player_units.type_enum() == game_data_type::ARRAY) {
        auto& units = player_units.to_array();

        for (size_t i = 0; i < units.size(); ++i) {
            if (units[i].is_nil() || units[i].type_enum() != game_data_type::OBJECT) {
                continue;
            }

            if (predicate(static_cast<object>(units[i]))) {
                selected_units.push_back(units[i]);
            }
        }
    }

    return kh_cba_target_event(event_name, arguments, game_value(std::move(selected_units)));
}

// Mirrors the SQF group dispatch: a local group with no involved player units
// fires locally, groups without player units use targetEvent, otherwise the
// server fans out per-owner events
static game_value kh_cba_dispatch_group(const game_value& event_name, const game_value& arguments, const game_value& target_group) {
    group grp = static_cast<group>(target_group);
    auto_array<game_value> group_player_units;
    game_value player_units = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits");

    if (!player_units.is_nil() && player_units.type_enum() == game_data_type::ARRAY) {
        auto& units = player_units.to_array();

        for (size_t i = 0; i < units.size(); ++i) {
            if (units[i].is_nil() || units[i].type_enum() != game_data_type::OBJECT) {
                continue;
            }

            if (sqf::is_equal_to(game_value(sqf::get_group(static_cast<object>(units[i]))), target_group)) {
                group_player_units.push_back(units[i]);
            }
        }
    }

    bool group_local = sqf::local(grp);
    bool player_in_group = false;
    game_value player_unit = game_value(sqf::player());

    for (size_t i = 0; i < group_player_units.size(); ++i) {
        if (sqf::is_equal_to(group_player_units[i], player_unit)) {
            player_in_group = true;
            break;
        }
    }

    if ((group_local && group_player_units.empty()) || (group_local && player_in_group && group_player_units.size() <= 1)) {
        return kh_cba_local_event(event_name, arguments);
    }

    if (group_player_units.empty()) {
        return kh_cba_target_event(event_name, arguments, target_group);
    }

    return kh_cba_execute_remote(kh_make_array({event_name, arguments, target_group}), g_compiled_kh_cba_group_owner_dispatch, "SERVER");
}

// Shared local fallback for string targets: player name/roleDescription match
// -> targetEvent; groupId match -> per-group dispatch; marker match ->
// targetEvent on units in area
static game_value kh_cba_dispatch_named_target(const game_value& event_name, const game_value& arguments, const std::string& target_string) {
    auto_array<game_value> player_targets;
    game_value player_units = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits");

    if (!player_units.is_nil() && player_units.type_enum() == game_data_type::ARRAY) {
        auto& units = player_units.to_array();

        for (size_t i = 0; i < units.size(); ++i) {
            if (units[i].is_nil() || units[i].type_enum() != game_data_type::OBJECT) {
                continue;
            }

            object unit = static_cast<object>(units[i]);

            if (sqf::name(unit) == target_string || sqf::role_description(unit) == target_string) {
                player_targets.push_back(units[i]);
            }
        }
    }

    if (!player_targets.empty()) {
        return kh_cba_target_event(event_name, arguments, game_value(std::move(player_targets)));
    }

    auto groups = sqf::all_groups();
    bool group_matched = false;
    game_value group_return;

    for (auto& current_group : groups) {
        if (sqf::group_id(current_group) == target_string) {
            group_matched = true;
            group_return = kh_cba_dispatch_group(event_name, arguments, game_value(current_group));
        }
    }

    if (group_matched) {
        return group_return;
    }

    auto markers = sqf::all_map_markers();

    for (size_t i = 0; i < markers.size(); ++i) {
        if (markers[i] == target_string) {
            return kh_cba_target_event_player_units(event_name, arguments, [&](const object& unit) {
                return sqf::in_area(unit, target_string);
            });
        }
    }

    return game_value();
}

// Native KH_fnc_triggerCbaEvent, registered as the unary 'triggerCbaEvent'
// command; right arg is [_event, _arguments, _target, _jip]
static game_value trigger_cba_event_sqf(game_value_parameter params) {
    try {
        if (params.type_enum() != game_data_type::ARRAY) return game_value();
        auto& p = params.to_array();
        game_value event = kh_param(p, 0, game_value(std::string()), {game_data_type::STRING, game_data_type::ARRAY});
        game_value arguments = p.size() > 1 ? p[1] : game_value();
        game_value target = kh_param(p, 2, game_value(true), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY, game_data_type::CODE, game_data_type::OBJECT, game_data_type::TEAM_MEMBER, game_data_type::GROUP, game_data_type::SIDE, game_data_type::LOCATION});
        game_value jip = kh_param(p, 3, game_value(false), {game_data_type::BOOL, game_data_type::ARRAY});

        // _event as [_eventName, _entity] -> ["KH_eve_entityCbaEvent",
        // hashValue _entity, _eventName] joinString "_"
        if (event.type_enum() == game_data_type::ARRAY) {
            auto& ev = event.to_array();
            std::string event_name = static_cast<std::string>(kh_param(ev, 0, game_value(std::string()), {game_data_type::STRING}));
            game_value entity = kh_param(ev, 1, game_value(sqf::obj_null()), {game_data_type::OBJECT, game_data_type::GROUP});
            event = game_value("KH_eve_entityCbaEvent_" + static_cast<std::string>(sqf::hash_value(entity)) + "_" + event_name);
        }

        game_value return_value;

        switch (target.type_enum()) {
            case game_data_type::BOOL: {
                if (static_cast<bool>(target)) {
                    return_value = kh_cba_local_event(event, arguments);
                }

                break;
            }

            case game_data_type::SCALAR: {
                float target_machine = static_cast<float>(target);

                if (target_machine == static_cast<float>(sqf::client_owner())) {
                    return_value = kh_cba_local_event(event, arguments);
                } else if (target_machine >= 0.0f) {
                    return_value = kh_cba_owner_event(event, arguments, target);
                } else {
                    kh_cba_owner_event_broadcast(event, arguments, "kh_var_allmachines", true, std::fabs(target_machine));
                }

                break;
            }

            case game_data_type::OBJECT: {
                if (sqf::local(static_cast<object>(target))) {
                    return_value = kh_cba_local_event(event, arguments);
                } else {
                    return_value = kh_cba_target_event(event, arguments, target);
                }

                break;
            }

            case game_data_type::TEAM_MEMBER: {
                object agent_unit = sqf::agent(static_cast<team_member>(target));

                if (sqf::local(agent_unit)) {
                    return_value = kh_cba_local_event(event, arguments);
                } else {
                    return_value = kh_cba_target_event(event, arguments, game_value(agent_unit));
                }

                break;
            }

            case game_data_type::GROUP: {
                return_value = kh_cba_dispatch_group(event, arguments, target);
                break;
            }

            case game_data_type::SIDE: {
                return_value = kh_cba_target_event_player_units(event, arguments, [&](const object& unit) {
                    return sqf::is_equal_to(game_value(sqf::get_side(sqf::get_group(unit))), target);
                });

                break;
            }

            case game_data_type::STRING: {
                std::string target_string = static_cast<std::string>(target);

                if (target_string == "LOCAL") {
                    return_value = kh_cba_local_event(event, arguments);
                } else if (target_string == "SERVER") {
                    if (sqf::is_server()) {
                        return_value = kh_cba_local_event(event, arguments);
                    } else {
                        return_value = kh_cba_server_event(event, arguments);
                    }
                } else if (target_string == "GLOBAL") {
                    kh_cba_owner_event_broadcast(event, arguments, "kh_var_allmachines", false, 0.0f);
                } else if (target_string == "REMOTE") {
                    kh_cba_owner_event_broadcast(event, arguments, "kh_var_allmachines", true, static_cast<float>(sqf::client_owner()));
                } else if (target_string == "PLAYERS") {
                    kh_cba_owner_event_broadcast(event, arguments, "kh_var_allplayermachines", false, 0.0f);
                } else if (target_string == "ADMIN") {
                    game_value admin_machine = sqf::get_variable(sqf::mission_namespace(), "kh_var_adminmachine");

                    if (!admin_machine.is_nil() && admin_machine.type_enum() == game_data_type::SCALAR && static_cast<float>(admin_machine) == static_cast<float>(sqf::client_owner())) {
                        return_value = kh_cba_local_event(event, arguments);
                    } else {
                        return_value = kh_cba_owner_event(event, arguments, admin_machine);
                    }
                } else if (target_string == "CURATORS") {
                    return_value = kh_cba_target_event_player_units(event, arguments, [](const object& unit) {
                        return !sqf::is_null(sqf::get_assigned_curator_logic(unit));
                    });
                } else if (target_string == "HEADLESS") {
                    kh_cba_owner_event_broadcast(event, arguments, "kh_var_allheadlessmachines", false, 0.0f);
                } else {
                    // parseNumber on a non-numeric first character yields 0,
                    // which is the fault check; owner ids, uids and net ids
                    // never start with 0
                    if (!target_string.empty() && parse_number(target_string.substr(0, 1)) != 0.0f) {
                        if (target_string.find(':') == std::string::npos) {
                            const char* machine_maps[] = {"kh_var_allplayeruidmachines", "kh_var_allplayeridmachines", "kh_var_allheadlessidmachines"};
                            bool dispatched = false;

                            for (const char* map_name : machine_maps) {
                                game_value machine_map = sqf::get_variable(sqf::mission_namespace(), map_name);

                                if (machine_map.is_nil()) {
                                    continue;
                                }

                                game_value client = kh_hashmap_get(machine_map, target);

                                if (!client.is_nil()) {
                                    return_value = kh_cba_dispatch_machine(event, arguments, client);
                                    dispatched = true;
                                    break;
                                }
                            }

                            if (!dispatched) {
                                return_value = kh_cba_dispatch_named_target(event, arguments, target_string);
                            }
                        } else {
                            object net_object = sqf::object_from_net_id(target_string);

                            if (!sqf::is_null(net_object)) {
                                if (sqf::local(net_object)) {
                                    return_value = kh_cba_local_event(event, arguments);
                                } else {
                                    return_value = kh_cba_target_event(event, arguments, game_value(net_object));
                                }
                            } else {
                                group net_group = sqf::group_from_net_id(target_string);

                                if (!sqf::is_null(net_group)) {
                                    return_value = kh_cba_dispatch_group(event, arguments, game_value(net_group));
                                } else {
                                    return_value = kh_cba_dispatch_named_target(event, arguments, target_string);
                                }
                            }
                        }
                    } else {
                        return_value = kh_cba_dispatch_named_target(event, arguments, target_string);
                    }
                }

                break;
            }

            case game_data_type::ARRAY: {
                auto_array<game_value> flattened_targets;
                kh_flatten_into(target, flattened_targets);
                return_value = kh_cba_execute_remote(
                    kh_make_array({event, arguments, game_value(std::move(flattened_targets)), game_value(static_cast<float>(sqf::client_owner()))}),
                    g_compiled_kh_cba_array_target_dispatch,
                    "SERVER"
                );

                break;
            }

            case game_data_type::CODE: {
                return_value = kh_cba_execute_remote(
                    kh_make_array({event, arguments, target}),
                    g_compiled_kh_cba_code_target_dispatch,
                    "GLOBAL"
                );

                break;
            }

            case game_data_type::LOCATION: {
                location target_location = static_cast<location>(target);
                return_value = kh_cba_target_event_player_units(event, arguments, [&](const object& unit) {
                    return sqf::in_area(unit, target_location);
                });

                break;
            }

            default: {
                break;
            }
        }

        // _jip isNotEqualTo false
        if (!(jip.type_enum() == game_data_type::BOOL && !static_cast<bool>(jip))) {
            game_value dependency = game_value(true);
            game_value unit_required = game_value(false);
            std::string jip_id;

            if (jip.type_enum() == game_data_type::ARRAY) {
                auto& j = jip.to_array();
                dependency = kh_param(j, 0, game_value(true), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY, game_data_type::CODE, game_data_type::OBJECT, game_data_type::TEAM_MEMBER, game_data_type::GROUP});
                unit_required = kh_param(j, 1, game_value(false), {game_data_type::BOOL});
                jip_id = static_cast<std::string>(kh_param(j, 2, game_value(std::string()), {game_data_type::STRING}));
            }

            if (jip_id.empty()) {
                jip_id = UIDGenerator::generate();
            }

            kh_cba_server_event(game_value("KH_eve_jipSetup"), kh_make_array({event, arguments, dependency, unit_required, game_value(jip_id)}));
            return kh_make_array({game_value(sqf::mission_namespace()), game_value(jip_id), game_value(2)});
        }

        return return_value;
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("An unknown error occurred in triggerCbaEvent");
        return game_value();
    }
}

// pushBackUnique with SQF isEqualTo semantics
static void kh_push_back_unique(auto_array<game_value>& arr, const game_value& value) {
    for (size_t i = 0; i < arr.size(); ++i) {
        if (sqf::is_equal_to(arr[i], value)) {
            return;
        }
    }

    arr.push_back(value);
}

// _parsedTargets insert [-1, KH_var_allPlayerUnits select {...}, true]
template <typename Predicate>
static void kh_insert_unique_player_unit_owners(auto_array<game_value>& out, Predicate predicate) {
    game_value player_units = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits");

    if (!player_units.is_nil() && player_units.type_enum() == game_data_type::ARRAY) {
        auto& units = player_units.to_array();

        for (size_t i = 0; i < units.size(); ++i) {
            if (units[i].is_nil() || units[i].type_enum() != game_data_type::OBJECT) {
                continue;
            }

            object unit = static_cast<object>(units[i]);

            if (predicate(unit)) {
                kh_push_back_unique(out, game_value(sqf::owner(unit)));
            }
        }
    }
}

// Unique-inserts a KH machine list, optionally excluding one machine
// (KH_var_allMachines - [_caller] semantics)
static void kh_insert_unique_machines(auto_array<game_value>& parsed_targets, const char* machines_variable, const game_value* excluded_machine) {
    game_value machines = sqf::get_variable(sqf::mission_namespace(), machines_variable);

    if (!machines.is_nil() && machines.type_enum() == game_data_type::ARRAY) {
        auto& machine_list = machines.to_array();

        for (size_t i = 0; i < machine_list.size(); ++i) {
            if (machine_list[i].is_nil()) {
                continue;
            }

            if (excluded_machine && sqf::is_equal_to(machine_list[i], *excluded_machine)) {
                continue;
            }

            kh_push_back_unique(parsed_targets, machine_list[i]);
        }
    }
}

// Server-side named-target fallback for the array dispatcher:
// name/roleDescription -> unit targets; groupId -> unit owners; marker ->
// units in area
static void kh_cba_array_collect_named_target(auto_array<game_value>& parsed_targets, const std::string& target_string) {
    auto_array<game_value> player_matches;
    game_value player_units = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits");

    if (!player_units.is_nil() && player_units.type_enum() == game_data_type::ARRAY) {
        auto& units = player_units.to_array();

        for (size_t i = 0; i < units.size(); ++i) {
            if (units[i].is_nil() || units[i].type_enum() != game_data_type::OBJECT) {
                continue;
            }

            object unit = static_cast<object>(units[i]);

            if (sqf::name(unit) == target_string || sqf::role_description(unit) == target_string) {
                player_matches.push_back(game_value(sqf::owner(unit)));
            }
        }
    }

    if (!player_matches.empty()) {
        for (size_t i = 0; i < player_matches.size(); ++i) {
            kh_push_back_unique(parsed_targets, player_matches[i]);
        }

        return;
    }

    auto groups = sqf::all_groups();
    bool group_matched = false;

    for (auto& current_group : groups) {
        if (sqf::group_id(current_group) == target_string) {
            group_matched = true;
            auto units = sqf::units(current_group);

            for (auto& unit : units) {
                kh_push_back_unique(parsed_targets, game_value(sqf::owner(unit)));
            }
        }
    }

    if (group_matched) {
        return;
    }

    auto markers = sqf::all_map_markers();

    for (size_t i = 0; i < markers.size(); ++i) {
        if (markers[i] == target_string) {
            kh_insert_unique_player_unit_owners(parsed_targets, [&](const object& unit) {
                return sqf::in_area(unit, target_string);
            });

            return;
        }
    }
}

// Native group fan-out, registered as 'processCbaGroupEvent'; runs on the
// server via the dispatch shim. Format [event, arguments, group]
static game_value process_cba_group_event_sqf(game_value_parameter params) {
    try {
        if (params.type_enum() != game_data_type::ARRAY) return game_value();
        auto& p = params.to_array();
        game_value event = p.size() > 0 ? p[0] : game_value();
        game_value arguments = p.size() > 1 ? p[1] : game_value();
        game_value target = kh_param(p, 2, game_value(), {game_data_type::GROUP});

        if (target.is_nil()) {
            return game_value();
        }

        auto units = sqf::units(static_cast<group>(target));
        auto_array<game_value> owners;

        for (auto& unit : units) {
            kh_push_back_unique(owners, game_value(sqf::owner(unit)));
        }

        for (size_t i = 0; i < owners.size(); ++i) {
            kh_cba_owner_event(event, arguments, owners[i]);
        }

        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("An unknown error occurred in processCbaGroupEvent");
        return game_value();
    }
}

// Native predicate dispatch, registered as 'processCbaCodeEvent'; runs on
// every machine via the dispatch shim. Format [event, arguments, function]
static game_value process_cba_code_event_sqf(game_value_parameter params) {
    try {
        if (params.type_enum() != game_data_type::ARRAY) return game_value();
        auto& p = params.to_array();
        game_value event = p.size() > 0 ? p[0] : game_value();
        game_value arguments = kh_param(p, 1, game_value(auto_array<game_value>()), {});
        game_value function = kh_param(p, 2, game_value(g_compiled_kh_empty_code), {game_data_type::CODE});
        static const r_string n_arguments("_thisarguments");
        static const r_string n_function("_thisfunction");
        auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;
        game_state->set_local_variable(n_arguments, arguments);
        game_state->set_local_variable(n_function, function);
        game_value result = raw_call_sqf_native(g_compiled_sqf_generic_call_args);

        if (!result.is_nil() && result.type_enum() == game_data_type::BOOL && static_cast<bool>(result)) {
            return kh_cba_local_event(event, arguments);
        }

        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("An unknown error occurred in processCbaCodeEvent");
        return game_value();
    }
}

// Native array target parser, registered as 'processCbaArrayEvent'; runs on
// the server via the dispatch shim. Format [event, arguments,
// flattenedTargets, caller]
static game_value process_cba_array_event_sqf(game_value_parameter params) {
    try {
        if (params.type_enum() != game_data_type::ARRAY) return game_value();
        auto& p = params.to_array();
        game_value event = p.size() > 0 ? p[0] : game_value();
        game_value arguments = p.size() > 1 ? p[1] : game_value();
        game_value targets = kh_param(p, 2, game_value(auto_array<game_value>()), {game_data_type::ARRAY});
        game_value caller = kh_param(p, 3, game_value(static_cast<float>(sqf::client_owner())), {game_data_type::SCALAR});

        // Built inside a game_value from the start so the CODE-case callback
        // closure shares the same live array for deduplication, mirroring SQF
        // reference semantics
        game_value parsed_targets_value = game_value(auto_array<game_value>());
        auto& parsed_targets = parsed_targets_value.to_array();
        auto& target_list = targets.to_array();

        for (size_t i = 0; i < target_list.size(); ++i) {
            const game_value& element = target_list[i];

            if (element.is_nil()) {
                continue;
            }

            switch (element.type_enum()) {
                case game_data_type::BOOL: {
                    if (static_cast<bool>(element)) {
                        kh_push_back_unique(parsed_targets, caller);
                    }

                    break;
                }

                case game_data_type::SCALAR: {
                    kh_push_back_unique(parsed_targets, element);
                    break;
                }

                case game_data_type::OBJECT: {
                    kh_push_back_unique(parsed_targets, game_value(sqf::owner(static_cast<object>(element))));
                    break;
                }

                case game_data_type::TEAM_MEMBER: {
                    kh_push_back_unique(parsed_targets, game_value(sqf::owner(sqf::agent(static_cast<team_member>(element)))));
                    break;
                }

                case game_data_type::GROUP: {
                    auto units = sqf::units(static_cast<group>(element));

                    for (auto& unit : units) {
                        kh_push_back_unique(parsed_targets, game_value(sqf::owner(unit)));
                    }

                    break;
                }

                case game_data_type::SIDE: {
                    kh_insert_unique_player_unit_owners(parsed_targets, [&](const object& unit) {
                        return sqf::is_equal_to(game_value(sqf::get_side(sqf::get_group(unit))), element);
                    });

                    break;
                }

                case game_data_type::STRING: {
                    std::string target_string = static_cast<std::string>(element);

                    if (target_string == "LOCAL") {
                        kh_push_back_unique(parsed_targets, caller);
                    } else if (target_string == "SERVER") {
                        kh_push_back_unique(parsed_targets, game_value(2));
                    } else if (target_string == "GLOBAL") {
                        kh_insert_unique_machines(parsed_targets, "kh_var_allmachines", nullptr);
                    } else if (target_string == "REMOTE") {
                        kh_insert_unique_machines(parsed_targets, "kh_var_allmachines", &caller);
                    } else if (target_string == "PLAYERS") {
                        kh_insert_unique_machines(parsed_targets, "kh_var_allplayermachines", nullptr);
                    } else if (target_string == "ADMIN") {
                        kh_push_back_unique(parsed_targets, sqf::get_variable(sqf::mission_namespace(), "kh_var_adminmachine"));
                    } else if (target_string == "CURATORS") {
                        kh_insert_unique_player_unit_owners(parsed_targets, [](const object& unit) {
                            return !sqf::is_null(sqf::get_assigned_curator_logic(unit));
                        });
                    } else if (target_string == "HEADLESS") {
                        kh_insert_unique_machines(parsed_targets, "kh_var_allheadlessmachines", nullptr);
                    } else {
                        // parseNumber fault check; a non-numeric first
                        // character means it cannot be an id, uid or net id
                        if (!target_string.empty() && parse_number(target_string.substr(0, 1)) != 0.0f) {
                            if (target_string.find(':') == std::string::npos) {
                                const char* machine_maps[] = {"kh_var_allplayeruidmachines", "kh_var_allplayeridmachines", "kh_var_allheadlessidmachines"};
                                bool matched = false;

                                for (const char* map_name : machine_maps) {
                                    game_value machine_map = sqf::get_variable(sqf::mission_namespace(), map_name);

                                    if (machine_map.is_nil()) {
                                        continue;
                                    }

                                    game_value client = kh_hashmap_get(machine_map, element);

                                    if (!client.is_nil()) {
                                        kh_push_back_unique(parsed_targets, client);
                                        matched = true;
                                        break;
                                    }
                                }

                                if (!matched) {
                                    kh_cba_array_collect_named_target(parsed_targets, target_string);
                                }
                            } else {
                                object net_object = sqf::object_from_net_id(target_string);

                                if (!sqf::is_null(net_object)) {
                                    kh_push_back_unique(parsed_targets, game_value(sqf::owner(net_object)));
                                } else {
                                    group net_group = sqf::group_from_net_id(target_string);

                                    if (!sqf::is_null(net_group)) {
                                        auto units = sqf::units(net_group);

                                        for (auto& unit : units) {
                                            kh_push_back_unique(parsed_targets, game_value(sqf::owner(unit)));
                                        }
                                    } else {
                                        kh_cba_array_collect_named_target(parsed_targets, target_string);
                                    }
                                }
                            }
                        } else {
                            kh_cba_array_collect_named_target(parsed_targets, target_string);
                        }
                    }

                    break;
                }

                case game_data_type::CODE: {
                    auto_array<game_value> callback_special;
                    callback_special.push_back(game_value(std::string("CALLBACK")));
                    callback_special.push_back(kh_make_array({arguments, element}));
                    callback_special.push_back(game_value(g_compiled_kh_cba_callback_predicate));
                    auto_array<game_value> exec_args;
                    exec_args.push_back(kh_make_array({event, arguments, parsed_targets_value}));
                    exec_args.push_back(game_value(g_compiled_kh_cba_callback_receiver));
                    exec_args.push_back(game_value(std::string("GLOBAL")));
                    exec_args.push_back(game_value(true));
                    exec_args.push_back(game_value(std::move(callback_special)));
                    kh_execute_impl(game_value(std::move(exec_args)));
                    break;
                }

                case game_data_type::LOCATION: {
                    location target_location = static_cast<location>(element);
                    kh_insert_unique_player_unit_owners(parsed_targets, [&](const object& unit) {
                        return sqf::in_area(unit, target_location);
                    });

                    break;
                }

                default: {
                    break;
                }
            }
        }

        for (size_t i = 0; i < parsed_targets.size(); ++i) {
            kh_cba_owner_event(event, arguments, parsed_targets[i]);
        }

        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("An unknown error occurred in processCbaArrayEvent");
        return game_value();
    }
}

static game_value kh_trigger_cba_event_native(game_value event_name, game_value args, game_value target, game_value jip) {
    auto_array<game_value> cba_params;
    cba_params.push_back(std::move(event_name));
    cba_params.push_back(std::move(args));
    cba_params.push_back(std::move(target));
    cba_params.push_back(std::move(jip));
    return trigger_cba_event_sqf(game_value(std::move(cba_params)));
}

static void kh_trigger_stack_handler(const std::string& environment_id, bool delete_handler, bool override_timeout_on_deletion, bool condition_failure) {
    kh_trigger_cba_event_native(
        game_value("KH_eve_temporalExecutionStackHandler"),
        kh_make_array({game_value(environment_id), game_value(delete_handler), game_value(override_timeout_on_deletion), game_value(condition_failure)}),
        game_value(true),
        game_value(false)
    );
}

static void kh_push_temporal_addition(const game_value& entry, bool prepend) {
    auto& additions = g_kh_cached_temporal_additions.to_array();

    if (prepend) {
        auto_array<game_value> single;
        single.push_back(entry);
        additions.insert(additions.begin(), single.begin(), single.end());
    } else {
        additions.push_back(entry);
    }
}

static void kh_monitor_set(const std::string& environment_id, game_value entry) {
    auto_array<game_value> set_params;
    set_params.push_back(game_value(environment_id));
    set_params.push_back(std::move(entry));
    raw_call_sqf_args_native(g_compiled_kh_monitor_set, game_value(std::move(set_params)));
}

static game_value kh_due_time(float delay) {
    if (delay == 0.0f) return game_value(sqf::diag_frameno() + 1.0f);
    if (delay > 0.0f) return game_value(sqf::diag_ticktime() + delay);
    return game_value(sqf::diag_frameno() + std::fabs(delay));
}

// Mirrors the private declarations at the top of the original immediate
// blocks, so user functions can still read them through SQF's shared call
// scope
static game_value kh_immediate_call(const game_value& arguments, const game_value& function, const game_value& handler_id, float execution_time) {
    auto_array<game_value> wrapper_params;
    wrapper_params.push_back(arguments);
    wrapper_params.push_back(function);
    wrapper_params.push_back(handler_id);
    wrapper_params.push_back(game_value(execution_time));
    return raw_call_sqf_args_native(g_compiled_kh_immediate_call, game_value(std::move(wrapper_params)));
}

struct KHSpecialParseResult {
    game_value return_value;
    std::string special_id_override;
};

// Native KH_fnc_parseSpecialExecution
static KHSpecialParseResult kh_parse_special_execution(const game_value& special_in, const game_value& target) {
    KHSpecialParseResult result;
    result.return_value = game_value(auto_array<game_value>());
    result.special_id_override = "";
    game_value special = special_in;

    if (special.type_enum() == game_data_type::BOOL && static_cast<bool>(special)) {
        special = kh_make_array({game_value("JIP"), game_value(true), game_value(false), game_value(std::string())});
    }

    // Covers _special isEqualTo false and any non-array leftovers
    if (special.type_enum() != game_data_type::ARRAY) {
        return result;
    }

    auto& sp = special.to_array();
    std::string special_type = static_cast<std::string>(kh_param(sp, 0, game_value(std::string()), {game_data_type::STRING}));

    if (special_type == "JIP" || special_type == "PLAYER_PRESENCE") {
        const size_t id_index = (special_type == "JIP") ? 3 : 5;
        std::string special_id = static_cast<std::string>(kh_param(sp, id_index, game_value(std::string()), {game_data_type::STRING}));
        result.special_id_override = special_id.empty() ? UIDGenerator::generate() : special_id;
        result.return_value = kh_make_array({game_value(sqf::mission_namespace()), game_value(result.special_id_override), game_value(2)});
    } else if (special_type == "PERSISTENT") {
        std::string special_id = static_cast<std::string>(kh_param(sp, 4, game_value(std::string()), {game_data_type::STRING}));
        result.special_id_override = special_id.empty() ? UIDGenerator::generate() : special_id;
        result.return_value = kh_make_array({target, game_value(result.special_id_override), game_value(true)});
    }

    return result;
}

// Native KH_fnc_processExecution; also registered as the internal
// 'processExecution' command so deferred temporal stack entries can reach it
static game_value process_execution_sqf(game_value_parameter execute_params) {
    try {
        if (execute_params.type_enum() != game_data_type::ARRAY) return game_value();
        auto& p = execute_params.to_array();
        game_value arguments = p.size() > 0 ? p[0] : game_value();
        game_value function = kh_param(p, 1, game_value(g_compiled_kh_empty_code), {game_data_type::STRING, game_data_type::CODE});
        game_value target = kh_param(p, 2, game_value(true), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY, game_data_type::CODE, game_data_type::OBJECT, game_data_type::TEAM_MEMBER, game_data_type::GROUP, game_data_type::SIDE, game_data_type::LOCATION});
        game_value special = kh_param(p, 3, game_value(false), {game_data_type::BOOL, game_data_type::ARRAY});
        std::string special_id_override = static_cast<std::string>(kh_param(p, 4, game_value(std::string()), {game_data_type::STRING}));
        game_value unscheduled = kh_param(p, 5, game_value(true), {game_data_type::BOOL});
        game_value client_owner = game_value(static_cast<float>(sqf::client_owner()));

        if (special.type_enum() == game_data_type::BOOL && !static_cast<bool>(special)) {
            return kh_trigger_cba_event_native(
                game_value("KH_eve_execution"),
                kh_make_array({arguments, function, client_owner, unscheduled}),
                target,
                game_value(false)
            );
        }

        if (special.type_enum() == game_data_type::BOOL && static_cast<bool>(special)) {
            special = kh_make_array({game_value("JIP"), game_value(true), game_value(false), game_value(std::string())});
        }

        if (special.type_enum() != game_data_type::ARRAY) return game_value();
        auto& sp = special.to_array();
        std::string special_type = static_cast<std::string>(kh_param(sp, 0, game_value(std::string()), {game_data_type::STRING}));

        if (special_type == "JIP") {
            game_value dependency = kh_param(sp, 1, game_value(true), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY, game_data_type::CODE, game_data_type::OBJECT, game_data_type::TEAM_MEMBER, game_data_type::GROUP});
            game_value unit_required = kh_param(sp, 2, game_value(false), {game_data_type::BOOL});
            std::string jip_id = static_cast<std::string>(kh_param(sp, 3, game_value(std::string()), {game_data_type::STRING}));

            if (jip_id.empty()) {
                jip_id = special_id_override.empty() ? UIDGenerator::generate() : special_id_override;
            }

            raw_call_sqf_args_native(g_compiled_kh_set_variable_generic, kh_make_array({game_value(sqf::mission_namespace()), game_value(jip_id), game_value(true), game_value(2)}));

            return kh_trigger_cba_event_native(
                game_value("KH_eve_execution"),
                kh_make_array({arguments, function, client_owner, unscheduled}),
                target,
                kh_make_array({dependency, unit_required, game_value(jip_id)})
            );
        }

        if (special_type == "CALLBACK") {
            game_value callback_arguments = kh_param(sp, 1, game_value(auto_array<game_value>()), {});
            game_value callback_function = kh_param(sp, 2, game_value(g_compiled_kh_empty_code), {game_data_type::STRING, game_data_type::CODE});
            std::string callback_id = UIDGenerator::generate();
            auto_array<game_value> add_params;
            add_params.push_back(game_value("CBA"));
            add_params.push_back(game_value(callback_id));
            add_params.push_back(kh_make_array({arguments, function, unscheduled}));
            add_params.push_back(game_value(g_compiled_kh_callback_handler));
            raw_call_sqf_args_native(g_compiled_sqf_add_game_event_handler, game_value(std::move(add_params)));

            return kh_trigger_cba_event_native(
                game_value("KH_eve_registerCallback"),
                kh_make_array({callback_arguments, serialize_function_impl(callback_function, false), client_owner, unscheduled, game_value(callback_id)}),
                target,
                game_value(false)
            );
        }

        if (special_type == "PERSISTENT") {
            game_value entity = kh_param(sp, 1, game_value(sqf::obj_null()), {game_data_type::OBJECT, game_data_type::GROUP});
            game_value sendoff_arguments = sp.size() > 2 ? sp[2] : game_value();
            game_value sendoff_function = kh_param(sp, 3, game_value(g_compiled_kh_empty_code), {game_data_type::STRING, game_data_type::CODE});
            std::string persistent_execution_id = static_cast<std::string>(kh_param(sp, 4, game_value(std::string()), {game_data_type::STRING}));

            if (persistent_execution_id.empty()) {
                persistent_execution_id = special_id_override.empty() ? UIDGenerator::generate() : special_id_override;
            }

            if (entity.type_enum() == game_data_type::OBJECT) {
                sqf::set_variable(static_cast<object>(entity), persistent_execution_id, game_value(true), true);
            } else if (entity.type_enum() == game_data_type::GROUP) {
                sqf::set_variable(static_cast<group>(entity), persistent_execution_id, game_value(true), true);
            }

            std::string initial_id = UIDGenerator::generate();

            kh_trigger_cba_event_native(
                game_value("KH_eve_execution"),
                kh_make_array({kh_make_array({entity, game_value(initial_id)}), game_value(g_compiled_kh_persistent_marker), client_owner, unscheduled}),
                target,
                game_value(false)
            );

            kh_trigger_cba_event_native(
                game_value("KH_eve_execution"),
                kh_make_array({arguments, function, client_owner, unscheduled}),
                target,
                game_value(false)
            );

            kh_trigger_cba_event_native(
                game_value("KH_eve_persistentExecutionSetup"),
                kh_make_array({arguments, function, entity, sendoff_arguments, serialize_function_impl(sendoff_function, false), client_owner, unscheduled, game_value(persistent_execution_id), game_value(initial_id)}),
                game_value("SERVER"),
                game_value(false)
            );

            return kh_make_array({entity, game_value(persistent_execution_id), game_value(true)});
        }

        if (special_type == "PLAYER_PRESENCE") {
            game_value presence_object = kh_param(sp, 1, game_value(sqf::obj_null()), {game_data_type::OBJECT});
            game_value present = kh_param(sp, 2, game_value(true), {game_data_type::BOOL});
            game_value distance = kh_param(sp, 3, game_value(0.0f), {game_data_type::SCALAR});
            game_value jip = kh_param(sp, 4, game_value(true), {game_data_type::BOOL});
            std::string near_id = static_cast<std::string>(kh_param(sp, 5, game_value(std::string()), {game_data_type::STRING}));

            if (near_id.empty()) {
                near_id = special_id_override.empty() ? UIDGenerator::generate() : special_id_override;
            }

            raw_call_sqf_args_native(g_compiled_kh_set_variable_generic, kh_make_array({game_value(sqf::mission_namespace()), game_value(near_id), game_value(true), game_value(2.0f)}));

            kh_trigger_cba_event_native(
                game_value("KH_eve_execution"),
                kh_make_array({arguments, function, client_owner, unscheduled}),
                target,
                game_value(false)
            );

            // +KH_var_allPlayerControlledUnits
            auto_array<game_value> units_copy;
            game_value units_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayercontrolledunits");

            if (!units_gv.is_nil() && units_gv.type_enum() == game_data_type::ARRAY) {
                auto& units = units_gv.to_array();
                units_copy.reserve(units.size());

                for (size_t i = 0; i < units.size(); ++i) {
                    units_copy.push_back(units[i]);
                }
            }

            kh_trigger_cba_event_native(
                game_value("KH_eve_playerPresenceExecutionSetup"),
                kh_make_array({arguments, function, client_owner, unscheduled, presence_object, present, distance, game_value(near_id), game_value(std::move(units_copy)), jip}),
                game_value("SERVER"),
                game_value(false)
            );

            return kh_make_array({game_value(sqf::mission_namespace()), game_value(near_id), game_value(2)});
        }

        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("An unknown error occurred in processExecution");
        return game_value();
    }
}

// Immediate dispatch of the subfunction: basic -> callSerializedFunction
// path, otherwise processExecution
static game_value kh_call_subfunction(const game_value& fed_arguments, bool basic) {
    if (!basic) {
        return process_execution_sqf(fed_arguments);
    }

    auto& fed = fed_arguments.to_array();
    game_value arguments = fed.size() > 0 ? fed[0] : game_value();
    auto_array<game_value> call_params;

    if (fed.size() > 1) {
        call_params.reserve(fed.size() - 1);

        for (size_t i = 1; i < fed.size(); ++i) {
            call_params.push_back(fed[i]);
        }
    }

    return call_serialized_function_sqf(arguments, game_value(std::move(call_params)));
}

// Native KH_fnc_execute
static game_value kh_execute_impl(game_value_parameter execute_params) {
    try {
        if (execute_params.type_enum() != game_data_type::ARRAY) return game_value();
        auto& p = execute_params.to_array();
        rv_namespace ns = sqf::mission_namespace();
        game_value arguments = p.size() > 0 ? p[0] : game_value();
        game_value function = kh_param(p, 1, game_value(g_compiled_kh_empty_code), {game_data_type::STRING, game_data_type::CODE});
        game_value target = kh_param(p, 2, game_value(true), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY, game_data_type::CODE, game_data_type::OBJECT, game_data_type::TEAM_MEMBER, game_data_type::GROUP, game_data_type::SIDE, game_data_type::LOCATION});
        game_value environment = kh_param(p, 3, game_value(true), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY, game_data_type::CODE});
        game_value special = kh_param(p, 4, game_value(false), {game_data_type::BOOL, game_data_type::ARRAY});
        const bool basic = (target.type_enum() == game_data_type::BOOL && static_cast<bool>(target)) && (special.type_enum() == game_data_type::BOOL && !static_cast<bool>(special));

        if (special.type_enum() == game_data_type::BOOL && static_cast<bool>(special)) {
            special = kh_make_array({game_value("JIP"), game_value(true), game_value(false), game_value(std::string())});
        }

        game_value serialized_function = serialize_function_impl(function, false);
        game_value client_owner = game_value(static_cast<float>(sqf::client_owner()));
        game_value subfunction_code = basic ? game_value(g_compiled_kh_subfunction_basic) : game_value(g_compiled_kh_subfunction_process);

        // BOOL environment: immediate local dispatch, _environment doubles as
        // _unscheduled
        if (environment.type_enum() == game_data_type::BOOL) {
            game_value fed = basic
                ? kh_make_array({arguments, serialized_function, client_owner, environment})
                : kh_make_array({arguments, serialized_function, target, special, game_value(std::string()), environment});
            return kh_call_subfunction(fed, basic);
        }

        if (arguments.is_nil()) {
            arguments = game_value(auto_array<game_value>());
        }

        if (environment.type_enum() != game_data_type::ARRAY) {
            environment = kh_make_array({environment});
        }

        auto& env = environment.to_array();
        game_value environment_type = kh_param(env, 0, game_value(std::string("0")), {game_data_type::SCALAR, game_data_type::STRING, game_data_type::CODE});
        std::string environment_id = UIDGenerator::generate();
        const float cba_time = sqf::get_variable(sqf::mission_namespace(), "cba_missiontime");

        // ============================== SCALAR
        // ==============================
        if (environment_type.type_enum() == game_data_type::SCALAR) {
            const float environment_type_number = static_cast<float>(environment_type);
            const bool immediate = static_cast<bool>(kh_param(env, 1, game_value(true), {game_data_type::BOOL}));
            game_value timeout_rules = kh_param(env, 2, game_value(false), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY});
            game_value timeout_function = kh_param(env, 3, game_value(g_compiled_kh_empty_code), {game_data_type::CODE});
            const bool verbose_delta = static_cast<bool>(kh_param(env, 4, game_value(false), {game_data_type::BOOL}));
            game_value unscheduled = kh_param(env, 5, game_value(true), {game_data_type::BOOL});
            std::string handler_tick_counter_id = UIDGenerator::generate();
            bool iteration_count = false;

            switch (timeout_rules.type_enum()) {
                case game_data_type::BOOL:
                    timeout_rules = static_cast<bool>(timeout_rules)
                        ? kh_make_array({kh_make_array({game_value(1.0f)}), game_value(false), game_value(false)})
                        : kh_make_array({game_value(0.0f), game_value(false), game_value(false)});
                    break;
                case game_data_type::SCALAR:
                case game_data_type::STRING:
                    timeout_rules = kh_make_array({timeout_rules, game_value(false), game_value(false)});
                    break;
                default:
                    break;
            }

            auto& tr = timeout_rules.to_array();
            game_value timeout = kh_param(tr, 0, game_value(0.0f), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY});
            const bool timeout_priority = static_cast<bool>(kh_param(tr, 1, game_value(false), {game_data_type::BOOL}));
            const bool timeout_on_deletion = static_cast<bool>(kh_param(tr, 2, game_value(false), {game_data_type::BOOL}));
            float timeout_number = 0.0f;

            switch (timeout.type_enum()) {
                case game_data_type::BOOL:
                    if (static_cast<bool>(timeout)) {
                        timeout_number = 1.0f;
                        iteration_count = true;
                        handler_tick_counter_id = UIDGenerator::generate();
                        sqf::set_variable(ns, handler_tick_counter_id, game_value(1.0f));
                    }

                    break;
                case game_data_type::STRING:
                    timeout_number = std::max(parse_number(static_cast<std::string>(timeout)) - cba_time, 0.0f);
                    break;
                case game_data_type::ARRAY: {
                    auto& ta = timeout.to_array();
                    timeout_number = std::max(static_cast<float>(kh_param(ta, 0, game_value(1.0f), {game_data_type::SCALAR})), 1.0f);
                    iteration_count = true;
                    handler_tick_counter_id = UIDGenerator::generate();
                    sqf::set_variable(ns, handler_tick_counter_id, game_value(1.0f));
                    break;
                }
                case game_data_type::SCALAR:
                    timeout_number = static_cast<float>(timeout);
                    break;
                default:
                    break;
            }

            KHSpecialParseResult parsed_special = kh_parse_special_execution(special, target);

            game_value fed = basic
                ? kh_make_array({arguments, serialized_function, client_owner, unscheduled})
                : kh_make_array({arguments, serialized_function, target, special, game_value(parsed_special.special_id_override), unscheduled});

            kh_monitor_set(environment_id, kh_make_array({
                kh_make_array({arguments, timeout_function, environment_type, game_value(environment_id), parsed_special.return_value}),
                game_value(g_compiled_kh_monitor_wrapper_scalar),
                game_value(handler_tick_counter_id),
                game_value(timeout_number),
                game_value(timeout_on_deletion)
            }));

            game_value handler_id = kh_make_array({
                kh_make_array({kh_make_array({game_value("TEMPORAL")}), environment_type, game_value(environment_id), client_owner}),
                parsed_special.return_value
            });

            game_value previous_return;
            bool continue_execution = true;

            if (immediate) {
                previous_return = kh_immediate_call(fed, subfunction_code, handler_id, cba_time);

                if (iteration_count) {
                    kh_trigger_stack_handler(environment_id, false, false, false);

                    if (timeout_number == 1.0f) {
                        continue_execution = false;
                    }
                }
            }

            if (!continue_execution) {
                return handler_id;
            }

            kh_push_temporal_addition(kh_make_array({
                kh_make_array({fed, subfunction_code, game_value(environment_id)}),
                iteration_count ? game_value(g_compiled_kh_handler_scalar_iteration) : game_value(g_compiled_kh_handler_scalar),
                environment_type,
                kh_due_time(environment_type_number),
                verbose_delta ? get_epoch_sqf() : game_value(-1.0f),
                handler_id,
                game_value(environment_id),
                previous_return,
                game_value(cba_time),
                game_value(immediate ? 1.0f : 0.0f)
            }), false);

            if (!iteration_count && timeout_number != 0.0f) {
                std::string timeout_id = UIDGenerator::generate();

                kh_push_temporal_addition(kh_make_array({
                    kh_make_array({game_value(environment_id), game_value(timeout_id)}),
                    game_value(g_compiled_kh_handler_timeout),
                    game_value(timeout_number),
                    kh_due_time(timeout_number),
                    game_value(-1.0f),
                    game_value(timeout_id),
                    game_value(timeout_id),
                    game_value(),
                    game_value(cba_time),
                    game_value(0.0f)
                }), timeout_priority);
            }

            return handler_id;
        }

        // =============================== CODE
        // ===============================
        if (environment_type.type_enum() == game_data_type::CODE) {
            const bool immediate = static_cast<bool>(kh_param(env, 1, game_value(true), {game_data_type::BOOL}));
            const float interval = static_cast<float>(kh_param(env, 2, game_value(0.0f), {game_data_type::SCALAR}));
            game_value timeout_rules = kh_param(env, 3, kh_make_array({game_value(true), game_value(false), game_value(false), game_value(false)}), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY});
            game_value timeout_function = kh_param(env, 4, game_value(g_compiled_kh_empty_code), {game_data_type::CODE});
            const bool verbose_delta = static_cast<bool>(kh_param(env, 5, game_value(false), {game_data_type::BOOL}));
            game_value unscheduled = kh_param(env, 6, game_value(true), {game_data_type::BOOL});
            std::string handler_tick_counter_id = UIDGenerator::generate();
            bool iteration_count = false;
            float iteration_timeout = 0.0f;
            bool count_condition_failure = false;

            switch (timeout_rules.type_enum()) {
                case game_data_type::BOOL:
                    timeout_rules = static_cast<bool>(timeout_rules)
                        ? kh_make_array({game_value(true), game_value(false), game_value(false), game_value(false)})
                        : kh_make_array({game_value(0.0f), game_value(false), game_value(false), game_value(false)});
                    break;
                case game_data_type::SCALAR:
                case game_data_type::STRING:
                    timeout_rules = kh_make_array({timeout_rules, game_value(false), game_value(false), game_value(false)});
                    break;
                default:
                    break;
            }

            auto& tr = timeout_rules.to_array();
            game_value timeout = kh_param(tr, 0, game_value(0.0f), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY});
            const bool timeout_priority = static_cast<bool>(kh_param(tr, 1, game_value(false), {game_data_type::BOOL}));
            const bool timeout_on_condition_failure = static_cast<bool>(kh_param(tr, 2, game_value(false), {game_data_type::BOOL}));
            const bool timeout_on_deletion = static_cast<bool>(kh_param(tr, 3, game_value(false), {game_data_type::BOOL}));
            float timeout_number = 0.0f;

            switch (timeout.type_enum()) {
                case game_data_type::BOOL:
                    if (static_cast<bool>(timeout)) {
                        timeout_number = 1.0f;
                        iteration_count = true;
                        count_condition_failure = false;
                        handler_tick_counter_id = UIDGenerator::generate();
                        sqf::set_variable(ns, handler_tick_counter_id, game_value(1.0f));
                    }

                    break;
                case game_data_type::STRING:
                    timeout_number = std::max(parse_number(static_cast<std::string>(timeout)) - cba_time, 0.0f);
                    break;
                case game_data_type::ARRAY: {
                    auto& ta = timeout.to_array();
                    count_condition_failure = static_cast<bool>(kh_param(ta, 1, game_value(false), {game_data_type::BOOL}));
                    iteration_timeout = static_cast<float>(kh_param(ta, 2, game_value(0.0f), {game_data_type::SCALAR}));
                    timeout_number = std::max(static_cast<float>(kh_param(ta, 0, game_value(1.0f), {game_data_type::SCALAR})), 1.0f);
                    iteration_count = true;
                    handler_tick_counter_id = UIDGenerator::generate();
                    sqf::set_variable(ns, handler_tick_counter_id, game_value(1.0f));
                    break;
                }
                case game_data_type::SCALAR:
                    timeout_number = static_cast<float>(timeout);
                    break;
                default:
                    break;
            }

            KHSpecialParseResult parsed_special = kh_parse_special_execution(special, target);
            game_value condition = sqf::get_variable(ns, static_cast<std::string>(serialize_function_impl(environment_type, false)));

            game_value fed = basic
                ? kh_make_array({arguments, serialized_function, client_owner, unscheduled})
                : kh_make_array({arguments, serialized_function, target, special, game_value(parsed_special.special_id_override), unscheduled});

            kh_monitor_set(environment_id, kh_make_array({
                kh_make_array({arguments, timeout_function, game_value(environment_id), game_value(interval), parsed_special.return_value}),
                game_value(g_compiled_kh_monitor_wrapper_code),
                game_value(handler_tick_counter_id),
                game_value(timeout_number),
                game_value(timeout_on_deletion)
            }));

            game_value handler_id = kh_make_array({
                kh_make_array({kh_make_array({game_value("TEMPORAL")}), game_value(interval), game_value(environment_id), client_owner}),
                parsed_special.return_value
            });

            game_value previous_return;
            bool continue_execution = true;

            if (immediate) {
                game_value condition_value = kh_immediate_call(arguments, condition, handler_id, cba_time);
                const bool condition_result = condition_value.type_enum() == game_data_type::BOOL && static_cast<bool>(condition_value);

                if (iteration_count) {
                    if (count_condition_failure) {
                        if (condition_result) {
                            previous_return = kh_immediate_call(fed, subfunction_code, handler_id, cba_time);
                            kh_trigger_stack_handler(environment_id, false, false, false);
                        } else {
                            if (timeout_on_condition_failure) {
                                kh_trigger_stack_handler(environment_id, true, true, true);
                            } else {
                                kh_trigger_stack_handler(environment_id, false, false, true);
                            }
                        }

                        if (timeout_number == 1.0f) {
                            continue_execution = false;
                        }
                    } else {
                        if (condition_result) {
                            previous_return = kh_immediate_call(fed, subfunction_code, handler_id, cba_time);
                            kh_trigger_stack_handler(environment_id, false, false, false);

                            if (timeout_number == 1.0f) {
                                continue_execution = false;
                            }
                        } else if (timeout_on_condition_failure) {
                            kh_trigger_stack_handler(environment_id, true, true, true);
                        }
                    }
                } else {
                    if (condition_result) {
                        previous_return = kh_immediate_call(fed, subfunction_code, handler_id, cba_time);
                    } else if (timeout_on_condition_failure) {
                        kh_trigger_stack_handler(environment_id, true, true, true);
                    }
                }
            }

            if (!continue_execution) {
                return handler_id;
            }

            game_value handler_code;

            if (iteration_count) {
                if (timeout_on_condition_failure) {
                    handler_code = game_value(g_compiled_kh_handler_code_iteration_hard_fail);
                } else {
                    handler_code = count_condition_failure ? game_value(g_compiled_kh_handler_code_iteration_soft_fail) : game_value(g_compiled_kh_handler_code_iteration);
                }
            } else {
                handler_code = timeout_on_condition_failure ? game_value(g_compiled_kh_handler_code_hard_fail) : game_value(g_compiled_kh_handler_code);
            }

            kh_push_temporal_addition(kh_make_array({
                kh_make_array({arguments, fed, subfunction_code, game_value(environment_id), condition}),
                handler_code,
                game_value(interval),
                kh_due_time(interval),
                verbose_delta ? get_epoch_sqf() : game_value(-1.0f),
                handler_id,
                game_value(environment_id),
                previous_return,
                game_value(cba_time),
                game_value(immediate ? 1.0f : 0.0f)
                        }), false);

            if ((!iteration_count && timeout_number != 0.0f) || iteration_timeout != 0.0f) {
                std::string timeout_id = UIDGenerator::generate();
                const float true_timeout = (iteration_timeout != 0.0f) ? iteration_timeout : timeout_number;

                kh_push_temporal_addition(kh_make_array({
                    kh_make_array({game_value(environment_id), game_value(timeout_id)}),
                    game_value(g_compiled_kh_handler_timeout),
                    game_value(true_timeout),
                    kh_due_time(true_timeout),
                    game_value(-1.0f),
                    game_value(timeout_id),
                    game_value(timeout_id),
                    game_value(),
                    game_value(cba_time),
                    game_value(0.0f)
                }), timeout_priority);
            }

            return handler_id;
        }

        // ============================== STRING
        // ==============================
        if (environment_type.type_enum() == game_data_type::STRING) {
            game_value unscheduled = kh_param(env, 1, game_value(true), {game_data_type::BOOL});
            KHSpecialParseResult parsed_special = kh_parse_special_execution(special, target);
            const float environment_type_number = parse_number(static_cast<std::string>(environment_type));

            game_value fed = basic
                ? kh_make_array({arguments, serialized_function, client_owner, unscheduled})
                : kh_make_array({arguments, serialized_function, target, special, game_value(parsed_special.special_id_override), unscheduled});

            game_value handler_id = kh_make_array({
                kh_make_array({kh_make_array({game_value("TEMPORAL")}), game_value(environment_type_number), game_value(environment_id), client_owner}),
                parsed_special.return_value
            });

            if (environment_type_number == 0.0f) {
                kh_call_subfunction(fed, basic);
                return handler_id;
            }

            kh_monitor_set(environment_id, kh_make_array({
                kh_make_array({game_value(auto_array<game_value>()), game_value(g_compiled_kh_empty_code), game_value(environment_type_number), game_value(environment_id), parsed_special.return_value}),
                game_value(g_compiled_kh_empty_code),
                game_value(environment_id),
                game_value(0.0f),
                game_value(false)
            }));

            kh_push_temporal_addition(kh_make_array({
                kh_make_array({fed, subfunction_code, game_value(environment_id)}),
                game_value(g_compiled_kh_handler_string),
                game_value(environment_type_number),
                kh_due_time(environment_type_number),
                game_value(-1.0f),
                handler_id,
                game_value(environment_id),
                game_value(),
                game_value(cba_time),
                game_value(0.0f)
            }), false);

            return handler_id;
        }

        // ============================== DEFAULT
        // ==============================
        return game_value();
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("An unknown error occurred in execute");
        return game_value();
    }
}

static game_value curve_conversion_sqf(game_value_parameter type, game_value_parameter params) {
    if (type.type_enum() != game_data_type::STRING) return game_value(0.0f);
    auto& a = params.to_array();
    if (a.size() < 5) return game_value(0.0f);
    const float min_from = static_cast<float>(a[0]);
    const float max_from = static_cast<float>(a[1]);
    const float value = static_cast<float>(a[2]);
    const float min_to = static_cast<float>(a[3]);
    const float max_to = static_cast<float>(a[4]);
    std::string curve = static_cast<std::string>(type);
    std::transform(curve.begin(), curve.end(), curve.begin(), ::tolower);
    const bool is_bezier = (curve == "bezier");
    std::vector<float> interior;
    size_t clip_index;

    if (is_bezier) {
        interior = read_bezier_interior(a.size() > 5 ? a[5] : game_value());
        clip_index = 6;
    } else {
        clip_index = 5;
    }

    const bool clip = a.size() > clip_index && a[clip_index].type_enum() == game_data_type::BOOL && static_cast<bool>(a[clip_index]);
    const float span = max_from - min_from;
    float t = (span == 0.0f) ? 0.0f : (value - min_from) / span;
    if (clip) { if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f; }
    const float shaped = curve_shape(curve, is_bezier, t, interior);
    return game_value(min_to + (max_to - min_to) * shaped);
}

static game_value inverse_curve_conversion_sqf(game_value_parameter type, game_value_parameter params) {
    auto_array<game_value> result;
    auto fail = [&]() { result.push_back(game_value(0.0f)); result.push_back(game_value(0.0f)); return game_value(std::move(result)); };
    if (type.type_enum() != game_data_type::STRING) return fail();
    auto& a = params.to_array();
    if (a.size() < 5) return fail();
    const float min_from = static_cast<float>(a[0]);
    const float max_from = static_cast<float>(a[1]);
    const float out = static_cast<float>(a[2]);
    const float min_to = static_cast<float>(a[3]);
    const float max_to = static_cast<float>(a[4]);
    std::string curve = static_cast<std::string>(type);
    std::transform(curve.begin(), curve.end(), curve.begin(), ::tolower);
    const bool is_bezier = (curve == "bezier");
    std::vector<float> interior;
    if (is_bezier) interior = read_bezier_interior(a.size() > 5 ? a[5] : game_value());
    const float to_span = max_to - min_to;
    float shaped = (to_span == 0.0f) ? 0.0f : (out - min_to) / to_span;
    if (shaped < 0.0f) shaped = 0.0f; else if (shaped > 1.0f) shaped = 1.0f;
    const float t = curve_inverse_shape(curve, is_bezier, shaped, interior);
    const float value = min_from + t * (max_from - min_from);
    result.push_back(game_value(value));
    result.push_back(game_value(t));
    return game_value(std::move(result));
}

static game_value vector_curve_conversion_sqf(game_value_parameter type, game_value_parameter params) {
    if (type.type_enum() != game_data_type::STRING) return game_value(auto_array<game_value>());
    auto& a = params.to_array();
    if (a.size() < 5) return game_value(auto_array<game_value>());

    if (a[3].type_enum() != game_data_type::ARRAY || a[4].type_enum() != game_data_type::ARRAY) {
        return game_value(auto_array<game_value>());
    }

    const float min_from = static_cast<float>(a[0]);
    const float max_from = static_cast<float>(a[1]);
    const float value = static_cast<float>(a[2]);
    auto& from_vec = a[3].to_array();
    auto& to_vec = a[4].to_array();
    const size_t dim = from_vec.size() < to_vec.size() ? from_vec.size() : to_vec.size();   // 2 or 3 (min of both)
    std::string curve = static_cast<std::string>(type);
    std::transform(curve.begin(), curve.end(), curve.begin(), ::tolower);
    const bool is_bezier = (curve == "bezier");
    std::vector<float> interior;
    size_t clip_index;

    if (is_bezier) {
        interior = read_bezier_interior(a.size() > 5 ? a[5] : game_value());
        clip_index = 6;
    } else {
        clip_index = 5;
    }

    const bool clip = a.size() > clip_index && a[clip_index].type_enum() == game_data_type::BOOL && static_cast<bool>(a[clip_index]);
    const float span = max_from - min_from;
    float t = (span == 0.0f) ? 0.0f : (value - min_from) / span;
    if (clip) { if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f; }
    const float shaped = curve_shape(curve, is_bezier, t, interior);
    auto_array<game_value> result;

    for (size_t i = 0; i < dim; ++i) {
        const float f = static_cast<float>(from_vec[i]);
        const float to = static_cast<float>(to_vec[i]);
        result.push_back(game_value(f + (to - f) * shaped));
    }

    return game_value(std::move(result));
}

static game_value inverse_vector_curve_conversion_sqf(game_value_parameter type, game_value_parameter params) {
    auto_array<game_value> fail;
    fail.push_back(game_value(0.0f));
    fail.push_back(game_value(0.0f));
    if (type.type_enum() != game_data_type::STRING) return game_value(std::move(fail));
    auto& a = params.to_array();
    if (a.size() < 5) return game_value(std::move(fail));

    if (a[2].type_enum() != game_data_type::ARRAY || a[3].type_enum() != game_data_type::ARRAY || a[4].type_enum() != game_data_type::ARRAY) {
        return game_value(std::move(fail));
    }

    const float min_from = static_cast<float>(a[0]);
    const float max_from = static_cast<float>(a[1]);
    auto& out_vec = a[2].to_array();   // desired output vector (old curve's current result)
    auto& from_vec = a[3].to_array();
    auto& to_vec = a[4].to_array();
    size_t dim = from_vec.size();
    if (to_vec.size() < dim) dim = to_vec.size();
    if (out_vec.size() < dim) dim = out_vec.size();
    std::string curve = static_cast<std::string>(type);
    std::transform(curve.begin(), curve.end(), curve.begin(), ::tolower);
    const bool is_bezier = (curve == "bezier");
    std::vector<float> interior;
    if (is_bezier) interior = read_bezier_interior(a.size() > 5 ? a[5] : game_value());

    // recover shaped from the component with the largest span (most stable);
    // shared t means any non-degenerate axis works
    float best_span_abs = 0.0f;
    float shaped = 0.0f;

    for (size_t i = 0; i < dim; ++i) {
        const float f = static_cast<float>(from_vec[i]);
        const float to = static_cast<float>(to_vec[i]);
        const float comp_span = to - f;
        const float comp_span_abs = comp_span < 0.0f ? -comp_span : comp_span;

        if (comp_span_abs > best_span_abs) {
            best_span_abs = comp_span_abs;
            shaped = (static_cast<float>(out_vec[i]) - f) / comp_span;
        }
    }

    // if all components are degenerate (from == to), shaped stays 0 -> t = 0
    if (shaped < 0.0f) shaped = 0.0f; else if (shaped > 1.0f) shaped = 1.0f;
    const float t = curve_inverse_shape(curve, is_bezier, shaped, interior);
    const float value = min_from + t * (max_from - min_from);
    auto_array<game_value> result;
    result.push_back(game_value(value));
    result.push_back(game_value(t));
    return game_value(std::move(result));
}

static game_value curve_slope_sqf(game_value_parameter type, game_value_parameter params) {
    if (type.type_enum() != game_data_type::STRING) return game_value(0.0f);
    auto& a = params.to_array();
    if (a.size() < 5) return game_value(0.0f);
    const float min_from = static_cast<float>(a[0]);
    const float max_from = static_cast<float>(a[1]);
    const float value    = static_cast<float>(a[2]);
    std::string curve = static_cast<std::string>(type);
    std::transform(curve.begin(), curve.end(), curve.begin(), ::tolower);
    const bool is_bezier = (curve == "bezier");
    std::vector<float> interior;
    size_t floor_index;
    if (is_bezier) { interior = read_bezier_interior(a.size() > 5 ? a[5] : game_value()); floor_index = 6; }
    else { floor_index = 5; }
    const float floor_val = (a.size() > floor_index && a[floor_index].type_enum() == game_data_type::SCALAR) ? static_cast<float>(a[floor_index]) : 0.0f;
    const float span = max_from - min_from;
    float t = (span == 0.0f) ? 0.0f : (value - min_from) / span;
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    float slope = curve_slope(curve, is_bezier, t, interior);
    if (slope < floor_val) slope = floor_val;
    return game_value(slope);
}

static game_value vector_curve_slope_sqf(game_value_parameter type, game_value_parameter params) {
    if (type.type_enum() != game_data_type::STRING) return game_value(auto_array<game_value>());
    auto& a = params.to_array();
    if (a.size() < 5) return game_value(auto_array<game_value>());

    if (a[3].type_enum() != game_data_type::ARRAY || a[4].type_enum() != game_data_type::ARRAY) {
        return game_value(auto_array<game_value>());
    }

    const float min_from = static_cast<float>(a[0]);
    const float max_from = static_cast<float>(a[1]);
    const float value = static_cast<float>(a[2]);
    auto& from_vec = a[3].to_array();
    auto& to_vec = a[4].to_array();
    const size_t dim = from_vec.size() < to_vec.size() ? from_vec.size() : to_vec.size();
    std::string curve = static_cast<std::string>(type);
    std::transform(curve.begin(), curve.end(), curve.begin(), ::tolower);
    const bool is_bezier = (curve == "bezier");
    std::vector<float> interior;
    size_t floor_index;
    if (is_bezier) { interior = read_bezier_interior(a.size() > 5 ? a[5] : game_value()); floor_index = 6; }
    else { floor_index = 5; }
    const float floor_val = (a.size() > floor_index && a[floor_index].type_enum() == game_data_type::SCALAR) ? static_cast<float>(a[floor_index]) : 0.0f;
    const float span = max_from - min_from;
    float t = (span == 0.0f) ? 0.0f : (value - min_from) / span;
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    float slope = curve_slope(curve, is_bezier, t, interior);
    if (slope < floor_val) slope = floor_val;
    auto_array<game_value> result;

    for (size_t i = 0; i < dim; ++i) {
        const float f = static_cast<float>(from_vec[i]);
        const float to = static_cast<float>(to_vec[i]);
        result.push_back(game_value((to - f) * slope));
    }

    return game_value(std::move(result));
}

static game_value get_unit_yaw_speed_sqf(game_value_parameter obj) {
    game_data_object* gd = (game_data_object*)obj.data.get();
    if (gd == nullptr || gd->object == nullptr || gd->object->object == nullptr) return game_value(0.0f);
    void* key = (void*)gd->object->object;
    auto it = g_unit_states.find(key);
    if (it == g_unit_states.end()) return game_value(0.0f);
    const auto& st = it->second;
    if (st.count < 2) return game_value(0.0f);
    const float newest_time = st.time[st.head];
    const float newest_head = st.heading[st.head];

    // walk backwards accumulating wrap-corrected delta until we cover the
    // window (or run out of samples)
    int idx = st.head;
    float accum_delta = 0.0f;
    float prev_head = newest_head;
    float oldest_time = newest_time;

    for (int n = 1; n < st.count; ++n) {
        int prev_idx = idx - 1; if (prev_idx < 0) prev_idx += YAW_MAXSAMPLES;
        const float h = st.heading[prev_idx];
        const float t = st.time[prev_idx];
        float step = prev_head - h;
        if (step > 180.0f) step -= 360.0f;
        else if (step < -180.0f) step += 360.0f;
        accum_delta += step;
        oldest_time = t;
        prev_head = h;
        idx = prev_idx;
        if (newest_time - t >= YAW_WINDOW) break;   // covered the window
    }

    const float span = newest_time - oldest_time;
    if (span < 1e-6f) return game_value(0.0f);
    return game_value(accum_delta / span);
}

static game_value serialize_function_unary(game_value_parameter function) {
    return serialize_function_impl(function, false);
}

static game_value serialize_function_binary(game_value_parameter is_public, game_value_parameter function) {
    bool pub = (is_public.type_enum() == game_data_type::BOOL) && static_cast<bool>(is_public);
    return serialize_function_impl(function, pub);
}

static game_value execute_lua_sqf_unary(game_value_parameter code_or_function) {
    return execute_lua_sqf(game_value(), code_or_function);
}

static game_value generate_random_string_sqf_unary(game_value_parameter length) {
    auto_array<game_value> default_options;
    default_options.push_back(game_value(true));
    default_options.push_back(game_value(true));
    default_options.push_back(game_value(true));
    return generate_random_string_sqf(game_value(std::move(default_options)), length);
}

static game_value trigger_lua_event_sqf_unary(game_value_parameter right_arg) {
    return trigger_lua_event_sqf(game_value(), right_arg);
}

static game_value execute_sqf_unary(game_value_parameter code_or_function) {
    // ARRAY form is the full signature: [arguments, function, target,
    // environment, special]
    if (code_or_function.type_enum() == game_data_type::ARRAY) {
        return kh_execute_impl(code_or_function);
    }

    return execute_sqf(game_value(), code_or_function);
}

static game_value get_rotation_euler_unary(game_value_parameter right_arg) {
    return get_rotation_euler_sqf(game_value(), right_arg);
}

static game_value get_quaternion_rotation_unary(game_value_parameter right_arg) {
    return get_quaternion_rotation_sqf(game_value(), right_arg);
}

static game_value network_message_send_unary_sqf(game_value_parameter right_arg) {
    return network_message_send_sqf(game_value(), right_arg);
}

static game_value kh_set_variable_namespace(game_value_parameter left_arg, game_value_parameter right_arg) {
    return kh_set_variable_impl(left_arg, right_arg);
}

static game_value kh_set_variable_object(game_value_parameter left_arg, game_value_parameter right_arg) {
    return kh_set_variable_impl(left_arg, right_arg);
}

static game_value kh_set_variable_group(game_value_parameter left_arg, game_value_parameter right_arg) {
    return kh_set_variable_impl(left_arg, right_arg);
}

static game_value kh_set_variable_location(game_value_parameter left_arg, game_value_parameter right_arg) {
    return kh_set_variable_impl(left_arg, right_arg);
}

static game_value kh_set_variable_display(game_value_parameter left_arg, game_value_parameter right_arg) {
    return kh_set_variable_impl(left_arg, right_arg);
}

static bool kh_gv_equals(const game_value& a, const game_value& b) {
    auto ta = a.type_enum();
    if (ta != b.type_enum()) return false;

    switch (ta) {
        case game_data_type::STRING: return static_cast<std::string>(a) == static_cast<std::string>(b);
        case game_data_type::SCALAR: return static_cast<float>(a) == static_cast<float>(b);
        case game_data_type::BOOL:   return static_cast<bool>(a) == static_cast<bool>(b);
        default: return false;
    }
}

static bool kh_gv_in_array(const game_value& needle, const auto_array<game_value>& haystack) {
    for (size_t i = 0; i < haystack.size(); ++i) {
        if (kh_gv_equals(needle, haystack[i])) return true;
    }

    return false;
}

static std::unordered_set<std::string> kh_build_string_set(const auto_array<game_value>& arr) {
    std::unordered_set<std::string> set;
    set.reserve(arr.size());

    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i].type_enum() == game_data_type::STRING) {
            set.insert(static_cast<std::string>(arr[i]));
        }
    }

    return set;
}

static bool kh_set_contains(const std::unordered_set<std::string>& set, const game_value& v) {
    return v.type_enum() == game_data_type::STRING && set.find(static_cast<std::string>(v)) != set.end();
}

static void process_temporal_execution_stack() {
    rv_namespace ns = sqf::mission_namespace();

    if (!g_kh_cached_entity_initializations_deletions.is_nil() && g_kh_cached_entity_initializations_deletions.type_enum() == game_data_type::ARRAY) {
        auto& entity_deletions = g_kh_cached_entity_initializations_deletions.to_array();

        if (entity_deletions.size() > 0) {
            if (!g_kh_cached_entity_initializations.is_nil() && g_kh_cached_entity_initializations.type_enum() == game_data_type::ARRAY) {
                auto& entity_init = g_kh_cached_entity_initializations.to_array();
                size_t w = 0;
                const std::unordered_set<std::string> entity_deletion_set = kh_build_string_set(entity_deletions);

                for (size_t r = 0; r < entity_init.size(); ++r) {
                    auto& e = entity_init[r].to_array();
                    bool del = (e.size() > 4 && kh_set_contains(entity_deletion_set, e[4]));

                    if (!del) {
                        if (w != r) entity_init[w] = entity_init[r];
                        ++w;
                    }
                }

                entity_init.resize(w);
            }

            entity_deletions.resize(0);
        }
    }

    if (g_kh_cached_temporal_stack.is_nil() || g_kh_cached_temporal_stack.type_enum() != game_data_type::ARRAY) {
        return;
    }

    auto& stack = g_kh_cached_temporal_stack.to_array();

    // Temporal execution stack additions
    if (!g_kh_cached_temporal_additions.is_nil() && g_kh_cached_temporal_additions.type_enum() == game_data_type::ARRAY) {
        auto& additions = g_kh_cached_temporal_additions.to_array();

        if (additions.size() > 0) {
            for (size_t i = 0; i < additions.size(); ++i) {
                stack.push_back(additions[i]);
            }

            additions.resize(0);
        }
    }

    // Temporal execution stack deletions
    auto_array<game_value>* deletions = nullptr;

    if (!g_kh_cached_temporal_deletions.is_nil() && g_kh_cached_temporal_deletions.type_enum() == game_data_type::ARRAY) {
        deletions = &g_kh_cached_temporal_deletions.to_array();
    }

    std::unordered_set<std::string> deletion_set;

    if (deletions != nullptr && deletions->size() > 0) {
        deletion_set = kh_build_string_set(*deletions);
        size_t w = 0;

        for (size_t r = 0; r < stack.size(); ++r) {
            auto& e = stack[r].to_array();
            bool del = (e.size() > 6 && kh_set_contains(deletion_set, e[6]));

            if (!del) {
                if (w != r) stack[w] = stack[r];
                ++w;
            }
        }

        stack.resize(w);
        deletions->resize(0);
    }

    // Execute due entries
    const float tick = sqf::diag_ticktime();
    const float frame = sqf::diag_frameno();
    const size_t n = stack.size();
    static const r_string n_arguments("_thisarguments");
    static const r_string n_function("_thisfunction");
    static const r_string n_total_delta("_totaldelta");
    static const r_string n_handler_id("_handlerid");
    static const r_string n_event_name("_eventname");
    static const r_string n_previous_return("_previousreturn");
    static const r_string n_execution_time("_executiontime");
    static const r_string n_execution_count("_executioncount");
    game_value frame_delta = game_value(sqf::diag_delta_time());
    auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;

    for (size_t i = 0; i < n; ++i) {
        auto& e = stack[i].to_array();

        if (deletions != nullptr && !deletions->empty() && kh_gv_in_array(e[6], *deletions)) {
            continue;
        }

        const float delay = static_cast<float>(e[2]);
        const float delta = static_cast<float>(e[3]);
        const bool tick_based = delay > 0.0f;
        const float clock = tick_based ? tick : frame;

        if (clock >= delta) {
            const game_value old_total_delta = e[4];
            const float execution_count = static_cast<float>(e[9]);
            game_value total_delta;
            const bool is_minus_one = (old_total_delta.type_enum() == game_data_type::SCALAR && static_cast<float>(old_total_delta) == -1.0f);

            if (is_minus_one) {
                total_delta = frame_delta;
            } else {
                e[4] = get_epoch_sqf();
                total_delta = get_epoch_delta_sqf(old_total_delta);
            }

            // Inject the loop locals so the executed function inherits them
            // (call shares scope)
            game_state->set_local_variable(n_arguments, e[0]);
            game_state->set_local_variable(n_function, static_cast<code>(e[1]));
            game_state->set_local_variable(n_total_delta, total_delta);
            game_state->set_local_variable(n_handler_id, e[5]);
            game_state->set_local_variable(n_event_name, e[6]);
            game_state->set_local_variable(n_previous_return, e[7]);
            game_state->set_local_variable(n_execution_time, e[8]);
            game_state->set_local_variable(n_execution_count, e[9]);
            e[7] = raw_call_sqf_native(g_compiled_sqf_generic_call_args);
            const float step = tick_based ? delay : (delay < 0.0f ? -delay : delay);
            e[3] = game_value(delta + step);
            e[9] = game_value(execution_count + 1.0f);
        }
    }
}

static void update_unit_states() {
    auto units = sqf::get_variable(sqf::mission_namespace(), "kh_var_allmen");
    const float now = static_cast<float>(sqf::diag_ticktime());
    for (auto& kv : g_unit_states) kv.second.seen_this_frame = false;

    for (size_t i = 0; i < units.size(); ++i) {
        const auto& unit = units[i];
        game_data_object* gd = (game_data_object*)unit.data.get();
        if (gd == nullptr || gd->object == nullptr || gd->object->object == nullptr) continue;
        void* key = (void*)gd->object->object;
        float heading = static_cast<float>(sqf::get_dir_visual(unit));
        auto& st = g_unit_states[key];
        st.seen_this_frame = true;
        st.head = (st.head + 1) % YAW_MAXSAMPLES;
        st.heading[st.head] = heading;
        st.time[st.head] = now;
        if (st.count < YAW_MAXSAMPLES) st.count++;
    }

    // prune units no longer present
    for (auto it = g_unit_states.begin(); it != g_unit_states.end(); ) {
        if (!it->second.seen_this_frame) it = g_unit_states.erase(it);
        else ++it;
    }
}

// SQF entry points

// Uniform ARRAY error shape for the array-returning query commands (the same
// pair-in-array idiom as [["status","armed"]]): success returns the data
// array, failure returns [["error", <sentence>]] - one return type per
// command, message preserved. Callers key on element 0's first field.
static game_value kh_error_pairs(const std::string& msg) {
    auto_array<game_value> pair;
    pair.push_back(game_value("error"));
    pair.push_back(game_value(msg));
    auto_array<game_value> out;
    out.push_back(game_value(std::move(pair)));
    return game_value(std::move(out));
}

static game_value sample_scene_depth_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 2) return kh_error_pairs("usage: sampleSceneDepth [u, v]");
        if (arr[0].type_enum() != game_data_type::SCALAR ||
            arr[1].type_enum() != game_data_type::SCALAR) return kh_error_pairs("u and v must be numbers");
        float u = static_cast<float>(arr[0]);
        float v = static_cast<float>(arr[1]);

        // Convert uv (0..1) to pixel coords against the live depth buffer
        // dimensions, then run the single-pixel compute sample.
        float results[4] = {};

        std::string status = [&]() -> std::string {
            ID3D11Device* dev = RVExtBridge::get_d3d_device();
            ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
            if (!dev || !ctx) return "device/context null";
            UINT w = 0, h = 0;
            {
                RVExtBridge::ScopedGraphicsLock lock;
                if (!lock.acquired()) return "SKIP: graphics lock not acquired";
                std::string e = RenderIntegration::ensure_depth_srv(dev, ctx, &w, &h);
                if (!e.empty()) return "depth SRV: " + e;
            }
            float px = u * static_cast<float>(w);
            float py = v * static_cast<float>(h);
            return RenderIntegration::run_depth_compute(RenderIntegration::ComputeKernel::SampleDepth, 0, px, py, results, 4);
        }();

        if (status != "OK") return kh_error_pairs(status);
        auto_array<game_value> out;
        out.push_back(game_value(results[2]));   // scene distance, meters
        out.push_back(game_value(results[3]));   // raw depth buffer value
        return game_value(std::move(out));
    } catch (const std::exception& e) {
        report_error(std::string("sampleSceneDepth: ") + e.what());
        return kh_error_pairs(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("sampleSceneDepth: unknown exception");
        return kh_error_pairs("EXCEPTION: unknown");
    }
}

// Returns one entry per point: [status, pointDistM, sceneDistM] status: 1 =
// visible, 0 = occluded by scene geometry, -1 = offscreen/behind camera Note:
// like all depth-based tests, cannot account for particles (they do not write
// depth).
static game_value gpu_visibility_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        UINT count = static_cast<UINT>(arr.size());
        if (count == 0) return game_value(auto_array<game_value>());

        std::vector<float> pts(count * 3);
        for (UINT i = 0; i < count; ++i) {
            if (arr[i].type_enum() != game_data_type::ARRAY) return kh_error_pairs("each point must be [x, y, zASL]");
            auto& p = arr[i].to_array();
            if (p.size() < 3 ||
                p[0].type_enum() != game_data_type::SCALAR ||
                p[1].type_enum() != game_data_type::SCALAR ||
                p[2].type_enum() != game_data_type::SCALAR) return kh_error_pairs("each point must be [x, y, zASL]");
            pts[i * 3 + 0] = static_cast<float>(p[0]);
            pts[i * 3 + 1] = static_cast<float>(p[1]);
            pts[i * 3 + 2] = static_cast<float>(p[2]);
        }

        std::string status = RenderIntegration::upload_query_points(pts.data(), count);
        if (!status.empty()) return kh_error_pairs(status);

        std::vector<float> results(count * 4);
        status = RenderIntegration::run_depth_compute(RenderIntegration::ComputeKernel::Visibility, count, 0.0f, 0.0f,
                                                      results.data(), count * 4);

        if (status != "OK") return kh_error_pairs(status);
        auto_array<game_value> out;
        out.reserve(count);
        for (UINT i = 0; i < count; ++i) {
            auto_array<game_value> e;
            e.push_back(game_value(results[i * 4 + 0]));   // status
            e.push_back(game_value(results[i * 4 + 1]));   // point distance, m
            e.push_back(game_value(results[i * 4 + 2]));   // scene distance at pixel, m
            out.push_back(game_value(std::move(e)));
        }
        return game_value(std::move(out));
    } catch (const std::exception& e) {
        report_error(std::string("gpuVisibility: ") + e.what());
        return kh_error_pairs(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("gpuVisibility: unknown exception");
        return kh_error_pairs("EXCEPTION: unknown");
    }
}

// Aspect ratio is always preserved - there is no box fit, so a model is never
// squashed to a cube. addRender3D [[x,y,zASL], size, [r,g,b,a]?, mode?,
// sceneRead?, effect?, fxParams?, band?, blend?, duration?, lit?, mesh?,
// farVis?, rotation?, twoSided?] Adds a persistent mesh drawn every frame by
// the internal Draw3D EH until removed.

static game_value add_render3d_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 2) return game_value("usage: addRender3D [[x,y,zASL], size, [r,g,b,a]?, mode?, sceneRead?, effect?, fxParams?, band?, blend?, duration?, lit?, mesh?, farVis?, rotation?, twoSided?]");
        RenderIntegration::RenderObject obj;
        if (arr[0].type_enum() != game_data_type::ARRAY) return game_value("position must be [x, y, zASL]");
        auto& pos = arr[0].to_array();
        if (pos.size() < 3 ||
            pos[0].type_enum() != game_data_type::SCALAR ||
            pos[1].type_enum() != game_data_type::SCALAR ||
            pos[2].type_enum() != game_data_type::SCALAR) return game_value("position must be [x, y, zASL]");
        obj.pos[0] = static_cast<float>(pos[0]);
        obj.pos[1] = static_cast<float>(pos[1]);
        obj.pos[2] = static_cast<float>(pos[2]);

        if (!RenderIntegration::read_vec3_or_uniform(arr[1], obj.size_mul)) {
            return game_value("size must be a number or [x, y, z] multipliers of the mesh's own size");
        }

        if (arr.size() > 2 && !arr[2].is_nil()) {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value("color must be [r, g, b, a] numbers");
            auto& col = arr[2].to_array();
            if ((col.size() > 0 && col[0].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 1 && col[1].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 2 && col[2].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 3 && col[3].type_enum() != game_data_type::SCALAR)) return game_value("color must be [r, g, b, a] numbers");
            for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
            RenderIntegration::kh_sanitize_color(obj.color);
        }

        if (arr.size() > 3 && !arr[3].is_nil()) {
            if (arr[3].type_enum() != game_data_type::SCALAR) return game_value("mode must be a number (0 = test, 1 = test + write, 2 = overlay)");
            int m = static_cast<int>(static_cast<float>(arr[3]));
            if (m >= 0 && m <= 2) obj.mode = static_cast<RenderIntegration::DepthMode>(m);
        }

        if (arr.size() > 4 && !arr[4].is_nil()) {
            if (arr[4].type_enum() != game_data_type::BOOL) return game_value("sceneRead must be a boolean");
            obj.effect = static_cast<bool>(arr[4]) ? 2 : 0;   // sceneRead = tinted scene-read (colorgrade defaults)
        }

        const auto_array<game_value>* fx_params = nullptr;

        if (arr.size() > 5 &&
            !(arr[5].type_enum() == game_data_type::STRING && static_cast<std::string>(arr[5]).empty())) {
            // empty string = slot skipped (placeholder to reach later args)
            std::string khfx_path, khfx_err;
            const int e = RenderIntegration::kh_effect_from_gv(arr[5], khfx_path, khfx_err);
            if (e < 0) return game_value(khfx_err.empty() ? std::string("unknown effect") : khfx_err);
            obj.effect = e;
            obj.fx_shader = khfx_path;
        }
        if (arr.size() > 6 && !arr[6].is_nil()) {
            if (arr[6].type_enum() != game_data_type::ARRAY) return game_value("fxParams must be an array of numbers");
            fx_params = &arr[6].to_array();
        }

        if (!RenderIntegration::set_effect_params(obj, fx_params)) return game_value("fxParams entries must be numbers");

        if (arr.size() > 7 && !arr[7].is_nil()) {
            if (arr[7].type_enum() != game_data_type::ARRAY) return game_value("band must be [minDist, maxDist, falloff?] numbers");
            auto& band = arr[7].to_array();
            if ((band.size() > 0 && band[0].type_enum() != game_data_type::SCALAR) ||
                (band.size() > 1 && band[1].type_enum() != game_data_type::SCALAR) ||
                (band.size() > 2 && band[2].type_enum() != game_data_type::SCALAR)) return game_value("band must be [minDist, maxDist, falloff?] numbers");

            if (band.size() >= 2) {
                obj.banded = true;
                obj.band_min = static_cast<float>(band[0]);
                obj.band_max = static_cast<float>(band[1]);
                if (band.size() >= 3) obj.band_falloff = static_cast<float>(band[2]);
            }
        }

        if (arr.size() > 8 &&
            !(arr[8].type_enum() == game_data_type::STRING && static_cast<std::string>(arr[8]).empty())) {
            // empty string = slot skipped (the positional-placeholder
            // convention, matching the effect slot): blend stays default
            const int bm = RenderIntegration::blend_id_from_gv(arr[8]);
            if (bm < 0) return game_value("unknown blend mode");
            obj.blend_mode = bm;
        }

        if (arr.size() > 9) {
            if (!RenderIntegration::parse_duration_gv(arr[9], obj)) {
                return game_value("duration must be seconds or [fadeIn, hold, fadeOut]");
            }
        }

        if (arr.size() > 10 && !arr[10].is_nil()) {
            if (arr[10].type_enum() == game_data_type::BOOL) {
                obj.lit = static_cast<bool>(arr[10]);
            } else if (arr[10].type_enum() == game_data_type::ARRAY) {
                auto& la = arr[10].to_array();
                if ((la.size() > 0 && la[0].type_enum() != game_data_type::SCALAR) ||
                    (la.size() > 1 && la[1].type_enum() != game_data_type::SCALAR)) return game_value("lit must be a boolean or [ambient, diffuse] numbers");
                obj.lit = true;
                if (la.size() >= 1) obj.light_ambient = static_cast<float>(la[0]);
                if (la.size() >= 2) obj.light_diffuse = static_cast<float>(la[1]);

            } else {
                return game_value("lit must be a boolean or [ambient, diffuse] numbers");
            }
        }

        if (arr.size() > 11) {
            int mid = -1;

            if (arr[11].type_enum() == game_data_type::STRING &&
                RenderIntegration::kh_ends_with_ci(static_cast<std::string>(arr[11]), ".fbx")) {
                // A ".fbx" path (case-insensitive suffix REQUIRED) in the
                // mesh slot: resolved Documents-first then mod "rendering"
                // folders, imported once (synchronous - the first spawn of a
                // model pays the parse), cached thereafter.
                std::string khfb_err;
                mid = RenderIntegration::kh_fbx_mesh_id(static_cast<std::string>(arr[11]), khfb_err);
                if (mid < 0) return game_value("fbx: " + khfb_err);
            } else {
                mid = RenderIntegration::mesh_id_from_gv(arr[11]);
                if (mid < 0) return game_value("unknown mesh (builtin name, registry index, or a path ending .fbx)");
            }

            obj.mesh = mid;
        }

        // Native-size substitution AFTER the mesh is known: any size
        // component <= 0 reads the mesh's native dimension (0 = native,
        // negative = |value| x native) - the true-scale path for FBX.
        RenderIntegration::kh_apply_native_size(obj);

        if (arr.size() > 12 && !arr[12].is_nil()) {
            if (arr[12].type_enum() != game_data_type::BOOL) return game_value("farVis must be a boolean");
            obj.far_vis = static_cast<bool>(arr[12]);   // visible beyond max view distance
        }

        if (arr.size() > 13) {
            float khr_p = 0.0f, khr_y = 0.0f, khr_r = 0.0f;

            if (arr[13].type_enum() == game_data_type::SCALAR) {
                khr_y = static_cast<float>(arr[13]);   // bare number = heading (yaw)
            } else if (arr[13].type_enum() == game_data_type::ARRAY) {
                auto& ra = arr[13].to_array();
                if ((ra.size() > 0 && ra[0].type_enum() != game_data_type::SCALAR) ||
                    (ra.size() > 1 && ra[1].type_enum() != game_data_type::SCALAR) ||
                    (ra.size() > 2 && ra[2].type_enum() != game_data_type::SCALAR)) return game_value("rotation must be a number (yaw) or [pitch, yaw, roll] degrees");
                if (ra.size() >= 1) khr_p = static_cast<float>(ra[0]);
                if (ra.size() >= 2) khr_y = static_cast<float>(ra[1]);
                if (ra.size() >= 3) khr_r = static_cast<float>(ra[2]);
            } else {
                return game_value("rotation must be a number (yaw) or [pitch, yaw, roll] degrees");
            }

            RenderIntegration::kh_set_rotation(obj, khr_p, khr_y, khr_r);
        }

        if (arr.size() > 14 && !arr[14].is_nil()) {
            if (arr[14].type_enum() != game_data_type::BOOL) return game_value("twoSided must be a boolean");
            obj.two_sided = static_cast<bool>(arr[14]);   // false = back-face culling
        }

        return game_value(RenderIntegration::add_render_object(obj));
    } catch (const std::exception& e) {
        report_error(std::string("addRender3D: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("addRender3D: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// 3D mesh objects (addRender3D) and fullscreen passes (addPostFX /
// addLocalPostFX) share a handle space and several properties, but their
// non-shared properties must not overlap: each command owns exactly its kind,
// rejects the other's handles, and the genuinely common set lives in ONE
// helper so the two can never drift.

// Property set BOTH kinds own. Returns 1 = applied, 0 = recognized but the
// value was invalid, -1 = not a shared property (fall through to the caller's
// kind-specific set).
static int kh_apply_shared_prop(RenderIntegration::RenderObject& obj,
                                const std::string& prop, const game_value& val) {
    using namespace RenderIntegration;

    if (prop == "color") {
        if (val.type_enum() != game_data_type::ARRAY) return 0;
        auto& col = val.to_array();
        if ((col.size() > 0 && col[0].type_enum() != game_data_type::SCALAR) ||
            (col.size() > 1 && col[1].type_enum() != game_data_type::SCALAR) ||
            (col.size() > 2 && col[2].type_enum() != game_data_type::SCALAR) ||
            (col.size() > 3 && col[3].type_enum() != game_data_type::SCALAR)) return 0;
        for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
        RenderIntegration::kh_sanitize_color(obj.color);
        return 1;
    }

    if (prop == "visible") {
        if (val.type_enum() != game_data_type::BOOL) return 0;
        obj.visible = static_cast<bool>(val);
        return 1;
    }

    if (prop == "casterOnly") {
        if (val.type_enum() != game_data_type::BOOL) return 0;
        obj.caster_only = static_cast<bool>(val);
        return 1;
    }

    if (prop == "params") {
        if (val.type_enum() != game_data_type::ARRAY) return 0;
        return set_effect_params(obj, &val.to_array()) ? 1 : 0;
    }

    if (prop == "blend") {
        const int bm = blend_id_from_gv(val);
        if (bm < 0) return 0;
        obj.blend_mode = bm;
        return 1;
    }

    if (prop == "band") {
        if (val.type_enum() != game_data_type::ARRAY) return 0;
        auto& band = val.to_array();
        if ((band.size() > 0 && band[0].type_enum() != game_data_type::SCALAR) ||
            (band.size() > 1 && band[1].type_enum() != game_data_type::SCALAR) ||
            (band.size() > 2 && band[2].type_enum() != game_data_type::SCALAR)) return 0;

        if (band.size() < 2) {
            obj.banded = false;   // empty/short array clears the band
        } else {
            obj.banded = true;
            obj.band_min = static_cast<float>(band[0]);
            obj.band_max = static_cast<float>(band[1]);
            if (band.size() >= 3) obj.band_falloff = static_cast<float>(band[2]);
        }

        return 1;
    }

    if (prop == "duration") {
        if (!parse_duration_gv(val, obj)) return 0;
        obj.birth_time = effect_time_seconds();   // re-arm from now
        return 1;
    }

    return -1;
}

static game_value update_render3d_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 3) return game_value(false);
        if (arr[0].type_enum() != game_data_type::STRING) return game_value(false);
        if (arr[1].type_enum() != game_data_type::STRING) return game_value(false);
        const std::string handle = static_cast<std::string>(arr[0]);
        std::string prop = static_cast<std::string>(arr[1]);
        std::transform(prop.begin(), prop.end(), prop.begin(), ::tolower);
        std::lock_guard<std::mutex> g(RenderIntegration::g_draw_list_mutex);
        auto it = RenderIntegration::g_draw_list.find(handle);
        if (it == RenderIntegration::g_draw_list.end()) return game_value(false);
        auto& obj = it->second;
        if (obj.fullscreen) return game_value(false);   // that handle belongs to updatePostFX

        const int shared = kh_apply_shared_prop(obj, prop, arr[2]);
        if (shared >= 0) return game_value(shared == 1);

        if (prop == "position") {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value(false);
            auto& pos = arr[2].to_array();
            if (pos.size() < 3 ||
                pos[0].type_enum() != game_data_type::SCALAR ||
                pos[1].type_enum() != game_data_type::SCALAR ||
                pos[2].type_enum() != game_data_type::SCALAR) return game_value(false);
            obj.pos[0] = static_cast<float>(pos[0]);
            obj.pos[1] = static_cast<float>(pos[1]);
            obj.pos[2] = static_cast<float>(pos[2]);
        } else if (prop == "size" || prop == "scale") {
            if (!RenderIntegration::read_vec3_or_uniform(arr[2], obj.size_mul)) return game_value(false);
            RenderIntegration::kh_apply_native_size(obj);   // multiplier -> metres
        } else if (prop == "rotation") {
            float khr_p = 0.0f, khr_y = 0.0f, khr_r = 0.0f;

            if (arr[2].type_enum() == game_data_type::SCALAR) {
                khr_y = static_cast<float>(arr[2]);   // bare number = heading (yaw)
            } else if (arr[2].type_enum() == game_data_type::ARRAY) {
                auto& ra = arr[2].to_array();
                if ((ra.size() > 0 && ra[0].type_enum() != game_data_type::SCALAR) ||
                    (ra.size() > 1 && ra[1].type_enum() != game_data_type::SCALAR) ||
                    (ra.size() > 2 && ra[2].type_enum() != game_data_type::SCALAR)) return game_value(false);
                if (ra.size() >= 1) khr_p = static_cast<float>(ra[0]);
                if (ra.size() >= 2) khr_y = static_cast<float>(ra[1]);
                if (ra.size() >= 3) khr_r = static_cast<float>(ra[2]);
            } else {
                return game_value(false);
            }

            RenderIntegration::kh_set_rotation(obj, khr_p, khr_y, khr_r);
        } else if (prop == "mesh") {
            int mid = -1;

            if (arr[2].type_enum() == game_data_type::STRING &&
                RenderIntegration::kh_ends_with_ci(static_cast<std::string>(arr[2]), ".fbx")) {
                std::string khfb_err;
                mid = RenderIntegration::kh_fbx_mesh_id(static_cast<std::string>(arr[2]), khfb_err);
                if (mid < 0) { report_error("updateRender3D mesh: " + khfb_err); return game_value(false); }
            } else {
                mid = RenderIntegration::mesh_id_from_gv(arr[2]);
                if (mid < 0) return game_value(false);
            }

            obj.mesh = mid;
            RenderIntegration::kh_apply_native_size(obj);
        } else if (prop == "material") {
            // "material" update: per-submesh shader + texture + channel
            // assignments (format at kh_apply_material_update). Errors report
            // the reason and return false.
            std::string khmt_err;

            if (!RenderIntegration::kh_apply_material_update(obj, arr[2], khmt_err)) {
                report_error("updateRender3D material: " + khmt_err);
                return game_value(false);
            }
        } else if (prop == "mode") {
            if (arr[2].type_enum() != game_data_type::SCALAR) return game_value(false);
            int m = static_cast<int>(static_cast<float>(arr[2]));
            if (m < 0 || m > 2) return game_value(false);
            obj.mode = static_cast<RenderIntegration::DepthMode>(m);
        } else if (prop == "sceneread") {
            if (arr[2].type_enum() != game_data_type::BOOL) return game_value(false);
            obj.effect = static_cast<bool>(arr[2]) ? 2 : 0;
            RenderIntegration::set_effect_params(obj, nullptr);
        } else if (prop == "effect") {
            std::string khfx_path, khfx_err;
            const int e = RenderIntegration::kh_effect_from_gv(arr[2], khfx_path, khfx_err);
            if (e < 0) return game_value(false);
            obj.effect = e;
            obj.fx_shader = khfx_path;
            RenderIntegration::set_effect_params(obj, nullptr);
        } else if (prop == "lit" || prop == "lighting") {
            // BOOL toggles, ARRAY [ambient, diffuse] configures.
            if (arr[2].type_enum() == game_data_type::ARRAY) {
                auto& la = arr[2].to_array();

                if ((la.size() > 0 && la[0].type_enum() != game_data_type::SCALAR) ||
                    (la.size() > 1 && la[1].type_enum() != game_data_type::SCALAR)) return game_value(false);

                if (la.size() < 1) {
                    obj.lit = false;   // empty array disables the shading
                } else {
                    obj.lit = true;
                    obj.light_ambient = static_cast<float>(la[0]);
                    if (la.size() >= 2) obj.light_diffuse = static_cast<float>(la[1]);

                }
            } else if (arr[2].type_enum() == game_data_type::BOOL) {
                obj.lit = static_cast<bool>(arr[2]);
            } else {
                return game_value(false);
            }
        } else if (prop == "twosided") {
            if (arr[2].type_enum() != game_data_type::BOOL) return game_value(false);
            obj.two_sided = static_cast<bool>(arr[2]);   // false = back-face culling
        } else if (prop == "farvis") {
            if (arr[2].type_enum() != game_data_type::BOOL) return game_value(false);
            obj.far_vis = static_cast<bool>(arr[2]);   // visible beyond max view distance
        } else {
            return game_value(false);
        }

        return game_value(true);
    } catch (const std::exception& e) {
        report_error(std::string("updateRender3D: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("updateRender3D: unknown exception");
        return game_value(false);
    }
}

// updatePostFX [handle, property, value] -> BOOL Fullscreen post-processing
// passes ONLY (addPostFX / addLocalPostFX handles); 3D mesh objects belong to
// updateRender3D. Returns false for unknown handles, 3D-object handles,
// unknown properties, or invalid values.

// Returns false for anything else.
static bool kh_ui_phase_from_gv(const game_value& gv, bool& affect_ui, bool& ui_only) {
    if (gv.type_enum() == game_data_type::BOOL) {
        affect_ui = static_cast<bool>(gv);
        ui_only = false;
        return true;
    }

    if (gv.type_enum() != game_data_type::STRING) return false;
    std::string khup_s = static_cast<std::string>(gv);
    if (khup_s.empty()) return true;   // positional placeholder: skip
    std::transform(khup_s.begin(), khup_s.end(), khup_s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (khup_s == "SCENE") { affect_ui = false; ui_only = false; return true; }
    if (khup_s == "BOTH")  { affect_ui = true;  ui_only = false; return true; }
    if (khup_s == "UI")    { affect_ui = true;  ui_only = true;  return true; }
    return false;
}

static game_value update_post_fx_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 3) return game_value(false);
        if (arr[0].type_enum() != game_data_type::STRING) return game_value(false);
        if (arr[1].type_enum() != game_data_type::STRING) return game_value(false);
        const std::string handle = static_cast<std::string>(arr[0]);
        std::string prop = static_cast<std::string>(arr[1]);
        std::transform(prop.begin(), prop.end(), prop.begin(), ::tolower);
        std::lock_guard<std::mutex> g(RenderIntegration::g_draw_list_mutex);
        auto it = RenderIntegration::g_draw_list.find(handle);
        if (it == RenderIntegration::g_draw_list.end()) return game_value(false);
        auto& obj = it->second;
        if (!obj.fullscreen) return game_value(false);   // that handle belongs to updateRender3D
        if (obj.affect_ui) RenderIntegration::kh_ui_driver_rehoist();   // back on top

        const int shared = kh_apply_shared_prop(obj, prop, arr[2]);
        if (shared >= 0) return game_value(shared == 1);

        if (prop == "position") {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value(false);
            auto& pos = arr[2].to_array();
            if (pos.size() < 3 ||
                pos[0].type_enum() != game_data_type::SCALAR ||
                pos[1].type_enum() != game_data_type::SCALAR ||
                pos[2].type_enum() != game_data_type::SCALAR) return game_value(false);
            obj.pos[0] = static_cast<float>(pos[0]);
            obj.pos[1] = static_cast<float>(pos[1]);
            obj.pos[2] = static_cast<float>(pos[2]);
        } else if (prop == "effect") {
            std::string khfx_path, khfx_err;
            const int e = RenderIntegration::kh_effect_from_gv(arr[2], khfx_path, khfx_err);
            if (e <= 0) return game_value(false);   // a fullscreen pass without an effect is meaningless
            obj.effect = e;
            obj.fx_shader = khfx_path;
            RenderIntegration::set_effect_params(obj, nullptr);
        } else if (prop == "ui") {
            // "SCENE"/"UI"/"BOTH" or the legacy boolean pair
            if (!kh_ui_phase_from_gv(arr[2], obj.affect_ui, obj.ui_only)) return game_value(false);
            if (obj.affect_ui) RenderIntegration::kh_ui_driver_rehoist();   // Uphase demanded
        } else if (prop == "uispill") {
            // Builtin gather effects are auto-classified; this bit exists so
            // a custom.hlsl never needs coverage code.
            if (arr[2].type_enum() != game_data_type::BOOL) return game_value(false);
            obj.ui_spill = static_cast<bool>(arr[2]);
        } else if (prop == "radius") {
            if (!RenderIntegration::read_vec3_or_uniform(arr[2], obj.local_radius)) return game_value(false);
        } else if (prop == "falloff") {
            if (arr[2].type_enum() != game_data_type::SCALAR) return game_value(false);
            obj.local_falloff = static_cast<float>(arr[2]);
        } else if (prop == "localsphere") {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value(false);
            auto& sp = arr[2].to_array();

            if (sp.size() < 1) {
                obj.localized = false;
            } else {
                obj.localized = true;
                if (!RenderIntegration::read_vec3_or_uniform(sp[0], obj.local_radius)) return game_value(false);
                if (sp.size() >= 2 && sp[1].type_enum() != game_data_type::SCALAR) return game_value(false);
                if (sp.size() >= 2) obj.local_falloff = static_cast<float>(sp[1]);
            }
        } else if (prop == "shape") {
            const int sh = RenderIntegration::shape_id_from_gv(arr[2]);
            if (sh < 0) return game_value(false);
            obj.local_shape = sh;
        } else {
            return game_value(false);
        }

        return game_value(true);
    } catch (const std::exception& e) {
        report_error(std::string("updatePostFX: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("updatePostFX: unknown exception");
        return game_value(false);
    }
}

// removeRenderHandler handle -> BOOL (true if an object was removed)
// removeRenderHandler "" or "all" -> BOOL (clears the entire draw list)
static game_value remove_render_handler_sqf(game_value_parameter arg) {
    try {
        if (arg.type_enum() != game_data_type::STRING) return game_value(false);
        const std::string handle = static_cast<std::string>(arg);

        if (handle.empty() || handle == "all") {
            RenderIntegration::clear_render_objects();
            return game_value(true);
        }

        return game_value(RenderIntegration::remove_render_object(handle));
    } catch (const std::exception& e) {
        report_error(std::string("removeRenderHandler: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("removeRenderHandler: unknown exception");
        return game_value(false);
    }
}

static game_value queue_visibility_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        UINT count = static_cast<UINT>(arr.size());
        RenderIntegration::g_query_points_pending.resize(static_cast<size_t>(count) * 3);

        for (UINT i = 0; i < count; ++i) {
            if (arr[i].type_enum() != game_data_type::ARRAY) return game_value(-1.0f);   // invalid point shape
            auto& p = arr[i].to_array();
            if (p.size() < 3 ||
                p[0].type_enum() != game_data_type::SCALAR ||
                p[1].type_enum() != game_data_type::SCALAR ||
                p[2].type_enum() != game_data_type::SCALAR) return game_value(-1.0f);   // invalid point shape
            RenderIntegration::g_query_points_pending[i * 3 + 0] = static_cast<float>(p[0]);
            RenderIntegration::g_query_points_pending[i * 3 + 1] = static_cast<float>(p[1]);
            RenderIntegration::g_query_points_pending[i * 3 + 2] = static_cast<float>(p[2]);
        }

        RenderIntegration::g_query_pending = count > 0;
        if (count > 0) RenderIntegration::ensure_draw_eh();   // the flush performs the dispatch
        return game_value(static_cast<float>(count));
    } catch (const std::exception& e) {
        report_error(std::string("queueVisibility: ") + e.what());
        return game_value(-1.0f);
    } catch (...) {
        report_error("queueVisibility: unknown exception");
        return game_value(-1.0f);
    }
}

// getVisibilityResults -> [ageInFrames, [[status, pointDistM,
// sceneDistM],...]] status: 1 visible, 0 occluded, -1 offscreen/behind
// camera. Results array is empty until the first queued batch completes.
static game_value get_visibility_results_sqf() {
    try {
        auto_array<game_value> results;
        results.reserve(RenderIntegration::g_vis_result_count);

        for (UINT i = 0; i < RenderIntegration::g_vis_result_count; ++i) {
            auto_array<game_value> e;
            e.push_back(game_value(RenderIntegration::g_vis_results_cpu[i * 4 + 0]));
            e.push_back(game_value(RenderIntegration::g_vis_results_cpu[i * 4 + 1]));
            e.push_back(game_value(RenderIntegration::g_vis_results_cpu[i * 4 + 2]));
            results.push_back(game_value(std::move(e)));
        }

        const float age = static_cast<float>(
            RenderIntegration::g_flush_frame - RenderIntegration::g_vis_result_frame);

        auto_array<game_value> out;
        out.push_back(game_value(age));
        out.push_back(game_value(std::move(results)));
        return game_value(std::move(out));
    } catch (...) {
        report_error("getVisibilityResults: unknown exception");
        return game_value(auto_array<game_value>());
    }
}

// Outline, Pulse, Ssgi and Fogscatter sample the engine depth buffer per
// pixel; on frames where they are active, mode-1 meshes do not write depth
// (read-only DSV phase).
static game_value add_postfx_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 1) return game_value("usage: addPostFX [effect, params?, [r,g,b,a]?, band?, blend?, affectUI?, duration?]");
        RenderIntegration::RenderObject obj;
        obj.fullscreen = true;
        obj.mode = RenderIntegration::DepthMode::Off;
        std::string khfx_path, khfx_err;
        const int e = RenderIntegration::kh_effect_from_gv(arr[0], khfx_path, khfx_err);
        if (e <= 0) return game_value(khfx_err.empty() ? std::string("unknown or non-fullscreen effect") : khfx_err);
        obj.effect = e;
        obj.fx_shader = khfx_path;
        const auto_array<game_value>* fx_params = nullptr;

        if (arr.size() > 1 && !arr[1].is_nil()) {
            if (arr[1].type_enum() != game_data_type::ARRAY) return game_value("params must be an array of numbers");
            fx_params = &arr[1].to_array();
        }

        if (!RenderIntegration::set_effect_params(obj, fx_params)) return game_value("params entries must be numbers");

        if (arr.size() > 2 && !arr[2].is_nil()) {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value("color must be [r, g, b, a] numbers");
            auto& col = arr[2].to_array();
            if ((col.size() > 0 && col[0].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 1 && col[1].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 2 && col[2].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 3 && col[3].type_enum() != game_data_type::SCALAR)) return game_value("color must be [r, g, b, a] numbers");
            for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
            RenderIntegration::kh_sanitize_color(obj.color);
        }

        if (arr.size() > 3 && !arr[3].is_nil()) {
            if (arr[3].type_enum() != game_data_type::ARRAY) return game_value("band must be [minDist, maxDist, falloff?] numbers");
            auto& band = arr[3].to_array();
            if ((band.size() > 0 && band[0].type_enum() != game_data_type::SCALAR) ||
                (band.size() > 1 && band[1].type_enum() != game_data_type::SCALAR) ||
                (band.size() > 2 && band[2].type_enum() != game_data_type::SCALAR)) return game_value("band must be [minDist, maxDist, falloff?] numbers");

            if (band.size() >= 2) {
                obj.banded = true;
                obj.band_min = static_cast<float>(band[0]);
                obj.band_max = static_cast<float>(band[1]);
                if (band.size() >= 3) obj.band_falloff = static_cast<float>(band[2]);
            }
        }

        if (arr.size() > 4 &&
            !(arr[4].type_enum() == game_data_type::STRING && static_cast<std::string>(arr[4]).empty())) {
            // empty string = slot skipped (the positional-placeholder
            // convention, matching the effect slot): blend stays default
            const int bm = RenderIntegration::blend_id_from_gv(arr[4]);
            if (bm < 0) return game_value("unknown blend mode");
            obj.blend_mode = bm;
        }

        if (arr.size() > 5 && !arr[5].is_nil()) {
            // "SCENE"/"UI"/"BOTH" (empty = slot skipped) or the legacy
            // boolean pair false=SCENE / true=BOTH
            if (!kh_ui_phase_from_gv(arr[5], obj.affect_ui, obj.ui_only)) {
                return game_value("affectUI must be \"SCENE\", \"UI\", \"BOTH\", or a boolean");
            }
        }

        if (obj.affect_ui) RenderIntegration::kh_ui_driver_rehoist();

        if (arr.size() > 6) {
            if (!RenderIntegration::parse_duration_gv(arr[6], obj)) {
                return game_value("duration must be seconds or [fadeIn, hold, fadeOut]");
            }
        }

        return game_value(RenderIntegration::add_render_object(obj));
    } catch (const std::exception& e) {
        report_error(std::string("addPostFX: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("addPostFX: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

static game_value set_ssgi_scale_sqf(game_value_parameter arg) {
    try {
        if (arg.type_enum() != game_data_type::SCALAR) return game_value(false);
        float khss = static_cast<float>(arg);
        if (!(khss == khss)) return game_value(false);
        if (khss < 0.25f) khss = 0.25f;
        if (khss > 2.0f)  khss = 2.0f;
        RenderIntegration::g_khsg_scale = khss;
        return game_value(true);
    } catch (...) {
        return game_value(false);
    }
}

static game_value set_render_debug_sqf(game_value_parameter arg) {
    try {
        if (arg.type_enum() != game_data_type::SCALAR) return game_value(false);
        const int khd_m = static_cast<int>(static_cast<float>(arg));
        // 178 = witness farthest-plane revert (the slice returns), 179 =
        // depth-bias clamp revert (the render-through returns).
        const bool khd_ok = khd_m == 222 || khd_m == 224 || khd_m == 227 ||
                            khd_m == 228 || khd_m == 229 || khd_m == 230 ||
        // 235 = RESTORE THE D32 DEPTH-QUANTISATION FLOOR on the self-shadow
        // constant bias.
                            khd_m == 231 || khd_m == 232 || khd_m == 233 ||
                            khd_m == 234 || khd_m == 235 ||
        // 236 = KH_ENGCAM_LOCATOR, the engine's OWN camera consumed instead
        // of the one recovered from the RV projection-view matrix (full
        // ledger before the seam inject in rendering_integration.hpp).
                            khd_m == 236 ||
        // 237 = REVERT KH_ENGCAM_LOCATOR to the recovered-camera fold, i.e.
                            khd_m == 237 ||
        // 223 = REVERT the abrupt-motion stencil finalization
        // (recent-velocity ring + floor-refusal witness gate together.hpp).
        // One axis, one number.
                            khd_m == 223 ||
                            khd_m == 238 ||
        // 239 = REVERT the hero sun map (union-only, behaviour verbatim incl.
                            khd_m == 239 ||
        // 240 = REVERT the engcam absolute agreement bound (per-component
        // 4-ulp form restored; ledger in kh_engcam_agree4).
                            khd_m == 240 ||
        // 241 = ARM the camera-anchored union window at g_sun_range (default
        // was the caster-anchored fit). the camera-free hash and the 250 m
        // eligibility radius).
                            khd_m == 241 ||
        // 242 = stand the MID cascade band down (8 m @ 4 mm, t26). CPU-side:
        // the band is not rendered, its meta stays zeroed, and the self +
        // cast chains fall through to the next tier.
                            khd_m == 242 ||
        // 243 = revert the CAST TIER CHAIN to the union-only compare
        // (lighting0.y 21, written only by the fire's cast CB fill).
                            khd_m == 243 ||
        // 244 = stand the OUTER cascade band down (32 m @ 16 mm, t27). Same
        // CPU-side shape as 242; expect under 244: 8-32 m falls to the
        // union's range-priced texels.
                            khd_m == 244 ||
        // 245 = REVERT the cascade filtering (lighting0.y 22, carried by both
        // the mesh and fire CB fills): the cast compare drops back to the
        // hard single bilinear at every tier AND the band self kernels
        // re-collapse to the b single tap.
                            khd_m == 245 ||
        // 246 = REVERT the self range fade (lighting0.y 23, mesh CB): the
        // union self tier drops its edge fade and the hard uv-window cutoff
        // returns.
                            khd_m == 246 ||
        // 247 = REVERT the whole stencil mirror counting pass (KH_VOL_MIRROR;
        // CPU-side, byte-identical): no prepass, no b2 patch, no re-issue,
        // mirMeta 0.
                            khd_m == 247 ||
        // 248 = mirror mask PRODUCED, consumption OFF (lighting0.y 24, mesh
        // fill). Expect under 248: visuals identical to 247 with the
        // production lanes still live.
                            khd_m == 248 ||
        // 249 = REVERT the sun anchor (KH_SUN_ANCHOR): every sun matrix
        // world-absolute again, sunOrigin (0,0,0), shader subtraction
        // bit-exact no-op.
                            khd_m == 249 ||
        // 250 = REVERT the band stage pool to 2 (count; creation is
        // take-site-lazy either way, so the change is live mid-session).
                            khd_m == 250 ||
        // 251 = DISABLE the layout-change escape. Expect under 251:
        // bandLayoutEscapes pinned 0 and casts landing offset for up to the 4
        // Hz interval after a cascade re-author (the shadow-view-distance
        // entry).
                            khd_m == 251 ||
        // 252 = DEBUG VISUAL 20, the mirror-mask probe (dbgCtl.x 20;
        // translated at the mesh fill like 84 -> 18). MAGENTA = mask invalid
        // (production), BLUE = mask shadowed, GREEN = mask lit.
                            khd_m == 252 ||
        // 253 = REVERT the gradient slack (KH_SUN_GRAD_SLACK, lighting0.y
        // 25): every self tier drops the fwidth depth slack. Expect under
        // 253: the grazing motion z-fight returns in full.
                            khd_m == 253 ||
        // 254 = REVERT the re-issue's depth clamp (the mirror draws with the
        // engine's own DepthClipEnable).
                            khd_m == 254 ||
        // 255 = REVERT the shadow-view-distance range fade
        // (KH_SUN_RANGE_FADE, CPU sentinel: mirMeta.w stays 0).
                            khd_m == 255 ||
        // 256 = REVERT the double-precision band snap (fp32 form restored).
        // Expect under 256: the grazing motion shimmer on lit faces returns,
        // hero-worst, with the 239-visible edge line.
                            khd_m == 256 ||
        // 257 = REVERT the cascade tier blend (lighting0.y 26, BOTH fills):
        // band boundaries return to hard 1/4/16 mm steps.
                            khd_m == 257 ||
        // 259 = REVERT normal-offset sampling (lighting0.y 27, self chain;
        // the front-face-standard primary anti-acne, grazing- scaled 1-3
        // texels).
                            khd_m == 259 ||
        // 260 = REVERT footprint tap spread (lighting0.y 28, self chain;
        // fixed +-1 ring restored). Expect under 260: the camera-grazing
        // salt-and-pepper and the crevice noise return.
                            khd_m == 260 ||
        // 261 = RESTORE the historic 16/3.0 sun raster bias (the low 8/1.0
        // state is the default). Expect under 261: the separation-dependent
        // contact/crevice dapple returns.
                            khd_m == 261 ||
        // 262 = contact shortcut OFF - an accepted ALIAS OF THE DEFAULT
        // (expect: identical to mode 0).
                            khd_m == 262 ||
        // 263 = ARM the contact blocker shortcut (lighting0.y 30, opt-in
        // experimental; slope-thresholded form).
                            khd_m == 263 ||
        // 258 = REVERT the lighting-block anchor gate (the lum band returns
        // to the instantaneous standing; CPU-only,).
                            khd_m == 258 ||
        // 264 = REVERT self-tier bilinear PCF (KH_SELF_PCF, lighting0.y 31,
        // mesh fill; hard single-texel taps restored on all four self tiers).
                            khd_m == 264 ||
        // 265 = REVERT the MinHook install retry ladder (single- attempt
        // historic behavior: the first failed install round latches the
        // session off and reports immediately).
                            khd_m == 265 ||
        // 266 = ARM the tid-relax experiment (opt-in; ledger in
        // reorder_pre_draw): track target-context draws from non-clear
        // threads, own draws excluded by flags.
                            khd_m == 267 ||
        // 268 = FULL revert of the noise axis (lighting0.y 33): offset 6/8
        // restored, gradient off - mode-0 behavior verbatim. Expect under
        // 268: the close-contact noise returns, the grazing strip gone.
                            khd_m == 268 ||
        // 269 = DEBUG VISUAL 21, the self-tier probe (dbgCtl.x 21; pure
        // gauge,).
                            khd_m == 269 ||
        // 270 = REVERT the publish rate gate (instant adoption of any in-band
        // coherent step).
                            khd_m == 270 ||
        // 271 = REVERT the scoped off-thread trigger (tid-gated historic
        // behavior).
                            khd_m == 272 ||
        // 273 = REVERT the injection-instant block snapshot (the publish
        // returns to the park-time live mirror, behavior).
                            khd_m == 273 ||
        // 274 = REVERT the whole off-thread funnel (tid- gated historic
        // behavior on blend/DSS/RTS AND map/unmap/ updatesubresource).
                            khd_m == 275 ||
        // 276 = REVERT the snapshot-anchor consistency gate (any
        // injection-instant snapshot adopts regardless of cluster).
                            khd_m == 276 ||
        // 277 = ADMIT lane-15 == 2 blocks into the arbitration (opt-in
        // experiment).
                            khd_m == 277 ||
        // 278 = REVERT snapshot-outright adoption (snapshot publishes return
        // through the 50% band + 500 ms pending).
                            khd_m == 278 ||
        // 279 = REVERT continuity selection (the snapshot takes the last
        // upload before the injection again).
                            khd_m == 279 ||
        // 280 = the measured-bias revert. AUDIT - THIS ARM IS INERT and this
        // entry contradicted the 288 entry below it for four builds.
                            khd_m == 280 ||
        // 281 = REVERT the off-thread atlas consumption (the SRV-keyed sweep
        // returns to render-thread draws only). Expect under 281 with FSAA
        // off: litGate falls back to 0, receive dies again; MSAA-on
        // unchanged.
                            khd_m == 282 ||
        // 283 = RESTORE the anchor gate over ring picks (-behavior). Expect
        // under 283: blkSnapRejects climbs again and the few-percent flip
        // variant returns.
                            khd_m == 283 ||
                            // 291 = ring pick anchor-band filter OFF (plain
                            // nearest-to-previous). Expect the far-range
                            // 0.70x both-channel dimming step to return under
                            // 291 during fast orbits.
                            khd_m == 291 ||
                            // 292 = starvation hold OFF (raw nearest on
                            // no-band). Expect the range-banded 0.75x dimming
                            // at ~7000 m to return under 292.
                            khd_m == 292 ||
                            // 293 = publish slew OFF (instant commit). Expect
                            // referee snap steps (~7% one-frame) to return
                            // under 293 during cloud transitions.
                            khd_m == 293 ||
                            // 294 = caster-union window growth OFF (the pure
                            // camera window).
                            khd_m == 294 ||
                            // 295 = band lit-return restored (cast tier chain
                            // verbatim; lighting0.y 40, fire cast fill only).
                            // Expect the ~50 m lit bubble around the camera
                            // to return under 295 inside/on a giant caster's
                            // shadow.
                            khd_m == 295 ||
                            // 296 = SELF band lit-return restored (twin;
                            // lighting0.y 41, mesh fill only).
                            khd_m == 296 ||
                            // 297 = strict ordinal gate restored (liveness
                            // stand-down OFF). Expect the map-screen 20 Hz FX
                            // flicker to return under 297 (arms at the 50 ms
                            // failsafe, staleSkips ~= flushes - arms).
                            khd_m == 297 ||
                            // 298 = pre-fog arming (engine terms published
                            // only when setFog > 0, so the far ramp is
                            // fog-gated again).
                            khd_m == 298 ||
                            // 299 = zero-decay anchor hold OFF (probe).
                            khd_m == 299 ||
                            // 300 = ramp coherence guard OFF (verbatim pair,
                            // incoherent or not).
                            khd_m == 300 ||
                            // 301 = VISUAL 22, the fog convergence colour
                            // painted raw (full-fog endpoint).
                            khd_m == 301 ||
                            // 302 = scale the fog TARGET by the engine's
                            // shading floor cb5[0].w (the probe, not the
                            // fix).
                            khd_m == 302 ||
                            // 303 = FAR-DISCARD ISOLATOR (forces the shaders'
                            // existing farVis skip on every mesh).
                            khd_m == 303 ||
                            // 304 = far-keep routing back to far_vis ONLY
                            // (the pre-condition).
                            khd_m == 304 ||
                            // 305 = VISUAL 23, flat opaque magenta returned
                            // BEFORE the atmospheric block. Run it on the
                            // exact scene that shows the see-through cut.
                            khd_m == 305 ||
                            // 306 = VISUAL 24, the encoded depth ladder (ndc
                            // = m22 + m32/w). WHITE = at/past the far plane,
                            // where the fragment clamps to MaxDepth and a
                            // LESS_EQUAL cloud/sky pass ties and wins.
                            khd_m == 306 ||
                            // 307 = VISUAL 25, view depth in METRES. blue
                            // <500, green 500-1000, yellow 1000-1400, orange
                            // 1400-1531 (fog ramp end), RED 1531-1547 (engine
                            // far plane), purple 1547-2500, white >2500.
                            khd_m == 307 ||
                            // 308 = OUR atmospherics fully disarmed (fog +
                            // haze + engine terms), leaving the engine's own
                            // pass as the only authority over our pixels.
                            khd_m == 308 ||
                            // 309 = force the mesh onto the POST-SCENE FLUSH
                            // path (declared ineligible for the reorder
                            // injection). Run it on the cut scene: CUT GONE =
                            // draw order is the author, proven in one flip.
                            khd_m == 309 ||
                            khd_m == 310 ||
                            khd_m == 311 ||
                            khd_m == 312 ||
                            // 313 = VISUAL 26, ndc in the ENGINE frame pair
                            // (khFarSplit.xy) rather than our own. WHITE =
                            // at/past the far plane the rest of the scene
                            // shares.
                            khd_m == 313 ||
                            // 314 = beyond-far fragments encode in the FRAME
                            // pair clamped at the far plane instead of
                            // keeping the far-keep pair. Diagnostic only,
                            // never a default.
                            khd_m == 314 ||
                            // 315 = VISUAL 27, the depth we actually WRITE
                            // (SV_Position.z), banded tightly at the top of
                            // the shared 0.011-0.999 range.
                            khd_m == 315 ||
                            // 316 = REVERT the far tie-break (saturated
                            // beyond-far fragments go back to sitting ON the
                            // viewport maximum). Expect the see-through cut
                            // to return under 316 and only under 316.
                            khd_m == 316 ||
                            // 317 = VISUAL 28, paints MAGENTA exactly where
                            // the tie-break moved a fragment and green
                            // elsewhere - the magenta region must coincide
                            // with what used to be the cut.
                            khd_m == 317 ||
                            // 318 = REVERT the seam-basis staleness refusal -
                            // the colour pass adopts the seam's published
                            // basis unconditionally again.
                            khd_m == 318 ||
                            // 319 = REVERT the flush's fast-camera escape.
                            // The flush goes back to drawing miss frames on
                            // the held latch.
                            khd_m == 319 ||
                            // 320 = REVERT the latch trajectory confirmation
                            // - the continuity gate goes back to escaping
                            // only via the 1000 ms lost-adopt.
                            khd_m == 320 ||
                            // 321 = REVERT the fog-lane refresh. Fog terms
                            // ride the luminance arbitration again. Expect
                            // blkFogRefreshes 0 and the multi-second fog
                            // latency on weather or camera changes.
                            khd_m == 321 ||
                            // 322 = REVERT the farVis view-distance
                            // stand-down. farVis meshes take the engine's
                            // boundary fade again (height fog and haze are
                            // unaffected either way).
                            khd_m == 322 ||
                            // 323 = RESTORE below-layer stand-down.
                            khd_m == 323 ||
                            // 324 = fall back to 's LAYER CLAMP - the
                            // above-layer arm evaluated at the boundary
                            // instead of the engine's decoded camera-below
                            // arm.
                            khd_m == 324 ||
                            // 325 = REVERT the below-layer convergence
                            // COLOUR. Underwater our meshes go back to
                            // converging on PSC_FogColor (the above-layer
                            // target) instead of the engine's sky colour.
                            khd_m == 325 ||
                            // 326 = KH_BAND_SUN_REACH OFF (isotropic sphere +
                            // symmetric window, verbatim).
                            khd_m == 326 ||
                            // 327 = KH_UNION_TEXEL_SNAP OFF (the union centre
                            // back on the bare two-metre camera cell, no grid
                            // phase).
                            khd_m == 327 ||
                            // 328 = KH_LIT_AUTHORITATIVE OFF, i.e.
                            khd_m == 328 ||
                            // 329 = KH_UNION_R_LATCH OFF (the union radius
                            // back to the raw per-fit enclosing radius, so
                            // its texel is a function of camera position
                            // again).
                            khd_m == 329 ||
        // 330 = KH_SNAP_REPROJECT off - the contact clamp reads the occlusion
        // snapshot at the CURRENT pixel, decoded with the CURRENT pair, along
        // the CURRENT ray (read).
                            khd_m == 330 ||
        // 331 = KH_ARB_FIXED_PULL off - need-based pull (gap + 0.25 m) under
        // the honest read, i.e.
                            khd_m == 331 ||
        // 332 = fixed pull at a 0.10 m margin (mode 0 = 0.25 m).
                            khd_m == 332 ||
        // 333 = the 0.25 m fixed-pull margin (default; 0.10 m is the default,
        // operator-validated). Expect: the ground-line overtake of
        // objects/terrain within 0.10-0.25 m returns.
                            khd_m == 333 ||
        // 334 = KH_LIVE_CAM_PAIR off - the live-table seal pairs its matrix
        // with the INJECTION camera again.
                            khd_m == 334 ||
        // 335 = KH_BAND_PAIR_SAME - band seals pair the harvested sm with the
        // view of ITS OWN capture frame (staged path: the held capture-time
        // view; immediate path: the seal keeps its frame view, the next
        // publish does not replace it).
                            khd_m == 335 ||
        // 336 = KH_BAND_FOREIGN_VIEW off - foreign cameras (PIP / UAV feed /
        // mirror renders) may be committed into band seals again.
                            khd_m == 336 ||
        // 337 = KH_BAND_PAIR_MIXED weld guards OFF (stage-commit drop +
        // pending-completion invalidate revert to).
                            khd_m == 337 ||
        // 338 = KH_BAND_ALL_NEAR - every band tier reseals at the near
        // cadence (0.05 s). Read bandCadenceForced for engagement and WATCH
        // bandStageWaitMs + frame time (whole-atlas copy cost).
                            khd_m == 338 ||
        // 339 = KH_BAND_COHERENT_COMPLETE OFF. NOTE: INERT - the default IS
        // the pre-overwrite, so 339 equals baseline.
                            khd_m == 339 ||
        // 340 = KH_BAND_SAME_SOURCE OFF. NOTE: INERT - the branch lives
        // inside the mode-341 stack and one mode slot cannot hold both, so
        // 340 yields BASELINE. Reserved.
                            khd_m == 340 ||
        // 341 = THE FULL EXPERIMENTAL COMPLETION STACK (-flip + adaptive
        // tails + fast reseal). The DEFAULT is baseline (completions
        // overwrite unconditionally; original epoch death).
                            khd_m == 341 ||
        // 342 = same-source lock. NOTE: INERT - reachable only inside the
        // mode-341 stack, which one mode slot cannot combine with 342, so 342
        // yields BASELINE. Reserved.
                            khd_m == 342 ||
        // 343 = KH_BAND_ALT_PROVISIONAL (alt-birth; FIELD-CONVICTED
        // regression - pairs bands with random other cascades, 48.8 m class.
        // Kept for archaeology only).
                            khd_m == 343 ||
        // 344 = the flip's TAIL FIXES OFF. NOTE: INERT - reachable only
        // inside the mode-341 stack, which one mode slot cannot combine with
        // 344, so 344 yields BASELINE. Reserved.
                            khd_m == 344 ||
        // 345 = KH_BAND_INJ_PAIR ON (archaeology; field-falsified - constant
        // 3-9 texel offset, worse and sooner: the injection camera is not the
        // receive-reconstruction camera).
                            khd_m == 345 ||
        // 346 = KH_BAND_STAGE_FLIP - the staged commit welds the FIRST
        // cross-source publish latched after THIS band's own stage (per-band
        // time-adjacency) instead of the commit-time global frame_view
        // lottery.
                            khd_m == 346 ||
        // 347 = the pool-miss fall-through UNCAPPED (every miss takes an
        // immediate whole-atlas copy - the pre-behaviour; restores the
        // threshold-crossing frame-drop storm).
                            khd_m == 347 ||
        // 348 = epoch-death fast reseal OFF (an invalidated pending band
        // waits out its tier interval again - the capture/camera- switch
        // shadow vanish recovers slowly, tier by tier).
                            khd_m == 348 ||
        // 349 = receive warm-up RE-ARMS on every wipe (the 2 s shadow
        // blackout after F12 captures / camera-class switches returns;
        // session-start warm-up is identical either way).
                            khd_m == 349 ||
        // 350 = the 1.0 s band age-kill bound (restores the one-frame
        // far-shadow hole at F12 captures / camera switches).
                            khd_m == 350 ||
        // 351 = rotational weld coherence OFF (the far cascade's one-boundary
        // rotation skew returns - the far-pose offset).
                            khd_m == 351 ||
        // 352 = stencil range fade OFF (lighting0.y 43; engine unit-shadow
        // darkening on our meshes pops again at the shadow view distance
        // instead of thinning over the last ~15%).
                            khd_m == 352 ||
        // 353 = KH_SELF_SOFT_CMP OFF (lighting0.y 44; the hard PCF corner
        // compare returns - the one-switch proof for the grazing / crevice /
        // halo speckle class).
                            khd_m == 353 ||
        // 354 = KH_SELF_SPREAD_SMOOTH OFF (lighting0.y 45; the int-quantized
        // ring spread returns - per-quad footprint jumps).
                            khd_m == 354 ||
        // 355 = VISUAL 29 (dbgCtl.x 29) - receive-term decomposition paint: R
        // = cascade/band receive occlusion, G = our sun-map self term, B =
        // stencil volume.
                            khd_m == 355 ||
        // 356 = KH_SELF_REL_INTERP revert (self chain returns to the
        // world-absolute interpolant + PS-side anchor sub; the camera-motion
        // millimetre jitter should RETURN under it).
                            khd_m == 356 ||
        // 357 = KH_SELF_PREFILTER revert (classic taps only; the
        // grazing/crevice static speckle should RETURN under it).
                            khd_m == 357 ||
        // 358 = PF-EXCLUSIVE (lighting0.y 47) - armed band verdicts from the
        // pyramid alone; pairs with 357 to separate tap vs prefilter
        // authorship at the hole in two screenshots.
                            khd_m == 358 ||
        // 359 = VISUAL 30 (pyramid-vs-depth probe; hero window; grey healthy
        // / red-only convert dead / red+green chain dead / displaced
        // silhouette = addressing flip).
                            khd_m == 359 ||
        // 360 = VISUAL 31 (Load-vs-SampleLevel splitter; R+B silhouettes with
        // G flat convicts the linear sampler on RG32F).
                            khd_m == 360 ||
        // 361 = KH_PF_CPU_PROBE (staging readback of the pyramid and depth;
        // STALLS THE PIPE - dump, then return to 0).
                            khd_m == 361 ||
        // 362 = unbounded grad slack (KH_GS_CLAMP revert; the halo dots and
        // crevice salt-and-pepper should RETURN).
                            khd_m == 362 ||
        // 363 = KH_BAND_STABLE_RING revert (rotated decision ring + raw 7.5
        // km hash; the fireflies and cast-shadow halo dither should RETURN).
                            khd_m == 363 ||
        // 364 = KH_VOL_MAX_FALLBACK revert (the stencil-resolve weight
        // collapse falls back to the single-texel point read; the traced
        // crevice fireflies should RETURN).
                            khd_m == 364 ||
        // 365 = KH_SELF_CLASSIC - the textbook self kernel in one flip (gs
        // slack 0, RPDB 0, spread 1, offset damper off, prefilter off;
        // lighting0.y 50).
                            khd_m == 365 ||
        // 366 = THE ERA ARM - classic kernel + hero/mid/outer stand-down +
        // prefilter off: the closest single mode to the pre-accretion
        // self-shadow era (union-only maps).
                            khd_m == 366 ||
        // 367 = KH_UNION_LIT_GUARD revert (the union override of lit band
        // verdicts returns; the cast-shadow halo, whole-face dither and
        // union-scale shimmer should ALL RETURN).
                            khd_m == 367 ||
        // 368 = straddle guard OFF (unguarded footprint spread + pf
        // engagement; the crevice fireflies and shadow-edge dither should
        // RETURN).
                            khd_m == 368 ||
        // 369 = facet bias normal re-armed (override; crevice fireflies, edge
        // dither and the camera-motion film grain should RETURN). 233 is an
        // alias of the default now.
                            khd_m == 369 ||
        // 370 = range-fade curve returns (fade from 85% of shadowVisibility;
        // self shadows thin ~30 m early).
                            khd_m == 370 ||
        // 371 = thin 10% blend band + always-on jurisdiction return (the
        // quality seams snap again; halo protections stay).
                            khd_m == 371 ||
        // RELOCATION: 381 = KH_BAND_REL_INTERP revert (the
        // absolute-interpolant A/B).
                            khd_m == 381 ||
        // 382 = KH_SUN_LADDER_SCALE revert (mid/outer bands back to the fixed
        // 8/32 m footprints; tier switch distances stop scaling with
        // shadowVisibility).
                            khd_m == 382 ||
        // 383 = KH_SUN_UNION_LAT_FIT revert (union texel priced by the
        // engine-axis enclosing sphere again; the far-caster balloon returns
        // - expect sunUnionTexelM back at the 0.6 m class in spread-out
        // scenes).
                            khd_m == 383 ||
        // 384 = KH_BLK_SLEW_REGIME revert (unconditional publish slew
        // returns; skipTime lighting decays at 25%/s again - expect the
        // glowing-mesh tail after day-to-night skips).
                            khd_m == 384 ||
        // 385 = KH_NIGHT_ZERO_SUN revert (the unconditional zero-sun refusal
        // returns; expect flat-white/glowing meshes at night while
        // blkZeroSunRefusals climbs).
                            khd_m == 385 ||
        // 386 = KH_JURIS_EDGE_FADE revert (lighting0.y 60; the hard
        // certification cliff returns - expect the sharp quality- level line
        // at the far tier handoffs).
                            khd_m == 386 ||
        // 387 = fixed 12/48/192 m jurisdiction guards (lighting0.y 61; the
        // pre-literals - under-claims at ladder scale > 1, expect residual
        // mid/outer mush on certified surfaces at high shadowVisibility).
                            khd_m == 387 ||
        // 388 = KH_TIER_HOLD revert (lighting0.y 62; 0.60 fade start returns
        // - the fine tier yields a third of its window to the coarser mix
        // again).
                            khd_m == 388 ||
        // 389 = KH_SUN_FAR_BAND OFF (the range-priced far band stands down;
        // the union's mesh-priced texels serve the far field again - expect
        // wobble and the mesh-size quality law to return together).
                            khd_m == 389 ||
        // 428 = KH_BAND_FWD_LATCH OFF - the camera bands' DEPTH window
        // re-fits to the admitted set every frame again, form.
                            khd_m == 428 ||
        // 429 = KH_BLK_CONTRA_PENDING OFF - the standing-match branch
        // executes the bright pending on every sighting again, the pre-form.
                            khd_m == 429 ||
        // 430 = KH_NIGHT_ADMIT_WITNESS OFF - zero-sun admit trusts the
        // measured-dark standing alone again. Night sessions are byte-
        // identical (witness below horizon; Admits climb, WitRefusals 0).
                            khd_m == 430 ||
        // 431 = KH_STEN_CYCLE_TAP OFF. The tap itself lives on as arm 432
        // below.
                            khd_m == 431 ||
        // 432 = KH_STEN_CYCLE_TAP ARMED - cycle-basis tap, field-falsified as
        // a default (brief offset at abrupt movement; the cycle latch is
        // stale on exactly those frames -), kept as the controlled A/B arm.
                            khd_m == 432 ||
        // 433 = KH_STARVED_DIM_HOLD OFF - a starved adoption crowns the first
        // resumer again whatever its brightness (the dump2-dim entry, ratio
        // 0.222, returns).
                            khd_m == 433 ||
        // 434 = KH_STEN_CENTER_TAP OFF - the witnessed 7x7 filter returns as
        // it stood through (dbgCtl.w 0).
                            khd_m == 434 ||
        // 435 = KH_CASCBIND_RESURRECT OFF - the cascade-bind harvest stands
        // down and the live table depends on the funnel feed alone again (the
        // dump3-outage state whenever the funnel starves post-skip).
                            khd_m == 435 ||
        // 436 = KH_LIVE_WIPE_PARITY OFF - the sun-jump wipe clears the live
        // cascade table again, and the post-skip mesh goes shadowless until
        // the engine's lazy visibility-driven re-render refills it (the
        // dump1-drought).
                            khd_m == 436 ||
        // 437 = KH_OBJ_VIS_CUT OFF - farVis-off meshes cut at the projection
        // far plane again instead of the engine's object view distance (the
        // fog-tinted mesh past the last engine object).
                            khd_m == 437 ||
        // 438 = KH_MESH_OWNER_PREPASS OFF - no owner map, no owner draws, no
        // head-of-PSComposite rejection; every fragment pays the full late-Z
        // shader again.
                            khd_m == 438 ||
        // 439 = KH_TEX_CACHE OFF - stb decode + GenerateMips path, bit-exact
        // (no.khtc read or write, mipless without a context).
                            khd_m == 439 ||
        // 440 = KH_LIVE_WIPE_DEFER OFF - the lock-boundary wipes (doors 3, 4
        // and 5; door 5 is the cold-hold release that fires when a recompile
        // finishes) empty the live cascade table immediately again instead of
        // deferring to the first post-lock commit; with a parked camera the
        // mesh then goes shadowless until a look-away.
                            khd_m == 440 ||
        // 441 = VISUAL 33, OWNER-REJECT PAINT - every fragment the owner map
        // rejects returns magenta instead of discarding.
                            khd_m == 441 ||
        // 445 arms the hold.
                            khd_m == 442 ||
        // 443 = KH_TIER_PARTNER_PAINT - inside every blend band the ladder
        // returns the PARTNER tier's verdict alone (both chains and the world
        // cast).
                            khd_m == 443 ||
        // 444 = KH_PF_RAMP_FLOOR OFF - the self kernel's moment filter takes
        // precision floor (2.5e-7 normalised) as its sigma again wherever the
        // footprint ramp sits under it.
                            khd_m == 444 ||
        // 445 = KH_TIER_FADE_DIR ON - hold, opt-in: a shadowed fine verdict
        // holds through 90 pct of its window against a lighter partner.
                            khd_m == 445 ||
        // 446 = KH_ABSENCE_WITNESS OFF - a band certifies absence from a
        // clear texel and the cast chain trusts a band's lit verdict WITHOUT
        // the union map's witness again.
                            khd_m == 446 ||
        // 447 = KH_BAND_ADMIT_NORM OFF - band admission returns to radial
        // (disc) test, which refuses casters whose shadows land in the square
        // window's corners (the 50 m fade's root, dump3).
                            khd_m == 447 ||
        // 426 = KH_BAND_BIAS_TEXELS OFF - the camera bands' receive bias goes
        // back to HALF a texel from two, bit-exactly.
                            khd_m == 426 ||
        // They armed KH_SUN_FIT_SCALE, KH_SUN_ER_TIGHTEN and KH_SUN_OVERSIZE,
        // all three of which are gone from the file; no reader remains, so
        // setting them does nothing at all. They stay listed so a stale
        // script cannot be refused, and so that a future build cannot mint
        // them again over a fielded meaning.
                            khd_m == 425 || khd_m == 424 || khd_m == 423 ||
        // 427 = KH_SHADOW_LOD BACK ON - the sun depth ladder and the mask
        // cast take level 2 again, form.
                            khd_m == 427 ||
        // 422 = KH_SHADOW_LOD OFF. The seam and volume clones were never in
        // this change, so a registration fault is NOT this mode.
                            khd_m == 422 ||
        // 421 = KH_MESH_SORT OFF - one far-first order for every mesh again.
        // Under it opaques draw back to front, so an occluded fragment still
        // runs the whole shadow ladder before failing the depth test.
                            khd_m == 421 ||
        // 420 = KH_MESH_CULL OFF - no frustum test; every registered mesh is
        // submitted every flush and every injection, which is behaviour.
                            khd_m == 420 ||
        // 419 = KH_MESH_LOD CROSSFADE OFF. The ladder still selects and the
        // levels still draw, but the dithered transition pair does not - each
        // level swaps in whole at its boundary.
                            khd_m == 419 ||
        // 418 = KH_MESH_LOD OFF.
                            khd_m == 418 ||
        // 417 = KH_MOD_CACHE OFF. Only the writable Documents cache is
        // consulted; mod-shipped.khsc and.khmc files are ignored.
                            khd_m == 417 ||
        // 416 = KH_USER_SETTLE OFF - the pump arms the instant one unarmed
        // request exists, form.
                            khd_m == 416 ||
        // 415 = KH_USER_MAT_WHITE OFF - a submesh whose user.hlsl is still
        // compiling, or whose compile FAILED, falls back to builtin PBR with
        // its real textures again, which is behaviour.
                            khd_m == 415 ||
        // 414 = KH_ACQ_COLD_HOLD OFF - the shadow view scan runs during the
        // cold-cache deferral again and the acquisition chain is NOT reset
        // when the gate finally opens, which is behaviour and the one the
        // field convicted: mesh lit but receiving no shadows and no cascades
        // until a mission restart.
                            khd_m == 414 ||
        // 413 = KH_MESH_CACHE_MT OFF - the.khmc write and the 1 GiB trim run
        // SYNCHRONOUSLY on the game thread again, above the register, exactly
        // as they did through
                            khd_m == 413 ||
        // 412 = KH_FX_BATCH OFF - the post-processing unit leaves the startup
        // batch and ensure_effect_shader compiles it inline on the calling
        // thread again, which is behaviour.
                            khd_m == 412 ||
        // 411 = KH_USER_ASYNC OFF - user.hlsl effect and material shaders
        // compile SYNCHRONOUSLY on the render thread inside the graphics lock
        // again, the pre-behaviour, which for a material means a
        // PSMain/PSComposite-class unit and a multi-minute freeze on a cold
        // cache.
                            khd_m == 411 ||
        // 410 = THE WHITE-PREVIEW FORM, RESTORED WHOLE.
                            khd_m == 410 ||
        // 409 = KH_WHITE_PREVIEW OFF - the flat-white placeholder pass stands
        // down and the screen stays empty through the whole cold compile,
        // behaviour exactly.
                            khd_m == 409 ||
        // 407 = KH_POOL_LIFETIME REVERT - restores teardown form whole: the
        // abort latch is cleared at the BOTTOM of kh_shader_mt_shutdown again
        // (so a detached worker resumes claiming units), the 5 s wait becomes
        // unconditional including on the two device-reset paths that hold the
        // graphics lock, and a batch that spawned no worker at all is armed
        // rather than stood down.
                            khd_m == 407 ||
        // 408 = KH_QUERY_DEFER REVERT - gpuVisibility and sampleSceneDepth
        // treat the KH_SHADERS_COMPILING sentinel as a fatal error again and
        // answer [["error","shaders compiling"]] for the whole cold-compile
        // window, behaviour.
                            khd_m == 408 ||
        // 406 = KH_SHADER_ASYNC OFF - ensure_resources BLOCKS the render
        // thread through the whole cold compile again, the pre-form.
                            khd_m == 406 ||
        // 403/404/405 = KH_SHADER_FLAGS experiment arms, set BEFORE the first
        // spawn. 403 = PREFER_FLOW_CONTROL, 404 = OPTIMIZATION_ LEVEL0, 405 =
        // SKIP_OPTIMIZATION.
                            khd_m == 403 || khd_m == 404 || khd_m == 405 ||
        // 402 = KH_SHADER_MT OFF - shaders compile SERIALLY on the calling
        // thread, the pre-behaviour. Set it before the first ensure to take
        // effect; the long cold-cache startup returns with it.
                            khd_m == 402 ||
        // 401 = KH_FAR_SELF_CERT_SCOPE REVERT, lighting0.y 71 - far-self gate
        // returns, and with it the outer-to-far hard cut and the far-tier
        // wobble, together.
                            khd_m == 401 ||
        // 400 = KH_FAR_SELF_CERT_SCOPE. 401 is its revert; 392 is the
        // UNBOUNDED form of the same lift and now isolates the kh4_cert
        // bound, differing from the default only where outer did not certify.
                            khd_m == 400 ||
        // 399 = KH_BAND_SHADOW_ADMIT revert (C++-side only, no lighting0.y
        // code; the camera bands stop re-admitting a caster whose shadow
        // segment reaches them, so their caster sets fall short of the
        // union's again and a long shadow can be certified away by a band
        // that never held its caster).
                            khd_m == 399 ||
        // 398 = KH_FAR_GUARD_ADMIT as an OPT-IN (lighting0.y 69; clamps the
        // far tier's certified guard to its admission bound - the withdrawn
        // default, a real over-claim repair but unproven as any artifact's
        // author).
                            khd_m == 398 ||
        // 396 = KH_CERT_CLEAR revert (lighting0.y 67; a clear texel stops
        // certifying, so the halo class returns and visual 32 paints those
        // pixels RED again).
                            khd_m == 396 ||
        // 395 = visual 32, the certification probe (pure gauge - paints why
        // each pixel is or is not certified; nothing gated).
                            khd_m == 395 ||
        // 394 = KH_CERT_CONTENT revert (lighting0.y 66; certification returns
        // to surface-identity test, so the layered-fabric certification hole
        // reopens and the splotch plus its halo return together).
                            khd_m == 394 ||
        // 393 = KH_PF_SRV_RESTORE revert (C++-side only, NO lighting0.y code
        // - kh_sun_pf_convert stops restoring the caller's t0/t1 pair, so the
        // injection path draws with depthTex and shadowAtlas unbound on every
        // band-render frame again, exactly as it did -).
                            khd_m == 393 ||
        // 392 = KH_FAR_SELF_SCOPE revert (lighting0.y 65; the far self tier
        // serves inside the finer ladder's coverage again - the all-distance
        // splotch ring returns).
                            khd_m == 392 ||
        // 284 = tier-proportional floor OFF (lighting0.y 37; arm, now
        // reachable). Fielded once; never remint. Fielded once; never remint.
        // Expect: distance noise returns.
                            khd_m == 284 ||
        // 285 = diamond ring opt-in (lighting0.y 38). Expect: fewer taps,
        // more penumbra banding - the retired default.
                            khd_m == 285 ||
        // 286 = matrix-offset scan OFF (fixed [180] only, the pre-behavior).
        // Expect at 1x: receive dies again.
                            khd_m == 286 ||
        // 287 = FSAA-toggle wipe OFF (held receive state survives an FSAA
        // change - the pre-behavior). Expect under 287: toggling FSAA
        // mid-session can poison lighting until restart.
                            khd_m == 287 ||
        // 288 = crop-measured bias opt-in (lighting0.y 39: slope 0.8, floor
        // x1.25, damper off).
                            khd_m == 288 ||
        // 289 = mission-teardown ordering revert, behavior wholesale:
        // destroy-before-disarm on the success path, and on lock exhaustion
        // the destroy is SKIPPED outright (the deferral never arms;
        // missionResetFails still counts under both arms).
                            khd_m == 289 ||
        // 290 = FSAA-requirement stand-down OFF - world meshes and the shadow
        // machinery attempt to run at 1x again (behavior: meshes render, lit
        // ones full-bright albedo, shadowLiveLatches 0, the whole retired
        // campaign's symptom set).
                            khd_m == 290 ||
                            khd_m == 98 ||
                            // 84 = DEBUG VISUAL 18, the sealed-tile content
                            // probe. It is a free number carrying a visual
                            // because all eighteen 0-17 slots are spent; the
                            // mesh fill maps it to dbgCtl.x 18.
                            khd_m == 84 ||
                            // 88 reverts the overlap-anchored reseal budget
                            // to the border[0] < 10 classification.
                            khd_m == 88 ||
                            // 99 reverts the zero-sun publish refusal.
                            khd_m == 99 ||
                            // 199 = the receiver-depth compensation on the
                            // stencil tap (rides dbgCtl.w as 5.0).
                            (khd_m >= 129 && khd_m <= 221 && khd_m != 216) ||   // 201; -203..221
                            khd_m == 110 || khd_m == 111 ||   // era volume paints were NEVER whitelisted
                            khd_m == 113 || khd_m == 115 ||   // "mode 110 was never run" because it never could run
                            khd_m == 118 ||   // vol-depth + count composite paint (was inert)
                            // 175 = the SAME transform over a CAMERA-RELATIVE
                            // position instead of absolute world - 174
                            // resolved the flicker but landed the footprint
                            // at the ground's depth, which is what a
                            // wrong-space position does; 176 = THE FIX: our
                            // own transform for x/y/w with only the ENGINE'S
                            // DEPTH PAIR remapping z - space-agnostic, since
                            // b2's position space proved to be neither
                            // absolute nor camera-relative while its depth
                            // pair is exact 370/370.
                            (khd_m >= 0 && khd_m <= 17) ||   // shader visuals (10-13 are the LADDERS: see the)
                                                             // g_dbg_mode catalog. The 'retired' note
                                                             // here was stale - corrected; 14 is free)
                            khd_m == 18 ||   // dark re-seed refusal OFF (the black-box)
                            khd_m == 19 ||   // last-fire clamp with a FRESH epoch on
                                                              // the late fire (= 40 without the epoch
                                                              // restore). The motion-drift experiment.
                            khd_m == 20 ||   // cast ownership kill switch
                            khd_m == 21 ||   // cast readiness latch + slab retirement OFF (pristine)
                            khd_m == 24 ||   // terrain snap off (diagnostic)
                            khd_m == 25 ||   // cast viewport: live grid (pristine) instead of frozen
                            khd_m == 26 ||   // lock-settle cast hold off (diagnostic)
                            khd_m == 27 ||   // Ucoverage debug view (write window)
                            khd_m == 30 ||   // matrix-defect repair bypass (pristine)
                            khd_m == 31 ||   // ghost tint - per-CYCLE parity (odd cycles lighter)
                            khd_m == 32 ||   // ghost tint - per-FIRE index (re-fires lighter)
                            khd_m == 33 ||   // depth-snapshot freeze OFF (pointer-freeze)
                            khd_m == 34 ||   // FLOOD - every paint pixel forced fully dark
                                                              // (does our paint reach the screen at
                                                              // all?)
                            khd_m == 37 ||   // pristine sun-axis replay (boot)
                                                              // reference, cold bar and no-view refusal
                                                              // all off)
                            khd_m == 39 ||   // freeze from the ENGINE render view
                                                              // skips the bridge adoption (person-view
                                                              // pitch ghost)
                            khd_m == 40 ||   // clamp to the LAST fire (behaviour)
                                                              // the default is now the FIRST fire)
                            khd_m == 22 ||   // LAST-fire clamp OFF - every fire paints
                            khd_m == 35 ||   // moved-epoch MUTE restored (0.35 strength)
                                                              // retired by default - the clamp replaces
                                                              // it)
                            khd_m == 36 ||   // FIRST-fire clamp (default)
                                                              // default is now the SRV-bound fire)
                            khd_m == 38 ||   // REUSED: relock RECOVERY BAR OFF
                                                              // (revert to the 1e-3-only relock probe).
                                                              // The default also reclaims the remembered
                                                              // view source at the bar its own drop was
                                                              // measured against, forced-drop path only,
                                                              // camera- relative only, two consecutive
                                                              // cycles.
                            khd_m == 44 ||   // ENABLES
                                                              // form (0.05 s freshness, always left
                                                              // pending for a later publish to
                                                              // complete). Default frame-anchors the
                                                              // view and seals vcol in the epoch sm came
                                                              // from.
                            khd_m == 43 ||   // BRIDGE ARM off - revert to the
                                                              // publish-gated arm (behaviour). The
                                                              // default arms the cast on the bridge view
                                                              // the freeze actually paints with, so a
                                                              // late publish no longer holds the fire
                                                              // past its own read window. This is the
                                                              // flicker fix.
                            khd_m == 42 ||   // MOTION-GATED CARRY ON (opt-in). The
                                                              // 27-42 px of double, so the 1 px gate
                                                              // refuses every frame that needs it and
                                                              // opens only on static blocks that never
                                                              // transition. Kept as an opt-in so the
                                                              // effect can be seen.
                            khd_m == 41 ||   // ring RUNNING ARGMIN OFF (revert to the)
                                                              // bare 16-slot ring). The ring is wiped at
                                                              // the injection and the injection lands
                                                              // BEFORE the view upload on every late
                                                              // cycle on record, so the hunt samples the
                                                              // last sixteen uploads before draw ~74 out
                                                              // of ~2518 a cycle. Default scores every
                                                              // candidate as it arrives.
                            khd_m == 23 ||   // REUSED: view-lock DROP HYSTERESIS OFF
                                                              // (revert to the bare bar: a reject streak
                                                              // drops the lock on ANY overshoot, however
                                                              // small). Default is the 1.5x hysteresis;
                                                              // this restores
                            khd_m == 59 ||   // DL DENSE GATE back to 4 sightings AND
                                                              // a 40 ms absolute gap bar. That bar is
                                                              // tied to frame rate, not to evidence
                                                              // quality, and our own harvest cadence is
                                                              // 47-189 ms - so it threw entries with
                                                              // 6-11 sightings onto a flat 250 ms floor
                                                              // 2.5x their own worst gap. Default gates
                                                              // on sightings alone (>= 6).
                            khd_m == 58 ||   // OCCLUDER-IDENTITY TOLERANCE back to
                                                              // 0.25 cell / 2 m floor = 3.125 m on a
                                                              // 12.5 m grid, which calls every kerb,
                                                              // wall, rock and fence 'terrain' and
                                                              // drives the contact band through them.
                                                              // Default is 0.08 cell / 1 m. Too tight
                                                              // fails CLOSED - the terrain bite returns
                                                              // at the ground line and mode 10 paints it
                                                              // cyan where the gate suppressed.
                            khd_m == 56 ||   // CONTACT BAND back to 25 m. At 25 the
                                                              // clearance ramp is full-strength below
                                                              // 17.5 m of ground clearance, so the 'thin
                                                              // strip at the ground line' the design
                                                              // describes is in fact the WHOLE mesh -
                                                              // which is why mode 11 was never black.
                                                              // Default is 6 m: full pull below 4.2 m,
                                                              // zero above 6.0 m.
                            khd_m == 57 ||   // DL TTL MULTIPLIER back to 6x the worst
                                                              // observed re-sight gap. 6x a MAXIMUM is
                                                              // not a confidence interval, it tolerates
                                                              // six consecutive worst-case misses: the
                                                              // census read tailMean 558 ms against
                                                              // lifeMean 368, max 908. Default is 2x.
                            khd_m == 55 ||   // FAR-ARB NEAR FLOOR OFF - arm the
                                                              // analytic depth clamp on every injection.
                                                              // The floor is not only a far-frame
                                                              // classifier - it keeps a metres-scale
                                                              // depth lie (0.06*d = 3.0 m at 50 m) away
                                                              // from close geometry, against an
                                                              // identity-gate tolerance of 3.125 m that
                                                              // cannot discriminate at that scale.
                            khd_m == 53 ||   // DL CADENCE-MARK DECAY off - restore
                                                              // That mark never decayed, so one unlucky
                                                              // sampling gap priced an entry's tail at
                                                              // 6x that gap forever and permanently
                                                              // barred it from the dense fast lane - the
                                                              // 0.25-1 s muzzle-flash/lightning tails.
                                                              // the LIFETIME gap high-water mark.
                            khd_m == 54 ||   // BLOCK MIRROR/STANDING COHERENCE
                                                              // That is the slight camera-rotation
                                                              // dimming: the mirror is overwritten
                                                              // inside the frame by a pass flavor the
                                                              // arbitration never granted standing, and
                                                              // the publish ships it. off - publish
                                                              // whatever the mirror holds at flush time,
                                                              // certified or not. Watch blkIncohHolds.
                            khd_m == 52 ||   // NEED-BASED PULL off - restore the
                            khd_m == 51 ||   // EXACT DEPTH PASS-THROUGH off - go
                                                              // The default writes the rasterizer's own
                                                              // i.pos.z whenever nothing deliberately
                                                              // moves the depth. back to recomputing the
                                                              // fragment's NDC from the sniffed pair on
                                                              // every pixel.
                            khd_m == 50 ||   // DEFERRED CAPTURE off - revert to the
                                                              // The default stages the new sm/border
                                                              // instead and commits the whole triple at
                                                              // once when the pairing view lands, so the
                                                              // slot always holds a complete, coherent
                                                              // seal and there is nothing to either
                                                              // offset or refuse.
                            khd_m == 48 ||   // HALF-BUILT SEAL REFUSAL off - revert
                                                              // Absent beats offset. Watch
                                                              // bandPendRefused (expect ~= the old
                                                              // bandPendConsumed) and bandPickNone
                                                              // (expect UNCHANGED - a climb there means
                                                              // the offset was traded for a dropout). to
                                                              // refusing only the BRIDGE-provisional
                                                              // case (behaviour).
                            khd_m == 49 ||   // OCCLUDER IDENTITY GATE off - the
                            khd_m == 45 ||   // RECEIVE SEAL COMPLETE-AT-CAPTURE
                                                              // Watch bandProvSkips / bandPickNone: a
                                                              // dropout traded for the offset falsifies
                                                              // it too. When a same-frame ENGINE view
                                                              // exists the seal is complete at capture,
                                                              // so no later publish can overwrite a
                                                              // coherent frame-N sm + frame-N vcol pair
                                                              // with a frame-N+1 view.
                            khd_m == 46 ||   // sun-map CASTER CALM ROLL off - restore
                            khd_m == 47 ||   // COLD RECEIVE HEALTH GATE off - the
                                                              // receive un-gates on the 2 s warm-up
                                                              // alone (behaviour). The default also
                                                              // requires a locked view source, a fresh
                                                              // publish stream and a converged derived
                                                              // sun, held for 500 ms, and drops every
                                                              // cold-era seal at the rise. For the
                                                              // foreign-receive cold start.
                            khd_m == 87 ||   // THE FEATURE REVERT, AND THE ONLY THING
                                                              // It is NOT a product safety net and must
                                                              // not be reasoned about as one - the
                                                              // shipped fallback is kh_svs_sten_ready,
                                                              // gauged by svStenRejCold and
                                                              // svStenRejDims. Removal date: the
                                                              // scaffolding strip.
                            khd_m == 109 ||   // THE VOLUME TRANSPORT. Reads the stencil
                            khd_m == 110 ||   // PAINT THE STENCIL COUNT - THE WRAP
                            khd_m == 111 ||   // PAINT THE FOOTPRINT RESIDUAL as a ladder
                            khd_m == 128 ||   // THE PROJECTION HALF. 127 shared the
                                                              // VIEW and proved it, and svReprojPxMean
                                                              // did not move - so the residual offset is
                                                              // the PROJECTION, which is also where
                                                              // injNear's 0.07<->0.77 oscillation lives.
                                                              // 128 shares the boundary pair too, still
                                                              // subject to the near-class gate.
                            khd_m == 127 ||   // ONE PUBLICATION FOR BOTH PASSES.
                            khd_m == 126 ||   // RESTORE REPROJECTION.
                                                              // 126 puts the reprojected read back so 's
                                                              // truth table can be re-run under the
                                                              // VOLUME transport it was never measured
                                                              // against.
                            khd_m == 125 ||   // PROVABLY OR ABSENT.
                                                              // Depth we never wrote cannot be tested
                                                              // wrongly, so the stencil artifact stops
                                                              // being reduced and starts being
                                                              // impossible. Arms the engine view like
                                                              // 116 AND suppresses the injection
                                                              // entirely on frames where no candidate
                                                              // basis validates - zero-area triangles,
                                                              // no depth written. Cost: no stencil
                                                              // shadow on those frames.
                            khd_m == 124 ||   // DISABLE THE AGREEMENT BOUND.
                                                              // The engine-view path keeps its
                                                              // structural gates but stops refusing a
                                                              // basis that disagrees with the historic
                                                              // transform by more than a quarter screen.
                            khd_m == 121 || khd_m == 122 || khd_m == 123 ||
                                                              // FORCE THE COPY TO READ WIDE- ENUMERATION
                                                              // CANDIDATE 1/2/3 instead of the latched
                                                              // offset. Byte 0 is a SHARED pool offset
                                                              // (engBindRejects 0 with engVerifyFail 44
                                                              // proves the binding cannot separate its
                                                              // two writers); if a family-private copy
                                                              // of the view block exists further into
                                                              // the 64 KB pool, engVerifyFail collapses
                                                              // on the right candidate.
                            khd_m == 120 ||   // STATEBACKUP CONSTANT-BUFFER OFFSET REVERT.
                                                              // Restores VS CBs through the plain
                                                              // (non-1.1) setter, which drops the
                                                              // engine's per-slot firstConstant - the
                                                              // pre-behaviour.
                            khd_m == 119 ||   // REVERT TO THE PRE-MASK TRANSPORT.
                            khd_m == 116 ||   // INJECTION ON THE ENGINE'S OWN VIEW.
                            khd_m == 117 ||   // VISIBLE MESH ON THE ENGINE'S OWN VIEW
                                                              // NOTE mode 107 was never this test: 107
                                                              // draws the mesh with the INJECTION's
                                                              // stale matrix, the worst available value.
                                                              // injection unchanged.
                            khd_m == 118 ||   // BOTH. If 116 and 117 each hold, this is
                                                              // the shipping candidate: both passes on
                                                              // the engine's view means they agree with
                                                              // each other and with the volumes by
                                                              // construction, and reprojection becomes
                                                              // unnecessary rather than merely better -
                                                              // which retires the epoch ring, the
                                                              // footprint witness, the sentinel, the
                                                              // tolerance, modes 111-115 and the
                                                              // interlacing.
                            khd_m == 115 ||   // VERDICT DISAGREEMENT PAINT - the one view
                            khd_m == 114 ||   // FOOTPRINT TOLERANCE BACK TO 1e-4 (was the)
                            khd_m == 113 ||   // RAW UNGUARDED COUNT PAINT - 110's
                            khd_m == 112 ||   // FOOTPRINT TEST OFF under the volume
                                                              // transport (tolerance zeroed): accept
                                                              // every reprojected sample. Separates the
                                                              // witness's cost in coverage from its
                                                              // benefit in correctness with one
                                                              // variable.
                            khd_m == 108 ||   // PRIME WITH 1.0 + DISOCCLUSION GUARD OFF
                                                              // Brings the halo back.
                            khd_m == 107 ||   // VISIBLE MESH IN ENGINE SPACE. Draws the
                            khd_m == 106 ||   // DISOCCLUSION GUARD ON (opt-in). Defaulted
                                                              // OFF because it did not move the halo:
                                                              // the reprojected lookup is landing INSIDE
                                                              // our footprint and the value there is
                                                              // still wrong, so the footprint is not the
                                                              // fault. Costs one extra Load per mesh
                                                              // pixel, so it does not ride on the
                                                              // default path unproven.
                            khd_m == 105 ||   // RETIRED TO A NO-OP (the guard is off by)
                                                              // default now - see mode 106). Was: guard
                                                              // OFF exactly: reprojection with no
                                                              // validity test on the reprojected texel.
                                                              // 0 vs 105 vs 104 is the full ladder:
                                                              // guarded reprojection, raw reprojection,
                                                              // no reprojection.
                            khd_m == 104 ||   // REPROJECTION OFF (raster lookup). The
                                                              // The ring stores the ABSOLUTE transform
                                                              // now and the subtraction is gone. If the
                                                              // halo survives that, the aliasing is
                                                              // inherent and 104 is the answer.
                            khd_m == 103 ||   // RETIRED TO A NO-OP (reprojection is the)
                                                              // Enabled three times now , a caster ghost
                                                              // every time, the strip never once moved.
                                                              // default now - see mode 104). Was:
                                                              // opt-in.
                            khd_m == 102 ||   // INJECTION VIEW-ADOPTION OFF (opt-in).
                                                              // Pair with svSwingPxMean, which reads a
                                                              // tautological 0 whenever adoption is off.
                                                              // Backed out of the default: under mode
                                                              // 100 the on-box stencil cut disappeared,
                                                              // so the injection DOES need the visible
                                                              // draw's view and was right.
                            khd_m == 101 ||   // RETIRED TO A NO-OP (its revert is the)
                                                              // default now - see mode 103). Was: HALF B
                                                              // OFF - the mesh reads the mask at its own
                                                              // raster position instead of reprojecting
                                                              // through the injection's transform.
                                                              // Restores the on-box stencil strip. Pair
                                                              // with svReprojPxMean.
                            khd_m == 100 ||   // RETIRED TO A NO-OP (its revert is the)
                                                              // default now - see mode 102). Pair with
                                                              // svSwingPxMean and svInjViewAdopts.
                                                              // Neither half works alone: 83 (= half A)
                                                              // fixed the cascade cut and left the
                                                              // stencil strip; reprojection alone  only
                                                              // added a ghost, because it was aligning
                                                              // the lookup to a footprint that was
                                                              // itself misplaced.
                            khd_m == 98 ||   // INJECTION DEPTH-CLIP REVERT. Restores
                            khd_m == 97 ||   // MASK CLAMP BOUND FORCED to the mask's
                                                              // fxMeta.zw is filled TWICE per object and
                                                              // the second fill uses the occlusion guard
                                                              // snapshot's size, not the mask's - added
                                                              // the unconditional fill and never removed
                                                              // the conditional one. own dimensions.
                            khd_m == 96 ||   // SPLIT ARM B: injection OFF, snapshots
                                                              // The mesh multiplies by a mask our depth
                                                              // never reached. and multiply ON. If the
                                                              // missing-shadow sliver survives, the mask
                                                              // content is wrong for reasons unrelated
                                                              // to our injection.
                            khd_m == 95 ||   // SPLIT ARM A: injection and snapshots ON
                                                              // MULTIPLY OFF. If the box-edge sliver
                                                              // survives, it comes from our depth
                                                              // entering the volume buffer and NOT from
                                                              // the shading term - i.e. the two slivers
                                                              // are two faults. Neither 95 nor 96
                                                              // paints, so the geometry stays legible.
                            khd_m == 94 ||   // STENCIL REPROJECTION ON (OPT-IN, and it)
                                                              // does NOT fix the silhouette strip - it
                                                              // is kept only because the machinery is
                                                              // what measured the strip's cause OUT).
                                                              // Modes 90, 91 and 92 are all retired
                                                              // no-ops now: the default IS the raster
                                                              // lookup.
                            khd_m == 93 ||   // DARK-STANDING HARD ESCAPE OFF. Restores
                                                              // the pre-behaviour where a sun-zero
                                                              // lighting standing under a live static
                                                              // sun can hold indefinitely - i.e. it
                                                              // restores the black-box-on-cold-start
                                                              // fault. Pair with blkDarkForceAdopts
                                                              // (stops climbing) and blkStickyRejects
                                                              // (resumes climbing without bound).
                                                              // Diagnostic only.
                            khd_m == 92 ||   // STENCIL REPROJECTION OFF. The default is
                                                              // now EPOCH-MATCHED reprojection:
                                                              // svPostAgeMax measured 1, so the mask the
                                                              // mesh reads is one frame older than the
                                                              // mesh, and the lookup uses the transform
                                                              // stamped with the SNAPSHOT's epoch. 92
                                                              // falls back to the raster position, which
                                                              // restores the silhouette strip. Read
                                                              // svReprojEpochHits / svReprojEpochMiss.
                            khd_m == 91 ||   // STENCIL REPROJECTION ON - RETIRED (the)
                            khd_m == 90 ||   // STENCIL REPROJECTION REVERT - RETIRED
                            khd_m == 89 ||   // INJECTION TESSELLATION REVERT. Leaves
                            khd_m == 80 ||   // INJECTION VIEWPORT REVERT. Restores
                                                              // MinDepth 0 / MaxDepth 1 in
                                                              // kh_volume_seam_inject instead of the
                                                              // engine's own 0.011 / 0.999. Paints the
                                                              // isolated stencil term exactly as 73, so
                                                              // 73 vs 80 is one variable with one
                                                              // visual. Read svInjVpLo/Hi and
                                                              // svInjVpReverts.
                            khd_m == 86 ||   // MASK-PRIMING REVERT. Stops writing lit
                                                              // into the engine mask over our footprint,
                                                              // so pre carries the BACKGROUND's cascade
                                                              // verdict again and KhStenRatio's pre <=
                                                              // 0.02 guard discards the stencil term
                                                              // wherever the terrain behind our mesh is
                                                              // fully shadowed - the blue band. Paints
                                                              // as 73.
                            khd_m == 83 ||   // RETIRED TO A NO-OP: this revert IS the
                            khd_m == 82 ||   // INJECTION RASTERIZER REVERT. Inherits
                                                              // the engine's volume-pass rasterizer
                                                              // instead of rast_sun, so depth is clamped
                                                              // not clipped, carries whatever bias the
                                                              // engine had, and is culled however the
                                                              // engine culls. The pre-path. Paints as
                                                              // 73. Read svInjRsInherit.
                            khd_m == 81 ||   // INJECTION REBASE REVERT. Keeps the
                                                              // absolute (un-rebased) viewProj and
                                                              // leaves center_rel unarmed - the
                                                              // pre-path. Also paints as 73. Read
                                                              // svInjAbs: under 81 it takes over from
                                                              // svInjRebases one for one.
                            khd_m == 79 ||   // SUBTRACTIVE STENCIL FORM. Paints
                                                              // 1 - saturate(pre - post) instead of the
                                                              // guarded division. Continuous, so no step
                                                              // at the edge of a cascade shadow, but it
                                                              // under-darkens a full stencil shadow to 1
                                                              // - pre rather than 0.
                            khd_m == 78 ||   // RATIO INPUTS PER CHANNEL. R = post
                                                              // (live mask), G = pre (snapshot), B = the
                                                              // sten ratio. Yellow = correct
                                                              // cancellation; red/green fringes on
                                                              // building edges are the residual.
                            khd_m == 77 ||   // MASK-ADOPTION / WITH-RTV COUPLING
                            khd_m == 76 ||   // REPURPOSED: LIVE-POST. Reads post
                                                              // from the live mask instead of the
                                                              // bracketed snapshot - i.e. Under 76 our
                                                              // own mask_cast_engine paint lands between
                                                              // the resolve and our draw, so the box's
                                                              // OWN cast shadow comes back as a
                                                              // counterfeit stencil term; under every
                                                              // other selector it cannot.
                            khd_m == 75 ||   // THE FEATURE. Multiply the isolated
                                                              // stencil term into the shading path. Band
                                                              // receive (cascades) is untouched, so
                                                              // terrain and building shadows must look
                                                              // exactly as they do at mode 0 while unit
                                                              // shadows now land on the mesh.
                            khd_m == 74 ||   // PAINT THE PRE SNAPSHOT ALONE. The
                                                              // copy's own liveness: black everywhere
                                                              // means the CopyResource never landed and
                                                              // sten is pinned to 1, which mode 73 alone
                                                              // could not tell apart from a wrong ratio.
                            khd_m == 73 ||   // PAINT THE ISOLATED STENCIL TERM
                                                              // (post/pre). Those belong to the cascade
                                                              // half of the mask, which this discards.
                            khd_m == 72 ||   // PER-FRAME WITH-RTV SET RESTORED
                                                              // behaviour, which measured svOrdRtvFirst
                                                              // 0 against svOrdOnlyFirst 1863: the set
                                                              // was wiped between the prepass and the
                                                              // volume pass every frame, so no seam
                                                              // could ever fire.
                            khd_m == 71 ||   // ENGINE SHADOW-MASK PAINT. Arms the
                            khd_m == 70 ||   // IN-HOOK SEAM INJECTION RESTORED
                                                              // behaviour: draw from inside the OM hook
                                                              // instead of deferring to the next draw.
                                                              // Runs in-game either way; the difference
                                                              // is whether a RenderDoc capture contains
                                                              // the draw.
                            khd_m == 69 ||   // SEAM ADJACENCY RULE RESTORED
                                                              // back to 's "immediately previous bind"
                                                              // test, which measured a 47% miss rate
                                                              // against a boundary present every frame.
                                                              // Pair with svSeamAdj.
                            khd_m == 68 ||   // SHADOW-VOLUME SEAM DEPTH INJECTION
                                                              // ON (opt-in; default is census only).
                                                              // Renders our meshes DEPTH-ONLY into the
                                                              // engine's scene depth buffer at the
                                                              // prepass -> stencil-volume seam, so the
                                                              // volume counting sees our surface. Pair
                                                              // with svInjects + svInjectDraws: both
                                                              // must climb.
                            khd_m == 67 ||   // PROJECTION-CENSUS RETENTION OFF
                                                              // restore 's zeroing of the census at the
                                                              // stats arm. Pair with encNearRefM reading
                                                              // -1 in a freshly armed dump.
                            khd_m == 66 ||   // NEAR-CLASS IONS OFF - restore
                                                              // Pair with encEndorseRejects: under 66 it
                                                              // stops climbing because nothing can be
                                                              // refused an endorsement any more. NOT
                                                              // restored under 66: 's probe also SPENT
                                                              // encNearClassRejects on every look.
                            khd_m == 65 ||   // ENCODE NEAR-CLASS AGREEMENT OFF
                                                              // restore the absolute near band alone.
                                                              // Underwater this brings the punch-through
                                                              // back. Pair with encNearClassRejects.
                            khd_m == 64 ||   // BELOW-LAYER ATMOSPHERIC STAND-DOWN
                                                              // OFF - run the above-layer fog/haze model
                                                              // with the camera underwater. Pair with
                                                              // fogBelowStands + fogBelowCamY.
                            khd_m == 61 ||   // FOG FAR-FADE RAMP OFF ENTIRELY
                                                              // restore the unconditional far-fade ramp.
                                                              // Pair with fogRampStands + fogEngEnd.
                            khd_m == 62 ||   // ENGINE DECAY LANE OFF - revert to
                                                              // the SQF-staged fogParams decay. At
                                                              // staged decay 0 this restores the flat
                                                              // fog fill. Pair with fogDecayShipped +
                                                              // fogDecaySubs.
                            khd_m == 63 ||   // RELATIVE DARK RE-SEED BAR OFF
                                                              // back to the absolute 1.0 of Pair with
                                                              // blkDarkSeedBlocks.
                            khd_m == 60 ||   // ENGINE DISTANCE HAZE OFF. Zeroes
                                                              // hazePars.w, so KhHazeT returns 1.0 and
                                                              // the atmospheric block falls back to its
                                                              // fog-only arming - the mesh goes crisp
                                                              // against hazed terrain again, which IS
                                                              // the fault. Pair with hazeArms: the
                                                              // counter stops climbing under 60.
                            khd_m == 29 ||   // shader reject bypass - locality clamp and
                                                              // near floor forced to pass (look-down
                                                              // diagnostic)
                            khd_m == 28;   // REUSED: last-fire clamp with the camera
        const bool khd_dead = khd_m == 397 ||
                              khd_m == 90  || khd_m == 91  || khd_m == 92  ||
                              khd_m == 94  || khd_m == 100 || khd_m == 101 ||
                              khd_m == 103 || khd_m == 104 || khd_m == 105 ||
                              khd_m == 106 || khd_m == 109 || khd_m == 120 ||
                              khd_m == 135 ||
                              khd_m == 147 || khd_m == 151 || khd_m == 169 ||
                              khd_m == 172 ||
                              // The question it asked is moot now that serves
                              // every pool miss immediately (PoolFall ==
                              // PoolMiss, DropAge/DropDead 0), so it is
                              // delisted rather than repaired.
                              khd_m == 213 ||
                              khd_m == 390 ||   // fielded revert, withdrawn - never remint (rule 1.18)
                              khd_m == 391 ||   // fielded revert, withdrawn - never remint (rule 1.18)
                              // 381 briefly sat HERE by a misfile - it is a
                              // LIVE revert, not a retired number, and is
                              // relocated to the accept list beside 371. 85
                              // was the live-atlas fallback. Removed, not
                              // recycled.
                              khd_m == 85;
        if (!khd_ok || khd_dead) return game_value(false);
        RenderIntegration::g_dbg_mode.store(khd_m, std::memory_order_relaxed);
        return game_value(true);
    } catch (...) {
        return game_value(false);
    }
}

// This re-zeroes the same block the arming call zeroes, so a mode can be
// measured in a clean window without restarting the session. Cold-timeline
// stamps are session state and are deliberately NOT touched.
static game_value reset_render_stats_sqf() {
    try {
        RenderIntegration::reset_stat_counters();
        return game_value(true);
    } catch (...) {
        return game_value(false);
    }
}

static game_value get_render_stats_sqf() {
    try {
        // OPT-IN (see g_stats_armed): first call arms + zeroes the pure
        // diagnostics and returns a status pair; stats flow from call two.
        // Arms the flight recorder as well (see g_diag_armed).
        RenderIntegration::g_diag_armed.store(true, std::memory_order_relaxed);

        if (!RenderIntegration::g_stats_armed.exchange(true, std::memory_order_relaxed)) {
            RenderIntegration::reset_stat_counters();
            auto_array<game_value> pair;
            pair.push_back(game_value("status"));
            pair.push_back(game_value("armed"));
            auto_array<game_value> armed_out;
            armed_out.push_back(game_value(std::move(pair)));
            return game_value(std::move(armed_out));
        }

        auto kv = [](const char* k, uint64_t v) {
            auto_array<game_value> pair;
            pair.push_back(game_value(k));
            pair.push_back(game_value(static_cast<float>(v)));
            return game_value(std::move(pair));
        };
        auto kvf = [](const char* k, float v) {
            auto_array<game_value> pair;
            pair.push_back(game_value(k));
            pair.push_back(game_value(v));
            return game_value(std::move(pair));
        };

        auto_array<game_value> out;
        out.push_back(kv("flushes", RenderIntegration::g_stats.flushes));
        out.push_back(kv("gatePassed", RenderIntegration::g_stats.gate_passed));
        out.push_back(kv("lockRetries", RenderIntegration::g_stats.lock_retries));
        out.push_back(kv("lockFailedFrames", RenderIntegration::g_stats.lock_failed_frames));
        out.push_back(kv("skipNoDsv", RenderIntegration::g_stats.skip_no_dsv));
        out.push_back(kv("skipWrongPass", RenderIntegration::g_stats.skip_wrong_pass));
        out.push_back(kv("effectSetupFails", RenderIntegration::g_stats.effect_setup_fails));
        out.push_back(kv("uiFlushes", RenderIntegration::g_stats.ui_flushes));
        out.push_back(kv("uiGatePassed", RenderIntegration::g_stats.ui_gate_passed));
        out.push_back(kv("uiGateSkips", RenderIntegration::g_stats.ui_gate_skips));
        out.push_back(kv("uiMaskClears", RenderIntegration::g_ui_mask_clears));
        out.push_back(kv("uiMaskSkips", RenderIntegration::g_ui_mask_skips));
        out.push_back(kv("uiCovVetoes", RenderIntegration::g_ui_cov_vetoes));
        // chain GPU timing (timestamp ring; last completed frame):
        out.push_back(kv("fxChainGpuUs", RenderIntegration::g_stats.fx_chain_gpu_us));
        out.push_back(kv("fxTopFxId", RenderIntegration::g_stats.fx_top_fx_id));
        out.push_back(kv("fxTopFxUs", RenderIntegration::g_stats.fx_top_fx_us));
        out.push_back(kv("fxUiGpuUs", RenderIntegration::g_stats.fx_ui_gpu_us));
        out.push_back(kv("fxUiTopFxId", RenderIntegration::g_stats.fx_ui_top_fx_id));
        out.push_back(kv("fxUiTopFxUs", RenderIntegration::g_stats.fx_ui_top_fx_us));
        // scene-capture GPU cost (per-flush aggregate):
        out.push_back(kv("fxSceneCapUs", RenderIntegration::g_stats.fx_scene_cap_us));
        // CPU-side flush attribution (Campaign-18 Step-1; QPC wall time, last
        // completed measurement - the two-dump experiment's keys: CPU stats
        // stay FLAT across 4K vs 1080p, GPU rings scale):
        out.push_back(kv("fxCpuParkUs", RenderIntegration::g_stats.fx_cpu_park_us));
        out.push_back(kv("fxCpuFlushUs", RenderIntegration::g_stats.fx_cpu_flush_us));
        out.push_back(kv("fxCpuUiParkUs", RenderIntegration::g_stats.fx_cpu_ui_park_us));
        out.push_back(kv("fxCpuUiFlushUs", RenderIntegration::g_stats.fx_cpu_ui_flush_us));
        out.push_back(kv("fxCpuMaskUs", RenderIntegration::g_stats.fx_cpu_mask_us));
        out.push_back(kv("fxCpuSnapUs", RenderIntegration::g_stats.fx_cpu_snap_us));
        out.push_back(kv("fxCpuUiSnapUs", RenderIntegration::g_stats.fx_cpu_ui_snap_us));
        out.push_back(kv("uiOnlyDraws", RenderIntegration::g_ui_only_draws));
        // arming-path census - arms = UI-phase-thread compose detections
        // (~1/frame while a UI-mode pass is visible); aborts expected 0
        // (pending clear killed by a foreign target).
        out.push_back(kv("uiMaskArms", RenderIntegration::g_ui_mask_arms));
        out.push_back(kv("uiMaskAborts", RenderIntegration::g_ui_mask_aborts));
        // bndEmaMs = live boundary cadence in ms (~the frame period; the
        // adaptive floor's basis).
        out.push_back(kv("uiMaskFloorHolds", RenderIntegration::g_ui_mask_floor_holds));
        out.push_back(kvf("uiMaskBndEmaMs",
            RenderIntegration::kh_qpc_ticks_to_us(RenderIntegration::g_ui_mask.bnd_ema_ticks) / 1000.0f));
        // Lo keys are the low 24 bits of the weak identities (SQF scalars are
        // 32-bit floats; full pointers do not survive the trip). probes > 0
        // with misses ~ probes and arms 0 = identity mismatch (compare
        // ProbeLastLo against LearnLo0); probes > 0 with a matching Lo and
        // arms > 0 = machine armed.
        out.push_back(kv("uiMaskProbes", RenderIntegration::g_ui_mask_probes));
        out.push_back(kv("uiMaskProbeMisses", RenderIntegration::g_ui_mask_probe_misses));
        out.push_back(kv("uiMaskProbeLastLo",
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(RenderIntegration::g_ui_mask.last_probe_id) & 0xFFFFFFu)));
        out.push_back(kv("uiMaskLearnLo0",
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(RenderIntegration::g_ui_mask.bb_id[0]) & 0xFFFFFFu)));
        out.push_back(kv("uiMaskLearnN", static_cast<uint64_t>(RenderIntegration::g_ui_mask.bb_n)));
        // 999 = never sampled. AlphaMin/Max (, retained as the live health
        // check): 255/255 = the coverage channel is pinned (no clear landed
        // this frame); a low pair = the injected mask is alive.
        out.push_back(kv("uiMaskAlphaMin", static_cast<uint64_t>(
            RenderIntegration::g_ui_mask.alpha_min < 0 ? 999 : RenderIntegration::g_ui_mask.alpha_min)));
        out.push_back(kv("uiMaskAlphaMax", static_cast<uint64_t>(
            RenderIntegration::g_ui_mask.alpha_max < 0 ? 999 : RenderIntegration::g_ui_mask.alpha_max)));
        // the apply-once gate census - ~0 in normal play, ~frame count while
        // a pause menu / map / editor holds the frame still (each of those
        // frames would otherwise have compounded the UI-phase effects onto
        // their own previous output).
        out.push_back(kv("uiStaleSkips", RenderIntegration::g_ui_stale_skips));
        out.push_back(kv("uiMaskStallArms", RenderIntegration::g_ui_mask_stall_arms));
        out.push_back(kv("compositeInjections", RenderIntegration::g_stats.composite_injections));
        out.push_back(kv("compositeMeshes", RenderIntegration::g_stats.composite_meshes));
        out.push_back(kv("compositeSkips", RenderIntegration::g_stats.composite_skips));
        // SESSION STATE like the latches themselves - reset_render_stats
        // leaves these alone (cold-timeline rule). The active bit already
        // rides the long-standing reorderHook lane below and is NOT
        // duplicated here.
        out.push_back(kv("reorderHookAttempts", RenderIntegration::g_reorder_hook_attempts));
        out.push_back(kvf("reorderHookMhStatus", static_cast<float>(RenderIntegration::g_reorder_hook_mh_status)));
        out.push_back(kvf("reorderHookFailPhase", static_cast<float>(RenderIntegration::g_reorder_hook_fail_phase)));
        out.push_back(kvf("reorderHookFailSlot", static_cast<float>(RenderIntegration::g_reorder_hook_fail_slot)));
        out.push_back(kv("reorderHookVtDrift", RenderIntegration::g_reorder_hook_vt_drift));
        // the retry ladder + the no-MSAA trigger census (catalog entry; the
        // first census lane reading 0 in a no-FSAA dump names the broken
        // trigger link).
        out.push_back(kvf("reorderHookFailRounds", static_cast<float>(RenderIntegration::g_reorder_hook_fail_count)));
        out.push_back(kv("roCycBlend", RenderIntegration::g_ro_cyc_blend));
        out.push_back(kv("roCycNoWrite", RenderIntegration::g_ro_cyc_nowrite));
        out.push_back(kv("roCycMainDsv", RenderIntegration::g_ro_cyc_maindsv));
        out.push_back(kv("roCycTrig", RenderIntegration::g_ro_cyc_trig));
        out.push_back(kv("msaaDepthSamples", static_cast<uint64_t>(RenderIntegration::g_res.depth_sample_count)));
        out.push_back(kv("locRefState", static_cast<uint64_t>(
            (RenderIntegration::g_fog_valid ? 1u : 0u) |
            (RenderIntegration::g_fog[1] > 1.0e-4f ? 2u : 0u) |
            (RenderIntegration::g_ls.cam[1] != 0.0f ? 4u : 0u))));
        out.push_back(kv("hookDrawPass", RenderIntegration::g_hook_draw_pass.load(std::memory_order_relaxed)));
        out.push_back(kv("hookDrawTidBail", RenderIntegration::g_hook_draw_tidbail.load(std::memory_order_relaxed)));
        out.push_back(kv("hookDrawForeign", RenderIntegration::g_hook_draw_foreign.load(std::memory_order_relaxed)));
        out.push_back(kv("hookMapTidBail", RenderIntegration::g_hook_map_tidbail.load(std::memory_order_relaxed)));
        out.push_back(kv("hookMapForeign", RenderIntegration::g_hook_map_foreign.load(std::memory_order_relaxed)));
        out.push_back(kvf("hookFctxType0", static_cast<float>(RenderIntegration::g_hook_fctx_type[0])));
        out.push_back(kvf("hookFctxType1", static_cast<float>(RenderIntegration::g_hook_fctx_type[1])));
        out.push_back(kvf("hookFctxType2", static_cast<float>(RenderIntegration::g_hook_fctx_type[2])));
        out.push_back(kvf("hookFctxType3", static_cast<float>(RenderIntegration::g_hook_fctx_type[3])));
        out.push_back(kv("trigMissCycles", RenderIntegration::g_trigmiss_cycles));
        out.push_back(kv("trigMissDsvNull", RenderIntegration::g_trigmiss_null));
        out.push_back(kv("trigMissMainTex", RenderIntegration::g_trigmiss_mainmatch));
        out.push_back(kv("trigMissW", static_cast<uint64_t>(RenderIntegration::g_trigmiss_w)));
        out.push_back(kv("trigMissH", static_cast<uint64_t>(RenderIntegration::g_trigmiss_h)));
        out.push_back(kv("trigMissFmt", static_cast<uint64_t>(RenderIntegration::g_trigmiss_fmt)));
        out.push_back(kv("trigMissSamp", static_cast<uint64_t>(RenderIntegration::g_trigmiss_samp)));
        // trig-miss extras + the DSV-bind census + block pipeline.
        out.push_back(kv("trigMissDraws", RenderIntegration::g_trigmiss_draws));
        out.push_back(kv("trigMissAfterMain", RenderIntegration::g_trigmiss_after_main));
        out.push_back(kv("trigMissOpq", static_cast<uint64_t>(RenderIntegration::g_trigmiss_opq)));
        out.push_back(kvf("trigMissVpMin", RenderIntegration::g_trigmiss_vpmin));
        out.push_back(kvf("trigMissVpMax", RenderIntegration::g_trigmiss_vpmax));
        out.push_back(kvf("dsvCenN", static_cast<float>(RenderIntegration::g_dsvcen_n)));
        for (int khdl_i = 0; khdl_i < 6; ++khdl_i) {
            const std::string khdl_p = "dsvCen" + std::to_string(khdl_i);
            out.push_back(kv((khdl_p + "W").c_str(), static_cast<uint64_t>(RenderIntegration::g_dsvcen_w[khdl_i])));
            out.push_back(kv((khdl_p + "H").c_str(), static_cast<uint64_t>(RenderIntegration::g_dsvcen_h[khdl_i])));
            out.push_back(kv((khdl_p + "Fmt").c_str(), static_cast<uint64_t>(RenderIntegration::g_dsvcen_fmt[khdl_i])));
            out.push_back(kv((khdl_p + "Samp").c_str(), static_cast<uint64_t>(RenderIntegration::g_dsvcen_samp[khdl_i])));
            out.push_back(kv((khdl_p + "Binds").c_str(), RenderIntegration::g_dsvcen_binds[khdl_i]));
        }
        out.push_back(kv("blkApplies", RenderIntegration::g_blk_applies));
        out.push_back(kv("blkHolds", RenderIntegration::g_blk_holds));
        out.push_back(kv("blkRateHolds", RenderIntegration::g_blk_rate_holds));   // (retired;)
        out.push_back(kv("blkSnapAdopts", RenderIntegration::g_blk_snap_adopts));
        out.push_back(kv("blkSnapRejects", RenderIntegration::g_blk_snap_rejects));
        out.push_back(kv("blkRingPicks", RenderIntegration::g_blk_ring_picks));
        out.push_back(kv("blkRingBandFiltered", RenderIntegration::g_blk_ring_band_filtered));
        out.push_back(kv("blkRingNoBand", RenderIntegration::g_blk_ring_no_band));
        out.push_back(kv("blkRingStarveHolds", RenderIntegration::g_blk_ring_starve_holds));
        out.push_back(kv("blkPubSlews", RenderIntegration::g_blk_pub_slews));
        // 0 across a session containing a skip = the bypass never engaged
        // (mode 384, or the skip landed inside 3x and was slewed - read
        // blkAmbLum's frame trace for the decay tail then).
        out.push_back(kv("blkSlewRegimeCommits", RenderIntegration::g_blk_slew_regime_commits));
        out.push_back(kv("sunVpSpanXM", RenderIntegration::g_kh_sunvp_span[0]));
        out.push_back(kv("sunVpSpanYM", RenderIntegration::g_kh_sunvp_span[1]));
        out.push_back(kv("sunVpSpanZM", RenderIntegration::g_kh_sunvp_span[2]));
        out.push_back(kv("sunVpLatches", RenderIntegration::g_kh_sunvp_latches));
        out.push_back(kv("sunVpFitGrows", RenderIntegration::g_kh_sunvp_fit_grows));
        // the 1x sweep's inner verdict.
        out.push_back(kv("rtResolveTrue", RenderIntegration::g_rt_resolve_true));
        out.push_back(kv("rtResolveFalse", RenderIntegration::g_rt_resolve_false));
        out.push_back(kv("rtLastRejW", RenderIntegration::g_rt_last_rej_w));
        out.push_back(kv("mtxScanHits", RenderIntegration::g_mtx_scan_hits));
        out.push_back(kv("mtxScanOff", RenderIntegration::g_mtx_scan_off));
        out.push_back(kv("msaaToggleWipes", RenderIntegration::g_msaa_toggle_wipes));
        out.push_back(kv("missionResetFails", RenderIntegration::g_mission_reset_fails));   // mission-end lock exhaustions (destroy deferred, never skipped)
        out.push_back(kv("fsaaStandDownFrames", RenderIntegration::g_fsaa_standdown_frames));   // injection triggers refused by the FSAA requirement
        out.push_back(kv("fsaaDepthSamples", RenderIntegration::g_scene_depth_samples));   // the learned scene-depth sample count (1 = standing down, 0 = not yet learned; learned at the flush adoption)
        // mode-lane census + cache telemetry.
        out.push_back(kvf("blkModeLast", RenderIntegration::g_light_probe.last_mode));
        out.push_back(kvf("blkModeV0", RenderIntegration::g_blk_mode_census_v[0]));
        out.push_back(kv("blkModeN0", RenderIntegration::g_blk_mode_census_n[0]));
        out.push_back(kvf("blkModeV1", RenderIntegration::g_blk_mode_census_v[1]));
        out.push_back(kv("blkModeN1", RenderIntegration::g_blk_mode_census_n[1]));
        out.push_back(kvf("blkModeV2", RenderIntegration::g_blk_mode_census_v[2]));
        out.push_back(kv("blkModeN2", RenderIntegration::g_blk_mode_census_n[2]));
        out.push_back(kvf("blkModeV3", RenderIntegration::g_blk_mode_census_v[3]));
        out.push_back(kv("blkModeN3", RenderIntegration::g_blk_mode_census_n[3]));
        out.push_back(kv("shaderCacheHits", RenderIntegration::g_shader_cache_hits.load(std::memory_order_relaxed)));
        out.push_back(kv("shaderCacheMisses", RenderIntegration::g_shader_cache_misses.load(std::memory_order_relaxed)));
        // CPU ms summed across every compiling thread, NOT wall time - read
        // it against shaderMtWallMs; the ratio IS the parallelism.
        out.push_back(kv("shaderCompileMs", RenderIntegration::g_shader_compile_ms.load(std::memory_order_relaxed)));
        out.push_back(kv("shaderMtWallMs", RenderIntegration::g_khsm_wall_ms));
        out.push_back(kv("shaderMtThreads", RenderIntegration::g_khsm_threads_n));
        out.push_back(kv("shaderMtBatches", RenderIntegration::g_khsm_batches));
        out.push_back(kv("shaderMtJobs", RenderIntegration::g_khsm_jobs.load(std::memory_order_relaxed)));
        out.push_back(kv("shaderMtStolen", RenderIntegration::g_khsm_stolen.load(std::memory_order_relaxed)));
        out.push_back(kv("shaderMtWaits", RenderIntegration::g_khsm_waits.load(std::memory_order_relaxed)));
        // shaderSlowestMs is the AMDAHL BOUND - the longest single compile
        // unit, the floor no pool width can go under. The per-unit breakdown
        // is the "KH shader census" RPT line, one per batch.
        out.push_back(kv("shaderUnits", RenderIntegration::g_khsu_units.load(std::memory_order_relaxed)));
        out.push_back(kv("shaderSlowestMs", RenderIntegration::g_khsu_slowest_ms.load(std::memory_order_relaxed)));
        // async: AsyncFrames counts frames served WHILE compiling - the
        // arming lane, non-zero means the game was playable through it.
        // MtLive/MtDetached are the teardown lanes: Detached non-zero means a
        // mission ended with a compile genuinely in flight.
        out.push_back(kv("shaderAsyncFrames", RenderIntegration::g_khsa_frames));
        out.push_back(kv("shaderAsyncMs", RenderIntegration::g_khsa_ms));
        out.push_back(kv("shaderMtLive", static_cast<uint64_t>(RenderIntegration::g_khsm_live.load(std::memory_order_relaxed))));
        out.push_back(kv("shaderMtDetached", RenderIntegration::g_khsm_detached));
        // teardown/arming lanes. StallMs is the ms actually spent inside
        // kh_shader_mt_shutdown's wait - the graphics-lock cost of a teardown
        // that landed mid-compile, near 0 on a normal session. SpawnFails
        // counts batches stood down for want of a single worker thread.
        out.push_back(kv("shaderMtStallMs", RenderIntegration::g_khsm_stall_ms));
        out.push_back(kv("shaderMtSpawnFails", RenderIntegration::g_khsm_spawn_fails));
        // Frames/Draws are the arming lanes - both 0 on a warm cache is
        // correct, not a failure, because a warm session never defers and
        // never builds the pass. KH_WHITE_PREVIEW. CompileMs is the
        // placeholder pair's ONE-TIME cost and is the lane that tests the
        // parsing-is-free premise: tens of ms is the prediction, tens of
        // seconds falsifies it.
        out.push_back(kv("whiteCompileMs", RenderIntegration::g_khw_compile_ms));
        out.push_back(kv("whiteFrames", RenderIntegration::g_khw_frames));
        out.push_back(kv("whiteDraws", RenderIntegration::g_khw_draws));
        out.push_back(kv("whiteVbFails", RenderIntegration::g_khw_vb_fails));
        out.push_back(kv("userAsyncQueued", RenderIntegration::g_user_async_queued));
        out.push_back(kv("userAsyncDone", RenderIntegration::g_user_async_done));
        out.push_back(kv("userAsyncFails", RenderIntegration::g_user_async_fails));
        out.push_back(kv("userAsyncBatches", RenderIntegration::g_user_async_batches));
        out.push_back(kv("userAsyncRelost", RenderIntegration::g_user_async_relost));
        out.push_back(kv("userAsyncDrops", RenderIntegration::g_user_async_drops));
        // Zero with a user material assigned means the substitute never
        // engaged - check that ps_white exists, since mode 409 suppresses it.
        // Counting while userAsyncQueued climbs is the compile window and is
        // expected to stop; counting FOREVER with userAsyncFails non-zero is
        // a broken shader file and the RPT carries the fxc error once.
        out.push_back(kv("userMatWhiteDraws", RenderIntegration::g_user_mat_white_draws));
        // A large SettleWaits with batches never climbing means the queue is
        // growing every frame and the cap is carrying it. Small and non-zero
        // is the feature working - two or three per material.
        out.push_back(kv("userSettleWaits", RenderIntegration::g_user_settle_waits));
        // modShaderHits and modMeshHits are blobs served by a mod rather than
        // by the user's own cache; they are a SUBSET of shaderCacheHits and
        // fbxCacheHits, never additional to them. KH_MOD_CACHE.
        out.push_back(kv("modCacheDirs", RenderIntegration::g_mod_cache_dirs_n));
        out.push_back(kv("modShaderHits",
                         RenderIntegration::g_mod_shader_hits.load(std::memory_order_relaxed)));
        out.push_back(kv("modMeshHits", RenderIntegration::g_mod_mesh_hits));
        out.push_back(kv("modTexHits", RenderIntegration::g_mod_tex_hits));   // KH_TEX_CACHE
        // lodFades counts draws that issued the crossfade PAIR: zero while
        // lodMeshes is non-zero and the camera is moving means the selector
        // never enters a fade band and the crossfade is not being exercised
        // at all. KH_MESH_LOD.
        out.push_back(kv("lodMeshes", RenderIntegration::g_lod_meshes));
        out.push_back(kv("lodLevels", RenderIntegration::g_lod_levels));
        out.push_back(kv("lodBuildMs", RenderIntegration::g_lod_build_ms));
        out.push_back(kv("lodFades", RenderIntegration::g_lod_fades));
        // CHANGED THE ACCOUNTING OF ALL FOUR AND THE HISTORIC NUMBERS DO NOT
        // COMPARE ACROSS IT. And meshConsidered stopped counting fullscreen
        // passes, which have no world extent and can never be culled, so the
        // frustum ratio stopped being diluted by them.
        out.push_back(kv("meshConsidered", RenderIntegration::g_mesh_considered));
        out.push_back(kv("meshCulled", RenderIntegration::g_mesh_culled));
        out.push_back(kv("meshTris", RenderIntegration::g_mesh_tris));
        out.push_back(kv("meshTrisL0", RenderIntegration::g_mesh_tris_l0));
        // WHAT THEY DO AND DO NOT SETTLE: they price the mesh passes, so
        // meshTris against meshTrisL0 finally has a cost attached. A pass
        // that never armed - no queries, a busy slot, a disjoint interval -
        // simply does not count itself, so N below the pass count is a
        // skipped measurement rather than a fast frame.
        out.push_back(kv("meshGpuUs",  RenderIntegration::g_msh_gpu_us));
        out.push_back(kv("meshGpuN",   RenderIntegration::g_msh_gpu_n));
        out.push_back(kv("meshGpuMaxUs", RenderIntegration::g_msh_gpu_max));
        out.push_back(kv("injGpuUs",   RenderIntegration::g_inj_gpu_us));
        out.push_back(kv("injGpuN",    RenderIntegration::g_inj_gpu_n));
        out.push_back(kv("injGpuMaxUs", RenderIntegration::g_inj_gpu_max));
        // THE CULL'S SELF-CHECK. cullBuilds is passes that built a frustum
        // the apex guard accepted; cullStandDowns is passes that refused, and
        // a refusing pass culls NOTHING - it is the safe state, not an error.
        out.push_back(kv("cullBuilds", RenderIntegration::g_cull_builds));
        out.push_back(kv("cullStandDowns", RenderIntegration::g_cull_standdowns));
        // A pass that stands down culls NOTHING, so standDowns is a report
        // and never a lost object. cullRebased counts passes whose matrix was
        // camera-rebased - expect it to track cullBuilds on this renderer,
        // because both mesh passes rebase.
        out.push_back(kvf("cullApexMax", RenderIntegration::g_cull_apex_max));
        out.push_back(kv("cullRebased", RenderIntegration::g_cull_rebased));
        out.push_back(kv("offthreadTrigs", RenderIntegration::g_offthread_trigs.load(std::memory_order_relaxed)));
        out.push_back(kvf("hookInstallAtS", RenderIntegration::g_hook_install_at_s));
        out.push_back(kv("compositeAmbiguous", RenderIntegration::g_stats.composite_ambiguous));
        out.push_back(kv("compositeProjLock", RenderIntegration::g_stats.composite_proj_lock));
        out.push_back(kv("compositeRearms", RenderIntegration::g_stats.composite_rearms));
        out.push_back(kv("compositeRejSpan", RenderIntegration::g_stats.composite_rej_span));
        out.push_back(kv("compositeRejVerify", RenderIntegration::g_stats.composite_rej_verify));
        out.push_back(kv("compositeRejFloor", RenderIntegration::g_stats.composite_rej_floor));
        out.push_back(kv("compositeSlotEncodes", RenderIntegration::g_stats.composite_slot_encodes));
        out.push_back(kv("compositeFarPhaseSkips", RenderIntegration::g_stats.composite_far_phase_skips));
        out.push_back(kv("compositeFarInjects", RenderIntegration::g_stats.composite_far_injects));
        out.push_back(kv("compositeFarArbs", RenderIntegration::g_stats.composite_far_arbs));
        out.push_back(kv("compositeFarArbDenied", RenderIntegration::g_stats.composite_far_arb_denied));
        // injections the near floor would leave on the STOCK path but that
        // armed anyway.
        out.push_back(kv("arbNearDenied", RenderIntegration::g_arb_near_denied));
        // verdict changes of the near classification - every flip is a
        // discontinuity in the mesh's depth against nearby world geometry.
        // Any next model must explain an effect that persists with this lane
        // flat.
        out.push_back(kv("arbNearFlips", RenderIntegration::g_arb_near_flips));
        out.push_back(kv("flushFxArbs", RenderIntegration::g_stats.flush_fx_arbs));
        out.push_back(kv("compositeTranslDefers", RenderIntegration::g_stats.composite_transl_defers));
        out.push_back(kv("texturedDraws", RenderIntegration::g_stats.textured_draws));
        out.push_back(kv("texLoads", RenderIntegration::g_stats.tex_loads));
        // KH_TEX_CACHE (mode 439 bypasses): hits skip decode and mips, misses
        // pay texMipMs once and queue a write; texCacheLoadMs prices a hit.
        out.push_back(kv("texCacheHits", RenderIntegration::g_stats.tex_cache_hits));
        out.push_back(kv("texCacheMisses", RenderIntegration::g_stats.tex_cache_misses));
        out.push_back(kv("texCacheWrites", RenderIntegration::g_stats.tex_cache_writes));
        out.push_back(kv("texMipMs", RenderIntegration::g_stats.tex_mip_ms));
        out.push_back(kv("texCacheLoadMs", RenderIntegration::g_stats.tex_cache_load_ms));
        out.push_back(kv("fbxImports", RenderIntegration::g_stats.fbx_imports));
        out.push_back(kv("fbxCacheHits", RenderIntegration::g_stats.fbx_cache_hits));
        out.push_back(kv("fbxCacheWrites", RenderIntegration::g_stats.fbx_cache_writes));
        out.push_back(kv("fbxCacheEvicts", RenderIntegration::g_stats.fbx_cache_evicts));
        // ParseMs against fbxImports is the number that decides whether the
        // parse is worth moving off the game thread at all - do not redesign
        // that path without it. THE FIRST TIMING THIS PATH HAS EVER HAD.
        // Counts have existed since the cache was built and nobody has ever
        // known what an import COSTS.
        out.push_back(kv("fbxReadMs", RenderIntegration::g_fbx_read_ms));
        out.push_back(kv("fbxLoadMs", RenderIntegration::g_fbx_load_ms));
        out.push_back(kv("fbxParseMs", RenderIntegration::g_fbx_parse_ms));
        out.push_back(kv("fbxRegMs", RenderIntegration::g_fbx_reg_ms));
        // KH_MESH_CACHE_MT. Queued and Done must converge; a permanent gap
        // means the writer thread is wedged. Dropped counts writes abandoned
        // at mission end and a small non-zero value is normal.
        out.push_back(kv("meshWriteQueued", RenderIntegration::g_khmw_queued));
        out.push_back(kv("meshWriteDone", RenderIntegration::g_khmw_done));
        out.push_back(kv("meshWriteDropped", RenderIntegration::g_khmw_dropped));
        out.push_back(kv("acqHoldFrames", RenderIntegration::g_acq_hold_frames));
        out.push_back(kv("acqHoldResets", RenderIntegration::g_acq_hold_resets));
        out.push_back(kv("acqHoldScans", RenderIntegration::g_acq_hold_scans));
        out.push_back(kv("registeredMeshes", static_cast<uint64_t>(RenderIntegration::mesh_count())));
        out.push_back(kv("compositeKeepEncodes", RenderIntegration::g_stats.composite_keep_encodes));
        out.push_back(kv("compositeAnomalySkips", RenderIntegration::g_stats.composite_anomaly_skips));
        out.push_back(kv("sunDepthPasses", RenderIntegration::g_stats.sun_depth_passes));
        out.push_back(kv("sunDepthCasters", RenderIntegration::g_stats.sun_depth_casters));
        out.push_back(kv("sunJumpFlushes", RenderIntegration::g_stats.sun_jump_flushes));
        out.push_back(kv("castArmsLost", RenderIntegration::g_mask.cast_arms_lost));

        {
            uint32_t bsv = 0;

            for (int b = 0; b < 8; ++b) {
                if (RenderIntegration::g_ls.band[b].valid) ++bsv;
            }

            out.push_back(kv("bandSlotsValid", bsv));
        }

        out.push_back(kvf("fireFovX", RenderIntegration::g_mask.last_fire_fov[0]));
        out.push_back(kvf("fireFovY", RenderIntegration::g_mask.last_fire_fov[1]));
        out.push_back(kvf("fireRotErr", RenderIntegration::g_mask.last_fire_rot_err));
        out.push_back(kv("encVpRejects", RenderIntegration::g_stats.enc_vp_rejects));
        out.push_back(kv("reorderHook", RenderIntegration::g_reorder_hook_active.load(std::memory_order_acquire) ? 1 : 0));
        out.push_back(kv("uiDriverPolls", RenderIntegration::g_ui_poll_attempts));
        out.push_back(kv("uiDriverCtrl", RenderIntegration::g_ui_ctrl_created ? 1 : 0));
        out.push_back(kv("mainSceneW", RenderIntegration::g_main_depth_w));
        out.push_back(kv("mainSceneH", RenderIntegration::g_main_depth_h));
        out.push_back(kv("shadowLiveLatches", RenderIntegration::g_stats.shadow_live_latches));
        out.push_back(kv("shadowLiveCascades", RenderIntegration::g_stats.shadow_live_cascades));
        out.push_back(kv("shadowSrvFailed", RenderIntegration::g_stats.shadow_srv_failed));
        out.push_back(kv("liveRejOrtho", RenderIntegration::g_stats.live_rej_ortho));
        out.push_back(kv("liveRejScale", RenderIntegration::g_stats.live_rej_scale));
        out.push_back(kv("liveRejIso", RenderIntegration::g_stats.live_rej_iso));
        out.push_back(kv("liveRejRatio", RenderIntegration::g_stats.live_rej_ratio));
        out.push_back(kv("liveRejTrans", RenderIntegration::g_stats.live_rej_trans));
        out.push_back(kv("liveAccepts", RenderIntegration::g_stats.live_accepts));
        // KH_LIVE_FIN_CENSUS: the finalize bail by reason + the last
        // successful commit stamp (effect time; subtract from effectTimeS for
        // the outage age). All three near zero with the table still empty =
        // finalizes are not RUNNING and the cycle-boundary detection is the
        // suspect instead.
        out.push_back(kv("liveFinBailPend", RenderIntegration::g_live_fin_bail_pend));
        out.push_back(kv("liveFinBailVp", RenderIntegration::g_live_fin_bail_vp));
        out.push_back(kv("liveFinBailAtlas", RenderIntegration::g_live_fin_bail_atlas));
        out.push_back(kvf("liveLastCommitT", RenderIntegration::g_live_last_commit_t));
        out.push_back(kv("livePendSets", RenderIntegration::g_live_pend_sets));
        out.push_back(kv("liveStashAccepts", RenderIntegration::g_live_stash_accepts));
        out.push_back(kv("livePendWipesNw", RenderIntegration::g_live_pend_wipes_nw));
        out.push_back(kv("livePendWipesMaint", RenderIntegration::g_live_pend_wipes_maint));
        out.push_back(kvf("liveCycleStartT", RenderIntegration::g_live_cycle_start_t));
        out.push_back(kvf("cascBindFeedT", RenderIntegration::g_cascbind_feed_t));
        out.push_back(kv("cascBindSpanLast", RenderIntegration::g_cascbind_span_last));
        out.push_back(kv("cascBindSpanMax", RenderIntegration::g_cascbind_span_max));
        // harvest near-miss census (stages: 0/1 scale hi/lo, 2 ratio, 3 iso,
        // 4 trans, 5 ortho). hvBestStage names how deep the best harvested
        // candidate got through the bar ladder during a drought.
        out.push_back(kv("hvWindows", RenderIntegration::g_hv_windows));
        out.push_back(kv("hvRejScale", RenderIntegration::g_hv_rej_scale));
        out.push_back(kv("hvRejRatio", RenderIntegration::g_hv_rej_ratio));
        out.push_back(kv("hvRejIso", RenderIntegration::g_hv_rej_iso));
        out.push_back(kv("hvRejTrans", RenderIntegration::g_hv_rej_trans));
        out.push_back(kv("hvRejOrtho", RenderIntegration::g_hv_rej_ortho));
        out.push_back(kv("hvBestStage", RenderIntegration::g_hv_best_stage));
        out.push_back(kvf("hvBestN0", RenderIntegration::g_hv_best_n0));
        out.push_back(kv("shadowAtlasSize", RenderIntegration::g_ls.atlas_size));
        out.push_back(kv("resolveHits", RenderIntegration::g_ls.resolve_hits));
        out.push_back(kv("topoDraws", RenderIntegration::g_topo_pub.draws));
        out.push_back(kv("topoSweeps", RenderIntegration::g_topo_pub.sweeps));
        out.push_back(kv("topoFirstSweep", RenderIntegration::g_topo_pub.d_first_sweep));
        out.push_back(kv("topoLastSweep", RenderIntegration::g_topo_pub.d_last_sweep));
        out.push_back(kv("topoInjects", RenderIntegration::g_topo_pub.injects));
        out.push_back(kv("topoInject", RenderIntegration::g_topo_pub.d_inject));
        out.push_back(kv("topoLastMainDsv", RenderIntegration::g_topo_pub.d_last_main_dsv));
        out.push_back(kv("topoLastSceneRt", RenderIntegration::g_topo_pub.d_last_scene_rt));
        out.push_back(kv("topoPpHead", RenderIntegration::g_topo_pub.d_pp_head));
        out.push_back(kv("topoPpW", RenderIntegration::g_topo_pub.pp_w));
        out.push_back(kv("topoPpFmt", RenderIntegration::g_topo_pub.pp_fmt));
        out.push_back(kv("topoSceneSrv", RenderIntegration::g_topo_pub.d_scene_srv));
        out.push_back(kv("topoCycles", RenderIntegration::g_topo_cycles));
        out.push_back(kv("topoLastColor", RenderIntegration::g_topo_pub.d_last_color));
        out.push_back(kv("topoForeignColor", RenderIntegration::g_topo_pub.d_last_foreign));
        out.push_back(kv("topoColorIds", RenderIntegration::g_topo_pub.color_ids));
        out.push_back(kv("topoAfterDraws", RenderIntegration::g_topo_pub_after.draws));
        out.push_back(kv("topoAfterMainDsv", RenderIntegration::g_topo_pub_after.d_last_main_dsv));
        out.push_back(kv("topoAfterSceneRt", RenderIntegration::g_topo_pub_after.d_last_scene_rt));
        out.push_back(kv("topoAfterColor", RenderIntegration::g_topo_pub_after.d_last_color));
        out.push_back(kv("topoAfterForeign", RenderIntegration::g_topo_pub_after.d_last_foreign));
        out.push_back(kv("topoAfterColorIds", RenderIntegration::g_topo_pub_after.color_ids));
        out.push_back(kv("topoAfterSweeps", RenderIntegration::g_topo_pub_after.sweeps));
        out.push_back(kv("topoSceneSrvSlot", RenderIntegration::g_topo_pub.srv_slot));
        out.push_back(kv("topoSceneCopy", RenderIntegration::g_topo_pub.d_scene_copy));
        out.push_back(kv("topoResolveFirst", RenderIntegration::g_topo_pub.d_resolve_first));
        out.push_back(kv("topoResolveLast", RenderIntegration::g_topo_pub.d_resolve_last));
        out.push_back(kv("sunHoldArmed", RenderIntegration::g_sun_valid_ms != 0 ? 1 : 0));
        out.push_back(kv("sunJumpRateRefused", RenderIntegration::g_sun_jump_rate_refused));
        out.push_back(kv("sunSettleHolds", RenderIntegration::g_sun_settle_holds));
        out.push_back(kv("sunGlideClamps", RenderIntegration::g_sun_glide_clamps));
        out.push_back(kv("sunSettled", RenderIntegration::kh_sun_settled() ? 1 : 0));
        out.push_back(kv("resolveDraws", RenderIntegration::g_ls.resolve_draws));
        out.push_back(kv("resolveCbFound", RenderIntegration::g_ls.resolve_cb_found));
        out.push_back(kv("bandCaptures", RenderIntegration::g_ls.band_captures));
        out.push_back(kvf("band0Near", RenderIntegration::g_ls.band[0].valid ? RenderIntegration::g_ls.band[0].border[0] : -1.0f));
        out.push_back(kvf("band0Far", RenderIntegration::g_ls.band[0].valid ? RenderIntegration::g_ls.band[0].border[1] : -1.0f));
        out.push_back(kv("band0Copies", RenderIntegration::g_ls.band[0].copies));
        out.push_back(kvf("band1Near", RenderIntegration::g_ls.band[1].valid ? RenderIntegration::g_ls.band[1].border[0] : -1.0f));
        out.push_back(kvf("band1Far", RenderIntegration::g_ls.band[1].valid ? RenderIntegration::g_ls.band[1].border[1] : -1.0f));
        out.push_back(kv("band1Copies", RenderIntegration::g_ls.band[1].copies));
        out.push_back(kv("compCompiles", RenderIntegration::g_comp_compiles));
        out.push_back(kv("compFailStreak", RenderIntegration::g_comp_fail_streak));
        out.push_back(kv("bandBailPv", RenderIntegration::g_ls.band_bail_pv));
        out.push_back(kv("bandBailOff", RenderIntegration::g_ls.band_bail_off));
        out.push_back(kv("bandBailBorder", RenderIntegration::g_ls.band_bail_border));
        out.push_back(kv("bandBailSlot", RenderIntegration::g_ls.band_bail_slot));
        out.push_back(kv("bandBailTime", RenderIntegration::g_ls.band_bail_time));
        out.push_back(kvf("bandRejB0", RenderIntegration::g_ls.band_last_reject[0]));
        out.push_back(kvf("bandRejB1", RenderIntegration::g_ls.band_last_reject[1]));
        out.push_back(kvf("bandRejB2", RenderIntegration::g_ls.band_last_reject[2]));
        out.push_back(kvf("bandRejB3", RenderIntegration::g_ls.band_last_reject[3]));
        out.push_back(kv("castMisses", RenderIntegration::g_ls.cast_misses));
        out.push_back(kv("resolveGated", RenderIntegration::g_ls.resolve_gated));
        out.push_back(kv("analyticCasts", RenderIntegration::g_mask.analytic_casts));
        out.push_back(kv("maskRtvSwaps", RenderIntegration::g_mask.mask_rtv_swaps));
        out.push_back(kv("castBatches", RenderIntegration::g_mask.cast_batches));
        out.push_back(kvf("coldFirstInject", RenderIntegration::g_mask.cold_first_inject));
        out.push_back(kvf("coldFirstCast", RenderIntegration::g_mask.cold_first_cast));
        out.push_back(kv("coldLeadAmbiguous", RenderIntegration::g_mask.cold_lead_ambiguous));
        out.push_back(kvf("coldFirstTrigger", RenderIntegration::g_mask.cold_first_trigger));
        out.push_back(kvf("coldFirstStaged", RenderIntegration::g_mask.cold_first_stage));
        out.push_back(kv("coldGNoDsv", RenderIntegration::g_mask.cold_g_nodsv));
        out.push_back(kv("coldGFloor", RenderIntegration::g_mask.cold_g_floor));
        out.push_back(kv("coldGTid", RenderIntegration::g_mask.cold_g_tid));
        out.push_back(kv("coldCastMiss", RenderIntegration::g_mask.cold_cast_miss));
        out.push_back(kv("coldPubRejects", RenderIntegration::g_ls.cold_pub_rejects));
        out.push_back(kv("fireSnapCopies", RenderIntegration::g_fire_snap_copies));
        out.push_back(kv("fireSnapFails", RenderIntegration::g_fire_snap_fails));
        out.push_back(kv("fireSnapLive", RenderIntegration::g_fire_snap_live ? 1 : 0));
        out.push_back(kvf("fireLiveViewDeltaM", RenderIntegration::g_fire_live_view_delta_m));
        out.push_back(kvf("fireLiveViewDeltaMax", RenderIntegration::g_fire_live_view_delta_max));
        out.push_back(kv("fireLiveViewMoved", RenderIntegration::g_fire_live_view_moved));
        out.push_back(kv("castRearmAtlasBlocks", RenderIntegration::g_cast_rearm_atlas_blocks));
        out.push_back(kv("castRearmStrandedClears", RenderIntegration::g_cast_rearm_stranded_clears));
        out.push_back(kv("fireEpochSkips", RenderIntegration::g_fire_epoch_skips));
        out.push_back(kv("fireEpochSkipsBridge", RenderIntegration::g_fire_epoch_skips_bridge));
        out.push_back(kvf("fireEpochLastM", RenderIntegration::g_fire_epoch_last_m));
        out.push_back(kvf("fireEpochLastRot", RenderIntegration::g_fire_epoch_last_rot));
        out.push_back(kv("blkDarkContraSticky", RenderIntegration::g_blk_dark_contra_sticky));
        // The ratio lanes that exist to decide this fix live in the other
        // branch and read zero because control never reaches them. If this
        // reads > 0.5 the pending IS ageing and the adoption chain is never
        // entered (khp_agree never true); if it stays pinned near 0 something
        // upstream is refreshing pend_t.
        out.push_back(kv("blkStickyRejects", RenderIntegration::g_blk_sticky_rejects));
        out.push_back(kvf("blkStickyPendAge", RenderIntegration::g_blk_sticky_pend_age));
        out.push_back(kvf("blkStickyPendAgeMax", RenderIntegration::g_blk_sticky_pend_age_max));
        out.push_back(kvf("blkStickyInSl", RenderIntegration::g_blk_sticky_in_sl));
        out.push_back(kvf("blkStickyPendSl", RenderIntegration::g_blk_sticky_pend_sl));
        out.push_back(kvf("blkStickyStdSl", RenderIntegration::g_blk_sticky_std_sl));
        out.push_back(kv("blkDarkForceAdopts", RenderIntegration::g_blk_dark_force_adopts));
        out.push_back(kvf("blkDarkForceHeldS", RenderIntegration::g_blk_dark_force_held_s));
        out.push_back(kv("blkDarkSeedBlocks", RenderIntegration::g_blk_dark_seed_blocks));
        out.push_back(kv("fireExtrapUsed", RenderIntegration::g_fire_extrap_used));
        out.push_back(kv("fireExtrapFails", RenderIntegration::g_fire_extrap_fails));
        out.push_back(kv("blkDarkReadopts", RenderIntegration::g_blk_dark_readopts));
        out.push_back(kv("sunBootPublishes", RenderIntegration::g_sun_boot_publishes));
        out.push_back(kv("sunBootActive", RenderIntegration::g_pub_boot ? 1 : 0));
        out.push_back(kvf("sunBootHandoverDeg", RenderIntegration::g_sun_boot_handover_deg));
        out.push_back(kvf("lastInjectNear", RenderIntegration::g_mask.last_inject_near));
        out.push_back(kv("ovListed", RenderIntegration::g_mask.ov_listed));
        out.push_back(kv("ovSkipped", RenderIntegration::g_mask.ov_skipped));
        out.push_back(kv("ovDrawn", RenderIntegration::g_mask.ov_drawn));
        out.push_back(kvf("fogStagedValue", RenderIntegration::g_fog_valid ? RenderIntegration::g_fog[0] : -1.0f));
        out.push_back(kvf("fogStagedDecay", RenderIntegration::g_fog_valid ? RenderIntegration::g_fog[1] : -1.0f));
        out.push_back(kvf("fogStagedBase", RenderIntegration::g_fog_valid ? RenderIntegration::g_fog[2] : -1.0f));
        out.push_back(kv("fxDepthPairHolds", RenderIntegration::g_khfx_pair_holds));
        out.push_back(kv("fxDepthPairJumpAdopts", RenderIntegration::g_khfx_pair_jump_adopts));
        out.push_back(kv("fxDepthPairBridgeAdopts", RenderIntegration::g_khfx_bridge_adopts));
        // confirmVetoes = second sightings REFUSED because the live bridge
        // contradicted the candidate (multi-flush foreign latch runs held to
        // zero error frames).
        out.push_back(kv("fxDepthPairConfirmVetoes", RenderIntegration::g_khfx_confirm_vetoes));
        out.push_back(kv("sunDirDerivedValid", RenderIntegration::g_sun_dir_derived_valid ? 1u : 0u));
        out.push_back(kvf("sunDirDerivedX", RenderIntegration::g_sun_dir_derived[0]));
        out.push_back(kvf("sunDirDerivedY", RenderIntegration::g_sun_dir_derived[1]));
        out.push_back(kvf("sunDirDerivedZ", RenderIntegration::g_sun_dir_derived[2]));
        out.push_back(kv("bandBailView", RenderIntegration::g_ls.band_bail_view));
        out.push_back(kv("bandBailQuality", RenderIntegration::g_ls.band_bail_quality));
        out.push_back(kv("bandProvSkips", RenderIntegration::g_ls.band_prov_skips));
        out.push_back(kv("viewLocks", RenderIntegration::g_ls.view_locks));
        out.push_back(kv("viewRelocks", RenderIntegration::g_ls.view_relocks));
        out.push_back(kv("debugMode", static_cast<uint64_t>(RenderIntegration::g_dbg_mode.load(std::memory_order_relaxed))));
        out.push_back(kv("perceptualCaptures", RenderIntegration::g_stats.perceptual_captures));
        out.push_back(kv("viewSrcValid", RenderIntegration::g_ls.view_src_valid ? 1u : 0u));
        out.push_back(kv("frameViewHits", RenderIntegration::g_ls.frame_view_hits));
        out.push_back(kv("viewAdopts", RenderIntegration::g_view_adopts));
        out.push_back(kv("viewAdoptStale", RenderIntegration::g_view_adopt_stale));
        out.push_back(kv("viewAdoptFamily", RenderIntegration::g_view_adopt_family));
        out.push_back(kvf("viewAdoptLastRot", RenderIntegration::g_view_adopt_last_rot));
        out.push_back(kvf("viewAdoptMaxRot", RenderIntegration::g_view_adopt_max_rot));
        out.push_back(kvf("viewBestRot", RenderIntegration::g_ls.view_best_rot));
        out.push_back(kvf("viewBestTrans", RenderIntegration::g_ls.view_best_trans));
        out.push_back(kv("sealCompletions", RenderIntegration::g_ls.seal_completions));
        out.push_back(kv("sunDirValid", RenderIntegration::g_sun_valid ? 1 : 0));
        out.push_back(kvf("sunDirEngineX", RenderIntegration::g_sun_dir_engine[0]));
        out.push_back(kvf("sunDirEngineY", RenderIntegration::g_sun_dir_engine[1]));
        out.push_back(kvf("sunDirEngineZ", RenderIntegration::g_sun_dir_engine[2]));
        {   // engine HDR sun magnitude (getLighting retired): peak of the
            // located block's sun lane; zero under moonlight by design
            const float* sl = RenderIntegration::g_light_probe.nb + 16;
            const float sm = sl[0] > sl[1] ? (sl[0] > sl[2] ? sl[0] : sl[2]) : (sl[1] > sl[2] ? sl[1] : sl[2]);
            out.push_back(kvf("sunBrightness", sm));
        }
        out.push_back(kv("lightLocValid", RenderIntegration::g_light_probe.valid ? 1u : 0u));
        out.push_back(kv("lightLocOff", RenderIntegration::g_light_probe.off));
        out.push_back(kv("lightLocFloats", RenderIntegration::g_light_probe.floats));
        out.push_back(kv("lightLocMeta", static_cast<uint64_t>(RenderIntegration::g_light_probe.meta)));
        out.push_back(kv("lightLocHits", RenderIntegration::g_light_probe.hits));
        out.push_back(kv("lightLocMisses", RenderIntegration::g_light_probe.misses));
        out.push_back(kv("lightLocRelocs", RenderIntegration::g_light_probe.relocs));
        out.push_back(kv("lightLocNbBase", RenderIntegration::g_light_probe.nb_base));
        out.push_back(kvf("lightLocErr", RenderIntegration::g_light_probe.last_err));
        out.push_back(kv("blockHolds", RenderIntegration::g_blk_holds));
        out.push_back(kv("blockRegimeRejects", RenderIntegration::g_light_probe.regime_rejects));
        out.push_back(kv("blockRegimeAdopts", RenderIntegration::g_light_probe.regime_adopts));
        out.push_back(kv("blockModeRejects", RenderIntegration::g_blk_mode_rejects));
        out.push_back(kv("blockErrRejects", RenderIntegration::g_blk_err_rejects));
        out.push_back(kv("blockJumpAdopts", RenderIntegration::g_blk_jump_adopts));
        out.push_back(kvf("lightLocAge", RenderIntegration::effect_time_seconds() - RenderIntegration::g_light_probe.last_confirm));
        out.push_back(kvf("lightLocMode", RenderIntegration::g_light_probe.last_mode));
        {   // BLACK-BOX (see the): the engine block AMBIENT color
            // nb[8..10] -> lightAmb.rgb, the term ndl<=0 faces collapse to -
            // distinct from the sun lane (sunBrightness reads nb[16..18]). -1
            // = block not locked this frame (matches fill_lighting_frame_cb).
            const bool amb_locked = RenderIntegration::g_light_probe.hits > 0 &&
                                    RenderIntegration::g_light_probe.meta == 40;
            const float* amb = RenderIntegration::g_light_probe.nb + 8;
            out.push_back(kvf("ambHDR_R", amb_locked ? amb[0] : -1.0f));
            out.push_back(kvf("ambHDR_G", amb_locked ? amb[1] : -1.0f));
            out.push_back(kvf("ambHDR_B", amb_locked ? amb[2] : -1.0f));
        }
        {   // BLACK-BOX: what the last LIT mesh each path actually received.
            // AmbTermMax = max(amb)*amb_scalar = the ndl<=0 face brightness
            // before the base-multiply; ~0 with GuardBase 1e9 convicts the
            // ambient-zero + guard-stood-down root. Path valid=0 => that path
            // drew no lit mesh since the arm (its lanes read the -1
            // sentinel).
            auto termmax = [](const float* a, float s) -> float {
                if (s < 0.0f || a[0] < 0.0f) return -1.0f;
                float m = a[0] > a[1] ? (a[0] > a[2] ? a[0] : a[2])
                                      : (a[1] > a[2] ? a[1] : a[2]);
                return m * s;
            };
            const RenderIntegration::RenderStats& S = RenderIntegration::g_stats;
            out.push_back(kv ("injBoxValid",       static_cast<uint64_t>(S.inj_box_valid)));
            out.push_back(kv ("injBoxEffect",      static_cast<uint64_t>(S.inj_box_effect < 0 ? 0 : S.inj_box_effect)));
            out.push_back(kvf("injBoxAmbR",        S.inj_box_amb[0]));
            out.push_back(kvf("injBoxAmbG",        S.inj_box_amb[1]));
            out.push_back(kvf("injBoxAmbB",        S.inj_box_amb[2]));
            out.push_back(kvf("injBoxAmbScalar",   S.inj_box_amb_scalar));
            out.push_back(kvf("injBoxAmbTermMax",  termmax(S.inj_box_amb, S.inj_box_amb_scalar)));
            out.push_back(kvf("injBoxGuardBase",   S.inj_box_guard_base));
            out.push_back(kv ("flushBoxValid",     static_cast<uint64_t>(S.flush_box_valid)));
            out.push_back(kv ("flushBoxEffect",    static_cast<uint64_t>(S.flush_box_effect < 0 ? 0 : S.flush_box_effect)));
            out.push_back(kvf("flushBoxAmbR",       S.flush_box_amb[0]));
            out.push_back(kvf("flushBoxAmbG",       S.flush_box_amb[1]));
            out.push_back(kvf("flushBoxAmbB",       S.flush_box_amb[2]));
            out.push_back(kvf("flushBoxAmbScalar",  S.flush_box_amb_scalar));
            out.push_back(kvf("flushBoxAmbTermMax", termmax(S.flush_box_amb, S.flush_box_amb_scalar)));
            out.push_back(kvf("flushBoxGuardBase",  S.flush_box_guard_base));
            out.push_back(kvf("injBoxSunMax",       termmax(S.inj_box_sun, 1.0f)));
            out.push_back(kvf("flushBoxSunMax",     termmax(S.flush_box_sun, 1.0f)));
        }
        {   // BLACK-BOX phase 1: the block colors snapshotted under the park in
            const float* bs = RenderIntegration::g_pub_block_sun;
            const bool bv = RenderIntegration::g_pub_block_valid;
            float bsm = bs[0] > bs[1] ? (bs[0] > bs[2] ? bs[0] : bs[2])
                                      : (bs[1] > bs[2] ? bs[1] : bs[2]);
            out.push_back(kv ("blockStagedValid",  bv ? 1ull : 0ull));
            out.push_back(kvf("blockStagedAmbR",   RenderIntegration::g_pub_block_amb[0]));
            out.push_back(kvf("blockStagedAmbG",   RenderIntegration::g_pub_block_amb[1]));
            out.push_back(kvf("blockStagedAmbB",   RenderIntegration::g_pub_block_amb[2]));
            out.push_back(kvf("blockStagedSunMax", bv ? bsm : -1.0f));
            out.push_back(kvf("blockStagedFogR",   RenderIntegration::g_pub_block_fog[0]));
            out.push_back(kvf("blockStagedFogG",   RenderIntegration::g_pub_block_fog[1]));
            out.push_back(kvf("blockStagedFogB",   RenderIntegration::g_pub_block_fog[2]));
        }

        out.push_back(kv("skyLocValid", RenderIntegration::g_sky_probe.valid ? 1u : 0u));
        out.push_back(kv("skyLocFloats", RenderIntegration::g_sky_probe.floats));
        out.push_back(kv("skyLocHits", RenderIntegration::g_sky_probe.hits));
        out.push_back(kv("skyLocMisses", RenderIntegration::g_sky_probe.misses));
        out.push_back(kvf("skyLocAge", RenderIntegration::effect_time_seconds() - RenderIntegration::g_sky_probe.last_confirm));
        // CLAIM UNDER TEST, stated so the dump can kill it: (A) g_sky_probe's
        // buffer IS the engine's PSCB_NonFrequent, bound at PS b0 - the
        // buffer whose row 14 the terrain and object shaders read as
        // PSC_HazePars, the DISTANCE HAZE that tints world geometry blue and
        // that our meshes do not receive.
        out.push_back(kv("atmSkyNbBase", RenderIntegration::g_sky_probe.nb_base));
        out.push_back(kvf("encNearSlot", RenderIntegration::g_ro.slot_near_live));
        // the TRUE near (-m32/m22), which is what the gate now compares.
        out.push_back(kvf("encNearMeasured", RenderIntegration::g_ro.engine_proj_valid
            ? RenderIntegration::kh_enc_pair_near(RenderIntegration::g_ro.engine_m22,
                                                  RenderIntegration::g_ro.engine_m32) : -1.0f));
        out.push_back(kvf("encFarMeasured", RenderIntegration::g_ro.engine_proj_valid
            ? RenderIntegration::kh_enc_far(RenderIntegration::g_ro.engine_m22,
                                            RenderIntegration::g_ro.engine_m32) : -1.0f));
        out.push_back(kvf("encFarSlot", RenderIntegration::kh_enc_far(
            RenderIntegration::g_ro.slot_m22, RenderIntegration::g_ro.slot_m32)));
        out.push_back(kv("encNearClassRejects", RenderIntegration::g_enc_near_class_rejects));
        out.push_back(kv("encBridgeSwaps", RenderIntegration::g_enc_bridge_swaps));
        out.push_back(kvf("encNearRefM", RenderIntegration::g_enc_near_ref_m));
        out.push_back(kvf("encNearRejM", RenderIntegration::g_enc_near_rej_m));
        out.push_back(kv("encSwapSrc", static_cast<uint64_t>(RenderIntegration::g_enc_swap_src)));
        out.push_back(kv("encSwapSrcBits", static_cast<uint64_t>(RenderIntegration::g_enc_swap_src_bits)));
        out.push_back(kvf("encSwapFromM", RenderIntegration::g_enc_swap_from_m));
        out.push_back(kvf("encSwapToM", RenderIntegration::g_enc_swap_to_m));
        out.push_back(kv("encEndorseRejects", RenderIntegration::g_enc_endorse_rejects));
        // encNearRatioMax near 10 means the gate was close to the fault;
        // stuck at 2-3 means it never came near firing and a remaining clip
        // is not something this gate can see. The ratio between the shipped
        // near and the census reference, every frame, and the session peak.
        // KH_ENC_NEAR_CLASS refuses above 10.
        out.push_back(kvf("encNearRatio", RenderIntegration::g_enc_near_ratio_last));
        out.push_back(kvf("encNearRatioMax", RenderIntegration::g_enc_near_ratio_max));
        out.push_back(kv("fogRampStands", RenderIntegration::g_fog_ramp_stands));
        out.push_back(kv("fogRampHolds", RenderIntegration::g_fog_ramp_holds));
        out.push_back(kvf("fogRampEndMin", RenderIntegration::g_fog_ramp_end_min));
        out.push_back(kvf("fogRampEndMax", RenderIntegration::g_fog_ramp_end_max));
        out.push_back(kvf("fogRampMinInv", RenderIntegration::g_fog_ramp_min_inv));
        out.push_back(kvf("fogRampMinProd", RenderIntegration::g_fog_ramp_min_prod));
        out.push_back(kvf("fogRampMinCamY", RenderIntegration::g_fog_ramp_min_camy));
        out.push_back(kvf("fogRampMinMode", RenderIntegration::g_fog_ramp_min_mode));
        out.push_back(kv("fogRampIncoh", RenderIntegration::g_fog_ramp_incoh));
        // cb5[0].w, the engine's floor on the per-pixel shading scalar it
        // multiplies its fog TARGET by. Our target is unscaled, which is why
        // our mesh settles brighter than the fog around it. fogLightScales
        // counts the mode-302 probe applying the floor.
        out.push_back(kvf("fogLightFloor", RenderIntegration::g_fog_light_floor));
        out.push_back(kv("fogLightScales", RenderIntegration::g_fog_light_scales));
        out.push_back(kv("atmosStandDowns", RenderIntegration::g_atmos_stand_downs));
        out.push_back(kv("dcopyPre", RenderIntegration::g_dcopy_pre_inject));
        out.push_back(kv("dcopyPost", RenderIntegration::g_dcopy_post_inject));
        out.push_back(kv("dcopyDrawAt", RenderIntegration::g_dcopy_draw_at));
        out.push_back(kv("dcopyInjectAt", RenderIntegration::g_dcopy_inject_at));
        out.push_back(kv("trigDelaySkips", RenderIntegration::g_trig_delay_skips));
        out.push_back(kv("trigQualMax", RenderIntegration::g_trig_qual_max));
        // A multi-second fogEndHoldMaxS across a climb is OUR mirror
        // freezing; sub-second holds mean the engine is transitioning and we
        // are tracking it faithfully.
        out.push_back(kvf("fogEndHoldMaxS", RenderIntegration::g_fog_end_hold_max));
        // KH_FOG_LANE_REFRESH: fog lanes patched into the mirror past a
        // LUMINANCE reject. blkFogModeStands counts uploads whose fog-model
        // selector was not 1 or 3, i.e. a variant block correctly refused.
        out.push_back(kv("blkFogRefreshes", RenderIntegration::g_blk_fog_refreshes));
        out.push_back(kv("blkFogModeStands", RenderIntegration::g_blk_fog_mode_stands));
        out.push_back(kv("fogBelowStands", RenderIntegration::g_fog_below_stands));
        out.push_back(kvf("fogBelowCamY", RenderIntegration::g_fog_below_cam_y));
        // fogBelowClampY == hazeLayerY on every below frame is the arming
        // proof; if it still equals fogBelowCamY the clamp never ran.
        // KH_FOG_BELOW_CLAMP: the altitude actually SHIPPED while below the
        // layer.
        out.push_back(kvf("fogBelowClampY", RenderIntegration::g_fog_below_clamp_y));
        // ARMING LANE - if this reads -1 while fogBelowStands counts, the
        // branch never got its constant and any verdict on underwater fog is
        // unread. KH_FOG_BELOW_BRANCH: the engine below-layer extinction the
        // branch actually shipped (block nb[51]). Expect ~0.07.
        out.push_back(kvf("fogBelowExt", RenderIntegration::g_fog_below_ext));
        // ARMING LANE - all -1 means the block never filled and the white box
        // was not tested. KH_FOG_UW_TARGET: the below-layer convergence
        // colour actually shipped (cb0[7]). Compare against fogTgt*, which is
        // the ABOVE-layer target: they must DIFFER, and by a lot.
        out.push_back(kvf("fogUwColR", RenderIntegration::g_fog_uw_col[0]));
        out.push_back(kvf("fogUwColG", RenderIntegration::g_fog_uw_col[1]));
        out.push_back(kvf("fogUwColB", RenderIntegration::g_fog_uw_col[2]));
        out.push_back(kvf("atmBlkR12w2", RenderIntegration::g_light_probe.nb[51]));
        out.push_back(kv("fogZeroHolds", RenderIntegration::g_fog_zero_holds));
        out.push_back(kv("fogDecaySubs", RenderIntegration::g_fog_decay_subs));
        out.push_back(kvf("fogDecayShipped", RenderIntegration::g_fog_decay_pub));
        out.push_back(kvf("fogDecayEngine", RenderIntegration::g_light_probe.nb[40]));
        out.push_back(kv("hazeArms", RenderIntegration::g_haze_arms));
        out.push_back(kv("hazeModeStands", RenderIntegration::g_haze_mode_stands));
        out.push_back(kv("hazeSaneRejects", RenderIntegration::g_haze_sane_rejects));
        out.push_back(kvf("hazeRefAltM", RenderIntegration::g_haze_ref_pub));
        out.push_back(kvf("hazeDensity", RenderIntegration::g_haze_den_pub));
        out.push_back(kvf("hazeFalloff", RenderIntegration::g_haze_fal_pub));
        out.push_back(kvf("hazeLayerY", RenderIntegration::g_haze_layer_pub));
        {   // the engine fog-mode selector as the shader switch reads it:
            // raw BITS, not a float. Measured 1 (layered fog + haze).
            uint32_t khz_m = 0;
            memcpy(&khz_m, &RenderIntegration::g_light_probe.nb[44], sizeof(khz_m));
            out.push_back(kv("hazeFogModeBits", static_cast<uint64_t>(khz_m)));
        }
        // ZERO means the instrument never ran and every other lane here is
        // meaningless - the probe only fires on depth-only binds whose target
        // CHANGED, while shadow_live_wanted. The instrument for the
        // character-shadow task: every depth texture that reaches
        // shadow_live_consider_atlas, its shape, and the clause that refused
        // it.
        out.push_back(kv("shadowTexConsiders", RenderIntegration::g_shadow_tex_considers));
        out.push_back(kv("shadowTexArrayRejects", RenderIntegration::g_shadow_tex_array_rejects));
        out.push_back(kv("shadowTexArrayMaxW",
            static_cast<uint64_t>(RenderIntegration::g_shadow_tex_array_max_w)));
        out.push_back(kv("shadowTexArrayMaxSlices",
            static_cast<uint64_t>(RenderIntegration::g_shadow_tex_array_max_slices)));
        out.push_back(kv("shadowTexAdopts", RenderIntegration::g_shadow_tex_adopts));
        out.push_back(kv("shadowTexKept", RenderIntegration::g_shadow_tex_kept));
        out.push_back(kv("shadowTexSame", RenderIntegration::g_shadow_tex_same));
        out.push_back(kv("shadowTexNRows",
            static_cast<uint64_t>(RenderIntegration::g_shadow_tex_census_n)));
        out.push_back(kv("shadowTexEvicts",
            static_cast<uint64_t>(RenderIntegration::g_shadow_tex_census_evicts)));
        {
            // Literal name table, same reason as the projection census: the
            // lane names are a dump contract and <cstdio> is not guaranteed
            // in scope here.
            static const char* const khsx_names[12][8] = {
                { "shadowTex0W", "shadowTex0H", "shadowTex0Arr", "shadowTex0Fmt", "shadowTex0Samp", "shadowTex0Clause", "shadowTex0Ids", "shadowTex0Hits" },
                { "shadowTex1W", "shadowTex1H", "shadowTex1Arr", "shadowTex1Fmt", "shadowTex1Samp", "shadowTex1Clause", "shadowTex1Ids", "shadowTex1Hits" },
                { "shadowTex2W", "shadowTex2H", "shadowTex2Arr", "shadowTex2Fmt", "shadowTex2Samp", "shadowTex2Clause", "shadowTex2Ids", "shadowTex2Hits" },
                { "shadowTex3W", "shadowTex3H", "shadowTex3Arr", "shadowTex3Fmt", "shadowTex3Samp", "shadowTex3Clause", "shadowTex3Ids", "shadowTex3Hits" },
                { "shadowTex4W", "shadowTex4H", "shadowTex4Arr", "shadowTex4Fmt", "shadowTex4Samp", "shadowTex4Clause", "shadowTex4Ids", "shadowTex4Hits" },
                { "shadowTex5W", "shadowTex5H", "shadowTex5Arr", "shadowTex5Fmt", "shadowTex5Samp", "shadowTex5Clause", "shadowTex5Ids", "shadowTex5Hits" },
                { "shadowTex6W", "shadowTex6H", "shadowTex6Arr", "shadowTex6Fmt", "shadowTex6Samp", "shadowTex6Clause", "shadowTex6Ids", "shadowTex6Hits" },
                { "shadowTex7W", "shadowTex7H", "shadowTex7Arr", "shadowTex7Fmt", "shadowTex7Samp", "shadowTex7Clause", "shadowTex7Ids", "shadowTex7Hits" },
                { "shadowTex8W", "shadowTex8H", "shadowTex8Arr", "shadowTex8Fmt", "shadowTex8Samp", "shadowTex8Clause", "shadowTex8Ids", "shadowTex8Hits" },
                { "shadowTex9W", "shadowTex9H", "shadowTex9Arr", "shadowTex9Fmt", "shadowTex9Samp", "shadowTex9Clause", "shadowTex9Ids", "shadowTex9Hits" },
                { "shadowTex10W", "shadowTex10H", "shadowTex10Arr", "shadowTex10Fmt", "shadowTex10Samp", "shadowTex10Clause", "shadowTex10Ids", "shadowTex10Hits" },
                { "shadowTex11W", "shadowTex11H", "shadowTex11Arr", "shadowTex11Fmt", "shadowTex11Samp", "shadowTex11Clause", "shadowTex11Ids", "shadowTex11Hits" },
            };

            for (int khsx_i = 0;
                 khsx_i < RenderIntegration::KH_SHADOW_TEX_CENSUS_N && khsx_i < 12;
                 ++khsx_i) {
                const RenderIntegration::KhShadowTexObs& khsx_e =
                    RenderIntegration::g_shadow_tex_census[khsx_i];
                if (khsx_e.sightings == 0) continue;   // unfilled rows stay out of the dump
                out.push_back(kv(khsx_names[khsx_i][0], static_cast<uint64_t>(khsx_e.width)));
                out.push_back(kv(khsx_names[khsx_i][1], static_cast<uint64_t>(khsx_e.height)));
                out.push_back(kv(khsx_names[khsx_i][2], static_cast<uint64_t>(khsx_e.array_size)));
                out.push_back(kv(khsx_names[khsx_i][3], static_cast<uint64_t>(khsx_e.format)));
                out.push_back(kv(khsx_names[khsx_i][4], static_cast<uint64_t>(khsx_e.samples)));
                out.push_back(kv(khsx_names[khsx_i][5], static_cast<uint64_t>(khsx_e.clause)));
                out.push_back(kv(khsx_names[khsx_i][6], static_cast<uint64_t>(khsx_e.identities)));
                out.push_back(kv(khsx_names[khsx_i][7], khsx_e.sightings));
            }
        }
        // An array is invisible to a DSV census if it is never bound
        // depth-only, so this one is keyed on how a resource is READ. ZERO
        // means the instrument never ran - it is behind the stats arm and the
        // render-thread test - and nothing else here means anything. The
        // separator for the two surviving character-shadow hypotheses.
        out.push_back(kv("srvArrScans", RenderIntegration::g_srv_arr_scans));
        out.push_back(kv("srvArrHits", RenderIntegration::g_srv_arr_hits));
        out.push_back(kv("srvArrDepthHits", RenderIntegration::g_srv_arr_depth_hits));
        out.push_back(kv("srvArrDepthMaxW",
            static_cast<uint64_t>(RenderIntegration::g_srv_arr_depth_max_w)));
        out.push_back(kv("srvArrDepthMaxSlices",
            static_cast<uint64_t>(RenderIntegration::g_srv_arr_depth_max_slices)));
        out.push_back(kv("srvArrNRows",
            static_cast<uint64_t>(RenderIntegration::g_srv_arr_census_n)));
        out.push_back(kv("srvArrEvicts",
            static_cast<uint64_t>(RenderIntegration::g_srv_arr_evicts)));
        out.push_back(kv("srvArrBudgetStops", RenderIntegration::g_srv_arr_budget_stops));
        {
            static const char* const khsa_names[8][8] = {
                { "srvArr0W", "srvArr0H", "srvArr0Arr", "srvArr0Fmt", "srvArr0ResFmt", "srvArr0Dim", "srvArr0Slots", "srvArr0Hits" },
                { "srvArr1W", "srvArr1H", "srvArr1Arr", "srvArr1Fmt", "srvArr1ResFmt", "srvArr1Dim", "srvArr1Slots", "srvArr1Hits" },
                { "srvArr2W", "srvArr2H", "srvArr2Arr", "srvArr2Fmt", "srvArr2ResFmt", "srvArr2Dim", "srvArr2Slots", "srvArr2Hits" },
                { "srvArr3W", "srvArr3H", "srvArr3Arr", "srvArr3Fmt", "srvArr3ResFmt", "srvArr3Dim", "srvArr3Slots", "srvArr3Hits" },
                { "srvArr4W", "srvArr4H", "srvArr4Arr", "srvArr4Fmt", "srvArr4ResFmt", "srvArr4Dim", "srvArr4Slots", "srvArr4Hits" },
                { "srvArr5W", "srvArr5H", "srvArr5Arr", "srvArr5Fmt", "srvArr5ResFmt", "srvArr5Dim", "srvArr5Slots", "srvArr5Hits" },
                { "srvArr6W", "srvArr6H", "srvArr6Arr", "srvArr6Fmt", "srvArr6ResFmt", "srvArr6Dim", "srvArr6Slots", "srvArr6Hits" },
                { "srvArr7W", "srvArr7H", "srvArr7Arr", "srvArr7Fmt", "srvArr7ResFmt", "srvArr7Dim", "srvArr7Slots", "srvArr7Hits" },
            };

            for (int khsa_i = 0;
                 khsa_i < RenderIntegration::KH_SRV_ARR_CENSUS_N && khsa_i < 8;
                 ++khsa_i) {
                const RenderIntegration::KhSrvArrObs& khsa_e =
                    RenderIntegration::g_srv_arr_census[khsa_i];
                if (khsa_e.sightings == 0) continue;
                out.push_back(kv(khsa_names[khsa_i][0], static_cast<uint64_t>(khsa_e.width)));
                out.push_back(kv(khsa_names[khsa_i][1], static_cast<uint64_t>(khsa_e.height)));
                out.push_back(kv(khsa_names[khsa_i][2], static_cast<uint64_t>(khsa_e.array_size)));
                out.push_back(kv(khsa_names[khsa_i][3], static_cast<uint64_t>(khsa_e.format)));
                out.push_back(kv(khsa_names[khsa_i][4], static_cast<uint64_t>(khsa_e.res_format)));
                out.push_back(kv(khsa_names[khsa_i][5], static_cast<uint64_t>(khsa_e.view_dim)));
                out.push_back(kv(khsa_names[khsa_i][6], static_cast<uint64_t>(khsa_e.slots)));
                out.push_back(kv(khsa_names[khsa_i][7], khsa_e.sightings));
            }
        }
        // READ svSeams FIRST: zero means the prepass -> volume transition was
        // never detected and every other lane here is meaningless. Non-zero
        // with svSeamW/H at scene resolution means the seam is the scene
        // depth prepass, which is the one we want.
        out.push_back(kv("svSeams", RenderIntegration::g_svs_seams));
        out.push_back(kv("svSeamW", static_cast<uint64_t>(RenderIntegration::g_svs_seam_w)));
        out.push_back(kv("svSeamH", static_cast<uint64_t>(RenderIntegration::g_svs_seam_h)));
        out.push_back(kv("svSeamFmt", static_cast<uint64_t>(RenderIntegration::g_svs_seam_fmt)));
        out.push_back(kv("svSeamSquare", RenderIntegration::g_svs_seam_square));
        out.push_back(kv("svSeamMain", RenderIntegration::g_svs_seam_main));
        out.push_back(kv("svArmSkips", RenderIntegration::g_svs_arm_skips));
        out.push_back(kv("svInjects", RenderIntegration::g_svs_injects));
        out.push_back(kv("svInjectDraws", RenderIntegration::g_svs_inject_draws));
        out.push_back(kv("svFrameDupes", RenderIntegration::g_svs_frame_dupes));
        out.push_back(kv("svSkipProj", RenderIntegration::g_svs_skip_proj));
        out.push_back(kv("svSkipEmpty", RenderIntegration::g_svs_skip_empty));
        out.push_back(kv("svSkipRes", RenderIntegration::g_svs_skip_res));
        // svSkipDims is the replacement guard: seam target not at scene
        // resolution. svMainW/H/Fmt is the shape it classified as main;
        // compare with svSeamW/H/Fmt to see whether they are the same buffer
        // (they should be).
        out.push_back(kv("svMainW", static_cast<uint64_t>(RenderIntegration::g_svs_main_w)));
        out.push_back(kv("svMainH", static_cast<uint64_t>(RenderIntegration::g_svs_main_h)));
        out.push_back(kv("svMainFmt", static_cast<uint64_t>(RenderIntegration::g_svs_main_fmt)));
        out.push_back(kv("svSkipDims", RenderIntegration::g_svs_skip_dims));
        out.push_back(kv("svSeamOrd", static_cast<uint64_t>(RenderIntegration::g_svs_seam_ord)));
        // That is not a matrix error and ten builds of matrix work could
        // never have touched it. THE ORDINAL. A capture finally asked where
        // our injection sits among the engine's counting draws, and the
        // answer was: 13123, in a run of 760 spanning EID 47..13809.
        out.push_back(kv("svCountFrame", static_cast<uint64_t>(RenderIntegration::g_svs_count_frame)));
        out.push_back(kv("svCountFrameMax", static_cast<uint64_t>(RenderIntegration::g_svs_count_frame_max)));
        out.push_back(kv("svInjCountOrd", static_cast<uint64_t>(RenderIntegration::g_svs_inj_count_ord)));
        out.push_back(kv("svInjCountOrdMax", static_cast<uint64_t>(RenderIntegration::g_svs_inj_count_ord_max)));
        out.push_back(kv("svSeamFirstOrd", static_cast<uint64_t>(RenderIntegration::g_svs_seam_first_ord)));
        out.push_back(kv("svBpvInjTakes", RenderIntegration::g_svs_bpv_inj_takes));
        out.push_back(kv("svBpvCompTakes", RenderIntegration::g_svs_bpv_comp_takes));
        out.push_back(kv("svBpvMisses", RenderIntegration::g_svs_bpv_misses));
        out.push_back(kv("svBpvProjTakes", RenderIntegration::g_svs_bpv_proj_takes));
        out.push_back(kv("svBpvProjRejects", RenderIntegration::g_svs_bpv_proj_rejects));
        out.push_back(kvf("svBpvProjRejNear", RenderIntegration::g_svs_bpv_proj_rej_near));
        out.push_back(kv("svInjPubOrd", RenderIntegration::g_svs_inj_pub_ord));
        out.push_back(kv("svCompPubOrd", RenderIntegration::g_svs_comp_pub_ord));
        out.push_back(kv("svPubOrdDelta", RenderIntegration::g_svs_pub_ord_delta));
        out.push_back(kv("svPubOrdMax", RenderIntegration::g_svs_pub_ord_max));
        out.push_back(kv("svPubOrdPairs", RenderIntegration::g_svs_pub_ord_pairs));
        out.push_back(kvf("svPubOrdMean", RenderIntegration::g_svs_pub_ord_pairs
            ? static_cast<float>(RenderIntegration::g_svs_pub_ord_sum /
                static_cast<double>(RenderIntegration::g_svs_pub_ord_pairs)) : -1.0f));
        out.push_back(kv("svSeamsFrameMax", static_cast<uint64_t>(RenderIntegration::g_svs_seams_frame_max)));
        // the shape of the buffer we ACTUALLY wrote into. svSeamW/H/Fmt is
        // the LAST seam of the frame and is a different buffer; these are
        // stamped at the write itself. svInjFmt 45 (D24_UNORM_S8_UINT) is the
        // stencil-volume buffer - the target. 44 (R24G8_TYPELESS) would be
        // the colour pass's depth, i.e. the wrong one.
        out.push_back(kv("svInjW", static_cast<uint64_t>(RenderIntegration::g_svs_inj_w)));
        out.push_back(kv("svInjH", static_cast<uint64_t>(RenderIntegration::g_svs_inj_h)));
        out.push_back(kv("svInjFmt", static_cast<uint64_t>(RenderIntegration::g_svs_inj_fmt)));
        out.push_back(kv("svInjBind", static_cast<uint64_t>(RenderIntegration::g_svs_inj_bind)));
        out.push_back(kv("svSkipSrv", RenderIntegration::g_svs_skip_srv));
        // SELECTED vs WRITTEN. svSel* is the target CHOSEN at the seam;
        // svInj* is now stamped only after a draw actually landed. Both were
        // silent before.
        out.push_back(kv("svSelW", static_cast<uint64_t>(RenderIntegration::g_svs_sel_w)));
        out.push_back(kv("svSelH", static_cast<uint64_t>(RenderIntegration::g_svs_sel_h)));
        out.push_back(kv("svSelFmt", static_cast<uint64_t>(RenderIntegration::g_svs_sel_fmt)));
        out.push_back(kv("svSelBind", static_cast<uint64_t>(RenderIntegration::g_svs_sel_bind)));
        out.push_back(kv("svEnters", RenderIntegration::g_svs_enters));
        out.push_back(kv("svFailFrameCb", RenderIntegration::g_svs_fail_framecb));
        out.push_back(kv("svFailObjCb", RenderIntegration::g_svs_fail_objcb));
        // svSeams minus svSeamAdj is what the order-free detector recovered;
        // if they are equal, adjacency was never the problem and the miss is
        // elsewhere.
        out.push_back(kv("svSeamAdj", RenderIntegration::g_svs_seam_adj));
        // If svBindVolOnly climbs while svBindVolRtv stays at or near zero,
        // the prepass half never reaches our hook - deferred context or
        // another thread - and no seam rule can recover it. DEPTH-BIND CENSUS
        // - what the OM hook actually SEES, before any rule runs. "vol" =
        // depth target with NO shader-resource bind, i.e.
        out.push_back(kv("svBindVolRtv", RenderIntegration::g_svs_bind_vol_rtv));
        out.push_back(kv("svBindVolOnly", RenderIntegration::g_svs_bind_vol_only));
        out.push_back(kv("svBindSrvRtv", RenderIntegration::g_svs_bind_srv_rtv));
        out.push_back(kv("svBindSrvOnly", RenderIntegration::g_svs_bind_srv_only));
        out.push_back(kv("svRtvSetMax", static_cast<uint64_t>(RenderIntegration::g_svs_rtv_set_max)));
        // DEFERRED INJECTION. svPendArms is how often the seam armed one;
        // svPendFires how often a later draw performed it. They should track
        // each other closely. svPendDropped counts arms still outstanding at
        // the frame boundary - i.e. no draw followed the seam, which would
        // mean the deferral point is wrong.
        out.push_back(kv("svPendArms", RenderIntegration::g_svs_pend_arms));
        out.push_back(kv("svPendFires", RenderIntegration::g_svs_pend_fires));
        out.push_back(kv("svPendDropped", RenderIntegration::g_svs_pend_dropped));
        // svMaskAdopts 0 means the resolve was never recognised and the paint
        // will show nothing; svMaskFails means SRV creation was refused.
        // ENGINE SHADOW MASK.
        out.push_back(kv("svMaskHits", RenderIntegration::g_svs_mask_hits));
        out.push_back(kv("svMaskAdopts", RenderIntegration::g_svs_mask_adopts));
        out.push_back(kv("svMaskFails", RenderIntegration::g_svs_mask_fails));
        out.push_back(kv("svMaskBinds", RenderIntegration::g_svs_mask_binds));
        out.push_back(kv("svMaskW", static_cast<uint64_t>(RenderIntegration::g_svs_mask_w)));
        out.push_back(kv("svMaskH", static_cast<uint64_t>(RenderIntegration::g_svs_mask_h)));
        out.push_back(kv("svMaskFmt", static_cast<uint64_t>(RenderIntegration::g_svs_mask_fmt)));
        out.push_back(kv("svOrdRtvFirst", RenderIntegration::g_svs_ord_rtv_first));
        out.push_back(kv("svOrdOnlyFirst", RenderIntegration::g_svs_ord_only_first));
        // svPreBinds is the shader-side liveness - if it stays 0 while
        // svCopyMade climbs, the snapshot is being taken and never read, and
        // the paint will show nothing. PRE-RESOLVE SNAPSHOT. Read in this
        // order.
        out.push_back(kv("svCopyFrameMax", static_cast<uint64_t>(RenderIntegration::g_svs_copy_frame_max)));
        out.push_back(kv("svCopyArms", RenderIntegration::g_svs_copy_arms));
        out.push_back(kv("svCopyMade", RenderIntegration::g_svs_copy_made));
        out.push_back(kv("svCopyDrops", RenderIntegration::g_svs_copy_drops));
        out.push_back(kv("svCopySkips", RenderIntegration::g_svs_copy_skips));
        out.push_back(kv("svCopyFails", RenderIntegration::g_svs_copy_fails));
        out.push_back(kv("svPreW", static_cast<uint64_t>(RenderIntegration::g_svs_pre_w)));
        out.push_back(kv("svPreH", static_cast<uint64_t>(RenderIntegration::g_svs_pre_h)));
        out.push_back(kv("svPreFmt", static_cast<uint64_t>(RenderIntegration::g_svs_pre_fmt)));
        out.push_back(kv("svPreBinds", RenderIntegration::g_svs_pre_binds));
        out.push_back(kv("svRtvSetWipes", RenderIntegration::g_svs_rtv_set_wipes));
        // svPostMade must track svCopyMade one for one: the pre half fires at
        // the resolve draw, the post half at the draw after it, so a
        // shortfall means no draw followed the resolve and the second half
        // never landed.
        out.push_back(kv("svPostMade", RenderIntegration::g_svs_post_made));
        out.push_back(kv("svPostSkips", RenderIntegration::g_svs_post_skips));
        out.push_back(kv("svPostBinds", RenderIntegration::g_svs_post_binds));
        out.push_back(kv("svSnapGated", RenderIntegration::g_svs_snap_gated));
        out.push_back(kvf("svInjVpLo", RenderIntegration::g_svs_inj_vp_lo));
        out.push_back(kvf("svInjVpHi", RenderIntegration::g_svs_inj_vp_hi));
        out.push_back(kv("svInjVpRejects", RenderIntegration::g_svs_inj_vp_rejects));
        out.push_back(kv("svInjVpReverts", RenderIntegration::g_svs_inj_vp_reverts));
        out.push_back(kv("svInjRebases", RenderIntegration::g_svs_inj_rebases));
        out.push_back(kv("svInjAbs", RenderIntegration::g_svs_inj_abs));
        // RASTERIZER STATE. svInjRsSets + svInjRsInherit == svInjects.
        // svInjRsInherit must be 0 on the shipped path: non-zero with no
        // debug mode means rast_sun is null, i.e. the injection is still
        // rasterizing with the engine's volume-pass state and its depth is
        // clamped rather than clipped. Climbs only under mode 82.
        out.push_back(kv("svInjRsSets", RenderIntegration::g_svs_inj_rs_sets));
        out.push_back(kv("svInjRsInherit", RenderIntegration::g_svs_inj_rs_inherit));
        // split out of svInjRsInherit, which reported two different failures
        // through one lane. svInjRsSets + svInjRsInherit + svInjRsNull ==
        // svInjects. svInjRsNull must be 0 in every dump, mode or no mode.
        out.push_back(kv("svInjRsNull", RenderIntegration::g_svs_inj_rs_null));
        // svPrimeNoShader -, SPLIT OUT of svPrimeNoState: PSMaskPrime did not
        // compile, or the input layout / VS is absent. svPrimeNoList -, split
        // out of svPrimeNoView: the transform WAS cached and the caster list
        // was empty, which is a different frame from "the injection never
        // ran".
        out.push_back(kv("svPrimeDraws", RenderIntegration::g_svs_prime_draws));
        out.push_back(kv("svPrimeNoRtv", RenderIntegration::g_svs_prime_no_rtv));
        out.push_back(kv("svPrimeMismatch", RenderIntegration::g_svs_prime_mismatch));
        out.push_back(kv("svPrimeNoState", RenderIntegration::g_svs_prime_no_state));
        out.push_back(kv("svPrimeReverts", RenderIntegration::g_svs_prime_reverts));
        // the bracket armed on a frame the injection never ran. MUST be 0
        // whenever svCopyMade is climbing; non-zero means the two are firing
        // on different frames.
        out.push_back(kv("svPrimeNoView", RenderIntegration::g_svs_prime_no_view));
        out.push_back(kv("svPrimeNoShader", RenderIntegration::g_svs_prime_no_shader));
        out.push_back(kv("svPrimeNoList", RenderIntegration::g_svs_prime_no_list));
        out.push_back(kv("svPrimeFailFrameCb", RenderIntegration::g_svs_prime_fail_framecb));
        out.push_back(kv("svPrimeFailObjCb", RenderIntegration::g_svs_prime_fail_objcb));
        out.push_back(kv("svPrimeDrawMeshes", RenderIntegration::g_svs_prime_draw_meshes));
        out.push_back(kv("svPrimeNoVb", RenderIntegration::g_svs_prime_no_vb));
        out.push_back(kv("svPrimePixels", RenderIntegration::g_svs_prime_px));
        out.push_back(kv("svPrimePixelsMin",
            RenderIntegration::g_svs_prime_px_min == ~0ull
                ? 0ull : RenderIntegration::g_svs_prime_px_min));
        out.push_back(kv("svPrimeZero", RenderIntegration::g_svs_prime_zero));
        out.push_back(kv("svPrimeOcclReads", RenderIntegration::g_svs_prime_occl_reads));
        out.push_back(kv("svPrimeOcclSkips", RenderIntegration::g_svs_prime_occl_skips));
        out.push_back(kv("svStenRejCold", RenderIntegration::g_svs_sten_rej_cold));
        out.push_back(kv("svStenRejDims", RenderIntegration::g_svs_sten_rej_dims));
        out.push_back(kvf("svReprojPx", RenderIntegration::g_svs_reproj_px));
        out.push_back(kvf("svReprojPxMax", RenderIntegration::g_svs_reproj_px_max));
        out.push_back(kv("svReprojBehind", RenderIntegration::g_svs_reproj_behind));
        out.push_back(kv("svPostAgeFrames", RenderIntegration::g_svs_post_age_last));
        out.push_back(kv("svPostAgeMax", RenderIntegration::g_svs_post_age_max));
        // Falls back to the raster position, so a miss is a strip, never a
        // wrong answer. EPOCH-MATCHED REPROJECTION. svPostAgeMax measured 1:
        // the mesh reads a mask one seam-frame older than itself, so the
        // lookup now uses the transform stamped with the SNAPSHOT's epoch
        // rather than the newest one.
        out.push_back(kv("svReprojEpochHits", RenderIntegration::g_svs_reproj_epoch_hits));
        out.push_back(kv("stenCycPubs", RenderIntegration::g_sten_cyc_pubs));
        out.push_back(kv("stenCycMiss", RenderIntegration::g_sten_cyc_miss));
        out.push_back(kvf("stenCycCtrPx", RenderIntegration::g_sten_cyc_ctr_px));
        out.push_back(kvf("stenCycCtrPxMax", RenderIntegration::g_sten_cyc_ctr_px_max));
        out.push_back(kv("svReprojEpochMiss", RenderIntegration::g_svs_reproj_epoch_miss));
        out.push_back(kv("svReprojWild", RenderIntegration::g_svs_reproj_wild));
        // svEngVpTakes - composite draws that borrowed the injection's (= the
        // engine's) transform under mode 107. 0 on the default path. renamed
        // from svEngVpTakes.
        out.push_back(kv("svInjVpTakes", RenderIntegration::g_svs_inj_vp_takes));
        out.push_back(kv("svReprojFrames", RenderIntegration::g_svs_reproj_frames));
        out.push_back(kv("svReprojOver4", RenderIntegration::g_svs_reproj_over4));
        out.push_back(kv("svReprojOver32", RenderIntegration::g_svs_reproj_over32));
        out.push_back(kvf("svSwingPx", RenderIntegration::g_svs_swing_px));
        out.push_back(kvf("svSwingPxMax", RenderIntegration::g_svs_swing_px_max));
        out.push_back(kvf("svSwingPxMean",
            RenderIntegration::g_svs_swing_frames > 0
                ? static_cast<float>(RenderIntegration::g_svs_swing_px_sum /
                    static_cast<double>(RenderIntegration::g_svs_swing_frames))
                : -1.0f));
        out.push_back(kv("svSwingFrames", RenderIntegration::g_svs_swing_frames));
        out.push_back(kv("svSwingOver4", RenderIntegration::g_svs_swing_over4));
        out.push_back(kv("svSwingOver32", RenderIntegration::g_svs_swing_over32));
        out.push_back(kvf("svReprojPxMean",
            RenderIntegration::g_svs_reproj_frames > 0
                ? static_cast<float>(RenderIntegration::g_svs_reproj_px_sum /
                    static_cast<double>(RenderIntegration::g_svs_reproj_frames))
                : -1.0f));
        // If they do not, every mask, pre and post Load is clamped to the
        // wrong bound and a band of pixels reads one repeated texel, which
        // looks exactly like a strip. Every mask read goes through KhMaskPx,
        // which clamps to fxMeta.zw.
        out.push_back(kv("fxDimW", static_cast<uint64_t>(RenderIntegration::g_fx_dim_w)));
        out.push_back(kv("fxDimH", static_cast<uint64_t>(RenderIntegration::g_fx_dim_h)));
        out.push_back(kv("compDepthW", static_cast<uint64_t>(RenderIntegration::g_res.comp_depth_w)));
        out.push_back(kv("compDepthH", static_cast<uint64_t>(RenderIntegration::g_res.comp_depth_h)));
        out.push_back(kv("fxDimMismatch", RenderIntegration::g_fx_dim_mismatch));
        out.push_back(kv("fxDimForced", RenderIntegration::g_fx_dim_forced));
        out.push_back(kv("svInjRsClamp", RenderIntegration::g_svs_inj_rs_clamp));
        out.push_back(kv("svInjRsClip", RenderIntegration::g_svs_inj_rs_clip));
        out.push_back(kv("svInjTessNulls", RenderIntegration::g_svs_inj_tess_nulls));
        out.push_back(kv("svInjTessInherit", RenderIntegration::g_svs_inj_tess_inherit));
        out.push_back(kv("svInjViewAdopts", RenderIntegration::g_svs_inj_view_adopts));
        out.push_back(kv("svInjViewRefused", RenderIntegration::g_svs_inj_view_refused));
        out.push_back(kv("svInjViewReverts", RenderIntegration::g_svs_inj_view_reverts));
        // Read this FIRST: every other lane below is meaningless if the
        // transport never armed, and a 0 under mode 109 means readiness
        // refused, not that the read is wrong. Must be non-zero whenever
        // svStenSrc is 1, and it is the lane that separates "the shader had
        // no textures" from "the shader read the wrong thing".
        out.push_back(kv("svStenSrc", static_cast<uint64_t>(RenderIntegration::g_svs_sten_src)));
        out.push_back(kv("svVolFmt", static_cast<uint64_t>(RenderIntegration::g_svs_vol_fmt)));
        out.push_back(kv("svVolSamples", static_cast<uint64_t>(RenderIntegration::g_svs_vol_samples)));
        out.push_back(kv("svVolW", static_cast<uint64_t>(RenderIntegration::g_svs_vol_w)));
        out.push_back(kv("svVolH", static_cast<uint64_t>(RenderIntegration::g_svs_vol_h)));
        out.push_back(kv("svVolCopyMade", RenderIntegration::g_svs_vol_copy_made));
        out.push_back(kv("svVolCopyFails", RenderIntegration::g_svs_vol_copy_fails));
        out.push_back(kv("svVolCopySkips", RenderIntegration::g_svs_vol_copy_skips));
        out.push_back(kv("svVolCreateFails", RenderIntegration::g_svs_vol_create_fails));
        out.push_back(kv("svVolRejMs", RenderIntegration::g_svs_vol_rej_ms));
        out.push_back(kv("svVolRejShape", RenderIntegration::g_svs_vol_rej_shape));
        out.push_back(kv("svVolRejFmt", RenderIntegration::g_svs_vol_rej_fmt));
        out.push_back(kv("svVolRejDims", RenderIntegration::g_svs_vol_rej_dims));
        out.push_back(kv("svVolRejCold", RenderIntegration::g_svs_vol_rej_cold));
        out.push_back(kv("svVolStenBinds", RenderIntegration::g_svs_vol_sten_binds));
        out.push_back(kv("svVolLatches", RenderIntegration::g_svs_vol_latches));
        out.push_back(kv("svVolCode", RenderIntegration::g_svs_vol_code));
        out.push_back(kv("svVolWitFrames", RenderIntegration::g_svs_vol_wit_frames));
        out.push_back(kv("svVolWitAbsent", RenderIntegration::g_svs_vol_wit_absent));
        // svSharePubs / svShareStale / svShareAbsent - published, wrong
        // frame, never published. DO NOT USE svReprojPxMean AS THE BAR. THE
        // SHARED-VIEW ARM (mode 133). svShareTakes - seam frames that adopted
        // the colour injection's own view.
        out.push_back(kv("svSharePubs", RenderIntegration::g_svs_share_pubs));
        out.push_back(kv("svShareTakes", RenderIntegration::g_svs_share_takes));
        out.push_back(kv("svShareStale", RenderIntegration::g_svs_share_stale));
        out.push_back(kv("svShareAbsent", RenderIntegration::g_svs_share_absent));
        // If this arm works that offset goes to zero and the hole with it.
        // THE INVERTED SHARE (mode 134). Acceptance is svSeamViewTakes ==
        // compositeInjections with Stale 0.
        out.push_back(kv("svSeamViewPubs", RenderIntegration::g_svs_seam_view_pubs));
        out.push_back(kv("svSeamViewTakes", RenderIntegration::g_svs_seam_view_takes));
        out.push_back(kv("svSeamViewStale", RenderIntegration::g_svs_seam_view_stale));
        out.push_back(kv("svSeamViewAbsent", RenderIntegration::g_svs_seam_view_absent));
        out.push_back(kv("svSeamViewAge", RenderIntegration::g_svs_seam_view_age_last));
        out.push_back(kv("svSeamViewAgeMax", RenderIntegration::g_svs_seam_view_age_max));
        out.push_back(kv("svSeamRotTakes", RenderIntegration::g_svs_seam_rot_takes));
        out.push_back(kvf("svSeamRotCamDx", RenderIntegration::g_svs_seam_rot_cam_dx));
        // svSeamRotCamDxMax - THE lane 135 needed and did not have. It does:
        // 134 matched both terms and svReprojPxMean went to 0; 135 matched
        // rotation only and it returned to 26.1. svSeamTrnTakes - mode 136:
        // translation shared, rotation kept.
        out.push_back(kvf("svSeamRotCamDxMax", RenderIntegration::g_svs_seam_rot_cam_dx_max));
        // Snap = takes whose adopted basis was bit-identical to ours, so the
        // translation row was left alone - at rest this should track
        // svInjects, and it is the arm that removes the stationary offset.
        // Exact = takes rebuilt through the honest 3x3 inverse. Refused =
        // degenerate basis, our own view kept.
        out.push_back(kv("nearzRampFills", RenderIntegration::g_nearz_ramp_fills));
        out.push_back(kv("blkDarkPubRefusals", RenderIntegration::g_blk_dark_pub_refusals));
        // 28 other metric lanes had the same defect - full list and the
        // reason they were never caught at KH_STATS_KVF_AUDIT. 0 means the
        // gate never fired and any null result on the black box is void.
        // Sizes the gate's bar.
        out.push_back(kv("blkZeroSunRefusals", RenderIntegration::g_blk_zero_sun_refusals));
        // KH_NIGHT_ZERO_SUN: zero-sun blocks SHIPPED because the standing
        // scene is measured dark.
        out.push_back(kv("blkZeroSunAdmits", RenderIntegration::g_blk_zero_sun_admits));
        // zero-sun blocks refused because the sky witness contradicts the
        // dark standing. Day-hijack sessions must show this climbing with
        // Admits flat; night sessions must show it at 0 with Admits climbing.
        out.push_back(kv("blkZeroSunWitRefusals", RenderIntegration::g_blk_zero_sun_wit_refusals));
        // standing-band sightings that spared the bright pending under
        // dark-contradiction. Non-zero = the hijack occurred AND the kill was
        // withheld; read blkDarkReadopts/blkRegimeAdopts for the heal, and
        // blkStickyPendAgeMax finally clearing 0.5 for the mechanism proof.
        out.push_back(kv("blkContraPendHolds", RenderIntegration::g_blk_contra_pend_holds));
        out.push_back(kvf("blkShadeNow", RenderIntegration::g_blk_shade_now));
        out.push_back(kvf("blkShadeMin", RenderIntegration::g_blk_shade_min));
        out.push_back(kvf("blkShadeMax", RenderIntegration::g_blk_shade_max));
        out.push_back(kv("blkShadeN", RenderIntegration::g_blk_shade_n));
        out.push_back(kv("blkShadeDarkFrames", RenderIntegration::g_blk_shade_dark));
        out.push_back(kvf("blkShadeAmbR", RenderIntegration::g_blk_shade_amb[0]));
        out.push_back(kvf("blkShadeAmbG", RenderIntegration::g_blk_shade_amb[1]));
        out.push_back(kvf("blkShadeAmbB", RenderIntegration::g_blk_shade_amb[2]));
        out.push_back(kvf("blkShadeSunR", RenderIntegration::g_blk_shade_sun[0]));
        out.push_back(kvf("blkShadeSunG", RenderIntegration::g_blk_shade_sun[1]));
        out.push_back(kvf("blkShadeSunB", RenderIntegration::g_blk_shade_sun[2]));
        out.push_back(kvf("blkShadeValid", RenderIntegration::g_blk_shade_valid));
        out.push_back(kvf("blkShadeLitGate", RenderIntegration::g_blk_shade_litgate));
        out.push_back(kvf("blkShadeStrength", RenderIntegration::g_blk_shade_strength));
        out.push_back(kvf("blkShadeStdAmb", RenderIntegration::g_blk_shade_std_amb));
        out.push_back(kvf("blkShadeStdSun", RenderIntegration::g_blk_shade_std_sun));
        out.push_back(kvf("blkKeelSl", RenderIntegration::g_blk_keel_sl));
        out.push_back(kv("blkKeelSnaps", RenderIntegration::g_blk_keel_snaps));
        out.push_back(kvf("blkWalkRatio", RenderIntegration::g_blk_walk_ratio));
        out.push_back(kvf("blkWalkRatioMin", RenderIntegration::g_blk_walk_ratio_min));
        out.push_back(kvf("blkWalkRatioMax", RenderIntegration::g_blk_walk_ratio_max));
        out.push_back(kvf("blkWalkMinT", RenderIntegration::g_blk_walk_min_t));
        out.push_back(kvf("blkWalkMinKeel", RenderIntegration::g_blk_walk_min_keel));
        out.push_back(kvf("blkWalkMinStd", RenderIntegration::g_blk_walk_min_std));
        out.push_back(kv("blkWalkDimFrames", RenderIntegration::g_blk_walk_dim_frames));
        out.push_back(kvf("blkWalkDimFirstS", RenderIntegration::g_blk_walk_dim_first));
        out.push_back(kvf("blkWalkDimLastS", RenderIntegration::g_blk_walk_dim_last));
        out.push_back(kvf("blkWalkDimSpanMaxS", RenderIntegration::g_blk_walk_dim_span_max));
        out.push_back(kvf("blkAdoptRatioLast", RenderIntegration::g_blk_adopt_ratio_last));
        out.push_back(kvf("blkAdoptRatioMin", RenderIntegration::g_blk_adopt_ratio_min));
        out.push_back(kvf("blkAdoptRatioMax", RenderIntegration::g_blk_adopt_ratio_max));
        out.push_back(kvf("blkAdoptGapLastS", RenderIntegration::g_blk_adopt_gap_last));
        out.push_back(kvf("blkAdoptGapMaxS", RenderIntegration::g_blk_adopt_gap_max));
        out.push_back(kvf("blkAdoptTLast", RenderIntegration::g_blk_adopt_t_last));
        out.push_back(kv("blkStarvedDimHolds", RenderIntegration::g_blk_starved_dim_holds));
        out.push_back(kv("blkStarvedDimConcedes", RenderIntegration::g_blk_starved_dim_concedes));
        out.push_back(kvf("blkStarvedDimHeldMaxS", RenderIntegration::g_blk_sdh_held_max));
        // raw effect-time stamp of the last bright upload seen by the
        // arbitration (-1 = never). Subtract from effectTimeS to age it.
        out.push_back(kvf("blkBrightSeenT", RenderIntegration::g_blk_bright_seen_t));
        out.push_back(kvf("blkShadeProbeAgeS", RenderIntegration::g_blk_shade_probe_age));
        out.push_back(kvf("blkShadeT", RenderIntegration::g_blk_shade_t));
        out.push_back(kv("blkDimFrames", RenderIntegration::g_blk_dim_frames));
        out.push_back(kvf("blkDimFirstS", RenderIntegration::g_blk_dim_first_s));
        out.push_back(kvf("blkDimLastS", RenderIntegration::g_blk_dim_last_s));
        out.push_back(kvf("blkShadeAnchSun", RenderIntegration::g_blk_shade_anch_sun));
        out.push_back(kvf("blkShadeMapAge", RenderIntegration::g_blk_shade_map_age));
        out.push_back(kvf("blkAcceptRatioMin", RenderIntegration::g_blk_accept_ratio_min));
        out.push_back(kvf("blkAcceptRatioAmb", RenderIntegration::g_blk_accept_ratio_amb));
        out.push_back(kv("blkAcceptRatioN", RenderIntegration::g_blk_accept_ratio_n));
        // the colour pass taking the seam's own publication. compShareStale
        // must stay near 0 - if it climbs, the seq-adjacency assumption is
        // wrong and this fold is unsafe.
        out.push_back(kv("compShareTakes", RenderIntegration::g_comp_share_takes));
        out.push_back(kv("compShareStale", RenderIntegration::g_comp_share_stale));
        out.push_back(kvf("blkDarkPubSl", RenderIntegration::g_blk_dark_pub_sl));
        out.push_back(kvf("blkDarkPubRefSl", RenderIntegration::g_blk_dark_pub_ref_sl));
        out.push_back(kv("svSeamRotSnap", RenderIntegration::g_svs_seam_rot_snap));
        out.push_back(kv("svSeamRotExact", RenderIntegration::g_svs_seam_rot_exact));
        out.push_back(kv("svSeamRotRefused", RenderIntegration::g_svs_seam_rot_refused));
        // basis takes the colour pass DECLINED because the seam flagged its
        // published basis unfreshened. Reads ~svLiveTrnGuard on a fast-camera
        // session and 0 in ordinary play.
        out.push_back(kv("svSeamRotUnfresh", RenderIntegration::g_svs_seam_rot_unfresh));
        out.push_back(kvf("svCamRtMax", RenderIntegration::g_svs_cam_rt_max));
        out.push_back(kvf("jitRebaseMm", RenderIntegration::g_jit_rebase_mm));
        out.push_back(kvf("jitRebaseMaxMm", RenderIntegration::g_jit_rebase_max_mm));
        out.push_back(kvf("jitCamUlpMm", RenderIntegration::g_jit_cam_ulp_mm));
        out.push_back(kvf("jitCamAbsM", RenderIntegration::g_jit_cam_abs_m));
        out.push_back(kvf("jitCamStepMm", RenderIntegration::g_jit_cam_step_mm));
        out.push_back(kvf("jitCamStepMaxMm", RenderIntegration::g_jit_cam_step_max_mm));
        out.push_back(kvf("jitBasisStep", RenderIntegration::g_jit_basis_step));
        out.push_back(kv("jitFrames", RenderIntegration::g_jit_frames));
        out.push_back(kv("jitBasisWit", RenderIntegration::g_jit_basis_wit));
        out.push_back(kv("jitCamHolds", RenderIntegration::g_jit_cam_holds));
        out.push_back(kv("jitCamHoldRun", RenderIntegration::g_jit_hold_run_max));
        // jitLatchT is effect time, so a latch older than the trace ring is
        // still readable here - which is the entire point of latching rather
        // than sampling.
        out.push_back(kvf("jitLatchMm", RenderIntegration::g_jit_latch_mm));
        out.push_back(kvf("jitLatchUlpMm", RenderIntegration::g_jit_latch_ulp_mm));
        out.push_back(kvf("jitLatchCamAbsM", RenderIntegration::g_jit_latch_cam_abs));
        out.push_back(kvf("jitLatchStepMm", RenderIntegration::g_jit_latch_step_mm));
        out.push_back(kvf("jitLatchBasis", RenderIntegration::g_jit_latch_basis));
        out.push_back(kvf("jitLatchT", RenderIntegration::g_jit_latch_t));
        // Then engCamTakes, the arm's positive control: 0 under mode 236
        // means the fix never engaged and any screen verdict is VOID (the
        // mode-118 lesson). READ ORDER: engCamDxMaxMm FIRST - millimetres (0
        // to ~0.5) is a correct lock, METRES is a wrong row and every other
        // lane is then about the wrong value.
        out.push_back(kv("engCamLocks", RenderIntegration::g_engcam_locks));
        out.push_back(kv("engCamLockDrops", RenderIntegration::g_engcam_lock_drops));
        out.push_back(kvf("engCamLockOff", static_cast<float>(RenderIntegration::g_engcam_off)));
        out.push_back(kv("engCamFresh", RenderIntegration::g_engcam_fresh));
        out.push_back(kv("engCamStale", RenderIntegration::g_engcam_stale));
        out.push_back(kv("engCamTakes", RenderIntegration::g_engcam_takes));
        out.push_back(kv("engCamCands", RenderIntegration::g_engcam_cand_total));
        out.push_back(kv("engCamCandOver", RenderIntegration::g_engcam_cand_over));
        out.push_back(kv("engCamValOver", RenderIntegration::g_engcam_val_over));
        out.push_back(kv("engCamPhaseStand", RenderIntegration::g_engcam_phase_stands));
        out.push_back(kv("engCamAmbig", RenderIntegration::g_engcam_ambig));
        out.push_back(kvf("engCamDxMm", RenderIntegration::g_engcam_dx_mm));
        out.push_back(kvf("engCamDxMaxMm", RenderIntegration::g_engcam_dx_max_mm));
        out.push_back(kv("svSeamTrnTakes", RenderIntegration::g_svs_seam_trn_takes));
        // svNoAdoptFrames - mode 137: BOTH passes stood the view adoption
        // down and read the shared cycle_pv latch instead. Mode 102 only ever
        // disabled it in the SEAM, so every 102 reading is a half-applied
        // revert. Expect == compositeInjections under 137.
        out.push_back(kv("svNoAdoptFrames", RenderIntegration::g_svs_noadopt_frames));
        // svSeamNoAdopt - SEAM-side stand-downs (102/137/142). The
        // colour-only counter above is how 102's half-application hid for ten
        // builds; under 137/142 BOTH must track their pass counts or a half
        // did not engage - that is a reading now, not a trust.
        out.push_back(kv("svSeamNoAdopt", RenderIntegration::g_svs_seam_noadopt));
        out.push_back(kv("svPvLatchSerial",
                         static_cast<uint64_t>(RenderIntegration::g_pv_latch_serial)));
        // svLiveTrnTakes == svInjects with Miss/Guard ~0 is engagement;
        // svLiveTrnDx/Max is the correction applied (expect ~camStepM); the
        // residual is svSeamRotCamDx / per-frame seamCamDxM.
        out.push_back(kv("svLiveTrnTakes", RenderIntegration::g_svs_live_trn_takes));
        out.push_back(kv("svLiveTrnMiss", RenderIntegration::g_svs_live_trn_miss));
        out.push_back(kv("svLiveTrnGuard", RenderIntegration::g_svs_live_trn_guard));
        out.push_back(kvf("svLiveTrnDx", RenderIntegration::g_svs_live_trn_dx));
        out.push_back(kvf("svLiveTrnDxMax", RenderIntegration::g_svs_live_trn_dx_max));
        // svLivePrjTakes == svInjects with Ref ~0 is engagement; the verdict
        // lane is per-frame seamNear vs injNear through a ramp.
        out.push_back(kv("svLivePrjTakes", RenderIntegration::g_svs_live_prj_takes));
        out.push_back(kv("svLivePrjRef", RenderIntegration::g_svs_live_prj_ref));
        out.push_back(kvf("svSeamNear", RenderIntegration::g_svs_seam_near));
        // svTrnExtrap - mode 147, RETIRED (deceleration false positives
        // regressed the slice). Kept so old scripts validate.
        out.push_back(kv("svTrnExtrap", RenderIntegration::g_svs_trn_extrap));
        // Comm+Live == svInjects after warmup is engagement; the verdict lane
        // stays per-frame seamNear vs injNear. Bound = translation takes
        // refused by the trajectory bound (foreign publications).
        out.push_back(kv("svPairComm", RenderIntegration::g_svs_pair_comm));
        out.push_back(kv("svPairLive", RenderIntegration::g_svs_pair_live));
        out.push_back(kv("svPairFall", RenderIntegration::g_svs_pair_fall));
        out.push_back(kv("svLiveTrnBound", RenderIntegration::g_svs_live_trn_bound));
        out.push_back(kv("svLiveRotTakes", RenderIntegration::g_svs_live_rot_takes));
        out.push_back(kv("svLiveRotRef", RenderIntegration::g_svs_live_rot_ref));
        out.push_back(kv("svPairWide", RenderIntegration::g_svs_pair_wide));
        out.push_back(kv("svPairHold", RenderIntegration::g_svs_pair_hold));
        // svPairSniff - sniff-pair encodes (fallback).
        out.push_back(kv("svPairSniff", RenderIntegration::g_svs_pair_sniff));
        // predicted fallback + the live-refusal reasons, split.
        out.push_back(kv("svPairPred", RenderIntegration::g_svs_pair_pred));
        out.push_back(kv("svPairSaneRef", RenderIntegration::g_svs_pair_sane_ref));
        out.push_back(kv("svPairCorrRef", RenderIntegration::g_svs_pair_corr_ref));
        // base = return-to-baseline live accepts; rebase = predictor re-bases
        // off two agreeing out-of-band commits; refGuard = corridor referee
        // substitutions (polluted committed reference); noRef = live refused
        // solely for want of a fresh committed reference (previously
        // invisible).
        out.push_back(kv("svPairBase", RenderIntegration::g_svs_pair_base));
        out.push_back(kv("svPairRebase", RenderIntegration::g_svs_pair_rebase));
        out.push_back(kv("svPairRefGuard", RenderIntegration::g_svs_pair_ref_guard));
        out.push_back(kv("svPairNoRef", RenderIntegration::g_svs_pair_noref));
        out.push_back(kv("svCbcRecords", RenderIntegration::g_cbc_records));
        out.push_back(kv("svCbcEvicts", RenderIntegration::g_cbc_evicts));
        out.push_back(kv("svCbcMatches", RenderIntegration::g_cbc_matches));
        out.push_back(kv("svCbcMisses", RenderIntegration::g_cbc_misses));
        out.push_back(kv("svCbcEncodes", RenderIntegration::g_cbc_encodes));
        out.push_back(kv("svCbcCamRecs", RenderIntegration::g_cbc_cam_recs));
        out.push_back(kv("svCbcCamHits", RenderIntegration::g_cbc_cam_hits));
        out.push_back(kv("svLiveTrnRebase", RenderIntegration::g_svs_live_trn_rebase));
        out.push_back(kv("svLiveTrnWit", RenderIntegration::g_svs_live_trn_wit));
        out.push_back(kv("svLiveTrnWide", RenderIntegration::g_svs_live_trn_wide));
        // svLiveTrnAnchOff/Max - |raw live camera - anchored candidate| in
        // metres = the second-flavor anchor offset the delta take removed
        // (expected ~0 healthy; 0.18-0.28 on the capture12 scenario).
        // svLiveTrnLpMiss - cold/post-miss runs that fell back to the
        // absolute form for one frame.
        out.push_back(kvf("svLiveTrnAnchOff", RenderIntegration::g_svs_trn_anch_off));
        // svSeamAnchDx/Max - |seam adopted-view camera - cycle latch camera|
        // in metres, the multiplex-flavor gauge (0.258 constant in
        // capture13's Zeus session; ~0 healthy).
        out.push_back(kvf("svSeamAnchDx", RenderIntegration::g_svs_seam_anch_dx));
        out.push_back(kvf("svSeamAnchDxMax", RenderIntegration::g_svs_seam_anch_dx_max));
        // svLiveFovTakes/Ref - seam FOV terms from the live fetch (the zoom
        // axis); svCbcClassRef/Near - census encodes refused by the
        // two-witness class referee, and the last refused near.
        out.push_back(kv("svLiveFovTakes", RenderIntegration::g_svs_live_fov_takes));
        out.push_back(kv("svLiveFovRef", RenderIntegration::g_svs_live_fov_ref));
        out.push_back(kv("svCbcClassRef", RenderIntegration::g_cbc_class_ref));
        out.push_back(kvf("svCbcClassRefNear", RenderIntegration::g_cbc_class_ref_near));
        // svCbcBandSkips - recorder notes skipped by the shared camera-class
        // band (out-of-band content no longer overwrites a tracked buffer's
        // world-class pair).
        out.push_back(kv("svCbcBandSkips", RenderIntegration::g_cbc_band_skips));
        // svLiveFovRatio/Max - per-frame live/latch FOV ratio (the flicker
        // gauge; legit zoom tops ~1.13, the bar is 1.30).
        out.push_back(kvf("svLiveFovRatio", RenderIntegration::g_svs_live_fov_ratio));
        out.push_back(kvf("svLiveFovRatioMax", RenderIntegration::g_svs_live_fov_ratio_max));
        // The seam has ridden the live fetch's scale terms and the visible
        // box never did, so through a zoom the box rasterised one frame
        // behind its own footprint and the engine's cascade + stencil
        // verdicts leaked across its silhouette.
        out.push_back(kv("volPassReads", RenderIntegration::g_vpx_reads));
        out.push_back(kv("volPassBusy", RenderIntegration::g_vpx_busy));
        out.push_back(kv("volPassSkips", RenderIntegration::g_vpx_skips));
        out.push_back(kvf("volPassWCol", RenderIntegration::g_vpx_wcol));
        out.push_back(kvf("volPassNearLast", RenderIntegration::g_vpx_near));
        out.push_back(kv("svEngVpArms", RenderIntegration::g_svs_engvp_arms));
        out.push_back(kv("volDrawReads", RenderIntegration::g_vpx_draw_reads));
        out.push_back(kvf("volDrawNearLast", RenderIntegration::g_vpx_draw_near));
        out.push_back(kv("seamFetchDups", RenderIntegration::g_svs_fetch_dups));
        out.push_back(kv("seamFetchMoves", RenderIntegration::g_svs_fetch_moves));
        out.push_back(kvf("seamFetchDupM", RenderIntegration::g_svs_fetch_dup_m));
        out.push_back(kvf("bandPickCamDxMax", RenderIntegration::g_band_pick_cam_dx_max));
        {
            static const char* const KHBT_N[8] = { "bandNear0", "bandNear1", "bandNear2", "bandNear3",
                                                   "bandNear4", "bandNear5", "bandNear6", "bandNear7" };
            static const char* const KHBT_F[8] = { "bandFar0", "bandFar1", "bandFar2", "bandFar3",
                                                   "bandFar4", "bandFar5", "bandFar6", "bandFar7" };
            static const char* const KHBT_A[8] = { "bandAgeMs0", "bandAgeMs1", "bandAgeMs2", "bandAgeMs3",
                                                   "bandAgeMs4", "bandAgeMs5", "bandAgeMs6", "bandAgeMs7" };
            static const char* const KHBT_D[8] = { "bandCamDx0", "bandCamDx1", "bandCamDx2", "bandCamDx3",
                                                   "bandCamDx4", "bandCamDx5", "bandCamDx6", "bandCamDx7" };
            for (int khbt_i = 0; khbt_i < 8; ++khbt_i) {
                out.push_back(kvf(KHBT_N[khbt_i], RenderIntegration::g_band_tab_near[khbt_i]));
                out.push_back(kvf(KHBT_F[khbt_i], RenderIntegration::g_band_tab_far[khbt_i]));
                out.push_back(kvf(KHBT_A[khbt_i], RenderIntegration::g_band_tab_age[khbt_i]));
                out.push_back(kvf(KHBT_D[khbt_i], RenderIntegration::g_band_tab_cdx[khbt_i]));
            }
            out.push_back(kvf("bandGapLastM", RenderIntegration::g_band_gap_last_m));
            out.push_back(kvf("bandGapMaxM", RenderIntegration::g_band_gap_max_m));
            out.push_back(kv("bandGapFrames", RenderIntegration::g_band_gap_frames));
            out.push_back(kvf("bandGapPair", RenderIntegration::g_band_gap_pair));
            out.push_back(kvf("bandGapCamStep", RenderIntegration::g_band_gap_camstep));
            // The lanes above are the LAST committed frame, which for a
            // few-frame artifact is almost never the interesting one; these
            // are latched at the event. THE TABLE AS IT STOOD ON THE
            // WORST-GAP FRAME.
            static const char* const KHGL_N[8] = { "gapNear0", "gapNear1", "gapNear2", "gapNear3",
                                                   "gapNear4", "gapNear5", "gapNear6", "gapNear7" };
            static const char* const KHGL_F[8] = { "gapFar0", "gapFar1", "gapFar2", "gapFar3",
                                                   "gapFar4", "gapFar5", "gapFar6", "gapFar7" };
            static const char* const KHGL_A[8] = { "gapAgeMs0", "gapAgeMs1", "gapAgeMs2", "gapAgeMs3",
                                                   "gapAgeMs4", "gapAgeMs5", "gapAgeMs6", "gapAgeMs7" };
            static const char* const KHGL_D[8] = { "gapCamDx0", "gapCamDx1", "gapCamDx2", "gapCamDx3",
                                                   "gapCamDx4", "gapCamDx5", "gapCamDx6", "gapCamDx7" };
            for (int khgl_i = 0; khgl_i < 8; ++khgl_i) {
                out.push_back(kvf(KHGL_N[khgl_i], RenderIntegration::g_band_gap_near[khgl_i]));
                out.push_back(kvf(KHGL_F[khgl_i], RenderIntegration::g_band_gap_far[khgl_i]));
                out.push_back(kvf(KHGL_A[khgl_i], RenderIntegration::g_band_gap_age[khgl_i]));
                out.push_back(kvf(KHGL_D[khgl_i], RenderIntegration::g_band_gap_cdx[khgl_i]));
            }
        }
        out.push_back(kv("bandPickCamBad", RenderIntegration::g_band_pick_cam_bad));
        out.push_back(kv("bandTrnEscapes", RenderIntegration::g_ls.band_trn_escapes));
        out.push_back(kv("bandRotEscapes", RenderIntegration::g_ls.band_rot_escapes));
        // This is the engagement lane: 0 means the fold never fired and any
        // null is void.
        out.push_back(kv("bandBudgetEscapes", RenderIntegration::g_ls.band_budget_escapes));
        out.push_back(kv("bandLayoutEscapes", RenderIntegration::g_ls.band_layout_escapes));
        out.push_back(kv("seamRetry", RenderIntegration::g_svs_live_trn_retry));
        out.push_back(kv("seamRetryOk", RenderIntegration::g_svs_live_trn_retry_ok));
        out.push_back(kv("trigFrozen", RenderIntegration::g_ffr_freeze ? 1 : 0));
        // The lane could never report the state it holds in most sessions.
        // kvf, and the two above it drop a static_cast<float> that only ever
        // detoured.
        out.push_back(kv("trigSerial", RenderIntegration::g_ffr_trig_serial));
        out.push_back(kv("trigReason", RenderIntegration::g_ffr_trig_reason));
        out.push_back(kvf("trigPostLeft", static_cast<float>(RenderIntegration::g_ffr_trig_post)));
        // Read vpxRefMiss FIRST: nonzero means the serial ring lost rows and
        // the per-row lanes on those rows are -1 rather than wrong.
        out.push_back(kv("vpxRefPubs", RenderIntegration::g_vpxr_pubs));
        out.push_back(kv("vpxRefHits", RenderIntegration::g_vpxr_hits));
        out.push_back(kv("vpxRefMiss", RenderIntegration::g_vpxr_miss));
        out.push_back(kvf("volDrawSwingPx", RenderIntegration::g_vpx_draw_swing_px));
        out.push_back(kvf("volDrawSwingMax", RenderIntegration::g_vpx_draw_swing_max));
        out.push_back(kvf("volPassSwingPx", RenderIntegration::g_vpx_pass_swing_px));
        out.push_back(kvf("volPassSwingMax", RenderIntegration::g_vpx_pass_swing_max));
        out.push_back(kv("svSniffOvr", RenderIntegration::g_svs_sniff_ovr));
        out.push_back(kvf("svSniffOvrRatio", RenderIntegration::g_svs_sniff_ovr_ratio));
        out.push_back(kvf("svSniffOvrNear", RenderIntegration::g_svs_sniff_ovr_near));
        out.push_back(kv("svCompFovTakes", RenderIntegration::g_comp_fov_takes));
        out.push_back(kv("svCompFovRef", RenderIntegration::g_comp_fov_ref));
        out.push_back(kvf("svCompFovRatio", RenderIntegration::g_comp_fov_ratio));
        out.push_back(kvf("svCompFovRatioMax", RenderIntegration::g_comp_fov_ratio_max));
        out.push_back(kvf("svLiveTrnAnchOffMax", RenderIntegration::g_svs_trn_anch_off_max));
        out.push_back(kv("svLiveTrnLpMiss", RenderIntegration::g_svs_trn_lp_miss));
        // This arm does not try to align the two rasterisations at all - it
        // makes the depth-only footprint bigger than the visible box so a
        // 18-34 px displacement cannot expose a hole. svGrowDraws /
        // svGrowLast - seam draws issued with an ENLARGED footprint, and the
        // factor. Expect == svInjectDraws under 138-140.
        out.push_back(kv("svGrowDraws", RenderIntegration::g_svs_grow_draws));
        out.push_back(kvf("svGrowLast", RenderIntegration::g_svs_grow_last));
        out.push_back(kv("sunProxyCasters", RenderIntegration::g_sun_proxy_casters));
        out.push_back(kv("svVolIsColour", RenderIntegration::g_svs_vol_is_colour));
        out.push_back(kv("svVolNotColour", RenderIntegration::g_svs_vol_not_colour));
        out.push_back(kv("svColourDsvNone", RenderIntegration::g_svs_colour_dsv_none));
        out.push_back(kv("svSeamDimChanges", RenderIntegration::g_svs_seam_dim_changes));
        out.push_back(kv("svSeamWMin", RenderIntegration::g_svs_seam_w_min));
        out.push_back(kv("svSeamHMin", RenderIntegration::g_svs_seam_h_min));
        out.push_back(kv("svVolLatchDrops", RenderIntegration::g_svs_vol_latch_drops));
        // That misplacement - not the reprojection band, not the guard - is
        // the shadow lag: the stencil verdict is computed for a world point
        // our misplaced footprint corresponds to. THE TRANSPORT'S OWN EPOCH
        // KEY.
        out.push_back(kv("injAdoptSrc", static_cast<uint64_t>(RenderIntegration::g_inj_adopt_src)));
        out.push_back(kvf("injAdoptRotDeg", RenderIntegration::g_inj_adopt_rot_deg));
        out.push_back(kvf("injAdoptRotMax", RenderIntegration::g_inj_adopt_rot_max));
        out.push_back(kv("injAdoptFvCold", RenderIntegration::g_inj_adopt_fv_cold));
        out.push_back(kvf("injCycleErrPx", RenderIntegration::g_inj_cycle_err_px));
        out.push_back(kvf("injCycleErrMax", RenderIntegration::g_inj_cycle_err_max));
        out.push_back(kvf("injCycleErrMean",
            RenderIntegration::g_inj_cycle_frames > 0
                ? static_cast<float>(RenderIntegration::g_inj_cycle_err_sum /
                                     static_cast<double>(RenderIntegration::g_inj_cycle_frames))
                : -1.0f));
        out.push_back(kv("injCycleFrames", RenderIntegration::g_inj_cycle_frames));
        // injCycleIncoherent - MUST BE 0. The three arms are the sides of one
        // triangle: injCycleErrPx must lie within svSwingPx +- svReprojPx on
        // the same frame.
        out.push_back(kv("injCycleIncoherent", RenderIntegration::g_inj_cycle_incoherent));
        out.push_back(kv("injCycleUnchecked", RenderIntegration::g_inj_cycle_unchecked));
        // Then mode 116 and svReprojPxMean is the verdict (predicted:
        // collapses from 26-95 px toward 0). the engine re-binds that
        // resource at different offsets per family.
        out.push_back(kv("engFpHits", RenderIntegration::g_svs_eng_fp_hits));
        out.push_back(kv("engFpMiss", RenderIntegration::g_svs_eng_fp_miss));
        out.push_back(kv("engBlkOff", static_cast<uint64_t>(
            RenderIntegration::g_svs_eng_blk_off < 0 ? 0xFFFFFFFFull
                : static_cast<uint64_t>(RenderIntegration::g_svs_eng_blk_off))));
        // the ABSOLUTE address in the resource - the one the copy uses.
        // engBlkOff is kept as the window-relative value it always was,
        // purely so engOffDisagree has two things to compare.
        out.push_back(kv("engBlkAbs", static_cast<uint64_t>(
            RenderIntegration::g_svs_eng_blk_abs < 0 ? 0xFFFFFFFFull
                : static_cast<uint64_t>(RenderIntegration::g_svs_eng_blk_abs))));
        out.push_back(kv("engOffDisagree", RenderIntegration::g_svs_eng_off_disagree));
        out.push_back(kv("engCopies", RenderIntegration::g_svs_eng_copies));
        out.push_back(kv("engCopySkips", RenderIntegration::g_svs_eng_copy_skips));
        out.push_back(kv("engCopyFails", RenderIntegration::g_svs_eng_copy_fails));
        out.push_back(kv("engBinds", RenderIntegration::g_svs_eng_binds));
        out.push_back(kv("engArmInj", static_cast<uint64_t>(RenderIntegration::g_svs_eng_arm_inj)));
        out.push_back(kv("engArmComp", static_cast<uint64_t>(RenderIntegration::g_svs_eng_arm_comp)));
        out.push_back(kv("engCopyAgeLast", RenderIntegration::g_svs_eng_copy_age_last));
        out.push_back(kv("engCopyAgeMax", RenderIntegration::g_svs_eng_copy_age_max));
        out.push_back(kv("engArmCompStale", RenderIntegration::g_svs_eng_arm_comp_stale));
        out.push_back(kv("engSweeps", RenderIntegration::g_svs_eng_sweeps));
        out.push_back(kv("engSweepFrameMax",
            static_cast<uint64_t>(RenderIntegration::g_svs_eng_sweep_frame_max)));
        out.push_back(kv("engFpReconfirms", RenderIntegration::g_svs_eng_fp_reconfirms));
        out.push_back(kv("engFpDrifts", RenderIntegration::g_svs_eng_fp_drifts));
        out.push_back(kv("engStageBusy", RenderIntegration::g_svs_eng_stage_busy));
        // Slot FOUR - the capture's b2 - which is why the four-slot scan
        // through could never have found it, and why the 240-byte decoy at
        // slot 0 (F0 1.0, F16 0) made read the miss as a wrong-draw problem.
        out.push_back(kv("engBindRejects", RenderIntegration::g_svs_eng_bind_rejects));
        out.push_back(kv("engSrcSlot", static_cast<uint64_t>(RenderIntegration::g_svs_eng_src_slot)));
        out.push_back(kv("engSrcFirst", static_cast<uint64_t>(RenderIntegration::g_svs_eng_src_first)));
        out.push_back(kv("engSrcNum", static_cast<uint64_t>(RenderIntegration::g_svs_eng_src_num)));
        out.push_back(kv("engBindRejSlot", static_cast<uint64_t>(RenderIntegration::g_svs_eng_bind_rej_slot)));
        out.push_back(kv("engBindRejFirst", static_cast<uint64_t>(RenderIntegration::g_svs_eng_bind_rej_first)));
        out.push_back(kv("engBindRejNum", static_cast<uint64_t>(RenderIntegration::g_svs_eng_bind_rej_num)));
        out.push_back(kv("engVerifyPass", RenderIntegration::g_svs_eng_ver_pass));
        out.push_back(kv("engVerifyFail", RenderIntegration::g_svs_eng_ver_fail));
        out.push_back(kv("engVerifyBusy", RenderIntegration::g_svs_eng_ver_busy));
        out.push_back(kvf("engVerLen", RenderIntegration::g_svs_eng_ver_len));
        out.push_back(kvf("engVerWcol", RenderIntegration::g_svs_eng_ver_wcol));
        out.push_back(kvf("engVerRow3", RenderIntegration::g_svs_eng_ver_row3));
        out.push_back(kvf("engVerWcolMax", RenderIntegration::g_svs_eng_ver_wcol_max));
        out.push_back(kvf("engVerRow3Max", RenderIntegration::g_svs_eng_ver_row3_max));
        // Confirm it with mode 121/122/123, which force the copy to read
        // candidate 1/2/3 - engVerifyFail should collapse and the fallback
        // frames go with it.
        out.push_back(kvf("engVerRotDeg", RenderIntegration::g_svs_eng_ver_rot_deg));
        out.push_back(kvf("engVerRotMaxDeg", RenderIntegration::g_svs_eng_ver_rot_max_deg));
        out.push_back(kv("engVerRotOver20", RenderIntegration::g_svs_eng_ver_rot_over20));
        out.push_back(kv("engVerRotSamples", RenderIntegration::g_svs_eng_ver_rot_samples));
        out.push_back(kvf("engVerRotMeanDeg", RenderIntegration::g_svs_eng_ver_rot_samples
            ? static_cast<float>(RenderIntegration::g_svs_eng_ver_rot_sum /
                                 static_cast<double>(RenderIntegration::g_svs_eng_ver_rot_samples))
            : -1.0f));
        out.push_back(kv("engVerRotUnder1", RenderIntegration::g_svs_eng_ver_rot_under1));
        out.push_back(kv("engArmInjFrames", RenderIntegration::g_svs_eng_arm_inj_frames));
        for (int khc_i = 0; khc_i < RenderIntegration::KH_SVS_ENG_CAND; ++khc_i) {
            char khc_nm[24];
            sprintf_s(khc_nm, "engCandCopies%d", khc_i);
            out.push_back(kv(khc_nm, RenderIntegration::g_svs_eng_cand_copies[khc_i]));
        }
        out.push_back(kvf("engAgreeNdcBar", RenderIntegration::KH_SVS_ENG_AGREE_NDC));
        out.push_back(kv("engWideScans", RenderIntegration::g_svs_eng_wide_scans));
        out.push_back(kv("engWideBusy", RenderIntegration::g_svs_eng_wide_busy));
        out.push_back(kv("engWideBytes", static_cast<uint64_t>(RenderIntegration::g_svs_eng_wide_bytes)));
        out.push_back(kv("engWideN", static_cast<uint64_t>(RenderIntegration::g_svs_eng_wide_n)));
        out.push_back(kv("engWideNMax", static_cast<uint64_t>(RenderIntegration::g_svs_eng_wide_n_max)));
        out.push_back(kv("engWidePick", static_cast<uint64_t>(RenderIntegration::kh_svs_eng_cand_pick())));
        for (int khw_i = 0; khw_i < RenderIntegration::KH_SVS_ENG_WIDE_CAND; ++khw_i) {
            char khw_nm[24];
            sprintf_s(khw_nm, "engWideOff%d", khw_i);
            out.push_back(kv(khw_nm, static_cast<uint64_t>(
                RenderIntegration::g_svs_eng_wide_off[khw_i] < 0 ? 0xFFFFFFFFull
                    : static_cast<uint64_t>(RenderIntegration::g_svs_eng_wide_off[khw_i]))));
            sprintf_s(khw_nm, "engWidePass%d", khw_i);
            out.push_back(kv(khw_nm, RenderIntegration::g_svs_eng_wide_pass[khw_i]));
            sprintf_s(khw_nm, "engWideSeen%d", khw_i);
            out.push_back(kv(khw_nm, RenderIntegration::g_svs_eng_wide_seen[khw_i]));
        }
        // SETTLED, AND THE ANSWER IS THAT IT NEVER MATTERED. The 1.1 path is
        // kept because it is correct rather than merely harmless, and mode
        // 120 can be deleted in the next build that touches this struct.
        // Recorded here rather than quietly dropped: a concern raised and
        // then measured to zero is a result.
        out.push_back(kv("engSbCbOffRestores", RenderIntegration::g_sb_cb_off_restores));
        out.push_back(kv("engSbCbPlainRestores", RenderIntegration::g_sb_cb_plain_restores));
        out.push_back(kv("engSbCbOffMax",
            static_cast<uint64_t>(RenderIntegration::g_sb_cb_off_max)));
        // The fingerprint was transcribed literally and run offline against
        // capture 5's b2: it matches at byte 0 and rejects every other offset
        // for the right reason.
        for (int khe_i = 0; khe_i < RenderIntegration::KH_SVS_ENG_SLOTS; ++khe_i) {
            char khe_nm[24];
            sprintf_s(khe_nm, "engSl%dBytes", khe_i);
            out.push_back(kv(khe_nm, static_cast<uint64_t>(RenderIntegration::g_svs_eng_sl_bytes[khe_i])));
            sprintf_s(khe_nm, "engSl%dF0", khe_i);
            out.push_back(kvf(khe_nm, RenderIntegration::g_svs_eng_sl_f0[khe_i]));
            sprintf_s(khe_nm, "engSl%dF16", khe_i);
            out.push_back(kvf(khe_nm, RenderIntegration::g_svs_eng_sl_f16[khe_i]));
        }
        out.push_back(kv("engFpFail", static_cast<uint64_t>(RenderIntegration::g_svs_eng_fp_fail)));
        out.push_back(kv("engFpSlot", static_cast<uint64_t>(
            RenderIntegration::g_svs_eng_fp_slot < 0 ? 0xFFFFFFFFull
                : static_cast<uint64_t>(RenderIntegration::g_svs_eng_fp_slot))));
        out.push_back(kvf("engFpLen", RenderIntegration::g_svs_eng_fp_len));   // bar 0.01
        out.push_back(kvf("engFpLenBar", 0.01f));
        out.push_back(kvf("engFpOrth", RenderIntegration::g_svs_eng_fp_orth));   // bar 0.02
        out.push_back(kvf("engFpOrthBar", 0.02f));
        out.push_back(kvf("engFpWcol", RenderIntegration::g_svs_eng_fp_wcol));   // bar 1e-4
        out.push_back(kvf("engFpWcolBar", 1.0e-4f));
        out.push_back(kvf("engFpRow7", RenderIntegration::g_svs_eng_fp_row7));   // bar 1e-4
        out.push_back(kvf("engFpRow7Bar", 1.0e-4f));
        out.push_back(kvf("engFpDet", RenderIntegration::g_svs_eng_fp_det));   // want +1
        out.push_back(kvf("engFpTrace", RenderIntegration::g_svs_eng_fp_trace));   // 3 = identity
        out.push_back(kv("engIdentSeen", RenderIntegration::g_svs_eng_ident_seen));
        out.push_back(kv("svVolAgeLast", RenderIntegration::g_svs_vol_age_last));
        out.push_back(kv("svVolAgeMax", RenderIntegration::g_svs_vol_age_max));
        out.push_back(kv("svOmRtvMax", static_cast<uint64_t>(RenderIntegration::g_svs_om_rtv_max)));
        out.push_back(kv("svVolArmMax", static_cast<uint64_t>(RenderIntegration::g_svs_vol_arm_max)));
        // DARK-WINDOW RE-SEED CENSUS (black-mesh cold start).
        out.push_back(kv("blkDarkReseeds", RenderIntegration::g_blk_dark_reseeds));
        out.push_back(kvf("blkDarkReseedSl", RenderIntegration::g_blk_dark_reseed_sl));
        out.push_back(kvf("blkDarkReseedSlMin", RenderIntegration::g_blk_dark_reseed_sl_min));
        out.push_back(kvf("blkDarkReseedSlMax", RenderIntegration::g_blk_dark_reseed_sl_max));
        out.push_back(kvf("blkDarkReseedSlMean",
            RenderIntegration::g_blk_dark_reseeds > 0
                ? static_cast<float>(RenderIntegration::g_blk_dark_reseed_sl_sum /
                                     static_cast<double>(RenderIntegration::g_blk_dark_reseeds))
                : -1.0f));
        out.push_back(kvf("blkDarkReseedPendSl", RenderIntegration::g_blk_dark_reseed_pend_sl));
        out.push_back(kvf("blkDarkStdSl", RenderIntegration::g_blk_dark_std_sl));
        out.push_back(kvf("blkDarkReseedRatioMin", RenderIntegration::g_blk_dark_reseed_ratio_min));
        out.push_back(kvf("blkDarkReseedRatioMax", RenderIntegration::g_blk_dark_reseed_ratio_max));
        out.push_back(kvf("blkDarkFirstS", RenderIntegration::g_blk_dark_first_s));
        out.push_back(kvf("blkDarkLastS", RenderIntegration::g_blk_dark_last_s));
        out.push_back(kvf("blkDarkSpanS",
            RenderIntegration::g_blk_dark_first_s >= 0.0f
                ? RenderIntegration::g_blk_dark_last_s - RenderIntegration::g_blk_dark_first_s
                : -1.0f));
        {
            const float* sk = RenderIntegration::g_sky_probe.nb;
            // row 17: THE RULER (expect 0.25 / 0.75 / 3.5)
            out.push_back(kvf("atmSkyR17x", sk[68]));
            out.push_back(kvf("atmSkyR17y", sk[69]));
            out.push_back(kvf("atmSkyR17z", sk[70]));
            out.push_back(kvf("atmSkyR17w", sk[71]));
            // row 14: THE TARGET - expected (refAlt m, density, heightDecay,
            // ?)
            out.push_back(kvf("atmSkyR14x", sk[56]));
            out.push_back(kvf("atmSkyR14y", sk[57]));
            out.push_back(kvf("atmSkyR14z", sk[58]));
            out.push_back(kvf("atmSkyR14w", sk[59]));
            // rows 15/16: the extinction triples named in the sky anchor -
            // they bracket row 14 and make a mis-based window obvious
            out.push_back(kvf("atmSkyR15x", sk[60]));
            out.push_back(kvf("atmSkyR15y", sk[61]));
            out.push_back(kvf("atmSkyR15z", sk[62]));
            out.push_back(kvf("atmSkyR16x", sk[64]));
            out.push_back(kvf("atmSkyR16y", sk[65]));
            out.push_back(kvf("atmSkyR16z", sk[66]));
            // row 13 ClearColor, row 7 the SECOND fog color the engine
            // composites through the row-17 gradient (fogTgtR/G/B already
            // publish row 1 = FogColor.rgb;.w joins them here)
            out.push_back(kvf("atmSkyR13x", sk[52]));
            out.push_back(kvf("atmSkyR13y", sk[53]));
            out.push_back(kvf("atmSkyR13z", sk[54]));
            out.push_back(kvf("atmSkyR13w", sk[55]));
            out.push_back(kvf("atmSkyR7x", sk[28]));
            out.push_back(kvf("atmSkyR7y", sk[29]));
            out.push_back(kvf("atmSkyR7z", sk[30]));
            out.push_back(kvf("atmSkyR7w", sk[31]));
            out.push_back(kvf("atmSkyR1w", sk[7]));
            out.push_back(kvf("atmSkyR0x", sk[0]));
            out.push_back(kvf("atmSkyR0y", sk[1]));
            out.push_back(kvf("atmSkyR0z", sk[2]));
        }
        {
            const float* bl = RenderIntegration::g_light_probe.nb;
            // row 10: x = height decay (the DECAY ANCHOR the locator locks
            // on, hence lightLocMeta 40), y = density (already fogEngine.x)
            out.push_back(kvf("atmBlkR10x", bl[40]));
            out.push_back(kvf("atmBlkR10y", bl[41]));
            // row 11: x is the engine's FOG MODE selector in the shader
            // switch (1/3 = layered model, 2 = the variant, else no fog)
            out.push_back(kvf("atmBlkR11x", bl[44]));
            out.push_back(kvf("atmBlkR11y", bl[45]));
            out.push_back(kvf("atmBlkR11z", bl[46]));
            out.push_back(kvf("atmBlkR11w", bl[47]));
            // z and w have never been read. row 12: x = fog end
            // (fogEngine.y), y = inverse range (fogEngine.z), z = LAYER
            // ALTITUDE, w = below-layer extinction.
            out.push_back(kvf("atmBlkR12x", bl[48]));
            out.push_back(kvf("atmBlkR12y", bl[49]));
            out.push_back(kvf("atmBlkR12z", bl[50]));
            out.push_back(kvf("atmBlkR12w", bl[51]));
            // row 13: y is the layer-altitude OFFSET the shader adds to row
            // 12 z before the branch (layerY = R12z + R13y)
            out.push_back(kvf("atmBlkR13x", bl[52]));
            out.push_back(kvf("atmBlkR13y", bl[53]));
            out.push_back(kvf("atmBlkR13z", bl[54]));
            out.push_back(kvf("atmBlkR13w", bl[55]));
        }
        out.push_back(kv("skyBindReads", RenderIntegration::g_skybind_reads));
        out.push_back(kv("skyBindHits", RenderIntegration::g_skybind_hits));
        out.push_back(kv("skyBindMinBw", static_cast<uint64_t>(RenderIntegration::g_skybind_minbw)));
        out.push_back(kv("skyBindMaxBw", static_cast<uint64_t>(RenderIntegration::g_skybind_maxbw)));
        out.push_back(kv("skyBindSlots", static_cast<uint64_t>(RenderIntegration::g_skybind_slots)));
        out.push_back(kv("skyBindOff1", static_cast<uint64_t>(RenderIntegration::g_skybind_off1)));
        out.push_back(kv("skyBindMaxBwVs", static_cast<uint64_t>(RenderIntegration::g_skybind_maxbw_vs)));
        out.push_back(kv("viewBindScans", RenderIntegration::g_viewbind_scans));
        out.push_back(kv("stageTotal", static_cast<uint64_t>(RenderIntegration::g_stage_total)));
        out.push_back(kv("stageRejVis", static_cast<uint64_t>(RenderIntegration::g_stage_rej_vis)));
        out.push_back(kv("recvTermSkips", RenderIntegration::g_recv_term_skips));
        out.push_back(kv("recvStreamSkips", RenderIntegration::g_recv_stream_skips));
        out.push_back(kv("recvWipes", RenderIntegration::g_recv_wipes));
        out.push_back(kv("sunJumpRefused", RenderIntegration::g_sun_jump_refused));
        out.push_back(kv("sunJumpStreamRefused", RenderIntegration::g_sun_jump_stream_refused));
        out.push_back(kv("viewRelockForced", RenderIntegration::g_view_relock_forced));
        // Two paths can clear view_src_valid and only one of them was ever
        // counted, so a lock that died on the health path was
        // indistinguishable in a dump from a lock that was never taken.
        // viewRelockErMin is the discriminator for 2A: it scores the
        // REMEMBERED source against the bridge for as long as the lock stays
        // dead.
        out.push_back(kv("viewDropForced", RenderIntegration::g_view_drop_forced));
        out.push_back(kv("viewDropHealth", RenderIntegration::g_view_drop_health));
        out.push_back(kv("viewDropPath", static_cast<uint64_t>(RenderIntegration::g_view_drop_path)));
        // (age_s is declared further down this function; the drop stamp is
        // aged inline rather than moving a lane out of its family.)
        out.push_back(kvf("viewDropAgeS",
            RenderIntegration::g_view_drop_ms == 0 ? -1.0f :
                static_cast<float>(RenderIntegration::steady_now_ms() -
                                   RenderIntegration::g_view_drop_ms) * 0.001f));
        out.push_back(kvf("viewDropRd", RenderIntegration::g_view_drop_rd));
        out.push_back(kvf("viewDropMd", RenderIntegration::g_view_drop_md));
        out.push_back(kvf("viewDropBar", RenderIntegration::g_view_drop_bar));
        out.push_back(kvf("viewRelockErMin", RenderIntegration::g_view_relock_er_min));
        out.push_back(kvf("viewRelockTmagMin", RenderIntegration::g_view_relock_tmag_min));
        out.push_back(kv("viewRelockProbes", RenderIntegration::g_view_relock_probes));
        // viewLockSameSlot is the whole question: 1 = the ring came back to
        // the identity the guard dropped, so the guard ate a correct lock; 2
        // = it came back somewhere else, so the guard was right and the
        // defect is purely how long re-acquisition takes.
        out.push_back(kv("viewLockSameSlot", static_cast<uint64_t>(RenderIntegration::g_view_lock_same_slot)));
        out.push_back(kv("viewLockAcqPath", static_cast<uint64_t>(RenderIntegration::g_view_lock_acq_path)));
        out.push_back(kvf("viewLockDeadS", RenderIntegration::g_view_lock_dead_s));
        out.push_back(kvf("viewLockDeadMaxS", RenderIntegration::g_view_lock_dead_max_s));
        // 2A is now scoped to how long re-acquisition takes. ringBestMin says
        // whether the correct candidate was even available during the dead
        // window; ringConfirmHolds and ringConfirmChurn say whether the
        // two-boundary confirm gate is what held it out, and churn is the one
        // that can hold it out without bound.
        out.push_back(kvf("viewRingBestMin", RenderIntegration::g_view_ring_best_min));
        out.push_back(kv("viewRingConfirmHolds", RenderIntegration::g_view_ring_confirm_holds));
        out.push_back(kv("viewRingConfirmChurn", RenderIntegration::g_view_ring_confirm_churn));
        out.push_back(kv("viewRingTmagRejects", RenderIntegration::g_view_ring_tmag_rejects));
        out.push_back(kvf("viewRingBestTmag", RenderIntegration::g_view_ring_best_tmag));
        out.push_back(kv("dropHystHolds", RenderIntegration::g_view_drop_hyst_holds));
        out.push_back(kv("dropHystForced", RenderIntegration::g_view_drop_hyst_forced));
        out.push_back(kvf("dropHystRd", RenderIntegration::g_view_drop_hyst_rd));
        out.push_back(kvf("dropHystBar", RenderIntegration::g_view_drop_hyst_bar));
        out.push_back(kvf("dropHystMaxMs", RenderIntegration::g_view_drop_hyst_max_ms));
        out.push_back(kv("relockWideArms", RenderIntegration::g_view_relock_wide_arms));
        out.push_back(kv("relockWideTakes", RenderIntegration::g_view_relock_wide_takes));
        out.push_back(kvf("relockWideEr", RenderIntegration::g_view_relock_wide_er));
        out.push_back(kv("pubLateFrames", static_cast<uint64_t>(RenderIntegration::g_pub_late_frames)));
        out.push_back(kv("pubLateDry", static_cast<uint64_t>(RenderIntegration::g_pub_late_dry)));
        out.push_back(kv("pubLateRejected", static_cast<uint64_t>(RenderIntegration::g_pub_late_rejected)));
        out.push_back(kv("pubEarlyFrames", static_cast<uint64_t>(RenderIntegration::g_pub_early_frames)));
        out.push_back(kv("pubAccepts", RenderIntegration::g_pub_accepts));
        out.push_back(kv("pubRejTmag", RenderIntegration::g_pub_rej_tmag));
        out.push_back(kv("pubRejFamily", RenderIntegration::g_pub_rej_family));
        out.push_back(kv("pubRejCold", RenderIntegration::g_pub_rej_cold));
        out.push_back(kv("pubBindAccepts", RenderIntegration::g_pub_bind_accepts));
        out.push_back(kvf("pubFirstD", RenderIntegration::g_pub_first_d_pub));
        out.push_back(kvf("pubRejFirstD", RenderIntegration::g_pub_rej_first_d_pub));
        out.push_back(kv("pubSlotHits", static_cast<uint64_t>(RenderIntegration::g_pub_slot_hits_pub)));
        out.push_back(kv("pubPreAccept", static_cast<uint64_t>(RenderIntegration::g_pub_hits_pre_accept_pub)));
        // Read pubAltLateEarly against pubLateFrames: pubAltLateEarly ~=
        // pubLateFrames -> on the late cycles a view was already present at
        // ANOTHER offset inside the read window. The arm can move; widen the
        // publish key to that offset and the flicker goes with it. pubAltOff
        // names the offset. THE remaining question, and the last one task 1
        // has.
        out.push_back(kv("pubAltLateEarly", static_cast<uint64_t>(RenderIntegration::g_pub_alt_late_early)));
        out.push_back(kv("pubAltLateAny", static_cast<uint64_t>(RenderIntegration::g_pub_alt_late_any)));
        out.push_back(kv("pubAltFrames", static_cast<uint64_t>(RenderIntegration::g_pub_alt_frames)));
        out.push_back(kv("pubAltHits", RenderIntegration::g_pub_alt_hits));
        out.push_back(kvf("pubAltFirstD", RenderIntegration::g_pub_alt_first_d_pub));
        out.push_back(kv("pubAltOff", static_cast<uint64_t>(RenderIntegration::g_pub_alt_off_pub)));
        out.push_back(kvf("pubAltEr", RenderIntegration::g_pub_alt_er));
        // setRenderDebug 42 = carry OFF (single paint, verbatim).
        // fireCarryPaints > 0 and the blink SURVIVES -> the carry never
        // reaches the reader and the whole carry family is dead.
        out.push_back(kv("bandPickChanges", RenderIntegration::g_band_pick_changes));
        out.push_back(kv("bandPickNone", RenderIntegration::g_band_pick_none));
        out.push_back(kv("bandPickNoneRuns", RenderIntegration::g_band_pick_none_runs));
        // the winner had a PROVISIONAL vcol (sm from this resolve, view from
        // a publish up to 50 ms old). how each seal got its vcol.
        // bandSealPending should be ~0; anything there is a seal still open
        // to a cross-frame completion.
        out.push_back(kv("bandSealSame", RenderIntegration::g_band_seal_same));
        out.push_back(kv("bandSealBridge", RenderIntegration::g_band_seal_bridge));
        out.push_back(kv("bandSealPending", RenderIntegration::g_band_seal_pending));
        out.push_back(kv("bandPendConsumed", RenderIntegration::g_band_pend_consumed));
        out.push_back(kv("bandPendRuns", RenderIntegration::g_band_pend_runs));
        out.push_back(kv("bandPendMax", static_cast<uint64_t>(RenderIntegration::g_band_pend_max)));
        out.push_back(kvf("bandPendAgeMs", RenderIntegration::g_band_pend_age_ms));
        out.push_back(kvf("bandPendAgeMaxMs", RenderIntegration::g_band_pend_age_max_ms));
        // bandCompleteCross is a completion of a seal captured before this
        // cycle's boundary - frame-N sm paired with a frame-N+1 view, the
        // offset. If it reads ~0 the model is wrong and setRenderDebug 45
        // must not be promoted.
        out.push_back(kv("bandCompleteCross", RenderIntegration::g_band_complete_cross));
        out.push_back(kv("bandCompleteSame", RenderIntegration::g_band_complete_same));
        out.push_back(kvf("bandCompleteSkewDeg", RenderIntegration::g_band_complete_skew_deg));
        out.push_back(kvf("bandCompleteSkewMaxDeg", RenderIntegration::g_band_complete_skew_max));
        out.push_back(kvf("bandCompleteSkewM", RenderIntegration::g_band_complete_skew_m));
        out.push_back(kvf("bandCompleteSkewMaxM", RenderIntegration::g_band_complete_skew_max_m));
        out.push_back(kv("bandPickNoneMax", static_cast<uint64_t>(RenderIntegration::g_band_pick_none_max)));
        out.push_back(kv("bandPickCycles", RenderIntegration::g_band_pick_cycles));
        out.push_back(kv("bandPickEvals", RenderIntegration::g_band_pick_evals));
        out.push_back(kv("bandPick", static_cast<uint64_t>(RenderIntegration::g_band_pick_prev)));
        out.push_back(kvf("bandPickAgeMs", RenderIntegration::g_band_pick_age_ms));
        out.push_back(kvf("bandPickAgeMaxMs", RenderIntegration::g_band_pick_age_max_ms));
        out.push_back(kvf("bandPickVz", RenderIntegration::g_band_pick_vz));
        out.push_back(kv("castBridgeArms", RenderIntegration::g_cast_bridge_arms));
        out.push_back(kv("castBridgeFirst", RenderIntegration::g_cast_bridge_first));
        out.push_back(kv("flickerTransitions", RenderIntegration::g_fire_flicker_transitions));
        out.push_back(kv("paintPath2Runs", RenderIntegration::g_fire_path2_runs));
        out.push_back(kv("carrySkipsMotion", RenderIntegration::g_fire_carry_skips_motion));
        out.push_back(kv("carrySkipsGauge", RenderIntegration::g_fire_carry_skips_gauge));
        out.push_back(kvf("fireCarryPx", RenderIntegration::g_fire_carry_px));
        out.push_back(kvf("fireCarryPxMax", RenderIntegration::g_fire_carry_px_max));
        out.push_back(kvf("fireCarryPxSkip", RenderIntegration::g_fire_carry_px_skip));
        out.push_back(kv("ringAdmits", RenderIntegration::g_ring_admits));
        out.push_back(kv("ringHunts", RenderIntegration::g_ring_hunts));
        out.push_back(kv("ringDryHunts", RenderIntegration::g_ring_dry_hunts));
        out.push_back(kv("ringHuntNMax", static_cast<uint64_t>(RenderIntegration::g_ring_hunt_n_max)));
        out.push_back(kv("ringBestTakes", RenderIntegration::g_ring_best_takes));
        out.push_back(kv("ringBestLocks", RenderIntegration::g_ring_best_locks));
        out.push_back(kvf("ringBestTakeEr", RenderIntegration::g_ring_best_take_er));
        out.push_back(kv("viewBindInjSkips", RenderIntegration::g_viewbind_inj_skips));
        out.push_back(kv("lockWipes", RenderIntegration::g_lock_wipes));
        out.push_back(kv("stageRejExp", static_cast<uint64_t>(RenderIntegration::g_stage_rej_exp)));
        out.push_back(kv("cascBindScans", RenderIntegration::g_cascbind_scans));
        out.push_back(kv("skyBindOffsSeen", RenderIntegration::g_skybind_offs_seen));

        out.push_back(kv("castFrozenFires", RenderIntegration::g_cast_frozen_fires));
        out.push_back(kv("castRtResolveTrue", RenderIntegration::g_rt_resolve_true));
        out.push_back(kv("castRtResolveFalse", RenderIntegration::g_rt_resolve_false));
        out.push_back(kv("castRtHalfAccepts", RenderIntegration::g_rt_half_accepts));
        out.push_back(kv("sweepGapResets", RenderIntegration::g_sweep_gap_resets));
        out.push_back(kv("injGuardOff", RenderIntegration::g_inj_guard_off));
        out.push_back(kv("injFxDimBaseline", RenderIntegration::g_inj_fx_dim_baseline));
        // THE 8.5 GAUGE: injection draws by PS signature (arb = SV_Depth,
        // late-Z; earlyZ = no SV_Depth), read against compositeMeshes.
        out.push_back(kv("injPsArb", RenderIntegration::g_inj_ps_arb_draws));
        out.push_back(kv("injPsEarlyZ", RenderIntegration::g_inj_ps_earlyz_draws));
        // KH_MESH_OWNER_PREPASS (mode 438): ownFails is the arming lane -
        // read it first; ownSkips = admitted objects that test but never own.
        out.push_back(kv("ownFrames", RenderIntegration::g_own_frames));
        out.push_back(kv("ownDraws", RenderIntegration::g_own_draws));
        out.push_back(kv("ownSkips", RenderIntegration::g_own_skips));
        out.push_back(kv("ownFails", RenderIntegration::g_own_fails));
        // KH_CB_MIRROR_GAUGE: the HLSL CBObj/CBFrame sizes reflected off the
        // PSMain blob against the C++ split. Read Refl first (1 = the lanes
        // are live); Bad true is a desynchronised mirror and every CB lane
        // past the divergence is garbage whatever the screen shows.
        out.push_back(kv("cbMirrorObjB", RenderIntegration::g_cb_mirror_obj_b));
        out.push_back(kv("cbMirrorFrameB", RenderIntegration::g_cb_mirror_frame_b));
        out.push_back(kv("cbMirrorRefl", RenderIntegration::g_cb_mirror_refl));
        out.push_back(kv("cbMirrorBad", RenderIntegration::g_cb_mirror_bad ? 1 : 0));
        {   // OCCLUSION-GUARD OVERHAUL diagnostics + config echo
            out.push_back(kv("snapSerial", RenderIntegration::g_snap_serial));
            out.push_back(kv("snapFails", RenderIntegration::g_snap_fails));
            out.push_back(kv("snapConsumed", RenderIntegration::g_snap_consumed));
            out.push_back(kvf("snapAgeNowMs", RenderIntegration::g_snap_ms != 0
                ? static_cast<float>(RenderIntegration::steady_now_ms() - RenderIntegration::g_snap_ms)
                : -1.0f));
            out.push_back(kvf("snapAgeInjMs", RenderIntegration::g_snap_age_last));
            out.push_back(kvf("snapAgeInjMaxMs", RenderIntegration::g_snap_age_max));
            // KH_ARB_JITTER_CENSUS (instrument only)
            out.push_back(kv("arbJitFrames", RenderIntegration::g_arb_jit_frames));
            out.push_back(kv("arbJitOverTol", RenderIntegration::g_arb_jit_over_tol));
            out.push_back(kvf("arbJitPairM", RenderIntegration::g_arb_jit_pair_m));
            out.push_back(kvf("arbJitCamM", RenderIntegration::g_arb_jit_cam_m));
            out.push_back(kvf("arbJitVertM", RenderIntegration::g_arb_jit_vert_m));
            out.push_back(kvf("arbJitVertMaxM", RenderIntegration::g_arb_jit_vert_max_m));
            out.push_back(kvf("arbJitVertMeanM", RenderIntegration::g_arb_jit_frames
                ? static_cast<float>(RenderIntegration::g_arb_jit_vert_sum /
                                     static_cast<double>(RenderIntegration::g_arb_jit_frames)) : -1.0f));
            out.push_back(kvf("arbTolM", RenderIntegration::g_arb_tol_m));
            out.push_back(kvf("arbSnapNearM", RenderIntegration::g_arb_snap_near_m));
            out.push_back(kvf("arbInjNearM", RenderIntegration::g_arb_inj_near_m));
            out.push_back(kv("snapPairSrc", static_cast<uint64_t>(RenderIntegration::g_snap_pair_src)));
            // KH_SNAP_REPROJECT arming lanes (mode 330 reverts)
            out.push_back(kv("snapReprojArms", RenderIntegration::g_snap_reproj_arms));
            out.push_back(kv("snapReprojStands", RenderIntegration::g_snap_reproj_stands));
            // (Campaign diagnostics - jitter probe, config echo, GPU pixel
            // autopsy - retired with the settled model; the notes doc records
            // their findings.)
            out.push_back(kv("thmValid", RenderIntegration::g_thm_valid ? 1u : 0u));
            out.push_back(kv("thmW", RenderIntegration::g_thml_w));
            out.push_back(kv("thmH", RenderIntegration::g_thml_h));
            out.push_back(kvf("thmCell", RenderIntegration::g_thml_cell));
            out.push_back(kv("thmFilled", static_cast<uint64_t>(RenderIntegration::g_thm_filled)));
            out.push_back(kv("thmUploads", RenderIntegration::g_thm_uploads));
            out.push_back(kv("thmAutoState", static_cast<uint64_t>(RenderIntegration::g_thm_auto_state)));
            out.push_back(kv("thmAutoSrc", static_cast<uint64_t>(RenderIntegration::g_thm_auto_src)));
            out.push_back(kv("thmAutoSamples", RenderIntegration::g_thm_auto_samples));
            out.push_back(kv("injDpValid", RenderIntegration::g_inj_dp_valid ? 1u : 0u));
            out.push_back(kvf("injDpM22", RenderIntegration::g_inj_dp[0]));
            out.push_back(kvf("injDpM32", RenderIntegration::g_inj_dp[1]));
            out.push_back(kvf("injDpVpMin", RenderIntegration::g_inj_dp[2]));
            out.push_back(kvf("injDpVpMax", RenderIntegration::g_inj_dp[3]));
        }
        out.push_back(kv("flushFallbackDraws", RenderIntegration::g_flush_fallback_draws));
        out.push_back(kv("flushLatchPvs", RenderIntegration::g_flush_latch_pvs));
        out.push_back(kv("flushPvRepairs", RenderIntegration::g_flush_pv_repairs));
        out.push_back(kv("flushRepaintSaves", RenderIntegration::g_flush_repaint_saves));
        out.push_back(kv("flushAnomalyCarries", RenderIntegration::g_flush_anomaly_carries));
        out.push_back(kv("castArmsLostMiss", RenderIntegration::g_mask.arms_lost_miss));
        out.push_back(kv("sunMapSkips", RenderIntegration::g_sun_map_skips));
        auto age_s = [](uint64_t ms) {
            return ms == 0 ? -1.0f :
                static_cast<float>(RenderIntegration::steady_now_ms() - ms) * 0.001f;
        };
        out.push_back(kvf("flAgeFallbackS", age_s(RenderIntegration::g_fl_fallback_ms)));
        out.push_back(kvf("flAgeAnomSkipS", age_s(RenderIntegration::g_fl_anom_skip_ms)));
        out.push_back(kv("ccPostFlushRedraws", RenderIntegration::g_cc_postflush_redraws));
        out.push_back(kv("ccPfLastDraws", static_cast<uint64_t>(RenderIntegration::g_cc_pf_last_draws)));
        out.push_back(kvf("ccPfLastAgeS", age_s(RenderIntegration::g_cc_pf_last_ms)));
        out.push_back(kv("missFrames", RenderIntegration::g_ms_frames));
        out.push_back(kvf("missLastNear", RenderIntegration::g_ms_near));
        out.push_back(kv("flushSlotKeeps", RenderIntegration::g_flush_slot_keeps));
        out.push_back(kv("flushInjEncodes", RenderIntegration::g_flush_inj_encodes));
        out.push_back(kv("flushInjPairHolds", RenderIntegration::g_flush_inj_pair_holds));
        out.push_back(kv("flushPubFarRejects", RenderIntegration::g_flush_pub_far_rejects));
        out.push_back(kv("farKeepMeshDraws", RenderIntegration::g_farkeep_mesh_draws));
        out.push_back(kv("fkVetoFills", RenderIntegration::g_fk_veto_fills));
        out.push_back(kv("fkVetoLastN", static_cast<uint64_t>(RenderIntegration::g_fk_veto_last_n)));
        out.push_back(kv("blkCollapseHolds", RenderIntegration::g_blk_collapse_holds));
        out.push_back(kv("blkBlankSkips", RenderIntegration::g_blk_blank_skips));   // blank-guard census
        out.push_back(kv("blkAnchorRejects", RenderIntegration::g_blk_anchor_rejects));
        out.push_back(kv("blkAnchorSnaps", RenderIntegration::g_blk_anchor_snaps));
        out.push_back(kv("blkSmallNfSkips", RenderIntegration::g_blk_smallnf_skips));
        out.push_back(kvf("blkAnchorSunL", RenderIntegration::g_light_probe.anch_sun_l));
        out.push_back(kvf("blkAnchorAmbL", RenderIntegration::g_light_probe.anch_amb_l));
        out.push_back(kv("blkStarvedAdopts", RenderIntegration::g_blk_starved_adopts));   // starvation census
        // the capture-side pending slot itself (the publish twin's
        // blkPendAgeS already exists) - the next stuck log shows WHICH flavor
        // is pending and whether it is maturing or thrashing.
        out.push_back(kvf("stdPendAgeS", RenderIntegration::g_light_probe.pend_t >= 0.0f
            ? RenderIntegration::effect_time_seconds() - RenderIntegration::g_light_probe.pend_t
            : -1.0f));
        out.push_back(kvf("stdPendAmbLum", RenderIntegration::g_light_probe.pend_amb_l));
        out.push_back(kvf("stdPendSunLum", RenderIntegration::g_light_probe.pend_sun_l));
        out.push_back(kv("blkRegimeAdopts", RenderIntegration::g_light_probe.regime_adopts));
        out.push_back(kv("blkRegimeRejects", RenderIntegration::g_light_probe.regime_rejects));
        out.push_back(kv("blkJumpAdopts", RenderIntegration::g_blk_jump_adopts));
        out.push_back(kv("khBuildTag", static_cast<uint64_t>(RenderIntegration::KH_BUILD_TAG)));
        out.push_back(kv("nearzGapDraws", RenderIntegration::g_nearz_gap_draws));
        // mode 193 or disarmed. The near population is several coexisting
        // constants, not one value that moves - read all four classes
        // together, never nearCls0 alone. THE FOOTPRINT'S OWN ENCODE.
        {
            uint64_t khnc_v = 0;
            for (int khnc_i = 0; khnc_i < 4; ++khnc_i) {
                const float khnc_n = RenderIntegration::kh_near_class(khnc_i, &khnc_v);
                out.push_back(kvf(("nearCls" + std::to_string(khnc_i)).c_str(), khnc_n));
                out.push_back(kv(("nearClsN" + std::to_string(khnc_i)).c_str(), khnc_v));
            }
        }
        out.push_back(kv("injNzDraws", RenderIntegration::g_inj_nz_draws));
        // kvf, NOT kv - kv takes uint64_t and truncated this float to 0 in
        // both dumps, a dead gauge shipped in the same build that was warned
        // about dead gauges.
        out.push_back(kvf("injNzFloor", RenderIntegration::g_inj_nz_floor));
        out.push_back(kvf("nearzNearEst", RenderIntegration::g_nearz_last_near));
        out.push_back(kvf("nearzGapFloor", RenderIntegration::g_nearz_last_floor));
        {
            // Literal name table rather than snprintf: <cstdio> is not
            // guaranteed in scope here and the lane names are a dump
            // contract, so they are spelled out.
            static const char* const khpc_names[8][5] = {
                { "projPair0Near", "projPair0Far", "projPair0M22", "projPair0M32", "projPair0Hits" },
                { "projPair1Near", "projPair1Far", "projPair1M22", "projPair1M32", "projPair1Hits" },
                { "projPair2Near", "projPair2Far", "projPair2M22", "projPair2M32", "projPair2Hits" },
                { "projPair3Near", "projPair3Far", "projPair3M22", "projPair3M32", "projPair3Hits" },
                { "projPair4Near", "projPair4Far", "projPair4M22", "projPair4M32", "projPair4Hits" },
                { "projPair5Near", "projPair5Far", "projPair5M22", "projPair5M32", "projPair5Hits" },
                { "projPair6Near", "projPair6Far", "projPair6M22", "projPair6M32", "projPair6Hits" },
                { "projPair7Near", "projPair7Far", "projPair7M22", "projPair7M32", "projPair7Hits" },
            };

            for (int khpc_i = 0; khpc_i < RenderIntegration::KH_PROJ_CENSUS_N && khpc_i < 8; ++khpc_i) {
                const auto& khpc_p = RenderIntegration::g_proj_census_pub[khpc_i];
                out.push_back(kvf(khpc_names[khpc_i][0], khpc_p.nearp));
                out.push_back(kvf(khpc_names[khpc_i][1], khpc_p.farp));
                out.push_back(kvf(khpc_names[khpc_i][2], khpc_p.m22));
                out.push_back(kvf(khpc_names[khpc_i][3], khpc_p.m32));
                out.push_back(kv(khpc_names[khpc_i][4], static_cast<uint64_t>(khpc_p.hits)));
            }
        }
        out.push_back(kv("projPairEvicts", static_cast<uint64_t>(RenderIntegration::g_proj_census_evicts)));
        out.push_back(kvf("sliceSeenNear", RenderIntegration::g_slice_seen_near));
        out.push_back(kvf("sliceSeenFar", RenderIntegration::g_slice_seen_far));
        out.push_back(kvf("sliceSeenFarMin", RenderIntegration::g_slice_seen_far_min));
        out.push_back(kvf("sliceSeenFarMax", RenderIntegration::g_slice_seen_far_max));
        out.push_back(kvf("sliceSeenAgeS", RenderIntegration::g_slice_seen_ms == 0 ? -1.0f
            : (RenderIntegration::steady_now_ms() - RenderIntegration::g_slice_seen_ms) / 1000.0f));
        out.push_back(kv("sliceSeenCount", RenderIntegration::g_slice_seen_count));
        out.push_back(kvf("lightLocStdAgeS", RenderIntegration::g_light_probe.last_std_time < 0.0f ? -1.0f
            : RenderIntegration::effect_time_seconds() - RenderIntegration::g_light_probe.last_std_time));
        out.push_back(kv("keepStampRejects", RenderIntegration::g_keep_stamp_rejects));
        out.push_back(kv("keepStaleSkips", RenderIntegration::g_keep_stale_skips));
        out.push_back(kvf("castArmLostAgeS", age_s(RenderIntegration::g_cast_arm_lost_ms)));
        out.push_back(kv("sunLocalCount", static_cast<uint64_t>(RenderIntegration::g_sun_local_count)));
        out.push_back(kvf("sunMapHalfDiag", sqrtf(
            RenderIntegration::g_sun_map_bounds[3] * RenderIntegration::g_sun_map_bounds[3] +
            RenderIntegration::g_sun_map_bounds[4] * RenderIntegration::g_sun_map_bounds[4] +
            RenderIntegration::g_sun_map_bounds[5] * RenderIntegration::g_sun_map_bounds[5])));
        out.push_back(kv("sunHeroValid", RenderIntegration::g_sun2_map_valid ? 1ull : 0ull));
        out.push_back(kv("sunHeroCasters", RenderIntegration::g_sun2_casters));
        out.push_back(kv("sunHeroRenders", RenderIntegration::g_sun2_renders));
        out.push_back(kvf("sunHeroHalfDiag", RenderIntegration::g_sun2_half_diag));
        // 1 = the band never fitted at all, in which case NOTHING about this
        // axis was tested whatever the screen shows. KH_BAND_SUN_REACH - THE
        // ARMING LANES. Each band's sun-ward reach in metres, floored at its
        // own KH_SUN_*_DEPTH (12 / 48 / 192).
        out.push_back(kvf("sunHeroReach", RenderIntegration::g_sun_band_reach[0]));
        out.push_back(kvf("sunMidReach", RenderIntegration::g_sun_band_reach[1]));
        out.push_back(kvf("sunOutReach", RenderIntegration::g_sun_band_reach[2]));
        // KH_SUN_FAR_BAND lanes: halfDiag must read == sunRangeM (the window
        // IS the range); valid 1 with renders tracking flushes is the
        // engagement read; reach floors at 6x the range.
        out.push_back(kv("sunFarValid", RenderIntegration::g_sun5_map_valid ? 1ull : 0ull));
        out.push_back(kvf("sunFarHalfDiag", RenderIntegration::g_sun5_half_diag));
        out.push_back(kv("sunFarRenders", RenderIntegration::g_sun5_renders));
        out.push_back(kv("sunFarCasters", RenderIntegration::g_sun5_casters));
        out.push_back(kvf("sunFarReach", RenderIntegration::g_sun_band_reach[3]));
        out.push_back(kv("sunBandReachTakes", RenderIntegration::g_sun_band_reach_takes));
        out.push_back(kv("sunHeroFitWhy", RenderIntegration::g_sun_fit_why[0]));
        out.push_back(kv("sunMidFitWhy",  RenderIntegration::g_sun_fit_why[1]));
        out.push_back(kv("sunOutFitWhy",  RenderIntegration::g_sun_fit_why[2]));
        out.push_back(kv("sunFarFitWhy",  RenderIntegration::g_sun_fit_why[3]));
        out.push_back(kv("sunHeroFitN",   RenderIntegration::g_sun_fit_n[0]));
        out.push_back(kv("sunMidFitN",    RenderIntegration::g_sun_fit_n[1]));
        out.push_back(kv("sunOutFitN",    RenderIntegration::g_sun_fit_n[2]));
        out.push_back(kv("sunFarFitN",    RenderIntegration::g_sun_fit_n[3]));
        // THE OVERSIZE GAUGES, per tier, STAMPED AT THE ADMISSION LOOP -
        // which every call runs before any exit, so unlike khsh_out_reach and
        // khsh_out_casters these are never stale from an earlier frame's
        // pose. THIS IS THE MUTUAL-INTERACTION TERM and it has never had a
        // lane. ErMax - largest enclosing radius ADMITTED to that tier,
        // metres.
        out.push_back(kvf("sunHeroErMax", RenderIntegration::g_sun_fit_er_max[0]));
        out.push_back(kvf("sunMidErMax",  RenderIntegration::g_sun_fit_er_max[1]));
        out.push_back(kvf("sunOutErMax",  RenderIntegration::g_sun_fit_er_max[2]));
        out.push_back(kvf("sunFarErMax",  RenderIntegration::g_sun_fit_er_max[3]));
        out.push_back(kvf("sunHeroFwdM",  RenderIntegration::g_sun_fit_fwd[0]));
        out.push_back(kvf("sunMidFwdM",   RenderIntegration::g_sun_fit_fwd[1]));
        out.push_back(kvf("sunOutFwdM",   RenderIntegration::g_sun_fit_fwd[2]));
        out.push_back(kvf("sunFarFwdM",   RenderIntegration::g_sun_fit_fwd[3]));
        // sunHero/Mid/Out/FarRefus and sunErTightens are RETIRED with the
        // mechanisms they measured.
        out.push_back(kvf("sunLadderCasterR", RenderIntegration::g_sun_ladder_caster_r));
        // They exist because the leading account of the oversized-mesh
        // self-shadow acne is an ARITHMETIC claim about one expression and
        // that expression has never been read off a machine. READ THEM
        // AGAINST sunHero/Mid/Out/FarValid: a band that never ran reports 0
        // in all four and is not evidence of anything.
        out.push_back(kvf("sunHeroTexM",    RenderIntegration::g_sun_fit_tex[0]));
        out.push_back(kvf("sunMidTexM",     RenderIntegration::g_sun_fit_tex[1]));
        out.push_back(kvf("sunOutTexM",     RenderIntegration::g_sun_fit_tex[2]));
        out.push_back(kvf("sunFarTexM",     RenderIntegration::g_sun_fit_tex[3]));
        out.push_back(kvf("sunHeroBiasFlM", RenderIntegration::g_sun_fit_bfl[0]));
        out.push_back(kvf("sunMidBiasFlM",  RenderIntegration::g_sun_fit_bfl[1]));
        out.push_back(kvf("sunOutBiasFlM",  RenderIntegration::g_sun_fit_bfl[2]));
        out.push_back(kvf("sunFarBiasFlM",  RenderIntegration::g_sun_fit_bfl[3]));
        out.push_back(kvf("sunHeroBiasM",   RenderIntegration::g_sun_fit_bias[0]));
        out.push_back(kvf("sunMidBiasM",    RenderIntegration::g_sun_fit_bias[1]));
        out.push_back(kvf("sunOutBiasM",    RenderIntegration::g_sun_fit_bias[2]));
        out.push_back(kvf("sunFarBiasM",    RenderIntegration::g_sun_fit_bias[3]));
        // Relatches tracking the band render count means the latch is
        // thrashing and nothing was stabilised whatever the screen shows;
        // both reading 0 means no band was ever stretched above its floor at
        // this pose and the latch was never exercised - which is the NORMAL
        // reading for an ordinary-sized caster and is not a fault.
        out.push_back(kv("sunFwdHolds",     RenderIntegration::g_sun_fwd_holds));
        out.push_back(kv("sunFwdRelatches", RenderIntegration::g_sun_fwd_relatches));
        out.push_back(kvf("sunHeroFwdHeldM", RenderIntegration::g_sun_fwd_held[0]));
        out.push_back(kvf("sunMidFwdHeldM",  RenderIntegration::g_sun_fwd_held[1]));
        out.push_back(kvf("sunOutFwdHeldM",  RenderIntegration::g_sun_fwd_held[2]));
        out.push_back(kvf("sunFarFwdHeldM",  RenderIntegration::g_sun_fwd_held[3]));
        out.push_back(kvf("sunHeroD2vM",    RenderIntegration::g_sun_fit_d2v[0]));
        out.push_back(kvf("sunMidD2vM",     RenderIntegration::g_sun_fit_d2v[1]));
        out.push_back(kvf("sunOutD2vM",     RenderIntegration::g_sun_fit_d2v[2]));
        // KH_BAND_ADMIT_CENSUS: which union casters each band admitted, and
        // the outer/far lateral margins and up-sun distances of the first
        // four. Since the margin is PER-AXIS: max(|dx.r3|,|dx.u3|) - (r2+er),
        // same sign convention (negative = inside the clip); radial under
        // 447.
        out.push_back(kv("sunHeroAdmit", RenderIntegration::g_sun_fit_admit[0]));
        out.push_back(kv("sunMidAdmit",  RenderIntegration::g_sun_fit_admit[1]));
        out.push_back(kv("sunOutAdmit",  RenderIntegration::g_sun_fit_admit[2]));
        out.push_back(kv("sunFarAdmit",  RenderIntegration::g_sun_fit_admit[3]));
        out.push_back(kvf("sunOutLatM0", RenderIntegration::g_sun_fit_lat[2][0]));
        out.push_back(kvf("sunOutLatM1", RenderIntegration::g_sun_fit_lat[2][1]));
        out.push_back(kvf("sunOutLatM2", RenderIntegration::g_sun_fit_lat[2][2]));
        out.push_back(kvf("sunOutLatM3", RenderIntegration::g_sun_fit_lat[2][3]));
        out.push_back(kvf("sunOutDsM0",  RenderIntegration::g_sun_fit_ds[2][0]));
        out.push_back(kvf("sunOutDsM1",  RenderIntegration::g_sun_fit_ds[2][1]));
        out.push_back(kvf("sunOutDsM2",  RenderIntegration::g_sun_fit_ds[2][2]));
        out.push_back(kvf("sunOutDsM3",  RenderIntegration::g_sun_fit_ds[2][3]));
        out.push_back(kvf("sunFarLatM0", RenderIntegration::g_sun_fit_lat[3][0]));
        out.push_back(kvf("sunFarLatM1", RenderIntegration::g_sun_fit_lat[3][1]));
        out.push_back(kvf("sunFarLatM2", RenderIntegration::g_sun_fit_lat[3][2]));
        out.push_back(kvf("sunFarLatM3", RenderIntegration::g_sun_fit_lat[3][3]));
        out.push_back(kvf("sunFarD2vM",     RenderIntegration::g_sun_fit_d2v[3]));
    // 0 with the band valid means the convert path failed and that band
    // kernel is CLASSIC regardless of mode - read these before judging any
    // prefilter round.
    out.push_back(kv("sunPfHeroOk", RenderIntegration::g_sun_pf_valid[0] ? 1 : 0));
    out.push_back(kv("sunPfMidOk", RenderIntegration::g_sun_pf_valid[1] ? 1 : 0));
    out.push_back(kv("sunPfOutOk", RenderIntegration::g_sun_pf_valid[2] ? 1 : 0));
    // mu0 in [0.90, 1.00] with mu3 in the same neighbourhood = pyramid +
    // chain healthy; kilometre-class values = the foreign delivery followed
    // the SRV object to the new site (next: per-frame SRV).
    out.push_back(kvf("sunPfCpuMu0C", RenderIntegration::g_sun_pf_cpu[0]));
    out.push_back(kvf("sunPfCpuMu3C", RenderIntegration::g_sun_pf_cpu[1]));
    out.push_back(kvf("sunPfCpuMu0O", RenderIntegration::g_sun_pf_cpu[2]));
    out.push_back(kvf("sunPfCpuMu3O", RenderIntegration::g_sun_pf_cpu[3]));
        // sunUnionTexelM is the union window's texel in world metres (~0.0996
        // at shadowVisibility 200) and is the MAXIMUM edge displacement a
        // phase re-roll could produce - i.e.
        out.push_back(kvf("sunUnionSnapM", RenderIntegration::g_sun_union_snap_m));
        out.push_back(kvf("sunUnionTexelM", RenderIntegration::g_sun_union_texel_m));
        out.push_back(kv("sunUnionSnaps", RenderIntegration::g_sun_union_snaps));
        // 42 = the fallthrough re-armed (mode 328, or mode 326 carrying it as
        // a dependant of the KH_BAND_SUN_REACH revert). 21/26/22/40 = the
        // older cast-chain arms.
        out.push_back(kvf("castChainCode", RenderIntegration::g_cast_chain_code));
        // sunUnionRadHeld -1 = the latch never took. KH_UNION_R_LATCH - THE
        // ARMING LANES, and the ratio is the whole verdict. sunUnionRHolds
        // must DOMINATE sunUnionRelatches while the camera moves and the
        // caster stands still: a hold is a frame whose union texel is bitwise
        // identical to the last one, which is a frame whose grid cannot have
        // re-rolled.
        out.push_back(kvf("sunUnionRadHeld", RenderIntegration::g_sun_union_rad_held));
        out.push_back(kv("sunUnionRHolds", RenderIntegration::g_sun_union_rad_holds));
        out.push_back(kv("sunUnionRelatches", RenderIntegration::g_sun_union_rad_relatches));
        // The lat trio reads exactly as the rad trio above: holds must
        // dominate relatches under a camera walk or nothing was stabilised;
        // -1 held = the lateral latch never took (mode 383/329, or no
        // camera-anchored union fit yet). sunLadderScale = s (1.0 = stock
        // ladder).
        out.push_back(kvf("sunLadderScale", RenderIntegration::g_sun_ladder_scale));
        // KH_SUN_RANGE_SOURCE: the range every subsystem rides and WHO WROTE
        // IT (0 = nobody/the 200 default; 1 = the getVideoOptions fallback; 2
        // = the engine's own committed cascade table, the primary source).
        out.push_back(kvf("sunRangeM", RenderIntegration::g_sun_range.load(std::memory_order_relaxed)));
        out.push_back(kv("sunRangeSrc", static_cast<uint64_t>(RenderIntegration::g_sun_range_src.load(std::memory_order_relaxed))));
        // KH_OBJ_VIS_CUT (mode 437): src 1 = the getVideoOptions map (the
        // shared read in flush_ui_render_sqf), 0 = never matched (cut inert).
        // Session-scoped.
        out.push_back(kvf("objVisM", RenderIntegration::g_obj_vis.load(std::memory_order_relaxed)));
        out.push_back(kv("objVisSrc", static_cast<uint64_t>(RenderIntegration::g_obj_vis_src.load(std::memory_order_relaxed))));
        {   // KH_VIDEO_OPT_KEYS: the map's string keys, '|'-joined, once per session
            auto_array<game_value> khvk_pair;
            khvk_pair.push_back(game_value("videoOptKeys"));
            khvk_pair.push_back(game_value(RenderIntegration::g_video_opt_keys));
            out.push_back(game_value(std::move(khvk_pair)));
        }
        out.push_back(kv("liveWipeSite", static_cast<uint64_t>(RenderIntegration::g_live_wipe_site)));
        out.push_back(kvf("liveWipeT", RenderIntegration::g_live_wipe_t));
        out.push_back(kv("liveWipeN", RenderIntegration::g_live_wipe_n));
        out.push_back(kv("liveWipeDeferrals", RenderIntegration::g_live_wipe_deferrals));   // (session)
        out.push_back(kv("liveWipePrunes", RenderIntegration::g_live_wipe_prunes));   // (session)
        // per-door wipe counts (session); door 5 = cold-hold release
        out.push_back(kv("liveWipeDoor1", RenderIntegration::g_live_wipe_door[1]));
        out.push_back(kv("liveWipeDoor2", RenderIntegration::g_live_wipe_door[2]));
        out.push_back(kv("liveWipeDoor3", RenderIntegration::g_live_wipe_door[3]));
        out.push_back(kv("liveWipeDoor4", RenderIntegration::g_live_wipe_door[4]));
        out.push_back(kv("liveWipeDoor5", RenderIntegration::g_live_wipe_door[5]));
        out.push_back(kv("liveWipeDoor6", RenderIntegration::g_live_wipe_door[6]));
        out.push_back(kvf("sunUnionLatHeld", RenderIntegration::g_sun_union_lat_held));
        out.push_back(kv("sunUnionLatHolds", RenderIntegration::g_sun_union_lat_holds));
        out.push_back(kv("sunUnionLatRelatches", RenderIntegration::g_sun_union_lat_relatches));
        // cascade bands (KH_SUN_CASCADE): halfDiag lanes are the acceptance
        // instrument - KH_SUN_LADDER_SCALE: pinned at 8*s / 32*s with s =
        // clamp(shadowVisibility/200, 1, 5) (the sunLadderScale lane carries
        // s; under mode 382 or 241 s = 1 and the historic 8 / 32 pins
        // return). Any other value is a defect.
        out.push_back(kv("sunMidValid", RenderIntegration::g_sun3_map_valid ? 1ull : 0ull));
        out.push_back(kv("sunMidCasters", RenderIntegration::g_sun3_casters));
        out.push_back(kv("sunMidRenders", RenderIntegration::g_sun3_renders));
        out.push_back(kvf("sunMidHalfDiag", RenderIntegration::g_sun3_half_diag));
        out.push_back(kv("sunOutValid", RenderIntegration::g_sun4_map_valid ? 1ull : 0ull));
        out.push_back(kv("sunOutCasters", RenderIntegration::g_sun4_casters));
        out.push_back(kv("sunOutRenders", RenderIntegration::g_sun4_renders));
        out.push_back(kvf("sunOutHalfDiag", RenderIntegration::g_sun4_half_diag));
        out.push_back(kv("vmirDssFront", RenderIntegration::g_vmir_dss_front));
        out.push_back(kv("vmirDssBack", RenderIntegration::g_vmir_dss_back));
        out.push_back(kv("vmirDssFunc", RenderIntegration::g_vmir_dss_func));
        out.push_back(kv("vmirDssMasks", RenderIntegration::g_vmir_dss_masks));
        out.push_back(kv("vmirDssRef", RenderIntegration::g_vmir_dss_ref));
        out.push_back(kv("vmirCull", RenderIntegration::g_vmir_cull));
        out.push_back(kv("vmirTopoLast", RenderIntegration::g_vmir_topo_last));
        out.push_back(kv("vmirTopoOdd", RenderIntegration::g_vmir_topo_odd));
        out.push_back(kv("vmirCbChanges", RenderIntegration::g_vmir_cb_changes));
        out.push_back(kv("vmirVsChanges", RenderIntegration::g_vmir_vs_changes));
        out.push_back(kv("vmirPsNull", RenderIntegration::g_vmir_ps_null));
        out.push_back(kv("vmirRtvN", RenderIntegration::g_vmir_rtv_n));
        out.push_back(kvf("vmirVpMinD", RenderIntegration::g_vmir_vp_mind));
        out.push_back(kvf("vmirVpMaxD", RenderIntegration::g_vmir_vp_maxd));
        out.push_back(kv("vmirFrames", RenderIntegration::g_vmir_frames));
        out.push_back(kv("vmirDraws", RenderIntegration::g_vmir_draws));
        out.push_back(kv("vmirPrepassDraws", RenderIntegration::g_vmir_prepass_draws));
        out.push_back(kv("vmirMaskFrames", RenderIntegration::g_vmir_mask_frames));
        out.push_back(kv("vmirCsRuns", RenderIntegration::g_vmir_cs_runs));
        out.push_back(kv("vmirB2Off", RenderIntegration::g_vmir_b2_off));
        out.push_back(kv("vmirUavN", RenderIntegration::g_vmir_uav_n));
        out.push_back(kv("vmirUavSkips", RenderIntegration::g_vmir_uav_skips));
        out.push_back(kv("vmirEnsureFails", RenderIntegration::g_vmir_ensure_fails));
        out.push_back(kv("vmirClampStates", RenderIntegration::g_vmir_clamp_states));
        // the live half of the clamp gauge pair + the session gate census.
        out.push_back(kv("vmirClampSwaps", RenderIntegration::g_vmir_clamp_swaps));
        out.push_back(kv("vmirSessionSkips", RenderIntegration::g_vmir_session_skips));
        // proves the sun anchor ACTIVE in the same capture that tests the
        // shimmer (0.0 here on a valid-camera session = inert).
        out.push_back(kvf("sunAnchorMag", std::sqrt(
            RenderIntegration::g_sun_anchor_now[0] * RenderIntegration::g_sun_anchor_now[0] +
            RenderIntegration::g_sun_anchor_now[1] * RenderIntegration::g_sun_anchor_now[1] +
            RenderIntegration::g_sun_anchor_now[2] * RenderIntegration::g_sun_anchor_now[2])));
        out.push_back(kv("fireMaskSrvFires", RenderIntegration::g_fire_mask_srv_fires));
        out.push_back(kv("fireMaskSrvLast", static_cast<uint64_t>(RenderIntegration::g_fire_mask_srv_last)));
        out.push_back(kvf("fireFovMaxDelta", RenderIntegration::g_fov_max_delta));
        out.push_back(kvf("sunChurnMaxDeg", RenderIntegration::g_sun_churn_max_deg));
        out.push_back(kvf("camStepMaxM", RenderIntegration::g_cam_step_max));
        out.push_back(kvf("fireCamDeltaMaxM", RenderIntegration::g_fire_cam_delta_max));
        out.push_back(kvf("fireFreezeDyMaxM", RenderIntegration::g_fire_freeze_dy_max_m));
        out.push_back(kvf("fireFreezePitchMaxDeg", RenderIntegration::g_fire_freeze_pitch_max_deg));
        out.push_back(kvf("fireFreezeYawMaxDeg", RenderIntegration::g_fire_freeze_yaw_max_deg));
        out.push_back(kvf("fireFreezeDyM", RenderIntegration::g_fire_freeze_dy_m));
        out.push_back(kvf("fireFreezePitchDeg", RenderIntegration::g_fire_freeze_pitch_deg));
        out.push_back(kvf("fireFreezeYawDeg", RenderIntegration::g_fire_freeze_yaw_deg));
        out.push_back(kv("fireFreezeSamples", RenderIntegration::g_fire_freeze_samples));
        out.push_back(kv("fireRefreezes", RenderIntegration::g_fire_refreezes));
        out.push_back(kv("fireRefreezeFails", RenderIntegration::g_fire_refreeze_fails));
        // the engine-render-view vs boundary-latch pair. Read EngPitchMaxDeg
        // against EngYawMaxDeg and EngPosMaxM: pitch alone, with position and
        // yaw near zero, is the first-person neck-pivot signature and
        // convicts the bridge adoption.
        out.push_back(kvf("fireEngDyM", RenderIntegration::g_fire_eng_dy_m));
        out.push_back(kvf("fireEngDyMaxM", RenderIntegration::g_fire_eng_dy_max_m));
        out.push_back(kvf("fireEngPitchDeg", RenderIntegration::g_fire_eng_pitch_deg));
        out.push_back(kvf("fireEngPitchMaxDeg", RenderIntegration::g_fire_eng_pitch_max_deg));
        out.push_back(kvf("fireEngYawDeg", RenderIntegration::g_fire_eng_yaw_deg));
        out.push_back(kvf("fireEngYawMaxDeg", RenderIntegration::g_fire_eng_yaw_max_deg));
        out.push_back(kvf("fireEngPosM", RenderIntegration::g_fire_eng_pos_m));
        out.push_back(kvf("fireEngPosMaxM", RenderIntegration::g_fire_eng_pos_max_m));
        out.push_back(kv("fireEngSamples", RenderIntegration::g_fire_eng_samples));
        out.push_back(kv("fireEngFreezes", RenderIntegration::g_fire_eng_freezes));
        out.push_back(kv("fireSingleSuppressed", RenderIntegration::g_fire_single_suppressed));   // dbg 40
        // last-fire clamp. fireClampTarget is the learned index of the
        // frame's last fire; fireClampPaints should track topoCycles almost
        // exactly (one paint per frame) and fireClampRelearns should stay
        // near zero - a large relearn count means the fire count is unstable
        // frame to frame and the clamp is chasing it.
        out.push_back(kv("fireClampTarget", static_cast<uint64_t>(RenderIntegration::g_fire_clamp_target)));
        out.push_back(kv("fireClampPaints", RenderIntegration::g_fire_clamp_paints));
        out.push_back(kv("fireClampRelearns", RenderIntegration::g_fire_clamp_relearns));
        // backstop paints - frames that reached the learned last fire without
        // the sweep ever verifying. Should be a small fraction of
        // fireClampPaints; if it is most of them, the sweep gate is the
        // problem and not the clamp.
        out.push_back(kv("fireClampUnverified", RenderIntegration::g_fire_clamp_unverified));
        // fireClampPaints should now track topoCycles closely. If it still
        // falls well short, the fire count is unstable frame to frame (watch
        // fireClampRelearns) and the clamp is chasing it.
        out.push_back(kv("fireDepthRefreshes", RenderIntegration::g_fire_depth_refreshes));
        out.push_back(kv("bandRejStaleView", RenderIntegration::g_band_rej_stale_view));
        out.push_back(kvf("castPitchDeg", RenderIntegration::g_cast_pitch_deg));
        out.push_back(kvf("castPitchMinDeg", RenderIntegration::g_cast_pitch_min_deg));
        out.push_back(kvf("castPitchMaxDeg", RenderIntegration::g_cast_pitch_max_deg));
        out.push_back(kv("castPitchNegFrames", RenderIntegration::g_cast_pitch_neg_frames));
        out.push_back(kv("castPitchNegPaints", RenderIntegration::g_cast_pitch_neg_paints));
        out.push_back(kv("castPitchNegHolds", RenderIntegration::g_cast_pitch_neg_holds));
        out.push_back(kv("castPitchNegMiss", static_cast<uint64_t>(RenderIntegration::g_cast_pitch_neg_miss)));
        out.push_back(kv("castPitchPosFrames", RenderIntegration::g_cast_pitch_pos_frames));
        out.push_back(kv("castPitchPosPaints", RenderIntegration::g_cast_pitch_pos_paints));
        out.push_back(kv("castPitchPosHolds", RenderIntegration::g_cast_pitch_pos_holds));
        out.push_back(kv("castPitchPosMiss", static_cast<uint64_t>(RenderIntegration::g_cast_pitch_pos_miss)));
        // per-bucket, how many frames had ANY fire land while the engine held
        // the mask SRV-bound. Compare against the bucket's Frames count -
        // that ratio, not the paint count, is what the shadow follows.
        out.push_back(kv("castPitchNegSrv", RenderIntegration::g_cast_pitch_neg_srv));
        out.push_back(kv("castPitchPosSrv", RenderIntegration::g_cast_pitch_pos_srv));
        out.push_back(kv("fireSrvClampPaints", RenderIntegration::g_fire_srv_clamp_paints));
        out.push_back(kv("fireSrvClampFallbacks", RenderIntegration::g_fire_srv_clamp_fallbacks));
        // second paints added on fire-1 frames so the NEXT frame has
        // something to read. Should track fireSrvClampPaints closely.
        out.push_back(kv("fireCarryPaints", RenderIntegration::g_fire_carry_paints));   // retired
        // extra early paints on frames that follow an early-paint frame.
        // Should be a handful a session, not hundreds - if it tracks
        // fireSrvClampPaints the targeting is wrong again.
        out.push_back(kv("firePrimePaints", RenderIntegration::g_fire_prime_paints));
        out.push_back(kv("fireSrvIndex", static_cast<uint64_t>(RenderIntegration::g_fire_srv_index)));
        out.push_back(kv("fireSrvIndexMoves", RenderIntegration::g_fire_srv_index_moves));
        // retired - the learned index no longer steers the backstop (it was
        // degenerate at 1). Kept at 0 so the lane order is stable.
        out.push_back(kv("fireSrvBackstopAtIndex", RenderIntegration::g_fire_srv_backstop_at_index));
        // The go/no-go for re-keying the arm is maskSrvBindFrames against
        // flushes: at parity the engine announces its apply window on every
        // frame and the fire can be hung off it; well below parity the re-key
        // would starve the fire and the idea is dead. maskSrvDryFrames
        // counts, directly, the frames it would have starved.
        out.push_back(kv("maskSrvBindHits", RenderIntegration::g_mask_srv_bind_hits));
        out.push_back(kv("maskSrvBindFrames", RenderIntegration::g_mask_srv_bind_frames));
        out.push_back(kv("maskSrvDryFrames", RenderIntegration::g_mask_srv_dry_frames));
        out.push_back(kv("maskSrvBindsMax", static_cast<uint64_t>(RenderIntegration::g_mask_srv_binds_max)));
        out.push_back(kvf("maskSrvFirstD", RenderIntegration::g_mask_srv_first_d_pub));
        out.push_back(kvf("maskSrvLastD", RenderIntegration::g_mask_srv_last_d_pub));
        out.push_back(kvf("fireFirstDPub", RenderIntegration::g_fire_first_d_pub));
        out.push_back(kvf("fireLastDPub", RenderIntegration::g_fire_last_d_pub));
        // Tracked per frame, never emitted outside the flight recorder.
        out.push_back(kv("maskRtBindsPub", static_cast<uint64_t>(RenderIntegration::g_mask_rt_binds_pub)));
        out.push_back(kvf("maskLastBindDPub", RenderIntegration::g_mask_last_bind_d_pub));
        out.push_back(kvf("maskFirstBindDPub", RenderIntegration::g_mask_first_bind_d_pub));
        out.push_back(kv("firePaintIxPub", static_cast<uint64_t>(RenderIntegration::g_fire_paint_ix_pub)));
        out.push_back(kv("firePaintPathPub", static_cast<uint64_t>(RenderIntegration::g_fire_paint_path_pub)));
        // fireSrvGapMax is the margin the flicker turns on: draws between the
        // engine's last mask read and our first fire.
        out.push_back(kvf("fireSrvGapD", RenderIntegration::g_fire_srv_gap_d_pub));
        out.push_back(kvf("fireSrvGapMax", RenderIntegration::g_fire_srv_gap_max));
        out.push_back(kv("fireCatchupArms", RenderIntegration::g_fire_catchup_arms));
        out.push_back(kv("fireCatchupPaints", RenderIntegration::g_fire_catchup_paints));
        out.push_back(kv("maskSrvArmFires", RenderIntegration::g_mask_srv_arm_fires));
        out.push_back(kv("maskSrvArmLate", RenderIntegration::g_mask_srv_arm_late));
        out.push_back(kv("fireSrvBackstopAtLast", RenderIntegration::g_fire_srv_backstop_at_last));
        out.push_back(kvf("fireOrthoDefect", RenderIntegration::g_fire_ortho_defect));
        out.push_back(kvf("fireOrthoDefectMax", RenderIntegration::g_fire_ortho_defect_max));
        out.push_back(kvf("fireCamSkewM", RenderIntegration::g_fire_cam_skew_m));
        out.push_back(kvf("fireCamSkewMaxM", RenderIntegration::g_fire_cam_skew_max));
        out.push_back(kv("sunTravelWitnessVetoes", RenderIntegration::g_sun_travel_witness_vetoes));
        out.push_back(kv("sunPursuitWitnessHolds", RenderIntegration::g_sun_pursuit_witness_holds));
        out.push_back(kv("skySunRefValid", static_cast<uint64_t>(RenderIntegration::g_skysun_ref_valid ? 1 : 0)));
        {
            float khss_deg = -1.0f;

            if (RenderIntegration::g_skysun_ref_valid) {
                float khss_d = RenderIntegration::g_skysun_ref[0] * RenderIntegration::g_sun_dir_engine[0] +
                               RenderIntegration::g_skysun_ref[1] * RenderIntegration::g_sun_dir_engine[1] +
                               RenderIntegration::g_skysun_ref[2] * RenderIntegration::g_sun_dir_engine[2];
                khss_d = khss_d > 1.0f ? 1.0f : (khss_d < -1.0f ? -1.0f : khss_d);
                khss_deg = acosf(khss_d) * 57.29578f;
            }

            out.push_back(kvf("skySunVsPubDeg", khss_deg));
        }
        out.push_back(kvf("skySunMotDeg", RenderIntegration::g_skysun_mot_deg));
        out.push_back(kv("sunMapCalmRolls", RenderIntegration::g_sun_mat_calm_rolls));
        // sun-map maturity. sunMapMature is localityMeta.z as the SHADER
        // received it - 0 means the far-plane presence test is DISARMED and
        // the longest shadow is being cut at c.z = 1. PREDICTION: spawning a
        // second caster increments sunMatDemCchg once and drops sunMapMature
        // to 0 for ~0.5 s.
        out.push_back(kvf("sunMapMature", RenderIntegration::g_sun_map_mature_pub));
        out.push_back(kv("sunMatDemNew", RenderIntegration::g_sun_mat_dem_new));
        out.push_back(kv("sunMatDemCchg", RenderIntegration::g_sun_mat_dem_cchg));
        out.push_back(kv("sunMatDemBig", RenderIntegration::g_sun_mat_dem_big));
        out.push_back(kv("sunMatDemDrift", RenderIntegration::g_sun_mat_dem_drift));
        out.push_back(kv("sunMapCasterRolls", RenderIntegration::g_sun_mat_caster_rolls));
        out.push_back(kv("relockRecvWipes", RenderIntegration::g_relock_recv_wipes));
        out.push_back(kv("fireDimsIncoherent", RenderIntegration::g_fire_dims_incoh_total));
        out.push_back(kvf("fireDimsDivMaxPx", RenderIntegration::g_fire_dims_div_max));
        out.push_back(kv("castDimsForeign", RenderIntegration::g_cast_dims_foreign));
        out.push_back(kv("fireVpMode", static_cast<uint64_t>(RenderIntegration::g_fire_vp_mode)));
        out.push_back(kvf("castDimsLiveW", RenderIntegration::g_mask.cast_dims[0]));
        out.push_back(kvf("castDimsLiveH", RenderIntegration::g_mask.cast_dims[1]));
        out.push_back(kvf("fireDims2W", RenderIntegration::g_fire_dims2[0]));
        out.push_back(kvf("fireDims2H", RenderIntegration::g_fire_dims2[1]));
        out.push_back(kvf("lockChurnAgeS", age_s(RenderIntegration::g_lock_churn_ms_v)));
        out.push_back(kv("castLockSettleHolds", RenderIntegration::g_cast_lock_settle_holds));
        // BRIDGE-EPOCH FREEZE health: bridge adoption is the norm; pub fires
        // / era rejects / repairs are anomaly counters.
        out.push_back(kv("fireViewBridgeFires", RenderIntegration::g_fire_view_bridge_fires));
        out.push_back(kv("fireViewPubFires", RenderIntegration::g_fire_view_pub_fires));
        out.push_back(kv("fireViewBridgeRepairs", RenderIntegration::g_fire_view_repairs));
        out.push_back(kv("fireViewEraRejects", RenderIntegration::g_fire_view_era_rejects));
        // CAST READINESS LATCH : ready state + age, holds by class (57
        // readiness, 58 stale-map), and latch drops.
        out.push_back(kv("castReady", static_cast<uint64_t>(RenderIntegration::g_cast_ready ? 1 : 0)));
        out.push_back(kvf("castReadyAgeS", age_s(RenderIntegration::g_cast_ready_ms)));
        out.push_back(kv("castReadyHolds", RenderIntegration::g_cast_ready_holds));
        out.push_back(kv("castMapHolds", RenderIntegration::g_cast_map_holds));
        out.push_back(kv("castReadyDrops", RenderIntegration::g_cast_ready_drops));
        // castReadyHolds counts the unstable branch and the still-elapsing
        // branch together and so can never name the flapping term.
        // castDwellResets is the number that matters for a 64 s cold: a dwell
        // that never elapses is being restarted, and exactly one of the three
        // terms below is doing it.
        out.push_back(kv("castDwellChurn", RenderIntegration::g_cast_dwell_churn));
        // WHICH site stamps the churn the dwell keys on, and how many land
        // inside the 1.5 s window that makes them compound.
        out.push_back(kv("churnStampRelock", RenderIntegration::g_lock_churn_stamp_relock));
        out.push_back(kv("churnStampDrop", RenderIntegration::g_lock_churn_stamp_drop));
        out.push_back(kv("churnStampPrewarm", RenderIntegration::g_lock_churn_stamp_prewarm));
        out.push_back(kv("churnStampRing", RenderIntegration::g_lock_churn_stamp_ring));
        out.push_back(kv("churnStampsClose", RenderIntegration::g_lock_churn_stamps_close));
        out.push_back(kv("castDwellTravel", RenderIntegration::g_cast_dwell_travel));
        out.push_back(kv("castDwellJump", RenderIntegration::g_cast_dwell_jump));
        out.push_back(kv("castDwellResets", RenderIntegration::g_cast_dwell_resets));
        out.push_back(kv("castDwellTicks", RenderIntegration::g_cast_dwell_ticks));
        out.push_back(kv("castUnarmedFrames", RenderIntegration::g_cast_unarmed_frames));
        out.push_back(kv("liveRejSunAxis", RenderIntegration::g_live_rej_sun_axis));
        out.push_back(kvf("liveRejSunAxisLastDeg", RenderIntegration::g_live_rej_sun_axis_deg));
        out.push_back(kv("bandRejSunAxis", RenderIntegration::g_band_rej_sun_axis));
        out.push_back(kvf("bandRejSunAxisLastDeg", RenderIntegration::g_band_rej_sun_axis_deg));
        out.push_back(kv("bandRejNoView", RenderIntegration::g_band_rej_no_view));
        out.push_back(kv("liveRejNoView", RenderIntegration::g_live_rej_no_view));
        // KH_LIVE_CAM_PAIR (mode 334 reverts)
        out.push_back(kv("liveCamPairFv", RenderIntegration::g_live_cam_pair_fv));
        out.push_back(kv("liveCamPairPv", RenderIntegration::g_live_cam_pair_pv));
        out.push_back(kv("liveCamPairStale", RenderIntegration::g_live_cam_pair_stale));
        out.push_back(kvf("liveCamPairDxM", RenderIntegration::g_live_cam_pair_dx));
        out.push_back(kvf("liveCamPairDxMaxM", RenderIntegration::g_live_cam_pair_dx_max));
        out.push_back(kv("bandPairSameSeals", RenderIntegration::g_band_pair_same_seals));   // arming lane
        // KH_BAND_UV_DRIFT (pure gauge)
        out.push_back(kvf("bandUvDriftPx", RenderIntegration::g_band_uv_drift_px));
        out.push_back(kvf("bandUvDriftPxMax", RenderIntegration::g_band_uv_drift_px_max));
        out.push_back(kvf("bandUvDriftPxMean", RenderIntegration::g_band_uv_drift_n
            ? static_cast<float>(RenderIntegration::g_band_uv_drift_sum /
                                 static_cast<double>(RenderIntegration::g_band_uv_drift_n)) : -1.0f));
        out.push_back(kv("bandUvDriftN", RenderIntegration::g_band_uv_drift_n));
        out.push_back(kv("bandUvDriftOver2", RenderIntegration::g_band_uv_drift_over2));
        out.push_back(kv("bandUvDriftSlot", static_cast<uint64_t>(
            RenderIntegration::g_band_uv_drift_slot < 0 ? 99 : RenderIntegration::g_band_uv_drift_slot)));
        out.push_back(kv("bandUvDriftWhy", static_cast<uint64_t>(RenderIntegration::g_band_uv_drift_why)));
        // KH_BAND_FOREIGN_VIEW (mode 336 reverts)
        out.push_back(kv("bandSealForeignRef", RenderIntegration::g_band_seal_foreign_ref));
        out.push_back(kvf("bandSealForeignM", RenderIntegration::g_band_seal_foreign_m));
        out.push_back(kvf("bandSealForeignMaxM", RenderIntegration::g_band_seal_foreign_max_m));
        // KH_BAND_PAIR_MIXED (mode 337 reverts) + completion-census split
        out.push_back(kv("bandPairMixedRef", RenderIntegration::g_band_pair_mixed_ref));
        out.push_back(kvf("bandPairMixedM", RenderIntegration::g_band_pair_mixed_m));
        out.push_back(kvf("bandPairMixedMaxM", RenderIntegration::g_band_pair_mixed_max_m));
        out.push_back(kv("bandCompleteInv", RenderIntegration::g_band_complete_inv));
        out.push_back(kvf("bandCompleteOldTrnM", RenderIntegration::g_band_complete_old_trn_m));
        out.push_back(kvf("bandCompleteOldTrnMaxM", RenderIntegration::g_band_complete_old_trn_max_m));
        // drift metres twin (texels / (|sm row0| * atlas_size))
        out.push_back(kvf("bandUvDriftM", RenderIntegration::g_band_uv_drift_m));
        out.push_back(kvf("bandUvDriftMaxM", RenderIntegration::g_band_uv_drift_m_max));
        // KH_BAND_ALL_NEAR (mode 338) engagement
        out.push_back(kv("bandCadenceForced", RenderIntegration::g_band_cadence_forced));
        // KH_BAND_COHERENT_COMPLETE (mode 339 reverts)
        out.push_back(kv("bandPendLateKept", RenderIntegration::g_band_pend_late_kept));
        out.push_back(kv("bandPendLateBridge", RenderIntegration::g_band_pend_late_bridge));
        // ORDER CENSUS (pure gauge)
        out.push_back(kvf("bandCapFvAgeMs", RenderIntegration::g_band_cap_fv_age_ms));
        out.push_back(kvf("bandCapFvAgeMsMean", RenderIntegration::g_band_cap_fv_age_n
            ? static_cast<float>(RenderIntegration::g_band_cap_fv_age_sum /
                                 static_cast<double>(RenderIntegration::g_band_cap_fv_age_n))
            : -1.0f));
        out.push_back(kvf("bandCapFvAgeMsMax", RenderIntegration::g_band_cap_fv_age_max_ms));
        out.push_back(kvf("bandCompAgeMsMean", RenderIntegration::g_band_comp_age_n
            ? static_cast<float>(RenderIntegration::g_band_comp_age_sum /
                                 static_cast<double>(RenderIntegration::g_band_comp_age_n))
            : -1.0f));
        out.push_back(kvf("bandCompAgeMsMax", RenderIntegration::g_band_comp_age_max_ms));
        // KH_BAND_SAME_SOURCE (mode 340 reverts)
        out.push_back(kv("bandCompSrcRef", RenderIntegration::g_band_comp_src_ref));
        out.push_back(kvf("bandCompSrcM", RenderIntegration::g_band_comp_src_m));
        out.push_back(kvf("bandCompSrcMaxM", RenderIntegration::g_band_comp_src_max_m));
        out.push_back(kv("bandCompSrcAlt", RenderIntegration::g_band_comp_src_alt));
        // KH_BAND_ALT_PROVISIONAL (mode 342 reverts) KH_BAND_INJ_PAIR (mode
        // 344 reverts)
        out.push_back(kv("bandInjPair", RenderIntegration::g_band_inj_pair));
        out.push_back(kv("bandInjPairMiss", RenderIntegration::g_band_inj_pair_miss));
        out.push_back(kv("bandProvAlt", RenderIntegration::g_band_prov_alt));
        out.push_back(kv("bandProvAltMiss", RenderIntegration::g_band_prov_alt_miss));
        out.push_back(kv("bandPendTimeoutKept", RenderIntegration::g_band_pend_timeout_kept));
        out.push_back(kv("bandPendSingleSrc", RenderIntegration::g_band_pend_single_src));
        // PICK CENSUS (always on, both paths)
        out.push_back(kv("bandCompXSrc", RenderIntegration::g_band_comp_xsrc));
        out.push_back(kv("bandCompSameSrcN", RenderIntegration::g_band_comp_samesrc));
        out.push_back(kv("bandStageXSrc", RenderIntegration::g_band_stage_xsrc));
        out.push_back(kv("bandStageSameSrc", RenderIntegration::g_band_stage_samesrc));
        out.push_back(kv("bandStageFlip", RenderIntegration::g_band_stage_flip));
        out.push_back(kv("bandStageFlipHold", RenderIntegration::g_band_stage_flip_hold));
        out.push_back(kv("bandStageFlipSingle", RenderIntegration::g_band_stage_flip_single));
        out.push_back(kv("bandStageFlipLate", RenderIntegration::g_band_stage_flip_late));
        out.push_back(kvf("weldFlipInjDxM", RenderIntegration::g_weld_flip_inj_dx));
        out.push_back(kvf("weldFlipInjDxMaxM", RenderIntegration::g_weld_flip_inj_dx_max));
        out.push_back(kvf("weldFlipInjDxMeanM", RenderIntegration::g_weld_flip_inj_dx_n
            ? static_cast<float>(RenderIntegration::g_weld_flip_inj_dx_sum /
                static_cast<double>(RenderIntegration::g_weld_flip_inj_dx_n)) : -1.0f));
        out.push_back(kv("weldFlipInjDxN", RenderIntegration::g_weld_flip_inj_dx_n));
        out.push_back(kvf("weldFlipInjRotMaxDeg", RenderIntegration::g_weld_flip_inj_rot_max));
        out.push_back(kv("bandStageFlipPoolBail", RenderIntegration::g_band_stage_flip_poolbail));
        out.push_back(kv("bandStagePoolCap", RenderIntegration::g_band_stage_pool_cap));
        out.push_back(kv("bandPendDeadFast", RenderIntegration::g_band_pend_dead_fast));
        out.push_back(kv("bandWipeEvents", RenderIntegration::g_band_wipe_events));
        out.push_back(kv("bandAgeKills", RenderIntegration::g_band_age_kills));
        out.push_back(kv("bandRotWelds", RenderIntegration::g_band_rot_welds));
        out.push_back(kv("sunAxisBootRefs", RenderIntegration::g_sun_axis_boot_refs));
        out.push_back(kv("sunAxisColdRejects", RenderIntegration::g_sun_axis_cold_rejects));
        out.push_back(kvf("sunAxisLastRefDeg", RenderIntegration::g_sun_axis_last_ref_deg));
        out.push_back(kv("sunDerivedSnaps", RenderIntegration::g_sun_snap_adopts));
        out.push_back(kv("sunDerivedSnapNeutral", RenderIntegration::g_sun_snap_neutral));
        out.push_back(kv("sunDerivedSnapHolds", RenderIntegration::g_sun_snap_refusals));
        out.push_back(kvf("sunDerivedSnapLastDeg", RenderIntegration::g_sun_snap_last_deg));
        out.push_back(kvf("sunDerivedSnapAgeS", age_s(RenderIntegration::g_sun_snap_ms)));
        out.push_back(kvf("sunPubTravelAgeS", age_s(RenderIntegration::g_sun_pub_travel_ms)));
        out.push_back(kv("sunDerivedViewSkips", RenderIntegration::g_sun_derived_view_skips));
        out.push_back(kv("sunDerivedBridgeSamples", RenderIntegration::g_sun_derived_bridge));
        {
            auto khcold = [](float khc_t) {
                return khc_t >= 0.0f && RenderIntegration::g_mask.cold_t0 >= 0.0
                    ? khc_t - static_cast<float>(RenderIntegration::g_mask.cold_t0)
                    : -1.0f;
            };
            out.push_back(kvf("coldSunBootS", khcold(RenderIntegration::g_first_sun_boot_t)));
            out.push_back(kvf("coldSunValidS", khcold(RenderIntegration::g_first_sun_valid_t)));
            out.push_back(kvf("coldSunSettledS", khcold(RenderIntegration::g_first_sun_settled_t)));
            out.push_back(kvf("coldDerived120S", khcold(RenderIntegration::g_first_derived120_t)));
            out.push_back(kvf("coldFirstBandCapS", khcold(RenderIntegration::g_first_bandcap_t)));
            out.push_back(kvf("coldFirstLiveLatchS", khcold(RenderIntegration::g_first_livelatch_t)));
            out.push_back(kvf("coldRecvHealthS", khcold(RenderIntegration::g_recv_health_t)));
        }
        out.push_back(kv("atlasSrvEvicts", RenderIntegration::g_atlas_srv_evicts));
        out.push_back(kv("bandInsaneSkips", RenderIntegration::g_band_insane_skips));
        out.push_back(kv("bandWarmupSkips", RenderIntegration::g_band_warmup_skips));
        // COLD RECEIVE HEALTH (setRenderDebug 47 reverts). bandColdHolds is
        // the receive standing down for it; recvColdWipes is the cold-era
        // seals dropped when the latch rose. coldRecvHealthS is when that
        // happened, on the same cold timeline as coldFirstBandCapS - compare
        // the two: the gap is the window that used to be consumable.
        out.push_back(kv("bandSealUnlocked", RenderIntegration::g_band_seal_unlocked));
        out.push_back(kv("bandColdHolds", RenderIntegration::g_band_cold_holds));
        // EXPECT this to land near the old bandPendConsumed with bandPickNone
        // unchanged; bandPickNone climbing by about the same amount means the
        // offset was traded for a one-cycle dropout.
        out.push_back(kv("bandPendRefused", RenderIntegration::g_band_pend_refused));
        // EXPECT commits ~= takes, bandPendRefused ~0 and bandPickNone back
        // to its value. bandStageAtlasMoved is measured, not acted on: it
        // counts commits whose cascade re-rendered between the resolve that
        // paired them and the copy, against a receive that is already 50-250
        // ms stale between reseals by design.
        out.push_back(kv("bandStageTakes", RenderIntegration::g_band_stage_takes));
        out.push_back(kv("bandStageCommits", RenderIntegration::g_band_stage_commits));
        out.push_back(kv("bandStageDropAge", RenderIntegration::g_band_stage_drop_age));
        out.push_back(kv("bandStageDropDead", RenderIntegration::g_band_stage_drop_dead));
        // RETIRED and published at 0 (lane order is a dump contract).
        out.push_back(kv("bandStageAtlasMoved", RenderIntegration::g_band_stage_atlas_moved));
        out.push_back(kv("bandStagePoolMiss", RenderIntegration::g_band_stage_pool_miss));
        // misses served by the immediate capture rather than dropped. Should
        // track bandStagePoolMiss; a gap means 214 is set.
        out.push_back(kv("bandStagePoolFall", RenderIntegration::g_band_stage_pool_fall));
        // increments once per STAGED SLOT PER ENGINE DRAW while a commit
        // waits on the exact-class publish bar - kh_band_stage_commit runs
        // from reorder_pre_draw. It is a dwell measure in slot-draws; any
        // ratio against takes or commits is meaningless.
        out.push_back(kv("bandStageHoldQuality", RenderIntegration::g_band_stage_hold_quality));
        out.push_back(kvf("bandStageWaitMs", RenderIntegration::g_band_stage_wait_ms));
        out.push_back(kvf("bandStageWaitMaxMs", RenderIntegration::g_band_stage_wait_max_ms));
        out.push_back(kv("bandEscLands", RenderIntegration::g_band_esc_lands));
        out.push_back(kvf("bandEscLandMs", RenderIntegration::g_band_esc_land_ms));
        out.push_back(kvf("bandEscLandMsMax", RenderIntegration::g_band_esc_land_ms_max));
        out.push_back(kvf("bandEscLandMsMean", RenderIntegration::g_band_esc_lands
            ? static_cast<float>(RenderIntegration::g_band_esc_land_ms_sum /
                                 static_cast<double>(RenderIntegration::g_band_esc_lands)) : -1.0f));
        out.push_back(kv("bandEscLandFr", static_cast<uint64_t>(RenderIntegration::g_band_esc_land_fr)));
        out.push_back(kv("bandEscLandFrMax", static_cast<uint64_t>(RenderIntegration::g_band_esc_land_fr_max)));
        out.push_back(kvf("bandEscLandCdxM", RenderIntegration::g_band_esc_land_cdx));
        out.push_back(kvf("bandEscLandCdxMaxM", RenderIntegration::g_band_esc_land_cdx_max));
        out.push_back(kvf("bandSealInjDxM", RenderIntegration::g_band_seal_inj_dx));
        out.push_back(kvf("bandSealInjDxMaxM", RenderIntegration::g_band_seal_inj_dx_max));
        out.push_back(kvf("bandSealInjRotDeg", RenderIntegration::g_band_seal_inj_rot));
        out.push_back(kvf("bandSealInjRotMaxDeg", RenderIntegration::g_band_seal_inj_rot_max));
        out.push_back(kvf("bandResealStepM", RenderIntegration::g_band_reseal_step));
        out.push_back(kvf("bandResealStepNearMaxM", RenderIntegration::g_band_reseal_step_max[0]));
        out.push_back(kvf("bandResealStepMidMaxM", RenderIntegration::g_band_reseal_step_max[1]));
        out.push_back(kvf("bandResealStepFarMaxM", RenderIntegration::g_band_reseal_step_max[2]));
        out.push_back(kvf("bandResealStepNearMeanM", RenderIntegration::g_band_reseal_n[0]
            ? static_cast<float>(RenderIntegration::g_band_reseal_sum[0] / static_cast<double>(RenderIntegration::g_band_reseal_n[0])) : -1.0f));
        out.push_back(kvf("bandResealStepMidMeanM", RenderIntegration::g_band_reseal_n[1]
            ? static_cast<float>(RenderIntegration::g_band_reseal_sum[1] / static_cast<double>(RenderIntegration::g_band_reseal_n[1])) : -1.0f));
        out.push_back(kvf("bandResealStepFarMeanM", RenderIntegration::g_band_reseal_n[2]
            ? static_cast<float>(RenderIntegration::g_band_reseal_sum[2] / static_cast<double>(RenderIntegration::g_band_reseal_n[2])) : -1.0f));
        out.push_back(kv("bandResealNear", RenderIntegration::g_band_reseal_n[0]));
        out.push_back(kv("bandResealMid", RenderIntegration::g_band_reseal_n[1]));
        out.push_back(kv("bandResealFar", RenderIntegration::g_band_reseal_n[2]));
        out.push_back(kv("recvColdWipes", RenderIntegration::g_recv_cold_wipes));
        out.push_back(kv("recvHealthOk", static_cast<uint64_t>(RenderIntegration::g_recv_health_ok ? 1 : 0)));
        out.push_back(kv("bandOverlapPairs", RenderIntegration::g_band_overlap_pairs));
        out.push_back(kv("meshDarkFrames", RenderIntegration::g_ffr_dark_frames));
        out.push_back(kv("carryErasedFrames", RenderIntegration::g_ffr_erased_frames));
        out.push_back(kvf("ffrDarkAgeS", age_s(RenderIntegration::g_ffr_dark_ms)));
        out.push_back(kvf("ffrErasedAgeS", age_s(RenderIntegration::g_ffr_erased_ms)));
        out.push_back(kv("ffrFrames", static_cast<uint64_t>(RenderIntegration::g_ffr_serial)));
        out.push_back(kv("compositeRescues", RenderIntegration::g_composite_rescues));
        out.push_back(kvf("rescueLastAgeS", age_s(RenderIntegration::g_rescue_last_ms)));
        out.push_back(kv("rescueRefCarried", RenderIntegration::g_rescue_ref_carried));
        out.push_back(kv("rescueRefPreflush", RenderIntegration::g_rescue_ref_preflush));
        out.push_back(kv("rescueRefShield", RenderIntegration::g_rescue_ref_shield));
        out.push_back(kv("rescueCapStops", RenderIntegration::g_rescue_cap_stops));
        out.push_back(kv("flushSlotBandRejects", RenderIntegration::g_flush_slot_band_rejects));
        out.push_back(kvf("flushSlotRejNear", RenderIntegration::g_flush_slot_rej_near));
        out.push_back(kv("injSlotBandRejects", RenderIntegration::g_inj_slot_band_rejects));
        out.push_back(kvf("injSlotRejNear", RenderIntegration::g_inj_slot_rej_near));
        out.push_back(kv("latchHolds", RenderIntegration::g_latch_holds));
        out.push_back(kvf("latchLiveDeltaMaxM", RenderIntegration::g_latch_live_delta_max));
        out.push_back(kvf("latchAgeMaxMs", RenderIntegration::g_latch_age_max_ms));
        out.push_back(kv("injViewLiveAdopts", RenderIntegration::g_inj_view_live_adopts));
        out.push_back(kvf("injViewLiveLastM", RenderIntegration::g_inj_view_live_last_m));
        out.push_back(kv("injViewClearAdopts", RenderIntegration::g_inj_view_clear_adopts));
        out.push_back(kvf("injRotDeltaMaxDeg", RenderIntegration::g_inj_rot_delta_max));
        out.push_back(kv("viewAdoptPreframe", RenderIntegration::g_view_adopt_preframe));
        out.push_back(kv("injEncLiveOverrides", RenderIntegration::g_inj_enc_live_overrides));
        // encOvrVetoes/encOvrVetoNear - encSrc-4 collapse overrides stood
        // down by the measured witness (baseline fallback), and the last
        // vetoed (= shipped) candidate near. Expect vetoes counting on FP
        // stance collapses and Zeus glide flips; overrides ~0.
        out.push_back(kv("encOvrVetoes", RenderIntegration::g_inj_enc_ovr_vetoes));
        out.push_back(kvf("encOvrVetoNear", RenderIntegration::g_inj_enc_ovr_veto_near));
        // worldLatchWits - class-refused world-latch captures accepted on the
        // engine's own measured-pair agreement (the glide-flip escape).
        out.push_back(kv("worldLatchWits", RenderIntegration::g_world_latch_wits));
        // copies ~ flushes when the box is on screen; maps ~ copies
        // (readbacks landing); busy = in-flight skips; skips =
        // off-screen/cold/refused. svVolCopyHeld - later-bracket vol copies
        // held by the first-bracket latch (expect ~brackets-1 per frame; 0
        // under 170).
        out.push_back(kv("svVolCopyHeld", RenderIntegration::g_svs_vol_copy_held));
        out.push_back(kv("prbCopies", RenderIntegration::g_prb_copies));
        out.push_back(kv("prbMaps", RenderIntegration::g_prb_maps));
        out.push_back(kv("prbBusy", RenderIntegration::g_prb_busy));
        out.push_back(kv("prbSkips", RenderIntegration::g_prb_skips));
        // prbPreSamples > 1 or prbPreArray > 1 = the single-sample gate
        // refused (main depth is MSAA); prbPreIssued counting with the lane
        // still -1 = the 1x1 copy itself, which D3D11 does not permit out of
        // a depth-stencil resource (whole subresource only). Read these
        // BEFORE reading any preBot verdict.
        out.push_back(kv("prbPreFmt", RenderIntegration::g_prb_pre_fmt));
        out.push_back(kv("prbPreSamples", RenderIntegration::g_prb_pre_samples));
        out.push_back(kv("prbPreArray", RenderIntegration::g_prb_pre_array));
        out.push_back(kv("prbPreIssued", RenderIntegration::g_prb_pre_issued));
        out.push_back(kv("prbPreSkips", RenderIntegration::g_prb_pre_skips));
        out.push_back(kv("liveNearRefAdopts", RenderIntegration::g_live_ref_adopts));
        out.push_back(kv("worldPairEncodes", RenderIntegration::g_world_pair_encodes));
        out.push_back(kvf("farKeepFar", RenderIntegration::g_far_keep_far));
        {
            const float khpc_km22 = RenderIntegration::g_part_keep_m22;
            const float khpc_km32 = RenderIntegration::g_part_keep_m32;
            const float khpc_fm22 = RenderIntegration::g_part_frame_m22;
            const float khpc_fm32 = RenderIntegration::g_part_frame_m32;
            const float khpc_bnd = (khpc_fm22 > 1.0e-6f)
                ? khpc_fm32 / (1.0f - khpc_fm22) : -1.0f;
            const float khpc_ndc = (khpc_bnd > 0.0f)
                ? khpc_km22 + khpc_km32 / khpc_bnd : -1.0f;
            const float khpc_den = khpc_fm22 - khpc_ndc;
            const float khpc_app = (khpc_ndc > 0.0f && khpc_den > 1.0e-9f)
                ? khpc_fm32 / khpc_den : -1.0f;
            out.push_back(kvf("partBoundaryM", khpc_bnd));
            out.push_back(kvf("partKeepNdcAtBnd", khpc_ndc));
            out.push_back(kvf("partKeepApparentM", khpc_app));
            out.push_back(kvf("partKeepFar",
                RenderIntegration::kh_enc_far(khpc_km22, khpc_km32)));
            out.push_back(kvf("partFrameFar",
                RenderIntegration::kh_enc_far(khpc_fm22, khpc_fm32)));
            out.push_back(kvf("partKeepNear",
                RenderIntegration::kh_enc_pair_near(khpc_km22, khpc_km32)));
            out.push_back(kvf("partFrameNear",
                RenderIntegration::kh_enc_pair_near(khpc_fm22, khpc_fm32)));
        }
        out.push_back(kvf("partSceneVpLo", RenderIntegration::g_scene_vp_min_d));
        out.push_back(kvf("partSceneVpHi", RenderIntegration::g_scene_vp_max_d));
        out.push_back(kvf("partTrigAccLo", RenderIntegration::g_trig_acc_vp[0]));
        out.push_back(kvf("partTrigAccHi", RenderIntegration::g_trig_acc_vp[1]));
        out.push_back(kvf("partTrigRejLo", RenderIntegration::g_trig_rej_vp[0]));
        out.push_back(kvf("partTrigRejHi", RenderIntegration::g_trig_rej_vp[1]));
        out.push_back(kvf("partRejLoMin", RenderIntegration::g_part_rej_lo_min));
        out.push_back(kvf("partRejLoMax", RenderIntegration::g_part_rej_lo_max));
        out.push_back(kvf("partRejHiMax", RenderIntegration::g_part_rej_hi_max));
        out.push_back(kv("partSkySpans", RenderIntegration::g_part_sky_spans));
        out.push_back(kvf("partMeshNearM", RenderIntegration::g_part_mesh_near_m));
        out.push_back(kvf("partMeshFarM", RenderIntegration::g_part_mesh_far_m));
        out.push_back(kvf("partFkAgeMs", RenderIntegration::g_part_fk_age_ms));
        out.push_back(kvf("partVpLo", RenderIntegration::g_part_vp_lo));
        out.push_back(kvf("partVpHi", RenderIntegration::g_part_vp_hi));
        out.push_back(kvf("partKeepM22", RenderIntegration::g_part_keep_m22));
        out.push_back(kvf("partKeepM32", RenderIntegration::g_part_keep_m32));
        out.push_back(kvf("partFrameM22", RenderIntegration::g_part_frame_m22));
        out.push_back(kvf("partFrameM32", RenderIntegration::g_part_frame_m32));
        out.push_back(kv("farKeepInsideRoutes", RenderIntegration::g_far_keep_inside_routes));
        out.push_back(kv("farClipNoFarVis", RenderIntegration::g_far_clip_no_farvis));
        out.push_back(kv("farClipStale", RenderIntegration::g_far_clip_stale));
        out.push_back(kvf("farClipAccFar", RenderIntegration::g_far_clip_acc_far));
        out.push_back(kvf("liveNearRef", RenderIntegration::g_live_near_ref));
        out.push_back(kvf("fogEngX", RenderIntegration::g_fog_eng_dbg[0]));
        out.push_back(kvf("fogEngEnd", RenderIntegration::g_fog_eng_dbg[1]));
        out.push_back(kvf("fogEngInv", RenderIntegration::g_fog_eng_dbg[2]));
        out.push_back(kvf("fogEngOn", RenderIntegration::g_fog_eng_dbg[3]));
        out.push_back(kvf("latchHoldLastDist", RenderIntegration::g_latch_hold_dist));
        out.push_back(kv("latchJumpAdopts", RenderIntegration::g_latch_jump_adopts));
        // KH_LATCH_TRAJECTORY: escapes won by PROGRESSION rather than by the
        // 1000 ms lost-adopt. latchHeldCycle is the gate's live verdict at
        // dump time.
        out.push_back(kv("latchTrajAdopts", RenderIntegration::g_latch_traj_adopts));
        out.push_back(kv("latchHeldNow", RenderIntegration::g_latch_held_cycle ? 1ull : 0ull));
        out.push_back(kv("flushBoundaryAdopts", RenderIntegration::g_flush_boundary_adopts));
        out.push_back(kv("dlMode", static_cast<uint64_t>(RenderIntegration::g_dl_mode.load(std::memory_order_relaxed))));
        out.push_back(kv("dlAcquires", RenderIntegration::g_dl.acquires));
        out.push_back(kv("dlCopies", RenderIntegration::g_dl.copies));
        out.push_back(kv("dlHarvests", static_cast<uint64_t>(RenderIntegration::g_dl.serial)));
        out.push_back(kv("dlLastPointN", static_cast<uint64_t>(RenderIntegration::g_dl.point_n)));
        out.push_back(kv("dlLastSpotN", static_cast<uint64_t>(RenderIntegration::g_dl.spot_n)));
        out.push_back(kvf("dlListAgeS", age_s(RenderIntegration::g_dl.stamp_ms)));
        out.push_back(kv("dlHashChanges", RenderIntegration::g_dl.hash_changes));
        out.push_back(kv("dlValRejects", RenderIntegration::g_dl.val_rejects));
        out.push_back(kv("dlSlotNulls", RenderIntegration::g_dl.slot_nulls));
        out.push_back(kv("dlStillDrawing", RenderIntegration::g_dl.still_drawing));
        out.push_back(kv("dlBoundsRejects", RenderIntegration::g_dl.bounds_rejects));
        out.push_back(kv("dlPerDrawDistinct", static_cast<uint64_t>(RenderIntegration::g_dl_cd_distinct_last)));
        out.push_back(kv("dlPerDrawDistinctMax", static_cast<uint64_t>(RenderIntegration::g_dl_cd_distinct_max)));
        out.push_back(kv("dlPoolN", static_cast<uint64_t>(RenderIntegration::g_dl.pool_n)));
        out.push_back(kv("dlWinCaptured", RenderIntegration::g_dl.win_captured));
        out.push_back(kv("dlListsAnchored", RenderIntegration::g_dl.lists_anchored));
        out.push_back(kv("dlListsUnanchored", RenderIntegration::g_dl.lists_unanchored));
        out.push_back(kv("dlPoolExpired", RenderIntegration::g_dl.pool_expired));
        // Zero with lights present means the decay never engages and the
        // tails are NOT the high-water mark.
        out.push_back(kv("dlGapDecays", RenderIntegration::g_dl_gap_decays));
        out.push_back(kv("dlFillLastN", static_cast<uint64_t>(RenderIntegration::g_dl.fill_last_n)));
        out.push_back(kv("dlSpotFlips", RenderIntegration::g_dl.pool_spot_flips));
        out.push_back(kv("dlWinAux", RenderIntegration::g_dl.win_aux));
        out.push_back(kv("dlOriginRangeRejects", RenderIntegration::g_dl.origin_range_rejects));
        out.push_back(kv("dlOriginPoolVotes", RenderIntegration::g_dl.origin_pool_votes));
        out.push_back(kv("dlOriginVoteAmbiguous", RenderIntegration::g_dl.origin_vote_ambiguous));
        out.push_back(kv("dlAnchorTableN", static_cast<uint64_t>(RenderIntegration::g_dl.anchor_n)));
        {
            // Band-layout census (the 132 m band session): the widest valid
            // band's span and the layout's total reach - the receive decode
            // was built against 8-35 m bands.
            float khb_max_far = 0.0f, khb_widest = 0.0f;
            int khb_valid = 0;

            for (int bi = 0; bi < 8; ++bi) {
                const auto& kb = RenderIntegration::g_ls.band[bi];
                if (!kb.valid) continue;
                khb_valid++;
                if (kb.border[1] > khb_max_far) khb_max_far = kb.border[1];
                const float w = kb.border[1] - kb.border[0];
                if (w > khb_widest) khb_widest = w;
            }

            out.push_back(kvf("bandMaxFar", khb_max_far));
            out.push_back(kvf("bandWidest", khb_widest));
            out.push_back(kv("bandValidN", static_cast<uint64_t>(khb_valid)));
        }
        out.push_back(kv("vaShaped", RenderIntegration::g_va_shaped));
        out.push_back(kvf("vaTmagMin", RenderIntegration::g_va_tmag_min));
        out.push_back(kv("viewSrcMisses", static_cast<uint64_t>(RenderIntegration::g_ls.view_src_miss)));
        out.push_back(kv("viewCandN", static_cast<uint64_t>(RenderIntegration::g_ls.vc_n)));
        out.push_back(kvf("viewPubRotErr", RenderIntegration::g_ls.last_publish_rot_err));
        out.push_back(kvf("viewPubExactAgeS", age_s(RenderIntegration::g_ls.pub_exact_ms)));
        out.push_back(kvf("viewPubAnyAgeS", age_s(RenderIntegration::g_ls.pub_any_ms)));
        out.push_back(kvf("viewPubFreshAgeS", age_s(RenderIntegration::g_ls.pub_fresh_ms)));
        out.push_back(kv("litGraceSaves", RenderIntegration::g_lit_grace_saves));
        out.push_back(kvf("litGateSinceAgeS", age_s(RenderIntegration::g_lit_gate_since_ms)));
        out.push_back(kvf("missLastAgeS", age_s(RenderIntegration::g_ms_ms)));
        out.push_back(kv("rtLastRejW", static_cast<uint64_t>(RenderIntegration::g_rt_last_rej_w)));
        out.push_back(kvf("fogColR", RenderIntegration::g_fog_dbg[0]));
        out.push_back(kvf("fogColG", RenderIntegration::g_fog_dbg[1]));
        out.push_back(kvf("fogColB", RenderIntegration::g_fog_dbg[2]));
        out.push_back(kvf("fogEnabled", RenderIntegration::g_fog_dbg[3]));
        out.push_back(kvf("fogTgtR", RenderIntegration::g_sky_probe.nb[4]));
        out.push_back(kvf("fogTgtG", RenderIntegration::g_sky_probe.nb[5]));
        out.push_back(kvf("fogTgtB", RenderIntegration::g_sky_probe.nb[6]));
        out.push_back(kvf("trigRejMin", RenderIntegration::g_trig_rej_vp[0]));
        out.push_back(kvf("trigRejMax", RenderIntegration::g_trig_rej_vp[1]));
        out.push_back(kvf("trigAccMin", RenderIntegration::g_trig_acc_vp[0]));
        out.push_back(kvf("trigAccMax", RenderIntegration::g_trig_acc_vp[1]));

        {   // finest published cascade, world meters per shadow texel: the
            // receive-resolution question in one number (compare across
            // sessions; if it grew, fine cascades stopped entering the
            // table). Diagnostic read of render-written state, like the rest
            // of the stats.
            float finest = -1.0f;
            int   valid = 0;

            for (uint32_t i = 0; i < RenderIntegration::KH_LIVE_MAX_CASCADES; ++i) {
                const auto& e = RenderIntegration::g_ls.entries[i];
                if (e.tile[2] <= 0.0f || e.stamp == 0) continue;
                valid++;
                const float ilen = sqrtf(e.m[0] * e.m[0] + e.m[3] * e.m[3] + e.m[6] * e.m[6]);
                if (ilen <= 1e-9f) continue;
                const float texels = e.tile[2] * static_cast<float>(RenderIntegration::g_ls.atlas_size);
                if (texels <= 0.0f) continue;
                const float wpt = (2.0f / ilen) / texels;
                if (finest < 0.0f || wpt < finest) finest = wpt;
            }

            out.push_back(kvf("liveFinestWpt", finest));
            out.push_back(kv("liveValidEntries", static_cast<uint64_t>(valid)));
        }

        out.push_back(kv("locScanUploads", RenderIntegration::g_loc_scan_uploads));
        out.push_back(kv("locMaxCbFloats", RenderIntegration::g_loc_max_cb_floats));

        // ===== DEBUG DEEPENING: instantaneous state sections. Every key
        // below is a plain read of standing state at call time - no new
        // per-frame cost anywhere; the one lock taken (cache census) parks
        // the render thread exactly like dumpDynamicLights' snapshot does.

        // Collapse-guard forensics: the standing reference pair, the pending
        // jump's age in both layers' terms, and the witness recency the guard
        // tests against (15 s bar).
        out.push_back(kv("fkVetoCandLast", static_cast<uint64_t>(RenderIntegration::g_fk_veto_cand_n)));
        // flushes where the mirror was NOT the arbitration's certified
        // standing, so the published block held instead of shipping
        // uncertified pass content (the slight camera-rotation dimming).
        out.push_back(kv("blkIncohHolds", RenderIntegration::g_blk_incoh_holds));
        out.push_back(kvf("stdAmbLum", RenderIntegration::g_light_probe.std_amb_l));
        out.push_back(kvf("stdSunLum", RenderIntegration::g_light_probe.std_sun_l));
        out.push_back(kvf("blkPendAgeS", age_s(RenderIntegration::g_blk_pend_ms)));
        out.push_back(kvf("sunLastJumpAgeS", age_s(RenderIntegration::g_sun_last_jump_ms)));

        {   // MESH REGISTRY CENSUS: published-contract reads (lock-free by)
            // the registry's acquire/release design) - registration health
            // and CPU footprint at a glance.
            uint64_t khm_verts = 0, khm_max = 0;
            const uint32_t khm_n = RenderIntegration::mesh_count();

            for (uint32_t khm_i = 0; khm_i < khm_n; ++khm_i) {
                const uint64_t khm_vc = RenderIntegration::mesh_def(static_cast<int>(khm_i)).verts.size();
                khm_verts += khm_vc;
                if (khm_vc > khm_max) khm_max = khm_vc;
            }

            out.push_back(kv("meshVertsTotal", khm_verts));
            out.push_back(kv("meshVertsMax", khm_max));
            out.push_back(kv("meshBytesCpu", khm_verts * static_cast<uint64_t>(sizeof(RenderIntegration::MeshVertex))));
        }

        {   // DRAW-LIST CENSUS: the live retained-object mix under the list
            // mutex (the same lock every SQF mutator takes; game thread, so
            // no render-side contention) - names the workload shape any
            // capture ran under, feature by feature.
            std::lock_guard<std::mutex> khdl_g(RenderIntegration::g_draw_list_mutex);
            uint64_t khdl_vis = 0, khdl_solid = 0, khdl_fx = 0, khdl_fs = 0,
                     khdl_ui = 0, khdl_overlay = 0, khdl_write = 0, khdl_lit = 0,
                     khdl_farvis = 0, khdl_rot = 0, khdl_cull = 0, khdl_tex = 0,
                     khdl_ushdr = 0, khdl_blend = 0, khdl_band = 0, khdl_loc = 0,
                     khdl_timed = 0, khdl_mesh = 0, khdl_uiOnly = 0;

            for (const auto& khdl_kv : RenderIntegration::g_draw_list) {
                const RenderIntegration::RenderObject& khdl_o = khdl_kv.second;
                if (khdl_o.visible) khdl_vis++;
                if (khdl_o.fullscreen) {
                    khdl_fs++;
                    if (khdl_o.affect_ui) khdl_ui++;
                    if (khdl_o.affect_ui && khdl_o.ui_only) khdl_uiOnly++;
                }
                else if (khdl_o.effect != 0) khdl_fx++;
                else khdl_solid++;
                if (khdl_o.mode == RenderIntegration::DepthMode::Off) khdl_overlay++;
                if (khdl_o.mode == RenderIntegration::DepthMode::TestWrite) khdl_write++;
                if (khdl_o.lit) khdl_lit++;
                if (khdl_o.far_vis) khdl_farvis++;
                if (khdl_o.rotated) khdl_rot++;
                if (!khdl_o.two_sided) khdl_cull++;
                if (khdl_o.materials) khdl_tex++;
                if (!khdl_o.fx_shader.empty()) khdl_ushdr++;
                if (khdl_o.blend_mode != 0) khdl_blend++;
                if (khdl_o.banded) khdl_band++;
                if (khdl_o.localized) khdl_loc++;
                if (khdl_o.timed) khdl_timed++;
                if (khdl_o.mesh != 0) khdl_mesh++;
            }

            out.push_back(kv("objTotal", static_cast<uint64_t>(RenderIntegration::g_draw_list.size())));
            out.push_back(kv("objVisible", khdl_vis));
            out.push_back(kv("objSolid", khdl_solid));
            out.push_back(kv("objEffect", khdl_fx));
            out.push_back(kv("objFullscreen", khdl_fs));
            out.push_back(kv("objAffectUi", khdl_ui));
            out.push_back(kv("objUiOnly", khdl_uiOnly));   // UI-mode passes
            out.push_back(kv("objOverlay", khdl_overlay));
            out.push_back(kv("objDepthWrite", khdl_write));
            out.push_back(kv("objLit", khdl_lit));
            out.push_back(kv("objFarVis", khdl_farvis));
            out.push_back(kv("objRotated", khdl_rot));
            out.push_back(kv("objBackfaceCull", khdl_cull));
            out.push_back(kv("objTextured", khdl_tex));
            out.push_back(kv("objCustomShader", khdl_ushdr));
            out.push_back(kv("objBlendNonNormal", khdl_blend));
            out.push_back(kv("objBanded", khdl_band));
            out.push_back(kv("objLocalized", khdl_loc));
            out.push_back(kv("objTimed", khdl_timed));
            out.push_back(kv("objNonBoxMesh", khdl_mesh));
        }

        {   // RESOURCE + CACHE CENSUS: the caches are written only inside
            // serialized graphics windows, so the read parks the render
            // thread through the graphics lock (dumpDynamicLights' snapshot
            // pattern). -1 everywhere = lock unavailable this call; the keys
            // heal on the next call.
            float khrc_tex = -1.0f, khrc_tex_fail = -1.0f;
            float khrc_ps = -1.0f, khrc_ps_fail = -1.0f, khrc_vb = -1.0f;

            for (int khrc_a = 0; khrc_a < 4; ++khrc_a) {
                RVExtBridge::ScopedGraphicsLock khrc_lock;
                if (!khrc_lock.acquired()) continue;
                khrc_tex = 0.0f; khrc_tex_fail = 0.0f;

                for (const auto& khrc_e : RenderIntegration::g_tex_cache) {
                    khrc_tex += 1.0f;
                    if (khrc_e.second.failed) khrc_tex_fail += 1.0f;
                }

                khrc_ps = 0.0f; khrc_ps_fail = 0.0f;

                for (const auto& khrc_e : RenderIntegration::g_user_ps_cache) {
                    khrc_ps += 1.0f;
                    if (khrc_e.second.failed) khrc_ps_fail += 1.0f;
                }

                khrc_vb = static_cast<float>(RenderIntegration::g_res.mesh_vb.size());
                break;
            }

            out.push_back(kvf("texCacheEntries", khrc_tex));
            out.push_back(kvf("texCacheFailed", khrc_tex_fail));
            out.push_back(kvf("userShaderEntries", khrc_ps));
            out.push_back(kvf("userShaderFailed", khrc_ps_fail));
            out.push_back(kvf("meshVbCount", khrc_vb));
        }

        // ASYNC VISIBILITY PIPELINE: queue depth + result freshness (the
        // queueVisibility / getVisibilityResults pump; all game-thread state,
        // same thread as this call).
        out.push_back(kv("visQueuePending", RenderIntegration::g_query_pending ? 1u : 0u));
        out.push_back(kv("visQueuePoints", static_cast<uint64_t>(RenderIntegration::g_query_points_pending.size() / 3)));
        out.push_back(kv("visResultPoints", static_cast<uint64_t>(RenderIntegration::g_vis_result_count)));
        out.push_back(kvf("visResultLagFrames", RenderIntegration::g_vis_result_frame != 0
            ? static_cast<float>(RenderIntegration::g_flush_frame - RenderIntegration::g_vis_result_frame)
            : -1.0f));
        out.push_back(kv("visInflightA", static_cast<uint64_t>(RenderIntegration::g_async_inflight_count[0])));
        out.push_back(kv("visInflightB", static_cast<uint64_t>(RenderIntegration::g_async_inflight_count[1])));
        out.push_back(kv("flushFrameOrdinal", RenderIntegration::g_flush_frame));

        // PRIVATE SUN-DEPTH MAP: validity, age and geometry (sunMapHalfDiag
        // above reads the same bounds; this names the rest).
        out.push_back(kv("sunMapValid", RenderIntegration::g_sun_map_valid ? 1u : 0u));
        out.push_back(kvf("sunMapAgeS", RenderIntegration::g_sun_map_time >= 0.0f
            ? RenderIntegration::effect_time_seconds() - RenderIntegration::g_sun_map_time
            : -1.0f));
        out.push_back(kv("sunMapSizePx", static_cast<uint64_t>(RenderIntegration::KH_SUN_DEPTH_SIZE)));
        out.push_back(kvf("sunMapCenterX", RenderIntegration::g_sun_map_bounds[0]));
        out.push_back(kvf("sunMapCenterY", RenderIntegration::g_sun_map_bounds[1]));
        out.push_back(kvf("sunMapCenterZ", RenderIntegration::g_sun_map_bounds[2]));

        // HEIGHTFIELD ORIGIN: completes the world mapping next to thmW/H/
        // thmCell above (cross-checkable against getTerrainMatrix).
        out.push_back(kvf("thmOriginX", RenderIntegration::g_thml_origin[0]));
        out.push_back(kvf("thmOriginZ", RenderIntegration::g_thml_origin[1]));

        // INJECTION HEALTH + ACCEPTED ENCODE: age of the last pre-translucent
        // injection (the flush's stand-down bar is 0.5 s) and the last
        // committed pair's far in meters (injDpM22/M32 above are the raw
        // coefficients; this is the derived number).
        out.push_back(kvf("injLastAgeS", age_s(RenderIntegration::g_composite_last_inject_ms.load(std::memory_order_relaxed))));
        out.push_back(kvf("injAccFarM", RenderIntegration::g_inj_dp_valid
            ? RenderIntegration::kh_enc_far(RenderIntegration::g_inj_dp[0], RenderIntegration::g_inj_dp[1])
            : -1.0f));

        // CAMERA ECHO: the injection-recorded render camera (engine axes; Y =
        // altitude ASL) - locates every distance- and altitude-tuned gate in
        // the same dump that reports them.
        out.push_back(kvf("camEngX", RenderIntegration::g_ls.cam[0]));
        out.push_back(kvf("camEngAltY", RenderIntegration::g_ls.cam[1]));
        out.push_back(kvf("camEngZ", RenderIntegration::g_ls.cam[2]));

        // RING GEOMETRY + TIME ANCHORS: lets a paired stats + trace read be
        // aligned without guessing (ffrFrames above is the serial; the ring
        // capacity here says how far back a dump can reach).
        out.push_back(kv("ffrRingFrames", static_cast<uint64_t>(RenderIntegration::KH_FFR_RING)));
        out.push_back(kvf("effectTimeS", RenderIntegration::effect_time_seconds()));
        return game_value(std::move(out));
    } catch (...) {
        report_error("getRenderStats: unknown exception");
        return game_value(auto_array<game_value>());
    }
}

// Each frame's values align 1:1 with "fields". The ring is copied under the
// graphics lock (parking the render thread - the ring's write invariant) and
// formatted after release; the newest 256 populated records are dumped.
static game_value dump_render_trace_sqf() {
    try {
        // OPT-IN (see g_diag_armed): the recorder is idle until armed, so the
        // first call starts it and reports that; frames flow from the next
        // call on.
        if (!RenderIntegration::g_diag_armed.exchange(true, std::memory_order_relaxed)) {
            auto_array<game_value> pair;
            pair.push_back(game_value("status"));
            pair.push_back(game_value("armed"));
            auto_array<game_value> armed_out;
            armed_out.push_back(game_value(std::move(pair)));
            return game_value(std::move(armed_out));
        }

        constexpr uint32_t KHT_MAX = 512;
        static RenderIntegration::FfrRecord khtr_snap[RenderIntegration::KH_FFR_RING];
        uint32_t khtr_head = 0;
        uint32_t khtr_serial = 0;
        bool khtr_got = false;

        for (int attempt = 0; attempt < 4 && !khtr_got; ++attempt) {
            RVExtBridge::ScopedGraphicsLock lock;

            if (!lock.acquired()) continue;
            memcpy(khtr_snap, RenderIntegration::g_ffr, sizeof(khtr_snap));
            khtr_head = RenderIntegration::g_ffr_head;
            khtr_serial = RenderIntegration::g_ffr_serial;
            khtr_got = true;
        }

        auto_array<game_value> out;

        if (!khtr_got) {
            auto_array<game_value> pair;
            pair.push_back(game_value("status"));
            pair.push_back(game_value("lockFailed"));
            out.push_back(game_value(std::move(pair)));
            return game_value(std::move(out));
        }

        static const char* const KHT_FIELDS[] = {
            "serial", "ageS", "pvValid", "pvStale", "retryFixed",
            "attempts", "opqAtInject", "opqFinal", "pfDelta", "lockFolds",
            "stage", "hadObjects", "compHealthy", "injSinceFlush",
            "repainted", "anomaly", "pvSrc", "eligible", "meshesSnap",
            "fsSnap", "pvNear", "dark", "erased", "rescues",
            "camLsDelta", "rejVpMin", "rejVpMax",
            "injNear", "slotNearInj", "encSrc", "injBandRej",
            "litGate", "sunOk", "mapOk", "bandOn", "dlN", "relock",
            "packOn", "mapAgeS", "pubExactAgeS", "pubAnyAgeS",
            "blkValid", "blkMode", "blkErr", "blkAmbLum", "blkSunLum",
            "pubFreshAgeS",
        };
        constexpr uint32_t KHT_NFIX = sizeof(KHT_FIELDS) / sizeof(KHT_FIELDS[0]);

        auto_array<game_value> names;

        for (uint32_t i = 0; i < KHT_NFIX; ++i) names.push_back(game_value(KHT_FIELDS[i]));

        for (uint32_t i = 0; i < RenderIntegration::KH_FFR_NDELTA; ++i) {
            names.push_back(game_value(RenderIntegration::KH_FFR_DELTA_NAMES[i]));
        }

        names.push_back(game_value("camStepM"));
        names.push_back(game_value("fireCamDeltaM"));
        names.push_back(game_value("fireDimsDivPx"));
        names.push_back(game_value("fireVpMode"));
        names.push_back(game_value("maskRtBinds"));
        names.push_back(game_value("maskLastBindD"));
        names.push_back(game_value("fireFirstD"));
        names.push_back(game_value("fireLastD"));
        names.push_back(game_value("firePaintIx"));
        names.push_back(game_value("firePaintPath"));   // 1 SRV-bound, 2 backstop
        names.push_back(game_value("maskSrvFirstD"));
        names.push_back(game_value("maskSrvLastD"));
        names.push_back(game_value("fireSrvGapD"));
        names.push_back(game_value("maskFirstBindD"));
        names.push_back(game_value("pubFirstD"));   // publish census
        names.push_back(game_value("pubRejFirstD"));
        names.push_back(game_value("pubSlotHits"));
        names.push_back(game_value("pubPreAccept"));   // 0 = slot was SILENT
        names.push_back(game_value("pubRejTmag"));
        names.push_back(game_value("pubRejFamily"));
        names.push_back(game_value("pubRejCold"));
        names.push_back(game_value("pubAltFirstD"));
        names.push_back(game_value("pubAltOff"));
        names.push_back(game_value("latchLiveDeltaM"));
        names.push_back(game_value("latchAgeMs"));
        names.push_back(game_value("injViewSrc"));
        names.push_back(game_value("fvAdopt"));
        names.push_back(game_value("fvAgeMs"));
        names.push_back(game_value("injRotDeltaDeg"));

        names.push_back(game_value("injFovY"));
        names.push_back(game_value("liveNearInj"));
        names.push_back(game_value("injFar"));
        names.push_back(game_value("liveFar"));
        names.push_back(game_value("slotFar"));
        names.push_back(game_value("injVpMin"));
        names.push_back(game_value("injVpMax"));
        names.push_back(game_value("fkRouted"));   // C8  lanes
        names.push_back(game_value("fkVetoN"));
        names.push_back(game_value("fkFar"));
        names.push_back(game_value("slcNear"));
        names.push_back(game_value("slcFar"));
        names.push_back(game_value("cycVpLo"));
        names.push_back(game_value("cycVpHi"));
        names.push_back(game_value("skyTrigs"));   // sky-window census
        names.push_back(game_value("vetoCandN"));   // lanes (at FfrRecord)
        names.push_back(game_value("camAltM"));
        names.push_back(game_value("stdAmbLum"));
        names.push_back(game_value("stdSunLum"));
        names.push_back(game_value("blkPendAgeS"));

        // append-only tail: registration + first-person forensics. Keep 1:1
        // with the pushes below - fieldsN/frameN now verify that in-band on
        // every dump.
        names.push_back(game_value("reprojPx"));   // |footprint - visible box| this frame, px
        names.push_back(game_value("swingPx"));   // what view adoption moved the footprint, px
        names.push_back(game_value("seamPvStamp"));   // latch write serial at the seam's read (0 = seam idle)
        names.push_back(game_value("compPvStamp"));   // latch write serial at the colour read (0 = no injection)
        names.push_back(game_value("seamCamDxM"));   // seam-published vs colour camera, m (-1 = no publication)
        names.push_back(game_value("seamViewAge"));   // seam-view age at the colour pass, frames (-1 = absent)
        names.push_back(game_value("seamSrc"));   // 0 none 1 adopted 2 cycle 3 live 4 boundary 5 share
        names.push_back(game_value("compSrc"));   // 0 latch 1 live 2 boundary 3 adopted 4 seam-copy 5 bpv-override
        names.push_back(game_value("volCode"));   // shader arm this frame
        names.push_back(game_value("volWit"));   // footprint witness present this frame
        names.push_back(game_value("volAge"));   // volume-copy age, frames (-1 = none)
        names.push_back(game_value("seamLiveDxM"));   // live translation correction applied, m (-1 = stood down)
        names.push_back(game_value("seamNear"));   // the seam's committed near - injNear's twin
        names.push_back(game_value("seamProjSrc"));   // 0 latch pair 1 engine sniff 2 live pair
        names.push_back(game_value("sniffNear"));   // the engine's sniffed pair at the seam (-1 = invalid)
        names.push_back(game_value("primeCamDxM"));   // |colour rebase cam - seam prime cam|, m
        names.push_back(game_value("seamRuns"));   // seam executions folded into this row
        // append-only tail: pair-arbitration forensics + bound-CB census.
        names.push_back(game_value("seamLiveNear"));   // raw live-bridge near at the seam fetch (-1 = none)
        names.push_back(game_value("seamPairWhy"));   // 0 live-corr 1 live-base 2 insane 3 corridor 4 no-ref 5 no sample
        names.push_back(game_value("boundPairNear"));   // bound-CB census pair at the seam (-1 = no match)
        names.push_back(game_value("boundPairAgeMs"));   // age of that buffer's last recorded upload
        names.push_back(game_value("censusMatch"));   // bit0-3 VS b0-b3, bit4-7 PS b0-b3 with a census hit
        // append-only tail: census camera + trajectory-bound forensics.
        names.push_back(game_value("boundCamDxM"));   // |census camera - seam draw camera| (m; -1 = none)
        names.push_back(game_value("boundCamLiveDxM"));   // |census camera - live fetch camera| (m; -1 = none)
        names.push_back(game_value("boundCamOk"));   // census camera validated this frame
        names.push_back(game_value("seamTrnEvent"));   // 0 none 1 bound-refused 2 wide-take 3 escape-take, 5 witness-take
        names.push_back(game_value("seamTrnWit"));   // floor-refusal witness verdict (-1 none, 0 fail/abstain, 1 pass)
        // |adopted-view camera - cycle latch camera|, metres; -1 = no latch
        // reference that run.
        names.push_back(game_value("seamAnchDxM"));
        names.push_back(game_value("prbCtrZ"));
        names.push_back(game_value("prbCtrDM"));
        names.push_back(game_value("prbCtrDzM"));
        names.push_back(game_value("prbGndZ"));
        names.push_back(game_value("prbGndDM"));
        names.push_back(game_value("prbBotZ"));
        names.push_back(game_value("prbBotDM"));
        names.push_back(game_value("prbVolZ"));
        names.push_back(game_value("prbVolGndZ"));
        names.push_back(game_value("prbCnt"));
        names.push_back(game_value("seamFovLatR"));
        names.push_back(game_value("cycN"));
        names.push_back(game_value("cycWorldN"));
        names.push_back(game_value("injCycIdx"));
        names.push_back(game_value("prbPreBotZ"));
        names.push_back(game_value("prbPreBotDM"));
        // append-only tail: the colour pass's applied FOV ratio. seamFovLatR
        // says how far apart frame k and k+1 are in scale; compFovR says
        // whether the VISIBLE box closed that gap this frame (-1 = refused or
        // absent, i.e. it rasterised at the old scale).
        names.push_back(game_value("compFovR"));
        // append-only tail: the near the seam's arbitration chain would have
        // encoded on frames the sniff referee replaced it (-1 = none).
        // seamNear now always reports what actually shipped.
        names.push_back(game_value("seamSnifOvr"));
        names.push_back(game_value("volPassNear"));
        names.push_back(game_value("volPassM22"));
        // the readback age in TRACE ROWS - the sample belongs to row (serial
        // - age). Without it the alternating near classes let two different
        // alignments fit the same data, which is what left unreadable.
        // volDrawNear is the same measurement at the engine's first COUNTING
        // draw, which is where the value is consumed.
        names.push_back(game_value("volPassAge"));
        names.push_back(game_value("volDrawNear"));
        names.push_back(game_value("volDrawAge"));
        // the box centre's clip.w through the ENGINE's own matrix, taken both
        // ways, against its known distance. Whichever of Abs/Rel equals True
        // names the space the engine's matrix expects - and so which of mode
        // 174 / 175 transforms our footprint correctly.
        names.push_back(game_value("engVpWAbs"));
        names.push_back(game_value("engVpWRel"));
        names.push_back(game_value("engVpWTrue"));
        // camAbsM is the amplifier it multiplies (the absolute map magnitude
        // of the camera). compTakeSrc: 0 no take, 1 snapped (bit-identical
        // basis, translation untouched), 2 rebuilt through the exact camera,
        // 3 refused.
        names.push_back(game_value("compCamRtM"));
        names.push_back(game_value("seamCamRtM"));
        names.push_back(game_value("camAbsM"));
        names.push_back(game_value("compTakeSrc"));
        // the depth-encode route the VISIBLE box took this frame (0 ordinary,
        // 1 near-gap SV_Depth, 2 far-keep). The seam has no such routing, so
        // a non-zero here is a frame where the box and its own footprint were
        // in different encodes - the standing suspect for the sub-1 m
        // zoom/fire size pop if does not close it.
        names.push_back(game_value("compNearZ"));
        // APPEND-ONLY TAIL: the caster centre through the ENGINE'S own
        // view-projection versus through the SEAM'S, in pixels. Compare
        // volDrawSwingPx against swingPx ROW BY ROW - that pair is the whole
        // question. -1 = no sample this row.
        names.push_back(game_value("seamFetchDupM"));   // (mis-scoped)
        names.push_back(game_value("compCamStepM"));
        names.push_back(game_value("seamTrnDxM"));
        names.push_back(game_value("seamTrnBndM"));
        names.push_back(game_value("seamFetchN"));   // seam
        names.push_back(game_value("seamBoundN"));
        names.push_back(game_value("bandPickAgeMs"));   // cascades
        names.push_back(game_value("bandPickCamDxM"));
        names.push_back(game_value("bandAltCamDxM"));
        names.push_back(game_value("bandAltBetter"));
        names.push_back(game_value("bandPickNoneN"));
        names.push_back(game_value("bandPoolMissN"));
        names.push_back(game_value("bandStageWaitMs"));
        names.push_back(game_value("stenTol"));   // blur
        names.push_back(game_value("volPassSwingPx"));
        names.push_back(game_value("volDrawSwingPx"));
        // append-only tail: receive coverage. Keep 1:1 with the pushes below
        // - fieldsN/frameN verify it in-band on every dump.
        names.push_back(game_value("recvBandsN"));   // bands committed this frame
        names.push_back(game_value("recvBandNear"));   // finest committed band near (m)
        names.push_back(game_value("recvDropWhy"));   // bitmask of the gates that dropped one
        names.push_back(game_value("recvBandAgeMs"));   // seal age of the finest band
        names.push_back(game_value("recvBandCamDxM"));   // its baked camera vs the pass camera
        names.push_back(game_value("vmirDraws"));   // re-issued counting draws
        names.push_back(game_value("blkMirSunLum"));   // mirror at the row latch
        names.push_back(game_value("blkAnchSunLum"));   // anchor at the row latch
        names.push_back(game_value("dBlkApplies"));   // applies this frame
        names.push_back(game_value("dBlkAnchorRej"));   // anchor rejects this frame
        // append-only tail: the two instruments (keep 1:1 with the pushes)
        names.push_back(game_value("bandEscLandFr"));   // escape->land cycles of a landing this row (-1 none)
        names.push_back(game_value("bandEscLandCdxM"));   // baked cam vs injection cam AT LAND
        names.push_back(game_value("bandResealStepM"));   // largest baked-camera jump of a reseal this row
        names.push_back(game_value("arbOn"));   // injection ran the analytic arb PS
        names.push_back(game_value("arbJitPairM"));   // predicted decode error from pair churn (m)
        names.push_back(game_value("arbJitCamM"));   // ... from camera advance since the snapshot (m)
        names.push_back(game_value("arbJitVertM"));   // vertical component at the reconstructed point (m)
        names.push_back(game_value("bandUvDriftPx"));
        // append-only tail: WELD IDENTITY CENSUS (keep 1:1 with the pushes;
        // fieldsN 222 -> 225, in-band audited).
        names.push_back(game_value("khWeldInjDxM"));   // worst weld vs injection cam (m)
        names.push_back(game_value("khWeldRotDeg"));   // its forward-axis rotation (deg)
        names.push_back(game_value("khWeldCode"));   // 1 flip 2 single 3 late 4 base 5 s335 6 imm // sealed-vs-live uv of the probe point (texels)
        names.push_back(game_value("khLiveTab"));   // live table size at fill
        names.push_back(game_value("khLiveServed"));   // live cascades written (0 = gated/empty)
        names.push_back(game_value("khSunSt"));   // 0 cold 1 unstable 2 settled -1 no fill
        names.push_back(game_value("khWipeLo"));   // wipe counter low bits

        // fieldsN/frameN are the machine's own numbers riding every dump, so
        // alignment can never again be a hand-count debate. THE COUNT IS
        // VERIFIED IN-BAND.
        const uint32_t khtr_nfields = static_cast<uint32_t>(names.size());
        uint32_t khtr_nrow = 0;

        const uint64_t khtr_now = RenderIntegration::steady_now_ms();
        auto_array<game_value> frames;

        for (uint32_t k = 0; k < RenderIntegration::KH_FFR_RING; ++k) {
            const uint32_t idx = (khtr_head + 1u + k) % RenderIntegration::KH_FFR_RING;
            const RenderIntegration::FfrRecord& r = khtr_snap[idx];

            if (r.serial == 0) continue;
            if (khtr_serial > KHT_MAX && r.serial + KHT_MAX <= khtr_serial) continue;

            auto_array<game_value> f;
            f.push_back(game_value(static_cast<float>(r.serial)));
            f.push_back(game_value(r.t_ms ? static_cast<float>(khtr_now - r.t_ms) / 1000.0f : -1.0f));
            f.push_back(game_value(static_cast<float>(r.pv_valid)));
            f.push_back(game_value(static_cast<float>(r.pv_stale)));
            f.push_back(game_value(static_cast<float>(r.retry_fixed)));
            f.push_back(game_value(static_cast<float>(r.attempts)));
            f.push_back(game_value(r.opq_at_inject == 0xFFFFFFFFu ? -1.0f : static_cast<float>(r.opq_at_inject)));
            f.push_back(game_value(static_cast<float>(r.opq_final)));
            f.push_back(game_value(r.pf_delta == 0xFFFFFFFFu ? -1.0f : static_cast<float>(r.pf_delta)));
            f.push_back(game_value(static_cast<float>(r.lockfail_folded)));
            f.push_back(game_value(static_cast<float>(r.stage)));
            f.push_back(game_value(static_cast<float>(r.had_objects)));
            f.push_back(game_value(static_cast<float>(r.comp_healthy)));
            f.push_back(game_value(static_cast<float>(r.inj_since_flush)));
            f.push_back(game_value(static_cast<float>(r.repainted)));
            f.push_back(game_value(static_cast<float>(r.anomaly)));
            f.push_back(game_value(static_cast<float>(r.pv_src)));
            f.push_back(game_value(static_cast<float>(r.eligible)));
            f.push_back(game_value(static_cast<float>(r.meshes_snap)));
            f.push_back(game_value(static_cast<float>(r.fs_snap)));
            f.push_back(game_value(r.pv_near));
            f.push_back(game_value(static_cast<float>(r.dark)));
            f.push_back(game_value(static_cast<float>(r.erased)));
            f.push_back(game_value(static_cast<float>(r.rescues)));
            f.push_back(game_value(r.cam_ls_delta));
            f.push_back(game_value(r.rej_vp_min));
            f.push_back(game_value(r.rej_vp_max));
            f.push_back(game_value(r.inj_near));
            f.push_back(game_value(r.slot_near_inj));
            f.push_back(game_value(static_cast<float>(r.enc_src)));
            f.push_back(game_value(static_cast<float>(r.inj_band_rej)));
            f.push_back(game_value(static_cast<float>(r.lit_gate)));
            f.push_back(game_value(static_cast<float>(r.sun_ok)));
            f.push_back(game_value(static_cast<float>(r.map_ok)));
            f.push_back(game_value(static_cast<float>(r.band_on)));
            f.push_back(game_value(static_cast<float>(r.dl_n_fill)));
            f.push_back(game_value(static_cast<float>(r.relock_tick)));
            f.push_back(game_value(static_cast<float>(r.pack_on)));
            f.push_back(game_value(r.map_age_s));
            f.push_back(game_value(r.pub_exact_age_s));
            f.push_back(game_value(r.pub_any_age_s));
            f.push_back(game_value(static_cast<float>(r.blk_valid)));
            f.push_back(game_value(static_cast<float>(r.blk_mode)));
            f.push_back(game_value(r.blk_err));
            f.push_back(game_value(r.blk_amb_lum));
            f.push_back(game_value(r.blk_sun_lum));
            f.push_back(game_value(r.pub_fresh_age_s));

            for (uint32_t i = 0; i < RenderIntegration::KH_FFR_NDELTA; ++i) {
                f.push_back(game_value(static_cast<float>(r.d[i])));
            }

            f.push_back(game_value(r.cam_step_m));
            f.push_back(game_value(r.fire_cam_delta_m));
            f.push_back(game_value(r.fire_dims_div_px));
            f.push_back(game_value(static_cast<float>(r.fire_vp_mode)));
            f.push_back(game_value(static_cast<float>(r.mask_rt_binds)));
            f.push_back(game_value(r.mask_last_bind_d));
            f.push_back(game_value(r.fire_first_d));
            f.push_back(game_value(r.fire_last_d));
            f.push_back(game_value(static_cast<float>(r.fire_paint_ix)));
            f.push_back(game_value(static_cast<float>(r.fire_paint_path)));
            f.push_back(game_value(r.mask_srv_first_d));
            f.push_back(game_value(r.mask_srv_last_d));
            f.push_back(game_value(r.fire_srv_gap_d));
            f.push_back(game_value(r.mask_first_bind_d));
            f.push_back(game_value(r.pub_first_d));   // publish census
            f.push_back(game_value(r.pub_rej_first_d));
            f.push_back(game_value(static_cast<float>(r.pub_slot_hits)));
            f.push_back(game_value(static_cast<float>(r.pub_pre_accept)));
            f.push_back(game_value(static_cast<float>(r.pub_rej_tmag)));
            f.push_back(game_value(static_cast<float>(r.pub_rej_family)));
            f.push_back(game_value(static_cast<float>(r.pub_rej_cold)));
            f.push_back(game_value(r.pub_alt_first_d));
            f.push_back(game_value(static_cast<float>(r.pub_alt_off)));
            f.push_back(game_value(r.latch_live_delta_m));
            f.push_back(game_value(r.latch_age_ms));
            f.push_back(game_value(static_cast<float>(r.inj_view_src)));
            f.push_back(game_value(static_cast<float>(r.fv_adopt)));
            f.push_back(game_value(r.fv_age_ms));
            f.push_back(game_value(r.inj_rot_delta_deg));
            f.push_back(game_value(r.inj_fov_y));
            f.push_back(game_value(r.live_near_inj));
            f.push_back(game_value(r.inj_far));
            f.push_back(game_value(r.live_far));
            f.push_back(game_value(r.slot_far));
            f.push_back(game_value(r.inj_vp_min));
            f.push_back(game_value(r.inj_vp_max));
            f.push_back(game_value(static_cast<float>(r.fk_routed)));   // C8  lanes
            f.push_back(game_value(static_cast<float>(r.fk_veto_n)));
            f.push_back(game_value(r.fk_far));
            f.push_back(game_value(r.slc_near));
            f.push_back(game_value(r.slc_far));
            f.push_back(game_value(r.cyc_vp_lo));
            f.push_back(game_value(r.cyc_vp_hi));
            f.push_back(game_value(static_cast<float>(r.sky_trigs)));   // sky-window census
            f.push_back(game_value(static_cast<float>(r.veto_cand_n)));   // lanes
            f.push_back(game_value(r.cam_alt_m));
            f.push_back(game_value(r.std_amb_lum));
            f.push_back(game_value(r.std_sun_lum));
            f.push_back(game_value(r.blk_pend_age_s));
            // tail - 1:1 with the names above.
            f.push_back(game_value(r.reproj_px));
            f.push_back(game_value(r.swing_px));
            f.push_back(game_value(static_cast<float>(r.seam_pv_stamp)));
            f.push_back(game_value(static_cast<float>(r.comp_pv_stamp)));
            f.push_back(game_value(r.seam_cam_dx_m));
            f.push_back(game_value(r.seam_view_age));
            f.push_back(game_value(static_cast<float>(r.seam_src)));
            f.push_back(game_value(static_cast<float>(r.comp_src)));
            f.push_back(game_value(static_cast<float>(r.vol_code)));
            f.push_back(game_value(static_cast<float>(r.vol_wit)));
            f.push_back(game_value(r.vol_age == 0xFFFFu
                                   ? -1.0f : static_cast<float>(r.vol_age)));
            f.push_back(game_value(r.seam_live_dx_m));
            f.push_back(game_value(r.seam_near));
            f.push_back(game_value(static_cast<float>(r.seam_proj_src)));
            f.push_back(game_value(r.sniff_near));
            f.push_back(game_value(r.prime_cam_dx_m));
            f.push_back(game_value(static_cast<float>(r.seam_runs)));
            f.push_back(game_value(r.seam_live_near));   // tail
            f.push_back(game_value(static_cast<float>(r.seam_pair_why)));
            f.push_back(game_value(r.bound_pair_near));
            f.push_back(game_value(r.bound_pair_age_ms));
            f.push_back(game_value(static_cast<float>(r.census_match)));
            f.push_back(game_value(r.bound_cam_dx_m));   // tail
            f.push_back(game_value(r.bound_cam_live_dx_m));
            f.push_back(game_value(static_cast<float>(r.bound_cam_ok)));
            f.push_back(game_value(static_cast<float>(r.seam_trn_event)));
            f.push_back(game_value(r.seam_trn_wit));
            f.push_back(game_value(r.seam_anch_dx_m));   // tail
            f.push_back(game_value(r.prb_ctr_z));   // tail
            f.push_back(game_value(r.prb_ctr_dm));
            f.push_back(game_value(r.prb_ctr_dz_m));
            f.push_back(game_value(r.prb_gnd_z));
            f.push_back(game_value(r.prb_gnd_dm));
            f.push_back(game_value(r.prb_bot_z));
            f.push_back(game_value(r.prb_bot_dm));
            f.push_back(game_value(r.prb_vol_z));
            f.push_back(game_value(r.prb_vol_gnd_z));
            f.push_back(game_value(r.prb_cnt));
            f.push_back(game_value(r.seam_fov_lat_r));
            f.push_back(game_value(static_cast<float>(r.cyc_n)));
            f.push_back(game_value(static_cast<float>(r.cyc_world_n)));
            f.push_back(game_value(static_cast<float>(r.inj_cyc_idx)));
            f.push_back(game_value(r.prb_pre_bot_z));
            f.push_back(game_value(r.prb_pre_bot_dm));
            f.push_back(game_value(r.comp_fov_r));   // tail
            f.push_back(game_value(r.seam_sniff_ovr));   // tail
            f.push_back(game_value(r.vol_pass_near));   // tail
            f.push_back(game_value(r.vol_pass_m22));
            f.push_back(game_value(r.vol_pass_age));   // tail
            f.push_back(game_value(r.vol_draw_near));
            f.push_back(game_value(r.vol_draw_age));
            f.push_back(game_value(r.eng_vp_w_abs));   // tail
            f.push_back(game_value(r.eng_vp_w_rel));
            f.push_back(game_value(r.eng_vp_w_true));
            f.push_back(game_value(r.comp_cam_rt_m));   // tail
            f.push_back(game_value(r.seam_cam_rt_m));
            f.push_back(game_value(r.cam_abs_m));
            f.push_back(game_value(static_cast<float>(r.comp_take_src)));
            f.push_back(game_value(static_cast<float>(r.comp_near_z)));
            f.push_back(game_value(r.seam_fetch_dup_m));
            f.push_back(game_value(r.comp_cam_step_m));
            f.push_back(game_value(r.seam_trn_dx_m));
            f.push_back(game_value(r.seam_trn_bnd_m));
            f.push_back(game_value(static_cast<float>(r.seam_fetch_n)));
            f.push_back(game_value(static_cast<float>(r.seam_bound_n)));
            f.push_back(game_value(r.band_pick_age_ms));
            f.push_back(game_value(r.band_pick_cam_dx_m));
            f.push_back(game_value(r.band_alt_cam_dx_m));
            f.push_back(game_value(static_cast<float>(r.band_alt_better)));
            f.push_back(game_value(static_cast<float>(r.band_pick_none_n)));
            f.push_back(game_value(static_cast<float>(r.band_pool_miss_n)));
            f.push_back(game_value(r.band_stage_wait_ms));
            f.push_back(game_value(r.sten_tol));
            f.push_back(game_value(r.vol_pass_swing_px));   // tail
            f.push_back(game_value(r.vol_draw_swing_px));
            f.push_back(game_value(static_cast<float>(r.recv_bands_n)));
            f.push_back(game_value(r.recv_band_near));
            f.push_back(game_value(static_cast<float>(r.recv_drop_why)));
            f.push_back(game_value(r.recv_band_age_ms));
            f.push_back(game_value(r.recv_band_cam_dx_m));
            f.push_back(game_value(static_cast<float>(r.vmir_draws)));
            f.push_back(game_value(r.blk_mir_lum));
            f.push_back(game_value(r.blk_anch_lum));
            f.push_back(game_value(static_cast<float>(r.d_blk_applies)));
            f.push_back(game_value(static_cast<float>(r.d_blk_anch_rej)));
            f.push_back(game_value(static_cast<float>(r.band_esc_land_fr)));
            f.push_back(game_value(r.band_esc_land_cdx_m));
            f.push_back(game_value(r.band_reseal_step_m));
            f.push_back(game_value(static_cast<float>(r.arb_on)));
            f.push_back(game_value(r.arb_jit_pair_m));
            f.push_back(game_value(r.arb_jit_cam_m));
            f.push_back(game_value(r.arb_jit_vert_m));
            f.push_back(game_value(r.band_uv_drift_px));
            f.push_back(game_value(r.weld_inj_dx_m));
            f.push_back(game_value(r.weld_inj_rot_deg));
            f.push_back(game_value(r.weld_code));
            f.push_back(game_value(static_cast<float>(r.kh_live_tab)));
            f.push_back(game_value(static_cast<float>(r.kh_live_served)));
            f.push_back(game_value(static_cast<float>(r.kh_sun_st)));
            f.push_back(game_value(static_cast<float>(r.kh_wipe_lo)));
            if (khtr_nrow == 0) khtr_nrow = static_cast<uint32_t>(f.size());
            frames.push_back(game_value(std::move(f)));
        }

        {
            auto_array<game_value> pair;
            pair.push_back(game_value("status"));
            pair.push_back(game_value("ok"));
            out.push_back(game_value(std::move(pair)));
        }

        {   // build verification in-band (field protocol step 1).
            auto_array<game_value> pair;
            pair.push_back(game_value("buildTag"));
            pair.push_back(game_value(static_cast<float>(RenderIntegration::KH_BUILD_TAG)));
            out.push_back(game_value(std::move(pair)));
        }

        {
            auto_array<game_value> pair;
            pair.push_back(game_value("fields"));
            pair.push_back(game_value(std::move(names)));
            out.push_back(game_value(std::move(pair)));
        }

        {
            auto_array<game_value> pair;
            pair.push_back(game_value("frames"));
            pair.push_back(game_value(std::move(frames)));
            out.push_back(game_value(std::move(pair)));
        }

        {   // in-band field/row count - the permanent audit.
            auto_array<game_value> pair;
            pair.push_back(game_value("fieldsN"));
            pair.push_back(game_value(static_cast<float>(khtr_nfields)));
            out.push_back(game_value(std::move(pair)));
        }

        {
            auto_array<game_value> pair;
            pair.push_back(game_value("frameN"));
            pair.push_back(game_value(static_cast<float>(khtr_nrow)));
            out.push_back(game_value(std::move(pair)));
        }

        if (khtr_nrow != 0 && khtr_nrow != khtr_nfields) {
            auto_array<game_value> pair;
            pair.push_back(game_value("alignment"));
            pair.push_back(game_value("MISALIGNED"));
            out.push_back(game_value(std::move(pair)));
        }

        return game_value(std::move(out));
    } catch (...) {
        report_error("dumpRenderTrace: unknown exception");
        return game_value(auto_array<game_value>());
    }
}

static game_value dump_dynamic_lights_sqf() {
    try {
        if (!RenderIntegration::g_dl_recon.exchange(true, std::memory_order_relaxed)) {
            // Arm: census on; counters/mirror zeroed under the lock so the
            // session starts clean. A failed lock still arms - the state then
            // zeroes lazily at the next successful call instead.
            RenderIntegration::g_dl_census_on.store(true, std::memory_order_relaxed);

            for (int attempt = 0; attempt < 4; ++attempt) {
                RVExtBridge::ScopedGraphicsLock lock;
                if (!lock.acquired()) continue;
                RenderIntegration::dynlights_reset_session();
                break;
            }

            auto_array<game_value> pair;
            pair.push_back(game_value("status"));
            pair.push_back(game_value("armed"));
            auto_array<game_value> armed_out;
            armed_out.push_back(game_value(std::move(pair)));
            return game_value(std::move(armed_out));
        }

        // The struct copy includes raw device pointers used only as printed
        // identities - never dereferenced. Copy under the lock, format after
        // release (dumpRenderTrace's pattern).
        static RenderIntegration::DynLightsState khd_snap;
        uint32_t khd_cd_last = 0, khd_cd_max = 0, khd_cd_null = 0, khd_cd_samples = 0;
        bool khd_got = false;

        for (int attempt = 0; attempt < 4 && !khd_got; ++attempt) {
            RVExtBridge::ScopedGraphicsLock lock;
            if (!lock.acquired()) continue;
            khd_snap = RenderIntegration::g_dl;
            khd_cd_last = RenderIntegration::g_dl_cd_distinct_last;
            khd_cd_max = RenderIntegration::g_dl_cd_distinct_max;
            khd_cd_null = RenderIntegration::g_dl_cd_null_last;
            khd_cd_samples = RenderIntegration::g_dl_cd_samples_last;
            khd_got = true;
        }

        if (!khd_got) {
            auto_array<game_value> pair;
            pair.push_back(game_value("status"));
            pair.push_back(game_value("lockFailed"));
            auto_array<game_value> fail_out;
            fail_out.push_back(game_value(std::move(pair)));
            return game_value(std::move(fail_out));
        }

        auto kv = [](const char* k, float v) {
            auto_array<game_value> pair;
            pair.push_back(game_value(k));
            pair.push_back(game_value(v));
            return game_value(std::move(pair));
        };
        auto kvs = [](const char* k, const char* v) {
            auto_array<game_value> pair;
            pair.push_back(game_value(k));
            pair.push_back(game_value(v));
            return game_value(std::move(pair));
        };
        auto kva = [](const char* k, auto_array<game_value>&& v) {
            auto_array<game_value> pair;
            pair.push_back(game_value(k));
            pair.push_back(game_value(std::move(v)));
            return game_value(std::move(pair));
        };
        auto hex64 = [](uint64_t v) {
            // dependency-free formatter (no <cstdio> in the include chain)
            char buf[17];
            const char* khd_dig = "0123456789ABCDEF";
            for (int i = 15; i >= 0; --i) { buf[i] = khd_dig[v & 0xF]; v >>= 4; }
            buf[16] = 0;
            return game_value(buf);
        };
        const uint64_t khd_now = RenderIntegration::steady_now_ms();

        auto_array<game_value> out;
        out.push_back(kvs("status", "ok"));
        out.push_back(kv("buildTag", static_cast<float>(RenderIntegration::KH_BUILD_TAG)));
        out.push_back(kv("mode", static_cast<float>(RenderIntegration::g_dl_mode.load(std::memory_order_relaxed))));

        {
            const uint32_t khd_bits = RenderIntegration::g_dl_intensity_bits.load(std::memory_order_relaxed);
            float khd_int = 1.0f;
            memcpy(&khd_int, &khd_bits, sizeof(khd_int));
            out.push_back(kv("intensity", khd_int));
        }

        out.push_back(kv("mirrorValid", khd_snap.valid ? 1.0f : 0.0f));
        out.push_back(kv("listAgeS", khd_snap.stamp_ms != 0
                                   ? static_cast<float>(khd_now - khd_snap.stamp_ms) / 1000.0f : -1.0f));
        out.push_back(kv("harvests", static_cast<float>(khd_snap.serial)));
        out.push_back(kv("pointN", static_cast<float>(khd_snap.point_n)));
        out.push_back(kv("spotN", static_cast<float>(khd_snap.spot_n)));

        {   // cb10 + the count lanes' int interpretations
            auto_array<game_value> ctl;
            for (int i = 0; i < 16; ++i) ctl.push_back(game_value(khd_snap.ctl[i]));
            out.push_back(kva("ctlRaw", std::move(ctl)));
            int32_t khd_pc_i = 0, khd_sc_i = 0;
            memcpy(&khd_pc_i, &khd_snap.ctl[0], 4);
            memcpy(&khd_sc_i, &khd_snap.ctl[4], 4);
            auto_array<game_value> ints;
            ints.push_back(game_value(static_cast<float>(khd_pc_i)));
            ints.push_back(game_value(static_cast<float>(khd_sc_i)));
            out.push_back(kva("ctlCountInts", std::move(ints)));
        }

        out.push_back(kv("distScale", khd_snap.ctl[8]));

        {
            auto_array<game_value> gd;
            gd.push_back(game_value(khd_snap.ctl[12]));
            gd.push_back(game_value(khd_snap.ctl[13]));
            gd.push_back(game_value(khd_snap.ctl[14]));
            out.push_back(kva("globalDiffuse", std::move(gd)));
        }

        {   // binding census: identities, offsets, window sizes, widths
            auto_array<game_value> b10;
            b10.push_back(hex64(reinterpret_cast<uint64_t>(khd_snap.slot10_buf)));
            b10.push_back(game_value(static_cast<float>(khd_snap.slot10_first)));
            b10.push_back(game_value(static_cast<float>(khd_snap.slot10_num)));
            b10.push_back(game_value(static_cast<float>(khd_snap.bw10)));
            out.push_back(kva("bind10", std::move(b10)));
            auto_array<game_value> b11;
            b11.push_back(hex64(reinterpret_cast<uint64_t>(khd_snap.slot11_buf)));
            b11.push_back(game_value(static_cast<float>(khd_snap.slot11_first)));
            b11.push_back(game_value(static_cast<float>(khd_snap.slot11_num)));
            b11.push_back(game_value(static_cast<float>(khd_snap.bw11)));
            out.push_back(kva("bind11", std::move(b11)));
        }

        out.push_back(kv("acquires", static_cast<float>(khd_snap.acquires)));
        out.push_back(kv("copies", static_cast<float>(khd_snap.copies)));
        out.push_back(kv("stillDrawing", static_cast<float>(khd_snap.still_drawing)));
        out.push_back(kv("noCtx1", static_cast<float>(khd_snap.no_ctx1)));
        out.push_back(kv("slotNulls", static_cast<float>(khd_snap.slot_nulls)));
        out.push_back(kv("issueSkips", static_cast<float>(khd_snap.issue_skips)));
        out.push_back(kv("boundsRejects", static_cast<float>(khd_snap.bounds_rejects)));
        out.push_back(kv("windowClamps", static_cast<float>(khd_snap.window_clamps)));
        out.push_back(kv("valRejects", static_cast<float>(khd_snap.val_rejects)));
        out.push_back(kv("countsFloat", static_cast<float>(khd_snap.counts_float)));
        out.push_back(kv("clampedCounts", static_cast<float>(khd_snap.clamped_counts)));
        out.push_back(kv("zeroLists", static_cast<float>(khd_snap.zero_lists)));
        out.push_back(kv("hashChanges", static_cast<float>(khd_snap.hash_changes)));
        out.push_back(kv("staleSkips", static_cast<float>(khd_snap.stale_skips)));
        out.push_back(kv("viewMissSkips", static_cast<float>(khd_snap.view_miss_skips)));
        out.push_back(kv("poolOffsetBinds", static_cast<float>(khd_snap.pool_offset_binds)));
        out.push_back(kv("sharedPoolBinds", static_cast<float>(khd_snap.shared_pool_binds)));

        {
            auto_array<game_value> rej;
            for (int i = 0; i < 4; ++i) rej.push_back(game_value(khd_snap.last_rej[i]));
            out.push_back(kva("lastReject", std::move(rej)));
        }

        {   // per-draw variability: [distinct last frame, max, null samples, samples]
            auto_array<game_value> pd;
            pd.push_back(game_value(static_cast<float>(khd_cd_last)));
            pd.push_back(game_value(static_cast<float>(khd_cd_max)));
            pd.push_back(game_value(static_cast<float>(khd_cd_null)));
            pd.push_back(game_value(static_cast<float>(khd_cd_samples)));
            out.push_back(kva("perDraw", std::move(pd)));
        }

        {   // capture-frame camera + view columns (the space-decode inputs)
            auto_array<game_value> cam;
            for (int i = 0; i < 3; ++i) cam.push_back(game_value(khd_snap.cam[i]));
            out.push_back(kva("camera", std::move(cam)));
            auto_array<game_value> vc;
            for (int i = 0; i < 12; ++i) vc.push_back(game_value(khd_snap.view_cols[i]));
            out.push_back(kva("viewCols", std::move(vc)));
            out.push_back(kv("viewValid", khd_snap.view_valid ? 1.0f : 0.0f));
        }

        {   // every active light record, 24 floats each, engine
            auto_array<game_value> lights;
            uint32_t khd_n = khd_snap.point_n + khd_snap.spot_n;
            if (khd_n > RenderIntegration::KH_DL_MAX_LIGHTS) khd_n = RenderIntegration::KH_DL_MAX_LIGHTS;

            for (uint32_t i = 0; i < khd_n; ++i) {
                auto_array<game_value> rec;
                for (int f = 0; f < 24; ++f) rec.push_back(game_value(khd_snap.lights[i * 24 + f]));
                lights.push_back(game_value(std::move(rec)));
            }

            out.push_back(kva("lights", std::move(lights)));
        }

        {   // acquisition ring, oldest-first
            auto_array<game_value> ring;

            for (uint32_t k = 0; k < RenderIntegration::KH_DL_RING; ++k) {
                const auto& r = khd_snap.ring[(khd_snap.ring_head + k) % RenderIntegration::KH_DL_RING];
                if (r.serial == 0) continue;
                auto_array<game_value> e;
                e.push_back(game_value(static_cast<float>(r.serial)));
                e.push_back(game_value(r.t_ms != 0 ? static_cast<float>(khd_now - r.t_ms) / 1000.0f : -1.0f));
                e.push_back(game_value(static_cast<float>(r.point_n)));
                e.push_back(game_value(static_cast<float>(r.spot_n)));
                e.push_back(game_value(r.dist_scale));
                e.push_back(game_value(static_cast<float>(r.hash32)));
                e.push_back(game_value(r.cam[0]));
                e.push_back(game_value(r.cam[1]));
                e.push_back(game_value(r.cam[2]));
                e.push_back(hex64(r.b10));
                e.push_back(game_value(static_cast<float>(r.f10)));
                e.push_back(game_value(static_cast<float>(r.n10)));
                e.push_back(hex64(r.b11));
                e.push_back(game_value(static_cast<float>(r.f11)));
                e.push_back(game_value(static_cast<float>(r.n11)));
                e.push_back(game_value(static_cast<float>(r.counts_as_float)));
                for (int f = 0; f < 8; ++f) e.push_back(game_value(r.l0[f]));
                ring.push_back(game_value(std::move(e)));
            }

            out.push_back(kva("ring", std::move(ring)));
        }

        {   // last harvest's per-window origin verdicts
            auto_array<game_value> wins;

            for (uint32_t w = 0; w < khd_snap.win_report_n && w < RenderIntegration::KH_DL_WIN_MAX; ++w) {
                const auto& r = khd_snap.win_report[w];
                auto_array<game_value> e;
                e.push_back(hex64(r.buf));
                e.push_back(game_value(static_cast<float>(r.first)));
                e.push_back(game_value(static_cast<float>(r.point_n)));
                e.push_back(game_value(static_cast<float>(r.spot_n)));
                e.push_back(game_value(static_cast<float>(r.origin_ok)));
                e.push_back(game_value(r.ox));
                e.push_back(game_value(r.oz));
                e.push_back(game_value(r.scale));
                e.push_back(game_value(r.gdiff[0]));
                e.push_back(game_value(r.gdiff[1]));
                e.push_back(game_value(r.gdiff[2]));
                e.push_back(game_value(static_cast<float>(r.derived_ok)));
                e.push_back(game_value(r.d_ox));
                e.push_back(game_value(r.d_oz));
                e.push_back(game_value(r.d_res));
                e.push_back(game_value(static_cast<float>(r.d_slot)));
                e.push_back(game_value(static_cast<float>(r.d_wslot)));
                e.push_back(game_value(static_cast<float>(r.pool_vote)));
                e.push_back(game_value(static_cast<float>(r.v_best)));
                e.push_back(game_value(static_cast<float>(r.v_runner)));
                e.push_back(game_value(static_cast<float>(r.v_cands)));
                e.push_back(game_value(r.v_res));
                wins.push_back(game_value(std::move(e)));
            }

            out.push_back(kva("windows", std::move(wins)));
        }

        {   // the merged absolute-world pool: [x, y, z, spot] per light
            auto_array<game_value> pool;

            for (uint32_t i = 0; i < khd_snap.pool_n; ++i) {
                const auto& p = khd_snap.pool[i];
                auto_array<game_value> e;
                e.push_back(game_value(p.rec[0]));
                e.push_back(game_value(p.rec[1]));
                e.push_back(game_value(p.rec[2]));
                e.push_back(game_value(static_cast<float>(p.spot)));
                e.push_back(game_value(static_cast<float>(p.aux)));
                e.push_back(game_value(static_cast<float>(p.derived)));
                pool.push_back(game_value(std::move(e)));
            }

            out.push_back(kva("pool", std::move(pool)));
        }

        {   // mode-3 harvest ring, oldest-first: [ageS, poolN, anchored
            // added, updated, expired, spotFlips, gdiffR, gdiffG, gdiffB,
            // scale]
            auto_array<game_value> m3;

            for (uint32_t k = 0; k < RenderIntegration::KH_DL_RING; ++k) {
                const auto& r = khd_snap.m3_ring[(khd_snap.m3_ring_head + k) % RenderIntegration::KH_DL_RING];
                if (r.t_ms == 0) continue;
                auto_array<game_value> e;
                e.push_back(game_value(static_cast<float>(khd_now - r.t_ms) / 1000.0f));
                e.push_back(game_value(static_cast<float>(r.pool_n)));
                e.push_back(game_value(static_cast<float>(r.anchored)));
                e.push_back(game_value(static_cast<float>(r.added)));
                e.push_back(game_value(static_cast<float>(r.updated)));
                e.push_back(game_value(static_cast<float>(r.expired)));
                e.push_back(game_value(static_cast<float>(r.spot_flips)));
                e.push_back(game_value(r.gdiff[0]));
                e.push_back(game_value(r.gdiff[1]));
                e.push_back(game_value(r.gdiff[2]));
                e.push_back(game_value(r.scale));
                m3.push_back(game_value(std::move(e)));
            }

            out.push_back(kva("m3Ring", std::move(m3)));
        }

        out.push_back(kv("fillCalls", static_cast<float>(khd_snap.fill_calls)));
        out.push_back(kv("fillLastN", static_cast<float>(khd_snap.fill_last_n)));
        out.push_back(kv("fillMinN", khd_snap.fill_min_n == 0xFFFFFFFFu
                                   ? -1.0f : static_cast<float>(khd_snap.fill_min_n)));
        out.push_back(kv("fillMaxN", static_cast<float>(khd_snap.fill_max_n)));
        out.push_back(kv("spotFlips", static_cast<float>(khd_snap.pool_spot_flips)));
        out.push_back(kv("winAux", static_cast<float>(khd_snap.win_aux)));
        out.push_back(kv("auxAdds", static_cast<float>(khd_snap.aux_adds)));
        out.push_back(kv("auxRefreshes", static_cast<float>(khd_snap.aux_refreshes)));
        out.push_back(kv("vsCopies", static_cast<float>(khd_snap.vs_copies)));
        out.push_back(kv("vsScans", static_cast<float>(khd_snap.vs_scans)));
        out.push_back(kv("vsHintHits", static_cast<float>(khd_snap.vs_hint_hits)));
        out.push_back(kv("vsOrthoPass", static_cast<float>(khd_snap.vs_ortho_pass)));
        out.push_back(kv("vsRotRejects", static_cast<float>(khd_snap.vs_rot_rejects)));
        out.push_back(kv("vsYRejects", static_cast<float>(khd_snap.vs_y_rejects)));
        out.push_back(kv("vsGridRejects", static_cast<float>(khd_snap.vs_grid_rejects)));
        out.push_back(kv("vsNoRef", static_cast<float>(khd_snap.vs_noref)));
        out.push_back(kv("vsValid", static_cast<float>(khd_snap.vs_valid)));
        out.push_back(kv("vsHintSlot", static_cast<float>(khd_snap.vs_hint_slot)));
        out.push_back(kv("vsHintWSlot", static_cast<float>(khd_snap.vs_hint_wslot)));
        out.push_back(kv("vsHintForm", static_cast<float>(khd_snap.vs_hint_form)));
        out.push_back(kv("vsHintWForm", static_cast<float>(khd_snap.vs_hint_wform)));
        out.push_back(kv("vsHintKind", static_cast<float>(khd_snap.vs_hint_kind)));
        out.push_back(kv("vsTripleValid", static_cast<float>(khd_snap.vs_triple_valid)));
        out.push_back(kv("vsZeroOrigins", static_cast<float>(khd_snap.vs_zero_origins)));
        out.push_back(kv("vsNonzeroOrigins", static_cast<float>(khd_snap.vs_nonzero_origins)));
        out.push_back(kv("originRangeRejects", static_cast<float>(khd_snap.origin_range_rejects)));
        out.push_back(kv("originPoolVotes", static_cast<float>(khd_snap.origin_pool_votes)));
        out.push_back(kv("originVoteAmbiguous", static_cast<float>(khd_snap.origin_vote_ambiguous)));
        out.push_back(kv("vsRangePrunes", static_cast<float>(khd_snap.vs_range_prunes)));

        {
            uint32_t khd_dn = 0;

            for (uint32_t i = 0; i < khd_snap.pool_n; ++i) {
                if (khd_snap.pool[i].derived) khd_dn++;
            }

            out.push_back(kv("poolDerivedN", static_cast<float>(khd_dn)));
        }

        out.push_back(kv("anchorTableN", static_cast<float>(khd_snap.anchor_n)));
        out.push_back(kv("anchorAdmits", static_cast<float>(khd_snap.anchor_admits)));
        out.push_back(kv("anchorResRejects", static_cast<float>(khd_snap.anchor_res_rejects)));

        {   // the anchor table: [x, y, z, ageS] - pollution is
            // visible at a glance (more anchors than real lights, or
            // positions off the known lamp set)
            auto_array<game_value> an;

            for (uint32_t a = 0; a < khd_snap.anchor_n && a < RenderIntegration::KH_DL_ANCHOR_N; ++a) {
                auto_array<game_value> e;
                e.push_back(game_value(khd_snap.anchor_pos[a][0]));
                e.push_back(game_value(khd_snap.anchor_pos[a][1]));
                e.push_back(game_value(khd_snap.anchor_pos[a][2]));
                e.push_back(game_value(khd_snap.anchor_stamp[a] != 0
                          ? static_cast<float>(khd_now - khd_snap.anchor_stamp[a]) / 1000.0f : -1.0f));
                an.push_back(game_value(std::move(e)));
            }

            out.push_back(kva("anchors", std::move(an)));
        }

        {   // derivation-candidate recon ring, oldest-first:
            // [ageS, bufLo, kind, slot, off, ox, oz, res, ydelta]
            auto_array<game_value> cr;

            for (uint32_t k = 0; k < 256u; ++k) {
                const auto& c = khd_snap.cand_ring[(khd_snap.cand_head + k) % 256u];
                if (c.t_ms == 0) continue;
                auto_array<game_value> e;
                e.push_back(game_value(static_cast<float>(khd_now - c.t_ms) / 1000.0f));
                e.push_back(game_value(static_cast<float>(c.buf_lo)));
                e.push_back(game_value(static_cast<float>(c.kind)));
                e.push_back(game_value(static_cast<float>(c.slot)));
                e.push_back(game_value(static_cast<float>(c.off)));
                e.push_back(game_value(c.ox));
                e.push_back(game_value(c.oz));
                e.push_back(game_value(c.res));
                e.push_back(game_value(c.ydelta));
                cr.push_back(game_value(std::move(e)));
            }

            out.push_back(kva("candRing", std::move(cr)));
        }
        out.push_back(kv("mainRefValid", static_cast<float>(khd_snap.main_ref_valid)));
        out.push_back(kv("mainRefScale", khd_snap.main_scale));
        out.push_back(kv("refHolds", static_cast<float>(khd_snap.ref_holds)));
        out.push_back(kv("refJumpAdopts", static_cast<float>(khd_snap.ref_jump_adopts)));
        out.push_back(kv("refPendAgeS", khd_snap.ref_pend_ms != 0
            ? static_cast<float>(RenderIntegration::steady_now_ms() - khd_snap.ref_pend_ms) / 1000.0f
            : -1.0f));

        {
            auto_array<game_value> mg;
            mg.push_back(game_value(khd_snap.main_gdiff[0]));
            mg.push_back(game_value(khd_snap.main_gdiff[1]));
            mg.push_back(game_value(khd_snap.main_gdiff[2]));
            out.push_back(kva("mainRefGdiff", std::move(mg)));
        }
        out.push_back(kv("vsPairTries", static_cast<float>(khd_snap.vs_pair_tries)));
        out.push_back(kv("vsPairValid", static_cast<float>(khd_snap.vs_pair_valid)));
        out.push_back(kv("vsPairYRejects", static_cast<float>(khd_snap.vs_pair_y_rejects)));
        out.push_back(kv("vsPairGridRejects", static_cast<float>(khd_snap.vs_pair_grid_rejects)));
        out.push_back(kv("vsLastYDelta", khd_snap.vs_last_ydelta));
        out.push_back(kv("poolN", static_cast<float>(khd_snap.pool_n)));
        out.push_back(kv("winCaptured", static_cast<float>(khd_snap.win_captured)));
        out.push_back(kv("winTableFull", static_cast<float>(khd_snap.win_table_full)));
        out.push_back(kv("listsAnchored", static_cast<float>(khd_snap.lists_anchored)));
        out.push_back(kv("listsUnanchored", static_cast<float>(khd_snap.lists_unanchored)));
        out.push_back(kv("poolAdded", static_cast<float>(khd_snap.pool_added)));
        out.push_back(kv("poolUpdated", static_cast<float>(khd_snap.pool_updated)));
        out.push_back(kv("poolExpired", static_cast<float>(khd_snap.pool_expired)));
        // EXPIRY CENSUS - the lane the transient tail has never had.
        // dlExpLifeMean is how long the entry was actually alive, so
        // tail/life prices the fault directly.
        out.push_back(kv("dlExpN", static_cast<float>(RenderIntegration::g_dl_exp_n)));
        out.push_back(kv("dlExpFloorHits", static_cast<float>(RenderIntegration::g_dl_exp_floor_hits)));
        out.push_back(kv("dlExpDenseHits", static_cast<float>(RenderIntegration::g_dl_exp_dense_hits)));
        out.push_back(kv("dlExpTailMinMs", static_cast<float>(
            RenderIntegration::g_dl_exp_tail_min == 0xFFFFFFFFu
                ? 0u : RenderIntegration::g_dl_exp_tail_min)));
        out.push_back(kv("dlExpTailMaxMs", static_cast<float>(RenderIntegration::g_dl_exp_tail_max)));
        out.push_back(kv("dlExpTailMeanMs", RenderIntegration::g_dl_exp_n
            ? static_cast<float>(RenderIntegration::g_dl_exp_tail_sum) /
              static_cast<float>(RenderIntegration::g_dl_exp_n) : -1.0f));
        out.push_back(kv("dlExpLifeMeanMs", RenderIntegration::g_dl_exp_n
            ? static_cast<float>(RenderIntegration::g_dl_exp_life_sum) /
              static_cast<float>(RenderIntegration::g_dl_exp_n) : -1.0f));

        {   // per-expiry ring: [lifeMs, tailMs, ttlMs, gapMaxMs, sightings, dense, floored]
            auto_array<game_value> khx_rows;

            for (uint32_t khx_i = 0; khx_i < RenderIntegration::KH_DL_EXP_RING; ++khx_i) {
                const uint32_t khx_k =
                    (RenderIntegration::g_dl_exp_head + khx_i) % RenderIntegration::KH_DL_EXP_RING;
                const RenderIntegration::DlExpiry& khx_e = RenderIntegration::g_dl_exp[khx_k];
                if (khx_e.ttl_ms == 0) continue;   // unfilled slot
                auto_array<game_value> khx_r;
                khx_r.push_back(game_value(static_cast<float>(khx_e.life_ms)));
                khx_r.push_back(game_value(static_cast<float>(khx_e.tail_ms)));
                khx_r.push_back(game_value(static_cast<float>(khx_e.ttl_ms)));
                khx_r.push_back(game_value(static_cast<float>(khx_e.gap_max_ms)));
                khx_r.push_back(game_value(static_cast<float>(khx_e.sightings)));
                khx_r.push_back(game_value(static_cast<float>(khx_e.dense)));
                khx_r.push_back(game_value(static_cast<float>(khx_e.floored)));
                khx_rows.push_back(game_value(std::move(khx_r)));
            }

            out.push_back(kva("expiryRing", std::move(khx_rows)));
        }

        return game_value(std::move(out));
    } catch (...) {
        report_error("dumpDynamicLights: unknown exception");
        return game_value(auto_array<game_value>());
    }
}

// Localized passes always sample the depth buffer (read-only DSV phase rules
// apply).
static game_value add_local_postfx_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 4) return game_value("usage: addLocalPostFX [[x,y,zASL], radius, falloff, effect, params?, [r,g,b,a]?, shape?, blend?, duration?]");
        RenderIntegration::RenderObject obj;
        obj.fullscreen = true;
        obj.localized = true;
        obj.mode = RenderIntegration::DepthMode::Off;
        if (arr[0].type_enum() != game_data_type::ARRAY) return game_value("position must be [x, y, zASL]");
        auto& pos = arr[0].to_array();
        if (pos.size() < 3 ||
            pos[0].type_enum() != game_data_type::SCALAR ||
            pos[1].type_enum() != game_data_type::SCALAR ||
            pos[2].type_enum() != game_data_type::SCALAR) return game_value("position must be [x, y, zASL]");
        obj.pos[0] = static_cast<float>(pos[0]);
        obj.pos[1] = static_cast<float>(pos[1]);
        obj.pos[2] = static_cast<float>(pos[2]);

        if (!RenderIntegration::read_vec3_or_uniform(arr[1], obj.local_radius)) {
            return game_value("radius must be a number or [x, y, z]");
        }

        if (arr[2].type_enum() != game_data_type::SCALAR) return game_value("falloff must be a number");
        obj.local_falloff = static_cast<float>(arr[2]);
        std::string khfx_path, khfx_err;
        const int e = RenderIntegration::kh_effect_from_gv(arr[3], khfx_path, khfx_err);
        if (e <= 0) return game_value(khfx_err.empty() ? std::string("unknown or non-fullscreen effect") : khfx_err);
        obj.effect = e;
        obj.fx_shader = khfx_path;
        const auto_array<game_value>* fx_params = nullptr;

        if (arr.size() > 4 && !arr[4].is_nil()) {
            if (arr[4].type_enum() != game_data_type::ARRAY) return game_value("params must be an array of numbers");
            fx_params = &arr[4].to_array();
        }

        if (!RenderIntegration::set_effect_params(obj, fx_params)) return game_value("params entries must be numbers");

        if (arr.size() > 5 && !arr[5].is_nil()) {
            if (arr[5].type_enum() != game_data_type::ARRAY) return game_value("color must be [r, g, b, a] numbers");
            auto& col = arr[5].to_array();
            if ((col.size() > 0 && col[0].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 1 && col[1].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 2 && col[2].type_enum() != game_data_type::SCALAR) ||
                (col.size() > 3 && col[3].type_enum() != game_data_type::SCALAR)) return game_value("color must be [r, g, b, a] numbers");
            for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
            RenderIntegration::kh_sanitize_color(obj.color);
        }

        if (arr.size() > 6) {
            const int sh = RenderIntegration::shape_id_from_gv(arr[6]);
            if (sh < 0) return game_value("unknown shape (sphere | cube)");
            obj.local_shape = sh;
        }

        if (arr.size() > 7 &&
            !(arr[7].type_enum() == game_data_type::STRING && static_cast<std::string>(arr[7]).empty())) {
            // empty string = slot skipped (the positional-placeholder
            // convention, matching the effect slot): blend stays default
            const int bm = RenderIntegration::blend_id_from_gv(arr[7]);
            if (bm < 0) return game_value("unknown blend mode");
            obj.blend_mode = bm;
        }

        if (arr.size() > 8) {
            if (!RenderIntegration::parse_duration_gv(arr[8], obj)) {
                return game_value("duration must be seconds or [fadeIn, hold, fadeOut]");
            }
        }

        return game_value(RenderIntegration::add_render_object(obj));
    } catch (const std::exception& e) {
        report_error(std::string("addLocalPostFX: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("addLocalPostFX: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// Driven automatically by the internal overlay control created by
// ensure_ui_driver; also callable from a Draw EH on a custom display. Cheap
// no-op when no UI-affecting passes exist. Returns BOOL: true if passes were
// queued this call.
static game_value flush_ui_render_sqf() {

    try {
        RenderIntegration::ensure_ui_driver();   // explicit UI-render demand is an enabling command
        return game_value(RenderIntegration::flush_ui_frame());
    } catch (const std::exception& e) {
        report_error(std::string("flushUIRender: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("flushUIRender: unknown exception");
        return game_value(false);
    }
}

static void initialize_sqf_integration() {
    _sqf_execute_lua_any_string = intercept::client::host::register_sqf_command(
        "luaExecute",
        "Execute Lua code or function",
        userFunctionWrapper<execute_lua_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::STRING
    );

    _sqf_execute_lua_any_array = intercept::client::host::register_sqf_command(
        "luaExecute",
        "Execute Lua code or function",
        userFunctionWrapper<execute_lua_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_execute_lua_any_code = intercept::client::host::register_sqf_command(
        "luaExecute",
        "Execute Lua code or function",
        userFunctionWrapper<execute_lua_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::CODE
    );

    _sqf_compile_lua_string_string = intercept::client::host::register_sqf_command(
        "luaCompile",
        "Compile Lua code and register it as a named function",
        userFunctionWrapper<compile_lua_sqf>,
        game_data_type::NOTHING,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_compile_lua_string_code = intercept::client::host::register_sqf_command(
        "luaCompile",
        "Compile Lua code and register it as a named function",
        userFunctionWrapper<compile_lua_sqf>,
        game_data_type::NOTHING,
        game_data_type::STRING,
        game_data_type::CODE
    );

    _sqf_crypto_hash_string_string = intercept::client::host::register_sqf_command(
        "cryptoHash",
        "Compute cryptographic hash of input",
        userFunctionWrapper<crypto_hash_sqf>,
        game_data_type::STRING,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_generate_random_string_array_scalar = intercept::client::host::register_sqf_command(
        "generateRandomString", 
        "Generate random string",
        userFunctionWrapper<generate_random_string_sqf>,
        game_data_type::STRING,
        game_data_type::ARRAY,
        game_data_type::SCALAR
    );

    _sqf_generate_uid = intercept::client::host::register_sqf_command(
        "generateUid",
        "Generate a unique identifier",
        userFunctionWrapper<generate_uid_sqf>,
        game_data_type::STRING
    );

    _sqf_get_epoch = intercept::client::host::register_sqf_command(
        "getEpoch",
        "Get current epoch time",
        userFunctionWrapper<get_epoch_sqf>,
        game_data_type::STRING
    );

    _sqf_get_epoch_delta = intercept::client::host::register_sqf_command(
        "getEpochDelta",
        "Get delta time in seconds from a past epoch to now",
        userFunctionWrapper<get_epoch_delta_sqf>,
        game_data_type::SCALAR,
        game_data_type::STRING
    );

    _sqf_write_khdata_string_array = intercept::client::host::register_sqf_command(
        "writeKhData",
        "Write variable to KHData file",
        userFunctionWrapper<write_khdata_sqf>,
        game_data_type::NOTHING,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_read_khdata_string = intercept::client::host::register_sqf_command(
        "readKhData",
        "Read variable from KHData file",
        userFunctionWrapper<read_khdata_unary_sqf>,
        game_data_type::ARRAY,
        game_data_type::STRING
    );

    _sqf_read_khdata_string_string = intercept::client::host::register_sqf_command(
        "readKhData",
        "Read variable from KHData file",
        userFunctionWrapper<read_khdata_sqf>,
        game_data_type::ANY,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_read_khdata_string_array = intercept::client::host::register_sqf_command(
        "readKhData",
        "Read variable from KHData file",
        userFunctionWrapper<read_khdata_sqf>,
        game_data_type::ANY,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_flush_khdata = intercept::client::host::register_sqf_command(
        "flushKhData",
        "Flush all dirty KHData files to disk",
        userFunctionWrapper<flush_khdata_sqf>,
        game_data_type::NOTHING
    );

    _sqf_delete_khdata_file_string = intercept::client::host::register_sqf_command(
        "deleteKhDataFile",
        "Delete KHData file",
        userFunctionWrapper<delete_khdata_file_sqf>,
        game_data_type::NOTHING,
        game_data_type::STRING
    );

    _sqf_get_terrain_matrix = intercept::client::host::register_sqf_command(
        "getTerrainMatrix",
        "Get the pre-calculated terrain height matrix",
        userFunctionWrapper<get_terrain_matrix_sqf>,
        game_data_type::ARRAY
    );

    _sqf_trigger_lua_event_any_string = intercept::client::host::register_sqf_command(
        "luaTriggerEvent",
        "Trigger Lua event handlers",
        userFunctionWrapper<trigger_lua_event_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::STRING
    );

    _sqf_trigger_lua_event_any_array = intercept::client::host::register_sqf_command(
        "luaTriggerEvent",
        "Trigger Lua event handlers",
        userFunctionWrapper<trigger_lua_event_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_emit_lua_variable_string = intercept::client::host::register_sqf_command(
        "luaEmitVariable",
        "Emit Lua variable",
        userFunctionWrapper<emit_lua_variable_sqf>,
        game_data_type::ANY,
        game_data_type::STRING
    );

    _sqf_emit_lua_variable_array = intercept::client::host::register_sqf_command(
        "luaEmitVariable",
        "Emit Lua variable",
        userFunctionWrapper<emit_lua_variable_sqf>,
        game_data_type::NOTHING,
        game_data_type::ARRAY
    );

    _sqf_lua_set_variable_array = intercept::client::host::register_sqf_command(
        "luaSetVariable",
        "Set Lua variable",
        userFunctionWrapper<lua_set_variable_sqf>,
        game_data_type::NOTHING,
        game_data_type::ARRAY
    );

    _sqf_lua_get_variable_string = intercept::client::host::register_sqf_command(
        "luaGetVariable",
        "Get Lua variable",
        userFunctionWrapper<lua_get_variable_sqf>,
        game_data_type::ANY,
        game_data_type::STRING
    );

    _sqf_lua_get_variable_array = intercept::client::host::register_sqf_command(
        "luaGetVariable",
        "Get Lua variable",
        userFunctionWrapper<lua_get_variable_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_execute_any_code = intercept::client::host::register_sqf_command(
        "execute",
        "Execute SQF",
        userFunctionWrapper<execute_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::CODE
    );

    _sqf_execute_any_string = intercept::client::host::register_sqf_command(
        "execute",
        "Execute SQF",
        userFunctionWrapper<execute_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::STRING
    );

    _sqf_execute_any_array = intercept::client::host::register_sqf_command(
        "execute",
        "Execute SQF",
        userFunctionWrapper<execute_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_remove_handler_array = intercept::client::host::register_sqf_command(
        "removeHandler",
        "Remove an execution handler",
        userFunctionWrapper<remove_handler_sqf>,
        game_data_type::NOTHING,
        game_data_type::ARRAY
    );

    _sqf_execute_lua_string = intercept::client::host::register_sqf_command(
        "luaExecute",
        "Execute Lua code or function",
        userFunctionWrapper<execute_lua_sqf_unary>,
        game_data_type::ANY,
        game_data_type::STRING
    );

    _sqf_execute_lua_array = intercept::client::host::register_sqf_command(
        "luaExecute",
        "Execute Lua code or function",
        userFunctionWrapper<execute_lua_sqf_unary>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_execute_lua_code = intercept::client::host::register_sqf_command(
        "luaExecute",
        "Execute Lua code or function",
        userFunctionWrapper<execute_lua_sqf_unary>,
        game_data_type::ANY,
        game_data_type::CODE
    );

    _sqf_generate_random_string_scalar = intercept::client::host::register_sqf_command(
        "generateRandomString",
        "Generate random string",
        userFunctionWrapper<generate_random_string_sqf_unary>,
        game_data_type::STRING,
        game_data_type::SCALAR
    );

    _sqf_trigger_lua_event_string = intercept::client::host::register_sqf_command(
        "luaTriggerEvent",
        "Trigger Lua event handlers",
        userFunctionWrapper<trigger_lua_event_sqf_unary>,
        game_data_type::ANY,
        game_data_type::STRING
    );

    _sqf_trigger_lua_event_array = intercept::client::host::register_sqf_command(
        "luaTriggerEvent",
        "Trigger Lua event handlers",
        userFunctionWrapper<trigger_lua_event_sqf_unary>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_execute_code = intercept::client::host::register_sqf_command(
        "execute",
        "Execute SQF",
        userFunctionWrapper<execute_sqf_unary>,
        game_data_type::ANY,
        game_data_type::CODE
    );

    _sqf_execute_string = intercept::client::host::register_sqf_command(
        "execute",
        "Execute SQF",
        userFunctionWrapper<execute_sqf_unary>,
        game_data_type::ANY,
        game_data_type::STRING
    );

    _sqf_execute_array = intercept::client::host::register_sqf_command(
        "execute",
        "Execute SQF",
        userFunctionWrapper<execute_sqf_unary>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_set_return_value = intercept::client::host::register_sqf_command(
        "setReturnValue",
        "Store a return value in fast memory",
        userFunctionWrapper<set_return_value_sqf>,
        game_data_type::NOTHING,
        game_data_type::ANY
    );

    _sqf_get_return_value = intercept::client::host::register_sqf_command(
        "getReturnValue",
        "Retrieve the stored return value",
        userFunctionWrapper<get_return_value_sqf>,
        game_data_type::ANY
    );

    _sqf_set_call_arguments = intercept::client::host::register_sqf_command(
        "setCallArguments",
        "Store call arguments in fast memory",
        userFunctionWrapper<set_call_arguments_sqf>,
        game_data_type::NOTHING,
        game_data_type::ANY
    );

    _sqf_get_call_arguments = intercept::client::host::register_sqf_command(
        "getCallArguments",
        "Retrieve the stored call arguments",
        userFunctionWrapper<get_call_arguments_sqf>,
        game_data_type::ANY
    );

    _sqf_get_rotation_euler_object = intercept::client::host::register_sqf_command(
        "getRotationEuler",
        "Get object rotation as Euler angles [pitch, roll, yaw] in degrees",
        userFunctionWrapper<get_rotation_euler_unary>,
        game_data_type::ARRAY,
        game_data_type::OBJECT
    );

    _sqf_get_rotation_euler_object_object = intercept::client::host::register_sqf_command(
        "getRotationEuler",
        "Get object rotation as Euler angles [pitch, roll, yaw] in degrees",
        userFunctionWrapper<get_rotation_euler_sqf>,
        game_data_type::ARRAY,
        game_data_type::OBJECT,
        game_data_type::OBJECT
    );

    _sqf_set_rotation_euler = intercept::client::host::register_sqf_command(
        "setRotationEuler",
        "Set object rotation from Euler angles [pitch, roll, yaw] in degrees",
        userFunctionWrapper<set_rotation_euler_sqf>,
        game_data_type::NOTHING,
        game_data_type::OBJECT,
        game_data_type::ARRAY
    );

    _sqf_vector_to_euler = intercept::client::host::register_sqf_command(
        "vectorToEuler",
        "Convert [vectorDir, vectorUp] to Euler angles [pitch, roll, yaw] in degrees",
        userFunctionWrapper<vector_to_euler_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_euler_to_vector = intercept::client::host::register_sqf_command(
        "eulerToVector",
        "Convert Euler angles [pitch, roll, yaw] in degrees to [vectorDir, vectorUp]",
        userFunctionWrapper<euler_to_vector_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_euler_to_quaternion = intercept::client::host::register_sqf_command(
        "eulerToQuaternion",
        "Convert Euler angles [pitch, roll, yaw] in degrees to quaternion [w, x, y, z]",
        userFunctionWrapper<euler_to_quaternion_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_quaternion_to_euler = intercept::client::host::register_sqf_command(
        "quaternionToEuler",
        "Convert quaternion [w, x, y, z] to Euler angles [pitch, roll, yaw] in degrees",
        userFunctionWrapper<quaternion_to_euler_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_vector_to_quaternion = intercept::client::host::register_sqf_command(
        "vectorToQuaternion",
        "Convert [vectorDir, vectorUp] to quaternion [w, x, y, z]",
        userFunctionWrapper<vector_to_quaternion_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_quaternion_to_vector = intercept::client::host::register_sqf_command(
        "quaternionToVector",
        "Convert quaternion [w, x, y, z] to [vectorDir, vectorUp]",
        userFunctionWrapper<quaternion_to_vector_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_quaternion_slerp = intercept::client::host::register_sqf_command(
        "quaternionSlerp",
        "Spherical linear interpolation between two quaternions",
        userFunctionWrapper<quaternion_slerp_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_quaternion_multiply = intercept::client::host::register_sqf_command(
        "quaternionMultiply",
        "Multiply two quaternions",
        userFunctionWrapper<quaternion_multiply_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_get_rotation_quaternion_object = intercept::client::host::register_sqf_command(
        "getRotationQuaternion",
        "Get object rotation as quaternion [w, x, y, z]",
        userFunctionWrapper<get_quaternion_rotation_unary>,
        game_data_type::ARRAY,
        game_data_type::OBJECT
    );

    _sqf_get_rotation_quaternion_object_object = intercept::client::host::register_sqf_command(
        "getRotationQuaternion",
        "Get object rotation as quaternion [w, x, y, z]",
        userFunctionWrapper<get_quaternion_rotation_sqf>,
        game_data_type::ARRAY,
        game_data_type::OBJECT,
        game_data_type::OBJECT
    );

    _sqf_set_rotation_quaternion = intercept::client::host::register_sqf_command(
        "setRotationQuaternion",
        "Set object rotation from quaternion [w, x, y, z]",
        userFunctionWrapper<set_quaternion_rotation_sqf>,
        game_data_type::NOTHING,
        game_data_type::OBJECT,
        game_data_type::ARRAY
    );

    _sqf_axis_angle_to_quaternion = intercept::client::host::register_sqf_command(
        "axisAngleToQuaternion",
        "Convert [[axisX, axisY, axisZ], angle] to quaternion [w, x, y, z]",
        userFunctionWrapper<axis_angle_to_quaternion_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_quaternion_to_axis_angle = intercept::client::host::register_sqf_command(
        "quaternionToAxisAngle",
        "Convert quaternion [w, x, y, z] to [[axisX, axisY, axisZ], angle]",
        userFunctionWrapper<quaternion_to_axis_angle_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_initialize_ai = intercept::client::host::register_sqf_command(
        "initializeAi",
        "Initialize an AI instance with specified name",
        userFunctionWrapper<initialize_ai_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_stop_ai = intercept::client::host::register_sqf_command(
        "stopAi",
        "Stop a specific AI instance",
        userFunctionWrapper<stop_ai_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_stop_all_ai = intercept::client::host::register_sqf_command(
        "stopAllAi",
        "Stop all AI instances",
        userFunctionWrapper<stop_all_ai_sqf>,
        game_data_type::BOOL
    );

    _sqf_is_ai_active = intercept::client::host::register_sqf_command(
        "isAiActive",
        "Check if a specific AI is currently active",
        userFunctionWrapper<is_ai_active_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_is_ai_generating = intercept::client::host::register_sqf_command(
        "isAiGenerating",
        "Check if a specific AI is currently generating a response",
        userFunctionWrapper<is_ai_generating_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_get_active_ai = intercept::client::host::register_sqf_command(
        "getActiveAi",
        "Get array of active AI names",
        userFunctionWrapper<get_active_ai_sqf>,
        game_data_type::ARRAY
    );

    _sqf_set_ai_model_string = intercept::client::host::register_sqf_command(
        "setAiModel",
        "Set the global AI model",
        userFunctionWrapper<set_ai_model_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_set_ai_model_string_string = intercept::client::host::register_sqf_command(
        "setAiModel",
        "Set the specific AI instance model",
        userFunctionWrapper<set_ai_instance_model_path_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_update_ai_system_prompt = intercept::client::host::register_sqf_command(
        "updateAiSystemPrompt",
        "Update the system prompt for an AI",
        userFunctionWrapper<update_ai_system_prompt_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_update_ai_master_prompt = intercept::client::host::register_sqf_command(
        "updateAiMasterPrompt",
        "Update the master prompt for an AI",
        userFunctionWrapper<update_ai_master_prompt_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_update_ai_user_prompt = intercept::client::host::register_sqf_command(
        "updateAiUserPrompt",
        "Update the user prompt for an AI",
        userFunctionWrapper<update_ai_user_prompt_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_set_ai_parameters = intercept::client::host::register_sqf_command(
        "setAiParameters",
        "Set parameters for an AI",
        userFunctionWrapper<set_ai_parameters_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_trigger_ai_inference = intercept::client::host::register_sqf_command(
        "triggerAiInference",
        "Trigger inference for an AI",
        userFunctionWrapper<trigger_ai_inference_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_set_ai_markers = intercept::client::host::register_sqf_command(
        "setAiMarkers",
        "Set custom prompt markers for an AI instance",
        userFunctionWrapper<set_ai_markers_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_abort_ai_generation = intercept::client::host::register_sqf_command(
        "abortAiGeneration",
        "Abort current AI response generation for specified AI",
        userFunctionWrapper<abort_ai_generation_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_log_ai_generation = intercept::client::host::register_sqf_command(
        "logAiGeneration",
        "Enable or disable generation statistics logging for specified AI",
        userFunctionWrapper<log_ai_generation_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::BOOL
    );

    _sqf_reset_ai_context = intercept::client::host::register_sqf_command(
        "resetAiContext",
        "Reset conversation context for specified AI",
        userFunctionWrapper<reset_ai_context_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_tts_load_model_string = intercept::client::host::register_sqf_command(
        "ttsLoadModel",
        "Load a TTS model",
        userFunctionWrapper<tts_load_model_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_tts_load_model_string_array = intercept::client::host::register_sqf_command(
        "ttsLoadModel",
        "Load a TTS model with configuration",
        userFunctionWrapper<tts_load_model_with_config_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_tts_is_initialized = intercept::client::host::register_sqf_command(
        "ttsIsInitialized",
        "Check if TTS system is initialized",
        userFunctionWrapper<tts_is_initialized_sqf>,
        game_data_type::BOOL
    );

    _sqf_tts_speak = intercept::client::host::register_sqf_command(
        "ttsSpeak",
        "Generate and play speech",
        userFunctionWrapper<tts_speak_sqf>,
        game_data_type::BOOL,
        game_data_type::ARRAY
    );

    _sqf_tts_update_speaker = intercept::client::host::register_sqf_command(
        "ttsUpdateSpeaker",
        "Update speaker position/volume",
        userFunctionWrapper<tts_update_speaker_sqf>,
        game_data_type::BOOL,
        game_data_type::ARRAY
    );

    _sqf_tts_stop_speaker = intercept::client::host::register_sqf_command(
        "ttsStopSpeaker",
        "Stop specific speaker",
        userFunctionWrapper<tts_stop_speaker_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_tts_is_playing = intercept::client::host::register_sqf_command(
        "ttsIsPlaying",
        "Check if speaker is playing",
        userFunctionWrapper<tts_is_playing_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_tts_stop_all = intercept::client::host::register_sqf_command(
        "ttsStopAll",
        "Stop all speakers",
        userFunctionWrapper<tts_stop_all_sqf>,
        game_data_type::BOOL
    );

    _sqf_stt_load_model_string = intercept::client::host::register_sqf_command(
        "sttLoadModel",
        "Load an STT model by name",
        userFunctionWrapper<stt_load_model_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_stt_load_model_string_array = intercept::client::host::register_sqf_command(
        "sttLoadModel",
        "Load STT model with config",
        userFunctionWrapper<stt_load_model_with_config_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_stt_is_initialized = intercept::client::host::register_sqf_command(
        "sttIsInitialized",
        "Check if STT is initialized",
        userFunctionWrapper<stt_is_initialized_sqf>,
        game_data_type::BOOL
    );

    _sqf_stt_is_capturing = intercept::client::host::register_sqf_command(
        "sttIsCapturing",
        "Check if currently capturing audio",
        userFunctionWrapper<stt_is_capturing_sqf>,
        game_data_type::BOOL
    );

    _sqf_stt_start_capture = client::host::register_sqf_command(
        "sttStartCapture", 
        "Manually start audio capture",
        userFunctionWrapper<stt_start_capture_sqf>,
        game_data_type::BOOL
    );

    _sqf_stt_stop_capture = client::host::register_sqf_command(
        "sttStopCapture", 
        "Manually stop audio capture and process",
        userFunctionWrapper<stt_stop_capture_sqf>,
        game_data_type::BOOL
    );

    _sqf_html_create = intercept::client::host::register_sqf_command(
        "htmlCreate",
        "Create an HTML UI overlay from HTML content string",
        userFunctionWrapper<ui_create_html_sqf>,
        game_data_type::STRING,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_html_open = intercept::client::host::register_sqf_command(
        "htmlOpen",
        "Open an HTML file as UI overlay",
        userFunctionWrapper<ui_open_html_sqf>,
        game_data_type::STRING,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_html_close = intercept::client::host::register_sqf_command(
        "htmlClose",
        "Close an HTML UI document by ID",
        userFunctionWrapper<ui_close_html_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_html_set_visible = intercept::client::host::register_sqf_command(
        "htmlSetVisible",
        "Set HTML UI visibility",
        userFunctionWrapper<ui_set_html_visible_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::BOOL
    );

    _sqf_html_get_open = intercept::client::host::register_sqf_command(
        "htmlGetOpenDocuments",
        "Get array of open HTML UI document IDs",
        userFunctionWrapper<ui_get_open_documents_sqf>,
        game_data_type::ARRAY
    );

    _sqf_html_is_initialized = intercept::client::host::register_sqf_command(
        "htmlIsInitialized",
        "Check if HTML UI framework is initialized",
        userFunctionWrapper<ui_is_initialized_sqf>,
        game_data_type::BOOL
    );

    _sqf_html_execute_js = intercept::client::host::register_sqf_command(
        "htmlExecuteJS",
        "Execute JavaScript in HTML document",
        userFunctionWrapper<ui_execute_js_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_html_set_js_variable = intercept::client::host::register_sqf_command(
        "htmlSetJsVariable",
        "Set a global JavaScript variable in HTML document",
        userFunctionWrapper<ui_set_js_variable_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_html_get_js_variable = intercept::client::host::register_sqf_command(
        "htmlGetJsVariable",
        "Get a global JavaScript variable from HTML document",
        userFunctionWrapper<ui_get_js_variable_sqf>,
        game_data_type::ANY,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_html_set_position = intercept::client::host::register_sqf_command(
        "htmlSetPosition",
        "Set HTML UI position",
        userFunctionWrapper<ui_set_position_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_html_set_opacity = intercept::client::host::register_sqf_command(
        "htmlSetOpacity",
        "Set HTML UI opacity",
        userFunctionWrapper<ui_set_opacity_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::SCALAR
    );

    _sqf_html_set_size = intercept::client::host::register_sqf_command(
        "htmlSetSize",
        "Resize HTML UI",
        userFunctionWrapper<ui_set_size_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_html_set_z_order = intercept::client::host::register_sqf_command(
        "htmlSetZOrder",
        "Set HTML UI z-order",
        userFunctionWrapper<ui_set_z_order_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING,
        game_data_type::SCALAR
    );

    _sqf_html_bring_to_front = intercept::client::host::register_sqf_command(
        "htmlBringToFront",
        "Bring HTML UI to front of all others",
        userFunctionWrapper<ui_bring_to_front_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_html_send_to_back = intercept::client::host::register_sqf_command(
        "htmlSendToBack",
        "Send HTML UI behind all others",
        userFunctionWrapper<ui_send_to_back_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_html_reload = intercept::client::host::register_sqf_command(
        "htmlReload",
        "Reload HTML file from disk",
        userFunctionWrapper<ui_reload_html_sqf>,
        game_data_type::STRING,
        game_data_type::STRING
    );

    _sqf_kh_network_message_send_any_array = intercept::client::host::register_sqf_command(
        "khNetworkMessageSend",
        "Send a network message",
        userFunctionWrapper<network_message_send_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_kh_network_message_send_array = intercept::client::host::register_sqf_command(
        "khNetworkMessageSend",
        "Send a network message",
        userFunctionWrapper<network_message_send_unary_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_kh_network_remove_jip = intercept::client::host::register_sqf_command(
        "khNetworkMessageRemoveJip",
        "Remove a JIP message by its key",
        userFunctionWrapper<network_remove_jip_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_kh_network_message_receive_string_array = intercept::client::host::register_sqf_command(
        "khNetworkMessageReceive",
        "Register a handler for network messages",
        userFunctionWrapper<network_message_receive_sqf>,
        game_data_type::ARRAY,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_kh_network_message_receive_string_code = intercept::client::host::register_sqf_command(
        "khNetworkMessageReceive",
        "Register a handler for network messages",
        userFunctionWrapper<network_message_receive_sqf>,
        game_data_type::ARRAY,
        game_data_type::STRING,
        game_data_type::CODE
    );

    _sqf_kh_network_remove_handler = intercept::client::host::register_sqf_command(
        "khNetworkRemoveHandler",
        "Remove a network message handler by ID",
        userFunctionWrapper<network_remove_handler_sqf>,
        game_data_type::BOOL,
        game_data_type::SCALAR
    );

    _sqf_kh_network_is_initialized = intercept::client::host::register_sqf_command(
        "khNetworkIsInitialized",
        "Check if the network framework is initialized",
        userFunctionWrapper<network_is_initialized_sqf>,
        game_data_type::BOOL
    );

    _sqf_kh_network_initialize = intercept::client::host::register_sqf_command(
        "khNetworkInitialize",
        "Initialize network framework",
        userFunctionWrapper<network_initialize_sqf>,
        game_data_type::BOOL
    );

    _sqf_kh_network_shutdown = intercept::client::host::register_sqf_command(
        "khNetworkShutdown",
        "Shutdown the network framework",
        userFunctionWrapper<network_shutdown_sqf>,
        game_data_type::BOOL
    );

    _sqf_enable_network_logging = intercept::client::host::register_sqf_command(
        "enableKhNetworkLogging",
        "Network message logging",
        userFunctionWrapper<enable_network_logging_sqf>,
        game_data_type::BOOL,
        game_data_type::BOOL
    );

    _sqf_kh_set_variable_namespace_array = intercept::client::host::register_sqf_command(
        "khSetVariable",
        "Set a variable on a namespace and synchronize across network. Usage: namespace khSetVariable [name, value, target, jip]",
        userFunctionWrapper<kh_set_variable_namespace>,
        game_data_type::ANY,
        game_data_type::NAMESPACE,
        game_data_type::ARRAY
    );

    _sqf_kh_set_variable_object_array = intercept::client::host::register_sqf_command(
        "khSetVariable",
        "Set a variable on an object and synchronize across network. Usage: object khSetVariable [name, value, target, jip]",
        userFunctionWrapper<kh_set_variable_object>,
        game_data_type::ANY,
        game_data_type::OBJECT,
        game_data_type::ARRAY
    );

    _sqf_kh_set_variable_group_array = intercept::client::host::register_sqf_command(
        "khSetVariable",
        "Set a variable on a group and synchronize across network. Usage: group khSetVariable [name, value, target, jip]",
        userFunctionWrapper<kh_set_variable_group>,
        game_data_type::ANY,
        game_data_type::GROUP,
        game_data_type::ARRAY
    );

    _sqf_kh_set_variable_location_array = intercept::client::host::register_sqf_command(
        "khSetVariable",
        "Set a variable on a location and synchronize across network. Usage: location khSetVariable [name, value, target, jip]",
        userFunctionWrapper<kh_set_variable_location>,
        game_data_type::ANY,
        game_data_type::LOCATION,
        game_data_type::ARRAY
    );

    _sqf_kh_set_variable_display_array = intercept::client::host::register_sqf_command(
        "khSetVariable",
        "Set a variable on a display and synchronize across network. Usage: display khSetVariable [name, value, target, jip]",
        userFunctionWrapper<kh_set_variable_display>,
        game_data_type::ANY,
        game_data_type::DISPLAY,
        game_data_type::ARRAY
    );

    _sqf_ts_connect = intercept::client::host::register_sqf_command(
        "tsConnect",
        "Initialize TeamSpeak IPC connection",
        userFunctionWrapper<ts_connect_sqf>,
        game_data_type::BOOL
    );

    _sqf_ts_disconnect = intercept::client::host::register_sqf_command(
        "tsDisconnect",
        "Cleanup TeamSpeak IPC connection",
        userFunctionWrapper<ts_disconnect_sqf>,
        game_data_type::BOOL
    );

    _sqf_ts_apply_voice_effects = intercept::client::host::register_sqf_command(
        "tsApplyVoiceEffects",
        "Apply voice effects to TeamSpeak transmission",
        userFunctionWrapper<ts_apply_voice_effects_sqf>,
        game_data_type::BOOL,
        game_data_type::ARRAY
    );

    _sqf_ts_clear_voice_effects = intercept::client::host::register_sqf_command(
        "tsClearVoiceEffects",
        "Clear all voice effects from TeamSpeak transmission",
        userFunctionWrapper<ts_clear_voice_effects_sqf>,
        game_data_type::BOOL
    );

    _sqf_ts_is_initialized = intercept::client::host::register_sqf_command(
        "tsIsInitialized",
        "Check if TeamSpeak integration is initialized",
        userFunctionWrapper<ts_is_initialized_sqf>,
        game_data_type::BOOL
    );

    _sqf_ts_is_plugin_active = intercept::client::host::register_sqf_command(
        "tsIsPluginActive",
        "Check if TeamSpeak plugin is active and responding",
        userFunctionWrapper<ts_is_plugin_active_sqf>,
        game_data_type::BOOL
    );

    _sqf_ts_is_connected = intercept::client::host::register_sqf_command(
        "tsIsConnected",
        "Check if connected to a TeamSpeak server",
        userFunctionWrapper<ts_is_connected_sqf>,
        game_data_type::BOOL
    );

    _sqf_ts_is_plugin_installed = intercept::client::host::register_sqf_command(
        "tsIsPluginInstalled",
        "Check if TeamSpeak plugin is installed",
        userFunctionWrapper<ts_is_plugin_installed_sqf>,
        game_data_type::BOOL
    );

    _sqf_serialize_function_code = intercept::client::host::register_sqf_command(
        "serializeFunction", 
        "Serialize a function for execution/transfer",
        userFunctionWrapper<serialize_function_unary>,
        game_data_type::STRING, 
        game_data_type::CODE
    );

    _sqf_serialize_function_string = intercept::client::host::register_sqf_command(
        "serializeFunction", 
        "Serialize a function for execution/transfer",
        userFunctionWrapper<serialize_function_unary>,
        game_data_type::STRING, 
        game_data_type::STRING
    );

    _sqf_serialize_function_bool_code = intercept::client::host::register_sqf_command(
        "serializeFunction", 
        "Serialize a function for execution/transfer",
        userFunctionWrapper<serialize_function_binary>,
        game_data_type::STRING, 
        game_data_type::BOOL, 
        game_data_type::CODE
    );

    _sqf_serialize_function_bool_string = intercept::client::host::register_sqf_command(
        "serializeFunction", 
        "Serialize a function for execution/transfer",
        userFunctionWrapper<serialize_function_binary>,
        game_data_type::STRING, 
        game_data_type::BOOL, 
        game_data_type::STRING
    );

    _sqf_call_serialized_function = intercept::client::host::register_sqf_command(
        "callSerializedFunction",
        "Call a serialized function",
        userFunctionWrapper<call_serialized_function_sqf>,
        game_data_type::ANY,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_curve_conversion = intercept::client::host::register_sqf_command(
        "curveConversion",
        "Remaps value through an easing curve; linear, smoothstep, smootherstep, easeIn, easeOut, sine, exponentialIn, exponentialOut, circular all take [minFrom, maxFrom, value, minTo, maxTo, clip]; bezier takes [minFrom, maxFrom, value, minTo, maxTo, points, clip] where points is an array of interior control ordinates (endpoints 0 and 1 are implicit; omit or [] for a classic cubic ease)",
        userFunctionWrapper<curve_conversion_sqf>,
        game_data_type::SCALAR,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_inverse_curve_conversion = intercept::client::host::register_sqf_command(
        "inverseCurveConversion",
        "Returns [value, t] where value is the input this curve needs to produce desiredOutput and 't' is the normalized progress (0..1) at that point. Useful to swap curve type/ranges mid-animation without snapping by feeding the old curve's current output and the new curve's params. Format [minFrom, maxFrom, desiredOutput, minTo, maxTo]; bezier takes [minFrom, maxFrom, desiredOutput, minTo, maxTo, points]",
        userFunctionWrapper<inverse_curve_conversion_sqf>,
        game_data_type::ARRAY,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_vector_curve_conversion = intercept::client::host::register_sqf_command(
        "vectorCurveConversion",
        "Eases a single progress value along a path between fromVec and toVec (2D or 3D), returning the eased vector with one shared curve parameter across components. Curves linear, smoothstep, smootherstep, easeIn, easeOut, sine, exponentialIn, exponentialOut, circular take [minFrom, maxFrom, value, fromVec, toVec, clip]; bezier takes [minFrom, maxFrom, value, fromVec, toVec, points, clip] where points is an array of interior control ordinates (omit or [] for a classic cubic ease)",
        userFunctionWrapper<vector_curve_conversion_sqf>,
        game_data_type::ARRAY,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_inverse_vector_curve_conversion = intercept::client::host::register_sqf_command(
        "inverseVectorCurveConversion",
        "Returns [value, t] that are the scalar progress input and normalized t (0..1) that make this curve output desiredOutputVec along the fromVec->toVec path. Useful to swap curve type/endpoints mid-animation without snapping. Format [minFrom, maxFrom, desiredOutputVec, fromVec, toVec]; bezier takes [minFrom, maxFrom, desiredOutputVec, fromVec, toVec, points]",
        userFunctionWrapper<inverse_vector_curve_conversion_sqf>,
        game_data_type::ARRAY,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_curve_slope = intercept::client::host::register_sqf_command(
        "curveSlope",
        "Format [minFrom, maxFrom, value, minTo, maxTo, floor]. Returns the curve's normalized rate of change (slope) at the position of value within minFrom..maxFrom. Use as a speed multiplier: linear gives constant 1, easeIn accelerates, easeOut decelerates, smoothstep is slow-fast-slow. Optional floor sets a minimum slope so flat regions still progress. For bezier: [minFrom, maxFrom, value, minTo, maxTo, points, floor].",
        userFunctionWrapper<curve_slope_sqf>,
        game_data_type::SCALAR,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_vector_curve_slope = intercept::client::host::register_sqf_command(
        "vectorCurveSlope",
        "Format [minFrom, maxFrom, value, fromVec, toVec, floor]. Returns the per-component rate-of-change vector slope*(toVec-fromVec) at the shared progress of value, i.e. the velocity direction/magnitude along the path. Optional floor sets a minimum slope. For bezier: [minFrom, maxFrom, value, fromVec, toVec, points, floor].",
        userFunctionWrapper<vector_curve_slope_sqf>,
        game_data_type::ARRAY,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_get_unit_yaw_speed = intercept::client::host::register_sqf_command(
        "getUnitYawSpeed",
        "Returns the unit's yaw rotation speed in degrees/second, measured over a 100ms window",
        userFunctionWrapper<get_unit_yaw_speed_sqf>,
        game_data_type::SCALAR,
        game_data_type::OBJECT
    );

    _sqf_process_execution = intercept::client::host::register_sqf_command(
        "processExecution",
        "Internal KH execution processor - dispatches an execution descriptor to its target(s)",
        userFunctionWrapper<process_execution_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_trigger_cba_event_array = intercept::client::host::register_sqf_command(
        "triggerCbaEvent",
        "Triggers a CBA event through the KH target resolution model. Format [event, arguments, target, jip], where event is either a string or [eventName, entity] for entity events. Returns the JIP handler id array when jip is requested",
        userFunctionWrapper<trigger_cba_event_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_process_cba_group_event = intercept::client::host::register_sqf_command(
        "processCbaGroupEvent",
        "Internal KH CBA dispatcher - fires an owner event on every machine owning a unit of the given group. Format [event, arguments, group]",
        userFunctionWrapper<process_cba_group_event_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_process_cba_array_event = intercept::client::host::register_sqf_command(
        "processCbaArrayEvent",
        "Internal KH CBA dispatcher - resolves an array of mixed targets to owner machines and fires per-owner events. Format [event, arguments, flattenedTargets, caller]",
        userFunctionWrapper<process_cba_array_event_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_process_cba_code_event = intercept::client::host::register_sqf_command(
        "processCbaCodeEvent",
        "Internal KH CBA dispatcher - calls the predicate with the arguments and fires a local event if it returns true. Format [event, arguments, function]",
        userFunctionWrapper<process_cba_code_event_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_sample_scene_depth_array = intercept::client::host::register_sqf_command(
        "sampleSceneDepth",
        "Read the engine depth buffer at screen position [u, v]. Returns [sceneDistM, rawDepth]; on failure [[\"error\", reason]]",
        userFunctionWrapper<sample_scene_depth_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_gpu_visibility_array = intercept::client::host::register_sqf_command(
        "gpuVisibility",
        "Synchronously test world points [[x,y,zASL], ...] (stalls the GPU; prefer queueVisibility for per-frame use). Returns [[status, pointDistM, sceneDistM], ...]; on failure [[\"error\", reason]]",
        userFunctionWrapper<gpu_visibility_sqf>,
        game_data_type::ARRAY,
        game_data_type::ARRAY
    );

    _sqf_remove_render_handler_string = intercept::client::host::register_sqf_command(
        "removeRenderHandler",
        "Remove a retained render object (mesh or post-processing pass) by its khr_ handle; '' or 'all' removes every object",
        userFunctionWrapper<remove_render_handler_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_queue_visibility_array = intercept::client::host::register_sqf_command(
        "queueVisibility",
        "Queue world points [[x,y,zASL], ...] for an async GPU depth-visibility test. Returns the queued count; -1 = invalid input",
        userFunctionWrapper<queue_visibility_sqf>,
        game_data_type::SCALAR,
        game_data_type::ARRAY
    );

    _sqf_get_visibility_results = intercept::client::host::register_sqf_command(
        "getVisibilityResults",
        "Fetch the latest completed async visibility batch. Returns [ageInFrames, [[status, pointDistM, sceneDistM], ...]]",
        userFunctionWrapper<get_visibility_results_sqf>,
        game_data_type::ARRAY
    );

    _sqf_add_render3d_array = intercept::client::host::register_sqf_command(
        "addRender3D",
        "[[x,y,zASL], size, [r,g,b,a]?, mode?, sceneRead?, effect?, fxParams?, band?, blend?, duration?, lit?, mesh?, farVis?, rotation?, twoSided?]. Returns the khr_ handle, or a plain error sentence",
        userFunctionWrapper<add_render3d_sqf>,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_update_render3d_array = intercept::client::host::register_sqf_command(
        "updateRender3D",
        "[handle, property, value]. Update a persistent 3D mesh object",
        userFunctionWrapper<update_render3d_sqf>,
        game_data_type::BOOL,
        game_data_type::ARRAY
    );

    _sqf_update_post_fx_array = intercept::client::host::register_sqf_command(
        "updatePostFX",
        "[handle, property, value]. Update a fullscreen post-processing pass",
        userFunctionWrapper<update_post_fx_sqf>,
        game_data_type::BOOL,
        game_data_type::ARRAY
    );

    _sqf_add_postfx_array = intercept::client::host::register_sqf_command(
        "addPostFX",
        "Create a persistent fullscreen post-processing pass",
        userFunctionWrapper<add_postfx_sqf>,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_add_local_postfx_array = intercept::client::host::register_sqf_command(
        "addLocalPostFX",
        "Create a persistent post-processing effect confined to a world-space volume. Returns the khr_ handle, or a plain error sentence",
        userFunctionWrapper<add_local_postfx_sqf>,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_get_render_stats = intercept::client::host::register_sqf_command(
        "getRenderStats",
        "Render health counters. First call arms + zeroes the diagnostics (and the flight recorder) and returns [['status', 'armed']]",
        userFunctionWrapper<get_render_stats_sqf>,
        game_data_type::ARRAY
    );

    _sqf_reset_render_stats = intercept::client::host::register_sqf_command(
        "resetRenderStats",
        "Re-zero the render diagnostic counters without restarting the session, so a setRenderDebug A/B can be measured in its own window. Cold-timeline stamps are left alone. Returns true",
        userFunctionWrapper<reset_render_stats_sqf>,
        game_data_type::BOOL
    );

    _sqf_set_render_debug = intercept::client::host::register_sqf_command(
        "setRenderDebug",
        // the SQF-visible description is deliberately NARROW. ADDING A MODE
        // STILL MEANS TWO EDITS - that catalog comment and the whitelist
        // above - and there is no compiler check that they agree.
        "Sets the render debug mode",
        userFunctionWrapper<set_render_debug_sqf>,
        game_data_type::BOOL,
        game_data_type::SCALAR
    );

    _sqf_set_ssgi_scale = intercept::client::host::register_sqf_command(
        "setSsgiScale",
        "SSGI gather resolution multiplier: 1 full res, 0.5 half (default), 2 supersampled; clamped 0.25-2. Global (the gather buffers are singletons); applies next flush. Returns true on accept",
        userFunctionWrapper<set_ssgi_scale_sqf>,
        game_data_type::BOOL,
        game_data_type::SCALAR
    );

    _sqf_flush_ui_render = intercept::client::host::register_sqf_command(
        "flushUIRender",
        "Renders all UI-affecting passes",
        userFunctionWrapper<flush_ui_render_sqf>,
        game_data_type::BOOL
    );

    _sqf_dump_render_trace = intercept::client::host::register_sqf_command(
        "dumpRenderTrace",
        "Dump the render flight recorder. First call arms recording and returns [['status', 'armed']]; frames flow from the next call",
        userFunctionWrapper<dump_render_trace_sqf>,
        game_data_type::ARRAY
    );

    _sqf_dump_dynamic_lights = intercept::client::host::register_sqf_command(
        "dumpDynamicLights",
        "Dynamic-light recon dump. First call arms the census and returns [['status', 'armed']]",
        userFunctionWrapper<dump_dynamic_lights_sqf>,
        intercept::types::game_data_type::ARRAY
    );

    g_compiled_sqf_generic_call = sqf::compile(R"(setReturnValue (call _thisFunction);)");
    g_compiled_sqf_generic_call_args = sqf::compile(R"(setReturnValue (_thisArguments call _thisFunction);)");

    g_compiled_kh_set_variable_generic = sqf::compile(R"(
        getCallArguments params ["_khNamespace", "_khVariable", "_khValue", "_khTarget"];
        _khNamespace setVariable [_khVariable, _khValue, _khTarget];
    )");

    g_compiled_kh_cba_local_event = sqf::compile(R"(setReturnValue (getCallArguments call CBA_fnc_localEvent);)");
    g_compiled_kh_cba_server_event = sqf::compile(R"(setReturnValue (getCallArguments call CBA_fnc_serverEvent);)");
    g_compiled_kh_cba_owner_event = sqf::compile(R"(setReturnValue (getCallArguments call CBA_fnc_ownerEvent);)");
    g_compiled_kh_cba_target_event = sqf::compile(R"(setReturnValue (getCallArguments call CBA_fnc_targetEvent);)");
    g_compiled_kh_cba_group_owner_dispatch = sqf::compile(R"(processCbaGroupEvent _this;)");
    g_compiled_kh_cba_array_target_dispatch = sqf::compile(R"(processCbaArrayEvent _this;)");
    g_compiled_kh_cba_code_target_dispatch = sqf::compile(R"(processCbaCodeEvent _this;)");

    g_compiled_kh_cba_callback_receiver = sqf::compile(R"(
        params ["_event", "_arguments", "_parsedTargets"];
        _argsCallback params ["_eventReceiver"];

        if !(isNil "_eventReceiver") then {
            if !(_eventReceiver in _parsedTargets) then {
                _parsedTargets pushBackUnique _eventReceiver;
                [_event, _arguments, _eventReceiver] call CBA_fnc_ownerEvent;
            };
        };
    )");

    g_compiled_kh_cba_callback_predicate = sqf::compile(R"(
        params [["_arguments", []], "_function"];

        if (_arguments call _function) then {
            [clientOwner];
        }
        else {
            [];
        };
    )");

    g_compiled_sqf_add_game_event_handler = sqf::compile(R"(setReturnValue (getCallArguments call KH_fnc_addEventHandler);)");
    g_compiled_sqf_remove_game_event_handler = sqf::compile(R"(setReturnValue (getCallArguments call KH_fnc_removeHandler);)");
    g_compiled_sqf_game_event_handler_lua_bridge = sqf::compile(R"(_this luaTriggerEvent _args;)");

    g_compiled_sqf_execute_lua = sqf::compile(R"(
        private _khargs = getCallArguments;
        _khArgs set [1, compile (["_khArgs luaExecute ", _khArgs select 1] joinString "")];
        private _special = _khargs param [4, false, [true, []]];

        if (_special isEqualType []) then {
            private _type = _special param [0, "", [""]];

            if (_type isEqualTo "CALLBACK") then {
                _special set [2, compile (["_this append _argsCallback; _this luaExecute ", _special select 2] joinString "")];
            }
            else {
                if (_type isEqualTo "PERSISTENT") then {
                    _special set [3, compile (["_this luaExecute ", _special select 3] joinString "")];
                };
            };
        };

        setReturnValue ((_khArgs select 0) execute (_khArgs select [1]));
    )");

    g_compiled_sqf_remove_handler = sqf::compile(R"(setReturnValue (getCallArguments call KH_fnc_removeHandler);)");
    g_compiled_sqf_create_hash_map_from_array = sqf::compile(R"(setReturnValue (createHashMapFromArray getCallArguments);)");
    g_compiled_sqf_create_hash_map = sqf::compile(R"(setReturnValue createHashMap;)");
    g_compiled_sqf_trigger_lua_reset_event = sqf::compile(R"(setReturnValue (["KH_eve_luaReset"] call CBA_fnc_localEvent);)");
    g_compiled_ai_initialized_event = sqf::compile(R"(["KH_eve_aiInitialized", _khargs] call CBA_fnc_localEvent;)");
    g_compiled_ai_response_progress_event = sqf::compile(R"(["KH_eve_aiResponseProgress", _khargs] call CBA_fnc_localEvent;)");
    g_compiled_ai_response_event = sqf::compile(R"(["KH_eve_aiResponse", _khargs] call CBA_fnc_localEvent;)");
    g_compiled_tts_generated_event = sqf::compile(R"(["KH_eve_ttsGenerated", _khargs] call CBA_fnc_localEvent;)");
    g_compiled_tts_finished_event = sqf::compile(R"(["KH_eve_ttsFinished", _khargs] call CBA_fnc_localEvent;)");
    g_compiled_stt_transcription_event = sqf::compile(R"(["KH_eve_sttTranscription", _khargs] call CBA_fnc_localEvent;)");
    g_compiled_html_js_event = sqf::compile(R"(["KH_eve_htmlJsEvent", _khargs] call CBA_fnc_localEvent;)");
    g_compiled_kh_empty_code = sqf::compile(R"(nil;)");
    g_compiled_kh_subfunction_basic = sqf::compile(R"((_this select 0) callSerializedFunction (_this select [1]);)");
    g_compiled_kh_subfunction_process = sqf::compile(R"(processExecution _this;)");
    g_compiled_kh_monitor_set = sqf::compile(R"(KH_var_temporalExecutionStackMonitor set getCallArguments;)");

    g_compiled_kh_monitor_wrapper_scalar = sqf::compile(R"(
        params ["_arguments", "_timeoutFunction", "_environmentType", "_environmentId", "_return"];
        private _handlerId = [[["TEMPORAL"], _environmentType, _environmentId, clientOwner], _return];
        _arguments call _timeoutFunction;
    )");

    g_compiled_kh_monitor_wrapper_code = sqf::compile(R"(
        params ["_arguments", "_timeoutFunction", "_environmentId", "_interval", "_return"];
        private _handlerId = [[["TEMPORAL"], _interval, _environmentId, clientOwner], _return];
        _arguments call _timeoutFunction;
    )");

    g_compiled_kh_handler_scalar_iteration = sqf::compile(R"(
        params ["_fedArguments", "_subfunction", "_environmentId"];
        _fedArguments call _subfunction;
        triggerCbaEvent ["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false];
    )");

    g_compiled_kh_handler_scalar = sqf::compile(R"(
        params ["_fedArguments", "_subfunction"];
        _fedArguments call _subfunction;
    )");

    g_compiled_kh_handler_timeout = sqf::compile(R"(
        params ["_environmentId", "_timeoutId"];
        triggerCbaEvent ["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, false], true, false];
        KH_var_temporalExecutionStackDeletions pushBackUnique _timeoutId;
    )");

    g_compiled_kh_handler_code_iteration_hard_fail = sqf::compile(R"(
        params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

        if (_arguments call _environmentType) then {
            _fedArguments call _subfunction;
            triggerCbaEvent ["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false];
        }
        else {
            triggerCbaEvent ["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true], true, false];
        };
    )");

    g_compiled_kh_handler_code_iteration_soft_fail = sqf::compile(R"(
        params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

        if (_arguments call _environmentType) then {
            _fedArguments call _subfunction;
            triggerCbaEvent ["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false];
        }
        else {
            triggerCbaEvent ["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, true], true, false];
        };
    )");

    g_compiled_kh_handler_code_iteration = sqf::compile(R"(
        params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

        if (_arguments call _environmentType) then {
            _fedArguments call _subfunction;
            triggerCbaEvent ["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false];
        };
    )");

    g_compiled_kh_handler_code_hard_fail = sqf::compile(R"(
        params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

        if (_arguments call _environmentType) then {
            _fedArguments call _subfunction;
        }
        else {
            triggerCbaEvent ["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true], true, false];
        };
    )");

    g_compiled_kh_handler_code = sqf::compile(R"(
        params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

        if (_arguments call _environmentType) then {
            _fedArguments call _subfunction;
        };
    )");

    g_compiled_kh_handler_string = sqf::compile(R"(
        params ["_fedArguments", "_subfunction", "_environmentId"];
        _fedArguments call _subfunction;
        KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
        KH_var_temporalExecutionStackMonitor deleteAt _environmentId;
    )");

    g_compiled_kh_callback_handler = sqf::compile(R"(
        _args params ["_arguments", "_function", "_unscheduled"];
        private _argsCallback = _this;

        if _unscheduled then {
            if (isNil "_arguments") then {
                call (missionNamespace getVariable _function);
            }
            else {
                _arguments call (missionNamespace getVariable _function);
            };
        }
        else {
            if (isNil "_arguments") then {
                [] spawn (missionNamespace getVariable _function);
            }
            else {
                _arguments spawn (missionNamespace getVariable _function);
            };
        };

        [_handlerId] call KH_fnc_removeHandler;
    )");

    g_compiled_kh_persistent_marker = sqf::compile(R"((_this select 0) setVariable [_this select 1, true];)");

    g_compiled_kh_immediate_call = sqf::compile(R"(
        private _khImmediate = getCallArguments;
        private _handlerId = _khImmediate select 2;
        private _totalDelta = 0;
        private _executionTime = _khImmediate select 3;
        private _executionCount = 0;
        private "_previousReturn";
        setReturnValue ((_khImmediate select 0) call (_khImmediate select 1));
    )");

    g_compiled_kh_ui_render_init = sqf::compile(R"(
        private _display = findDisplay 46;

        if (isNull _display) exitWith { 
            setReturnValue false;
        };

        private _old = uiNamespace getVariable ["kh_var_uiDriverControl", controlNull];

        if !(isNull _old) then { 
            ctrlDelete _old;
        };

        private _control = _display ctrlCreate ["RscMapControlEmpty", -1];

        if (isNull _control) then { 
            _control = _display ctrlCreate ["RscMapControlEmpty", -1] 
        };

        if (isNull _control) exitWith { 
            setReturnValue false;
        };

        _control ctrlSetPosition [0, 0, 0.000001, 0.000001];
        _control ctrlCommit 0;

        _control ctrlAddEventHandler [
            "Draw", 
            { 
                flushUIRender;
            }
        ];

        uiNamespace setVariable ["kh_var_uiDriverControl", _control];
        setReturnValue true;
    )");
}