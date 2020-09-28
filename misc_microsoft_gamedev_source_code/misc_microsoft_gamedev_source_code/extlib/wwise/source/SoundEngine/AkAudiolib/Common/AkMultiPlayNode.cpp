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
// AkMultiPlayNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMultiPlayNode.h"
#include "AkPBI.h"
#include "AkPathManager.h"

SafeContinuationList::SafeContinuationList( AkPBIParams& in_rPBIParams, CAkMultiPlayNode* in_pMultiPlayNode )
{
	m_spBackupContinuationList = NULL;
	if ( in_rPBIParams.eType == AkPBIParams::ContinuousPBI )
	{
		AKASSERT( in_rPBIParams.pContinuousParams );
		if( in_rPBIParams.pContinuousParams->spContList )
		{
			m_spBackupContinuationList = in_rPBIParams.pContinuousParams->spContList;
			in_pMultiPlayNode->ContRefList( m_spBackupContinuationList );
		}
	}
}

AkContParamsAndPath::AkContParamsAndPath( ContParams* in_pFrom )
	:m_continuousParams( in_pFrom )
{
	if( g_pPathManager )
	{
		if( m_continuousParams.pPathInfo->pPBPath )
		{
			g_pPathManager->AddPotentialUser( m_continuousParams.pPathInfo->pPBPath );
		}
	}
}

AkContParamsAndPath::~AkContParamsAndPath()
{
	if( g_pPathManager )
	{
		if( m_continuousParams.pPathInfo->pPBPath )
		{
			g_pPathManager->RemovePotentialUser( m_continuousParams.pPathInfo->pPBPath );
		}
	}
}


CAkMultiPlayNode::CAkMultiPlayNode( AkUniqueID in_ulID )
:CAkContainerBase( in_ulID )
{
}

CAkMultiPlayNode::~CAkMultiPlayNode()
{
}

bool CAkMultiPlayNode::IsContinuousPlayback()
{
	// By default, nodes are supposed to be in non continuous playback.
	// overload this function if the inheriting node has the possibility to be virtual.
	return false;
}

AKRESULT CAkMultiPlayNode::Init()
{
	AKRESULT eResult = CAkContainerBase::Init();
	if( eResult == AK_Success )
	{
		eResult = m_listContParameters.Init( AK_MIN_CONTINUOUS_ITEM_IN_SWLIST, AK_MAX_CONTINUOUS_ITEM_IN_SWLIST, g_DefaultPoolId ); 
	}
	return eResult;
}

void CAkMultiPlayNode::Term()
{
	m_listContParameters.Term();
}

AKRESULT CAkMultiPlayNode::ContRefList( CAkContinuationList* in_pList )
{
	AkUInt32* l_pulCount = m_listContParameters.Exists( in_pList );
	if( l_pulCount )
	{
		++(*l_pulCount);
	}
	else
	{
		if ( !m_listContParameters.Set( in_pList, 1 ) ) 
			return AK_Fail;
	}

	return AK_Success;
}

CAkContinuationList* CAkMultiPlayNode::ContGetList( CAkContinuationList* in_pList )
{
	CAkContinuationList* l_pContList = NULL;

	AkUInt32* l_pulCount = m_listContParameters.Exists( in_pList );
	if( l_pulCount )
	{
		if( (*l_pulCount) <= 1 )
		{
			if( !IsContinuousPlayback() )
			{
				l_pContList = in_pList;
			}
			m_listContParameters.Unset( in_pList );
		}
		else
		{
			--(*l_pulCount);
		}
	}

	return l_pContList;
}

AKRESULT CAkMultiPlayNode::ContUnrefList( CAkContinuationList* in_pList )
{
	AkUInt32* l_pulCount = m_listContParameters.Exists( in_pList );
	if( l_pulCount )
	{
		if( (*l_pulCount) <= 1 )
		{
			m_listContParameters.Unset( in_pList );
		}
		else
		{
			--(*l_pulCount);
		}
	}

	return AK_Success;
}


AKRESULT CAkMultiPlayNode::AddMultiplayItem( AkContParamsAndPath& in_rContParams, AkPBIParams& in_rParams, SafeContinuationList& in_rSafeContList /*warning, not necessarily the one in the in_rContParams*/ )
{
	CAkContinueListItem* pItem = in_rContParams.Get().spContList->m_listItems.AddLast();
	if ( pItem )
	{
		pItem->m_pAlternateContList = in_rSafeContList.Get();
		pItem->m_pMultiPlayNode = this;

		// The following line is there only for Wwise and is used only in the situation of a force next to play command
		// this is voluntarily not set in #ifndef AK_OPTIMIZED, but it could be
		pItem->m_LoopingInfo = in_rParams.pContinuousParams->spContList->m_listItems.Begin().pItem->m_LoopingInfo;

		if( pItem->m_pAlternateContList )
		{
			ContRefList( pItem->m_pAlternateContList );
		}
	}
	else
	{
		in_rContParams.Get().spContList = NULL;
		return AK_InsufficientMemory;
	}
	return AK_Success;
}

AKRESULT CAkMultiPlayNode::PlayAndContinueAlternateMultiPlay( AkPBIParams& in_rPBIParams )
{
	AKRESULT eResult = AK_Success;

	AkContParamsAndPath continuousParams( in_rPBIParams.pContinuousParams );

    continuousParams.Get().spContList = ContGetList( in_rPBIParams.pContinuousParams->spContList );

	if( continuousParams.Get().spContList )
	{
		in_rPBIParams.pContinuousParams = &continuousParams.Get();
		eResult = PlayAndContinueAlternate( in_rPBIParams );
		if( eResult == AK_PartialSuccess )
		{
			eResult = AK_Success;
		}
	}

	return eResult;
}


