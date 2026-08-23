#include "Header.hlsli"

MstcVsOutput VS(MstcVsInput input)
{
    MstcVsOutput output;

    // 剛体オブジェクトのためスキニング無しの単純な変換.
    float4 worldPos  = mul(world, float4(input.pos, 1.0f));
    float4 worldN    = mul(world, float4(input.normal, 0.0f));

    float4 viewPos = mul(view, worldPos);
    output.svpos   = mul(proj, viewPos);

    output.pos         = worldPos;
    output.normal      = float4(normalize(worldN.xyz), 0.0f);
    output.localNormal = float4(normalize(input.normal), 0.0f);
    output.uv          = input.uv;
    output.ray         = normalize(eye - worldPos.xyz);

    return output;
}
