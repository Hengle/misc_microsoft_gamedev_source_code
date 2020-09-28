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
// AkIDStringMap.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ID_STRING_MAP_H_
#define _ID_STRING_MAP_H_

#include "AkHashList.h"
#include "AkKeyArray.h"

// bernstein hash
// IMPORTANT: must be kept in sync with version in SoundBankIDToStringChunk.cpp
inline AkUInt32 AkHash( const char * in_string )
{
    AkUInt32 hash = 5381;
    int c;

    while (c = *in_string++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

inline AkUInt32 AkHash( AkLpCtstr in_string )
{
    AkUInt32 hash = 5381;
    int c;

    while (c = *in_string++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

// Basically, just a simple wrapper around data read straight from the bank.
class AkStringIDHash
{
public:
	AkUInt32 GetID( const char * in_string );
	AkUInt32 GetID( AkUInt32 in_uiHash, const char * in_string );

	// Although data is kept as const char *, clients use TCHAR
	AkUInt32 GetID( AkLpCtstr in_string );
	AkUInt32 GetID( AkUInt32 in_uiHash, AkLpCtstr in_string );

private:
	AkStringIDHash() {}; // do not create -- simply cast bank data into (AkStringIDHash *) 

	AkUInt32 m_uiTableSize;
};

typedef CAkKeyArray<AkUniqueID, AkStringIDHash *> AkGroupStringIdx;

class AkIDStringHash
{
public:
	typedef AkHashList< AkUInt32, char, 31 > AkStringHash;

	AKRESULT Init( AkMemPoolId in_MemPoolId );

	void Term();

	AkStringHash::Item * Preallocate( AkUInt32 in_ID, const char* in_pszString );
	void FreePreallocatedString( AkStringHash::Item * in_pItem );

	AKRESULT Set( AkStringHash::Item * in_pItem );

	AKRESULT Set( AkUInt32 in_ID, const char* in_pszString );

	void Unset( AkUInt32 in_ID );

	char * GetStr( AkUInt32 in_ID );

//Private, but set as public to allowing to iterate in the list.
	AkStringHash m_list;
};


#endif //_ID_STRING_MAP_H_
