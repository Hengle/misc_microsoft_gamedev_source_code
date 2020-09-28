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
// AkDynamicSequencePBI.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkDynamicSequencePBI.h"
#include "AkDynamicSequence.h"
#include "AkActionPlayAndContinue.h"
#include "AkAudioMgr.h"
#include "AkPlayingMgr.h"
#include "AkLEngine.h"

// Constructor
CAkDynamicSequencePBI::CAkDynamicSequencePBI(
		CAkSoundBase*	in_pSound,			// Sound associated to the PBI (NULL if none).	
		CAkSource*		in_pSource,
		CAkRegisteredObj * in_pGameObj,		// Game object.
		ContParams&		in_rCparameters,	// Continuation parameters.
		UserParams&		in_rUserparams,		// User parameters.
		PlayHistory&	in_rPlayHistory,	// History stuff.
		bool			in_bIsFirst,		// Is it the first play of a series.
		AkUniqueID		in_SeqID,			// Sample accurate sequence id.	 
        CAkPBIAware*    in_pInstigator,
		AkPriority		in_Priority,
		AK::SoundEngine::DynamicSequence::DynamicSequenceType in_eDynamicSequenceType )
: CAkContinuousPBI( in_pSound, in_pSource, in_pGameObj, in_rCparameters, in_rUserparams, in_rPlayHistory, in_bIsFirst, in_SeqID, in_pInstigator, in_Priority, false /*No support for feedback devices*/ )
, m_bRequestNextFromDynSeq( true )
, m_eDynamicSequenceType( in_eDynamicSequenceType )
{
}


CAkDynamicSequencePBI::~CAkDynamicSequencePBI( void )
{
}

void CAkDynamicSequencePBI::Term()
{
	CAkDynamicSequence* pDynamicSequence = static_cast<CAkDynamicSequence*>( m_pInstigator );

	g_pPlayingMgr->NotifyEndOfDynamicSequenceItem( this, pDynamicSequence->PlayingNodeID(), pDynamicSequence->PlayingNodeCustomInfo() );

	CAkContinuousPBI::Term();
}

void CAkDynamicSequencePBI::PrepareNextToPlay( bool in_bIsPreliminary )
{
	CAkContinuousPBI::PrepareNextToPlay( in_bIsPreliminary );

	if ( m_bIsNextPrepared && !m_bWasStopped )
	{
		if ( HasNextToPlay() )
		{
			m_bRequestNextFromDynSeq = false;
		}
		else if ( m_bRequestNextFromDynSeq  )
		{
			if( !in_bIsPreliminary || m_eDynamicSequenceType == AK::SoundEngine::DynamicSequence::DynamicSequenceType_SampleAccurate )
			{
				m_bRequestNextFromDynSeq = false;
				AkTimeMs delay = 0;
				CAkDynamicSequence* pDynamicSequence = static_cast<CAkDynamicSequence*>( m_pInstigator );
				AkUniqueID nextElement = pDynamicSequence->GetNextToPlay( delay );

				if ( nextElement != AK_INVALID_UNIQUE_ID )
				{
					PlayNextElement( nextElement, delay );
				}
			}
		}
	}
}

void CAkDynamicSequencePBI::PlayNextElement( AkUniqueID in_nextElementID, AkTimeMs in_delay )
{
	CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_nextElementID );
	if ( !pNode )
		return;
		
	ContParams Cparameters;

	Cparameters.pPlayStopTransition = m_PBTrans.pvPSTrans;
	Cparameters.pPauseResumeTransition = m_PBTrans.pvPRTrans;
	Cparameters.pPathInfo = GetPathInfo();
	Cparameters.ulPauseCount = m_ulPauseCount;

	Cparameters.bIsPlayStopTransitionFading = m_PBTrans.bIsPSTransFading;
	Cparameters.bIsPauseResumeTransitionFading = m_PBTrans.bIsPRTransFading;
	
	Cparameters.spContList.Attach( CAkContinuationList::Create() );

	TransParams	Tparameters;

	Tparameters.TransitionTime = 0;
	Tparameters.eFadeCurve = AkCurveInterpolation_Constant;

	AkPBIParams pbiParams;
    
	pbiParams.eType = AkPBIParams::DynamicSequencePBI;
    pbiParams.pInstigator = m_pInstigator;
	pbiParams.userParams = m_UserParams;
	pbiParams.ePlaybackState = PB_Playing;
	pbiParams.uFrameOffset = CAkTimeConv::MillisecondsToSamples( in_delay );
    pbiParams.bIsFirst = false;

	pbiParams.pGameObj = m_pGameObj;

	pbiParams.pTransitionParameters = &Tparameters;
    pbiParams.pContinuousParams = &Cparameters;
    pbiParams.sequenceID = m_SeqID;

	pNode->Play( pbiParams );

	if( m_bNeedNotifyEndReached )
	{
		m_bIsNotifyEndReachedContinuous = true;
	}

	pNode->Release();

	CAkLEngine::IncrementSyncCount();//We must increment it there, to ensure it will be processed.
}
