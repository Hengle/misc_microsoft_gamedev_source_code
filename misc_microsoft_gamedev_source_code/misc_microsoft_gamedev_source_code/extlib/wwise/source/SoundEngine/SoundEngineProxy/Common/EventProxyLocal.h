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

#include "ObjectProxyLocal.h"
#include "IEventProxy.h"

class CAkEvent;

class EventProxyLocal : public ObjectProxyLocal
						, virtual public IEventProxy
{
public:
	EventProxyLocal( AkUniqueID in_id );
	virtual ~EventProxyLocal();

	// IEventProxy members
	virtual void Add( AkUniqueID in_actionID );
	virtual void Remove( AkUniqueID in_actionID );
	virtual void Clear();
};
