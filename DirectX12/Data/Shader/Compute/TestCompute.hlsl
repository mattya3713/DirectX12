// Async Compute動作確認用コンピュートシェーダー.
// DispatchThreadIDから決定的な値を書き込み、読み戻し検証で正しさを確認する.

struct TestInput
{
    float base; // 全要素に加算する基準値(定数バッファ相当の代わりにルート定数でも可. v1は未使用).
};

RWStructuredBuffer<float> outputBuffer : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    // 検証用の決定的な式: 期待値 = base + index * 2.0
    outputBuffer[dtid.x] = 1.0f + (float)dtid.x * 2.0f;
}
