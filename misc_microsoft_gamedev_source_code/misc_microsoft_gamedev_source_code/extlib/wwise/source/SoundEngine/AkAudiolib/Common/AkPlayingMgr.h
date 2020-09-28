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
// AkPlayingMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _PLAYING_MGR_H_
#define _PLAYING_MGR_H_

#include "AkCommon.h"
#include "AkList2.h"
#include "AkHashList.h"
#include <AK/Tools/Common/AkLock.h>
#include <AK/SoundEngine/Common/AkCallback.h>

class CAkContinueListItem;
class CAkPBI;
class CAkPBIAware;
class IAkTransportAware;
class CAkRegisteredObj;
struct AkQueuedMsg_EventBase;
struct AkQueuedMsg_Event;
struct AkQueuedMsg_OpenDynamicSequence;

class CAkPlayingMgr : public CAkObject
{
public:
	AKRESULT Init();
	void Term();

	// Add a playing ID in the "Under observation" event list
	AKRESULT AddPlayingID( 
		AkQueuedMsg_EventBase & in_event,
		AkCallbackFunc in_pfnCallback,
		void * in_pCookie,
		AkUInt32 in_uiRegisteredNotif,
		AkUniqueID in_id
		);

	// Ask to not get notified for the given cookie anymore.
	void CancelCallbackCookie( void* in_pCookie );

	// Ask to not get notified for the given Playing ID anymore.
	void CancelCallback( AkPlayingID in_playingID );

	// Set the actual referenced PBI
	AKRESULT SetPBI(
		AkPlayingID in_PlayingID,	// Playing ID
		IAkTransportAware* in_pPBI	// PBI pointer, only to be stored
		);

	void Remove(
		AkPlayingID in_PlayingID,	// Playing ID
		IAkTransportAware* in_pPBI	// PBI pointer
		);

	bool IsActive(
		AkPlayingID in_PlayingID // Playing ID
		);

	void AddItemActiveCount(
		AkPlayingID in_PlayingID // Playing ID
		);

	void RemoveItemActiveCount(
		AkPlayingID in_PlayingID // Playing ID
		);

#ifndef AK_OPTIMIZED
	void StopAndContinue(
		AkPlayingID			in_PlayingID,
		CAkRegisteredObj *	in_pGameObj,
		CAkContinueListItem&in_ContinueListItem,
		AkUniqueID			in_ItemToPlay,
		AkUInt16			in_wPosition,
        CAkPBIAware*        in_pInstigator
		);
#endif

	void GetNotificationInformation( 
		CAkPBI* in_pPBI,
		bool* out_pWantMarkerInformation,
		bool* out_pWantPositionInformation
		);

	void NotifyEndOfDynamicSequenceItem(
		CAkPBI* in_pPBI,
		AkUniqueID in_itemID,
		void* in_pCustomInfo
		);

	void NotifyMarker(
		CAkPBI* in_pPBI,					// PBI pointer
		AkAudioMarker* in_pMarkerInfo	// Marker being notified
		);

	void NotifyMarkers( AkBufferMarker*& pCurrMarker, AkUInt16 & in_uNumMarkers );

private:
	struct PlayingMgrItem
	{
		// Hold a list of PBI with Wwise, only keep count otherwise
#ifndef AK_OPTIMIZED
		typedef CAkList2<IAkTransportAware*, IAkTransportAware*, AkAllocAndFree> AkPBIList;
		AkPBIList m_PBIList;
#else
		int cPBI;
#endif

		int cAction; // count of actions

		AkUniqueID uniqueID;
		AkGameObjectID GameObj;
		AkCustomParamType CustomParam;
		AkCallbackFunc pfnCallback;
		void* pCookie;
		AkUInt32 uiRegisteredNotif;
	};

	void CheckRemovePlayingID( AkPlayingID in_PlayingID, PlayingMgrItem* in_pItem );

	typedef AkHashList<AkPlayingID, PlayingMgrItem, 31> AkPlayingMap;
	AkPlayingMap m_PlayingMap;
	CAkLock m_csLock;
};

extern CAkPlayingMgr*			g_pPlayingMgr;

#endif
