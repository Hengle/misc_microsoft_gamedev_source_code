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
// AkLEngineDefs.h
//
// Defines, enums, and structs for inter-engine interface.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_LENGINE_DEFS_H_
#define _AK_LENGINE_DEFS_H_

#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "AkSharedEnum.h"

namespace AK
{
	class IAkPluginParam;
}

//-----------------------------------------------------------------------------
// SOURCES
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: enum AkSrcType
// Desc: Enumeration of types of sources.
//-----------------------------------------------------------------------------
enum AkSrcType
{
    SrcTypeNone     = 0,
	SrcTypeFile     = 1,
    SrcTypeModelled = 2,
    SrcTypeMemory   = 3	
#define SRC_TYPE_NUM_BITS   (5)
};

//-----------------------------------------------------------------------------
// Name: struct AkFXDesc
// Desc: Effect passing info
//-----------------------------------------------------------------------------
struct AkFXDesc
{
	AkPluginID				EffectTypeID;	// Effect unique type ID. 
	AK::IAkPluginParam*		pParam;			// Parameters.	
	AkUniqueID				FxReferenceID;
	bool					bIsBypassed;
};

struct AkSourceDesc
{
	AkPluginID				EffectTypeID;	// Effect unique type ID. 
	AK::IAkPluginParam*		pParam;			// Parameters.
};


#endif // _AK_LENGINE_DEFS_H_
