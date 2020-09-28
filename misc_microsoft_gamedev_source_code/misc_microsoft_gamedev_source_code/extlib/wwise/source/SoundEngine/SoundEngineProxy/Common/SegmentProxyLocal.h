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

#include "MusicNodeProxyLocal.h"
#include "ISegmentProxy.h"

class SegmentProxyLocal : public MusicNodeProxyLocal
								, virtual public ISegmentProxy
{
public:
	SegmentProxyLocal( AkUniqueID in_id );
	virtual ~SegmentProxyLocal();

	virtual void Duration(
        AkReal64 in_fDuration               // Duration in milliseconds.
        );

	virtual void StartPos(
        AkReal64 in_fStartPos               // PlaybackStartPosition.
        );

	virtual void SetMarkers(
		AkMusicMarkerWwise*     in_pArrayMarkers, 
		AkUInt32                in_ulNumMarkers
		);

    virtual void RemoveMarkers();

};
