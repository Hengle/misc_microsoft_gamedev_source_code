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
// AkActionBypassFX.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdAfx.h"
#include "AkActionBypassFX.h"
#include "AkBankFloatConversion.h"
#include "AkParameterNodeBase.h"
#include "AkAudiolibIndex.h"
#include "AkRegistryMgr.h"
#include "AkRegisteredObj.h"

#include "AkAudioMgr.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionBypassFX::CAkActionBypassFX( AkActionType in_eActionType, AkUniqueID in_ulID ) 
: CAkActionExcept( in_eActionType, in_ulID )
, m_bIsBypass( false )
, m_uTargetMask( 0xFFFFFFFF )
{
}

CAkActionBypassFX::~CAkActionBypassFX()
{
}

AKRESULT CAkActionBypassFX::Execute( AkPendingAction * in_pAction )
{
	AKRESULT eResult = AK_Fail;

	CAkAudioNode* pNode = NULL;

	CAkRegisteredObj * pGameObj = in_pAction->GameObj();

	switch( ActionType() )
	{
	case AkActionType_BypassFX_M:
	case AkActionType_BypassFX_O:
		pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_ulElementID );
		if( pNode )
		{
			eResult = static_cast<CAkParameterNodeBase*>(pNode)->BypassFX( m_bIsBypass ? m_uTargetMask : 0, m_uTargetMask, pGameObj );
			pNode->Release();
		}
		break;

	case AkActionType_ResetBypassFX_M:
	case AkActionType_ResetBypassFX_O:
		pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_ulElementID );
		if( pNode )
		{
			eResult = static_cast<CAkParameterNodeBase*>(pNode)->ResetBypassFX( m_uTargetMask, pGameObj );
			pNode->Release();
		}
		break;

	case AkActionType_ResetBypassFX_ALL:
		{
			CAkRegistryMgr::AkListNode listID;
			eResult = listID.Init( MIN_SIZE_REG_ENTRIES, MAX_SIZE_REG_ENTRIES,g_DefaultPoolId );
			if( eResult == AK_Success )
			{
				g_pRegistryMgr->GetAllModifiedElements( listID );

				for( CAkRegistryMgr::AkListNode::Iterator iter = listID.Begin(); iter != listID.End(); ++iter )
				{
					pNode = static_cast<CAkParameterNodeBase*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef( *iter ) );
					if( pNode )
					{
						eResult = static_cast<CAkParameterNodeBase*>(pNode)->ResetBypassFX( m_uTargetMask );
						pNode->Release();
					}
				}
				listID.Term();
			}
		}
		break;

	case AkActionType_ResetBypassFX_ALL_O:
		{
			CAkRegistryMgr::AkListNode listID;
			eResult = listID.Init( MIN_SIZE_REG_ENTRIES, MAX_SIZE_REG_ENTRIES, g_DefaultPoolId );
			if( eResult == AK_Success )
			{
				pGameObj->FillElementList( listID );

				for( CAkRegistryMgr::AkListNode::Iterator iter = listID.Begin(); iter != listID.End(); ++iter )
				{
					pNode = static_cast<CAkParameterNodeBase*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef( *iter ) );
					if(pNode)
					{
						eResult = static_cast<CAkParameterNodeBase*>(pNode)->ResetBypassFX( m_uTargetMask, pGameObj );
						pNode->Release();
					}
				}
				listID.Term();
			}
		}
		break;

	case AkActionType_ResetBypassFX_AE:
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
						bool l_bIsException = false;
						for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
						{
							if( (*iter) == pNode->ID() )
							{
								l_bIsException = true;
								break;
							}
						}
						if( !l_bIsException )
						{
							eResult = static_cast<CAkParameterNodeBase*>(pNode)->ResetBypassFX( m_uTargetMask );
						}
						pNode->Release();
					}
				}
				listID.Term();
			}
		}
		break;

	case AkActionType_ResetBypassFX_AE_O:
		{
			CAkRegistryMgr::AkListNode listID;
			eResult = listID.Init( MIN_SIZE_REG_ENTRIES, MAX_SIZE_REG_ENTRIES, g_DefaultPoolId );
			if( eResult == AK_Success )
			{
				pGameObj->FillElementList( listID );
				
				for( CAkRegistryMgr::AkListNode::Iterator iter = listID.Begin(); iter != listID.End(); ++iter )
				{
					pNode = static_cast<CAkParameterNodeBase*>( g_pIndex->m_idxAudioNode.GetPtrAndAddRef( *iter ) );
					if( pNode )
					{
						bool l_bIsException = false;
						for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
						{
							if( (*iter) == pNode->ID() )
							{
								l_bIsException = true;
								break;
							}
						}
						if( !l_bIsException )
						{
							eResult = static_cast<CAkParameterNodeBase*>(pNode)->ResetBypassFX( m_uTargetMask );
						}
						pNode->Release();
					}
				}
				listID.Term();
			}
		}
		break;
	}

	return eResult;
}

CAkActionBypassFX* CAkActionBypassFX::Create( AkActionType in_eActionType, AkUniqueID in_ulID )
{
	CAkActionBypassFX* pActionBypassFX = AkNew( g_DefaultPoolId, CAkActionBypassFX( in_eActionType, in_ulID ) );
	if( pActionBypassFX )
	{
		if( pActionBypassFX->Init() != AK_Success )
		{
			pActionBypassFX->Release();
			pActionBypassFX = NULL;
		}
	}
	return pActionBypassFX;
}

void CAkActionBypassFX::Bypass( const bool in_bIsBypass )
{
	m_bIsBypass = in_bIsBypass;
}

void CAkActionBypassFX::SetBypassTarget( bool in_bTargetAll, AkUInt32 in_uTargetMask )
{
	m_uTargetMask = in_uTargetMask;
	if ( in_bTargetAll )
		m_uTargetMask |= ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG );
}

AKRESULT CAkActionBypassFX::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt8 l_cVal = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize);
	m_bIsBypass = l_cVal != 0;

	m_uTargetMask = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize);

	return SetExceptParams( io_rpData, io_rulDataSize );
}
