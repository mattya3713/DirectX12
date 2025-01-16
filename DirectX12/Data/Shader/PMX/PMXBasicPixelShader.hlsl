#include "PmxShaderHeader.hlsli"

float4 PS(Output input) : SV_TARGET
{
	float4 color = tex.Sample(smp, input.uv);
	return color;
}