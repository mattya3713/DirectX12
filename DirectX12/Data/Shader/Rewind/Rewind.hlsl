// 巻き戻り演出用のフルスクリーン.blit(キャプチャ縮小コピーと逆再生表示で共用).

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float2 uv    : TEXCOORD0;
};

// SV_VertexIDだけでフルスクリーンの三角形を作る(頂点バッファ不要).
VSOutput VS(uint vid : SV_VertexID)
{
    VSOutput output;

    const float2 pos = float2((vid << 1) & 2, vid & 2);
    output.svpos = float4(pos.x * 2.0f - 1.0f, 1.0f - pos.y * 2.0f, 0.0f, 1.0f);
    output.uv    = pos;

    return output;
}

float4 PS(VSOutput input) : SV_TARGET
{
    return tex.Sample(smp, input.uv);
}
