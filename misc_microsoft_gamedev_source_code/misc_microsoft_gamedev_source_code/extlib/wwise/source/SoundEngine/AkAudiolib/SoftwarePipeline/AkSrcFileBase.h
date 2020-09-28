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

#ifndef _AK_SRC_FILEBASE_H_
#define _AK_SRC_FILEBASE_H_

#include "AkSrcBase.h"
#include <AK/SoundEngine/Common/IAkStreamMgr.h>

class CAkSrcFileBase : public CAkSrcBaseEx
{
public:

	//Constructor and destructor
    CAkSrcFileBase( CAkPBI * in_pCtx );
	virtual ~CAkSrcFileBase();

	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 StopStream();

	virtual AKRESULT StopLooping();

	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual AKRESULT VirtualOff( AkVirtualQueueBehavior eBehavior );

protected:
    virtual AKRESULT ParseHeader( AkInt16 in_sNumLoop ) = 0;

	virtual AkUInt32 SampleOffsetToFileOffset( AkUInt32 in_uNumSamples, AkUInt32& out_uRoundingError/*num samples*/ ) = 0;

    AKRESULT ProcessStreamBuffer( );
    AKRESULT ProcessFirstBuffer( );

    AK::IAkAutoStream * m_pStream;          // Stream handle.
	AkUInt32			m_ulDataSize;       // Data size.
	AkUInt32			m_ulDataOffset;     // Data offset.

    AkUInt8 *           m_pBuffer;          // Buffer currently granted.
    AkUInt8 *           m_pNextAddress;     // Address of next client buffer.
    AkUInt32            m_ulSizeLeft;       // Size left in stream buffer.
    bool				m_bIsLastStmBuffer; // True when last stream buffer.
    bool				m_bIsFirstBufferPending;    // Flag, true when waiting for first buffer to be parsed.
    AkUInt32            m_ulFileOffset;     // Current offset relative to beginning of file.
    AkUInt32			m_ulCurSample;		// Used for markers

    // Looping data.
    AkUInt32             m_ulLoopStart;      // Loop start position: Byte offset from beginning of stream.
    AkUInt32             m_ulLoopEnd;        // Loop back boundary from beginning of stream.
    AkUInt32             m_uiCorrection;     // Correction amount (when loop start not on sector boundary).
	AkUInt8				 m_uDidLoop;
};

#endif // _AK_SRC_FILEBASE_H_
