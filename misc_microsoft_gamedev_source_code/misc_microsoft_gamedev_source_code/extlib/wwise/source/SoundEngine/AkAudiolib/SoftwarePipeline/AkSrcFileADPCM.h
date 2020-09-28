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

#ifndef _AK_SRC_FILEADPCM_H_
#define _AK_SRC_FILEADPCM_H_

#include "AkSrcFileBase.h"
#include "AkADPCMCodec.h"

#define ADPCM_MAX_BLOCK_ALIGN (ADPCM_BLOCK_SIZE*AK_VOICE_MAX_NUM_CHANNELS)

class CAkSrcFileADPCM : public CAkSrcFileBase
{
public:

	//Constructor and destructor
    CAkSrcFileADPCM( CAkPBI * in_pCtx );
	virtual ~CAkSrcFileADPCM();

	// Data access.
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
	virtual AKRESULT TimeSkip( AkUInt32 & io_uFrames );
	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );

	virtual AkTimeMs GetDuration( void ) const;
	virtual void	 StopStream();

protected:
    virtual AKRESULT ParseHeader( AkInt16 in_sNumLoop );
	virtual AkUInt32 SampleOffsetToFileOffset( AkUInt32 in_uNumSamples, AkUInt32& out_uRoundingError/*num samples*/ );

	AkUInt16        m_wBlockAlign;

	AkUInt8 *		m_pOutBuffer;

	AkUInt8			m_ExtraBlock[ ADPCM_MAX_BLOCK_ALIGN ];
	AkUInt16        m_wExtraSize;
};

#endif // _AK_SRC_FILEADPCM_H_
