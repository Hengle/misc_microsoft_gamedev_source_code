/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

#include "AkVorbisCodec.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

AkMemPoolId g_VorbisPoolId = AK_INVALID_POOL_ID;

//================================================================================
// Decoding audio data packets
//================================================================================
void UnpackPacket( AkUInt8 * in_pHeaderPtr, AkUInt8* in_pOggPacketPtr, AkInt32 in_iDecodedPackets, bool in_bLastPacket, ogg_packet & out_Packet )
{
	AkUInt32 dwPacketSize = Read((AkUInt32*)in_pHeaderPtr);
	AkUInt32 dwGranulePos = Read((AkUInt32*)in_pHeaderPtr + 1);

	out_Packet.buffer.data		= in_pOggPacketPtr;
	out_Packet.buffer.size		= dwPacketSize;

	// Build packet
	// We don't tell vorbis about looping, seeking etc. it just sees a sequential packet input
	out_Packet.packetno = in_iDecodedPackets;
	if ( in_bLastPacket )
	{
		out_Packet.e_o_s = 1;
	}
	else
	{
		out_Packet.e_o_s = 0;
	}
	out_Packet.granulepos = dwGranulePos;
}
//================================================================================
// decode a bunch of vorbis packets
//================================================================================
void DecodeVorbis( AkTremorInfo* in_pTremorInfo, AkUInt8* in_pInputBuf, AkInt16* in_pOuputBuf )
{
	AkInt16* pOutput	= in_pOuputBuf;
	AkInt32 iResult;
	bool bGotPacket		= false;
	AkUInt32 uDecodedSamples;
	AkVorbisPacketReader PacketReader( in_pInputBuf, in_pTremorInfo->uInputDataSize );
	static bool bLastPacket;

	if ( in_pTremorInfo->ReturnInfo.bPacketLeftOver )
	{
		goto PacketLeftOver;
	}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
	while ( true ) 
	{
		// Get the next packet
		ogg_packet Packet;
		bGotPacket = PacketReader.ReadPacket( in_pTremorInfo->ReturnInfo.iDecodedPackets, in_pTremorInfo->bNoMoreInputPackets, Packet, bLastPacket );
		if ( !bGotPacket )
		{
			// Can't read more input packets from buffer. Will return the data we have decoded so far.
			in_pTremorInfo->ReturnInfo.bPacketLeftOver = false;
			goto SuccessfulEnd;
		}
		else
		{
			++in_pTremorInfo->ReturnInfo.iDecodedPackets;
			// Decode packet.

			// Prepare for vorbis synthesis and decode entire packet
			iResult = vorbis_dsp_synthesis(&in_pTremorInfo->VorbisDSPState, &Packet);
			if( iResult != 0 )
			{
				AKASSERT( !"Error while setting up Vorbis synthesis.\n" );
				goto FailureEnd;
			}
//--------------------------------------------------------------------------------
// Ask Vorbis how many PCM samples it has ready and retrieve its output buffer.
// Vorbis buffer format is not interleaved, IEEE754 normalized float
// vorbis_synthesis_pcmout outputs single packet a time
//--------------------------------------------------------------------------------
			while ( true )
			{
				// Completed request. Return what we have immediately
				if ( in_pTremorInfo->ReturnInfo.uFramesProduced == in_pTremorInfo->uRequestedFrames  )
				{
					// any left ?
					uDecodedSamples = vorbis_dsp_pcmout( &in_pTremorInfo->VorbisDSPState, NULL, 0 );
					if ( uDecodedSamples )
					{
						in_pTremorInfo->ReturnInfo.bPacketLeftOver = true;
					}
					// nope we're done here
					else
					{
						in_pTremorInfo->ReturnInfo.bPacketLeftOver = false;
					}
					goto SuccessfulEnd;
				}
//--------------------------------------------------------------------------------
// Start synthesizing where we left of
//--------------------------------------------------------------------------------
PacketLeftOver:
				AkUInt32 uRequestedFrames = in_pTremorInfo->uRequestedFrames - in_pTremorInfo->ReturnInfo.uFramesProduced;
				uDecodedSamples = vorbis_dsp_pcmout( &in_pTremorInfo->VorbisDSPState, pOutput,uRequestedFrames );
				if ( uDecodedSamples == 0 )
				{
					// Did not decode sample this time, go up a level to retrieve next packet (overlap-add)
					break;
				}

				// Only take what is necessary to fullfil request
				pOutput += (uDecodedSamples * AK::GetNumChannels(in_pTremorInfo->uChannelMask));
				in_pTremorInfo->ReturnInfo.uFramesProduced += (AkUInt16)uDecodedSamples;
				AKASSERT( in_pTremorInfo->ReturnInfo.uFramesProduced <= in_pTremorInfo->uRequestedFrames );
				iResult = vorbis_dsp_read( &in_pTremorInfo->VorbisDSPState, uDecodedSamples );
				if ( iResult )
				{
					AKASSERT( !"Error reading from Vorbis PCM output buffer.\n" );
					goto FailureEnd;
				}
			}
		}
	}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
FailureEnd:

	// Should not get here unless the algorithm above does work
	AKASSERT( !"Error while decoding Vorbis.\n" );
	in_pTremorInfo->ReturnInfo.eDecoderStatus = AK_Fail;
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
SuccessfulEnd:

#ifdef _CHECKSTACKALLOC
	Stack.PrintMaxStackAlloc();
#endif

	in_pTremorInfo->ReturnInfo.uInputBytesConsumed = PacketReader.GetSizeConsumed();

	// Handle no production case
	if ( in_pTremorInfo->ReturnInfo.uFramesProduced == 0 )
	{
		in_pTremorInfo->ReturnInfo.eDecoderStatus = AK_NoDataReady;
	}
	else
	{
		// is it the end of the stream ?
		if( in_pTremorInfo->bNoMoreInputPackets && PacketReader.DidConsumeAll() && !in_pTremorInfo->ReturnInfo.bPacketLeftOver )
		{
			in_pTremorInfo->ReturnInfo.eDecoderStatus = AK_NoMoreData;
		}
		// nope we have some more
		else
		{
			in_pTremorInfo->ReturnInfo.eDecoderStatus = AK_DataReady;
		}
	}

}
