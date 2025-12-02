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
static registered_sqf_function _sqf_write_khdata_string_array;
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
static registered_sqf_function _sqf_set_network_port;
static registered_sqf_function _sqf_network_message_send_any_array;
static registered_sqf_function _sqf_network_message_send_array;
static registered_sqf_function _sqf_network_remove_jip;
static registered_sqf_function _sqf_network_message_receive_string_array;
static registered_sqf_function _sqf_network_message_receive_string_code;
static registered_sqf_function _sqf_network_remove_handler;
static registered_sqf_function _sqf_network_is_initialized;
static registered_sqf_function _sqf_network_initialize;
static registered_sqf_function _sqf_network_shutdown;

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
        
        // Local execution - handle CODE or STRING type
        std::string code_str;
        
        if (code_or_function.type_enum() == game_data_type::CODE) {
            auto code_data = code_or_function.get_as<game_data_code>();
            code_str = static_cast<std::string>(code_data->code_string);
        } else {
            code_str = static_cast<std::string>(code_or_function);
        }

        // Check if it's a function call or arbitrary code execution
        sol::protected_function_result result;
        
        if (code_str.find(' ') == std::string::npos && code_str.find('(') == std::string::npos) {
            // Try to get the function from cache or global namespace
            auto cache_it = g_call_cache.find(code_str);
            sol::protected_function func;
            
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
            
            if (args.type_enum() == game_data_type::ARRAY) {
                // Unpack array
                auto& arr = args.to_array();
                std::vector<sol::object> arg_vec;
                arg_vec.reserve(arr.size());

                for (size_t i = 0; i < arr.size(); i++) {
                    arg_vec.push_back(convert_game_value_to_lua(arr[i]));
                }

                if (arg_vec.empty()) {
                    result = func();
                } else {
                    result = func(sol::as_args(arg_vec));
                }
            } else if (args.is_nil()) {
                result = func();
            } else {
                result = func(convert_game_value_to_lua(args));
            }
        } else {
            // Execute code with arguments in scope
            size_t code_hash = std::hash<std::string>{}(code_str);
            auto code_it = g_code_cache.find(code_hash);
            sol::protected_function compiled_code;
            
            if (code_it != g_code_cache.end()) {
                compiled_code = code_it->second;
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
                
                compiled_code = factory_result;
                g_code_cache[code_hash] = compiled_code;
            }
            
            if (args.type_enum() == game_data_type::ARRAY) {
                // Unpack array
                auto& arr = args.to_array();
                std::vector<sol::object> arg_vec;
                arg_vec.reserve(arr.size());

                for (size_t i = 0; i < arr.size(); i++) {
                    arg_vec.push_back(convert_game_value_to_lua(arr[i]));
                }

                if (arg_vec.empty()) {
                    result = compiled_code();
                } else {
                    result = compiled_code(sol::as_args(arg_vec));
                }
            } else if (args.is_nil()) {
                result = compiled_code();
            } else {
                result = compiled_code(convert_game_value_to_lua(args));
            }
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

static game_value read_khdata_sqf(game_value_parameter filename, game_value_parameter var_param) {
    try {
        std::string file_str = static_cast<std::string>(filename);
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
        
        auto* file = KHDataManager::instance().get_or_create_file(file_str);
        
        if (!file) {
            return has_default ? default_value : game_value();
        }
        
        // Special case: if var_name == filename, return all variable names
        if (var_str == file_str) {
            auto names = file->get_variable_names();
            auto_array<game_value> arr;
            arr.reserve(names.size());
            
            for (const auto& name : names) {
                arr.push_back(game_value(name));
            }
            
            return game_value(std::move(arr));
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

static game_value write_khdata_sqf(game_value_parameter filename, game_value_parameter params) {
    try {
        std::string file_str = static_cast<std::string>(filename);
        
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
            raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params)));
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
            return raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params)));
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
        raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params)));
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
            raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params)));
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

static game_value execute_sqf(game_value_parameter args, game_value_parameter code_or_function) {
    try {
        auto_array<game_value> params;
        
        if (code_or_function.type_enum() == game_data_type::ARRAY) {
            // Right arg is array: prepend left arg as first element
            params.push_back(args);
            auto& arr = code_or_function.to_array();
            params.insert(params.end(), arr.begin(), arr.end());
        } else if (code_or_function.type_enum() == game_data_type::CODE || code_or_function.type_enum() == game_data_type::STRING) {
            // Right arg is CODE or STRING: [arguments, code/string, nil, nil, nil]
            params.push_back(args);
            params.push_back(code_or_function);
            params.push_back(game_value());
            params.push_back(game_value());
            params.push_back(game_value());
        }
        
        return raw_call_sqf_args_native(g_compiled_sqf_execute_sqf, game_value(std::move(params)));
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
        constexpr float RAD_TO_DEG = 180.0f / 3.14159265359f;
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
        
        float dirX = dir.x;
        float dirY = dir.y;
        float dirZ = dir.z;
        float upX = up.x;
        float upZ = up.z;
        float aroundX = std::fmod(std::atan2(-dirZ, std::sqrt(dirX * dirX + dirY * dirY)) * RAD_TO_DEG + 360.0f, 360.0f);
        float aroundY = std::fmod(360.0f - std::atan2(upX, upZ) * RAD_TO_DEG, 360.0f);
        float aroundZ = std::fmod(std::atan2(dirX, dirY) * RAD_TO_DEG + 360.0f, 360.0f);
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
        
        constexpr float DEG_TO_RAD = 3.14159265359f / 180.0f;
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
            report_error("vectorToEuler requires vectors with 3 components each");
            return game_value();
        }
        
        constexpr float RAD_TO_DEG = 180.0f / 3.14159265359f;
        float dirX = static_cast<float>(dir_array[0]);
        float dirY = static_cast<float>(dir_array[1]);
        float dirZ = static_cast<float>(dir_array[2]);
        float upX = static_cast<float>(up_array[0]);
        float upZ = static_cast<float>(up_array[2]);
        float aroundX = std::fmod(std::atan2(-dirZ, std::sqrt(dirX * dirX + dirY * dirY)) * RAD_TO_DEG + 360.0f, 360.0f);
        float aroundY = std::fmod(360.0f - std::atan2(upX, upZ) * RAD_TO_DEG, 360.0f);
        float aroundZ = std::fmod(std::atan2(dirX, dirY) * RAD_TO_DEG + 360.0f, 360.0f);
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
    
        constexpr float DEG_TO_RAD = 3.14159265359f / 180.0f;
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

static game_value set_network_port_sqf(game_value_parameter port_value) {
    try {
        int port = static_cast<int>(static_cast<float>(port_value));
        
        if (port < 1 || port > 65535) {
            report_error("KH Network: Invalid port number. Must be between 1 and 65535");
            return game_value(false);
        }
        
        NetworkFramework::instance().set_port(port);
        sqf::diag_log("KH Network: Port set to " + std::to_string(port));
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH Network: Error in setNetworkPort - " + std::string(e.what()));
        return game_value(false);
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
        game_value target = arr[1];
        std::string jip_key = "";
        std::string dependency_net_id = "";
        bool dependency_is_group = false;

        if (arr.size() > 2) {
            game_value jip_arg = arr[2];
            
            if (jip_arg.type_enum() == game_data_type::STRING) {
                jip_key = static_cast<std::string>(jip_arg);
            } else if (jip_arg.type_enum() == game_data_type::BOOL && static_cast<bool>(jip_arg)) {
                jip_key = UIDGenerator::generate();
            } else if (jip_arg.type_enum() == game_data_type::ARRAY) {
                // Array format: [dependency, jip_key (optional)]
                // dependency can be object or group - will be stored as netId
                auto& jip_arr = jip_arg.to_array();
                
                if (!jip_arr.empty()) {
                    game_value dependency = jip_arr[0];
                    
                    if (dependency.type_enum() == game_data_type::OBJECT) {
                        object dep_obj = static_cast<object>(dependency);

                        if (!sqf::is_null(dep_obj)) {
                            dependency_net_id = static_cast<std::string>(sqf::net_id(dep_obj));
                            dependency_is_group = false;
                            
                            // Get jip_key from second element or generate
                            if (jip_arr.size() > 1 && jip_arr[1].type_enum() == game_data_type::STRING) {
                                std::string key_str = static_cast<std::string>(jip_arr[1]);
                                jip_key = key_str.empty() ? UIDGenerator::generate() : key_str;
                            } else {
                                jip_key = UIDGenerator::generate();
                            }
                        }
                    } else if (dependency.type_enum() == game_data_type::GROUP) {
                        group dep_grp = static_cast<group>(dependency);

                        if (!sqf::is_null(dep_grp)) {
                            dependency_net_id = static_cast<std::string>(sqf::net_id(dep_grp));
                            dependency_is_group = true;
                            
                            // Get jip_key from second element or generate
                            if (jip_arr.size() > 1 && jip_arr[1].type_enum() == game_data_type::STRING) {
                                std::string key_str = static_cast<std::string>(jip_arr[1]);
                                jip_key = key_str.empty() ? UIDGenerator::generate() : key_str;
                            } else {
                                jip_key = UIDGenerator::generate();
                            }
                        }
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

        if (!jip_key.empty()) {
            NetworkFramework::instance().store_jip_message(jip_key, event_name, message, static_cast<int>(sqf::client_owner()), dependency_net_id, dependency_is_group);
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
                
                // Override JIP storage for CODE - must store the conditional wrapper
                // so JIP clients also evaluate the condition
                if (!jip_key.empty()) {
                    NetworkFramework::instance().remove_jip_message(jip_key);  // Remove the one stored earlier
                    NetworkFramework::instance().store_jip_message(jip_key, NET_INTERNAL_CONDITIONAL_EVENT, cond_payload, static_cast<int>(sqf::client_owner()), dependency_net_id, dependency_is_group);
                }
                
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
        
        NetworkFramework::instance().remove_jip_message(jip_key);
        return game_value(true);
    } catch (const std::exception& e) {
        report_error("KH Network: Error in networkMessageRemoveJip - " + std::string(e.what()));
        return game_value(false);
    }
}

static game_value network_message_receive_sqf(game_value_parameter left_arg, game_value_parameter right_arg) {
    try {
        std::string event_name = static_cast<std::string>(left_arg);
        
        if (event_name.empty()) {
            report_error("KH Network: Event name cannot be empty");
            return game_value(-1.0f);
        }
        
        game_value handler_args;
        code handler_function;
        
        if (right_arg.type_enum() == game_data_type::ARRAY) {
            auto& arr = right_arg.to_array();

            if (arr.size() < 2) {
                report_error("KH Network: networkMessageReceive requires [arguments, function] or just function");
                return game_value(-1.0f);
            }

            handler_args = arr[0];
            
            if (arr[1].type_enum() == game_data_type::CODE) {
                handler_function = static_cast<code>(arr[1]);
            } else {
                report_error("KH Network: Handler must be code");
                return game_value(-1.0f);
            }
        } else {
            handler_function = static_cast<code>(right_arg);
        }
        
        // Ensure network is initialized
        if (!NetworkFramework::instance().is_initialized()) {
            if (!NetworkFramework::instance().initialize()) {
                report_error("KH Network: Failed to initialize network framework");
                return game_value(-1.0f);
            }
        }
        
        int handler_id = NetworkFramework::instance().add_message_handler(event_name, handler_function, handler_args);
        sqf::diag_log("KH Network: Handler " + std::to_string(handler_id) + " registered for event '" + event_name + "'");
        return game_value(static_cast<float>(handler_id));
    } catch (const std::exception& e) {
        report_error("KH Network: Error in networkMessageReceive - " + std::string(e.what()));
        return game_value(-1.0f);
    }
}

static game_value network_is_initialized_sqf() {
    return game_value(NetworkFramework::instance().is_initialized());
}

static game_value network_remove_handler_sqf(game_value_parameter handler_id_value) {
    try {
        int handler_id = static_cast<int>(static_cast<float>(handler_id_value));
        
        if (handler_id < 0) {
            report_error("KH Network: Invalid handler ID");
            return game_value(false);
        }
        
        bool success = NetworkFramework::instance().remove_message_handler(handler_id);
        
        if (success) {
            sqf::diag_log("KH Network: Handler " + std::to_string(handler_id) + " removed");
        } else {
            sqf::diag_log("KH Network: Handler " + std::to_string(handler_id) + " not found");
        }
        
        return game_value(success);
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
    return execute_sqf(game_value(), code_or_function);
}

static game_value get_rotation_euler_unary(game_value_parameter right_arg) {
    return get_rotation_euler_sqf(game_value(), right_arg);
}

static game_value network_message_send_unary_sqf(game_value_parameter right_arg) {
    return network_message_send_sqf(game_value(), right_arg);
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

    _sqf_write_khdata_string_array = intercept::client::host::register_sqf_command(
        "writeKhData",
        "Write variable to KHData file",
        userFunctionWrapper<write_khdata_sqf>,
        game_data_type::NOTHING,
        game_data_type::STRING,
        game_data_type::ARRAY
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
    
    _sqf_set_network_port = intercept::client::host::register_sqf_command(
        "setNetworkPort",
        "Set the server network port for KH Framework networking",
        userFunctionWrapper<set_network_port_sqf>,
        game_data_type::BOOL,
        game_data_type::SCALAR
    );
    
    _sqf_network_message_send_any_array = intercept::client::host::register_sqf_command(
        "networkMessageSend",
        "Send a network message",
        userFunctionWrapper<network_message_send_sqf>,
        game_data_type::BOOL,
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_network_message_send_array = intercept::client::host::register_sqf_command(
        "networkMessageSend",
        "Send a network message",
        userFunctionWrapper<network_message_send_unary_sqf>,
        game_data_type::BOOL,
        game_data_type::ARRAY
    );

    _sqf_network_remove_jip = intercept::client::host::register_sqf_command(
        "networkMessageRemoveJip",
        "Remove a JIP message by its key",
        userFunctionWrapper<network_remove_jip_sqf>,
        game_data_type::BOOL,
        game_data_type::STRING
    );

    _sqf_network_message_receive_string_array = intercept::client::host::register_sqf_command(
        "networkMessageReceive",
        "Register a handler for network messages",
        userFunctionWrapper<network_message_receive_sqf>,
        game_data_type::SCALAR,
        game_data_type::STRING,
        game_data_type::ARRAY
    );

    _sqf_network_message_receive_string_code = intercept::client::host::register_sqf_command(
        "networkMessageReceive",
        "Register a handler for network messages",
        userFunctionWrapper<network_message_receive_sqf>,
        game_data_type::SCALAR,
        game_data_type::STRING,
        game_data_type::CODE
    );
    
    _sqf_network_remove_handler = intercept::client::host::register_sqf_command(
        "networkRemoveHandler",
        "Remove a network message handler by ID",
        userFunctionWrapper<network_remove_handler_sqf>,
        game_data_type::BOOL,
        game_data_type::SCALAR
    );
    
    _sqf_network_is_initialized = intercept::client::host::register_sqf_command(
        "networkIsInitialized",
        "Check if the network framework is initialized",
        userFunctionWrapper<network_is_initialized_sqf>,
        game_data_type::BOOL
    );

    _sqf_network_initialize = intercept::client::host::register_sqf_command(
        "networkInitialize",
        "Initialize network framework",
        userFunctionWrapper<network_initialize_sqf>,
        game_data_type::BOOL
    );
    
    _sqf_network_shutdown = intercept::client::host::register_sqf_command(
        "networkShutdown",
        "Shutdown the network framework",
        userFunctionWrapper<network_shutdown_sqf>,
        game_data_type::BOOL
    );
    
    g_compiled_sqf_trigger_cba_event = sqf::compile(R"(setReturnValue (getCallArguments call KH_fnc_triggerCbaEvent);)");
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

        setReturnValue (_khArgs call KH_fnc_execute);
    )");

    g_compiled_sqf_execute_sqf = sqf::compile(R"(setReturnValue (getCallArguments call KH_fnc_execute);)");
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
}