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

#ifndef _AKSRCFILEXMA_H
#define _AKSRCFILEXMA_H

#include "AkSrcBase.h"
#include <XMAHardwareAbstraction.h>

struct XMAPLAYBACK;

class CAkSrcFileXMA
	: public CAkSrcBaseEx
{
public:

	//Constructor and destructor
    CAkSrcFileXMA( CAkPBI * in_pCtx );
	virtual ~CAkSrcFileXMA();

	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 StopStream();
	virtual AKRESULT StopLooping();
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual AKRESULT VirtualOff( AkVirtualQueueBehavior eBehavior );
    virtual AKRESULT TimeSkip( AkUInt32 & io_uFrames );

	AkTimeMs		 GetDuration() const;

private:
    AKRESULT ConsumeData( 
        DWORD in_nReqSamples,
        DWORD & out_nSamplesConsumed,
        AkUInt8 *& out_pBuffer );
	AKRESULT ParseHeader( AkUInt8 * in_pBuffer, AkUInt32 in_uiSize );
    AKRESULT ProcessFirstBuffer();
    AKRESULT RefillInputBuffers();
    void     ReleaseAllBuffers();
    AKRESULT SubmitData( AkUInt32 in_uiSizeRead, AkUInt8 * in_pData );
	AKRESULT VirtualSeek( AkUInt32 in_uSamplesToSkip );
	AKRESULT CreateXMAContext();
	AKRESULT SubmitFirstBuffer( AkUInt8 * in_pBuffer, AkUInt32 in_uFirstDataSize );
    
    // File data.
    AK::IAkAutoStream * m_pStream;			// Stream handle.
    AkUInt32        m_uIOBlockSizeCorr;     // Low-level IO block size correction. 

    // Looping data.
    AkUInt32        m_uXMADataPosition;     // Current XMA data offset relative to beginning of data (bytes).
    
    // For virtual voices.
    // Seek table.
    AkUInt32 *      m_arSeekTable;          // Seek table (cumulative number of PCM samples in block i+1).
    AkUInt32        m_uNumBlocks;           // Number of blocks in seek table.
    AkUInt32        m_uXMA2BlockSize;       // XMA2 Block size in bytes.
    AkUInt32        m_uSamplesToSkip;       // If set, number of samples to skip in the next buffer.

    XMA_PLAYBACK_INIT m_init;               // Init parameters of XMA decoder.

    AkUInt32        m_uDataOffset;          // XMA data chunk offset in bytes.
    AkUInt32        m_uDataSize;            // XMA data chunk size in bytes.

    AkUInt32        m_uDecodedSmpls;        // Number of decoded samples from beginning.

	struct Buffer
	{
		AkUInt8 * pXMAData;
		AkUInt8 * pToRelease;						// Pointer to memory that must be released when pXMAData is consumed
	};

	Buffer			m_buffers[2];					// Input buffers

	AkUInt8 *		m_pXMADecodingBuffer;			// XMA internal decoding buffer

	AkUInt8 *       m_pOutBuffer;					// Decoded buffer

	// XMA
	AkUInt32        m_uiTotalOutputSamples;			// size of total output data in samples
	XMAPLAYBACK *   m_pPlayback;
	bool            m_bEndReached;					// End of file reached.
    bool            m_bIsFirstBufferPending;        // Flag, true when waiting for first buffer to be parsed.
	AkUInt8			m_uDidLoop;
};

#endif
