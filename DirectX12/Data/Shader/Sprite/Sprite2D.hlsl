// Sprite2D用シェーダー(画面座標→NDC変換済みの頂点をそのまま描く. 射影変換なし).

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct VSInput
{
    float2 pos    : POSITION; // NDC座標(-1..1).
    float2 uv     : TEXCOORD0;
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
    output.svpos = float4(input.pos, 0.0f, 1.0f);
    output.uv    = input.uv;
    output.color = input.color;
    return output;
}

float4 PS(VSOutput input) : SV_TARGET
{
    return tex.Sample(smp, input.uv) * input.color;
}
