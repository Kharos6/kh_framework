#pragma once

#define NOMINMAX

#include <windows.h>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <unordered_map>
#include <shlobj.h>
#include <chrono>
#include <wincrypt.h>

#include "intercept/include/intercept.hpp"
#include "intercept/include/client/sqf/sqf.hpp"
#include "intercept/include/client/pointers.hpp"
#include "sol/sol.hpp"

using namespace intercept;
using namespace intercept::types;

static game_value g_compiled_sqf_trigger_cba_event;
static game_value g_compiled_sqf_add_game_event_handler;
static game_value g_compiled_sqf_remove_game_event_handler;
static game_value g_compiled_sqf_game_event_handler_lua_bridge;
static game_value g_compiled_sqf_execute_lua;
static game_value g_compiled_sqf_execute_sqf;
static game_value g_compiled_sqf_remove_handler;
static game_value g_compiled_sqf_create_hash_map_from_array;
static game_value g_compiled_sqf_create_hash_map;
static float g_game_time = 0.0f;
static int g_game_frame = 0;
static float g_mission_time = 0.0f;
static int g_mission_frame = 0;
static std::vector<std::vector<float>> g_terrain_matrix;
static float g_terrain_grid_width = 0.0f;
static int g_terrain_grid_size = 0;
static float g_world_size = 0.0f;

// Detect explicitly dedicated server
static bool get_machine_is_server() {
    static bool initialized = false;
    static bool is_server = false;
    
    if (!initialized) {
        char module_name[MAX_PATH];

        if (GetModuleFileNameA(NULL, module_name, MAX_PATH) != 0) {
            std::string exe_name = std::filesystem::path(module_name).filename().string();
            std::transform(exe_name.begin(), exe_name.end(), exe_name.begin(), ::tolower);
            is_server = exe_name == "arma3server_x64.exe" || exe_name == "arma3server.exe";
        }

        initialized = true;
    }

    return is_server;
}

static void report_error(const std::string& error_message) {
    sqf::diag_log(error_message);
    sqf::throw_exception(error_message);
}

static game_value raw_call_sqf_native(const code& code_obj) noexcept {
    static std::vector<game_value> arr;
    arr.clear();
    arr.push_back(game_value(std::vector<game_value>{}));
    auto&& result = sqf::apply(arr, code_obj);
    return result.empty() ? game_value() : std::move(result[0]);
}

static game_value raw_call_sqf_args_native(const code& code_obj, const game_value& args) noexcept {
    static std::vector<game_value> arr;
    arr.clear();
    arr.push_back(args);
    auto&& result = sqf::apply(arr, code_obj);
    return result.empty() ? game_value() : std::move(result[0]);
}

class RandomStringGenerator {
private:
    static std::mt19937& get_rng() {
        static std::mt19937 gen(std::random_device{}());
        return gen;
    }
    
public:
    static std::string generate(int length, bool use_numbers = true, 
                                bool use_letters = true, bool use_symbols = false) {
        if (length <= 0) return "";
        std::string charset;
        if (use_numbers) charset += "0123456789";
        if (use_letters) charset += "abcdefghijklmnopqrstuvwxyz";
        if (use_symbols) charset += "!@#$%^&*()_+-=[]{}|;:,.<>?/~`\\";
        
        if (charset.empty()) {
            charset = "0123456789abcdefghijklmnopqrstuvwxyz!@#$%^&*()_+-=[]{}|;:,.<>?/~`\\";
        }
        
        std::uniform_int_distribution<> dis(0, static_cast<int>(charset.size() - 1));
        std::string result;
        result.reserve(length);
        auto& gen = get_rng();

        for (int i = 0; i < length; i++) {
            result += charset[dis(gen)];
        }
        
        return result;
    }
};

class UIDGenerator {
private:
    static std::atomic<uint32_t> counter;
    static std::mt19937 rng;
    
    // Pre-computed hex lookup
    static constexpr char hex_chars[] = "0123456789abcdef";
    
    static inline void to_hex(char* buffer, uint32_t value) {
        for (int i = 7; i >= 0; --i) {
            buffer[i] = hex_chars[value & 0xF];
            value >>= 4;
        }
    }
    
public:
    static std::string generate() {
        static bool initialized = false;
        static uint32_t unique_machine_id;
        
        if (!initialized) {
            // Machine ID: combine multiple sources
            uint32_t pid = GetCurrentProcessId();
            
            // Get network adapter MAC address or computer name hash
            char computerName[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD size = sizeof(computerName);
            uint32_t name_hash = 0;

            if (GetComputerNameA(computerName, &size)) {
                for (DWORD i = 0; i < size; ++i) {
                    name_hash = name_hash * 31 + computerName[i];
                }
            }
            
            // Combine and mix bits
            unique_machine_id = (pid ^ name_hash ^ (name_hash >> 16));
            rng.seed(std::random_device{}() ^ unique_machine_id);
            initialized = true;
        }
        
        // Use actual timestamp, not relative
        uint32_t timestamp = static_cast<uint32_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()
        );
        
        // Bit distribution
        uint32_t part1 = (timestamp & 0xFFFFF) | ((unique_machine_id & 0xFFF) << 20);
        uint32_t part2 = counter.fetch_add(1, std::memory_order_relaxed);
        char buffer[17];
        to_hex(buffer, part1);
        to_hex(buffer + 8, part2);
        buffer[16] = '\0';
        return std::string(buffer, 16);
    }
};

std::atomic<uint32_t> UIDGenerator::counter{0};
std::mt19937 UIDGenerator::rng;

static void initialize_terrain_matrix() {
    try {
        static std::string cached_world;
        static float cached_grid_width = 0;
        std::string current_world = sqf::world_name();
        auto terrain_info = sqf::get_terrain_info();
        
        // Check if we need to recalculate
        if (current_world == cached_world && 
            terrain_info.terrain_grid_width == cached_grid_width &&
            !g_terrain_matrix.empty()) {
            
            // Matrix already calculated for this world
            return;
        }
        
        // Update cache keys
        cached_world = current_world;
        cached_grid_width = terrain_info.terrain_grid_width;
        
        // Store terrain info
        g_terrain_grid_width = terrain_info.terrain_grid_width;
        g_terrain_grid_size = terrain_info.terrain_grid_size;
        g_world_size = sqf::world_size();
        
        if (g_world_size <= 0 || g_terrain_grid_width <= 0) {
            return;
        }
        
        // Calculate grid dimensions
        int grid_points = static_cast<int>(g_world_size / g_terrain_grid_width) + 1;
        
        // Initialize matrix
        g_terrain_matrix.clear();
        g_terrain_matrix.reserve(grid_points);
        
        // Populate the matrix
        for (int y = 0; y < grid_points; y++) {
            std::vector<float> row;
            row.reserve(grid_points);
            float world_y = y * g_terrain_grid_width;
            
            for (int x = 0; x < grid_points; x++) {
                float world_x = x * g_terrain_grid_width;
                
                // Get terrain height at this position
                vector3 pos_atl(world_x, world_y, 0);
                vector3 pos_asl = sqf::atl_to_asl(pos_atl);
                float height = pos_asl.z;
                row.push_back(height);
            }
            
            g_terrain_matrix.push_back(std::move(row));
        }        
    } catch (const std::exception& e) {
        report_error("Failed to initialize terrain matrix: " + std::string(e.what()));
    } catch (...) {
        report_error("Unknown error initializing terrain matrix");
    }
}