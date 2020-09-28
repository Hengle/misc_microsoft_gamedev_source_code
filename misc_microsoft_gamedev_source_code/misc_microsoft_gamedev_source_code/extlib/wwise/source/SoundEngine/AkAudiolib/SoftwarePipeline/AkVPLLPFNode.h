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
// AkVPLLPFNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_LPF_NODE_H_
#define _AK_VPL_LPF_NODE_H_

#include "AkSrcLpFilter.h"
#include "AkVPLNode.h"

class CAkVPLLPFNode : public CAkVPLNode
{
public:
	CAkVPLLPFNode() : m_SoundParams( NULL ) {}

	/*virtual*/ void	ConsumeBuffer( AkVPLState & io_state );
	/*virtual*/ void	ProcessDone( AkVPLState & io_state );
	virtual void		ReleaseBuffer();

	AKRESULT			ProcessBuffer( AkPipelineBuffer* io_pBuffer, AkReal32 in_fLPFValue );
	AKRESULT			TimeSkip( AkUInt32 & io_uFrames );
	AKRESULT			Init( AkSoundParams *	in_pSoundParams,			// SoundParams
							  AkAudioFormat *	io_pFormat );				// Format
	void				Term();
	bool 				IsInitialized() { return m_SoundParams != NULL; }

	// Retrieve internal LPF state so that it can be passed on to following sample accurate sources
	void GetLPFState( AkSharedLPFState & out_rLPFState )
	{
		m_LPF.GetLPFState( out_rLPFState );
	}

	// Retrieve internal LPF state so that it can be passed on to following sample accurate sources
	AKRESULT SetLPFState( AkSharedLPFState & in_rLPFState )
	{
		return m_LPF.SetLPFState( in_rLPFState );
	}

private:
	AkSoundParams *		m_SoundParams;	// Sound parameters
	CAkSrcLpFilter		m_LPF;			// Pointer to lpf object.
};

#endif //_AK_VPL_LPF_NODE_H_
