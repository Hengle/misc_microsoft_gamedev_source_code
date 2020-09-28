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
// AkActionSetValue.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionSetValue.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include "AkRegistryMgr.h"
#include "AkBankFloatConversion.h"
#include "AkModifiers.h"
#include "AkRegisteredObj.h"

#include "AkAudioMgr.h"

CAkActionSetValue::CAkActionSetValue(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionExcept(in_eActionType, in_ulID)
, m_eFadeCurve(AkCurveInterpolation_Linear)
{
}

CAkActionSetValue::~CAkActionSetValue()
{

}

AKRESULT CAkActionSetValue::Init()
{
	AKRESULT eResult = CAkActionExcept::Init();

	RandomizerModifier::SetModValue<AkTimeMs>( m_TransitionTime, 0 );

	return eResult;
}

void CAkActionSetValue::TransitionTime(const AkTimeMs in_TransitionTime, const AkTimeMs in_RangeMin/*=0*/, const AkTimeMs in_RangeMax/*=0*/)
{
	RandomizerModifier::SetModValue( m_TransitionTime, in_TransitionTime, in_RangeMin, in_RangeMax );
}

AKRESULT CAkActionSetValue::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pIndex);
	AKASSERT(g_pRegistryMgr);
	AKRESULT eResult = AK_Success;

	CAkParameterNodeBase* pNode = NULL;

	switch(ActionType())
	{
		case AkActionType_SetVolume_O:
		case AkActionType_SetPitch_O:
		case AkActionType_Mute_O:
		case AkActionType_SetLFE_O:
		case AkActionType_SetLPF_O:
			pNode = static_cast<CAkParameterNodeBase*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_ulElementID));
			if(pNode)
			{
				ExecSetValue(pNode, in_pAction->GameObj());
				pNode->Release();
			}
			break;
		case AkActionType_SetVolume_M:
		case AkActionType_SetPitch_M:
		case AkActionType_Mute_M:
		case AkActionType_SetLFE_M:
		case AkActionType_SetLPF_M:
			pNode = static_cast<CAkParameterNodeBase*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_ulElementID));
			if(pNode)
			{
				ExecSetValue(pNode);
				pNode->Release();
			}
			break;
		case AkActionType_ResetVolume_O:
		case AkActionType_ResetPitch_O:
		case AkActionType_Unmute_O:
		case AkActionType_ResetLFE_O:
		case AkActionType_ResetLPF_O:
			pNode = static_cast<CAkParameterNodeBase*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_ulElementID));
			if(pNode)
			{
				ExecResetValue(pNode,in_pAction->GameObj());
				pNode->Release();
			}
			break;
		case AkActionType_ResetVolume_M:
		case AkActionType_ResetPitch_M:
		case AkActionType_Unmute_M:
		case AkActionType_ResetLFE_M:
		case AkActionType_ResetLPF_M:
			pNode = static_cast<CAkParameterNodeBase*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_ulElementID));
			if(pNode)
			{
				ExecResetValue(pNode);
				pNode->Release();
			}
			break;
		case AkActionType_ResetVolume_ALL_O:
		case AkActionType_ResetPitch_ALL_O:
		case AkActionType_Unmute_ALL_O:
		case AkActionType_ResetLFE_ALL_O:
		case AkActionType_ResetLPF_ALL_O:
			{
				CAkRegistryMgr::AkListNode listID;
				eResult = listID.Init( MIN_SIZE_REG_ENTRIES, MAX_SIZE_REG_ENTRIES,g_DefaultPoolId );
				if( eResult == AK_Success )
				{
					in_pAction->GameObj()->FillElementList( listID );

					for( CAkRegistryMgr::AkListNode::Iterator iter = listID.Begin(); iter != listID.End(); ++iter )
					{
						pNode = static_cast<CAkParameterNodeBase*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef( *iter ) );
						if(pNode)
						{
							ExecResetValue( pNode, in_pAction->GameObj() );
							pNode->Release();
						}
					}
					listID.Term();
				}
				break;
			}
		case AkActionType_ResetVolume_ALL:
		case AkActionType_ResetPitch_ALL:
		case AkActionType_Unmute_ALL:
		case AkActionType_ResetLFE_ALL:
		case AkActionType_ResetLPF_ALL:
			{
				CAkRegistryMgr::AkListNode listID;
				eResult = listID.Init( MIN_SIZE_REG_ENTRIES, MAX_SIZE_REG_ENTRIES,g_DefaultPoolId );
				if( eResult == AK_Success )
				{
					g_pRegistryMgr->GetAllModifiedElements( listID );

					for( CAkRegistryMgr::AkListNode::Iterator iter = listID.Begin(); iter != listID.End(); ++iter )
					{
						pNode = static_cast<CAkParameterNodeBase*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef( *iter ) );
						if(pNode)
						{
							ExecResetValueAll(pNode);
							pNode->Release();
						}
					}
					listID.Term();
				}
				break;
			}
		case AkActionType_ResetVolume_AE:
		case AkActionType_ResetPitch_AE:
		case AkActionType_Unmute_AE:
		case AkActionType_ResetLFE_AE:
		case AkActionType_ResetLPF_AE:
			{
				CAkRegistryMgr::AkListNode listID;
				eResult = listID.Init( MIN_SIZE_REG_ENTRIES, MAX_SIZE_REG_ENTRIES,g_DefaultPoolId );
				if( eResult == AK_Success )
				{
					g_pRegistryMgr->GetAllModifiedElements( listID );
					for( CAkRegistryMgr::AkListNode::Iterator iter = listID.Begin(); iter != listID.End(); ++iter )
					{
						pNode = static_cast<CAkParameterNodeBase*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef( *iter ) );
						if(pNode)
						{
							ExecResetValueExcept(pNode);
							pNode->Release();
						}
					}
					listID.Term();
				}
				break;
			}
		case AkActionType_ResetVolume_AE_O:
		case AkActionType_ResetPitch_AE_O:
		case AkActionType_Unmute_AE_O:
		case AkActionType_ResetLFE_AE_O:
		case AkActionType_ResetLPF_AE_O:
			{
				CAkRegistryMgr::AkListNode listID;
				eResult = listID.Init( MIN_SIZE_REG_ENTRIES, MAX_SIZE_REG_ENTRIES,g_DefaultPoolId );
				if( eResult == AK_Success )
				{
					in_pAction->GameObj()->FillElementList( listID );
					
					for( CAkRegistryMgr::AkListNode::Iterator iter = listID.Begin(); iter != listID.End(); ++iter )
					{
						pNode = static_cast<CAkParameterNodeBase*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef( *iter ) );
						if( pNode )
						{
							ExecResetValueExcept( pNode, in_pAction->GameObj() );
							pNode->Release();
						}
					}
					listID.Term();
				}
				break;
			}
		default:
			AKASSERT(!"Unknown or unsupported Action Type Requested");
			break;
	}
	return eResult;
}

AKRESULT CAkActionSetValue::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKRESULT eResult = AK_Success;

	AkTimeMs TTime = READBANKDATA(AkTimeMs, io_rpData, io_rulDataSize );

	AkTimeMs TTimeMin = READBANKDATA(AkTimeMs, io_rpData, io_rulDataSize );

	AkTimeMs TTimeMax = READBANKDATA(AkTimeMs, io_rpData, io_rulDataSize );

	AkUInt8 ucFadeCurveType = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize );

	m_eFadeCurve = (AkCurveInterpolation)ucFadeCurveType;
	RandomizerModifier::SetModValue( m_TransitionTime, TTime, TTimeMin, TTimeMax);

	eResult = SetActionSpecificParams(io_rpData, io_rulDataSize);

	if(eResult == AK_Success)
	{
		eResult = SetExceptParams( io_rpData, io_rulDataSize );
	}

	return AK_Success;
}
