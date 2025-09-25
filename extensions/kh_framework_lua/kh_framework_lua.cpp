#include <windows.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "intercept/include/intercept.hpp"
#include "intercept/include/client/sqf/sqf.hpp"
#include "intercept/include/client/pointers.hpp"
#include "sol/sol.hpp"

#pragma optimize("t", on)
#pragma inline_recursion(on)
#pragma inline_depth(255)

using namespace intercept;

static types::registered_sqf_function _execute_lua_sqf_command;
static types::registered_sqf_function _compile_lua_sqf_command;
static sol::protected_function cached_lua_init;
static sol::protected_function cached_lua_frame_update;  
static sol::protected_function cached_lua_deinit;
static std::unique_ptr<sol::state> g_lua_state;

// Userdata wrapper for game_value to preserve native Arma types
struct GameValueWrapper {
    game_value value;
    GameValueWrapper() = default;
    GameValueWrapper(const game_value& v) : value(v) {}
    GameValueWrapper(game_value&& v) : value(std::move(v)) {}
    
    std::string to_string() const {
        if (value.is_nil()) return "nil";
        
        switch (value.type_enum()) {
            case types::game_data_type::OBJECT: return "[OBJECT]";
            case types::game_data_type::GROUP: return "[GROUP]";
            case types::game_data_type::NAMESPACE: return "[NAMESPACE]";
            case types::game_data_type::CONFIG: return "[CONFIG]";
            case types::game_data_type::CONTROL: return "[CONTROL]";
            case types::game_data_type::DISPLAY: return "[DISPLAY]";
            case types::game_data_type::LOCATION: return "[LOCATION]";
            case types::game_data_type::SCRIPT: return "[SCRIPT]";
            case types::game_data_type::SIDE: return "[SIDE]";
            case types::game_data_type::TEXT: return "[TEXT]";
            case types::game_data_type::TEAM_MEMBER: return "[TEAM_MEMBER]";
            case types::game_data_type::CODE: return "[CODE]";
            case types::game_data_type::TASK: return "[TASK]";
            case types::game_data_type::DIARY_RECORD: return "[DIARY_RECORD]";
            case types::game_data_type::NetObject: return "[NETOBJECT]";
            case types::game_data_type::SUBGROUP: return "[SUBGROUP]";
            case types::game_data_type::TARGET: return "[TARGET]";
            default: return value.data ? static_cast<std::string>(value.data->to_string()) : "nil";
        }
    }
    
    bool equals(const GameValueWrapper& other) const {
        return value == other.value;
    }

    // Method to get the type name
    std::string type_name() const {
        switch (value.type_enum()) {
            case types::game_data_type::NOTHING: return "NOTHING";
            case types::game_data_type::ANY: return "ANY";
            case types::game_data_type::SCALAR: return "SCALAR";
            case types::game_data_type::BOOL: return "BOOL";
            case types::game_data_type::ARRAY: return "ARRAY";
            case types::game_data_type::STRING: return "STRING";
            case types::game_data_type::OBJECT: return "OBJECT";
            case types::game_data_type::GROUP: return "GROUP";
            case types::game_data_type::NAMESPACE: return "NAMESPACE";
            case types::game_data_type::CONFIG: return "CONFIG";
            case types::game_data_type::CONTROL: return "CONTROL";
            case types::game_data_type::DISPLAY: return "DISPLAY";
            case types::game_data_type::LOCATION: return "LOCATION";
            case types::game_data_type::SCRIPT: return "SCRIPT";
            case types::game_data_type::SIDE: return "SIDE";
            case types::game_data_type::TEXT: return "TEXT";
            case types::game_data_type::TEAM_MEMBER: return "TEAM_MEMBER";
            case types::game_data_type::CODE: return "CODE";
            case types::game_data_type::TASK: return "TASK";
            case types::game_data_type::DIARY_RECORD: return "DIARY_RECORD";
            case types::game_data_type::NetObject: return "NETOBJECT";
            case types::game_data_type::SUBGROUP: return "SUBGROUP";
            case types::game_data_type::TARGET: return "TARGET";
            default: return "UNKNOWN";
        }
    }
};

struct LuaCallCache {
    std::string function_name;
    sol::protected_function func;
    bool is_valid;
};

static std::unordered_map<std::string, LuaCallCache> g_call_cache;
static std::unordered_map<size_t, sol::protected_function> g_code_cache;
static constexpr size_t MAX_CACHE_SIZE = 128;

class Lua_Compilation {
private:
    static std::unordered_map<std::string, sol::protected_function> s_cpp_function_registry;
    
public:
    struct CompileResult {
        bool success;
        std::string error_message;
        sol::protected_function function;
        
        CompileResult(bool s, const std::string& err, sol::protected_function func = {}) 
            : success(s), error_message(err), function(func) {}
    };
    
    static CompileResult lua_compile(const std::string& lua_code, 
                                   const std::string& cpp_name = "", 
                                   const std::string& lua_name = "") {
        // Validate input
        if (lua_code.empty()) {
            return CompileResult(false, "Empty Lua code provided");
        }

        try {
            // Compile the Lua code
            sol::load_result load_result = g_lua_state->load(lua_code);
            
            // Check if compilation was successful
            if (!load_result.valid()) {
                sol::error err = load_result;
                return CompileResult(false, "Syntax error: " + std::string(err.what()));
            }
            
            // Get the compiled function
            sol::protected_function compiled_func = load_result;
            
            // Register in C++ registry if cpp_name is provided
            if (!cpp_name.empty()) {
                if (s_cpp_function_registry.find(cpp_name) == s_cpp_function_registry.end()) {
                    s_cpp_function_registry[cpp_name] = compiled_func;
                }
            }
            
            // Register in Lua global namespace if lua_name is provided
            if (!lua_name.empty()) {
                (*g_lua_state)[lua_name] = compiled_func;
            }
            
            return CompileResult(true, "Success", compiled_func);
            
        } catch (const sol::error& e) {
            return CompileResult(false, "Compilation failed: " + std::string(e.what()));
        } catch (const std::exception& e) {
            return CompileResult(false, "Unexpected error: " + std::string(e.what()));
        }
    }
    
    // Get Lua code by c++ reference
    static sol::protected_function get_cpp_function(const std::string& cpp_name) {
        auto it = s_cpp_function_registry.find(cpp_name);
        return (it != s_cpp_function_registry.end()) ? it->second : sol::protected_function{};
    }
    
    // Get Lua code by Lua reference
    static sol::protected_function get_lua_function(const std::string& lua_name) {
        try {
            return (*g_lua_state)[lua_name];
        } catch (...) {
            return sol::protected_function{};
        }
    }

    // Check if Lua function exists in c++
    static bool has_cpp_function(const std::string& cpp_name) {
        return s_cpp_function_registry.find(cpp_name) != s_cpp_function_registry.end();
    }
    
    // Check if Lua function exists in Lua
    static bool has_lua_function(const std::string& lua_name) {
        try {
            auto lua_global = (*g_lua_state)[lua_name];
            return (lua_global.get_type() == sol::type::function);
        } catch (...) {
            return false;
        }
    }

    static void reset_compilation_state() {
        s_cpp_function_registry.clear();
        g_call_cache.clear(); // Clear the call cache
        g_code_cache.clear();
        cached_lua_init = sol::protected_function{};
        cached_lua_frame_update = sol::protected_function{};
        cached_lua_deinit = sol::protected_function{};
    }
};

std::unordered_map<std::string, sol::protected_function> Lua_Compilation::s_cpp_function_registry;

// Convert game_value to Lua object
static sol::object convert_game_value_to_lua(const game_value& value) {
    sol::state& lua = *g_lua_state;
    
    switch (value.type_enum()) {
        case types::game_data_type::BOOL:
            return sol::make_object(lua, static_cast<bool>(value));

        case types::game_data_type::SCALAR:
            return sol::make_object(lua, static_cast<float>(value));
            
        case types::game_data_type::STRING:
            return sol::make_object(lua, static_cast<std::string>(value));
            
        case types::game_data_type::ARRAY: {
            sol::table lua_table = lua.create_table();
            auto& array = value.to_array();

            for (size_t i = 0; i < array.size(); ++i) {
                // Recursively convert array elements
                lua_table[i + 1] = convert_game_value_to_lua(array[i]);
            }

            return sol::make_object(lua, lua_table);
        }

        case types::game_data_type::NOTHING:
        case types::game_data_type::ANY:
            return sol::make_object(lua, sol::nil);
            
        // Native Arma types - wrap in userdata
        case types::game_data_type::OBJECT:
        case types::game_data_type::GROUP:
        case types::game_data_type::NAMESPACE:
        case types::game_data_type::CONFIG:
        case types::game_data_type::CONTROL:
        case types::game_data_type::DISPLAY:
        case types::game_data_type::LOCATION:
        case types::game_data_type::SCRIPT:
        case types::game_data_type::SIDE:
        case types::game_data_type::TEXT:
        case types::game_data_type::TEAM_MEMBER:
        case types::game_data_type::CODE:
        case types::game_data_type::TASK:
        case types::game_data_type::DIARY_RECORD:
        case types::game_data_type::NetObject:
        case types::game_data_type::SUBGROUP:
        case types::game_data_type::TARGET:
            return sol::make_object(lua, GameValueWrapper(value));
            
        default:
            // For unhandled types, wrap as userdata
            return sol::make_object(lua, GameValueWrapper(value));
    }
}

// Convert Lua object to game_value
static game_value convert_lua_to_game_value(const sol::object& obj) {
    if (obj.get_type() == sol::type::nil) {
        return game_value();
    } else if (obj.get_type() == sol::type::boolean) {
        return game_value(obj.as<bool>());
    } else if (obj.get_type() == sol::type::number) {
        return game_value(obj.as<float>());
    } else if (obj.get_type() == sol::type::string) {
        return game_value(obj.as<std::string>());
    } else if (obj.get_type() == sol::type::table) {
        sol::table tbl = obj;
        auto_array<game_value> arr;

        for (size_t i = 1; i <= tbl.size(); i++) {
            arr.push_back(convert_lua_to_game_value(tbl[i]));
        }

        return game_value(std::move(arr));
    } else if (obj.get_type() == sol::type::userdata) {
        // Try to extract GameValueWrapper
        sol::optional<GameValueWrapper> wrapper = obj.as<sol::optional<GameValueWrapper>>();

        if (wrapper) {
            return wrapper->value;
        }
    }
    
    return game_value();
}

// Initialize Lua state
static void initialize_lua_state() {
    if (!g_lua_state) {
        g_lua_state = std::make_unique<sol::state>();
        
        g_lua_state->open_libraries(
            sol::lib::base,
            sol::lib::string,
            sol::lib::math,
            sol::lib::table,
            sol::lib::bit32,
            sol::lib::coroutine,
            sol::lib::utf8,
            sol::lib::jit
        );

        // Register GameValueWrapper userdata type
        g_lua_state->new_usertype<GameValueWrapper>("GameValue",
            sol::constructors<GameValueWrapper(), GameValueWrapper(const game_value&)>(),
            sol::meta_function::to_string, &GameValueWrapper::to_string,
            sol::meta_function::equal_to, &GameValueWrapper::equals,
            "value", &GameValueWrapper::value,
            "type_name", &GameValueWrapper::type_name
        );

        Lua_Compilation::lua_compile(
            R"(
                KHLUA_Var_Frame = 0
            )", "lua_init", ""
        );

        Lua_Compilation::lua_compile(
            R"(
                KHLUA_Var_Frame = KHLUA_Var_Frame + 1
            )", "lua_frame_update", ""
        );

        Lua_Compilation::lua_compile(
            R"(
                KHLUA_Var_Frame = 0
            )", "lua_deinit", ""
        );

        Lua_Compilation::lua_compile(
            R"(
                local params, code, count = ...

                if type(code) ~= "string" then
                    return "ERROR: code must be a string"
                end
                
                count = tonumber(count) or 1
                if count < 1 then
                    return "ERROR: execution count must be at least 1"
                end
                
                local compiled

                if not string.find(code, " ") and not string.find(code, "\t") and not string.find(code, "\n") then
                    compiled = _G[code]
                    if type(compiled) ~= "function" then
                        return "ERROR: Function '" .. code .. "' not found or is not a function"
                    end
                else
                    local func, err = load("return function(...) " .. code .. " end")

                    if not func then
                        return "ERROR: Failed to compile code: " .. tostring(err)
                    end

                    compiled = func()
                end

                local args = {}

                if type(params) == "table" then
                    args = params
                elseif params ~= nil then
                    args = {params}
                end

                local getTime = _G._kh_get_time_ms

                if not getTime then
                    return "ERROR: High precision timer not available"
                end
                
                if count > 10 then
                    for i = 1, 3 do
                        compiled(unpack(args))
                    end
                end
                
                local start_time = getTime()
                
                for i = 1, count do
                    compiled(unpack(args))
                end
                
                local end_time = getTime()
                local total_time = end_time - start_time
                local average_time = total_time / count

                local result = string.format("Count %d\nTotal: %.6f\nAverage: %.6f", 
                    count, total_time, average_time)

                return result
            )", "", "KHLUA_Fnc_ProfileCode"
        );

        Lua_Compilation::lua_compile(
            R"(
                local inputTable = ...

                if type(inputTable) ~= "table" then
                    return {}
                end
                
                local result = {}
                local index = 1
                
                for key, value in pairs(inputTable) do
                    result[index] = {key, value}
                    index = index + 1
                end
                
                return result
            )", "", "KHLUA_Fnc_TableToPairs"
        );

        Lua_Compilation::lua_compile(
            R"(
                local inputTable = ...

                if type(inputTable) ~= "table" then
                    return {}
                end
                
                local result = {}
                
                for i = 1, #inputTable do
                    local pair = inputTable[i]
                    if type(pair) == "table" and #pair >= 2 then
                        result[pair[1]] = pair[2]
                    end
                end
                
                return result
            )", "", "KHLUA_Fnc_PairsToTable"
        );

        Lua_Compilation::lua_compile(
            R"(
                local inputTable = ...

                if type(inputTable) ~= "table" then
                    return {}
                end
                
                local result = {}
                local index = 1
                
                for key, _ in pairs(inputTable) do
                    result[index] = key
                    index = index + 1
                end
                
                return result
            )", "", "KHLUA_Fnc_TableKeys"
        );

        Lua_Compilation::lua_compile(
            R"(
                local inputTable = ...

                if type(inputTable) ~= "table" then
                    return {}
                end
                
                local result = {}
                local index = 1
                
                for _, value in pairs(inputTable) do
                    result[index] = value
                    index = index + 1
                end
                
                return result
            )", "", "KHLUA_Fnc_TableValues"
        );

        Lua_Compilation::lua_compile(
            R"(
                local inputTable = ...

                if type(inputTable) ~= "table" then
                    return false
                end
                
                local count = 0
                
                for _ in pairs(inputTable) do
                    count = count + 1
                end
                
                for i = 1, count do
                    if inputTable[i] == nil then
                        return false
                    end
                end
                
                return true
            )", "", "KHLUA_Fnc_IsArray"
        );

        Lua_Compilation::lua_compile(
            R"(
                local inputValue = ...

                if inputValue == nil then
                    return "NOTHING"
                end
                
                local lua_type = type(inputValue)
                
                if lua_type == "boolean" then
                    return "BOOL"
                elseif lua_type == "number" then
                    return "SCALAR"
                elseif lua_type == "string" then
                    return "STRING"
                elseif lua_type == "table" then
                    return "ARRAY"
                elseif lua_type == "userdata" then
                    local mt = getmetatable(inputValue)

                    if mt and mt.__name == "GameValue" then
                        return inputValue:type_name()
                    else
                        return "USERDATA"
                    end
                else
                    return "UNKNOWN"
                end
            )", "", "KHLUA_Fnc_GetDataType"
        );

        (*g_lua_state)["_kh_get_time_ms"] = []() -> double {
            LARGE_INTEGER frequency, counter;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&counter);
            return (counter.QuadPart * 1000.0) / frequency.QuadPart;
        };

        cached_lua_init = Lua_Compilation::get_cpp_function("lua_init");
        cached_lua_frame_update = Lua_Compilation::get_cpp_function("lua_frame_update");
        cached_lua_deinit = Lua_Compilation::get_cpp_function("lua_deinit");
        
        // Create sqf table for SQF functions, limited commands
        auto sqf_table = g_lua_state->create_named_table("sqf");
        
        sqf_table["diag_log"] = [](const std::string& string) {
            sqf::diag_log(string);
            return sol::nil;
        };

        sqf_table["call"] = [](sol::variadic_args args) -> sol::object {
            if (args.size() == 1) {
                std::string code = args[0];
                return convert_game_value_to_lua(sqf::call2(sqf::compile(code)));
            } else if (args.size() == 2) {
                std::string code = args[0];
                sol::object arg = args[1];
                game_value gv_arg = convert_lua_to_game_value(arg);
                return convert_game_value_to_lua(sqf::call2(sqf::compile(code), gv_arg));
            }

            return sol::make_object(*g_lua_state, sol::nil);
        };

        sqf_table["command"] = [](sol::variadic_args args) -> sol::object {
            if (args.size() == 1) {
                std::string command = args[0];
                return convert_game_value_to_lua(sqf::call2(sqf::compile(command)));
            } else if (args.size() == 2) {
                std::string command = args[0];
                sol::object arg = args[1];
                game_value gv_arg = convert_lua_to_game_value(arg);
                return convert_game_value_to_lua(sqf::call2(sqf::compile(command + " _this"), gv_arg));
            } else if (args.size() == 3) {
                std::string command = args[0];
                sol::object arg1 = args[1];
                sol::object arg2 = args[2];
                
                // Convert both arguments to game_value
                game_value gv_arg1 = convert_lua_to_game_value(arg1);
                game_value gv_arg2 = convert_lua_to_game_value(arg2);
                
                // Create an array containing both arguments
                auto_array<game_value> arr;
                arr.push_back(gv_arg1);
                arr.push_back(gv_arg2);
                game_value args_array(std::move(arr));
                
                // Compile the binary command pattern and call with the array
                return convert_game_value_to_lua(sqf::call2(
                    sqf::compile("(_this select 0) " + command + " (_this select 1)"), 
                    args_array
                ));
            }

            return sol::make_object(*g_lua_state, sol::nil);
        };

        sqf_table["time"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::time());
        };
    }
}

// Native SQF command implementation for execution
static game_value execute_lua_sqf(game_value_parameter args, game_value_parameter code_or_function) {    
    try {
        std::string code_str = static_cast<std::string>(code_or_function);
        sol::object lua_args = sol::nil;

        if (!args.is_nil()) {
            lua_args = convert_game_value_to_lua(args);
        }
        
        // Check if it's a function call or code execution
        bool is_function = code_str.find(' ') == std::string::npos && 
                          code_str.find('\t') == std::string::npos &&
                          code_str.find('\n') == std::string::npos;
        
        sol::protected_function_result result;
        
        if (is_function) {
            // Try to get the function from cache or global namespace
            auto cache_it = g_call_cache.find(code_str);
            sol::protected_function func;
            
            if (cache_it != g_call_cache.end()) {
                func = cache_it->second.func;
            } else {
                // Get function from Lua global namespace
                func = (*g_lua_state)[code_str];

                if (!func.valid()) {
                    return game_value("ERROR: Function '" + code_str + "' not found");
                }

                // Cache it for future use
                if (g_call_cache.size() >= MAX_CACHE_SIZE) {
                    g_call_cache.erase(g_call_cache.begin());
                }
                
                g_call_cache[code_str] = {code_str, func, true};
            }
            
            // Call the function with arguments
            if (args.type_enum() == types::game_data_type::ARRAY) {
                // If args is an array, unpack it
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
                // No arguments
                result = func();
            } else {
                // Single argument
                result = func(lua_args);
            }
        } else {
            // Execute code with arguments in scope
            size_t code_hash = std::hash<std::string>{}(code_str);
            auto code_it = g_code_cache.find(code_hash);
            sol::protected_function compiled_code;
            
            if (code_it != g_code_cache.end()) {
                compiled_code = code_it->second;
            } else {
                // Compile the code
                std::string full_code = "return function(...) " + code_str + " end";
                sol::load_result load_res = g_lua_state->load(full_code);

                if (!load_res.valid()) {
                    sol::error err = load_res;
                    return game_value("ERROR: " + std::string(err.what()));
                }
                
                sol::protected_function factory = load_res;
                auto factory_result = factory();

                if (!factory_result.valid()) {
                    sol::error err = factory_result;
                    return game_value("ERROR: Failed to create function: " + std::string(err.what()));
                }
                
                compiled_code = factory_result;
                
                // Cache the compiled code
                if (g_code_cache.size() >= MAX_CACHE_SIZE) {
                    g_code_cache.erase(g_code_cache.begin());
                }

                g_code_cache[code_hash] = compiled_code;
            }
            
            // Execute with arguments
            if (args.type_enum() == types::game_data_type::ARRAY) {
                // Unpack array arguments
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
                result = compiled_code(lua_args);
            }
        }
        
        // Check for errors
        if (!result.valid()) {
            sol::error err = result;
            return game_value("ERROR: " + std::string(err.what()));
        }
        
        // Convert return value(s) to game_value
        if (result.return_count() == 0) {
            return game_value();  // nil
        } else if (result.return_count() == 1) {
            return convert_lua_to_game_value(result.get<sol::object>());
        } else {
            // Multiple return values - return as array
            auto_array<game_value> returns;
            returns.reserve(result.return_count());

            for (size_t i = 0; i < result.return_count(); i++) {
                returns.push_back(convert_lua_to_game_value(result[i]));
            }

            return game_value(std::move(returns));
        }
        
    } catch (const sol::error& e) {
        return game_value("ERROR: Lua error - " + std::string(e.what()));
    } catch (const std::exception& e) {
        return game_value("ERROR: " + std::string(e.what()));
    } catch (...) {
        return game_value("ERROR: Unknown error occurred");
    }
}

// Native SQF command implementation for compileLua
static game_value compile_lua_sqf(game_value_parameter name, game_value_parameter code) {    
    try {
        std::string lua_code = static_cast<std::string>(code);
        std::string lua_name = static_cast<std::string>(name);
        
        // Validate the Lua name
        if (lua_name.empty()) {
            return game_value("ERROR: Function name cannot be empty");
        }
        
        // Compile using the existing compilation system
        auto result = Lua_Compilation::lua_compile(lua_code, "", lua_name);
        
        if (result.success) {            
            // Also update our caches if the function was compiled
            if (!lua_name.empty() && result.function.valid()) {
                // Update call cache
                if (g_call_cache.size() >= MAX_CACHE_SIZE) {
                    // Remove oldest entry if cache is full
                    g_call_cache.erase(g_call_cache.begin());
                }
                g_call_cache[lua_name] = {lua_name, result.function, true};
            }
            
            return game_value(true);
        } else {
            return game_value("ERROR: " + result.error_message);
        }
        
    } catch (const sol::error& e) {
        return game_value("ERROR: Lua compilation error - " + std::string(e.what()));
    } catch (const std::exception& e) {
        return game_value("ERROR: " + std::string(e.what()));
    } catch (...) {
        return game_value("ERROR: Unknown error during compilation");
    }
}

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start() {
    initialize_lua_state();

    _execute_lua_sqf_command = intercept::client::host::register_sqf_command(
        "luaExecute",
        "Execute Lua code or function.",
        userFunctionWrapper<execute_lua_sqf>,
        types::game_data_type::ANY,     // Return type - can be any type
        types::game_data_type::ANY,     // Left argument - parameters (any type)
        types::game_data_type::STRING   // Right argument - code/function name
    );

    _compile_lua_sqf_command = intercept::client::host::register_sqf_command(
        "luaCompile",
        "Compile Lua code and register it as a named function",
        userFunctionWrapper<compile_lua_sqf>,
        types::game_data_type::ANY,     // Return type - true on success, error string on failure
        types::game_data_type::STRING,  // Left argument - name
        types::game_data_type::STRING   // Right argument - Lua code
    );

    sqf::diag_log("KH Framework Lua - Pre-start Complete");
}

void intercept::pre_init() {
    cached_lua_init();    
    sqf::diag_log("KH Framework Lua - Pre-init Complete");
}

void intercept::post_init() {
    sqf::diag_log("KH Framework Lua - Post-init Complete");
}

void intercept::on_frame() {
    cached_lua_frame_update();
}

void intercept::mission_ended() {
    cached_lua_deinit();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            break;
            
        case DLL_PROCESS_DETACH:
            if (lpReserved != nullptr) {
                // Process is terminating - skip complex cleanup to avoid deadlocks
                // The OS will reclaim all memory and handles automatically
                break;
            }

            if (g_lua_state) {
                Lua_Compilation::reset_compilation_state();
                g_lua_state.reset();
            }
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}