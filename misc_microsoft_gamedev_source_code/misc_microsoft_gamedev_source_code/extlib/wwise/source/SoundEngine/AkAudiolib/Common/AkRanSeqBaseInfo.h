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
// AkRanSeqBaseInfo.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _RAN_SEQ_BASE_INFO_H_
#define _RAN_SEQ_BASE_INFO_H_

#include "AkAudioLibExport.h"
#include "AkList2.h"

enum AkContainerMode
{
	ContainerMode_Random	= 0,
	ContainerMode_Sequence	= 1
};

enum AkRandomMode
{
	RandomMode_Normal	= 0,
	RandomMode_Shuffle	= 1
};

// Base class for containers having to keep specific parameters
class CAkContainerBaseInfo : public CAkObject
{
public:
	class CAkRegisteredObj * key;		// for AkMapObjectCntrInfo
	CAkContainerBaseInfo * pNextItem; 

public:

	virtual void Destroy() = 0;

	virtual AkContainerMode Type() = 0;

	// Returns a reference to a new object allocated therein, which is a copy of this. NULL if failed.
	virtual CAkContainerBaseInfo * Clone( AkUInt16 in_wItemCount ) = 0;
};

// Random container playlist specific information
class CAkRandomInfo : public CAkContainerBaseInfo
{
public:
	//Constructor
	CAkRandomInfo(
		AkUInt16 in_wItemCount // Number of items in the playlist
		);

	//Destructor
	virtual ~CAkRandomInfo();

	AKRESULT Init();

	virtual void Destroy();

	// Returns a reference to a new object allocated therein, which is a copy of this. NULL if failed.
	virtual CAkContainerBaseInfo * Clone( AkUInt16 in_wItemCount );

	
public:

	// Sets the Position as Played
	void FlagSetPlayed(
		AkUInt16 in_wPosition //Playlist Position
		);

	// Unsets the Position as Played
	void FlagUnSetPlayed(
		AkUInt16 in_wPosition//Playlist Position
		);

	// Query if the position has played
	//
	// Returns - bool - Has the specified played?
	bool IsFlagSetPlayed(
		AkUInt16 in_wPosition//Playlist Position
		);

	//Reset the Played flag list
	void ResetFlagsPlayed(
		size_t in_Size		// Playlist size
		);

	// Sets the Position as Blocked
	void FlagAsBlocked(
		AkUInt16 in_wPosition//Playlist Position
		);

	// Sets the Position as UnBlocked
	void FlagAsUnBlocked(
		AkUInt16 in_wPosition//Playlist Position
		);

	// Query if the position is Blocked
	//
	// Returns - bool - Is the specified Blocked?
	bool IsFlagBlocked(
		AkUInt16 in_wPosition//Playlist Position
		);

	virtual AkContainerMode Type();

public:
	//public, optimisation access purpose
	AkUInt32	m_ulTotalWeight;		// Total weight of the container
	AkUInt32	m_ulRemainingWeight;	// Remaining weight of the container
	AkUInt16	m_wRemainingItemsToPlay;// Remaining items playable in the container
	AkUInt16	m_wCounter;				// Number of items not set as been played in the container

	typedef CAkList2<AkUInt16, AkUInt16, AkAllocAndFree> AkAvoidList;
	AkAvoidList	m_listAvoid;	// Avoid(blocked)list

private:
	AkChar*	m_pcArrayBeenPlayedFlag;	// Bitfield array for played flags
	AkChar*	m_pcArrayBlockedFlag;		// Bitfield array for Blocked flags
};

// Sequence container playlist specific information
class CAkSequenceInfo : public CAkContainerBaseInfo
{
public:
	CAkSequenceInfo();
	virtual ~CAkSequenceInfo();

	virtual AkContainerMode Type();

	virtual void Destroy();

	// Returns a reference to a new object allocated therein, which is a copy of this. NULL if failed.
	virtual CAkContainerBaseInfo * Clone( AkUInt16 in_wItemCount );

	// No dynamic data is allowed in this structure
	// The reason for that is that a copy of this structure 
	// is required in mode "Do not reset playlist at each play"
	// the copy is done in CAkRanSeqCntr::GetNextToPlayContinuous method
public:
	AkUInt8	m_bIsForward;						//Is the sequence going forward
	AkInt16	m_i16LastPositionChosen;			//The last position played(given as a target element)
	
};
#endif
