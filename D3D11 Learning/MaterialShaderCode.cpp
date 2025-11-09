#include "MaterialShaderCode.h"

namespace MyEngine
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
    float4 vOutputColor;
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
    const char* g_vscode_outline = R"(
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
    float4 vOutputColor;
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
    const char* g_vscode_outline_useSkinning = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vOutputColor;
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

	float3 N = normalize(input.Norm);
	float3 expanded = output.WorldPos + N * OutlineThickness;
	float4 expandedPos = float4(expanded, 1.0);


    output.Pos = mul(expandedPos, View);
    output.Pos = mul(output.Pos, Projection);

	// finalWorld 행렬이 적용된 위치를 LightViewProjection으로 변환
	output.LightPos = mul(float4(output.WorldPos, 1.0f),LightViewProjection);
    
    output.Norm = normalize(mul(input.Norm, (float3x3) finalWorld));
    output.Tan = normalize(mul(input.Tan, (float3x3) finalWorld));
    output.Tex = input.Tex;
    
    return output;
}
)";

    const char* g_vscode_blinnphong = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vOutputColor;
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
    const char* g_vscode_blinnphong_rigid = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vOutputColor;
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

    const char* g_vscode_blinnphong_skinning = R"(
cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float3 CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vOutputColor;
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

namespace MyEngine
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
    float4 vOutputColor;
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
	float4 baseColor;
}

Texture2D txDiffuse : register(t0);
TextureCube skyBoxTX : register(t1);
Texture2D normalMap : register(t2);
Texture2D specularMap : register(t3);
Texture2D emmisiveMap : register(t4);
Texture2D lutMap : register(t5);
SamplerState samLinear : register(s0);
Texture2D shadowMap : register(t6); 
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
    float4 vOutputColor;
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
	float4 baseColor;
}

cbuffer GradientBuffer : register(b5)
{
	float4 gradientTop;
	float4 gradientBottom;
	float3 gradientPos;
	float gradientIntensity;
}

Texture2D txDiffuse : register(t0);
TextureCube skyBoxTX : register(t1);
Texture2D normalMap : register(t2);
Texture2D specularMap : register(t3);
Texture2D emmisiveMap : register(t4);
Texture2D lutMap : register(t5);
SamplerState samLinear : register(s0);
SamplerState samPoint : register(s2);
Texture2D shadowMap : register(t6); 
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
    float3 envRGB = (specular + diffuse + ambient).rgb * skyBoxTX.Sample(samLinear, R).rgb;
    
    float3 finalRGB = lerp(baseRGB, envRGB, reflectionFactor);

	float3 toGradientPos = gradientPos - input.WorldPos;
	//float3 GradientL = normalize(toGradientPos);
	float GradientDistance = length(toGradientPos);
	GradientDistance = max(GradientDistance, 0.0001f);
	float GradientAttenuation = 1.0f / (GradientDistance);
	
	GradientAttenuation *= 3.5f * gradientIntensity; 
	GradientAttenuation = saturate(GradientAttenuation);
	
	finalRGB = finalRGB * GradientAttenuation;
	
	// y 범위 값들
    //float yMin = minY; // 0
    //float yMax = maxY; // 10

    // 안전하게 분모가 0이 되는 경우 방지
    //float span = max(0.00001, yMax - yMin);

    // 0..1 로 정규화 (y=0 -> 0, y=10 -> 1)
	 //   float gradient = saturate( (input.WorldPos.y - yMin) / span );
	//gradient = smoothstep(0.0, 1.0, gradient);

	//float4 gradientColor = lerp(gradientBottom, gradientTop, gradient);
	//
	//finalRGB = finalRGB * gradientColor.rgb * gradientIntensity;

	//finalRGB = lerp(vAmbientColor , finalRGB, lerp(1.0, gradient, gradientIntensity)); // 흰색으로 보간


    return float4(finalRGB, baseTex.a);
}
)";

}
