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
// AkPositionRepository.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_POSITION_REPOSITORY_H_
#define _AK_POSITION_REPOSITORY_H_

#include "AkKeyArray.h"
#include <AK/Tools/Common/AkLock.h>

#define AK_POSITION_REPOSITORY_MIN_NUM_INSTANCES 8

struct AkPositionInfo
{
	AkBufferPosInformation	bufferPosInfo;	//Buffer position information
	AkInt64					timeUpdated;	//Last time information was updated
	void *					cookie;			//Unique caller ID to avoid clash between 2 sources
};

class CAkPositionRepository : public CAkObject
{
public:
	CAkPositionRepository(); //constructor
	~CAkPositionRepository(); //destructor

	//initialization
	AKRESULT Init();
	AKRESULT Term();

	//Public Methods
	AKRESULT UpdatePositionInfo( AkPlayingID in_PlayingID, AkBufferPosInformation* in_pPosInfo, void* in_cookie );
	AKRESULT GetCurrPosition( AkPlayingID in_PlayingID, AkUInt32* out_puPos );
	AKRESULT RemovePlayingID( AkPlayingID in_PlayingID );

	void UpdateTime();

	// SetRate, mostly used for paused/resumed sounds.
	void SetRate( AkPlayingID in_PlayingID, AkReal32 in_fNewRate );

private:
	CAkKeyArray<AkPlayingID, AkPositionInfo> m_mapPosInfo;

	CAkLock m_lock;
	AkInt64 m_i64LastTimeUpdated;
};

extern CAkPositionRepository* g_pPositionRepository;

#endif //_AK_POSITION_REPOSITORY_H_
