// ポストプロセス共用定義(フルスクリーンパス).

Texture2D<float4> tex0 : register(t0); // 入力1(シーンカラー等).
Texture2D<float4> tex1 : register(t1); // 入力2(CompositeのBloom合成元. 単独パスでは未使用).

SamplerState smp : register(s0);

// パラメータ(Passごとに意味が変わる. RootConstantsで設定).
//   BrightExtract : Param0.x=輝度しきい値
//   GaussianBlur  : Param0.xy=ぼかし方向(正規化済みテクセル単位)
//   Composite     : Param0.y=Bloom強度
cbuffer Params : register(b0)
{
    float4 Param0;
    float4 Param1;
};

struct VsOutput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// SV_VertexIDによるフルスクリーントライアングル(頂点バッファ不要).
VsOutput FullscreenVS(uint VertexId : SV_VertexID)
{
    VsOutput output;
    output.uv   = float2((VertexId << 1) & 2, VertexId & 2);
    output.svpos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}
