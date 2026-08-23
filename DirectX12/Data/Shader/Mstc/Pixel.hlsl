#include "Header.hlsli"

float4 PS(MstcVsOutput input) : SV_TARGET
{
    float3 light      = normalize(float3(1, -1, 1)); // 光の向かうベクトル(平行光源).
    float3 lightColor = float3(1, 1, 1);

    float4 texColor = baseTex.Sample(smp, input.uv);

    // 使用法線: 法線マップ有効時はオブジェクト空間絶対法線(テクスチャ値をローカル座標系の
    // 法線方向として復元し、ワールド回転のみ適用). 無効時はジオメトリ法線.
    float3 n = input.normal.xyz;
    if (ambientUseNormalMap.a > 0.5)
    {
        float3 objN = normalMapTex.Sample(smp, input.uv).xyz * 2.0 - 1.0;
        n = normalize(mul(objN, (float3x3)world));
    }

    float diffuseB = saturate(dot(-light, n));

    // 鏡面反射(反射ベクトル).
    float3 refLight  = normalize(reflect(light, n));
    float specularB  = 0.0f;
    if (any(specularAmount.rgb > 0.0f) && specularAmount.a > 0.0f)
    {
        specularB = pow(saturate(dot(refLight, -input.ray)), specularAmount.a);
    }

    float4 ambCol = float4(ambientUseNormalMap.rgb * 0.6, 1);

    // PMXのPixel.hlslと同じ構成(トゥーン/スフィア無し版).
    float4 color = saturate(float4(lightColor, 1.0f) * diffuse * texColor * diffuseB)
        + float4(specularB * specularAmount.rgb, 1)
        + float4(texColor.xyz * ambientUseNormalMap.rgb * 0.5, 1);
    color.a = saturate(diffuse.a * texColor.a);
    return color;
}
