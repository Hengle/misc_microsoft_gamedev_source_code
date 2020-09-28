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
// AkMotionGenerator.cpp
//
// Implements a silent source.
// Note: Target output format is currently determined by the source itself.
// Out format currently used is : 48 kHz, 16 bits, Mono
//
// Copyright (c) 2006-2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkMotionGenerator.h"

#ifdef RVL_OS
#include <string.h>
#include <stdlib.h>
#endif

using namespace AK;

#define PARAM(__name) m_pSharedParams->GetParams().__name

// Plugin mechanism. Implement Create function and register its address to the FX manager.
IAkPluginParam * AkCreateMotionGeneratorParams( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkMotionGeneratorParams( in_pAllocator ) );
}

// Constructor.
CAkMotionGeneratorParams::CAkMotionGeneratorParams( IAkPluginMemAlloc * in_pAllocator )
{
	m_pAllocator = in_pAllocator;
}

// Copy constructor.
CAkMotionGeneratorParams::CAkMotionGeneratorParams( const CAkMotionGeneratorParams &Copy )
{
	m_pAllocator = Copy.m_pAllocator;
	m_Params.m_fPeriod				=Copy.m_Params.m_fPeriod;
	m_Params.m_fPeriodMultiplier	=Copy.m_Params.m_fPeriodMultiplier;
	m_Params.m_fDuration			=Copy.m_Params.m_fDuration;
	m_Params.m_fAttackTime			=Copy.m_Params.m_fAttackTime;
	m_Params.m_fDecayTime			=Copy.m_Params.m_fDecayTime;
	m_Params.m_fSustainTime			=Copy.m_Params.m_fSustainTime;
	m_Params.m_fSustainLevel		=Copy.m_Params.m_fSustainLevel;
	m_Params.m_fReleaseTime			=Copy.m_Params.m_fReleaseTime;
	m_Params.m_eDurationType		=Copy.m_Params.m_eDurationType;
#ifdef RVL_OS
	m_Params.m_fWiiPitch			=Copy.m_Params.m_fWiiPitch;
#endif

	for(AkUInt32 i = 0; i < AK_NUM_CURVES; i++)
	{
		AkCurve &rCurve = m_Params.m_Curves[i];
		const AkCurve &rSrcCurve = Copy.m_Params.m_Curves[i];
		rCurve.Set(rSrcCurve.m_pArrayGraphPoints, rSrcCurve.m_ulArraySize, AkCurveScaling_None);
	}
}

// Destructor.
CAkMotionGeneratorParams::~CAkMotionGeneratorParams()
{
	for(AkUInt32 i = 0; i < AK_NUM_CURVES; i++)
		m_Params.m_Curves[i].Unset();
}

// Create shared parameters duplicate.
IAkPluginParam * CAkMotionGeneratorParams::Clone( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkMotionGeneratorParams( *this ) );
}

// Shared parameters initialization.
AKRESULT CAkMotionGeneratorParams::Init( IAkPluginMemAlloc *	in_pAllocator,									  
	                                  void *			in_pvParamsBlock, 
	                                  AkUInt32			in_ulBlockSize 
                                 )
{
    if ( in_ulBlockSize == 0)
        return AK_Success;
	 
    return SetParamsBlock( in_pvParamsBlock, in_ulBlockSize );
}

// Shared parameters termination.
AKRESULT CAkMotionGeneratorParams::Term( IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Set all shared parameters at once.
AKRESULT CAkMotionGeneratorParams::SetParamsBlock( void * in_pvParamsBlock, 
                                                AkUInt32 in_ulBlockSize
                                              )
{

    assert( NULL != in_pvParamsBlock && in_ulBlockSize >= m_Params.GetFixedStructureSize());
    if ( NULL == in_pvParamsBlock || in_ulBlockSize < m_Params.GetFixedStructureSize() )
    {
        return AK_InvalidParameter;
    }

	LockParams( );

	//Copy the fixed block of parameters.
	AkUInt32 lFixedSize = m_Params.GetFixedStructureSize();
	memcpy( &m_Params, in_pvParamsBlock, lFixedSize);

	m_Params.m_fSustainLevel = powf( 10.f, m_Params.m_fSustainLevel * 0.05f );

	in_pvParamsBlock = (char*)in_pvParamsBlock + lFixedSize;
	AKRESULT eResult = ReadAllCurves(in_pvParamsBlock);
   
    UnlockParams( );

    return AK_Success;
}

// Update one parameter.
AKRESULT CAkMotionGeneratorParams::SetParam( AkPluginParamID in_ParamID,
                                          void * in_pvValue, 
                                          AkUInt32 in_ulParamSize
                                        )
{
    if ( in_pvValue == NULL )
	{
		return AK_InvalidParameter;
	}

	// Pointer should be aligned on 4 bytes
#if defined(WIN32) || defined(XBOX360)
	assert(((__w64 int)in_pvValue & 3) == 0);
#else
	assert(((int)in_pvValue & 3) == 0);
#endif

	AKRESULT eResult = AK_Success;

	LockParams( );

    // Set parameter value.
    switch ( in_ParamID )
    {
	case AK_Period_Param:
		m_Params.m_fPeriod = *(AkReal32*)in_pvValue;
		break;
	case AK_PeriodMultiplier_Param:
		m_Params.m_fPeriodMultiplier = *(AkReal32*)in_pvValue;
		break;
	case AK_DurationType_Param:	
		m_Params.m_eDurationType = *(AkUInt16*)in_pvValue;
		break;
	case AK_Duration_Param:		
		m_Params.m_fDuration = *(AkReal32*)in_pvValue;
		break;
	case AK_AttackTime_Param:
		m_Params.m_fAttackTime = *(AkReal32*)in_pvValue;
		break;
	case AK_DecayTime_Param:	
		m_Params.m_fDecayTime = *(AkReal32*)in_pvValue;
		break;
	case AK_SustainTime_Param:
		m_Params.m_fSustainTime = *(AkReal32*)in_pvValue;
		break;
	case AK_SustainLevel_Param:		
		m_Params.m_fSustainLevel = powf( 10.f, *(AkReal32*)in_pvValue * 0.05f );
		break;
	case AK_ReleaseTime_Param:		
		m_Params.m_fReleaseTime = *(AkReal32*)in_pvValue;
		break;
#ifdef RVL_OS
	case AK_WiiPitch_Param:
		m_Params.m_fWiiPitch = *(AkReal32*)in_pvValue;
		break;
#endif

	case AK::IAkPluginParam::ALL_PLUGIN_DATA_ID:		
		eResult = ReadAllCurves(in_pvValue);
		break;

	default:		
		eResult = AK_InvalidParameter;
		break;
    }

    UnlockParams( );

    return eResult ;
}

AKRESULT CAkMotionGeneratorParams::ReadCurve(AkUInt16 in_iIndex, void* &io_pData)
{
	char * pData = (char *) io_pData;

	AkUInt16 cPoints = *( (AkUInt16 *) pData );
	pData += sizeof( AkUInt16 );

	AkCurve &rCurve = m_Params.m_Curves[in_iIndex];
	if ( cPoints > 0 )
	{
		rCurve.Set( (AkRTPCGraphPoint *) pData, cPoints, AkCurveScaling_None);
		pData += sizeof(AkRTPCGraphPoint) * cPoints;
	}

	//Advance the pointer at the end of the structure we just read.
	io_pData = pData;
	
	return AK_Success;
}

AKRESULT CAkMotionGeneratorParams::ReadAllCurves(void* &io_pData)
{
	char * pData = (char *) io_pData;

	AkUInt16 cCurves = *( (AkUInt16 *) pData );
	pData += sizeof( AkUInt16 );

	for ( AkUInt16 i = 0; i < cCurves; ++i )
	{
		AKRESULT eResult = ReadCurve( i, (void *&) pData );
		if ( eResult != AK_Success ) 
			return eResult;
	}

	io_pData = pData;

	return AK_Success;
}
 
//-----------------------------------------------------------------------------
// Name: CreateEffect
// Desc: Plugin mechanism. Dynamic create function whose address must be 
//       registered to the FX manager.
//-----------------------------------------------------------------------------
IAkPlugin* AkCreateMotionGenerator( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkMotionGenerator( ) );
}

//-----------------------------------------------------------------------------
// Name: CAkMotionGenerator
// Desc: Constructor.
//-----------------------------------------------------------------------------
CAkMotionGenerator::CAkMotionGenerator()
{
	// Initialize members.
	m_uSampleRate = 0;
	m_uBytesPerSample = 0;
	m_pSourceFXContext = NULL;
	m_pSharedParams = NULL;
	m_fTime = 0.0;
	m_uDuration = 0;
	m_uSamplesProduced = 0;
	m_uLoops = 0;	
	m_uSection = 0;
	m_fVol = 1.0f;

	memset(m_ADSRState, 0, sizeof(m_ADSRState));
}

//-----------------------------------------------------------------------------
// Name: ~CAkMotionGenerator
// Desc: Destructor.
//-----------------------------------------------------------------------------
CAkMotionGenerator::~CAkMotionGenerator()
{
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Init.
//-----------------------------------------------------------------------------
AKRESULT CAkMotionGenerator::Init( IAkPluginMemAlloc *			in_pAllocator,		// Memory allocator interface.
								IAkSourcePluginContext *	in_pSourceFXContext,// Source FX context
								IAkPluginParam *			in_pParams,			// Effect parameters.
								AkAudioFormat &				io_rFormat			// Supported audio output format.
								)
{
	// Keep source FX context (looping etc.)
	m_pSourceFXContext = in_pSourceFXContext;
	m_pSharedParams = reinterpret_cast<CAkMotionGeneratorParams*>(in_pParams);

	io_rFormat.uChannelMask = AK_SPEAKER_FRONT_LEFT | AK_SPEAKER_FRONT_RIGHT;

	// Save audio format internally
	m_uSampleRate = io_rFormat.uSampleRate;
	m_uBytesPerSample = io_rFormat.GetBitsPerSample() / 8;

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Term.
// Desc: Term. The effect must destroy itself herein
//-----------------------------------------------------------------------------
AKRESULT CAkMotionGenerator::Term( IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Reset
// Desc: Reset or seek to start (looping).
//-----------------------------------------------------------------------------
AKRESULT CAkMotionGenerator::Reset( void )
{
	m_fTime = 0.0;
	m_uSamplesProduced = 0;

	m_uLoops = m_pSourceFXContext->GetNumLoops();
	if (m_uLoops == 0)
		m_uLoops = 0xFFFFFFFF;

	if(PARAM(m_eDurationType) == 2 /*ADSR*/)
	{
		if (PARAM(m_fAttackTime) != 0.0)
		{
			//Attack makes the volume go from 0 to 1 in m_fAttackTime seconds.  
			AkReal32 fSamples = PARAM(m_fAttackTime) * m_uSampleRate;
			m_ADSRState[0].m_iNextSection = (AkUInt32)fSamples;
			m_ADSRState[0].m_fStep = 1.0f/fSamples;
			m_ADSRState[0].m_fStartVol = 0.0f;
		}
		else
		{
			m_ADSRState[0].m_fStartVol = 0.0f;
			m_ADSRState[0].m_fStep = 0.0;
			m_ADSRState[0].m_iNextSection = -1; 
		}

		if (PARAM(m_fDecayTime) != 0.0)
		{
			//Decay makes the volume go from 1 to m_fSustainLevel in m_fDecayTime seconds.   
			AkReal32 fSamples = PARAM(m_fDecayTime) * m_uSampleRate;
			m_ADSRState[1].m_iNextSection = (AkUInt32)fSamples + m_ADSRState[0].m_iNextSection;
			m_ADSRState[1].m_fStep = -1.0f/fSamples * (1.0f - PARAM(m_fSustainLevel));
			m_ADSRState[1].m_fStartVol = 1.0f;
		}
		else
		{
			m_ADSRState[1].m_fStep = 0.0;
			m_ADSRState[1].m_iNextSection = -1; 
			m_ADSRState[1].m_fStartVol = 1.0f;
		}

		if (PARAM(m_fSustainTime) != 0.0)
		{
			//Sustains... well keeps the signal at the given level
			m_ADSRState[2].m_fStartVol = PARAM(m_fSustainLevel);
			m_ADSRState[2].m_iNextSection = (AkUInt32)(PARAM(m_fSustainTime) * m_uSampleRate) + m_ADSRState[1].m_iNextSection;
			m_ADSRState[2].m_fStep = 0.0;
		}
		else
		{
			m_ADSRState[2].m_fStartVol = 1.0f;
			m_ADSRState[2].m_fStep = 0.0;
			m_ADSRState[2].m_iNextSection = -1;
		}

		if (PARAM(m_fReleaseTime) != 0.0)
		{
			//Release makes the volume go from m_fSustainLevel to 0 in m_fReleaseTime seconds.   
			AkReal32 fSamples = PARAM(m_fReleaseTime) * m_uSampleRate;
			m_ADSRState[3].m_fStep = -1.0f/fSamples * PARAM(m_fSustainLevel);
			m_ADSRState[3].m_fStartVol = PARAM(m_fSustainLevel);
		}
		else
		{
			m_ADSRState[3].m_fStep = 0.0;
			m_ADSRState[3].m_fStartVol = 0.0f;
		}

		ComputeDuration();
		m_ADSRState[3].m_iNextSection = m_uDuration-1;

		m_fVol = 0.0f;
	}
	else
	{
		//Just make sure that it is 0.0.  It will be used anyway in the other modes (fixed duration and period) so
		//we must make sure it is a no-op.
		m_ADSRState[0].m_fStep = 0.0;
		m_ADSRState[0].m_fStartVol = 1.0;
		m_fVol = 1.0f;
	}
	m_uSection = 0;

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: GetEffectType
// Desc: Effect type query.
//-----------------------------------------------------------------------------
// Info query:
// Effect type (source, monadic, mixer, ...)
// Buffer type: in-place, input(?), output (source), io.
AKRESULT CAkMotionGenerator::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
    out_rPluginInfo.eType = AkPluginTypeMotionSource;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Effect processing.
//-----------------------------------------------------------------------------
void CAkMotionGenerator::Execute(	AkAudioBuffer * io_pBufferOut // Output buffer interface
#ifdef AK_PS3
				, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	///< Asynchronous DSP process utilities on PS3
#endif
				)
{
    assert( io_pBufferOut != NULL );

	AkUInt32 uBufferSize = io_pBufferOut->MaxFrames() * m_uBytesPerSample;
	io_pBufferOut->eState = AK_DataReady;

	AkReal32 fSampleTime = PARAM(m_fPeriodMultiplier) * m_uSampleRate;

#ifdef RVL_OS
	fSampleTime	*= PARAM(m_fWiiPitch);
#endif

	fSampleTime = 1/fSampleTime;


	//Check when we need to stop for one loop (m_fDuration)
	//And check when we definitely need to stop (m_fDuration * loop count)
	ComputeDuration();
	if (m_uSamplesProduced > m_uDuration)
	{
		m_uSamplesProduced = m_uSamplesProduced - m_uDuration;
		m_fTime = (AkReal32)m_uSamplesProduced * fSampleTime;
		m_uLoops--;
	}

	if (m_uLoops == 0)
	{
		io_pBufferOut->eState = AK_NoMoreData;
		return ;
	}

	if(PARAM(m_eDurationType) == 2/*ADSR*/)
	{

		if ((AkInt32)m_uSamplesProduced >= 
#ifdef RVL_OS
			m_ADSRState[m_uSection].m_iNextSection * PARAM(m_fWiiPitch))
#else
			m_ADSRState[m_uSection].m_iNextSection)
#endif
		{
			//Move to the next section of ADSR.
			do
			{
				m_uSection++;
			}while(m_ADSRState[m_uSection].m_iNextSection == -1);

			m_fVol = m_ADSRState[m_uSection].m_fStartVol;
		}
	}

	const AkUInt32 iChannelCount = 2;
	AkReal32 fNextVol = m_ADSRState[m_uSection].m_fStep;
#ifdef RVL_OS
	fNextVol /= PARAM(m_fWiiPitch);
#endif

	for(AkUInt32 i = 0; i < io_pBufferOut->MaxFrames(); i++)
	{
		for(AkUInt32 iChannel = 0; iChannel < iChannelCount; iChannel++)
		{
			AkReal32 *pData = (AkReal32*)io_pBufferOut->GetChannel(iChannel) + i;
			*pData = PARAM(m_Curves[iChannel]).Convert(m_fTime) * m_fVol;
		}

		m_fVol += fNextVol;
		m_fTime += fSampleTime;
		if (m_fTime >= PARAM(m_fPeriod))
			m_fTime -= PARAM(m_fPeriod);
	}

	m_uSamplesProduced += io_pBufferOut->MaxFrames();

	// Notify buffers of updated production
	io_pBufferOut->uValidFrames = io_pBufferOut->MaxFrames();
}

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkTimeMs : duration of the source.
//
//-----------------------------------------------------------------------------
AkTimeMs CAkMotionGenerator::GetDuration( void ) const
{
	return static_cast<AkTimeMs>( const_cast<CAkMotionGenerator*>(this)->ComputeDuration() * 1000.f );
}

AKRESULT CAkMotionGenerator::StopLooping()
{
	m_uLoops = 1;

	return AK_Success;
}

AkReal32 CAkMotionGenerator::ComputeDuration()
{
	AkReal32 fDurationSec = 0;
	if (PARAM(m_eDurationType) == 0)		//One period
	{
		fDurationSec = PARAM(m_fPeriod) * PARAM(m_fPeriodMultiplier);
	}
	else if (PARAM(m_eDurationType) == 1)	//Fixed duration
	{
		fDurationSec = PARAM(m_fDuration);
	}
	else if (PARAM(m_eDurationType) == 2)	//ADSR
	{
		fDurationSec = PARAM(m_fAttackTime) + PARAM(m_fDecayTime) + PARAM(m_fSustainTime) + PARAM(m_fReleaseTime);
	}
#ifdef RVL_OS
	fDurationSec *= PARAM(m_fWiiPitch);
#endif

	m_uDuration = (AkUInt32)(fDurationSec * m_uSampleRate);
	return fDurationSec;
}

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: RandRange returns a random float value between in_fMin and in_fMax
//-----------------------------------------------------------------------------
AkReal32 CAkMotionGenerator::RandRange( AkReal32 in_fMin, AkReal32 in_fMax )
{
	// Get an integer in range (0,1.)
	AkReal32 fRandVal = rand() / static_cast<AkReal32>(RAND_MAX);
	return ( fRandVal * (in_fMax - in_fMin) + in_fMin );
}
