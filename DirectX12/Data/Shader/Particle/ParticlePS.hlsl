// パーティクル用ピクセルシェーダー(単色+アルファ. テクスチャ無しの最小構成).

struct PSInput
{
    float4 svpos : SV_POSITION;
    float4 color : COLOR0;
};

float4 main(PSInput input) : SV_TARGET
{
    return input.color;
}
