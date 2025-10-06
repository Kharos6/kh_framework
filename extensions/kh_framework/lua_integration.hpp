#pragma once

using namespace intercept;
using namespace intercept::types;

struct LuaCallCache {
    std::string function_name;
    sol::protected_function func;
    bool is_valid;
};

static std::unique_ptr<sol::state> g_lua_state;
static std::unordered_map<std::string, LuaCallCache> g_call_cache;
static std::unordered_map<size_t, sol::protected_function> g_code_cache;
static std::unordered_map<size_t, game_value> g_sqf_compiled_cache;
static std::unordered_map<std::string, game_value> g_sqf_function_cache;
static std::unordered_map<std::string, game_value> g_sqf_command_cache;

class LuaStackGuard {
    lua_State* L;
    int top;
public:
    LuaStackGuard(sol::state& state) : L(state.lua_state()), top(lua_gettop(L)) {}
    
    ~LuaStackGuard() { 
        lua_settop(L, top);  // Restore stack to the original size
    }
};

class Lua_Compilation {
public:
    static std::string preprocess_lua_operators(const std::string& code) {
        // Quick check - if no C-style operators, return immediately
        if (code.find("!=") == std::string::npos && 
            code.find("&&") == std::string::npos && 
            code.find("||") == std::string::npos) {
            return code;
        }
        
        std::string result = code;  // Keep original for range checking
        
        // Build a map of string literal ranges to avoid
        std::vector<std::pair<size_t, size_t>> string_ranges;
        
        // Find all string literals (including long brackets)
        for (size_t i = 0; i < result.length(); i++) {
            // Check for long bracket strings [[...]]
            if (i + 1 < result.length() && result[i] == '[' && result[i + 1] == '[') {
                size_t start = i;
                i += 2;

                while (i + 1 < result.length()) {
                    if (result[i] == ']' && result[i + 1] == ']') {
                        string_ranges.push_back({start, i + 1});
                        i++;
                        break;
                    }

                    i++;
                }
            }
            // Check for long bracket strings with equals [==[...]==]
            else if (result[i] == '[') {
                size_t start = i;
                size_t equals_count = 0;
                size_t j = i + 1;
                
                while (j < result.length() && result[j] == '=') {
                    equals_count++;
                    j++;
                }
                
                if (j < result.length() && result[j] == '[') {
                    i = j + 1;
                    std::string closing = "]" + std::string(equals_count, '=') + "]";
                    size_t close_pos = result.find(closing, i);

                    if (close_pos != std::string::npos) {
                        string_ranges.push_back({start, close_pos + closing.length() - 1});
                        i = close_pos + closing.length() - 1;
                    }
                }
            }
            // Regular quoted strings
            else if (result[i] == '"' || result[i] == '\'') {
                char quote = result[i];
                size_t start = i;
                i++;

                while (i < result.length()) {
                    if (result[i] == quote) {
                        int backslash_count = 0;
                        
                        for (int j = static_cast<int>(i) - 1; j >= 0 && result[j] == '\\'; j--) {
                            backslash_count++;
                        }
                        
                        if (backslash_count % 2 == 0) {
                            string_ranges.push_back({start, i});
                            break;
                        }
                    }
                    i++;
                }
            }
        }
        
        // Helper to check if position is in any string
        auto in_string = [&](size_t pos) {
            for (const auto& range : string_ranges) {
                if (pos >= range.first && pos <= range.second) {
                    return true;
                }
            }
            return false;
        };
        
        // Replacement definitions
        struct Replacement {
            std::string from;
            std::string to;
            size_t extra_bytes;  // How many extra bytes this replacement adds
        };

        std::vector<Replacement> replacements = {
            {"!=", "~=", 0},
            {"&&", " and ", 3},
            {"||", " or ", 2}
        };
                
        // Calculate space needed based on actual operator count
        size_t extra_space = 0;
        size_t pos = 0;

        while (pos < result.length()) {
            if (!in_string(pos)) {
                for (const auto& rep : replacements) {
                    if (result.compare(pos, rep.from.length(), rep.from) == 0) {
                        extra_space += rep.extra_bytes;
                        pos += rep.from.length();
                        goto next_position;  // Skip to next position after match
                    }
                }
            }
            pos++;
            next_position:;
        }

        // Build output string in single pass
        std::string output;
        output.reserve(result.length() + extra_space);

        for (size_t i = 0; i < result.length(); ) {
            bool replaced = false;
            
            // Only try replacements if not inside a string
            if (!in_string(i)) {
                for (const auto& rep : replacements) {
                    if (result.compare(i, rep.from.length(), rep.from) == 0) {
                        output += rep.to;
                        i += rep.from.length();
                        replaced = true;
                        break;
                    }
                }
            }
            
            // If no replacement made, copy character as-is
            if (!replaced) {
                output += result[i];
                i++;
            }
        }
        
        return output;
    }

    struct CompileResult {
        bool success;
        std::string error_message;
        sol::protected_function function;
        
        CompileResult(bool s, const std::string& err, sol::protected_function func = {}) 
            : success(s), error_message(err), function(func) {}
    };
    
    static CompileResult lua_compile(const std::string& lua_code, const std::string& lua_name = "") {
        if (lua_code.empty()) {
            report_error("Empty Lua code provided");
            return CompileResult(false, "Empty Lua code provided");
        }

        try {
            // Preprocess C-style operators HERE
            std::string processed_code = preprocess_lua_operators(lua_code);
            
            // Compile the preprocessed Lua code
            sol::load_result load_result = g_lua_state->load(processed_code);
            
            if (!load_result.valid()) {
                sol::error err = load_result;
                report_error("Syntax error: " + std::string(err.what()));
                return CompileResult(false, "Syntax error: " + std::string(err.what()));
            }
            
            // Get the compiled function
            sol::protected_function compiled_func = load_result;
            
            // Register in Lua global namespace
            if (!lua_name.empty()) {
                (*g_lua_state)[lua_name] = compiled_func;
            }
            
            return CompileResult(true, "Success", compiled_func);
            
        } catch (const sol::error& e) {
            report_error("Compilation failed: " + std::string(e.what()));
            return CompileResult(false, "Compilation failed: " + std::string(e.what()));
        } catch (const std::exception& e) {
            report_error("Unexpected error: " + std::string(e.what()));
            return CompileResult(false, "Unexpected error: " + std::string(e.what()));
        }
    }
};

// Userdata wrapper for game_value to preserve native Arma data types within Lua
struct GameValueWrapper {
    game_value value;
    static constexpr const char* TYPE_IDENTIFIER = "GameValueWrapper";
    GameValueWrapper() = default;
    GameValueWrapper(const game_value& v) : value(v) {}
    GameValueWrapper(game_value&& v) : value(std::move(v)) {}
    
    std::string to_string() const {
        if (value.is_nil()) return "nil";
        
        switch (value.type_enum()) {
            case game_data_type::OBJECT: return "[OBJECT]";
            case game_data_type::GROUP: return "[GROUP]";
            case game_data_type::NAMESPACE: return "[NAMESPACE]";
            case game_data_type::CONFIG: return "[CONFIG]";
            case game_data_type::CONTROL: return "[CONTROL]";
            case game_data_type::DISPLAY: return "[DISPLAY]";
            case game_data_type::LOCATION: return "[LOCATION]";
            case game_data_type::SCRIPT: return "[SCRIPT]";
            case game_data_type::SIDE: return "[SIDE]";
            case game_data_type::TEXT: return "[TEXT]";
            case game_data_type::TEAM_MEMBER: return "[TEAM_MEMBER]";
            case game_data_type::CODE: return "[CODE]";
            case game_data_type::TASK: return "[TASK]";
            case game_data_type::DIARY_RECORD: return "[DIARY_RECORD]";
            case game_data_type::NetObject: return "[NETOBJECT]";
            case game_data_type::SUBGROUP: return "[SUBGROUP]";
            case game_data_type::TARGET: return "[TARGET]";
            case game_data_type::HASHMAP: return "[HASHMAP]";
            default: return value.data ? static_cast<std::string>(value.data->to_string()) : "nil";
        }
    }
    
    bool equals(const GameValueWrapper& other) const {
        return value == other.value;
    }

    // Method to get the type name
    std::string type_name() const {
        switch (value.type_enum()) {
            case game_data_type::NOTHING: return "NOTHING";
            case game_data_type::ANY: return "ANY";
            case game_data_type::SCALAR: return "SCALAR";
            case game_data_type::BOOL: return "BOOL";
            case game_data_type::ARRAY: return "ARRAY";
            case game_data_type::STRING: return "STRING";
            case game_data_type::OBJECT: return "OBJECT";
            case game_data_type::GROUP: return "GROUP";
            case game_data_type::NAMESPACE: return "NAMESPACE";
            case game_data_type::CONFIG: return "CONFIG";
            case game_data_type::CONTROL: return "CONTROL";
            case game_data_type::DISPLAY: return "DISPLAY";
            case game_data_type::LOCATION: return "LOCATION";
            case game_data_type::SCRIPT: return "SCRIPT";
            case game_data_type::SIDE: return "SIDE";
            case game_data_type::TEXT: return "TEXT";
            case game_data_type::TEAM_MEMBER: return "TEAM_MEMBER";
            case game_data_type::CODE: return "CODE";
            case game_data_type::TASK: return "TASK";
            case game_data_type::DIARY_RECORD: return "DIARY_RECORD";
            case game_data_type::NetObject: return "NETOBJECT";
            case game_data_type::SUBGROUP: return "SUBGROUP";
            case game_data_type::TARGET: return "TARGET";
            case game_data_type::HASHMAP: return "HASHMAP";
            default: return "UNKNOWN";
        }
    }
    
    // Identification method
    bool is_game_value() const { return true; }
};

// Convert game_value to Lua object
static sol::object convert_game_value_to_lua(const game_value& value) {
    sol::state& lua = *g_lua_state;
    lua_State* L = lua.lua_state();
    
    switch (value.type_enum()) {
        case game_data_type::BOOL:
            lua_pushboolean(L, static_cast<bool>(value));
            return sol::stack::pop<sol::object>(L);

        case game_data_type::SCALAR:
            lua_pushnumber(L, static_cast<float>(value));
            return sol::stack::pop<sol::object>(L);
            
        case game_data_type::STRING: {
            std::string str = static_cast<std::string>(value);
            lua_pushlstring(L, str.c_str(), str.length());
            return sol::stack::pop<sol::object>(L);
        }
            
        case game_data_type::ARRAY: {
            auto& array = value.to_array();
            
            if (array.empty()) {
                return sol::make_object(lua, lua.create_table());
            }
            
            size_t arr_size = array.size();
            
            // Single-pass homogeneous detection + conversion for common types
            game_data_type first_type = array[0].type_enum();
            
            // Fast path for homogeneous primitives - single pass
            switch (first_type) {
                case game_data_type::SCALAR: {
                    // Check homogeneity while converting
                    sol::table lua_table = lua.create_table(arr_size, 0);
                    lua_table.raw_set(1, static_cast<float>(array[0]));
                    
                    for (size_t i = 1; i < arr_size; ++i) {
                        if (array[i].type_enum() != game_data_type::SCALAR) goto heterogeneous_array;
                        lua_table.raw_set(i + 1, static_cast<float>(array[i]));
                    }

                    return sol::make_object(lua, lua_table);
                }
                
                case game_data_type::BOOL: {
                    sol::table lua_table = lua.create_table(arr_size, 0);
                    lua_table.raw_set(1, static_cast<bool>(array[0]));
                    
                    for (size_t i = 1; i < arr_size; ++i) {
                        if (array[i].type_enum() != game_data_type::BOOL) goto heterogeneous_array;
                        lua_table.raw_set(i + 1, static_cast<bool>(array[i]));
                    }

                    return sol::make_object(lua, lua_table);
                }
                
                case game_data_type::STRING: {
                    sol::table lua_table = lua.create_table(arr_size, 0);
                    lua_table.raw_set(1, static_cast<std::string>(array[0]));
                    
                    for (size_t i = 1; i < arr_size; ++i) {
                        if (array[i].type_enum() != game_data_type::STRING) goto heterogeneous_array;
                        lua_table.raw_set(i + 1, static_cast<std::string>(array[i]));
                    }

                    return sol::make_object(lua, lua_table);
                }
                
                case game_data_type::OBJECT:
                case game_data_type::GROUP:
                case game_data_type::SIDE: {
                    sol::table lua_table = lua.create_table(arr_size, 0);
                    lua_table.raw_set(1, GameValueWrapper(array[0]));
                    
                    for (size_t i = 1; i < arr_size; ++i) {
                        if (array[i].type_enum() != first_type) goto heterogeneous_array;
                        lua_table.raw_set(i + 1, GameValueWrapper(array[i]));
                    }

                    return sol::make_object(lua, lua_table);
                }
                
                case game_data_type::NOTHING:
                case game_data_type::ANY: {
                    sol::table lua_table = lua.create_table(arr_size, 0);

                    for (size_t i = 0; i < arr_size; ++i) {
                        if (i > 0 && array[i].type_enum() != first_type) goto heterogeneous_array;
                        lua_table.raw_set(i + 1, sol::nil);
                    }

                    return sol::make_object(lua, lua_table);
                }
                
                case game_data_type::ARRAY: {
                    // Check for uniform nested array size (position arrays)
                    size_t nested_size = array[0].size();
                    
                    if (nested_size == 2 || nested_size == 3) {
                        // Pre-check first element for all scalars
                        auto& first_sub = array[0].to_array();
                        bool all_numbers = true;
                        
                        for (size_t j = 0; j < nested_size; ++j) {
                            if (first_sub[j].type_enum() != game_data_type::SCALAR) {
                                all_numbers = false;
                                break;
                            }
                        }
                        
                        if (all_numbers) {
                            // Single-pass: check + convert
                            sol::table lua_table = lua.create_table(arr_size, 0);
                            
                            for (size_t i = 0; i < arr_size; ++i) {
                                if (array[i].type_enum() != game_data_type::ARRAY) goto heterogeneous_array;
                                auto& sub = array[i].to_array();
                                if (sub.size() != nested_size) goto heterogeneous_array;
                                sol::table pos = lua.create_table(nested_size, 0);

                                for (size_t j = 0; j < nested_size; ++j) {
                                    if (sub[j].type_enum() != game_data_type::SCALAR) goto heterogeneous_array;
                                    pos.raw_set(j + 1, static_cast<float>(sub[j]));
                                }

                                lua_table.raw_set(i + 1, pos);
                            }
                            return sol::make_object(lua, lua_table);
                        }
                    }
                    // Fall through to heterogeneous
                    break;
                }
                
                default:
                    // Fall through to heterogeneous
                    break;
            }
            
        heterogeneous_array:
            // Heterogeneous array - recursive conversion with pre-allocated table
            sol::table lua_table = lua.create_table(arr_size, 0);

            for (size_t i = 0; i < arr_size; ++i) {
                lua_table.raw_set(i + 1, convert_game_value_to_lua(array[i]));
            }

            return sol::make_object(lua, lua_table);
        }

        case game_data_type::HASHMAP: {
            auto& hashmap = value.to_hashmap();
            size_t count = hashmap.count();
            
            if (count == 0) {
                return sol::make_object(lua, lua.create_table(0, 0));
            }
            
            // Try optimized homogeneous conversion, fall back to full conversion
            auto it = hashmap.begin();
            game_data_type value_type = it->value.type_enum();
            bool is_homogeneous = true;
            
            // Check if all values are same type
            auto check_it = it;

            for (; check_it != hashmap.end(); ++check_it) {
                if (check_it->value.type_enum() != value_type) {
                    is_homogeneous = false;
                    break;
                }
            }
            
            // Fast path for homogeneous SCALAR
            if (is_homogeneous && value_type == game_data_type::SCALAR) {
                sol::table lua_table = lua.create_table(0, count);

                for (auto& pair : hashmap) {
                    sol::object lua_key = convert_game_value_to_lua(pair.key);
                    lua_table.raw_set(lua_key, static_cast<float>(pair.value));
                }

                return sol::make_object(lua, lua_table);
            }
            
            // Fast path for homogeneous STRING
            if (is_homogeneous && value_type == game_data_type::STRING) {
                sol::table lua_table = lua.create_table(0, count);

                for (auto& pair : hashmap) {
                    sol::object lua_key = convert_game_value_to_lua(pair.key);
                    lua_table.raw_set(lua_key, static_cast<std::string>(pair.value));
                }

                return sol::make_object(lua, lua_table);
            }
            
            // Heterogeneous or other types - full conversion
            sol::table lua_table = lua.create_table(0, count);

            for (auto& pair : hashmap) {
                lua_table.raw_set(
                    convert_game_value_to_lua(pair.key),
                    convert_game_value_to_lua(pair.value)
                );
            }

            return sol::make_object(lua, lua_table);
        }

        case game_data_type::NOTHING:
        case game_data_type::ANY:
            return sol::make_object(lua, sol::nil);
            
        // Native Arma types - wrap in userdata
        case game_data_type::OBJECT:
        case game_data_type::GROUP:
        case game_data_type::NAMESPACE:
        case game_data_type::CONFIG:
        case game_data_type::CONTROL:
        case game_data_type::DISPLAY:
        case game_data_type::LOCATION:
        case game_data_type::SCRIPT:
        case game_data_type::SIDE:
        case game_data_type::TEXT:
        case game_data_type::TEAM_MEMBER:
        case game_data_type::CODE:
        case game_data_type::TASK:
        case game_data_type::DIARY_RECORD:
        case game_data_type::NetObject:
        case game_data_type::SUBGROUP:
        case game_data_type::TARGET:
            return sol::make_object(lua, GameValueWrapper(value));
            
        default:
            // For unhandled types, wrap as userdata
            return sol::make_object(lua, GameValueWrapper(value));
    }
}

// Convert Lua object to game_value
static game_value convert_lua_to_game_value(const sol::object& obj) {
    sol::type type = obj.get_type();

    if (type == sol::type::number) {
        return game_value(obj.as<float>());
    }
    
    if (type == sol::type::boolean) {
        return game_value(obj.as<bool>());
    }

    if (type == sol::type::string) {
        return game_value(obj.as<std::string>());
    }

    if (type == sol::type::nil) {
        return game_value();
    }

    if (obj.get_type() == sol::type::table) {
        sol::table tbl = obj;

        // Check for metatable with __toSQF metamethod
        sol::optional<sol::table> metatable = tbl[sol::metatable_key];
        
        if (metatable) {
            sol::optional<sol::function> to_sqf = (*metatable)["__toSQF"];

            if (to_sqf) {
                sol::object result = (*to_sqf)(tbl);
                return convert_lua_to_game_value(result);
            }
        }
        
        // Single-pass table analysis
        bool is_array = true;
        bool has_non_integer_keys = false;
        size_t max_index = 0;
        size_t non_nil_count = 0;
        
        // Detect type while counting
        for (auto& pair : tbl) {
            non_nil_count++;
            sol::object key = pair.first;
            sol::type key_type = key.get_type();
            
            if (key_type == sol::type::number) {
                double key_num = key.as<double>();
                
                if (key_num > 0 && key_num == std::floor(key_num)) {
                    size_t idx = static_cast<size_t>(key_num);
                    if (idx > max_index) max_index = idx;
                } else {
                    is_array = false;
                    has_non_integer_keys = true;
                    break;
                }
            } else {
                is_array = false;
                has_non_integer_keys = true;
                break;
            }
        }
        
        // Empty table
        if (non_nil_count == 0) {
            return game_value(auto_array<game_value>());
        }
        
        // Dense array fast path
        if (is_array && max_index > 0) {
            float density = static_cast<float>(non_nil_count) / static_cast<float>(max_index);
            
            if (max_index <= 10000 && density >= 0.01f) {
                auto_array<game_value> arr;
                arr.reserve(max_index);
                
                for (size_t i = 1; i <= max_index; i++) {
                    arr.push_back(convert_lua_to_game_value(tbl[i]));
                }

                return game_value(std::move(arr));
            }
            
            // Sparse - treat as hashmap
            is_array = false;
            has_non_integer_keys = true;
        }
        
        // Hashmap conversion
        if (!is_array || has_non_integer_keys) {
            auto_array<game_value> kv_array;
            kv_array.reserve(non_nil_count);
            
            for (auto& pair : tbl) {
                auto_array<game_value> kv_pair;
                kv_pair.reserve(2);
                sol::object key = pair.first;
                sol::type key_type = key.get_type();
                game_value key_value;
                
                switch (key_type) {
                    case sol::type::string:
                        key_value = game_value(key.as<std::string>());
                        break;
                    case sol::type::number:
                        key_value = game_value(key.as<float>());
                        break;
                    case sol::type::boolean:
                        key_value = game_value(key.as<bool>());
                        break;
                    default:
                        key_value = convert_lua_to_game_value(key);
                        break;
                }
                
                kv_pair.push_back(std::move(key_value));
                kv_pair.push_back(convert_lua_to_game_value(pair.second));
                kv_array.push_back(game_value(std::move(kv_pair)));
            }
            
            if (!kv_array.empty()) {
                return raw_call_sqf_args_native(g_compiled_sqf_create_hash_map_from_array, game_value(std::move(kv_array)));
            } else {
                return raw_call_sqf_native(g_compiled_sqf_create_hash_map);
            }
        }
        
        // Empty array
        return game_value(auto_array<game_value>());
    } else if (obj.get_type() == sol::type::userdata) {
        // Try to extract GameValueWrapper
        sol::optional<GameValueWrapper> wrapper = obj.as<sol::optional<GameValueWrapper>>();

        if (wrapper) {
            return wrapper->value;
        }
    }
    
    return game_value();
}

// C++ implementations of Lua utility functions
namespace LuaFunctions {
    static sol::object sqf_call(sol::object code_or_func, sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            game_value compiled;
            
            // Convert variadic args to game_value
            game_value args_gv;

            if (args.size() == 0) {
                // No arguments
                args_gv = game_value();
            } else if (args.size() == 1) {
                // Single argument - pass directly
                args_gv = convert_lua_to_game_value(args[0]);
            } else {
                // Multiple arguments - wrap in array
                auto_array<game_value> args_array;
                args_array.reserve(args.size());

                for (const auto& arg : args) {
                    args_array.push_back(convert_lua_to_game_value(arg));
                }
                
                args_gv = game_value(std::move(args_array));
            }
            
            // Check if first argument is already compiled code
            if (code_or_func.get_type() == sol::type::userdata) {
                sol::optional<GameValueWrapper> wrapper = code_or_func.as<sol::optional<GameValueWrapper>>();
                
                if (wrapper && wrapper->value.type_enum() == game_data_type::CODE) {
                    compiled = wrapper->value;
                    
                    if (args.size() == 0) {
                        return convert_game_value_to_lua(raw_call_sqf_native(compiled));
                    } else {
                        return convert_game_value_to_lua(raw_call_sqf_args_native(compiled, args_gv));
                    }
                }
            }
            
            // Otherwise, must be string
            if (code_or_func.get_type() != sol::type::string) {
                report_error("Code or function name must be string or CODE type");
                return sol::nil;
            }

            std::string code_or_func_str = code_or_func.as<std::string>();
            
            if (code_or_func_str.find(' ') == std::string::npos && code_or_func_str.find(';') == std::string::npos) {
                // Function call path
                if (args.size() == 0) {
                    // No parameters - compile as "call functionName"
                    std::string cache_key = "call " + code_or_func_str;
                    auto cache_it = g_sqf_function_cache.find(cache_key);
                    
                    if (cache_it != g_sqf_function_cache.end()) {
                        compiled = cache_it->second;
                    } else {
                        compiled = sqf::compile(cache_key);
                        g_sqf_function_cache[cache_key] = compiled;
                    }
                    
                    return convert_game_value_to_lua(raw_call_sqf_native(compiled));
                } else {
                    // With parameters - compile as "_this call functionName"
                    std::string cache_key = "_x call " + code_or_func_str;
                    auto cache_it = g_sqf_function_cache.find(cache_key);
                    
                    if (cache_it != g_sqf_function_cache.end()) {
                        compiled = cache_it->second;
                    } else {
                        compiled = sqf::compile(cache_key);
                        g_sqf_function_cache[cache_key] = compiled;
                    }
                    
                    return convert_game_value_to_lua(raw_call_sqf_args_native(compiled, args_gv));
                }
            } else {
                // Code execution path
                size_t code_hash = std::hash<std::string>{}(code_or_func_str);
                auto cache_it = g_sqf_compiled_cache.find(code_hash);
                
                if (cache_it != g_sqf_compiled_cache.end()) {
                    compiled = cache_it->second;
                } else {
                    compiled = sqf::compile(code_or_func_str);
                    g_sqf_compiled_cache[code_hash] = compiled;
                }
                
                if (args.size() == 0) {
                    return convert_game_value_to_lua(raw_call_sqf_native(compiled));
                } else {
                    return convert_game_value_to_lua(raw_call_sqf_args_native(compiled, args_gv));
                }
            }
            
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("Failed to call SQF: " + std::string(e.what()));
            return sol::nil;
        }
    }

    struct ScheduledTask {
        sol::protected_function callback;
        float execute_time;      // For time-based delays
        int execute_frame;        // For frame-based delays  
        bool use_frames;
        bool repeating;
        float interval;           // For repeating tasks
        int frame_interval;       // For repeating frame tasks
        
        // Timeout fields
        float timeout_time;       // When to stop (for time-based)
        int timeout_frame;        // When to stop (for frame-based)
        bool has_timeout;
        bool prioritize_timeout;  // If true, times out even if last execution matches exact timeout interval
        
        // Track start for timeout calculation
        float start_time;
        int start_frame;
    };
    
    static std::unordered_map<int, ScheduledTask> lua_scheduled_tasks;
    static int next_task_id = 1;
    
    // Schedule a function to run after a delay in seconds
    static int delay(float time_or_frames, sol::protected_function callback) {
        try {
            if (!callback.valid()) {
                report_error("Invalid callback function");
                return -1;
            }
            
            int task_id = next_task_id++;
            ScheduledTask task;
            task.callback = callback;
            task.repeating = false;
            task.has_timeout = false;
            task.prioritize_timeout = false;
            task.timeout_time = 0.0f;
            task.timeout_frame = 0;
            task.start_time = g_mission_time;
            task.start_frame = g_mission_frame;
            
            if (time_or_frames >= 0) {
                // Seconds
                task.execute_time = g_mission_time + time_or_frames;
                task.execute_frame = 0;
                task.use_frames = false;
                task.interval = 0.0f;
                task.frame_interval = 0;
            } else {
                // Frames
                task.execute_time = 0.0f;
                task.execute_frame = g_mission_frame + static_cast<int>(std::abs(time_or_frames));
                task.use_frames = true;
                task.interval = 0.0f;
                task.frame_interval = 0;
            }
            
            lua_scheduled_tasks[task_id] = task;
            return task_id;
        } catch (const std::exception& e) {
            report_error("Failed to create temporal handler: " + std::string(e.what()));
            return -1;
        }
    }

    // Update interval function
    static int interval(float time_or_frames, bool execute_immediately, float timeout, bool prioritize, sol::protected_function callback) {
        try {
            if (!callback.valid()) {
                report_error("Invalid callback function");
                return -1;
            }
            
            int task_id = next_task_id++;
            ScheduledTask task;
            task.callback = callback;
            task.repeating = true;
            task.start_time = g_mission_time;
            task.start_frame = g_mission_frame;
            
            if (time_or_frames > 0) {
                task.execute_time = execute_immediately ? g_mission_time : g_mission_time + time_or_frames;
                task.execute_frame = 0;
                task.use_frames = false;
                task.interval = time_or_frames;
                task.frame_interval = 0;
                
                if (timeout != 0) {
                    task.has_timeout = true;
                    task.timeout_time = g_mission_time + timeout;
                    task.timeout_frame = 0;
                    task.prioritize_timeout = prioritize;
                } else {
                    task.has_timeout = false;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = 0;
                    task.prioritize_timeout = false;
                }
            } else if (time_or_frames < 0) {
                int frames = static_cast<int>(std::abs(time_or_frames));
                task.execute_time = 0.0f;
                task.execute_frame = execute_immediately ? g_mission_frame : g_mission_frame + frames;
                task.use_frames = true;
                task.interval = 0.0f;
                task.frame_interval = frames;
                
                if (timeout != 0) {
                    task.has_timeout = true;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = g_mission_frame + static_cast<int>(std::abs(timeout));
                    task.prioritize_timeout = prioritize;
                } else {
                    task.has_timeout = false;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = 0;
                    task.prioritize_timeout = false;
                }
            } else {
                task.execute_time = 0.0f;
                task.execute_frame = execute_immediately ? g_mission_frame : g_mission_frame + 1;
                task.use_frames = true;
                task.interval = 0.0f;
                task.frame_interval = 1;
                
                if (timeout != 0) {
                    task.has_timeout = true;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = g_mission_frame + static_cast<int>(std::abs(timeout));
                    task.prioritize_timeout = prioritize;
                } else {
                    task.has_timeout = false;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = 0;
                    task.prioritize_timeout = false;
                }
            }
            
            lua_scheduled_tasks[task_id] = task;
            return task_id;
        } catch (const std::exception& e) {
            report_error("Failed to create temporal handler: " + std::string(e.what()));
            return -1;
        }
    }

    // Update cancel_task
    static bool cancel_task(int task_id) {
        try {
            return lua_scheduled_tasks.erase(task_id) > 0;
        } catch (const std::exception& e) {
            report_error("Failed to cancel task: " + std::string(e.what()));
            return false;
        }
    }

    // Optimized update_scheduler - O(n) instead of O(n²)
    static void update_scheduler() {
        std::vector<int> tasks_to_remove;
        
        for (auto& [task_id, task] : lua_scheduled_tasks) {
            // Skip cancelled tasks
            if (!task.callback.valid()) {
                tasks_to_remove.push_back(task_id);
                continue;
            }
            
            // Check timeout first
            bool past_timeout = false;
            bool at_timeout_boundary = false;
            
            if (task.has_timeout && task.repeating) {
                if (task.use_frames) {
                    past_timeout = g_mission_frame > task.timeout_frame;
                    
                    if (!task.prioritize_timeout && g_mission_frame == task.timeout_frame) {
                        int elapsed = g_mission_frame - task.start_frame;
                        at_timeout_boundary = (elapsed % task.frame_interval) == 0;
                    }
                } else {
                    past_timeout = g_mission_time > task.timeout_time;
                    
                    if (!task.prioritize_timeout) {
                        float elapsed = g_mission_time - task.start_time;
                        float epsilon = 0.01f;
                        
                        if (std::abs(elapsed - task.timeout_time + task.start_time) < epsilon) {
                            float intervals = elapsed / task.interval;
                            at_timeout_boundary = std::abs(intervals - std::round(intervals)) < epsilon;
                        }
                    }
                }
                
                if (past_timeout && !at_timeout_boundary) {
                    tasks_to_remove.push_back(task_id);
                    continue;
                }
            }
            
            // Check if should execute
            bool should_execute = false;
            
            if (task.use_frames) {
                should_execute = g_mission_frame >= task.execute_frame;
            } else {
                should_execute = g_mission_time >= task.execute_time;
            }
            
            if (should_execute) {
                // Execute callback
                sol::protected_function_result result = task.callback();
                
                if (!result.valid()) {
                    sol::error err = result;
                    report_error("Scheduled task error: " + std::string(err.what()));
                }
                
                // Check if callback returned true to stop interval
                bool should_cancel = false;

                if (result.valid() && result.return_count() > 0) {
                    sol::object ret_val = result.get<sol::object>();

                    if (ret_val.is<bool>() && ret_val.as<bool>() == true) {
                        should_cancel = true;
                    }
                }
                
                // If this was the timeout boundary execution or callback returned true, remove the task
                if (at_timeout_boundary || should_cancel) {
                    tasks_to_remove.push_back(task_id);
                } else if (task.repeating) {
                    // Reschedule
                    if (task.use_frames) {
                        task.execute_frame = g_mission_frame + task.frame_interval;
                    } else {
                        task.execute_time = g_mission_time + task.interval;
                    }
                } else {
                    // Remove completed one-time task
                    tasks_to_remove.push_back(task_id);
                }
            }
        }
        
        // Remove all marked tasks in one pass - O(n) where n is number of tasks to remove
        for (int task_id : tasks_to_remove) {
            lua_scheduled_tasks.erase(task_id);
        }
    }

    static std::unordered_map<std::string, std::unordered_map<int, sol::protected_function>> lua_event_handlers;
    static int next_event_handler_id = 1;

    static int add_event_handler(const std::string& event_name, sol::protected_function handler) {
        try {
            if (!handler.valid()) {
                report_error("Invalid handler function");
                return -1;
            }
            
            int handler_id = next_event_handler_id++;
            lua_event_handlers[event_name][handler_id] = handler;
            return handler_id;
        } catch (const std::exception& e) {
            report_error("Failed to add event handler: " + std::string(e.what()));
            return -1;
        }
    }
    
    static bool remove_event_handler(const std::string& event_name, int handler_id) {
        try {
            auto it = lua_event_handlers.find(event_name);
            
            if (it != lua_event_handlers.end()) {
                return it->second.erase(handler_id) > 0;
            }

            return false;
        } catch (const std::exception& e) {
            report_error("Failed to remove event handler: " + std::string(e.what()));
            return false;
        }
    }
    
    // CBA-enabled trigger that uses the helper
    static sol::object trigger_event(const std::string& event_name, sol::object target, sol::object jip, sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            
            if ((target.get_type() == sol::type::nil || (target.is<bool>() && target.as<bool>())) && 
                (jip.get_type() == sol::type::nil || (jip.is<bool>() && !jip.as<bool>()))) {
                // Local emission
                auto it = lua_event_handlers.find(event_name);
                
                if (it == lua_event_handlers.end() || it->second.empty()) {
                    return sol::make_object(*g_lua_state, 0);
                }
                
                // Collect handler IDs to execute (prevents iterator invalidation if handlers modify the map)
                std::vector<int> handlers_to_execute;
                handlers_to_execute.reserve(it->second.size());
                
                for (const auto& [handler_id, handler] : it->second) {
                    if (handler.valid()) {
                        handlers_to_execute.push_back(handler_id);
                    }
                }
                
                // Execute handlers by ID
                sol::object last_result = sol::nil;
                sol::state& lua = *g_lua_state;
                lua_State* L = lua.lua_state();
                
                for (int handler_id : handlers_to_execute) {
                    // Re-lookup the event name in case the map was modified
                    auto event_it = lua_event_handlers.find(event_name);

                    if (event_it == lua_event_handlers.end()) {
                        break; // Event was completely removed
                    }
                    
                    // Re-lookup the handler in case it was removed
                    auto handler_it = event_it->second.find(handler_id);
                    
                    if (handler_it == event_it->second.end() || !handler_it->second.valid()) {
                        continue; // Handler was removed or invalidated
                    }
                    
                    sol::protected_function& handler = handler_it->second;
                    handler.push(L);
                    
                    // Push variadic args directly
                    for (const auto& arg : args) {
                        arg.push(L);
                    }
                    
                    if (lua_pcall(L, args.size(), 1, 0) == 0) {
                        last_result = sol::stack::pop<sol::object>(L);
                    } else {
                        std::string err = lua_tostring(L, -1);
                        report_error("Event handler error: " + err);
                        lua_pop(L, 1);
                    }
                }
                
                return last_result;
            }
            
            // CBA emission
            game_value target_gv = convert_lua_to_game_value(target);
            game_value jip_gv = convert_lua_to_game_value(jip);
            auto_array<game_value> args_array;
            args_array.reserve(args.size());
            
            for (const auto& arg : args) {  // Use iterator, not index
                args_array.push_back(convert_lua_to_game_value(arg));
            }
            
            auto_array<game_value> cba_event_data;
            cba_event_data.push_back(game_value(event_name));
            cba_event_data.push_back(game_value(std::move(args_array)));
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_luaEventTrigger"));
            cba_params.push_back(game_value(std::move(cba_event_data)));
            cba_params.push_back(target_gv);
            cba_params.push_back(jip_gv);
            return convert_game_value_to_lua(raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params))));
        } catch (const std::exception& e) {
            report_error("Failed to trigger event: " + std::string(e.what()));
            return sol::nil;
        }
    }
        
    // Clear handlers
    static void clear_handlers(const std::string& event_name) {
        try {
            if (event_name.empty()) {
                report_error("Handler name cannot be empty");
            } else {
                lua_event_handlers.erase(event_name);
            }
        } catch (const std::exception& e) {
            report_error("Failed to clear handlers: " + std::string(e.what()));
        }
    }
    
    // Get handler count for debugging
    static int get_handler_count(const std::string& event_name) {
        try {
            auto it = lua_event_handlers.find(event_name);
            return it != lua_event_handlers.end() ? static_cast<int>(it->second.size()) : 0;
        } catch (const std::exception& e) {
            report_error("Failed to get handler count: " + std::string(e.what()));
            return 0;
        }
    }

    static sol::object add_game_event_handler(sol::object type, sqf_string_const_ref event, sol::protected_function handler) {
        try {
            LuaStackGuard guard(*g_lua_state);
            
            if (!handler.valid()) {
                report_error("Invalid handler function");
                return sol::nil;
            }
            
            // Generate unique ID for this handler
            std::string handler_uid = UIDGenerator::generate();
            
            // Register the Lua function as an event handler using the UID as event name
            int handler_id = add_event_handler(handler_uid, handler);
            
            // Call SQF to register the game event handler
            // KH_fnc_addEventHandler will trigger the Lua event when the game event fires
            auto_array<game_value> sqf_params;
            sqf_params.push_back(convert_lua_to_game_value(type));
            sqf_params.push_back(game_value(static_cast<std::string>(event)));
            sqf_params.push_back(game_value(handler_uid));
            sqf_params.push_back(game_value(g_compiled_sqf_game_event_handler_lua_bridge));
            game_value sqf_result = raw_call_sqf_args_native(g_compiled_sqf_add_game_event_handler, game_value(std::move(sqf_params)));
            sol::table result = g_lua_state->create_table();
            result[1] = handler_uid;
            result[2] = handler_id;
            result[3] = convert_game_value_to_lua(sqf_result);
            return sol::make_object(*g_lua_state, result);
        } catch (const std::exception& e) {
            report_error("Failed to add game event handler: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static bool remove_game_event_handler(sol::table handler_info) {
        try {
            LuaStackGuard guard(*g_lua_state);

            // Extract uid, handler_id, and sqf_id from the handler info table [uid, handler_id, sqf_id]
            sol::object uid_obj = handler_info[1];
            sol::object handler_id_obj = handler_info[2];
            sol::object sqf_id_obj = handler_info[3];
            
            if (!uid_obj.valid() || uid_obj.get_type() != sol::type::string) {
                report_error("Invalid handler info: missing or invalid uid");
                return false;
            }
            
            std::string uid = uid_obj.as<std::string>();
            
            // Remove the Lua event handler
            if (handler_id_obj.valid() && handler_id_obj.get_type() == sol::type::number) {
                int handler_id = handler_id_obj.as<int>();
                remove_event_handler(uid, handler_id);
            }
            
            // Call SQF to remove the game event handler
            if (sqf_id_obj.valid() && sqf_id_obj.get_type() != sol::type::nil) {
                game_value id_gv = convert_lua_to_game_value(sqf_id_obj);            
                raw_call_sqf_args_native(g_compiled_sqf_remove_game_event_handler, id_gv);
            }
            
            return true;
        } catch (const std::exception& e) {
            report_error("Failed to remove game event handler: " + std::string(e.what()));
            return false;
        }
    }

    static sol::object execute_lua(sol::object target, sol::object environment, sol::object special, sol::object func, sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            sol::state& lua = *g_lua_state;
            
            // Check if we need to schedule execution instead of executing immediately
            if ((target.get_type() == sol::type::nil || (target.is<bool>() && target.as<bool>())) && 
                (special.get_type() == sol::type::nil || (special.is<bool>() && !special.as<bool>()))) {
                if (environment.get_type() == sol::type::number) {
                    // Number: schedule as interval with default settings
                    float interval_time = environment.as<float>();
                    
                    // Create a Lua callback function
                    sol::function callback = lua["util"]["execute"];
                    
                    // Convert args to Lua table
                    sol::table args_table = lua.create_table();
                    int idx = 1;

                    for (const auto& arg : args) {
                        args_table[idx++] = arg;
                    }
                    
                    // Create wrapper function that unpacks args
                    std::string wrapper_code = R"(
                        return function()
                            local target, special, func, args = ...
                            return util.execute(target, nil, special, func, table.unpack(args))
                        end
                    )";
                    
                    sol::protected_function wrapper_factory = lua.script(wrapper_code);
                    sol::protected_function wrapper = wrapper_factory(target, special, func, args_table);
                    
                    // Schedule interval: timeout=0, execute_immediately=true, prioritizeTimeout=false
                    int task_id = interval(interval_time, true, 0.0f, false, wrapper);
                    return sol::make_object(lua, task_id);
                } else if (environment.get_type() == sol::type::string) {
                    // String: try to parse as number for delay
                    std::string env_str = environment.as<std::string>();
                    float delay_time;
                    
                    try {
                        delay_time = std::stof(env_str);
                    } catch (...) {
                        report_error("Environment string must be a valid number for delay");
                        return sol::nil;
                    }
                    
                    // Convert args to Lua table
                    sol::table args_table = lua.create_table();
                    int idx = 1;
                    
                    for (const auto& arg : args) {
                        args_table[idx++] = arg;
                    }
                    
                    // Create wrapper function that unpacks args
                    std::string wrapper_code = R"(
                        return function()
                            local target, special, func, args = ...
                            return util.execute(target, nil, special, func, table.unpack(args))
                        end
                    )";
                    
                    sol::protected_function wrapper_factory = lua.script(wrapper_code);
                    sol::protected_function wrapper = wrapper_factory(target, special, func, args_table);
                    
                    // Schedule delay
                    int task_id = delay(delay_time, wrapper);
                    return sol::make_object(lua, task_id);
                } else if (environment.get_type() == sol::type::nil) {
                    if (func.get_type() == sol::type::string) {
                        // String function name - look it up in globals
                        std::string func_name = func.as<std::string>();
                        sol::object func_obj = lua[func_name];
                        
                        if (func_obj.get_type() != sol::type::function) {
                            report_error("Function '" + func_name + "' not found in Lua globals");
                            return sol::nil;
                        }
                        
                        sol::protected_function pfunc = func_obj;
                        std::vector<sol::object> args_vec;

                        for (const auto& arg : args) {
                            args_vec.push_back(arg);
                        }
                        
                        if (args_vec.empty()) {
                            return pfunc();
                        } else {
                            return pfunc(sol::as_args(args_vec));
                        }
                    } else if (func.get_type() == sol::type::function) {
                        // Direct function - call it
                        sol::protected_function pfunc = func;
                        std::vector<sol::object> args_vec;
                        
                        for (const auto& arg : args) {
                            args_vec.push_back(arg);
                        }
                        
                        if (args_vec.empty()) {
                            return pfunc();
                        } else {
                            return pfunc(sol::as_args(args_vec));
                        }
                    } else {
                        report_error("Function must be a string or Lua function for local execution");
                        return sol::nil;
                    }
                }
            }
            
            // environment is nil or other type
            game_value target_gv = convert_lua_to_game_value(target);
            game_value environment_gv = convert_lua_to_game_value(environment);
            game_value special_gv = convert_lua_to_game_value(special);
            
            // Convert function to string
            std::string function_str;

            if (func.get_type() == sol::type::string) {
                function_str = func.as<std::string>();
            } else if (func.get_type() == sol::type::function) {
                bool found = false;
                sol::table globals = lua.globals();

                for (const auto& pair : globals) {
                    if (pair.second.get_type() == sol::type::function) {
                        sol::function global_func = pair.second;

                        if (global_func == func) {
                            if (pair.first.get_type() == sol::type::string) {
                                function_str = pair.first.as<std::string>();
                                found = true;
                                break;
                            }
                        }
                    }
                }
                
                if (!found) {
                    sol::protected_function string_dump = lua["string"]["dump"];
                    auto dump_result = string_dump(func);
                    
                    if (dump_result.valid()) {
                        std::string bytecode = dump_result.get<std::string>();
                        std::stringstream ss;
                        ss << "return loadstring(\"";
                        
                        for (unsigned char c : bytecode) {
                            ss << "\\" << std::setfill('0') << std::setw(3) << (int)c;
                        }
                        
                        ss << "\")()";
                        function_str = ss.str();
                    } else {
                        report_error("Cannot serialize anonymous function for remote execution");
                        return sol::nil;
                    }
                }
            } else {
                report_error("Function must be a string or Lua function");
                return sol::nil;
            }
                
            auto_array<game_value> args_array;

            for (const auto& arg : args) {
                args_array.push_back(convert_lua_to_game_value(arg));
            }

            game_value args_gv = game_value(std::move(args_array));
            auto_array<game_value> sqf_params;
            sqf_params.push_back(args_gv);
            sqf_params.push_back(game_value(function_str));
            sqf_params.push_back(target_gv);
            sqf_params.push_back(environment_gv);
            sqf_params.push_back(special_gv);       
            return convert_game_value_to_lua(raw_call_sqf_args_native(g_compiled_sqf_execute_lua, game_value(std::move(sqf_params))));
        } catch (const std::exception& e) {
            report_error("Failed to execute: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static sol::object trigger_cba_event(const std::string& event_name, sol::object target, sol::object jip, sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);

            // Convert variadic args to game_value
            game_value args_gv;
            
            if (args.size() == 0) {
                // No arguments
                args_gv = game_value();
            } else if (args.size() == 1) {
                // Single argument - pass directly without array wrapper
                args_gv = convert_lua_to_game_value(args[0]);
            } else {
                // Multiple arguments - wrap in array
                auto_array<game_value> args_array;
                args_array.reserve(args.size());
                
                for (const auto& arg : args) {
                    args_array.push_back(convert_lua_to_game_value(arg));
                }
                
                args_gv = game_value(std::move(args_array));
            }
            
            game_value target_gv = convert_lua_to_game_value(target);
            game_value jip_gv = convert_lua_to_game_value(jip);
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value(event_name));
            cba_params.push_back(args_gv);
            cba_params.push_back(target_gv);
            cba_params.push_back(jip_gv);
            return convert_game_value_to_lua(raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params))));
        } catch (const std::exception& e) {
            report_error("Failed to trigger CBA event: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static sol::object emit_variable(const std::string& var_name, sol::optional<sol::object> value_opt,
                                     sol::optional<sol::object> target_opt, sol::optional<sol::object> jip_opt) {
        try {
            LuaStackGuard guard(*g_lua_state);
            sol::state& lua = *g_lua_state;
            game_value emit_value;

            if (value_opt) {
                emit_value = convert_lua_to_game_value(*value_opt);
            } else {
                // Get value from Lua global variable
                sol::object lua_var = lua[var_name];

                if (lua_var.valid()) {
                    emit_value = convert_lua_to_game_value(lua_var);
                } else {
                    report_error("Lua global variable '" + var_name + "' not found or is nil");
                    return sol::nil;
                }
            }

            game_value target = target_opt ? convert_lua_to_game_value(*target_opt) : game_value("GLOBAL");
            game_value jip = jip_opt ? convert_lua_to_game_value(*jip_opt) : game_value();
            auto_array<game_value> emission_data;
            emission_data.push_back(game_value(var_name));
            emission_data.push_back(emit_value);
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_luaVariableEmission"));
            cba_params.push_back(game_value(std::move(emission_data)));
            cba_params.push_back(target);
            cba_params.push_back(jip);
            return convert_game_value_to_lua(raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params))));
        } catch (const std::exception& e) {
            report_error("Failed to emit variable: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static sol::object remove_handler(sol::table handler_info) {
        try {
            LuaStackGuard guard(*g_lua_state);
            
            // Convert table to game_value array
            auto_array<game_value> info_array;

            for (size_t i = 1; i <= handler_info.size(); i++) {
                info_array.push_back(convert_lua_to_game_value(handler_info[i]));
            }
            
            // Nest in outer array for _this call since remover accepts array in _this
            auto_array<game_value> nested;
            nested.push_back(game_value(std::move(info_array)));
            raw_call_sqf_args_native(g_compiled_sqf_remove_handler, game_value(std::move(nested)));
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("Failed to remove handler: " + std::string(e.what()));
            return sol::nil;
        }
    }
    
    // Get formatted date/time string
    static std::string get_date_time() {
        try {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            char buffer[100];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
            return std::string(buffer);
        } catch (const std::exception& e) {
            report_error("Failed to get time: " + std::string(e.what()));
            return "";
        }
    }

    // Get high-resolution timestamp in seconds (for delta calculations)
    static double get_time_epoch() {
        try {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            return microseconds / 1000000.0;  // Convert to seconds
        } catch (const std::exception& e) {
            report_error("Failed to get time: " + std::string(e.what()));
            return 0.0;
        }
    }

    // Get system time in seconds with millisecond precision
    static double get_time_boot() {
        try {
            LARGE_INTEGER frequency, counter;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&counter);
            return (counter.QuadPart * 1000.0) / frequency.QuadPart;
        } catch (const std::exception& e) {
            report_error("Failed to get time: " + std::string(e.what()));
            return 0.0;
        }
    }
    
    // Profile code execution speed
    static sol::object profile_code(sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            sol::state& lua = *g_lua_state;
            
            if (args.size() < 2) {
                report_error("Not enough arguments: need at least count and function");
                return sol::nil;
            }
            
            // First argument: iteration count
            sol::object count_obj = args[0];
            int count = count_obj.as<int>();
            
            if (count < 1) {
                report_error("Execution count must be at least 1");
                return sol::nil;
            }
            
            // Second argument: function
            sol::object code_obj = args[1];
            sol::protected_function compiled;
            
            if (code_obj.get_type() == sol::type::string) {
                // String - treat as function name
                std::string func_name = code_obj.as<std::string>();
                sol::object func = lua[func_name];

                if (func.get_type() != sol::type::function) {
                    report_error("Function '" + func_name + "' not found or is not a function");
                    return sol::nil;
                }
                
                compiled = func;
            } else if (code_obj.get_type() == sol::type::function) {
                // Direct function object
                compiled = code_obj;
            } else {
                report_error("Second argument must be a string (function name) or function object");
                return sol::nil;
            }
            
            // Remaining arguments: function parameters (varargs)
            std::vector<sol::object> func_args;
            
            for (size_t i = 2; i < args.size(); i++) {
                func_args.push_back(args[i]);
            }
            
            // Warm up if count > 100
            if (count > 100) {
                for (int i = 0; i < 100; i++) {
                    if (func_args.empty()) {
                        compiled();
                    } else {
                        compiled(sol::as_args(func_args));
                    }
                }
            }
            
            // Get timer function
            sol::function get_time = lua["time"]["getBoot"];

            if (!get_time.valid()) {
                report_error("High precision timer not available");
                return sol::nil;
            }
            
            // Profile execution
            double start_time = get_time();
            
            if (func_args.empty()) {
                // Loop for no arguments
                for (int i = 0; i < count; i++) {
                    compiled();
                }
            } else {
                // Loop for with arguments
                for (int i = 0; i < count; i++) {
                    compiled(sol::as_args(func_args));
                }
            }
            
            double end_time = get_time();
            double total_time = end_time - start_time;
            double average_time = total_time / count;
            char buffer[256];

            snprintf(buffer, sizeof(buffer), "Count %d\nTotal: %.6f\nAverage: %.6f", 
                    count, total_time, average_time);
            
            return sol::make_object(lua, std::string(buffer));
        } catch (const std::exception& e) {
            report_error("Failed to profile code: " + std::string(e.what()));
            return sol::nil;
        }
    }

    // Get specific data type
    static std::string get_data_type(sol::object input) {
        try {
            LuaStackGuard guard(*g_lua_state);

            if (input.get_type() == sol::type::nil) {
                return "NOTHING";
            }
            
            sol::type lua_type = input.get_type();
            
            switch (lua_type) {
                case sol::type::boolean:
                    return "BOOL";
                case sol::type::number:
                    return "SCALAR";
                case sol::type::string:
                    return "STRING";
                case sol::type::table:
                    return "ARRAY";
                case sol::type::userdata: {
                    // Check if it's a GameValueWrapper
                    sol::optional<GameValueWrapper> wrapper = input.as<sol::optional<GameValueWrapper>>();
                    
                    if (wrapper) {
                        return wrapper->type_name();
                    }

                    return "USERDATA";
                }
                default:
                    return "UNKNOWN";
            }
        } catch (const std::exception& e) {
            report_error("Failed to get data type: " + std::string(e.what()));
            return "";
        }
    }

    // Execute in SQF-first namespace
    static sol::object with_sqf(sol::protected_function func, sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            sol::state& lua = *g_lua_state;
            sol::table env = lua.create_table();
            sol::table meta = lua.create_table();
            
            // Store original globals for lua.* access
            sol::table lua_namespace = lua.create_table();

            lua_namespace[sol::metatable_key] = lua.create_table_with(
                "__index", lua.globals(),
                "__newindex", lua.globals()
            );
            
            // __index: Read from sqf commands first, then sqf variables, then error
            meta["__index"] = [&lua, lua_namespace](sol::table t, sol::object key) -> sol::object {
                try {
                    LuaStackGuard guard(*g_lua_state);

                    if (key.get_type() != sol::type::string) {
                        return sol::nil;
                    }
                    
                    std::string key_str = key.as<std::string>();
                    
                    // Special case: "lua" gives access to Lua namespace
                    if (key_str == "lua") {
                        return sol::make_object(lua, lua_namespace);
                    }
                    
                    // First check sqf table for commands
                    sol::table sqf_table = lua["sqf"];
                    sol::object sqf_result = sqf_table[key];

                    if (sqf_result.valid() && sqf_result.get_type() != sol::type::nil) {
                        return sqf_result;
                    }

                    // Then try to get from SQF variables
                    game_value sqfVar = sqf::get_variable(sqf::current_namespace(), key_str);

                    if (!sqfVar.is_nil()) {
                        return convert_game_value_to_lua(sqfVar);
                    }
                    
                    // Not found - return nil
                    return sol::nil;
                } catch (const std::exception& e) {
                    report_error("SQF variable handler error: " + std::string(e.what()));
                    return sol::nil;
                }
            };
            
            // __newindex: Write to SQF variables by default
            meta["__newindex"] = [&lua](sol::table t, sol::object key, sol::object value) {
                try {
                    if (key.get_type() != sol::type::string) return;
                    std::string var_name = key.as<std::string>();
                    
                    // Don't allow overwriting "lua" keyword
                    if (var_name == "lua") {
                        report_error("Cannot overwrite 'lua' keyword in withSqf context");
                        return;
                    }
                    
                    // Set as SQF variable
                    sqf::set_variable(sqf::current_namespace(), var_name, convert_lua_to_game_value(value));
                } catch (const std::exception& e) {
                    report_error("Failed to set SQF variable: " + std::string(e.what()));
                }
            };
            
            env[sol::metatable_key] = meta;
            
            // Set the function's environment using raw Lua API
            lua_State* L = lua.lua_state();
            func.push(L);
            env.push(L);
            lua_setfenv(L, -2);
            lua_pop(L, 1);
            
            // Convert args to vector for easier handling
            std::vector<sol::object> arg_vec;
            arg_vec.reserve(args.size());
            
            for (auto arg : args) {
                arg_vec.push_back(arg);
            }
            
            // Call the function with the new environment
            if (arg_vec.empty()) {
                return func();
            } else {
                return func(sol::as_args(arg_vec));
            }
        } catch (const std::exception& e) {
            report_error("Failed to add wrapper: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static sol::object switch_case(sol::object value, sol::table cases) {
        try {
            LuaStackGuard guard(*g_lua_state);
            sol::state_view lua(value.lua_state());
            
            // Try to find exact match first
            sol::object case_handler = cases[value];
            
            if (case_handler.valid() && case_handler.get_type() == sol::type::function) {
                sol::protected_function func = case_handler;
                auto result = func();

                if (result.valid()) {
                    return result.get<sol::object>();
                }

                return sol::nil;
            }
            
            // No match found, try default
            sol::object default_handler = cases["default"];

            if (default_handler.valid() && default_handler.get_type() == sol::type::function) {
                sol::protected_function func = default_handler;
                auto result = func();

                if (result.valid()) {
                    return result.get<sol::object>();
                }
            }
            
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("Failed to switch statement: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static std::string generate_random_string(int length, sol::optional<bool> use_numbers, 
                                             sol::optional<bool> use_letters, 
                                             sol::optional<bool> use_symbols) {
        try {
            bool nums = use_numbers.value_or(true);
            bool letters = use_letters.value_or(true);
            bool syms = use_symbols.value_or(true);
            return RandomStringGenerator::generate(length, nums, letters, syms);
        } catch (const std::exception& e) {
            report_error("Failed to generate random string: " + std::string(e.what()));
            return "";
        }
    }

    static std::string generate_uid() {
        try {
            return UIDGenerator::generate();
        } catch (const std::exception& e) {
            report_error("Failed to generate UID: " + std::string(e.what()));
            return "";
        }
    }

    static sol::object write_khdata(const std::string& filename, const std::string& var_name, 
                                    sol::object value, sol::optional<sol::object> target_opt, 
                                    sol::optional<sol::object> jip_opt) {
        try {
            LuaStackGuard guard(*g_lua_state);
            game_value gv = convert_lua_to_game_value(value);

            // Check if we should trigger CBA event
            if (target_opt && !target_opt->is<sol::nil_t>() && !(target_opt->is<bool>() && target_opt->as<bool>() == true)) {
                game_value target = convert_lua_to_game_value(*target_opt);
                game_value jip = jip_opt ? convert_lua_to_game_value(*jip_opt) : game_value();
                auto_array<game_value> value_array;
                value_array.push_back(filename);
                value_array.push_back(var_name);
                value_array.push_back(gv);
                auto_array<game_value> cba_params;
                cba_params.push_back(game_value("KH_eve_khDataWriteEmission"));
                cba_params.push_back(game_value(std::move(value_array)));
                cba_params.push_back(target);
                cba_params.push_back(jip);
                return convert_game_value_to_lua(raw_call_sqf_args_native(g_compiled_sqf_trigger_cba_event, game_value(std::move(cba_params))));
            } else {
                auto* file = KHDataManager::instance().get_or_create_file(filename);

                if (!file) {
                    report_error("Failed to access file");
                    return sol::nil;
                }

                file->write_variable(var_name, gv);
            }

            return sol::nil;
        } catch (const std::exception& e) {
            report_error("Failed to write KHData: " + std::string(e.what()));
            return sol::nil;
        }
    }
    
    // KHData read
    static sol::object read_khdata(const std::string& filename, const std::string& var_name, 
                                   sol::optional<sol::object> default_value) {
        try {
            LuaStackGuard guard(*g_lua_state);
            auto* file = KHDataManager::instance().get_or_create_file(filename);
            
            if (!file) {
                return default_value.value_or(sol::nil);
            }
            
            // Special case: if var_name == filename, return all variable names
            if (var_name == filename) {
                auto names = file->get_variable_names();
                sol::table tbl = g_lua_state->create_table();
                
                for (size_t i = 0; i < names.size(); i++) {
                    tbl[i + 1] = names[i];
                }
                
                return sol::make_object(*g_lua_state, tbl);
            }
            
            game_value result = file->read_variable(var_name);
            
            if (result.is_nil() && default_value) {
                return *default_value;
            }
            
            return convert_game_value_to_lua(result);
        } catch (const std::exception& e) {
            report_error("Failed to read KHData: " + std::string(e.what()));
            return sol::nil;
        }
    }

    // KHData flush
    static sol::object flush_khdata() {
        try {
            KHDataManager::instance().flush_all();
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("Failed to flush KHData: " + std::string(e.what()));
            return sol::nil;
        }
    }

    // KHData delete file
    static sol::object delete_khdata_file(const std::string& filename) {
        try {
            KHDataManager::instance().delete_file(filename);
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("Failed to delete KHData file: " + std::string(e.what()));
            return sol::nil;
        }
    }

    // Terrain matrix getter
    static sol::object get_terrain_matrix() {
        try {
            LuaStackGuard guard(*g_lua_state);

            if (g_terrain_matrix.empty()) {
                return sol::nil;
            }
            
            sol::table matrix = g_lua_state->create_table();

            for (size_t y = 0; y < g_terrain_matrix.size(); y++) {
                sol::table row = g_lua_state->create_table();

                for (size_t x = 0; x < g_terrain_matrix[y].size(); x++) {
                    row[x + 1] = g_terrain_matrix[y][x];
                }

                matrix[y + 1] = row;
            }
            
            return sol::make_object(*g_lua_state, matrix);
        } catch (const std::exception& e) {
            report_error("Failed to get terrain matrix: " + std::string(e.what()));
            return sol::nil;
        }
    }

    // Get terrain height at grid coordinates
    static sol::object get_terrain_height_at(int grid_x, int grid_y) {
        try {
            LuaStackGuard guard(*g_lua_state);

            // Convert from 1-based Lua indexing to 0-based C++ indexing
            int x = grid_x - 1;
            int y = grid_y - 1;
            
            if (g_terrain_matrix.empty() || x < 0 || y < 0 || 
                y >= static_cast<int>(g_terrain_matrix.size()) || 
                x >= static_cast<int>(g_terrain_matrix[y].size())) {
                return sol::nil;
            }
            
            return sol::make_object(*g_lua_state, g_terrain_matrix[y][x]);
        } catch (const std::exception& e) {
            report_error("Failed to get terrain height: " + std::string(e.what()));
            return sol::nil;
        }
    }

    // Get interpolated terrain height at world position
    static float get_terrain_height_at_pos(float world_x, float world_y) {
        try {
            // Doesn't need stack guard, returns float
            if (g_terrain_matrix.empty() || g_terrain_grid_width <= 0) {
                return 0.0f;
            }
            
            // Calculate grid coordinates
            float fx = world_x / g_terrain_grid_width;
            float fy = world_y / g_terrain_grid_width;
            
            // Get integer grid coordinates
            int x0 = static_cast<int>(std::floor(fx));
            int y0 = static_cast<int>(std::floor(fy));
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            
            // Clamp to grid bounds
            int max_idx = static_cast<int>(g_terrain_matrix.size()) - 1;
            x0 = std::max(0, std::min(x0, max_idx));
            y0 = std::max(0, std::min(y0, max_idx));
            x1 = std::max(0, std::min(x1, max_idx));
            y1 = std::max(0, std::min(y1, max_idx));
            
            // Get heights at corners
            float h00 = g_terrain_matrix[y0][x0];
            float h10 = g_terrain_matrix[y0][x1];
            float h01 = g_terrain_matrix[y1][x0];
            float h11 = g_terrain_matrix[y1][x1];
            
            // Bilinear interpolation
            float fx_frac = fx - std::floor(fx);
            float fy_frac = fy - std::floor(fy);
            float h0 = h00 * (1 - fx_frac) + h10 * fx_frac;
            float h1 = h01 * (1 - fx_frac) + h11 * fx_frac;
            return h0 * (1 - fy_frac) + h1 * fy_frac;
        } catch (const std::exception& e) {
            report_error("Failed to get terrain height at position: " + std::string(e.what()));
            return 0.0f;
        }
    }
}

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
            sol::lib::jit
        );

        // Override print function to use Arma's system_chat and diag_log
        (*g_lua_state)["print"] = [](sol::variadic_args args) {
            try {
                std::stringstream ss;
                bool first = true;

                for (auto arg : args) {
                    if (!first) ss << "\t";
                    first = false;
                    
                    sol::object obj = arg;
                    switch (obj.get_type()) {
                        case sol::type::nil:
                            ss << "nil";
                            break;
                        case sol::type::boolean:
                            ss << (obj.as<bool>() ? "true" : "false");
                            break;
                        case sol::type::number:
                            ss << obj.as<double>();
                            break;
                        case sol::type::string:
                            ss << obj.as<std::string>();
                            break;
                        case sol::type::table:
                            ss << "table: 0x" << std::hex << obj.pointer();
                            break;
                        case sol::type::function:
                            ss << "function: 0x" << std::hex << obj.pointer();
                            break;
                        case sol::type::userdata: {
                            sol::optional<GameValueWrapper> wrapper = obj.as<sol::optional<GameValueWrapper>>();

                            if (wrapper) {
                                ss << wrapper->to_string();
                            } else {
                                ss << "userdata: 0x" << std::hex << obj.pointer();
                            }
                            
                            break;
                        }
                        default:
                            ss << "unknown: 0x" << std::hex << obj.pointer();
                            break;
                    }
                }
                
                std::string message = ss.str();
                sqf::diag_log(message);
                sqf::system_chat(message);
            } catch (const std::exception& e) {
                report_error("Failed to print: " + std::string(e.what()));
            }
        };

        // Set up panic handler for unprotected errors
        lua_atpanic(g_lua_state->lua_state(), [](lua_State* L) -> int {
            std::string error_msg = lua_tostring(L, -1);
            report_error(error_msg);
            return 0;
        });

        // Panic guard
        g_lua_state->set_panic([](lua_State* L) -> int {
            const char* msg = lua_tostring(L, -1);
            report_error((msg ? msg : "unknown"));
            lua_settop(L, 0);  // Clear stack on panic
            return 0;
        });
        
        // Override the default Lua error function
        g_lua_state->set_function("error", [](sol::variadic_args va) {
            try {
                std::stringstream ss;

                for (auto v : va) {
                    sol::object obj = v;

                    if (obj.is<std::string>()) {
                        ss << obj.as<std::string>();
                    } else if (obj.is<const char*>()) {
                        ss << obj.as<const char*>();
                    } else {
                        ss << lua_tostring(obj.lua_state(), -1);
                    }

                    ss << " ";
                }

                report_error(ss.str());
            } catch (const std::exception& e) {
                report_error("Failed to error: " + std::string(e.what()));
            }
        });
        
        // Set up Sol's default error handler for protected calls
        g_lua_state->set_exception_handler([](lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) -> int {
            if (maybe_exception) {
                report_error(maybe_exception->what());
            } else {
                report_error(std::string(description));
            }

            return sol::stack::push(L, description);
        });

        (*g_lua_state)["crypto"] = g_lua_state->create_table();

        auto crypto_wrapper = [](auto hash_func) {
            return [hash_func](const std::string& input) -> std::string {
                try {
                    return hash_func(input);
                } catch (const std::exception& e) {
                    report_error("Crypto error: " + std::string(e.what()));
                    return "";
                }
            };
        };

        (*g_lua_state)["crypto"] = g_lua_state->create_table_with(
            "md5", crypto_wrapper(CryptoGenerator::md5),
            "sha1", crypto_wrapper(CryptoGenerator::sha1),
            "sha256", crypto_wrapper(CryptoGenerator::sha256),
            "sha512", crypto_wrapper(CryptoGenerator::sha512),
            "fnv1a32", crypto_wrapper(CryptoGenerator::fnv1a32),
            "fnv1a64", crypto_wrapper(CryptoGenerator::fnv1a64),
            "crc32", crypto_wrapper(CryptoGenerator::crc32),
            "xxhash32", crypto_wrapper(CryptoGenerator::xxhash32),
            "adler32", crypto_wrapper(CryptoGenerator::adler32),
            "djb2", crypto_wrapper(CryptoGenerator::djb2),
            "sdbm", crypto_wrapper(CryptoGenerator::sdbm)
        );

        // Register GameValueWrapper userdata type
        g_lua_state->new_usertype<GameValueWrapper>("GameValue",
            sol::constructors<GameValueWrapper(), GameValueWrapper(const game_value&)>(),
            sol::meta_function::to_string, &GameValueWrapper::to_string,
            sol::meta_function::equal_to, &GameValueWrapper::equals,
            "value", &GameValueWrapper::value,
            "type_name", &GameValueWrapper::type_name,
            "is_game_value", &GameValueWrapper::is_game_value
        );

        sol::table game_table = g_lua_state->create_table();
        (*g_lua_state)["game"] = game_table;
        sol::table mission_table = g_lua_state->create_table();
        (*g_lua_state)["mission"] = mission_table;
        sol::table event_table = g_lua_state->create_table();
        (*g_lua_state)["event"] = event_table;
        sol::table game_event_table = g_lua_state->create_table();
        (*g_lua_state)["gameEvent"] = game_event_table;
        sol::table time_table = g_lua_state->create_table();
        (*g_lua_state)["time"] = time_table;
        sol::table temporal_table = g_lua_state->create_table();
        (*g_lua_state)["temporal"] = temporal_table;
        sol::table kh_data_table = g_lua_state->create_table();
        (*g_lua_state)["khData"] = kh_data_table;
        sol::table network_table = g_lua_state->create_table();
        (*g_lua_state)["network"] = network_table;
        sol::table util_table = g_lua_state->create_table();
        (*g_lua_state)["util"] = util_table;
        sol::table terrain_table = g_lua_state->create_table();
        (*g_lua_state)["terrain"] = terrain_table;
        event_table["add"] = LuaFunctions::add_event_handler;
        event_table["remove"] = LuaFunctions::remove_event_handler;
        event_table["trigger"] = LuaFunctions::trigger_event;
        event_table["clear"] = LuaFunctions::clear_handlers;
        event_table["getHandlerCount"] = LuaFunctions::get_handler_count;
        game_event_table["add"] = LuaFunctions::add_game_event_handler;
        game_event_table["remove"] = LuaFunctions::remove_game_event_handler;
        game_event_table["trigger"] = LuaFunctions::trigger_cba_event;
        time_table["getDate"] = LuaFunctions::get_date_time;
        time_table["getEpoch"] = LuaFunctions::get_time_epoch;
        time_table["getBoot"] = LuaFunctions::get_time_boot;
        temporal_table["delay"] = LuaFunctions::delay;
        temporal_table["interval"] = LuaFunctions::interval;
        temporal_table["cancel"] = LuaFunctions::cancel_task;
        network_table["emitVariable"] = LuaFunctions::emit_variable;
        terrain_table["getTerrainMatrix"] = LuaFunctions::get_terrain_matrix;
        terrain_table["getTerrainHeightAt"] = LuaFunctions::get_terrain_height_at;
        terrain_table["getTerrainHeightAtPos"] = LuaFunctions::get_terrain_height_at_pos;
        util_table["profile"] = LuaFunctions::profile_code;
        util_table["execute"] = LuaFunctions::execute_lua;
        util_table["generateRandomString"] = LuaFunctions::generate_random_string;
        util_table["generateUid"] = LuaFunctions::generate_uid;
        util_table["getDataType"] = LuaFunctions::get_data_type;
        util_table["withSqf"] = LuaFunctions::with_sqf;
        util_table["switch"] = LuaFunctions::switch_case;
        util_table["sqfCall"] = LuaFunctions::sqf_call;
        util_table["removeHandler"] = LuaFunctions::remove_handler;
        
        kh_data_table["write"] = sol::overload(
            [](const std::string& f, const std::string& v, sol::object val) {
                return LuaFunctions::write_khdata(f, v, val, sol::nullopt, sol::nullopt);
            },
            LuaFunctions::write_khdata
        );

        kh_data_table["read"] = sol::overload(
            [](const std::string& f, const std::string& v) {
                return LuaFunctions::read_khdata(f, v, sol::nullopt);
            },
            LuaFunctions::read_khdata
        );

        kh_data_table["flush"] = LuaFunctions::flush_khdata;
        kh_data_table["deleteFile"] = LuaFunctions::delete_khdata_file;

        // This lets you get and set sqf variables using sqfVar.someVariable
        sol::table sqfVar = g_lua_state->create_table();

        sqfVar[sol::metatable_key] = g_lua_state->create_table_with(
            "__index", [](sol::object key) -> sol::object {
                try {
                    if (key.get_type() != sol::type::string) return sol::nil;
                    std::string var_name = key.as<std::string>();
                    game_value result = sqf::get_variable(sqf::current_namespace(), var_name);
                    return convert_game_value_to_lua(result);
                } catch (const std::exception& e) {
                    report_error("sqfVar.__index error: " + std::string(e.what()));
                    return sol::nil;
                }
            },
            "__newindex", [](sol::object key, sol::object value) {
                try {
                    if (key.get_type() != sol::type::string) return;
                    std::string var_name = key.as<std::string>();
                    sqf::set_variable(sqf::current_namespace(), var_name, convert_lua_to_game_value(value));
                } catch (const std::exception& e) {
                    report_error("sqfVar.__newindex error: " + std::string(e.what()));
                }
            }
        );

        (*g_lua_state)["sqfVar"] = sqfVar;

        auto command_handler = [](std::string cmd, sol::variadic_args args) -> sol::object {
            try {
                LuaStackGuard guard(*g_lua_state);
                
                // Fast path for zero args (most common case)
                if (args.size() == 0) {
                    auto cache_it = g_sqf_command_cache.find(cmd);
                    
                    if (cache_it != g_sqf_command_cache.end()) {
                        return convert_game_value_to_lua(raw_call_sqf_native(cache_it->second));
                    }
                    
                    game_value compiled = sqf::compile(cmd);
                    g_sqf_command_cache.emplace(cmd, compiled);
                    return convert_game_value_to_lua(raw_call_sqf_native(compiled));
                }
                
                // Build the cache key once
                std::string key;
                
                if (args.size() == 1) {
                    key = cmd + " _x";
                } else if (args.size() == 2) {
                    key = "(_x select 0) " + cmd + " (_x select 1)";
                } else {
                    report_error("SQF commands only support 0-2 arguments");
                    return sol::nil;
                }
                
                auto cache_it = g_sqf_command_cache.find(key);
                game_value compiled;
                
                if (cache_it != g_sqf_command_cache.end()) {
                    compiled = cache_it->second;
                } else {
                    compiled = sqf::compile(key);
                    g_sqf_command_cache.emplace(key, compiled);
                }
                
                // Convert args based on count
                if (args.size() == 1) {
                    return convert_game_value_to_lua(
                        raw_call_sqf_args_native(compiled, convert_lua_to_game_value(args[0]))
                    );
                } else {
                    return convert_game_value_to_lua(raw_call_sqf_args_native(
                        compiled,
                        game_value({
                            convert_lua_to_game_value(args[0]),
                            convert_lua_to_game_value(args[1])
                        })
                    ));
                }
            } catch (const std::exception& e) {
                report_error("Failed to handle command: " + std::string(e.what()));
                return sol::nil;
            }
        };

        // Sqf table for SQF commands
        auto sqf_table = g_lua_state->create_named_table("sqf");
        sol::table sqf_metatable = g_lua_state->create_table();

        sqf_metatable["__index"] = [command_handler](sol::table table, std::string key) -> sol::object {
            try {
                LuaStackGuard guard(*g_lua_state);
                
                // First check if key exists in the table
                sol::object existing = table.raw_get<sol::object>(key);
                
                if (existing != sol::nil) {
                    return existing;
                }
                
                // Create the command wrapper
                sol::state_view lua(table.lua_state());
                
                auto command_func = [key, command_handler](sol::variadic_args args) -> sol::object {
                    return command_handler(key, args);
                };
                
                sol::object wrapped = sol::make_object(lua, command_func);
                
                // Cache it in the table for next time
                table.raw_set(key, wrapped);
                return wrapped;
            } catch (const std::exception& e) {
                report_error("SQF metatable error: " + std::string(e.what()));
                return sol::nil;
            }
        };

        sqf_table[sol::metatable_key] = sqf_metatable;

        // All SQF command wrappers
        #include "lua_wrappers_sqf.hpp"
    }
}

static void clean_lua_state() {
    g_call_cache.clear();
    g_code_cache.clear();
    g_sqf_compiled_cache.clear();
    g_sqf_function_cache.clear();
    g_sqf_command_cache.clear();
    LuaFunctions::lua_scheduled_tasks.clear();
    LuaFunctions::lua_event_handlers.clear();
    LuaFunctions::next_task_id = 1;
    LuaFunctions::next_event_handler_id = 1;
}

static void reset_lua_state() {
    try {
        sqf::diag_log("KH Framework - Resetting Lua state");
        clean_lua_state();
        g_lua_state.reset();
        initialize_lua_state();
        sqf::diag_log("KH Framework - Lua state reset");
    } catch (const std::exception& e) {
        report_error("Error resetting Lua state: " + std::string(e.what()));
    }
}