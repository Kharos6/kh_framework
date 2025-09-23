#include <windows.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "intercept/include/intercept.hpp"
#include "intercept/include/client/sqf/sqf.hpp"
#include "intercept/include/client/pointers.hpp"
#include "sol/sol.hpp"

// UTF-8 helper functions
static size_t utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;
    else if ((c & 0xE0) == 0xC0) return 2;
    else if ((c & 0xF0) == 0xE0) return 3;
    else if ((c & 0xF8) == 0xF0) return 4;
    return 1; // Invalid, treat as single byte
}

static bool is_utf8_continuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

// UTF-8 safe substring that won't break multi-byte characters
static std::string utf8_safe_substr(const std::string& str, size_t start, size_t length) {
    if (start >= str.length()) return "";
    
    // Find actual start position (skip to next character boundary if needed)
    while (start > 0 && is_utf8_continuation(str[start])) {
        start--;
    }
    
    // Find safe end position
    size_t end = std::min(start + length, str.length());
    while (end < str.length() && is_utf8_continuation(str[end])) {
        end++;
    }
    
    return str.substr(start, end - start);
}

static sol::protected_function cached_lua_init;
static sol::protected_function cached_lua_frame_update;  
static sol::protected_function cached_lua_deinit;

// Global Lua state and globals
static std::unique_ptr<sol::state> g_lua_state;
static int g_frame_counter = 0;

// Userdata wrapper for game_value to preserve native Arma types
struct GameValueWrapper {
    game_value value;
    
    GameValueWrapper() = default;
    GameValueWrapper(const game_value& v) : value(v) {}
    GameValueWrapper(game_value&& v) : value(std::move(v)) {}
    
    std::string to_string() const {
        if (value.is_nil()) return "nil";
        
        switch (value.type_enum()) {
            case intercept::types::game_data_type::OBJECT:
                return "[OBJECT]";
            case intercept::types::game_data_type::GROUP:
                return "[GROUP]";
            case intercept::types::game_data_type::NAMESPACE:
                return "[NAMESPACE]";
            case intercept::types::game_data_type::CONFIG:
                return "[CONFIG]";
            case intercept::types::game_data_type::CONTROL:
                return "[CONTROL]";
            case intercept::types::game_data_type::DISPLAY:
                return "[DISPLAY]";
            case intercept::types::game_data_type::LOCATION:
                return "[LOCATION]";
            case intercept::types::game_data_type::SCRIPT:
                return "[SCRIPT]";
            case intercept::types::game_data_type::SIDE:
                return "[SIDE]";
            case intercept::types::game_data_type::TEXT:
                return "[TEXT]";
            case intercept::types::game_data_type::TEAM_MEMBER:
                return "[TEAM_MEMBER]";
            case intercept::types::game_data_type::CODE:
                return "[CODE]";
            case intercept::types::game_data_type::TASK:
                return "[TASK]";
            default:
                // Explicitly convert r_string to std::string
                return value.data ? static_cast<std::string>(value.data->to_string()) : "nil";
        }
    }
    
    bool equals(const GameValueWrapper& other) const {
        return value == other.value;
    }
};

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
            
        // Native Arma types - wrap in userdata
        case intercept::types::game_data_type::OBJECT:
        case intercept::types::game_data_type::GROUP:
        case intercept::types::game_data_type::NAMESPACE:
        case intercept::types::game_data_type::CONFIG:
        case intercept::types::game_data_type::CONTROL:
        case intercept::types::game_data_type::DISPLAY:
        case intercept::types::game_data_type::LOCATION:
        case intercept::types::game_data_type::SCRIPT:
        case intercept::types::game_data_type::SIDE:
        case intercept::types::game_data_type::TEXT:
        case intercept::types::game_data_type::TEAM_MEMBER:
        case intercept::types::game_data_type::CODE:
        case intercept::types::game_data_type::TASK:
        case intercept::types::game_data_type::DIARY_RECORD:
        case intercept::types::game_data_type::NetObject:
        case intercept::types::game_data_type::SUBGROUP:
        case intercept::types::game_data_type::TARGET:
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
            sol::lib::jit
        );

        // Register GameValueWrapper userdata type
        g_lua_state->new_usertype<GameValueWrapper>("GameValue",
            sol::constructors<GameValueWrapper(), GameValueWrapper(const game_value&)>(),
            sol::meta_function::to_string, &GameValueWrapper::to_string,
            sol::meta_function::equal_to, &GameValueWrapper::equals,
            "value", &GameValueWrapper::value
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
        
        Lua_Compilation::lua_compile(
            R"(
                local inputArray = {...}
                
                local function ParseArgumentsTable(array)
                    local result = {}
                    
                    for i, value in ipairs(array) do
                        if type(value) == "table" and #value == 2 then
                            result[i] = sqf.call("_this call KH_fnc_parseValue", value)
                        end
                    end
                    
                    return result
                end
                
                return ParseArgumentsTable(inputArray)
            )", "", "ParseArguments", true
        );

        Lua_Compilation::lua_compile(
            R"(
                local inputArray = ...
                
                local function ConvertLuaToGameValue(value)
                    if type(value) == "table" then
                        local converted = {}
                        local isArray = true
                        local maxIndex = 0
                        
                        for k, v in pairs(value) do
                            if type(k) ~= "number" or k ~= math.floor(k) or k < 1 then
                                isArray = false
                                break
                            end

                            if k > maxIndex then maxIndex = k end
                        end
                        
                        if isArray and maxIndex > 0 then

                            for i = 1, maxIndex do
                                if value[i] ~= nil then
                                    converted[i] = ConvertLuaToGameValue(value[i])
                                else
                                    converted[i] = nil
                                end
                            end
                        else
                            for k, v in pairs(value) do
                                table.insert(converted, {ConvertLuaToGameValue(k), ConvertLuaToGameValue(v)})
                            end
                        end
                        
                        return converted
                    elseif value == nil then
                        return nil
                    else
                        return value
                    end
                end
                
                local function SerializeReturnTable(value)
                    if type(value) == "table" then
                        local result = {}

                        for i, v in ipairs(value) do
                            local converted = ConvertLuaToGameValue(v)
                            result[i] = sqf.call("['', _this] call KH_fnc_serializeValue", converted)
                        end

                        return result
                    else
                        local converted = ConvertLuaToGameValue(value)
                        return sqf.call("['', _this] call KH_fnc_serializeValue", converted)
                    end
                end
                
                return SerializeReturnTable(inputArray)
            )", "", "SerializeReturn", true
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

        sqf_table["call"] = [](sol::variadic_args args) -> sol::object {
            if (args.size() == 1) {
                // Simple call with just code
                std::string code = args[0];
                return convert_game_value_to_lua(intercept::sqf::call2(intercept::sqf::compile(code)));
            } else if (args.size() == 2) {
                // Call with code and arguments
                std::string code = args[0];
                sol::object arg = args[1];
                game_value gv_arg = convert_lua_to_game_value(arg);
                return convert_game_value_to_lua(intercept::sqf::call2(intercept::sqf::compile(code), gv_arg));
            }
            return sol::make_object(*g_lua_state, sol::nil);
        };
        
        // Add more SQF functions that work with native types
        sqf_table["player"] = []() -> sol::object {
            return convert_game_value_to_lua(intercept::sqf::player());
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
    
    // First try to parse as game_value if it's not a plain array string
    // This handles cases where we're passing native types directly
    try {
        game_value parsed = intercept::sqf::call2(intercept::sqf::compile("_this"), game_value(params_str));
        if (!parsed.is_nil() && parsed.type_enum() == intercept::types::game_data_type::ARRAY) {
            auto& array = parsed.to_array();
            for (size_t i = 0; i < array.size(); ++i) {
                result[i + 1] = convert_game_value_to_lua(array[i]);
            }
            return result;
        }
    } catch (...) {
        // Fall back to string parsing
    }
    
    // Original string parsing logic for backwards compatibility
    const char* ptr = params_str.c_str();
    int table_index = 1;
    
    // [Rest of the original parsing logic stays the same...]
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
                            // String - remove quotes with UTF-8 safety
                            char quote = current_value[0];
                            std::string str_val;
                            size_t i = 1;
                            while (i < current_value.length() - 1) {
                                unsigned char c = current_value[i];
                                
                                if (c == quote && i + 1 < current_value.length() - 1 && 
                                    current_value[i + 1] == quote) {
                                    str_val += quote;
                                    i += 2;
                                } else if (c < 0x80) {
                                    str_val += c;
                                    i++;
                                } else {
                                    // UTF-8 multi-byte character
                                    size_t char_len = utf8_char_len(c);
                                    if (i + char_len <= current_value.length() - 1) {
                                        str_val.append(current_value, i, char_len);
                                    }
                                    i += char_len;
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
                // Element separator - process current value same as above
                if (!current_value.empty()) {
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

// Helper to serialize Lua table to Arma array format
static std::string serialize_lua_value(const sol::object& obj, bool is_root = true) {
    if (obj.get_type() == sol::type::nil) {
        return "nil";
    } else if (obj.get_type() == sol::type::boolean) {
        return obj.as<bool>() ? "true" : "false";
    } else if (obj.get_type() == sol::type::number) {
        double val = obj.as<double>();
        // Check if it's an integer
        if (val == std::floor(val)) {
            return std::to_string(static_cast<long long>(val));
        }
        return std::to_string(val);
    } else if (obj.get_type() == sol::type::string) {
        std::string str = obj.as<std::string>();
        
        // If this is the root level (direct return), don't add quotes
        if (is_root) {
            return str; // Return as-is, preserving UTF-8
        }
        
        // Inside arrays/tables, we need quotes for Arma to parse correctly
        std::string escaped;
        escaped += '"';
        
        // UTF-8 aware quote escaping
        size_t i = 0;
        while (i < str.length()) {
            unsigned char c = str[i];
            
            if (c == '"') {
                escaped += "\"\"";  // Double quotes for Arma escaping
                i++;
            } else if (c < 0x80) {
                // ASCII character
                escaped += c;
                i++;
            } else {
                // UTF-8 multi-byte character - copy entire sequence
                size_t char_len = utf8_char_len(c);
                if (i + char_len <= str.length()) {
                    escaped.append(str, i, char_len);
                }
                i += char_len;
            }
        }
        
        escaped += '"';
        return escaped;
    } else if (obj.get_type() == sol::type::table) {
        sol::table tbl = obj.as<sol::table>();
        
        // Get the actual size using sol's size() method
        size_t tbl_size = tbl.size();
        
        // Check if it's an array (has sequential numeric indices from 1 to size)
        bool is_array = tbl_size > 0;
        if (is_array) {
            for (size_t i = 1; i <= tbl_size; i++) {
                sol::object elem = tbl[i];
                if (elem.get_type() == sol::type::nil) {
                    is_array = false;
                    break;
                }
            }
        }
        
        // If it's not an array but has elements, it's a hashmap
        if (!is_array && tbl_size == 0) {
            // Check if table has any non-array elements
            size_t actual_count = 0;
            for (auto& pair : tbl) {
                actual_count++;
            }
            if (actual_count == 0) {
                return "[]";  // Empty table
            }
        }
        
        std::string result = "[";
        
        if (is_array) {
            // Serialize as array
            bool first = true;
            for (size_t i = 1; i <= tbl_size; i++) {
                if (!first) result += ", ";
                result += serialize_lua_value(tbl[i], false);  // false = not root level
                first = false;
            }
        } else {
            // Serialize as hashmap (key-value pairs)
            bool first = true;
            for (auto& pair : tbl) {
                if (!first) result += ", ";
                result += "[";
                result += serialize_lua_value(pair.first, false);
                result += ", ";
                result += serialize_lua_value(pair.second, false);
                result += "]";
                first = false;
            }
        }
        
        result += "]";
        return result;
    } else if (obj.get_type() == sol::type::userdata) {
        // Check for GameValueWrapper
        sol::optional<GameValueWrapper> wrapper = obj.as<sol::optional<GameValueWrapper>>();
        if (wrapper) {
            return wrapper->to_string();
        }
        return "[nil]";
    } else {
        return "[nil]";
    }
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
        
        sol::protected_function_result result;
        
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
            
            result = g_lua_state->safe_script(wrapped_code, sol::script_pass_on_error);
            
            (*g_lua_state)["_kh_temp_args"] = sol::nil;
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
            
            result = func(sol::as_args(arg_vec));
        }
        
        if (!result.valid()) {
            sol::error err = result;
            return "ERROR: " + std::string(err.what());
        }
        
        // Extract the first return value from the result
        if (result.return_count() == 0) {
            return "nil";
        } else if (result.return_count() == 1) {
            // Single return value
            sol::object return_value = result;
            return serialize_lua_value(return_value, true);  // true = root level
        } else {
            // Multiple return values - wrap in array
            std::string multi_result = "[";
            bool first = true;
            for (size_t i = 0; i < result.return_count(); i++) {
                if (!first) multi_result += ", ";
                sol::object val = result[i];
                multi_result += serialize_lua_value(val, false);  // false = inside array
                first = false;
            }
            multi_result += "]";
            return multi_result;
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
            if (args.size() != 2) {
                result_buffer = "ERROR: ExecuteLua requires exactly 2 arguments (code/function and parameters array)";
                return result_buffer.c_str();
            }
            
            // Extract and clean the code/function argument
            std::string code_or_func = args[0];
            
            // Handle Arma's quote escaping - UTF-8 safe
            if (code_or_func.length() >= 2 && 
                ((code_or_func[0] == '"' && code_or_func[code_or_func.length()-1] == '"') ||
                (code_or_func[0] == '\'' && code_or_func[code_or_func.length()-1] == '\''))) {
                char quote = code_or_func[0];
                std::string cleaned;
                
                // Skip opening quote and process until closing quote
                size_t i = 1;
                while (i < code_or_func.length() - 1) {
                    unsigned char c = code_or_func[i];
                    
                    if (c == quote && i + 1 < code_or_func.length() - 1 && 
                        code_or_func[i + 1] == quote) {
                        // Doubled quote - add single quote
                        cleaned += quote;
                        i += 2; // Skip both quotes
                    } else if (c < 0x80) {
                        // ASCII character
                        cleaned += c;
                        i++;
                    } else {
                        // UTF-8 multi-byte character
                        size_t char_len = utf8_char_len(c);
                        if (i + char_len <= code_or_func.length() - 1) {
                            cleaned.append(code_or_func, i, char_len);
                        }
                        i += char_len;
                    }
                }
                code_or_func = cleaned;
            }
            
            // The second argument is already the parameters array
            std::string exec_params = args[1];
            
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
            
            // Handle quote escaping for code
            if ((code[0] == '"' && code[code.length()-1] == '"') ||
                (code[0] == '\'' && code[code.length()-1] == '\'')) {
                char quote = code[0];
                std::string cleaned;
                
                for (size_t i = 1; i < code.length() - 1; i++) {
                    if (code[i] == quote && i + 1 < code.length() - 1 && 
                        code[i + 1] == quote) {
                        cleaned += quote;
                        i++;
                    } else {
                        cleaned += code[i];
                    }
                }
                code = cleaned;
            }
            
            // Handle quote escaping for lua_name
            if ((lua_name[0] == '"' && lua_name[lua_name.length()-1] == '"') ||
                (lua_name[0] == '\'' && lua_name[lua_name.length()-1] == '\'')) {
                char quote = lua_name[0];
                std::string cleaned;
                
                for (size_t i = 1; i < lua_name.length() - 1; i++) {
                    if (lua_name[i] == quote && i + 1 < lua_name.length() - 1 && 
                        lua_name[i + 1] == quote) {
                        cleaned += quote;
                        i++;
                    } else {
                        cleaned += lua_name[i];
                    }
                }
                lua_name = cleaned;
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