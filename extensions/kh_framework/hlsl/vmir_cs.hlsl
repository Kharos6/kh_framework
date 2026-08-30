// vmir_cs.hlsl - RCDATA resource in kh_shaders.rc, concatenated into its unit by C++ (no #include). Any edit changes the unit's shader cache key.

ByteAddressBuffer khmc_in : register(t0);
RWByteAddressBuffer khmc_out : register(u0);
[numthreads(1, 1, 1)]
void CSMirB2(uint3 khmc_id : SV_DispatchThreadID)
{
    float khmc_m[16];
    [unroll] for (uint khmc_i = 0; khmc_i < 16; ++khmc_i) {
        khmc_m[khmc_i] = asfloat(khmc_in.Load(khmc_i * 4));
    }
    // Least-squares m22 over the top three rows.
    float khmc_num = 0.0f, khmc_den = 0.0f;
    [unroll] for (uint khmc_r = 0; khmc_r < 3; ++khmc_r) {
        khmc_num += khmc_m[khmc_r * 4 + 2] * khmc_m[khmc_r * 4 + 3];
        khmc_den += khmc_m[khmc_r * 4 + 3] * khmc_m[khmc_r * 4 + 3];
    }
    float khmc_w = sqrt(khmc_den);
    bool khmc_ok = khmc_den > 1.0e-12f && khmc_w > 0.90f && khmc_w < 1.10f;
    float khmc_m22 = khmc_ok ? khmc_num / khmc_den : 0.0f;
    khmc_ok = khmc_ok && abs(khmc_m22) > 1.0e-9f;
    float khmc_m32 = khmc_m[14] - khmc_m[15] * khmc_m22;
    float khmc_n = khmc_ok ? (-khmc_m32 / khmc_m22) : -1.0f;
    khmc_ok = khmc_ok && khmc_n > 0.0f && khmc_m22 > 1.0f + 1.0e-6f && khmc_m32 < -1.0e-6f;
    float khmc_f = khmc_ok ? (khmc_n * khmc_m22 / (khmc_m22 - 1.0f)) : 0.0f;
    khmc_ok = khmc_ok && khmc_f > 0.06f;
    float khmc_l22 = khmc_ok ? (khmc_f / (khmc_f - 0.05f)) : 0.0f;
    float khmc_l32 = -0.05f * khmc_l22;
    float khmc_a = khmc_ok ? (khmc_l32 / khmc_m32) : 1.0f;
    float khmc_b = khmc_ok ? (khmc_l22 - khmc_m22 * khmc_a) : 0.0f;
    [unroll] for (uint khmc_r2 = 0; khmc_r2 < 4; ++khmc_r2) {
        float khmc_z = khmc_m[khmc_r2 * 4 + 2];
        float khmc_ww = khmc_m[khmc_r2 * 4 + 3];
        float khmc_zo = khmc_ok ? (khmc_a * khmc_z + khmc_b * khmc_ww) : khmc_z;
        khmc_out.Store(khmc_r2 * 16 + 0, asuint(khmc_m[khmc_r2 * 4 + 0]));
        khmc_out.Store(khmc_r2 * 16 + 4, asuint(khmc_m[khmc_r2 * 4 + 1]));
        khmc_out.Store(khmc_r2 * 16 + 8, asuint(khmc_zo));
        khmc_out.Store(khmc_r2 * 16 + 12, asuint(khmc_ww));
    }
}
