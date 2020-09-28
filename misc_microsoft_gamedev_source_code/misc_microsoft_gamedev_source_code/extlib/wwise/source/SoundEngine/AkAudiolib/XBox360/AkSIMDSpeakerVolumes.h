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
#include <vectorintrinsics.h>
#include "PlatformAudiolibDefs.h"
#include "AkMath.h"

struct AkSIMDSpeakerVolumes
{
	union
	{
		__vector4 vector[2];

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
		vector[1].v[2] = 0;
		vector[1].v[3] = 0;
#endif
		return *this;
	}

	AkForceInline void Set( AkReal32 in_fVol )
	{
		__vector4 vfVolume =  __lvlx( &in_fVol, 0 );
		Set( vfVolume );
	}

	AkForceInline void Set( __vector4 in_fVol )
	{
		vector[0] = vector[1] = __vspltw( in_fVol, 0 );
	}

	AkForceInline void Zero()
	{
		vector[0] = vector[1] = __vspltisw( 0 );
	}

	AkForceInline void Add( AkReal32 in_fVol )
	{
		__vector4 vfVolume = __lvlx( &in_fVol, 0 );
		Add( vfVolume );
	}
	
	AkForceInline void Add( AkSIMDSpeakerVolumes & in_vol )
	{
		__vector4 vTmp0 = __vaddfp( vector[0], in_vol.vector[0] );
		__vector4 vTmp1 = __vaddfp( vector[1], in_vol.vector[1] );
		vector[0] = vTmp0;
		vector[1] = vTmp1;
	}

	AkForceInline void Add( __vector4 in_fVol )
	{
		__vector4 vAdd = __vspltw( in_fVol, 0 );
		__vector4 vTmp0 = __vaddfp( vector[0], vAdd );
		__vector4 vTmp1 = __vaddfp( vector[1], vAdd );
		vector[0] = vTmp0;
		vector[1] = vTmp1;
	}

	AkForceInline void Mul( AkReal32 in_fVol )
	{
		__vector4 vfVolume = __lvlx( &in_fVol, 0 );
		Mul( vfVolume );
	}
	
	AkForceInline void Mul( __vector4 in_fVol )
	{
		__vector4 vMul = __vspltw( in_fVol, 0 );
		__vector4 vTmp0 = __vmulfp( vector[0], vMul );
		__vector4 vTmp1 = __vmulfp( vector[1], vMul );
		vector[0] = vTmp0;
		vector[1] = vTmp1;
	}

	AkForceInline void Mul( AkSIMDSpeakerVolumes & in_vol )
	{
		__vector4 vTmp0 = __vmulfp( vector[0], in_vol.vector[0] );
		__vector4 vTmp1 = __vmulfp( vector[1], in_vol.vector[1] );
		vector[0] = vTmp0;
		vector[1] = vTmp1;
	}

	AkForceInline void Max( AkSIMDSpeakerVolumes & in_vol )
	{
		__vector4 vTmp0 = __vmaxfp( vector[0], in_vol.vector[0] );
		__vector4 vTmp1 = __vmaxfp( vector[1], in_vol.vector[1] );
		vector[0] = vTmp0;
		vector[1] = vTmp1;
	}

	AkForceInline void Sqrt()
	{
#if 1
		volumes.fFrontLeft = sqrtf( volumes.fFrontLeft );
		volumes.fFrontRight = sqrtf( volumes.fFrontRight );
		volumes.fCenter = sqrtf( volumes.fCenter );
		volumes.fLfe = sqrtf( volumes.fLfe );
		volumes.fRearLeft = sqrtf( volumes.fRearLeft );
		volumes.fRearRight = sqrtf( volumes.fRearRight );
#ifdef AK_71AUDIO
		volumes.fExtraLeft = sqrtf( volumes.fExtraLeft );
		volumes.fExtraRight = sqrtf( volumes.fExtraRight );
#endif
#else
		vector[0] = __vrsqrtefp( vector[0] );
		vector[1] = __vrsqrtefp( vector[1] );
#endif
	}

	AkForceInline void CopyTo( AkSpeakerVolumes & out_vol ) const
	{
		out_vol = *((AkSpeakerVolumes *) this );
	}

	AkForceInline bool IsLessOrEqual( float * in_pThreshold )
	{
		static const AkSIMDVectorUInt16 v51Shuffle = { 0x00010203, 0x04050607, 0x00010203, 0x04050607 };

		__vector4 vThreshold = vec_loadAndSplatScalar( in_pThreshold );

		unsigned int compareResult0, compareResult1;
		__vcmpgtfpR(vector[0], vThreshold, &compareResult0);
		__vector4 vTmp = __vperm( vector[1], vector[1], *((__vector4 *) &v51Shuffle) ); // duplicate the first two volumes 
		__vcmpgtfpR(vTmp, vThreshold, &compareResult1);

		// Bit 5 of the CR is set to 1 if none of the four elements in vSrcA are greater than 
		// the corresponding element in vSrcB

		return ( ( compareResult0 & compareResult1 ) & (1<<5) ) != 0 ; 
	}

	AkForceInline void dBToLin()
	{
		static const float K = 0.16609640474436813f; // log2(10)/20
		static const __vector4 vK = { K, K, K, K };

		__vector4 vTmp0 = __vexptefp( __vmulfp( vector[0], vK ) );
		__vector4 vTmp1 = __vexptefp( __vmulfp( vector[1], vK ) );

		vector[0] = vTmp0;
		vector[1] = vTmp1;
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
