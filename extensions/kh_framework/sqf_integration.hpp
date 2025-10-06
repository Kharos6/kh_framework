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
            sqf_params.push_back(args);
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

        // Check if it's a function call or code execution
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
        
        // If target is specified, trigger CBA event
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
            return raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params)));
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
        game_value event_args = left_arg;  // Left arg is the arguments
        
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
            
            // First element must be event name
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
            
            // Push function name
            lua_getglobal(L, "event");
            lua_getfield(L, -1, "trigger");
            lua_remove(L, -2);  // Clean up event table from stack
                        
            // Push event name
            lua_pushstring(L, event_name.c_str());
            
            // Push arguments
            int arg_count = 1; // event name

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
            lua_pcall(L, arg_count, 1, 0);
            sol::object result = sol::stack::pop<sol::object>(L);

            // Return result of last added event handler
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
        return raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params)));
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
            return raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params)));
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
        game_data_type::ANY,
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
        game_data_type::ANY,
        game_data_type::ARRAY
    );

    _sqf_lua_set_variable_array = intercept::client::host::register_sqf_command(
        "luaSetVariable",
        "Set Lua variable",
        userFunctionWrapper<lua_set_variable_sqf>,
        game_data_type::ANY,
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

    g_compiled_sqf_trigger_cba_event = sqf::compile("_x call KH_fnc_triggerCbaEvent;");
    g_compiled_sqf_add_game_event_handler = sqf::compile("_x call KH_fnc_addEventHandler;");
    g_compiled_sqf_remove_game_event_handler = sqf::compile("_x call KH_fnc_removeEventHandler;");
    g_compiled_sqf_game_event_handler_lua_bridge = sqf::compile("_x luaTriggerEvent _args;");

    g_compiled_sqf_execute_lua = sqf::compile(R"(
        _x set [1, ["_x luaExecute ", _x select 1] joinString ""];
        private _special = param [5, false];

        if (_special isEqualType []) then {
            private _type = _special param [0, "", [""]];

            if (_type isEqualTo "CALLBACK") then {
                _special set [2, ["_x append _argsCallback; _x luaExecute ", _special select 2] joinString ""];
            }
            else {
                if (_type isEqualTo "PERSISTENT") then {
                    _special set [3, ["_x luaExecute ", _special select 3] joinString ""];
                };
            };
        };

        _x call KH_fnc_execute;
    )");

    g_compiled_sqf_execute_sqf = sqf::compile("_x call KH_fnc_execute;");
    g_compiled_sqf_remove_handler = sqf::compile("_x call KH_fnc_removeHandler;");
    g_compiled_sqf_create_hash_map_from_array = sqf::compile("createHashMapFromArray _x;");
    g_compiled_sqf_create_hash_map = sqf::compile("createHashMap;");
}