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

#include "AttenuationProxyLocal.h"

#include "AkAudiolib.h"
#include "AkAttenuationMgr.h"
#include "AkCritical.h"


AttenuationProxyLocal::AttenuationProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Attenuation );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkAttenuation::Create( in_id ) );
}

AttenuationProxyLocal::~AttenuationProxyLocal()
{
}

void AttenuationProxyLocal::SetAttenuationParams( AkWwiseAttenuation& in_Params )
{
	CAkAttenuation* pIndexable = static_cast<CAkAttenuation*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		CAkAttenuation::AcquireAttenuationLock();
		pIndexable->SetAttenuationParams( in_Params );
		CAkAttenuation::ReleaseAttenuationLock();
	}
}

