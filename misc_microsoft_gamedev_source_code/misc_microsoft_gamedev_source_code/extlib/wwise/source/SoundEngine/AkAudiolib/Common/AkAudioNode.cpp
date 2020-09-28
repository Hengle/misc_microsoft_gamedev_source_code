/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h"
#include "AkAudioNode.h"
#include "AkAudioLibIndex.h"
#include "AkCntrHistory.h"

CAkAudioNode::CAkAudioNode(AkUniqueID in_ulID)
:CAkPBIAware( in_ulID )
,m_pParentNode(NULL)
,m_pBusOutputNode(NULL)
,m_PlayCount(0)
,m_uActivityCount(0)
{
}

CAkAudioNode::~CAkAudioNode(void)
{
}

AKRESULT CAkAudioNode::Init()
{
	AddToIndex();
	return AK_Success;
}

void CAkAudioNode::AddToIndex()
{
	AKASSERT( g_pIndex );
	g_pIndex->m_idxAudioNode.SetIDToPtr(this);
}

void CAkAudioNode::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	g_pIndex->m_idxAudioNode.RemoveID(ID());
}

AkUInt32 CAkAudioNode::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxAudioNode.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkAudioNode::Release() 
{ 
    AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxAudioNode.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
        RemoveFromIndex(); 

		if(m_pParentNode != NULL)
		{
			m_pParentNode->RemoveChild(ID());
		}

		if(m_pBusOutputNode != NULL)
		{
			m_pBusOutputNode->RemoveChild(ID());
		}

        AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

void CAkAudioNode::Parent(CAkAudioNode* in_pParent)
{
	m_pParentNode = in_pParent;
}

void CAkAudioNode::ParentBus(CAkAudioNode* in_pParent)
{
	m_pBusOutputNode = in_pParent;
}

void CAkAudioNode::Unregister(CAkRegisteredObj * in_pGameObj)
{
}

AKRESULT CAkAudioNode::AddChild(
        AkUniqueID in_ulID          // Input node ID to add
		)
{
	AKASSERT(!"Addchild/removechild not defined for this node type");
	return AK_NotImplemented;
}

AKRESULT CAkAudioNode::RemoveChild(
        AkUniqueID in_ulID          // Input node ID to remove
		)
{
	AKASSERT(!"Addchild/removechild not defined for this node type");
	return AK_NotImplemented;
}

AKRESULT CAkAudioNode::RemoveAllChildren()
{
	AKASSERT(!"Addchild/removechild not defined for this node type");
	return AK_NotImplemented;
}

AKRESULT CAkAudioNode::GetChildren( AkUInt32& io_ruNumItems, AkObjectInfo* out_aObjectInfos, AkUInt32& index_out, AkUInt32 iDepth )
{
	return AK_Success; //no children by default. Function will be overridden in AkParentNode.h
}

CAkBus* CAkAudioNode::GetMixingBus()
{
	if(m_pBusOutputNode)
	{
		return m_pBusOutputNode->GetMixingBus();
	}
	else
	{
		if(m_pParentNode)
		{
			return m_pParentNode->GetMixingBus();
		}
		else
		{
			return NULL;//No mixing BUS found, should go directly to the master output.
		}
	}
}

CAkBus* CAkAudioNode::GetLimitingBus()
{
	if( m_pBusOutputNode )
	{
		return m_pBusOutputNode->GetLimitingBus();
	}
	else
	{
		if( m_pParentNode )
		{
			return m_pParentNode->GetLimitingBus();
		}
		else
		{
			return NULL;//No Limiting BUS found, should go directly to the master output.
		}
	}
}

bool CAkAudioNode::Has3DParams()
{
	return false;
}

bool CAkAudioNode::IsException( AkUniqueID in_ID, ExceptionList& in_rExceptionList )
{
	AKASSERT( in_ID != AK_INVALID_UNIQUE_ID );

	return ( in_rExceptionList.Exists( in_ID ) != NULL );
}

AKRESULT CAkAudioNode::Stop( CAkRegisteredObj * in_pGameObj, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
{
	ActionParams l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = false;
	l_Params.transParams.eFadeCurve = AkCurveInterpolation_Linear;
	l_Params.eType = ActionParamType_Stop;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_PlayingID;
	l_Params.transParams.TransitionTime = 0;
	l_Params.bIsMasterCall = false;
	return ExecuteAction( l_Params );
}

AKRESULT CAkAudioNode::Pause( CAkRegisteredObj * in_pGameObj, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
{
	ActionParams l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = false;
	l_Params.transParams.eFadeCurve = AkCurveInterpolation_Linear;
	l_Params.eType = ActionParamType_Pause;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_PlayingID;
	l_Params.transParams.TransitionTime = 0;
	l_Params.bIsMasterCall = false;
	return ExecuteAction( l_Params );
}

AKRESULT CAkAudioNode::Resume( CAkRegisteredObj * in_pGameObj, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
{
	ActionParams l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = false;
	l_Params.transParams.eFadeCurve = AkCurveInterpolation_Linear;
	l_Params.eType = ActionParamType_Resume;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_PlayingID;
	l_Params.transParams.TransitionTime = 0;
	l_Params.bIsMasterCall = false;
	return ExecuteAction( l_Params );
}

void CAkAudioNode::PitchNotification( AkPitchValue in_Pitch, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = NotifParamType_Pitch;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.UnionType.pitch = in_Pitch;
	l_Params.pExceptObjects = in_pExceptArray;
	ParamNotification( l_Params );
}

void CAkAudioNode::VolumeNotification( AkVolumeValue in_Volume, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = NotifParamType_Volume;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.UnionType.volume = in_Volume;
	l_Params.pExceptObjects = in_pExceptArray;
	ParamNotification( l_Params );
}
		
void CAkAudioNode::LPFNotification( AkLPFType in_LPF, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = NotifParamType_LPF;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.UnionType.LPF = in_LPF;
	l_Params.pExceptObjects = in_pExceptArray;
	ParamNotification( l_Params );
}

void CAkAudioNode::LFENotification( AkVolumeValue in_LFE, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = NotifParamType_LFE;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.UnionType.LFE = in_LFE;
	l_Params.pExceptObjects = in_pExceptArray;
	ParamNotification( l_Params );
}

void CAkAudioNode::FeedbackVolumeNotification( AkVolumeValue in_FeedbackVolume, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = NotifParamType_FeedbackVolume;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.UnionType.FeedbackVolume = in_FeedbackVolume;
	l_Params.pExceptObjects = in_pExceptArray;
	ParamNotification( l_Params );
}

void CAkAudioNode::FeedbackPitchNotification( AkPitchValue in_Pitch, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = NotifParamType_FeedbackBusPitch;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.UnionType.pitch = in_Pitch;
	l_Params.pExceptObjects = in_pExceptArray;
	ParamNotification( l_Params );
}

void CAkAudioNode::FeedbackLPFNotification( AkLPFType in_FeedbackLPF, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = NotifParamType_FeedbackLPF;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.UnionType.FeedbackLPF = in_FeedbackLPF;
	l_Params.pExceptObjects = in_pExceptArray;
	ParamNotification( l_Params );
}


AKRESULT CAkAudioNode::PrepareNodeData( AkUniqueID in_NodeID )
{
	AKRESULT eResult = AK_Fail;
	CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_NodeID );

	if( pNode )
	{
		eResult = pNode->PrepareData();
		if(eResult != AK_Success)
		{
			pNode->Release();
		}
		//pNode->Release();// must keep the AudionodeAlive as long as it is prepared, an the node is required to unprepare the data.
	}

	return eResult;
}

void CAkAudioNode::UnPrepareNodeData( AkUniqueID in_NodeID )
{
	CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_NodeID );

	if( pNode )
	{
		pNode->UnPrepareData();
		pNode->Release();
		pNode->Release();// This is to compensate for the release that have not been done in PrepareData()
	}
}
