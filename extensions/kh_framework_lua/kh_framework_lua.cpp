#include <windows.h>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <memory>
#include <unordered_map>
#include <wincrypt.h>

#include "intercept/include/intercept.hpp"
#include "intercept/include/client/sqf/sqf.hpp"
#include "intercept/include/client/pointers.hpp"
#include "sol/sol.hpp"

// Crypto provider singleton
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
        if (!hProv) return "";
        
        HCRYPTHASH hHash = 0;
        if (!CryptCreateHash(hProv, algorithm, 0, 0, &hHash)) {
            return "";
        }
        
        if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.c_str()), 
                        static_cast<DWORD>(input.length()), 0)) {
            CryptDestroyHash(hHash);
            return "";
        }
        
        DWORD hashSize = 0;
        DWORD sizeSize = sizeof(DWORD);

        if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashSize), &sizeSize, 0)) {
            CryptDestroyHash(hHash);
            return "";
        }
        
        std::vector<uint8_t> hashData(hashSize);

        if (!CryptGetHashParam(hHash, HP_HASHVAL, hashData.data(), &hashSize, 0)) {
            CryptDestroyHash(hHash);
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

    static std::string fnv1a_32(const std::string& input) {
        uint32_t hash = 0x811c9dc5u;

        for (char c : input) {
            hash ^= static_cast<uint32_t>(static_cast<uint8_t>(c));
            hash *= 0x01000193u;
        }

        std::vector<uint8_t> bytes(4);

        for (int i = 0; i < 4; i++) {
            bytes[i] = (hash >> (i * 8)) & 0xFF;
        }

        return bytes_to_hex(bytes);
    }

    static std::string fnv1a_64(const std::string& input) {
        uint64_t hash = 0xcbf29ce484222325ull;

        for (char c : input) {
            hash ^= static_cast<uint64_t>(static_cast<uint8_t>(c));
            hash *= 0x100000001b3ull;
        }

        std::vector<uint8_t> bytes(8);

        for (int i = 0; i < 8; i++) {
            bytes[i] = (hash >> (i * 8)) & 0xFF;
        }

        return bytes_to_hex(bytes);
    }

    static std::string crc32(const std::string& input) {
        static bool table_initialized = false;
        static uint32_t crc_table[256];
        
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
        for (char c : input) {
            uint8_t byte = static_cast<uint8_t>(c);
            crc = crc_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
        }

        crc ^= 0xFFFFFFFFu;
        
        std::vector<uint8_t> bytes(4);

        for (int i = 0; i < 4; i++) {
            bytes[i] = (crc >> (i * 8)) & 0xFF;
        }

        return bytes_to_hex(bytes);
    }

    static std::string xxhash32(const std::string& input) {
        const uint32_t PRIME32_1 = 0x9E3779B1u;
        const uint32_t PRIME32_2 = 0x85EBCA77u;
        const uint32_t PRIME32_3 = 0xC2B2AE3Du;
        const uint32_t PRIME32_4 = 0x27D4EB2Fu;
        const uint32_t PRIME32_5 = 0x165667B1u;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.c_str());
        size_t len = input.length();
        uint32_t seed = 0;
        uint32_t hash;
        
        if (len >= 16) {
            const uint8_t* limit = data + len - 16;
            uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
            uint32_t v2 = seed + PRIME32_2;
            uint32_t v3 = seed + 0;
            uint32_t v4 = seed - PRIME32_1;
            
            do {
                uint32_t k1, k2, k3, k4;
                memcpy(&k1, data, 4); data += 4;
                memcpy(&k2, data, 4); data += 4;
                memcpy(&k3, data, 4); data += 4;
                memcpy(&k4, data, 4); data += 4;
                v1 = ((v1 + k1 * PRIME32_2) << 13) | ((v1 + k1 * PRIME32_2) >> 19);
                v1 *= PRIME32_1;
                v2 = ((v2 + k2 * PRIME32_2) << 13) | ((v2 + k2 * PRIME32_2) >> 19);
                v2 *= PRIME32_1;
                v3 = ((v3 + k3 * PRIME32_2) << 13) | ((v3 + k3 * PRIME32_2) >> 19);
                v3 *= PRIME32_1;
                v4 = ((v4 + k4 * PRIME32_2) << 13) | ((v4 + k4 * PRIME32_2) >> 19);
                v4 *= PRIME32_1;
            } while (data <= limit);
            
            hash = ((v1 << 1) | (v1 >> 31)) + ((v2 << 7) | (v2 >> 25)) + 
                ((v3 << 12) | (v3 >> 20)) + ((v4 << 18) | (v4 >> 14));
        } else {
            hash = seed + PRIME32_5;
        }
        
        hash += static_cast<uint32_t>(len);
        
        const uint8_t* end = reinterpret_cast<const uint8_t*>(input.c_str()) + len;

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
        
        while (data < end) {
            uint32_t k1 = *data;
            k1 *= PRIME32_5;
            k1 = (k1 << 11) | (k1 >> 21);
            k1 *= PRIME32_1;
            hash ^= k1;
            hash = ((hash << 15) | (hash >> 17)) * PRIME32_2 + PRIME32_3;
            data++;
        }
        
        hash ^= hash >> 16;
        hash *= PRIME32_2;
        hash ^= hash >> 13;
        hash *= PRIME32_3;
        hash ^= hash >> 16;
        
        std::vector<uint8_t> bytes(4);

        for (int i = 0; i < 4; i++) {
            bytes[i] = (hash >> (i * 8)) & 0xFF;
        }

        return bytes_to_hex(bytes);
    }

    static std::string adler32(const std::string& input) {
        uint32_t a = 1, b = 0;
        const uint32_t MOD_ADLER = 65521;
        
        for (char c : input) {
            a = (a + static_cast<uint8_t>(c)) % MOD_ADLER;
            b = (b + a) % MOD_ADLER;
        }
        
        uint32_t checksum = (b << 16) | a;
        std::vector<uint8_t> bytes(4);

        for (int i = 0; i < 4; i++) {
            bytes[i] = (checksum >> (i * 8)) & 0xFF;
        }

        return bytes_to_hex(bytes);
    }

    static std::string djb2(const std::string& input) {
        uint32_t hash = 5381;

        for (char c : input) {
            hash = ((hash << 5) + hash) + static_cast<uint8_t>(c);
        }
        
        std::vector<uint8_t> bytes(4);

        for (int i = 0; i < 4; i++) {
            bytes[i] = (hash >> (i * 8)) & 0xFF;
        }

        return bytes_to_hex(bytes);
    }

    static std::string sdbm(const std::string& input) {
        uint32_t hash = 0;

        for (char c : input) {
            hash = static_cast<uint8_t>(c) + (hash << 6) + (hash << 16) - hash;
        }
        
        std::vector<uint8_t> bytes(4);

        for (int i = 0; i < 4; i++) {
            bytes[i] = (hash >> (i * 8)) & 0xFF;
        }
        
        return bytes_to_hex(bytes);
    }
};

// Random string generation
class RandomStringGenerator {
private:
    static std::mt19937& get_rng() {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
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
            // Default to numbers and letters
            charset = "0123456789abcdefghijklmnopqrstuvwxyz";
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

using namespace intercept;

static types::registered_sqf_function _execute_lua_sqf_command;
static types::registered_sqf_function _compile_lua_sqf_command;
static types::registered_sqf_function _crypto_hash_sqf_command;
static types::registered_sqf_function _generate_random_string_sqf_command;
static std::unique_ptr<sol::state> g_lua_state;

// Userdata wrapper for game_value to preserve native Arma types
struct GameValueWrapper {
    game_value value;
    static constexpr const char* TYPE_IDENTIFIER = "GameValueWrapper";
    
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
            case types::game_data_type::HASHMAP: return "[HASHMAP]";
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
            case types::game_data_type::HASHMAP: return "HASHMAP";
            default: return "UNKNOWN";
        }
    }
    
    // Identification method
    bool is_game_value() const { return true; }
};

struct LuaCallCache {
    std::string function_name;
    sol::protected_function func;
    bool is_valid;
};

static std::unordered_map<std::string, LuaCallCache> g_call_cache;
static std::unordered_map<size_t, sol::protected_function> g_code_cache;

class Lua_Compilation {
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
            
            // Register in Lua global namespace
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

    static void reset_compilation_state() {
        g_call_cache.clear();
        g_code_cache.clear();
    }
};

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

        case types::game_data_type::HASHMAP: {
            sol::table lua_table = lua.create_table();
            auto& hashmap = value.to_hashmap();
            
            // Convert directly to native Lua table format
            for (auto& pair : hashmap) {
                sol::object lua_key = convert_game_value_to_lua(pair.key);
                sol::object lua_value = convert_game_value_to_lua(pair.value);
                
                // Set the key-value pair in the Lua table
                // This works for all key types that Lua supports
                lua_table[lua_key] = lua_value;
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
                return sqf::call2(sqf::compile("createHashMapFromArray _this"), game_value(std::move(kv_array)));
            } else {
                return sqf::call2(sqf::compile("createHashMap"));
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
namespace lua_functions {
    static int lua_init() {
        (*g_lua_state)["GameFrame"] = 0;
        return 0;
    }
    
    static int lua_frame_update() {
        int frame = (*g_lua_state)["GameFrame"].get_or(0);
        (*g_lua_state)["GameFrame"] = frame + 1;
        return 0;
    }
    
    static int lua_deinit() {
        (*g_lua_state)["GameFrame"] = 0;
        return 0;
    }

    // Get system time in seconds with millisecond precision
    static double get_system_time() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count() / 1000.0;
    }
    
    // Get formatted date/time string
    static std::string get_date_time() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
        return std::string(buffer);
    }

    // Get high-resolution timestamp in seconds (for delta calculations)
    static double get_timestamp() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        return microseconds / 1000000.0;  // Convert to seconds
    }

    // Get system time in seconds with millisecond precision
    static double get_time_ms() {
        LARGE_INTEGER frequency, counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        return (counter.QuadPart * 1000.0) / frequency.QuadPart;
    }
    
    // Profile code execution
    static sol::object profile_code(sol::variadic_args args) {
        sol::state& lua = *g_lua_state;
        
        if (args.size() < 2) {
            return sol::make_object(lua, "ERROR: Not enough arguments");
        }
        
        sol::object params = args[0];
        sol::object code_obj = args[1];
        sol::object count_obj = args.size() > 2 ? args[2] : sol::make_object(lua, 1);
        
        // Validate code parameter
        if (code_obj.get_type() != sol::type::string) {
            return sol::make_object(lua, "ERROR: code must be a string");
        }
        
        std::string code = code_obj.as<std::string>();
        int count = count_obj.get_type() == sol::type::number ? count_obj.as<int>() : 1;
        
        if (count < 1) {
            return sol::make_object(lua, "ERROR: execution count must be at least 1");
        }
        
        sol::protected_function compiled;
        
        // Check if it's a function name or code to compile
        if (code.find(' ') == std::string::npos && 
            code.find('\t') == std::string::npos && 
            code.find('\n') == std::string::npos) {
            
            // It's a function name
            sol::object func = lua[code];

            if (func.get_type() != sol::type::function) {
                return sol::make_object(lua, "ERROR: Function '" + code + "' not found or is not a function");
            }
            compiled = func;
        } else {
            // Compile the code
            std::string full_code = "return function(...) " + code + " end";
            sol::load_result load_res = lua.load(full_code);
            
            if (!load_res.valid()) {
                sol::error err = load_res;
                return sol::make_object(lua, "ERROR: Failed to compile code: " + std::string(err.what()));
            }
            
            sol::protected_function factory = load_res;
            auto factory_result = factory();

            if (!factory_result.valid()) {
                sol::error err = factory_result;
                return sol::make_object(lua, "ERROR: Failed to create function: " + std::string(err.what()));
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
        sol::function get_time = lua["GetTimeMs"];

        if (!get_time.valid()) {
            return sol::make_object(lua, "ERROR: High precision timer not available");
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
    }

    // Get the data type of a value
    static std::string get_data_type(sol::object input) {
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
    }

    /*
    Execute in Sqf Namespace

    local result = with_sqf(function(x, y)
        diag_log("Test: " .. tostring(x))  -- sqf function without prefix
        local p = player()  -- works without sqf.
        return x + y
    end, 10, 20)
    */
    static sol::object with_sqf(sol::protected_function func, sol::variadic_args args) {
        sol::state& lua = *g_lua_state;
        sol::table env = lua.create_table();
        sol::table meta = lua.create_table();
        
        // __index function that checks sqf first, then _G
        meta["__index"] = [&lua](sol::table t, sol::object key) -> sol::object {
            // First check sqf table
            sol::table sqf_table = lua["sqf"];
            sol::object sqf_result = sqf_table[key];
            
            if (sqf_result.valid() && sqf_result.get_type() != sol::type::nil) {
                return sqf_result;
            }
            
            // Fall back to global namespace
            return lua.globals()[key];
        };
        
        // __newindex delegates to _G
        meta["__newindex"] = lua.globals();
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
    }

    static sol::object sqf_function_call(sol::object params, std::string function_name) {
        try {            
            // Handle parameters
            if (params.get_type() == sol::type::nil) {
                return convert_game_value_to_lua(sqf::call2(sqf::compile("call " + function_name)));
            } else {
                return convert_game_value_to_lua(sqf::call2(sqf::compile("_this call " + function_name), convert_lua_to_game_value(params)));
            }
        } catch (const std::exception& e) {
            sol::state_view lua(params.lua_state());
            return sol::make_object(lua, "ERROR: " + std::string(e.what()));
        } catch (...) {
            sol::state_view lua(params.lua_state());
            return sol::make_object(lua, "ERROR: Unknown error in SQF_FunctionCall");
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
        };

        (*g_lua_state)["crypto"] = g_lua_state->create_table();

        (*g_lua_state)["crypto"]["md5"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::md5(input);
        };

        (*g_lua_state)["crypto"]["sha1"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::sha1(input);
        };

        (*g_lua_state)["crypto"]["sha256"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::sha256(input);
        };

        (*g_lua_state)["crypto"]["sha512"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::sha512(input);
        };

        (*g_lua_state)["crypto"]["fnv1a32"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::fnv1a_32(input);
        };

        (*g_lua_state)["crypto"]["fnv1a64"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::fnv1a_64(input);
        };

        (*g_lua_state)["crypto"]["crc32"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::crc32(input);
        };

        (*g_lua_state)["crypto"]["xxhash32"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::xxhash32(input);
        };

        (*g_lua_state)["crypto"]["adler32"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::adler32(input);
        };

        (*g_lua_state)["crypto"]["djb2"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::djb2(input);
        };

        (*g_lua_state)["crypto"]["sdbm"] = [](const std::string& input) -> std::string {
            return CryptoGenerator::sdbm(input);
        };

        // Register random string generator
        (*g_lua_state)["GenerateRandomString"] = [](int length, sol::optional<bool> use_numbers, 
                                                    sol::optional<bool> use_letters, 
                                                    sol::optional<bool> use_symbols) -> std::string {
            bool nums = use_numbers.value_or(true);
            bool letters = use_letters.value_or(true);
            bool syms = use_symbols.value_or(false);
            return RandomStringGenerator::generate(length, nums, letters, syms);
        };

        // Register GameValueWrapper userdata type
        g_lua_state->new_usertype<GameValueWrapper>("GameValue",
            sol::constructors<GameValueWrapper(), GameValueWrapper(const game_value&)>(),
            sol::meta_function::to_string, &GameValueWrapper::to_string,
            sol::meta_function::equal_to, &GameValueWrapper::equals,
            "value", &GameValueWrapper::value,
            "type_name", &GameValueWrapper::type_name,
            "is_game_value", &GameValueWrapper::is_game_value
        );
        
        // Register variables in Lua global namespace
        (*g_lua_state)["GameFrame"] = 0;

        // Register all utility functions in Lua global namespace
        (*g_lua_state)["GetSystemTime"] = lua_functions::get_system_time;
        (*g_lua_state)["GetDateTime"] = lua_functions::get_date_time;
        (*g_lua_state)["GetTimestamp"] = lua_functions::get_timestamp;
        (*g_lua_state)["GetTimeMs"] = lua_functions::get_time_ms;
        (*g_lua_state)["ProfileCode"] = lua_functions::profile_code;
        (*g_lua_state)["GetDataType"] = lua_functions::get_data_type;
        (*g_lua_state)["with_sqf"] = lua_functions::with_sqf;
        (*g_lua_state)["SQF_FunctionCall"] = lua_functions::sqf_function_call;

        auto command_handler = [](std::string cmd, sol::variadic_args args) -> sol::object {
            if (args.size() == 0) {
                return convert_game_value_to_lua(sqf::call2(sqf::compile(cmd)));
            } else if (args.size() == 1) {
                return convert_game_value_to_lua(sqf::call2(sqf::compile(cmd + " _this"), convert_lua_to_game_value(args[0])));
            } else if (args.size() == 2) {
                // Compile the binary command pattern and call with the array
                return convert_game_value_to_lua(sqf::call2(
                    sqf::compile("(_this#0) " + cmd + " (_this#1)"), 
                    game_value({convert_lua_to_game_value(args[0]), convert_lua_to_game_value(args[1])})
                ));
            } else {
                sol::state_view lua(args.lua_state());
                return sol::make_object(lua, "ERROR: SQF commands only support 0-2 arguments (no argument, right argument, or left and right argument)");
            }
        };

        // Create sqf table for SQF functions, limited commands
        auto sqf_table = g_lua_state->create_named_table("sqf");
        sol::table sqf_metatable = g_lua_state->create_table();

        sqf_metatable["__index"] = [command_handler](sol::table table, std::string key) -> sol::object {
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

        sqf_table["diag_captureFrame"] = [](float frame) -> sol::object {
            sqf::diag_capture_frame(frame);
            return sol::nil;
        };

        sqf_table["diag_captureFrameToFile"] = [](float frame) -> sol::object {
            sqf::diag_capture_frame_to_file(frame);
            return sol::nil;
        };

        sqf_table["diag_captureSlowFrame"] = [](sqf_string_const_ref section, float threshold) -> sol::object {
            sqf::diag_capture_slow_frame(section, threshold);
            return sol::nil;
        };

        sqf_table["diag_log"] = [](sqf_string_const_ref text) -> sol::object {
            sqf::diag_log(text);
            return sol::nil;
        };

        sqf_table["diag_logSlowFrame"] = [](sqf_string_const_ref section, float threshold) -> sol::object {
            sqf::diag_log_slow_frame(section, threshold);
            return sol::nil;
        };

        sqf_table["call"] = [](sol::variadic_args args) -> sol::object {
            if (args[0].get_type() != sol::type::string) {
                return sol::nil;
            }

            if (args.size() == 1) {
                return convert_game_value_to_lua(sqf::call2(sqf::compile(args[0].as<std::string>())));
            } else if (args.size() == 2) {
                return convert_game_value_to_lua(sqf::call2(sqf::compile(args[0].as<std::string>()), convert_lua_to_game_value(args[1])));
            }
            
            return sol::nil;
        };

        sqf_table["isNil"] = [](sol::object input) -> bool {
            if (input.get_type() != sol::type::userdata) {
                return false;
            } else {
                return sqf::is_nil(convert_lua_to_game_value(input));
            }

            return true;
        };

        sqf_table["isNull"] = [](sol::object input) -> bool {
            game_value gv = convert_lua_to_game_value(input);
            return gv.is_null();
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
        std::string code_str = static_cast<std::string>(code_or_function);
        
        // Check if it's a function call or code execution
        bool is_function = code_str.find(' ') == std::string::npos && code_str.find('(') == std::string::npos;
        sol::protected_function_result result;
        
        if (is_function) {
            // Try to get the function from cache or global namespace
            auto cache_it = g_call_cache.find(code_str);
            sol::protected_function func;
            
            if (cache_it != g_call_cache.end()) {
                func = cache_it->second.func;
            } else {
                func = (*g_lua_state)[code_str];

                if (!func.valid()) {
                    return game_value("ERROR: Function '" + code_str + "' not found");
                }
                
                g_call_cache[code_str] = {code_str, func, true};
            }
            
            // Call the function with arguments
            if (args.is_nil()) {
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
                    return game_value("ERROR: " + std::string(err.what()));
                }
                
                sol::protected_function factory = load_res;
                auto factory_result = factory();

                if (!factory_result.valid()) {
                    sol::error err = factory_result;
                    return game_value("ERROR: Failed to create function: " + std::string(err.what()));
                }
                
                compiled_code = factory_result;
                g_code_cache[code_hash] = compiled_code;
            }
            
            // Execute with arguments
            if (args.is_nil()) {
                result = compiled_code();
            } else {
                result = compiled_code(convert_game_value_to_lua(args));
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
        } else {
            return convert_lua_to_game_value(result.get<sol::object>());
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
        
        auto result = Lua_Compilation::lua_compile(lua_code, lua_name);
        
        if (result.success) {
            if (!lua_name.empty() && result.function.valid()) {
                // Update call cache
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
            result = CryptoGenerator::fnv1a_32(input_str);
        } else if (hash_type == "fnv1a64") {
            result = CryptoGenerator::fnv1a_64(input_str);
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
            return game_value("ERROR: Unknown hash type: " + hash_type);
        }
        
        return game_value(result);
    } catch (const std::exception& e) {
        return game_value("ERROR: " + std::string(e.what()));
    }
}

static game_value generate_random_string_sqf(game_value_parameter options, game_value_parameter length) {
    try {
        int len = static_cast<int>(static_cast<float>(length));
        
        if (len <= 0) {
            return game_value("ERROR: Length must be positive");
        }
        
        bool use_numbers = true;
        bool use_letters = true;
        bool use_symbols = true;
        
        // Parse options if provided (expects array of 3 bools)
        if (options.type_enum() == types::game_data_type::ARRAY) {
            auto& arr = options.to_array();
            
            // Default to false for each element if array is provided
            use_numbers = false;
            use_letters = false;
            use_symbols = false;
            
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
        
        std::string result = RandomStringGenerator::generate(
            len, use_numbers, use_letters, use_symbols);
        
        return game_value(result);
    } catch (const std::exception& e) {
        return game_value("ERROR: " + std::string(e.what()));
    }
}

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start() {
    initialize_lua_state();
    (*g_lua_state)["MissionActive"] = false;

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

    _crypto_hash_sqf_command = intercept::client::host::register_sqf_command(
        "cryptoHash",
        "Compute cryptographic hash of input string",
        userFunctionWrapper<crypto_hash_sqf>,
        types::game_data_type::STRING,   // Return type
        types::game_data_type::STRING,   // Left argument - hash type
        types::game_data_type::STRING    // Right argument - input string
    );

    _generate_random_string_sqf_command = intercept::client::host::register_sqf_command(
        "generateRandomString", 
        "Generate random string with specified length and character sets",
        userFunctionWrapper<generate_random_string_sqf>,
        types::game_data_type::STRING,   // Return type
        types::game_data_type::ARRAY,   // Right argument - options [useNumbers, useLetters, useSymbols]
        types::game_data_type::SCALAR     // Left argument - length
    );

    sqf::diag_log("KH Framework Lua - Pre-start Complete");
}

void intercept::pre_init() {
    (*g_lua_state)["MissionActive"] = true;

    if (sqf::is_server()) {
        (*g_lua_state)["MachineIsServer"] = true;
    }
    else {
        (*g_lua_state)["MachineIsServer"] = false;
    }

    if (sqf::is_dedicated()) {
        (*g_lua_state)["MachineIsDedicatedServer"] = true;
    }
    else {
        (*g_lua_state)["MachineIsDedicatedServer"] = false;
    }

    if (!(sqf::is_server()) && !(sqf::has_interface())) {
        (*g_lua_state)["MachineIsHeadless"] = true;
    }
    else {
        (*g_lua_state)["MachineIsHeadless"] = false;
    }

    if (sqf::has_interface()) {
        (*g_lua_state)["MachineIsPlayer"] = true;
    }
    else {
        (*g_lua_state)["MachineIsPlayer"] = false;
    }

    lua_functions::lua_init();
    sqf::diag_log("KH Framework Lua - Pre-init Complete");
}

void intercept::post_init() {
    sqf::diag_log("KH Framework Lua - Post-init Complete");
}

void intercept::on_frame() {
    lua_functions::lua_frame_update();
}

void intercept::mission_ended() {
    (*g_lua_state)["MissionActive"] = false;
    lua_functions::lua_deinit();
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