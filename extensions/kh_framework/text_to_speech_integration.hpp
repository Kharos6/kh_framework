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
    std::atomic<bool> pending_finished_event{false};
    std::atomic<bool> finished_was_stopped{false};
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
        std::string speaker_id_copy = state->speaker_id;

        MainThreadScheduler::instance().schedule([speaker_id_copy]() {
            report_error("KH - TTS Framework: Failed to open audio output device for speaker: " + speaker_id_copy);
        });
        
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

    waveOutReset(hWaveOut);

    // Now all buffers are guaranteed to be done (WHDR_DONE set, WHDR_INQUEUE cleared)
    for (int i = 0; i < NUM_BUFFERS; i++) {
        waveOutUnprepareHeader(hWaveOut, &headers[i], sizeof(WAVEHDR));
    }

    waveOutClose(hWaveOut);
    state->finished_was_stopped = state->should_stop.load();
    state->pending_finished_event = true;
    state->is_playing = false;
}

class TTSFramework {
private:
    TTSFramework() = default;
    ~TTSFramework() { cleanup(); }
    TTSFramework(const TTSFramework&) = delete;
    TTSFramework& operator=(const TTSFramework&) = delete;
    std::shared_ptr<const SherpaOnnxOfflineTts> tts_handle;
    std::mutex tts_handle_mutex;  // Protects tts_handle, sample_rate, num_speakers, current_model_name
    std::unordered_map<std::string, std::unique_ptr<SpeakerState>> speaker_states;
    mutable std::shared_mutex speaker_mutex;     // Protects speaker_states only
    std::string current_model_name;
    int sample_rate = 22050;
    int num_speakers = 1;
    std::atomic<bool> is_initialized_flag{false};
    std::thread cleanup_thread;
    std::atomic<bool> cleanup_thread_running{false};
    std::mutex cleanup_mutex;
    std::condition_variable cleanup_cv;
    std::deque<std::thread> threads_to_cleanup;

    struct TtsHandleDeleter {
        void operator()(const SherpaOnnxOfflineTts* p) const {
            if (p) SherpaOnnxDestroyOfflineTts(p);
        }
    };

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

    // Helper functions to schedule events on main thread
    void schedule_tts_generated_event(const std::string& speaker_id, const std::string& text) {
        MainThreadScheduler::instance().schedule([speaker_id, text]() {
            auto_array<game_value> tts_data;
            tts_data.push_back(game_value(speaker_id));
            tts_data.push_back(game_value(text));
            raw_call_sqf_args_native_no_return(g_compiled_tts_generated_event, game_value(std::move(tts_data)));
        });
    }

    void schedule_tts_finished_event(const std::string& speaker_id, bool was_stopped) {
        MainThreadScheduler::instance().schedule([speaker_id, was_stopped]() {
            auto_array<game_value> tts_finished_data;
            tts_finished_data.push_back(game_value(speaker_id));
            tts_finished_data.push_back(game_value(was_stopped));
            raw_call_sqf_args_native_no_return(g_compiled_tts_finished_event, game_value(std::move(tts_finished_data)));
        });
    }

    void cleanup_worker() {
        while (cleanup_thread_running) {
            std::thread thread_to_join;
            bool has_thread = false;
            
            {
                std::unique_lock<std::mutex> lock(cleanup_mutex);
                
                cleanup_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return !threads_to_cleanup.empty() || !cleanup_thread_running;
                });
                
                if (!threads_to_cleanup.empty()) {
                    thread_to_join = std::move(threads_to_cleanup.front());
                    threads_to_cleanup.pop_front();
                    has_thread = true;
                }
            }

            if (has_thread && thread_to_join.joinable()) {
                thread_to_join.join();
            }
        }
        
        // Drain remaining threads on shutdown
        std::unique_lock<std::mutex> lock(cleanup_mutex);

        for (auto& t : threads_to_cleanup) {
            if (t.joinable()) {
                lock.unlock();
                t.join();
                lock.lock();
            }
        }
        threads_to_cleanup.clear();
    }

    void queue_thread_for_cleanup(std::thread&& t) {
        if (!t.joinable()) return;
        std::lock_guard<std::mutex> lock(cleanup_mutex);
        threads_to_cleanup.push_back(std::move(t));
        cleanup_cv.notify_one();
    }

    void generation_worker() {
        while (generation_thread_running.load(std::memory_order_acquire)) {
            GenerationRequest request;
            bool has_request = false;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                queue_cv.wait_for(lock, std::chrono::milliseconds(16), [this] {
                    return !generation_queue.empty() || !generation_thread_running.load(std::memory_order_acquire);
                });
                
                if (!generation_queue.empty()) {
                    request = generation_queue.front();
                    generation_queue.pop_front();
                    has_request = true;
                }
            }
            
            // Check for pending finished events from playback threads
            {
                std::unique_lock<std::shared_mutex> speaker_lock(speaker_mutex, std::try_to_lock);
                
                if (speaker_lock.owns_lock()) {
                    std::vector<std::string> speakers_to_cleanup;
                    
                    for (auto& [id, state] : speaker_states) {
                        if (state->pending_finished_event.exchange(false, std::memory_order_acq_rel)) {
                            schedule_tts_finished_event(state->speaker_id, 
                                          state->finished_was_stopped.load(std::memory_order_acquire));
                            
                            if (!state->is_playing.load(std::memory_order_acquire)) {
                                speakers_to_cleanup.push_back(id);
                            }
                        }
                    }
                    
                    for (const auto& speaker_id : speakers_to_cleanup) {
                        auto it = speaker_states.find(speaker_id);

                        if (it != speaker_states.end() && !it->second->is_playing.load()) {
                            if (it->second->playback_thread.joinable()) {
                                queue_thread_for_cleanup(std::move(it->second->playback_thread));
                            }

                            speaker_states.erase(it);
                        }
                    }
                }
            }
                        
            if (has_request) {
                try {
                    std::shared_ptr<const SherpaOnnxOfflineTts> handle_copy;
                    int current_sample_rate = 0;
                    
                    {
                        std::lock_guard<std::mutex> lock(tts_handle_mutex);

                        if (!tts_handle) {
                            schedule_tts_finished_event(request.speaker_id, true);
                            continue;
                        }

                        handle_copy = tts_handle;
                        current_sample_rate = sample_rate;
                    }

                    // Generate audio - handle_copy keeps the handle alive even if load_model() is called
                    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(
                        handle_copy.get(), request.text.c_str(), request.sid, request.speed);
                    
                    // No need to check if handle changed - our ref keeps it alive
                    // But we should skip if a NEW model was loaded (user expectation)
                    {
                        std::lock_guard<std::mutex> handle_lock(tts_handle_mutex);

                        if (tts_handle != handle_copy) {
                            if (audio) SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
                            continue;
                        }
                    }

                    bool should_start_playback = false;
                    std::string speaker_id_for_event = request.speaker_id;
                    std::string text_for_event = request.text;

                    if (audio && audio->samples && audio->n > 0) {
                        std::unique_lock<std::shared_mutex> speaker_lock(speaker_mutex);
                        auto it = speaker_states.find(request.speaker_id);

                        if (it != speaker_states.end()) {
                            auto& old_state = it->second;
                            
                            // Check if actually playing/generating - fire interrupted event
                            bool was_active = old_state->is_playing.load(std::memory_order_acquire);
                            
                            // Signal stop
                            old_state->should_stop.store(true, std::memory_order_release);

                            if (old_state->playback_thread.joinable()) {
                                std::thread thread_to_join = std::move(old_state->playback_thread);
                                auto old_state_kept_alive = std::move(it->second);
                                speaker_states.erase(it);
                                speaker_lock.unlock();
                                thread_to_join.join();
                                speaker_lock.lock();
                            } else {
                                speaker_states.erase(it);
                            }
                            
                            // Fire ttsFinished for the interrupted speech
                            if (was_active) {
                                schedule_tts_finished_event(request.speaker_id, true);
                            }
                        }
                        
                        // Create fresh state for new playback
                        auto new_state = std::make_unique<SpeakerState>();
                        new_state->speaker_id = request.speaker_id;
                        
                        {
                            std::lock_guard<std::mutex> buffer_lock(new_state->buffer_mutex);
                            new_state->audio_buffer = float_to_int16(audio->samples, audio->n);
                        }
                        
                        new_state->position_3d[0].store(request.x, std::memory_order_release);
                        new_state->position_3d[1].store(request.y, std::memory_order_release);
                        new_state->position_3d[2].store(request.z, std::memory_order_release);
                        new_state->volume.store(request.volume, std::memory_order_release);
                        new_state->is_playing.store(true, std::memory_order_release);               
                        speaker_states[request.speaker_id] = std::move(new_state);
                        speaker_states[request.speaker_id]->playback_thread = std::thread(audio_playback_thread, speaker_states[request.speaker_id].get(), current_sample_rate);
                        should_start_playback = true;
                    }

                    if (should_start_playback) {
                        schedule_tts_generated_event(speaker_id_for_event, text_for_event);
                    }

                    if (audio) {
                        SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
                    }
                    
                } catch (const std::exception& e) {
                    std::string error_msg = e.what();

                    MainThreadScheduler::instance().schedule([error_msg]() {
                        report_error("KH - TTS Framework: Generation exception: " + error_msg);
                    });
                } catch (...) {}
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
        // Collect info for logging outside the lock
        std::string log_message;
        bool success = false;
        bool dml_fallback = false;
        std::string resolved_model_name;
        std::string actual_provider = provider;
        int loaded_sample_rate = 0;
        int loaded_num_speakers = 0;
        
        {
            std::lock_guard<std::mutex> lock(tts_handle_mutex);

            // Clear generation queue when loading new model
            {
                std::lock_guard<std::mutex> queue_lock(queue_mutex);
                generation_queue.clear();
            }
            
            // Cleanup existing model
            tts_handle.reset();

            try {
                std::filesystem::path model_path;
                resolved_model_name = model_name;
                
                if (model_name.empty()) {
                    model_path = TTSModelDiscovery::find_any_model();
                    
                    if (model_path.empty()) {
                        log_message = "KH - TTS Framework: No TTS models found in any search location";
                    } else {
                        resolved_model_name = model_path.filename().string();
                    }
                } else {
                    model_path = TTSModelDiscovery::find_model(model_name);
                    
                    if (model_path.empty()) {
                        log_message = "KH - TTS Framework: Model not found: " + model_name;
                    }
                }

                if (log_message.empty() && !model_path.empty()) {
                    std::filesystem::path espeak_data_path = TTSModelDiscovery::find_espeak_data(model_path);
                    
                    if (espeak_data_path.empty()) {
                        log_message = "KH - TTS Framework: espeak-ng-data not found in model directory: " + model_path.string();
                    } else {
                        // Find ONNX model file and tokens
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
                            log_message = "KH - TTS Framework: No .onnx file found in model directory";
                        } else {
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
                            config.model.provider = actual_provider.c_str();
                            config.max_num_sentences = 1;
                            config.rule_fsts = "";
                            config.rule_fars = "";
                            tts_handle = std::shared_ptr<const SherpaOnnxOfflineTts>(SherpaOnnxCreateOfflineTts(&config), TtsHandleDeleter{});
                            
                            // Fallback to CPU if DML fails
                            if (!tts_handle && _stricmp(actual_provider.c_str(), "dml") == 0) {
                                dml_fallback = true;
                                actual_provider = "cpu";
                                config.model.provider = actual_provider.c_str();
                                tts_handle = std::shared_ptr<const SherpaOnnxOfflineTts>(SherpaOnnxCreateOfflineTts(&config), TtsHandleDeleter{});
                            }
                            
                            if (!tts_handle) {
                                log_message = "KH - TTS Framework: Failed to create TTS instance";
                            } else {
                                num_speakers = SherpaOnnxOfflineTtsNumSpeakers(tts_handle.get());
                                sample_rate = SherpaOnnxOfflineTtsSampleRate(tts_handle.get());
                                current_model_name = resolved_model_name;
                                loaded_num_speakers = num_speakers;
                                loaded_sample_rate = sample_rate;
                                is_initialized_flag.store(true, std::memory_order_release);
                                success = true;
                                
                                std::string provider_name = actual_provider;
                                std::transform(provider_name.begin(), provider_name.end(), 
                                             provider_name.begin(), ::toupper);
                                
                                log_message = "KH - TTS Framework: Model loaded successfully - " + model_path.string() + 
                                    " | Provider: " + provider_name + 
                                    " | Speakers: " + std::to_string(loaded_num_speakers) + 
                                    " | Sample Rate: " + std::to_string(loaded_sample_rate) + " Hz";
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                log_message = "KH - TTS Framework: Model loading exception: " + std::string(e.what());
            }
        }

        if (!log_message.empty()) {
            std::string msg = log_message;
            bool was_success = success;
            bool had_dml_fallback = dml_fallback;

            MainThreadScheduler::instance().schedule([msg, was_success, had_dml_fallback]() {
                if (was_success) {
                    sqf::diag_log(msg);

                    if (had_dml_fallback) {
                        sqf::diag_log("KH - TTS Framework: DirectML not supported, fell back to CPU");
                    }
                } else {
                    report_error(msg);
                }
            });
        }

        if (success) {
            if (!cleanup_thread_running.load(std::memory_order_acquire)) {
                cleanup_thread_running.store(true, std::memory_order_release);
                cleanup_thread = std::thread(&TTSFramework::cleanup_worker, this);
            }
            
            if (!generation_thread_running.load(std::memory_order_acquire)) {
                generation_thread_running.store(true, std::memory_order_release);
                generation_thread = std::thread(&TTSFramework::generation_worker, this);
            }
        }
        
        return success;
    }

    bool speak(const std::string& speaker_id, const std::string& text, float x, float y, float z, float volume, float speed = 1.0f, int sid = 0) {
        if (speaker_id.empty() || text.empty()) return false;
        
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

            // Remove any pending (not yet started) requests for this speaker
            // Note: We do NOT fire events for these - they never started generating
            generation_queue.erase(
                std::remove_if(generation_queue.begin(), generation_queue.end(),
                    [&speaker_id](const GenerationRequest& req) {
                        return req.speaker_id == speaker_id;
                    }),
                generation_queue.end()
            );
            
            generation_queue.push_back(request);
        }

        queue_cv.notify_one();
        return true;
    }

    bool update_speaker(const std::string& speaker_id, float x, float y, float z, float volume) {
        if (speaker_id.empty()) return false;
        std::unique_lock<std::shared_mutex> lock(speaker_mutex);
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
        if (speaker_id.empty()) return false;
        bool was_playing = false;
        
        {
            std::unique_lock<std::shared_mutex> lock(speaker_mutex);
            auto it = speaker_states.find(speaker_id);
            if (it == speaker_states.end()) return false;
            auto& state = it->second;
            was_playing = state->is_playing.load(std::memory_order_acquire);
            state->should_stop.store(true, std::memory_order_release);

            if (state->playback_thread.joinable()) {
                std::thread thread_to_join = std::move(state->playback_thread);
                auto state_kept_alive = std::move(it->second);
                speaker_states.erase(it);
                lock.unlock();
                thread_to_join.join();
            } else {
                speaker_states.erase(it);
            }
        }

        if (was_playing) {
            schedule_tts_finished_event(speaker_id, true);
        }
        
        return true;
    }

    bool is_playing(const std::string& speaker_id) {
        if (speaker_id.empty()) return false;
        std::shared_lock<std::shared_mutex> lock(speaker_mutex);
        auto it = speaker_states.find(speaker_id);
        if (it == speaker_states.end()) return false;
        return it->second->is_playing.load(std::memory_order_acquire);
    }

    void stop_all() {
        std::vector<std::string> speakers_that_were_playing;
        std::vector<std::thread> threads_to_join;
        std::vector<std::unique_ptr<SpeakerState>> states_to_keep_alive;
        
        {
            std::unique_lock<std::shared_mutex> lock(speaker_mutex);
            
            for (auto& [id, state] : speaker_states) {
                if (state->is_playing.load(std::memory_order_acquire)) {
                    speakers_that_were_playing.push_back(id);
                }

                state->should_stop.store(true, std::memory_order_release);
                
                if (state->playback_thread.joinable()) {
                    threads_to_join.push_back(std::move(state->playback_thread));
                    states_to_keep_alive.push_back(std::move(state));
                }
            }
            
            speaker_states.clear();
        }

        for (auto& t : threads_to_join) {
            if (t.joinable()) {
                t.join();
            }
        }

        // Schedule events for main thread
        for (const auto& speaker_id : speakers_that_were_playing) {
            schedule_tts_finished_event(speaker_id, true);
        }
    }

    bool is_initialized() {
        return is_initialized_flag.load(std::memory_order_acquire);
    }

    void cleanup() {
        generation_thread_running.store(false, std::memory_order_release);
        cleanup_thread_running.store(false, std::memory_order_release);
        queue_cv.notify_all();
        cleanup_cv.notify_all();

        {
            std::unique_lock<std::shared_mutex> lock(speaker_mutex);

            for (auto& [id, state] : speaker_states) {
                if (state) {
                    state->should_stop.store(true, std::memory_order_release);
                }
            }
        }

        if (generation_thread.joinable()) {
            try {
                generation_thread.join();
            } catch (...) {
                // Thread join failed, continue cleanup
            }
        }

        if (cleanup_thread.joinable()) {
            try {
                cleanup_thread.join();
            } catch (...) {
                // Thread join failed, continue cleanup
            }
        }

        std::vector<std::thread> remaining_threads;
        std::vector<std::unique_ptr<SpeakerState>> remaining_states;

        {
            std::unique_lock<std::shared_mutex> lock(speaker_mutex);

            for (auto& [id, state] : speaker_states) {
                if (state && state->playback_thread.joinable()) {
                    remaining_threads.push_back(std::move(state->playback_thread));
                    remaining_states.push_back(std::move(state));
                }
            }

            speaker_states.clear();
        }

        for (auto& t : remaining_threads) {
            if (t.joinable()) {
                try {
                    t.join();
                } catch (...) {
                    // Thread join failed, continue cleanup
                }
            }
        }

        remaining_states.clear();
        remaining_threads.clear();

        {
            std::lock_guard<std::mutex> handle_lock(tts_handle_mutex);
            is_initialized_flag.store(false, std::memory_order_release);
            tts_handle.reset();
            current_model_name.clear();
            sample_rate = 22050;
            num_speakers = 1;
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            generation_queue.clear();
        }

        {
            std::lock_guard<std::mutex> lock(cleanup_mutex);

            for (auto& t : threads_to_cleanup) {
                if (t.joinable()) {
                    try {
                        t.join();
                    } catch (...) {
                        // Continue anyway
                    }
                }
            }

            threads_to_cleanup.clear();
        }
    }
};