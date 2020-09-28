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


#pragma once

#include "ParameterNodeProxyLocal.h"
#include "ITrackProxy.h"

class CAkMusicTrack;

class TrackProxyLocal : public ParameterNodeProxyLocal
						, virtual public ITrackProxy
{
public:
	TrackProxyLocal( AkUniqueID in_id );
	virtual ~TrackProxyLocal();

	// ITrackProxy members
	virtual AKRESULT AddSource(
		AkUniqueID      in_srcID,
        AkLpCtstr       in_pszFilename,
        AkPluginID      in_pluginID,
        AkAudioFormat & in_audioFormat
        );

	virtual AKRESULT AddPluginSource( 
		AkUniqueID	in_srcID,
		AkPluginID	in_ulID, 
		void*		in_pParam, 
		AkUInt32	in_uSize 
		);

	virtual void SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		);

	virtual void SetSrcParam(	AkUniqueID	in_srcID,
								AkPluginID	in_ID,
								AkPluginParamID in_ParamID,
								void *		in_vpParam,
								AkUInt32	in_ulSize 
								);

    // Note: Identifying sources by sound ID is not good since the same source can be used at various places.
    virtual void RemoveSource( 
        AkUniqueID      in_srcID
        );

    virtual void RemoveAllSources();

	virtual void Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_sLoopCount, AkInt16 in_sCountModMin, AkInt16 in_sCountModMax );

	virtual void IsStreaming( bool in_bIsStreaming );

	virtual void IsZeroLatency( bool in_bIsZeroLatency );

	virtual void LookAheadTime( AkTimeMs in_LookAheadTime );

	virtual void SetMusicTrackRanSeqType( AkMusicTrackRanSeqType in_eRSType );
};
