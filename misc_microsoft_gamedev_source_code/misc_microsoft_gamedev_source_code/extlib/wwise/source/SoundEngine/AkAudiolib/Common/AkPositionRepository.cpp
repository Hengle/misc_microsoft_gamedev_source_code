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
// AkPositionRepository.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkCommon.h"
#include "AkPositionRepository.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Name: CAkPositionRepository
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkPositionRepository::CAkPositionRepository()
{
}

//-----------------------------------------------------------------------------
// Name: ~CAkPositionRepository
// Desc: Destructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkPositionRepository::~CAkPositionRepository()
{
	m_mapPosInfo.RemoveAll();
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialize CAkPositionRepository.
//
// Return: Ak_Success
//-----------------------------------------------------------------------------
AKRESULT CAkPositionRepository::Init()
{
	AkAutoLock<CAkLock> gate(m_lock);
	m_mapPosInfo.Reserve( AK_POSITION_REPOSITORY_MIN_NUM_INSTANCES );
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminates CAkPositionRepository.
//
// Return: Ak_Success
//-----------------------------------------------------------------------------
AKRESULT CAkPositionRepository::Term()
{
	AkAutoLock<CAkLock> gate(m_lock);
	m_mapPosInfo.Term();
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: UpdateTime
// Desc: Update the timer, should be called once per frame.
//
//-----------------------------------------------------------------------------
void CAkPositionRepository::UpdateTime()
{
	if( !m_mapPosInfo.IsEmpty() )
		AKPLATFORM::PerformanceCounter( &m_i64LastTimeUpdated );
}

//-----------------------------------------------------------------------------
// Name: SetRate
// Desc: Update the last rate for a given ID, useful when sounds gets paused/resumed.
//
//-----------------------------------------------------------------------------
void CAkPositionRepository::SetRate( AkPlayingID in_PlayingID, AkReal32 in_fNewRate )
{
	AkPositionInfo* pPosInfo = m_mapPosInfo.Exists( in_PlayingID );
	if( pPosInfo )
	{
		AkAutoLock<CAkLock> gate(m_lock);
		pPosInfo->timeUpdated = m_i64LastTimeUpdated;
		pPosInfo->bufferPosInfo.fLastRate = in_fNewRate;
	}
}

//-----------------------------------------------------------------------------
// Name: UpdatePositionInfo
// Desc: Updates the position information associated with a PlayingID.
//
// Return:	AK_Success
//			AK_Fail if a structure can't be allocated or the cookie is not from the original caller
//-----------------------------------------------------------------------------
AKRESULT CAkPositionRepository::UpdatePositionInfo( AkPlayingID in_PlayingID, AkBufferPosInformation* in_pPosInfo, void* in_cookie )
{
	AkAutoLock<CAkLock> gate(m_lock);
	AkPositionInfo* pPosInfo = m_mapPosInfo.Exists( in_PlayingID );
	if( !pPosInfo )
	{
		pPosInfo = m_mapPosInfo.Set( in_PlayingID );
		if( !pPosInfo )
			return AK_Fail;

		UpdateTime();
		pPosInfo->cookie = in_cookie;
	}	
	else //info already existed
	{
		if( in_cookie != pPosInfo->cookie )
			return AK_Fail; //not the original caller
	}

	pPosInfo->bufferPosInfo = *in_pPosInfo;
	pPosInfo->timeUpdated = m_i64LastTimeUpdated;

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: GetCurrPosition
// Desc: Returns the interpolated position associated with a PlayingID.
//
// Return: AK_Success, AK_Fail if structure doesn't exist yet
//-----------------------------------------------------------------------------
AKRESULT CAkPositionRepository::GetCurrPosition( AkPlayingID in_PlayingID, AkUInt32* out_puPos )
{
	AkAutoLock<CAkLock> gate(m_lock);
	AkPositionInfo* pPosInfo = m_mapPosInfo.Exists( in_PlayingID );
	if( !pPosInfo )
	{
		*out_puPos = 0;
		return AK_Fail;
	}

	//interpolation using timer
	AkInt64 CurrTime;
	AKPLATFORM::PerformanceCounter( &CurrTime );
	AkReal32 fElapsed = AKPLATFORM::Elapsed( CurrTime, pPosInfo->timeUpdated );


	AkUInt32 uInterpolatedPos = (AkUInt32)(((pPosInfo->bufferPosInfo.uStartPos*1000.f)/pPosInfo->bufferPosInfo.uSampleRate)
		+ (fElapsed * pPosInfo->bufferPosInfo.fLastRate));
	AkUInt32 uEndFileTime = (AkUInt32)((pPosInfo->bufferPosInfo.uFileEnd*1000.f)/pPosInfo->bufferPosInfo.uSampleRate);
	*out_puPos = AkMin( uInterpolatedPos, uEndFileTime );
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: RemovePlayingID
// Desc: Removes the position information associated with a PlayingID.
//
// Return: AK_Success
//-----------------------------------------------------------------------------
AKRESULT CAkPositionRepository::RemovePlayingID( AkPlayingID in_PlayingID )
{
	AkAutoLock<CAkLock> gate(m_lock);
	m_mapPosInfo.Unset( in_PlayingID );

	return AK_Success;
}
