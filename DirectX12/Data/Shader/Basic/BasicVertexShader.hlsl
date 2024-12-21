#include"..\BasicShaderHeader.hlsli"
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

//頂点シェーダ→ピクセルシェーダへのやり取りに使用する
//構造体
struct Output
{
	float4 svpos : SV_POSITION; //システム用頂点座標
	float4 pos : POSITION; //システム用頂点座標
	float4 normal : NORMAL0; //法線ベクトル
	float4 vnormal : NORMAL1; //法線ベクトル
	float2 uv : TEXCOORD; //UV値
	float3 ray : VECTOR; //ベクトル
};

BasicType BasicVS(
    float4 Pos          : POSITION,
    float4 Normal       : NORMAL,
    float2 UV           : TEXCOORD,
    min16uint Weight    : WEIGHT,
    min16uint2 BoneNo   : BONENO)
{ 
    // ピクセルシェーダへ渡す値.
	Output output;
    
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
}

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1)); //光の向かうベクトル(平行光線)
	float3 lightColor = float3(1, 1, 1); //ライトのカラー(1,1,1で真っ白)

	//ディフューズ計算
	float diffuseB = saturate(dot(-light, input.normal.xyz));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	//光の反射ベクトル
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), Specular.a);

	//スフィアマップ用UV
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	float4 ambCol = float4(Ambient * 0.6, 1);
	float4 texColor = tex.Sample(smp, input.uv); //テクスチャカラー
	return saturate((toonDif //輝度(トゥーン)
		* Diffuse + ambCol * 0.5) //ディフューズ色
		* texColor //テクスチャカラー
		* sph.Sample(smp, sphereMapUV) //スフィアマップ(乗算)
		+ spa.Sample(smp, sphereMapUV) //スフィアマップ(加算)
		+ float4(specularB * Specular.rgb, 1) //スペキュラー
		);
}