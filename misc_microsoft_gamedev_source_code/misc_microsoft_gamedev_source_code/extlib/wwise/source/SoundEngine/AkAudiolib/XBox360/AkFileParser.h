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
// AkFileParser.h
//
// CAkFileParser defines the interface of the singleton access to
// file parsers in the AudioLib.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_FILE_PARSER_H_
#define _AK_FILE_PARSER_H_

struct AkAudioFormat;
struct AkMarkersHeader;
struct WaveFormatEx;
class CAkMarkers;

// For XMA.
#include "xauddefs.h"
#include "XMADecoder.h"
// IMPORTANT! Keep in sync with AudioFileConversion/XMAPlugin
struct AkXMACustHWLoopData
{
	DWORD   LoopStart;          // Loop start offset (in bits).
    DWORD   LoopEnd;            // Loop end offset (in bits).

    // Format for SubframeData: eeee ssss.
    // e: Subframe number of loop end point [0,3].
    // s: Number of subframes to skip before decoding and outputting at the loop start point [1,4].

    BYTE    SubframeData;       // Data for decoding subframes.  See above.
};
#pragma pack(push, 1)
struct AkXMA2CustomData
{
	AkUInt32        	uSeekTableOffset;
	AkXMACustHWLoopData loopData;
};
#pragma pack(pop)

//-----------------------------------------------------------------------------
// Name: class CAkFileParser
// Desc: XBox360 implementation of the file parsers of the AudioLib.
//-----------------------------------------------------------------------------

class CAkFileParser
{
public:
    // File parsing function.
    static AKRESULT Parse( 
		const void * in_pvBuffer,				// Buffer to be parsed.
        AkUInt32 in_ulBufferSize,               // Buffer size.
        WaveFormatEx * out_pAudioFormat,		// Returned audio format.
		AkUInt32 in_ulOutFormatSize,            // Buffer size for out_pAudioFormat ( for extended WAVEFORMATEX )
        CAkMarkers * out_pMarkers,   			// Markers. NULL if not wanted. (Mandatory for markers).
        AkUInt32 * out_pulLoopStart,            // Loop start position (in sample frames). NULL if not wanted.
        AkUInt32 * out_pulLoopEnd,              // Loop end position (in sample frames). NULL if not wanted.
		AkUInt32 * out_pulDataSize,				// Size of data portion of the file.
		AkUInt32 * out_pulDataOffset,			// Offset in file to the data.
		void * out_pFmtSpecificInfo,			// Additional format specific information (NULL if not required)
		AkUInt32 in_uFmtSpecificInfoSize		// Size of format specific information 
	);

private:
	CAkFileParser() {} // Not for instantiation
};

#endif //_AK_FILE_PARSER_H_
