cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Tex : TEXCOORD0;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    float4 scaledPos = mul(float4(input.Pos.xyz, 1), World);
    float4 viewPos = mul(scaledPos, View);
    float4 clipSpacePos = mul(viewPos, Projection);
    
    output.Pos = clipSpacePos;
    output.Pos.z = output.Pos.w * 0.999999f; // 깊이 버퍼의 최대값에 가깝게 설정 <-- 스카이박스가 뷰클립에 걸리지 않도록 함
    
    output.Tex = input.Pos;
    
    return output;
}


