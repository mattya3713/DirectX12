// パーティクル用頂点シェーダー(CPU側でビルボード展開済みのワールド座標を変換するだけ).

struct SceneData
{
    float4x4 view;   // ビュー行列(DirectX12のSceneDataと同じレイアウト先頭部).
    float4x4 proj;   // 射影行列.
    float3   eye;    // 視点座標(未使用).
    float    padding;
};

cbuffer SceneBuffer : register(b0)
{
    SceneData g_scene;
}

struct VSInput
{
    float3 pos   : POSITION;
    float4 color : COLOR0;
};

struct PSInput
{
    float4 svpos : SV_POSITION;
    float4 color : COLOR0;
};

PSInput main(VSInput input)
{
    PSInput output;

    const float4 world = float4(input.pos, 1.0f);
    output.svpos = mul(g_scene.proj, mul(g_scene.view, world));
    output.color = input.color;

    return output;
}
