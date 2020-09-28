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
// AkEvent.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkEvent.h"
#include "AkAction.h"
#include "AkAudioLibIndex.h"
#include "AkAudioMgr.h"
#include "AkBankFloatConversion.h"
#include "AkAudioNode.h"

extern AkMemPoolId g_DefaultPoolId;

CAkEvent::CAkEvent(AkUniqueID in_ulID)
:CAkIndexable(in_ulID)
,m_iPreparationCount(0)
{
}

CAkEvent::~CAkEvent()
{
	Clear();
	m_actions.Term();
	AKASSERT( m_iPreparationCount == 0 );
}

CAkEvent* CAkEvent::Create(AkUniqueID in_ulID)
{
	CAkEvent* pEvent = AkNew( g_DefaultPoolId, CAkEvent(in_ulID) );
	if( pEvent )
	{
		pEvent->AddToIndex();
	}
	return pEvent;
}

CAkEvent* CAkEvent::CreateNoIndex(AkUniqueID in_ulID)
{
	return AkNew( g_DefaultPoolId, CAkEvent(in_ulID) );
}

void CAkEvent::AddToIndex()
{
	AKASSERT( g_pIndex );
	AKASSERT( ID() != AK_INVALID_UNIQUE_ID );
	g_pIndex->m_idxEvents.SetIDToPtr( this );
}

void CAkEvent::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	AKASSERT( ID() != AK_INVALID_UNIQUE_ID );
	g_pIndex->m_idxEvents.RemoveID(ID());
}

AkUInt32 CAkEvent::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxEvents.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkEvent::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxEvents.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
		RemoveFromIndex();
		AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

AKRESULT CAkEvent::Add( AkUniqueID in_ulAction )
{
	if( in_ulAction == AK_INVALID_UNIQUE_ID )
	{
		return AK_InvalidID;
	}

	CAkAction* pAction = g_pIndex->m_idxActions.GetPtrAndAddRef(in_ulAction);
	if ( !pAction )
		return AK_Fail;

	AKASSERT( !m_actions.Exists(pAction) );

	if ( !m_actions.AddLast(pAction) )
	{
		pAction->Release();
		return AK_Fail;
	}

	return AK_Success;
}

void CAkEvent::Remove( AkUniqueID in_ulAction )
{
	CAkAction* pAction = g_pIndex->m_idxActions.GetPtrAndAddRef(in_ulAction);
	if ( pAction )
	{
	    if ( m_actions.Remove( pAction ) == AK_Success )
			pAction->Release();
	
		pAction->Release();
	}
}

void CAkEvent::Clear()
{
	for( AkActionList::Iterator iter = m_actions.Begin(); iter != m_actions.End(); ++iter )
	{
		(*iter)->Release();
	}
	m_actions.RemoveAll();
}

AKRESULT CAkEvent::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	// Read ID

	// We don't care about the ID, just skip it
	AkUInt32 ulID = READBANKDATA(AkUInt32, in_pData, in_ulDataSize );

	// Read Action List Size
	AkUInt32 ulActionListSize = READBANKDATA(AkUInt32, in_pData, in_ulDataSize );

	AKRESULT eResult = m_actions.Reserve( ulActionListSize );
	if( eResult != AK_Success )
		return eResult;

	for(AkUInt32 i = 0; i < ulActionListSize; ++i)
	{
		AkUInt32 ulActionID = READBANKDATA(AkUInt32, in_pData, in_ulDataSize );

		eResult = Add(ulActionID);
		if(eResult != AK_Success)
		{
			break;
		}
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AkUInt32 CAkEvent::GetNumActions()
{
	return m_actions.Length();
}

AKRESULT CAkEvent::QueryAudioObjectIDs( AkUInt32& io_ruNumItems, AkObjectInfo* out_aObjectInfos )
{
	AkUInt32 uIndex_Out = 0;
	AkUInt32 uDepth = 0;
	AKRESULT eResult = AK_Success;
	CAkEvent::AkActionList::Iterator iter = m_actions.Begin();
	while( iter != m_actions.End() )
	{
		if( (*iter)->ActionType() != AkActionType_Play )
			continue;

		AkUniqueID audioNodeID = (*iter)->ElementID();
		CAkAudioNode * pObj = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( audioNodeID );
		AKASSERT( pObj );
		if( !pObj )
			return AK_UnknownObject;

		if( io_ruNumItems == 0 )
			uIndex_Out++;
		else
		{
			out_aObjectInfos[uIndex_Out].objID = audioNodeID;
			out_aObjectInfos[uIndex_Out].parentID = pObj->Parent() ? pObj->Parent()->ID() : AK_INVALID_UNIQUE_ID;
			out_aObjectInfos[uIndex_Out].iDepth = uDepth;
			uIndex_Out++;
			if( uIndex_Out >= io_ruNumItems )
			{
				pObj->Release();
				break; //exit while() loop
			}
		}

		//now query all children of this object
		eResult = pObj->GetChildren( io_ruNumItems, out_aObjectInfos, uIndex_Out, ++uDepth );
		pObj->Release();
		if( eResult != AK_Success )
			break;

		++iter;
	}

	if( eResult == AK_Success && io_ruNumItems == 0 )
		eResult = AK_PartialSuccess;

	io_ruNumItems = uIndex_Out;
	return eResult;
}
