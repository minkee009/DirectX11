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
    // 뷰 이동성분을 제거
    float4x4 viewWithoutTranslation = View;
    viewWithoutTranslation[3][0] = 0;
    viewWithoutTranslation[3][1] = 0;
    viewWithoutTranslation[3][2] = 0;

    // 정점의 위치를 투영
    output.Pos = mul(float4(input.Pos, 1.0f), mul(viewWithoutTranslation, Projection));
    output.Pos.z = output.Pos.w * 0.999999f;

    // 픽셀 셰이더로 정점의 위치를 텍스처 좌표로 전달
    output.Tex = input.Pos;
    
    return output;
}


