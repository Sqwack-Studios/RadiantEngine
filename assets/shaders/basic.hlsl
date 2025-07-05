#ifndef _BASIC_HLSL_
#define _BASIC_HLSL_

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


float3 PSMain(VSOut vout) : SV_Target0
{
    return vout.color;
}
#endif //_BASIC_HLSL_