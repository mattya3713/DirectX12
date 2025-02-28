#include "PmxShaderHeader.hlsli"

float4 PS(Output input) : SV_TARGET
{
	return float4(input.normal.xyz, 1);
	
	float4 color = tex.Sample(smp, input.uv);
	return color;
}