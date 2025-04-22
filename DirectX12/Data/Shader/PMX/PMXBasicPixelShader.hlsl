#include "PmxShaderHeader.hlsli"

float4 PS(Output input) : SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1));
    float3 normal = normalize(input.normal);

    float diffuse = saturate(dot(-light, normal));

    float4 color = tex.Sample(smp, input.uv);

    color.rgb = color.rgb * diffuse;

    return color;
}