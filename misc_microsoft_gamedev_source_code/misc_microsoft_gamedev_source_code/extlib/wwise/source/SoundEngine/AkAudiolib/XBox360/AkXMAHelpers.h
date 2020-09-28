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

#ifndef _AK_XMA_HELPERS_H_
#define _AK_XMA_HELPERS_H_

#include <XMAHardwareAbstraction.h>

#define XMADECODER_FRAME_SIZE_IN_SUBFRAMES 4
#define XMA_PACKET_SIZE     (2048)			// Note. Not defined anywhere in XAudio headers.
#define XMA_POOL_ALLOC_SIZE	8192			// Size of one allocation in the XMA pool
#define SUBFRAMES_TO_DECODE 2				// Number of subframes decoded in one pass by the hardware


namespace AK
{
    namespace XMAHELPERS
    {
        inline DWORD ReadBits( const BYTE* pbSrc, DWORD dwBitOffset, DWORD dwBitCount )
        {
            DWORD dwResult = 0;

            //
            // skip to the first byte that contains bits we want
            //
            pbSrc += dwBitOffset / 8;
            dwBitOffset = dwBitOffset % 8;

            if( ( dwBitCount + dwBitOffset ) < 8 )
            {
                // special case where the bits fit into one byte
                DWORD dwMask = ( 1 << dwBitCount ) - 1;
                dwMask <<= ( 7 - dwBitOffset );
                
                DWORD dwTemp = *pbSrc & dwMask;
                dwResult |= dwTemp >> ( 7 - dwBitOffset );
            }
            else
            {
                //
                // Copy leading bits
                //
                DWORD dwLeadBits = 8 - dwBitOffset;

                DWORD dwMask = ( 1 << dwLeadBits ) - 1;
                dwResult |= *pbSrc & dwMask;

                dwBitCount -= dwLeadBits;
                pbSrc++;

                //
                // Copy whole bytes, if any
                //
                dwMask = 0x000000ff;
                while( dwBitCount > 8 )
                {
                    dwResult <<= 8;
                    dwResult |= *pbSrc & dwMask;

                    pbSrc++;
                    dwBitCount -= 8;
                }


                //
                // Copy trailing bits
                //
                DWORD dwTrailBits = dwBitCount;
                dwMask = 0xff - ( ( 1 << ( 8 - dwTrailBits ) ) - 1 );

                // the trailing bits need to be shifted from the top of the 
                // byte down to the bottom before getting copied into the result
                DWORD dwTemp = *pbSrc & dwMask;
                dwTemp >>= 8 - dwTrailBits;

                dwResult <<= dwTrailBits;
                dwResult |= dwTemp;
            }

            return dwResult;
        }

        // Parses the XMA data and finds the position to which the decoder should be set provided the total 
        // number of samples.
        // io_dwBitOffset must point at the beginning of a packet.
        inline AKRESULT FindDecodePosition( 
            AkUInt8 * in_pData, 
            AkUInt32 in_uiDataSize, 
            AkUInt32 in_uNumSamples,
            DWORD & io_dwBitOffset,
            DWORD & out_uSubframesSkip )
        {
            out_uSubframesSkip = in_uNumSamples / XMADECODER_SUBFRAME_SIZE_IN_SAMPLES;
            DWORD nextpacket = io_dwBitOffset + XMA_PACKET_SIZE * 8;
            DWORD lastbit = in_uiDataSize * 8;
            io_dwBitOffset += XMAPlaybackGetFrameOffsetFromPacketHeader( *(DWORD*)( in_pData ) );

            while ( out_uSubframesSkip >= XMADECODER_FRAME_SIZE_IN_SUBFRAMES && 
					io_dwBitOffset + 15 <= lastbit )
		    {
				// Compute new frame size.
                AkUInt32 framesize = AK::XMAHELPERS::ReadBits( in_pData, io_dwBitOffset, 15 );
			    while ( io_dwBitOffset + framesize + 15 < nextpacket ) 
		        {
                    io_dwBitOffset += framesize;
                    out_uSubframesSkip -= XMADECODER_FRAME_SIZE_IN_SUBFRAMES;
                    if ( out_uSubframesSkip < XMADECODER_FRAME_SIZE_IN_SUBFRAMES )
						return AK_Success;
					
					AkUInt32 continuebit = AK::XMAHELPERS::ReadBits( in_pData, io_dwBitOffset - 1, 1 );
			        if ( !continuebit )
                        break;

					// Compute next frame size.
					framesize = AK::XMAHELPERS::ReadBits( in_pData, io_dwBitOffset, 15 );
		        }

				if ( io_dwBitOffset + framesize + 15 >= nextpacket ) 
				{
					// Next frame crosses the packet header. Consume 4 subframes before moving to following frame.
					out_uSubframesSkip -= XMADECODER_FRAME_SIZE_IN_SUBFRAMES;
					// Before proceeding, verify that we are not done yet.
                    if ( out_uSubframesSkip < XMADECODER_FRAME_SIZE_IN_SUBFRAMES )
						return AK_Success;
				}

		        // Place io_dwBitOffset to first frame of new packet
				io_dwBitOffset = AK::XMAHELPERS::ReadBits( in_pData, nextpacket + 6, 15 ) + nextpacket + 32;
                nextpacket += XMA_PACKET_SIZE * 8;
	        }

            return ( out_uSubframesSkip < XMADECODER_FRAME_SIZE_IN_SUBFRAMES ) ? AK_Success : AK_Fail;
        }
    }
}

#endif // _AK_XMA_HELPERS_H_