cbuffer GridConstantBuffer : register(b0)
{
	matrix ViewProj;
};

struct VSInput
{
	float3 Position : POSITION;
	float3 Color    : COLOR;
};

struct PSInput
{
	float4 Position : SV_POSITION;
	float3 Color    : COLOR;
};

PSInput main(VSInput Input)
{
	PSInput Output;
	Output.Position = mul(float4(Input.Position, 1.0f), ViewProj);
	Output.Color = Input.Color;
	return Output;
}
