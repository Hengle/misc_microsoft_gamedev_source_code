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



static inline AkUInt16 SampleNeedExtra(AkUInt16 in_iState)
{
	return (AkUInt16)(in_iState == 2);	//Transform the bool in 1
}

/****************************************************************
* FillFadeOut
* This will construct a small fade out to ensure that even at top 
* speed, the user doesn't feel a shock when stopping.  
* Params: 
* AkInt16* in_pDst: Buffer to fill
* AkInt32 in_nSamples: Number of samples to fill (not counting interleaving)
******************************************************************/
template<class TRAITS>
bool CSinkBase::FillFadeOut(typename TRAITS::OutputType* in_pDst, AkInt16 in_nFrames, AkInt16 &out_nNextBufferNeedExtra)
{
	AKASSERT(!"Not used anymore."); //But keep it around if we decide otherwise.

	AkReal32 fMaxAccel = NORM_DISPLACEMENT * TRANSITION_ACCELERATION / MAX_DISPLACEMENT;
	__m128 mOne = _mm_set_ps1(1.0f);

	//Compute the current speed.  (Y[-1] - Y[-2]) * SampleRate
	__m128 mSpeed = _mm_mul_ps(_mm_sub_ps(m_Output1, m_Output2), _mm_load_ps1(&m_fSampleRate));

	//Compute the delta speed for each sample. (Amax * T)	
	__m128 mTime = _mm_load_ps1(&m_fTimeSample);
	__m128 mSpeedDelta = _mm_mul_ps(_mm_load_ps1(&fMaxAccel), mTime);

	//For the acceleration, we want to go in the opposite direction of speed
	__m128 mReversedSigns = _mm_andnot_ps(mSpeed, *(__m128*)(&M128_SIGNS_MASK));	//Copy reverse signs

	//Get the smaller value between the speed and the delta (check the absolute values and reset the signs properly).
	mSpeedDelta = _mm_min_ps(mSpeedDelta, _mm_abs_ps(mSpeed));
	mSpeedDelta = _mm_or_ps(mSpeedDelta, mReversedSigns);	//Tadam, signs are transferred.

	//Generate the data in a buffer and then send that.
	AkUInt32 nInputFrames = (AkUInt32)ceilf(in_nFrames / SAMPLE_MULTIPLIER);

	//Make sure the pointer is aligned to _m128 boundaries.  We add one frame to have enough space to align manually.
	void* pMem = _alloca((nInputFrames + 1) * sizeof(__m128));

	//Align the pointer.
	__m128* AK_RESTRICT pData = (__m128* AK_RESTRICT)(((ULONG_PTR)pMem / sizeof(__m128) + 1) * sizeof(__m128));

	for(AkUInt32 i = 0; i < nInputFrames; i++)
	{
		mSpeed = _mm_add_ps(mSpeed, mSpeedDelta);
		m_Output1 = _mm_add_ps(m_Output1, _mm_mul_ps(mSpeed, mTime));

		//Ensure we don't exceed 1.0
		__m128 mSign = _mm_and_ps(m_Output1, *(__m128*)(&M128_SIGNS_MASK));
		m_Output1 = _mm_min_ps(mOne, _mm_abs_ps(m_Output1));
		_mm_copysign_ps(m_Output1, mSign);

		pData[i] = m_Output1;
	}

	Fill((AkReal32*&)pData, in_pDst, in_nFrames, out_nNextBufferNeedExtra);

	//Return true if we really stopped.
	return mSpeed.m128_f32[0] == 0.0 && mSpeed.m128_f32[1] == 0.0 && mSpeed.m128_f32[2] == 0.0 && mSpeed.m128_f32[3] == 0.0;
}

/****************************************************************
* FillSilence
* Fills the buffer with the last sample sent to keep the chair still.
* Params: 
* AkInt16* in_pDst: Buffer to fill
* AkInt32 in_nSamples: Number of samples to fill (not counting interleaving)
******************************************************************/
template<class TRAITS>
AkUInt16 CSinkBase::FillSilence(typename TRAITS::OutputType* in_pDst, AkUInt16 in_nFrames, AkUInt32 &io_uOffset)
{
	//Generate the data in a buffer and then send that.
	//Make sure the pointer is aligned to _m128 boundaries.  We add one frame to have enough space to align manually.
	void* pMem = _alloca((in_nFrames + 1) * sizeof(__m128));

	//Align the pointer.
	__m128* AK_RESTRICT pData = (__m128* AK_RESTRICT)(((ULONG_PTR)pMem / sizeof(__m128) + 1) * sizeof(__m128));

	for(AkUInt32 i = 0; i < in_nFrames; i++)
		pData[i] = m_Output1;

	AkUInt16 uWritten = Fill<TRAITS>((AkReal32*&)pData, in_nFrames, in_pDst, io_uOffset);

	//We don't want to record silence.
#ifdef DEBUG_OUTPUT
	if (g_nOutput >= in_nFrames * SAMPLE_MULTIPLIER)
		g_nOutput -= in_nFrames * SAMPLE_MULTIPLIER;
#endif
#ifdef DEBUG_INPUT
	if (g_nOutput >= DBOX_NUM_REFILL_FRAMES)
		g_nOutput -= DBOX_NUM_REFILL_FRAMES;
#endif
	return uWritten;
}

/****************************************************************
* Fill
* Fills the buffer with the source data.   It will be upsampled. 
* Params: 
* AkReal32 *&in_pSrc: Source samples, interleaved
* AkInt16* in_pDst: Buffer to fill
* AkInt32 in_nFrames: Number of output frames to fill 
******************************************************************/
template<class TRAITS>
AkUInt16 CSinkBase::Fill(AkReal32 *in_pSrc,						//Beginning of source buffer
						 AkUInt16 in_nFrames,					//Number of input 
						 typename TRAITS::OutputType *in_pDst,	//Beginning of destination buffer
						 AkUInt32 &io_uOffset)					//Offset in the destination buffer
{
	typename TRAITS::BlockType * AK_RESTRICT pmOut = (typename TRAITS::BlockType *) in_pDst;

	//Put the offset in frames, not in bytes.
	io_uOffset /= sizeof(typename TRAITS::BlockType);

	__m128* AK_RESTRICT pData = (__m128*) in_pSrc;

	__m128 mNew;
	__m128 mOld;
	__m128 mStep;

	AkUInt16 uOutputFrames = 0;
	AkUInt16 uWritten = 0;

	//Fill with the new data
	AkUInt16 iMultiplier = (AkUInt16)SAMPLE_MULTIPLIER;
	while(in_nFrames > 0)
	{
		DEBUG_INPUT_FRAME(*pData);

#ifndef AK_OPTIMIZED
		m_mPeaks = _mm_max_ps(m_mPeaks, _mm_abs_ps(*pData));
#endif

		TRAITS::ScaleValue(*pData, mNew);
		TRAITS::ScaleValue(m_Output1, mOld);
		mStep = _mm_mul_ps(_mm_sub_ps(mNew, mOld), g_InvMultiplier[m_iExtraState]);

		uOutputFrames = iMultiplier + SampleNeedExtra(m_iExtraState);
		for(AkInt16 i = 0; i < uOutputFrames; i++)
		{
			mOld = _mm_add_ps(mOld, mStep);
			TRAITS::OutputOneFrame(mOld, pmOut, io_uOffset, m_uBufferMask);
			DEBUG_OUTPUT_FRAME(mOld);
		}

		m_Output2 = m_Output1;
		m_Output1 = *pData;

		pData++;
		in_nFrames--;
		uWritten += uOutputFrames;

		m_iExtraState = g_SampleNextState[m_iExtraState];
	}

	//Put the offset back in bytes.
	io_uOffset *= sizeof(typename TRAITS::BlockType);

	_mm_empty();

	return uWritten;
}
