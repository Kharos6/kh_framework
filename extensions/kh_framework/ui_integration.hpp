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

// PATH_CONFINE (search_mod_folders.hpp's note): every path Ultralight asks for - a page, its links, a fetch from
// its script, Ultralight's own resources - is answered only from inside a trusted folder: Ultralight's
// resources, an html_ui folder (relative below it, or absolute inside it), the PBOs under pbo_root, and the
// game's working folder for Ultralight's "resources/" alone. A drive path elsewhere, a network path and anything
// needing '..' to leave a folder are answered "no such file" without touching the disk - a page, which a
// script can write whole (createHTML), cannot read the player's files or open a network share.
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
        
        std::string pbo_key;

        if (pbo_key_of(path_str, pbo_key)) {   // PBO_PATH: from the PBOs, never the disk.
            const std::shared_ptr<const std::vector<uint8_t>> pbo_data =
                ModFolderSearcher::read_pbo_file_shared(pbo_key);
            if (!pbo_data || pbo_data->empty()) return nullptr;
            return ultralight::Buffer::CreateFromCopy(pbo_data->data(), pbo_data->size());
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
        
        // KH_UI_NARROW: the extension from the UTF-8 text itself, by path::extension's rule (the last '.' of the
        // file name, not its first character; none for "." / "..") - a path made of the text converts it
        // with the code page, and string() back, either of which throws for a name the code page cannot take.
        const std::string khmt_p = path.utf8().data();
        const size_t khmt_s = khmt_p.find_last_of("\\/");
        const std::string khmt_n = khmt_s == std::string::npos ? khmt_p : khmt_p.substr(khmt_s + 1);
        const size_t khmt_d = khmt_n.rfind('.');
        std::string ext = (khmt_n == "." || khmt_n == ".." || khmt_d == std::string::npos || khmt_d == 0)
                              ? std::string() : khmt_n.substr(khmt_d);
        
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

    // KH_UI_ONE_RENDERER: the html_ui folders again at a session's start (the file system lives for the process
    // with the Renderer). Ultralight may ask from its own threads, so readers take a copy under the lock.
    void set_search_paths(const std::vector<std::filesystem::path>& khsp_paths) {
        std::unique_lock<std::shared_mutex> khsp_l(search_mx_);
        search_paths_ = khsp_paths;
    }

    // PBO_PATH (search_mod_folders.hpp's note): an HTML file named with one leading slash is inside the loaded PBOs.
    // Its page is loaded from a file:/// URL under this root, which no file on disk is ever read from: every path
    // Ultralight asks this file system for below it - the page itself, and the css / js / images / fonts its
    // relative links reach, in its own PBO or another's - is the engine path after the root, answered from the PBOs
    // (ModFolderSearcher) in memory. A plain drive path, so Ultralight resolves relative links against it as it
    // does for a page on disk.
    static const std::filesystem::path& pbo_root() {
        static const std::filesystem::path root("C:\\kh_framework_pbo");
        return root;
    }

    // The engine path of a path under pbo_root() ('/' and '\\' alike, ASCII case-insensitive, a separator before
    // the drive tolerated), else false.
    static bool pbo_key_of(const std::string& path_str, std::string& key) {
        auto norm = [](const std::string& s) {
            std::string o;
            o.reserve(s.size());

            for (char ch : s) {
                if (ch == '/') ch = '\\';
                if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch + 32);
                o.push_back(ch);
            }
            
            return o;
        };

        static const std::string root = norm(pbo_root().string());
        std::string p = norm(path_str);
        size_t a = 0;
        while (a < p.size() && p[a] == '\\') ++a;

        if (p.size() <= a + root.size() + 1 || p.compare(a, root.size(), root) != 0 || p[a + root.size()] != '\\') {
            return false;
        }

        key = p.substr(a + root.size() + 1);
        return !key.empty();
    }

private:
    std::vector<std::filesystem::path> search_paths_;
    mutable std::shared_mutex search_mx_;   // KH_UI_ONE_RENDERER: search_paths_ (set_search_paths).

    std::vector<std::filesystem::path> search_paths() const {
        std::shared_lock<std::shared_mutex> khsp_l(search_mx_);
        return search_paths_;
    }
    std::filesystem::path resources_path_;
    mutable LRUCache<std::string, bool> exists_cache_;

    // PATH_CONFINE: the file path_str names inside folder - folder / path_str for a path below it, the path itself
    // for an absolute one inside it - else empty, with nothing touched.
    static std::filesystem::path ConfinedIn(const std::filesystem::path& folder, const std::string& path_str) {
        if (folder.empty()) return {};
        std::filesystem::path full = ModFolderSearcher::confined_join(folder, path_str);
        if (full.empty() && ModFolderSearcher::path_within(path_str, folder)) full = path_str;
        return full;
    }

    // PATH_CONFINE: the game's working folder answers Ultralight's own resources alone - its resource prefix
    // "resources/" (icudt67l.dat, cacert.pem) - so a page reads nothing else of the install folder.
    static bool WorkingFolderResource(const std::string& path_str) {
        if (path_str.size() <= 10 || !ModFolderSearcher::is_plain_relative_path(path_str)) return false;
        for (size_t k = 0; k < 9; ++k) {
            if ((path_str[k] | 0x20) != "resources"[k]) return false;
        }
        return path_str[9] == '/' || path_str[9] == '\\';
    }

    bool CheckExistsInternal(const std::string& path_str) const {
        std::string pbo_key;
        if (pbo_key_of(path_str, pbo_key)) return ModFolderSearcher::pbo_file_exists(pbo_key);   // PBO_PATH.

        try {
            // Check resources path first
            std::filesystem::path full = ConfinedIn(resources_path_, path_str);

            if (!full.empty() && std::filesystem::exists(full)) {
                return true;
            }
            
            // Check search paths
            for (const auto& base : search_paths()) {
                full = ConfinedIn(base, path_str);

                if (!full.empty() && std::filesystem::exists(full)) {
                    return true;
                }
            }
            
            // PATH_CONFINE: the working folder, for Ultralight's resources only (an absolute path is answered
            // above or not at all).
            return WorkingFolderResource(path_str) && std::filesystem::exists(path_str);
        } catch (const std::exception&) {
            // KH_UI_NARROW: a filesystem error, or a name the code page cannot take (std::system_error from the
            // narrow-to-wide conversion: ConfinedIn's absolute path, the working-folder test) - no such file,
            // never an exception into Ultralight.
            return false;
        }
    }
    
    std::filesystem::path ResolvePath(const std::string& path_str) const {
        try {
            // Check resources path first
            std::filesystem::path full = ConfinedIn(resources_path_, path_str);

            if (!full.empty() && std::filesystem::exists(full)) {
                return full;
            }
            
            // Check search paths
            for (const auto& base : search_paths()) {
                full = ConfinedIn(base, path_str);

                if (!full.empty() && std::filesystem::exists(full)) {
                    return full;
                }
            }
            
            // PATH_CONFINE: the working folder, for Ultralight's resources only.
            if (WorkingFolderResource(path_str) && std::filesystem::exists(path_str)) {
                return path_str;
            }
        } catch (const std::exception&) {
            // KH_UI_NARROW: as CheckExistsInternal - a filesystem error or a name the code page cannot take
            // resolves to nothing.
        }
        
        return {};
    }
};

// KH_JS_BOUND: a page's value is page data, and one call converts at most KH_JS_ELEMS_MAX array elements in all,
// nested at most KH_JS_DEPTH_MAX deep. Unbounded, an array holding itself (a = []; a.push(a)) recursed until the
// worker's stack overflowed and the process died; one holding itself twice doubled the work at every level; and a
// length set far past the elements (a.length = 4e9) reserved it all. An array past the depth converts as nil, one
// whose elements would take the call past the budget as empty; each element is counted once, when its array
// reserves it.
// KH_JS_GAME_THREAD: JavaScriptCore calls the bridge on the Ultralight worker (renderer_->Update, the page commands),
// and a game_value may not be made or freed there - it allocates from the engine's SQF pools, which are the game
// thread's alone (the renderer's own rule, rendering_integration.hpp KH_ATTACH_OFFSET). The page's values are copied
// into this plain tree on the worker instead and made game_values in the scheduled call, on the game thread: the same
// nil / bool / number / string / array shapes, the same values, the same bounds as before.
struct KhJsVal {
    enum Kind : uint8_t { KH_JSV_NIL, KH_JSV_BOOL, KH_JSV_NUM, KH_JSV_STR, KH_JSV_ARR };
    Kind kind = KH_JSV_NIL;
    bool b = false;
    float n = 0.0f;
    std::string s;
    std::vector<KhJsVal> a;
};
static constexpr int KH_JS_DEPTH_MAX = 64;
static constexpr size_t KH_JS_ELEMS_MAX = 1000000;
static KhJsVal js_value_to_kh_js(JSContextRef ctx, JSValueRef value, int depth, size_t& budget) {
    KhJsVal out;   // KH_JS_GAME_THREAD: nil.

    if (JSValueIsNull(ctx, value) || JSValueIsUndefined(ctx, value)) {
        return out;
    }

    if (depth > KH_JS_DEPTH_MAX) return out;   // KH_JS_BOUND.
    
    if (JSValueIsBoolean(ctx, value)) {
        out.kind = KhJsVal::KH_JSV_BOOL;
        out.b = JSValueToBoolean(ctx, value);
        return out;
    }
    
    if (JSValueIsNumber(ctx, value)) {
        out.kind = KhJsVal::KH_JSV_NUM;
        out.n = static_cast<float>(JSValueToNumber(ctx, value, nullptr));
        return out;
    }
    
    if (JSValueIsString(ctx, value)) {
        JSStringRef js_str = JSValueToStringCopy(ctx, value, nullptr);
        size_t max_size = JSStringGetMaximumUTF8CStringSize(js_str);
        std::vector<char> buffer(max_size);
        JSStringGetUTF8CString(js_str, buffer.data(), max_size);
        JSStringRelease(js_str);
        out.kind = KhJsVal::KH_JSV_STR;
        out.s = std::string(buffer.data());
        return out;
    }
    
    if (JSValueIsArray(ctx, value)) {
        JSObjectRef arr_obj = JSValueToObject(ctx, value, nullptr);
        JSStringRef length_str = JSStringCreateWithUTF8CString("length");
        JSValueRef length_val = JSObjectGetProperty(ctx, arr_obj, length_str, nullptr);
        JSStringRelease(length_str);
        const double js_len = JSValueToNumber(ctx, length_val, nullptr);
        // KH_JS_BOUND: a count within what the call has left, or empty.
        size_t length = 0;

        if (js_len >= 0.0 && js_len <= static_cast<double>(budget)) {
            length = static_cast<size_t>(js_len);
            budget -= length;
        }

        out.kind = KhJsVal::KH_JSV_ARR;
        out.a.reserve(length);
        
        for (size_t i = 0; i < length; i++) {
            JSValueRef element = JSObjectGetPropertyAtIndex(ctx, arr_obj, static_cast<unsigned>(i), nullptr);
            out.a.push_back(js_value_to_kh_js(ctx, element, depth + 1, budget));
        }
        
        return out;
    }
    
    if (JSValueIsObject(ctx, value)) {
        // Convert object to string representation (JSON)
        JSStringRef json_str = JSValueCreateJSONString(ctx, value, 0, nullptr);

        if (json_str) {
            size_t max_size = JSStringGetMaximumUTF8CStringSize(json_str);
            std::vector<char> buffer(max_size);
            JSStringGetUTF8CString(json_str, buffer.data(), max_size);
            JSStringRelease(json_str);
            out.kind = KhJsVal::KH_JSV_STR;
            out.s = std::string(buffer.data());
            return out;
        }
    }
    
    return out;
}

// KH_JS_GAME_THREAD: a converted value as its game_value. GAME THREAD ONLY (the scheduled call). The tree is at most
// KH_JS_DEPTH_MAX + 1 deep and KH_JS_ELEMS_MAX elements in all (js_value_to_kh_js).
static game_value kh_js_to_game_value(const KhJsVal& v) {
    switch (v.kind) {
    case KhJsVal::KH_JSV_BOOL: return game_value(v.b);
    case KhJsVal::KH_JSV_NUM:  return game_value(v.n);
    case KhJsVal::KH_JSV_STR:  return game_value(std::string(v.s));
    case KhJsVal::KH_JSV_ARR: {
        auto_array<game_value> result;
        result.reserve(v.a.size());
        for (const KhJsVal& e : v.a) result.push_back(kh_js_to_game_value(e));
        return game_value(std::move(result));
    }
    default: return game_value();
    }
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
        
        // KH_JS_BOUND: an exception (an allocation) must not unwind through JavaScriptCore's own frames - the
        // event is dropped instead. The arguments share one element budget.
        try {
            // KH_JS_GAME_THREAD: the arguments as plain values here (this is the Ultralight worker); the game_value
            // - the single argument, or the array of them - is made in the scheduled call, on the game thread.
            size_t budget = KH_JS_ELEMS_MAX;
            std::vector<KhJsVal> args_list;
            args_list.reserve(argumentCount);

            for (size_t i = 0; i < argumentCount; i++) {
                args_list.push_back(js_value_to_kh_js(ctx, arguments[i], 0, budget));
            }

            MainThreadScheduler::instance().schedule([args = std::move(args_list)]() mutable {
                game_value args_to_send;

                try {
                    if (args.size() == 1) {
                        args_to_send = kh_js_to_game_value(args[0]);
                    } else {
                        auto_array<game_value> args_array;
                        args_array.reserve(args.size());
                        for (const KhJsVal& khjs_a : args) args_array.push_back(kh_js_to_game_value(khjs_a));
                        args_to_send = game_value(std::move(args_array));
                    }
                } catch (...) {
                    return;   // An allocation failed: the event is dropped, as on the worker before.
                }

                std::vector<KhJsVal>().swap(args);   // The plain copy is done with: not held across the call.
                raw_call_sqf_args_native_no_return(g_compiled_html_js_event, std::move(args_to_send));
            });
        } catch (...) {}

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
        
    ComPtr<ID3D11ShaderResourceView> get_srv(const std::string& doc_id) {
        std::lock_guard<std::mutex> lock(texture_mutex_);
        auto it = doc_textures_.find(doc_id);
        if (it == doc_textures_.end() || !it->second.srv) return nullptr;
        return it->second.srv;   // ComPtr copy AddRefs while the lock is held
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
        // KH_PLAYER_ONLY: only a machine with an interface shows HTML (hasInterface,
        // framework.hpp's g_is_player). On a dedicated server or a headless client
        // nothing starts - no worker, no hook - and create / open return ''. The
        // html* commands stand down before reaching here (sqf_integration.hpp,
        // kh_gfx_off); this is the gate for any other caller.
        if (!g_is_player) return false;
        if (shutting_down_.load(std::memory_order_acquire)) return false;
        if (initialized_.load(std::memory_order_acquire)) return true;
        std::lock_guard<std::mutex> lock(init_mutex_);
        if (shutting_down_.load(std::memory_order_acquire)) return false;
        if (initialized_.load(std::memory_order_acquire)) return true;
        ladder_reset(present_ladder_);   // KH_UI_HOOK_LADDER: fresh strikes per init
        ladder_reset(wndproc_ladder_);

        // KH_UI_ONE_RENDERER: a worker that ended (an exception out of its loop; a failed start joins itself) is
        // joined and a new one started; its Ultralight objects are left alive (the new worker's Create would
        // release the old Renderer off its thread). A running one - with its Renderer - serves this session too.
        if (worker_thread_.joinable() && !worker_running_.load(std::memory_order_acquire)) {
            worker_thread_.join();
            leak_ultralight_objects(false);
        }

        {   // Under the worker's mutex: an idle worker's wait sees it (no lost wake).
            std::lock_guard<std::mutex> khui_l(worker_mutex_);
            session_active_.store(true, std::memory_order_release);
        }

        if (!worker_thread_.joinable()) {
            should_stop_.store(false, std::memory_order_release);
            worker_initialized_.store(false, std::memory_order_release);
            worker_init_failed_.store(false, std::memory_order_release);
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

                    session_active_.store(false, std::memory_order_release);
                    return false;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        
            if (worker_init_failed_.load(std::memory_order_acquire)) {
                if (worker_thread_.joinable()) {
                    worker_thread_.join();
                }

                session_active_.store(false, std::memory_order_release);
                return false;
            }
        } else {
            // KH_UI_ONE_RENDERER: the session's html_ui folders and a clean exists cache, on the worker, ahead of
            // any command this session queues (the queue is in order).
            queue_command([this]() { refresh_session_paths(); });
        }
        
        initialized_.store(true, std::memory_order_release);
        worker_cv_.notify_all();   // KH_UI_ONE_RENDERER: an idle worker resumes the session's work.
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
                kh_present_subscribe(KH_PRESENT_SLOT_UI, nullptr);   // KH_SHARED_PRESENT: never the hook itself.
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Ignore failures during emergency shutdown
        }
        
        hook_installed_.store(false, std::memory_order_release);
    }

    // KH_UI_ONE_RENDERER: shutdown() closes the session (the mission edges): every document closed on the worker,
    // which then idles with its Renderer for the next session. shutdown(true) stops and joins the worker (the
    // DLL's detach, the singleton's destructor). A session close whose worker has left (an exception out of its
    // loop) is taken to the final stop.
    void shutdown(bool khui_final = false) {
        if (!khui_final && !initialized_.load(std::memory_order_acquire)) return;   // No session open.
        if (!initialized_.load(std::memory_order_acquire) && 
            !worker_thread_.joinable()) {
            return;
        }

        session_active_.store(false, std::memory_order_seq_cst);   // KH_UI_ONE_RENDERER: no hook / update.
        shutting_down_.store(true, std::memory_order_seq_cst);

        // KH_UI_ONE_RENDERER: the stop's wake only on the final path (a session close wakes the worker with its
        // own command; a pending flag with no command made the idle worker spin until it came). The lock is
        // bounded: at the process's exit the worker was ended, possibly holding it.
        if (khui_final) {
            should_stop_.store(true, std::memory_order_release);
            std::unique_lock<std::mutex> lock = khui_lock_bounded(worker_mutex_);
            has_pending_commands_.store(true, std::memory_order_release);
        }

        worker_cv_.notify_all();

        {
            std::lock_guard<std::mutex> lock(hook_mutex_);
            
            if (hook_installed_.load(std::memory_order_acquire) && hooked_present_addr_) {
                kh_present_subscribe(KH_PRESENT_SLOT_UI, nullptr);   // KH_SHARED_PRESENT: never the hook itself.
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

        // KH_UI_ONE_RENDERER: a session close waits for the worker to close every document (as long as it runs:
        // the baseline's join had no bound either); a worker that has left becomes the final stop.
        if (!khui_final && !close_session_documents()) {
            khui_final = true;
            should_stop_.store(true, std::memory_order_release);
            worker_cv_.notify_all();
        }

        // Wait for worker thread to finish
        if (khui_final && worker_thread_.joinable()) {
            worker_thread_.join();
            leak_ultralight_objects(true);   // KH_UI_ONE_RENDERER: only a worker that ran no cleanup.
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

        // Reset ALL state for potential re-initialization (KH_UI_ONE_RENDERER: the worker's own state only when
        // it was stopped)
        d3d_initialized_.store(false, std::memory_order_release);
        initialized_.store(false, std::memory_order_release);
        if (khui_final) {
            worker_initialized_.store(false, std::memory_order_release);
            worker_init_failed_.store(false, std::memory_order_release);
            should_stop_.store(false, std::memory_order_release);
            has_pending_commands_.store(false, std::memory_order_release);
        }
        shutting_down_.store(false, std::memory_order_release);
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

        struct RenderItem {
            std::shared_ptr<UIDocument> doc;
            const std::string* id;
            ComPtr<ID3D11ShaderResourceView> srv;
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

        // KH_PRESENT_OWN (RenderIntegration's kh_present_cb, the same rule): the shared Present runs for every
        // swap chain this process presents. One of another device (an overlay or capture tool) is not the
        // game's frame: no document is drawn for it and its size is not taken as the screen's. Identity by
        // IUnknown; a device that cannot be told is served as before. Two queries and two releases per Present
        // with a document to draw.
        {
            auto khpo_ri = RVExtBridge::get_render_info();
            auto* const khpo_game = khpo_ri ? khpo_ri->d3dDevice : nullptr;
            if (khpo_game) {
                IUnknown* khpo_sd = nullptr;
                IUnknown* khpo_gd = nullptr;
                const bool khpo_have =
                    SUCCEEDED(swap_chain->GetDevice(__uuidof(IUnknown), reinterpret_cast<void**>(&khpo_sd))) &&
                    SUCCEEDED(khpo_game->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&khpo_gd)));
                const bool khpo_foreign = khpo_have && khpo_sd != khpo_gd;
                if (khpo_sd) khpo_sd->Release();
                if (khpo_gd) khpo_gd->Release();
                if (khpo_foreign) return;
            }
        }

        // KH_UI_RESIZE: the back buffer's size, read every Present that draws (nothing before this reads it). A
        // resolution change goes through ResizeBuffers on the same swapchain (RenderIntegration's
        // KH_PRESENT_RESIZE), so a read keyed on the swapchain pointer kept the old size - the viewport, the pixel
        // mapping and the off-screen test with it, and a document past the old edge was clipped or never drawn.
        // GetDesc takes no buffer reference.
        {
            DXGI_SWAP_CHAIN_DESC resize_desc = {};

            if (SUCCEEDED(swap_chain->GetDesc(&resize_desc)) &&
                resize_desc.BufferDesc.Width > 0 && resize_desc.BufferDesc.Height > 0) {
                screen_width_.store(static_cast<int>(resize_desc.BufferDesc.Width), std::memory_order_relaxed);
                screen_height_.store(static_cast<int>(resize_desc.BufferDesc.Height), std::memory_order_relaxed);
            }
        }
        int sw = screen_width_.load(std::memory_order_relaxed);
        int sh = screen_height_.load(std::memory_order_relaxed);

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
                item.srv.Get(),
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
    ~UIFramework() { shutdown(true); }   // KH_UI_ONE_RENDERER: the worker stops with the process.
    UIFramework(const UIFramework&) = delete;
    UIFramework& operator=(const UIFramework&) = delete;

    void worker_thread_func() {
        // KH_UI_ONE_RENDERER: whether this thread is still in its function (initialize() joins a worker that left).
        worker_running_.store(true, std::memory_order_release);
        struct KhUiRunning {
            std::atomic<bool>& flag;
            ~KhUiRunning() { flag.store(false, std::memory_order_release); }
        } khui_running{worker_running_};

        try {
            if (!initialize_ultralight()) {
                worker_init_failed_.store(true, std::memory_order_release);
                return;
            }
            
            worker_initialized_.store(true, std::memory_order_release);

            // KH_UI_HOOK_LADDER: both hooks go through the three-strike
            // ensure, once per worker iteration (~16 ms), so a failed
            // install retries at +1 s and +10 s before it is declared
            // dead - the same shape as RenderIntegration's
            // ensure_reorder_hook. Cheap early-out once installed.
            // Main processing loop
            // KH_UI_WORKER_KEEP: an exception out of an iteration (an allocation in the pixel cache, the hook
            // ladder's scheduling) ended the worker with initialized_ still set - and create_html, open_html and
            // the JS getter wait on the worker with no timeout, so the next such call froze the game thread for
            // good. The step is dropped instead (reported once) and the loop goes on - the hook ensures and the
            // work each under their own guard, so a hook step that keeps failing never stops the commands being
            // served. The ladder's first calls are the loop's own (a pair ahead of it, outside the guard, went).
            bool worker_iter_reported = false;
            auto worker_iter_failed = [&worker_iter_reported]() {
                if (worker_iter_reported) return;
                worker_iter_reported = true;

                try {
                    MainThreadScheduler::instance().schedule([]() {
                        report_error("KH - UI Framework: a worker update failed; the worker carries on");
                    });
                } catch (...) {}
            };

            while (!should_stop_.load(std::memory_order_acquire)) {
                // KH_UI_ONE_RENDERER: between sessions the worker keeps its Renderer and serves commands only (the
                // session close, the next session's refresh): no hook ensured, no update, a 100 ms wait.
                const bool khui_live = session_active_.load(std::memory_order_acquire);

                if (khui_live) {
                    try {
                        ensure_present_hook();
                        ensure_wndproc_hook();
                    } catch (...) {
                        worker_iter_failed();
                    }
                }

                try {
                    process_commands();
                    if (khui_live) update_ultralight();
                } catch (...) {
                    worker_iter_failed();
                }

                {
                    std::unique_lock<std::mutex> lock(worker_mutex_);

                    if (khui_live) {
                        worker_cv_.wait_for(lock, std::chrono::milliseconds(16), [this] {
                            return should_stop_.load(std::memory_order_acquire) ||
                                   has_pending_commands_.load(std::memory_order_acquire);
                        });
                    } else {
                        // Timed as well: a wake missed by a flag set outside this mutex costs <= 100 ms.
                        worker_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                            return should_stop_.load(std::memory_order_acquire) ||
                                   has_pending_commands_.load(std::memory_order_acquire) ||
                                   session_active_.load(std::memory_order_acquire);
                        });
                    }
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

    // KH_UI_ONE_RENDERER: the documents (their Views) closed, the Renderer left standing - the session's end.
    // Worker thread.
    void close_all_documents_internal() {
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
    }

    // The worker's end: every document, then the Renderer, the file system and the font loader.
    void cleanup_ultralight() {
        close_all_documents_internal();
        web_session_ = nullptr;   // KH_UI_SESSION_RESET: before the Renderer that made it.
        web_session_failed_ = false;
        renderer_ = nullptr;
        file_system_.reset();
        font_loader_.reset();
    }

    // KH_UI_ONE_RENDERER: the next session's html_ui folders (find_html_file's and the file system's) and an
    // empty exists cache - what a new file system had at every mission before. Worker thread.
    void refresh_session_paths() {
        html_dirs_ = find_html_ui_directories();

        if (file_system_) {
            file_system_->set_search_paths(html_dirs_);
            file_system_->clear_cache();
        }
    }

    // KH_UI_SESSION_RESET: the session's web storage, made at its first view. With one Renderer for the process
    // (KH_UI_ONE_RENDERER) the default session would carry a page's cookies and local storage from one mission
    // into the next; a non-persistent session per mission starts clean, as the per-mission Renderer did, and
    // writes nothing to disk. Released at the session close, after its views. A session that cannot be made
    // gives nullptr - every view of this session then takes the default one (one store for all of its pages,
    // as before), and the next session asks again. Worker thread.
    ultralight::RefPtr<ultralight::Session> web_session() {
        if (!web_session_ && !web_session_failed_ && renderer_) {
            const std::string khws_name = "kh_ui_" + std::to_string(++web_session_n_);
            web_session_ = renderer_->CreateSession(false, ultralight::String(khws_name.c_str()));
            web_session_failed_ = !web_session_;

            if (web_session_failed_) {   // Once a session: its pages keep the default store (no reset).
                MainThreadScheduler::instance().schedule([]() {
                    report_error("KH - UI Framework: could not create a web session; this mission's pages "
                                 "share the default one (their storage is not reset)");
                });
            }
        }

        return web_session_;
    }

    // KH_UI_ONE_RENDERER: the session close on the worker, waited for while the worker runs (no time bound: a
    // worker that is only slow finishes it, and a stuck one held the baseline's join the same way); false when
    // no worker runs or it left before answering. Game thread.
    bool close_session_documents() {
        if (!worker_thread_.joinable() || !worker_running_.load(std::memory_order_acquire)) return false;
        auto khcs_p = std::make_shared<std::promise<void>>();
        std::future<void> khcs_f = khcs_p->get_future();

        queue_command([this, khcs_p]() {
            try {
                close_all_documents_internal();
                web_session_ = nullptr;   // KH_UI_SESSION_RESET: its views are closed; the next mission's is new.
                web_session_failed_ = false;
                // The memory the closed pages leave (cached images, scripts, style sheets) is given back at the
                // mission edge, as the Renderer's destruction gave back its own: on the worker, outside any
                // Ultralight callback (PurgeMemory's rule).
                if (renderer_) renderer_->PurgeMemory();
            } catch (...) {}

            khcs_p->set_value();
        });

        while (khcs_f.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
            if (!worker_running_.load(std::memory_order_acquire)) {
                return khcs_f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }
        }

        return true;
    }

    // KH_UI_ONE_RENDERER: a lock a thread the process already ended may hold for good (the exit): tried for
    // ~100 ms, then the caller goes on without it - no other thread runs then; a live holder lets go well
    // inside it (the worker's predicate test; the documents' lock with the worker joined). The bound is a
    // steady-clock deadline: a 1 ms sleep lasts a whole timer tick (15.6 ms at Windows' default
    // resolution), so a count of 100 sleeps was ~1.6 s - and the exit takes two such locks.
    static std::unique_lock<std::mutex> khui_lock_bounded(std::mutex& khlb_m) {
        std::unique_lock<std::mutex> khlb_l(khlb_m, std::try_to_lock);
        const auto khlb_end = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        while (!khlb_l.owns_lock() && std::chrono::steady_clock::now() < khlb_end) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            (void)khlb_l.try_lock();
        }

        return khlb_l;
    }

    // KH_UI_ONE_RENDERER: a worker that ended without cleanup_ultralight (ended by the process at its exit, or
    // by an exception out of its loop) leaves its Ultralight objects here. Released off the thread that made
    // them - at the exit, into a library whose threads are gone - they could hang or fault, so they are left
    // alive, never deleted by intent: the Renderer, the web session (KH_UI_SESSION_RESET), the documents (their
    // Views), the file system and the font loader (the Platform holds raw pointers to both). No worker runs.
    // khlu_exit: the documents' lock is bounded (khui_lock_bounded).
    void leak_ultralight_objects(bool khlu_exit) {
        if (!renderer_) return;
        (void)new ultralight::RefPtr<ultralight::Renderer>(renderer_);
        renderer_ = nullptr;   // A count step only: the copy above keeps it.
        if (web_session_) (void)new ultralight::RefPtr<ultralight::Session>(web_session_);   // KH_UI_SESSION_RESET.
        web_session_ = nullptr;
        web_session_failed_ = false;
        (void)file_system_.release();
        (void)font_loader_.release();
        std::unique_lock<std::mutex> khlu_l = khlu_exit ? khui_lock_bounded(documents_mutex_)
                                                        : std::unique_lock<std::mutex>(documents_mutex_);
        (void)new std::unordered_map<std::string, std::shared_ptr<UIDocument>>(std::move(documents_));
        documents_.clear();
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

                    // KH_UI_DIRTY_UNION: the range a Present has not uploaded yet (texture_needs_update still set;
                    // on_present clears it under this lock once it has) is still owed - the new rows join it.
                    // Replacing it lost those rows whenever two passes landed between Presents (a frame under
                    // 60 fps, a mouse move waking the worker, a document skipped off screen), and the texture kept
                    // them stale: a hover highlight left lit, text half redrawn.
                    if (doc->texture_needs_update.load(std::memory_order_acquire) &&
                        doc->dirty_top < doc->dirty_bottom) {
                        doc->dirty_top = std::min(doc->dirty_top, dirty_top);
                        doc->dirty_bottom = std::max(doc->dirty_bottom, dirty_bottom);
                    } else {
                        doc->dirty_top = dirty_top;
                        doc->dirty_bottom = dirty_bottom;
                    }

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
        // KH_UI_ONE_RENDERER: through the worker's mutex once, so a worker between its predicate test and its
        // wait cannot miss this notify (the idle wait is long; the session's was 16 ms).
        { std::lock_guard<std::mutex> khqc_l(worker_mutex_); }
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
        auto view = renderer_->CreateView(w, h, vc, web_session());   // KH_UI_SESSION_RESET.

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
        auto view = renderer_->CreateView(w, h, vc, web_session());   // KH_UI_SESSION_RESET.

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

        // KH_UI_CLOSE_ORDER: the pixels first, then the texture. A Present holding this document from its snapshot
        // uploads under pixel_mutex through update_texture, which makes the texture entry when it is missing; with
        // the texture removed first, an upload in between made it again, and nothing removed it before shutdown.
        // Cleared first, that upload finds no pixels; one already under the lock finishes before the removal.
        if (doc_to_close) {
            doc_to_close->view = nullptr;
            std::lock_guard<std::mutex> pixel_lock(doc_to_close->pixel_mutex);
            doc_to_close->cached_pixels.clear();
            doc_to_close->pixels_ready.store(false, std::memory_order_release);
        }

        d3d_renderer_.remove_doc_texture(doc_id);
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
        auto new_view = renderer_->CreateView(width, height, vc, web_session());   // KH_UI_SESSION_RESET.
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
    // KH_UI_ONE_RENDERER: the worker is inside its function; a session is open (hooks ensured, Ultralight
    // updated) - the worker outlives sessions.
    std::atomic<bool> worker_running_{false};
    std::atomic<bool> session_active_{false};
    // KH_UI_SESSION_RESET: this session's in-memory web storage (cookies, local / session storage, caches) and
    // the count that names each one uniquely. Worker thread only (made, used and released there; a dead
    // worker's is left alive with its other objects).
    ultralight::RefPtr<ultralight::Session> web_session_;
    uint32_t web_session_n_ = 0;
    bool web_session_failed_ = false;   // This session's CreateSession failed: its views take the default one.
    std::mutex worker_mutex_;
    std::condition_variable worker_cv_;
    
    // Command queue
    std::deque<std::function<void()>> pending_commands_;
    std::mutex command_mutex_;
    std::atomic<bool> has_pending_commands_{false};

    // MinHook members
    static std::atomic<UIFramework*> instance_ptr_;
    static void* hooked_present_addr_;   // KH_SHARED_PRESENT: the target this module subscribed through.
    static std::atomic<bool> hook_installed_;
    static std::mutex hook_mutex_;

    // KH_UI_HOOK_LADDER: the three-strike install ladder, one per
    // hook. TWIN of RenderIntegration's g_reorder_hook_* lanes and
    // kh_reorder_hook_fail_round: a failed install is retried after 1 s,
    // then after 10 s, then declared dead for the session with ONE
    // report naming the phase, the MH status and the attempt count. The
    // intermediate strikes are silent (a transient - window not up yet,
    // swap chain refused - must not spam the RPT). Everything is atomic:
    // the worker thread drives the Present ladder, the game thread runs
    // the WndProc install. Self-contained: no render-side lane or build
    // tag reads it; the third-strike report is the only observable. Session-scoped:
    // never reset (a re-init after a failed ladder starts over via
    // ladder_reset, called from the init path only).
    struct HookLadder {
        std::atomic<int32_t>  fail_count{0};   // failed rounds this ladder (0..3)
        std::atomic<int32_t>  fail_phase{0};   // Present: 1 temp swap chain, 2 MinHook init, 3 shared hook
                                                // WndProc: 1 no game window, 2 SetWindowLongPtr
        std::atomic<int32_t>  status{0};   // MH_STATUS (Present) or GetLastError (WndProc) of the failing call
        std::atomic<bool>     failed{false};   // third strike: permanent for the session
        std::atomic<uint64_t> retry_ms{0};   // steady deadline of the next attempt (0 = none scheduled)
        std::atomic<bool>     pending{false};   // WndProc only: an install is scheduled on the game thread
    };

    static HookLadder present_ladder_;
    static HookLadder wndproc_ladder_;

    static uint64_t ladder_now_ms() {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    // An attempt is due when the ladder is alive and no retry deadline is
    // pending or the deadline has passed.
    static bool ladder_due(const HookLadder& l) {
        if (l.failed.load(std::memory_order_acquire)) return false;
        const uint64_t at = l.retry_ms.load(std::memory_order_acquire);
        return at == 0 || ladder_now_ms() >= at;
    }

    static void ladder_success(HookLadder& l) {
        l.fail_count.store(0, std::memory_order_release);   // a success clears the ladder
        l.retry_ms.store(0, std::memory_order_release);
    }

    // The strike. 1st: retry in 1 s. 2nd: retry in 10 s. 3rd: dead, one
    // report. Same intervals as kh_reorder_hook_fail_round.
    static void ladder_fail_round(HookLadder& l, const char* what, const char* disabled) {
        const int32_t n = l.fail_count.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (n == 1) { l.retry_ms.store(ladder_now_ms() + 1000, std::memory_order_release); return; }
        if (n == 2) { l.retry_ms.store(ladder_now_ms() + 10000, std::memory_order_release); return; }
        l.failed.store(true, std::memory_order_release);
        l.retry_ms.store(0, std::memory_order_release);

        const std::string msg = std::string("KH - UI Framework: ") + what + " (phase "
                              + std::to_string(l.fail_phase.load(std::memory_order_acquire)) + ", status "
                              + std::to_string(l.status.load(std::memory_order_acquire)) + ", attempt "
                              + std::to_string(n) + "); " + disabled;

        MainThreadScheduler::instance().schedule([msg]() { report_error(msg); });
    }

    static void ladder_reset(HookLadder& l) {
        l.fail_count.store(0, std::memory_order_release);
        l.fail_phase.store(0, std::memory_order_release);
        l.status.store(0, std::memory_order_release);
        l.failed.store(false, std::memory_order_release);
        l.retry_ms.store(0, std::memory_order_release);
        l.pending.store(false, std::memory_order_release);
    }

    // Worker thread. Early-out once installed; otherwise one attempt per
    // due deadline.
    void ensure_present_hook() {
        if (hook_installed_.load(std::memory_order_acquire)) return;

        if (shutting_down_.load(std::memory_order_acquire) ||
            should_stop_.load(std::memory_order_acquire)) return;

        if (!ladder_due(present_ladder_)) return;
        if (install_present_hook()) { ladder_success(present_ladder_); return; }
        ladder_fail_round(present_ladder_, "Present hook install failed", "HTML rendering disabled");
    }

    // Worker thread schedules, game thread installs (the window's own
    // thread must own the WndProc swap). 'pending' keeps exactly one
    // install in flight so the scheduler is never flooded.
    void ensure_wndproc_hook() {
        if (original_wndproc_.load(std::memory_order_acquire)) return;

        if (shutting_down_.load(std::memory_order_acquire) ||
            should_stop_.load(std::memory_order_acquire)) return;
            
        if (wndproc_ladder_.pending.load(std::memory_order_acquire)) return;
        if (!ladder_due(wndproc_ladder_)) return;
        wndproc_ladder_.pending.store(true, std::memory_order_release);

        // KH_UI_WORKER_KEEP: a schedule that throws must not leave 'pending' set - every later call returned at it
        // and the install never came. Not rethrown: the next iteration asks again.
        try {
            MainThreadScheduler::instance().schedule([this]() {
                if (!shutting_down_.load(std::memory_order_acquire) &&
                    !should_stop_.load(std::memory_order_acquire) &&
                    session_active_.load(std::memory_order_acquire)) {   // KH_UI_ONE_RENDERER.
                    if (install_wndproc_hook()) ladder_success(wndproc_ladder_);
                    else ladder_fail_round(wndproc_ladder_, "WndProc (mouse/keyboard) hook install failed",
                                           "HTML input disabled");
                }

                wndproc_ladder_.pending.store(false, std::memory_order_release);
            });
        } catch (...) {
            wndproc_ladder_.pending.store(false, std::memory_order_release);
        }
    }


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
        // PBO_PATH: one leading slash - the file inside the loaded PBOs at that engine path, no html_ui folder; the
        // page loads from under UIFileSystem::pbo_root, which answers it and its relative links from the PBOs.
        if (ModFolderSearcher::is_pbo_path(filename)) {
            const std::string key = ModFolderSearcher::normalise_pbo_path(filename);
            if (key.empty() || !ModFolderSearcher::pbo_file_exists(key)) return {};
            return UIFileSystem::pbo_root() / key;
        }

        // PATH_CONFINE: a file inside an html_ui folder only - below it (relative), or an absolute path inside it
        // (KH_HTML_ABS: path_within, the rule the page's own links are answered by in UIFileSystem::ConfinedIn).
        // A drive, rooted or network path elsewhere, or one leaving the folder through '..', is not found (and
        // touches nothing).
        for (const auto& base : html_dirs_) {
            auto p = ModFolderSearcher::confined_join(base, filename);
            if (p.empty() && ModFolderSearcher::path_within(filename, base)) p = filename;   // KH_HTML_ABS.
            if (!p.empty() && std::filesystem::exists(p)) return p;
        }

        return {};
    }

    // KH_SHARED_PRESENT: a subscriber of framework.hpp's one Present detour
    // (slot KH_PRESENT_SLOT_UI, after the renderer's); the detour forwards to
    // the original under this module's old rule (an exception there reads as
    // S_OK).
    static void present_cb(IDXGISwapChain* sc, UINT, UINT) {
        __try {
            UIFramework* ptr = instance_ptr_.load(std::memory_order_acquire);
            if (ptr && !ptr->shutting_down_.load(std::memory_order_acquire)) {
                ptr->on_present(sc);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Crash during rendering - likely D3D teardown, ignore
        }
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

        // KH_UI_HOOK_LADDER: failures below record the phase and status
        // and return false; the ladder (ensure_present_hook) decides when
        // to retry and reports once, on the third strike. Nothing partial
        // survives a failed attempt: the temp device/swap chain are ComPtr,
        // a created-but-not-enabled hook is removed before returning.
        const HRESULT khsc_hr = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, 
            D3D11_SDK_VERSION, &sd, &temp_swap_chain, &dev, &fl, &ctx);

        if (FAILED(khsc_hr)) {
            present_ladder_.fail_phase.store(1, std::memory_order_release);
            present_ladder_.status.store(static_cast<int32_t>(khsc_hr), std::memory_order_release);
            instance_ptr_.store(nullptr, std::memory_order_release);
            return false;
        }

        void** vtable = *(void***)temp_swap_chain.Get();
        void* present_addr = vtable[8];

        // KH_SHARED_PRESENT: the renderer detours the same Present, and MinHook
        // takes one hook per target - the one detour is framework.hpp's, shared.
        // Status -1 = MinHook unavailable (phase 2, as the render side records
        // it); any other failure is phase 3 with its MH_STATUS (-2 = no target
        // slot). A hook that would not enable was removed by kh_present_hook.
        const int khmh_st = kh_present_hook(present_addr);
        if (khmh_st != MH_OK) {
            present_ladder_.fail_phase.store(khmh_st == -1 ? 2 : 3, std::memory_order_release);
            present_ladder_.status.store(static_cast<int32_t>(khmh_st), std::memory_order_release);
            instance_ptr_.store(nullptr, std::memory_order_release);
            return false;
        }
        kh_present_subscribe(KH_PRESENT_SLOT_UI, &present_cb);

        hooked_present_addr_ = present_addr;
        hook_installed_.store(true, std::memory_order_release);
        return true;
    }

    void uninstall_present_hook_internal() {
        std::lock_guard<std::mutex> lock(hook_mutex_);
        if (!hook_installed_.load(std::memory_order_acquire)) return;

        if (hooked_present_addr_) {
            // KH_SHARED_PRESENT: unsubscribe only - the detour is shared with the
            // renderer and stays until MH_Uninitialize.
            kh_present_subscribe(KH_PRESENT_SLOT_UI, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
                    if (ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                             ultralight::MouseEvent::kButton_Left, true)) return 0;
                                             
                    break;

                case WM_LBUTTONUP:
                    if (ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                             ultralight::MouseEvent::kButton_Left, false)) return 0;

                    break;

                case WM_RBUTTONDOWN:
                    if (ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                             ultralight::MouseEvent::kButton_Right, true)) return 0;

                    break;

                case WM_RBUTTONUP:
                    if (ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                             ultralight::MouseEvent::kButton_Right, false)) return 0;

                    break;

                case WM_MBUTTONDOWN:
                    if (ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                             ultralight::MouseEvent::kButton_Middle, true)) return 0;

                    break;

                case WM_MBUTTONUP:
                    if (ptr->on_mouse_button(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                             ultralight::MouseEvent::kButton_Middle, false)) return 0;

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

    // Game thread (scheduled by ensure_wndproc_hook). KH_UI_HOOK_LADDER:
    // failures record the phase and status and return false; the ladder
    // retries and reports once, on the third strike. A missing window is
    // the transient this ladder exists for.
    bool install_wndproc_hook() {
        if (original_wndproc_.load(std::memory_order_acquire)) return true;
        HWND hwnd = find_game_window();

        if (!hwnd) {
            wndproc_ladder_.fail_phase.store(1, std::memory_order_release);
            wndproc_ladder_.status.store(0, std::memory_order_release);
            return false;
        }

        game_hwnd_.store(hwnd, std::memory_order_release);
        HWND stored_hwnd = hwnd;
        WNDPROC prev = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(stored_hwnd, GWLP_WNDPROC));
        original_wndproc_.store(prev, std::memory_order_release);

        WNDPROC actual_prev = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(stored_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hooked_wndproc))
        );

        if (!actual_prev) {
            DWORD err = GetLastError();
            original_wndproc_.store(nullptr, std::memory_order_release);   // nothing partial survives the round
            game_hwnd_.store(nullptr, std::memory_order_release);
            wndproc_ladder_.fail_phase.store(2, std::memory_order_release);
            wndproc_ladder_.status.store(static_cast<int32_t>(err), std::memory_order_release);
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
void* UIFramework::hooked_present_addr_ = nullptr;
std::atomic<bool> UIFramework::hook_installed_{false};
std::mutex UIFramework::hook_mutex_;
UIFramework::HookLadder UIFramework::present_ladder_;
UIFramework::HookLadder UIFramework::wndproc_ladder_;
std::atomic<WNDPROC> UIFramework::original_wndproc_{nullptr};
std::atomic<HWND> UIFramework::game_hwnd_{nullptr};