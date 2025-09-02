struct VS_OUTPUT
{
    float4 Pos : SV_Position;
    float4 Color : COLOR;
};

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}