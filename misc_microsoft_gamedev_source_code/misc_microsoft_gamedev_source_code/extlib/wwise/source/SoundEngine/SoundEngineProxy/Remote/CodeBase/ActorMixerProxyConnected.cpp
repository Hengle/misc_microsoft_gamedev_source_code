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


#include "stdafx.h"

#include "ActorMixerProxyConnected.h"
#include "CommandData.h"


ActorMixerProxyConnected::ActorMixerProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

ActorMixerProxyConnected::~ActorMixerProxyConnected()
{
}

void ActorMixerProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
}
