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

#include "stdafx.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include "AkDecisionTree.h"

extern AkMemPoolId g_DefaultPoolId;

AkDecisionTree::AkDecisionTree()
	: m_uDepth( 0 )
	, m_pNodes( NULL )
{
	AKASSERT( sizeof( Node ) == 8 ); // make sure we catch platforms where this ain't so. persistence depends on it.
}

AkDecisionTree::~AkDecisionTree()
{
	if ( m_pNodes )
		AkFree( g_DefaultPoolId, m_pNodes );
}

AKRESULT AkDecisionTree::Init( AkUInt32 in_uDepth, void * in_pData, AkUInt32 in_uDataSize )
{
	AKASSERT( m_pNodes == NULL );
	
	if( in_uDataSize )
	{
		m_pNodes = (AkDecisionTree::Node *) AkAlloc( g_DefaultPoolId, in_uDataSize );
		if ( !m_pNodes )
			return AK_InsufficientMemory;

		memcpy( m_pNodes, in_pData, in_uDataSize );
	}
	m_uDepth = in_uDepth;

	return AK_Success;
}

AkUniqueID AkDecisionTree::ResolvePath( AkArgumentValueID * in_pPath, AkUInt32 in_cPath )
{
	AKASSERT( in_pPath );

	if ( in_cPath != Depth() )
		return AK_INVALID_UNIQUE_ID;

	bool bFound = false;
	return _ResolvePath( m_pNodes, in_pPath, in_cPath, bFound );
}

AkUniqueID AkDecisionTree::_ResolvePath( Node * in_pRootNode, AkArgumentValueID * in_pPath, AkUInt32 in_cPath, bool & io_bFound )
{
	if( m_pNodes )
	{
		Node * pChildrenNodes = m_pNodes + in_pRootNode->children.uIdx;
		Node * pNode = BinarySearch( pChildrenNodes, in_pRootNode->children.uCount, *in_pPath );

		// handle direct match

		if ( pNode ) 
		{
			if ( in_cPath == 1 )
			{
				io_bFound = true;
				return pNode->audioNodeID;
			}

			AkUniqueID id = _ResolvePath( pNode, in_pPath + 1, in_cPath - 1, io_bFound );
			if ( io_bFound )
				return id;
		}

		// if no direct match, search for *
		// guaranteed to be at index 0 if present

		if ( pChildrenNodes[ 0 ].key == AK_FALLBACK_ARGUMENTVALUE_ID
			&& *in_pPath != AK_FALLBACK_ARGUMENTVALUE_ID )
		{
			if ( in_cPath == 1 )
			{
				io_bFound = true;
				return pChildrenNodes[ 0 ].audioNodeID;
			}

			AkUniqueID id = _ResolvePath( pChildrenNodes, in_pPath + 1, in_cPath - 1, io_bFound );
			if ( io_bFound )
				return id;
		}
	}

	return AK_INVALID_UNIQUE_ID;
}

AkDecisionTree::Node * AkDecisionTree::BinarySearch( Node * in_pNodes, AkUInt32 in_cNodes, AkArgumentValueID in_key )
{
	AKASSERT( in_pNodes );

	AkInt32 uTop = 0, uBottom = in_cNodes-1;

	// binary search for key
	do
	{
		AkInt32 uThis = ( uBottom - uTop ) / 2 + uTop; 
		if ( in_pNodes[ uThis ].key > in_key ) 
			uBottom = uThis - 1;
		else if ( in_pNodes[ uThis ].key < in_key ) 
			uTop = uThis + 1;
		else
			return in_pNodes + uThis;
	}
	while ( uTop <= uBottom );

	return NULL;
}
