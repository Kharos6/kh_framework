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
    std::vector<std::pair<std::string, float>> effects_chain;
    mutable std::mutex effects_mutex;
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

    static std::filesystem::path find_lexicon(const std::filesystem::path& model_path) {
        if (model_path.empty()) return std::filesystem::path();
        
        for (const auto& entry : std::filesystem::directory_iterator(model_path)) {
            if (!entry.is_regular_file()) continue;
            std::string filename = entry.path().filename().string();
            std::string lower_filename = filename;
            std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
            
            if (lower_filename == "lexicon.txt") {
                return entry.path();
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

                    has_espeak = !find_espeak_data(model_path).empty();
                    bool has_lexicon = !find_lexicon(model_path).empty();
                    
                    // Check for Matcha TTS (has acoustic + vocoder)
                    bool has_acoustic = false;
                    bool has_vocoder = false;
                    
                    for (const auto& file : std::filesystem::directory_iterator(model_path)) {
                        if (!file.is_regular_file()) continue;
                        std::string filename = file.path().filename().string();
                        std::string lower_filename = filename;
                        std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
                        
                        if (lower_filename.find("acoustic") != std::string::npos && lower_filename.ends_with(".onnx")) {
                            has_acoustic = true;
                        }
                        if (lower_filename.find("vocoder") != std::string::npos && lower_filename.ends_with(".onnx")) {
                            has_vocoder = true;
                        }
                    }
                    
                    bool is_vits = has_onnx && (has_espeak || has_lexicon);
                    bool is_matcha = has_acoustic && has_vocoder;
                    
                    if (is_vits || is_matcha) {
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

class TTSFramework {
private:
    TTSFramework() = default;
    ~TTSFramework() { cleanup(); }
    TTSFramework(const TTSFramework&) = delete;
    TTSFramework& operator=(const TTSFramework&) = delete;
    std::shared_ptr<const SherpaOnnxOfflineTts> tts_handle;
    std::mutex tts_handle_mutex;  // Protects tts_handle, sample_rate, num_speakers
    std::unordered_map<std::string, std::unique_ptr<SpeakerState>> speaker_states;
    mutable std::shared_mutex speaker_mutex;     // Protects speaker_states only
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
        std::vector<std::pair<std::string, float>> effects_chain;
    };

    class AudioProcessor {
    public:
        struct FilterState {
            float lp_prev_l = 0.0f, lp_prev_r = 0.0f;
            float hp_prev_in_l = 0.0f, hp_prev_out_l = 0.0f;
            float hp_prev_in_r = 0.0f, hp_prev_out_r = 0.0f;
            uint32_t rng_state = 12345;
            float ring_mod_phase = 0.0f;
            std::vector<float> echo_buffer_l, echo_buffer_r;
            size_t echo_pos = 0;
            float last_echo_delay = 0.0f;
            float echo_decay = 0.5f;
            std::vector<float> reverb_buf1, reverb_buf2, reverb_buf3;
            size_t rev_pos1 = 0, rev_pos2 = 0, rev_pos3 = 0;
            float pitch_accumulator = 0.0f;
            
            void init(int sample_rate) {
                reverb_buf1.resize(static_cast<size_t>(sample_rate * 0.03f), 0.0f);
                reverb_buf2.resize(static_cast<size_t>(sample_rate * 0.04f), 0.0f);
                reverb_buf3.resize(static_cast<size_t>(sample_rate * 0.05f), 0.0f);
            }
        };

        static float lowpass(float sample, float cutoff, float& prev) {
            if (cutoff >= 1.0f) return sample;
            float rc = 1.0f / (cutoff * 2.0f * 3.14159f + 0.001f);
            float alpha = 1.0f / (1.0f + rc);
            prev = prev + alpha * (sample - prev);
            return prev;
        }
        
        static float highpass(float sample, float cutoff, float& prev_in, float& prev_out) {
            if (cutoff <= 0.0f) return sample;
            float rc = 1.0f / ((1.0f - cutoff) * 2.0f * 3.14159f + 0.001f);
            float alpha = rc / (rc + 1.0f);
            float output = alpha * (prev_out + sample - prev_in);
            prev_in = sample;
            prev_out = output;
            return output;
        }
        
        static float distort(float sample, float amount) {
            if (amount <= 0.0f) return sample;
            float drive = 1.0f + amount * 10.0f;
            sample *= drive;
            float x = std::clamp(sample, -1.5f, 1.5f);
            return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
        }
        
        static float bitcrush(float sample, float amount) {
            if (amount <= 0.0f) return sample;
            float bits = 16.0f - amount * 14.0f;
            float levels = std::pow(2.0f, bits);
            return std::round(sample * levels) / levels;
        }
        
        static float add_noise(float sample, float amount, uint32_t& rng_state) {
            if (amount <= 0.0f) return sample;
            rng_state ^= rng_state << 13;
            rng_state ^= rng_state >> 17;
            rng_state ^= rng_state << 5;
            float noise = (static_cast<float>(rng_state) / static_cast<float>(UINT32_MAX)) * 2.0f - 1.0f;
            return sample + noise * amount * 0.3f;
        }
        
        static float ring_mod(float sample, float& phase, float freq, int sample_rate) {
            if (freq <= 0.0f) return sample;
            float result = sample * std::sin(phase * 2.0f * 3.14159f);
            phase += freq / static_cast<float>(sample_rate);
            if (phase >= 1.0f) phase -= 1.0f;
            return result;
        }
        
        static float reverb_comb(float sample, float amount, std::vector<float>& buffer, size_t& pos) {
            if (amount <= 0.0f || buffer.empty()) return sample;
            float delayed = buffer[pos];
            float output = sample + delayed * amount * 0.6f;
            buffer[pos] = sample + delayed * 0.5f;
            pos = (pos + 1) % buffer.size();
            return output;
        }
        
        static float apply_reverb(float sample, float amount, FilterState& state) {
            if (amount <= 0.0f) return sample;
            float rev1 = reverb_comb(sample, amount, state.reverb_buf1, state.rev_pos1);
            float rev2 = reverb_comb(sample, amount * 0.8f, state.reverb_buf2, state.rev_pos2);
            float rev3 = reverb_comb(sample, amount * 0.6f, state.reverb_buf3, state.rev_pos3);
            return (rev1 + rev2 + rev3) / 3.0f;
        }
        
        static void update_echo_buffer(float echo_delay, int sample_rate, FilterState& state) {
            if (echo_delay > 0.0f && std::abs(echo_delay - state.last_echo_delay) > 0.001f) {
                size_t echo_samples = static_cast<size_t>(echo_delay * sample_rate);

                if (echo_samples > 0 && echo_samples != state.echo_buffer_l.size()) {
                    state.echo_buffer_l.resize(echo_samples, 0.0f);
                    state.echo_buffer_r.resize(echo_samples, 0.0f);
                    state.echo_pos = 0;
                }

                state.last_echo_delay = echo_delay;
            }
        }
        
        static void apply_echo(float& left, float& right, float delay, FilterState& state) {
            if (delay <= 0.0f || state.echo_buffer_l.empty()) return;
            float echo_l = state.echo_buffer_l[state.echo_pos];
            float echo_r = state.echo_buffer_r[state.echo_pos];
            state.echo_buffer_l[state.echo_pos] = left + echo_l * state.echo_decay;
            state.echo_buffer_r[state.echo_pos] = right + echo_r * state.echo_decay;
            left += echo_l * 0.5f;
            right += echo_r * 0.5f;
            state.echo_pos = (state.echo_pos + 1) % state.echo_buffer_l.size();
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
        AudioProcessor::FilterState fx_state;
        fx_state.init(sample_rate);
        bool source_exhausted = false;
        size_t tail_samples_written = 0;
        size_t max_tail_samples = 0;
        
        for (int i = 0; i < NUM_BUFFERS; i++) {
            stereo_buffers[i].resize(CHUNK_SAMPLES * 2);
            headers[i] = {};
            headers[i].lpData = reinterpret_cast<LPSTR>(stereo_buffers[i].data());
            headers[i].dwBufferLength = static_cast<DWORD>(stereo_buffers[i].size() * sizeof(int16_t));
            headers[i].dwFlags = 0;
            headers[i].dwUser = i;
            waveOutPrepareHeader(hWaveOut, &headers[i], sizeof(WAVEHDR));
        }

        int buffers_in_use = 0;

        auto fill_buffer = [&](int buffer_idx) -> size_t {
            float x = std::clamp<float>(state->position_3d[0].load(), -1.0f, 1.0f);
            float y = std::clamp<float>(state->position_3d[1].load(), -1.0f, 1.0f);
            float z = std::clamp<float>(state->position_3d[2].load(), -1.0f, 1.0f);
            float base_volume = std::clamp<float>(state->volume.load(), 0.0f, 1.0f);
            std::vector<std::pair<std::string, float>> effects;

            {
                std::lock_guard<std::mutex> fx_lock(state->effects_mutex);
                effects = state->effects_chain;
            }

            std::unordered_map<std::string, float> fx_map;

            for (const auto& [name, value] : effects) {
                fx_map[name] = value;
            }

            float pitch = 1.0f;
            auto pit = fx_map.find("pitch_shift");
            if (pit != fx_map.end()) pitch = std::clamp(pit->second, 0.5f, 2.0f);
            auto echo_it = fx_map.find("echo_delay");
            float echo_delay = (echo_it != fx_map.end()) ? std::clamp(echo_it->second, 0.0f, 1.0f) : 0.0f;
            AudioProcessor::update_echo_buffer(echo_delay, sample_rate, fx_state);
            auto decay_it = fx_map.find("echo_decay");

            if (decay_it != fx_map.end()) {
                fx_state.echo_decay = std::clamp(decay_it->second, 0.0f, 0.95f);
            }

            float y_factor = 0.3f + ((y + 1.0f) / 2.0f) * 0.7f;
            float z_factor = 1.0f - (std::abs(z) * 0.2f);
            float distance_volume = base_volume * y_factor * z_factor;
            float pan_position = (x + 1.0f) / 2.0f;
            float left_gain = distance_volume * std::cos(pan_position * PI_2);
            float right_gain = distance_volume * std::sin(pan_position * PI_2);
            size_t samples_to_process;

            if (!source_exhausted) {
                float remaining_source = static_cast<float>(total_samples) - fx_state.pitch_accumulator;
                size_t max_output = (pitch > 0.0f) ? static_cast<size_t>(remaining_source / pitch) : 0;
                samples_to_process = std::min(static_cast<size_t>(CHUNK_SAMPLES), max_output);
            } else {
                size_t remaining_tail = (max_tail_samples > tail_samples_written) ? (max_tail_samples - tail_samples_written) : 0;
                samples_to_process = std::min(static_cast<size_t>(CHUNK_SAMPLES), remaining_tail);
            }
            
            auto& stereo = stereo_buffers[buffer_idx];

            for (size_t i = 0; i < samples_to_process; i++) {
                float mono_f = 0.0f;

                if (!source_exhausted) {
                    float src_idx = fx_state.pitch_accumulator;
                    fx_state.pitch_accumulator += pitch;
                    
                    if (src_idx >= static_cast<float>(total_samples)) {
                        source_exhausted = true;
                        // Calculate tail based on active effects
                        if (echo_delay > 0.0f) {
                            max_tail_samples = static_cast<size_t>(echo_delay * sample_rate * (1.0f / (1.0f - fx_state.echo_decay + 0.05f)));
                        }
                        if (fx_map.count("reverb") && fx_map.at("reverb") > 0.0f) {
                            max_tail_samples = std::max(max_tail_samples, static_cast<size_t>(sample_rate * 0.5f));
                        }
                    } else {
                        size_t idx0 = static_cast<size_t>(src_idx);
                        size_t idx1 = std::min(idx0 + 1, total_samples - 1);
                        float frac = src_idx - static_cast<float>(idx0);
                        mono_f = (audio_copy[idx0] * (1.0f - frac) + audio_copy[idx1] * frac) / 32767.0f;
                    }
                }

                if (source_exhausted) {
                    tail_samples_written++;
                }
                
                // Apply effects in user-specified order (mono effects before panning)
                for (const auto& [name, value] : effects) {
                    if (name == "distortion") {
                        mono_f = AudioProcessor::distort(mono_f, std::clamp(value, 0.0f, 1.0f));
                    } else if (name == "bitcrush") {
                        mono_f = AudioProcessor::bitcrush(mono_f, std::clamp(value, 0.0f, 1.0f));
                    } else if (name == "ring_mod") {
                        mono_f = AudioProcessor::ring_mod(mono_f, fx_state.ring_mod_phase, std::clamp(value, 0.0f, 1000.0f), sample_rate);
                    } else if (name == "noise") {
                        mono_f = AudioProcessor::add_noise(mono_f, std::clamp(value, 0.0f, 1.0f), fx_state.rng_state);
                    } else if (name == "reverb") {
                        mono_f = AudioProcessor::apply_reverb(mono_f, std::clamp(value, 0.0f, 1.0f), fx_state);
                    }
                }

                float left_f = mono_f * left_gain;
                float right_f = mono_f * right_gain;

                for (const auto& [name, value] : effects) {
                    if (name == "lowpass") {
                        float cutoff = std::clamp(value, 0.0f, 1.0f);
                        left_f = AudioProcessor::lowpass(left_f, cutoff, fx_state.lp_prev_l);
                        right_f = AudioProcessor::lowpass(right_f, cutoff, fx_state.lp_prev_r);
                    } else if (name == "highpass") {
                        float cutoff = std::clamp(value, 0.0f, 1.0f);
                        left_f = AudioProcessor::highpass(left_f, cutoff, fx_state.hp_prev_in_l, fx_state.hp_prev_out_l);
                        right_f = AudioProcessor::highpass(right_f, cutoff, fx_state.hp_prev_in_r, fx_state.hp_prev_out_r);
                    } else if (name == "echo_delay") {
                        AudioProcessor::apply_echo(left_f, right_f, std::clamp(value, 0.0f, 1.0f), fx_state);
                    }
                }
                
                stereo[i * 2] = static_cast<int16_t>(std::clamp(left_f * 32767.0f, -32767.0f, 32767.0f));
                stereo[i * 2 + 1] = static_cast<int16_t>(std::clamp(right_f * 32767.0f, -32767.0f, 32767.0f));
            }

            for (size_t i = samples_to_process; i < CHUNK_SAMPLES; i++) {
                stereo[i * 2] = 0;
                stereo[i * 2 + 1] = 0;
            }

            headers[buffer_idx].dwBufferLength = static_cast<DWORD>(samples_to_process * 2 * sizeof(int16_t));
            return samples_to_process;
        };

        for (int i = 0; i < NUM_BUFFERS && (!source_exhausted || tail_samples_written < max_tail_samples); i++) {
            size_t processed = fill_buffer(i);

            if (processed > 0) {
                MMRESULT result = waveOutWrite(hWaveOut, &headers[i], sizeof(WAVEHDR));

                if (result != MMSYSERR_NOERROR) {
                    int error_code = result;
                    
                    MainThreadScheduler::instance().schedule([error_code]() {
                        report_error("KH - TTS Framework: waveOutWrite failed with error code: " + std::to_string(error_code));
                    });

                    break;
                }

                buffers_in_use++;
            }
        }

        while (buffers_in_use > 0 && !state->should_stop) {
            for (int i = 0; i < NUM_BUFFERS; i++) {
                if (headers[i].dwFlags & WHDR_DONE) {
                    headers[i].dwFlags &= ~WHDR_DONE;
                    buffers_in_use--;

                    if (!source_exhausted || tail_samples_written < max_tail_samples) {
                        size_t processed = fill_buffer(i);
                        
                        if (processed > 0) {
                            waveOutWrite(hWaveOut, &headers[i], sizeof(WAVEHDR));
                            buffers_in_use++;
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        waveOutReset(hWaveOut);

        for (int i = 0; i < NUM_BUFFERS; i++) {
            waveOutUnprepareHeader(hWaveOut, &headers[i], sizeof(WAVEHDR));
        }

        waveOutClose(hWaveOut);
        state->finished_was_stopped = state->should_stop.load();
        state->pending_finished_event = true;
        state->is_playing = false;
    }
    
    std::deque<GenerationRequest> generation_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::thread generation_thread;
    std::atomic<bool> generation_thread_running{false};

    // Helper functions to schedule events on main thread
    void schedule_tts_generated_event(const std::string& speaker_id, const std::string& text, float duration) {
        MainThreadScheduler::instance().schedule([speaker_id, text, duration]() {
            auto_array<game_value> tts_data;
            tts_data.push_back(game_value(speaker_id));
            tts_data.push_back(game_value(text));
            tts_data.push_back(game_value(duration));
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
        while (generation_thread_running) {
            GenerationRequest request;
            bool has_request = false;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                queue_cv.wait_for(lock, std::chrono::milliseconds(16), [this] {
                    return !generation_queue.empty() || !generation_thread_running;
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
                    float duration_for_event = static_cast<float>(audio->n) / static_cast<float>(current_sample_rate);

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

                        {
                            std::lock_guard<std::mutex> fx_lock(new_state->effects_mutex);
                            new_state->effects_chain = request.effects_chain;
                        }
                            
                        speaker_states[request.speaker_id] = std::move(new_state);
                        speaker_states[request.speaker_id]->playback_thread = std::thread(audio_playback_thread, speaker_states[request.speaker_id].get(), current_sample_rate);
                        should_start_playback = true;
                    }

                    if (should_start_playback) {
                        schedule_tts_generated_event(speaker_id_for_event, text_for_event, duration_for_event);
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
        int num_threads = 4,
        float noise_scale = 0.667f,
        float noise_scale_w = 0.8f,
        float length_scale = 1.0f
    ) {
        std::string log_message;
        bool success = false;
        std::string resolved_model_name;
        int loaded_sample_rate = 0;
        int loaded_num_speakers = 0;
        
        {
            std::lock_guard<std::mutex> lock(tts_handle_mutex);

            {
                std::lock_guard<std::mutex> queue_lock(queue_mutex);
                generation_queue.clear();
            }
            
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

                if (!model_path.empty()) {
                    std::filesystem::path espeak_data_path = TTSModelDiscovery::find_espeak_data(model_path);
                    
                    // Scan for all model files in one pass
                    std::string model_file_path;
                    std::string tokens_file_path;
                    std::string acoustic_path;
                    std::string vocoder_path;
                    std::string lexicon_path;

                    for (const auto& entry : std::filesystem::directory_iterator(model_path)) {
                        if (!entry.is_regular_file()) continue;  
                        std::string filename = entry.path().filename().string();
                        std::string lower_filename = filename;

                        std::transform(lower_filename.begin(), lower_filename.end(), 
                                       lower_filename.begin(), ::tolower);
                        
                        if (lower_filename.ends_with(".onnx")) {
                            if (lower_filename.find("acoustic") != std::string::npos) {
                                acoustic_path = entry.path().string();
                            }
                            else if (lower_filename.find("vocoder") != std::string::npos) {
                                vocoder_path = entry.path().string();
                            }
                            else {
                                model_file_path = entry.path().string();
                            }
                        }
                        else if (lower_filename == "tokens.txt") {
                            tokens_file_path = entry.path().string();
                        }
                        else if (lower_filename == "lexicon.txt") {
                            lexicon_path = entry.path().string();
                        }
                    }
                    
                    bool is_matcha = !acoustic_path.empty() && !vocoder_path.empty();
                    bool is_vits = !model_file_path.empty();
                    
                    if (!is_matcha && !is_vits) {
                        log_message = "KH - TTS Framework: No valid model files found in directory";
                    } else {
                        std::string espeak_data_str = espeak_data_path.empty() ? "" : espeak_data_path.string();
                        SherpaOnnxOfflineTtsConfig config;
                        memset(&config, 0, sizeof(config));
                        
                        if (is_matcha) {
                            // Matcha TTS configuration
                            config.model.matcha.acoustic_model = acoustic_path.c_str();
                            config.model.matcha.vocoder = vocoder_path.c_str();
                            config.model.matcha.lexicon = lexicon_path.empty() ? "" : lexicon_path.c_str();
                            config.model.matcha.tokens = tokens_file_path.empty() ? "" : tokens_file_path.c_str();
                            config.model.matcha.data_dir = espeak_data_str.c_str();
                            config.model.matcha.noise_scale = noise_scale;
                            config.model.matcha.length_scale = length_scale;
                        } else {
                            // VITS configuration
                            config.model.vits.model = model_file_path.c_str();
                            config.model.vits.lexicon = lexicon_path.empty() ? "" : lexicon_path.c_str();
                            config.model.vits.tokens = tokens_file_path.empty() ? "" : tokens_file_path.c_str();
                            config.model.vits.data_dir = espeak_data_str.c_str();
                            config.model.vits.noise_scale = noise_scale;
                            config.model.vits.noise_scale_w = noise_scale_w;
                            config.model.vits.length_scale = length_scale;
                            config.model.vits.dict_dir = "";
                        }

                        config.model.num_threads = num_threads;
                        config.model.debug = 0;
                        config.model.provider = "directml";
                        config.max_num_sentences = 1;
                        config.rule_fsts = "";
                        config.rule_fars = "";
                        tts_handle = std::shared_ptr<const SherpaOnnxOfflineTts>(SherpaOnnxCreateOfflineTts(&config), TtsHandleDeleter{});
                        
                        if (!tts_handle) {
                            log_message = "KH - TTS Framework: Failed to create TTS instance";
                        } else {
                            num_speakers = SherpaOnnxOfflineTtsNumSpeakers(tts_handle.get());
                            sample_rate = SherpaOnnxOfflineTtsSampleRate(tts_handle.get());
                            loaded_num_speakers = num_speakers;
                            loaded_sample_rate = sample_rate;
                            is_initialized_flag.store(true, std::memory_order_release);
                            success = true;
                            
                            log_message = "KH - TTS Framework: Model loaded successfully - " + model_path.string() + 
                                " | Type: " + (is_matcha ? "Matcha" : "VITS") +
                                " | Speakers: " + std::to_string(loaded_num_speakers) + 
                                " | Sample Rate: " + std::to_string(loaded_sample_rate) + " Hz";
                        }
                    }
                }
            } catch (const std::exception& e) {
                log_message = "KH - TTS Framework: Model loading exception: " + std::string(e.what());
            }
        }

        if (!log_message.empty()) {
            std::string msg = log_message;

            MainThreadScheduler::instance().schedule([msg, success]() {
                if (success) {
                    sqf::diag_log(msg);
                } else {
                    report_error(msg);
                }
            });
        }

        if (success) {
            if (!cleanup_thread_running) {
                cleanup_thread_running.store(true, std::memory_order_release);
                cleanup_thread = std::thread(&TTSFramework::cleanup_worker, this);
            }
            
            if (!generation_thread_running) {
                generation_thread_running.store(true, std::memory_order_release);
                generation_thread = std::thread(&TTSFramework::generation_worker, this);
            }
        }
        
        return success;
    }

    bool speak(const std::string& speaker_id, const std::string& text, 
            float x, float y, float z, float volume, float speed = 1.0f, int sid = 0,
            const std::vector<std::pair<std::string, float>>& effects = {}) {
        if (!is_initialized_flag) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - TTS Framework: Cannot start speak - no model loaded");
            });
            
            return false;
        }

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
        request.effects_chain = effects;
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex);

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

    bool update_speaker(const std::string& speaker_id, float x, float y, float z, float volume,
                        const std::vector<std::pair<std::string, float>>& effects = {}) {
        if (speaker_id.empty()) return false;
        std::shared_lock<std::shared_mutex> lock(speaker_mutex);
        auto it = speaker_states.find(speaker_id);
        if (it == speaker_states.end()) return false;
        auto& state = it->second;
        state->position_3d[0] = x;
        state->position_3d[1] = y;
        state->position_3d[2] = z;
        state->volume = std::clamp(volume, 0.0f, 1.0f);
        
        if (!effects.empty()) {
            std::lock_guard<std::mutex> fx_lock(state->effects_mutex);
            state->effects_chain = effects;
        }
        
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
                state_kept_alive->pending_finished_event.store(false, std::memory_order_release);
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

    static std::vector<std::pair<std::string, float>> parse_effects_from_args(const game_value& args, size_t start_index) {
        std::vector<std::pair<std::string, float>> effects;
        if (args.type_enum() != game_data_type::ARRAY) return effects;
        auto& arr = args.to_array();

        for (size_t i = start_index; i < arr.size(); i++) {
            if (arr[i].type_enum() == game_data_type::ARRAY) {
                auto& effect_arr = arr[i].to_array();

                if (effect_arr.size() >= 2 && 
                    effect_arr[0].type_enum() == game_data_type::STRING &&
                    effect_arr[1].type_enum() == game_data_type::SCALAR) {
                    std::string name = effect_arr[0];
                    float value = effect_arr[1];
                    effects.emplace_back(name, value);
                }
            }
        }
        
        return effects;
    }

    bool is_initialized() {
        return is_initialized_flag;
    }

    void cleanup() {
        generation_thread_running.store(false, std::memory_order_release);
        cleanup_thread_running.store(false, std::memory_order_release);
        queue_cv.notify_all();
        cleanup_cv.notify_all();

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
                if (state) {
                    state->should_stop.store(true, std::memory_order_release);
                    
                    if (state->playback_thread.joinable()) {
                        remaining_threads.push_back(std::move(state->playback_thread));
                        remaining_states.push_back(std::move(state));
                    }
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