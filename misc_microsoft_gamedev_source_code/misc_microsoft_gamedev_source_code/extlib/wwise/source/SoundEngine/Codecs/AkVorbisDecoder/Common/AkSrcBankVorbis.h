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

#ifndef _AK_SRC_BANK_VORBIS_H_
#define _AK_SRC_BANK_VORBIS_H_

#include "AkSrcBase.h"
#include "AkSrcVorbis.h"
#include "AkVorbisCodec.h"

class CAkSrcBankVorbis : public CAkSrcBaseEx
{
public:

	//Constructor and destructor
    CAkSrcBankVorbis( CAkPBI * in_pCtx
#ifdef WIN32    
    , bool in_bSSE2 
#endif    
    );
	virtual ~CAkSrcBankVorbis();

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

	AKRESULT DecodeVorbisHeader();							// Decode headers and seek table
	AKRESULT VirtualSeek( AkUInt32 & io_uSeekPosition );	// Seek to new position in virtual voice
	AKRESULT InitVorbisInfo();
	void InitVorbisState();
	AKRESULT TermVorbisState();
	void LoopInit();

private:
	// Shared between file and bank sources
	AkVorbisSourceState		m_VorbisState;		// Shared source information with vorbis file

	// Bank specific
	AkUInt8*			m_pucData;				// current data pointer
	AkUInt32			m_uDataSize;			// whole data size
	AkUInt8*			m_pucDataStart;			// start of audio data
	AkUInt8*			m_pucBackBoundary;		// Either the loop end position or file end
};

#endif // _AK_SRC_BANK_VORBIS_H_
