#pragma once

// The feature half (rendering_integration_features.hpp): the material textures: the texture pages, DDS loading, PAA
// decoding and the texture resolve. Never include it on its own.
namespace RenderIntegration {
inline void kh_tex_cache_release();   // material-texture cache (defined with the loader).
// KH_STB_DEAD_TAIL: the png / jpg / jpeg / tga / bmp route of the synchronous load (the loader cannot start) - the disk
// cache, else the decode and the CPU mip chain.
inline ID3D11ShaderResourceView* kh_tex_load_cached(ID3D11Device* dev, const std::string& path,
                                                    bool srgb, std::string& err);
inline ID3D11ShaderResourceView* kh_dds_create(ID3D11Device* dev, const std::vector<uint8_t>& f, bool srgb,
                                               std::string& err);   // KH_DDS_MIP_FILL.

// Defined with the loader worker in rendering/texture_loader.hpp (it needs the blob and disk-cache helpers).
inline bool kh_texldr_queue(const std::string& khtl_key, const std::string& khtl_path, bool khtl_srgb, bool khtl_force);

// Resolution happens only inside the serialized graphics windows (the injection, or a lock-held flush), so the map
// needs no lock; entries name page layers, and the pages hold the device objects - released by
// kh_tex_cache_release_device at a device reset (a pending entry stays: KH_TEX_RESET_KEEP_PENDING) and by
// kh_tex_cache_release at mission end (reloads come from disk). A page is a Texture2DArray of every loaded texture
// of one (width, height, mips, format) class; a map resolves to (page, layer). Pages grow by doubling and die when
// their last layer is freed; page ids are stable indices into g_tex_pages.
struct KhTexPage {
    ID3D11Texture2D* tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    UINT w = 0, h = 0, mips = 0, cap = 0;
    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    std::vector<uint8_t> used;   // Per layer.
    uint32_t used_n = 0;
};
static std::vector<KhTexPage> g_tex_pages;
static constexpr UINT KH_TEX_PAGE_CAP0 = 4;
// Bumps whenever what a table entry names may change: a page created, grown or freed, a layer placed
// (kh_tex_page_place), and every table rebuild (kh_mat_gpu_rebuild: a material or texture GC).
static uint32_t g_tex_page_gen = 0;

// KH_MAT_TABLE: any pool change (GC) or page change invalidates every set's table base; kh_mat_gpu_ensure re-adds
// what draws. Called by kh_material_gc and kh_tex_cache_gc in flush_locked's housekeeping (the render thread's
// scene flush, or the game thread under a park).
inline void kh_mat_gpu_rebuild() {
    ++g_tex_page_gen;   // Forces the stale path in kh_mat_gpu_ensure.
}

struct KhTexEntry {
    int page = -1;
    int layer = -1;
    bool failed = false;
    uint64_t last_use_ms = 0;   // KH_TEX_GC: GetTickCount64 of the last resolve.
};
struct KhTexRef { ID3D11ShaderResourceView* srv = nullptr; int layer = -1; };

static std::unordered_map<std::string, KhTexEntry> g_tex_cache;
// KH_TEX_WARN_CAP: texture failures reported this session (any thread). A mod with hundreds of wrong paths would
// otherwise fill report_error_once_safe's 256-message cap with one family and silence every later error of the
// session; past 64 distinct failures this family says so once and goes quiet.
static std::atomic<uint32_t> g_tex_warned{ 0 };

inline void kh_tex_page_free_all() {
    for (KhTexPage& khtp : g_tex_pages) { KH_SAFE_RELEASE(khtp.srv); KH_SAFE_RELEASE(khtp.tex); }
    g_tex_pages.clear();
    ++g_tex_page_gen;
}

inline void kh_tex_cache_release() {
    g_tex_cache.clear();
    kh_tex_page_free_all();
}
// KH_TEX_RESET_KEEP_PENDING - the device-reset release (kh_engine_reset_release): the pages and every entry that named
// one go with the device, the failed ones too (a device reset re-resolves every failed texture), but a pending entry
// (no page, not failed) holds nothing of the device and its job is still queued or running on the loader, which a reset
// does not stop. Do not clear it: the next resolve queues the texture a second time (decoded, or read from the disk
// cache, twice; the late first result lands in the new entry and the duplicate is dropped). Kept, the one result lands
// in it and is created on the new device. The mission-end release (kh_session_teardown) still clears them all, after
// the loader's stop.
inline void kh_tex_cache_release_device() {
    for (auto khtd_it = g_tex_cache.begin(); khtd_it != g_tex_cache.end();) {
        if (khtd_it->second.page < 0 && !khtd_it->second.failed) ++khtd_it;
        else khtd_it = g_tex_cache.erase(khtd_it);
    }
    kh_tex_page_free_all();
}

// (Re)creates the page's array at khtg_cap layers, carrying the used layers. One page per (size, mips, format),
// doubling with no cap: a grow past the device's per-resource limit (a quarter of a small card's VRAM, or 2048
// layers) fails that texture.
inline bool kh_tex_page_grow(ID3D11Device* dev, ID3D11DeviceContext* ctx, KhTexPage& khtg_p, UINT khtg_cap) {
    D3D11_TEXTURE2D_DESC khtg_td = {};
    khtg_td.Width = khtg_p.w; khtg_td.Height = khtg_p.h;
    khtg_td.MipLevels = khtg_p.mips; khtg_td.ArraySize = khtg_cap;
    khtg_td.Format = khtg_p.fmt;
    khtg_td.SampleDesc.Count = 1;
    khtg_td.Usage = D3D11_USAGE_DEFAULT;
    khtg_td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* khtg_tex = nullptr;
    if (FAILED(dev->CreateTexture2D(&khtg_td, nullptr, &khtg_tex))) return false;
    // KH_TEX_PAGE_FIT: an explicit array view - every reader declares Texture2DArray, and a null description on a
    // one-layer texture makes a Texture2D view. At two layers or more it is the view the null description makes.
    D3D11_SHADER_RESOURCE_VIEW_DESC khtg_sd = {};
    khtg_sd.Format = khtg_p.fmt;
    khtg_sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    khtg_sd.Texture2DArray.MostDetailedMip = 0;
    khtg_sd.Texture2DArray.MipLevels = khtg_p.mips;
    khtg_sd.Texture2DArray.FirstArraySlice = 0;
    khtg_sd.Texture2DArray.ArraySize = khtg_cap;
    ID3D11ShaderResourceView* khtg_srv = nullptr;
    if (FAILED(dev->CreateShaderResourceView(khtg_tex, &khtg_sd, &khtg_srv))) { khtg_tex->Release(); return false; }
    if (khtg_p.tex) {
        for (UINT khtg_l = 0; khtg_l < khtg_p.cap; ++khtg_l) {
            if (!khtg_p.used[khtg_l]) continue;
            for (UINT khtg_m = 0; khtg_m < khtg_p.mips; ++khtg_m) {
                const UINT khtg_sr = D3D11CalcSubresource(khtg_m, khtg_l, khtg_p.mips);
                ctx->CopySubresourceRegion(khtg_tex, khtg_sr, 0, 0, 0, khtg_p.tex, khtg_sr, nullptr);
            }
        }
        KH_SAFE_RELEASE(khtg_p.srv);
        KH_SAFE_RELEASE(khtg_p.tex);
    }
    khtg_p.tex = khtg_tex;
    khtg_p.srv = khtg_srv;
    khtg_p.used.resize(khtg_cap, 0);
    khtg_p.cap = khtg_cap;
    ++g_tex_page_gen;
    return true;
}

// Moves a freshly loaded single texture into a page layer. The single stays the caller's: it is not released here
// (both callers release it after the call, placed or not). False (entry untouched) on any failure.
inline bool kh_tex_page_place(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                              ID3D11ShaderResourceView* khpp_single, KhTexEntry& khpp_e) {
    ID3D11Resource* khpp_res = nullptr;
    khpp_single->GetResource(&khpp_res);
    if (!khpp_res) return false;
    ID3D11Texture2D* khpp_tex = nullptr;
    khpp_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khpp_tex));
    khpp_res->Release();
    if (!khpp_tex) return false;
    D3D11_TEXTURE2D_DESC khpp_td = {};
    khpp_tex->GetDesc(&khpp_td);
    int khpp_pg = -1;
    for (size_t khpp_i = 0; khpp_i < g_tex_pages.size(); ++khpp_i) {
        const KhTexPage& khpp_p = g_tex_pages[khpp_i];
        if (khpp_p.tex && khpp_p.w == khpp_td.Width && khpp_p.h == khpp_td.Height &&
            khpp_p.mips == khpp_td.MipLevels && khpp_p.fmt == khpp_td.Format) { khpp_pg = static_cast<int>(khpp_i); break; }
    }
    if (khpp_pg < 0) {
        for (size_t khpp_i = 0; khpp_i < g_tex_pages.size(); ++khpp_i) {   // Reuse a dead slot.
            if (!g_tex_pages[khpp_i].tex) { khpp_pg = static_cast<int>(khpp_i); break; }
        }
        if (khpp_pg < 0) { khpp_pg = static_cast<int>(g_tex_pages.size()); g_tex_pages.emplace_back(); }
        KhTexPage& khpp_n = g_tex_pages[khpp_pg];
        khpp_n = KhTexPage();
        khpp_n.w = khpp_td.Width; khpp_n.h = khpp_td.Height;
        khpp_n.mips = khpp_td.MipLevels; khpp_n.fmt = khpp_td.Format;
        // KH_TEX_PAGE_FIT: a first page the device refuses at KH_TEX_PAGE_CAP0 layers (past its per-resource limit,
        // min(max(128 MB, a quarter of the VRAM), 2048 MB): a 16384 x 8192 RGBA8 layer is 683 MB with its chain) is
        // asked for at half the layers, down to one.
        UINT khpp_c0 = KH_TEX_PAGE_CAP0;
        while (!kh_tex_page_grow(dev, ctx, khpp_n, khpp_c0)) {
            if (khpp_c0 <= 1u) { khpp_tex->Release(); return false; }
            khpp_c0 /= 2u;
        }
    }
    KhTexPage& khpp_p = g_tex_pages[khpp_pg];
    if (khpp_p.used_n >= khpp_p.cap) {
        if (!kh_tex_page_grow(dev, ctx, khpp_p, khpp_p.cap * 2)) { khpp_tex->Release(); return false; }
    }
    int khpp_l = -1;
    for (UINT khpp_i = 0; khpp_i < khpp_p.cap; ++khpp_i) if (!khpp_p.used[khpp_i]) { khpp_l = static_cast<int>(khpp_i); break; }
    if (khpp_l < 0) { khpp_tex->Release(); return false; }   // Unreachable: grown above.
    for (UINT khpp_m = 0; khpp_m < khpp_p.mips; ++khpp_m) {
        ctx->CopySubresourceRegion(khpp_p.tex, D3D11CalcSubresource(khpp_m, static_cast<UINT>(khpp_l), khpp_p.mips),
                                   0, 0, 0, khpp_tex, khpp_m, nullptr);
    }
    khpp_tex->Release();
    khpp_p.used[khpp_l] = 1;
    khpp_p.used_n++;
    khpp_e.page = khpp_pg;
    khpp_e.layer = khpp_l;
    // Every path that changes what a table entry would say bumps this counter: kh_mat_gpu_ensure bakes a set once and
    // early-returns on gpu.base >= 0, so a placement into a free layer (the common async case) must invalidate the
    // table as a page grow does, or the material stays stuck with its first-draw scalars.
    ++g_tex_page_gen;
    return true;
}

// Pre-compressed + pre-mipped is the performance path for real assets (a .paa comes here too, made into these
// bytes: KH_PAA); everything else goes through stb.
// KH_DDS_MIPS: a header's level count is held to the full chain to 1 x 1 (Create refuses more). A BC1 / BC2 / BC3
// file whose full-size level is not a multiple of 4 arrives here already decoded (KH_DDS_BLOCK4). A 32-bit file
// short of its chain arrives here only when KH_DDS_MIP_FILL had no memory to build it; else a short chain here is a
// BC file's.
inline ID3D11ShaderResourceView* kh_load_dds(ID3D11Device* dev, const std::vector<uint8_t>& f,
                                             bool srgb, std::string& err) {
    auto rd32 = [&](size_t off) { uint32_t v; memcpy(&v, f.data() + off, 4); return v; };
    if (f.size() < 128 || rd32(0) != 0x20534444u) { err = "not a DDS file"; return nullptr; }
    const uint32_t height = rd32(12), width = rd32(16);
    uint32_t mips = rd32(28);
    const uint32_t pf_flags = rd32(80), fourcc = rd32(84);
    const uint32_t bitcount = rd32(88), rmask = rd32(92), amask = rd32(104);
    if (width == 0 || height == 0 || width > 16384 || height > 16384) { err = "bad dimensions"; return nullptr; }
    if (mips == 0) mips = 1;
    if (mips > 16) mips = 16;
    uint32_t khdd_chain = 1;   // KH_DDS_MIPS: the full chain's levels.
    for (uint32_t khdd_s = width > height ? width : height; khdd_s > 1; khdd_s >>= 1) ++khdd_chain;
    if (mips > khdd_chain) mips = khdd_chain;

    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    bool bc = false;
    uint32_t blk = 16;
    size_t data_off = 128;
    const uint32_t FOURCC_DX10 = 0x30315844u;

    if ((pf_flags & 0x4u) && fourcc == FOURCC_DX10) {
        if (f.size() < 148) { err = "truncated DX10 header"; return nullptr; }
        const uint32_t dxgi = rd32(128);
        data_off = 148;

        switch (dxgi) {
            case 71: case 72: fmt = srgb ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM; bc = true; blk = 8;  break;
            case 74: case 75: fmt = srgb ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM; bc = true; blk = 16; break;
            case 77: case 78: fmt = srgb ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM; bc = true; blk = 16; break;
            case 98: case 99: fmt = srgb ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM; bc = true; blk = 16; break;
            case 28: case 29: fmt = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM; break;
            case 87: case 91: fmt = srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM; break;
            default: err = "unsupported DXGI format " + std::to_string(dxgi); return nullptr;
        }
    } else if (pf_flags & 0x4u) {
        const uint32_t DXT1 = 0x31545844u, DXT3 = 0x33545844u, DXT5 = 0x35545844u;
        if      (fourcc == DXT1) { fmt = srgb ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM; bc = true; blk = 8; }
        else if (fourcc == DXT3) { fmt = srgb ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM; bc = true; }
        else if (fourcc == DXT5) { fmt = srgb ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM; bc = true; }
        else { err = "unsupported FourCC"; return nullptr; }
    } else if ((pf_flags & 0x40u) && bitcount == 32) {
        const bool has_a = amask != 0;
        if      (rmask == 0x00ff0000u) fmt = srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
        else if (rmask == 0x000000ffu) fmt = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        else { err = "unsupported 32-bit masks"; return nullptr; }
        // An x8 variant (no alpha mask) is still created with an A8 format, so its alpha is the byte the writer left
        // there - 255 from most writers, but a 0 reads transparent to a cutout or blend material.
        (void)has_a;
    } else {
        err = "unsupported pixel format (BC1/2/3/7 or 32-bit RGBA expected)";
        return nullptr;
    }

    D3D11_SUBRESOURCE_DATA init[16] = {};
    size_t off = data_off;

    for (uint32_t m = 0; m < mips; ++m) {
        const uint32_t mw = width >> m ? width >> m : 1;
        const uint32_t mh = height >> m ? height >> m : 1;
        const size_t pitch = bc ? static_cast<size_t>((mw + 3) / 4) * blk : static_cast<size_t>(mw) * 4;
        const size_t rows = bc ? (mh + 3) / 4 : mh;
        const size_t bytes = pitch * rows;
        if (off + bytes > f.size()) {
            if (m == 0) { err = "truncated pixel data"; return nullptr; }
            mips = m;   // Short chain: use what the file actually holds.
            break;
        }
        init[m].pSysMem = f.data() + off;
        init[m].SysMemPitch = static_cast<UINT>(pitch);
        off += bytes;
    }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = width; td.Height = height;
    td.MipLevels = mips; td.ArraySize = 1;
    td.Format = fmt;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = dev->CreateTexture2D(&td, init, &tex);
    if (FAILED(hr)) { err = "CreateTexture2D " + hr_str(hr); return nullptr; }
    ID3D11ShaderResourceView* srv = nullptr;
    hr = dev->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    if (FAILED(hr)) { err = "CreateSRV " + hr_str(hr); return nullptr; }
    return srv;
}

// KH_PAA - Bohemia's own texture file, .paa (and .pac, the same file): the Bohemia wiki's "PAA File Format", checked
// against three open readers (BIS.PAA, HEMTT, woozymasta/paa) and against files BI's ImageToPAA / TexView wrote from
// known pictures. Layout: a 2-byte type; tags ("GGAT", a reversed 4-character name, a 4-byte length, the data) of
// which only the channel swizzle (SWIZ) is used; a palette count (0 for these types); then the levels, each { u16
// width (bit 15: LZO-packed, DXT types only), u16 height, u24 stored size, data }, to a 0 x 0 level. A DXT level is
// its blocks, as stored or LZO-packed (Arma 2 on); every other type is BI's LZSS, always.
// The file becomes the .dds route's bytes (kh_paa_to_dds -> kh_load_dds, on the loader worker or the synchronous
// fallback), so creation, page placement, the colour space and the failure reports are the .dds route's. DXT1 / DXT3 /
// DXT5 keep their blocks (BC1 / BC2 / BC3) and the file's own mip chain. Everything else becomes 8-bit BGRA, every
// level: the 16-bit ARGB4444 / ARGB1555 types and the 8:8 grey + alpha one (in Direct3D's own bit order, what BI's
// tools write, measured on their files; the open readers above swap red and blue in ARGB4444, and BIS.PAA in ARGB1555
// too), ARGB8888 (BGRA bytes, as the open readers have it; no BI-written file to measure), the premultiplied DXT2 /
// DXT4 (their colour divided back by alpha), a DXT level 0 whose size is not a multiple of 4 (a block format needs
// that), and any file whose SWIZ tag moved or inverted a channel - every _nohq / _nofhq / _novhq / _nshq / _nopx normal
// map, _sky, _adshq, _dt and _detail: 4x a DXT5's memory, 8x a DXT1's (_dt, _detail). The swizzle is undone (BIS.PAA's
// inverse): each channel the converter kept comes back where the picture had it (a _nohq normal map's X out of alpha).
// One it dropped cannot come back: an alpha reads opaque, a colour channel what the file stores in its place - except a
// _novhq normal map's Z, rebuilt from X and Y (KH_PAA_NOVHQ). A swizzle that only fills colour channels with a
// constant, or alpha with 1 (_smdi, _as, _ads, ...), undoes to itself and keeps the blocks. MAXC (always white from
// BI's tools) and the other tags are not applied. Operation Flashpoint's palette-indexed files are refused. Every
// length and offset in the file is checked before use, and a level's size against what its stored bytes can hold before
// anything is allocated; a level that does not decode, is not the next of level 0's chain (to 1 x 1), or could not come
// from its bytes ends the chain there (level 0: the file fails). The size cap is the stb route's: 16384 on a side.
inline bool kh_paa_ext(const std::string& khpe_path) {
    return kh_ends_with_ci(khpe_path, ".paa") || kh_ends_with_ci(khpe_path, ".pac");
}
// BI's LZSS as a .paa packs a non-DXT level - pbo_unlzss's scheme (a 4 KB window of spaces, flag bits low first,
// 1 = a literal byte, 0 = a 12-bit distance back and a 4-bit length - 3), but only the output's length is known:
// khpz_on bytes from at most khpz_n input bytes, then a 4-byte sum of the output. A .paa's sum is signed (each byte
// as -128 .. 127; measured on BI's files), a PBO's unsigned; either is taken. A back-reference running past the
// output's end stops there (the wiki's reference decoder). False on a short input or a sum that matches neither.
inline bool kh_paa_unlzss(const uint8_t* khpz_in, size_t khpz_n, uint8_t* khpz_out, size_t khpz_on) {
    uint8_t khpz_ring[4096];
    memset(khpz_ring, ' ', sizeof(khpz_ring));
    size_t khpz_r = 0, khpz_ip = 0, khpz_op = 0;
    uint32_t khpz_su = 0, khpz_ss = 0;
    auto khpz_put = [&](uint8_t khpz_c) {
        khpz_out[khpz_op++] = khpz_c;
        khpz_su += khpz_c;
        khpz_ss += static_cast<uint32_t>(static_cast<int32_t>(static_cast<int8_t>(khpz_c)));
        khpz_ring[khpz_r] = khpz_c;
        khpz_r = (khpz_r + 1u) & 4095u;
    };
    while (khpz_op < khpz_on) {
        if (khpz_ip >= khpz_n) return false;
        const uint32_t khpz_f = khpz_in[khpz_ip++];
        for (int khpz_bit = 0; khpz_bit < 8 && khpz_op < khpz_on; ++khpz_bit) {
            if (khpz_f & (1u << khpz_bit)) {
                if (khpz_ip >= khpz_n) return false;
                khpz_put(khpz_in[khpz_ip++]);
            } else {
                if (khpz_n - khpz_ip < 2) return false;
                const size_t khpz_d = khpz_in[khpz_ip] | (static_cast<size_t>(khpz_in[khpz_ip + 1] & 0xF0u) << 4);
                size_t khpz_len = static_cast<size_t>(khpz_in[khpz_ip + 1] & 0x0Fu) + 3u;
                khpz_ip += 2;
                if (khpz_len > khpz_on - khpz_op) khpz_len = khpz_on - khpz_op;
                size_t khpz_s = (khpz_r - khpz_d) & 4095u;
                for (size_t khpz_k = 0; khpz_k < khpz_len; ++khpz_k) {
                    const uint8_t khpz_c = khpz_ring[khpz_s];
                    khpz_s = (khpz_s + 1u) & 4095u;
                    khpz_put(khpz_c);
                }
            }
        }
    }
    if (khpz_n - khpz_ip < 4) return false;
    const uint32_t khpz_st = static_cast<uint32_t>(khpz_in[khpz_ip]) |
                             (static_cast<uint32_t>(khpz_in[khpz_ip + 1]) << 8) |
                             (static_cast<uint32_t>(khpz_in[khpz_ip + 2]) << 16) |
                             (static_cast<uint32_t>(khpz_in[khpz_ip + 3]) << 24);
    return khpz_st == khpz_ss || khpz_st == khpz_su;
}
// LZO1X, as Arma 2 and later pack a large DXT level: minilzo's lzo1x_decompress_safe, instruction for instruction,
// with every read and write bounds-checked. True only when the stream reaches its end marker having written exactly
// khpo_on bytes (input past the marker is not read).
inline bool kh_paa_unlzo(const uint8_t* khpo_in, size_t khpo_n, uint8_t* khpo_out, size_t khpo_on) {
    size_t khpo_ip = 0, khpo_op = 0, khpo_t = 0;
    auto khpo_lit = [&](size_t khpo_k) {
        if (khpo_k > khpo_n - khpo_ip || khpo_k > khpo_on - khpo_op) return false;
        memcpy(khpo_out + khpo_op, khpo_in + khpo_ip, khpo_k);
        khpo_ip += khpo_k;
        khpo_op += khpo_k;
        return true;
    };
    auto khpo_copy = [&](size_t khpo_dist, size_t khpo_k) {   // Byte by byte: a match may overlap its own output.
        if (khpo_dist == 0 || khpo_dist > khpo_op || khpo_k > khpo_on - khpo_op) return false;
        for (size_t khpo_i = 0; khpo_i < khpo_k; ++khpo_i, ++khpo_op) {
            khpo_out[khpo_op] = khpo_out[khpo_op - khpo_dist];
        }
        return true;
    };
    // A length's run of zero bytes, each worth 255, then the byte that ends it plus khpo_base.
    auto khpo_run = [&](size_t khpo_base) {
        size_t khpo_z = 0;
        while (khpo_ip < khpo_n && khpo_in[khpo_ip] == 0) {
            ++khpo_ip;
            khpo_z += 255u;
            if (khpo_z > khpo_on) return false;   // Longer than the output can hold.
        }
        if (khpo_ip >= khpo_n) return false;
        khpo_t = khpo_z + khpo_base + khpo_in[khpo_ip++];
        return true;
    };
    enum { KHPO_LOOP, KHPO_FIRST, KHPO_MATCH, KHPO_DONE } khpo_st = KHPO_LOOP;
    if (khpo_n == 0) return false;
    if (khpo_in[0] > 17) {
        khpo_t = khpo_in[0] - 17u;
        khpo_ip = 1;
        if (!khpo_lit(khpo_t)) return false;
        if (khpo_t < 4) {   // match_next: the literals, then a match.
            if (khpo_ip >= khpo_n) return false;
            khpo_t = khpo_in[khpo_ip++];
            khpo_st = KHPO_MATCH;
        } else {
            khpo_st = KHPO_FIRST;
        }
    }
    for (;;) {
        if (khpo_st == KHPO_LOOP) {
            if (khpo_ip >= khpo_n) return false;
            khpo_t = khpo_in[khpo_ip++];
            if (khpo_t >= 16) { khpo_st = KHPO_MATCH; continue; }
            if (khpo_t == 0 && !khpo_run(15)) return false;
            if (!khpo_lit(khpo_t + 3u)) return false;
            khpo_st = KHPO_FIRST;
        } else if (khpo_st == KHPO_FIRST) {   // first_literal_run.
            if (khpo_ip >= khpo_n) return false;
            khpo_t = khpo_in[khpo_ip++];
            if (khpo_t >= 16) { khpo_st = KHPO_MATCH; continue; }
            if (khpo_ip >= khpo_n) return false;
            const size_t khpo_d = 1u + 0x0800u + (khpo_t >> 2) + (static_cast<size_t>(khpo_in[khpo_ip++]) << 2);
            if (!khpo_copy(khpo_d, 3)) return false;
            khpo_st = KHPO_DONE;
        } else if (khpo_st == KHPO_MATCH) {
            size_t khpo_d = 0, khpo_len = 0;
            if (khpo_t >= 64) {   // M2.
                if (khpo_ip >= khpo_n) return false;
                khpo_d = 1u + ((khpo_t >> 2) & 7u) + (static_cast<size_t>(khpo_in[khpo_ip++]) << 3);
                khpo_len = (khpo_t >> 5) + 1u;
            } else if (khpo_t >= 32) {   // M3.
                khpo_t &= 31u;
                if (khpo_t == 0 && !khpo_run(31)) return false;
                if (khpo_n - khpo_ip < 2) return false;
                khpo_d = 1u + (khpo_in[khpo_ip] >> 2) + (static_cast<size_t>(khpo_in[khpo_ip + 1]) << 6);
                khpo_ip += 2;
                khpo_len = khpo_t + 2u;
            } else if (khpo_t >= 16) {   // M4, or the end marker.
                const size_t khpo_hi = (khpo_t & 8u) << 11;
                khpo_t &= 7u;
                if (khpo_t == 0 && !khpo_run(7)) return false;
                if (khpo_n - khpo_ip < 2) return false;
                const size_t khpo_lo = (khpo_in[khpo_ip] >> 2) + (static_cast<size_t>(khpo_in[khpo_ip + 1]) << 6);
                khpo_ip += 2;
                if (khpo_hi + khpo_lo == 0) return khpo_op == khpo_on;   // eof_found.
                khpo_d = khpo_hi + khpo_lo + 0x4000u;
                khpo_len = khpo_t + 2u;
            } else {   // M1, after a match's literals.
                if (khpo_ip >= khpo_n) return false;
                khpo_d = 1u + (khpo_t >> 2) + (static_cast<size_t>(khpo_in[khpo_ip++]) << 2);
                khpo_len = 2;
            }
            if (!khpo_copy(khpo_d, khpo_len)) return false;
            khpo_st = KHPO_DONE;
        } else {   // match_done: the low two bits of the match's second-last byte are trailing literals.
            khpo_t = khpo_in[khpo_ip - 2] & 3u;
            if (khpo_t == 0) { khpo_st = KHPO_LOOP; continue; }
            if (!khpo_lit(khpo_t)) return false;   // match_next.
            if (khpo_ip >= khpo_n) return false;
            khpo_t = khpo_in[khpo_ip++];
            khpo_st = KHPO_MATCH;
        }
    }
}
// One DXT block's 16 texels as BGRA (row by row), Direct3D's decode: 5:6:5 end points widened by bit replication;
// DXT1 in its 3-colour mode (c0 <= c1) gives transparent black for index 3; DXT2 / DXT3 carry 4-bit alpha, DXT4 /
// DXT5 interpolated alpha, and their colour block is always 4-colour.
inline void kh_paa_bc_block(int khpb_dxt, const uint8_t* khpb_b, uint8_t khpb_px[16][4]) {
    const uint8_t* const khpb_cb = khpb_dxt == 1 ? khpb_b : khpb_b + 8;
    const uint32_t khpb_c0 = khpb_cb[0] | (static_cast<uint32_t>(khpb_cb[1]) << 8);
    const uint32_t khpb_c1 = khpb_cb[2] | (static_cast<uint32_t>(khpb_cb[3]) << 8);
    uint32_t khpb_c[4][4];   // B, G, R, A.
    const uint32_t khpb_e[2] = { khpb_c0, khpb_c1 };
    for (int khpb_k = 0; khpb_k < 2; ++khpb_k) {
        const uint32_t khpb_v = khpb_e[khpb_k];
        const uint32_t khpb_r5 = (khpb_v >> 11) & 31u, khpb_g6 = (khpb_v >> 5) & 63u, khpb_b5 = khpb_v & 31u;
        khpb_c[khpb_k][0] = (khpb_b5 << 3) | (khpb_b5 >> 2);
        khpb_c[khpb_k][1] = (khpb_g6 << 2) | (khpb_g6 >> 4);
        khpb_c[khpb_k][2] = (khpb_r5 << 3) | (khpb_r5 >> 2);
        khpb_c[khpb_k][3] = 255u;
    }
    const bool khpb_four = khpb_dxt != 1 || khpb_c0 > khpb_c1;
    for (int khpb_ch = 0; khpb_ch < 3; ++khpb_ch) {
        const uint32_t khpb_a = khpb_c[0][khpb_ch], khpb_z = khpb_c[1][khpb_ch];
        if (khpb_four) {
            khpb_c[2][khpb_ch] = (2u * khpb_a + khpb_z + 1u) / 3u;
            khpb_c[3][khpb_ch] = (khpb_a + 2u * khpb_z + 1u) / 3u;
        } else {
            khpb_c[2][khpb_ch] = (khpb_a + khpb_z + 1u) / 2u;
            khpb_c[3][khpb_ch] = 0u;
        }
    }
    khpb_c[2][3] = 255u;
    khpb_c[3][3] = khpb_four ? 255u : 0u;
    const uint32_t khpb_ix = khpb_cb[4] | (static_cast<uint32_t>(khpb_cb[5]) << 8) |
                             (static_cast<uint32_t>(khpb_cb[6]) << 16) | (static_cast<uint32_t>(khpb_cb[7]) << 24);
    uint32_t khpb_al[8] = {};
    uint64_t khpb_ab = 0;   // DXT2 / 3: sixteen 4-bit alphas; DXT4 / 5: sixteen 3-bit indices.
    if (khpb_dxt == 2 || khpb_dxt == 3) {
        for (int khpb_i = 0; khpb_i < 8; ++khpb_i) khpb_ab |= static_cast<uint64_t>(khpb_b[khpb_i]) << (8 * khpb_i);
    } else if (khpb_dxt == 4 || khpb_dxt == 5) {
        const uint32_t khpb_a0 = khpb_b[0], khpb_a1 = khpb_b[1];
        khpb_al[0] = khpb_a0;
        khpb_al[1] = khpb_a1;
        if (khpb_a0 > khpb_a1) {
            for (uint32_t khpb_i = 2; khpb_i < 8; ++khpb_i)
                khpb_al[khpb_i] = ((8u - khpb_i) * khpb_a0 + (khpb_i - 1u) * khpb_a1 + 3u) / 7u;
        } else {
            for (uint32_t khpb_i = 2; khpb_i < 6; ++khpb_i)
                khpb_al[khpb_i] = ((6u - khpb_i) * khpb_a0 + (khpb_i - 1u) * khpb_a1 + 2u) / 5u;
            khpb_al[6] = 0u;
            khpb_al[7] = 255u;
        }
        for (int khpb_i = 0; khpb_i < 6; ++khpb_i) {
            khpb_ab |= static_cast<uint64_t>(khpb_b[2 + khpb_i]) << (8 * khpb_i);
        }
    }
    for (int khpb_i = 0; khpb_i < 16; ++khpb_i) {
        const uint32_t* const khpb_s = khpb_c[(khpb_ix >> (2 * khpb_i)) & 3u];
        khpb_px[khpb_i][0] = static_cast<uint8_t>(khpb_s[0]);
        khpb_px[khpb_i][1] = static_cast<uint8_t>(khpb_s[1]);
        khpb_px[khpb_i][2] = static_cast<uint8_t>(khpb_s[2]);
        uint32_t khpb_av = khpb_s[3];
        if (khpb_dxt == 2 || khpb_dxt == 3) khpb_av = static_cast<uint32_t>((khpb_ab >> (4 * khpb_i)) & 15u) * 17u;
        else if (khpb_dxt == 4 || khpb_dxt == 5) khpb_av = khpb_al[(khpb_ab >> (3 * khpb_i)) & 7u];
        khpb_px[khpb_i][3] = static_cast<uint8_t>(khpb_av);
    }
}
// The file as .dds bytes (a 128-byte header, then every level) for kh_load_dds: FourCC DXT1 / DXT3 / DXT5 blocks, or
// 32-bit BGRA (KH_PAA above). khpa_err says why on false. Never throws: a failed allocation is a failed file.
inline bool kh_paa_to_dds(const std::vector<uint8_t>& khpa_f, std::vector<uint8_t>& khpa_dds, std::string& khpa_err) {
    khpa_dds.clear();
    try {
        const uint8_t* const khpa_b = khpa_f.data();
        const size_t khpa_n = khpa_f.size();
        auto khpa_u16 = [&](size_t khpa_o) {
            return static_cast<uint32_t>(khpa_b[khpa_o]) | (static_cast<uint32_t>(khpa_b[khpa_o + 1]) << 8);
        };
        if (khpa_n < 2) { khpa_err = "not a PAA file"; return false; }
        const uint32_t khpa_type = khpa_u16(0);
        int khpa_dxt = 0;          // 1 .. 5: DXTn.
        uint32_t khpa_bpp = 0;     // The other types' bytes per texel.
        switch (khpa_type) {
            case 0xFF01u: case 0xFF02u: case 0xFF03u: case 0xFF04u: case 0xFF05u:
                khpa_dxt = static_cast<int>(khpa_type & 0xFFu); break;
            case 0x4444u: case 0x1555u: case 0x8080u: khpa_bpp = 2; break;
            case 0x8888u: khpa_bpp = 4; break;
            default: {
                if (khpa_type == 0x4747u) {   // "GG": an untyped file's first tag - OFP's palette index.
                    khpa_err = "palette-indexed PAA (Operation Flashpoint) is not supported";
                } else {
                    static const char khpa_hx[] = "0123456789ABCDEF";
                    khpa_err = "unknown PAA type 0x";
                    for (int khpa_s = 12; khpa_s >= 0; khpa_s -= 4) khpa_err += khpa_hx[(khpa_type >> khpa_s) & 15u];
                }
                return false;
            }
        }
        size_t khpa_p = 2;
        uint8_t khpa_swz[4] = { 0, 1, 2, 3 };   // SWIZ as stored: the sources of A, R, G, B.
        while (khpa_n - khpa_p >= 4 && memcmp(khpa_b + khpa_p, "GGAT", 4) == 0) {
            if (khpa_n - khpa_p < 12) { khpa_err = "truncated PAA tag"; return false; }
            const uint32_t khpa_tl = khpa_u16(khpa_p + 8) | (khpa_u16(khpa_p + 10) << 16);
            if (khpa_tl > khpa_n - khpa_p - 12) { khpa_err = "truncated PAA tag"; return false; }
            if (khpa_tl == 4 && memcmp(khpa_b + khpa_p + 4, "ZIWS", 4) == 0) memcpy(khpa_swz, khpa_b + khpa_p + 12, 4);
            khpa_p += 12u + khpa_tl;
        }
        if (khpa_n - khpa_p < 2) { khpa_err = "truncated PAA header"; return false; }
        const size_t khpa_pal = static_cast<size_t>(khpa_u16(khpa_p)) * 3u;   // 0 for these types; skipped.
        khpa_p += 2;
        if (khpa_pal > khpa_n - khpa_p) { khpa_err = "truncated PAA palette"; return false; }
        khpa_p += khpa_pal;
        // The swizzle's inverse, per channel of the picture (A, R, G, B): the stored channel it came from, and
        // whether it was inverted (BIS.PAA's inverse). A colour channel no stored one came from keeps the stored one
        // in its place; an alpha none came from is opaque (khpa_a_one) - unless the stored alpha is the converter's
        // own constant 1 (SWIZ 8), which already is.
        int khpa_src[4] = { 0, 1, 2, 3 };
        bool khpa_neg[4] = { false, false, false, false };
        bool khpa_hit_a = false;
        for (int khpa_ch = 0; khpa_ch < 4; ++khpa_ch) {
            const uint32_t khpa_v = khpa_swz[khpa_ch];
            if (khpa_v >= 4u && khpa_v <= 7u) { khpa_src[khpa_v - 4u] = khpa_ch; khpa_neg[khpa_v - 4u] = true; }
            else if (khpa_v <= 3u) { khpa_src[khpa_v] = khpa_ch; khpa_neg[khpa_v] = false; }
            khpa_hit_a = khpa_hit_a || (khpa_v <= 7u && (khpa_v & 3u) == 0u);
        }
        const bool khpa_a_one = !khpa_hit_a && khpa_swz[0] != 8u;
        bool khpa_swz_id = !khpa_a_one;
        for (int khpa_ch = 0; khpa_ch < 4; ++khpa_ch) {
            khpa_swz_id = khpa_swz_id && khpa_src[khpa_ch] == khpa_ch && !khpa_neg[khpa_ch];
        }
        // KH_PAA_NOVHQ: TexConvert's two-channel normal map (_novhq: R <- 1, G <- G, B <- 1, A <- 1 - R; SWIZ
        // 05 08 02 08) keeps X and Y and drops Z, storing a constant 1 in its place. A unit normal's Z is
        // sqrt(1 - X^2 - Y^2), so it is rebuilt per texel from the X and Y undone below (the decode of a two-channel
        // normal map); normalstrength then scales X and Y in the shader as for any normal map, so a _novhq shades
        // as the _nohq of the same picture does. Every other swizzle, a three-channel _nohq's stored Z among them,
        // is read as stored.
        static const uint8_t khpa_novhq[4] = { 5u, 8u, 2u, 8u };
        const bool khpa_zr = memcmp(khpa_swz, khpa_novhq, 4) == 0;
        struct KhPaaLevel { uint32_t w, h; bool lzo; size_t off, size; };
        KhPaaLevel khpa_lv[16];
        int khpa_nl = 0;
        while (khpa_nl < 16 && khpa_n - khpa_p >= 4) {
            uint32_t khpa_w = khpa_u16(khpa_p);
            const uint32_t khpa_h = khpa_u16(khpa_p + 2);
            if (khpa_w == 0 && khpa_h == 0) break;   // The end.
            const bool khpa_lzo = khpa_dxt != 0 && (khpa_w & 0x8000u) != 0;
            if (khpa_lzo) khpa_w &= 0x7FFFu;
            if (khpa_n - khpa_p < 7) break;
            const size_t khpa_sz = khpa_b[khpa_p + 4] | (static_cast<size_t>(khpa_b[khpa_p + 5]) << 8) |
                                   (static_cast<size_t>(khpa_b[khpa_p + 6]) << 16);
            if (khpa_sz > khpa_n - khpa_p - 7) break;   // Truncated: the levels before it stand.
            khpa_lv[khpa_nl++] = { khpa_w, khpa_h, khpa_lzo, khpa_p + 7, khpa_sz };
            khpa_p += 7u + khpa_sz;
        }
        if (khpa_nl == 0) { khpa_err = "PAA holds no mip level"; return false; }
        const uint32_t khpa_w0 = khpa_lv[0].w, khpa_h0 = khpa_lv[0].h;
        if (khpa_w0 == 0 || khpa_h0 == 0 || khpa_w0 > 16384 || khpa_h0 > 16384) {
            khpa_err = "bad PAA dimensions";
            return false;
        }
        // Blocks as stored only for the three plain DXT types, a level 0 a block format can take, and no swizzle to
        // undo; everything else is BGRA.
        const bool khpa_keep = (khpa_dxt == 1 || khpa_dxt == 3 || khpa_dxt == 5) && khpa_swz_id &&
                               (khpa_w0 & 3u) == 0 && (khpa_h0 & 3u) == 0;
        const uint32_t khpa_blk = khpa_dxt == 1 ? 8u : 16u;
        // The chain, before anything is allocated: each level must be the next of level 0's full chain (which ends
        // at 1 x 1), and its stored bytes must be able to hold its texels - as stored, or packed no tighter than the
        // format can (LZSS: 18 bytes from 2, 144 from 17 with the flag byte; LZO1X: under 256 from each input byte,
        // a run's zero bytes being 255 each). A 50-byte file claiming 16384 x 16384 fails here, not after a 1 GB
        // allocation. The levels that pass are what the body reserves, once level 0 has decoded.
        uint32_t khpa_full = 1;
        for (uint32_t khpa_m = khpa_w0 > khpa_h0 ? khpa_w0 : khpa_h0; khpa_m > 1u; khpa_m >>= 1) ++khpa_full;
        size_t khpa_rn[16] = {};
        size_t khpa_total = 128;   // The DDS header goes in front of the body: no copy at the end.
        int khpa_cl = 0;           // Levels in the chain.
        for (; khpa_cl < khpa_nl && static_cast<uint32_t>(khpa_cl) < khpa_full; ++khpa_cl) {
            const KhPaaLevel& khpa_v = khpa_lv[khpa_cl];
            const uint32_t khpa_ew = khpa_w0 >> khpa_cl ? khpa_w0 >> khpa_cl : 1u;
            const uint32_t khpa_eh = khpa_h0 >> khpa_cl ? khpa_h0 >> khpa_cl : 1u;
            if (khpa_v.w != khpa_ew || khpa_v.h != khpa_eh) break;   // Not the next level of the chain.
            const size_t khpa_n4 = ((khpa_ew + 3u) / 4u) * static_cast<size_t>((khpa_eh + 3u) / 4u);
            const size_t khpa_r = khpa_dxt ? khpa_n4 * khpa_blk : static_cast<size_t>(khpa_ew) * khpa_eh * khpa_bpp;
            const bool khpa_fits = khpa_dxt == 0 ? khpa_r / 9u <= khpa_v.size
                                 : khpa_v.lzo    ? khpa_r / 256u <= khpa_v.size
                                                 : khpa_r <= khpa_v.size;
            if (!khpa_fits) break;
            khpa_rn[khpa_cl] = khpa_r;
            khpa_total += khpa_keep ? khpa_r : static_cast<size_t>(khpa_ew) * khpa_eh * 4u;
        }
        if (khpa_cl == 0) { khpa_err = "corrupt PAA level 0 data"; return false; }
        std::vector<uint8_t> khpa_raw;   // One level's texel data as stored, unpacked.
        std::vector<uint8_t> khpa_body(128);
        uint32_t khpa_mips = 0;
        for (int khpa_l = 0; khpa_l < khpa_cl; ++khpa_l) {
            const KhPaaLevel& khpa_v = khpa_lv[khpa_l];
            const uint32_t khpa_ew = khpa_w0 >> khpa_l ? khpa_w0 >> khpa_l : 1u;
            const uint32_t khpa_eh = khpa_h0 >> khpa_l ? khpa_h0 >> khpa_l : 1u;
            const size_t khpa_bw = (khpa_ew + 3u) / 4u, khpa_bh = (khpa_eh + 3u) / 4u;
            const size_t khpa_rl = khpa_rn[khpa_l];
            const uint8_t* const khpa_src_b = khpa_b + khpa_v.off;
            khpa_raw.resize(khpa_rl);
            bool khpa_ok;
            if (khpa_dxt == 0) khpa_ok = kh_paa_unlzss(khpa_src_b, khpa_v.size, khpa_raw.data(), khpa_rl);
            else if (khpa_v.lzo) khpa_ok = kh_paa_unlzo(khpa_src_b, khpa_v.size, khpa_raw.data(), khpa_rl);
            else { khpa_ok = true; memcpy(khpa_raw.data(), khpa_src_b, khpa_rl); }   // Size checked above.
            if (!khpa_ok) {
                if (khpa_l == 0) { khpa_err = "corrupt PAA level 0 data"; return false; }
                break;
            }
            if (khpa_l == 0) khpa_body.reserve(khpa_total);   // Keeps the header's 128 bytes; no reallocation after.
            if (khpa_keep) {
                khpa_body.insert(khpa_body.end(), khpa_raw.begin(), khpa_raw.end());
            } else {
                const size_t khpa_o0 = khpa_body.size();
                khpa_body.resize(khpa_o0 + static_cast<size_t>(khpa_ew) * khpa_eh * 4u);
                uint8_t* const khpa_d = khpa_body.data() + khpa_o0;   // BGRA, row by row.
                const uint8_t* const khpa_r = khpa_raw.data();
                if (khpa_dxt) {
                    uint8_t khpa_px[16][4];
                    for (size_t khpa_by = 0; khpa_by < khpa_bh; ++khpa_by) {
                        for (size_t khpa_bx = 0; khpa_bx < khpa_bw; ++khpa_bx) {
                            kh_paa_bc_block(khpa_dxt, khpa_r + (khpa_by * khpa_bw + khpa_bx) * khpa_blk, khpa_px);
                            for (size_t khpa_i = 0; khpa_i < 16; ++khpa_i) {
                                const size_t khpa_x = khpa_bx * 4u + (khpa_i & 3u);
                                const size_t khpa_y = khpa_by * 4u + (khpa_i >> 2);
                                if (khpa_x >= khpa_ew || khpa_y >= khpa_eh) continue;
                                memcpy(khpa_d + (khpa_y * khpa_ew + khpa_x) * 4u, khpa_px[khpa_i], 4);
                            }
                        }
                    }
                } else {
                    const size_t khpa_np = static_cast<size_t>(khpa_ew) * khpa_eh;
                    for (size_t khpa_i = 0; khpa_i < khpa_np; ++khpa_i) {
                        uint8_t* const khpa_o = khpa_d + khpa_i * 4u;
                        if (khpa_type == 0x8888u) { memcpy(khpa_o, khpa_r + khpa_i * 4u, 4); continue; }
                        const uint32_t khpa_q = khpa_r[khpa_i * 2u] |
                                                (static_cast<uint32_t>(khpa_r[khpa_i * 2u + 1u]) << 8);
                        if (khpa_type == 0x4444u) {   // A4R4G4B4.
                            for (int khpa_k = 0; khpa_k < 4; ++khpa_k) {
                                khpa_o[khpa_k] = static_cast<uint8_t>(((khpa_q >> (4 * khpa_k)) & 15u) * 17u);
                            }
                        } else if (khpa_type == 0x1555u) {   // A1R5G5B5.
                            for (int khpa_k = 0; khpa_k < 3; ++khpa_k) {
                                const uint32_t khpa_c5 = (khpa_q >> (5 * khpa_k)) & 31u;
                                khpa_o[khpa_k] = static_cast<uint8_t>((khpa_c5 << 3) | (khpa_c5 >> 2));
                            }
                            khpa_o[3] = (khpa_q & 0x8000u) ? 255u : 0u;
                        } else {   // 8:8 grey, then alpha.
                            khpa_o[0] = khpa_o[1] = khpa_o[2] = static_cast<uint8_t>(khpa_q & 0xFFu);
                            khpa_o[3] = static_cast<uint8_t>(khpa_q >> 8);
                        }
                    }
                }
                const size_t khpa_np = static_cast<size_t>(khpa_ew) * khpa_eh;
                if (khpa_dxt == 2 || khpa_dxt == 4) {   // Premultiplied: the colour back over its alpha.
                    for (size_t khpa_i = 0; khpa_i < khpa_np; ++khpa_i) {
                        uint8_t* const khpa_o = khpa_d + khpa_i * 4u;
                        const uint32_t khpa_a = khpa_o[3];
                        if (khpa_a == 0 || khpa_a == 255) continue;
                        for (int khpa_k = 0; khpa_k < 3; ++khpa_k) {
                            const uint32_t khpa_u = (khpa_o[khpa_k] * 255u + khpa_a / 2u) / khpa_a;
                            khpa_o[khpa_k] = static_cast<uint8_t>(khpa_u > 255u ? 255u : khpa_u);
                        }
                    }
                }
                if (!khpa_swz_id) {   // BGRA bytes: A is byte 3, R 2, G 1, B 0.
                    for (size_t khpa_i = 0; khpa_i < khpa_np; ++khpa_i) {
                        uint8_t* const khpa_o = khpa_d + khpa_i * 4u;
                        const uint8_t khpa_s[4] = { khpa_o[0], khpa_o[1], khpa_o[2], khpa_o[3] };
                        for (int khpa_ch = 0; khpa_ch < 4; ++khpa_ch) {
                            const uint8_t khpa_sv = khpa_s[khpa_src[khpa_ch] == 0 ? 3 : 3 - khpa_src[khpa_ch]];
                            khpa_o[khpa_ch == 0 ? 3 : 3 - khpa_ch] = khpa_neg[khpa_ch]
                                                                    ? static_cast<uint8_t>(255u - khpa_sv) : khpa_sv;
                        }
                        if (khpa_a_one) khpa_o[3] = 255u;
                        if (khpa_zr) {   // KH_PAA_NOVHQ: B from R and G (the shader's t * 2 - 1 both ways).
                            const float khpa_x = static_cast<float>(khpa_o[2]) / 127.5f - 1.0f;
                            const float khpa_y = static_cast<float>(khpa_o[1]) / 127.5f - 1.0f;
                            const float khpa_z2 = 1.0f - khpa_x * khpa_x - khpa_y * khpa_y;
                            const float khpa_z = khpa_z2 > 0.0f ? sqrtf(khpa_z2) : 0.0f;
                            const int khpa_zb = static_cast<int>((khpa_z + 1.0f) * 127.5f + 0.5f);
                            khpa_o[0] = static_cast<uint8_t>(khpa_zb > 255 ? 255 : khpa_zb);
                        }
                    }
                }
            }
            ++khpa_mips;
        }
        uint8_t* const khpa_hd = khpa_body.data();   // The 128 bytes reserved in front, zeroed.
        auto khpa_put32 = [&](size_t khpa_o, uint32_t khpa_x) {
            for (int khpa_k = 0; khpa_k < 4; ++khpa_k) {
                khpa_hd[khpa_o + khpa_k] = static_cast<uint8_t>(khpa_x >> (8 * khpa_k));
            }
        };
        khpa_put32(0, 0x20534444u);   // "DDS ".
        khpa_put32(4, 124u);
        khpa_put32(8, 0x1u | 0x2u | 0x4u | 0x1000u | 0x20000u);   // Caps, height, width, pixel format, mip count.
        khpa_put32(12, khpa_h0);
        khpa_put32(16, khpa_w0);
        khpa_put32(28, khpa_mips);
        khpa_put32(76, 32u);
        if (khpa_keep) {
            khpa_put32(80, 0x4u);   // FourCC.
            khpa_put32(84, khpa_dxt == 1 ? 0x31545844u : khpa_dxt == 3 ? 0x33545844u : 0x35545844u);
        } else {
            khpa_put32(80, 0x40u | 0x1u);   // RGB + alpha: B8G8R8A8.
            khpa_put32(88, 32u);
            khpa_put32(92, 0x00FF0000u);
            khpa_put32(96, 0x0000FF00u);
            khpa_put32(100, 0x000000FFu);
            khpa_put32(104, 0xFF000000u);
        }
        khpa_put32(108, 0x1000u | 0x400000u | 0x8u);   // Texture, mipmap, complex.
        khpa_dds.swap(khpa_body);
        return true;
    } catch (const std::exception&) {
        khpa_dds.clear();
        khpa_err = "out of memory";
        return false;
    }
}

// KH_DDS_BLOCK4: Direct3D creates a block-compressed texture only when its full-size level is a multiple of 4 pixels on
// a side. A BC1 / BC2 / BC3 .dds that is not (a 2 x 2, a 6 x 10) is rewritten here, before kh_load_dds (on the loader
// worker, or in the synchronous fallback), as the 32-bit BGRA .dds kh_paa_to_dds makes of such a .paa: every level the
// file holds, decoded by kh_paa_bc_block (Direct3D's decode). Such a BC7 file, which has no decoder here, is refused
// with the reason (CreateTexture2D would give E_INVALIDARG). Every other file is left as it is - one whose header or
// level 0 kh_load_dds refuses too, so its report stands. False: khdu_err says why, khdu_f unchanged. Never throws: a
// failed allocation is a failed file.
inline bool kh_dds_unblock(std::vector<uint8_t>& khdu_f, std::string& khdu_err) {
    try {
        auto khdu_rd = [&](size_t khdu_o) {
            uint32_t khdu_v;
            memcpy(&khdu_v, khdu_f.data() + khdu_o, 4);
            return khdu_v;
        };
        if (khdu_f.size() < 128 || khdu_rd(0) != 0x20534444u) return true;
        const uint32_t khdu_h = khdu_rd(12), khdu_w = khdu_rd(16);
        if (khdu_w == 0 || khdu_h == 0 || khdu_w > 16384 || khdu_h > 16384) return true;
        if ((khdu_w & 3u) == 0u && (khdu_h & 3u) == 0u) return true;   // Blocks Direct3D takes as they are.
        // kh_load_dds's format rule, its block formats only.
        const uint32_t khdu_pf = khdu_rd(80), khdu_cc = khdu_rd(84);
        int khdu_dxt = 0;   // 1 / 3 / 5: DXT1 / DXT3 / DXT5 (BC1 / BC2 / BC3); 7: BC7.
        size_t khdu_off = 128;
        if ((khdu_pf & 0x4u) && khdu_cc == 0x30315844u) {   // "DX10".
            if (khdu_f.size() < 148) return true;
            const uint32_t khdu_dxgi = khdu_rd(128);
            khdu_off = 148;
            if (khdu_dxgi == 71 || khdu_dxgi == 72) khdu_dxt = 1;
            else if (khdu_dxgi == 74 || khdu_dxgi == 75) khdu_dxt = 3;
            else if (khdu_dxgi == 77 || khdu_dxgi == 78) khdu_dxt = 5;
            else if (khdu_dxgi == 98 || khdu_dxgi == 99) khdu_dxt = 7;
        } else if (khdu_pf & 0x4u) {
            if (khdu_cc == 0x31545844u) khdu_dxt = 1;
            else if (khdu_cc == 0x33545844u) khdu_dxt = 3;
            else if (khdu_cc == 0x35545844u) khdu_dxt = 5;
        }
        if (khdu_dxt == 0) return true;
        if (khdu_dxt == 7) {
            khdu_err = "a BC7 .dds must be a multiple of 4 pixels on a side (" + std::to_string(khdu_w) + " x " +
                       std::to_string(khdu_h) + ")";
            return false;
        }
        // The levels kh_load_dds would take: its count rule (KH_DDS_MIPS - its cap of 16 never binds below the
        // chain's 15), a short chain where the file ends.
        uint32_t khdu_mips = khdu_rd(28);
        if (khdu_mips == 0) khdu_mips = 1;
        uint32_t khdu_chain = 1;
        for (uint32_t khdu_s = khdu_w > khdu_h ? khdu_w : khdu_h; khdu_s > 1; khdu_s >>= 1) ++khdu_chain;
        if (khdu_mips > khdu_chain) khdu_mips = khdu_chain;
        const size_t khdu_blk = khdu_dxt == 1 ? 8u : 16u;
        size_t khdu_src[16] = {};   // Each level's blocks in the file.
        size_t khdu_out = 128;      // The BGRA file's size.
        for (uint32_t khdu_m = 0; khdu_m < khdu_mips; ++khdu_m) {
            const size_t khdu_mw = khdu_w >> khdu_m ? khdu_w >> khdu_m : 1u;
            const size_t khdu_mh = khdu_h >> khdu_m ? khdu_h >> khdu_m : 1u;
            const size_t khdu_n = (khdu_mw + 3u) / 4u * khdu_blk * ((khdu_mh + 3u) / 4u);
            if (khdu_off + khdu_n > khdu_f.size()) {
                if (khdu_m == 0) return true;   // kh_load_dds: "truncated pixel data".
                khdu_mips = khdu_m;
                break;
            }
            khdu_src[khdu_m] = khdu_off;
            khdu_off += khdu_n;
            khdu_out += khdu_mw * khdu_mh * 4u;
        }
        std::vector<uint8_t> khdu_d(khdu_out, 0);
        auto khdu_put = [&](size_t khdu_o, uint32_t khdu_x) { memcpy(khdu_d.data() + khdu_o, &khdu_x, 4); };
        khdu_put(0, 0x20534444u);   // "DDS " - kh_paa_to_dds's 32-bit header.
        khdu_put(4, 124u);
        khdu_put(8, 0x1u | 0x2u | 0x4u | 0x1000u | 0x20000u);   // Caps, height, width, pixel format, mip count.
        khdu_put(12, khdu_h);
        khdu_put(16, khdu_w);
        khdu_put(28, khdu_mips);
        khdu_put(76, 32u);
        khdu_put(80, 0x40u | 0x1u);   // RGB + alpha: B8G8R8A8.
        khdu_put(88, 32u);
        khdu_put(92, 0x00FF0000u);
        khdu_put(96, 0x0000FF00u);
        khdu_put(100, 0x000000FFu);
        khdu_put(104, 0xFF000000u);
        khdu_put(108, 0x1000u | 0x400000u | 0x8u);   // Texture, mipmap, complex.
        size_t khdu_at = 128;
        uint8_t khdu_t[16][4];
        for (uint32_t khdu_m = 0; khdu_m < khdu_mips; ++khdu_m) {
            const size_t khdu_mw = khdu_w >> khdu_m ? khdu_w >> khdu_m : 1u;
            const size_t khdu_mh = khdu_h >> khdu_m ? khdu_h >> khdu_m : 1u;
            const size_t khdu_bw = (khdu_mw + 3u) / 4u, khdu_bh = (khdu_mh + 3u) / 4u;
            const uint8_t* const khdu_b = khdu_f.data() + khdu_src[khdu_m];
            uint8_t* const khdu_px = khdu_d.data() + khdu_at;
            for (size_t khdu_by = 0; khdu_by < khdu_bh; ++khdu_by) {
                for (size_t khdu_bx = 0; khdu_bx < khdu_bw; ++khdu_bx) {
                    kh_paa_bc_block(khdu_dxt, khdu_b + (khdu_by * khdu_bw + khdu_bx) * khdu_blk, khdu_t);
                    for (size_t khdu_i = 0; khdu_i < 16; ++khdu_i) {   // The block's texels inside the level.
                        const size_t khdu_x = khdu_bx * 4u + (khdu_i & 3u), khdu_y = khdu_by * 4u + (khdu_i >> 2);
                        if (khdu_x >= khdu_mw || khdu_y >= khdu_mh) continue;
                        memcpy(khdu_px + (khdu_y * khdu_mw + khdu_x) * 4u, khdu_t[khdu_i], 4);
                    }
                }
            }
            khdu_at += khdu_mw * khdu_mh * 4u;
        }
        khdu_f.swap(khdu_d);
        return true;
    } catch (const std::exception&) {
        khdu_err = "out of memory";
        return false;
    }
}

// An entry's page layer as a ref; an entry without a live page (pending, failed, or its page freed) gives the empty ref
// (srv nullptr).
inline KhTexRef kh_tex_ref_of(const KhTexEntry& khtr_e) {
    KhTexRef khtr_r;
    if (khtr_e.page >= 0 && static_cast<size_t>(khtr_e.page) < g_tex_pages.size() && g_tex_pages[khtr_e.page].tex) {
        khtr_r.srv = g_tex_pages[khtr_e.page].srv;
        khtr_r.layer = khtr_e.layer;
    }
    return khtr_r;
}

// Cache key: lower-cased path + colour space (a map may be bound in both). kh_tex_slot_srgb is the per-slot rule:
// diffuse / emissive / specular / speccolor are colour data, normal / orm linear - except that the arma model's
// specular slot holds SMDI, whose channels are data, so it is linear there (as is a .hlsl material's when the script
// named it "smdi": kh_tex_map_srgb).
inline std::string kh_tex_cache_key(const std::string& khtk_path, bool khtk_srgb) {
    std::string khtk_key = khtk_path;
    khtk_key = kh_lower_copy(std::move(khtk_key));
    khtk_key += khtk_srgb ? "|s" : "|l";
    return khtk_key;
}
inline bool kh_tex_slot_srgb(int khts_shader, int khts_slot) {
    if (khts_slot == 4) return khts_shader != 2;
    return khts_slot == 0 || khts_slot == 3 || khts_slot == 5;
}
// A material map's colour space: kh_tex_slot_srgb's rule for the builtin slots, the script's own choice for
// KH_USER_TEX's user slots.
inline bool kh_tex_map_srgb(const KhMaterial& khtm_m, int khtm_slot) {
    if (khtm_slot >= KH_MAT_MAPS_BUILTIN) return khtm_m.maps[khtm_slot].srgb;
    if (khtm_slot == 4 && khtm_m.shader == 1 && khtm_m.map_smdi) return false;   // KH_USER_SLOTS: SMDI is data.
    return kh_tex_slot_srgb(khtm_m.shader, khtm_slot);
}

// KH_TEX_WARN_CAP: a texture's failure, reported under the family's cap. Counted only when the text is
// new to the session (a device reset re-resolves every failed texture; the same path can be asked as
// sRGB and as linear): the count is of distinct failures, as g_chain_warned's is of distinct keys.
inline void kh_tex_report(const std::string& khtw_path, const std::string& khtw_err) {
    const std::string khtw_msg = "KH texture '" + khtw_path + "': " + khtw_err;
    {
        std::lock_guard<std::mutex> khtw_g(g_err_once_mutex);
        if (std::find(g_err_once.begin(), g_err_once.end(), khtw_msg) != g_err_once.end()) return;
    }
    const uint32_t khtw_n = g_tex_warned.fetch_add(1u, std::memory_order_relaxed);
    if (khtw_n < 64u) {
        report_error_once_safe(khtw_msg);
    } else if (khtw_n == 64u) {
        report_error_once_safe("KH texture: more than 64 textures failed this session; later ones are not "
                               "reported");
    }
}

// Cache front door: resolved path -> page layer (queued to the loader, or loaded here if it cannot start, on the first
// miss). Serialized-window contract above; the empty ref = missing / pending / failed (latched).
inline KhTexRef kh_tex_resolve(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                               const std::string& path, bool srgb) {
    if (path.empty() || !dev || !ctx) return KhTexRef();
    const std::string key = kh_tex_cache_key(path, srgb);
    auto it = g_tex_cache.find(key);
    if (it != g_tex_cache.end()) { it->second.last_use_ms = GetTickCount64(); return kh_tex_ref_of(it->second); }

    // A first sight is a disk read + decode + mip build: hand it to the loader worker and answer 'absent' (the material
    // draws defaults for a frame or two; kh_tex_async_pump places finished results at the pass heads and the
    // page-generation bump refills the table). A pending entry has no page and failed clear: a re-resolve refreshes it
    // without re-queueing, the GC ages it (an entry it erased drops the late result by key miss), a device reset keeps
    // it (KH_TEX_RESET_KEEP_PENDING) and the mission end stops the loader before it clears. A loader that cannot
    // start falls through to the synchronous path.
    if (kh_texldr_queue(key, path, srgb, false)) {
        KhTexEntry khtr_p;
        khtr_p.last_use_ms = GetTickCount64();
        g_tex_cache[key] = khtr_p;
        return KhTexRef();
    }

    KhTexEntry e;
    e.last_use_ms = GetTickCount64();
    std::string err;
    ID3D11ShaderResourceView* khtr_single = nullptr;

    if (kh_ends_with_ci(path, ".dds")) {
        std::vector<uint8_t> bytes;
        if (!kh_asset_read(path, bytes, 256ull * 1024u * 1024u)) err = "read failed";   // PBO_PATH.
        else if (kh_dds_unblock(bytes, err)) khtr_single = kh_dds_create(dev, bytes, srgb, err);   // KH_DDS_MIP_FILL.
    } else if (kh_paa_ext(path)) {   // KH_PAA: made into .dds bytes, then the .dds route.
        std::vector<uint8_t> khtr_paa, khtr_dds;
        if (!kh_asset_read(path, khtr_paa, 256ull * 1024u * 1024u)) err = "read failed";   // PBO_PATH.
        else if (kh_paa_to_dds(khtr_paa, khtr_dds, err)) khtr_single = kh_dds_create(dev, khtr_dds, srgb, err);
    } else {
        khtr_single = kh_tex_load_cached(dev, path, srgb, err);   // KH_STB_DEAD_TAIL.
    }

    if (khtr_single) {   // KH_MAT_PAGES: into a page layer; the single view goes.
        if (!kh_tex_page_place(dev, ctx, khtr_single, e)) err = "texture page placement failed";
        khtr_single->Release();
    }

    if (e.page < 0) {
        e.failed = true;
        if (kh_ends_with_ci(path, ".tga")) {
            err += " - if the file opens elsewhere, re-export as uncompressed true-color 24/32-bit TGA";
        }
        kh_tex_report(path, err);   // KH_TEX_WARN_CAP.
    }

    g_tex_cache[key] = e;
    return kh_tex_ref_of(e);
}
}