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

#include "IParameterNodeProxy.h"
#include "AkMusicStructs.h"

struct AkTrackSrcInfo;

class ITrackProxy : virtual public IParameterNodeProxy
{
	DECLARE_BASECLASS( IParameterNodeProxy );
public:

	virtual AKRESULT AddSource(
		AkUniqueID      in_srcID,
        AkLpCtstr       in_pszFilename,
        AkPluginID      in_pluginID,
        AkAudioFormat & in_audioFormat
        ) = 0;

	virtual AKRESULT AddPluginSource( 
		AkUniqueID	in_srcID,
		AkPluginID	in_ulID, 
		void*		in_pParam, 
		AkUInt32	in_uSize 
		) = 0;

	virtual void SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		) = 0;

	virtual void SetSrcParam(	
		AkUniqueID in_srcID,
		AkPluginID in_ID,
		AkPluginParamID in_ParamID,
		void * in_vpParam,
		AkUInt32 in_ulSize 
		) = 0;

    // Note: Identifying sources by sound ID is not good since the same source can be used at various places.
    virtual void RemoveSource( 
        AkUniqueID      in_srcID
        ) = 0;

    virtual void RemoveAllSources() = 0;

    virtual void Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_loopCount, AkInt16 in_countModMin = 0, AkInt16 in_countModMax = 0) = 0;

	virtual void IsStreaming( bool in_bIsStreaming ) = 0;

	virtual void IsZeroLatency( bool in_bIsZeroLatency ) = 0;

	virtual void LookAheadTime( AkTimeMs in_LookAheadTime ) = 0;

	virtual void SetMusicTrackRanSeqType( AkMusicTrackRanSeqType in_eRSType ) = 0;

	enum MethodIDs
	{
		MethodAddSource = __base::LastMethodID,
		MethodAddPluginSource,
		MethodSetPlayList,
		MethodRemoveSource,
		MethodRemoveAllSources,
		MethodSetSrcParam,
		MethodLoop,
		MethodIsStreaming,
		MethodIsZeroLatency,
		MethodLookAheadTime,
		MethodSetMusicTrackRanSeqType,

		LastMethodID
	};
};
