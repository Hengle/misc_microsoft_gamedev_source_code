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
// AkSound.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkSound.h"
#include "AkAudioLibIndex.h"
#include "AkContinuationList.h"
#include "PrivateStructures.h"
#include "AkURenderer.h"
#include "AudiolibDefs.h"
#include "AkBankFloatConversion.h"
#include "AkCntrHistory.h"
#include "AkModifiers.h"
#include "AkFXMemAlloc.h"


//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
extern AkMemPoolId		g_DefaultPoolId;

CAkSound::CAkSound( AkUniqueID in_ulID )
:CAkSoundBase( in_ulID )
{
}

CAkSound::~CAkSound()
{
}

CAkSound* CAkSound::Create( AkUniqueID in_ulID )
{
	CAkSound* pAkSound = AkNew( g_DefaultPoolId, CAkSound( in_ulID ) );

	if( pAkSound && pAkSound->Init() != AK_Success )
	{
		pAkSound->Release();
		pAkSound = NULL;
	}
	
	return pAkSound;
}

AkNodeCategory CAkSound::NodeCategory()
{
	return AkNodeCategory_Sound;
}

AKRESULT CAkSound::Play( AkPBIParams& in_rPBIParams )
{
    return CAkURenderer::Play( this, &m_Source, in_rPBIParams );
}

AKRESULT CAkSound::ExecuteAction( ActionParams& in_rAction )
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

AKRESULT CAkSound::ExecuteActionExcept( ActionParamsExcept& in_rAction )
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
AkObjectCategory CAkSound::Category()
{
	return ObjCategory_Sound;
}

//-----------------------------------------------------------------------------
// Name: SetInitialValues
// Desc: Sets the initial Bank source.
//
// Return: Ak_Success:          Initial values were set.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to set the initial values.
//-----------------------------------------------------------------------------
AKRESULT CAkSound::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize, CAkUsageSlot* in_pUsageSlot, bool in_bIsPartialLoadOnly )
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just read/skip it
	AkUInt32 ulID = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	//Read Source info
	if(eResult == AK_Success)
	{
		AkBankSourceData oSourceInfo;
		eResult = CAkBankMgr::LoadSource(in_pData, in_ulDataSize, oSourceInfo);
		if (eResult != AK_Success)
			return eResult;

		if (oSourceInfo.m_pParam == NULL)
		{
			//This is a file source
			SetSource( oSourceInfo.m_PluginID, oSourceInfo.m_MediaInfo, oSourceInfo.m_audioFormat );
		}
		else
		{
			//This is a plugin
			SetSource( oSourceInfo.m_PluginID, oSourceInfo.m_pParam, oSourceInfo.m_uSize );
		}
	}

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

AKRESULT CAkSound::PrepareData()
{
	return m_Source.PrepareData();
}

void CAkSound::UnPrepareData()
{
	m_Source.UnPrepareData();
}
