#include "LiteralShaderCode.h"

namespace MyEngine::D3DCTX
{
    const char* g_vscode_def = R"(
struct VS_INPUT                                   
{                                                 
	float4 Pos : POSITION;       // float4 -> float3
	float3 Norm : NORMAL;                         
	float3 Tan : TANGENT;                         
	float2 Tex : TEXCOORD0;               
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;        
};                                                
			                                                   
struct PS_INPUT                                   
{                                                 
	float4 Pos : SV_POSITION;                     
};                                                
			                                                   
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}                                            
			                                                   
PS_INPUT VS(VS_INPUT input)                       
{                                                 
	PS_INPUT output = (PS_INPUT)0;                
			                                                   
	// 변환                                        
	float4 worldPos = mul(input.Pos, World);     
	float4 viewPos = mul(worldPos, View);                     
	output.Pos = mul(viewPos, Projection);                    
			                                                   
	return output;                                
}
)";
    const char* g_vscode_outline_static = R"(
struct VS_INPUT                                   
{                                                 
	float4 Pos : POSITION;     
	float3 Norm : NORMAL;                         
	float3 Tan : TANGENT;                         
	float2 Tex : TEXCOORD0;               
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;        
};                                                
			                                                   
struct PS_INPUT                                   
{                                                 
	float4 Pos : SV_POSITION;                     
};                                                
			                                                   
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}      

cbuffer ConstantBuffer : register(b4)
{
    float OutlineThickness;
}                                 
			                                                   
PS_INPUT VS(VS_INPUT input)                       
{                                                 
	PS_INPUT output = (PS_INPUT)0;                
                              
	// 변환                                        
	float4 worldPos = mul(input.Pos, World);     

	float3 N = normalize(input.Norm);
	float3 expanded = worldPos + N * OutlineThickness;
	float4 expandedPos = float4(expanded, 1.0);

	float4 viewPos = mul(expandedPos, View);                     
	output.Pos = mul(viewPos, Projection);                    
			                                                   
	return output;                                
}
)";
    const char* g_vscode_outline_skinning = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
	matrix LightViewProjection;
	float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

cbuffer BoneModelBuffer : register(b2)
{
	matrix ModelMatricies[128];
}

cbuffer BoneOffsetBuffer : register(b3)
{
	matrix OffsetMatricies[128];
}

cbuffer ConstantBuffer : register(b4)
{
    float OutlineThickness;
}               

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
	
    matrix skinningMatrix =  {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };

    for (int i = 0; i < 4; i++)
    {
        skinningMatrix += mul(OffsetMatricies[input.BoneIndices[i]],ModelMatricies[input.BoneIndices[i]]) * input.BoneWeights[i] ;
    }

	matrix finalWorld = mul(skinningMatrix,World);

				                     

    output.Pos = mul(input.Pos, finalWorld);
    output.WorldPos = output.Pos.xyz;
    output.Norm = normalize(mul(input.Norm, (float3x3) finalWorld));

	float3 expanded = output.WorldPos + output.Norm * OutlineThickness;
	float4 expandedPos = float4(expanded, 1.0);


    output.Pos = mul(expandedPos, View);
    output.Pos = mul(output.Pos, Projection);

	// finalWorld 행렬이 적용된 위치를 LightViewProjection으로 변환
	output.LightPos = mul(float4(output.WorldPos, 1.0f),LightViewProjection);
    
    output.Tan = normalize(mul(input.Tan, (float3x3) finalWorld));
    output.Tex = input.Tex;
    
    return output;
}
)";
    const char* g_vscode_outline_rigid = R"(
    cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

cbuffer BoneBuffer : register(b2)
{
	matrix ModelMatricies[128];
}
cbuffer BoneBuffer : register(b3)
{
	uint boneIdx;
}
cbuffer ConstantBuffer : register(b4)
{
    float OutlineThickness;
}               

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
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
	matrix tWorld = mul(ModelMatricies[boneIdx], World);

    output.Pos = mul(input.Pos, tWorld);
    output.WorldPos = output.Pos.xyz;

	float3 N = normalize(input.Norm);
	float3 expanded = output.WorldPos + N * OutlineThickness;
	float4 expandedPos = float4(expanded, 1.0);

    output.Pos = mul(expandedPos, View);
    output.Pos = mul(output.Pos, Projection);
    
    output.Norm = normalize(mul(input.Norm, (float3x3) tWorld));
    output.Tan = normalize(mul(input.Tan, (float3x3) tWorld));
    output.Tex = input.Tex;
    
    return output;
}

)";

    const char* g_vscode_common_static = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    output.Pos = mul(input.Pos, World);
    output.WorldPos = output.Pos.xyz;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
	// finalWorld 행렬이 적용된 위치를 LightViewProjection으로 변환
	output.LightPos = mul(float4(output.WorldPos, 1.0f),LightViewProjection);

    output.Norm = normalize(mul(input.Norm, (float3x3) World));
    output.Tan = normalize(mul(input.Tan, (float3x3) World));
    output.Tex = input.Tex;
    
    return output;
}
)";
    const char* g_vscode_common_rigid = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

cbuffer BoneBuffer : register(b2)
{
	matrix ModelMatricies[128];
}
cbuffer BoneBuffer : register(b3)
{
	uint boneIdx;
}


struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
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
	matrix tWorld = mul(ModelMatricies[boneIdx], World);

    output.Pos = mul(input.Pos, tWorld);
    output.WorldPos = output.Pos.xyz;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    output.Norm = normalize(mul(input.Norm, (float3x3) tWorld));
    output.Tan = normalize(mul(input.Tan, (float3x3) tWorld));
    output.Tex = input.Tex;
    
    return output;
}
)";

    const char* g_vscode_common_skinning = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

cbuffer BoneModelBuffer : register(b2)
{
	matrix ModelMatricies[128];
}

cbuffer BoneOffsetBuffer : register(b3)
{
	matrix OffsetMatricies[128];
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
	
    matrix skinningMatrix =  {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };

    for (int i = 0; i < 4; i++)
    {
        skinningMatrix += mul(OffsetMatricies[input.BoneIndices[i]],ModelMatricies[input.BoneIndices[i]]) * input.BoneWeights[i] ;
    }

	matrix finalWorld = mul(skinningMatrix,World);

    output.Pos = mul(input.Pos, finalWorld);
    output.WorldPos = output.Pos.xyz;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

	// finalWorld 행렬이 적용된 위치를 LightViewProjection으로 변환
	output.LightPos = mul(float4(output.WorldPos, 1.0f),LightViewProjection);
    
    output.Norm = normalize(mul(input.Norm, (float3x3) finalWorld));
    output.Tan = normalize(mul(input.Tan, (float3x3) finalWorld));
    output.Tex = input.Tex;
    
    return output;
}
)";

}

namespace MyEngine::D3DCTX
{
    const char* g_pscode_def = R"(			 
struct PS_INPUT                                    
{                                                 
	float4 Pos : SV_POSITION;                     
};                                                
			                                                   
float4 PS(PS_INPUT input) : SV_Target             
{                                                 
	return float4(1.0f, 0.0f, 1.0f, 1.0f);        
}  
)";

    const char* g_pscode_outline = R"(			 
struct PS_INPUT                                    
{                                                 
	float4 Pos : SV_POSITION;                     
};                                                
			                                                   
float4 PS(PS_INPUT input) : SV_Target             
{                                                 
	return float4(0.0f, 0.0f, 0.0f, 1.0f);        
}  
)";

    const char* g_pscode_blinnphong = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

cbuffer MaterialBuffer : register(b1)
{
	uint textureFlags; 
    uint propertyFlags;
	float4 baseColor;
    float metallic;
    float roughness;
}

Texture2D txDiffuse : register(t0);
TextureCube skyBoxTX : register(t1);
Texture2D normalMap : register(t2);
Texture2D specularMap : register(t3);
Texture2D emmisiveMap : register(t4);
Texture2D lutMap : register(t9);
SamplerState samLinear : register(s0);
Texture2D shadowMap : register(t10); 
SamplerComparisonState samShadow : register(s1);


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4; 
	uint  IsFrontFace : SV_IsFrontFace; 
};

// 그림자 테스트 함수 (0.0: 그림자, 1.0: 밝음)
float CalculateShadow(float4 LightPos, float3 normal, float3 lightDir)
{
    float3 projCoords = LightPos.xyz / LightPos.w;
    float2 texCoords; 
    texCoords.x = projCoords.x * 0.5f + 0.5f;
    texCoords.y = -projCoords.y * 0.5f + 0.5f;
    
    if (texCoords.x < 0.0f || texCoords.x > 1.0f || 
        texCoords.y < 0.0f || texCoords.y > 1.0f)
        return 1.0f;
    
    float currentDepth = projCoords.z;
    if (currentDepth < 0.0f || currentDepth > 1.0f)
        return 1.0f;
    
    // 각도 기반 동적 바이어스 (가장 효과적!)
    float cosTheta = saturate(dot(normal, lightDir));
    float bias = 0.0005f * tan(acos(cosTheta));
    bias = clamp(bias, 0.0f, 0.01f);
    
    currentDepth -= bias;
    
    float shadow = shadowMap.SampleCmpLevelZero(samShadow, texCoords, currentDepth);
    return shadow;
}

// 수동 PCF
float CalculateShadowPCF(float4 LightPos)
{
    float3 projCoords = LightPos.xyz / LightPos.w;
    float2 texCoords; 
    texCoords.x = projCoords.x * 0.5f + 0.5f;
    texCoords.y = -projCoords.y * 0.5f + 0.5f;
    
    if (texCoords.x < 0.0f || texCoords.x > 1.0f || 
        texCoords.y < 0.0f || texCoords.y > 1.0f)
        return 1.0f;
    
    float currentDepth = projCoords.z;
    if (currentDepth < 0.0f || currentDepth > 1.0f)
        return 1.0f;
    
    float bias = 0.005f;
    currentDepth -= bias;
    
    // 3x3 PCF
    float shadow = 0.0f;
    float2 texelSize = 1.0f / 2048.0f; // Shadow Map 크기
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(samShadow, texCoords + offset, currentDepth);
        }
    }
    shadow /= 9.0f;
    
    return shadow;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 ambient = ambientStr * vAmbientColor;
    
    float3 normalTex = normalMap.Sample(samLinear, input.Tex).xyz * 2.0f - 1.0f; // 정규화


	float3 N = normalize(input.Norm);
  

	float3 T = normalize(input.Tan);
	T = normalize(T - dot(T, N) * N); // N에 직교하도록 조정
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
    
    normalTex = normalize(mul(normalTex, TBN)); // TBN 행렬을 곱해서 월드공간으로 변환
    
    N = lerp(N, normalTex, (textureFlags & 4) != 0);
    float3 I = normalize(input.WorldPos - CameraPos.xyz);
    float3 R = reflect(I, N);  // 큐브맵 반사를 위한 리플렉트 벡터
    R.x = -R.x;
    float3 L = -vLightDir.xyz;
    
    // 조명 위치와 픽셀 위치
    float3 toLight = vLightPos - input.WorldPos;
    float distance = length(toLight);

    // 감쇠 계수 (1 / d² 형태)
    //float attenuation = 1.0f / (distance * distance);
    
    float lightDist = 1.0f;
    
	float shadow = CalculateShadowPCF(input.LightPos); // 그림자 인자 계산

    float diff = saturate(dot(N, L));
	//float bandLevel = 1.0f;
	//diff = ceil(diff * bandLevel)/bandLevel;
	//diff = lutMap.Sample(samLinear, float2((diff * 0.5f) + 0.495f,0.5f)).r;  // 카툰렌더링 활성화
    float4 diffuse = diffuseStr * diff * vLightColor * lightDist * shadow;
    
    float3 viewDir = normalize(CameraPos.xyz - input.WorldPos);
    float3 halfDir = normalize(viewDir + L); // 스펙큘러연산을 위한 하프 벡터
    
    float specTex = lerp(1.0f, specularMap.Sample(samLinear, input.Tex).r,(textureFlags & 2) != 0);
    float spec = pow(saturate(dot(halfDir, N)), shininess) * sqrt(diff); // * sqrt(diff) <- 이걸 쓰면 shininess < 32 에서 아티팩트가 사라짐..!!! 
    
	//spec = smoothstep(0.01, 0.02f, spec); // <- 카툰렌더링용 스펙큘러
	float4 specular = specularStr * specTex * spec * vLightColor * lightDist * shadow;
    
	float4 emmisive = lerp(float4(0, 0, 0, 0), emmisiveMap.Sample(samLinear, input.Tex),(textureFlags & 8) != 0);

    // 알파 클리핑용 디퓨즈 샘플링
    float4 baseTex = lerp(baseColor, txDiffuse.Sample(samLinear, input.Tex),(textureFlags & 1) != 0);

    // 알파 임계값 설정 (0.1~0.5 정도 보통 사용)
    const float alphaCutoff = 0.5f;

    // 알파가 낮으면 픽셀 폐기
    clip(baseTex.a - alphaCutoff);
    
    float3 baseRGB = (specular + diffuse + ambient).rgb * baseTex.rgb + emmisive.rgb;
    float3 envRGB = (specular + diffuse + ambient).rgb * skyBoxTX.Sample(samLinear, R).rgb;
    
    float3 finalRGB = lerp(baseRGB, envRGB, reflectionFactor);

    return float4(finalRGB, baseTex.a);
}
)";
    const char* g_pscode_blinnphong_toon = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

cbuffer MaterialBuffer : register(b1)
{
	uint textureFlags; 
    uint propertyFlags;
	float4 baseColor;
    float metallic;
    float roughness;
}

cbuffer GradientBuffer : register(b5)
{
	float3 gradientPos;
	float gradientIntensity;
}

Texture2D txDiffuse : register(t0);
Texture2D normalMap : register(t2);
Texture2D specularMap : register(t3);
Texture2D emmisiveMap : register(t4);
Texture2D lutMap : register(t9);
SamplerState samLinear : register(s0);
SamplerState samPoint : register(s2);
Texture2D shadowMap : register(t10); 
SamplerComparisonState samShadow : register(s1);


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4; 
	uint  IsFrontFace : SV_IsFrontFace; 
};

// 그림자 테스트 함수 (0.0: 그림자, 1.0: 밝음)
float CalculateShadow(float4 LightPos, float3 normal, float3 lightDir)
{
    float3 projCoords = LightPos.xyz / LightPos.w;
    float2 texCoords; 
    texCoords.x = projCoords.x * 0.5f + 0.5f;
    texCoords.y = -projCoords.y * 0.5f + 0.5f;
    
    if (texCoords.x < 0.0f || texCoords.x > 1.0f || 
        texCoords.y < 0.0f || texCoords.y > 1.0f)
        return 1.0f;
    
    float currentDepth = projCoords.z;
    if (currentDepth < 0.0f || currentDepth > 1.0f)
        return 1.0f;
    
    // 각도 기반 동적 바이어스 (가장 효과적!)
    float cosTheta = saturate(dot(normal, lightDir));
    float bias = 0.0005f * tan(acos(cosTheta));
    bias = clamp(bias, 0.0f, 0.01f);
    
    currentDepth -= bias;
    
    float shadow = shadowMap.SampleCmpLevelZero(samShadow, texCoords, currentDepth);
    return shadow;
}

// 수동 PCF
float CalculateShadowPCF(float4 LightPos)
{
    float3 projCoords = LightPos.xyz / LightPos.w;
    float2 texCoords; 
    texCoords.x = projCoords.x * 0.5f + 0.5f;
    texCoords.y = -projCoords.y * 0.5f + 0.5f;
    
    if (texCoords.x < 0.0f || texCoords.x > 1.0f || 
        texCoords.y < 0.0f || texCoords.y > 1.0f)
        return 1.0f;
    
    float currentDepth = projCoords.z;
    if (currentDepth < 0.0f || currentDepth > 1.0f)
        return 1.0f;
    
    float bias = 0.005f;
    currentDepth -= bias;
    
    // 3x3 PCF
    float shadow = 0.0f;
    float2 texelSize = 1.0f / 4096.0f; // Shadow Map 크기
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(samShadow, texCoords + offset, currentDepth);
        }
    }
    shadow /= 9.0f;
    
    return shadow;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 ambient = ambientStr * vAmbientColor;
    
    float3 normalTex = normalMap.Sample(samLinear, input.Tex).xyz * 2.0f - 1.0f; // 정규화


	float3 N = normalize(input.Norm);
  

	float3 T = normalize(input.Tan);
	T = normalize(T - dot(T, N) * N); // N에 직교하도록 조정
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
    
    normalTex = normalize(mul(normalTex, TBN)); // TBN 행렬을 곱해서 월드공간으로 변환
    
    N = lerp(N, normalTex, (textureFlags & 4) != 0);
    float3 I = normalize(input.WorldPos - CameraPos.xyz);
    float3 R = reflect(I, N);  // 큐브맵 반사를 위한 리플렉트 벡터
    R.x = -R.x;
    float3 L = -vLightDir.xyz;
	float RimNdotL = dot(vLightPos, N);
	float NdotL = saturate(dot(N, L));
    
    // 조명 위치와 픽셀 위치
    //float3 toLight =  - input.WorldPos;
    //float distance = length(toLight);

    // 감쇠 계수 (1 / d² 형태)
    //float attenuation = 1.0f / (distance * distance);
    
    //float lightDist = attenuation;
    
	float shadow = CalculateShadowPCF(input.LightPos); // 그림자 인자 계산
	float shadowLut = lutMap.Sample(samPoint, float2(shadow * 0.5f + 0.495f, 0.5f)).r;

    float diff = NdotL;
	
	//float bandLevel = 1.0f;
	//diff = ceil(diff * bandLevel)/bandLevel;
	float diffLut = lutMap.Sample(samPoint, float2((diff * 0.5f) + 0.495f,0.5f)).r;  // 카툰렌더링 활성화
	

	float diffShadow = min(shadowLut, diffLut); // 그림자와 조명값 중 작은 값을 사용
	diffShadow = diffShadow > lowLut ? diffShadow : (dot(N, -L) * diffGradientDistHalf + diffGradientDepth);

    float4 diffuse = diffuseStr * vLightColor * diffShadow;
    
    float3 viewDir = normalize(CameraPos.xyz - input.WorldPos);
    float3 halfDir = normalize(viewDir + L); // 스펙큘러연산을 위한 하프 벡터
    
    float specTex = lerp(1.0f, specularMap.Sample(samLinear, input.Tex).r,(textureFlags & 2) != 0);
    float spec = pow(saturate(dot(halfDir, N)), shininess) * sqrt(diff); // * sqrt(diff) <- 이걸 쓰면 shininess < 32 에서 아티팩트가 사라짐..!!! 
    
	spec = smoothstep(0.01, 0.02f, spec); // <- 카툰렌더링용 스펙큘러
	shadowLut = shadowLut > 0.999f ? 1.0f : 0.0f; // 카툰렌더링용 그림자
	shadowLut = diff > 0.6f ? shadowLut : 0.0f; // 디퓨즈가 낮으면 그림자 제거
	float4 specular = specularStr * specTex * spec * vLightColor * shadowLut;
    
	float4 emmisive = lerp(float4(0, 0, 0, 0), emmisiveMap.Sample(samLinear, input.Tex),(textureFlags & 8) != 0);

    // 알파 클리핑용 디퓨즈 샘플링
    float4 baseTex = lerp(baseColor, txDiffuse.Sample(samLinear, input.Tex),(textureFlags & 1) != 0);

    // 알파 임계값 설정 (0.1~0.5 정도 보통 사용)
    const float alphaCutoff = 0.5f;

    // 알파가 낮으면 픽셀 폐기
    clip(baseTex.a - alphaCutoff);

	float minusRimNdotL  = dot(-vLightPos.xyz , N);

	// 림 라이트
	float _RimAmount = 0.716;
	float4 rimDot = 1 - dot(viewDir, N);
	float rimIntensity = rimDot * RimNdotL;
	rimIntensity = smoothstep(_RimAmount - 0.01, _RimAmount + 0.01, rimIntensity);


	float _negativeRimAmount = 0.58;
	float rimMinusIntensity = rimDot * minusRimNdotL;
	rimMinusIntensity = smoothstep(_negativeRimAmount - 0.21, _negativeRimAmount + 0.21, rimMinusIntensity);

	float4 minusRim = rimMinusIntensity * vAmbientColor * 0.35 * rimLightStr;
	
	float4 rim = rimIntensity * vLightColor * rimLightStr;
    
    float3 baseRGB = (specular + diffuse + ambient + rim + minusRim).rgb * baseTex.rgb + emmisive.rgb ;

	float3 toGradientPos = gradientPos - input.WorldPos;
	float GradientDistance = length(toGradientPos);
	GradientDistance = max(GradientDistance, 0.0001f);
	float GradientAttenuation = 1.0f / (GradientDistance);
	
	GradientAttenuation *= 3.5f * gradientIntensity; 
	GradientAttenuation = saturate(GradientAttenuation);
	
	baseRGB = baseRGB * GradientAttenuation;

    return float4(baseRGB, baseTex.a);
}
)";
    const char* g_pscode_blinnphong_shadowmap = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

cbuffer MaterialBuffer : register(b1)
{
	uint textureFlags; 
    uint propertyFlags;
	float4 baseColor;
    float metallic;
    float roughness;
}

Texture2D txDiffuse : register(t0);
TextureCube skyBoxTX : register(t1);
Texture2D normalMap : register(t2);
Texture2D specularMap : register(t3);
Texture2D emmisiveMap : register(t4);
Texture2D lutMap : register(t9);
SamplerState samLinear : register(s0);
Texture2D shadowMap : register(t10); 
SamplerComparisonState samShadow : register(s1);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
	float4 LightPos : TEXCOORD4; 
	uint  IsFrontFace : SV_IsFrontFace; 
};

float4 PS(PS_INPUT input) : SV_TARGET
{
    float4 color = txDiffuse.Sample(samLinear, input.Tex);

    if (color.a < 0.5f)
    {
        discard; // 또는 clip(-1);
    }

    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
)";


    const char* g_pscode_BRDF_cook_torrance = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;               // 사용하지 않음
    float ambientStr;    // 현재 코드에서 Roughness로 사용 -> 임시적용 추후에 ConstantBuffer의 부분을 전부 분리해야 함
    float diffuseStr;    // 현재 코드에서 Metalic로 사용 -> 임시적용 추후에 ConstantBuffer의 부분을 전부 분리해야 함
    float specularStr;   // 현재 코드에서 lightIntensity로 사용 -> 임시적용 추후에 ConstantBuffer의 부분을 전부 분리해야 함
    uint shininess;      // 사용하지 않음
    float reflectionFactor;    // 사용하지 않음
    matrix LightViewProjection;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;  // 현재 코드에서 ambientIntensity로 사용 -> 임시적용 추후에 ConstantBuffer의 부분을 전부 분리해야 함
}

cbuffer MaterialBuffer : register(b1)
{
	uint textureFlags; 
    uint propertyFlags;
	float4 baseColor;
    float metallic;
    float roughness;
}
Texture2D txDiffuse : register(t0);
Texture2D normalMap : register(t2);
Texture2D specularMap : register(t3);
Texture2D emmisiveMap : register(t4);
Texture2D roughnessMap : register(t6); 
Texture2D metallicMap : register(t7); 
Texture2D lutMap : register(t9);
Texture2D shadowMap : register(t10); 

Texture2D brdfLUT : register(t20);
TextureCube irradianceMap : register(t21);
TextureCube prefilterMap : register(t22);

SamplerState samLinear : register(s0);
SamplerComparisonState samShadow : register(s1);

static const float EPS = 1e-6;
static const float PI = 3.14159265f;

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm : TEXCOORD1;
    float3 Tan : TEXCOORD2;
    float2 Tex : TEXCOORD3;
    float4 LightPos : TEXCOORD4; 
    uint  IsFrontFace : SV_IsFrontFace; 
};

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) 
                * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ndfGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = max(denom, EPS);
    denom = PI * denom * denom;
    return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    denom = max(denom, EPS);
	
    return num / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

float CalculateShadowPCF(float4 LightPos)
{
    float3 projCoords = LightPos.xyz / LightPos.w;
    float2 texCoords; 
    texCoords.x = projCoords.x * 0.5f + 0.5f;
    texCoords.y = -projCoords.y * 0.5f + 0.5f;
    
    if (texCoords.x < 0.0f || texCoords.x > 1.0f || 
        texCoords.y < 0.0f || texCoords.y > 1.0f)
        return 1.0f;
    
    float currentDepth = projCoords.z;
    if (currentDepth < 0.0f || currentDepth > 1.0f)
        return 1.0f;
    
    float bias = 0.005f;
    currentDepth -= bias;
    
    // 3x3 PCF
    float shadow = 0.0f;
    float2 texelSize = 1.0f / 4096.0f; // Shadow Map 크기
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(samShadow, texCoords + offset, currentDepth);
        }
    }
    shadow /= 9.0f;
    
    return shadow;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    // 필요한 변수를 모두 구하기
    
    float lightIntensity = specularStr;
    float3 lightColor = vLightColor.rgb * lightIntensity;

    float3 N = normalize(input.Norm);
    float3 T = normalize(input.Tan);
    T = normalize(T - dot(T, N) * N); // N에 직교하도록 조정
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    float3 normalTex = normalMap.Sample(samLinear, input.Tex).xyz * 2.0f - 1.0f; // 정규화
    normalTex = normalize(mul(normalTex, TBN)); // TBN 행렬을 곱해서 월드공간으로 변환
    N = lerp(N, normalTex, (textureFlags & 4) != 0);
    float3 V = normalize(CameraPos.xyz - input.WorldPos);
    float3 L = normalize(-vLightDir);
    float3 H = normalize(V + L);
    float3 R = reflect(-V, N);
    R.x = -R.x;

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    float4 albedo = lerp(baseColor, txDiffuse.Sample(samLinear, input.Tex), (textureFlags & 1) != 0);
    //albedo = pow(albedo, 2.2);      // 감마 보정
    float3 emmisive = lerp(float3(0,0,0), emmisiveMap.Sample(samLinear, input.Tex).rgb, (textureFlags & 8) != 0);
    //emmisive = pow(emmisive, 2.2);  // 감마 보정

    const float alphaCutoff = 0.5f;
    clip(albedo.a - alphaCutoff);


    float metallicTex = metallicMap.Sample(samLinear, input.Tex).r;
    float _metallic = lerp(diffuseStr, metallicTex,(textureFlags & 128) != 0);
    _metallic = lerp(_metallic, metallic,(propertyFlags & 128) != 0);

    float roughnessTex = roughnessMap.Sample(samLinear, input.Tex).r;
    float roughnessClamp = lerp(0.05f, 1.0f, ambientStr);
    float _roughness = lerp(roughnessClamp, roughnessTex,(textureFlags & 64) != 0);
    
    _roughness = lerp(_roughness, roughness,(propertyFlags & 64) != 0);
    _roughness = max(_roughness, 0.001f);

    float ambientIntensity = rimLightStr;

    // BRDF 계산
    // Cook-Torrance 모델

    // 직접광
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, _metallic);
    float NDF = ndfGGX(N, H, _roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float G = geometrySmith(N, V, L, _roughness);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 1e-4f;
    float3 specularBRDF = numerator / denominator;
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - _metallic;
    float3 diffuseBRDF = (kD * albedo.rgb / PI);
    float3 Lo = (diffuseBRDF + specularBRDF) * lightColor * NdotL;

    float shadow = CalculateShadowPCF(input.LightPos);
    Lo *= shadow;

    // 간접광 (IBL)
    float3 irradiance = irradianceMap.Sample(samLinear, N).rgb;
    //irradiance = pow(irradiance, 2.2);
    float3 F_ibl = fresnelSchlickRoughness(NdotV, F0, _roughness);
    float3 kD_ibl = (1.0 - F_ibl) * (1.0 - _metallic);
    float3 diffuseIBL = kD_ibl * irradiance * albedo.rgb;
    
    uint width, height, numMips;
    prefilterMap.GetDimensions(0,width, height, numMips);

    float maxMip = float(numMips - 1);
    float lod = _roughness * maxMip;

    float3 prefilteredColor = prefilterMap.SampleLevel(samLinear, R, lod).rgb;
    //prefilteredColor = pow(prefilteredColor, 2.2);
    float2 brdf  = brdfLUT.Sample(samLinear, float2(NdotV, _roughness)).rg;
    float3 specularIBL = prefilteredColor * (F_ibl * brdf.x + brdf.y);

    float3 ambient = (diffuseIBL + specularIBL) * ambientIntensity;
    
    float3 finalColor = Lo + emmisive + ambient;

    return float4(finalColor,1.0f);
}
)"; 

    const char* g_vscode_shadowcast_common = R"(
cbuffer ObjectMatBuffer : register(b5)
{
    matrix World;
    uint isSkinnedMesh;
}

cbuffer DirectionalLightBuffer : register(b7)
{
    float3 Position;
    float4 Direction;
    float3 Color;
    float Intensity;
    matrix LightViewProjection;
};

cbuffer BoneModelBuffer : register(b2)
{
    matrix ModelMatricies[128];
}

cbuffer BoneOffsetBuffer : register(b3)
{
    matrix OffsetMatricies[128];
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    float4 localPos = input.Pos;
    
    if (isSkinnedMesh != 0)
    {
        matrix skinningMatrix = {
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f
        };
        
        [unroll]
        for (int i = 0; i < 4; i++)
        {
            skinningMatrix += mul(
                OffsetMatricies[input.BoneIndices[i]], 
                ModelMatricies[input.BoneIndices[i]]
            ) * input.BoneWeights[i];
        }
        
        localPos = mul(localPos, skinningMatrix);
    }
    
    float4 worldPos = mul(localPos, World);
    output.Pos = mul(worldPos, LightViewProjection);
    
    return output;
}
)";

    const char* g_vscode_deffered_static = R"(
cbuffer ObjectMatBuffer : register(b5)
{
    matrix World;
    uint isSkinnedMesh;
}
cbuffer CameraBuffer : register(b6)
{
    matrix View;
    matrix Projection;
    float3 CameraPos;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
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
)";

    const char* g_vscode_deffered_skinning = R"(
cbuffer ObjectMatBuffer : register(b5)
{
    matrix World;
    uint isSkinnedMesh;
}
cbuffer CameraBuffer : register(b6)
{
    matrix View;
    matrix Projection;
    float3 CameraPos;
}

cbuffer BoneModelBuffer : register(b2)
{
	matrix ModelMatricies[128];
}

cbuffer BoneOffsetBuffer : register(b3)
{
	matrix OffsetMatricies[128];
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tan : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 BoneIndices : BONEINDICES;
    float4 BoneWeights : BONEWEIGHTS;
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
	
    matrix skinningMatrix =  {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };

    for (int i = 0; i < 4; i++)
    {
        skinningMatrix += mul(OffsetMatricies[input.BoneIndices[i]],ModelMatricies[input.BoneIndices[i]]) * input.BoneWeights[i] ;
    }

	matrix finalWorld = mul(skinningMatrix,World);

    output.Pos = mul(input.Pos, finalWorld);
    output.WorldPos = output.Pos.xyz;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    output.Norm = normalize(mul(input.Norm, (float3x3) finalWorld));
    output.Tan = normalize(mul(input.Tan, (float3x3) finalWorld));
    output.Tex = input.Tex;
    
    return output;
}
)";

    const char* g_pscode_deffered_Geometry = R"(
Texture2D AlbedoMap    : register(t0);
Texture2D NormalMap    : register(t2);
Texture2D RoughnessMap : register(t6);
Texture2D MetallicMap  : register(t7);
SamplerState samLinear : register(s0);

cbuffer MaterialBuffer : register(b1)
{
    uint textureFlags; 
    uint propertyFlags;
    float4 baseColor;
    float metallic;
    float roughness;
}

cbuffer PBRDebugBuffer : register(b9)
{
    float useMaterialOverride;
    float metallicOverride;
    float roughnessOverride;
    float ambientIntensity;
}

struct GBufferOut
{
    float4 Position  : SV_Target0; // GBuffer #1
    float4 Normal    : SV_Target1; // GBuffer #2
    float4 Albedo    : SV_Target2; // GBuffer #3
    float4 Metallic  : SV_Target3; // GBuffer #4 (R만 사용)
    float4 Roughness : SV_Target4; // GBuffer #5 (R만 사용)
    float4 DebugNormal : SV_Target5;
};

struct PS_INPUT
{
    float4 Pos      : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Norm     : TEXCOORD1;
    float3 Tan      : TEXCOORD2;
    float2 Tex      : TEXCOORD3;
};

GBufferOut PS(PS_INPUT input)
{
    GBufferOut output;
    
    output.Position = float4(input.WorldPos, 1.0f);

    float3 N = normalize(input.Norm);
    float3 T = normalize(input.Tan);
    T = normalize(T - dot(T, N) * N); // N에 직교하도록 조정
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    float3 normalTex = NormalMap.Sample(samLinear, input.Tex).xyz * 2.0f - 1.0f; // 정규화
    normalTex = normalize(mul(normalTex, TBN)); // TBN 행렬을 곱해서 월드공간으로 변환

    output.Normal = float4(lerp(N, normalTex, (textureFlags & 4) != 0),0.0f);

    output.DebugNormal = float4(N * 0.5 + 0.5, 1.0f);
    
    float4 albedo = lerp(baseColor, AlbedoMap.Sample(samLinear, input.Tex), (textureFlags & 1) != 0);
    albedo = lerp(baseColor, albedo, (propertyFlags & 1) != 0);
    if(useMaterialOverride)
    {
        albedo = float4(1.0,1.0,1.0,1.0);
    }

    //알파 컷
    const float alphaCutoff = 0.5f;
    clip(albedo.a - alphaCutoff);

    output.Albedo = float4(albedo.rgb, 1.0f);


    float _metallic = lerp(metallic, MetallicMap.Sample(samLinear, input.Tex).r, (textureFlags & 128) != 0);
    _metallic = lerp(_metallic, metallic, (propertyFlags & 128) != 0);
    if(useMaterialOverride)
        _metallic = metallicOverride;
    output.Metallic = float4(_metallic, 0, 0, 0);
    

    float _roughness = lerp(roughness, RoughnessMap.Sample(samLinear, input.Tex).r, (textureFlags & 64) != 0);
    //_roughness = lerp(_roughness,roughness, (propertyFlags & 64) != 0);

    if(useMaterialOverride)
    {
        float roughnessClamp = lerp(0.025f, 1.0f, roughnessOverride);
        _roughness = roughnessClamp;
    }    
    _roughness = max(_roughness, 0.025f);
    output.Roughness = float4(_roughness, 0, 0, 0);

    return output;
}
)";

    const char* g_pscode_deffered_Light = R"(
Texture2D PositionMap  : register(t0);
Texture2D NormalMap    : register(t1);
Texture2D AlbedoMap    : register(t2);
Texture2D MetallicMap  : register(t3);
Texture2D RoughnessMap : register(t4);

Texture2D shadowMap : register(t10); 

Texture2D brdfLUT : register(t20);
TextureCube irradianceMap : register(t21);
TextureCube prefilterMap  : register(t22);

SamplerState samLinear : register(s0);
SamplerState samPoint : register(s2);

SamplerComparisonState samShadow : register(s1);

cbuffer CameraBuffer : register(b6)
{
    matrix View;
    matrix Projection;
    float3 CameraPos;
};

cbuffer DirectionalLightBuffer : register(b7)
{
    float3 DirectionalLightPos;
    float4 DirectionalLightDir;
    float3 DirectionalLightColor;
    float DirectionalLightIntensity;
    matrix LightViewProjection;
};

cbuffer PBRDebugBuffer : register(b9)
{
    float useMaterialOverride;
    float metallicOverride;
    float roughnessOverride;
    float ambientIntensity;
};

static const float PI = 3.14159265359f;
static const float EPS = 1e-6f;

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) 
                * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ndfGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = max(denom, EPS);
    denom = PI * denom * denom;
    return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    denom = max(denom, EPS);
	
    return num / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

float CalculateShadowPCF(float4 LightPos)
{
    float3 projCoords = LightPos.xyz / LightPos.w;
    float2 texCoords; 
    texCoords.x = projCoords.x * 0.5f + 0.5f;
    texCoords.y = -projCoords.y * 0.5f + 0.5f;
    
    if (texCoords.x < 0.0f || texCoords.x > 1.0f || 
        texCoords.y < 0.0f || texCoords.y > 1.0f)
        return 1.0f;
    
    float currentDepth = projCoords.z;
    if (currentDepth < 0.0f || currentDepth > 1.0f)
        return 1.0f;
    
    float bias = 0.005f;
    currentDepth -= bias;
    
    // 3x3 PCF
    float shadow = 0.0f;
    float2 texelSize = 1.0f / 4096.0f; // Shadow Map 크기
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(samShadow, texCoords + offset, currentDepth);
        }
    }
    shadow /= 9.0f;
    
    return shadow;
}

struct PSIn
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

float4 PS(PSIn input) : SV_Target
{
    // 필요한 변수를 모두 구하기
    float3 lightColor = DirectionalLightColor * DirectionalLightIntensity;

    float3 WorldPos = PositionMap.Sample(samPoint, input.uv);    
    float4 toDirLightViewPos = mul(float4(WorldPos, 1.0f), LightViewProjection);

    float3 N = normalize(NormalMap.Sample(samPoint, input.uv).xyz); 

    float3 V = normalize(CameraPos.xyz - WorldPos.xyz);
    float3 L = normalize(-DirectionalLightDir.xyz);
    float3 H = normalize(V + L);
    float3 R = reflect(-V, N);
    R.x = -R.x;

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    float4 albedo = AlbedoMap.Sample(samPoint, input.uv);
    float _metallic = MetallicMap.Sample(samPoint, input.uv).r;
    float _roughness = RoughnessMap.Sample(samPoint, input.uv).r;

    // BRDF 계산
    // Cook-Torrance 모델

    // 직접광
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, _metallic);
    float NDF = ndfGGX(N, H, _roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float G = geometrySmith(N, V, L, _roughness);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 1e-4f;
    float3 specularBRDF = numerator / denominator;
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - _metallic;
    float3 diffuseBRDF = (kD * albedo.rgb / PI);
    float3 Lo = (diffuseBRDF + specularBRDF) * lightColor * NdotL;

    float shadow = CalculateShadowPCF(toDirLightViewPos);
    Lo *= shadow;

    // 간접광 (IBL)
    float3 irradiance = irradianceMap.Sample(samLinear, N).rgb;
    float3 F_ibl = fresnelSchlickRoughness(NdotV, F0, _roughness);
    float3 kD_ibl = (1.0 - F_ibl) * (1.0 - _metallic);
    float3 diffuseIBL = kD_ibl * irradiance * albedo.rgb;
    
    uint width, height, numMips;
    prefilterMap.GetDimensions(0,width, height, numMips);

    float maxMip = float(numMips - 1);
    float lod = _roughness * maxMip;

    float3 prefilteredColor = prefilterMap.SampleLevel(samLinear, R, lod).rgb;
    float2 brdf  = brdfLUT.Sample(samLinear, float2(NdotV, _roughness)).rg;
    float3 specularIBL = prefilteredColor * (F_ibl * brdf.x + brdf.y);

    float3 ambient = (diffuseIBL + specularIBL) * ambientIntensity;
    
    float3 finalColor = Lo + ambient;

    return float4(finalColor,1.0f);
}
)";

    const char* g_pscode_deffered_AdditivePointLight = R"(
Texture2D PositionMap  : register(t0);
Texture2D NormalMap    : register(t1);
Texture2D AlbedoMap    : register(t2);
Texture2D MetallicMap  : register(t3);
Texture2D RoughnessMap : register(t4);

SamplerState samLinear : register(s0);
SamplerState samPoint : register(s2);

cbuffer CameraBuffer : register(b6)
{
    matrix View;
    matrix Projection;
    float3 CameraPos;
};

cbuffer PointLightBuffer : register(b8)
{
	float3 LightColor;
	float LightIntensity;
	float3 LightPos;
	float LightRange;
};


static const float PI = 3.14159265359f;
static const float EPS = 1e-6f;

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) 
                * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ndfGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = max(denom, EPS);
    denom = PI * denom * denom;
    return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    denom = max(denom, EPS);
	
    return num / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

struct PSIn
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

float4 PS(PSIn input) : SV_Target
{
    float3 worldPos = PositionMap.Sample(samPoint, input.uv).xyz;
    float3 N = normalize(NormalMap.Sample(samPoint, input.uv).xyz);
    float3 V = normalize(CameraPos - worldPos);

    float3 L = LightPos - worldPos;
    float dist = length(L);

    if (dist > LightRange)
            discard;

    L /= dist;

    float atten = saturate(1.0 - dist / LightRange);
    float NdotL = saturate(dot(N, L));

    float3 albedo = AlbedoMap.Sample(samPoint, input.uv).rgb;
    float metallic = MetallicMap.Sample(samPoint, input.uv).r;
    float roughness = RoughnessMap.Sample(samPoint, input.uv).r;

    float3 F0 = lerp(float3(0.04,0.04,0.04), albedo, metallic);

    float3 H = normalize(V + L);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = ndfGGX(N, H, roughness);
    float G   = geometrySmith(N, V, L, roughness);

    float3 spec =
        (NDF * G * F) /
        max(4.0 * max(dot(N,V),0) * NdotL, 1e-4);

    float3 kD = (1.0 - F) * (1.0 - metallic);
    float3 diffuse = kD * albedo / PI;

    float3 color =
        (diffuse + spec) *
        LightColor *
        LightIntensity *
        NdotL *
        atten;

    return float4(color, 1);
}
)";

    const char* g_postprocess_vscode_quad = R"(
struct VS_Output
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

VS_Output VS(uint id : SV_VertexID)
{
    VS_Output Output;
    Output.Tex = float2((id << 1) & 2, id & 2);
    Output.Pos = float4(Output.Tex * float2(2, -2) + float2(-1,1), 0, 1);
    return Output;
}
)";

    const char* g_postprocess_pscode_ACES_toneMapping = R"(
cbuffer PostProcessBuffer : register(b0)
{
    float exposure;
    float supportHDR;
}
Texture2D txInput : register(t0);
SamplerState samLinear : register(s0);

float3 LinearToSRGB(float3 linearColor)
{
    return pow(linearColor, 1.0f / 2.2f);
}

static const float MaxNits = 1000.0;   // HDR10 기준

float3 LinearToPQ(float3 nits)
{
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;

    float3 L = saturate(nits / MaxNits);
    float3 Lm1 = pow(L, m1);
    return pow((c1 + c2 * Lm1) / (1 + c3 * Lm1), m2);
}

float3 ACESFilmic(float3 Color)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return (Color * (a * Color + b)) / (Color * (c * Color + d) + e);
}

struct PSIn
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

float4 PS(PSIn input) : SV_Target
{
    //uint width, height;

    //txInput.GetDimensions(width, height);

    //float2 pixel = input.uv * float2(width, height);    
    //float pixelScale = 4.0f;

    //pixel = floor(pixel / pixelScale) * pixelScale;

    float4 color = float4(0,0,0,0);

    color = txInput.Sample(samLinear, input.uv);

    // Linear HDR 밝기 조절 (nits 스케일)
    color *= exposure;

    // PQ 인코딩
    float3 pq = LinearToPQ(color);
   
    // ACES Filmic
    float3 aces = ACESFilmic(color.rgb);
    aces = saturate(aces);
    aces = LinearToSRGB(aces);

    float3 finalColor = lerp(aces,pq,supportHDR);

    return float4(finalColor, 1.0f);
}
)";

}
