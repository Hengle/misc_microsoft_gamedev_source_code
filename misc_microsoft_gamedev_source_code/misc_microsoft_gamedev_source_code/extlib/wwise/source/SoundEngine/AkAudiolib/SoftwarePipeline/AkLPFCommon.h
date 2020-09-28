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

#ifndef _AK_LPF_COMMON_H_
#define _AK_LPF_COMMON_H_

#include "AkInternalLPFState.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

bool EvalLPFBypass( AkReal32 in_fCurrentLPFPar, bool in_bIsForFeedbackPipeline, AkReal32 & out_fCutFreq );
void ComputeLPFCoefs( AkReal32 in_fCutFreq, AkReal32 * out_pfFiltCoefs );

// Prefetch leads to gains on AMD but is cancelled by lost on P4. 360 is also penalized.
// #define USEPREFETCH
#if defined(WIN32) && defined(USEPREFETCH)
#define DOPREFETCH( __ptr ) _mm_prefetch((char *) (__ptr), _MM_HINT_NTA ) 
#elif defined(XBOX360) && defined(USEPREFETCH)
#define DOPREFETCH( __ptr ) __dcbt( 128, (const void *) __ptr );
#else
#define DOPREFETCH( __ptr )
#endif

// Platform specific handling of denormal IIR filter pitfalls
#ifdef WIN32
#include <xmmintrin.h>
#elif defined(XBOX360)
#include "ppcintrinsics.h"
void AkForceInline RemoveDenormal( AkReal32 & fVal )
{
	// if the whole calculation is done in the FPU registers, 
	// a 80-bit arithmetic may be used, with 64-bit mantissas. The
	// anti_denormal value should therefore be 2^64 times higher than 
	// FLT_MIN. On the other hand, if everything is done with a 32-bit accuracy, 
	// one may reduce the anti_denormal value to get a better
	// accuracy for the small values. (FLT_MIN == 1.175494351e-38F) 

	static const AkReal32 fAntiDenormal = 1e-18f;
	fVal += fAntiDenormal;
	fVal -= fAntiDenormal;
}
#endif
// Note: SPU treat denormals as zeros

#define LPF_INTERPOLATION_SETUP()	\
	const AkReal32 fLPFParamStart = io_pLPFState->fCurrentLPFPar;\
	const AkReal32 fLPFParamDiff = io_pLPFState->fTargetLPFPar - fLPFParamStart;

#define LPF_INTERPOLATE_COEFFICIENTS()	\
	if ( io_pLPFState->IsInterpolating() )	\
	{\
		++io_pLPFState->uNumInterBlocks;\
		io_pLPFState->fCurrentLPFPar = fLPFParamStart + (fLPFParamDiff*io_pLPFState->uNumInterBlocks)/NUMBLOCKTOREACHTARGET;\
		AkReal32 fCutFreq = 0;\
		io_pLPFState->bBypassFilter = EvalLPFBypass( io_pLPFState->fCurrentLPFPar, io_pLPFState->bComputeCoefsForFeedback, fCutFreq );\
		if ( !io_pLPFState->bBypassFilter )\
			ComputeLPFCoefs( fCutFreq, pfCoefs );\
	}

static void LPF_APPLY_CHANNEL( AkReal32 * pFiltMem, AkReal32 * pfCoefs, AkReal32 * AK_RESTRICT & pfBuf, AkUInt32 uFramesInBlock )
{
	DOPREFETCH( pfBuf );

	register AkReal32 xn1 = pFiltMem[0];	// xn1
	register AkReal32 xn2 = pFiltMem[1];	// xn2 -> mem will be thrashed so xn2 can be used as tmp register
	register AkReal32 yn1 = pFiltMem[2];	// yn1
	register AkReal32 yn2 = pFiltMem[3];	// yn2 -> mem will be thrashed so xn2 can be used as tmp register
	register AkReal32 b0 = pfCoefs[0];	// b0
	register AkReal32 b1 = pfCoefs[1];	// b1
	register AkReal32 a1 = pfCoefs[2];	// a1
	register AkReal32 out;				// output accumulator register

	register AkUInt32 uFrameCount = uFramesInBlock;
	while ( uFrameCount > 0 )
	{
		DOPREFETCH( pfBuf );

		// Feedforward part
		xn2 = xn2 + *pfBuf;	// xn2 = xn2 + in
		xn2 = xn2 * b0;		// xn2 = (xn2 + in) * b0 (b0 == b2) 
		out = xn1;			// will need xn1 later so copy in out
		out = out * b1;		// out = xn1 * b1
		out = out + xn2;	// out = (xn2 + in) * b2 + xn1 * b1
		xn2 = xn1;			// xn2 = xn1
		xn1 = *pfBuf;		// xn1 = in 

		// Feedback part
		yn2 = yn2 * pfCoefs[3];	// yn2 = yn2 * a2
		out = out + yn2;		// out = (xn2 + in) * b2 + xn1 * b1 + yn2 * a2
		yn2 = yn1;				// will need yn1 later so copy in yn2		
		yn2 = yn2 * a1;			// yn2 = yn1 * a1
		out = out + yn2;		// out = (xn2 + in) * b2 + xn1 * b1 + yn2 * a2 + yn1 * a1
		yn2 = yn1;				// yn2 = yn1
		yn1 = out;				// yn1 = out
		*pfBuf = out;			// *pfBuf = out

		++pfBuf;				
		--uFrameCount;
	}

	// save registers to memory
	pFiltMem[0] = xn1;
	pFiltMem[1] = xn2;
	pFiltMem[2] = yn1;
	pFiltMem[3] = yn2;
}

static void LPF_BYPASS_CHANNEL( AkReal32 * pFiltMem, AkReal32 * AK_RESTRICT & pfBuf, AkUInt32 uFramesInBlock )
{
	if ( uFramesInBlock >= 2 )
	{
		pFiltMem[0] = pfBuf[uFramesInBlock-1];
		pFiltMem[1] = pfBuf[uFramesInBlock-2];
		pFiltMem[2] = pfBuf[uFramesInBlock-1]; 
		pFiltMem[3] = pfBuf[uFramesInBlock-2];
	}
	pfBuf += uFramesInBlock;
}

static void LPF_CHANNEL_DENORMAL_REMOVE( AkReal32 * pFiltMem )
{
#ifdef XBOX360
	RemoveDenormal( pFiltMem[2] );
	RemoveDenormal( pFiltMem[3] );
#endif 
}


	// LPF DSP routines declarations
	void Perform1Chan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
	void Perform2Chan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
	void PerformNChan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
	void Perform1ChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
	void Perform2ChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );
	void PerformNChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem );


#endif // _AK_LPF_COMMON_H_
