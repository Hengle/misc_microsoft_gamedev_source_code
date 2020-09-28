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
// AkRanSeqCntr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkBankFloatConversion.h"
#include "AkRanSeqCntr.h"
#include "AkAudioLibIndex.h"
#include "AkRandom.h"
#include "AkContinuationList.h"
#include "PrivateStructures.h"
#include "AkPlayingMgr.h"
#include "AkMonitor.h"
#include "AkAudioMgr.h"
#include "AkCntrHistory.h"
#include "AkModifiers.h"
#include "AkPlayList.h"
#include "AkContinuousPBI.h"

#define AK_MIN_NUM_PER_OBJ_CNTR_INFO 8

// PhM : __w64 is compiler dependent + use the AkTypes
#if defined(WIN32) || defined(XBOX360)
inline AkUInt32 AkHash( CAkRegisteredObj *  in_obj ) { return (AkUInt32) (__w64 AkUInt32) in_obj; }
#else
inline AkUInt32 AkHash( CAkRegisteredObj *  in_obj ) { return (AkUInt32) in_obj; }
#endif

extern AkMemPoolId g_DefaultPoolId;

CAkRanSeqCntr::CAkRanSeqCntr(AkUniqueID in_ulID, AkContainerMode in_ContainerMode /*= ContainerMode_Sequence*/)
:CAkContainerBase( in_ulID )
,m_pGlobalContainerInfo( NULL )
,m_pPlayList( NULL )
,m_bContainerBeenPlayed( false )
,m_Loop( AkLoopVal_NotLooping )
{
	m_RanSeqParams.m_bIsContinuous = false;
	m_RanSeqParams.m_bIsGlobal = true;
	m_RanSeqParams.m_bIsRestartBackward = false;
	m_RanSeqParams.m_bIsUsingWeight = false;
	m_RanSeqParams.m_bResetPlayListAtEachPlay = true;
	m_RanSeqParams.m_eMode = in_ContainerMode;
	m_RanSeqParams.m_eRandomMode = RandomMode_Normal;
	m_RanSeqParams.m_eTransitionMode = Transition_Disabled;
	m_RanSeqParams.m_wAvoidRepeatCount = 0;
}

AKRESULT CAkRanSeqCntr::SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	//ReadParameterNode
	eResult = SetNodeBaseParams(in_pData, in_ulDataSize, false);

	//Read Looping info
	if(eResult == AK_Success)
	{
		AkUInt16 us1 = READBANKDATA(AkUInt16, in_pData, in_ulDataSize);

		Loop(true, us1 == 0, us1 );

		AkTimeMs	tTransitionTime			= READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );
		AkTimeMs	tTransitionTimeModMin	= READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );
		AkTimeMs	tTransitionTimeModMax	= READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );

		AkUInt16	wAvoidRepeatCount		= READBANKDATA( AkUInt16, in_pData, in_ulDataSize );

		AkUInt8	eTransitionMode				= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
		AkUInt8	eRandomMode					= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
		AkUInt8	eMode						= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
		AkUInt8	bIsUsingWeight				= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
		AkUInt8	bResetPlayListAtEachPlay	= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
		AkUInt8	bIsRestartBackward			= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
		AkUInt8	bIsContinuous				= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
		AkUInt8	bIsGlobal					= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );

		eResult = Mode( (AkContainerMode)eMode ); //Mode must be set first
		if(eResult == AK_Success)
		{
			TransitionTime( tTransitionTime, tTransitionTimeModMin, tTransitionTimeModMax );

			AvoidRepeatingCount	( wAvoidRepeatCount );
			TransitionMode		( (AkTransitionMode)eTransitionMode );
			RandomMode			( (AkRandomMode)eRandomMode );
			//bIsUsingWeight is useless for now... will be recalculated by the playlist
			ResetPlayListAtEachPlay( bResetPlayListAtEachPlay != 0 );
			RestartBackward		( bIsRestartBackward != 0 );
			Continuous			( bIsContinuous != 0 );
			IsGlobal			( bIsGlobal != 0 );
		}
	}

	if(eResult == AK_Success)
	{
		eResult = SetChildren( in_pData, in_ulDataSize );
	}

	if(eResult == AK_Success)
	{
		//Process Play list
		AkUInt32 ulPlayListItem = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		for(AkUInt32 i = 0; i < ulPlayListItem; ++i)
		{
			AkUInt32 ulPlayID = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			AkUInt8	ucWeight = READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
			eResult = AddPlaylistItem(ulPlayID, ucWeight);
			if( eResult == AK_IDNotFound )
			{
				// It is not an error if something is in a playlist but not in a childlist.
				// It may occurs for example if a Parent container is packaged only because there is a play directly on one of its children
				eResult = AK_Success;
			}
			if(eResult != AK_Success)
			{
				break;
			}
		}
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

CAkRanSeqCntr::~CAkRanSeqCntr()
{
	Term();
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkRanSeqCntr::Init()
{
	AKRESULT eResult = CAkContainerBase::Init();

	if( eResult == AK_Success )
	{
		eResult = m_mapObjectCntrInfo.Init();
		if( eResult == AK_Success )
		{
			if(m_RanSeqParams.m_eMode == ContainerMode_Sequence)
			{
				m_pPlayList = AkNew(g_DefaultPoolId,CAkPlayListSequence());
			}
			else
			{
				m_pPlayList = AkNew(g_DefaultPoolId,CAkPlayListRandom());
			}
			if( m_pPlayList != NULL )
			{
				eResult = m_pPlayList->Init();
			}
			else
			{
				eResult = AK_Fail;
			}
		}
	}
    
	return eResult;
}
//====================================================================================================
//====================================================================================================
void CAkRanSeqCntr::Term()
{
	if( m_pPlayList )
	{
		ClearPlaylist();
		m_pPlayList->Destroy();
	}
	m_mapObjectCntrInfo.Term();
}
//====================================================================================================
//====================================================================================================

CAkRanSeqCntr* CAkRanSeqCntr::Create(AkUniqueID in_ulID, AkContainerMode in_ContainerMode /*= ContainerMode_Sequence*/)
{
	CAkRanSeqCntr* pAkRanSeqCntr = AkNew( g_DefaultPoolId, CAkRanSeqCntr( in_ulID, in_ContainerMode ) );
	if( pAkRanSeqCntr )
	{
		if( pAkRanSeqCntr->Init() != AK_Success )
		{
			pAkRanSeqCntr->Release();
			pAkRanSeqCntr = NULL;
		}
	}
	return pAkRanSeqCntr;
}

AkNodeCategory CAkRanSeqCntr::NodeCategory()
{
	return AkNodeCategory_RanSeqCntr;
}

// Notify the children that the associated object was unregistered
void CAkRanSeqCntr::Unregister(
	CAkRegisteredObj * in_pGameObj //Game object associated to the action
	)
{
	CAkContainerBase::Unregister( in_pGameObj );

	AkMapObjectCntrInfo::IteratorEx it = m_mapObjectCntrInfo.FindEx( in_pGameObj );
	if ( it != m_mapObjectCntrInfo.End() )
	{
		// careful: order is important in bare container
		CAkContainerBaseInfo * pItem = *it;
		m_mapObjectCntrInfo.Erase( it );
		pItem->Destroy();
	}
}

CAkAudioNode* CAkRanSeqCntr::GetNextToPlay(CAkRegisteredObj * in_pGameObj, AkUInt16& out_rwPositionSelected)
{
	AKASSERT(g_pIndex);
	CAkAudioNode* pNode = NULL; 
	out_rwPositionSelected = 0;
	if(m_pPlayList->Length() != 0)
	{
		if(m_pPlayList->Length() != 1)
		{
			if(m_RanSeqParams.m_eMode == ContainerMode_Sequence)
			{
				CAkSequenceInfo* pSeqInfo = NULL;
				if(m_RanSeqParams.m_bIsGlobal)
				{
					if(!m_pGlobalContainerInfo)
					{
						m_pGlobalContainerInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
						AKASSERT(m_pGlobalContainerInfo != NULL);
						if( !m_pGlobalContainerInfo )
							return NULL;
					}
					pSeqInfo = static_cast<CAkSequenceInfo*>(m_pGlobalContainerInfo);
				}
				else//PerObject
				{
					CAkContainerBaseInfo* pCntrBaseInfo = m_mapObjectCntrInfo.Exists( in_pGameObj );
					if( !pCntrBaseInfo )
					{
						pSeqInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
						AKASSERT( pSeqInfo );
						if( !pSeqInfo)
							return NULL;

						pSeqInfo->key = in_pGameObj;
						m_mapObjectCntrInfo.Set( pSeqInfo );
						in_pGameObj->SetNodeAsModified( this );
					}
					else
					{
						pSeqInfo = static_cast<CAkSequenceInfo*>( pCntrBaseInfo );
					}
				}
				AKASSERT(pSeqInfo);
				bool bIsValid = true;
				AkUInt16 wPos = SelectSequentially(pSeqInfo,bIsValid);
				if(bIsValid)
				{
					out_rwPositionSelected = wPos;
					pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_pPlayList->ID(wPos));
				}
			}
			else //Else is random
			{
				CAkRandomInfo* pRandomInfo = NULL;
				if(m_RanSeqParams.m_bIsGlobal)
				{
					if(!m_pGlobalContainerInfo)
					{
						m_pGlobalContainerInfo = AkNew( g_DefaultPoolId, CAkRandomInfo( (AkUInt16)( m_pPlayList->Length() ) ) );
						if( !m_pGlobalContainerInfo )
						{
							return NULL;
						}

						pRandomInfo = static_cast<CAkRandomInfo*>(m_pGlobalContainerInfo);
						AKASSERT( pRandomInfo );
						if( pRandomInfo->Init() != AK_Success )
						{
							m_pGlobalContainerInfo->Destroy();
							m_pGlobalContainerInfo = NULL;
							return NULL;
						}
						
						if(m_RanSeqParams.m_bIsUsingWeight)
						{
							pRandomInfo->m_ulTotalWeight = pRandomInfo->m_ulRemainingWeight = CalculateTotalWeight();
						}
					}
					else
					{
						pRandomInfo = static_cast<CAkRandomInfo*>(m_pGlobalContainerInfo);
					}	
				}
				else//PerObject
				{
					CAkContainerBaseInfo* pCntrBaseInfo = m_mapObjectCntrInfo.Exists( in_pGameObj );
					if( !pCntrBaseInfo )
					{
						pRandomInfo = AkNew( g_DefaultPoolId, CAkRandomInfo( (AkUInt16)(m_pPlayList->Length()) ) );
						AKASSERT( pRandomInfo );
						if( !pRandomInfo )
							return NULL;

						if( pRandomInfo->Init() != AK_Success )
						{
							pRandomInfo->Destroy();
							return NULL;
						}

						pRandomInfo->key = in_pGameObj;
						m_mapObjectCntrInfo.Set( pRandomInfo );
						in_pGameObj->SetNodeAsModified( this );

						if(m_RanSeqParams.m_bIsUsingWeight)
						{
							pRandomInfo->m_ulTotalWeight = pRandomInfo->m_ulRemainingWeight = CalculateTotalWeight();
						}
					}
					else
					{
						pRandomInfo = static_cast<CAkRandomInfo*>( pCntrBaseInfo );
					}
				}
				AKASSERT(pRandomInfo);
				bool bIsValid = true;
				AkUInt16 wPos = SelectRandomly(pRandomInfo,bIsValid);
				if(bIsValid)
				{
					out_rwPositionSelected = wPos;
					pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_pPlayList->ID(wPos));
				}
			}
		}
		else
		{
			pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_pPlayList->ID(0) );
		}
	}
	return pNode;
}

CAkAudioNode* CAkRanSeqCntr::GetNextToPlayContinuous(CAkRegisteredObj * in_pGameObj, AkUInt16& out_rwPositionSelected, CAkContainerBaseInfoPtr& io_pContainerInfo, AkLoop& io_rLoopInfo)
{
	AKASSERT(g_pIndex);
	CAkAudioNode* pNode = NULL; 
	if(m_pPlayList->Length() != 0)
	{
		if(m_pPlayList->Length() != 1)
		{
			bool bIsValid = true;
			AkUInt16 wPos = 0;
			if(!m_RanSeqParams.m_bResetPlayListAtEachPlay && !io_pContainerInfo && m_RanSeqParams.m_eMode == ContainerMode_Sequence)
			{
				CAkSequenceInfo* pSeqInfo = NULL;
				if(!io_pContainerInfo)
				{
					CAkSequenceInfo* pSeqInfoReference = NULL;
					if(IsGlobal())
					{
						if(!m_pGlobalContainerInfo)
						{
							m_pGlobalContainerInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
							AKASSERT(m_pGlobalContainerInfo != NULL);
							if( !m_pGlobalContainerInfo )
								return NULL;
						}
						pSeqInfoReference = static_cast<CAkSequenceInfo*>(m_pGlobalContainerInfo);
					}
					else
					{
						CAkContainerBaseInfo* pCntrBaseInfo = m_mapObjectCntrInfo.Exists( in_pGameObj );
						if( !pCntrBaseInfo )
						{
							
							pSeqInfoReference = AkNew( g_DefaultPoolId,CAkSequenceInfo() );
							AKASSERT( pSeqInfoReference );
							if( !pSeqInfoReference )
								return NULL;

							pSeqInfoReference->key = in_pGameObj;
							m_mapObjectCntrInfo.Set( pSeqInfoReference );
							in_pGameObj->SetNodeAsModified( this );
						}
						else
						{
							pSeqInfoReference = static_cast<CAkSequenceInfo*>( pCntrBaseInfo );
						}
					}
					pSeqInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
					AKASSERT(pSeqInfo != NULL);
					if( !pSeqInfo )
						return NULL;

					*pSeqInfo = *pSeqInfoReference;
				}
				else
				{
					pSeqInfo = static_cast<CAkSequenceInfo*>( io_pContainerInfo );
				}	

				wPos = SelectSequentially(pSeqInfo,bIsValid,&io_rLoopInfo);
				io_pContainerInfo = pSeqInfo;
			}
			else //m_bResetPlayListAtEachPlay == TRUE (normal behavior)
			{
				if( m_RanSeqParams.m_eMode == ContainerMode_Sequence )
				{
					CAkSequenceInfo* pSeqInfo = NULL;
					if(!io_pContainerInfo)
					{
						io_pContainerInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
						AKASSERT(io_pContainerInfo != NULL);
						if( !io_pContainerInfo)
							return NULL;

						if(m_pGlobalContainerInfo)
						{
							CAkSequenceInfo* pSeqInfoGlobalLocal = static_cast<CAkSequenceInfo*>(m_pGlobalContainerInfo);
							//Support the ForceNextToPlay behavior on non-playing Sequence sound
							static_cast<CAkSequenceInfo*>(io_pContainerInfo)->m_i16LastPositionChosen = pSeqInfoGlobalLocal->m_i16LastPositionChosen;
							pSeqInfoGlobalLocal->m_i16LastPositionChosen = -1;
						}
					}
					pSeqInfo = static_cast<CAkSequenceInfo*>(io_pContainerInfo);
					AKASSERT(pSeqInfo);

					wPos = SelectSequentially(pSeqInfo,bIsValid,&io_rLoopInfo);
					if(!m_RanSeqParams.m_bResetPlayListAtEachPlay)
					{
						UpdateResetPlayListSetup(pSeqInfo, in_pGameObj);
					}
				}
				else //Else is random
				{
					CAkRandomInfo* pRandomInfo = NULL;
					if(!io_pContainerInfo)
					{
						pRandomInfo = AkNew( g_DefaultPoolId, CAkRandomInfo( (AkUInt16)( m_pPlayList->Length() ) ) );
						
						if( !pRandomInfo )
							return NULL;

						if( pRandomInfo->Init() != AK_Success )
						{
							pRandomInfo->Destroy();
							return NULL;
						}

						io_pContainerInfo = pRandomInfo;

						if(m_RanSeqParams.m_bIsUsingWeight)
						{
							pRandomInfo->m_ulTotalWeight = pRandomInfo->m_ulRemainingWeight = CalculateTotalWeight();
						}
					}
					else
					{
						pRandomInfo = static_cast<CAkRandomInfo*>(io_pContainerInfo);
					}	
					AKASSERT(pRandomInfo);

					wPos = SelectRandomly(pRandomInfo,bIsValid,&io_rLoopInfo);
				}
			}
			if(bIsValid)
			{
				out_rwPositionSelected = wPos;
				pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_pPlayList->ID(wPos));
			}
		}
		else
		{
			if(io_rLoopInfo.lLoopCount > 0)
			{
				if(!io_rLoopInfo.bIsInfinite)
				{
					--(io_rLoopInfo.lLoopCount);
				}
				pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_pPlayList->ID(0));
			}
		}
	}
	return pNode;
}

AKRESULT CAkRanSeqCntr::AddPlaylistItem(AkUniqueID in_ElementID, AkUInt8 in_cWeight /*=DEFAULT_RANDOM_WEIGHT*/)
{
	AKRESULT eResult = AK_Success;

	if(in_cWeight!=DEFAULT_RANDOM_WEIGHT)
	{
		m_RanSeqParams.m_bIsUsingWeight = true;
	}

	if(m_RanSeqParams.m_eMode == ContainerMode_Sequence || !m_pPlayList->Exists(in_ElementID))
	{
		eResult = m_pPlayList->Add( in_ElementID, in_cWeight );
	}
	else
	{
		eResult = AK_ElementAlreadyInList;
	}

	if(eResult == AK_Success)
	{
		ResetSpecificInfo();
	}
	return eResult;
}

AKRESULT CAkRanSeqCntr::RemovePlaylistItem(AkUniqueID in_ElementID)
{
	m_pPlayList->Remove(in_ElementID);
	ResetSpecificInfo();
	return AK_Success;
}

AKRESULT CAkRanSeqCntr::ClearPlaylist()
{
	if(m_pPlayList->Length())
	{
		m_pPlayList->RemoveAll();
		m_RanSeqParams.m_bIsUsingWeight = false;
		ResetSpecificInfo();
	}
	return AK_Success;
}

//---------------  Not to be exported to thd SDK  ---------------
void CAkRanSeqCntr::_SetItemWeight(AkUniqueID in_ID, AkUInt8 in_cWeight)
{
	AkUInt16 wPosition = 0;
	if(m_RanSeqParams.m_eMode == ContainerMode_Random && m_pPlayList->GetPosition(in_ID, wPosition))
	{
		SetItemWeight(wPosition,in_cWeight);
	}
	else
	{
		////////////////////////////////////////////////////////////////////////////////
		// You either tried to call this WAL reserved function on a Non-Random container 
		// or specified an ID that was not found in the Playlist.
		//AKASSERT(!"Invalid Try to Set the weight of a container playlist element");
		////////////////////////////////////////////////////////////////////////////////
	}
}

void CAkRanSeqCntr::SetItemWeight(AkUInt16 in_wPosition, AkUInt8 in_cWeight)
{
	AKASSERT(m_RanSeqParams.m_eMode == ContainerMode_Random);
	CAkPlayListRandom* pRandomPlaylist = static_cast<CAkPlayListRandom *>(m_pPlayList);
	AKASSERT(pRandomPlaylist);
	AKASSERT(in_cWeight && "Weight == 0 means unpredictable behavior, ignored");
	if(in_cWeight)
	{
		pRandomPlaylist->SetWeight(in_wPosition,in_cWeight);
		m_RanSeqParams.m_bIsUsingWeight = true;
		ResetSpecificInfo();
	}
}

bool CAkRanSeqCntr::RestartBackward()
{
	return m_RanSeqParams.m_bIsRestartBackward;
}

void CAkRanSeqCntr::RestartBackward(const bool in_bRestartBackward)
{
	m_RanSeqParams.m_bIsRestartBackward = in_bRestartBackward;
}

bool CAkRanSeqCntr::Continuous()
{
	return m_RanSeqParams.m_bIsContinuous;
}

void CAkRanSeqCntr::Continuous(const bool in_bIsContinuous)
{
	m_RanSeqParams.m_bIsContinuous = in_bIsContinuous;
	ResetSpecificInfo();
}

bool CAkRanSeqCntr::IsGlobal()
{
	return m_RanSeqParams.m_bIsGlobal;
}

void CAkRanSeqCntr::IsGlobal(bool in_bIsGlobal)
{
	m_RanSeqParams.m_bIsGlobal = in_bIsGlobal;
	ResetSpecificInfo();
}

bool CAkRanSeqCntr::ResetPlayListAtEachPlay()
{
	return m_RanSeqParams.m_bResetPlayListAtEachPlay;
}

void CAkRanSeqCntr::ResetPlayListAtEachPlay(bool in_bResetPlayListAtEachPlay)
{
	m_RanSeqParams.m_bResetPlayListAtEachPlay = in_bResetPlayListAtEachPlay;
	ResetSpecificInfo();
}

AkTransitionMode CAkRanSeqCntr::TransitionMode()
{
	return m_RanSeqParams.m_eTransitionMode;
}

void CAkRanSeqCntr::TransitionMode(AkTransitionMode in_eTransitionMode)
{
	m_RanSeqParams.m_eTransitionMode = in_eTransitionMode;
	ResetSpecificInfo();
}

AkTimeMs CAkRanSeqCntr::TransitionTime()
{
	AkTimeMs transitionTime = RandomizerModifier::GetModValue( m_RanSeqParams.m_TransitionTime );
	return AkMax( 0, transitionTime );
}

void CAkRanSeqCntr::TransitionTime(AkTimeMs in_TransitionTime,AkTimeMs in_RangeMin/*=0*/, AkTimeMs in_RangeMax/*=0*/)
{
	RandomizerModifier::SetModValue( m_RanSeqParams.m_TransitionTime, in_TransitionTime, in_RangeMin, in_RangeMax ) ;
}

AkRandomMode CAkRanSeqCntr::RandomMode()
{
	return m_RanSeqParams.m_eRandomMode;
}

void CAkRanSeqCntr::RandomMode(AkRandomMode in_eRandomMode)
{
	m_RanSeqParams.m_eRandomMode = in_eRandomMode;
	ResetSpecificInfo();
}

AkUInt16 CAkRanSeqCntr::AvoidRepeatingCount()
{
	return m_RanSeqParams.m_wAvoidRepeatCount;
}

void CAkRanSeqCntr::AvoidRepeatingCount(AkUInt16 in_wCount)
{
	m_RanSeqParams.m_wAvoidRepeatCount = in_wCount;
	ResetSpecificInfo();
}

AkContainerMode CAkRanSeqCntr::Mode()
{
	return m_RanSeqParams.m_eMode;
}

AKRESULT CAkRanSeqCntr::Mode( AkContainerMode in_eMode )
{
	AKRESULT eResult = AK_Success;

	if(in_eMode != m_RanSeqParams.m_eMode)
	{
		m_RanSeqParams.m_eMode = in_eMode;
		if( m_pPlayList )
		{
			m_pPlayList->Destroy();
		}
		if(m_RanSeqParams.m_eMode == ContainerMode_Sequence)
		{
			m_pPlayList = AkNew(g_DefaultPoolId,CAkPlayListSequence());
		}
		else
		{
			m_pPlayList = AkNew(g_DefaultPoolId,CAkPlayListRandom());
		}
    
		if( m_pPlayList )
		{
			eResult = m_pPlayList->Init();
			if( eResult != AK_Success )
			{
				m_pPlayList->Destroy();
				m_pPlayList = NULL;
			}
		}
		else
		{
			eResult = AK_Fail;
		}

		ResetSpecificInfo();
	}

	return eResult;
}

void	CAkRanSeqCntr::Loop(
		bool  in_bIsLoopEnabled,
		bool  in_bIsInfinite,
		AkInt16 in_sLoopCount
		)
{
	if( in_bIsLoopEnabled )
	{
		if( in_bIsInfinite )
		{
			m_Loop = AkLoopVal_Infinite;
		}
		else
		{
			m_Loop = in_sLoopCount;
		}
	}
	else
	{
		m_Loop = AkLoopVal_NotLooping;
	}
}

void CAkRanSeqCntr::ResetSpecificInfo()
{
	for ( AkMapObjectCntrInfo::IteratorEx it = m_mapObjectCntrInfo.BeginEx(); it != m_mapObjectCntrInfo.End(); ++it )
	{
		// careful: order is important in bare container
		CAkContainerBaseInfo * pItem = *it;
		m_mapObjectCntrInfo.Erase( it );
		pItem->Destroy();
	}

	if(m_pGlobalContainerInfo)
	{
		m_pGlobalContainerInfo->Destroy();
		m_pGlobalContainerInfo = NULL;
	}
	if(m_bContainerBeenPlayed)
	{
		if( g_pAudioMgr )
		{
			g_pAudioMgr->RemovePausedPendingAction( ID() );
			g_pAudioMgr->RemovePendingAction( ID() );
		}

		Stop();
	}
}

AkUInt32 CAkRanSeqCntr::CalculateTotalWeight()
{
	AKASSERT(m_RanSeqParams.m_eMode == ContainerMode_Random);
	CAkPlayListRandom* pRandomPlaylist = static_cast<CAkPlayListRandom *>(m_pPlayList);
	AKASSERT(pRandomPlaylist);
	return pRandomPlaylist->CalculateTotalWeight();
}

AkUInt16 CAkRanSeqCntr::SelectSequentially(CAkSequenceInfo* in_pSeqInfo, bool& out_bIsAnswerValid, AkLoop* io_pLoopCount)
{
	out_bIsAnswerValid = true;
	if(in_pSeqInfo->m_bIsForward)
	{
		if(in_pSeqInfo->m_i16LastPositionChosen+1 == m_pPlayList->Length())//reached the end of the sequence
		{
			if(m_RanSeqParams.m_bIsRestartBackward)
			{
				--(in_pSeqInfo->m_i16LastPositionChosen);
				in_pSeqInfo->m_bIsForward = false;
			}
			else//Restart to zero
			{
				in_pSeqInfo->m_i16LastPositionChosen = 0;
				if(!CanContinueAfterCompleteLoop(io_pLoopCount))
				{
					out_bIsAnswerValid = false;
					return 0;
				}
			}
		}
		else//not finished sequence
		{
			++(in_pSeqInfo->m_i16LastPositionChosen);
		}
	}
	else//isgoingbackward
	{
		if(in_pSeqInfo->m_i16LastPositionChosen == 0)//reached the end of the sequence ( == 0)
		{
			in_pSeqInfo->m_i16LastPositionChosen = 1;
			in_pSeqInfo->m_bIsForward = true;
			if(!CanContinueAfterCompleteLoop(io_pLoopCount))
			{
				out_bIsAnswerValid = false;
				return 0;
			}
		}
		else//not finished sequence
		{
			--(in_pSeqInfo->m_i16LastPositionChosen);
		}
	}
	return in_pSeqInfo->m_i16LastPositionChosen;
}


AkUInt16 CAkRanSeqCntr::SelectRandomly(CAkRandomInfo* in_pRandomInfo, bool& out_bIsAnswerValid, AkLoop* io_pLoopCount)
{
	AKASSERT(in_pRandomInfo);
	out_bIsAnswerValid = true;
	AkUInt16 wPosition = 0;
	AkInt iValidCount = -1;
	AkInt iCycleCount = 0;

	AKASSERT(m_RanSeqParams.m_eMode == ContainerMode_Random);
	CAkPlayListRandom* pRandomPlaylist = static_cast<CAkPlayListRandom *>(m_pPlayList);

	if(!in_pRandomInfo->m_wCounter)
	{
		if(!CanContinueAfterCompleteLoop(io_pLoopCount))
		{
			out_bIsAnswerValid = false;
			return 0;
		}
		in_pRandomInfo->m_wCounter = (AkUInt16)m_pPlayList->Length();
		in_pRandomInfo->ResetFlagsPlayed(m_pPlayList->Length());

		if(RandomMode() == RandomMode_Shuffle)
		{	
			in_pRandomInfo->m_ulRemainingWeight = in_pRandomInfo->m_ulTotalWeight;
			for( CAkRandomInfo::AkAvoidList::Iterator iter = in_pRandomInfo->m_listAvoid.Begin(); iter != in_pRandomInfo->m_listAvoid.End(); ++iter )
			{
				in_pRandomInfo->m_ulRemainingWeight -= pRandomPlaylist->GetWeight(*iter);
			}
		}
		in_pRandomInfo->m_wRemainingItemsToPlay -= (AkUInt16)in_pRandomInfo->m_listAvoid.Length();
	}

	AKASSERT(in_pRandomInfo->m_wRemainingItemsToPlay);//Should never be here if empty...

	if(m_RanSeqParams.m_bIsUsingWeight)
	{
		AkInt iRandomValue = (AkUInt16)((AKRANDOM::AkRandom() % in_pRandomInfo->m_ulRemainingWeight));
		while(iValidCount < iRandomValue)
		{
			if(CanPlayPosition(in_pRandomInfo, iCycleCount))
			{
				iValidCount += pRandomPlaylist->GetWeight(iCycleCount);
			}
			++iCycleCount;
			AKASSERT(((size_t)(iCycleCount-1)) < m_pPlayList->Length());
		}
	}
	else
	{
		AkInt iRandomValue = (AkUInt16)(AKRANDOM::AkRandom() % in_pRandomInfo->m_wRemainingItemsToPlay);
		while(iValidCount < iRandomValue)
		{
			if(CanPlayPosition(in_pRandomInfo, iCycleCount))
			{
				++iValidCount;
			}
			++iCycleCount;
			AKASSERT(((size_t)(iCycleCount-1)) < m_pPlayList->Length());
		}
	}
	wPosition = iCycleCount - 1;
	
	if(RandomMode() == RandomMode_Normal)//Normal
	{
		if(!in_pRandomInfo->IsFlagSetPlayed(wPosition))
		{
			in_pRandomInfo->FlagSetPlayed(wPosition);
			in_pRandomInfo->m_wCounter -=1;
		}
		if(m_RanSeqParams.m_wAvoidRepeatCount)
		{
			in_pRandomInfo->m_wRemainingItemsToPlay -= 1;
			if( in_pRandomInfo->m_listAvoid.AddLast(wPosition) == NULL )
			{
				// Reset counter( will force refresh on next play ).
				// and return position 0.
				in_pRandomInfo->m_wCounter = 0;
				return wPosition;
			}
			in_pRandomInfo->FlagAsBlocked(wPosition);
			in_pRandomInfo->m_ulRemainingWeight -= pRandomPlaylist->GetWeight(wPosition);
			AkUInt16 uVal = (AkUInt16)( m_pPlayList->Length()-1 );
			if(in_pRandomInfo->m_listAvoid.Length() > AkMin(m_RanSeqParams.m_wAvoidRepeatCount, uVal ))
			{
				AkUInt16 wToBeRemoved = in_pRandomInfo->m_listAvoid.First();
				in_pRandomInfo->FlagAsUnBlocked(wToBeRemoved);
				in_pRandomInfo->m_ulRemainingWeight += pRandomPlaylist->GetWeight(wToBeRemoved);
				in_pRandomInfo->m_wRemainingItemsToPlay += 1;
				in_pRandomInfo->m_listAvoid.RemoveFirst();
			}
		}
	}
	else//Shuffle
	{
		AkUInt16 wBlockCount = m_RanSeqParams.m_wAvoidRepeatCount?m_RanSeqParams.m_wAvoidRepeatCount:1;

		in_pRandomInfo->m_ulRemainingWeight -= pRandomPlaylist->GetWeight(wPosition);
		in_pRandomInfo->m_wRemainingItemsToPlay -= 1;
		in_pRandomInfo->m_wCounter -=1;
		in_pRandomInfo->FlagSetPlayed(wPosition);
		if( in_pRandomInfo->m_listAvoid.AddLast(wPosition) == NULL )
		{
			// Reset counter( will force refresh on next play ).
			// and return position 0.
			in_pRandomInfo->m_wCounter = 0;
			return wPosition;
		}
		in_pRandomInfo->FlagAsBlocked(wPosition);

		AkUInt16 uVal = (AkUInt16)( m_pPlayList->Length()-1 );
		if(in_pRandomInfo->m_listAvoid.Length() > AkMin( wBlockCount, uVal ))
		{
			AkUInt16 wToBeRemoved = in_pRandomInfo->m_listAvoid.First();
			in_pRandomInfo->m_listAvoid.RemoveFirst();
			in_pRandomInfo->FlagAsUnBlocked(wToBeRemoved);
			if(!in_pRandomInfo->IsFlagSetPlayed(wToBeRemoved))
			{
				in_pRandomInfo->m_ulRemainingWeight += pRandomPlaylist->GetWeight(wToBeRemoved);
				in_pRandomInfo->m_wRemainingItemsToPlay += 1;
			}
		}
	}
	return wPosition;
}

bool CAkRanSeqCntr::CanContinueAfterCompleteLoop(AkLoop* io_pLoopingInfo/*= NULL*/)
{
	bool bAnswer = false;
	if(!io_pLoopingInfo)
	{
		bAnswer = true;
	}
	else if(io_pLoopingInfo->bIsEnabled)
	{
		if(io_pLoopingInfo->bIsInfinite)
		{
			bAnswer = true;
		}
		else
		{
			--(io_pLoopingInfo->lLoopCount);
			bAnswer = (io_pLoopingInfo->lLoopCount)? true:false; 
		}
	}
	return bAnswer;
}

bool CAkRanSeqCntr::CanPlayPosition(CAkRandomInfo* in_pRandomInfo, AkUInt16 in_wPosition)
{
	if(RandomMode() == RandomMode_Normal)
	{
		if(m_RanSeqParams.m_wAvoidRepeatCount)
		{
			return !in_pRandomInfo->IsFlagBlocked(in_wPosition);
		}
		else
		{
			return true;
		}
	}
	else//Shuffle
	{
		return !in_pRandomInfo->IsFlagSetPlayed(in_wPosition) &&
			!in_pRandomInfo->IsFlagBlocked(in_wPosition);
	}
}

AKRESULT CAkRanSeqCntr::Play( AkPBIParams& in_rPBIParams )
{
	m_bContainerBeenPlayed = true;
	AKRESULT eResult = AK_Fail;

	if ( m_RanSeqParams.m_bIsContinuous )
	{
		// We are going from PBIParams to ContinuousPBIParams
		if ( in_rPBIParams.eType == AkPBIParams::PBI )
		{
			// in_rPBIParams.pContinuousParams should not exists
			AKASSERT( in_rPBIParams.pContinuousParams == NULL );

			in_rPBIParams.eType = AkPBIParams::ContinuousPBI;
            in_rPBIParams.pInstigator = this;

			AkPathInfo	PathInfo = {NULL, AK_INVALID_UNIQUE_ID };

            ContParams continuousParams( &PathInfo );
            continuousParams.spContList.Attach( CAkContinuationList::Create() );
			if ( !continuousParams.spContList )
				return AK_Fail;
			
            in_rPBIParams.pContinuousParams = &continuousParams;

			// We need to duplicate this function call since "continuousParams" is created on the stack.
			// If we get out of this scope, it won't be valid anymore
			return _PlayContinuous( in_rPBIParams );
		}

		return _PlayContinuous( in_rPBIParams );
	}
	else
	{
		return _Play( in_rPBIParams );
	}
}

AKRESULT CAkRanSeqCntr::_Play( AkPBIParams& in_rPBIParams )
{
	AKASSERT( !m_RanSeqParams.m_bIsContinuous );
	AKRESULT eResult( AK_Fail );

	AkUInt16 wPositionSelected( 0 );

	CAkAudioNode* pSelectedNode = GetNextToPlay( in_rPBIParams.pGameObj, wPositionSelected );

	if( pSelectedNode )
	{
		in_rPBIParams.playHistory.Add( wPositionSelected, false );
		eResult = pSelectedNode->Play( in_rPBIParams );
		pSelectedNode->Release();
		return eResult;
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_SelectedChildNotAvailable );
		if ( in_rPBIParams.eType == AkPBIParams::PBI )
		{
			CAkCntrHist HistArray;
			MONITOR_OBJECTNOTIF( in_rPBIParams.userParams.PlayingID, in_rPBIParams.pGameObj->ID(), in_rPBIParams.userParams.CustomParam, AkMonitorData::NotificationReason_PlayFailed, HistArray, ID(), 0 );
			return eResult;
		}
	}

	eResult =  PlayAndContinueAlternate( in_rPBIParams );

	if( eResult == AK_PartialSuccess )
	{
		eResult = AK_Success;
	}
	return eResult;
}

AKRESULT CAkRanSeqCntr::_PlayContinuous( AkPBIParams& in_rPBIParams )
{
	AKASSERT( m_RanSeqParams.m_bIsContinuous &&  
	          in_rPBIParams.pContinuousParams && 
	          in_rPBIParams.pContinuousParams->spContList );

    AKRESULT eResult( AK_Fail );

	CAkContinueListItem * pItem = in_rPBIParams.pContinuousParams->spContList->m_listItems.AddLast();
	if ( pItem )
	{
		pItem->m_pContainer = this;

		pItem->m_LoopingInfo.bIsEnabled = ( m_Loop != AkLoopVal_NotLooping );
		pItem->m_LoopingInfo.bIsInfinite = ( m_Loop == AkLoopVal_Infinite );

		if(!(pItem->m_LoopingInfo.bIsEnabled))
		{
			pItem->m_LoopingInfo.lLoopCount = AkLoopVal_NotLooping;
		}
		else
		{
			pItem->m_LoopingInfo.lLoopCount = (pItem->m_LoopingInfo.bIsInfinite? AkLoopVal_NotLooping : m_Loop );
		}

		AkUInt16 wPositionSelected = 0;
		CAkAudioNode* pSelectedNode = pItem->m_pContainer->GetNextToPlayContinuous( in_rPBIParams.pGameObj,
																					wPositionSelected,
																					pItem->m_pContainerInfo,
																			pItem->m_LoopingInfo );

		if( pSelectedNode )
		{
			in_rPBIParams.playHistory.Add( wPositionSelected, true );
			eResult = pSelectedNode->Play( in_rPBIParams );
			pSelectedNode->Release();
			return eResult;
		}
		else
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_SelectedChildNotAvailable );
            in_rPBIParams.pContinuousParams->spContList->m_listItems.RemoveLast();
		}
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_PlayFailed );
	}

	eResult =  PlayAndContinueAlternate( in_rPBIParams );

	if( eResult == AK_PartialSuccess )
	{
		eResult = AK_Success;
	}

	return eResult;
}

CAkPBI* CAkRanSeqCntr::CreatePBI( CAkSoundBase* in_pSound,
                                  CAkSource*	in_pSource,
                                  AkPBIParams&  in_rPBIParams,
                                  AkPriority    in_priority ) const
{
    if ( in_rPBIParams.eType == AkPBIParams::ContinuousPBI )
    {
        return AkNew( RENDERER_DEFAULT_POOL_ID, CAkContinuousPBI( in_pSound,
                                                                  in_pSource,
                                                                  in_rPBIParams.pGameObj,
                                                                  *in_rPBIParams.pContinuousParams,
                                                                  in_rPBIParams.userParams,
                                                                  in_rPBIParams.playHistory,
                                                                  in_rPBIParams.bIsFirst,
                                                                  in_rPBIParams.sequenceID,
                                                                  in_rPBIParams.pInstigator,
                                                                  in_priority,
																  in_rPBIParams.bTargetFeedback) );
    }

    return CAkPBIAware::CreatePBI( in_pSound, in_pSource, in_rPBIParams, in_priority );
    
}


void CAkRanSeqCntr::UpdateResetPlayListSetup(CAkSequenceInfo* in_pSeqInfo, CAkRegisteredObj * in_pGameObj)
{
	AKASSERT(m_RanSeqParams.m_eMode == ContainerMode_Sequence);
	AKASSERT(in_pSeqInfo);
	CAkSequenceInfo* pSequenceInfo;
	if(IsGlobal())
	{
		AKASSERT(m_pGlobalContainerInfo);
		pSequenceInfo = static_cast<CAkSequenceInfo*>(m_pGlobalContainerInfo);
	}
	else//per object
	{
		CAkContainerBaseInfo* pCntrBaseInfo = m_mapObjectCntrInfo.Exists(in_pGameObj);
		AKASSERT( pCntrBaseInfo );
		pSequenceInfo = static_cast<CAkSequenceInfo*>( pCntrBaseInfo );
	}
	pSequenceInfo->m_bIsForward = in_pSeqInfo->m_bIsForward;

	// Always put the last one as the last selected, since the lastselected is never 
	// playing yet unless in crossfade.
	if(pSequenceInfo->m_bIsForward)
	{
		pSequenceInfo->m_i16LastPositionChosen = in_pSeqInfo->m_i16LastPositionChosen-1;
	}
	else
	{
		pSequenceInfo->m_i16LastPositionChosen = in_pSeqInfo->m_i16LastPositionChosen+1;
	}

	// Reset the continuous mode if reached the end of the list, otherwise, 
	// the next play will conclude that there is nothing more to play
	if(pSequenceInfo->m_i16LastPositionChosen + 1 == m_pPlayList->Length() && !m_RanSeqParams.m_bIsRestartBackward)
	{
		pSequenceInfo->m_i16LastPositionChosen = -1;
	}
	else if(pSequenceInfo->m_i16LastPositionChosen == 0 && !(pSequenceInfo->m_bIsForward) )
	{
		pSequenceInfo->m_bIsForward = true;
	}
}

AKRESULT CAkRanSeqCntr::RemoveChild(AkUniqueID in_ulID)
{
	// TODO(alessard) créer un event stop all qui va vider la pending list sous ce container
	// À faire quand la pending list sera implémentée
	AKASSERT(in_ulID);
    AKRESULT eResult = AK_Success;
	bool bToBeReleased = false;
	CAkAudioNode** l_ppANPtr = m_mapChildId.Exists( in_ulID );
	if( l_ppANPtr )
    {
		(*l_ppANPtr)->Parent(NULL);
        m_mapChildId.Unset( in_ulID );
		bToBeReleased = true;
    }
	
	if(bToBeReleased)
	{
		this->Release();
	}

    return eResult;
}

#ifndef AK_OPTIMIZED

void CAkRanSeqCntr::ForceNextToPlay(AkInt16 in_iPosition, CAkRegisteredObj * in_pGameObj/*=NULL*/,AkPlayingID in_PlayingID/*= NO_PLAYING_ID*/)
{
	AKASSERT(g_pPlayingMgr);
	AKASSERT(in_iPosition >= 0);
	if((in_iPosition < static_cast<AkInt16>(m_pPlayList->Length())) && (m_RanSeqParams.m_eMode == ContainerMode_Sequence))
	{
		if(!m_RanSeqParams.m_bIsContinuous)
		{
			if(m_RanSeqParams.m_bIsGlobal)
			{
				if(!m_pGlobalContainerInfo)
				{
					m_pGlobalContainerInfo =  AkNew( g_DefaultPoolId, CAkSequenceInfo() );
					AKASSERT( m_pGlobalContainerInfo );
					if( !m_pGlobalContainerInfo )
						return;
					
				}
				CAkSequenceInfo* pSeqInfo = static_cast<CAkSequenceInfo*>(m_pGlobalContainerInfo);
				pSeqInfo->m_i16LastPositionChosen = in_iPosition-1;
				pSeqInfo->m_bIsForward = true;
			}
			else
			{
				CAkSequenceInfo* pSeqInfo;
				CAkContainerBaseInfo* pCntrBaseInfo = m_mapObjectCntrInfo.Exists(in_pGameObj);
				if( !pCntrBaseInfo )
				{
					pSeqInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
					AKASSERT(pSeqInfo != NULL);
					if( !pSeqInfo )
						return;

					pSeqInfo->key = in_pGameObj;
					m_mapObjectCntrInfo.Set( pSeqInfo );
					in_pGameObj->SetNodeAsModified( this );
				}
				else
				{
					pSeqInfo = static_cast<CAkSequenceInfo*>( pCntrBaseInfo );
				}
				AKASSERT(pSeqInfo);
				pSeqInfo->m_i16LastPositionChosen = in_iPosition-1;
				pSeqInfo->m_bIsForward = true;
			}
		}
		else if( in_PlayingID && g_pPlayingMgr->IsActive(in_PlayingID) )//It is continuous
		{
			CAkContinueListItem item;

			item.m_pContainer = this;

			item.m_LoopingInfo.bIsEnabled = ( m_Loop != AkLoopVal_NotLooping );
			item.m_LoopingInfo.bIsInfinite = ( m_Loop == AkLoopVal_Infinite );
				
			item.m_LoopingInfo.lLoopCount = AkLoopVal_NotLooping;

			CAkSequenceInfo* pSeqInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
			if( !pSeqInfo )
				return;

			pSeqInfo->m_bIsForward = true;
			pSeqInfo->m_i16LastPositionChosen = in_iPosition;
			item.m_pContainerInfo = pSeqInfo;

			g_pPlayingMgr->StopAndContinue( in_PlayingID, in_pGameObj, item, m_pPlayList->ID( in_iPosition ), in_iPosition, this );

			item.m_pContainerInfo = NULL; // IMPORTANT: because we cheat and use a CAkContinueListItem on the stack,
										  // avoid destruction of m_pContainerInfo in ~CAkContinueListItem.
		}
		else//It is continuous
		{
			if(!m_pGlobalContainerInfo)
			{
				m_pGlobalContainerInfo =  AkNew(g_DefaultPoolId,CAkSequenceInfo());
				AKASSERT(m_pGlobalContainerInfo);
				if( !m_pGlobalContainerInfo )
				{
					return;
				}
			}
			CAkSequenceInfo* pSeqInfo = static_cast<CAkSequenceInfo*>(m_pGlobalContainerInfo);
			pSeqInfo->m_i16LastPositionChosen = in_iPosition-1;
			pSeqInfo->m_bIsForward = true;
		}
	}
}

AkInt16 CAkRanSeqCntr::NextToPlay(CAkRegisteredObj * in_pGameObj/*=NULL*/)
{
	CAkSequenceInfo* pSeqInfo = NULL;
	AkInt16 i16Next = 0;
	if(m_RanSeqParams.m_eMode == ContainerMode_Sequence && m_pPlayList->Length() > 1)
	{			
		if(!m_RanSeqParams.m_bIsContinuous)
		{
			if(m_RanSeqParams.m_bIsGlobal)
			{
				if(m_pGlobalContainerInfo)
				{
					pSeqInfo = static_cast<CAkSequenceInfo*>(m_pGlobalContainerInfo);
				}
			}
			else
			{
				CAkContainerBaseInfo* pCntrBaseInfo = m_mapObjectCntrInfo.Exists(in_pGameObj);
				if( pCntrBaseInfo )
				{
					pSeqInfo = static_cast<CAkSequenceInfo*>( pCntrBaseInfo );
				}
			}
		}
		else// Continuous
		{
			if( m_pGlobalContainerInfo )
			{
				pSeqInfo = static_cast<CAkSequenceInfo*>(m_pGlobalContainerInfo);
			}
		}
		if(pSeqInfo)
		{
			if(pSeqInfo->m_bIsForward)
			{
				if(pSeqInfo->m_i16LastPositionChosen >= static_cast<AkInt16>(m_pPlayList->Length()-1))
				{
					if(m_RanSeqParams.m_bIsRestartBackward)
					{
						i16Next = pSeqInfo->m_i16LastPositionChosen - 1;
					}
				}
				else
				{
					i16Next = pSeqInfo->m_i16LastPositionChosen+1;
				}
			}
			else
			{
				AKASSERT(pSeqInfo->m_i16LastPositionChosen >=0);
				if(pSeqInfo->m_i16LastPositionChosen > 0)
				{
					i16Next = pSeqInfo->m_i16LastPositionChosen - 1;
				}
				else
				{
					i16Next = 1;
				}
			}
		}
	}
	return i16Next;
}

#endif // AK_OPTIMIZED
