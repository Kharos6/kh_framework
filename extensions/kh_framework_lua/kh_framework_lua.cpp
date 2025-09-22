#include <windows.h>
#include <string>
#include <memory>
#include <mutex>

#include "intercept/include/intercept.hpp"
#include "intercept/include/client/sqf/sqf.hpp"
#include "intercept/include/client/pointers.hpp"
#include "sol/sol.hpp"

// Global Lua state
static std::unique_ptr<sol::state> g_lua_state;
static std::mutex g_lua_mutex;
static int g_frame_counter = 0;

// Convert Intercept game_value to Sol2 Lua object
static sol::object convert_game_value_to_lua(const intercept::types::game_value& value) {
    if (!g_lua_state) {
        return sol::nil;
    }
    
    sol::state& lua = *g_lua_state;
    
    switch (value.type_enum()) {
        case intercept::types::game_data_type::SCALAR:
            return sol::make_object(lua, static_cast<float>(value));
            
        case intercept::types::game_data_type::BOOL:
            return sol::make_object(lua, static_cast<bool>(value));
            
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
        
        case intercept::types::game_data_type::OBJECT: {
            // For objects, we could return a special wrapper or just nil
            // For now, return nil or you could return object ID as string
            return sol::make_object(lua, sol::nil);
        }
        
        case intercept::types::game_data_type::NOTHING:
        case intercept::types::game_data_type::ANY:
            return sol::make_object(lua, sol::nil);
            
        default:
            // For unhandled types, return nil
            intercept::sqf::diag_log("Unhandled game_value type: " + std::to_string((int)value.type_enum()));
            return sol::make_object(lua, sol::nil);
    }
}

// Initialize Lua state
static void initialize_lua_state() {
    std::lock_guard<std::mutex> lock(g_lua_mutex);
    
    if (!g_lua_state) {
        g_lua_state = std::make_unique<sol::state>();
        
        // Open standard libraries
        g_lua_state->open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::string,
            sol::lib::math,
            sol::lib::table,
            sol::lib::bit32,
            sol::lib::jit
        );
        
        // Create sqf table for SQF functions
        auto sqf_table = g_lua_state->create_named_table("sqf");
        
        // Functions that don't return values
        sqf_table["diag_log"] = [](const std::string& string) {
            intercept::sqf::diag_log(string);
            return sol::nil;
        };

        sqf_table["call"] = [](const std::string& string) -> sol::object {
            intercept::types::game_value result = intercept::sqf::call2(intercept::sqf::compile(string));
            return convert_game_value_to_lua(result);
        };
    }
}

// Execute Lua code stack
static void execute_lua() {
    std::lock_guard<std::mutex> lock(g_lua_mutex);

    std::string lua_code = R"(
        local code_array = sqf.call(
            "missionNamespace getVariable ['KH_var_queuedLuaExecutions', []];"
        )

        for i, code_str in ipairs(code_array) do
            pcall(load(code_str))
        end
    )";
    
    auto result = g_lua_state->safe_script(lua_code);
    if (!result.valid()) {
        sol::error err = result;
        intercept::sqf::diag_log("Lua error: " + std::string(err.what()));
    }
}

static void execute_lua_code(const std::string& code, const intercept::types::game_value& data) {
    std::lock_guard<std::mutex> lock(g_lua_mutex);
    
    if (!g_lua_state) {
        intercept::sqf::diag_log("Error: Lua state not initialized");
        return;
    }
    
    // Convert data to Lua object and set it as a global variable
    (*g_lua_state)["_signal_data"] = convert_game_value_to_lua(data);
    
    // Execute the Lua code
    auto result = g_lua_state->safe_script(code);
    if (!result.valid()) {
        sol::error err = result;
        intercept::sqf::diag_log("Lua execution error: " + std::string(err.what()));
    }
    
    // Clear the signal data after execution
    (*g_lua_state)["_signal_data"] = sol::nil;
}

// Intercept API implementation
int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::register_interfaces() {
    // No interfaces to register
}

void intercept::pre_start() {
    // Initialize Lua state
    initialize_lua_state();
    intercept::sqf::diag_log("KH Framework Lua - Pre-start Complete");
}

void intercept::pre_init() {
    // Log that we're loaded
    intercept::sqf::diag_log("KH Framework Lua - Pre-init Complete");
}

void intercept::post_init() {
    // Post initialization
    intercept::sqf::diag_log("KH Framework Lua - Post-init Complete");
}

void intercept::mission_ended() {
    g_frame_counter = 0;
}

void intercept::on_frame() {
    g_frame_counter++;
    execute_lua();
}

void intercept::on_signal(std::string& signal_name_, intercept::types::game_value& value_) {
    // Handle different signal types
    if (signal_name_ == "execute") {
        // Direct code execution
        // value_ should be a string containing Lua code
        if (value_.type_enum() == intercept::types::game_data_type::STRING) {
            std::string code = static_cast<std::string>(value_);
            execute_lua_code(code, intercept::types::game_value());
        } else {
            intercept::sqf::diag_log("Error: 'execute' signal expects string data");
        }
    }
    else if (signal_name_ == "execute_with_data") {
        // Code execution with data
        // value_ should be an array: [code, data]
        if (value_.type_enum() == intercept::types::game_data_type::ARRAY) {
            auto& array = value_.to_array();
            if (array.size() >= 2 && array[0].type_enum() == intercept::types::game_data_type::STRING) {
                std::string code = static_cast<std::string>(array[0]);
                execute_lua_code(code, array[1]);
            }
        }
    }
    else if (signal_name_ == "process_queue") {
        // Process a queue of Lua code
        process_lua_queue(value_);
    }
    else if (signal_name_ == "reset") {
        // Reset the Lua state
        std::lock_guard<std::mutex> lock(g_lua_mutex);
        g_lua_state.reset();
        initialize_lua_state();
        intercept::sqf::diag_log("Lua state reset");
    }
    else {
        intercept::sqf::diag_log("Unknown signal: " + signal_name_);
    }
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            // Initialize on attach
            break;
            
        case DLL_PROCESS_DETACH:
            // Cleanup Lua state
            if (g_lua_state) {
                std::lock_guard<std::mutex> lock(g_lua_mutex);
                g_lua_state.reset();
            }
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}