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

// Forward declarations and variables
static float g_current_game_time = 0.0f;
static int g_current_game_frame = 0;
static float g_current_mission_time = 0.0f;
static int g_current_mission_frame = 0;
constexpr uint32_t KHDATA_MAGIC = 0x5444484B; // "KHDT"
constexpr uint32_t KHDATA_VERSION = 1;
static game_value g_trigger_cba_event;
static game_value g_add_game_event_handler;
static game_value g_remove_game_event_handler;
static game_value g_game_event_handler_bridge;
static game_value g_remote_execute;
static game_value g_create_hash_map_from_array;
static game_value g_create_hash_map;
static registered_sqf_function _execute_lua_sqf_command;
static registered_sqf_function _compile_lua_sqf_command;
static registered_sqf_function _crypto_hash_sqf_command;
static registered_sqf_function _generate_random_string_sqf_command;
static registered_sqf_function _generate_uid_sqf_command;
static registered_sqf_function _write_khdata_sqf_command;
static registered_sqf_function _read_khdata_sqf_command;
static registered_sqf_function _flush_khdata_sqf_command;
static registered_sqf_function _delete_khdata_file_sqf_command;
static registered_sqf_function _get_terrain_matrix_sqf_command;
static registered_sqf_function _trigger_lua_event_sqf_command;
static registered_sqf_function _trigger_lua_event_local_sqf_command;
static registered_sqf_function _emit_lua_variable_sqf_command;
static registered_sqf_function _lua_set_variable_sqf_command;
static registered_sqf_function _lua_get_variable_sqf_command;
static std::vector<std::vector<float>> g_terrain_matrix;
static float g_terrain_grid_width = 0.0f;
static int g_terrain_grid_size = 0;
static float g_world_size = 0.0f;

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

class CryptoProvider {
private:
    HCRYPTPROV hProv = 0;
    bool initialized = false;
    static CryptoProvider instance;
    
    CryptoProvider() {
        if (CryptAcquireContextW(&hProv, NULL, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
            CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
            CryptAcquireContextW(&hProv, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ||
            CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            initialized = true;
        }
    }
    
public:
    ~CryptoProvider() {
        if (initialized && hProv) {
            CryptReleaseContext(hProv, 0);
        }
    }
    
    static HCRYPTPROV get() {
        return instance.hProv;
    }
    
    static bool is_initialized() {
        return instance.initialized;
    }
};

CryptoProvider CryptoProvider::instance;

class CryptoGenerator {
private:
    // Pre-computed lookup table for hex conversion
    static constexpr char hex_chars[] = "0123456789abcdef";
    
    // Inline hex conversion for maximum speed
    template<typename T>
    
    static inline void append_hex(std::string& result, T value) {
        for (size_t i = 0; i < sizeof(T); i++) {
            uint8_t byte = (value >> (i * 8)) & 0xFF;
            result.push_back(hex_chars[byte >> 4]);
            result.push_back(hex_chars[byte & 0x0F]);
        }
    }

public:
    static std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        for (uint8_t byte : bytes) {
            ss << std::setw(2) << static_cast<int>(byte);
        }

        return ss.str();
    }

    // Windows CryptoAPI hash wrapper
    static std::string windows_hash(const std::string& input, ALG_ID algorithm) {
        HCRYPTPROV hProv = CryptoProvider::get();

        if (!hProv || !CryptoProvider::is_initialized()) {
            report_error("CryptoProvider not initialized - Windows cryptography unavailable");
            return "";
        }
            
        HCRYPTHASH hHash = 0;

        if (!CryptCreateHash(hProv, algorithm, 0, 0, &hHash)) {
            report_error("Failed to create hash context for algorithm " + std::to_string(algorithm));
            return "";
        }
        
        if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.c_str()), 
                        static_cast<DWORD>(input.length()), 0)) {
            CryptDestroyHash(hHash);
            report_error("Failed to hash data");
            return "";
        }
        
        DWORD hashSize = 0;
        DWORD sizeSize = sizeof(DWORD);

        if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashSize), &sizeSize, 0)) {
            CryptDestroyHash(hHash);
            report_error("Failed to get hash size");
            return "";
        }
        
        std::vector<uint8_t> hashData(hashSize);

        if (!CryptGetHashParam(hHash, HP_HASHVAL, hashData.data(), &hashSize, 0)) {
            CryptDestroyHash(hHash);
            report_error("Failed to get hash value");
            return "";
        }
        
        CryptDestroyHash(hHash);
        return bytes_to_hex(hashData);
    }

    // Hash implementations
    static std::string md5(const std::string& input) {
        return windows_hash(input, CALG_MD5);
    }

    static std::string sha1(const std::string& input) {
        return windows_hash(input, CALG_SHA1);
    }

    static std::string sha256(const std::string& input) {
        return windows_hash(input, CALG_SHA_256);
    }

    static std::string sha512(const std::string& input) {
        return windows_hash(input, CALG_SHA_512);
    }

    static std::string fnv1a32(const std::string& input) {
        uint32_t hash = 0x811c9dc5u;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
   
        while (data + 4 <= end) {
            hash ^= data[0];
            hash *= 0x01000193u;
            hash ^= data[1];
            hash *= 0x01000193u;
            hash ^= data[2];
            hash *= 0x01000193u;
            hash ^= data[3];
            hash *= 0x01000193u;
            data += 4;
        }

        // Handle remaining bytes
        while (data != end) {
            hash ^= *data++;
            hash *= 0x01000193u;
        }
        
        std::string result;
        result.reserve(8);
        append_hex(result, hash);
        return result;
    }

    static std::string fnv1a64(const std::string& input) {
        uint64_t hash = 0xcbf29ce484222325ull;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
        
        while (data + 8 <= end) {
            uint64_t chunk;
            memcpy(&chunk, data, 8);
            hash ^= chunk;
            hash *= 0x100000001b3ull;
            data += 8;
        }

        while (data != end) {
            hash ^= *data++;
            hash *= 0x100000001b3ull;
        }
        
        std::string result;
        result.reserve(16);
        append_hex(result, hash);
        return result;
    }

    static std::string crc32(const std::string& input) {
        // Static table initialization without nested function
        static uint32_t crc_table[256];
        static bool table_initialized = false;
        
        if (!table_initialized) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t crc = i;

                for (int j = 0; j < 8; j++) {
                    crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
                }

                crc_table[i] = crc;
            }

            table_initialized = true;
        }
        
        uint32_t crc = 0xFFFFFFFFu;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
        
        // Process 4 bytes at a time for better performance on longer strings
        while (data + 4 <= end) {
            crc = crc_table[(crc ^ data[0]) & 0xFF] ^ (crc >> 8);
            crc = crc_table[(crc ^ data[1]) & 0xFF] ^ (crc >> 8);
            crc = crc_table[(crc ^ data[2]) & 0xFF] ^ (crc >> 8);
            crc = crc_table[(crc ^ data[3]) & 0xFF] ^ (crc >> 8);
            data += 4;
        }
        
        // Handle remaining bytes
        while (data != end) {
            crc = crc_table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        }
        
        crc ^= 0xFFFFFFFFu;
        std::string result;
        result.reserve(8);
        append_hex(result, crc);
        return result;
    }

    static std::string xxhash32(const std::string& input) {
        const uint32_t PRIME32_1 = 0x9E3779B1u;
        const uint32_t PRIME32_2 = 0x85EBCA77u;
        const uint32_t PRIME32_3 = 0xC2B2AE3Du;
        const uint32_t PRIME32_4 = 0x27D4EB2Fu;
        const uint32_t PRIME32_5 = 0x165667B1u;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        size_t len = input.length();
        const uint8_t* end = data + len;
        uint32_t seed = 0;
        uint32_t hash;
        
        if (len >= 16) {
            const uint8_t* limit = end - 16;
            uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
            uint32_t v2 = seed + PRIME32_2;
            uint32_t v3 = seed;
            uint32_t v4 = seed - PRIME32_1;
            
            do {
                // Process 16 bytes at a time (4x4 bytes)
                uint32_t k1, k2, k3, k4;
                memcpy(&k1, data, 4);
                memcpy(&k2, data + 4, 4);
                memcpy(&k3, data + 8, 4);
                memcpy(&k4, data + 12, 4);
                v1 += k1 * PRIME32_2;
                v1 = ((v1 << 13) | (v1 >> 19)) * PRIME32_1;
                v2 += k2 * PRIME32_2;
                v2 = ((v2 << 13) | (v2 >> 19)) * PRIME32_1;
                v3 += k3 * PRIME32_2;
                v3 = ((v3 << 13) | (v3 >> 19)) * PRIME32_1;
                v4 += k4 * PRIME32_2;
                v4 = ((v4 << 13) | (v4 >> 19)) * PRIME32_1;
                data += 16;
            } while (data <= limit);
            
            hash = ((v1 << 1) | (v1 >> 31)) + 
                ((v2 << 7) | (v2 >> 25)) + 
                ((v3 << 12) | (v3 >> 20)) + 
                ((v4 << 18) | (v4 >> 14));
        } else {
            hash = seed + PRIME32_5;
        }
        
        hash += static_cast<uint32_t>(len);
        
        // Process remaining 4-byte chunks
        while (data + 4 <= end) {
            uint32_t k1;
            memcpy(&k1, data, 4);
            k1 *= PRIME32_3;
            k1 = (k1 << 17) | (k1 >> 15);
            k1 *= PRIME32_4;
            hash ^= k1;
            hash = ((hash << 19) | (hash >> 13)) * PRIME32_1 + PRIME32_4;
            data += 4;
        }
        
        // Process remaining bytes
        while (data < end) {
            uint32_t k1 = *data++;
            k1 *= PRIME32_5;
            k1 = (k1 << 11) | (k1 >> 21);
            k1 *= PRIME32_1;
            hash ^= k1;
            hash = ((hash << 15) | (hash >> 17)) * PRIME32_2 + PRIME32_3;
        }
        
        hash ^= hash >> 16;
        hash *= PRIME32_2;
        hash ^= hash >> 13;
        hash *= PRIME32_3;
        hash ^= hash >> 16;
        std::string result;
        result.reserve(8);
        append_hex(result, hash);
        return result;
    }

    static std::string adler32(const std::string& input) {
        // Use larger modulo operations less frequently for better performance
        const uint32_t MOD_ADLER = 65521;
        const size_t NMAX = 5552; // Largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1
        uint32_t a = 1, b = 0;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        size_t len = input.length();
        
        while (len > 0) {
            size_t k = len < NMAX ? len : NMAX;
            len -= k;
            
            // Unroll loop for better performance
            while (k >= 16) {
                a += data[0]; b += a;
                a += data[1]; b += a;
                a += data[2]; b += a;
                a += data[3]; b += a;
                a += data[4]; b += a;
                a += data[5]; b += a;
                a += data[6]; b += a;
                a += data[7]; b += a;
                a += data[8]; b += a;
                a += data[9]; b += a;
                a += data[10]; b += a;
                a += data[11]; b += a;
                a += data[12]; b += a;
                a += data[13]; b += a;
                a += data[14]; b += a;
                a += data[15]; b += a;
                data += 16;
                k -= 16;
            }
            
            while (k-- > 0) {
                a += *data++;
                b += a;
            }
            
            a %= MOD_ADLER;
            b %= MOD_ADLER;
        }
        
        uint32_t checksum = (b << 16) | a;
        std::string result;
        result.reserve(8);
        append_hex(result, checksum);
        return result;
    }

    static std::string djb2(const std::string& input) {
        uint32_t hash = 5381;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
        
        // Unroll loop for better performance
        while (data + 4 <= end) {
            hash = ((hash << 5) + hash) + data[0];
            hash = ((hash << 5) + hash) + data[1];
            hash = ((hash << 5) + hash) + data[2];
            hash = ((hash << 5) + hash) + data[3];
            data += 4;
        }
        
        while (data != end) {
            hash = ((hash << 5) + hash) + *data++;
        }
        
        std::string result;
        result.reserve(8);
        append_hex(result, hash);
        return result;
    }

    static std::string sdbm(const std::string& input) {
        uint32_t hash = 0;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
        
        // The SDBM algorithm with loop unrolling
        while (data + 4 <= end) {
            hash = data[0] + (hash << 6) + (hash << 16) - hash;
            hash = data[1] + (hash << 6) + (hash << 16) - hash;
            hash = data[2] + (hash << 6) + (hash << 16) - hash;
            hash = data[3] + (hash << 6) + (hash << 16) - hash;
            data += 4;
        }
        
        while (data != end) {
            hash = *data++ + (hash << 6) + (hash << 16) - hash;
        }
        
        std::string result;
        result.reserve(8);
        append_hex(result, hash);
        return result;
    }
};

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
    
    // Fast hex conversion without sprintf
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

class KHDataFile {
public:
    enum class DirtyState {
        Clean,
        Modified,
        SaveFailed,
        SizeExceeded
    };

private:
    std::unordered_map<std::string, game_value> variables;
    std::string filename;
    std::filesystem::path filepath;
    DirtyState dirty_state = DirtyState::Clean;
    int failed_save_attempts = 0;
    static constexpr int MAX_SAVE_ATTEMPTS = 3;
    std::chrono::steady_clock::time_point last_modified;
    std::chrono::steady_clock::time_point last_save_attempt;

    void mark_dirty() {
        if (dirty_state == DirtyState::Clean) {
            dirty_state = DirtyState::Modified;
            last_modified = std::chrono::steady_clock::now();
        }
    }

public:
    KHDataFile(const std::string& name) : filename(name), dirty_state(DirtyState::Clean) {}

    game_value read_variable(const std::string& var_name) const {
        auto it = variables.find(var_name);

        if (it != variables.end()) {
            return it->second;
        }

        return game_value();
    }

    bool write_variable(const std::string& var_name, const game_value& value) {        
        // Store old value for rollback
        game_value old_value;
        bool had_old_value = false;
        auto it = variables.find(var_name);

        if (it != variables.end()) {
            old_value = it->second;
            had_old_value = true;
        }
        
        try {
            if (value.is_nil()) {
                if (had_old_value) {
                    variables.erase(var_name);
                    mark_dirty();
                    return true;
                }

                return false;
            }
            
            variables[var_name] = value;
            mark_dirty();
            return true;
            
        } catch (...) {
            // Rollback on failure
            if (had_old_value) {
                variables[var_name] = old_value;
            } else {
                variables.erase(var_name);
            }

            throw;
        }
    }

    bool delete_variable(const std::string& var_name) {
        auto it = variables.find(var_name);

        if (it != variables.end()) {
            variables.erase(it);
            mark_dirty();
            return true;
        }

        return false;
    }

    std::vector<std::string> get_variable_names() const {
        std::vector<std::string> names;
        names.reserve(variables.size());

        for (const auto& [name, value] : variables) {
            names.push_back(name);
        }

        return names;
    }

    bool needs_save() const { 
        return dirty_state == DirtyState::Modified && failed_save_attempts < MAX_SAVE_ATTEMPTS; 
    }
    
    void mark_save_failed() { 
        dirty_state = DirtyState::SaveFailed; 
        failed_save_attempts++;
        last_save_attempt = std::chrono::steady_clock::now();
    }
    
    void mark_size_exceeded() { 
        dirty_state = DirtyState::SizeExceeded; 
    }
    
    void clear_dirty() { 
        dirty_state = DirtyState::Clean;
        failed_save_attempts = 0;
    }
    
    void set_filepath(const std::filesystem::path& path) { filepath = path; }
    const std::filesystem::path& get_filepath() const { return filepath; }
    const std::unordered_map<std::string, game_value>& get_variables() const { return variables; }
    
    void set_variables(std::unordered_map<std::string, game_value>&& vars) {
        variables = std::move(vars);
        clear_dirty(); // Explicitly mark as clean after loading
    }
};

class KHDataManager {
private:
    std::unordered_map<std::string, std::unique_ptr<KHDataFile>> files;
    std::filesystem::path base_path;
    bool initialized = false;
    KHDataManager() = default;
    
    // Binary serialization helpers
    static void write_string(std::ofstream& stream, const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.length());
        stream.write(reinterpret_cast<const char*>(&len), sizeof(len));

        if (!stream.good()) {
            report_error("Failed to write type");
            throw std::runtime_error("Failed to write type");
        }

        if (len > 0) {
            stream.write(str.c_str(), len);

            if (!stream.good()) {
                report_error("Failed to write type");
                throw std::runtime_error("Failed to write type");
            }
        }
    }
    
    static std::string read_string(std::ifstream& stream) {
        uint32_t len;
        stream.read(reinterpret_cast<char*>(&len), sizeof(len));

        if (!stream.good()) {
            report_error("Failed to read string length");
            throw std::runtime_error("Failed to read string length");
        }

        std::string str(len, '\0');
        stream.read(&str[0], len);

        if (!stream.good() && len > 0) {
            report_error("Failed to read string data");
            throw std::runtime_error("Failed to read string data");
        }

        return str;
    }
    
    static void write_game_value(std::ofstream& stream, const game_value& value) {
        auto type = value.type_enum();
        
        // Check if THIS specific value needs special handling (not its contents)
        bool needs_special_handling = false;
        switch (type) {
            case game_data_type::OBJECT:
            case game_data_type::GROUP:
            case game_data_type::CODE:
            case game_data_type::NAMESPACE:
            case game_data_type::SIDE:
            case game_data_type::TEXT:
            case game_data_type::CONFIG:
            case game_data_type::LOCATION:
            case game_data_type::TEAM_MEMBER:
            case game_data_type::DISPLAY:
                needs_special_handling = true;
                break;
            case game_data_type::CONTROL:
            case game_data_type::SCRIPT:
            case game_data_type::TASK:
            case game_data_type::DIARY_RECORD:
            case game_data_type::NetObject:
            case game_data_type::SUBGROUP:
            case game_data_type::TARGET:
                report_error("Cannot serialize unsupported type: " + std::to_string((int)type));
                throw std::runtime_error("Cannot serialize unsupported type: " + std::to_string((int)type));
            default:
                break;
        }
            
        // Write a special marker for serialized types
        stream.write(reinterpret_cast<const char*>(&needs_special_handling), sizeof(bool));

        if (!stream.good()) {
            report_error("Failed to write type");
            throw std::runtime_error("Failed to write type");
        }
        
        if (needs_special_handling) {
            // Write the original type for reference
            stream.write(reinterpret_cast<const char*>(&type), sizeof(type));

            if (!stream.good()) {
                report_error("Failed to write type");
                throw std::runtime_error("Failed to write type");
            }
            
            // Serialize as string based on type
            std::string serialized;

            switch (type) {
                case game_data_type::CODE: {
                    // Store code as string, don't need to check for curly braces
                    std::string code_str = static_cast<std::string>(value);
                    serialized = code_str;
                    break;
                }
                case game_data_type::NAMESPACE: {
                    // Store namespace identifier
                    if (value == sqf::mission_namespace()) {
                        serialized = "missionNamespace";
                    } else if (value == sqf::profile_namespace()) {
                        serialized = "profileNamespace";
                    } else if (value == sqf::ui_namespace()) {
                        serialized = "uiNamespace";
                    } else if (value == sqf::parsing_namespace()) {
                        serialized = "parsingNamespace";
                    } else if (value == sqf::server_namespace()) {
                        serialized = "serverNamespace";
                    } else if (value == sqf::mission_profile_namespace()) {
                        serialized = "missionProfileNamespace";
                    } else {
                        serialized = "missionNamespace"; // Default fallback
                    }

                    break;
                }
                case game_data_type::SIDE: {
                    // Store side identifier
                    if (value == sqf::west()) {
                        serialized = "west";
                    } else if (value == sqf::blufor()) {
                        serialized = "blufor";
                    } else if (value == sqf::east()) {
                        serialized = "east";
                    } else if (value == sqf::opfor()) {
                        serialized = "opfor";
                    } else if (value == sqf::resistance()) {
                        serialized = "resistance";
                    } else if (value == sqf::independent()) {
                        serialized = "independent";
                    } else if (value == sqf::civilian()) {
                        serialized = "civilian";
                    } else if (value == sqf::side_logic()) {
                        serialized = "sideLogic";
                    } else if (value == sqf::side_unknown()) {
                        serialized = "sideUnknown";
                    } else if (value == sqf::side_enemy()) {
                        serialized = "sideEnemy";
                    } else if (value == sqf::side_friendly()) {
                        serialized = "sideFriendly";
                    } else if (value == sqf::side_ambient_life()) {
                        serialized = "sideAmbientLife";
                    } else if (value == sqf::side_empty()) {
                        serialized = "sideEmpty";
                    } else {
                        serialized = "sideUnknown";
                    }

                    break;
                }
                case game_data_type::GROUP: {
                    // Store group ID
                    serialized = static_cast<std::string>(sqf::group_id(static_cast<group>(value)));
                    break;
                }
                case game_data_type::OBJECT: {
                    // Store vehicle var name or hash
                    serialized = static_cast<std::string>(sqf::vehicle_var_name(value));

                    if (serialized.empty()) {
                        serialized = "UNREGISTERED_OBJECT";
                    }
                    
                    break;
                }
                case game_data_type::TEXT: {
                    // Convert structured text to string representation
                    std::string text_str = static_cast<std::string>(value);
                    serialized = text_str;
                    break;
                }
                case game_data_type::CONFIG: {
                    // Get config hierarchy: [bin\config.bin, bin\config.bin/CfgVehicles, ...]
                    auto hierarchy = sqf::config_hierarchy(value);
                    
                    if (hierarchy.size() <= 1) {
                        // Just root or empty, store as configFile
                        serialized = "configFile";
                    } else {
                        // Skip first element, build path with >> separators
                        std::string config_path = "configFile";
                        
                        for (size_t i = 1; i < hierarchy.size(); i++) {
                            std::string entry = static_cast<std::string>(hierarchy[i]);

                            // Extract the part after the last '/'
                            size_t last_slash = entry.find_last_of('/');

                            if (last_slash != std::string::npos) {
                                entry = entry.substr(last_slash + 1);
                            }
                            config_path += " >> " + entry;
                        }

                        serialized = config_path;
                    }
                    break;
                }

                case game_data_type::LOCATION: {
                    // Store location name
                    std::string location_name = static_cast<std::string>(sqf::class_name(value));                    
                    serialized = location_name.empty() ? "LOCATION_NULL" : location_name;
                    break;
                }

                case game_data_type::TEAM_MEMBER: {
                    // Get the agent object from team member
                    game_value agent_obj = sqf::agent(value);
                    
                    // Store agent's vehicle var name or hash (same as OBJECT logic)
                    serialized = static_cast<std::string>(sqf::vehicle_var_name(agent_obj));
                    
                    if (serialized.empty()) {
                        if (agent_obj.data) {
                            std::string obj_str = static_cast<std::string>(agent_obj.data->to_string());
                            
                            if (!obj_str.empty()) {
                                serialized = "AGENT_HASH_" + agent_obj.get_hash();
                            } else {
                                serialized = "AGENT_HASH_NULL";
                            }
                        } else {
                            serialized = "AGENT_HASH_INVALID";
                        }
                    }

                    break;
                }

                case game_data_type::DISPLAY: {
                    int idd = static_cast<int>(sqf::ctrl_idd(value));
                    serialized = "DISPLAY_" + std::to_string(idd);
                    break;
                }

                default:
                    report_error("Cannot serialize game value type: " + std::to_string((int)type));
                    throw std::runtime_error("Cannot serialize game value type: " + std::to_string((int)type));
            }
            
            write_string(stream, serialized);
        } else {
            // Write type and flags
            stream.write(reinterpret_cast<const char*>(&type), sizeof(type));

            if (!stream.good()) {
                report_error("Failed to write type");
                throw std::runtime_error("Failed to write type");
            }
            
            // Write value normally for simple types
            switch (type) {
                case game_data_type::NOTHING:
                case game_data_type::ANY:
                    break;
                case game_data_type::SCALAR: {
                    float val = value;
                    stream.write(reinterpret_cast<const char*>(&val), sizeof(val));
                    break;
                }
                case game_data_type::BOOL: {
                    bool val = value;
                    stream.write(reinterpret_cast<const char*>(&val), sizeof(val));
                    break;
                }
                case game_data_type::STRING: {
                    std::string str_val = static_cast<std::string>(value);
                    write_string(stream, str_val);
                    break;
                }
                case game_data_type::ARRAY: {
                    auto& arr = value.to_array();
                    uint32_t size = static_cast<uint32_t>(arr.size());
                    stream.write(reinterpret_cast<const char*>(&size), sizeof(size));

                    for (const auto& elem : arr) {
                        write_game_value(stream, elem);  // Each element handles its own serialization
                    }

                    break;
                }                
                case game_data_type::HASHMAP: {
                    // Manually convert hashmap to array of [key, value] pairs
                    auto& map = value.to_hashmap();
                    uint32_t size = static_cast<uint32_t>(map.count());
                    stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
                    
                    for (const auto& pair : map) {
                        // Create [key, value] array
                        auto_array<game_value> kv_pair;
                        kv_pair.push_back(pair.key);
                        kv_pair.push_back(pair.value);
                        
                        // Write the pair as an array
                        write_game_value(stream, game_value(std::move(kv_pair)));
                    }

                    break;
                }
                default:
                    break;
            }

            if (!stream.good()) {
                report_error("Failed to write type");
                throw std::runtime_error("Failed to write type");
            }
        }
    }
    
    static game_value read_game_value(std::ifstream& stream) {
        bool is_serialized;
        stream.read(reinterpret_cast<char*>(&is_serialized), sizeof(bool));
        
        if (is_serialized) {
            // Read the original type
            game_data_type original_type;
            stream.read(reinterpret_cast<char*>(&original_type), sizeof(original_type));
            
            // Read the serialized string
            std::string serialized = read_string(stream);
            
            // Reconstruct based on type
            switch (original_type) {
                case game_data_type::CODE:
                    return sqf::compile(serialized);
                
                case game_data_type::NAMESPACE:
                    if (serialized == "missionNamespace") return sqf::mission_namespace();
                    if (serialized == "profileNamespace") return sqf::profile_namespace();
                    if (serialized == "uiNamespace") return sqf::ui_namespace();
                    if (serialized == "parsingNamespace") return sqf::parsing_namespace();
                    if (serialized == "serverNamespace") return sqf::server_namespace();
                    if (serialized == "missionProfileNamespace") return sqf::mission_profile_namespace();
                    return sqf::mission_namespace();
                
                case game_data_type::SIDE:
                    if (serialized == "west") return sqf::west();
                    if (serialized == "blufor") return sqf::blufor();
                    if (serialized == "east") return sqf::east();
                    if (serialized == "opfor") return sqf::opfor();
                    if (serialized == "resistance") return sqf::resistance();
                    if (serialized == "independent") return sqf::resistance();
                    if (serialized == "civilian") return sqf::civilian();
                    if (serialized == "sideLogic") return sqf::side_logic();
                    if (serialized == "sideUnknown") return sqf::side_unknown();
                    if (serialized == "sideEnemy") return sqf::side_enemy();
                    if (serialized == "sideFriendly") return sqf::side_friendly();
                    if (serialized == "sideAmbientLife") return sqf::side_ambient_life();
                    if (serialized == "sideEmpty") return sqf::side_empty();
                    return sqf::side_unknown();
                
                case game_data_type::GROUP: {
                    // Find group by ID
                    auto all_groups = sqf::all_groups();

                    for (const auto& grp : all_groups) {
                        if (static_cast<std::string>(sqf::group_id(static_cast<group>(grp))) == serialized) {
                            return grp;
                        }
                    }

                    return sqf::grp_null(); // Group not found
                }
                
                case game_data_type::OBJECT: {
                    return sqf::get_variable(sqf::mission_namespace(), serialized, sqf::obj_null());
                }

                case game_data_type::TEXT: {
                    // Use parseText to reconstruct structured text from string
                    // If the string is empty, return a default TEXT
                    if (serialized.empty()) {
                        return sqf::parse_text("");
                    }
                    
                    return sqf::parse_text(serialized);
                }

                case game_data_type::CONFIG: {
                    if (serialized == "configFile" || serialized.empty()) {
                        return sqf::config_file();
                    }
                    
                    // Compile and call the config path string
                    game_value compiled = sqf::compile(serialized);
                    return sqf::call2(compiled);
                }

                case game_data_type::LOCATION: {
                    if (serialized == "LOCATION_NULL" || serialized.empty()) {
                        return sqf::location_null();
                    }
                    
                    // Get world size for search radius
                    float world_size = sqf::world_size();
                    vector3 center(world_size / 2.0f, world_size / 2.0f, 0.0f);
                    float radius = world_size * std::sqrt(2.0f) / 2.0f;
                    
                    // Get all locations
                    std::vector<std::string> all_types;
                    auto all_locations = sqf::nearest_locations(center, all_types, radius);
                    
                    // Search for matching location by className
                    for (const auto& loc : all_locations) {                        
                        if (static_cast<std::string>(sqf::class_name(loc)) == serialized) {
                            return loc;
                        }
                    }
                    
                    // Not found
                    return sqf::location_null();
                }

                case game_data_type::TEAM_MEMBER: {
                    // Named agent - look for variable
                    game_value agent_obj = sqf::get_variable(sqf::mission_namespace(), serialized, sqf::obj_null());
                    
                    if (agent_obj.is_null()) {
                        return sqf::team_member_null();
                    }
                    
                    return sqf::agent(agent_obj);
                }

                case game_data_type::DISPLAY: {
                    if (serialized.substr(0, 8) != "DISPLAY_") {
                        return sqf::display_null();
                    }
                    
                    // Extract IDD
                    std::string idd_str = serialized.substr(8);
                    int idd;
                    
                    try {
                        idd = std::stoi(idd_str);
                    } catch (...) {
                        return sqf::display_null();
                    }
                    
                    // Try to find the display
                    display result = sqf::find_display(idd);
                    
                    // findDisplay returns displayNull if not found, which is fine
                    return result;
                }

                default:
                    // For unsupported serialized types, return nil
                    return game_value();
            }
        } else {
            // Read type normally
            game_data_type type;
            stream.read(reinterpret_cast<char*>(&type), sizeof(type));
            
            switch (type) {
                case game_data_type::NOTHING:
                case game_data_type::ANY:
                    return game_value();
                case game_data_type::SCALAR: {
                    float val;
                    stream.read(reinterpret_cast<char*>(&val), sizeof(val));
                    return game_value(val);
                }
                case game_data_type::BOOL: {
                    bool val;
                    stream.read(reinterpret_cast<char*>(&val), sizeof(val));
                    return game_value(val);
                }
                case game_data_type::STRING:
                    return game_value(read_string(stream));
                case game_data_type::ARRAY: {
                    uint32_t size;
                    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
                    auto_array<game_value> arr;
                    arr.reserve(size);

                    for (uint32_t i = 0; i < size; i++) {
                        arr.push_back(read_game_value(stream));
                    }
                    
                    return game_value(std::move(arr));
                }
                case game_data_type::HASHMAP: {
                    uint32_t size;
                    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
                    auto_array<game_value> arr;
                    arr.reserve(size);

                    for (uint32_t i = 0; i < size; i++) {
                        arr.push_back(read_game_value(stream));
                    }

                    return sqf::call2(g_create_hash_map_from_array, game_value(std::move(arr)));
                }
                default:
                    return game_value();
            }
        }
    }

    static bool validate_filename(const std::string& filename) {
        // Check empty or too long
        if (filename.empty() || filename.length() > 255) {
            return false;
        }
        
        // Check for path traversal attempts
        if (filename.find("..") != std::string::npos ||
            filename.find("/") != std::string::npos ||
            filename.find("\\") != std::string::npos ||
            filename.find(":") != std::string::npos) {
            return false;
        }
        
        // Check for dangerous characters
        if (filename.find_first_of("*?<>|\"\0", 0) != std::string::npos) {
            return false;
        }
        
        // Check for control characters or non-ASCII
        for (char c : filename) {
            if (c < 32 || c == 127 || (unsigned char)c > 127) {
                return false;
            }
        }
        
        // Check for leading/trailing dots or spaces
        if (filename.front() == '.' || filename.front() == ' ' ||
            filename.back() == '.' || filename.back() == ' ') {
            return false;
        }
        
        // Extract base name without extension for reserved name check
        std::string base_name = filename;
        size_t dot_pos = filename.rfind('.');

        if (dot_pos != std::string::npos) {
            base_name = filename.substr(0, dot_pos);
        }
        
        // Convert to uppercase for comparison
        std::string upper = base_name;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        
        // Check Windows reserved device names
        if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL") {
            return false;
        }
        
        // Check COM1-COM9 and LPT1-LPT9
        if (upper.length() == 4) {
            if ((upper.substr(0, 3) == "COM" && upper[3] >= '1' && upper[3] <= '9') ||
                (upper.substr(0, 3) == "LPT" && upper[3] >= '1' && upper[3] <= '9')) {
                return false;
            }
        }
        
        return true;
    }

    static constexpr size_t MAX_KHDATA_FILES = 1024;
    static constexpr size_t MAX_TOTAL_KHDATA_SIZE = 1024LL * 1024LL * 1024LL;
    size_t total_data_size = 0;
public:
    static KHDataManager& instance() {
        static KHDataManager inst;
        return inst;
    }

    bool initialize() {
        if (initialized) return true;
        
        // Get Documents\Arma 3\kh_framework\kh_data path
        char docs_path[MAX_PATH];

        if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs_path) != S_OK) {
            return false;
        }
        
        base_path = std::filesystem::path(docs_path) / "Arma 3" / "kh_framework" / "kh_data";
        
        // Create directories if they don't exist
        try {
            std::filesystem::create_directories(base_path);
        } catch (...) {
            return false;
        }
        
        // Load all existing .khdata files
        try {
            for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
                if (entry.path().extension() == ".khdata") {
                    std::string filename = entry.path().stem().string();
                    load_file(filename);
                }
            }

            update_total_size(); // Calculate initial total size
        } catch (...) {
            // Directory iteration failed, but initialization can continue
        }

        try {
            for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
                auto ext = entry.path().extension();
                
                if (ext == ".tmp" || ext == ".backup") {
                    try {
                        std::filesystem::remove(entry.path());
                    } catch (...) {
                        // Non-critical, continue
                    }
                }
            }
        } catch (...) {
            // Directory iteration failed, non-critical
        }
        
        initialized = true;
        return true;
    }

    size_t estimate_file_size(KHDataFile* file) {
        if (!file) return 0;
        size_t size = sizeof(uint32_t) * 3; // Header size (magic, version, var_count)
        
        for (const auto& [name, value] : file->get_variables()) {
            size += sizeof(uint32_t) + name.length(); // Variable name
            size += calculate_value_size(value);      // Variable value
        }
        
        // Add 10% overhead for potential alignment and metadata
        size = static_cast<size_t>(size * 1.1);
        
        if (size > MAX_TOTAL_KHDATA_SIZE && (!get_machine_is_server())) {
            file->mark_size_exceeded();
            report_error("File " + file->get_filepath().string() + 
                                    " exceeds size limit: " + std::to_string(size));
        }
        
        return size;
    }
    
    void update_total_size() {
        total_data_size = 0;

        for (const auto& [name, file] : files) {
            total_data_size += estimate_file_size(file.get());
        }
    }

    KHDataFile* get_or_create_file(const std::string& filename) {        
        // Validate filename
        if (!validate_filename(filename)) {
            report_error("Invalid filename for backup load: " + filename);
            return nullptr;
        }
        
        auto it = files.find(filename);
        
        if (it != files.end()) {
            return it->second.get();
        }
        
        // Try to load from disk first
        if (load_file(filename)) {
            auto it2 = files.find(filename);

            if (it2 != files.end()) {
                return it2->second.get();
            }
        }
        
        if (!get_machine_is_server()) {
            // Check file limit before creating new
            if (files.size() >= MAX_KHDATA_FILES) {
                return nullptr;  // Exceeded file limit
            }
            
            // Estimate for new empty file
            if (total_data_size >= MAX_TOTAL_KHDATA_SIZE) {
                return nullptr; // Would likely exceed size limit
            }
        }
        
        // Create new file
        auto file = std::make_unique<KHDataFile>(filename);
        file->set_filepath(base_path / (filename + ".khdata"));
        auto* ptr = file.get();
        files[filename] = std::move(file);
        return ptr;
    }

    bool load_file(const std::string& filename) {
        auto filepath = base_path / (filename + ".khdata");
        auto backup_path = filepath;
        backup_path += ".backup";

        if (!validate_filename(filename)) {
            report_error("Invalid filename: " + filename);
            return false;
        }

        if (!std::filesystem::exists(filepath)) {
            return false;
        }

        // File size check
        auto file_size = std::filesystem::file_size(filepath);

        if (file_size < 12) { // Minimum header size
            report_error("File too small to be valid: " + filename);
            return false;
        }
        
        if (!get_machine_is_server()) {
            if (file_size > MAX_TOTAL_KHDATA_SIZE) {
                report_error("File exceeds maximum size: " + filename);
                return false;
            }
        }

        try {
            // Try loading main file first
            std::ifstream stream(filepath, std::ios::binary);

            if (!stream) {
                // Try backup if main fails
                if (std::filesystem::exists(backup_path)) {
                    stream.open(backup_path, std::ios::binary);

                    if (stream) {
                        sqf::diag_log("Loading from backup: " + filename);
                    }
                }

                if (!stream) return false;
            }
            
            // Read header
            uint32_t magic, version, var_count;
            stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
            stream.read(reinterpret_cast<char*>(&version), sizeof(version));
            stream.read(reinterpret_cast<char*>(&var_count), sizeof(var_count));

            if (!stream.good()) {
                report_error("Failed to read file header: " + filename);
                return false;
            }
            
            if (magic != KHDATA_MAGIC || version != KHDATA_VERSION) {
                return false;
            }
            
            // Read variables
            std::unordered_map<std::string, game_value> vars;
            
            for (uint32_t i = 0; i < var_count; i++) {
                std::string var_name = read_string(stream);
                game_value value = read_game_value(stream);
                vars[var_name] = value;
            }
            
            if (!stream.eof() && stream.peek() != EOF) {
                report_error("Warning - extra data in file: " + filename);
                // File may be corrupt but we loaded what we could
            }

            // Create file object
            auto file = std::make_unique<KHDataFile>(filename);
            file->set_filepath(filepath);
            file->set_variables(std::move(vars));
            
            // On successful load and validation, create/update backup if needed
            try {
                bool should_backup = true;
                
                if (std::filesystem::exists(backup_path)) {
                    auto main_time = std::filesystem::last_write_time(filepath);
                    auto backup_time = std::filesystem::last_write_time(backup_path);
                    should_backup = (main_time > backup_time);
                }
                
                if (should_backup) {
                    std::filesystem::copy_file(filepath, backup_path, 
                        std::filesystem::copy_options::overwrite_existing);
                }
            } catch (...) {
                // Non-critical, continue
            }

            files[filename] = std::move(file);
            update_total_size();            
            return true;
        } catch (...) {
            // Try loading backup on any error
            if (std::filesystem::exists(backup_path)) {
                return load_file_from_backup(filename);
            }

            return false;
        }
    }

    bool load_file_from_backup(const std::string& filename) {
        auto backup_path = base_path / (filename + ".khdata.backup");
        
        if (!validate_filename(filename)) {
            report_error("Invalid filename for backup load: " + filename);
            return false;
        }
        
        if (!std::filesystem::exists(backup_path)) {
            return false;
        }
        
        // Check backup file size
        auto file_size = std::filesystem::file_size(backup_path);
        
        if (file_size < 12) {
            report_error("Backup file size invalid: " + filename);
            return false;
        }

        if (!get_machine_is_server()) {
            if (file_size > MAX_TOTAL_KHDATA_SIZE) {
                report_error("Backup file size invalid: " + filename);
                return false;
            }
        }
        
        try {
            std::ifstream stream(backup_path, std::ios::binary);
            if (!stream) return false;
            
            // Read header
            uint32_t magic, version, var_count;
            stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
            stream.read(reinterpret_cast<char*>(&version), sizeof(version));
            stream.read(reinterpret_cast<char*>(&var_count), sizeof(var_count));
            
            if (magic != KHDATA_MAGIC || version != KHDATA_VERSION) {
                report_error("Backup file has invalid header: " + filename);
                return false;
            }
            
            // Read variables
            std::unordered_map<std::string, game_value> vars;
            
            for (uint32_t i = 0; i < var_count; i++) {
                std::string var_name = read_string(stream);

                if (var_name.length() > 256) {  // Sanity check
                    report_error("Variable name too long in backup");
                    return false;
                }
                
                game_value value = read_game_value(stream);
                vars[var_name] = value;
            }
            
            // Create file object
            auto file = std::make_unique<KHDataFile>(filename);
            file->set_filepath(base_path / (filename + ".khdata"));
            file->set_variables(std::move(vars));
            
            // Try to restore the main file from backup
            auto main_path = base_path / (filename + ".khdata");

            try {
                std::filesystem::copy_file(backup_path, main_path, 
                    std::filesystem::copy_options::overwrite_existing);
                sqf::diag_log("Restored " + filename + " from backup");
            } catch (...) {
                report_error("Could not restore main file from backup for " + filename);
            }
            
            files[filename] = std::move(file);
            update_total_size();

            try {
                std::filesystem::remove(backup_path);
                sqf::diag_log("Deleted backup after successful restoration: " + filename);
            } catch (...) {
                // Non-critical if we can't delete the backup
            }

            return true;
            
        } catch (const std::exception& e) {
            report_error("Failed to load backup: " + std::string(e.what()));
            return false;
        } catch (...) {
            report_error("Unknown error loading backup: " + filename);
            return false;
        }
    }

    static size_t calculate_value_size(const game_value& value, int depth = 0) {       
        size_t size = sizeof(bool);  // For the special handling flag
        size += sizeof(game_data_type);  // For the type enum
        
        switch (value.type_enum()) {
            case game_data_type::NOTHING:
            case game_data_type::ANY:
                break;  // Just the type info
                
            case game_data_type::SCALAR:
                size += sizeof(float);
                break;
                
            case game_data_type::BOOL:
                size += sizeof(bool);
                break;
                
            case game_data_type::STRING: {
                std::string str = static_cast<std::string>(value);
                size += sizeof(uint32_t) + str.length();  // Length + data
                break;
            }
                
            case game_data_type::ARRAY: {
                auto& arr = value.to_array();
                size += sizeof(uint32_t);  // Array size
                
                // Recursively calculate size of each element
                for (const auto& elem : arr) {
                    size += calculate_value_size(elem, depth + 1);
                }

                break;
            }
                
            case game_data_type::HASHMAP: {
                auto& map = value.to_hashmap();
                size += sizeof(uint32_t);  // Map entry count
                
                // Each entry is stored as a [key,value] array
                for (const auto& pair : map) {
                    // Account for the array wrapper
                    size += sizeof(bool) + sizeof(game_data_type) + sizeof(uint32_t);
                    // Key and value
                    size += calculate_value_size(pair.key, depth + 1);
                    size += calculate_value_size(pair.value, depth + 1);
                }

                break;
            }
                
            case game_data_type::CODE: {
                // Code is serialized as a string
                std::string code_str = static_cast<std::string>(value);
                size += sizeof(game_data_type);  // Original type storage
                size += sizeof(uint32_t) + code_str.length();
                break;
            }
                
            case game_data_type::NAMESPACE:
            case game_data_type::SIDE:
            case game_data_type::GROUP:
            case game_data_type::OBJECT: {
                // These are serialized as strings
                size += sizeof(game_data_type);  // Original type storage
                
                // Estimate serialized string size
                if (value.type_enum() == game_data_type::NAMESPACE) {
                    size += sizeof(uint32_t) + 20;  // "missionNamespace" etc
                } else if (value.type_enum() == game_data_type::SIDE) {
                    size += sizeof(uint32_t) + 15;  // "west", "east", etc
                } else if (value.type_enum() == game_data_type::GROUP) {
                    std::string group_id_str = static_cast<std::string>(sqf::group_id(static_cast<group>(value)));
                    size += sizeof(uint32_t) + group_id_str.length();
                } else if (value.type_enum() == game_data_type::OBJECT) {
                    std::string var_name = static_cast<std::string>(sqf::vehicle_var_name(value));

                    if (var_name.empty()) {
                        size += sizeof(uint32_t) + 25;  // "OBJ_HASH_" + 16 char hash
                    } else {
                        size += sizeof(uint32_t) + var_name.length();
                    }
                }

                break;
            }
                
            default:
                // For unknown types, estimate 100 bytes
                size += 100;
                break;
        }
        
        return size;
    }

    bool save_file(KHDataFile* file) {
        if (!file) return false;
        
        // Check size limit before saving
        size_t new_size = estimate_file_size(file);
        size_t total_without_this = total_data_size - new_size;
        
        if ((total_without_this + new_size >= MAX_TOTAL_KHDATA_SIZE) && (!get_machine_is_server())) {
            file->mark_size_exceeded();
            report_error("File size limit exceeded: " + file->get_filepath().string());
            return false;
        }
        
        try {
            std::filesystem::path temp_path = file->get_filepath();
            temp_path += ".tmp";
            
            struct TempFileGuard {
                std::filesystem::path path;
                bool should_delete = true;

                ~TempFileGuard() {
                    if (should_delete) {
                        try { 
                            std::filesystem::remove(path); 
                        } catch (...) {}
                    }
                }
            } temp_guard{temp_path};

            {
                std::ofstream stream(temp_path, std::ios::binary);

                if (!stream) {
                    file->mark_save_failed();
                    report_error("Failed to open temp file: " + temp_path.string());
                    return false;
                }
                
                // Write header
                uint32_t magic = KHDATA_MAGIC;
                uint32_t version = KHDATA_VERSION;
                uint32_t var_count = static_cast<uint32_t>(file->get_variables().size());
                stream.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
                stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
                stream.write(reinterpret_cast<const char*>(&var_count), sizeof(var_count));
                
                // Write variables
                for (const auto& [name, value] : file->get_variables()) {
                    write_string(stream, name);
                    write_game_value(stream, value);
                }

                if (!stream.good()) {
                    file->mark_save_failed();
                    report_error("Write failed for file: " + file->get_filepath().string());
                    return false;
                }
            }
            
            // Delete old file if it exists, then rename temp
            std::error_code ec;

            if (std::filesystem::exists(file->get_filepath())) {
                std::filesystem::remove(file->get_filepath(), ec);

                if (ec) {
                    // Couldn't delete old file, try copy+replace instead
                    std::filesystem::copy_file(temp_path, file->get_filepath(), 
                        std::filesystem::copy_options::overwrite_existing, ec);
                    if (ec) {
                        file->mark_save_failed();
                        report_error("Failed to replace file: " + ec.message());
                        return false;
                    }
                    std::filesystem::remove(temp_path);
                    temp_guard.should_delete = false;
                    file->clear_dirty();
                    return true;
                }
            }
            
            // Old file deleted (or didn't exist), now rename should work
            std::filesystem::rename(temp_path, file->get_filepath(), ec);
            
            if (!ec) {
                temp_guard.should_delete = false;
                file->clear_dirty();
                return true;
            }
            
            // Rename failed for some reason, final fallback
            std::filesystem::copy_file(temp_path, file->get_filepath(), 
                std::filesystem::copy_options::overwrite_existing, ec);

            if (ec) {
                file->mark_save_failed();
                report_error("Failed to save file: " + ec.message());
                return false;
            }
            
            std::filesystem::remove(temp_path);
            temp_guard.should_delete = false;
            file->clear_dirty();
            return true;
        } catch (...) {
            file->mark_save_failed();
            return false;
        }
    }

    bool delete_file(const std::string& filename) {        
        // Validate filename
        if (!validate_filename(filename)) {
            report_error("Invalid filename for deletion: " + filename);
            return false;
        }
        
        auto it = files.find(filename);

        if (it != files.end() && it->second->needs_save()) {
            report_error("Attempting to save dirty file before deletion: " + filename);

            if (!save_file(it->second.get())) {
                report_error("Warning - failed to save dirty file before deletion: " + filename);
                // Continue with deletion anyway
            }
        }

        // Remove from memory
        files.erase(filename);
        update_total_size();
        
        // Delete from disk
        auto filepath = base_path / (filename + ".khdata");

        try {
            return std::filesystem::remove(filepath);
        } catch (...) {
            return false;
        }
    }

    int flush_all() {
        int saved_count = 0;
        
        for (auto& [name, file] : files) {
            if (file->needs_save()) {
                if (save_file(file.get())) {
                    saved_count++;
                }
            }
        }
        
        return saved_count;
    }
};

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
        lua_settop(L, top);  // Restore stack to original size
    }
};

class Lua_Compilation {
private:
    static std::string preprocess_lua_operators(const std::string& code) {
        std::string result = code;
        
        // Quick check - if no C-style operators, return immediately
        if (code.find("!=") == std::string::npos && 
            code.find("&&") == std::string::npos && 
            code.find("||") == std::string::npos) {
            return code;
        }
        
        // Build a map of string literal ranges to avoid
        std::vector<std::pair<size_t, size_t>> string_ranges;
        
        // Find all string literals (including long brackets)
        for (size_t i = 0; i < result.length(); i++) {
            // Check for long bracket strings [[...]]
            if (i + 1 < result.length() && result[i] == '[' && result[i + 1] == '[') {
                size_t start = i;
                i += 2;

                // Find the closing ]]
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
                
                // Count equals signs
                while (j < result.length() && result[j] == '=') {
                    equals_count++;
                    j++;
                }
                
                // Check if it's a valid long bracket start
                if (j < result.length() && result[j] == '[') {
                    i = j + 1;
                    // Build closing pattern
                    std::string closing = "]" + std::string(equals_count, '=') + "]";
                    // Find the closing bracket
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
                    if (result[i] == quote && result[i-1] != '\\') {
                        string_ranges.push_back({start, i});
                        break;
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
        
        // Replace operators
        struct Replacement {
            std::string from;
            std::string to;
        };
        
        std::vector<Replacement> replacements = {
            {"!=", "~="},
            {"&&", " and "},
            {"||", " or "}
        };
        
        for (const auto& rep : replacements) {
            size_t pos = 0;
            
            while ((pos = result.find(rep.from, pos)) != std::string::npos) {
                if (!in_string(pos)) {
                    result.replace(pos, rep.from.length(), rep.to);
                    pos += rep.to.length();
                } else {
                    pos += rep.from.length();
                }
            }
        }
        
        return result;
    }

public:
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
            
            // Check if compilation was successful
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
    
    switch (value.type_enum()) {
        case game_data_type::BOOL:
            return sol::make_object(lua, static_cast<bool>(value));

        case game_data_type::SCALAR:
            return sol::make_object(lua, static_cast<float>(value));
            
        case game_data_type::STRING:
            return sol::make_object(lua, static_cast<std::string>(value));
            
        case game_data_type::ARRAY: {
            auto& array = value.to_array();
            
            if (array.empty()) {
                return sol::make_object(lua, lua.create_table());
            }
            
            // Check first element type
            game_data_type first_type = array[0].type_enum();
            bool is_homogeneous = true;
            
            // Quick scan for homogeneity
            for (size_t i = 1; i < array.size(); ++i) {
                if (array[i].type_enum() != first_type) {
                    is_homogeneous = false;
                    break;
                }
            }
            
            if (is_homogeneous) {
                sol::table lua_table = lua.create_table(array.size(), 0);
                
                switch (first_type) {
                    case game_data_type::SCALAR: {
                        // Fast path for numbers
                        for (size_t i = 0; i < array.size(); ++i) {
                            lua_table.raw_set(i + 1, static_cast<float>(array[i]));
                        }

                        return sol::make_object(lua, lua_table);
                    }
                    
                    case game_data_type::BOOL: {
                        // Fast path for booleans
                        for (size_t i = 0; i < array.size(); ++i) {
                            lua_table.raw_set(i + 1, static_cast<bool>(array[i]));
                        }

                        return sol::make_object(lua, lua_table);
                    }
                    
                    case game_data_type::STRING: {
                        // Fast path for strings
                        for (size_t i = 0; i < array.size(); ++i) {
                            lua_table.raw_set(i + 1, static_cast<std::string>(array[i]));
                        }

                        return sol::make_object(lua, lua_table);
                    }
                    
                    case game_data_type::OBJECT: {
                        // Fast path for objects - wrap all at once
                        for (size_t i = 0; i < array.size(); ++i) {
                            lua_table.raw_set(i + 1, GameValueWrapper(array[i]));
                        }

                        return sol::make_object(lua, lua_table);
                    }
                    
                    case game_data_type::GROUP: {
                        // Fast path for groups
                        for (size_t i = 0; i < array.size(); ++i) {
                            lua_table.raw_set(i + 1, GameValueWrapper(array[i]));
                        }

                        return sol::make_object(lua, lua_table);
                    }
                    
                    case game_data_type::SIDE: {
                        // Fast path for sides
                        for (size_t i = 0; i < array.size(); ++i) {
                            lua_table.raw_set(i + 1, GameValueWrapper(array[i]));
                        }

                        return sol::make_object(lua, lua_table);
                    }
                    
                    case game_data_type::NOTHING:
                    case game_data_type::ANY: {
                        // Array of nil values
                        for (size_t i = 0; i < array.size(); ++i) {
                            lua_table.raw_set(i + 1, sol::nil);
                        }

                        return sol::make_object(lua, lua_table);
                    }
                    
                    // Check for homogeneous nested arrays (e.g., array of positions)
                    case game_data_type::ARRAY: {
                        // Check if all nested arrays are same size and type
                        size_t nested_size = array[0].size();
                        bool same_size = true;
                        
                        for (size_t i = 1; i < array.size(); ++i) {
                            if (array[i].size() != nested_size) {
                                same_size = false;
                                break;
                            }
                        }
                        
                        if (same_size && nested_size == 2) {
                            // Likely array of 2D positions
                            bool all_numbers = true;

                            for (const auto& elem : array) {
                                auto& sub = elem.to_array();

                                if (sub[0].type_enum() != game_data_type::SCALAR ||
                                    sub[1].type_enum() != game_data_type::SCALAR) {
                                    all_numbers = false;
                                    break;
                                }
                            }
                            
                            if (all_numbers) {
                                for (size_t i = 0; i < array.size(); ++i) {
                                    sol::table pos = lua.create_table(2, 0);
                                    auto& sub = array[i].to_array();
                                    pos.raw_set(1, static_cast<float>(sub[0]));
                                    pos.raw_set(2, static_cast<float>(sub[1]));
                                    lua_table.raw_set(i + 1, pos);
                                }

                                return sol::make_object(lua, lua_table);
                            }
                        } else if (same_size && nested_size == 3) {
                            // Likely array of 3D positions
                            bool all_numbers = true;
                            for (const auto& elem : array) {
                                auto& sub = elem.to_array();

                                if (sub[0].type_enum() != game_data_type::SCALAR ||
                                    sub[1].type_enum() != game_data_type::SCALAR ||
                                    sub[2].type_enum() != game_data_type::SCALAR) {
                                    all_numbers = false;
                                    break;
                                }
                            }
                            
                            if (all_numbers) {
                                for (size_t i = 0; i < array.size(); ++i) {
                                    sol::table pos = lua.create_table(3, 0);
                                    auto& sub = array[i].to_array();
                                    pos.raw_set(1, static_cast<float>(sub[0]));
                                    pos.raw_set(2, static_cast<float>(sub[1]));
                                    pos.raw_set(3, static_cast<float>(sub[2]));
                                    lua_table.raw_set(i + 1, pos);
                                }

                                return sol::make_object(lua, lua_table);
                            }
                        }
                        
                        // Fall through to default recursive conversion
                        break;
                    }
                    
                    default:
                        // Fall through to recursive conversion
                        break;
                }
            }
            
            // Default recursive conversion for heterogeneous or complex arrays
            sol::table lua_table = lua.create_table(array.size(), 0);

            for (size_t i = 0; i < array.size(); ++i) {
                lua_table[i + 1] = convert_game_value_to_lua(array[i]);
            }

            return sol::make_object(lua, lua_table);
        }

        case game_data_type::HASHMAP: {
            auto& hashmap = value.to_hashmap();
            
            // Check if all values are same type for optimization
            if (hashmap.count() > 0) {
                game_data_type value_type = game_data_type::NOTHING;
                bool homogeneous_values = true;
                
                for (auto& pair : hashmap) {
                    if (value_type == game_data_type::NOTHING) {
                        value_type = pair.value.type_enum();
                    } else if (pair.value.type_enum() != value_type) {
                        homogeneous_values = false;
                        break;
                    }
                }
                
                if (homogeneous_values && value_type == game_data_type::SCALAR) {
                    // Fast path for number-valued hashmaps
                    sol::table lua_table = lua.create_table(0, hashmap.count());

                    for (auto& pair : hashmap) {
                        // Keys still need conversion, but values can be fast
                        sol::object lua_key = convert_game_value_to_lua(pair.key);
                        lua_table.raw_set(lua_key, static_cast<float>(pair.value));
                    }

                    return sol::make_object(lua, lua_table);
                }
                
                if (homogeneous_values && value_type == game_data_type::STRING) {
                    // Fast path for string-valued hashmaps
                    sol::table lua_table = lua.create_table(0, hashmap.count());

                    for (auto& pair : hashmap) {
                        sol::object lua_key = convert_game_value_to_lua(pair.key);
                        lua_table.raw_set(lua_key, static_cast<std::string>(pair.value));
                    }
                    
                    return sol::make_object(lua, lua_table);
                }
            }
            
            // Default conversion
            sol::table lua_table = lua.create_table(0, hashmap.count());
            
            for (auto& pair : hashmap) {
                sol::object lua_key = convert_game_value_to_lua(pair.key);
                sol::object lua_value = convert_game_value_to_lua(pair.value);
                lua_table[lua_key] = lua_value;
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

        // Check for metatable with __toSQF metamethod
        sol::optional<sol::table> metatable = tbl[sol::metatable_key];
        
        if (metatable) {
            sol::optional<sol::function> to_sqf = (*metatable)["__toSQF"];

            if (to_sqf) {
                // Call the __toSQF metamethod and convert its result
                sol::object result = (*to_sqf)(tbl);
                return convert_lua_to_game_value(result);
            }
        }
        
        bool is_array = true;
        bool has_non_integer_keys = false;
        size_t max_index = 0;
        size_t non_nil_count = 0;
        
        // First pass: analyze the table structure
        for (auto& pair : tbl) {
            non_nil_count++;
            sol::object key = pair.first;
            
            if (key.get_type() == sol::type::number) {
                double key_num = key.as<double>();
                
                // Check if it's a positive integer
                if (key_num > 0 && key_num == std::floor(key_num)) {
                    size_t idx = static_cast<size_t>(key_num);

                    if (idx > max_index) {
                        max_index = idx;
                    }
                } else {
                    // Non-positive or non-integer numeric key
                    is_array = false;
                    has_non_integer_keys = true;
                    break;
                }
            } else {
                // Non-numeric key (string, etc.)
                is_array = false;
                has_non_integer_keys = true;
                break;
            }
        }
        
        // Empty tables are arrays
        if (non_nil_count == 0) {
            is_array = true;
            max_index = 0;
        }
        
        // Additional heuristic: if we have string keys, it's definitely a hashmap
        // But if we only have positive integer keys (even with gaps), treat it as an array
        
        if (is_array && max_index > 0) {
            // Convert as array, including nil values for gaps
            auto_array<game_value> arr;
            arr.reserve(max_index);
            
            for (size_t i = 1; i <= max_index; i++) {
                sol::object elem = tbl[i];
                arr.push_back(convert_lua_to_game_value(elem));
            }
            
            return game_value(std::move(arr));
        } else if (!is_array || has_non_integer_keys) {
            // Convert native Lua table to hashmap
            auto_array<game_value> kv_array;
            
            for (auto& pair : tbl) {
                auto_array<game_value> kv_pair;
                
                // Convert the key properly
                sol::object key = pair.first;
                game_value key_value;
                
                // Handle different key types
                if (key.get_type() == sol::type::string) {
                    key_value = game_value(key.as<std::string>());
                } else if (key.get_type() == sol::type::number) {
                    key_value = game_value(key.as<float>());
                } else if (key.get_type() == sol::type::boolean) {
                    key_value = game_value(key.as<bool>());
                } else {
                    // For other types, try generic conversion
                    key_value = convert_lua_to_game_value(key);
                }
                
                kv_pair.push_back(key_value);
                kv_pair.push_back(convert_lua_to_game_value(pair.second));
                kv_array.push_back(game_value(std::move(kv_pair)));
            }
            
            if (!kv_array.empty()) {
                return sqf::call2(g_create_hash_map_from_array, game_value(std::move(kv_array)));
            } else {
                return sqf::call2(g_create_hash_map);
            }
        } else {
            // Shouldn't reach here, but just in case
            return game_value(auto_array<game_value>());
        }
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
    static sol::object sqf_call(sol::object code_or_func, sol::optional<sol::object> arguments) {
        try {
            LuaStackGuard guard(*g_lua_state);
            game_value compiled;
            
            // Check if first argument is already compiled code
            if (code_or_func.get_type() == sol::type::userdata) {
                sol::optional<GameValueWrapper> wrapper = code_or_func.as<sol::optional<GameValueWrapper>>();
                
                if (wrapper && wrapper->value.type_enum() == game_data_type::CODE) {
                    compiled = wrapper->value;
                    
                    if (!arguments) {
                        return convert_game_value_to_lua(sqf::call2(compiled));
                    } else {
                        return convert_game_value_to_lua(sqf::call2(compiled, convert_lua_to_game_value(*arguments)));
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
                if (!arguments) {
                    // No parameters - compile as "call functionName"
                    std::string cache_key = "call " + code_or_func_str;
                    auto cache_it = g_sqf_function_cache.find(cache_key);
                    
                    if (cache_it != g_sqf_function_cache.end()) {
                        compiled = cache_it->second;
                    } else {
                        compiled = sqf::compile(cache_key);
                        g_sqf_function_cache[cache_key] = compiled;
                    }
                    
                    return convert_game_value_to_lua(sqf::call2(compiled));
                } else {
                    // With parameters - compile as "_this call functionName"
                    std::string cache_key = "_this call " + code_or_func_str;
                    auto cache_it = g_sqf_function_cache.find(cache_key);
                    
                    if (cache_it != g_sqf_function_cache.end()) {
                        compiled = cache_it->second;
                    } else {
                        compiled = sqf::compile(cache_key);
                        g_sqf_function_cache[cache_key] = compiled;
                    }
                    
                    return convert_game_value_to_lua(sqf::call2(compiled, convert_lua_to_game_value(*arguments)));
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
                
                if (!arguments) {
                    return convert_game_value_to_lua(sqf::call2(compiled));
                } else {
                    return convert_game_value_to_lua(sqf::call2(compiled, convert_lua_to_game_value(*arguments)));
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
    
    static std::vector<ScheduledTask> lua_scheduled_tasks;
    static int next_task_id = 1;
    static std::unordered_map<int, size_t> lua_task_id_map;
    
    // Schedule a function to run after a delay in seconds
    static int delay(float time_or_frames, sol::protected_function callback) {
        try {
            if (!callback.valid()) {
                report_error("Invalid callback function");
                return -1;
            }
            
            int task_id = next_task_id++;
            size_t index = lua_scheduled_tasks.size();
            ScheduledTask task;
            task.callback = callback;
            task.repeating = false;
            task.has_timeout = false;
            task.prioritize_timeout = false;
            task.timeout_time = 0.0f;
            task.timeout_frame = 0;
            task.start_time = g_current_mission_time;
            task.start_frame = g_current_mission_frame;
            
            if (time_or_frames >= 0) {
                // Positive or zero = seconds
                task.execute_time = g_current_mission_time + time_or_frames;
                task.execute_frame = 0;
                task.use_frames = false;
                task.interval = 0.0f;
                task.frame_interval = 0;
            } else {
                // Negative = frames (use absolute value)
                task.execute_time = 0.0f;
                task.execute_frame = g_current_mission_frame + static_cast<int>(std::abs(time_or_frames));
                task.use_frames = true;
                task.interval = 0.0f;
                task.frame_interval = 0;
            }
            
            lua_scheduled_tasks.push_back(task);
            lua_task_id_map[task_id] = index;
            return task_id;
        } catch (const std::exception& e) {
            report_error("Failed to create temporal handler: " + std::string(e.what()));
            return -1;
        }
    }

    static int interval(float time_or_frames, float timeout, bool prioritize, sol::protected_function callback) {
        try {
            if (!callback.valid()) {
                report_error("Invalid callback function");
                return -1;
            }
            
            int task_id = next_task_id++;
            size_t index = lua_scheduled_tasks.size();
            ScheduledTask task;
            task.callback = callback;
            task.repeating = true;
            task.start_time = g_current_mission_time;
            task.start_frame = g_current_mission_frame;
            
            if (time_or_frames > 0) {
                // Positive = seconds
                task.execute_time = g_current_mission_time + time_or_frames;
                task.execute_frame = 0;
                task.use_frames = false;
                task.interval = time_or_frames;
                task.frame_interval = 0;
                
                if (timeout != 0) {
                    task.has_timeout = true;
                    task.timeout_time = g_current_mission_time + timeout;
                    task.timeout_frame = 0;
                    task.prioritize_timeout = prioritize;
                } else {
                    task.has_timeout = false;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = 0;
                    task.prioritize_timeout = false;
                }
            } else if (time_or_frames < 0) {
                // Negative = frames (use absolute value)
                int frames = static_cast<int>(std::abs(time_or_frames));
                task.execute_time = 0.0f;
                task.execute_frame = g_current_mission_frame + frames;
                task.use_frames = true;
                task.interval = 0.0f;
                task.frame_interval = frames;
                
                if (timeout != 0) {
                    task.has_timeout = true;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = g_current_mission_frame + static_cast<int>(std::abs(timeout));
                    task.prioritize_timeout = prioritize;
                } else {
                    task.has_timeout = false;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = 0;
                    task.prioritize_timeout = false;
                }
            } else {
                // Zero = every frame (1 frame interval)
                task.execute_time = 0.0f;
                task.execute_frame = g_current_mission_frame + 1;
                task.use_frames = true;
                task.interval = 0.0f;
                task.frame_interval = 1;
                
                if (timeout != 0) {
                    task.has_timeout = true;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = g_current_mission_frame + static_cast<int>(std::abs(timeout));
                    task.prioritize_timeout = prioritize;
                } else {
                    task.has_timeout = false;
                    task.timeout_time = 0.0f;
                    task.timeout_frame = 0;
                    task.prioritize_timeout = false;
                }
            }
            
            lua_scheduled_tasks.push_back(task);
            lua_task_id_map[task_id] = index;
            return task_id;
        } catch (const std::exception& e) {
            report_error("Failed to create temporal handler: " + std::string(e.what()));
            return -1;
        }
    }
    
    // Cancel a scheduled task
    static bool cancel_task(int task_id) {
        try {
            auto it = lua_task_id_map.find(task_id);

            if (it != lua_task_id_map.end()) {
                // Mark as invalid instead of erasing to avoid index issues
                if (it->second < lua_scheduled_tasks.size()) {
                    lua_scheduled_tasks[it->second].callback = sol::nil;
                }
                
                lua_task_id_map.erase(it);
                return true;
            }

            return false;
        } catch (const std::exception& e) {
            report_error("Failed to cancel task: " + std::string(e.what()));
            return false;
        }
    }
    
    // Update scheduler - called from on_frame
    static void update_scheduler() {            
        for (size_t i = 0; i < lua_scheduled_tasks.size(); ) {
            auto& task = lua_scheduled_tasks[i];
            
            // Skip cancelled tasks
            if (!task.callback.valid()) {
                lua_scheduled_tasks.erase(lua_scheduled_tasks.begin() + i);
                
                for (auto& pair : lua_task_id_map) {
                    if (pair.second > i) {
                        pair.second--;
                    } else if (pair.second == i) {
                        lua_task_id_map.erase(pair.first);
                        break;
                    }
                }

                continue;
            }
            
            // Check timeout first
            bool past_timeout = false;
            bool at_timeout_boundary = false;
            
            if (task.has_timeout && task.repeating) {
                if (task.use_frames) {
                    past_timeout = g_current_mission_frame > task.timeout_frame;
                    
                    if (!task.prioritize_timeout && g_current_mission_frame == task.timeout_frame) {
                        // Check if this frame is exactly on an interval boundary
                        int elapsed = g_current_mission_frame - task.start_frame;
                        at_timeout_boundary = (elapsed % task.frame_interval) == 0;
                    }
                } else {
                    past_timeout = g_current_mission_time > task.timeout_time;
                    
                    if (!task.prioritize_timeout) {
                        float elapsed = g_current_mission_time - task.start_time;
                        float epsilon = 0.01f; // Small tolerance for floating point
                        
                        // Check if we're at or very close to a timeout that aligns with interval
                        if (std::abs(elapsed - task.timeout_time + task.start_time) < epsilon) {
                            float intervals = elapsed / task.interval;
                            at_timeout_boundary = std::abs(intervals - std::round(intervals)) < epsilon;
                        }
                    }
                }
                
                // If past timeout and not at boundary (or not prioritizing), remove task
                if (past_timeout && !at_timeout_boundary) {
                    lua_scheduled_tasks.erase(lua_scheduled_tasks.begin() + i);
                    
                    for (auto& pair : lua_task_id_map) {
                        if (pair.second > i) {
                            pair.second--;
                        } else if (pair.second == i) {
                            lua_task_id_map.erase(pair.first);
                            break;
                        }
                    }

                    continue;
                }
            }
            
            // Check if should execute
            bool should_execute = false;
            
            if (task.use_frames) {
                should_execute = g_current_mission_frame >= task.execute_frame;
            } else {
                should_execute = g_current_mission_time >= task.execute_time;
            }
            
            if (should_execute) {
                // Execute callback
                sol::protected_function_result result = task.callback();
                
                if (!result.valid()) {
                    sol::error err = result;
                    report_error("Scheduled task error: " + std::string(err.what()));
                }
                
                // If this was the timeout boundary execution, remove the task
                if (at_timeout_boundary) {
                    lua_scheduled_tasks.erase(lua_scheduled_tasks.begin() + i);
                    
                    for (auto& pair : lua_task_id_map) {
                        if (pair.second > i) {
                            pair.second--;
                        } else if (pair.second == i) {
                            lua_task_id_map.erase(pair.first);
                            break;
                        }
                    }
                } else if (task.repeating) {
                    // Reschedule
                    if (task.use_frames) {
                        task.execute_frame = g_current_mission_frame + task.frame_interval;
                    } else {
                        task.execute_time = g_current_mission_time + task.interval;
                    }

                    i++;
                } else {
                    // Remove completed one-time task
                    lua_scheduled_tasks.erase(lua_scheduled_tasks.begin() + i);
                    
                    for (auto& pair : lua_task_id_map) {
                        if (pair.second > i) {
                            pair.second--;
                        } else if (pair.second == i) {
                            lua_task_id_map.erase(pair.first);
                            break;
                        }
                    }
                }
            } else {
                i++;
            }
        }
    }

    static std::unordered_map<std::string, std::vector<sol::protected_function>> lua_event_handlers;

    static int add_event_handler(const std::string& event_name, sol::protected_function handler) {
        try {
            if (!handler.valid()) {
                report_error("Invalid handler function");
                return -1;
            }
            
            auto& handlers = lua_event_handlers[event_name];
            handlers.push_back(handler);
            return static_cast<int>(handlers.size());
        } catch (const std::exception& e) {
            report_error("Failed to add event handler: " + std::string(e.what()));
            return -1;
        }
    }
    
    static bool remove_event_handler(const std::string& event_name, int handler_id) {
        try {
            auto it = lua_event_handlers.find(event_name);

            if (it != lua_event_handlers.end() && handler_id > 0 && handler_id <= it->second.size()) {
                it->second.erase(it->second.begin() + (handler_id - 1));
                return true;
            }

            return false;
        } catch (const std::exception& e) {
            report_error("Failed to remove event handler: " + std::string(e.what()));
            return false;
        }
    }
    
    static sol::object trigger_event_local(const std::string& event_name, sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            auto it = lua_event_handlers.find(event_name);
            
            if (it == lua_event_handlers.end()) {
                return 0;
            }
            
            sol::object last_result = sol::nil;
            
            // Convert variadic_args to vector once
            std::vector<sol::object> arg_vec;

            for (auto arg : args) {
                arg_vec.push_back(arg);
            }
            
            for (auto& handler : it->second) {
                if (!handler.valid()) continue;
                
                // Use the raw Lua C API for flexible argument passing
                sol::state& lua = *g_lua_state;
                lua_State* L = lua.lua_state();
                
                handler.push(L);
                for (const auto& arg : arg_vec) {
                    arg.push(L);
                }
                
                if (lua_pcall(L, arg_vec.size(), 1, 0) == 0) {
                    last_result = sol::stack::pop<sol::object>(L);
                } else {
                    std::string err = lua_tostring(L, -1);
                    report_error("Event handler error: " + err);
                    lua_pop(L, 1);
                }
            }
            
            return last_result;
        } catch (const std::exception& e) {
            report_error("Failed to trigger event: " + std::string(e.what()));
            return sol::nil;
        }
    }

    // Helper that takes a vector directly
    static sol::object trigger_event_from_vector(const std::string& event_name, const std::vector<sol::object>& args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            auto it = lua_event_handlers.find(event_name);
            
            if (it == lua_event_handlers.end()) {
                return 0;
            }
            
            sol::object last_result = sol::nil;
            
            for (auto& handler : it->second) {
                if (!handler.valid()) continue;
                
                sol::state& lua = *g_lua_state;
                lua_State* L = lua.lua_state();
                
                handler.push(L);
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
        } catch (const std::exception& e) {
            report_error("Failed to trigger event: " + std::string(e.what()));
            return sol::nil;
        }
    }
    
    // CBA-enabled trigger that uses the helper
    static sol::object trigger_event(const std::string& event_name, sol::table args, 
                              sol::object target, sol::optional<sol::object> jip) {
        try {
            LuaStackGuard guard(*g_lua_state);
            game_value target_gv = convert_lua_to_game_value(target);
            
            if (target_gv.is_nil()) {
                // Fall back to local emission using the vector version
                std::vector<sol::object> arg_vec;

                for (size_t i = 1; i <= args.size(); i++) {
                    arg_vec.push_back(args[i]);
                }

                return trigger_event_from_vector(event_name, arg_vec);
            }
            
            // CBA emission (rest unchanged)
            game_value jip_gv = jip ? convert_lua_to_game_value(*jip) : game_value();
            auto_array<game_value> args_array;

            for (size_t i = 1; i <= args.size(); i++) {
                args_array.push_back(convert_lua_to_game_value(args[i]));
            }
            
            // Build CBA parameters: ["KH_eve_luaEventTrigger", arguments, target, jip]
            auto_array<game_value> cba_event_data;
            cba_event_data.push_back(game_value(event_name));
            cba_event_data.push_back(game_value(std::move(args_array)));
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_luaEventTrigger"));
            cba_params.push_back(game_value(std::move(cba_event_data)));
            cba_params.push_back(target_gv);
            cba_params.push_back(jip_gv);
            sqf::call2(g_trigger_cba_event, game_value(std::move(cba_params)));
            return sol::nil;
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
            }
            
            lua_event_handlers.erase(event_name);
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

    static sol::object add_game_event_handler(sqf_string_const_ref type, sqf_string_const_ref event, sol::protected_function handler) {
        try {
            LuaStackGuard guard(*g_lua_state);
            
            if (!handler.valid()) {
                report_error("Invalid handler function");
                return sol::nil;
            }
            
            // Generate unique ID for this handler
            std::string handler_uid = UIDGenerator::generate();
            
            // Register the Lua function as an event handler using the UID as event name
            auto& handlers = lua_event_handlers[handler_uid];
            handlers.push_back(handler);
            
            // Call SQF to register the game event handler
            // KH_fnc_addEventHandler will trigger the Lua event when the game event fires
            // Build parameters: [type, event, arguments, function]
            auto_array<game_value> sqf_params;
            sqf_params.push_back(game_value(static_cast<std::string>(type)));
            sqf_params.push_back(game_value(static_cast<std::string>(event)));
            sqf_params.push_back(game_value(handler_uid));
            sqf_params.push_back(game_value(g_game_event_handler_bridge));
            game_value sqf_result = sqf::call2(g_add_game_event_handler, game_value(std::move(sqf_params)));
            sol::table result = g_lua_state->create_table();
            result[1] = handler_uid;
            result[2] = convert_game_value_to_lua(sqf_result);
            return sol::make_object(*g_lua_state, result);
        } catch (const std::exception& e) {
            report_error("Failed to add game event handler: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static bool remove_game_event_handler(sol::table handler_info) {
        try {
            LuaStackGuard guard(*g_lua_state);

            // Extract uid and id from the handler info table [uid, id]
            sol::object uid_obj = handler_info[1];
            sol::object id_obj = handler_info[2];
            
            if (!uid_obj.valid() || uid_obj.get_type() != sol::type::string) {
                report_error("Invalid handler info: missing or invalid uid");
                return false;
            }
            
            std::string uid = uid_obj.as<std::string>();
            
            // Remove the Lua event handler
            auto it = lua_event_handlers.find(uid);

            if (it != lua_event_handlers.end()) {
                lua_event_handlers.erase(it);
            }
            
            // Call SQF to remove the game event handler
            if (id_obj.valid() && id_obj.get_type() != sol::type::nil) {
                game_value id_gv = convert_lua_to_game_value(id_obj);            
                sqf::call2(g_remove_game_event_handler, id_gv);
            }
            
            return true;
        } catch (const std::exception& e) {
            report_error("Failed to remove game event handler: " + std::string(e.what()));
            return false;
        }
    }

    static sol::object remote_execute(sol::object target, sol::object func, sol::object arguments, sol::optional<sol::object> special) {
        try {
            LuaStackGuard guard(*g_lua_state);
            game_value target_gv = convert_lua_to_game_value(target);
            game_value args_gv = convert_lua_to_game_value(arguments);
            game_value special_gv = special ? convert_lua_to_game_value(*special) : game_value();
            
            // Convert function to string
            std::string function_str;

            if (func.get_type() == sol::type::string) {
                function_str = func.as<std::string>();
            } else {
                // Convert non-string to string (handles function objects via tostring)
                sol::state& lua = *g_lua_state;
                function_str = lua["tostring"](func);
            }
            
            // Build parameters: [arguments, function, target, true, special]
            auto_array<game_value> sqf_params;
            sqf_params.push_back(args_gv);
            sqf_params.push_back(game_value(function_str));
            sqf_params.push_back(target_gv);
            sqf_params.push_back(game_value(true));
            sqf_params.push_back(special_gv);            
            sqf::call2(g_remote_execute, game_value(std::move(sqf_params)));
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("Failed to remote execute: " + std::string(e.what()));
            return sol::nil;
        }
    }

    static sol::object trigger_cba_event(const std::string& event_name, sol::object arguments,
                                sol::optional<sol::object> target_opt, sol::optional<sol::object> jip_opt) {
        try {
            // Convert arguments
            game_value args_gv;

            if (arguments.get_type() == sol::type::table) {
                sol::table args_table = arguments;
                auto_array<game_value> args_array;

                for (size_t i = 1; i <= args_table.size(); i++) {
                    args_array.push_back(convert_lua_to_game_value(args_table[i]));
                }

                args_gv = game_value(std::move(args_array));
            } else if (arguments.get_type() != sol::type::nil) {
                args_gv = convert_lua_to_game_value(arguments);
            } else {
                // No arguments
                args_gv = game_value(auto_array<game_value>());
            }
            
            game_value target_gv = target_opt ? convert_lua_to_game_value(*target_opt) : game_value("GLOBAL");
            game_value jip_gv = jip_opt ? convert_lua_to_game_value(*jip_opt) : game_value();
            
            // Build CBA parameters: [event_name, arguments, target, jip]
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value(event_name));
            cba_params.push_back(args_gv);
            cba_params.push_back(target_gv);
            cba_params.push_back(jip_gv);
            sqf::call2(g_trigger_cba_event, game_value(std::move(cba_params)));
            return sol::nil;
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
            
            // Determine the value to emit
            game_value emit_value;

            if (value_opt) {
                // Use provided value
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

            // Build CBA parameters: ["KH_eve_luaVariableEmission", [var_name, emit_value], target, jip]
            auto_array<game_value> emission_data;
            emission_data.push_back(game_value(var_name));
            emission_data.push_back(emit_value);
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_luaVariableEmission"));
            cba_params.push_back(game_value(std::move(emission_data)));
            cba_params.push_back(target);
            cba_params.push_back(jip);
            sqf::call2(g_trigger_cba_event, game_value(std::move(cba_params)));
            return sol::nil;
        } catch (const std::exception& e) {
            report_error("Failed to emit variable: " + std::string(e.what()));
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
    
    // Profile code execution
    static sol::object profile_code(sol::variadic_args args) {
        try {
            LuaStackGuard guard(*g_lua_state);
            sol::state& lua = *g_lua_state;
            
            if (args.size() < 2) {
                report_error("Not enough arguments");
                return sol::nil;
            }
            
            sol::object params = args[0];
            sol::object code_obj = args[1];
            sol::object count_obj = args.size() > 2 ? args[2] : sol::make_object(lua, 1000);
            
            // Validate code parameter
            if (code_obj.get_type() != sol::type::string) {
                report_error("Code must be a string");
                return sol::nil;
            }
            
            std::string code = code_obj.as<std::string>();
            int count = count_obj.get_type() == sol::type::number ? count_obj.as<int>() : 1;
            
            if (count < 1) {
                report_error("Execution count must be at least 1");
                return sol::nil;
            }
            
            sol::protected_function compiled;
            
            // Check if it's a function name or code to compile
            if (code.find(' ') == std::string::npos && code.find('(') == std::string::npos) {
                // It's a function name
                sol::object func = lua[code];

                if (func.get_type() != sol::type::function) {
                    report_error("Function '" + code + "' not found or is not a function");
                    return sol::nil;
                }
                
                compiled = func;
            } else {
                // Compile the code
                std::string full_code = "return function(...) " + code + " end";
                sol::load_result load_res = lua.load(full_code);
                
                if (!load_res.valid()) {
                    sol::error err = load_res;
                    report_error("Failed to compile code: " + std::string(err.what()));
                    return sol::nil;
                }
                
                sol::protected_function factory = load_res;
                auto factory_result = factory();

                if (!factory_result.valid()) {
                    sol::error err = factory_result;
                    report_error("Failed to create function: " + std::string(err.what()));
                    return sol::nil;
                }

                compiled = factory_result;
            }
            
            // Prepare arguments
            std::vector<sol::object> func_args;

            if (params.get_type() == sol::type::table) {
                sol::table tbl = params;

                for (size_t i = 1; i <= tbl.size(); i++) {
                    func_args.push_back(tbl[i]);
                }
            } else if (params.get_type() != sol::type::nil) {
                func_args.push_back(params);
            }
            
            // Warm up if count > 10
            if (count > 10) {
                for (int i = 0; i < 3; i++) {
                    if (func_args.empty()) {
                        compiled();
                    } else {
                        compiled(sol::as_args(func_args));
                    }
                }
            }
            
            // Get timer function
            sol::function get_time = lua["time"]["boot"];

            if (!get_time.valid()) {
                report_error("High precision timer not available");
                return sol::nil;
            }
            
            // Profile execution
            double start_time = get_time();
            
            for (int i = 0; i < count; i++) {
                if (func_args.empty()) {
                    compiled();
                } else {
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

    // Get the data type of a value
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

    // Execute in SQF namespace
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
            bool syms = use_symbols.value_or(false);
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
            game_value gv = convert_lua_to_game_value(value);

            // Check if we should trigger CBA event
            if (target_opt && !target_opt->is<sol::nil_t>()) {
                game_value target = convert_lua_to_game_value(*target_opt);
                game_value jip = jip_opt ? convert_lua_to_game_value(*jip_opt) : game_value();
                
                // Build CBA parameters: ["KH_eve_khDataWriteEmission", [filename, var_name, gv], target, jip]
                auto_array<game_value> value_array;
                value_array.push_back(filename);
                value_array.push_back(var_name);
                value_array.push_back(gv);
                auto_array<game_value> cba_params;
                cba_params.push_back(game_value("KH_eve_khDataWriteEmission"));
                cba_params.push_back(game_value(std::move(value_array)));
                cba_params.push_back(target);
                cba_params.push_back(jip);
                sqf::call2(g_trigger_cba_event, game_value(std::move(cba_params)));
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
            
            if (x < 0 || y < 0 || 
                y >= static_cast<int>(g_terrain_matrix.size()) || 
                x >= static_cast<int>(g_terrain_matrix[0].size())) {
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
        event_table["trigger"] = LuaFunctions::trigger_event_local;
        event_table["clear"] = LuaFunctions::clear_handlers;
        event_table["getHandlerCount"] = LuaFunctions::get_handler_count;
        game_event_table["add"] = LuaFunctions::add_game_event_handler;
        game_event_table["remove"] = LuaFunctions::remove_game_event_handler;
        time_table["getDate"] = LuaFunctions::get_date_time;
        time_table["getEpoch"] = LuaFunctions::get_time_epoch;
        time_table["getBoot"] = LuaFunctions::get_time_boot;
        temporal_table["delay"] = LuaFunctions::delay;
        temporal_table["interval"] = LuaFunctions::interval;
        temporal_table["cancel"] = LuaFunctions::cancel_task;
        network_table["remoteExecute"] = LuaFunctions::remote_execute;
        network_table["triggerEvent"] = LuaFunctions::trigger_event;
        network_table["emitVariable"] = LuaFunctions::emit_variable;
        terrain_table["getTerrainMatrix"] = LuaFunctions::get_terrain_matrix;
        terrain_table["getTerrainHeightAt"] = LuaFunctions::get_terrain_height_at;
        terrain_table["getTerrainHeightAtPos"] = LuaFunctions::get_terrain_height_at_pos;
        util_table["profile"] = LuaFunctions::profile_code;
        util_table["generateRandomString"] = LuaFunctions::generate_random_string;
        util_table["generateUid"] = LuaFunctions::generate_uid;
        util_table["getDataType"] = LuaFunctions::get_data_type;
        util_table["withSqf"] = LuaFunctions::with_sqf;
        util_table["switch"] = LuaFunctions::switch_case;
        util_table["sqfCall"] = LuaFunctions::sqf_call;
        util_table["triggerCbaEvent"] = LuaFunctions::trigger_cba_event;
        
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

                if (args.size() == 0) {
                    auto cache_it = g_sqf_command_cache.find(cmd);
                    game_value compiled;
                    
                    if (cache_it != g_sqf_command_cache.end()) {
                        compiled = cache_it->second;
                    } else {
                        compiled = sqf::compile(cmd);
                        g_sqf_command_cache[cmd] = compiled;
                    }

                    return convert_game_value_to_lua(sqf::call2(compiled));
                } else if (args.size() == 1) {
                    std::string key = cmd + " _this";
                    auto cache_it = g_sqf_command_cache.find(key);
                    game_value compiled;

                    if (cache_it != g_sqf_command_cache.end()) {
                        compiled = cache_it->second;
                    } else {
                        compiled = sqf::compile(key);
                        g_sqf_command_cache[key] = compiled;
                    }

                    return convert_game_value_to_lua(sqf::call2(compiled, convert_lua_to_game_value(args[0])));
                } else if (args.size() == 2) {
                    std::string key = "(_this#0) " + cmd + " (_this#1)";
                    auto cache_it = g_sqf_command_cache.find(key);
                    game_value compiled;

                    if (cache_it != g_sqf_command_cache.end()) {
                        compiled = cache_it->second;
                    } else {
                        compiled = sqf::compile(key);
                        g_sqf_command_cache[key] = compiled;
                    }

                    return convert_game_value_to_lua(sqf::call2(
                        compiled, 
                        game_value({convert_lua_to_game_value(args[0]), convert_lua_to_game_value(args[1])})
                    ));
                } else {
                    report_error("SQF commands only support 0-2 arguments");
                    return sol::nil;
                }
            } catch (const std::exception& e) {
                report_error("Failed to handle command: " + std::string(e.what()));
                return sol::nil;
            }
        };

        // Create sqf table for SQF functions, limited commands
        auto sqf_table = g_lua_state->create_named_table("sqf");
        sol::table sqf_metatable = g_lua_state->create_table();

        sqf_metatable["__index"] = [command_handler](sol::table table, std::string key) -> sol::object {
            try {
                // First check if the key exists in the table
                sol::object existing = table.raw_get<sol::object>(key);
                
                if (existing.valid() && existing != sol::nil) {
                    return existing;
                }
                
                // Create a lambda that will call the command handler with the captured key
                sol::state_view lua(table.lua_state());
                
                auto command_func = [key, command_handler](sol::variadic_args args) -> sol::object {
                    return command_handler(key, args);
                };
                
                // Return the function wrapped as a Lua object
                return sol::make_object(lua, command_func);
            } catch (const std::exception& e) {
                report_error("SQF metatable error: " + std::string(e.what()));
                return sol::nil;
            }
        };

        // Apply the metatable to the sqf table
        sqf_table[sol::metatable_key] = sqf_metatable;

        sqf_table["diag_fps"] = []() -> float {
            return sqf::diag_fps();
        };

        sqf_table["diag_fpsMin"] = []() -> float {
            return sqf::diag_fpsmin();
        };

        sqf_table["diag_frameNo"] = []() -> float {
            return sqf::diag_frameno();
        };

        sqf_table["diag_tickTime"] = []() -> float {
            return sqf::diag_ticktime();
        };

        sqf_table["diag_log"] = [](sqf_string_const_ref text) -> sol::object {
            sqf::diag_log(text);
            return sol::nil;
        };

        sqf_table["throw"] = [](sqf_string_const_ref text) -> sol::object {
            sqf::throw_exception(text);
            return sol::nil;
        };
        
        sqf_table["publicVariable"] = [](sqf_string_const_ref var_name) -> sol::object {
            sqf::public_variable(var_name);
            return sol::nil;
        };

        sqf_table["publicVariableServer"] = [](sqf_string_const_ref var_name) -> sol::object {
            sqf::public_variable_server(var_name);
            return sol::nil;
        };

        sqf_table["publicVariableClient"] = [](float client_id, sqf_string_const_ref var_name) -> sol::object {
            sqf::public_variable_client(client_id, var_name);
            return sol::nil;
        };

        sqf_table["clientOwner"] = []() -> float {
            return sqf::client_owner();
        };

        sqf_table["isServer"] = []() -> bool {
            return sqf::is_server();
        };

        sqf_table["isDedicated"] = []() -> bool {
            return sqf::is_dedicated();
        };

        sqf_table["hasInterface"] = []() -> bool {
            return sqf::has_interface();
        };

        sqf_table["currentNamespace"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::current_namespace());
        };

        sqf_table["missionNamespace"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::mission_namespace());
        };

        sqf_table["profileNamespace"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::profile_namespace());
        };

        sqf_table["uiNamespace"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::ui_namespace());
        };

        sqf_table["parsingNamespace"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::parsing_namespace());
        };

        sqf_table["serverNamespace"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::server_namespace());
        };

        sqf_table["missionProfileNamespace"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::mission_profile_namespace());
        };

        sqf_table["missionName"] = []() -> std::string {
            return static_cast<std::string>(sqf::mission_name());
        };

        sqf_table["profileName"] = []() -> std::string {
            return static_cast<std::string>(sqf::profile_name());
        };

        sqf_table["profileNameSteam"] = []() -> std::string {
            return static_cast<std::string>(sqf::profile_namesteam());
        };

        sqf_table["blufor"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::blufor());
        };

        sqf_table["west"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::west());
        };

        sqf_table["opfor"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::opfor());
        };

        sqf_table["east"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::east());
        };

        sqf_table["resistance"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::resistance());
        };

        sqf_table["independent"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::independent());
        };

        sqf_table["civilian"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::civilian());
        };

        sqf_table["sideLogic"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::side_logic());
        };

        sqf_table["sideUnknown"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::side_unknown());
        };

        sqf_table["sideEnemy"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::side_enemy());
        };

        sqf_table["sideFriendly"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::side_friendly());
        };

        sqf_table["sideAmbientLife"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::side_ambient_life());
        };

        sqf_table["sideEmpty"] = []() -> sol::object {
            return convert_game_value_to_lua(sqf::side_empty());
        };
    }
}

// Native SQF command implementation for execution
static game_value execute_lua_sqf(game_value_parameter args, game_value_parameter code_or_function) {    
    try {
        LuaStackGuard guard(*g_lua_state);
        std::string code_str;
        
        // Handle CODE type or STRING type
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
                // Compile the code
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
                // If args is an array, unpack it
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
        
        // Check for errors
        if (!result.valid()) {
            sol::error err = result;
            report_error(std::string(err.what()));
            return game_value();
        }
        
        // Convert return value(s) to game_value
        if (result.return_count() == 0) {
            return game_value();  // nil
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
        return game_value();
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
        
        // Parse options if provided (expects array of 3 bools)
        if (options.type_enum() == game_data_type::ARRAY) {
            auto& arr = options.to_array();
            
            // Set based on array elements
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
        }

        // If options is nil or not an array, defaults are already set to all true
        std::string result = RandomStringGenerator::generate(len, use_numbers, use_letters, use_symbols);
        return game_value(result);
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value generate_uid_sqf() {
    try {
        std::string uid = UIDGenerator::generate();
        return game_value(uid);
    } catch (const std::exception& e) {
        report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value read_khdata_sqf(game_value_parameter filename, game_value_parameter var_param) {
    try {
        std::string file_str = static_cast<std::string>(filename);
        std::string var_str;
        game_value default_value;
        bool has_default = false;
        
        // Parse the variable parameter
        if (var_param.type_enum() == game_data_type::STRING) {
            // Simple string - just variable name
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
        } else {
            report_error("Variable parameter must be string or array");
            return game_value();
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
        
        if (params.type_enum() != game_data_type::ARRAY || params.size() < 2) {
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
        if (!target.is_nil()) {
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
            sqf::call2(g_trigger_cba_event, game_value(std::move(cba_params)));
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
        return game_value();
    }
}

static game_value trigger_lua_event_local_sqf(game_value_parameter params) {
    try {
        LuaStackGuard guard(*g_lua_state);
        std::string event_name;
        game_value event_args;
        
        // Handle different input formats
        if (params.type_enum() == game_data_type::STRING) {
            // Simple string: just event name, no arguments
            event_name = static_cast<std::string>(params);
            event_args = game_value();
        } else if (params.type_enum() == game_data_type::ARRAY) {
            auto& arr = params.to_array();
            
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
            event_args = arr.size() > 1 ? arr[1] : game_value();
        } else {
            report_error("String or array parameter required");
            return game_value();
        }
        
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
        return convert_lua_to_game_value(result);
    } catch (const std::exception& e) {
        report_error("Failed to trigger local event: " + std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Failed to trigger local event");
        return game_value();
    }
}

static game_value trigger_lua_event_sqf(game_value_parameter params) {
    try {
        std::string event_name;
        game_value event_args;
        game_value target;
        game_value jip;
        
        // Handle different input formats
        if (params.type_enum() == game_data_type::STRING) {
            // Simple string: just event name, no arguments
            event_name = static_cast<std::string>(params);
            event_args = game_value();  // nil
            target = game_value();       // nil
            jip = game_value();          // nil
        } else if (params.type_enum() == game_data_type::ARRAY) {
            auto& arr = params.to_array();
            
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
            event_args = arr.size() > 1 ? arr[1] : game_value();
            target = arr.size() > 2 ? arr[2] : game_value("GLOBAL");
            jip = arr.size() > 3 ? arr[3] : game_value();
        } else {
            report_error("String or array parameter required");
            return game_value();
        }
        
        // Check if we should use CBA events
        if (!target.is_nil()) {            
            // Build CBA parameters: ["KH_eve_luaEventTrigger", [event_name, event_args], target, jip]
            auto_array<game_value> cba_event_data;
            cba_event_data.push_back(game_value(event_name));
            cba_event_data.push_back(event_args);
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_luaEventTrigger"));
            cba_params.push_back(game_value(std::move(cba_event_data)));
            cba_params.push_back(target);
            cba_params.push_back(jip);
            sqf::call2(g_trigger_cba_event, game_value(std::move(cba_params)));
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
            // Simple string: just variable name, fetch from Lua
            var_name = static_cast<std::string>(params);
            
            // Get value from Lua global
            sol::object lua_var = lua[var_name];

            if (lua_var.valid()) {
                emit_value = convert_lua_to_game_value(lua_var);
            } else {
                report_error("Lua global variable '" + var_name + "' not found");
                return game_value();
            }
        } else if (params.type_enum() == game_data_type::ARRAY) {
            auto& arr = params.to_array();
            
            if (arr.empty() || arr[0].type_enum() != game_data_type::STRING) {
                report_error("First element must be variable name (string)");
                return game_value();
            }
            
            var_name = static_cast<std::string>(arr[0]);
            
            if (arr.size() == 1) {
                // Just variable name, fetch from Lua
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
                if (arr.size() > 2 && !arr[2].is_nil()) {
                    target = arr[2];
                }
                
                // JIP
                if (arr.size() > 3) {
                    jip = arr[3];
                }
            }
        } else {
            report_error("Parameter must be string or array");
            return game_value();
        }
        
        // Build CBA parameters: ["KH_eve_luaVariableEmission", [var_name, emit_value], target, jip]
        auto_array<game_value> emission_data;
        emission_data.push_back(game_value(var_name));
        emission_data.push_back(emit_value);
        auto_array<game_value> cba_params;
        cba_params.push_back(game_value("KH_eve_luaVariableEmission"));
        cba_params.push_back(game_value(std::move(emission_data)));
        cba_params.push_back(target);
        cba_params.push_back(jip);
        sqf::call2(g_trigger_cba_event, game_value(std::move(cba_params)));
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
        
        if (params.type_enum() == game_data_type::ARRAY) {
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
            
            // Optional target
            if (arr.size() > 2 && !arr[2].is_nil()) {
                target = arr[2];
            }
            
            // Optional JIP
            if (arr.size() > 3) {
                jip = arr[3];
            }
        } else {
            report_error("Parameter must be array [name, value, target?, jip?]");
            return game_value();
        }
        
        if (var_name.empty()) {
            report_error("Variable name cannot be empty");
            return game_value();
        }
        
        // If target is specified, emit via CBA event
        if (!target.is_nil()) {
            // Build CBA parameters: ["KH_eve_luaVariableEmission", [var_name, set_value], target, jip]
            auto_array<game_value> emission_data;
            emission_data.push_back(game_value(var_name));
            emission_data.push_back(set_value);
            auto_array<game_value> cba_params;
            cba_params.push_back(game_value("KH_eve_luaVariableEmission"));
            cba_params.push_back(game_value(std::move(emission_data)));
            cba_params.push_back(target);
            cba_params.push_back(jip);
            sqf::call2(g_trigger_cba_event, game_value(std::move(cba_params)));
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
            // Simple string - just variable name
            var_name = static_cast<std::string>(params);
        } else if (params.type_enum() == game_data_type::ARRAY) {
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
        } else {
            report_error("Parameter must be string or array [name, defaultValue?]");
            return game_value();
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
            return game_value();  // Return nil
        }
        
        // Convert Lua value to game_value
        return convert_lua_to_game_value(lua_var);
        
    } catch (const std::exception& e) {
        report_error("Failed to get Lua variable: " + std::string(e.what()));
        return game_value();
    } catch (...) {
        report_error("Failed to get Lua variable");
        return game_value();
    }
}

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

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start() {
    _execute_lua_sqf_command = intercept::client::host::register_sqf_command(
        "luaExecute",
        "Execute Lua code or function.",
        userFunctionWrapper<execute_lua_sqf>,
        game_data_type::ANY,     // Return type
        game_data_type::ANY,     // Left argument - arguments
        game_data_type::STRING   // Right argument - function name or code
    );

    _compile_lua_sqf_command = intercept::client::host::register_sqf_command(
        "luaCompile",
        "Compile Lua code and register it as a named function",
        userFunctionWrapper<compile_lua_sqf>,
        game_data_type::NOTHING,     // Return type
        game_data_type::STRING,  // Left argument - name
        game_data_type::STRING   // Right argument - code
    );

    _crypto_hash_sqf_command = intercept::client::host::register_sqf_command(
        "cryptoHash",
        "Compute cryptographic hash of input string",
        userFunctionWrapper<crypto_hash_sqf>,
        game_data_type::STRING,   // Return type
        game_data_type::STRING,   // Left argument - hash type
        game_data_type::STRING    // Right argument - input string
    );

    _generate_random_string_sqf_command = intercept::client::host::register_sqf_command(
        "generateRandomString", 
        "Generate random string with specified length and character sets",
        userFunctionWrapper<generate_random_string_sqf>,
        game_data_type::STRING,   // Return type
        game_data_type::ARRAY,   // Right argument - options [numbers, letters, symbols]
        game_data_type::SCALAR     // Left argument - length
    );

    _generate_uid_sqf_command = intercept::client::host::register_sqf_command(
        "generateUid",
        "Generate a unique identifier",
        userFunctionWrapper<generate_uid_sqf>,
        game_data_type::STRING   // Return type
    );

    _write_khdata_sqf_command = intercept::client::host::register_sqf_command(
        "writeKhData",
        "Write variable to KHData file with optional broadcast",
        userFunctionWrapper<write_khdata_sqf>,
        game_data_type::NOTHING,     // Return type
        game_data_type::STRING,      // Left argument - filename
        game_data_type::ARRAY        // Right argument - [variableName, value, target?, jip?]
    );

    _read_khdata_sqf_command = intercept::client::host::register_sqf_command(
        "readKhData",
        "Read variable from KHData file with optional default",
        userFunctionWrapper<read_khdata_sqf>,
        game_data_type::ANY,         // Return type
        game_data_type::STRING,      // Left argument - file name
        game_data_type::ANY          // Right argument - variable name or [variableName, defaultValue?]
    );

    _flush_khdata_sqf_command = intercept::client::host::register_sqf_command(
        "flushKhData",
        "Flush all dirty KHData files to disk",
        userFunctionWrapper<flush_khdata_sqf>,
        game_data_type::NOTHING      // Return type
    );

    _delete_khdata_file_sqf_command = intercept::client::host::register_sqf_command(
        "deleteKhDataFile",
        "Delete KHData file",
        userFunctionWrapper<delete_khdata_file_sqf>,
        game_data_type::NOTHING,     // Return type
        game_data_type::STRING      // Argument - file name
    );

    _get_terrain_matrix_sqf_command = intercept::client::host::register_sqf_command(
        "getTerrainMatrix",
        "Get the pre-calculated terrain height matrix",
        userFunctionWrapper<get_terrain_matrix_sqf>,
        game_data_type::ARRAY      // Return type
    );

    _trigger_lua_event_sqf_command = intercept::client::host::register_sqf_command(
        "luaTriggerEvent",
        "Trigger Lua event handlers",
        userFunctionWrapper<trigger_lua_event_sqf>,
        game_data_type::NOTHING,    // Return type
        game_data_type::ANY    // Event name or params in format [event, arguments?, target?, jip?]
    );

    _trigger_lua_event_local_sqf_command = intercept::client::host::register_sqf_command(
        "luaTriggerEventLocal",
        "Trigger local Lua event handlers",
        userFunctionWrapper<trigger_lua_event_local_sqf>,
        game_data_type::NOTHING, // Return type
        game_data_type::ANY    // Event name or params in format [event, arguments?]
    );

    _emit_lua_variable_sqf_command = intercept::client::host::register_sqf_command(
        "luaEmitVariable",
        "Emit Lua variable",
        userFunctionWrapper<emit_lua_variable_sqf>,
        game_data_type::NOTHING,    // Return type
        game_data_type::ANY    // String or array [name, value?, target?, jip?]
    );

    _lua_set_variable_sqf_command = intercept::client::host::register_sqf_command(
        "luaSetVariable",
        "Set Lua global variable with optional emission",
        userFunctionWrapper<lua_set_variable_sqf>,
        game_data_type::NOTHING,    // Return type
        game_data_type::ARRAY        // Right argument - [name, value, target?, jip?]
    );

    _lua_get_variable_sqf_command = intercept::client::host::register_sqf_command(
        "luaGetVariable",
        "Get Lua global variable with optional default value",
        userFunctionWrapper<lua_get_variable_sqf>,
        game_data_type::ANY,         // Return type
        game_data_type::ANY          // Right argument - name or [name, defaultValue?]
    );
    
    g_trigger_cba_event = sqf::compile("_this call KH_fnc_triggerCbaEvent;");
    g_add_game_event_handler = sqf::compile("_this call KH_fnc_addEventHandler;");
    g_remove_game_event_handler = sqf::compile("_this call KH_fnc_removeEventHandler;");
    g_game_event_handler_bridge = sqf::compile("luaTriggerEventLocal [_args, _this];");
    g_remote_execute = sqf::compile("_this set [1, ['_this luaExecute ', _this#1] joinString '']; _this call KH_fnc_execute;");
    g_create_hash_map_from_array = sqf::compile("createHashMapFromArray _this;");
    g_create_hash_map = sqf::compile("createHashMap;");
    initialize_lua_state();
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = false;
    game["postInit"] = false;
    game["frame"] = g_current_game_frame;
    game["time"] = g_current_game_time;
    game["server"] = sqf::is_server();
    game["dedicated"] = sqf::is_dedicated();
    game["headless"] = (!(sqf::is_server()) && !(sqf::has_interface()));
    game["player"] = sqf::has_interface();
    mission["frame"] = g_current_mission_frame;
    mission["time"] = g_current_mission_time;
    mission["active"] = false;
    KHDataManager::instance().initialize();
    sqf::diag_log("KH Framework - Pre-start");
}

void intercept::pre_init() {
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = true;
    game["postInit"] = false;
    game["server"] = sqf::is_server();
    game["dedicated"] = sqf::is_dedicated();
    game["headless"] = (!(sqf::is_server()) && !(sqf::has_interface()));
    game["player"] = sqf::has_interface();
    g_current_mission_time = 0.0f;
    g_current_mission_frame = 0;
    mission["frame"] = g_current_mission_frame;
    mission["time"] = g_current_mission_time;
    mission["active"] = true;
    initialize_terrain_matrix();
    g_call_cache.clear();
    g_code_cache.clear();
    g_sqf_compiled_cache.clear();
    g_sqf_function_cache.clear();
    g_sqf_command_cache.clear();
    LuaFunctions::lua_scheduled_tasks.clear();
    LuaFunctions::lua_task_id_map.clear();
    LuaFunctions::lua_event_handlers.clear();
    LuaFunctions::next_task_id = 1;
    KHDataManager::instance().flush_all();
    sqf::diag_log("KH Framework - Pre-init");
}

void intercept::post_init() {
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    game["preInit"] = false;
    game["postInit"] = true;
    sqf::diag_log("KH Framework - Post-init");
}

void intercept::on_frame() {
    LuaStackGuard guard(*g_lua_state);
    float current_delta = sqf::diag_delta_time();
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    g_current_game_time += current_delta;
    g_current_game_frame++;
    game["frame"] = g_current_game_frame;
    game["time"] = g_current_game_time;
    g_current_mission_time += current_delta;
    g_current_mission_frame++;
    mission["frame"] = g_current_mission_frame;
    mission["time"] = g_current_mission_time;
    LuaFunctions::update_scheduler();
}

void intercept::mission_ended() {
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = false;
    game["postInit"] = false;
    g_current_mission_time = 0.0f;
    g_current_mission_frame = 0;
    mission["frame"] = g_current_mission_frame;
    mission["time"] = g_current_mission_time;
    mission["active"] = false;
    g_call_cache.clear();
    g_code_cache.clear();
    g_sqf_compiled_cache.clear();
    g_sqf_function_cache.clear();
    g_sqf_command_cache.clear();
    LuaFunctions::lua_scheduled_tasks.clear();
    LuaFunctions::lua_task_id_map.clear();
    LuaFunctions::lua_event_handlers.clear();
    LuaFunctions::next_task_id = 1;
    KHDataManager::instance().flush_all();
    sqf::diag_log("KH Framework - Mission End");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            break;
            
        case DLL_PROCESS_DETACH:
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}