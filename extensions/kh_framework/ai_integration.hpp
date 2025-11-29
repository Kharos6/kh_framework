#pragma once

using namespace intercept;
using namespace intercept::types;

class LlamaBackend {
private:
    static std::atomic<int> ref_count;
    static std::mutex backend_mutex;
    
public:
    static void init() {
        std::lock_guard<std::mutex> lock(backend_mutex);

        if (ref_count.fetch_add(1) == 0) {
            // Only init on first AI
            llama_backend_init();
            llama_numa_init(GGML_NUMA_STRATEGY_DISABLED);
        }
    }
    
    static void cleanup() {
        std::lock_guard<std::mutex> lock(backend_mutex);

        if (ref_count.fetch_sub(1) == 1) {
            // Only free on last AI
            llama_backend_free();
        }
    }
};

std::atomic<int> LlamaBackend::ref_count{0};
std::mutex LlamaBackend::backend_mutex;

class AIModelDiscovery {
public:
    static std::vector<std::filesystem::path> find_all_ai_model_directories() {
        return ModFolderSearcher::find_directories_in_mods("ai_models");
    }

    // Search for a specific .gguf file across all locations (Documents + Mods)
    static std::string find_model_file(const std::string& filename) {
        std::vector<std::filesystem::path> search_paths;
        
        // Priority 1: Documents folder
        try {
            char docs_path[MAX_PATH];
            
            if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs_path) == S_OK) {
                std::filesystem::path docs_ai_models = std::filesystem::path(docs_path) / "Arma 3" / "kh_framework" / "ai_models";
                search_paths.push_back(docs_ai_models);
            }
        } catch (...) {}

        // Priority 2: Mod folders
        auto mod_ai_dirs = find_all_ai_model_directories();
        search_paths.insert(search_paths.end(), mod_ai_dirs.begin(), mod_ai_dirs.end());
        
        // Search for the file
        auto found_path = ModFolderSearcher::find_file_by_name(search_paths, filename);
        return found_path.empty() ? "" : found_path.string();
    }

    static std::string find_any_gguf_model() {
        std::vector<std::filesystem::path> search_paths;
        
        // Priority 1: Documents folder
        try {
            char docs_path[MAX_PATH];
            
            if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs_path) == S_OK) {
                std::filesystem::path docs_ai_models = std::filesystem::path(docs_path) / "Arma 3" / "kh_framework" / "ai_models";
                search_paths.push_back(docs_ai_models);
            }
        } catch (...) {}

        // Priority 2: Mod folders
        auto mod_ai_dirs = find_all_ai_model_directories();
        search_paths.insert(search_paths.end(), mod_ai_dirs.begin(), mod_ai_dirs.end());
        
        // Find first .gguf file
        auto found_path = ModFolderSearcher::find_first_file_with_extension(search_paths, ".gguf");
        return found_path.empty() ? "" : found_path.string();
    }
};

struct SharedModel {
    llama_model* model;
    std::atomic<int> ref_count{0};
    std::string model_path;
    llama_model_params model_params;
    
    SharedModel(llama_model* m, const std::string& path, const llama_model_params& params)
        : model(m), model_path(path), model_params(params), ref_count(1) {}
    
    ~SharedModel() {
        if (model) {
            llama_free_model(model);
            model = nullptr;
        }
    }
};

class SharedModelManager {
private:
    static std::mutex models_mutex;
    static std::unordered_map<std::string, std::shared_ptr<SharedModel>> shared_models;

    static std::string create_key(const std::string& model_path, const llama_model_params& params) {
        std::stringstream ss;

        ss << model_path 
           << "|gpu_layers:" << params.n_gpu_layers
           << "|mmap:" << params.use_mmap
           << "|mlock:" << params.use_mlock
           << "|main_gpu:" << params.main_gpu
           << "|vocab_only:" << params.vocab_only;

        if (params.tensor_split != nullptr) {
            ss << "|tensor_split:";

            for (int i = 0; i < 128; i++) { // LLAMA_MAX_DEVICES is typically 128
                if (params.tensor_split[i] > 0.0f) {
                    ss << i << "=" << std::fixed << std::setprecision(3) << params.tensor_split[i] << ",";
                }
            }
        }
        
        return ss.str();
    }
    
public:
    static std::shared_ptr<SharedModel> get_or_create_model(
        const std::string& model_path, 
        const llama_model_params& model_params) {
        std::lock_guard<std::mutex> lock(models_mutex);
        std::string key = create_key(model_path, model_params);
        
        // Check if model already exists with EXACT same parameters
        auto it = shared_models.find(key);

        if (it != shared_models.end()) {
            auto shared_model = it->second;
            shared_model->ref_count.fetch_add(1, std::memory_order_relaxed);            
            return shared_model;
        }

        llama_model* model = llama_load_model_from_file(model_path.c_str(), model_params);
        
        if (!model) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: Failed to load model from file");
            });

            throw std::runtime_error("Failed to load model from file");
        }
        
        auto shared_model = std::make_shared<SharedModel>(model, model_path, model_params);
        shared_models[key] = shared_model;        
        return shared_model;
    }

    static void release_model(const std::shared_ptr<SharedModel>& shared_model) {
        if (!shared_model) return;
        std::lock_guard<std::mutex> lock(models_mutex);
        int remaining_refs = shared_model->ref_count.fetch_sub(1) - 1;
        
        if (remaining_refs <= 0) {
            std::string key = create_key(shared_model->model_path, shared_model->model_params);
            shared_models.erase(key);
        }
    }
    
    // Force cleanup all models (for emergency shutdown)
    static void cleanup_all() {
        std::lock_guard<std::mutex> lock(models_mutex);
        shared_models.clear();
    }
};

std::mutex SharedModelManager::models_mutex;
std::unordered_map<std::string, std::shared_ptr<SharedModel>> SharedModelManager::shared_models;

class AIController {
private:
    std::string ai_name;
    std::shared_ptr<SharedModel> shared_model;
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    const llama_vocab* vocab = nullptr;
    llama_sampler* sampler = nullptr;
    bool initialized{false};
    std::vector<llama_token> system_prompt_tokens;
    std::atomic<int> system_prompt_token_count{0};
    std::atomic<bool> system_prompt_cached{false};
    std::atomic<bool> force_terminate{false};
    std::vector<llama_token> master_prompt_tokens;
    std::atomic<int> master_prompt_token_count{0};
    std::atomic<bool> master_prompt_cached{false};
    
    // Base AI SETTINGS  
    int MAX_NEW_TOKENS = 3072;  
    int N_CTX = 32768;  // Context window
    const int SAFETY_MARGIN = 256;
    int MAX_PROMPT_TOKENS = N_CTX - SAFETY_MARGIN - MAX_NEW_TOKENS;
    float TEMPERATURE = 0.3f;
    int TOP_K = 30;
    float TOP_P = 0.9f;
    int N_BATCH = 2048; // Up to 2048 is good but depends on model
    int N_UBATCH = 1024;
    int CPU_THREADS = 4;
    int CPU_THREADS_BATCH = 6;
    int GPU_LAYERS = g_gpu_available() ? 999 : 0;
    bool FLASH_ATTENTION = g_gpu_available();
    bool OFFLOAD_KV_CACHE = g_gpu_available();
    std::atomic<bool> running{false};
    std::atomic<bool> should_stop{false};
    std::thread ai_thread;
    std::string system_prompt;
    std::string master_prompt;
    std::string user_prompt;
    mutable std::mutex prompt_mutex;
    mutable std::mutex kv_cache_mutex;
    std::condition_variable inference_trigger;
    std::mutex inference_mutex;
    std::atomic<bool> inference_requested{false};
    std::atomic<bool> is_generating{false};
    std::atomic<bool> abort_generation{false};
    std::atomic<bool> log_generation{false};
    std::string marker_system_start = "<|begin_of_text|><|start_header_id|>system<|end_header_id|>";
    std::string marker_system_end = "<|eot_id|>";
    std::string marker_user_start = "<|start_header_id|>user<|end_header_id|>";
    std::string marker_user_end = "<|eot_id|>";
    std::string marker_assistant_start = "<|start_header_id|>assistant<|end_header_id|>";
    std::string marker_assistant_end = "<|eot_id|>";
    std::atomic<uint64_t> current_generation_id{0};

    void schedule_log(const std::string& message) {
        if (!log_generation) return;

        MainThreadScheduler::instance().schedule([message]() {
            sqf::diag_log(message);
        });
    }

    void schedule_progress_callback(const std::string& response_so_far) {
        std::string name = ai_name;
        
        MainThreadScheduler::instance().schedule([name, response_so_far]() {
            auto_array<game_value> ai_response_progress_data;
            ai_response_progress_data.push_back(game_value(name));
            ai_response_progress_data.push_back(game_value(response_so_far));
            raw_call_sqf_args_native_no_return(g_compiled_ai_response_progress_event, game_value(std::move(ai_response_progress_data)));
        });
    }

    struct ConversationTurn {
        std::string user_message;
        std::string assistant_reply;
        
        ConversationTurn(const std::string& user, const std::string& assistant)
            : user_message(user), assistant_reply(assistant) {}
    };

    std::deque<ConversationTurn> conversation_history;
    mutable std::mutex conversation_mutex;

    void add_conversation_turn(const std::string& user_msg, const std::string& assistant_reply) {
        std::lock_guard<std::mutex> lock(conversation_mutex);
        conversation_history.emplace_back(user_msg, assistant_reply);
    }

    std::string create_system_prompt() const {
        std::stringstream prompt;
        prompt << marker_system_start;
        prompt << "\n";
        prompt << system_prompt;
        prompt << "\n";
        prompt << marker_system_end;
        prompt << "\n";
        return prompt.str();
    }

    void cleanup_resources() {
        // Clean up in reverse order of initialization
        if (sampler) {
            try {
                llama_sampler_free(sampler);
            } catch (...) {}

            sampler = nullptr;
        }
        
        if (ctx) {
            try {
                llama_free(ctx);
            } catch (...) {}

            ctx = nullptr;
        }
        
        if (shared_model) {
            try {
                SharedModelManager::release_model(shared_model);
            } catch (...) {}

            shared_model.reset();
            model = nullptr;
        }
                
        vocab = nullptr;
        
        if (initialized) {
            try {
                LlamaBackend::cleanup();
            } catch (...) {}
            
            initialized = false;
        }
        
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        master_prompt_cached = false;
        master_prompt_tokens.clear();
        master_prompt_token_count = 0;
        is_generating = false;
        abort_generation = false;
        inference_requested = false;

        {
            std::lock_guard<std::mutex> lock(prompt_mutex);
            system_prompt.clear();
            master_prompt.clear();
            user_prompt.clear();
        }

        {
            std::lock_guard<std::mutex> lock(conversation_mutex);
            conversation_history.clear();
        }
    }
    
    std::vector<llama_token> tokenize_with_chunking(const std::string& text) const {
        try {
            std::vector<llama_token> tokens = common_tokenize(ctx, text, true, true);
            
            if (tokens.size() > MAX_PROMPT_TOKENS) {
                tokens.resize(MAX_PROMPT_TOKENS);
            }
            
            return tokens;
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - AI Framework: Tokenization error: " + error_msg);
            });

            throw;
        }
    }
    
    void cache_system_prompt() {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        
        try {
            std::string sys_prompt = create_system_prompt();
            system_prompt_tokens = tokenize_with_chunking(sys_prompt);
            system_prompt_token_count = static_cast<int>(system_prompt_tokens.size());
            
            for (size_t i = 0; i < system_prompt_tokens.size(); i += N_BATCH) {
                size_t batch_size = std::min(static_cast<size_t>(N_BATCH), system_prompt_tokens.size() - i);
                llama_batch batch = llama_batch_get_one(&system_prompt_tokens[i], static_cast<int32_t>(batch_size));
                int result = llama_decode(ctx, batch);

                if (result != 0) {
                    MainThreadScheduler::instance().schedule([]() {
                        report_error("KH - AI Framework: System prompt decode failed");
                    });
                }
            }
      
            system_prompt_cached = true;
        } catch (const std::exception& e) {
            std::string name = ai_name;
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([name, error_msg]() {
                report_error("KH - AI Framework: AI Controller (" + name + "): Failed to cache system prompt: " + error_msg);
            });
        }
    }

    void cache_master_prompt() {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        master_prompt_cached = false;
        master_prompt_tokens.clear();
        master_prompt_token_count = 0;
        
        if (master_prompt.empty()) {
            master_prompt_cached = true;
            return;
        }
        
        try {
            std::string master_text = marker_user_start + "\n" + master_prompt + "\n" + marker_user_end + "\n";
            master_prompt_tokens = tokenize_with_chunking(master_text);
            master_prompt_token_count = static_cast<int>(master_prompt_tokens.size());
            
            for (size_t i = 0; i < master_prompt_tokens.size(); i += N_BATCH) {
                size_t batch_size = std::min(static_cast<size_t>(N_BATCH), master_prompt_tokens.size() - i);
                llama_batch batch = llama_batch_get_one(&master_prompt_tokens[i], static_cast<int32_t>(batch_size));
                int result = llama_decode(ctx, batch);

                if (result != 0) {
                    MainThreadScheduler::instance().schedule([]() {
                        report_error("KH - AI Framework: Master prompt decode failed");
                    });
                }
            }
    
            master_prompt_cached = true;
        } catch (const std::exception& e) {
            std::string name = ai_name;
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([name, error_msg]() {
                report_error("KH - AI Framework: AI Controller (" + name + "): Failed to cache master prompt: " + error_msg);
            });
        }
    }

    class GenerationGuard {
    private:
        std::atomic<bool>& flag;
        bool active;
        
    public:
        explicit GenerationGuard(std::atomic<bool>& f) : flag(f), active(true) {
            flag = true;
        }
        
        ~GenerationGuard() {
            if (active) {
                flag = false;
            }
        }
        
        // Prevent copying
        GenerationGuard(const GenerationGuard&) = delete;
        GenerationGuard& operator=(const GenerationGuard&) = delete;
    };

    std::string generate_response() {
        GenerationGuard guard(is_generating);
        std::string current_user_message;
        
        {
            std::lock_guard<std::mutex> lock(prompt_mutex);
            
            if (user_prompt.empty()) {
                return "";
            }
            
            current_user_message = user_prompt;
        }
        
        if (!ctx || !model) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: Context or model not initialized");
            });

            throw std::runtime_error("Context or model not initialized");
        }

        std::unique_lock<std::mutex> kv_lock(kv_cache_mutex);
        bool system_prompt_was_recached = false;
        
        if (!system_prompt_cached) {
            llama_memory_t kv_memory = llama_get_memory(ctx);
            llama_memory_seq_rm(kv_memory, -1, 0, -1);
            cache_system_prompt();
            master_prompt_cached = false;
            system_prompt_was_recached = true;
        }

        bool master_prompt_was_recached = false;
        
        if (!master_prompt_cached) {
            cache_master_prompt();
            master_prompt_was_recached = true;
        }

        std::string new_message_text = marker_user_start + "\n" + current_user_message + "\n" + marker_user_end + "\n" + marker_assistant_start + "\n";
        
        // Tokenize new message
        std::vector<llama_token> new_message_tokens = tokenize_with_chunking(new_message_text);
        int new_message_token_count = static_cast<int>(new_message_tokens.size());
        
        // Calculate available space for history
        int available_for_history = MAX_PROMPT_TOKENS - system_prompt_token_count - master_prompt_token_count - new_message_token_count;
        std::vector<llama_token> all_prompt_tokens;

        {
            std::lock_guard<std::mutex> lock(conversation_mutex);
            std::deque<std::string> history_parts;
            int accumulated_tokens = 0;

            for (auto it = conversation_history.rbegin(); it != conversation_history.rend(); ++it) {
                std::string user_part = marker_user_start + "\n" + it->user_message + "\n" + marker_user_end + "\n";
                std::string assistant_part = marker_assistant_start + "\n" + it->assistant_reply + "\n" + marker_assistant_end + "\n";
                std::string full_turn = user_part + assistant_part;
                std::vector<llama_token> turn_tokens = tokenize_with_chunking(full_turn);
                int turn_token_count = static_cast<int>(turn_tokens.size());
                
                // Check if adding this turn would exceed budget
                if (accumulated_tokens + turn_token_count > available_for_history) {
                    break;
                }
                
                history_parts.push_front(full_turn);
                accumulated_tokens += turn_token_count;
            }

            std::stringstream conversation_builder;

            for (const auto& part : history_parts) {
                conversation_builder << part;
            }
            
            conversation_builder << new_message_text;
            std::string conversation_text = conversation_builder.str();
            all_prompt_tokens = tokenize_with_chunking(conversation_text);
            size_t turns_to_keep = history_parts.size();

            while (conversation_history.size() > turns_to_keep) {
                conversation_history.pop_front();
            }
        }
        
        if (all_prompt_tokens.empty()) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: Tokenization produced empty result");
            });

            throw std::runtime_error("Tokenization produced empty result");
        }

        llama_batch batch;
        int decode_result = 0;
        llama_memory_t kv_memory = llama_get_memory(ctx);

        if (!system_prompt_was_recached && !master_prompt_was_recached) {
            int cached_end = system_prompt_token_count + master_prompt_token_count;
            llama_memory_seq_rm(kv_memory, -1, cached_end, -1);
        }
        
        llama_sampler_reset(sampler);
        
        // Decode ONLY conversation tokens (system prompt already in KV cache)
        auto start_decode = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < all_prompt_tokens.size(); i += N_BATCH) {
            size_t batch_size = std::min(static_cast<size_t>(N_BATCH), all_prompt_tokens.size() - i);
            batch = llama_batch_get_one(&all_prompt_tokens[i], static_cast<int32_t>(batch_size));
            decode_result = llama_decode(ctx, batch);
            
            if (decode_result != 0) {
                int chunk_num = static_cast<int>(i / N_BATCH);
                int result_code = decode_result;

                MainThreadScheduler::instance().schedule([chunk_num, result_code]() {
                    report_error("KH - AI Framework: Decode failed at chunk " + 
                                std::to_string(chunk_num) + " with code " + 
                                std::to_string(result_code));
                });

                throw std::runtime_error("Decode failed");
            }
        }
        
        auto end_decode = std::chrono::high_resolution_clock::now();
        std::string processed_display_output;
        processed_display_output.reserve(MAX_NEW_TOKENS * 4);
        int n_generated = 0;
        auto start_gen = std::chrono::high_resolution_clock::now();
        auto last_log_time = start_gen;
        int tokens_since_last_log = 0;
        const int PROGRESS_UPDATE_INTERVAL = 8;
        bool generation_completed = true;

        if (log_generation) {
            int total_tokens = system_prompt_token_count + master_prompt_token_count + static_cast<int>(all_prompt_tokens.size());
            schedule_log("KH - AI Framework: (" + ai_name + "): ========== INFERENCE START ==========");
            schedule_log("KH - AI Framework: (" + ai_name + "):   System Token Count: " + std::to_string(system_prompt_token_count) + " tokens");
            schedule_log("KH - AI Framework: (" + ai_name + "):   Master Token Count: " + std::to_string(master_prompt_token_count) + " tokens");
            schedule_log("KH - AI Framework: (" + ai_name + "):   Conversation Token Count: " + std::to_string(total_tokens - system_prompt_token_count) + " tokens");
            schedule_log("KH - AI Framework: (" + ai_name + "):   Total Context Usage: " + std::to_string(total_tokens) + " / " + std::to_string(N_CTX));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Maximum Generated Tokens: " + std::to_string(MAX_NEW_TOKENS));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Maximum Total Tokens: " + std::to_string(MAX_PROMPT_TOKENS));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Context Size: " + std::to_string(N_CTX));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Temperature: " + std::to_string(TEMPERATURE));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Top K: " + std::to_string(TOP_K));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Top P: " + std::to_string(TOP_P));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Batch Size: " + std::to_string(N_BATCH));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Micro Batch Size: " + std::to_string(N_UBATCH));
            schedule_log("KH - AI Framework: (" + ai_name + "):   CPU Threads: " + std::to_string(CPU_THREADS));
            schedule_log("KH - AI Framework: (" + ai_name + "):   CPU Threads Batch: " + std::to_string(CPU_THREADS_BATCH));
            schedule_log("KH - AI Framework: (" + ai_name + "):   GPU Layers: " + std::to_string(GPU_LAYERS));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Flash Attention: " + std::string(FLASH_ATTENTION ? "enabled" : "disabled"));
            schedule_log("KH - AI Framework: (" + ai_name + "):   KV Cache Offload: " + std::string(OFFLOAD_KV_CACHE ? "enabled" : "disabled"));
        }

        for (int i = 0; i < MAX_NEW_TOKENS; i++) {
            if (should_stop || force_terminate) {
                // Only set generation_completed here - we still want partial responses if the user stops
                generation_completed = false;
                break;
            }

            if (abort_generation) {
                break;
            };

            auto sample_start = std::chrono::high_resolution_clock::now();
            llama_token new_token_id = llama_sampler_sample(sampler, ctx, -1);
            auto sample_end = std::chrono::high_resolution_clock::now();
            int vocab_size = llama_vocab_n_tokens(vocab);

            if (new_token_id < 0 || new_token_id >= vocab_size) {
                int token_id = new_token_id;
                int vsize = vocab_size;

                MainThreadScheduler::instance().schedule([token_id, vsize]() {
                    report_error("KH - AI Framework: Invalid token ID sampled: " + std::to_string(token_id) + 
                                " (vocab size: " + std::to_string(vsize) + ")");
                });
                            
                break;
            }

            if (llama_token_is_eog(vocab, new_token_id)) {
                break;
            }
            
            // Convert token to text
            char token_buffer[256];
            auto ttp_start = std::chrono::high_resolution_clock::now();
            int n_chars = llama_token_to_piece(vocab, new_token_id, token_buffer, sizeof(token_buffer), 0, true);
            auto ttp_end = std::chrono::high_resolution_clock::now();
            
            if (n_chars < 0) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - AI Framework: ERROR: Token to piece conversion failed");
                });

                break;
            }

            processed_display_output += std::string(token_buffer, n_chars);
            auto get_batch_start = std::chrono::high_resolution_clock::now();
            batch = llama_batch_get_one(&new_token_id, 1);
            auto get_batch_end = std::chrono::high_resolution_clock::now();
            
            if (batch.n_tokens != 1) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - AI Framework: ERROR: Batch creation failed");
                });

                break;
            }
            
            n_generated++;
            tokens_since_last_log++;

            // Decode next token
            auto decode_start = std::chrono::high_resolution_clock::now();

            try {
                decode_result = llama_decode(ctx, batch);
            } catch (const std::exception& e) {
                std::string error_msg = e.what();

                MainThreadScheduler::instance().schedule([error_msg]() {
                    report_error("KH - AI Framework: EXCEPTION during generation: " + error_msg);
                });

                break;
            }

            auto decode_end = std::chrono::high_resolution_clock::now();
                    
            if (decode_result != 0) {
                int token_num = n_generated;

                MainThreadScheduler::instance().schedule([token_num]() {
                    report_error("KH - AI Framework: Generation decode failed at token " + std::to_string(token_num));
                });

                break;
            }

            if (n_generated % PROGRESS_UPDATE_INTERVAL == 0) {
                schedule_progress_callback(processed_display_output);
            }

            if (log_generation) {
                auto current_time = std::chrono::high_resolution_clock::now();
                auto time_since_last_log = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_log_time);
                auto sample_duration = std::chrono::duration_cast<std::chrono::milliseconds>(sample_end - sample_start);
                schedule_log("KH - AI Framework: (" + ai_name + "):   Token Sampling Time: " + std::to_string(sample_duration.count()) + "ms");
                auto ttp_duration = std::chrono::duration_cast<std::chrono::milliseconds>(ttp_end - ttp_start);
                schedule_log("KH - AI Framework: (" + ai_name + "):   Token To Piece Time: " + std::to_string(ttp_duration.count()) + "ms");
                auto get_batch_duration = std::chrono::duration_cast<std::chrono::milliseconds>(get_batch_end - get_batch_start);
                schedule_log("KH - AI Framework: (" + ai_name + "):   Token Batch Time: " + std::to_string(get_batch_duration.count()) + "ms");
                auto decode_duration = std::chrono::duration_cast<std::chrono::milliseconds>(decode_end - decode_start);
                schedule_log("KH - AI Framework: (" + ai_name + "):   Token Decode Time: " + std::to_string(decode_duration.count()) + "ms");
                
                if (time_since_last_log.count() >= 1000) {
                    float tokens_per_second = (tokens_since_last_log * 1000.0f) / time_since_last_log.count();

                    schedule_log("KH - AI Framework: (" + ai_name + "):   Generation rate: " + 
                                std::to_string(static_cast<int>(tokens_per_second)) + " tokens/sec (" + 
                                std::to_string(tokens_since_last_log) + " tokens in " + 
                                std::to_string(time_since_last_log.count()) + "ms)");

                    last_log_time = current_time;
                    tokens_since_last_log = 0;
                }
            }
        }

        kv_lock.unlock();
        auto gen_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_gen);

        if (log_generation) {
            schedule_log("KH - AI Framework: (" + ai_name + "):   Tokens Generated: " + std::to_string(n_generated));
            schedule_log("KH - AI Framework: (" + ai_name + "):   Total Generation Time: " + std::to_string(gen_duration.count()) + "ms");
            schedule_log("KH - AI Framework: (" + ai_name + "):   Response: " + processed_display_output);
            schedule_log("KH - AI Framework: (" + ai_name + "): ========== INFERENCE END ==========");
        }

        size_t end_pos = processed_display_output.find(marker_assistant_end);

        if (end_pos != std::string::npos) {
            processed_display_output = processed_display_output.substr(0, end_pos);
        }

        size_t start = processed_display_output.find_first_not_of(" \n\r\t");

        if (start != std::string::npos) {
            processed_display_output.erase(0, start);
        }

        if (!processed_display_output.empty()) {
            size_t end = processed_display_output.find_last_not_of(" \n\r\t");

            if (end != std::string::npos) {
                processed_display_output.erase(end + 1);
            }
        }

        if (generation_completed && !processed_display_output.empty()) {
            add_conversation_turn(current_user_message, processed_display_output);
        }
                                
        return processed_display_output;
    }
    
    // AI processing thread
    void ai_thread_func(const std::string& model_path) {
        try {
            initialize_internal(model_path);
        } catch (const std::exception& e) {
            std::string name = ai_name;
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([name, error_msg]() {
                report_error("KH - AI Framework: AI Controller (" + name + "): Failed to initialize: " + error_msg);
            });

            running = false;
            return;
        }
        
        // Main processing loop - waits for inference triggers
        while (!should_stop && !force_terminate) {
            try {
                if (force_terminate) {
                    std::string name = ai_name;

                    MainThreadScheduler::instance().schedule([name]() {
                        sqf::diag_log("KH - AI Framework: AI thread (" + name + ") received force termination signal");
                    });

                    break;
                }

                {
                    std::unique_lock<std::mutex> lock(inference_mutex);

                    inference_trigger.wait(lock, [this] { 
                        return (inference_requested && initialized) || should_stop || force_terminate;
                    });
                    
                    if (should_stop || force_terminate) {
                        break;
                    }

                    inference_requested = false;
                }
                
                bool should_process = false;

                {
                    std::lock_guard<std::mutex> lock(prompt_mutex);
                    should_process = !user_prompt.empty();
                }
                
                if (!should_process) {
                    continue;
                }

                if (force_terminate) break;
                abort_generation = false;
                current_generation_id++;  // Increment before generation so progress events use correct ID
                std::string raw_response = generate_response();
                if (force_terminate) break;

                {
                    std::string name = ai_name;
                    std::string response = raw_response;
                    
                    MainThreadScheduler::instance().schedule([name, response]() {
                        auto_array<game_value> ai_response_data;
                        ai_response_data.push_back(game_value(name));
                        ai_response_data.push_back(game_value(response));
                        raw_call_sqf_args_native_no_return(g_compiled_ai_response_event, game_value(std::move(ai_response_data)));
                    });
                }
            } catch (const std::exception& e) {
                std::string name = ai_name;
                std::string error_msg = e.what();

                MainThreadScheduler::instance().schedule([name, error_msg]() {
                    report_error("KH - AI Framework: AI Controller (" + name + "): Thread error: " + error_msg);
                });

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    void initialize_internal(const std::string& model_path) {        
        try {
            LlamaBackend::init();           
            llama_model_params model_params = llama_model_default_params();
            model_params.n_gpu_layers = g_gpu_available() ? GPU_LAYERS : 0;
            model_params.use_mmap = true;
            model_params.use_mlock = false;
            // main_gpu, tensor_split, etc... use their defaults

            {
                std::string backend = get_backend_name();
                bool gpu = g_gpu_available();

                MainThreadScheduler::instance().schedule([gpu, backend]() {
                    if (gpu) {
                        sqf::diag_log("KH - AI Framework: using GPU - " + backend);
                    } else {
                        sqf::diag_log("KH - AI Framework: using CPU");
                    }
                });
            }

            // Get or create shared model - will share if model_path AND all model_params match
            shared_model = SharedModelManager::get_or_create_model(model_path, model_params);

            if (!shared_model || !shared_model->model) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - AI Framework: Failed to load model");
                });

                throw std::runtime_error("Failed to load model");
            }

            model = shared_model->model;
            vocab = llama_model_get_vocab(model);

            if (!vocab) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - AI Framework: Failed to get vocabulary");
                });

                throw std::runtime_error("Failed to get vocabulary");
            }

            llama_context_params ctx_params = llama_context_default_params();
            ctx_params.n_ctx = N_CTX;
            ctx_params.n_batch = N_BATCH;
            ctx_params.n_ubatch = N_UBATCH;
            ctx_params.n_threads = CPU_THREADS;
            ctx_params.n_threads_batch = CPU_THREADS_BATCH;
            ctx_params.flash_attn_type = g_gpu_available() ? (FLASH_ATTENTION ? LLAMA_FLASH_ATTN_TYPE_ENABLED : LLAMA_FLASH_ATTN_TYPE_DISABLED) : LLAMA_FLASH_ATTN_TYPE_DISABLED;
            ctx_params.offload_kqv = g_gpu_available() ? OFFLOAD_KV_CACHE : false;
            ctx_params.type_k = GGML_TYPE_Q8_0;
            ctx_params.type_v = GGML_TYPE_Q8_0;

            ctx = llama_new_context_with_model(model, ctx_params);

            if (!ctx) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - AI Framework: Failed to create context");
                });

                throw std::runtime_error("Failed to create context");
            }

            auto sampler_params = llama_sampler_chain_default_params();
            sampler_params.no_perf = false;
            sampler = llama_sampler_chain_init(sampler_params);
            llama_sampler_chain_add(sampler, llama_sampler_init_temp(TEMPERATURE));
            llama_sampler_chain_add(sampler, llama_sampler_init_top_k(TOP_K));
            llama_sampler_chain_add(sampler, llama_sampler_init_top_p(TOP_P, 1));
            llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
            initialized = true;

            {
                std::string name = ai_name;
                
                MainThreadScheduler::instance().schedule([name]() {
                    auto_array<game_value> ai_initialized_data;
                    ai_initialized_data.push_back(game_value(name));
                    raw_call_sqf_args_native_no_return(g_compiled_ai_initialized_event, game_value(std::move(ai_initialized_data)));
                });
            }
        } catch (const std::exception& e) {
            cleanup_resources();
            std::string name = ai_name;
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([name, error_msg]() {
                report_error("KH - AI Framework: AI Controller (" + name + "): Initialization failed: " + error_msg);
                
                if (error_msg.find("CUDA") != std::string::npos || 
                    error_msg.find("cublas") != std::string::npos ||
                    error_msg.find("GPU") != std::string::npos) {
                    report_error("KH - AI Framework: This may be a CUDA-related error. Ensure CUDA Toolkit 12.x is installed.");
                }
            });
            
            throw;
        } catch (...) {
            cleanup_resources();
            std::string name = ai_name;

            MainThreadScheduler::instance().schedule([name]() {
                report_error("KH - AI Framework: AI Controller (" + name + "): FAILED: Unknown error");
                report_error("KH - AI Framework: CUDA libraries not found or failed to load.");
            });

            throw;
        }
    }
    
public:
    AIController(const std::string& name) : ai_name(name) {}
    
    ~AIController() {
        try {
            if (running) {
                stop();
            }

            if (ai_thread.joinable()) {
                ai_thread.join();
            }
        } catch (const std::exception& e) {
            // Log but don't throw from destructor - schedule for next frame
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - AI Framework: Exception in ~AIController: " + error_msg);
            });
        } catch (...) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: Unknown exception in ~AIController");
            });
        }
    }
    
    // Delete copy/move constructors
    AIController(const AIController&) = delete;
    AIController& operator=(const AIController&) = delete;
    AIController(AIController&&) = delete;
    AIController& operator=(AIController&&) = delete;

    bool start(const std::string& model_path) {
        if (running) {
            std::string name = ai_name;

            MainThreadScheduler::instance().schedule([name]() {
                report_error("KH - AI Framework: AI Controller (" + name + "): Already running");
            });

            return false;
        }
        
        // Use instance-specific model path if set, otherwise use provided path
        if (model_path.empty()) {
            std::string name = ai_name;

            MainThreadScheduler::instance().schedule([name]() {
                report_error("KH - AI Framework: AI Controller (" + name + "): No model path specified");
            });

            return false;
        }
        
        should_stop = false;
        running = true;
        force_terminate = false;
        abort_generation = false;
        is_generating = false;
        inference_requested = false;
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        master_prompt_cached = false;
        master_prompt_tokens.clear();
        master_prompt_token_count = 0;
        ai_thread = std::thread(&AIController::ai_thread_func, this, model_path);
        return true;
    }

    void stop() {
        if (!running) {
            return;
        }
        
        should_stop = true;
        abort_generation = true;
        running = false;
        
        {
            std::lock_guard<std::mutex> lock(inference_mutex);
            inference_requested = true;
        }

        inference_trigger.notify_all();
        auto start = std::chrono::steady_clock::now();
        
        while (is_generating && 
            std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // If still generating, force terminate
        if (is_generating) {
            force_terminate = true;
            inference_trigger.notify_all();
            
            // Give it another second for safety
            start = std::chrono::steady_clock::now();
            
            while (is_generating && 
                std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        
        // Final join - this will block until thread actually exits
        if (ai_thread.joinable()) {
            ai_thread.join();
        }
            
        cleanup_resources();
        should_stop = false;
        force_terminate = false;
        abort_generation = false;
        is_generating = false;
        running = false;
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        master_prompt_cached = false;
        master_prompt_tokens.clear();
        master_prompt_token_count = 0;
        current_generation_id = 0;
    }

    void set_log_generation(bool enabled) {
        log_generation = enabled;
    }

    void abort_current_generation() {
        abort_generation = true;
    }

    static std::string process_escape_sequences(const std::string& input) {
        std::string result;
        result.reserve(input.size());
        
        for (size_t i = 0; i < input.size(); ++i) {
            if (i + 1 < input.size() && input[i] == '\\') {
                if (input[i + 1] == 'n') { result += '\n'; ++i; continue; }
                if (input[i + 1] == 't') { result += '\t'; ++i; continue; }
            }
            
            if (i + 4 < input.size() && input.compare(i, 5, "<br/>") == 0) {
                result += '\n'; i += 4; continue;
            }

            if (i + 3 < input.size() && input.compare(i, 4, "<br>") == 0) {
                result += '\n'; i += 3; continue;
            }
            
            result += input[i];
        }
        
        return result;
    }

    void update_system_prompt(const std::string& prompt) {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        std::string processed = process_escape_sequences(prompt);

        if (system_prompt != processed) {
            system_prompt = processed;
            system_prompt_cached = false;
            system_prompt_tokens.clear();
            system_prompt_token_count = 0;
        }
    }

    void update_master_prompt(const std::string& prompt) {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        std::string processed = process_escape_sequences(prompt);

        if (master_prompt != processed) {
            master_prompt = processed;
            master_prompt_cached = false;
            master_prompt_tokens.clear();
            master_prompt_token_count = 0;
        }
    }
        
    void update_user_prompt(const std::string& prompt) {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        user_prompt = process_escape_sequences(prompt);
    }

    bool is_running() const {
        return running;
    }

    bool is_generating_response() const {
        return is_generating;
    }
        
    // Set AI parameters (must be called before start() or while stopped)
    bool set_parameters(int n_ctx, int max_new_tokens, float temperature, 
                    int top_k, float top_p, int n_batch, int n_ubatch, int cpu_threads, int cpu_threads_batch, int gpu_layers, bool flash_attention, bool offload_kv_cache) {
        if (running) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: Cannot change parameters while AI is running. Stop the AI first.");
            });

            return false;
        }
        
        // Validate params
        if (n_ctx < 512) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: N_CTX must be at least 512");
            });

            return false;
        }
        
        if (max_new_tokens < 1) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: MAX_NEW_TOKENS must be at least 1");
            });

            return false;
        }
        
        if (temperature < 0.0f || temperature > 2.0f) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: TEMPERATURE must be between 0.0 and 2.0");
            });

            return false;
        }
        
        if (top_k < 1 || top_k > 1000) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: TOP_K must be between 1 and 1000");
            });

            return false;
        }
        
        if (top_p < 0.0f || top_p > 1.0f) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: TOP_P must be between 0.0 and 1.0");
            });

            return false;
        }
        
        if (n_batch < 1) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: N_BATCH must be at least 1");
            });

            return false;
        }
        
        if (n_ubatch < 1) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: N_UBATCH must be at least 1");
            });

            return false;
        }
        
        if (cpu_threads < 1) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: CPU_THREADS must be greater than 0");
            });

            return false;
        }

        if (cpu_threads_batch < 1) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: CPU_THREADS_BATCH must be greater than 0");
            });

            return false;
        }

        if (n_ctx < SAFETY_MARGIN + max_new_tokens + 2048) {
            int required = SAFETY_MARGIN + max_new_tokens + 2048;
            
            MainThreadScheduler::instance().schedule([required]() {
                report_error("KH - AI Framework: N_CTX too small for MAX_NEW_TOKENS. Need at least " + std::to_string(required));
            });

            return false;
        }
        
        N_CTX = n_ctx;
        MAX_NEW_TOKENS = max_new_tokens;
        TEMPERATURE = temperature;
        TOP_K = top_k;
        TOP_P = top_p;
        N_BATCH = n_batch;
        N_UBATCH = n_ubatch;
        CPU_THREADS = cpu_threads;
        CPU_THREADS_BATCH = cpu_threads_batch;
        MAX_PROMPT_TOKENS = N_CTX - SAFETY_MARGIN - MAX_NEW_TOKENS;
        GPU_LAYERS = gpu_layers;
        FLASH_ATTENTION = flash_attention;
        OFFLOAD_KV_CACHE = offload_kv_cache;
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        master_prompt_cached = false;
        master_prompt_tokens.clear();
        master_prompt_token_count = 0;
        return true;
    }

    bool trigger_inference() {
        if (!running) {
            std::string name = ai_name;

            MainThreadScheduler::instance().schedule([name]() {
                report_error("KH - AI Framework: AI (" + name + ") is not running");
            });
            
            return false;
        }

        if (is_generating) {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(inference_mutex);
            inference_requested = true;
        }

        inference_trigger.notify_one();
        return true;
    }

    void reset_conversation() {
        // Request abort if generating
        if (is_generating) {
            abort_generation = true;
            auto start = std::chrono::steady_clock::now();

            while (is_generating && 
                std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        std::lock_guard<std::mutex> kv_lock(kv_cache_mutex);
        
        if (!initialized || !ctx) {
            abort_generation = false;
            return;
        }

        {
            std::lock_guard<std::mutex> conv_lock(conversation_mutex);
            conversation_history.clear();
        }

        llama_memory_t kv_memory = llama_get_memory(ctx);
        
        if (system_prompt_cached && system_prompt_token_count > 0) {
            llama_memory_seq_rm(kv_memory, -1, system_prompt_token_count, -1);
        } else {
            llama_memory_seq_rm(kv_memory, -1, 0, -1);
        }

        {
            std::lock_guard<std::mutex> prompt_lock(prompt_mutex);
            master_prompt.clear();
            user_prompt.clear();
        }
        
        abort_generation = false;
    }

    bool set_markers(const std::string& sys_start, const std::string& sys_end,
                     const std::string& usr_start, const std::string& usr_end,
                     const std::string& asst_start, const std::string& asst_end) {
        if (running) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: Cannot change markers while AI is running. Stop the AI first.");
            });

            return false;
        }

        std::lock_guard<std::mutex> lock(prompt_mutex);
        marker_system_start = sys_start;
        marker_system_end = sys_end;
        marker_user_start = usr_start;
        marker_user_end = usr_end;
        marker_assistant_start = asst_start;
        marker_assistant_end = asst_end;
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        master_prompt_cached = false;
        master_prompt_tokens.clear();
        master_prompt_token_count = 0;
        return true;
    }
};

struct AIPromptData {
    std::string system_prompt;
    std::string master_prompt;
    std::string user_prompt;
    AIPromptData() = default;

    AIPromptData(const std::string& sys, const std::string& master, const std::string& usr) 
        : system_prompt(sys), master_prompt(master), user_prompt(usr) {}
};

// Singleton framework for managing multiple AI instances
class AIFramework {
private:
    struct AIModelConfig {
        std::string model_path;
        bool has_custom_markers = false;
        bool log_generation = false;
        std::string marker_system_start;
        std::string marker_system_end;
        std::string marker_user_start;
        std::string marker_user_end;
        std::string marker_assistant_start;
        std::string marker_assistant_end;
        bool has_custom_parameters = false;
        int n_ctx = 32768;
        int max_new_tokens = 3072;
        float temperature = 0.3f;
        int top_k = 30;
        float top_p = 0.9f;
        int n_batch = 2048;
        int n_ubatch = 1024;
        int cpu_threads = 4;
        int cpu_threads_batch = 6;
        int gpu_layers = 999;
        bool flash_attention = true;
        bool offload_kv_cache = true;
    };

    std::unordered_map<std::string, AIModelConfig> ai_model_configs;
    mutable std::mutex model_configs_mutex;
    std::unordered_map<std::string, std::unique_ptr<AIController>> ai_instances;
    mutable std::mutex instances_mutex;
    std::unordered_map<std::string, AIPromptData> ai_prompts;
    mutable std::mutex prompts_mutex;
    std::string model_path;
    mutable std::mutex model_path_mutex;
    std::atomic<bool> initialized{false};
    
    // Private constructor for singleton
    AIFramework() = default;
    
public:
    // Get singleton instance
    static AIFramework& instance() {
        static AIFramework instance;
        return instance;
    }
    
    // Delete copy/move constructors
    AIFramework(const AIFramework&) = delete;
    AIFramework& operator=(const AIFramework&) = delete;
    AIFramework(AIFramework&&) = delete;
    AIFramework& operator=(AIFramework&&) = delete;

    static std::filesystem::path get_ai_models_path() {
        char docs_path[MAX_PATH];
        
        if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs_path) != S_OK) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: Failed to get Documents folder path");
            });

            throw std::runtime_error("Failed to get Documents folder path");
        }
        
        std::filesystem::path ai_models_path = std::filesystem::path(docs_path) / "Arma 3" / "kh_framework" / "ai_models";

        try {
            std::filesystem::create_directories(ai_models_path);
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - AI Framework: Failed to create AI models directory: " + error_msg);
            });

            throw std::runtime_error("Failed to create AI models directory: " + std::string(e.what()));
        }
        
        return ai_models_path;
    }

    static std::vector<std::filesystem::path> get_all_ai_model_paths() {
        std::vector<std::filesystem::path> paths;
        
        // Priority 1: Documents folder
        try {
            paths.push_back(get_ai_models_path());
        } catch (...) {
            // Documents folder unavailable, continue with mod folders
        }
        
        // Priority 2: Active mod folders
        try {
            auto mod_ai_dirs = AIModelDiscovery::find_all_ai_model_directories();
            paths.insert(paths.end(), mod_ai_dirs.begin(), mod_ai_dirs.end());
        } catch (...) {
            // Mod discovery failed, use only documents folder
        }
        
        return paths;
    }

    static std::string find_any_gguf_model() {
        try {
            std::string model_path = AIModelDiscovery::find_any_gguf_model();
            
            if (!model_path.empty()) {
                MainThreadScheduler::instance().schedule([model_path]() {
                    sqf::diag_log("KH - AI Framework: Model loaded successfully - " + model_path);
                });

                return model_path;
            }

            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - AI Framework: No .gguf model files found in any search location");
            });

            return "";
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - AI Framework: Error searching for model files: " + error_msg);
            });

            return "";
        }
    }

    void set_model_path(const std::string& filename) {
        try {
            // Search across all available locations
            std::string found_path = AIModelDiscovery::find_model_file(filename);
            
            if (found_path.empty()) {
                MainThreadScheduler::instance().schedule([filename]() {
                    report_error("KH - AI Framework: AI model file not found: " + filename);
                });

                return;
            }
            
            std::lock_guard<std::mutex> lock(model_path_mutex);
            model_path = found_path;

            MainThreadScheduler::instance().schedule([found_path]() {
                sqf::diag_log("KH - AI Framework: Model path set to: " + found_path);
            });
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - AI Framework: Error setting global model path - " + error_msg);
            });
        }
    }

    // Get global default model path
    std::string get_model_path() const {
        std::lock_guard<std::mutex> lock(model_path_mutex);
        return model_path;
    }

    bool set_ai_model_path(const std::string& ai_name, const std::string& filename) {
        try {
            std::string found_path = AIModelDiscovery::find_model_file(filename);
            
            if (found_path.empty()) {
                MainThreadScheduler::instance().schedule([filename]() {
                    report_error("KH - AI Framework: Model file not found: " + filename);
                });

                return false;
            }
            
            {
                std::lock_guard<std::mutex> lock(model_configs_mutex);
                ai_model_configs[ai_name].model_path = found_path;
            }
            
            MainThreadScheduler::instance().schedule([ai_name, found_path]() {
                sqf::diag_log("KH - AI Framework: AI (" + ai_name + ") model path set to: " + found_path);
            });

            return true;
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - AI Framework: Error setting AI model path - " + error_msg);
            });

            return false;
        }
    }

    // Get model path for specific AI (returns empty if using global default)
    std::string get_ai_model_path(const std::string& ai_name) const {
        std::lock_guard<std::mutex> lock(model_configs_mutex);
        auto it = ai_model_configs.find(ai_name);

        if (it != ai_model_configs.end() && !it->second.model_path.empty()) {
            return it->second.model_path;
        }

        return "";
    }
    
    // Set custom markers (for a SPECIFIC AI instance)
    bool set_ai_markers(const std::string& ai_name, 
                        const std::string& sys_start, const std::string& sys_end,
                        const std::string& usr_start, const std::string& usr_end,
                        const std::string& asst_start, const std::string& asst_end) {        
        {
            std::lock_guard<std::mutex> lock(model_configs_mutex);
            auto& config = ai_model_configs[ai_name];
            config.has_custom_markers = true;
            config.marker_system_start = sys_start;
            config.marker_system_end = sys_end;
            config.marker_user_start = usr_start;
            config.marker_user_end = usr_end;
            config.marker_assistant_start = asst_start;
            config.marker_assistant_end = asst_end;
        }

        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            auto it = ai_instances.find(ai_name);
            
            if (it != ai_instances.end()) {
                return it->second->set_markers(sys_start, sys_end, usr_start, usr_end, asst_start, asst_end);
            }
        }
        
        return true;
    }

    bool set_ai_parameters(const std::string& ai_name, int n_ctx, int max_new_tokens, 
                        float temperature, int top_k, float top_p, 
                        int n_batch, int n_ubatch, int cpu_threads, int cpu_threads_batch, 
                        int gpu_layers, bool flash_attention, bool offload_kv_cache) {
        {
            std::lock_guard<std::mutex> lock(model_configs_mutex);
            auto& config = ai_model_configs[ai_name];
            config.has_custom_parameters = true;
            config.n_ctx = n_ctx;
            config.max_new_tokens = max_new_tokens;
            config.temperature = temperature;
            config.top_k = top_k;
            config.top_p = top_p;
            config.n_batch = n_batch;
            config.n_ubatch = n_ubatch;
            config.cpu_threads = cpu_threads;
            config.cpu_threads_batch = cpu_threads_batch;
            config.gpu_layers = gpu_layers;
            config.flash_attention = flash_attention;
            config.offload_kv_cache = offload_kv_cache;
        }

        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            auto it = ai_instances.find(ai_name);
            
            if (it != ai_instances.end()) {
                return it->second->set_parameters(n_ctx, max_new_tokens, temperature, top_k, top_p, 
                                                n_batch, n_ubatch, cpu_threads, cpu_threads_batch, 
                                                gpu_layers, flash_attention, offload_kv_cache);
            }
        }

        return true;
    }

    bool set_ai_log_generation(const std::string& ai_name, bool enabled) {
        {
            std::lock_guard<std::mutex> lock(model_configs_mutex);
            auto& config = ai_model_configs[ai_name];
            config.log_generation = enabled;
        }

        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            auto it = ai_instances.find(ai_name);
            
            if (it != ai_instances.end()) {
                it->second->set_log_generation(enabled);
            }
        }
        
        return true;
    }

    bool trigger_ai_inference(const std::string& ai_name) {
        std::lock_guard<std::mutex> lock(instances_mutex);
        auto it = ai_instances.find(ai_name);
        
        if (it == ai_instances.end()) {
            MainThreadScheduler::instance().schedule([ai_name]() {
                report_error("KH - AI Framework: AI (" + ai_name + ") not found");
            });

            return false;
        }
        
        return it->second->trigger_inference();
    }

    bool initialize_ai(const std::string& ai_name) {        
        try {
            {
                std::lock_guard<std::mutex> lock(instances_mutex);

                if (ai_instances.find(ai_name) != ai_instances.end()) {
                    if (ai_instances[ai_name]->is_running()) {
                        return false;
                    }

                    ai_instances.erase(ai_name);
                }
            }

            std::string system_prompt, master_prompt, user_prompt;

            {
                std::lock_guard<std::mutex> lock(prompts_mutex);
                auto it = ai_prompts.find(ai_name);
                
                if (it != ai_prompts.end()) {
                    system_prompt = it->second.system_prompt;
                    master_prompt = it->second.master_prompt;
                    user_prompt = it->second.user_prompt;
                }
            }

            std::string current_model_path;

            {
                std::lock_guard<std::mutex> config_lock(model_configs_mutex);
                auto config_it = ai_model_configs.find(ai_name);

                if (config_it != ai_model_configs.end() && !config_it->second.model_path.empty()) {
                    current_model_path = config_it->second.model_path;
                }
            }
            
            // Fall back to global model path if no per-instance path set
            if (current_model_path.empty()) {
                current_model_path = get_model_path();
            }
            
            // Fall back to default if still empty
            if (current_model_path.empty()) {
                current_model_path = find_any_gguf_model();
                
                if (current_model_path.empty()) {
                    MainThreadScheduler::instance().schedule([]() {
                        report_error("KH - AI Framework: No model path set and no .gguf files found in AI models directory");
                    });

                    return false;
                }
                
                // Set as global default for future use (but don't validate again)
                {
                    std::lock_guard<std::mutex> lock(model_path_mutex);
                    model_path = current_model_path;
                }
            }
            
            if (current_model_path.empty()) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - AI Framework: No model path set");
                });

                return false;
            }
            
            // Create AI instance
            auto ai = std::make_unique<AIController>(ai_name);
            ai->update_system_prompt(system_prompt);
            ai->update_master_prompt(master_prompt);
            ai->update_user_prompt(user_prompt);

            {
                std::lock_guard<std::mutex> config_lock(model_configs_mutex);
                auto config_it = ai_model_configs.find(ai_name);
                
                if (config_it != ai_model_configs.end()) {
                    if (config_it->second.has_custom_markers) {
                        ai->set_markers(
                            config_it->second.marker_system_start,
                            config_it->second.marker_system_end,
                            config_it->second.marker_user_start,
                            config_it->second.marker_user_end,
                            config_it->second.marker_assistant_start,
                            config_it->second.marker_assistant_end
                        );
                    }

                    ai->set_log_generation(config_it->second.log_generation);

                    if (config_it->second.has_custom_parameters) {
                        ai->set_parameters(
                            config_it->second.n_ctx,
                            config_it->second.max_new_tokens,
                            config_it->second.temperature,
                            config_it->second.top_k,
                            config_it->second.top_p,
                            config_it->second.n_batch,
                            config_it->second.n_ubatch,
                            config_it->second.cpu_threads,
                            config_it->second.cpu_threads_batch,
                            config_it->second.gpu_layers,
                            config_it->second.flash_attention,
                            config_it->second.offload_kv_cache
                        );
                    }
                }
            }
            
            AIController* ai_ptr = ai.get();

            {
                std::lock_guard<std::mutex> lock(instances_mutex);
                ai_instances[ai_name] = std::move(ai);
                initialized = true;
            }

            if (!ai_ptr->start(current_model_path)) {
                {
                    std::lock_guard<std::mutex> lock(instances_mutex);
                    ai_instances.erase(ai_name);
                    
                    if (ai_instances.empty()) {
                        initialized = false;
                    }
                }
                
                MainThreadScheduler::instance().schedule([ai_name]() {
                    report_error("KH - AI Framework: Failed to initialize AI (" + ai_name + ")");
                });

                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([ai_name, error_msg]() {
                report_error("KH - AI Framework: Exception initializing AI (" + ai_name + "): " + error_msg);
            });

            return false;
        }
    }
    
    // Stop a specific AI instance
    bool stop_ai(const std::string& ai_name) {
        std::unique_ptr<AIController> ai_to_stop;
        
        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            auto it = ai_instances.find(ai_name);
            
            if (it == ai_instances.end()) {
                MainThreadScheduler::instance().schedule([ai_name]() {
                    report_error("KH - AI Framework: AI (" + ai_name + ") was not found");
                });

                return false;
            }

            ai_to_stop = std::move(it->second);
            ai_instances.erase(it);
            
            if (ai_instances.empty()) {
                initialized = false;
            }
        }

        ai_to_stop->stop();
        return true;
    }
    
    // Stop all AI instances
    void stop_all() {
        std::vector<std::unique_ptr<AIController>> ais_to_stop;
        
        {
            std::lock_guard<std::mutex> lock(instances_mutex);

            for (auto& [name, ai] : ai_instances) {
                ais_to_stop.push_back(std::move(ai));
            }
            
            ai_instances.clear();
            initialized = false;
        }
        
        for (auto& ai : ais_to_stop) {
            if (ai) {
                try {
                    ai->stop();
                } catch (...) {
                    // Continue cleanup even if one AI fails
                }
            }
        }

        ais_to_stop.clear();
        SharedModelManager::cleanup_all();

        {
            std::lock_guard<std::mutex> lock(prompts_mutex);
            ai_prompts.clear();
        }

        {
            std::lock_guard<std::mutex> lock(model_configs_mutex);
            ai_model_configs.clear();
        }
    }
    
    // Check if a specific AI is active
    bool is_ai_active(const std::string& ai_name) const {
        std::lock_guard<std::mutex> lock(instances_mutex);
        auto it = ai_instances.find(ai_name);

        if (it == ai_instances.end()) {
            return false;
        }
        
        return it->second->is_running();
    }
    
    bool is_ai_generating(const std::string& ai_name) const {
        std::lock_guard<std::mutex> lock(instances_mutex);
        auto it = ai_instances.find(ai_name);

        if (it == ai_instances.end()) {
            return false;
        }
        
        return it->second->is_generating_response();
    }

    std::vector<std::string> get_active_ai_names() const {
        std::lock_guard<std::mutex> lock(instances_mutex);
        
        std::vector<std::string> names;
        for (const auto& [name, ai] : ai_instances) {
            if (ai->is_running()) {
                names.push_back(name);
            }
        }
        
        return names;
    }

    bool abort_ai_generation(const std::string& ai_name) {
        std::lock_guard<std::mutex> lock(instances_mutex);
        auto it = ai_instances.find(ai_name);

        if (it == ai_instances.end()) {
            MainThreadScheduler::instance().schedule([ai_name]() {
                report_error("KH - AI Framework: AI (" + ai_name + ") was not found");
            });

            return false;
        }
        
        it->second->abort_current_generation();
        return true;
    }

    bool update_system_prompt(const std::string& ai_name, const std::string& prompt) {
        {
            std::lock_guard<std::mutex> lock(prompts_mutex);
            auto& prompt_data = ai_prompts[ai_name];
            prompt_data.system_prompt = prompt;
        }
 
        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            auto it = ai_instances.find(ai_name);

            if (it != ai_instances.end()) {
                it->second->update_system_prompt(prompt);
            }
        }
        
        return true;
    }

    bool update_master_prompt(const std::string& ai_name, const std::string& prompt) {
        {
            std::lock_guard<std::mutex> lock(prompts_mutex);
            auto& prompt_data = ai_prompts[ai_name];
            prompt_data.master_prompt = prompt;
        }

        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            auto it = ai_instances.find(ai_name);

            if (it != ai_instances.end()) {
                it->second->update_master_prompt(prompt);
            }
        }
        
        return true;
    }

    bool update_user_prompt(const std::string& ai_name, const std::string& prompt) {
        {
            std::lock_guard<std::mutex> lock(prompts_mutex);
            auto& prompt_data = ai_prompts[ai_name];
            prompt_data.user_prompt = prompt;
        }

        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            auto it = ai_instances.find(ai_name);

            if (it != ai_instances.end()) {
                it->second->update_user_prompt(prompt);
            }
        }
        
        return true;
    }

    bool reset_ai_context(const std::string& ai_name) {
        std::lock_guard<std::mutex> lock(instances_mutex);
        auto it = ai_instances.find(ai_name);

        if (it == ai_instances.end()) {
            MainThreadScheduler::instance().schedule([ai_name]() {
                report_error("KH - AI Framework: AI (" + ai_name + ") was not found");
            });

            return false;
        }
        
        it->second->reset_conversation();
        return true;
    }

    bool is_initialized() const {
        return initialized;
    }
};