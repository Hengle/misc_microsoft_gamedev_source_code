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
// AkFileParserBase.h
//
// Definitions shared by both implementations of the file parser.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_FILE_PARSER_BASE_H_
#define _AK_FILE_PARSER_BASE_H_

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

static const AkFourcc fccEmpty	= AkmmioFOURCC(0, 0, 0, 0);
static const AkFourcc RIFXChunkId = AkmmioFOURCC('R', 'I', 'F', 'X');
static const AkFourcc RIFFChunkId = AkmmioFOURCC('R', 'I', 'F', 'F');
static const AkFourcc WAVEChunkId = AkmmioFOURCC('W', 'A', 'V', 'E');
static const AkFourcc fmtChunkId  = AkmmioFOURCC('f', 'm', 't', ' ');
static const AkFourcc xma2ChunkId = AkmmioFOURCC('X', 'M', 'A', '2');
static const AkFourcc xmaCustomChunkId = AkmmioFOURCC('X', 'M', 'A', 'c');
static const AkFourcc xma2SeekTable = AkmmioFOURCC('s', 'e', 'e', 'k');
static const AkFourcc dataChunkId = AkmmioFOURCC('d', 'a', 't', 'a');
static const AkFourcc factChunkId = AkmmioFOURCC('f', 'a', 'c', 't');
static const AkFourcc wavlChunkId = AkmmioFOURCC('w', 'a', 'v', 'l');
static const AkFourcc slntChunkId = AkmmioFOURCC('s', 'l', 'n', 't');
static const AkFourcc cueChunkId  = AkmmioFOURCC('c', 'u', 'e', ' ');
static const AkFourcc plstChunkId = AkmmioFOURCC('p', 'l', 's', 't');
static const AkFourcc LISTChunkId = AkmmioFOURCC('L', 'I', 'S', 'T');
static const AkFourcc adtlChunkId = AkmmioFOURCC('a', 'd', 't', 'l');
static const AkFourcc lablChunkId = AkmmioFOURCC('l', 'a', 'b', 'l');
static const AkFourcc noteChunkId = AkmmioFOURCC('n', 'o', 't', 'e');
static const AkFourcc ltxtChunkId = AkmmioFOURCC('l', 't', 'x', 't');
static const AkFourcc smplChunkId = AkmmioFOURCC('s', 'm', 'p', 'l');
static const AkFourcc instChunkId = AkmmioFOURCC('i', 'n', 's', 't');
static const AkFourcc rgnChunkId  = AkmmioFOURCC('r', 'g', 'n', ' ');
static const AkFourcc JunkChunkId = AkmmioFOURCC('J', 'U', 'N', 'K');
static const AkFourcc vorbChunkId = AkmmioFOURCC('v', 'o', 'r', 'b');
static const AkFourcc WiiHChunkID = AkmmioFOURCC('W', 'i', 'i', 'H');

static const AkUInt8 HAVE_FMT	= 0x01;
static const AkUInt8 HAVE_DATA	= 0x02;
static const AkUInt8 HAVE_CUES	= 0x04;
static const AkUInt8 HAVE_SMPL	= 0x08;
static const AkUInt8 HAVE_VORB	= 0x10;
static const AkUInt8 HAVE_WIIH	= 0x20;

//-----------------------------------------------------------------------------
// Structs.
//-----------------------------------------------------------------------------

struct ChunkHeader
{
	AkFourcc	ChunkId;
	AkUInt32		dwChunkSize;
};

#pragma pack(push, 1)

// This is a copy of PCMWAVEFORMAT
struct PCMWaveFormat
{	
	AkUInt16  	wFormatTag;
	AkUInt16  	nChannels;
	AkUInt32  	nSamplesPerSec;
	AkUInt32  	nAvgBytesPerSec;
	AkUInt16  	nBlockAlign;
	AkUInt16  	wBitsPerSample;
};

#pragma pack(pop)

#pragma pack(push, 1)

// This is a copy of WAVEFORMATEX
struct WaveFormatEx
{	
	AkUInt16  	wFormatTag;
	AkUInt16  	nChannels;
	AkUInt32  	nSamplesPerSec;
	AkUInt32  	nAvgBytesPerSec;
	AkUInt16  	nBlockAlign;
	AkUInt16  	wBitsPerSample;
	AkUInt16    cbSize;	// size of extra chunk of data, after end of this struct
};

struct WaveFormatExtensible : public WaveFormatEx
{
	AkUInt16    wSamplesPerBlock;
	AkUInt32    dwChannelMask;
};

#pragma pack(pop)

struct CuePoint
{
	AkUInt32   dwIdentifier;
	AkUInt32   dwPosition;
	AkFourcc  fccChunk;       // Unused. Wav lists are not supported.
	AkUInt32   dwChunkStart;   // Unused. Wav lists are not supported.
	AkUInt32   dwBlockStart;   // Unused. Wav lists are not supported.
	AkUInt32   dwSampleOffset;
};

struct LabelCuePoint
{
	AkUInt32   dwCuePointID;
	char	  strLabel[1]; // variable-size string
};

struct Segment 
{
	AkUInt32    dwIdentifier;
	AkUInt32    dwLength;
	AkUInt32    dwRepeats;
};

struct SampleChunk
{
	AkUInt32     dwManufacturer;
	AkUInt32     dwProduct;
	AkUInt32     dwSamplePeriod;
	AkUInt32     dwMIDIUnityNote;
	AkUInt32     dwMIDIPitchFraction;
	AkUInt32     dwSMPTEFormat;
	AkUInt32     dwSMPTEOffset;
	AkUInt32     dwSampleLoops;
	AkUInt32     cbSamplerData;
};

struct SampleLoop
{
	AkUInt32     dwIdentifier;
	AkUInt32     dwType;
	AkUInt32     dwStart;
	AkUInt32     dwEnd;
	AkUInt32     dwFraction;
	AkUInt32     dwPlayCount;
};

#endif //_AK_FILE_PARSER_BASE_H_
