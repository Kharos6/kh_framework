#pragma once

using namespace intercept;
using namespace intercept::types;

constexpr int STT_SAMPLE_RATE = 16000;  // Standard ASR sample rate
constexpr int STT_CHANNELS = 1;         // Mono for ASR
constexpr int STT_BITS_PER_SAMPLE = 16;

class STTModelDiscovery {
public:
    static std::vector<std::filesystem::path> find_all_stt_model_directories() {
        std::vector<std::filesystem::path> search_paths;
        
        // Priority 1: Documents folder
        try {
            char docs_path[MAX_PATH];

            if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs_path) == S_OK) {
                std::filesystem::path docs_stt_models = std::filesystem::path(docs_path) / "Arma 3" / "kh_framework" / "stt_models";

                if (std::filesystem::exists(docs_stt_models)) {
                    search_paths.push_back(docs_stt_models);
                }
            }
        } catch (...) {}

        // Priority 2: Mod folders
        auto mod_stt_dirs = ModFolderSearcher::find_directories_in_mods("stt_models");
        search_paths.insert(search_paths.end(), mod_stt_dirs.begin(), mod_stt_dirs.end());
        return search_paths;
    }

    static std::filesystem::path find_model(const std::string& model_name) {
        auto search_paths = find_all_stt_model_directories();
        
        for (const auto& base_path : search_paths) {
            std::filesystem::path model_path = base_path / model_name;

            if (std::filesystem::exists(model_path) && std::filesystem::is_directory(model_path)) {
                return model_path;
            }
        }
        
        return std::filesystem::path();
    }

    static std::filesystem::path find_any_model() {
        auto search_paths = find_all_stt_model_directories();
        
        for (const auto& base_path : search_paths) {
            try {
                if (!std::filesystem::exists(base_path) || !std::filesystem::is_directory(base_path)) {
                    continue;
                }
                
                for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
                    if (!entry.is_directory()) continue;
                    std::filesystem::path model_path = entry.path();
                    bool has_encoder = false;
                    bool has_decoder = false;
                    bool has_joiner = false;
                    bool has_tokens = false;
                    
                    for (const auto& file : std::filesystem::directory_iterator(model_path)) {
                        if (!file.is_regular_file()) continue;
                        
                        std::string filename = file.path().filename().string();
                        std::string lower_filename = filename;

                        std::transform(lower_filename.begin(), lower_filename.end(), 
                                     lower_filename.begin(), ::tolower);
                        
                        if (lower_filename.find("encoder") != std::string::npos && 
                            lower_filename.ends_with(".onnx")) {
                            has_encoder = true;
                        }
                        else if (lower_filename.find("decoder") != std::string::npos && 
                                 lower_filename.ends_with(".onnx")) {
                            has_decoder = true;
                        }
                        else if (lower_filename.find("joiner") != std::string::npos && 
                                 lower_filename.ends_with(".onnx")) {
                            has_joiner = true;
                        }
                        else if (lower_filename == "tokens.txt") {
                            has_tokens = true;
                        }
                    }
                    
                    if (has_encoder && has_decoder && has_joiner && has_tokens) {
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

class AudioCaptureBuffer {
private:
    std::vector<int16_t> buffer;
    std::mutex buffer_mutex;
    std::atomic<bool> is_recording{false};
    std::atomic<bool> should_clear{false};

public:
    void start_recording() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        buffer.clear();
        is_recording = true;
        should_clear = false;
    }

    void stop_recording() {
        is_recording = false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        buffer.clear();
        should_clear = false;
    }

    void append_samples(const int16_t* samples, size_t count) {
        if (!is_recording) return;
        std::lock_guard<std::mutex> lock(buffer_mutex);

        if (should_clear) {
            buffer.clear();
            should_clear = false;
        }

        buffer.insert(buffer.end(), samples, samples + count);
    }

    std::vector<int16_t> get_buffer_copy() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        return buffer;
    }

    size_t get_sample_count() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        return buffer.size();
    }

    bool recording() const {
        return is_recording;
    }
};

class STTFramework {
private:
    STTFramework() = default;
    ~STTFramework() { cleanup(); }
    STTFramework(const STTFramework&) = delete;
    STTFramework& operator=(const STTFramework&) = delete;
    const SherpaOnnxOfflineRecognizer* recognizer_handle = nullptr;
    std::mutex stt_mutex;
    std::string current_model_name;
    int sample_rate = STT_SAMPLE_RATE;
    AudioCaptureBuffer capture_buffer;
    std::thread capture_thread;
    std::atomic<bool> capture_thread_running{false};
    std::atomic<bool> is_capturing{false};
    std::thread processing_thread;
    std::atomic<bool> processing_thread_running{false};
    std::chrono::steady_clock::time_point capture_start_time;
    std::mutex capture_time_mutex;
    static constexpr int MAX_CAPTURE_DURATION_MS = 120000;
    std::mutex processing_mutex;
    std::condition_variable processing_cv;
    std::deque<std::vector<int16_t>> processing_queue;

    void audio_capture_worker() {
        CoInitializeEx(NULL, COINIT_MULTITHREADED);
        HWAVEIN hWaveIn = NULL;
        WAVEFORMATEX wfx = {};
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = STT_CHANNELS;
        wfx.nSamplesPerSec = STT_SAMPLE_RATE;
        wfx.wBitsPerSample = STT_BITS_PER_SAMPLE;
        wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
        
        if (result != MMSYSERR_NOERROR) {
            try {
                intercept::client::invoker_lock lock;
                report_error("KH - STT Framework: Failed to open audio input device");
            } catch (...) {}

            CoUninitialize();
            return;
        }

        // Prepare buffers (100ms chunks)
        constexpr int BUFFER_SIZE = STT_SAMPLE_RATE / 10 * STT_CHANNELS;
        constexpr int NUM_BUFFERS = 4;
        std::vector<std::vector<int16_t>> buffers(NUM_BUFFERS);
        std::vector<WAVEHDR> headers(NUM_BUFFERS);
        
        for (int i = 0; i < NUM_BUFFERS; i++) {
            buffers[i].resize(BUFFER_SIZE);
            headers[i] = {};
            headers[i].lpData = reinterpret_cast<LPSTR>(buffers[i].data());
            headers[i].dwBufferLength = BUFFER_SIZE * sizeof(int16_t);
            headers[i].dwFlags = 0;
            waveInPrepareHeader(hWaveIn, &headers[i], sizeof(WAVEHDR));
            waveInAddBuffer(hWaveIn, &headers[i], sizeof(WAVEHDR));
        }

        waveInStart(hWaveIn);

        while (capture_thread_running) {
            // Check for completed buffers
            for (int i = 0; i < NUM_BUFFERS; i++) {
                if (headers[i].dwFlags & WHDR_DONE) {
                    size_t samples_captured = headers[i].dwBytesRecorded / sizeof(int16_t);
                    
                    if (samples_captured > 0) {
                        capture_buffer.append_samples(buffers[i].data(), samples_captured);
                        
                        // Check for timeout
                        if (is_capturing) {
                            std::lock_guard<std::mutex> lock(capture_time_mutex);
                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - capture_start_time).count();
                            
                            if (elapsed >= MAX_CAPTURE_DURATION_MS) {
                                try {
                                    intercept::client::invoker_lock inv_lock;
                                    sqf::diag_log("KH - STT Framework: 120 second timeout - auto-stopping");
                                } catch (...) {}

                                is_capturing = false;
                                capture_buffer.stop_recording();
                                std::vector<int16_t> audio_data = capture_buffer.get_buffer_copy();
                                
                                if (!audio_data.empty()) {
                                    std::lock_guard<std::mutex> proc_lock(processing_mutex);
                                    processing_queue.push_back(std::move(audio_data));
                                    processing_cv.notify_one();
                                    
                                    try {
                                        intercept::client::invoker_lock inv_lock;
                                        sqf::diag_log("KH - STT Framework: Audio queued (timeout)");
                                    } catch (...) {}
                                }
                                
                                capture_buffer.clear();
                            }
                        }
                    }
                    
                    // Reset and re-add buffer
                    headers[i].dwFlags = 0;
                    headers[i].dwBytesRecorded = 0;

                    MMRESULT prepare_result = waveInPrepareHeader(hWaveIn, &headers[i], sizeof(WAVEHDR));
                    if (prepare_result != MMSYSERR_NOERROR) {
                        try {
                            intercept::client::invoker_lock lock;
                            report_error("KH - STT Framework: Audio device error (prepare failed) - code: " + 
                                        std::to_string(prepare_result) + " - stopping capture");
                        } catch (...) {}
                        
                        capture_thread_running = false;
                        break;
                    }

                    MMRESULT add_result = waveInAddBuffer(hWaveIn, &headers[i], sizeof(WAVEHDR));

                    if (add_result != MMSYSERR_NOERROR) {
                        try {
                            intercept::client::invoker_lock lock;
                            report_error("KH - STT Framework: Audio device error (add buffer failed) - code: " + 
                                        std::to_string(add_result) + " - stopping capture");
                        } catch (...) {}

                        capture_thread_running = false;
                        break;
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        waveInStop(hWaveIn);
        waveInReset(hWaveIn);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        for (int i = 0; i < NUM_BUFFERS; i++) {
            // Check if buffer needs additional waiting, just in case I guess
            int timeout = 0;

            while ((headers[i].dwFlags & WHDR_PREPARED) && timeout < 10) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                timeout++;
            }
            
            if (headers[i].dwFlags & WHDR_PREPARED) {
                waveInUnprepareHeader(hWaveIn, &headers[i], sizeof(WAVEHDR));
            }
        }

        waveInClose(hWaveIn);       
        CoUninitialize();
    }

    void processing_worker() {
        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        while (processing_thread_running) {
            std::vector<int16_t> audio_data;
            bool has_data = false;
            
            {
                std::unique_lock<std::mutex> lock(processing_mutex);
                processing_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return !processing_queue.empty() || !processing_thread_running;
                });
                
                if (!processing_queue.empty()) {
                    audio_data = std::move(processing_queue.front());
                    processing_queue.pop_front();
                    has_data = true;
                }
            }
            
            if (has_data && !audio_data.empty()) {
                try {
                    std::string transcription = transcribe_audio(audio_data);
                    
                    if (!transcription.empty()) {
                        // Send to SQF via CBA event
                        try {
                            intercept::client::invoker_lock lock;
                            auto_array<game_value> stt_data;
                            stt_data.push_back(game_value(sqf::get_variable(sqf::mission_namespace(), "kh_var_playerunit", sqf::obj_null())));
                            stt_data.push_back(game_value(transcription));
                            raw_call_sqf_args_native_no_return(g_compiled_stt_transcription_event, game_value(std::move(stt_data)));
                        } catch (const std::exception& e) {
                            report_error("KH - STT Framework: Failed to send transcription event: " + 
                                       std::string(e.what()));
                        }
                    }
                } catch (const std::exception& e) {
                    try {
                        intercept::client::invoker_lock lock;
                        report_error("KH - STT Framework: Transcription failed: " + std::string(e.what()));
                    } catch (...) {}
                }
            }
        }

        CoUninitialize();
    }

    std::string transcribe_audio(const std::vector<int16_t>& audio_data) {
        std::lock_guard<std::mutex> lock(stt_mutex);
        
        if (!recognizer_handle) {
            throw std::runtime_error("STT not initialized");
        }

        std::vector<float> float_samples(audio_data.size());

        for (size_t i = 0; i < audio_data.size(); i++) {
            float_samples[i] = static_cast<float>(audio_data[i]) / 32768.0f;
        }

        const SherpaOnnxOfflineStream* stream = SherpaOnnxCreateOfflineStream(recognizer_handle);
        
        if (!stream) {
            throw std::runtime_error("Failed to create recognition stream");
        }

        // Feed audio to stream
        SherpaOnnxAcceptWaveformOffline(stream, sample_rate, 
                                       float_samples.data(), 
                                       static_cast<int32_t>(float_samples.size()));

        // Decode
        SherpaOnnxDecodeOfflineStream(recognizer_handle, stream);

        // Get result
        const SherpaOnnxOfflineRecognizerResult* result = SherpaOnnxGetOfflineStreamResult(stream);

        std::string transcription;

        if (result && result->text) {
            transcription = result->text;
        }

        SherpaOnnxDestroyOfflineRecognizerResult(result);
        SherpaOnnxDestroyOfflineStream(stream);

        if (!transcription.empty()) {
            size_t start = transcription.find_first_not_of(" \t\n\r");
            size_t end = transcription.find_last_not_of(" \t\n\r");
            
            if (start != std::string::npos && end != std::string::npos) {
                transcription = transcription.substr(start, end - start + 1);
            } else {
                transcription.clear();
            }
        }

        return transcription;
    }

    bool load_model(const std::string& model_name, 
                   const std::string& provider = "dml",
                   int num_threads = 4) {
        std::lock_guard<std::mutex> lock(stt_mutex);

        // Cleanup existing model
        if (recognizer_handle) {
            SherpaOnnxDestroyOfflineRecognizer(recognizer_handle);
            recognizer_handle = nullptr;
        }

        try {
            std::filesystem::path model_path;
            std::string resolved_model_name = model_name;
            
            if (model_name.empty()) {
                // Find any available model
                model_path = STTModelDiscovery::find_any_model();
                
                if (model_path.empty()) {
                    intercept::client::invoker_lock inv_lock;
                    report_error("KH - STT Framework: No STT models found in any search location");
                    return false;
                }
                
                resolved_model_name = model_path.filename().string();
            } else {
                model_path = STTModelDiscovery::find_model(model_name);
                
                if (model_path.empty()) {
                    intercept::client::invoker_lock inv_lock;
                    report_error("KH - STT Framework: Model not found: " + model_name);
                    return false;
                }
            }

            std::string encoder_path;
            std::string decoder_path;
            std::string joiner_path;
            std::string tokens_path;
            
            for (const auto& entry : std::filesystem::directory_iterator(model_path)) {
                if (!entry.is_regular_file()) continue;
                std::string filename = entry.path().filename().string();
                std::string lower_filename = filename;
                std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
                
                if (lower_filename.find("encoder") != std::string::npos && 
                    lower_filename.ends_with(".onnx")) {
                    encoder_path = entry.path().string();
                }
                else if (lower_filename.find("decoder") != std::string::npos && 
                         lower_filename.ends_with(".onnx")) {
                    decoder_path = entry.path().string();
                }
                else if (lower_filename.find("joiner") != std::string::npos && 
                         lower_filename.ends_with(".onnx")) {
                    joiner_path = entry.path().string();
                }
                else if (lower_filename == "tokens.txt") {
                    tokens_path = entry.path().string();
                }
            }

            if (encoder_path.empty() || decoder_path.empty() || 
                joiner_path.empty() || tokens_path.empty()) {
                intercept::client::invoker_lock lock;
                report_error("KH - STT Framework: Incomplete model files. Required: encoder.onnx, decoder.onnx, joiner.onnx, tokens.txt");
                return false;
            }

            SherpaOnnxOfflineRecognizerConfig config;
            memset(&config, 0, sizeof(config));
            
            // Model configuration (Transducer)
            config.model_config.transducer.encoder = encoder_path.c_str();
            config.model_config.transducer.decoder = decoder_path.c_str();
            config.model_config.transducer.joiner = joiner_path.c_str();
            config.model_config.tokens = tokens_path.c_str();
            config.model_config.num_threads = num_threads;
            config.model_config.provider = provider.c_str();
            config.model_config.debug = 0;
            
            // Decoding configuration
            config.decoding_method = "greedy_search";
            config.max_active_paths = 4;

            std::string actual_provider = provider;
            recognizer_handle = SherpaOnnxCreateOfflineRecognizer(&config);
            
            // Fallback to CPU if DML fails
            if (!recognizer_handle && _stricmp(actual_provider.c_str(), "dml") == 0) {
                intercept::client::invoker_lock lock;
                sqf::diag_log("KH - STT Framework: DirectML not supported, falling back to CPU");
                actual_provider = "cpu";
                config.model_config.provider = actual_provider.c_str();
                recognizer_handle = SherpaOnnxCreateOfflineRecognizer(&config);
            }
            
            if (!recognizer_handle) {
                intercept::client::invoker_lock lock;
                report_error("KH - STT Framework: Failed to create recognizer");
                return false;
            }

            // Sample rate is set in config (16000 Hz standard for speech)
            sample_rate = 16000;
            current_model_name = resolved_model_name;

            {
                intercept::client::invoker_lock lock;
                std::string provider_name = actual_provider;
                
                std::transform(provider_name.begin(), provider_name.end(), 
                             provider_name.begin(), ::toupper);
                
                sqf::diag_log("KH - STT Framework: Model loaded successfully - " + resolved_model_name + 
                    " | Provider: " + provider_name + 
                    " | Sample Rate: " + std::to_string(sample_rate) + " Hz"
                );
            }

            if (!capture_thread_running) {
                capture_thread_running = true;
                capture_thread = std::thread(&STTFramework::audio_capture_worker, this);
            }

            if (!processing_thread_running) {
                processing_thread_running = true;
                processing_thread = std::thread(&STTFramework::processing_worker, this);
            }

            return true;
        } catch (const std::exception& e) {
            intercept::client::invoker_lock lock;
            report_error("KH - STT Framework: Model loading exception: " + std::string(e.what()));
            return false;
        }
    }

    bool is_initialized() const {
        return recognizer_handle != nullptr;
    }

    bool is_capturing_audio() const {
        return is_capturing;
    }

    void cleanup() {
        is_capturing = false;
        capture_thread_running = false;
        processing_thread_running = false;
        processing_cv.notify_all();
        std::thread capture_to_join;
        std::thread processing_to_join;
        
        if (capture_thread.joinable()) {
            capture_to_join = std::move(capture_thread);
        }
        
        if (processing_thread.joinable()) {
            processing_to_join = std::move(processing_thread);
        }

        if (capture_to_join.joinable()) {
            capture_to_join.join();
        }
        
        if (processing_to_join.joinable()) {
            processing_to_join.join();
        }

        std::lock_guard<std::mutex> lock(stt_mutex);
        
        if (recognizer_handle) {
            SherpaOnnxDestroyOfflineRecognizer(recognizer_handle);
            recognizer_handle = nullptr;
        }

        capture_buffer.clear();
        
        {
            std::lock_guard<std::mutex> proc_lock(processing_mutex);
            processing_queue.clear();
        }
        
        current_model_name.clear();
    }

    bool start_capture_manual() {
        if (!recognizer_handle) {
            try {
                intercept::client::invoker_lock lock;
                report_error("KH - STT Framework: Cannot start capture - no model loaded");
            } catch (...) {}

            return false;
        }

        if (!capture_thread_running) {
            try {
                intercept::client::invoker_lock lock;
                report_error("KH - STT Framework: Capture thread not running");
            } catch (...) {}

            return false;
        }

        if (is_capturing) {
            return false;
        }

        // Clear and start recording in buffer
        capture_buffer.clear();
        capture_buffer.start_recording();
        is_capturing = true;

        {
            std::lock_guard<std::mutex> lock(capture_time_mutex);
            capture_start_time = std::chrono::steady_clock::now();
        }

        return true;
    }

    bool stop_capture_manual() {
        if (!is_capturing) {
            return false;
        }

        is_capturing = false;
        capture_buffer.stop_recording();
        std::vector<int16_t> audio_data = capture_buffer.get_buffer_copy();

        // Queue for processing if we have data
        if (!audio_data.empty()) {
            std::lock_guard<std::mutex> proc_lock(processing_mutex);
            processing_queue.push_back(std::move(audio_data));
            processing_cv.notify_one();
            return true;
        } else {
            return false;
        }
    }
    
public:
    static STTFramework& instance() {
        static STTFramework instance;
        return instance;
    }

    bool load_model_public(const std::string& model_name, const std::string& provider = "dml", int num_threads = 4) {
        return load_model(model_name, provider, num_threads);
    }

    bool is_initialized_public() const {
        return is_initialized();
    }

    bool is_capturing_audio_public() const {
        return is_capturing_audio();
    }

    bool start_capture_public() {
        return start_capture_manual();
    }

    bool stop_capture_public() {
        return stop_capture_manual();
    }

    void cleanup_public() {
        cleanup();
    }
};