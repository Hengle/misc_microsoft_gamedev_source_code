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
// AkPlayList.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _PLAY_LIST_H_
#define _PLAY_LIST_H_

#include <AK/Tools/Common/AkArray.h>
#include "AkPoolSizes.h"

struct AkPlaylistItem
{
	AkUniqueID ulID;	//Unique ID of the Item to play
	AkUInt8 cWeight;		//Weight of the Item to play
};

// Base Class representing a playlist inside a container
class CAkPlayList : public CAkObject
{

public:

	virtual AKRESULT Init() = 0;

	virtual void Destroy() = 0;

	// Returns the lenght of the playlist
	//
	// Returns - size_t - Lenght fo the playlist
	virtual size_t Length() = 0;

	// Returns the Unique ID of the given position
	//
	// Returns - AkUniqueID - Unique ID of the given position
	virtual AkUniqueID ID(
		AkUInt16 in_wPosition	//Position in the list
		) = 0;

	// Adds an item in the playlist
	virtual AKRESULT Add(
		AkUniqueID in_ulID,	// Unique ID of the element to add
		AkUInt8 in_cWeight = DEFAULT_RANDOM_WEIGHT	// Weight of the item
		) = 0;

	// Remove the specified element from the playlist
	virtual void Remove(
		AkUniqueID in_ulID	// Unique ID of the element to remove
		) = 0;

	// Remove all items from the playlist
	virtual void RemoveAll() = 0;

	// Tells if the specified element is in the playlist
	//
	// Returns - bool - Does the item is in the playlist
	virtual bool Exists(AkUniqueID in_ulID) = 0;

	// Returns the first position in the playlist containing the given Unique ID
	//
	// Return - bool - True if found, false otherwise
	virtual bool GetPosition(AkUniqueID in_ID, AkUInt16& out_rwPosition) = 0;
};

// Class representing a playlist inside a container random
class CAkPlayListRandom : public CAkPlayList
{
public:
	//Constructor
	CAkPlayListRandom();

	//Destructor
	virtual ~CAkPlayListRandom();

	// does the stuff that might fail
	virtual AKRESULT Init();
	virtual void Destroy();

	virtual size_t Length();
	virtual AkUniqueID ID(AkUInt16 in_wPosition);
	virtual AKRESULT Add(AkUniqueID in_ulID, AkUInt8 in_cWeight = DEFAULT_RANDOM_WEIGHT);
	virtual void Remove(AkUniqueID in_ulID);
	virtual void RemoveAll();
	virtual bool Exists(AkUniqueID in_ulID);
	virtual bool GetPosition(AkUniqueID in_ID, AkUInt16& out_rwPosition);
	virtual void SetWeight(AkUInt16 in_wPosition,AkUInt8 in_cWeight);
	virtual AkUInt8 GetWeight(AkUInt16 in_wPosition);
	virtual AkUInt32 CalculateTotalWeight();
	

private:
	typedef AkArray<AkPlaylistItem, const AkPlaylistItem&, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(AkPlaylistItem)> AkPlayListRdm;
	AkPlayListRdm m_PlayList;	//Playlist
};

// Class representing a playlist inside a container sequence
class CAkPlayListSequence : public CAkPlayList
{
public:
	//Constructor
	CAkPlayListSequence();

	//Destructor
	virtual ~CAkPlayListSequence();

	// does the stuff that might fail
	virtual AKRESULT Init();
	virtual void Destroy();

	// Returns the lenght of the playlist
	//
	// Returns - size_t - Lenght fo the playlist
	virtual size_t Length();
	virtual AkUniqueID ID(AkUInt16 in_wPosition);
	virtual AKRESULT Add(AkUniqueID in_ulID, AkUInt8 in_cWeight = DEFAULT_RANDOM_WEIGHT);
	virtual void Remove(AkUniqueID in_ulID);
	virtual void RemoveAll();
	virtual void SetWeight(AkUInt16 in_wPosition,AkUInt8 in_cWeight);
	virtual bool Exists(AkUniqueID in_ulID);
	virtual bool GetPosition(AkUniqueID in_ID, AkUInt16& out_rwPosition);

private:
	typedef AkArray<AkUniqueID, AkUniqueID, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(AkUniqueID)> AkPlaylistSeq;
	AkPlaylistSeq m_PlayList;  //Playlist
};
#endif
