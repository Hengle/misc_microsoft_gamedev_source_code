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
#include "ParameterNodeProxyLocal.h"
#include "IFeedbackNodeProxy.h"

class FeedbackNodeProxyLocal
	: public ParameterNodeProxyLocal
	, virtual public IFeedbackNodeProxy
{
public:
	FeedbackNodeProxyLocal( AkUniqueID in_id );
	~FeedbackNodeProxyLocal();

	void AddSource(
		AkUniqueID      in_srcID,
		AkLpCtstr       in_pszFilename,
		AkPluginID      in_pluginID,
		AkAudioFormat & in_audioFormat,
		AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin);

	void AddPluginSource( 
		AkUniqueID	in_srcID,
		AkPluginID in_ulID, 
		void * in_pParam, 
		AkUInt32 in_uSize,
		AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin
		);

	void SetSrcParam(					// Set the parameter on an physical model source.
		AkUniqueID      in_srcID,
		AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
		AkPluginParamID in_ulParamID,		// Parameter id.
		void *			in_pParam,			// Pointer to a setup param block.
		AkUInt32		in_ulSize			// Size of the parameter block.
		);

	void SetSourceVolumeOffset(AkUniqueID in_srcID, AkReal32 in_fOffset);

	void RemoveAllSources();

	void IsZeroLatency( bool in_bIsZeroLatency );

	void LookAheadTime( AkTimeMs in_LookAheadTime );

	void Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_loopCount, AkInt16 in_countModMin = 0, AkInt16 in_countModMax = 0) ;

};
