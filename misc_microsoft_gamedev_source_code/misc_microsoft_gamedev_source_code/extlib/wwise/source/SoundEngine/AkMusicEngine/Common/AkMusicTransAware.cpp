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
// AkMusicTransAware.cpp
//
// Class for music transition aware nodes. 
// Holds a map of music transition rules (based on exceptions), 
// and provides services for rule look-up.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkBankFloatConversion.h"
#include "AkMusicTransAware.h"


CAkMusicTransAware::CAkMusicTransAware( AkUniqueID in_ulID )
:CAkMusicNode( in_ulID )
{
}

CAkMusicTransAware::~CAkMusicTransAware()
{
	FlushTransitionRules();
    m_arTrRules.Term();
}

AKRESULT CAkMusicTransAware::SetMusicTransNodeParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly )
{
	AKRESULT eResult = SetMusicNodeParams( io_rpData, io_rulDataSize, in_bPartialLoadOnly );

	AkUInt32 numRules = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
	if( numRules )
	{
		AkWwiseMusicTransitionRule* pRules = (AkWwiseMusicTransitionRule*)AkAlloc( g_DefaultPoolId, numRules*sizeof( AkWwiseMusicTransitionRule ) );
		if( !pRules )
		{
			return AK_Fail;
		}
		for( AkUInt32 i = 0; i < numRules; ++i )
		{
			pRules[i].srcID		= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].destID	= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

			pRules[i].srcFade.transitionTime	= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRules[i].srcFade.eFadeCurve		= (AkCurveInterpolation)READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].srcFade.iFadeOffset		= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRules[i].eSrcSyncType				= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].bSrcPlayPostExit			= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

			pRules[i].destFade.transitionTime	= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRules[i].destFade.eFadeCurve		= (AkCurveInterpolation)READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].destFade.iFadeOffset		= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRules[i].uDestmarkerID				= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].uDestJumpToID				= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].eDestEntryType			= READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );
			pRules[i].bDestPlayPreEntry			= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

			pRules[i].bIsTransObjectEnabled		= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			pRules[i].segmentID					= READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].transFadeIn.transitionTime	= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRules[i].transFadeIn.eFadeCurve		= (AkCurveInterpolation)READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].transFadeIn.iFadeOffset		= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRules[i].transFadeOut.transitionTime	= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pRules[i].transFadeOut.eFadeCurve		= (AkCurveInterpolation)READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pRules[i].transFadeOut.iFadeOffset		= READBANKDATA( AkInt32, io_rpData, io_rulDataSize );

			pRules[i].bPlayPreEntry		= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			pRules[i].bPlayPostExit	= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
		}

		SetRules( numRules, pRules );
		AkFree( g_DefaultPoolId, pRules );
	}
	return eResult;
}

// Looks up transition rules list from end to beginning, returns when it finds a match.
// Note, returns the whole AkMusicTransitionRule, so that the client can see if the rule
// applies to a certain kind of node, or if it is general (e.g. a switch container might
// want to know if the destination is specifically a sequence).
const AkMusicTransitionRule & CAkMusicTransAware::GetTransitionRule( 
    AkUniqueID  in_srcID,   // Source (departure) node ID.
    AkUniqueID  in_destID   // Destination (arrival) node ID.
    )
{
    AKASSERT( m_arTrRules.Length( ) >= 1 ||
              !"Transition aware must own at least one default rule" );

    int iRule = (int)m_arTrRules.Length( ) - 1;
    while ( iRule >= 0 )
    {
        AkMusicTransitionRule & rule = m_arTrRules[iRule];
        if ( ( rule.srcID == AK_MUSIC_TRANSITION_RULE_ID_ANY || 
               rule.srcID == in_srcID ) &&
             ( rule.destID == AK_MUSIC_TRANSITION_RULE_ID_ANY || 
               rule.destID == in_destID ) )
        {
            return rule;
        }
        --iRule;
    }
    AKASSERT( !"Could not find music transition rule" );
    return m_arTrRules[0];
}

const AkMusicTransitionRule & CAkMusicTransAware::GetTransitionRule( 
	CAkAudioNode * in_pOwnerNode,		// Owner node.
	AkUniqueID  in_srcID,				// Source (departure) node ID.
	CAkAudioNode * in_pSrcParentNode,	// Source (departure) parent node (can be NULL).
    AkUniqueID  in_destID,				// Destination (arrival) node ID.        
    CAkAudioNode * in_pDestParentNode,	// Destination (arrival) parent node (can be NULL).
	bool & out_bIsDestSequenceSpecific	// True when rule's destination is a sequence.
	)
{
    AKASSERT( m_arTrRules.Length( ) >= 1 ||
              !"Transition aware must own at least one default rule" );

	int iRule = (int)m_arTrRules.Length( ) - 1;
    while ( iRule >= 0 )
    {
		CAkAudioNode * pActualDestParentNode = NULL;

        AkMusicTransitionRule & rule = m_arTrRules[iRule];
        if ( ( rule.srcID == AK_MUSIC_TRANSITION_RULE_ID_ANY || 
               rule.srcID == in_srcID ||
			   AscendentMatch( in_pOwnerNode, rule.srcID, in_pSrcParentNode ) ) &&
             ( rule.destID == AK_MUSIC_TRANSITION_RULE_ID_ANY || 
               rule.destID == in_destID ||
               ( pActualDestParentNode = AscendentMatch( in_pOwnerNode, rule.destID, in_pDestParentNode ) ) != NULL ) )
        {
        	if ( pActualDestParentNode && 
        		 pActualDestParentNode->NodeCategory() == AkNodeCategory_MusicRanSeqCntr )
				out_bIsDestSequenceSpecific = true;
			else
				out_bIsDestSequenceSpecific = false;
            return rule;
        }
        --iRule;
    }
    AKASSERT( !"Could not find music transition rule" );
    return m_arTrRules[0];
}

const AkMusicTransitionRule & CAkMusicTransAware::GetPanicTransitionRule()
{
	static AkMusicTransitionRule panicRule;

	panicRule.srcID = AK_MUSIC_TRANSITION_RULE_ID_ANY;
	panicRule.srcRule.eSyncType = SyncTypeExitMarker;
	panicRule.srcRule.bPlayPostExit = true;
	panicRule.srcRule.fadeParams.transitionTime = 0;
	panicRule.srcRule.fadeParams.iFadeOffset = 0;
	panicRule.destID = AK_MUSIC_TRANSITION_RULE_ID_ANY;
	panicRule.destRule.eEntryType = EntryTypeEntryMarker;
	panicRule.destRule.bPlayPreEntry = false;
	panicRule.destRule.fadeParams.transitionTime = 0;
	panicRule.destRule.fadeParams.iFadeOffset = 0;
	panicRule.destRule.markerID = 0;
	panicRule.destRule.uJumpToID = 0;
	panicRule.pTransObj = NULL;
	
	return panicRule;
}

CAkAudioNode * CAkMusicTransAware::AscendentMatch(
	CAkAudioNode *  in_pOwnerNode,		// Owner node.
	AkUniqueID		in_ruleID,
	CAkAudioNode *  in_pNode
	)
{
	AKASSERT( in_ruleID != AK_MUSIC_TRANSITION_RULE_ID_ANY );

	while ( in_pNode &&
			in_pNode != in_pOwnerNode )
	{
		if ( in_pNode->ID() == in_ruleID )
			return in_pNode;
		in_pNode = in_pNode->Parent();
	}
	return NULL;
}


// Interface for Wwise.
// -------------------
AKRESULT CAkMusicTransAware::AddRule(
    AkMusicTransitionRule & in_rule
    )
{
	//AKASSERT( !"AddRule() will soon be destroyed, please don't use it" );
    return ( m_arTrRules.AddLast( in_rule ) ? AK_Success : AK_Fail );
}

AKRESULT CAkMusicTransAware::SetRules(
		AkUInt32 in_NumRules,
		AkWwiseMusicTransitionRule* in_pRules
		)
{
	AKRESULT eResult = AK_Success;

	FlushTransitionRules();

	for( AkUInt32 i = 0 ;  i < in_NumRules; ++i )
	{
		AkMusicTransitionRule l_Rule;

		l_Rule.srcID	= in_pRules[i].srcID;
		l_Rule.destID	= in_pRules[i].destID;

		l_Rule.srcRule.bPlayPostExit =				in_pRules[i].bSrcPlayPostExit;
		l_Rule.srcRule.eSyncType =					in_pRules[i].eSrcSyncType;
		l_Rule.srcRule.fadeParams.transitionTime =	in_pRules[i].srcFade.transitionTime;
		l_Rule.srcRule.fadeParams.eFadeCurve =		in_pRules[i].srcFade.eFadeCurve;
		l_Rule.srcRule.fadeParams.iFadeOffset =		CAkTimeConv::MillisecondsToSamples( in_pRules[i].srcFade.iFadeOffset );
		
		l_Rule.destRule.bPlayPreEntry =				in_pRules[i].bDestPlayPreEntry;
		l_Rule.destRule.eEntryType =				in_pRules[i].eDestEntryType;
		l_Rule.destRule.fadeParams.transitionTime =	in_pRules[i].destFade.transitionTime;
		l_Rule.destRule.fadeParams.eFadeCurve =		in_pRules[i].destFade.eFadeCurve;
        l_Rule.destRule.fadeParams.iFadeOffset =	CAkTimeConv::MillisecondsToSamples( in_pRules[i].destFade.iFadeOffset );
		l_Rule.destRule.markerID =					in_pRules[i].uDestmarkerID;
		l_Rule.destRule.uJumpToID =					in_pRules[i].uDestJumpToID;

		if( in_pRules[i].bIsTransObjectEnabled )
		{
			l_Rule.pTransObj = (AkMusicTransitionObject*)AkAlloc( g_DefaultPoolId, sizeof(AkMusicTransitionObject) );
			if( !l_Rule.pTransObj )
			{
				eResult = AK_Fail;
				break;
			}
			l_Rule.pTransObj->bPlayPostExit = in_pRules[i].bPlayPostExit;
			l_Rule.pTransObj->bPlayPreEntry = in_pRules[i].bPlayPreEntry;
			l_Rule.pTransObj->fadeInParams.transitionTime = in_pRules[i].transFadeIn.transitionTime;
			l_Rule.pTransObj->fadeInParams.eFadeCurve = 	in_pRules[i].transFadeIn.eFadeCurve;
            l_Rule.pTransObj->fadeInParams.iFadeOffset =    CAkTimeConv::MillisecondsToSamples( in_pRules[i].transFadeIn.iFadeOffset );
			l_Rule.pTransObj->fadeOutParams.transitionTime =in_pRules[i].transFadeOut.transitionTime;
			l_Rule.pTransObj->fadeOutParams.eFadeCurve = 	in_pRules[i].transFadeOut.eFadeCurve;
			l_Rule.pTransObj->fadeOutParams.iFadeOffset =   CAkTimeConv::MillisecondsToSamples( in_pRules[i].transFadeOut.iFadeOffset );
			l_Rule.pTransObj->segmentID = in_pRules[i].segmentID;
		}
		else
		{
			l_Rule.pTransObj = NULL;
		}

		if( !m_arTrRules.AddLast( l_Rule ) )
		{
			eResult = AK_Fail;
			break;
		}
	}

	return eResult;
}

void CAkMusicTransAware::FlushTransitionRules()
{
	for( AkUInt32 i = 0; i < m_arTrRules.Length( ); ++i )
	{
		if( m_arTrRules[i].pTransObj )
		{
			AkFree( g_DefaultPoolId, m_arTrRules[i].pTransObj );
		}
	}
	m_arTrRules.RemoveAll();
}

AKRESULT CAkMusicTransAware::PrepareMusicalDependencies()
{
	AKRESULT eResult = CAkMusicNode::PrepareMusicalDependencies();
	if( eResult == AK_Success )
	{
		for( AkUInt32 i = 0; i < m_arTrRules.Length( ); ++i )
		{
			AkMusicTransitionObject* pTransObject = m_arTrRules[i].pTransObj;
			if( pTransObject )
			{
				eResult = PrepareNodeData( pTransObject->segmentID );
			}
			if( eResult != AK_Success )
			{
				for( AkUInt32 flush = 0; flush < i; ++flush )
				{
					pTransObject = m_arTrRules[flush].pTransObj;
					if( pTransObject )
					{
						UnPrepareNodeData( pTransObject->segmentID );
					}
				}
				break;
			}
		}

		if( eResult != AK_Success )
		{
			CAkMusicNode::UnPrepareMusicalDependencies();
		}
	}


	return eResult;
}

void CAkMusicTransAware::UnPrepareMusicalDependencies()
{
	for( AkUInt32 i = 0; i < m_arTrRules.Length( ); ++i )
	{
		AkMusicTransitionObject* pTransObject = m_arTrRules[i].pTransObj;
		if( pTransObject )
		{
			UnPrepareNodeData( pTransObject->segmentID );
		}
	}

	CAkMusicNode::UnPrepareMusicalDependencies();
}
