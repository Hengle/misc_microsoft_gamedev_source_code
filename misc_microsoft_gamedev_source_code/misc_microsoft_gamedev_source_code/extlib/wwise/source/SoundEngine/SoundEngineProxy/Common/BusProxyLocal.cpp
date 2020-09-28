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

#include "BusProxyLocal.h"
#include "AkBus.h"
#include "AkAudiolib.h"
#include "AkCritical.h"


BusProxyLocal::BusProxyLocal()
{
}

BusProxyLocal::~BusProxyLocal()
{
}

void BusProxyLocal::Init( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	SetIndexable( pIndexable != NULL ? pIndexable : CAkBus::Create( in_id ) );
}

void BusProxyLocal::Volume( AkReal32 in_volume )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Volume( in_volume );
	}
}

void BusProxyLocal::LFEVolume( AkReal32 in_LFEVolume )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->LFEVolume( in_LFEVolume );
	}
}

void BusProxyLocal::Pitch( AkPitchValue in_Pitch )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Pitch( in_Pitch );
	}
}

void BusProxyLocal::LPF( AkLPFType in_LPF )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->LPF( in_LPF );
	}
}

void BusProxyLocal::SetMaxNumInstancesOverrideParent( bool in_bOverride )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetMaxNumInstOverrideParent( in_bOverride );
	}
}

void BusProxyLocal::SetMaxNumInstances( AkUInt16 in_ulMaxNumInstance )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetMaxNumInstances( in_ulMaxNumInstance );
	}
}

void BusProxyLocal::SetMaxReachedBehavior( bool in_bKillNewest )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetMaxReachedBehavior( in_bKillNewest );
	}
}

void BusProxyLocal::AddChild( AkUniqueID in_id )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->AddChild( in_id );
	}
}

void BusProxyLocal::RemoveChild( AkUniqueID in_id )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->RemoveChild( in_id );
	}
}

void BusProxyLocal::RemoveAllChildren()
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->RemoveAllChildren();
	}
}

void BusProxyLocal::SetRecoveryTime(AkTimeMs in_recoveryTime)
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetRecoveryTime( CAkTimeConv::MillisecondsToSamples( in_recoveryTime ) );
	}
}

void BusProxyLocal::AddDuck(
		AkUniqueID in_busID,
		AkVolumeValue in_duckVolume,
		AkTimeMs in_fadeOutTime,
		AkTimeMs in_fadeInTime,
		AkCurveInterpolation in_eFadeCurve
		)
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->AddDuck( in_busID, in_duckVolume, in_fadeOutTime, in_fadeInTime, in_eFadeCurve );
	}
}

void BusProxyLocal::RemoveDuck( AkUniqueID in_busID )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveDuck( in_busID );
	}
}

void BusProxyLocal::RemoveAllDuck()
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveAllDuck();
	}
}

void BusProxyLocal::SetAsBackgroundMusic()
{
#ifdef XBOX360
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->XMP_SetAsXMPBus();
	}
#endif
}

void BusProxyLocal::UnsetAsBackgroundMusic()
{
#ifdef XBOX360
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->XMP_UnsetAsXMPBus();
	}
#endif
}

void BusProxyLocal::EnableWiiCompressor( bool in_bEnable )
{
	CAkBus::EnableHardwareCompressor( in_bEnable );
}

void BusProxyLocal::FeedbackVolume( AkReal32 in_fFeedbackVolume )
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->FeedbackVolume( in_fFeedbackVolume );
	}
}

void BusProxyLocal::FeedbackLPF( AkLPFType in_fFeedbackLPF)
{
	CAkBus* pIndexable = static_cast<CAkBus*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->FeedbackLPF( in_fFeedbackLPF );
	}
}
