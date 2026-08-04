struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    
    // Additional UVs (最大4つ)
    float4 additionalUV0 : TEXCOORD1; // 1つ目の追加UV
    float4 additionalUV1 : TEXCOORD2; // 2つ目の追加UV
    float4 additionalUV2 : TEXCOORD3; // 3つ目の追加UV
    float4 additionalUV3 : TEXCOORD4; // 4つ目の追加UV

    uint4  boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHTS;

    // SDEF Data
    // これらのセマンティクスはアプリケーションとシェーダーで合意したカスタムセマンティクスでも良い
    float3 sdef_c : TEXCOORD5; // または CUSTOM0 など
    float3 sdef_r0 : TEXCOORD6; // または CUSTOM1 など
    float3 sdef_r1 : TEXCOORD7; // または CUSTOM2 など

    float edge : BLENDFACTOR; // または PSIZE (ポイントサイズ) など、適切なセマンティクスを使用
                                  // もしくは TEXCOORD8 や CUSTOM3 など
};

struct Output
{

    float4 svpos    : SV_POSITION; // 画面への投影座標（これは VS の出力に必要）
    float4 pos      : POSITION1;   // ワールド空間の頂点位置
    float4 normal   : NORMAL;      // 法線（W成分は0.0f）
    float2 uv       : TEXCOORD0;   // テクスチャ座標
    float4 vnormal  : TEXCOORD1;   // スフィアマップ用UV計算に使う法線（通常はワールド空間法線）
    float3 ray      : TEXCOORD2;   // 視線ベクトル (頂点から視点へ)
};

SamplerState smp : register(s0);
SamplerState smpToon : register(s1);

cbuffer SceneBuffer : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float3 eye;
    float padding;
};

cbuffer Transform : register(b1)
{
    matrix world; // ワールド変換行列
    uint BoneCount; // ボーンの総数.
    float3 Padding; // パディング
};

cbuffer Material : register(b2)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
};

Texture2D<float4> tex : register(t0);
Texture2D<float4> toon : register(t1);
Texture2D<float4> sph : register(t2);

StructuredBuffer<float4x4> boneTransforms : register(t3);

