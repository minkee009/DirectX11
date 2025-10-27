#include "common_inc.fxh"

// 스키닝용 본 행렬
cbuffer BoneModelMatrices : register(b2)
{
    matrix BoneModels[96];
};

cbuffer BoneOffsetMatrices : register(b3)
{
    matrix BoneOffsets[96];
};

// 입력 구조체
struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD0;
    float4 BoneIndices : BLENDINDICES;
    float4 BoneWeights : BLENDWEIGHT;
};

// 출력 구조체 (깊이만 필요)
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // 스키닝 계산
    float4 pos = float4(input.Pos, 1.0f);
    float4 skinnedPos = float4(0, 0, 0, 0);
    
    for (int i = 0; i < 4; i++)
    {
        int boneIndex = (int) input.BoneIndices[i];
        float weight = input.BoneWeights[i];
        
        if (weight > 0.0f)
        {
            matrix boneTransform = mul(BoneOffsets[boneIndex], BoneModels[boneIndex]);
            skinnedPos += weight * mul(pos, boneTransform);
        }
    }
    
    // 광원 시점으로 변환 (World * LightView * LightProjection)
    output.Pos = mul(skinnedPos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    return output;
}