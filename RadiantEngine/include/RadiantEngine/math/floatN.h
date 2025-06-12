//  Filename: floatN 
//	Author:	Daniel														
//	Date: 11/06/2025 21:59:51		
//  Sqwack-Studios													

#ifndef RE_FLOATN_H
#define RE_FLOATN_H

#include "RadiantEngine/core/platform.h"
#include "RadiantEngine/core/types.h"
#include <cmath>

namespace RE::math
{

	



	struct float2
	{
		fp32 x, y;

		RE_INLINE float2& operator+=(const float2 other) 
		{
			x += other.x;
			y += other.y;

			return *this;
		}

		RE_INLINE float2& operator-= (const float2 other)
		{
			x -= other.x;
			y -= other.y;

			return *this;
		}

		RE_INLINE float2& operator*= (const float2 other) 
		{
			x *= other.x;
			y *= other.y;

			return *this;
		}

		RE_INLINE float2 operator/=(const float2 other)
		{
			x /= other.x;
			y /= other.y;

			return *this;
		}
	};

	RE_INLINE float2 operator-(const float2 a) { return float2{ -a.x, -a.y }; }
	RE_INLINE float2 operator+(const float2 a, const float2 b) { return float2{ a.x + b.x, a.y + b.y }; }
	RE_INLINE float2 operator-(const float2 a, const float2 b) { return float2{ a.x - b.x, a.y - b.y }; }
	RE_INLINE float2 operator*(const float2 a, const float2 b) { return float2{ a.x * b.x, a.y * b.y }; }
	RE_INLINE float2 operator*(const float2 v, const fp32 s) { return float2{ s * v.x, s * v.y }; }
	RE_INLINE float2 operator*(const fp32 s, const float2 v) { return v * s; }
	RE_INLINE float2 operator/(const float2 a, const float2 b) { return float2{ a.x / b.x, a.y / b.y }; }
	RE_INLINE float2 operator/(const fp32 s, const float2 v) { return float2{ s / v.x, s / v.y}; }
	RE_INLINE float2 operator/(const float2 v, const fp32 s) { fp32 is{ 1.f / s }; return is * v; }                                      

	

	struct float3
	{
		fp32 x, y, z;
	};

	RE_INLINE float3 operator-(const float3 a) { return float3{ -a.x, -a.y, -a.z }; }
	RE_INLINE float3 operator+(const float3 a, const float3 b) { return float3{a.x + b.x, a.y + b.y, a.z + b.z}; }
	RE_INLINE float3 operator-(const float3 a, const float3 b) { return float3{a.x - b.x, a.y - b.y, a.z - b.z}; }
	

	struct float4
	{
		fp32 x, y, z, w;
	};



	

}

#endif // !RE_FLOATN_H
