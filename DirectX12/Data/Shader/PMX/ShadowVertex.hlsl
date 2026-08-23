#include "Header.hlsli"

// シャドウ深度パス用の軽量頂点シェーダー(スキニングのみ. 法線・UVは不要).
struct ShadowOutput
{
    float4 svpos : SV_POSITION;
};

ShadowOutput VS(VSInput input)
{
    ShadowOutput output;

    float4 skinnedPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;

    for (int i = 0; i < 4; ++i)
    {
        float boneWeight = input.boneWeights[i];

        if (boneWeight > 0.0f)
        {
            uint boneIndex = input.boneIndices[i];

            if (boneIndex < BoneCount)
            {
                skinnedPos += mul(boneTransforms[boneIndex], float4(input.pos.xyz, 1.0f)) * boneWeight;
            }
            totalWeight += boneWeight;
        }
    }

    // メインパスのVertex.hlslと同じフォールバック・正規化(スキニング結果を揃える).
    if (totalWeight <= 0.0001f)
    {
        skinnedPos = float4(input.pos.xyz, 1.0f);
    }
    else
    {
        skinnedPos /= totalWeight;
        skinnedPos.w = 1.0f;
    }

    float4 worldPos = mul(world, skinnedPos);
    output.svpos = mul(lightProj, mul(lightView, worldPos));

    return output;
}
