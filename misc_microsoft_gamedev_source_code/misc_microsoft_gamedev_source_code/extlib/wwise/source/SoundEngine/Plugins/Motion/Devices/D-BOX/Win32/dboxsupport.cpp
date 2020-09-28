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
#include "DBoxMixBus.h"
#include <AK/SoundEngine/Common/AkSoundEngine.h>

AK::IAkPlugin* AkCreateDBox( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW(in_pAllocator, DBoxMixBus());
}