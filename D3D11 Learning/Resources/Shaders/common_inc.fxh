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
    matrix lightViewProj;
}
