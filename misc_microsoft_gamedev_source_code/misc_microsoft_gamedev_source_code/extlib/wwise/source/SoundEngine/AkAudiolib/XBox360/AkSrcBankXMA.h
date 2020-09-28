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

#ifndef _AKSRCBANKXMA_H
#define _AKSRCBANKXMA_H

#include "AkSrcBase.h"
#include "AkFileParser.h"

struct IXMAContext;

struct XMAPLAYBACK;

class CAkSrcBankXMA 
	: public CAkSrcBaseEx
{
public:

	//Constructor and destructor
	CAkSrcBankXMA( CAkPBI * in_pCtx );
	virtual ~CAkSrcBankXMA();
	
	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 StopStream();
	virtual AKRESULT StopLooping();
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
    virtual AKRESULT TimeSkip( AkUInt32 & io_uFrames );
	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );
    virtual AKRESULT VirtualOff( AkVirtualQueueBehavior eBehavior );
	
	AkTimeMs		 GetDuration( void ) const;

private:

    AKRESULT        InitXMA( void * in_pBuffer, AkUInt32 in_uBufferSize, XMA2WAVEFORMAT & out_xmaFmt, AkXMA2CustomData & out_xmaCustomData );
	void			VirtualSeek( AkUInt32& io_uSampleOffset, XMA2WAVEFORMAT & in_xmaFmt, AkUInt32 in_uSeekTableOffset, AkUInt8* pData );

	AKRESULT		TimeSkipNoMarkers( AkUInt32 & io_uFrames, bool &out_bDidLoop );

    // File data.
	AkUInt32		m_ulDataSize;         // Data size.	
	AkUInt32		m_ulDataOffset;       // Data offset.

	// XMA
	AkUInt32        m_uiTotalOutputSamples; // size of total output data in samples
	XMAPLAYBACK *   m_pPlayback;
	AkUInt8 *		m_pXMADecodingBuffer;	// XMA decoding buffer

    // For TimeElapsed virtual behavior.
    AkUInt32        m_uCurSampleOffset; // PCM samples offset from beginning of data.
    // When it loops.
	AkUInt8 *       m_pOutBuffer;		// Decoded buffer
};

#endif
