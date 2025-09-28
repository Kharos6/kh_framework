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

using namespace intercept;
constexpr uint32_t KHDATA_MAGIC = 0x5444484B; // "KHDT"
constexpr uint32_t KHDATA_VERSION = 1;

class ErrorHandling {
public:
    // Core error handler that logs and throws
    static void report_error(const std::string& error_message) {
        sqf::diag_log(error_message);
        sqf::throw_exception(error_message);
    }
    
    // Overload with context prefix
    static void report_error(const std::string& context, const std::string& error_message) {
        std::string full_message = context + ": " + error_message;
        sqf::diag_log(full_message);
        sqf::throw_exception(full_message);
    }
};

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

        if (!hProv || !CryptoProvider::is_initialized()) {
            ErrorHandling::report_error("CryptoProvider not initialized - Windows cryptography unavailable");
            // Return a fallback or empty string instead of continuing
            return "";
        }
            
        HCRYPTHASH hHash = 0;

        if (!CryptCreateHash(hProv, algorithm, 0, 0, &hHash)) {
            ErrorHandling::report_error("Failed to create hash context for algorithm " + std::to_string(algorithm));
            return "";
        }
        
        if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.c_str()), 
                        static_cast<DWORD>(input.length()), 0)) {
            CryptDestroyHash(hHash);
            ErrorHandling::report_error("Failed to hash data");
            return "";
        }
        
        DWORD hashSize = 0;
        DWORD sizeSize = sizeof(DWORD);

        if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashSize), &sizeSize, 0)) {
            CryptDestroyHash(hHash);
            ErrorHandling::report_error("Failed to get hash size");
            return "";
        }
        
        std::vector<uint8_t> hashData(hashSize);

        if (!CryptGetHashParam(hHash, HP_HASHVAL, hashData.data(), &hashSize, 0)) {
            CryptDestroyHash(hHash);
            ErrorHandling::report_error("Failed to get hash value");
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

// KHData file representation
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

// Main KHData manager singleton
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
            ErrorHandling::report_error("Failed to write type");
            throw std::runtime_error("Failed to write type");
        }

        if (len > 0) {
            stream.write(str.c_str(), len);

            if (!stream.good()) {
                ErrorHandling::report_error("Failed to write type");
                throw std::runtime_error("Failed to write type");
            }
        }
    }
    
    static std::string read_string(std::ifstream& stream) {
        uint32_t len;
        stream.read(reinterpret_cast<char*>(&len), sizeof(len));

        if (!stream.good()) {
            ErrorHandling::report_error("Failed to read string length");
            throw std::runtime_error("Failed to read string length");
        }

        std::string str(len, '\0');
        stream.read(&str[0], len);

        if (!stream.good() && len > 0) {
            ErrorHandling::report_error("Failed to read string data");
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
                needs_special_handling = true;
                break;
            case game_data_type::CONFIG:
            case game_data_type::CONTROL:
            case game_data_type::DISPLAY:
            case game_data_type::LOCATION:
            case game_data_type::SCRIPT:
            case game_data_type::TEXT:
            case game_data_type::TEAM_MEMBER:
            case game_data_type::TASK:
            case game_data_type::DIARY_RECORD:
            case game_data_type::NetObject:
            case game_data_type::SUBGROUP:
            case game_data_type::TARGET:
                ErrorHandling::report_error("Cannot serialize unsupported type: " + std::to_string((int)type));
                throw std::runtime_error("Cannot serialize unsupported type: " + std::to_string((int)type));
            default:
                break;
        }
            
        // Write a special marker for serialized types
        stream.write(reinterpret_cast<const char*>(&needs_special_handling), sizeof(bool));

        if (!stream.good()) {
            ErrorHandling::report_error("Failed to write type");
            throw std::runtime_error("Failed to write type");
        }
        
        if (needs_special_handling) {
            // Write the original type for reference
            stream.write(reinterpret_cast<const char*>(&type), sizeof(type));

            if (!stream.good()) {
                ErrorHandling::report_error("Failed to write type");
                throw std::runtime_error("Failed to write type");
            }
            
            // Serialize as string based on type
            std::string serialized;

            switch (type) {
                case game_data_type::CODE: {
                    // Store code as string without curly braces
                    std::string code_str = static_cast<std::string>(value);

                    if (code_str.length() >= 2 && code_str.front() == '{' && code_str.back() == '}') {
                        serialized = code_str.substr(1, code_str.length() - 2);
                    } else {
                        serialized = code_str;
                    }

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
                    serialized = static_cast<std::string>(sqf::group_id(value));
                    break;
                }
                case game_data_type::OBJECT: {
                    // Store vehicle var name or hash
                    serialized = static_cast<std::string>(sqf::vehicle_var_name(value));

                    if (serialized.empty()) {
                        if (value.data) {
                            std::string obj_str = static_cast<std::string>(value.data->to_string());

                            if (!obj_str.empty()) {
                                serialized = "OBJ_HASH_" + CryptoGenerator::fnv1a_64(obj_str);
                            } else {
                                serialized = "OBJ_HASH_NULL";
                            }
                        } else {
                            serialized = "OBJ_HASH_INVALID";
                        }
                    }
                    break;
                }
                default:
                    ErrorHandling::report_error("Cannot serialize game value type: " + std::to_string((int)type));
                    throw std::runtime_error("Cannot serialize game value type: " + std::to_string((int)type));
            }
            
            write_string(stream, serialized);
        } else {
            // Write type and flags
            stream.write(reinterpret_cast<const char*>(&type), sizeof(type));

            if (!stream.good()) {
                ErrorHandling::report_error("Failed to write type");
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
                    // Manually convert hashmap to array of [key,value] pairs
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
                ErrorHandling::report_error("Failed to write type");
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
                        if (static_cast<std::string>(sqf::group_id(grp)) == serialized) {
                            return grp;
                        }
                    }

                    return sqf::grp_null(); // Group not found
                }
                
                case game_data_type::OBJECT: {
                    // Try to find object by vehicle var name or hash
                    if (serialized.substr(0, 9) != "OBJ_HASH_") {
                        // Look for named object
                        return sqf::get_variable(sqf::mission_namespace(), serialized, sqf::obj_null());
                    } else {
                        // Extract the stored hash
                        std::string stored_hash = serialized.substr(9);
                        
                        // Get all entities
                        auto entities = sqf::entities("");
                        
                        // Find matching object by hash
                        for (const auto& entity : entities) {
                            std::string entity_str = static_cast<std::string>(entity.data->to_string());
                            std::string entity_hash = CryptoGenerator::fnv1a_64(entity_str);
                            
                            if (entity_hash == stored_hash) {
                                return entity;
                            }
                        }
                        
                        // Object not found - could have been deleted or is on a different machine
                        return game_value();
                    }
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

                    return sqf::call2(sqf::compile("createHashMapFromArray _this"), game_value(std::move(arr)));
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
            ErrorHandling::report_error("File " + file->get_filepath().string() + 
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
            ErrorHandling::report_error("Invalid filename: " + filename);
            return false;
        }

        if (!std::filesystem::exists(filepath)) {
            return false;
        }

        // File size check
        auto file_size = std::filesystem::file_size(filepath);

        if (file_size < 12) { // Minimum header size
            ErrorHandling::report_error("File too small to be valid: " + filename);
            return false;
        }
        
        if (!get_machine_is_server()) {
            if (file_size > MAX_TOTAL_KHDATA_SIZE) {
                ErrorHandling::report_error("File exceeds maximum size: " + filename);
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
                ErrorHandling::report_error("Failed to read file header: " + filename);
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
                ErrorHandling::report_error("Warning - extra data in file: " + filename);
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
            ErrorHandling::report_error("Invalid filename for backup load: " + filename);
            return false;
        }
        
        if (!std::filesystem::exists(backup_path)) {
            return false;
        }
        
        // Check backup file size
        auto file_size = std::filesystem::file_size(backup_path);
        
        if (file_size < 12) {
            ErrorHandling::report_error("Backup file size invalid: " + filename);
            return false;
        }

        if (!get_machine_is_server()) {
            if (file_size > MAX_TOTAL_KHDATA_SIZE) {
                ErrorHandling::report_error("Backup file size invalid: " + filename);
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
                ErrorHandling::report_error("Backup file has invalid header: " + filename);
                return false;
            }
            
            if (var_count > 10000) {  // Sanity check
                ErrorHandling::report_error("Backup file has too many variables: " + std::to_string(var_count));
                return false;
            }
            
            // Read variables
            std::unordered_map<std::string, game_value> vars;
            
            for (uint32_t i = 0; i < var_count; i++) {
                std::string var_name = read_string(stream);

                if (var_name.length() > 256) {  // Sanity check
                    ErrorHandling::report_error("Variable name too long in backup");
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
                ErrorHandling::report_error("Could not restore main file from backup for " + filename);
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
            ErrorHandling::report_error("Failed to load backup: " + std::string(e.what()));
            return false;
        } catch (...) {
            ErrorHandling::report_error("Unknown error loading backup: " + filename);
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
                    std::string group_id = static_cast<std::string>(sqf::group_id(value));
                    size += sizeof(uint32_t) + group_id.length();
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
            ErrorHandling::report_error("File size limit exceeded: " + file->get_filepath().string());
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
                    ErrorHandling::report_error("Failed to open temp file: " + temp_path.string());
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
                    ErrorHandling::report_error("Write failed for file: " + file->get_filepath().string());
                    return false;
                }
            }
            
            try {
                // Windows-safe atomic replace
                if (std::filesystem::exists(file->get_filepath())) {
                    std::filesystem::remove(file->get_filepath());
                }

                std::filesystem::rename(temp_path, file->get_filepath());
                temp_guard.should_delete = false;
                file->clear_dirty();
                return true;
            } catch (const std::filesystem::filesystem_error& e) {
                // Fallback: copy + delete
                try {
                    std::filesystem::copy_file(temp_path, file->get_filepath(), 
                        std::filesystem::copy_options::overwrite_existing);

                    std::filesystem::remove(temp_path);
                    temp_guard.should_delete = false;
                    file->clear_dirty();
                    return true;
                } catch (...) {
                    file->mark_save_failed();
                    ErrorHandling::report_error("Failed to save file: " + std::string(e.what()));
                    return false;
                }
            }
        } catch (...) {
            file->mark_save_failed();
            return false;
        }
    }

    bool delete_file(const std::string& filename) {        
        // Validate filename
        if (!validate_filename(filename)) {
            ErrorHandling::report_error("Invalid filename for deletion: " + filename);
            return false;
        }
        
        auto it = files.find(filename);

        if (it != files.end() && it->second->needs_save()) {
            ErrorHandling::report_error("Attempting to save dirty file before deletion: " + filename);

            if (!save_file(it->second.get())) {
                ErrorHandling::report_error("Warning - failed to save dirty file before deletion: " + filename);
                // Continue with deletion anyway - user explicitly requested it
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

static types::registered_sqf_function _execute_lua_sqf_command;
static types::registered_sqf_function _compile_lua_sqf_command;
static types::registered_sqf_function _crypto_hash_sqf_command;
static types::registered_sqf_function _generate_random_string_sqf_command;
static types::registered_sqf_function _generate_uid_sqf_command;
static types::registered_sqf_function _write_khdata_sqf_command;
static types::registered_sqf_function _read_khdata_sqf_command;
static types::registered_sqf_function _flush_khdata_sqf_command;
static types::registered_sqf_function _delete_khdata_file_sqf_command;
static types::registered_sqf_function _get_terrain_matrix_sqf_command;
static std::vector<std::vector<float>> g_terrain_matrix;
static float g_terrain_grid_width = 0.0f;
static int g_terrain_grid_size = 0;
static float g_world_size = 0.0f;
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
            ErrorHandling::report_error("Empty Lua code provided");
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
                ErrorHandling::report_error("Syntax error: " + std::string(err.what()));
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
            ErrorHandling::report_error("Compilation failed: " + std::string(e.what()));
            return CompileResult(false, "Compilation failed: " + std::string(e.what()));
        } catch (const std::exception& e) {
            ErrorHandling::report_error("Unexpected error: " + std::string(e.what()));
            return CompileResult(false, "Unexpected error: " + std::string(e.what()));
        }
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

        // Check for metatable with __toSQF method
        /*
        * Custom Lua to SQF Conversion via __toSQF metamethod
        * 
        * Users can define custom conversion for their Lua tables:
        * 
        * Example:
        *   local myCustomObject = {
        *       data = {x = 100, y = 200, z = 50},
        *       name = "MyObject"
        *   }
        *   
        *   setmetatable(myCustomObject, {
        *       __toSQF = function(self)
        *           -- Convert to SQF array format
        *           return {self.name, self.data.x, self.data.y, self.data.z}
        *       end
        *   })
        *   
        *   -- When passed to SQF, becomes: ["MyObject", 100, 200, 50]
        *   sqf.someCommand(myCustomObject)
        */
        sol::optional<sol::table> metatable = tbl[sol::metatable_key];
        if (metatable) {
            sol::optional<sol::function> to_sqf = (*metatable)["__toSQF"];

            if (to_sqf) {
                // Call the __toSQF method and convert its result
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
        (*g_lua_state)["MissionFrame"] = 0;
        return 0;
    }
    
    static int lua_frame_update() {
        int frame = (*g_lua_state)["MissionFrame"].get_or(0);
        (*g_lua_state)["MissionFrame"] = frame + 1;
        return 0;
    }
    
    static int lua_deinit() {
        (*g_lua_state)["MissionFrame"] = 0;
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
            ErrorHandling::report_error("Not enough arguments");
            return sol::make_object(lua, "Not enough arguments");
        }
        
        sol::object params = args[0];
        sol::object code_obj = args[1];
        sol::object count_obj = args.size() > 2 ? args[2] : sol::make_object(lua, 1);
        
        // Validate code parameter
        if (code_obj.get_type() != sol::type::string) {
            ErrorHandling::report_error("Code must be a string");
            return sol::make_object(lua, "Code must be a string");
        }
        
        std::string code = code_obj.as<std::string>();
        int count = count_obj.get_type() == sol::type::number ? count_obj.as<int>() : 1;
        
        if (count < 1) {
            ErrorHandling::report_error("Execution count must be at least 1");
            return sol::make_object(lua, "Execution count must be at least 1");
        }
        
        sol::protected_function compiled;
        
        // Check if it's a function name or code to compile
        if (code.find(' ') == std::string::npos && 
            code.find('\t') == std::string::npos && 
            code.find('\n') == std::string::npos) {
            
            // It's a function name
            sol::object func = lua[code];

            if (func.get_type() != sol::type::function) {
                ErrorHandling::report_error("Function '" + code + "' not found or is not a function");
                return sol::make_object(lua, "Function '" + code + "' not found or is not a function");
            }
            compiled = func;
        } else {
            // Compile the code
            std::string full_code = "return function(...) " + code + " end";
            sol::load_result load_res = lua.load(full_code);
            
            if (!load_res.valid()) {
                sol::error err = load_res;
                ErrorHandling::report_error("Failed to compile code: " + std::string(err.what()));
                return sol::make_object(lua, "Failed to compile code: " + std::string(err.what()));
            }
            
            sol::protected_function factory = load_res;
            auto factory_result = factory();

            if (!factory_result.valid()) {
                sol::error err = factory_result;
                ErrorHandling::report_error("Failed to create function: " + std::string(err.what()));
                return sol::make_object(lua, "Failed to create function: " + std::string(err.what()));
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
            ErrorHandling::report_error("High precision timer not available");
            return sol::make_object(lua, "High precision timer not available");
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
        
        // Store original globals for lua.* access
        sol::table lua_namespace = lua.create_table();

        lua_namespace[sol::metatable_key] = lua.create_table_with(
            "__index", lua.globals(),
            "__newindex", lua.globals()
        );
        
        // __index: Read from sqf commands first, then sqf variables, then error
        meta["__index"] = [&lua, lua_namespace](sol::table t, sol::object key) -> sol::object {
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
            game_value sqf_var = sqf::get_variable(sqf::mission_namespace(), key_str);

            if (!sqf_var.is_nil()) {
                return convert_game_value_to_lua(sqf_var);
            }
            
            // Not found - return nil
            return sol::nil;
        };
        
        // __newindex: Write to SQF variables by default
        meta["__newindex"] = [&lua](sol::table t, sol::object key, sol::object value) {
            if (key.get_type() != sol::type::string) return;
            std::string var_name = key.as<std::string>();
            
            // Don't allow overwriting "lua" keyword
            if (var_name == "lua") {
                ErrorHandling::report_error("Cannot overwrite 'lua' keyword in with_sqf context");
                return;
            }
            
            // Set as SQF variable
            sqf::set_variable(sqf::mission_namespace(), var_name, convert_lua_to_game_value(value));
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
            ErrorHandling::report_error(std::string(e.what()));
            return sol::make_object(lua, std::string(e.what()));
        } catch (...) {
            sol::state_view lua(params.lua_state());
            ErrorHandling::report_error("Unknown error in SQF_FunctionCall");
            return sol::make_object(lua, "Unknown error in SQF_FunctionCall");
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

        // Set up panic handler for unprotected errors
        lua_atpanic(g_lua_state->lua_state(), [](lua_State* L) -> int {
            std::string error_msg = lua_tostring(L, -1);
            ErrorHandling::report_error(error_msg);
            return 0;
        });

        // Panic guard
        g_lua_state->set_panic([](lua_State* L) -> int {
            const char* msg = lua_tostring(L, -1);
            ErrorHandling::report_error((msg ? msg : "unknown"));
            lua_settop(L, 0);  // Clear stack on panic
            return 0;
        });
        
        // Override the default Lua error function
        g_lua_state->set_function("error", [](sol::variadic_args va) {
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

            ErrorHandling::report_error("Lua", ss.str());
        });
        
        // Set up Sol's default error handler for protected calls
        g_lua_state->set_exception_handler([](lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) -> int {
            if (maybe_exception) {
                ErrorHandling::report_error("Lua exception", maybe_exception->what());
            } else {
                ErrorHandling::report_error("Lua", std::string(description));
            }

            return sol::stack::push(L, description);
        });

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

        (*g_lua_state)["GenerateUid"] = []() -> std::string {
            return RandomStringGenerator::generate(16, true, true, true);
        };

        // KHData functions
        (*g_lua_state)["WriteKHData"] = [](const std::string& filename, const std::string& var_name, sol::object value) -> sol::object {
            auto* file = KHDataManager::instance().get_or_create_file(filename);

            if (file) {
                game_value gv = convert_lua_to_game_value(value);
                file->write_variable(var_name, gv);
            }

            return sol::make_object(*g_lua_state, sol::nil);
        };

        (*g_lua_state)["ReadKHData"] = [](const std::string& filename, const std::string& var_name) -> sol::object {
            auto* file = KHDataManager::instance().get_or_create_file(filename);

            if (!file) {
                return sol::make_object(*g_lua_state, sol::nil);
            }

            if (var_name == filename) {
                auto names = file->get_variable_names();
                sol::table tbl = g_lua_state->create_table();

                for (size_t i = 0; i < names.size(); i++) {
                    tbl[i + 1] = names[i];
                }

                return sol::make_object(*g_lua_state, tbl);
            }
            
            return convert_game_value_to_lua(file->read_variable(var_name));
        };

        (*g_lua_state)["FlushKHData"] = []() -> sol::object {
            KHDataManager::instance().flush_all();
            return sol::make_object(*g_lua_state, sol::nil);
        };

        (*g_lua_state)["DeleteKHDataFile"] = [](const std::string& filename) -> sol::object {
            KHDataManager::instance().delete_file(filename);
            return sol::make_object(*g_lua_state, sol::nil);
        };

        (*g_lua_state)["TerrainMatrix"] = sol::readonly_property([]() -> sol::object {
            if (g_terrain_matrix.empty()) {
                return sol::make_object(*g_lua_state, sol::nil);
            }
            
            sol::table matrix = g_lua_state->create_table();
            
            for (size_t y = 0; y < g_terrain_matrix.size(); y++) {
                sol::table row = g_lua_state->create_table();

                for (size_t x = 0; x < g_terrain_matrix[y].size(); x++) {
                    row[x + 1] = g_terrain_matrix[y][x]; // Lua uses 1-based indexing
                }

                matrix[y + 1] = row;
            }
            
            return sol::make_object(*g_lua_state, matrix);
        });

        (*g_lua_state)["TerrainGridWidth"] = sol::readonly_property([]() -> float {
            return g_terrain_grid_width;
        });

        (*g_lua_state)["TerrainGridSize"] = sol::readonly_property([]() -> int {
            return g_terrain_grid_size;
        });

        (*g_lua_state)["WorldSize"] = sol::readonly_property([]() -> float {
            return g_world_size;
        });

        // Helper function to get terrain height at specific grid coordinates
        (*g_lua_state)["GetTerrainHeightAt"] = [](int grid_x, int grid_y) -> sol::object {
            // Convert from 1-based Lua indexing to 0-based C++ indexing
            int x = grid_x - 1;
            int y = grid_y - 1;
            
            if (x < 0 || y < 0 || 
                y >= static_cast<int>(g_terrain_matrix.size()) || 
                x >= static_cast<int>(g_terrain_matrix[0].size())) {
                return sol::make_object(*g_lua_state, sol::nil);
            }
            
            return sol::make_object(*g_lua_state, g_terrain_matrix[y][x]);
        };

        // Helper to get interpolated height at world position
        (*g_lua_state)["GetTerrainHeightAtPos"] = [](float world_x, float world_y) -> float {
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
        (*g_lua_state)["MissionFrame"] = 0;

        // Register all utility functions in Lua global namespace
        (*g_lua_state)["GetSystemTime"] = lua_functions::get_system_time;
        (*g_lua_state)["GetDateTime"] = lua_functions::get_date_time;
        (*g_lua_state)["GetTimestamp"] = lua_functions::get_timestamp;
        (*g_lua_state)["GetTimeMs"] = lua_functions::get_time_ms;
        (*g_lua_state)["ProfileCode"] = lua_functions::profile_code;
        (*g_lua_state)["GetDataType"] = lua_functions::get_data_type;
        (*g_lua_state)["with_sqf"] = lua_functions::with_sqf;
        (*g_lua_state)["SQF_FunctionCall"] = lua_functions::sqf_function_call;

        // This lets you get and set sqf variables using SQF_VAR.someVariable
        sol::table sqf_var = g_lua_state->create_table();

        sqf_var[sol::metatable_key] = g_lua_state->create_table_with(
            "__index", [](sol::this_state L, sol::object key) -> sol::object {
                if (key.get_type() != sol::type::string) return sol::nil;
                std::string var_name = key.as<std::string>();
                game_value result = sqf::get_variable(sqf::mission_namespace(), var_name);
                return convert_game_value_to_lua(result);
            },
            "__newindex", [](sol::this_state L, sol::object key, sol::object value) {
                if (key.get_type() != sol::type::string) return;
                std::string var_name = key.as<std::string>();
                sqf::set_variable(sqf::mission_namespace(), var_name, convert_lua_to_game_value(value));
            }
        );

        (*g_lua_state)["sqf_var"] = sqf_var;

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
                ErrorHandling::report_error("SQF commands only support 0-2 arguments (no argument, right argument, or left and right argument)");
                return sol::make_object(lua, "SQF commands only support 0-2 arguments (no argument, right argument, or left and right argument)");
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

        sqf_table["throw"] = [](sqf_string_const_ref text) -> sol::object {
            sqf::throw_exception(text);
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
        LuaStackGuard guard(*g_lua_state);
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
                    ErrorHandling::report_error("Function '" + code_str + "' not found");
                    return game_value();
                }
                
                g_call_cache[code_str] = {code_str, func, true};
            }
            
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
                    ErrorHandling::report_error(std::string(err.what()));
                    return game_value();
                }
                
                sol::protected_function factory = load_res;
                auto factory_result = factory();

                if (!factory_result.valid()) {
                    sol::error err = factory_result;
                    ErrorHandling::report_error("Failed to create function: " + std::string(err.what()));
                    return game_value();
                }
                
                compiled_code = factory_result;
                g_code_cache[code_hash] = compiled_code;
            }
            
            if (args.type_enum() == types::game_data_type::ARRAY) {
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
            ErrorHandling::report_error(std::string(err.what()));
            return game_value();
        }
        
        // Convert return value(s) to game_value
        if (result.return_count() == 0) {
            return game_value();  // nil
        } else {
            return convert_lua_to_game_value(result.get<sol::object>());
        }
    } catch (const sol::error& e) {
        ErrorHandling::report_error(std::string(e.what()));
        return game_value();
    } catch (const std::exception& e) {
        ErrorHandling::report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        ErrorHandling::report_error("Unknown error occurred");
        return game_value();
    }
}

// Native SQF command implementation for compileLua
static game_value compile_lua_sqf(game_value_parameter name, game_value_parameter code) {    
    try {
        std::string lua_code = static_cast<std::string>(code);
        std::string lua_name = static_cast<std::string>(name);
        
        // Validate the Lua name
        if (lua_name.empty()) {
            ErrorHandling::report_error("Function name cannot be empty");
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
            ErrorHandling::report_error(result.error_message);
            return game_value();
        }
        
    } catch (const sol::error& e) {
        ErrorHandling::report_error("Lua compilation - " + std::string(e.what()));
        return game_value();
    } catch (const std::exception& e) {
        ErrorHandling::report_error(std::string(e.what()));
        return game_value();
    } catch (...) {
        ErrorHandling::report_error("Unknown error during compilation");
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
            ErrorHandling::report_error("Unknown hash type: " + hash_type);
            return game_value();
        }
        
        return game_value(result);
    } catch (const std::exception& e) {
        ErrorHandling::report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value generate_random_string_sqf(game_value_parameter options, game_value_parameter length) {
    try {
        int len = static_cast<int>(static_cast<float>(length));
        
        if (len <= 0) {
            ErrorHandling::report_error("Length must be positive");
            return game_value();
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
        ErrorHandling::report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value generate_uid_sqf() {
    try {
        // Generate 16-character UID with all character types (numbers, letters, symbols)
        std::string uid = RandomStringGenerator::generate(16, true, true, true);
        return game_value(uid);
    } catch (const std::exception& e) {
        ErrorHandling::report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value write_khdata_sqf(game_value_parameter params, game_value_parameter value) {
    try {
        if (params.type_enum() != types::game_data_type::ARRAY || params.size() < 2) {
            ErrorHandling::report_error("Expected [filename, variable_name]");
            return game_value();
        }
        
        std::string filename = static_cast<std::string>(params[0]);
        std::string var_name = static_cast<std::string>(params[1]);

        if (filename.empty() || var_name.empty()) {
            ErrorHandling::report_error("Empty filename or variable name");
            return game_value();
        }
        
        auto* file = KHDataManager::instance().get_or_create_file(filename);
        if (!file) {
            ErrorHandling::report_error("Failed to access file");
            return game_value();
        }
        
        file->write_variable(var_name, value);
        
        return game_value();
    } catch (const std::exception& e) {
        ErrorHandling::report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value read_khdata_sqf(game_value_parameter filename, game_value_parameter var_name) {
    try {
        std::string file_str = static_cast<std::string>(filename);
        std::string var_str = static_cast<std::string>(var_name);
        auto* file = KHDataManager::instance().get_or_create_file(file_str);

        if (!file) {
            return game_value();
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
        
        return file->read_variable(var_str);
    } catch (...) {
        return game_value();
    }
}

static game_value flush_khdata_sqf() {
    try {
        int count = KHDataManager::instance().flush_all();
        return game_value();
    } catch (const std::exception& e) {
        ErrorHandling::report_error(std::string(e.what()));
        return game_value();
    }
}

static game_value delete_khdata_file_sqf(game_value_parameter filename) {
    try {
        std::string file_str = static_cast<std::string>(filename);
        
        if (KHDataManager::instance().delete_file(file_str)) {
            return game_value();
        } else {
            ErrorHandling::report_error("Failed to delete file");
            return game_value();
        }
    } catch (const std::exception& e) {
        ErrorHandling::report_error(std::string(e.what()));
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
        ErrorHandling::report_error("Failed to initialize terrain matrix: " + std::string(e.what()));
    } catch (...) {
        ErrorHandling::report_error("Unknown error initializing terrain matrix");
    }
}

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start() {
    initialize_lua_state();
    LuaStackGuard guard(*g_lua_state);
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
        types::game_data_type::NOTHING,     // Return type
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
        types::game_data_type::ARRAY,   // Right argument - options [numbers, letters, symbols]
        types::game_data_type::SCALAR     // Left argument - length
    );

    _generate_uid_sqf_command = intercept::client::host::register_sqf_command(
        "generateUid",
        "Generate a 16-character unique identifier",
        userFunctionWrapper<generate_uid_sqf>,
        types::game_data_type::STRING   // Return type - no arguments needed
    );

    KHDataManager::instance().initialize();

    // Register KHData commands
    _write_khdata_sqf_command = intercept::client::host::register_sqf_command(
        "writeKhData",
        "Write variable to KHData file",
        userFunctionWrapper<write_khdata_sqf>,
        types::game_data_type::NOTHING,     // Return type
        types::game_data_type::ARRAY,      // Left argument - [filename, variable_name]
        types::game_data_type::ANY         // Right argument - value (nil to delete)
    );

    _read_khdata_sqf_command = intercept::client::host::register_sqf_command(
        "readKhData",
        "Read variable from KHData file",
        userFunctionWrapper<read_khdata_sqf>,
        types::game_data_type::ANY,        // Return type
        types::game_data_type::STRING,     // Left argument - filename
        types::game_data_type::STRING      // Right argument - variable_name
    );

    _flush_khdata_sqf_command = intercept::client::host::register_sqf_command(
        "flushKhData",
        "Flush all dirty KHData files to disk",
        userFunctionWrapper<flush_khdata_sqf>,
        types::game_data_type::NOTHING      // Return type
    );

    _delete_khdata_file_sqf_command = intercept::client::host::register_sqf_command(
        "deleteKhDataFile",
        "Delete KHData file",
        userFunctionWrapper<delete_khdata_file_sqf>,
        types::game_data_type::NOTHING,     // Return type
        types::game_data_type::STRING      // Argument - filename
    );

    _get_terrain_matrix_sqf_command = intercept::client::host::register_sqf_command(
        "getTerrainMatrix",
        "Get the pre-calculated terrain height matrix",
        userFunctionWrapper<get_terrain_matrix_sqf>,
        types::game_data_type::ARRAY      // Return type
    );

    sqf::diag_log("KH Framework Lua - Pre-start Complete");
}

void intercept::pre_init() {
    LuaStackGuard guard(*g_lua_state);
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

    initialize_terrain_matrix();
    lua_functions::lua_init();
    KHDataManager::instance().flush_all();
    sqf::diag_log("KH Framework Lua - Pre-init Complete");
}

void intercept::post_init() {
    LuaStackGuard guard(*g_lua_state);
    sqf::diag_log("KH Framework Lua - Post-init Complete");
}

void intercept::on_frame() {
    LuaStackGuard guard(*g_lua_state);
    lua_functions::lua_frame_update();
}

void intercept::mission_ended() {
    LuaStackGuard guard(*g_lua_state);
    (*g_lua_state)["MissionActive"] = false;
    lua_functions::lua_deinit();
    KHDataManager::instance().flush_all();
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