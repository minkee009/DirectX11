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
    // 나중에 햇빛 블루밍 처리를 위해 쉐이더코드를 분리
    return skyBoxTX.Sample(samLinear, normalize(input.Tex));
}