#include "PmxShaderHeader.hlsli"

float4 PS(Output input) : SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1));
    float brightness = dot(-light, input.normal.xyz);
	
	
	//float4 color = tex.Sample(smp, input.uv);
    return float4(brightness, brightness, brightness, 1);
}