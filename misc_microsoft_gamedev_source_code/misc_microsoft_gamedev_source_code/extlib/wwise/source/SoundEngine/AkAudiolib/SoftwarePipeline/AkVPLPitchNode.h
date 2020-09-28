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

//////////////////////////////////////////////////////////////////////
//
// AkVPLPitchNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_PITCH_NODE_H_
#define _AK_VPL_PITCH_NODE_H_

#include "AkResampler.h"
#include "AkVPLNode.h"


class CAkVPLPitchNode : public CAkVPLNode
{
public:
	CAkVPLPitchNode() 
	{ 
		// IMPORTANT: Sensible data must be initialized: this node can be Term()ed even if Init() was not called!
		// FIXME: Review all Constructor/Init/Term/~.
		m_BufferIn.Clear();
		m_BufferOut.Clear();
	}

	/*virtual*/ void	GetBuffer( AkVPLState & io_state );
	/*virtual*/ void	ConsumeBuffer( AkVPLState & io_state );
	virtual void		ReleaseBuffer();

	void				VirtualOn( AkVirtualQueueBehavior eBehavior );
	AKRESULT			VirtualOff( AkVirtualQueueBehavior eBehavior );
	AKRESULT			TimeSkip( AkUInt32 & io_uFrames );
	AKRESULT			Init( AkSoundParams * in_pSoundParams,		// SoundParams
							  AkAudioFormat *    io_pFormat,		// Format.
							  CAkPBI* in_pPBI, // PBI, to access the initial pbi that created the pipeline.
							  AkUInt16 in_usMaxFrames,	//Maximum frames per buffer
							  AkUInt32 in_usSampleRate
							  );

	AKRESULT			Term( );	// Memory allocator interface.

	AkReal32			GetLastRate(){ return m_Pitch.GetLastRate(); }

	// Retrieve internal pitch state so that it can be passed on to following sample accurate sources
	void GetPitchState( AkSharedPitchState & rPitchState )
	{
		m_Pitch.GetPitchState( rPitchState );
	}

	// Retrieve internal pitch state so that it can be passed on to following sample accurate sources
	void SetPitchState( AkSharedPitchState & rPitchState )
	{
		m_Pitch.SetPitchState( rPitchState );
	}

#ifdef AK_PS3
	void ProcessDone( AkVPLState & io_state );
#endif

protected:
	void				ReleaseInputBuffer( AkPipelineBuffer & io_buffer );
	void				CopyRelevantMarkers( AkPipelineBuffer*	in_pInputBuffer,
											 AkPipelineBuffer*	io_pBuffer, 
											 AkUInt32		in_ulBufferStartOffset, 
											 AkUInt32		in_ulNumFrames );

private:

#ifdef AK_PS3
	AkUInt32 m_ulInputFrameOffsetBefore;
#endif
	CAkResampler			m_Pitch;			// Resampling object.
	AkPipelineBuffer		m_BufferIn;			// Input buffer.
	AkPipelineBuffer		m_BufferOut;		// Output buffer.
	AkSoundParams *			m_SoundParams;		// Sound parameters
	CAkPBI*					m_pPBI;
	bool					m_bLast;			// True=was the last input buffer.
	bool					m_bStartPosInfoUpdated;
	bool					m_bPadFrameOffset;	// True=Pad buffer with frame offset. Starts true, then indefinitely false.

	AkUInt16				m_usRequestedFrames;
	AkUInt16				m_usMaxFrames;
	//AkInt32*				m_pSkipCount;		// 0 if on time, negative if late. Pitch node must skip one buffer for each skip count.
};

#endif //_AK_VPL_PITCH_NODE_H_
