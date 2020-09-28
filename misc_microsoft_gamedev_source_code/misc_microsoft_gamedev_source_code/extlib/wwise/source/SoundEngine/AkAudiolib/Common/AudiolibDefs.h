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
// AudiolibDefs.h
//
// AkAudioLib Internal defines
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_DEFS_H_
#define _AUDIOLIB_DEFS_H_

#include "PlatformAudiolibDefs.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

// upper engine
extern AkMemPoolId g_DefaultPoolId;

// lower engine
extern AkMemPoolId g_LEngineDefaultPoolId;
//----------------------------------------------------------------------------------------------------
// (Upper) Renderer.
//----------------------------------------------------------------------------------------------------
#define RENDERER_DEFAULT_POOL_ID				(g_DefaultPoolId)

//----------------------------------------------------------------------------------------------------
// Sound.
//----------------------------------------------------------------------------------------------------
#define	SOUND_BUFFER_POOL_ID					(g_DefaultPoolId)

//----------------------------------------------------------------------------------------------------
// Sources.
//----------------------------------------------------------------------------------------------------
// Invalid ID (AkPluginID for plugins).
#define AK_INVALID_SOURCE_ID                    (-1)

//----------------------------------------------------------------------------------------------------
// SrcBank.
//----------------------------------------------------------------------------------------------------
#define BANK_SRC_BUFFER_POOL_ID					(g_LEngineDefaultPoolId)
#define BANK_BUFFER_FRAMES						(LE_MAX_FRAMES_PER_BUFFER)

//----------------------------------------------------------------------------------------------------
// FX.
//----------------------------------------------------------------------------------------------------
#define PHYS_MODEL_BUFFER_POOL_ID				(g_LEngineDefaultPoolId)
#define FX_BUFFER_POOL_ID						(g_LEngineDefaultPoolId)

//----------------------------------------------------------------------------------------------------
// SrcFile.
//----------------------------------------------------------------------------------------------------

#define AK_MARKERS_POOL_ID	                    (g_LEngineDefaultPoolId)

//----------------------------------------------------------------------------------------------------
// Paths
//----------------------------------------------------------------------------------------------------

// max number of users a path can have (<=255)
#define AK_PATH_USERS_LIST_SIZE					(8)

//----------------------------------------------------------------------------------------------------
// Transitions
//----------------------------------------------------------------------------------------------------

// max number of users a transition can have (<=255)
#define	AK_TRANSITION_USERS_LIST_SIZE			(255)

// how often variables are updated (ms)
#define AK_POSITION_UPDATE_RATE					(33)			// 30 Hz
#define AK_ONEOVER_POSITION_UPDATE_TIME			(1.0f / AK_POSITION_UPDATE_RATE * 1000.0f) // seconds / distance

//----------------------------------------------------------------------------------------------------
//  Voice Mgr
//----------------------------------------------------------------------------------------------------

#define AK_DEFAULT_LISTENER				(0)
#define AK_DEFAULT_LISTENER_MASK		(0x00000001)
#define AK_ALL_LISTENERS_MASK			(0x000000ff)
#define AK_ALL_REMOTE_LISTENER_MASK		(0x0000000f)


//----------------------------------------------------------------------------------------------------
// SwitchCntr
//----------------------------------------------------------------------------------------------------
#define AK_DEFAULT_SWITCH_LIST_SIZE		4
#define AK_DEFAULT_SUB_SWITCH_LIST_SIZE	4
#define AK_MAX_SWITCH_LIST_SIZE			AK_NO_MAX_LIST_SIZE// should be infinite, or max possible

#define AK_MIN_CONTINUOUS_ITEM_IN_SWLIST	16
#define AK_MAX_CONTINUOUS_ITEM_IN_SWLIST	AK_NO_MAX_LIST_SIZE// should be infinite, or max possible

//----------------------------------------------------------------------------------------------------
// Pipeline sample types
//----------------------------------------------------------------------------------------------------
#define AUDIOSAMPLE_FLOAT_MIN -1.f
#define AUDIOSAMPLE_FLOAT_MAX 1.f
#define AUDIOSAMPLE_SHORT_MIN -32768.f
#define AUDIOSAMPLE_SHORT_MAX 32767.f
#define AUDIOSAMPLE_UCHAR_MAX 255.f
#define AUDIOSAMPLE_UCHAR_MIN 0.f

//----------------------------------------------------------------------------------------------------
// Generic processing utilities
//----------------------------------------------------------------------------------------------------

#define SNAPTO4FRAMESUP(__NumFrames) \
	((((__NumFrames)+3) / 4) * 4); \

#endif //_AUDIOLIB_DEFS_H_

