#pragma once

using namespace intercept;
using namespace intercept::types;

struct LuaCallCache {
    sol::protected_function func;
};

struct LuaLocalExecCache {
    sol::protected_function func;
    game_value source;
};

static constexpr size_t LUA_LOCAL_EXEC_CACHE_MAX = 65536;
static std::unordered_map<uintptr_t, LuaLocalExecCache> g_local_exec_cache;
static std::unique_ptr<sol::state> g_lua_state;
static std::unordered_map<std::string, LuaCallCache> g_call_cache;
static std::unordered_map<size_t, sol::protected_function> g_code_cache;
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
    // Rewrites the C-style operators "!=", "&&", "||" and "!" to Lua's "~=", "and", "or" and "not" outside string
    // literals and comments (quoted strings, [=*[ long strings, "--" line comments and --[=*[ long comments).
    static std::string preprocess_lua_operators(const std::string& code) {
        if (code.find("&&") == std::string::npos &&
            code.find("||") == std::string::npos &&
            code.find('!') == std::string::npos) {
            return code;
        }

        const size_t n = code.length();

        // The ranges (inclusive) to leave alone, in order of position.
        std::vector<std::pair<size_t, size_t>> skip_ranges;

        // A long bracket opening at i ("[" then "="* then "["): its level, or -1 when there is none.
        auto long_bracket_level = [&](size_t i, size_t& open_end) -> int {
            if (i >= n || code[i] != '[') return -1;
            size_t j = i + 1;
            while (j < n && code[j] == '=') j++;
            if (j >= n || code[j] != '[') return -1;
            open_end = j;
            return static_cast<int>(j - i - 1);
        };
        // The range of the long bracket opened at i (open_end its second '['): to its closing bracket, or to the end
        // of the code when it has none (Lua refuses such code anyway).
        auto long_bracket_range = [&](size_t open_end, int level) -> size_t {
            const std::string closing = "]" + std::string(static_cast<size_t>(level), '=') + "]";
            const size_t close_pos = code.find(closing, open_end + 1);
            return close_pos == std::string::npos ? n - 1 : close_pos + closing.length() - 1;
        };

        for (size_t i = 0; i < n; i++) {
            const char c = code[i];

            if (c == '-' && i + 1 < n && code[i + 1] == '-') {
                size_t open_end = 0;
                const int level = long_bracket_level(i + 2, open_end);
                size_t end;

                if (level >= 0) {
                    end = long_bracket_range(open_end, level);
                } else {
                    const size_t newline = code.find('\n', i + 2);
                    end = newline == std::string::npos ? n - 1 : newline;
                }

                skip_ranges.push_back({i, end});
                i = end;
            } else if (c == '[') {
                size_t open_end = 0;
                const int level = long_bracket_level(i, open_end);

                if (level >= 0) {
                    const size_t end = long_bracket_range(open_end, level);
                    skip_ranges.push_back({i, end});
                    i = end;
                }
            } else if (c == '"' || c == '\'') {
                const size_t start = i;

                for (i++; i < n; i++) {
                    if (code[i] == '\\') {
                        i++;   // The escaped character (a quote among them) does not end the string.
                    } else if (code[i] == c) {
                        skip_ranges.push_back({start, i});
                        break;
                    }
                }
            }
        }

        std::string output;
        output.reserve(n + n / 8);
        size_t range = 0;

        for (size_t i = 0; i < n; ) {
            while (range < skip_ranges.size() && skip_ranges[range].second < i) range++;

            if (range < skip_ranges.size() && skip_ranges[range].first <= i) {
                const size_t end = skip_ranges[range].second;
                output.append(code, i, end - i + 1);
                i = end + 1;
                continue;
            }

            const char c = code[i];

            if (c == '!') {
                if (i + 1 < n && code[i + 1] == '=') {
                    output += "~=";
                    i += 2;
                } else {
                    output += " not ";
                    i++;
                }
            } else if (c == '&' && i + 1 < n && code[i + 1] == '&') {
                output += " and ";
                i += 2;
            } else if (c == '|' && i + 1 < n && code[i + 1] == '|') {
                output += " or ";
                i += 2;
            } else {
                output += c;
                i++;
            }
        }

        return output;
    }

    struct CompileResult {
        bool success;
        std::string error_message;
        sol::protected_function function;
        
        CompileResult(bool s, std::string err, sol::protected_function func = {}) 
            : success(s), error_message(std::move(err)), function(std::move(func)) {}
    };
    
    // The caller (luaCompile) reports error_message.
    static CompileResult lua_compile(const std::string& lua_code, const std::string& lua_name = "") {
        if (lua_code.empty()) return CompileResult(false, "empty Lua code provided");

        try {
            std::string processed_code = preprocess_lua_operators(lua_code);
            sol::load_result load_result = g_lua_state->load(processed_code);

            if (!load_result.valid()) {
                sol::error err = load_result;
                return CompileResult(false, "syntax error: " + std::string(err.what()));
            }

            sol::protected_function compiled_func = load_result;

            if (!lua_name.empty()) {
                (*g_lua_state)[lua_name] = compiled_func;
            }

            return CompileResult(true, "Success", std::move(compiled_func));
        } catch (const sol::error& e) {
            return CompileResult(false, "compilation failed: " + std::string(e.what()));
        } catch (const std::exception& e) {
            return CompileResult(false, "unexpected error: " + std::string(e.what()));
        }
    }
};

// Userdata wrapper for game_value to preserve native Arma data types within Lua
struct GameValueWrapper {
    game_value value;
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
                lua_createtable(L, 0, 0);
                return sol::stack::pop<sol::object>(L);
            }

            size_t arr_size = array.size();
            game_data_type first_type = array[0].type_enum();

            // Fast path for homogeneous primitives
            switch (first_type) {
                case game_data_type::SCALAR: {
                    lua_createtable(L, arr_size, 0);
                    lua_pushnumber(L, static_cast<float>(array[0]));
                    lua_rawseti(L, -2, 1);

                    for (size_t i = 1; i < arr_size; ++i) {
                        if (array[i].type_enum() != game_data_type::SCALAR) {
                            lua_pop(L, 1);  // Clean up table
                            goto heterogeneous_array;
                        }

                        lua_pushnumber(L, static_cast<float>(array[i]));
                        lua_rawseti(L, -2, i + 1);
                    }

                    return sol::stack::pop<sol::object>(L);
                }
                
                case game_data_type::BOOL: {
                    lua_createtable(L, arr_size, 0);
                    lua_pushboolean(L, static_cast<bool>(array[0]));
                    lua_rawseti(L, -2, 1);

                    for (size_t i = 1; i < arr_size; ++i) {
                        if (array[i].type_enum() != game_data_type::BOOL) {
                            lua_pop(L, 1);
                            goto heterogeneous_array;
                        }

                        lua_pushboolean(L, static_cast<bool>(array[i]));
                        lua_rawseti(L, -2, i + 1);
                    }

                    return sol::stack::pop<sol::object>(L);
                }
                
                case game_data_type::STRING: {
                    lua_createtable(L, arr_size, 0);
                    std::string str0 = static_cast<std::string>(array[0]);
                    lua_pushlstring(L, str0.c_str(), str0.length());
                    lua_rawseti(L, -2, 1);

                    for (size_t i = 1; i < arr_size; ++i) {
                        if (array[i].type_enum() != game_data_type::STRING) {
                            lua_pop(L, 1);
                            goto heterogeneous_array;
                        }

                        std::string str = static_cast<std::string>(array[i]);
                        lua_pushlstring(L, str.c_str(), str.length());
                        lua_rawseti(L, -2, i + 1);
                    }

                    return sol::stack::pop<sol::object>(L);
                }
                
                case game_data_type::OBJECT:
                case game_data_type::GROUP:
                case game_data_type::SIDE:
                case game_data_type::CONFIG:
                case game_data_type::CONTROL:
                case game_data_type::DISPLAY:
                case game_data_type::LOCATION:
                case game_data_type::SCRIPT:
                case game_data_type::TEXT:
                case game_data_type::TEAM_MEMBER:
                case game_data_type::CODE:
                case game_data_type::TASK:
                case game_data_type::DIARY_RECORD:
                case game_data_type::NetObject:
                case game_data_type::SUBGROUP:
                case game_data_type::TARGET:
                case game_data_type::HASHMAP:
                case game_data_type::NAMESPACE: {
                    lua_createtable(L, arr_size, 0);
                    sol::stack::push(*g_lua_state, GameValueWrapper(array[0]));
                    lua_rawseti(L, -2, 1);

                    for (size_t i = 1; i < arr_size; ++i) {
                        if (array[i].type_enum() != first_type) {
                            lua_pop(L, 1);
                            goto heterogeneous_array;
                        }

                        sol::stack::push(*g_lua_state, GameValueWrapper(array[i]));
                        lua_rawseti(L, -2, i + 1);
                    }

                    return sol::stack::pop<sol::object>(L);
                }
                
                case game_data_type::NOTHING:
                case game_data_type::ANY: {
                    lua_createtable(L, arr_size, 0);

                    for (size_t i = 0; i < arr_size; ++i) {
                        if (i > 0 && array[i].type_enum() != first_type) {
                            lua_pop(L, 1);
                            goto heterogeneous_array;
                        }

                        lua_pushnil(L);
                        lua_rawseti(L, -2, i + 1);
                    }

                    return sol::stack::pop<sol::object>(L);
                }
                
                case game_data_type::ARRAY: {
                    size_t nested_size = array[0].size();

                    if (nested_size == 2 || nested_size == 3) {
                        auto& first_sub = array[0].to_array();
                        bool all_numbers = true;
                        
                        for (size_t j = 0; j < nested_size; ++j) {
                            if (first_sub[j].type_enum() != game_data_type::SCALAR) {
                                all_numbers = false;
                                break;
                            }
                        }
                        
                        if (all_numbers) {
                            lua_createtable(L, arr_size, 0);
                            
                            for (size_t i = 0; i < arr_size; ++i) {
                                if (array[i].type_enum() != game_data_type::ARRAY) {
                                    lua_pop(L, 1);
                                    goto heterogeneous_array;
                                }

                                auto& sub = array[i].to_array();

                                if (sub.size() != nested_size) {
                                    lua_pop(L, 1);
                                    goto heterogeneous_array;
                                }
                                
                                lua_createtable(L, nested_size, 0);

                                for (size_t j = 0; j < nested_size; ++j) {
                                    if (sub[j].type_enum() != game_data_type::SCALAR) {
                                        lua_pop(L, 2);  // Pop nested table AND outer table
                                        goto heterogeneous_array;
                                    }
                                    lua_pushnumber(L, static_cast<float>(sub[j]));
                                    lua_rawseti(L, -2, j + 1);
                                }

                                lua_rawseti(L, -2, i + 1);
                            }

                            return sol::stack::pop<sol::object>(L);
                        }
                    }

                    break;
                }
                
                default:
                    break;
            }

        heterogeneous_array:
            lua_createtable(L, arr_size, 0);

            for (size_t i = 0; i < arr_size; ++i) {
                convert_game_value_to_lua(array[i]).push(L);
                lua_rawseti(L, -2, i + 1);
            }

            return sol::stack::pop<sol::object>(L);
        }

        case game_data_type::HASHMAP: {
            auto& hashmap = value.to_hashmap();
            size_t count = hashmap.count();

            if (count == 0) {
                lua_createtable(L, 0, 0);
                return sol::stack::pop<sol::object>(L);
            }

            auto it = hashmap.begin();
            game_data_type value_type = it->value.type_enum();
            bool is_homogeneous = true;
            auto check_it = it;

            for (; check_it != hashmap.end(); ++check_it) {
                if (check_it->value.type_enum() != value_type) {
                    is_homogeneous = false;
                    break;
                }
            }

            lua_createtable(L, 0, count);

            if (is_homogeneous && value_type == game_data_type::SCALAR) {
                for (auto& pair : hashmap) {
                    convert_game_value_to_lua(pair.key).push(L);
                    lua_pushnumber(L, static_cast<float>(pair.value));
                    lua_rawset(L, -3);
                }

                return sol::stack::pop<sol::object>(L);
            }

            if (is_homogeneous && value_type == game_data_type::STRING) {
                for (auto& pair : hashmap) {
                    convert_game_value_to_lua(pair.key).push(L);
                    std::string str = static_cast<std::string>(pair.value);
                    lua_pushlstring(L, str.c_str(), str.length());
                    lua_rawset(L, -3);
                }

                return sol::stack::pop<sol::object>(L);
            }

            for (auto& pair : hashmap) {
                convert_game_value_to_lua(pair.key).push(L);
                convert_game_value_to_lua(pair.value).push(L);
                lua_rawset(L, -3);
            }

            return sol::stack::pop<sol::object>(L);
        }

        case game_data_type::NOTHING:
        case game_data_type::ANY:
            return sol::make_object(lua, sol::nil);

        default:
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

    if (type == sol::type::function) {
        report_error("KH Lua: a function cannot be passed as an SQF value");
        return game_value();
    }

    if (type == sol::type::table) {
        lua_State* L = obj.lua_state();
        obj.push(L);
        
        // Check for metatable with __toSQF
        if (lua_getmetatable(L, -1)) {
            lua_getfield(L, -1, "__toSQF");
            if (lua_isfunction(L, -1)) {
                lua_pushvalue(L, -3);  // Push table

                if (lua_pcall(L, 1, 1, 0) == 0) {
                    sol::object result = sol::stack::pop<sol::object>(L);
                    lua_pop(L, 1);  // Pop metatable
                    lua_pop(L, 1);  // Pop original table
                    return convert_lua_to_game_value(result);
                }

                lua_pop(L, 1);  // Pop error
            } else {
                lua_pop(L, 1);  // Pop non-function
            }

            lua_pop(L, 1);  // Pop metatable
        }
        
        // Single-pass table analysis using lua_next: whether every key is a positive integer, the largest, and the
        // number of entries.
        bool is_array = true;
        size_t max_index = 0;
        size_t non_nil_count = 0;
        
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            non_nil_count++;
            double key_num = 0.0;

            if (lua_type(L, -2) == LUA_TNUMBER) key_num = lua_tonumber(L, -2);

            if (key_num > 0 && key_num == std::floor(key_num)) {
                size_t idx = static_cast<size_t>(key_num);
                if (idx > max_index) max_index = idx;
            } else {
                is_array = false;
                lua_pop(L, 2);  // Pop value and key
                break;
            }

            lua_pop(L, 1);  // Pop value, keep key for next iteration
        }
        
        if (non_nil_count == 0) {
            lua_pop(L, 1);  // Pop table
            return game_value(auto_array<game_value>());
        }
        
        // An SQF array: a proper sequence (1..n, no holes) of any length, or a sparse one with at most 10000 slots
        // and at least 1% of them set (the holes become nil). Anything else is a hash map.
        if (is_array) {
            const float density = static_cast<float>(non_nil_count) / static_cast<float>(max_index);

            if (non_nil_count == max_index || (max_index <= 10000 && density >= 0.01f)) {
                auto_array<game_value> arr;
                arr.reserve(max_index);
                
                for (size_t i = 1; i <= max_index; i++) {
                    lua_rawgeti(L, -1, static_cast<int>(i));
                    sol::object elem = sol::stack::pop<sol::object>(L);
                    arr.push_back(convert_lua_to_game_value(elem));
                }
                
                lua_pop(L, 1);  // Pop table
                return game_value(std::move(arr));
            }
        }
        
        // Hashmap conversion
        auto_array<game_value> kv_array;
        kv_array.reserve(non_nil_count);
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            auto_array<game_value> kv_pair;
            kv_pair.reserve(2);
            game_value key_value;

            switch (lua_type(L, -2)) {
                case LUA_TSTRING: {
                    size_t len;
                    const char* str = lua_tolstring(L, -2, &len);
                    key_value = game_value(std::string(str, len));
                    break;
                }

                case LUA_TNUMBER:
                    key_value = game_value(static_cast<float>(lua_tonumber(L, -2)));
                    break;

                case LUA_TBOOLEAN:
                    key_value = game_value(static_cast<bool>(lua_toboolean(L, -2)));
                    break;

                default: {
                    lua_pushvalue(L, -2);
                    sol::object key_obj = sol::stack::pop<sol::object>(L);
                    key_value = convert_lua_to_game_value(key_obj);
                    break;
                }
            }

            lua_pushvalue(L, -1);
            sol::object val_obj = sol::stack::pop<sol::object>(L);
            kv_pair.push_back(std::move(key_value));
            kv_pair.push_back(convert_lua_to_game_value(val_obj));
            kv_array.push_back(game_value(std::move(kv_pair)));
            lua_pop(L, 1);  // Pop value, keep key
        }
        
        lua_pop(L, 1);  // Pop table
        return raw_call_sqf_args_native(g_compiled_sqf_create_hash_map_from_array, game_value(std::move(kv_array)));
    } else if (type == sol::type::userdata) {
        sol::optional<GameValueWrapper> wrapper = obj.as<sol::optional<GameValueWrapper>>();

        if (wrapper) {
            return wrapper->value;
        }
    }
    
    return game_value();
}

// C++ implementations of Lua utility functions
namespace LuaFunctions {
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
    
    // Schedule a function to run after a delay
    static int delay(float time_or_frames, sol::protected_function callback) {
        try {
            if (!callback.valid()) {
                report_error("temporal.delay: invalid callback function");
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

            lua_scheduled_tasks[task_id] = std::move(task);
            return task_id;
        } catch (const std::exception& e) {
            report_error("temporal.delay: " + std::string(e.what()));
            return -1;
        }
    }

    // Schedule interval function
    static int interval(float time_or_frames, bool execute_immediately, float timeout, bool prioritize, sol::protected_function callback) {
        try {
            if (!callback.valid()) {
                report_error("temporal.interval: invalid callback function");
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
                // At least one frame: a fraction (-1 < t < 0) would truncate to 0 (every frame, and a division by
                // zero at the timeout boundary).
                int frames = std::max(1, static_cast<int>(std::abs(time_or_frames)));
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

            lua_scheduled_tasks[task_id] = std::move(task);
            return task_id;
        } catch (const std::exception& e) {
            report_error("temporal.interval: " + std::string(e.what()));
            return -1;
        }
    }

    static bool cancel_task(int task_id) {
        try {
            return lua_scheduled_tasks.erase(task_id) > 0;
        } catch (const std::exception& e) {
            report_error("temporal.cancel: " + std::string(e.what()));
            return false;
        }
    }

    // Update scheduler
    static void update_scheduler() {
        struct DueTask {
            int task_id;
            sol::protected_function callback;
            bool at_timeout_boundary;
        };

        std::vector<DueTask> due;
        std::vector<int> tasks_to_remove;

        for (auto& [task_id, task] : lua_scheduled_tasks) {
            if (!task.callback.valid()) {
                tasks_to_remove.push_back(task_id);
                continue;
            }

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

            bool should_execute = task.use_frames
                ? (g_mission_frame >= task.execute_frame)
                : (g_mission_time >= task.execute_time);

            if (should_execute) {
                due.push_back({task_id, task.callback, at_timeout_boundary});
            }
        }

        for (auto& d : due) {
            // An earlier callback this tick may have cancelled this task.
            if (lua_scheduled_tasks.find(d.task_id) == lua_scheduled_tasks.end()) continue;
            sol::protected_function_result result = d.callback();

            if (!result.valid()) {
                sol::error err = result;
                report_error("KH Lua: scheduled task error: " + std::string(err.what()));
            }

            bool should_cancel = false;

            if (result.valid() && result.return_count() > 0) {
                sol::object ret_val = result.get<sol::object>();
                if (ret_val.is<bool>() && ret_val.as<bool>() == true) should_cancel = true;
            }

            auto it = lua_scheduled_tasks.find(d.task_id);   // re-find AFTER the callback (may have rehashed)
            if (it == lua_scheduled_tasks.end()) continue;   // callback cancelled itself
            ScheduledTask& task = it->second;

            if (d.at_timeout_boundary || should_cancel) {
                lua_scheduled_tasks.erase(it);
            } else if (task.repeating) {
                if (task.use_frames) task.execute_frame = g_mission_frame + task.frame_interval;
                else task.execute_time = g_mission_time + task.interval;
            } else {
                lua_scheduled_tasks.erase(it);
            }
        }

        for (int task_id : tasks_to_remove) {
            lua_scheduled_tasks.erase(task_id);
        }
    }

    static std::unordered_map<std::string, std::unordered_map<int, sol::protected_function>> lua_event_handlers;
    static int next_event_handler_id = 1;

    static int add_event_handler(const std::string& event_name, sol::protected_function handler) {
        try {
            if (!handler.valid()) {
                report_error("event.add: invalid handler function");
                return -1;
            }

            int handler_id = next_event_handler_id++;
            lua_event_handlers[event_name][handler_id] = std::move(handler);
            return handler_id;
        } catch (const std::exception& e) {
            report_error("event.add: " + std::string(e.what()));
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
            report_error("event.remove: " + std::string(e.what()));
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
                        // lua_tostring is NULL for a non-string, non-number error object
                        // (assert(false, t) with t a table or nil raises t itself) - sqf's trigger has the twin.
                        const char* message = lua_tostring(L, -1);
                        std::string err = message
                            ? std::string(message)
                            : "error object of type " + std::string(lua_typename(L, lua_type(L, -1)));
                        report_error("event.trigger: handler error: " + err);
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
            return convert_game_value_to_lua(trigger_cba_event_sqf(game_value(std::move(cba_params))));
        } catch (const std::exception& e) {
            report_error("event.trigger: " + std::string(e.what()));
            return sol::nil;
        }
    }
        
    // Clear handlers
    static void clear_handlers(const std::string& event_name) {
        try {
            if (event_name.empty()) {
                report_error("event.clear: handler name cannot be empty");
            } else {
                lua_event_handlers.erase(event_name);
            }
        } catch (const std::exception& e) {
            report_error("event.clear: " + std::string(e.what()));
        }
    }
    
    // Get handler count for debugging
    static int get_handler_count(const std::string& event_name) {
        try {
            auto it = lua_event_handlers.find(event_name);
            return it != lua_event_handlers.end() ? static_cast<int>(it->second.size()) : 0;
        } catch (const std::exception& e) {
            report_error("event.getHandlerCount: " + std::string(e.what()));
            return 0;
        }
    }

    static sol::object add_game_event_handler(sol::object type, sol::object event, sol::protected_function handler) {
        try {
            LuaStackGuard guard(*g_lua_state);

            if (!handler.valid()) {
                report_error("gameEvent.add: invalid handler function");
                return sol::nil;
            }

            std::string event_name;

            if (event.get_type() == sol::type::string) {
                event_name = event.as<std::string>();
            } else {
                report_error("gameEvent.add: event name must be a string");
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
            sqf_params.push_back(game_value(event_name));
            sqf_params.push_back(game_value(handler_uid));
            sqf_params.push_back(g_compiled_sqf_game_event_handler_lua_bridge);
            game_value sqf_result = raw_call_sqf_args_native(g_compiled_sqf_add_game_event_handler, game_value(std::move(sqf_params)));
            sol::table result = g_lua_state->create_table();
            result[1] = handler_uid;
            result[2] = handler_id;
            result[3] = convert_game_value_to_lua(sqf_result);
            return sol::make_object(*g_lua_state, result);
        } catch (const std::exception& e) {
            report_error("gameEvent.add: " + std::string(e.what()));
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
                report_error("gameEvent.remove: invalid handler info: missing or invalid uid");
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
            report_error("gameEvent.remove: " + std::string(e.what()));
            return false;
        }
    }

    static sol::object execute_lua(sol::object target, sol::object environment, sol::object special, sol::object func, sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            sol::state& lua = *g_lua_state;

            // A local target (nil / true) without a special flag: a number or numeric string environment schedules
            // the call (interval / delay) and a nil one runs it now.
            if ((target.get_type() == sol::type::nil || (target.is<bool>() && target.as<bool>())) && 
                (special.get_type() == sol::type::nil || (special.is<bool>() && !special.as<bool>()))) {
                // A function that runs this call with its arguments (the scheduled forms).
                auto make_scheduled_call = [&]() -> sol::protected_function {
                    sol::table args_table = lua.create_table();
                    int idx = 1;

                    for (const auto& arg : args) {
                        args_table[idx++] = arg;
                    }

                    sol::protected_function wrapper_factory = lua.script(R"(
                        return function()
                            local target, special, func, args = ...
                            return util.execute(target, nil, special, func, table.unpack(args))
                        end
                    )");
                    return wrapper_factory(target, special, func, args_table);
                };

                if (environment.get_type() == sol::type::number) {
                    // Interval: execute_immediately=true, timeout=0, prioritizeTimeout=false
                    int task_id = interval(environment.as<float>(), true, 0.0f, false, make_scheduled_call());
                    return sol::make_object(lua, task_id);
                } else if (environment.get_type() == sol::type::string) {
                    float delay_time;

                    try {
                        delay_time = std::stof(environment.as<std::string>());
                    } catch (...) {
                        report_error("util.execute: environment string must be a valid number for delay");
                        return sol::nil;
                    }

                    int task_id = delay(delay_time, make_scheduled_call());
                    return sol::make_object(lua, task_id);
                } else if (environment.get_type() == sol::type::nil) {
                    sol::protected_function pfunc;

                    if (func.get_type() == sol::type::string) {
                        std::string func_name = func.as<std::string>();
                        sol::object func_obj = lua[func_name];
                        
                        if (func_obj.get_type() != sol::type::function) {
                            report_error("util.execute: function '" + func_name + "' not found in Lua globals");
                            return sol::nil;
                        }
                        
                        pfunc = func_obj;
                    } else if (func.get_type() == sol::type::function) {
                        pfunc = func;
                    } else {
                        report_error("util.execute: function must be a string or Lua function for local execution");
                        return sol::nil;
                    }

                    std::vector<sol::object> args_vec;

                    for (const auto& arg : args) {
                        args_vec.push_back(arg);
                    }

                    if (args_vec.empty()) {
                        return pfunc();
                    }

                    return pfunc(sol::as_args(args_vec));
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
                        report_error("util.execute: cannot serialize anonymous function for remote execution");
                        return sol::nil;
                    }
                }
            } else {
                report_error("util.execute: function must be a string or Lua function");
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
            report_error("util.execute: " + std::string(e.what()));
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
            return convert_game_value_to_lua(trigger_cba_event_sqf(game_value(std::move(cba_params))));
        } catch (const std::exception& e) {
            report_error("gameEvent.trigger: " + std::string(e.what()));
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
                    report_error("network.emitVariable: Lua global variable '" + var_name + "' not found or is nil");
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
            return convert_game_value_to_lua(trigger_cba_event_sqf(game_value(std::move(cba_params))));
        } catch (const std::exception& e) {
            report_error("network.emitVariable: " + std::string(e.what()));
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
            report_error("util.removeHandler: " + std::string(e.what()));
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
            report_error("time.getDate: " + std::string(e.what()));
            return "";
        }
    }

    // Seconds since the Unix epoch, with microsecond precision (getEpoch's clock; high_resolution_clock is
    // steady_clock on MSVC - uptime, not the epoch).
    static double get_time_epoch() {
        try {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            return microseconds / 1000000.0;  // Convert to seconds
        } catch (const std::exception& e) {
            report_error("time.getEpoch: " + std::string(e.what()));
            return 0.0;
        }
    }

    // Milliseconds since boot (QueryPerformanceCounter), fractional - for deltas (util.profile).
    static double get_time_boot() {
        try {
            LARGE_INTEGER frequency, counter;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&counter);
            return (counter.QuadPart * 1000.0) / frequency.QuadPart;
        } catch (const std::exception& e) {
            report_error("time.getBoot: " + std::string(e.what()));
            return 0.0;
        }
    }
    
    // Profile code execution speed
    static sol::object profile_code(sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            sol::state& lua = *g_lua_state;

            if (args.size() < 2) {
                report_error("util.profile: not enough arguments: need at least count and function");
                return sol::nil;
            }

            // First argument: iteration count
            sol::object count_obj = args[0];
            int count = count_obj.as<int>();

            if (count < 1) {
                report_error("util.profile: execution count must be at least 1");
                return sol::nil;
            }

            // Second argument: function
            sol::object code_obj = args[1];
            sol::protected_function compiled;

            if (code_obj.get_type() == sol::type::function) {
                compiled = code_obj;
            } else {
                report_error("util.profile: second argument must be a function object");
                return sol::nil;
            }

            // Remaining arguments: function parameters
            std::vector<sol::object> func_args;

            for (size_t i = 2; i < args.size(); i++) {
                func_args.push_back(args[i]);
            }

            // Warm up if count > 100 - user probably expects overusage speed
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
                report_error("util.profile: high precision timer not available");
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

            snprintf(buffer, sizeof(buffer), "Count: %d\nTotal (ms): %.6f\nAverage (ms): %.6f",
                    count, total_time, average_time);

            return sol::make_object(lua, std::string(buffer));
        } catch (const std::exception& e) {
            report_error("util.profile: " + std::string(e.what()));
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
            report_error("util.getDataType: " + std::string(e.what()));
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
                    report_error("util.withSqf: SQF variable read failed: " + std::string(e.what()));
                    return sol::nil;
                }
            };

            // __newindex: Write to SQF variables by default
            meta["__newindex"] = [](sol::table t, sol::object key, sol::object value) {
                try {
                    if (key.get_type() != sol::type::string) return;
                    std::string var_name = key.as<std::string>();

                    // Don't allow overwriting "lua" keyword
                    if (var_name == "lua") {
                        report_error("util.withSqf: the 'lua' name cannot be overwritten");
                        return;
                    }

                    // Set as SQF variable
                    sqf::set_variable(sqf::current_namespace(), var_name, convert_lua_to_game_value(value));
                } catch (const std::exception& e) {
                    report_error("util.withSqf: " + std::string(e.what()));
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
            report_error("util.withSqf: " + std::string(e.what()));
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
            report_error("util.generateRandomString: " + std::string(e.what()));
            return "";
        }
    }

    static std::string generate_uid() {
        try {
            return UIDGenerator::generate();
        } catch (const std::exception& e) {
            report_error("util.generateUid: " + std::string(e.what()));
            return "";
        }
    }

    // File and variable names are case-insensitive (lower-cased), as in writeKhData / readKhData.
    static sol::object write_khdata(const std::string& filename_raw, const std::string& var_name_raw,
                                    sol::object value, sol::optional<sol::object> target_opt,
                                    sol::optional<sol::object> jip_opt) {
        try {
            LuaStackGuard guard(*g_lua_state);
            const std::string filename = kh_lower_copy(filename_raw);
            const std::string var_name = kh_lower_copy(var_name_raw);
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
                return convert_game_value_to_lua(trigger_cba_event_sqf(game_value(std::move(cba_params))));
            } else {
                auto* file = KHDataManager::instance().get_or_create_file(filename);

                if (!file) {
                    report_error("khData.write: failed to access file");
                    return sol::nil;
                }

                file->write_variable(var_name, gv);
            }

            return sol::nil;
        } catch (const std::exception& e) {
            report_error("khData.write: " + std::string(e.what()));
            return sol::nil;
        }
    }
    
    static sol::object read_khdata(const std::string& filename_raw, const std::string& var_name_raw,
                                   sol::optional<sol::object> default_value) {
        try {
            LuaStackGuard guard(*g_lua_state);
            const std::string filename = kh_lower_copy(filename_raw);
            const std::string var_name = kh_lower_copy(var_name_raw);
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
            report_error("khData.read: " + std::string(e.what()));
            return default_value.value_or(sol::nil);
        }
    }

    // KHData flush
    static sol::object flush_khdata() {
        try {
            KHDataManager::instance().flush_all();
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("khData.flush: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static sol::object delete_khdata_file(const std::string& filename) {
        try {
            KHDataManager::instance().delete_file(kh_lower_copy(filename));
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("khData.deleteFile: " + std::string(e.what()));
            return sol::nil;
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

        // PATH_CONFINE (search_mod_folders.hpp's note): the base library's dofile and loadfile read a file by a
        // path the script chooses - anywhere on the disk, or a network share - so a mission could read the
        // player's files as chunks (a parse error quotes them) or make the game open a share. Nothing here uses
        // them; Lua code arrives as text from SQF (load, loadstring stay).
        (*g_lua_state)["dofile"] = sol::nil;
        (*g_lua_state)["loadfile"] = sol::nil;

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
                report_error("print: " + std::string(e.what()));
            }
        };

        // Panic handler for unprotected errors
        g_lua_state->set_panic([](lua_State* L) -> int {
            const char* msg = lua_tostring(L, -1);
            report_error(std::string("KH Lua: panic: ") + (msg ? msg : "unknown"));
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
                        // This argument itself (the stack top is the LAST argument), as its text
                        // when Lua has one (a number), else its type name - lua_tostring is NULL for a nil, a table,
                        // a function or a userdata, and a NULL char* written to a stream is undefined.
                        lua_State* L = va.lua_state();
                        obj.push(L);
                        const char* text = lua_tostring(L, -1);
                        ss << (text ? text : lua_typename(L, lua_type(L, -1)));
                        lua_pop(L, 1);
                    }

                    ss << " ";
                }

                report_error(ss.str());
            } catch (const std::exception& e) {
                report_error("error: " + std::string(e.what()));
            }
        });
        
        // A C++ exception that escapes a bound function (the lua_wrappers_sqf.hpp wrappers have no try blocks: a
        // wrong-typed argument throws here) is reported, then becomes the Lua error of the call. Protected callers
        // that report their errors (luaExecute, event.trigger, ...) report it a second time; the ones that do not
        // (util.profile, util.execute, util.withSqf, the __toSQF path) would otherwise lose it.
        g_lua_state->set_exception_handler([](lua_State* L, sol::optional<const std::exception&> maybe_exception,
                                              sol::string_view description) -> int {
            report_error("KH Lua: " + (maybe_exception ? std::string(maybe_exception->what())
                                                       : std::string(description)));
            return sol::stack::push(L, description);
        });

        auto crypto_wrapper = [](const char* name, auto hash_func) {
            return [name, hash_func](const std::string& input) -> std::string {
                try {
                    return hash_func(input);
                } catch (const std::exception& e) {
                    report_error(std::string("crypto.") + name + ": " + e.what());
                    return "";
                }
            };
        };

        (*g_lua_state)["crypto"] = g_lua_state->create_table_with(
            "md5", crypto_wrapper("md5", CryptoGenerator::md5),
            "sha1", crypto_wrapper("sha1", CryptoGenerator::sha1),
            "sha256", crypto_wrapper("sha256", CryptoGenerator::sha256),
            "sha512", crypto_wrapper("sha512", CryptoGenerator::sha512),
            "fnv1a32", crypto_wrapper("fnv1a32", CryptoGenerator::fnv1a32),
            "fnv1a64", crypto_wrapper("fnv1a64", CryptoGenerator::fnv1a64),
            "crc32", crypto_wrapper("crc32", CryptoGenerator::crc32),
            "xxhash32", crypto_wrapper("xxhash32", CryptoGenerator::xxhash32),
            "adler32", crypto_wrapper("adler32", CryptoGenerator::adler32),
            "djb2", crypto_wrapper("djb2", CryptoGenerator::djb2),
            "sdbm", crypto_wrapper("sdbm", CryptoGenerator::sdbm)
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
        util_table["profile"] = LuaFunctions::profile_code;
        util_table["execute"] = LuaFunctions::execute_lua;
        util_table["generateRandomString"] = LuaFunctions::generate_random_string;
        util_table["generateUid"] = LuaFunctions::generate_uid;
        util_table["getDataType"] = LuaFunctions::get_data_type;
        util_table["withSqf"] = LuaFunctions::with_sqf;
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
                    return convert_game_value_to_lua(sqf::get_variable(sqf::current_namespace(), key.as<std::string>()));
                } catch (const std::exception& e) {
                    report_error("sqfVar.__index: " + std::string(e.what()));
                    return sol::nil;
                }
            },
            "__newindex", [](sol::object key, sol::object value) {
                try {
                    if (key.get_type() != sol::type::string) return;
                    sqf::set_variable(sqf::current_namespace(), key.as<std::string>(), convert_lua_to_game_value(value));
                } catch (const std::exception& e) {
                    report_error("sqfVar.__newindex: " + std::string(e.what()));
                }
            }
        );

        (*g_lua_state)["sqfVar"] = sqfVar;

        auto command_handler = [](std::string cmd, sol::variadic_args args) -> sol::object {
            try {
                LuaStackGuard guard(*g_lua_state);
                const size_t nargs = args.size();

                if (nargs > 2) {
                    report_error("sqf." + cmd + ": SQF commands take 0 to 2 arguments");
                    return sol::nil;
                }

                // Keyed by the arity too: the same name compiles to a nullary, unary or binary call.
                const std::string key = std::to_string(nargs) + ":" + cmd;
                auto cache_it = g_sqf_command_cache.find(key);
                code compiled;
                
                if (cache_it != g_sqf_command_cache.end()) {
                    compiled = cache_it->second;
                } else {
                    std::string full_command;

                    if (nargs == 0) {
                        full_command = "setReturnValue " + cmd;
                    } else if (nargs == 1) {
                        full_command = "setReturnValue (" + cmd + " getCallArguments);";
                    } else {
                        full_command = "private _khargs = getCallArguments; setReturnValue ((_khargs select 0) " + cmd +
                                       " (_khargs select 1));";
                    }

                    compiled = sqf::compile(full_command);
                    g_sqf_command_cache.emplace(key, compiled);
                }
                
                if (nargs == 0) {
                    return convert_game_value_to_lua(raw_call_sqf_native(compiled));
                } else if (nargs == 1) {
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
                report_error("sqf." + cmd + ": " + std::string(e.what()));
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
                report_error("sqf." + key + ": " + std::string(e.what()));
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
    g_local_exec_cache.clear();
    g_code_cache.clear();
    g_sqf_function_cache.clear();
    g_sqf_command_cache.clear();
    LuaFunctions::lua_scheduled_tasks.clear();
    LuaFunctions::lua_event_handlers.clear();
    LuaFunctions::next_task_id = 1;
    LuaFunctions::next_event_handler_id = 1;
}

static void reset_lua_state() {
    try {
        sqf::diag_log("KH Lua: resetting the state");
        clean_lua_state();
        g_lua_state.reset();
        initialize_lua_state();
        raw_call_sqf_native(g_compiled_sqf_trigger_lua_reset_event);
        sqf::diag_log("KH Lua: state reset");
    } catch (const std::exception& e) {
        report_error("KH Lua: failed to reset the state: " + std::string(e.what()));
    }
}