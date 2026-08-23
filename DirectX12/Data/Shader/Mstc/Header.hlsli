// Mstc(静的メッシュ)用シェーダー共用定義.
// 頂点は位置・法線・UVのみ(タンジェント無し). 法線マップはオブジェクト空間絶対法線.

struct MstcVsInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct MstcVsOutput
{
    float4 svpos : SV_POSITION;     // 画面への投影座標.
    float4 pos : POSITION1;         // ワールド空間の頂点位置.
    float4 normal : NORMAL;         // ワールド空間法線(w=0).
    float4 localNormal : TEXCOORD1; // ローカル空間法線(フォールバック用. w=0).
    float2 uv : TEXCOORD0;
    float3 ray : TEXCOORD2;         // 視線ベクトル(頂点から視点へ).
};

SamplerState smp : register(s0);

cbuffer SceneBuffer : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float3 eye;
    float padding;
};

cbuffer Transform : register(b1)
{
    matrix world; // ワールド変換行列(剛体オブジェクト. スキニング無し).
};

cbuffer Material : register(b2)
{
    float4 diffuse;             // rgb=拡散色, a=アルファ.
    float4 specularAmount;      // rgb=鏡面反射色, a=鏡面反射強度.
    float4 ambientUseNormalMap; // rgb=環境光色, a=法線マップ使用フラグ.
};

Texture2D<float4> baseTex : register(t0);       // ベースカラー.
Texture2D<float4> normalMapTex : register(t1);  // オブジェクト空間法線マップ(未使用時は白テクスチャ).
