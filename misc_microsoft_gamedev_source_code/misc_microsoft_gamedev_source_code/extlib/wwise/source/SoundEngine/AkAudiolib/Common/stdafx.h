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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef _AUDIOLIB_STDAFX_H_
#define _AUDIOLIB_STDAFX_H_

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400	// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#include "AkPrivateTypes.h"
#include <AK/Tools/Common/AkAssert.h>
// TODO: reference additional headers your program requires here

#include <stdio.h>

#ifdef RVL_OS
#include <string.h>	// for memcpy
#endif

#endif //_AUDIOLIB_STDAFX_H_
