#pragma once

using namespace intercept;
using namespace intercept::types;

struct SpeakerState {
    std::string speaker_id;
    std::vector<int16_t> audio_buffer;
    std::atomic<bool> is_playing{false};
    std::atomic<bool> should_stop{false};
    std::atomic<float> position_3d[3] = {0.0f, 0.0f, 0.0f};
    std::atomic<float> volume{1.0f};
    std::thread playback_thread;
    std::mutex buffer_mutex;
};

class TTSModelDiscovery {
public:
    static std::vector<std::filesystem::path> find_all_tts_model_directories() {
        std::vector<std::filesystem::path> search_paths;
        
        // Priority 1: Documents folder
        try {
            char docs_path[MAX_PATH];

            if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs_path) == S_OK) {
                std::filesystem::path docs_tts_models = std::filesystem::path(docs_path) / "Arma 3" / "kh_framework" / "tts_models";

                if (std::filesystem::exists(docs_tts_models)) {
                    search_paths.push_back(docs_tts_models);
                }
            }
        } catch (...) {}

        // Priority 2: Mod folders
        auto mod_tts_dirs = ModFolderSearcher::find_directories_in_mods("tts_models");
        search_paths.insert(search_paths.end(), mod_tts_dirs.begin(), mod_tts_dirs.end());
        return search_paths;
    }

    static std::filesystem::path find_model(const std::string& model_name) {
        auto search_paths = find_all_tts_model_directories();
        
        for (const auto& base_path : search_paths) {
            std::filesystem::path model_path = base_path / model_name;
            if (std::filesystem::exists(model_path) && std::filesystem::is_directory(model_path)) {
                return model_path;
            }
        }
        
        return std::filesystem::path();
    }

    static std::filesystem::path find_espeak_data(const std::filesystem::path& model_path) {
        if (model_path.empty()) return std::filesystem::path();
        std::filesystem::path espeak_path = model_path / "espeak-ng-data";

        if (std::filesystem::exists(espeak_path) && std::filesystem::is_directory(espeak_path)) {
            // Validate it has required files
            if (std::filesystem::exists(espeak_path / "voices") || 
                std::filesystem::exists(espeak_path / "lang")) {
                return espeak_path;
            }
        }
        
        return std::filesystem::path();
    }

    static std::filesystem::path find_any_model() {
        auto search_paths = find_all_tts_model_directories();
        
        for (const auto& base_path : search_paths) {
            try {
                if (!std::filesystem::exists(base_path) || !std::filesystem::is_directory(base_path)) {
                    continue;
                }
                
                for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
                    if (!entry.is_directory()) continue;
                    std::filesystem::path model_path = entry.path();
                    
                    // Check if this directory contains a valid TTS model
                    bool has_onnx = false;
                    bool has_espeak = false;

                    for (const auto& file : std::filesystem::directory_iterator(model_path)) {
                        if (file.is_regular_file()) {
                            std::string filename = file.path().filename().string();
                            std::string lower_filename = filename;

                            std::transform(lower_filename.begin(), lower_filename.end(), 
                                         lower_filename.begin(), ::tolower);
                            
                            if (lower_filename.ends_with(".onnx") && !lower_filename.ends_with(".json")) {
                                has_onnx = true;
                                break;
                            }
                        }
                    }
                    
                    // Check for espeak-ng-data
                    has_espeak = !find_espeak_data(model_path).empty();
                    
                    if (has_onnx && has_espeak) {
                        return model_path;
                    }
                }
            } catch (...) {
                // Error reading directory, skip
            }
        }
        
        return std::filesystem::path();
    }
};

static std::vector<int16_t> float_to_int16(const float* samples, int count) {
    std::vector<int16_t> result(count);

    for (int i = 0; i < count; i++) {
        float sample = std::max(-1.0f, std::min(1.0f, samples[i]));
        result[i] = static_cast<int16_t>(sample * 32767.0f);
    }

    return result;
}

static void audio_playback_thread(SpeakerState* state, int sample_rate) {
    HWAVEOUT hWaveOut = NULL;
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = sample_rate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        state->is_playing = false;
        return;
    }

    // Copy the audio buffer at the start incase if speak() is called again with the same speaker_id during playback
    std::vector<int16_t> audio_copy;
    
    {
        std::lock_guard<std::mutex> lock(state->buffer_mutex);
        audio_copy = state->audio_buffer;
    }

    if (audio_copy.empty()) {
        waveOutClose(hWaveOut);
        state->is_playing = false;
        return;
    }

    constexpr int CHUNK_SAMPLES = 1024;
    constexpr int NUM_BUFFERS = 3;
    constexpr float PI_2 = 1.57079632679489661923f;
    size_t total_samples = audio_copy.size();
    std::vector<WAVEHDR> headers(NUM_BUFFERS);
    std::vector<std::vector<int16_t>> stereo_buffers(NUM_BUFFERS);
    
    for (int i = 0; i < NUM_BUFFERS; i++) {
        stereo_buffers[i].resize(CHUNK_SAMPLES * 2);
        headers[i] = {};
        headers[i].lpData = reinterpret_cast<LPSTR>(stereo_buffers[i].data());
        headers[i].dwBufferLength = static_cast<DWORD>(stereo_buffers[i].size() * sizeof(int16_t));
        headers[i].dwFlags = 0;
        headers[i].dwUser = i;
        waveOutPrepareHeader(hWaveOut, &headers[i], sizeof(WAVEHDR));
    }

    size_t current_sample = 0;
    int buffers_in_use = 0;

    // Lambda to fill a buffer - reads position/volume in real-time, but uses local audio copy
    auto fill_buffer = [&](int buffer_idx, size_t start_sample) -> size_t {
        float x = std::clamp<float>(state->position_3d[0].load(), -1.0f, 1.0f);
        float y = std::clamp<float>(state->position_3d[1].load(), -1.0f, 1.0f);
        float z = std::clamp<float>(state->position_3d[2].load(), -1.0f, 1.0f);
        float base_volume = std::clamp<float>(state->volume.load(), 0.0f, 1.0f);
        float y_factor = (y + 1.0f) / 2.0f;
        y_factor = 0.3f + (y_factor * 0.7f);
        float z_factor = 1.0f - (std::abs(z) * 0.2f);
        float distance_volume = base_volume * y_factor * z_factor;
        float pan_position = (x + 1.0f) / 2.0f;
        float left_gain = distance_volume * std::cos(pan_position * PI_2);
        float right_gain = distance_volume * std::sin(pan_position * PI_2);

        // Calculate samples to process from our LOCAL copy
        size_t remaining = total_samples - start_sample;
        size_t samples_to_process = std::min(static_cast<size_t>(CHUNK_SAMPLES), remaining);

        // Fill stereo buffer from local audio copy (no mutex needed!)
        auto& stereo = stereo_buffers[buffer_idx];

        for (size_t i = 0; i < samples_to_process; i++) {
            int16_t mono_sample = audio_copy[start_sample + i];
            stereo[i * 2] = static_cast<int16_t>(std::clamp<float>(mono_sample * left_gain, -32767.0f, 32767.0f));
            stereo[i * 2 + 1] = static_cast<int16_t>(std::clamp<float>(mono_sample * right_gain, -32767.0f, 32767.0f));
        }

        // Zero-fill remainder for last chunk
        for (size_t i = samples_to_process; i < CHUNK_SAMPLES; i++) {
            stereo[i * 2] = 0;
            stereo[i * 2 + 1] = 0;
        }

        headers[buffer_idx].dwBufferLength = static_cast<DWORD>(samples_to_process * 2 * sizeof(int16_t));
        return samples_to_process;
    };

    // Pre-fill initial buffers
    for (int i = 0; i < NUM_BUFFERS && current_sample < total_samples; i++) {
        size_t processed = fill_buffer(i, current_sample);

        if (processed > 0) {
            waveOutWrite(hWaveOut, &headers[i], sizeof(WAVEHDR));
            current_sample += processed;
            buffers_in_use++;
        }
    }

    // Main playback loop
    while (buffers_in_use > 0 && !state->should_stop) {
        for (int i = 0; i < NUM_BUFFERS; i++) {
            if (headers[i].dwFlags & WHDR_DONE) {
                headers[i].dwFlags &= ~WHDR_DONE;
                buffers_in_use--;

                // If more samples remain, refill and requeue
                if (current_sample < total_samples) {
                    size_t processed = fill_buffer(i, current_sample);

                    if (processed > 0) {
                        waveOutWrite(hWaveOut, &headers[i], sizeof(WAVEHDR));
                        current_sample += processed;
                        buffers_in_use++;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (state->should_stop) {
        waveOutReset(hWaveOut);
    }

    for (int i = 0; i < NUM_BUFFERS; i++) {
        int timeout = 0;

        while ((headers[i].dwFlags & WHDR_INQUEUE) && timeout < 50) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            timeout++;
        }

        waveOutUnprepareHeader(hWaveOut, &headers[i], sizeof(WAVEHDR));
    }

    waveOutClose(hWaveOut);
    state->is_playing = false;
    bool was_stopped = state->should_stop;

    try {
        intercept::client::invoker_lock lock;
        auto_array<game_value> tts_finished_data;
        tts_finished_data.push_back(game_value(state->speaker_id));
        tts_finished_data.push_back(game_value(was_stopped));
        raw_call_sqf_args_native_no_return(g_compiled_tts_finished_event, game_value(std::move(tts_finished_data)));
    } catch (...) {}
}

class TTSFramework {
private:
    TTSFramework() = default;
    ~TTSFramework() { cleanup(); }
    TTSFramework(const TTSFramework&) = delete;
    TTSFramework& operator=(const TTSFramework&) = delete;
    const SherpaOnnxOfflineTts* tts_handle = nullptr;
    std::mutex tts_handle_mutex;  // Protects tts_handle, sample_rate, num_speakers, current_model_name
    std::unordered_map<std::string, std::unique_ptr<SpeakerState>> speaker_states;
    std::mutex speaker_mutex;     // Protects speaker_states only
    std::string current_model_name;
    int sample_rate = 22050;
    int num_speakers = 1;

    struct GenerationRequest {
        std::string speaker_id;
        std::string text;
        int sid;
        float speed;
        float x, y, z, volume;
    };
    
    std::deque<GenerationRequest> generation_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::thread generation_thread;
    std::atomic<bool> generation_thread_running{false};
    
    void generation_worker() {
        while (generation_thread_running) {
            GenerationRequest request;
            bool has_request = false;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                queue_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return !generation_queue.empty() || !generation_thread_running;
                });
                
                if (!generation_queue.empty()) {
                    request = generation_queue.front();
                    generation_queue.pop_front();
                    has_request = true;
                }
            }
            
            if (has_request) {
                try {
                    const SherpaOnnxOfflineTts* handle_copy = nullptr;
                    int current_sample_rate = 0;
                    
                    {
                        std::lock_guard<std::mutex> lock(tts_handle_mutex);
                        
                        if (!tts_handle) {
                            intercept::client::invoker_lock lock;
                            report_error("KH - TTS Framework: Cannot generate speech - no model loaded");
                            continue;
                        }
                        
                        handle_copy = tts_handle;
                        current_sample_rate = sample_rate;
                    }

                    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(handle_copy, request.text.c_str(), request.sid, request.speed);

                    {
                        std::lock_guard<std::mutex> handle_lock(tts_handle_mutex);

                        if (tts_handle != handle_copy) {
                            if (audio) {
                                SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
                            }

                            continue;
                        }
                        
                        // Check if we need to join a previous playback thread - extract it without blocking
                        std::thread thread_to_join;
                        bool should_start_playback = false;
                        bool should_stop_early = false;
                        
                        {
                            std::lock_guard<std::mutex> speaker_lock(speaker_mutex);
                            
                            if (speaker_states.find(request.speaker_id) == speaker_states.end()) {
                                auto new_state = std::make_unique<SpeakerState>();
                                new_state->speaker_id = request.speaker_id;
                                speaker_states[request.speaker_id] = std::move(new_state);
                            }

                            auto& state = speaker_states[request.speaker_id];
                            
                            if (audio && audio->samples && audio->n > 0) {
                                std::lock_guard<std::mutex> buffer_lock(state->buffer_mutex);
                                state->audio_buffer = float_to_int16(audio->samples, audio->n);
                                state->position_3d[0] = request.x;
                                state->position_3d[1] = request.y;
                                state->position_3d[2] = request.z;
                                state->volume = request.volume;

                                if (!state->is_playing && !state->audio_buffer.empty()) {
                                    if (state->should_stop) {
                                        should_stop_early = true;
                                    } else {
                                        // Extract thread to join OUTSIDE the lock
                                        if (state->playback_thread.joinable()) {
                                            thread_to_join = std::move(state->playback_thread);
                                        }

                                        should_start_playback = true;
                                    }
                                }
                            } else {
                                if (audio) {
                                    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
                                }

                                intercept::client::invoker_lock lock;
                                report_error("KH - TTS Framework: Audio generation failed for speaker: " + request.speaker_id);
                                continue;
                            }
                        }
                        
                        // Join thread OUTSIDE the speaker_mutex lock to prevent deadlock
                        if (thread_to_join.joinable()) {
                            thread_to_join.join();
                        }
                        
                        if (should_stop_early) {
                            SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
                            
                            try {
                                intercept::client::invoker_lock lock;
                                auto_array<game_value> tts_finished_data;
                                tts_finished_data.push_back(game_value(request.speaker_id));
                                tts_finished_data.push_back(game_value(true));
                                raw_call_sqf_args_native_no_return(g_compiled_tts_finished_event, game_value(std::move(tts_finished_data)));
                            } catch (...) {}

                            continue;
                        }
                        
                        if (should_start_playback) {
                            // Re-acquire lock to start playback thread
                            std::lock_guard<std::mutex> speaker_lock(speaker_mutex);
                            auto it = speaker_states.find(request.speaker_id);
                            
                            if (it != speaker_states.end()) {
                                auto& state = it->second;
                                
                                // Double-check state hasn't changed
                                if (!state->is_playing && !state->should_stop) {
                                    state->is_playing = true;
                                    state->should_stop = false;
                                    state->playback_thread = std::thread(audio_playback_thread, state.get(), current_sample_rate);
                                    
                                    try {
                                        intercept::client::invoker_lock lock;
                                        auto_array<game_value> tts_data;
                                        tts_data.push_back(game_value(request.speaker_id));
                                        tts_data.push_back(game_value(request.text));
                                        raw_call_sqf_args_native_no_return(g_compiled_tts_generated_event, game_value(std::move(tts_data)));
                                    } catch (...) {}
                                }
                            }
                        }
                        
                        SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
                    }
                } catch (const std::exception& e) {
                    intercept::client::invoker_lock lock;
                    report_error("KH - TTS Framework: Generation exception: " + std::string(e.what()));
                } catch (...) {
                    intercept::client::invoker_lock lock;
                    report_error("KH - TTS Framework: Unknown generation exception");
                }
            }
        }
    }
    
public:
    static TTSFramework& instance() {
        static TTSFramework inst;
        return inst;
    }

    bool load_model(
        const std::string& model_name,
        const std::string& provider = "dml",
        int num_threads = 4,
        float noise_scale = 0.667f,
        float noise_scale_w = 0.8f,
        float length_scale = 1.0f
    ) {
        try {
            std::lock_guard<std::mutex> lock(tts_handle_mutex);

            {
                std::lock_guard<std::mutex> queue_lock(queue_mutex);
                generation_queue.clear();
            }

            if (tts_handle) {
                SherpaOnnxDestroyOfflineTts(tts_handle);
                tts_handle = nullptr;
            }

            std::filesystem::path model_path;
            std::string resolved_model_name = model_name;
            
            if (model_name.empty()) {
                // Find any available model
                model_path = TTSModelDiscovery::find_any_model();
                
                if (model_path.empty()) {
                    report_error("KH - TTS Framework: No TTS models found in any search location");
                    return false;
                }
                
                resolved_model_name = model_path.filename().string();
            } else {
                model_path = TTSModelDiscovery::find_model(model_name);
                
                if (model_path.empty()) {
                    report_error("KH - TTS Framework: Model not found: " + model_name);
                    return false;
                }
            }

            std::filesystem::path espeak_data_path = TTSModelDiscovery::find_espeak_data(model_path);

            if (espeak_data_path.empty()) {
                report_error("KH - TTS Framework: espeak-ng-data not found in model directory: " + model_path.string());
                return false;
            }

            std::string model_file_path;
            std::string tokens_file_path;
            
            for (const auto& entry : std::filesystem::directory_iterator(model_path)) {
                std::string filename = entry.path().filename().string();
                std::string lower_filename = filename;

                std::transform(lower_filename.begin(), lower_filename.end(), 
                             lower_filename.begin(), ::tolower);
                
                if (lower_filename.ends_with(".onnx") && !lower_filename.ends_with(".json")) {
                    model_file_path = entry.path().string();
                }
                else if (lower_filename == "tokens.txt") {
                    tokens_file_path = entry.path().string();
                }
            }

            if (model_file_path.empty()) {
                report_error("KH - TTS Framework: No .onnx file found in model directory");
                return false;
            }

            std::string espeak_data_str = espeak_data_path.string();
            SherpaOnnxOfflineTtsConfig config;
            memset(&config, 0, sizeof(config));
            config.model.vits.model = model_file_path.c_str();
            config.model.vits.lexicon = "";
            config.model.vits.tokens = tokens_file_path.empty() ? "" : tokens_file_path.c_str();
            config.model.vits.data_dir = espeak_data_str.c_str();
            config.model.vits.noise_scale = noise_scale;
            config.model.vits.noise_scale_w = noise_scale_w;
            config.model.vits.length_scale = length_scale;
            config.model.vits.dict_dir = "";
            config.model.num_threads = num_threads;
            config.model.debug = 0;
            config.model.provider = provider.c_str();
            config.max_num_sentences = 1;
            config.rule_fsts = "";
            config.rule_fars = "";
            std::string actual_provider = provider;
            config.model.provider = actual_provider.c_str();
            tts_handle = SherpaOnnxCreateOfflineTts(&config);
            
            // If DML failed, fallback to CPU
            if (!tts_handle && _stricmp(actual_provider.c_str(), "dml") == 0) {
                intercept::client::invoker_lock lock;
                intercept::sqf::diag_log("KH - TTS Framework: DirectML not supported, falling back to CPU");
                actual_provider = "cpu";
                config.model.provider = actual_provider.c_str();
                tts_handle = SherpaOnnxCreateOfflineTts(&config);
            }
            
            if (!tts_handle) {
                report_error("KH - TTS Framework: Failed to create TTS instance");
                return false;
            }

            num_speakers = SherpaOnnxOfflineTtsNumSpeakers(tts_handle);
            sample_rate = SherpaOnnxOfflineTtsSampleRate(tts_handle);
            current_model_name = resolved_model_name;
            
            {
                intercept::client::invoker_lock lock;
                std::string provider_name = actual_provider;

                std::transform(provider_name.begin(), provider_name.end(), 
                             provider_name.begin(), ::toupper);
                
                sqf::diag_log("KH - TTS Framework: Model loaded successfully - " + resolved_model_name + 
                    " | Provider: " + provider_name + 
                    " | Speakers: " + std::to_string(num_speakers) + 
                    " | Sample Rate: " + std::to_string(sample_rate) + " Hz"
                );
            }

            // Start generation thread if not running
            if (!generation_thread_running) {
                generation_thread_running = true;
                generation_thread = std::thread(&TTSFramework::generation_worker, this);
            }
            
            return true;
        } catch (const std::exception& e) {
            report_error("KH - TTS Framework: Model loading exception: " + std::string(e.what()));
            return false;
        }
    }

    bool speak(const std::string& speaker_id, const std::string& text, float x, float y, float z, float volume, float speed = 1.0f, int sid = 0) {
        {
            std::lock_guard<std::mutex> lock(tts_handle_mutex);
            if (!tts_handle) return false;
        }

        GenerationRequest request;
        request.speaker_id = speaker_id;
        request.text = text;
        request.sid = sid;
        request.speed = speed;
        request.x = x;
        request.y = y;
        request.z = z;
        request.volume = std::clamp(volume, 0.0f, 1.0f);
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            generation_queue.push_back(request);
        }

        queue_cv.notify_one();
        return true;
    }

    bool update_speaker(const std::string& speaker_id, float x, float y, float z, float volume) {
        std::lock_guard<std::mutex> lock(speaker_mutex);
        auto it = speaker_states.find(speaker_id);
        if (it == speaker_states.end()) return false;
        auto& state = it->second;
        state->position_3d[0] = x;
        state->position_3d[1] = y;
        state->position_3d[2] = z;
        state->volume = std::clamp(volume, 0.0f, 1.0f);
        return true;
    }

    bool stop_speaker(const std::string& speaker_id) {
        std::thread thread_to_join;
        
        {
            std::lock_guard<std::mutex> lock(speaker_mutex);
            auto it = speaker_states.find(speaker_id);
            if (it == speaker_states.end()) return false;
            auto& state = it->second;
            state->should_stop = true;

            if (state->playback_thread.joinable()) {
                thread_to_join = std::move(state->playback_thread);
            }
            
            speaker_states.erase(it);
        }

        if (thread_to_join.joinable()) {
            thread_to_join.join();
        }
        
        return true;
    }

    bool is_playing(const std::string& speaker_id) {
        std::lock_guard<std::mutex> lock(speaker_mutex);
        
        auto it = speaker_states.find(speaker_id);
        if (it == speaker_states.end()) return false;
        
        return it->second->is_playing;
    }

    void stop_all() {
        std::vector<std::thread> threads_to_join;
        
        {
            std::lock_guard<std::mutex> lock(speaker_mutex);
            
            for (auto& [id, state] : speaker_states) {
                state->should_stop = true;

                if (state->playback_thread.joinable()) {
                    threads_to_join.push_back(std::move(state->playback_thread));
                }
            }
            
            speaker_states.clear();
        }

        for (auto& thread : threads_to_join) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    bool is_initialized() {
        std::lock_guard<std::mutex> lock(tts_handle_mutex);
        return tts_handle != nullptr;
    }

    void cleanup() {
        generation_thread_running = false;
        queue_cv.notify_all();

        {
            std::lock_guard<std::mutex> lock(speaker_mutex);

            for (auto& [id, state] : speaker_states) {
                state->should_stop = true;
            }
        }

        if (generation_thread.joinable()) {
            generation_thread.join();
        }
        
        // Extract and join playback threads
        std::vector<std::thread> threads_to_join;

        {
            std::lock_guard<std::mutex> lock(speaker_mutex);

            for (auto& [id, state] : speaker_states) {
                if (state->playback_thread.joinable()) {
                    threads_to_join.push_back(std::move(state->playback_thread));
                }
            }
        }

        for (auto& thread : threads_to_join) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        {
            std::lock_guard<std::mutex> speaker_lock(speaker_mutex);
            speaker_states.clear();
        }
        
        {
            std::lock_guard<std::mutex> handle_lock(tts_handle_mutex);
            
            if (tts_handle) {
                SherpaOnnxDestroyOfflineTts(tts_handle);
                tts_handle = nullptr;
            }
            
            current_model_name.clear();
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            generation_queue.clear();
        }
    }
};