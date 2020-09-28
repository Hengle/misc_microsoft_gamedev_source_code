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
// AkFeedbackBus.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AkFeedbackBus.h"
#include "AkSIS.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>

CAkFeedbackBus* CAkFeedbackBus::s_pMasterBus = NULL;

CAkFeedbackBus::CAkFeedbackBus(AkUniqueID in_ulID) 
	:CAkBus(in_ulID)
{
	AkNew2( m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo());
	if (s_pMasterBus == NULL)
		s_pMasterBus = this;
}

CAkFeedbackBus::~CAkFeedbackBus()
{
	if (s_pMasterBus == this)
		s_pMasterBus = NULL;
}

CAkFeedbackBus* CAkFeedbackBus::Create(AkUniqueID in_ulID)
{
	CAkFeedbackBus* pBus = AkNew( g_DefaultPoolId, CAkFeedbackBus( in_ulID ) );
	if( pBus )
	{
		if( pBus->Init() != AK_Success )
		{
			pBus->Release();
			pBus = NULL;
		}
	}
	return pBus;
}

CAkFeedbackBus* CAkFeedbackBus::GetMasterBus()
{
	return s_pMasterBus;
}

CAkFeedbackBus* CAkFeedbackBus::ClearTempMasterBus()
{
	CAkFeedbackBus* pTemp = s_pMasterBus;
	s_pMasterBus = NULL;
	return pTemp;
}

void CAkFeedbackBus::ResetMasterBus(CAkFeedbackBus* in_pBus)
{
	if (s_pMasterBus == NULL && in_pBus != NULL)
		s_pMasterBus = in_pBus;
}

AkNodeCategory CAkFeedbackBus::NodeCategory()
{
	return AkNodeCategory_FeedbackBus;
}

AKRESULT CAkFeedbackBus::AddChild( AkUniqueID in_ulID )
{
	AKASSERT(g_pIndex);
	if(!in_ulID)
	{
		return AK_InvalidID;
	}
	
	CAkParameterNodeBase* pAudioNode = (CAkParameterNodeBase*)g_pIndex->m_idxAudioNode.GetPtrAndAddRef(in_ulID);
	if ( !pAudioNode )
		return AK_IDNotFound;

	AKRESULT eResult = CanAddChild(pAudioNode);
	if(eResult == AK_Success)
	{
		if( !m_mapChildId.Set( in_ulID, pAudioNode ) )
		{
			eResult = AK_Fail;
		}
		else
		{
			pAudioNode->FeedbackParentBus(this);
			this->AddRef();
		}
	}

	pAudioNode->Release();

	return eResult;
}

AKRESULT CAkFeedbackBus::RemoveChild( AkUniqueID in_ulID )
{
	AKASSERT(in_ulID);
	AKRESULT eResult = AK_Success;
	CAkParameterNodeBase** pAudioNode = (CAkParameterNodeBase**)m_mapChildId.Exists(in_ulID);
	if( pAudioNode )
	{
		(*pAudioNode)->FeedbackParentBus(NULL);
		m_mapChildId.Unset(in_ulID);
		this->Release();
	}
	return eResult;
}


AKRESULT CAkFeedbackBus::CanAddChild( CAkAudioNode * in_pAudioNode )
{
	AKRESULT eResult = AK_Success;	
	if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

// Get the compounded feedback parameters.  There is currenly only the volume.
AKRESULT CAkFeedbackBus::GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck /*= true*/ )
{

	AkUInt32 paramSelect = 0xFFFFFFFF;//enable all by default
	AKASSERT(m_pFeedbackInfo);

	io_Params.m_NewVolume += GetDuckedVolume();

	if (m_bUseState && m_pStateTransitionInfo)
	{
		CAkState* pState = GetState();
		if( pState != NULL )
		{
			if(pState->VolumeMeaning() == AkValueMeaning_Independent)
			{
				paramSelect &= ~PT_Volume;
			}
			io_Params.m_NewVolume += m_pStateTransitionInfo->m_Volume;

			if(pState->PitchMeaning() == AkValueMeaning_Independent)
			{
				paramSelect &= ~PT_Pitch;
			}
			io_Params.m_MotionBusPitch += m_pStateTransitionInfo->m_Pitch;

		}
		// we don't have any state
		else
		{
			//MUST do it, we actually don't have a state, but maybe we had one before.
			io_Params.m_NewVolume += m_pStateTransitionInfo->m_Volume;
			io_Params.m_MotionBusPitch += m_pStateTransitionInfo->m_Pitch;
		}
	}

	if(paramSelect & PT_Volume)
	{
		io_Params.m_NewVolume += m_VolumeMain.GetValue();
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Volume ) )
		{
			io_Params.m_NewVolume += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<AK::IAkRTPCSubscriber*>(this), RTPC_Volume, NULL );
		}
	}
	if(paramSelect & PT_Pitch)
	{
		io_Params.m_MotionBusPitch += m_PitchMain;
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Pitch ) )
		{
			io_Params.m_MotionBusPitch += (AkPitchValue) g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<AK::IAkRTPCSubscriber*>(this), RTPC_Pitch, NULL );
		}
	}
	if(m_pGlobalSIS)
	{
		io_Params.m_NewVolume += m_pGlobalSIS->m_EffectiveVolumeOffset;
		io_Params.m_MotionBusPitch += m_pGlobalSIS->m_EffectivePitchOffset;
	}
	

	//Recurse in the parent busses.  There can only be one parent!
	CAkFeedbackBus* pParentBus = FeedbackParentBus();
	if(pParentBus != NULL)
		pParentBus->GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, false);

	return AK_Success;
}

void CAkFeedbackBus::VolumeNotification( AkVolumeValue in_Volume, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	FeedbackVolumeNotification(in_Volume, in_pGameObj, in_pExceptArray);
}

void CAkFeedbackBus::PitchNotification( AkPitchValue in_Pitch, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	FeedbackPitchNotification( in_Pitch, in_pGameObj, in_pExceptArray );
}

void CAkFeedbackBus::ParamNotification( NotifParams& in_rParams )
{
	AKASSERT( in_rParams.pGameObj == NULL );

	// Note: master bus and bus volumes are applied lower in the hierarchy when the is no effect, 
	// otherwise they are applied at the proper level to avoid having pre-effect volumes
	
	//TODO TODO Enable this code when bus effects will be supported.
	/*in_rParams.bIsFromBus = true;
	if( (in_rParams.eType == NotifParamType_Volume) && HasEffect() )
	{
		if ( IsMasterBus() )
			CAkLEngine::SetMasterBusVolume( in_rParams.UnionType.volume );
		else
			CAkLEngine::SetBusVolume( ID(), in_rParams.UnionType.volume );
	}
	else if( (in_rParams.eType == NotifParamType_LFE) && HasEffect() )
	{
		if ( IsMasterBus() )
			CAkLEngine::SetMasterBusLFE( in_rParams.UnionType.LFE );
		else
			CAkLEngine::SetBusLFE( ID(), in_rParams.UnionType.LFE );
	}
	*/

	CAkBus::ParamNotification(in_rParams);
}
