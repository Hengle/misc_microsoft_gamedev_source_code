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

#include "IParameterNodeProxy.h"

struct AkAudioFormat;

class ISoundProxy : virtual public IParameterNodeProxy
{
	DECLARE_BASECLASS( IParameterNodeProxy );
public:
	virtual void SetSource( AkUInt32 in_PluginID, AkLpCtstr in_szFileName, const AkAudioFormat& in_AudioFormat ) = 0;

	virtual void SetSource( AkPluginID in_ID,				// Plug-in id.
							void * in_vpParam,				// Pointer to a setup param block.
							AkUInt32 in_ulSize ) = 0;

    virtual void SetSrcParam( AkPluginID in_ID,				// Plug-in id.
								AkPluginParamID in_ParamID,	// Parameter id.
								void * in_vpParam,			// Pointer to a setup param block.
								AkUInt32 in_ulSize ) = 0;

    virtual void Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_loopCount, AkInt16 in_countModMin = 0, AkInt16 in_countModMax = 0) = 0;

	virtual void IsZeroLatency( bool in_bIsZeroLatency ) = 0;

	enum MethodIDs
	{
		MethodSetSource = __base::LastMethodID,
		MethodSetSource_Plugin,
		MethodSetSrcParam,
		MethodLoop,
		MethodIsZeroLatency,

		LastMethodID
	};
};
