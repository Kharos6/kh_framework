#pragma once

// The feature half (rendering_integration_features.hpp): the texture disk cache: the file format, the mip builder, load
// and store. Never include it on its own.
namespace RenderIntegration {

static constexpr uint32_t TEX_CACHE_MAGIC = 0x4354484Bu;   // "KHTC" little-endian.
// CACHE_LIB_VER: a .khtc holds stb_image's decode and tex_build_mips' chain, and stb_image carries no usable
// version (STBI_VERSION is 1 in every release) - updating stb_image.h or changing the mip builder needs a bump here by
// hand.
static constexpr uint32_t TEX_CACHE_VERSION = 2;
static constexpr uint32_t TEX_CACHE_MAX_MIPS = 15;

inline std::filesystem::path tex_cache_file(uint64_t hash) {
    return mesh_cache_dir() / cache_file_name("kht_", hash, ".khtc");
}
inline uint64_t tex_hash(const std::vector<uint8_t>& bytes, bool srgb) {
    uint64_t h = CryptoGenerator::fnv1a64_raw(bytes.data(), bytes.size());
    if (srgb) h ^= 0x9E3779B97F4A7C15ull;   // The two colour spaces are two textures.
    return h;
}
// sRGB <-> linear tables: 256 -> float for the decode, 4096 -> byte for the encode. SRGB_TABLE_ONCE: each built by
// its function-local static's initialiser, which C++ runs once and thread-safely. Do not build them behind a plain
// bool: two decoding threads could race the fill, and the flag could be seen set before the table was - a half-filled
// table baked into a .khtc for good.
struct TexS2l { float t[256]; };
struct TexL2s { uint8_t t[4096]; };
inline const float* tex_s2l() {
    static const TexS2l tab = [] {
        TexS2l r;
        float* const t = r.t;
        for (int i = 0; i < 256; ++i) {
            const float c = i / 255.0f;
            t[i] = c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
        }
        return r;
    }();
    return tab.t;
}
inline const uint8_t* tex_l2s() {
    static const TexL2s tab = [] {
        TexL2s r;
        uint8_t* const t = r.t;
        for (int i = 0; i < 4096; ++i) {
            const float l = i / 4095.0f;
            const float s = l <= 0.0031308f ? l * 12.92f : 1.055f * powf(l, 1.0f / 2.4f) - 0.055f;
            int v = static_cast<int>(s * 255.0f + 0.5f);
            t[i] = static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v);
        }
        return r;
    }();
    return tab.t;
}
// MIP_ODD - one level from the one above it where an edge is odd: the polyphase box (Guthe & Heckbert,
// "Non-Power-of-Two Mipmap Creation", NVIDIA NVR-2005-002; NVIDIA Texture Tools' box). An odd edge 2m + 1 halves
// to m, each texel the exact area average of the (2m + 1) / m source texels it covers: taps 2x, 2x + 1, 2x + 2
// weighted (m - x, m, x + 1) / (2m + 1) - so no source texel is dropped and every one counts by the area it
// gives. An even edge keeps its two taps at 1/2; an edge of one, its one tap. Separable: the 2D weight is the
// product. Colour in linear light for an sRGB level (the 2x2 path's tables), alpha and linear data as stored;
// each rounded to nearest.
inline void tex_mip_poly(const uint8_t* s, uint32_t sw, uint32_t sh, uint8_t* d,
                         uint32_t dw, uint32_t dh, bool srgb, const float* s2l,
                         const uint8_t* l2s) {
    // An axis's taps for one destination texel: source index and weight, up to three.
    auto taps = [](uint32_t n, uint32_t i, uint32_t* ix, float* w) -> int {
        if (n <= 1u) { ix[0] = 0u; w[0] = 1.0f; return 1; }
        if ((n & 1u) == 0u) {
            ix[0] = 2u * i; ix[1] = 2u * i + 1u;
            w[0] = 0.5f; w[1] = 0.5f;
            return 2;
        }
        const uint32_t m = n >> 1;
        const float r = 1.0f / static_cast<float>(n);
        ix[0] = 2u * i; ix[1] = 2u * i + 1u; ix[2] = 2u * i + 2u;
        w[0] = static_cast<float>(m - i) * r;
        w[1] = static_cast<float>(m) * r;
        w[2] = static_cast<float>(i + 1u) * r;
        return 3;
    };
    for (uint32_t y = 0; y < dh; ++y) {
        uint32_t yi[3];
        float yw[3];
        const int ny = taps(sh, y, yi, yw);
        for (uint32_t x = 0; x < dw; ++x) {
            uint32_t xi[3];
            float xw[3];
            const int nx = taps(sw, x, xi, xw);
            float acc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            for (int j = 0; j < ny; ++j) {
                for (int i = 0; i < nx; ++i) {
                    const float w = yw[j] * xw[i];
                    const uint8_t* p = s + (static_cast<size_t>(yi[j]) * sw +
                                            xi[i]) * 4u;
                    for (int c = 0; c < 3; ++c) {
                        acc[c] += w * (srgb ? s2l[p[c]]
                                                                : static_cast<float>(p[c]));
                    }
                    acc[3] += w * static_cast<float>(p[3]);
                }
            }
            uint8_t* o = d + (static_cast<size_t>(y) * dw + x) * 4u;
            for (int c = 0; c < 4; ++c) {
                if (srgb && c < 3) {
                    const int q = static_cast<int>(acc[c] * 4095.0f + 0.5f);
                    o[c] = l2s[q < 0 ? 0 : q > 4095 ? 4095 : q];
                } else {
                    const int v = static_cast<int>(acc[c] + 0.5f);
                    o[c] = static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v);
                }
            }
        }
    }
}
// Full mip chain from level 0 (RGBA8). 2x2 box where both edges are even (or one); a level with an odd edge (above one)
// takes the polyphase box instead (MIP_ODD) - the 2x2 box would drop that edge's last texel.
inline void tex_build_mips(TexBlob& b, const uint8_t* px, uint32_t w, uint32_t h, bool srgb) {
    b.w = w; b.h = h; b.srgb = srgb;
    b.level_off.clear();
    b.data.clear();
    uint32_t lw = w, lh = h, n = 0;
    size_t total = 0;
    for (;;) {
        total += static_cast<size_t>(lw) * lh * 4u;
        ++n;
        if ((lw == 1 && lh == 1) || n >= TEX_CACHE_MAX_MIPS) break;
        lw = lw > 1 ? lw >> 1 : 1;
        lh = lh > 1 ? lh >> 1 : 1;
    }
    b.mips = n;
    b.data.resize(total);
    b.level_off.resize(n);
    memcpy(b.data.data(), px, static_cast<size_t>(w) * h * 4u);
    b.level_off[0] = 0;
    const float* s2l = tex_s2l();
    const uint8_t* l2s = tex_l2s();
    uint32_t sw = w, sh = h;
    size_t soff = 0;
    for (uint32_t l = 1; l < n; ++l) {
        const uint32_t dw = sw > 1 ? sw >> 1 : 1;
        const uint32_t dh = sh > 1 ? sh >> 1 : 1;
        const size_t doff = soff + static_cast<size_t>(sw) * sh * 4u;
        b.level_off[l] = static_cast<uint32_t>(doff);
        const uint8_t* s = b.data.data() + soff;
        uint8_t* d = b.data.data() + doff;
        if ((sw > 1u && (sw & 1u) != 0u) || (sh > 1u && (sh & 1u) != 0u)) {   // MIP_ODD.
            tex_mip_poly(s, sw, sh, d, dw, dh, srgb, s2l, l2s);
            soff = doff;
            sw = dw; sh = dh;
            continue;
        }
        for (uint32_t y = 0; y < dh; ++y) {
            const uint32_t y0 = y * 2u < sh ? y * 2u : sh - 1u;
            const uint32_t y1 = y0 + 1u < sh ? y0 + 1u : sh - 1u;
            for (uint32_t x = 0; x < dw; ++x) {
                const uint32_t x0 = x * 2u < sw ? x * 2u : sw - 1u;
                const uint32_t x1 = x0 + 1u < sw ? x0 + 1u : sw - 1u;
                const uint8_t* p[4] = {
                    s + (static_cast<size_t>(y0) * sw + x0) * 4u,
                    s + (static_cast<size_t>(y0) * sw + x1) * 4u,
                    s + (static_cast<size_t>(y1) * sw + x0) * 4u,
                    s + (static_cast<size_t>(y1) * sw + x1) * 4u };
                uint8_t* o = d + (static_cast<size_t>(y) * dw + x) * 4u;
                if (srgb) {
                    for (int c = 0; c < 3; ++c) {
                        const float lin = 0.25f * (s2l[p[0][c]] + s2l[p[1][c]] +
                                                   s2l[p[2][c]] + s2l[p[3][c]]);
                        int q = static_cast<int>(lin * 4095.0f + 0.5f);
                        o[c] = l2s[q < 0 ? 0 : q > 4095 ? 4095 : q];
                    }
                } else {
                    for (int c = 0; c < 3; ++c) {
                        o[c] = static_cast<uint8_t>((p[0][c] + p[1][c] +
                                                     p[2][c] + p[3][c] + 2u) >> 2);
                    }
                }
                o[3] = static_cast<uint8_t>((p[0][3] + p[1][3] + p[2][3] + p[3][3] + 2u) >> 2);
            }
        }
        soff = doff;
        sw = dw; sh = dh;
    }
}
// DDS_MIP_FILL - a 32-bit RGBA / BGRA .dds (or a .paa made into one, PAA; or a BC file DDS_BLOCK4 decoded)
// that holds fewer levels than its full chain to 1 x 1 - a header that says 0 or 1 (a common exporter default), or a
// chain cut short. Created with only those, it has no smaller level to sample and aliases (shimmers) at every distance
// past its smallest level's one texel per pixel. Its levels are kept as RGBA8 (BGRA swapped, alpha as stored) and the
// chain goes on from its smallest with the stb route's builder (tex_build_mips: the 2x2 / polyphase box, colour in
// linear light for an sRGB map), created as that route's (tex_create; same colour space). A file with no chain gets
// it all; one cut short only the levels it lacks. BC1 / BC2 / BC3 / BC7 blocks are left as they are (no BC encoder
// here: such a file keeps only the levels it holds). The header is read as load_dds reads it; false = not such a
// file (or no memory): the bytes are created unchanged. Not disk-cached (the .khtc holds only the stb route's); the
// cost is the build at each first sight, on the loader thread (or in the synchronous fallback's dds_create).
inline bool dds_mip_fill(const std::vector<uint8_t>& f, bool srgb, std::shared_ptr<TexBlob>& out) {
    auto rd32 = [&](size_t off) { uint32_t v; memcpy(&v, f.data() + off, 4); return v; };
    if (f.size() < 128 || rd32(0) != 0x20534444u) return false;
    const uint32_t height = rd32(12), width = rd32(16);
    uint32_t mips = rd32(28);
    const uint32_t pf_flags = rd32(80), fourcc = rd32(84);
    const uint32_t bitcount = rd32(88), rmask = rd32(92);
    if (width == 0 || height == 0 || width > 16384 || height > 16384) return false;
    if (mips == 0) mips = 1;
    if (mips > 16) mips = 16;
    uint32_t chain = 1;
    for (uint32_t s = width > height ? width : height; s > 1; s >>= 1) ++chain;
    if (mips > chain) mips = chain;
    bool bgra = false;
    size_t data_off = 128;
    if ((pf_flags & 0x4u) && fourcc == 0x30315844u) {   // DX10.
        if (f.size() < 148) return false;
        const uint32_t dxgi = rd32(128);
        data_off = 148;
        if (dxgi == 28u || dxgi == 29u) bgra = false;
        else if (dxgi == 87u || dxgi == 91u) bgra = true;
        else return false;   // BC, or load_dds's refusal.
    } else if (!(pf_flags & 0x4u) && (pf_flags & 0x40u) && bitcount == 32u) {
        if (rmask == 0x00ff0000u) bgra = true;
        else if (rmask == 0x000000ffu) bgra = false;
        else return false;
    } else {
        return false;
    }
    uint32_t held = 0;   // The levels the file holds, as load_dds counts them.
    size_t off = data_off;
    for (uint32_t m = 0; m < mips; ++m) {
        const uint32_t mw = width >> m ? width >> m : 1;
        const uint32_t mh = height >> m ? height >> m : 1;
        const size_t bytes = static_cast<size_t>(mw) * 4u * mh;
        if (off + bytes > f.size()) break;
        off += bytes;
        ++held;
    }
    if (held == 0u || held >= chain) return false;   // No level 0 (load_dds says so), or whole.
    try {
        // The file's levels, tight-packed as the blob's are: RGBA8. Reserved for the whole chain (the levels the
        // tail adds below the smallest held one), so the tail's insert below does not reallocate a copy of it.
        size_t cap = off - data_off;
        for (uint32_t cw = width >> (held - 1u) ? width >> (held - 1u) : 1u,
                      ch = height >> (held - 1u) ? height >> (held - 1u) : 1u;
             cw > 1u || ch > 1u;) {
            cw = cw > 1u ? cw >> 1 : 1u;
            ch = ch > 1u ? ch >> 1 : 1u;
            cap += static_cast<size_t>(cw) * 4u * ch;
        }
        std::vector<uint8_t> px;
        px.reserve(cap);
        px.assign(f.begin() + static_cast<std::ptrdiff_t>(data_off), f.begin() + static_cast<std::ptrdiff_t>(off));
        if (bgra) {
            for (size_t i = 0; i + 3u < px.size(); i += 4u) {
                std::swap(px[i], px[i + 2u]);
            }
        }
        std::shared_ptr<TexBlob> b = std::make_shared<TexBlob>();
        b->w = width; b->h = height; b->srgb = srgb;
        size_t o = 0;
        for (uint32_t m = 0; m < held; ++m) {
            const uint32_t mw = width >> m ? width >> m : 1;
            const uint32_t mh = height >> m ? height >> m : 1;
            b->level_off.push_back(static_cast<uint32_t>(o));
            o += static_cast<size_t>(mw) * 4u * mh;
        }
        // The chain from the smallest held level on: its own level 0 is that level, so its levels 1 .. follow.
        const uint32_t lw = width >> (held - 1u) ? width >> (held - 1u) : 1u;
        const uint32_t lh = height >> (held - 1u) ? height >> (held - 1u) : 1u;
        TexBlob tail;
        tex_build_mips(tail, px.data() + b->level_off[held - 1u], lw, lh, srgb);
        b->data = std::move(px);
        const size_t t1 = tail.level_off.size() > 1u ? tail.level_off[1] : tail.data.size();
        for (uint32_t m = 1; m < tail.mips; ++m) {
            b->level_off.push_back(static_cast<uint32_t>(o + (tail.level_off[m] - t1)));
        }
        b->data.insert(b->data.end(), tail.data.begin() + static_cast<std::ptrdiff_t>(t1),
                       tail.data.end());
        b->mips = static_cast<uint32_t>(b->level_off.size());
        out = std::move(b);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}
// Full-validation load (any structural doubt is a miss). own = the Documents copy: lru-touched, under the trim
// budget.
inline bool tex_cache_load_one(const std::filesystem::path& p, bool own,
                               uint64_t hash, bool srgb, TexBlob& b) {
    try {
        std::ifstream f(p, std::ios::binary | std::ios::ate);
        if (!f) return false;
        const std::streamsize n = f.tellg();
        if (n < 36) return false;
        f.seekg(0);
        uint32_t hd[7] = {};   // Magic, version, hash lo, hash hi, w, h, mips.
        uint32_t flags = 0;
        f.read(reinterpret_cast<char*>(hd), sizeof(hd));
        f.read(reinterpret_cast<char*>(&flags), 4);
        if (!f) return false;
        const uint64_t fh = (static_cast<uint64_t>(hd[3]) << 32) | hd[2];
        if (hd[0] != TEX_CACHE_MAGIC || hd[1] != TEX_CACHE_VERSION || fh != hash) return false;
        const uint32_t w = hd[4], h = hd[5], m = hd[6];
        if (w == 0 || h == 0 || w > 16384 || h > 16384 || m == 0 || m > TEX_CACHE_MAX_MIPS) return false;
        if (((flags & 1u) != 0u) != srgb) return false;
        size_t total = 0;
        uint32_t lw = w, lh = h;
        std::vector<uint32_t> off(m);
        for (uint32_t l = 0; l < m; ++l) {
            off[l] = static_cast<uint32_t>(total);
            total += static_cast<size_t>(lw) * lh * 4u;
            lw = lw > 1 ? lw >> 1 : 1;
            lh = lh > 1 ? lh >> 1 : 1;
        }
        if (static_cast<size_t>(n) != 32u + total) return false;
        b.data.resize(total);
        f.read(reinterpret_cast<char*>(b.data.data()), static_cast<std::streamsize>(total));
        if (!f) return false;
        f.close();
        b.w = w; b.h = h; b.mips = m; b.srgb = srgb;
        b.level_off = std::move(off);
        if (own) {
            std::error_code ec;
            std::filesystem::last_write_time(p, std::filesystem::file_time_type::clock::now(), ec);
        }
        return true;
    } catch (...) { return false; }
}
inline bool tex_cache_load(uint64_t hash, bool srgb, TexBlob& b) {
    std::vector<std::filesystem::path> cand;
    const bool docs = !mesh_cache_dir().empty();
    if (docs) cand.push_back(tex_cache_file(hash));
    mod_cache_append(cand, cache_file_name("kht_", hash, ".khtc"));
    for (size_t i = 0; i < cand.size(); ++i) {
        const bool own = i == 0 && docs;
        if (tex_cache_load_one(cand[i], own, hash, srgb, b)) {
            return true;
        }
    }
    return false;
}
inline void tex_cache_store(uint64_t hash, const TexBlob& b) {
    if (mesh_cache_dir().empty()) return;
    if (b.data.empty() || b.mips == 0) return;
    std::filesystem::path tmp;   // CACHE_TMP_WRITE.
    try {
        const std::filesystem::path p = tex_cache_file(hash);
        tmp = cache_tmp_of(p);
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return;
        const uint32_t hd[7] = { TEX_CACHE_MAGIC, TEX_CACHE_VERSION,
                                 static_cast<uint32_t>(hash & 0xFFFFFFFFu),
                                 static_cast<uint32_t>(hash >> 32),
                                 b.w, b.h, b.mips };
        const uint32_t flags = b.srgb ? 1u : 0u;
        f.write(reinterpret_cast<const char*>(hd), sizeof(hd));
        f.write(reinterpret_cast<const char*>(&flags), 4);
        f.write(reinterpret_cast<const char*>(b.data.data()), static_cast<std::streamsize>(b.data.size()));
        cache_tmp_commit(f, tmp, p);   // CACHE_TMP_WRITE.
    } catch (...) {
        std::error_code ec;   // CACHE_TMP_WRITE: a throw mid-write leaves no temp behind.
        if (!tmp.empty()) std::filesystem::remove(tmp, ec);
    }
}
inline void tex_cache_queue(uint64_t hash, std::shared_ptr<const TexBlob> b) {
    if (!mesh_cache_writer_start()) {   // No thread: write inline, the mesh cache's own
                                           // fallback.
        tex_cache_store(hash, *b);
        std::lock_guard<std::mutex> tl(g_shpool_trim_mu);
        mesh_cache_trim();
        return;
    }
    {
        std::lock_guard<std::mutex> l(g_meshwriter_mu);
        MeshWriteJob j = { hash, -1, std::move(b) };
        g_meshwriter_q.push_back(std::move(j));
    }
    g_meshwriter_cv.notify_one();
}
// Immutable upload, every level in one create - no GenerateMips, no RT bind, no context needed. hro: the failing
// call's HRESULT (TEX_RETRY_INVALID).
inline ID3D11ShaderResourceView* tex_create(ID3D11Device* dev, const TexBlob& b, std::string& err,
                                            HRESULT* hro = nullptr) {
    if (hro) *hro = S_OK;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = b.w; td.Height = b.h;
    td.MipLevels = b.mips; td.ArraySize = 1;
    td.Format = b.srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    std::vector<D3D11_SUBRESOURCE_DATA> init(b.mips);
    uint32_t lw = b.w;
    for (uint32_t l = 0; l < b.mips; ++l) {
        init[l].pSysMem = b.data.data() + b.level_off[l];
        init[l].SysMemPitch = lw * 4u;
        init[l].SysMemSlicePitch = 0;
        lw = lw > 1 ? lw >> 1 : 1;
    }
    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = dev->CreateTexture2D(&td, init.data(), &tex);
    if (FAILED(hr)) {
        if (hro) *hro = hr;
        err = "CreateTexture2D(mipped) " + hr_str(hr);
        return nullptr;
    }
    ID3D11ShaderResourceView* srv = nullptr;
    hr = dev->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    if (FAILED(hr)) {
        if (hro) *hro = hr;
        err = "CreateSRV(mipped) " + hr_str(hr);
        return nullptr;
    }
    return srv;
}
// TEX_RETRY_INVALID - the corrupt-cache retry (tex_load_cached, tex_async_pump) decodes the source again when
// a blob read from the disk cache fails to create. The reader passes only a file of the exact size its header
// describes, so a damaged file reaching the create can only fail it by its description (a level count past its chain:
// E_INVALIDARG); every other failure - out of memory with VRAM full, a removed or reset device - fails the fresh
// decode's create the same way, so a retry of it gains nothing and costs a source read, a decode and a cache rewrite
// first: only E_INVALIDARG retries.
inline bool tex_cache_retry_worth(HRESULT hr) { return hr == E_INVALIDARG; }
// DDS_MIP_FILL: the synchronous route's .dds / .paa create - the chain built where the file lacks it, else
// the file's own levels (load_dds).
inline ID3D11ShaderResourceView* dds_create(ID3D11Device* dev, const std::vector<uint8_t>& f, bool srgb,
                                            std::string& err) {
    std::shared_ptr<TexBlob> b;
    if (dds_mip_fill(f, srgb, b)) return tex_create(dev, *b, err);
    return load_dds(dev, f, srgb, err);
}
// CPU half of the first-sight load: disk read, content hash, disk-cache probe, else decode + CPU mips (+ cache-store
// queue). No device, no context, so the loader worker runs exactly this. force skips the probe (the corrupt-cache
// retry); from_cache tells the caller which arm produced the blob.
inline bool tex_blob_load(const std::string& path, bool srgb,
                          std::shared_ptr<TexBlob>& out, bool& from_cache,
                          std::string& err, bool force) {
    from_cache = false;
    std::vector<uint8_t> bytes;
    if (!asset_read(path, bytes, 512ull * 1024u * 1024u, &err)) return false;   // PBO_PATH; EA_TEX_READ_WHY.
    const uint64_t hash = tex_hash(bytes, srgb);
    if (!force) {
        // TEXLDR_GUARD: a failed allocation is a failed load, not a throw into the caller.
        try {
            auto hit = std::make_shared<TexBlob>();
            if (tex_cache_load(hash, srgb, *hit)) {
                out = std::move(hit);
                from_cache = true;
                return true;
            }
        } catch (const std::bad_alloc&) {
            err = "out of memory";
            return false;
        }
    }
    int w = 0, h = 0, comp = 0;
    stbi_uc* px = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()),
                                        &w, &h, &comp, 4);
    if (!px) { err = std::string("decode failed: ") + (stbi_failure_reason() ? stbi_failure_reason() : "?"); return false; }
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384) { stbi_image_free(px); err = "bad dimensions"; return false; }
    // TEXLDR_GUARD: the mip chain of a 16384^2 image is ~1.4 GB on top of the decode; a failed
    // allocation frees the pixels and fails the load (the loader thread and the sync path alike).
    std::shared_ptr<TexBlob> blob;
    try {
        blob = std::make_shared<TexBlob>();
        tex_build_mips(*blob, px, static_cast<uint32_t>(w), static_cast<uint32_t>(h), srgb);
    } catch (const std::bad_alloc&) {
        stbi_image_free(px);
        err = "out of memory";
        return false;
    }
    stbi_image_free(px);
    if (!mesh_cache_dir().empty()) {
        try {
            tex_cache_queue(hash, blob);
        } catch (...) {}   // No disk copy this time; the blob stands.
    }
    out = std::move(blob);
    return true;
}

inline ID3D11ShaderResourceView* tex_load_cached(ID3D11Device* dev, const std::string& path,
                                                 bool srgb, std::string& err) {
    std::shared_ptr<TexBlob> b;
    bool fc = false;
    if (!tex_blob_load(path, srgb, b, fc, err, false)) return nullptr;
    HRESULT hr = S_OK;
    ID3D11ShaderResourceView* srv = tex_create(dev, *b, err, &hr);
    // A corrupt-but-valid-looking cache file: decode from the source (TEX_RETRY_INVALID: only a refused one).
    if (!srv && fc && tex_cache_retry_worth(hr)) {
        b.reset();
        if (tex_blob_load(path, srgb, b, fc, err, true)) srv = tex_create(dev, *b, err);
    }
    return srv;
}
}