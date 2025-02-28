#include "PmxShaderHeader.hlsli"

Output VS(
	float4 pos : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD)
{
	Output output;
	pos = mul(world, pos);
	output.svpos = mul(mul(proj, view), pos);
	output.uv = uv;
	output.normal = float4(normal,1);
	return output;
}