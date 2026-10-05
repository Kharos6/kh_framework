#pragma once

using namespace intercept;
using namespace intercept::types;

constexpr uint32_t KHDATA_MAGIC = 0x5444484B; // "KHDT"
constexpr uint32_t KHDATA_VERSION = 1;
constexpr size_t MAX_KHDATA_FILES = 1024;
constexpr size_t MAX_TOTAL_KHDATA_SIZE = 1024LL * 1024LL * 1024LL;
constexpr int MAX_KHDATA_SAVE_ATTEMPTS = 3;
// KH_KHDATA_BOUND: a .khdata file is read as untrusted bytes (a crash mid-write elsewhere, a disk error, another
// program): every length is held to the file's size, and the counts of all its containers together to the
// values it has bytes for, before anything is allocated; every read is checked; and no value lies
// KHDATA_MAX_DEPTH levels deep (the stored value is level 0, an array's elements one level below it, a hash
// map's keys and values two - its [key, value] pairs are arrays in the file). write_variable refuses such a
// value, and save_file leaves out one that became so after its write (the file holds the script's own array),
// so every value a save writes can be read back.
// KHDATA_MIN_VALUE_BYTES is the least a stored value takes: its flag and its type.
constexpr int KHDATA_MAX_DEPTH = 256;
constexpr uint64_t KHDATA_MIN_VALUE_BYTES = sizeof(bool) + sizeof(game_data_type);
static_assert(sizeof(bool) == 1, "KH_KHDATA_BOUND: the file's flags are one byte each");

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
    std::chrono::steady_clock::time_point last_modified;
    std::chrono::steady_clock::time_point last_save_attempt;

    void mark_dirty() {
        if (dirty_state == DirtyState::Clean) {
            dirty_state = DirtyState::Modified;
            last_modified = std::chrono::steady_clock::now();
        }
    }
    
    KHDataFile(const std::string& name) : filename(name), dirty_state(DirtyState::Clean) {}

    game_value read_variable(const std::string& var_name) const {
        auto it = variables.find(var_name);

        if (it != variables.end()) {
            return it->second;
        }

        return game_value();
    }

    // KH_KHDATA_BOUND: whether no value in it lies as deep as read_game_value refuses (its depth rule, mirrored).
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
        // KH_KHDATA_BOUND: refused whole, before anything changes.
        if (!depth_ok(value, 0)) {
            throw std::runtime_error("KHData: '" + var_name + "' holds a value " + std::to_string(KHDATA_MAX_DEPTH) +
                                     " levels deep or more and cannot be saved");
        }

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
        return dirty_state == DirtyState::Modified && failed_save_attempts < MAX_KHDATA_SAVE_ATTEMPTS; 
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
        clear_dirty();
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
    
    // KH_KHDATA_BOUND: limit = the file's size - no length can exceed it.
    static std::string read_string(std::ifstream& stream, uint64_t limit) {
        uint32_t len;
        stream.read(reinterpret_cast<char*>(&len), sizeof(len));

        if (!stream.good()) {
            report_error("Failed to read string length");
            throw std::runtime_error("Failed to read string length");
        }

        if (len > limit) throw std::runtime_error("KHData: a string longer than its file");

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
                    // Store code as string
                    std::string code_str = static_cast<std::string>(value);
                    serialized = code_str;
                    break;
                }

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
    
    // KH_KHDATA_BOUND: limit = the file's size (no length can exceed it); elems = the container elements the file
    // still has bytes for, shared by all of its containers (each count is taken from it before the engine
    // reserves); depth = the containers above.
    static game_value read_game_value(std::ifstream& stream, uint64_t limit, uint64_t& elems, int depth = 0) {
        if (depth >= KHDATA_MAX_DEPTH) throw std::runtime_error("KHData: nested too deep");
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

                case game_data_type::TEXT: {
                    // Use parseText to reconstruct structured text from string
                    // If the string is empty, return a default TEXT (still will be text, not just an empty string)
                    if (serialized.empty()) {
                        return sqf::parse_text("");
                    }
                    
                    return sqf::parse_text(serialized);
                }

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
            if (!stream.good()) throw std::runtime_error("KHData: truncated value");
            
            switch (type) {
                case game_data_type::NOTHING:
                case game_data_type::ANY:
                    return game_value();

                case game_data_type::SCALAR: {
                    float val;
                    stream.read(reinterpret_cast<char*>(&val), sizeof(val));
                    if (!stream.good()) throw std::runtime_error("KHData: truncated value");
                    return game_value(val);
                }

                case game_data_type::BOOL: {
                    uint8_t val = 0;   // KH_KHDATA_BOUND: a byte, as the flag above.
                    stream.read(reinterpret_cast<char*>(&val), sizeof(val));
                    if (!stream.good()) throw std::runtime_error("KHData: truncated value");
                    return game_value(val != 0);
                }

                case game_data_type::STRING:
                    return game_value(read_string(stream, limit));

                case game_data_type::ARRAY: {
                    uint32_t size;
                    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
                    if (!stream.good()) throw std::runtime_error("KHData: truncated value");
                    // KH_KHDATA_BOUND: no more values than the file has bytes for, counting every container's, before
                    // the engine allocates.
                    if (size > elems) throw std::runtime_error("KHData: count past file");
                    elems -= size;
                    auto_array<game_value> arr;
                    arr.reserve(size);

                    for (uint32_t i = 0; i < size; i++) {
                        arr.push_back(read_game_value(stream, limit, elems, depth + 1));
                    }
                    
                    return game_value(std::move(arr));
                }

                case game_data_type::HASHMAP: {
                    //Reconstruct from array
                    uint32_t size;
                    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
                    if (!stream.good()) throw std::runtime_error("KHData: truncated value");
                    // KH_KHDATA_BOUND: no more values than the file has bytes for, counting every container's, before
                    // the engine allocates.
                    if (size > elems) throw std::runtime_error("KHData: count past file");
                    elems -= size;
                    auto_array<game_value> arr;
                    arr.reserve(size);

                    for (uint32_t i = 0; i < size; i++) {
                        arr.push_back(read_game_value(stream, limit, elems, depth + 1));
                    }

                    return raw_call_sqf_args_native(g_compiled_sqf_create_hash_map_from_array, game_value(std::move(arr)));
                }

                default:
                    return game_value();
            }
        }
    }

    // KH_KHDATA_BOUND: the open file's size in bytes, the reads' limit (0 when it cannot be told - the stream has
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
                
                // KH_KHDATA_BACKUP: a .backup stays - load_file keeps it the last copy that loaded, and
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
            if (value_size == KHDATA_SIZE_DEEP) continue;   // KH_KHDATA_BOUND: not written (save_file).
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

    KHDataFile* get_or_create_file(const std::string& filename) {
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
            // KH_KHDATA_BACKUP: a main cut short (a crash before its data reached the disk) is the shape the
            // backup is kept for - left unloaded, the next save would replace it with an empty file.
            if (std::filesystem::exists(backup_path)) return load_file_from_backup(filename);
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
            // KH_KHDATA_BACKUP: whether the values came from the main, every byte of it accounted for - only such
            // a main refreshes the backup below.
            bool khld_exact = true;

            if (!stream) {
                // Try backup if main fails
                if (std::filesystem::exists(backup_path)) {
                    stream.open(backup_path, std::ios::binary);
                    khld_exact = false;   // KH_KHDATA_BACKUP: the backup's values - the main is unread.

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
                // KH_KHDATA_BACKUP: no magic - the main's first bytes are lost (zeroed by a crash), so the
                // backup. A version this build does not know is a real file, not damage: left as it is - as is
                // a header that failed to read (above), which may be a passing read error on a good file.
                if (magic != KHDATA_MAGIC && std::filesystem::exists(backup_path)) {
                    stream.close();   // The restore copies the backup over this file.
                    return load_file_from_backup(filename);
                }

                return false;
            }
            
            // Read variables
            std::unordered_map<std::string, game_value> vars;
            const uint64_t limit = stream_bytes(stream);   // KH_KHDATA_BOUND.
            uint64_t elems = limit / KHDATA_MIN_VALUE_BYTES;
            
            for (uint32_t i = 0; i < var_count; i++) {
                std::string var_name = read_string(stream, limit);
                game_value value = read_game_value(stream, limit, elems);
                vars[var_name] = value;
            }
            
            if (!stream.eof() && stream.peek() != EOF) {
                sqf::diag_log("Warning - extra data in file: " + filename);
                // File may be corrupt but we loaded what we could
                khld_exact = false;   // KH_KHDATA_BACKUP: bytes left over (a count damaged lower) - not exact.
            }

            // Create file object
            auto file = std::make_unique<KHDataFile>(filename);
            file->set_filepath(filepath);
            file->set_variables(std::move(vars));
            
            // On successful load and validation, create/update backup if needed
            try {
                bool should_backup = khld_exact;   // KH_KHDATA_BACKUP: from an exact main only.
                
                if (should_backup && std::filesystem::exists(backup_path)) {
                    auto main_time = std::filesystem::last_write_time(filepath);
                    auto backup_time = std::filesystem::last_write_time(backup_path);
                    // KH_KHDATA_BACKUP: any difference - the copy keeps the main's time (CopyFile), so equal
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
            const uint64_t limit = stream_bytes(stream);   // KH_KHDATA_BOUND.
            uint64_t elems = limit / KHDATA_MIN_VALUE_BYTES;
            
            for (uint32_t i = 0; i < var_count; i++) {
                std::string var_name = read_string(stream, limit);

                if (var_name.length() > 256) {  // Sanity check
                    report_error("Variable name too long in backup");
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
            // KH_KHDATA_BACKUP: the main it replaces is kept first, as <file>.khdata.bad - the load that failed may
            // have met a passing read error on a good file newer than the backup. One copy, the latest; no
            // load reads it (initialize takes ".khdata" only), delete_file removes it. A main that cannot be kept
            // is not replaced, and the backup stays: the values are the backup's, in memory only, until a save.
            bool khkb_kept = true, khkb_restored = false;
            {
                auto bad_path = main_path;
                bad_path += ".bad";
                std::error_code bad_ec;
                const bool main_there = std::filesystem::exists(main_path, bad_ec);
                if (bad_ec) {
                    khkb_kept = false;
                } else if (main_there) {
                    std::filesystem::copy_file(main_path, bad_path, std::filesystem::copy_options::overwrite_existing,
                                               bad_ec);
                    khkb_kept = !bad_ec;
                }
            }

            if (khkb_kept) {
                try {
                    std::filesystem::copy_file(backup_path, main_path,
                        std::filesystem::copy_options::overwrite_existing);
                    khkb_restored = true;
                    sqf::diag_log("Restored " + filename + " from backup");
                } catch (...) {
                    report_error("Could not restore main file from backup for " + filename);
                }
            } else {
                report_error("Loaded " + filename + " from its backup; its main file could not be read or set aside "
                             "and was left as it is");
            }
            
            files[filename] = std::move(file);
            update_total_size();

            if (khkb_restored) {   // KH_KHDATA_BACKUP: else the backup is the only copy on disk of what loaded.
                try {
                    std::filesystem::remove(backup_path);
                    sqf::diag_log("Deleted backup after successful restoration: " + filename);
                } catch (...) {
                    // Non-critical if we can't delete the backup
                }
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

    // KH_KHDATA_BOUND: what calculate_value_size answers for a value that reaches KHDATA_MAX_DEPTH - one that
    // save_file does not write. It is passed straight up, so the walk stops at the first such path (an array
    // that holds itself twice would otherwise be walked 2^256 times; past the depth, the stack's).
    static constexpr size_t KHDATA_SIZE_DEEP = ~static_cast<size_t>(0);

    static size_t calculate_value_size(const game_value& value, int depth = 0) {       
        if (depth >= KHDATA_MAX_DEPTH) return KHDATA_SIZE_DEEP;   // KH_KHDATA_BOUND.
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
                    if (elem_size == KHDATA_SIZE_DEEP) return KHDATA_SIZE_DEEP;   // KH_KHDATA_BOUND.
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
                    if (key_size == KHDATA_SIZE_DEEP) return KHDATA_SIZE_DEEP;   // KH_KHDATA_BOUND.
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
                // KH_KHDATA_BOUND: only values the reader accepts. write_variable refused deeper ones, but the file
                // holds the script's own array, which the script can nest further after the write; written, it
                // would fail the whole file's next load. Left out (it stays in memory) and reported.
                std::vector<const std::pair<const std::string, game_value>*> khsv_vars;
                std::string khsv_skipped;
                khsv_vars.reserve(file->get_variables().size());

                for (const auto& khsv_v : file->get_variables()) {
                    if (KHDataFile::depth_ok(khsv_v.second, 0)) {
                        khsv_vars.push_back(&khsv_v);
                    } else {
                        khsv_skipped += (khsv_skipped.empty() ? "'" : ", '") + khsv_v.first + "'";
                    }
                }

                if (!khsv_skipped.empty()) {
                    report_error("KHData: not saved - nested " + std::to_string(KHDATA_MAX_DEPTH) +
                                 " levels deep or more since written: " + khsv_skipped + " in " +
                                 file->get_filepath().string());
                }

                uint32_t var_count = static_cast<uint32_t>(khsv_vars.size());
                stream.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
                stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
                stream.write(reinterpret_cast<const char*>(&var_count), sizeof(var_count));
                
                // Write variables
                for (const auto* khsv_v : khsv_vars) {
                    write_string(stream, khsv_v->first);
                    write_game_value(stream, khsv_v->second);
                }

                if (!stream.good()) {
                    file->mark_save_failed();
                    report_error("Write failed for file: " + file->get_filepath().string());
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
                report_error("Failed to save file: " + ec.message());
                return false;
            }
            
            std::filesystem::remove(temp_path);
            temp_guard.should_delete = false;
            file->clear_dirty();
            return true;
        } catch (...) {
            file->mark_save_failed();
            report_error("Failed to save file");
            return false;
        }
    }

    bool delete_file(const std::string& filename) {        
        if (!validate_filename(filename)) {
            report_error("Invalid filename for deletion: " + filename);
            return false;
        }
        
        auto it = files.find(filename);

        if (it != files.end() && it->second->needs_save()) {
            report_error("Attempting to save dirty file before deletion: " + filename);

            if (!save_file(it->second.get())) {
                sqf::diag_log("Warning - failed to save dirty file before deletion: " + filename);
                // Continue with deletion anyway
            }
        }

        // Remove from memory
        files.erase(filename);
        update_total_size();
        
        // Delete from disk
        auto filepath = base_path / (filename + ".khdata");
        // KH_KHDATA_BACKUP: its backup goes with it - a later file of the name must not restore deleted data.
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