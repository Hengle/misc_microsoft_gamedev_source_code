//////////////////////////////////////////////////////////////////////
// 
// AkFilePackageLUT.cpp
//
// This class parses the header of file packages that were created with the 
// AkFilePackager utility app (located in ($WWISESDK)/samples/FilePackager/),
// and looks-up files at run-time.
// 
// The header of these file packages contains look-up tables that describe the 
// internal offset of each file it references, their block size (required alignment), 
// and their language. Each combination of AkFileID and Language ID is unique.
//
// The language was created dynamically when the package was created. The header 
// also contains a map of language names (strings) to their ID, so that the proper 
// language-specific version of files can be resolved. The language name that is stored
// matches the name of the directory that is created by the Wwise Bank Manager,
// except for the trailing slash.
//
// Copyright (c) 2007-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "common.h"
//#include "stdafx.h"
#include "CAkFilePackageLUT.h"

#if defined (WIN32) || defined (XBOX360)
#include <malloc.h> 								// For alloca().
#else
#include <alloca.h>
#endif

#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>	// For string hash.

#ifdef _DEBUG
	template<bool> struct AkCompileTimeAssert;
	template<> struct AkCompileTimeAssert<true> { };
	#define AK_STATIC_ASSERT(e) (AkCompileTimeAssert<(e) != 0>())
#else
	#define AK_STATIC_ASSERT(e)
#endif

CAkFilePackageLUT::CAkFilePackageLUT()
:m_poolID( AK_INVALID_POOL_ID )
,m_pToRelease( NULL )
,m_curLangID( AK_INVALID_LANGUAGE_ID )
,m_pLangMap( NULL )
,m_pSoundBanks( NULL )
,m_pStmFiles( NULL )
{
	AK_STATIC_ASSERT(sizeof(AkFileEntry) == 24);
}

CAkFilePackageLUT::~CAkFilePackageLUT()
{
}

// Creates a memory pool to contain the header of the file package. Returns its address.
void * CAkFilePackageLUT::PreAllocate( 
	AkUInt32			in_uHeaderSize,		// Size of file package header.
	AkUInt32			in_uBlockAlign /*= 0*/	// Alignment of memory block.
	)
{
	AKASSERT( m_poolID == AK_INVALID_POOL_ID &&
			  in_uHeaderSize > 0 );
	
	// Create memory pool and copy header.
	m_poolID = AK::MemoryMgr::CreatePool( NULL, in_uHeaderSize, in_uHeaderSize, AkMalloc | AkFixedSizeBlocksMode, in_uBlockAlign );
	if ( m_poolID == AK_INVALID_POOL_ID )
		return NULL;
	m_pToRelease = AK::MemoryMgr::GetBlock( m_poolID );
	return m_pToRelease;
}

// Create a new LUT from a packaged file header.
// It must have been allocated first (see PreAllocate).
AKRESULT CAkFilePackageLUT::CreateLUT(
	AkUInt8 *			in_pData,			// Header data.
	AkUInt32			in_uHeaderSize		// Size of file package header.
	)
{
	AKASSERT( m_poolID != AK_INVALID_POOL_ID && m_pToRelease );

	struct FileHeaderFormat
	{
		char				headerDefinition[AKPK_HEADER_CHUNK_DEF_SIZE];
		AkUInt32			uVersion;
		AkUInt32			uLanguageMapSize;
		AkUInt32			uSoundBanksLUTSize;
		AkUInt32			uStmFilesLUTSize;
	};
	FileHeaderFormat * pHeader = (FileHeaderFormat*)in_pData;

	// Check header size,
	if ( in_uHeaderSize < sizeof(FileHeaderFormat)
			+ pHeader->uLanguageMapSize
			+ pHeader->uSoundBanksLUTSize
			+ pHeader->uStmFilesLUTSize )
	{
		return AK_Fail;
	}

	// Check version.
	if ( pHeader->uVersion < AKPK_CURRENT_VERSION )
		return AK_Fail;

	// Get address of maps and LUTs.
	in_pData += sizeof(FileHeaderFormat);
	m_pLangMap		= (StringMap*)in_pData;
	in_pData += pHeader->uLanguageMapSize;
	m_pSoundBanks	= (FileLUT*)in_pData;
	in_pData += pHeader->uSoundBanksLUTSize;
	m_pStmFiles		= (FileLUT*)in_pData;
	
	return AK_Success;
}

// Clear the current LUT, destroy internal memory pool.
void CAkFilePackageLUT::ClearLUT()
{
	m_curLangID		= AK_INVALID_LANGUAGE_ID;
	m_pLangMap		= NULL;
	m_pSoundBanks	= NULL;
	m_pStmFiles		= NULL;
	
	ClearMemory();
}

// Find a file entry by ID.
const CAkFilePackageLUT::AkFileEntry * CAkFilePackageLUT::LookupFile(
	AkFileID			in_uID,			// File ID.
	AkFileSystemFlags * in_pFlags		// Special flags. Do not pass NULL.
	)
{
	AKASSERT( in_pFlags && in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC );

	if ( in_pFlags->uCodecID == AKCODECID_BANK 
		&& m_pSoundBanks
		&& m_pSoundBanks->HasFiles() )
	{
		return LookupFile( in_uID, m_pSoundBanks, in_pFlags->bIsLanguageSpecific );
	}
	else if ( m_pStmFiles && m_pStmFiles->HasFiles() )
	{
		// We assume that the file is a streamed audio file.
		return LookupFile( in_uID, m_pStmFiles, in_pFlags->bIsLanguageSpecific );
	}
	// No table loaded.
	return NULL;
}

// Helper: Find a file entry by ID.
const CAkFilePackageLUT::AkFileEntry * CAkFilePackageLUT::LookupFile(
	AkFileID			in_uID,					// File ID.
	const FileLUT *		in_pLut,				// LUT to search.
	bool				in_bIsLanguageSpecific	// True: match language ID.
	)
{
	const AkFileEntry * pTable	= in_pLut->FileEntries();

	AKASSERT( pTable && in_pLut->HasFiles() );
	AkUInt16 uLangID = in_bIsLanguageSpecific ? m_curLangID : AK_INVALID_LANGUAGE_ID;

	// Binary search. LUT items should be sorted by fileID, then by language ID.
	AkInt32 uTop = 0, uBottom = in_pLut->NumFiles()-1;
	do
	{
		AkInt32 uThis = ( uBottom - uTop ) / 2 + uTop; 
		if ( pTable[ uThis ].fileID > in_uID ) 
			uBottom = uThis - 1;
		else if ( pTable[ uThis ].fileID < in_uID ) 
			uTop = uThis + 1;
		else
		{
			// Correct ID. Check language.
			if ( pTable[ uThis ].uLanguageID > uLangID ) 
				uBottom = uThis - 1;
			else if ( pTable[ uThis ].uLanguageID < uLangID ) 
				uTop = uThis + 1;
			else
				return pTable + uThis;
		}
	}
	while ( uTop <= uBottom );

	return NULL;
}

// Set current language. 
// Fails if a package is loaded but the language string cannot be found.
// Returns AK_Success otherwise.
AKRESULT CAkFilePackageLUT::SetCurLanguage(
	AkLpCtstr			in_pszLanguage	// Language string.
	)
{
	m_curLangID = AK_INVALID_LANGUAGE_ID;
	if ( m_pLangMap && in_pszLanguage )
	{
		AkUInt16 uLangID = (AkUInt16)m_pLangMap->GetID( in_pszLanguage );
		if ( uLangID == AK_INVALID_UNIQUE_ID )
			return AK_Fail;
		m_curLangID = uLangID;
	}

	return AK_Success;
}

void RemoveFileExtension( AkTChar* in_pstring );// prototype to avoid warning
void RemoveFileExtension( AkTChar* in_pstring )
{
	while( *in_pstring != 0 )
	{
		if( *in_pstring == L'.' )
		{
			*in_pstring = 0;
			return;
		}
		++in_pstring;
	}
}

// Find a soundbank ID by its name. 
// Returns AK_INVALID_FILE_ID if no soundbank LUT is loaded.
AkFileID CAkFilePackageLUT::GetSoundBankID( 
	AkLpCtstr			in_pszBankName	// Soundbank name.
	)
{
	if ( m_pSoundBanks )
	{
		// Remove the file extension if it was used.
		AkUInt32 stringSize = (AkUInt32)wcslen( in_pszBankName );
		AkTChar* pStringWithoutExtension = (AkTChar *) alloca( (stringSize+1) * sizeof( AkTChar ) );
		wcscpy( pStringWithoutExtension, in_pszBankName );
		RemoveFileExtension( pStringWithoutExtension );
		
		// Hash
		return AK::SoundEngine::GetIDFromString( pStringWithoutExtension );
	}
	return AK_INVALID_FILE_ID;
}

void CAkFilePackageLUT::ClearMemory()
{
	if ( m_poolID != AK_INVALID_POOL_ID )
	{
		if ( m_pToRelease )
		{
			AK::MemoryMgr::ReleaseBlock( m_poolID, m_pToRelease );
			m_pToRelease = NULL;
		}
		AKVERIFY( AK::MemoryMgr::DestroyPool( m_poolID ) == AK_Success );
		m_poolID = AK_INVALID_POOL_ID;
	}
}

void _MakeLower( AkTChar* in_pString );
void _MakeLower( AkTChar* in_pString )
{
	size_t uStrlen = wcslen( in_pString );
	const AkTChar CaseDiff = L'a' - L'A';
	for( size_t i = 0; i < uStrlen; ++i )
	{
		if( in_pString[i] >= L'A' && in_pString[i] <= L'Z' )
		{
			in_pString[i] += CaseDiff;
		}
	}
}

AkUInt32 CAkFilePackageLUT::StringMap::GetID( AkLpCtstr in_pszString )
{
	// Make string lower case.
	AkTChar * pszLowerCaseString = (AkTChar*)alloca((wcslen(in_pszString)+1)*sizeof(AkTChar));
	AKASSERT( pszLowerCaseString );
	wcscpy(pszLowerCaseString, in_pszString);
	_MakeLower( pszLowerCaseString );

	// 'this' is m_uNumStrings. +1 points to the beginning of the StringEntry array.
	StringEntry * pTable = (StringEntry*)((AkUInt32*)this + 1);	
	
	// Binary search: strings are sorted (case sensitive).
	AkInt32 uTop = 0, uBottom = m_uNumStrings-1;
	do
	{
		AkInt32 uThis = ( uBottom - uTop ) / 2 + uTop; 
		AkTChar * pString = (AkTChar*)((AkUInt8*)this + pTable[ uThis ].uOffset);
		int iCmp = wcscmp( pString, pszLowerCaseString );
		if ( 0 == iCmp )
			return pTable[uThis].uID;
		else if ( iCmp > 0 )	//in_pTable[ uThis ].pString > pszLowerCaseString 
			uBottom = uThis - 1;
		else					//in_pTable[ uThis ].pString < pszLowerCaseString 
			uTop = uThis + 1;
	}
	while ( uTop <= uBottom );

	// ID not found.
	return AK_INVALID_UNIQUE_ID;
}
