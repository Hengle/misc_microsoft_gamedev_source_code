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
// AkBusCtx.h
//
// Definition of the Audio engine bus runtime context.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_BUS_CTX_H_
#define _AK_BUS_CTX_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkLEngineDefs.h"
#include "AkFeedbackBus.h"

class CAkBus;

namespace AK
{
    //-----------------------------------------------------------------------------
    // CAkBusCtx interface.
    //-----------------------------------------------------------------------------
    class CAkBusCtx
    {
    public:
		CAkBusCtx():m_pBus(NULL){}

	    // Sound parameters.
	    AkUniqueID		ID();
		AkVolumeValue	GetVolume();
		AkVolumeValue	GetLfe();
		bool			IsMasterBus();

	    // Effects access.
		void			GetFX( AkUInt32 in_uFXIndex, AkFXDesc& out_rFXInfo );
		bool			GetBypassAllFX();
		bool			HasEffect();
		bool			IsEnvironmental();

		// Effects access.
		AKRESULT		SubscribeRTPC( IAkRTPCSubscriber* in_prtpcSubs, AkPluginID in_fxID );
		AKRESULT		SubscribeRTPCforEnv( IAkRTPCSubscriber* in_prtpcSubs, AkEnvID in_envID );
		AKRESULT		UnsubscribeRTPC( IAkRTPCSubscriber* in_prtpcSubs );	

		void SetBus( CAkBus* in_pBus ){ m_pBus = in_pBus; }
		bool HasBus(){ return ( m_pBus != NULL ); }

	private:
		CAkBus* m_pBus;
    };
}

#endif // _AK_BUS_CTX_H_
