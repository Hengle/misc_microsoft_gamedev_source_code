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

#include "MusicTransAwareProxyLocal.h"
#include "IMusicSwitchProxy.h"

class MusicSwitchProxyLocal : public MusicTransAwareProxyLocal
						, virtual public IMusicSwitchProxy
{
public:
	MusicSwitchProxyLocal( AkUniqueID in_id );
	virtual ~MusicSwitchProxyLocal();

	virtual void SetSwitchAssocs(
		AkUInt32 in_uNumAssocs,
		AkMusicSwitchAssoc* in_pAssocs
		);

    virtual void  SetSwitchGroup( 
        AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
        );

    virtual void  SetDefaultSwitch( 
		AkUInt32 in_Switch 
		);

	virtual void ContinuePlayback( bool in_bContinuePlayback );


};
