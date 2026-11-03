#pragma once

// The injection half (rendering_integration_injection.hpp): the sun locality extension (upload_locality_ext). Never
// include it on its own.
namespace RenderIntegration {

inline bool upload_locality_ext(ID3D11DeviceContext* ctx) {
    const UINT n = static_cast<UINT>(g_sun_maps.sun_locals.size() / 6);
    if (n == 0) return false;

    if (n > g_res.locality_cap || !g_res.locality_buf) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) return false;
        KH_SAFE_RELEASE(g_res.locality_srv);
        KH_SAFE_RELEASE(g_res.locality_buf);
        g_res.locality_cap = 0;
        UINT cap = 64;
        while (cap < n) cap *= 2;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = cap * 2 * sizeof(float) * 4;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(float) * 4;
        const bool ok =
            SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &g_res.locality_buf));
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = DXGI_FORMAT_UNKNOWN;
        sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        sd.Buffer.NumElements = cap * 2;
        if (ok &&
            SUCCEEDED(dev->CreateShaderResourceView(g_res.locality_buf, &sd,
                                                    &g_res.locality_srv))) {
            g_res.locality_cap = cap;
        } else {
            KH_SAFE_RELEASE(g_res.locality_srv);
            KH_SAFE_RELEASE(g_res.locality_buf);
        }
        dev->Release();
        if (g_res.locality_cap == 0) return false;
    }

    D3D11_MAPPED_SUBRESOURCE m = {};
    if (FAILED(ctx->Map(g_res.locality_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) return false;
    float* d = static_cast<float*>(m.pData);

    for (UINT i = 0; i < n; ++i) {
        const float* lb = g_sun_maps.sun_locals.data() + static_cast<size_t>(i) * 6;
        d[i * 8 + 0] = lb[0]; d[i * 8 + 1] = lb[1];
        d[i * 8 + 2] = lb[2]; d[i * 8 + 3] = 0.0f;
        d[i * 8 + 4] = lb[3]; d[i * 8 + 5] = lb[4];
        d[i * 8 + 6] = lb[5]; d[i * 8 + 7] = 0.0f;
    }

    ctx->Unmap(g_res.locality_buf, 0);
    return true;
}
}