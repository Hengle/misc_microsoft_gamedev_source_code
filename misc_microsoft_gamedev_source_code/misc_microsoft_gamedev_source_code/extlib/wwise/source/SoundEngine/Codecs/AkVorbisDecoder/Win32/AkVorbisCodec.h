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
// AkVorbisCodec.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_VORBIS_CODEC_H_
#define _AK_VORBIS_CODEC_H_

#include "AkSrcVorbis.h"

inline AkUInt32 Read(AkUInt32* pAddress)
{
	return *pAddress;
}

void UnpackPacket( AkUInt8 * in_pHeaderPtr, AkUInt8* in_pOggPacketPtr, AkInt32 in_iDecodedPackets, bool in_bLastPacket, ogg_packet & out_Packet );

struct AkVorbisPacketReader
{
	inline AkVorbisPacketReader( AkUInt8* in_pFirstPacketStart, AkUInt32 in_uTotalSize )
	{
		m_pFirstPacketStart = in_pFirstPacketStart;
		m_uTotalSize = in_uTotalSize;
		m_uCurrentOffset = 0;
	}

	inline bool ReadPacket( AkInt32 in_iDecodedPackets, bool in_bNoMoreInputPackets, ogg_packet & out_Packet, bool & out_bLastPacket )
	{
		if ( m_uCurrentOffset + sizeof(OggPacketHeader) <= m_uTotalSize )
		{
			AkUInt8* pCurPtr = (AkUInt8*)m_pFirstPacketStart + m_uCurrentOffset;
			AkUInt32 uPacketSize = Read((AkUInt32*)pCurPtr);
			if ( m_uCurrentOffset + sizeof(OggPacketHeader) + uPacketSize <= m_uTotalSize )
			{
				m_uCurrentOffset += sizeof(OggPacketHeader) + uPacketSize;
				out_bLastPacket = DidConsumeAll() && in_bNoMoreInputPackets;
				UnpackPacket( pCurPtr, pCurPtr+sizeof(OggPacketHeader),in_iDecodedPackets, out_bLastPacket, out_Packet );	
				return true;
			}
			else
			{
				return false;
			}
			
		}
		else
		{
			return false;
		}
	}

	inline AkUInt32 GetSizeConsumed( )
	{
		return m_uCurrentOffset;
	}

	inline bool DidConsumeAll( )
	{
		return (m_uCurrentOffset == m_uTotalSize);
	}

private:
	AkUInt8*	m_pFirstPacketStart;
	AkUInt32 	m_uCurrentOffset;
	AkUInt32	m_uTotalSize;
};

struct AkTremorInfoReturn
{
	AkUInt32			uFramesProduced;		// Frames produced by decoder
	AKRESULT			eDecoderStatus;			// Decoder status
	AkInt32				iDecodedPackets;		// Current decoded packet count
	AkUInt32			uInputBytesConsumed;	// Size of all packets consumed
	bool				bPacketLeftOver;		// Did we finish decoding current packet last time ?
};

struct AkTremorInfo
{
	AkTremorInfoReturn		ReturnInfo;				// the codec will update and return this part
	vorbis_dsp_state		VorbisDSPState;			// central working state for the packet->PCM decoder
	AkUInt8*				pucData;				// Pointer to output location
	AkUInt32				uChannelMask;			// Channels to decode	
	AkUInt32				uRequestedFrames;		// Requested frames to decode
	AkUInt32				uInputDataSize;			// Data size of all input (not necessarly a number of packets)
	AkUInt8					uInputDataOffset;		// Input data offset for alignment
	bool					bNoMoreInputPackets;	// Signal to the codec that there will not be anymore incoming packets after this
};

// Information common to Vorbis file and Vorbis bank (members)
struct AkVorbisSourceState
{
	AkTremorInfo			TremorInfo;				// Information used by the codec for decoding
	AkVorbisInfo			VorbisInfo;				// Additional information from encoder
	CAkVorbisAllocator		Allocator;				// Pseudo vorbis pool memory allocator
	AkUInt32				uSampleRate;			// Sample rate
	AkVorbisSeekTableItem*	pSeekTable;				// Seek table storage (only allocated if necessary)
	AkUInt32				uSeekTableSizeRead;		// Size read from seek table
	AkUInt32				uCurrentPCMFrame;		// Current PCM frame position
	AkUInt32				uBufferStartPCMFrame;	// Cache for process done
	AkUInt32				uPCMStartLimitMarkers;	// PCM start frame to consider for markers with looping
	AkUInt32				uPCMEndLimitMarkers;	// PCM end frame to consider for markers with looping
	bool					bNeedSeekTable;			// Do we need a seek table ?
};

void DecodeVorbis( AkTremorInfo* in_pTremorInfo, AkUInt8* in_pInputBuf, AkInt16* in_pOuputBuf );

#endif // _AK_VORBIS_CODEC_H_
