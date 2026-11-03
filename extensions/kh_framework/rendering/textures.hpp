#pragma once

// The feature half (rendering_integration_features.hpp): the material textures: the texture pages, DDS loading, PAA
// decoding and the texture resolve. Never include it on its own.
namespace RenderIntegration {
// STB_DEAD_TAIL: the png / jpg / jpeg / tga / bmp route of the synchronous load (the loader cannot start) - the disk
// cache, else the decode and the CPU mip chain.
inline ID3D11ShaderResourceView* tex_load_cached(ID3D11Device* dev, const std::string& path,
                                                 bool srgb, std::string& err);
inline ID3D11ShaderResourceView* dds_create(ID3D11Device* dev, const std::vector<uint8_t>& f, bool srgb,
                                            std::string& err);   // DDS_MIP_FILL.

// Defined with the loader worker in rendering/texture_loader.hpp (it needs the blob and disk-cache helpers).
inline bool texldr_queue(const std::string& key, const std::string& path, bool srgb, bool force);

// Resolution happens only inside the serialized graphics windows (the render thread's injection, scene flush and UI
// flush, or a flush under the park - TEX_ASYNC), so the map needs no lock; entries name page layers, and the
// pages hold the device objects - released by tex_cache_release_device at a device reset (a pending entry stays:
// TEX_RESET_KEEP_PENDING) and by tex_cache_release at mission end (reloads come from disk). A page is a
// Texture2DArray of every loaded texture of one (width, height, mips, format) class; a map resolves to (page, layer).
// Pages grow by doubling and die when their last layer is freed; page ids are stable indices into g_tex_pages.
struct TexPage {
    ID3D11Texture2D* tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    UINT w = 0, h = 0, mips = 0, cap = 0;
    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    std::vector<uint8_t> used;   // Per layer.
    uint32_t used_n = 0;
};
static std::vector<TexPage> g_tex_pages;
static constexpr UINT TEX_PAGE_CAP0 = 4;
// Bumps whenever what a table entry names may change: a page created, grown or freed, a layer placed
// (tex_page_place), and every table rebuild (mat_gpu_rebuild: a material or texture GC).
static uint32_t g_tex_page_gen = 0;

// MAT_TABLE: any pool change (GC) or page change invalidates every set's table base; mat_gpu_ensure re-adds
// what draws. Called by material_gc and tex_cache_gc in flush_locked's housekeeping (the render thread's
// scene flush, or the game thread under a park).
inline void mat_gpu_rebuild() {
    ++g_tex_page_gen;   // Forces the stale path in mat_gpu_ensure.
}

struct TexEntry {
    int page = -1;
    int layer = -1;
    bool failed = false;
    uint64_t last_use_ms = 0;   // TEX_GC: GetTickCount64 of the last resolve.
};
struct TexRef { ID3D11ShaderResourceView* srv = nullptr; int layer = -1; };

static std::unordered_map<std::string, TexEntry> g_tex_cache;
// TEX_WARN_CAP: texture failures reported this session (any thread). A mod with hundreds of wrong paths would
// otherwise fill report_error_once_safe's 256-message cap with one family and silence every later error of the
// session; past 64 distinct failures this family says so once and goes quiet.
static std::atomic<uint32_t> g_tex_warned{ 0 };

inline void tex_page_free_all() {
    for (TexPage& tp : g_tex_pages) { KH_SAFE_RELEASE(tp.srv); KH_SAFE_RELEASE(tp.tex); }
    g_tex_pages.clear();
    ++g_tex_page_gen;
}

inline void tex_cache_release() {
    g_tex_cache.clear();
    tex_page_free_all();
}
// TEX_RESET_KEEP_PENDING - the device-reset release (engine_reset_release): the pages and every entry that named
// one go with the device, the failed ones too (a device reset re-resolves every failed texture), but a pending entry
// (no page, not failed) holds nothing of the device and its job is still queued or running on the loader, which a reset
// does not stop. Do not clear it: the next resolve queues the texture a second time (decoded, or read from the disk
// cache, twice; the late first result lands in the new entry and the duplicate is dropped). Kept, the one result lands
// in it and is created on the new device. The mission-end release (session_teardown) still clears them all, after
// the loader's stop.
inline void tex_cache_release_device() {
    for (auto it = g_tex_cache.begin(); it != g_tex_cache.end();) {
        if (it->second.page < 0 && !it->second.failed) ++it;
        else it = g_tex_cache.erase(it);
    }
    tex_page_free_all();
}

// (Re)creates the page's array at cap layers, carrying the used layers. One page per (size, mips, format),
// doubling with no cap: a grow past the device's per-resource limit (a quarter of a small card's VRAM, or 2048
// layers) fails that texture.
inline bool tex_page_grow(ID3D11Device* dev, ID3D11DeviceContext* ctx, TexPage& p, UINT cap) {
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = p.w; td.Height = p.h;
    td.MipLevels = p.mips; td.ArraySize = cap;
    td.Format = p.fmt;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &tex))) return false;
    // TEX_PAGE_FIT: an explicit array view - every reader declares Texture2DArray, and a null description on a
    // one-layer texture makes a Texture2D view. At two layers or more it is the view the null description makes.
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = p.fmt;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    sd.Texture2DArray.MostDetailedMip = 0;
    sd.Texture2DArray.MipLevels = p.mips;
    sd.Texture2DArray.FirstArraySlice = 0;
    sd.Texture2DArray.ArraySize = cap;
    ID3D11ShaderResourceView* srv = nullptr;
    if (FAILED(dev->CreateShaderResourceView(tex, &sd, &srv))) { tex->Release(); return false; }
    if (p.tex) {
        for (UINT l = 0; l < p.cap; ++l) {
            if (!p.used[l]) continue;
            for (UINT m = 0; m < p.mips; ++m) {
                const UINT sr = D3D11CalcSubresource(m, l, p.mips);
                ctx->CopySubresourceRegion(tex, sr, 0, 0, 0, p.tex, sr, nullptr);
            }
        }
        KH_SAFE_RELEASE(p.srv);
        KH_SAFE_RELEASE(p.tex);
    }
    p.tex = tex;
    p.srv = srv;
    p.used.resize(cap, 0);
    p.cap = cap;
    ++g_tex_page_gen;
    return true;
}

// Moves a freshly loaded single texture into a page layer. The single stays the caller's: it is not released here
// (both callers release it after the call, placed or not). False (entry untouched) on any failure.
inline bool tex_page_place(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                           ID3D11ShaderResourceView* single, TexEntry& e) {
    ID3D11Resource* res = nullptr;
    single->GetResource(&res);
    if (!res) return false;
    ID3D11Texture2D* tex = nullptr;
    res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    res->Release();
    if (!tex) return false;
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    int pg = -1;
    for (size_t i = 0; i < g_tex_pages.size(); ++i) {
        const TexPage& p = g_tex_pages[i];
        if (p.tex && p.w == td.Width && p.h == td.Height &&
            p.mips == td.MipLevels && p.fmt == td.Format) { pg = static_cast<int>(i); break; }
    }
    if (pg < 0) {
        for (size_t i = 0; i < g_tex_pages.size(); ++i) {   // Reuse a dead slot.
            if (!g_tex_pages[i].tex) { pg = static_cast<int>(i); break; }
        }
        if (pg < 0) { pg = static_cast<int>(g_tex_pages.size()); g_tex_pages.emplace_back(); }
        TexPage& n = g_tex_pages[pg];
        n = TexPage();
        n.w = td.Width; n.h = td.Height;
        n.mips = td.MipLevels; n.fmt = td.Format;
        // TEX_PAGE_FIT: a first page the device refuses at TEX_PAGE_CAP0 layers (past its per-resource limit,
        // min(max(128 MB, a quarter of the VRAM), 2048 MB): a 16384 x 8192 RGBA8 layer is 683 MB with its chain) is
        // asked for at half the layers, down to one.
        UINT c0 = TEX_PAGE_CAP0;
        while (!tex_page_grow(dev, ctx, n, c0)) {
            if (c0 <= 1u) { tex->Release(); return false; }
            c0 /= 2u;
        }
    }
    TexPage& p = g_tex_pages[pg];
    if (p.used_n >= p.cap) {
        if (!tex_page_grow(dev, ctx, p, p.cap * 2)) { tex->Release(); return false; }
    }
    int l = -1;
    for (UINT i = 0; i < p.cap; ++i) if (!p.used[i]) { l = static_cast<int>(i); break; }
    if (l < 0) { tex->Release(); return false; }   // Unreachable: grown above.
    for (UINT m = 0; m < p.mips; ++m) {
        ctx->CopySubresourceRegion(p.tex, D3D11CalcSubresource(m, static_cast<UINT>(l), p.mips),
                                   0, 0, 0, tex, m, nullptr);
    }
    tex->Release();
    p.used[l] = 1;
    p.used_n++;
    e.page = pg;
    e.layer = l;
    // Every path that changes what a table entry would say bumps this counter: mat_gpu_ensure bakes a set once and
    // early-returns on gpu.base >= 0, so a placement into a free layer (the common async case) must invalidate the
    // table as a page grow does, or the material stays stuck with its first-draw scalars.
    ++g_tex_page_gen;
    return true;
}

// Pre-compressed + pre-mipped is the performance path for real assets (a .paa comes here too, made into these
// bytes: PAA); everything else goes through stb.
// DDS_MIPS: a header's level count is held to the full chain to 1 x 1 (Create refuses more). A BC1 / BC2 / BC3
// file whose full-size level is not a multiple of 4 arrives here already decoded (DDS_BLOCK4). A 32-bit file
// short of its chain arrives here only when DDS_MIP_FILL had no memory to build it; else a short chain here is a
// BC file's.
inline ID3D11ShaderResourceView* load_dds(ID3D11Device* dev, const std::vector<uint8_t>& f,
                                          bool srgb, std::string& err) {
    auto rd32 = [&](size_t off) { uint32_t v; memcpy(&v, f.data() + off, 4); return v; };
    if (f.size() < 128 || rd32(0) != 0x20534444u) { err = "not a DDS file"; return nullptr; }
    const uint32_t height = rd32(12), width = rd32(16);
    uint32_t mips = rd32(28);
    const uint32_t pf_flags = rd32(80), fourcc = rd32(84);
    const uint32_t bitcount = rd32(88), rmask = rd32(92);
    if (width == 0 || height == 0 || width > 16384 || height > 16384) { err = "bad dimensions"; return nullptr; }
    if (mips == 0) mips = 1;
    if (mips > 16) mips = 16;
    uint32_t chain = 1;   // DDS_MIPS: the full chain's levels.
    for (uint32_t s = width > height ? width : height; s > 1; s >>= 1) ++chain;
    if (mips > chain) mips = chain;

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
        if      (rmask == 0x00ff0000u) fmt = srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
        else if (rmask == 0x000000ffu) fmt = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        else { err = "unsupported 32-bit masks"; return nullptr; }
        // An x8 variant (no alpha mask) is still created with an A8 format: its padding byte was set to 255 by the
        // routes' pre-pass (EA_DDS_X8, dds_unblock), so it reads opaque, as the format defines.
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

// PAA - Bohemia's own texture file, .paa (and .pac, the same file): the Bohemia wiki's "PAA File Format", checked
// against three open readers (BIS.PAA, HEMTT, woozymasta/paa) and against files BI's ImageToPAA / TexView wrote from
// known pictures. Layout: a 2-byte type; tags ("GGAT", a reversed 4-character name, a 4-byte length, the data) of
// which only the channel swizzle (SWIZ) is used; a palette count (0 for these types); then the levels, each { u16
// width (bit 15: LZO-packed, DXT types only), u16 height, u24 stored size, data }, to a 0 x 0 level. A DXT level is
// its blocks, as stored or LZO-packed (Arma 2 on); every other type is BI's LZSS, always.
// The file becomes the .dds route's bytes (paa_to_dds -> load_dds, on the loader worker or the synchronous
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
// _novhq normal map's Z, rebuilt from X and Y (PAA_NOVHQ). A swizzle that only fills colour channels with a
// constant, or alpha with 1 (_smdi, _as, _ads, ...), undoes to itself and keeps the blocks. MAXC (always white from
// BI's tools) and the other tags are not applied. Operation Flashpoint's palette-indexed files are refused. Every
// length and offset in the file is checked before use, and a level's size against what its stored bytes can hold before
// anything is allocated; a level that does not decode, is not the next of level 0's chain (to 1 x 1), or could not come
// from its bytes ends the chain there (level 0: the file fails). The size cap is the stb route's: 16384 on a side.
inline bool paa_ext(const std::string& path) {
    return ends_with_ci(path, ".paa") || ends_with_ci(path, ".pac");
}
// BI's LZSS as a .paa packs a non-DXT level - pbo_unlzss's scheme (a 4 KB window of spaces, flag bits low first,
// 1 = a literal byte, 0 = a 12-bit distance back and a 4-bit length - 3), but only the output's length is known:
// on bytes from at most n input bytes, then a 4-byte sum of the output. A .paa's sum is signed (each byte
// as -128 .. 127; measured on BI's files), a PBO's unsigned; either is taken. A back-reference running past the
// output's end stops there (the wiki's reference decoder). False on a short input or a sum that matches neither.
inline bool paa_unlzss(const uint8_t* in, size_t n, uint8_t* out, size_t on) {
    uint8_t ring[4096];
    memset(ring, ' ', sizeof(ring));
    size_t r = 0, ip = 0, op = 0;
    uint32_t su = 0, ss = 0;
    auto put = [&](uint8_t c) {
        out[op++] = c;
        su += c;
        ss += static_cast<uint32_t>(static_cast<int32_t>(static_cast<int8_t>(c)));
        ring[r] = c;
        r = (r + 1u) & 4095u;
    };
    while (op < on) {
        if (ip >= n) return false;
        const uint32_t f = in[ip++];
        for (int bit = 0; bit < 8 && op < on; ++bit) {
            if (f & (1u << bit)) {
                if (ip >= n) return false;
                put(in[ip++]);
            } else {
                if (n - ip < 2) return false;
                const size_t d = in[ip] | (static_cast<size_t>(in[ip + 1] & 0xF0u) << 4);
                size_t len = static_cast<size_t>(in[ip + 1] & 0x0Fu) + 3u;
                ip += 2;
                if (len > on - op) len = on - op;
                size_t s = (r - d) & 4095u;
                for (size_t k = 0; k < len; ++k) {
                    const uint8_t c = ring[s];
                    s = (s + 1u) & 4095u;
                    put(c);
                }
            }
        }
    }
    if (n - ip < 4) return false;
    const uint32_t st = static_cast<uint32_t>(in[ip]) |
                        (static_cast<uint32_t>(in[ip + 1]) << 8) |
                        (static_cast<uint32_t>(in[ip + 2]) << 16) |
                        (static_cast<uint32_t>(in[ip + 3]) << 24);
    return st == ss || st == su;
}
// LZO1X, as Arma 2 and later pack a large DXT level: minilzo's lzo1x_decompress_safe, instruction for instruction,
// with every read and write bounds-checked. True only when the stream reaches its end marker having written exactly
// on bytes (input past the marker is not read).
inline bool paa_unlzo(const uint8_t* in, size_t n, uint8_t* out, size_t on) {
    size_t ip = 0, op = 0, t = 0;
    auto lit = [&](size_t k) {
        if (k > n - ip || k > on - op) return false;
        memcpy(out + op, in + ip, k);
        ip += k;
        op += k;
        return true;
    };
    auto copy = [&](size_t dist, size_t k) {   // Byte by byte: a match may overlap its own output.
        if (dist == 0 || dist > op || k > on - op) return false;
        for (size_t i = 0; i < k; ++i, ++op) {
            out[op] = out[op - dist];
        }
        return true;
    };
    // A length's run of zero bytes, each worth 255, then the byte that ends it plus base.
    auto run = [&](size_t base) {
        size_t z = 0;
        while (ip < n && in[ip] == 0) {
            ++ip;
            z += 255u;
            if (z > on) return false;   // Longer than the output can hold.
        }
        if (ip >= n) return false;
        t = z + base + in[ip++];
        return true;
    };
    enum { LZO_LOOP, LZO_FIRST, LZO_MATCH, LZO_DONE } st = LZO_LOOP;
    if (n == 0) return false;
    if (in[0] > 17) {
        t = in[0] - 17u;
        ip = 1;
        if (!lit(t)) return false;
        if (t < 4) {   // match_next: the literals, then a match.
            if (ip >= n) return false;
            t = in[ip++];
            st = LZO_MATCH;
        } else {
            st = LZO_FIRST;
        }
    }
    for (;;) {
        if (st == LZO_LOOP) {
            if (ip >= n) return false;
            t = in[ip++];
            if (t >= 16) { st = LZO_MATCH; continue; }
            if (t == 0 && !run(15)) return false;
            if (!lit(t + 3u)) return false;
            st = LZO_FIRST;
        } else if (st == LZO_FIRST) {   // first_literal_run.
            if (ip >= n) return false;
            t = in[ip++];
            if (t >= 16) { st = LZO_MATCH; continue; }
            if (ip >= n) return false;
            const size_t d = 1u + 0x0800u + (t >> 2) + (static_cast<size_t>(in[ip++]) << 2);
            if (!copy(d, 3)) return false;
            st = LZO_DONE;
        } else if (st == LZO_MATCH) {
            size_t d = 0, len = 0;
            if (t >= 64) {   // M2.
                if (ip >= n) return false;
                d = 1u + ((t >> 2) & 7u) + (static_cast<size_t>(in[ip++]) << 3);
                len = (t >> 5) + 1u;
            } else if (t >= 32) {   // M3.
                t &= 31u;
                if (t == 0 && !run(31)) return false;
                if (n - ip < 2) return false;
                d = 1u + (in[ip] >> 2) + (static_cast<size_t>(in[ip + 1]) << 6);
                ip += 2;
                len = t + 2u;
            } else if (t >= 16) {   // M4, or the end marker.
                const size_t hi = (t & 8u) << 11;
                t &= 7u;
                if (t == 0 && !run(7)) return false;
                if (n - ip < 2) return false;
                const size_t lo = (in[ip] >> 2) + (static_cast<size_t>(in[ip + 1]) << 6);
                ip += 2;
                if (hi + lo == 0) return op == on;   // eof_found.
                d = hi + lo + 0x4000u;
                len = t + 2u;
            } else {   // M1, after a match's literals.
                if (ip >= n) return false;
                d = 1u + (t >> 2) + (static_cast<size_t>(in[ip++]) << 2);
                len = 2;
            }
            if (!copy(d, len)) return false;
            st = LZO_DONE;
        } else {   // match_done: the low two bits of the match's second-last byte are trailing literals.
            t = in[ip - 2] & 3u;
            if (t == 0) { st = LZO_LOOP; continue; }
            if (!lit(t)) return false;   // match_next.
            if (ip >= n) return false;
            t = in[ip++];
            st = LZO_MATCH;
        }
    }
}
// One DXT block's 16 texels as BGRA (row by row), Direct3D's decode: 5:6:5 end points widened by bit replication;
// DXT1 in its 3-colour mode (c0 <= c1) gives transparent black for index 3; DXT2 / DXT3 carry 4-bit alpha, DXT4 /
// DXT5 interpolated alpha, and their colour block is always 4-colour.
inline void paa_bc_block(int dxt, const uint8_t* b, uint8_t px[16][4]) {
    const uint8_t* const cb = dxt == 1 ? b : b + 8;
    const uint32_t c0 = cb[0] | (static_cast<uint32_t>(cb[1]) << 8);
    const uint32_t c1 = cb[2] | (static_cast<uint32_t>(cb[3]) << 8);
    uint32_t c[4][4];   // B, G, R, A.
    const uint32_t e[2] = { c0, c1 };
    for (int k = 0; k < 2; ++k) {
        const uint32_t v = e[k];
        const uint32_t r5 = (v >> 11) & 31u, g6 = (v >> 5) & 63u, b5 = v & 31u;
        c[k][0] = (b5 << 3) | (b5 >> 2);
        c[k][1] = (g6 << 2) | (g6 >> 4);
        c[k][2] = (r5 << 3) | (r5 >> 2);
        c[k][3] = 255u;
    }
    const bool four = dxt != 1 || c0 > c1;
    for (int ch = 0; ch < 3; ++ch) {
        const uint32_t a = c[0][ch], z = c[1][ch];
        if (four) {
            c[2][ch] = (2u * a + z + 1u) / 3u;
            c[3][ch] = (a + 2u * z + 1u) / 3u;
        } else {
            c[2][ch] = (a + z + 1u) / 2u;
            c[3][ch] = 0u;
        }
    }
    c[2][3] = 255u;
    c[3][3] = four ? 255u : 0u;
    const uint32_t ix = cb[4] | (static_cast<uint32_t>(cb[5]) << 8) |
                        (static_cast<uint32_t>(cb[6]) << 16) | (static_cast<uint32_t>(cb[7]) << 24);
    uint32_t al[8] = {};
    uint64_t ab = 0;   // DXT2 / 3: sixteen 4-bit alphas; DXT4 / 5: sixteen 3-bit indices.
    if (dxt == 2 || dxt == 3) {
        for (int i = 0; i < 8; ++i) ab |= static_cast<uint64_t>(b[i]) << (8 * i);
    } else if (dxt == 4 || dxt == 5) {
        const uint32_t a0 = b[0], a1 = b[1];
        al[0] = a0;
        al[1] = a1;
        if (a0 > a1) {
            for (uint32_t i = 2; i < 8; ++i)
                al[i] = ((8u - i) * a0 + (i - 1u) * a1 + 3u) / 7u;
        } else {
            for (uint32_t i = 2; i < 6; ++i)
                al[i] = ((6u - i) * a0 + (i - 1u) * a1 + 2u) / 5u;
            al[6] = 0u;
            al[7] = 255u;
        }
        for (int i = 0; i < 6; ++i) {
            ab |= static_cast<uint64_t>(b[2 + i]) << (8 * i);
        }
    }
    for (int i = 0; i < 16; ++i) {
        const uint32_t* const s = c[(ix >> (2 * i)) & 3u];
        px[i][0] = static_cast<uint8_t>(s[0]);
        px[i][1] = static_cast<uint8_t>(s[1]);
        px[i][2] = static_cast<uint8_t>(s[2]);
        uint32_t av = s[3];
        if (dxt == 2 || dxt == 3) av = static_cast<uint32_t>((ab >> (4 * i)) & 15u) * 17u;
        else if (dxt == 4 || dxt == 5) av = al[(ab >> (3 * i)) & 7u];
        px[i][3] = static_cast<uint8_t>(av);
    }
}
// The file as .dds bytes (a 128-byte header, then every level) for load_dds: FourCC DXT1 / DXT3 / DXT5 blocks, or
// 32-bit BGRA (PAA above). err says why on false. Never throws: a failed allocation is a failed file.
inline bool paa_to_dds(const std::vector<uint8_t>& f, std::vector<uint8_t>& dds, std::string& err) {
    dds.clear();
    try {
        const uint8_t* const b = f.data();
        const size_t n = f.size();
        auto u16 = [&](size_t o) {
            return static_cast<uint32_t>(b[o]) | (static_cast<uint32_t>(b[o + 1]) << 8);
        };
        if (n < 2) { err = "not a PAA file"; return false; }
        const uint32_t type = u16(0);
        int dxt = 0;          // 1 .. 5: DXTn.
        uint32_t bpp = 0;     // The other types' bytes per texel.
        switch (type) {
            case 0xFF01u: case 0xFF02u: case 0xFF03u: case 0xFF04u: case 0xFF05u:
                dxt = static_cast<int>(type & 0xFFu); break;
            case 0x4444u: case 0x1555u: case 0x8080u: bpp = 2; break;
            case 0x8888u: bpp = 4; break;
            default: {
                if (type == 0x4747u) {   // "GG": an untyped file's first tag - OFP's palette index.
                    err = "palette-indexed PAA (Operation Flashpoint) is not supported";
                } else {
                    static const char hx[] = "0123456789ABCDEF";
                    err = "unknown PAA type 0x";
                    for (int s = 12; s >= 0; s -= 4) err += hx[(type >> s) & 15u];
                }
                return false;
            }
        }
        size_t p = 2;
        uint8_t swz[4] = { 0, 1, 2, 3 };   // SWIZ as stored: the sources of A, R, G, B.
        while (n - p >= 4 && memcmp(b + p, "GGAT", 4) == 0) {
            if (n - p < 12) { err = "truncated PAA tag"; return false; }
            const uint32_t tl = u16(p + 8) | (u16(p + 10) << 16);
            if (tl > n - p - 12) { err = "truncated PAA tag"; return false; }
            if (tl == 4 && memcmp(b + p + 4, "ZIWS", 4) == 0) memcpy(swz, b + p + 12, 4);
            p += 12u + tl;
        }
        if (n - p < 2) { err = "truncated PAA header"; return false; }
        const size_t pal = static_cast<size_t>(u16(p)) * 3u;   // 0 for these types; skipped.
        p += 2;
        if (pal > n - p) { err = "truncated PAA palette"; return false; }
        p += pal;
        // The swizzle's inverse, per channel of the picture (A, R, G, B): the stored channel it came from, and
        // whether it was inverted (BIS.PAA's inverse). A colour channel no stored one came from keeps the stored one
        // in its place; an alpha none came from is opaque (a_one) - unless the stored alpha is the converter's
        // own constant 1 (SWIZ 8), which already is.
        int src[4] = { 0, 1, 2, 3 };
        bool neg[4] = { false, false, false, false };
        bool hit_a = false;
        for (int ch = 0; ch < 4; ++ch) {
            const uint32_t v = swz[ch];
            if (v >= 4u && v <= 7u) { src[v - 4u] = ch; neg[v - 4u] = true; }
            else if (v <= 3u) { src[v] = ch; neg[v] = false; }
            hit_a = hit_a || (v <= 7u && (v & 3u) == 0u);
        }
        const bool a_one = !hit_a && swz[0] != 8u;
        bool swz_id = !a_one;
        for (int ch = 0; ch < 4; ++ch) {
            swz_id = swz_id && src[ch] == ch && !neg[ch];
        }
        // PAA_NOVHQ: TexConvert's two-channel normal map (_novhq: R <- 1, G <- G, B <- 1, A <- 1 - R; SWIZ
        // 05 08 02 08) keeps X and Y and drops Z, storing a constant 1 in its place. A unit normal's Z is
        // sqrt(1 - X^2 - Y^2), so it is rebuilt per texel from the X and Y undone below (the decode of a two-channel
        // normal map); normalstrength then scales X and Y in the shader as for any normal map, so a _novhq shades
        // as the _nohq of the same picture does. Every other swizzle, a three-channel _nohq's stored Z among them,
        // is read as stored.
        static const uint8_t novhq[4] = { 5u, 8u, 2u, 8u };
        const bool zr = memcmp(swz, novhq, 4) == 0;
        struct PaaLevel { uint32_t w, h; bool lzo; size_t off, size; };
        PaaLevel lv[16];
        int nl = 0;
        while (nl < 16 && n - p >= 4) {
            uint32_t pa_w = u16(p);
            const uint32_t pa_h = u16(p + 2);
            if (pa_w == 0 && pa_h == 0) break;   // The end.
            const bool pa_lzo = dxt != 0 && (pa_w & 0x8000u) != 0;
            if (pa_lzo) pa_w &= 0x7FFFu;
            if (n - p < 7) break;
            const size_t sz = b[p + 4] | (static_cast<size_t>(b[p + 5]) << 8) |
                              (static_cast<size_t>(b[p + 6]) << 16);
            if (sz > n - p - 7) break;   // Truncated: the levels before it stand.
            lv[nl++] = { pa_w, pa_h, pa_lzo, p + 7, sz };
            p += 7u + sz;
        }
        if (nl == 0) { err = "PAA holds no mip level"; return false; }
        const uint32_t w0 = lv[0].w, h0 = lv[0].h;
        if (w0 == 0 || h0 == 0 || w0 > 16384 || h0 > 16384) {
            err = "bad PAA dimensions";
            return false;
        }
        // Blocks as stored only for the three plain DXT types, a level 0 a block format can take, and no swizzle to
        // undo; everything else is BGRA.
        const bool keep = (dxt == 1 || dxt == 3 || dxt == 5) && swz_id &&
                          (w0 & 3u) == 0 && (h0 & 3u) == 0;
        const uint32_t blk = dxt == 1 ? 8u : 16u;
        // The chain, before anything is allocated: each level must be the next of level 0's full chain (which ends
        // at 1 x 1), and its stored bytes must be able to hold its texels - as stored, or packed no tighter than the
        // format can (LZSS: 18 bytes from 2, 144 from 17 with the flag byte; LZO1X: under 256 from each input byte,
        // a run's zero bytes being 255 each). A 50-byte file claiming 16384 x 16384 fails here, not after a 1 GB
        // allocation. The levels that pass are what the body reserves, once level 0 has decoded.
        uint32_t full = 1;
        for (uint32_t m = w0 > h0 ? w0 : h0; m > 1u; m >>= 1) ++full;
        size_t rn[16] = {};
        size_t total = 128;   // The DDS header goes in front of the body: no copy at the end.
        int cl = 0;           // Levels in the chain.
        for (; cl < nl && static_cast<uint32_t>(cl) < full; ++cl) {
            const PaaLevel& v = lv[cl];
            const uint32_t ew = w0 >> cl ? w0 >> cl : 1u;
            const uint32_t eh = h0 >> cl ? h0 >> cl : 1u;
            if (v.w != ew || v.h != eh) break;   // Not the next level of the chain.
            const size_t n4 = ((ew + 3u) / 4u) * static_cast<size_t>((eh + 3u) / 4u);
            const size_t r = dxt ? n4 * blk : static_cast<size_t>(ew) * eh * bpp;
            const bool fits = dxt == 0 ? r / 9u <= v.size
                            : v.lzo    ? r / 256u <= v.size
                                                 : r <= v.size;
            if (!fits) break;
            rn[cl] = r;
            total += keep ? r : static_cast<size_t>(ew) * eh * 4u;
        }
        if (cl == 0) { err = "corrupt PAA level 0 data"; return false; }
        std::vector<uint8_t> raw;   // One level's texel data as stored, unpacked.
        std::vector<uint8_t> body(128);
        uint32_t mips = 0;
        for (int l = 0; l < cl; ++l) {
            const PaaLevel& v = lv[l];
            const uint32_t ew = w0 >> l ? w0 >> l : 1u;
            const uint32_t eh = h0 >> l ? h0 >> l : 1u;
            const size_t bw = (ew + 3u) / 4u, bh = (eh + 3u) / 4u;
            const size_t rl = rn[l];
            const uint8_t* const src_b = b + v.off;
            raw.resize(rl);
            bool ok;
            if (dxt == 0) ok = paa_unlzss(src_b, v.size, raw.data(), rl);
            else if (v.lzo) ok = paa_unlzo(src_b, v.size, raw.data(), rl);
            else { ok = true; memcpy(raw.data(), src_b, rl); }   // Size checked above.
            if (!ok) {
                if (l == 0) { err = "corrupt PAA level 0 data"; return false; }
                break;
            }
            if (l == 0) body.reserve(total);   // Keeps the header's 128 bytes; no reallocation after.
            if (keep) {
                body.insert(body.end(), raw.begin(), raw.end());
            } else {
                const size_t o0 = body.size();
                body.resize(o0 + static_cast<size_t>(ew) * eh * 4u);
                uint8_t* const d = body.data() + o0;   // BGRA, row by row.
                const uint8_t* const r = raw.data();
                if (dxt) {
                    uint8_t px[16][4];
                    for (size_t by = 0; by < bh; ++by) {
                        for (size_t bx = 0; bx < bw; ++bx) {
                            paa_bc_block(dxt, r + (by * bw + bx) * blk, px);
                            for (size_t i = 0; i < 16; ++i) {
                                const size_t x = bx * 4u + (i & 3u);
                                const size_t y = by * 4u + (i >> 2);
                                if (x >= ew || y >= eh) continue;
                                memcpy(d + (y * ew + x) * 4u, px[i], 4);
                            }
                        }
                    }
                } else {
                    const size_t np = static_cast<size_t>(ew) * eh;
                    for (size_t i = 0; i < np; ++i) {
                        uint8_t* const o = d + i * 4u;
                        if (type == 0x8888u) { memcpy(o, r + i * 4u, 4); continue; }
                        const uint32_t q = r[i * 2u] |
                                           (static_cast<uint32_t>(r[i * 2u + 1u]) << 8);
                        if (type == 0x4444u) {   // A4R4G4B4.
                            for (int k = 0; k < 4; ++k) {
                                o[k] = static_cast<uint8_t>(((q >> (4 * k)) & 15u) * 17u);
                            }
                        } else if (type == 0x1555u) {   // A1R5G5B5.
                            for (int k = 0; k < 3; ++k) {
                                const uint32_t c5 = (q >> (5 * k)) & 31u;
                                o[k] = static_cast<uint8_t>((c5 << 3) | (c5 >> 2));
                            }
                            o[3] = (q & 0x8000u) ? 255u : 0u;
                        } else {   // 8:8 grey, then alpha.
                            o[0] = o[1] = o[2] = static_cast<uint8_t>(q & 0xFFu);
                            o[3] = static_cast<uint8_t>(q >> 8);
                        }
                    }
                }
                const size_t np = static_cast<size_t>(ew) * eh;
                if (dxt == 2 || dxt == 4) {   // Premultiplied: the colour back over its alpha.
                    for (size_t i = 0; i < np; ++i) {
                        uint8_t* const o = d + i * 4u;
                        const uint32_t a = o[3];
                        if (a == 0 || a == 255) continue;
                        for (int k = 0; k < 3; ++k) {
                            const uint32_t u = (o[k] * 255u + a / 2u) / a;
                            o[k] = static_cast<uint8_t>(u > 255u ? 255u : u);
                        }
                    }
                }
                if (!swz_id) {   // BGRA bytes: A is byte 3, R 2, G 1, B 0.
                    for (size_t i = 0; i < np; ++i) {
                        uint8_t* const o = d + i * 4u;
                        const uint8_t s[4] = { o[0], o[1], o[2], o[3] };
                        for (int ch = 0; ch < 4; ++ch) {
                            const uint8_t sv = s[src[ch] == 0 ? 3 : 3 - src[ch]];
                            o[ch == 0 ? 3 : 3 - ch] = neg[ch]
                                                                    ? static_cast<uint8_t>(255u - sv) : sv;
                        }
                        if (a_one) o[3] = 255u;
                        if (zr) {   // PAA_NOVHQ: B from R and G (the shader's t * 2 - 1 both ways).
                            const float x = static_cast<float>(o[2]) / 127.5f - 1.0f;
                            const float y = static_cast<float>(o[1]) / 127.5f - 1.0f;
                            const float z2 = 1.0f - x * x - y * y;
                            const float z = z2 > 0.0f ? sqrtf(z2) : 0.0f;
                            const int zb = static_cast<int>((z + 1.0f) * 127.5f + 0.5f);
                            o[0] = static_cast<uint8_t>(zb > 255 ? 255 : zb);
                        }
                    }
                }
            }
            ++mips;
        }
        uint8_t* const hd = body.data();   // The 128 bytes reserved in front, zeroed.
        auto put32 = [&](size_t o, uint32_t x) {
            for (int k = 0; k < 4; ++k) {
                hd[o + k] = static_cast<uint8_t>(x >> (8 * k));
            }
        };
        put32(0, 0x20534444u);   // "DDS ".
        put32(4, 124u);
        put32(8, 0x1u | 0x2u | 0x4u | 0x1000u | 0x20000u);   // Caps, height, width, pixel format, mip count.
        put32(12, h0);
        put32(16, w0);
        put32(28, mips);
        put32(76, 32u);
        if (keep) {
            put32(80, 0x4u);   // FourCC.
            put32(84, dxt == 1 ? 0x31545844u : dxt == 3 ? 0x33545844u : 0x35545844u);
        } else {
            put32(80, 0x40u | 0x1u);   // RGB + alpha: B8G8R8A8.
            put32(88, 32u);
            put32(92, 0x00FF0000u);
            put32(96, 0x0000FF00u);
            put32(100, 0x000000FFu);
            put32(104, 0xFF000000u);
        }
        put32(108, 0x1000u | 0x400000u | 0x8u);   // Texture, mipmap, complex.
        dds.swap(body);
        return true;
    } catch (const std::exception&) {
        dds.clear();
        err = "out of memory";
        return false;
    }
}

// EA_DDS_X8 - a 32-bit file with no alpha mask (X8R8G8B8 / X8B8G8R8) has no alpha: its fourth byte is padding, and
// the DDS format reads such a file opaque. load_dds and dds_mip_fill create every 32-bit file with an A8 format,
// so that byte is set to 255 here, in every level load_dds would take (its 32-bit rule), before either reads the
// bytes: both .dds routes (the loader worker, the synchronous fallback) pass through dds_unblock first. A .paa's
// bytes always carry an alpha mask; any other file is left as it is. No allocation.
inline void dds_x8_opaque(std::vector<uint8_t>& f) {
    if (f.size() < 128) return;
    auto rd = [&](size_t o) {
        uint32_t v;
        memcpy(&v, f.data() + o, 4);
        return v;
    };
    if (rd(0) != 0x20534444u) return;
    const uint32_t h = rd(12), w = rd(16), pf = rd(80), rm = rd(92);
    if (w == 0 || h == 0 || w > 16384 || h > 16384) return;
    if ((pf & 0x4u) || !(pf & 0x40u) || rd(88) != 32u || rd(104) != 0u) return;
    if (rm != 0x00ff0000u && rm != 0x000000ffu) return;
    uint32_t mips = rd(28);   // DDS_MIPS: load_dds's count, a short chain where the file ends.
    if (mips == 0) mips = 1;
    uint32_t chain = 1;
    for (uint32_t s = w > h ? w : h; s > 1; s >>= 1) ++chain;
    if (mips > chain) mips = chain;
    size_t end = 128;
    for (uint32_t m = 0; m < mips; ++m) {
        const size_t mw = w >> m ? w >> m : 1u;
        const size_t mh = h >> m ? h >> m : 1u;
        const size_t n = mw * 4u * mh;
        if (end + n > f.size()) break;
        end += n;
    }
    for (size_t i = 128u + 3u; i < end; i += 4u) f[i] = 0xFFu;
}
// DDS_BLOCK4: Direct3D creates a block-compressed texture only when its full-size level is a multiple of 4 pixels on
// a side. A BC1 / BC2 / BC3 .dds that is not (a 2 x 2, a 6 x 10) is rewritten here, before load_dds (on the loader
// worker, or in the synchronous fallback), as the 32-bit BGRA .dds paa_to_dds makes of such a .paa: every level the
// file holds, decoded by paa_bc_block (Direct3D's decode). Such a BC7 file, which has no decoder here, is refused
// with the reason (CreateTexture2D would give E_INVALIDARG). Every other file is left as it is - one whose header or
// level 0 load_dds refuses too, so its report stands (an X8 file's padding byte aside: dds_x8_opaque, first).
// False: err says why, f unchanged. Never throws: a failed allocation is a failed file.
inline bool dds_unblock(std::vector<uint8_t>& f, std::string& err) {
    dds_x8_opaque(f);   // EA_DDS_X8.
    try {
        auto rd = [&](size_t o) {
            uint32_t v;
            memcpy(&v, f.data() + o, 4);
            return v;
        };
        if (f.size() < 128 || rd(0) != 0x20534444u) return true;
        const uint32_t h = rd(12), w = rd(16);
        if (w == 0 || h == 0 || w > 16384 || h > 16384) return true;
        if ((w & 3u) == 0u && (h & 3u) == 0u) return true;   // Blocks Direct3D takes as they are.
        // load_dds's format rule, its block formats only.
        const uint32_t pf = rd(80), cc = rd(84);
        int dxt = 0;   // 1 / 3 / 5: DXT1 / DXT3 / DXT5 (BC1 / BC2 / BC3); 7: BC7.
        size_t off = 128;
        if ((pf & 0x4u) && cc == 0x30315844u) {   // "DX10".
            if (f.size() < 148) return true;
            const uint32_t dxgi = rd(128);
            off = 148;
            if (dxgi == 71 || dxgi == 72) dxt = 1;
            else if (dxgi == 74 || dxgi == 75) dxt = 3;
            else if (dxgi == 77 || dxgi == 78) dxt = 5;
            else if (dxgi == 98 || dxgi == 99) dxt = 7;
        } else if (pf & 0x4u) {
            if (cc == 0x31545844u) dxt = 1;
            else if (cc == 0x33545844u) dxt = 3;
            else if (cc == 0x35545844u) dxt = 5;
        }
        if (dxt == 0) return true;
        if (dxt == 7) {
            err = "a BC7 .dds must be a multiple of 4 pixels on a side (" + std::to_string(w) + " x " +
                  std::to_string(h) + ")";
            return false;
        }
        // The levels load_dds would take: its count rule (DDS_MIPS - its cap of 16 never binds below the
        // chain's 15), a short chain where the file ends.
        uint32_t mips = rd(28);
        if (mips == 0) mips = 1;
        uint32_t chain = 1;
        for (uint32_t s = w > h ? w : h; s > 1; s >>= 1) ++chain;
        if (mips > chain) mips = chain;
        const size_t blk = dxt == 1 ? 8u : 16u;
        size_t src[16] = {};   // Each level's blocks in the file.
        size_t out = 128;      // The BGRA file's size.
        for (uint32_t m = 0; m < mips; ++m) {
            const size_t mw = w >> m ? w >> m : 1u;
            const size_t mh = h >> m ? h >> m : 1u;
            const size_t n = (mw + 3u) / 4u * blk * ((mh + 3u) / 4u);
            if (off + n > f.size()) {
                if (m == 0) return true;   // load_dds: "truncated pixel data".
                mips = m;
                break;
            }
            src[m] = off;
            off += n;
            out += mw * mh * 4u;
        }
        std::vector<uint8_t> d(out, 0);
        auto put = [&](size_t o, uint32_t x) { memcpy(d.data() + o, &x, 4); };
        put(0, 0x20534444u);   // "DDS " - paa_to_dds's 32-bit header.
        put(4, 124u);
        put(8, 0x1u | 0x2u | 0x4u | 0x1000u | 0x20000u);   // Caps, height, width, pixel format, mip count.
        put(12, h);
        put(16, w);
        put(28, mips);
        put(76, 32u);
        put(80, 0x40u | 0x1u);   // RGB + alpha: B8G8R8A8.
        put(88, 32u);
        put(92, 0x00FF0000u);
        put(96, 0x0000FF00u);
        put(100, 0x000000FFu);
        put(104, 0xFF000000u);
        put(108, 0x1000u | 0x400000u | 0x8u);   // Texture, mipmap, complex.
        size_t at = 128;
        uint8_t t[16][4];
        for (uint32_t m = 0; m < mips; ++m) {
            const size_t mw = w >> m ? w >> m : 1u;
            const size_t mh = h >> m ? h >> m : 1u;
            const size_t bw = (mw + 3u) / 4u, bh = (mh + 3u) / 4u;
            const uint8_t* const b = f.data() + src[m];
            uint8_t* const px = d.data() + at;
            for (size_t by = 0; by < bh; ++by) {
                for (size_t bx = 0; bx < bw; ++bx) {
                    paa_bc_block(dxt, b + (by * bw + bx) * blk, t);
                    for (size_t i = 0; i < 16; ++i) {   // The block's texels inside the level.
                        const size_t x = bx * 4u + (i & 3u), y = by * 4u + (i >> 2);
                        if (x >= mw || y >= mh) continue;
                        memcpy(px + (y * mw + x) * 4u, t[i], 4);
                    }
                }
            }
            at += mw * mh * 4u;
        }
        f.swap(d);
        return true;
    } catch (const std::exception&) {
        err = "out of memory";
        return false;
    }
}

// An entry's page layer as a ref; an entry without a live page (pending, failed, or its page freed) gives the empty ref
// (srv nullptr).
inline TexRef tex_ref_of(const TexEntry& e) {
    TexRef r;
    if (e.page >= 0 && static_cast<size_t>(e.page) < g_tex_pages.size() && g_tex_pages[e.page].tex) {
        r.srv = g_tex_pages[e.page].srv;
        r.layer = e.layer;
    }
    return r;
}

// Cache key: lower-cased path + colour space (a map may be bound in both). tex_slot_srgb is the per-slot rule:
// diffuse / emissive / specular / speccolor are colour data, normal / orm linear - except that the arma model's
// specular slot holds SMDI, whose channels are data, so it is linear there (as is a .hlsl material's when the script
// named it "smdi": tex_map_srgb).
inline std::string tex_cache_key(const std::string& path, bool srgb) {
    std::string key = path;
    key = kh_lower_copy(std::move(key));
    key += srgb ? "|s" : "|l";
    return key;
}
inline bool tex_slot_srgb(int shader, int slot) {
    if (slot == 4) return shader != 2;
    return slot == 0 || slot == 3 || slot == 5;
}
// A material map's colour space: tex_slot_srgb's rule for the builtin slots, the script's own choice for
// USER_TEX's user slots.
inline bool tex_map_srgb(const Material& m, int slot) {
    if (slot >= MAT_MAPS_BUILTIN) return m.maps[slot].srgb;
    if (slot == 4 && m.shader == 1 && m.map_smdi) return false;   // USER_SLOTS: SMDI is data.
    return tex_slot_srgb(m.shader, slot);
}

// TEX_WARN_CAP: a texture's failure, reported under the family's cap. Counted only when the text is
// new to the session (a device reset re-resolves every failed texture; the same path can be asked as
// sRGB and as linear): the count is of distinct failures, as g_chain_warned's is of distinct keys.
inline void tex_report(const std::string& path, const std::string& err) {
    const std::string msg = "KH RenderIntegration: texture '" + path + "': " + err;
    {
        std::lock_guard<std::mutex> g(g_err_once_mutex);
        if (std::find(g_err_once.begin(), g_err_once.end(), msg) != g_err_once.end()) return;
    }
    const uint32_t n = g_tex_warned.fetch_add(1u, std::memory_order_relaxed);
    if (n < 64u) {
        report_error_once_safe(msg);
    } else if (n == 64u) {
        // EA_ERR_CLASS: a notice, not an error - logged, never thrown.
        log_once_safe("KH RenderIntegration: texture: more than 64 textures failed this session; later ones are "
                      "not reported");
    }
}

// Cache front door: resolved path -> page layer (queued to the loader, or loaded here if it cannot start, on the first
// miss). Serialized-window contract above; the empty ref = missing / pending / failed (latched).
inline TexRef tex_resolve(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                          const std::string& path, bool srgb) {
    if (path.empty() || !dev || !ctx) return TexRef();
    const std::string key = tex_cache_key(path, srgb);
    auto it = g_tex_cache.find(key);
    if (it != g_tex_cache.end()) { it->second.last_use_ms = GetTickCount64(); return tex_ref_of(it->second); }

    // A first sight is a disk read + decode + mip build: hand it to the loader worker and answer 'absent' (the material
    // draws defaults for a frame or two; tex_async_pump places finished results at the pass heads and the
    // page-generation bump refills the table). A pending entry has no page and failed clear: a re-resolve refreshes it
    // without re-queueing, the GC ages it (an entry it erased drops the late result by key miss), a device reset keeps
    // it (TEX_RESET_KEEP_PENDING) and the mission end stops the loader before it clears. A loader that cannot
    // start falls through to the synchronous path.
    if (texldr_queue(key, path, srgb, false)) {
        TexEntry p;
        p.last_use_ms = GetTickCount64();
        g_tex_cache[key] = p;
        return TexRef();
    }

    TexEntry e;
    e.last_use_ms = GetTickCount64();
    std::string err;
    ID3D11ShaderResourceView* single = nullptr;

    if (ends_with_ci(path, ".dds")) {
        std::vector<uint8_t> bytes;
        // PBO_PATH. EA_TEX_READ_WHY: a failed read says why in err (the loader worker's twin).
        if (asset_read(path, bytes, 256ull * 1024u * 1024u, &err) && dds_unblock(bytes, err)) {
            single = dds_create(dev, bytes, srgb, err);   // DDS_MIP_FILL.
        }
    } else if (paa_ext(path)) {   // PAA: made into .dds bytes, then the .dds route.
        std::vector<uint8_t> paa, dds;
        if (asset_read(path, paa, 256ull * 1024u * 1024u, &err) &&   // As above.
            paa_to_dds(paa, dds, err)) {
            single = dds_create(dev, dds, srgb, err);
        }
    } else {
        single = tex_load_cached(dev, path, srgb, err);   // STB_DEAD_TAIL.
    }

    if (single) {   // MAT_PAGES: into a page layer; the single view goes.
        if (!tex_page_place(dev, ctx, single, e)) err = "texture page placement failed";
        single->Release();
    }

    if (e.page < 0) {
        e.failed = true;
        if (ends_with_ci(path, ".tga")) {
            err += " - if the file opens elsewhere, re-export as uncompressed true-color 24/32-bit TGA";
        }
        tex_report(path, err);   // TEX_WARN_CAP.
    }

    g_tex_cache[key] = e;
    return tex_ref_of(e);
}
}