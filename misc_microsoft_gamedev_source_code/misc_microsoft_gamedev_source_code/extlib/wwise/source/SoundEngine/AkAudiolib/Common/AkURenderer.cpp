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
// AkURenderer.cpp
//
// Microsoft Windows (DirectSound) Implementation of the Audio renderer
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMath.h"
#include "AkURenderer.h"
#include "AkTransitionManager.h"
#include "AkContinuousPBI.h"
#include "AkRTPCMgr.h"
#include "AkLEngine.h"
#include "AkRandom.h"
#include "AkRegisteredObj.h"
#include "AkRegistryMgr.h"
#include "AkSoundBase.h"
#include "AkSource.h"
#include "Ak3DListener.h"
#include "Ak3DParams.h"
#include "AkProfile.h"
#include "AkAudiolibTimer.h"
#include "AkEnvironmentsMgr.h"
#include "AkMonitor.h"
#include "AkAudioMgr.h"
#include "AkAudiolib.h"
#include "AkCritical.h"

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
extern CAkTransitionManager* g_pTransitionManager;
extern AkMemPoolId g_DefaultPoolId;

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------
// List sizes.
#define MIN_NUM_CTX					64

#define MIN_NUM_PLAY_EVENT			16
#define MIN_NUM_RENDER_EVENT		64 // must not be zero

#define UENGINE_DEFAULT_POOL_ID		g_DefaultPoolId

//-----------------------------------------------------------------------------
//Static variables.
//-----------------------------------------------------------------------------
CAkURenderer::AkListCtxs		CAkURenderer::m_listCtxs;	 
CAkURenderer::AkContextNotifQueue CAkURenderer::m_CtxNotifQueue;
CAkURenderer::AkListFxParams	CAkURenderer::m_listFxParam;
CAkLock							CAkURenderer::m_Lock;

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialise the object.
//
// Parameters:
//	AkPlatformInitSettings * io_pPDSettings : Initialisation parameters.
//
// Return: 
//	Ak_Success:          Object was initialised correctly.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failed to initialise the object correctly.
//-----------------------------------------------------------------------------
void CAkURenderer::ApplyGlobalSettings( AkPlatformInitSettings *   io_pPDSettings )
{
	CAkLEngine::ApplyGlobalSettings( io_pPDSettings );
}
//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialise the object.
//
// Return: 
//	Ak_Success:          Object was initialised correctly.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failed to initialise the object correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Init()
{
	AKRESULT l_eResult = m_listCtxs.Init();
	if( l_eResult != AK_Success ) return l_eResult;
 	
	l_eResult = m_listFxParam.Init( MIN_NUM_RENDER_EVENT, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId );
	if( l_eResult != AK_Success ) return l_eResult;

	l_eResult = m_CtxNotifQueue.Init( MIN_NUM_RENDER_EVENT, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId );
	if( l_eResult != AK_Success ) return l_eResult;

	// Initialize the Lower Audio Engine. 
	l_eResult = CAkLEngine::Init();

    return l_eResult;
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate the object.
//
// Parameters:
//	None.
//
// Return: 
//	Ak_Success: Object was terminated correctly.
//  AK_Fail:    Failed to terminate correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Term()
{
	AKRESULT l_eResult = AK_Success;

	Lock();

	CAkLEngine::Term();

	l_eResult = DestroyAllPBIs();

	m_listCtxs.Term();

	m_CtxNotifQueue.Term();

	m_listFxParam.Term();

	Unlock();
    return AK_Success;
} // Term

AkPriority CAkURenderer::_CalcInitialPriority( CAkSoundBase * in_pSound, CAkRegisteredObj * in_pGameObj )
{
	AkPriority iDistanceOffset = 0;
	AkReal32 fMaxRadius = 0.0f;
	AkPriority priority = in_pSound->GetPriority( iDistanceOffset );
	if ( iDistanceOffset && in_pSound->GetMaxRadius( fMaxRadius ) )
	{
		AkSoundPositionEntry posObj = in_pGameObj->GetPosition();

		AkReal32 fMinDistance = AK_UPPER_MAX_DISTANCE;

		unsigned int uMask = posObj.uListenerMask;
		for ( unsigned int uListener = 0; uMask; ++uListener, uMask >>= 1 )
		{
			if ( !( uMask & 1 ) )
				continue; // listener not active for this sound

			if ( posObj.uListenerIdx == uListener )
				return priority; // no need to calculate anything else -- no distance, no offset

			const AkListenerData & Listener = CAkListener::GetListenerData( uListener );
			
			AkReal32 fDistance = AkMath::Distance( Listener.Pos.Position, posObj.pos.Position );

			fMinDistance = AkMin( fMinDistance, fDistance ); // not entirely accurate, but all of this is fudge
		}
		
		if ( fMinDistance < fMaxRadius )
			priority += (AkPriority) ( fMinDistance/fMaxRadius * iDistanceOffset );
		else
			priority += iDistanceOffset;

		priority = (AkPriority)AkMin( AK_MAX_PRIORITY, AkMax( AK_MIN_PRIORITY, priority ) );
	}

	return priority;
}

AKRESULT CAkURenderer::Play( CAkSoundBase*  in_pSound,
							 CAkSource*		in_pSource,
                             AkPBIParams&   in_rPBIParams )
{
	AKRESULT eResult = AK_Fail;

    // Check parameters.
    AKASSERT( in_pSound != NULL );

    AkPriority priority = _CalcInitialPriority( in_pSound, in_rPBIParams.pGameObj );

	if ( in_pSound->IncrementPlayCount( priority, in_rPBIParams.pGameObj ) )
	{
        CAkPBI * l_pContext = in_rPBIParams.pInstigator->CreatePBI( in_pSound, in_pSource, in_rPBIParams, priority );

		if( l_pContext != NULL )
		{
            bool bInitSucceed = false;

            if ( in_rPBIParams.eType == AkPBIParams::PBI )
            {
                AkPathInfo pathInfo = { NULL, AK_INVALID_UNIQUE_ID };

                bInitSucceed = l_pContext->Init( &pathInfo ) == AK_Success;
            }
            else
            {
                bInitSucceed = l_pContext->Init( in_rPBIParams.pContinuousParams->pPathInfo ) == AK_Success;
            }

			if( bInitSucceed )
            {
                l_pContext->SetFrameOffset( in_rPBIParams.uFrameOffset );
                return Play( l_pContext, *in_rPBIParams.pTransitionParameters, in_rPBIParams.ePlaybackState );
            }

			l_pContext->FlagAsPlayFailed();// 2008.2.1 patch hack, a better fix was incorporated in the main branch.

        	l_pContext->Term(); // does call DecrementPlayCount()
            AkDelete( RENDERER_DEFAULT_POOL_ID, l_pContext );
            l_pContext = NULL;

		}
		else
		{
			in_pSound->DecrementPlayCount( in_rPBIParams.pGameObj );
		}

		// Failure

		in_pSound->MonitorNotif( in_rPBIParams.bIsFirst ? AkMonitorData::NotificationReason_PlayFailed : AkMonitorData::NotificationReason_ContinueAborted,
			in_rPBIParams.pGameObj->ID(),
			in_rPBIParams.userParams,
			in_rPBIParams.playHistory );

		MONITOR_ERROR( AK::Monitor::ErrorCode_PlayFailed );

	}
	else
	{
		eResult = AK_PartialSuccess;

		in_pSound->MonitorNotif( in_rPBIParams.bIsFirst ? AkMonitorData::NotificationReason_PlayFailedLimit : AkMonitorData::NotificationReason_ContinueAbortedLimit,
			in_rPBIParams.pGameObj->ID(),
			in_rPBIParams.userParams,
			in_rPBIParams.playHistory );
		
		in_pSound->DecrementPlayCount( in_rPBIParams.pGameObj );
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: Play
// Desc: Play a specified sound.
//
// Parameters:
//	CAkPBI * in_pContext				: Pointer to context to play.
//	AkPlaybackState in_ePlaybackState	: Play may be paused.
//
// Return: 
//	Ak_Success:          Sound is scheduled to be played.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failed to play sound.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Play( CAkPBI *		 in_pContext, 
                             TransParams&    in_rTparameters,
							 AkPlaybackState in_ePlaybackState
						    )
{
    // Check parameters.
    AKASSERT( in_pContext != NULL );
    if( in_pContext == NULL )
        return AK_InvalidParameter;

	AKRESULT l_eResult = AK_Success;

	// Add PBI context to list.
	Lock();
	m_listCtxs.AddLast( in_pContext );
	Unlock();

	l_eResult = in_pContext->_InitPlay();
	AKASSERT( l_eResult == AK_Success );
	
	bool l_bPaused = false;
	// Check if the play command is actually a play-pause.
	if( in_ePlaybackState == PB_Paused )
	{
		l_bPaused = true;
	}

	l_eResult = in_pContext->_Play( in_rTparameters, l_bPaused );
	AKASSERT( l_eResult == AK_Success );

	return l_eResult;
} // Play

//-----------------------------------------------------------------------------
// Name: Stop
// Desc: Stop a specified sound.
//
// Parameters:  
//	AkUInt8		in_ucCommand			: Stop, Pause, Resume.
//	CAkSoundBase* in_pSound				: Pointer to sound object to play.
//	bool		in_bIsObjectSpecific	: True = Game object active, false = no object.
//	AkTimeMs	in_lTransitionTime		: Length of transition in mSec.
//	AkCurveInterpolation	in_eFadeCurve			: Type of curve.
//
// Return: 
//	Ak_Success:          Sound was scheduled to be stopped.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failure.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Stop( CAkSoundBase*		in_pSound,
						     CAkRegisteredObj*	in_pGameObj,
							 TransParams&		in_rTparameters,
							 AkPlayingID		in_PlayingID /* = AK_INVALID_PLAYING_ID */)
{
	return	ProcessCommand( UEStateStop,
							in_pSound,
							in_pGameObj,
							in_PlayingID,
                            in_rTparameters,
							false );
} // Stop

//-----------------------------------------------------------------------------
// Name: Pause
// Desc: Pause a specified sound.
//
// Parameters:  
//	AkUInt8		in_ucCommand			: Stop, Pause, Resume.
//	CAkSoundBase* in_pSound			: Pointer to sound object to play.
//	bool		in_bIsObjectSpecific	: True = Game object active, false = no object.
//	AkTimeMs	in_lTransitionTime		: Length of transition in mSec.
//	AkCurveInterpolation	in_eFadeCurve			: Type of curve.
//
// Return: 
//	Ak_Success:          Sound was paused.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failure.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Pause( CAkSoundBase *	 in_pSound,
						      CAkRegisteredObj *	 in_pGameObj,
							  TransParams&    in_rTparameters,
							  AkPlayingID	in_PlayingID )
{
	return ProcessCommand( UEStatePause,
							in_pSound,
							in_pGameObj,
							in_PlayingID,
                            in_rTparameters,
							false );
} // Pause

//-----------------------------------------------------------------------------
// Name: Resume
// Desc: Resume a specified sound.
//
// Parameters:
//	AkUInt8		in_ucCommand			: Stop, Pause, Resume.
//	CAkSoundBase*  in_pSound			: Pointer to sound object to play.
//	bool		in_bIsObjectSpecific	: True = Game object active, false = no object.
//	AkTimeMs	in_lTransitionTime		: Length of transition in mSec.
//	AkCurveInterpolation	in_eFadeCurve			: Type of curve.
//
// Return:
//	Ak_Success:          Sound was resumed.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failure.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Resume( CAkSoundBase *	in_pSound,
							   CAkRegisteredObj *	in_pGameObj,
							   TransParams& in_rTparameters,
							   bool		in_bIsMasterResume,
							   AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
{
	return ProcessCommand( UEStateResume,
							in_pSound,
							in_pGameObj,
							in_PlayingID,
                            in_rTparameters,
							in_bIsMasterResume);
} // Resume

//-----------------------------------------------------------------------------
// Name: ProcessCommand
// Desc: Process commands for playing sounds (stop, pause, resume).
//
// Parameters:  
//	UEState		in_eCommand   			: Stop, Pause, Resume.
//	CAkSoundBase * in_pSound			: Pointer to sound object to play.
//	bool		in_bIsObjectSpecific	: True = Game object active, false = no object.
//	AkTimeMs	in_lTransitionTime		: Length of transition in mSec.
//	AkCurveInterpolation	in_eFadeCurve			: Type of curve.
//
// Return: 
//	Ak_Success:          Sound was scheduled to be stopped.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             in_pSound Sound was not found.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::ProcessCommand( UEState		in_eCommand,
									   CAkSoundBase *	in_pSound,
									   CAkRegisteredObj * in_pGameObj,
									   AkPlayingID in_PlayingID,
									   TransParams& in_rTparameters,
									   bool		in_bIsMasterResume )
{
	CAkPBI*	l_pPBI	= NULL;

	Lock();

	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
    {
		l_pPBI = *iter;
		AKASSERT( l_pPBI != NULL );

		if( l_pPBI->GetSound() == in_pSound )
		{
			if( ( !in_pGameObj || l_pPBI->GetGameObjectPtr() == in_pGameObj ) &&
				( in_PlayingID == AK_INVALID_PLAYING_ID || l_pPBI->GetPlayingID() == in_PlayingID ) )
			{
				switch( in_eCommand )
				{
				case UEStatePlay :
					AKASSERT( !"Play event not performed here." );
					break;

				case UEStateStop :
					l_pPBI->_Stop( in_rTparameters );
					break;

				case UEStatePause :
					l_pPBI->_Pause( in_rTparameters );
					break;

				case UEStateResume :
					l_pPBI->_Resume( in_rTparameters, in_bIsMasterResume );
					break;

				default:
					AKASSERT(!"ERROR: Command not defined.");
					break;
				}
			}
		}
	} // End if.

	Unlock();
	return AK_Success;
} // ProcessCommand


//-----------------------------------------------------------------------------
// Name: EnqueueContext
// Desc: Enqueues a PBI that was created elsewhere (the Upper Renderer creates
//       all standalone PBIs, but this is used for linked contexts (Music Renderer)
//
// Parameters:
//	PBI*
//
// Return:
//	AKRESULT
//-----------------------------------------------------------------------------
void CAkURenderer::EnqueueContext( CAkPBI * in_pContext )
{
    AKASSERT( in_pContext );
    AkAutoLock<CAkLock> gate( m_Lock );
    m_listCtxs.AddLast( in_pContext );
}


//-----------------------------------------------------------------------------
// Name: KickOutOldest
// Desc: Asks to kick out the Oldest sound responding to the given IDs.
//
// Parameters:
//	CAkRegisteredObj *	in_pGameObj : GameObject that must match before kicking out
//	AkUniqueID		in_NodeID  : Node to check if the sound comes from ( Excluding OverrideParent Exceptions )
//
// Return:
//	void
//-----------------------------------------------------------------------------
bool CAkURenderer::Kick( AkPriority in_Priority, CAkRegisteredObj * in_pGameObj, AkUniqueID in_NodeID, bool in_bKickNewest, CAkAudioNode*& out_pKicked )
{
	Lock();

	CAkPBI * pWeakest = NULL;

	AkPriority priorityWeakest = 101;//Max priority(100) + 1

	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
	{
		CAkPBI * l_pPBI = *iter;
		AKASSERT( l_pPBI != NULL );

		if( in_pGameObj == NULL || l_pPBI->GetGameObjectPtr() == in_pGameObj )
		{
			AkPriority priority = l_pPBI->GetPriority();
			if( priority < priorityWeakest && l_pPBI->IsInstanceCountCompatible( in_NodeID ) && !( l_pPBI->WasKicked() ) )
			{
				pWeakest = l_pPBI;
				priorityWeakest = priority;
				if( in_bKickNewest )
					priorityWeakest += 1;
			}
		}
	}

	if( in_Priority < priorityWeakest )
	{
		pWeakest = NULL;
	}
	else if ( pWeakest )
	{
		out_pKicked = pWeakest->GetSound();
		pWeakest->Kick();
	}

	Unlock();

	return pWeakest != NULL;
}//KickOutOldest

//-----------------------------------------------------------------------------
// Name: EnqueueContextNotif
// Desc: 
//
// Return: AKRESULT -- AK_Success if success.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::EnqueueContextNotif( CAkPBI* in_pPBI, AkCtxState in_eState, AkCtxDestroyReason in_eDestroyReason, AkTimeMs in_EstimatedTime /*= 0*/ )
{
	ContextNotif* pCtxNotif = m_CtxNotifQueue.AddLast();

	if( !pCtxNotif )
	{
		// Not enough memory to send the notification. 
		// But we cannot simply drop it since it may cause game inconsistency and leaks.
		// So we will do right now the job we are supposed to be doing after the perform of the lower engine.
		// It is gonna cost few locks since we must grab the g_csMain lock and the upper renderer lock.
		g_csMain.Lock();
		PerformContextNotif();
		g_csMain.Unlock();

		// once context notifs have been performed, the m_CtxNotifQueue queue will be empty, so the nest addlast will succeed.
		pCtxNotif = m_CtxNotifQueue.AddLast();

		AKASSERT( pCtxNotif && MIN_NUM_RENDER_EVENT );// Assert, because if it fails that means the minimal size of m_CtxNotifQueue of 64
		// MIN_NUM_RENDER_EVENT just cannot be 0.
	}

	if( pCtxNotif )
	{
		pCtxNotif->pPBI = in_pPBI;
		pCtxNotif->state = in_eState;
		pCtxNotif->DestroyReason = in_eDestroyReason;
		pCtxNotif->EstimatedLength = in_EstimatedTime;
		return AK_Success;
	}
	else
	{
		return AK_Fail;
	}
}

void CAkURenderer::PerformContextNotif()
{
	Lock();
	while( !m_CtxNotifQueue.IsEmpty() )
	{
		ContextNotif& pCtxNotif = m_CtxNotifQueue.First();
		pCtxNotif.pPBI->ProcessContextNotif( pCtxNotif.state, pCtxNotif.DestroyReason, pCtxNotif.EstimatedLength );
		if( pCtxNotif.state == CtxStateToDestroy )
		{
			m_listCtxs.Remove( pCtxNotif.pPBI );
			DestroyPBI( pCtxNotif.pPBI );
		}
		m_CtxNotifQueue.RemoveFirst();
	}
	Unlock();
}

//-----------------------------------------------------------------------------
// Name: DestroyPBI
// Desc: Destroy a specified PBI.
//
// Parameters:
//	CAkPBI * in_pPBI  : Pointer to a PBI to destroy.
//
// Return:
//	Ak_Success:          PBI was destroyed.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failed to destroyed PBI.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::DestroyPBI( CAkPBI * in_pPBI )
{
	// Check parameters.
	AKASSERT( in_pPBI != NULL );
	if( in_pPBI == NULL )
		return AK_InvalidParameter;

	AKRESULT l_eResult = AK_Success;

	CAkLEngine::DequeuePBI( in_pPBI );
	in_pPBI->Term();
	AkDelete( RENDERER_DEFAULT_POOL_ID, in_pPBI );

	return l_eResult;
} // DestroyPBI

//-----------------------------------------------------------------------------
// Name: DestroyAllPBIs
// Desc: Destroy all PBIs.
//
// Parameters:
//	None.
//
// Return: 
//	Ak_Success:          PBIs were removed or nothing to do.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Invalid pointer.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::DestroyAllPBIs( void )
{
	while ( CAkPBI * pPBI = m_listCtxs.First() )
	{
		m_listCtxs.RemoveFirst();
		pPBI->_Stop( AkPBIStopMode_Normal, true ); // necessary to avoid infinitely regenerating continuous PBIs
        DestroyPBI( pPBI );
	}

	return AK_Success;
} // DestroyAllPBIs

#ifndef AK_OPTIMIZED

void CAkURenderer::NotifBusUnsetRTPC( AkUniqueID in_MixBusID, AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetMixBusFXParams( in_MixBusID, in_FXID );
	if( l_Subs )
	{
		g_pRTPCMgr->UnSubscribeRTPC( l_Subs, in_ParamID, in_RTPCCurveID );
	}
	CAkLEngine::UnlockFxParams();
}

void CAkURenderer::NotifMasterBusUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetMasterBusFXParams( in_FXID );
	if( l_Subs )
	{
		g_pRTPCMgr->UnSubscribeRTPC( l_Subs, in_ParamID, in_RTPCCurveID );
	}
	CAkLEngine::UnlockFxParams();
}

void CAkURenderer::UpdateBusRTPC( AkUniqueID in_MixBusID, AkRTPCFXSubscription& in_rSubsItem )
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetMixBusFXParams( in_MixBusID, in_rSubsItem.FXID );

	if( l_Subs )
	{
		g_pRTPCMgr->SubscribeRTPC( l_Subs,
									in_rSubsItem.RTPCID,
									in_rSubsItem.ParamID,
									in_rSubsItem.RTPCCurveID,
									in_rSubsItem.ConversionTable.m_eScaling,
									in_rSubsItem.ConversionTable.m_pArrayGraphPoints,
									in_rSubsItem.ConversionTable.m_ulArraySize,
									NULL,
									CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
									);
	}
	CAkLEngine::UnlockFxParams();
}

void CAkURenderer::UpdateMasterBusRTPC( AkRTPCFXSubscription& in_rSubsItem )
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetMasterBusFXParams( in_rSubsItem.FXID );

	if( l_Subs )
	{
		g_pRTPCMgr->SubscribeRTPC( l_Subs,
									in_rSubsItem.RTPCID,
									in_rSubsItem.ParamID,
									in_rSubsItem.RTPCCurveID,
									in_rSubsItem.ConversionTable.m_eScaling,
									in_rSubsItem.ConversionTable.m_pArrayGraphPoints,
									in_rSubsItem.ConversionTable.m_ulArraySize,
									NULL,
									CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
									);
	}
	CAkLEngine::UnlockFxParams();
}

void CAkURenderer::UpdateBusFxParam(
	AkUniqueID in_MixBusID,
	AkPluginID		in_FXID,
	AkPluginParamID	in_ulParamID,
	void*			in_pvParamsBlock,
	AkUInt32			in_ulParamBlockSize
	)
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetMixBusFXParams( in_MixBusID, in_FXID );
	if( l_Subs )
	{
		l_Subs->SetParam( in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
	}
	CAkLEngine::UnlockFxParams();
}

void CAkURenderer::UpdateMasterBusFxParam(
	AkPluginID		in_FXID,
	AkPluginParamID	in_ulParamID,
	void*			in_pvParamsBlock,
	AkUInt32		in_ulParamBlockSize
	)
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetMasterBusFXParams( in_FXID );
	if( l_Subs )
	{
		l_Subs->SetParam( in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
	}
	CAkLEngine::UnlockFxParams();
}

void CAkURenderer::NotifEnvUnsetRTPC( AkEnvID in_envID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetEnvFXParams( in_envID );
	if( l_Subs )
	{
		g_pRTPCMgr->UnSubscribeRTPC( l_Subs, in_ParamID, in_RTPCCurveID );
	}
	CAkLEngine::UnlockFxParams();
}

void CAkURenderer::UpdateEnvRTPC( AkEnvID in_envID, AkRTPCEnvSubscription& in_rSubsItem )
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetEnvFXParams( in_envID );

	if( l_Subs )
	{
		g_pRTPCMgr->SubscribeRTPC( l_Subs,
									in_rSubsItem.RTPCID,
									in_rSubsItem.ParamID,
									in_rSubsItem.RTPCCurveID,
									in_rSubsItem.ConversionTable.m_eScaling,
									in_rSubsItem.ConversionTable.m_pArrayGraphPoints,
									in_rSubsItem.ConversionTable.m_ulArraySize,
									NULL,
									CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
									);
	}
	CAkLEngine::UnlockFxParams();
}

void CAkURenderer::UpdateEnvParam(
	AkEnvID			in_envID,
	AkPluginParamID	in_ulParamID,
	void*			in_pvParamsBlock,
	AkUInt32		in_ulParamBlockSize
	)
{
	CAkLEngine::LockFxParams();
	IAkRTPCSubscriber * l_Subs = CAkLEngine::GetEnvFXParams( in_envID );
	if( l_Subs )
	{
		l_Subs->SetParam( in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
	}
	CAkLEngine::UnlockFxParams();
}

#endif

//-----------------------------------------------------------------------------
// Name: AddFxParam
// Desc: Add an effect parameter to the fx param list.
//
// Parameters:
//	IAkPluginParam * in_pMaster :	Pointer to the master param.
//	IAkPluginParam * in_pClone  :	Pointer to the cloned param.
//
// Return:
//	AK_Success : object was added.
//	AK_Fail    : failed to add object, list was full.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::AddFxParam( IAkPluginParam * in_pMaster, IAkPluginParam * in_pClone )
{
	FxParamRec	l_ParamRec;
	l_ParamRec.m_pMaster = in_pMaster;
	l_ParamRec.m_pClone  = in_pClone;

	return m_listFxParam.AddFirst( l_ParamRec ) ? AK_Success : AK_Fail;
}

//-----------------------------------------------------------------------------
// Name: RemoveFxParam
// Desc: Remove the specified effect parameter.
//
// Parameters:
//	IAkPluginParam * in_pClone : Pointer to param object to remove.
//
// Return:
//	AK_Success : object was destroyed.
//	AK_Fail    : failed to destroy the object.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::RemoveFxParam( IAkPluginParam * in_pClone )
{
	
	AKRESULT	l_eResult = AK_Success;

	Lock();
	CAkURenderer::AkListFxParams::IteratorEx iter = m_listFxParam.BeginEx();
	while( iter != m_listFxParam.End() )
    {
		FxParamRec	l_ParamRec = *iter;

		AKASSERT( l_ParamRec.m_pMaster != NULL );
		if( l_ParamRec.m_pClone == in_pClone )
		{
			iter = m_listFxParam.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
	Unlock();

	return l_eResult;
}
//-----------------------------------------------------------------------------
// Name: SetParam()
// Desc: Set the parameter of a physical model source.
//
// Parameters:
//	IAkPluginParam * 	in_pMaster :	Pointer to the master param.
//	AkPluginParamID		in_ulParamID:  Id of parameter to set.
//	void *		 	in_vpParam:	Pointer to parameter block.
//  AkUInt32			 	in_ulSize:		Size of the parameter block.
//
// Return: Ak_Success:	Parameter set correctly.
//		   AK_Fail:		Failed to set parameter.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::SetParam(			// Set the parameter on an physical model source.
		IAkPluginParam * 	in_pMaster,		// Pointer to the master parm of the cloned param object.
		AkPluginParamID		in_ulParamID,		// Parameter id.
		void *		 	in_vpParam,		// Pointer to a setup param block.
		AkUInt32			 	in_ulSize			// Size of the parameter block.
		)
{
	AKRESULT	l_eResult = AK_Success;

	Lock();
	for( CAkURenderer::AkListFxParams::Iterator iter = m_listFxParam.Begin(); iter != m_listFxParam.End(); ++iter )
    {
		FxParamRec&	l_rParamRec = *iter;

		AKASSERT( l_rParamRec.m_pMaster != NULL );

		if( l_rParamRec.m_pMaster == in_pMaster )
		{
			l_eResult = AK_Fail;
			AKASSERT( l_rParamRec.m_pClone != NULL );
			if( l_rParamRec.m_pClone != NULL )
			{
				l_eResult = l_rParamRec.m_pClone->SetParam( in_ulParamID, in_vpParam, in_ulSize );
				AKASSERT( l_eResult == AK_Success );
			}
		}
	}
	Unlock();

	return l_eResult;
}

void CAkURenderer::StopAllPBIs( CAkUsageSlot* in_pUsageSlot )
{
	Lock();

	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
    {
		CAkPBI* l_pPBI = *iter;
		if( l_pPBI->GetUsageSlotToRelease() == in_pUsageSlot )
		{
			l_pPBI->_Stop();
		}
	} 

	Unlock();
}

//-----------------------------------------------------------------------------
// Name: Lock()
// Desc: Synchronisation lock.
//
// Parameters:
//	None.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Lock()
{
    return m_Lock.Lock();
}

//-----------------------------------------------------------------------------
// Name: Unlock()
// Desc: Synchronisation unlock.
//
// Parameters:
//	None.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Unlock()
{
    return m_Lock.Unlock();
}
