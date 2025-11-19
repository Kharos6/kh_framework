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

// Static member definitions
std::atomic<int> LlamaBackend::ref_count{0};
std::mutex LlamaBackend::backend_mutex;

class AIController {
private:
    std::string ai_name;
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    const llama_vocab* vocab = nullptr;
    llama_sampler* sampler = nullptr;
    bool initialized{false};
    std::vector<llama_token> system_prompt_tokens;
    int system_prompt_token_count = 0;
    std::atomic<bool> system_prompt_cached{false};
    std::atomic<bool> force_terminate{false};  // Add this new member
    
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
    int CPU_THREADS_BATCH = 8;
    int GPU_LAYERS = g_cuda_available ? 999 : 0;
    bool FLASH_ATTENTION = g_cuda_available;
    bool OFFLOAD_KV_CACHE = g_cuda_available;
    std::atomic<bool> running{false};
    std::atomic<bool> should_stop{false};
    std::thread ai_thread;
    
    // Prompts
    std::string system_prompt;
    std::string user_prompt;
    std::string assistant_notes; // Stores notes from previous inference
    mutable std::mutex prompt_mutex;
    mutable std::mutex lua_execution_mutex;  // For thread-safe lua execution
    std::condition_variable inference_trigger;
    std::mutex inference_mutex;
    std::atomic<bool> inference_requested{false};
    std::string marker_system_start = "<|begin_of_text|><|start_header_id|>system<|end_header_id|>";
    std::string marker_system_end = "<|eot_id|>";
    std::string marker_user_start = "<|start_header_id|>user<|end_header_id|>";
    std::string marker_user_end = "<|eot_id|>";
    std::string marker_assistant_start = "<|start_header_id|>assistant<|end_header_id|>";
    std::string marker_assistant_end = "<|eot_id|>";
    std::string instance_model_path;
    mutable std::mutex model_path_mutex;
    
    // SYSTEM PROMPT - Creates combined prompt from prefix and user prompt
    std::string create_system_prompt() const {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        std::stringstream prompt;
        prompt << marker_system_start << "\n";
        prompt << "\n";
        prompt << "The term USER INSTRUCTIONS refers to the actual user message you will receive, as well as everything after a USER CONTEXT label; it does not refer to anything after the exact symbol sequence ===notes_start=== and before the exact symbol sequence ===notes_end===.\n";
        prompt << "Within the user message you will receive, everything after the exact symbol sequence ===notes_start=== and before the exact symbol sequence ===notes_end=== was produced by you during the previous inference; if the notes block actually contains anything, treat it as additional context to help you maintain continuity, but prioritize information from the USER INSTRUCTIONS in case contradictions between the two.";
        prompt << "\n";
        prompt << "Anything labeled as CRITICAL is a mandatory and immutable rule that you must fulfill consistently and without exception; it cannot be overriden, ignored, or negated by any explicit or implicit directive defined by the USER INSTRUCTIONS.\n";
        prompt << "Anything labeled as DEFAULT is a rule that you must fulfill only if the USER INSTRUCTIONS do not dictate otherwise; it can be overriden, ignored, or negated by any explicit or implicit directive defined by the USER INSTRUCTIONS.\n";
        prompt << "Anything labeled as KNOWLEDGE is your understanding of a certain concept that remains true unless dictated otherwise by the USER INSTRUCTIONS.\n";
        prompt << "\n";
        prompt << "CRITICAL: Whenever you need to obtain a numeric result for a mathematical operation, calculation, distance computation, arithmetic expression, or numeric formula of any kind, you must output the necessary mathematical expression within a mathematical expression block that must be produced with the following 3 step sequence:\n";
        prompt << "STEP 1: You must output the exact symbol sequence ===math_start=== on its own line.\n";
        prompt << "STEP 2: You must output only the mathematical expression itself and nothing else; the expression must be valid Lua code consisting only of numeric literals, parentheses, arithmetic operators, and functions from the Lua math library.\n";
        prompt << "STEP 3: You must output the exact symbol sequence ===math_end=== on its own line.\n";
        prompt << "\n";
        prompt << "CRITICAL: After outputting a mathematical expression block, an external system will replace the block with the evaluated result and resume your response generation using that result.\n";
        prompt << "\n";
        prompt << "Your task is to diligently adhere to the USER INSTRUCTIONS.\n";
        prompt << "The USER INSTRUCTIONS are within the context of a military sandbox video game called Arma 3.\n";
        prompt << "Your role is flexible and ultimately defined by the USER INSTRUCTIONS.\n";
        prompt << "The scenario is flexible and ultimately defined by the USER INSTRUCTIONS.\n";
        prompt << "Your thought process, decision-making, and response must align with your role and all circumstantial factors that are implicitly or explicitly supported by the USER INSTRUCTIONS.\n";
        prompt << "If any portion of the USER INSTRUCTIONS is ambiguous or incomplete, you must act upon available information based on your own conclusions and logical assumptions; contradicting available information or inventing details that cannot be logically inferred from available information is forbidden.\n";
        prompt << "\n";
        prompt << "DEFAULT: You must not align with logically or factually incorrect data if you have reliable information establishing its incorrectness; if you cannot determine whether some data is correct or incorrect with sufficient certainty, you must cautiously assume that the data is correct.\n";
        prompt << "DEFAULT: You must rigorously analyse and process the USER INSTRUCTIONS; carefully consider minute details, explicit requirements, implicit intent, contextual nuances, and desired outcomes.\n";
        prompt << "DEFAULT: You must be creative while remaining consistent with the information provided by the USER INSTRUCTIONS; exercise innovations and unconventional solutions that are logically supported by the USER INSTRUCTIONS.\n";
        prompt << "DEFAULT: Your response must be concise and contain only the level of verbosity that is necessary to satisfy the directive defined by the USER INSTRUCTIONS without being excessively long.\n";
        prompt << "DEFAULT: At the end of your response, you must output a concise notes block that will serve as the content of the ASSISTANT CONTEXT label during the next inference; it must be produced with the following 3 step sequence:\n";
        prompt << "STEP 1: You must output the exact symbol sequence ===notes_start=== on its own line.\n";
        prompt << "STEP 2: You must output concise notes that summarise only the most important information, facts, and decisions from your response. Format them in accordance with the context and in a way that you can easily process them during future inferences.\n";
        prompt << "STEP 3: You must output the exact symbol sequence ===notes_end=== on its own line.\n";
        prompt << "\n";
        prompt << "KNOWLEDGE: Any array that is referenced as a position is a vector that represents position using the [X, Y, Z] format, and is relative to the world unless dictated otherwise by the USER INSTRUCTIONS; negative X is left or West, positive X is right or East, negative Y is back or South, positive Y is front or North, negative Z is down, and positive Z is up, with the Z value being relative to the sea level unless dictated otherwise by the USER INSTRUCTIONS.\n";
        prompt << "KNOWLEDGE: Any array that is referenced as a rotation is a vector that represents rotation using the euler [X, Y, Z] format from 0 to 360 degrees, and is relative to the world unless dictated otherwise by the USER INSTRUCTIONS; X is pitch, Y is bank, and Z is yaw.\n";
        prompt << "KNOWLEDGE: Anything referenced as a direction is a value from 0 to 360 that represents compass direction; 0 or 360 is North, 180 is South, 90 is East, 270 is West.\n";
        prompt << "KNOWLEDGE: Any array that is referenced as velocity is a vector that represents positional speed in metres per second using the [X, Y, Z] format, and is relative to the world unless dictated otherwise by the USER INSTRUCTIONS; negative X is left or West, positive X is right or East, negative Y is back or South, positive Y is front or North, negative Z is down, and positive Z is up.\n";
        prompt << "KNOWLEDGE: Any array that is referenced as angular velocity is a vector that represents rotational speed in degrees per second using the [X, Y, Z] format, and is relative to the world unless dictated otherwise by the USER INSTRUCTIONS; negative X is left or West, positive X is right or East, negative Y is back or South, positive Y is front or North, negative Z is down, and positive Z is up.\n";
        prompt << "KNOWLEDGE: Anything defined as a unit is an entity that is directly controlled by an AI or by a player.\n";
        prompt << "KNOWLEDGE: Anything defined as an object is an environmental entity, like a static structure or a prop affected by physics, that cannot be directly or indirectly controlled by an AI or by a player.\n";
        prompt << "KNOWLEDGE: Anything defined as a vehicle is an entity similar to an object, but can be entered and indirectly controlled by one or multiple units.\n";
        prompt << "KNOWLEDGE: Anything defined as a group is an organized collection of units; groups may sometimes be empty, but units always belong to a group.\n";
        prompt << "KNOWLEDGE: Anything defined as a side represents the affiliation of units and groups; sides have relations that dictate whether units of different sides are friendly, neutral, or hostile towards each other.\n";
        prompt << "KNOWLEDGE: The overall measurement system used is the metric system, time is in the 24-hour format, dates are in the day/month/year format, and speed or velocity is measured in metres per second.\n";
        prompt << "\n";
        prompt << "USER CONTEXT:\n";
        prompt << system_prompt;
        prompt << "\n";
        prompt << "\n";
        prompt << marker_system_end;
        return prompt.str();
    }
    
    // USER PROMPT - Creates the current situation prompt
    std::string create_user_prompt() const {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        std::stringstream prompt;
        prompt << marker_user_start << "\n";
        prompt << "\n";
        prompt << "===notes_start===\n";
        prompt << assistant_notes;
        prompt << "===notes_end===\n";
        prompt << "\n";
        prompt << user_prompt;
        prompt << marker_user_end << marker_assistant_start << "\n";
        prompt << "\n";
        return prompt.str();
    }

    void cleanup_resources() {
        // Clean up in reverse order of initialization
        if (sampler) {
            try {
                llama_sampler_free(sampler);
            } catch (...) {
                // Ignore errors during cleanup
            }

            sampler = nullptr;
        }
        
        if (ctx) {
            try {
                llama_free(ctx);
            } catch (...) {
                // Ignore errors during cleanup
            }

            ctx = nullptr;
        }
        
        if (model) {
            try {
                llama_free_model(model);
            } catch (...) {
                // Ignore errors during cleanup
            }

            model = nullptr;
        }
        
        vocab = nullptr;
        
        if (initialized) {
            try {
                LlamaBackend::cleanup();
            } catch (...) {
                // Ignore errors during cleanup
            }
            
            initialized = false;
        }
        
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
    }
    
    std::vector<llama_token> tokenize_with_chunking(const std::string& text) const {
        try {
            std::vector<llama_token> tokens = common_tokenize(ctx, text, true, true);
            
            if (tokens.size() > MAX_PROMPT_TOKENS) {
                intercept::client::invoker_lock lock;

                sqf::diag_log("WARNING: Prompt truncated from " + 
                             std::to_string(tokens.size()) + " to " + 
                             std::to_string(MAX_PROMPT_TOKENS) + " tokens");

                tokens.resize(MAX_PROMPT_TOKENS);
            }
            
            return tokens;
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Tokenization error: " + std::string(e.what()));
            throw;
        }
    }
    
    // Cache system prompt tokens
    void cache_system_prompt() {
        // Always regenerate when this is called
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        
        try {
            // Tokenize system prompt with current content
            auto start = std::chrono::high_resolution_clock::now();
            std::string sys_prompt = create_system_prompt();
            system_prompt_tokens = tokenize_with_chunking(sys_prompt);
            system_prompt_token_count = static_cast<int>(system_prompt_tokens.size());
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // Decode system prompt into KV cache (in batches)
            start = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < system_prompt_tokens.size(); i += N_BATCH) {
                size_t batch_size = std::min(static_cast<size_t>(N_BATCH), system_prompt_tokens.size() - i);
                llama_batch batch = llama_batch_get_one(&system_prompt_tokens[i], static_cast<int32_t>(batch_size));
                int result = llama_decode(ctx, batch);

                if (result != 0) {
                    intercept::client::invoker_lock lock;
                    report_error("KH - AI Framework: System prompt decode failed");
                }
            }

            end = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);            
            system_prompt_cached = true;
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI Controller (" + ai_name + "): Failed to cache system prompt: " + std::string(e.what()));
        }
    }

    std::unique_ptr<sol::state> create_basic_lua_state() {
        // Create a new lua state with basic libraries
        auto lua_copy = std::make_unique<sol::state>();
        lua_copy->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
        return lua_copy;
    }
    
    // Execute lua code and return result as string
    std::string execute_lua_block(const std::string& lua_code) {
        std::lock_guard<std::mutex> lock(lua_execution_mutex);
        
        try {
            auto lua_state = create_basic_lua_state();
            auto result = lua_state->script(lua_code);
            
            // Convert result to string
            if (result.valid()) {
                sol::object obj = result;
                
                if (obj.is<double>()) {
                    double num = obj.as<double>();
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(6) << num;
                    std::string str = oss.str();
                    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                    if (str.back() == '.') str.pop_back();
                    return str;
                } else if (obj.is<int>()) {
                    return std::to_string(obj.as<int>());
                } else if (obj.is<bool>()) {
                    return obj.as<bool>() ? "true" : "false";
                } else if (obj.is<std::string>()) {
                    return obj.as<std::string>();
                } else {
                    return "nil";
                }
            } else {
                sol::error err = result;
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: Lua execution error: " + std::string(err.what()));
                return "[ERROR: " + std::string(err.what()) + "]";
            }
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Lua block execution exception: " + std::string(e.what()));
            return "[ERROR: " + std::string(e.what()) + "]";
        }
    }
    
    // Process expression blocks in real-time during generation
    std::string process_expression_block(const std::string& response_so_far, const std::string& new_token) {
        static const std::string MATH_START = "===math_start===";
        static const std::string MATH_END = "===math_end===";
        std::string updated_response = response_so_far + new_token;
        size_t math_start_pos = updated_response.rfind(MATH_START);

        if (math_start_pos != std::string::npos) {
            size_t math_end_pos = updated_response.find(MATH_END, math_start_pos);

            if (math_end_pos != std::string::npos) {
                size_t code_start = math_start_pos + MATH_START.length();
                std::string math_code = updated_response.substr(code_start, math_end_pos - code_start);
                math_code.erase(0, math_code.find_first_not_of(" \n\r\t"));
                math_code.erase(math_code.find_last_not_of(" \n\r\t") + 1);
                
                // Execute and get result
                std::string result = "return " + math_code;
                std::string evaluated = execute_lua_block(result);
                
                // Replace the entire block with the result
                size_t block_end = math_end_pos + MATH_END.length();
                updated_response = updated_response.substr(0, math_start_pos) + evaluated + updated_response.substr(block_end);
                return updated_response;
            }
        }
        
        return updated_response;
    }
    
    // Extract notes from response
    void extract_and_store_notes(const std::string& response) {
        static const std::string NOTES_START = "===notes_start===";
        static const std::string NOTES_END = "===notes_end===";
        size_t notes_start_pos = response.find(NOTES_START);

        if (notes_start_pos != std::string::npos) {
            size_t notes_end_pos = response.find(NOTES_END, notes_start_pos);

            if (notes_end_pos != std::string::npos) {
                // Extract the entire notes block, including the markers
                size_t block_end = notes_end_pos + NOTES_END.length();
                std::string notes_block = response.substr(notes_start_pos, block_end - notes_start_pos);
                std::lock_guard<std::mutex> lock(prompt_mutex);
                assistant_notes = notes_block;
            }
        } else {
            // No notes found, clear previous notes
            std::lock_guard<std::mutex> lock(prompt_mutex);
            assistant_notes.clear();
        }
    }

    std::string generate_response() {
        {
            std::lock_guard<std::mutex> lock(prompt_mutex);
            if (user_prompt.empty()) {
                // Return empty string if no user prompt (saves resources)
                return "";
            }
        }
        
        if (!ctx || !model) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Context or model not initialized");
            throw std::runtime_error("Context or model not initialized");
        }

        // Ensure system prompt is cached
        if (!system_prompt_cached) {
            cache_system_prompt();
        }

        llama_batch batch;
        int decode_result = 0;
        llama_memory_t kv_memory = llama_get_memory(ctx);
        llama_memory_seq_rm(kv_memory, -1, system_prompt_token_count, -1);
        llama_sampler_reset(sampler);

        // Tokenize user prompt only
        std::string user_text = create_user_prompt();
        auto start_tokenize = std::chrono::high_resolution_clock::now();
        std::vector<llama_token> user_tokens = tokenize_with_chunking(user_text);
        auto end_tokenize = std::chrono::high_resolution_clock::now();
        auto tokenize_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_tokenize - start_tokenize);
        int user_token_count = static_cast<int>(user_tokens.size());
        int total_token_count = system_prompt_token_count + user_token_count;
        
        if (user_tokens.empty()) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: User tokenization produced empty result");
            throw std::runtime_error("User tokenization produced empty result");
        }
        
        // Safety check
        if (total_token_count > MAX_PROMPT_TOKENS) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Prompt too large");
            throw std::runtime_error("Prompt too large");
        }
        
        auto start_decode = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < user_tokens.size(); i += N_BATCH) {
            size_t batch_size = std::min(static_cast<size_t>(N_BATCH), user_tokens.size() - i);
            batch = llama_batch_get_one(&user_tokens[i], static_cast<int32_t>(batch_size));
            decode_result = llama_decode(ctx, batch);
            
            if (decode_result != 0) {
                intercept::client::invoker_lock lock;

                report_error("KH - AI Framework: ERROR: User decode failed at chunk " + 
                             std::to_string(i / N_BATCH) + " with code " + 
                             std::to_string(decode_result));

                throw std::runtime_error("User decode failed");
            }
        }
        
        auto end_decode = std::chrono::high_resolution_clock::now();
        auto decode_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_decode - start_decode);
        
        // Generate response with expression block processing
        std::string response;
        std::string raw_response;  // Accumulates tokens before processing
        int n_generated = 0;
        auto start_gen = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < MAX_NEW_TOKENS; i++) {
            if (should_stop || force_terminate) break;
            llama_token new_token_id = llama_sampler_sample(sampler, ctx, -1);
            
            if (llama_token_is_eog(vocab, new_token_id)) {
                break;
            }
            
            // Convert token to text
            char token_buffer[256];
            int n_chars = llama_token_to_piece(vocab, new_token_id, token_buffer, sizeof(token_buffer), 0, true);
            
            if (n_chars < 0) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: ERROR: Token to piece conversion failed");
                break;
            }
            
            std::string token_str(token_buffer, n_chars);
            raw_response += token_str;
            response = process_expression_block(response, token_str);
            
            // Prepare next batch
            batch = llama_batch_get_one(&new_token_id, 1);
            
            if (batch.n_tokens != 1) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: ERROR: Batch creation failed");
                break;
            }
            
            n_generated++;
            
            // Decode next token
            try {
                decode_result = llama_decode(ctx, batch);
            } catch (const std::exception& e) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: EXCEPTION during generation: " + std::string(e.what()));
                break;
            }
            
            if (decode_result != 0) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: Generation decode failed at token " + std::to_string(n_generated));
                break;
            }

            try {
                intercept::client::invoker_lock lock;
                auto_array<game_value> ai_response_progress_data;
                ai_response_progress_data.push_back(game_value(ai_name));
                ai_response_progress_data.push_back(game_value(raw_response));
                sqf::call2(g_compiled_ai_response_progress_event, game_value(std::move(ai_response_progress_data)));
            } catch (const std::exception& e) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: AI Controller (" + ai_name + "): Error calling response progress event: " + std::string(e.what()));
            }
        }
        
        auto end_gen = std::chrono::high_resolution_clock::now();
        auto gen_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_gen - start_gen);
        
        // Extract and store notes before cleaning response
        extract_and_store_notes(response);
        
        // Clean up response (remove end marker and notes block)
        size_t end_pos = response.find(marker_assistant_end);

        if (end_pos != std::string::npos) {
            response = response.substr(0, end_pos);
        }
        
        // Remove notes block from the response shown to user
        size_t notes_start_pos = response.find("===notes_start===");

        if (notes_start_pos != std::string::npos) {
            size_t notes_end_pos = response.find("===notes_end===", notes_start_pos);

            if (notes_end_pos != std::string::npos) {
                size_t block_end = notes_end_pos + std::string("===notes_end===").length();
                response = response.substr(0, notes_start_pos) + response.substr(block_end);
            }
        }

        response.erase(0, response.find_first_not_of(" \n\r\t"));
        response.erase(response.find_last_not_of(" \n\r\t") + 1);
        return response;
    }
    
    // AI processing thread
    void ai_thread_func(const std::string& model_path) {
        try {
            initialize_internal(model_path);
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI Controller (" + ai_name + "): Failed to initialize: " + std::string(e.what()));
            running = false;
            return;
        }
        
        // Main processing loop - waits for inference triggers
        while (!should_stop && !force_terminate) {
            try {
                if (force_terminate) {
                    intercept::client::invoker_lock lock;
                    sqf::diag_log("KH - AI Framework: AI thread '" + ai_name + "' received force termination signal");
                    break;
                }

                // Wait for inference trigger
                {
                    std::unique_lock<std::mutex> lock(inference_mutex);

                    inference_trigger.wait(lock, [this] { 
                        return inference_requested.load() || should_stop.load() || force_terminate.load();
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
                std::string raw_response = generate_response();
                if (force_terminate) break;
                
                // Trigger events for the response
                try {
                    intercept::client::invoker_lock lock;
                    auto_array<game_value> ai_response_data;
                    ai_response_data.push_back(game_value(ai_name));
                    ai_response_data.push_back(game_value(raw_response));
                    sqf::call2(g_compiled_ai_response_event, game_value(std::move(ai_response_data)));
                } catch (const std::exception& e) {
                    intercept::client::invoker_lock lock;
                    report_error("KH - AI Framework: AI Controller (" + ai_name + "): Error calling response event: " + std::string(e.what()));
                }

                // Clear user prompt after processing (one-shot behavior)
                {
                    std::lock_guard<std::mutex> lock(prompt_mutex);
                    user_prompt.clear();
                }
            } catch (const std::exception& e) {
                {
                    std::lock_guard<std::mutex> lock(prompt_mutex);
                    user_prompt.clear();
                }

                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: AI Controller (" + ai_name + "): Thread error: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    void initialize_internal(const std::string& model_path) {        
        try {
            LlamaBackend::init();           
            llama_model_params model_params = llama_model_default_params();

            if (g_cuda_available) {
                model_params.n_gpu_layers = GPU_LAYERS;  // Offload all layers to GPU
                intercept::client::invoker_lock lock;
                sqf::diag_log("KH - AI Framework: using CUDA");
            } else {
                model_params.n_gpu_layers = 0;  // CPU-only mode
                intercept::client::invoker_lock lock;
                sqf::diag_log("KH - AI Framework: using CPU - SLOW");
            }

            model_params.use_mmap = true;
            model_params.use_mlock = false;            
            model = llama_load_model_from_file(model_path.c_str(), model_params);

            if (!model) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: Failed to load model");
                throw std::runtime_error("Failed to load model");
            }
            
            vocab = llama_model_get_vocab(model);

            if (!vocab) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: Failed to get vocabulary");
                throw std::runtime_error("Failed to get vocabulary");
            }

            llama_context_params ctx_params = llama_context_default_params();
            ctx_params.n_ctx = N_CTX;
            ctx_params.n_batch = N_BATCH;
            ctx_params.n_ubatch = N_UBATCH;
            ctx_params.n_threads = CPU_THREADS;
            ctx_params.n_threads_batch = CPU_THREADS_BATCH;
            ctx_params.flash_attn_type = g_cuda_available ? (FLASH_ATTENTION ? LLAMA_FLASH_ATTN_TYPE_ENABLED : LLAMA_FLASH_ATTN_TYPE_DISABLED) : LLAMA_FLASH_ATTN_TYPE_DISABLED;
            ctx_params.offload_kqv = g_cuda_available ? OFFLOAD_KV_CACHE : false;

            ctx = llama_new_context_with_model(model, ctx_params);

            if (!ctx) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: Failed to create context");
                throw std::runtime_error("Failed to create context");
            }

            auto sampler_params = llama_sampler_chain_default_params();
            sampler_params.no_perf = false;
            sampler = llama_sampler_chain_init(sampler_params);
            llama_sampler_chain_add(sampler, llama_sampler_init_top_k(TOP_K));
            llama_sampler_chain_add(sampler, llama_sampler_init_top_p(TOP_P, 1));
            llama_sampler_chain_add(sampler, llama_sampler_init_temp(TEMPERATURE));
            llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
            initialized = true;            
        } catch (const std::exception& e) {
            cleanup_resources();
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI Controller (" + ai_name + "): Initialization failed: " + std::string(e.what()));
            std::string error_msg = e.what();
            
            if (error_msg.find("CUDA") != std::string::npos || 
                error_msg.find("cublas") != std::string::npos ||
                error_msg.find("GPU") != std::string::npos) {
                report_error("KH - AI Framework: This may be a CUDA-related error. Ensure CUDA Toolkit 12.x is installed.");
            }
            
            throw;
        } catch (...) {
            cleanup_resources();
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI Controller (" + ai_name + "): FAILED: Unknown error");
            report_error("KH - AI Framework: CUDA libraries not found or failed to load.");
            throw;
        }
    }
    
public:
    AIController(const std::string& name) : ai_name(name) {}
    
    ~AIController() {
        try {
            // Ensure AI is stopped
            if (running.load()) {
                stop();
            }
            
            // Wait for thread to finish if it's joinable
            if (ai_thread.joinable()) {
                ai_thread.join();
            }
        } catch (const std::exception& e) {
            // Log but don't throw from destructor
            report_error("KH - AI Framework: Exception in ~AIController: " + 
                        std::string(e.what()));
        } catch (...) {
            report_error("KH - AI Framework: Unknown exception in ~AIController");
        }
    }
    
    // Delete copy/move constructors
    AIController(const AIController&) = delete;
    AIController& operator=(const AIController&) = delete;
    AIController(AIController&&) = delete;
    AIController& operator=(AIController&&) = delete;
    
    // Start AI processing
    bool start(const std::string& model_path) {
        if (running) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI Controller (" + ai_name + "): Already running");
            return false;
        }
        
        // Use instance-specific model path if set, otherwise use provided path
        std::string actual_model_path;

        {
            std::lock_guard<std::mutex> lock(model_path_mutex);
            actual_model_path = instance_model_path.empty() ? model_path : instance_model_path;
        }
        
        if (actual_model_path.empty()) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI Controller (" + ai_name + "): No model path specified");
            return false;
        }
        
        should_stop = false;
        running = true;
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        ai_thread = std::thread(&AIController::ai_thread_func, this, actual_model_path);
        return true;
    }
    
    // Stop AI processing
    void stop() {
        if (!running) {
            return;
        }
        
        should_stop = true;
        running = false;
        
        // Wake up thread if it's waiting
        {
            std::lock_guard<std::mutex> lock(inference_mutex);
            inference_requested = true;
        }

        inference_trigger.notify_one();
        
        // Wait for thread with timeout - using shared_ptr to avoid use-after-scope
        if (ai_thread.joinable()) {
            // First attempt: polite shutdown (5 seconds)
            auto start = std::chrono::steady_clock::now();
            bool joined = false;
            
            while (!joined && std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
                if (ai_thread.joinable()) {
                    // Try non-blocking check by attempting join with timeout simulation
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                } else {
                    joined = true;
                    break;
                }
            }
            
            // If still not joined, try force termination
            if (!joined && ai_thread.joinable()) {
                intercept::client::invoker_lock lock;
                sqf::diag_log("KH - AI Framework: AI thread '" + ai_name + "' not responding, forcing termination");
                force_terminate = true;
                inference_trigger.notify_all();
                start = std::chrono::steady_clock::now();

                while (ai_thread.joinable() && 
                    std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                if (ai_thread.joinable()) {
                    try {
                        ai_thread.join();
                        joined = true;
                    } catch (...) {
                        // If join fails, this is a critical error
                        report_error("KH - AI Framework: CRITICAL - AI thread '" + ai_name + "' hung - forcing detach (may cause instability)");
                        // Detach as last resort - but we've done everything we can
                        ai_thread.detach();
                    }
                }
            } else if (ai_thread.joinable()) {
                // Normal case - thread stopped cleanly
                ai_thread.join();
            }
        }
            
        // Clean up llama resources
        if (sampler) {
            llama_sampler_free(sampler);
            sampler = nullptr;
        }
        
        if (ctx) {
            llama_free(ctx);
            ctx = nullptr;
        }
        
        if (model) {
            llama_free_model(model);
            model = nullptr;
        }
        
        vocab = nullptr;
        
        if (initialized) {
            LlamaBackend::cleanup();
            initialized = false;
        }
        
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        force_terminate = false;
    }

    void clear_notes() {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        assistant_notes.clear();
    }

    void set_model_path(const std::string& path) {
        std::lock_guard<std::mutex> lock(model_path_mutex);
        instance_model_path = path;
    }
    
    void update_system_prompt(const std::string& prompt) {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        system_prompt = prompt;
        
        // Invalidate cache
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
    }
    
    void update_user_prompt(const std::string& prompt) {
        std::lock_guard<std::mutex> lock(prompt_mutex);
        user_prompt = prompt;
    }
    
    // Check if running
    bool is_running() const {
        return running;
    }
    
    // Get AI name
    std::string get_name() const {
        return ai_name;
    }
    
    // Set AI parameters (must be called before start() or while stopped)
    bool set_parameters(int n_ctx, int max_new_tokens, float temperature, 
                    int top_k, float top_p, int n_batch, int n_ubatch, int cpu_threads, int cpu_threads_batch, int gpu_layers, bool flash_attention, bool offload_kv_cache) {
        if (running) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Cannot change parameters while AI is running. Stop the AI first.");
            return false;
        }
        
        // Validate parameters
        if (n_ctx < 512) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: N_CTX must be at least 512");
            return false;
        }
        
        if (max_new_tokens < 1) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: MAX_NEW_TOKENS must be at least 1");
            return false;
        }
        
        if (temperature < 0.0f || temperature > 2.0f) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: TEMPERATURE must be between 0.0 and 2.0");
            return false;
        }
        
        if (top_k < 1 || top_k > 1000) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: TOP_K must be between 1 and 1000");
            return false;
        }
        
        if (top_p < 0.0f || top_p > 1.0f) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: TOP_P must be between 0.0 and 1.0");
            return false;
        }
        
        if (n_batch < 1) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: N_BATCH must be at least 1");
            return false;
        }
        
        if (n_ubatch < 1) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: N_UBATCH must be at least 1");
            return false;
        }
        
        if (cpu_threads < 1) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: CPU_THREADS must be greater than 1");
            return false;
        }

        if (cpu_threads_batch < 1) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: CPU_THREADS_BATCH must be greater than 1");
            return false;
        }
        
        // Check that context window can fit the tokens
        if (n_ctx < SAFETY_MARGIN + max_new_tokens + 512) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: N_CTX too small for MAX_NEW_TOKENS. Need at least " + std::to_string(SAFETY_MARGIN + max_new_tokens + 512));
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
        FLASH_ATTENTION = flash_attention ? LLAMA_FLASH_ATTN_TYPE_ENABLED : LLAMA_FLASH_ATTN_TYPE_DISABLED;
        system_prompt_cached = false;
        system_prompt_tokens.clear();
        system_prompt_token_count = 0;
        return true;
    }

    bool trigger_inference() {
        if (!running) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI '" + ai_name + "' is not running");
            return false;
        }

        bool has_prompt = false;

        {
            std::lock_guard<std::mutex> lock(prompt_mutex);
            has_prompt = !user_prompt.empty();
        }
        
        if (!has_prompt) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI '" + ai_name + "' has no user prompt set");
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(inference_mutex);
            inference_requested = true;
        }

        inference_trigger.notify_one();
        return true;
    }

    bool set_markers(const std::string& sys_start, const std::string& sys_end,
                     const std::string& usr_start, const std::string& usr_end,
                     const std::string& asst_start, const std::string& asst_end) {
        if (running) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Cannot change markers while AI is running. Stop the AI first.");
            return false;
        }
        
        // Validate markers are not empty
        if (sys_start.empty() || sys_end.empty() || 
            usr_start.empty() || usr_end.empty() ||
            asst_start.empty() || asst_end.empty()) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Prompt markers cannot be empty");
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
        return true;
    }

    bool is_initialized() const {
        return initialized;
    }
};

// Structure to hold AI prompts
struct AIPromptData {
    std::string system_prompt;
    std::string user_prompt;
    AIPromptData() = default;

    AIPromptData(const std::string& sys, const std::string& usr) 
        : system_prompt(sys), user_prompt(usr) {}
};

// Singleton framework for managing multiple AI instances
class AIFramework {
private:
    struct AIModelConfig {
        std::string model_path;
        bool has_custom_markers = false;
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
        int cpu_threads_batch = 8;
        int gpu_layers = 999;  // Will be adjusted based on CUDA availability
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
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Failed to get Documents folder path");
            throw std::runtime_error("Failed to get Documents folder path");
        }
        
        std::filesystem::path ai_models_path = std::filesystem::path(docs_path) / "Arma 3" / "kh_framework" / "ai_models";
        
        // Create directory if it doesn't exist
        try {
            std::filesystem::create_directories(ai_models_path);
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Failed to create AI models directory: " + std::string(e.what()));
            throw std::runtime_error("Failed to create AI models directory: " + std::string(e.what()));
        }
        
        return ai_models_path;
    }

    void set_model_path(const std::string& filename) {
        try {
            std::filesystem::path ai_models_dir = get_ai_models_path();
            std::filesystem::path full_path = ai_models_dir / filename;

            if (!std::filesystem::exists(full_path)) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: AI model file not found: " + full_path.string());
                return;
            }
            
            std::lock_guard<std::mutex> lock(model_path_mutex);
            model_path = full_path.string();
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Error setting global model path - " + std::string(e.what()));
        }
    }

    // Get global default model path
    std::string get_model_path() const {
        std::lock_guard<std::mutex> lock(model_path_mutex);
        return model_path;
    }

    bool set_ai_model_path(const std::string& ai_name, const std::string& filename) {
        try {
            std::filesystem::path ai_models_dir = get_ai_models_path();
            std::filesystem::path full_path = ai_models_dir / filename;
            
            // Validate that file exists
            if (!std::filesystem::exists(full_path)) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: Model file not found: " + full_path.string());
                return false;
            }
            
            {
                std::lock_guard<std::mutex> lock(model_configs_mutex);
                ai_model_configs[ai_name].model_path = full_path.string();
            }
            
            // Update running AI if it exists (will take effect on restart)
            {
                std::lock_guard<std::mutex> lock(instances_mutex);
                auto it = ai_instances.find(ai_name);

                if (it != ai_instances.end()) {
                    it->second->set_model_path(full_path.string());
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Error setting AI model path - " + std::string(e.what()));
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
    
    // Set custom markers for a specific AI instance
    bool set_ai_markers(const std::string& ai_name, 
                        const std::string& sys_start, const std::string& sys_end,
                        const std::string& usr_start, const std::string& usr_end,
                        const std::string& asst_start, const std::string& asst_end) {
        // Validate markers
        if (sys_start.empty() || sys_end.empty() || 
            usr_start.empty() || usr_end.empty() ||
            asst_start.empty() || asst_end.empty()) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Prompt markers cannot be empty");
            return false;
        }
        
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
        
        // Update running AI if it exists
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

    bool trigger_ai_inference(const std::string& ai_name) {
        std::lock_guard<std::mutex> lock(instances_mutex);
        auto it = ai_instances.find(ai_name);
        
        if (it == ai_instances.end()) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI '" + ai_name + "' not found");
            return false;
        }
        
        return it->second->trigger_inference();
    }

    bool get_prompts_local(const std::string& ai_name, std::string& system_prompt, std::string& user_prompt) {
        std::lock_guard<std::mutex> lock(prompts_mutex);
        auto it = ai_prompts.find(ai_name);
        
        if (it == ai_prompts.end()) {
            return false;
        }
        
        system_prompt = it->second.system_prompt;
        user_prompt = it->second.user_prompt;
        return true;
    }
    
    // Initialize and start an AI instance
    bool initialize_ai(const std::string& ai_name) {
        std::lock_guard<std::mutex> lock(instances_mutex);
        
        try {
            if (ai_instances.find(ai_name) != ai_instances.end()) {
                if (ai_instances[ai_name]->is_running()) {
                    return false;
                }

                ai_instances.erase(ai_name);
            }
            
            // Get prompts
            std::string system_prompt, user_prompt;
            
            if (!get_prompts_local(ai_name, system_prompt, user_prompt)) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: Failed to get prompts for AI '" + ai_name + "'");
                return false;
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
                try {
                    std::filesystem::path ai_models_dir = get_ai_models_path();
                    current_model_path = (ai_models_dir / "Llama-3.1-13B-Instruct.Q8_0.gguf").string();
                    set_model_path("Llama-3.1-13B-Instruct.Q8_0.gguf");
                } catch (const std::exception& e) {
                    intercept::client::invoker_lock lock;
                    report_error("KH - AI Framework: Failed to set default model path");
                    return false;
                }
            }
            
            if (current_model_path.empty()) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: No model path set");
                return false;
            }
            
            // Create AI instance
            auto ai = std::make_unique<AIController>(ai_name);
            ai->update_system_prompt(system_prompt);
            ai->update_user_prompt(user_prompt);

            {
                std::lock_guard<std::mutex> config_lock(model_configs_mutex);
                auto config_it = ai_model_configs.find(ai_name);
                if (config_it != ai_model_configs.end() && !config_it->second.model_path.empty()) {
                    ai->set_model_path(config_it->second.model_path);
                }
            }

            {
                std::lock_guard<std::mutex> config_lock(model_configs_mutex);
                auto config_it = ai_model_configs.find(ai_name);
                
                if (config_it != ai_model_configs.end() && config_it->second.has_custom_markers) {
                    ai->set_markers(
                        config_it->second.marker_system_start,
                        config_it->second.marker_system_end,
                        config_it->second.marker_user_start,
                        config_it->second.marker_user_end,
                        config_it->second.marker_assistant_start,
                        config_it->second.marker_assistant_end
                    );
                }
            }

            {
                std::lock_guard<std::mutex> config_lock(model_configs_mutex);
                auto config_it = ai_model_configs.find(ai_name);
                
                if (config_it != ai_model_configs.end() && config_it->second.has_custom_parameters) {
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
            
            if (!ai->start(current_model_path)) {
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: Failed to initialize AI '" + ai_name + "'");
                return false;
            }

            ai_instances[ai_name] = std::move(ai);
            initialized = true;
            return true;
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: Exception initializing AI '" + ai_name + "': " + std::string(e.what()));
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
                intercept::client::invoker_lock lock;
                report_error("KH - AI Framework: AI '" + ai_name + "' was not found");
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
            ai->stop();
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
    
    // Get list of active AI names
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

    bool clear_ai_notes(const std::string& ai_name) {
        std::lock_guard<std::mutex> lock(instances_mutex);
        auto it = ai_instances.find(ai_name);

        if (it == ai_instances.end()) {
            intercept::client::invoker_lock lock;
            report_error("KH - AI Framework: AI '" + ai_name + "' was not found");
            return false;
        }
        
        it->second->clear_notes();
        return true;
    }
        
    // Update system prompt for specific AI
    bool update_system_prompt(const std::string& ai_name, const std::string& prompt) {
        // Update local storage
        {
            std::lock_guard<std::mutex> lock(prompts_mutex);
            auto& prompt_data = ai_prompts[ai_name];
            prompt_data.system_prompt = prompt;
        }
        
        // Update running AI if it exists
        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            auto it = ai_instances.find(ai_name);

            if (it != ai_instances.end()) {
                it->second->update_system_prompt(prompt);
            }
        }
        
        return true;
    }
    
    // Update user prompt for a specific AI
    bool update_user_prompt(const std::string& ai_name, const std::string& prompt) {
        // Update local storage
        {
            std::lock_guard<std::mutex> lock(prompts_mutex);
            
            auto& prompt_data = ai_prompts[ai_name];
            prompt_data.user_prompt = prompt;
        }
        
        // Update running AI if it exists
        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            
            auto it = ai_instances.find(ai_name);
            if (it != ai_instances.end()) {
                it->second->update_user_prompt(prompt);
            }
        }
        
        return true;
    }
    
    // Clear all stored prompts
    void clear_all_prompts() {
        std::lock_guard<std::mutex> lock(prompts_mutex);
        ai_prompts.clear();
    }
    
    // Check if prompts exist for an AI
    bool has_prompts(const std::string& ai_name) const {
        std::lock_guard<std::mutex> lock(prompts_mutex);
        return ai_prompts.find(ai_name) != ai_prompts.end();
    }

    bool is_initialized() const {
        return initialized;
    }
};