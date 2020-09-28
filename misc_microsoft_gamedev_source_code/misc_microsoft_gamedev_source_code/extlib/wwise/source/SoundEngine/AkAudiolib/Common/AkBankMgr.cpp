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
// AkBankMgr.cpp
//
// The Bank Creator is simply creating a dummy bank usable by the Audiolib
// This bank creator, as it is implemented now, is not intended 
// to be used but for development purpose
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#include "AkBankMgr.h"
#include "AkCommon.h"
#include "AkEvent.h"
#include "AkAction.h"
#include "AkSound.h"
#include "AkAudioLibIndex.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkMonitor.h"
#include "AkActorMixer.h"
#include "AkState.h"
#include "AkBus.h"
#include "AkPathManager.h"
#include "AkPath.h"
#include "AudiolibDefs.h"
#include "AkStateMgr.h"
#include "AkSwitchCntr.h"
#include "AkRanSeqCntr.h"
#include "AkLayerCntr.h"
#include "AkEnvironmentsMgr.h"
#include "AkAudioLib.h"
#include "AkAudioMgr.h"
#include "AkRTPCMgr.h"
#include "AkBusCtx.h"
#include "AkAttenuationMgr.h"
#include "AkDialogueEvent.h"
#include "AkFeedbackBus.h"
#include "AkFeedbackNode.h"
#include "AkProfile.h"

extern AkMemPoolId	g_DefaultPoolId;
extern CAkBusCtx g_MasterBusCtx;
extern CAkBus*	g_pMasterBus;
extern AkExternalBankHandlerCallback g_pExternalBankHandlerCallback;
extern AkPlatformInitSettings g_PDSettings;
extern AkInitSettings g_settings;

#define AK_READ_BUFFER_SIZE (32*1024)	// 32k, Arbitrarily should be optimal size read and be platform specific

#define AK_BANK_DEFAULT_LIST_SIZE 10

#define HOOKEDPOOLID AkMediaEntry::GetPrepareMemPoolID()
#define ALLOCWITHHOOK( _size_, _Alignment_ )	AK::MemoryMgr::Malign( HOOKEDPOOLID, _size_, _Alignment_ )
#define FREEWITHHOOK( _ptr_tofree_ )			AK::MemoryMgr::Falign( HOOKEDPOOLID, _ptr_tofree_ )

#define REAL_HEADER_SIZE ( sizeof( AkSubchunkHeader ) + sizeof( AkBankHeader ) )

// Rounding REAL_HEADER_SIZE up to AK_BANK_PLATFORM_DATA_ALIGNMENT boundary.
#define OFFSET_PLUS_ALIGN_MUNUS_ONE ( REAL_HEADER_SIZE + AK_BANK_PLATFORM_DATA_ALIGNMENT - 1 )
#define BANK_DATA_OFFSET ( OFFSET_PLUS_ALIGN_MUNUS_ONE - ( OFFSET_PLUS_ALIGN_MUNUS_ONE % AK_BANK_PLATFORM_DATA_ALIGNMENT ) )

AkThread CAkBankMgr::m_BankMgrThread;

CAkUsageSlot::CAkUsageSlot( AkUniqueID in_BankID, AkMemPoolId in_memPoolId, AkInt32 in_mainRefCount, AkInt32 in_prepareRefCount )
	:m_pData( NULL )
    ,m_memPoolId( in_memPoolId )
    ,m_bIsInternalPool(false)
	,m_iRefCount( in_mainRefCount )
	,m_iPrepareRefCount( in_prepareRefCount )
	,m_BankID( in_BankID )
	,m_pfnBankCallback( NULL )
	,m_pCookie( NULL )
	,m_uNumLoadedItems( 0 )
	,m_paLoadedMedia( NULL )
	,m_bWasLoadedAsABank( in_mainRefCount != 0 )
	,m_bWasLoadedFromMemory( false )
	,m_uLoadedDataSize( 0 )
{
}

CAkUsageSlot::~CAkUsageSlot()
{
}

void CAkUsageSlot::AddRef()
{
	// XXX; 
	// TODO
	// maybe we should increment only if one of the two count is non zero.
	// Which also suggest the AddRef could fail... which would be an issue too. 
	// ...kind of in the middle of some problem here...
	// Must check where the AddRefs are done.
	AkInterlockedIncrement( &m_iRefCount );
}

void CAkUsageSlot::AddRefPrepare()
{
	AkInterlockedIncrement( &m_iPrepareRefCount );
}

void CAkUsageSlot::Release( bool in_bSkipNotification )
{
	// XXX; should be locked???
	AkInt32 iNewRefCount = AkInterlockedDecrement( &m_iRefCount );
	AKASSERT( iNewRefCount >= 0 );

	if ( iNewRefCount <= 0 )
	{
		Unload();
		g_pBankManager->UnloadMedia( this );

		if( m_iPrepareRefCount <= 0 )
		{
			g_pBankManager->EnsureRemoveLoadedBankItem( m_BankID );
			MONITOR_LOADEDBANK( this, true );

			if( !in_bSkipNotification )
			{
				UnloadCompletionNotification();
			}
			
			AkDelete( g_DefaultPoolId, this );
		}
		else 
		{
			MONITOR_LOADEDBANK( this, false );
			if( !in_bSkipNotification )
			{
				UnloadCompletionNotification();
			}
		}
	}
}

void CAkUsageSlot::ReleasePrepare()
{	
	// XXX; should be locked???
	AkInt32 iNewRefCount = AkInterlockedDecrement( &m_iPrepareRefCount );
	AKASSERT( iNewRefCount >= 0 );

	if ( iNewRefCount <= 0 )
	{
		if( m_iRefCount <= 0 )
		{
			UnloadPre();
			RemoveContent();
			Unload();
			g_pBankManager->EnsureRemoveLoadedBankItem( m_BankID );
			MONITOR_LOADEDBANK( this, true );

			AkDelete( g_DefaultPoolId, this );
		}
	}
}

/***************************
// WARNING
//
// This function must be called by the audiothread inside of the global lock
****************************/
void CAkUsageSlot::StopContent()
{
	AkListLoadedItem::Iterator iter = m_listLoadedItem.Begin();
	while( iter != m_listLoadedItem.End() )
	{
		// At this point, only sounds, track and music objects are remaining
		CAkAudioNode* pNode = static_cast<CAkAudioNode*>(*iter);

		g_pAudioMgr->StopPendingAction( pNode->ID(), NULL );	

		pNode->Stop();
		pNode->Release();

		++iter;
	}
	m_listLoadedItem.Term();
}

// This is the version of StopContent used when cleaning up a failed bank load
void CAkUsageSlot::RemoveContent()
{
	AkListLoadedItem::Iterator iter = m_listLoadedItem.Begin();
	while( iter != m_listLoadedItem.End() )
	{
		// At this point, only sounds, track and music objects are remaining
		CAkAudioNode* pNode = static_cast<CAkAudioNode*>(*iter);
		{
			CAkFunctionCritical SpaceSetAsCritical;
			pNode->Release();
		}

		++iter;
	}
	m_listLoadedItem.Term();
}

void CAkUsageSlot::UnloadPre()
{
	for( AkListLoadedItem::Iterator iter = m_listLoadedItem.Begin(); iter != m_listLoadedItem.End();  )
	{
		CAkIndexable*	pIndexable = *iter;

		switch( pIndexable->Category() )
		{
		case ObjCategory_Sound:
		case ObjCategory_FeedbackNode:
		case ObjCategory_Track:
			// no break here intentionnal
		case ObjCategory_MusicNode: // All music nodes must be stopped, not only Tracks : see : WG-8287
			++iter;
			break;

		default:
			{
				CAkFunctionCritical SpaceSetAsCritical; // destruction of an item needs to be protected
				pIndexable->Release();
			}
			m_listLoadedItem.EraseSwap( iter );
			break;
		}
	}
}

void CAkUsageSlot::Unload()
{
	if( m_pData )
	{
		AK_PERF_DECREMENT_BANK_MEMORY( m_uLoadedDataSize );
		AKASSERT( m_memPoolId != AK_INVALID_POOL_ID );
		if ( AK::MemoryMgr::GetPoolAttributes( m_memPoolId ) & AkBlockMgmtMask )
			AK::MemoryMgr::ReleaseBlock( m_memPoolId, m_pData );
		else
			AkFree( m_memPoolId, m_pData );
		// If it was an internally created pool, it should be empty now and we destroy it.
		m_pData = NULL;
		if ( m_bIsInternalPool )
		{
			AKVERIFY( AK::MemoryMgr::DestroyPool( m_memPoolId ) == AK_Success );
			m_memPoolId = AK_INVALID_POOL_ID;
		}
	}
}

void CAkUsageSlot::UnloadCompletionNotification()
{
	MONITOR_BANKNOTIF( m_BankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankUnloaded );
	// Notify user.

	if( m_pfnBankCallback )
	{
		g_pBankManager->DoCallback( m_pfnBankCallback, m_BankID, AK_Success, m_memPoolId, m_pCookie );
		m_pfnBankCallback = NULL;// to make sure the notification is sent only once
	}
}

CAkBankMgr::CAkBankMgr()
:m_BankIDBeingProcessed( 0 )
,m_bCancelFlag( false )
,m_bIsFirstBusLoaded( false )
,m_bStopThread( false )
,m_bFeedbackInBank( false )
{
	AkClearEvent( m_eventQueue );
	AkClearThread(&m_BankMgrThread);
}

CAkBankMgr::~CAkBankMgr()
{
}

AKRESULT CAkBankMgr::Init()
{
	AKRESULT eResult = AK_Success;

	AkMediaEntry::SetPrepareMemPoolID( g_settings.uPrepareEventMemoryPoolID );

	if(eResult == AK_Success)
	{
		eResult = m_BankReader.Init( this );
	}
	if(eResult == AK_Success)
	{
		eResult = m_MediaHashTable.Init( g_DefaultPoolId );
	}
	if(eResult == AK_Success)
	{
		eResult = m_BankIDToFileName.Init( g_DefaultPoolId );
	}
	if(eResult == AK_Success)
	{
		eResult = m_BankQueue.Init( AK_BANK_DEFAULT_LIST_SIZE ,AK_NO_MAX_LIST_SIZE,g_DefaultPoolId );
	}
	if(eResult == AK_Success)
	{
		eResult = StartThread();
	}

	return eResult;
}

AKRESULT CAkBankMgr::Term()
{
	StopThread();
	m_BankQueue.Term();

	UnloadAll();

	m_ListLoadedBanks.Term();

	AKASSERT( m_MediaHashTable.Length() == 0 );

	m_MediaHashTable.Term();

	FlushFileNameTable();

	m_BankReader.Term();

	return AK_Success;
}

void CAkBankMgr::FlushFileNameTable()
{
	if( m_BankIDToFileName.IsInitialized() )
	{
		for( AkIDtoStringHash::Iterator iter = m_BankIDToFileName.Begin(); iter != m_BankIDToFileName.End(); ++iter )
		{
			AKASSERT( iter.pItem->Assoc.item );
			AkFree( g_DefaultPoolId, iter.pItem->Assoc.item );
		}
	}
	m_BankIDToFileName.Term();
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkBankMgr::StartThread()
{
	if(AkIsValidThread(&m_BankMgrThread))
	{
		AKASSERT( !"Wrong thread trying to start another thread." );
		return AK_Fail;
	}

	m_bStopThread = false;

	if ( AkCreateEvent( m_eventQueue ) != AK_Success )
	{
		AKASSERT( !"Could not create event required to start BankMgr thread." );
		return AK_Fail;
	}

    AkCreateThread(	BankThreadFunc,		// Start address
					this,				// Parameter
					&g_PDSettings.threadBankManager, // Thread properties 
					&m_BankMgrThread,
					"AK::BankManager" );	// Name
	if ( !AkIsValidThread(&m_BankMgrThread) )
	{
		AKASSERT( !"Could not create bank manager thread" );
		return AK_Fail;
	}
	return AK_Success;
}
//====================================================================================================
//====================================================================================================
void CAkBankMgr::StopThread()
{
	m_bStopThread = true;
	if ( AkIsValidThread( &m_BankMgrThread ) )
	{
		// stop the eventMgr thread
		AkSignalEvent( m_eventQueue );
		AkWaitForSingleThread( &m_BankMgrThread );

		AkCloseThread( &m_BankMgrThread );
	}
	AkDestroyEvent( m_eventQueue );
}
//====================================================================================================
//====================================================================================================
AK_DECLARE_THREAD_ROUTINE( CAkBankMgr::BankThreadFunc )
{
	CAkBankMgr& rThis = *reinterpret_cast<CAkBankMgr*>(AK_THREAD_ROUTINE_PARAMETER);

	while(true)
	{
		AkWaitForEvent( rThis.m_eventQueue );

		if ( rThis.m_bStopThread )
			break;
			
		// Something in the queue!
		rThis.ExecuteCommand();
	}

	AkExitThread(AK_RETURN_THREAD_OK);
}

AKRESULT CAkBankMgr::QueueBankCommand( AkBankQueueItem in_Item  )
{
	AkAutoLock<CAkLock> gate(m_queueLock);

	// Cancellation phase 1
	if( in_Item.eType == QueueItemUnload && m_BankIDBeingProcessed == in_Item.load.BankID )
	{
		m_bCancelFlag = true;
	}

	// Cancellation phase 2
	AkBankQueue::IteratorEx iter = m_BankQueue.BeginEx();
	while( iter != m_BankQueue.End() )
	{
		AkBankQueueItem&  rItem = *iter;
		if( rItem.load.BankID == in_Item.load.BankID )
		{
			if( rItem.eType == QueueItemLoad && in_Item.eType == QueueItemUnload )
			{
				MONITOR_BANKNOTIF( rItem.load.BankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankLoadCancelled );
				NotifyCompletion( rItem, AK_Cancelled );
				iter = m_BankQueue.Erase( iter );
			}
			else if( rItem.eType == QueueItemUnload && in_Item.eType == QueueItemLoad  )
			{
				MONITOR_BANKNOTIF( rItem.load.BankID,  AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankUnloadCancelled );
				NotifyCompletion( rItem, AK_Cancelled );
				iter = m_BankQueue.Erase( iter );
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
	AKRESULT eResult = AK_Success;
	if( in_Item.callbackInfo.pfnBankCallback )
		eResult = m_CallbackMgr.AddCookie( in_Item.callbackInfo.pCookie );

	if(eResult == AK_Success)
	{
		eResult = m_BankQueue.AddLast( in_Item ) ? AK_Success : AK_Fail;
		if( in_Item.callbackInfo.pfnBankCallback && eResult != AK_Success )
			m_CallbackMgr.RemoveOneCookie( in_Item.callbackInfo.pCookie );
	}
	if(eResult == AK_Success)
	{
		if( in_Item.eType == QueueItemLoad )
		{
			MONITOR_BANKNOTIF( in_Item.load.BankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankLoadRequestReceived );
		}
		else if ( in_Item.eType == QueueItemUnload )
		{
			MONITOR_BANKNOTIF( in_Item.load.BankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankUnloadRequestReceived );
		}
		else if ( in_Item.eType == QueueItemPrepareEvent )
		{
			MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_PrepareEventRequestReceived, in_Item.prepare.numEvents );
		}
		else if ( in_Item.eType == QueueItemUnprepareEvent )
		{
			MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_UnPrepareEventRequestReceived, in_Item.prepare.numEvents );
		}
		else if ( in_Item.eType == QueueItemSupportedGameSync )
		{
			if( !in_Item.gameSync.bSupported )
			{
				MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_UnPrepareGameSyncRequested, in_Item.gameSync.uNumGameSync );
			}
			else
			{
				MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_PrepareGameSyncRequested, in_Item.gameSync.uNumGameSync );
			}
		}
		else if( in_Item.eType == QueueItemUnprepareAllEvents )
		{
			MONITOR_PREPARENOTIFREQUESTED( AkMonitorData::NotificationReason_ClearPreparedEventsRequested, 0 );
		}
		else if( in_Item.eType == QueueItemClearBanks )
		{
			MONITOR_BANKNOTIF( AK_INVALID_UNIQUE_ID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_ClearAllBanksRequestReceived );
		}
		AkSignalEvent( m_eventQueue );
	}

	return eResult;
}

AKRESULT CAkBankMgr::ExecuteCommand()
{
	AKRESULT eResult = AK_Success;
	while( true )
	{
		m_queueLock.Lock();
		if( m_BankQueue.Length() )
		{
			CAkBankMgr::AkBankQueueItem Item = m_BankQueue.First();
			m_BankQueue.RemoveFirst();
			m_queueLock.Unlock();

			switch ( Item.eType )
			{
			case QueueItemLoad:
				eResult = LoadBankPre( Item );
				break;

			case QueueItemUnload:
				eResult = UnloadBankPre( Item );
				break;

			case QueueItemPrepareEvent:
				eResult = PrepareEvents( Item );
				break;

			case QueueItemUnprepareEvent:
				eResult = UnprepareEvents( Item );
				break;

			case QueueItemSupportedGameSync:
				eResult = PrepareGameSync( Item );
				break;

			case QueueItemUnprepareAllEvents:
				eResult = UnprepareAllEvents( Item );
				break;

			case QueueItemClearBanks:
				eResult = ClearBanksInternal( Item );
				break;
			}
		}
		else //Yes, the queue may be empty even if the semaphore was signaled, because a cancellation may have occurred
		{
			m_queueLock.Unlock();
			break; //exit while( true ) loop
		}
	}

	return eResult;
}

void CAkBankMgr::NotifyCompletion( AkBankQueueItem & in_rItem, AKRESULT in_OperationResult )
{
	AkMemPoolId memPoolID = AK_INVALID_POOL_ID;
	AkUniqueID itemID = AK_INVALID_UNIQUE_ID;

	if( in_rItem.eType == QueueItemLoad || in_rItem.eType == QueueItemUnload )
	{
		memPoolID = in_rItem.load.memPoolId;
		itemID = in_rItem.load.BankID;
	}
	else if( in_rItem.eType == QueueItemSupportedGameSync )
	{
		// nothing, but let this bracket empty, that allows the following else to have no if to check.
	}
	else
	{
		itemID = (in_rItem.prepare.numEvents == 1) ? in_rItem.prepare.eventID : AK_INVALID_UNIQUE_ID;
	}

	DoCallback( in_rItem.callbackInfo.pfnBankCallback, itemID, in_OperationResult, memPoolID, in_rItem.callbackInfo.pCookie );
}

AKRESULT CAkBankMgr::SetFileReader( AkFileID in_FileID, AkUInt32 in_uFileOffset, AkUInt32 in_codecID )
{
	// We are in a prepare Event.
	// If we find the bank name, we use the string, otherwise, we use the file ID directly.

	char** ppString = m_BankIDToFileName.Exists( in_FileID );
	if( ppString )
	{
		AKASSERT( *ppString );
		// Convert the string to W_char and go by string.

		AkTChar wideString[ AK_MAX_PATH ];
		AkAnsiToWideChar( *ppString, AK_MAX_PATH, wideString );

		return m_BankReader.SetFile( wideString, in_uFileOffset );
	}
	else
	{
		if( in_uFileOffset != 0 )
		{
			in_codecID = AKCODECID_BANK;
		}
		return m_BankReader.SetFile( in_FileID, in_uFileOffset, in_codecID );
	}
}

const char* CAkBankMgr::GetBankFileName( AkBankID in_bankID )
{
	char** ppString = m_BankIDToFileName.Exists( in_bankID );

	if( ppString )
		return *ppString;

	return NULL;
}

AKRESULT CAkBankMgr::LoadSoundFromFile( AkSrcTypeInfo& in_rMediaInfo, AkUInt8* io_pData )
{
	m_BankReader.Reset();
	AkUInt32 uFileOffset = in_rMediaInfo.mediaInfo.uFileOffset;
	if( !in_rMediaInfo.mediaInfo.bPrefetch )
	{
		// It is a real in memory sound, so add the Bank data offset.
		uFileOffset += BANK_DATA_OFFSET;
	}
	AKRESULT eResult = SetFileReader( in_rMediaInfo.mediaInfo.uFileID, uFileOffset, CODECID_FROM_PLUGINID( in_rMediaInfo.dwID ) );

	AkUInt32 uReadSize = 0;
	if( eResult == AK_Success )
	{
		eResult = m_BankReader.FillData( io_pData, in_rMediaInfo.mediaInfo.uInMemoryMediaSize, uReadSize
#ifdef RVL_OS
			,true // flush cache range on Wii.
#endif
			);
	}
	if( eResult == AK_Success && in_rMediaInfo.mediaInfo.uInMemoryMediaSize != uReadSize )
			eResult = AK_Fail;

	m_BankReader.CloseFile();

	return eResult;
}

AKRESULT CAkBankMgr::LoadBank( AkBankQueueItem in_Item, CAkUsageSlot *& out_pUsageSlot, bool in_bLoadingMedia )
{
	AKRESULT eResult = AK_InvalidFile;	

	AkUniqueID bankID = in_Item.load.BankID;
	CAkUsageSlot ** ppUsageSlot = m_ListLoadedBanks.Exists( bankID );
	if( ppUsageSlot && (*ppUsageSlot)->WasLoadedAsABank() )
	{
		AKASSERT( in_bLoadingMedia );// If the call is not preparing media, then it should not pass here.
		MONITOR_BANKNOTIF( bankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankAlreadyLoaded );
		return AK_BankAlreadyLoaded;
	}

	m_BankReader.Reset();

	// If not loading media, means we are in a prepare event and the file ID was specified, 
	if( !in_bLoadingMedia )
	{
		eResult = SetFileReader( in_Item.load.BankID, 0, AKCODECID_BANK );
	}
	else if( in_Item.load.bankLoadFlag == AkBankLoadFlag_UsingFileID )
	{
		eResult = m_BankReader.SetFile( in_Item.load.BankID, 0, AKCODECID_BANK );
	}
	else if( in_Item.load.bankLoadFlag == AkBankLoadFlag_InMemory )
	{
		eResult = m_BankReader.SetFile( in_Item.load.pInMemoryBank, in_Item.load.ui32InMemoryBankSize );
	}
	else
	{
		eResult = SetFileReader( bankID, 0, AKCODECID_BANK );
	}
	
	// Getting the bank information from its header
    AkBankHeader BankHeaderInfo = {0};
	if(eResult == AK_Success)
	{
		eResult = ProcessBankHeader( BankHeaderInfo );
	}

	bool l_LoadingOnlyMedia = false;

	bool bDone = false;

	if(eResult == AK_Success)
	{
		if( ppUsageSlot )
		{
			// This will occur only when loading a bank which content has only been loaded by 
			// a PrepareEvent but the media has not been loaded for the whole bank yet.
			AKASSERT( in_bLoadingMedia );
			AKASSERT( (*ppUsageSlot) != NULL );
			AKASSERT( !(*ppUsageSlot)->WasLoadedAsABank() );

			out_pUsageSlot = *ppUsageSlot;
			out_pUsageSlot->AddRef();
			// If the data is still there, lets continue to use it. We dont have to (can't) replace it.
			if( ((*ppUsageSlot)->m_pData) )
			{
				//////////////////////////////////////////////////////////////////////////////////////////////////////
				// Q:
				// Why do we call UnloadCompletionNotification() at this point? Isn't it weird, especially since
				// we are actually LOADING a bank?
				//
				// A:
				// We are in a load bank command actually, and we arrived here in the following situation:
				// - This bank has been previously loaded
				// - This bank is actually tagged as not loaded.
				// - The media Data is there.
				//
				// This bank is tagged as unloaded, which means that an unload bank request has been received 
				// and been successfully processed. But the bank is still loaded as probably a sound is playing
				// one of its media, and holding a reference to the bank media.
				// Since we just addref'ed it, the unload notification will be missing, so we send it just to be 
				// consistent.
				//
				out_pUsageSlot->UnloadCompletionNotification();
				//////////////////////////////////////////////////////////////////////////////////////////////////////

				// Nothing to do, we just stole the bank from the previously loaded bank.
				bDone = true;
			}
			else
			{
				out_pUsageSlot->m_memPoolId = in_Item.load.memPoolId;// The prepare event did not specify a correct ID, let us fix it.
				l_LoadingOnlyMedia = true;
			}
		}
		else 
		{
			if( in_bLoadingMedia )
			{
				out_pUsageSlot = AkNew( g_DefaultPoolId, CAkUsageSlot( bankID, in_Item.load.memPoolId, 1, 0 ) );
			}
			else
			{
				out_pUsageSlot = AkNew( g_DefaultPoolId, CAkUsageSlot( bankID, AK_INVALID_POOL_ID, 0, 1 ) );
			}

			if ( !out_pUsageSlot )
			{
				eResult = AK_Fail;
			}
		}
	}

	// Clearing the Master busses.
	// Why:
	//    The init bank contains the master bus, but it is possible that another
	//    temporary master bus has been added if Wwise has been connected prior to this init bank load bank.
	//    The ID of the Master bus wouldn't match, and the results would be that global events and information about master bus volume and more be wrong.	
	CAkFeedbackBus* pOldFeedbackMasterBus = CAkFeedbackBus::ClearTempMasterBus();

	AkUInt8* pDataBank = NULL;

	// Loading the Bank
	while (!bDone && (eResult == AK_Success))
	{
		if(IsCancelled())
		{
			eResult = AK_Cancelled;
			break;
		}

		AkSubchunkHeader SubChunkHeader;

		AkUInt32 ulTotalRead = 0;
		AkUInt32 ulReadBytes = 0;

		eResult = m_BankReader.FillData(&SubChunkHeader, sizeof(SubChunkHeader), ulReadBytes);
		ulTotalRead += ulReadBytes;
		if(eResult != AK_Success)
		{
			break;
		}
		if(ulTotalRead == sizeof(SubChunkHeader))
		{
			switch( SubChunkHeader.dwTag )
			{
			case BankDataChunkID:
				if( !in_bLoadingMedia )
				{
					m_BankReader.Skip(SubChunkHeader.dwChunkSize, ulReadBytes);
					if(ulReadBytes != SubChunkHeader.dwChunkSize)
					{
						eResult = AK_InvalidFile;
						bDone = true;
					}
				}
				else
				{
					if( in_Item.load.bankLoadFlag == AkBankLoadFlag_InMemory )
					{
						//Bank laoded from memory
						pDataBank = (AkUInt8*)m_BankReader.GetData( SubChunkHeader.dwChunkSize );
						m_BankReader.ReleaseData();
					}
					else
					{
						//Bank loaded from file
						eResult = ProcessDataChunk( SubChunkHeader.dwChunkSize, out_pUsageSlot );
						pDataBank = out_pUsageSlot->m_pData;
					}
				}
				break;

			case BankDataIndexChunkID:
				if( !in_bLoadingMedia )
				{
					m_BankReader.Skip(SubChunkHeader.dwChunkSize, ulReadBytes);
					if( ulReadBytes != SubChunkHeader.dwChunkSize )
					{
						eResult = AK_InvalidFile;
						bDone = true;
					}
				}
				else
				{
					AKASSERT( pDataBank ); // must have been filled when processing BankDataChunkID chunk
					eResult = LoadMedia( 
						bankID, 
						pDataBank, 
						out_pUsageSlot, 
						SubChunkHeader.dwChunkSize,
						in_bLoadingMedia && in_Item.load.bankLoadFlag == AkBankLoadFlag_InMemory
						);
				}
				break;

			case BankHierarchyChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessHircChunk( out_pUsageSlot, bankID );
				break;

			case BankStrMapChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessStringMappingChunk( SubChunkHeader.dwChunkSize, out_pUsageSlot );
				break;

			case BankStateMgrChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessGlobalSettingsChunk( SubChunkHeader.dwChunkSize, out_pUsageSlot );
				break;

			case BankFXParamsChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessFxParamsChunk( SubChunkHeader.dwChunkSize );
				break;

			case BankEnvSettingChunkID:
				if( l_LoadingOnlyMedia ){ bDone = true; break; }// If only loading media, we are finished.
				eResult = ProcessEnvSettingsChunk( SubChunkHeader.dwChunkSize );
				break;

			default:
				AKASSERT(!"Unknown Bank chunk for this Reader version, it will be ignored");
				//Skip it
				m_BankReader.Skip(SubChunkHeader.dwChunkSize, ulReadBytes);
				if(ulReadBytes != SubChunkHeader.dwChunkSize)
				{
					eResult = AK_InvalidFile;
					bDone = true;
				}
				break;

			}
		}
		else
		{
			if(ulReadBytes != 0)
			{
				AKASSERT(!"Should not happen on a valid file");
				eResult = AK_InvalidFile;
			}
			bDone = true;
		}
	}

	// If a temporary bus was set for a device from Wwise but there wasn't any of that type loaded in the bank, 
	// set the old (from Wwise).
	CAkFeedbackBus::ResetMasterBus(pOldFeedbackMasterBus);

	m_BankReader.CloseFile();

	AkMonitorData::NotificationReason Reason;
	AkUniqueID l_langageID = AK_INVALID_UNIQUE_ID;
	if(eResult == AK_Success)
	{
		Reason = AkMonitorData::NotificationReason_BankLoaded;
		l_langageID = BankHeaderInfo.dwLanguageID;
	}
	else if(eResult == AK_Cancelled)
	{
		Reason = AkMonitorData::NotificationReason_BankLoadCancelled;
	}
	else
	{
		Reason = AkMonitorData::NotificationReason_BankLoadFailed;
		MONITOR_ERROR( AK::Monitor::ErrorCode_BankLoadFailed );
	}
	MONITOR_BANKNOTIF( bankID, l_langageID, Reason );

	return eResult;
}

AKRESULT CAkBankMgr::LoadBankPre( AkBankQueueItem& in_rItem )
{
	AKRESULT eLoadResult = AK_Success;

	m_queueLock.Lock();
	m_BankIDBeingProcessed = in_rItem.load.BankID;
	m_bCancelFlag = false;
	m_queueLock.Unlock();

	CAkUsageSlot * pUsageSlot = NULL;
	AKRESULT eResult = LoadBank( in_rItem, pUsageSlot, true );
	if( eResult == AK_BankAlreadyLoaded )
	{
		eResult = AK_Success;
		eLoadResult = AK_BankAlreadyLoaded;
	}
	else if ( eResult == AK_Success )
	{
		pUsageSlot->WasLoadedAsABank( true );
		pUsageSlot->WasLoadedFromMemory( in_rItem.load.pInMemoryBank != NULL );
		if ( !m_ListLoadedBanks.Set( in_rItem.load.BankID, pUsageSlot ) )
		{
			eResult = AK_Fail;
		}
	}

	m_queueLock.Lock();
	m_BankIDBeingProcessed = 0;
	bool bCancelled = m_bCancelFlag;
	m_bCancelFlag = false;
	m_queueLock.Unlock();

	MONITOR_LOADEDBANK( pUsageSlot, false );

	if(bCancelled || (eResult != AK_Success))
	{
		// Here, should not pass the notification flag.
		if ( pUsageSlot )
		{
			m_ListLoadedBanks.Unset( in_rItem.load.BankID );
			pUsageSlot->UnloadPre();
			pUsageSlot->RemoveContent();
			pUsageSlot->Release( true );
		}

		// Re-place callback.
		// Set status for callback (below).
		if ( bCancelled )
		{
			eLoadResult = AK_Cancelled;
		}
		else
		{
			eLoadResult = eResult;
		}
	}

	// Notify user.
	NotifyCompletion( in_rItem, eLoadResult );

	return eResult;
}

AKRESULT CAkBankMgr::UnloadBankPre( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

	m_BankListLock.Lock(); // CAREFUL with the tricky unlocking below. We want to be atomic with unset.

	AkUniqueID bankID = in_Item.load.BankID;
	AkListLoadedBanks::Iterator it = m_ListLoadedBanks.FindEx( bankID );
	if( it != m_ListLoadedBanks.End() )
	{
		CAkUsageSlot * pUsageSlot = (*it).item;

		m_BankListLock.Unlock();

		if( pUsageSlot->WasLoadedAsABank() )
		{
			pUsageSlot->UnloadPre();
			pUsageSlot->WasLoadedAsABank( false );

			// Prepare bank for final release notification
			pUsageSlot->m_pfnBankCallback	= in_Item.callbackInfo.pfnBankCallback;
			pUsageSlot->m_pCookie			= in_Item.callbackInfo.pCookie;
			
			AkQueuedMsg Item;

			Item.type = QueuedMsgType_KillBank;
			Item.killbank.pUsageSlot = pUsageSlot;

			MONITOR_LOADEDBANK( pUsageSlot, false );// Must be done before enqueuing the command as the pUsageSlot could be destroyed after that.

			g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_KillBank() );

			eResult = AK::SoundEngine::RenderAudio();
		}
		else
		{
			// The bank is there, but do not have to be unloaded.
			eResult = AK_Success;
			AkMonitorData::NotificationReason Reason = AkMonitorData::NotificationReason_BankUnloaded;
			MONITOR_BANKNOTIF( bankID, AK_INVALID_UNIQUE_ID, Reason );

			MONITOR_LOADEDBANK( pUsageSlot, false );

			// Notify user.
			NotifyCompletion( in_Item, eResult );
		}
	}
	else // not found
	{
		m_BankListLock.Unlock();

		eResult = AK_Success;
		AkMonitorData::NotificationReason Reason = AkMonitorData::NotificationReason_BankUnloaded;
		MONITOR_BANKNOTIF( bankID, AK_INVALID_UNIQUE_ID, Reason );

		// Notify user.
		NotifyCompletion( in_Item, eResult );
	}

	return eResult;
}

AKRESULT CAkBankMgr::ClearBanksInternal( AkBankQueueItem in_Item )
{
	AkUInt32 l_ulNumBanks = m_ListLoadedBanks.Length();
	
	if( l_ulNumBanks )
	{
		AkBankID* l_paBankIDs = (AkBankID*)alloca( l_ulNumBanks * sizeof( AkBankID ) );
		AKASSERT( l_paBankIDs );
		
		int i = 0;
		for( AkListLoadedBanks::Iterator iter = m_ListLoadedBanks.Begin(); iter != m_ListLoadedBanks.End(); ++iter )
		{
			// Unload only banks that have been loaded using LoadBank, and not those loaded only with prepare event (or that have already been unloaded).
			if( (*(m_ListLoadedBanks.Exists( (*iter).key )))->WasLoadedAsABank() )
			{
				l_paBankIDs[i] = (*iter).key;
				++i;
			}
		}

		// unloading in reverse order
		while ( i > 0 )
		{
			m_BankListLock.Lock(); // CAREFUL with the tricky unlocking below. We want to be atomic with unset.

			AkUniqueID bankID = l_paBankIDs[ --i ];
			AkListLoadedBanks::Iterator it = m_ListLoadedBanks.FindEx( bankID );
			if( it != m_ListLoadedBanks.End() )
			{
				CAkUsageSlot * pUsageSlot = (*it).item;

				m_BankListLock.Unlock();

				if( pUsageSlot->WasLoadedAsABank() )
				{
					pUsageSlot->UnloadPre();
					pUsageSlot->WasLoadedAsABank( false );

					AK::SoundEngine::AkSyncLoader syncLoader;
					AKRESULT eResult = syncLoader.Init();
					AKASSERT( eResult == AK_Success );

					// Prepare bank for final release notification
					pUsageSlot->m_pfnBankCallback	= AK::SoundEngine::DefaultBankCallbackFunc;
					pUsageSlot->m_pCookie			= syncLoader.GetCookie();

					eResult = m_CallbackMgr.AddCookie( pUsageSlot->m_pCookie );// If the cookie fails to be added, the system will not wait for completion
					
					AkQueuedMsg Item;

					Item.type = QueuedMsgType_KillBank;
					Item.killbank.pUsageSlot = pUsageSlot;

					MONITOR_LOADEDBANK( pUsageSlot, false );// Must be done before enqueuing the command as the pUsageSlot could be destroyed after that.

					g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_KillBank() );

					AK::SoundEngine::RenderAudio();

					syncLoader.Wait( eResult );
				}
				else
				{
					// The bank is there, but do not have to be unloaded.
					MONITOR_BANKNOTIF( bankID, AK_INVALID_UNIQUE_ID, AkMonitorData::NotificationReason_BankUnloaded );

					MONITOR_LOADEDBANK( pUsageSlot, false );
				}
			}
			else // not found
			{
				m_BankListLock.Unlock();
			}
		}
	}

	NotifyCompletion( in_Item, AK_Success );

	return AK_Success;
}

AKRESULT CAkBankMgr::PrepareEvents( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

	AKASSERT( in_Item.prepare.numEvents );

	if( in_Item.prepare.numEvents == 1 )
	{
		eResult = PrepareEvent( in_Item, in_Item.prepare.eventID );
	}
	else
	{
		// Multiple events to prepare
		AKASSERT( in_Item.prepare.pEventID );

		for( AkUInt32 i = 0; i < in_Item.prepare.numEvents; ++i )
		{
			eResult = PrepareEvent( in_Item, in_Item.prepare.pEventID[i] );
			if( eResult != AK_Success )
			{
				while( i > 0 )
				{
					--i;
					UnprepareEvent( in_Item.prepare.pEventID[i] );
				}
				break;
			}
		}

		AkFree( g_DefaultPoolId, in_Item.prepare.pEventID );
		in_Item.prepare.pEventID = NULL;
	}

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::PrepareEvent( AkBankQueueItem in_Item, AkUniqueID in_EventID )
{
	AKRESULT eResult = AK_Success;

	AKASSERT( g_pIndex );
	CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_EventID );
	if( pEvent )
	{
		if( !pEvent->IsPrepared() )
		{
			for( CAkEvent::AkActionList::Iterator iter = pEvent->m_actions.Begin(); iter != pEvent->m_actions.End(); ++iter )
			{
				CAkAction* pAction = *iter;
				if( pAction->ActionType() == AkActionType_Play )
				{
					CAkActionPlay* pActionPlay = (CAkActionPlay*)pAction;
					eResult = PrepareHierarchy( in_Item, pActionPlay->GetFileID() );

					if( eResult == AK_Success )
					{
						eResult = CAkAudioNode::PrepareNodeData( pActionPlay->ElementID() );
						if( eResult != AK_Success )
						{
							UnPrepareHierarchy( pActionPlay->GetFileID() );
						}
					}
					if( eResult != AK_Success )
					{
						// Iterate to undo the partial prepare
						for( CAkEvent::AkActionList::Iterator iterFlush = pEvent->m_actions.Begin(); iterFlush != iter; ++iterFlush )
						{
							pAction = *iterFlush;
							if( pAction->ActionType() == AkActionType_Play )
							{
								pActionPlay = (CAkActionPlay*)pAction;
								CAkAudioNode::UnPrepareNodeData( pActionPlay->ElementID() );
								UnPrepareHierarchy( pActionPlay->GetFileID() );
							}
						}
						break;
					}
				}
			}
			// If successfully prepared, we increment the refcount
			if( eResult == AK_Success )
			{
				AK_PERF_INCREMENT_PREPARE_EVENT_COUNT();
				pEvent->AddRef();
			}
		}

		if( eResult == AK_Success )
			pEvent->IncrementPreparedCount();

		MONITOR_EVENTPREPARED( pEvent->ID(), pEvent->GetPreparationCount() );

		pEvent->Release();
	}
	else
	{
		eResult = AK_IDNotFound;
	}

	MONITOR_PREPAREEVENTNOTIF( 
		eResult == AK_Success ? AkMonitorData::NotificationReason_EventPrepareSuccess : AkMonitorData::NotificationReason_EventPrepareFailure, 
		in_EventID );

	return eResult;
}

AKRESULT CAkBankMgr::UnprepareEvents( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

	AKASSERT( in_Item.prepare.numEvents );

	if( in_Item.prepare.numEvents == 1 )
	{
		eResult = UnprepareEvent( in_Item.prepare.eventID );
	}
	else
	{
		// Multiple events to prepare
		AKASSERT( in_Item.prepare.pEventID );

		for( AkUInt32 i = 0; i < in_Item.prepare.numEvents; ++i )
		{
			eResult = UnprepareEvent( in_Item.prepare.pEventID[i] );
			if( eResult != AK_Success ) 
				break;
		}

		AkFree( g_DefaultPoolId, in_Item.prepare.pEventID );
		in_Item.prepare.pEventID = NULL;
	}

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::UnprepareAllEvents( AkBankQueueItem in_Item )
{
	ClearPreparedEvents();
	
	// Notify user.
	NotifyCompletion( in_Item, AK_Success );

	return AK_Success;
}

AKRESULT CAkBankMgr::UnprepareEvent( AkUniqueID in_EventID )
{
	AKRESULT eResult = AK_Success;

	CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_EventID );
	if( pEvent )
	{
		UnprepareEvent( pEvent );
		pEvent->Release();
	}
	else
	{
		eResult = AK_IDNotFound;
	}

	MONITOR_PREPAREEVENTNOTIF( 
		eResult == AK_Success ? AkMonitorData::NotificationReason_EventUnPrepareSuccess : AkMonitorData::NotificationReason_EventUnPrepareFailure, 
		in_EventID );

	return eResult;
}

void CAkBankMgr::UnprepareEvent( CAkEvent* in_pEvent, bool in_bCompleteUnprepare /*= false*/ )
{
	if( in_pEvent->IsPrepared() )
	{
		if( in_bCompleteUnprepare )
		{
			in_pEvent->FlushPreparationCount();
		}
		else
		{
			in_pEvent->DecrementPreparedCount();
		}

		if( !in_pEvent->IsPrepared() )// must check again as DecrementPreparedCount() just changed it.
		{
			AK_PERF_DECREMENT_PREPARE_EVENT_COUNT();
			CAkEvent::AkActionList::Iterator iter = in_pEvent->m_actions.Begin();
			while( iter != in_pEvent->m_actions.End() )
			{
				CAkAction* pAction = *iter;
				++iter;// incrementing iter BEFORE unpreparing hirarchy and releasing the event.

				if( pAction->ActionType() == AkActionType_Play )
				{
					CAkAudioNode::UnPrepareNodeData( ((CAkActionPlay*)(pAction))->ElementID() );
					UnPrepareHierarchy( ((CAkActionPlay*)(pAction))->GetFileID() );
				}
			}
			// If successfully unprepared, we increment decrement the refcount.
			in_pEvent->Release();
		}
	}
	MONITOR_EVENTPREPARED( in_pEvent->ID(), in_pEvent->GetPreparationCount() );
}

void NotifyPrepareGameSync( AkPrepareGameSyncQueueItem in_Item, AKRESULT in_Result )
{
	AkMonitorData::NotificationReason reason;
	if( in_Result == AK_Success )
	{
		if( in_Item.bSupported )
			reason = AkMonitorData::NotificationReason_PrepareGameSyncSuccess;
		else
			reason = AkMonitorData::NotificationReason_UnPrepareGameSyncSuccess;
	}
	else
	{
		if( in_Item.bSupported )
			reason = AkMonitorData::NotificationReason_PrepareGameSyncFailure;
		else
			reason = AkMonitorData::NotificationReason_UnPrepareGameSyncFailure;
	}
	MONITOR_PREPAREGAMESYNCNOTIF( reason, in_Item.uGameSyncID, in_Item.uGroupID, in_Item.eGroupType );
}

AKRESULT CAkBankMgr::PrepareGameSync( AkBankQueueItem in_Item )
{
	AKRESULT eResult = AK_Success;

#ifndef AK_OPTIMIZED
	if( !g_settings.bEnableGameSyncPreparation )
	{
		MONITOR_ERRORMSG2( L"Unexpected call to PrepareGameSyncs.",
			L"see: \"bEnableGameSyncPreparation\" parameter in AkInitSettings for more information");
	}
#endif

	if( in_Item.gameSync.uNumGameSync == 1 )
	{
		eResult = g_pStateMgr->PrepareGameSync( in_Item.gameSync.eGroupType, in_Item.gameSync.uGroupID, in_Item.gameSync.uGameSyncID, in_Item.gameSync.bSupported );
		NotifyPrepareGameSync( in_Item.gameSync, eResult );
	}
	else
	{
		AKASSERT( in_Item.gameSync.uNumGameSync && in_Item.gameSync.pGameSyncID );

		for( AkUInt32 i = 0; i < in_Item.gameSync.uNumGameSync; ++i )
		{
			eResult = g_pStateMgr->PrepareGameSync( in_Item.gameSync.eGroupType, in_Item.gameSync.uGroupID, in_Item.gameSync.pGameSyncID[i], in_Item.gameSync.bSupported );
			if( eResult != AK_Success )
			{
				// Rollback if was adding content and one failed
				AKASSERT( in_Item.gameSync.bSupported );// Must not fail when unsupporting

				for( AkUInt32 k = 0; k < i; ++k )
				{
					g_pStateMgr->PrepareGameSync( in_Item.gameSync.eGroupType, in_Item.gameSync.uGroupID, in_Item.gameSync.pGameSyncID[k], false );
				}
				break;
			}
			NotifyPrepareGameSync( in_Item.gameSync, eResult );

		}
			// flush data if in_Item.gameSync.uNumGameSync greater than 1
		AkFree( g_DefaultPoolId, in_Item.gameSync.pGameSyncID );
	}

	// Notify user.
	NotifyCompletion( in_Item, eResult );

	return eResult;
}

AKRESULT CAkBankMgr::PrepareHierarchy( AkBankQueueItem in_rItem, AkFileID in_FileID )
{
	CAkUsageSlot ** ppUsageSlot = m_ListLoadedBanks.Exists( in_FileID );
	if( ppUsageSlot )
	{
		(*ppUsageSlot)->AddRefPrepare();
		return AK_Success;
	}

	CAkUsageSlot * pUsageSlot = NULL;

	in_rItem.load.BankID = in_FileID;
	AKRESULT eResult = LoadBank( in_rItem, pUsageSlot, false );
	AKASSERT( eResult != AK_BankAlreadyLoaded );// must not happen in prepare event.
	if ( eResult == AK_Success )
	{
		if ( !m_ListLoadedBanks.Set( in_FileID, pUsageSlot ) )
		{
			eResult = AK_Fail;
		}
	}

	if( eResult != AK_Success )
	{
		// Here, should not pass the notification flag.
		if ( pUsageSlot )
		{
			m_ListLoadedBanks.Unset( in_FileID );
			pUsageSlot->ReleasePrepare();
		}
	}

	MONITOR_LOADEDBANK( pUsageSlot, false );

	return eResult;
}

void CAkBankMgr::UnPrepareHierarchy( AkUniqueID in_BankID )
{
	CAkUsageSlot ** ppUsageSlot = m_ListLoadedBanks.Exists( in_BankID );
	if( ppUsageSlot )
	{
		(*ppUsageSlot)->ReleasePrepare();
	}
}

// Not to be called directly, the thread must be stopped for it to be possible
void  CAkBankMgr::UnloadAll()
{
	ClearPreparedEvents();

	AKASSERT(!AkIsValidThread(&m_BankMgrThread));

	AkListLoadedBanks::Iterator it = m_ListLoadedBanks.Begin();
	while ( it != m_ListLoadedBanks.End() )
	{
		(*it).item->UnloadPre();
		(*it).item->RemoveContent();
		(*it).item->Release( true );
		it = m_ListLoadedBanks.Begin();
	}

	m_ListLoadedBanks.Term();
}
	
void CAkBankMgr::AddLoadedItem(CAkUsageSlot* in_pUsageSlot, CAkIndexable* in_pIndexable)
{
	// Guaranteed to succeed because the necessary space has already been reserved
	AKVERIFY( in_pUsageSlot->m_listLoadedItem.AddLast(in_pIndexable) );
}

AkBankID CAkBankMgr::GetBankIDFromInMemorySpace( void* in_pData, AkUInt32 in_uSize )
{
	// The bank must at least be the size of the header so we can read the bank ID.
	if( in_uSize >= AK_MINIMUM_BANK_SIZE )
	{
		AkUInt8* pdata = (AkUInt8*)in_pData;
		pdata += sizeof(AkSubchunkHeader);
		return ((AkBankHeader*)(pdata))->dwSoundBankID;
	}
	else
	{
		AKASSERT( !"Invalid memory to load bank from" );
		return AK_INVALID_BANK_ID;
	}
}

AKRESULT CAkBankMgr::ProcessBankHeader(AkBankHeader& in_rBankHeader)
{
	AKRESULT eResult = AK_Success;

	AkSubchunkHeader SubChunkHeader;

	eResult = m_BankReader.FillDataEx(&SubChunkHeader, sizeof(SubChunkHeader));
	if( eResult == AK_Success && SubChunkHeader.dwTag == BankHeaderChunkID )
	{
		eResult = m_BankReader.FillDataEx(&in_rBankHeader, sizeof(in_rBankHeader) );
		AkUInt32 uSizeToSkip = SubChunkHeader.dwChunkSize - sizeof(in_rBankHeader);
		if(eResult == AK_Success && uSizeToSkip )
		{
			AkUInt32 ulSizeSkipped = 0;
			eResult = m_BankReader.Skip( uSizeToSkip, ulSizeSkipped);
			if(eResult == AK_Success && ulSizeSkipped != uSizeToSkip)
			{
				eResult = AK_BankReadError;
			}
		}
		if( eResult == AK_Success && in_rBankHeader.dwBankGeneratorVersion != AK_BANK_READER_VERSION )
		{
			MONITOR_ERRORMSG2( L"Load bank failed: ", L"Incompatible bank version" );
			eResult = AK_WrongBankVersion;
		}
	}
	else
	{
		eResult = AK_InvalidFile;
	}

	m_bFeedbackInBank = (in_rBankHeader.bFeedbackSupported != 0);
 
	return eResult;
}

AKRESULT CAkBankMgr::ProcessDataChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* in_pUsageSlot )
{
	AKRESULT eResult = AK_Success;
	if ( in_dwDataChunkSize == 0 )
		return eResult; // Empty chunk -- return immediately

	AKASSERT(in_pUsageSlot->m_pData == NULL);

    if ( in_pUsageSlot->m_memPoolId == AK_INVALID_POOL_ID )
    {
		// No pool was specified by client: we need to create one internally for this bank.
        // Size is the exact size needed for data in this bank, 1 block.
		// A Fixed-size mode pool is used.
        in_pUsageSlot->m_memPoolId = AK::MemoryMgr::CreatePool( NULL,
																in_dwDataChunkSize,
																in_dwDataChunkSize,
#if defined(WIN32) || defined(__PPU__)
																AkMalloc | AkFixedSizeBlocksMode,// Malloc on Win32.
#elif defined(XBOX360)
																AkPhysicalAlloc | AkFixedSizeBlocksMode, // Physical alloc on XBox360 (for XMA data... cannot hurt).
#elif defined(RVL_OS)
																AkMallocMEM2 | AkFixedSizeBlocksMode,// Malloc on Win32.
#endif
																AK_BANK_PLATFORM_DATA_ALIGNMENT
                                                                 );

		if( in_pUsageSlot->m_memPoolId != AK_INVALID_POOL_ID)
		{
#ifndef AK_OPTIMIZED
			char** ppString = m_BankIDToFileName.Exists( in_pUsageSlot->m_BankID );
			if( ppString )
			{
				AKASSERT( *ppString );
				// Convert the string to W_char and go by string.

				AkTChar wideString[ AK_MAX_PATH ];
				AkAnsiToWideChar( *ppString, AK_MAX_PATH, wideString );

				AK_SETPOOLNAME( in_pUsageSlot->m_memPoolId, wideString );
			}
#endif
			// Flag usage slot as using an internal pool. Store the generated pool id.
            in_pUsageSlot->m_bIsInternalPool = true;
		}
		else
		{
			eResult = AK_Fail;
		}
	}

	// Allocate a bank from a pre-existing pool
	if ( eResult == AK_Success )
	{
		eResult = AK::MemoryMgr::CheckPoolId( in_pUsageSlot->m_memPoolId );

		// Allocate data in the pool.
		if ( eResult == AK_Success )
		{
			if ( AK::MemoryMgr::GetPoolAttributes( in_pUsageSlot->m_memPoolId ) & AkBlockMgmtMask )
			{
				in_pUsageSlot->m_pData = (AkUInt8*)AK::MemoryMgr::GetBlock( in_pUsageSlot->m_memPoolId );
			}
			else
			{
				in_pUsageSlot->m_pData = (AkUInt8*)AkAlloc( in_pUsageSlot->m_memPoolId, in_dwDataChunkSize );
			}
			if ( in_pUsageSlot->m_pData == NULL )
			{
				// Cannot allocate bank memory.
				eResult = AK_Fail;
			}
			else
			{
				in_pUsageSlot->m_uLoadedDataSize = in_dwDataChunkSize;
				AK_PERF_INCREMENT_BANK_MEMORY( in_pUsageSlot->m_uLoadedDataSize );
			}
		}
	}

	if(eResult == AK_Success)
	{
		AkUInt32 ulReadBytes = 0;
		eResult = m_BankReader.FillData( in_pUsageSlot->m_pData, in_dwDataChunkSize, ulReadBytes 
#ifdef RVL_OS
			, true
#endif
			);
		if(eResult == AK_Success && ulReadBytes != in_dwDataChunkSize)
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_ErrorWhileLoadingBank );
			eResult = AK_InvalidFile;
		}
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_InsufficientSpaceToLoadBank );
	}

	return eResult;
}

AKRESULT CAkBankMgr::ProcessHircChunk(CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID)
{
	AkUInt32 l_NumReleasableHircItem = 0;

	AKRESULT eResult = m_BankReader.FillDataEx(&l_NumReleasableHircItem, sizeof(l_NumReleasableHircItem));
	if( eResult == AK_Success && l_NumReleasableHircItem )
	{
		eResult = in_pUsageSlot->m_listLoadedItem.GrowArray( l_NumReleasableHircItem ) ? AK_Success : AK_Fail;
	}
	
	bool bWarnedUnknownContent = false;

	for( AkUInt32 i = 0; i < l_NumReleasableHircItem && eResult == AK_Success; ++i )
	{
		AKBKSubHircSection Section;

		eResult = m_BankReader.FillDataEx(&Section, sizeof(Section));
		if(eResult == AK_Success)
		{
			switch( Section.eHircType )
			{
			case HIRCType_Action:
				eResult = ReadAction(Section, in_pUsageSlot);
				break;

			case HIRCType_Event:
				eResult = ReadEvent(Section, in_pUsageSlot);
				break;

			case HIRCType_Sound:
				eResult = ReadSourceParent<CAkSound>(Section, in_pUsageSlot, in_dwBankID);
				break;

			case HIRCType_RanSeqCntr:
				eResult = StdBankRead<CAkRanSeqCntr, CAkAudioNode>( Section, in_pUsageSlot, g_pIndex->m_idxAudioNode );
				break;

			case HIRCType_SwitchCntr:
				eResult = StdBankRead<CAkSwitchCntr, CAkAudioNode>( Section, in_pUsageSlot, g_pIndex->m_idxAudioNode );
				break;

			case HIRCType_LayerCntr:
				eResult = StdBankRead<CAkLayerCntr, CAkAudioNode>( Section, in_pUsageSlot, g_pIndex->m_idxAudioNode );
				break;

			case HIRCType_ActorMixer:
				eResult = StdBankRead<CAkActorMixer, CAkAudioNode>( Section, in_pUsageSlot, g_pIndex->m_idxAudioNode );
				break;

			case HIRCType_State:
				{
					AkUniqueID StateID = 0; // Unused
					eResult = ReadState( Section, in_pUsageSlot, StateID, true, AK_INVALID_UNIQUE_ID );
				}
				break;

			case HIRCType_Bus:
				eResult = ReadBus( Section, in_pUsageSlot );
				break;

			case HIRCType_Attenuation:
				eResult = StdBankRead<CAkAttenuation, CAkAttenuation>( Section, in_pUsageSlot, g_pIndex->m_idxAttenuations );
				break;

			case HIRCType_DialogueEvent:
				eResult = StdBankRead<CAkDialogueEvent, CAkDialogueEvent>( Section, in_pUsageSlot, g_pIndex->m_idxDialogueEvents );
				break;

			case HIRCType_FeedbackBus:
				eResult = StdBankRead<CAkFeedbackBus, CAkAudioNode>( Section, in_pUsageSlot, g_pIndex->m_idxAudioNode );
				break;

			case HIRCType_FeedbackNode:
				eResult = ReadSourceParent<CAkFeedbackNode>(Section, in_pUsageSlot, in_dwBankID);
				break;

			default:
				{
					if( g_pExternalBankHandlerCallback )
					{
						eResult = g_pExternalBankHandlerCallback( Section, in_pUsageSlot, in_dwBankID );
						if( eResult != AK_PartialSuccess )
						{
							break;
						}
					}
					else if( !bWarnedUnknownContent )
					{
						if(	   Section.eHircType == HIRCType_Segment 
							|| Section.eHircType == HIRCType_Track
							|| Section.eHircType == HIRCType_MusicSwitch 
							|| Section.eHircType == HIRCType_MusicRanSeq )
						{
							bWarnedUnknownContent = true;
							MONITOR_ERRORMSG2( L"Music engine not initialized : Content can not be loaded from bank", L"" );
						}
					}
					AkUInt32 ulReadBytes = 0;
					m_BankReader.Skip(Section.dwSectionSize, ulReadBytes );
					if(ulReadBytes != Section.dwSectionSize)
					{
						eResult = AK_InvalidFile;
					}
				}
				break;
			}
		}
	}

	return eResult;
}

AKRESULT CAkBankMgr::ProcessStringGroup( AkUInt32 & io_dwDataChunkSize, AkGroupStringIdx & in_group )
{
	AkUInt32 uiNumGroups;
	AKRESULT eResult = m_BankReader.FillDataEx( &uiNumGroups, sizeof( AkUInt32 ) );
	if( eResult != AK_Success ) 
		return eResult;

	io_dwDataChunkSize -= sizeof( AkUInt32 );

	for ( AkUInt32 i = 0; i < uiNumGroups; ++i )
	{
		AKBKHashHeader hdrGroup;

		eResult = m_BankReader.FillDataEx( &hdrGroup, sizeof( hdrGroup ) );
		if ( eResult != AK_Success ) 
			return eResult;

		io_dwDataChunkSize -= sizeof( hdrGroup );

		AkStringIDHash * pStringToID = (AkStringIDHash *) AkAlloc( g_DefaultPoolId, hdrGroup.uiSize );
		if ( pStringToID == NULL )
			return AK_Fail;

		eResult = m_BankReader.FillDataEx( pStringToID, hdrGroup.uiSize );
		if ( eResult != AK_Success ) 
		{
			AkFree( g_DefaultPoolId, pStringToID );
			return eResult;
		}

		io_dwDataChunkSize -= hdrGroup.uiSize;

		if( in_group.Set( hdrGroup.uiType, pStringToID ) == NULL )
		{
			AkFree( g_DefaultPoolId, pStringToID );
			return AK_Fail;
		}
	}

	return eResult;
}

AKRESULT CAkBankMgr::ProcessStringMappingChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* in_pUsageSlot )
{
	AKRESULT eResult = AK_Success;

	while( in_dwDataChunkSize && eResult == AK_Success )
	{
		// Read header

		AKBKHashHeader hdr;

		eResult = m_BankReader.FillDataEx( &hdr, sizeof( hdr ) );
		if ( eResult != AK_Success ) break;

		in_dwDataChunkSize -= sizeof( hdr );

		// StringType_Bank is a special case
		
		AKASSERT( hdr.uiType == StringType_Bank );

		////////////////////////////////////////////////////
		//    StringType_Bank
		////////////////////////////////////////////////////
		// hdr.uiSize is the number of entries following
		// each entry is
		// UINT32 bankID
		// UINT8 stringSize
		// char[] string (NOT NULL TERMINATED)
		////////////////////////////////////////////////////

		for( AkUInt32 stringNumber = 0; stringNumber < hdr.uiSize; ++stringNumber )
		{	
			// read bank ID
			AkBankID bankID;
			eResult = m_BankReader.FillDataEx( &bankID, sizeof( bankID ) );
			if( eResult != AK_Success )break;
			in_dwDataChunkSize -= sizeof( bankID );

			// read string size (NOT NULL TERMINATED)
			AkUInt8 stringsize;
			eResult = m_BankReader.FillDataEx( &stringsize, sizeof( stringsize ) );
			if( eResult != AK_Success )break;
			in_dwDataChunkSize -= sizeof( stringsize );

			// Check the list.
			if( m_BankIDToFileName.Exists( bankID ) )
			{
				AkUInt32 uIgnoredSkippedSize;
				m_BankReader.Skip( stringsize, uIgnoredSkippedSize );
				AKASSERT( uIgnoredSkippedSize == stringsize );
				in_dwDataChunkSize -= stringsize;
			}
			else
			{
				// Alloc string of size stringsize + 1 ( for missing NULL character )
				// Alloc string of size stringsize + 4 ( for missing .bnk extension )
				char* pString = (char*)AkAlloc( g_DefaultPoolId, stringsize + 1 + 4 );
				if( pString )
				{
					// Read/Copy the string
					pString[ stringsize ] = '.';
					pString[ stringsize + 1] = 'b';
					pString[ stringsize + 2] = 'n';
					pString[ stringsize + 3] = 'k';
					pString[ stringsize + 4] = 0;
					eResult = m_BankReader.FillDataEx( pString, stringsize );

					if( eResult == AK_Success )
					{
						in_dwDataChunkSize -= stringsize;
						// Add the entry in the table
						char** ppString = m_BankIDToFileName.Set( bankID );
						if( ppString )
						{
							*ppString = pString;
						}
						else
						{
							eResult = AK_InsufficientMemory;
						}
					}
					if( eResult != AK_Success )
					{
						AkFree( g_DefaultPoolId, pString );
						break;
					}
				}
				else
				{
					// Fill eResult and break.
					eResult = AK_InsufficientMemory;
					break;
				}
			}
		} // for( AkUInt32 stringNumber = 0; stringNumber < hdr.uiSize; ++stringNumber )
	}

	return eResult;
}

void CAkBankMgr::UpdateBankName( AkBankID in_bankID, AkChar* in_pStringWithoutExtension )
{
	// Check the list.
	if( !m_BankIDToFileName.Exists( in_bankID ) )
	{
		// Alloc string of size stringsize + 1 ( for missing NULL character )
		// Alloc string of size stringsize + 4 ( for missing .bnk extension )
		size_t stringsize = strlen( in_pStringWithoutExtension );
		char* pString = (char*)AkAlloc( g_DefaultPoolId, stringsize + 1 + 4 );
		if( pString )
		{
			// Copy the string + .bnk
			memcpy( pString, in_pStringWithoutExtension, stringsize );
			
			pString[ stringsize ] = '.';
			pString[ stringsize + 1] = 'b';
			pString[ stringsize + 2] = 'n';
			pString[ stringsize + 3] = 'k';
			pString[ stringsize + 4] = 0;

			// Add the entry in the table
			char** ppString = m_BankIDToFileName.Set( in_bankID );
			if( ppString )
			{
				*ppString = pString;
			}
			else
			{
				AkFree( g_DefaultPoolId, pString );
			}
		}
	}
}

AKRESULT CAkBankMgr::ProcessGlobalSettingsChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* in_pUsageSlot )
{
	AKRESULT eResult = AK_Success;

	AKASSERT(g_pStateMgr);

	if( in_dwDataChunkSize )
	{
		//Here, read the first item, which is the volume threshold
		AkReal32 fVolumeThreshold;
		eResult = m_BankReader.FillDataEx( &fVolumeThreshold, sizeof( fVolumeThreshold ) );
		AK::SoundEngine::SetVolumeThreshold( fVolumeThreshold );

		AkUInt32 ulNumStateGroups = 0;
		if( eResult == AK_Success )
		{
			eResult = m_BankReader.FillDataEx( &ulNumStateGroups, sizeof( ulNumStateGroups ) );
		}
		if( eResult == AK_Success )
		{
			for( AkUInt32 i = 0; i < ulNumStateGroups; ++i ) //iterating trough state groups
			{
				AkUInt32 ulStateGroupID = 0;
				AkTimeMs DefaultTransitionTime = 0;
				AkUInt32 ulNumCustomStates = 0;
				AkUInt32 ulNumTransitions = 0;

				eResult = m_BankReader.FillDataEx( &ulStateGroupID, sizeof( ulStateGroupID ) );
				if( eResult == AK_Success )
				{
					eResult = m_BankReader.FillDataEx( &DefaultTransitionTime, sizeof( DefaultTransitionTime ) );
				}
				if( eResult == AK_Success )
				{
					eResult = g_pStateMgr->AddStateGroup( ulStateGroupID );
				}
				if( eResult == AK_Success )
				{
					eResult = g_pStateMgr->SetdefaultTransitionTime( ulStateGroupID, DefaultTransitionTime );
				}
				if( eResult == AK_Success )
				{
					eResult = m_BankReader.FillDataEx( &ulNumCustomStates, sizeof( ulNumCustomStates ) );
				}
				if( eResult == AK_Success )
				{
					if( ulNumCustomStates )
						eResult = in_pUsageSlot->m_listLoadedItem.GrowArray( ulNumCustomStates ) ? AK_Success : AK_Fail;
				}
				if ( eResult == AK_Success )
				{
					for( AkUInt32 j = 0; j < ulNumCustomStates; ++j )//iterating trough custom states
					{
						AkUInt32 ulStateType;
						AKBKSubHircSection Section;
						AkUniqueID ulStateID = 0;

						eResult = m_BankReader.FillDataEx( &ulStateType, sizeof( ulStateType ) );
						if( eResult == AK_Success )
						{
							eResult = m_BankReader.FillDataEx( &Section, sizeof( Section ) );
						}
						if( eResult == AK_Success )
						{
							eResult = ReadState( Section, in_pUsageSlot, ulStateID, false, ulStateGroupID );
						}
						if( eResult == AK_Success )
						{
							eResult = g_pStateMgr->AddState( ulStateGroupID, ulStateType, ulStateID );
						}
						// If failed, quit!
						if( eResult != AK_Success )
						{
							break;
						}
					}
				}
				if( eResult == AK_Success )
				{
					eResult = m_BankReader.FillDataEx( &ulNumTransitions, sizeof( ulNumTransitions ) );
				}
				if( eResult == AK_Success )
				{
					for( AkUInt32 j = 0; j < ulNumTransitions; ++j )//iterating trough Transition time
					{
						AkUInt32		StateFrom;
						AkUInt32		StateTo;
						AkTimeMs	TransitionTime;

						eResult = m_BankReader.FillDataEx( &StateFrom, sizeof( StateFrom ) );
						if( eResult == AK_Success )
						{
							eResult = m_BankReader.FillDataEx( &StateTo, sizeof( StateTo ) );
						}
						if( eResult == AK_Success )
						{
							eResult = m_BankReader.FillDataEx( &TransitionTime, sizeof( TransitionTime ) );
						}
						if( eResult == AK_Success )
						{
							eResult = g_pStateMgr->AddStateTransition( ulStateGroupID, StateFrom, StateTo, TransitionTime );
						}
						// If failed, quit!
						if( eResult != AK_Success)
						{
							break;
						}
					}
				}
				// If failed, quit!
				if( eResult != AK_Success )
				{
					break;
				}
			}

			AkUInt32 ulNumSwitchGroups = 0;
			if ( eResult == AK_Success )
			{
				eResult = m_BankReader.FillDataEx( &ulNumSwitchGroups, sizeof( ulNumSwitchGroups ) );
			}
			if( eResult == AK_Success )
			{
				for( AkUInt32 i = 0; i < ulNumSwitchGroups; ++i ) //iterating trough switch groups
				{
					AkUInt32	SwitchGroupID;
					AkUInt32	RTPC_ID;
					AkUInt32	ulSize;

					eResult = m_BankReader.FillDataEx( &SwitchGroupID, sizeof( SwitchGroupID ) );
					if( eResult == AK_Success )
					{
						eResult = m_BankReader.FillDataEx( &RTPC_ID, sizeof( RTPC_ID ) );
					}
					if( eResult == AK_Success )
					{
						eResult = m_BankReader.FillDataEx( &ulSize, sizeof( ulSize ) );
					}
					if( eResult == AK_Success )
					{
						// It is possible that the size be 0 in the following situation:
						// Wwise user created a switch to RTPC graph, and disabled it.
						if( ulSize )
						{
							AkUInt32 l_blockSize = ulSize*sizeof( AkRTPCGraphPointInteger );
							AkRTPCGraphPointInteger* pGraphPoints = (AkRTPCGraphPointInteger*)AkAlloc( g_DefaultPoolId, l_blockSize );
							if( pGraphPoints )
							{
								eResult = m_BankReader.FillDataEx( pGraphPoints, l_blockSize );
								if( eResult == AK_Success )
								{
									eResult = g_pRTPCMgr->AddSwitchRTPC( SwitchGroupID, RTPC_ID, pGraphPoints, ulSize );
								}
								AkFree( g_DefaultPoolId, pGraphPoints );
							}
							else
							{
								eResult = AK_Fail;
							}
						}
					}
					// If failed, quit!
					if( eResult != AK_Success)
					{
						break;
					}
				}
			}
		}
	}
	else
	{
		AKASSERT(!"Invalid STMG chunk found in the Bank");
	}

	return eResult;
}

AKRESULT CAkBankMgr::ProcessFxParamsChunk( AkUInt32 in_dwDataChunkSize )
{
	AKRESULT eResult = AK_Success;
	if( in_dwDataChunkSize )
	{
		AkUInt32 ulNumParamSets;
		eResult = m_BankReader.FillDataEx( &ulNumParamSets, sizeof( ulNumParamSets ) );
		if( eResult == AK_Success )
		{
			for( AkUInt32 i = 0; i < ulNumParamSets; ++i )
			{
				AkUniqueID l_EnvID;
				AkUInt32 l_FXID;
				AkUInt32 l_ulPresetSize;
				eResult = m_BankReader.FillDataEx( &l_EnvID, sizeof( l_EnvID ) );
				if( eResult == AK_Success )
				{
					eResult = m_BankReader.FillDataEx( &l_FXID, sizeof( l_FXID ) );
				}
				if( eResult == AK_Success )
				{
					eResult = m_BankReader.FillDataEx( &l_ulPresetSize, sizeof( l_ulPresetSize ) );
				}
				if( eResult == AK_Success && l_ulPresetSize > 0 )
				{
					void * l_pDataBloc = m_BankReader.GetData( l_ulPresetSize );
					if( l_pDataBloc )
					{
						eResult = g_pEnvironmentMgr->AddFXParameterSet( l_EnvID, l_FXID, l_pDataBloc, l_ulPresetSize );
						m_BankReader.ReleaseData();
					}
				}
				//Read RTPCs
				AkUInt32 l_ulRTPCSize;
				if( eResult == AK_Success )
				{
					eResult = m_BankReader.FillDataEx( &l_ulRTPCSize, sizeof( l_ulRTPCSize ) );
				}
				if( eResult == AK_Success )
				{
					for(AkUInt32 i = 0; i < l_ulRTPCSize; ++i)
					{
						AkPluginID l_FXID;
						m_BankReader.FillDataEx( &l_FXID, sizeof( l_FXID ));
						AkRtpcID l_RTPCID;
						m_BankReader.FillDataEx( &l_RTPCID, sizeof( l_RTPCID ));

						// Read ParameterID//ie. Volume, Pitch, LFE...
						AkRTPC_ParameterID l_ParamID;
						m_BankReader.FillDataEx( &l_ParamID, sizeof( l_ParamID ));

						// Read Curve ID
						AkUniqueID rtpcCurveID;
						m_BankReader.FillDataEx( &rtpcCurveID, sizeof( rtpcCurveID ));

						// Read curve scaling type (None, dB...)
						AkCurveScaling eScaling;
						m_BankReader.FillDataEx( &eScaling, sizeof( eScaling ));

						// ArraySize //i.e.:number of conversion points
						AkUInt32 ulSize;
						m_BankReader.FillDataEx( &ulSize, sizeof( ulSize ));

						void * l_pDataBloc = m_BankReader.GetData( ulSize*sizeof(AkRTPCGraphPoint) );
						if( l_pDataBloc )
						{
							eResult = g_pEnvironmentMgr->SetRTPC( l_EnvID, l_RTPCID, l_ParamID, rtpcCurveID, eScaling, (AkRTPCGraphPoint*)l_pDataBloc, ulSize );
							m_BankReader.ReleaseData();
						}
					}
				}

				if( eResult != AK_Success )
				{
					break;
				}
			}
		}
	}
	else
	{
		AKASSERT(!"Invalid FXPR chunk found in the Bank");
	}
	return eResult;
}

AKRESULT CAkBankMgr::ProcessEnvSettingsChunk( AkUInt32 in_dwDataChunkSize )
{
	AKRESULT eResult = AK_Success;

	AKASSERT(g_pStateMgr);
	if(!g_pStateMgr)
	{
		return AK_Fail;
	}

	if( in_dwDataChunkSize )
	{
		for ( int i=0; i<CAkEnvironmentsMgr::MAX_CURVE_X_TYPES; ++i )
		{
			for( int j=0; j<CAkEnvironmentsMgr::MAX_CURVE_Y_TYPES; ++j )
			{
				AkUInt8 bCurveEnabled;
				eResult = m_BankReader.FillDataEx( &bCurveEnabled, sizeof( bCurveEnabled ) );
				if( eResult == AK_Success )
				{
					g_pEnvironmentMgr->SetCurveEnabled( (CAkEnvironmentsMgr::eCurveXType)i, (CAkEnvironmentsMgr::eCurveYType)j, bCurveEnabled ? true : false );
				}

				if( eResult == AK_Success )
				{
					AkCurveScaling l_eCurveScaling;
					eResult = m_BankReader.FillDataEx( &l_eCurveScaling, sizeof( l_eCurveScaling ) );

					AkUInt32 l_ulCurveSize;
					if( eResult == AK_Success )
						eResult = m_BankReader.FillDataEx( &l_ulCurveSize, sizeof( l_ulCurveSize ) );

					if( eResult == AK_Success )
					{
						AkRTPCGraphPoint* aPoints = (AkRTPCGraphPoint *) AkAlloc( g_DefaultPoolId, sizeof( AkRTPCGraphPoint ) * l_ulCurveSize );
						if ( aPoints )
                        {
                            eResult = m_BankReader.FillDataEx( &aPoints[0], sizeof( AkRTPCGraphPoint ) * l_ulCurveSize );
                            if( eResult == AK_Success )
						    {
							    g_pEnvironmentMgr->SetObsOccCurve( (CAkEnvironmentsMgr::eCurveXType)i, (CAkEnvironmentsMgr::eCurveYType)j, l_ulCurveSize, aPoints, l_eCurveScaling );
						    }
						    AkFree( g_DefaultPoolId, aPoints ); //env mgr will have copied the curve
                        }
                        else
                        	eResult = AK_Fail;
					}
				}

				// If failed, quit!
				if( eResult != AK_Success)
				{
					break;
				}
			}

			// If failed, quit!
			if( eResult != AK_Success)
			{
				break;
			}
		}
	}
	else
	{
		AKASSERT(!"Invalid ENVS chunk found in the Bank");
		eResult = AK_Fail;
	}

	return eResult;
}

/////////////////////////////  HIRC PARSERS  ////////////////////////////////////
//
// STATE HIRC PARSER
AKRESULT CAkBankMgr::ReadState( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUniqueID& out_rulStateID, bool in_bIsCustomState, AkStateGroupID in_StateGroupID )
{
	AKRESULT eResult = AK_Success;

	void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	out_rulStateID = ((AkUInt32*)(pData))[0];
	AKASSERT(out_rulStateID);

	CAkState* pState;
	if( in_bIsCustomState )
	{
		pState = g_pIndex->m_idxCustomStates.GetPtrAndAddRef( out_rulStateID );
	}
	else
	{
		pState = g_pIndex->m_idxStates.GetPtrAndAddRef( in_StateGroupID, out_rulStateID );
	}

	if( pState == NULL )
	{
		pState = CAkState::Create( out_rulStateID, in_bIsCustomState, in_StateGroupID );
		if(!pState)
		{
			eResult = AK_Fail;
		}
		else
		{
			CAkFunctionCritical SpaceSetAsCritical;
			eResult = pState->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize );
			if(eResult != AK_Success)
			{
				pState->Release();
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem( in_pUsageSlot, pState ); //This will allow it to be removed on unload
	}

	m_BankReader.ReleaseData();

	return eResult;
}


AKRESULT CAkBankMgr::ReadBus(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot)
{
	AKRESULT eResult = AK_Success;

	void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	AkUniqueID ulID = *(AkUniqueID*)(pData);

	CAkBus* pBus = static_cast<CAkBus*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef(ulID));
	if( pBus == NULL )
	{
		if( !m_bIsFirstBusLoaded )
		{
			// Clearing the Master bus.
			// Why:
			//    The init bank contains the master bus, but it is possible that another
			//    temporary master bus has been added if Wwise has been connected prior to this init bank load bank.
			//    The ID of the Master bus wouldn<t match, and the results would be that global events and information about master bus volume and more be wrong.	
			g_pMasterBus = NULL;
			g_MasterBusCtx.SetBus( NULL );
		}
		pBus = CAkBus::Create(ulID);
		if(!pBus)
		{
			eResult = AK_Fail;
		}
		else
		{
			CAkFunctionCritical SpaceSetAsCritical;
			eResult = pBus->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize );
			if(eResult != AK_Success)
			{
				pBus->Release();
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem( in_pUsageSlot, pBus); //This will allow it to be removed on unload

		//at least one bus has been loaded with success in the init bank
		SetIsFirstBusLoaded( true );
	}

	m_BankReader.ReleaseData();

	return eResult;
}

// ACTION HIRC PARSER
AKRESULT CAkBankMgr::ReadAction(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot)
{
	AKRESULT eResult = AK_Success;

	void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	AkUInt32 ulID = ((AkUInt32*)(pData))[0];
	AkActionType ulActionType = ((AkActionType*)(pData))[1];

	CAkAction* pAction = g_pIndex->m_idxActions.GetPtrAndAddRef(ulID);
	if( pAction == NULL )
	{
		pAction = CAkAction::Create(ulActionType, ulID );
		if(!pAction)
		{
			eResult = AK_Fail;
		}
		else
		{
			CAkFunctionCritical SpaceSetAsCritical;
			eResult = pAction->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize);
			if(eResult != AK_Success)
			{
				pAction->Release();
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem(in_pUsageSlot,pAction); //This will allow it to be removed on unload
	}

	m_BankReader.ReleaseData();

	return eResult;
}

// EVENT HIRC PARSER
AKRESULT CAkBankMgr::ReadEvent(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot)
{
	AKRESULT eResult = AK_Success;

	void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

	if(!pData)
	{
		return AK_Fail;
	}

	AkUniqueID ulID = *(AkUniqueID*)(pData);

	//If the event already exists, simply addref it
	CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef(ulID);
	if( pEvent == NULL )
	{
		// Using CreateNoIndex() instead of Create(), we will do the init right after. 
		// The goal is to avoid the event to be in the index prior it is completely loaded.
		pEvent = CAkEvent::CreateNoIndex(ulID);
		if(!pEvent)
		{
			eResult = AK_Fail;
		}
		else
		{
			CAkFunctionCritical SpaceSetAsCritical;
			eResult = pEvent->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize);
			if(eResult != AK_Success)
			{
				pEvent->Release();
			}
			else
			{
				// And here we proceed with the AddToIndex() call, required since we previously used CreateNoIndex() to create the event.
				pEvent->AddToIndex();
			}
		}
	}

	if(eResult == AK_Success)
	{
		AddLoadedItem(in_pUsageSlot,pEvent); //This will allow it to be removed on unload
	}

	m_BankReader.ReleaseData();

	return eResult;
}
//
//
/////////////////////////////  END OF HIRC PARSERS  ////////////////////////////////////

void CAkBankMgr::ClearPreparedEvents()
{
	AKASSERT( g_pIndex );
	CAkIndexItem<CAkEvent*>& l_rIdx = g_pIndex->m_idxEvents;

	CAkFunctionCritical SpaceSetAsCritical;// To avoid possible deadlocks
	// This function could potentially slow down the audio thread, but this is required to avoid deadlocks.
	AkAutoLock<CAkLock> IndexLock( l_rIdx.m_IndexLock );

	AkHashListBare<AkUniqueID, CAkIndexable, 31>::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
	while( iter != l_rIdx.m_mapIDToPtr.End() )
	{
		CAkEvent* pEvent = static_cast<CAkEvent*>( *iter );
		
		if( pEvent->IsPrepared() )
		{
			pEvent->AddRef();
			UnprepareEvent( pEvent, true );
			++iter;	// Must be done before releasing the event if we unprepared it.
					// It must be done after calling UnprepareEvent as UnprepareEvent may destroy multiple Events and they can all de removed from the index.
			pEvent->Release();
		}
		else
		{
			++iter;
		}
	}
}

AKRESULT CAkBankMgr::LoadMedia( AkFileID in_BankMediaID, AkUInt8* in_pDataBank, CAkUsageSlot* in_pCurrentSlot, AkUInt32 in_uIndexChunkSize, bool in_bIsLoadedFromMemory )
{
	AKRESULT eResult = AK_InsufficientMemory;

	if( in_pCurrentSlot->m_uNumLoadedItems )// m_uNumLoadedItems != 0 means it has already been loaded.
	{
		// skip the chunk
		AkUInt32 ulSkippedBytes;
		m_BankReader.Skip( in_uIndexChunkSize, ulSkippedBytes );
		AKASSERT( ulSkippedBytes == in_uIndexChunkSize );

		eResult = AK_Success;
	}
	else
	{

		AkUInt32 uNumMedias = in_uIndexChunkSize / sizeof( AkMediaHeader );

		AKASSERT( uNumMedias != 0 );

		AkUInt32 uArraySize = uNumMedias * sizeof( AkMediaHeader );

		if( !in_bIsLoadedFromMemory )
		{
			in_pCurrentSlot->m_paLoadedMedia = (AkMediaHeader*)AkAlloc( g_DefaultPoolId, uArraySize );
			if( in_pCurrentSlot->m_paLoadedMedia )
			{
				AKRESULT local_eResult = m_BankReader.FillDataEx( in_pCurrentSlot->m_paLoadedMedia, uArraySize ) ;
				AKASSERT( local_eResult == AK_Success );
			}
		}
		else
		{
			in_pCurrentSlot->m_paLoadedMedia = (AkMediaHeader*)m_BankReader.GetData( uArraySize );
			m_BankReader.ReleaseData();
			AKASSERT( in_pCurrentSlot->m_paLoadedMedia );
		}

		if( in_pCurrentSlot->m_paLoadedMedia )
		{
			AkUInt32 nNumLoadedMedias = 0;
			for( ; nNumLoadedMedias < uNumMedias; ++nNumLoadedMedias )
			{
				AkMediaHeader& rMediaHeader = in_pCurrentSlot->m_paLoadedMedia[ nNumLoadedMedias ];
				{
					AkAutoLock<CAkLock> gate( m_MediaLock );// Will have to decide if performance would be bettre if acquiring the lock outside the for loop.

					bool b_WasAlreadyEnlisted = false;
					AkMediaEntry* pMediaEntry = m_MediaHashTable.Set( rMediaHeader.id, b_WasAlreadyEnlisted );
					if( b_WasAlreadyEnlisted && pMediaEntry->GetPreparedMemoryPointer() != NULL )
					{
						// TODO : this optimization may be source of some confusion.
						// Maybe we should call add alternate bank anyway...
						pMediaEntry->AddRef();
					}
					else if( pMediaEntry )
					{
						pMediaEntry->SetSourceID( rMediaHeader.id );
						eResult = pMediaEntry->AddAlternateBank(
												in_pDataBank + rMediaHeader.uOffset,// Data pointer
												rMediaHeader.uSize,					// Data size
												in_pCurrentSlot						// pUsageSlot
												);

						if( eResult != AK_Success )
						{
							m_MediaHashTable.Unset( rMediaHeader.id );
							break;
						}
						else if( b_WasAlreadyEnlisted )
						{
							pMediaEntry->AddRef();
						}
					}
					else
					{
						break;
					}

					if( pMediaEntry )
					{
						MONITOR_MEDIAPREPARED( *pMediaEntry );
					}
				}

				++( in_pCurrentSlot->m_uNumLoadedItems );
			}

			if( uNumMedias == nNumLoadedMedias ) // if everything loaded successfully.
			{
				eResult = AK_Success;
			}
		}

		if( eResult != AK_Success )
		{
			UnloadMedia( in_pCurrentSlot );
		}
	}

	return eResult;
}

void CAkBankMgr::UnloadMedia( CAkUsageSlot* in_pCurrentSlot )
{
	if( in_pCurrentSlot->m_paLoadedMedia )
	{
		{
			AkAutoLock<CAkLock> gate( m_MediaLock );// Will have to decide if performance would be better if acquiring the lock inside the for loop.
			for( AkUInt32 i = 0; i < in_pCurrentSlot->m_uNumLoadedItems; ++i )
			{
				AkMediaID mediaID = in_pCurrentSlot->m_paLoadedMedia[ i ].id;
	
				AkMediaHashTable::IteratorEx iter = m_MediaHashTable.FindEx( mediaID );
	
				if( iter != m_MediaHashTable.End() )
				{
					AkMediaEntry& rMediaEntry = iter.pItem->Assoc.item;
					rMediaEntry.RemoveAlternateBank( in_pCurrentSlot );
					MONITOR_MEDIAPREPARED( rMediaEntry );

					if( rMediaEntry.Release() == 0 ) // if the memory was released
					{
						m_MediaHashTable.Erase( iter );
					}
				}
			}
		}

		// If the bank was loaded from memory, pPackage->aLoadedMedia was used in-place and do not have to be freed.
		if( !in_pCurrentSlot->WasLoadedFromMemory() )
		{
			AkFree( g_DefaultPoolId, in_pCurrentSlot->m_paLoadedMedia );
		}
		in_pCurrentSlot->m_paLoadedMedia = NULL;
	}
}

AkMediaInfo CAkBankMgr::GetMedia( AkMediaID in_mediaId, CAkPBI* in_pPBI )
{
	AkMediaInfo returnedMediaInfo;
	returnedMediaInfo.pInMemoryData = NULL;
	returnedMediaInfo.uInMemoryDataSize = 0;

	AkAutoLock<CAkLock> gate( m_MediaLock );

	AkMediaEntry* pMediaEntry = m_MediaHashTable.Exists( in_mediaId );

	if( pMediaEntry )
	{
		pMediaEntry->GetMedia( returnedMediaInfo, in_pPBI );
	}

	return returnedMediaInfo;
}

// GetMedia() and ReleaseMedia() calls must be balanced. 
void CAkBankMgr::ReleaseMedia( AkMediaID in_mediaId )
{
	AkAutoLock<CAkLock> gate( m_MediaLock );
	AkMediaHashTable::IteratorEx iter = m_MediaHashTable.FindEx( in_mediaId );

	if( iter != m_MediaHashTable.End() )
	{
		AkMediaEntry& rMediaEntry = iter.pItem->Assoc.item;

		if( rMediaEntry.Release() == 0 ) // if the memory was released
		{
			m_MediaHashTable.Erase( iter );
		}
	}
}

AKRESULT CAkBankMgr::LoadSingleMedia( AkSrcTypeInfo& in_rMediaInfo )
{
	AkMediaEntry* pMediaEntry;

	AkAutoLock<CAkLock> gate( m_MediaLock );

	pMediaEntry = m_MediaHashTable.Exists( in_rMediaInfo.mediaInfo.sourceID );

	if( pMediaEntry )
	{
		pMediaEntry->AddRef();
		if( pMediaEntry->IsDataPrepared() )
		{
			// Data already available, simply return success.
			return AK_Success;
		}
	}
	else
	{
		// add the missing entry
		pMediaEntry = m_MediaHashTable.Set( in_rMediaInfo.mediaInfo.sourceID );
		if( !pMediaEntry )
			return AK_Fail;
		pMediaEntry->SetSourceID( in_rMediaInfo.mediaInfo.sourceID );
	}
	
	AKASSERT( pMediaEntry );

	AkUInt32 uAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;

#ifdef XBOX360
	// on 360, we load banks aligned on AK_BANK_PLATFORM_DATA_ALIGNMENT (2048)
	// this is to support having XMA in any bank
	// on on a sound by sound basis, when no XMA in implied, it is not required.
	if( CODECID_FROM_PLUGINID( in_rMediaInfo.dwID ) != AKCODECID_XMA )
	{
		uAlignment = AK_BANK_PLATFORM_DATA_NON_XMA_ALIGNMENT;
	}
#endif

	if( in_rMediaInfo.mediaInfo.uInMemoryMediaSize == 0 )
	{
		// if the size is 0, well... there is nothing to load.
		return AK_Success;
	}

	AkUInt8* pAllocated = (AkUInt8*)ALLOCWITHHOOK( in_rMediaInfo.mediaInfo.uInMemoryMediaSize, uAlignment );

	AKRESULT eResult = AK_Success;

	// Take the data and copy it over
	if( pAllocated )
	{
		if( pMediaEntry->HasBankSource() )
		{
			pMediaEntry->PrepareFromBank( pAllocated );
		}
		else
		{
			// Stream/Load it
			m_MediaLock.Unlock();// no need to keep the lock during the IO. pMediaEntry is safe as it is already addref'ed at this point.
			eResult = LoadSoundFromFile( in_rMediaInfo, pAllocated );
			m_MediaLock.Lock();
		}
		if( eResult == AK_Success )
		{
			pMediaEntry->SetPreparedData( pAllocated, in_rMediaInfo.mediaInfo.uInMemoryMediaSize );
			AK_PERF_INCREMENT_PREPARED_MEMORY( in_rMediaInfo.mediaInfo.uInMemoryMediaSize );
			MONITOR_MEDIAPREPARED( *pMediaEntry );
			return eResult;
		}
		FREEWITHHOOK( pAllocated );
	}

	
	if( pMediaEntry->Release() == 0 ) // if the memory was released
	{
		m_MediaHashTable.Unset( in_rMediaInfo.mediaInfo.sourceID );
	}
	return  AK_Fail;
}

void CAkBankMgr::ReleaseSingleMedia( AkUniqueID in_SourceID )
{
	AkAutoLock<CAkLock> gate( m_MediaLock );

	AkMediaHashTable::IteratorEx iter = m_MediaHashTable.FindEx( in_SourceID );

	if( iter != m_MediaHashTable.End() )
	{
		AkMediaEntry& rMediaEntry = iter.pItem->Assoc.item;
		if( rMediaEntry.Release() == 0 ) // if the memory was released
		{
			m_MediaHashTable.Erase( iter );
		}
	}
}

void CAkBankMgr::SignalLastBankUnloaded()
{
	CAkFunctionCritical SpaceSetAsCritical;

	// Clear bank states
	g_pStateMgr->RemoveAllStates();
	g_pStateMgr->RemoveAllStateGroups( true );
}

AKRESULT CAkBankMgr::LoadSource(AkUInt8*& io_pData, AkUInt32 &io_ulDataSize, AkBankSourceData& out_rSource)
{
	memset(&out_rSource, 0, sizeof(out_rSource));

	//Read Source info
	out_rSource.m_PluginID = READBANKDATA(AkUInt32, io_pData, io_ulDataSize);

	AkUInt32 StreamType = READBANKDATA(AkUInt32, io_pData, io_ulDataSize);

	AkUInt32 uSampleRate = READBANKDATA(AkUInt32, io_pData, io_ulDataSize);
	AkUInt32 uFormatBits = READBANKDATA(AkUInt32, io_pData, io_ulDataSize);
	SetFormatFromBank( uSampleRate, uFormatBits, out_rSource.m_audioFormat );

	out_rSource.m_MediaInfo.sourceID		= READBANKDATA( AkUInt32, io_pData, io_ulDataSize );
	out_rSource.m_MediaInfo.uFileID		= READBANKDATA( AkFileID, io_pData, io_ulDataSize );
	if ( StreamType != SourceType_Streaming )
	{
		out_rSource.m_MediaInfo.uFileOffset		 = READBANKDATA( AkUInt32, io_pData, io_ulDataSize );//in bytes
		out_rSource.m_MediaInfo.uInMemoryMediaSize = READBANKDATA( AkUInt32, io_pData, io_ulDataSize );//in bytes
	}
	else
	{
		out_rSource.m_MediaInfo.uFileOffset = 0;
		out_rSource.m_MediaInfo.uInMemoryMediaSize = 0;
	}			
	out_rSource.m_MediaInfo.bIsLanguageSpecific = READBANKDATA( AkUInt8, io_pData, io_ulDataSize ) != 0;
	out_rSource.m_MediaInfo.bPrefetch		= ( StreamType == SourceType_PrefetchStreaming );

	AkPluginType PluginType = (AkPluginType) ( out_rSource.m_PluginID & AkPluginTypeMask );

	if( PluginType == AkPluginTypeCodec )
	{
		if ( StreamType == SourceType_Data )
		{
			// In-memory source.
			out_rSource.m_MediaInfo.Type = SrcTypeMemory;
		}
		else
		{
			// Streaming file source.
			if( StreamType != SourceType_Streaming &&
				StreamType != SourceType_PrefetchStreaming )
			{
				return AK_Fail;
			}
			out_rSource.m_MediaInfo.Type = SrcTypeFile;
		}
	}
	// Set source (according to type).
	else if ( PluginType == AkPluginTypeMotionSource || PluginType == AkPluginTypeSource )
	{
		out_rSource.m_uSize = READBANKDATA(AkUInt32, io_pData, io_ulDataSize);
		out_rSource.m_pParam = io_pData;

		// Skip BLOB.
		io_pData += out_rSource.m_uSize;
		io_ulDataSize -= out_rSource.m_uSize;
	}
	else if ( PluginType != AkPluginTypeNone )
	{
		//invalid PluginType
		//do not assert here as this will cause a pseudo-deadlock
		//if we are loading the banks in synchronous mode (WG-1592)
		return AK_Fail;
	}

	return AK_Success;
};

///////////////////////////////////////////////////////////////////////////////////////
// class AkMediaEntry
///////////////////////////////////////////////////////////////////////////////////////

AkMemPoolId AkMediaEntry::m_uPrepareEventMemoryPoolID;

void AkMediaEntry::AddRef()
{
	++uRefCount;
}

AkUInt32 AkMediaEntry::Release()
{
	AKASSERT( uRefCount );// Should never be 0 at this point
	if( !(--uRefCount) )
	{
		if( m_mediaInfo.pInMemoryData )
		{
			FREEWITHHOOK( m_mediaInfo.pInMemoryData );
			m_mediaInfo.pInMemoryData = NULL;
			AK_PERF_DECREMENT_PREPARED_MEMORY( m_mediaInfo.uInMemoryDataSize );
		}
		MONITOR_MEDIAPREPARED( *this );
	}
	return uRefCount;
}

AKRESULT AkMediaEntry::AddAlternateBank( AkUInt8* in_pData, AkUInt32 in_uSize, CAkUsageSlot* in_pSlot )
{
	m_mediaInfo.uInMemoryDataSize = in_uSize;//reset size everytime, even if it is the same, faster than checking.
	return m_BankSlots.Set( in_pSlot, in_pData ) ? AK_Success: AK_InsufficientMemory;
}

void AkMediaEntry::RemoveAlternateBank( CAkUsageSlot* in_pSlot )
{
	m_BankSlots.UnsetSwap( in_pSlot );
}

void AkMediaEntry::GetMedia( AkMediaInfo& out_mediaInfo, CAkPBI* in_pPBI )
{
	out_mediaInfo = m_mediaInfo;

	AddRef();

	if( m_mediaInfo.pInMemoryData == NULL )
	{
		// No dynamic data available, check for data from bank.
		if( m_BankSlots.Length() != 0 )
		{
			CAkUsageSlot* pSlot = m_BankSlots[0].key;
			out_mediaInfo.pInMemoryData = m_BankSlots[0].item;

			in_pPBI->SetUsageSlotToRelease( pSlot );
			pSlot->AddRef(); // AddRef the Slot as one sound will be using it
		}
		else
		{
			// We end here in case of error or if data was not ready.

			// No AddRef() if not enough memory to trace it. So we simply release.
			// or
			// The entry is in the table, but no data is not yet ready.
			out_mediaInfo.pInMemoryData = NULL;
			out_mediaInfo.uInMemoryDataSize = 0;
			Release();// cancel the AddRef done at the beginning of the function.
		}
	}
}

void AkMediaEntry::PrepareFromBank( AkUInt8* in_pAllocatedData )
{
	AKASSERT( !m_mediaInfo.pInMemoryData );
	AKASSERT( m_BankSlots.Length() != 0 );
	memcpy( in_pAllocatedData, m_BankSlots[0].item, m_mediaInfo.uInMemoryDataSize );

#ifdef RVL_OS
	DCStoreRange( in_pAllocatedData, m_mediaInfo.uInMemoryDataSize );
#endif	
}


