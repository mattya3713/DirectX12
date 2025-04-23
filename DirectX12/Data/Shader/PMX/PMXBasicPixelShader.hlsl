#include "PmxShaderHeader.hlsli"

float4 PS(Output input) : SV_TARGET
{
    float3 lightDir = normalize(float3(1, -1, 1));
    float3 normal = normalize(input.normal.xyz);

    // 環境光
    float3 ambientColor = ambient * diffuse.rgb;

    // 拡散反射
    float diffuseIntensity = saturate(dot(normal, -lightDir));
    float3 diffuseColor = diffuse.rgb * diffuseIntensity;

    // 鏡面反射（視線方向必要）
    float3 viewDir = normalize(eye - input.svpos.xyz);
    float3 reflectDir = reflect(lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specular.a); // shininessはalphaに格納してると仮定
    float3 specularColor = specular.rgb * spec;

    // ベーステクスチャ
    float4 texColor = tex.Sample(smp, input.uv);

    // トゥーン適用（トーン段階で離散化）
    float toonStep = floor(diffuseIntensity * 5.0) / 5.0;
    float4 toonColor = toon.Sample(smpToon, float2(toonStep, 0.5));

    // スフィアマップはアドオン的に加算
    float3 sphColor = float3(0, 0, 0);
    if (useSphereMap > 0.5f)
    {
        float3 r = reflect(-viewDir, normal);
        float2 sphUV = normalize(r.xy) * 0.5 + 0.5;
        sphColor = sph.Sample(smp, sphUV).rgb;
    }
    
    // 最終カラー合成
    float3 finalColor = (ambientColor + diffuseColor) * texColor.rgb * toonColor.rgb + specularColor + sphColor;

    return float4(finalColor, texColor.a);
}