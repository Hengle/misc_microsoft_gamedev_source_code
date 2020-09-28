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
// PrivateStructures.h
//
// AkAudioLib Internal structures definitions
//
//////////////////////////////////////////////////////////////////////
#ifndef _PRIVATE_STRUCTURES_H_
#define _PRIVATE_STRUCTURES_H_

#include "AkContinuationList.h"
#include "AkSettings.h"

struct UserParams
{
	AkCustomParamType		CustomParam;
	AkPlayingID				PlayingID;
};

//----------------------------------------------------------------------------------------------------
// parameters needed for play and play&continue actions
//----------------------------------------------------------------------------------------------------
class CAkTransition;
class CAkTransition;
class CAkPath;

// Synchronisation type. Applies to Source transitions as well as to State changes.
enum AkSyncType
{
    SyncTypeImmediate		= 0,
    SyncTypeNextGrid		= 1,
    SyncTypeNextBar			= 2,
    SyncTypeNextBeat		= 3,
    SyncTypeNextMarker		= 4,
    SyncTypeNextUserMarker	= 5,
    SyncTypeEntryMarker		= 6,    // not used with SrcTransRules.
    SyncTypeExitMarker		= 7,

#define NUM_BITS_SYNC_TYPE  (5)
};

enum AkPlaybackState
{
	PB_Playing,
	PB_Paused,
};

struct AkPathInfo
{
	CAkPath*	pPBPath;
	AkUniqueID	PathOwnerID;
};

struct ContParams
{
	// Default constructor
	ContParams()
	{
		// doesn't init a thing by default
	}

	// Copy constructor, copy everything BUT THE spContList
	ContParams( ContParams* in_pFrom )
		: pPlayStopTransition( in_pFrom->pPlayStopTransition ) 
		, pPauseResumeTransition( in_pFrom->pPauseResumeTransition )
		, pPathInfo( in_pFrom->pPathInfo )
		, bIsPlayStopTransitionFading( in_pFrom->bIsPlayStopTransitionFading )
		, bIsPauseResumeTransitionFading( in_pFrom->bIsPauseResumeTransitionFading )
		, ulPauseCount( in_pFrom->ulPauseCount )
	{
		// DO NOT COPY spContList object.
	}
	
	ContParams( AkPathInfo* in_pPathInfo )
		: pPlayStopTransition( NULL ) 
		, pPauseResumeTransition( NULL )
		, pPathInfo( in_pPathInfo )
		, bIsPlayStopTransitionFading( false )
		, bIsPauseResumeTransitionFading( false )
		, ulPauseCount( 0 )
	{
	}

	CAkTransition*						pPlayStopTransition;		// the running play / stop transition
	CAkTransition*						pPauseResumeTransition;		// the running pause / resume transition
	AkPathInfo*							pPathInfo;					// the current path if any
	bool								bIsPlayStopTransitionFading;
	bool								bIsPauseResumeTransitionFading;
	CAkSmartPtr<CAkContinuationList>	spContList;
	AkUInt32							ulPauseCount;
};

struct TransParams
{
	AkTimeMs				TransitionTime;				// how long this should take
	AkCurveInterpolation	eFadeCurve;					// what shape it should have
};

struct PlaybackTransition
{
	CAkTransition*		pvPSTrans;	// Play Stop transition reference.
	CAkTransition*		pvPRTrans;	// Pause Resume transition reference.
	AkUInt8				bIsPSTransFading : 1;
	AkUInt8				bIsPRTransFading : 1;
	PlaybackTransition()
		:pvPSTrans( NULL )
		,pvPRTrans( NULL )
		,bIsPSTransFading( false )
		,bIsPRTransFading( false )
	{}
};

// Time conversion helpers.
// ---------------------------------------------------------
class CAkTimeConv
{
public:
    static AkInt32 MicrosecondsToSamples( 
        AkReal64 in_microseconds 
        )
    {
        return (AkInt32)( (AkReal64)( (in_microseconds/(AkReal64)1.0e6) * (AkReal64)AK_CORE_SAMPLERATE ) );
    }
    static AkInt32 MillisecondsToSamples( 
        AkReal64 in_milliseconds 
        )
    {
        return (AkInt32)( (AkReal64)( (in_milliseconds/(AkReal64)1000.0) * (AkReal64)AK_CORE_SAMPLERATE ) );
    }
	static AkInt32 MillisecondsToSamples( 
        AkTimeMs in_milliseconds 
        )
    {
		return static_cast<AkInt32>( static_cast<AkInt64>( in_milliseconds ) * AK_CORE_SAMPLERATE / 1000 );
    }
    static AkInt32 SecondsToSamples( 
        AkReal64 in_seconds 
        )
    {
        return (AkInt32)( (AkReal64)( in_seconds * (AkReal64)AK_CORE_SAMPLERATE ) );
    }
    static AkTimeMs SamplesToMilliseconds( 
        AkInt32 in_samples
        )
    {
        return (AkTimeMs)( 1000 * (AkReal64)in_samples / (AkReal64)AK_CORE_SAMPLERATE );
    }
	static AkReal64 SamplesToSeconds( 
        AkInt32 in_samples
        )
    {
        return (AkReal64)( (AkReal64)in_samples / (AkReal64)AK_CORE_SAMPLERATE );
    }
};

#endif
