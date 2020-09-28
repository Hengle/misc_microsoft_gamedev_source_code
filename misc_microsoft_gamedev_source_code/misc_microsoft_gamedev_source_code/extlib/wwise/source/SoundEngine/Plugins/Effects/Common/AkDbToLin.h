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

// Copyright (C) 2006 Audiokinetic Inc.
/// \file 
/// Platform independent optimized dB to linear conversion services for software plugins.

#ifndef _AK_PLUGINDBTOLIN_H_
#define _AK_PLUGINDBTOLIN_H_

#ifdef __SPU__

#include <math.h>

namespace AK
{
	static AkForceInline AkReal32 dBToLin( AkReal32 in_fdB )
	{
		return powf( 10.f, in_fdB * 0.05 );
	}

	static AkForceInline AkReal32 FastLog10( AkReal32 fX )
	{
		return log10f( fX );
	}
}

#else

#include <AK/SoundEngine/Common/AkTypes.h>

namespace AK
{

	#define DBTOLINTABLEOFFSET (97)
	/// Interpolation table for dB to linear conversion
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

	/// Convert decibels to linear scale.
	/// \note Supports (-96.3,0) input range
	/// \return Linear value
	static AkForceInline AkReal32 dBToLin(
		AkReal32 in_fdB	///< Input dB value
		)
	{
		// Clamp out-of-range input values
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
	/// DO NOT USE POSITIVE VALUES
	AkForceInline AkReal32 FastLog10( AkReal32 fX )
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

#endif // __SPU__

#endif // _AK_PLUGINDBTOLIN_H_
