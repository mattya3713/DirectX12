#include "Header.hlsli"

// ガウシアンブラー(9タップ). Param0.xyにテクセル単位のぼかし方向を設定する
// (水平パス=(1/幅,0)、垂直パス=(0,1/高さ). 2パスで簡易2Dブラーになる).
float4 PS(VsOutput input) : SV_TARGET
{
    const float weights[5] = { 0.227f, 0.194f, 0.121f, 0.054f, 0.016f };

    float4 color = tex0.Sample(smp, input.uv) * weights[0];

    for (int i = 1; i < 5; ++i)
    {
        const float2 offset = Param0.xy * float(i);
        color += tex0.Sample(smp, input.uv + offset) * weights[i];
        color += tex0.Sample(smp, input.uv - offset) * weights[i];
    }

    return color;
}
