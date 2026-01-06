#pragma once

using Microsoft::WRL::ComPtr;

class DirectWriteFontLoader : public ultralight::FontLoader {
public:
    DirectWriteFontLoader() : font_cache_(128) {
        HRESULT hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&dwrite_factory_)
        );
        
        if (FAILED(hr)) {
            dwrite_factory_ = nullptr;
        }
    }

    ~DirectWriteFontLoader() {
        font_cache_.clear();
        
        if (dwrite_factory_) {
            dwrite_factory_->Release();
            dwrite_factory_ = nullptr;
        }
    }
    
    DirectWriteFontLoader(const DirectWriteFontLoader&) = delete;
    DirectWriteFontLoader& operator=(const DirectWriteFontLoader&) = delete;

    ultralight::String fallback_font() const override {
        return "Segoe UI";
    }

    ultralight::String fallback_font_for_characters(
        const ultralight::String& characters,
        int weight,
        bool italic) const override {
        return "Segoe UI";
    }

    ultralight::RefPtr<ultralight::FontFile> Load(
        const ultralight::String& family,
        int weight,
        bool italic) override {
        if (!dwrite_factory_) return nullptr;
        std::string family_str(family.utf8().data());
        uint64_t key = hash_font_key(family_str, weight, italic);
        auto cached = font_cache_.get(key);

        if (cached.has_value()) {
            return cached.value();
        }
        
        // Cache miss - load the font
        auto font_file = LoadFontInternal(family_str, weight, italic);

        if (font_file) {
            font_cache_.put(key, font_file);
        }
                
        return font_file;
    }

private:
    IDWriteFactory* dwrite_factory_ = nullptr;
    mutable LRUCache<uint64_t, ultralight::RefPtr<ultralight::FontFile>> font_cache_;
    
    static uint64_t hash_font_key(const std::string& family, int weight, bool italic) {
        uint64_t h = 14695981039346656037ULL;
        
        for (char c : family) {
            h ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
            h *= 1099511628211ULL;
        }

        h ^= static_cast<uint64_t>(static_cast<uint32_t>(weight));
        h *= 1099511628211ULL;
        h ^= static_cast<uint64_t>(italic ? 1 : 0);
        h *= 1099511628211ULL;
        return h;
    }
    
    ultralight::RefPtr<ultralight::FontFile> LoadFontInternal(
        const std::string& family_str,
        int weight,
        bool italic) {
        int wide_len = MultiByteToWideChar(CP_UTF8, 0, family_str.c_str(), -1, nullptr, 0);
        if (wide_len <= 0) return nullptr;
        std::wstring family_wide(wide_len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, family_str.c_str(), -1, &family_wide[0], wide_len);
        family_wide.resize(wide_len - 1);
        ComPtr<IDWriteFontCollection> font_collection;

        if (FAILED(dwrite_factory_->GetSystemFontCollection(&font_collection, FALSE))) {
            return nullptr;
        }

        UINT32 family_index = 0;
        BOOL exists = FALSE;
        font_collection->FindFamilyName(family_wide.c_str(), &family_index, &exists);

        if (!exists) {
            font_collection->FindFamilyName(L"Segoe UI", &family_index, &exists);
            if (!exists) return nullptr;
        }

        ComPtr<IDWriteFontFamily> font_family;

        if (FAILED(font_collection->GetFontFamily(family_index, &font_family))) {
            return nullptr;
        }

        DWRITE_FONT_WEIGHT dw_weight = static_cast<DWRITE_FONT_WEIGHT>(weight);
        DWRITE_FONT_STYLE dw_style = italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
        ComPtr<IDWriteFont> font;

        if (FAILED(font_family->GetFirstMatchingFont(dw_weight, DWRITE_FONT_STRETCH_NORMAL, dw_style, &font))) {
            return nullptr;
        }

        ComPtr<IDWriteFontFace> font_face;

        if (FAILED(font->CreateFontFace(&font_face))) {
            return nullptr;
        }

        UINT32 file_count = 0;
        font_face->GetFiles(&file_count, nullptr);

        if (file_count == 0) {
            return nullptr;
        }

        std::vector<IDWriteFontFile*> font_files(file_count);

        if (FAILED(font_face->GetFiles(&file_count, font_files.data()))) {
            return nullptr;
        }

        struct FontFileCleanup {
            std::vector<IDWriteFontFile*>& files;
            
            ~FontFileCleanup() {
                for (auto f : files) {
                    if (f) f->Release();
                }
            }
        } cleanup{font_files};

        ComPtr<IDWriteFontFileLoader> loader;

        if (FAILED(font_files[0]->GetLoader(&loader)) || !loader) {
            return nullptr;
        }

        const void* ref_key = nullptr;
        UINT32 ref_key_size = 0;

        if (FAILED(font_files[0]->GetReferenceKey(&ref_key, &ref_key_size))) {
            return nullptr;
        }

        ComPtr<IDWriteFontFileStream> stream;

        if (FAILED(loader->CreateStreamFromKey(ref_key, ref_key_size, &stream))) {
            return nullptr;
        }

        UINT64 file_size = 0;

        if (FAILED(stream->GetFileSize(&file_size))) {
            return nullptr;
        }

        const void* font_data = nullptr;
        void* context = nullptr;

        if (FAILED(stream->ReadFileFragment(&font_data, 0, file_size, &context))) {
            return nullptr;
        }
        
        // Create Ultralight buffer and font file
        auto buffer = ultralight::Buffer::CreateFromCopy(font_data, static_cast<size_t>(file_size));
        stream->ReleaseFileFragment(context);
        return ultralight::FontFile::Create(buffer);
    }
};

class UIFileSystem : public ultralight::FileSystem {
public:
    UIFileSystem(
        const std::vector<std::filesystem::path>& search_paths,
        const std::filesystem::path& resources_path)
        : search_paths_(search_paths)
        , resources_path_(resources_path)
        , exists_cache_(1024) {}
    
    UIFileSystem(const UIFileSystem&) = delete;
    UIFileSystem& operator=(const UIFileSystem&) = delete;

    bool FileExists(const ultralight::String& path) override {
        std::string path_str = path.utf8().data();
        
        if (path_str.find("file:///") == 0) {
            path_str = path_str.substr(8);
        }

        auto cached = exists_cache_.get(path_str);

        if (cached.has_value()) {
            return cached.value();
        }
        
        // Cache miss - check filesystem
        bool exists = CheckExistsInternal(path_str);
        exists_cache_.put(path_str, exists);
        return exists;
    }

    ultralight::RefPtr<ultralight::Buffer> OpenFile(const ultralight::String& path) override {
        std::string path_str = path.utf8().data();
        
        if (path_str.find("file:///") == 0) {
            path_str = path_str.substr(8);
        }
        
        std::filesystem::path file_path = ResolvePath(path_str);
        
        if (file_path.empty()) {
            return nullptr;
        }
        
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        
        if (!file) {
            return nullptr;
        }
        
        std::streamsize size = file.tellg();
        
        if (size <= 0) {
            return nullptr;
        }
        
        file.seekg(0, std::ios::beg);
        std::vector<char> data(static_cast<size_t>(size));
        
        if (!file.read(data.data(), size)) {
            return nullptr;
        }
        
        return ultralight::Buffer::CreateFromCopy(data.data(), data.size());
    }

    ultralight::String GetFileMimeType(const ultralight::String& path) override {
        static const std::unordered_map<std::string, const char*> mime_types = {
            {".html", "text/html"}, {".htm", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"}, {".mjs", "application/javascript"},
            {".png", "image/png"}, {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
            {".gif", "image/gif"}, {".webp", "image/webp"},
            {".svg", "image/svg+xml"}, {".ico", "image/x-icon"},
            {".woff", "font/woff"}, {".woff2", "font/woff2"},
            {".ttf", "font/ttf"}, {".otf", "font/otf"},
            {".eot", "application/vnd.ms-fontobject"},
            {".json", "application/json"}, {".xml", "application/xml"}
        };
        
        std::string ext = std::filesystem::path(path.utf8().data()).extension().string();
        
        // Lowercase in-place
        for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        auto it = mime_types.find(ext);
        return (it != mime_types.end()) ? it->second : "application/octet-stream";
    }

    ultralight::String GetFileCharset(const ultralight::String& path) override {
        return "utf-8";
    }
    
    void clear_cache() {
        exists_cache_.clear();
    }

private:
    std::vector<std::filesystem::path> search_paths_;
    std::filesystem::path resources_path_;
    mutable LRUCache<std::string, bool> exists_cache_;

    bool CheckExistsInternal(const std::string& path_str) const {
        try {
            // Check resources path first
            if (!resources_path_.empty()) {
                if (std::filesystem::exists(resources_path_ / path_str)) {
                    return true;
                }
            }
            
            // Check search paths
            for (const auto& base : search_paths_) {
                if (std::filesystem::exists(base / path_str)) {
                    return true;
                }
            }
            
            // Check absolute/relative path
            return std::filesystem::exists(path_str);
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }
    
    std::filesystem::path ResolvePath(const std::string& path_str) const {
        try {
            // Check resources path first
            if (!resources_path_.empty()) {
                std::filesystem::path full = resources_path_ / path_str;

                if (std::filesystem::exists(full)) {
                    return full;
                }
            }
            
            // Check search paths
            for (const auto& base : search_paths_) {
                std::filesystem::path full = base / path_str;

                if (std::filesystem::exists(full)) {
                    return full;
                }
            }
            
            // Check absolute/relative path
            if (std::filesystem::exists(path_str)) {
                return path_str;
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Ignore filesystem errors
        }
        
        return {};
    }
};

static game_value js_value_to_game_value(JSContextRef ctx, JSValueRef value) {
    if (JSValueIsNull(ctx, value) || JSValueIsUndefined(ctx, value)) {
        return game_value();
    }
    
    if (JSValueIsBoolean(ctx, value)) {
        return game_value(JSValueToBoolean(ctx, value));
    }
    
    if (JSValueIsNumber(ctx, value)) {
        return game_value(static_cast<float>(JSValueToNumber(ctx, value, nullptr)));
    }
    
    if (JSValueIsString(ctx, value)) {
        JSStringRef js_str = JSValueToStringCopy(ctx, value, nullptr);
        size_t max_size = JSStringGetMaximumUTF8CStringSize(js_str);
        std::vector<char> buffer(max_size);
        JSStringGetUTF8CString(js_str, buffer.data(), max_size);
        JSStringRelease(js_str);
        return game_value(std::string(buffer.data()));
    }
    
    if (JSValueIsArray(ctx, value)) {
        JSObjectRef arr_obj = JSValueToObject(ctx, value, nullptr);
        JSStringRef length_str = JSStringCreateWithUTF8CString("length");
        JSValueRef length_val = JSObjectGetProperty(ctx, arr_obj, length_str, nullptr);
        JSStringRelease(length_str);
        size_t length = static_cast<size_t>(JSValueToNumber(ctx, length_val, nullptr));
        auto_array<game_value> result;
        result.reserve(length);
        
        for (size_t i = 0; i < length; i++) {
            JSValueRef element = JSObjectGetPropertyAtIndex(ctx, arr_obj, static_cast<unsigned>(i), nullptr);
            result.push_back(js_value_to_game_value(ctx, element));
        }
        
        return game_value(std::move(result));
    }
    
    if (JSValueIsObject(ctx, value)) {
        // Convert object to string representation (JSON)
        JSStringRef json_str = JSValueCreateJSONString(ctx, value, 0, nullptr);

        if (json_str) {
            size_t max_size = JSStringGetMaximumUTF8CStringSize(json_str);
            std::vector<char> buffer(max_size);
            JSStringGetUTF8CString(json_str, buffer.data(), max_size);
            JSStringRelease(json_str);
            return game_value(std::string(buffer.data()));
        }
    }
    
    return game_value();
}

class UIJavaScriptBridge {
public:
    static void inject_bridge(ultralight::View* view) {
        if (!view) return;
        auto ctx = view->LockJSContext();
        JSContextRef js_ctx = ctx->ctx();
        JSObjectRef global = JSContextGetGlobalObject(js_ctx);
        JSStringRef func_name = JSStringCreateWithUTF8CString("call_sqf_event");
        JSObjectRef func = JSObjectMakeFunctionWithCallback(js_ctx, func_name, call_sqf_event_callback);
        JSObjectSetProperty(js_ctx, global, func_name, func, kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(func_name);
    }
    
private:
    static JSValueRef call_sqf_event_callback(JSContextRef ctx, JSObjectRef function,
                                               JSObjectRef thisObject, size_t argumentCount,
                                               const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount == 0) {
            return JSValueMakeUndefined(ctx);
        }
        
        // Convert all arguments to a game_value array
        auto_array<game_value> args_array;
        args_array.reserve(argumentCount);
        
        for (size_t i = 0; i < argumentCount; i++) {
            args_array.push_back(js_value_to_game_value(ctx, arguments[i]));
        }
        
        // Create the final game_value (single arg or array)
        game_value args_to_send;

        if (argumentCount == 1) {
            args_to_send = std::move(args_array[0]);
        } else {
            args_to_send = game_value(std::move(args_array));
        }
        
        // Schedule execution on main thread
        MainThreadScheduler::instance().schedule([args = std::move(args_to_send)]() mutable {
            raw_call_sqf_args_native_no_return(g_compiled_html_js_event, std::move(args));
        });
        
        return JSValueMakeUndefined(ctx);
    }
};

class UIDocumentListener : public ultralight::ViewListener, public ultralight::LoadListener {
public:
    void OnAddConsoleMessage(ultralight::View* caller,
                             const ultralight::ConsoleMessage& msg) override {
        auto level = msg.level();
        
        if (level == ultralight::kMessageLevel_Error || 
            level == ultralight::kMessageLevel_Warning) {
            std::string level_str = (level == ultralight::kMessageLevel_Error) ? "Error" : "Warning";

            std::string err = "JS " + level_str
                            + " at line " + std::to_string(msg.line_number()) 
                            + ", col " + std::to_string(msg.column_number())
                            + ": " + std::string(msg.message().utf8().data());
            
            MainThreadScheduler::instance().schedule([err]() {
                report_error("KH - UI Framework: " + err);
            });
        }
    }
    
    void OnDOMReady(ultralight::View* caller,
                    uint64_t frame_id,
                    bool is_main_frame,
                    const ultralight::String& url) override {
        if (is_main_frame) {
            UIJavaScriptBridge::inject_bridge(caller);
        }
    }
};

struct UIDocument {
    std::string id;
    std::string html_path;
    std::string html_content;
    ultralight::RefPtr<ultralight::View> view;
    bool visible = true;
    bool fullscreen = false;
    bool interactive = true;
    int x = 0, y = 0, width = 0, height = 0;
    float opacity = 1.0f;
    int z_order = 0;
    std::vector<uint8_t> cached_pixels;
    int cached_width = 0;
    int cached_height = 0;
    int dirty_top = 0;
    int dirty_bottom = 0;
    uint32_t cached_stride = 0;
    std::atomic<bool> pixels_ready{false};
    std::atomic<bool> texture_needs_update{true};
    std::mutex pixel_mutex;
    std::unique_ptr<UIDocumentListener> listener;
};

class UIOverlayRenderer {
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11VertexShader> vertex_shader_;
    ComPtr<ID3D11PixelShader> pixel_shader_;
    ComPtr<ID3D11InputLayout> input_layout_;
    ComPtr<ID3D11Buffer> vertex_buffer_;
    ComPtr<ID3D11Buffer> constant_buffer_;
    ComPtr<ID3D11SamplerState> sampler_state_;
    ComPtr<ID3D11BlendState> blend_state_;
    ComPtr<ID3D11RasterizerState> rasterizer_state_;
    ComPtr<ID3D11DepthStencilState> depth_stencil_state_;
    ComPtr<ID3D11BlendState> saved_blend_;
    FLOAT saved_bf_[4] = {};
    UINT saved_sm_ = 0;
    ComPtr<ID3D11DepthStencilState> saved_ds_;
    UINT saved_sr_ = 0;
    ComPtr<ID3D11RasterizerState> saved_rs_;
    ComPtr<ID3D11RenderTargetView> saved_rtv_;
    ComPtr<ID3D11DepthStencilView> saved_dsv_;
    D3D11_VIEWPORT saved_vp_[16] = {};
    UINT saved_nvp_ = 0;
    D3D11_RECT saved_scissor_[16] = {};
    UINT saved_nsr_ = 0;
    ComPtr<ID3D11VertexShader> saved_vs_;
    ComPtr<ID3D11PixelShader> saved_ps_;
    ComPtr<ID3D11GeometryShader> saved_gs_;
    ComPtr<ID3D11InputLayout> saved_il_;
    D3D11_PRIMITIVE_TOPOLOGY saved_pt_ = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ComPtr<ID3D11Buffer> saved_vb_;
    UINT saved_vb_st_ = 0, saved_vb_of_ = 0;
    ComPtr<ID3D11Buffer> saved_ib_;
    DXGI_FORMAT saved_ib_fmt_ = DXGI_FORMAT_UNKNOWN;
    UINT saved_ib_off_ = 0;
    ComPtr<ID3D11Buffer> saved_cb_vs_, saved_cb_ps_;
    ComPtr<ID3D11ShaderResourceView> saved_srv_;
    ComPtr<ID3D11SamplerState> saved_samp_;

    struct DocTexture {
        ComPtr<ID3D11Texture2D> texture;
        ComPtr<ID3D11ShaderResourceView> srv;
        int width = 0, height = 0;
    };

    std::unordered_map<std::string, DocTexture> doc_textures_;
    std::atomic<bool> initialized_{false};
    mutable std::mutex texture_mutex_;
    std::mutex init_mutex_;
    struct Vertex { float x, y, z, u, v; };
    struct ConstantBuffer { float screen_width, screen_height, offset_x, offset_y, doc_width, doc_height, opacity, padding; };

    const char* vs_code_ = R"(
        cbuffer CB : register(b0) { float sw, sh, ox, oy, dw, dh, op, pad; };
        struct VSI { float3 pos : POSITION; float2 uv : TEXCOORD; };
        struct VSO { float4 pos : SV_POSITION; float2 uv : TEXCOORD; };
        VSO main(VSI i) { VSO o; o.pos = float4((i.pos.x*dw+ox)/sw*2-1, 1-(i.pos.y*dh+oy)/sh*2, 0, 1); o.uv = i.uv; return o; }
    )";

    const char* ps_code_ = R"(
        cbuffer CB : register(b0) { float sw, sh, ox, oy, dw, dh, op, pad; };
        Texture2D tex : register(t0); SamplerState samp : register(s0);
        float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET { float4 c = tex.Sample(samp, uv); c *= op; return c; }
    )";

public:
    bool initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
        std::lock_guard<std::mutex> lock(init_mutex_);
        if (initialized_.load(std::memory_order_acquire)) return true;
        if (!device || !context) return false;
        device_ = device; 
        context_ = context;
        ComPtr<ID3DBlob> vs_blob, ps_blob, err;
        if (FAILED(D3DCompile(vs_code_, strlen(vs_code_), 0, 0, 0, "main", "vs_4_0", 0, 0, &vs_blob, &err))) return false;
        if (FAILED(D3DCompile(ps_code_, strlen(ps_code_), 0, 0, 0, "main", "ps_4_0", 0, 0, &ps_blob, &err))) return false;
        if (FAILED(device_->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), 0, &vertex_shader_))) return false;
        if (FAILED(device_->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), 0, &pixel_shader_))) return false;

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

        if (FAILED(device_->CreateInputLayout(layout, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout_))) return false;
        Vertex verts[] = {{0,0,0,0,0},{1,0,0,1,0},{0,1,0,0,1},{1,1,0,1,1}};
        D3D11_BUFFER_DESC vbd = {sizeof(verts), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
        D3D11_SUBRESOURCE_DATA vd = {verts, 0, 0};
        if (FAILED(device_->CreateBuffer(&vbd, &vd, &vertex_buffer_))) return false;
        D3D11_BUFFER_DESC cbd = {sizeof(ConstantBuffer), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        if (FAILED(device_->CreateBuffer(&cbd, 0, &constant_buffer_))) return false;
        D3D11_SAMPLER_DESC sd = {}; sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        if (FAILED(device_->CreateSamplerState(&sd, &sampler_state_))) return false;
        D3D11_BLEND_DESC bd = {}; bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE; bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(device_->CreateBlendState(&bd, &blend_state_))) return false;
        D3D11_RASTERIZER_DESC rd = {}; rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
        if (FAILED(device_->CreateRasterizerState(&rd, &rasterizer_state_))) return false;
        D3D11_DEPTH_STENCIL_DESC dsd = {}; dsd.DepthEnable = FALSE;
        if (FAILED(device_->CreateDepthStencilState(&dsd, &depth_stencil_state_))) return false;
        initialized_.store(true, std::memory_order_release);
        return true;
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(init_mutex_);
        
        {
            std::lock_guard<std::mutex> tex_lock(texture_mutex_);
            doc_textures_.clear();
        }

        depth_stencil_state_.Reset();
        rasterizer_state_.Reset();
        blend_state_.Reset();
        sampler_state_.Reset();
        constant_buffer_.Reset();
        vertex_buffer_.Reset();
        input_layout_.Reset();
        pixel_shader_.Reset();
        vertex_shader_.Reset();
        context_.Reset();
        device_.Reset();
        initialized_.store(false, std::memory_order_release);
    }

    bool update_texture(const std::string& doc_id, const void* pixels, int width, int height, uint32_t stride) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        if (!device_ || !context_ || !pixels || width <= 0 || height <= 0) return false;
        std::lock_guard<std::mutex> lock(texture_mutex_);
        auto& tex = doc_textures_[doc_id];
        
        if (tex.width != width || tex.height != height) {
            tex.texture.Reset();
            tex.srv.Reset();
            D3D11_TEXTURE2D_DESC td = {};
            td.Width = width;
            td.Height = height;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.SampleDesc.Quality = 0;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            td.CPUAccessFlags = 0;
            td.MiscFlags = 0;
            if (FAILED(device_->CreateTexture2D(&td, nullptr, &tex.texture))) return false;
            if (FAILED(device_->CreateShaderResourceView(tex.texture.Get(), nullptr, &tex.srv))) return false;
            tex.width = width;
            tex.height = height;
        }

        context_->UpdateSubresource(tex.texture.Get(), 0, nullptr, pixels, stride, 0);
        return true;
    }
    
    bool update_texture_partial(const std::string& doc_id, const void* pixels, 
                                int width, int height, uint32_t stride,
                                int dirty_top, int dirty_bottom) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        if (!context_ || !pixels || width <= 0 || height <= 0) return false;
        if (dirty_top >= dirty_bottom) return true;
        std::lock_guard<std::mutex> lock(texture_mutex_);
        auto it = doc_textures_.find(doc_id);

        if (it == doc_textures_.end() || it->second.width != width || it->second.height != height) {
            return false;
        }
        
        D3D11_BOX box = {};
        box.left = 0;
        box.right = width;
        box.top = dirty_top;
        box.bottom = dirty_bottom;
        box.front = 0;
        box.back = 1;
        const uint8_t* src = static_cast<const uint8_t*>(pixels) + (dirty_top * stride);
        context_->UpdateSubresource(it->second.texture.Get(), 0, &box, src, stride, 0);
        return true;
    }

    void begin_batch(int sw, int sh) {
        if (!initialized_.load(std::memory_order_acquire) || !context_) return;
        context_->OMGetBlendState(&saved_blend_, saved_bf_, &saved_sm_);
        context_->OMGetDepthStencilState(&saved_ds_, &saved_sr_);
        context_->RSGetState(&saved_rs_);
        context_->OMGetRenderTargets(1, &saved_rtv_, &saved_dsv_);
        saved_nvp_ = 16; context_->RSGetViewports(&saved_nvp_, saved_vp_);
        saved_nsr_ = 16; context_->RSGetScissorRects(&saved_nsr_, saved_scissor_);
        context_->VSGetShader(&saved_vs_, 0, 0);
        context_->PSGetShader(&saved_ps_, 0, 0);
        context_->GSGetShader(&saved_gs_, 0, 0);
        context_->IAGetInputLayout(&saved_il_);
        context_->IAGetPrimitiveTopology(&saved_pt_);
        context_->IAGetVertexBuffers(0, 1, &saved_vb_, &saved_vb_st_, &saved_vb_of_);
        context_->IAGetIndexBuffer(&saved_ib_, &saved_ib_fmt_, &saved_ib_off_);
        context_->VSGetConstantBuffers(0, 1, &saved_cb_vs_);
        context_->PSGetConstantBuffers(0, 1, &saved_cb_ps_);
        context_->PSGetShaderResources(0, 1, &saved_srv_);
        context_->PSGetSamplers(0, 1, &saved_samp_);
        D3D11_VIEWPORT vp = {0, 0, (float)sw, (float)sh, 0, 1};
        context_->RSSetViewports(1, &vp);
        context_->OMSetBlendState(blend_state_.Get(), 0, 0xFFFFFFFF);
        context_->OMSetDepthStencilState(depth_stencil_state_.Get(), 0);
        context_->RSSetState(rasterizer_state_.Get());
        context_->VSSetShader(vertex_shader_.Get(), 0, 0);
        context_->PSSetShader(pixel_shader_.Get(), 0, 0);
        context_->GSSetShader(nullptr, 0, 0);
        context_->IASetInputLayout(input_layout_.Get());
        UINT stride = sizeof(Vertex), offset = 0;
        context_->IASetVertexBuffers(0, 1, vertex_buffer_.GetAddressOf(), &stride, &offset);
        context_->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context_->VSSetConstantBuffers(0, 1, constant_buffer_.GetAddressOf());
        context_->PSSetConstantBuffers(0, 1, constant_buffer_.GetAddressOf());
        context_->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());
    }
    
    void draw_quad(const std::string& doc_id, int sw, int sh, int dx, int dy, int dw, int dh, float opacity) {
        if (!initialized_.load(std::memory_order_acquire) || !context_) return;
        std::lock_guard<std::mutex> lock(texture_mutex_);
        auto it = doc_textures_.find(doc_id);
        if (it == doc_textures_.end() || !it->second.srv) return;
        D3D11_MAPPED_SUBRESOURCE m;

        if (SUCCEEDED(context_->Map(constant_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
            ConstantBuffer* cb = (ConstantBuffer*)m.pData;
            cb->screen_width = (float)sw; cb->screen_height = (float)sh;
            cb->offset_x = (float)dx; cb->offset_y = (float)dy;
            cb->doc_width = (float)dw; cb->doc_height = (float)dh;
            cb->opacity = opacity;
            context_->Unmap(constant_buffer_.Get(), 0);
        }

        context_->PSSetShaderResources(0, 1, it->second.srv.GetAddressOf());
        context_->Draw(4, 0);
    }

    void draw_quad_with_srv(ID3D11ShaderResourceView* srv, int sw, int sh, 
                            int dx, int dy, int dw, int dh, float opacity) {
        if (!initialized_.load(std::memory_order_acquire) || !context_ || !srv) return;
        D3D11_MAPPED_SUBRESOURCE m;

        if (SUCCEEDED(context_->Map(constant_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
            ConstantBuffer* cb = (ConstantBuffer*)m.pData;
            cb->screen_width = (float)sw; cb->screen_height = (float)sh;
            cb->offset_x = (float)dx; cb->offset_y = (float)dy;
            cb->doc_width = (float)dw; cb->doc_height = (float)dh;
            cb->opacity = opacity;
            context_->Unmap(constant_buffer_.Get(), 0);
        }

        context_->PSSetShaderResources(0, 1, &srv);
        context_->Draw(4, 0);
    }
        
    ID3D11ShaderResourceView* get_srv(const std::string& doc_id) {
        std::lock_guard<std::mutex> lock(texture_mutex_);
        auto it = doc_textures_.find(doc_id);
        if (it == doc_textures_.end() || !it->second.srv) return nullptr;
        return it->second.srv.Get();
    }

    void remove_doc_texture(const std::string& doc_id) {
        std::lock_guard<std::mutex> lock(texture_mutex_);
        doc_textures_.erase(doc_id);
    }
    
    void end_batch() {
        if (!initialized_.load(std::memory_order_acquire) || !context_) return;
        ID3D11ShaderResourceView* null_srv = nullptr;
        context_->PSSetShaderResources(0, 1, &null_srv);
        context_->OMSetRenderTargets(1, saved_rtv_.GetAddressOf(), saved_dsv_.Get());
        context_->OMSetBlendState(saved_blend_.Get(), saved_bf_, saved_sm_);
        context_->OMSetDepthStencilState(saved_ds_.Get(), saved_sr_);
        context_->RSSetState(saved_rs_.Get());
        context_->RSSetViewports(saved_nvp_, saved_vp_);
        if (saved_nsr_ > 0) context_->RSSetScissorRects(saved_nsr_, saved_scissor_);
        context_->VSSetShader(saved_vs_.Get(), 0, 0);
        context_->PSSetShader(saved_ps_.Get(), 0, 0);
        context_->GSSetShader(saved_gs_.Get(), 0, 0);
        context_->IASetInputLayout(saved_il_.Get());
        context_->IASetPrimitiveTopology(saved_pt_);
        context_->IASetVertexBuffers(0, 1, saved_vb_.GetAddressOf(), &saved_vb_st_, &saved_vb_of_);
        context_->IASetIndexBuffer(saved_ib_.Get(), saved_ib_fmt_, saved_ib_off_);
        context_->VSSetConstantBuffers(0, 1, saved_cb_vs_.GetAddressOf());
        context_->PSSetConstantBuffers(0, 1, saved_cb_ps_.GetAddressOf());
        context_->PSSetShaderResources(0, 1, saved_srv_.GetAddressOf());
        context_->PSSetSamplers(0, 1, saved_samp_.GetAddressOf());
        saved_blend_.Reset(); saved_ds_.Reset(); saved_rs_.Reset();
        saved_rtv_.Reset(); saved_dsv_.Reset(); saved_vs_.Reset();
        saved_ps_.Reset(); saved_gs_.Reset(); saved_il_.Reset();
        saved_vb_.Reset(); saved_ib_.Reset(); saved_cb_vs_.Reset();
        saved_cb_ps_.Reset(); saved_srv_.Reset(); saved_samp_.Reset();
    }

    bool is_initialized() const { return initialized_.load(std::memory_order_acquire); }
};

class UIFramework {
public:
    static UIFramework& instance() {
        static UIFramework inst;
        return inst;
    }

    // Initialize the UI framework and start the worker thread
    bool initialize() {
        if (shutting_down_.load(std::memory_order_acquire)) return false;
        if (initialized_.load(std::memory_order_acquire)) return true;
        std::lock_guard<std::mutex> lock(init_mutex_);
        if (shutting_down_.load(std::memory_order_acquire)) return false;
        if (initialized_.load(std::memory_order_acquire)) return true;
        should_stop_.store(false, std::memory_order_release);
        worker_thread_ = std::thread(&UIFramework::worker_thread_func, this);
        auto start = std::chrono::steady_clock::now();
        constexpr auto timeout = std::chrono::seconds(10);
        
        while (!worker_initialized_.load(std::memory_order_acquire) &&
               !worker_init_failed_.load(std::memory_order_acquire)) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - UI Framework: Worker thread initialization timeout");
                });

                should_stop_.store(true, std::memory_order_release);
                worker_cv_.notify_all();

                if (worker_thread_.joinable()) {
                    worker_thread_.join();
                }

                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (worker_init_failed_.load(std::memory_order_acquire)) {
            if (worker_thread_.joinable()) {
                worker_thread_.join();
            }

            return false;
        }
        
        initialized_.store(true, std::memory_order_release);
        return true;
    }

    void emergency_shutdown() {
        shutting_down_.store(true, std::memory_order_seq_cst);
        initialized_.store(false, std::memory_order_seq_cst);
        instance_ptr_.store(nullptr, std::memory_order_seq_cst);
        should_stop_.store(true, std::memory_order_seq_cst);
        worker_cv_.notify_all();

        __try {
            uninstall_wndproc_hook();
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {}

        __try {
            if (hook_installed_.load(std::memory_order_acquire) && hooked_present_addr_) {
                MH_DisableHook(hooked_present_addr_);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Ignore failures during emergency shutdown
        }
        
        hook_installed_.store(false, std::memory_order_release);
    }

    void shutdown() {
        if (!initialized_.load(std::memory_order_acquire) && 
            !worker_thread_.joinable()) {
            return;
        }

        shutting_down_.store(true, std::memory_order_seq_cst);
        should_stop_.store(true, std::memory_order_release);
        
        {
            std::lock_guard<std::mutex> lock(worker_mutex_);
            has_pending_commands_.store(true, std::memory_order_release);
        }

        worker_cv_.notify_all();

        {
            std::lock_guard<std::mutex> lock(hook_mutex_);
            
            if (hook_installed_.load(std::memory_order_acquire) && hooked_present_addr_) {
                MH_DisableHook(hooked_present_addr_);
            }
        }

        auto start = std::chrono::steady_clock::now();
        constexpr auto timeout = std::chrono::milliseconds(1000);

        while (hook_executing_.load(std::memory_order_seq_cst)) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            
            if (elapsed >= timeout) {                
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            std::this_thread::yield();
        }

        // Wait for worker thread to finish
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        try {
            uninstall_wndproc_hook();
        } catch (...) {}
        
        try {
            uninstall_present_hook_internal();
        } catch (...) {}

        std::lock_guard<std::mutex> lock(init_mutex_);

        try {
            d3d_renderer_.cleanup();
        } catch (...) {}

        // Reset ALL state for potential re-initialization
        d3d_initialized_.store(false, std::memory_order_release);
        initialized_.store(false, std::memory_order_release);
        worker_initialized_.store(false, std::memory_order_release);
        worker_init_failed_.store(false, std::memory_order_release);
        should_stop_.store(false, std::memory_order_release);
        has_pending_commands_.store(false, std::memory_order_release);
        shutting_down_.store(false, std::memory_order_release);
        last_swap_chain_ = nullptr;
    }

    std::string create_html(const std::string& html_content, int x, int y, int width, int height, float opacity = 1.0f) {
        if (!initialized_.load(std::memory_order_acquire) && !initialize()) return "";
        if (html_content.empty()) return "";
        std::string doc_id = UIDGenerator::generate();
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        
        queue_command([this, doc_id, html_content, x, y, width, height, opacity, promise]() {
            try {
                create_html_internal(doc_id, html_content, x, y, width, height, opacity);
                promise->set_value(true);
            } catch (...) {
                promise->set_value(false);
            }
        });

        future.wait();

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            
            if (documents_.find(doc_id) == documents_.end()) {
                return "";
            }
        }
        
        return doc_id;
    }

    std::string open_html(const std::string& filename, int x, int y, int width, int height, float opacity = 1.0f) {
        if (!initialized_.load(std::memory_order_acquire) && !initialize()) return "";
        std::string doc_id = UIDGenerator::generate();
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        
        queue_command([this, doc_id, filename, x, y, width, height, opacity, promise]() {
            try {
                open_html_internal(doc_id, filename, x, y, width, height, opacity);
                promise->set_value(true);
            } catch (...) {
                promise->set_value(false);
            }
        });

        future.wait();

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            
            if (documents_.find(doc_id) == documents_.end()) {
                return "";
            }
        }
        
        return doc_id;
    }

    bool close_html(const std::string& doc_id) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            if (documents_.find(doc_id) == documents_.end()) return false;
        }
        
        queue_command([this, doc_id]() {
            close_html_internal(doc_id);
        });
        
        return true;
    }

    bool set_html_position(const std::string& doc_id, int x, int y) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        it->second->x = x;
        it->second->y = y;
        return true;
    }

    bool set_html_opacity(const std::string& doc_id, float opacity) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        it->second->opacity = std::clamp(opacity, 0.0f, 1.0f);
        return true;
    }

    bool set_html_size(const std::string& doc_id, int width, int height) {
        if (width <= 0 || height <= 0) return false;
        if (!initialized_.load(std::memory_order_acquire)) return false;
        if (shutting_down_.load(std::memory_order_acquire)) return false;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            if (documents_.find(doc_id) == documents_.end()) return false;
        }
        
        queue_command([this, doc_id, width, height]() {
            set_html_size_internal(doc_id, width, height);
        });
        
        return true;
    }

    bool set_html_z_order(const std::string& doc_id, int z_order) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        it->second->z_order = z_order;
        return true;
    }

    bool bring_html_to_front(const std::string& doc_id) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        int max_z = 0;

        for (const auto& [id, doc] : documents_) {
            if (doc->z_order > max_z) max_z = doc->z_order;
        }

        if (max_z > 100000) {
            normalize_z_orders();
            max_z = static_cast<int>(documents_.size()) - 1;
        }
        
        it->second->z_order = max_z + 1;
        return true;
    }

    bool send_html_to_back(const std::string& doc_id) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        int min_z = 0;

        for (const auto& [id, doc] : documents_) {
            if (doc->z_order < min_z) min_z = doc->z_order;
        }

        if (min_z < -100000) {
            normalize_z_orders();
            min_z = 0;
        }

        it->second->z_order = min_z - 1;
        return true;
    }

    std::string reload_html(const std::string& doc_id) {
        if (!initialized_.load(std::memory_order_acquire)) return "";
        if (shutting_down_.load(std::memory_order_acquire)) return "";
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return "";
            if (!it->second || !it->second->view) return "";
        }
        
        queue_command([this, doc_id]() {
            reload_html_internal(doc_id);
        });
        
        return doc_id;
    }

    bool execute_javascript(const std::string& doc_id, const std::string& script) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        if (shutting_down_.load(std::memory_order_acquire)) return false;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return false;
            if (!it->second || !it->second->view) return false;
        }
        
        queue_command([this, doc_id, script]() {
            execute_javascript_internal(doc_id, script);
        });
        
        return true;
    }

    bool set_js_variable(const std::string& doc_id, const std::string& var_name, const std::string& value_json) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        if (shutting_down_.load(std::memory_order_acquire)) return false;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return false;
            if (!it->second || !it->second->view) return false;
        }
        
        std::string script = "window[\"" + var_name + "\"] = " + value_json + ";";
        
        queue_command([this, doc_id, script]() {
            execute_javascript_internal(doc_id, script);
        });
        
        return true;
    }

    std::string get_js_variable(const std::string& doc_id, const std::string& var_name) {
        if (!initialized_.load(std::memory_order_acquire)) return "";
        if (shutting_down_.load(std::memory_order_acquire)) return "";
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return "";
            if (!it->second || !it->second->view) return "";
        }
        
        std::string script = "JSON.stringify(window[\"" + var_name + "\"])";
        std::string result;
        auto promise = std::make_shared<std::promise<std::string>>();
        auto future = promise->get_future();
        
        queue_command([this, doc_id, script, promise]() {
            try {
                promise->set_value(execute_javascript_internal(doc_id, script));
            } catch (...) {
                promise->set_value("");
            }
        });
        
        return future.get();
    }

    bool set_html_visible(const std::string& doc_id, bool visible) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        it->second->visible = visible;
        return true;
    }

    std::vector<std::string> get_open_documents() {
        std::lock_guard<std::mutex> lock(documents_mutex_);
        std::vector<std::string> result;
        result.reserve(documents_.size());
        for (const auto& [id, doc] : documents_) result.push_back(id);
        return result;
    }

    bool is_initialized() const { return initialized_.load(std::memory_order_acquire); }

    void set_mouse_enabled(bool enabled) {
        mouse_enabled_.store(enabled, std::memory_order_release);
    }

    void on_present(IDXGISwapChain* swap_chain) {
        if (!swap_chain) return;
        if (shutting_down_.load(std::memory_order_seq_cst)) return;
        if (!initialized_.load(std::memory_order_acquire)) return;
        hook_executing_.store(true, std::memory_order_seq_cst);

        struct HookGuard {
            std::atomic<bool>& flag;
            ~HookGuard() { flag.store(false, std::memory_order_seq_cst); }
        } guard{hook_executing_};

        if (shutting_down_.load(std::memory_order_seq_cst)) return;

        if (swap_chain != last_swap_chain_) {
            ComPtr<ID3D11Texture2D> back_buffer;

            if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
                D3D11_TEXTURE2D_DESC desc;
                back_buffer->GetDesc(&desc);
                screen_width_.store(desc.Width, std::memory_order_relaxed);
                screen_height_.store(desc.Height, std::memory_order_relaxed);
            }

            last_swap_chain_ = swap_chain;
        }

        // Initialize D3D renderer if needed
        if (!d3d_initialized_.load(std::memory_order_acquire)) {
            auto ri = RVExtBridge::get_render_info();

            if (ri && ri->d3dDevice && ri->d3dDeviceContext) {
                if (d3d_renderer_.initialize(ri->d3dDevice, ri->d3dDeviceContext)) {
                    d3d_initialized_.store(true, std::memory_order_release);
                }
            }
        }
        
        if (!d3d_initialized_.load(std::memory_order_acquire)) return;
        int sw = screen_width_.load(std::memory_order_relaxed);
        int sh = screen_height_.load(std::memory_order_relaxed);

        struct RenderItem {
            std::shared_ptr<UIDocument> doc;
            const std::string* id;
            ID3D11ShaderResourceView* srv;
            int x, y, width, height;
            float opacity;
            int z_order;
        };

        std::vector<RenderItem> render_list;

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            render_list.reserve(documents_.size());
            
            for (auto& [id, doc] : documents_) {
                if (doc && doc->visible && doc->pixels_ready.load(std::memory_order_acquire)) {
                    render_list.push_back({
                        doc,
                        &doc->id,
                        nullptr,
                        doc->x, doc->y, doc->width, doc->height,
                        doc->opacity,
                        doc->z_order
                    });
                }
            }
        }

        if (render_list.empty()) return;

        // Sort by z_order
        std::sort(render_list.begin(), render_list.end(),
            [](const auto& a, const auto& b) { return a.z_order < b.z_order; });

        d3d_renderer_.begin_batch(sw, sh);

        // Update textures that need it
        for (auto& item : render_list) {
            if (shutting_down_.load(std::memory_order_acquire)) break;

            if (item.x + item.width < 0 || item.x > sw ||
                item.y + item.height < 0 || item.y > sh) continue;

            if (!item.doc->texture_needs_update.load(std::memory_order_acquire)) continue;
            if (item.opacity <= 0.0f) continue;
            std::lock_guard<std::mutex> pixel_lock(item.doc->pixel_mutex);
            if (item.doc->cached_pixels.empty()) continue;
            if (!item.doc->pixels_ready.load(std::memory_order_acquire)) continue;
            
            // Try partial update first
            bool partial_ok = d3d_renderer_.update_texture_partial(
                *item.id,
                item.doc->cached_pixels.data(),
                item.doc->cached_width,
                item.doc->cached_height,
                item.doc->cached_stride,
                item.doc->dirty_top,
                item.doc->dirty_bottom
            );
            
            // Fall back to full update if partial failed
            if (!partial_ok) {
                d3d_renderer_.update_texture(
                    *item.id,
                    item.doc->cached_pixels.data(),
                    item.doc->cached_width,
                    item.doc->cached_height,
                    item.doc->cached_stride
                );
            }

            item.doc->texture_needs_update.store(false, std::memory_order_release);
        }

        // Pre-fetch all SRVs
        for (auto& item : render_list) {
            item.srv = d3d_renderer_.get_srv(*item.id);
        }

        // Draw all quads
        for (const auto& item : render_list) {
            if (shutting_down_.load(std::memory_order_acquire)) break;
            if (!item.srv || item.opacity <= 0.0f) continue;

            if (item.x + item.width < 0 || item.x > sw ||
                item.y + item.height < 0 || item.y > sh) continue;
            
            d3d_renderer_.draw_quad_with_srv(
                item.srv,
                sw, sh,
                item.x, item.y,
                item.width, item.height,
                item.opacity
            );
        }
        
        d3d_renderer_.end_batch();
    }

private:
    UIFramework() = default;
    ~UIFramework() { shutdown(); }
    UIFramework(const UIFramework&) = delete;
    UIFramework& operator=(const UIFramework&) = delete;

    void worker_thread_func() {
        try {
            if (!initialize_ultralight()) {
                worker_init_failed_.store(true, std::memory_order_release);
                return;
            }
            
            worker_initialized_.store(true, std::memory_order_release);
            install_present_hook();

            MainThreadScheduler::instance().schedule([this]() {
                install_wndproc_hook();
            });
                        
            // Main processing loop
            while (!should_stop_.load(std::memory_order_acquire)) {
                process_commands();
                update_ultralight();

                {
                    std::unique_lock<std::mutex> lock(worker_mutex_);

                    worker_cv_.wait_for(lock, std::chrono::milliseconds(16), [this] {
                        return should_stop_.load(std::memory_order_acquire) ||
                               has_pending_commands_.load(std::memory_order_acquire);
                    });
                }
            }

            cleanup_ultralight();            
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - UI Framework: Worker thread error: " + error_msg);
            });

            worker_init_failed_.store(true, std::memory_order_release);
        } catch (...) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Worker thread unknown error");
            });

            worker_init_failed_.store(true, std::memory_order_release);
        }
    }

    bool initialize_ultralight() {
        try {
            html_dirs_ = find_html_ui_directories();
            std::filesystem::path resources_dir;
            char module_path[MAX_PATH];
            HMODULE this_module = nullptr;
            static const int module_marker = 0;
            
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCSTR>(&module_marker), &this_module) &&
                GetModuleFileNameA(this_module, module_path, MAX_PATH)) {
                std::filesystem::path dll_path(module_path);
                std::string path_str = dll_path.string();
                
                if (path_str.find("\\\\?\\") == 0) {
                    path_str = path_str.substr(4);
                    dll_path = path_str;
                }

                std::filesystem::path mod_dir = dll_path.parent_path().parent_path();
                std::filesystem::path res_path = mod_dir / "ultralight_resources";
                
                if (std::filesystem::exists(res_path)) {
                    resources_dir = res_path;
                }
            }

            file_system_ = std::make_unique<UIFileSystem>(html_dirs_, resources_dir);
            ultralight::Platform::instance().set_file_system(file_system_.get());
            font_loader_ = std::make_unique<DirectWriteFontLoader>();
            ultralight::Platform::instance().set_font_loader(font_loader_.get());
            ultralight::Config config;
            config.animation_timer_delay = 1.0 / 60.0;
            config.scroll_timer_delay = 1.0 / 60.0;
            config.recycle_delay = 1.0 / 60.0;
            ultralight::Platform::instance().set_config(config);
            renderer_ = ultralight::Renderer::Create();
            
            if (!renderer_) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - UI Framework: Failed to create renderer");
                });

                file_system_.reset();
                font_loader_.reset();
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - UI Framework: init failed: " + error_msg);
            });

            file_system_.reset();
            font_loader_.reset();
            renderer_ = nullptr;
            return false;
        } catch (...) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: init failed: Unknown exception");
            });

            file_system_.reset();
            font_loader_.reset();
            renderer_ = nullptr;
            return false;
        }
    }

    void cleanup_ultralight() {
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);

            for (auto& [id, doc] : documents_) {
                if (doc && doc->view) {
                    doc->view->set_view_listener(nullptr);
                    doc->view->set_load_listener(nullptr);
                    doc->view = nullptr;  // Release the view
                }

                if (doc) {
                    doc->listener.reset();
                }
            }

            documents_.clear();
        }

        if (renderer_) {
            try {
                renderer_->Update();
                renderer_->Render();
            } catch (...) {}
        }

        renderer_ = nullptr;
        file_system_.reset();
        font_loader_.reset();
    }

    void update_ultralight() {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        if (!renderer_) return;
        
        // Update Ultralight - advances animations and JS timers
        renderer_->Update();
        renderer_->RefreshDisplay(0);
        renderer_->Render();

        // Cache pixels for each document
        std::lock_guard<std::mutex> lock(documents_mutex_);
        
        for (auto& [id, doc] : documents_) {
            if (!doc || !doc->visible || !doc->view) continue;
            auto* surface = doc->view->surface();
            if (!surface) continue;
            auto* bmp_surface = static_cast<ultralight::BitmapSurface*>(surface);
            ultralight::IntRect dirty = bmp_surface->dirty_bounds();

            if (dirty.IsEmpty()) {
                continue;
            }
            
            auto bitmap = bmp_surface->bitmap();
            if (!bitmap) continue;
            void* pixels = bitmap->LockPixels();

            if (pixels) {
                struct UnlockGuard {
                    ultralight::RefPtr<ultralight::Bitmap>& bmp;
                    ~UnlockGuard() { bmp->UnlockPixels(); }
                } unlock_guard{bitmap};
                
                std::lock_guard<std::mutex> pixel_lock(doc->pixel_mutex);
                int w = bitmap->width();
                int h = bitmap->height();
                uint32_t stride = bitmap->row_bytes();
                size_t size = static_cast<size_t>(h) * stride;

                if (doc->cached_pixels.size() != size) {
                    doc->cached_pixels.resize(size);
                    memcpy(doc->cached_pixels.data(), pixels, size);
                    doc->dirty_top = 0;
                    doc->dirty_bottom = h;
                } else {
                    int dirty_top = std::max(0, dirty.top);
                    int dirty_bottom = std::min(h, dirty.bottom);
                    doc->dirty_top = dirty_top;
                    doc->dirty_bottom = dirty_bottom;

                    if (dirty_top < dirty_bottom) {
                        size_t offset = static_cast<size_t>(dirty_top) * stride;
                        size_t copy_size = static_cast<size_t>(dirty_bottom - dirty_top) * stride;
                        
                        memcpy(doc->cached_pixels.data() + offset, 
                            static_cast<const uint8_t*>(pixels) + offset, 
                            copy_size);
                    }
                }
                
                doc->cached_width = w;
                doc->cached_height = h;
                doc->cached_stride = stride;
                doc->pixels_ready.store(true, std::memory_order_release);
                doc->texture_needs_update.store(true, std::memory_order_release);
                bmp_surface->ClearDirtyBounds();
            }
        }
    }

    void queue_command(std::function<void()> cmd) {
        {
            std::lock_guard<std::mutex> lock(command_mutex_);
            pending_commands_.push_back(std::move(cmd));
            has_pending_commands_.store(true, std::memory_order_release);
        }
        worker_cv_.notify_one();
    }

    void process_commands() {
        std::deque<std::function<void()>> commands_to_execute;
        
        {
            std::lock_guard<std::mutex> lock(command_mutex_);
            if (pending_commands_.empty()) return;
            commands_to_execute.swap(pending_commands_);
            has_pending_commands_.store(false, std::memory_order_release);
        }
        
        for (auto& cmd : commands_to_execute) {
            try {
                cmd();
            } catch (const std::exception& e) {
                std::string error_msg = e.what();

                MainThreadScheduler::instance().schedule([error_msg]() {
                    report_error("KH - UI Framework: Command error: " + error_msg);
                });
            } catch (...) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - UI Framework: Command unknown error");
                });
            }
        }
    }

    void create_html_internal(const std::string& doc_id, const std::string& html_content, 
                              int x, int y, int width, int height, float opacity) {
        if (!renderer_) return;
        int w = width, h = height;
        bool fs = (width <= 0 || height <= 0);
        if (fs) { w = GetSystemMetrics(SM_CXSCREEN); h = GetSystemMetrics(SM_CYSCREEN); }
        ultralight::ViewConfig vc;
        vc.is_accelerated = false;
        vc.is_transparent = true;
        auto view = renderer_->CreateView(w, h, vc, nullptr);

        if (!view) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Failed to create view for dynamic HTML");
            });

            return;
        }
        
        view->Focus();
        auto doc = std::make_shared<UIDocument>();
        doc->id = doc_id;
        doc->html_content = html_content;
        doc->view = view;
        doc->x = x; doc->y = y;
        doc->width = w; doc->height = h;
        doc->opacity = std::clamp(opacity, 0.0f, 1.0f);
        doc->fullscreen = fs;
        doc->listener = std::make_unique<UIDocumentListener>();
        view->set_view_listener(doc->listener.get());
        view->set_load_listener(doc->listener.get());
        view->LoadHTML(ultralight::String(html_content.c_str()));

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            doc->z_order = static_cast<int>(documents_.size());
            documents_[doc->id] = doc;
        }
    }

    void open_html_internal(const std::string& doc_id, const std::string& filename, 
                            int x, int y, int width, int height, float opacity) {
        auto html_path = find_html_file(filename);

        if (html_path.empty()) {
            MainThreadScheduler::instance().schedule([filename]() {
                report_error("KH - UI Framework: HTML not found: " + filename);
            });

            return;
        }

        if (!renderer_) return;
        int w = width, h = height;
        bool fs = (width <= 0 || height <= 0);
        if (fs) { w = GetSystemMetrics(SM_CXSCREEN); h = GetSystemMetrics(SM_CYSCREEN); }
        ultralight::ViewConfig vc;
        vc.is_accelerated = false;
        vc.is_transparent = true;
        auto view = renderer_->CreateView(w, h, vc, nullptr);

        if (!view) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Failed to create view");
            });

            return;
        }
        
        view->Focus();
        auto doc = std::make_shared<UIDocument>();
        doc->id = doc_id;
        doc->html_path = html_path.string();
        doc->view = view;
        doc->x = x; doc->y = y;
        doc->width = w; doc->height = h;
        doc->opacity = std::clamp(opacity, 0.0f, 1.0f);
        doc->fullscreen = fs;
        doc->listener = std::make_unique<UIDocumentListener>();
        view->set_view_listener(doc->listener.get());
        view->set_load_listener(doc->listener.get());
        std::string url = "file:///" + html_path.string();
        std::replace(url.begin(), url.end(), '\\', '/');
        view->LoadURL(ultralight::String(url.c_str()));

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            doc->z_order = static_cast<int>(documents_.size());
            documents_[doc->id] = doc;
        }
    }

    void close_html_internal(const std::string& doc_id) {
        std::shared_ptr<UIDocument> doc_to_close;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return;
            doc_to_close = it->second;
            documents_.erase(it);
        }

        d3d_renderer_.remove_doc_texture(doc_id);
        
        if (doc_to_close) {
            doc_to_close->view = nullptr;
            std::lock_guard<std::mutex> pixel_lock(doc_to_close->pixel_mutex);
            doc_to_close->cached_pixels.clear();
            doc_to_close->pixels_ready.store(false, std::memory_order_release);
        }
    }

    void set_html_size_internal(const std::string& doc_id, int width, int height) {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        
        std::shared_ptr<UIDocument> doc;
        std::string html_path_copy;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return;
            doc = it->second;
            html_path_copy = doc->html_path;
        }
        
        if (!renderer_ || !doc) return;
        ultralight::ViewConfig vc;
        vc.is_accelerated = false;
        vc.is_transparent = true;
        auto new_view = renderer_->CreateView(width, height, vc, nullptr);
        if (!new_view) return;
        new_view->set_view_listener(doc->listener.get());
        new_view->set_load_listener(doc->listener.get());

        if (!doc->html_content.empty()) {
            new_view->LoadHTML(ultralight::String(doc->html_content.c_str()));
        } else {
            std::string url = "file:///" + html_path_copy;
            std::replace(url.begin(), url.end(), '\\', '/');
            new_view->LoadURL(ultralight::String(url.c_str()));
        }

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            
            if (it == documents_.end() || it->second != doc) {
                return;
            }

            new_view->Focus();
            doc->view = new_view;
            doc->width = width;
            doc->height = height;
            doc->fullscreen = false;
            std::lock_guard<std::mutex> pixel_lock(doc->pixel_mutex);
            doc->pixels_ready.store(false, std::memory_order_release);
            doc->cached_pixels.clear();
        }
    }

    void reload_html_internal(const std::string& doc_id) {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        std::shared_ptr<UIDocument> doc;
        std::string html_path_copy;
        ultralight::RefPtr<ultralight::View> view_copy;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return;
            doc = it->second;
            if (!doc || !doc->view) return;
            html_path_copy = doc->html_path;
            view_copy = doc->view;
        }
        
        if (!view_copy) return;

        if (!doc->html_content.empty()) {
            view_copy->LoadHTML(ultralight::String(doc->html_content.c_str()));
        } else {
            std::string url = "file:///" + html_path_copy;
            std::replace(url.begin(), url.end(), '\\', '/');
            view_copy->LoadURL(ultralight::String(url.c_str()));
        }
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            
            if (it == documents_.end() || it->second != doc) {
                return;
            }

            std::lock_guard<std::mutex> pixel_lock(doc->pixel_mutex);
            doc->pixels_ready.store(false, std::memory_order_release);
        }
    }

    std::string execute_javascript_internal(const std::string& doc_id, const std::string& script) {
        if (shutting_down_.load(std::memory_order_acquire)) return "";
        ultralight::RefPtr<ultralight::View> view_copy;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return "";
            if (!it->second || !it->second->view) return "";
            view_copy = it->second->view;
        }
        
        if (!view_copy) return "";
        auto result = view_copy->EvaluateScript(ultralight::String(script.c_str()));
        return std::string(result.utf8().data());
    }

    std::unordered_map<std::string, std::shared_ptr<UIDocument>> documents_;
    std::vector<std::filesystem::path> html_dirs_;
    ultralight::RefPtr<ultralight::Renderer> renderer_;
    std::unique_ptr<UIFileSystem> file_system_;
    std::unique_ptr<DirectWriteFontLoader> font_loader_;
    UIOverlayRenderer d3d_renderer_;
    std::atomic<bool> d3d_initialized_{false};
    std::atomic<bool> initialized_{false};
    std::atomic<int> screen_width_{1920};
    std::atomic<int> screen_height_{1080};
    std::atomic<bool> shutting_down_{false};
    std::atomic<bool> hook_executing_{false};
    IDXGISwapChain* last_swap_chain_ = nullptr;
    static std::atomic<WNDPROC> original_wndproc_;
    static std::atomic<HWND> game_hwnd_;
    std::atomic<int> last_mouse_x_{0};
    std::atomic<int> last_mouse_y_{0};
    std::string hovered_doc_id_;
    std::atomic<bool> mouse_enabled_{false};
    std::mutex mouse_mutex_;
    
    // Mutex for document map operations
    std::mutex documents_mutex_;
    std::mutex init_mutex_;

    // Worker thread members
    std::thread worker_thread_;
    std::atomic<bool> should_stop_{false};
    std::atomic<bool> worker_initialized_{false};
    std::atomic<bool> worker_init_failed_{false};
    std::mutex worker_mutex_;
    std::condition_variable worker_cv_;
    
    // Command queue
    std::deque<std::function<void()>> pending_commands_;
    std::mutex command_mutex_;
    std::atomic<bool> has_pending_commands_{false};

    // MinHook members
    static std::atomic<UIFramework*> instance_ptr_;
    typedef HRESULT(STDMETHODCALLTYPE* PresentFn)(IDXGISwapChain*, UINT, UINT);
    static PresentFn original_present_;
    static void* hooked_present_addr_;
    static std::atomic<bool> hook_installed_;
    static std::atomic<bool> minhook_initialized_;
    static std::mutex hook_mutex_;

    std::vector<std::filesystem::path> find_html_ui_directories() {
        std::vector<std::filesystem::path> paths;

        try {
            char docs[MAX_PATH];

            if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs) == S_OK) {
                auto p = std::filesystem::path(docs) / "Arma 3" / "kh_framework" / "html_ui";
                if (std::filesystem::exists(p)) paths.push_back(p);
            }
        } catch (...) {}

        auto mod_dirs = ModFolderSearcher::find_directories_in_mods("html_ui");
        paths.insert(paths.end(), mod_dirs.begin(), mod_dirs.end());
        return paths;
    }

    std::filesystem::path find_html_file(const std::string& filename) {
        for (const auto& base : html_dirs_) {
            auto p = base / filename;
            if (std::filesystem::exists(p)) return p;
        }

        return {};
    }

    static HRESULT STDMETHODCALLTYPE hooked_present(IDXGISwapChain* sc, UINT si, UINT f) {
        PresentFn orig = original_present_;
        
        __try {
            UIFramework* ptr = instance_ptr_.load(std::memory_order_acquire);
            if (ptr && !ptr->shutting_down_.load(std::memory_order_acquire)) {
                ptr->on_present(sc);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Crash during rendering - likely D3D teardown, ignore
        }

        if (orig) {
            __try {
                return orig(sc, si, f);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                return S_OK;
            }
        }
        
        return S_OK;
    }

    static HWND find_game_window() {
        // Try window class name first
        HWND hwnd = FindWindowA("Arma 3", nullptr);
        if (hwnd) return hwnd;
        
        // Fallback: enumerate windows for this process
        DWORD current_pid = GetCurrentProcessId();
        HWND found_hwnd = nullptr;
        
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            HWND* out_hwnd = reinterpret_cast<HWND*>(lParam);
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            
            if (pid == GetCurrentProcessId() && IsWindowVisible(hwnd)) {
                // Check if it's a main window (has no owner)
                if (GetWindow(hwnd, GW_OWNER) == nullptr) {
                    *out_hwnd = hwnd;
                    return FALSE;  // Stop enumeration
                }
            }
            
            return TRUE;  // Continue enumeration
        }, reinterpret_cast<LPARAM>(&found_hwnd));
        
        return found_hwnd;
    }

    bool install_present_hook() {
        std::lock_guard<std::mutex> lock(hook_mutex_);
        if (hook_installed_.load(std::memory_order_acquire)) return true;
        instance_ptr_.store(this, std::memory_order_release);
        HWND hwnd = find_game_window();
        if (!hwnd) hwnd = GetDesktopWindow();
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = sd.BufferDesc.Height = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        ComPtr<ID3D11Device> dev;
        ComPtr<ID3D11DeviceContext> ctx;
        ComPtr<IDXGISwapChain> temp_swap_chain;
        D3D_FEATURE_LEVEL fl;

        if (FAILED(D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, 
            D3D11_SDK_VERSION, &sd, &temp_swap_chain, &dev, &fl, &ctx))) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Failed to create temp swap chain for hook");
            });

            instance_ptr_.store(nullptr, std::memory_order_release);
            return false;
        }

        void** vtable = *(void***)temp_swap_chain.Get();
        void* present_addr = vtable[8];

        // Only initialize MinHook once
        if (!minhook_initialized_.load(std::memory_order_acquire)) {
            if (MH_Initialize() != MH_OK) {
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - UI Framework: MinHook init failed");
                });

                instance_ptr_.store(nullptr, std::memory_order_release);
                return false;
            }
            
            minhook_initialized_.store(true, std::memory_order_release);
        }

        if (MH_CreateHook(present_addr, &hooked_present, (void**)&original_present_) != MH_OK) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Failed to create Present hook");
            });

            instance_ptr_.store(nullptr, std::memory_order_release);
            return false;
        }

        if (MH_EnableHook(present_addr) != MH_OK) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Failed to enable Present hook");
            });

            MH_RemoveHook(present_addr);
            instance_ptr_.store(nullptr, std::memory_order_release);
            return false;
        }

        hooked_present_addr_ = present_addr;
        hook_installed_.store(true, std::memory_order_release);
        return true;
    }

    void uninstall_present_hook_internal() {
        std::lock_guard<std::mutex> lock(hook_mutex_);
        if (!hook_installed_.load(std::memory_order_acquire)) return;

        if (hooked_present_addr_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            MH_RemoveHook(hooked_present_addr_);
            hooked_present_addr_ = nullptr;
        }
        
        hook_installed_.store(false, std::memory_order_release);
        instance_ptr_.store(nullptr, std::memory_order_release);
    }

    void normalize_z_orders() {
        std::vector<std::pair<std::string, int>> ordered;
        ordered.reserve(documents_.size());
        
        for (const auto& [id, doc] : documents_) {
            ordered.emplace_back(id, doc->z_order);
        }
        
        std::sort(ordered.begin(), ordered.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        
        int new_z = 0;

        for (const auto& [id, old_z] : ordered) {
            documents_[id]->z_order = new_z++;
        }
    }

    static LRESULT CALLBACK hooked_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        UIFramework* ptr = instance_ptr_.load(std::memory_order_acquire);
        
        if (ptr && !ptr->shutting_down_.load(std::memory_order_acquire)) {
            switch (msg) {
                case WM_MOUSEMOVE: {
                    int x = GET_X_LPARAM(lParam);
                    int y = GET_Y_LPARAM(lParam);
                    ptr->on_mouse_move(x, y);
                    break;
                }
                
                case WM_LBUTTONDOWN:
                    ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 
                                         ultralight::MouseEvent::kButton_Left, true);

                    return 0;
                    break;
                
                case WM_LBUTTONUP:
                    ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                         ultralight::MouseEvent::kButton_Left, false);

                    return 0;
                    break;
                
                case WM_RBUTTONDOWN:
                    ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                         ultralight::MouseEvent::kButton_Right, true);

                    return 0;
                    break;
                
                case WM_RBUTTONUP:
                    ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                         ultralight::MouseEvent::kButton_Right, false);

                    return 0;
                    break;
                
                case WM_MBUTTONDOWN:
                    ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                         ultralight::MouseEvent::kButton_Middle, true);

                    return 0;
                    break;
                
                case WM_MBUTTONUP:
                    ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                         ultralight::MouseEvent::kButton_Middle, false);

                    return 0;
                    break;
                
                case WM_MOUSEWHEEL: {
                    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    ScreenToClient(hwnd, &pt);
                    int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
                    if (ptr->on_mouse_scroll(pt.x, pt.y, delta)) return 0;
                    break;
                }
                
                // Keyboard events - fire to all documents but don't swallow
                case WM_KEYDOWN:
                    ptr->on_key_event(wParam, lParam, true, false);
                    break;
                
                case WM_KEYUP:
                    ptr->on_key_event(wParam, lParam, false, false);
                    break;
                
                case WM_SYSKEYDOWN:
                    ptr->on_key_event(wParam, lParam, true, true);
                    break;
                
                case WM_SYSKEYUP:
                    ptr->on_key_event(wParam, lParam, false, true);
                    break;
                
                case WM_CHAR:
                    ptr->on_char_event(wParam, lParam);
                    break;
                
                case WM_SYSCHAR:
                    ptr->on_char_event(wParam, lParam);
                    break;
            }
        }
        
        WNDPROC orig = original_wndproc_.load(std::memory_order_acquire);

        if (orig) {
            return CallWindowProcW(orig, hwnd, msg, wParam, lParam);
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    bool install_wndproc_hook() {
        if (original_wndproc_.load(std::memory_order_acquire)) return true;
        HWND hwnd = find_game_window();

        if (!hwnd) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Could not find game window for mouse hook");
            });

            return false;
        }

        game_hwnd_.store(hwnd, std::memory_order_release);
        HWND stored_hwnd = hwnd;

        if (!stored_hwnd) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Could not find game window for mouse hook");
            });

            return false;
        }

        WNDPROC prev = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(stored_hwnd, GWLP_WNDPROC));
        original_wndproc_.store(prev, std::memory_order_release);

        WNDPROC actual_prev = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(stored_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hooked_wndproc))
        );

        if (!actual_prev) {
            DWORD err = GetLastError();
            original_wndproc_.store(nullptr, std::memory_order_release);

            MainThreadScheduler::instance().schedule([err]() {
                report_error("KH - UI Framework: Failed to hook WndProc, error: " + std::to_string(err));
            });

            return false;
        }
        
        if (actual_prev != prev) {
            original_wndproc_.store(actual_prev, std::memory_order_release);
        }
        
        return true;
    }

    void uninstall_wndproc_hook() {
        WNDPROC orig = original_wndproc_.load(std::memory_order_acquire);
        HWND hwnd = game_hwnd_.load(std::memory_order_acquire);
        
        if (orig && hwnd && IsWindow(hwnd)) {
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(orig));
            original_wndproc_.store(nullptr, std::memory_order_release);
        }

        game_hwnd_.store(nullptr, std::memory_order_release);
    }

    void on_mouse_move(int x, int y) {
        last_mouse_x_.store(x, std::memory_order_release);
        last_mouse_y_.store(y, std::memory_order_release);
        std::string target_doc;
        int local_x = 0, local_y = 0;
        std::string old_hovered;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            target_doc = find_document_at(x, y, local_x, local_y);
        }
        
        {
            std::lock_guard<std::mutex> lock(mouse_mutex_);
            old_hovered = hovered_doc_id_;
            
            if (target_doc != old_hovered) {
                if (!old_hovered.empty()) {
                    queue_command([this, old_hovered]() {
                        fire_mouse_leave_internal(old_hovered);
                    });
                }

                hovered_doc_id_ = target_doc;
            }
        }
        
        if (!target_doc.empty()) {
            queue_command([this, target_doc, local_x, local_y]() {
                fire_mouse_move_internal(target_doc, local_x, local_y);
            });
        }
    }

    bool on_mouse_button(int x, int y, ultralight::MouseEvent::Button button, bool pressed) {
        std::string target_doc;
        int local_x = 0, local_y = 0;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            target_doc = find_document_at(x, y, local_x, local_y);
        }
        
        if (target_doc.empty()) return false;
        
        ultralight::MouseEvent::Type type = pressed 
            ? ultralight::MouseEvent::kType_MouseDown 
            : ultralight::MouseEvent::kType_MouseUp;
        
        queue_command([this, target_doc, local_x, local_y, type, button]() {
            fire_mouse_button_internal(target_doc, local_x, local_y, type, button);
        });
        
        return true;  // We handled it
    }

    bool on_mouse_scroll(int x, int y, int delta) {
        std::string target_doc;
        int local_x = 0, local_y = 0;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            target_doc = find_document_at(x, y, local_x, local_y);
        }
        
        if (target_doc.empty()) return false;
        
        queue_command([this, target_doc, local_x, local_y, delta]() {
            fire_scroll_internal(target_doc, local_x, local_y, delta);
        });
        
        return true;
    }

    // Find topmost interactive document at screen coordinates
    // Must be called with documents_mutex_ held
    std::string find_document_at(int screen_x, int screen_y, int& local_x, int& local_y) {
        if (!mouse_enabled_.load(std::memory_order_acquire)) {
            return "";  // Mouse input disabled
        }
        
        std::string result;
        int highest_z = INT_MIN;
        
        for (const auto& [id, doc] : documents_) {
            if (!doc || !doc->visible || !doc->interactive) continue;
            if (!doc->view) continue;
            
            if (screen_x >= doc->x && screen_x < doc->x + doc->width &&
                screen_y >= doc->y && screen_y < doc->y + doc->height) {
                
                if (doc->z_order > highest_z) {
                    highest_z = doc->z_order;
                    result = id;
                    local_x = screen_x - doc->x;
                    local_y = screen_y - doc->y;
                }
            }
        }
        
        return result;
    }

    void fire_mouse_move_internal(const std::string& doc_id, int x, int y) {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        std::shared_ptr<UIDocument> doc;

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return;
            doc = it->second;
        }
        
        if (!doc || !doc->view) return;
        ultralight::MouseEvent evt;
        evt.type = ultralight::MouseEvent::kType_MouseMoved;
        evt.x = x;
        evt.y = y;
        evt.button = ultralight::MouseEvent::kButton_None;
        doc->view->FireMouseEvent(evt);
    }

    void fire_mouse_leave_internal(const std::string& doc_id) {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        std::shared_ptr<UIDocument> doc;

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return;
            doc = it->second;
        }
        
        if (!doc || !doc->view) return;
        ultralight::MouseEvent evt;
        evt.type = ultralight::MouseEvent::kType_MouseMoved;
        evt.x = -1;
        evt.y = -1;
        evt.button = ultralight::MouseEvent::kButton_None;
        doc->view->FireMouseEvent(evt);
    }

    void fire_mouse_button_internal(const std::string& doc_id, int x, int y,
                                     ultralight::MouseEvent::Type type,
                                     ultralight::MouseEvent::Button button) {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        std::shared_ptr<UIDocument> doc;

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return;
            doc = it->second;
        }
        
        if (!doc || !doc->view) return;
        ultralight::MouseEvent evt;
        evt.type = type;
        evt.x = x;
        evt.y = y;
        evt.button = button;
        doc->view->FireMouseEvent(evt);
    }

    void fire_scroll_internal(const std::string& doc_id, int x, int y, int delta) {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        std::shared_ptr<UIDocument> doc;

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return;
            doc = it->second;
        }
        
        if (!doc || !doc->view) return;
        ultralight::ScrollEvent evt;
        evt.type = ultralight::ScrollEvent::kType_ScrollByPixel;
        evt.delta_x = 0;
        evt.delta_y = delta * 40;
        doc->view->FireScrollEvent(evt);
    }

    static int windows_vk_to_ultralight_keycode(WPARAM vk) {
        // Ultralight uses the same values as Windows for most keys
        switch (vk) {
            case VK_BACK:       return 0x08;  // Backspace
            case VK_TAB:        return 0x09;  // Tab
            case VK_CLEAR:      return 0x0C;  // Clear
            case VK_RETURN:     return 0x0D;  // Enter
            case VK_SHIFT:      return 0x10;  // Shift
            case VK_CONTROL:    return 0x11;  // Ctrl
            case VK_MENU:       return 0x12;  // Alt
            case VK_PAUSE:      return 0x13;  // Pause
            case VK_CAPITAL:    return 0x14;  // Caps Lock
            case VK_ESCAPE:     return 0x1B;  // Escape
            case VK_SPACE:      return 0x20;  // Space
            case VK_PRIOR:      return 0x21;  // Page Up
            case VK_NEXT:       return 0x22;  // Page Down
            case VK_END:        return 0x23;  // End
            case VK_HOME:       return 0x24;  // Home
            case VK_LEFT:       return 0x25;  // Left Arrow
            case VK_UP:         return 0x26;  // Up Arrow
            case VK_RIGHT:      return 0x27;  // Right Arrow
            case VK_DOWN:       return 0x28;  // Down Arrow
            case VK_SELECT:     return 0x29;  // Select
            case VK_PRINT:      return 0x2A;  // Print
            case VK_EXECUTE:    return 0x2B;  // Execute
            case VK_SNAPSHOT:   return 0x2C;  // Print Screen
            case VK_INSERT:     return 0x2D;  // Insert
            case VK_DELETE:     return 0x2E;  // Delete
            case VK_HELP:       return 0x2F;  // Help
            // 0-9 keys (0x30-0x39) - same as Windows
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                return static_cast<int>(vk);
            // A-Z keys (0x41-0x5A) - same as Windows
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
            case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
            case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
            case 'Y': case 'Z':
                return static_cast<int>(vk);
            case VK_LWIN:       return 0x5B;  // Left Windows
            case VK_RWIN:       return 0x5C;  // Right Windows
            case VK_APPS:       return 0x5D;  // Applications
            case VK_SLEEP:      return 0x5F;  // Sleep
            // Numpad keys
            case VK_NUMPAD0:    return 0x60;
            case VK_NUMPAD1:    return 0x61;
            case VK_NUMPAD2:    return 0x62;
            case VK_NUMPAD3:    return 0x63;
            case VK_NUMPAD4:    return 0x64;
            case VK_NUMPAD5:    return 0x65;
            case VK_NUMPAD6:    return 0x66;
            case VK_NUMPAD7:    return 0x67;
            case VK_NUMPAD8:    return 0x68;
            case VK_NUMPAD9:    return 0x69;
            case VK_MULTIPLY:   return 0x6A;
            case VK_ADD:        return 0x6B;
            case VK_SEPARATOR:  return 0x6C;
            case VK_SUBTRACT:   return 0x6D;
            case VK_DECIMAL:    return 0x6E;
            case VK_DIVIDE:     return 0x6F;
            // Function keys
            case VK_F1:         return 0x70;
            case VK_F2:         return 0x71;
            case VK_F3:         return 0x72;
            case VK_F4:         return 0x73;
            case VK_F5:         return 0x74;
            case VK_F6:         return 0x75;
            case VK_F7:         return 0x76;
            case VK_F8:         return 0x77;
            case VK_F9:         return 0x78;
            case VK_F10:        return 0x79;
            case VK_F11:        return 0x7A;
            case VK_F12:        return 0x7B;
            case VK_F13:        return 0x7C;
            case VK_F14:        return 0x7D;
            case VK_F15:        return 0x7E;
            case VK_F16:        return 0x7F;
            case VK_F17:        return 0x80;
            case VK_F18:        return 0x81;
            case VK_F19:        return 0x82;
            case VK_F20:        return 0x83;
            case VK_F21:        return 0x84;
            case VK_F22:        return 0x85;
            case VK_F23:        return 0x86;
            case VK_F24:        return 0x87;
            case VK_NUMLOCK:    return 0x90;
            case VK_SCROLL:     return 0x91;
            case VK_LSHIFT:     return 0xA0;
            case VK_RSHIFT:     return 0xA1;
            case VK_LCONTROL:   return 0xA2;
            case VK_RCONTROL:   return 0xA3;
            case VK_LMENU:      return 0xA4;
            case VK_RMENU:      return 0xA5;
            // OEM keys
            case VK_OEM_1:      return 0xBA;  // ;:
            case VK_OEM_PLUS:   return 0xBB;  // =+
            case VK_OEM_COMMA:  return 0xBC;  // ,
            case VK_OEM_MINUS:  return 0xBD;  // -_
            case VK_OEM_PERIOD: return 0xBE;  // .>
            case VK_OEM_2:      return 0xBF;  // /?
            case VK_OEM_3:      return 0xC0;  // `~
            case VK_OEM_4:      return 0xDB;  // [{
            case VK_OEM_5:      return 0xDC;  // \|
            case VK_OEM_6:      return 0xDD;  // ]}
            case VK_OEM_7:      return 0xDE;  // '"
            case VK_OEM_8:      return 0xDF;
            case VK_OEM_102:    return 0xE2;  // <> or \| on RT 102-key
            default:
                return static_cast<int>(vk);
        }
    }

    static unsigned get_keyboard_modifiers() {
        unsigned modifiers = 0;

        if (GetKeyState(VK_MENU) & 0x8000)     modifiers |= ultralight::KeyEvent::kMod_AltKey;
        if (GetKeyState(VK_CONTROL) & 0x8000)  modifiers |= ultralight::KeyEvent::kMod_CtrlKey;
        if (GetKeyState(VK_SHIFT) & 0x8000)    modifiers |= ultralight::KeyEvent::kMod_ShiftKey;
        if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) 
            modifiers |= ultralight::KeyEvent::kMod_MetaKey;

        return modifiers;
    }

    void on_key_event(WPARAM vk, LPARAM lParam, bool is_down, bool is_system_key) {
        int ul_keycode = windows_vk_to_ultralight_keycode(vk);
        unsigned modifiers = get_keyboard_modifiers();
        
        // Determine if this is a keypad key
        bool is_keypad = false;
        bool is_extended = (lParam >> 24) & 1;

        if (!is_extended) {
            switch (vk) {
                case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
                case VK_PRIOR: case VK_NEXT: case VK_LEFT: case VK_RIGHT:
                case VK_UP: case VK_DOWN:
                    is_keypad = true;
                    break;
            }
        }
        
        if (vk >= VK_NUMPAD0 && vk <= VK_DIVIDE) {
            is_keypad = true;
        }
        
        queue_command([this, ul_keycode, vk, modifiers, is_down, is_keypad]() {
            fire_key_event_to_all_documents(ul_keycode, static_cast<int>(vk), modifiers, is_down, is_keypad);
        });
    }

    void on_char_event(WPARAM charCode, LPARAM lParam) {
        if (charCode < 32 && charCode != '\r' && charCode != '\t') return;  // Skip control chars except enter/tab
        unsigned modifiers = get_keyboard_modifiers();
        wchar_t wch = static_cast<wchar_t>(charCode);
        char utf8[8] = {0};
        WideCharToMultiByte(CP_UTF8, 0, &wch, 1, utf8, sizeof(utf8), nullptr, nullptr);
        std::string text(utf8);
        
        queue_command([this, text, modifiers]() {
            fire_char_event_to_all_documents(text, modifiers);
        });
    }

    void fire_key_event_to_all_documents(int ul_keycode, int native_keycode, unsigned modifiers, bool is_down, bool is_keypad) {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        std::vector<std::shared_ptr<UIDocument>> docs_to_notify;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            
            for (const auto& [id, doc] : documents_) {
                if (doc && doc->view && doc->visible) {
                    docs_to_notify.push_back(doc);
                }
            }
        }
        
        for (const auto& doc : docs_to_notify) {
            if (!doc->view) continue;
            ultralight::KeyEvent evt;
            evt.type = is_down ? ultralight::KeyEvent::kType_RawKeyDown : ultralight::KeyEvent::kType_KeyUp;
            evt.virtual_key_code = ul_keycode;
            evt.native_key_code = native_keycode;
            evt.modifiers = modifiers;
            evt.is_auto_repeat = false;
            evt.is_keypad = is_keypad;
            evt.is_system_key = false;
            GetKeyIdentifierFromVirtualKeyCode(native_keycode, evt.key_identifier);
            doc->view->FireKeyEvent(evt);
        }
    }

    void fire_char_event_to_all_documents(const std::string& text, unsigned modifiers) {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        std::vector<std::shared_ptr<UIDocument>> docs_to_notify;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);

            for (const auto& [id, doc] : documents_) {
                if (doc && doc->view && doc->visible) {
                    docs_to_notify.push_back(doc);
                }
            }
        }
        
        for (const auto& doc : docs_to_notify) {
            if (!doc->view) continue;
            ultralight::KeyEvent evt;
            evt.type = ultralight::KeyEvent::kType_Char;
            evt.text = ultralight::String(text.c_str());
            evt.unmodified_text = ultralight::String(text.c_str());
            evt.modifiers = modifiers;
            evt.is_auto_repeat = false;
            evt.is_keypad = false;
            evt.is_system_key = false;
            doc->view->FireKeyEvent(evt);
        }
    }

    static void GetKeyIdentifierFromVirtualKeyCode(int virtual_key_code, ultralight::String& key_identifier) {
        // Map virtual key codes to DOM key identifiers
        switch (virtual_key_code) {
            case VK_MENU:    case VK_LMENU:   case VK_RMENU:    key_identifier = "Alt"; break;
            case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: key_identifier = "Control"; break;
            case VK_SHIFT:   case VK_LSHIFT:  case VK_RSHIFT:   key_identifier = "Shift"; break;
            case VK_LWIN:    case VK_RWIN:                       key_identifier = "Meta"; break;
            case VK_CAPITAL:  key_identifier = "CapsLock"; break;
            case VK_NUMLOCK:  key_identifier = "NumLock"; break;
            case VK_SCROLL:   key_identifier = "ScrollLock"; break;
            case VK_BACK:     key_identifier = "Backspace"; break;
            case VK_TAB:      key_identifier = "Tab"; break;
            case VK_RETURN:   key_identifier = "Enter"; break;
            case VK_ESCAPE:   key_identifier = "Escape"; break;
            case VK_SPACE:    key_identifier = " "; break;
            case VK_PRIOR:    key_identifier = "PageUp"; break;
            case VK_NEXT:     key_identifier = "PageDown"; break;
            case VK_END:      key_identifier = "End"; break;
            case VK_HOME:     key_identifier = "Home"; break;
            case VK_LEFT:     key_identifier = "ArrowLeft"; break;
            case VK_UP:       key_identifier = "ArrowUp"; break;
            case VK_RIGHT:    key_identifier = "ArrowRight"; break;
            case VK_DOWN:     key_identifier = "ArrowDown"; break;
            case VK_DELETE:   key_identifier = "Delete"; break;
            case VK_INSERT:   key_identifier = "Insert"; break;
            case VK_F1:       key_identifier = "F1"; break;
            case VK_F2:       key_identifier = "F2"; break;
            case VK_F3:       key_identifier = "F3"; break;
            case VK_F4:       key_identifier = "F4"; break;
            case VK_F5:       key_identifier = "F5"; break;
            case VK_F6:       key_identifier = "F6"; break;
            case VK_F7:       key_identifier = "F7"; break;
            case VK_F8:       key_identifier = "F8"; break;
            case VK_F9:       key_identifier = "F9"; break;
            case VK_F10:      key_identifier = "F10"; break;
            case VK_F11:      key_identifier = "F11"; break;
            case VK_F12:      key_identifier = "F12"; break;
            case VK_PAUSE:    key_identifier = "Pause"; break;
            default: {
                // For letter and number keys, use the character itself
                if ((virtual_key_code >= '0' && virtual_key_code <= '9') ||
                    (virtual_key_code >= 'A' && virtual_key_code <= 'Z')) {
                    char buf[2] = { static_cast<char>(virtual_key_code), 0 };
                    key_identifier = buf;
                } else {
                    key_identifier = "Unidentified";
                }

                break;
            }
        }
    }
};

std::atomic<UIFramework*> UIFramework::instance_ptr_{nullptr};
UIFramework::PresentFn UIFramework::original_present_ = nullptr;
void* UIFramework::hooked_present_addr_ = nullptr;
std::atomic<bool> UIFramework::hook_installed_{false};
std::atomic<bool> UIFramework::minhook_initialized_{false};
std::mutex UIFramework::hook_mutex_;
std::atomic<WNDPROC> UIFramework::original_wndproc_{nullptr};
std::atomic<HWND> UIFramework::game_hwnd_{nullptr};