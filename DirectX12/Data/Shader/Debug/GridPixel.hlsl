struct PSInput
{
	float4 Position : SV_POSITION;
	float3 Color    : COLOR;
};

float4 main(PSInput Input) : SV_TARGET
{
	return float4(Input.Color, 1.0f);
}
