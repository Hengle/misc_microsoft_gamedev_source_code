/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkPBI.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkPBI.h"
#include "Ak3DListener.h"
#include "AkBankMgr.h"
#include "AkSoundBase.h"
#include "AkRegisteredObj.h"
#include "AkTransition.h"
#include "AkTransitionManager.h"
#include "AkPlayingMgr.h"
#include "AkParentNode.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include "AkRTPCMgr.h"						// g_pRTPCMgr
#include "AkDefault3DParams.h"
#include "AkPathManager.h"					// g_pPathManager
#include "AkURenderer.h"
#include "AkProfile.h"
#include "AkAudioMgr.h"
#include "AkGen3DParams.h"
#include "AkBus.h"
#include "AkEnvironmentsMgr.h"
#include "AkFXMemAlloc.h"
#include "AkLEngine.h"
#include "AkFeedbackNode.h"

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
extern CAkRegistryMgr*	g_pRegistryMgr;

#define MIN_NUM_MUTED_ITEM_IN_LIST 4


CAkLock		CAkPBI::m_csLock;

#ifndef AK_OPTIMIZED
CAkLock		CAkPBI::m_csLockParams;
#endif

CAkPBI::CAkPBI(	CAkSoundBase*	in_pSound,
			    CAkSource*		in_pSource,
				CAkRegisteredObj * in_pGameObj,
				UserParams&		in_rUserparams,
				PlayHistory&	in_rPlayHistory,
				AkUniqueID		in_SeqID,
				AkPriority		in_Priority,
				bool			in_bTargetFeedback)
{
	m_pUsageSlot					= NULL;
	m_pGameObj						= in_pGameObj;
	m_pSound						= in_pSound;
	m_pSource						= in_pSource;
	m_p3DSound						= NULL;
	m_b3DPositionDone				= false;
	m_bGetAudioParamsCalled			= false;
	m_Volume						= AK_MAXIMUM_VOLUME_LEVEL;
	m_RealEffectiveVolume			= AK_MAXIMUM_VOLUME_LEVEL;
	m_Lfe							= AK_MAXIMUM_VOLUME_LEVEL;
	m_RealEffectiveLfe				= AK_MAXIMUM_VOLUME_LEVEL;
	m_EffectivePitch				= AK_DEFAULT_PITCH;
	m_EffectiveLPF					= AK_DEFAULT_LOPASS_VALUE;
	m_bAreParametersValid			= false;
	
	m_cPlayStopFade					= UNMUTED_LVL;
	m_cPauseResumeFade				= UNMUTED_LVL;
	m_Priority                      = in_Priority;
	m_bNeedNotifyEndReached			= false;
	m_bTerminatedByStop				= false;
	m_State							= CtxStateStop;
	m_bWasStopped					= false;
	m_bWasPreStopped				= false;
	m_bWasPaused					= false;
	m_SeqID							= in_SeqID;
	m_bIsNotifyEndReachedContinuous	= false;
	m_bInitPlayWasCalled			= false;
	m_PathInfo.pPBPath				= NULL;
	m_PathInfo.PathOwnerID			= AK_INVALID_UNIQUE_ID;
	m_bWasKicked					= false;
	m_bWasPlayCountDecremented		= false;
#ifndef AK_OPTIMIZED
	m_bNotifyStarvationAtStart 		= false;	// True for PBIs that should not starve on start up (e.g. music).
#endif
	m_eInitialState					= PBI_InitState_Playing;
	m_ulPauseCount					= 0;
	m_LoopCount						= LOOPING_ONE_SHOT;
	m_SourceInfo.EffectTypeID		= AK_INVALID_PLUGINID;
	m_SourceInfo.pParam				= NULL;
	m_bPlayFailed 					= false;
	m_iFrameOffset					= 0;
	m_pDataPtr						= NULL;
	m_FirstPosition.uListenerIdx	= AK_INVALID_LISTENER_INDEX;
	m_FirstPosition.pos				= g_DefaultSoundPosition;

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		m_aFXInfo[ uFXIndex ].bIsBypassed	= false;
		m_aFXInfo[ uFXIndex ].EffectTypeID	= AK_INVALID_PLUGINID;
		m_aFXInfo[ uFXIndex ].FxReferenceID	= AK_INVALID_UNIQUE_ID;
		m_aFXInfo[ uFXIndex ].pParam		= NULL;
	}
	m_bBypassAllFX					= false;

	m_pFeedbackInfo					= NULL;
	m_bTargetIsFeedback				= in_bTargetFeedback;
	m_bFeedbackParametersValid		= false;

#ifdef RVL_OS
	m_bDoPriority					= false;
#endif

	m_pGameObj->AddRef();

	in_pSound->AddRef();
	in_pSound->AddPBI( this );
//	in_pSound->IncrementPlayCount( in_GameObjPtr ); Already incremented by caller for priority handling

	m_UserParams = in_rUserparams;

	m_CntrHistArray = in_rPlayHistory.HistArray;

#ifdef RVL_OS
	m_pGameObj->IncrementGameObjectPlayCount();
#endif
}

AKRESULT CAkPBI::Init( AkPathInfo* in_pPathInfo )
{
	AKASSERT( g_pRegistryMgr );
	AKASSERT( g_pPathManager );

	AKRESULT eResult = AK_Fail;
	
	if( m_UserParams.PlayingID )
	{
		AKASSERT( g_pPlayingMgr );
		eResult = g_pPlayingMgr->SetPBI( m_UserParams.PlayingID, this );
	}
	if( eResult == AK_Success )
	{
		// Create fx.
		eResult = CreateFx();
		if( eResult != AK_Success )
			return eResult;

		// Register the fx with the RTPC manager.
		eResult = SubscribeFxRTPC();
		if( eResult != AK_Success )
			return eResult;

		// Setup 3D sound.
		m_p3DSound = NULL;
		m_pSound->Get3DParams( m_p3DSound, m_pGameObj, false, &m_BasePosParams );

		if( m_p3DSound != NULL )
		{
			CAkAttenuation* pAttenuation = m_p3DSound->GetParams()->GetAttenuation();
			if( pAttenuation )
			{
				// Taking the value from the attenuation, if they are RTPC based, they will be overriden upon RTPC subscription, coming right after this.
				m_p3DSound->SetConeOutsideVolume( pAttenuation->m_ConeParams.fOutsideVolume );
				m_p3DSound->SetConeLPF( pAttenuation->m_ConeParams.LoPass );

				eResult = SubscribeAttenuationRTPC( pAttenuation );
				if( eResult != AK_Success )
					return eResult;

#ifndef AK_OPTIMIZED
				pAttenuation->AddPBI( this );
#endif
			}
			else
			{
				//we were expecting an attenuation but we couldn't get one
				if( m_p3DSound->GetParams()->m_uAttenuationID != AK_INVALID_UNIQUE_ID )
					return AK_Fail; //return an error (see WG-6760)
			}

			m_p3DSound->Lock();
			Gen3DParams * l_p3DParams = m_p3DSound->GetParams();

			if( m_p3DSound && (l_p3DParams->m_eType == Ak3DUserDef) && (l_p3DParams->m_pArrayPlaylist != NULL) )
			{
				// get ID
				AkUniqueID PathOwnerID = m_p3DSound->GetPathOwner();

				// got one ?
				if(in_pPathInfo->pPBPath != NULL)
				{
					// same owner ?
					if(in_pPathInfo->PathOwnerID == PathOwnerID)
					{
						// use this path
						m_PathInfo.pPBPath = in_pPathInfo->pPBPath;
						// keep the id
						m_PathInfo.PathOwnerID = in_pPathInfo->PathOwnerID;
					}
				}

				// already got one ?
				if(m_PathInfo.pPBPath == NULL)
				{
					// no, get a path from the manager
					m_PathInfo.pPBPath = g_pPathManager->AddPathToList();

					// if we've got one then proceed
					if( m_PathInfo.pPBPath != NULL)
					{
						// set m_pPath according to what's in the sound
						eResult = m_p3DSound->SetPathPlayList( m_PathInfo.pPBPath,m_pSound->GetPathState());

						if (eResult != AK_Success)
						{
							g_pPathManager->RemovePathFromList( m_PathInfo.pPBPath );

							m_PathInfo.pPBPath = NULL;
							PathOwnerID = AK_INVALID_UNIQUE_ID;
						}
						// keep the id
						m_PathInfo.PathOwnerID = PathOwnerID;
					}
				}
			}
			m_p3DSound->Unlock();
		}

		if( ( m_PathInfo.pPBPath != NULL ) )
		{
			AKRESULT tempResult = g_pPathManager->AddPathUser( m_PathInfo.pPBPath, this );
			if( tempResult == AK_Fail )
			{
				m_PathInfo.pPBPath = NULL;
			}
			else
			{
				m_PathInfo.pPBPath->SetSoundUniqueID( m_pSound->ID() );
				m_PathInfo.pPBPath->SetPlayingID( m_UserParams.PlayingID );
			}
		}
	m_pSource->LockDataPtr( (void*&)m_pDataPtr, m_uDataSize, this );
	}
	return eResult;
}

CAkPBI::~CAkPBI()
{

}

void CAkPBI::Term()
{
	AKASSERT(m_pSound);
	AKASSERT(g_pTransitionManager);
	AKASSERT(g_pPathManager);

	Lock();
	DecrementPlayCount();

#ifdef RVL_OS
	m_pGameObj->DecrementGameObjectPlayCount();
#endif

	UnsubscribeRTPC();
	DestroyFx();

	if(m_PathInfo.pPBPath != NULL)
	{
		// if continous then the path got rid of the played flags
		if( m_PathInfo.pPBPath->IsContinuous() )
		{
			AkPathState* pPathState = m_pSound->GetPathState();
			pPathState->pbPlayed = NULL;
			pPathState->ulCurrentListIndex = 0;
		}
		g_pPathManager->RemovePathUser(m_PathInfo.pPBPath,this);
		m_PathInfo.pPBPath = NULL;
		m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
	}

	if( m_PBTrans.pvPSTrans )
	{
		Monitor(AkMonitorData::NotificationReason_Fade_Aborted);
		g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPSTrans, this );
	}
	if( m_PBTrans.pvPRTrans )
	{
		Monitor(AkMonitorData::NotificationReason_Fade_Aborted);
		g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPRTrans, this );
	}

	if( m_bNeedNotifyEndReached )
	{
		Monitor(AkMonitorData::NotificationReason_EndReached);
		m_bNeedNotifyEndReached = false;
	}

	AKASSERT(g_pPlayingMgr);
	if(m_UserParams.PlayingID)
	{
		g_pPlayingMgr->Remove(m_UserParams.PlayingID,this);
	}

	if ( m_pGameObj )
	{
		m_pGameObj->Release();
	}

	if( m_p3DSound )
	{
#ifndef AK_OPTIMIZED
		CAkAttenuation* pAttenuation = m_p3DSound->GetParams()->GetAttenuation();
		if( pAttenuation )
		{
			pAttenuation->RemovePBI( this );
		}
#endif
		AkDelete( g_DefaultPoolId, m_p3DSound );
		m_p3DSound = NULL;
	}

	m_mapMutedNodes.Term();

	m_pSound->RemovePBI( this );

	// Must be done BEFORE releasing m_pSound, as the source ID is required to free the data.
	if ( m_pDataPtr )
	{
		m_pSource->UnLockDataPtr();
	}
	if( m_pUsageSlot )
	{
		m_pUsageSlot->Release( false );
		m_pUsageSlot = NULL;
	}

	m_pSound->Release();

	if (m_pFeedbackInfo != NULL)
	{
		m_pFeedbackInfo->Destroy();
		m_pFeedbackInfo = NULL;
	}

	Unlock();
}

AKRESULT CAkPBI::_InitPlay()
{
	Lock();
	// Generate the next loop count.
    AKASSERT( m_pSound != NULL );
	m_LoopCount = m_pSound->Loop();

	//Ensure this function is not called twice
	AKASSERT( !m_bInitPlayWasCalled );
	if(!m_bInitPlayWasCalled)
	{
		m_bInitPlayWasCalled = true; // Must be done BEFORE calling AkMonitorData

		if( m_PathInfo.pPBPath != NULL )
		{
			g_pPathManager->Start( m_PathInfo.pPBPath,m_pSound->GetPathState());
		}
	}
	Unlock();
	return AK_Success;
}

AKRESULT CAkPBI::_Play( TransParams & in_transParams, bool in_bPaused, bool in_bForceIgnoreSync )
{
	Lock();

    // Start transition if applicable.
    if( in_transParams.TransitionTime != 0 )
	{
		m_cPlayStopFade = MUTED_LVL;
		CreateTransition( true, 
					 	TransTarget_Play, 
						in_transParams, 
						false );
	}

	if( in_bPaused == true || m_eInitialState == PBI_InitState_Paused )
	{
		m_bWasPaused = true;
		CAkLEngine::EnqueueAction( LEStatePlayPause, this );
		
		if( m_PBTrans.pvPSTrans )
		{
			g_pTransitionManager->Pause( m_PBTrans.pvPSTrans );
		}

		PausePath(true);		
	}
	else
	{
		CAkLEngine::EnqueueAction( LEStatePlay, this );
	}

	if( m_eInitialState == PBI_InitState_Stopped )
	{
		_Stop();
	}

	Unlock();

	if( in_bForceIgnoreSync )
	{
		// especially useful for IM, avoid making IM playback pending one on each others.
		CAkLEngine::IncrementSyncCount();
	}

	return AK_Success;
}

AKRESULT CAkPBI::_Stop( AkPBIStopMode in_eStopMode /*= AkPBIStopMode_Normal*/, bool in_bIsFromTransition /*= false*/)
{
	AKRESULT l_eResult = AK_Success;
	Lock();

	if(!m_bWasStopped)
	{
		m_bWasStopped = true;

		// In the case of transition, it is the combiner node that checks the WasStopped flag
		// once the last transition buffer has been processed.
		if(!in_bIsFromTransition)
		{
			// Necessary in the case of play-stop, or stop-and-continue.
			CAkLEngine::EnqueueActionStop( this );
		}

		if( in_eStopMode == AkPBIStopMode_Normal || in_eStopMode == AkPBIStopMode_StopAndContinueSequel )
		{
			if( m_PBTrans.pvPSTrans )
			{
				Monitor(AkMonitorData::NotificationReason_Fade_Aborted);
				g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPSTrans, this );
				m_PBTrans.pvPSTrans = NULL;
			}
			if( m_PBTrans.pvPRTrans )
			{
				Monitor(AkMonitorData::NotificationReason_Fade_Aborted);
				g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPRTrans, this );
				m_PBTrans.pvPRTrans = NULL;
			}

			if(m_PathInfo.pPBPath != NULL)
			{
				// if continous then the path got rid of the played flags
				if(m_PathInfo.pPBPath->IsContinuous())
				{
					AkPathState* pPathState = m_pSound->GetPathState();
					pPathState->pbPlayed = NULL;
					pPathState->ulCurrentListIndex = 0;
				}
				g_pPathManager->RemovePathUser(m_PathInfo.pPBPath,this);
				m_PathInfo.pPBPath = NULL;
				m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
			}

			if( m_bWasPaused )
			{
				Monitor(AkMonitorData::NotificationReason_Pause_Aborted);
			}

			if ( m_bNeedNotifyEndReached )
			{
				if( m_bIsNotifyEndReachedContinuous || in_eStopMode == AkPBIStopMode_StopAndContinueSequel )
				{
					Monitor(AkMonitorData::NotificationReason_StoppedAndContinue);
				}
				else
				{
					Monitor( m_bWasKicked ? AkMonitorData::NotificationReason_StoppedLimit : AkMonitorData::NotificationReason_Stopped );
				}
			}

			m_bNeedNotifyEndReached = false;
			m_bTerminatedByStop = true;
		}
	}
	Unlock();
	return l_eResult;
}

AKRESULT CAkPBI::_Stop( TransParams & in_transParams )
{
	AKRESULT eResult = AK_Success;
	Lock();

	if( m_bWasPaused || ( m_PBTrans.pvPRTrans && m_PBTrans.bIsPRTransFading ) )
	{
		// If we are paused or going to be, we stop right away.
		eResult = _Stop();
	}
	else
	{
		m_bWasPreStopped = true;
		if( in_transParams.TransitionTime != 0 )
		{
			CreateTransition(true, TransTarget_Stop, in_transParams, true );			 
		}
		else if( m_State == CtxStateStop )//That mean we are not yet started, just let it be stopped without the glitch caussed by the minimal transition time
		{
			eResult = _Stop();
		}
		else
		{
			if( m_PBTrans.pvPSTrans )
			{
				// Force actual transition to use new duration of 0 ms
				TransitionTarget NewTarget;
				NewTarget.fValue = AK_MINIMUM_VOLUME_LEVEL;
				g_pTransitionManager->ChangeParameter(	static_cast<CAkTransition*>( m_PBTrans.pvPSTrans ), 
														TransTarget_Stop, 
														NewTarget,
														0,//no transition time
														AkValueMeaning_Default );
			}
			else
			{
				StopWithMinTransTime();
			}
		}
	}

	Unlock();
	return eResult;
}

// Prepare PBI for stopping with minimum transition time. Note: also used in PBI subclasses.
void CAkPBI::StopWithMinTransTime()
{
	//The feedback engine has its own fade out which fades the *speed* of the signal, not the signal itself
	if (!IsForFeedbackPipeline())
	{
		//Set volume to 0 and flag to generate ramp in LEngine.
		m_cPlayStopFade = MUTED_LVL;
		//Set m_RealEffectiveVolume and m_RealEffectiveLfe to min directly instead of calling ComputeEffectiveVolumes
		m_RealEffectiveVolume = AK_MINIMUM_VOLUME_LEVEL;
		m_RealEffectiveLfe = AK_MINIMUM_VOLUME_LEVEL;
	}
	_Stop( AkPBIStopMode_Normal, true );
}

#ifndef AK_OPTIMIZED
AKRESULT CAkPBI::_StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		)
{
	return _Stop();
}
#endif

AKRESULT CAkPBI::_Pause(bool in_bIsFromTransition /*= false*/)
{
	AKRESULT l_eResult = AK_Success;
	Lock();
	if(!m_bWasStopped && !m_bWasPaused)
	{
		m_bWasPaused = true;
		//The feedback engine has its own fade out which fades the *speed* of the signal, not the signal itself
		if (!IsForFeedbackPipeline())
		{
			m_cPauseResumeFade = MUTED_LVL;
			m_RealEffectiveVolume = AK_MINIMUM_VOLUME_LEVEL;
			m_RealEffectiveLfe = AK_MINIMUM_VOLUME_LEVEL;
		}
		AKASSERT(g_pTransitionManager);

		// In the case of transition, it is the combiner node that checks the WasStopped flag
		// once the last transition buffer has been processed.
		if(!in_bIsFromTransition)
		{
			CAkLEngine::EnqueueAction( LEStatePause, this );
		}
		if( m_PBTrans.pvPSTrans )
		{
			g_pTransitionManager->Pause( m_PBTrans.pvPSTrans );
		}

		PausePath(true);
	}
	Unlock();
	return l_eResult;
}

AKRESULT CAkPBI::_Pause( TransParams & in_transParams )
{
	AKRESULT eResult = AK_Success;
	Lock();
	++m_ulPauseCount;

	AKASSERT( m_ulPauseCount != 0 );//Just in case we were at -1 unsigned

	if( in_transParams.TransitionTime != 0 )
	{
		CreateTransition(false, TransTarget_Pause, in_transParams, true );
	}
	else if( m_State == CtxStateStop )
	{	// either we were stopped or not yet started, do not use minimal transition time
		eResult = _Pause();
	}
	else
	{	
		if( m_PBTrans.pvPRTrans )
		{
			// Force actual transition to use new duration of 0 ms
			TransitionTarget NewTarget;
			NewTarget.fValue = AK_MINIMUM_VOLUME_LEVEL;
			g_pTransitionManager->ChangeParameter(	static_cast<CAkTransition*>( m_PBTrans.pvPRTrans ), 
													TransTarget_Pause, 
													NewTarget,
													0,//no transition time
													AkValueMeaning_Default );
		}
		else
		{
			eResult = _Pause( true );
		}
	}
	Unlock();
	return eResult;
}

AKRESULT CAkPBI::_Resume()
{
	AKRESULT l_eResult = AK_Success;

	Lock();
	if(!m_bWasStopped)
	{
		if(m_bWasPaused == true)
		{
			PausePath(false);

			m_bWasPaused = false;
			AKASSERT(g_pTransitionManager);
			CAkLEngine::EnqueueAction( LEStateResume, this );

			if( m_PBTrans.pvPSTrans )
			{
				g_pTransitionManager->Resume( m_PBTrans.pvPSTrans );
			}
		}
	}
	Unlock();
	return l_eResult;
}

AKRESULT CAkPBI::_Resume( TransParams & in_transParams, bool in_bIsMasterResume )
{
	if( in_bIsMasterResume || m_ulPauseCount <= 1)
	{
		m_ulPauseCount = 0;

		_Resume();

		if( in_transParams.TransitionTime != 0 )
		{
			// Use given transition time
			CreateTransition( false, TransTarget_Resume, in_transParams, false );
		}
		else if( m_PBTrans.pvPRTrans )
		{
			// Force actual transition to use new duration of 0 ms
			TransitionTarget NewTarget;
			NewTarget.fValue = AK_MAXIMUM_VOLUME_LEVEL;
			g_pTransitionManager->ChangeParameter(	static_cast<CAkTransition*>( m_PBTrans.pvPRTrans ), 
													TransTarget_Resume, 
													NewTarget,
													0,//no transition time
													AkValueMeaning_Default );
		}
		else
		{
			// no transition created, using minimal transition time
			m_cPauseResumeFade = UNMUTED_LVL;
			CalculateMutedEffectiveVolume();
		}
	}
	else
	{
		--m_ulPauseCount;
	}

	return AK_Success; 
}

AKRESULT CAkPBI::PlayToEnd( CAkAudioNode * in_pNode )
{
	m_LoopCount = 1;

	CAkLEngine::EnqueueAction( LEStateStopLooping, this );

	return AK_Success;
}

void CAkPBI::ParamNotification( NotifParams& in_rParams )
{
	LockParams();
	switch( in_rParams.eType )
	{
	case NotifParamType_Volume:
		m_Volume += in_rParams.UnionType.volume;
		CalculateMutedEffectiveVolume();
		break;
	case NotifParamType_FeedbackBusPitch:
		if( !IsForFeedbackPipeline())
			break;
		//otherwwise do NotifParamType_Pitch so no break intentional.
	case NotifParamType_Pitch:
		m_EffectivePitch += in_rParams.UnionType.pitch;
		break;
	case NotifParamType_LFE:
		m_Lfe += in_rParams.UnionType.LFE;
		CalculateMutedEffectiveVolume();
		break;
	case NotifParamType_LPF:
		m_EffectiveLPF += in_rParams.UnionType.LPF;
		break;

	case NotifParamType_FeedbackVolume:
		if(m_pFeedbackInfo != NULL)
		{
			m_pFeedbackInfo->m_NewVolume += in_rParams.UnionType.FeedbackVolume;
		}
		break;
	case NotifParamType_FeedbackLPF:
		if(m_pFeedbackInfo != NULL)
		{
			m_pFeedbackInfo->m_LPF += in_rParams.UnionType.FeedbackLPF;
		}
		break;
	}
	UnlockParams();
}

void CAkPBI::MuteNotification(AkUInt8 in_cMuteLevel, AkMutedMapItem& in_rMutedItem, bool in_bPrioritizeGameObjectSpecificItems)
{
	LockParams();

	if ( in_bPrioritizeGameObjectSpecificItems )
	{
		// Mute notifications never apply to persistent mute items.
        AKASSERT( !in_rMutedItem.m_bIsPersistent );
        
		// Search the "opposite" entry for this identifier (i.e. if we're setting
		// a global entry, let's search for a non-global entry, and vice-versa)
		AkMutedMapItem searchItem;
        searchItem.m_Identifier = in_rMutedItem.m_Identifier;
		searchItem.m_bIsGlobal = ! in_rMutedItem.m_bIsGlobal;
		searchItem.m_bIsPersistent = false; 

		if ( m_mapMutedNodes.Exists( searchItem ) )
		{
			if ( in_rMutedItem.m_bIsGlobal )
			{
				// We already have a non-global entry for this
				// identifier. Since we were asked to prioritize
				// game object-specific entries, we will simply
				// ignore the new info.
				UnlockParams();
				return;
			}
			else
			{
				// We have a global entry for this identifier. Since
				// we were asked to prioritize game object-specific
				// entries, we must remove it and replace it with the
				// new, game object-specific entry (below).
				m_mapMutedNodes.Unset( searchItem );
			}
		}
	}

	// There's no point in keeping an unmuted entry, except if we're asked
	// to prioritize game object-specific items, in which case we must
	// keep it to make sure it doesn't get replaced by a global entry later.
	if( in_cMuteLevel == UNMUTED_LVL && ( ! in_bPrioritizeGameObjectSpecificItems || in_rMutedItem.m_bIsGlobal ) )
		m_mapMutedNodes.Unset( in_rMutedItem );
	else
		m_mapMutedNodes.Set( in_rMutedItem, in_cMuteLevel );

	CalculateMutedEffectiveVolume();
	UnlockParams();
}

// direct access to Mute Map. Applies only to persistent items.
AKRESULT CAkPBI::SetMuteMapEntry( 
    AkMutedMapItem & in_key,
    AkUInt8 in_uFadeRatio
    )
{
    AKASSERT( in_key.m_bIsPersistent );

    AKRESULT eResult;
    if ( in_uFadeRatio != UNMUTED_LVL )
        eResult = ( m_mapMutedNodes.Set( in_key, in_uFadeRatio ) ) ? AK_Success : AK_Fail;
    else
    {
        m_mapMutedNodes.Unset( in_key );
        eResult = AK_Success;
    }
    CalculateMutedEffectiveVolume();
    return eResult;
}

void CAkPBI::RemoveAllVolatileMuteItems()
{
    AkMutedMap::Iterator iter = m_mapMutedNodes.Begin();
    while ( iter != m_mapMutedNodes.End() )
	{
        if ( !((*iter).key.m_bIsPersistent) )
            iter = m_mapMutedNodes.EraseSwap( iter );
        else
            ++iter;
	}   
}

AkReal32 CAkPBI::Scale3DUserDefRTPCValue( AkReal32 in_fValue )
{
	if( m_p3DSound ) //do not scale 2D!
	{
		CAkAttenuation* pAttenuation = m_p3DSound->GetParams()->GetAttenuation();
		if( pAttenuation )
		{
			CAkAttenuation::AkAttenuationCurve* pVolumeDryCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
			if( pVolumeDryCurve )
				return in_fValue * pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize - 1].From / 100.0f;
		}
	}

	return in_fValue;
}

void CAkPBI::PositioningChangeNotification(
		AkReal32			in_RTPCValue,
		AkRTPC_ParameterID	in_ParameterID	// RTPC ParameterID, must be a Positioning ID.
		)
{
	switch ( in_ParameterID )
	{
		case POSID_Position_PAN_RL:
			m_BasePosParams.m_fPAN_RL = Scale3DUserDefRTPCValue( in_RTPCValue );
			return;
		case POSID_Position_PAN_FR:
			m_BasePosParams.m_fPAN_FR = Scale3DUserDefRTPCValue( in_RTPCValue );
			return;
		case POSID_Positioning_Divergence_Center_PCT:
			m_BasePosParams.m_fCenterPct = in_RTPCValue;
			return;
		case POSID_2DPannerEnabled:
			m_BasePosParams.bIsPannerEnabled = ( in_RTPCValue > 0 )? true : false;
			return;
	}
	if( m_p3DSound )
	{
		m_p3DSound->Lock();

		switch (in_ParameterID)
		{
		case POSID_PositioningType:
			m_p3DSound->SetPositioningType( (AkPositioningType)(AkInt)in_RTPCValue );
			break;
		case POSID_ConeInsideAngle:
			m_p3DSound->SetConeInsideAngle( in_RTPCValue );
			break;
		case POSID_ConeOutsideAngle:
			m_p3DSound->SetConeOutsideAngle( in_RTPCValue );
			break;
		case POSID_IsPositionDynamic:
			m_p3DSound->SetIsPositionDynamic( in_RTPCValue?true:false );
			break;
		case POSID_IsLooping:
			m_p3DSound->SetIsLooping( in_RTPCValue?true:false );
			break;
		case POSID_Transition:
			m_p3DSound->SetTransition( (AkTimeMs)in_RTPCValue );
			break;
		case POSID_PathMode:
			m_p3DSound->SetPathMode( (AkPathMode)(AkInt)in_RTPCValue );
			break;

		default:
			AKASSERT( !"Invalid or unknown Positioning parameter passed to the PBI" );
			break;
		}
		m_p3DSound->Unlock();
	}
}

void CAkPBI::CalculateMutedEffectiveVolume()
{
	AkReal32 l_fRatio = 1.0f;
	for( AkMutedMap::Iterator iter = m_mapMutedNodes.Begin(); iter != m_mapMutedNodes.End(); ++iter )
	{
		l_fRatio *= (*iter).item;
		l_fRatio /= UNMUTED_LVL;
	}
	if( m_cPlayStopFade != UNMUTED_LVL )
	{
		l_fRatio *= m_cPlayStopFade;
		l_fRatio /= UNMUTED_LVL;
	}
	if( m_cPauseResumeFade != UNMUTED_LVL )
	{
		l_fRatio *= m_cPauseResumeFade;
		l_fRatio /= UNMUTED_LVL;
	}
	AkVolumeValue l_Volume = AkMath::Min( m_Volume + m_Ranges.VolumeOffset, 0.0f );
	AkVolumeValue l_Lfe = AkMath::Min( m_Lfe + m_Ranges.LFEOffset, 0.0f );

	m_RealEffectiveVolume = ( ( l_Volume - AK_MINIMUM_VOLUME_LEVEL	) * l_fRatio) + AK_MINIMUM_VOLUME_LEVEL;
	m_RealEffectiveLfe =	( ( l_Lfe	 - AK_MINIMUM_VOLUME_LEVEL	) * l_fRatio) + AK_MINIMUM_VOLUME_LEVEL;
}

void CAkPBI::RecalcNotification()
{
	m_bAreParametersValid = false;
	m_bFeedbackParametersValid = false;	
}

void CAkPBI::NotifyBypass(
	AkUInt32 in_bitsFXBypass,
	AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if ( in_uTargetMask & ( 1 << uFXIndex ) )
			m_aFXInfo[ uFXIndex ].bIsBypassed = ( in_bitsFXBypass & ( 1 << uFXIndex ) )!=0;
	}

	if ( in_uTargetMask & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) )
		m_bBypassAllFX = ( in_bitsFXBypass & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) )!=0;
}

#ifndef AK_OPTIMIZED
void CAkPBI::InvalidatePaths()
{
	Lock();
	if( m_PathInfo.pPBPath != NULL )
	{
		// if continuous then the path got rid of the played flags
		if(m_PathInfo.pPBPath->IsContinuous())
		{
			AkPathState* pPathState = m_pSound->GetPathState();
			pPathState->pbPlayed = NULL;
			pPathState->ulCurrentListIndex = 0;
		}
		g_pPathManager->RemovePathUser( m_PathInfo.pPBPath, this );
		g_pPathManager->RemovePathFromList( m_PathInfo.pPBPath );
		m_PathInfo.pPBPath = NULL;
		m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
	}
	if( m_p3DSound )
	{
		m_p3DSound->InvalidatePaths();
	}
	Unlock();
}
#endif

void CAkPBI::TransUpdateValue(TransitionTargetTypes in_eTargetType,TransitionTarget in_unionValue,bool in_bIsTerminated)
{
	AKASSERT(g_pTransitionManager);	 
	switch(in_eTargetType & TransTarget_TargetMask)
	{
	case TransTarget_Stop:
	case TransTarget_Play:
		if(in_bIsTerminated)
		{
			m_PBTrans.pvPSTrans = NULL;
			Monitor(AkMonitorData::NotificationReason_Fade_Completed);

			if((in_eTargetType & TransTarget_TargetMask) == TransTarget_Stop)
			{
				_Stop( AkPBIStopMode_Normal, true );
			}
		}
		m_cPlayStopFade = static_cast<AkUInt8>(((in_unionValue.fValue - AK_MINIMUM_VOLUME_LEVEL) * 255.0f) / -(AK_MINIMUM_VOLUME_LEVEL));		
		break;
	case TransTarget_Pause:
	case TransTarget_Resume:
		if(in_bIsTerminated)
		{
			m_PBTrans.pvPRTrans = NULL;
			Monitor(AkMonitorData::NotificationReason_Fade_Completed);

			if((in_eTargetType & TransTarget_TargetMask) == TransTarget_Pause)
			{
				_Pause(true);
			}
		}
		m_cPauseResumeFade = static_cast<AkUInt8>(((in_unionValue.fValue - AK_MINIMUM_VOLUME_LEVEL) * 255.0f) / -(AK_MINIMUM_VOLUME_LEVEL));		 
		break;
	default:
		AKASSERT(!"Unsupported data type");
		break;
	}

	LockParams();
	CalculateMutedEffectiveVolume();
	UnlockParams();
}

AKRESULT CAkPBI::SetParam(
			AkPluginParamID in_paramID,
			void *			in_pParam,
			AkUInt32		in_uParamSize
			)
{
	if( m_p3DSound )
	{
		AKASSERT( in_uParamSize == sizeof( AkReal32 ) );
		AkReal32 l_newVal = *( (AkReal32*)in_pParam );

		m_p3DSound->Lock();

		switch ( in_paramID )
		{
		case POSID_Positioning_Cone_Attenuation:
			m_p3DSound->SetConeOutsideVolume( l_newVal );
			break;
		case POSID_Positioning_Cone_LPF:
			m_p3DSound->SetConeLPF( l_newVal );
			break;

		default:
			AKASSERT( !"Invalid or unknown Positioning parameter passed to the PBI" );
			break;
		}
		m_p3DSound->Unlock();
	}
	return AK_Success;
}

void CAkPBI::CreateTransition(bool in_bIsPlayStopTransition, TransitionTargets in_transitionTarget, TransParams in_transParams, bool in_bIsFadingTransition )
{
	AKASSERT(g_pTransitionManager);
	Lock();
	CAkTransition* prTransition = in_bIsPlayStopTransition?m_PBTrans.pvPSTrans:m_PBTrans.pvPRTrans;
	if(!prTransition)
	{
		AkUInt8 cActualStartPoint = in_bIsPlayStopTransition?m_cPlayStopFade:m_cPauseResumeFade;
		TransitionParameters Params;
		Params.pUser = this;
		Params.eTargetType = static_cast<TransitionTargetTypes>(in_transitionTarget | AkTypeFloat);
		Params.uStartValue.fValue = ((cActualStartPoint * -(AK_MINIMUM_VOLUME_LEVEL))/255.0f) + AK_MINIMUM_VOLUME_LEVEL;
		Params.uTargetValue.fValue = in_bIsFadingTransition?AK_MINIMUM_VOLUME_LEVEL:AK_MAXIMUM_VOLUME_LEVEL;
		Params.lDuration = in_transParams.TransitionTime;
		Params.eFadeCurve = in_transParams.eFadeCurve;
		Params.bdBs = true;
		// PhM : AddTransitionToList() will return NULL if none is available
		prTransition = g_pTransitionManager->AddTransitionToList(Params);

		if(in_bIsPlayStopTransition)
		{
			m_PBTrans.pvPSTrans = prTransition;
			m_PBTrans.bIsPSTransFading = in_bIsFadingTransition;
		}
		else
		{
			m_PBTrans.pvPRTrans = prTransition;
			m_PBTrans.bIsPRTransFading = in_bIsFadingTransition;
		}

		MonitorFade( AkMonitorData::NotificationReason_Fade_Started, in_transParams.TransitionTime );

		if( !prTransition )
		{
			// TODO : Should send a warning to tell the user that the transition is being skipped because 
			// the max num of transition was reached, or that the transition manager refused to do 
			// it for any other reason.

			// Forcing the end of transition right now.
			TransUpdateValue( Params.eTargetType, Params.uTargetValue, true );
		}
	}
	else
	{
		TransitionTarget NewTarget;
		NewTarget.fValue = in_bIsFadingTransition?AK_MINIMUM_VOLUME_LEVEL:AK_MAXIMUM_VOLUME_LEVEL;
		g_pTransitionManager->ChangeParameter(static_cast<CAkTransition*>(prTransition),in_transitionTarget,NewTarget,in_transParams.TransitionTime,AkValueMeaning_Default);
	}
	Unlock();
}

void CAkPBI::DecrementPlayCount()
{
	if( m_bWasPlayCountDecremented == false )
	{
		m_bWasPlayCountDecremented = true;
		m_pSound->DecrementPlayCount( m_pGameObj );
	}
}

bool CAkPBI::IsInstanceCountCompatible( AkUniqueID in_NodeIDToTest )
{
	bool bRet = false;
	AKASSERT( in_NodeIDToTest != AK_INVALID_UNIQUE_ID );
	if( m_pSound )
	{
		bRet = m_pSound->IsInstanceCountCompatible( in_NodeIDToTest );
	}
	return bRet;
}

// Returns true if the Context may jump to virtual voices, false otherwise.
AkBelowThresholdBehavior CAkPBI::GetVirtualBehavior( AkVirtualQueueBehavior& out_Behavior )
{
	AKASSERT( m_pSound );
	return m_pSound->GetVirtualBehavior( out_Behavior );
}

void CAkPBI::MonitorFade( AkMonitorData::NotificationReason in_Reason, AkTimeMs in_TransitionTime )
{
	AkUniqueID id = 0;
	if(m_pSound)
	{
		id = m_pSound->ID();
	}
	MONITOR_OBJECTNOTIF(m_UserParams.PlayingID, m_pGameObj->ID(), m_UserParams.CustomParam, in_Reason, m_CntrHistArray, id, in_TransitionTime );
}

#ifndef AK_OPTIMIZED
void CAkPBI::Monitor(AkMonitorData::NotificationReason in_Reason)
{
	AkUniqueID id = 0;
	if( m_pSound )
	{
		id = m_pSound->ID();
	}
	if( !m_bInitPlayWasCalled )
	{
		AKASSERT( !( in_Reason == AkMonitorData::NotificationReason_StoppedAndContinue ) );//Should not happen
		AKASSERT( !( in_Reason == AkMonitorData::NotificationReason_EndReachedAndContinue ) );//Should not happen

		//If the PBI was not initialized, it must be considered as a PlayFailed since it never started...
		if( in_Reason == AkMonitorData::NotificationReason_Stopped 
			|| in_Reason == AkMonitorData::NotificationReason_EndReached
			)
		{
			in_Reason = AkMonitorData::NotificationReason_ContinueAborted;
		}
		else if( in_Reason == AkMonitorData::NotificationReason_StoppedAndContinue
			|| in_Reason == AkMonitorData::NotificationReason_EndReachedAndContinue
			)
		{
			return;
		}
	}
	MONITOR_OBJECTNOTIF(m_UserParams.PlayingID, m_pGameObj->ID(), m_UserParams.CustomParam, in_Reason, m_CntrHistArray, id, 0 );
}
#endif
//====================================================================================================
// get the current play stop transition
//====================================================================================================
CAkTransition* CAkPBI::GetPlayStopTransition()
{
	return m_PBTrans.pvPSTrans;
}

//====================================================================================================
// get the current pause resume transition
//====================================================================================================
CAkTransition* CAkPBI::GetPauseResumeTransition()
{
	return m_PBTrans.pvPRTrans;
}

void CAkPBI::SetPauseStateForContinuous(bool in_bIsPaused)
{
	//empty function for CAkPBI, overriden by continuous PBI
}

void CAkPBI::SetEstimatedLength( AkTimeMs in_EstimatedLength )
{
	//nothing to do, implemented only for continuousPBI
}

void CAkPBI::PrepareSampleAccurateTransition()
{
	//nothing to do, implemented only for continuousPBI
}

//-----------------------------------------------------------------------------
// Name: Implementation of the IAkAudioCtx interface.
//-----------------------------------------------------------------------------
AKRESULT CAkPBI::Destroy( AkCtxDestroyReason in_eReason )
{
	CAkURenderer::EnqueueContextNotif( this, CtxStateToDestroy, in_eReason );

	return AK_Success;
}

AKRESULT CAkPBI::Play()
{
	m_State = CtxStatePlay;
	return CAkURenderer::EnqueueContextNotif( this, CtxStatePlay );
}

AKRESULT CAkPBI::Stop()
{
	m_State = CtxStateStop;

	//Commented out since unused
	//CAkURenderer::EnqueueContextNotif( this, CtxStateStop );

	return AK_Success;
}

AKRESULT CAkPBI::Pause()
{
	m_State = CtxStatePause;
	return CAkURenderer::EnqueueContextNotif( this, CtxStatePause );
}

AKRESULT CAkPBI::Resume()
{
	m_State = CtxStatePlay;
	return CAkURenderer::EnqueueContextNotif( this, CtxStateResume );
}

void CAkPBI::NotifAddedAsSA()
{
	m_State = CtxStatePlay;
}

void CAkPBI::ProcessContextNotif( AkCtxState in_eState, AkCtxDestroyReason in_eDestroyReason, AkTimeMs in_EstimatedLength )
{
	switch( in_eState )
	{
	case CtxStatePlay:
		Monitor(AkMonitorData::NotificationReason_Play);
		m_bNeedNotifyEndReached = true;
		PrepareSampleAccurateTransition();
		break;
	case CtxStateToDestroy:
		if( in_eDestroyReason == CtxDestroyReasonPlayFailed )
		{
			m_bNeedNotifyEndReached = false;
			m_bPlayFailed = true;
			Monitor(AkMonitorData::NotificationReason_PlayFailed);
		}
		break;
	case CtxStatePause:
		Monitor(AkMonitorData::NotificationReason_Paused);
		break;
	case CtxStateResume:
		Monitor(AkMonitorData::NotificationReason_Resumed);
		break;
	case CtxStateStop:
		//Here doing nothing, but should be processing Stop notification soon...
		break;
	case CtxSetEstimate:
		SetEstimatedLength( in_EstimatedLength );
		break;
	default:
		AKASSERT( !"Unexpected Context notification" );
		break;
	}
}

AKRESULT CAkPBI::GetSrcDescriptor( AkSrcDescriptor * out_pSrcDesc )
{
	AKASSERT( out_pSrcDesc != NULL );
	AkSrcTypeInfo * l_pType = m_pSource->GetSrcTypeInfo();

    AKASSERT( l_pType != NULL );

	out_pSrcDesc->Type	  = l_pType->mediaInfo.Type;
	out_pSrcDesc->uiID	  = l_pType->dwID;
	out_pSrcDesc->bIsLanguageSpecific = l_pType->mediaInfo.bIsLanguageSpecific;

	if( l_pType->mediaInfo.Type == SrcTypeModelled )
	{
		out_pSrcDesc->pvPath = m_SourceInfo.pParam;
		out_pSrcDesc->ulSize = sizeof( m_SourceInfo.pParam );
		out_pSrcDesc->ulFileID = AK_INVALID_FILE_ID;
	}
	else
	{
		out_pSrcDesc->pvPath = l_pType->pvSrcDesc;
		out_pSrcDesc->ulSize = l_pType->ulSrcDescSize;
		out_pSrcDesc->ulFileID = l_pType->mediaInfo.uFileID;
	}
	return AK_Success;
}

AkUniqueID CAkPBI::GetSoundID()
{
	return m_pSound->ID();
}

CAkBusCtx CAkPBI::GetBusContext()
{
	CAkBusCtx l_BusContext;
	if(m_bTargetIsFeedback)
	{
		l_BusContext.SetBus( m_pSound->GetFeedbackParentBusOrDefault() );
	}
	else
	{
		l_BusContext.SetBus( m_pSound->GetMixingBus() );
	}
	return l_BusContext;
}

AKRESULT CAkPBI::SetDuration( AkTimeMs in_Duration )
{
	CAkURenderer::EnqueueContextNotif( this, CtxSetEstimate, CtxDestroyReasonFinished, in_Duration );

	return AK_Success;
}

void CAkPBI::SetPluginMediaFormat( AkAudioFormat & in_rMediaFormat )
{
	AKASSERT( m_pSource != NULL );
	m_pSource->SetMediaFormat( in_rMediaFormat );
}

AKRESULT CAkPBI::RefreshParameters( AkSoundParams & io_SoundParams )
{
	LockParams();
	// Make sure we start with an empty map before calling
	// GetAudioParameters(), to avoid keeping obsolete entries.
    // Note: Only volatile map items are removed. Persistent items are never obsolete.
	RemoveAllVolatileMuteItems();

	bool bIsUsingRTPC = m_pSound->UpdateBaseParams( m_pGameObj, &m_BasePosParams );

#ifndef AK_OPTIMIZED
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if ( m_aFXInfo[ uFXIndex ].EffectTypeID != AK_INVALID_PLUGINID )
		{
			AkFXDesc l_TempFxDesc;
			m_pSound->GetFX( uFXIndex, l_TempFxDesc, m_pGameObj );
			m_aFXInfo[ uFXIndex ].bIsBypassed = l_TempFxDesc.bIsBypassed;
		}
	}
	m_bBypassAllFX = m_pSound->GetBypassAllFX( m_pGameObj );
#endif

	if( m_p3DSound )
	{
		m_p3DSound->SetIsPanningFromRTPC( bIsUsingRTPC, m_BasePosParams );
	}

	if (m_bTargetIsFeedback)
	{
		CAkFeedbackNode* pFeedbackNode = static_cast<CAkFeedbackNode*>(m_pSound);
		io_SoundParams.Volume = pFeedbackNode->GetSourceVolumeOffset(m_pSource);
		GetFeedbackParameters();
	}

	AKRESULT l_eResult = m_pSound->GetAudioParameters( io_SoundParams,
											  PT_All,
											  m_mapMutedNodes,
											  m_pGameObj,
											  !m_bGetAudioParamsCalled,
											  m_Ranges );
	m_bGetAudioParamsCalled = true;
	m_EffectivePitch		= io_SoundParams.Pitch + m_Ranges.PitchOffset;
	m_EffectiveLPF			= io_SoundParams.LPF + m_Ranges.LPFOffset;
	m_Lfe					= io_SoundParams.LFE;
	m_Volume				= io_SoundParams.Volume;
	CalculateMutedEffectiveVolume();

	if( IsForFeedbackPipeline() && m_pFeedbackInfo )
	{
		m_EffectivePitch += m_pFeedbackInfo->m_MotionBusPitch;
	}

	m_bAreParametersValid = true;

	UnlockParams();

	return l_eResult;
}

void CAkPBI::ClearParameters( AkSoundParams & io_SoundParams )
{
	io_SoundParams.LFE			= AK_MAXIMUM_VOLUME_LEVEL;
	io_SoundParams.Volume		= AK_MAXIMUM_VOLUME_LEVEL;
	io_SoundParams.Pitch		= AK_DEFAULT_PITCH;
	io_SoundParams.LPF			= AK_DEFAULT_LOPASS_VALUE;
}

AKRESULT CAkPBI::GetParams( AkSoundParams * io_Parameters )
{
	AKRESULT	  l_eResult	  = AK_Success;

	AKASSERT( m_pSound );

	if( !m_bAreParametersValid )
	{
		ClearParameters( *io_Parameters );

		l_eResult = RefreshParameters( *io_Parameters );
	}

	io_Parameters->Volume	= m_RealEffectiveVolume;
	io_Parameters->LFE		= m_RealEffectiveLfe;
	io_Parameters->Pitch	= m_EffectivePitch;
	io_Parameters->LPF		= m_EffectiveLPF;

	return l_eResult;
}

void CAkPBI::GetPositioningParams( AkSoundPositioningParams * io_Parameters )
{
	if( m_p3DSound == NULL )
	{
		io_Parameters->ePosType = Ak2DPositioning;
		return;
	}

	m_p3DSound->Lock();

	Gen3DParams * l_p3DParams = m_p3DSound->GetParams();
	AKASSERT( l_p3DParams != NULL );

	// pass these along
	io_Parameters->ePosType = l_p3DParams->m_eType;
	io_Parameters->fPAN_RL = m_BasePosParams.m_fPAN_RL;
	io_Parameters->fPAN_FR = m_BasePosParams.m_fPAN_FR;

	if(l_p3DParams->m_bIsConeEnabled)
		io_Parameters->Cone	= l_p3DParams->m_ConeParams;
	else
		io_Parameters->Cone	= g_OmniConeParams;

	io_Parameters->fDivergenceCenter = m_BasePosParams.m_fCenterPct;


	// figure out what's going on with the position
	switch(l_p3DParams->m_eType)
	{
	// is it RTPC'ed ?
	case Ak3DGameDef:
		{
			if( !l_p3DParams->m_bIsDynamic && m_b3DPositionDone )
			{
				//then take the one we backed up
				io_Parameters->uListenerMask = m_FirstPosition.uListenerMask;
				io_Parameters->Position = m_FirstPosition.pos;
			}
			else if ( m_pGameObj )
			{
				const AkSoundPositionEntry & posEntry = m_pGameObj->GetPosition();

				io_Parameters->uListenerMask = posEntry.uListenerMask;

				if ( posEntry.uListenerIdx != AK_INVALID_LISTENER_INDEX )
				{
					// sound is facing the listener
					const AkListenerData & Listener = CAkListener::GetListenerData( posEntry.uListenerIdx );

					_SetSoundPosToListener( Listener.Pos, io_Parameters->Position );
				}
				else
				{
					io_Parameters->Position = posEntry.pos;

					AkInt8 iDistanceOffset = 0;
					AkPriority priority = m_pSound->GetPriority( iDistanceOffset );
					if ( iDistanceOffset != 0 )
					{
						// Priority changes based on distance to listener

						AkReal32 fMinDistance = AK_UPPER_MAX_DISTANCE;

						unsigned int uMask = posEntry.uListenerMask;
						for ( unsigned int uListener = 0; uMask; ++uListener, uMask >>= 1 )
						{
							if ( !( uMask & 1 ) )
								continue; // listener not active for this sound

							if ( posEntry.uListenerIdx == uListener )
							{
								iDistanceOffset = 0;
								break; // no need to calculate anything else -- no distance, no offset
							}

							const AkListenerData & Listener = CAkListener::GetListenerData( uListener );
							
							AkReal32 fDistance = AkMath::Distance( Listener.Pos.Position, posEntry.pos.Position );

							fMinDistance = AkMath::Min( fMinDistance, fDistance ); // not entirely accurate, but all of this is fudge
						}

						//Get curve max radius
						CAkAttenuation* pAttenuation = l_p3DParams->GetAttenuation();
						CAkAttenuation::AkAttenuationCurve* pVolumeDryCurve = NULL;
						if( pAttenuation )
						{
							pVolumeDryCurve  = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
							if( pVolumeDryCurve )
							{
								AkReal32 fMaxDistance = pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize - 1].From;
								if ( fMinDistance < fMaxDistance && fMaxDistance > 0 ) 
									priority += (AkPriority) ( fMinDistance/fMaxDistance * iDistanceOffset );
								else
									priority += iDistanceOffset;
							}
						}
						priority = (AkPriority)AkMin( AK_MAX_PRIORITY, AkMax( AK_MIN_PRIORITY, priority ) );
					}
#ifdef RVL_OS
					if( m_Priority != priority )
					{
						m_Priority = priority;
						m_bDoPriority = true;
					}
#else
					m_Priority = priority;
#endif
				}

				if(!l_p3DParams->m_bIsDynamic)
				{
					//Back it up for next time
					m_FirstPosition.uListenerMask = io_Parameters->uListenerMask;
					m_FirstPosition.pos = io_Parameters->Position;
				}

				m_b3DPositionDone = true;
			}
			else
			{
				// should not happen
				io_Parameters->uListenerMask = AK_DEFAULT_LISTENER_MASK;
				io_Parameters->Position = g_DefaultSoundPosition;
			}
		}
		break;
	// is it path'ed
	case Ak3DUserDef:
		{
			Lock();//must lock this part since m_PathInfo.pPBPath can be flushed meanwhile
			if(m_PathInfo.pPBPath != NULL)
			{
				// this one might have been changed so pass it along
				bool bIsLooping = l_p3DParams->m_bIsLooping;
				m_PathInfo.pPBPath->SetIsLooping(bIsLooping);

				if(bIsLooping
					&& m_PathInfo.pPBPath->IsContinuous()
					&& m_PathInfo.pPBPath->IsIdle())
				{
					g_pPathManager->Start(m_PathInfo.pPBPath,m_pSound->GetPathState());
				}
			}
			Unlock();
			// pass the position
			if( l_p3DParams->m_bIsPanningFromRTPC )
			{
				l_p3DParams->m_Position.X = m_BasePosParams.m_fPAN_RL;
				l_p3DParams->m_Position.Y = AK_DEFAULT_SOUND_POSITION_Y;
				l_p3DParams->m_Position.Z = m_BasePosParams.m_fPAN_FR;
			}
			io_Parameters->uListenerMask = AK_DEFAULT_LISTENER_MASK;
			io_Parameters->Position.Position = l_p3DParams->m_Position;
			io_Parameters->Position.Orientation.X = AK_DEFAULT_SOUND_ORIENTATION_X;
			io_Parameters->Position.Orientation.Y = AK_DEFAULT_SOUND_ORIENTATION_Y;
			io_Parameters->Position.Orientation.Z = AK_DEFAULT_SOUND_ORIENTATION_Z;
		}
		break;

	default:
		AKASSERT( !"Should be a 3d sound here" );
		break;
	}

	m_p3DSound->Unlock();
}

AkPositioningType CAkPBI::GetPositioningType()
{
	AkPositioningType l_type = Ak2DPositioning;
	if( m_p3DSound )
	{
		l_type = m_p3DSound->GetParams()->m_eType;
	}
	return l_type;
}

AkVolumeValue CAkPBI::GetVolume()
{
	LockParams();
	if( !m_bAreParametersValid )
	{
		AkSoundParams l_Params;
		ClearParameters( l_Params );
		RefreshParameters( l_Params );
	}
	AkVolumeValue Return = m_RealEffectiveVolume;
	UnlockParams();

	return Return;
}

AkPitchValue CAkPBI::GetPitch()
{
	if( !m_bAreParametersValid )
	{
		AkSoundParams l_Params;
		ClearParameters( l_Params );
		RefreshParameters( l_Params );
	}

	return m_EffectivePitch;
}

void CAkPBI::GetEnvironmentValues( AkEnvironmentValue* AK_RESTRICT io_paEnvVal )
{
	AKASSERT( m_pGameObj != NULL );

	const AkEnvironmentValue * AK_RESTRICT pValues = m_pGameObj->GetEnvironmentValues();

	AkUInt32 cValues = 0;
	for( AkUInt32 i = 0; i < AK_MAX_ENVIRONMENTS_PER_OBJ; ++i )
	{
		AkEnvID eEnvID = pValues[i].EnvID;

		if( eEnvID == AK_INVALID_ENV_ID )
			break;

		if( g_pEnvironmentMgr->IsBypassed( eEnvID ) )
			continue;

		io_paEnvVal[cValues].EnvID = eEnvID;
		io_paEnvVal[cValues].fControlValue = pValues[i].fControlValue;
		// AkEnvironmentValue::fUserData is unimportant for client of CAkPBI

		++cValues;
	}

	if ( cValues < AK_MAX_ENVIRONMENTS_PER_OBJ )
		io_paEnvVal[cValues].EnvID = AK_INVALID_ENV_ID;
}

AKRESULT CAkPBI::CreateFx()
{
	AK::IAkPluginParam * l_pMaster	= NULL;
	AK::IAkPluginParam * l_pClone	= NULL;
	AKRESULT			 l_eResult	= AK_Fail;

	AKASSERT( m_pSound != NULL );

	// Check if there is a source effect.
	AkSrcTypeInfo * l_pSrcInfo = m_pSource->GetSrcTypeInfo();

	AKASSERT( l_pSrcInfo != NULL );
	if( l_pSrcInfo != NULL )
	{
		if( l_pSrcInfo->mediaInfo.Type == SrcTypeModelled )
		{			
			if( l_pSrcInfo->pvSrcDesc != NULL )
			{
				l_pMaster = reinterpret_cast<IAkPluginParam*>(l_pSrcInfo->pvSrcDesc);
				AKASSERT( l_pMaster != NULL );

				l_pClone  = l_pMaster->Clone( AkFXMemAlloc::GetUpper( ) );
				if( !l_pClone )
					return AK_Fail;

				if( l_pMaster != NULL )
				{
					m_SourceInfo.EffectTypeID	= l_pSrcInfo->dwID;
					m_SourceInfo.pParam			= l_pClone;
					l_eResult = CAkURenderer::AddFxParam( l_pMaster, l_pClone );
					AKASSERT( l_eResult == AK_Success );
				}
			}
			else
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_PluginProcessingFailed );
				return AK_Fail;
			}
		}
	}

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		AkFXDesc l_FXPStruct;
		l_eResult = m_pSound->GetFX( uFXIndex, l_FXPStruct, m_pGameObj );
		AKASSERT( l_eResult == AK_Success );

		// Check if there are other non-source effects.
		if( l_FXPStruct.EffectTypeID != AK_INVALID_PLUGINID )
		{
			// l_FXPStruct.pParam should never be null here, it may be null only on bus when it is an AK_PLUGINID_ENVIRONMENTAL
			AKASSERT( l_FXPStruct.pParam != NULL );
			l_pClone  = l_FXPStruct.pParam->Clone( AkFXMemAlloc::GetUpper() );
			if( l_FXPStruct.pParam != NULL && l_pClone != NULL )
			{
				AkFXDesc & fxInfo = m_aFXInfo[ uFXIndex ];

				fxInfo.FxReferenceID	= l_FXPStruct.FxReferenceID;
				fxInfo.EffectTypeID		= l_FXPStruct.EffectTypeID;
				fxInfo.pParam			= l_pClone;
				fxInfo.bIsBypassed		= l_FXPStruct.bIsBypassed;

				l_eResult = CAkURenderer::AddFxParam( l_FXPStruct.pParam, l_pClone );
				AKASSERT( l_eResult == AK_Success );
			}
		}
	}
	m_bBypassAllFX = m_pSound->GetBypassAllFX( m_pGameObj );

	return l_eResult;
} // CreateFx


void CAkPBI::DestroyFx()
{
	AKRESULT l_eResult;

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		AkFXDesc & fxInfo = m_aFXInfo[ uFXIndex ];
		AK::IAkPluginParam* pParam = fxInfo.pParam;

		// Remove all fx records.
		if( pParam )
		{
			l_eResult = CAkURenderer::RemoveFxParam( pParam );
			AKASSERT( l_eResult == AK_Success );

			l_eResult = pParam->Term( AkFXMemAlloc::GetUpper() );
			AKASSERT( l_eResult == AK_Success );

			fxInfo.EffectTypeID = AK_INVALID_PLUGINID;
			fxInfo.pParam = NULL;
		}
	}

	AK::IAkPluginParam* pParam =  m_SourceInfo.pParam;
	if( pParam )
	{
		l_eResult = CAkURenderer::RemoveFxParam( pParam );
		AKASSERT( l_eResult == AK_Success );

		l_eResult = pParam->Term( AkFXMemAlloc::GetUpper( ) );
		AKASSERT( l_eResult == AK_Success );

		m_SourceInfo.EffectTypeID = AK_INVALID_PLUGINID;
		m_SourceInfo.pParam = NULL;
	}
} // DestroyFx

AKRESULT CAkPBI::SubscribeFxRTPC( )
{
	AKRESULT l_eResult = AK_Success;

	AkRTPCFXSubscriptionList * l_plistFxRTPC = m_pSound->GetFxRTPCSubscriptionList();

	if( l_plistFxRTPC != NULL )
	{
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			AkFXDesc & fxInfo = m_aFXInfo[ uFXIndex ];
			if ( fxInfo.pParam ) 
			{
				for( AkRTPCFXSubscriptionList::Iterator iter = l_plistFxRTPC->Begin(); iter != l_plistFxRTPC->End(); ++iter )
				{
					AkRTPCFXSubscription& l_rFXRTPC = *iter;

					if( l_rFXRTPC.FXID == fxInfo.EffectTypeID )
					{
						l_eResult = g_pRTPCMgr->SubscribeRTPC( fxInfo.pParam,
															l_rFXRTPC.RTPCID,
															l_rFXRTPC.ParamID,
															l_rFXRTPC.RTPCCurveID,
															l_rFXRTPC.ConversionTable.m_eScaling,
															l_rFXRTPC.ConversionTable.m_pArrayGraphPoints,
															l_rFXRTPC.ConversionTable.m_ulArraySize,
															m_pGameObj,
															CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
															);
						AKASSERT( l_eResult == AK_Success );
					}
				}
			}
		}
	}

	l_plistFxRTPC = m_pSound->GetSourceRTPCSubscriptionList();

	if( l_plistFxRTPC != NULL )
	{
		for( AkRTPCFXSubscriptionList::Iterator iter = l_plistFxRTPC->Begin(); iter != l_plistFxRTPC->End(); ++iter )
		{
			AkRTPCFXSubscription& l_rFXRTPC = *iter;

			if( l_rFXRTPC.FXID == m_SourceInfo.EffectTypeID && m_SourceInfo.pParam)
			{
				l_eResult = g_pRTPCMgr->SubscribeRTPC( m_SourceInfo.pParam,
													l_rFXRTPC.RTPCID,
													l_rFXRTPC.ParamID,
													l_rFXRTPC.RTPCCurveID,
													l_rFXRTPC.ConversionTable.m_eScaling,
													l_rFXRTPC.ConversionTable.m_pArrayGraphPoints,
													l_rFXRTPC.ConversionTable.m_ulArraySize,
													m_pGameObj,
													CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
													);
				AKASSERT( l_eResult == AK_Success );
			}
		}
	}
	return l_eResult;

} // SubscribeFxRTPC

AKRESULT CAkPBI::SubscribeAttenuationRTPC( CAkAttenuation* in_pAttenuation )
{
	AKASSERT( in_pAttenuation );
	AKRESULT eResult = AK_Success;
	AkRTPCFXSubscriptionList* pRTPCList = in_pAttenuation->GetRTPCSubscriptionList();
	if( pRTPCList )
	{
		for( AkRTPCFXSubscriptionList::Iterator iter = pRTPCList->Begin(); iter != pRTPCList->End(); ++iter )
		{
			AkRTPCFXSubscription& l_rRTPC = *iter;

			eResult = g_pRTPCMgr->SubscribeRTPC( this,
												l_rRTPC.RTPCID,
												l_rRTPC.ParamID,
												l_rRTPC.RTPCCurveID,
												l_rRTPC.ConversionTable.m_eScaling,
												l_rRTPC.ConversionTable.m_pArrayGraphPoints,
												l_rRTPC.ConversionTable.m_ulArraySize,
												m_pGameObj,
												CAkRTPCMgr::SubscriberType_PBI
												);
			if( eResult != AK_Success )
				break;
		}
	}
	return eResult;
}

void CAkPBI::UnsubscribeRTPC()
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		AkFXDesc & fxInfo = m_aFXInfo[ uFXIndex ];
		if ( fxInfo.pParam )
			g_pRTPCMgr->UnSubscribeRTPC( fxInfo.pParam );
	}

	if( m_SourceInfo.pParam != NULL )
	{
		g_pRTPCMgr->UnSubscribeRTPC( m_SourceInfo.pParam );
	}

	//For attenuations, try avoiding clling this uselessly if possible.

	if( m_p3DSound )
	{
		g_pRTPCMgr->UnSubscribeRTPC( this );
	}

} // UnsubscribeRTPC

AKRESULT CAkPBI::UpdateRTPC( AkRTPCFXSubscription& in_rSubsItem )
{
	AKRESULT	l_eResult = AK_Success;

	IAkRTPCSubscriber* l_pSubs = NULL;

	if ( in_rSubsItem.FXID == m_SourceInfo.EffectTypeID )
	{
		l_pSubs = m_SourceInfo.pParam;
	}
	else for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		AkFXDesc & fxInfo = m_aFXInfo[ uFXIndex ];
		if ( in_rSubsItem.FXID == fxInfo.EffectTypeID )
		{
			l_pSubs = fxInfo.pParam;
			break;
		}
	}

	if( l_pSubs )
	{
		l_eResult = g_pRTPCMgr->SubscribeRTPC( l_pSubs,
											in_rSubsItem.RTPCID,
											in_rSubsItem.ParamID,
											in_rSubsItem.RTPCCurveID,
											in_rSubsItem.ConversionTable.m_eScaling,
											in_rSubsItem.ConversionTable.m_pArrayGraphPoints,
											in_rSubsItem.ConversionTable.m_ulArraySize,
											m_pGameObj,
											CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
											);
	}

	return l_eResult;
}

AKRESULT CAkPBI::NotifUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	AKRESULT l_eResult = AK_Success;

	// Remove all fx's param from the RTPC manager.
	if( m_SourceInfo.EffectTypeID == in_FXID )
	{
		l_eResult = g_pRTPCMgr->UnSubscribeRTPC( m_SourceInfo.pParam, in_ParamID, in_RTPCCurveID );
		AKASSERT( l_eResult == AK_Success );
	}
	else for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		AkFXDesc & fxInfo = m_aFXInfo[ uFXIndex ];
		if ( in_FXID == fxInfo.EffectTypeID )
		{
			l_eResult = g_pRTPCMgr->UnSubscribeRTPC( fxInfo.pParam, in_ParamID, in_RTPCCurveID );
			AKASSERT( l_eResult == AK_Success );
			break;
		}
	}

	return l_eResult;
}

AKRESULT CAkPBI::Lock()
{
	return m_csLock.Lock();
}

AKRESULT CAkPBI::Unlock()
{
	return m_csLock.Unlock();
}

void CAkPBI::LockParams()
{
#ifndef AK_OPTIMIZED
	m_csLockParams.Lock();
#endif
}

void CAkPBI::UnlockParams()
{
#ifndef AK_OPTIMIZED
	m_csLockParams.Unlock();
#endif
}

void CAkPBI::PausePath(bool in_bPause)
{
	if(m_PathInfo.pPBPath != NULL)
	{
		if(in_bPause)
		{
			g_pPathManager->Pause(m_PathInfo.pPBPath);
		}
		else
		{
			g_pPathManager->Resume(m_PathInfo.pPBPath);
		}
	}
}

AKRESULT CAkPBI::Kick()
{
	m_bWasKicked = true;
    TransParams transParams;
    transParams.TransitionTime = 0;
    transParams.eFadeCurve = AkCurveInterpolation_Linear;
	return _Stop( transParams );
}

AkUInt32 CAkPBI::GetSourceOffset()
{
	return 0; 
}

AkUInt32 CAkPBI::GetAndClearStopOffset()
{ 
	//Not zero since 0 is an acceptable stop
	return AK_NO_IN_BUFFER_STOP_REQUESTED; 
}

#ifndef AK_OPTIMIZED

void CAkPBI::UpdateFxParam(
	AkPluginID		in_FXID,
	AkUInt32	   	in_uFXIndex,
	AkPluginParamID	in_ulParamID,
	void*			in_pvParamsBlock,
	AkUInt32		in_ulParamBlockSize
	)
{
	AkFXDesc & fxInfo = m_aFXInfo[ in_uFXIndex ];

	if( fxInfo.EffectTypeID == in_FXID )
	{
		fxInfo.pParam->SetParam( in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
	}
	else if( m_SourceInfo.EffectTypeID == in_FXID )
	{
		m_SourceInfo.pParam->SetParam( in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
	}
}

void CAkPBI::UpdateAttenuationInfo()
{
	AKASSERT( m_p3DSound );
	
	CAkAttenuation* pAttenuation = m_p3DSound->GetParams()->GetAttenuation();
	if( pAttenuation )
	{
		// Taking the value from the attenuation, if they are RTPC based, they will be overriden upon RTPC subscription, coming right after this.
		m_p3DSound->SetConeOutsideVolume( pAttenuation->m_ConeParams.fOutsideVolume );
		m_p3DSound->SetConeLPF( pAttenuation->m_ConeParams.LoPass );

		g_pRTPCMgr->UnSubscribeRTPC( this );

		SubscribeAttenuationRTPC( pAttenuation );
	}
}

#endif //AK_OPTIMIZED

// Feedback devices support
AkFeedbackParams* CAkPBI::GetFeedbackParameters()
{
	AKASSERT(m_pSound);

	CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
	if(pMgr == NULL || !pMgr->IsFeedbackEnabled() )
		return NULL;

	LockParams();

	if ( !m_bFeedbackParametersValid )
	{
		//If there is at least one, allocate the structure
		if ( m_pFeedbackInfo == NULL )
		{
			if( m_pSound->FeedbackParentBus() != NULL || IsForFeedbackPipeline() )
			{
				//The structure is of variable size.  Depends on the number of channels of the input
				//and the number of connected players.
				m_pFeedbackInfo = AkFeedbackParams::Create(pMgr->GetPlayerCount(), (AkUInt16)GetMediaFormat()->GetNumChannels(), GetPositioningType());
			}
		}

		if (m_pFeedbackInfo != NULL)
		{
			m_pSound->GetFeedbackParameters(*m_pFeedbackInfo, m_pSource, m_pGameObj, true);
		}

		m_bFeedbackParametersValid = true;
	}

	UnlockParams();

	return m_pFeedbackInfo;
}
