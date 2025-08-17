#ifndef _BASIC_VS_HLSL_
#define _BASIC_VS_HLSL_

struct VSin
{
    float3 pos : POSITION;
    float3 color : COLOR;
};

struct VSOut
{
    float4 pos : SV_Position;
    float3 color : TEXCOORD0;
};


VSOut VSMain(VSin vin)
{
    VSOut output;
    output.pos = float4(vin.pos, 1.f);
    output.color = vin.color;
    
    return output;
}


#endif //_BASIC_VS_HLSL_
