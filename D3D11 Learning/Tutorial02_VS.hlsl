struct VS_OUTPUT
{
    float4 Pos : SV_Position;
    float4 Color : COLOR;
};

VS_OUTPUT VS(float3 inPos : POSITION, float4 inColor : COLOR)
{
    VS_OUTPUT output;
    
    output.Pos = float4(inPos,1.0f);
    output.Color = inColor;
    
    return output;
}

