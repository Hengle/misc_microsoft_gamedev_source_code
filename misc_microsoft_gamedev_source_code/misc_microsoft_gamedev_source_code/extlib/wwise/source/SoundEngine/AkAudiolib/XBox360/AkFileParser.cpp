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
// AkFileParser.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkFileParser.h"
#include "AkFileParserBase.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkCommon.h"
#include "AkVorbisInfo.h"
#include "AkMarkers.h"

#include "Xauddefs.h"

static AKRESULT ValidateRIFX( AkUInt8 *& io_pParse, AkUInt8 * in_pBufferEnd )
{
	if ( ( io_pParse + sizeof( ChunkHeader ) + sizeof( AkUInt32 ) ) > in_pBufferEnd )
        return AK_InvalidFile; // Incomplete RIFF header

	ChunkHeader * pChunkHeader = (ChunkHeader *) io_pParse;

	if ( pChunkHeader->ChunkId != RIFXChunkId )
		return AK_InvalidFile; // Unsupported format

	io_pParse += sizeof( ChunkHeader );

	if ( *((AkUInt32 *) io_pParse ) != WAVEChunkId )
		return AK_InvalidFile; // Unsupported format

	io_pParse += sizeof( AkUInt32 );

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Desc: File parsing function.
//-----------------------------------------------------------------------------

AKRESULT CAkFileParser::Parse( 
	const void * in_pvBuffer,             	// Buffer to be parsed.
    AkUInt32 in_ulBufferSize,               // Buffer size.
    WaveFormatEx * out_pAudioFormat,		// Returned audio format.
	AkUInt32 in_ulOutFormatSize,            // Buffer size for out_pAudioFormat
    CAkMarkers * out_pMarkers,   			// Markers. NULL if not wanted. (Mandatory for markers).
    AkUInt32 * out_pulLoopStart,            // Loop start position (in sample frames). NULL if not wanted.
    AkUInt32 * out_pulLoopEnd,              // Loop end position (in sample frames). NULL if not wanted.
	AkUInt32 * out_pulDataSize,				// Size of data portion of the file.
	AkUInt32 * out_pulDataOffset,			// Offset in file to the data.
	void * out_pFmtSpecificInfo,			// Additional format specific information (NULL if not required)
	AkUInt32 in_uFmtSpecificInfoSize		// Size of format specific information 
    )
{
    ///////////////////////////////////////////////////////////////////////
    // WAV PARSER
    ///////////////////////////////////////////////////////////////////////

    AkUInt8 * pParse, * pBufferBegin, *pBufferEnd;
    AkUInt32 ulNumLoops;

    AkUInt8 iParseStage = 0x00;

    // Check parameters
    AKASSERT( in_pvBuffer );
    AKASSERT( in_ulBufferSize > 0 );
    if ( !in_pvBuffer || in_ulBufferSize == 0 )
    {
        return AK_InvalidParameter;
    }

    // Reset loop points values in the case no loop points are found.
    if ( NULL != out_pulLoopStart )
    {
        *out_pulLoopStart = 0;
    }
    if ( NULL != out_pulLoopEnd )
    {
        *out_pulLoopEnd = 0;
    }

    // Same for markers.
    AKASSERT( !out_pMarkers || ( !out_pMarkers->m_pMarkers && 0 == out_pMarkers->m_hdrMarkers.uNumMarkers ) );

    // Parse.

    // pParse is the pointer to data yet to be parsed. Point to beginning.
    pBufferBegin = pParse = (AkUInt8*)(in_pvBuffer);
    pBufferEnd = pParse + in_ulBufferSize;

	// Process first chunk.
	AKRESULT result = ValidateRIFX( pParse, pBufferEnd );
	if ( result != AK_Success )
		return result;
   
    // Process each chunk. For each chunk.
    while ( true )
    {
        // Compute size left.
        AKASSERT( pParse <= pBufferEnd );
        AkUInt32 ulSizeLeft = static_cast<AkUInt32>(pBufferEnd-pParse);

        // Validate chunk header size against data buffer size left.
        if ( ulSizeLeft < sizeof(ChunkHeader) )
        {
            // Incomplete chunk header. 
            return AK_InvalidFile;
        }
        
        // Get chunk header and move pointer.
	    ChunkHeader sChunkHeader = *reinterpret_cast<ChunkHeader*>(pParse);
        pParse += sizeof(ChunkHeader);
        ulSizeLeft -= sizeof(ChunkHeader);

		// Validate chunk size against data left in buffer.
        if ( ulSizeLeft < sChunkHeader.dwChunkSize && sChunkHeader.ChunkId != dataChunkId )
        {
            // Invalid chunk size.
            return AK_InvalidFile;                        
        }

        // Process parsing according to identifier.
        switch ( sChunkHeader.ChunkId )
		{
            case fmtChunkId:
                // Process only the first fmt chunk, discard any other.
				AKASSERT( out_pAudioFormat );
				if ( !( iParseStage & HAVE_FMT ) )
                {
                	AKASSERT( sChunkHeader.dwChunkSize >= 16 );
					AKPLATFORM::AkMemCpy( out_pAudioFormat, pParse, min( in_ulOutFormatSize, sChunkHeader.dwChunkSize ) );
                    iParseStage |= HAVE_FMT;
                }
                break;

            case xma2ChunkId:
				// Process only the first xma2 chunk, discard any other (xma2 chunk replaces standard fmt chunk in xma 2.0 files).
				AKASSERT( out_pAudioFormat );
				if ( !( iParseStage & HAVE_FMT ) )
                {
					AKPLATFORM::AkMemCpy( out_pAudioFormat, 
						pParse, 
						min( in_ulOutFormatSize, 
						sChunkHeader.dwChunkSize ) );
                    iParseStage |= HAVE_FMT;
                }
                break;

			case xmaCustomChunkId:
				// Expecting typeof(out_pFmtSpecificInfo) = AkXMA2CustomData.
				AKASSERT( out_pFmtSpecificInfo && in_uFmtSpecificInfoSize == sizeof(AkXMA2CustomData) );
				reinterpret_cast<AkXMA2CustomData*>(out_pFmtSpecificInfo)->loopData = *reinterpret_cast<AkXMACustHWLoopData*>(pParse);
                break;

            case xma2SeekTable:
                // Expecting typeof(out_pFmtSpecificInfo) = AkXMA2CustomData.
				AKASSERT( out_pFmtSpecificInfo && in_uFmtSpecificInfoSize == sizeof(AkXMA2CustomData) );
                reinterpret_cast<AkXMA2CustomData*>(out_pFmtSpecificInfo)->uSeekTableOffset = static_cast<AkUInt32>(pParse - pBufferBegin);
                break;

            case dataChunkId:

                // This is the last chunk to be detected.
                // Don't validate chunk size against data left in buffer.

                // RIFF chunk, "WAVE" fourcc identifier and FMT chunk must have been read.
                // The data chunk does (should) not have to be all included in the header buffer.
                if ( ( iParseStage & ( HAVE_FMT )) != ( HAVE_FMT ) )
                {
                    return AK_InvalidFile;
                }

                // DataSize and offset.
				*out_pulDataSize   = sChunkHeader.dwChunkSize;
				*out_pulDataOffset = static_cast<AkUInt32>(pParse - pBufferBegin);

				iParseStage |= HAVE_DATA;

                // Done parsing header. Successful. 
                return AK_Success;
                break;

            case cueChunkId:
                // RIFF chunk, "WAVE" fourcc identifier and FMT chunk must have been read.
                // Also check if there is enough data remaining in the buffer to parse all the
                // current chunk.
                if ( ( iParseStage & ( HAVE_FMT )) != ( HAVE_FMT ) )
                {
                    return AK_InvalidFile;
                }

                // Ignore second cue chunk, or ignore if input arguments are NULL
                if ( !( iParseStage & HAVE_CUES ) && out_pMarkers )
                {
                    // Parse markers.
                    // Note. Variable number of cue points. We need to access the memory manager
                    // to dynamically allocate cue structures.
                    // Prepare markers header.
					AkUInt32 uNumMarkers = *reinterpret_cast<AkUInt32*>(pParse);
					if ( uNumMarkers > 0 )
					{
	                    // Allocate markers.
						AKRESULT eResult = out_pMarkers->Allocate( uNumMarkers );
						if ( eResult != AK_Success )
							return eResult;

						AkUInt8 * pThisParse = pParse;

                        // Skip the number of markers field.
                        pThisParse += sizeof(AkUInt32);

                        // Get all the markers.
                        AkUInt32 ulMarkerCount = 0;
						while ( ulMarkerCount < out_pMarkers->Count() )
                        {
                            // Note. We do not support multiple data chunks (and never will).
                            CuePoint * pCuePoint = reinterpret_cast<CuePoint*>(pThisParse);
							out_pMarkers->m_pMarkers[ulMarkerCount].dwIdentifier = pCuePoint->dwIdentifier;
							out_pMarkers->m_pMarkers[ulMarkerCount].dwPosition = pCuePoint->dwPosition;
							out_pMarkers->m_pMarkers[ulMarkerCount].strLabel = NULL;
                            pThisParse += sizeof(CuePoint);
                            ulMarkerCount++;
                        }
                    }

                    // Status.
                    iParseStage |= HAVE_CUES;
                }
                break;
                
            case vorbChunkId:
				// Vorbis needs additional format specific information
				AKASSERT ( out_pFmtSpecificInfo != NULL );
                // Ignore second vorbis chunk
                if ( !( iParseStage & HAVE_VORB ) )
                {
					// Parse looping information
					*reinterpret_cast<AkVorbisInfo*>(out_pFmtSpecificInfo) = *reinterpret_cast<AkVorbisInfo*>(pParse);
  
                    // Status.
                    iParseStage |= HAVE_VORB;
                }
                break;   

			case LISTChunkId:
//				assert( pParse == "adtl" );
				sChunkHeader.dwChunkSize = 4; //adtl
				break;

            case lablChunkId:
				// Ignore label chunk if no cues were read, or if input arguments are NULL
				if ( ( iParseStage & HAVE_CUES ) && out_pMarkers )
				{
					// Find corresponding cue point for this label
					AkUInt32 dwCuePointID = reinterpret_cast<LabelCuePoint*>(pParse)->dwCuePointID;
                    AkUInt32 ulMarkerCount = 0;
                    while ( ulMarkerCount < out_pMarkers->Count() )
                    {
                        if( out_pMarkers->m_pMarkers[ulMarkerCount].dwIdentifier == dwCuePointID )
						{
							// NOTE: We don't fail if we can't allocate marker. Just ignore.
							char* strFileLabel = reinterpret_cast<LabelCuePoint*>(pParse)->strLabel;
							AkUInt32 uStrSize = sChunkHeader.dwChunkSize - sizeof( AkUInt32 );
							out_pMarkers->SetLabel( ulMarkerCount, strFileLabel, uStrSize );
							break; //exit while loop
						}

						ulMarkerCount++;
                    }
				}
				break;

            case smplChunkId:
                // Sample chunk parsing.
                ulNumLoops = reinterpret_cast<SampleChunk*>(pParse)->dwSampleLoops;
                // Parse if Loop arguments are not NULL, and if there are loops in the chunk.
                if ( NULL != out_pulLoopStart &&
                        NULL != out_pulLoopEnd && 
                        ulNumLoops > 0 )
                {
                    
                    // Chunk may have additionnal data. Ignore and advance.
                    AkUInt32 ulPad = reinterpret_cast<SampleChunk*>(pParse)->cbSamplerData;

                    // Move parsing pointer to the end of the chunk.
                    AkUInt8 * pThisParse = pParse + sizeof(SampleChunk) + ulPad;

                    // We should be at the first sample loop. Get relevant info.
                    // Notes. 
                    // - Only forward loops are supported. Taken for granted.
                    // - Play count is ignored. Set within the Wwise.
                    // - Fractions not supported. Ignored.
                    // - In SoundForge, loop points are stored in sample frames. Always true?
                    SampleLoop * pSampleLoop = reinterpret_cast<SampleLoop*>(pThisParse);
                    *out_pulLoopStart = pSampleLoop->dwStart;
                    *out_pulLoopEnd = pSampleLoop->dwEnd;
                    
                    // Ignore remaining sample loops.
                    while ( ulNumLoops > 0 )
                    {
                        pThisParse += sizeof(SampleLoop);
                        ulNumLoops--;
                    }
                }

                // Status.
                iParseStage |= HAVE_SMPL;
                break;

            default:
                // Unprocessed chunk. Discard.
                break;
        }
       
		// Go to next chunk.

		pParse += sChunkHeader.dwChunkSize;

        // Deal with odd chunk sizes. 
        if ( ( sChunkHeader.dwChunkSize % 2 ) != 0 )
        {
            // If next byte is zero, it is padded. Jump by one byte.
            if ( *pParse == 0 )
            {
                pParse += 1;
                // Verify size again just to be sure.
                if ( pParse > pBufferEnd )
                {
                    return AK_InvalidFile;
                }                            
            }
        }
    }

    // Cannot land here.
    AKASSERT( false );
    return AK_Fail;
}
