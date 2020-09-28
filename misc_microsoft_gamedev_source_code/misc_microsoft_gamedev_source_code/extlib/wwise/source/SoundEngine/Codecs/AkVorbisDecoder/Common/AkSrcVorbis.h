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
// AkSrcVorbis.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRC_VORBIS_H_
#define _AK_SRC_VORBIS_H_

#include "ogg/ogg.h"		// Ogg layer
#include "vorbis/ivorbiscodec.h"
#include "vorbis/codec_internal.h"

#include "AkVorbisInfo.h"	// Encoder information

enum AkVorbisDecoderState
{
	UNINITIALIZED			= -2,
	INITIALIZED				= -1,
	SEEKTABLEINTIALIZED		= 0,
	MAINHEADERDECODED		= 1,
	COMMENTHEADERDECODED	= 2,
	CODEBOOKHEADERDECODED	= 3,
};

#endif // _AK_SRC_VORBIS_H_
