#include "common_inc.fxh"

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    float4x4 noTransInView = View;
    
    noTransInView[3][0] = 0;
    noTransInView[3][1] = 0;
    noTransInView[3][2] = 0;

    float4 viewPos = mul(float4(input.Pos.xyz, 1), noTransInView);
    float4 clipSpacePos = mul(viewPos, Projection);
    
    output.Pos = clipSpacePos;
    output.Pos.z = output.Pos.w * 0.999999f; // 깊이 버퍼의 최대값에 가깝게 설정 <-- 스카이박스가 뷰클립에 걸리지 않도록 함
    
    output.Tex = float3(input.Pos.x, input.Pos.y, input.Pos.z);
    
    return output;
}


