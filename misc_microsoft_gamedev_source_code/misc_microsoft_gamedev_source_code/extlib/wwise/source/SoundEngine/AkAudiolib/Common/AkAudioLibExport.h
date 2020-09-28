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
// AkAudioLibExport.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_EXPORT_H_
#define _AUDIOLIB_EXPORT_H_

#include <AK/SoundEngine/Common/AkSoundEngineExport.h>

// WWISE exclusive Sound Engine export macro.
#if defined(AKSOUNDENGINE_STATIC) || defined(AK_OPTIMIZED)

    #define AKWWISEAUDIOLIB_API 

#else 
    // DLL with Wwise.
    #ifdef AKSOUNDENGINE_EXPORTS
	    #define AKWWISEAUDIOLIB_API __declspec(dllexport)
	#else
		#define AKWWISEAUDIOLIB_API __declspec(dllimport)
	#endif // Export
	
#endif

#endif
