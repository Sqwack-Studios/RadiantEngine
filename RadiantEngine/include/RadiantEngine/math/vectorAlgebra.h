//  Filename: vectorAlgebra 
//	Author:	Daniel														
//	Date: 12/06/2025 21:07:14		
//  Sqwack-Studios													
#ifndef RE_VECTOR_ALGEBRA_H
#define RE_VECTOR_ALGEBRA_H

#include "floatN.h"
#include <cmath>

//Scalar implementation of common euclidean operations
namespace RE
{
	
	/* API */

	fp32 dot(const float2, const float2);
	fp32 dot(const float3, const float3);
	fp32 dot(const float4, const float4);

	fp32 lengthSq(const float2);
	fp32 lengthSq(const float3);
	fp32 lengthSq(const float4);

	fp32 length(const float2);
	fp32 length(const float3);
	fp32 length(const float4);

	float2 normalize(const float2);
	float3 normalize(const float3);
	float4 normalize(const float4);
		
	//retrieves the Z component
	float cross(const float2, const float2);
	float3 cross(const float3, const float3);



	/* IMPLEMENTATIONS */


	RE_INLINE fp32 dot(const float2 a, const float2 b) { return a.x * b.x + a.y * b.y; }
	RE_INLINE fp32 dot(const float3 a, const float3 b) { return (a.x * b.x + a.y * b.y) + (a.z * b.z); }
	RE_INLINE fp32 dot(const float4 a, const float4 b) { return (a.x * b.x + a.y * b.y) + (a.z * b.z + a.w * b.w); }



	RE_INLINE fp32 lengthSq(const float2 a) { return a.x * a.x + a.y * a.y; }
	RE_INLINE fp32 lengthSq(const float3 a) { return (a.x * a.x + a.y * a.y) + a.z * a.z; }
	RE_INLINE fp32 lenghtSq(const float4 a) { return (a.x * a.x + a.y * a.y) + (a.z * a.z + a.w * a.w); }

	RE_INLINE fp32 length(const float2 a) { return std::sqrtf(lengthSq(a)); }
	RE_INLINE fp32 length(const float3 a) { return std::sqrtf(lengthSq(a)); }
	RE_INLINE fp32 length(const float4 a) { return std::sqrtf(lengthSq(a)); }


	RE_INLINE float cross(const float2 a, const float2 b) { return a.x * b.y - a.y * b.x; }
	RE_INLINE float3 cross(const float3 a, const float3 b) 
	{
		fp32 x{ a.y * b.z - a.z * b.y };
		fp32 y{ a.z * b.x - a.x * b.z };
		fp32 z{ a.x * b.y - a.y * b.x };

		return { x, y, z };
	}

	RE_INLINE float2 normalize(const float2 a) { return a / lengthSq(a); }
	RE_INLINE float3 normalize(const float3 a) { return a / lengthSq(a); }
	RE_INLINE float4 normalize(const float4 a) { return a / lengthSq(a); }


}


#endif // !RE_VECTOR_ALGEBRA_H