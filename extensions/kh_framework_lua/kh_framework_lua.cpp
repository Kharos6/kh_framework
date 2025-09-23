#include <windows.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "intercept/include/intercept.hpp"
#include "intercept/include/client/sqf/sqf.hpp"
#include "intercept/include/client/pointers.hpp"
#include "sol/sol.hpp"

static sol::protected_function cached_lua_init;
static sol::protected_function cached_lua_frame_update;  
static sol::protected_function cached_lua_deinit;

// Global Lua state and globals
static std::unique_ptr<sol::state> g_lua_state;
static int g_frame_counter = 0;

class Lua_Compilation {
private:
    static std::unordered_map<std::string, sol::protected_function> s_cpp_function_registry;
    
public:
    struct CompileResult {
        bool success;
        std::string error_message;
        sol::protected_function function;
        bool was_skipped; // Indicates if compilation was skipped due to existing function
        
        CompileResult(bool s, const std::string& err, sol::protected_function func = {}, bool skipped = false) 
            : success(s), error_message(err), function(func), was_skipped(skipped) {}
    };
    
    static CompileResult lua_compile(const std::string& lua_code, 
                                   const std::string& cpp_name = "", 
                                   const std::string& lua_name = "",
                                   bool allow_overwrite = false) {

        // Validate input
        if (lua_code.empty()) {
            return CompileResult(false, "Empty Lua code provided");
        }
        
        // Check for existing functions if overwrite is not allowed
        if (!allow_overwrite) {
            bool cpp_exists = false;
            bool lua_exists = false;
            
            // Check C++ registry
            if (!cpp_name.empty()) {
                cpp_exists = (s_cpp_function_registry.find(cpp_name) != s_cpp_function_registry.end());
            }
            
            // Check Lua globals
            if (!lua_name.empty()) {
                try {
                    auto lua_global = (*g_lua_state)[lua_name];
                    lua_exists = (lua_global.get_type() == sol::type::function);
                } catch (...) {
                    lua_exists = false; // Global doesn't exist or isn't accessible
                }
            }
            
            // If both names are specified and both exist, skip completely
            if (!cpp_name.empty() && !lua_name.empty() && cpp_exists && lua_exists) {
                auto existing_func = s_cpp_function_registry[cpp_name];
                return CompileResult(true, "Functions already exist", existing_func, true);
            }
            
            // If only C++ name specified and exists, return existing
            if (!cpp_name.empty() && lua_name.empty() && cpp_exists) {
                auto existing_func = s_cpp_function_registry[cpp_name];
                return CompileResult(true, "C++ function already exists", existing_func, true);
            }
            
            // If only Lua name specified and exists, try to get it
            if (cpp_name.empty() && !lua_name.empty() && lua_exists) {
                try {
                    sol::protected_function existing_func = (*g_lua_state)[lua_name];
                    return CompileResult(true, "Lua function already exists", existing_func, true);
                } catch (...) {
                    // Fall through to compilation if we can't retrieve the function
                }
            }
            
            // If we reach here, at least one registration target is new, so continue with compilation
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
                if (allow_overwrite || s_cpp_function_registry.find(cpp_name) == s_cpp_function_registry.end()) {
                    s_cpp_function_registry[cpp_name] = compiled_func;
                }
            }
            
            // Register in Lua global namespace if lua_name is provided
            if (!lua_name.empty()) {
                if (allow_overwrite) {
                    (*g_lua_state)[lua_name] = compiled_func;
                } else {
                    // Only set if it doesn't exist or isn't a function
                    try {
                        auto existing = (*g_lua_state)[lua_name];
                        if (existing.get_type() != sol::type::function) {
                            (*g_lua_state)[lua_name] = compiled_func;
                        }
                    } catch (...) {
                        // Doesn't exist, safe to set
                        (*g_lua_state)[lua_name] = compiled_func;
                    }
                }
            }
            
            return CompileResult(true, "Success", compiled_func, false);
            
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

    // Call this whenever resetting g_lua_state
    static void reset_compilation_state() {
        s_cpp_function_registry.clear();
        cached_lua_init = sol::protected_function{};
        cached_lua_frame_update = sol::protected_function{};
        cached_lua_deinit = sol::protected_function{};
    }
};

std::unordered_map<std::string, sol::protected_function> Lua_Compilation::s_cpp_function_registry;

// Convert Intercept game_value to Lua object
static sol::object convert_game_value_to_lua(const game_value& value) {
    sol::state& lua = *g_lua_state;
    
    switch (value.type_enum()) {
        case intercept::types::game_data_type::BOOL:
            return sol::make_object(lua, static_cast<bool>(value));

        case intercept::types::game_data_type::SCALAR:
            return sol::make_object(lua, static_cast<float>(value));
            
        case intercept::types::game_data_type::STRING:
            return sol::make_object(lua, static_cast<std::string>(value));
            
        case intercept::types::game_data_type::ARRAY: {
            sol::table lua_table = lua.create_table();
            auto& array = value.to_array();
            for (size_t i = 0; i < array.size(); ++i) {
                // Recursively convert array elements
                lua_table[i + 1] = convert_game_value_to_lua(array[i]);
            }
            return sol::make_object(lua, lua_table);
        }
        
        case intercept::types::game_data_type::NOTHING:
        case intercept::types::game_data_type::ANY:
            return sol::make_object(lua, sol::nil);
            
        default:
            // For unhandled types, return nil
            return sol::make_object(lua, sol::nil);
    }
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

        Lua_Compilation::lua_compile(
            R"(
                G_Frame = 0
            )", "lua_init", "", true
        );

        Lua_Compilation::lua_compile(
            R"(
                G_Frame = G_Frame + 1
            )", "lua_frame_update", "", true
        );

        Lua_Compilation::lua_compile(
            R"(
                G_Frame = 0
            )", "lua_deinit", "", true
        );

        cached_lua_init = Lua_Compilation::get_cpp_function("lua_init");
        cached_lua_frame_update = Lua_Compilation::get_cpp_function("lua_frame_update");
        cached_lua_deinit = Lua_Compilation::get_cpp_function("lua_deinit");
        
        // Create sqf table for SQF functions, limited commands
        auto sqf_table = g_lua_state->create_named_table("sqf");
        
        sqf_table["diag_log"] = [](const std::string& string) {
            intercept::sqf::diag_log(string);
            return sol::nil;
        };

        sqf_table["call"] = [](const std::string& string) -> sol::object {
            return convert_game_value_to_lua(intercept::sqf::call2(intercept::sqf::compile(string)));
        };
    }
}

// Helper function to parse array string and convert to Lua objects
static sol::table parse_parameters_to_lua(const std::string& params_str) {
    sol::state& lua = *g_lua_state;
    sol::table result = lua.create_table();
    
    if (params_str.empty() || params_str == "[]") {
        return result;
    }
    
    const char* ptr = params_str.c_str();
    int table_index = 1;
    
    // Skip opening bracket
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    if (*ptr != '[') return result;
    ptr++;
    
    std::string current_value;
    int bracket_depth = 1;
    bool in_string = false;
    char string_quote = 0;
    
    while (*ptr && bracket_depth > 0) {
        if (!in_string) {
            if (*ptr == '"' || *ptr == '\'') {
                in_string = true;
                string_quote = *ptr;
                current_value += *ptr;
            } else if (*ptr == '[') {
                bracket_depth++;
                current_value += *ptr;
            } else if (*ptr == ']') {
                bracket_depth--;
                if (bracket_depth == 0) {
                    // End of array, process last value if any
                    if (!current_value.empty()) {
                        // Trim whitespace
                        size_t start = current_value.find_first_not_of(" \t\n\r");
                        size_t end = current_value.find_last_not_of(" \t\n\r");
                        if (start != std::string::npos) {
                            current_value = current_value.substr(start, end - start + 1);
                        }
                        
                        // Parse value type
                        if (current_value[0] == '"' || current_value[0] == '\'') {
                            // String - remove quotes
                            char quote = current_value[0];
                            std::string str_val;
                            for (size_t i = 1; i < current_value.length() - 1; i++) {
                                if (current_value[i] == quote && i + 1 < current_value.length() - 1 && 
                                    current_value[i + 1] == quote) {
                                    str_val += quote;
                                    i++;
                                } else {
                                    str_val += current_value[i];
                                }
                            }
                            result[table_index++] = str_val;
                        } else if (current_value == "true" || current_value == "false") {
                            result[table_index++] = (current_value == "true");
                        } else if (current_value == "nil" || current_value == "null") {
                            result[table_index++] = sol::nil;
                        } else if (current_value[0] == '[') {
                            // Nested array - recursively parse
                            result[table_index++] = parse_parameters_to_lua(current_value);
                        } else {
                            // Try to parse as number
                            char* endptr;
                            double num_val = strtod(current_value.c_str(), &endptr);
                            if (*endptr == '\0') {
                                result[table_index++] = num_val;
                            } else {
                                // Fallback to string
                                result[table_index++] = current_value;
                            }
                        }
                    }
                    break;
                }
                current_value += *ptr;
            } else if (*ptr == ',' && bracket_depth == 1) {
                // Element separator
                if (!current_value.empty()) {
                    // Process current value (same logic as above)
                    size_t start = current_value.find_first_not_of(" \t\n\r");
                    size_t end = current_value.find_last_not_of(" \t\n\r");
                    if (start != std::string::npos) {
                        current_value = current_value.substr(start, end - start + 1);
                    }
                    
                    if (current_value[0] == '"' || current_value[0] == '\'') {
                        char quote = current_value[0];
                        std::string str_val;
                        for (size_t i = 1; i < current_value.length() - 1; i++) {
                            if (current_value[i] == quote && i + 1 < current_value.length() - 1 && 
                                current_value[i + 1] == quote) {
                                str_val += quote;
                                i++;
                            } else {
                                str_val += current_value[i];
                            }
                        }
                        result[table_index++] = str_val;
                    } else if (current_value == "true" || current_value == "false") {
                        result[table_index++] = (current_value == "true");
                    } else if (current_value == "nil" || current_value == "null") {
                        result[table_index++] = sol::nil;
                    } else if (current_value[0] == '[') {
                        result[table_index++] = parse_parameters_to_lua(current_value);
                    } else {
                        char* endptr;
                        double num_val = strtod(current_value.c_str(), &endptr);
                        if (*endptr == '\0') {
                            result[table_index++] = num_val;
                        } else {
                            result[table_index++] = current_value;
                        }
                    }
                }
                current_value.clear();
            } else {
                current_value += *ptr;
            }
        } else {
            if (*ptr == string_quote) {
                if (*(ptr + 1) == string_quote) {
                    current_value += *ptr;
                    current_value += *(ptr + 1);
                    ptr++;
                } else {
                    in_string = false;
                    current_value += *ptr;
                }
            } else {
                current_value += *ptr;
            }
        }
        ptr++;
    }
    
    return result;
}

// Execute Lua code or function
static std::string execute_lua(const std::string& code_or_function, const std::string& parameters) {
    if (!g_lua_state) {
        return "ERROR: Lua state not initialized";
    }
    
    try {
        sol::table args = parse_parameters_to_lua(parameters);
        
        // Check if it's code (contains space/tab) or function name
        bool is_code = (code_or_function.find(' ') != std::string::npos || 
                       code_or_function.find('\t') != std::string::npos ||
                       code_or_function.find('\n') != std::string::npos);
        
        if (is_code) {
            // Execute as code with arguments
            std::string wrapped_code = "return (function(...) " + code_or_function + " end)(";
            
            // Build argument list
            bool first = true;
            for (size_t i = 1; i <= args.size(); i++) {
                if (!first) wrapped_code += ", ";
                wrapped_code += "select(" + std::to_string(i) + ", ...)";
                first = false;
            }
            wrapped_code += ")";
            
            // Create argument passing function
            auto exec_func = [&args]() -> sol::variadic_results {
                sol::variadic_results results;
                for (size_t i = 1; i <= args.size(); i++) {
                    results.push_back(args[i]);
                }
                return results;
            };
            
            (*g_lua_state)["_kh_temp_args"] = exec_func;
            
            // Rewrite to use the temp args
            wrapped_code = "return (function(...) " + code_or_function + " end)(_kh_temp_args())";
            
            sol::protected_function_result result = g_lua_state->safe_script(wrapped_code, sol::script_pass_on_error);
            
            (*g_lua_state)["_kh_temp_args"] = sol::nil;
            
            if (!result.valid()) {
                sol::error err = result;
                return "ERROR: " + std::string(err.what());
            }
            
            // Convert result to string
            sol::type result_type = result.get_type();
            if (result_type == sol::type::nil) {
                return "nil";
            } else if (result_type == sol::type::boolean) {
                return result.get<bool>() ? "true" : "false";
            } else if (result_type == sol::type::number) {
                return std::to_string(result.get<double>());
            } else if (result_type == sol::type::string) {
                return "\"" + result.get<std::string>() + "\"";
            } else if (result_type == sol::type::table) {
                return "[table]"; // Could implement full serialization if needed
            } else {
                // Get the type name properly
                lua_State* L = g_lua_state->lua_state();
                return "[" + std::string(lua_typename(L, static_cast<int>(result_type))) + "]";
            }
        } else {
            // Execute as function
            if (!Lua_Compilation::has_lua_function(code_or_function)) {
                return "ERROR: Function '" + code_or_function + "' not found";
            }
            
            sol::protected_function func = Lua_Compilation::get_lua_function(code_or_function);
            if (!func.valid()) {
                return "ERROR: Failed to get function '" + code_or_function + "'";
            }
            
            // Call with unpacked arguments
            std::vector<sol::object> arg_vec;
            for (size_t i = 1; i <= args.size(); i++) {
                arg_vec.push_back(args[i]);
            }
            
            sol::protected_function_result result = func(sol::as_args(arg_vec));
            
            if (!result.valid()) {
                sol::error err = result;
                return "ERROR: " + std::string(err.what());
            }
            
            // Convert result
            sol::type result_type = result.get_type();
            if (result_type == sol::type::nil) {
                return "nil";
            } else if (result_type == sol::type::boolean) {
                return result.get<bool>() ? "true" : "false";
            } else if (result_type == sol::type::number) {
                return std::to_string(result.get<double>());
            } else if (result_type == sol::type::string) {
                return "\"" + result.get<std::string>() + "\"";
            } else if (result_type == sol::type::table) {
                return "[table]";
            } else {
                // Get the type name properly
                lua_State* L = g_lua_state->lua_state();
                return "[" + std::string(lua_typename(L, static_cast<int>(result_type))) + "]";
            }
        }
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

// Compile Lua code
static std::string compile_lua(const std::string& code, const std::string& lua_name, bool overwrite) {
    if (!g_lua_state) {
        return "ERROR: Lua state not initialized";
    }
    
    if (lua_name.empty()) {
        return "ERROR: Lua name cannot be empty";
    }
    
    auto result = Lua_Compilation::lua_compile(code, "", lua_name, overwrite);
    
    if (result.success) {
        if (result.was_skipped) {
            return "SKIPPED: Function already exists";
        } else {
            return "SUCCESS";
        }
    } else {
        return "ERROR: " + result.error_message;
    }
}

// External C interface for kh_framework.c
extern "C" {
    __declspec(dllexport) const char* KHLuaFrameworkInterface(const char* function, const char* parameters) {
        static std::string result_buffer;
        
        if (!function || !parameters) {
            result_buffer = "ERROR: Invalid arguments";
            return result_buffer.c_str();
        }
        
        std::string func_name(function);
        std::string params(parameters);
        
        // Parse the parameters array to extract individual arguments
        std::vector<std::string> args;
        const char* ptr = params.c_str();
        
        // Skip whitespace and opening bracket
        while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
        if (*ptr != '[') {
            result_buffer = "ERROR: Parameters must be an array";
            return result_buffer.c_str();
        }
        ptr++;
        
        std::string current_arg;
        int bracket_depth = 1;
        bool in_string = false;
        char string_quote = 0;
        
        while (*ptr && bracket_depth > 0) {
            if (!in_string) {
                if (*ptr == '"' || *ptr == '\'') {
                    in_string = true;
                    string_quote = *ptr;
                    current_arg += *ptr;
                } else if (*ptr == '[') {
                    bracket_depth++;
                    current_arg += *ptr;
                } else if (*ptr == ']') {
                    bracket_depth--;
                    if (bracket_depth == 0) {
                        if (!current_arg.empty()) {
                            args.push_back(current_arg);
                        }
                        break;
                    }
                    current_arg += *ptr;
                } else if (*ptr == ',' && bracket_depth == 1) {
                    if (!current_arg.empty()) {
                        // Trim whitespace
                        size_t start = current_arg.find_first_not_of(" \t\n\r");
                        size_t end = current_arg.find_last_not_of(" \t\n\r");
                        if (start != std::string::npos) {
                            current_arg = current_arg.substr(start, end - start + 1);
                        }
                        args.push_back(current_arg);
                    }
                    current_arg.clear();
                } else {
                    current_arg += *ptr;
                }
            } else {
                if (*ptr == string_quote) {
                    if (*(ptr + 1) == string_quote) {
                        current_arg += *ptr;
                        current_arg += *(ptr + 1);
                        ptr++;
                    } else {
                        in_string = false;
                        current_arg += *ptr;
                    }
                } else {
                    current_arg += *ptr;
                }
            }
            ptr++;
        }
        
        // Process the function call
        if (func_name == "ExecuteLua") {
            if (args.size() < 1) {
                result_buffer = "ERROR: ExecuteLua requires at least 1 argument";
                return result_buffer.c_str();
            }
            
            // Remove quotes from code/function argument
            std::string code_or_func = args[0];
            if ((code_or_func[0] == '"' && code_or_func[code_or_func.length()-1] == '"') ||
                (code_or_func[0] == '\'' && code_or_func[code_or_func.length()-1] == '\'')) {
                code_or_func = code_or_func.substr(1, code_or_func.length() - 2);
            }
            
            // Build parameters array from remaining arguments
            std::string exec_params = "[";
            for (size_t i = 1; i < args.size(); i++) {
                if (i > 1) exec_params += ", ";
                exec_params += args[i];
            }
            exec_params += "]";
            
            result_buffer = execute_lua(code_or_func, exec_params);
        } else if (func_name == "CompileLua") {
            if (args.size() != 3) {
                result_buffer = "ERROR: CompileLua requires exactly 3 arguments";
                return result_buffer.c_str();
            }
            
            // Extract arguments
            std::string code = args[0];
            std::string lua_name = args[1];
            std::string overwrite_str = args[2];
            
            // Remove quotes
            if ((code[0] == '"' && code[code.length()-1] == '"') ||
                (code[0] == '\'' && code[code.length()-1] == '\'')) {
                code = code.substr(1, code.length() - 2);
            }
            if ((lua_name[0] == '"' && lua_name[lua_name.length()-1] == '"') ||
                (lua_name[0] == '\'' && lua_name[lua_name.length()-1] == '\'')) {
                lua_name = lua_name.substr(1, lua_name.length() - 2);
            }
            
            bool overwrite = (overwrite_str == "true" || overwrite_str == "1");
            
            result_buffer = compile_lua(code, lua_name, overwrite);
        } else {
            result_buffer = "ERROR: Unknown function '" + func_name + "'";
        }
        
        return result_buffer.c_str();
    }
}

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start() {
    initialize_lua_state();
    intercept::sqf::diag_log("KH Framework Lua - Pre-start Complete");
}

void intercept::pre_init() {
    g_frame_counter = 0;
    cached_lua_init();
    intercept::sqf::diag_log("KH Framework Lua - Pre-init Complete");
}

void intercept::post_init() {
    intercept::sqf::diag_log("KH Framework Lua - Post-init Complete");
}

void intercept::on_frame() {
    g_frame_counter++;
    cached_lua_frame_update();
}

void intercept::mission_ended() {
    g_frame_counter = 0;
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