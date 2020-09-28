//	VinceUtil.h : Utility routines for Vince code
//
//	Created 2004/04/30 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#ifdef _XBOX
  #include <xtl.h>
  #include <xbdm.h>
  #include <cstdio>
#else
  #define WIN32_LEAN_AND_MEAN
  #include <Windows.h>
  #include <stdio.h>
#endif

#ifdef _DEBUG
  #define TRACE Trace
#else
  #define TRACE
#endif

#include <direct.h>

namespace Vince
{
	// Maintain a structure for holding path information to the VINCE input
	// and output folders. Initialize only once
	class SVincePaths
	{
		public:
			const char* SourceFolder;
			const char* DestinationFolder;
	};
	static SVincePaths VincePaths;

	// Machine and file access routines

	void InitializePaths();
	void ClearPaths();
	const char* GetVinceRoot();
	const char* GetVinceAppRoot();
	void SetVinceSourceFolder(const char* cstrFolder);
	void SetVinceDestinationFolder(const char* cstrFolder);
	void VerifyDestinationFolderWriteable();
	bool VinceFileExists( const char* cstrFilename, bool fOutput );
	const char* GetFullFileName(const char* cstrFilename, bool fOutput);
	FILE* VinceFileOpen(const char* cstrFilename, const char* cstrFlags, bool fOutput);
	const char* GetMachineName();
	const char* GetCurrentDateTimeString();
	void ShowVinceTimer(const char* message);
    void Trace(const char* szMessage, ...);
}