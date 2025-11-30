#pragma once

using Microsoft::WRL::ComPtr;

class DirectWriteFontLoader : public ultralight::FontLoader {
public:
    DirectWriteFontLoader() {
        HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                            reinterpret_cast<IUnknown**>(&dwrite_factory_));
        if (FAILED(hr)) {
            dwrite_factory_ = nullptr;
        }
    }

    ~DirectWriteFontLoader() {
        if (dwrite_factory_) dwrite_factory_->Release();
    }

    ultralight::String fallback_font() const override {
        return "Segoe UI";
    }

    ultralight::String fallback_font_for_characters(const ultralight::String& characters,
                                             int weight,
                                             bool italic) const override {
        return "Segoe UI";
    }

    ultralight::RefPtr<ultralight::FontFile> Load(const ultralight::String& family,
                                   int weight,
                                   bool italic) override {
        if (!dwrite_factory_) return nullptr;
        std::string family_str(family.utf8().data());
        int wide_len = MultiByteToWideChar(CP_UTF8, 0, family_str.c_str(), -1, nullptr, 0);
        if (wide_len <= 0) return nullptr;
        std::wstring family_wide(wide_len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, family_str.c_str(), -1, &family_wide[0], wide_len);
        family_wide.resize(wide_len - 1);
        ComPtr<IDWriteFontCollection> font_collection;

        if (FAILED(dwrite_factory_->GetSystemFontCollection(&font_collection, FALSE)))
            return nullptr;

        UINT32 family_index = 0;
        BOOL exists = FALSE;
        font_collection->FindFamilyName(family_wide.c_str(), &family_index, &exists);

        if (!exists) {
            font_collection->FindFamilyName(L"Segoe UI", &family_index, &exists);
            if (!exists) return nullptr;
        }

        ComPtr<IDWriteFontFamily> font_family;
        
        if (FAILED(font_collection->GetFontFamily(family_index, &font_family)))
            return nullptr;

        DWRITE_FONT_WEIGHT dw_weight = static_cast<DWRITE_FONT_WEIGHT>(weight);
        DWRITE_FONT_STYLE dw_style = italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
        ComPtr<IDWriteFont> font;

        if (FAILED(font_family->GetFirstMatchingFont(dw_weight, DWRITE_FONT_STRETCH_NORMAL, dw_style, &font)))
            return nullptr;

        ComPtr<IDWriteFontFace> font_face;

        if (FAILED(font->CreateFontFace(&font_face)))
            return nullptr;

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
        
        auto buffer = ultralight::Buffer::CreateFromCopy(font_data, static_cast<size_t>(file_size));
        stream->ReleaseFileFragment(context);
        return ultralight::FontFile::Create(buffer);
    }

private:
    IDWriteFactory* dwrite_factory_ = nullptr;
};

class UIFileSystem : public ultralight::FileSystem {
public:
    UIFileSystem(const std::vector<std::filesystem::path>& search_paths,
                   const std::filesystem::path& resources_path)
        : search_paths_(search_paths), resources_path_(resources_path) {}

    bool FileExists(const ultralight::String& path) override {
        std::string path_str = path.utf8().data();
        if (path_str.find("file:///") == 0) path_str = path_str.substr(8);
        
        if (!resources_path_.empty()) {
            std::filesystem::path full = resources_path_ / path_str;
            if (std::filesystem::exists(full)) return true;
        }
        
        for (const auto& base : search_paths_) {
            if (std::filesystem::exists(base / path_str)) return true;
        }

        return std::filesystem::exists(path_str);
    }

    ultralight::RefPtr<ultralight::Buffer> OpenFile(const ultralight::String& path) override {
        std::string path_str = path.utf8().data();
        if (path_str.find("file:///") == 0) path_str = path_str.substr(8);
        std::filesystem::path file_path;
        
        if (!resources_path_.empty()) {
            std::filesystem::path full = resources_path_ / path_str;
            if (std::filesystem::exists(full)) file_path = full;
        }
        
        if (file_path.empty()) {
            for (const auto& base : search_paths_) {
                std::filesystem::path full = base / path_str;
                if (std::filesystem::exists(full)) { file_path = full; break; }
            }
        }
        
        if (file_path.empty() && std::filesystem::exists(path_str)) file_path = path_str;
        if (file_path.empty()) return nullptr;
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file) return nullptr;
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> data(size);
        file.read(data.data(), size);
        return ultralight::Buffer::CreateFromCopy(data.data(), size);
    }

    ultralight::String GetFileMimeType(const ultralight::String& path) override {
        std::string ext = std::filesystem::path(std::string(path.utf8().data())).extension().string();
        if (ext == ".html" || ext == ".htm") return "text/html";
        if (ext == ".css") return "text/css";
        if (ext == ".js") return "application/javascript";
        if (ext == ".png") return "image/png";
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".svg") return "image/svg+xml";
        return "application/octet-stream";
    }

    ultralight::String GetFileCharset(const ultralight::String& path) override {
        return "utf-8";
    }

private:
    std::vector<std::filesystem::path> search_paths_;
    std::filesystem::path resources_path_;
};

struct UIDocument {
    std::string id;
    std::string html_path;
    ultralight::RefPtr<ultralight::View> view;
    bool visible = true;
    bool fullscreen = false;
    int x = 0, y = 0, width = 0, height = 0;
    float opacity = 1.0f;
    int z_order = 0;
    
    // Cached pixels for transfer between threads
    std::vector<uint8_t> cached_pixels;
    int cached_width = 0;
    int cached_height = 0;
    uint32_t cached_stride = 0;
    std::atomic<bool> pixels_ready{false};
    mutable std::mutex pixel_mutex;
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
    ComPtr<ID3D11Texture2D> overlay_texture_;
    ComPtr<ID3D11ShaderResourceView> overlay_srv_;
    int texture_width_ = 0, texture_height_ = 0;
    std::atomic<bool> initialized_{false};
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
        overlay_srv_.Reset();
        overlay_texture_.Reset();
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
        texture_width_ = 0;
        texture_height_ = 0;
        initialized_.store(false, std::memory_order_release);
    }

    bool update_texture(const void* pixels, int width, int height, uint32_t stride) {
        if (!initialized_.load(std::memory_order_acquire)) return false;
        if (!device_ || !pixels || width <= 0 || height <= 0) return false;

        if (texture_width_ != width || texture_height_ != height) {
            overlay_texture_.Reset(); overlay_srv_.Reset();
            D3D11_TEXTURE2D_DESC td = {(UINT)width, (UINT)height, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, {1,0}, D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE, 0};
            if (FAILED(device_->CreateTexture2D(&td, 0, &overlay_texture_))) return false;
            if (FAILED(device_->CreateShaderResourceView(overlay_texture_.Get(), 0, &overlay_srv_))) return false;
            texture_width_ = width; texture_height_ = height;
        }

        D3D11_MAPPED_SUBRESOURCE mapped;

        if (SUCCEEDED(context_->Map(overlay_texture_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            for (int y = 0; y < height; y++)
                memcpy((BYTE*)mapped.pData + y * mapped.RowPitch, (const BYTE*)pixels + y * stride, width * 4);
            context_->Unmap(overlay_texture_.Get(), 0);
        }

        return true;
    }

    void render(int sw, int sh, int dx, int dy, int dw, int dh, float opacity) {
        if (!initialized_.load(std::memory_order_acquire) || !overlay_srv_ || !context_) return;
        ComPtr<ID3D11BlendState> old_blend; FLOAT old_bf[4]; UINT old_sm;
        context_->OMGetBlendState(&old_blend, old_bf, &old_sm);
        ComPtr<ID3D11DepthStencilState> old_ds; UINT old_sr;
        context_->OMGetDepthStencilState(&old_ds, &old_sr);
        ComPtr<ID3D11RasterizerState> old_rs; context_->RSGetState(&old_rs);
        ComPtr<ID3D11RenderTargetView> old_rtv; ComPtr<ID3D11DepthStencilView> old_dsv;
        context_->OMGetRenderTargets(1, &old_rtv, &old_dsv);
        D3D11_VIEWPORT old_vp[16]; UINT nvp = 16; context_->RSGetViewports(&nvp, old_vp);
        D3D11_RECT old_scissor[16]; UINT nsr = 16; context_->RSGetScissorRects(&nsr, old_scissor);
        ComPtr<ID3D11VertexShader> old_vs; context_->VSGetShader(&old_vs, 0, 0);
        ComPtr<ID3D11PixelShader> old_ps; context_->PSGetShader(&old_ps, 0, 0);
        ComPtr<ID3D11GeometryShader> old_gs; context_->GSGetShader(&old_gs, 0, 0);
        ComPtr<ID3D11InputLayout> old_il; context_->IAGetInputLayout(&old_il);
        D3D11_PRIMITIVE_TOPOLOGY old_pt; context_->IAGetPrimitiveTopology(&old_pt);
        ComPtr<ID3D11Buffer> old_vb; UINT old_st, old_of; context_->IAGetVertexBuffers(0, 1, &old_vb, &old_st, &old_of);
        ComPtr<ID3D11Buffer> old_ib; DXGI_FORMAT old_ib_fmt; UINT old_ib_off;
        context_->IAGetIndexBuffer(&old_ib, &old_ib_fmt, &old_ib_off);
        ComPtr<ID3D11Buffer> old_cb_vs; context_->VSGetConstantBuffers(0, 1, &old_cb_vs);
        ComPtr<ID3D11Buffer> old_cb_ps; context_->PSGetConstantBuffers(0, 1, &old_cb_ps);
        ComPtr<ID3D11ShaderResourceView> old_srv; context_->PSGetShaderResources(0, 1, &old_srv);
        ComPtr<ID3D11SamplerState> old_samp; context_->PSGetSamplers(0, 1, &old_samp);
        D3D11_MAPPED_SUBRESOURCE m;

        if (SUCCEEDED(context_->Map(constant_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
            ConstantBuffer* cb = (ConstantBuffer*)m.pData;
            cb->screen_width = (float)sw; cb->screen_height = (float)sh;
            cb->offset_x = (float)dx; cb->offset_y = (float)dy;
            cb->doc_width = (float)dw; cb->doc_height = (float)dh;
            cb->opacity = opacity;
            context_->Unmap(constant_buffer_.Get(), 0);
        }

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
        context_->PSSetShaderResources(0, 1, overlay_srv_.GetAddressOf());
        context_->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());
        context_->Draw(4, 0);
        ID3D11ShaderResourceView* null_srv = nullptr;
        context_->PSSetShaderResources(0, 1, &null_srv);
        context_->OMSetRenderTargets(1, old_rtv.GetAddressOf(), old_dsv.Get());
        context_->OMSetBlendState(old_blend.Get(), old_bf, old_sm);
        context_->OMSetDepthStencilState(old_ds.Get(), old_sr);
        context_->RSSetState(old_rs.Get());
        context_->RSSetViewports(nvp, old_vp);
        if (nsr > 0) context_->RSSetScissorRects(nsr, old_scissor);
        context_->VSSetShader(old_vs.Get(), 0, 0);
        context_->PSSetShader(old_ps.Get(), 0, 0);
        context_->GSSetShader(old_gs.Get(), 0, 0);
        context_->IASetInputLayout(old_il.Get());
        context_->IASetPrimitiveTopology(old_pt);
        context_->IASetVertexBuffers(0, 1, old_vb.GetAddressOf(), &old_st, &old_of);
        context_->IASetIndexBuffer(old_ib.Get(), old_ib_fmt, old_ib_off);
        context_->VSSetConstantBuffers(0, 1, old_cb_vs.GetAddressOf());
        context_->PSSetConstantBuffers(0, 1, old_cb_ps.GetAddressOf());
        context_->PSSetShaderResources(0, 1, old_srv.GetAddressOf());
        context_->PSSetSamplers(0, 1, old_samp.GetAddressOf());
    }

    bool is_initialized() const { return initialized_.load(std::memory_order_acquire); }
};

class UIFramework {
public:
    static UIFramework& instance() {
        static UIFramework inst;
        return inst;
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(init_mutex_);
        if (initialized_.load(std::memory_order_acquire)) return true;

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

            // Set up FileSystem FIRST - Ultralight needs it to find icudt67l.dat
            file_system_ = std::make_unique<UIFileSystem>(html_dirs_, resources_dir);
            ultralight::Platform::instance().set_file_system(file_system_.get());

            // Set up FontLoader
            font_loader_ = std::make_unique<DirectWriteFontLoader>();
            ultralight::Platform::instance().set_font_loader(font_loader_.get());

            // Set config
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

                return false;
            }

            initialized_.store(true, std::memory_order_release);
            install_present_hook();
            return true;
        } catch (const std::exception& e) {
            std::string error_msg = e.what();

            MainThreadScheduler::instance().schedule([error_msg]() {
                report_error("KH - UI Framework: init failed: " + error_msg);
            });

            return false;
        } catch (...) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: init failed: Unknown exception");
            });

            return false;
        }
    }

    void emergency_shutdown() {
        // Minimal shutdown for process termination - just set flags, don't clean up
        shutting_down_.store(true, std::memory_order_seq_cst);
        initialized_.store(false, std::memory_order_seq_cst);
        instance_ptr_.store(nullptr, std::memory_order_seq_cst);
        
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
        shutting_down_.store(true, std::memory_order_seq_cst);

        {
            std::lock_guard<std::mutex> lock(hook_mutex_);
            if (hook_installed_.load(std::memory_order_acquire) && hooked_present_addr_) {
                MH_DisableHook(hooked_present_addr_);
            }
        }
        
        auto start = std::chrono::steady_clock::now();
        constexpr auto timeout = std::chrono::milliseconds(1000);

        while (hook_executing_.load(std::memory_order_seq_cst) && 
               std::chrono::steady_clock::now() - start < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            std::this_thread::yield();
        }

        if (!initialized_.load(std::memory_order_acquire)) {
            shutting_down_.store(false, std::memory_order_release);
            return;
        }
        
        initialized_.store(false, std::memory_order_seq_cst);

        try {
            uninstall_present_hook_internal();
        } catch (...) {}

        std::lock_guard<std::mutex> lock(init_mutex_);

        {
            std::lock_guard<std::mutex> doc_lock(documents_mutex_);

            for (auto& [id, doc] : documents_) {
                if (doc) {
                    doc->view = nullptr;
                    std::lock_guard<std::mutex> pixel_lock(doc->pixel_mutex);
                    doc->cached_pixels.clear();
                    doc->pixels_ready.store(false, std::memory_order_release);
                }
            }

            documents_.clear();
        }

        try {
            d3d_renderer_.cleanup();
        } catch (...) {}
        
        d3d_initialized_.store(false, std::memory_order_release); 
        renderer_ = nullptr;
        file_system_.reset();
        font_loader_.reset();
        shutting_down_.store(false, std::memory_order_release);
    }

    std::string open_html(const std::string& filename, int x, int y, int width, int height, float opacity = 1.0f) {
        if (!initialized_.load(std::memory_order_acquire) && !initialize()) return "";
        auto html_path = find_html_file(filename);

        if (html_path.empty()) {
            MainThreadScheduler::instance().schedule([filename]() {
                report_error("KH - UI Framework: HTML not found: " + filename);
            });
            
            return "";
        }

        ultralight::RefPtr<ultralight::Renderer> renderer_copy;

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            renderer_copy = renderer_;
        }
        
        if (!renderer_copy) return "";
        int w = width, h = height;
        bool fs = (width <= 0 || height <= 0);
        if (fs) { w = GetSystemMetrics(SM_CXSCREEN); h = GetSystemMetrics(SM_CYSCREEN); }
        ultralight::ViewConfig vc;
        vc.is_accelerated = false;
        vc.is_transparent = true;
        auto view = renderer_copy->CreateView(w, h, vc, nullptr);

        if (!view) {
            MainThreadScheduler::instance().schedule([]() {
                report_error("KH - UI Framework: Failed to create view");
            });

            return "";
        }
        
        view->Focus();
        auto doc = std::make_shared<UIDocument>();
        doc->id = UIDGenerator::generate();
        doc->html_path = html_path.string();
        doc->view = view;
        doc->x = x; doc->y = y;
        doc->width = w; doc->height = h;
        doc->opacity = std::clamp(opacity, 0.0f, 1.0f);
        doc->fullscreen = fs;
        std::string url = "file:///" + html_path.string();
        std::replace(url.begin(), url.end(), '\\', '/');
        view->LoadURL(ultralight::String(url.c_str()));

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            doc->z_order = static_cast<int>(documents_.size());
            documents_[doc->id] = doc;
        }
        
        return doc->id;
    }

    bool close_html(const std::string& doc_id) {
        std::shared_ptr<UIDocument> doc_to_close;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return false;
            doc_to_close = it->second;
            documents_.erase(it);
        }
        
        if (doc_to_close) {
            doc_to_close->view = nullptr;
            std::lock_guard<std::mutex> pixel_lock(doc_to_close->pixel_mutex);
            doc_to_close->cached_pixels.clear();
            doc_to_close->pixels_ready.store(false, std::memory_order_release);
        }
        
        return true;
    }

    bool set_html_position(const std::string& doc_id, int x, int y) {
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        it->second->x = x;
        it->second->y = y;
        return true;
    }

    bool set_html_opacity(const std::string& doc_id, float opacity) {
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        it->second->opacity = std::clamp(opacity, 0.0f, 1.0f);
        return true;
    }

    bool set_html_size(const std::string& doc_id, int width, int height) {
        if (width <= 0 || height <= 0) return false;
        ultralight::RefPtr<ultralight::Renderer> renderer_copy;
        std::shared_ptr<UIDocument> doc;
        std::string html_path_copy;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            renderer_copy = renderer_;
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return false;
            doc = it->second;
            html_path_copy = doc->html_path;
        }
        
        if (!renderer_copy || !doc) return false;
        ultralight::ViewConfig vc;
        vc.is_accelerated = false;
        vc.is_transparent = true;
        auto new_view = renderer_copy->CreateView(width, height, vc, nullptr);
        if (!new_view) return false;
        std::string url = "file:///" + html_path_copy;
        std::replace(url.begin(), url.end(), '\\', '/');
        new_view->LoadURL(ultralight::String(url.c_str()));
        new_view->Focus();
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);

            if (it == documents_.end() || it->second != doc) {
                return false;
            }

            doc->view = new_view;
            doc->width = width;
            doc->height = height;
            doc->fullscreen = false;
            std::lock_guard<std::mutex> pixel_lock(doc->pixel_mutex);
            doc->pixels_ready.store(false, std::memory_order_release);
            doc->cached_pixels.clear();
        }
        
        return true;
    }

    bool set_html_z_order(const std::string& doc_id, int z_order) {
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        it->second->z_order = z_order;
        return true;
    }

    bool bring_html_to_front(const std::string& doc_id) {
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        int max_z = 0;

        for (const auto& [id, doc] : documents_) {
            if (doc->z_order > max_z) max_z = doc->z_order;
        }

        it->second->z_order = max_z + 1;
        return true;
    }

    bool send_html_to_back(const std::string& doc_id) {
        std::lock_guard<std::mutex> lock(documents_mutex_);
        auto it = documents_.find(doc_id);
        if (it == documents_.end()) return false;
        int min_z = 0;

        for (const auto& [id, doc] : documents_) {
            if (doc->z_order < min_z) min_z = doc->z_order;
        }

        it->second->z_order = min_z - 1;
        return true;
    }

    std::string reload_html(const std::string& doc_id) {
        if (shutting_down_.load(std::memory_order_acquire)) return "";
        std::shared_ptr<UIDocument> doc;
        std::string html_path_copy;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return "";
            doc = it->second;
            if (!doc || !doc->view) return "";
            html_path_copy = doc->html_path;
        }
        
        std::string url = "file:///" + html_path_copy;
        std::replace(url.begin(), url.end(), '\\', '/');
        doc->view->LoadURL(ultralight::String(url.c_str()));
        
        {
            std::lock_guard<std::mutex> pixel_lock(doc->pixel_mutex);
            doc->pixels_ready.store(false, std::memory_order_release);
        }
        
        return doc_id;
    }

    std::string execute_javascript(const std::string& doc_id, const std::string& script) {
        if (!initialized_.load(std::memory_order_acquire)) return "";
        if (shutting_down_.load(std::memory_order_acquire)) return "";
        std::shared_ptr<UIDocument> doc;
        ultralight::RefPtr<ultralight::Renderer> renderer_copy;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            if (shutting_down_.load(std::memory_order_acquire)) return "";
            auto it = documents_.find(doc_id);
            if (it == documents_.end()) return "";
            doc = it->second;
            renderer_copy = renderer_;
        }
        
        if (!renderer_copy || !doc || !doc->view) return "";
        
        // Execute JS with our local copy of the renderer reference
        renderer_copy->Update();
        auto result = doc->view->EvaluateScript(ultralight::String(script.c_str()));
        return std::string(result.utf8().data());
    }

    bool set_html_visible(const std::string& doc_id, bool visible) {
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

    // Called from MAIN THREAD via on_frame()
    // Updates Ultralight and caches pixels for render thread
    void update() {
        if (!initialized_.load(std::memory_order_acquire)) return;
        if (shutting_down_.load(std::memory_order_acquire)) return;
        ultralight::RefPtr<ultralight::Renderer> renderer_copy;

        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            renderer_copy = renderer_;
        }
        
        if (!renderer_copy) return;
        
        // Update Ultralight - advances animations and JS timers
        renderer_copy->Update();
        renderer_copy->RefreshDisplay(0);
        renderer_copy->Render();

        // Cache pixels for each document
        std::lock_guard<std::mutex> lock(documents_mutex_);
        
        for (auto& [id, doc] : documents_) {
            if (!doc || !doc->visible || !doc->view) continue;
            auto* surface = doc->view->surface();
            if (!surface) continue;
            auto* bmp_surface = static_cast<ultralight::BitmapSurface*>(surface);
            auto bitmap = bmp_surface->bitmap();
            if (!bitmap) continue;
            void* pixels = bitmap->LockPixels();

            if (pixels) {
                struct UnlockGuard {
                    ultralight::RefPtr<ultralight::Bitmap>& bmp;
                    ~UnlockGuard() { bmp->UnlockPixels(); }
                } unlock_guard{bitmap};
                
                int w = bitmap->width();
                int h = bitmap->height();
                uint32_t stride = bitmap->row_bytes();
                size_t size = static_cast<size_t>(h) * stride;
                
                if (doc->cached_pixels.size() != size) {
                    doc->cached_pixels.resize(size);
                }

                memcpy(doc->cached_pixels.data(), pixels, size);
                doc->cached_width = w;
                doc->cached_height = h;
                doc->cached_stride = stride;
                doc->pixels_ready.store(true, std::memory_order_release);
            }
        }
    }

    // Called from RENDER THREAD via Present hook
    // Uploads cached pixels to D3D and renders
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

        // Get screen dimensions from swap chain
        ComPtr<ID3D11Texture2D> back_buffer;
        
        if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
            D3D11_TEXTURE2D_DESC desc;
            back_buffer->GetDesc(&desc);
            screen_width_.store(desc.Width, std::memory_order_relaxed);
            screen_height_.store(desc.Height, std::memory_order_relaxed);
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
        std::vector<std::shared_ptr<UIDocument>> docs_to_render;
        
        {
            std::lock_guard<std::mutex> lock(documents_mutex_);
            docs_to_render.reserve(documents_.size());
            
            for (auto& [id, doc] : documents_) {
                if (doc && doc->visible && doc->pixels_ready.load(std::memory_order_acquire)) {
                    docs_to_render.push_back(doc);
                }
            }
        }
        
        // Sort by z_order for proper layering
        std::sort(docs_to_render.begin(), docs_to_render.end(),
            [](const auto& a, const auto& b) { return a->z_order < b->z_order; });

        // Render each document
        for (const auto& doc : docs_to_render) {
            if (shutting_down_.load(std::memory_order_acquire)) break;
            if (!d3d_renderer_.is_initialized()) break;
            std::lock_guard<std::mutex> pixel_lock(doc->pixel_mutex);
            if (doc->cached_pixels.empty()) continue;
            if (!doc->pixels_ready.load(std::memory_order_acquire)) continue;
            if (shutting_down_.load(std::memory_order_acquire)) break;
            
            d3d_renderer_.update_texture(
                doc->cached_pixels.data(),
                doc->cached_width,
                doc->cached_height,
                doc->cached_stride
            );

            if (shutting_down_.load(std::memory_order_acquire)) break;
            
            d3d_renderer_.render(
                sw, sh,
                doc->x, doc->y,
                doc->width, doc->height,
                doc->opacity
            );
        }
    }

private:
    UIFramework() = default;

    ~UIFramework() {
        try {
            shutdown();
        } catch (...) {}
    }

    UIFramework(const UIFramework&) = delete;
    UIFramework& operator=(const UIFramework&) = delete;
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
    
    // Mutex for document map operations
    std::mutex documents_mutex_;
    std::mutex init_mutex_;

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

    bool install_present_hook() {
        std::lock_guard<std::mutex> lock(hook_mutex_);
        if (hook_installed_.load(std::memory_order_acquire)) return true;
        instance_ptr_.store(this, std::memory_order_release);
        HWND hwnd = FindWindowA("Arma 3", nullptr);
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
};

// Static member definitions
std::atomic<UIFramework*> UIFramework::instance_ptr_{nullptr};
UIFramework::PresentFn UIFramework::original_present_ = nullptr;
void* UIFramework::hooked_present_addr_ = nullptr;
std::atomic<bool> UIFramework::hook_installed_{false};
std::atomic<bool> UIFramework::minhook_initialized_{false};
std::mutex UIFramework::hook_mutex_;