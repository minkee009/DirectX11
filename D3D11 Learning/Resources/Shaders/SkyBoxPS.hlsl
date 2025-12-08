#include "common_inc.fxh"

TextureCube skyBoxTX : register(t0);
SamplerState samLinear : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Tex : TEXCOORD0;
};

float4 PS(PS_INPUT input) : SV_Target
{
    float gammaCorrect = pow(rimLightStr, 1.0 / 2.2);
    float3 skyBoxColor = skyBoxTX.Sample(samLinear, normalize(input.Tex));
    
    // 나중에 햇빛 블루밍 처리를 위해 쉐이더코드를 분리
    return float4(skyBoxColor * gammaCorrect,1.0f);
}