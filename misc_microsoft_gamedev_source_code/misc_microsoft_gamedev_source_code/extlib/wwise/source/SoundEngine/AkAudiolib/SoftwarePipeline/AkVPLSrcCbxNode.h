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
// AkVPLSrcCbxNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_SRC_CBX_NODE_H_
#define _AK_VPL_SRC_CBX_NODE_H_

#include "AkPBI.h"
#include "AkSrcLpFilter.h"
#include "AkResampler.h"
#include "AkVPLNode.h"
#include "AkVPLLPFNode.h"
#include "AkVPLPitchNode.h"
#include "Ak3DParams.h"

class CAkVPLNode;
class CAkVPLSrcNode;
class CAkVPLFilterNode;

#define MAX_NUM_SOURCES			2			// Max no. of sources in sample accurate container. 

struct AkVPLSrcCbxRec
{
	CAkVPLPitchNode 			m_Pitch;
	CAkVPLLPFNode 				m_LPF;
	CAkVPLSrcNode *				m_pSrc;
	CAkVPLFilterNode *			m_pFilter[AK_NUM_EFFECTS_PER_OBJ];
	
	AkForceInline CAkVPLLPFNode * Head() { return &m_LPF; }
};

class CAkVPLSrcCbxNode
{
	friend class CAkLEngine;
public:
	AKRESULT			Init(AkUInt32 in_iSampleRate, AkUInt16 in_usMaxFrames);
	void				Term();
	void				ReleaseMemory();
	void				Start();
	void				Stop();
	void				Pause();
	void				Resume();
	void				StopLooping();

	bool				StartRun( AkVPLState & io_state );
	void				GetBuffer( AkVPLState & io_state ) { AKASSERT( !m_bufferMix.HasData() ); }
	void				ConsumeBuffer( AkVPLState & io_state );
	void				ReleaseBuffer();
	AKRESULT			AddSrc( CAkPBI * in_pCtx, bool in_bActive );	
	AKRESULT			AddPipeline( AkVPLSrcCbxRec * in_pSrcRec );
    AKRESULT			AddPipelineDeferred( CAkPBI * in_pCtx );
	AKRESULT			FetchStreamedData( CAkVPLSrcNode * in_pSrc );
	CAkPBI *			GetContext();
	VPLNodeState		GetState(){ return m_eState; }

	// Environmental helper methods
	void						SetLastEnvironmentalValues( const AkEnvironmentValue *in_pValues );
	const AkEnvironmentValue*	GetLastEnvironmentalValues() { return m_LastEnvironmentValues; };
	AKRESULT					ProcessBufferForObstruction( AkAudioBufferCbx* io_pBuffer );

#ifndef AK_OPTIMIZED
	bool				LastAudible() { return m_bLastAudible; } 
#endif

private:
	void				RemoveSrc( AkUInt32 in_uSrcIdx, AkCtxDestroyReason in_eReason );
	static void			DeleteCbxRec( AkVPLSrcCbxRec * in_pSrcRec, AkCtxDestroyReason in_eReason );
	AKRESULT			SourceTimeSkip( AkUInt32 in_uFrames );
	AKRESULT			SetVirtual( bool in_fMode );
	void				GetVolumes( bool in_bEnvBus, CAkPBI* AK_RESTRICT in_pContext, AkAudioBufferCbx* AK_RESTRICT io_pBuffer, bool & out_bNextSilent );
	bool				IsInitiallyUnderThreshold( CAkPBI* in_pCtx );

private:

	AkUInt32			m_iSampleRate;
	AkUInt16			m_usMaxFrames;
	VPLNodeState		m_eState;
	AkVPLSrcCbxRec *	m_pCbxRec[MAX_NUM_SOURCES];	// [0] == Current, [1] == Next
	AkPipelineBuffer	m_bufferMix;				// Output stitching buffer for new pipeline.
	bool				m_bFirstBufferProcessed;
	bool                m_bLastAudible;				// For virtual voices
	bool				m_bPreviousSilent;			// Previous volumes was silent -- NOT equivalent to m_bLastAudible
	bool				m_bLastObsLPF;				// Did last buffer have obstruction lowpass filter ?

#ifndef AK_OPTIMIZED
	bool				m_bNotifyStarvationAtStart;	// Log source starvation on connect to VPL.
	bool				m_bStreamingStarted;
	int					m_iWasStarvationSignaled;// must signal when == 0
#endif

	AkSpeakerVolumes	m_PreviousDirect[AK_VOICE_MAX_NUM_CHANNELS]; // previous direct path volume values
	AkSpeakerVolumes	m_PreviousEnv[AK_VOICE_MAX_NUM_CHANNELS];	 // previous environment volume values
	AkSoundParams		m_Param;								 // Parameters.
	
	AkEnvironmentValue  m_LastEnvironmentValues[AK_MAX_ENVIRONMENTS_PER_OBJ];

	//Not uselessly initialized.
	AkVirtualQueueBehavior m_eVirtualBehavior;
	AkBelowThresholdBehavior m_eBelowThresholdBehavior;

	CAkVPLLPFNode		m_ObstructionLPF;		    // LPF node for obstruction.
};

#endif //_AK_VPL_SRC_CBX_NODE_H_
