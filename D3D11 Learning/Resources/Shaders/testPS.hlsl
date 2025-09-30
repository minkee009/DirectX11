#include "common_inc.hlsli"

Texture2D txDiffuse : register(t0);
TextureCube skyBoxTX : register(t1);
Texture2D normalMap : register(t2);
Texture2D specularMap : register(t3);
SamplerState samLinear : register(s0);


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float3 Binorm : TEXCOORD3;
    float2 Tex : TEXCOORD4;
};

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 ambient = ambientStr * vAmbientColor;
    
    float3 normalTex = normalMap.Sample(samLinear, input.Tex).xyz * 2.0f - 1.0f; //정규화
    
    float3x3 TBN = float3x3(
        input.Tan.x, input.Tan.y, input.Tan.z, 
        input.Binorm.x, input.Binorm.y, input.Binorm.z, 
        input.Norm.x, input.Norm.y, input.Norm.z     
    );
    
    normalTex = normalize(mul(normalTex, TBN)); //TBN 행렬을 곱해서 월드공간으로 변환
    
    float3 norm = normalTex;
    float3 I = normalize(input.WorldPos - CameraPos.xyz);
    float3 R = reflect(I, norm);  //큐브맵 반사를 위한 리플렉트 벡터
    R.x = -R.x;
    float3 L = isPointLight ? normalize(vLightPos.xyz - input.WorldPos) : vLightDir.xyz;
    
    float diff = max(dot(norm, L), 0.0);
    float4 diffuse = diffuseStr * diff * vLightColor;
    
    float3 viewDir = normalize(CameraPos.xyz - input.WorldPos);
    float3 halfDir = normalize(viewDir + L); //스펙큘러연산을 위한 하프 벡터
    
    float specTex = specularMap.Sample(samLinear, input.Tex).r;
    float spec = pow(saturate(dot(halfDir, norm)), shininess) * sqrt(diff); // * sqrt(diff) <- 이걸 쓰면 shininess < 32 에서 아티팩트가 사라짐..!!! 
    float4 specular = specularStr * specTex * spec * vLightColor;
    
    float3 baseRGB = (specular + diffuse + ambient).rgb * txDiffuse.Sample(samLinear, input.Tex).rgb;
    float3 envRGB = (specular + diffuse + ambient).rgb * skyBoxTX.Sample(samLinear, R).rgb;
    
    float3 finalRGB = lerp(baseRGB, envRGB, reflectionFactor);

    return float4(finalRGB, 1.0f);
}

float4 PSSolid(PS_INPUT input) : SV_Target
{
    return vOutputColor;
}