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

#include "IMusicTransAwareProxy.h"
#include "AkSwitchAware.h"

class IMusicSwitchProxy : virtual public IMusicTransAwareProxy
{
	DECLARE_BASECLASS( IMusicTransAwareProxy );
public:

	virtual void SetSwitchAssocs(
		AkUInt32 in_uNumAssocs,
		AkMusicSwitchAssoc* in_pAssocs
		) = 0;

    virtual void  SetSwitchGroup( 
        AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
        ) = 0;

    virtual void  SetDefaultSwitch( 
		AkUInt32 in_Switch 
		) = 0;

	virtual void ContinuePlayback( bool in_bContinuePlayback ) = 0;


	enum MethodIDs
	{
		MethodSetSwitchAssocs = __base::LastMethodID,
		MethodSetSwitchGroup,
		MethodSetDefaultSwitch,
		MethodContinuePlayback,

		LastMethodID
	};
};
