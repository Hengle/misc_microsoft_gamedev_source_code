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
// AkMath.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h" 
#include <stdlib.h>			// rand()

#include "AkMath.h"
#include "AkGen3DParams.h"
#include "AkSpeakerPan.h"

namespace AkMath
{
	//====================================================================================================
	//====================================================================================================
	AkReal32 ToRadians(AkReal32 in_fDegrees)
	{
		AkReal32	fRadians;
		int	iThreeSixty;

		iThreeSixty = static_cast<int>(in_fDegrees * ONEOVERTHREESIXTY);
		fRadians = (in_fDegrees - static_cast<AkReal32>(iThreeSixty)) * PIOVERONEEIGHTY;

		return fRadians;
	}
	//====================================================================================================
	//====================================================================================================
	AkReal32 ToDegrees(AkReal32 in_fRadians)
	{
		AkReal32	fDegrees;
		int	iTwoPi;

		iTwoPi = static_cast<int>(in_fRadians / TWOPI);
		fDegrees = (in_fRadians - static_cast<AkReal32>(iTwoPi)) * ONEEIGHTYOVERPI;

		return fDegrees;
	}
	//====================================================================================================
	//====================================================================================================
	void Normalise(AkVector& io_Vector )
	{
#ifdef RVL_OS
		AkReal32 fDistanceSqr = 
					io_Vector.X * io_Vector.X
					+ io_Vector.Y * io_Vector.Y
					+ io_Vector.Z * io_Vector.Z;
					
		if ( fDistanceSqr != 0.0f )
		{
			AkReal32 fMagnitudeInv = AkInvSqrtEstimate( fDistanceSqr );
			io_Vector.X *= fMagnitudeInv;
			io_Vector.Y *= fMagnitudeInv;
			io_Vector.Z *= fMagnitudeInv;
		}
#else
		AkReal32 fMagnitude = Magnitude(io_Vector);

		// PhM : this prevents from dividing by 0,
		// but does not prevent from getting huge values
		if(fMagnitude != 0.0f)
		{
			io_Vector.X /= fMagnitude;
			io_Vector.Y /= fMagnitude;
			io_Vector.Z /= fMagnitude;
		}
#endif
	}
	//====================================================================================================
	//====================================================================================================
	AkReal32 Magnitude(AkVector& in_rVector)
	{
#ifdef RVL_OS
		return AkSqrtEstimate(in_rVector.X * in_rVector.X
					+ in_rVector.Y * in_rVector.Y
					+ in_rVector.Z * in_rVector.Z);
#else
		return sqrt(in_rVector.X * in_rVector.X
					+ in_rVector.Y * in_rVector.Y
					+ in_rVector.Z * in_rVector.Z);
#endif
	}
	//====================================================================================================
	//====================================================================================================
	AkReal32 DotProduct(const AkVector& Vector1, const AkVector& Vector2)
	{
		return Vector1.X * Vector2.X + Vector1.Y * Vector2.Y + Vector1.Z * Vector2.Z;
	}
	//====================================================================================================
	//====================================================================================================
	AkVector CrossProduct(AkVector& Vector1, AkVector& Vector2)
	{
		AkVector	Result;

		Result.X = Vector1.Y * Vector2.Z - Vector1.Z * Vector2.Y;
		Result.Y = Vector1.Z * Vector2.X - Vector1.X * Vector2.Z;
		Result.Z = Vector1.X * Vector2.Y - Vector1.Y * Vector2.X;

		return Result;
	}
	//====================================================================================================
	//====================================================================================================
	AkReal32 Distance(const AkVector& Position1, const AkVector& Position2)
	{
		AkVector	Distance;

		Distance.X = Position1.X - Position2.X;
		Distance.Y = Position1.Y - Position2.Y;
		Distance.Z = Position1.Z - Position2.Z;

		return Magnitude(Distance);
	}
	//====================================================================================================
	// interpolates between |in_fLowerX and |in_fUpperX at position in_fX
	//                      |in_fLowerY     |in_fUpperY
	//====================================================================================================
	AkReal32 Interpolate(AkReal32 in_fLowerX,AkReal32 in_fLowerY,
								AkReal32 in_fUpperX,AkReal32 in_fUpperY,
								AkReal32 in_fX)
	{
		AkReal32 fInterpolated;

#ifdef XBOX360_DISABLED //this code doesn't work when in_fUpperX == in_fLowerX
		// are we below the min ?
		// fpDest = fpRegA >= 0 ? fpRegB : fpRegA
		AkReal32 fx = (AkReal32) __fsel(in_fX - in_fLowerX, in_fX, in_fLowerX);

		// are we above the max ?
		// fpDest = fpRegA >= 0 ? fpRegB : fpRegA
		fx = (AkReal32) __fsel(in_fUpperX - fx, fx, in_fUpperX);

		// compute this anyway
		AkReal32 fA = ( fx - in_fLowerX ) / ( in_fUpperX - in_fLowerX );
		fInterpolated = in_fLowerY + ( fA * (in_fUpperY - in_fLowerY) );
#else
		// are we below the min ?
		if(in_fX <= in_fLowerX)
		{
			fInterpolated = in_fLowerY;
		}
		// are we above the mas ?
		else if(in_fX >= in_fUpperX)
		{
			fInterpolated = in_fUpperY;
		}
		// we're in between
		else
		{
			AkReal32 fA = ( in_fX - in_fLowerX ) / ( in_fUpperX - in_fLowerX );
			fInterpolated = in_fLowerY + ( fA * (in_fUpperY - in_fLowerY) );
		}
#endif

		return fInterpolated;
	}
	//====================================================================================================
	// interpolates between |0.0f       and |1.0f       at position in_fX
	//                      |in_fLowerY     |in_fUpperY
	//====================================================================================================
	AkReal32 NormalisedInterpolate(AkReal32 in_fLowerY,
											AkReal32 in_fUpperY,
											AkReal32 in_fX)
	{
		AkReal32 fInterpolated;

		// are we below the min ?
		if(in_fX <= 0.0f)
		{
			fInterpolated = in_fLowerY;
		}
		// are we above the mas ?
		else if(in_fX >= 1.0f)
		{
			fInterpolated = in_fUpperY;
		}
		// we're in between
		else
		{
			fInterpolated = in_fLowerY + ( in_fX * (in_fUpperY - in_fLowerY) );
		}

		return fInterpolated;
	}
	//====================================================================================================
	//====================================================================================================
	void MatrixMul3by3(
		AkReal32 A[3][3],
		AkReal32 B[3][3],
		AkReal32 C[3][3]
		)
	{
		C[0][0] = A[0][0] * B[0][0] + A[0][1] * B[1][0] + A[0][2] * B[2][0];
		C[0][1] = A[0][0] * B[0][1] + A[0][1] * B[1][1] + A[0][2] * B[2][1];
		C[0][2] = A[0][0] * B[0][2] + A[0][1] * B[1][2] + A[0][2] * B[2][2];

		C[1][0] = A[1][0] * B[0][0] + A[1][1] * B[1][0] + A[1][2] * B[2][0];
		C[1][1] = A[1][0] * B[0][1] + A[1][1] * B[1][1] + A[1][2] * B[2][1];
		C[1][2] = A[1][0] * B[0][2] + A[1][1] * B[1][2] + A[1][2] * B[2][2];

		C[2][0] = A[2][0] * B[0][0] + A[2][1] * B[1][0] + A[2][2] * B[2][0];
		C[2][1] = A[2][0] * B[0][1] + A[2][1] * B[1][1] + A[2][2] * B[2][1];
		C[2][2] = A[2][0] * B[0][2] + A[2][1] * B[1][2] + A[2][2] * B[2][2];
	}
	//====================================================================================================
	// RangeRange returns a random float value between in_fMin and in_fMax
	//====================================================================================================
	AkReal32 RandRange(AkReal32 in_fMin, AkReal32 in_fMax )
	{
		// Note: RAND_MAX is in stdlib.h and RAND_MAX value may be platform independent
		// There may be a need in the future depending on target platforms to adapt this 
		// code. For the moment being all supported platforms should have no problem with the following.

		// Ensure max provided is above the minimum
		if (in_fMax < in_fMin)
		{
			in_fMax = in_fMin;
		}
		
		// Get an integer in range (0,RAND_MAX)
		AkReal32 fRandVal = rand() / static_cast<AkReal32>(RAND_MAX);
		return ( fRandVal * (in_fMax - in_fMin) + in_fMin );
	}

	//====================================================================================================
	//====================================================================================================

	#define DBTOLINTABLEOFFSET (97)
	static const AkReal32 dBToLinTable[] = 
	{
		0.0000141254f,0.0000158489f,0.0000177828f,0.0000199526f,0.0000223872f,0.0000251189f,0.0000281838f,0.0000316228f,
		0.0000354813f,0.0000398107f,0.0000446684f,0.0000501187f,0.0000562341f,0.0000630957f,0.0000707946f,0.0000794328f,
		0.0000891251f,0.0001000000f,0.0001122018f,0.0001258925f,0.0001412538f,0.0001584893f,0.0001778279f,0.0001995262f,
		0.0002238721f,0.0002511886f,0.0002818383f,0.0003162278f,0.0003548134f,0.0003981072f,0.0004466836f,0.0005011872f,
		0.0005623413f,0.0006309573f,0.0007079458f,0.0007943282f,0.0008912509f,0.0010000000f,0.0011220185f,0.0012589254f,
		0.0014125375f,0.0015848932f,0.0017782794f,0.0019952623f,0.0022387211f,0.0025118864f,0.0028183829f,0.0031622777f,
		0.0035481339f,0.0039810717f,0.0044668359f,0.0050118723f,0.0056234133f,0.0063095734f,0.0070794578f,0.0079432823f,
		0.0089125094f,0.0100000000f,0.0112201845f,0.0125892541f,0.0141253754f,0.0158489319f,0.0177827941f,0.0199526231f,
		0.0223872114f,0.0251188643f,0.0281838293f,0.0316227766f,0.0354813389f,0.0398107171f,0.0446683592f,0.0501187234f,
		0.0562341325f,0.0630957344f,0.0707945784f,0.0794328235f,0.0891250938f,0.1000000000f,0.1122018454f,0.1258925412f,
		0.1412537545f,0.1584893192f,0.1778279410f,0.1995262315f,0.2238721139f,0.2511886432f,0.2818382931f,0.3162277660f,
		0.3548133892f,0.3981071706f,0.4466835922f,0.5011872336f,0.5623413252f,0.6309573445f,0.7079457844f,0.7943282347f,
		0.8912509381f,1.0000000000f,1.0000000000f
	};

	// convert decibels [-96.3..0] to linear scale [0..1]
	AkReal32 dBToLin(
		AkReal32 in_fdB 
		)
	{
		// clamp out-of-range input values

		if ( in_fdB <= -96.3f )
			return 0.0f;
		else if ( in_fdB >= 0.0f )
			return 1.0f;

		AkReal32 fInterpLoc = in_fdB+DBTOLINTABLEOFFSET;

		unsigned int ulLeftIndex;

#ifdef WIN32
		static const AkReal32 fHalf = 0.5f; 
		_asm
		{
			fld fInterpLoc
			fsub fHalf
			fistp ulLeftIndex
		};
#else
		ulLeftIndex = static_cast<unsigned int>( fInterpLoc );
#endif

		AkReal32 fLeftValue = dBToLinTable[ulLeftIndex];
		AkReal32 fRightValue = dBToLinTable[ulLeftIndex+1];
		AkReal32 fDistFromLeft = fInterpLoc - ulLeftIndex;
		// Linear interpolated value
		return ( fLeftValue*(1.f-fDistFromLeft) + fRightValue*fDistFromLeft );
	}
	
	/// Computes log10(x) faster.
	/// \note Avoid 0 input values.
	/// \return Logarithmic value
	AkReal32 FastLog10( AkReal32 fX )
	{
		static const AkReal32 LOGN2 = 0.6931471805f;
		static const AkReal32 INVLOGN10 = 0.4342944819f;
		static const AkUInt32 SETEXPZEROMASK = 0x3F800000;

		// log10(fX) = ln(fX) / ln(10)
		// Range contraction -> ln(fX) = exponent(fX) * ln(2) + ln(mantissa(fX))

		union 
		{
			AkReal32 f;
			AkUInt32 u;
		} IntOrFloat;

		// Extract float mantissa
		IntOrFloat.f = fX;
		IntOrFloat.u |= SETEXPZEROMASK;
		AkReal32 fMantissa = IntOrFloat.f;

		// Extract float exponent
		IntOrFloat.f = fX;
		IntOrFloat.u <<= 1;
		IntOrFloat.u >>= 24;
		AkReal32 fExp = IntOrFloat.u - 127.f;

		//// 5th order Taylor series approximation of ln(mantissa(fX))
		//// Note: Worst case error: 0.0012512 dB
		//AkReal32 fZ = (fMantissa-1.f)/(fMantissa+1.f);
		//AkReal32 fZ2 = fZ*fZ;
		//AkReal32 fZ3 = fZ*fZ2;
		//AkReal32 fLnfXMantissa = 2.f*(fZ+0.3333333333f*fZ3+0.2f*fZ3*fZ2);

		// 3rd order Taylor series approximation of ln(mantissa(fX))
		// Note: Worst case error: 0.0155487 dB
		AkReal32 fZ = (fMantissa-1.f)/(fMantissa+1.f);
		AkReal32 fLnfXMantissa = 2.f*(fZ + 0.3333333333f*fZ*fZ*fZ);

		// Reconstruct ln(fX)
		AkReal32 fLnfX = fExp * LOGN2 + fLnfXMantissa;

		// Compute log10(fX)
		return fLnfX * INVLOGN10;
	}
}
