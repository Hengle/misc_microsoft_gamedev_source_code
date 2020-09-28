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
// AudiolibLimitations.h
//
// AkAudioLib Internal Maximas defines
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_LIMITATIONS_H_
#define _AUDIOLIB_LIMITATIONS_H_

// Important note!
// AK_MAX_NUM_CHILD cannot go over 65535 without modifying the core format of the PlayHistory object, actually storing position on a WORD
#define AK_MAX_NUM_CHILD						65535
#define AK_MAX_NUM_PLAYLIST_ITEM				AK_MAX_NUM_CHILD
#define AK_MAX_NUM_CONVERSION_TABLE_NODE		64

#define AK_MAX_HIERARCHY_DEEP					32

#define AK_NUM_EFFECTS_PROFILER					(4)

#define AK_NUM_EFFECTS_BYPASS_ALL_FLAG			(AK_NUM_EFFECTS_PROFILER)

#ifdef RVL_OS
#define AK_NUM_EFFECTS_PER_OBJ					(1) 
#else
#define AK_NUM_EFFECTS_PER_OBJ					(AK_NUM_EFFECTS_PROFILER) 
#endif

#define AK_MAX_STRING_SIZE						128

// max number of vertices in the path buffer (<=255)
#define AK_PATH_VERTEX_LIST_SIZE				(64)
#define AK_MAX_NUM_PATH							(64)

// Maximum number of effects per object

#endif //_AUDIOLIB_LIMITATIONS_H_
