#pragma once

// The feature half (rendering_integration_features.hpp): the texture disk cache: the file format, the mip builder, load
// and store. Never include it on its own.
namespace RenderIntegration {

static constexpr uint32_t KH_TEX_CACHE_MAGIC = 0x4354484Bu;   // "KHTC" little-endian.
// KH_CACHE_LIB_VER: a .khtc holds stb_image's decode and kh_tex_build_mips' chain, and stb_image carries no usable
// version (STBI_VERSION is 1 in every release) - updating stb_image.h or changing the mip builder needs a bump here by
// hand.
static constexpr uint32_t KH_TEX_CACHE_VERSION = 2;
static constexpr uint32_t KH_TEX_CACHE_MAX_MIPS = 15;

inline std::filesystem::path kh_tex_cache_file(uint64_t khtc_hash) {
    return kh_mesh_cache_dir() / kh_cache_file_name("kht_", khtc_hash, ".khtc");
}
inline uint64_t kh_tex_hash(const std::vector<uint8_t>& khtc_bytes, bool khtc_srgb) {
    uint64_t khtc_h = CryptoGenerator::fnv1a64_raw(khtc_bytes.data(), khtc_bytes.size());
    if (khtc_srgb) khtc_h ^= 0x9E3779B97F4A7C15ull;   // The two colour spaces are two textures.
    return khtc_h;
}
// sRGB <-> linear tables: 256 -> float for the decode, 4096 -> byte for the encode. KH_SRGB_TABLE_ONCE: each built by
// its function-local static's initialiser, which C++ runs once and thread-safely. Do not build them behind a plain
// bool: two decoding threads could race the fill, and the flag could be seen set before the table was - a half-filled
// table baked into a .khtc for good.
struct KhTexS2l { float t[256]; };
struct KhTexL2s { uint8_t t[4096]; };
inline const float* kh_tex_s2l() {
    static const KhTexS2l khtc_tab = [] {
        KhTexS2l khtc_r;
        float* const khtc_t = khtc_r.t;
        for (int khtc_i = 0; khtc_i < 256; ++khtc_i) {
            const float khtc_c = khtc_i / 255.0f;
            khtc_t[khtc_i] = khtc_c <= 0.04045f ? khtc_c / 12.92f : powf((khtc_c + 0.055f) / 1.055f, 2.4f);
        }
        return khtc_r;
    }();
    return khtc_tab.t;
}
inline const uint8_t* kh_tex_l2s() {
    static const KhTexL2s khtc_tab = [] {
        KhTexL2s khtc_r;
        uint8_t* const khtc_t = khtc_r.t;
        for (int khtc_i = 0; khtc_i < 4096; ++khtc_i) {
            const float khtc_l = khtc_i / 4095.0f;
            const float khtc_s = khtc_l <= 0.0031308f ? khtc_l * 12.92f : 1.055f * powf(khtc_l, 1.0f / 2.4f) - 0.055f;
            int khtc_v = static_cast<int>(khtc_s * 255.0f + 0.5f);
            khtc_t[khtc_i] = static_cast<uint8_t>(khtc_v < 0 ? 0 : khtc_v > 255 ? 255 : khtc_v);
        }
        return khtc_r;
    }();
    return khtc_tab.t;
}
// KH_MIP_ODD - one level from the one above it where an edge is odd: the polyphase box (Guthe & Heckbert,
// "Non-Power-of-Two Mipmap Creation", NVIDIA NVR-2005-002; NVIDIA Texture Tools' box). An odd edge 2m + 1 halves
// to m, each texel the exact area average of the (2m + 1) / m source texels it covers: taps 2x, 2x + 1, 2x + 2
// weighted (m - x, m, x + 1) / (2m + 1) - so no source texel is dropped and every one counts by the area it
// gives. An even edge keeps its two taps at 1/2; an edge of one, its one tap. Separable: the 2D weight is the
// product. Colour in linear light for an sRGB level (the 2x2 path's tables), alpha and linear data as stored;
// each rounded to nearest.
inline void kh_tex_mip_poly(const uint8_t* khpy_s, uint32_t khpy_sw, uint32_t khpy_sh, uint8_t* khpy_d,
                            uint32_t khpy_dw, uint32_t khpy_dh, bool khpy_srgb, const float* khpy_s2l,
                            const uint8_t* khpy_l2s) {
    // An axis's taps for one destination texel: source index and weight, up to three.
    auto khpy_taps = [](uint32_t khpy_n, uint32_t khpy_i, uint32_t* khpy_ix, float* khpy_w) -> int {
        if (khpy_n <= 1u) { khpy_ix[0] = 0u; khpy_w[0] = 1.0f; return 1; }
        if ((khpy_n & 1u) == 0u) {
            khpy_ix[0] = 2u * khpy_i; khpy_ix[1] = 2u * khpy_i + 1u;
            khpy_w[0] = 0.5f; khpy_w[1] = 0.5f;
            return 2;
        }
        const uint32_t khpy_m = khpy_n >> 1;
        const float khpy_r = 1.0f / static_cast<float>(khpy_n);
        khpy_ix[0] = 2u * khpy_i; khpy_ix[1] = 2u * khpy_i + 1u; khpy_ix[2] = 2u * khpy_i + 2u;
        khpy_w[0] = static_cast<float>(khpy_m - khpy_i) * khpy_r;
        khpy_w[1] = static_cast<float>(khpy_m) * khpy_r;
        khpy_w[2] = static_cast<float>(khpy_i + 1u) * khpy_r;
        return 3;
    };
    for (uint32_t khpy_y = 0; khpy_y < khpy_dh; ++khpy_y) {
        uint32_t khpy_yi[3];
        float khpy_yw[3];
        const int khpy_ny = khpy_taps(khpy_sh, khpy_y, khpy_yi, khpy_yw);
        for (uint32_t khpy_x = 0; khpy_x < khpy_dw; ++khpy_x) {
            uint32_t khpy_xi[3];
            float khpy_xw[3];
            const int khpy_nx = khpy_taps(khpy_sw, khpy_x, khpy_xi, khpy_xw);
            float khpy_acc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            for (int khpy_j = 0; khpy_j < khpy_ny; ++khpy_j) {
                for (int khpy_i = 0; khpy_i < khpy_nx; ++khpy_i) {
                    const float khpy_w = khpy_yw[khpy_j] * khpy_xw[khpy_i];
                    const uint8_t* khpy_p = khpy_s + (static_cast<size_t>(khpy_yi[khpy_j]) * khpy_sw +
                                                      khpy_xi[khpy_i]) * 4u;
                    for (int khpy_c = 0; khpy_c < 3; ++khpy_c) {
                        khpy_acc[khpy_c] += khpy_w * (khpy_srgb ? khpy_s2l[khpy_p[khpy_c]]
                                                                : static_cast<float>(khpy_p[khpy_c]));
                    }
                    khpy_acc[3] += khpy_w * static_cast<float>(khpy_p[3]);
                }
            }
            uint8_t* khpy_o = khpy_d + (static_cast<size_t>(khpy_y) * khpy_dw + khpy_x) * 4u;
            for (int khpy_c = 0; khpy_c < 4; ++khpy_c) {
                if (khpy_srgb && khpy_c < 3) {
                    const int khpy_q = static_cast<int>(khpy_acc[khpy_c] * 4095.0f + 0.5f);
                    khpy_o[khpy_c] = khpy_l2s[khpy_q < 0 ? 0 : khpy_q > 4095 ? 4095 : khpy_q];
                } else {
                    const int khpy_v = static_cast<int>(khpy_acc[khpy_c] + 0.5f);
                    khpy_o[khpy_c] = static_cast<uint8_t>(khpy_v < 0 ? 0 : khpy_v > 255 ? 255 : khpy_v);
                }
            }
        }
    }
}
// Full mip chain from level 0 (RGBA8). 2x2 box where both edges are even (or one); a level with an odd edge (above one)
// takes the polyphase box instead (KH_MIP_ODD) - the 2x2 box would drop that edge's last texel.
inline void kh_tex_build_mips(KhTexBlob& khtc_b, const uint8_t* khtc_px, uint32_t khtc_w, uint32_t khtc_h, bool khtc_srgb) {
    khtc_b.w = khtc_w; khtc_b.h = khtc_h; khtc_b.srgb = khtc_srgb;
    khtc_b.level_off.clear();
    khtc_b.data.clear();
    uint32_t khtc_lw = khtc_w, khtc_lh = khtc_h, khtc_n = 0;
    size_t khtc_total = 0;
    for (;;) {
        khtc_total += static_cast<size_t>(khtc_lw) * khtc_lh * 4u;
        ++khtc_n;
        if ((khtc_lw == 1 && khtc_lh == 1) || khtc_n >= KH_TEX_CACHE_MAX_MIPS) break;
        khtc_lw = khtc_lw > 1 ? khtc_lw >> 1 : 1;
        khtc_lh = khtc_lh > 1 ? khtc_lh >> 1 : 1;
    }
    khtc_b.mips = khtc_n;
    khtc_b.data.resize(khtc_total);
    khtc_b.level_off.resize(khtc_n);
    memcpy(khtc_b.data.data(), khtc_px, static_cast<size_t>(khtc_w) * khtc_h * 4u);
    khtc_b.level_off[0] = 0;
    const float* khtc_s2l = kh_tex_s2l();
    const uint8_t* khtc_l2s = kh_tex_l2s();
    uint32_t khtc_sw = khtc_w, khtc_sh = khtc_h;
    size_t khtc_soff = 0;
    for (uint32_t khtc_l = 1; khtc_l < khtc_n; ++khtc_l) {
        const uint32_t khtc_dw = khtc_sw > 1 ? khtc_sw >> 1 : 1;
        const uint32_t khtc_dh = khtc_sh > 1 ? khtc_sh >> 1 : 1;
        const size_t khtc_doff = khtc_soff + static_cast<size_t>(khtc_sw) * khtc_sh * 4u;
        khtc_b.level_off[khtc_l] = static_cast<uint32_t>(khtc_doff);
        const uint8_t* khtc_s = khtc_b.data.data() + khtc_soff;
        uint8_t* khtc_d = khtc_b.data.data() + khtc_doff;
        if ((khtc_sw > 1u && (khtc_sw & 1u) != 0u) || (khtc_sh > 1u && (khtc_sh & 1u) != 0u)) {   // KH_MIP_ODD.
            kh_tex_mip_poly(khtc_s, khtc_sw, khtc_sh, khtc_d, khtc_dw, khtc_dh, khtc_srgb, khtc_s2l, khtc_l2s);
            khtc_soff = khtc_doff;
            khtc_sw = khtc_dw; khtc_sh = khtc_dh;
            continue;
        }
        for (uint32_t khtc_y = 0; khtc_y < khtc_dh; ++khtc_y) {
            const uint32_t khtc_y0 = khtc_y * 2u < khtc_sh ? khtc_y * 2u : khtc_sh - 1u;
            const uint32_t khtc_y1 = khtc_y0 + 1u < khtc_sh ? khtc_y0 + 1u : khtc_sh - 1u;
            for (uint32_t khtc_x = 0; khtc_x < khtc_dw; ++khtc_x) {
                const uint32_t khtc_x0 = khtc_x * 2u < khtc_sw ? khtc_x * 2u : khtc_sw - 1u;
                const uint32_t khtc_x1 = khtc_x0 + 1u < khtc_sw ? khtc_x0 + 1u : khtc_sw - 1u;
                const uint8_t* khtc_p[4] = {
                    khtc_s + (static_cast<size_t>(khtc_y0) * khtc_sw + khtc_x0) * 4u,
                    khtc_s + (static_cast<size_t>(khtc_y0) * khtc_sw + khtc_x1) * 4u,
                    khtc_s + (static_cast<size_t>(khtc_y1) * khtc_sw + khtc_x0) * 4u,
                    khtc_s + (static_cast<size_t>(khtc_y1) * khtc_sw + khtc_x1) * 4u };
                uint8_t* khtc_o = khtc_d + (static_cast<size_t>(khtc_y) * khtc_dw + khtc_x) * 4u;
                if (khtc_srgb) {
                    for (int khtc_c = 0; khtc_c < 3; ++khtc_c) {
                        const float khtc_lin = 0.25f * (khtc_s2l[khtc_p[0][khtc_c]] + khtc_s2l[khtc_p[1][khtc_c]] +
                                                        khtc_s2l[khtc_p[2][khtc_c]] + khtc_s2l[khtc_p[3][khtc_c]]);
                        int khtc_q = static_cast<int>(khtc_lin * 4095.0f + 0.5f);
                        khtc_o[khtc_c] = khtc_l2s[khtc_q < 0 ? 0 : khtc_q > 4095 ? 4095 : khtc_q];
                    }
                } else {
                    for (int khtc_c = 0; khtc_c < 3; ++khtc_c) {
                        khtc_o[khtc_c] = static_cast<uint8_t>((khtc_p[0][khtc_c] + khtc_p[1][khtc_c] +
                                                               khtc_p[2][khtc_c] + khtc_p[3][khtc_c] + 2u) >> 2);
                    }
                }
                khtc_o[3] = static_cast<uint8_t>((khtc_p[0][3] + khtc_p[1][3] + khtc_p[2][3] + khtc_p[3][3] + 2u) >> 2);
            }
        }
        khtc_soff = khtc_doff;
        khtc_sw = khtc_dw; khtc_sh = khtc_dh;
    }
}
// KH_DDS_MIP_FILL - a 32-bit RGBA / BGRA .dds (or a .paa made into one, KH_PAA; or a BC file KH_DDS_BLOCK4 decoded)
// that holds fewer levels than its full chain to 1 x 1 - a header that says 0 or 1 (a common exporter default), or a
// chain cut short. Created with only those, it has no smaller level to sample and aliases (shimmers) at every distance
// past its smallest level's one texel per pixel. Its levels are kept as RGBA8 (BGRA swapped, alpha as stored) and the
// chain goes on from its smallest with the stb route's builder (kh_tex_build_mips: the 2x2 / polyphase box, colour in
// linear light for an sRGB map), created as that route's (kh_tex_create; same colour space). A file with no chain gets
// it all; one cut short only the levels it lacks. BC1 / BC2 / BC3 / BC7 blocks are left as they are (no BC encoder
// here: such a file keeps only the levels it holds). The header is read as kh_load_dds reads it; false = not such a
// file (or no memory): the bytes are created unchanged. Not disk-cached (the .khtc holds only the stb route's); the
// cost is the build at each first sight, on the loader thread (or in the synchronous fallback's kh_dds_create).
inline bool kh_dds_mip_fill(const std::vector<uint8_t>& f, bool srgb, std::shared_ptr<KhTexBlob>& khdf_out) {
    auto rd32 = [&](size_t off) { uint32_t v; memcpy(&v, f.data() + off, 4); return v; };
    if (f.size() < 128 || rd32(0) != 0x20534444u) return false;
    const uint32_t height = rd32(12), width = rd32(16);
    uint32_t mips = rd32(28);
    const uint32_t pf_flags = rd32(80), fourcc = rd32(84);
    const uint32_t bitcount = rd32(88), rmask = rd32(92);
    if (width == 0 || height == 0 || width > 16384 || height > 16384) return false;
    if (mips == 0) mips = 1;
    if (mips > 16) mips = 16;
    uint32_t khdf_chain = 1;
    for (uint32_t khdf_s = width > height ? width : height; khdf_s > 1; khdf_s >>= 1) ++khdf_chain;
    if (mips > khdf_chain) mips = khdf_chain;
    bool khdf_bgra = false;
    size_t data_off = 128;
    if ((pf_flags & 0x4u) && fourcc == 0x30315844u) {   // DX10.
        if (f.size() < 148) return false;
        const uint32_t dxgi = rd32(128);
        data_off = 148;
        if (dxgi == 28u || dxgi == 29u) khdf_bgra = false;
        else if (dxgi == 87u || dxgi == 91u) khdf_bgra = true;
        else return false;   // BC, or kh_load_dds's refusal.
    } else if (!(pf_flags & 0x4u) && (pf_flags & 0x40u) && bitcount == 32u) {
        if (rmask == 0x00ff0000u) khdf_bgra = true;
        else if (rmask == 0x000000ffu) khdf_bgra = false;
        else return false;
    } else {
        return false;
    }
    uint32_t khdf_held = 0;   // The levels the file holds, as kh_load_dds counts them.
    size_t off = data_off;
    for (uint32_t m = 0; m < mips; ++m) {
        const uint32_t mw = width >> m ? width >> m : 1;
        const uint32_t mh = height >> m ? height >> m : 1;
        const size_t bytes = static_cast<size_t>(mw) * 4u * mh;
        if (off + bytes > f.size()) break;
        off += bytes;
        ++khdf_held;
    }
    if (khdf_held == 0u || khdf_held >= khdf_chain) return false;   // No level 0 (kh_load_dds says so), or whole.
    try {
        // The file's levels, tight-packed as the blob's are: RGBA8. Reserved for the whole chain (the levels the
        // tail adds below the smallest held one), so the tail's insert below does not reallocate a copy of it.
        size_t khdf_cap = off - data_off;
        for (uint32_t khdf_cw = width >> (khdf_held - 1u) ? width >> (khdf_held - 1u) : 1u,
                      khdf_ch = height >> (khdf_held - 1u) ? height >> (khdf_held - 1u) : 1u;
             khdf_cw > 1u || khdf_ch > 1u;) {
            khdf_cw = khdf_cw > 1u ? khdf_cw >> 1 : 1u;
            khdf_ch = khdf_ch > 1u ? khdf_ch >> 1 : 1u;
            khdf_cap += static_cast<size_t>(khdf_cw) * 4u * khdf_ch;
        }
        std::vector<uint8_t> khdf_px;
        khdf_px.reserve(khdf_cap);
        khdf_px.assign(f.begin() + static_cast<std::ptrdiff_t>(data_off), f.begin() + static_cast<std::ptrdiff_t>(off));
        if (khdf_bgra) {
            for (size_t khdf_i = 0; khdf_i + 3u < khdf_px.size(); khdf_i += 4u) {
                std::swap(khdf_px[khdf_i], khdf_px[khdf_i + 2u]);
            }
        }
        std::shared_ptr<KhTexBlob> khdf_b = std::make_shared<KhTexBlob>();
        khdf_b->w = width; khdf_b->h = height; khdf_b->srgb = srgb;
        size_t khdf_o = 0;
        for (uint32_t m = 0; m < khdf_held; ++m) {
            const uint32_t mw = width >> m ? width >> m : 1;
            const uint32_t mh = height >> m ? height >> m : 1;
            khdf_b->level_off.push_back(static_cast<uint32_t>(khdf_o));
            khdf_o += static_cast<size_t>(mw) * 4u * mh;
        }
        // The chain from the smallest held level on: its own level 0 is that level, so its levels 1 .. follow.
        const uint32_t khdf_lw = width >> (khdf_held - 1u) ? width >> (khdf_held - 1u) : 1u;
        const uint32_t khdf_lh = height >> (khdf_held - 1u) ? height >> (khdf_held - 1u) : 1u;
        KhTexBlob khdf_tail;
        kh_tex_build_mips(khdf_tail, khdf_px.data() + khdf_b->level_off[khdf_held - 1u], khdf_lw, khdf_lh, srgb);
        khdf_b->data = std::move(khdf_px);
        const size_t khdf_t1 = khdf_tail.level_off.size() > 1u ? khdf_tail.level_off[1] : khdf_tail.data.size();
        for (uint32_t m = 1; m < khdf_tail.mips; ++m) {
            khdf_b->level_off.push_back(static_cast<uint32_t>(khdf_o + (khdf_tail.level_off[m] - khdf_t1)));
        }
        khdf_b->data.insert(khdf_b->data.end(), khdf_tail.data.begin() + static_cast<std::ptrdiff_t>(khdf_t1),
                            khdf_tail.data.end());
        khdf_b->mips = static_cast<uint32_t>(khdf_b->level_off.size());
        khdf_out = std::move(khdf_b);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}
// Full-validation load (any structural doubt is a miss). khtc_own = the Documents copy: lru-touched, under the trim
// budget.
inline bool kh_tex_cache_load_one(const std::filesystem::path& khtc_p, bool khtc_own,
                                  uint64_t khtc_hash, bool khtc_srgb, KhTexBlob& khtc_b) {
    try {
        std::ifstream khtc_f(khtc_p, std::ios::binary | std::ios::ate);
        if (!khtc_f) return false;
        const std::streamsize khtc_n = khtc_f.tellg();
        if (khtc_n < 36) return false;
        khtc_f.seekg(0);
        uint32_t khtc_hd[7] = {};   // Magic, version, hash lo, hash hi, w, h, mips.
        uint32_t khtc_flags = 0;
        khtc_f.read(reinterpret_cast<char*>(khtc_hd), sizeof(khtc_hd));
        khtc_f.read(reinterpret_cast<char*>(&khtc_flags), 4);
        if (!khtc_f) return false;
        const uint64_t khtc_fh = (static_cast<uint64_t>(khtc_hd[3]) << 32) | khtc_hd[2];
        if (khtc_hd[0] != KH_TEX_CACHE_MAGIC || khtc_hd[1] != KH_TEX_CACHE_VERSION || khtc_fh != khtc_hash) return false;
        const uint32_t khtc_w = khtc_hd[4], khtc_h = khtc_hd[5], khtc_m = khtc_hd[6];
        if (khtc_w == 0 || khtc_h == 0 || khtc_w > 16384 || khtc_h > 16384 || khtc_m == 0 || khtc_m > KH_TEX_CACHE_MAX_MIPS) return false;
        if (((khtc_flags & 1u) != 0u) != khtc_srgb) return false;
        size_t khtc_total = 0;
        uint32_t khtc_lw = khtc_w, khtc_lh = khtc_h;
        std::vector<uint32_t> khtc_off(khtc_m);
        for (uint32_t khtc_l = 0; khtc_l < khtc_m; ++khtc_l) {
            khtc_off[khtc_l] = static_cast<uint32_t>(khtc_total);
            khtc_total += static_cast<size_t>(khtc_lw) * khtc_lh * 4u;
            khtc_lw = khtc_lw > 1 ? khtc_lw >> 1 : 1;
            khtc_lh = khtc_lh > 1 ? khtc_lh >> 1 : 1;
        }
        if (static_cast<size_t>(khtc_n) != 32u + khtc_total) return false;
        khtc_b.data.resize(khtc_total);
        khtc_f.read(reinterpret_cast<char*>(khtc_b.data.data()), static_cast<std::streamsize>(khtc_total));
        if (!khtc_f) return false;
        khtc_f.close();
        khtc_b.w = khtc_w; khtc_b.h = khtc_h; khtc_b.mips = khtc_m; khtc_b.srgb = khtc_srgb;
        khtc_b.level_off = std::move(khtc_off);
        if (khtc_own) {
            std::error_code khtc_ec;
            std::filesystem::last_write_time(khtc_p, std::filesystem::file_time_type::clock::now(), khtc_ec);
        }
        return true;
    } catch (...) { return false; }
}
inline bool kh_tex_cache_load(uint64_t khtc_hash, bool khtc_srgb, KhTexBlob& khtc_b) {
    std::vector<std::filesystem::path> khtc_cand;
    const bool khtc_docs = !kh_mesh_cache_dir().empty();
    if (khtc_docs) khtc_cand.push_back(kh_tex_cache_file(khtc_hash));
    kh_mod_cache_append(khtc_cand, kh_cache_file_name("kht_", khtc_hash, ".khtc"));
    for (size_t khtc_i = 0; khtc_i < khtc_cand.size(); ++khtc_i) {
        const bool khtc_own = khtc_i == 0 && khtc_docs;
        if (kh_tex_cache_load_one(khtc_cand[khtc_i], khtc_own, khtc_hash, khtc_srgb, khtc_b)) {
            return true;
        }
    }
    return false;
}
inline void kh_tex_cache_store(uint64_t khtc_hash, const KhTexBlob& khtc_b) {
    if (kh_mesh_cache_dir().empty()) return;
    if (khtc_b.data.empty() || khtc_b.mips == 0) return;
    std::filesystem::path khtc_tmp;   // KH_CACHE_TMP_WRITE.
    try {
        const std::filesystem::path khtc_p = kh_tex_cache_file(khtc_hash);
        khtc_tmp = kh_cache_tmp_of(khtc_p);
        std::ofstream khtc_f(khtc_tmp, std::ios::binary | std::ios::trunc);
        if (!khtc_f) return;
        const uint32_t khtc_hd[7] = { KH_TEX_CACHE_MAGIC, KH_TEX_CACHE_VERSION,
                                      static_cast<uint32_t>(khtc_hash & 0xFFFFFFFFu),
                                      static_cast<uint32_t>(khtc_hash >> 32),
                                      khtc_b.w, khtc_b.h, khtc_b.mips };
        const uint32_t khtc_flags = khtc_b.srgb ? 1u : 0u;
        khtc_f.write(reinterpret_cast<const char*>(khtc_hd), sizeof(khtc_hd));
        khtc_f.write(reinterpret_cast<const char*>(&khtc_flags), 4);
        khtc_f.write(reinterpret_cast<const char*>(khtc_b.data.data()), static_cast<std::streamsize>(khtc_b.data.size()));
        kh_cache_tmp_commit(khtc_f, khtc_tmp, khtc_p);   // KH_CACHE_TMP_WRITE.
    } catch (...) {
        std::error_code khtc_ec;   // KH_CACHE_TMP_WRITE: a throw mid-write leaves no temp behind.
        if (!khtc_tmp.empty()) std::filesystem::remove(khtc_tmp, khtc_ec);
    }
}
inline void kh_tex_cache_queue(uint64_t khtc_hash, std::shared_ptr<const KhTexBlob> khtc_b) {
    if (!kh_mesh_cache_writer_start()) {   // No thread: write inline, the mesh cache's own
                                           // fallback.
        kh_tex_cache_store(khtc_hash, *khtc_b);
        std::lock_guard<std::mutex> khtc_tl(g_khsm_trim_mu);
        kh_mesh_cache_trim();
        return;
    }
    {
        std::lock_guard<std::mutex> khtc_l(g_khmw_mu);
        KhMeshWriteJob khtc_j = { khtc_hash, -1, std::move(khtc_b) };
        g_khmw_q.push_back(std::move(khtc_j));
    }
    g_khmw_cv.notify_one();
}
// Immutable upload, every level in one create - no GenerateMips, no RT bind, no context needed. khtc_hro: the failing
// call's HRESULT (KH_TEX_RETRY_INVALID).
inline ID3D11ShaderResourceView* kh_tex_create(ID3D11Device* dev, const KhTexBlob& khtc_b, std::string& err,
                                               HRESULT* khtc_hro = nullptr) {
    if (khtc_hro) *khtc_hro = S_OK;
    D3D11_TEXTURE2D_DESC khtc_td = {};
    khtc_td.Width = khtc_b.w; khtc_td.Height = khtc_b.h;
    khtc_td.MipLevels = khtc_b.mips; khtc_td.ArraySize = 1;
    khtc_td.Format = khtc_b.srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    khtc_td.SampleDesc.Count = 1;
    khtc_td.Usage = D3D11_USAGE_IMMUTABLE;
    khtc_td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    std::vector<D3D11_SUBRESOURCE_DATA> khtc_init(khtc_b.mips);
    uint32_t khtc_lw = khtc_b.w;
    for (uint32_t khtc_l = 0; khtc_l < khtc_b.mips; ++khtc_l) {
        khtc_init[khtc_l].pSysMem = khtc_b.data.data() + khtc_b.level_off[khtc_l];
        khtc_init[khtc_l].SysMemPitch = khtc_lw * 4u;
        khtc_init[khtc_l].SysMemSlicePitch = 0;
        khtc_lw = khtc_lw > 1 ? khtc_lw >> 1 : 1;
    }
    ID3D11Texture2D* khtc_tex = nullptr;
    HRESULT khtc_hr = dev->CreateTexture2D(&khtc_td, khtc_init.data(), &khtc_tex);
    if (FAILED(khtc_hr)) {
        if (khtc_hro) *khtc_hro = khtc_hr;
        err = "CreateTexture2D(mipped) " + hr_str(khtc_hr);
        return nullptr;
    }
    ID3D11ShaderResourceView* khtc_srv = nullptr;
    khtc_hr = dev->CreateShaderResourceView(khtc_tex, nullptr, &khtc_srv);
    khtc_tex->Release();
    if (FAILED(khtc_hr)) {
        if (khtc_hro) *khtc_hro = khtc_hr;
        err = "CreateSRV(mipped) " + hr_str(khtc_hr);
        return nullptr;
    }
    return khtc_srv;
}
// KH_TEX_RETRY_INVALID - the corrupt-cache retry (kh_tex_load_cached, kh_tex_async_pump) decodes the source again when
// a blob read from the disk cache fails to create. The reader passes only a file of the exact size its header
// describes, so a damaged file reaching the create can only fail it by its description (a level count past its chain:
// E_INVALIDARG); every other failure - out of memory with VRAM full, a removed or reset device - fails the fresh
// decode's create the same way, so a retry of it gains nothing and costs a source read, a decode and a cache rewrite
// first: only E_INVALIDARG retries.
inline bool kh_tex_cache_retry_worth(HRESULT khtr_hr) { return khtr_hr == E_INVALIDARG; }
// KH_DDS_MIP_FILL: the synchronous route's .dds / .paa create - the chain built where the file lacks it, else
// the file's own levels (kh_load_dds).
inline ID3D11ShaderResourceView* kh_dds_create(ID3D11Device* dev, const std::vector<uint8_t>& f, bool srgb,
                                               std::string& err) {
    std::shared_ptr<KhTexBlob> khdc_b;
    if (kh_dds_mip_fill(f, srgb, khdc_b)) return kh_tex_create(dev, *khdc_b, err);
    return kh_load_dds(dev, f, srgb, err);
}
// CPU half of the first-sight load: disk read, content hash, disk-cache probe, else decode + CPU mips (+ cache-store
// queue). No device, no context, so the loader worker runs exactly this. khtb_force skips the probe (the corrupt-cache
// retry); khtb_from_cache tells the caller which arm produced the blob.
inline bool kh_tex_blob_load(const std::string& path, bool srgb,
                             std::shared_ptr<KhTexBlob>& khtb_out, bool& khtb_from_cache,
                             std::string& err, bool khtb_force) {
    khtb_from_cache = false;
    std::vector<uint8_t> khtc_bytes;
    if (!kh_asset_read(path, khtc_bytes, 512ull * 1024u * 1024u)) { err = "read failed"; return false; }   // PBO_PATH.
    const uint64_t khtc_hash = kh_tex_hash(khtc_bytes, srgb);
    if (!khtb_force) {
        // KH_TEXLDR_GUARD: a failed allocation is a failed load, not a throw into the caller.
        try {
            auto khtc_hit = std::make_shared<KhTexBlob>();
            if (kh_tex_cache_load(khtc_hash, srgb, *khtc_hit)) {
                khtb_out = std::move(khtc_hit);
                khtb_from_cache = true;
                return true;
            }
        } catch (const std::bad_alloc&) {
            err = "out of memory";
            return false;
        }
    }
    int khtc_w = 0, khtc_h = 0, khtc_comp = 0;
    stbi_uc* khtc_px = stbi_load_from_memory(khtc_bytes.data(), static_cast<int>(khtc_bytes.size()),
                                             &khtc_w, &khtc_h, &khtc_comp, 4);
    if (!khtc_px) { err = std::string("decode failed: ") + (stbi_failure_reason() ? stbi_failure_reason() : "?"); return false; }
    if (khtc_w <= 0 || khtc_h <= 0 || khtc_w > 16384 || khtc_h > 16384) { stbi_image_free(khtc_px); err = "bad dimensions"; return false; }
    // KH_TEXLDR_GUARD: the mip chain of a 16384^2 image is ~1.4 GB on top of the decode; a failed
    // allocation frees the pixels and fails the load (the loader thread and the sync path alike).
    std::shared_ptr<KhTexBlob> khtc_blob;
    try {
        khtc_blob = std::make_shared<KhTexBlob>();
        kh_tex_build_mips(*khtc_blob, khtc_px, static_cast<uint32_t>(khtc_w), static_cast<uint32_t>(khtc_h), srgb);
    } catch (const std::bad_alloc&) {
        stbi_image_free(khtc_px);
        err = "out of memory";
        return false;
    }
    stbi_image_free(khtc_px);
    if (!kh_mesh_cache_dir().empty()) {
        try {
            kh_tex_cache_queue(khtc_hash, khtc_blob);
        } catch (...) {}   // No disk copy this time; the blob stands.
    }
    khtb_out = std::move(khtc_blob);
    return true;
}

inline ID3D11ShaderResourceView* kh_tex_load_cached(ID3D11Device* dev, const std::string& path,
                                                    bool srgb, std::string& err) {
    std::shared_ptr<KhTexBlob> khtc_b;
    bool khtc_fc = false;
    if (!kh_tex_blob_load(path, srgb, khtc_b, khtc_fc, err, false)) return nullptr;
    HRESULT khtc_hr = S_OK;
    ID3D11ShaderResourceView* khtc_srv = kh_tex_create(dev, *khtc_b, err, &khtc_hr);
    // A corrupt-but-valid-looking cache file: decode from the source (KH_TEX_RETRY_INVALID: only a refused one).
    if (!khtc_srv && khtc_fc && kh_tex_cache_retry_worth(khtc_hr)) {
        khtc_b.reset();
        if (kh_tex_blob_load(path, srgb, khtc_b, khtc_fc, err, true)) khtc_srv = kh_tex_create(dev, *khtc_b, err);
    }
    return khtc_srv;
}
}