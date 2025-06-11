//  Filename: floatN 
//	Author:	Daniel														
//	Date: 11/06/2025 21:59:51		
//  Sqwack-Studios													

#include "RadiantEngine/core/platform.h"
#include "RadiantEngine/core/types.h"

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
	RE_INLINE float2 operator+(const float2 lhs, const float2 rhs) { return float2{ lhs.x + rhs.x, lhs.y + rhs.y }; }
	RE_INLINE float2 operator-(const float2 lhs, const float2 rhs) { return float2{ lhs.x - rhs.x, lhs.y - rhs.y }; }
	RE_INLINE float2 operator*(const float2 lhs, const float2 rhs) { return float2{ lhs.x * rhs.x, lhs.y * rhs.y }; }
	RE_INLINE float2 operator/(const float2 lhs, const float2 rhs) { return float2{ lhs.x / rhs.x, lhs.y / rhs.y }; }


	struct float3
	{
		fp32 x, y, z;
	};

	struct float4
	{
		fp32 x, y, z, w;
	};


	

}

