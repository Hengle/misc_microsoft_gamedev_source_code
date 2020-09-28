/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkMath.h
//
// Library of static functions for math computations.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKMATH_H_
#define _AKMATH_H_

#include <math.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#ifdef XBOX360
#include <ppcintrinsics.h>
#endif

#ifdef __PPU__
#include <ppu_intrinsics.h>
#endif

#define PI					(3.1415926535897932384626433832795f)
#define TWOPI				(6.283185307179586476925286766559f)
#define PIOVERTWO			(1.5707963267948966192313216916398f)
#define PIOVERFOUR			(0.78539816339744830961566084581988f)
#define PIOVEREIGHT			(0.3926990816987241548078304229095f)
#define THREEPIOVERFOUR		(2.3561944901923449288469825374596f)
#define ONEOVERPI			(0.31830988618379067153776752674503f)
#define ONEOVERTWOPI		(0.15915494309189533576888376337251f)
#define ROOTTWO				(1.4142135623730950488016887242097f)
#define LOGTWO				(0.30102999566398119521373889472449f)
#define ONEOVERLOGTWO		(3.3219280948873623478703194294894)
#define ONEOVERTHREESIXTY	(0.0027777777777777777777777777777778f)
#define PIOVERONEEIGHTY		(0.017453292519943295769236907684886f)
#define ONEEIGHTYOVERPI		(57.295779513082320876798154814105f)

#define	EPSILON			(0.0009765625f)			// 2^-10

struct AkVector;
struct ConeParams;
struct RadiusParams;
struct AkSoundPosition;
struct AkSoundParams;

namespace AkMath
{
	// converts degrees to radians
	extern AkReal32 ToRadians(
		AkReal32	in_fDegrees
		);

	// converts radians to degrees
	extern AkReal32 ToDegrees(
		AkReal32	in_fRadians
		);

	// makes ||inout_Vector|| belong to [0,1]
	extern void Normalise(
		AkVector&	inout_Vector
		);

	// returns the "length"
	extern AkReal32 Magnitude(
		AkVector&	inout_Vector
		);

	// returns Vector1.Vector2
	extern AkReal32 DotProduct(
		const AkVector&	Vector1,
		const AkVector&	Vector2
		);

	// returns Vector1 x Vector2
	extern AkVector CrossProduct(
		AkVector&	Vector1,
		AkVector&	Vector2
		);

	// returns |Position1 - Position2|
	extern AkReal32 Distance(
		const AkVector&	Position1,
		const AkVector&	Position2
		);

	// 
	extern AkReal32 Interpolate(
		AkReal32	in_fLowerX,
		AkReal32	in_fLowerY,
		AkReal32	in_fUpperX,
		AkReal32	in_fUpperY,
		AkReal32	in_fX
		);

	// Linearly interpolate, without checking for out-of-bounds condition
	AkForceInline AkReal32 InterpolateNoCheck(
		AkReal32 in_fLowerX,AkReal32 in_fLowerY,
		AkReal32 in_fUpperX,AkReal32 in_fUpperY,
		AkReal32 in_fX
		)
	{
		AkReal32 fA = ( in_fX - in_fLowerX ) / ( in_fUpperX - in_fLowerX );
		return in_fLowerY + ( fA * (in_fUpperY - in_fLowerY) );
	}
	// 
	extern AkReal32 NormalisedInterpolate(
		AkReal32	in_fLowerY,
		AkReal32	in_fUpperY,
		AkReal32	in_fX
		);

	extern void MatrixMul3by3(
		AkReal32 A[3][3],
		AkReal32 B[3][3],
		AkReal32 C[3][3]
		);

	// returns in_fValue ^ in_fPower
	AkForceInline AkReal32 Pow(
		AkReal32	in_fValue,
		AkReal32	in_fPower
		)
	{
		return powf(in_fValue,in_fPower);
	}

	// 
	AkForceInline AkReal32 Log(
		AkReal32			in_fValue
		)
	{
		return log(in_fValue);
	}

	// 
	AkForceInline AkReal32 Log10(
		AkReal32			in_fValue
		)
	{
		return log10(in_fValue);
	}

	// returns cos(in_fAngle)
	AkForceInline AkReal32 Cos(
		AkReal32			in_fAngle			// radians
		)
	{
		return cos(in_fAngle);
	}

	// returns acos(in_fAngle)
	AkForceInline AkReal32 ACos(
		AkReal32			in_fAngle
		)
	{
		AKASSERT((in_fAngle <= 1.0f) && (in_fAngle >= -1.0f));
		return acos(in_fAngle);
	}

	// 
	AkForceInline AkReal32 Sin(
		AkReal32			in_fAngle			// radians
		)
	{
		return sin(in_fAngle);
	}

	// retuns |in_fNumber|
	AkForceInline AkReal32 Abs(
		AkReal32			in_fNumber
		)
	{
		return fabs(in_fNumber);
	}

	// returns a random float value between in_fMin and in_fMax
	extern AkReal32 RandRange(
		AkReal32			in_fMin,
		AkReal32			in_fMax
		);

	// convert decibels [-96.3..0] to linear scale [0..1]
	// DO NOT USE FOR VOLUME OFFSETS, their range is greater than the table.
	extern AkReal32 dBToLin(
		AkReal32 in_fdB 
		);

	// Faster log10 function (for lin2Db conversions)
	// Valid for range [0..1]
	extern AkReal32 FastLog10( 
		AkReal32 fX 
		);

	// Valid for range [0..1]
#ifndef __SPU__
	AkForceInline AkReal32 FastLinTodB( AkReal32 in_fLinValue )
	{
		return 20 * FastLog10( in_fLinValue );
	}
#endif

	// returns the minimum value
	AkForceInline AkReal32 Min(
		AkReal32			in_value1,
		AkReal32			in_value2
		)
	{
#ifdef XBOX360
		return (AkReal32)fpmin(in_value1, in_value2);
#elif defined(__PPU__)
		// __fsels( a, c, b );
		// if(a >= 0) return c;
		// else return b;
		return __fsels( in_value1 - in_value2, in_value2, in_value1 );
#else
		return AkMin(in_value1, in_value2);
#endif
	}

	// returns the maximum value
	AkForceInline AkReal32 Max(
		AkReal32			in_value1,
		AkReal32			in_value2
		)
	{
#ifdef XBOX360
		return (AkReal32)fpmax(in_value1, in_value2);
#elif defined(__PPU__)
		// __fsels( a, c, b );
		// if(a >= 0) return c;
		// else return b;
		return __fsels( in_value1 - in_value2, in_value1, in_value2 );
#else
		return AkMax(in_value1, in_value2);
#endif
	}
}

#ifdef RVL_OS
static inline AkReal32 AkSqrtEstimate( AkReal32 x )
{
	if(x > 0.0f)
	{
		AkReal32 guess = __frsqrte(x);   // returns an approximation to    
		AkReal32 hx = 0.5f * x;
		guess = guess * ( 1.5f - guess * guess * hx );  // now have 8 sig bits
		guess = guess * ( 1.5f - guess * guess * hx );  // now have 16 sig bits
		// Uncomment one line to gain accuracy
		//guess = guess * ( 1.5f - guess * guess * hx );  // now have 32 sig bits	// not working on floats, only on double
		//guess = guess * ( 1.5f - guess * guess * hx );  // now have > 53 sig bits // not working on floats, only on double
		return x * guess ;
	}
	return 0;
}
static inline AkReal32 AkInvSqrtEstimate( AkReal32 x )
{
	if(x > 0.0f)
	{
		AkReal32 guess = __frsqrte(x);   // returns an approximation to    
		AkReal32 hx = 0.5f * x;
		guess = guess * ( 1.5f - guess * guess * hx );  // now have 8 sig bits
		guess = guess * ( 1.5f - guess * guess * hx );  // now have 16 sig bits
		// Uncomment one line to gain accuracy
		//guess = guess * ( 1.5f - guess * guess * hx );  // now have 32 sig bits	// not working on floats, only on double
		//guess = guess * ( 1.5f - guess * guess * hx );  // now have > 53 sig bits // not working on floats, only on double
		return guess ;
	}
	return 0;
}
#endif

#endif
