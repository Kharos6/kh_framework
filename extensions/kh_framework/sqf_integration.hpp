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
static registered_sqf_function _sqf_remove_render_handler_scalar;
static registered_sqf_function _sqf_queue_visibility_array;
static registered_sqf_function _sqf_get_visibility_results;
static registered_sqf_function _sqf_add_render3d_array;
static registered_sqf_function _sqf_update_post_fx_array;
static registered_sqf_function _sqf_add_postfx_array;
static registered_sqf_function _sqf_add_local_postfx_array;
static registered_sqf_function _sqf_get_render_stats;
static registered_sqf_function _sqf_flush_ui_render;

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
            rv_namespace ns = sqf::mission_namespace();
            std::string fname = static_cast<std::string>(function);
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
        "Read the engine depth buffer at screen position [u, v] (0..1, top-left origin) via compute shader. Returns [sceneDistanceMeters, rawDepth] or a status string. Call from Draw3D.",
        userFunctionWrapper<sample_scene_depth_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_gpu_visibility_array = intercept::client::host::register_sqf_command(
        "gpuVisibility",
        "Test up to 1024 world points [[x,y,zASL], ...] against the engine depth buffer in one GPU dispatch. Returns [[status, pointDistM, sceneDistM], ...]; status 1 = visible, 0 = occluded, -1 = offscreen. Call from Draw3D.",
        userFunctionWrapper<gpu_visibility_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_remove_render_handler_scalar = intercept::client::host::register_sqf_command(
        "removeRenderHandler",
        "Remove an effect or persistent 3D object by handle. Pass -1 to clear all. Returns BOOL.",
        userFunctionWrapper<remove_render_handler_sqf>,
        game_data_type::BOOL,
        game_data_type::SCALAR
    );

    _sqf_queue_visibility_array = intercept::client::host::register_sqf_command(
        "queueVisibility",
        "Queue up to 1024 world points [[x,y,zASL], ...] for an async GPU depth-visibility test (no stall; results 1-2 frames later via getVisibilityResults). Returns SCALAR accepted count.",
        userFunctionWrapper<queue_visibility_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_get_visibility_results = intercept::client::host::register_sqf_command(
        "getVisibilityResults",
        "Fetch the latest completed async visibility batch: [ageInFrames, [[status, pointDistM, sceneDistM], ...]]. status: 1 visible, 0 occluded, -1 offscreen.",
        userFunctionWrapper<get_visibility_results_sqf>,
        game_data_type::ARRAY
    );

    _sqf_add_render3d_array = intercept::client::host::register_sqf_command(
        "addRender3D",
        "Add a persistent 3D box drawn every frame until removed. [[x,y,zASL], size, [r,g,b,a]?, mode?, sceneRead?, effect?, params?, band?, blend?, duration?, lit?]; size: number or [x,y,z] per-axis edge lengths; mode: 0=depth test (default), 1=test+write, 2=overlay; effect (string/scalar): 'invert','colorgrade','vignette','chromatic','grain','sharpen','blur','bloom','distortion','outline','pulse','halation','fog','lensflare','anamorphic','sunflare','glitch'; band [minDist,maxDist,falloff?]; blend: 'normal','additive','multiply','screen','lighten','darken'; duration: ARRAY or SCALAR (default 0), optionally [fadein,duration,fadeout]; lit: BOOL or [ambient, diffuse, shadowMode] - true enables the full shadow system: sun/moon shading from getLighting (fetched internally), per-pixel received world shadows (band-captured cascade atlas, deferred same-frame view), and an analytic cast shadow written into the engine's own shadow mask (MIN-blended, colored by the engine's lighting pass exactly like native shadows). shadowMode: 0 = no occlusion rays, 1 = center ray (default), 2 = center + 8 corners. Everything is automatic: no shadow commands, tuning, or per-frame calls exist or are needed. Solid non-overlay boxes are always composited: injected into the frame BEFORE the engine's translucent passes with depth written, so the engine itself composites smoke and particles against them pixel-perfectly (falls back to the post-scene flush automatically if the draw hook is unavailable). Callable from any context. Returns SCALAR handle or STRING error.",
        userFunctionWrapper<add_render3d_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_update_post_fx_array = intercept::client::host::register_sqf_command(
        "updatePostFX",
        "Update a persistent 3D object or post-processing pass: [handle, property, value]. Properties: 'position' [x,y,zASL], 'size' number|[x,y,z], 'color' [r,g,b,a], 'mode' 0..2, 'visible' bool, 'effect' string/scalar, 'params' array, 'ui' bool, 'radius' number|[x,y,z], 'falloff' scalar, 'shape' 'sphere'|'cube', 'blend' 'normal'|'additive'|'multiply'|'screen'|'lighten'|'darken', 'band' [minDist,maxDist,falloff?] ([] clears), 'localsphere' [radius,falloff?] ([] clears), 'sceneread' bool, 'lit' bool or [ambient, diffuse, shadowMode?] (toggles the shadow system per object), 'duration' number or [fadein,duration,fadeout]. Returns BOOL.",
        userFunctionWrapper<update_post_fx_sqf>,
        game_data_type::BOOL,
        game_data_type::ARRAY
    );

    _sqf_add_postfx_array = intercept::client::host::register_sqf_command(
        "addPostFX",
        "Create a persistent fullscreen post-processing pass: [effect, params?, [r,g,b,a]?, band?, blend?, affectUI?, duration?]. Effects: 'invert','colorgrade','vignette','chromatic','grain','sharpen','blur','bloom','distortion','outline','pulse','halation','fog','lensflare','anamorphic','sunflare','glitch'; color alpha = intensity; band [minDist,maxDist,falloff?]; blend: 'normal','additive','multiply','screen','lighten','darken'; affectUI: BOOL (default false) - true renders the pass post-tonemap over the composited frame INCLUDING the UI; duration: ARRAY or SCALAR (default 0), optionally [fadein,duration,fadeout]. Passes chain in creation order per phase. Manage with updatePostFX / removeRenderHandler. Returns SCALAR handle or STRING error.",
        userFunctionWrapper<add_postfx_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_add_local_postfx_array = intercept::client::host::register_sqf_command(
        "addLocalPostFX",
        "Create a persistent post-processing effect confined to a world-space volume: [[x,y,zASL], radius, falloff, effect, params?, [r,g,b,a]?, shape?, blend?, duration?]. radius: number or [x,y,z] per-axis (ellipsoid/box); shape: 'sphere' (default) or 'cube'; falloff in meters (scaled by mean radius); blend as addPostFX; duration as addPostFX. Mask follows geometry via the depth buffer. Manage via updatePostFX and removeRenderHandler. Returns SCALAR handle or STRING error.",
        userFunctionWrapper<add_local_postfx_sqf>,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_get_render_stats = intercept::client::host::register_sqf_command(
        "getRenderStats",
        "Render health counters (cumulative): [[name, value], ...]. Composite path: flushes, gatePassed, compositeInjections (should track flushes), compositeBoxes, compositeSkips, compositeAmbiguous, compositeProjLock, compositeRearms, compositeRejSpan, compositeRejFloor. Shadow receive: shadowLiveLatches, shadowLiveCascades, bandCaptures, band0/1 Near/Far/Copies, sealCompletions (healthy = equals bandCaptures), viewLocks, viewSrcValid, frameViewHits, resolveHits. Shadow cast: analyticCasts (healthy = roughly flushes, more on depth-partitioned frames), castMisses (0 = firing; nonzero names the first failed guard). Lighting: sunDirValid, sunDirEngine*, sunBrightness. A skipped flush is a frame rendered without effects (flicker).",
        userFunctionWrapper<get_render_stats_sqf>,
        game_data_type::ARRAY
    );

    _sqf_flush_ui_render = intercept::client::host::register_sqf_command(
        "flushUIRender",
        "Renders all UI-affecting passes (addPostFX with affectUI true) into the frame being composed. Driven automatically by the internal overlay control; also callable from a Draw EH on a custom display. Returns BOOL.",
        userFunctionWrapper<flush_ui_render_sqf>,
        game_data_type::BOOL
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
    g_compiled_kh_empty_code = sqf::compile(R"()");
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

        if (!isNull _old) then { 
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