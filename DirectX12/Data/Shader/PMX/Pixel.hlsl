#include "Header.hlsli"

float4 PS(Output input) : SV_TARGET
{
	float3 light = normalize(lightDirection.xyz); //光の向かうベクトル(平行光線. DirectionLightからCBuffer経由で受ける)
    float3 lightCol = lightColor.rgb; //ライトのカラー(DirectionLightからCBuffer経由で受ける)

	//ディフューズ計算(シャドウ有効時はPCF係数でトゥーン輝度を落とす)
    float diffuseB = saturate(dot(-light, input.normal.xyz));
    if (lightColor.a > 0.5f)
    {
        diffuseB *= lerp(0.35f, 1.0f, CalcShadowFactor(input.pos)); // 影側も完全な黒にせずトゥーンの階調を残す.
    }
    float4 toonDif = UseToonMap > 0.5 ? toon.Sample(smpToon, float2(0, 1.0 - diffuseB)) : float4(1, 1, 1, 1);

	//光の反射ベクトル
    float3 refLight = normalize(reflect(light, input.normal.xyz));
    float specularB = 0.0f;
    if (any(specular.rgb > 0.0f) && specular.a > 0.0f)
    {
        specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);
    }
	
	//スフィアマップ用UV
    float2 sphereMapUV = input.vnormal.xy;
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);
    
    float4 ambCol = float4(ambient * 0.6, 1);
    
    float4 texColor = tex.Sample(smp, input.uv); //テクスチャカラー

	float4 color = saturate(toonDif //輝度(トゥーン)
		* diffuse //ディフューズ色
		* texColor //テクスチャカラー
		* float4(lightCol, 1) //ライト色(DirectionLightで調整可能にするため乗算)
		* sph.Sample(smp, sphereMapUV)) //スフィアマップ(乗算)
		+ float4(specularB * specular.rgb, 1) //スペキュラー
		+ float4(texColor.xyz * ambient * 0.5, 1); //アンビエント(明るくなりすぎるので0.5にしてます)
	color.a = saturate(diffuse.a * texColor.a);
	return color;
}
