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


#include "stdafx.h"

#include "SoundProxyLocal.h"
#include "AkSound.h"
#include "AkAudiolib.h"
#include "AkCritical.h"


SoundProxyLocal::SoundProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkSound::Create( in_id ) );
}

SoundProxyLocal::~SoundProxyLocal()
{
}

void SoundProxyLocal::SetSource( AkUInt32 in_PluginID, AkLpCtstr in_szFileName, const AkAudioFormat& in_AudioFormat )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
#ifdef WIN32

#pragma message( "Remove this when AL is built in non-unicode" )

#ifdef _UNICODE
		pIndexable->SetSource( in_PluginID, in_szFileName, in_AudioFormat );
#else
		USES_CONVERSION;
		pIndexable->SetSource( in_PluginID, A2CW( in_szFileName ), in_AudioFormat );
#endif
#endif
	}
}

void SoundProxyLocal::SetSource( AkPluginID in_ID, void* in_vpParam, AkUInt32 in_ulSize )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetSource( in_ID, in_vpParam, in_ulSize );
	}
}

void SoundProxyLocal::SetSrcParam( AkPluginID in_ID, AkPluginParamID in_ParamID, void* in_vpParam, AkUInt32 in_ulSize )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetSrcParam( in_ID, in_ParamID, in_vpParam, in_ulSize );
	}
}

void SoundProxyLocal::Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_sLoopCount, AkInt16 in_sCountModMin, AkInt16 in_sCountModMax )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Loop( in_bIsLoopEnabled, in_bIsLoopInfinite, in_sLoopCount, in_sCountModMin, in_sCountModMax );
	}
}

void SoundProxyLocal::IsZeroLatency( bool in_bIsZeroLatency )
{
	CAkSound* pIndexable = static_cast<CAkSound*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->IsZeroLatency(in_bIsZeroLatency);
	}
}
