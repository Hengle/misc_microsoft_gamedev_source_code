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
// AkBankMgr.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _BANK_MGR_H_
#define _BANK_MGR_H_

#include "AkBanks.h"
#include "AkBankReader.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include <AK/Tools/Common/AkLock.h>
#include "AkIDStringMap.h"
#include "AkAudioLibIndex.h"// PS3 requires it for unused templates. compiler specific problem.
#include "AkCritical.h"
#include "AkSource.h"
#include "AkList2.h"
#include "AkBankCallbackMgr.h"

class CAkBankMemMgr;
class CAkParameterNode;
class CAkUsageSlot;
class CAkBankMgr;

using namespace AKPLATFORM;
using namespace AkBank;

#define AK_MINIMUM_BANK_SIZE ( sizeof(AkSubchunkHeader) + sizeof( AkBankHeader ) )

// Could be the ID of the SoundBase object directly, maybe no need to create it.
typedef AkUInt32 AkMediaID; // ID of a specific source

extern CAkBankMgr*		g_pBankManager;

struct AkMediaInfo
{
	AkUInt8*			pInMemoryData;
	AkUInt32			uInMemoryDataSize;
};

class AkMediaEntry
{
	friend class AkMonitor; // for profiling purpose.
private:
	typedef CAkKeyArray< CAkUsageSlot*, AkUInt8* > AkBankSlotsArray;

public:
	// Constructor
	AkMediaEntry()
		:uRefCount( 1 )
	{
		m_mediaInfo.pInMemoryData = NULL;
		m_mediaInfo.uInMemoryDataSize = 0;
	}

	~AkMediaEntry()
	{
		AKASSERT( m_BankSlots.Length() == 0);
		m_BankSlots.Term();
	}

	void AddRef();
	AkUInt32 Release();

	void GetMedia( AkMediaInfo& out_mediaInfo, CAkPBI* in_pPBI );

	AKRESULT AddAlternateBank( AkUInt8* in_pData, AkUInt32 in_uSize, CAkUsageSlot* in_pSlot );
	void RemoveAlternateBank( CAkUsageSlot* in_pSlot );

	bool IsDataPrepared()
	{ 
		return m_mediaInfo.pInMemoryData != NULL; 
	}

	void SetPreparedData( AkUInt8* in_pData, AkUInt32 in_uSize )
	{
		m_mediaInfo.uInMemoryDataSize = in_uSize;
		m_mediaInfo.pInMemoryData = in_pData;
	}

	AkUInt8* GetPreparedMemoryPointer()
	{ 
		return m_mediaInfo.pInMemoryData; 
	}

	bool HasBankSource()
	{
		return (m_BankSlots.Length() != 0);
	}

	AkUInt32 GetNumBankOptions()
	{
		return m_BankSlots.Length();
	}

	void PrepareFromBank( AkUInt8* in_pAllocatedData );

	AkUniqueID GetSourceID(){ return m_sourceID; }
	void SetSourceID( AkUniqueID in_sourceID ){ m_sourceID = in_sourceID; }

	static void SetPrepareMemPoolID( AkMemPoolId in_memPoolID )
	{
		if( in_memPoolID != AK_INVALID_POOL_ID )
		{
			AKASSERT( (AK::MemoryMgr::GetPoolAttributes( in_memPoolID )&AkBlockMgmtMask) != AkFixedSizeBlocksMode );
			m_uPrepareEventMemoryPoolID = in_memPoolID;
		}
		else
		{
			m_uPrepareEventMemoryPoolID = g_DefaultPoolId;
		}
	}

	static AkMemPoolId GetPrepareMemPoolID()
	{
		return m_uPrepareEventMemoryPoolID;
	}

private:

	// Members
	AkMediaInfo m_mediaInfo;

	AkBankSlotsArray m_BankSlots;

	AkUInt32 uRefCount;

	AkUniqueID m_sourceID;

	static AkMemPoolId m_uPrepareEventMemoryPoolID;
};

struct AkMediaHeader
{
	AkMediaID id;
	AkUInt32 uOffset;
	AkUInt32 uSize;
};

struct AkBankCompletionNotifInfo
{
	AkBankCallbackFunc pfnBankCallback;
    void * pCookie;
};

struct AkBankCompletionItem
{
	AkUniqueID BankID;
	AkMemPoolId memPoolId;
};

struct AkPrepareEventQueueItemLoad
{
	// TODO use the pointer instead of the ID here, will simplify things...
	AkUInt32	numEvents;
	// To avoid doing 4 bytes allocation, when the size is one, the ID itself will be passed instead of the array.
	union
	{
		AkUniqueID	eventID;
		AkUniqueID* pEventID;
	};
};

struct AkPrepareGameSyncQueueItem
{
	AkGroupType eGroupType;
	AkUInt32	uGroupID;
	bool		bSupported;

	AkUInt32	uNumGameSync;
	union
	{
		AkUInt32 uGameSyncID;
		AkUInt32* pGameSyncID;
	};
};

class CAkUsageSlot
	: public CAkObject
{
public:

	AkUniqueID	m_BankID;
	AkUInt8 *	m_pData;
	AkUInt32	m_uLoadedDataSize;
	AkMemPoolId m_memPoolId;
	bool		m_bIsInternalPool;

	AkUInt32		m_uNumLoadedItems;
	AkMediaHeader*	m_paLoadedMedia;
	
	// These two members are there only for the notify completion callback
    AkBankCallbackFunc m_pfnBankCallback;
    void * m_pCookie;

	typedef AkArray<CAkIndexable*, CAkIndexable*, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(CAkIndexable*)> AkListLoadedItem;
	AkListLoadedItem m_listLoadedItem;	//	Contains the list of CAkIndexable to release in case of unload

	CAkUsageSlot( AkUniqueID in_BankID, AkMemPoolId in_memPoolId, AkInt32 in_mainRefCount, AkInt32 in_prepareRefCount );
	~CAkUsageSlot();

	void AddRef();
	void Release( bool in_bSkipNotification );
	void AddRefPrepare();
	void ReleasePrepare();

	void StopContent();
	void RemoveContent();
	void UnloadPre();
	void Unload();

	bool WasLoadedAsABank(){ return m_bWasLoadedAsABank; }
	void WasLoadedAsABank( bool in_bWasLoadedAsABank ){ m_bWasLoadedAsABank = in_bWasLoadedAsABank; }

	bool WasLoadedFromMemory() { return m_bWasLoadedFromMemory; }
	void WasLoadedFromMemory(bool in_bWasLoadedFromMemory ){ m_bWasLoadedFromMemory = in_bWasLoadedFromMemory; }

	void UnloadCompletionNotification();

private:
	AkInt32 m_iRefCount;
	AkInt32 m_iPrepareRefCount;

	bool m_bWasLoadedAsABank;
	bool m_bWasLoadedFromMemory;
};

enum AkBankLoadFlag
{
	AkBankLoadFlag_None,
	AkBankLoadFlag_InMemory,
	AkBankLoadFlag_UsingFileID,
};

struct AkBankSourceData
{
	AkUniqueID			m_srcID; 
	AkUInt32			m_PluginID;
	AkMediaInformation	m_MediaInfo;
	AkAudioFormat		m_audioFormat;
	void *				m_pParam;
	AkUInt32			m_uSize;
};

// The Bank Manager
//		Manage bank slots allocation
//		Loads and unloads banks
class CAkBankMgr : public CAkObject
{
	friend class CAkUsageSlot;
	friend class AkMonitor;
public:
	enum AkBankQueueItemType
	{
		QueueItemLoad,
		QueueItemUnload,
		QueueItemPrepareEvent,
		QueueItemUnprepareEvent,
		QueueItemSupportedGameSync,
		QueueItemUnprepareAllEvents,
		QueueItemClearBanks
	};

	// Structure used to Store the information about a loaded bank
	// This information will mostly be used to identify a free Slot and keep information
	// on what has to be removed from the hyerarchy once removed

	struct AkBankQueueItemLoad
		: public AkBankCompletionItem
	{
		void* pInMemoryBank;
		AkUInt32 ui32InMemoryBankSize;
		AkBankLoadFlag bankLoadFlag;
	};

	struct AkBankQueueItem
	{
		AkBankQueueItemType eType;
		AkBankCompletionNotifInfo callbackInfo;
		union
		{
			AkPrepareEventQueueItemLoad prepare;
			AkBankQueueItemLoad load;
			AkPrepareGameSyncQueueItem gameSync;
		};
	};

	// Constructor and Destructor
	CAkBankMgr();
	~CAkBankMgr();

	AKRESULT Init();
	AKRESULT Term();

	AKRESULT QueueBankCommand( AkBankQueueItem in_Item );

	void NotifyCompletion( AkBankQueueItem & in_rItem, AKRESULT in_OperationResult );

	AkForceInline bool IsCancelled() { return m_bCancelFlag; }

	void	 ClearPreparedEvents();

	AKRESULT SetBankLoadIOSettings( AkReal32 in_fThroughput, AkPriority in_priority ) { return m_BankReader.SetBankLoadIOSettings( in_fThroughput, in_priority ); }

	void StopBankContent( AkUniqueID in_BankID );

	CAkLock m_BankListLock;

	// This template is to be used by all STD audionodes
	// Any audio node that has special requirements on bank loading ( as sounds and track ) should not use this template.
	// It is exposed in header file so that external plugins can use this template too
	template <class T_Type, class T_Index_Type>
	AKRESULT StdBankRead( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, CAkIndexItem<T_Index_Type*>& in_rIndex )
	{
		AKRESULT eResult = AK_Success;

		void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );

		if(!pData)
		{
			return AK_Fail;
		}

		AkUniqueID ulID = *(AkUniqueID*)( pData );

		T_Type* pObject = static_cast<T_Type*>( in_rIndex.GetPtrAndAddRef( ulID ) );
		if( pObject == NULL )
		{
			pObject = T_Type::Create(ulID);
			if( !pObject )
			{
				eResult = AK_Fail;
			}
			else
			{
				CAkFunctionCritical SpaceSetAsCritical;
				eResult = pObject->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize );
				if( eResult != AK_Success )
				{
					pObject->Release();
				}
			}
		}

		if(eResult == AK_Success)
		{
			AddLoadedItem( in_pUsageSlot,pObject ); //This will allow it to be removed on unload
		}

		m_BankReader.ReleaseData();

		return eResult;
	}

	template<class T>
	AKRESULT ReadSourceParent(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID)
	{
		AKRESULT eResult = AK_Success;

		void* pData = m_BankReader.GetData( in_rSection.dwSectionSize );
		if(!pData)
			return AK_Fail;

		AkUniqueID ulID = *(AkUniqueID*)(pData);

		T* pSound = static_cast<T*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef( ulID ) );
		if( pSound )
		{
			//The sound already exists, simply update it
			if( pSound->SourceLoaded() || !pSound->HasBankSource() )
			{
				CAkFunctionCritical SpaceSetAsCritical;
				eResult = pSound->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize, in_pUsageSlot, true );
			}

			if(eResult != AK_Success)
				pSound->Release();
		}
		else
		{
			pSound = T::Create(ulID);
			if(!pSound)
			{
				eResult = AK_Fail;
			}
			else
			{
				CAkFunctionCritical SpaceSetAsCritical;
				eResult = pSound->SetInitialValues( (AkUInt8*)pData, in_rSection.dwSectionSize, in_pUsageSlot, false );
				if(eResult != AK_Success)
				{
					pSound->Release();
				}
			}
		}

		if(eResult == AK_Success)
		{
			AddLoadedItem( in_pUsageSlot, pSound ); //This will allow it to be removed on unload
		}

		m_BankReader.ReleaseData();

		return eResult;
	}

	static AKRESULT LoadSource(AkUInt8* & io_pData, AkUInt32 &io_ulDataSize, AkBankSourceData& out_rSource);

	friend AKRESULT ReadTrack( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID );

	AKRESULT LoadSoundFromFile( AkSrcTypeInfo& in_rMediaInfo, AkUInt8* io_pData );


	AKRESULT LoadMedia( AkFileID in_BankMediaID, AkUInt8* in_pDataBank, CAkUsageSlot* in_pCurrentSlot, AkUInt32 in_uIndexChunkSize, bool in_bIsLoadedFromMemory );
	void UnloadMedia( CAkUsageSlot* in_pCurrentSlot );	// Works to cancel both Load and index.

	// Returns the best it can do. 
	// If the media is in memory, give it in memory.
	// If the media is prefetched, give the prefetch.
	AkMediaInfo GetMedia( AkMediaID in_mediaId, CAkPBI* in_pPBI );

	// GetMedia() and ReleaseMedia() calls must be balanced. 
	void ReleaseMedia( AkMediaID in_mediaId );

	AKRESULT LoadSingleMedia( AkSrcTypeInfo& in_rmediaInfo );
	void ReleaseSingleMedia( AkUniqueID in_SourceID );

	void SetIsFirstBusLoaded( bool in_Loaded ){ m_bIsFirstBusLoaded = in_Loaded; }

	static void SignalLastBankUnloaded();

	static AkBankID GetBankIDFromInMemorySpace( void* in_pData, AkUInt32 in_uSize );

	const char* GetBankFileName( AkBankID in_bankID );

	void UpdateBankName( AkBankID in_bankID, AkChar* in_pStringWithoutExtension );

	static bool BankHasFeedback() {return g_pBankManager->m_bFeedbackInBank;}

	// Safe way to actually do the callback
	// Same prototype than the callback itself, with the exception that it may actually not do the callback if the
	// event was cancelled
	void DoCallback(
		AkBankCallbackFunc	in_pfnBankCallback,
		AkBankID			in_bankID,
		AKRESULT			in_eLoadResult,
		AkMemPoolId			in_memPoolId,
		void *				in_pCookie
		)
	{
		m_CallbackMgr.DoCallback( 
			in_pfnBankCallback,
			in_bankID,
			in_eLoadResult,
			in_memPoolId,
			in_pCookie 
			);
	}

	void CancelCookie( void* in_pCookie ){ m_CallbackMgr.CancelCookie( in_pCookie ); }

private:

	// Load the Specified bank
	AKRESULT LoadBank( AkBankQueueItem in_Item, CAkUsageSlot *& out_pUsageSlot, bool in_bLoadingMedia );	

	AKRESULT LoadBankPre( AkBankQueueItem& in_rItem );
	// Called upon an unload request, the Unload may be delayed if the Bank is un use.
	AKRESULT UnloadBankPre( AkBankQueueItem in_Item );

	AKRESULT ClearBanksInternal( AkBankQueueItem in_Item );

	AKRESULT PrepareEvents( AkBankQueueItem in_Item );
	AKRESULT PrepareEvent( AkBankQueueItem in_Item, AkUniqueID in_EventID );
	AKRESULT UnprepareEvents( AkBankQueueItem in_Item );
	AKRESULT UnprepareEvent( AkUniqueID in_EventID );
	void	 UnprepareEvent( CAkEvent* in_pEvent, bool in_bCompleteUnprepare = false );
	AKRESULT UnprepareAllEvents( AkBankQueueItem in_Item );

	AKRESULT PrepareGameSync( AkBankQueueItem in_Item );

	// WG-9055 - do not pass AkBankQueueItem in_rItem by reference.
	AKRESULT PrepareHierarchy( AkBankQueueItem in_rItem, AkFileID in_FileID );
	void UnPrepareHierarchy( AkUniqueID in_BankID );

	//NOT TO BE CALLED DIRECTLY, internal tool only
	void UnloadAll();

	//Add the CAkIndexable* to the ToBeRemoved list associated with this slot
	void AddLoadedItem( CAkUsageSlot* in_pUsageSlot, CAkIndexable* in_pIndexable);

	AKRESULT ProcessBankHeader( AkBankHeader& in_rBankHeader );
	AKRESULT ProcessDataChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ProcessHircChunk( CAkUsageSlot* in_pUsageSlot, AkUInt32 in_dwBankID );
	AKRESULT ProcessStringGroup( AkUInt32 & io_dwDataChunkSize, AkGroupStringIdx & in_group );
	AKRESULT ProcessStringMappingChunk( AkUInt32 in_dwDataChunkSize, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ProcessGlobalSettingsChunk( AkUInt32 in_dwDataChunkSize , CAkUsageSlot* in_pUsageSlot );
	AKRESULT ProcessFxParamsChunk( AkUInt32 in_dwDataChunkSize );
	AKRESULT ProcessEnvSettingsChunk( AkUInt32 in_dwDataChunkSize );

	AKRESULT ReadState( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot, AkUniqueID& out_rulStateID, bool in_bIsCustomState, AkStateGroupID in_StateGroupID );
	AKRESULT ReadAction( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ReadEvent( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot );
	
	AKRESULT ReadBus( const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot );
	AKRESULT ReadFeedbackBus(const AKBKSubHircSection& in_rSection, CAkUsageSlot* in_pUsageSlot);

	CAkBankReader m_BankReader;

	AKRESULT SetFileReader( AkFileID in_FileID, AkUInt32 in_uFileOffset, AkUInt32 in_codecID );

// threading
    static AK_DECLARE_THREAD_ROUTINE( BankThreadFunc );
	void StopThread();
	AKRESULT StartThread();

	AKRESULT ExecuteCommand();

	static AkThread	m_BankMgrThread;

	AkEvent m_eventQueue;
	bool	m_bStopThread;


	CAkLock m_queueLock;
	CAkLock m_MediaLock;
	
	AkUniqueID m_BankIDBeingProcessed;
	bool m_bCancelFlag;

	typedef CAkList2<AkBankQueueItem, const AkBankQueueItem&, AkAllocAndFree> AkBankQueue;
	AkBankQueue m_BankQueue;

	void EnsureRemoveLoadedBankItem( AkBankID in_BankID ){ m_ListLoadedBanks.Unset(in_BankID); };

	typedef CAkKeyArray<AkBankID, CAkUsageSlot *> AkListLoadedBanks;
	AkListLoadedBanks m_ListLoadedBanks;

	bool m_bIsFirstBusLoaded;

	// Hash table containing all ready media.
	// Actual problem : some of this information is a duplicate of the index content. but it would be a faster
	// seek in a hash table than in the linear indexes.
	// We could find better memory usage, but will be more complex too.
	typedef AkHashList< AkMediaID, AkMediaEntry, 31 > AkMediaHashTable;
	AkMediaHashTable m_MediaHashTable;

	void FlushFileNameTable();

	typedef AkHashList< AkBankID, char*, 31 > AkIDtoStringHash;
	AkIDtoStringHash m_BankIDToFileName;

	bool m_bFeedbackInBank;

	CAkBankCallbackMgr m_CallbackMgr;
};

#endif



