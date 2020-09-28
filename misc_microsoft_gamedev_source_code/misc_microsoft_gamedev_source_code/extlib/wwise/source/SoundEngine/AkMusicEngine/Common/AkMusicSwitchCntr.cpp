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

//////////////////////////////////////////////////////////////////////
//
// AkMusicRanSeqCntr.cpp
//
// Music Random/Sequence container definition.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMusicSwitchCntr.h"
#include "AkMusicSwitchCtx.h"
#include "AkCritical.h"
#include "AkPBI.h"
#include <AK/SoundEngine/Common/AkSoundEngine.h>

extern AkMemPoolId g_DefaultPoolId;


CAkMusicSwitchCntr::CAkMusicSwitchCntr( AkUniqueID in_ulID )
:CAkMusicTransAware( in_ulID )
,m_bIsContinuePlayback( true )
{
}
CAkMusicSwitchCntr::~CAkMusicSwitchCntr()
{
    Term();
}

// Thread safe version of the constructor.
CAkMusicSwitchCntr * CAkMusicSwitchCntr::Create(
    AkUniqueID in_ulID
    )
{
    CAkFunctionCritical SpaceSetAsCritical;
	CAkMusicSwitchCntr * pSwitchCntr = AkNew( g_DefaultPoolId, CAkMusicSwitchCntr( in_ulID ) );
    if( pSwitchCntr )
	{
		if( pSwitchCntr->Init() != AK_Success )
		{
			pSwitchCntr->Release();
			pSwitchCntr = NULL;
		}
	}
    return pSwitchCntr;
}

AKRESULT CAkMusicSwitchCntr::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = SetMusicTransNodeParams( in_pData, in_ulDataSize, false );

	AkGroupType		l_groupType			= READBANKDATA( AkGroupType, in_pData, in_ulDataSize );
	AkUInt32		l_ulSwitchGroup		= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	AkUInt32		l_ulDefaultSwitch	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	ContinuePlayback( READBANKDATA( AkUInt8, in_pData, in_ulDataSize ) != 0 );

	SetSwitchGroup( l_ulSwitchGroup, l_groupType );
	SetDefaultSwitch( l_ulDefaultSwitch );

	AkUInt32 numSwitchAssocs = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	if( numSwitchAssocs )
	{
		AkMusicSwitchAssoc* pAssocs = (AkMusicSwitchAssoc*)AkAlloc( g_DefaultPoolId, numSwitchAssocs*sizeof(AkMusicSwitchAssoc) );
		if( !pAssocs ) 
			return AK_Fail;
		for( AkUInt32 i = 0; i < numSwitchAssocs; ++i )
		{
			pAssocs[i].switchID = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pAssocs[i].nodeID	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		}
		SetSwitchAssocs( numSwitchAssocs, pAssocs );
		AkFree( g_DefaultPoolId, pAssocs );
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

void CAkMusicSwitchCntr::Term()
{
    m_arSwitchNode.Term();
}

// Return the node category.
AkNodeCategory CAkMusicSwitchCntr::NodeCategory()
{
    return AkNodeCategory_MusicSwitchCntr;
}

// Hierarchy enforcement: Music RanSeq Cntr can only have Segments as parents.
AKRESULT CAkMusicSwitchCntr::CanAddChild(
    CAkAudioNode * in_pAudioNode 
    )
{
    AKASSERT( in_pAudioNode );

	AkNodeCategory eCategory = in_pAudioNode->NodeCategory();

	AKRESULT eResult = AK_Success;	
	if(Children() >= AK_MAX_NUM_CHILD)
	{
		eResult = AK_MaxReached;
	}
	else if(eCategory != AkNodeCategory_MusicSegment &&
            eCategory != AkNodeCategory_MusicRanSeqCntr &&
            eCategory != AkNodeCategory_MusicSwitchCntr )
	{
		eResult = AK_NotCompatible;
	}
	else if(in_pAudioNode->Parent() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

CAkMatrixAwareCtx * CAkMusicSwitchCntr::CreateContext( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams,
    CAkSegmentBucket *& out_pFirstRelevantBucket
    )
{
    CAkMusicSwitchCtx * pSwitchCntrCtx = AkNew( g_DefaultPoolId, CAkMusicSwitchCtx( 
        this,
        in_pParentCtx ) );
    if ( pSwitchCntrCtx )
    {
		pSwitchCntrCtx->AddRef();
        if ( pSwitchCntrCtx->Init( in_GameObject, 
                                   in_rUserparams, 
                                   m_ulGroupID, 
                                   m_eGroupType, 
                                   out_pFirstRelevantBucket ) == AK_Success )
		{
			pSwitchCntrCtx->Release();
		}
		else
        {
            pSwitchCntrCtx->OnStopped( 0 );
			pSwitchCntrCtx->Release();
            pSwitchCntrCtx = NULL;
        }
    }
    return pSwitchCntrCtx;

}

AKRESULT CAkMusicSwitchCntr::Play( AkPBIParams& in_rPBIParams )
{
    // Create a top-level switch container context (that is, attached to the Music Renderer).
    CAkSegmentBucket * pFirstRelevantBucket;
    
    // OPTIM. Could avoid virtual call.
    CAkMatrixAwareCtx * pCtx = CreateContext( NULL, in_rPBIParams.pGameObj, in_rPBIParams.userParams, pFirstRelevantBucket );
    if ( pCtx )
    {
        AKASSERT( pFirstRelevantBucket );

		// Complete initialization of the context.
		pCtx->EndInit();

        // Do not set source offset: let it start playback at the position specified by the descendant's 
        // transition rules.
        
        AkMusicFade fadeParams;
        fadeParams.transitionTime   = in_rPBIParams.pTransitionParameters->TransitionTime;
		fadeParams.eFadeCurve       = in_rPBIParams.pTransitionParameters->eFadeCurve;
		// Set fade offset to segment context's look-ahead time (if we don't switch to null).
        if ( pFirstRelevantBucket->SegmentCtx() )
            fadeParams.iFadeOffset  = pFirstRelevantBucket->SegmentCtx()->GetStreamingLookAheadTime();
        else
        	fadeParams.iFadeOffset	= 0;
        return pCtx->_Play( fadeParams );
    }
    return AK_Fail;
}

AKRESULT CAkMusicSwitchCntr::SetSwitchAssocs(
		AkUInt32 in_uNumAssocs,
		AkMusicSwitchAssoc* in_pAssocs
		)
{
	m_arSwitchNode.RemoveAll();
	for( AkUInt32 i = 0; i < in_uNumAssocs; ++i )
	{
		AkUniqueID* pID = m_arSwitchNode.Set( in_pAssocs[i].switchID );
		if( pID )
			*pID = in_pAssocs[i].nodeID;
		else
			return AK_Fail;
	}
	return AK_Success;
}

AKRESULT CAkMusicSwitchCntr::ModifyActiveState( AkUInt32 in_stateID, bool in_bSupported )
{
	AKRESULT eResult = AK_Success;

	if( m_uPreparationCount != 0 )
	{
		// seek in the switch list, to get the right node.

		SwitchNodeAssoc::Iterator iter = m_arSwitchNode.FindEx( in_stateID );
		if( iter != m_arSwitchNode.End() )
		{	
			if( in_bSupported )
			{
				eResult = PrepareNodeData( iter.pItem->item );
			}
			else
			{
				UnPrepareNodeData( iter.pItem->item );
			}
		}
		//else
		//{
			//not finding a switch is a success.
		//}
	}

	return eResult;
}

AKRESULT CAkMusicSwitchCntr::PrepareData()
{
	extern AkInitSettings g_settings;
	if( !g_settings.bEnableGameSyncPreparation )
	{
		return CAkMusicNode::PrepareData();
	}

	AKRESULT eResult = AK_Success;

	if( m_uPreparationCount == 0 )
	{
		eResult = PrepareMusicalDependencies();
		if( eResult == AK_Success )
		{

			CAkPreparedContent* pPreparedContent = GetPreparedContent( m_ulGroupID, m_eGroupType );
			if( pPreparedContent )
			{
				for( SwitchNodeAssoc::Iterator iter = m_arSwitchNode.Begin(); iter != m_arSwitchNode.End(); ++iter )
				{
					if( pPreparedContent->IsIncluded( iter.pItem->key ) )
					{
						eResult = PrepareNodeData( iter.pItem->item );
					}
					if( eResult != AK_Success )
					{
						// Do not let this half-prepared, unprepare what has been prepared up to now.
						for( SwitchNodeAssoc::Iterator iterFlush = m_arSwitchNode.Begin(); iterFlush != iter; ++iterFlush )
						{
							if( pPreparedContent->IsIncluded( iterFlush.pItem->key ) )
							{
								UnPrepareNodeData( iter.pItem->item );
							}
						}
					}
				}
				if( eResult == AK_Success )
				{
					++m_uPreparationCount;
					eResult = SubscribePrepare( m_ulGroupID, m_eGroupType );
					if( eResult != AK_Success )
						UnPrepareData();
				}
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
			if( eResult != AK_Success )
			{
				UnPrepareMusicalDependencies();
			}
		}
	}
	return eResult;
}

void CAkMusicSwitchCntr::UnPrepareData()
{
	extern AkInitSettings g_settings;
	if( !g_settings.bEnableGameSyncPreparation )
	{
		return CAkMusicNode::UnPrepareData();
	}

	if( m_uPreparationCount != 0 ) // must check in case there were more unprepare than prepare called that succeeded.
	{
		if( --m_uPreparationCount == 0 )
		{
			CAkPreparedContent* pPreparedContent = GetPreparedContent( m_ulGroupID, m_eGroupType );
			if( pPreparedContent )
			{
				for( SwitchNodeAssoc::Iterator iter = m_arSwitchNode.Begin(); iter != m_arSwitchNode.End(); ++iter )
				{
					if( pPreparedContent->IsIncluded( iter.pItem->key ) )
					{
						UnPrepareNodeData( iter.pItem->item );
					}
				}
			}
			CAkPreparationAware::UnsubscribePrepare( m_ulGroupID, m_eGroupType );
			UnPrepareMusicalDependencies();
		}
	}
}

void CAkMusicSwitchCntr::SetSwitchGroup( 
    AkUInt32 in_ulGroup, 
    AkGroupType in_eGroupType 
    )
{
    m_eGroupType = in_eGroupType; // Is it binded to state or to switches
	m_ulGroupID = in_ulGroup;
}

// Interface for Wwise
// ----------------------
// TEMP.
AKRESULT CAkMusicSwitchCntr::ObsoleteAddSwitch( 
    AkSwitchStateID in_switchID 
    )
{
    if ( !m_arSwitchNode.Exists( in_switchID ) )
    {
        return m_arSwitchNode.Set( in_switchID, AK_MUSIC_TRANSITION_RULE_ID_NONE ) ? AK_Success : AK_Fail;
    }
    return AK_Success;
}

void CAkMusicSwitchCntr::ObsoleteRemoveSwitch( 
    AkSwitchStateID in_switchID 
    )
{
    m_arSwitchNode.Unset( in_switchID );
}

AKRESULT CAkMusicSwitchCntr::ObsoleteSetNodeInSwitch(
	AkSwitchStateID in_switchID,
	AkUniqueID		in_nodeID
	)
{
    if( in_nodeID == AK_INVALID_UNIQUE_ID)
	{
        AKASSERT( !"Invalid node ID" );
		return AK_InvalidParameter;
	}

	AkUniqueID * pNodeID = m_arSwitchNode.Exists( in_switchID );
	if( pNodeID )
	{
		*pNodeID = in_nodeID;
        return AK_Success;
	}
	return AK_InvalidSwitchType;
}

// Interface for Contexts
// ----------------------

 AKRESULT CAkMusicSwitchCntr::GetSwitchNode(
    AkUniqueID in_switchID,
    AkUniqueID & out_nodeID
    )
{
    AkUniqueID * pNodeID = m_arSwitchNode.Exists( in_switchID );
    if ( pNodeID )
    {
        out_nodeID = *pNodeID;
        return AK_Success;
    }
    // Switch ID does not exist. Return child associated with default switch.
    pNodeID = m_arSwitchNode.Exists( m_ulDefaultSwitch );
    if ( pNodeID )
    {
        out_nodeID = *pNodeID;
        return AK_Success;
    }
    out_nodeID = AK_MUSIC_TRANSITION_RULE_ID_NONE;
    return AK_InvalidSwitchType;
}

