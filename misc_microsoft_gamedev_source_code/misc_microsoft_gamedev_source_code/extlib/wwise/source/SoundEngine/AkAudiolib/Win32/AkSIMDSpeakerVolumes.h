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

#include <AK/SoundEngine/Common/AkTypes.h>
#include <xmmintrin.h>
#include "AkMath.h"

struct AkSIMDSpeakerVolumes
{
	union
	{
		__m128 vector[2];

		AkSpeakerVolumes volumes;

		AkReal32 aVolumes[8];
	};

	AkForceInline AkSIMDSpeakerVolumes & operator=( const AkSIMDSpeakerVolumes& in_vol )
	{
		vector[0] = in_vol.vector[0];
		vector[1] = in_vol.vector[1];

		return *this;
	}

	AkForceInline AkSIMDSpeakerVolumes & operator=( const AkSpeakerVolumes& in_vol )
	{
		*((AkSpeakerVolumes *) this ) = in_vol;
		
#ifndef AK_71AUDIO
		vector[1].m128_f32[2] = 0;
		vector[1].m128_f32[3] = 0;
#endif

		return *this;
	}

	AkForceInline void Set( AkReal32 in_fVol )
	{
		__m128 vTmp = _mm_load_ps1( &in_fVol ); 
		vector[0] = vTmp;
		vector[1] = vTmp;
	}

	AkForceInline void Zero()
	{
		__m128 vTmp = _mm_setzero_ps(); 
		vector[0] = vTmp;
		vector[1] = vTmp;
	}

	AkForceInline void Add( AkReal32 in_fVol )
	{
		__m128 vTmp = _mm_load_ps1( &in_fVol ); 
		vector[0] = _mm_add_ps( vector[0], vTmp );
		vector[1] = _mm_add_ps( vector[1], vTmp );
	}

	AkForceInline void Add( AkSIMDSpeakerVolumes & in_vol )
	{
		vector[0] = _mm_add_ps( vector[0], in_vol.vector[0] );
		vector[1] = _mm_add_ps( vector[1], in_vol.vector[1] );
	}

	AkForceInline void Mul( AkReal32 in_fVol )
	{
		__m128 vTmp = _mm_load_ps1( &in_fVol ); 
		vector[0] = _mm_mul_ps( vector[0], vTmp );
		vector[1] = _mm_mul_ps( vector[1], vTmp );
	}

	AkForceInline void Mul( AkSIMDSpeakerVolumes & in_vol )
	{
		vector[0] = _mm_mul_ps( vector[0], in_vol.vector[0] );
		vector[1] = _mm_mul_ps( vector[1], in_vol.vector[1] );
	}

	AkForceInline void Max( AkSIMDSpeakerVolumes & in_vol )
	{
		vector[0] = _mm_max_ps( vector[0], in_vol.vector[0] );
		vector[1] = _mm_max_ps( vector[1], in_vol.vector[1] );
	}

	AkForceInline void Sqrt()
	{
		vector[0] = _mm_sqrt_ps( vector[0] );
		vector[1] = _mm_sqrt_ps( vector[1] );
	}

	AkForceInline void CopyTo( AkSpeakerVolumes & out_vol ) const
	{
		out_vol = *((AkSpeakerVolumes *) this );
	}

	AkForceInline void CopyTo( AkSIMDSpeakerVolumes & out_vol ) const
	{
		_mm_storeu_ps( (float*)&out_vol, vector[0] );
		_mm_storeu_ps( (float*)&out_vol + 4, vector[1] );		
	}

	AkForceInline bool IsLessOrEqual( float * in_pThreshold )
	{
		__m128 vThreshold = _mm_load_ps1( in_pThreshold ); 
		__m128 vResult0 = _mm_cmple_ps( vector[0], vThreshold );
		__m128 vResult1 = _mm_cmple_ps( vector[1], vThreshold );
		int result0 = _mm_movemask_ps( vResult0 ); 
		int result1 = _mm_movemask_ps( vResult1 ); 
		return ((result0 == 0xf) & ((result1 & 3)==3)) != 0; // entire 4-bit mask is set when ALL are less than
	}

	AkForceInline void dBToLin()
	{
		volumes.fFrontLeft = AkMath::dBToLin( volumes.fFrontLeft );
		volumes.fFrontRight = AkMath::dBToLin( volumes.fFrontRight );
		volumes.fCenter = AkMath::dBToLin( volumes.fCenter );
		volumes.fLfe = AkMath::dBToLin( volumes.fLfe );
		volumes.fRearLeft = AkMath::dBToLin( volumes.fRearLeft );
		volumes.fRearRight = AkMath::dBToLin( volumes.fRearRight );
	}

	AkForceInline void FastLinTodB()
	{
		volumes.fFrontLeft = AkMath::FastLinTodB( volumes.fFrontLeft );
		volumes.fFrontRight = AkMath::FastLinTodB( volumes.fFrontRight );
		volumes.fCenter = AkMath::FastLinTodB( volumes.fCenter );
		volumes.fLfe = AkMath::FastLinTodB( volumes.fLfe );
		volumes.fRearLeft = AkMath::FastLinTodB( volumes.fRearLeft );
		volumes.fRearRight = AkMath::FastLinTodB( volumes.fRearRight );
	}
};