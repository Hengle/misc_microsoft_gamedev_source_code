//	VinceUtil.cpp : Utility routines for Vince code
//
//	Created 2004/04/30 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "StringUtil.h"
#include "VinceUtil.h"

#ifndef _XBOX
#include <stdlib.h>
#endif

#include <cassert>

namespace Vince
{
	// Initialize paths to VINCE folders. The implementation is quite different
	// for XBox and PC implementations
	void InitializePaths()
	{
#ifdef _XBOX	// Xbox or Xenon
	// We always use the VINCE subfolder of the root path for XBOX input
	#if _XBOX_VER >= 200 // Xenon
		VincePaths.SourceFolder = SAFE_COPY("Game:\\Vince\\");
		VincePaths.DestinationFolder = SAFE_COPY("Game:\\Vince\\");
	#else				 // Xbox
		VincePaths.SourceFolder = SAFE_COPY("D:\\Vince\\");
		VincePaths.DestinationFolder = SAFE_COPY("D:\\Vince\\");
	#endif
	VerifyDestinationFolderWriteable();
#else
		// For the PC, we first look for the Vince.ini in a subfolder of the root

		VincePaths.SourceFolder = GetVinceRoot();
		if ( !VinceFileExists("Vince.ini", false) )
		{
			// No Vince.ini file in Vince subfolder of executable.
			// Set both paths to Application Data path
			SAFE_DELETE_ARRAY(VincePaths.SourceFolder);
			VincePaths.SourceFolder = GetVinceAppRoot();
			SetVinceDestinationFolder(VincePaths.SourceFolder);
		}
		else
		{
			// We start by setting destination path to be the same as source path
			VincePaths.DestinationFolder = SAFE_COPY(VincePaths.SourceFolder);
			
			// The source folder is okay, but make sure we can write to it
			VerifyDestinationFolderWriteable();
		}
#endif
	}

	// Makes sure current destination folder is writeable;
	// otherwise it relocates to safe location
	void VerifyDestinationFolderWriteable()
	{
		FILE* fileTest = VinceFileOpen("TestCreate.tmp", "w+", true);
		if ( !fileTest )
		{
			// If not, set to the app data folder.
			// This will differ for PC and Xbox
			SetVinceDestinationFolder( GetVinceAppRoot() );
		}
		else
		{
			// Get rid of temp file and leave as is
			fclose( fileTest );
			const char* cstrTempFile = GetFullFileName("TestCreate.tmp", true);
			DeleteFile(cstrTempFile);
			SAFE_DELETE_ARRAY(cstrTempFile);
		}
	}

	// Deallocates memory for folder paths
	void ClearPaths()
	{
		SAFE_DELETE_ARRAY(VincePaths.SourceFolder);
		SAFE_DELETE_ARRAY(VincePaths.DestinationFolder);
	}

	// Used only by PC versions, this retrieves the path to the Vince subfolder of the
	// executable.
	const char* GetVinceRoot()
	{
#ifdef _XBOX
		return NULL;
#else
		// We need enough room in the buffer to add "Vince\"
		char buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH - 7);
		buffer[MAX_PATH - 7] = '\0';
		size_t iLastSlash = 0;
		for (iLastSlash = strlen(buffer); iLastSlash > 0; iLastSlash--)
		{
			if ( '\\' == buffer[iLastSlash] )
				break;
		}

		if ( iLastSlash < 2 )
		{
			return SAFE_COPY("Vince\\");
		}
		else
		{
			strncpy_s( buffer + iLastSlash + 1, MAX_PATH - iLastSlash - 1, "Vince\\", 6);
			buffer[iLastSlash + 7] = '\0';
			return SAFE_COPY(buffer);
		}
#endif
	}

	const char* GetVinceAppRoot()
	{
#ifdef _XBOX
		// Just in case we are in DVD emulation mode, we may need
		// to write log data to the T: virtual drive

		return SAFE_COPY( "T:\\VINCE\\" );
#else
		char buffer[MAX_PATH];

		// We get the environmental variable for the APPDATA path, which should
		// always return non-null, but we check anyway.
        size_t requiredSize = 0;
        if (0 != getenv_s(&requiredSize, buffer, MAX_PATH - 24, "APPDATA"))
		{
			buffer[0] = '\0';
		}
		else
		{
			buffer[MAX_PATH - 24] = '\0';
		}
		strncat_s( buffer, MAX_PATH, "\\Microsoft Games\\Vince\\", 23 );
		return SAFE_COPY(buffer);
#endif
	}

	void SetVinceSourceFolder(const char* cstrFolder)
	{
		SAFE_DELETE_ARRAY(VincePaths.SourceFolder);
		VincePaths.SourceFolder = SAFE_COPY(cstrFolder);
	}

	void SetVinceDestinationFolder(const char* cstrFolder)
	{
		SAFE_DELETE_ARRAY(VincePaths.DestinationFolder);
		VincePaths.DestinationFolder = SAFE_COPY(cstrFolder);

#ifndef _XBOX
        // Create the directory in stages
        char *slash = strchr((char *)cstrFolder, '\\');
        while(slash != NULL) 
        {
            *slash = '\0';
            CreateDirectory(cstrFolder, NULL);
            *slash = '\\';
            slash = strchr(slash + 1, '\\');
        }
        CreateDirectory(cstrFolder, NULL);
#endif
	}

	bool VinceFileExists( const char* cstrFilename, bool fOutput )
	{
		// Check for valid argument
		if( NULL == cstrFilename )
		{
			return false;
		}

		// Default path is the filename itself as a fully qualified path
		const char* cstrTestFilename = GetFullFileName(cstrFilename, fOutput);

		// Try to open the file
		HANDLE hFile = CreateFile( cstrTestFilename, GENERIC_READ, FILE_SHARE_READ, NULL, 
								OPEN_EXISTING, 0, NULL );
		SAFE_DELETE_ARRAY(cstrTestFilename);

		if( INVALID_HANDLE_VALUE == hFile )
		{
			// Return error
			return false;
		}

		// Found the file. Close the file and return
		CloseHandle( hFile );
		return true;
	}

	// Add drive and folder to base file name.
	const char* GetFullFileName(const char* cstrFilename, bool fOutput)
	{
		// Make sure filename is specified

		if ( NULL == cstrFilename )
			return NULL;
		
		char strFullname[MAX_PATH];
		strFullname[0] = '\0';

		// Since drive paths are no longer single letters, but possibly
		// "GAME:" or "DEVKIT:", we look for a colon anywhere in the name
		// so long as there are at least 3 following characters.

		bool fFullPath = false;

		for ( int iChar = 0;  strlen(cstrFilename) - iChar > 2; iChar++ )
		{
			// Check for the ':' character to see if the filename is a fully
			// qualified path.

			if ( ':' == cstrFilename[iChar] )
			{
				fFullPath = true;
				break;
			}
		}

#ifndef _XBOX
			// An additional check for a share path is only valid for the PC.

		if ( (strlen(cstrFilename) > 3) && 
			 ('\\' == cstrFilename[0])  &&
			 ('\\' == cstrFilename[1]) )
			fFullPath = true;
#endif




		if( !fFullPath )
		{
			if (fOutput)
			{
				if ( NULL != VincePaths.DestinationFolder)
					strncpy_s(strFullname, MAX_PATH, VincePaths.DestinationFolder, MAX_PATH - 1);
			}
			else
			{
				if ( NULL != VincePaths.SourceFolder)
					strncpy_s(strFullname, MAX_PATH, VincePaths.SourceFolder, MAX_PATH - 1);
			}

			// Make sure the path terminates with a "\"
			// The source or destination folder (as appropriate) should never be null,
			// but if they were, we could get a bounds overflow when checking for the
			// terminating "\". Therefore we check first for safety, as suggested by
			// Prefix analysis. We will probably wind up with a bogus path name.

			size_t length = strlen(strFullname);
			if (length > 0)
			{
				if ( strFullname[length - 1] != '\\' )
					strncat_s(strFullname, MAX_PATH, "\\", 1);
			}
		}
		strncat_s(strFullname, MAX_PATH, cstrFilename, strlen(cstrFilename) );
		return SAFE_COPY(strFullname);
	}

	// Open file (in Vince folder if full path not specified)
	FILE* VinceFileOpen(const char* cstrFilename, const char* cstrFlags, bool fOutput)
	{
		// Make sure filename is specified

		if ( NULL == cstrFilename )
			return NULL;

		const char* cstrFullFileName = GetFullFileName(cstrFilename, fOutput);

		// The most likely reason for failure is the non-existence of the 
		// Vince subfolder. If this is the case, we will attempt to create it
		// and try again (but only for output files)

		FILE* fileResult = NULL;
        if (0 != fopen_s(&fileResult, cstrFullFileName, cstrFlags))
        {
            fileResult = NULL;
        }

		if (!fileResult && fOutput)
		{
			DWORD dwError = GetLastError();
			if ( 3 == dwError )		// Directory does not exist
			{
				// Try to create Vince subfolder
				if ( S_OK == _mkdir(VincePaths.DestinationFolder) )
				{
					// Try to open file once more
                    if (0 != fopen_s(&fileResult, cstrFullFileName, cstrFlags))
                    {
                        fileResult = NULL;
                    }
				}
			}
		}
		SAFE_DELETE_ARRAY(cstrFullFileName);
		return fileResult;
	}

	// Return machine name. Uses Xbox or PC dependent system call
	const char* GetMachineName()
	{
		DWORD dwBufLen;
		dwBufLen = 256;
		char buffer[256];
		buffer[0] = '\0';

		// The Xbox and PC version of the system function have
		// different return types, but the macros are defined
		// to resolve as boolean.
        bool success = false;

#ifdef _XBOX
  //DmGetXboxName is a debug-only function
  #ifdef _DEBUG
		success = (XBDM_NOERR == DmGetXboxName(buffer, &dwBufLen));
  #endif
#else
		success = (0 != GetComputerName(buffer, &dwBufLen));
#endif
		if (success)
		{
			return SAFE_COPY(buffer);
		}
		else
		{
			return SAFE_COPY("Unknown");
		}
	}

	// Format the current date and time into string pointer
	const char* GetCurrentDateTimeString()
	{
		char strDateTime[30];

		// Fetch date/time data
		SYSTEMTIME SystemTime;
		GetLocalTime( &SystemTime );

        _snprintf_s( strDateTime, 30, 29, "%d/%d/%d %d:%2.2d:%2.2d", SystemTime.wMonth,
                           SystemTime.wDay, SystemTime.wYear,
						   SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond );
		strDateTime[29] = '\0';

		return SAFE_COPY(strDateTime);
    }

	//-----------------------------------------------------------------------------
	// ShowVinceTimer()
	// Utility routine to assist in performance profiling
	//
	// Desc: Writes a message to the debug stream with a time stamp to identify
	//       elapsed time since last call. Very crude but adequate for rough testing
	//-----------------------------------------------------------------------------
	void ShowVinceTimer(const char* message)
	{
		static DWORD dwLastTickCount = 0;
		static DWORD dwTotalElapsed = 0;
		DWORD dwElapsed = 0;
		DWORD dwNewTickCount = GetTickCount();
		char buffer[80];

		if (0 == dwLastTickCount)
		{
			dwElapsed = 0;
		}
		else
		{
			dwElapsed = dwNewTickCount - dwLastTickCount;
		}
		dwTotalElapsed += dwElapsed;
		dwLastTickCount = dwNewTickCount;
		sprintf_s(buffer, 80, "%d %d %s \n", dwElapsed, dwTotalElapsed, message);
		OutputDebugString(buffer);
	}

}	// namespace
