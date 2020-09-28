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
// AkPath.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkTransition.h"
#include "AkTransitionManager.h"
#include "AkPBI.h"
#include "AkPath.h"
#include "AkAudioLibIndex.h"
#include "AudiolibDefs.h"
#include "AkMath.h"
#include "AkMonitor.h"
#include "AkRandom.h"
#include "AkGen3DParams.h"
#include "AkPathManager.h"

extern AkMemPoolId g_DefaultPoolId;
//====================================================================================================
//====================================================================================================
CAkPath::CAkPath(AkUniqueID in_ulID)
{
	m_eState = Idle;				// not active
	m_pPathsList = NULL;			// no play list
	m_pbPlayed = NULL;				// no played flags
	m_pCurrentList = NULL;			// not playing any
	m_ulListSize = 0;				// empty list
	m_PathMode = AkStepSequence;	// default
	m_bWasStarted = false;			// initially not started
	m_bIsLooping = false;
	m_iPotentialUsers = 0;			// no one could be using it
	m_iNumUsers = 0;				// no one is using it
	m_StartTime = 0;
	m_EndTime = 0;
	m_Duration = 0;
	m_TimePaused = 0;
	m_ulSoundUniqueID = AK_INVALID_UNIQUE_ID;
	m_playingID = AK_INVALID_PLAYING_ID;
}
//====================================================================================================
//====================================================================================================
CAkPath::~CAkPath()
{
}
//====================================================================================================
//====================================================================================================
void CAkPath::Term()
{
	AKASSERT(g_pTransitionManager);

	m_eState = Idle;
	m_PBIsList.Term();

	// if continuous then the state was not saved and the sound won't free the played flags
	if(m_PathMode & AkPathContinuous)
	{
		// get rid of the path played flags if any
		if(m_pbPlayed != NULL)
		{
			AkFree(g_DefaultPoolId,m_pbPlayed);
			m_pbPlayed = NULL;
		}
	}

}
//====================================================================================================
// start moving along our path
//====================================================================================================
AKRESULT CAkPath::Start( AkUInt32 in_CurrentBufferTick )
{
	AKASSERT(g_pTransitionManager);
	AKRESULT	eResult;

	// assume no list
	eResult = AK_Fail;

	// have we got a play list ?
	if(m_pCurrentList != NULL)
	{
		// assume none
		eResult = AK_PathNoVertices;
		m_bWasStarted = true;

		// any vertex ? // copy these vertices
		if( m_pCurrentList->iNumVertices > 0 )
        {
			m_uCurrentVertex = 0;

			eResult = AK_Success;

			// get the starting vertex
			AkPathVertex & From = m_pCurrentList->pVertices[ m_uCurrentVertex++ ];

			// set start position
			m_StartPosition.X = From.Vertex.X;
			m_StartPosition.Y = From.Vertex.Y;
			m_StartPosition.Z = From.Vertex.Z;

			// taking that much time
			m_Duration = CAkPath::Convert( From.Duration );

			// any destination vertex ?
			if( m_uCurrentVertex < m_pCurrentList->iNumVertices )
			{
				// get the destination vertex
				AkPathVertex & To = m_pCurrentList->pVertices[ m_uCurrentVertex ];

				// compute direction
				m_Direction.X = To.Vertex.X - m_StartPosition.X;
				m_Direction.Y = To.Vertex.Y - m_StartPosition.Y;
				m_Direction.Z = To.Vertex.Z - m_StartPosition.Z;
			}
			else
			{
				// we're not moving
				m_Direction.X = 0.0f;
				m_Direction.Y = 0.0f;
				m_Direction.Z = 0.0f;
			}

			// send the start position to the users
			UpdateStartPosition();

			// start moving
			m_StartTime = in_CurrentBufferTick;
			m_EndTime = in_CurrentBufferTick + m_Duration;
			m_fa = 1.0f / m_Duration;
			m_fb = -( m_StartTime * m_fa );
			// update state
			m_eState = Running;

			// PhM : insert path started notification here
			MONITOR_PATH_EVENT(m_playingID,m_ulSoundUniqueID,AkMonitorData::AkPathEvent_ListStarted,m_ulCurrentListIndex);
		}
	}

	return eResult;
}
//====================================================================================================
// stop moving the sound
//====================================================================================================
void CAkPath::Stop()
{

	// update the state
	m_eState = Idle;
}
//====================================================================================================
//====================================================================================================
void CAkPath::Pause( AkUInt32 in_CurrentBufferTick )
{
	m_TimePaused = in_CurrentBufferTick;
	// update the state
	m_eState = Paused;
}
//====================================================================================================
//====================================================================================================
void CAkPath::Resume( AkUInt32 in_CurrentBufferTick )
{
	// erase the time spent in pause mode
	AkUInt32 TimeDelta = in_CurrentBufferTick - m_TimePaused;
	m_StartTime += TimeDelta;
	m_EndTime += TimeDelta;
	m_fb = -( m_StartTime * m_fa );

	// update the state
	m_eState = Running;
}
//====================================================================================================
// start moving toward next one
//====================================================================================================
void CAkPath::NextVertex()
{
	AKASSERT( m_pCurrentList );

	// get the starting vertex

    // REVIEW: This condition is meant to prevent crashes when the SECOND vertex could not be added to the
    // list. Wwise always packages at least 2 vertices even if there is only one. But out-of-memory conditions
    // can still occur, so this check has to be done anyway. TODO: Package only one vertex when path lenght is 1.
    if ( m_uCurrentVertex < m_pCurrentList->iNumVertices )
    {
	    AkPathVertex & To = m_pCurrentList->pVertices[ m_uCurrentVertex++ ];

	    // -- The start position must be set even if there in no more vertex
	    // -- If there is no more vertex, it will also stand as the last notified position, required for Continuous mode containers sharing the same path
	    // -- When the transition reaches the end, that position will be given as unique position to every other sounds sharing this path.
	    m_StartPosition.X = To.Vertex.X;
	    m_StartPosition.Y = To.Vertex.Y;
	    m_StartPosition.Z = To.Vertex.Z;

	    m_Duration = CAkPath::Convert( To.Duration );
    }

	bool bHasNextOne = true;
	bool bStartNewList = false;
	// not enough to start a new move ?
	if(m_uCurrentVertex >= m_pCurrentList->iNumVertices )
	{
		// no more vertices, look for another list
		if(GetNextPathList() == AK_Success)
		{
			bStartNewList = true;
		}
		else
		{
			m_eState = Idle;
			bHasNextOne = false;
		}
	}

	// can we keep moving on ?
	if(bHasNextOne)
	{
	    AkPathVertex & To = m_pCurrentList->pVertices[ m_uCurrentVertex ];

		// compute direction
		m_Direction.X = To.Vertex.X - m_StartPosition.X;
		m_Direction.Y = To.Vertex.Y - m_StartPosition.Y;
		m_Direction.Z = To.Vertex.Z - m_StartPosition.Z;

		// start moving
		m_StartTime = m_EndTime;
		m_EndTime += m_Duration;
		m_fa = 1.0f / m_Duration;
		m_fb = -( m_StartTime * m_fa );

		if(bStartNewList)
		{
			// new one started
			MONITOR_PATH_EVENT(m_playingID,m_ulSoundUniqueID,AkMonitorData::AkPathEvent_ListStarted,m_ulCurrentListIndex);
		}
	}
}
//====================================================================================================
//====================================================================================================
bool CAkPath::IsRunning()
{
	return (m_eState == Running);
}
//====================================================================================================
//====================================================================================================
bool CAkPath::IsIdle()
{
	return (m_eState == Idle);
}
//====================================================================================================
// tells how many we can take
//====================================================================================================
/*
AkInt32 CAkPath::NeedRefill()
{
	return AK_PATH_VERTEX_LIST_SIZE - m_VertexList.Length();
}
*/
//====================================================================================================
// update the positions
//====================================================================================================
void CAkPath::UpdatePosition( AkUInt32 in_CurrentBufferTick )
{
	// compute the current value
	AkReal32 fdP = m_fa * in_CurrentBufferTick + m_fb;

	fdP = AkMath::Min(fdP,1.0f);
	fdP = AkMath::Max(fdP,0.0f);

	// compute the new position
	AkVector	Position;
	Position.X = m_StartPosition.X + fdP * m_Direction.X;
	Position.Y = m_StartPosition.Y + fdP * m_Direction.Y;
	Position.Z = m_StartPosition.Z + fdP * m_Direction.Z;

	// send the new stuff to the users
	for( AkPBIList::Iterator iter = m_PBIsList.Begin(); iter != m_PBIsList.End(); ++iter )
	{
			AKASSERT(*iter);

			// set the sound's position
			SetPosition( *iter, Position );
	}

	// are we done with this one ?
	if( in_CurrentBufferTick >= m_EndTime )
	{
		// go to next one
		NextVertex();
/*
		// PhM : insert vertex reached notification here
		MONITOR_PATH_EVENT(m_playingID,m_ulSoundUniqueID,AkMonitorData::VertexReached,m_ulVertexIndex);
*/
	}
}
//====================================================================================================
// sets the play list that is going to be used
//====================================================================================================
AKRESULT CAkPath::SetPathsList(AkPathListItem*	in_pPathList,
							   AkUInt32			in_ulListSize,
							   AkPathMode		in_PathMode,
							   bool				in_bIsLooping,
							   AkPathState*		in_pState)
{
	AKRESULT eResult = AK_Fail;

	// is it ok to set the list ?
	if(m_eState == Idle)
	{
		m_pPathsList = in_pPathList;
		m_ulListSize = (AkUInt16) in_ulListSize;
		m_PathMode = in_PathMode;
		m_bIsLooping = in_bIsLooping;
//----------------------------------------------------------------------------------------------------
// we have got played flags in the saved state
//----------------------------------------------------------------------------------------------------
		if(in_pState->pbPlayed != NULL)
		{
			// use these
			m_pbPlayed = in_pState->pbPlayed;
			m_ulCurrentListIndex = (AkUInt16) in_pState->ulCurrentListIndex;
			// set the current one
			m_pCurrentList = m_pPathsList + m_ulCurrentListIndex;
		}
//----------------------------------------------------------------------------------------------------
// we have not got played flags in the saved state
//----------------------------------------------------------------------------------------------------
		else
		{
			m_ulCurrentListIndex = 0;
			m_pCurrentList = in_pPathList;

			// allocate the played flags array
			void*	pvTempPtr;
			pvTempPtr = AkAlloc(g_DefaultPoolId,in_ulListSize);
			m_pbPlayed = (bool*)pvTempPtr;
			if( !m_pbPlayed )
			{
				return AK_Fail;
			}

			// in case we get an array that was previously used
			ClearPlayedFlags();

			// should we randomly pick the first one ?
			if(in_PathMode & AkPathRandom)
			{
				PickRandomList();
			}
		}
		// we're all set
		eResult = AK_Success;
	}

	return eResult;
}
//====================================================================================================
//====================================================================================================
AkUInt32 CAkPath::GetListIndex()
{
	return m_ulCurrentListIndex;
}
//====================================================================================================
// looks for the next path in the playlist, makes it current and adds its vertices
//====================================================================================================
AKRESULT CAkPath::GetNextPathList()
{
	AKRESULT	eResult = AK_NoMoreData;
	bool		bLooped;

	// have we got a list already ?
	if(m_pCurrentList != NULL)
	{
		// next one is random
		if(m_PathMode & AkPathRandom)
		{
			bLooped = PickRandomList();
		}
		// next one is sequence
		else
		{
			bLooped = PickSequenceList();
		}

		// should we continue ?
		if(m_PathMode & AkPathContinuous)
		{
			//  should we keep going ?
			if(!bLooped || (bLooped && m_bIsLooping))
			{
				m_uCurrentVertex = 0;
                eResult = AK_Success;
			}
		}
	}

	return eResult;
}
//====================================================================================================
//====================================================================================================
bool CAkPath::PickSequenceList()
{
	// assume some more to play
	bool	bLooped = false;

	// move to next one, is it ok ?
	if(++m_ulCurrentListIndex < m_ulListSize)
	{
		// go to the next list
		// m_pCurrentList modification is not allowed in step mode, see bug WG-3846
		if( m_PathMode & AkPathContinuous )
		{
			++m_pCurrentList;
		}
	}
	else
	{
		// back to first list
		// m_pCurrentList modification is not allowed in step mode, see bug WG-3846
		if( m_PathMode & AkPathContinuous )
		{
			m_pCurrentList = m_pPathsList;
		}
		m_ulCurrentListIndex = 0;
		bLooped = true;
	}
	return bLooped;
}
//====================================================================================================
// random : loop = all of them played at least once.
//====================================================================================================
bool CAkPath::PickRandomList()
{
	// assume some more to play
	bool	bLooped = true;

	// all of them played ?
	bool* pPlayed = m_pbPlayed;
	for(AkUInt32 ulCounter = 0 ; ulCounter < m_ulListSize ; ++ulCounter)
	{
		bLooped = bLooped && *pPlayed;
		++pPlayed;
	}

	// if all have been played once reset flags
	if(bLooped)
	{
		ClearPlayedFlags();
	}

	// pick next one
	m_ulCurrentListIndex = AKRANDOM::AkRandom() % m_ulListSize;

	// set the one we picked as current
	// m_pCurrentList modification is not allowed in step mode, see bug WG-3846
	if( m_PathMode & AkPathContinuous )
	{
		m_pCurrentList = m_pPathsList + m_ulCurrentListIndex;
	}

	// set this one has played
	if( m_pbPlayed )
	{
		*(m_pbPlayed + m_ulCurrentListIndex) = true;
	}

	return bLooped;
}
//====================================================================================================
//====================================================================================================
void CAkPath::ClearPlayedFlags()
{
	if( m_pbPlayed )
	{
		bool*	pbBoolPtr = m_pbPlayed;
		// clear them all
		for(AkUInt32 ulCounter = 0 ; ulCounter < m_ulListSize ; ++ulCounter,++pbBoolPtr)
		{
			*pbBoolPtr = false;
		}
	}
}
//====================================================================================================
//====================================================================================================
void CAkPath::SetPosition(CAkPBI* in_pPBI,AkVector& in_rPosition)
{
	CAkGen3DParams* pThreeDee = in_pPBI->Get3DSound();
	AKASSERT(pThreeDee != NULL);
	pThreeDee->SetPosition(in_rPosition);
}
//====================================================================================================
//====================================================================================================

void CAkPath::SetSoundUniqueID( AkUniqueID in_soundUniqueID )
{
	m_ulSoundUniqueID = in_soundUniqueID;
}

void CAkPath::SetPlayingID( AkPlayingID in_playingID )
{
	m_playingID = in_playingID;
}

void CAkPath::SetIsLooping(bool in_bIsLooping)
{
	m_bIsLooping = in_bIsLooping;
}

void CAkPath::UpdateStartPosition()
{
	for( AkPBIList::Iterator iter = m_PBIsList.Begin(); iter != m_PBIsList.End(); ++iter )
	{
		AKASSERT(*iter);
		// set the 3D position
		SetPosition(*iter, m_StartPosition );
	}
}
