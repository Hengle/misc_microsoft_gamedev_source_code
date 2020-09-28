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
// AkFeedbackNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkFeedbackNode.h"
#include "AkFeedbackBus.h"
#include "IAkMotionMixBus.h"
#include "AkAudioLibIndex.h"
#include "AkContinuationList.h"
#include "PrivateStructures.h"
#include "AkURenderer.h"
#include "AudiolibDefs.h"
#include "AkBankFloatConversion.h"
#include "AkCntrHistory.h"
#include "AkModifiers.h"
#include "AkFXMemAlloc.h"
#include "AkFeedbackMgr.h"

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
extern AkMemPoolId		g_DefaultPoolId;

CAkFeedbackNode::CAkFeedbackNode( AkUniqueID in_ulID )
:CAkSoundBase( in_ulID )
, m_iLookAheadTime( 0 )
{
}

CAkFeedbackNode::~CAkFeedbackNode()
{
	RemoveAllSources( );
	m_arSrcInfo.Term( );
}

CAkFeedbackNode* CAkFeedbackNode::Create( AkUniqueID in_ulID )
{
	CAkFeedbackNode* pAkFeedbackNode = AkNew( g_DefaultPoolId, CAkFeedbackNode( in_ulID ) );

	if( pAkFeedbackNode && pAkFeedbackNode->Init() != AK_Success )
	{
		pAkFeedbackNode->Release();
		pAkFeedbackNode = NULL;
	}
	
	return pAkFeedbackNode;
}

AkNodeCategory CAkFeedbackNode::NodeCategory()
{
	return AkNodeCategory_FeedbackNode;
}

AKRESULT CAkFeedbackNode::Play( AkPBIParams& in_rPBIParams )
{
	in_rPBIParams.bTargetFeedback = true;

	// The in_rPBIParams.uFrameOffset was calculated for an audio delay, convert it for feedback delay. 
	in_rPBIParams.uFrameOffset = in_rPBIParams.uFrameOffset * AK_FEEDBACK_MAX_FRAMES_PER_BUFFER / AK_NUM_VOICE_REFILL_FRAMES;

	//Start sources for all devices, one PBI for each.
	CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
	if(pMgr == NULL)
		return AK_Success;

	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		if( !pMgr->IsDeviceActive(iter.pItem->item->m_idDeviceCompany, iter.pItem->item->m_idDevicePlugin) )
			continue;

		CAkURenderer::Play( this, iter.pItem->item, in_rPBIParams );
	}

    return AK_Success;
}

AKRESULT CAkFeedbackNode::ExecuteAction( ActionParams& in_rAction )
{
	AKRESULT eResult = AK_Success;
	if( in_rAction.bIsMasterCall )
	{
		//Only global pauses should Pause a state transition
		bool l_bPause = false;
		if( in_rAction.eType == ActionParamType_Pause )
		{
			l_bPause = true;
		}
		PauseTransitions( l_bPause );
	}
	if( IsPlaying() )
	{
		switch( in_rAction.eType )
		{
		case ActionParamType_Stop:
			eResult = CAkURenderer::Stop( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
			break;
		case ActionParamType_Pause:
			eResult = CAkURenderer::Pause( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
			break;
		case ActionParamType_Resume:
			eResult = CAkURenderer::Resume( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.bIsMasterResume, in_rAction.playingID );
			break;
		case ActionParamType_Break:
			PlayToEnd( in_rAction.pGameObj, in_rAction.targetNodeID, in_rAction.playingID );
			break;
		}
	}
	return eResult;
}

AKRESULT CAkFeedbackNode::ExecuteActionExcept( ActionParamsExcept& in_rAction )
{
	AKRESULT eResult = AK_Success;

	if( in_rAction.pGameObj == NULL )
	{
		//Only global pauses should Pause a state transition
		bool l_bPause = false;
		if( in_rAction.eType == ActionParamType_Pause )
		{
			l_bPause = true;
		}
		PauseTransitions( l_bPause );
	}

	if( IsPlaying() )
	{
		switch( in_rAction.eType )
		{
		case ActionParamType_Stop:
			eResult = CAkURenderer::Stop( this, in_rAction.pGameObj, in_rAction.transParams );
			break;
		case ActionParamType_Pause:
			eResult = CAkURenderer::Pause( this, in_rAction.pGameObj, in_rAction.transParams );
			break;
		case ActionParamType_Resume:
			eResult = CAkURenderer::Resume( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.bIsMasterResume );
			break;
		}
	}
	return eResult;
}

//-----------------------------------------------------------------------------
// Name: Category
// Desc: Returns the category of the sound.
//
// Parameters: None.
//
// Return: AkObjectCategory.
//-----------------------------------------------------------------------------
AkObjectCategory CAkFeedbackNode::Category()
{
	return ObjCategory_FeedbackNode;
}

//-----------------------------------------------------------------------------
// Name: SetInitialValues
// Desc: Sets the initial Bank source.
//
// Return: Ak_Success:          Initial values were set.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to set the initial values.
//-----------------------------------------------------------------------------
AKRESULT CAkFeedbackNode::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize, CAkUsageSlot* in_pUsageSlot, bool in_bIsPartialLoadOnly )
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just read/skip it
	AkUInt32 ulID = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	AkUInt32 numSources = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	for( AkUInt32 i = 0; i < numSources && eResult == AK_Success; ++i )
	{
		//Read device ID
		AkUInt16 CompanyID = READBANKDATA(AkUInt16, in_pData, in_ulDataSize);
		AkUInt16 DeviceID = READBANKDATA(AkUInt16, in_pData, in_ulDataSize);
		AkReal32 fVolumeOffset = READBANKDATA(AkReal32, in_pData, in_ulDataSize);

		AkBankSourceData oSourceInfo;
		eResult = CAkBankMgr::LoadSource(in_pData, in_ulDataSize, oSourceInfo);
		if (eResult != AK_Success)
			return eResult;

		if (oSourceInfo.m_pParam == NULL)
		{
			//This is a file source
			eResult = AddSource( oSourceInfo.m_MediaInfo.sourceID, oSourceInfo.m_PluginID, oSourceInfo.m_MediaInfo, oSourceInfo.m_audioFormat, CompanyID, DeviceID );
		}
		else
		{
			//This is a plugin
			eResult = AddPluginSource( oSourceInfo.m_MediaInfo.sourceID, oSourceInfo.m_PluginID, oSourceInfo.m_pParam, oSourceInfo.m_uSize, CompanyID, DeviceID );
		}

		SetSourceVolumeOffset(oSourceInfo.m_MediaInfo.sourceID, fVolumeOffset);
	}

	if( eResult != AK_Success )
		return eResult;

	//ReadParameterNode
	eResult = SetNodeBaseParams(in_pData, in_ulDataSize, in_bIsPartialLoadOnly);

	if( in_bIsPartialLoadOnly )
	{
		//Partial load has been requested, probable simply replacing the actual source created by the Wwise on load bank.
		return eResult;
	}

	//Read Looping info
	if(eResult == AK_Success)
	{
		m_Loop = READBANKDATA(AkUInt16, in_pData, in_ulDataSize);
		m_LoopMod.m_min = READBANKDATA(AkUInt16, in_pData, in_ulDataSize);
		m_LoopMod.m_max = READBANKDATA(AkUInt16, in_pData, in_ulDataSize);
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkFeedbackNode::AddSource( 
								  AkUniqueID      in_srcID,
								  AkLpCtstr       in_pszFilename,
								  AkPluginID      in_pluginID,
								  AkAudioFormat & in_audioFormat,
								  AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin
								  )
{
	SrcInfo ** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		//Already there.
		return AK_Success;
	}
	else
	{
		ppSource = m_arSrcInfo.Set( in_srcID );
	}

	if ( ppSource )	//Memory alloc check
	{   
		*ppSource = AkNew( g_DefaultPoolId, SrcInfo() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_pluginID, in_pszFilename, in_audioFormat );
			(*ppSource)->StreamingLookAhead( m_iLookAheadTime );
			(*ppSource)->m_idDeviceCompany = in_idDeviceCompany;
			(*ppSource)->m_idDevicePlugin = in_idDevicePlugin;
			(*ppSource)->m_fVolumeOffset = 0.0;
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
	}
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

AKRESULT CAkFeedbackNode::AddSource( 
								  AkUniqueID in_srcID, 
								  AkUInt32 in_pluginID, 
								  AkMediaInformation in_MediaInfo, 
								  AkAudioFormat in_audioFormat,
								  AkUInt16 in_idDeviceCompany, AkUInt16 in_idDevicePlugin
								  )
{
	SrcInfo ** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		//Already there.
		return AK_Success;
	}
	else
	{
		ppSource = m_arSrcInfo.Set( in_srcID );
	}

	if ( ppSource ) //Memory alloc check
	{   
		*ppSource = AkNew( g_DefaultPoolId, SrcInfo() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_pluginID, in_MediaInfo, in_audioFormat );
			(*ppSource)->StreamingLookAhead( m_iLookAheadTime );
			(*ppSource)->m_idDeviceCompany = in_idDeviceCompany;
			(*ppSource)->m_idDevicePlugin = in_idDevicePlugin;
			(*ppSource)->m_fVolumeOffset = 0.0;
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
	}
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

AKRESULT CAkFeedbackNode::AddPluginSource( 
										AkUniqueID	in_srcID,
										AkPluginID	in_ulID, 
										void*		in_pParam, 
										AkUInt32	in_uSize,
										AkUInt16	in_idDeviceCompany, AkUInt16 in_idDevicePlugin
										)
{
	SrcInfo ** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		//Already there.
		return AK_Success;
	}
	else
	{
		ppSource = m_arSrcInfo.Set( in_srcID );
	}

	if ( ppSource ) //Memory alloc check
	{   
		*ppSource = AkNew( g_DefaultPoolId, SrcInfo() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_ulID, in_pParam, in_uSize );
			(*ppSource)->m_idDeviceCompany = in_idDeviceCompany;
			(*ppSource)->m_idDevicePlugin = in_idDevicePlugin;
			(*ppSource)->m_fVolumeOffset = 0.0;
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
	}
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

AKRESULT CAkFeedbackNode::SetSrcParam(		// Set the parameter on an physical model source.
									AkUniqueID      in_srcID,			// Source ID
									AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
									AkPluginParamID in_ulParamID,		// Parameter id.
									void *			in_pParam,			// Pointer to a setup param block.
									AkUInt32		in_ulSize			// Size of the parameter block.
									)
{
	SrcInfo ** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
		return (*ppSource)->SetSrcParam( in_ID, in_ulParamID, in_pParam, in_ulSize );
	return AK_Fail;
}

void CAkFeedbackNode::SetSourceVolumeOffset(AkUniqueID in_srcID, AkReal32 in_fOffset)
{
	SrcInfo ** ppSource = m_arSrcInfo.Exists( in_srcID );
	if(ppSource == NULL)
		return;

	NotifParams l_Params;
	l_Params.eType = NotifParamType_Volume;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = NULL;
	l_Params.UnionType.volume = in_fOffset - (*ppSource)->m_fVolumeOffset;
	l_Params.pExceptObjects = NULL;

	CAkPBI* l_pPBI    = NULL;
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
	{
		l_pPBI = *iter;
		AKASSERT( l_pPBI != NULL );

		if(l_pPBI->GetSource() == *ppSource)
		{
			l_pPBI->ParamNotification( l_Params );
		}
	}

	(*ppSource)->m_fVolumeOffset = in_fOffset;
}

AkReal32 CAkFeedbackNode::GetSourceVolumeOffset(CAkSource *in_pSource)
{
	SrcInfoArray::Iterator iter;
	for( iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End() && iter.pItem->item != in_pSource; ++iter )
		/* empty*/;

	AKASSERT(iter != m_arSrcInfo.End());

	return iter.pItem->item->m_fVolumeOffset;
}

void CAkFeedbackNode::RemoveSource( AkUniqueID in_srcID )
{
	SrcInfo ** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		AkDelete( g_DefaultPoolId, *ppSource );
		m_arSrcInfo.Unset( in_srcID );
	}
}

bool CAkFeedbackNode::HasBankSource()
{ 
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		if( iter.pItem->item->HasBankSource() )
			return true;
	}
	return false;
}

void CAkFeedbackNode::RemoveAllSources()
{
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		AkDelete( g_DefaultPoolId, iter.pItem->item );
	}
	m_arSrcInfo.RemoveAll();
}

void CAkFeedbackNode::IsZeroLatency( bool in_bIsZeroLatency )
{
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->IsZeroLatency( in_bIsZeroLatency );
	}
}

void CAkFeedbackNode::LookAheadTime( AkTimeMs in_LookAheadTime )
{
	m_iLookAheadTime = in_LookAheadTime;
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->StreamingLookAhead( m_iLookAheadTime );
	}
}

AKRESULT CAkFeedbackNode::GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck /*= true*/ )
{
	CAkFeedbackBus* pBus = GetFeedbackParentBusOrDefault();
	AKASSERTANDRETURN(pBus != NULL, AK_Fail);

	SrcInfoArray::Iterator iter;
	for( iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End() && iter.pItem->item != in_pSource; ++iter )
		/* empty*/;

	AKASSERT(iter != m_arSrcInfo.End());

	//The param structure isn't initialized yet.  It is the first time
	//we compute the volume on this object.
	if (io_Params.m_usPluginID == 0)
	{
		io_Params.m_usPluginID = iter.pItem->item->m_idDevicePlugin;
		io_Params.m_usCompanyID = iter.pItem->item->m_idDeviceCompany;
		io_Params.m_pOutput = pBus;	
	}
	io_Params.m_NewVolume = GetEffectiveFeedbackVolume(in_GameObjPtr);
	io_Params.m_MotionBusPitch = 0;

	//Walk up the feedback busses
	return pBus->GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, false);
}

AKRESULT CAkFeedbackNode::PrepareData()
{
	AKRESULT akr = AK_Success;
	SrcInfoArray::Iterator iter;
	for( iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		AKRESULT akr2 = iter.pItem->item->PrepareData();
		if (akr2 != AK_Success)
			akr = akr2;
	}

	return akr;
}

void CAkFeedbackNode::UnPrepareData()
{
	SrcInfoArray::Iterator iter;
	for( iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->UnPrepareData();
	}
}

AKRESULT CAkFeedbackNode::GetAudioParameters( AkSoundParams &io_Parameters, AkUInt32 in_ulParamSelect, AkMutedMap& io_rMutedMap, CAkRegisteredObj * in_GameObjPtr, bool in_bIncludeRange, AkPBIModValues& io_Ranges, bool in_bDoBusCheck /*= true*/ )
{
	// 	 WG-10841.  Set the "Do Bus Check" to false to avoid reducing the volume trough the audio busses.  That should 
	//have no effect on the MotionFX nodes.
	return CAkSoundBase::GetAudioParameters(io_Parameters, in_ulParamSelect, io_rMutedMap, in_GameObjPtr, in_bIncludeRange, io_Ranges, false);
}