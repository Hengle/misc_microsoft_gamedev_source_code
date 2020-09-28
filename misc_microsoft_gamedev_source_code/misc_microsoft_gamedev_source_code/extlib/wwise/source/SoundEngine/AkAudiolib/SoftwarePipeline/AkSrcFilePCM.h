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

#ifndef _AK_SRC_FILEPCM_H_
#define _AK_SRC_FILEPCM_H_

#include "AkSrcFileBase.h"

class CAkSrcFilePCM : public CAkSrcFileBase
{
public:

	//Constructor and destructor
    CAkSrcFilePCM( CAkPBI * in_pCtx );
	virtual ~CAkSrcFilePCM();

	// Data access.
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
	virtual AKRESULT TimeSkip( AkUInt32 & io_uFrames );
	virtual void	 VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual void	 StopStream();

	virtual AkTimeMs GetDuration( void ) const;	

protected:
    virtual AKRESULT ParseHeader( AkInt16 in_sNumLoop );
	virtual AkUInt32 SampleOffsetToFileOffset( AkUInt32 in_uNumSamples, AkUInt32& out_uRoundingError/*num samples*/ );

	// Incomplete sample frames handling.
	AkUInt8	*		 m_pStitchBuffer;
	AkUInt16         m_uNumBytesBuffered;	// Number of bytes temporarily stored in stitched sample frame buffer.
	
	AkUInt16		 m_uSizeToRelease;		// Size of returned streamed buffer (in bytes).
	
};

#endif // _AK_SRC_FILEPCM_H_
