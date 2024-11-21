#include"BasicShaderHeader.hlsli"
Texture2D<float4> tex : register(t0); //0番スロットに設定されたテクスチャ(ベース)
Texture2D<float4> sph : register(t1); //1番スロットに設定されたテクスチャ(乗算)
Texture2D<float4> spa : register(t2); //2番スロットに設定されたテクスチャ(加算)
Texture2D<float4> toon : register(t3); //3番スロットに設定されたテクスチャ(トゥーン)

SamplerState smp : register(s0); //0番スロットに設定されたサンプラ
SamplerState smpToon : register(s1); //1番スロットに設定されたサンプラ

// 定数バッファ0.
cbuffer SceneData : register(b0)
{
    matrix View;
    matrix Proj; // ビュープロジェクション行列.
    float3 Eye;
};
cbuffer Transform : register(b1)
{
    matrix World;       // ワールド変換行列
	matrix Bones[256];  // ボーン行列.
}

// 定数バッファ1.
// マテリアル用.
cbuffer Material : register(b2)
{
    float4 Diffuse;     // ディフューズ色.
    float4 Specular;    // スペキュラ.
    float3 Ambient;     // アンビエント.
};


BasicType BasicVS(
    float4 Pos          : POSITION,
    float4 Normal       : NORMAL,
    float2 UV           : TEXCOORD,
    min16uint Weight    : WEIGHT,
    min16uint2 BoneNo   : BONENO)
{ 
    // ピクセルシェーダへ渡す値.
	BasicType output;
    
    // 0~100を0~1fに丸める.
	float w = Weight * 0.01f;
    // ボーンを線形補間.
	matrix Bone = Bones[BoneNo[0]] * w + Bones[BoneNo[1]] * (1 - w);
    // ボーン行列を乗算.
	Pos = mul(Bones[BoneNo[0]], Pos);           
	Pos = mul(World, Pos);
	output.svpos = mul(mul(Proj, View), Pos);   // シェーダでは列優先なので注意
	output.pos = mul(View, Pos);
	Normal.w = 0;                               // ここ重要(平行移動成分を無効にする)
	output.normal = mul(World, Normal);         // 法線にもワールド変換を行う
    output.vnormal = mul(View, output.normal);
	output.uv = UV;
	output.ray = normalize(Pos.xyz - mul((float4x3) View, Eye).xyz); //視線ベクトル

    return output;
    return output;
}
