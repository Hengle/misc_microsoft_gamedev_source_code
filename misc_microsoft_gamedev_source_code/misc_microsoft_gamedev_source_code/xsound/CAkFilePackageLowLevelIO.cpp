//////////////////////////////////////////////////////////////////////
//
// AkFilePackageLowLevelIO.h
//
// CAkFilePackageLowLevelIOextends the default Low-Level I/O 
// implementation by providing the ability to reference files that are
// part of a file package.
// 
// Supports loading and parsing of file packages that were created with the 
// AkFilePackager utility app (located in ($WWISESDK)/samples/FilePackager/). 
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
// The map of soundbank names is optionally stored in the header. When this is the case, 
// users can load banks through the string overload of LoadBank().
//
// The file package searched first, then the default implementation is used.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "common.h"
//#include "stdafx.h"
#include "CAkFilePackageLowLevelIO.h"
#include <malloc.h>

// Defines.
// ------------------------------------------------------------

CAkFilePackageLowLevelIO::CAkFilePackageLowLevelIO()
:m_hBaseFile( NULL )
{
}

CAkFilePackageLowLevelIO::~CAkFilePackageLowLevelIO()
{
}

// Initialize/terminate.
AKRESULT CAkFilePackageLowLevelIO::Init( )
{
	ResetLUT();
    return CAkDefaultLowLevelIO::Init( );
}
void CAkFilePackageLowLevelIO::Term( )
{
    ResetLUT();
    CAkDefaultLowLevelIO::Term();
}

// Override Open (string): Search file in LUT first. If it cannot be found, use base class services.
AKRESULT CAkFilePackageLowLevelIO::Open( 
    AkLpCtstr       in_pszFileName,     // File name.
    AkOpenMode      in_eOpenMode,       // Open mode.
    AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
    AkFileDesc &    out_fileDesc        // Returned file descriptor.
    )
{
    // If the file is an AK sound bank, try to find the identifier in the lookup table first.
    if ( in_eOpenMode == AK_OpenModeRead 
		&& in_pFlags 
		&& in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC 
		&& in_pFlags->uCodecID == AKCODECID_BANK )
	{
		AkFileID fileID = m_lut.GetSoundBankID( in_pszFileName );
		if ( fileID != AK_INVALID_FILE_ID )
		{
			// Found the ID in the lut. 
			return FindPackagedFile( fileID, in_pFlags, out_fileDesc );
		}
    }

    // It is not a soundbank, or it is not in the file package LUT. Use default implementation.
    return CAkDefaultLowLevelIO::Open( in_pszFileName,
                                       in_eOpenMode,
                                       in_pFlags,
                                       out_fileDesc);
}

// Override Open (ID): Search file in LUT first. If it cannot be found, use base class services.
AKRESULT CAkFilePackageLowLevelIO::Open( 
    AkFileID        in_fileID,          // File ID.
    AkOpenMode      in_eOpenMode,       // Open mode.
    AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
    AkFileDesc &    out_fileDesc        // Returned file descriptor.
    )
{
    // Try to find the identifier in the lookup table first.
    if ( in_eOpenMode == AK_OpenModeRead 
		&& in_pFlags 
		&& in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC 
		&& FindPackagedFile( in_fileID, in_pFlags, out_fileDesc ) == AK_Success )
    {
        // File found. Return now.
        return AK_Success;
    }

    // If it the fileID is not in the LUT, perform standard path concatenation logic.
    return CAkDefaultLowLevelIO::Open( in_fileID,
                                       in_eOpenMode,
                                       in_pFlags,
                                       out_fileDesc);
}

// Override Close: Do not close handle if file descriptor is part of the current packaged file.
AKRESULT CAkFilePackageLowLevelIO::Close(
    const AkFileDesc & in_fileDesc      // File descriptor.
    )
{
	// Do not close handle if it is that of the packaged file (closed only in ResetFS()).
    if ( in_fileDesc.hFile != m_hBaseFile )
        return CAkDefaultLowLevelIO::Close( in_fileDesc );
    
	return AK_Success;
}

// Override Read: If current device uses blocking I/O and file descriptor is part of the current 
// packaged file, the file pointer always needs to be updated before reading from the storage
// device (note that packaged file scheme is more optimal when used along DeviceDVD - overlapped).
AKRESULT CAkFilePackageLowLevelIO::Read(
    AkFileDesc &    io_fileDesc,        // File descriptor.
    void *          out_pBuffer,        // Buffer to be filled with data.
    AkIOTransferInfo & io_transferInfo  // Platform-specific IO data transfer info. 
    )
{
    // Force io_transferInfo.bIsSequential to false if file handle is that of the base file.
    if ( io_fileDesc.hFile == m_hBaseFile )
        io_transferInfo.bIsSequential = false;
    return CAkDefaultLowLevelIO::Read( io_fileDesc,
                                       out_pBuffer,
                                       io_transferInfo );
}

// Override GetBlockSize: Get the block size of the LUT if a file package is loaded.
AkUInt32 CAkFilePackageLowLevelIO::GetBlockSize(
    const AkFileDesc &  in_fileDesc     // File descriptor.
    )
{
	if ( in_fileDesc.hFile == m_hBaseFile )
	{
		// This file is part of a package. At Open(), we used the 
		// AkFileDesc.pCustomParam field to store the entry of the LUT.
		AKASSERT( in_fileDesc.pCustomParam );
		return ((CAkFilePackageLUT::AkFileEntry*)in_fileDesc.pCustomParam)->uBlockSize;
	}
	return CAkDefaultLowLevelIO::GetBlockSize( in_fileDesc );
}

// Override SetDevice: Reset LUT.
AKRESULT CAkFilePackageLowLevelIO::SetDevice(
    AkDeviceSettings & in_deviceSettings
    )
{
	// Destroy current file table (if loaded).
    // Table will have to be reloaded.
    ResetLUT();
    return CAkDefaultLowLevelIO::SetDevice( in_deviceSettings );
}

// Override SetLangSpecificDirName: Also need to set current language based on the language directoy name.
AKRESULT CAkFilePackageLowLevelIO::SetLangSpecificDirName(
    AkLpCtstr   in_pszDirName
    )
{
    AKRESULT eResult = CAkDefaultLowLevelIO::SetLangSpecificDirName( in_pszDirName );
    if ( eResult == AK_Success )
    {
        // If a file LUT is loaded, use the language directory name to set up the
        // current language ID for correct language specific file mapping.
        return SetLanguageLUT();
    }
    return eResult;
}

// Reset file package LUT.
// Close file package system handle.
void CAkFilePackageLowLevelIO::ResetLUT()
{
	m_lut.ClearLUT();

	if ( m_hBaseFile )
	{
		::CloseHandle( m_hBaseFile );
		m_hBaseFile = NULL;
	}
}

// Searches the LUT to find the file data associated with the FileID.
// Returns AK_Success if the file is found.
AKRESULT CAkFilePackageLowLevelIO::FindPackagedFile( 
    AkFileID        in_fileID,          // File ID.
    AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
    AkFileDesc &    out_fileDesc        // Returned file descriptor.
    )
{
	AKASSERT( in_pFlags );
	const CAkFilePackageLUT::AkFileEntry * pEntry = m_lut.LookupFile( in_fileID, in_pFlags );

	if ( pEntry )
	{
		// Fill file descriptor.
        out_fileDesc.deviceID   = m_curDeviceID;
        out_fileDesc.hFile      = m_hBaseFile;
        out_fileDesc.iFileSize	= pEntry->iFileSize;
        out_fileDesc.uSector	= pEntry->uStartBlock;
		// NOTE: We use the uCustomParam to remember the LUT entry.
		// This is useful when we want to retrieve this file's block size.
        out_fileDesc.pCustomParam = (void*)pEntry;
        out_fileDesc.uCustomParamSize = 0;
        return AK_Success;
    }
    return AK_FileNotFound;
}

// File package loading:
// Opens a package file, parses its header, fills LUT.
// Overrides of Open() will search FileIDs in LUT first, then use default Low-Level IO services 
// if they cannot be found.
// Only one file package can be loaded at a time. The LUT is reset each time LoadFilePackage() is called.
// Note: The file handle is first opened for blocking I/O. 
// If the current device is "DeviceDVD", it will be closed after parsing the header, 
// and reopened according to the current device settings.
AKRESULT CAkFilePackageLowLevelIO::LoadFilePackage(
    AkLpCtstr   in_pszFilePackageName
    )
{
	// Erase any table previously loaded.
    ResetLUT();

	// Open package file.
	// Use Low-Level IO basic services (path concatenation) to find full file path,
    // but not to open and read packaged file: need to read blocking and buffered.
    AkUInt32 uSector;
    AkTChar szFullFilePath[AK_MAX_PATH];
    AKRESULT eRes = GetFullFilePath(
        in_pszFilePackageName,
        NULL,
        uSector,
        szFullFilePath
        );  
    if ( eRes != AK_Success )
        return eRes;
    eRes = OpenFile( szFullFilePath,
                     AK_OpenModeRead,
                     true,
                     false,
                     m_hBaseFile );
    if ( eRes != AK_Success )
        return eRes;

	// Read header chunk definition.
	struct AkFilePackageHeader
	{
		AkUInt32 uFileFormatTag;
		AkUInt32 uHeaderSize;
	};
	AkFilePackageHeader uFileHeader;
	AkUInt32 uSizeRead;
	if ( !::ReadFile( m_hBaseFile, &uFileHeader, sizeof(AkFilePackageHeader), &uSizeRead, NULL )
		|| uSizeRead < sizeof(AkFilePackageHeader) )
	{
		AKASSERT( !"Could not read file package" );
        return AK_Fail;
	}

	if ( uFileHeader.uFileFormatTag != AKPK_FILE_FORMAT_TAG 
		|| 0 == uFileHeader.uHeaderSize )
	{
		AKASSERT( !"Invalid file package header" );
        return AK_Fail;
	}


	// Allocate LUT.
	// NOTE: The header size excludes the AKPK_HEADER_CHUNK_DEF_SIZE.
	AkUInt8 * pFilePackageHeader = (AkUInt8*)m_lut.PreAllocate( uFileHeader.uHeaderSize + AKPK_HEADER_CHUNK_DEF_SIZE );
	if ( !pFilePackageHeader )
	{
		AKASSERT( !"Could not allocate file package LUT" );
        return AK_Fail;
	}

	// Stream in whole header.
	if ( !::ReadFile( m_hBaseFile, pFilePackageHeader + AKPK_HEADER_CHUNK_DEF_SIZE, uFileHeader.uHeaderSize, &uSizeRead, NULL ) ||
         uSizeRead < uFileHeader.uHeaderSize )
    {
		AKASSERT( !"Could not read file package" );
        return AK_Fail;
	}

	// Parse LUT.
	eRes = m_lut.CreateLUT( pFilePackageHeader, uFileHeader.uHeaderSize + AKPK_HEADER_CHUNK_DEF_SIZE );
	if ( eRes != AK_Success )
		return eRes;

    // If the current device is "Device DVD", close handle and reopen using 
    // OVERLAPPED I/O.
    if ( m_eCurDeviceType == DeviceDVD )
    {
        AKASSERT( m_hBaseFile );
        ::CloseHandle( m_hBaseFile );
        eRes = OpenFile( szFullFilePath,
                         AK_OpenModeRead,
                         false,
                         true,
                         m_hBaseFile );
        if ( eRes != AK_Success )
            return eRes;
    }
	
    // Use the current language path (if defined) to set the language ID, 
    // for language specific file mapping.
    return SetLanguageLUT();
}

// If a packaged file is loaded, this method uses the language-specific directory 
// name to compute a language name, and file the associated LanguageID defined in 
// the LUT.
// Returns AK_Success if a LanguageID is found, or if no packed file is loaded.
AKRESULT CAkFilePackageLowLevelIO::SetLanguageLUT()
{
	const size_t numChars = wcslen(m_szLangSpecificDirName);

    if ( numChars > 0 )
    {
        // A language specific directory is specified. Use its name to find
		// the language ID, as stored in the packed file header.

		// Remove trailing slash out of the language specific directory name.
		AkTChar * szLanguage = (AkTChar*)_alloca( numChars * sizeof(AkTChar) );
		if ( !szLanguage )
			return AK_Fail;
		
		memcpy( szLanguage, m_szLangSpecificDirName, numChars * sizeof(AkTChar) );
		szLanguage[numChars-1] = 0;
		return m_lut.SetCurLanguage( szLanguage );
	}
	return m_lut.SetCurLanguage( NULL );
}

