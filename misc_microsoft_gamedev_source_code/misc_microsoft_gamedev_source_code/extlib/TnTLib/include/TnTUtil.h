//	TnTUtil.h : Utility routines for TnTLib
//
//	Created 2005/12/05 Rich Bonny <rbonny@microsoft.com>
//
//  Temporary repository for string and miscellaneous functions.
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.


#pragma once

#ifdef _XBOX
  #include <xtl.h>
  #include <xbdm.h>
  #include <cstdio>
#else
  #include <Windows.h>
  #include <stdio.h>
#endif

//-----------------------------------------------------------------------------
// Miscellaneous helper macros
//-----------------------------------------------------------------------------

// For copying, deleting and releasing objects
#undef SAFE_COPY
#define SAFE_COPY(p)		 TnT::MakeCopy(p)
#undef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p); (p)=NULL; } }
#undef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

namespace TnT
{
	// String functions
	char* MakeCopy( const char* cstrInput );
	size_t TrimLine( char* cstrInput );
	char* MakeTrimCopy( const char* cstrInput );
	wchar_t* MakeCopy( const wchar_t* cstrInput );
	const char* MakeSafeXMLchars( const char* cstrInput );
	char* MakeSingle(const WCHAR* wcstrSource);
	wchar_t* MakeWide(const char* cstrInput);
	void CopyWide( const char* strThin, wchar_t* strWide, size_t sizeMax );

}	// namespace