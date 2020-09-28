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
// AkAudioLibIndex.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkAudioLibIndex.h"
#include "AkCritical.h"
#include "AkRanseqCntr.h"

#define AK_STRING_INDEX_MIN_LIST_SIZE 10
#define AK_STRING_INDEX_MAX_LIST_SIZE AK_NO_MAX_LIST_SIZE

CAkAudioLibIndex* CAkAudioLibIndex::pInstance = 0;// initialize singleton pointer

CAkAudioLibIndex* CAkAudioLibIndex::Instance () 
{
	if (pInstance == NULL)
	{
		pInstance = AkNew( g_DefaultPoolId, CAkAudioLibIndex() );
	}
	return pInstance;
}

void CAkAudioLibIndex::Destroy()
{
    if( pInstance )
    {
		AkDelete( g_DefaultPoolId, pInstance );
        pInstance = NULL;
    }
}

AKRESULT CAkAudioLibIndex::Init()
{
	AKRESULT eResult = m_idxAudioNode.Init();
	if( eResult == AK_Success )
	{
		eResult = m_idxStates.Init();
	}
	if( eResult == AK_Success )
	{
		eResult = m_idxCustomStates.Init();
	}
	if( eResult == AK_Success )
	{
		eResult = m_idxEvents.Init();
	}
	if( eResult == AK_Success )
	{
		eResult = m_idxActions.Init();
	}
	if( eResult == AK_Success )
	{
		eResult = m_idxLayers.Init();
	}
	if( eResult == AK_Success )
	{
		eResult = m_idxAttenuations.Init();
	}
	if( eResult == AK_Success )
	{
		eResult = m_idxDynamicSequences.Init();
	}
	if ( eResult == AK_Success )
	{
		eResult = m_idxDialogueEvents.Init();
	}

	return eResult;
}

void CAkAudioLibIndex::Term()
{
	m_idxAudioNode.Term();
	m_idxStates.Term();
	m_idxCustomStates.Term();
	m_idxEvents.Term();
	m_idxActions.Term();
	m_idxLayers.Term();
	m_idxAttenuations.Term();
	m_idxDynamicSequences.Term();
	m_idxDialogueEvents.Term();
}

#ifndef AK_OPTIMIZED
AKRESULT CAkAudioLibIndex::ResetRndSeqCntrPlaylists()
{
	for( AkHashListBare<AkUniqueID, CAkIndexable, 31>::Iterator iter = m_idxAudioNode.m_mapIDToPtr.Begin(); iter != m_idxAudioNode.m_mapIDToPtr.End(); ++iter )
	{
		CAkParameterNodeBase* pNode = static_cast<CAkParameterNodeBase*>( *iter );
		if( pNode->NodeCategory() == AkNodeCategory_RanSeqCntr )
		{
			static_cast<CAkRanSeqCntr*>( pNode )->ResetSpecificInfo();
		}
	}
	return AK_Success;
}
#endif

AKRESULT CAkIndexSiblingItem::SetIDToPtr( AkStateGroupID in_StateGroupID, CAkState* in_Ptr )
{
	AkAutoLock<CAkLock> IndexLock( m_IndexLock );

	CAkIndexItem<CAkState*>* pIndex = GetOrCreateStateGroup( in_StateGroupID );
	if( pIndex )
	{
		pIndex->SetIDToPtr( in_Ptr );
		return AK_Success;
	}
	else
	{
		return AK_InsufficientMemory;
	}
}

//Remove an ID from the index
void CAkIndexSiblingItem::RemoveID( AkStateGroupID in_StateGroupID, AkUniqueID in_ID )
{
	AkAutoLock<CAkLock> IndexLock( m_IndexLock );
	
	CAkIndexItem<CAkState*>* pIndex = GetStateGroup( in_StateGroupID );
	if( pIndex )
	{
		pIndex->RemoveID( in_ID );
	}
}

CAkState* CAkIndexSiblingItem::GetPtrAndAddRef( AkStateGroupID in_StateGroupID, AkUniqueID in_ID ) 
{ 
	AkAutoLock<CAkLock> IndexLock( m_IndexLock ); 

	CAkIndexItem<CAkState*>* pIndex = GetStateGroup( in_StateGroupID );
	if( pIndex )
	{
		return pIndex->GetPtrAndAddRef( in_ID );
	}
	else
	{
		return NULL;
	}
} 


AKRESULT CAkIndexSiblingItem::Init()
{
	return AK_Success;
}

void CAkIndexSiblingItem::Term()
{
	for( AkMapSibling::Iterator iter = m_ArrayStateGroups.Begin(); iter != m_ArrayStateGroups.End(); ++iter )
	{
		(*iter).item->Term();
		AkDelete2( g_DefaultPoolId, CAkIndexItem<CAkState*>, (*iter).item );
	}

	//then term the array
	m_ArrayStateGroups.Term();
}

CAkIndexItem<CAkState*>* CAkIndexSiblingItem::GetStateGroup( AkStateGroupID in_StateGroupID )
{
	CAkIndexItem<CAkState*>** l_ppIndex = m_ArrayStateGroups.Exists( in_StateGroupID );
	if( l_ppIndex )
	{
		return *l_ppIndex;
	}
	return NULL;
}

CAkIndexItem<CAkState*>* CAkIndexSiblingItem::GetOrCreateStateGroup( AkStateGroupID in_StateGroupID )
{
	CAkIndexItem<CAkState*>* pIndex = GetStateGroup( in_StateGroupID );
	if( !pIndex )
	{
		AkNew2( pIndex, g_DefaultPoolId, CAkIndexItem<CAkState*>, CAkIndexItem<CAkState*>() );
		if( pIndex )
		{
			if( pIndex->Init() != AK_Success )
			{
				AkDelete2( g_DefaultPoolId, CAkIndexItem<CAkState*>, pIndex );
				pIndex = NULL;
			}
			else if( !m_ArrayStateGroups.Set( in_StateGroupID, pIndex ) )
			{
				pIndex->Term();
				AkDelete2( g_DefaultPoolId, CAkIndexItem<CAkState*>, pIndex );
				pIndex = NULL;
			}
		}
	}	
	return pIndex;
}

