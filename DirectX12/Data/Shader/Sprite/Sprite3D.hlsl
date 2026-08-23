// Sprite3D(ビルボード)用シェーダー.
// 既存SceneBuffer(b0: view/proj)のカメラ基底で頂点を展開し、常にカメラ正面を向かせる.

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

// DirectX12::SceneBufferと同じレイアウトの先頭部分(view/projのみ使用).
cbuffer SceneBuffer : register(b0)
{
    matrix view;
    matrix proj;
};

struct VSInput
{
    float3 center : POSITION;  // ビルボード中心(ワールド座標).
    float2 corner : TEXCOORD0; // 頂点オフセット(-0.5..0.5).
    float2 size   : TEXCOORD1; // 幅・高さ(ワールド単位).
    float2 uv     : TEXCOORD2;
    float4 color  : COLOR0;
};

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 color : COLOR0;
};

VSOutput VS(VSInput input)
{
    VSOutput output;

    // ビュー行列の第0行=カメラRight、第1行=カメラUp(既存のmul(view, p)規約に対応).
    const float3 cam_right = normalize(float3(view._11, view._12, view._13));
    const float3 cam_up    = normalize(float3(view._21, view._22, view._23));

    const float3 world = input.center
        + cam_right * (input.corner.x * input.size.x)
        + cam_up    * (input.corner.y * input.size.y);

    output.svpos = mul(proj, mul(view, float4(world, 1.0f)));
    output.uv    = input.uv;
    output.color = input.color;

    return output;
}

float4 PS(VSOutput input) : SV_TARGET
{
    return tex.Sample(smp, input.uv) * input.color;
}
