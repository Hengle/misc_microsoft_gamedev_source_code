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

#ifndef _AK_SRC_FILE_VORBIS_H_
#define _AK_SRC_FILE_VORBIS_H_

#include "AkSrcFileBase.h"
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include "AkVorbisCodec.h"

class CAkSrcFileVorbis : public CAkSrcFileBase
{
public:

	//Constructor and destructor
    CAkSrcFileVorbis( CAkPBI * in_pCtx
#ifdef WIN32    
    , bool in_bSSE2 
#endif    
    );
	virtual ~CAkSrcFileVorbis();

	// Data access.
	virtual void		GetBuffer( AkVPLState & io_state );
	virtual void		ReleaseBuffer();
	virtual AkTimeMs	GetDuration( ) const;
	virtual AKRESULT	StartStream( );// Overide default implementation
	virtual void		StopStream( ); // Overide default implementation

	// Use inherited default implementation
	virtual AKRESULT	StopLooping();
	virtual void		VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual AKRESULT	VirtualOff( AkVirtualQueueBehavior eBehavior );
	virtual AKRESULT	TimeSkip( AkUInt32 & io_uFrames );	

protected:

	AKRESULT ProcessStreamBuffer( );						// Override default implementation
	AKRESULT ProcessFirstBuffer( );							// Override default implementation
    AKRESULT ParseHeader( AkInt16 in_sNumLoop );			// Parse header information
	virtual AkUInt32 SampleOffsetToFileOffset( AkUInt32 in_uNumSamples, AkUInt32& out_uRoundingError/*num samples*/ ); //Convert Sample number to a file data offset, used for starting at non zero position.
	AKRESULT RetrieveBuffer( );								// Retrieve new data from input stream
	void SeekStream( AkInt64 in_lSeekPosition );			// Move position with the stream
	AKRESULT VirtualSeek( AkUInt32 & io_uSeekPosition );	// Seek to new position in virtual voice	
	AKRESULT DecodeVorbisHeader( );							// Decode headers and seek table
	AKRESULT InitVorbisInfo();
	void InitVorbisState( );
	AKRESULT TermVorbisState( );
	void LoopInit();

private:
	// Shared between file and bank sources
	AkVorbisSourceState		m_VorbisState;			// Shared source information with vorbis file

	inline void ConsumeData( AkUInt32 uSizeConsumed )
	{
		m_pNextAddress += uSizeConsumed;
		m_ulSizeLeft -= uSizeConsumed;
	}

	// File specific
	AkUInt8*		m_pStitchStreamBuffer;		// Cache (stitch) buffer necessary when crossing stream buffers
	AkUInt16		m_uStitchBufferEndOffset;	// Offset of data coming from previous stream buffer in stitch buffer
	AkUInt16		m_uStitchBufferValidDataSize;	// Complete size of valid data initially (before we consume in it)
	AkUInt16		m_uStitchBufferLeft;		// Effective size of data (from previous and next stream buffer) remaining in stitch buffer

	AkUInt16		m_uBufferLoopCnt;		// Number of loops remaining on the output side
};

#endif // _AK_SRC_FILE_VORBIS_H_
