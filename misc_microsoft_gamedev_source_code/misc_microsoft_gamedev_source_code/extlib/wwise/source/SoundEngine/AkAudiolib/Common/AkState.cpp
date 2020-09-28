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
// AkState.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkState.h"
#include "AkAudioLibIndex.h"
#include "AkBankFloatConversion.h"
#include "AkParameterNodeBase.h"
#include "AkStateMgr.h"

extern AkMemPoolId g_DefaultPoolId;
extern CAkStateMgr* g_pStateMgr;

CAkState::CAkState( AkUniqueID in_ulID, bool in_bIsCustomState, AkStateGroupID in_StateGroupID )
:CAkIndexable(in_ulID)
,m_pParentForNotify(NULL)
,m_bNotifyStateMgr( false )
,m_bIsCustomState( in_bIsCustomState )
,m_StateGroupID( in_StateGroupID )
{
}

CAkState::~CAkState()
{
}

CAkState* CAkState::Create( AkUniqueID in_ulID, bool in_bIsCustomState, AkStateGroupID in_StateGroupID )
{
	CAkState* pState = AkNew(g_DefaultPoolId,CAkState(in_ulID, in_bIsCustomState, in_StateGroupID));
	if( pState )
	{
		if( pState->Init() != AK_Success )
		{
			pState->Release();
			pState = NULL;
		}
	}
	return pState;
}


AKRESULT CAkState::SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA( AkUInt32, in_pData, in_ulDataSize );


	m_Parameters.Volume = READBANKDATA( AkReal32, in_pData, in_ulDataSize );

	m_Parameters.LFEVolume = READBANKDATA( AkReal32, in_pData, in_ulDataSize );

	m_Parameters.Pitch = READBANKDATA( AkInt32, in_pData, in_ulDataSize );

	m_Parameters.LPF = READBANKDATA( AkLPFType, in_pData, in_ulDataSize );

	m_Parameters.eVolumeValueMeaning = (AkValueMeaning)READBANKDATA( AkUInt8, in_pData, in_ulDataSize );

	m_Parameters.eLFEValueMeaning = (AkValueMeaning)READBANKDATA( AkUInt8, in_pData, in_ulDataSize );

	m_Parameters.ePitchValueMeaning = (AkValueMeaning)READBANKDATA( AkUInt8, in_pData, in_ulDataSize );

	m_Parameters.eLPFValueMeaning = (AkValueMeaning)READBANKDATA( AkUInt8, in_pData, in_ulDataSize );

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkState::AddToIndex()
{
	if( m_bIsCustomState )
	{
		g_pIndex->m_idxCustomStates.SetIDToPtr( this );
		return AK_Success;
	}
	else
	{
		return g_pIndex->m_idxStates.SetIDToPtr( m_StateGroupID, this );
	}
		
}

void CAkState::RemoveFromIndex()
{
	if( m_bIsCustomState )
		g_pIndex->m_idxCustomStates.RemoveID( ID() );
	else
		g_pIndex->m_idxStates.RemoveID( m_StateGroupID, ID() );
}

AkUInt32 CAkState::AddRef() 
{ 
	if( m_bIsCustomState )
	{
		AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxCustomStates.GetLock() ); 
		return ++m_lRef;
	}
	else
	{
		AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxStates.GetLock() ); 
		return ++m_lRef;
	} 
} 

AkUInt32 CAkState::Release() 
{
	if( m_bIsCustomState )
	{
		AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxCustomStates.GetLock() ); 
		AkInt32 lRef = --m_lRef; 
		AKASSERT( lRef >= 0 ); 
		if ( !lRef ) 
		{ 
			RemoveFromIndex(); 
			AkDelete( g_DefaultPoolId, this ); 
		} 
		return lRef; 
	}
	else
	{
		AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxStates.GetLock() ); 
		AkInt32 lRef = --m_lRef; 
		AKASSERT( lRef >= 0 ); 
		if ( !lRef ) 
		{ 
			RemoveFromIndex(); 
			AkDelete( g_DefaultPoolId, this ); 
		} 
		return lRef; 
	}
}

void CAkState::InitNotificationSystem( CAkParameterNodeBase * in_pNode )
{
	m_pParentForNotify = in_pNode;
	m_bNotifyStateMgr = ( in_pNode == NULL );
}

void CAkState::TermNotificationSystem()
{
	m_pParentForNotify = NULL;
	m_bNotifyStateMgr = false;
}

void CAkState::NotifyParent()
{
	if( m_pParentForNotify )
		m_pParentForNotify->NotifyStateParametersModified();
	else if ( m_bNotifyStateMgr )
		g_pStateMgr->NotifyStateModified(ID());
}

void CAkState::Volume(AkVolumeValue in_Volume)
{
	m_Parameters.Volume = in_Volume;
	NotifyParent();
}

AkVolumeValue CAkState::Volume()
{
	return m_Parameters.Volume;
}

void CAkState::VolumeMeaning(AkValueMeaning in_eVolumeMeaning)
{
	m_Parameters.eVolumeValueMeaning = in_eVolumeMeaning;
	NotifyParent();
}

AkValueMeaning CAkState::VolumeMeaning()
{
	return m_Parameters.eVolumeValueMeaning;
}

void CAkState::Pitch(AkPitchValue in_Pitch)
{
	m_Parameters.Pitch = in_Pitch;
	NotifyParent();
}

AkPitchValue CAkState::Pitch()
{
	return m_Parameters.Pitch;
}

void CAkState::PitchMeaning(AkValueMeaning in_ePitchMeaning)
{
	m_Parameters.ePitchValueMeaning = in_ePitchMeaning;
	NotifyParent();
}

AkValueMeaning CAkState::PitchMeaning()
{
	return m_Parameters.ePitchValueMeaning;
}

void CAkState::LPF(AkLPFType in_LPF)
{
	m_Parameters.LPF = in_LPF;
	NotifyParent();
}

AkLPFType CAkState::LPF()
{
	return m_Parameters.LPF;
}

void CAkState::LPFMeaning(AkValueMeaning in_eLPFMeaning)
{
	m_Parameters.eLPFValueMeaning = in_eLPFMeaning;
	NotifyParent();
}

AkValueMeaning CAkState::LPFMeaning()
{
	return m_Parameters.eLPFValueMeaning;
}

void CAkState::LFEVolume(AkVolumeValue in_LFE)
{
	m_Parameters.LFEVolume = in_LFE;
	NotifyParent();
}

AkVolumeValue CAkState::LFEVolume()
{
	return m_Parameters.LFEVolume;
}

void CAkState::LFEVolumeMeaning(AkValueMeaning in_eLFEMeaning)
{
	m_Parameters.eLFEValueMeaning = in_eLFEMeaning;
	NotifyParent();
}

AkValueMeaning CAkState::LFEVolumeMeaning()
{
	return m_Parameters.eLFEValueMeaning;
}
