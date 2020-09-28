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

#include "AkDialogueEvent.h"

#include <AK/Tools/Common/AkAssert.h>
#include "AkAudioLibIndex.h"
#include "AkBankFloatConversion.h"
#include "AkMonitor.h"
#include "AkAudiolib.h"

extern AkMemPoolId g_DefaultPoolId;

CAkDialogueEvent* CAkDialogueEvent::Create( AkUniqueID in_ulID )
{
	CAkDialogueEvent* pObj = AkNew( g_DefaultPoolId, CAkDialogueEvent( in_ulID ) );

	if( pObj && pObj->Init() != AK_Success )
	{
		pObj->Release();
		pObj = NULL;
	}
	
	return pObj;
}

AkUInt32 CAkDialogueEvent::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxDialogueEvents.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkDialogueEvent::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxDialogueEvents.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
		AKASSERT(g_pIndex);
		g_pIndex->m_idxDialogueEvents.RemoveID( ID() );
		AkDelete( g_DefaultPoolId, this ); 
	} 
    return lRef; 
}

AKRESULT CAkDialogueEvent::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = AK_Success;

	// Skip DynamicSequenceItem ID
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	AkUInt32 uTreeDepth = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	// Read arguments

	AkUInt32 uArgumentsSize = uTreeDepth * sizeof( AkUniqueID );
	if( uArgumentsSize )
	{
		m_pArguments = (AkUniqueID *) AkAlloc( g_DefaultPoolId, uArgumentsSize );
		if ( !m_pArguments )
			return AK_InsufficientMemory;
		memcpy( m_pArguments, in_pData, uArgumentsSize );
	}
	else
	{
		m_pArguments = NULL;
	}

	
	SKIPBANKBYTES( uArgumentsSize, in_pData, in_ulDataSize );

	// Read decision tree

	AkUInt32 uTreeDataSize = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	eResult = m_decisionTree.Init( uTreeDepth, in_pData, uTreeDataSize );
	if ( eResult != AK_Success )
		return eResult;

	SKIPBANKBYTES( uTreeDataSize, in_pData, in_ulDataSize );

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkDialogueEvent::ResolveArgumentValueNames( AkLpCtstr * in_aNames, AkArgumentValueID * out_pPath, AkUInt32 in_cPath )
{
	if ( in_cPath != m_decisionTree.Depth() )
		return AK_Fail;

	for ( AkUInt32 i = 0; i < in_cPath; ++ i ) 
	{
		AkArgumentValueID valueID( AK_FALLBACK_ARGUMENTVALUE_ID );

		if ( in_aNames[ i ][ 0 ] != '\0' ) // Empty string == default argument value
		{
			valueID = AK::SoundEngine::GetIDFromString( in_aNames[ i ] );
			
			if ( valueID == AK_INVALID_UNIQUE_ID  )
			{
				valueID = AK_FALLBACK_ARGUMENTVALUE_ID;
				MONITOR_ERRORMSG2( L"Unknown Argument Value name: ", in_aNames[ i ] );
			}
		}

		out_pPath[ i ] = valueID;
	}

	return AK_Success;
}

CAkDialogueEvent::CAkDialogueEvent( AkUniqueID in_ulID )
	: CAkIndexable( in_ulID )
	, m_pArguments( NULL )
{
}

CAkDialogueEvent::~CAkDialogueEvent()
{
	if ( m_pArguments )
		AkFree( g_DefaultPoolId, m_pArguments );
}

AKRESULT CAkDialogueEvent::Init()
{
	AKASSERT( g_pIndex );
	g_pIndex->m_idxDialogueEvents.SetIDToPtr( this );

	return AK_Success;
}
