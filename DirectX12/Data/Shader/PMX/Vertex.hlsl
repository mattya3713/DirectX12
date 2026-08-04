#include "Header.hlsli"

Output VS(VSInput input)
{
    Output output;

    // --- 修正点1: 初期化は全て 0.0 で行う ---
    float4 skinnedPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 skinnedNormal = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float totalWeight = 0.0f;

    for (int i = 0; i < 4; ++i)
    {
        float boneWeight = input.boneWeights[i]; // 変数キャッシュ

        if (boneWeight > 0.0f)
        {
            uint boneIndex = input.boneIndices[i];

            if (boneIndex < BoneCount)
            {
                float4x4 boneTransform = boneTransforms[boneIndex];

                // 位置のスキニング
                // w成分は計算結果に重みを掛けて加算するだけでよい
                skinnedPos += mul(boneTransform, float4(input.pos.xyz, 1.0f)) * boneWeight;
                
                // --- 修正点2: ループ内での w=1.0 の強制代入は削除 ---
                
                // 法線のスキニング (回転のみ考慮)
                skinnedNormal += mul(boneTransform, float4(input.normal.xyz, 0.0f)) * boneWeight;
            }
            totalWeight += boneWeight;
        }
    }
    
    // フォールバックと正規化
    if (totalWeight <= 0.0001f)
    {
        skinnedPos = float4(input.pos.xyz, 1.0f);
        skinnedNormal = float4(input.normal.xyz, 0.0f);
    }
    else
    {
        // ウェイトの合計で割って正規化 (重心座標を保つ)
        skinnedPos /= totalWeight;
        skinnedNormal /= totalWeight;
        
        // --- 修正点3: 最後に必ず w=1.0 に設定する ---
        // これにより、totalWeightでの除算によるwのズレを修正し、
        // 次のワールド行列乗算を正しく行えるようにする。
        skinnedPos.w = 1.0f;
    }


    // --- 以下の行列演算順序は環境に合っているならそのままでOK ---

    // モデル変換
    float4 worldPos = mul(world, skinnedPos);
    
    // 法線の変換
    // 法線は方向ベクトルなので w=0 として扱う（または (float3x3)world を掛ける）
    // 正規化は最後に行うのでここでは変換のみ
    float4 worldNormal = mul(world, float4(skinnedNormal.xyz, 0.0f));

    // ビュー・プロジェクション変換
    float4 viewPos = mul(view, worldPos);
    output.svpos = mul(proj, viewPos);

    // 出力
    output.uv = input.uv;
    output.normal = float4(normalize(worldNormal.xyz), 0.0f); // ここで正規化
    output.pos = worldPos;
    output.vnormal = float4(normalize(worldNormal.xyz), 0.0f);
    output.ray = float4(normalize(eye - worldPos.xyz), 0.0f);

    return output;
}