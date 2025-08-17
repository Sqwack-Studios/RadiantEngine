#ifndef _BASIC_PS_HLSL_
#define _BASIC_PS_HLSL_

struct PSin
{
    float4 pos : SV_Position;
    float3 color : TEXCOORD0;
};

	float4 PSMain(PSin psIn) : SV_Target0
{
    return float4(psIn.color, 1.0f);
}

	  
#endif //_BASIC_PS_HLSL_
