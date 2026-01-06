#pragma once

using namespace intercept;
using namespace intercept::types;

constexpr int STT_SAMPLE_RATE = 16000;  // Standard ASR sample rate
constexpr int STT_CHANNELS = 1;         // Mono for ASR
constexpr int STT_BITS_PER_SAMPLE = 16;

class STTModelDiscovery {
public:
    enum class STTModelType {
        UNKNOWN,
        TRANSDUCER,
        WHISPER,
        PARAFORMER,
        SENSEVOICE,
        MOONSHINE,
        NEMO_CTC,
        ZIPFORMER_CTC,
        TDNN,
        WENET_CTC
    };

    static std::string stt_model_type_to_string(STTModelType type) {
        switch (type) {
            case STTModelType::TRANSDUCER: return "Transducer";
            case STTModelType::WHISPER: return "Whisper";
            case STTModelType::PARAFORMER: return "Paraformer";
            case STTModelType::SENSEVOICE: return "SenseVoice";
            case STTModelType::MOONSHINE: return "Moonshine";
            case STTModelType::NEMO_CTC: return "NeMo CTC";
            case STTModelType::ZIPFORMER_CTC: return "Zipformer CTC";
            case STTModelType::TDNN: return "TDNN";
            case STTModelType::WENET_CTC: return "Wenet CTC";
            default: return "Unknown";
        }
    }

    static STTModelType detect_stt_model_type(const std::filesystem::path& model_path) {
        if (model_path.empty() || !std::filesystem::exists(model_path) || !std::filesystem::is_directory(model_path)) {
            return STTModelType::UNKNOWN;
        }

        std::string dir_name = model_path.filename().string();
        std::string lower_dir_name = dir_name;
        std::transform(lower_dir_name.begin(), lower_dir_name.end(), lower_dir_name.begin(), ::tolower);
        bool has_tokens = false;
        bool has_encoder = false;
        bool has_decoder = false;
        bool has_joiner = false;
        bool has_generic_model = false;
        bool has_preprocess = false;
        bool has_encode = false;
        bool has_uncached_decode = false;
        bool has_cached_decode = false;
        std::string encoder_path;
        std::string decoder_path;
        std::string joiner_path;
        std::string generic_model_path;
        std::string preprocess_path;
        std::string encode_path;
        std::string uncached_decode_path;
        std::string cached_decode_path;
        std::string tokens_path;
        
        try {
            for (const auto& file : std::filesystem::directory_iterator(model_path)) {
                if (!file.is_regular_file()) continue;
                std::string filename = file.path().filename().string();
                std::string lower_filename = filename;
                std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
                
                if (lower_filename == "tokens.txt") {
                    has_tokens = true;
                    tokens_path = file.path().string();
                }
                else if (lower_filename.ends_with(".onnx")) {
                    // Moonshine detection (must check first - has specific naming)
                    if (lower_filename.find("preprocess") != std::string::npos) {
                        has_preprocess = true;
                        preprocess_path = file.path().string();
                    }
                    else if (lower_filename.find("uncached_decode") != std::string::npos ||
                            lower_filename.find("uncached-decode") != std::string::npos) {
                        has_uncached_decode = true;
                        uncached_decode_path = file.path().string();
                    }
                    else if (lower_filename.find("cached_decode") != std::string::npos ||
                            lower_filename.find("cached-decode") != std::string::npos) {
                        has_cached_decode = true;
                        cached_decode_path = file.path().string();
                    }
                    // Note: Moonshine uses "encode" not "encoder"
                    else if ((lower_filename.find("encode") != std::string::npos && 
                            lower_filename.find("encoder") == std::string::npos) ||
                            lower_filename == "encode.onnx" ||
                            lower_filename == "encode.int8.onnx") {
                        has_encode = true;
                        encode_path = file.path().string();
                    }
                    // Transducer/Whisper components
                    else if (lower_filename.find("encoder") != std::string::npos) {
                        has_encoder = true;
                        encoder_path = file.path().string();
                    }
                    else if (lower_filename.find("decoder") != std::string::npos) {
                        has_decoder = true;
                        decoder_path = file.path().string();
                    }
                    else if (lower_filename.find("joiner") != std::string::npos) {
                        has_joiner = true;
                        joiner_path = file.path().string();
                    }
                    // Generic model.onnx (for Paraformer, SenseVoice, NeMo, etc.)
                    else if (lower_filename == "model.onnx" || 
                            lower_filename == "model.int8.onnx" ||
                            lower_filename.starts_with("model.")) {
                        has_generic_model = true;
                        generic_model_path = file.path().string();
                    }
                }
            }
        } catch (...) {
            return STTModelType::UNKNOWN;
        }
        
        // Detection logic - ORDER MATTERS (most specific first)
        // 1. MOONSHINE: preprocess + encode + uncached_decode + cached_decode
        if (has_preprocess && has_encode && has_uncached_decode && has_cached_decode && has_tokens) {
            return STTModelType::MOONSHINE;
        }
        
        // 2. TRANSDUCER: encoder + decoder + joiner
        if (has_encoder && has_decoder && has_joiner && has_tokens) {
            return STTModelType::TRANSDUCER;
        }
        
        // 3. WHISPER: encoder + decoder (no joiner)
        if (has_encoder && has_decoder && !has_joiner && has_tokens) {
            return STTModelType::WHISPER;
        }
        
        // 4-9. Single model types - use directory name to distinguish
        if (has_generic_model && has_tokens) {
            // Check directory name for model type hints
            
            // SenseVoice
            if (lower_dir_name.find("sense-voice") != std::string::npos ||
                lower_dir_name.find("sensevoice") != std::string::npos ||
                lower_dir_name.find("sense_voice") != std::string::npos) {
                return STTModelType::SENSEVOICE;
            }
            
            // Paraformer
            if (lower_dir_name.find("paraformer") != std::string::npos) {
                return STTModelType::PARAFORMER;
            }
            
            // NeMo CTC (check for both "nemo" and "ctc")
            if (lower_dir_name.find("nemo") != std::string::npos &&
                lower_dir_name.find("ctc") != std::string::npos) {
                return STTModelType::NEMO_CTC;
            }
            
            // Zipformer CTC
            if (lower_dir_name.find("zipformer") != std::string::npos &&
                lower_dir_name.find("ctc") != std::string::npos) {
                return STTModelType::ZIPFORMER_CTC;
            }
            
            // Wenet CTC
            if (lower_dir_name.find("wenet") != std::string::npos) {
                return STTModelType::WENET_CTC;
            }
            
            // TDNN
            if (lower_dir_name.find("tdnn") != std::string::npos) {
                return STTModelType::TDNN;
            }
            
            // NeMo CTC without explicit "ctc" in name (NeMo models are usually CTC)
            if (lower_dir_name.find("nemo") != std::string::npos) {
                return STTModelType::NEMO_CTC;
            }
            
            // Default single-model fallback: Try Paraformer (most common single-model type)
            // This is a reasonable default since Paraformer is widely used
            return STTModelType::PARAFORMER;
        }
        
        return STTModelType::UNKNOWN;
    }

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
                    
                    // Use the new detection function to validate
                    STTModelType model_type = detect_stt_model_type(model_path);
                    
                    if (model_type != STTModelType::UNKNOWN) {
                        return model_path;  // Just return path, not type
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

public:
    void start_recording() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        buffer.clear();
        is_recording = true;
    }

    void stop_recording() {
        is_recording = false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        buffer.clear();
    }

    void append_samples(const int16_t* samples, size_t count) {
        if (!is_recording) return;
        std::lock_guard<std::mutex> lock(buffer_mutex);
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
    std::shared_ptr<const SherpaOnnxOfflineRecognizer> recognizer_handle;
    mutable std::mutex stt_mutex;
    int sample_rate = STT_SAMPLE_RATE;
    AudioCaptureBuffer capture_buffer;
    std::thread capture_thread;
    std::atomic<bool> is_initialized_flag{false};
    std::atomic<bool> capture_thread_running{false};
    std::atomic<bool> capture_thread_alive{false};
    std::atomic<bool> is_capturing{false};
    std::mutex capture_state_mutex;
    std::thread processing_thread;
    std::atomic<bool> processing_thread_running{false};
    std::chrono::steady_clock::time_point capture_start_time;
    std::mutex capture_time_mutex;
    static constexpr int MAX_CAPTURE_DURATION_MS = 30000;
    std::mutex processing_mutex;
    std::condition_variable processing_cv;
    std::deque<std::vector<int16_t>> processing_queue;

    struct RecognizerDeleter {
        void operator()(const SherpaOnnxOfflineRecognizer* p) const {
            if (p) SherpaOnnxDestroyOfflineRecognizer(p);
        }
    };

    void resample_audio(const std::vector<int16_t>& input, uint32_t input_rate,
                        std::vector<int16_t>& output, uint32_t output_rate) {
        if (input.empty() || input_rate == 0 || output_rate == 0) {
            output.clear();
            return;
        }
        
        double ratio = static_cast<double>(input_rate) / static_cast<double>(output_rate);
        size_t output_size = static_cast<size_t>(input.size() / ratio);
        output.resize(output_size);
        
        for (size_t i = 0; i < output_size; i++) {
            double src_idx = i * ratio;
            size_t idx0 = static_cast<size_t>(src_idx);
            size_t idx1 = std::min(idx0 + 1, input.size() - 1);
            double frac = src_idx - idx0;
            
            output[i] = static_cast<int16_t>(
                input[idx0] * (1.0 - frac) + input[idx1] * frac
            );
        }
    }

void audio_capture_worker() {
        capture_thread_alive = true;

        struct ThreadAliveGuard {
            std::atomic<bool>& flag;
            ThreadAliveGuard(std::atomic<bool>& f) : flag(f) {}
            ~ThreadAliveGuard() { flag = false; }
        } alive_guard(capture_thread_alive);
        
        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        struct CoInitGuard {
            ~CoInitGuard() { CoUninitialize(); }
        } co_guard;
        
        HRESULT hr;
        IMMDeviceEnumerator* enumerator = nullptr;
        IMMDevice* device = nullptr;
        IAudioClient* audio_client = nullptr;
        IAudioCaptureClient* capture_client = nullptr;
        WAVEFORMATEX* device_format = nullptr;
        
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                              __uuidof(IMMDeviceEnumerator), (void**)&enumerator);

        if (FAILED(hr)) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - STT Framework: Failed to create device enumerator");
            });

            return;
        }
        
        hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);

        if (FAILED(hr)) {
            enumerator->Release();

            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - STT Framework: Failed to get default capture device");
            });

            return;
        }
        
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audio_client);

        if (FAILED(hr)) {
            device->Release();
            enumerator->Release();

            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - STT Framework: Failed to activate audio client");
            });

            return;
        }
        
        hr = audio_client->GetMixFormat(&device_format);

        if (FAILED(hr)) {
            audio_client->Release();
            device->Release();
            enumerator->Release();

            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - STT Framework: Failed to get mix format");
            });

            return;
        }
        
        REFERENCE_TIME buffer_duration = 200000;
        hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, buffer_duration, 0, device_format, nullptr);

        if (FAILED(hr)) {
            CoTaskMemFree(device_format);
            audio_client->Release();
            device->Release();
            enumerator->Release();

            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - STT Framework: Failed to initialize audio client");
            });

            return;
        }
        
        hr = audio_client->GetService(__uuidof(IAudioCaptureClient), (void**)&capture_client);

        if (FAILED(hr)) {
            CoTaskMemFree(device_format);
            audio_client->Release();
            device->Release();
            enumerator->Release();

            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - STT Framework: Failed to get capture client");
            });

            return;
        }
        
        uint32_t device_sample_rate = device_format->nSamplesPerSec;
        uint16_t device_channels = device_format->nChannels;
        uint16_t device_bits = device_format->wBitsPerSample;
        bool is_float = (device_format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);

        if (device_format->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            WAVEFORMATEXTENSIBLE* ext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(device_format);
            // Check SubFormat - IEEE float GUID is {00000003-0000-0010-8000-00AA00389B71}
            is_float = (ext->SubFormat.Data1 == 3 && ext->SubFormat.Data2 == 0 && ext->SubFormat.Data3 == 0x10);
        }
        
        CoTaskMemFree(device_format);
        hr = audio_client->Start();

        if (FAILED(hr)) {
            capture_client->Release();
            audio_client->Release();
            device->Release();
            enumerator->Release();

            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - STT Framework: Failed to start capture");
            });

            return;
        }
        
        std::vector<int16_t> resample_buffer;
        
        while (capture_thread_running) {
            UINT32 packet_length = 0;
            hr = capture_client->GetNextPacketSize(&packet_length);
            
            while (SUCCEEDED(hr) && packet_length > 0) {
                BYTE* data;
                UINT32 frames_available;
                DWORD flags;
                hr = capture_client->GetBuffer(&data, &frames_available, &flags, nullptr, nullptr);
                if (FAILED(hr)) break;
                
                if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && frames_available > 0) {
                    std::vector<int16_t> mono_samples(frames_available);
                    
                    for (UINT32 i = 0; i < frames_available; i++) {
                        float sample_sum = 0.0f;
                        
                        for (uint16_t ch = 0; ch < device_channels; ch++) {
                            float sample;

                            if (is_float) {
                                sample = reinterpret_cast<float*>(data)[i * device_channels + ch];
                            } else if (device_bits == 16) {
                                sample = reinterpret_cast<int16_t*>(data)[i * device_channels + ch] / 32768.0f;
                            } else if (device_bits == 32) {
                                sample = reinterpret_cast<int32_t*>(data)[i * device_channels + ch] / 2147483648.0f;
                            } else {
                                sample = 0.0f;
                            }

                            sample_sum += sample;
                        }
                        
                        float mono = sample_sum / device_channels;
                        mono_samples[i] = static_cast<int16_t>(std::clamp(mono, -1.0f, 1.0f) * 32767.0f);
                    }
                    
                    if (device_sample_rate != STT_SAMPLE_RATE) {
                        resample_audio(mono_samples, device_sample_rate, resample_buffer, STT_SAMPLE_RATE);
                        capture_buffer.append_samples(resample_buffer.data(), resample_buffer.size());
                    } else {
                        capture_buffer.append_samples(mono_samples.data(), mono_samples.size());
                    }
                    
                    if (is_capturing) {
                        bool timed_out = false;
                        
                        {
                            std::lock_guard<std::mutex> lock(capture_time_mutex);
                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - capture_start_time).count();
                            timed_out = (elapsed >= MAX_CAPTURE_DURATION_MS);
                        }
                        
                        if (timed_out) {
                            std::lock_guard<std::mutex> state_lock(capture_state_mutex);

                            if (is_capturing) {
                                is_capturing = false;
                                capture_buffer.stop_recording();
                                std::vector<int16_t> audio_data = capture_buffer.get_buffer_copy();
                                
                                if (!audio_data.empty()) {
                                    {
                                        std::lock_guard<std::mutex> proc_lock(processing_mutex);
                                        processing_queue.clear();
                                        processing_queue.push_back(std::move(audio_data));
                                    }

                                    processing_cv.notify_one();
                                }
                                
                                capture_buffer.clear();
                            }
                        }
                    }
                }
                
                capture_client->ReleaseBuffer(frames_available);
                hr = capture_client->GetNextPacketSize(&packet_length);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        audio_client->Stop();
        capture_client->Release();
        audio_client->Release();
        device->Release();
        enumerator->Release();
    }

    void processing_worker() {
        CoInitializeEx(NULL, COINIT_MULTITHREADED);
        
        struct CoInitGuard {
            ~CoInitGuard() { CoUninitialize(); }
        } co_guard;

        while (processing_thread_running) {
            std::vector<int16_t> audio_data;
            bool has_data = false;
            
            {
                std::unique_lock<std::mutex> lock(processing_mutex);
                
                processing_cv.wait_for(lock, std::chrono::milliseconds(50), [this] {
                    return !processing_queue.empty() || !processing_thread_running;
                });
                
                if (!processing_queue.empty()) {
                    audio_data = std::move(processing_queue.back());
                    processing_queue.clear();
                    has_data = true;
                }
            }
            
            if (has_data && !audio_data.empty()) {
                try {
                    std::string transcription = transcribe_audio(audio_data);
                    
                    if (!transcription.empty()) {
                        MainThreadScheduler::instance().schedule([transcription]() {                            
                            auto_array<game_value> stt_data;
                            stt_data.push_back(game_value(transcription));
                            raw_call_sqf_args_native_no_return(g_compiled_stt_transcription_event, game_value(std::move(stt_data)));
                        });
                    }
                } catch (const std::exception& e) {
                    std::string error_msg = e.what();

                    MainThreadScheduler::instance().schedule([error_msg]() {
                        report_error("KH - STT Framework: Transcription failed: " + error_msg);
                    });
                }
            }
        }
    }

    std::string transcribe_audio(const std::vector<int16_t>& audio_data) {
        // Get handle under lock, release before actual transcription
        std::shared_ptr<const SherpaOnnxOfflineRecognizer> handle_copy;
        int current_sample_rate = 0;
        
        {
            std::lock_guard<std::mutex> lock(stt_mutex);

            if (!recognizer_handle) {
                throw std::runtime_error("STT not initialized");
            }

            handle_copy = recognizer_handle;
            current_sample_rate = sample_rate;
        }

        std::vector<float> float_samples(audio_data.size());

        for (size_t i = 0; i < audio_data.size(); i++) {
            float_samples[i] = static_cast<float>(audio_data[i]) / 32768.0f;
        }

        struct StreamDeleter {
            void operator()(const SherpaOnnxOfflineStream* s) const {
                if (s) SherpaOnnxDestroyOfflineStream(s);
            }
        };

        struct ResultDeleter {
            void operator()(const SherpaOnnxOfflineRecognizerResult* r) const {
                if (r) SherpaOnnxDestroyOfflineRecognizerResult(r);
            }
        };

        std::unique_ptr<const SherpaOnnxOfflineStream, StreamDeleter> stream(
            SherpaOnnxCreateOfflineStream(handle_copy.get())
        );
        
        if (!stream) {
            throw std::runtime_error("Failed to create recognition stream");
        }

        SherpaOnnxAcceptWaveformOffline(stream.get(), current_sample_rate, 
                                       float_samples.data(), 
                                       static_cast<int32_t>(float_samples.size()));

        SherpaOnnxDecodeOfflineStream(handle_copy.get(), stream.get());

        std::unique_ptr<const SherpaOnnxOfflineRecognizerResult, ResultDeleter> result(
            SherpaOnnxGetOfflineStreamResult(stream.get())
        );

        std::string transcription;
        
        if (result && result->text) {
            transcription = result->text;
        }

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

    bool load_model(const std::string& model_name, int num_threads = 4) {
        std::string log_message;
        bool success = false;
        std::string resolved_model_name;
        int loaded_sample_rate = 0;
        
        {
            std::lock_guard<std::mutex> lock(stt_mutex);
            recognizer_handle.reset();

            try {
                std::filesystem::path model_path;
                resolved_model_name = model_name;
                
                if (model_name.empty()) {
                    model_path = STTModelDiscovery::find_any_model();
                    
                    if (model_path.empty()) {
                        log_message = "KH - STT Framework: No STT models found in any search location";
                    } else {
                        resolved_model_name = model_path.filename().string();
                    }
                } else {
                    model_path = STTModelDiscovery::find_model(model_name);
                    
                    if (model_path.empty()) {
                        log_message = "KH - STT Framework: Model not found: " + model_name;
                    }
                }

                if (!model_path.empty()) {
                    // Detect model type
                    STTModelDiscovery::STTModelType model_type = STTModelDiscovery::detect_stt_model_type(model_path);
                    
                    if (model_type == STTModelDiscovery::STTModelType::UNKNOWN) {
                        log_message = "KH - STT Framework: Unable to determine model type for: " + model_path.string();
                    }
                    else {
                        std::string encoder_path;
                        std::string decoder_path;
                        std::string joiner_path;
                        std::string tokens_path;
                        std::string generic_model_path;
                        
                        // Moonshine-specific
                        std::string preprocess_path;
                        std::string encode_path;
                        std::string uncached_decode_path;
                        std::string cached_decode_path;
                        
                        for (const auto& entry : std::filesystem::directory_iterator(model_path)) {
                            if (!entry.is_regular_file()) continue;
                            std::string filename = entry.path().filename().string();
                            std::string lower_filename = filename;
                            std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
                            
                            if (lower_filename == "tokens.txt") {
                                tokens_path = entry.path().string();
                            }
                            else if (lower_filename.ends_with(".onnx")) {
                                // Moonshine files
                                if (lower_filename.find("preprocess") != std::string::npos) {
                                    preprocess_path = entry.path().string();
                                }
                                else if (lower_filename.find("uncached_decode") != std::string::npos ||
                                        lower_filename.find("uncached-decode") != std::string::npos) {
                                    uncached_decode_path = entry.path().string();
                                }
                                else if (lower_filename.find("cached_decode") != std::string::npos ||
                                        lower_filename.find("cached-decode") != std::string::npos) {
                                    cached_decode_path = entry.path().string();
                                }
                                else if ((lower_filename.find("encode") != std::string::npos && 
                                        lower_filename.find("encoder") == std::string::npos) ||
                                        lower_filename == "encode.onnx" ||
                                        lower_filename == "encode.int8.onnx") {
                                    encode_path = entry.path().string();
                                }
                                // Transducer/Whisper
                                else if (lower_filename.find("encoder") != std::string::npos) {
                                    encoder_path = entry.path().string();
                                }
                                else if (lower_filename.find("decoder") != std::string::npos) {
                                    decoder_path = entry.path().string();
                                }
                                else if (lower_filename.find("joiner") != std::string::npos) {
                                    joiner_path = entry.path().string();
                                }
                                // Generic model
                                else if (lower_filename == "model.onnx" || 
                                        lower_filename == "model.int8.onnx" ||
                                        lower_filename.starts_with("model.")) {
                                    generic_model_path = entry.path().string();
                                }
                            }
                        }
                        
                        // Configure recognizer based on model type
                        SherpaOnnxOfflineRecognizerConfig config;
                        memset(&config, 0, sizeof(config));
                        bool config_valid = false;
                        
                        switch (model_type) {
                            case STTModelDiscovery::STTModelType::TRANSDUCER:
                                if (!encoder_path.empty() && !decoder_path.empty() && 
                                    !joiner_path.empty() && !tokens_path.empty()) {
                                    config.model_config.transducer.encoder = encoder_path.c_str();
                                    config.model_config.transducer.decoder = decoder_path.c_str();
                                    config.model_config.transducer.joiner = joiner_path.c_str();
                                    config_valid = true;
                                }

                                break;
                                
                            case STTModelDiscovery::STTModelType::WHISPER:
                                if (!encoder_path.empty() && !decoder_path.empty() && !tokens_path.empty()) {
                                    config.model_config.whisper.encoder = encoder_path.c_str();
                                    config.model_config.whisper.decoder = decoder_path.c_str();
                                    config.model_config.whisper.language = "en";
                                    config.model_config.whisper.task = "transcribe";
                                    config_valid = true;
                                }

                                break;
                                
                            case STTModelDiscovery::STTModelType::PARAFORMER:
                                if (!generic_model_path.empty() && !tokens_path.empty()) {
                                    config.model_config.paraformer.model = generic_model_path.c_str();
                                    config_valid = true;
                                }

                                break;
                                
                            case STTModelDiscovery::STTModelType::SENSEVOICE:
                                if (!generic_model_path.empty() && !tokens_path.empty()) {
                                    config.model_config.sense_voice.model = generic_model_path.c_str();
                                    config.model_config.sense_voice.language = "";
                                    config.model_config.sense_voice.use_itn = 0;
                                    config_valid = true;
                                }

                                break;
                                
                            case STTModelDiscovery::STTModelType::MOONSHINE:
                                if (!preprocess_path.empty() && !encode_path.empty() && 
                                    !uncached_decode_path.empty() && !cached_decode_path.empty() && 
                                    !tokens_path.empty()) {
                                    config.model_config.moonshine.preprocessor = preprocess_path.c_str();
                                    config.model_config.moonshine.encoder = encode_path.c_str();
                                    config.model_config.moonshine.uncached_decoder = uncached_decode_path.c_str();
                                    config.model_config.moonshine.cached_decoder = cached_decode_path.c_str();
                                    config_valid = true;
                                }
                                
                                break;
                                
                            case STTModelDiscovery::STTModelType::NEMO_CTC:
                                if (!generic_model_path.empty() && !tokens_path.empty()) {
                                    config.model_config.nemo_ctc.model = generic_model_path.c_str();
                                    config_valid = true;
                                }
                                
                                break;
                                
                            case STTModelDiscovery::STTModelType::ZIPFORMER_CTC:
                                if (!generic_model_path.empty() && !tokens_path.empty()) {
                                    config.model_config.zipformer_ctc.model = generic_model_path.c_str();
                                    config_valid = true;
                                }

                                break;
                                
                            case STTModelDiscovery::STTModelType::TDNN:
                                if (!generic_model_path.empty() && !tokens_path.empty()) {
                                    config.model_config.tdnn.model = generic_model_path.c_str();
                                    config_valid = true;
                                }

                                break;
                                
                            case STTModelDiscovery::STTModelType::WENET_CTC:
                                if (!generic_model_path.empty() && !tokens_path.empty()) {
                                    config.model_config.wenet_ctc.model = generic_model_path.c_str();
                                    config_valid = true;
                                }
                                
                                break;
                                
                            default:
                                log_message = "KH - STT Framework: Unsupported model type";
                                break;
                        }
                        
                        if (config_valid) {
                            config.model_config.tokens = tokens_path.c_str();
                            config.model_config.num_threads = num_threads;
                            config.model_config.provider = "directml";
                            config.model_config.debug = 0;
                            config.decoding_method = "greedy_search";
                            config.max_active_paths = 4;
                            
                            recognizer_handle = std::shared_ptr<const SherpaOnnxOfflineRecognizer>(
                                SherpaOnnxCreateOfflineRecognizer(&config), 
                                RecognizerDeleter{}
                            );
                            
                            if (!recognizer_handle) {
                                log_message = "KH - STT Framework: Failed to create recognizer for " + 
                                            STTModelDiscovery::stt_model_type_to_string(model_type) + " model";
                            }
                            else {
                                sample_rate = 16000;
                                loaded_sample_rate = sample_rate;
                                is_initialized_flag.store(true, std::memory_order_release);
                                success = true;
                                
                                log_message = "KH - STT Framework: " + STTModelDiscovery::stt_model_type_to_string(model_type) + 
                                    " model loaded successfully - " + model_path.string() + 
                                    " | Sample Rate: " + std::to_string(loaded_sample_rate) + " Hz";
                            }
                        }
                        else if (log_message.empty()) {
                            log_message = "KH - STT Framework: Missing required files for " + 
                                        STTModelDiscovery::stt_model_type_to_string(model_type) + " model";
                        }
                    }
                }
            } catch (const std::exception& e) {
                log_message = "KH - STT Framework: Model loading exception: " + std::string(e.what());
            }
        }

        if (!log_message.empty()) {
            std::string msg = log_message;

            MainThreadScheduler::instance().schedule([msg, success]() {
                if (success) {
                    sqf::diag_log(msg);
                } else {
                    report_error("KH - STT Framework: " + msg);
                }
            });
        }

        if (success) {
            if (!capture_thread_running) {
                capture_thread_running.store(true, std::memory_order_release);
                capture_thread = std::thread(&STTFramework::audio_capture_worker, this);
            }

            if (!processing_thread_running) {
                processing_thread_running.store(true, std::memory_order_release);
                processing_thread = std::thread(&STTFramework::processing_worker, this);
            }
        }

        return success;
    }

    bool is_initialized() const {
        return is_initialized_flag;
    }

    bool is_capturing_audio() const {
        return is_capturing;
    }

    void cleanup() {
        {
            std::lock_guard<std::mutex> state_lock(capture_state_mutex);
            is_capturing.store(false, std::memory_order_release);
            capture_buffer.stop_recording();
        }

        capture_thread_running.store(false, std::memory_order_release);
        processing_thread_running.store(false, std::memory_order_release);
        processing_cv.notify_all();

        if (capture_thread.joinable()) {
            try {
                capture_thread.join();
            } catch (...) {
                // Thread join failed, continue cleanup
            }
        }

        if (processing_thread.joinable()) {
            try {
                processing_thread.join();
            } catch (...) {
                // Thread join failed, continue cleanup
            }
        }

        {
            std::lock_guard<std::mutex> lock(stt_mutex);
            is_initialized_flag.store(false, std::memory_order_release);
            recognizer_handle.reset();
            sample_rate = STT_SAMPLE_RATE;
        }

        capture_buffer.clear();

        {
            std::lock_guard<std::mutex> proc_lock(processing_mutex);
            processing_queue.clear();
        }

        capture_thread_alive.store(false, std::memory_order_release);
    }

    bool start_capture_manual() {
        if (!is_initialized_flag) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - STT Framework: Cannot start capture - no model loaded");
            });
            
            return false;
        }

        if (!capture_thread_alive) {
            if (capture_thread.joinable()) {
                capture_thread.join();
            }

            if (is_initialized_flag) {
                capture_thread_running.store(true, std::memory_order_release);
                capture_thread = std::thread(&STTFramework::audio_capture_worker, this);

                for (int i = 0; i < 50 && !capture_thread_alive && capture_thread_running; i++) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                
                if (!capture_thread_alive) {
                    MainThreadScheduler::instance().schedule([]() {
                        report_error("KH - STT Framework: Failed to restart capture thread");
                    });

                    return false;
                }
            } else {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - STT Framework: Capture thread not running and no model loaded");
                });

                return false;
            }
        }

        // Clear pending transcriptions when starting new capture
        // This prevents old recordings from being processed after new ones
        {
            std::lock_guard<std::mutex> proc_lock(processing_mutex);

            if (!processing_queue.empty()) {
                processing_queue.clear();
            }
        }

        std::lock_guard<std::mutex> state_lock(capture_state_mutex);

        // If already capturing, RESTART instead of failing
        if (is_capturing) {
            // Stop current capture without processing
            capture_buffer.stop_recording();
            capture_buffer.clear();
            // Don't return false - continue to start new capture
        }

        // Start fresh recording
        capture_buffer.clear();
        capture_buffer.start_recording();
        is_capturing.store(true, std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock(capture_time_mutex);
            capture_start_time = std::chrono::steady_clock::now();
        }

        return true;
    }

    bool stop_capture_manual() {
        std::unique_lock<std::mutex> state_lock(capture_state_mutex);
        bool was_capturing = is_capturing.exchange(false, std::memory_order_acq_rel);
        
        if (!was_capturing) {
            return false;
        }

        capture_buffer.stop_recording();
        std::vector<int16_t> audio_data = capture_buffer.get_buffer_copy();
        capture_buffer.clear();
        state_lock.unlock();

        if (!audio_data.empty()) {
            // Clear any old pending items and add new one
            {
                std::lock_guard<std::mutex> proc_lock(processing_mutex);
                processing_queue.clear();
                processing_queue.push_back(std::move(audio_data));
            }

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

    bool load_model_public(const std::string& model_name, int num_threads = 4) {
        return load_model(model_name, num_threads);
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