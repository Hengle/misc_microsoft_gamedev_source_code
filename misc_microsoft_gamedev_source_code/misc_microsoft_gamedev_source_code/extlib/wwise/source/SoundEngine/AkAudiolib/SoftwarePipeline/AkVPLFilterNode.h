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
// AkVPLFilterNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_FILTER_NODE_H_
#define _AK_VPL_FILTER_NODE_H_

#include "AkFXContext.h"
#include "Ak3DParams.h"
#include "AkVPLNode.h"

class CAkVPLFilterNode : public CAkVPLNode
{
public:
	/*virtual*/ void	GetBuffer( AkVPLState & io_state );
	/*virtual*/ void	ConsumeBuffer( AkVPLState & io_state );
	virtual void		ReleaseBuffer();
#ifdef AK_PS3
	virtual void		ProcessDone( AkVPLState & io_state );
#endif

	AKRESULT			TimeSkip( AkUInt32 & io_uFrames );
	void				VirtualOn( AkVirtualQueueBehavior eBehavior );
    AKRESULT            VirtualOff( AkVirtualQueueBehavior eBehavior );
	AKRESULT			Init(
		const AkFXDesc & in_fxDesc,
		AkUInt32 in_uFXIndex,
		CAkPBI * in_pCtx,
		AkAudioFormat &	in_format );
	void				Term();
	void				ReleaseMemory();

private:
	CAkPBI *				m_pCtx;			// Pointer to context.
	AK::IAkEffectPlugin *	m_pEffect;		// Pointer to Fx.
	CAkInsertFXContext *	m_pInsertFXContext;	// FX context.
	bool					m_bLast;		// True=was the last input buffer.
	bool					m_LastBypassed;	// FX bypassed last buffer (determine whether Reset necessary)
	bool                    m_bAllocatedBuffer; // True=releasebuffer must deallocate
	AkChannelMask			m_uChannelMask; // Channels
	AkUInt32                m_uFXIndex;
#ifndef AK_OPTIMIZED
	AkUInt32				m_uiID;			// Cached copy of fx id for profiling.
#endif

	AkPipelineBuffer *		m_pCurBufferOut;	// Node's output buffer.
};

#endif //_AK_VPL_FILTER_NODE_H_
