#include "common_inc.fxh"

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    output.Pos = mul(input.Pos, World);
    output.WorldPos = output.Pos.xyz;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    output.Norm = normalize(mul(input.Norm, (float3x3) World));
    output.Tan = normalize(mul(input.Tan, (float3x3) World));
    output.Tex = input.Tex;
    
    return output;
}

