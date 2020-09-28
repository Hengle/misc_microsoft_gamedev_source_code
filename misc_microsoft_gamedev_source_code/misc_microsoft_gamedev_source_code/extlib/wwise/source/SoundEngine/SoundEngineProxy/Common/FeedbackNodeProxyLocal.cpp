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
#include "AkCritical.h"
#include "FeedbackNodeProxyLocal.h"
#include "AkAudioLib.h"
#include "AkFeedbackNode.h"

FeedbackNodeProxyLocal::FeedbackNodeProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	SetIndexable( pIndexable != NULL ? pIndexable : CAkFeedbackNode::Create( in_id ) );
}

FeedbackNodeProxyLocal::~FeedbackNodeProxyLocal()
{
}

void FeedbackNodeProxyLocal::AddSource(
					   AkUniqueID      in_srcID,
					   AkLpCtstr       in_pszFilename,
					   AkPluginID      in_pluginID,
					   AkAudioFormat & in_audioFormat,
					   AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin)
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>(GetIndexable());
	if(pIndexable)
	{
		pIndexable->AddSource(in_srcID, in_pszFilename, in_pluginID, in_audioFormat, in_idDeviceCompany, in_idDevicePlugin);
	}
}

void FeedbackNodeProxyLocal::AddPluginSource( 
		AkUniqueID	in_srcID,
		AkPluginID in_ulID, 
		void * in_pParam, 
		AkUInt32 in_uSize,
		AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin)
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>(GetIndexable());
	if(pIndexable)
	{
		pIndexable->AddPluginSource(in_srcID, in_ulID, in_pParam, in_uSize, in_idDeviceCompany, in_idDevicePlugin);
	}
}

void FeedbackNodeProxyLocal::SetSrcParam(					// Set the parameter on an physical model source.
		AkUniqueID      in_srcID,
		AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
		AkPluginParamID in_ulParamID,		// Parameter id.
		void *			in_pParam,			// Pointer to a setup param block.
		AkUInt32		in_ulSize			// Size of the parameter block.
		)
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>(GetIndexable());
	if(pIndexable)
	{
		pIndexable->SetSrcParam(in_srcID, in_ID, in_ulParamID, in_pParam, in_ulSize);
	}
}

void FeedbackNodeProxyLocal::RemoveAllSources()
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>(GetIndexable());
	if(pIndexable)
	{
		pIndexable->RemoveAllSources();
	}
}

void FeedbackNodeProxyLocal::IsZeroLatency( bool in_bIsZeroLatency )
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>(GetIndexable());
	if(pIndexable)
	{
		pIndexable->IsZeroLatency(in_bIsZeroLatency);
	}
}

void FeedbackNodeProxyLocal::LookAheadTime( AkTimeMs in_LookAheadTime )
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>(GetIndexable());
	if(pIndexable)
	{
		pIndexable->LookAheadTime(in_LookAheadTime);
	}
}

void FeedbackNodeProxyLocal::Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_sLoopCount, AkInt16 in_sCountModMin, AkInt16 in_sCountModMax )
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->Loop( in_bIsLoopEnabled, in_bIsLoopInfinite, in_sLoopCount, in_sCountModMin, in_sCountModMax );
	}
}

void FeedbackNodeProxyLocal::SetSourceVolumeOffset(AkUniqueID in_srcID, AkReal32 in_fOffset)
{
	CAkFeedbackNode* pIndexable = static_cast<CAkFeedbackNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetSourceVolumeOffset(in_srcID, in_fOffset);
	}
}
