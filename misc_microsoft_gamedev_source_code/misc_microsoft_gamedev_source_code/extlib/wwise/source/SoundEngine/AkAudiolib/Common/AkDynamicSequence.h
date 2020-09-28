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
// AkDynamicSequence.h
//
//////////////////////////////////////////////////////////////////////

#ifndef AKDYNAMICSEQUENCE_H
#define AKDYNAMICSEQUENCE_H

#include <AK/SoundEngine/Common/AkDynamicSequence.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkDecisionTree.h"
#include "AkPoolSizes.h"
#include "PrivateStructures.h"
#include "AkPBIAware.h"
#include "AkAudioMgr.h"
#include "AkAudioNode.h"

class AkPBI;
class AkPBIAware;
class CAkRegisteredObj;

class CAkDynamicSequence : public CAkPBIAware
{
public:
	enum AkDynamicSequenceState
	{
		State_Stopped,
		State_Playing,
		State_Waiting
	};

	//Thread safe version of the constructor
	static CAkDynamicSequence* Create( AkPlayingID in_PlayingID, AK::SoundEngine::DynamicSequence::DynamicSequenceType in_eDynamicSequenceType );

	AKRESULT Init();

	// AkCommonBase interface
	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	// Add to the general index
	void AddToIndex();

	// Remove from general index
	void RemoveFromIndex();

	AkPlayingID GetPlayingID() const { return m_userParams.PlayingID; }
	void SetGameObject( CAkRegisteredObj* in_pGameObj );

	AkUniqueID GetNextToPlay( AkTimeMs & out_delay );

	// Transport function
	AKRESULT Play();
	AKRESULT Stop();
	AKRESULT Break();
	AKRESULT Pause();
	AKRESULT Resume();
	void Close(){ m_bClosed = true; }

	bool WasClosed(){ return m_bClosed; }

	void ResumeWaiting();

	// CAkPBIAware interface
	virtual CAkPBI* CreatePBI( CAkSoundBase* in_pSound, CAkSource* in_pSource, AkPBIParams& in_rPBIParams, AkPriority in_priority ) const;

	AkUniqueID PlayingNodeID() const { return m_playingItem.audioNodeID; }
	void* PlayingNodeCustomInfo() const { return m_playingItem.pCustomInfo; }

	AK::SoundEngine::DynamicSequence::Playlist * LockPlaylist()
	{
		m_lockPlaylist.Lock();
		return &m_playList;
	}

	void UnlockPlaylist();

	void AllExec( ActionParamType in_eType, CAkRegisteredObj * in_pGameObj );

protected:

	enum AkDynamicSequenceStopMode
	{
		AK_StopImmediate		= 0,	///<  Stop now (with minimum fade out time)
		AK_StopAfterCurrentItem	= 1,	///<  Stop when next item has completed
	};

	// Constructors
    CAkDynamicSequence( AkPlayingID in_PlayingID, AK::SoundEngine::DynamicSequence::DynamicSequenceType in_eDynamicSequenceType );

	//Destructor
    virtual ~CAkDynamicSequence();

	AKRESULT _PlayNode( AkUniqueID in_nodeID, AkTimeMs in_delay );
	AKRESULT _Stop( AkDynamicSequenceStopMode in_eStopMode );
	AkUniqueID _GetNextToPlay( AkTimeMs & out_delay );

	void _StopNoPropagation();
	void _PauseNoPropagation();
	void _ResumeNoPropagation();

	AK::SoundEngine::DynamicSequence::Playlist m_playList;
	CAkLock m_lockPlaylist;

	AkDynamicSequenceState m_eState;

	CAkRegisteredObj*	m_pGameObj;
	UserParams			m_userParams;
	bool				m_bClosed;

	AK::SoundEngine::DynamicSequence::DynamicSequenceType m_eDynamicSequenceType;

	AK::SoundEngine::DynamicSequence::PlaylistItem m_playingItem;
	AK::SoundEngine::DynamicSequence::PlaylistItem m_queuedItem;

	AkUInt32 m_ulPauseCount;
	AkUniqueID m_uSequenceID;
};
#endif // AKDYNAMICSEQUENCE_H
