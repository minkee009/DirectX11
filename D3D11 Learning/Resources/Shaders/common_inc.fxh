cbuffer ConstantBuffer : register(b0)
{
    matrix oldVer_World;
    matrix oldVer_View;
    matrix oldVer_Projection;
    float3 oldVer_CameraPos;
    float3 vLightPos;
    float4 vLightDir;
    float4 vLightColor;
    float4 vAmbientColor;
    float ambientStr;
    float diffuseStr;
    float specularStr;
    uint shininess;
    float reflectionFactor;
    matrix lightViewProj;
    float lowLut;
    float diffGradientDistHalf;
    float diffGradientDepth;
    float rimLightStr;
}

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

cbuffer ObjectMatBuffer : register(b5)
{
    matrix World;
    uint isSkinnedMesh;
}
