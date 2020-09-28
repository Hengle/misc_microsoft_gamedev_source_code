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
// AkMusicTrack.h
//
// Class for music track node.
// The music track is not a music node. It cannot be played, and does
// not implement a context creation method. However it is an active
// parent because it has children sounds, and propagates notifications
// to them. Only MusicSegment can be a parent of MusicTrack.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_TRACK_H_
#define _MUSIC_TRACK_H_

#include "AkSoundBase.h"
#include "AkMusicStructs.h"
#include "AkSource.h"

// -------------------------------------------------------------------
// Structures.
// -------------------------------------------------------------------
struct AkTrackSrc
{
	AkUniqueID	id;
    AkUInt32    uClipStartPosition;	// Clip start position, relative to beginning of the track.
    AkUInt32    uClipDuration;		// Clip duration.
	AkUInt32    uSrcDuration;		// Source's original duration.
    AkInt32     iSourceTrimOffset;	// Source offset at the beginning of the clip (may be positive or negative, 
									// but always |iSourceTrimOffset| < OriginalSourceDuration).
};

// -------------------------------------------------------------------
// Name: CAkMusicTrack
// Desc: Track audio node.
// -------------------------------------------------------------------
class CAkMusicTrack : public CAkSoundBase
{
public:
    // Thread safe version of the constructor.
	static CAkMusicTrack * Create(
        AkUniqueID in_ulID = 0
        );

	AKRESULT SetInitialValues( AkUInt8* pData, AkUInt32 ulDataSize, CAkUsageSlot* in_pUsageSlot, bool in_bIsPartialLoadOnly );

	// Return the node category.
	virtual AkNodeCategory NodeCategory();

    // Play the specified node
    // NOT IMPLEMENTED.
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT Play( AkPBIParams& in_rPBIParams );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );

    // Wwise specific interface.
    // -----------------------------------------
	AKRESULT AddPlaylistItem(
		AkTrackSrcInfo &in_srcInfo
		);

	AKRESULT SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		);

    AKRESULT AddSource(
		AkUniqueID      in_srcID,
        AkLpCtstr       in_pszFilename,
        AkPluginID      in_pluginID,
        AkAudioFormat & in_audioFormat
        );

	AKRESULT AddSource( 
		AkUniqueID in_srcID, 
		AkUInt32 in_pluginID, 
		AkMediaInformation in_MediaInfo, 
		AkAudioFormat in_audioFormat
		);

	AKRESULT AddPluginSource( 
		AkUniqueID	in_srcID,
		AkPluginID in_ulID, 
		void * in_pParam, 
		AkUInt32 in_uSize 
		);

	AKRESULT SetSrcParam(					// Set the parameter on an physical model source.
		AkUniqueID      in_srcID,
		AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
		AkPluginParamID in_ulParamID,		// Parameter id.
		void *			in_pParam,			// Pointer to a setup param block.
		AkUInt32		in_ulSize			// Size of the parameter block.
		);

    // Note: Identifying sources by sound ID is not good since the same source can be used at various places.
    void RemoveSource( 
        AkUniqueID      in_srcID
        );

	bool HasBankSource();

	bool SourceLoaded(){ return !m_arSrcInfo.IsEmpty(); }

    void RemoveAllSources();

	void IsZeroLatency( bool in_bIsZeroLatency );

	void LookAheadTime( AkTimeMs in_LookAheadTime );

	virtual AkObjectCategory Category();

	// Like ParameterNodeBase's, but does not check parent.
	bool GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes );

    // Interface for Contexts
    // ----------------------

	CAkSource* GetSourcePtr( AkUniqueID in_SourceID );

	typedef CAkMultiKeyList<AkUInt32, AkTrackSrc, AkAllocAndKeep> TrackPlaylist;

	TrackPlaylist* GetTrackPlaylist(){ return &m_mmapTrackPlaylist; }

	void SetMusicTrackRanSeqType( AkMusicTrackRanSeqType in_eType ){ m_eRSType = in_eType; };

	AkUInt16 GetNextRS();

	virtual AKRESULT PrepareData();
	virtual void UnPrepareData();

protected:
    CAkMusicTrack( 
        AkUniqueID in_ulID
        );
    virtual ~CAkMusicTrack();

    AKRESULT Init() { return CAkSoundBase::Init(); }

    // Array of source descriptors.
    // Note: AkTrackSrcInfo is the internal representation of audio source positions in tracks: in samples.
    // Loop count is stored in the Sound object.
    // TODO Optimize: store sorted by Play At.
	typedef CAkKeyArray<AkUniqueID, CAkSource*> SrcInfoArray;
    SrcInfoArray    m_arSrcInfo;

	AkUInt32		m_uNumSubTrack;

	// Classified by index of the sub track
	
	TrackPlaylist			m_mmapTrackPlaylist;
	AkInt32					m_iLookAheadTime;
	AkMusicTrackRanSeqType	m_eRSType;
	AkUInt16				m_SequenceIndex;
};

#endif
