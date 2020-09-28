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

////////////////////////////////////////////////////////////////////////
// AkVorbisInfo.h
//
// Structures common to sound engine and Wwise encoder
//
///////////////////////////////////////////////////////////////////////

#ifndef _AK_VORBISINFO_H_
#define _AK_VORBISINFO_H_

// this file is also used in AkFileParser.cpp
#include <AK/SoundEngine/Common/AkTypes.h>

struct AkVorbisLoopInfo
{
	AkUInt32 dwLoopStartFrameOffset;	// Loop start PCM frame offset into its packet
	AkUInt32 dwLoopStartPacketOffset;	// File offset of packet containing loop start sample
	AkUInt32 dwLoopEndFrameOffset;		// Loop end PCM frame offset into its packet
	AkUInt32 dwLoopEndPacketOffset;		// File offset of packet following the one containing loop end sample
};

struct AkVorbisSeekTableItem
{
	AkUInt32 dwPacketFirstPCMFrame;		// Granule position (first PCM frame) of Ogg packet
	AkUInt32 dwPacketFileOffset;		// File offset of packet in question
};

struct AkVorbisInfo
{
	AkUInt32 dwTotalPCMFrames;			// Total number of encoded PCM frames
	AkUInt32 dwTotalNumPackets;			// Total number of packets encoded
	AkVorbisLoopInfo LoopInfo;			// Looping related information
	AkUInt32 dwSeekTableSize;			// Size of seek table items (0 == not present)
	AkUInt32 dwVorbisDataOffset;		// Offset in data chunk to first audio packet
	AkUInt32 dwMaxPacketSize;			// Maximum packet size
	AkUInt32 dwDecodeAllocSize;			// Decoder expected allocation size (PS3 specific)
};

struct OggPacketHeader
{
	AkUInt32 dwPacketSize;
	AkUInt32 dwGranulePos;
};

#endif // _AK_VORBISINFO_H_
