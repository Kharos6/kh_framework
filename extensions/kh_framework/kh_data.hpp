#pragma once

using namespace intercept;
using namespace intercept::types;

constexpr uint32_t KHDATA_MAGIC = 0x5444484B; // "KHDT"
constexpr uint32_t KHDATA_VERSION = 1;
constexpr size_t MAX_KHDATA_FILES = 1024;
constexpr size_t MAX_TOTAL_KHDATA_SIZE = 1024LL * 1024LL * 1024LL;
constexpr int MAX_KHDATA_SAVE_ATTEMPTS = 3;

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
                    serialized = static_cast<int>(sqf::ctrl_idd(value));
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

                    return sqf::grp_null();
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
                    
                    // Compile and call the config path string to get actual config
                    game_value compiled = sqf::compile(serialized);
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
                    
                    // Search for matching location by class Name
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
                    //Reconstruct from array
                    uint32_t size;
                    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
                    auto_array<game_value> arr;
                    arr.reserve(size);

                    for (uint32_t i = 0; i < size; i++) {
                        arr.push_back(read_game_value(stream));
                    }

                    return raw_call_sqf_args_native(g_compiled_sqf_create_hash_map_from_array, game_value(std::move(arr)));
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
                    size += sizeof(uint32_t) + 19;  // Potential "UNREGISTERED_OBJECT"
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
                    size += sizeof(uint32_t) + 13;  // Potential "LOCATION_NULL"
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
                    size += sizeof(uint32_t) + 19;  // Potential "UNREGISTERED_OBJECT"
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
            report_error("Failed to save file");
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