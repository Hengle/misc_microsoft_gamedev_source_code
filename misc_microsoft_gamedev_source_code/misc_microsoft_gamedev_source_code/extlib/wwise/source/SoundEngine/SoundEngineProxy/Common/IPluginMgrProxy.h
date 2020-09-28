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

#include "AkPrivateTypes.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>

class IPluginMgrProxy
{
public:
	virtual AKRESULT RegisterPlugin( 
		AkPluginType in_eType,
		AkUInt32 in_ulCompanyID, 
		AkUInt32 in_ulPluginID,  
		AkCreatePluginCallback in_pCreateFunc,
        AkCreateParamCallback in_pCreateParamFunc 
		) const = 0;

	enum MethodIDs
	{
		MethodRegisterEffect = 1,
        
		LastMethodID
	};
};
