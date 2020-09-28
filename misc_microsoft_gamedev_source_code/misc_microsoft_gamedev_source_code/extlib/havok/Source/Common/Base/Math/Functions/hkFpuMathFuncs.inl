/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#include <Common/Base/Fwd/hkcmath.h>
#include <Common/Base/Fwd/hkcfloat.h>

extern "C"
{
	hkReal HK_CALL hkMath_atan2fApproximation( hkReal x, hkReal y );
}

#define HK_INT32_MIN		(-2147483647 - 1)	// Minimum (signed) int 32-bit value
#define HK_INT32_MAX		2147483647			// Maximum (signed) int 32-bit value
#define HK_REAL_PI			3.14159265358979f
#define HK_REAL_DEG_TO_RAD	3.14159265358979f / 180.0f
#define HK_REAL_EPSILON		FLT_EPSILON			// smallest such that 1.0+FLT_EPSILON != 1.0
#define HK_REAL_MIN			FLT_MIN				// min positive value
#define HK_REAL_MAX			3.40282e+38f		// max value - not actually FLT_MAX since on some systems
												// FLT_MAX is indistinguishable from NaN or Inf which we reserve
												// for error checking.

#if defined(HK_PLATFORM_PS3) || defined(HK_PLATFORM_PS3SPU) || \
	defined(HK_PLATFORM_RVL) || defined(HK_PLATFORM_GC)
#define HK_STD_NAMESPACE std
#else
#define HK_STD_NAMESPACE /*nothing*/
#endif

namespace hkMath
{

	//
	// Some implementations may be overridden on a given platform.
	//

#ifndef HK_MATH_sqrt
	inline hkReal HK_CALL sqrt(hkReal r) { return HK_STD_NAMESPACE::sqrtf(r); }
#endif

#ifndef HK_MATH_sqrtInverse
	inline hkReal HK_CALL sqrtInverse(hkReal r) { return 1.0f / hkMath::sqrt(r); }
#endif

#ifndef HK_MATH_fabs
	inline hkReal HK_CALL fabs(hkReal r) { return HK_STD_NAMESPACE::fabsf(r); }
#endif

#ifndef  HK_MATH_pow 
	inline hkReal HK_CALL pow( hkReal r, hkReal s ) { return HK_STD_NAMESPACE::powf( r, s ); }
#endif

#ifndef HK_MATH_ceil
	inline hkReal HK_CALL ceil( hkReal r ) { return HK_STD_NAMESPACE::ceilf( r ); }
#endif

#ifndef HK_MATH_sin
	inline hkReal HK_CALL sin (hkReal r) { return HK_STD_NAMESPACE::sinf(r); }
#endif

#ifndef HK_MATH_cos
	inline hkReal HK_CALL cos (hkReal r) { return HK_STD_NAMESPACE::cosf(r); }
#endif

#ifndef HK_MATH_acos
	inline hkReal HK_CALL acos(hkReal r)
	{
		// be generous about numbers slightly outside range
		HK_ASSERT(0x41278654,  hkMath::fabs(r) < 1.001f );
		if( hkMath::fabs(r) >= 1.0f )
		{
			r = ( r>0 )	? 0 : HK_REAL_PI;
			return r;
		}
		return HK_STD_NAMESPACE::acosf(r);
	}
#endif

#ifndef HK_MATH_asin
	inline hkReal HK_CALL asin(hkReal r)
	{
		// be generous about numbers outside range
		HK_ASSERT(0x286a6f5f,  hkMath::fabs(r) < 1.001f );
		if( hkMath::fabs(r) >= 1.0f )
		{
			r = ( r>0 )	? 0.5f * HK_REAL_PI : -0.5f * HK_REAL_PI;
			return r;
		}
		return HK_STD_NAMESPACE::asinf(r);
	}
#endif

#ifndef HK_MATH_floor
	inline hkReal HK_CALL floor(hkReal r) { return HK_STD_NAMESPACE::floorf(r); }
#endif

#ifndef HK_MATH_prefetch128
		// prefetch at least 128 bytes
	inline void prefetch128( const void* ) { }
#endif

#ifndef HK_MATH_forcePrefetch
	template<int SIZE>
	inline void forcePrefetch( const void* p )
	{
		// volatile register int a = (int*)p;
	}
#endif

#ifndef HK_MATH_hkFloor
	hkReal HK_CALL hkFloor(hkReal r);
#endif

#ifndef HK_MATH_hkFloatToInt
	int HK_CALL hkFloatToInt(hkReal r);
#endif

#ifndef HK_MATH_hkFloorToInt
	int HK_CALL hkFloorToInt(hkReal r);
#endif

#ifndef HK_MATH_hkToIntFast
		// Fast rounding, however last bit might be wrong.
	inline int HK_CALL hkToIntFast( hkReal r )
	{
#	if defined(HK_ARCH_IA32) && !defined(HK_COMPILER_GCC)
		int i; // use even when simd disabled on ia32
		_asm {
			fld r
			fistp i
		}
		return i;
#	else
		return int(r);
#	endif
	}
#endif

	inline int HK_CALL isNegative(hkReal r0)
	{
		return (r0<0)? hkVector4Comparison::MASK_X : 0;
	}

	inline hkBool HK_CALL equal(hkReal x, hkReal y, hkReal tolerance2=1e-5f)
	{
		return hkMath::fabs(x-y) < tolerance2;
	}

#ifndef HK_MATH_max2
	template <typename T>
	inline T HK_CALL max2( T x, T y)
	{
		return x > y ? x : y;
	}
#endif

#ifndef HK_MATH_min2
	template <typename T>
	inline T HK_CALL min2( T x, T y)
	{
		return x < y ? x : y;
	}
#endif

	template <typename T>
	inline T HK_CALL clamp( T x, T mi, T ma)
	{
		if ( x < mi ) return mi;
		if ( x > ma ) return ma;
		return x;
	}

	inline hkBool HK_CALL isFinite(hkReal r)
	{
		// Check the 8 exponent bits.
		// Usually NAN == (exponent = all 1, mantissa = non-zero)
		//         INF == (exponent = all 1, mantissa = zero)
		// This simply checks the exponent
		HK_ASSERT(0x2d910c70, sizeof(hkReal) == sizeof(int));
		union {
			hkReal f;
			unsigned int i;
		} val;

		val.f = r;
		return ((val.i & 0x7f800000) != 0x7f800000);
	}
	
	inline bool isPower2(unsigned int v)
	{
		return (v & (v - 1)) == 0;
	}

	hkReal HK_CALL rand01();
	inline hkReal HK_CALL atan2fApproximation( hkReal sina, hkReal cosa )
	{
		return hkMath_atan2fApproximation(sina, cosa);
	}

	inline hkReal HK_CALL randRange(hkReal minv, hkReal maxv)
	{
		return minv + rand01() * (maxv-minv);
	}

#ifndef HK_MATH_fselectGreaterEqualZero
	inline hkReal fselectGreaterEqualZero( hkReal testVar, hkReal ifTrue, hkReal ifFalse)
	{
		return (testVar >= 0.0f) ? ifTrue : ifFalse;
	}
#endif

#ifndef HK_MATH_fselectGreaterZero
	inline hkReal fselectGreaterZero( hkReal testVar, hkReal ifTrue, hkReal ifFalse)
	{
		return (testVar > 0.0f) ? ifTrue : ifFalse;
	}
#endif

#ifndef HK_MATH_fselectEqualZero
	inline hkReal fselectEqualZero( hkReal testVar, hkReal ifTrue, hkReal ifFalse)
	{
		return (testVar == 0.0f) ? ifTrue : ifFalse;
	}
#endif

#ifndef HK_MATH_intInRange
		/// Returns any nonzero value if lowInclusive<=value and value<highExclusive.
	inline int intInRange( int value, int lowInclusive, int highExclusive )
	{
		return (lowInclusive <= value) & (value < highExclusive);
	}
#endif
}

/*
* Havok SDK - PUBLIC RELEASE, BUILD(#20070919)
*
* Confidential Information of Havok.  (C) Copyright 1999-2007 
* Telekinesys Research Limited t/a Havok. All Rights Reserved. The Havok
* Logo, and the Havok buzzsaw logo are trademarks of Havok.  Title, ownership
* rights, and intellectual property rights in the Havok software remain in
* Havok and/or its suppliers.
*
* Use of this software for evaluation purposes is subject to and indicates 
* acceptance of the End User licence Agreement for this product. A copy of 
* the license is included with this software and is also available from salesteam@havok.com.
*
*/
