#pragma once

using namespace intercept;
using namespace intercept::types;

constexpr uint32_t NET_MSG_MAGIC = 0x4E48484B; // "KHHN"
constexpr uint32_t NET_MSG_VERSION = 1;
constexpr uint32_t NET_COALESCED_MAGIC = 0x434E484B; // "KHNC" - KH Network Coalesced
constexpr size_t NET_DEFAULT_COALESCE_MAX_SIZE = 65536;      // 64KB max coalesced packet
constexpr size_t NET_DEFAULT_COALESCE_MAX_MESSAGES = 128;    // Max messages per coalesced packet
constexpr int NET_DEFAULT_COALESCE_DELAY_US = 3000;           // Microseconds to wait for more messages
constexpr size_t NET_COMPRESSION_THRESHOLD = 256;   // Only compress payloads > 256 bytes
constexpr uint8_t NET_FLAG_NONE = 0x00;
constexpr uint8_t NET_FLAG_COMPRESSED = 0x01;
constexpr int NET_DEFAULT_PORT = 21337;
constexpr size_t NET_DEFAULT_MAX_MESSAGE_SIZE = 16 * 1024 * 1024; // 16MB
constexpr size_t NET_DEFAULT_RECV_BUFFER_SIZE = 262144; // 256KB
constexpr size_t NET_DEFAULT_SEND_BUFFER_SIZE = 262144; // 256KB
constexpr int NET_DEFAULT_CONNECT_TIMEOUT_MS = 5000;
constexpr int NET_DEFAULT_SEND_TIMEOUT_MS = 3000;
constexpr int NET_DEFAULT_RECV_TIMEOUT_MS = 3000;
constexpr int NET_DEFAULT_CLIENT_STALL_TIMEOUT_MS = 10000;
constexpr int NET_DEFAULT_KEEPALIVE_TIME_MS = 15000;
constexpr int NET_DEFAULT_KEEPALIVE_INTERVAL_MS = 1000;
constexpr int NET_DEFAULT_SEND_BATCH_SIZE = 64;
constexpr const char* NET_INTERNAL_ROUTE_EVENT = "_KH_INTERNAL_ROUTE_";
constexpr const char* NET_INTERNAL_CONDITIONAL_EVENT = "_KH_INTERNAL_COND_";

enum class NetworkTargetType : uint8_t {
    CLIENT_ID = 0,           // Specific client owner ID (positive)
    CLIENT_ID_EXCLUDE = 1,   // Everyone except this client (negative ID)
    LOCAL_ONLY = 2,          // Local execution only (bool true)
    DO_NOTHING = 3,          // No execution (bool false)
    OBJECT_OWNER = 4,        // Send to owner of object
    GROUP_MEMBERS = 5,       // Send to all group member owners  
    TEAM_MEMBER_OWNER = 6,   // Send to team member's agent owner
    SIDE_MEMBERS = 7,        // Send to all players of a side
    LOCATION_UNITS = 8,      // Send to all players in location
    STRING_SERVER = 9,       // "SERVER"
    STRING_GLOBAL = 10,      // "GLOBAL"
    STRING_LOCAL = 11,       // "LOCAL"
    STRING_PLAYERS = 12,     // "PLAYERS"
    STRING_REMOTE = 13,      // "REMOTE"
    STRING_ADMIN = 14,       // "ADMIN"
    STRING_HEADLESS = 15,    // "HEADLESS"
    STRING_CURATORS = 16,    // "CURATORS"
    CODE_CONDITION = 17,     // CODE
    ARRAY_TARGETS = 18,      // Array of mixed target types
    STRING_EXTENDED = 19     // Specific string type
};

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

struct LocalMessage {
    std::string event_name;
    game_value message;
    int sender_client_id;
    
    LocalMessage() : sender_client_id(-1) {}
    LocalMessage(const std::string& name, const game_value& msg, int sender = -1)
        : event_name(name), message(msg), sender_client_id(sender) {}
};

struct JipMessage {
    std::string event_name;
    std::vector<uint8_t> payload;
    int original_sender;
    std::string dependency_net_id;  // Empty = no dependency, otherwise netId of object/group
    bool dependency_is_group;       // true = group, false = object
};

struct OutgoingMessage {
    std::shared_ptr<std::vector<uint8_t>> payload;
    std::string event_name;
    int target_client;
    int sender_client;
    OutgoingMessage() : target_client(-1), sender_client(-1) {}
    
    OutgoingMessage(int target, int sender, std::string&& event, std::shared_ptr<std::vector<uint8_t>> data)
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

template<typename T>
class LockFreeMPSCQueue {
private:
    struct Node {
        std::atomic<Node*> next{nullptr};
        T data;
        Node() = default;
        explicit Node(T&& item) : data(std::move(item)) {}
    };
    
    alignas(64) std::atomic<Node*> head_;
    alignas(64) std::atomic<Node*> tail_;
    alignas(64) std::atomic<size_t> size_{0};
    
public:
    LockFreeMPSCQueue() {
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }
    
    ~LockFreeMPSCQueue() {
        T item;
        while (try_pop(item)) {}
        delete head_.load(std::memory_order_relaxed);
    }
    
    LockFreeMPSCQueue(const LockFreeMPSCQueue&) = delete;
    LockFreeMPSCQueue& operator=(const LockFreeMPSCQueue&) = delete;
    
    void push(T&& item) {
        Node* new_node = new Node(std::move(item));
        Node* prev_tail = tail_.exchange(new_node, std::memory_order_acq_rel);
        prev_tail->next.store(new_node, std::memory_order_release);
        size_.fetch_add(1, std::memory_order_relaxed);
    }
    
    bool try_pop(T& item) {
        Node* head = head_.load(std::memory_order_relaxed);
        Node* next = head->next.load(std::memory_order_acquire);
        
        if (next == nullptr) {
            return false;
        }
        
        item = std::move(next->data);
        head_.store(next, std::memory_order_release);
        delete head;
        size_.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }
    
    // Bulk pop for efficiency - pops up to max_count items
    size_t pop_bulk(std::vector<T>& out, size_t max_count) {
        size_t count = 0;
        T item;
        
        while (count < max_count && try_pop(item)) {
            out.push_back(std::move(item));
            ++count;
        }
        
        return count;
    }
    
    bool empty() const {
        Node* head = head_.load(std::memory_order_relaxed);
        return head->next.load(std::memory_order_acquire) == nullptr;
    }
    
    size_t size_approx() const {
        return size_.load(std::memory_order_relaxed);
    }
};

class PayloadPool : public std::enable_shared_from_this<PayloadPool> {
private:
    std::vector<std::vector<uint8_t>*> pool_;
    std::mutex mutex_;
    size_t max_size_;
    
public:
    PayloadPool(size_t max_size = 256) : max_size_(max_size) {}
    
    ~PayloadPool() {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto* p : pool_) {
            delete p;
        }
    }
    
    PayloadPool(const PayloadPool&) = delete;
    PayloadPool& operator=(const PayloadPool&) = delete;
    
    std::shared_ptr<std::vector<uint8_t>> acquire() {
        std::vector<uint8_t>* raw = nullptr;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!pool_.empty()) {
                raw = pool_.back();
                pool_.pop_back();
            }
        }
        
        if (raw) {
            raw->clear();
        } else {
            raw = new std::vector<uint8_t>();
            raw->reserve(4096);
        }
        
        std::weak_ptr<PayloadPool> weak_self = shared_from_this();

        return std::shared_ptr<std::vector<uint8_t>>(raw, [weak_self](std::vector<uint8_t>* p) {
            if (auto self = weak_self.lock()) {
                std::lock_guard<std::mutex> lock(self->mutex_);

                if (self->pool_.size() < self->max_size_) {
                    self->pool_.push_back(p);
                    return;
                }
            }
            
            delete p;
        });
    }
};

using HandlerList = std::vector<NetworkMessageHandler>;
using HandlerListPtr = std::shared_ptr<const HandlerList>;

class COWHandlerMap {
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, HandlerListPtr> handlers_;
    std::unordered_map<int, std::string> handler_id_to_event_;
    
public:
    HandlerListPtr get_handlers(const std::string& event_name) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = handlers_.find(event_name);

        if (it != handlers_.end()) {
            return it->second;
        }

        return nullptr;
    }
    
    int add_handler(const std::string& event_name, const code& handler, const game_value& args) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        int handler_id = static_cast<int>(g_network_handler_id_counter.fetch_add(1) & 0x7FFFFFFF);
        
        // Copy-on-write: create new list with added handler
        auto it = handlers_.find(event_name);
        HandlerListPtr old_list = (it != handlers_.end()) ? it->second : nullptr;
        auto new_list = std::make_shared<HandlerList>();
        
        if (old_list) {
            *new_list = *old_list;  // Copy existing handlers
        }
        
        new_list->push_back({handler_id, event_name, handler, args});
        handlers_[event_name] = new_list;
        handler_id_to_event_[handler_id] = event_name;
        return handler_id;
    }
    
    bool remove_handler(int handler_id) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto id_it = handler_id_to_event_.find(handler_id);

        if (id_it == handler_id_to_event_.end()) {
            return false;
        }
        
        std::string event_name = id_it->second;
        handler_id_to_event_.erase(id_it);
        auto handlers_it = handlers_.find(event_name);

        if (handlers_it != handlers_.end() && handlers_it->second) {
            // Copy-on-write: create new list without the removed handler
            auto new_list = std::make_shared<HandlerList>();
            
            for (const auto& h : *handlers_it->second) {
                if (h.handler_id != handler_id) {
                    new_list->push_back(h);
                }
            }
            
            if (new_list->empty()) {
                handlers_.erase(handlers_it);
            } else {
                handlers_[event_name] = new_list;
            }
        }
        
        return true;
    }
    
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        handlers_.clear();
        handler_id_to_event_.clear();
    }
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
    COWHandlerMap cow_handlers_;
    std::unordered_map<int, std::deque<OutgoingMessage>> per_client_queues_;
    std::unordered_map<int, int64_t> client_stall_start_times_;
    std::shared_ptr<PayloadPool> payload_pool_;
    std::atomic<bool> warned_fd_limit_{false};
    size_t config_max_message_size_{NET_DEFAULT_MAX_MESSAGE_SIZE};
    size_t config_recv_buffer_size_{NET_DEFAULT_RECV_BUFFER_SIZE};
    size_t config_send_buffer_size_{NET_DEFAULT_SEND_BUFFER_SIZE};
    int config_connect_timeout_ms_{NET_DEFAULT_CONNECT_TIMEOUT_MS};
    int config_send_timeout_ms_{NET_DEFAULT_SEND_TIMEOUT_MS};
    int config_recv_timeout_ms_{NET_DEFAULT_RECV_TIMEOUT_MS};
    int config_client_stall_timeout_ms_{NET_DEFAULT_CLIENT_STALL_TIMEOUT_MS};
    int config_keepalive_time_ms_{NET_DEFAULT_KEEPALIVE_TIME_MS};
    int config_keepalive_interval_ms_{NET_DEFAULT_KEEPALIVE_INTERVAL_MS};
    int config_send_batch_size_{NET_DEFAULT_SEND_BATCH_SIZE};
    bool config_compression_enabled_{true};
    bool config_coalesce_enabled_{true};
    size_t config_coalesce_max_size_{NET_DEFAULT_COALESCE_MAX_SIZE};
    size_t config_coalesce_max_messages_{NET_DEFAULT_COALESCE_MAX_MESSAGES};
    int config_coalesce_delay_us_{NET_DEFAULT_COALESCE_DELAY_US};

    // Incoming message queue
    std::deque<NetworkMessage> incoming_queue_;
    std::mutex incoming_mutex_;
    std::deque<LocalMessage> local_incoming_queue_;
    std::mutex local_incoming_mutex_;
        
    // Lock-free outgoing message queue
    LockFreeMPSCQueue<OutgoingMessage> outgoing_queue_lockfree_;
    std::atomic<bool> outgoing_has_data_{false};
    
    // Network threads
    std::thread listen_thread_;
    std::thread send_thread_;
    std::thread receive_thread_;
    
    // Jip
    std::unordered_map<std::string, JipMessage> jip_messages_;
    std::mutex jip_mutex_;

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

    static std::vector<uint8_t> compress_payload(const std::vector<uint8_t>& input) {
        if (input.empty()) {
            return {};
        }
        
        int max_compressed_size = LZ4_compressBound(static_cast<int>(input.size()));

        if (max_compressed_size <= 0) {
            return {};
        }

        std::vector<uint8_t> compressed(4 + static_cast<size_t>(max_compressed_size));
        uint32_t original_size = static_cast<uint32_t>(input.size());
        compressed[0] = static_cast<uint8_t>(original_size & 0xFF);
        compressed[1] = static_cast<uint8_t>((original_size >> 8) & 0xFF);
        compressed[2] = static_cast<uint8_t>((original_size >> 16) & 0xFF);
        compressed[3] = static_cast<uint8_t>((original_size >> 24) & 0xFF);
        
        int compressed_size = LZ4_compress_default(
            reinterpret_cast<const char*>(input.data()),
            reinterpret_cast<char*>(compressed.data() + 4),
            static_cast<int>(input.size()),
            max_compressed_size
        );
        
        if (compressed_size <= 0) {
            return {};
        }
        
        // Only use compression if it actually saves space
        size_t total_compressed_size = 4 + static_cast<size_t>(compressed_size);
        
        if (total_compressed_size >= input.size()) {
            return {};
        }
        
        compressed.resize(total_compressed_size);
        return compressed;
    }

    std::vector<uint8_t> decompress_payload(const uint8_t* data, size_t data_size) {
        if (data_size < 4) {
            return {};
        }
        
        // Read original size (little-endian)
        uint32_t original_size = static_cast<uint32_t>(data[0]) |
                                (static_cast<uint32_t>(data[1]) << 8) |
                                (static_cast<uint32_t>(data[2]) << 16) |
                                (static_cast<uint32_t>(data[3]) << 24);
        
        if (original_size > config_max_message_size_) {
            return {};
        }
        
        std::vector<uint8_t> decompressed(original_size);
        
        int decompressed_size = LZ4_decompress_safe(
            reinterpret_cast<const char*>(data + 4),
            reinterpret_cast<char*>(decompressed.data()),
            static_cast<int>(data_size - 4),
            static_cast<int>(original_size)
        );
        
        if (decompressed_size < 0 || static_cast<uint32_t>(decompressed_size) != original_size) {
            return {};
        }
        
        return decompressed;
    }
    
    // Coalesced packet format:
    // [COALESCED_MAGIC(4)][VERSION(4)][MESSAGE_COUNT(2)]
    // For each message:
    //   [FLAGS(1)][TARGET(4)][SENDER(4)][EVENT_LEN(2)][EVENT][PAYLOAD_LEN(4)][PAYLOAD]
    // If FLAGS has NET_FLAG_COMPRESSED, PAYLOAD = [ORIGINAL_SIZE(4)][COMPRESSED_DATA]
    
    static void write_uint16(std::vector<uint8_t>& buffer, uint16_t value) {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }
    
    static uint16_t read_uint16(const uint8_t* data, size_t& offset, size_t max_size) {
        if (offset + 2 > max_size) {
            throw std::runtime_error("Buffer underrun reading uint16");
        }
        
        uint16_t value = static_cast<uint16_t>(data[offset]) |
                        (static_cast<uint16_t>(data[offset + 1]) << 8);

        offset += 2;
        return value;
    }

    // Creates a coalesced packet from multiple OutgoingMessages
    // Returns empty vector if coalescing isn't beneficial (single small message)
    void create_coalesced_packet(
        std::vector<uint8_t>& output,
        const std::vector<OutgoingMessage>& messages,
        std::vector<uint8_t>& temp_uncompressed_buffer
    ) {
        if (messages.empty()) {
            output.clear();
            return;
        }
        
        // For single message, use regular packet format (no coalescing overhead)
        if (messages.size() == 1) {
            create_network_packet_into(output, messages[0].target_client, messages[0].sender_client,
                                       messages[0].event_name, *messages[0].payload);

            return;
        }
        
        // Build uncompressed coalesced content first
        // Format: [MESSAGE_COUNT(2)] + for each: [TARGET(4)][SENDER(4)][EVENT_LEN(2)][EVENT][PAYLOAD_LEN(4)][PAYLOAD]
        temp_uncompressed_buffer.clear();
        
        // Estimate size for reservation
        size_t estimated_size = 2;  // message count

        for (const auto& msg : messages) {
            estimated_size += 14 + msg.event_name.size() + msg.payload->size();
        }

        temp_uncompressed_buffer.reserve(estimated_size);
        
        // Write message count
        write_uint16(temp_uncompressed_buffer, static_cast<uint16_t>(messages.size()));
        
        // Write each message (without per-message compression)
        for (const auto& msg : messages) {
            write_uint32(temp_uncompressed_buffer, static_cast<uint32_t>(msg.target_client));
            write_uint32(temp_uncompressed_buffer, static_cast<uint32_t>(msg.sender_client));
            write_uint16(temp_uncompressed_buffer, static_cast<uint16_t>(msg.event_name.size()));
            
            temp_uncompressed_buffer.insert(temp_uncompressed_buffer.end(), 
                                            msg.event_name.begin(), msg.event_name.end());

            write_uint32(temp_uncompressed_buffer, static_cast<uint32_t>(msg.payload->size()));

            temp_uncompressed_buffer.insert(temp_uncompressed_buffer.end(), 
                                            msg.payload->begin(), msg.payload->end());
        }
        
        // Now build final packet with optional whole-packet compression
        output.clear();
        
        bool should_compress = config_compression_enabled_ && 
                               temp_uncompressed_buffer.size() > NET_COMPRESSION_THRESHOLD;
        
        std::vector<uint8_t> compressed_data;
        const std::vector<uint8_t>* final_payload = &temp_uncompressed_buffer;
        uint8_t flags = NET_FLAG_NONE;
        
        if (should_compress) {
            compressed_data = compress_payload(temp_uncompressed_buffer);

            if (!compressed_data.empty()) {
                final_payload = &compressed_data;
                flags |= NET_FLAG_COMPRESSED;
            }
        }
        
        // Header: [COALESCED_MAGIC(4)][VERSION(4)][FLAGS(1)][PAYLOAD_LEN(4)][PAYLOAD]
        output.reserve(13 + final_payload->size());
        write_uint32(output, NET_COALESCED_MAGIC);
        write_uint32(output, NET_MSG_VERSION);
        output.push_back(flags);
        write_uint32(output, static_cast<uint32_t>(final_payload->size()));
        output.insert(output.end(), final_payload->begin(), final_payload->end());
    }
    
    // Parses either a coalesced packet or a single packet
    bool parse_coalesced_or_single_packet(
        const std::vector<uint8_t>& data,
        std::vector<NetworkMessage>& out_messages,
        std::vector<OutgoingMessage>& out_forward_messages
    ) {
        if (data.size() < 4) return false;
        size_t offset = 0;
        uint32_t magic = read_uint32(data.data(), offset, data.size());
        
        if (magic == NET_COALESCED_MAGIC) {
            // We compress the coalesced packet
            if (data.size() < 13) return false;
            uint32_t version = read_uint32(data.data(), offset, data.size());
            if (version > NET_MSG_VERSION) return false;
            uint8_t flags = data[offset++];
            uint32_t payload_len = read_uint32(data.data(), offset, data.size());
            if (offset + payload_len > data.size()) return false;
            
            // Decompress if needed
            const uint8_t* payload_data;
            std::vector<uint8_t> decompressed_payload;
            size_t payload_size;
            
            if (flags & NET_FLAG_COMPRESSED) {
                decompressed_payload = decompress_payload(data.data() + offset, payload_len);
                if (decompressed_payload.empty() && payload_len > 4) return false;
                payload_data = decompressed_payload.data();
                payload_size = decompressed_payload.size();
            } else {
                payload_data = data.data() + offset;
                payload_size = payload_len;
            }
            
            // Parse the decompressed coalesced content
            size_t inner_offset = 0;
            if (payload_size < 2) return false;
            uint16_t message_count = read_uint16(payload_data, inner_offset, payload_size);
            if (message_count == 0 || message_count > config_coalesce_max_messages_) return false;
            out_messages.reserve(out_messages.size() + message_count);
            out_forward_messages.reserve(out_forward_messages.size() + message_count);
            
            for (uint16_t i = 0; i < message_count; ++i) {
                if (inner_offset + 8 > payload_size) return false;
                int target_client = static_cast<int>(read_uint32(payload_data, inner_offset, payload_size));
                int sender_client = static_cast<int>(read_uint32(payload_data, inner_offset, payload_size));
                if (inner_offset + 2 > payload_size) return false;
                uint16_t event_len = read_uint16(payload_data, inner_offset, payload_size);
                if (inner_offset + event_len > payload_size) return false;
                std::string event_name(reinterpret_cast<const char*>(payload_data + inner_offset), event_len);
                inner_offset += event_len;
                if (inner_offset + 4 > payload_size) return false;
                uint32_t msg_payload_len = read_uint32(payload_data, inner_offset, payload_size);
                if (inner_offset + msg_payload_len > payload_size) return false;

                std::vector<uint8_t> payload(payload_data + inner_offset, 
                                              payload_data + inner_offset + msg_payload_len);

                inner_offset += msg_payload_len;
                
                if (target_client == 2) {
                    out_messages.emplace_back(event_name, std::move(payload), sender_client);
                } else {
                    out_forward_messages.emplace_back(
                        target_client, sender_client, std::string(event_name),
                        std::make_shared<std::vector<uint8_t>>(std::move(payload))
                    );
                }
            }
            
            return true;
        } else if (magic == NET_MSG_MAGIC) {
            // Single packet - use existing parser
            offset = 0;
            int target_client, sender_client;
            std::string event_name;
            std::vector<uint8_t> payload;
            
            if (!parse_network_packet(data, target_client, sender_client, event_name, payload)) {
                return false;
            }
            
            if (target_client == 2) {
                out_messages.emplace_back(event_name, std::move(payload), sender_client);
            } else {
                out_forward_messages.emplace_back(
                    target_client, sender_client, std::move(event_name),
                    std::make_shared<std::vector<uint8_t>>(std::move(payload))
                );
            }
            
            return true;
        }
        
        return false;
    }
    
    // Client-side: parse incoming coalesced packet from server
    bool parse_incoming_coalesced_packet(
        const std::vector<uint8_t>& data,
        std::vector<NetworkMessage>& out_messages
    ) {
        if (data.size() < 4) return false;
        size_t offset = 0;
        uint32_t magic = read_uint32(data.data(), offset, data.size());
        
        if (magic == NET_COALESCED_MAGIC) {
            if (data.size() < 13) return false;
            uint32_t version = read_uint32(data.data(), offset, data.size());
            if (version > NET_MSG_VERSION) return false;
            uint8_t flags = data[offset++];
            uint32_t payload_len = read_uint32(data.data(), offset, data.size());
            if (offset + payload_len > data.size()) return false;
            
            // Decompress if needed
            const uint8_t* payload_data;
            std::vector<uint8_t> decompressed_payload;
            size_t payload_size;
            
            if (flags & NET_FLAG_COMPRESSED) {
                decompressed_payload = decompress_payload(data.data() + offset, payload_len);
                if (decompressed_payload.empty() && payload_len > 4) return false;
                payload_data = decompressed_payload.data();
                payload_size = decompressed_payload.size();
            } else {
                payload_data = data.data() + offset;
                payload_size = payload_len;
            }
            
            // Parse the decompressed coalesced content
            size_t inner_offset = 0;
            if (payload_size < 2) return false;
            uint16_t message_count = read_uint16(payload_data, inner_offset, payload_size);
            if (message_count == 0 || message_count > config_coalesce_max_messages_) return false;
            out_messages.reserve(out_messages.size() + message_count);
            
            for (uint16_t i = 0; i < message_count; ++i) {
                if (inner_offset + 8 > payload_size) return false;
                int target_client = static_cast<int>(read_uint32(payload_data, inner_offset, payload_size));
                int sender_client = static_cast<int>(read_uint32(payload_data, inner_offset, payload_size));
                (void)target_client;  // Client doesn't need this
                if (inner_offset + 2 > payload_size) return false;
                uint16_t event_len = read_uint16(payload_data, inner_offset, payload_size);
                if (inner_offset + event_len > payload_size) return false;
                std::string event_name(reinterpret_cast<const char*>(payload_data + inner_offset), event_len);
                inner_offset += event_len;
                if (inner_offset + 4 > payload_size) return false;
                uint32_t msg_payload_len = read_uint32(payload_data, inner_offset, payload_size);
                if (inner_offset + msg_payload_len > payload_size) return false;

                std::vector<uint8_t> payload(payload_data + inner_offset,
                                              payload_data + inner_offset + msg_payload_len);

                inner_offset += msg_payload_len;
                out_messages.emplace_back(event_name, std::move(payload), sender_client);
            }
            
            return true;
        } else if (magic == NET_MSG_MAGIC) {
            offset = 0;
            int target_client, sender_client;
            std::string event_name;
            std::vector<uint8_t> payload;
            
            if (!parse_network_packet(data, target_client, sender_client, event_name, payload)) {
                return false;
            }
            
            out_messages.emplace_back(event_name, std::move(payload), sender_client);
            return true;
        }
        
        return false;
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
                    int idd;
                    
                    try {
                        idd = std::stoi(serialized);
                    } catch (...) {
                        return sqf::display_null();
                    }
                    
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
    
    static std::vector<int> get_unique_client_ids(const std::vector<int>& ids) {
        std::vector<int> unique_ids;
        std::unordered_set<int> seen;
        
        for (int id : ids) {
            if (seen.find(id) == seen.end()) {
                seen.insert(id);
                unique_ids.push_back(id);
            }
        }
        
        return unique_ids;
    }

    // Resolve targets when we are the server
    // Returns vector of client IDs to send to, empty means no send needed
    // Sets local_exec to true if local execution should happen
    std::vector<int> resolve_targets_server(
        NetworkTargetType target_type,
        const game_value& target_data,
        int sender_client_id,
        bool& local_exec
    ) {
        std::vector<int> targets;
        local_exec = false;
        int our_client_id = cached_client_id_.load();
        
        switch (target_type) {
            case NetworkTargetType::CLIENT_ID: {
                int target_id = static_cast<int>(static_cast<float>(target_data));
                
                if (target_id == our_client_id || target_id == 2) {
                    local_exec = true;
                } else {
                    targets.push_back(target_id);
                }
                
                break;
            }
            
            case NetworkTargetType::CLIENT_ID_EXCLUDE: {
                int exclude_id = static_cast<int>(static_cast<float>(target_data));
                
                if (exclude_id < 0) {
                    exclude_id = -exclude_id;
                }
                
                // Get all connected clients
                {
                    std::lock_guard<std::mutex> lock(connections_mutex_);
                    
                    for (const auto& pair : client_connections_) {
                        if (pair.first != exclude_id) {
                            targets.push_back(pair.first);
                        }
                    }
                }
                
                // Include ourselves if we're not excluded
                if (our_client_id != exclude_id) {
                    local_exec = true;
                }
                
                break;
            }
            
            case NetworkTargetType::LOCAL_ONLY: {
                local_exec = true;
                break;
            }
            
            case NetworkTargetType::DO_NOTHING: {
                // No action
                break;
            }
            
            case NetworkTargetType::OBJECT_OWNER: {
                object obj = static_cast<object>(target_data);
                
                if (sqf::is_null(obj)) {
                    break;
                }
                
                int owner_id = static_cast<int>(sqf::owner(obj));
                
                if (owner_id == our_client_id || owner_id == 2) {
                    local_exec = true;
                } else {
                    targets.push_back(owner_id);
                }
                
                break;
            }
            
            case NetworkTargetType::GROUP_MEMBERS: {
                group grp = static_cast<group>(target_data);
                
                if (sqf::is_null(grp)) {
                    break;
                }
                
                auto units = sqf::units(grp);
                std::vector<int> owner_ids;
                
                for (const auto& unit : units) {
                    int owner_id = static_cast<int>(sqf::owner(unit));
                    owner_ids.push_back(owner_id);
                }
                
                auto unique_ids = get_unique_client_ids(owner_ids);
                
                for (int id : unique_ids) {
                    if (id == our_client_id || id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(id);
                    }
                }
                
                break;
            }
            
            case NetworkTargetType::TEAM_MEMBER_OWNER: {
                game_value agent_obj = sqf::agent(target_data);
                object obj = static_cast<object>(agent_obj);
                
                if (sqf::is_null(obj)) {
                    break;
                }
                
                int owner_id = static_cast<int>(sqf::owner(obj));
                
                if (owner_id == our_client_id || owner_id == 2) {
                    local_exec = true;
                } else {
                    targets.push_back(owner_id);
                }
                
                break;
            }
            
            case NetworkTargetType::SIDE_MEMBERS: {
                side target_side = static_cast<side>(target_data);
                game_value all_players_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits", game_value());
                
                if (all_players_gv.is_nil() || all_players_gv.type_enum() != game_data_type::ARRAY) {
                    break;
                }
                
                auto& all_players = all_players_gv.to_array();
                std::vector<int> owner_ids;
                
                for (const auto& player_gv : all_players) {
                    object player_unit = static_cast<object>(player_gv);
                    
                    if (sqf::is_null(player_unit)) {
                        continue;
                    }
                    
                    group player_group = sqf::get_group(player_unit);
                    side player_side = sqf::get_side(player_group);
                    
                    if (player_side == target_side) {
                        int owner_id = static_cast<int>(sqf::owner(player_unit));
                        owner_ids.push_back(owner_id);
                    }
                }
                
                auto unique_ids = get_unique_client_ids(owner_ids);
                
                for (int id : unique_ids) {
                    if (id == our_client_id || id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(id);
                    }
                }
                
                break;
            }
            
            case NetworkTargetType::LOCATION_UNITS: {
                location loc = static_cast<location>(target_data);
                
                if (sqf::is_null(loc)) {
                    break;
                }
                
                game_value all_players_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits", game_value());
                
                if (all_players_gv.is_nil() || all_players_gv.type_enum() != game_data_type::ARRAY) {
                    break;
                }
                
                auto& all_players = all_players_gv.to_array();
                std::vector<int> owner_ids;
                
                for (const auto& player_gv : all_players) {
                    object player_unit = static_cast<object>(player_gv);
                    
                    if (sqf::is_null(player_unit)) {
                        continue;
                    }
                    
                    if (sqf::in_area(player_unit, loc)) {
                        int owner_id = static_cast<int>(sqf::owner(player_unit));
                        owner_ids.push_back(owner_id);
                    }
                }
                
                auto unique_ids = get_unique_client_ids(owner_ids);
                
                for (int id : unique_ids) {
                    if (id == our_client_id || id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(id);
                    }
                }
                
                break;
            }
            
            case NetworkTargetType::STRING_SERVER: {
                if (our_client_id == 2) {
                    local_exec = true;
                } else {
                    targets.push_back(2);
                }
                
                break;
            }
            
            case NetworkTargetType::STRING_GLOBAL: {
                // Everyone including local
                local_exec = true;
                
                {
                    std::lock_guard<std::mutex> lock(connections_mutex_);
                    
                    for (const auto& pair : client_connections_) {
                        targets.push_back(pair.first);
                    }
                }
                
                break;
            }
            
            case NetworkTargetType::STRING_LOCAL: {
                local_exec = true;
                break;
            }
            
            case NetworkTargetType::STRING_PLAYERS: {
                game_value all_machines_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayermachines", game_value());
                
                if (all_machines_gv.is_nil() || all_machines_gv.type_enum() != game_data_type::ARRAY) {
                    break;
                }
                
                auto& all_machines = all_machines_gv.to_array();
                
                for (const auto& machine_gv : all_machines) {
                    int client_id = static_cast<int>(static_cast<float>(machine_gv));
                    
                    if (client_id == our_client_id || client_id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(client_id);
                    }
                }
                
                break;
            }
            
            case NetworkTargetType::STRING_REMOTE: {
                // Everyone except sender
                {
                    std::lock_guard<std::mutex> lock(connections_mutex_);
                    
                    for (const auto& pair : client_connections_) {
                        if (pair.first != sender_client_id) {
                            targets.push_back(pair.first);
                        }
                    }
                }
                
                // Include server if sender is not server
                if (sender_client_id != our_client_id && sender_client_id != 2) {
                    local_exec = true;
                }
                
                break;
            }
            
            case NetworkTargetType::STRING_ADMIN: {
                game_value admin_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_adminmachine", game_value());
                
                if (admin_gv.is_nil()) {
                    break;
                }
                
                int admin_id = static_cast<int>(static_cast<float>(admin_gv));
                
                if (admin_id == our_client_id || admin_id == 2) {
                    local_exec = true;
                } else {
                    targets.push_back(admin_id);
                }
                
                break;
            }
            
            case NetworkTargetType::STRING_HEADLESS: {
                game_value headless_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allheadlessmachines", game_value());
                
                if (headless_gv.is_nil() || headless_gv.type_enum() != game_data_type::ARRAY) {
                    break;
                }
                
                auto& headless_machines = headless_gv.to_array();
                
                for (const auto& machine_gv : headless_machines) {
                    int client_id = static_cast<int>(static_cast<float>(machine_gv));
                    
                    if (client_id == our_client_id || client_id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(client_id);
                    }
                }
                
                break;
            }
            
            case NetworkTargetType::STRING_CURATORS: {
                game_value all_players_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits", game_value());
                
                if (all_players_gv.is_nil() || all_players_gv.type_enum() != game_data_type::ARRAY) {
                    break;
                }
                
                auto& all_players = all_players_gv.to_array();
                std::vector<int> owner_ids;
                
                for (const auto& player_gv : all_players) {
                    object player_unit = static_cast<object>(player_gv);
                    
                    if (sqf::is_null(player_unit)) {
                        continue;
                    }
                    
                    game_value curator_logic = sqf::get_assigned_curator_logic(player_unit);
                    
                    if (!curator_logic.is_nil() && !sqf::is_null(static_cast<object>(curator_logic))) {
                        int owner_id = static_cast<int>(sqf::owner(player_unit));
                        owner_ids.push_back(owner_id);
                    }
                }
                
                auto unique_ids = get_unique_client_ids(owner_ids);
                
                for (int id : unique_ids) {
                    if (id == our_client_id || id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(id);
                    }
                }
                                
                break;
            }

            case NetworkTargetType::STRING_EXTENDED: {
                std::string target_str = static_cast<std::string>(target_data);
                targets = resolve_string_target_extended(target_str, sender_client_id, local_exec);
                break;
            }

            case NetworkTargetType::CODE_CONDITION: {
                // Same as GLOBAL - send to everyone, condition checked on receive
                local_exec = true;
                
                {
                    std::lock_guard<std::mutex> lock(connections_mutex_);
                    
                    for (const auto& pair : client_connections_) {
                        targets.push_back(pair.first);
                    }
                }
                
                break;
            }

            case NetworkTargetType::ARRAY_TARGETS: {
                targets = resolve_array_targets(target_data, sender_client_id, local_exec);
                break;
            }
                        
            default:
                break;
        }
        
        return targets;
    }

    void resolve_single_target(
        const game_value& target,
        int sender_client_id,
        std::vector<int>& out_targets,
        bool& out_local_exec
    ) {
        if (target.is_nil()) {
            return;
        }
        
        auto type = target.type_enum();
        
        switch (type) {
            case game_data_type::SCALAR: {
                int client_id = static_cast<int>(static_cast<float>(target));
                
                if (client_id < 0) {
                    // Exclude mode
                    bool local_exec = false;
                    auto targets = resolve_targets_server(NetworkTargetType::CLIENT_ID_EXCLUDE, target, sender_client_id, local_exec);
                    out_targets.insert(out_targets.end(), targets.begin(), targets.end());
                    if (local_exec) out_local_exec = true;
                } else {
                    int our_client_id = cached_client_id_.load();
                    
                    if (client_id == our_client_id || client_id == 2) {
                        out_local_exec = true;
                    } else {
                        out_targets.push_back(client_id);
                    }
                }
                break;
            }
            
            case game_data_type::BOOL: {
                bool val = static_cast<bool>(target);

                if (val) {
                    out_local_exec = true;
                }

                // false = do nothing
                break;
            }
            
            case game_data_type::OBJECT: {
                bool local_exec = false;
                auto targets = resolve_targets_server(NetworkTargetType::OBJECT_OWNER, target, sender_client_id, local_exec);
                out_targets.insert(out_targets.end(), targets.begin(), targets.end());
                if (local_exec) out_local_exec = true;
                break;
            }
            
            case game_data_type::GROUP: {
                bool local_exec = false;
                auto targets = resolve_targets_server(NetworkTargetType::GROUP_MEMBERS, target, sender_client_id, local_exec);
                out_targets.insert(out_targets.end(), targets.begin(), targets.end());
                if (local_exec) out_local_exec = true;
                break;
            }
            
            case game_data_type::TEAM_MEMBER: {
                bool local_exec = false;
                auto targets = resolve_targets_server(NetworkTargetType::TEAM_MEMBER_OWNER, target, sender_client_id, local_exec);
                out_targets.insert(out_targets.end(), targets.begin(), targets.end());
                if (local_exec) out_local_exec = true;
                break;
            }
            
            case game_data_type::SIDE: {
                bool local_exec = false;
                auto targets = resolve_targets_server(NetworkTargetType::SIDE_MEMBERS, target, sender_client_id, local_exec);
                out_targets.insert(out_targets.end(), targets.begin(), targets.end());
                if (local_exec) out_local_exec = true;
                break;
            }
            
            case game_data_type::LOCATION: {
                bool local_exec = false;
                auto targets = resolve_targets_server(NetworkTargetType::LOCATION_UNITS, target, sender_client_id, local_exec);
                out_targets.insert(out_targets.end(), targets.begin(), targets.end());
                if (local_exec) out_local_exec = true;
                break;
            }
            
            case game_data_type::STRING: {
                std::string target_str = static_cast<std::string>(target);
                if (target_str.empty()) break;
                std::string target_upper = target_str;
                std::transform(target_upper.begin(), target_upper.end(), target_upper.begin(), ::toupper);
                NetworkTargetType str_type;
                bool use_extended = false;
                if (target_upper == "SERVER") str_type = NetworkTargetType::STRING_SERVER;
                else if (target_upper == "GLOBAL") str_type = NetworkTargetType::STRING_GLOBAL;
                else if (target_upper == "LOCAL") str_type = NetworkTargetType::STRING_LOCAL;
                else if (target_upper == "PLAYERS") str_type = NetworkTargetType::STRING_PLAYERS;
                else if (target_upper == "REMOTE") str_type = NetworkTargetType::STRING_REMOTE;
                else if (target_upper == "ADMIN") str_type = NetworkTargetType::STRING_ADMIN;
                else if (target_upper == "HEADLESS") str_type = NetworkTargetType::STRING_HEADLESS;
                else if (target_upper == "CURATORS") str_type = NetworkTargetType::STRING_CURATORS;
                else {
                    use_extended = true;
                }
                
                if (use_extended) {
                    // Use extended string lookup
                    bool ext_local = false;
                    auto ext_targets = resolve_string_target_extended(target_str, sender_client_id, ext_local);
                    out_targets.insert(out_targets.end(), ext_targets.begin(), ext_targets.end());
                    if (ext_local) out_local_exec = true;
                } else {
                    bool local_exec = false;
                    auto targets = resolve_targets_server(str_type, game_value(), sender_client_id, local_exec);
                    out_targets.insert(out_targets.end(), targets.begin(), targets.end());
                    if (local_exec) out_local_exec = true;
                }
                
                break;
            }
                        
            case game_data_type::ARRAY: {
                // Recursive case
                auto& arr = target.to_array();

                for (const auto& elem : arr) {
                    resolve_single_target(elem, sender_client_id, out_targets, out_local_exec);
                }

                break;
            }
            
            default:
                break;
        }
    }

    std::vector<int> resolve_array_targets(
        const game_value& array_target,
        int sender_client_id,
        bool& local_exec
    ) {
        std::vector<int> all_targets;
        local_exec = false;
        
        if (array_target.type_enum() != game_data_type::ARRAY) {
            return all_targets;
        }
        
        auto& arr = array_target.to_array();
        
        for (const auto& elem : arr) {
            resolve_single_target(elem, sender_client_id, all_targets, local_exec);
        }
        
        return get_unique_client_ids(all_targets);
    }

    // Resolve arbitrary string target using various lookup methods
    std::vector<int> resolve_string_target_extended(
        const std::string& target_str,
        int sender_client_id,
        bool& local_exec
    ) {
        std::vector<int> targets;
        local_exec = false;
        int our_client_id = cached_client_id_.load();
        
        // Check for colon - netId format
        if (target_str.find(':') != std::string::npos) {
            // Try object first
            object obj = sqf::object_from_net_id(target_str);
            
            if (!sqf::is_null(obj)) {
                int owner_id = static_cast<int>(sqf::owner(obj));
                
                if (owner_id == our_client_id || owner_id == 2) {
                    local_exec = true;
                } else {
                    targets.push_back(owner_id);
                }
                
                return targets;
            }
            
            // Try group
            group grp = sqf::group_from_net_id(target_str);
            
            if (!sqf::is_null(grp)) {
                auto units = sqf::units(grp);
                std::vector<int> owner_ids;
                
                for (const auto& unit : units) {
                    owner_ids.push_back(static_cast<int>(sqf::owner(unit)));
                }
                
                auto unique_ids = get_unique_client_ids(owner_ids);
                
                for (int id : unique_ids) {
                    if (id == our_client_id || id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(id);
                    }
                }
                
                return targets;
            }
            
            // NetId format but nothing found
            return targets;
        }
        
        // Check UID/ID hashmaps
        game_value uid_machines_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayeruidmachines", game_value());

        if (!uid_machines_gv.is_nil() && uid_machines_gv.type_enum() == game_data_type::HASHMAP) {
            auto& map = uid_machines_gv.to_hashmap();
            
            for (const auto& pair : map) {
                if (pair.key.type_enum() == game_data_type::STRING) {
                    std::string key_str = static_cast<std::string>(pair.key);
                    
                    if (key_str == target_str && pair.value.type_enum() == game_data_type::SCALAR) {
                        int client_id = static_cast<int>(static_cast<float>(pair.value));
                        
                        if (client_id == our_client_id || client_id == 2) {
                            local_exec = true;
                        } else {
                            targets.push_back(client_id);
                        }
                        
                        return targets;
                    }
                }
            }
        }

        game_value id_machines_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allidmachines", game_value());

        if (!id_machines_gv.is_nil() && id_machines_gv.type_enum() == game_data_type::HASHMAP) {
            auto& map = id_machines_gv.to_hashmap();
            
            for (const auto& pair : map) {
                if (pair.key.type_enum() == game_data_type::STRING) {
                    std::string key_str = static_cast<std::string>(pair.key);
                    
                    if (key_str == target_str && pair.value.type_enum() == game_data_type::SCALAR) {
                        int client_id = static_cast<int>(static_cast<float>(pair.value));
                        
                        if (client_id == our_client_id || client_id == 2) {
                            local_exec = true;
                        } else {
                            targets.push_back(client_id);
                        }
                        
                        return targets;
                    }
                }
            }
        }
        
        // Check player units by role_description and name
        game_value all_players_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_allplayerunits", game_value());
        
        if (!all_players_gv.is_nil() && all_players_gv.type_enum() == game_data_type::ARRAY) {
            auto& all_players = all_players_gv.to_array();
            object matched_unit;
            bool found_role_desc = false;
            
            for (const auto& player_gv : all_players) {
                object player_unit = static_cast<object>(player_gv);
                
                if (sqf::is_null(player_unit)) {
                    continue;
                }
                
                std::string role_desc = static_cast<std::string>(sqf::role_description(player_unit));
                
                if (role_desc == target_str) {
                    matched_unit = player_unit;
                    found_role_desc = true;
                    break; // Role description takes priority
                }
                
                if (!found_role_desc) {
                    std::string unit_name = static_cast<std::string>(sqf::name(player_unit));
                    
                    if (unit_name == target_str) {
                        matched_unit = player_unit;
                    }
                }
            }
            
            if (!sqf::is_null(matched_unit)) {
                int owner_id = static_cast<int>(sqf::owner(matched_unit));
                
                if (owner_id == our_client_id || owner_id == 2) {
                    local_exec = true;
                } else {
                    targets.push_back(owner_id);
                }
                
                return targets;
            }
        }
        
        // Check all groups by group_id
        auto all_groups = sqf::all_groups();
        
        for (const auto& grp : all_groups) {
            std::string grp_id = static_cast<std::string>(sqf::group_id(grp));
            
            if (grp_id == target_str) {
                auto units = sqf::units(grp);
                std::vector<int> owner_ids;
                
                for (const auto& unit : units) {
                    owner_ids.push_back(static_cast<int>(sqf::owner(unit)));
                }
                
                auto unique_ids = get_unique_client_ids(owner_ids);
                
                for (int id : unique_ids) {
                    if (id == our_client_id || id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(id);
                    }
                }
                
                return targets;
            }
        }
        
        // Check map markers
        auto all_markers = sqf::all_map_markers();
        
        for (const auto& marker : all_markers) {
            std::string marker_name = static_cast<std::string>(marker);
            
            if (marker_name == target_str) {
                std::vector<int> owner_ids;
                
                if (!all_players_gv.is_nil() && all_players_gv.type_enum() == game_data_type::ARRAY) {
                    auto& all_players = all_players_gv.to_array();
                    
                    for (const auto& player_gv : all_players) {
                        object player_unit = static_cast<object>(player_gv);
                        
                        if (!sqf::is_null(player_unit) && sqf::in_area(player_unit, marker_name)) {
                            owner_ids.push_back(static_cast<int>(sqf::owner(player_unit)));
                        }
                    }
                }
                
                auto unique_ids = get_unique_client_ids(owner_ids);
                
                for (int id : unique_ids) {
                    if (id == our_client_id || id == 2) {
                        local_exec = true;
                    } else {
                        targets.push_back(id);
                    }
                }
                
                return targets;
            }
        }
        
        // Nothing matched - do nothing
        return targets;
    }

    // Send to multiple targets
    bool send_to_targets(
        const std::vector<int>& targets,
        bool local_exec,
        const std::string& event_name,
        const game_value& message,
        int sender_client_id
    ) {
        if (!running_.load()) {
            return false;
        }
        
        try {
            std::shared_ptr<std::vector<uint8_t>> payload;
            
            if (!targets.empty()) {
                payload = payload_pool_->acquire();
                serialize_game_value(*payload, message);
            }
            
            if (local_exec) {
                queue_local_message(event_name, message, sender_client_id);
            }
            
            if (!targets.empty()) {
                for (int target : targets) {
                    outgoing_queue_lockfree_.push(
                        OutgoingMessage(target, sender_client_id, std::string(event_name), payload)
                    );
                }

                outgoing_has_data_.store(true, std::memory_order_release);
            }
            
            return true;
        } catch (const std::exception& e) {
            report_error("KH Network: Failed to send to targets - " + std::string(e.what()));
            return false;
        }
    }

    // [MAGIC(4)][VERSION(4)][FLAGS(1)][TARGET(4)][SENDER(4)][EVENT_LEN(4)][EVENT][PAYLOAD_LEN(4)][PAYLOAD]
    // If compressed: PAYLOAD = [ORIGINAL_SIZE(4)][COMPRESSED_DATA]

    void create_network_packet_into(std::vector<uint8_t>& packet, int target_client, int sender_client,
                                    const std::string& event_name, const std::vector<uint8_t>& payload) {
        packet.clear();
        bool should_compress = config_compression_enabled_ && payload.size() > NET_COMPRESSION_THRESHOLD;
        std::vector<uint8_t> compressed_payload;
        const std::vector<uint8_t>* final_payload = &payload;
        uint8_t flags = NET_FLAG_NONE;
        
        if (should_compress) {
            compressed_payload = compress_payload(payload);

            if (!compressed_payload.empty()) {
                final_payload = &compressed_payload;
                flags |= NET_FLAG_COMPRESSED;
            }
        }
        
        // Header size: magic(4) + version(4) + flags(1) + target(4) + sender(4) + event_len(4) + event + payload_len(4) + payload
        size_t required_size = 25 + event_name.size() + final_payload->size();
        packet.reserve(required_size);
        write_uint32(packet, NET_MSG_MAGIC);
        write_uint32(packet, NET_MSG_VERSION);
        packet.push_back(flags);
        write_uint32(packet, static_cast<uint32_t>(target_client));
        write_uint32(packet, static_cast<uint32_t>(sender_client));
        write_string(packet, event_name);
        write_uint32(packet, static_cast<uint32_t>(final_payload->size()));
        packet.insert(packet.end(), final_payload->begin(), final_payload->end());
    }

    std::vector<uint8_t> create_network_packet(int target_client, int sender_client,
                                                    const std::string& event_name,
                                                    const std::vector<uint8_t>& payload) {
        std::vector<uint8_t> packet;
        create_network_packet_into(packet, target_client, sender_client, event_name, payload);
        return packet;
    }
        
    bool parse_network_packet(const std::vector<uint8_t>& data, int& target_client, 
                                    int& sender_client, std::string& event_name, 
                                    std::vector<uint8_t>& payload) {
        if (data.size() < 12) return false;
        size_t offset = 0;
        uint32_t magic = read_uint32(data.data(), offset, data.size());
        if (magic != NET_MSG_MAGIC) return false;
        uint32_t version = read_uint32(data.data(), offset, data.size());
        if (data.size() < 25) return false;
        if (offset >= data.size()) return false;
        uint8_t flags = data[offset++];
        target_client = static_cast<int>(read_uint32(data.data(), offset, data.size()));
        sender_client = static_cast<int>(read_uint32(data.data(), offset, data.size()));
        
        try {
            event_name = read_string(data.data(), offset, data.size());
            uint32_t payload_len = read_uint32(data.data(), offset, data.size());
            if (payload_len > data.size() - offset) return false;
            
            if (flags & NET_FLAG_COMPRESSED) {
                std::vector<uint8_t> decompressed = decompress_payload(data.data() + offset, payload_len);

                if (decompressed.empty() && payload_len > 4) {
                    return false;  // Decompression failed
                }

                payload = std::move(decompressed);
            } else {
                payload.assign(data.begin() + offset, data.begin() + offset + payload_len);
            }
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool set_socket_options(SOCKET sock) {
        int flag = 1;

        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag)) != 0) {
            return false;
        }
        
        DWORD timeout = static_cast<DWORD>(config_send_timeout_ms_);
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        timeout = static_cast<DWORD>(config_recv_timeout_ms_);
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        int recvbuf = static_cast<int>(config_recv_buffer_size_);
        int sendbuf = static_cast<int>(config_send_buffer_size_);
        setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&recvbuf), sizeof(recvbuf));
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&sendbuf), sizeof(sendbuf));
        int keepalive = 1;
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&keepalive), sizeof(keepalive));
        struct tcp_keepalive ka;
        ka.onoff = 1;
        ka.keepalivetime = static_cast<ULONG>(config_keepalive_time_ms_);
        ka.keepaliveinterval = static_cast<ULONG>(config_keepalive_interval_ms_);
        DWORD bytes_returned = 0;
        WSAIoctl(sock, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &bytes_returned, nullptr, nullptr);
        struct linger lin;
        lin.l_onoff = 1;
        lin.l_linger = 2;
        setsockopt(sock, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&lin), sizeof(lin));
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
        
        if (len > config_max_message_size_) {
            return false;
        }
        
        // Resize thread-local buffer if needed (will reuse capacity)
        if (tls_recv_buffer.capacity() < len) {
            tls_recv_buffer.reserve(std::max(len, static_cast<uint32_t>(config_recv_buffer_size_)));
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
        tls_recv_buffer.clear();  // Clear swapped-in data
        
        if (tls_recv_buffer.capacity() < config_recv_buffer_size_) {
            tls_recv_buffer.reserve(config_recv_buffer_size_);
        }
        
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
            listen_active_clients_.clear();

            {
                std::lock_guard<std::mutex> lock(connections_mutex_);
                listen_active_clients_.reserve(client_connections_.size());
                
                for (auto& pair : client_connections_) {
                    listen_active_clients_.push_back(pair);
                }
            }

            size_t clients_in_fdset = 0;
            constexpr size_t max_clients_per_select = FD_SETSIZE - 1;

            for (size_t i = 0; i < listen_active_clients_.size() && clients_in_fdset < max_clients_per_select; ++i) {
                FD_SET(listen_active_clients_[i].second, &read_fds);
                clients_in_fdset++;
            }

            if (listen_active_clients_.size() > max_clients_per_select && !warned_fd_limit_.exchange(true)) {
                MainThreadScheduler::instance().schedule([]() {
                    sqf::diag_log("KH Network: WARNING - More than " + std::to_string(FD_SETSIZE - 1) + 
                                " clients connected, some may experience delayed messages");
                });
            }
            
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1000;
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
                    std::vector<uint8_t> id_data;

                    if (recv_all(client_sock, id_data) && id_data.size() >= 4) {
                        size_t offset = 0;
                        int client_id = static_cast<int>(read_uint32(id_data.data(), offset, id_data.size()));
                        
                        {
                            std::lock_guard<std::mutex> lock(connections_mutex_);
                            auto existing = client_connections_.find(client_id);

                            if (existing != client_connections_.end()) {
                                ::shutdown(existing->second, SD_BOTH);
                                closesocket(existing->second);
                                per_client_queues_.erase(client_id);
                                client_stall_start_times_.erase(client_id);
                            }

                            client_connections_[client_id] = client_sock;
                        }
                        
                        MainThreadScheduler::instance().schedule([this, client_id]() {
                            send_jip_messages_to_client(client_id);
                        });
                        
                        MainThreadScheduler::instance().schedule([client_id]() {
                            sqf::diag_log("KH Network: Client " + std::to_string(client_id) + " connected");
                        });
                    } else {
                        closesocket(client_sock);
                    }
                }
            }
            
            // Check for incoming data from clients
            listen_clients_to_remove_.clear();
            listen_messages_to_queue_.clear();
            std::vector<OutgoingMessage> forward_messages;
            
            for (size_t i = 0; i < clients_in_fdset && i < listen_active_clients_.size(); ++i) {
                auto& [client_id, client_sock] = listen_active_clients_[i];

                if (FD_ISSET(client_sock, &read_fds)) {
                    std::vector<uint8_t> packet;

                    if (recv_all(client_sock, packet)) {
                        std::vector<NetworkMessage> local_messages;
                        std::vector<OutgoingMessage> fwd_messages;
                        
                        // Handle both coalesced and single packets
                        if (parse_coalesced_or_single_packet(packet, local_messages, fwd_messages)) {
                            for (auto& msg : local_messages) {
                                msg.sender_client_id = client_id;
                                listen_messages_to_queue_.push_back(std::move(msg));
                            }

                            for (auto& msg : fwd_messages) {
                                msg.sender_client = client_id;
                                forward_messages.push_back(std::move(msg));
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
            
            if (!forward_messages.empty()) {
                for (auto& msg : forward_messages) {
                    outgoing_queue_lockfree_.push(std::move(msg));
                }

                outgoing_has_data_.store(true, std::memory_order_release);
            }
            
            // Remove disconnected clients
            if (!listen_clients_to_remove_.empty()) {
                std::lock_guard<std::mutex> lock(connections_mutex_);

                for (int id : listen_clients_to_remove_) {
                    auto it = client_connections_.find(id);
                    
                    if (it != client_connections_.end()) {
                        ::shutdown(it->second, SD_BOTH);
                        closesocket(it->second);
                        client_connections_.erase(it);
                        per_client_queues_.erase(id);
                        client_stall_start_times_.erase(id);
                        
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
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET) {
            return false;
        }

        if (inet_pton(AF_INET, ip.c_str(), &server.sin_addr) != 1) {
            closesocket(sock);
            return false;
        }

        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
        connect(sock, reinterpret_cast<struct sockaddr*>(&server), sizeof(server));
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);
        struct timeval tv;
        tv.tv_sec = config_connect_timeout_ms_ / 1000;
        tv.tv_usec = (config_connect_timeout_ms_ % 1000) * 1000;
        
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
        std::vector<NetworkMessage> parsed_messages;
        parsed_messages.reserve(config_coalesce_max_messages_);
        
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
            tv.tv_usec = 1000;
            
            if (select(0, &read_fds, nullptr, nullptr, &tv) > 0) {
                std::vector<uint8_t> packet;

                if (recv_all(sock, packet)) {
                    parsed_messages.clear();
                    
                    if (parse_incoming_coalesced_packet(packet, parsed_messages)) {
                        if (!parsed_messages.empty()) {
                            std::lock_guard<std::mutex> lock(incoming_mutex_);
                            
                            for (auto& msg : parsed_messages) {
                                incoming_queue_.push_back(std::move(msg));
                            }
                        }
                    }
                } else {
                    // Connection lost
                    std::lock_guard<std::mutex> lock(server_connection_mutex_);

                    if (server_connection_ != INVALID_SOCKET && server_connection_ == sock) {
                        ::shutdown(server_connection_, SD_BOTH);
                        closesocket(server_connection_);
                        server_connection_ = INVALID_SOCKET;
                    }
                }
            }
        }
    }
    
    void send_thread_func() {
        std::vector<OutgoingMessage> batch;
        batch.reserve(config_send_batch_size_);
        std::vector<uint8_t> packet_buffer;
        packet_buffer.reserve(config_send_buffer_size_);
        std::vector<uint8_t> temp_uncompressed_buffer;
        temp_uncompressed_buffer.reserve(config_coalesce_max_size_);
        std::unordered_set<int> clients_to_disconnect;
        std::vector<OutgoingMessage> client_batch;
        client_batch.reserve(config_coalesce_max_messages_);
        
        while (running_.load()) {
            // Wait for data with timeout
            if (!outgoing_has_data_.load(std::memory_order_acquire)) {
                if (config_coalesce_enabled_ && config_coalesce_delay_us_ > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(config_coalesce_delay_us_));
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            
            if (!running_.load()) break;
            
            // Drain the lock-free queue
            batch.clear();

            size_t max_grab = config_coalesce_enabled_ ? 
                std::max(static_cast<size_t>(config_send_batch_size_), config_coalesce_max_messages_ * 2) :
                static_cast<size_t>(config_send_batch_size_);
            
            outgoing_queue_lockfree_.pop_bulk(batch, max_grab);
            
            if (batch.empty()) {
                outgoing_has_data_.store(false, std::memory_order_release);
                continue;
            }
            
            // Check if more data might be available
            if (outgoing_queue_lockfree_.empty()) {
                outgoing_has_data_.store(false, std::memory_order_release);
            }

            if (is_server_.load()) {
                clients_to_disconnect.clear();
                
                // Group messages by target client for coalescing
                std::unordered_map<int, std::vector<OutgoingMessage>> per_client_batch;
                
                {
                    std::lock_guard<std::mutex> conn_lock(connections_mutex_);
                    
                    for (auto& msg : batch) {
                        if (client_connections_.find(msg.target_client) != client_connections_.end()) {
                            per_client_batch[msg.target_client].push_back(std::move(msg));
                        }
                    }

                    batch.clear();
                    
                    // Also gather from per-client queues (for requeued messages)
                    for (auto& [client_id, queue] : per_client_queues_) {
                        if (queue.empty()) continue;
                        auto& client_msgs = per_client_batch[client_id];

                        size_t to_grab = std::min(queue.size(), 
                            config_coalesce_max_messages_ - client_msgs.size());
                        
                        for (size_t i = 0; i < to_grab; ++i) {
                            client_msgs.push_back(std::move(queue.front()));
                            queue.pop_front();
                        }
                    }
                }
                
                // Process each client's messages with coalescing
                for (auto& [client_id, messages] : per_client_batch) {
                    if (messages.empty()) continue;
                    SOCKET sock = INVALID_SOCKET;
                    
                    {
                        std::lock_guard<std::mutex> conn_lock(connections_mutex_);
                        auto conn_it = client_connections_.find(client_id);
                        if (conn_it == client_connections_.end()) continue;
                        sock = conn_it->second;
                        fd_set write_fds, error_fds;
                        FD_ZERO(&write_fds);
                        FD_ZERO(&error_fds);
                        FD_SET(sock, &write_fds);
                        FD_SET(sock, &error_fds);
                        struct timeval tv = {0, 0};
                        int sel_result = select(0, nullptr, &write_fds, &error_fds, &tv);
                        
                        if (sel_result == SOCKET_ERROR || FD_ISSET(sock, &error_fds)) {
                            clients_to_disconnect.insert(client_id);
                            continue;
                        }
                        
                        if (sel_result == 0 || !FD_ISSET(sock, &write_fds)) {
                            // Socket not ready - requeue messages and track stall
                            auto& queue = per_client_queues_[client_id];

                            for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
                                queue.push_front(std::move(*it));
                            }
                            
                            int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now().time_since_epoch()).count();
                            
                            auto stall_it = client_stall_start_times_.find(client_id);

                            if (stall_it == client_stall_start_times_.end()) {
                                client_stall_start_times_[client_id] = now;
                            } else if (now - stall_it->second > config_client_stall_timeout_ms_) {
                                clients_to_disconnect.insert(client_id);
                            }

                            continue;
                        }
                        
                        client_stall_start_times_.erase(client_id);
                    }
                    
                    // Build coalesced packet or packets and send
                    if (config_coalesce_enabled_ && messages.size() > 1) {
                        // Coalesce messages up to max size
                        size_t current_batch_start = 0;
                        size_t current_batch_size = 0;
                        bool send_error = false;
                        
                        for (size_t i = 0; i < messages.size(); ++i) {
                            size_t msg_size = 14 + messages[i].event_name.size() + messages[i].payload->size();
                            
                            bool should_flush = (i - current_batch_start >= config_coalesce_max_messages_) ||
                                               (current_batch_size + msg_size > config_coalesce_max_size_ && 
                                                i > current_batch_start);
                            
                            if (should_flush) {
                                client_batch.clear();

                                for (size_t j = current_batch_start; j < i; ++j) {
                                    client_batch.push_back(std::move(messages[j]));
                                }
                                
                                create_coalesced_packet(packet_buffer, client_batch, temp_uncompressed_buffer);
                                
                                if (!send_all(sock, packet_buffer)) {
                                    clients_to_disconnect.insert(client_id);

                                    std::lock_guard<std::mutex> conn_lock(connections_mutex_);
                                    auto& queue = per_client_queues_[client_id];
                                    
                                    for (size_t k = messages.size(); k-- > i; ) {
                                        queue.push_front(std::move(messages[k]));
                                    }
                                    
                                    for (auto it = client_batch.rbegin(); it != client_batch.rend(); ++it) {
                                        queue.push_front(std::move(*it));
                                    }
                                    
                                    send_error = true;
                                    break;
                                }
                                
                                current_batch_start = i;
                                current_batch_size = msg_size;
                            } else {
                                current_batch_size += msg_size;
                            }
                        }
                        
                        // Send remaining batch
                        if (!send_error && current_batch_start < messages.size() && 
                            clients_to_disconnect.count(client_id) == 0) {
                            client_batch.clear();

                            for (size_t j = current_batch_start; j < messages.size(); ++j) {
                                client_batch.push_back(std::move(messages[j]));
                            }
                            
                            create_coalesced_packet(packet_buffer, client_batch, temp_uncompressed_buffer);
                            
                            if (!send_all(sock, packet_buffer)) {
                                clients_to_disconnect.insert(client_id);

                                std::lock_guard<std::mutex> conn_lock(connections_mutex_);
                                auto& queue = per_client_queues_[client_id];

                                for (auto it = client_batch.rbegin(); it != client_batch.rend(); ++it) {
                                    queue.push_front(std::move(*it));
                                }
                            }
                        }
                    } else {
                        // No coalescing - send individually
                        for (size_t i = 0; i < messages.size(); ++i) {
                            auto& msg = messages[i];

                            create_network_packet_into(packet_buffer, msg.target_client, msg.sender_client,
                                                       msg.event_name, *msg.payload);
                            
                            if (!send_all(sock, packet_buffer)) {
                                clients_to_disconnect.insert(client_id);
                                
                                std::lock_guard<std::mutex> conn_lock(connections_mutex_);
                                auto& queue = per_client_queues_[client_id];
                                
                                for (size_t k = messages.size(); k-- > i; ) {
                                    queue.push_front(std::move(messages[k]));
                                }
                                
                                break;
                            }
                        }
                    }
                }
                
                // Disconnect bad clients
                if (!clients_to_disconnect.empty()) {
                    std::lock_guard<std::mutex> conn_lock(connections_mutex_);

                    for (int id : clients_to_disconnect) {
                        auto it = client_connections_.find(id);

                        if (it != client_connections_.end()) {
                            ::shutdown(it->second, SD_BOTH);
                            closesocket(it->second);
                            client_connections_.erase(it);
                            per_client_queues_.erase(id);
                            client_stall_start_times_.erase(id);
                            
                            MainThreadScheduler::instance().schedule([id]() {
                                sqf::diag_log("KH Network: Client " + std::to_string(id) + " disconnected (stall/error)");
                            });
                        }
                    }
                }
                
            } else {
                // Client mode - send to server with coalescing
                if (batch.empty()) continue;
                
                if (!connect_to_server_internal()) {
                    // Requeue all
                    for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                        outgoing_queue_lockfree_.push(std::move(*it));
                    }
                    
                    outgoing_has_data_.store(true, std::memory_order_release);
                    batch.clear();
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                
                bool send_failed = false;
                
                if (config_coalesce_enabled_ && batch.size() > 1) {
                    // Coalesce messages to server
                    size_t current_batch_start = 0;
                    size_t current_batch_size = 0;
                    
                    for (size_t i = 0; i <= batch.size(); ++i) {
                        size_t msg_size = (i < batch.size()) ? 
                            14 + batch[i].event_name.size() + batch[i].payload->size() : 0;
                        
                        bool should_flush = (i == batch.size()) ||
                                           (i - current_batch_start >= config_coalesce_max_messages_) ||
                                           (current_batch_size + msg_size > config_coalesce_max_size_ && 
                                            i > current_batch_start);
                        
                        if (should_flush && i > current_batch_start) {
                            client_batch.clear();

                            for (size_t j = current_batch_start; j < i; ++j) {
                                client_batch.push_back(std::move(batch[j]));
                            }
                            
                            create_coalesced_packet(packet_buffer, client_batch, temp_uncompressed_buffer);
                            
                            {
                                std::lock_guard<std::mutex> srv_lock(server_connection_mutex_);

                                if (server_connection_ == INVALID_SOCKET) {
                                    send_failed = true;
                                } else if (!send_all(server_connection_, packet_buffer)) {
                                    ::shutdown(server_connection_, SD_BOTH);
                                    closesocket(server_connection_);
                                    server_connection_ = INVALID_SOCKET;
                                    send_failed = true;
                                }
                            }
                            
                            if (send_failed) {
                                // Requeue remaining messages
                                for (size_t k = batch.size(); k-- > i; ) {
                                    outgoing_queue_lockfree_.push(std::move(batch[k]));
                                }
                                
                                for (auto it = client_batch.rbegin(); it != client_batch.rend(); ++it) {
                                    outgoing_queue_lockfree_.push(std::move(*it));
                                }
                                
                                outgoing_has_data_.store(true, std::memory_order_release);
                                break;
                            }
                                                        
                            current_batch_start = i;
                            current_batch_size = msg_size;
                        } else if (i < batch.size()) {
                            current_batch_size += msg_size;
                        }
                    }
                } else {
                    // No coalescing
                    for (size_t i = 0; i < batch.size(); ++i) {
                        auto& msg = batch[i];

                        create_network_packet_into(packet_buffer, msg.target_client, msg.sender_client,
                                                   msg.event_name, *msg.payload);
                        
                        {
                            std::lock_guard<std::mutex> srv_lock(server_connection_mutex_);

                            if (server_connection_ == INVALID_SOCKET) {
                                send_failed = true;
                            } else if (!send_all(server_connection_, packet_buffer)) {
                                ::shutdown(server_connection_, SD_BOTH);
                                closesocket(server_connection_);
                                server_connection_ = INVALID_SOCKET;
                                send_failed = true;
                            }
                        }
                        
                        if (send_failed) {
                            for (size_t j = batch.size(); j-- > i; ) {
                                outgoing_queue_lockfree_.push(std::move(batch[j]));
                            }

                            outgoing_has_data_.store(true, std::memory_order_release);
                            break;
                        }
                    }
                }
                
                batch.clear();
                
                if (send_failed) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
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
        
        payload_pool_ = std::make_shared<PayloadPool>(256);
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

        if (listen_thread_.joinable()) {
            listen_thread_.join();
        }

        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }

        if (send_thread_.joinable()) {
            send_thread_.join();
        }
        
        if (listen_socket_ != INVALID_SOCKET) {
            closesocket(listen_socket_);
            listen_socket_ = INVALID_SOCKET;
        }
        
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);

            for (auto& pair : client_connections_) {
                ::shutdown(pair.second, SD_BOTH);
                closesocket(pair.second);
            }

            client_connections_.clear();
            per_client_queues_.clear();
            client_stall_start_times_.clear();
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
                ::shutdown(server_connection_, SD_BOTH);
                closesocket(server_connection_);
                server_connection_ = INVALID_SOCKET;
            }
        }

        {
            std::lock_guard<std::mutex> lock(incoming_mutex_);
            incoming_queue_.clear();
        }

        {
            std::lock_guard<std::mutex> lock(local_incoming_mutex_);
            local_incoming_queue_.clear();
        }

        cow_handlers_.clear();

        {
            std::lock_guard<std::mutex> lock(jip_mutex_);
            jip_messages_.clear();
        }
        
        cached_client_id_.store(-1);
        warned_fd_limit_.store(false);
        payload_pool_.reset();
        initialized_.store(false);
        sqf::diag_log("KH Network: SHUTDOWN");

        if (wsa_initialized_) {
            WSACleanup();
            wsa_initialized_ = false;
        }
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

    void store_jip_message(const std::string& jip_key, const std::string& event_name, const game_value& message, int sender, const std::string& dependency_net_id = "", bool dependency_is_group = false) {
        if (jip_key.empty() || !is_server_.load()) {
            return;
        }
        
        try {
            std::vector<uint8_t> payload;
            serialize_game_value(payload, message);
            std::lock_guard<std::mutex> lock(jip_mutex_);
            jip_messages_[jip_key] = {event_name, std::move(payload), sender, dependency_net_id, dependency_is_group};
        } catch (const std::exception& e) {
            report_error("KH Network: Failed to store JIP message '" + jip_key + "' - " + std::string(e.what()));
        } catch (...) {
            report_error("KH Network: Failed to store JIP message '" + jip_key + "' - unknown error");
        }
    }

    void remove_jip_message(const std::string& jip_key) {
        std::lock_guard<std::mutex> lock(jip_mutex_);
        jip_messages_.erase(jip_key);
    }

    void clear_all_jip_messages() {
        std::lock_guard<std::mutex> lock(jip_mutex_);
        jip_messages_.clear();
    }

    void send_jip_messages_to_client(int client_id) {
        std::vector<std::tuple<std::string, std::shared_ptr<std::vector<uint8_t>>, int>> messages_to_send;
        
        {
            std::lock_guard<std::mutex> lock(jip_mutex_);
            messages_to_send.reserve(jip_messages_.size());
            
            for (const auto& [key, msg] : jip_messages_) {
                if (!msg.dependency_net_id.empty()) {
                    if (msg.dependency_is_group) {
                        group grp = sqf::group_from_net_id(msg.dependency_net_id);

                        if (sqf::is_null(grp)) {
                            continue;
                        }
                    } else {
                        object obj = sqf::object_from_net_id(msg.dependency_net_id);
                        
                        if (sqf::is_null(obj)) {
                            continue;
                        }
                    }
                }
                
                auto pooled_payload = payload_pool_->acquire();
                *pooled_payload = msg.payload;  // Copy from stored JIP message
                messages_to_send.emplace_back(msg.event_name, std::move(pooled_payload), msg.original_sender);
            }
        }
        
        if (!messages_to_send.empty()) {
            for (auto& [event_name, payload, sender] : messages_to_send) {
                outgoing_queue_lockfree_.push(
                    OutgoingMessage(client_id, sender, std::string(event_name), std::move(payload))
                );
            }

            outgoing_has_data_.store(true, std::memory_order_release);
        }
    }

    bool send_message(int target_client, const std::string& event_name, const game_value& message) {
        if (!running_.load()) {
            return false;
        }
        
        int our_client_id = cached_client_id_.load();

        // Special case: server sending to itself (client 2)
        // Or any machine sending to its own client ID
        if (target_client == our_client_id || 
            (is_server_.load() && target_client == 2)) {
            queue_local_message(event_name, message, our_client_id);
            return true;
        }
        
        try {
            auto payload = payload_pool_->acquire();
            serialize_game_value(*payload, message);
            
            outgoing_queue_lockfree_.push(
                OutgoingMessage(target_client, our_client_id, std::string(event_name), std::move(payload))
            );

            outgoing_has_data_.store(true, std::memory_order_release);
            return true;
        } catch (const std::exception& e) {
            report_error("KH Network: Failed to serialize message - " + std::string(e.what()));
            return false;
        }
    }
    
    bool send_message_to_target(
        NetworkTargetType target_type,
        const game_value& target_data,
        const std::string& event_name,
        const game_value& message
    ) {
        if (!running_.load()) {
            return false;
        }
        
        int our_client_id = cached_client_id_.load();
        
        // Handle simple local-only cases first
        if (target_type == NetworkTargetType::DO_NOTHING) {
            return true;
        }
        
        if (target_type == NetworkTargetType::LOCAL_ONLY) {
            queue_local_message(event_name, message, our_client_id);
            return true;
        }
        
        // If we're the server, we can resolve all targets directly
        if (is_server_.load()) {
            bool local_exec = false;
            std::vector<int> targets = resolve_targets_server(target_type, target_data, our_client_id, local_exec);
            return send_to_targets(targets, local_exec, event_name, message, our_client_id);
        }
        
        // We're a client - check if we can resolve locally or need server help
        bool can_resolve_locally = false;
        bool local_exec = false;
        std::vector<int> targets;
        
        switch (target_type) {
            case NetworkTargetType::CLIENT_ID: {
                int target_id = static_cast<int>(static_cast<float>(target_data));
                
                if (target_id == our_client_id) {
                    local_exec = true;
                } else {
                    targets.push_back(target_id);
                }
                
                can_resolve_locally = true;
                break;
            }
            
            case NetworkTargetType::STRING_SERVER: {
                targets.push_back(2);
                can_resolve_locally = true;
                break;
            }
            
            case NetworkTargetType::STRING_LOCAL: {
                local_exec = true;
                can_resolve_locally = true;
                break;
            }
            
            case NetworkTargetType::OBJECT_OWNER: {
                object obj = static_cast<object>(target_data);
                
                if (sqf::is_null(obj)) {
                    return true;
                }
                
                // If object is local, execute locally
                if (sqf::local(obj)) {
                    local_exec = true;
                    can_resolve_locally = true;
                }

                // Otherwise need server resolution
                break;
            }
            
            case NetworkTargetType::TEAM_MEMBER_OWNER: {
                game_value agent_obj = sqf::agent(target_data);
                object obj = static_cast<object>(agent_obj);
                
                if (sqf::is_null(obj)) {
                    return true;
                }
                
                if (sqf::local(obj)) {
                    local_exec = true;
                    can_resolve_locally = true;
                }

                // Otherwise need server resolution
                break;
            }
            
            default:
                // All other types need server resolution when on client
                break;
        }
        
        if (can_resolve_locally) {
            return send_to_targets(targets, local_exec, event_name, message, our_client_id);
        }
        
        // Need server resolution - send routing request to server
        try {
            // Create routing request payload
            // Format: [target_type, target_data, event_name, message]
            auto_array<game_value> route_data;
            route_data.push_back(game_value(static_cast<float>(static_cast<uint8_t>(target_type))));
            route_data.push_back(target_data);
            route_data.push_back(game_value(event_name));
            route_data.push_back(message);
            
            // Send to server with special routing event name
            return send_message(2, NET_INTERNAL_ROUTE_EVENT, game_value(std::move(route_data)));
        } catch (const std::exception& e) {
            report_error("KH Network: Failed to send routing request - " + std::string(e.what()));
            return false;
        }
    }

    void queue_local_message(const std::string& event_name, const game_value& message, int sender_client_id) {
        std::lock_guard<std::mutex> lock(local_incoming_mutex_);
        local_incoming_queue_.emplace_back(event_name, message, sender_client_id);
    }
    
    // Process internal routing messages (called by server when receiving _KH_INTERNAL_ROUTE_ events)
    void process_routing_request(const game_value& route_data, int sender_client_id) {
        if (!is_server_.load()) {
            return; // Only server processes routing requests
        }
        
        try {
            auto& arr = route_data.to_array();
            
            if (arr.size() < 4) {
                report_error("KH Network: Invalid routing request format");
                return;
            }
            
            NetworkTargetType target_type = static_cast<NetworkTargetType>(static_cast<uint8_t>(static_cast<float>(arr[0])));
            game_value target_data = arr[1];
            std::string event_name = static_cast<std::string>(arr[2]);
            game_value message = arr[3];
            bool local_exec = false;
            std::vector<int> targets = resolve_targets_server(target_type, target_data, sender_client_id, local_exec);
            send_to_targets(targets, local_exec, event_name, message, sender_client_id);
        } catch (const std::exception& e) {
            report_error("KH Network: Failed to process routing request - " + std::string(e.what()));
        }
    }
    
    int add_message_handler(const std::string& event_name, const code& handler, const game_value& args) {
        return cow_handlers_.add_handler(event_name, handler, args);
    }
    
    bool remove_message_handler(int handler_id) {
        return cow_handlers_.remove_handler(handler_id);
    }
    
    // Called from main thread during on_frame
    void process_incoming_messages() {
        // Process local messages first (no deserialization needed)
        std::deque<LocalMessage> local_messages;
        
        {
            std::lock_guard<std::mutex> lock(local_incoming_mutex_);
            
            if (!local_incoming_queue_.empty()) {
                local_messages.swap(local_incoming_queue_);
            }
        }
        
        for (auto& msg : local_messages) {
            try {
                // Internal routing requests should never be local, but handle defensively
                if (msg.event_name == NET_INTERNAL_ROUTE_EVENT) {
                    process_routing_request(msg.message, msg.sender_client_id);
                    continue;
                }
                
                // Check for internal conditional event
                if (msg.event_name == NET_INTERNAL_CONDITIONAL_EVENT) {
                    try {
                        auto& arr = msg.message.to_array();
                        
                        if (arr.size() >= 3) {
                            code condition = arr[0];
                            std::string real_event_name = static_cast<std::string>(arr[1]);
                            game_value real_message = arr[2];
                            game_value result = raw_call_sqf_native(condition);
                            
                            if (result.type_enum() == game_data_type::BOOL && static_cast<bool>(result)) {
                                HandlerListPtr handlers = cow_handlers_.get_handlers(real_event_name);
                                
                                if (handlers) {
                                    for (const auto& handler : *handlers) {
                                        try {
                                            auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;
                                            static r_string message_name = "_this"sv;
                                            static r_string sender_name = "_sender"sv;
                                            static r_string args_name = "_args"sv;
                                            static r_string handler_id_name = "_handlerid"sv;
                                            game_state->set_local_variable(message_name, real_message);
                                            game_state->set_local_variable(sender_name, game_value(static_cast<float>(msg.sender_client_id)));
                                            game_state->set_local_variable(args_name, handler.handler_arguments);
                                            game_state->set_local_variable(handler_id_name, game_value(static_cast<float>(handler.handler_id)));
                                            intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, handler.handler_function);
                                        } catch (const std::exception& e) {
                                            report_error("KH Network: Handler error for '" + msg.event_name + "' - " + std::string(e.what()));
                                        }
                                    }
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        report_error("KH Network: Failed to process local conditional message - " + std::string(e.what()));
                    }

                    continue;
                }
                
                HandlerListPtr handlers = cow_handlers_.get_handlers(msg.event_name);
                
                if (handlers) {
                    for (const auto& handler : *handlers) {
                        try {
                            auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;
                            static r_string message_name = "_this"sv;
                            static r_string sender_name = "_sender"sv;
                            static r_string args_name = "_args"sv;
                            static r_string handler_id_name = "_handlerid"sv;
                            game_state->set_local_variable(message_name, msg.message);
                            game_state->set_local_variable(sender_name, game_value(static_cast<float>(msg.sender_client_id)));
                            game_state->set_local_variable(args_name, handler.handler_arguments);
                            game_state->set_local_variable(handler_id_name, game_value(static_cast<float>(handler.handler_id)));
                            intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, handler.handler_function);
                        } catch (const std::exception& e) {
                            report_error("KH Network: Handler error for '" + msg.event_name + "' - " + std::string(e.what()));
                        }
                    }
                }
            } catch (const std::exception& e) {
                report_error("KH Network: Failed to process local message - " + std::string(e.what()));
            }
        }
        
        // Process network messages (require deserialization)
        std::deque<NetworkMessage> messages;

        {
            std::lock_guard<std::mutex> lock(incoming_mutex_);
            if (incoming_queue_.empty()) return;
            messages.swap(incoming_queue_);
        }
        
        for (auto& msg : messages) {
            try {
                // Check for internal routing request
                if (msg.event_name == NET_INTERNAL_ROUTE_EVENT) {
                    if (!msg.payload.empty()) {
                        size_t offset = 0;
                        game_value route_data = deserialize_game_value(msg.payload.data(), offset, msg.payload.size());
                        process_routing_request(route_data, msg.sender_client_id);
                    }

                    continue;
                } else if (msg.event_name == NET_INTERNAL_CONDITIONAL_EVENT) {
                    if (!msg.payload.empty()) {
                        try {
                            size_t offset = 0;
                            game_value cond_data = deserialize_game_value(msg.payload.data(), offset, msg.payload.size());
                            auto& arr = cond_data.to_array();
                            
                            if (arr.size() >= 3) {
                                code condition = arr[0];
                                std::string real_event_name = static_cast<std::string>(arr[1]);
                                game_value real_message = arr[2];
                                game_value result = raw_call_sqf_native(condition);
                                
                                if (result.type_enum() == game_data_type::BOOL && static_cast<bool>(result)) {
                                    HandlerListPtr handlers = cow_handlers_.get_handlers(real_event_name);
                                    
                                    if (handlers) {
                                        for (const auto& handler : *handlers) {
                                            try {
                                                auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;
                                                static r_string message_name = "_this"sv;
                                                static r_string sender_name = "_sender"sv;
                                                static r_string args_name = "_args"sv;
                                                static r_string handler_id_name = "_handlerid"sv;
                                                game_state->set_local_variable(message_name, real_message);
                                                game_state->set_local_variable(sender_name, game_value(static_cast<float>(msg.sender_client_id)));
                                                game_state->set_local_variable(args_name, handler.handler_arguments);
                                                game_state->set_local_variable(handler_id_name, game_value(static_cast<float>(handler.handler_id)));
                                                intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, handler.handler_function);
                                            } catch (const std::exception& e) {
                                                report_error("KH Network: Handler error for '" + msg.event_name + "' - " + std::string(e.what()));
                                            }
                                        }
                                    }
                                }
                            }
                        } catch (const std::exception& e) {
                            report_error("KH Network: Failed to process conditional message - " + std::string(e.what()));
                        }
                    }

                    continue;
                }

                game_value deserialized;
                
                if (!msg.payload.empty()) {
                    size_t offset = 0;
                    deserialized = deserialize_game_value(msg.payload.data(), offset, msg.payload.size());
                }
                
                HandlerListPtr handlers = cow_handlers_.get_handlers(msg.event_name);
                
                if (handlers) {
                    for (const auto& handler : *handlers) {
                        try {
                            auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;
                            static r_string message_name = "_this"sv;
                            static r_string sender_name = "_sender"sv;
                            static r_string args_name = "_args"sv;
                            static r_string handler_id_name = "_handlerid"sv;
                            game_state->set_local_variable(message_name, deserialized);
                            game_state->set_local_variable(sender_name, game_value(static_cast<float>(msg.sender_client_id)));
                            game_state->set_local_variable(args_name, handler.handler_arguments);
                            game_state->set_local_variable(handler_id_name, game_value(static_cast<float>(handler.handler_id)));
                            intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, handler.handler_function);
                        } catch (const std::exception& e) {
                            report_error("KH Network: Handler error for '" + msg.event_name + "' - " + std::string(e.what()));
                        }
                    }
                }
            } catch (const std::exception& e) {
                report_error("KH Network: Failed to deserialize message - " + std::string(e.what()));
            }
        }
    }

    void apply_settings(const game_value& settings) {
        if (running_.load()) {
            report_error("KH Network: Cannot apply settings while running");
            return;
        }

        if (settings.is_nil() || settings.type_enum() != game_data_type::ARRAY) {
            return; // Use defaults
        }
        
        auto& arr = settings.to_array();
        
        // Helper to safely get values with bounds checking
        auto get_int = [&arr](size_t index, int default_val) -> int {
            if (index < arr.size() && arr[index].type_enum() == game_data_type::SCALAR) {
                return static_cast<int>(static_cast<float>(arr[index]));
            }

            return default_val;
        };
        
        auto get_size = [&arr](size_t index, size_t default_val) -> size_t {
            if (index < arr.size() && arr[index].type_enum() == game_data_type::SCALAR) {
                float val = static_cast<float>(arr[index]);
                return val > 0 ? static_cast<size_t>(val) : default_val;
            }
            
            return default_val;
        };
        
        // Apply settings from array indices
        network_port_.store(get_int(0, NET_DEFAULT_PORT));
        config_max_message_size_ = get_size(1, NET_DEFAULT_MAX_MESSAGE_SIZE);
        config_recv_buffer_size_ = get_size(2, NET_DEFAULT_RECV_BUFFER_SIZE);
        config_send_buffer_size_ = get_size(3, NET_DEFAULT_SEND_BUFFER_SIZE);
        config_connect_timeout_ms_ = get_int(4, NET_DEFAULT_CONNECT_TIMEOUT_MS);
        config_send_timeout_ms_ = get_int(5, NET_DEFAULT_SEND_TIMEOUT_MS);
        config_recv_timeout_ms_ = get_int(6, NET_DEFAULT_RECV_TIMEOUT_MS);
        config_client_stall_timeout_ms_ = get_int(7, NET_DEFAULT_CLIENT_STALL_TIMEOUT_MS);
        config_keepalive_time_ms_ = get_int(8, NET_DEFAULT_KEEPALIVE_TIME_MS);
        config_keepalive_interval_ms_ = get_int(9, NET_DEFAULT_KEEPALIVE_INTERVAL_MS);
        config_send_batch_size_ = std::max(1, get_int(10, NET_DEFAULT_SEND_BATCH_SIZE));
        
        if (arr.size() > 11) {
            if (arr[11].type_enum() == game_data_type::BOOL) {
                config_compression_enabled_ = static_cast<bool>(arr[11]);
            } else if (arr[11].type_enum() == game_data_type::SCALAR) {
                config_compression_enabled_ = static_cast<float>(arr[11]) != 0.0f;
            }
        }

        if (arr.size() > 12) {
            if (arr[12].type_enum() == game_data_type::BOOL) {
                config_coalesce_enabled_ = static_cast<bool>(arr[12]);
            } else if (arr[12].type_enum() == game_data_type::SCALAR) {
                config_coalesce_enabled_ = static_cast<float>(arr[12]) != 0.0f;
            }
        }
        
        // Index 13: Coalesce max size
        config_coalesce_max_size_ = get_size(13, NET_DEFAULT_COALESCE_MAX_SIZE);
        
        // Index 14: Coalesce max messages
        config_coalesce_max_messages_ = get_size(14, NET_DEFAULT_COALESCE_MAX_MESSAGES);
        
        // Index 15: Coalesce delay microseconds
        config_coalesce_delay_us_ = get_int(15, NET_DEFAULT_COALESCE_DELAY_US);
                
        // Clamp values to reasonable ranges
        if (config_max_message_size_ < 1024) config_max_message_size_ = 1024;
        if (config_max_message_size_ > 64 * 1024 * 1024) config_max_message_size_ = 64 * 1024 * 1024;
        if (config_recv_buffer_size_ < 4096) config_recv_buffer_size_ = 4096;
        if (config_send_buffer_size_ < 4096) config_send_buffer_size_ = 4096;
        if (config_connect_timeout_ms_ < 1000) config_connect_timeout_ms_ = 1000;
        if (config_send_timeout_ms_ < 500) config_send_timeout_ms_ = 500;
        if (config_recv_timeout_ms_ < 500) config_recv_timeout_ms_ = 500;
        if (config_client_stall_timeout_ms_ < 1000) config_client_stall_timeout_ms_ = 1000;
        if (config_send_batch_size_ > 256) config_send_batch_size_ = 256;
        if (config_coalesce_max_size_ < 1024) config_coalesce_max_size_ = 1024;
        if (config_coalesce_max_size_ > 1024 * 1024) config_coalesce_max_size_ = 1024 * 1024; // 1MB max
        if (config_coalesce_max_messages_ < 1) config_coalesce_max_messages_ = 1;
        if (config_coalesce_max_messages_ > 1024) config_coalesce_max_messages_ = 1024;
        if (config_coalesce_delay_us_ < 0) config_coalesce_delay_us_ = 0;
        if (config_coalesce_delay_us_ > 10000) config_coalesce_delay_us_ = 10000; // 10ms max
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

    game_value settings = sqf::get_variable(sqf::mission_namespace(), "kh_var_networkingsettings", game_value());
    NetworkFramework::instance().apply_settings(settings);
    
    if (sqf::is_server()) {
        std::string ip_port = NetworkFramework::instance().get_local_ip_port();
        sqf::set_variable(sqf::mission_namespace(), "kh_var_serveraddress", game_value(ip_port), true);

        if (NetworkFramework::instance().start(true)) {
            sqf::diag_log("KH Network: Server started successfully");
        }
    } else {
        game_value server_ip_gv = sqf::get_variable(sqf::mission_namespace(), "kh_var_serveraddress", game_value(""));
        std::string server_ip = static_cast<std::string>(server_ip_gv);

        if (!server_ip.empty()) {
            NetworkFramework::instance().set_server_ip(server_ip);
            
            if (NetworkFramework::instance().start(false)) {
                sqf::diag_log("KH Network: Client started successfully");
            }
        } else {
            report_error("KH Network: Failed to get server");
        }
    }
}

static void network_on_frame() {
    if (NetworkFramework::instance().is_running()) {
        NetworkFramework::instance().process_incoming_messages();
    }
}