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
// AkSrcLpFilter.cpp
//
// Single Butterworth low pass filter section (IIR). 
// The input control parameter in range (0,100) is non -linearly mapped to cutoff frequency (Hz)
// Assumes same thread will call both SetLPFPar and Execute (not locking)
// There is some interpolation done on the LPF control parameter do avoid stepping behavior in transition
// or fast parameter changes.
// 
// We can think of latency / stepping problem by looking at rates at which different thinks occur:
// Control rate: rate at which SetLPFPar are called -> 1x per buffer (linked with buffer time)
// Interpolation rate: rate at which transitional LPFParam values are changed ( coefficient recalculation needed)
// -> NUMBLOCKTOREACHTARGET per buffer, necessary to avoid stepping while introduces up to 1 buffer latency
// Audio rate: rate at which samples are calculated == sample rate
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h" 
#include "AkSrcLpFilter.h"
#include "AkLPFCommon.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#ifdef PERFORMANCE_BENCHMARK
#include "AkMonitor.h"
#endif

#include "AkCommon.h"
extern AkMemPoolId g_LEngineDefaultPoolId;

#ifdef AK_PS3

#include "AkLEngine.h"

extern char _binary_SrcLpFilter_spu_bin_start[];
extern char _binary_SrcLpFilter_spu_bin_size[];

static AK::MultiCoreServices::BinData SrcLpFilterJobsBin =
	{ _binary_SrcLpFilter_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_SrcLpFilter_spu_bin_size ) };

#else

#include "AkLPFCommon.h"
typedef	void (*LPFDSPFuncPtr) ( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );

extern void Perform1Chan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
extern void Perform2Chan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
extern void PerformNChan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
extern void Perform1ChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
extern void Perform2ChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
extern void PerformNChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );


static const AkUInt8 NumLPFDSPFuncs = 6;
static LPFDSPFuncPtr LPFDSPFuncTable[NumLPFDSPFuncs] = 
{
	Perform1Chan,
	Perform2Chan,
	PerformNChan,
	Perform1ChanInterp,
	Perform2ChanInterp,
	PerformNChanInterp,
};

#endif

CAkSrcLpFilter::CAkSrcLpFilter()
{
	m_pfFiltMem = NULL;
}

CAkSrcLpFilter::~CAkSrcLpFilter()
{

}

// Pass on internal LPF state (must be called after Init)
AKRESULT CAkSrcLpFilter::SetLPFState( AkSharedLPFState & in_rLPFState )
{
	// The filter should have been initialized
	AKASSERT( m_pfFiltMem );
	// Reallocate for new source
	if ( m_pfFiltMem )
	{
		AK::MemoryMgr::Falign(g_LEngineDefaultPoolId, m_pfFiltMem);
		m_pfFiltMem = NULL;
	}
	AkUInt32 uNumChannels = AK::GetNumChannels( m_InternalLPFState.uChannelMask );
#ifdef AK_PS3
	AkUInt32 uAllocSize = ALIGN_SIZE_16( AKLPFNUMCOEFICIENTS*uNumChannels*sizeof(AkReal32) );
	AkUInt8 uAllocAlign = 16;
#else
	AkUInt32 uAllocSize = AKLPFNUMCOEFICIENTS*uNumChannels*sizeof(AkReal32);
	AkUInt8 uAllocAlign = 1;
#endif
	m_pfFiltMem = (AkReal32*) AK::MemoryMgr::Malign( g_LEngineDefaultPoolId, uAllocSize, uAllocAlign );
	if ( !m_pfFiltMem )
		return AK_InsufficientMemory;

	// Set only memories for channel that will persist in this source
	AKPLATFORM::AkMemCpy( m_pfFiltMem, in_rLPFState.pfFiltMem, uNumChannels*AKLPFNUMCOEFICIENTS*sizeof(AkReal32) );

	m_InternalLPFState.fCurrentLPFPar = in_rLPFState.fCurrentLPFPar;
	m_InternalLPFState.fTargetLPFPar = in_rLPFState.fTargetLPFPar;
	m_InternalLPFState.uNumInterBlocks = in_rLPFState.uNumInterBlocks;
	m_bFirstSetLPF = in_rLPFState.bFirstSetLPF;

	{
		AkReal32 fCutFreq = 0;
#ifdef AK_PS3
		// Need to evaluate both current and target 
		m_InternalLPFState.bBypassFilter = 
			EvalLPFBypass( m_InternalLPFState.fCurrentLPFPar, m_InternalLPFState.bComputeCoefsForFeedback, fCutFreq ) &&
			EvalLPFBypass( m_InternalLPFState.fTargetLPFPar, m_InternalLPFState.bComputeCoefsForFeedback, fCutFreq );
#else
		m_InternalLPFState.bBypassFilter = EvalLPFBypass( m_InternalLPFState.fCurrentLPFPar, m_InternalLPFState.bComputeCoefsForFeedback, fCutFreq );
#endif
		if ( !m_InternalLPFState.bBypassFilter && !m_InternalLPFState.IsInterpolating() )
			ComputeLPFCoefs( fCutFreq, m_InternalLPFState.fFiltCoefs );
	}

	return AK_Success;
}

// Retrieve internal LPF state
void CAkSrcLpFilter::GetLPFState( AkSharedLPFState & out_rLPFState )
{
	AKASSERT( m_pfFiltMem );

	// Return zeros for channels we don't have in case state is transfered to an instance with more channels.
	AKPLATFORM::AkMemSet( out_rLPFState.pfFiltMem, 0, AK_VOICE_MAX_NUM_CHANNELS*AKLPFNUMCOEFICIENTS*sizeof(AkReal32) );

	AkUInt32 uNumChannels = AK::GetNumChannels( m_InternalLPFState.uChannelMask );
	AKPLATFORM::AkMemCpy( out_rLPFState.pfFiltMem, m_pfFiltMem, uNumChannels*AKLPFNUMCOEFICIENTS*sizeof(AkReal32) );

	out_rLPFState.fCurrentLPFPar = m_InternalLPFState.fCurrentLPFPar;
	out_rLPFState.fTargetLPFPar = m_InternalLPFState.fTargetLPFPar ;
	out_rLPFState.uNumInterBlocks = m_InternalLPFState.uNumInterBlocks;
	out_rLPFState.bFirstSetLPF = m_bFirstSetLPF;
}


AKRESULT CAkSrcLpFilter::Init( AkChannelMask in_uChannelMask, bool in_bComputeCoefsForFeedback )
{
	// Note: for the moment, the LFE channel is a a full band channel treated no differently than others
	// This means that what really matters here is only the number of channel and not the input channel configuration
	// The number set here may be offset by half the DSP function table size to get the corresponding interpolating routine
	AkUInt32 uNumChannels = AK::GetNumChannels( in_uChannelMask );
#ifndef AK_PS3
	switch ( uNumChannels )
	{
	case 1:
		m_DSPFunctionIndex = 0;
		break;
	case 2:
		m_DSPFunctionIndex = 1;
		break;
	default:
		m_DSPFunctionIndex = 2;
		break;
	}
#endif

	m_InternalLPFState.bComputeCoefsForFeedback = in_bComputeCoefsForFeedback;
	m_InternalLPFState.fCurrentLPFPar = 0.f;
	m_InternalLPFState.fTargetLPFPar = 0.f;
	m_InternalLPFState.uNumInterBlocks = NUMBLOCKTOREACHTARGET;
	m_InternalLPFState.uChannelMask = in_uChannelMask;
	m_InternalLPFState.bBypassFilter = true;

	m_bFirstSetLPF = true;

	// Allocate enough space for filter memories
#ifdef AK_PS3
	AkUInt32 uAllocSize = ALIGN_SIZE_16( AKLPFNUMCOEFICIENTS*uNumChannels*sizeof(AkReal32) );
	AkUInt8 uAllocAlign = 16;
#else
	AkUInt32 uAllocSize = AKLPFNUMCOEFICIENTS*uNumChannels*sizeof(AkReal32);
	AkUInt8 uAllocAlign = 1;
#endif
	m_pfFiltMem = (AkReal32*) AK::MemoryMgr::Malign( g_LEngineDefaultPoolId, uAllocSize, uAllocAlign );
	if ( !m_pfFiltMem )
		return AK_InsufficientMemory;
	AKPLATFORM::AkMemSet( m_pfFiltMem, 0, uAllocSize );

	// Passtrough until coefficients are computed
	m_InternalLPFState.fFiltCoefs[0] = 1.f;
	m_InternalLPFState.fFiltCoefs[1] = 0.f;
	m_InternalLPFState.fFiltCoefs[2] = 0.f;
	m_InternalLPFState.fFiltCoefs[3] = 0.f;

#ifdef PERFORMANCE_BENCHMARK
	m_fTotalTime = 0.f;
	m_uNumberCalls = 0;
#endif 

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminates.
//-----------------------------------------------------------------------------
void CAkSrcLpFilter::Term( )
{
#ifdef PERFORMANCE_BENCHMARK
	if ( m_uNumberCalls )
	{
		AkTChar szString[64];
		swprintf( szString, L"%f\n", m_fTotalTime/m_uNumberCalls );
		AKPLATFORM::OutputDebugMsg( szString );
		MONITOR_MSG( szString );
	}
#endif 

	if ( m_pfFiltMem )
	{
		AK::MemoryMgr::Falign(g_LEngineDefaultPoolId, m_pfFiltMem);
		m_pfFiltMem = NULL;
	}
}

#ifdef AK_PS3
void CAkSrcLpFilter::ExecutePS3( AkAudioBuffer * io_pBuffer, AKRESULT &io_result )
{
	// Set PS3 specific information
	m_InternalLPFState.uChannelMask = io_pBuffer->GetChannelMask();
	AkUInt32 uNumChannels = AK::GetNumChannels( m_InternalLPFState.uChannelMask );
	m_InternalLPFState.uValidFrames = io_pBuffer->uValidFrames;
	m_InternalLPFState.uMaxFrames = io_pBuffer->MaxFrames();
	
	// if we're not in bypass mode then start a job
	if (!m_InternalLPFState.bBypassFilter)
	{
		AK::MultiCoreServices::DspProcess * pDsp = CAkLEngine::GetDspProcess();
		pDsp->ResetDspProcess( true );
		pDsp->SetDspProcess( SrcLpFilterJobsBin );
		// LPF state DMA
		pDsp->AddDspProcessDma( &m_InternalLPFState, sizeof(AkInternalLPFState) );
		// Filter memory DMA
		AkUInt32 uDmaSize = ALIGN_SIZE_16( AKLPFNUMCOEFICIENTS*uNumChannels*sizeof(AkReal32) );
		pDsp->AddDspProcessDma( m_pfFiltMem, uDmaSize );
		// Input data DMA (possibly 2 in multi-channel)
		uDmaSize = ALIGN_SIZE_16(uNumChannels * m_InternalLPFState.uMaxFrames * sizeof(AkReal32));
		pDsp->AddDspProcessDma( io_pBuffer->GetDataStartDMA() , uDmaSize );
		io_result = AK_ProcessNeeded;
	}
	// otherwise just update filter memory
	else
	{
		for ( AkUInt32 i = 0; i < uNumChannels; ++i )
		{
			AkUInt32 uIndexOffset = i*AKLPFNUMCOEFICIENTS;
			AkReal32 * AK_RESTRICT pfBuf = io_pBuffer->GetChannel( i );
			LPF_BYPASS_CHANNEL( &m_pfFiltMem[uIndexOffset], pfBuf, m_InternalLPFState.uValidFrames );
		}
	}
}

#else

AkUInt32 CAkSrcLpFilter::Execute( AkAudioBuffer * io_pBuffer )
{
	AKASSERT( m_pfFiltMem != NULL );
	AKASSERT( io_pBuffer != NULL && io_pBuffer->GetChannel( 0 ) != NULL );
	AKASSERT( io_pBuffer->MaxFrames() != 0 && io_pBuffer->uValidFrames <= io_pBuffer->MaxFrames() );

#ifdef PERFORMANCE_BENCHMARK
	AkInt64 TimeBefore;
	AKPLATFORM::PerformanceCounter( &TimeBefore ); 
#endif

#ifdef WIN32
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif

	// Call appropriate DSP function
	AkUInt32 uInterpOffset = m_InternalLPFState.IsInterpolating() ? (NumLPFDSPFuncs / 2) : 0;
	LPFDSPFuncTable[m_DSPFunctionIndex+uInterpOffset]( io_pBuffer, &m_InternalLPFState, m_pfFiltMem );

#ifdef WIN32
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
#endif

#ifdef PERFORMANCE_BENCHMARK
	AkInt64 TimeAfter;
	AKPLATFORM::PerformanceCounter( &TimeAfter );
	AkReal32 fElapsed = AKPLATFORM::Elapsed( TimeAfter, TimeBefore );
	m_fTotalTime += fElapsed;
	++m_uNumberCalls;
#endif

	// Does not modify amount of valid data
	return io_pBuffer->uValidFrames;
}
#endif

AKRESULT CAkSrcLpFilter::SetLPFPar(	AkReal32 in_fTargetLPFPar )
{
	// With modifiers, cannot assume that this will be in range so clip it.
	if ( in_fTargetLPFPar < 0.f )
	{
		in_fTargetLPFPar = 0.f;
	}
	else if ( in_fTargetLPFPar > 100.f )
	{
		in_fTargetLPFPar = 100.f;
	}

	if ( m_bFirstSetLPF )
	{
		// No interpolation required upon first SetLPF call (current == target)
		m_InternalLPFState.fCurrentLPFPar = in_fTargetLPFPar;
		m_InternalLPFState.fTargetLPFPar = in_fTargetLPFPar;
		m_InternalLPFState.uNumInterBlocks = NUMBLOCKTOREACHTARGET;
		AkReal32 fCutFreq = 0;
		m_InternalLPFState.bBypassFilter = EvalLPFBypass( in_fTargetLPFPar, m_InternalLPFState.bComputeCoefsForFeedback, fCutFreq );
		if ( !m_InternalLPFState.bBypassFilter )
			ComputeLPFCoefs( fCutFreq, m_InternalLPFState.fFiltCoefs );
		m_bFirstSetLPF = false;
	}

	if ( in_fTargetLPFPar != m_InternalLPFState.fTargetLPFPar )
	{
		// New LPF interpolation is required (set target)
		m_InternalLPFState.fTargetLPFPar = in_fTargetLPFPar;
		m_InternalLPFState.uNumInterBlocks = 0;
		// Because no SPU job will be triggered if the bypass state is true, we must determine on PPU side if
		// new LPF value requires to filtering or not. SPU will be responsible of updating this as when interpolation
		// towards new LPF value (e.g. 0) should put the LPF in bypass mode 
#ifdef AK_PS3
		AkReal32 fCutFreq = 0;
		m_InternalLPFState.bBypassFilter = 
			EvalLPFBypass( m_InternalLPFState.fCurrentLPFPar, m_InternalLPFState.bComputeCoefsForFeedback, fCutFreq ) &&
			EvalLPFBypass( m_InternalLPFState.fTargetLPFPar, m_InternalLPFState.bComputeCoefsForFeedback, fCutFreq );
#endif	
	}

	return AK_Success;
}
