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

#ifndef _AK_SRC_BANKPCM_H_
#define _AK_SRC_BANKPCM_H_

#include "AkSrcBase.h"

class CAkSrcBankPCM : public CAkSrcBaseEx
{
public:

	//Constructor and destructor
	CAkSrcBankPCM( CAkPBI * in_pCtx );
	virtual ~CAkSrcBankPCM();
	
	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 StopStream();
	virtual void	 GetBuffer( AkVPLState & io_state );
	virtual void	 ReleaseBuffer();
	virtual AKRESULT TimeSkip( AkUInt32 & io_uFrames );
	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );

	AkTimeMs		 GetDuration() const;

	virtual AKRESULT StopLooping();

private:
    // State variables.
	AkUInt8 *        m_pucData;			// Current position in bank data.

    // File data.
    AkUInt8	*		 m_pucLoopStart;	// Beginning of loop.
    AkUInt8 *		 m_pucLoopEnd;		// End of loop.
	AkUInt32         m_ulDataSize;		// Data size.
	AkUInt8	*		 m_pucDataStart;
	AkUInt16		 m_usBlockAlign;
};

#endif // _AK_SRC_BANKPCM_H_
