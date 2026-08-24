#include "Header.hlsli"

// 輝度抽出: しきい値(Param0.x)を超える明部だけを滑らかに残す.
float4 PS(VsOutput input) : SV_TARGET
{
    const float4 c = tex0.Sample(smp, input.uv);
    const float luma = dot(c.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    const float k = saturate(luma - Param0.x);
    return c * k;
}
