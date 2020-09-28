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
// AkIDStringMap.cpp
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <string.h>

#include "AkIDStringMap.h"

// frankensteinesque string compare function. will only work for letters and numbers,
// as they have the same value from TCHAR to char.
static bool strequal_bitype(AkLpCtstr s1, const char *s2)
{
    while (*s1 != '\0' && *s1 == *s2) 
	{
        s1++;
        s2++;
    }
	
	return  *s1 == *s2;
}


AkUInt32 AkStringIDHash::GetID( const char * in_string )
{
	return GetID( AkHash( in_string ), in_string );
}

// This somewhat obscure piece of code looks up a string in a hash table such as written by the bank.
// "this" actually points to the start of the hash table (i.e. the table size).

AkUInt32 AkStringIDHash::GetID( AkUInt32 in_uiHash, const char * in_string )
{
	AkUInt32 uiTableIdx = in_uiHash % m_uiTableSize;

	AkUInt32 * pTable = (AkUInt32 *) this + 1;

	AkUInt32 uiListOffset = pTable[ uiTableIdx ];

	if ( uiListOffset == 0xFFFFFFFF ) // 0xFFFFFFFF means table entry has no corresponding list
		return AK_INVALID_UNIQUE_ID;

	char * pListItem = (char *) ( pTable + m_uiTableSize ) + uiListOffset;

	do
	{
		char * pThisString = pListItem + sizeof( AkUInt32 );
		
		if ( !strcmp( in_string, pThisString ) )
			return *((AkUInt32 *) pListItem); // ID

		pListItem = pThisString + strlen( pThisString ) + 1;
	}
	while ( *((AkUInt32 *) pListItem) != 0xFFFFFFFF );

	return AK_INVALID_UNIQUE_ID;
}

AkUInt32 AkStringIDHash::GetID( AkLpCtstr in_string )
{
	return GetID( AkHash( in_string ), in_string );
}

// This somewhat obscure piece of code looks up a string in a hash table such as written by the bank.
// "this" actually points to the start of the hash table (i.e. the table size).

AkUInt32 AkStringIDHash::GetID( AkUInt32 in_uiHash, AkLpCtstr in_string )
{
	AkUInt32 uiTableIdx = in_uiHash % m_uiTableSize;

	AkUInt32 * pTable = (AkUInt32 *) this + 1;

	AkUInt32 uiListOffset = pTable[ uiTableIdx ];

	if ( uiListOffset == 0xFFFFFFFF ) // 0xFFFFFFFF means table entry has no corresponding list
		return AK_INVALID_UNIQUE_ID;

	char * pListItem = (char *) ( pTable + m_uiTableSize ) + uiListOffset;

	do
	{
		char * pThisString = pListItem + sizeof( AkUInt32 );
		
		if ( strequal_bitype( in_string, pThisString ) )
			return *((AkUInt32 *) pListItem); // ID

		pListItem = pThisString + strlen( pThisString ) + 1;
	}
	while ( *((AkUInt32 *) pListItem) != 0xFFFFFFFF );

	return AK_INVALID_UNIQUE_ID;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of AkIDStringHash
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

AKRESULT AkIDStringHash::Init( AkMemPoolId in_MemPoolId )
{
	return m_list.Init( in_MemPoolId );
}

void AkIDStringHash::Term()
{
	m_list.Term();
}

AkIDStringHash::AkStringHash::Item * AkIDStringHash::Preallocate( AkUInt32 in_ID, const char* in_pszString )
{
	// Allocate string 'in-place' in the hash item structure to save memory blocks.

	AkUInt32 sizeofString = (AkUInt32) ( sizeof(char)*(strlen(in_pszString) + 1) );
	AkUInt32 sizeofItem = offsetof( AkStringHash::Item, Assoc.item ) + sizeofString;
	AkStringHash::Item * pItem = (AkStringHash::Item *) AkAlloc( m_list.PoolId(), sizeofItem );
	if ( !pItem ) 
		return NULL;

	pItem->Assoc.key = in_ID;
	strcpy( &( pItem->Assoc.item ), in_pszString );

	return pItem;
}

void AkIDStringHash::FreePreallocatedString( AkStringHash::Item * in_pItem )
{
	AkFree( m_list.PoolId(), in_pItem );
}

AKRESULT AkIDStringHash::Set( AkUInt32 in_ID, const char* in_pszString )
{
	// Do not try to unset previous, as external calls to Set/Unset should be balanced. (performance issue)

	if( in_pszString != NULL )
	{
		AkStringHash::Item * pItem = Preallocate( in_ID, in_pszString );
		if ( pItem == NULL )
			return AK_Fail;
        
		m_list.Set( pItem );
	}

	return AK_Success;
}

AKRESULT AkIDStringHash::Set( AkStringHash::Item * in_pItem )
{
	m_list.Set( in_pItem );

	return AK_Success;
}


void AkIDStringHash::Unset( AkUInt32 in_ID )
{
	m_list.Unset( in_ID );
}

char * AkIDStringHash::GetStr( AkUInt32 in_ID )
{
	return m_list.Exists( in_ID );
}
