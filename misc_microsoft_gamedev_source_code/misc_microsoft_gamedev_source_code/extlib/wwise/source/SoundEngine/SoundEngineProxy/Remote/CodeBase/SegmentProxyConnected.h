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

#include "MusicNodeProxyConnected.h"
#include "SegmentProxyLocal.h"

class SegmentProxyConnected : public MusicNodeProxyConnected
{
public:
	SegmentProxyConnected( AkUniqueID in_id );
	virtual ~SegmentProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual AudioNodeProxyLocal& GetLocalProxy() { return m_proxyLocal; }

private:

	SegmentProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( MusicNodeProxyConnected );
};
