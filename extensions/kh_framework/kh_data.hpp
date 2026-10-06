#pragma once

using namespace intercept;
using namespace intercept::types;

constexpr uint32_t KHDATA_MAGIC = 0x5444484B; // "KHDT"
constexpr uint32_t KHDATA_VERSION = 1;
constexpr size_t MAX_KHDATA_FILES = 1024;
constexpr size_t MAX_TOTAL_KHDATA_SIZE = 1024LL * 1024LL * 1024LL;
constexpr int MAX_KHDATA_SAVE_ATTEMPTS = 3;
// A .khdata file is read as untrusted bytes (a crash mid-write elsewhere, a disk error, another
// program): every length is held to the file's size, and the counts of all its containers together to the
// values it has bytes for, before anything is allocated; every read is checked; and no value lies
// KHDATA_MAX_DEPTH levels deep (the stored value is level 0, an array's elements one level below it, a hash
// map's keys and values two - its [key, value] pairs are arrays in the file). write_variable refuses such a
// value, and save_file leaves out one that became so after its write (the file holds the script's own array),
// so every value a save writes can be read back.
// KHDATA_MIN_VALUE_BYTES is the least a stored value takes: its flag and its type.
constexpr int KHDATA_MAX_DEPTH = 256;
constexpr uint64_t KHDATA_MIN_VALUE_BYTES = sizeof(bool) + sizeof(game_data_type);
static_assert(sizeof(bool) == 1, "the file's flags are one byte each");

class KHDataFile {
public:
    enum class DirtyState {
        Clean,
        Modified,
        SaveFailed,
        SizeExceeded
    };

    std::unordered_map<std::string, game_value> variables;
    std::string filename;
    std::filesystem::path filepath;
    DirtyState dirty_state = DirtyState::Clean;
    int failed_save_attempts = 0;

    // A write arms the next save with fresh attempts; a failed save is retried by the following flushes (needs_save),
    // MAX_KHDATA_SAVE_ATTEMPTS times in a row at most, until a write re-arms it. A save refused over the size limit
    // waits for a write (the size has to change).
    void mark_dirty() {
        dirty_state = DirtyState::Modified;
        failed_save_attempts = 0;
    }
    
    KHDataFile(const std::string& name) : filename(name), dirty_state(DirtyState::Clean) {}

    game_value read_variable(const std::string& var_name) const {
        auto it = variables.find(var_name);

        if (it != variables.end()) {
            return it->second;
        }

        return game_value();
    }

    // Whether no value in it lies as deep as read_game_value refuses (its depth rule, mirrored).
    static bool depth_ok(const game_value& value, int depth) {
        if (depth >= KHDATA_MAX_DEPTH) return false;   // Depth 0 is the value itself.
        const auto type = value.type_enum();

        if (type == game_data_type::ARRAY) {
            for (const auto& elem : value.to_array()) {
                if (!depth_ok(elem, depth + 1)) return false;
            }
        } else if (type == game_data_type::HASHMAP) {
            for (const auto& pair : value.to_hashmap()) {   // Each pair is an array in the file.
                if (!depth_ok(pair.key, depth + 2) || !depth_ok(pair.value, depth + 2)) return false;
            }
        }

        return true;
    }

    bool write_variable(const std::string& var_name, const game_value& value) {        
        // Refused whole, before anything changes.
        if (!depth_ok(value, 0)) {
            throw std::runtime_error("'" + var_name + "' holds a value " + std::to_string(KHDATA_MAX_DEPTH) +
                                     " levels deep or more and cannot be saved");
        }

        if (value.is_nil()) return delete_variable(var_name);
        variables[var_name] = value;
        mark_dirty();
        return true;
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
        return (dirty_state == DirtyState::Modified || dirty_state == DirtyState::SaveFailed) &&
               failed_save_attempts < MAX_KHDATA_SAVE_ATTEMPTS;
    }
    
    void mark_save_failed() { 
        dirty_state = DirtyState::SaveFailed; 
        failed_save_attempts++;
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
        clear_dirty();
    }
};

class KHDataManager {
private:
    std::unordered_map<std::string, std::unique_ptr<KHDataFile>> files;
    std::filesystem::path base_path;
    bool initialized = false;
    KHDataManager() = default;
    
    // A read or write fault; the load / save that called reports it once, with the file's name.
    [[noreturn]] static void fail(const std::string& message) {
        throw std::runtime_error(message);
    }

    static void write_string(std::ofstream& stream, const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.length());
        stream.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (!stream.good()) fail("failed to write a string length");

        if (len > 0) {
            stream.write(str.c_str(), len);
            if (!stream.good()) fail("failed to write a string");
        }
    }
    
    // Limit = the file's size - no length can exceed it.
    static std::string read_string(std::ifstream& stream, uint64_t limit) {
        uint32_t len;
        stream.read(reinterpret_cast<char*>(&len), sizeof(len));

        if (!stream.good()) fail("failed to read a string length");
        if (len > limit) throw std::runtime_error("a string longer than its file");

        std::string str(len, '\0');
        stream.read(&str[0], len);

        if (!stream.good() && len > 0) fail("failed to read a string");
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
                fail("cannot serialize type " + std::to_string(static_cast<int>(type)));
            default:
                break;
        }
            
        // Write a special marker for serialized types
        stream.write(reinterpret_cast<const char*>(&needs_special_handling), sizeof(bool));
        if (!stream.good()) fail("failed to write a value flag");
        stream.write(reinterpret_cast<const char*>(&type), sizeof(type));
        if (!stream.good()) fail("failed to write a type");
        
        if (needs_special_handling) {
            // Serialize as string based on type
            std::string serialized;

            switch (type) {
                case game_data_type::CODE:
                case game_data_type::TEXT:
                    serialized = static_cast<std::string>(value);
                    break;

                case game_data_type::NAMESPACE: {
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
                        serialized = "missionNamespace";
                    }

                    break;
                }

                case game_data_type::SIDE: {
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
                    // Store vehicle var name
                    serialized = static_cast<std::string>(sqf::vehicle_var_name(value));

                    if (serialized.empty()) {
                        serialized = "UNREGISTERED_OBJECT";
                    }
                    
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
                    // Store location class name
                    std::string location_name = static_cast<std::string>(sqf::class_name(value));                    
                    serialized = location_name.empty() ? "LOCATION_NULL" : location_name;
                    break;
                }

                case game_data_type::TEAM_MEMBER: {
                    game_value agent_obj = sqf::agent(value);
                    
                    // Storing agent, not team member object
                    serialized = static_cast<std::string>(sqf::vehicle_var_name(agent_obj));
                    
                    if (serialized.empty()) {
                        serialized = "UNREGISTERED_OBJECT";
                    }

                    break;
                }

                case game_data_type::DISPLAY: {
                    serialized = std::to_string(static_cast<int>(sqf::ctrl_idd(value)));
                    break;
                }

                default:
                    fail("cannot serialize type " + std::to_string(static_cast<int>(type)));
            }
            
            write_string(stream, serialized);
        } else {
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

            if (!stream.good()) fail("failed to write a value");
        }
    }
    
    // Limit = the file's size (no length can exceed it); elems = the container elements the file
    // still has bytes for, shared by all of its containers (each count is taken from it before the engine
    // reserves); depth = the containers above.
    static game_value read_game_value(std::ifstream& stream, uint64_t limit, uint64_t& elems, int depth = 0) {
        if (depth >= KHDATA_MAX_DEPTH) throw std::runtime_error("a value nested too deep");
        // Read as a byte: a stored byte other than 0 / 1 is no bool. A failed read leaves it 0 and the stream
        // failed, which the type's read below reports.
        uint8_t serialized_flag = 0;
        stream.read(reinterpret_cast<char*>(&serialized_flag), sizeof(serialized_flag));
        const bool is_serialized = serialized_flag != 0;
        
        if (is_serialized) {
            // Read the original serialized string-type
            game_data_type original_type;
            stream.read(reinterpret_cast<char*>(&original_type), sizeof(original_type));
            std::string serialized = read_string(stream, limit);   // A failed read above fails its length's.
            
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
                    if (serialized == "independent") return sqf::independent();
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

                    return sqf::grp_null();
                }
                
                case game_data_type::OBJECT: {
                    return sqf::get_variable(sqf::mission_namespace(), serialized, sqf::obj_null());
                }

                case game_data_type::TEXT:
                    return sqf::parse_text(serialized);

                case game_data_type::CONFIG: {
                    if (serialized == "configFile" || serialized.empty()) {
                        return sqf::config_file();
                    }
                    
                    // Compile and call the config path string to get actual config
                    code compiled = sqf::compile("setReturnValue (call {" + serialized + "});");
                    return raw_call_sqf_native(compiled);
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
                    
                    // We will use class name, probably for the best
                    for (const auto& loc : all_locations) {                        
                        if (static_cast<std::string>(sqf::class_name(loc)) == serialized) {
                            return loc;
                        }
                    }
                    
                    return sqf::location_null();
                }

                case game_data_type::TEAM_MEMBER: {
                    game_value agent_obj = sqf::get_variable(sqf::mission_namespace(), serialized, sqf::obj_null());
                    
                    if (agent_obj.is_null()) {
                        return sqf::team_member_null();
                    }
                    
                    return sqf::agent(agent_obj);
                }

                case game_data_type::DISPLAY: {                    
                    int idd;
                    
                    try {
                        idd = std::stoi(serialized);
                    } catch (...) {
                        return sqf::display_null();
                    }
                    
                    // Try to find the display by idd
                    return sqf::find_display(idd);
                }

                default:
                    // For unsupported serialized types, shouoldn't happen though
                    return game_value();
            }
        } else {
            // Read type normally
            game_data_type type;
            stream.read(reinterpret_cast<char*>(&type), sizeof(type));
            if (!stream.good()) throw std::runtime_error("a truncated value");
            
            switch (type) {
                case game_data_type::NOTHING:
                case game_data_type::ANY:
                    return game_value();

                case game_data_type::SCALAR: {
                    float val;
                    stream.read(reinterpret_cast<char*>(&val), sizeof(val));
                    if (!stream.good()) throw std::runtime_error("a truncated value");
                    return game_value(val);
                }

                case game_data_type::BOOL: {
                    uint8_t val = 0;   // A byte, as the flag above.
                    stream.read(reinterpret_cast<char*>(&val), sizeof(val));
                    if (!stream.good()) throw std::runtime_error("a truncated value");
                    return game_value(val != 0);
                }

                case game_data_type::STRING:
                    return game_value(read_string(stream, limit));

                case game_data_type::ARRAY:
                    return game_value(read_elements(stream, limit, elems, depth));

                case game_data_type::HASHMAP:   // Stored as [key, value] arrays.
                    return raw_call_sqf_args_native(g_compiled_sqf_create_hash_map_from_array,
                                                    game_value(read_elements(stream, limit, elems, depth)));

                default:
                    return game_value();
            }
        }
    }

    // A container's count and elements (read_game_value's ARRAY and HASHMAP).
    static auto_array<game_value> read_elements(std::ifstream& stream, uint64_t limit, uint64_t& elems, int depth) {
        uint32_t size;
        stream.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!stream.good()) throw std::runtime_error("a truncated value");
        // No more values than the file has bytes for, counting every container's, before the engine allocates.
        if (size > elems) throw std::runtime_error("a count past the end of the file");
        elems -= size;
        auto_array<game_value> arr;
        arr.reserve(size);

        for (uint32_t i = 0; i < size; i++) {
            arr.push_back(read_game_value(stream, limit, elems, depth + 1));
        }

        return arr;
    }

    // The open file's size in bytes, the reads' limit (0 when it cannot be told - the stream has
    // then failed, so the first read after it throws, and the load with it).
    static uint64_t stream_bytes(std::ifstream& stream) {
        const std::streampos cur = stream.tellg();
        stream.seekg(0, std::ios::end);
        const std::streampos end = stream.tellg();
        stream.seekg(cur);
        if (cur < 0 || end < 0 || !stream.good()) return 0;
        return static_cast<uint64_t>(static_cast<std::streamoff>(end));
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
        if (filename.find_first_of("*?<>|\"") != std::string::npos) {
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
        
        std::string upper = kh_upper_copy(base_name);

        // Check Windows reserved device names
        if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL") {
            return false;
        }

        // PATH_CONFINE: the stored name as a plain file name too (search_mod_folders.hpp) - Windows also takes a
        // device name with spaces before the extension ("nul .x", "com1 .x") as the device.
        if (!ModFolderSearcher::is_plain_relative_path(filename + ".khdata")) {
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

    size_t total_data_size = 0;
public:
    static KHDataManager& instance() {
        static KHDataManager inst;
        return inst;
    }

    bool initialize() {
        if (initialized) return true;
        
        const std::filesystem::path docs = ModFolderSearcher::kh_framework_documents_dir();
        if (docs.empty()) return false;
        base_path = docs / "kh_data";
        
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

                    try {
                        load_file(filename);
                    } catch (const std::exception& e) {   // A name no command could address: reported, skipped.
                        report_error("KHData: " + std::string(e.what()) + " - not loaded");
                    }
                }
            }

            update_total_size(); // Calculate initial total size
        } catch (...) {
            // Directory iteration failed, but initialization can continue
        }

        try {
            for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
                auto ext = entry.path().extension();
                
                // A .backup stays - load_file keeps it the last copy that loaded, and
                // load_file_from_backup restores from it; only a save's leftover .tmp goes.
                if (ext == ".tmp") {
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
            const size_t value_size = calculate_value_size(value);
            if (value_size == KHDATA_SIZE_DEEP) continue;   // Not written (save_file).
            size += sizeof(uint32_t) + name.length(); // Variable name
            size += value_size;                       // Variable value
        }
        
        size = static_cast<size_t>(size * 1.1);        
        return size;
    }
    
    void update_total_size() {
        total_data_size = 0;

        for (const auto& [name, file] : files) {
            total_data_size += estimate_file_size(file.get());
        }
    }

    // Throws on an invalid name (the calling command reports it once, under its own name); null when a client's
    // file or size limit refuses a new file.
    KHDataFile* get_or_create_file(const std::string& filename) {
        if (!validate_filename(filename)) throw std::runtime_error("invalid file name: " + filename);
        
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

        if (!validate_filename(filename)) throw std::runtime_error("invalid file name: " + filename);

        if (!std::filesystem::exists(filepath)) {
            return false;
        }

        auto file_size = std::filesystem::file_size(filepath);

        if (file_size < 12) {   // The header alone is 12 bytes.
            // A main cut short (a crash before its data reached the disk) is the shape the backup is kept for.
            if (std::filesystem::exists(backup_path)) {
                sqf::diag_log("KHData: " + filename + " is too small to be valid; loading its backup");
                return load_file_from_backup(filename);
            }

            report_error("KHData: " + filename + " is too small to be valid");
            return false;
        }

        if (!get_machine_is_server()) {
            if (file_size > MAX_TOTAL_KHDATA_SIZE) {
                report_error("KHData: " + filename + " exceeds the maximum size");
                return false;
            }
        }

        bool from_backup = false;   // The stream is the backup's: the main (unopenable) is never set aside.
        bool io_error = false;      // The read failed on the stream itself, not on the file's content (below).

        try {
            // Try loading main file first
            std::ifstream stream(filepath, std::ios::binary);
            // Whether the values came from the main, every byte of it accounted for - only such a main refreshes
            // the backup below.
            bool exact = true;

            if (!stream) {
                // Try backup if main fails
                if (std::filesystem::exists(backup_path)) {
                    stream.open(backup_path, std::ios::binary);
                    exact = false;   // The backup's values - the main is unread.
                    from_backup = true;

                    if (stream) {
                        sqf::diag_log("KHData: " + filename + " could not be opened; loading its backup");
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
                report_error(std::string("KHData: failed to read the header of ") +
                             (from_backup ? "the backup of " : "") + filename);
                return false;
            }
            
            if (magic != KHDATA_MAGIC || version != KHDATA_VERSION) {
                // No magic - the main's first bytes are lost (zeroed by a crash), so the backup when there is one;
                // otherwise, as for a version this build does not know, the file is set aside: this name is created
                // afresh in memory and its next save would overwrite what the file still holds. A header that
                // failed to read (above) is left alone - it may be a passing read error on a good file.
                stream.close();   // A restore copies the backup over this file; a set-aside renames it.
                const std::string reason = magic != KHDATA_MAGIC ? std::string("bad header")
                                           : "unsupported version " + std::to_string(version);

                if (from_backup) {
                    report_error("KHData: " + filename + " could not be opened and its backup is unreadable (" +
                                 reason + ")");
                    return false;
                }

                if (magic != KHDATA_MAGIC && std::filesystem::exists(backup_path)) {
                    return load_file_from_backup(filename) || set_aside_unreadable(filename, filepath, reason);
                }

                return set_aside_unreadable(filename, filepath, reason);
            }
            
            // Read variables
            std::unordered_map<std::string, game_value> vars;
            const uint64_t limit = stream_bytes(stream);
            uint64_t elems = limit / KHDATA_MIN_VALUE_BYTES;
            
            try {
                for (uint32_t i = 0; i < var_count; i++) {
                    std::string var_name = read_string(stream, limit);
                    game_value value = read_game_value(stream, limit, elems);
                    vars[var_name] = value;
                }
            } catch (...) {
                // A passing read error, not damage, when the stream itself failed (badbit), or when the read stopped
                // short of the file's end (MSVC's filebuf reports a failed ReadFile as a short read: eof + fail, the
                // same bits a truncation sets - but a truncated or miscounted file is read to its real end).
                if (stream.bad()) {
                    io_error = true;
                } else if (stream.fail()) {
                    stream.clear();
                    const std::streampos pos = stream.tellg();
                    io_error = pos < 0 || static_cast<uint64_t>(static_cast<std::streamoff>(pos)) < file_size;
                }

                throw;
            }
            
            if (!stream.eof() && stream.peek() != EOF) {
                sqf::diag_log("KHData: extra data at the end of " + filename + " (loaded what could be read)");
                exact = false;   // Bytes left over (a count damaged lower) - not exact.
            }

            // Create file object
            auto file = std::make_unique<KHDataFile>(filename);
            file->set_filepath(filepath);
            file->set_variables(std::move(vars));
            
            // On successful load and validation, create/update backup if needed
            try {
                bool should_backup = exact;   // From an exact main only.
                
                if (should_backup && std::filesystem::exists(backup_path)) {
                    auto main_time = std::filesystem::last_write_time(filepath);
                    auto backup_time = std::filesystem::last_write_time(backup_path);
                    // Any difference - the copy keeps the main's time (CopyFile), so equal
                    // means already copied; a backup stamped later than its main (a clock change) is refreshed
                    // too, where ">" left it stale for good.
                    should_backup = (main_time != backup_time);
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
        } catch (const std::bad_alloc&) {   // Not the file's fault: left where it is, like a passing read error.
            report_error("KHData: " + filename + " could not be read (out of memory)");
            return false;
        } catch (const std::exception& e) {
            return load_failed(filename, filepath, backup_path, from_backup, io_error, e.what());
        } catch (...) {
            return load_failed(filename, filepath, backup_path, from_backup, io_error, "unknown error");
        }
    }

    // load_file's read failed (reason): the backup, when there is one and the main was the stream read, else the
    // main set aside - unless the stream itself failed (io_error: a passing read error, not damage), when the file
    // is left where it is. Always false.
    bool load_failed(const std::string& filename, const std::filesystem::path& filepath,
                     const std::filesystem::path& backup_path, bool from_backup, bool io_error,
                     const std::string& reason) {
        if (from_backup) {
            report_error("KHData: " + filename + " could not be opened and its backup could not be read (" + reason +
                         ")");
            return false;
        }

        if (std::filesystem::exists(backup_path)) {
            sqf::diag_log("KHData: " + filename + " could not be read (" + reason + "); loading its backup");
            if (load_file_from_backup(filename)) return true;
        }

        if (io_error) {
            report_error("KHData: " + filename + " could not be read (" + reason + "); left as it is");
            return false;
        }

        return set_aside_unreadable(filename, filepath, reason);
    }

    // No backup (or none that loads): the damaged main is set aside as <file>.khdata.bad (as a restore sets a main
    // aside), so the next save under this name does not overwrite what it still holds. Always false (the file is
    // not loaded).
    bool set_aside_unreadable(const std::string& filename, const std::filesystem::path& filepath,
                              const std::string& reason) {
        auto bad_path = filepath;
        bad_path += ".bad";
        std::error_code ec;
        std::filesystem::rename(filepath, bad_path, ec);
        report_error("KHData: " + filename + " could not be read (" + reason + ")" +
                     (ec ? " and could not be set aside" : "; set aside as " + bad_path.filename().string()));
        return false;
    }

    bool load_file_from_backup(const std::string& filename) {
        auto backup_path = base_path / (filename + ".khdata.backup");
        if (!validate_filename(filename)) throw std::runtime_error("invalid file name: " + filename);
        
        if (!std::filesystem::exists(backup_path)) {
            return false;
        }
        
        // Check backup file size
        auto file_size = std::filesystem::file_size(backup_path);
        
        if (file_size < 12) {
            report_error("KHData: the backup of " + filename + " is too small to be valid");
            return false;
        }

        if (!get_machine_is_server()) {
            if (file_size > MAX_TOTAL_KHDATA_SIZE) {
                report_error("KHData: the backup of " + filename + " exceeds the maximum size");
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
                report_error("KHData: the backup of " + filename + " has an invalid header");
                return false;
            }
            
            // Read variables
            std::unordered_map<std::string, game_value> vars;
            const uint64_t limit = stream_bytes(stream);
            uint64_t elems = limit / KHDATA_MIN_VALUE_BYTES;
            
            for (uint32_t i = 0; i < var_count; i++) {
                std::string var_name = read_string(stream, limit);

                if (var_name.length() > 256) {
                    report_error("KHData: a variable name in the backup of " + filename + " is too long");
                    return false;
                }
                
                game_value value = read_game_value(stream, limit, elems);
                vars[var_name] = value;
            }
            
            // Create file object
            auto file = std::make_unique<KHDataFile>(filename);
            file->set_filepath(base_path / (filename + ".khdata"));
            file->set_variables(std::move(vars));
            
            // Try to restore the main file from backup
            auto main_path = base_path / (filename + ".khdata");
            // The main it replaces is kept first, as <file>.khdata.bad - the load that failed may
            // have met a passing read error on a good file newer than the backup. One copy, the latest; no
            // load reads it (initialize takes ".khdata" only), delete_file removes it. A main that cannot be kept
            // is not replaced, and the backup stays: the values are the backup's, in memory only, until a save.
            bool kept = true, restored = false;
            {
                auto bad_path = main_path;
                bad_path += ".bad";
                std::error_code bad_ec;
                const bool main_there = std::filesystem::exists(main_path, bad_ec);
                if (bad_ec) {
                    kept = false;
                } else if (main_there) {
                    std::filesystem::copy_file(main_path, bad_path, std::filesystem::copy_options::overwrite_existing,
                                               bad_ec);
                    kept = !bad_ec;
                }
            }

            if (kept) {
                try {
                    std::filesystem::copy_file(backup_path, main_path,
                        std::filesystem::copy_options::overwrite_existing);
                    restored = true;
                    sqf::diag_log("KHData: restored " + filename + " from its backup");
                } catch (...) {
                    report_error("KHData: could not restore " + filename + " from its backup");
                }
            } else {
                report_error("KHData: loaded " + filename + " from its backup; its main file could not be read or "
                             "set aside and was left as it is");
            }

            files[filename] = std::move(file);
            update_total_size();

            if (restored) {   // Else the backup is the only copy on disk of what loaded.
                try {
                    std::filesystem::remove(backup_path);
                } catch (...) {
                    // Non-critical if we can't delete the backup
                }
            }

            return true;
        } catch (const std::exception& e) {
            report_error("KHData: the backup of " + filename + " could not be read (" + e.what() + ")");
            return false;
        } catch (...) {
            report_error("KHData: the backup of " + filename + " could not be read (unknown error)");
            return false;
        }
    }

    // What calculate_value_size answers for a value that reaches KHDATA_MAX_DEPTH - one that
    // save_file does not write. It is passed straight up, so the walk stops at the first such path (an array
    // that holds itself twice would otherwise be walked 2^256 times; past the depth, the stack's).
    static constexpr size_t KHDATA_SIZE_DEEP = ~static_cast<size_t>(0);

    static size_t calculate_value_size(const game_value& value, int depth = 0) {       
        if (depth >= KHDATA_MAX_DEPTH) return KHDATA_SIZE_DEEP;
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
                    const size_t elem_size = calculate_value_size(elem, depth + 1);
                    if (elem_size == KHDATA_SIZE_DEEP) return KHDATA_SIZE_DEEP;
                    size += elem_size;
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
                    const size_t key_size = calculate_value_size(pair.key, depth + 1);
                    if (key_size == KHDATA_SIZE_DEEP) return KHDATA_SIZE_DEEP;
                    const size_t pair_value_size = calculate_value_size(pair.value, depth + 1);
                    if (pair_value_size == KHDATA_SIZE_DEEP) return KHDATA_SIZE_DEEP;
                    size += key_size + pair_value_size;
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
                
            case game_data_type::NAMESPACE: {
                size += sizeof(game_data_type);
                size += sizeof(uint32_t) + 26;  // "missionProfileNamespace" (longest)
                break;
            }
            
            case game_data_type::SIDE: {
                size += sizeof(game_data_type);
                size += sizeof(uint32_t) + 16;  // "sideAmbientLife" (longest)
                break;
            }
            
            case game_data_type::GROUP: {
                size += sizeof(game_data_type);
                std::string group_id_str = static_cast<std::string>(sqf::group_id(static_cast<group>(value)));
                size += sizeof(uint32_t) + group_id_str.length();
                break;
            }
            
            case game_data_type::OBJECT: {
                size += sizeof(game_data_type);
                std::string var_name = static_cast<std::string>(sqf::vehicle_var_name(value));
                
                if (var_name.empty()) {
                    size += sizeof(uint32_t) + 19;  // Potentially "UNREGISTERED_OBJECT"
                } else {
                    size += sizeof(uint32_t) + var_name.length();
                }
                break;
            }
            
            case game_data_type::TEXT: {
                // Text serialized as string
                std::string text_str = static_cast<std::string>(value);
                size += sizeof(game_data_type);
                size += sizeof(uint32_t) + text_str.length();
                break;
            }
            
            case game_data_type::CONFIG: {
                // Config serialized as path string
                auto hierarchy = sqf::config_hierarchy(value);
                size += sizeof(game_data_type);
                
                if (hierarchy.size() <= 1) {
                    size += sizeof(uint32_t) + 10;  // "configFile"
                } else {
                    // Estimate: "configFile" + (N-1) * average_entry_length
                    // Average config entry is ~20 chars + " >> " separator
                    size += sizeof(uint32_t) + 10 + ((hierarchy.size() - 1) * 24);
                }
                break;
            }
            
            case game_data_type::LOCATION: {
                size += sizeof(game_data_type);
                std::string location_name = static_cast<std::string>(sqf::class_name(value));
                
                if (location_name.empty()) {
                    size += sizeof(uint32_t) + 13;  // Potentially "LOCATION_NULL"
                } else {
                    size += sizeof(uint32_t) + location_name.length();
                }
                break;
            }
            
            case game_data_type::TEAM_MEMBER: {
                size += sizeof(game_data_type);
                game_value agent_obj = sqf::agent(value);
                std::string var_name = static_cast<std::string>(sqf::vehicle_var_name(agent_obj));
                
                if (var_name.empty()) {
                    size += sizeof(uint32_t) + 19;  // Potentially "UNREGISTERED_OBJECT"
                } else {
                    size += sizeof(uint32_t) + var_name.length();
                }

                break;
            }
            
            case game_data_type::DISPLAY: {
                size += sizeof(game_data_type);

                // IDD stored as string representation of integer (max ~10 digits for safety)
                size += sizeof(uint32_t) + 10;
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
        update_total_size();
        
        if ((total_data_size >= MAX_TOTAL_KHDATA_SIZE) && (!get_machine_is_server())) {
            file->mark_size_exceeded();
            report_error("KHData: the size limit is exceeded; " + file->get_filepath().string() + " not saved");
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
                    report_error("KHData: failed to open " + temp_path.string());
                    return false;
                }
                
                // Write header
                uint32_t magic = KHDATA_MAGIC;
                uint32_t version = KHDATA_VERSION;
                // Only values the reader accepts. write_variable refused deeper ones, but the file
                // holds the script's own array, which the script can nest further after the write; written, it
                // would fail the whole file's next load. Left out (it stays in memory) and reported.
                std::vector<const std::pair<const std::string, game_value>*> saved_vars;
                std::string skipped;
                saved_vars.reserve(file->get_variables().size());

                for (const auto& var : file->get_variables()) {
                    if (KHDataFile::depth_ok(var.second, 0)) {
                        saved_vars.push_back(&var);
                    } else {
                        skipped += (skipped.empty() ? "'" : ", '") + var.first + "'";
                    }
                }

                if (!skipped.empty()) {
                    report_error("KHData: not saved - nested " + std::to_string(KHDATA_MAX_DEPTH) +
                                 " levels deep or more since written: " + skipped + " in " +
                                 file->get_filepath().string());
                }

                uint32_t var_count = static_cast<uint32_t>(saved_vars.size());
                stream.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
                stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
                stream.write(reinterpret_cast<const char*>(&var_count), sizeof(var_count));
                
                // Write variables
                for (const auto* var : saved_vars) {
                    write_string(stream, var->first);
                    write_game_value(stream, var->second);
                }

                if (!stream.good()) {
                    file->mark_save_failed();
                    report_error("KHData: failed to write " + file->get_filepath().string());
                    return false;
                }
            }
            
            std::error_code ec;
            
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
                report_error("KHData: failed to save " + file->get_filepath().string() + ": " + ec.message());
                return false;
            }
            
            std::filesystem::remove(temp_path);
            temp_guard.should_delete = false;
            file->clear_dirty();
            return true;
        } catch (const std::exception& e) {
            file->mark_save_failed();
            report_error("KHData: failed to save " + file->get_filepath().string() + ": " + e.what());
            return false;
        } catch (...) {
            file->mark_save_failed();
            report_error("KHData: failed to save " + file->get_filepath().string() + ": unknown error");
            return false;
        }
    }

    // Throws on an invalid name (the calling command reports it once).
    bool delete_file(const std::string& filename) {        
        if (!validate_filename(filename)) throw std::runtime_error("invalid file name: " + filename);
        auto it = files.find(filename);

        if (it != files.end() && it->second->dirty_state == KHDataFile::DirtyState::Modified) {
            // Saved first so a failed disk delete leaves the file whole; the deletion goes on either way (a save
            // that already failed is not tried again just to delete the file).
            save_file(it->second.get());
        }

        // Remove from memory
        files.erase(filename);
        update_total_size();
        
        // Delete from disk
        auto filepath = base_path / (filename + ".khdata");
        // Its backup goes with it - a later file of the name must not restore deleted data.
        auto backup_path = filepath;
        backup_path += ".backup";

        try {
            std::error_code backup_ec;
            std::filesystem::remove(backup_path, backup_ec);
            auto bad_path = filepath;   // And a main a restore set aside (load_file_from_backup).
            bad_path += ".bad";
            std::filesystem::remove(bad_path, backup_ec);
            return std::filesystem::remove(filepath);
        } catch (...) {
            return false;
        }
    }

    int flush_all() {
        // Save to disk
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