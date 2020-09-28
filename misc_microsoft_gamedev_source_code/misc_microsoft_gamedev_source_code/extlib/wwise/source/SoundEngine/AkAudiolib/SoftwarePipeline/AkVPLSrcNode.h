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
// AkVPLSrcNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_SRC_NODE_H_
#define _AK_VPL_SRC_NODE_H_

#include "AkPBI.h"
#include "AkVPLNode.h"

// Note: Currently, the software codec abstraction only exists on the Wii.
#ifndef RVL_OS
class IAkSoftwareCodec : public CAkVPLNode
{
};
#endif


class CAkVPLSrcNode : public IAkSoftwareCodec
{
public:
	static CAkVPLSrcNode * Create(
		CAkPBI *		   in_pCtx,
		bool             in_bActive );

	AKRESULT			Term( AkCtxDestroyReason in_eReason );
	AKRESULT			Start();
	AKRESULT			Stop();
	AKRESULT			Pause();
	AKRESULT			Resume( AkReal32 in_fOutputRate );
	
	virtual AKRESULT	TimeSkip( AkUInt32 & io_uFrames );

	AKRESULT			Connect( CAkVPLNode * in_pInput );
	AKRESULT			Disconnect( CAkVPLNode * in_pInput );
	CAkPBI *			GetContext() { return m_pCtx; }

	// CAkVPLSrcNode interface 
	virtual AKRESULT	StartStream() = 0;
	virtual void		StopStream() = 0;
	virtual void		GetBuffer( AkVPLState & io_state ) {}

	virtual AkTimeMs	GetDuration() const = 0;
	virtual AKRESULT	StopLooping() = 0;

	inline void			SetIOReady() { m_bIOReady = true; }
	inline bool			IsIOReady() { return m_bIOReady; }


protected:
    CAkVPLSrcNode( CAkPBI * in_pCtx );
	CAkPBI *			m_pCtx;		// Pointer to the associated sound context.
	bool				m_bIOReady : 1;	// True=I/O is ready.
};

#endif //_AK_VPL_SRC_NODE_H_
