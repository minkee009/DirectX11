#include "common_inc.fxh"

Texture2D txDiffuse : register(t0);
TextureCube skyBoxTX : register(t1);
SamplerState samLinear : register(s0);


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float2 Tex : TEXCOORD2;
};

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 ambient = ambientStr * vAmbientColor;
    
    float3 norm = normalize(input.Norm);
    float3 I = normalize(input.WorldPos - CameraPos.xyz);
    float3 R = reflect(I, norm);  //큐브맵 반사를 위한 리플렉트 벡터
    R.x = -R.x;
    float3 L = normalize(vLightPos.xyz - input.WorldPos);
    
    float diff = max(dot(norm, L), 0.0);
    float4 diffuse = diffuseStr * diff * vLightColor;
    
    float3 viewDir = normalize(CameraPos.xyz - input.WorldPos);
    float3 reflectDir = reflect(-L,norm);
    
    float spec = pow(saturate(dot(reflectDir, viewDir)), shininess);
    float4 specular = specularStr * spec * vLightColor;
    
    float3 baseRGB = (specular + diffuse + ambient).rgb * txDiffuse.Sample(samLinear, input.Tex).rgb;
    float3 envRGB = (specular + diffuse + ambient).rgb * skyBoxTX.Sample(samLinear, R).rgb;

    float reflectionFactor = 0.6f;
    float3 finalRGB = lerp(baseRGB, envRGB, reflectionFactor);

    return float4(finalRGB, 1.0f);
}

float4 PSSolid(PS_INPUT input) : SV_Target
{
    return vOutputColor;
}