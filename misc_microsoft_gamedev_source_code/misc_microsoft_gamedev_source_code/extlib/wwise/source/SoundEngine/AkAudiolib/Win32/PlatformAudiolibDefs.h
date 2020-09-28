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
// PlatformAudiolibDefs.h
//
// AkAudioLib Internal defines
//
//////////////////////////////////////////////////////////////////////
#ifndef _PLATFORM_AUDIOLIB_DEFS_H_
#define _PLATFORM_AUDIOLIB_DEFS_H_

//----------------------------------------------------------------------------------------------------
// Voice manager
//----------------------------------------------------------------------------------------------------
#define AK_NUM_VOICE_REFILL_FRAMES				(1024)
#define AK_DEFAULT_NUM_REFILLS_IN_VOICE_BUFFER	(4)

#define LE_MAX_FRAMES_PER_BUFFER				(AK_NUM_VOICE_REFILL_FRAMES)

//----------------------------------------------------------------------------------------------------
// Paths
//----------------------------------------------------------------------------------------------------
#define DEFAULT_MAX_NUM_PATHS                   (255)

// max number of vertices in the path buffer (<=255)
#define AK_PATH_VERTEX_LIST_SIZE				(64)

// max number of users a path can have (<=255)
#define AK_PATH_USERS_LIST_SIZE					(8)

//----------------------------------------------------------------------------------------------------
// Transitions
//----------------------------------------------------------------------------------------------------
#define DEFAULT_MAX_NUM_TRANSITIONS             (255)

// max number of users a transition can have (<=255)
#define	AK_TRANSITION_USERS_LIST_SIZE			(255)

// Below this volume, a node won't bother sending out data to the mix bus
#define AK_OUTPUT_THRESHOLD						(0.5f/32768.0f)

//----------------------------------------------------------------------------------------------------
// Bank manager platform-specific
//----------------------------------------------------------------------------------------------------
#define AK_BANK_MGR_THREAD_STACK_BYTES			(AK_DEFAULT_STACK_SIZE)
#define AK_THREAD_BANK_MANAGER_PRIORITY			(AK_THREAD_PRIORITY_NORMAL)

#endif //_PLATFORM_AUDIOLIB_DEFS_H_
