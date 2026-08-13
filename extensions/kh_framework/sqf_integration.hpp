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
static registered_sqf_function _sqf_reset_render_stats;   // 26180
static registered_sqf_function _sqf_set_ssgi_scale;   // 26115
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
    std::string canonical_name;      // as reported by supportInfo, for error text
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
// operators supportInfo also reports (+, -, ==, #, !, &&, ...), which would
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
        return;  // supportInfo repeats each command once per type combination
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

// supportInfo entries look like:
//   "n:time", "u:count ARRAY", "b:ARRAY select SCALAR", "t:OBJECT"
static void kh_parse_support_info_entry(const std::string& raw) {
    if (raw.size() < 3 || raw[1] != ':') {
        return;
    }

    const char kind = static_cast<char>(std::tolower(static_cast<unsigned char>(raw[0])));

    if (kind != 'n' && kind != 'u' && kind != 'b') {
        return;  // 't:' type entries and anything unexpected
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
            // Build CBA parameters: ["KH_eve_khDataWriteEmission", [file_str, var_name, value], target, jip]
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
        game_value event_args = left_arg;  // Left arg is the actual arguments
        
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
            lua_remove(L, -2);  // Clean up event table from stack
            lua_pushstring(L, event_name.c_str());
            
            // Push target (arg 2) - always true for local
            lua_pushboolean(L, true);
            
            // Push jip (arg 3) - always false for local
            lua_pushboolean(L, false);
            
            // Now push event arguments (variadic args starting from arg 4)
            int arg_count = 3; // event_name, target, jip

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
                
                // Target
                if (arr.size() > 2) {
                    target = arr[2];
                }
                
                // JIP
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
        
        // Target
        if (arr.size() > 2) {
            target = arr[2];
        }
        
        // JIP
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
        // Nest in outer array for _this call since the remover accepts array in _this
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

        // Right vector = dir × up
        float rightX = dirY * upZ - dirZ * upY;
        float rightY = dirZ * upX - dirX * upZ;
        float rightZ = dirX * upY - dirY * upX;
        float rightLen = std::sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
        if (rightLen < EPSILON) return make_zero_result();
        rightX /= rightLen; rightY /= rightLen; rightZ /= rightLen;

        // Re-orthogonalize up = right × dir
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

        // Right vector = dir × up
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

        // Negate to match eulerToVector convention: R_Z(-yaw) · R_Y(-roll) · R_X(-pitch)
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

        // ZYX euler extraction
        // These extract the angles of R_Z(-yaw) R_Y(-roll) R_X(-pitch)
        float sinRoll = 2.0f * (w * y - z * x);
        float aroundX, aroundY, aroundZ;

        if (std::abs(sinRoll) > 1.0f - EPSILON) {
            // Gimbal lock — roll is ±90°
            aroundX = 0.0f;
            aroundY = (sinRoll < 0.0f) ? 90.0f : 270.0f;
            float coupled = std::atan2(2.0f * (x * y + w * z), 1.0f - 2.0f * (y * y + z * z));
            aroundZ = std::fmod(-coupled * RAD_TO_DEG + 360.0f, 360.0f);
        } else {
            // Normal case — negate extracted angles to undo the convention negation
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

        // Right = dir × up
        float rightX = dirY * upZ - dirZ * upY;
        float rightY = dirZ * upX - dirX * upZ;
        float rightZ = dirX * upY - dirY * upX;
        float rightLen = std::sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
        if (rightLen < EPSILON) return make_identity();
        rightX /= rightLen; rightY /= rightLen; rightZ /= rightLen;

        // Re-orthogonalize up = right × dir
        upX = rightY * dirZ - rightZ * dirY;
        upY = rightZ * dirX - rightX * dirZ;
        upZ = rightX * dirY - rightY * dirX;

        // Rotation matrix columns: [right, dir, up]
        // m00=rightX  m01=dirX  m02=upX
        // m10=rightY  m11=dirY  m12=upY
        // m20=rightZ  m21=dirZ  m22=upZ
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

        // Rotation matrix from quaternion
        // dir = R * [0,1,0] (column 1 of rotation matrix)
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
            // Very close — linear interpolation to avoid division by near-zero sin
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

        // Right = dir × up
        float rightX = dirY * upZ - dirZ * upY;
        float rightY = dirZ * upX - dirX * upZ;
        float rightZ = dirX * upY - dirY * upX;
        float rightLen = std::sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
        if (rightLen < EPSILON) return make_identity();
        rightX /= rightLen; rightY /= rightLen; rightZ /= rightLen;

        // Re-orthogonalize up = right × dir
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
            // Zero axis — return identity
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
            // Near-zero rotation — axis is arbitrary
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
                    // [0] dependency (object or group; null/other = no dependency gating)
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

                    // jip_key
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

        // below stores the conditional wrapper instead (no plain-then-replace).
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
            // Back-compat: a bare handler id targets a handler owned by this machine.
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

                    // jip_key
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

        // Store the JIP message now, unless the target is CODE — the CODE branch
        // below stores the conditional wrapper instead (no plain-then-replace).
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

            // _parse == false: a bare global function name is returned unchanged, no engine calls.
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

            // Case 2b: not a command -> resolve from missionNamespace (unchanged)
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

// Dispatches a compiled dispatcher shim through the native KH_fnc_execute equivalent: execute [_arguments, _function, _target, true, false]
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

// Fires CBA_fnc_targetEvent at the subset of KH_var_allPlayerUnits matching the predicate; the event fires unconditionally, even on an empty selection, mirroring the original
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

// Mirrors the SQF group dispatch: a local group with no involved player units fires locally, groups without player units use targetEvent, otherwise the server fans out per-owner events
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

// Shared local fallback for string targets: player name/roleDescription match -> targetEvent; groupId match -> per-group dispatch; marker match -> targetEvent on units in area
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

// Native KH_fnc_triggerCbaEvent, registered as the unary 'triggerCbaEvent' command; right arg is [_event, _arguments, _target, _jip]
static game_value trigger_cba_event_sqf(game_value_parameter params) {
    try {
        if (params.type_enum() != game_data_type::ARRAY) return game_value();
        auto& p = params.to_array();
        game_value event = kh_param(p, 0, game_value(std::string()), {game_data_type::STRING, game_data_type::ARRAY});
        game_value arguments = p.size() > 1 ? p[1] : game_value();
        game_value target = kh_param(p, 2, game_value(true), {game_data_type::BOOL, game_data_type::SCALAR, game_data_type::STRING, game_data_type::ARRAY, game_data_type::CODE, game_data_type::OBJECT, game_data_type::TEAM_MEMBER, game_data_type::GROUP, game_data_type::SIDE, game_data_type::LOCATION});
        game_value jip = kh_param(p, 3, game_value(false), {game_data_type::BOOL, game_data_type::ARRAY});

        // _event as [_eventName, _entity] -> ["KH_eve_entityCbaEvent", hashValue _entity, _eventName] joinString "_"
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
                    // parseNumber on a non-numeric first character yields 0, which is the fault check; owner ids, uids and net ids never start with 0
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

// Unique-inserts a KH machine list, optionally excluding one machine (KH_var_allMachines - [_caller] semantics)
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

// Server-side named-target fallback for the array dispatcher: name/roleDescription -> unit targets; groupId -> unit owners; marker -> units in area
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

// Native group fan-out, registered as 'processCbaGroupEvent'; runs on the server via the dispatch shim. Format [event, arguments, group]
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

// Native predicate dispatch, registered as 'processCbaCodeEvent'; runs on every machine via the dispatch shim. Format [event, arguments, function]
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

// Native array target parser, registered as 'processCbaArrayEvent'; runs on the server via the dispatch shim. Format [event, arguments, flattenedTargets, caller]
static game_value process_cba_array_event_sqf(game_value_parameter params) {
    try {
        if (params.type_enum() != game_data_type::ARRAY) return game_value();
        auto& p = params.to_array();
        game_value event = p.size() > 0 ? p[0] : game_value();
        game_value arguments = p.size() > 1 ? p[1] : game_value();
        game_value targets = kh_param(p, 2, game_value(auto_array<game_value>()), {game_data_type::ARRAY});
        game_value caller = kh_param(p, 3, game_value(static_cast<float>(sqf::client_owner())), {game_data_type::SCALAR});

        // Built inside a game_value from the start so the CODE-case callback closure shares the same live array for deduplication, mirroring SQF reference semantics
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
                        // parseNumber fault check; a non-numeric first character means it cannot be an id, uid or net id
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

// Mirrors the private declarations at the top of the original immediate blocks, so user functions can still read them through SQF's shared call scope
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

// Native KH_fnc_processExecution; also registered as the internal 'processExecution' command so deferred temporal stack entries can reach it
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

// Immediate dispatch of the subfunction: basic -> callSerializedFunction path, otherwise processExecution
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

        // BOOL environment: immediate local dispatch, _environment doubles as _unscheduled
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

        // ============================== SCALAR ==============================
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

        // =============================== CODE ===============================
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

        // ============================== STRING ==============================
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

        // ============================== DEFAULT ==============================
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
    const size_t dim = from_vec.size() < to_vec.size() ? from_vec.size() : to_vec.size(); // 2 or 3 (min of both)
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

    // recover shaped from the component with the largest span (most stable); shared t means any non-degenerate axis works
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

    // walk backwards accumulating wrap-corrected delta until we cover the window (or run out of samples)
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
    // ARRAY form is the full signature: [arguments, function, target, environment, special]
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

    // Entity initialization deletions
    game_value entity_deletions_gv = sqf::get_variable(ns, "kh_var_entityinitializationsdeletions");

    if (!entity_deletions_gv.is_nil() && entity_deletions_gv.type_enum() == game_data_type::ARRAY) {
        auto& entity_deletions = entity_deletions_gv.to_array();

        if (entity_deletions.size() > 0) {
            game_value entity_init_gv = sqf::get_variable(ns, "kh_var_entityinitializations");

            if (!entity_init_gv.is_nil() && entity_init_gv.type_enum() == game_data_type::ARRAY) {
                auto& entity_init = entity_init_gv.to_array();
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

            // Inject the loop locals so the executed function inherits them (call shares scope)
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

// ---------------------------------------------------------------------------
// SQF entry points
// ---------------------------------------------------------------------------

// Uniform ARRAY error shape for the array-returning query commands (the
// same pair-in-array idiom as [["status","armed"]]): success returns the
// data array, failure returns [["error", <sentence>]] - one return type
// per command, message preserved. Callers key on element 0's first field.
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
        out.push_back(game_value(results[2]));  // scene distance, meters
        out.push_back(game_value(results[3]));  // raw depth buffer value
        return game_value(std::move(out));
    } catch (const std::exception& e) {
        report_error(std::string("sampleSceneDepth: ") + e.what());
        return kh_error_pairs(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("sampleSceneDepth: unknown exception");
        return kh_error_pairs("EXCEPTION: unknown");
    }
}

// gpuVisibility [[x,y,zASL], [x,y,zASL], ...]   (any count; the GPU set grows to fit)
// Tests every point against the engine depth buffer in ONE GPU dispatch.
// Returns one entry per point: [status, pointDistM, sceneDistM]
//   status: 1 = visible, 0 = occluded by scene geometry, -1 = offscreen/behind camera
// Note: like all depth-based tests, cannot account for particles (they do not
// write depth). Call from Draw3D.
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
            e.push_back(game_value(results[i * 4 + 0]));  // status
            e.push_back(game_value(results[i * 4 + 1]));  // point distance, m
            e.push_back(game_value(results[i * 4 + 2]));  // scene distance at pixel, m
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

// addRender3D [[x,y,zASL], size, [r,g,b,a]?, mode?, sceneRead?, effect?,
//              fxParams?, band?, blend?, duration?, lit?, mesh?, farVis?,
//              rotation?, twoSided?]
// Adds a persistent mesh drawn every frame by the internal Draw3D EH until
// removed. Callable from ANY context (scheduled, unscheduled, callbacks).
//   size:      NUMBER (uniform) or ARRAY [x, y, z] - a MULTIPLIER of the
//              mesh's OWN native dimensions, per axis. 1 = true scale,
//              2 = twice as big, 0.5 = half; 0 reads as 1. Aspect ratio
//              is always preserved - there is no box fit, so a model is
//              never squashed to a cube. Negative values are accepted
//              and mean |value|, identical to positive (the pre-26422
//              spelling, kept working). The array's axes are the
//              OBJECT'S OWN [x, y, z] in Arma sense (east/north/up at
//              zero rotation) and are applied BEFORE 'rotation', so
//              [1, 1, 3] makes an object three times taller along its
//              own up axis at any heading. Builtin meshes author at
//              native 1, so for "box"/"sphere"/... the multiplier is
//              still exactly metres and nothing about them changes.
//   mode:      0 = depth test (default), 1 = test + depth write, 2 = overlay
//   sceneRead: BOOL, shorthand for a tinted scene-read surface
//              (effect "colorgrade" at neutral defaults: scene through the
//              mesh, tinted by color.rgb, blended by color.a)
//   effect:    STRING or SCALAR - screen-space effect applied inside the
//              mesh's footprint: "solid" 0, "invert" 1, "colorgrade" 2,
//              "vignette" 3, "chromatic" 4, "grain" 5, "sharpen" 6,
//              "blur" 7, "bloom" 8, "distortion" 9, "outline" 10,
//              "pulse" 11, "halation" 12, "fog" 13, "lensflare" 14,
//              "anamorphic" 15, "sunflare" 16, "glitch" 17
//              "clarity" 18 [strength, radiusPx]
//              (luma-only local contrast; chroma-preserving),
//              "deband" 19 [threshold 1/255, rangePx, grain 1/255]
//              (gradient debanding + triangular dither),
//              "rainlens"/"rain" 20 [intensity, speed, condensation,
//              refract] (procedural motion-reactive lens rain; params
//              [4..7] are SYSTEM-owned - the camera velocity; 26083: ids
//              compacted after the ssao retirement - scripts using the
//              old NUMERIC 19/20/21 must move to 18/19/20 or, better,
//              the stable string names),
//              "crt" 21 [curvature, scanlines, lineCount, maskStrength,
//              aberrationPx, flicker, rollingBand, cornerRadius,
//              scanSpeed, scanWobblePx] (26089: params [8..9] - raster
//              scroll in lines/s, signed for direction, and per-line
//              horizontal tracking wobble in px; 0/0 = static raster;
//              effect params widened to 12 slots pipeline-wide)
//              (26088: curved-glass CRT tube - barrel distortion with
//              rounded black bezel, off-axis chromatic fringing,
//              luma-widened beam scanlines, RGB aperture grille,
//              rolling sync band, mains flicker; color.rgb = phosphor
//              tint, color.a = opacity),
//              "ssgi" 22 [intensity, radiusM, samples, normalBias,
//              falloffPow, saturation, maxDistM, lumClamp, albedoMod,
//              giOnly, planeBias] (26096: single-bounce screen-space global
//              illumination - gathers nearby scene radiance through
//              depth-reconstructed positions and normals; radius in
//              METERS, samples 4..32 is the quality knob (cost is
//              linear in it), normalBias raises the cosine-lobe floor
//              against acne/leaks, maxDistM fades GI out by camera
//              distance (26098: default 300 m - reconstruction
//              precision degrades toward the standard-z far; 0 = no
//              fade), lumClamp caps HDR fireflies,
//              albedoMod 0..1 tints the bounce by the receiving
//              surface, giOnly 1 renders the bounce term alone for
//              tuning, planeBias scales the depth-proportional
//              tangent-plane admission floor that excludes depth-
//              quantization self-lighting (26097; 0 = auto, raise if
//              shimmer, lower if near-field bounce looks starved);
//              color.rgb = bounce tint, color.a = opacity;
//              scene-phase effect - needs live depth, stands down
//              for the frame without it; 26099: halo suppression via
//              a depth-gap sample-facing term is built in; 26101: the
//              pass now draws as a GATHER + depth-aware RESOLVE pair -
//              the bounce is bilaterally smoothed before compositing
//              (fx2.w = resolve spread px, 0 = auto 3.0, clamp 1..8) -
//              and is scene-phase only, rejected in the write window;
//              26102: the gather runs HALF-RES on an fp16 side buffer
//              (banding-free) and the resolve upsamples it depth-
//              guided at full res; 26111: a half-res a-trous
//              pre-smooth between gather and resolve widens the
//              noise support (sparse small emitters smooth out), the
//              composite is output-dithered against 8-bit banding,
//              and every tap is segment-occlusion tested against the
//              depth buffer - blockers attenuate bounce smoothly;
//              26112: radiance taps sample a scene MIP chain and the
//              projected-radius cap rose 384 -> 2048 px (the close-
//              range/zoom bounce-contraction fix), the dither is
//              TPDF, and the a-trous runs two iterations; 26115:
//              the emitter-cosine slack scales with sample depth
//              (distant-bounce regression fix), taps carry +-1
//              source-LSB TPDF (intense-source banding), and
//              setSsgiScale <0.25..2> picks the gather resolution
//              globally - 0.5 default; 26116: taps read a dedicated
//              fp16 radiance pyramid, retiring the intense-source
//              banding/grain the 8-bit mips re-quantized in; 26117:
//              emitter cosines use TRUE reconstructed sample normals
//              (view-independent - closes the oblique-view leak of a
//              proud screen onto its adjacent wall), proxy fallback
//              at silhouettes; 26119: per-stratum radius jitter
//              retires the concentric estimator rings around compact
//              hot emitters, and the AUTO resolve spread scales with
//              the gather grid so coarse setSsgiScale values smooth
//              proportionally; 26120: tap mip footprints floor at
//              the stratum gap (contiguous radial coverage - the
//              residual expectation rings), the mip rebase honors
//              setSsgiScale, and the a-trous runs a third iteration;
//              26121: pyramid taps sample a quincunx TENT over the
//              box mips; 26122: the pyramid LEVELS build manually
//              with a wide-tent decimation kernel (GenerateMips'
//              box aliased at every level - the ring family and the
//              camera-motion/rotation phasing were its aliasing
//              sliding across the screen-fixed decimation grid). KNOWN SCREEN-SPACE LIMIT: a blocker
//              at the SAME view depth as the emitter (a hood or
//              cowl over a screen) cannot be seen by the segment
//              occlusion test - penetration reads zero inside the
//              margin - so some spill past same-depth covers
//              remains; the lit inner face of such a cover is a
//              genuine emitter besides. Localized masks or per-pass
//              intensity/maxDist are the practical dampers),
//              "fogscatter" 23 [intensity, maxRadiusPx (0 = auto:
//              screenHeight / 90), samples (4..24)] (26104: fog light
//              scattering - everything seen through fog blurs, body
//              and silhouette alike, in proportion to the fog along
//              the sight line. The GAME's fog (mission fogParams
//              through the located engine terms) and this framework's
//              own "fog" passes BOTH feed the density - they stack as
//              independent media. params [4..11] are SYSTEM lanes:
//              the flush packs the two strongest active GLOBAL "fog"
//              passes there; localized/banded "fog" passes keep their
//              look but shed no scatter. Scene-phase only (write-
//              window rejected). Create it AFTER the "fog" passes it
//              should scatter, so it blurs the fogged image; color.a
//              = opacity) - or a PATH
//              ENDING ".hlsl" (case-insensitive suffix, the mesh slot's
//              ".fbx" rule; same Documents-then-mods "rendering"
//              resolution) - or a PATH ENDING ".cube" (same resolution):
//              a 3D color LUT applied inside the footprint, params =
//              [strength 0..1 (default 1), domain 0 auto/1 raw/2
//              display (default 0)], tetrahedral interpolation
//              (26077/26078). A ".hlsl" path is a CUSTOM pixel shader
//              compiled against the
//              framework's shared cbuffer header, entry
//              float4 PSEffect(VSOut i) : SV_Target. The shader declares
//              its own sceneColor t0 / depth t1; fx0/fx1 carry this
//              slot's 8 params verbatim and fxMeta = (id, time, w, h),
//              so custom parameters flow through the existing params
//              argument and "params" update property - no new command
//              arguments. A failed compile is reported once and the
//              effect simply does not draw
//   params:    ARRAY of up to 12 numbers (26089 widening), effect-specific (see set_effect_params
//              for meanings and defaults; omitted entries take defaults)
//   band:      [minDist, maxDist, falloff?] - additionally confines the mesh's
//              effect to a camera-distance band (maxDist <= 0 = unbounded)
//   lit:       (index 10) BOOL, or ARRAY [ambient, diffuse] - shade the
//              mesh with the engine's own sun/moon light (cascade-derived
//              direction, located-block colors) and per-pixel world
//              shadowing. Defaults ambient 0.4 / diffuse 0.6; with the
//              lighting block live,
//              [1, 1] is engine-true brightness.
//   mesh:      (index 11) STRING or SCALAR - registry mesh: builtins
//              "box"/"cube" (0, default), "steps"/"test" (1, concave
//              self-shadowing exercise), "sphere"/"ball" (2),
//              "cylinder"/"cyl" (3), "cone" (4), "pyramid" (5), a
//              registry index - or a PATH ENDING ".fbx" (case-
//              insensitive suffix required): resolved against
//              Documents\Arma 3\kh_framework\rendering first, then
//              every active mod's "rendering" folder (subfolders
//              searched; relative paths honored), imported once and
//              cached. The model is drawn at its NATIVE dimensions
//              times 'size' per axis (see 'size' above), so aspect
//              ratio is always preserved. Textures/shaders attach via
//              updateRender3D "material".
// Solid, non-overlay meshes are ALWAYS composited: injected into the frame
// BEFORE the engine draws its translucents, with depth written, so the
// engine itself composites smoke/particles against them pixel-perfectly
// (automatic fallback to the post-scene flush if the draw hook is
// unavailable). Effect and overlay meshes render on the flush path.

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
        
        // 26422: script sets the MULTIPLIER (ledger at
        // KH_SCALE_IS_A_MULTIPLIER); kh_apply_native_size resolves it to
        // metres below, once the mesh slot has been read.
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
                // folders, imported once (synchronous - the first spawn of
                // a model pays the parse), cached thereafter.
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

// ---------------------------------------------------------------------------
// Update commands, SPLIT by object kind. 3D mesh objects (addRender3D) and
// fullscreen passes (addPostFX / addLocalPostFX) share a handle space and
// several properties, but their non-shared properties must not overlap:
// each command owns exactly its kind, rejects the other's handles, and the
// genuinely common set lives in ONE helper so the two can never drift.
// ---------------------------------------------------------------------------

// Property set BOTH kinds own. Returns 1 = applied, 0 = recognized but the
// value was invalid, -1 = not a shared property (fall through to the
// caller's kind-specific set).
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

    // 26279 SHADOW CASTER PROXY. Pair it with visible false: the object then
    // casts into our private sun map and is drawn by nothing. This is the whole
    // unit-shadow feature - a proxy box per unit, refreshed from SQF, sampled by
    // SunShadowFactorSelf, which both mesh shaders already multiply into smf.
    // Nothing here touches the engine stencil-volume transport.
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

// updateRender3D [handle, property, value] -> BOOL
// 3D mesh objects ONLY (addRender3D handles); fullscreen passes belong to
// updatePostFX. Properties: "position" [x,y,zASL] | "size" number|[x,y,z]
// (a MULTIPLIER of the mesh's own native dimensions per OBJECT-LOCAL axis,
// applied before "rotation"; 1 = true scale, 0 reads as 1, negatives mean
// |value| - full rule at KH_SCALE_IS_A_MULTIPLIER. Changing "mesh" re-
// resolves the stored multiplier against the NEW mesh's dimensions) | "mesh"
// string/scalar (builtin name, registry index, or a ".fbx" path - same
// resolution as addRender3D) | "material" array - per-submesh shader +
// texture assignments:
//   [ [selector, shader, textures?, params?], ... ]
//   shader:   "pbr", or a PATH ENDING ".hlsl" (same resolution as .fbx):
//             a CUSTOM surface shader defining
//             float3 KhUserShade(KhMatSurf m, float3 wpos, float3 n, float smf)
//             called in place of the builtin PBR on every textured
//             pipeline variant (KhApplyPBR remains callable as a base);
//             builtin PBR is the fallback for a failed compile
//             (reported once)
//   selector: FBX material / submesh name (case-insensitive; builtins
//             have one submesh, "default") | submesh index | -1 or "*"
//             for all
//   textures: [ [path, slot, routing?], ... ] with slot "diffuse"|
//             "normal"|"orm"|"emissive"|"specular" (png/jpg/tga/bmp/dds,
//             resolved like .fbx paths) and optional routing
//             [ [input, channel], ... ] - input "occlusion"|"roughness"|
//             "metallic"|"alpha"|"gloss", channel "r"|"g"|"b"|"a" - to
//             read any channel of that texture into that shader input
//   params:   [ [key, value], ... ] - "baseColor" [r,g,b] | "roughness" |
//             "metalness" | "emissiveIntensity" | "normalStrength" |
//             "cutoff" | "alphaMode" ("opaque"|"cutout")
// | "color" [r,g,b,a] | "mode" 0..2 | "visible" bool | "sceneread" bool |
// "effect" string/scalar/".hlsl"/".cube" path | "params" array (resets omitted entries to the
// effect's defaults) | "blend" string | "band" [minDist, maxDist,
// falloff?] ([] clears) | "lit" bool or [ambient, diffuse] ("lighting"
// accepted as an alias) | "duration" number or [fadeIn, hold, fadeOut].
// Returns false for unknown handles, fullscreen handles, unknown
// properties, or invalid values.
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
            RenderIntegration::kh_apply_native_size(obj);   // multiplier -> metres (26422)
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
            // assignments (format at kh_apply_material_update). Errors
            // report the reason and return false.
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

// updatePostFX [handle, property, value] -> BOOL
// Fullscreen post-processing passes ONLY (addPostFX / addLocalPostFX
// handles); 3D mesh objects belong to updateRender3D. Properties:
// "effect" string/scalar/".hlsl"/".cube" path (fullscreen effects only,
// id > 0; custom .hlsl passes draw with the user's PSEffect, .cube is a
// 3D-LUT color grade at [strength 0..1, domain 0 auto/1 raw/2
// display], 26077/26078) | "params"
// array | "color" [r,g,b,a] | "blend" string | "band" [minDist, maxDist,
// falloff?] ([] clears) | "visible" bool | "ui" "SCENE"/"UI"/"BOTH" or bool
// (phase enum: SCENE = the pre-tonemap 3D scene chain; UI = coverage-
// masked to the engine UI, gather-effect glow spills past UI edges; BOTH
// (26061) = the scene chain PLUS a coverage-masked UI application - each
// half in its correct domain; booleans stay accepted as the legacy pair
// false=SCENE, true=BOTH) | "uispill" bool (26062: custom-shader lane
// declaration - true = additive/black-preserving, the system feeds the
// coverage-premultiplied capture and the effect's glow spills past UI
// edges; builtins are auto-classified and ignore this)
// | "duration" number or [fadeIn, hold, fadeOut] | "position"
// [x,y,zASL] (the localized volume's center) | "radius" number|[x,y,z] |
// "falloff" scalar | "shape" "sphere"|"cube" | "localsphere" [radius,
// falloff?] (enables the world-space volume mask; [] clears it). Returns
// false for unknown handles, 3D-object handles, unknown properties, or
// invalid values.

// 26055: the affectUI slot's dual acceptance, shared by addPostFX and
// updatePostFX "ui". STRING "SCENE"/"UI"/"BOTH" (case-insensitive; empty =
// slot skipped, leaves the fields untouched) or the legacy BOOL pair.
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
        if (obj.affect_ui) RenderIntegration::kh_ui_driver_rehoist();   // 26063: back on top

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
            // 26055: "SCENE"/"UI"/"BOTH" or the legacy boolean pair
            if (!kh_ui_phase_from_gv(arr[2], obj.affect_ui, obj.ui_only)) return game_value(false);
            if (obj.affect_ui) RenderIntegration::kh_ui_driver_rehoist();   // UI phase demanded
        } else if (prop == "uispill") {
            // 26062: the custom-shader lane declaration - true = the
            // effect is additive/black-preserving, so the system feeds it
            // the coverage-premultiplied capture and its glow spills past
            // UI edges. Builtin gather effects are auto-classified; this
            // bit exists so a custom .hlsl never needs coverage code.
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

// queueVisibility [[x,y,zASL], ...] -> SCALAR accepted count
// Dispatched during the next flush; results readable 1-2 frames later via
// getVisibilityResults (no GPU stall, unlike the synchronous gpuVisibility).
// Queueing again before the flush overwrites the pending batch.
// Returns the queued point count (SCALAR); -1 = invalid input or exception.
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

// getVisibilityResults -> [ageInFrames, [[status, pointDistM, sceneDistM], ...]]
// status: 1 visible, 0 occluded, -1 offscreen/behind camera.
// Results array is empty until the first queued batch completes.
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

// addPostFX [effect, params?, color?, band?, blend?, affectUI?, duration?]
// The effect slot also accepts a ".cube" 3D-LUT path (26077/26078): a
// color grade with tetrahedral interpolation, params = [strength 0..1
// (default 1), domain 0 auto/1 raw/2 display (default 0 - scene lanes
// grade through display space so display-referred .cube looks land as
// authored)], color.rgb = post-grade tint, color.a = pass opacity.
// Notes: runs pre-tonemap, so the engine's eye adaptation applies on top.
// Outline, Pulse, Ssgi and Fogscatter sample the engine depth buffer per pixel; on frames where
// they are active, mode-1 meshes do not write depth (read-only DSV phase).
// affectUI is a phase enum - "SCENE" (default; the pre-tonemap 3D scene
// chain), "UI" (masked per pixel to the engine-UI coverage; gather effects
// - Bloom/Halation/LensFlare/Anamorphic, plus customs declared via the
// "uispill" property - source their glow from UI pixels and spill past
// hard UI edges, systemically, with no shader cooperation; appears one
// frame after creation, the mask learn latency), or "BOTH" (26061: the
// scene chain PLUS a coverage-masked
// UI application - each half in its correct tonemap domain). Legacy
// booleans stay accepted: false = SCENE, true = BOTH.
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
            // 26055: "SCENE"/"UI"/"BOTH" (empty = slot skipped) or the
            // legacy boolean pair false=SCENE / true=BOTH
            if (!kh_ui_phase_from_gv(arr[5], obj.affect_ui, obj.ui_only)) {
                return game_value("affectUI must be \"SCENE\", \"UI\", \"BOTH\", or a boolean");
            }
        }

        // UI phase demanded: start the overlay control driver (state stays
        // dead otherwise - the operator's no-passive-activation rule)
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

// getRenderStats -> ARRAY of [name, value] pairs (cumulative since the
// arming call: the first call arms + zeroes and returns a status pair).
// Skip counters name the reason for any effect flicker: a skipped flush is a
// frame rendered without our draws.
// 26052: also carries instantaneous state sections read at call time -
// draw-list census (obj*), mesh registry totals, texture / user-shader /
// mesh-VB cache census, async visibility pump state, sun-map geometry,
// heightfield origin, far-keep echoes, camera echo and time anchors. All
// are plain reads of standing state; only the cache census takes the
// graphics lock briefly (its keys read -1 if the lock is unavailable
// this call - just call again).
// Debug visual selector for the solid-mesh pixel shaders (see g_dbg_mode
// for the mode catalog). Diagnostic-only, defaults off, survives nothing
// past session destroy.
// 26115 SSGI RESOLUTION SCALE (operator request; the resource-side
// ledger at RenderIntegration::g_khsg_scale explains why this is
// GLOBAL: the gather side buffers are singletons). Multiplier on
// the gather grid - 1 = full res, 0.5 = half (the default), 2 =
// supersampled; clamped to [0.25, 2]. Takes effect on the next
// flush (the buffers recreate). Config, not census: survives
// getRenderStats arming, like setRenderDebug.
static game_value set_ssgi_scale_sqf(game_value_parameter arg) {
    try {
        if (arg.type_enum() != game_data_type::SCALAR) return game_value(false);
        float khss = static_cast<float>(arg);
        if (!(khss == khss)) return game_value(false);   // NaN
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
        // 26281 THE WHITELIST IS WHY 129-133 RETURNED FALSE, AND IT SILENTLY
        // VOIDED THREE FIELD RESULTS. setRenderDebug refuses any mode not named
        // here. 129/130/131/132 were added to the catalog at 26277 and to this
        // list NEVER, so every "I tried 129 and 131 and nothing changed" was a
        // session that ran mode 0 - including the one that retired the signed-
        // count hypothesis. The command returns false on refusal and the return
        // value was not being read, so the failure was invisible from the game.
        //
        // svVolCode IS THE LANE THAT WOULD HAVE CAUGHT IT and it was already in
        // the dump: it reports the arm the SHADER ran, so a requested 131 that
        // reads back as code 1 is a refused mode, in writing. That is exactly
        // what it was built for at 26277 and nobody read it against the request.
        // ADDING A MODE MEANS TWO EDITS - the catalog and this list - and there
        // is no compiler check that they agree.
        // 26334: 178 = witness farthest-plane revert (the slice returns),
        // 179 = depth-bias clamp revert (the render-through returns).
        // 26342: 184 = revert the exact-camera inverse / snap / carried
        // pass camera (restores 26341 and its stationary offset).
        // 26342: 185 = disarm the composite's near-gap reroute (the
        // close-range symmetry A/B; NOT a fix - see the ledger at the arm).
        // 26343: 186 = revert the exact extractor across the registration
        // chain (26342's composite take stays). Bisects the two halves.
        // 26344: 188 = revert the near-gap ramp repair (restores the
        // inverted 1/w comparison); 189 = revert the dark-publish gate.
        // 26346: 190/191 = per-site reverts of the 26343 exact-camera
        // extractor (adopt rebuild / seam anchor), for the third-person
        // fast-move tracking regression.
        // 26348: 192 = disarm PSInjDepth (the footprint's own near-gap
        // encode) = 26347 verbatim; 193 = shader on, viewport floor NOT
        // widened, which bisects the re-encode from the sub-floor band.
        // 26349: 194/195 = the 26348 injection encode (PS + band / PS alone),
        // OPT-IN after the 26348 field regression; 192/193 alias them.
        // 196 = revert the seam trajectory-bound ratio to 4x.
        // 26353: MODE 98 HAS NEVER BEEN REACHABLE. It is catalogued as
        // "injection depth-CLIP instead of clamp", the code for it is live in
        // kh_volume_seam_inject (khv_dbg_vp == 98 selects rast_sun over
        // rast_inject, two states that differ in DepthClipEnable and nothing
        // else), and it has never once been in this list - so every attempt to
        // set it returned false and the arm has never run. Exactly the 26320
        // discovery for 110/111/113/115, in the one slot the sub-1 m axis now
        // needs. Whitelisted here for the first time.
        // 26421: 222 ARMS the exact-order rebase (kh_rebase_vp_exact: the
        // camera folded into the VIEW row before the projection multiply
        // instead of into the finished fp32 product). MODE 0 IS THE BASELINE
        // FOLD - this is opt-in because it was FALSIFIED as the close-range
        // mesh jitter it was built for, twice, in the field. The arithmetic
        // is correct and the number it removes grows with |camera|; it just
        // is not the artifact. Full ledger at KH_ENGINE_VIEW_PRECISION in
        // rendering_integration.hpp, positive control jitRebaseMm /
        // jitRebaseMaxMm. First number spent out of the free range, so 223 is
        // next - and the census still has to be re-run before it is spent,
        // NOW ALSO CHECKING THAT THE MACHINERY A MODE ARMS IS REACHABLE:
        // 116/117/118/125 pass the reader test and arm a chain with no call
        // site (KhEngTry and kh_svs_eng_sweep have no callers), so they
        // return true and do nothing. Adding a mode is STILL two edits with
        // no compiler check between them: this list and the g_dbg_mode
        // catalog.
        // 26422: 224 REVERTS THE THIN-CREVICE SELF-SHADOW BIAS FIX, all three
        // parts as one switch (full ledger at KH_SELF_BIAS_CREVICE in
        // rendering_integration.hpp): the unanchored 10 mm compare-bias floor,
        // the depth guard band inside the LATERAL ortho extent, and the slope
        // scale on the lateral half of selfOfs. Mode 0 carries the fix - it is
        // the answer to a reported artifact, not a speculative change - and
        // 224 restores 26421 exactly. Expect under 224: narrow slots, strap
        // channels and recessed seams in complex meshes reading SUNLIT again
        // where their own lip must occlude them, worst on small objects.
        // Fail direction of the fix itself is self-acne stipple on sun-facing
        // faces; if that ever appears it is the TEXEL term that wants raising,
        // never the floor. Spends the first number of the free range opened at
        // 26421 (223 reserved), so 226 is next.
        // 26423: 225 REVERTS THE GRAZING ESCAPE (KH_SELF_GRAZING_ESCAPE) - a
        // SEPARATE axis from 224 and deliberately its own number. The self
        // term's grazing cut and fade (26400, absent-beats-wrong at harsh
        // incidence) were applied to the whole occlusion unconditionally, so a
        // face at ndl ~0.15 lost its shadow even when the occluder was other
        // geometry 20 cm away rather than its own texel ramp. The escape reads
        // the receiver's own unbiased texel and skips cut and fade only where
        // the stored depth proves the occluder is not this surface. Expect
        // under 225: grazing faces of complex meshes reading sunlit again
        // through geometry that plainly occludes them. Fail direction of the
        // fix is a ~1 texel sliver at silhouettes shading fully instead of
        // fading - the correct verdict, and accepted.
        // 26424 NUMBERING NOTE: 223 IS FREE. A throwaway investigative branch
        // ran a frozen-camera jitter experiment on that number; the branch is
        // gone, nothing from it is in this build, and 223 was therefore never
        // spent in this lineage - the next campaign may take it with no
        // caveat. Its RESULT is kept at KH_FROZEN_CAMERA_TEST in
        // rendering_integration.hpp because it is the evidence behind the
        // outstanding developer request, quoted with the mode label stripped
        // so no future 223 can be misread against it. 224/225/226 were spent
        // while that was still being settled, which cost nothing.
        // 26425: 216, 225 AND 226 ARE DELISTED, and 224 keeps only half its
        // meaning. All three armed knobs inside the old offset/fade/escape
        // self-shadow term, and KH_SELF_RPDB replaced that term outright, so
        // they would now return TRUE and do NOTHING - the 26281 class, and
        // the one this file has paid for most often. They were spent and
        // retired inside a single session, so no script can be relying on
        // them. 224 STAYS because its CPU half is still live (the compare-
        // bias floor and the lateral/depth extent split of 26422); it simply
        // no longer changes anything in the shader. MODE 183 is the A/B for
        // the new term - it kills the sun-map self-shadow outright, which is
        // the only honest one-switch control now that the knobs it used to
        // sit beside are gone.
        // 26426: 227 = THE SUN-DEPTH MAP RENDERS BACK FACES (CullFront)
        // instead of CullNone. Full ledger at KH_SUN_BACKFACE in
        // rendering_integration.hpp. The map has always stored the NEAREST
        // surface, which for any lit receiver is that receiver - so every
        // version of the self term has been asking a surface not to shadow
        // itself, and the bias interval that would clear its own ramp at
        // grazing without walking out of thin geometry is EMPTY. Storing the
        // far side removes the self-compare entirely.
        // IT IS AN ARM AND NOT A DEFAULT for one honest reason: a
        // single-sided panel has no back face and CASTS NOTHING under 227.
        // Whether that matters depends on how the models are built, which is
        // a field question. THE READ: both the grazing z-fighting and the
        // leak gone with casts intact = fold it; fixed but parts stop
        // casting = the models are open and the fix is a two-pass map;
        // neither changes = the artifact is not the self-compare and the
        // axis is wrong.
        // 26427: 228 = DEBUG VISUAL 19, the sun-map probe (full ledger at
        // KH_SUN_MAP_PROBE). PURE GAUGE, mode 0 byte-identical. It paints,
        // at the fragment's own texel: GRAY no map, MAGENTA outside the map,
        // RED the texel was never written, BLUE nothing in front of this
        // fragment, ORANGE a real occluder THE BIAS IS EATING, GREEN a real
        // occluder the bias does not eat. Five builds have changed the
        // self-shadow COMPARE without anything ever reporting what the map
        // CONTAINS; read this before another one is written. ORANGE on the
        // leak = bias-side and bounded; BLUE or RED = the occluder is not in
        // the map and the whole compare-side line of work was aimed at the
        // wrong half of the pipeline; MAGENTA = coverage/ortho fit; GREEN =
        // the term says shadowed while the screen says lit, so the loss is
        // downstream and this function is exonerated. It rides 228 rather
        // than a 0-17 slot because all eighteen are spent and 84 already
        // took visual 18; the mesh fill maps 228 -> dbgCtl.x 19.
        // 26428: 229 = RESTORE THE 26425 RECEIVER-PLANE GRADIENT, which
        // visual 19 falsified (full ledger at KH_SELF_RPDB_FALSIFIED). The
        // gradient is built from the interpolated SHADING normal while the
        // sun map holds RASTERIZED FACETS, and its 1/ndl scaling multiplies
        // that mismatch without bound - so at grazing the nine taps disagreed
        // and diluted an occlusion the centre tap called solid. Expect under
        // 229: the light bleed AND the grazing flicker back together, which
        // is the single-toggle proof they share an author.
        // 26429: 230 = RESTORE THE GEOMETRIC N.L GATE on the receive block
        // (full ledger at KH_SHADING_NORMAL_GATE). The block was gated on
        // i.nrm while the textured path lights through the NORMAL-MAPPED
        // khtxN, so a fragment whose geometry grazed the sun skipped cascade
        // receive, self-shadow and stencil multiply entirely, kept smf 1.0,
        // and was then lit anyway. Expect under 230: the grazing light bleed
        // AND the grazing flicker back TOGETHER - the single-toggle proof
        // they are one mechanism, and the reason every compare-side build
        // from 26422 to 26428 changed a function these fragments never
        // reached.
        // 26430: 231 = RESTORE THE GEOMETRIC N.L VETO inside the self term
        // (full ledger at KH_GEOMETRIC_NL_VETO). SunShadowOcclusionSelf used
        // to return 0 occlusion outright when the GEOMETRIC normal faced away
        // from the sun, while the textured path lights through the MAPPED
        // normal - so a facet the normal map turns sun-facing was lit with no
        // shadow lookup at all. Visual 7 measured it directly: the leaking
        // patch reads WHITE (smf 1) while visual 19 reads GREEN (occluder
        // present) on the same fragment. Expect under 231: the white hole and
        // the light bleed back together.
        // 26431: 232 = RESTORE THE SLOPE CEILING OF 8 on the self-shadow bias
        // (full ledger at KH_SLOPE_CEILING). The 3x3 kernel's worst tap sits
        // 2.12 texels away, so the receiver's own plane climbs 2.12*tan*texel
        // across it - and a bias capped at 8 is arithmetically short of that
        // below ndl ~0.10, which MAKES the grazing z-fighting rather than
        // merely failing to stop it. The cap was 26400's KH_SELF_SLOPE_MAX,
        // built for an offset that MOVED the sample; that offset is gone and a
        // pure depth bias has the opposite cost curve (surface-space leak is
        // bias/tan = 3 texels, CONSTANT at any incidence). Expect under 232:
        // the grazing flicker back, everything else unchanged.
        // 26432: 233 = TAKE THE BIAS SLOPE FROM THE VERTEX NORMAL AGAIN
        // (full ledger at KH_FACET_BIAS_NORMAL). The self-shadow bias scales
        // with tan(theta), and theta was measured on the INTERPOLATED normal
        // while the sun map holds RASTERIZED FACETS - degrees apart on a
        // smoothed FBX, and tan is brutally sensitive near grazing. Default
        // now uses whichever normal grazes HARDER, which can only raise the
        // bias. Expect under 233: the last of the grazing shimmer back.
        // NOTE THE FLOOR before spending another build here: one map texel
        // spans 0.49/ndl mm ACROSS the surface - 24 mm at ndl 0.02 - so the
        // grazing shadow boundary is quantised at centimetre scale by the
        // map's RESOLUTION, which no bias can move.
        // 26436: 234 = PUT THE SLOPE BIAS BACK IN THE SHADER and zero the
        // rasterizer's (full ledger at KH_RASTER_SLOPE_BIAS). The sun-depth
        // map was rendered with NO bias and all slope compensation ran in the
        // per-fragment compare, where it is derived from a NORMAL and scaled
        // by tan - so it varied between neighbouring fragments wherever the
        // normal does, which is what tight geometry means, and a binary
        // compare against a varying threshold is speckle. Hardware
        // SlopeScaledDepthBias is PER TRIANGLE and therefore has no
        // fragment-to-fragment variance at any magnitude - which is why the
        // standard technique does not noise. Expect under 234: the tight-
        // geometry speckle back.
        const bool khd_ok = khd_m == 222 || khd_m == 224 || khd_m == 227 ||
                            khd_m == 228 || khd_m == 229 || khd_m == 230 ||
        // 26438: 235 = RESTORE THE D32 DEPTH-QUANTISATION FLOOR on the
        // self-shadow constant bias (full ledger at KH_WORLD_ULP_FLOOR). The
        // floor must clear fp32 WORLD precision, not depth-buffer precision:
        // i.wpos is quantised at ulp(|wpos|) - 0.488 mm at 7 km - while the
        // caster's stored depth arrives through a different interpolation
        // path, so a bias under one ulp compares noise and lands arbitrarily
        // per triangle. Expect under 235: isolated dark triangles on faces
        // that are almost DIRECTLY lit (not grazing).
                            khd_m == 231 || khd_m == 232 || khd_m == 233 ||
                            khd_m == 234 || khd_m == 235 ||
        // 26440: 236 = KH_ENGCAM_LOCATOR, the engine's OWN camera consumed
        // instead of the one recovered from the RV projection-view matrix
        // (full ledger before the seam inject in rendering_integration.hpp).
        // 26441: THE FIELD A/B PASSED (dump1: Takes 777 engaged, Dx exactly
        // one ulp, Ambig 0, wrong-row never) AND THIS IS NOW THE DEFAULT -
        // mode 0 carries it, and 236 is a KEPT ALIAS selecting what mode 0
        // already does, so 26440-era test scripts keep working and meaning
        // it (the 143/144-family convention).
                            khd_m == 236 ||
        // 26441: 237 = REVERT KH_ENGCAM_LOCATOR to the recovered-camera
        // fold, i.e. 26440 mode-0 behaviour, with the locator's gauge lanes
        // still live for the A/B. The standing check runs in the REVERT
        // direction now: under 237 the close-range jitter must RETURN at
        // high |C| (rotation and translation), or something other than the
        // camera is carrying it and the fold verdict needs re-opening.
        // Camera-path diagnostics (184/186/192/222) stand the default down
        // by themselves - ledger at kh_engcam_armed. Under mode 0 do NOT
        // read jitCamHolds / jitCamStepMm as engine truth: the gauge samples
        // the taken camera (the frozen-camera ledger's side-effect (b));
        // engCamDxMm is the honest lane, captured before the overwrite.
                            khd_m == 237 ||
        // 26443: 223 = REVERT the abrupt-motion stencil finalization
        // (recent-velocity ring + floor-refusal witness gate together;
        // full ledger at the trajectory bound in
        // rendering_integration.hpp). One axis, one number.
                            khd_m == 223 ||
        // 26443b: 238 = REVERT the density-adaptive self-kernel ring
        // (flat 3x3 restored; rides lighting0.y 20; full ledger at
        // KH_SUN_UNION_DENSITY beside the sun-map fit).
                            khd_m == 238 ||
        // 26444: 239 = REVERT the hero sun map (union-only, 26443
        // behaviour verbatim incl. the hash; full ledger at
        // KH_SUN_HERO_MAP).
                            khd_m == 239 ||
        // 26447: 240 = REVERT the engcam absolute agreement bound
        // (per-component 4-ulp form restored; ledger in
        // kh_engcam_agree4).
                            khd_m == 240 ||
        // 26453: 241 = ARM the camera-anchored union window at
        // g_sun_range (default was the caster-anchored fit).
        // 26454: MEANING FLIPPED WITH THE DEFAULT - the cascade ladder
        // exists (KH_SUN_CASCADE in rendering_integration.hpp), the
        // camera-anchored window IS the default union fit now, and 241
        // REVERTS to the caster-anchored fit (26453 mode-0 behaviour
        // verbatim, incl. the camera-free hash and the 250 m
        // eligibility radius). The 143/144-family convention does NOT
        // apply (241 was an arm, not an alias): scripts setting 241
        // now get the OLD behaviour, which is what a revert number
        // means everywhere else in this list.
                            khd_m == 241 ||
        // 26454: 242 = stand the MID cascade band down (8 m @ 4 mm,
        // t26). CPU-side: the band is not rendered, its meta stays
        // zeroed, and the self + cast chains fall through to the next
        // tier. Expect under 242: self shadows and casts between 2 m
        // and 8 m degrade to outer-band texels (16 mm).
                            khd_m == 242 ||
        // 26454: 243 = revert the CAST TIER CHAIN to the union-only
        // compare (lighting0.y 21, written only by the fire's cast CB
        // fill). Self shadows keep the full ladder; expect under 243:
        // near casts blur back to range-priced texels - the 26452
        // report reproduced on demand.
                            khd_m == 243 ||
        // 26454: 244 = stand the OUTER cascade band down (32 m @
        // 16 mm, t27). Same CPU-side shape as 242; expect under 244:
        // 8-32 m falls to the union's range-priced texels.
                            khd_m == 244 ||
        // 26455: 245 = REVERT the cascade filtering (lighting0.y 22,
        // carried by both the mesh and fire CB fills): the cast
        // compare drops back to the hard single bilinear at every
        // tier AND the band self kernels re-collapse to the 26443b
        // single tap. One filtering axis, one number. Expect under
        // 245: the 26454 screenshot reproduced - blocky staircase
        // self shadow on mesh flanks past ~8 m and hard cast edges.
                            khd_m == 245 ||
        // 26456: 246 = REVERT the self range fade (lighting0.y 23,
        // mesh CB): the union self tier drops its edge fade and the
        // hard uv-window cutoff returns. Expect under 246: a mesh
        // walked toward shadowVisibility keeps full-strength self
        // shadow to the window edge, then it vanishes in one step.
                            khd_m == 246 ||
        // 26458: 247 = REVERT the whole stencil mirror counting pass
        // (KH_VOL_MIRROR; CPU-side, byte-identical 26457): no prepass,
        // no b2 patch, no re-issue, mirMeta 0. Expect under 247: the
        // engine stencil shadow on our meshes washes back out inside
        // the sub-near fade window (the last ~metre) exactly as every
        // build before 26458.
                            khd_m == 247 ||
        // 26458: 248 = mirror mask PRODUCED, consumption OFF
        // (lighting0.y 24, mesh fill). The pre-registered A/B split:
        // vmirDraws / vmirMaskFrames keep counting while the composite
        // fade returns to its constant 1.0. Expect under 248: visuals
        // identical to 247 with the production lanes still live.
                            khd_m == 248 ||
        // 26458: 249 = REVERT the sun anchor (KH_SUN_ANCHOR): every
        // sun matrix world-absolute again, sunOrigin (0,0,0), shader
        // subtraction bit-exact no-op. Expect under 249: the grazing
        // motion shimmer on lit faces inside ~10 m returns.
                            khd_m == 249 ||
        // 26458: 250 = REVERT the band stage pool to 2 (the 26416
        // count; creation is take-site-lazy either way, so the change
        // is live mid-session). Expect under 250 at speed:
        // bandStagePoolMiss climbing back toward the 59% class and the
        // shadow-view-distance entry offset returning.
                            khd_m == 250 ||
        // 26458: 251 = DISABLE the layout-change escape. Expect under
        // 251: bandLayoutEscapes pinned 0 and casts landing offset for
        // up to the 4 Hz interval after a cascade re-author (the
        // shadow-view-distance entry).
                            khd_m == 251 ||
        // 26459: 252 = DEBUG VISUAL 20, the mirror-mask probe (dbgCtl.x
        // 20; translated at the mesh fill like 84 -> 18). MAGENTA = mask
        // invalid (production), BLUE = mask shadowed, GREEN = mask lit.
        // Pure gauge; mode 0 byte-identical.
                            khd_m == 252 ||
        // 26459: 253 = REVERT the gradient slack (KH_SUN_GRAD_SLACK,
        // lighting0.y 25): every self tier drops the fwidth depth slack.
        // Expect under 253: the grazing motion z-fight returns in full.
                            khd_m == 253 ||
        // 26461: 254 = REVERT the re-issue's depth clamp (the mirror
        // draws with the engine's own DepthClipEnable). Expect under
        // 254: the uniform sub-near dark returns (the 26460 field
        // capture reproduced) - the near-clip imbalance made visible.
                            khd_m == 254 ||
        // 26464: 255 = REVERT the shadow-view-distance range fade
        // (KH_SUN_RANGE_FADE, CPU sentinel: mirMeta.w stays 0). Expect
        // under 255: casts onto terrain and self shadows POP in/out at
        // the shadow view distance instead of thinning over the last
        // ~15% of it.
                            khd_m == 255 ||
        // 26465: 256 = REVERT the double-precision band snap (fp32 form
        // restored). Expect under 256: the grazing motion shimmer on lit
        // faces returns, hero-worst, with the 239-visible edge line.
                            khd_m == 256 ||
        // 26465: 257 = REVERT the cascade tier blend (lighting0.y 26,
        // BOTH fills): band boundaries return to hard 1/4/16 mm steps.
        // Expect under 257: a visible sharpness line sweeping with the
        // camera at ~2 m and ~8 m and ~32 m from the shadow.
                            khd_m == 257 ||
        // 26467: 259 = REVERT normal-offset sampling (lighting0.y 27,
        // self chain; the front-face-standard primary anti-acne, grazing-
        // scaled 1-3 texels). Expect under 259: the grazing motion
        // shimmer and the 239-class edge line return - THE single
        // diagnostic revert for the offset axis. 258 was freed unspent
        // (the 26466 back-face default was withdrawn pre-field on the
        // operator's 227 bleed data; ledger at the union RSSetState).
                            khd_m == 259 ||
        // 26470: 260 = REVERT footprint tap spread (lighting0.y 28,
        // self chain; fixed +-1 ring restored). Expect under 260: the
        // camera-grazing salt-and-pepper and the crevice noise return.
                            khd_m == 260 ||
        // 26471: 261 = RESTORE the historic 16/3.0 sun raster bias (the
        // low 8/1.0 state is the default). Expect under 261: the
        // separation-dependent contact/crevice dapple returns.
                            khd_m == 261 ||
        // 26472/26474: 262 = contact shortcut OFF - since 26474 an
        // accepted ALIAS OF THE DEFAULT (expect: identical to mode 0).
                            khd_m == 262 ||
        // 26474: 263 = ARM the contact blocker shortcut (lighting0.y 30,
        // opt-in experimental; the 26473 slope-thresholded form). Expect
        // under 263: crevices quieter, grazing misfire dapple returns on
        // sun-sloped faces - the banked margin>2.5 experiment's baseline.
                            khd_m == 263 ||
        // 26476: 258 = REVERT the lighting-block anchor gate (the lum
        // band returns to the instantaneous standing; CPU-only, ledger
        // at kh_probe_std_refresh). Expect under 258: the camera-
        // dependent ~5% mesh brightness flip RETURNS (blkSunLum walking
        // 17<->13.5-class pairs in the trace); at mode 0 one cluster
        // holds with blkAnchorRejects climbing under camera motion.
                            khd_m == 258 ||
        // 26476: 264 = REVERT self-tier bilinear PCF (KH_SELF_PCF,
        // lighting0.y 31, mesh fill; hard single-texel taps restored on
        // all four self tiers). Expect under 264: the lit/shadow
        // terminator dither returns, worst at close caster-receiver
        // contact.
                            khd_m == 264 ||
        // 26476: 265 = REVERT the MinHook install retry ladder (single-
        // attempt historic behavior: the first failed install round
        // latches the session off and reports immediately; ledger at
        // kh_reorder_hook_fail_round).
                            khd_m == 265 ||
        // 26477: 266 = ARM the tid-relax experiment (opt-in; ledger in
        // reorder_pre_draw): track target-context draws from non-clear
        // threads, own draws excluded by flags. Expect under 266 with
        // FSAA off: IF the standdown is a submission-thread change,
        // injections revive (compositeInjections counting, meshes lit,
        // bottom cut gone); if the foreign census instead convicts a
        // deferred context (hookFctxType reading 1), 266 stays inert.
                            // 266: DELISTED 26497 - the tid-relax experiment stripped with the FSAA-off campaign (number never reused)
        // 26477/26478: 267 = the GRADIENT-OFF isolator (lighting0.y 32;
        // the capped 2/2 offset is the 26478 default, so 267 now selects
        // default-offset WITHOUT the clamped RPDB gradient). Expect
        // under 267: the 26477-reported grazing terminator strip returns
        // (that strip is the gradient's whole contribution).
                            khd_m == 267 ||
        // 26478: 268 = FULL revert of the noise axis (lighting0.y 33):
        // offset 6/8 restored, gradient off - the 26477 mode-0 behavior
        // verbatim. Expect under 268: the close-contact noise returns,
        // the grazing strip gone.
                            khd_m == 268 ||
        // 26479: 269 = DEBUG VISUAL 21, the self-tier probe (dbgCtl.x
        // 21; pure gauge, ledger at KhSelfTierProbe). Expect: tier hues
        // (red-orange hero / yellow mid / cyan outer / blue union /
        // magenta none), darker where the self term reads occluded.
                            khd_m == 269 ||
        // 26480: 270 = REVERT the publish rate gate (instant adoption
        // of any in-band coherent step). Expect under 270: the ~6.7%
        // camera-flip returns at ~1/s while panning; mode 0 must hold
        // steady with blkRateHolds ticking at each provoked flip.
                            khd_m == 270 ||
        // 26480: 271 = REVERT the scoped off-thread trigger (tid-gated
        // historic behavior). Expect under 271 with FSAA off: injections
        // return to 0 and the meshes fall back to the mis-encoded
        // fallback (bottom cut); mode 0 with FSAA off must read
        // offthreadTrigs and (if tid was the whole story)
        // compositeInjections climbing together.
                            // 271: DELISTED 26497 - the off-thread trigger + opaque floor mirror stripped with the FSAA-off campaign (number never reused)
        // 26480: 272 = gradient curvature damper OFF (lighting0.y 34;
        // the 26478 undamped gradient). Expect under 272: the curved-
        // contact speckle (the 26479 screenshot) returns to mode-0-of-
        // 26478 level while flat-wall z-fight coverage is unchanged.
                            khd_m == 272 ||
        // 26481: 273 = REVERT the injection-instant block snapshot (the
        // publish returns to the park-time live mirror, 26479 behavior).
        // Expect under 273: the camera flip class returns while panning;
        // mode 0 must hold with blkSnapAdopts ~ flushes.
                            khd_m == 273 ||
        // 26481/26482: 274 = REVERT the whole off-thread funnel (tid-
        // gated historic behavior on blend/DSS/RTS AND map/unmap/
        // updatesubresource). Expect under 274 with FSAA off:
        // offthreadTrigs stops, lightLocValid drops, injections die;
        // MSAA-on must be unchanged under BOTH 0 and 274.
                            // 274: DELISTED 26497 - the off-thread funnel/state/SRV admissions stripped with the FSAA-off campaign (number never reused)
        // 26481: 275 = shader disk cache OFF (every launch recompiles;
        // ledger at compile_shader). Expect under 275: the long spawn
        // returns; mode 0 second-launch spawn must be fast.
                            khd_m == 275 ||
        // 26483: 276 = REVERT the snapshot-anchor consistency gate (any
        // injection-instant snapshot adopts regardless of cluster).
        // Expect under 276: rare wrong-cluster snapshot flips return;
        // mode 0 must read blkSnapRejects > 0 while panning with the
        // picture steady.
                            khd_m == 276 ||
        // 26484: 277 = ADMIT lane-15 == 2 blocks into the arbitration
        // (opt-in experiment; ledger at the locked-confirm gate). Expect
        // under 277 with FSAA off: EITHER the meshes light up (sane
        // FSAA-off block wearing the variant flag - convicted) OR
        // nothing changes and blkModeV/N census names hostile lanes.
                            khd_m == 277 ||
        // 26486: 278 = REVERT snapshot-outright adoption (snapshot
        // publishes return through the 50% band + 500 ms pending).
        // Expect under 278: real weather swings render as freeze-then-
        // snap again (mesh pops ~500 ms after the world); mode 0 must
        // track swings frame-accurately with blkJumpAdopts ~ 0.
                            khd_m == 278 ||
        // 26487: 279 = REVERT continuity selection (the snapshot takes
        // the last upload before the injection again). Expect under
        // 279: the small-gap camera flip class returns; mode 0 must
        // hold zero flips with blkRingPicks climbing.
                            khd_m == 279 ||
        // 26488: 280 = REVERT the measured bias defaults (lighting0.y
        // 35: slope back to 0.35, floor x1, curvature damper back on -
        // the 26487 form verbatim). Expect under 280: the steep-face
        // terminator noise returns at the 26487 level (measured 0.267%
        // vs the new default's 0.093% on the operator's own map).
                            khd_m == 280 ||
        // 26489: 281 = REVERT the off-thread atlas consumption (the
        // SRV-keyed sweep returns to render-thread draws only). Expect
        // under 281 with FSAA off: litGate falls back to 0, receive
        // dies again; MSAA-on unchanged.
                            // 281: DELISTED 26497 - the off-thread atlas-consumption call stripped with the FSAA-off campaign (number never reused)
        // 26489: 282 = 3x3 ring restored on the band tiers
        // (lighting0.y 36; the diamond is the default - measured
        // 0.075% vs 0.093% on the operator's map). Expect under 282:
        // marginally more penumbra body, marginally more steep-face
        // noise, ~2x the tap cost.
                            khd_m == 282 ||
        // 26490: 283 = RESTORE the anchor gate over ring picks (the
        // 26483-26489 behavior). Expect under 283: blkSnapRejects
        // climbs again and the few-percent flip variant returns.
                            khd_m == 283 ||
                            // 26501: 291 = ring pick anchor-band filter OFF
                            // (plain 26490 nearest-to-previous). Expect the
                            // far-range 0.70x both-channel dimming step to
                            // return under 291 during fast orbits.
                            khd_m == 291 ||
                            // 26502: 292 = starvation hold OFF (26501 raw
                            // nearest on no-band). Expect the range-banded
                            // 0.75x dimming at ~7000 m to return under 292.
                            khd_m == 292 ||
                            // 26503: 293 = publish slew OFF (instant commit).
                            // Expect referee snap steps (~7% one-frame) to
                            // return under 293 during cloud transitions.
                            khd_m == 293 ||
                            // 26505: 294 = caster-union window growth OFF
                            // (the pure 26454/26504 camera window). Expect
                            // the giant-caster shadow truncation at ~the
                            // shadowVisibility range to return under 294
                            // (edge crawling with the camera; interior of
                            // an enclosing caster sunlit).
                            khd_m == 294 ||
                            // 26506: 295 = band lit-return restored (the
                            // 26454 cast tier chain verbatim; lighting0.y
                            // 40, fire cast fill only). Expect the ~50 m
                            // lit bubble around the camera to return under
                            // 295 inside/on a giant caster's shadow.
                            khd_m == 295 ||
                            // 26507: 296 = SELF band lit-return restored
                            // (the 26506 twin; lighting0.y 41, mesh fill
                            // only). Expect the camera-tracking SQUARE lit
                            // patch to return under 296 on OUR meshes
                            // receiving a giant caster's shadow; terrain
                            // stays clean (295 owns that chain).
                            khd_m == 296 ||
        // 26491: 284 = tier-proportional floor OFF (lighting0.y 37;
        // the 26490 arm, now reachable). Expect: distance noise returns.
                            khd_m == 284 ||
        // 26491: 285 = diamond ring opt-in (lighting0.y 38). Expect:
        // fewer taps, more penumbra banding - the retired 26489 default.
                            khd_m == 285 ||
        // 26491: 286 = matrix-offset scan OFF (fixed [180] only, the
        // pre-26491 behavior). Expect at 1x: receive dies again.
                            khd_m == 286 ||
        // 26492: 287 = FSAA-toggle wipe OFF (held receive state survives
        // an FSAA change - the pre-26492 behavior). Expect under 287:
        // toggling FSAA mid-session can poison lighting until restart.
                            khd_m == 287 ||
        // 26494: 288 = the 26488 crop-measured bias opt-in (lighting0.y
        // 39: slope 0.8, floor x1.25, damper off). The default is now
        // mode 280's form by operator ranking; 280/35 are aliases.
                            khd_m == 288 ||
        // 26495: 289 = mission-teardown ordering revert, the 26494
        // behavior wholesale: destroy-before-disarm on the success path,
        // and on lock exhaustion the destroy is SKIPPED outright (the
        // deferral never arms; missionResetFails still counts under both
        // arms). Expect under 289 after a lock-exhausted mission end:
        // the next mission inherits the old session's lighting
        // arbitration/atlas/locators armed - old-mission flavors can
        // bias its first ~1 s and worse.
                            khd_m == 289 ||
        // 26496: 290 = FSAA-requirement stand-down OFF - world meshes and
        // the shadow machinery attempt to run at 1x again (the 26495
        // behavior: meshes render, lit ones full-bright albedo,
        // shadowLiveLatches 0, the whole retired campaign's symptom set).
        // The A/B for anything suspected of the stand-down, and the
        // escape hatch if a mesh must exist at 1x regardless of lighting.
                            khd_m == 290 ||
                            khd_m == 98 ||
                            // 26407: 84 = DEBUG VISUAL 18, the sealed-tile
                            // content probe (ledger at KH_BAND_CONTENT_PROBE).
                            // It is a free number carrying a visual because all
                            // eighteen 0-17 slots are spent; the mesh fill maps
                            // it to dbgCtl.x 18. Caught by the census as a
                            // reader with no whitelist entry - the 26353 class
                            // (mode 98 unreachable for its whole life), which is
                            // exactly what that census is for.
                            khd_m == 84 ||
                            // 26415: 88 reverts the overlap-anchored reseal
                            // budget to the border[0] < 10 classification.
                            khd_m == 88 ||
                            // 26418: 99 reverts the zero-sun publish refusal.
                            khd_m == 99 ||
                            // 26358: 199 = the receiver-depth compensation on
                            // the stencil tap (rides dbgCtl.w as 5.0).
                            // 26382: 202 / 203 are the near-gap seam reverts.
                            // 26383: 204 / 205 are the footprint SV_Depth encode
                            // arms, moved off the recycled 192/193/194/195.
                            // Catalog entry at the g_dbg_mode CATALOG COMMENT.
                            // 26411: the cascade coverage fallback is OUT of mode
                            // 0 - falsified with a positive control (visual 18
                            // paints YELLOW on the artifact: the fallback
                            // recovers a tile and that tile is empty). 220 ARMS
                            // it, 217 is an accepted ALIAS of the default. Both
                            // stay whitelisted deliberately: this pair has
                            // flipped twice on field evidence and delisting one
                            // each time is churn that rule 1.18 would then have
                            // to police. Ledger at KH_BAND_COVER_FALLBACK.
                            // 26402: the seam trajectory-bound backstop at 100x is
                            // FOLDED TO DEFAULT, so 219 is now the revert (20x =
                            // 26350 verbatim) and 218 is an accepted ALIAS of the
                            // default - it selected what mode 0 now does, so it is
                            // deliberately NOT delisted below. Adding a mode is
                            // still TWO edits with no compiler check between
                            // them - this list and the g_dbg_mode catalog.
                            // 26405: 221 restores the 0.02 m "still" line at the
                            // seam trajectory bound (ledger at the bound).
                            // 26425: 216 CARVED OUT OF THE RANGE. It armed the fixed
                            // self-shadow offset inside the term KH_SELF_RPDB replaced,
                            // so it now has no reader anywhere and would return TRUE
                            // and do NOTHING. This block range is how 216 stayed
                            // whitelisted without an entry of its own, which is also
                            // why a range is a worse instrument than a list: the 26383
                            // census reads equality comparisons and a range hides
                            // exactly this. Do not recycle 216 (rule 1.18).
                            (khd_m >= 129 && khd_m <= 221 && khd_m != 216) ||   // 26361: 201; 26382-26405: 203..221
                            khd_m == 110 || khd_m == 111 ||   // 26320: the 26249-era volume paints were NEVER whitelisted -
                            khd_m == 113 || khd_m == 115 ||   // "mode 110 was never run" because it never could run
                            khd_m == 118 ||                   // 26320: vol-depth + count composite paint (was inert)
                            // 129-166: 26277-26320 arms (143/144/150/152 default aliases, 145/146/149/153-162 reverts,
                            // 147/151 retired, 135/136 arms; 158-162 = the 26309-26312 reverts; 163 = 26315 content-probe
                            // KILL switch (probes default ON); 164/165 = the 26314 reverts: collapse-override witness
                            // veto / world-latch measured-witness escape; 166 = 26320 receive-kill A/B;
                            // 167/168 = the 26321 cascade-kill / stencil-kill split, 169 = armed alias, 170 = the
                            // 26324 first-bracket copy-latch revert; 171 = the 26325 revert: the COLOUR pass's FOV
                            // take stood down, so the visible box goes back to the cycle latch's scale terms and
                            // rasterises one frame behind its own footprint through a zoom;
                            // 172 = RETIRED to an accepted no-op at 26327 (the referee it reverted is
                            // now opt-in, so there is nothing left to revert); 173 = 26327: ARM the
                            // engine-sniff referee over the seam's depth encode - A/B ONLY, it
                            // regressed the field at 26326 by forcing the k phase on shooting ramps;
                            // 174 = 26330 THE FIX UNDER TEST: the injected footprint is transformed by
                            // the ENGINE'S OWN matrix out of the bound b2 instead of by any pair we
                            // arbitrate - measured identical to what the counting draws consume on
                            // 278/278 samples. Opt-in; mode 0 unchanged. 175 = 26331: the SAME
                            // transform over a CAMERA-RELATIVE position instead of absolute world -
                            // 174 resolved the flicker but landed the footprint at the ground's
                            // depth, which is what a wrong-space position does; 176 = 26332 THE FIX:
                            // our own transform for x/y/w with only the ENGINE'S DEPTH PAIR remapping
                            // z - space-agnostic, since b2's position space proved to be neither
                            // absolute nor camera-relative while its depth pair is exact 370/370.
                            // 26333: 176 IS NOW THE DEFAULT and stays as an accepted alias;
                            // 177 = the REVERT to the arbitrated encode)
                            (khd_m >= 0 && khd_m <= 17) ||   // shader visuals (10-13 are the LADDERS: see the
                                                             // g_dbg_mode catalog. The 'retired' note here was
                                                             // stale from 26189 - corrected 26202; 14 is free)
                            khd_m == 18 ||                    // 26159: dark re-seed refusal OFF (the black-box
                                                              // A/B - restores the 26137 behaviour)
                            khd_m == 19 ||                    // 26159: last-fire clamp with a FRESH epoch on
                                                              // the late fire (= 40 without the epoch
                                                              // restore). The motion-drift experiment.
                            khd_m == 20 ||                    // cast ownership kill switch
                            khd_m == 21 ||                    // cast readiness latch + slab retirement OFF (pristine A/B)
                            khd_m == 24 ||                    // terrain snap off (diagnostic)
                            khd_m == 25 ||                    // cast viewport A/B: live grid (pristine) instead of frozen
                            khd_m == 26 ||                    // lock-settle cast hold off (diagnostic)
                            khd_m == 27 ||                    // 26069: UI coverage debug view (write window)
                            khd_m == 30 ||                    // 26125: matrix-defect repair bypass (pristine A/B;
                                                              // moved off 27 - the 26124 collision)
                            khd_m == 31 ||                    // 26133: ghost tint - per-CYCLE parity (odd cycles lighter)
                            khd_m == 32 ||                    // 26133: ghost tint - per-FIRE index (re-fires lighter)
                            khd_m == 33 ||                    // 26133: depth-snapshot freeze OFF (pointer-freeze A/B)
                            khd_m == 34 ||                    // 26149: FLOOD - every paint pixel forced fully dark
                                                              // (does our paint reach the screen at all?)
                            khd_m == 37 ||                    // 26141: pristine 26140 sun-axis replay (boot
                                                              // reference, cold bar and no-view refusal all off)
                            khd_m == 39 ||                    // 26142: freeze from the ENGINE render view -
                                                              // skips the bridge adoption (person-view pitch ghost)
                            khd_m == 40 ||                    // 26147: clamp to the LAST fire (26146 behaviour;
                                                              // the default is now the FIRST fire)
                            khd_m == 22 ||                    // 26144: LAST-fire clamp OFF - every fire paints
                                                              // (the pre-26144 ghosting behaviour, for the A/B)
                            khd_m == 35 ||                    // 26152: moved-epoch MUTE restored (0.35 strength;
                                                              // retired by default - the clamp replaces it)
                            khd_m == 36 ||                    // 26152: FIRST-fire clamp (the 26147 default;
                                                              // default is now the SRV-bound fire)
                            khd_m == 38 ||                    // 26166 REUSED: relock RECOVERY BAR OFF
                                                              // (revert to the 1e-3-only relock probe). The
                                                              // default also reclaims the remembered view
                                                              // source at the bar its own drop was measured
                                                              // against, forced-drop path only, camera-
                                                              // relative only, two consecutive cycles.
                                                              // viewRelocks has read 0 in every dump back
                                                              // to dump50 because 1e-3 is twenty times
                                                              // tighter than the bar that does the dropping.
                                                              // (Was the 26158 CATCH-UP FIRE - field-
                                                              //  falsified, the shadow vanished outright -
                                                              //  RETIRED at 26166; fireCatchup* stay at 0.)
                            khd_m == 44 ||                    // 26178: ENABLES the falsified 26177
                                                              // 26176 form (0.05 s freshness, always left
                                                              // pending for a later publish to complete).
                                                              // Default frame-anchors the view and seals
                                                              // vcol in the epoch sm came from.
                            khd_m == 43 ||                    // 26172: BRIDGE ARM off - revert to the
                                                              // publish-gated arm (26171 behaviour). The
                                                              // default arms the cast on the bridge view the
                                                              // freeze actually paints with, so a late
                                                              // publish no longer holds the fire past its
                                                              // own read window. This is the flicker fix.
                            khd_m == 42 ||                    // 26170: MOTION-GATED CARRY ON (opt-in). The
                                                              // default is 26165 single paint, verbatim.
                                                              // FALSIFIED at 26170: every 1->2 transition on
                                                              // record carries at camStepM 0.046-0.071, i.e.
                                                              // 27-42 px of double, so the 1 px gate refuses
                                                              // every frame that needs it and opens only on
                                                              // static blocks that never transition. Kept as
                                                              // an opt-in so the effect can be seen.
                            khd_m == 41 ||                    // 26168: ring RUNNING ARGMIN OFF (revert to the
                                                              // bare 16-slot ring). The ring is wiped at the
                                                              // injection and the injection lands BEFORE the
                                                              // view upload on every late cycle on record, so
                                                              // the hunt samples the last sixteen uploads
                                                              // before draw ~74 out of ~2518 a cycle. Default
                                                              // scores every candidate as it arrives.
                            khd_m == 23 ||                    // 26166 REUSED: view-lock DROP HYSTERESIS OFF
                                                              // (revert to the bare bar: a reject streak
                                                              // drops the lock on ANY overshoot, however
                                                              // small). dump99 dropped on 0.0217 against a
                                                              // 0.02 bar and paid 6.387 s dead with the
                                                              // dropped source scoring 0.0174. Default is
                                                              // the 1.5x hysteresis; this restores 26165.
                                                              // (Was the 26165 PRIME PAINT, and before it
                                                              //  the 26155 SRV-KEYED ARM. Both field-
                                                              //  falsified in campaign 27 and both RETIRED
                                                              //  at 26166; firePrimePaints and the SRV-arm
                                                              //  lanes stay published at 0.)
                            khd_m == 59 ||                    // 26193 DL DENSE GATE back to 4 sightings AND
                                                              // a 40 ms absolute gap bar. That bar is tied
                                                              // to frame rate, not to evidence quality, and
                                                              // our own harvest cadence is 47-189 ms - so it
                                                              // threw entries with 6-11 sightings onto a flat
                                                              // 250 ms floor 2.5x their own worst gap.
                                                              // Default gates on sightings alone (>= 6).
                            khd_m == 58 ||                    // 26192 OCCLUDER-IDENTITY TOLERANCE back to
                                                              // 0.25 cell / 2 m floor = 3.125 m on a 12.5 m
                                                              // grid, which calls every kerb, wall, rock and
                                                              // fence 'terrain' and drives the contact band
                                                              // through them. Default is 0.08 cell / 1 m.
                                                              // Too tight fails CLOSED - the terrain bite
                                                              // returns at the ground line and mode 10
                                                              // paints it cyan where the gate suppressed.
                            khd_m == 56 ||                    // 26191 CONTACT BAND back to 25 m. At 25 the
                                                              // clearance ramp is full-strength below 17.5 m
                                                              // of ground clearance, so the 'thin strip at
                                                              // the ground line' the design describes is in
                                                              // fact the WHOLE mesh - which is why mode 11
                                                              // was never black. Default is 6 m: full pull
                                                              // below 4.2 m, zero above 6.0 m.
                            khd_m == 57 ||                    // 26191 DL TTL MULTIPLIER back to 6x the worst
                                                              // observed re-sight gap. 6x a MAXIMUM is not a
                                                              // confidence interval, it tolerates six
                                                              // consecutive worst-case misses: the census
                                                              // read tailMean 558 ms against lifeMean 368,
                                                              // max 908. Default is 2x.
                            khd_m == 55 ||                    // 26188 FAR-ARB NEAR FLOOR OFF - arm the
                                                              // analytic depth clamp on every injection.
                                                              // FIELD-FALSIFIED at 26189 and kept opt-in
                                                              // as a dead end: dump555 arbNearDenied 936
                                                              // of 1024 and the operator called it an
                                                              // objective regression. The floor is not
                                                              // only a far-frame classifier - it keeps a
                                                              // metres-scale depth lie (0.06*d = 3.0 m at
                                                              // 50 m) away from close geometry, against an
                                                              // identity-gate tolerance of 3.125 m that
                                                              // cannot discriminate at that scale.
                            khd_m == 53 ||                    // 26187: DL CADENCE-MARK DECAY off - restore
                                                              // the LIFETIME gap high-water mark. That
                                                              // mark never decayed, so one unlucky
                                                              // sampling gap priced an entry's tail at
                                                              // 6x that gap forever and permanently
                                                              // barred it from the dense fast lane - the
                                                              // 0.25-1 s muzzle-flash/lightning tails.
                            khd_m == 54 ||                    // 26187: BLOCK MIRROR/STANDING COHERENCE
                                                              // off - publish whatever the mirror holds
                                                              // at flush time, certified or not. That is
                                                              // the slight camera-rotation dimming: the
                                                              // mirror is overwritten inside the frame
                                                              // by a pass flavor the arbitration never
                                                              // granted standing, and the publish ships
                                                              // it. Watch blkIncohHolds.
                            khd_m == 52 ||                    // 26186: NEED-BASED PULL off - restore the
                                                              // 26183 form, where the terrain-LOD clamp
                                                              // spends a flat 6% of view distance (capped
                                                              // at 15 m) whether or not anything needs
                                                              // overtaking. That fixed size sweeping past
                                                              // whatever sits near the box surface is the
                                                              // distance-tracking banding. The default
                                                              // spends only the measured gap to the
                                                              // competing depth plus 0.25 m, which also
                                                              // puts most of the mesh on the exact
                                                              // rasterizer depth instead of a recomputed
                                                              // one. (The 26184 smoothing ramp is gone
                                                              // outright - it turned one step into a swept
                                                              // range and multiplied the crossings.)
                            khd_m == 51 ||                    // 26183: EXACT DEPTH PASS-THROUGH off - go
                                                              // back to recomputing the fragment's NDC
                                                              // from the sniffed pair on every pixel.
                                                              // That recomputation lands within a few ulp
                                                              // of 1.0 where fp32 quantises at 5.96e-8,
                                                              // one ulp is 8.6 mm of scene distance, and
                                                              // the 1e-4 anti-shimmer duty is itself only
                                                              // ~1 ulp - so it rounds rather than biases
                                                              // and the box crosses in front of and behind
                                                              // world geometry in a smooth wave as the
                                                              // camera backs away. The default writes the
                                                              // rasterizer's own i.pos.z whenever nothing
                                                              // deliberately moves the depth. Far-keep
                                                              // split and near-gap ramp fragments keep the
                                                              // computed path either way.
                            khd_m == 50 ||                    // 26181: DEFERRED CAPTURE off - revert to the
                                                              // 26180 immediate capture, where a sweep
                                                              // overwrites the slot's sm/border/texture a
                                                              // whole cycle before the view that pairs
                                                              // with them arrives, and the half-built
                                                              // window is refused (the DROPOUT). The
                                                              // default stages the new sm/border instead
                                                              // and commits the whole triple at once when
                                                              // the pairing view lands, so the slot always
                                                              // holds a complete, coherent seal and there
                                                              // is nothing to either offset or refuse.
                            khd_m == 48 ||                    // 26180: HALF-BUILT SEAL REFUSAL off - revert
                                                              // to refusing only the BRIDGE-provisional
                                                              // case (26179 behaviour). The default also
                                                              // refuses a band whose seal has been
                                                              // re-captured but not yet re-completed: its
                                                              // vcol belongs to the epoch before the sm
                                                              // beside it. Absent beats offset. Watch
                                                              // bandPendRefused (expect ~= the old
                                                              // bandPendConsumed) and bandPickNone
                                                              // (expect UNCHANGED - a climb there means
                                                              // the offset was traded for a dropout).
                                                              // 26181: this also disables the deferred
                                                              // capture, so 48 is the full revert to the
                                                              // original OFFSET behaviour and 50 is the
                                                              // intermediate dropout behaviour.
                            khd_m == 49 ||                    // 26180: OCCLUDER IDENTITY GATE off - the
                                                              // analytic depth clamp pulls every
                                                              // contact-band fragment forward again,
                                                              // whatever is standing in front of it.
                                                              // That pull is 6% of view distance capped
                                                              // at 15 m, so it punched our meshes through
                                                              // buildings and trees at range while
                                                              // looking correct up close. The default
                                                              // spends it only where the competing depth
                                                              // IS the terrain. Pair with setRenderDebug
                                                              // 10 to see it: magenta = pull applied,
                                                              // cyan = pull suppressed.
                            khd_m == 45 ||                    // 26179: RECEIVE SEAL COMPLETE-AT-CAPTURE
                                                              // (opt-in A/B). 26177 minus the bridge seal -
                                                              // the half that was never tested alone. When a
                                                              // same-frame ENGINE view exists the seal is
                                                              // complete at capture, so no later publish can
                                                              // overwrite a coherent frame-N sm + frame-N
                                                              // vcol pair with a frame-N+1 view. With no
                                                              // same-frame view the band stays bridge-
                                                              // provisional and the existing consumption
                                                              // guard refuses it: 26176 behaviour verbatim.
                                                              // The BRIDGE is never sealed complete, so the
                                                              // 26177 failure cannot return through this.
                                                              // FIELD-FALSIFIED AT 26180, same report as
                                                              // 26177: drift at any angle under motion,
                                                              // snaps right at rest. bandCompleteSame 0
                                                              // against bandCompleteCross 2957 says EVERY
                                                              // completion crosses the boundary, i.e. our
                                                              // cycle boundary sits mid-engine-frame and
                                                              // the deferred completion IS the pairing.
                                                              // Kept as a documented dead end.
                                                              // Watch bandProvSkips / bandPickNone: a dropout
                                                              // traded for the offset falsifies it too.
                            khd_m == 46 ||                    // 26179: sun-map CASTER CALM ROLL off - restore
                                                              // the unconditional de-mature on a caster-set
                                                              // change. The default holds maturity when the
                                                              // caster set changes under a static sun and a
                                                              // static sky witness, which keeps the far-plane
                                                              // presence test armed and stops the ~0.5 s
                                                              // half-length cut on spawn. New maps, > 3 deg
                                                              // sun steps and drift under a moving sky still
                                                              // de-mature exactly as before.
                            khd_m == 47 ||                    // 26179: COLD RECEIVE HEALTH GATE off - the
                                                              // receive un-gates on the 2 s warm-up alone
                                                              // (26178 behaviour). The default also requires
                                                              // a locked view source, a fresh publish stream
                                                              // and a converged derived sun, held for 500 ms,
                                                              // and drops every cold-era seal at the rise.
                                                              // For the foreign-receive cold start.
                            khd_m == 87 ||                    // 26233 THE FEATURE REVERT, AND THE ONLY THING
                                                              // IT IS FOR IS THE A/B. Restores the 26231
                                                              // whitelists exactly: the injection, the
                                                              // snapshot demand gate and the multiply all go
                                                              // back to being mode-driven, so 0 vs 87 is
                                                              // "unconditional" against "off" with one
                                                              // variable. It is NOT a product safety net and
                                                              // must not be reasoned about as one - the
                                                              // shipped fallback is kh_svs_sten_ready(),
                                                              // gauged by svStenRejCold and svStenRejDims.
                                                              // Removal date: the scaffolding strip.
                            khd_m == 109 ||                   // 26249 THE VOLUME TRANSPORT. Reads the stencil
                                                              // COUNT out of the engine's volume buffer through
                                                              // our own copy (t23/t24) instead of inferring it
                                                              // from post/pre on the engine's screen-space mask,
                                                              // AND disables the priming pass - which is the
                                                              // cascade cut's directly-observed cause, not a
                                                              // suspect. The footprint witness becomes exact: the
                                                              // copy's DEPTH plane against the window z the
                                                              // injection would have written, so the 254/255
                                                              // sentinel and the pre <= 0.02 guard have nothing
                                                              // left to do. 0 vs 109 is one variable in one
                                                              // session: the cascade cut must be GONE under 109
                                                              // and the unit shadow must be visually identical.
                                                              // If the shadow differs at all, the stencil read is
                                                              // wrong and the default stays where it is.
                                                              // Read svStenSrc / svVolSamples / svVolCopyFails
                                                              // before believing anything you see.
                            khd_m == 110 ||                   // 26249 PAINT THE STENCIL COUNT - THE WRAP
                                                              // INSTRUMENT, and the one thing that must be looked
                                                              // at before 26250 flips the default. Counting is
                                                              // IncWrap on front faces and DecWrap on back faces,
                                                              // so a back face reached without a front-face
                                                              // increment wraps 0 -> 255 and `count != 0` calls a
                                                              // LIT pixel shadowed. Black = 0 (lit), blue->cyan =
                                                              // 1..8, MAGENTA = 128 or above. ANY MAGENTA MEANS
                                                              // != 0 IS THE WRONG TEST and the count needs a
                                                              // signed reading before this transport ships. The
                                                              // 26231 pixel history saw counts of 0, 4 and 5 at
                                                              // three pixels; three pixels is not a survey.
                            khd_m == 111 ||                   // 26249 PAINT THE FOOTPRINT RESIDUAL as a ladder,
                                                              // same legend shape as the mode-13 transmittance
                                                              // ladder: gray = nothing reprojected, black <=1e-6,
                                                              // navy 1e-5, blue 1e-4, cyan 1e-3, green 1e-2,
                                                              // yellow 1e-1, red above. The shipped tolerance is
                                                              // 1e-4 - the BLUE rung. Our surface painting green
                                                              // or warmer means the tolerance is too tight and
                                                              // coverage is being thrown away; off-surface
                                                              // background painting blue means it is too loose
                                                              // and the witness is not witnessing. A scalar is
                                                              // the wrong shape for a perspective-dependent
                                                              // residual, so this is chosen from a screenshot the
                                                              // way mode 78 settled the ratio arithmetic.
                            khd_m == 128 ||                   // 26276 THE PROJECTION HALF. 127 shared the
                                                              // VIEW and proved it, and svReprojPxMean
                                                              // did not move - so the residual offset is
                                                              // the PROJECTION, which is also where
                                                              // injNear's 0.07<->0.77 oscillation lives.
                                                              // 128 shares the boundary pair too, still
                                                              // subject to the near-class gate.
                            khd_m == 127 ||                   // 26275 ONE PUBLICATION FOR BOTH PASSES.
                                                              // The injection and the composite read
                                                              // publications ~289 apart out of ~534 per
                                                              // frame (svPubOrdMean), which is the whole
                                                              // remaining defect: raster pays it as the
                                                              // edge slice, reprojection as the lag, 107
                                                              // as a lagging box. 127 makes both take the
                                                              // frame's clear-time sample, so they are
                                                              // byte-identical by construction.
                                                              // Acceptance: svPubOrdMean -> 0.
                            khd_m == 126 ||                   // 26273 RESTORE REPROJECTION.
                                                              // The raster lookup is the DEFAULT now -
                                                              // mode 104's behaviour, which closed the
                                                              // lag completely in the field after ten
                                                              // builds of injection-side work changed
                                                              // nothing. 126 puts the reprojected read
                                                              // back so 26244's truth table can be
                                                              // re-run under the VOLUME transport it was
                                                              // never measured against.
                            khd_m == 125 ||                   // 26270 PROVABLY CORRECT OR ABSENT.
                                                              // Arms the engine view like 116 AND
                                                              // suppresses the injection entirely on
                                                              // frames where no candidate basis
                                                              // validates - zero-area triangles, no
                                                              // depth written. Depth we never wrote
                                                              // cannot be tested wrongly, so the
                                                              // stencil artifact stops being reduced
                                                              // and starts being impossible. Cost:
                                                              // no stencil shadow on those frames.
                                                              // Third person at 26269 armed 825/825,
                                                              // so it suppresses NOTHING there.
                            khd_m == 124 ||                   // 26268 DISABLE THE AGREEMENT BOUND.
                                                              // The engine-view path keeps its
                                                              // structural gates but stops refusing a
                                                              // basis that disagrees with the historic
                                                              // transform by more than a quarter screen.
                                                              // This is the A/B for the first-person
                                                              // weapon-fire artifact: if 124 brings it
                                                              // back and 118 does not have it, the bound
                                                              // is what fixed it.
                            khd_m == 121 || khd_m == 122 || khd_m == 123 ||
                                                              // 26267 FORCE THE COPY TO READ WIDE-
                                                              // ENUMERATION CANDIDATE 1/2/3 instead of
                                                              // the latched offset. Byte 0 is a SHARED
                                                              // pool offset (engBindRejects 0 with
                                                              // engVerifyFail 44 proves the binding
                                                              // cannot separate its two writers); if a
                                                              // family-private copy of the view block
                                                              // exists further into the 64 KB pool,
                                                              // engVerifyFail collapses on the right
                                                              // candidate. Read engWideOffK / engWidePassK
                                                              // FIRST - a candidate that is not in the
                                                              // table makes these no-ops.
                            khd_m == 120 ||                   // 26265 STATEBACKUP CONSTANT-BUFFER OFFSET REVERT.
                                                              // Restores VS CBs through the plain
                                                              // (non-1.1) setter, which drops the
                                                              // engine's per-slot firstConstant -
                                                              // the pre-26265 behaviour. Kept as
                                                              // its own A/B because it is a
                                                              // SYSTEMIC change riding a build whose
                                                              // headline is the engine-view locator,
                                                              // and two variables in one field
                                                              // session is the campaign-35 mistake.
                            khd_m == 119 ||                   // 26264 REVERT TO THE PRE-26264 MASK TRANSPORT.
                                                              // The volume buffer is now the DEFAULT (mode 0 is
                                                              // what 109 was): the stencil count comes from our
                                                              // own CopyResource of the engine's volume buffer,
                                                              // and the priming pass is gone - svPrimeDraws 0 -
                                                              // which IS the cascade-cut fix. 119 puts the mask
                                                              // transport and priming back, cascade cut
                                                              // included, as a one-key A/B. The mask path is
                                                              // deliberately NOT deleted: svVolArmMax 2-3 means
                                                              // the mask identity compare is still load-bearing
                                                              // as the BRACKET'S CLOCK even though the mask is
                                                              // no longer the transport.
                            khd_m == 116 ||                   // 26262 INJECTION ON THE ENGINE'S OWN VIEW.
                                                              // The injection stops adopting frame_view - a
                                                              // stream written 589x/frame that alternates
                                                              // between this frame's camera and the next - and
                                                              // rasterizes through the rotation copied GPU-side
                                                              // out of the engine's own view constant buffer,
                                                              // same frame. THE PREDICTION: svReprojPxMean
                                                              // collapses from ~95 px toward 0 and the stencil
                                                              // shadow stops trailing the camera. Selects the
                                                              // volume transport too, so the only variable on
                                                              // screen is the view source.
                            khd_m == 117 ||                   // 26262 VISIBLE MESH ON THE ENGINE'S OWN VIEW,
                                                              // injection unchanged. Capture 6 showed our
                                                              // COMPOSITE 16 deg out in pitch from the engine
                                                              // during a fast vertical flick while that frame's
                                                              // engine volume and colour passes were
                                                              // byte-identical - so the composite is a second
                                                              // patient, not a yardstick. NOTE mode 107 was
                                                              // never this test: 107 draws the mesh with the
                                                              // INJECTION's stale matrix, the worst available
                                                              // value. Judge whether the box sits BETTER on the
                                                              // ground under 117 than under 0.
                            khd_m == 118 ||                   // 26262 BOTH. If 116 and 117 each hold, this is
                                                              // the shipping candidate: both passes on the
                                                              // engine's view means they agree with each other
                                                              // and with the volumes by construction, and
                                                              // reprojection becomes unnecessary rather than
                                                              // merely better - which retires the epoch ring,
                                                              // the footprint witness, the sentinel, the
                                                              // tolerance, modes 111-115 and the interlacing.
                            khd_m == 115 ||                   // 26253 VERDICT DISAGREEMENT PAINT - the one view
                                                              // that localises the interlacing instead of
                                                              // inferring it. Reads the stencil at BOTH candidate
                                                              // positions (reprojected and raster) and paints
                                                              // their RELATIONSHIP: black both lit, white both
                                                              // shadowed - those pixels cannot interlace whatever
                                                              // the guard decides. Dark red / dark blue = the two
                                                              // disagree but the guard did not fire. ORANGE = they
                                                              // disagree AND the guard fired, losing shadow;
                                                              // CYAN = disagree, guard fired, adding shadow.
                                                              // Orange+cyan IS the artifact's support set. A
                                                              // coherent band ~svReprojPxMean wide means the
                                                              // displaced-edge mechanism and a spatial fallback
                                                              // has neighbours to work from; salt-and-pepper means
                                                              // no spatial fallback can help; sparse-but-still-
                                                              // visible means the guard is not the cause and the
                                                              // view lag is. Diagnostic only.
                            khd_m == 114 ||                   // 26252 FOOTPRINT TOLERANCE BACK TO 1e-4 (was the
                                                              // 26249 value). The default is 3.16e-4, chosen from
                                                              // mode 111's ladder: our surface reads <= 1e-4
                                                              // (blue), the halo reads >= 1e-3 (green/yellow), and
                                                              // the 1e-4..1e-3 rung is EMPTY - so 26249 had the
                                                              // threshold hard against the top edge of the
                                                              // on-footprint population instead of in the gap
                                                              // between the two. Body pixels straddled it,
                                                              // neighbouring pixels chose different lookups, and
                                                              // because the two lookups give different verdicts
                                                              // that paints as an INTERLACED shadow edge. This
                                                              // mode restores the straddle so "the interlacing
                                                              // went away" and "the threshold moved" stay
                                                              // separable. Does NOT address the underlying view
                                                              // lag (svReprojPxMean 21-43 px).
                            khd_m == 113 ||                   // 26251 RAW UNGUARDED COUNT PAINT - 110's A/B
                                                              // partner. Reads the stencil at the reprojected
                                                              // texel with NO footprint test, which is what 110
                                                              // did at 26249/26250 by mistake. 110 vs 113 shows
                                                              // exactly what the witness removes: under 113 the
                                                              // caster is OUTLINED against our surface on
                                                              // rotation (the disocclusion band carrying the
                                                              // caster's verdict at full contrast); under 110
                                                              // those pixels paint orange/brown as guard-fired.
                                                              // The band's absence from 109's shading is then an
                                                              // observation, not an inference.
                            khd_m == 112 ||                   // 26249 FOOTPRINT TEST OFF under the volume
                                                              // transport (tolerance zeroed): accept every
                                                              // reprojected sample. Separates the witness's cost
                                                              // in coverage from its benefit in correctness with
                                                              // one variable. Expect the 26248 halo's analogue to
                                                              // return; if it does NOT, the witness is not the
                                                              // thing holding it back and the reprojection band
                                                              // is smaller than svReprojPxMean suggests.
                            khd_m == 108 ||                   // 26248 PRIME WITH 1.0 + DISOCCLUSION GUARD OFF -
                                                              // the 26247 pair, restored together. The default
                                                              // primes with 254/255 so a primed texel is
                                                              // distinguishable from lit background (both were
                                                              // 1.0 before, which is why the 26245 guard could
                                                              // not work), and the guard refuses a reprojected
                                                              // sample that is not on the footprint. Brings the
                                                              // halo back. Note pre reads 254/255 on our
                                                              // footprint now, so modes 74 and 78 paint it one
                                                              // step below white.
                            khd_m == 107 ||                   // 26247 VISIBLE MESH IN ENGINE SPACE. Draws the
                                                              // composite mesh with the transform the INJECTION
                                                              // uses - which the RenderDoc capture proves is the
                                                              // engine's own camera basis and projection to six
                                                              // decimals. svReprojPxMean 41 px says the visible
                                                              // draw is that far from it, and one band explains
                                                              // the strip, the cascade cut and the halo. PAIR
                                                              // WITH MODE 104 conceptually - under 107 the
                                                              // reprojection is correcting a band that no
                                                              // longer exists - but 107 alone is the first look.
                                                              // If the mesh visibly lags the camera instead, the
                                                              // band is real and the fix belongs on the priming
                                                              // side. Read svEngVpTakes.
                            khd_m == 106 ||                   // 26246 DISOCCLUSION GUARD ON (opt-in). Defaulted
                                                              // OFF because it did not move the halo: the
                                                              // reprojected lookup is landing INSIDE our
                                                              // footprint and the value there is still wrong,
                                                              // so the footprint is not the fault. Costs one
                                                              // extra Load per mesh pixel, so it does not ride
                                                              // on the default path unproven.
                            khd_m == 105 ||                   // 26246 RETIRED TO A NO-OP (the guard is off by
                                                              // default now - see mode 106). Was: guard OFF
                                                              // exactly: reprojection with no validity test on
                                                              // the reprojected texel. Brings back the
                                                              // scattered halo around the caster, which is the
                                                              // A/B for the guard. 0 vs 105 vs 104 is the full
                                                              // ladder: guarded reprojection, raw reprojection,
                                                              // no reprojection.
                            khd_m == 104 ||                   // 26244 REPROJECTION OFF (raster lookup). The
                                                              // default reprojects again: the 26242/26243
                                                              // truth table shows the on-box stencil cut clears
                                                              // ONLY with adoption AND reprojection both on
                                                              // (mode 100 at 26242), and the halo that came
                                                              // with it is now attributed to feeding the shader
                                                              // a REBASED matrix, which forced a per-pixel fp32
                                                              // camera subtraction at world magnitudes. The ring
                                                              // stores the ABSOLUTE transform now and the
                                                              // subtraction is gone. If the halo survives that,
                                                              // the aliasing is inherent and 104 is the answer.
                            khd_m == 103 ||                   // 26244 RETIRED TO A NO-OP (reprojection is the
                                                              // default now - see mode 104). Was: opt-in.
                                                              // the default: under mode 101 the scattered halo
                                                              // around the caster disappeared, so reprojection
                                                              // causes it. Enabled three times now (26234,
                                                              // 26236, 26242), a caster ghost every time, the
                                                              // strip never once moved.
                            khd_m == 102 ||                   // 26243 INJECTION VIEW-ADOPTION OFF (opt-in).
                                                              // Backed out of the default: under mode 100 the
                                                              // on-box stencil cut disappeared, so the
                                                              // injection DOES need the visible draw's view and
                                                              // 26224 was right. Pair with svSwingPxMean, which
                                                              // reads a tautological 0 whenever adoption is off.
                            khd_m == 101 ||                   // 26242 RETIRED TO A NO-OP (its revert is the
                                                              // default now - see mode 103). Was: HALF B OFF -
                                                              // the mesh reads the mask at
                                                              // its own raster position instead of reprojecting
                                                              // through the injection's transform. Restores the
                                                              // on-box stencil strip. Pair with svReprojPxMean.
                            khd_m == 100 ||                   // 26243 RETIRED TO A NO-OP (its revert is the
                                                              // default now - see mode 102). IT IS ALSO THE
                                                              // MODE THAT BACKED 26242 OUT: under 100 the
                                                              // on-box stencil cut vanished in a
                                                              // single-session, single-variable A/B.
                                                              // Was: HALF A OFF - restores the 26224 fresh-view
                                                              // adoption in the injection, which puts our
                                                              // footprint one sim-frame ahead of the volume
                                                              // buffer again and brings back the CASCADE CUT
                                                              // (the box ghost eating world shadow). Pair with
                                                              // svSwingPxMean and svInjViewAdopts.
                                                              // 0 vs 100 vs 101 is a three-way A/B, one
                                                              // variable each. Neither half works alone: 83
                                                              // (= half A) fixed the cascade cut and left the
                                                              // stencil strip; reprojection alone (26234/26236)
                                                              // only added a ghost, because it was aligning the
                                                              // lookup to a footprint that was itself misplaced.
                            khd_m == 98 ||                    // 26239 INJECTION DEPTH-CLIP REVERT. Restores
                                                              // rast_sun (DepthClipEnable TRUE) for the
                                                              // injection instead of the new rast_inject
                                                              // (FALSE, matching the visible draw). THE PIXEL
                                                              // HISTORY CONVICTED THIS: at a strip pixel our
                                                              // injection draw does not appear in the volume
                                                              // depth history at all, while it passes at a
                                                              // correct pixel and a lit one. Clipped fragments
                                                              // vanish from the injection and are CLAMPED into
                                                              // the visible draw, so the box is there and its
                                                              // depth is not. Read svInjRsClamp / svInjRsClip
                                                              // and compare svPrimePixels: it must FALL under
                                                              // 98 if the clip was eating coverage.
                            khd_m == 97 ||                    // 26238 MASK CLAMP BOUND FORCED to the mask's
                                                              // own dimensions. fxMeta.zw is filled TWICE per
                                                              // object and the second fill uses the occlusion
                                                              // guard snapshot's size, not the mask's - 26221
                                                              // added the unconditional fill and never removed
                                                              // the conditional one. Pair with fxDimMismatch:
                                                              // if that reads 0 this mode is a no-op and the
                                                              // double fill is only untidy.
                            khd_m == 96 ||                    // 26238 SPLIT ARM B: injection OFF, snapshots
                                                              // and multiply ON. The mesh multiplies by a mask
                                                              // our depth never reached. If the missing-shadow
                                                              // sliver survives, the mask content is wrong for
                                                              // reasons unrelated to our injection.
                            khd_m == 95 ||                    // 26238 SPLIT ARM A: injection and snapshots ON,
                                                              // MULTIPLY OFF. If the box-edge sliver survives,
                                                              // it comes from our depth entering the volume
                                                              // buffer and NOT from the shading term - i.e.
                                                              // the two slivers are two faults. Neither 95 nor
                                                              // 96 paints, so the geometry stays legible.
                            khd_m == 94 ||                    // 26237 STENCIL REPROJECTION ON (OPT-IN, and it
                                                              // does NOT fix the silhouette strip - it is kept
                                                              // only because the machinery is what measured
                                                              // the strip's cause OUT). svReprojPx reads 0.139
                                                              // px between the two views on a settled frame,
                                                              // so there is no misregistration to correct;
                                                              // enabling this adds a ghost around the caster
                                                              // and changes the strip not at all. Modes 90, 91
                                                              // and 92 are all retired no-ops now: the default
                                                              // IS the raster lookup.
                            khd_m == 93 ||                    // 26236 DARK-STANDING HARD ESCAPE OFF. Restores
                                                              // the pre-26236 behaviour where a sun-zero
                                                              // lighting standing under a live static sun can
                                                              // hold indefinitely - i.e. it restores the
                                                              // black-box-on-cold-start fault. Pair with
                                                              // blkDarkForceAdopts (stops climbing) and
                                                              // blkStickyRejects (resumes climbing without
                                                              // bound). Diagnostic only.
                            khd_m == 92 ||                    // 26236 STENCIL REPROJECTION OFF. The default is
                                                              // now EPOCH-MATCHED reprojection: svPostAgeMax
                                                              // measured 1, so the mask the mesh reads is one
                                                              // frame older than the mesh, and the lookup uses
                                                              // the transform stamped with the SNAPSHOT's
                                                              // epoch. 92 falls back to the raster position,
                                                              // which restores the silhouette strip. That is
                                                              // the A/B for this build. Read
                                                              // svReprojEpochHits / svReprojEpochMiss.
                            khd_m == 91 ||                    // 26235 STENCIL REPROJECTION ON - RETIRED (the
                                                              // opt-in it enabled is the default now). No-op.
                                                              // Was: 26235 opt-in. The
                                                              // default no longer reprojects: 0 and 90 carried
                                                              // the same strip, so the lookup position was not
                                                              // the cause, and 0 additionally ghosted around
                                                              // the caster - reprojecting against THIS frame's
                                                              // injection matrix adds error if the snapshot
                                                              // being read is older than that matrix. Read
                                                              // svPostAgeMax before trusting this mode.
                            khd_m == 90 ||                    // 26234 STENCIL REPROJECTION REVERT - RETIRED
                                                              // as a revert (the default it reverted is gone)
                                                              // and kept only so old scripts still validate.
                                                              // Behaves identically to 0.
                                                              // Was: loads the
                                                              // mask at the mesh's OWN raster position again
                                                              // instead of the pixel it occupied under the
                                                              // injection's view. Restores the ~50 px
                                                              // misregistered strip at every silhouette edge
                                                              // under camera rotation, which is the whole
                                                              // A/B. Pair with svReprojPxMax: under 90 the
                                                              // gauge still measures the delta, so the
                                                              // number and the artifact can be read in one
                                                              // session.
                            khd_m == 89 ||                    // 26232 INJECTION TESSELLATION REVERT. Leaves
                                                              // HS/DS inherited from the engine's volume-pass
                                                              // bind instead of nulling them, which is what
                                                              // the injector did for its whole life while
                                                              // every one of its nine sibling passes nulled
                                                              // them. Expect NO visual difference; this is
                                                              // parity, like 80-83, and all four of those
                                                              // held as parity and caused nothing visible.
                                                              // Read svInjTessNulls / svInjTessInherit.
                            khd_m == 80 ||                    // 26221 INJECTION VIEWPORT REVERT. Restores
                                                              // MinDepth 0 / MaxDepth 1 in
                                                              // kh_volume_seam_inject instead of the
                                                              // engine's own 0.011 / 0.999. Paints the
                                                              // isolated stencil term exactly as 73, so
                                                              // 73 vs 80 is one variable with one visual.
                                                              // Read svInjVpLo/Hi and svInjVpReverts.
                            khd_m == 86 ||                    // 26230 MASK-PRIMING REVERT. Stops writing lit
                                                              // into the engine mask over our footprint, so
                                                              // pre carries the BACKGROUND's cascade verdict
                                                              // again and KhStenRatio's pre <= 0.02 guard
                                                              // discards the stencil term wherever the
                                                              // terrain behind our mesh is fully shadowed -
                                                              // the blue band. Paints as 73. A/B 73 vs 86 in
                                                              // mode 78: the band is present under 86 only.
                            khd_m == 83 ||                    // 26242 RETIRED TO A NO-OP: this revert IS the
                                                              // default now (see mode 100). It is what proved
                                                              // half A in the field - under 83 the cascade cut
                                                              // disappeared while the on-box stencil strip
                                                              // remained, which is what split the two faults.
                                                              // Kept whitelisted so old scripts validate.
                                                              // Was: 26224 INJECTION VIEW-ADOPTION REVERT. Keeps
                                                              // g_ro.cycle_pv instead of the same-frame
                                                              // adopted view, so the injected box and the
                                                              // visible box are transformed by different
                                                              // view matrices again - the pre-26224 path,
                                                              // and the one that manufactures the duplicate
                                                              // opposite-side silhouette. Paints as 73.
                            khd_m == 82 ||                    // 26222 INJECTION RASTERIZER REVERT. Inherits
                                                              // the engine's volume-pass rasterizer instead
                                                              // of rast_sun, so depth is clamped not clipped,
                                                              // carries whatever bias the engine had, and is
                                                              // culled however the engine culls. The
                                                              // pre-26222 path. Paints as 73. Read
                                                              // svInjRsInherit.
                            khd_m == 81 ||                    // 26221 INJECTION REBASE REVERT. Keeps the
                                                              // absolute (un-rebased) viewProj and leaves
                                                              // center_rel unarmed - the pre-26221 path.
                                                              // Also paints as 73. Read svInjAbs: under 81
                                                              // it takes over from svInjRebases one for one.
                            khd_m == 79 ||                    // 26217 SUBTRACTIVE STENCIL FORM. Paints
                                                              // 1 - saturate(pre - post) instead of the
                                                              // guarded division. Continuous, so no step
                                                              // at the edge of a cascade shadow, but it
                                                              // under-darkens a full stencil shadow to
                                                              // 1 - pre rather than 0. A/B against 73:
                                                              // the fringe decides.
                            khd_m == 78 ||                    // 26216 RATIO INPUTS PER CHANNEL. R = post
                                                              // (live mask), G = pre (snapshot), B = the
                                                              // sten ratio. THE instrument for 26215's
                                                              // falsified premise: at a building-shadow
                                                              // pixel over the mesh, R == G means the
                                                              // cascade term is identical in both and the
                                                              // ratio must cancel (arithmetic fault);
                                                              // R != G means the copy lands at the wrong
                                                              // point relative to the engine's cascade
                                                              // writes (premise fault). Yellow = correct
                                                              // cancellation; red/green fringes on
                                                              // building edges are the residual.
                            khd_m == 77 ||                    // 26215 MASK-ADOPTION / WITH-RTV COUPLING
                                                              // RESTORED - re-enact the 26214 fault on
                                                              // every mask adoption (the old release path
                                                              // did both jobs). Pair with svRtvSetWipes,
                                                              // which must read 0 under the default.
                            khd_m == 76 ||                    // 26218 REPURPOSED: LIVE-POST A/B. Reads post
                                                              // from the live mask instead of the bracketed
                                                              // snapshot - i.e. the 26217 configuration,
                                                              // kept because it is the A/B that documents
                                                              // the fault. Under 76 our own mask_cast_engine
                                                              // paint lands between the resolve and our
                                                              // draw, so the box's OWN cast shadow comes
                                                              // back as a counterfeit stencil term; under
                                                              // every other selector it cannot.
                                                              // (Was the 26215 in-hook copy option, retired
                                                              // with the bracket - the post half must fire
                                                              // AFTER the resolve draw and cannot be issued
                                                              // from the OM hook at all.)
                            khd_m == 75 ||                    // 26215 THE FEATURE. Multiply the isolated
                                                              // stencil term into the shading path. Band
                                                              // receive (cascades) is untouched, so
                                                              // terrain and building shadows must look
                                                              // exactly as they do at mode 0 while unit
                                                              // shadows now land on the mesh.
                            khd_m == 74 ||                    // 26215 PAINT THE PRE SNAPSHOT ALONE. The
                                                              // copy's own liveness: black everywhere
                                                              // means the CopyResource never landed and
                                                              // sten is pinned to 1, which mode 73 alone
                                                              // could not tell apart from a wrong ratio.
                            khd_m == 73 ||                    // 26215 PAINT THE ISOLATED STENCIL TERM
                                                              // (post/pre). ACCEPTANCE TEST: it must show
                                                              // the soldier's shadow and NOTHING else -
                                                              // no power lines, no building edges, no
                                                              // terrain. Those belong to the cascade half
                                                              // of the mask, which this discards.
                            khd_m == 72 ||                    // 26214 PER-FRAME WITH-RTV SET RESTORED -
                                                              // 26213 behaviour, which measured
                                                              // svOrdRtvFirst 0 against svOrdOnlyFirst
                                                              // 1863: the set was wiped between the
                                                              // prepass and the volume pass every
                                                              // frame, so no seam could ever fire.
                            khd_m == 71 ||                    // 26212 ENGINE SHADOW-MASK PAINT. Arms the
                                                              // seam depth injection AND paints the
                                                              // engine's screen-space shadow mask onto
                                                              // the mesh instead of shading it. If the
                                                              // box shows the SOLDIER's shadow, sharp
                                                              // and tracking him, the injected depth
                                                              // reached the stencil volume test. If it
                                                              // shows shadowing belonging to the ground
                                                              // BEHIND the box, it did not. Pair with
                                                              // svMaskAdopts + svMaskBinds.
                            khd_m == 70 ||                    // 26211 IN-HOOK SEAM INJECTION RESTORED -
                                                              // 26210 behaviour: draw from inside the
                                                              // OM hook instead of deferring to the
                                                              // next draw. Runs in-game either way;
                                                              // the difference is whether a RenderDoc
                                                              // capture contains the draw.
                            khd_m == 69 ||                    // 26209 SEAM ADJACENCY RULE RESTORED -
                                                              // back to 26208's "immediately previous
                                                              // bind" test, which measured a 47% miss
                                                              // rate against a boundary present every
                                                              // frame. Pair with svSeamAdj.
                            khd_m == 68 ||                    // 26204 SHADOW-VOLUME SEAM DEPTH INJECTION
                                                              // ON (opt-in; default is census only).
                                                              // Renders our meshes DEPTH-ONLY into the
                                                              // engine's scene depth buffer at the
                                                              // prepass -> stencil-volume seam, so the
                                                              // volume counting sees our surface. Pair
                                                              // with svInjects + svInjectDraws: both
                                                              // must climb. Nothing reads the shadow
                                                              // mask yet - this build only puts our
                                                              // depth where the test can see it.
                            khd_m == 67 ||                    // 26202 PROJECTION-CENSUS RETENTION OFF -
                                                              // restore 26201's zeroing of the census at
                                                              // the stats arm. The census IS the near-class
                                                              // gate's reference, so under 67 both the
                                                              // 26199 rung gate and the 26201 substitution
                                                              // stand down for a frame or two after every
                                                              // arm. Pair with encNearRefM reading -1 in a
                                                              // freshly armed dump.
                            khd_m == 66 ||                    // 26202 NEAR-CLASS CORRECTIONS OFF - restore
                                                              // the 26201 forms: compare raw -m32 instead
                                                              // of the true near -m32/m22, and qualify a
                                                              // substitute with the fail-OPEN refusal test
                                                              // instead of the fail-closed endorsement.
                                                              // Pair with encEndorseRejects: under 66 it
                                                              // stops climbing because nothing can be
                                                              // refused an endorsement any more.
                                                              // NOT restored under 66: 26201's probe also
                                                              // SPENT encNearClassRejects on every look.
                                                              // Reinstating that would make the one lane
                                                              // this A/B needs to read unreadable during
                                                              // the A/B, so the probe stays side-effect
                                                              // free in both arms and only the two
                                                              // behavioural forms revert.
                            khd_m == 65 ||                    // 26199 ENCODE NEAR-CLASS AGREEMENT OFF -
                                                              // restore the absolute near band alone.
                                                              // Underwater this brings the punch-through
                                                              // back. Pair with encNearClassRejects.
                            khd_m == 64 ||                    // 26197 BELOW-LAYER ATMOSPHERIC STAND-DOWN
                                                              // OFF - run the above-layer fog/haze model
                                                              // with the camera underwater. Pair with
                                                              // fogBelowStands + fogBelowCamY.
                            khd_m == 61 ||                    // 26198 FOG FAR-FADE RAMP OFF ENTIRELY -
                                                              // restore the unconditional far-fade ramp.
                                                              // Pair with fogRampStands + fogEngEnd.
                            khd_m == 62 ||                    // 26196 ENGINE DECAY LANE OFF - revert to
                                                              // the SQF-staged fogParams decay. At staged
                                                              // decay 0 this restores the flat fog fill.
                                                              // Pair with fogDecayShipped + fogDecaySubs.
                            khd_m == 63 ||                    // 26196 RELATIVE DARK RE-SEED BAR OFF -
                                                              // back to the absolute 1.0 of 26159.
                                                              // Pair with blkDarkSeedBlocks.
                            khd_m == 60 ||                    // 26195 ENGINE DISTANCE HAZE OFF. Zeroes
                                                              // hazePars.w, so KhHazeT returns 1.0 and
                                                              // the atmospheric block falls back to its
                                                              // 26194 fog-only arming - the mesh goes
                                                              // crisp against hazed terrain again, which
                                                              // IS the fault. Pair with hazeArms: the
                                                              // counter stops climbing under 60.
                            khd_m == 29 ||                    // 26148: shader reject bypass - locality clamp and
                                                              // near floor forced to pass (look-down diagnostic)
                            khd_m == 28;                      // 26160 REUSED: last-fire clamp with the camera
                                                              // EXTRAPOLATED one frame forward, because the
                                                              // late paint is consumed by the next frame.
                                                              // (was 26092-26093 adaptive-floor opt-in - RETIRED
                                                              // to an accepted no-op at 26094 (adaptive is
                                                              // the default now; kept for script compat).
                                                              // 26095: mode 29 (the legacy arm-floor escape
                                                              // hatch) is REMOVED with its floor branch per
                                                              // §9 - 29 now rejects; ledger at the T-machine
        // 26383 RETIRED NUMBERS FAIL LOUDLY NOW. Each of these was
        // whitelisted with no reader anywhere in rendering_integration.hpp,
        // so setRenderDebug returned TRUE and did nothing - the worst
        // failure mode a switch has, because a script cannot tell it from a
        // working arm. Verified at this build by expanding this whitelist
        // and matching it against every equality reader plus every raw
        // occurrence in the source. 143/144/150 are NOT here: they are
        // documented aliases FOR the default, where returning true and
        // changing nothing is the correct answer.
        // 26401 THE 26383 CENSUS WAS INCOMPLETE, AND ITS OWN LEDGER NAMES
        // THREE OF THE MISSES. Re-run with a TRANSITIVE alias closure from
        // g_dbg_mode (97 aliases; 26383's single-pass scan could not see
        // second-hop locals such as khv_nz_m = khv_dbg_vp, which is the only
        // reader of 204/205) followed by an exhaustive `== N` / `!= N` sweep
        // over the comment- and string-stripped source. SEVEN MORE NUMBERS
        // HAVE NO COMPARISON ANYWHERE: 90, 91, 92, 100, 101, 104, 120. The
        // 26244 ledger in kh_fill_reproj already says so in writing - "Retired
        // to no-ops by this build: 83 (its revert is the default now), 90, 91,
        // 92 and 94" - and only 94 was delisted from that sentence. They are
        // added here.
        // 207 IS NOT ADDED and this is the note that stops the next census
        // delisting it. 26386 folded the upward identity-gate ramp into mode 0
        // and mapped 207 to dbg_ctl[2] = 0 deliberately, so it is a default
        // alias exactly like 143/144/150/190 - no reader, correct behaviour.
        // The kept-alias list is 143 / 144 / 150 / 190 / 207 / 218 (26402
        // folded 218's 100x backstop into mode 0, so setting it is correct
        // and changes nothing; 219 is its revert).
        const bool khd_dead = khd_m == 90  || khd_m == 91  || khd_m == 92  ||
                              khd_m == 94  || khd_m == 100 || khd_m == 101 ||
                              khd_m == 103 || khd_m == 104 || khd_m == 105 ||
                              khd_m == 106 || khd_m == 109 || khd_m == 120 ||
                              khd_m == 135 ||
                              khd_m == 147 || khd_m == 151 || khd_m == 169 ||
                              khd_m == 172 ||
                              // 26416: 213 was INERT since it was minted - slots are
                              // only created inside the atlas-ensure block, so raising
                              // the count mid-session finds nulls. The question it
                              // asked is moot now that 26396 serves every pool miss
                              // immediately (PoolFall == PoolMiss, DropAge/DropDead 0),
                              // so it is delisted rather than repaired.
                              khd_m == 213 ||
                              // 26420: 85 was the live-atlas fallback. Its fault (the
                              // drift gap) was closed at the source by 26415/26416 and
                              // it was field-tested against the teleport transient and
                              // did not fix that either. Removed, not recycled.
                              khd_m == 85;
        if (!khd_ok || khd_dead) return game_value(false);
        RenderIntegration::g_dbg_mode.store(khd_m, std::memory_order_relaxed);
        return game_value(true);
    } catch (...) {
        return game_value(false);
    }
}


// 26180 A/B WINDOWING. getRenderStats arms ONCE per session
// (g_stats_armed.exchange), so before this existed every mode A/B carried
// its own pre-toggle window with it - dump99 and dump54 overlapped by 2270
// cycles for exactly that reason, and nine lanes read byte-identical across
// two supposedly different field rounds. This re-zeroes the same block the
// arming call zeroes, so a mode can be measured in a clean window without
// restarting the session. Cold-timeline stamps are session state and are
// deliberately NOT touched.
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
        // UI-mask census (ledger at KhUiMask): clears ~ one per frame while
        // any UI-phase pass is visible; skips ~ one at spawn (the learn
        // latency) and whenever the mask machinery could not fire;
        // uiOnlyDraws counts ALL write-window pass draws (26061).
        out.push_back(kv("uiMaskClears", RenderIntegration::g_ui_mask_clears));
        out.push_back(kv("uiMaskSkips", RenderIntegration::g_ui_mask_skips));
        out.push_back(kv("uiCovVetoes", RenderIntegration::g_ui_cov_vetoes));
        // 26080 chain GPU timing (timestamp ring; last completed frame):
        out.push_back(kv("fxChainGpuUs", RenderIntegration::g_stats.fx_chain_gpu_us));
        out.push_back(kv("fxTopFxId", RenderIntegration::g_stats.fx_top_fx_id));
        out.push_back(kv("fxTopFxUs", RenderIntegration::g_stats.fx_top_fx_us));
        // 26084 write-window GPU timing (the UI ring; last completed frame):
        out.push_back(kv("fxUiGpuUs", RenderIntegration::g_stats.fx_ui_gpu_us));
        out.push_back(kv("fxUiTopFxId", RenderIntegration::g_stats.fx_ui_top_fx_id));
        out.push_back(kv("fxUiTopFxUs", RenderIntegration::g_stats.fx_ui_top_fx_us));
        // 26085 scene-capture GPU cost (per-flush aggregate):
        out.push_back(kv("fxSceneCapUs", RenderIntegration::g_stats.fx_scene_cap_us));
        // 26090 CPU-side flush attribution (Campaign-18 Step-1; QPC wall
        // time, last completed measurement - the two-dump experiment's
        // keys: CPU stats stay FLAT across 4K vs 1080p, GPU rings scale):
        out.push_back(kv("fxCpuParkUs", RenderIntegration::g_stats.fx_cpu_park_us));
        out.push_back(kv("fxCpuFlushUs", RenderIntegration::g_stats.fx_cpu_flush_us));
        out.push_back(kv("fxCpuUiParkUs", RenderIntegration::g_stats.fx_cpu_ui_park_us));
        out.push_back(kv("fxCpuUiFlushUs", RenderIntegration::g_stats.fx_cpu_ui_flush_us));
        out.push_back(kv("fxCpuMaskUs", RenderIntegration::g_stats.fx_cpu_mask_us));
        out.push_back(kv("fxCpuSnapUs", RenderIntegration::g_stats.fx_cpu_snap_us));
        out.push_back(kv("fxCpuUiSnapUs", RenderIntegration::g_stats.fx_cpu_ui_snap_us));
        out.push_back(kv("uiOnlyDraws", RenderIntegration::g_ui_only_draws));
        // 26059: arming-path census - arms = UI-phase-thread compose
        // detections (~1/frame while a UI-mode pass is visible); aborts
        // expected 0 (pending clear killed by a foreign target).
        out.push_back(kv("uiMaskArms", RenderIntegration::g_ui_mask_arms));
        out.push_back(kv("uiMaskAborts", RenderIntegration::g_ui_mask_aborts));
        // 26094 arm-floor keys (ledger at the T-machine's adaptive
        // floor, the DEFAULT since 26094): floorHolds = genuine
        // boundaries the ACTIVE floor suppressed - expect ~0 in play;
        // sustained ticking means dump and report (mode 29 is
        // RETIRED at 26095; rollback is a build revert). bndEmaMs =
        // live boundary cadence in ms (~the
        // frame period; the adaptive floor's basis).
        out.push_back(kv("uiMaskFloorHolds", RenderIntegration::g_ui_mask_floor_holds));
        out.push_back(kvf("uiMaskBndEmaMs",
            RenderIntegration::kh_qpc_ticks_to_us(RenderIntegration::g_ui_mask.bnd_ema_ticks) / 1000.0f));
        // 26057: draw-time probe census + identity eyeball keys. probes >
        // 0 with misses ~ probes and arms 0 = identity mismatch (compare
        // ProbeLastLo against LearnLo0); probes > 0 with a matching Lo and
        // arms > 0 = machine armed. Lo keys are the low 24 bits of the
        // weak identities (SQF scalars are 32-bit floats; full pointers
        // do not survive the trip).
        out.push_back(kv("uiMaskProbes", RenderIntegration::g_ui_mask_probes));
        out.push_back(kv("uiMaskProbeMisses", RenderIntegration::g_ui_mask_probe_misses));
        out.push_back(kv("uiMaskProbeLastLo",
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(RenderIntegration::g_ui_mask.last_probe_id) & 0xFFFFFFu)));
        out.push_back(kv("uiMaskLearnLo0",
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(RenderIntegration::g_ui_mask.bb_id[0]) & 0xFFFFFFu)));
        out.push_back(kv("uiMaskLearnN", static_cast<uint64_t>(RenderIntegration::g_ui_mask.bb_n)));
        // AlphaMin/Max (26058, retained as the live health check): 255/255
        // = the coverage channel is pinned (no clear landed this frame);
        // a low pair = the injected mask is alive. 999 = never sampled.
        out.push_back(kv("uiMaskAlphaMin", static_cast<uint64_t>(
            RenderIntegration::g_ui_mask.alpha_min < 0 ? 999 : RenderIntegration::g_ui_mask.alpha_min)));
        out.push_back(kv("uiMaskAlphaMax", static_cast<uint64_t>(
            RenderIntegration::g_ui_mask.alpha_max < 0 ? 999 : RenderIntegration::g_ui_mask.alpha_max)));
        // 26060: the apply-once gate census - ~0 in normal play, ~frame
        // count while a pause menu / map / editor holds the frame still
        // (each of those frames would otherwise have compounded the
        // UI-phase effects onto their own previous output).
        out.push_back(kv("uiStaleSkips", RenderIntegration::g_ui_stale_skips));
        out.push_back(kv("compositeInjections", RenderIntegration::g_stats.composite_injections));
        out.push_back(kv("compositeMeshes", RenderIntegration::g_stats.composite_meshes));
        out.push_back(kv("compositeSkips", RenderIntegration::g_stats.composite_skips));
        // 26439 the hook-latch instrument (campaign-53 handoff, standing
        // finding 2). attempts counts installs actually tried; a nonzero
        // failPhase with its MH status is the transient-failure conviction
        // the ledger asked for; vtDrift counting while hookActive reads 1 is
        // the recreated-context blind spot, previously invisible. SESSION
        // STATE like the latches themselves - reset_render_stats leaves
        // these alone (cold-timeline rule). The active bit already rides the
        // long-standing reorderHook lane below and is NOT duplicated here.
        // The three signed lanes ride kvf: MhStatus is -1 for the init phase
        // and FailSlot is -1 at rest, and the uint64 lambda would wrap both
        // (the 26406 lesson, signedness flavour).
        out.push_back(kv("reorderHookAttempts", RenderIntegration::g_reorder_hook_attempts));
        out.push_back(kvf("reorderHookMhStatus", static_cast<float>(RenderIntegration::g_reorder_hook_mh_status)));
        out.push_back(kvf("reorderHookFailPhase", static_cast<float>(RenderIntegration::g_reorder_hook_fail_phase)));
        out.push_back(kvf("reorderHookFailSlot", static_cast<float>(RenderIntegration::g_reorder_hook_fail_slot)));
        out.push_back(kv("reorderHookVtDrift", RenderIntegration::g_reorder_hook_vt_drift));
        // 26476: the retry ladder + the no-MSAA trigger census (catalog
        // entry; the first census lane reading 0 in a no-FSAA dump names
        // the broken trigger link).
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
        // 26477 funnel-split lanes (ledger at g_hook_draw_foreign).
        out.push_back(kv("hookDrawPass", RenderIntegration::g_hook_draw_pass.load(std::memory_order_relaxed)));
        out.push_back(kv("hookDrawTidBail", RenderIntegration::g_hook_draw_tidbail.load(std::memory_order_relaxed)));
        out.push_back(kv("hookDrawForeign", RenderIntegration::g_hook_draw_foreign.load(std::memory_order_relaxed)));
        out.push_back(kv("hookMapTidBail", RenderIntegration::g_hook_map_tidbail.load(std::memory_order_relaxed)));
        out.push_back(kv("hookMapForeign", RenderIntegration::g_hook_map_foreign.load(std::memory_order_relaxed)));
        out.push_back(kvf("hookFctxType0", static_cast<float>(RenderIntegration::g_hook_fctx_type[0])));
        out.push_back(kvf("hookFctxType1", static_cast<float>(RenderIntegration::g_hook_fctx_type[1])));
        out.push_back(kvf("hookFctxType2", static_cast<float>(RenderIntegration::g_hook_fctx_type[2])));
        out.push_back(kvf("hookFctxType3", static_cast<float>(RenderIntegration::g_hook_fctx_type[3])));
        // 26478 trig-miss DSV census (ledger at g_trigmiss_cycles).
        out.push_back(kv("trigMissCycles", RenderIntegration::g_trigmiss_cycles));
        out.push_back(kv("trigMissDsvNull", RenderIntegration::g_trigmiss_null));
        out.push_back(kv("trigMissMainTex", RenderIntegration::g_trigmiss_mainmatch));
        out.push_back(kv("trigMissW", static_cast<uint64_t>(RenderIntegration::g_trigmiss_w)));
        out.push_back(kv("trigMissH", static_cast<uint64_t>(RenderIntegration::g_trigmiss_h)));
        out.push_back(kv("trigMissFmt", static_cast<uint64_t>(RenderIntegration::g_trigmiss_fmt)));
        out.push_back(kv("trigMissSamp", static_cast<uint64_t>(RenderIntegration::g_trigmiss_samp)));
        // 26479 trig-miss extras + the DSV-bind census + block pipeline.
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
        out.push_back(kv("blkRateHolds", RenderIntegration::g_blk_rate_holds));   // 26480 (retired 26481; reads 0)
        out.push_back(kv("blkSnapAdopts", RenderIntegration::g_blk_snap_adopts));   // 26481
        out.push_back(kv("blkSnapRejects", RenderIntegration::g_blk_snap_rejects));   // 26483
        out.push_back(kv("blkRingPicks", RenderIntegration::g_blk_ring_picks));   // 26487
        out.push_back(kv("blkRingBandFiltered", RenderIntegration::g_blk_ring_band_filtered));   // 26501
        out.push_back(kv("blkRingNoBand", RenderIntegration::g_blk_ring_no_band));   // 26501
        out.push_back(kv("blkRingStarveHolds", RenderIntegration::g_blk_ring_starve_holds));   // 26502
        out.push_back(kv("blkPubSlews", RenderIntegration::g_blk_pub_slews));   // 26503
        out.push_back(kv("sunVpSpanXM", RenderIntegration::g_kh_sunvp_span[0]));   // 26504
        out.push_back(kv("sunVpSpanYM", RenderIntegration::g_kh_sunvp_span[1]));   // 26504
        out.push_back(kv("sunVpSpanZM", RenderIntegration::g_kh_sunvp_span[2]));   // 26504
        out.push_back(kv("sunVpLatches", RenderIntegration::g_kh_sunvp_latches));   // 26504
        out.push_back(kv("sunVpFitGrows", RenderIntegration::g_kh_sunvp_fit_grows));   // 26505
        // 26490: the 1x sweep's inner verdict.
        out.push_back(kv("rtResolveTrue", RenderIntegration::g_rt_resolve_true));
        out.push_back(kv("rtResolveFalse", RenderIntegration::g_rt_resolve_false));
        out.push_back(kv("rtLastRejW", RenderIntegration::g_rt_last_rej_w));
        out.push_back(kv("mtxScanHits", RenderIntegration::g_mtx_scan_hits));   // 26491
        out.push_back(kv("mtxScanOff", RenderIntegration::g_mtx_scan_off));   // 26491
        out.push_back(kv("msaaToggleWipes", RenderIntegration::g_msaa_toggle_wipes));   // 26492
        out.push_back(kv("missionResetFails", RenderIntegration::g_mission_reset_fails));   // 26495: mission-end lock exhaustions (destroy deferred, never skipped)
        out.push_back(kv("fsaaStandDownFrames", RenderIntegration::g_fsaa_standdown_frames));   // 26496: injection triggers refused by the FSAA requirement
        out.push_back(kv("fsaaDepthSamples", RenderIntegration::g_scene_depth_samples));   // 26496/26497: the learned scene-depth sample count (1 = standing down, 0 = not yet learned; learned at the flush adoption since 26497)
        // 26484: mode-lane census + cache telemetry.
        out.push_back(kvf("blkModeLast", RenderIntegration::g_light_probe.last_mode));
        out.push_back(kvf("blkModeV0", RenderIntegration::g_blk_mode_census_v[0]));
        out.push_back(kv("blkModeN0", RenderIntegration::g_blk_mode_census_n[0]));
        out.push_back(kvf("blkModeV1", RenderIntegration::g_blk_mode_census_v[1]));
        out.push_back(kv("blkModeN1", RenderIntegration::g_blk_mode_census_n[1]));
        out.push_back(kvf("blkModeV2", RenderIntegration::g_blk_mode_census_v[2]));
        out.push_back(kv("blkModeN2", RenderIntegration::g_blk_mode_census_n[2]));
        out.push_back(kvf("blkModeV3", RenderIntegration::g_blk_mode_census_v[3]));
        out.push_back(kv("blkModeN3", RenderIntegration::g_blk_mode_census_n[3]));
        out.push_back(kv("shaderCacheHits", RenderIntegration::g_shader_cache_hits));
        out.push_back(kv("shaderCacheMisses", RenderIntegration::g_shader_cache_misses));
        out.push_back(kv("shaderCompileMs", RenderIntegration::g_shader_compile_ms));
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
        // 26189: injections the near floor would leave on the STOCK path but
        // that armed anyway. 0 by construction under the restored floor;
        // under setRenderDebug 55 it prices the falsified 26188 exposure
        // (936 of 1024 in dump555).
        out.push_back(kv("arbNearDenied", RenderIntegration::g_arb_near_denied));
        // 26189: verdict changes of the near classification - every flip is a
        // discontinuity in the mesh's depth against nearby world geometry.
        // dump555 read 4 across a whole 1025-flush session, which is TOO FEW
        // to be the operator's continuous per-distance banding: the toggle is
        // real but is not the dominant term. Any next model must explain an
        // effect that persists with this lane flat.
        out.push_back(kv("arbNearFlips", RenderIntegration::g_arb_near_flips));
        out.push_back(kv("flushFxArbs", RenderIntegration::g_stats.flush_fx_arbs));
        out.push_back(kv("compositeTranslDefers", RenderIntegration::g_stats.composite_transl_defers));
        out.push_back(kv("texturedDraws", RenderIntegration::g_stats.textured_draws));
        out.push_back(kv("texLoads", RenderIntegration::g_stats.tex_loads));
        out.push_back(kv("fbxImports", RenderIntegration::g_stats.fbx_imports));
        out.push_back(kv("fbxCacheHits", RenderIntegration::g_stats.fbx_cache_hits));
        out.push_back(kv("fbxCacheWrites", RenderIntegration::g_stats.fbx_cache_writes));
        out.push_back(kv("fbxCacheEvicts", RenderIntegration::g_stats.fbx_cache_evicts));
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
        // 26232 DO NOT USE liveAccepts AS AN ACCEPTANCE GATE. It increments in
        // shadow_live_test_window when a sampling transform is classified, and
        // reset_stat_counters assigns g_stats = RenderStats{} - the WHOLE
        // struct - at every arming. The live table latches once, seconds into a
        // session (coldFirstLiveLatchS names the moment, and that lane is a
        // sticky cold marker that arming does NOT clear), and the table itself
        // is live state that correctly survives the reset. So in any dump armed
        // after the first few seconds this reads 0 BY CONSTRUCTION while the
        // band receive is perfectly healthy. The campaign-34 handoff made
        // "liveAccepts > 0" a hard blocker on shipping the stencil term; it is
        // unsatisfiable in a properly windowed A/B dump, and the campaign's own
        // rule applies - if a gauge reads impossibly, suspect the gauge.
        // THE GAUGE THAT ACTUALLY ANSWERS IT is recvTermSkips 0 (no lit mesh
        // drawn without the received-shadow term) plus recvStreamSkips 0,
        // bandSlotsValid 8 and liveValidEntries 8. Those are what to require.
        out.push_back(kv("liveAccepts", RenderIntegration::g_stats.live_accepts));
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
        // 26133 motion-ghost campaign gauges (ledger at the fire statics).
        out.push_back(kv("fireSnapCopies", RenderIntegration::g_fire_snap_copies));
        out.push_back(kv("fireSnapFails", RenderIntegration::g_fire_snap_fails));
        out.push_back(kv("fireSnapLive", RenderIntegration::g_fire_snap_live ? 1 : 0));
        out.push_back(kvf("fireLiveViewDeltaM", RenderIntegration::g_fire_live_view_delta_m));
        out.push_back(kvf("fireLiveViewDeltaMax", RenderIntegration::g_fire_live_view_delta_max));
        out.push_back(kv("fireLiveViewMoved", RenderIntegration::g_fire_live_view_moved));
        out.push_back(kv("castRearmAtlasBlocks", RenderIntegration::g_cast_rearm_atlas_blocks));
        out.push_back(kv("castRearmStrandedClears", RenderIntegration::g_cast_rearm_stranded_clears));
        // 26135 moved-epoch skip gauges (ledger at the skip in mask_cast_engine).
        out.push_back(kv("fireEpochSkips", RenderIntegration::g_fire_epoch_skips));
        out.push_back(kv("fireEpochSkipsBridge", RenderIntegration::g_fire_epoch_skips_bridge));
        out.push_back(kvf("fireEpochLastM", RenderIntegration::g_fire_epoch_last_m));
        out.push_back(kvf("fireEpochLastRot", RenderIntegration::g_fire_epoch_last_rot));
        out.push_back(kv("blkDarkContraSticky", RenderIntegration::g_blk_dark_contra_sticky));
        // 26236 THE BLACK-BOX LANES THAT SEE THE PATH THAT ACTUALLY RUNS.
        // A cold start caught in the act read blkDarkContraSticky 1.01e+06 with
        // blkDarkReseeds 0, blkDarkSeedBlocks 0, blkDarkReadopts 0 and
        // blockRegimeAdopts 0 - so every contradicted upload took the STICKY
        // branch, which had no census. The 26195 ratio lanes that exist to
        // decide this fix live in the other branch and read zero because
        // control never reaches them. Three campaigns have measured the wrong
        // path.
        //   blkStickyRejects   - refusals on the path that actually runs.
        //   blkStickyPendAgeMax - THE decisive lane. The sticky guard refuses to
        //     re-seed and deliberately does not touch pend_t, so a genuinely
        //     ageing pending must eventually clear the 500 ms exclusivity bar
        //     and adopt. It did not, a million times. If this reads > 0.5 the
        //     pending IS ageing and the adoption chain is never entered
        //     (khp_agree never true); if it stays pinned near 0 something
        //     upstream is refreshing pend_t. Those are different faults with
        //     different fixes and no previous build could tell them apart.
        //   blkStickyInSl / blkStickyPendSl / blkStickyStdSl - the three sun
        //     lanes at the last refusal: what arrived, what is waiting, what is
        //     ruling.
        //   blkDarkForceAdopts / blkDarkForceHeldS - the 26236 escape. Expect 0
        //     in a healthy session. Non-zero means the fault occurred AND was
        //     broken; HeldS says how long the world was wrong before it broke.
        //     This is a SYMPTOM fix - the root cause is still unmeasured, which
        //     is what the lanes above are for.
        out.push_back(kv("blkStickyRejects", RenderIntegration::g_blk_sticky_rejects));
        out.push_back(kvf("blkStickyPendAge", RenderIntegration::g_blk_sticky_pend_age));
        out.push_back(kvf("blkStickyPendAgeMax", RenderIntegration::g_blk_sticky_pend_age_max));
        out.push_back(kvf("blkStickyInSl", RenderIntegration::g_blk_sticky_in_sl));
        out.push_back(kvf("blkStickyPendSl", RenderIntegration::g_blk_sticky_pend_sl));
        out.push_back(kvf("blkStickyStdSl", RenderIntegration::g_blk_sticky_std_sl));
        out.push_back(kv("blkDarkForceAdopts", RenderIntegration::g_blk_dark_force_adopts));
        out.push_back(kvf("blkDarkForceHeldS", RenderIntegration::g_blk_dark_force_held_s));
        // 26159 (ledger at g_blk_dark_seed_blocks): re-seeds refused so the
        // bright pending can actually age past its 500 ms exclusivity bar.
        out.push_back(kv("blkDarkSeedBlocks", RenderIntegration::g_blk_dark_seed_blocks));
        out.push_back(kv("fireExtrapUsed", RenderIntegration::g_fire_extrap_used));   // 26160
        out.push_back(kv("fireExtrapFails", RenderIntegration::g_fire_extrap_fails));
        out.push_back(kv("blkDarkReadopts", RenderIntegration::g_blk_dark_readopts));
        // 26133 sky-sun cold bootstrap gauges (ledger at publish_world_lighting).
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
        // 26106..26108 fx depth-pair churn latch (ledger at
        // kh_fx_depth_pair_guard; 26108: guards EVERY depth-consuming
        // chain draw - the needs_depth set - with a per-flush verdict):
        // holds = guarded DRAWS served the held pair (several per held
        // flush when multiple depth effects run), jumpAdopts =
        // confirmed genuine cuts/teleports adopted one flush late.
        // (26106's fogScatterPair* names retired with the widening.)
        // 26109: bridgeAdopts = >1.6x jumps corroborated by the live
        // bridge projection and adopted the SAME flush (zero error
        // frames - the genuine zoom/teleport class); jumpAdopts now
        // counts only the UNCORROBORATED jumps that still needed the
        // hold + second sighting. All three reset with the stat arm.
        out.push_back(kv("fxDepthPairHolds", RenderIntegration::g_khfx_pair_holds));
        out.push_back(kv("fxDepthPairJumpAdopts", RenderIntegration::g_khfx_pair_jump_adopts));
        out.push_back(kv("fxDepthPairBridgeAdopts", RenderIntegration::g_khfx_bridge_adopts));
        // 26113: confirmVetoes = second sightings REFUSED because the
        // live bridge contradicted the candidate (multi-flush foreign
        // latch runs held to zero error frames).
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
        {   // BLACK-BOX (see the campaign): the engine block AMBIENT color
            //   nb[8..10] -> lightAmb.rgb, the term ndl<=0 faces collapse to -
            //   distinct from the sun lane (sunBrightness reads nb[16..18]).
            //   -1 = block not locked this frame (matches fill_lighting_frame_cb).
            const bool amb_locked = RenderIntegration::g_light_probe.hits > 0 &&
                                    RenderIntegration::g_light_probe.meta == 40;
            const float* amb = RenderIntegration::g_light_probe.nb + 8;
            out.push_back(kvf("ambHDR_R", amb_locked ? amb[0] : -1.0f));
            out.push_back(kvf("ambHDR_G", amb_locked ? amb[1] : -1.0f));
            out.push_back(kvf("ambHDR_B", amb_locked ? amb[2] : -1.0f));
        }
        {   // BLACK-BOX: what the last LIT mesh each path actually received.
            //   AmbTermMax = max(amb)*amb_scalar = the ndl<=0 face brightness
            //   before the base-multiply; ~0 with GuardBase 1e9 convicts the
            //   ambient-zero + guard-stood-down root. Path valid=0 => that path
            //   drew no lit mesh since the arm (its lanes read the -1 sentinel).
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
            //   publish_world_lighting - what phase 2 would feed BOTH draw paths.
            //   Compare blockStagedAmb* against the live injBoxAmb* (dark) and
            //   ambHDR* (healthy): if these read healthy, repointing the consumers
            //   here fixes the injection's stale read. SunMax vs injBoxSunMax says
            //   whether the direct term is starved at injection too. valid=0 =>
            //   block unlocked at publish time.
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
        // ===================================================================
        // 26195 ATMOSPHERIC ROW CENSUS (pure reads; no state, no behaviour).
        //
        // CLAIM UNDER TEST, stated so the dump can kill it:
        //   (A) g_sky_probe's buffer IS the engine's PSCB_NonFrequent, bound
        //       at PS b0 - the buffer whose row 14 the terrain and object
        //       shaders read as PSC_HazePars, the DISTANCE HAZE that tints
        //       world geometry blue and that our meshes do not receive.
        //   (B) g_light_probe's buffer IS the engine's PS b5 - the buffer
        //       whose row 10 / 12 hold the fog decay, density, end, inverse
        //       range, LAYER ALTITUDE and below-layer extinction. Three of
        //       those six are already consumed (fogEngine.xyz = nb 41/48/49);
        //       the other three have never been read.
        //
        // The census is ANCHOR-RELATIVE by construction, so it survives a
        // non-zero nb_base instead of silently lying about it: the sky
        // anchor already pins row 17 to the literal gradient triple
        // (0.25, 0.75, 3.5), so atmSkyR17x/y/z is the RULER. If those three
        // read the triple, the row indexing below is confirmed and
        // atmSkyR14* IS PSC_HazePars. If they do not, count rows from
        // wherever the triple actually lands and every other row follows.
        //
        // Row semantics (PSCB_NonFrequent, from the engine's own shader
        // reflection): 0 TerrainSatNormDist, 1 FogColor, 7 WaterFogColor,
        // 13 ClearColor, 14 HazePars, 15/16 extinction triples, 17 the
        // gradient control points. nb[i] is ABSOLUTE float (nb_base + i) -
        // read atmSkyNbBase / lightLocNbBase / lightLocOff alongside these.
        // ===================================================================
        out.push_back(kv("atmSkyNbBase", RenderIntegration::g_sky_probe.nb_base));
        // 26195 HAZE ARM CENSUS: what kh_fill_haze actually decided, and
        // the exact operands the shader integrated - publish them and any
        // hazeT is reproducible offline for any distance, which is why no
        // visual slot was burned on a ladder.
        // 26200 THREE-SOURCE NEAR CENSUS. dump993 (1.46 m above water, the thin
        // band where the mesh clips the horizon) shipped near 10.005 while the
        // census-dominant pair read 1.793 with 2031 hits - so the near-class
        // gate correctly refused the SLOT and the fault came in through the
        // fallback instead. These three side by side say which of the three
        // sources is the odd one out, in one dump, without guessing.
        out.push_back(kvf("encNearSlot", RenderIntegration::g_ro.slot_near_live));
        // 26202: the TRUE near (-m32/m22), which is what the gate now
        // compares. 26201 published raw -m32 here; the two differ by the
        // factor m22, ~1.0005 on every pair measured, so no comparison an
        // operator has made against this lane changes - but the lane and
        // the gate now agree exactly instead of nearly.
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
        // 26202 SUBSTITUTION FORENSICS. encBridgeSwaps says a swap happened;
        // these say WHOSE pair it displaced and by how much. encSwapSrc is
        // the encode author of the displaced pair - 0 bridge, 1 measured,
        // 2 slot, 3 keep, 4 live-ref, 5 world, 4294967295 = no swap yet -
        // and encSwapSrcBits is the accumulated set (bit N = author N).
        // THE QUESTION THIS SETTLES: 26201 named its fix for the BRIDGE,
        // but dump5933 had the sniffer live and reading 4.35528, which
        // means the measured overwrite had already replaced the bridge's
        // flat 10 before any rung ran - so the shipped 10.005 should not
        // have been able to come from the bridge at all. If encSwapSrc
        // reads 3 or 4, the fault was the injection KEEP or LIVE-REF rung,
        // neither of which carries a near-class gate, and the 26201 swap
        // has been working as a backstop for an ungated rung rather than
        // as the bridge guard its ledger describes. Do not re-gate those
        // rungs on this reasoning: read the lane first.
        out.push_back(kv("encSwapSrc", static_cast<uint64_t>(RenderIntegration::g_enc_swap_src)));
        out.push_back(kv("encSwapSrcBits", static_cast<uint64_t>(RenderIntegration::g_enc_swap_src_bits)));
        out.push_back(kvf("encSwapFromM", RenderIntegration::g_enc_swap_from_m));
        out.push_back(kvf("encSwapToM", RenderIntegration::g_enc_swap_to_m));
        out.push_back(kv("encEndorseRejects", RenderIntegration::g_enc_endorse_rejects));
        // 26202 BAR MARGIN (full ledger at g_enc_near_ratio_last). The ratio
        // between the shipped near and the census reference, every frame, and
        // the session peak. KH_ENC_NEAR_CLASS refuses above 10. The bridge's
        // near is a flat 10 while the camera's tracks altitude, so the bridge
        // is out of class below a reference of 1.0 near (~2 m altitude) and
        // IN class above it - the 26201 substitution therefore covers the
        // bottom of the 1-2 m band and not the top. encNearRatioMax near 10
        // means the gate was close to the fault; stuck at 2-3 means it never
        // came near firing and a remaining clip is not something this gate
        // can see. Do not move the bar on one dump.
        out.push_back(kvf("encNearRatio", RenderIntegration::g_enc_near_ratio_last));
        out.push_back(kvf("encNearRatioMax", RenderIntegration::g_enc_near_ratio_max));
        out.push_back(kv("fogRampStands", RenderIntegration::g_fog_ramp_stands));
        out.push_back(kv("fogRampHolds", RenderIntegration::g_fog_ramp_holds));
        out.push_back(kvf("fogRampEndMin", RenderIntegration::g_fog_ramp_end_min));
        out.push_back(kvf("fogRampEndMax", RenderIntegration::g_fog_ramp_end_max));
        out.push_back(kv("fogBelowStands", RenderIntegration::g_fog_below_stands));
        out.push_back(kvf("fogBelowCamY", RenderIntegration::g_fog_below_cam_y));
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
        // ===================================================================
        // 26202 SHADOW DEPTH-TARGET CENSUS (full ledger at KhShadowTexObs in
        // rendering_integration.hpp). The instrument for the character-shadow
        // task: every depth texture that reaches shadow_live_consider_atlas,
        // its shape, and the clause that refused it.
        //
        // READ IT IN THIS ORDER.
        //  1. shadowTexConsiders. ZERO means the instrument never ran and
        //     every other lane here is meaningless - the probe only fires on
        //     depth-only binds whose target CHANGED, while shadow_live_wanted.
        //     A zero is an instrument fault, not a finding.
        //  2. shadowTexArrayRejects. NON-ZERO is the lead confirmed: an
        //     array-shaped depth target is being offered and refused. Read
        //     shadowTexArrayMaxW and shadowTexArrayMaxSlices beside it for
        //     the shape, and find its row below for the format and the count.
        //     ZERO against a healthy considers count is the lead KILLED -
        //     characters are not in an array target on this path, and the
        //     next question is coverage (does the probe see the character
        //     cascade pass at all), not the ArraySize clause.
        //  3. shadowTexAdopts / shadowTexKept / shadowTexSame. A candidate
        //     that passes every clause can still lose to the held atlas.
        //     Kept climbing with adopts at 1 is the normal steady state;
        //     kept climbing against a DIFFERENT shape means there are two
        //     single-slice candidates and we hold the larger one.
        //  4. shadowTexNRows / shadowTexEvicts. Evicts non-zero means more
        //     than twelve distinct shapes and the table is not the whole
        //     picture.
        //
        // clause: 0 = passed every clause (candidate), 1 = no SHADER_RESOURCE
        // bind, 2 = not square or under 1024, 3 = multisampled,
        // 4 = ArraySize != 1, 5 = format not depth-typeless. fmt is the raw
        // DXGI_FORMAT (R32_TYPELESS 39, R24G8_TYPELESS 44, R16_TYPELESS 53,
        // R32G8X24_TYPELESS 19). ids counts identity CHANGES at that shape,
        // not a distinct set: 1 = one stable texture for the whole session,
        // anything higher = recreated (a quality change) or two coexisting
        // targets alternating. Either way >1 says the shape is not served by
        // a single stable resource, which is the thing worth knowing.
        // ===================================================================
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
        // ===================================================================
        // 26203 PIXEL-STAGE ARRAY-SRV CENSUS (full ledger at KhSrvArrObs).
        // The separator for the two surviving character-shadow hypotheses.
        // The 26202 DSV census proved the ArraySize clause never fires and
        // the 4096 R32_TYPELESS atlas is the only depth-only candidate; the
        // field control proved the soldier DOES cast onto terrain beside the
        // box. So either the character is in our atlas and the receive misses
        // it (H1), or the engine resolves dynamic casters out of the array
        // that PS 142-155 sample and we only ever see half the shadow world
        // (H2). An array is invisible to a DSV census if it is never bound
        // depth-only, so this one is keyed on how a resource is READ.
        //
        // READ IT IN THIS ORDER.
        //  1. srvArrScans. ZERO means the instrument never ran - it is behind
        //     the stats arm and the render-thread test - and nothing else
        //     here means anything.
        //  2. srvArrDepthHits. NON-ZERO settles it for H2: a depth-shaped
        //     texture2darray at 1024 or wider is being bound to the pixel
        //     stage. Read srvArrDepthMaxW / srvArrDepthMaxSlices for the
        //     shape and find its row for the slot mask - the slot is the
        //     strongest hint about which shader family consumes it.
        //     ZERO across a session with healthy scans is H2 KILLED by
        //     elimination: the atlas we already hold is the only shadow
        //     resource in play, the character is in it, and the fault is in
        //     the receive - bands, bias, or coverage - not in the resource.
        //  3. srvArrHits vs srvArrDepthHits. Arrays that are NOT depth-shaped
        //     (cloud, particle, decal atlases) are expected and land in the
        //     table too; the gap between these two lanes is that traffic.
        //  4. srvArrBudgetStops. Non-zero means frames hit the 64-inspection
        //     cap and the census is a SAMPLE of those frames, not a complete
        //     account. A zero here makes the zero in lane 2 much stronger.
        //     srvArrEvicts non-zero means more than eight distinct shapes.
        //
        // fmt is the VIEW format (what the shader reads: R32_FLOAT 41,
        // R24_UNORM_X8_TYPELESS 46, R16_UNORM 56); resFmt is the resource's,
        // typeless for depth. dim: 5 = TEXTURE2DARRAY, 8 = TEXTURE2DMSARRAY.
        // slots is a BITMASK of the pixel-stage slots the view was bound to,
        // so 32768 = slot 15, the shadow slot the sample_c receivers use.
        // ===================================================================
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
        // ===================================================================
        // 26204 SHADOW-VOLUME SEAM (full ledger at g_svs_prev_dsv).
        // READ svSeams FIRST: zero means the prepass -> volume transition was
        // never detected and every other lane here is meaningless. Non-zero
        // with svSeamW/H at scene resolution means the seam is the scene
        // depth prepass, which is the one we want.
        // svSeamSquare counts cascade transitions correctly refused;
        // svSeamMain counts refusals because the target was our own main
        // depth identity - if THAT is climbing while svInjects stays zero,
        // our main-depth classification is what is blocking, not the seam.
        // With setRenderDebug 68 armed, svInjects and svInjectDraws must both
        // climb; svArmSkips is how often the seam fired while disarmed.
        // ===================================================================
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
        // 26205: svSeamMain is now a CENSUS, not a refusal - the main-depth
        // latch adopts the first DSV seen, which is the prepass/volume buffer
        // itself, so refusing on it refused the target. svMainW/H/Fmt is the
        // shape it classified as main; compare with svSeamW/H/Fmt to see
        // whether they are the same buffer (they should be). svSkipDims is the
        // replacement guard: seam target not at scene resolution.
        out.push_back(kv("svMainW", static_cast<uint64_t>(RenderIntegration::g_svs_main_w)));
        out.push_back(kv("svMainH", static_cast<uint64_t>(RenderIntegration::g_svs_main_h)));
        out.push_back(kv("svMainFmt", static_cast<uint64_t>(RenderIntegration::g_svs_main_fmt)));
        out.push_back(kv("svSkipDims", RenderIntegration::g_svs_skip_dims));
        out.push_back(kv("svSeamOrd", static_cast<uint64_t>(RenderIntegration::g_svs_seam_ord)));
        // ===================================================================
        // 26272 THE ORDINAL. A capture finally asked where our injection sits
        // among the engine's counting draws, and the answer was: 13123, in a run
        // of 760 spanning EID 47..13809. Roughly seven hundred shadow volumes
        // are counted BEFORE our depth exists in that buffer, so their verdict
        // at our pixels is computed against the floor behind us. That is not a
        // matrix error and ten builds of matrix work could never have touched
        // it.
        //   svInjCountOrd / svCountFrame  - the ordinal and its denominator.
        //     Near 0 -> we are first, this reading is WRONG, say so.
        //     Large  -> confirmed; arm on an earlier seam.
        //   svSeamFirstOrd - the ordinal at the frame's FIRST seam, i.e. the
        //     earliest point we could inject. If that is ALSO large, moving the
        //     seam buys nothing and the injection needs a different home
        //     entirely - worth knowing before building it.
        // svCountFrame stable frame to frame means one stencil pass; swinging
        // with the scene means the classifier folds several together and the
        // ordinal must be split per pass before it means anything.
        // ===================================================================
        out.push_back(kv("svCountFrame", static_cast<uint64_t>(RenderIntegration::g_svs_count_frame)));
        out.push_back(kv("svCountFrameMax", static_cast<uint64_t>(RenderIntegration::g_svs_count_frame_max)));
        out.push_back(kv("svInjCountOrd", static_cast<uint64_t>(RenderIntegration::g_svs_inj_count_ord)));
        out.push_back(kv("svInjCountOrdMax", static_cast<uint64_t>(RenderIntegration::g_svs_inj_count_ord_max)));
        out.push_back(kv("svSeamFirstOrd", static_cast<uint64_t>(RenderIntegration::g_svs_seam_first_ord)));
        // ===================================================================
        // 26274 WHICH PUBLICATION EACH PASS READ. Three field arms - raster
        // (lag gone, edges sliced), reprojection (slice gone, lag back), 107
        // (slice gone, BOX lags) - are one quantity seen three ways: the
        // injection and the composite are transformed by different camera
        // samples ~svReprojPxMean apart, and each arm only chooses who eats it.
        //   svPubOrdMean/Max LARGE -> the two passes catch different alternates
        //     of one frame (kh_adopt_frame_view's ledger: the sim republishes
        //     mid-cycle). Fix is SELECTION, and g_boundary_pv already identifies
        //     the right alternate.
        //   SMALL (0-2) -> same publication, so the 20 px is a real intra-frame
        //     camera change. Selection cannot help; the fix is one FROZEN sample
        //     per frame for both passes, with the box eating a sub-frame offset
        //     that campaign 37 sec 3.6 already judged invisible.
        // svPubOrdPairs should track flushes; well below means the two stamps
        // are not landing in the same frame and the deltas mean nothing.
        // ===================================================================
        // 26275 MODE 127: both passes take the frame's BOUNDARY publication -
        // fetched at the clear, before the seam and before the colour pass, so
        // it is determined before either pass runs and they get byte-identical
        // views. Acceptance is svPubOrdMean collapsing to 0; if it does not,
        // they still are not sharing and nothing needs judging by eye.
        //   svBpvInjTakes / svBpvCompTakes should both track flushes.
        //   svBpvMisses  - cycles with no clear-time sample, where BOTH passes
        //     stay on the historic path together. Large means the boundary
        //     fetch is unreliable and this approach cannot be the default.
        out.push_back(kv("svBpvInjTakes", RenderIntegration::g_svs_bpv_inj_takes));
        out.push_back(kv("svBpvCompTakes", RenderIntegration::g_svs_bpv_comp_takes));
        out.push_back(kv("svBpvMisses", RenderIntegration::g_svs_bpv_misses));
        // 26276 MODE 128: the boundary PROJECTION as well as the view. 127 proved
        // the view is shared (svBpvInjTakes == svBpvCompTakes == flushes,
        // svBpvMisses 0) and svReprojPxMean did not move - so the residual is the
        // projection, which is also where injNear's 0.07<->0.77 oscillation lives.
        //   svBpvProjTakes    should track flushes under 128.
        //   svBpvProjRejects  boundary pairs refused by the near-class gate, which
        //     still rules: a forced out-of-class pair trades a POSITION error for
        //     a DEPTH error (box punching through geometry), which is worse and
        //     harder to attribute. Small -> the shared projection is safe to ship.
        //     Large -> force is the wrong answer and both passes need the same
        //     GUARDED pair instead of the same raw one.
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
        // 26206: the shape of the buffer we ACTUALLY wrote into. svSeamW/H/Fmt
        // is the LAST seam of the frame and is a different buffer; these are
        // stamped at the write itself. svInjFmt 45 (D24_UNORM_S8_UINT) is the
        // stencil-volume buffer - the target. 44 (R24G8_TYPELESS) would be the
        // colour pass's depth, i.e. the wrong one.
        out.push_back(kv("svInjW", static_cast<uint64_t>(RenderIntegration::g_svs_inj_w)));
        out.push_back(kv("svInjH", static_cast<uint64_t>(RenderIntegration::g_svs_inj_h)));
        out.push_back(kv("svInjFmt", static_cast<uint64_t>(RenderIntegration::g_svs_inj_fmt)));
        // 26207: bind flags of the written target, and the count of seams
        // refused for being SRV-readable. The stencil-volume buffer is the
        // only scene-resolution depth target created WITHOUT a shader-resource
        // bind, so svInjBind 64 (DEPTH_STENCIL alone) is the target and 72
        // (DEPTH_STENCIL|SHADER_RESOURCE) would be the colour pass's depth.
        // svSkipSrv climbing is the guard working, not a fault.
        out.push_back(kv("svInjBind", static_cast<uint64_t>(RenderIntegration::g_svs_inj_bind)));
        out.push_back(kv("svSkipSrv", RenderIntegration::g_svs_skip_srv));
        // 26208 SELECTED vs WRITTEN. svSel* is the target CHOSEN at the seam;
        // svInj* is now stamped only after a draw actually landed. If svSelFmt
        // reads 45 while svInjFmt reads 0, selection is right and the injector
        // is bailing - svEnters says whether it was even called, and
        // svFailFrameCb / svFailObjCb name the bail. Both were silent before.
        out.push_back(kv("svSelW", static_cast<uint64_t>(RenderIntegration::g_svs_sel_w)));
        out.push_back(kv("svSelH", static_cast<uint64_t>(RenderIntegration::g_svs_sel_h)));
        out.push_back(kv("svSelFmt", static_cast<uint64_t>(RenderIntegration::g_svs_sel_fmt)));
        out.push_back(kv("svSelBind", static_cast<uint64_t>(RenderIntegration::g_svs_sel_bind)));
        out.push_back(kv("svEnters", RenderIntegration::g_svs_enters));
        out.push_back(kv("svFailFrameCb", RenderIntegration::g_svs_fail_framecb));
        out.push_back(kv("svFailObjCb", RenderIntegration::g_svs_fail_objcb));
        // 26209: svSeamAdj is how many seams ALSO satisfied the retired
        // adjacency rule. svSeams minus svSeamAdj is what the order-free
        // detector recovered; if they are equal, adjacency was never the
        // problem and the miss is elsewhere.
        out.push_back(kv("svSeamAdj", RenderIntegration::g_svs_seam_adj));
        // 26210 DEPTH-BIND CENSUS - what the OM hook actually SEES, before any
        // rule runs. "vol" = depth target with NO shader-resource bind, i.e.
        // the stencil-volume buffer class; "srv" = the readable ones.
        // THE NUMBER THAT MATTERS IS svBindVolRtv. The seam needs the volume
        // buffer bound WITH a render target (the prepass) and later depth-only
        // (the volumes). If svBindVolOnly climbs while svBindVolRtv stays at
        // or near zero, the prepass half never reaches our hook - deferred
        // context or another thread - and no seam rule can recover it. That
        // would mean this approach needs a different hook, not a fourth rule.
        out.push_back(kv("svBindVolRtv", RenderIntegration::g_svs_bind_vol_rtv));
        out.push_back(kv("svBindVolOnly", RenderIntegration::g_svs_bind_vol_only));
        out.push_back(kv("svBindSrvRtv", RenderIntegration::g_svs_bind_srv_rtv));
        out.push_back(kv("svBindSrvOnly", RenderIntegration::g_svs_bind_srv_only));
        out.push_back(kv("svRtvSetMax", static_cast<uint64_t>(RenderIntegration::g_svs_rtv_set_max)));
        // 26211 DEFERRED INJECTION. svPendArms is how often the seam armed one;
        // svPendFires how often a later draw performed it. They should track
        // each other closely. svPendDropped counts arms still outstanding at
        // the frame boundary - i.e. no draw followed the seam, which would
        // mean the deferral point is wrong.
        out.push_back(kv("svPendArms", RenderIntegration::g_svs_pend_arms));
        out.push_back(kv("svPendFires", RenderIntegration::g_svs_pend_fires));
        out.push_back(kv("svPendDropped", RenderIntegration::g_svs_pend_dropped));
        // 26212 ENGINE SHADOW MASK. svMaskHits = resolve binds recognised;
        // svMaskAdopts = SRVs created over a new identity (1 in a steady
        // session); svMaskW/H/Fmt = its shape; svMaskBinds = times bound to
        // our mesh shaders, which only happens under setRenderDebug 71.
        // svMaskAdopts 0 means the resolve was never recognised and the paint
        // will show nothing; svMaskFails means SRV creation was refused.
        out.push_back(kv("svMaskHits", RenderIntegration::g_svs_mask_hits));
        out.push_back(kv("svMaskAdopts", RenderIntegration::g_svs_mask_adopts));
        out.push_back(kv("svMaskFails", RenderIntegration::g_svs_mask_fails));
        out.push_back(kv("svMaskBinds", RenderIntegration::g_svs_mask_binds));
        out.push_back(kv("svMaskW", static_cast<uint64_t>(RenderIntegration::g_svs_mask_w)));
        out.push_back(kv("svMaskH", static_cast<uint64_t>(RenderIntegration::g_svs_mask_h)));
        out.push_back(kv("svMaskFmt", static_cast<uint64_t>(RenderIntegration::g_svs_mask_fmt)));
        // 26213 SEAM ORDERING. Per frame, which came first for the volume-class
        // depth target: a bind WITH a render target (the prepass) or a
        // depth-only bind (the volume pass). The seam needs the former first.
        // If svOrdOnlyFirst dominates, our frame boundary is falling between
        // the prepass and the volume pass, the with-RTV set is empty when the
        // volume binds arrive, and no seam can fire - which would explain a
        // 1.6% eligible rate against a volume pass that runs every frame.
        out.push_back(kv("svOrdRtvFirst", RenderIntegration::g_svs_ord_rtv_first));
        out.push_back(kv("svOrdOnlyFirst", RenderIntegration::g_svs_ord_only_first));
        // ===============================================================
        // 26215 PRE-RESOLVE SNAPSHOT. Read in this order.
        //
        // svCopyFrameMax IS THE CENSUS THIS BUILD EXISTS TO SETTLE. It is
        // the peak number of pre-resolve copies in a single frame. The
        // design wants exactly 1: the trigger is "volume-class depth
        // target AND the render target IS the adopted mask", which should
        // name the stencil resolve and nothing else. At 1, pre is
        // cascade-only and the ratio is the intended snapshot. Above 1 the
        // last copy of the frame still wins and the feature still works,
        // but each one is a full-screen copy (8.3 MB at 3840x2160) and the
        // discriminator wants narrowing - which is then a measurement in
        // hand rather than an argument. At 0 nothing ever fired: check
        // svMaskAdopts first, because the trigger needs an adopted mask
        // identity to compare against and cannot fire before one exists.
        //
        // svCopyArms / svCopyMade / svCopyDrops are the 26211 triple in
        // its second use: armed at the bind, performed at the next draw,
        // and still outstanding at the frame boundary. Arms and made must
        // track; drops climbing means no draw followed the resolve bind
        // and the deferral point is wrong (mode 76 is the in-hook A/B).
        //
        // svCopySkips and svCopyFails are the two bail paths, split
        // because they mean different things: skips = the trigger fired
        // with no usable destination (no mask SRV, no device, the source
        // resource refused), fails = the destination texture or its SRV
        // was refused outright. Every silent bail gets a counter - 26208.
        //
        // svPreW/H/Fmt is the destination's shape and MUST equal
        // svMaskW/H/Fmt; a mismatch means the clone missed a rebuild.
        // svPreBinds is the shader-side liveness - if it stays 0 while
        // svCopyMade climbs, the snapshot is being taken and never read,
        // and the paint will show nothing.
        //
        // svRtvSetWipes counts clears of the persistent with-RTV set that
        // did NOT come from a device reset. Under the shipped default it
        // must be 0. Non-zero with no debug mode set means something is
        // re-enacting the 26214 fault.
        // ===============================================================
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
        // 26218 THE BRACKET. svPostMade must track svCopyMade one for one:
        // the pre half fires at the resolve draw, the post half at the draw
        // after it, so a shortfall means no draw followed the resolve and the
        // second half never landed. svCopyDrops counts brackets left
        // incomplete at the frame boundary and is the same lane for the same
        // question. svPostBinds is the shader-side liveness for t22.
        //
        // WHY THE BRACKET EXISTS: reading post from the LIVE mask made the
        // ratio depend on every writer between the resolve and our draw, and
        // the one that mattered was OURS - mask_cast_engine paints our
        // meshes' shadows into that same mask so the world receives them,
        // late in the frame. The ratio reported our box's own cast shadow as
        // a unit shadow, and it survived walking the soldier out of range,
        // which is what proved it was never a stencil term. Mode 76 restores
        // the live-post read for the A/B.
        out.push_back(kv("svPostMade", RenderIntegration::g_svs_post_made));
        out.push_back(kv("svPostSkips", RenderIntegration::g_svs_post_skips));
        out.push_back(kv("svPostBinds", RenderIntegration::g_svs_post_binds));
        // 26220 DEMAND GATE. svSnapGated counts frames where the trigger armed
        // and no mode wanted the snapshots, so no copy was issued. On the
        // default path it should equal svCopyArms and svCopyMade should be 0 -
        // i.e. the bracket costs nothing until something reads it. Under any
        // of modes 73/74/75/76/78/79 it stops climbing and svCopyMade takes
        // over one for one.
        // 26233 svSnapGated IS RETIRED AS AN INSTRUMENT. It counts frames where
        // the bracket armed and nothing consumed the snapshots. The demand gate
        // is now permanently open, so it reads 0 forever and its 26221
        // confirmation (svCopyArms == svCopyMade + svSnapGated, 4591 = 3884 +
        // 707) can never be re-measured except under mode 87. svCopyArms stays
        // UNGATED, so the trigger census itself survives. Noted here rather
        // than discovered later in a dump that looks healthy for the wrong
        // reason.
        out.push_back(kv("svSnapGated", RenderIntegration::g_svs_snap_gated));
        // 26221 THE INJECTED BOX'S RASTERIZATION. svInjVpLo/Hi are the depth
        // range the injection actually wrote with and are THE number this
        // build exists to change: they must read 0.011 / 0.999, matching
        // injDpVpMin/Max and trigAccMin/Max, and NOT 0 / 1. If they read
        // 0 / 1 with no debug mode set, the sibling's source is not reaching
        // the injector. svInjVpRejects must be 0 - a non-zero reading means
        // trig_vp is being poisoned and the fallback is carrying the build.
        // svInjVpReverts climbs only under mode 80.
        //
        // svInjRebases + svInjAbs == svInjects, always. svInjAbs must be 0 on
        // the shipped path: it counts injections that rasterized in absolute
        // world space, which is either mode 81 or kh_rebase_vp refusing a
        // sub-metre camera, and the second one is a wrong-space draw.
        // 26222 GAUGE FIX. 26221 published these through kv, which takes
        // uint64_t - so 0.011 and 0.999 BOTH truncated to 0 and the lane that
        // existed to prove the depth range read 0/0, a value no code path can
        // produce. The instrument was wrong, not the fix (73 vs 80 differed in
        // the field, which is the range doing exactly what it should). kvf is
        // the float lambda and is what every other float lane in here uses.
        out.push_back(kvf("svInjVpLo", RenderIntegration::g_svs_inj_vp_lo));
        out.push_back(kvf("svInjVpHi", RenderIntegration::g_svs_inj_vp_hi));
        out.push_back(kv("svInjVpRejects", RenderIntegration::g_svs_inj_vp_rejects));
        out.push_back(kv("svInjVpReverts", RenderIntegration::g_svs_inj_vp_reverts));
        out.push_back(kv("svInjRebases", RenderIntegration::g_svs_inj_rebases));
        out.push_back(kv("svInjAbs", RenderIntegration::g_svs_inj_abs));
        // 26222 RASTERIZER STATE. svInjRsSets + svInjRsInherit == svInjects.
        // svInjRsInherit must be 0 on the shipped path: non-zero with no debug
        // mode means rast_sun is null, i.e. the injection is still rasterizing
        // with the engine's volume-pass state and its depth is clamped rather
        // than clipped. Climbs only under mode 82.
        out.push_back(kv("svInjRsSets", RenderIntegration::g_svs_inj_rs_sets));
        out.push_back(kv("svInjRsInherit", RenderIntegration::g_svs_inj_rs_inherit));
        // 26223: split out of svInjRsInherit, which reported two different
        // failures through one lane. svInjRsSets + svInjRsInherit + svInjRsNull
        // == svInjects. svInjRsNull must be 0 in every dump, mode or no mode.
        out.push_back(kv("svInjRsNull", RenderIntegration::g_svs_inj_rs_null));
        // 26230 MASK PRIMING - THE MASKING FIX. svPrimeDraws must equal
        // svCopyMade: every frame that takes a pre snapshot must have primed
        // first, or that frame still has the old guard behaviour and its blue
        // band. The three refusal lanes are deliberately separate because they
        // mean different things and one merged lane would hide which:
        //   svPrimeNoRtv     - the engine mask RTV was not captured yet. Expect
        //     a small count from session start (it is captured at the engine's
        //     own resolve bind, which is LATER in the frame than the priming
        //     pass); if it keeps climbing, mask_cast_engine is not capturing.
        //   svPrimeMismatch  - the captured RTV is not the resource we snapshot.
        //     MUST be 0. Non-zero means we would have primed a buffer nobody
        //     reads while leaving the real mask unprimed, silently.
        //   svPrimeNoState   - the blend or depth-stencil state was refused by
        //     the device. MUST be 0; non-zero degrades to 26229 behaviour.
        //   svPrimeNoShader  - 26232, SPLIT OUT of svPrimeNoState: PSMaskPrime
        //     did not compile, or the input layout / VS is absent. A different
        //     failure with a different fix, and it was sharing a lane.
        //   svPrimeFailFrameCb - 26232, ALSO split out of svPrimeNoState: the
        //     frame CB upload was refused, so nothing was drawn. This one was
        //     not even in svPrimeNoState's documented meaning.
        //   svPrimeNoList    - 26232, split out of svPrimeNoView: the transform
        //     WAS cached and the caster list was empty, which is a different
        //     frame from "the injection never ran".
        //   svPrimeFailObjCb - 26232: object CB refused mid-loop. Was a silent
        //     break, so a partly primed footprint read as a whole one.
        //   svPrimeNoVb      - 26232: a caster's mesh VB slot was absent.
        // svPrimeReverts climbs only under mode 86, and it is stamped by the
        // PUMP now - kh_svs_prime_wanted owns the revert decision, and
        // kh_svs_prime_mask no longer keeps a second copy of it.
        //
        // svPrimeDraws + svPrimeReverts == svCopyMade is the exact invariant.
        // 26230 documented it as svPrimeDraws == svCopyMade, which is only true
        // with no mode set.
        // svPrimeDrawMeshes is the svInjectDraws analogue and must track
        // svInjectDraws one for one: the two passes walk the same caster list.
        //
        // 26232 THE LANDING GAUGE, AND IT IS THE POINT OF THIS BUILD.
        // svPrimeDraws proves the priming pass was ISSUED. It says nothing
        // about whether one pixel survived LESS_EQUAL against our injected
        // depth - and 26230 was exactly that: every refusal lane flat zero, the
        // pass demonstrably running, the artifact untouched, because the pass
        // was in the wrong place. An occlusion query now wraps the priming
        // draws:
        //   svPrimePixels    - samples that passed, most recent read
        //   svPrimePixelsMin - low-water mark across the window
        //   svPrimeZero      - reads that came back EMPTY. MUST BE 0. Non-zero
        //     means the prime is rasterizing where the depth test rejects it,
        //     pre is carrying the background verdict again, and the blue band
        //     is back with nothing else in the dump to say so.
        //   svPrimeOcclReads / svPrimeOcclSkips - ring liveness. Skips are
        //     slots still in flight, never reused hot; a climbing skip count
        //     means the ring is too shallow, not that the prime failed.
        // This replaces a screenshot with a number, which matters because after
        // the scaffolding strip there is no mode 78 left to look at.
        //
        // THE ACCEPTANCE TEST IS VISUAL AND IS MODE 78: the blue band (post 0,
        // pre 0 - the guard firing) must be GONE, because pre is now 1.0 at our
        // pixels by construction. Mode 71 is the reach gauge - it paints the
        // live mask, so any bleed beyond our footprint shows there.
        out.push_back(kv("svPrimeDraws", RenderIntegration::g_svs_prime_draws));
        out.push_back(kv("svPrimeNoRtv", RenderIntegration::g_svs_prime_no_rtv));
        out.push_back(kv("svPrimeMismatch", RenderIntegration::g_svs_prime_mismatch));
        out.push_back(kv("svPrimeNoState", RenderIntegration::g_svs_prime_no_state));
        out.push_back(kv("svPrimeReverts", RenderIntegration::g_svs_prime_reverts));
        // 26231: svPrimeNoView counts the priming pass finding no cached
        // transform or an empty caster list - i.e. the bracket armed on a
        // frame the injection never ran. MUST be 0 whenever svCopyMade is
        // climbing; non-zero means the two are firing on different frames.
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
        // 26232 THE SHIPPED FALLBACK'S OWN GAUGES. kh_svs_sten_ready is the
        // only path by which the multiply can be skipped once the modes are
        // gone, so its two new refusals are published rather than inferred.
        //   svStenRejCold - readiness refused because no post snapshot had
        //     landed yet. Expect a handful at session start; it is the window
        //     in which the clones exist and hold uninitialised memory, which
        //     26218 left open for a whole build.
        // BOTH LANES ARE CALL-SCOPED, NOT FRAME-SCOPED: kh_svs_sten_ready is a
        // predicate with a side effect now, and the CB fill asks it twice per
        // pass (once for maskMeta.z, once for maskMeta.w) on each of the
        // injection and flush editions. Expect a small integer multiple of the
        // frame count, the way svMaskHits reads 2x svInjects. The 26201 trap -
        // a probe that SPENT the one lane its own A/B needed to read - does not
        // apply here because nothing reads these during an A/B; they exist to
        // explain a mesh that lost its shadows after the modes are gone.
        //   svStenRejDims - the mask's shape disagrees with fxMeta.zw, the
        //     clamp bound every mask Load goes through. MUST be 0. Non-zero
        //     means a resolution change moved the CB and left the clones
        //     behind, and every Load is landing on the wrong texel.
        out.push_back(kv("svStenRejCold", RenderIntegration::g_svs_sten_rej_cold));
        out.push_back(kv("svStenRejDims", RenderIntegration::g_svs_sten_rej_dims));
        // 26232 injector tessellation parity. svInjTessNulls == svInjects on
        // the shipped path; svInjTessInherit climbs only under mode 89.
        // 26234 THE REGISTRATION GAUGE, and it is the number that closes the
        // silhouette-strip question. svReprojPx is the screen-space distance,
        // in pixels, between where the object centre lands under the LIVE pass
        // view and where it landed under the view the INJECTION adopted. The
        // mask, the snapshots and our injected depth are all registered to the
        // latter; the mesh is drawn with the former.
        //   svReprojPx    - last frame's delta
        //   svReprojPxMax - worst in the window. THIS is the one to read. If it
        //     tracks the observed strip width under rotation and falls to ~0
        //     when still, the diagnosis is arithmetic rather than argument.
        //     A large value with mode 90 set and a clean edge with it unset is
        //     the complete proof.
        //   svReprojBehind - centre behind one of the two eyes, so no delta was
        //     computable. Expect 0 unless the camera is inside the mesh.
        // NOTE the delta is NOT the same thing as the error after the fix: with
        // reprojection on, this lane keeps reporting how far apart the two views
        // were, which is exactly what we want to keep watching. It is a measure
        // of the HAZARD, not of a remaining defect.
        out.push_back(kvf("svReprojPx", RenderIntegration::g_svs_reproj_px));
        out.push_back(kvf("svReprojPxMax", RenderIntegration::g_svs_reproj_px_max));
        out.push_back(kv("svReprojBehind", RenderIntegration::g_svs_reproj_behind));
        // 26235 THE SNAPSHOT EPOCH, and it is the number this build exists for.
        // svPostAgeFrames / svPostAgeMax count seam frame boundaries between the
        // post snapshot being written and the mesh consuming it.
        //   0 - same frame. Everything since 26215 has assumed this.
        //   1 - the mesh shades against the PREVIOUS frame's mask. That is a
        //       full frame of camera motion of screen-space misregistration:
        //       constant in pixels under rotation, scaling with speed, absent
        //       when still - the observed strip, exactly. It would also explain
        //       why reprojecting against the CURRENT injection matrix made
        //       things worse rather than better.
        //   >1 - the bracket is landing on some frames only; cross-check
        //       svCopyDrops and svFrameDupes.
        // svReprojPx also reports for the first time in this build: at 26234 it
        // gated on a FRAME-SCOPED flag that the seam boundary clears before the
        // mesh draws, so it early-returned every frame and published its -1
        // sentinel. That was a gauge bug, not a zero measurement.
        out.push_back(kv("svPostAgeFrames", RenderIntegration::g_svs_post_age_last));
        out.push_back(kv("svPostAgeMax", RenderIntegration::g_svs_post_age_max));
        // 26236 EPOCH-MATCHED REPROJECTION. svPostAgeMax measured 1 at 26235:
        // the mesh reads a mask one seam-frame older than itself, so the lookup
        // now uses the transform stamped with the SNAPSHOT's epoch rather than
        // the newest one.
        //   svReprojEpochHits - the snapshot's epoch was found in the ring.
        //     Should track svPostMade. This is the lane that says the fix is
        //     actually engaged.
        //   svReprojEpochMiss - not found: cold start, a dropped bracket, or an
        //     age beyond the 4-deep ring. Falls back to the raster position, so
        //     a miss is a strip, never a wrong answer. A CLIMBING miss count
        //     means the ring is too shallow or svPostAgeMax has moved.
        //   svReprojWild - a computed delta wider than the screen diagonal,
        //     rejected. At 26235 the max lane published 1.27e+06 px, which was
        //     a near-zero w as the camera crossed the mesh, not a measurement.
        //     The gauge now guards at w > 0.05 against a 0.07 near plane.
        out.push_back(kv("svReprojEpochHits", RenderIntegration::g_svs_reproj_epoch_hits));
        out.push_back(kv("svReprojEpochMiss", RenderIntegration::g_svs_reproj_epoch_miss));
        out.push_back(kv("svReprojWild", RenderIntegration::g_svs_reproj_wild));
        // 26247 svEngVpTakes - composite draws that borrowed the injection's
        // (= the engine's) transform under mode 107. 0 on the default path.
        // 26264: renamed from svEngVpTakes. The matrix is the INJECTION's, not
        // the engine's - see the ledger at g_svs_inj_vp_abs. Mode 107 draws the
        // visible mesh with it as a forensic A/B for how far the injection sits
        // from the engine; it is NOT a candidate fix.
        out.push_back(kv("svInjVpTakes", RenderIntegration::g_svs_inj_vp_takes));
        // 26240 THE DELTA CENSUS. svReprojPxMax is a session maximum and one
        // relock transient owns it forever - at 26237 I called the registration
        // hypothesis dead on svReprojPx 0.139 (the last frame, settled) while
        // the max lane read 1077 in the same dump. Neither answers the only
        // question that matters: do the injection view and the visible-draw view
        // diverge on a TYPICAL MOVING frame?
        //   svReprojFrames - frames the delta was computable
        //   svReprojOver4 / svReprojOver32 - of those, how many exceeded 4 px
        //     and 32 px. A handful out of several hundred means the max is a
        //     transient and registration is genuinely fine. A count that tracks
        //     the frames spent moving means it is not.
        //   svReprojPxMean - the average, which no single-sample lane can give.
        out.push_back(kv("svReprojFrames", RenderIntegration::g_svs_reproj_frames));
        out.push_back(kv("svReprojOver4", RenderIntegration::g_svs_reproj_over4));
        out.push_back(kv("svReprojOver32", RenderIntegration::g_svs_reproj_over32));
        // 26241 THE VIEW SWING - screen-space distance between our caster's
        // centre under the view the injection ADOPTS and under g_ro.cycle_pv,
        // the pre-26224 view that mode 83 restores. Measured at the injection,
        // both matrices absolute, one projection per injection.
        //   svSwingPxMean / svSwingOver4 / svSwingOver32 / svSwingPxMax
        // If the mean is the size of the observed strip, 26224's adoption is the
        // variable and mode 83 will move the strip. If it is ~0, adoption is
        // exonerated and the coverage mismatch comes from somewhere else - which
        // is worth knowing before another build rather than after one.
        //
        // WHY THIS EXISTS: svReprojPxMean 25.17 with Over4 520 of 980 says the
        // injection view and the visible-draw view diverge by tens of pixels on
        // a typical moving frame. I closed that hypothesis at 26237 on the
        // last-frame value of 0.139 px and was wrong. Reprojection still did not
        // fix the strip because it aligns the LOOKUP while the mask's CONTENT is
        // produced in the ENGINE's volume-pass space - a coverage disagreement no
        // lookup transform can reach. The rasterizer ledger states the sim-thread
        // PV matrix runs one frame ahead of the depth buffer; this lane measures
        // how much of that reaches our footprint.
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
        // 26238 THE MASK CLAMP BOUND, never published before this build. Every
        // mask read goes through KhMaskPx, which clamps to fxMeta.zw. That field
        // is written TWICE per object in BOTH CB editions and the second write
        // uses g_res.comp_depth_w/h - the occlusion-guard snapshot's size -
        // rather than the mask's. 26221 added the first write and left the
        // second in place, so its own ledger entry is only half true.
        //   fxDimW / fxDimH - what ACTUALLY reached the GPU on the last fill.
        //     THESE MUST EQUAL svMaskW / svMaskH. If they do not, every mask,
        //     pre and post Load is clamped to the wrong bound and a band of
        //     pixels reads one repeated texel, which looks exactly like a strip.
        //   compDepthW / compDepthH - the guard snapshot's own size.
        //   fxDimMismatch - object fills where the two disagreed. 0 means this
        //     is only untidy; non-zero means mode 97 is the fix and the strip
        //     has a cause that is neither registration nor snapshot epoch.
        //   fxDimForced - climbs only under mode 97.
        out.push_back(kv("fxDimW", static_cast<uint64_t>(RenderIntegration::g_fx_dim_w)));
        out.push_back(kv("fxDimH", static_cast<uint64_t>(RenderIntegration::g_fx_dim_h)));
        out.push_back(kv("compDepthW", static_cast<uint64_t>(RenderIntegration::g_res.comp_depth_w)));
        out.push_back(kv("compDepthH", static_cast<uint64_t>(RenderIntegration::g_res.comp_depth_h)));
        out.push_back(kv("fxDimMismatch", RenderIntegration::g_fx_dim_mismatch));
        out.push_back(kv("fxDimForced", RenderIntegration::g_fx_dim_forced));
        // 26239 DEFECT 6 - THE INJECTION WAS CLIPPING WHERE THE VISIBLE DRAW
        // CLAMPS. Convicted by pixel history, not inferred: at a strip pixel the
        // injection draw is absent from the volume depth history entirely, while
        // it passes at a correct pixel and at a lit one. rast_sun carries
        // DepthClipEnable TRUE - justified in its own ledger by the private sun
        // map's fitted ortho volume, which does not apply to the engine's
        // perspective volume buffer - and the three mesh rasterizers the visible
        // draw uses run FALSE deliberately. So a near-plane-crossing fragment
        // survives the visible draw and vanishes from the injection.
        //   svInjRsClamp - injections through rast_inject (the fix). Should
        //     equal svInjects on the shipped path.
        //   svInjRsClip  - through rast_sun: mode 98, or rast_inject failed to
        //     create. MUST be 0 with no mode set.
        // svPrimePixels is the confirmation lane - the priming pass covers the
        // injected footprint, so it must RISE with the fix and fall under 98.
        out.push_back(kv("svInjRsClamp", RenderIntegration::g_svs_inj_rs_clamp));
        out.push_back(kv("svInjRsClip", RenderIntegration::g_svs_inj_rs_clip));
        out.push_back(kv("svInjTessNulls", RenderIntegration::g_svs_inj_tess_nulls));
        out.push_back(kv("svInjTessInherit", RenderIntegration::g_svs_inj_tess_inherit));
        // 26224 SAME-FRAME VIEW ADOPTION. svInjViewAdopts + svInjViewRefused +
        // svInjViewReverts == svInjects. svInjViewAdopts should track the
        // composite's own viewAdopts; svInjViewRefused counts injections that
        // still ran on the cycle view because no fresh frame view existed, and
        // those are the frames where the duplicate can still appear. If it is
        // large while viewAdopts is healthy, the injector is being called
        // outside the window the adoption is valid in and the ARM point moves,
        // not the call. svInjViewReverts climbs only under mode 83.
        out.push_back(kv("svInjViewAdopts", RenderIntegration::g_svs_inj_view_adopts));
        out.push_back(kv("svInjViewRefused", RenderIntegration::g_svs_inj_view_refused));
        out.push_back(kv("svInjViewReverts", RenderIntegration::g_svs_inj_view_reverts));
        // ===================================================================
        // 26249 THE VOLUME TRANSPORT. Read the stencil where the engine left it
        // instead of laundering it through the engine's screen-space mask.
        // Ledger at kh_svs_vol_ensure; mode 109 selects it, 110/111 paint it,
        // 112 reverts the footprint test. Default is unchanged at this build.
        //
        // THE ACCEPTANCE SET, and what each failure means:
        //   svStenSrc      - WHICH TRANSPORT IS LIVE. 0 = the mask ratio,
        //     1 = the volume stencil. Read this FIRST: every other lane below
        //     is meaningless if the transport never armed, and a 0 under mode
        //     109 means readiness refused, not that the read is wrong.
        //   svVolFmt       - the SOURCE volume buffer's format. MUST read 45
        //     (D24_UNORM_S8_UINT). 44 is R24G8_TYPELESS and also fine; anything
        //     else and svVolRejFmt will be non-zero.
        //   svVolSamples   - the SOURCE's sample count. MUST read 1. THIS IS
        //     THE LANE THE 26248 HANDOFF DID NOT HAVE: it cited shadowTex3Samp,
        //     which belongs to the 26202 SHADOW DEPTH-TARGET census and is a
        //     claim about a different resource. This one reads the latched
        //     volume texture's own desc. MSAA makes the copy illegal outright
        //     and svVolRejMs counts the refusal rather than issuing it.
        //   svVolW / svVolH - the COPY's dimensions. Must equal svInjW/svInjH
        //     and the main depth, because the raster fallback hands this
        //     transport a position in composite screen space. svVolRejDims is
        //     the refusal if they ever disagree - the transport declines rather
        //     than reading a scaled texel, which is the fxMeta.zw clamp fault
        //     this feature already paid for once.
        //   svVolCopyMade  - copies landed. Should track svCopyArms under mode
        //     109 and be 0 on the default path (the copy is demand-gated on the
        //     mode; 33 MB a frame is not spent without a consumer).
        //   svVolCopyFails - the copy was wanted and the destination could not
        //     be made. MUST be 0.
        //   svVolCopySkips - armed with no source latched or no consumer. Large
        //     on the default path BY DESIGN; that is the demand gate working.
        //   svVolCreateFails / svVolRejMs / svVolRejShape / svVolRejFmt - the
        //     four ways the destination is refused, split per cause. "MSAA" and
        //     "CreateTexture2D failed" are different failures and a single lane
        //     covering both is how three campaigns measured a path control
        //     never took.
        //   svVolRejCold   - readiness refused because no copy had landed into
        //     the resources that exist NOW. Expect a small non-zero at session
        //     start and after every device reset; a climbing value means the
        //     bracket is not reaching the copy.
        //   svVolStenBinds - t23/t24 bind pairs. Must be non-zero whenever
        //     svStenSrc is 1, and it is the lane that separates "the shader had
        //     no textures" from "the shader read the wrong thing".
        //   svVolLatches / svVolLatchDrops - the AddRef'd source. Latches should
        //     be small (one per device, one per identity change); Drops
        //     climbing means the engine is rotating volume buffers and the
        //     copy's shape is being rebuilt behind it.
        //
        // TWO LANES THAT EXIST FOR THE NEXT BUILD, NOT THIS ONE:
        //   svOmRtvMax - the peak count of SIMULTANEOUS render targets bound at
        //     the bracket. kh_volume_seam_inject and kh_svs_prime_mask both
        //     save and restore only FOUR, and OMSetRenderTargets NULLS every
        //     slot it does not name - so if this reads above 4, both of those
        //     passes have been silently dropping an engine bind since 26221 and
        //     that is a real defect with a one-line fix in three places.
        //     g_svs_rtv_set_max sounds like this lane and is NOT: it counts
        //     distinct DSV identities seen with a render target. The volume copy
        //     already saves all eight.
        //   svVolArmMax - per-frame peak of class-0-depth-plus-render-target
        //     binds. THE BRACKET STILL ARMS ON A COMPARE AGAINST THE ADOPTED
        //     MASK, so the new transport cannot fire until mask discovery has
        //     succeeded - which contradicts its own premise and must go before
        //     26251 deletes the mask apparatus. If this reads 1, the identity
        //     compare is redundant and the arm can drop it safely. If it reads
        //     9 (eight cascades plus the resolve), it cannot, and the arm needs
        //     a different property. Measure first, then move the trigger.
        // ===================================================================
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
        // 26277 THE WALK'S OWN LANES.
        //   svVolCode - the arm the SHADER ran, read off the value actually
        //     written into stenReprojCam.w: 0 reprojection (126), 1 raster only
        //     (129, the 26273 revert), 2 walk + signed count (the default),
        //     3 walk with the historic unsigned count (130), 4 signed count
        //     with no walk (131). This is deliberately NOT the debugMode: when
        //     the epoch ring misses there is no far endpoint for the walk and
        //     the fill demotes 2 -> 4 on its own. A mode-named counter that
        //     cannot report a demotion is what made the 108 dump unreadable.
        //   svVolWitFrames / svVolWitAbsent - frames published with and without
        //     a footprint witness. Acceptance for the walk is WitFrames ==
        //     flushes with WitAbsent 0; any climb in WitAbsent means the ring
        //     is missing and the slice will be back on those frames, so read
        //     this BEFORE concluding anything from the screen.
        out.push_back(kv("svVolCode", RenderIntegration::g_svs_vol_code));
        out.push_back(kv("svVolWitFrames", RenderIntegration::g_svs_vol_wit_frames));
        out.push_back(kv("svVolWitAbsent", RenderIntegration::g_svs_vol_wit_absent));
        // 26278 THE SHARED-VIEW ARM (mode 133).
        //   svShareTakes - seam frames that adopted the colour injection's own
        //     view. Should equal svInjects under 133 and be 0 otherwise. If it
        //     reads 0 under 133 the pass ordering assumption is wrong and
        //     svShareStale says so; nothing silently half-applies.
        //   svSharePubs / svShareStale / svShareAbsent - published, wrong frame,
        //     never published. Stale climbing means the seam runs BEFORE the
        //     colour injection and the arm has to be inverted, which is a
        //     reading and not a guess.
        //   DO NOT USE svReprojPxMean AS THE BAR. The 127 dump falsified it: the
        //     slice went away while that lane went UP, 18.96 to 39.72. It is
        //     computed in the colour injection against g_svs_prime_vp, which the
        //     SEAM writes later in the frame, so it reports a whole frame of
        //     camera motion and not the gap it has been read as since 26273.
        //     svShareTakes vs svShareStale settles that ordering in one dump.
        out.push_back(kv("svSharePubs", RenderIntegration::g_svs_share_pubs));
        out.push_back(kv("svShareTakes", RenderIntegration::g_svs_share_takes));
        out.push_back(kv("svShareStale", RenderIntegration::g_svs_share_stale));
        out.push_back(kv("svShareAbsent", RenderIntegration::g_svs_share_absent));
        // 26283 THE INVERTED SHARE (mode 134). Acceptance is svSeamViewTakes ==
        // compositeInjections with Stale 0. A capture measured the footprints
        // directly: colour translated RIGHT of seam by 18-34 px, slice 33078 px
        // at background depth with stencil 0 - a genuine hole. If this arm works
        // that offset goes to zero and the hole with it.
        out.push_back(kv("svSeamViewPubs", RenderIntegration::g_svs_seam_view_pubs));
        out.push_back(kv("svSeamViewTakes", RenderIntegration::g_svs_seam_view_takes));
        out.push_back(kv("svSeamViewStale", RenderIntegration::g_svs_seam_view_stale));
        out.push_back(kv("svSeamViewAbsent", RenderIntegration::g_svs_seam_view_absent));
        //   svSeamViewAge / Max - frames between the seam's publication and the
        //     colour pass reading it. MUST be 1: the frame-seq boundary fires at
        //     the resolve, which sits between the two passes. 26283 tested for 0
        //     and refused 712 of 712. If this ever reads above 1 the passes have
        //     drifted apart and the adoption is carrying stale geometry.
        out.push_back(kv("svSeamViewAge", RenderIntegration::g_svs_seam_view_age_last));
        out.push_back(kv("svSeamViewAgeMax", RenderIntegration::g_svs_seam_view_age_max));
        //   svSeamRotTakes - 26296: the DEFAULT path now (135 is an alias);
        //     engagement bar is ~svInjects. Frames where the seam BASIS was
        //     adopted and the translation row rebuilt from this pass's own
        //     camera. Should equal svSeamViewTakes under 135 and be 0 under 134.
        //   svSeamRotCamDx - metres between the camera 134 would have inherited
        //     and the one 135 keeps. THIS IS THE DRIFT 134 SHOWED, in metres;
        //     at camStepMaxM 2.6 expect roughly that under motion. Non-zero here
        //     with a stable box is the whole point of 135.
        out.push_back(kv("svSeamRotTakes", RenderIntegration::g_svs_seam_rot_takes));
        //     26291: now filled EVERY mode by the unconditional gauge in the
        //     colour pass (it was 135/136-gated, so mode 0 - where the slice
        //     lives - never produced it; method note 32, gauge placement).
        out.push_back(kvf("svSeamRotCamDx", RenderIntegration::g_svs_seam_rot_cam_dx));
        //   svSeamRotCamDxMax - THE lane 135 needed and did not have. Last-value
        //     only read 0 on a settled final frame, which is how 135 shipped on
        //     the belief that translation did not matter. It does: 134 matched
        //     both terms and svReprojPxMean went to 0; 135 matched rotation only
        //     and it returned to 26.1.
        //   svSeamTrnTakes - mode 136: translation shared, rotation kept.
        out.push_back(kvf("svSeamRotCamDxMax", RenderIntegration::g_svs_seam_rot_cam_dx_max));
        //   26342 THE ROUND-TRIP GAUGES (ledger at kh_view_camera_exact).
        //   Snap = takes whose adopted basis was bit-identical to ours, so the
        //   translation row was left alone - at rest this should track
        //   svInjects, and it is the arm that removes the stationary offset.
        //   Exact = takes rebuilt through the honest 3x3 inverse. Refused =
        //   degenerate basis, our own view kept. CamRtMax = the worst
        //   |transpose camera - exact camera| the session saw, in metres: the
        //   size of the error 26341 was injecting every frame it rebuilt.
        //   26344: the near-gap ramp's CPU half and the dark-publish gate.
        //   nearzRampFills should track nearzGapDraws; blkDarkPubRefusals
        //   counting outside a cold start means the bar is too tight.
        out.push_back(kv("nearzRampFills", RenderIntegration::g_nearz_ramp_fills));
        out.push_back(kv("blkDarkPubRefusals", RenderIntegration::g_blk_dark_pub_refusals));
        // 26379: the deepest single-publish collapse ever ACCEPTED, and how
        // many publishes that is over. Sizes the gate's bar.
        // 26406 THESE TWO WERE TRUNCATED TO INTEGER FROM THE DAY THEY WERE
        // ADDED, AND THAT IS WHY HANDOFF 50 SECTION 3.4 IS STILL BLOCKED.
        // kv() above takes uint64_t; kvf() takes float. Both of these are
        // float RATIOS in roughly [0, 3], so kv() published (uint64_t)0.87
        // == 0 on every dump since 26379 - and the -1.0f sentinel through an
        // unsigned conversion is undefined behaviour on top. Section 3.4
        // says "get that number, then set the bar between it and 2.79. Do
        // not move the bar before it exists." It has existed since 26379.
        // It has been reading 0 because of a lambda choice, not because the
        // deepest accepted collapse is zero, and blkAcceptRatioN 1885 in the
        // same dump proves the accumulator itself was running the whole
        // time. 28 other metric lanes had the same defect - full list and
        // the reason they were never caught at KH_STATS_KVF_AUDIT.
        // 26417 THE SHADE LATCH (ledger at KH_BLK_SHADE_LATCH). What the SHADER
        // WAS HANDED, latched at the session's darkest frame - not what was
        // published, refused or believed. 26416 proved the black box is not on
        // the publish path, so these are the lanes to read on the next event and
        // the blkDark* family is not.
        // 26418 ENGAGEMENT LANE for the zero-sun refusal. 0 means the gate never
        // fired and any null result on the black box is void.
        out.push_back(kv("blkZeroSunRefusals", RenderIntegration::g_blk_zero_sun_refusals));
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
        out.push_back(kvf("blkShadeProbeAgeS", RenderIntegration::g_blk_shade_probe_age));
        out.push_back(kvf("blkShadeT", RenderIntegration::g_blk_shade_t));
        out.push_back(kvf("blkAcceptRatioMin", RenderIntegration::g_blk_accept_ratio_min));
        out.push_back(kvf("blkAcceptRatioAmb", RenderIntegration::g_blk_accept_ratio_amb));
        out.push_back(kv("blkAcceptRatioN", RenderIntegration::g_blk_accept_ratio_n));
        // 26380: the colour pass taking the seam's own publication.
        // compShareStale must stay near 0 - if it climbs, the seq-adjacency
        // assumption is wrong and this fold is unsafe.
        out.push_back(kv("compShareTakes", RenderIntegration::g_comp_share_takes));
        out.push_back(kv("compShareStale", RenderIntegration::g_comp_share_stale));
        out.push_back(kvf("blkDarkPubSl", RenderIntegration::g_blk_dark_pub_sl));
        out.push_back(kvf("blkDarkPubRefSl", RenderIntegration::g_blk_dark_pub_ref_sl));
        out.push_back(kv("svSeamRotSnap", RenderIntegration::g_svs_seam_rot_snap));
        out.push_back(kv("svSeamRotExact", RenderIntegration::g_svs_seam_rot_exact));
        out.push_back(kv("svSeamRotRefused", RenderIntegration::g_svs_seam_rot_refused));
        out.push_back(kvf("svCamRtMax", RenderIntegration::g_svs_cam_rt_max));
        // KH_JITTER_FLOOR (26421; full ledger in rendering_integration.hpp,
        // beside kh_rebase_vp_exact). READ jitRebaseMm FIRST - it is the
        // POSITIVE CONTROL for the close-range jitter fix, and a null on the
        // artifact is void while it reads 0 (rule 1.21). Then read
        // jitCamUlpMm beside it: that is the part of the jitter the fix does
        // NOT close, and if it dwarfs the gain the fix cannot have been the
        // whole artifact. jitCamHolds is only meaningful while jitBasisWit is
        // non-zero - a pure-translation session leaves it NOT MEASURED, not
        // falsified. EVERY lane here is derived from pv.view, so none of them
        // can say whether the ENGINE's camera steps too; that needs the b2
        // block and it is the named next instrument.
        // kvf, not kv - these are sub-millimetre quantities and 1.24 is the
        // reason this note exists.
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
        // The worst-frame latch (rule 1.26): the whole set AT the largest
        // jitRebaseMm, not at the last frame. jitLatchT is effect time, so a
        // latch older than the trace ring is still readable here - which is
        // the entire point of latching rather than sampling.
        out.push_back(kvf("jitLatchMm", RenderIntegration::g_jit_latch_mm));
        out.push_back(kvf("jitLatchUlpMm", RenderIntegration::g_jit_latch_ulp_mm));
        out.push_back(kvf("jitLatchCamAbsM", RenderIntegration::g_jit_latch_cam_abs));
        out.push_back(kvf("jitLatchStepMm", RenderIntegration::g_jit_latch_step_mm));
        out.push_back(kvf("jitLatchBasis", RenderIntegration::g_jit_latch_basis));
        out.push_back(kvf("jitLatchT", RenderIntegration::g_jit_latch_t));
        // KH_ENGCAM_LOCATOR (26440; full ledger before the seam inject).
        // READ ORDER: engCamDxMaxMm FIRST - millimetres (0 to ~0.5) is a
        // correct lock, METRES is a wrong row and every other lane is then
        // about the wrong value. Then engCamTakes, the arm's positive
        // control: 0 under mode 236 means the fix never engaged and any
        // screen verdict is VOID (the mode-118 lesson). engCamFresh counts
        // frames a confirmed this-frame value existed and reads at mode 0
        // with stats armed - validate the lock BEFORE spending the A/B.
        // Stale = lock present, no upload that frame (fell back, correct);
        // LockDrops = confirm failures - after the 26441 distinct-set
        // retention these should be ~0 outside genuine relocations (dump1's
        // Locks 105 / Drops 104 churn is the before-figure); Ambig = frames
        // two distinct values both matched, no pick taken; ValOver =
        // distinct sightings at the locked offset beyond the 4-deep set.
        // engCamLockOff rides kvf: -1 at rest is a sentinel and kv would
        // wrap it (the 26406 lesson). PhaseStand = 26442 latch refusals
        // (the seam's upcoming-camera phase under motion - ledger at the
        // frame latch in kh_engcam_consume); it should track MOVING frames
        // in a Zeus session and read ~0 at rest. The failure counters are
        // per-attempt since 26442, not per-frame.
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
        //   svNoAdoptFrames - mode 137: BOTH passes stood the view adoption
        //     down and read the shared cycle_pv latch instead. Mode 102 only
        //     ever disabled it in the SEAM, so every 102 reading is a
        //     half-applied revert. Expect == compositeInjections under 137.
        out.push_back(kv("svNoAdoptFrames", RenderIntegration::g_svs_noadopt_frames));
        //   svSeamNoAdopt - 26291: SEAM-side stand-downs (102/137/142). The
        //     colour-only counter above is how 102's half-application hid for
        //     ten builds; under 137/142 BOTH must track their pass counts or
        //     a half did not engage - that is a reading now, not a trust.
        out.push_back(kv("svSeamNoAdopt", RenderIntegration::g_svs_seam_noadopt));
        //   svPvLatchSerial - 26291: total cycle_pv writes this session (the
        //     stamp the per-frame seamPvStamp/compPvStamp lanes record). Read
        //     against flushes: meaningfully above one per frame means the
        //     latch is rewritten mid-frame, which is the surviving suspect
        //     from mode 137's failure to register. NOT zeroed at the arm - it
        //     is an identity, and zeroing it would alias trace stamps across
        //     an arm boundary.
        out.push_back(kv("svPvLatchSerial",
                         static_cast<uint64_t>(RenderIntegration::g_pv_latch_serial)));
        //   26292 MODE 143 (seam live translation; ledger at the arm).
        //     svLiveTrnTakes == svInjects with Miss/Guard ~0 is engagement;
        //     svLiveTrnDx/Max is the correction applied (expect ~camStepM);
        //     the residual is svSeamRotCamDx / per-frame seamCamDxM.
        out.push_back(kv("svLiveTrnTakes", RenderIntegration::g_svs_live_trn_takes));
        out.push_back(kv("svLiveTrnMiss", RenderIntegration::g_svs_live_trn_miss));
        out.push_back(kv("svLiveTrnGuard", RenderIntegration::g_svs_live_trn_guard));
        out.push_back(kvf("svLiveTrnDx", RenderIntegration::g_svs_live_trn_dx));
        out.push_back(kvf("svLiveTrnDxMax", RenderIntegration::g_svs_live_trn_dx_max));
        //   26293 MODE 144 (live depth pair at the seam; ledger at the arm).
        //     svLivePrjTakes == svInjects with Ref ~0 is engagement; the
        //     verdict lane is per-frame seamNear vs injNear through a ramp.
        out.push_back(kv("svLivePrjTakes", RenderIntegration::g_svs_live_prj_takes));
        out.push_back(kv("svLivePrjRef", RenderIntegration::g_svs_live_prj_ref));
        out.push_back(kvf("svSeamNear", RenderIntegration::g_svs_seam_near));
        //   svTrnExtrap - 26294 mode 147, RETIRED 26295 (deceleration false
        //     positives regressed the slice). Kept so old scripts validate.
        out.push_back(kv("svTrnExtrap", RenderIntegration::g_svs_trn_extrap));
        //   26295 SEAM PAIR AUTHORITY (ledger at the arm): Comm = committed-
        //     pair encodes, Live = live pair on 40% agreement, Fall = cold.
        //     Comm+Live == svInjects after warmup is engagement; the verdict
        //     lane stays per-frame seamNear vs injNear. Bound = translation
        //     takes refused by the trajectory bound (foreign publications).
        out.push_back(kv("svPairComm", RenderIntegration::g_svs_pair_comm));
        out.push_back(kv("svPairLive", RenderIntegration::g_svs_pair_live));
        out.push_back(kv("svPairFall", RenderIntegration::g_svs_pair_fall));
        out.push_back(kv("svLiveTrnBound", RenderIntegration::g_svs_live_trn_bound));
        //   26297 LIVE BASIS (ledger at the arm): RotLive ~ svInjects is
        //     engagement, RotRef counts row-norm refusals (fv basis stood).
        out.push_back(kv("svLiveRotTakes", RenderIntegration::g_svs_live_rot_takes));
        out.push_back(kv("svLiveRotRef", RenderIntegration::g_svs_live_rot_ref));
        //   svPairWide - 26298: coherent wide-band pair accepts (the per-shot
        //     near steps the 40% band lagged by a frame; ledger at the arm).
        out.push_back(kv("svPairWide", RenderIntegration::g_svs_pair_wide));
        //   svPairHold - 26300 mode 151: frames the debounced encode held
        //     against a 1-frame committed step (ledger at the arm).
        out.push_back(kv("svPairHold", RenderIntegration::g_svs_pair_hold));
        //   svPairSniff - 26301: sniff-pair encodes (fallback since 26302).
        out.push_back(kv("svPairSniff", RenderIntegration::g_svs_pair_sniff));
        //   26303: predicted fallback + the live-refusal reasons, split.
        out.push_back(kv("svPairPred", RenderIntegration::g_svs_pair_pred));
        out.push_back(kv("svPairSaneRef", RenderIntegration::g_svs_pair_sane_ref));
        out.push_back(kv("svPairCorrRef", RenderIntegration::g_svs_pair_corr_ref));
        //   26304: base = return-to-baseline live accepts; rebase = predictor
        //   re-bases off two agreeing out-of-band commits; refGuard =
        //   corridor referee substitutions (polluted committed reference);
        //   noRef = live refused solely for want of a fresh committed
        //   reference (previously invisible). svCbc* = the bound-CB pointer
        //   census (handoff 3.1): records/evicts on the upload side,
        //   matches/misses at the seam lookup, encodes under mode 152.
        out.push_back(kv("svPairBase", RenderIntegration::g_svs_pair_base));
        out.push_back(kv("svPairRebase", RenderIntegration::g_svs_pair_rebase));
        out.push_back(kv("svPairRefGuard", RenderIntegration::g_svs_pair_ref_guard));
        out.push_back(kv("svPairNoRef", RenderIntegration::g_svs_pair_noref));
        out.push_back(kv("svCbcRecords", RenderIntegration::g_cbc_records));
        out.push_back(kv("svCbcEvicts", RenderIntegration::g_cbc_evicts));
        out.push_back(kv("svCbcMatches", RenderIntegration::g_cbc_matches));
        out.push_back(kv("svCbcMisses", RenderIntegration::g_cbc_misses));
        out.push_back(kv("svCbcEncodes", RenderIntegration::g_cbc_encodes));
        //   26306: camRecs/camHits = the census view probe (instrument);
        //   trnRebase = refused-relocation confirmations taken; trnWide =
        //   takes the 26305 trajectory bound would have refused.
        out.push_back(kv("svCbcCamRecs", RenderIntegration::g_cbc_cam_recs));
        out.push_back(kv("svCbcCamHits", RenderIntegration::g_cbc_cam_hits));
        out.push_back(kv("svLiveTrnRebase", RenderIntegration::g_svs_live_trn_rebase));
        out.push_back(kv("svLiveTrnWit", RenderIntegration::g_svs_live_trn_wit));   // 26443
        out.push_back(kv("svLiveTrnWide", RenderIntegration::g_svs_live_trn_wide));
        //   svLiveTrnAnchOff/Max - 26309: |raw live camera - anchored
        //   candidate| in metres = the second-flavor anchor offset the
        //   delta take removed (expected ~0 healthy; 0.18-0.28 on the
        //   capture12 scenario). svLiveTrnLpMiss - cold/post-miss runs
        //   that fell back to the absolute form for one frame.
        out.push_back(kvf("svLiveTrnAnchOff", RenderIntegration::g_svs_trn_anch_off));
        //   svSeamAnchDx/Max - 26310: |seam adopted-view camera - cycle
        //   latch camera| in metres, the multiplex-flavor gauge (0.258
        //   constant in capture13's Zeus session; ~0 healthy).
        out.push_back(kvf("svSeamAnchDx", RenderIntegration::g_svs_seam_anch_dx));
        out.push_back(kvf("svSeamAnchDxMax", RenderIntegration::g_svs_seam_anch_dx_max));
        //   svLiveFovTakes/Ref - 26311: seam FOV terms from the live fetch
        //   (the zoom axis); svCbcClassRef/Near - census encodes refused by
        //   the two-witness class referee, and the last refused near.
        out.push_back(kv("svLiveFovTakes", RenderIntegration::g_svs_live_fov_takes));
        out.push_back(kv("svLiveFovRef", RenderIntegration::g_svs_live_fov_ref));
        out.push_back(kv("svCbcClassRef", RenderIntegration::g_cbc_class_ref));
        out.push_back(kvf("svCbcClassRefNear", RenderIntegration::g_cbc_class_ref_near));
        //   svCbcBandSkips - 26312: recorder notes skipped by the shared
        //   camera-class band (out-of-band content no longer overwrites a
        //   tracked buffer's world-class pair).
        out.push_back(kv("svCbcBandSkips", RenderIntegration::g_cbc_band_skips));
        //   svLiveFovRatio/Max - 26313: per-frame live/latch FOV ratio (the
        //   flicker gauge; legit zoom tops ~1.13, the bar is 1.30).
        out.push_back(kvf("svLiveFovRatio", RenderIntegration::g_svs_live_fov_ratio));
        out.push_back(kvf("svLiveFovRatioMax", RenderIntegration::g_svs_live_fov_ratio_max));
        //   svCompFovTakes/Ref + svCompFovRatioMax - 26325: the COLOUR pass's
        //   FOV take (171 reverts). The seam has ridden the live fetch's
        //   scale terms since 26311 and the visible box never did, so through
        //   a zoom the box rasterised one frame behind its own footprint and
        //   the engine's cascade + stencil verdicts leaked across its
        //   silhouette. Takes ~ compositeInjections with Ref ~0 is
        //   engagement; RatioMax is the peak deviation from 1.0 that was
        //   actually APPLIED, so it should land near seamFovLatR's peak.
        //   NOTE svLiveFovRatio/Max above are the SEAM's bar and are known
        //   inert (they read exactly 1.0 with Ref 0 through a session where
        //   seamFovLatR reached 1.1775) - do not read them as agreement.
        //   svSniffOvr/Ratio/Near - 26326: seam encodes replaced by the
        //   engine's own sniffed pair (172 reverts). The capture measured our
        //   injection encoding near 0.81643 where every engine draw, both
        //   scene cycles and the shadow volumes included, used 0.2236; the
        //   trace then named the source, with sniffNear == injNear on 511/511
        //   and the chain diverging >3x on 11. Expect Ovr on ~2 pct of
        //   injects with Ratio in the 2-6x band. Ovr climbing toward
        //   svInjects means the chain has broken wholesale; Ovr 0 with the
        //   artifact still present means the sniff went cold and the
        //   fallback is what shipped - read seamProjSrc.
        //   26327: the referee is DISARMED (opt-in under 173), so on mode 0
        //   these now report what it WOULD have displaced, displacing
        //   nothing. Read svSniffOvr against svInjects to size any future
        //   bar honestly BEFORE shipping it - at 26326 a 2 pct threshold
        //   fired on 22.5 pct of injects, which is what regressed the field.
        //   volPass* - 26328: the pair the ENGINE'S OWN shadow-volume pass
        //   had bound at VS b2 at the injection instant, copied GPU-side and
        //   decoded from its view-projection. This is the number every fix in
        //   this campaign has been guessing at. volPassReads should track
        //   flushes; volPassWCol must sit near 1.0 (it is the check that b2
        //   really held a view-projection - if it drifts, the slot moved and
        //   nothing downstream should be trusted). PURE GAUGE: nothing
        //   encodes with it yet.
        out.push_back(kv("volPassReads", RenderIntegration::g_vpx_reads));
        out.push_back(kv("volPassBusy", RenderIntegration::g_vpx_busy));
        out.push_back(kv("volPassSkips", RenderIntegration::g_vpx_skips));
        out.push_back(kvf("volPassWCol", RenderIntegration::g_vpx_wcol));
        out.push_back(kvf("volPassNearLast", RenderIntegration::g_vpx_near));
        //   volDraw* - 26329: the same measurement taken at the engine's
        //   FIRST COUNTING DRAW instead of at our injection. That is the b2
        //   the shadow volumes actually consume, so volDrawNear vs seamNear
        //   (aligned by volDrawAge) is the comparison that decides whether
        //   our encode is right. volDrawReads should track flushes.
        //   svEngVpArms - 26330: injections transformed by the engine's own
        //   b2 (mode 174). Should track svInjects when 174 is set and read 0
        //   otherwise. This is the arm that removes the encode from the
        //   problem entirely rather than trying to get it right.
        out.push_back(kv("svEngVpArms", RenderIntegration::g_svs_engvp_arms));
        out.push_back(kv("volDrawReads", RenderIntegration::g_vpx_draw_reads));
        out.push_back(kvf("volDrawNearLast", RenderIntegration::g_vpx_draw_near));
        // 26365 the duplicate-publication detector (ledger at g_svs_fetch_dup_m)
        out.push_back(kv("seamFetchDups", RenderIntegration::g_svs_fetch_dups));
        out.push_back(kv("seamFetchMoves", RenderIntegration::g_svs_fetch_moves));
        out.push_back(kvf("seamFetchDupM", RenderIntegration::g_svs_fetch_dup_m));
        out.push_back(kvf("bandPickCamDxMax", RenderIntegration::g_band_pick_cam_dx_max));
        // 26412 THE WHOLE COMMITTED BAND TABLE (ledger at KH_BAND_TABLE_CENSUS).
        // Four lanes per slot in the shader's consumption order, finest first;
        // -1 on all four means the slot did not commit. bandGapMaxM is the worst
        // hole between consecutive slots once each one's own camera drift is
        // applied - positive IS a hole, in metres - and bandGapFrames counts the
        // frames carrying one. Seven builds argued about the tiling from the
        // near pair alone; this ends that.
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
            // 26414 THE TABLE AS IT STOOD ON THE WORST-GAP FRAME. The lanes above
            // are the LAST committed frame, which for a few-frame artifact is
            // almost never the interesting one; these are latched at the event.
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
        // 26386: mode 209. Read it exactly as bandTrnEscapes is read for 195 -
        // 0 means the arm never fired and the field verdict is void.
        out.push_back(kv("bandRotEscapes", RenderIntegration::g_ls.band_rot_escapes));
        // 26415: bands promoted to the 20 Hz tier because their camera drift
        // passed 0.15 x their own width - the overlap-anchored budget. This is
        // the engagement lane: 0 means the fold never fired and any null is void.
        out.push_back(kv("bandBudgetEscapes", RenderIntegration::g_ls.band_budget_escapes));
        // 26458: the layout-change escape (ledger beside the throttle).
        out.push_back(kv("bandLayoutEscapes", RenderIntegration::g_ls.band_layout_escapes));
        // 26370 the scope trigger. trigFrozen 1 = the ring is held around
        // trigSerial; every row in the dump is real and none has been
        // overwritten since. reason 1 = no-pick, 2 = stale pick.
        // 26371 the seam retry. retryOk/retry is the hit rate; if retry
        // climbs while retryOk stays flat the bridge is re-serving the same
        // publication and the answer is upstream of the take.
        out.push_back(kv("seamRetry", RenderIntegration::g_svs_live_trn_retry));
        out.push_back(kv("seamRetryOk", RenderIntegration::g_svs_live_trn_retry_ok));
        out.push_back(kv("trigFrozen", RenderIntegration::g_ffr_freeze ? 1 : 0));
        // 26421: g_ffr_trig_post is int with -1 = "not fired", and kv takes
        // uint64_t - so the sentinel went through float and then OUT OF RANGE
        // for the destination type (UB; 0 or 2^64-1 in practice). The lane
        // could never report the state it holds in most sessions. kvf, and
        // the two above it drop a static_cast<float> that only ever detoured.
        out.push_back(kv("trigSerial", RenderIntegration::g_ffr_trig_serial));
        out.push_back(kv("trigReason", RenderIntegration::g_ffr_trig_reason));
        out.push_back(kvf("trigPostLeft", static_cast<float>(RenderIntegration::g_ffr_trig_post)));
        // 26362 THE ENGINE-VS-SEAM SWING (ledger at kh_vpx_ref_publish).
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
        //   svGrowDraws / svGrowLast - 26288: seam draws issued with an ENLARGED
        //     footprint, and the factor. Expect == svInjectDraws under 138-140.
        //     This arm does not try to align the two rasterisations at all - it
        //     makes the depth-only footprint bigger than the visible box so a
        //     18-34 px displacement cannot expose a hole. The visible draw is
        //     untouched: no drift, no lag. Cost is a thin band of wrong GROUND
        //     shadow hugging the box, scaling with the factor - take the
        //     SMALLEST of 138/139/140 that closes the slice.
        out.push_back(kv("svGrowDraws", RenderIntegration::g_svs_grow_draws));
        out.push_back(kvf("svGrowLast", RenderIntegration::g_svs_grow_last));
        //   26289 MODE 141 is the soft filter: svVolCode reads 5 and
        //     svVolWitFrames must equal flushes. It weights a 7x7 by how well
        //     each texel s depth matches our own surface and AVERAGES their
        //     verdicts instead of choosing one - so it cannot interlace the way
        //     the walk did, and it survives registration error from any cause.
        // 26279: proxy casters that entered the private sun map this session.
        // Should track (proxies registered) x (sun-depth passes). 0 with proxies
        // registered means they are not reaching render_sun_depth - check that
        // casterOnly is set and mode is not DepthMode::Off.
        out.push_back(kv("sunProxyCasters", RenderIntegration::g_sun_proxy_casters));
        // 26280 THE PREMISE TEST, and it is the one lane on this list that can
        // invalidate the rest of them.
        //   svVolIsColour  - frames where the buffer the COLOUR injection writes
        //     depth into is THE SAME RESOURCE the volume copy reads. If this is
        //     non-zero the seam injection is redundant: our depth is already in
        //     the counted buffer, at the visible box position, registered by
        //     construction - and every pixel of slice is self-inflicted by a
        //     second differently-transformed write. Delete the seam injection.
        //   svVolNotColour - the two are different resources, which is what the
        //     26205 ledger claims from a capture. Expect this to carry the count;
        //     it is published because an inherited premise this much machinery
        //     rests on should be a reading, not a citation.
        //   svColourDsvNone - no DSV bound at the injection. Should be 0.
        //   svSeamDimChanges / svSeamWMin - the seam target flipped between
        //     1920x1080 and 3840x2160 across sessions on one build. Non-zero
        //     changes, or a WMin below scene width, means the injection is going
        //     into a half-res buffer on some frames and NOTHING measured that
        //     until now. Read this before trusting any A/B in this campaign.
        out.push_back(kv("svVolIsColour", RenderIntegration::g_svs_vol_is_colour));
        out.push_back(kv("svVolNotColour", RenderIntegration::g_svs_vol_not_colour));
        out.push_back(kv("svColourDsvNone", RenderIntegration::g_svs_colour_dsv_none));
        out.push_back(kv("svSeamDimChanges", RenderIntegration::g_svs_seam_dim_changes));
        out.push_back(kv("svSeamWMin", RenderIntegration::g_svs_seam_w_min));
        out.push_back(kv("svSeamHMin", RenderIntegration::g_svs_seam_h_min));
        out.push_back(kv("svVolLatchDrops", RenderIntegration::g_svs_vol_latch_drops));
        // 26250 THE TRANSPORT'S OWN EPOCH KEY. svReprojEpochHits/Miss now report
        //   the ring lookup for WHICHEVER transport is live - keyed on the volume
        //   copy under 109+, on the post snapshot otherwise. Under the volume
        //   transport svReprojEpochHits must track svVolCopyMade; a Miss count
        //   equal to the flush count means NO REPROJECTION IS HAPPENING and the
        //   mesh is reading at its raster position, which is the 26243 strip.
        //   That is what 26249 shipped, and mode 111 painting flat grey is the
        //   one-screenshot version of the same statement.
        //   svVolAgeLast / svVolAgeMax - frames between the keyed pass and the
        //   mesh draw. Expect 0 or 1 (the epoch bumps at the seam boundary,
        //   which falls between the copy and the colour pass - the same clock
        //   artifact svPostAgeMax documents, NOT staleness). A large value means
        //   the key stopped advancing and the ring is missing on age.
        // 26254 THE INJECTION'S REGISTRATION, and the discriminator for the
        // last artifact. RenderDoc measured our INJECTION 21.5 deg (721 px) off
        // the engine's view in a frame where our COMPOSITE matched it to 0.0 px
        // and the engine used ONE view. That misplacement - not the reprojection
        // band, not the guard - is the shadow lag: the stencil verdict is
        // computed for a world point our misplaced footprint corresponds to.
        //   injAdoptSrc     - what the injection's view actually came from.
        //     1 = frame_view adopted (the intended path), 2 = cycle_pv (a frame
        //     AHEAD by its own ledger), 3 = a raw bridge fetch, 0 = none.
        //   injAdoptRotDeg / injAdoptRotMax - angle between the rotation the
        //     injection ended up with and the rotation g_ls.frame_view held at
        //     that instant. READ IT WITH svReprojPxMean:
        //       near 0 + svReprojPxMean large -> the injection DID adopt and
        //         frame_view ITSELF moved between the seam and the flush.
        //       large -> the injection is not on frame_view at all and
        //         injAdoptSrc names the fallback that took it.
        //     These are different faults with different fixes, which is why
        //     they get separate lanes instead of one.
        //   injAdoptFvCold  - frame_view absent at the injection.
        // 1 px = 0.0298 deg at fireFovY 0.5625, so multiply by 33.5 to compare
        // against svReprojPxMean.
        out.push_back(kv("injAdoptSrc", static_cast<uint64_t>(RenderIntegration::g_inj_adopt_src)));
        out.push_back(kvf("injAdoptRotDeg", RenderIntegration::g_inj_adopt_rot_deg));
        out.push_back(kvf("injAdoptRotMax", RenderIntegration::g_inj_adopt_rot_max));
        out.push_back(kv("injAdoptFvCold", RenderIntegration::g_inj_adopt_fv_cold));
        // 26255 WHICH CHANNEL SHOULD THE INJECTION BE ON. 26254 proved it adopts
        // frame_view faithfully (injAdoptRotMax 0.0198 deg over a rotating
        // window), so the 37 px gap to the composite is frame_view MOVING during
        // the frame - 589 writes per frame, alternating between this frame's and
        // the next frame's camera by kh_adopt_frame_view's own ledger. Only two
        // sources exist at seam time. These lanes say which is closer to the
        // engine, which the composite is measured equal to.
        //   injCycleErrPx / Max / Mean - |cycle_pv - the composite's view|, in
        //     pixels, built from cycle_pv's VIEW against the SAME projection so
        //     only the view source differs. COMPARE DIRECTLY WITH
        //     svReprojPxMean, which is |frame_view(early) - composite|:
        //       injCycleErrMean MUCH LOWER -> the bridge is the better channel
        //         and MODE 102's existing revert IS the fix. One line.
        //       injCycleErrMean >= svReprojPxMean -> both channels are wrong at
        //         seam time and the injection needs the ENGINE's own view
        //         sniffed from its bound CB at the seam (precedent in-file:
        //         shadow_view_scan from the bind path, viewBindScans).
        //     Read the MEANS, not the maxes - a session max is not a reading,
        //     and one relock transient owns a max forever (campaign 35).
        out.push_back(kvf("injCycleErrPx", RenderIntegration::g_inj_cycle_err_px));
        out.push_back(kvf("injCycleErrMax", RenderIntegration::g_inj_cycle_err_max));
        out.push_back(kvf("injCycleErrMean",
            RenderIntegration::g_inj_cycle_frames > 0
                ? static_cast<float>(RenderIntegration::g_inj_cycle_err_sum /
                                     static_cast<double>(RenderIntegration::g_inj_cycle_frames))
                : -1.0f));
        out.push_back(kv("injCycleFrames", RenderIntegration::g_inj_cycle_frames));
        //   injCycleIncoherent - MUST BE 0. The three arms are the sides of one
        //     triangle: injCycleErrPx must lie within svSwingPx +- svReprojPx on
        //     the same frame. 26255 shipped this gauge feeding a CAMERA-RELATIVE
        //     centre to an ABSOLUTE matrix and published 2871 px against a
        //     hand-computed bound of 10-64; the bound was in a chat message, not
        //     in the code, so nothing but a human caught it. Now the instrument
        //     reports its own incoherence. Non-zero means one of the three lanes
        //     is not measuring what its name says and NO conclusion may be drawn
        //     from any of them.
        //   injCycleFrames should also track svReprojFrames closely - both arms
        //     skip the same behind-eye and wild frames. 156 against 1304 was the
        //     second symptom of the same defect.
        //   injCycleUnchecked - frames where the coherence test was SKIPPED
        //     because the swing arm had no fresh sample (the injection ran
        //     without producing one). 26257 read injCycleIncoherent 2 purely
        //     from comparing this frame's arms against last frame's swing;
        //     an unchecked frame and a coherent one must not share a lane.
        out.push_back(kv("injCycleIncoherent", RenderIntegration::g_inj_cycle_incoherent));
        out.push_back(kv("injCycleUnchecked", RenderIntegration::g_inj_cycle_unchecked));
        // ===================================================================
        // 26264 THE ENGINE-VIEW LOCATOR, trimmed to what is true. The 26257-26262
        // lanes (injEngLocOff/Slot/Hits/Miss/NoRef/Best/Relocs, injEngErr*) are
        // GONE: they measured a candidate block against OUR COMPOSITE's basis on
        // the belief the composite equals the engine's view, and capture 6
        // disproved that - our composite sat 16 deg out in PITCH while that same
        // frame's engine volume and colour passes were byte-identical. A moving
        // yardstick validates nothing, so the lanes built on it are worse than
        // absent.
        //
        // What remains is the SELF-CONTAINED fingerprint, which needs no
        // reference: identity 3x3 with a camera in its .w column, orthonormal
        // 3x3 sixteen floats later, (0,0,0,1) after that.
        //
        // 26265 IT DID NOT FIRE FOR TWO REASONS, AND ONLY ONE OF THEM WAS THE
        // ONE BEING CHASED.
        //
        // 26263 read engFpFail 1 - no staged range begins with an identity 3x3 -
        // and concluded the stage point must be on the wrong draw, because the
        // capture's b2 does begin with one. Both halves of that are true and the
        // conclusion still does not follow: the identity 3x3 is the DRAW'S WORLD
        // MATRIX (the ledger in rendering_integration.hpp says so in as many
        // words - "[world | view]", "rows 0-3 are an IDENTITY WORLD MATRIX"), so
        // gate one could only ever pass on a caster sitting at the world origin
        // unrotated. A corrected stage point would have read engFpFail 1 too,
        // and the session that proved it would have proved nothing.
        //
        // So: gate one is gone (the identity block is measured, not gating -
        // engIdentSeen), the gates that remain are properties of the VIEW block
        // alone, and the locator no longer sits on one draw at all - it samples
        // the draw stream (engSweeps), which removes the dependency on which
        // draw carries the view instead of answering it.
        //
        // READING ORDER FOR THE NEXT SESSION:
        //   engFpHits > 0 and engBlkAbs settled -> the address is found. Then
        //     mode 116 and svReprojPxMean is the verdict (predicted: collapses
        //     from 26-95 px toward 0).
        //   engFpHits 0 -> engFpFail names the furthest gate any candidate
        //     reached and engFpLen/Orth/Wcol/Row7/Det/Trace are that
        //     candidate's residuals against the bars printed beside them. A
        //     stage of 2 with engSweeps large means no sampled draw binds an
        //     orthonormal 3x3 anywhere, which is a real finding and not a
        //     tolerance argument.
        //   engFpDrifts > 0 -> the address MOVES between families. Four builds
        //     have assumed it cannot; this is the lane that says so.
        //   engOffDisagree > 0 -> window-relative and absolute reconstructions
        //     of the address differ, i.e. the engine re-binds that resource at
        //     different offsets per family. Absolute is the one in use.
        // ===================================================================
        out.push_back(kv("engFpHits", RenderIntegration::g_svs_eng_fp_hits));
        out.push_back(kv("engFpMiss", RenderIntegration::g_svs_eng_fp_miss));
        out.push_back(kv("engBlkOff", static_cast<uint64_t>(
            RenderIntegration::g_svs_eng_blk_off < 0 ? 0xFFFFFFFFull
                : static_cast<uint64_t>(RenderIntegration::g_svs_eng_blk_off))));
        // 26265: the ABSOLUTE address in the resource - the one the copy uses.
        // engBlkOff is kept as the window-relative value it always was, purely
        // so engOffDisagree has two things to compare.
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
        // 26265 THE COMPOSITE'S ARM NOW ASKS ABOUT FRESHNESS, NOT EXISTENCE.
        // engArmCompStale counts arms REFUSED because the copy was more than one
        // epoch old. Non-zero is not a fault by itself - the late flush path runs
        // after the seam boundary bumps the epoch - but engArmCompStale tracking
        // flushes means the visible mesh is taking the historic path every frame
        // and mode 117/118 is measuring nothing.
        out.push_back(kv("engCopyAgeLast", RenderIntegration::g_svs_eng_copy_age_last));
        out.push_back(kv("engCopyAgeMax", RenderIntegration::g_svs_eng_copy_age_max));
        out.push_back(kv("engArmCompStale", RenderIntegration::g_svs_eng_arm_comp_stale));
        // 26265 THE SWEEP. engSweeps is scan+stage attempts; engSweepFrameMax is
        // the per-frame peak against its own cap (32 unlocated, 2 located), so a
        // max pinned at the cap says the budget is the limit and not the draw
        // stream. engFpReconfirms climbing with engFpDrifts 0 is a stable
        // address; engFpDrifts non-zero retires the assumption that it is.
        out.push_back(kv("engSweeps", RenderIntegration::g_svs_eng_sweeps));
        out.push_back(kv("engSweepFrameMax",
            static_cast<uint64_t>(RenderIntegration::g_svs_eng_sweep_frame_max)));
        out.push_back(kv("engFpReconfirms", RenderIntegration::g_svs_eng_fp_reconfirms));
        out.push_back(kv("engFpDrifts", RenderIntegration::g_svs_eng_fp_drifts));
        out.push_back(kv("engStageBusy", RenderIntegration::g_svs_eng_stage_busy));
        // ===================================================================
        // 26266 THE ADDRESS WAS RIGHT AND THE CONTENTS WERE NOT.
        //
        // 26265 located the block: engFpHits 519, engBlkAbs 0, engFpDrifts 0,
        // engOffDisagree 0, engFpSlot 4. Slot FOUR - the capture's b2 - which is
        // why the four-slot scan through 26262 could never have found it, and
        // why the 240-byte decoy at slot 0 (F0 1.0, F16 0) made 26263 read the
        // miss as a wrong-draw problem. Both defects had to go together.
        //
        // Then the same lane, same slot, same offset, across four sessions:
        //   116     stage 8   wcol 0       row7 0       trace 2.554
        //   mode 0  stage 4   wcol 40.45   row7 46.98   trace 0.306
        //   117     stage 4   wcol  2.35   row7  2.13   trace 2.987
        // Orthonormal to 1e-7 and det exactly 1 in all three - so sometimes a
        // ROTATION-ONLY block and sometimes a FULL RIGID TRANSFORM, at one
        // address. The locator refused the translated variant; the COPY did not,
        // because it fired unconditionally on a cached address. engRot then
        // carried a translation, mul(engRot, float4(rel,1)) applied the camera
        // offset TWICE, and the field saw the box leave the frustum under 117 and
        // the shadow resolve onto the floor through the box under 116. It SNAPPED
        // with what was on screen rather than degrading with heading, which is
        // what separates this from a transpose error.
        //
        // THREE LAYERS NOW, and the lanes say which one is carrying the load:
        //   engBindRejects  - refused same-frame because the BINDING moved
        //                     (firstConstant / numConstants against the values
        //                     the gates passed through). Free, fails closed.
        //                     Non-zero: binding metadata discriminates the two
        //                     variants and this is the cheap fix.
        //                     ZERO with engVerifyFail non-zero: it does NOT, the
        //                     shader gate is the only defence, and the next build
        //                     must move the copy rather than tighten this.
        //   engVerifyPass / engVerifyFail - the 64 bytes the SHADER ACTUALLY READ,
        //                     staged back and gated one frame later. Every lane
        //                     before this one measured the source; none measured
        //                     the destination, which is the only quantity the
        //                     artifact is a function of. engCopies tracked
        //                     svInjects perfectly while the copy was landing a
        //                     translated matrix.
        //   KhEngRotUsable  - the live decision, in the vertex shader, on those
        //                     same bytes with no latency. Cannot be counted from
        //                     the CPU; engVerifyFail is its proxy.
        //
        // engVerifyFail / (engVerifyPass + engVerifyFail) is the fraction of
        // frames the engine-view path must refuse at the current copy site. Small
        // -> ship it and accept the historic path at those angles. Large -> the
        // copy site is wrong and engBindRejects says whether the binding can pick
        // the right frames.
        // ===================================================================
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
        // ===================================================================
        // 26267 THE WIDE ENUMERATION, and the reading that forced it.
        //
        // 26266 WORKED: both artifacts went - the box stopped disappearing under
        // 117, the shadow stopped resolving through it under 116 - because the
        // shader refuses a translated engRot and takes the historic path. The
        // cost is engVerifyFail 44/692 (6.4%) and 32/475 (6.7%): that many frames
        // per session run on the LAG instead. A limitation, not a defect.
        //
        // THE FORK RESOLVED THE WAY THAT SAYS "MOVE THE COPY". engBindRejects 0
        // against engVerifyFail 44 means the two variants share an IDENTICAL
        // binding - slot 4, firstConstant 0, numConstants 4096 - so binding
        // metadata cannot separate them and tightening that fingerprint is dead
        // ground.
        //
        // AND engSrcNum 4096 IS THE LEAD: 4096 constants is 65536 bytes, so the
        // engine binds the WHOLE pool and the staging window is min(range, 1024).
        // Every scan this campaign has run has read the first 1.6% of the bound
        // range. Byte 0 is a SHARED pool offset that more than one family writes,
        // which is precisely what "one address, two matrices, snaps with what is
        // on screen" describes. A family-private copy further into that 64 KB
        // would not be shared.
        //
        // HOW TO READ IT:
        //   engWideScans      - completed 64 KB readbacks (one per 32 injections).
        //   engWideN          - gate-8 offsets in the LAST scan. engWideNMax the
        //                       most ever. 1 means byte 0 is the only copy of the
        //                       view block in the pool and this line of attack is
        //                       closed - go back to moving the copy POINT.
        //   engWideOffK       - candidate K's byte offset (0 is the incumbent).
        //   engWidePassK / engWideSeenK - how often K still passed, over how many
        //                       scans since it first appeared. A RATE: every
        //                       known offset ages each scan whether it reappears
        //                       or not, so an offset that stops showing up decays
        //                       instead of keeping a flattering total.
        //
        // THE TEST: any K>0 with PassK == SeenK while candidate 0 sits below it is
        // an UNSHARED copy. Confirm it with mode 121/122/123, which force the copy
        // to read candidate 1/2/3 - engVerifyFail should collapse and the fallback
        // frames go with it. NOTHING ADOPTS AUTOMATICALLY: a mechanism that
        // relocated the address on its own measurement would be the fourth moving
        // yardstick this campaign has had to retire.
        // ===================================================================
        // ===================================================================
        // 26268 THE ANGLE, and it is the number that decides the whole question.
        //
        // 26267's mode-0 control closed the offset line for good: engWideBytes 240
        // says the engine's constant buffer is a DEDICATED 240-byte resource, not
        // a 64 KB pool - engSrcNum 4096 was D3D11's "whole buffer" sentinel and I
        // read it as a real range. So engWideNMax 1 is EXHAUSTIVE: there is one
        // view block in that resource, at byte 0, and nowhere to move to.
        //
        // WHICH LEAVES THE HOLE EVERY GATE SO FAR IS BLIND TO. All of them - the
        // locator's eight and the shader's three - test that the block is a rigid
        // rotation. A worldView matrix whose world part is a pure rotation about a
        // pivot near the camera IS a rigid rotation, and it is the wrong one.
        // First-person weapon fire is exactly that geometry: arms and weapon
        // rotating hard about a pivot centimetres from the eye, near-zero
        // translation. Our depth then lands at arbitrary screen positions in the
        // ENGINE'S volume buffer, its stencil volumes test against those texels,
        // and the soldier's shadow resolves onto them - random pieces of shadow
        // over random parts of the view, only while firing.
        //
        //   engVerRotMaxDeg < ~5   -> every copied basis really is a view
        //                             rotation. The hypothesis is DEAD and the
        //                             first-person artifact is something else -
        //                             say so and look elsewhere.
        //   engVerRotMaxDeg > ~20  -> CONFIRMED, and engVerRotOver20 says on how
        //                             many frames of engVerRotSamples. Those are
        //                             the frames mode 0's agreement bound now
        //                             refuses; 124 disables it for the A/B.
        //
        // The reference is the CPU's own view rotation, stashed AT THE COPY, so it
        // is one frame off the bytes it judges - a few degrees on a fast flick
        // against tens for recoil. It is a COARSE reference and that is legitimate
        // precisely where a fine one is not: campaign 37 retired the composite as
        // a yardstick for PRECISION, and over-generalised that into never
        // comparing at all. The historic path cannot certify the engine view. It
        // can refuse a basis that is not a view.
        // ===================================================================
        out.push_back(kvf("engVerRotDeg", RenderIntegration::g_svs_eng_ver_rot_deg));
        out.push_back(kvf("engVerRotMaxDeg", RenderIntegration::g_svs_eng_ver_rot_max_deg));
        out.push_back(kv("engVerRotOver20", RenderIntegration::g_svs_eng_ver_rot_over20));
        out.push_back(kv("engVerRotSamples", RenderIntegration::g_svs_eng_ver_rot_samples));
        // 26271 THE MEAN AND THE UNDER-ONE-DEGREE COUNT. engVerRotMaxDeg is a
        // worst-of and says nothing about the typical sample. If engVerRotUnder1
        // is most of engVerRotSamples, the engine's buffer at the seam carries
        // the rotation the CPU ALREADY HAD - the GPU copy is a no-op, and 95%
        // armed with no visible change is explained. The camera half of 26271 is
        // then where the error actually lives.
        out.push_back(kvf("engVerRotMeanDeg", RenderIntegration::g_svs_eng_ver_rot_samples
            ? static_cast<float>(RenderIntegration::g_svs_eng_ver_rot_sum /
                                 static_cast<double>(RenderIntegration::g_svs_eng_ver_rot_samples))
            : -1.0f));
        out.push_back(kv("engVerRotUnder1", RenderIntegration::g_svs_eng_ver_rot_under1));
        out.push_back(kv("engArmInjFrames", RenderIntegration::g_svs_eng_arm_inj_frames));
        // ===================================================================
        // 26269 THREE CANDIDATES. engCandCopiesK is how often slot K was filled;
        // slot 0 is the SEAM copy and owns engVerify*/engCopyAge*/engWide*, slots
        // 1..2 are opportunistic sweep samples at other draws in the same frame.
        //
        // The verdict is still engVerifyFail, which measures SLOT 0 only - so
        // under this build engVerifyFail staying near 100% in first person is
        // EXPECTED and no longer fatal: it says the seam's sample is bad, not
        // that the feature refused. What says the feature worked is the artifact
        // going away while mode 87 remains the only thing that removed it before.
        //
        // If it does NOT go away, the next question is sharp rather than open:
        // no draw the sweep lands on carries a plain view either, and the capture
        // to take is "which draw writes byte 64 of that 240-byte resource in
        // first person" - a targeted search, not a hunt.
        // ===================================================================
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
        // 26265 STATEBACKUP'S CONSTANT-BUFFER OFFSETS (ledger above the struct).
        // 26266 SETTLED, AND THE ANSWER IS THAT IT NEVER MATTERED. engSbCbOffMax
        // read 0 across four field sessions - including three running the 1.1
        // path with engSbCbOffRestores 2556 / 2444 / 1580 - so the engine never
        // binds a VS constant buffer at a non-zero firstConstant anywhere we
        // capture, and the pre-26265 plain restore was never corrupting engine
        // state. The 1.1 path is kept because it is correct rather than merely
        // harmless, and mode 120 can be deleted in the next build that touches
        // this struct. Recorded here rather than quietly dropped: a concern
        // raised and then measured to zero is a result.
        out.push_back(kv("engSbCbOffRestores", RenderIntegration::g_sb_cb_off_restores));
        out.push_back(kv("engSbCbPlainRestores", RenderIntegration::g_sb_cb_plain_restores));
        out.push_back(kv("engSbCbOffMax",
            static_cast<uint64_t>(RenderIntegration::g_sb_cb_off_max)));
        // 26263 WHAT ACTUALLY ARRIVED IN THE STAGING BUFFERS. The fingerprint was
        // transcribed literally and run offline against capture 5's b2: it
        // matches at byte 0 and rejects every other offset for the right reason.
        // So engFpHits 0 says the BYTES are not that buffer's, and no further
        // reasoning can separate "wrong buffer" from "right buffer, wrong bytes".
        //   engSlNBytes - the staged size of VS slot N (26265: six slots; b2 and
        //     b3 were an assumption and 26259's defect 2 was the last time one
        //     of those cost a session).
        //   engSlNF0 / engSlNF16 - the first float of row 0 and of row 4 of the
        //     staged WINDOW. Retained as raw evidence about which buffer is
        //     there; note they describe window offset 0 only, while the
        //     fingerprint scans every float4 offset, so a zero here does NOT
        //     mean the block is absent.
        for (int khe_i = 0; khe_i < RenderIntegration::KH_SVS_ENG_SLOTS; ++khe_i) {
            char khe_nm[24];
            sprintf_s(khe_nm, "engSl%dBytes", khe_i);
            out.push_back(kv(khe_nm, static_cast<uint64_t>(RenderIntegration::g_svs_eng_sl_bytes[khe_i])));
            sprintf_s(khe_nm, "engSl%dF0", khe_i);
            out.push_back(kvf(khe_nm, RenderIntegration::g_svs_eng_sl_f0[khe_i]));
            sprintf_s(khe_nm, "engSl%dF16", khe_i);
            out.push_back(kvf(khe_nm, RenderIntegration::g_svs_eng_sl_f16[khe_i]));
        }
        // ===================================================================
        // 26265 engFpFail NAMES THE GATE, AND EACH GATE PUBLISHES ITS RESIDUAL
        // NEXT TO ITS OWN BAR.
        //
        // The 26263 lane could only ever read 1 or 2 while its ledger in
        // rendering_integration.hpp promised four stages - so a miss could not
        // say WHICH test refused, which is the whole reason the lane existed.
        // Method note 2: an instrument that cannot be false is not an
        // instrument, and a bound that lives only in a chat message is not a
        // bound. The bars are constants in the code and printed here.
        //
        //   1 nothing examined      2 row length      3 orthogonality
        //   4 .w column non-zero    5 row 7           6 rotation is IDENTITY
        //   7 determinant not +1    8 every gate passed
        //
        // Stage 6 deserves a note: an identity 4x4 satisfies every other gate
        // trivially, and that is exactly what the 26262 offline _find_basis
        // latched onto before publishing a 166 deg verdict. A run that parks at
        // 6 is finding identity world matrices, not view blocks.
        // ===================================================================
        out.push_back(kv("engFpFail", static_cast<uint64_t>(RenderIntegration::g_svs_eng_fp_fail)));
        out.push_back(kv("engFpSlot", static_cast<uint64_t>(
            RenderIntegration::g_svs_eng_fp_slot < 0 ? 0xFFFFFFFFull
                : static_cast<uint64_t>(RenderIntegration::g_svs_eng_fp_slot))));
        out.push_back(kvf("engFpLen", RenderIntegration::g_svs_eng_fp_len));      // bar 0.01
        out.push_back(kvf("engFpLenBar", 0.01f));
        out.push_back(kvf("engFpOrth", RenderIntegration::g_svs_eng_fp_orth));    // bar 0.02
        out.push_back(kvf("engFpOrthBar", 0.02f));
        out.push_back(kvf("engFpWcol", RenderIntegration::g_svs_eng_fp_wcol));    // bar 1e-4
        out.push_back(kvf("engFpWcolBar", 1.0e-4f));
        out.push_back(kvf("engFpRow7", RenderIntegration::g_svs_eng_fp_row7));    // bar 1e-4
        out.push_back(kvf("engFpRow7Bar", 1.0e-4f));
        out.push_back(kvf("engFpDet", RenderIntegration::g_svs_eng_fp_det));      // want +1
        out.push_back(kvf("engFpTrace", RenderIntegration::g_svs_eng_fp_trace));  // 3 = identity
        out.push_back(kv("engIdentSeen", RenderIntegration::g_svs_eng_ident_seen));
        out.push_back(kv("svVolAgeLast", RenderIntegration::g_svs_vol_age_last));
        out.push_back(kv("svVolAgeMax", RenderIntegration::g_svs_vol_age_max));
        out.push_back(kv("svOmRtvMax", static_cast<uint64_t>(RenderIntegration::g_svs_om_rtv_max)));
        out.push_back(kv("svVolArmMax", static_cast<uint64_t>(RenderIntegration::g_svs_vol_arm_max)));
        // 26195 DARK-WINDOW RE-SEED CENSUS (black-mesh cold start).
        // blkDarkReseedRatioMin/Max is THE number: it says whether the
        // 26159 refusal bar should be absolute (as now) or relative to
        // the pending it protects.
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
            // row 14: THE TARGET - expected (refAlt m, density, heightDecay, ?)
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
            // publish row 1 = FogColor.rgb; .w joins them here)
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
            // row 12: x = fog end (fogEngine.y), y = inverse range
            // (fogEngine.z), z = LAYER ALTITUDE, w = below-layer extinction.
            // z and w have never been read.
            out.push_back(kvf("atmBlkR12x", bl[48]));
            out.push_back(kvf("atmBlkR12y", bl[49]));
            out.push_back(kvf("atmBlkR12z", bl[50]));
            out.push_back(kvf("atmBlkR12w", bl[51]));
            // row 13: y is the layer-altitude OFFSET the shader adds to
            // row 12 z before the branch (layerY = R12z + R13y)
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
        // 26155 LOCK-DEATH FORENSICS (ledger at g_view_drop_forced). Two paths
        // can clear view_src_valid and only one of them was ever counted, so a
        // lock that died on the health path was indistinguishable in a dump from
        // a lock that was never taken. viewRelockErMin is the discriminator for
        // 2A: it scores the REMEMBERED source against the bridge for as long as
        // the lock stays dead. Settling back under the guard bar means the drop
        // ate a correct lock; staying high means the drop was right and the
        // defect is upstream. These are STATE, not census - they survive the
        // getRenderStats arming call on purpose, because a cold-fault dump is
        // always armed after the death.
        out.push_back(kv("viewDropForced", RenderIntegration::g_view_drop_forced));
        out.push_back(kv("viewDropHealth", RenderIntegration::g_view_drop_health));
        out.push_back(kv("viewDropPath", static_cast<uint64_t>(RenderIntegration::g_view_drop_path)));
        // (age_s is declared further down this function; the drop stamp is
        //  aged inline rather than moving a lane out of its family.)
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
        // 26156 (ledger at g_view_lock_res_pre). viewLockSameSlot is the whole
        // question: 1 = the ring came back to the identity the guard dropped,
        // so the guard ate a correct lock; 2 = it came back somewhere else, so
        // the guard was right and the defect is purely how long re-acquisition
        // takes. viewLockDeadS is that latency - dump8 implies ~42 s.
        out.push_back(kv("viewLockSameSlot", static_cast<uint64_t>(RenderIntegration::g_view_lock_same_slot)));
        out.push_back(kv("viewLockAcqPath", static_cast<uint64_t>(RenderIntegration::g_view_lock_acq_path)));
        out.push_back(kvf("viewLockDeadS", RenderIntegration::g_view_lock_dead_s));
        out.push_back(kvf("viewLockDeadMaxS", RenderIntegration::g_view_lock_dead_max_s));
        // 26157 (ledger at g_view_ring_best_min). 2A is now scoped to how long
        // re-acquisition takes. ringBestMin says whether the correct candidate
        // was even available during the dead window; ringConfirmHolds and
        // ringConfirmChurn say whether the two-boundary confirm gate is what
        // held it out, and churn is the one that can hold it out without bound.
        out.push_back(kvf("viewRingBestMin", RenderIntegration::g_view_ring_best_min));
        out.push_back(kv("viewRingConfirmHolds", RenderIntegration::g_view_ring_confirm_holds));
        out.push_back(kv("viewRingConfirmChurn", RenderIntegration::g_view_ring_confirm_churn));
        out.push_back(kv("viewRingTmagRejects", RenderIntegration::g_view_ring_tmag_rejects));
        out.push_back(kvf("viewRingBestTmag", RenderIntegration::g_view_ring_best_tmag));   // 26161
        // 26166 COLD-START REPAIRS (ledger at g_view_drop_hyst_holds). Read
        // these three together on any session that used to fault:
        //   dropHystHolds  > 0 and viewDropForced 0  -> the hair-trigger
        //       drop that dump99 measured (0.0217 against a 0.02 bar) is
        //       being refused and the cold fault should not appear at all
        //   relockWideTakes > 0                      -> a drop still
        //       happened and the remembered source was reclaimed at the bar
        //       its own drop was measured against; viewLockDeadS says how
        //       fast (dump99 baseline: 6.387 s via the ring, path 3)
        //   dropHystForced > 0                       -> the streak safety
        //       valve fired: a small disagreement that would not clear the
        //       hysteresis stood for 4x the streak length and dropped anyway
        // setRenderDebug 23 reverts the hysteresis, 38 the recovery bar.
        out.push_back(kv("dropHystHolds", RenderIntegration::g_view_drop_hyst_holds));
        out.push_back(kv("dropHystForced", RenderIntegration::g_view_drop_hyst_forced));
        out.push_back(kvf("dropHystRd", RenderIntegration::g_view_drop_hyst_rd));
        out.push_back(kvf("dropHystBar", RenderIntegration::g_view_drop_hyst_bar));
        out.push_back(kvf("dropHystMaxMs", RenderIntegration::g_view_drop_hyst_max_ms));
        out.push_back(kv("relockWideArms", RenderIntegration::g_view_relock_wide_arms));
        out.push_back(kv("relockWideTakes", RenderIntegration::g_view_relock_wide_takes));
        out.push_back(kvf("relockWideEr", RenderIntegration::g_view_relock_wide_er));
        // 26166 PUBLISH CENSUS (ledger at g_pub_slot_hits_f). THE FLICKER
        // QUESTION, and the only pair that answers it. A "late" cycle is one
        // whose first accepted publish landed past draw 60 - the empty band
        // in fireFirstD's bimodal split (dump99: 1-18 or 107-160, nothing
        // between, on 512 of 512 frames).
        //   pubLateDry >> pubLateRejected  -> the engine does not upload the
        //       view CB until then. The arm CANNOT be moved, the flicker is
        //       structural, single-paint stays, and the campaign stops.
        //   pubLateRejected >> pubLateDry  -> uploads were arriving and the
        //       publish path refused them. pubRejTmag / pubRejFamily /
        //       pubRejCold name which filter; pubRejFirstD says how early the
        //       refused traffic starts. The fix is then in shadow_view_scan
        //       and it is small.
        // Both are per-CYCLE counts over the same window as flushes/topo*.
        // Nothing in this build reads any of them.
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
        // 26167 ALT-OFFSET CENSUS (ledger at g_pub_alt_first_d). THE remaining
        // question, and the last one task 1 has. Read pubAltLateEarly against
        // pubLateFrames:
        //   pubAltLateEarly ~= pubLateFrames -> on the late cycles a view was
        //       already present at ANOTHER offset inside the read window. The
        //       arm can move; widen the publish key to that offset and the
        //       flicker goes with it. pubAltOff names the offset.
        //   pubAltLateEarly 0                -> no view exists anywhere in the
        //       buffer by then. The arm cannot be moved, single paint is the
        //       floor, and task 1 is CLOSED - stop attacking the flicker.
        // pubAltLateAny is the weaker form (alt merely beat the accept).
        out.push_back(kv("pubAltLateEarly", static_cast<uint64_t>(RenderIntegration::g_pub_alt_late_early)));
        out.push_back(kv("pubAltLateAny", static_cast<uint64_t>(RenderIntegration::g_pub_alt_late_any)));
        out.push_back(kv("pubAltFrames", static_cast<uint64_t>(RenderIntegration::g_pub_alt_frames)));
        out.push_back(kv("pubAltHits", RenderIntegration::g_pub_alt_hits));
        out.push_back(kvf("pubAltFirstD", RenderIntegration::g_pub_alt_first_d_pub));
        out.push_back(kv("pubAltOff", static_cast<uint64_t>(RenderIntegration::g_pub_alt_off_pub)));
        out.push_back(kvf("pubAltEr", RenderIntegration::g_pub_alt_er));
        // 26168 HARVEST CENSUS + RUNNING ARGMIN (ledger at g_ring_best_valid).
        // Task 2 at last, and it is the harvest the campaign-27 handoff named.
        //   ringAdmits / ringHunts -> candidates admitted per hunt. dump101
        //       implies ~2518 shaped a cycle against SIXTEEN ring slots, and
        //       the ring is wiped at the injection, which lands BEFORE the
        //       view upload on every late cycle of every session on record.
        //   ringBestTakes  -> hunts where the running-argmin slot beat the
        //       ring. Large means the 16-slot window was missing the view,
        //       which is the whole claim.
        //   ringBestLocks  -> locks actually taken through the slot. Read
        //       against viewLockDeadS: if this is nonzero and deadS collapses
        //       from 6-31 s to under a second, the cold start is fixed.
        //   ringDryHunts   -> hunts that found an EMPTY ring.
        // FALSIFIED IF ringBestTakes stays 0 while viewLockDeadS stays high.
        // setRenderDebug 41 reverts to ring-only for the A/B.
        // 26169 MOTION-GATED CARRY (ledger at KH_CARRY_MAX_PX). The flicker,
        // last quadrant. Task 1 closed the ARM, not the paint count.
        //   fireCarryPaints > 0 and the operator stops seeing the blink ->
        //       fixed; the 26163 cure is back with its defect gated out.
        //   fireCarryPaints > 0 and the blink SURVIVES -> the carry never
        //       reaches the reader and the whole carry family is dead.
        //   fireCarryPxMax should sit at or under 1.0. If the operator sees a
        //       shadow thicken when they stop moving, lower KH_CARRY_MAX_PX.
        //   carrySkipsMotion / fireCarryPxSkip say how often and by how much
        //       the gate stood the carry down. Large is expected under motion.
        // setRenderDebug 42 = carry OFF (26165 single paint, verbatim).
        // 26171: count what you saw against what the model predicts. If these
        // agree, the flicker is structural at this rate and the campaign is
        // done with it. If you see many more blinks than flickerTransitions,
        // there is a second mechanism and every closed lever reopens.
        // 26172 BRIDGE ARM: castBridgeFirst is the number of cycles that fired
        // WITHOUT waiting for a publish - i.e. the late frames, made early.
        // flickerTransitions should read 0.
        // 26173 RECEIVE BAND CENSUS (ledger at g_band_pick). Run the fault:
        // Zeus camera above the box, steep look-down, MOVE. Then read
        //   bandPickChanges / bandPickCycles -> winner instability. Large and
        //       motion-correlated = the walk is flipping slots. That is it.
        //   bandPickNone   -> cycles the box left every sealed extent.
        //   bandPickAgeMaxMs -> worst seal age actually consumed. If changes
        //       are ~0 and this is large, the fault is staleness inside ONE
        //       slot, not selection.
        out.push_back(kv("bandPickChanges", RenderIntegration::g_band_pick_changes));
        out.push_back(kv("bandPickNone", RenderIntegration::g_band_pick_none));
        out.push_back(kv("bandPickNoneRuns", RenderIntegration::g_band_pick_none_runs));
        // 26176: the winner had a PROVISIONAL vcol (sm from this resolve, view
        // from a publish up to 50 ms old). bandPendRuns against your flicker
        // count is the field match; bandPendAgeMaxMs is how stale it got.
        // 26177: how each seal got its vcol. bandSealPending should be ~0;
        // anything there is a seal still open to a cross-frame completion.
        out.push_back(kv("bandSealSame", RenderIntegration::g_band_seal_same));
        out.push_back(kv("bandSealBridge", RenderIntegration::g_band_seal_bridge));
        out.push_back(kv("bandSealPending", RenderIntegration::g_band_seal_pending));
        out.push_back(kv("bandPendConsumed", RenderIntegration::g_band_pend_consumed));
        out.push_back(kv("bandPendRuns", RenderIntegration::g_band_pend_runs));
        out.push_back(kv("bandPendMax", static_cast<uint64_t>(RenderIntegration::g_band_pend_max)));
        out.push_back(kvf("bandPendAgeMs", RenderIntegration::g_band_pend_age_ms));
        out.push_back(kvf("bandPendAgeMaxMs", RenderIntegration::g_band_pend_age_max_ms));
        // 26179: the seal COMPLETION census. bandPendConsumed flags the band at
        // consumption, which is BEFORE the overwrite it is blamed for; these
        // lanes measure the overwrite itself. bandCompleteCross is a completion
        // of a seal captured before this cycle's boundary - frame-N sm paired
        // with a frame-N+1 view, the offset. PREDICTION: bandCompleteCross
        // tracks bandPendConsumed one for one. If it reads ~0 the model is
        // wrong and setRenderDebug 45 must not be promoted.
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
        // 26221 TWIN-PARITY GAUGE. injFxDimBaseline counts injection-edition
        // mesh draws that took fxMeta.zw from the unconditional fill alone -
        // i.e. exactly the draws that before 26221 shipped a zero clamp bound
        // and silently neutralised every mask read. It is a census, not a
        // fault: any value is fine now, and it should track injGuardOff on a
        // session with no far-arb / farkeep / nearz traffic.
        out.push_back(kv("injFxDimBaseline", RenderIntegration::g_inj_fx_dim_baseline));
        {   // OCCLUSION-GUARD OVERHAUL diagnostics + config echo
            out.push_back(kv("snapSerial", RenderIntegration::g_snap_serial));
            out.push_back(kv("snapFails", RenderIntegration::g_snap_fails));
            out.push_back(kv("snapConsumed", RenderIntegration::g_snap_consumed));
            out.push_back(kvf("snapAgeNowMs", RenderIntegration::g_snap_ms != 0
                ? static_cast<float>(RenderIntegration::steady_now_ms() - RenderIntegration::g_snap_ms)
                : -1.0f));
            out.push_back(kvf("snapAgeInjMs", RenderIntegration::g_snap_age_last));
            out.push_back(kvf("snapAgeInjMaxMs", RenderIntegration::g_snap_age_max));
            // (Campaign diagnostics - jitter probe, config echo, GPU
            // pixel autopsy - retired with the settled model; the notes
            // doc records their findings.)
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
        out.push_back(kv("blkBlankSkips", RenderIntegration::g_blk_blank_skips));   // 26053 blank-guard census
        // 26476 anchor-gate lanes (ledger at kh_probe_std_refresh).
        out.push_back(kv("blkAnchorRejects", RenderIntegration::g_blk_anchor_rejects));
        out.push_back(kv("blkAnchorSnaps", RenderIntegration::g_blk_anchor_snaps));
        out.push_back(kv("blkSmallNfSkips", RenderIntegration::g_blk_smallnf_skips));
        out.push_back(kvf("blkAnchorSunL", RenderIntegration::g_light_probe.anch_sun_l));
        out.push_back(kvf("blkAnchorAmbL", RenderIntegration::g_light_probe.anch_amb_l));
        out.push_back(kv("blkStarvedAdopts", RenderIntegration::g_blk_starved_adopts));   // 26054 starvation census
        // 26054: the capture-side pending slot itself (the publish twin's
        // blkPendAgeS already exists) - the next stuck log shows WHICH
        // flavor is pending and whether it is maturing or thrashing.
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
        // 26348 THE FOOTPRINT'S OWN ENCODE. injNzDraws = injections that bound
        // PSInjDepth (engagement: it should track compositeInjections).
        // injNzFloor = the widened gap floor last programmed on the injection
        // viewport; 0 means not widened, i.e. mode 193 or disarmed.
        // 26355 THE NEAR-CLASS CENSUS (pure gauge). The near population is
        // several coexisting constants, not one value that moves - read all
        // four classes together, never nearCls0 alone.
        {
            uint64_t khnc_v = 0;
            for (int khnc_i = 0; khnc_i < 4; ++khnc_i) {
                const float khnc_n = RenderIntegration::kh_near_class(khnc_i, &khnc_v);
                out.push_back(kvf(("nearCls" + std::to_string(khnc_i)).c_str(), khnc_n));
                out.push_back(kv(("nearClsN" + std::to_string(khnc_i)).c_str(), khnc_v));
            }
        }
        out.push_back(kv("injNzDraws", RenderIntegration::g_inj_nz_draws));
        // 26349: kvf, NOT kv - kv takes uint64_t and truncated this float to
        // 0 in both 26348 dumps, a dead gauge shipped in the same build that
        // was warned about dead gauges.
        out.push_back(kvf("injNzFloor", RenderIntegration::g_inj_nz_floor));
        out.push_back(kvf("nearzNearEst", RenderIntegration::g_nearz_last_near));
        out.push_back(kvf("nearzGapFloor", RenderIntegration::g_nearz_last_floor));
        // 26185 PROJECTION-PAIR CENSUS (full ledger at g_proj_census). One
        // cycle's DISTINCT encode spaces, most-seen first. Compare each
        // projPairNFar against injDpM22/injDpM32 in the same dump: the pair
        // our injection encodes with is the one whose near/far match those,
        // and any other pair in the table is content being written into the
        // same depth buffer through a different mapping. With a shared near
        // of 0.07 a far of 195734 writes a SMALLER value than a far of 2531
        // at the same distance, so which content wins stops being a
        // function of distance at all. projPairEvicts > 0 means there were
        // more than four and the table is not the whole picture.
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
        out.push_back(kv("sunHeroValid", RenderIntegration::g_sun2_map_valid ? 1ull : 0ull));   // 26444
        out.push_back(kv("sunHeroCasters", RenderIntegration::g_sun2_casters));
        out.push_back(kv("sunHeroRenders", RenderIntegration::g_sun2_renders));
        out.push_back(kvf("sunHeroHalfDiag", RenderIntegration::g_sun2_half_diag));
        // 26454 cascade bands (KH_SUN_CASCADE): halfDiag lanes are the
        // acceptance instrument - pinned at 8 / 32 by construction, any
        // other value is a defect. FLOATS THROUGH kvf (the 26406 trap).
        out.push_back(kv("sunMidValid", RenderIntegration::g_sun3_map_valid ? 1ull : 0ull));
        out.push_back(kv("sunMidCasters", RenderIntegration::g_sun3_casters));
        out.push_back(kv("sunMidRenders", RenderIntegration::g_sun3_renders));
        out.push_back(kvf("sunMidHalfDiag", RenderIntegration::g_sun3_half_diag));
        out.push_back(kv("sunOutValid", RenderIntegration::g_sun4_map_valid ? 1ull : 0ull));
        out.push_back(kv("sunOutCasters", RenderIntegration::g_sun4_casters));
        out.push_back(kv("sunOutRenders", RenderIntegration::g_sun4_renders));
        out.push_back(kvf("sunOutHalfDiag", RenderIntegration::g_sun4_half_diag));
        // 26457 KH_VOL_MIRROR_SURVEY lanes (uints via kv, floats via
        // kvf - the 26349 injNzFloor truncation lesson).
        out.push_back(kv("vmirDssFront", RenderIntegration::g_vmir_dss_front));
        out.push_back(kv("vmirDssBack", RenderIntegration::g_vmir_dss_back));
        // 26458 (section 4.2): vmirDssDepth is REPLACED by the split pair -
        // the packed field put StencilWriteMask past float precision and
        // dump1 published it truncated (rule 1.7: name + value together).
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
        // 26458 KH_VOL_MIRROR (ledger at KH_VMIR_NEAR in rendering):
        out.push_back(kv("vmirDraws", RenderIntegration::g_vmir_draws));
        out.push_back(kv("vmirPrepassDraws", RenderIntegration::g_vmir_prepass_draws));
        out.push_back(kv("vmirMaskFrames", RenderIntegration::g_vmir_mask_frames));
        out.push_back(kv("vmirCsRuns", RenderIntegration::g_vmir_cs_runs));
        out.push_back(kv("vmirB2Off", RenderIntegration::g_vmir_b2_off));
        out.push_back(kv("vmirUavN", RenderIntegration::g_vmir_uav_n));
        out.push_back(kv("vmirUavSkips", RenderIntegration::g_vmir_uav_skips));
        out.push_back(kv("vmirEnsureFails", RenderIntegration::g_vmir_ensure_fails));
        out.push_back(kv("vmirClampStates", RenderIntegration::g_vmir_clamp_states));   // 26461
        // 26462: the live half of the clamp gauge pair + the session gate census.
        out.push_back(kv("vmirClampSwaps", RenderIntegration::g_vmir_clamp_swaps));
        out.push_back(kv("vmirSessionSkips", RenderIntegration::g_vmir_session_skips));
        // 26459: proves the sun anchor ACTIVE in the same capture that
        // tests the shimmer (0.0 here on a valid-camera session = inert).
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
        // 26141 campaign-25 issues 1+2: the freeze epoch decomposed. The lane
        // above is a POSITION magnitude and reads ~0 through any pure pitch
        // rotation, which is why every previous 'the freeze is clean' verdict
        // could not see the vertical axis. Read FreezePitchMaxDeg against
        // FreezeYawMaxDeg: pitch alone means the latch is behind on the
        // vertical axis specifically; both together mean the latch is simply
        // late. fireRefreezes / fireRefreezeFails report dbg 36 uptake.
        out.push_back(kvf("fireFreezeDyMaxM", RenderIntegration::g_fire_freeze_dy_max_m));
        out.push_back(kvf("fireFreezePitchMaxDeg", RenderIntegration::g_fire_freeze_pitch_max_deg));
        out.push_back(kvf("fireFreezeYawMaxDeg", RenderIntegration::g_fire_freeze_yaw_max_deg));
        out.push_back(kvf("fireFreezeDyM", RenderIntegration::g_fire_freeze_dy_m));
        out.push_back(kvf("fireFreezePitchDeg", RenderIntegration::g_fire_freeze_pitch_deg));
        out.push_back(kvf("fireFreezeYawDeg", RenderIntegration::g_fire_freeze_yaw_deg));
        out.push_back(kv("fireFreezeSamples", RenderIntegration::g_fire_freeze_samples));
        out.push_back(kv("fireRefreezes", RenderIntegration::g_fire_refreezes));
        out.push_back(kv("fireRefreezeFails", RenderIntegration::g_fire_refreeze_fails));
        // 26142: the engine-render-view vs boundary-latch pair. The lanes
        // above compare two SIM-side sources and read 0 with the ghost on
        // screen; these compare the epoch actually consumed against the
        // camera the frame was actually drawn with. Read EngPitchMaxDeg
        // against EngYawMaxDeg and EngPosMaxM: pitch alone, with position
        // and yaw near zero, is the first-person neck-pivot signature and
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
        out.push_back(kv("fireSingleSuppressed", RenderIntegration::g_fire_single_suppressed));   // 26143 dbg 40
        // 26144 last-fire clamp. fireClampTarget is the learned index of the
        // frame's last fire; fireClampPaints should track topoCycles almost
        // exactly (one paint per frame) and fireClampRelearns should stay
        // near zero - a large relearn count means the fire count is unstable
        // frame to frame and the clamp is chasing it.
        out.push_back(kv("fireClampTarget", static_cast<uint64_t>(RenderIntegration::g_fire_clamp_target)));
        out.push_back(kv("fireClampPaints", RenderIntegration::g_fire_clamp_paints));
        out.push_back(kv("fireClampRelearns", RenderIntegration::g_fire_clamp_relearns));
        // 26145: backstop paints - frames that reached the learned last fire
        // without the sweep ever verifying. Should be a small fraction of
        // fireClampPaints; if it is most of them, the sweep gate is the
        // problem and not the clamp.
        out.push_back(kv("fireClampUnverified", RenderIntegration::g_fire_clamp_unverified));
        // 26146: fireClampPaints should now track topoCycles closely. If it
        // still falls well short, the fire count is unstable frame to frame
        // (watch fireClampRelearns) and the clamp is chasing it.
        out.push_back(kv("fireDepthRefreshes", RenderIntegration::g_fire_depth_refreshes));
        out.push_back(kv("bandRejStaleView", RenderIntegration::g_band_rej_stale_view));
        // 26150 pitch census. Read the bucket whose range covers look-down
        // (castPitchMinDeg / castPitchMaxDeg name the sign convention):
        //   Holds >> 0        -> the fire is being HELD on look-down frames,
        //                        and castPitch*Miss names the gate doing it.
        //   Holds ~0, Paints
        //   ~= Frames         -> we paint on those frames and the engine is
        //                        not consuming the mask; the defect is
        //                        downstream of everything this file controls.
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
        // 26151: per-bucket, how many frames had ANY fire land while the
        // engine held the mask SRV-bound. Compare against the bucket's
        // Frames count - that ratio, not the paint count, is what the
        // shadow follows.
        out.push_back(kv("castPitchNegSrv", RenderIntegration::g_cast_pitch_neg_srv));
        out.push_back(kv("castPitchPosSrv", RenderIntegration::g_cast_pitch_pos_srv));
        out.push_back(kv("fireSrvClampPaints", RenderIntegration::g_fire_srv_clamp_paints));
        out.push_back(kv("fireSrvClampFallbacks", RenderIntegration::g_fire_srv_clamp_fallbacks));
        // 26163: second paints added on fire-1 frames so the NEXT frame has
        // something to read. Should track fireSrvClampPaints closely.
        out.push_back(kv("fireCarryPaints", RenderIntegration::g_fire_carry_paints));   // 26163 retired
        // 26164: extra early paints on frames that follow an early-paint
        // frame. Should be a handful a session, not hundreds - if it tracks
        // fireSrvClampPaints the targeting is wrong again.
        out.push_back(kv("firePrimePaints", RenderIntegration::g_fire_prime_paints));
        // 26153: fireSrvIndex is the learned fire index of the engine's apply
        // window. fireSrvIndexMoves should settle to a small number - a value
        // that keeps moving means the window wanders per frame and a single
        // learned index cannot track it. Of the backstop paints,
        // fireSrvBackstopAtIndex are the aimed ones and fireSrvBackstopAtLast
        // are the remaining blind shots at the end of the frame; the second
        // number is what the residual flicker comes out of.
        out.push_back(kv("fireSrvIndex", static_cast<uint64_t>(RenderIntegration::g_fire_srv_index)));
        out.push_back(kv("fireSrvIndexMoves", RenderIntegration::g_fire_srv_index_moves));
        // 26154: retired - the learned index no longer steers the backstop
        // (it was degenerate at 1). Kept at 0 so the lane order is stable.
        out.push_back(kv("fireSrvBackstopAtIndex", RenderIntegration::g_fire_srv_backstop_at_index));
        // 26155 MASK-SRV BIND CENSUS (ledger at g_mask_srv_bind_hits). The
        // go/no-go for re-keying the arm is maskSrvBindFrames against
        // flushes: at parity the engine announces its apply window on every
        // frame and the fire can be hung off it; well below parity the
        // re-key would starve the fire and the idea is dead. maskSrvDryFrames
        // counts, directly, the frames it would have starved.
        out.push_back(kv("maskSrvBindHits", RenderIntegration::g_mask_srv_bind_hits));
        out.push_back(kv("maskSrvBindFrames", RenderIntegration::g_mask_srv_bind_frames));
        out.push_back(kv("maskSrvDryFrames", RenderIntegration::g_mask_srv_dry_frames));
        out.push_back(kv("maskSrvBindsMax", static_cast<uint64_t>(RenderIntegration::g_mask_srv_binds_max)));
        out.push_back(kvf("maskSrvFirstD", RenderIntegration::g_mask_srv_first_d_pub));
        out.push_back(kvf("maskSrvLastD", RenderIntegration::g_mask_srv_last_d_pub));
        // The fire indices off the SAME frame - this is the whole picture of
        // the ordering the campaign has been guessing at.
        out.push_back(kvf("fireFirstDPub", RenderIntegration::g_fire_first_d_pub));
        out.push_back(kvf("fireLastDPub", RenderIntegration::g_fire_last_d_pub));
        // 26156: the mask as a RENDER TARGET - when a paint stops existing.
        // Tracked per frame since 26133, never emitted outside the flight
        // recorder. With maskSrvFirstD/LastD, topoFirstSweep/LastSweep and
        // fireFirstDPub/LastDPub this is the complete frame on one draw axis,
        // which is what the 26151 census was standing in for and getting wrong.
        out.push_back(kv("maskRtBindsPub", static_cast<uint64_t>(RenderIntegration::g_mask_rt_binds_pub)));
        out.push_back(kvf("maskLastBindDPub", RenderIntegration::g_mask_last_bind_d_pub));
        out.push_back(kvf("maskFirstBindDPub", RenderIntegration::g_mask_first_bind_d_pub));   // 26162
        out.push_back(kv("firePaintIxPub", static_cast<uint64_t>(RenderIntegration::g_fire_paint_ix_pub)));
        out.push_back(kv("firePaintPathPub", static_cast<uint64_t>(RenderIntegration::g_fire_paint_path_pub)));
        // 26158 (ledger at g_mask_srv_first_d_pub). fireSrvGapMax is the
        // margin the flicker turns on: draws between the engine's last
        // mask read and our first fire.
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
        // 26179: sun-map maturity. sunMapMature is localityMeta.z as the SHADER
        // received it - 0 means the far-plane presence test is DISARMED and the
        // longest shadow is being cut at c.z = 1. PREDICTION: spawning a second
        // caster increments sunMatDemCchg once and drops sunMapMature to 0 for
        // ~0.5 s. sunMapCasterRolls counts the caster changes the 26179 roll
        // held mature instead.
        out.push_back(kvf("sunMapMature", RenderIntegration::g_sun_map_mature_pub));
        out.push_back(kv("sunMatDemNew", RenderIntegration::g_sun_mat_dem_new));
        out.push_back(kv("sunMatDemCchg", RenderIntegration::g_sun_mat_dem_cchg));
        out.push_back(kv("sunMatDemBig", RenderIntegration::g_sun_mat_dem_big));
        out.push_back(kv("sunMatDemDrift", RenderIntegration::g_sun_mat_dem_drift));
        out.push_back(kv("sunMapCasterRolls", RenderIntegration::g_sun_mat_caster_rolls));
        out.push_back(kv("relockRecvWipes", RenderIntegration::g_relock_recv_wipes));
        // GRID-COHERENCE CENSUS (campaign 5): the S4 conviction numbers.
        out.push_back(kv("fireDimsIncoherent", RenderIntegration::g_fire_dims_incoh_total));
        out.push_back(kvf("fireDimsDivMaxPx", RenderIntegration::g_fire_dims_div_max));
        out.push_back(kv("castDimsForeign", RenderIntegration::g_cast_dims_foreign));
        out.push_back(kv("fireVpMode", static_cast<uint64_t>(RenderIntegration::g_fire_vp_mode)));
        out.push_back(kvf("castDimsLiveW", RenderIntegration::g_mask.cast_dims[0]));
        out.push_back(kvf("castDimsLiveH", RenderIntegration::g_mask.cast_dims[1]));
        out.push_back(kvf("fireDims2W", RenderIntegration::g_fire_dims2[0]));
        out.push_back(kvf("fireDims2H", RenderIntegration::g_fire_dims2[1]));
        // LOCK-SETTLE GATE (campaign 5): churn recency + fires held.
        out.push_back(kvf("lockChurnAgeS", age_s(RenderIntegration::g_lock_churn_ms_v)));
        out.push_back(kv("castLockSettleHolds", RenderIntegration::g_cast_lock_settle_holds));
        // BRIDGE-EPOCH FREEZE health: bridge adoption is the norm;
        // pub fires / era rejects / repairs are anomaly counters.
        out.push_back(kv("fireViewBridgeFires", RenderIntegration::g_fire_view_bridge_fires));
        out.push_back(kv("fireViewPubFires", RenderIntegration::g_fire_view_pub_fires));
        out.push_back(kv("fireViewBridgeRepairs", RenderIntegration::g_fire_view_repairs));
        out.push_back(kv("fireViewEraRejects", RenderIntegration::g_fire_view_era_rejects));
        // CAST READINESS LATCH (26026): ready state + age, holds by
        // class (57 readiness, 58 stale-map), and latch drops.
        out.push_back(kv("castReady", static_cast<uint64_t>(RenderIntegration::g_cast_ready ? 1 : 0)));
        out.push_back(kvf("castReadyAgeS", age_s(RenderIntegration::g_cast_ready_ms)));
        out.push_back(kv("castReadyHolds", RenderIntegration::g_cast_ready_holds));
        out.push_back(kv("castMapHolds", RenderIntegration::g_cast_map_holds));
        out.push_back(kv("castReadyDrops", RenderIntegration::g_cast_ready_drops));
        // 26155 DWELL TERM CENSUS (ledger at g_cast_dwell_churn). castReadyHolds
        // counts the unstable branch and the still-elapsing branch together and
        // so can never name the flapping term. castDwellResets is the number that
        // matters for a 64 s cold: a dwell that never elapses is being restarted,
        // and exactly one of the three terms below is doing it.
        out.push_back(kv("castDwellChurn", RenderIntegration::g_cast_dwell_churn));
        // 26162: WHICH site stamps the churn the dwell keys on, and how many
        // land inside the 1.5 s window that makes them compound.
        out.push_back(kv("churnStampRelock", RenderIntegration::g_lock_churn_stamp_relock));
        out.push_back(kv("churnStampDrop", RenderIntegration::g_lock_churn_stamp_drop));
        out.push_back(kv("churnStampPrewarm", RenderIntegration::g_lock_churn_stamp_prewarm));
        out.push_back(kv("churnStampRing", RenderIntegration::g_lock_churn_stamp_ring));
        out.push_back(kv("churnStampsClose", RenderIntegration::g_lock_churn_stamps_close));
        out.push_back(kv("castDwellTravel", RenderIntegration::g_cast_dwell_travel));
        out.push_back(kv("castDwellJump", RenderIntegration::g_cast_dwell_jump));
        out.push_back(kv("castDwellResets", RenderIntegration::g_cast_dwell_resets));
        out.push_back(kv("castDwellTicks", RenderIntegration::g_cast_dwell_ticks));
        // 26155: frames the fire was never ARMED on (miss 2). Without this,
        // castMisses on an unarmed frame is stale residue from the last armed
        // one - dump59 read 53 while the true state was "no view, never armed".
        out.push_back(kv("castUnarmedFrames", RenderIntegration::g_cast_unarmed_frames));
        // SUN-AXIS PROVENANCE + SNAP CONFIRMATION (campaign 5 round 2).
        out.push_back(kv("liveRejSunAxis", RenderIntegration::g_live_rej_sun_axis));
        out.push_back(kvf("liveRejSunAxisLastDeg", RenderIntegration::g_live_rej_sun_axis_deg));
        out.push_back(kv("bandRejSunAxis", RenderIntegration::g_band_rej_sun_axis));
        out.push_back(kvf("bandRejSunAxisLastDeg", RenderIntegration::g_band_rej_sun_axis_deg));
        // 26141 campaign-25 issue 3: the ingress census. bandRejNoView /
        // liveRejNoView count commits refused because no view existed to
        // rotate the cascade axis with - the path dump116 showed sealing
        // unverified. sunAxisBootRefs counts tests answered against the
        // bootstrap sun, sunAxisColdRejects those that needed the tightened
        // cold bar, and sunAxisLastRefDeg is the last angle measured at all
        // (accepted or refused), so a near-miss is visible before it seals.
        out.push_back(kv("bandRejNoView", RenderIntegration::g_band_rej_no_view));
        out.push_back(kv("liveRejNoView", RenderIntegration::g_live_rej_no_view));
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
        // COLD TIMELINE (campaign 5 round 2): stage times relative to
        // cold_t0; -1 = never happened; negative = happened before this
        // cold window opened (a later cold in a warm session).
        {
            auto khcold = [](float khc_t) {
                return khc_t >= 0.0f && RenderIntegration::g_mask.cold_t0 >= 0.0
                    ? khc_t - static_cast<float>(RenderIntegration::g_mask.cold_t0)
                    : -1.0f;
            };
            out.push_back(kvf("coldSunBootS", khcold(RenderIntegration::g_first_sun_boot_t)));   // 26133
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
        // 26179 COLD RECEIVE HEALTH (setRenderDebug 47 reverts). bandSealUnlocked
        // counts seals taken with no view lock - the 26149 cold escape hatch, and
        // the prime suspect for a receive band showing a foreign part of the
        // world. bandColdHolds is the receive standing down for it; recvColdWipes
        // is the cold-era seals dropped when the latch rose. coldRecvHealthS is
        // when that happened, on the same cold timeline as coldFirstBandCapS -
        // compare the two: the gap is the window that used to be consumable.
        out.push_back(kv("bandSealUnlocked", RenderIntegration::g_band_seal_unlocked));
        out.push_back(kv("bandColdHolds", RenderIntegration::g_band_cold_holds));
        // 26180: half-built seals withheld from the receive (setRenderDebug 48
        // reverts). EXPECT this to land near the old bandPendConsumed with
        // bandPickNone unchanged; bandPickNone climbing by about the same
        // amount means the offset was traded for a one-cycle dropout.
        out.push_back(kv("bandPendRefused", RenderIntegration::g_band_pend_refused));
        // 26181 DEFERRED CAPTURE (setRenderDebug 50 reverts to the 26180
        // dropout, 48 all the way to the 26179 offset). EXPECT commits ~=
        // takes, bandPendRefused ~0 and bandPickNone back to its 26179
        // value. bandStageDropAge large against takes = the pairing view is
        // not arriving inside the window, and the double buffer has to be
        // paid for after all. bandStageAtlasMoved is measured, not acted on:
        // it counts commits whose cascade re-rendered between the resolve
        // that paired them and the copy, against a receive that is already
        // 50-250 ms stale between reseals by design.
        out.push_back(kv("bandStageTakes", RenderIntegration::g_band_stage_takes));
        out.push_back(kv("bandStageCommits", RenderIntegration::g_band_stage_commits));
        out.push_back(kv("bandStageDropAge", RenderIntegration::g_band_stage_drop_age));
        out.push_back(kv("bandStageDropDead", RenderIntegration::g_band_stage_drop_dead));
        // 26182: RETIRED and published at 0 (lane order is a dump contract).
        // It read 5633 of 6278 commits at 26181 and convicted the deferred
        // copy: the copy is back at the resolve that pairs sm with content,
        // so matrix/content drift is structurally impossible now.
        out.push_back(kv("bandStageAtlasMoved", RenderIntegration::g_band_stage_atlas_moved));
        // 26182: reseals skipped because no staging back buffer was free.
        // A miss is benign - the slot keeps its previous COMPLETE seal - so
        // this is freshness, not correctness.
        // 26187: READ THIS AS A DWELL-WEIGHTED SPIN COUNT, NOT A FAILURE
        // RATE. last_time is not advanced on a miss, so a blocked reseal
        // re-attempts every frame it stays blocked and each attempt adds one
        // to BOTH sides of poolMiss / (poolMiss + takes). At ~2-3 spins per
        // blocked reseal, dump558's 40.2% is nearer 15-20% of reseals truly
        // deferred. Do not size KH_BAND_STAGE_POOL off this ratio; a lane
        // counting a miss ONCE per slot per blocked era is the number that
        // would justify the +67 MB.
        out.push_back(kv("bandStagePoolMiss", RenderIntegration::g_band_stage_pool_miss));
        // 26396: misses served by the immediate capture rather than dropped.
        // Should track bandStagePoolMiss; a gap means 214 is set.
        out.push_back(kv("bandStagePoolFall", RenderIntegration::g_band_stage_pool_fall));
        // 26187: increments once per STAGED SLOT PER ENGINE DRAW while a
        // commit waits on the exact-class publish bar - kh_band_stage_commit
        // runs from reorder_pre_draw. It is a dwell measure in slot-draws;
        // any ratio against takes or commits is meaningless.
        out.push_back(kv("bandStageHoldQuality", RenderIntegration::g_band_stage_hold_quality));
        out.push_back(kvf("bandStageWaitMs", RenderIntegration::g_band_stage_wait_ms));
        out.push_back(kvf("bandStageWaitMaxMs", RenderIntegration::g_band_stage_wait_max_ms));
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
        //   encOvrVetoes/encOvrVetoNear - 26314: encSrc-4 collapse overrides
        //   stood down by the measured witness (baseline fallback), and the
        //   last vetoed (= shipped) candidate near. Expect vetoes counting on
        //   FP stance collapses and Zeus glide flips; overrides ~0.
        out.push_back(kv("encOvrVetoes", RenderIntegration::g_inj_enc_ovr_vetoes));
        out.push_back(kvf("encOvrVetoNear", RenderIntegration::g_inj_enc_ovr_veto_near));
        //   worldLatchWits - 26314: class-refused world-latch captures accepted
        //   on the engine's own measured-pair agreement (the glide-flip escape).
        out.push_back(kv("worldLatchWits", RenderIntegration::g_world_latch_wits));
        //   prb* - 26315 content-probe health (ledger at KH_BUILD_TAG). copies
        //   ~ flushes when the box is on screen; maps ~ copies (readbacks
        //   landing); busy = in-flight skips; skips = off-screen/cold/refused.
        //   svVolCopyHeld - 26324: later-bracket vol copies held by the
        //   first-bracket latch (expect ~brackets-1 per frame; 0 under 170).
        out.push_back(kv("svVolCopyHeld", RenderIntegration::g_svs_vol_copy_held));
        out.push_back(kv("prbCopies", RenderIntegration::g_prb_copies));
        out.push_back(kv("prbMaps", RenderIntegration::g_prb_maps));
        out.push_back(kv("prbBusy", RenderIntegration::g_prb_busy));
        out.push_back(kv("prbSkips", RenderIntegration::g_prb_skips));
        //   prbPre* - 26325: why the 26324 pre-injection clip decider is
        //   silent. prbPreBotZ/DM read -1 on every row of dump1, including
        //   rows where the flush probes landed, so the lane was never a
        //   verdict. prbPreSamples > 1 or prbPreArray > 1 = the single-sample
        //   gate refused (main depth is MSAA); prbPreIssued counting with the
        //   lane still -1 = the 1x1 copy itself, which D3D11 does not permit
        //   out of a depth-stencil resource (whole subresource only).
        //   prbPreFmt is the DSV resource's DXGI format. Read these BEFORE
        //   reading any preBot verdict.
        out.push_back(kv("prbPreFmt", RenderIntegration::g_prb_pre_fmt));
        out.push_back(kv("prbPreSamples", RenderIntegration::g_prb_pre_samples));
        out.push_back(kv("prbPreArray", RenderIntegration::g_prb_pre_array));
        out.push_back(kv("prbPreIssued", RenderIntegration::g_prb_pre_issued));
        out.push_back(kv("prbPreSkips", RenderIntegration::g_prb_pre_skips));
        out.push_back(kv("liveNearRefAdopts", RenderIntegration::g_live_ref_adopts));
        out.push_back(kv("worldPairEncodes", RenderIntegration::g_world_pair_encodes));
        out.push_back(kvf("farKeepFar", RenderIntegration::g_far_keep_far));
        out.push_back(kvf("liveNearRef", RenderIntegration::g_live_near_ref));
        out.push_back(kvf("fogEngX", RenderIntegration::g_fog_eng_dbg[0]));
        out.push_back(kvf("fogEngEnd", RenderIntegration::g_fog_eng_dbg[1]));
        out.push_back(kvf("fogEngInv", RenderIntegration::g_fog_eng_dbg[2]));
        out.push_back(kvf("fogEngOn", RenderIntegration::g_fog_eng_dbg[3]));
        out.push_back(kvf("latchHoldLastDist", RenderIntegration::g_latch_hold_dist));
        out.push_back(kv("latchJumpAdopts", RenderIntegration::g_latch_jump_adopts));
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
        // 26187: sightings whose cadence mark decayed below its previous
        // value - the count of transient tails being repriced downward.
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
            // Band-layout census (the 132 m band session): the widest
            // valid band's span and the layout's total reach - the
            // receive decode was built against 8-35 m bands.
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
            // table). Diagnostic read of render-written state, like the
            // rest of the stats.
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

        // ===== 26052 DEBUG DEEPENING: instantaneous state sections. Every
        // key below is a plain read of standing state at call time - no new
        // per-frame cost anywhere; the one lock taken (cache census) parks
        // the render thread exactly like dumpDynamicLights' snapshot does.

        // Collapse-guard forensics: the standing reference pair, the
        // pending jump's age in both layers' terms, and the witness
        // recency the guard tests against (15 s bar).
        out.push_back(kv("fkVetoCandLast", static_cast<uint64_t>(RenderIntegration::g_fk_veto_cand_n)));
        // 26187: flushes where the mirror was NOT the arbitration's certified
        // standing, so the published block held instead of shipping
        // uncertified pass content (the slight camera-rotation dimming).
        // FALSIFIED IF this reads ~0 while the operator still sees the dim:
        // then the std/blk divergence is an artefact of the two ring sample
        // sites, not an intra-frame mirror overwrite, and the model is wrong.
        out.push_back(kv("blkIncohHolds", RenderIntegration::g_blk_incoh_holds));
        out.push_back(kvf("stdAmbLum", RenderIntegration::g_light_probe.std_amb_l));
        out.push_back(kvf("stdSunLum", RenderIntegration::g_light_probe.std_sun_l));
        out.push_back(kvf("blkPendAgeS", age_s(RenderIntegration::g_blk_pend_ms)));
        out.push_back(kvf("sunLastJumpAgeS", age_s(RenderIntegration::g_sun_last_jump_ms)));

        {   // MESH REGISTRY CENSUS: published-contract reads (lock-free by
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
                    if (khdl_o.affect_ui && khdl_o.ui_only) khdl_uiOnly++;   // 26055
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
            out.push_back(kv("objUiOnly", khdl_uiOnly));   // 26055: UI-mode passes
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
            // thread through the graphics lock (dumpDynamicLights'
            // snapshot pattern). -1 everywhere = lock unavailable this
            // call; the keys heal on the next call.
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
        // queueVisibility / getVisibilityResults pump; all game-thread
        // state, same thread as this call).
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

        // INJECTION HEALTH + ACCEPTED ENCODE: age of the last
        // pre-translucent injection (the flush's stand-down bar is 0.5 s)
        // and the last committed pair's far in meters (injDpM22/M32 above
        // are the raw coefficients; this is the derived number).
        out.push_back(kvf("injLastAgeS", age_s(RenderIntegration::g_composite_last_inject_ms.load(std::memory_order_relaxed))));
        out.push_back(kvf("injAccFarM", RenderIntegration::g_inj_dp_valid
            ? RenderIntegration::kh_enc_far(RenderIntegration::g_inj_dp[0], RenderIntegration::g_inj_dp[1])
            : -1.0f));

        // CAMERA ECHO: the injection-recorded render camera (engine axes;
        // Y = altitude ASL) - locates every distance- and altitude-tuned
        // gate in the same dump that reports them.
        out.push_back(kvf("camEngX", RenderIntegration::g_ls.cam[0]));
        out.push_back(kvf("camEngAltY", RenderIntegration::g_ls.cam[1]));
        out.push_back(kvf("camEngZ", RenderIntegration::g_ls.cam[2]));

        // RING GEOMETRY + TIME ANCHORS: lets a paired stats + trace read be
        // aligned without guessing (ffrFrames above is the serial; the
        // ring capacity here says how far back a dump can reach).
        out.push_back(kv("ffrRingFrames", static_cast<uint64_t>(RenderIntegration::KH_FFR_RING)));
        out.push_back(kvf("effectTimeS", RenderIntegration::effect_time_seconds()));
        return game_value(std::move(out));
    } catch (...) {
        report_error("getRenderStats: unknown exception");
        return game_value(auto_array<game_value>());
    }
}

// dumpRenderTrace -> the flight recorder ring, oldest-first, newest last:
// [["status","ok"], ["buildTag", N], ["fields", [names...]],
//  ["frames", [[values...], ...]]] (buildTag added 26052: the field
// protocol's build verification rides every dump, not just stats).
// Each frame's values align 1:1 with "fields". The ring is copied under the
// graphics lock (parking the render thread - the ring's write invariant)
// and formatted after release; the newest 256 populated records are dumped.
static game_value dump_render_trace_sqf() {
    try {
        // OPT-IN (see g_diag_armed): the recorder is idle until armed, so
        // the first call starts it and reports that; frames flow from the
        // next call on.
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

        // append-only past the deltas (shadow-jitter campaign): keep in
        // step with the per-frame pushes after the delta loop below.
        names.push_back(game_value("camStepM"));
        names.push_back(game_value("fireCamDeltaM"));
        names.push_back(game_value("fireDimsDivPx"));
        names.push_back(game_value("fireVpMode"));
        names.push_back(game_value("maskRtBinds"));
        names.push_back(game_value("maskLastBindD"));
        names.push_back(game_value("fireFirstD"));
        names.push_back(game_value("fireLastD"));
        names.push_back(game_value("firePaintIx"));     // 26157
        names.push_back(game_value("firePaintPath"));   // 1 SRV-bound, 2 backstop
        names.push_back(game_value("maskSrvFirstD"));   // 26158
        names.push_back(game_value("maskSrvLastD"));
        names.push_back(game_value("fireSrvGapD"));
        names.push_back(game_value("maskFirstBindD"));   // 26162
        names.push_back(game_value("pubFirstD"));        // 26166 publish census
        names.push_back(game_value("pubRejFirstD"));
        names.push_back(game_value("pubSlotHits"));
        names.push_back(game_value("pubPreAccept"));     // 0 = slot was SILENT
        names.push_back(game_value("pubRejTmag"));
        names.push_back(game_value("pubRejFamily"));
        names.push_back(game_value("pubRejCold"));
        names.push_back(game_value("pubAltFirstD"));   // 26167
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
        names.push_back(game_value("fkRouted"));     // C8 (26049) lanes
        names.push_back(game_value("fkVetoN"));
        names.push_back(game_value("fkFar"));
        names.push_back(game_value("slcNear"));
        names.push_back(game_value("slcFar"));
        names.push_back(game_value("cycVpLo"));
        names.push_back(game_value("cycVpHi"));
        names.push_back(game_value("skyTrigs"));     // sky-window census (26051)
        names.push_back(game_value("vetoCandN"));    // 26052 lanes (ledger at FfrRecord)
        names.push_back(game_value("camAltM"));
        names.push_back(game_value("stdAmbLum"));
        names.push_back(game_value("stdSunLum"));
        names.push_back(game_value("blkPendAgeS"));

        // 26291 append-only tail: registration + first-person forensics.
        // reprojPx/swingPx have been recorded per frame since 26290 and never
        // dumped; the rest are this build's lanes. Keep 1:1 with the pushes
        // below - fieldsN/frameN now verify that in-band on every dump.
        names.push_back(game_value("reprojPx"));      // |footprint - visible box| this frame, px
        names.push_back(game_value("swingPx"));       // what view adoption moved the footprint, px
        names.push_back(game_value("seamPvStamp"));   // latch write serial at the seam's read (0 = seam idle)
        names.push_back(game_value("compPvStamp"));   // latch write serial at the colour read (0 = no injection)
        names.push_back(game_value("seamCamDxM"));    // seam-published vs colour camera, m (-1 = no publication)
        names.push_back(game_value("seamViewAge"));   // seam-view age at the colour pass, frames (-1 = absent)
        names.push_back(game_value("seamSrc"));       // 0 none 1 adopted 2 cycle 3 live 4 boundary 5 share
        names.push_back(game_value("compSrc"));       // 0 latch 1 live 2 boundary 3 adopted 4 seam-copy 5 bpv-override
        names.push_back(game_value("volCode"));       // shader arm this frame
        names.push_back(game_value("volWit"));        // footprint witness present this frame
        names.push_back(game_value("volAge"));        // volume-copy age, frames (-1 = none)
        names.push_back(game_value("seamLiveDxM"));   // 26292: live translation correction applied, m (-1 = stood down)
        names.push_back(game_value("seamNear"));      // 26293: the seam's committed near - injNear's twin
        names.push_back(game_value("seamProjSrc"));   // 26293: 0 latch pair 1 engine sniff 2 live pair
        names.push_back(game_value("sniffNear"));     // 26300: the engine's sniffed pair at the seam (-1 = invalid)
        names.push_back(game_value("primeCamDxM"));   // 26294: |colour rebase cam - seam prime cam|, m
        names.push_back(game_value("seamRuns"));      // 26294: seam executions folded into this row
        // 26304 append-only tail: pair-arbitration forensics + bound-CB census.
        names.push_back(game_value("seamLiveNear"));   // raw live-bridge near at the seam fetch (-1 = none)
        names.push_back(game_value("seamPairWhy"));    // 0 live-corr 1 live-base 2 insane 3 corridor 4 no-ref 5 no sample
        names.push_back(game_value("boundPairNear"));  // bound-CB census pair at the seam (-1 = no match)
        names.push_back(game_value("boundPairAgeMs")); // age of that buffer's last recorded upload
        names.push_back(game_value("censusMatch"));    // bit0-3 VS b0-b3, bit4-7 PS b0-b3 with a census hit
        // 26306 append-only tail: census camera + trajectory-bound forensics.
        names.push_back(game_value("boundCamDxM"));    // |census camera - seam draw camera| (m; -1 = none)
        names.push_back(game_value("boundCamLiveDxM"));// |census camera - live fetch camera| (m; -1 = none)
        names.push_back(game_value("boundCamOk"));     // census camera validated this frame
        names.push_back(game_value("seamTrnEvent"));   // 0 none 1 bound-refused 2 wide-take 3 escape-take, 5 witness-take (26443)
        names.push_back(game_value("seamTrnWit"));     // 26443: floor-refusal witness verdict (-1 none, 0 fail/abstain, 1 pass)
        // 26310 append-only tail: the flavor gauge (ledger at the seam's
        // translation anchor). |adopted-view camera - cycle latch camera|,
        // metres; -1 = no latch reference that run.
        names.push_back(game_value("seamAnchDxM"));
        // 26315 content-probe tail (one frame latent; ledger at KH_BUILD_TAG)
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
        // 26325 append-only tail: the colour pass's applied FOV ratio.
        // seamFovLatR says how far apart frame k and k+1 are in scale;
        // compFovR says whether the VISIBLE box closed that gap this frame
        // (-1 = refused or absent, i.e. it rasterised at the old scale).
        names.push_back(game_value("compFovR"));
        // 26326 append-only tail: the near the seam's arbitration chain would
        // have encoded on frames the sniff referee replaced it (-1 = none).
        // seamNear now always reports what actually shipped.
        names.push_back(game_value("seamSnifOvr"));
        // 26328 append-only tail: the ENGINE's own volume-pass pair, measured
        // at the injection instant (one frame latent, -1 = no sample). Read
        // volPassNear against seamNear on the SAME row - that comparison is
        // the whole question this campaign has been unable to answer.
        names.push_back(game_value("volPassNear"));
        names.push_back(game_value("volPassM22"));
        // 26329: the readback age in TRACE ROWS - the sample belongs to row
        // (serial - age). Without it the alternating near classes let two
        // different alignments fit the same data, which is what left 26328
        // unreadable. volDrawNear is the same measurement at the engine's
        // first COUNTING draw, which is where the value is consumed.
        names.push_back(game_value("volPassAge"));
        names.push_back(game_value("volDrawNear"));
        names.push_back(game_value("volDrawAge"));
        // 26331: the box centre's clip.w through the ENGINE's own matrix,
        // taken both ways, against its known distance. Whichever of Abs/Rel
        // equals True names the space the engine's matrix expects - and so
        // which of mode 174 / 175 transforms our footprint correctly.
        names.push_back(game_value("engVpWAbs"));
        names.push_back(game_value("engVpWRel"));
        names.push_back(game_value("engVpWTrue"));
        // 26342 THE ROUND-TRIP LANES (ledger at kh_view_camera_exact).
        // compCamRtM / seamCamRtM = |transpose camera - exact camera| for the
        // composite's and the seam's own view, in metres: the error the
        // 26341 translation rebuild was injecting, per frame. camAbsM is the
        // amplifier it multiplies (the absolute map magnitude of the camera).
        // compTakeSrc: 0 no take, 1 snapped (bit-identical basis, translation
        // untouched), 2 rebuilt through the exact camera, 3 refused.
        names.push_back(game_value("compCamRtM"));
        names.push_back(game_value("seamCamRtM"));
        names.push_back(game_value("camAbsM"));
        names.push_back(game_value("compTakeSrc"));
        // 26342: the depth-encode route the VISIBLE box took this frame
        // (0 ordinary, 1 near-gap SV_Depth, 2 far-keep). The seam has no such
        // routing, so a non-zero here is a frame where the box and its own
        // footprint were in different encodes - the standing suspect for the
        // sub-1 m zoom/fire size pop if 26342 does not close it.
        names.push_back(game_value("compNearZ"));
        // 26362 APPEND-ONLY TAIL: the caster centre through the ENGINE'S own
        // view-projection versus through the SEAM'S, in pixels. Compare
        // volDrawSwingPx against swingPx ROW BY ROW - that pair is the whole
        // question. -1 = no sample this row.
        names.push_back(game_value("seamFetchDupM"));   // 26365 (mis-scoped)
        names.push_back(game_value("compCamStepM"));    // 26373
        names.push_back(game_value("seamTrnDxM"));      // 26366
        names.push_back(game_value("seamTrnBndM"));
        names.push_back(game_value("seamFetchN"));       // 26367 seam
        names.push_back(game_value("seamBoundN"));
        names.push_back(game_value("bandPickAgeMs"));    // 26367 cascades
        names.push_back(game_value("bandPickCamDxM"));  // 26369
        names.push_back(game_value("bandAltCamDxM"));   // 26375
        names.push_back(game_value("bandAltBetter"));
        names.push_back(game_value("bandPickNoneN"));
        names.push_back(game_value("bandPoolMissN"));
        names.push_back(game_value("bandStageWaitMs"));
        names.push_back(game_value("stenTol"));          // 26367 blur
        names.push_back(game_value("volPassSwingPx"));
        names.push_back(game_value("volDrawSwingPx"));
        // 26387 append-only tail: receive coverage. Keep 1:1 with the
        // pushes below - fieldsN/frameN verify it in-band on every dump.
        names.push_back(game_value("recvBandsN"));      // bands committed this frame
        names.push_back(game_value("recvBandNear"));    // finest committed band near (m)
        names.push_back(game_value("recvDropWhy"));     // bitmask of the gates that dropped one
        names.push_back(game_value("recvBandAgeMs"));   // 26388: seal age of the finest band
        names.push_back(game_value("recvBandCamDxM"));  // 26389: its baked camera vs the pass camera
        names.push_back(game_value("vmirDraws"));       // 26458: re-issued counting draws
        names.push_back(game_value("blkMirSunLum"));    // 26479: mirror at the row latch
        names.push_back(game_value("blkAnchSunLum"));   // 26479: anchor at the row latch
        names.push_back(game_value("dBlkApplies"));     // 26479: applies this frame
        names.push_back(game_value("dBlkAnchorRej"));   // 26479: anchor rejects this frame

        // 26291 THE COUNT IS VERIFIED IN-BAND. trace_field_audit hand-counted
        // push CALL SITES against non-delta names and read the delta loop as
        // one push - 96 vs 97 was a counting artifact, not a skew (the dump
        // was verified aligned column-by-column at 26291). fieldsN/frameN are
        // the machine's own numbers riding every dump, so alignment can never
        // again be a hand-count debate.
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
            f.push_back(game_value(static_cast<float>(r.fire_paint_ix)));     // 26157
            f.push_back(game_value(static_cast<float>(r.fire_paint_path)));
            f.push_back(game_value(r.mask_srv_first_d));   // 26158
            f.push_back(game_value(r.mask_srv_last_d));
            f.push_back(game_value(r.fire_srv_gap_d));
            f.push_back(game_value(r.mask_first_bind_d));   // 26162
            f.push_back(game_value(r.pub_first_d));        // 26166 publish census
            f.push_back(game_value(r.pub_rej_first_d));
            f.push_back(game_value(static_cast<float>(r.pub_slot_hits)));
            f.push_back(game_value(static_cast<float>(r.pub_pre_accept)));
            f.push_back(game_value(static_cast<float>(r.pub_rej_tmag)));
            f.push_back(game_value(static_cast<float>(r.pub_rej_family)));
            f.push_back(game_value(static_cast<float>(r.pub_rej_cold)));
            f.push_back(game_value(r.pub_alt_first_d));   // 26167
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
            f.push_back(game_value(static_cast<float>(r.fk_routed)));   // C8 (26049) lanes
            f.push_back(game_value(static_cast<float>(r.fk_veto_n)));
            f.push_back(game_value(r.fk_far));
            f.push_back(game_value(r.slc_near));
            f.push_back(game_value(r.slc_far));
            f.push_back(game_value(r.cyc_vp_lo));
            f.push_back(game_value(r.cyc_vp_hi));
            f.push_back(game_value(static_cast<float>(r.sky_trigs)));   // sky-window census (26051)
            f.push_back(game_value(static_cast<float>(r.veto_cand_n)));  // 26052 lanes
            f.push_back(game_value(r.cam_alt_m));
            f.push_back(game_value(r.std_amb_lum));
            f.push_back(game_value(r.std_sun_lum));
            f.push_back(game_value(r.blk_pend_age_s));
            // 26291 tail - 1:1 with the names above.
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
            f.push_back(game_value(r.seam_live_dx_m));   // 26292
            f.push_back(game_value(r.seam_near));                          // 26293
            f.push_back(game_value(static_cast<float>(r.seam_proj_src)));  // 26293
            f.push_back(game_value(r.sniff_near));                         // 26300
            f.push_back(game_value(r.prime_cam_dx_m));                     // 26294
            f.push_back(game_value(static_cast<float>(r.seam_runs)));      // 26294
            f.push_back(game_value(r.seam_live_near));                     // 26304 tail
            f.push_back(game_value(static_cast<float>(r.seam_pair_why)));
            f.push_back(game_value(r.bound_pair_near));
            f.push_back(game_value(r.bound_pair_age_ms));
            f.push_back(game_value(static_cast<float>(r.census_match)));
            f.push_back(game_value(r.bound_cam_dx_m));                     // 26306 tail
            f.push_back(game_value(r.bound_cam_live_dx_m));
            f.push_back(game_value(static_cast<float>(r.bound_cam_ok)));
            f.push_back(game_value(static_cast<float>(r.seam_trn_event)));
            f.push_back(game_value(r.seam_trn_wit));   // 26443
            f.push_back(game_value(r.seam_anch_dx_m));                     // 26310 tail
            f.push_back(game_value(r.prb_ctr_z));                          // 26315 tail
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
            f.push_back(game_value(r.comp_fov_r));                        // 26325 tail
            f.push_back(game_value(r.seam_sniff_ovr));                    // 26326 tail
            f.push_back(game_value(r.vol_pass_near));                     // 26328 tail
            f.push_back(game_value(r.vol_pass_m22));
            f.push_back(game_value(r.vol_pass_age));                      // 26329 tail
            f.push_back(game_value(r.vol_draw_near));
            f.push_back(game_value(r.vol_draw_age));
            f.push_back(game_value(r.eng_vp_w_abs));                      // 26331 tail
            f.push_back(game_value(r.eng_vp_w_rel));
            f.push_back(game_value(r.eng_vp_w_true));
            f.push_back(game_value(r.comp_cam_rt_m));                     // 26342 tail
            f.push_back(game_value(r.seam_cam_rt_m));
            f.push_back(game_value(r.cam_abs_m));
            f.push_back(game_value(static_cast<float>(r.comp_take_src)));
            f.push_back(game_value(static_cast<float>(r.comp_near_z)));
            f.push_back(game_value(r.seam_fetch_dup_m));                   // 26365
            f.push_back(game_value(r.comp_cam_step_m));                    // 26373
            f.push_back(game_value(r.seam_trn_dx_m));                      // 26366
            f.push_back(game_value(r.seam_trn_bnd_m));
            f.push_back(game_value(static_cast<float>(r.seam_fetch_n)));     // 26367
            f.push_back(game_value(static_cast<float>(r.seam_bound_n)));
            f.push_back(game_value(r.band_pick_age_ms));
            f.push_back(game_value(r.band_pick_cam_dx_m));                 // 26369
            f.push_back(game_value(r.band_alt_cam_dx_m));                  // 26375
            f.push_back(game_value(static_cast<float>(r.band_alt_better)));
            f.push_back(game_value(static_cast<float>(r.band_pick_none_n)));
            f.push_back(game_value(static_cast<float>(r.band_pool_miss_n)));
            f.push_back(game_value(r.band_stage_wait_ms));
            f.push_back(game_value(r.sten_tol));
            f.push_back(game_value(r.vol_pass_swing_px));                  // 26362 tail
            f.push_back(game_value(r.vol_draw_swing_px));
            f.push_back(game_value(static_cast<float>(r.recv_bands_n)));    // 26387
            f.push_back(game_value(r.recv_band_near));
            f.push_back(game_value(static_cast<float>(r.recv_drop_why)));
            f.push_back(game_value(r.recv_band_age_ms));   // 26388
            f.push_back(game_value(r.recv_band_cam_dx_m));   // 26389
            f.push_back(game_value(static_cast<float>(r.vmir_draws)));   // 26458
            f.push_back(game_value(r.blk_mir_lum));                      // 26479
            f.push_back(game_value(r.blk_anch_lum));
            f.push_back(game_value(static_cast<float>(r.d_blk_applies)));
            f.push_back(game_value(static_cast<float>(r.d_blk_anch_rej)));
            if (khtr_nrow == 0) khtr_nrow = static_cast<uint32_t>(f.size());   // 26291
            frames.push_back(game_value(std::move(f)));
        }

        {
            auto_array<game_value> pair;
            pair.push_back(game_value("status"));
            pair.push_back(game_value("ok"));
            out.push_back(game_value(std::move(pair)));
        }

        {   // 26052: build verification in-band (field protocol step 1).
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

        {   // 26291: in-band field/row count - the permanent audit.
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

// dumpDynamicLights -> ARRAY. The standing diagnostic for the finalized
// dynamic-lights system (ON by default, engine-derived origins). OPT-IN
// like getRenderStats: the FIRST call arms the per-draw census + zeroes
// the counters and returns [["status","armed"]]; subsequent calls return
// the full picture - mirror age/counts, the raw cb10 control block, the
// binding census, per-window origin verdicts (route kind, slots,
// residuals), the derivation censuses (route mix, degeneracy wins, range
// rejects), the absolute-world pool, the harvest rings and the fill
// census. State is copied under the graphics lock (parking the render
// thread, the mirror's write invariant) and formatted after release.
static game_value dump_dynamic_lights_sqf() {
    try {
        if (!RenderIntegration::g_dl_recon.exchange(true, std::memory_order_relaxed)) {
            // Arm: census on; counters/mirror zeroed under the lock so the
            // session starts clean. A failed lock still arms - the state
            // then zeroes lazily at the next successful call instead.
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

        // Copy under the lock, format after release (dumpRenderTrace's
        // pattern). The struct copy includes raw device pointers used only
        // as printed identities - never dereferenced.
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
        out.push_back(kv("buildTag", static_cast<float>(RenderIntegration::KH_BUILD_TAG)));   // 26052
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

        {   // cb10 verbatim + the count lanes' int interpretations
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

        {   // every active light record, 24 floats each, engine-verbatim
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

        {   // mode-3 harvest ring, oldest-first: [ageS, poolN, anchored,
            // added, updated, expired, spotFlips, gdiffR, gdiffG, gdiffB, scale]
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

        {   // the anchor table verbatim: [x, y, z, ageS] - pollution is
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
        // 26190 EXPIRY CENSUS - the lane the transient tail has never had.
        // dlExpTail* is how long each entry was KEPT PAST ITS LAST SIGHTING:
        // that IS the over-persistence the operator sees, in ms, measured
        // rather than modelled. dlExpLifeMean is how long the entry was
        // actually alive, so tail/life prices the fault directly.
        // dlExpFloorHits counts expiries whose TTL came from a FLOOR rather
        // than from 6 x the cadence mark - if that dominates, the 250 ms
        // KH_DL_TTL_FLOOR_MS is the fault and the 26187 mark decay could never
        // have reached it. dlExpDenseHits counts entries that reached the 80 ms
        // fast lane; near-zero means the sighting stream is too sparse for the
        // dense test (zeroLists is the corroborating lane) and the fast lane is
        // structurally unreachable for exactly the lights that need it.
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

// addLocalPostFX [[x,y,zASL], radius, falloff, effect, params?, color?]
// Same effect table and parameters as addPostFX, but the effect is confined
// to a world-space sphere: full strength within 'radius' meters of the
// position, smoothly fading to nothing over the next 'falloff' meters.
// The mask is computed per pixel from the depth buffer, so it hugs geometry:
// a localized colorgrade desaturates the buildings inside the sphere and
// nothing behind them. Shares the handle space with addRender3D/addPostFX;
// manage via updatePostFX ("position" moves the center, "radius",
// "falloff", "effect", "params", "color", "visible") and removeRenderHandler.
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

// flushUIRender
// Renders all UI-affecting passes (addPostFX with affectUI "BOTH"/"UI") into the
// frame being composed. Driven automatically by the internal overlay control
// created by ensure_ui_driver(); also callable from a Draw EH on a custom
// display. Cheap no-op when no UI-affecting passes exist. Returns BOOL: true
// if passes were queued this call.
static game_value flush_ui_render_sqf() {
    // 26450: the union sun-map range follows the game's shadow view
    // distance, read directly HERE every flush (operator call: a per-
    // frame read beats command plumbing; the cost - one getVideoOptions
    // hashmap build per frame - is accepted). The wrapper's keyed
    // lookup returns a SCALAR, so nothing here is positioned to
    // version-drift. Failures leave the previous value standing (default
    // 200 = the game's stock setting). 26451: direct
    // sqf::get_video_options() wrapper, no compiled snippet.
    try {
        // 26452: rv_hashmap read per the intercept types header - the map
        // is game_data_hashmap_pair<game_value> entries with .key/.value,
        // so ITERATION with a case-folded key compare is the access the
        // header guarantees, independent of internal_hashmap's lookup
        // signatures. RV string keys are compared case-insensitively.
        rv_hashmap khsr_map = sqf::get_video_options();

        for (auto& khsr_e : khsr_map) {
            if (khsr_e.key.type_enum() != game_data_type::STRING) continue;
            const std::string khsr_k = static_cast<std::string>(khsr_e.key);
            if (khsr_k.size() != 16) continue;   // "shadowVisibility"
            static const char khsr_w[17] = "shadowvisibility";
            bool khsr_m = true;

            for (size_t khsr_i = 0; khsr_i < 16; ++khsr_i) {
                char khsr_c = khsr_k[khsr_i];
                if (khsr_c >= 'A' && khsr_c <= 'Z') khsr_c = static_cast<char>(khsr_c + 32);
                if (khsr_c != khsr_w[khsr_i]) { khsr_m = false; break; }
            }

            if (!khsr_m) continue;

            if (khsr_e.value.type_enum() == game_data_type::SCALAR) {
                const float khsr_f = static_cast<float>(khsr_e.value);
                if (khsr_f == khsr_f && khsr_f > 0.0f) {
                    RenderIntegration::g_sun_range.store(khsr_f, std::memory_order_relaxed);
                }
            }

            break;
        }
    } catch (...) {}

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
        // 26314: the SQF-visible description is deliberately NARROW. The full
        // mode catalog (the operator's reference prose) grew past what
        // register_sqf_command's argument shape tolerates in practice and now
        // lives as the g_dbg_mode CATALOG COMMENT in rendering_integration.hpp
        // (search: "26314 THE MODE CATALOG"). ADDING A MODE STILL MEANS TWO
        // EDITS - that catalog comment and the whitelist above - and there is
        // no compiler check that they agree.
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

        private _old = uiNamespace getVariable ["kh_uiDriverControl", controlNull];

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

        uiNamespace setVariable ["kh_uiDriverControl", _control];
        setReturnValue true;
    )");
}