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
// AkSrcLpFilter.h
//
// Single Butterworth low pass filter section (IIR). 
// The input control parameter in range (0,100) is non -linearly mapped to cutoff frequency (Hz)
// Assumes same thread will call both SetLPFPar and Execute (not locking)
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRCLPFILTER_H_
#define _AK_SRCLPFILTER_H_

#include "AkInternalLPFState.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

#if defined(WIN32) || defined(XBOX360)
//#define PERFORMANCE_BENCHMARK
#endif

// Internal LPF state that can be passed on to following sources for seemless sample accurate playback
struct AkSharedLPFState
{
	AkReal32	pfFiltMem[AK_VOICE_MAX_NUM_CHANNELS*AKLPFNUMCOEFICIENTS];
	AkReal32    fCurrentLPFPar;						// Actual LPF value
	AkReal32	fTargetLPFPar;						// Target LPF value
	AkUInt32	uNumInterBlocks;					// Current number of interpolation steps performed
	bool		bFirstSetLPF;						// Flags first set pitch received, not needed at execute time
};

class CAkSrcLpFilter
{
public:  
	CAkSrcLpFilter();
    virtual ~CAkSrcLpFilter();

    AKRESULT Init( AkChannelMask in_uChannelMask, bool in_bComputeCoefsForFeedback = false );
	void Term();

	AKRESULT SetLPFPar( AkReal32 in_fTargetLPFPar );

	// Internal LPF state set/get
	AKRESULT	SetLPFState( AkSharedLPFState & in_rLPFState );
	void		GetLPFState( AkSharedLPFState & out_rLPFState );

#ifdef AK_PS3
	void ExecutePS3( AkAudioBuffer * io_pBuffer, AKRESULT &io_result );
#else
    AkUInt32 Execute( AkAudioBuffer * io_pBuffer ); 	// Input/output audio buffer 
#endif

#ifndef AK_PS3
private:
#endif
	AkInternalLPFState m_InternalLPFState;	// These only needs to be public on PS3

private:

#ifdef PERFORMANCE_BENCHMARK
	AkUInt32			m_uNumberCalls;
	AkReal32			m_fTotalTime;
#endif

	AkReal32*	m_pfFiltMem;				// Filter memories

#ifndef AK_PS3
	AkUInt8		m_DSPFunctionIndex;			// Index of appropriate DSP function dependent on channel configuration
#endif	
	
	bool		m_bFirstSetLPF;				// Flags first set pitch received, not needed at execute time
};

#endif  // _AK_SRCLPFILTER_H_
