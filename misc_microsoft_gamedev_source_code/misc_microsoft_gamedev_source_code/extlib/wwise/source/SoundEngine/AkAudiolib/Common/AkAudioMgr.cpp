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
// CAkAudioMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkAudioMgr.h"
#include "AkAudioLib.h"
#include "AkAudioLibIndex.h"
#include "AkActions.h"
#include "AkRandom.h"

#include "Ak3DListener.h"
#include "AkLEngine.h"
#include "AkEnvironmentsMgr.h"
#include "AkURenderer.h"
#include "AkTransitionManager.h"
#include "AkPathManager.h"
#include "AkPlayingMgr.h"
#include "AkStateMgr.h"
#include "AkMonitor.h"
#include "AkCritical.h"
#include "AkParentNode.h"
#include "AkRegistryMgr.h"
#include "AkRTPCMgr.h"
#include "AkCntrHistory.h"
#include "AkEvent.h"
#include "AkBus.h"
#include "AkDynamicSequence.h"

#include "AkProfile.h"
#include "AkAudiolibTimer.h"

#include "AkBankMgr.h"

#include "AkFeedbackMgr.h"

#ifdef RVL_OS
#include "AkWiimoteMgr.h"
#endif

extern AkInitSettings					g_settings; 
extern AkBehavioralExtensionCallback g_pBehavioralExtensionCallback;
extern CAkBus * g_pMasterBus;

AkTimeMs	AkMonitor::m_ThreadTime = 0;

#define AK_MIN_NUM_EVENT_QUEUE_ITEMS 32
#define AK_MAX_NUM_EVENT_QUEUE_ITEMS AK_NO_MAX_LIST_SIZE

#define AK_MIN_NUM_EVENT_ACTION_ITEMS 32
#define AK_MAX_NUM_EVENT_ACTION_ITEMS AK_NO_MAX_LIST_SIZE

#define AK_MIN_NUM_PENDING_ITEMS 32
#define AK_MAX_NUM_PENDING_ITEMS AK_NO_MAX_LIST_SIZE

#define AK_MIN_NUM_PAUSED_PENDING_ITEMS 32
#define AK_MAX_NUM_PAUSED_PENDING_ITEMS AK_NO_MAX_LIST_SIZE

//-----------------------------------------------------------------------------
// Name: CAkAudioMgr
// Desc: Constructor.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
CAkAudioMgr::CAkAudioMgr()
	:m_bDrainMsgQueue(false)
	,m_MsgQueuePercentageFilled(0.0f)
	,m_MsgQueueActualSize(0)
	,m_ulWriterFlag(0)
	,m_ulReaderFlag(0)
	,m_uBufferTick(0)
{
	AkClearEvent(m_hEventMgrThreadDrainEvent);
}

//-----------------------------------------------------------------------------
// Name: ~CAkAudioMgr
// Desc: Destructor.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
CAkAudioMgr::~CAkAudioMgr()
{
}

AKRESULT CAkAudioMgr::Init()
{
	m_ulWriterFlag = 0;
	m_ulReaderFlag = 0;

	// Calculate the number of blocks we will create in the command queue
	AkUInt32 totalsize = g_settings.uCommandQueueSize;
	AkUInt32 numblocks = totalsize / m_MsgQueue.GetChunkSize();
	AKRESULT eResult = m_MsgQueue.Init(numblocks);
	
	if( eResult == AK_Success )
	{
		eResult = m_mmapPending.Init( AK_MIN_NUM_PENDING_ITEMS, AK_MAX_NUM_PENDING_ITEMS,g_DefaultPoolId );
	}
	if( eResult == AK_Success )
	{
		eResult = m_mmapPausedPending.Init( AK_MIN_NUM_PAUSED_PENDING_ITEMS, AK_MAX_NUM_PAUSED_PENDING_ITEMS,g_DefaultPoolId );
	}
	return eResult;
}

void CAkAudioMgr::Term()
{
	Stop();

	RemoveAllPreallocAndReferences();
	RemoveAllPausedPendingAction();
	RemoveAllPendingAction();

	m_MsgQueue.Term();
	m_mmapPending.Term();
	m_mmapPausedPending.Term();
}

//-----------------------------------------------------------------------------
// Name: RenderAudio
// Desc: Render Upper Audio Engine.
//
// Parameters:
//	None.
//
// Return: 
//	AK_Success :
//  AK_Fail    :
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::RenderAudio()
{
	bool bProcess = false; // do not unnecessarily wake up the audio thread if no events have been enqueued.

	m_queueLock.Lock();
	if( !m_MsgQueue.IsEmpty() )
	{
		// Add end-of-list only if one does not exist.
		AkQueuedMsg EndOfListItem;
		EndOfListItem.type = QueuedMsgType_EndOfList;
		EndOfListItem.size = AkQueuedMsg::Sizeof_EndOfList();
		if ( !m_MsgQueue.Write( &EndOfListItem, AkQueuedMsg::Sizeof_EndOfList() ) == AK_Success )
		{
			m_bDrainMsgQueue = true;
		}
		
		//the flag must be incremented only AFTER having added the EndOf List flag in the queue
		++m_ulWriterFlag;
		bProcess = true;
	}
	m_queueLock.Unlock();

	if ( bProcess )
	{
		m_audioThread.WakeupEventsConsumer();
	}

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Enqueue
// Desc: Enqueue actions.
//
// Parameters:
//	None.
//
// Return: 
//	AK_Success : VPL executed. 
//  AK_Fail    : Failed to execute a VPL.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::Enqueue( AkQueuedMsg& in_rItem, AkUInt32 in_uSize )
{
	in_rItem.size = (AkUInt16) in_uSize;

	AKRESULT eResult; 

	m_queueLock.Lock();
	eResult = m_MsgQueue.Write( &in_rItem, in_uSize );
	m_queueLock.Unlock();

	// Our message queue is full; drain it and wait for the audio thread
	// to tell us that we can re-enque this message
	while (eResult == AK_InsufficientMemory)
	{
		// tell audio thread to drain the message queue
		m_bDrainMsgQueue = true;			

		// increment writer flag to trigger process properly
		++m_ulWriterFlag;

		// wake up the audio thread
		m_audioThread.WakeupEventsConsumer();

		// wait for audio thread to signal this thread 
		AkWaitForEvent( m_hEventMgrThreadDrainEvent ); 

		// try writing again
		m_queueLock.Lock();
		eResult = m_MsgQueue.Write( &in_rItem, in_uSize );
		m_queueLock.Unlock();
	}

	return eResult;
}

void CAkAudioMgr::Perform()
{
	g_csMain.Lock();

	AK_START_TIMER_AUDIO();

	// Process events from the main queue.
	ProcessMsgQueue();

	AkUInt32 l_uNumBufferToFill = CAkLEngine::GetNumBufferNeededAndSubmit();

	while( l_uNumBufferToFill )
	{
		// Process events that are pending.
		ProcessPendingList();

		if ( g_pBehavioralExtensionCallback )
            g_pBehavioralExtensionCallback();

		// Here, the +1 forces the one buffer look ahead.
		// We must tell the transitions where we want to be on next buffer, not where we are now
		AkUInt32 l_ActualBufferTick = GetBufferTick() + 1;

		g_pTransitionManager->ProcessTransitionsList( l_ActualBufferTick );
		g_pPathManager->ProcessPathsList( l_ActualBufferTick );

		g_csMain.Unlock(); // Do not keep lock during low-level rendering.

		// Execute the Lower Audio Engine.
		CAkAttenuation::AcquireAttenuationLock();
		l_uNumBufferToFill = CAkLEngine::Perform( l_uNumBufferToFill );
		CAkAttenuation::ReleaseAttenuationLock();

		g_csMain.Lock();

		CAkURenderer::PerformContextNotif();

		IncrementBufferTick();
	}

	//Command driven devices don't automatically grab their samples.  We must send them periodically.
	//This is the case for all rumble controllers for example.
	CAkFeedbackDeviceMgr *pFeedbackMgr = CAkFeedbackDeviceMgr::Get();
	if (pFeedbackMgr != NULL)
	{
#ifdef RVL_OS
		//On the wii, send a sample only once every 4 buffers.  This makes a sample rate of 83 samples per second.
		if ((GetBufferTick() & (AK_FEEDBACK_DIVIDER - 1)) == 0)
#endif
		pFeedbackMgr->CommandTick();
	}

	AK_STOP_TIMER_AUDIO();

	g_csMain.Unlock();

	AK_PERF_TICK_AUDIO();
}

//-----------------------------------------------------------------------------
// Name: ProcessMsgQueue
// Desc: Process the message queue.
//
// Parameters:
//	None.
//
// Return: 
//	AK_Success:	Succeeded.
//  AK_Fail   : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::ProcessMsgQueue()
{
	AKASSERT(g_pIndex);


#ifndef AK_OPTIMIZED
	//m_queueLock.Lock(); locking removed for idle performance -- values read are not critical
	// how big has the message queue grown
	m_MsgQueueActualSize = m_MsgQueue.GetActualSize();

	// how much of the queue has been filled
	AkReal32 uMsgQueuePercentageFilled = m_MsgQueue.GetPercentageUsed();
	if(m_MsgQueuePercentageFilled < uMsgQueuePercentageFilled)
	{
		m_MsgQueuePercentageFilled = uMsgQueuePercentageFilled;
	}

	//m_queueLock.Unlock();
#endif

	// Skip this if we are going to drain the message queue
	if(!m_bDrainMsgQueue)
	{
		if( m_ulWriterFlag == m_ulReaderFlag )
		{
			return AK_Success;
		}
	}

	bool bReachedEnd = false;

	m_queueLock.Lock();

	do
	{
		AkUInt32 uSizeAvail = 0, uSizeRead = 0;

		// Since we are draining the queue, bail out early
		if(m_MsgQueue.IsEmpty())
		{
			break;
		}
	
		AkUInt8 * pData =  (AkUInt8 *) m_MsgQueue.BeginReadEx( uSizeAvail );
		AKASSERT( pData );

		m_queueLock.Unlock();

		do
		{
			AkQueuedMsg * pItem = (AkQueuedMsg *) pData;

			switch ( pItem->type )
			{
			default:
				AKASSERT( !"Invalid msg queue item type!" );
				break;

			case QueuedMsgType_EndOfList:
					bReachedEnd = ( ++m_ulReaderFlag == m_ulWriterFlag );
				break;

			case QueuedMsgType_Event:
				{
					MONITOR_EVENTTRIGGERED( pItem->event.PlayingID, pItem->event.Event->ID(), pItem->event.GameObjID, pItem->event.CustomParam );

					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->event.GameObjID );

					for( CAkEvent::AkActionList::Iterator iter = pItem->event.Event->m_actions.Begin(); iter != pItem->event.Event->m_actions.End(); ++iter )
					{
						CAkAction* pAction = *iter;
						AKASSERT( pAction );

						AkPendingAction* pThisAction = NULL;

						if ( ACTION_TYPE_USE_OBJECT & pAction->ActionType() )
						{
							if ( pGameObj ) // Action requires an object
							{
								pThisAction = AkNew( g_DefaultPoolId, AkPendingAction( pGameObj ) );
							}
							else
							{
								MONITOR_ERRORMSG2( L"Action not executed: ", L"Invalid or non-registered game object" );
							}
						}
						else
						{
							pThisAction = AkNew( g_DefaultPoolId, AkPendingAction( NULL ) );
						}

						if( pThisAction )
						{
							pThisAction->pAction = pAction;

							pThisAction->UserParam.CustomParam = pItem->event.CustomParam;
							pThisAction->UserParam.PlayingID = pItem->event.PlayingID;

							EnqueueOrExecuteAction( pThisAction );// true tells to hold delayed items, they will be added in the delayed list later.
						}
					}
					if ( pGameObj )
					{
						pGameObj->Release();
					}
					g_pPlayingMgr->RemoveItemActiveCount( pItem->event.PlayingID );

					pItem->event.Event->Release();
				}
				break;

			case QueuedMsgType_RTPC:
				if ( pItem->rtpc.GameObjID == AK_INVALID_GAME_OBJECT )
					g_pRTPCMgr->SetRTPCInternal( pItem->rtpc.ID, pItem->rtpc.Value, NULL );
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->rtpc.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->SetRTPCInternal( pItem->rtpc.ID, pItem->rtpc.Value, pGameObj );
						pGameObj->Release();
					}
				}
				break;

			case QueuedMsgType_State:
				g_pStateMgr->SetStateInternal( 
					pItem->setstate.StateGroupID, 
					pItem->setstate.StateID, 
					pItem->setstate.bSkipTransition,
                    pItem->setstate.bSkipExtension);
				break;

			case QueuedMsgType_Switch:
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->setswitch.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->SetSwitchInternal( 
							pItem->setswitch.SwitchGroupID, 
							pItem->setswitch.SwitchStateID, 
							pGameObj );
						pGameObj->Release();
					}
				}
				break;

			case QueuedMsgType_Trigger:
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->trigger.GameObjID );

					g_pStateMgr->Trigger( pItem->trigger.TriggerID, pGameObj );

					if( pGameObj )
						pGameObj->Release();
				}
				break;

			case QueuedMsgType_RegisterGameObj:
				g_pRegistryMgr->RegisterObject( pItem->reggameobj.GameObjID, pItem->reggameobj.pMonitorData );
				break;

			case QueuedMsgType_UnregisterGameObj:
				if ( pItem->unreggameobj.GameObjID == AK_INVALID_GAME_OBJECT )
					g_pRegistryMgr->UnregisterAll();
				else
					g_pRegistryMgr->UnregisterObject( pItem->unreggameobj.GameObjID );
				break;

			case QueuedMsgType_GameObjPosition:
				g_pRegistryMgr->SetPosition( pItem->gameobjpos.GameObjID, pItem->gameobjpos.Position, pItem->gameobjpos.uListenerIndex );
				break;

			case QueuedMsgType_GameObjActiveListeners:
				g_pRegistryMgr->SetActiveListeners( pItem->gameobjactlist.GameObjID, pItem->gameobjactlist.uListenerMask );
				break;

#ifdef RVL_OS
			case QueuedMsgType_GameObjActiveControllers:
				g_pRegistryMgr->SetActiveControllers( pItem->gameobjactcontroller.GameObjID, pItem->gameobjactcontroller.uActiveControllerMask );
				break;

			case QueuedMsgType_ControllerVolume:
				CAkWiimoteMgr::SetWiimoteVolume( pItem->controllervolume.uControllerID, pItem->controllervolume.fControllerVolume );
				break;
#endif

			case QueuedMsgType_ListenerPosition:
				CAkListener::SetListenerPosition( pItem->listpos.uListenerIndex, pItem->listpos.Position );
				break;

			case QueuedMsgType_ListenerSpatialization:
				CAkListener::SetListenerSpatialization( 
					pItem->listspat.uListenerIndex, 
					pItem->listspat.bSpatialized, 
					pItem->listspat.bSetVolumes ? &pItem->listspat.Volumes : NULL );
				break;

			case QueuedMsgType_ListenerPipeline:
				CAkListener::SetListenerPipeline(
					pItem->listpipe.uListenerIndex, 
					pItem->listpipe.bAudio,
					pItem->listpipe.bFeedback);
				break;

			case QueuedMsgType_GameObjEnvValues:
				g_pRegistryMgr->SetGameObjectEnvironmentsValues( 
					pItem->gameobjenvvalues.GameObjID, 
					pItem->gameobjenvvalues.EnvValues, 
					pItem->gameobjenvvalues.uNumValues );
				break;

			case QueuedMsgType_GameObjDryLevel:
				g_pRegistryMgr->SetGameObjectDryLevelValue( 
					pItem->gameobjdrylevel.GameObjID, 
					pItem->gameobjdrylevel.fValue );
				break;

			case QueuedMsgType_EnvVolume:
				g_pEnvironmentMgr->SetEnvironmentVolume( pItem->envvolume.EnvID, pItem->envvolume.fVolume );
				break;

			case QueuedMsgType_EnvBypass:
				g_pEnvironmentMgr->BypassEnvironment( pItem->envbypass.EnvID, pItem->envbypass.bBypass );
				break;

			case QueuedMsgType_GameObjObstruction:
				g_pRegistryMgr->SetObjectObstructionAndOcclusion( 
					pItem->gameobjobstr.GameObjID, 
					pItem->gameobjobstr.uListenerIndex, 
					pItem->gameobjobstr.fObstructionLevel, 
					pItem->gameobjobstr.fOcclusionLevel );
				break;

			case QueuedMsgType_ResetSwitches:
				if ( pItem->resetswitches.GameObjID == AK_INVALID_GAME_OBJECT )
					g_pRTPCMgr->ResetSwitches();
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->resetswitches.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->ResetSwitches( pGameObj );
						pGameObj->Release();
					}
				}
				break;

			case QueuedMsgType_ResetRTPC:
				if ( pItem->resetrtpc.GameObjID == AK_INVALID_GAME_OBJECT )
					g_pRTPCMgr->ResetRTPC();
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->resetrtpc.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->ResetRTPC( pGameObj );
						pGameObj->Release();
					}
				}
				break;

			///////////////////////////////////////
			// Dynamic Sequence Related messages
			/////////////////////////////////////
			case QueuedMsgType_OpenDynamicSequence:
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->opendynamicsequence.GameObjID );
					if ( pGameObj )
					{
						pItem->opendynamicsequence.pDynamicSequence->SetGameObject( pGameObj );
						pGameObj->Release();
					}
				}
				break;

			case QueuedMsgType_DynamicSequenceCmd:
				switch ( pItem->dynamicsequencecmd.eCommand )
				{
				case AkQueuedMsg_DynamicSequenceCmd::Play:
					pItem->dynamicsequencecmd.pDynamicSequence->Play();
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Pause:
					pItem->dynamicsequencecmd.pDynamicSequence->Pause();
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Resume:
					pItem->dynamicsequencecmd.pDynamicSequence->Resume();
					break;

				case AkQueuedMsg_DynamicSequenceCmd::ResumeWaiting:
					pItem->dynamicsequencecmd.pDynamicSequence->ResumeWaiting();
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Stop:
					pItem->dynamicsequencecmd.pDynamicSequence->Stop();
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Break:
					pItem->dynamicsequencecmd.pDynamicSequence->Break();
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Close:
					pItem->dynamicsequencecmd.pDynamicSequence->Close();

					// Ok, we can not get rid of the playing id for which we add an active count in the OpenDynamicSequence
					g_pPlayingMgr->RemoveItemActiveCount( pItem->dynamicsequencecmd.pDynamicSequence->GetPlayingID() );

					// Release for the creation by the OpenDynamicSequence message
					pItem->dynamicsequencecmd.pDynamicSequence->Release();
					break;
				}

				// Release for enqueue item
				AKASSERT( pItem->dynamicsequencecmd.pDynamicSequence );
				pItem->dynamicsequencecmd.pDynamicSequence->Release();
				break;

			case QueuedMsgType_StopAll:
				{
					AkGameObjectID gameObjectID = pItem->stopAll.GameObjID;
					AkActionType actionType = gameObjectID == AK_INVALID_GAME_OBJECT ? AkActionType_Stop_ALL : AkActionType_Stop_ALL_O;
					CAkActionStop* pStopAction = CAkActionStop::Create( actionType );
					if( pStopAction )
					{
						if( gameObjectID != AK_INVALID_GAME_OBJECT )
						{
							CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( gameObjectID );
							if ( pGameObj )
							{
								AkPendingAction PendingAction( pGameObj );// dummy with no game object.
								pStopAction->Execute( &PendingAction );
								pGameObj->Release();
							}
						}
						else
						{
							AkPendingAction PendingAction( NULL );// dummy with no game object.
							pStopAction->Execute( &PendingAction );
						}
						pStopAction->Release();
					}
				}
				break;

			case QueuedMsgType_StopPlayingID:
				{
					ClearPendingItems( pItem->stopEvent.playingID );

					if( g_pMasterBus )
					{
						ActionParams l_Params;
						l_Params.bIsFromBus = false;
						l_Params.bIsMasterResume = false;
						l_Params.transParams.eFadeCurve = AkCurveInterpolation_Linear;
						l_Params.eType = ActionParamType_Stop;
						l_Params.pGameObj = NULL;
						l_Params.playingID = pItem->stopEvent.playingID;
						l_Params.transParams.TransitionTime = 0;
						l_Params.bIsMasterCall = false;

						g_pMasterBus->ExecuteAction( l_Params );
					}
				}
				break;

			case QueuedMsgType_KillBank:
				{
					CAkUsageSlot* pUsageSlot = pItem->killbank.pUsageSlot;
					AKASSERT( pUsageSlot );

					pUsageSlot->StopContent();
					CAkURenderer::StopAllPBIs( pUsageSlot );
					pUsageSlot->Release( false );
				}
				break;

			case QueuedMsgType_AddRemovePlayerDevice:
				{
					if (pItem->playerdevice.bAdd)
					{
						//Always create the device manager (checks are done inside).  It is the only way to enable feedback.
						CAkFeedbackDeviceMgr::Create()->AddPlayerFeedbackDevice(pItem->playerdevice.iPlayer, pItem->playerdevice.idCompany, pItem->playerdevice.idDevice);
					}
					else
					{
						CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
						if(pMgr != NULL)
							pMgr->RemovePlayerFeedbackDevice(pItem->playerdevice.iPlayer, pItem->playerdevice.idCompany, pItem->playerdevice.idDevice);
					}
				}
				break;

			case QueuedMsgType_SetPlayerListener:
				{
					CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
					if(pMgr != NULL)
						pMgr->SetPlayerListener(pItem->playerlistener.iPlayer, pItem->playerlistener.iListener);
				}
				break;
			case QueuedMsgType_SetPlayerVolume:
				{
					CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
					if(pMgr != NULL)
						pMgr->SetPlayerVolume((AkUInt8)pItem->playervolume.iPlayer, pItem->playervolume.fVolume);
				}
			}

			
			pData += pItem->size;
			uSizeRead += pItem->size;
			
			// For each event processed, increment the synchronization count.
			// for RTPC and switches too, since they can cause new playback to occur.
			CAkLEngine::IncrementSyncCount();
		}
		while ( uSizeRead < uSizeAvail && !bReachedEnd );

		m_queueLock.Lock();

		m_MsgQueue.EndRead( uSizeRead );
	}
	while( !bReachedEnd );

	m_queueLock.Unlock();

	// If we were asked to drain the queue, make sure to reset
	m_bDrainMsgQueue = false;

	// We are setting this event in case multiple threads have asked the audio
	// thread to drain the queue
	AkSignalEvent( m_hEventMgrThreadDrainEvent );

	return AK_Success;
}
//-----------------------------------------------------------------------------
// Name: ProcessPendingList
// Desc: figure out if some of those pending are ready to run.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//----------------------------------------------------------------------------- 
void CAkAudioMgr::ProcessPendingList()
{
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		// is it time to go ?
		if( (*iter).key <= m_uBufferTick)
		{
			AkPendingAction* pPendingAction = (*iter).item;
			m_mmapPending.Erase( iter );

			NotifyDelayEnded( pPendingAction );

			ProcessAction( pPendingAction );
			
			//increment the sync count for pending events too, so that they don't sync with next events.
			CAkLEngine::IncrementSyncCount();
			
			iter = m_mmapPending.BeginEx();
		}
		else
        {
			break;
        }
	}
}

//-----------------------------------------------------------------------------
// Name: EnqueueOrExecuteAction
// Desc: Enqueue or execute an action.
//
// Parameters:
//	AkPendingAction* in_pActionItem : Action to enqueue.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::EnqueueOrExecuteAction( AkPendingAction* in_pActionItem )
{
	AKASSERT(in_pActionItem);
	AKASSERT(in_pActionItem->pAction);

	g_pPlayingMgr->AddItemActiveCount(in_pActionItem->UserParam.PlayingID);

	// make sure it won't get thrown away
	in_pActionItem->pAction->AddRef();

	AkUInt32 delayS = in_pActionItem->pAction->Delay();
	AkUInt32 delayTicks = delayS / AK_NUM_VOICE_REFILL_FRAMES;

	in_pActionItem->LaunchTick = m_uBufferTick + delayTicks;
	in_pActionItem->LaunchFrameOffset = delayS - delayTicks * AK_NUM_VOICE_REFILL_FRAMES;

	if( delayTicks > 0 )
	{
		if( m_mmapPending.Insert( in_pActionItem->LaunchTick, in_pActionItem ) == AK_Success )
		{
			MONITOR_ACTIONDELAYED( in_pActionItem->UserParam.PlayingID, in_pActionItem->pAction->ID(), in_pActionItem->GameObjID(), in_pActionItem->pAction->Delay() * 1000 / AK_CORE_SAMPLERATE, in_pActionItem->UserParam.CustomParam );
			NotifyDelayStarted( in_pActionItem );
		}
		else
		{
			FlushAndCleanPendingAction( in_pActionItem );
		}
	}
	else
	{
		ProcessAction( in_pActionItem );
	}
}

void CAkAudioMgr::FlushAndCleanPendingAction( AkPendingAction* in_pPendingAction )
{
	in_pPendingAction->pAction->Release();
	AkDelete( g_DefaultPoolId, in_pPendingAction );
}

//-----------------------------------------------------------------------------
// Name: ProcessActionQueue
// Desc: Process the action queue.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::ProcessAction( AkPendingAction * in_pAction ) 
{
	AKASSERT( in_pAction->pAction );

	MONITOR_ACTIONTRIGGERED( in_pAction->UserParam.PlayingID, in_pAction->pAction->ID(), in_pAction->GameObjID(), in_pAction->UserParam.CustomParam );

	// execute it
	in_pAction->pAction->Execute( in_pAction );

	// Important to do it AFTER calling execute, that will avoid having the Playing mgr thinking that the Playing ID is dead.
	if( in_pAction->UserParam.PlayingID )
	{
		g_pPlayingMgr->RemoveItemActiveCount( in_pAction->UserParam.PlayingID );
	}

	// we don't care anymore about that one
	in_pAction->pAction->Release();

	// get rid of it
	AkDelete( g_DefaultPoolId, in_pAction );
}

//-----------------------------------------------------------------------------
// Name: PausePendingAction
// Desc: move actions(in_ulElementID) pending in the paused pending.
//
// Parameters:
//	AkUniqueID	 in_ulElementID		   :
//	CAkRegisteredObj * in_GameObj		       :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::PausePendingAction( AkUniqueID	in_ulElementID, 
										  CAkRegisteredObj * in_GameObj,
										  bool		in_bIsMasterOnResume )
{
	AKRESULT	eResult = AK_Success;

	AkPendingAction*	pThisAction = NULL;
	CAkAction*			pAction		= NULL;

	// scan'em all

	AkMultimapPausedPending::Iterator iterPaused = m_mmapPausedPending.Begin();
	while( iterPaused != m_mmapPausedPending.End() )
	{
		pThisAction = (*iterPaused).item;
		pAction = pThisAction->pAction;

		// is it ours ? we do pause Resume actions only if we are a master pause
		if(
			(!in_ulElementID || IsElementOf( in_ulElementID, pAction->ElementID() ) )
			&& ( ( (pAction->ActionType() & ACTION_TYPE_ACTION) != ACTION_TYPE_RESUME) || in_bIsMasterOnResume )
			&& ( in_GameObj == NULL || in_GameObj == pThisAction->GameObj())
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{
			++(pThisAction->ulPauseCount);
		}

		++iterPaused;
	}

	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		pAction = pThisAction->pAction;

		// is it ours ? we do pause Resume actions only if we are a master pause
		if(
			(!in_ulElementID || IsElementOf( in_ulElementID, pAction->ElementID() ) )
			&& ( ( (pAction->ActionType() & ACTION_TYPE_ACTION) != ACTION_TYPE_RESUME) || in_bIsMasterOnResume )
			&& (in_GameObj == NULL || in_GameObj == pThisAction->GameObj())
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{
			InsertAsPaused( pAction->ElementID(), pThisAction );
			iter = m_mmapPending.Erase( iter );
		}
		else
		{
			++iter;
		}
	}

	return eResult;
}
//--------------------------------------------------------------------------------------------
// Name: PausePendingItem
// Desc: move actions affecting an audionode (in_ulAudioNodeID) pending in the paused pending.
//
// Parameters:
//	AkUniqueID	 in_ulElementID		   :
//	CAkRegisteredObj * in_GameObj		       :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::PausePendingItems( AkPlayingID in_PlayingID )
{
	AKRESULT	eResult = AK_Success;

	AkPendingAction*	pThisAction = NULL;
	CAkAction*			pAction		= NULL;

	// scan'em all

	AkMultimapPausedPending::Iterator iterPaused = m_mmapPausedPending.Begin();
	while( iterPaused != m_mmapPausedPending.End() )
	{
		pThisAction = (*iterPaused).item;

		// is it ours ? we do pause Resume actions only if they match our playing id
		if(	   ( pThisAction->UserParam.PlayingID == in_PlayingID )
			&& ( pAction->ActionType() != AkActionType_Duck ) )
		{
			++(pThisAction->ulPauseCount);
		}

		++iterPaused;
	}

	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		pAction = pThisAction->pAction;

		// is it ours ? we do pause Resume actions only if they match our playing id
		if(	   ( pThisAction->UserParam.PlayingID == in_PlayingID )
			&& ( pAction->ActionType() != AkActionType_Duck ) )
		{
			InsertAsPaused( pAction->ElementID(), pThisAction );
			iter = m_mmapPending.Erase( iter );
		}
		else
		{
			++iter;
		}
	}

	return eResult;
}

void CAkAudioMgr::InsertAsPaused( AkUniqueID in_ElementID, AkPendingAction* in_pPendingAction, AkUInt32 in_ulPauseCount )
{
	in_pPendingAction->PausedTick = m_uBufferTick;
	in_pPendingAction->ulPauseCount = in_ulPauseCount;

	// shove the action in the PausedPending list
	AKRESULT eResult = m_mmapPausedPending.Insert( in_ElementID, in_pPendingAction );

	if( eResult == AK_Success )
	{
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF( in_pPendingAction->UserParam.PlayingID, in_pPendingAction->GameObjID(), in_pPendingAction->UserParam.CustomParam, AkMonitorData::NotificationReason_Paused, HistArray, in_pPendingAction->pAction->ID(), 0 );
	}
	else
	{
		MONITOR_ERRORMSG2( L"Pending action was destroyed because a critical memory allocation failed.", L"" );
		NotifyDelayAborted( in_pPendingAction, false );
		FlushAndCleanPendingAction( in_pPendingAction );
	}
}

void CAkAudioMgr::TransferToPending( AkPendingAction* in_pPendingAction )
{
	// offset the launch time by the pause duration
	in_pPendingAction->LaunchTick += ( m_uBufferTick - in_pPendingAction->PausedTick );

	// shove it action in the Pending list
	AKRESULT eResult = m_mmapPending.Insert( in_pPendingAction->LaunchTick, in_pPendingAction );

	if( eResult == AK_Success )
	{
		// Here we must send the resume notification anyway!, to balance the pause notification.
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF( in_pPendingAction->UserParam.PlayingID, in_pPendingAction->GameObjID(), in_pPendingAction->UserParam.CustomParam, AkMonitorData::NotificationReason_Resumed, HistArray, in_pPendingAction->pAction->ID(), 0 );
	}
	else
	{
		MONITOR_ERRORMSG2( L"Pending action was destroyed because a critical memory allocation failed.", L"" );
		NotifyDelayAborted( in_pPendingAction, true );
		FlushAndCleanPendingAction( in_pPendingAction );
	}
}

//-----------------------------------------------------------------------------
// Name: BreakPendingAction
// Desc: Break pending actions.
//
// Parameters:
// AkUniqueID   in_TargetID			  :
// CAkRegisteredObj * in_GameObj      :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::BreakPendingAction( AkUniqueID   in_TargetID, 
										 CAkRegisteredObj * in_GameObj )
{
	AKRESULT	eResult = AK_Success;

	CAkAction* pAction;
	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			pAction = pPendingAction->pAction;
			if(
				(!in_TargetID || IsElementOf( in_TargetID, pAction->ElementID() ) )
				&& 
				( in_GameObj == NULL || pPendingAction->GameObj() == in_GameObj)
				)
			{	
				bool l_bFlush = false;
				switch( pAction->ActionType() )
				{
				case AkActionType_PlayAndContinue:
					l_bFlush = static_cast<CAkActionPlayAndContinue*>( pPendingAction->pAction )->BreakToNode( in_TargetID, pPendingAction->GameObj(), pPendingAction );
					break;
				case AkActionType_Play:
					l_bFlush = true;
					break;
				}

				if( l_bFlush )
				{
					NotifyDelayAborted( pPendingAction, false );
					iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
				}
				else
				{
					++iter;
				}
			}
			else
			{
				++iter;
			}
		}
	}

	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			pAction = pPendingAction->pAction;
			if(
				( !in_TargetID || IsElementOf( in_TargetID, pAction->ElementID() ) )
				&& 
				( in_GameObj == NULL || pPendingAction->GameObj() == in_GameObj )
				&& 
				( pAction->ActionType() == AkActionType_PlayAndContinue )
				)
			{
				bool l_bFlush = static_cast<CAkActionPlayAndContinue*>( pPendingAction->pAction )->BreakToNode( in_TargetID, pPendingAction->GameObj(), pPendingAction );
				if( l_bFlush )
				{
					NotifyDelayAborted( pPendingAction, true );
					iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
				}
				else
				{
					++iter;
				}
			}
			else
			{
				++iter;
			}
		}
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: StopPendingAction
// Desc: Stop pending actions.
//
// Parameters:
// AkUniqueID   in_TargetID			  :
// CAkRegisteredObj * in_GameObj            :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::StopPendingAction( AkUniqueID   in_TargetID, 
										 CAkRegisteredObj * in_GameObj )
{
	AKRESULT	eResult = AK_Success;

	CAkAction* pAction;
	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			pAction = pPendingAction->pAction;
			if(
				(!in_TargetID || IsElementOf( in_TargetID, pAction->ElementID() ) )
				&& 
				( in_GameObj == NULL || pPendingAction->GameObj() == in_GameObj)
				&& 
				( pAction->ActionType() != AkActionType_Duck )
				)
			{
				NotifyDelayAborted( pPendingAction, false );

				iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}

	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			pAction = pPendingAction->pAction;
			if(
				( !in_TargetID || IsElementOf( in_TargetID, pAction->ElementID() ) )
				&& 
				( in_GameObj == NULL || pPendingAction->GameObj() == in_GameObj )
				&& 
				( pAction->ActionType() != AkActionType_Duck )
				)
			{
				NotifyDelayAborted( pPendingAction, true );

				iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: PausePendingActionAllExcept
// Desc:
//
// Parameters:
// CAkRegisteredObj *		in_GameObj				:
// ExceptionList*	in_pExceptionList		:
//
// Return:
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::PausePendingActionAllExcept(CAkRegisteredObj *in_GameObj, 
												  ExceptionList*	in_pExceptionList,
												  bool			in_bIsMasterOnResume )
{
	AKRESULT	eResult = AK_Success;

	AkPendingAction*	pThisAction = NULL;
	CAkAction*			pAction		= NULL;

	AkMultimapPausedPending::Iterator iterPaused = m_mmapPausedPending.Begin();
	while( iterPaused != m_mmapPausedPending.End() )
	{
		pThisAction = (*iterPaused).item;
		pAction = pThisAction->pAction;

		// is it ours ? we don't pause pending resumes or we'll get stuck
		if(
			(( (pAction->ActionType() & ACTION_TYPE_ACTION) != ACTION_TYPE_RESUME) || in_bIsMasterOnResume )
			&& ( in_GameObj == NULL || in_GameObj == pThisAction->GameObj())
			&& !IsAnException( pAction, in_pExceptionList )
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{	
			++(pThisAction->ulPauseCount);
		}
		++iterPaused;
	}

	// scan'em all
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		pAction = pThisAction->pAction;

		// is it ours ? we don't pause pending resumes or we'll get stuck
		if(
			(( (pAction->ActionType() & ACTION_TYPE_ACTION) != ACTION_TYPE_RESUME) || in_bIsMasterOnResume )
			&& ( in_GameObj == NULL || in_GameObj == pThisAction->GameObj())
			&& !IsAnException( pAction, in_pExceptionList )
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{	
			InsertAsPaused( pAction->ElementID(), pThisAction );
			iter = m_mmapPending.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
	return eResult;
}

//-----------------------------------------------------------------------------
// Name: StopPendingActionAllExcept
// Desc:
//
// Parameters:
// CAkRegisteredObj *	  in_GameObj		    :
// ExceptionList& in_pExceptionList      :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::StopPendingActionAllExcept( CAkRegisteredObj * in_GameObj,
												  ExceptionList* in_pExceptionList )
{
	AKRESULT	eResult = AK_Success;

	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			if(
				( in_GameObj == NULL || pPendingAction->GameObj() == in_GameObj )
				&& !IsAnException( pPendingAction->pAction, in_pExceptionList )
				&& ( pPendingAction->pAction->ActionType() != AkActionType_Duck )
				)
			{
				NotifyDelayAborted( pPendingAction, false );

				iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}

	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			if(
				( in_GameObj == NULL || pPendingAction->GameObj() == in_GameObj )
				&& !IsAnException( pPendingAction->pAction, in_pExceptionList )
				&& ( pPendingAction->pAction->ActionType() != AkActionType_Duck )
				)
			{
				NotifyDelayAborted( pPendingAction, true );

				iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: ResumePausedPendingAction
// Desc: move paused pending actions(in_ulElementID) in the pending list
//
// Parameters:
//	AkUniqueID		in_ulElementID		  :
//	CAkRegisteredObj *	in_GameObj			  :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::ResumePausedPendingAction( AkUniqueID		in_ulElementID, 
												 CAkRegisteredObj *	in_GameObj,
												 bool			in_bIsMasterOnResume )
{
	AKRESULT eResult = AK_Success;

	AkPendingAction* pThisAction;

	// if we've got it then move it
	
	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		pThisAction = (*iter).item;
		if( (!in_ulElementID || IsElementOf( in_ulElementID, (*iter).key ))
			&& ( in_GameObj == NULL || pThisAction->GameObj() == in_GameObj))
		{
			if( in_bIsMasterOnResume || pThisAction->ulPauseCount == 0 )
			{
				TransferToPending( pThisAction );
				iter = m_mmapPausedPending.Erase( iter );
			}
			else
			{
				--(pThisAction->ulPauseCount);
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}

	return eResult;
}

AKRESULT CAkAudioMgr::ResumePausedPendingItems( AkPlayingID in_playingID )
{
	AKRESULT eResult = AK_Success;

	AkPendingAction* pThisAction;

	// if we've got it then move it
	
	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		pThisAction = (*iter).item;
		if( pThisAction->UserParam.PlayingID == in_playingID )
		{
			if( pThisAction->ulPauseCount == 0 )
			{
				TransferToPending( pThisAction );
				iter = m_mmapPausedPending.Erase( iter );
			}
			else
			{
				--(pThisAction->ulPauseCount);
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}

	return eResult;
}

AKRESULT CAkAudioMgr::ResumeNotPausedPendingAction( AkUniqueID		in_ulElementID,
													CAkRegisteredObj * in_GameObj,
													bool		in_bIsMasterOnResume )
{
	//in_bIsMasterOnResume here is unused for now, but will be useful when supporting pause counting
	AKRESULT eResult = AK_Success;

	AkPendingAction* pThisAction;

	// if we've got it then move it
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		CAkAction* pAction = pThisAction->pAction;
		if( (!in_ulElementID || IsElementOf( in_ulElementID, pAction->ElementID() ))
			&& ( in_GameObj == NULL || pThisAction->GameObj() == in_GameObj))
		{	 
			if( pAction->ActionType() == AkActionType_PlayAndContinue )
			{
				CAkActionPlayAndContinue* pActionPAC = static_cast<CAkActionPlayAndContinue*>( pAction );
				pActionPAC->Resume();
			}
		}
		++iter;
	}
	return eResult;	
}

//-----------------------------------------------------------------------------
// Name: ResumePausedPendingActionAllExcept
// Desc: Execute behavioural side and lower engine side.
//
// Parameters:
// CAkRegisteredObj *		in_GameObj			  :
// ExceptionList&	in_pExceptionList      :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::ResumePausedPendingActionAllExcept(CAkRegisteredObj * in_GameObj, 
														 ExceptionList* in_pExceptionList,
														 bool			in_bIsMasterOnResume )
{
	AKRESULT eResult = AK_Success;

	AkPendingAction*	pThisAction = NULL;

	// if we've got it then move it
	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		pThisAction = (*iter).item;
		if(
			( in_GameObj == NULL || pThisAction->GameObj() == in_GameObj )
			&& !IsAnException( pThisAction->pAction, in_pExceptionList )
			)
		{	
			if( in_bIsMasterOnResume || pThisAction->ulPauseCount == 0 )
			{
				TransferToPending( pThisAction );
				iter = m_mmapPausedPending.Erase( iter );
			}
			else
			{
				--(pThisAction->ulPauseCount);
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}
	return eResult;
}

AKRESULT CAkAudioMgr::ResumeNotPausedPendingActionAllExcept(CAkRegisteredObj * in_GameObj, 
															ExceptionList*  in_pExceptionList,
															bool			in_bIsMasterOnResume )
{
	//in_bIsMasterOnResume here is unused for now, but will be useful when supporting pause counting
	AKRESULT eResult = AK_Success;
	AkPendingAction*	pThisAction = NULL;


	// if we've got it then move it
	AkMultimapPending::Iterator iter = m_mmapPending.Begin();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		CAkAction* pAction = pThisAction->pAction;
		if(
			( in_GameObj == NULL || pThisAction->GameObj() == in_GameObj )
			&&
			!IsAnException( pAction, in_pExceptionList )
			)
		{	 
			if( pAction->ActionType() == AkActionType_PlayAndContinue )
			{
				CAkActionPlayAndContinue* pActionPAC = static_cast<CAkActionPlayAndContinue*>( pAction );
				pActionPAC->Resume();
			}
		}
		++iter;
	}
	return eResult;
}
//-----------------------------------------------------------------------------
// Name: RemovePendingAction
// Desc: remove actions(in_ulElementID) from the pending list.
//
// Parameters:
//	AkUniqueID in_ulElementID
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::RemovePendingAction(AkUniqueID in_ulElementID)
{
	AKRESULT	eResult = AK_Success;

	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		AkPendingAction* pPending = (*iter).item;
		if( IsElementOf( in_ulElementID, pPending->pAction->ElementID() ))
		{
			NotifyDelayAborted( pPending, false );

			iter = FlushPendingItem( pPending, m_mmapPending, iter );
		}
		else
		{
			++iter;
		}
	}
	return eResult;
}

//-----------------------------------------------------------------------------
// Name: RemovePausedPendingAction
// Desc: remove paused pending actions(in_ulElementID) from the list.
//
// Parameters:
//	AkUniqueID in_ulElementID
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::RemovePausedPendingAction( AkUniqueID in_ulElementID )
{
	AKRESULT eResult = AK_Success;

	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		if( IsElementOf( in_ulElementID, (*iter).key ) )
		{
			AkPendingAction* pPending = (*iter).item;

			NotifyDelayAborted( pPending, true );

			iter = FlushPendingItem( pPending, m_mmapPausedPending, iter );
		}
		else
		{
			++iter;
		}
	}
	return eResult;
}

//-----------------------------------------------------------------------------
// Name: RemoveAllPausedPendingAction
// Desc:
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::RemoveAllPausedPendingAction()
{
	if ( m_mmapPausedPending.IsInitialized() )
	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			AkPendingAction* pPending = (*iter).item;

			NotifyDelayAborted( pPending, true );

			iter = FlushPendingItem( pPending, m_mmapPausedPending, iter );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: RemoveAllPendingAction
// Desc:
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::RemoveAllPendingAction()
{
	if ( m_mmapPending.IsInitialized() )
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			AkPendingAction* pPending = (*iter).item;
			
			NotifyDelayAborted( pPending, false );

			iter = FlushPendingItem( pPending, m_mmapPending, iter );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: RemoveAllPreallocAndReferences
// Desc: Message queue clean-up before destroying the Audio Manager.
//		 Free command queue's pre-allocated and pre-referenced items (that is, that were allocated 
//		 or referenced by the game thread, in AkAudioLib). 
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::RemoveAllPreallocAndReferences()
{
	AkAutoLock<CAkLock> protectQueuePurge( m_queueLock );

	while(!m_MsgQueue.IsEmpty())
	{
		AkQueuedMsg * pItem = (AkQueuedMsg *) m_MsgQueue.BeginRead();
		AKASSERT( pItem );

		AkQueuedMsg Item;
		memcpy( &Item, pItem, pItem->size ); // DO NOT CHANGE TO AkMemCpy, it would affect performance.

		m_MsgQueue.EndRead( pItem->size );

		switch ( Item.type )
		{
		case QueuedMsgType_RegisterGameObj:
			if ( Item.reggameobj.pMonitorData )
			{
				MONITOR_FREESTRING( Item.reggameobj.pMonitorData );
			}
			break;

		case QueuedMsgType_Event:
			
			AKASSERT( Item.event.Event );
			g_pPlayingMgr->RemoveItemActiveCount( Item.event.PlayingID );
			Item.event.Event->Release();
			break;

		/// TODO: Add cases for messages that need special cleanup.
		}
	}
}

//-----------------------------------------------------------------------------
// Name: NotifyDelayStarted
// Desc:
//
// Parameters:
//	AkPendingAction* in_pPending : Point to a pending action.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyDelayStarted(AkPendingAction* in_pPending)
{
	NotifyDelay( in_pPending, AkMonitorData::NotificationReason_Delay_Started, false );
}

//-----------------------------------------------------------------------------
// Name: NotifyDelayAborted
// Desc:
//
// Parameters:
//	AkPendingAction* in_pPending : Point to a pending action.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyDelayAborted( AkPendingAction* in_pPending, bool in_bWasPaused )
{
	NotifyDelay(in_pPending, AkMonitorData::NotificationReason_Delay_Aborted, in_bWasPaused );
	g_pPlayingMgr->RemoveItemActiveCount( in_pPending->UserParam.PlayingID );
}

//-----------------------------------------------------------------------------
// Name: NotifyDelayEnded
// Desc:
//
// Parameters:
//	AkPendingAction* in_pPending : Point to a pending action.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyDelayEnded(AkPendingAction* in_pPending, bool in_bWasPaused )
{
	NotifyDelay(in_pPending, AkMonitorData::NotificationReason_Delay_Ended, in_bWasPaused );
}

//-----------------------------------------------------------------------------
// Name: NotifyImminentAborted
// Desc:
//
// Parameters:
//	AkPendingAction* in_pPending : Point to a pending action.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyImminentAborted( AkPendingAction* in_pPending )
{
	MONITOR_OBJECTNOTIF( in_pPending->UserParam.PlayingID, in_pPending->GameObjID(), in_pPending->UserParam.CustomParam, AkMonitorData::NotificationReason_ContinueAborted, CAkCntrHist(), in_pPending->pAction->ID(), 0 );
	g_pPlayingMgr->RemoveItemActiveCount( in_pPending->UserParam.PlayingID );
}

//-----------------------------------------------------------------------------
// Name: NotifyDelay
// Desc: 
//
// Parameters:
//	AkPendingAction*				  in_pPending : Point to a pending action.
//	AkMonitorData::NotificationReason in_Reason   :
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyDelay(AkPendingAction* in_pPending, 
								AkMonitorData::NotificationReason in_Reason,
								bool in_bWasPaused )
{
	CAkCntrHist HistArray;
	if( in_bWasPaused )
	{
		MONITOR_OBJECTNOTIF( in_pPending->UserParam.PlayingID, in_pPending->GameObjID(), in_pPending->UserParam.CustomParam, AkMonitorData::NotificationReason_Pause_Aborted, HistArray, in_pPending->pAction->ID(), 0 );
	}

	switch ( in_pPending->pAction->ActionType() )
	{
	case AkActionType_Duck:
		// WG-4697: Don't notify if the action is a ducking action. 
		break;
	case AkActionType_PlayAndContinue:
		if(!static_cast<CAkActionPlayAndContinue*>(in_pPending->pAction)->NeedNotifyDelay() )
		{
			//Do not notify in the case of a cross-fade
			if(in_Reason == AkMonitorData::NotificationReason_Delay_Aborted)
			{
				in_Reason = AkMonitorData::NotificationReason_ContinueAborted;
			}
			else
			{
				break;
			}
		}
		//no break here
	case AkActionType_Play:
		static_cast<CAkActionPlay*>(in_pPending->pAction)->GetHistArray( HistArray );

		//no break here
	default:
		MONITOR_OBJECTNOTIF( in_pPending->UserParam.PlayingID, in_pPending->GameObjID(), in_pPending->UserParam.CustomParam, in_Reason, HistArray, in_pPending->pAction->ID(), 0 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Name: GetActionMatchingPlayingID
// Desc:
//
// Parameters:
//	AkPlayingID in_PlayingID
//
// Return: 
//	AkPendingAction * : Pointer to pending action.
//-----------------------------------------------------------------------------
AkPendingAction* CAkAudioMgr::GetActionMatchingPlayingID(AkPlayingID in_PlayingID)
{
	for( AkMultimapPending::Iterator iter = m_mmapPending.Begin(); iter != m_mmapPending.End(); ++iter )
	{
		if( (*iter).item->UserParam.PlayingID == in_PlayingID )
		{
			return (*iter).item;
		}
	}
	for( AkMultimapPausedPending::Iterator iter = m_mmapPausedPending.Begin(); iter != m_mmapPausedPending.End(); ++iter )
	{
		if( (*iter).item->UserParam.PlayingID == in_PlayingID)
		{
			return (*iter).item;
		}
	}
	return NULL;
}

#ifndef AK_OPTIMIZED
//-----------------------------------------------------------------------------
// Name: StopAction
// Desc:
//
// Parameters:
//	AkUniqueID in_ActionID
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::StopAction(AkUniqueID in_ActionID)
{
	AKRESULT	eResult = AK_Success;

	AkPendingAction* pPending;

	for( AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx(); iter != m_mmapPending.End(); )
	{
		pPending = (*iter).item;

		if( pPending->pAction->ID() == in_ActionID )
		{
			NotifyDelayAborted( pPending, false );
			iter = FlushPendingItem( pPending, m_mmapPending, iter );
		}
		else
			++iter;
	}

	for( AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx(); iter != m_mmapPausedPending.End(); )
	{
		pPending = (*iter).item;

		if( pPending->pAction->ID() == in_ActionID )
		{
			NotifyDelayAborted( pPending, true );
			iter = FlushPendingItem( pPending, m_mmapPausedPending, iter );
		}
		else
			++iter;
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: PauseAction
// Desc:
//
// Parameters:
//	AkUniqueID in_ActionID
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::PauseAction(AkUniqueID in_ActionID)
{
	AKRESULT eResult = AK_Success;

	AkPendingAction*	pThisAction;
	CAkAction*			pAction;

	AkMultimapPausedPending::Iterator iterPause = m_mmapPausedPending.Begin();
	while( iterPause != m_mmapPausedPending.End() )
	{
		pThisAction = (*iterPause).item;
		pAction = pThisAction->pAction;
		AKASSERT(pAction);

		// is it ours ? we don't pause pending resumes or we'll get stuck
		if(in_ActionID == pAction->ID())
		{	
			++(pThisAction->ulPauseCount);
		}
		++iterPause;
	}

	// scan'em all
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		pAction = pThisAction->pAction;
		AKASSERT(pAction);

		// is it ours ? we don't pause pending resumes or we'll get stuck
		if(in_ActionID == pAction->ID())
		{	
			InsertAsPaused( pAction->ElementID(), pThisAction );
			iter = m_mmapPending.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
	return eResult;
}

//-----------------------------------------------------------------------------
// Name: ResumeAction
// Desc:
//
// Parameters:
//	AkUniqueID in_ActionID
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::ResumeAction(AkUniqueID in_ActionID)
{
	AKRESULT eResult = AK_Success;
	AkPendingAction*	pThisAction;

	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		pThisAction = (*iter).item;
		if( pThisAction->pAction->ID() == in_ActionID )
		{
			if( pThisAction->ulPauseCount == 0 )
			{
				TransferToPending( pThisAction );
				iter = m_mmapPausedPending.Erase( iter );
			}
			else
			{
				--(pThisAction->ulPauseCount);
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}
	return eResult;
}

#endif // AK_OPTIMIZED

//-----------------------------------------------------------------------------
// Name: ClearPendingItems
// Desc:
//
// Parameters:
// AkPlayingID in_PlayingID
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::ClearPendingItems( AkPlayingID in_PlayingID )
{
	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			if( pPendingAction->UserParam.PlayingID == in_PlayingID )
			{
				NotifyDelayAborted( pPendingAction, false );

				iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}
	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			if( pPendingAction->UserParam.PlayingID == in_PlayingID)
			{
				NotifyDelayAborted( pPendingAction, true );

				iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: ClearPendingItemsExemptOne
// Desc:
//
// Parameters:
//	AkPlayingID in_PlayingID
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::ClearPendingItemsExemptOne(AkPlayingID in_PlayingID)
{
	bool bExempt = true;
	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			if( pPendingAction->UserParam.PlayingID == in_PlayingID )
			{
				if(bExempt)
				{
					NotifyDelayEnded( pPendingAction );
					g_pPlayingMgr->RemoveItemActiveCount( pPendingAction->UserParam.PlayingID );
					bExempt = false;
				}
				else
				{
					NotifyDelayAborted( pPendingAction, false );
				}
				iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}
	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			if(pPendingAction->UserParam.PlayingID == in_PlayingID)
			{
				if(bExempt)
				{
					// This is a special situation, the notification use true as optionnal flag, 
					// telling that the sound is unpaused and continuated in one shot
					NotifyDelayEnded( pPendingAction, true );
					g_pPlayingMgr->RemoveItemActiveCount( pPendingAction->UserParam.PlayingID );
					bExempt = false;
				}
				else
				{
					NotifyDelayAborted( pPendingAction, true );
				}

				iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: ClearCrossFadeOccurence
// Desc:
//
// Parameters:
//	 CAkPBI* in_pPBIToCheck :
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::ClearCrossFadeOccurence( CAkPBI* in_pPBIToCheck )
{
	CAkAction* pAction;
	//do not use in_pPBIToCheck here, only for comparaison purpose.
	for( AkMultimapPending::Iterator iter = m_mmapPending.Begin(); iter != m_mmapPending.End(); ++iter )
	{
		pAction = (*iter).item->pAction;
		if( pAction->ActionType() == AkActionType_PlayAndContinue )
		{
			static_cast<CAkActionPlayAndContinue*>( pAction )->UnsetFadeBack( in_pPBIToCheck );
		}
	}
	for( AkMultimapPausedPending::Iterator iter = m_mmapPausedPending.Begin(); iter != m_mmapPausedPending.End(); ++iter )
	{
		pAction = (*iter).item->pAction;
		if(pAction->ActionType() == AkActionType_PlayAndContinue)
		{
			static_cast<CAkActionPlayAndContinue*>( pAction )->UnsetFadeBack( in_pPBIToCheck );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: IsAnException
// Desc:
//
// Parameters:
//	None.
//
// Return: 
//	true  : An exception.
//	false : Not an exception.
//-----------------------------------------------------------------------------
bool CAkAudioMgr::IsAnException( CAkAction* in_pAction, ExceptionList* in_pExceptionList )
{
	AKASSERT(in_pAction);

	if( !in_pExceptionList )
	{
		return false;
	}

	bool l_bCheckedBus = false;

	CAkAudioNode* pBusNode = NULL;

	if(in_pAction->ElementID())
	{
		CAkAudioNode* pNode = static_cast<CAkAudioNode*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef(in_pAction->ElementID()) );
		CAkAudioNode* pNodeInitial = pNode;

		while(pNode != NULL)
		{
			for( ExceptionList::Iterator iter = in_pExceptionList->Begin(); iter != in_pExceptionList->End(); ++iter )
			{
				if( *iter == pNode->ID() )
				{
					pNodeInitial->Release();
					return true;
				}
			}

			if( !l_bCheckedBus )
			{
				pBusNode = pNode->ParentBus();
				if(pBusNode)
				{
					l_bCheckedBus = true;
				}
			}

			pNode = pNode->Parent();
		}
		while(pBusNode != NULL)
		{
			for( ExceptionList::Iterator iter = in_pExceptionList->Begin(); iter != in_pExceptionList->End(); ++iter )
			{
				if( (*iter) == pBusNode->ID() )
				{
					pNodeInitial->Release();
					return true;
				}
			}

			pBusNode = pBusNode->ParentBus();
		}

		if ( pNodeInitial )
			pNodeInitial->Release();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Name: IsElementOf
// Desc:
//
// Parameters:
// AkUniqueID in_TargetID	:
// AkUniqueID in_IDToCheck	:
//
// Return: 
//	true  : is element of.
//	false : not an element of.
//-----------------------------------------------------------------------------
bool CAkAudioMgr::IsElementOf( AkUniqueID in_TargetID, 
								 AkUniqueID in_IDToCheck )
{
	
	bool bIsElementOf = false;

	if(in_TargetID && in_IDToCheck)
	{
		if(in_TargetID == in_IDToCheck)//most situations
		{
			bIsElementOf = true;
		}
		else
		{
			AKASSERT(g_pIndex);
			CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(in_IDToCheck);
			if(pNode)
			{
				CAkAudioNode* pNodeInitial = pNode;
				CAkAudioNode* pBus = pNode->ParentBus();

				for( pNode = pNode->Parent(); pNode ; pNode = pNode->Parent() )
				{
					if( in_TargetID == pNode->ID() )
					{
						bIsElementOf = true;
						break;
					}
					if( pBus == NULL )
					{
						pBus = pNode->ParentBus();
					}
				}
				if( !bIsElementOf )
				{
					//checking bus
					for( /*noinit*/ ; pBus ; pBus = pBus->ParentBus() )
					{
						if( in_TargetID == pBus->ID() )
						{
							bIsElementOf = true;
							break;
						}
					}
				}

				pNodeInitial->Release();
			}
			
		}
	}

	return bIsElementOf;
}
//-----------------------------------------------------------------------------
// Name: Start
// Desc: Execute behavioural side and lower engine side.
//
// Parameters:
//	None.
//
// Return: 
//	AK_Success:	VPL executed. 
//  AK_Fail:		Failed to execute a VPL.
//-----------------------------------------------------------------------------
//Start the AudioThread
AKRESULT CAkAudioMgr::Start()
{
	if ( AkCreateEvent( m_hEventMgrThreadDrainEvent ) == AK_Success )
		return m_audioThread.Start();
	return AK_Fail;
}

//Stop the AudioThread 
void CAkAudioMgr::Stop()
{
	m_audioThread.Stop();
	AkDestroyEvent( m_hEventMgrThreadDrainEvent );
}

AkPendingAction::AkPendingAction( CAkRegisteredObj * in_pGameObj )
	: pGameObj( in_pGameObj )
{
	if ( pGameObj )
		pGameObj->AddRef();
}

AkPendingAction::~AkPendingAction()
{
	if ( pGameObj )
		pGameObj->Release();
}

AkGameObjectID AkPendingAction::GameObjID() 
{ 
	return pGameObj ? pGameObj->ID() : AK_INVALID_GAME_OBJECT; 
}

void AkPendingAction::TransUpdateValue( TransitionTargetTypes in_eTargetType, TransitionTarget in_unionValue, bool in_bIsTerminated )
{
	AKASSERT( g_pAudioMgr );
	if( pAction->ActionType() == AkActionType_PlayAndContinue )
	{
		CAkActionPlayAndContinue* pActionPAC = static_cast<CAkActionPlayAndContinue*>( pAction );
		AKASSERT(g_pTransitionManager);	 

		TransitionTargets l_Target = (TransitionTargets)(in_eTargetType & TransTarget_TargetMask);
		switch( l_Target )
		{
		case TransTarget_Stop:
		case TransTarget_Play:
			if(in_bIsTerminated)
			{
				pActionPAC->m_PBTrans.pvPSTrans = NULL;
				pActionPAC->m_PBTrans.bIsPSTransFading = false;

				if( l_Target == TransTarget_Stop )
				{
					g_pAudioMgr->StopPending( this );
				}
			}
			break;
		case TransTarget_Pause:
		case TransTarget_Resume:
			if(in_bIsTerminated)
			{
				pActionPAC->m_PBTrans.pvPRTrans = NULL;
				pActionPAC->m_PBTrans.bIsPRTransFading = false;

				if( l_Target == TransTarget_Pause )
				{
					g_pAudioMgr->PausePending( this );
				}

				pActionPAC->SetPauseCount( 0 );
			}
			break;
		default:
			AKASSERT(!"Unsupported data type");
			break;
		}
	}
	else
	{
		// Should not happen, only PlayAndContinue Actions can have transitions.
		AKASSERT( pAction->ActionType() == AkActionType_PlayAndContinue );
	}
}

AKRESULT CAkAudioMgr::PausePending( AkPendingAction* in_pPA )
{
	AKRESULT	eResult = AK_Success;

	if( in_pPA )
	{
		AkPendingAction*	pThisAction = NULL;
		CAkAction*			pAction		= NULL;

		// scan'em all
		for( AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx(); iter != m_mmapPending.End(); ++iter )
		{
			pThisAction = (*iter).item;
			pAction = pThisAction->pAction;

			if( pThisAction == in_pPA )
			{	
				AkUInt32 l_ulPauseCount = 0;

				if( pAction->ActionType() == AkActionType_PlayAndContinue )
				{
					l_ulPauseCount = static_cast<CAkActionPlayAndContinue*>(pAction)->GetPauseCount() - 1;
					static_cast<CAkActionPlayAndContinue*>(pAction)->SetPauseCount( 0 );
				}

				InsertAsPaused( pAction->ElementID(), pThisAction, l_ulPauseCount );
				m_mmapPending.Erase( iter );
				return eResult;
			}
		}
		
		// scan'em all
		for( AkMultimapPausedPending::Iterator iter = m_mmapPausedPending.Begin(); iter != m_mmapPausedPending.End(); ++iter )
		{
			pThisAction = (*iter).item;
			pAction = pThisAction->pAction;

			if( pThisAction == in_pPA )
			{	
				if( pAction->ActionType() == AkActionType_PlayAndContinue )
				{
					(pThisAction->ulPauseCount) += static_cast<CAkActionPlayAndContinue*>(pAction)->GetPauseCount();
				}
				else
				{
					++(pThisAction->ulPauseCount);
				}
				return eResult;
			}
		}
	}
	return eResult;
}

AKRESULT CAkAudioMgr::StopPending( AkPendingAction* in_pPA )
{
	AKRESULT	eResult = AK_Success;

	if( in_pPA )
	{
		CAkAction* pAction = NULL;
		
		for( AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx(); iter != m_mmapPausedPending.End(); ++iter )
		{
			pAction = (*iter).item->pAction;
			if( in_pPA == (*iter).item )
			{
				NotifyDelayAborted( in_pPA, true );
				FlushPendingItem( in_pPA, m_mmapPausedPending, iter );
				break;
			}
		}
		
		for( AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx(); iter != m_mmapPending.End(); ++iter )
		{
			pAction = (*iter).item->pAction;
			if( in_pPA == (*iter).item )
			{
				NotifyDelayAborted( in_pPA, false );
				FlushPendingItem( in_pPA, m_mmapPending, iter );
				break;
			}
		}
	}
	return eResult;
}
