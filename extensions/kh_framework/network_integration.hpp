#pragma once

using namespace intercept;
using namespace intercept::types;

constexpr uint32_t NET_MSG_MAGIC = 0x4E48484B; // "KHHN" - KH Network Header
constexpr uint32_t NET_MSG_VERSION = 1;
constexpr size_t NET_MAX_MESSAGE_SIZE = 16 * 1024 * 1024; // 16MB max message size
constexpr size_t NET_RECV_BUFFER_SIZE = 262144; // 256KB receive buffer
constexpr size_t NET_SEND_BUFFER_SIZE = 262144; // 256KB send buffer
constexpr int NET_DEFAULT_PORT = 21337;
constexpr int NET_CONNECT_TIMEOUT_MS = 5000;
constexpr int NET_SEND_TIMEOUT_MS = 3000;
constexpr int NET_RECV_TIMEOUT_MS = 3000;

// Global handler ID counter
static std::atomic<uint64_t> g_network_handler_id_counter{0};

// Network message structure for queue
struct NetworkMessage {
    std::string event_name;
    std::vector<uint8_t> payload;
    int sender_client_id;
    
    NetworkMessage() : sender_client_id(-1) {}
    NetworkMessage(const std::string& name, std::vector<uint8_t>&& data, int sender = -1)
        : event_name(name), payload(std::move(data)), sender_client_id(sender) {}
};

struct OutgoingMessage {
    std::vector<uint8_t> payload;
    std::string event_name;
    int target_client;
    int sender_client;
    OutgoingMessage() : target_client(-1), sender_client(-1) {}
    
    OutgoingMessage(int target, int sender, std::string&& event, std::vector<uint8_t>&& data)
        : payload(std::move(data)), event_name(std::move(event)), 
          target_client(target), sender_client(sender) {}
};

// Message handler structure
struct NetworkMessageHandler {
    int handler_id;
    std::string event_name;
    code handler_function;
    game_value handler_arguments;
};

class NetworkFramework {
private:
    // Singleton
    NetworkFramework() = default;
    ~NetworkFramework() { shutdown(); }
    NetworkFramework(const NetworkFramework&) = delete;
    NetworkFramework& operator=(const NetworkFramework&) = delete;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    std::atomic<int> network_port_{NET_DEFAULT_PORT};
    SOCKET listen_socket_{INVALID_SOCKET};
    std::unordered_map<int, SOCKET> client_connections_;
    std::mutex connections_mutex_;
    SOCKET server_connection_{INVALID_SOCKET};
    std::mutex server_connection_mutex_;
    std::string local_ip_;
    std::mutex local_ip_mutex_;
    std::atomic<bool> is_server_{false};
    std::string server_ip_;
    std::mutex server_ip_mutex_;
    std::unordered_map<std::string, std::vector<NetworkMessageHandler>> message_handlers_;
    std::unordered_map<int, std::string> handler_id_to_event_;
    std::mutex handlers_mutex_;
    
    // Incoming message queue
    std::deque<NetworkMessage> incoming_queue_;
    std::mutex incoming_mutex_;
    
    // Outgoing message queue
    std::deque<OutgoingMessage> outgoing_queue_;
    std::mutex outgoing_mutex_;
    std::condition_variable outgoing_cv_;
    
    // Network threads
    std::thread listen_thread_;
    std::thread send_thread_;
    std::thread receive_thread_;
    
    // Initialization
    bool wsa_initialized_{false};
    std::atomic<int> cached_client_id_{-1};
    std::vector<std::pair<int, SOCKET>> listen_active_clients_;
    std::vector<int> listen_clients_to_remove_;
    std::vector<NetworkMessage> listen_messages_to_queue_;
    
    static void write_uint32(std::vector<uint8_t>& buffer, uint32_t value) {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }
    
    static uint32_t read_uint32(const uint8_t* data, size_t& offset, size_t max_size) {
        if (offset + 4 > max_size) {
            throw std::runtime_error("Buffer underrun reading uint32");
        }
        
        uint32_t value = static_cast<uint32_t>(data[offset]) |
                        (static_cast<uint32_t>(data[offset + 1]) << 8) |
                        (static_cast<uint32_t>(data[offset + 2]) << 16) |
                        (static_cast<uint32_t>(data[offset + 3]) << 24);

        offset += 4;
        return value;
    }
    
    static void write_string(std::vector<uint8_t>& buffer, const std::string& str) {
        write_uint32(buffer, static_cast<uint32_t>(str.length()));
        buffer.insert(buffer.end(), str.begin(), str.end());
    }
    
    static std::string read_string(const uint8_t* data, size_t& offset, size_t max_size) {
        uint32_t len = read_uint32(data, offset, max_size);

        if (len > max_size - offset) {
            throw std::runtime_error("String length exceeds buffer size");
        }

        std::string result(reinterpret_cast<const char*>(data + offset), len);
        offset += len;
        return result;
    }
    
    static void write_float(std::vector<uint8_t>& buffer, float value) {
        uint32_t raw;
        std::memcpy(&raw, &value, sizeof(float));
        write_uint32(buffer, raw);
    }
    
    static float read_float(const uint8_t* data, size_t& offset, size_t max_size) {
        uint32_t raw = read_uint32(data, offset, max_size);
        float value;
        std::memcpy(&value, &raw, sizeof(float));
        return value;
    }

    static void write_bool(std::vector<uint8_t>& buffer, bool value) {
        buffer.push_back(value ? 1 : 0);
    }

    static bool read_bool(const uint8_t* data, size_t& offset, size_t max_size) {
        if (offset + 1 > max_size) {
            throw std::runtime_error("Buffer underrun reading bool");
        }

        bool value = data[offset] != 0;
        offset += 1;
        return value;
    }
    
    static void serialize_game_value(std::vector<uint8_t>& buffer, const game_value& value) {
        auto type = value.type_enum();
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
                throw std::runtime_error("Cannot serialize unsupported network type");
            default:
                break;
        }

        write_bool(buffer, needs_special_handling);
        
        if (needs_special_handling) {
            write_uint32(buffer, static_cast<uint32_t>(type));
            std::string serialized;
            
            switch (type) {
                case game_data_type::CODE: {
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
                    if (value == sqf::west() || value == sqf::blufor()) {
                        serialized = "west";
                    } else if (value == sqf::east() || value == sqf::opfor()) {
                        serialized = "east";
                    } else if (value == sqf::resistance() || value == sqf::independent()) {
                        serialized = "resistance";
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
                    serialized = static_cast<std::string>(sqf::net_id(static_cast<group>(value)));
                    break;
                }

                case game_data_type::OBJECT: {
                    serialized = static_cast<std::string>(sqf::net_id(static_cast<object>(value)));
                    break;
                }
                
                case game_data_type::TEXT: {
                    std::string text_str = static_cast<std::string>(value);
                    serialized = text_str;
                    break;
                }
                
                case game_data_type::CONFIG: {
                    auto hierarchy = sqf::config_hierarchy(value);

                    if (hierarchy.size() <= 1) {
                        serialized = "configFile";
                    } else {
                        std::string config_path = "configFile";

                        for (size_t i = 1; i < hierarchy.size(); i++) {
                            std::string entry = static_cast<std::string>(hierarchy[i]);
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
                    std::string location_name = static_cast<std::string>(sqf::class_name(value));
                    serialized = location_name.empty() ? "LOCATION_NULL" : location_name;
                    break;
                }
                
                case game_data_type::TEAM_MEMBER: {
                    game_value agent_obj = sqf::agent(value);
                    serialized = static_cast<std::string>(sqf::net_id(static_cast<object>(agent_obj)));
                    break;
                }
                
                case game_data_type::DISPLAY: {
                    serialized = std::to_string(static_cast<int>(sqf::ctrl_idd(value)));
                    break;
                }
                
                default:
                    throw std::runtime_error("Cannot serialize game value type");
            }
            
            write_string(buffer, serialized);
        } else {
            write_uint32(buffer, static_cast<uint32_t>(type));
            
            // Write value normally for simple types
            switch (type) {
                case game_data_type::NOTHING:
                case game_data_type::ANY:
                    break;
                    
                case game_data_type::SCALAR: {
                    float val = value;
                    write_float(buffer, val);
                    break;
                }
                
                case game_data_type::BOOL: {
                    bool val = value;
                    write_bool(buffer, val);
                    break;
                }
                
                case game_data_type::STRING: {
                    std::string str_val = static_cast<std::string>(value);
                    write_string(buffer, str_val);
                    break;
                }
                
                case game_data_type::ARRAY: {
                    auto& arr = value.to_array();
                    write_uint32(buffer, static_cast<uint32_t>(arr.size()));

                    for (const auto& elem : arr) {
                        serialize_game_value(buffer, elem);
                    }

                    break;
                }
                
                case game_data_type::HASHMAP: {
                    auto& map = value.to_hashmap();
                    write_uint32(buffer, static_cast<uint32_t>(map.count()));

                    for (const auto& pair : map) {
                        serialize_game_value(buffer, pair.key);
                        serialize_game_value(buffer, pair.value);
                    }

                    break;
                }
                
                default:
                    break;
            }
        }
    }
    
    static game_value deserialize_game_value(const uint8_t* data, size_t& offset, size_t max_size) {
        if (offset >= max_size) {
            throw std::runtime_error("Buffer underrun during deserialization");
        }
        
        bool is_serialized = read_bool(data, offset, max_size);
        
        if (is_serialized) {
            game_data_type original_type = static_cast<game_data_type>(read_uint32(data, offset, max_size));
            std::string serialized = read_string(data, offset, max_size);
            
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
                    if (serialized == "east") return sqf::east();
                    if (serialized == "resistance") return sqf::resistance();
                    if (serialized == "civilian") return sqf::civilian();
                    if (serialized == "sideLogic") return sqf::side_logic();
                    if (serialized == "sideUnknown") return sqf::side_unknown();
                    if (serialized == "sideEnemy") return sqf::side_enemy();
                    if (serialized == "sideFriendly") return sqf::side_friendly();
                    if (serialized == "sideAmbientLife") return sqf::side_ambient_life();
                    if (serialized == "sideEmpty") return sqf::side_empty();
                    return sqf::side_unknown();
                
                case game_data_type::GROUP: {
                    return sqf::group_from_net_id(serialized);
                }
                
                case game_data_type::OBJECT: {
                    return sqf::object_from_net_id(serialized);
                }
                
                case game_data_type::TEXT: {
                    if (serialized.empty()) {
                        return sqf::parse_text("");
                    }

                    return sqf::parse_text(serialized);
                }
                
                case game_data_type::CONFIG: {
                    if (serialized == "configFile" || serialized.empty()) {
                        return sqf::config_file();
                    }

                    code compiled = sqf::compile("setReturnValue (call {" + serialized + "});");
                    return raw_call_sqf_native(compiled);
                }
                
                case game_data_type::LOCATION: {
                    if (serialized == "LOCATION_NULL" || serialized.empty()) {
                        return sqf::location_null();
                    }

                    float world_size = sqf::world_size();
                    vector3 center(world_size / 2.0f, world_size / 2.0f, 0.0f);
                    float radius = world_size * std::sqrt(2.0f) / 2.0f;
                    std::vector<std::string> all_types;
                    auto all_locations = sqf::nearest_locations(center, all_types, radius);

                    for (const auto& loc : all_locations) {
                        if (static_cast<std::string>(sqf::class_name(loc)) == serialized) {
                            return loc;
                        }
                    }

                    return sqf::location_null();
                }
                
                case game_data_type::TEAM_MEMBER: {
                    game_value agent_obj = sqf::object_from_net_id(serialized);

                    if (agent_obj.is_null()) {
                        return sqf::team_member_null();
                    }

                    return sqf::agent(agent_obj);
                }
                
                case game_data_type::DISPLAY: {
                    int idd = std::stoi(serialized);
                    return sqf::find_display(idd);
                }
                
                default:
                    return game_value();
            }
        } else {
            game_data_type type = static_cast<game_data_type>(read_uint32(data, offset, max_size));
            
            switch (type) {
                case game_data_type::NOTHING:
                case game_data_type::ANY:
                    return game_value();
                
                case game_data_type::SCALAR: {
                    float val = read_float(data, offset, max_size);
                    return game_value(val);
                }
                
                case game_data_type::BOOL: {
                    bool val = read_bool(data, offset, max_size);
                    return game_value(val);
                }
                
                case game_data_type::STRING: {
                    std::string str = read_string(data, offset, max_size);
                    return game_value(str);
                }
                
                case game_data_type::ARRAY: {
                    uint32_t size = read_uint32(data, offset, max_size);
                    auto_array<game_value> arr;
                    arr.reserve(size);

                    for (uint32_t i = 0; i < size; i++) {
                        arr.push_back(deserialize_game_value(data, offset, max_size));
                    }

                    return game_value(std::move(arr));
                }
                
                case game_data_type::HASHMAP: {
                    uint32_t size = read_uint32(data, offset, max_size);
                    auto_array<game_value> pairs;
                    pairs.reserve(size);

                    for (uint32_t i = 0; i < size; i++) {
                        auto_array<game_value> kv;
                        kv.push_back(deserialize_game_value(data, offset, max_size));
                        kv.push_back(deserialize_game_value(data, offset, max_size));
                        pairs.push_back(game_value(std::move(kv)));
                    }

                    return raw_call_sqf_args_native(g_compiled_sqf_create_hash_map_from_array, game_value(std::move(pairs)));
                }
                
                default:
                    return game_value();
            }
        }
    }

    // Message format: [MAGIC(4)][VERSION(4)][TARGET_CLIENT(4)][SENDER_CLIENT(4)][EVENT_LEN(4)][EVENT_NAME][PAYLOAD_LEN(4)][PAYLOAD]

    static void create_network_packet_into(std::vector<uint8_t>& packet, int target_client, int sender_client,
                                            const std::string& event_name, const std::vector<uint8_t>& payload) {
        size_t required_size = 24 + event_name.size() + payload.size();
        packet.clear();
        packet.reserve(required_size);
        
        write_uint32(packet, NET_MSG_MAGIC);
        write_uint32(packet, NET_MSG_VERSION);
        write_uint32(packet, static_cast<uint32_t>(target_client));
        write_uint32(packet, static_cast<uint32_t>(sender_client));
        write_string(packet, event_name);
        write_uint32(packet, static_cast<uint32_t>(payload.size()));
        packet.insert(packet.end(), payload.begin(), payload.end());
    }

    static std::vector<uint8_t> create_network_packet(int target_client, int sender_client,
                                                    const std::string& event_name,
                                                    const std::vector<uint8_t>& payload) {
        std::vector<uint8_t> packet;
        create_network_packet_into(packet, target_client, sender_client, event_name, payload);
        return packet;
    }
        
    static bool parse_network_packet(const std::vector<uint8_t>& data, int& target_client, 
                                     int& sender_client, std::string& event_name, 
                                     std::vector<uint8_t>& payload) {
        if (data.size() < 24) return false;
        size_t offset = 0;
        uint32_t magic = read_uint32(data.data(), offset, data.size());
        if (magic != NET_MSG_MAGIC) return false;
        uint32_t version = read_uint32(data.data(), offset, data.size());
        if (version != NET_MSG_VERSION) return false;
        target_client = static_cast<int>(read_uint32(data.data(), offset, data.size()));
        sender_client = static_cast<int>(read_uint32(data.data(), offset, data.size()));
        
        try {
            event_name = read_string(data.data(), offset, data.size());
            uint32_t payload_len = read_uint32(data.data(), offset, data.size());
            if (payload_len > data.size() - offset) return false;
            payload.assign(data.begin() + offset, data.begin() + offset + payload_len);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool set_socket_options(SOCKET sock) {
        // Set TCP_NODELAY for low latency
        int flag = 1;

        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag)) != 0) {
            return false;
        }
        
        // Set send/recv timeouts
        DWORD timeout = NET_SEND_TIMEOUT_MS;
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        timeout = NET_RECV_TIMEOUT_MS;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        
        // Large-ish buffer sizes
        int recvbuf = NET_RECV_BUFFER_SIZE;
        int sendbuf = NET_SEND_BUFFER_SIZE;
        setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&recvbuf), sizeof(recvbuf));
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&sendbuf), sizeof(sendbuf));
        
        // Enable keep-alive for connection health
        int keepalive = 1;
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&keepalive), sizeof(keepalive));
        return true;
    }
    
    bool send_all(SOCKET sock, const std::vector<uint8_t>& data) {
        // Combine length prefix and data into single buffer to reduce syscalls
        size_t total_size = 4 + data.size();
        
        // Use stack buffer for small messages, heap for large
        constexpr size_t STACK_THRESHOLD = 8192;
        uint8_t stack_buf[STACK_THRESHOLD];
        std::vector<uint8_t> heap_buf;
        uint8_t* send_buf;
        
        if (total_size <= STACK_THRESHOLD) {
            send_buf = stack_buf;
        } else {
            heap_buf.resize(total_size);
            send_buf = heap_buf.data();
        }

        uint32_t len = static_cast<uint32_t>(data.size());
        send_buf[0] = static_cast<uint8_t>(len & 0xFF);
        send_buf[1] = static_cast<uint8_t>((len >> 8) & 0xFF);
        send_buf[2] = static_cast<uint8_t>((len >> 16) & 0xFF);
        send_buf[3] = static_cast<uint8_t>((len >> 24) & 0xFF);
        std::memcpy(send_buf + 4, data.data(), data.size());
        
        // Send all in one or more calls
        size_t total_sent = 0;

        while (total_sent < total_size) {
            int result = send(sock, reinterpret_cast<const char*>(send_buf + total_sent),
                            static_cast<int>(total_size - total_sent), 0);

            if (result == SOCKET_ERROR || result == 0) {
                return false;
            }

            total_sent += result;
        }
        
        return true;
    }
    
    bool recv_all(SOCKET sock, std::vector<uint8_t>& data) {
        thread_local std::vector<uint8_t> tls_recv_buffer;
        uint8_t len_buf[4];
        int total_recv = 0;

        while (total_recv < 4) {
            int result = recv(sock, reinterpret_cast<char*>(len_buf + total_recv), 4 - total_recv, 0);

            if (result == SOCKET_ERROR || result == 0) {
                return false;
            }

            total_recv += result;
        }
        
        uint32_t len = static_cast<uint32_t>(len_buf[0]) |
                    (static_cast<uint32_t>(len_buf[1]) << 8) |
                    (static_cast<uint32_t>(len_buf[2]) << 16) |
                    (static_cast<uint32_t>(len_buf[3]) << 24);
        
        if (len > NET_MAX_MESSAGE_SIZE) {
            return false;
        }
        
        // Resize thread-local buffer if needed (will reuse capacity)
        if (tls_recv_buffer.capacity() < len) {
            tls_recv_buffer.reserve(std::max(len, static_cast<uint32_t>(NET_RECV_BUFFER_SIZE)));
        }

        tls_recv_buffer.resize(len);
        total_recv = 0;

        while (static_cast<uint32_t>(total_recv) < len) {
            int result = recv(sock, reinterpret_cast<char*>(tls_recv_buffer.data() + total_recv),
                            static_cast<int>(len - total_recv), 0);

            if (result == SOCKET_ERROR || result == 0) {
                return false;
            }

            total_recv += result;
        }

        data.swap(tls_recv_buffer);
        return true;
    }

    std::string detect_local_ip() {
        std::string result = "127.0.0.1";
        char hostname[256];

        if (gethostname(hostname, sizeof(hostname)) != 0) {
            return result;
        }
        
        struct addrinfo hints, *info;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        if (getaddrinfo(hostname, nullptr, &hints, &info) != 0) {
            return result;
        }
        
        for (struct addrinfo* p = info; p != nullptr; p = p->ai_next) {
            struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
            std::string ip = ip_str;

            if (ip != "127.0.0.1" && ip.find("169.254.") != 0) {
                result = ip;
                break;
            }
        }
        
        freeaddrinfo(info);
        return result;
    }

    void listen_thread_func() {
        while (running_.load()) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(listen_socket_, &read_fds);
            
            // Reuse vector, just clear and refill
            listen_active_clients_.clear();

            {
                std::lock_guard<std::mutex> lock(connections_mutex_);
                listen_active_clients_.reserve(client_connections_.size());
                
                for (auto& pair : client_connections_) {
                    listen_active_clients_.push_back(pair);
                }
            }

            size_t clients_in_fdset = 0;
            constexpr size_t max_clients_per_select = FD_SETSIZE - 1; // -1 for listen socket

            for (size_t i = 0; i < listen_active_clients_.size() && clients_in_fdset < max_clients_per_select; ++i) {
                FD_SET(listen_active_clients_[i].second, &read_fds);
                clients_in_fdset++;
            }
            
            // Warn once if we hit the limit
            static std::atomic<bool> warned_fd_limit{false};

            if (listen_active_clients_.size() > max_clients_per_select && !warned_fd_limit.exchange(true)) {
                MainThreadScheduler::instance().schedule([]() {
                    sqf::diag_log("KH Network: WARNING - More than " + std::to_string(FD_SETSIZE - 1) + 
                                " clients connected, some may experience delayed messages");
                });
            }
            
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1000; // 1ms timeout for minimal latency
            int result = select(0, &read_fds, nullptr, nullptr, &tv);
            
            if (result == SOCKET_ERROR) {
                continue;
            }
            
            if (result == 0) {
                continue;
            }
            
            // Check for new connections
            if (FD_ISSET(listen_socket_, &read_fds)) {
                struct sockaddr_in client_addr;
                int addr_len = sizeof(client_addr);
                SOCKET client_sock = accept(listen_socket_, reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);
                
                if (client_sock != INVALID_SOCKET) {
                    set_socket_options(client_sock);

                    // Receive client ID
                    std::vector<uint8_t> id_data;

                    if (recv_all(client_sock, id_data) && id_data.size() >= 4) {
                        size_t offset = 0;
                        int client_id = static_cast<int>(read_uint32(id_data.data(), offset, id_data.size()));
                        
                        {
                            std::lock_guard<std::mutex> lock(connections_mutex_);
                            // Close existing connection for this client if any
                            auto existing = client_connections_.find(client_id);

                            if (existing != client_connections_.end()) {
                                closesocket(existing->second);
                            }

                            client_connections_[client_id] = client_sock;
                        }
                        
                        // Schedule log on main thread
                        MainThreadScheduler::instance().schedule([client_id]() {
                            sqf::diag_log("KH Network: Client " + std::to_string(client_id) + " connected");
                        });
                    } else {
                        closesocket(client_sock);
                    }
                }
            }
            
            // Check for incoming data from clients (only those we added to fd_set)
            listen_clients_to_remove_.clear();
            listen_messages_to_queue_.clear();
            
            for (size_t i = 0; i < clients_in_fdset && i < listen_active_clients_.size(); ++i) {
                auto& [client_id, client_sock] = listen_active_clients_[i];

                if (FD_ISSET(client_sock, &read_fds)) {
                    std::vector<uint8_t> packet;

                    if (recv_all(client_sock, packet)) {
                        int target_client, sender_client;
                        std::string event_name;
                        std::vector<uint8_t> payload;
                        
                        if (parse_network_packet(packet, target_client, sender_client, event_name, payload)) {
                            if (target_client == 2) {
                                listen_messages_to_queue_.emplace_back(event_name, std::move(payload), sender_client);
                            } else {
                                {
                                    std::lock_guard<std::mutex> out_lock(outgoing_mutex_);
                                    outgoing_queue_.emplace_back(target_client, sender_client, std::move(event_name), std::move(payload));
                                }

                                outgoing_cv_.notify_one();
                            }
                        }
                    } else {
                        listen_clients_to_remove_.push_back(client_id);
                    }
                }
            }

            if (!listen_messages_to_queue_.empty()) {
                std::lock_guard<std::mutex> inc_lock(incoming_mutex_);

                for (auto& msg : listen_messages_to_queue_) {
                    incoming_queue_.push_back(std::move(msg));
                }
            }
            
            // Remove disconnected clients
            if (!listen_clients_to_remove_.empty()) {
                std::lock_guard<std::mutex> lock(connections_mutex_);

                for (int id : listen_clients_to_remove_) {
                    auto it = client_connections_.find(id);
                    
                    if (it != client_connections_.end()) {
                        closesocket(it->second);
                        client_connections_.erase(it);
                        
                        MainThreadScheduler::instance().schedule([id]() {
                            sqf::diag_log("KH Network: Client " + std::to_string(id) + " disconnected");
                        });
                    }
                }
            }
        }
    }

    bool connect_to_server_internal() {
        std::string server_addr;

        {
            std::lock_guard<std::mutex> ip_lock(server_ip_mutex_);
            server_addr = server_ip_;
        }
        
        if (server_addr.empty()) {
            return false;
        }

        {
            std::lock_guard<std::mutex> conn_lock(server_connection_mutex_);

            if (server_connection_ != INVALID_SOCKET) {
                return true;
            }
        }
        
        // Parse IP:port or just IP
        std::string ip = server_addr;
        int port = network_port_.load();
        size_t colon_pos = server_addr.find(':');

        if (colon_pos != std::string::npos) {
            ip = server_addr.substr(0, colon_pos);

            try {
                port = std::stoi(server_addr.substr(colon_pos + 1));
            } catch (...) {
                return false;
            }
        }
        
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(static_cast<u_short>(port));
        inet_pton(AF_INET, ip.c_str(), &server.sin_addr);
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET) {
            return false;
        }

        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
        connect(sock, reinterpret_cast<struct sockaddr*>(&server), sizeof(server));
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);
        struct timeval tv;
        tv.tv_sec = NET_CONNECT_TIMEOUT_MS / 1000;
        tv.tv_usec = (NET_CONNECT_TIMEOUT_MS % 1000) * 1000;
        
        if (select(0, nullptr, &write_fds, nullptr, &tv) <= 0) {
            closesocket(sock);
            return false;
        }

        mode = 0;
        ioctlsocket(sock, FIONBIO, &mode);
        set_socket_options(sock);
        int our_client_id = cached_client_id_.load();

        if (our_client_id < 0) {
            closesocket(sock);
            return false;
        }
        
        std::vector<uint8_t> id_packet;
        write_uint32(id_packet, static_cast<uint32_t>(our_client_id));
        
        if (!send_all(sock, id_packet)) {
            closesocket(sock);
            return false;
        }

        std::lock_guard<std::mutex> lock(server_connection_mutex_);

        if (server_connection_ != INVALID_SOCKET) {
            // Another thread connected while we were connecting
            closesocket(sock);
            return true;
        }

        server_connection_ = sock;
        return true;
    }
    
    void receive_thread_func() {
        while (running_.load()) {
            if (is_server_.load()) {
                // Server mode - handled by listen thread
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            
            // Client mode - receive from server
            SOCKET sock = INVALID_SOCKET;

            {
                std::lock_guard<std::mutex> lock(server_connection_mutex_);
                sock = server_connection_;
            }
            
            if (sock == INVALID_SOCKET) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sock, &read_fds);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1000; // 1ms timeout for minimal latency
            
            if (select(0, &read_fds, nullptr, nullptr, &tv) > 0) {
                std::vector<uint8_t> packet;
                if (recv_all(sock, packet)) {
                    int target_client, sender_client;
                    std::string event_name;
                    std::vector<uint8_t> payload;
                    
                    if (parse_network_packet(packet, target_client, sender_client, event_name, payload)) {
                        std::lock_guard<std::mutex> lock(incoming_mutex_);
                        incoming_queue_.emplace_back(event_name, std::move(payload), sender_client);
                    }
                } else {
                    // Connection lost
                    std::lock_guard<std::mutex> lock(server_connection_mutex_);

                    if (server_connection_ == sock) {
                        closesocket(server_connection_);
                        server_connection_ = INVALID_SOCKET;
                    }
                }
            }
        }
    }
    
    void send_thread_func() {
        std::vector<OutgoingMessage> batch;
        batch.reserve(64); // Pre-allocate for batch processing
        std::vector<uint8_t> packet_buffer;
        packet_buffer.reserve(NET_RECV_BUFFER_SIZE);
        
        while (running_.load()) {
            {
                std::unique_lock<std::mutex> lock(outgoing_mutex_);
                
                outgoing_cv_.wait_for(lock, std::chrono::milliseconds(1), [this]() {
                    return !outgoing_queue_.empty() || !running_.load();
                });
                
                if (!running_.load()) break;
                
                // Grab all pending messages at once
                while (!outgoing_queue_.empty() && batch.size() < 64) {
                    batch.push_back(std::move(outgoing_queue_.front()));
                    outgoing_queue_.pop_front();
                }
            }

            if (batch.empty()) {
                continue;
            }
            
            // Process batch without holding lock
            for (size_t i = 0; i < batch.size(); ++i) {
                auto& msg = batch[i];
                create_network_packet_into(packet_buffer, msg.target_client, msg.sender_client, msg.event_name, msg.payload);
                
                if (is_server_.load()) {
                    // Server mode - send directly to client
                    std::lock_guard<std::mutex> conn_lock(connections_mutex_);
                    auto it = client_connections_.find(msg.target_client);

                    if (it != client_connections_.end()) {
                        send_all(it->second, packet_buffer);
                    }
                } else {
                    // Client mode - send to server for relaying
                    if (!connect_to_server_internal()) {
                        // Re-queue this and all remaining messages
                        std::lock_guard<std::mutex> lock(outgoing_mutex_);

                        for (size_t j = batch.size(); j-- > i; ) {
                            outgoing_queue_.push_front(std::move(batch[j]));
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        break;
                    }

                    std::lock_guard<std::mutex> srv_lock(server_connection_mutex_);

                    if (server_connection_ != INVALID_SOCKET) {
                        if (!send_all(server_connection_, packet_buffer)) {
                            closesocket(server_connection_);
                            server_connection_ = INVALID_SOCKET;
                        }
                    }
                }
            }

            batch.clear();
        }
    }

public:
    static NetworkFramework& instance() {
        static NetworkFramework inst;
        return inst;
    }
    
    bool initialize() {
        if (initialized_.load()) {
            return true;
        }
        
        // Initialize Winsock
        WSADATA wsa_data;

        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            report_error("KH Network: WSAStartup failed");
            return false;
        }

        wsa_initialized_ = true;
        
        // Detect local IP
        {
            std::lock_guard<std::mutex> lock(local_ip_mutex_);
            local_ip_ = detect_local_ip();
        }
        
        initialized_.store(true);
        return true;
    }
    
    bool start(bool as_server) {
        if (!initialized_.load()) {
            if (!initialize()) {
                return false;
            }
        }
        
        if (running_.load()) {
            return true;
        }
        
        // Cache client ID from main thread before starting background threads
        // This MUST be called from the main thread
        cached_client_id_.store(static_cast<int>(sqf::client_owner()));
        is_server_.store(as_server);
        
        if (as_server) {
            // Create listening socket
            listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            if (listen_socket_ == INVALID_SOCKET) {
                report_error("KH Network: Failed to create listen socket");
                return false;
            }
            
            // Allow address reuse
            int reuse = 1;
            setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = htons(static_cast<u_short>(network_port_.load()));
            
            if (bind(listen_socket_, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
                report_error("KH Network: Failed to bind listen socket on port " + std::to_string(network_port_.load()));
                closesocket(listen_socket_);
                listen_socket_ = INVALID_SOCKET;
                return false;
            }
            
            if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
                report_error("KH Network: Failed to listen");
                closesocket(listen_socket_);
                listen_socket_ = INVALID_SOCKET;
                return false;
            }
            
            sqf::diag_log("KH Network: Server listening on port " + std::to_string(network_port_.load()));
        }
        
        running_.store(true);

        if (as_server) {
            listen_thread_ = std::thread(&NetworkFramework::listen_thread_func, this);
        }

        receive_thread_ = std::thread(&NetworkFramework::receive_thread_func, this);
        send_thread_ = std::thread(&NetworkFramework::send_thread_func, this);
        return true;
    }
    
    void shutdown() {
        if (!running_.load() && !initialized_.load()) {
            return;
        }
        
        running_.store(false);
        outgoing_cv_.notify_all();

        if (listen_thread_.joinable()) {
            listen_thread_.join();
        }

        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }

        if (send_thread_.joinable()) {
            send_thread_.join();
        }
        
        // Close all sockets
        if (listen_socket_ != INVALID_SOCKET) {
            closesocket(listen_socket_);
            listen_socket_ = INVALID_SOCKET;
        }
        
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);

            for (auto& pair : client_connections_) {
                closesocket(pair.second);
            }

            client_connections_.clear();

            // Clear reusable listen thread buffers
            listen_active_clients_.clear();
            listen_active_clients_.shrink_to_fit();
            listen_clients_to_remove_.clear();
            listen_clients_to_remove_.shrink_to_fit();
            listen_messages_to_queue_.clear();
            listen_messages_to_queue_.shrink_to_fit();
        }
        
        {
            std::lock_guard<std::mutex> lock(server_connection_mutex_);

            if (server_connection_ != INVALID_SOCKET) {
                closesocket(server_connection_);
                server_connection_ = INVALID_SOCKET;
            }
        }

        {
            std::lock_guard<std::mutex> lock(incoming_mutex_);
            incoming_queue_.clear();
        }

        {
            std::lock_guard<std::mutex> lock(outgoing_mutex_);
            outgoing_queue_.clear();
        }

        {
            std::lock_guard<std::mutex> lock(handlers_mutex_);
            message_handlers_.clear();
            handler_id_to_event_.clear();
        }
        
        cached_client_id_.store(-1);
        initialized_.store(false);

        if (wsa_initialized_) {
            WSACleanup();
            wsa_initialized_ = false;
        }
        
        sqf::diag_log("KH Network: SHUTDOWN");
    }
    
    void set_port(int port) {
        network_port_.store(port);
    }
    
    int get_port() const {
        return network_port_.load();
    }
    
    std::string get_local_ip_port() {
        std::lock_guard<std::mutex> lock(local_ip_mutex_);
        return local_ip_ + ":" + std::to_string(network_port_.load());
    }
    
    void set_server_ip(const std::string& ip) {
        std::lock_guard<std::mutex> lock(server_ip_mutex_);
        server_ip_ = ip;
    }
    
    bool send_message(int target_client, const std::string& event_name, const game_value& message) {
        if (!running_.load()) {
            return false;
        }
        
        try {
            std::vector<uint8_t> payload;
            serialize_game_value(payload, message);
            int our_client_id = cached_client_id_.load();
            
            // Special case: server sending to itself (client 2)
            // Or any machine sending to its own client ID
            if (target_client == our_client_id || 
                (is_server_.load() && target_client == 2)) {
                // Queue directly to incoming queue - no network needed
                std::lock_guard<std::mutex> lock(incoming_mutex_);
                incoming_queue_.emplace_back(event_name, std::move(payload), our_client_id);
                return true;
            }
            
            {
                std::lock_guard<std::mutex> lock(outgoing_mutex_);
                outgoing_queue_.emplace_back(target_client, our_client_id, std::string(event_name), std::move(payload));
            }

            outgoing_cv_.notify_one();
            return true;
        } catch (const std::exception& e) {
            report_error("KH Network: Failed to serialize message - " + std::string(e.what()));
            return false;
        }
    }
    
    int add_message_handler(const std::string& event_name, const code& handler, const game_value& args) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        int handler_id = static_cast<int>(g_network_handler_id_counter.fetch_add(1) & 0x7FFFFFFF);
        message_handlers_[event_name].push_back({handler_id, event_name, handler, args});
        handler_id_to_event_[handler_id] = event_name;
        return handler_id;
    }
    
    bool remove_message_handler(int handler_id) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        
        // Find which event this handler belongs to
        auto id_it = handler_id_to_event_.find(handler_id);

        if (id_it == handler_id_to_event_.end()) {
            return false;
        }
        
        std::string event_name = id_it->second;
        handler_id_to_event_.erase(id_it);
        auto handlers_it = message_handlers_.find(event_name);

        if (handlers_it != message_handlers_.end()) {
            auto& handlers = handlers_it->second;
            
            handlers.erase(
                std::remove_if(handlers.begin(), handlers.end(),
                    [handler_id](const NetworkMessageHandler& h) { return h.handler_id == handler_id; }),
                handlers.end()
            );

            if (handlers.empty()) {
                message_handlers_.erase(handlers_it);
            }
        }
        
        return true;
    }
    
    // Called from main thread during on_frame
    void process_incoming_messages() {
        std::vector<NetworkMessage> messages;

        {
            std::lock_guard<std::mutex> lock(incoming_mutex_);

            messages = std::vector<NetworkMessage>(
                std::make_move_iterator(incoming_queue_.begin()),
                std::make_move_iterator(incoming_queue_.end())
            );

            incoming_queue_.clear();
        }
        
        for (auto& msg : messages) {
            try {
                // Handle empty payload - deserialize as nil/nothing
                game_value deserialized;
                
                if (!msg.payload.empty()) {
                    size_t offset = 0;
                    deserialized = deserialize_game_value(msg.payload.data(), offset, msg.payload.size());
                }
                
                std::vector<NetworkMessageHandler> handlers_copy;

                {
                    std::lock_guard<std::mutex> lock(handlers_mutex_);
                    auto it = message_handlers_.find(msg.event_name);

                    if (it != message_handlers_.end()) {
                        handlers_copy = it->second;
                    }
                }
                
                for (auto& handler : handlers_copy) {
                    try {
                        // Create arguments array: [message, sender_client_id, handler_args]
                        auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;
                        static r_string message_name = "_this"sv;
                        static r_string sender_name = "_sender"sv;
                        static r_string args_name = "_args"sv;
                        game_state->set_local_variable(message_name, deserialized);
                        game_state->set_local_variable(sender_name, game_value(static_cast<float>(msg.sender_client_id)));
                        game_state->set_local_variable(args_name, handler.handler_arguments);
                        intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, handler.handler_function);
                    } catch (const std::exception& e) {
                        report_error("KH Network: Handler error for '" + msg.event_name + "' - " + std::string(e.what()));
                    }
                }
            } catch (const std::exception& e) {
                report_error("KH Network: Failed to deserialize message - " + std::string(e.what()));
            }
        }
    }
    
    bool is_initialized() const {
        return initialized_.load();
    }
    
    bool is_running() const {
        return running_.load();
    }
    
    bool is_server() const {
        return is_server_.load();
    }
};

static void network_pre_init() {
    if (!NetworkFramework::instance().initialize()) {
        report_error("KH Network: Failed to initialize");
        return;
    }
    
    if (sqf::is_server()) {
        std::string ip_port = NetworkFramework::instance().get_local_ip_port();
        sqf::set_variable(sqf::mission_namespace(), "kh_var_serverAddress", game_value(ip_port), true);

        if (NetworkFramework::instance().start(true)) {
            sqf::diag_log("KH Network: Server started successfully");
        }
    }
}

static void network_on_frame() {
    if (NetworkFramework::instance().is_running()) {
        NetworkFramework::instance().process_incoming_messages();
    }
}