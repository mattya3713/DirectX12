#include "Header.hlsli"

// 合成: シーンカラー+Bloom(Param0.y=強度)を出力する.
float4 PS(VsOutput input) : SV_TARGET
{
    const float4 base  = tex0.Sample(smp, input.uv);
    const float4 bloom = tex1.Sample(smp, input.uv);

    return base + bloom * Param0.y;
}
