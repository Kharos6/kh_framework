#pragma once

// The injection half (rendering_integration_injection.hpp): the sun locality extension (kh_upload_locality_ext). Never
// include it on its own.
namespace RenderIntegration {

inline bool kh_upload_locality_ext(ID3D11DeviceContext* ctx) {
    const UINT khle_n = static_cast<UINT>(g_sun_locals.size() / 6);
    if (khle_n == 0) return false;

    if (khle_n > g_res.locality_cap || !g_res.locality_buf) {
        ID3D11Device* khle_dev = nullptr;
        ctx->GetDevice(&khle_dev);
        if (!khle_dev) return false;
        KH_SAFE_RELEASE(g_res.locality_srv);
        KH_SAFE_RELEASE(g_res.locality_buf);
        g_res.locality_cap = 0;
        UINT khle_cap = 64;
        while (khle_cap < khle_n) khle_cap *= 2;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = khle_cap * 2 * sizeof(float) * 4;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(float) * 4;
        const bool khle_ok =
            SUCCEEDED(khle_dev->CreateBuffer(&bd, nullptr, &g_res.locality_buf));
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = DXGI_FORMAT_UNKNOWN;
        sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        sd.Buffer.NumElements = khle_cap * 2;
        if (khle_ok &&
            SUCCEEDED(khle_dev->CreateShaderResourceView(g_res.locality_buf, &sd,
                                                         &g_res.locality_srv))) {
            g_res.locality_cap = khle_cap;
        } else {
            KH_SAFE_RELEASE(g_res.locality_srv);
            KH_SAFE_RELEASE(g_res.locality_buf);
        }
        khle_dev->Release();
        if (g_res.locality_cap == 0) return false;
    }

    D3D11_MAPPED_SUBRESOURCE khle_m = {};
    if (FAILED(ctx->Map(g_res.locality_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &khle_m))) return false;
    float* khle_d = static_cast<float*>(khle_m.pData);

    for (UINT i = 0; i < khle_n; ++i) {
        const float* lb = g_sun_locals.data() + static_cast<size_t>(i) * 6;
        khle_d[i * 8 + 0] = lb[0]; khle_d[i * 8 + 1] = lb[1];
        khle_d[i * 8 + 2] = lb[2]; khle_d[i * 8 + 3] = 0.0f;
        khle_d[i * 8 + 4] = lb[3]; khle_d[i * 8 + 5] = lb[4];
        khle_d[i * 8 + 6] = lb[5]; khle_d[i * 8 + 7] = 0.0f;
    }

    ctx->Unmap(g_res.locality_buf, 0);
    return true;
}
}