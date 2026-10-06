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

    // The files of a model folder, each by its role ("" when absent): tokens.txt and the .onnx files by name -
    // Moonshine's preprocess / encode / uncached_decode / cached_decode, a transducer's or Whisper's encoder /
    // decoder / joiner, else the generic model.* file.
    struct ModelFiles {
        std::string tokens;
        std::string encoder;
        std::string decoder;
        std::string joiner;
        std::string model;
        std::string preprocess;
        std::string encode;
        std::string uncached_decode;
        std::string cached_decode;
    };

    // False when the folder cannot be read.
    static bool scan_model_files(const std::filesystem::path& model_path, ModelFiles& files) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(model_path)) {
                if (!entry.is_regular_file()) continue;
                std::string lower_filename = kh_lower_copy(entry.path().filename().string());
                const std::string path = entry.path().string();

                if (lower_filename == "tokens.txt") {
                    files.tokens = path;
                } else if (lower_filename.ends_with(".onnx")) {
                    // Moonshine's names first (its "encode" is not "encoder")
                    if (lower_filename.find("preprocess") != std::string::npos) {
                        files.preprocess = path;
                    } else if (lower_filename.find("uncached_decode") != std::string::npos ||
                               lower_filename.find("uncached-decode") != std::string::npos) {
                        files.uncached_decode = path;
                    } else if (lower_filename.find("cached_decode") != std::string::npos ||
                               lower_filename.find("cached-decode") != std::string::npos) {
                        files.cached_decode = path;
                    } else if (lower_filename.find("encode") != std::string::npos &&
                               lower_filename.find("encoder") == std::string::npos) {
                        files.encode = path;
                    } else if (lower_filename.find("encoder") != std::string::npos) {
                        files.encoder = path;
                    } else if (lower_filename.find("decoder") != std::string::npos) {
                        files.decoder = path;
                    } else if (lower_filename.find("joiner") != std::string::npos) {
                        files.joiner = path;
                    } else if (lower_filename.starts_with("model.")) {   // model.onnx, model.int8.onnx, ...
                        files.model = path;
                    }
                }
            }
        } catch (...) {
            return false;
        }

        return true;
    }

    // The model type its files say, most specific first: Moonshine (its four files), a transducer (encoder +
    // decoder + joiner), Whisper (encoder + decoder), else a single model.* file whose folder name tells the type
    // (SenseVoice, Paraformer, NeMo CTC, Zipformer CTC, Wenet CTC, TDNN; Paraformer when nothing does).
    static STTModelType detect_stt_model_type(const std::filesystem::path& model_path, const ModelFiles& files) {
        std::string lower_dir_name = kh_lower_copy(model_path.filename().string());
        const bool has_tokens = !files.tokens.empty();

        if (!files.preprocess.empty() && !files.encode.empty() && !files.uncached_decode.empty() &&
            !files.cached_decode.empty() && has_tokens) {
            return STTModelType::MOONSHINE;
        }

        if (!files.encoder.empty() && !files.decoder.empty() && has_tokens) {
            return files.joiner.empty() ? STTModelType::WHISPER : STTModelType::TRANSDUCER;
        }

        if (!files.model.empty() && has_tokens) {
            if (lower_dir_name.find("sense-voice") != std::string::npos ||
                lower_dir_name.find("sensevoice") != std::string::npos ||
                lower_dir_name.find("sense_voice") != std::string::npos) {
                return STTModelType::SENSEVOICE;
            }

            if (lower_dir_name.find("paraformer") != std::string::npos) return STTModelType::PARAFORMER;

            if (lower_dir_name.find("nemo") != std::string::npos && lower_dir_name.find("ctc") != std::string::npos) {
                return STTModelType::NEMO_CTC;
            }

            if (lower_dir_name.find("zipformer") != std::string::npos &&
                lower_dir_name.find("ctc") != std::string::npos) {
                return STTModelType::ZIPFORMER_CTC;
            }

            if (lower_dir_name.find("wenet") != std::string::npos) return STTModelType::WENET_CTC;
            if (lower_dir_name.find("tdnn") != std::string::npos) return STTModelType::TDNN;
            if (lower_dir_name.find("nemo") != std::string::npos) return STTModelType::NEMO_CTC;   // NeMo is CTC
            return STTModelType::PARAFORMER;   // The most common single-model type
        }

        return STTModelType::UNKNOWN;
    }

    static STTModelType detect_stt_model_type(const std::filesystem::path& model_path) {
        if (model_path.empty() || !std::filesystem::exists(model_path) || !std::filesystem::is_directory(model_path)) {
            return STTModelType::UNKNOWN;
        }

        ModelFiles files;
        if (!scan_model_files(model_path, files)) return STTModelType::UNKNOWN;
        return detect_stt_model_type(model_path, files);
    }

    static std::vector<std::filesystem::path> find_all_stt_model_directories() {
        return ModFolderSearcher::kh_framework_search_paths("stt_models");
    }

    static std::filesystem::path find_model(const std::string& model_name) {
        // PBO_PATH (search_mod_folders.hpp's note): one leading slash names the model FOLDER inside the loaded PBOs
        // by its engine path, no stt_models folder. sherpa-onnx opens files on disk, so the model is the folder's
        // extracted copy (PBO_EXTRACT: every file under it, written once, reused while its PBO is unchanged).
        if (ModFolderSearcher::is_pbo_path(model_name)) {
            std::filesystem::path out;
            std::string err;
            if (ModFolderSearcher::extract_pbo_directory(model_name, out, &err)) return out;
            MainThreadScheduler::instance().schedule([err]() { sqf::diag_log("KH STT: " + err); });
            return std::filesystem::path();
        }

        auto search_paths = find_all_stt_model_directories();
        
        for (const auto& base_path : search_paths) {
            // PATH_CONFINE (search_mod_folders.hpp's note): a folder below the models folder only - a drive,
            // rooted or network path, or one leaving the folder through '..', is not found (and touches nothing).
            std::filesystem::path model_path = ModFolderSearcher::confined_join(base_path, model_name);
            if (model_path.empty()) break;   // The name's verdict, the same under every folder.

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
    ~STTFramework() = default;
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

    // capture_thread_alive is set by the spawning code before the thread exists (a thread that has not run yet is
    // alive, so start_capture never joins one that is about to enter the capture loop) and cleared here on exit.
    void audio_capture_worker() {
        struct ThreadAliveGuard {
            std::atomic<bool>& flag;
            ThreadAliveGuard(std::atomic<bool>& f) : flag(f) {}
            ~ThreadAliveGuard() { flag = false; }
        } alive_guard(capture_thread_alive);
        
        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        struct CoInitGuard {
            ~CoInitGuard() { CoUninitialize(); }
        } co_guard;
        
        // The interfaces release themselves (ComPtr) on every path out of here; a failure reports what failed.
        auto fail = [](const char* what) {
            std::string message = std::string("KH STT: ") + what;
            MainThreadScheduler::instance().schedule([message]() { report_error(message); });
        };

        Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
        Microsoft::WRL::ComPtr<IMMDevice> device;
        Microsoft::WRL::ComPtr<IAudioClient> audio_client;
        Microsoft::WRL::ComPtr<IAudioCaptureClient> capture_client;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                      IID_PPV_ARGS(enumerator.GetAddressOf()));
        if (FAILED(hr)) return fail("failed to create the device enumerator");
        hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, device.GetAddressOf());
        if (FAILED(hr)) return fail("failed to get the default capture device");
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(audio_client.GetAddressOf()));
        if (FAILED(hr)) return fail("failed to activate the audio client");

        uint32_t device_sample_rate;
        uint16_t device_channels;
        uint16_t device_bits;
        bool is_float;

        {
            WAVEFORMATEX* device_format = nullptr;
            hr = audio_client->GetMixFormat(&device_format);
            if (FAILED(hr)) return fail("failed to get the mix format");
            struct FormatFree { WAVEFORMATEX* p; ~FormatFree() { CoTaskMemFree(p); } } format_free{ device_format };
            REFERENCE_TIME buffer_duration = 200000;
            hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, buffer_duration, 0, device_format, nullptr);
            if (FAILED(hr)) return fail("failed to initialize the audio client");
            hr = audio_client->GetService(IID_PPV_ARGS(capture_client.GetAddressOf()));
            if (FAILED(hr)) return fail("failed to get the capture client");
            device_sample_rate = device_format->nSamplesPerSec;
            device_channels = device_format->nChannels;
            device_bits = device_format->wBitsPerSample;
            is_float = (device_format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);

            if (device_format->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                WAVEFORMATEXTENSIBLE* ext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(device_format);
                // Check SubFormat - IEEE float GUID is {00000003-0000-0010-8000-00AA00389B71}
                is_float = (ext->SubFormat.Data1 == 3 && ext->SubFormat.Data2 == 0 && ext->SubFormat.Data3 == 0x10);
            }

            // The loop below reads 32-bit float, 16-bit and 32-bit integer frames; anything else would be silence.
            const bool supported = device_channels > 0 && (is_float ? device_bits == 32
                                                                     : device_bits == 16 || device_bits == 32);
            if (!supported) return fail("unsupported capture format");
        }

        hr = audio_client->Start();
        if (FAILED(hr)) return fail("failed to start the capture");
        
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
                        report_error("KH STT: transcription failed: " + error_msg);
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
                throw std::runtime_error("no model loaded");
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
            throw std::runtime_error("failed to create the recognition stream");
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

public:
    // Never destroyed: at process exit the worker threads are already gone when static destructors would run
    // (a mutex one of them held would hang the exit); DllMain's unload path stops the framework instead.
    static STTFramework& instance() {
        static STTFramework* inst = new STTFramework();
        return *inst;
    }

    bool load_model(const std::string& model_name, int num_threads = 4) {
        std::string log_message;
        bool success = false;
        std::string resolved_model_name;
        int loaded_sample_rate = 0;
        
        {   // A capture in progress is ended and its audio dropped: after a failed load there is nothing to transcribe
            // it with, and SQF guarded by sttIsInitialized could not stop it.
            std::lock_guard<std::mutex> state_lock(capture_state_mutex);
            is_capturing.store(false, std::memory_order_release);
            capture_buffer.stop_recording();
            capture_buffer.clear();
        }

        {
            std::lock_guard<std::mutex> lock(stt_mutex);
            recognizer_handle.reset();
            is_initialized_flag.store(false, std::memory_order_release);   // Until the new model is in.

            try {
                std::filesystem::path model_path;
                resolved_model_name = model_name;
                
                if (model_name.empty()) {
                    model_path = STTModelDiscovery::find_any_model();
                    
                    if (model_path.empty()) {
                        log_message = "sttLoadModel: no STT model found in any search location";
                    } else {
                        resolved_model_name = model_path.filename().string();
                    }
                } else {
                    model_path = STTModelDiscovery::find_model(model_name);
                    
                    if (model_path.empty()) {
                        log_message = "sttLoadModel: model not found: " + model_name;
                    }
                }

                if (!model_path.empty()) {
                    STTModelDiscovery::ModelFiles files;
                    STTModelDiscovery::STTModelType model_type = STTModelDiscovery::STTModelType::UNKNOWN;

                    if (STTModelDiscovery::scan_model_files(model_path, files)) {
                        model_type = STTModelDiscovery::detect_stt_model_type(model_path, files);
                    }
                    
                    if (model_type == STTModelDiscovery::STTModelType::UNKNOWN) {
                        log_message = "sttLoadModel: could not determine the model type of " + model_path.string();
                    }
                    else {
                        const std::string& encoder_path = files.encoder;
                        const std::string& decoder_path = files.decoder;
                        const std::string& joiner_path = files.joiner;
                        const std::string& tokens_path = files.tokens;
                        const std::string& generic_model_path = files.model;
                        const std::string& preprocess_path = files.preprocess;
                        const std::string& encode_path = files.encode;
                        const std::string& uncached_decode_path = files.uncached_decode;
                        const std::string& cached_decode_path = files.cached_decode;
                        
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
                                log_message = "sttLoadModel: unsupported model type";
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
                                log_message = "sttLoadModel: failed to create the recognizer for " +
                                            STTModelDiscovery::stt_model_type_to_string(model_type) + " model";
                            }
                            else {
                                sample_rate = 16000;
                                loaded_sample_rate = sample_rate;
                                is_initialized_flag.store(true, std::memory_order_release);
                                success = true;
                                
                                log_message = "KH STT: " + STTModelDiscovery::stt_model_type_to_string(model_type) +
                                    " model loaded - " + model_path.string() +
                                    " | Sample Rate: " + std::to_string(loaded_sample_rate) + " Hz";
                            }
                        }
                        else if (log_message.empty()) {
                            log_message = "sttLoadModel: missing required files for " +
                                        STTModelDiscovery::stt_model_type_to_string(model_type) + " model";
                        }
                    }
                }
            } catch (const std::exception& e) {
                log_message = "sttLoadModel: " + std::string(e.what());
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
            if (!capture_thread_running) {
                capture_thread_running.store(true, std::memory_order_release);
                capture_thread_alive.store(true, std::memory_order_release);
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

    bool start_capture() {
        if (!is_initialized_flag) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("sttStartCapture: no model loaded");
            });
            
            return false;
        }

        // A capture thread that exited (its device failed; it reported why) is joined and started again.
        if (!capture_thread_alive) {
            if (capture_thread.joinable()) {
                capture_thread.join();
            }

            capture_thread_running.store(true, std::memory_order_release);
            capture_thread_alive.store(true, std::memory_order_release);
            capture_thread = std::thread(&STTFramework::audio_capture_worker, this);
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

    bool stop_capture() {
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
};