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

#ifndef _CAK_FILE_PACKAGE_LOW_LEVEL_IO_H_
#define _CAK_FILE_PACKAGE_LOW_LEVEL_IO_H_

#include "CAkDefaultLowLevelIO.h"
#include "CAkFilePackageLUT.h"


//-----------------------------------------------------------------------------
// Name: class CAkFilePackageLowLevelIO.
// Desc: Extends default Low-level IO implementation with packaged file support.
//-----------------------------------------------------------------------------
class CAkFilePackageLowLevelIO : public CAkDefaultLowLevelIO
{
public:

    CAkFilePackageLowLevelIO();
    virtual ~CAkFilePackageLowLevelIO();

    // Initialize/terminate.
    AKRESULT Init();
    void Term();

	// Override Open (string): Search file in LUT first. If it cannot be found, use base class services.
	// Applies to AK soundbanks only.
    virtual AKRESULT Open( 
        AkLpCtstr       in_pszFileName,     // File name.
        AkOpenMode      in_eOpenMode,       // Open mode.
        AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
        AkFileDesc &    out_fileDesc        // Returned file descriptor.
        );

    // Override Open (ID): Search file in LUT first. If it cannot be found, use base class services.
    virtual AKRESULT Open( 
        AkFileID        in_fileID,          // File ID.
        AkOpenMode      in_eOpenMode,       // Open mode.
        AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
        AkFileDesc &    out_fileDesc        // Returned file descriptor.
        );

    // Override Close: Do not close handle if file descriptor is part of the current packaged file.
    virtual AKRESULT Close(
        const AkFileDesc & in_fileDesc      // File descriptor.
        );

    // Override Read: If current device uses blocking I/O (DeviceHDD) and file descriptor is part of the current 
    // packaged file, the file pointer always needs to be updated before reading from the storage
    // device (note that packaged file scheme is more optimal when used along DeviceDVD - overlapped).
    virtual AKRESULT Read(
        AkFileDesc &    io_fileDesc,        // File descriptor.
        void *          out_pBuffer,        // Buffer to be filled with data.
        AkIOTransferInfo & io_transferInfo  // Platform-specific IO data transfer info. 
        );

	// Override GetBlockSize: Get the block size of the LUT if a file package is loaded.
	virtual AkUInt32 GetBlockSize(
        const AkFileDesc &  in_fileDesc     // File descriptor.
        );

    // Override SetDevice: Also need to reset LUT.
    AKRESULT SetDevice(
        AkDeviceSettings & in_deviceSettings
        );

    // Override SetLangSpecificDirName: Also need to set current language based on the language directoy name.
    AKRESULT SetLangSpecificDirName(
        AkLpCtstr   in_pszDirName
        );

    // File package loading:
	// Opens a package file, parses its header, fills LUT.
	// Overrides of Open() will search FileIDs in LUT first, then use default Low-Level IO services 
	// if they cannot be found.
	// Only one file package can be loaded at a time. The LUT is reset each time LoadFilePackage() is called.
	// Note: The file handle is first opened for blocking I/O. 
	// If the current device is "DeviceDVD", it will be closed after parsing the header, 
	// and reopened according to the current device settings.
    AKRESULT LoadFilePackage(
        AkLpCtstr   in_pszFilePackageName
        );

private:

	// File package handling methods.
    // ------------------------------------------

    // Reset file package LUT.
	// Close file package system handle.
    void ResetLUT();

    // Searches the LUT to find the file data associated with the FileID.
    // Returns AK_Success if the file is found.
    AKRESULT FindPackagedFile( 
		AkFileID        in_fileID,          // File ID.
		AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
		AkFileDesc &    out_fileDesc        // Returned file descriptor.
		);

    // If a packaged file is loaded, this method uses the language-specific directory 
	// name to compute a language name, and file the associated LanguageID defined in 
	// the LUT.
    // Returns AK_Success if a LanguageID is found, or if no packaged file is loaded.
    AKRESULT SetLanguageLUT();

private:
	// The file package info is stored in this object.
	CAkFilePackageLUT	m_lut;

    // Package file system handle: base handle for file descriptors.
    HANDLE              m_hBaseFile;    
};

#endif //_CAK_FILE_PACKAGE_LOW_LEVEL_IO_H_
