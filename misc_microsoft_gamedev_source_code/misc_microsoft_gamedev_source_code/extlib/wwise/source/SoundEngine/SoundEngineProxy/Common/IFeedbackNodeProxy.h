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
class IFeedbackNodeProxy : virtual public IParameterNodeProxy
{
	DECLARE_BASECLASS( IParameterNodeProxy );
public:
	virtual void AddSource(
		AkUniqueID      in_srcID,
		AkLpCtstr       in_pszFilename,
		AkPluginID      in_pluginID,
		AkAudioFormat & in_audioFormat,
		AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin
		) = 0;

	virtual void AddPluginSource( 
		AkUniqueID	in_srcID,
		AkPluginID in_ulID, 
		void * in_pParam, 
		AkUInt32 in_uSize,
		AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin
		) = 0;

	virtual void SetSrcParam(				// Set the parameter on an physical model source.
		AkUniqueID      in_srcID,
		AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
		AkPluginParamID in_ulParamID,		// Parameter id.
		void *			in_pParam,			// Pointer to a setup param block.
		AkUInt32		in_ulSize			// Size of the parameter block.
		) = 0;

	virtual void SetSourceVolumeOffset(AkUniqueID in_srcID, AkReal32 in_fOffset) = 0;

	virtual void RemoveAllSources() = 0;

	virtual void IsZeroLatency( bool in_bIsZeroLatency ) = 0;

	virtual void LookAheadTime( AkTimeMs in_LookAheadTime ) = 0;

	virtual void Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_loopCount, AkInt16 in_countModMin = 0, AkInt16 in_countModMax = 0) = 0;

	enum MethodIDs
	{
		MethodAddPluginSource = __base::LastMethodID,
		MethodSetSrcParam,
		MethodIsZeroLatency,
		MethodLookAheadTime,
		MethodLoop,
		MethodSetSourceVolumeOffset,


		LastMethodID
	};
};
