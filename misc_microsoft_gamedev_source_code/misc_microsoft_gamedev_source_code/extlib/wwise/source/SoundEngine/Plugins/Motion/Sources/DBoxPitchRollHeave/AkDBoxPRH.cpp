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
// AkPitchRollHeave.cpp
//
// Copyright (c) 2006-2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkDBoxPRH.h"
#include "assert.h"
#include "DBoxPRHMaths.h"

using namespace AK;

#define DBOX_FRONT_RIGHT	(0)
#define DBOX_FRONT_LEFT		(1)
#define DBOX_BACK_RIGHT		(2)
#define DBOX_BACK_LEFT		(3)

#define PARAM(__name) m_pSharedParams->GetParams().__name

// Plugin mechanism. Implement Create function and register its address to the FX manager.
IAkPluginParam * CreateDBoxPitchRollHeaveParams( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkPitchRollHeaveParams( in_pAllocator ) );
}

// Constructor.
CAkPitchRollHeaveParams::CAkPitchRollHeaveParams( IAkPluginMemAlloc * in_pAllocator )
{
	m_pAllocator = in_pAllocator;
	m_Params.m_fPitch		= 0.0f;
	m_Params.m_fRoll		= 0.0f;
	m_Params.m_fHeave		= 0.0f;
	m_Params.m_fDuration	= 0.0f;
	m_Params.m_fFrontLeft	= 0.0f;
	m_Params.m_fFrontRight	= 0.0f;
	m_Params.m_fBackLeft	= 0.0f;
	m_Params.m_fBackRight	= 0.0f;
#ifndef AK_OPTIMIZED
	m_Params.m_bSimulate	= false;
#endif
}

// Copy constructor.
CAkPitchRollHeaveParams::CAkPitchRollHeaveParams( const CAkPitchRollHeaveParams &Copy )
{
	m_pAllocator					=Copy.m_pAllocator;
	m_Params.m_fPitch				=Copy.m_Params.m_fPitch;
	m_Params.m_fRoll				=Copy.m_Params.m_fRoll;
	m_Params.m_fHeave				=Copy.m_Params.m_fHeave;
	m_Params.m_fDuration			=Copy.m_Params.m_fDuration;
	m_Params.m_fFrontLeft			=Copy.m_Params.m_fFrontLeft;
	m_Params.m_fFrontRight			=Copy.m_Params.m_fFrontRight;
	m_Params.m_fBackLeft			=Copy.m_Params.m_fBackLeft;
	m_Params.m_fBackRight			=Copy.m_Params.m_fBackRight;

#ifndef AK_OPTIMIZED
	m_Params.m_bSimulate			=Copy.m_Params.m_bSimulate;
#endif

	for(AkUInt32 i = 0; i < AK_NUM_CURVES; i++)
	{
		AkCurve &rCurve = m_Params.m_Curves[i];
		const AkCurve &rSrcCurve = Copy.m_Params.m_Curves[i];
		rCurve.Set(rSrcCurve.m_pArrayGraphPoints, rSrcCurve.m_ulArraySize, AkCurveScaling_None);
	}
}

// Destructor.
CAkPitchRollHeaveParams::~CAkPitchRollHeaveParams()
{
	for(AkUInt32 i = 0; i < AK_NUM_CURVES; i++)
		m_Params.m_Curves[i].Unset();
}

// Create shared parameters duplicate.
IAkPluginParam * CAkPitchRollHeaveParams::Clone( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkPitchRollHeaveParams( *this ) );
}

// Shared parameters initialization.
AKRESULT CAkPitchRollHeaveParams::Init( IAkPluginMemAlloc *	in_pAllocator,									  
	                                  void *			in_pvParamsBlock, 
	                                  AkUInt32			in_ulBlockSize 
                                 )
{
    if ( in_ulBlockSize == 0)
        return AK_Success;
	 
    return SetParamsBlock( in_pvParamsBlock, in_ulBlockSize );
}

// Shared parameters termination.
AKRESULT CAkPitchRollHeaveParams::Term( IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Set all shared parameters at once.
AKRESULT CAkPitchRollHeaveParams::SetParamsBlock( void * in_pvParamsBlock, 
                                                AkUInt32 in_ulBlockSize
                                              )
{

    assert( NULL != in_pvParamsBlock && in_ulBlockSize >= sizeof(AkPitchRollHeaveParams));
    if ( NULL == in_pvParamsBlock || in_ulBlockSize < sizeof(AkPitchRollHeaveParams) )
    {
        return AK_InvalidParameter;
    }

	LockParams( );
	AkUInt32 lFixedSize = (AkUInt32)((char*)&m_Params.m_Curves - (char*)&m_Params);
	memcpy( &m_Params, in_pvParamsBlock, lFixedSize);

	in_pvParamsBlock = (char*)in_pvParamsBlock + lFixedSize;

	AKRESULT eResult = ReadAllCurves(in_pvParamsBlock);

    UnlockParams( );

    return eResult;
}

// Update one parameter.
AKRESULT CAkPitchRollHeaveParams::SetParam( AkPluginParamID in_ParamID,
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
	case AK_Pitch_Param:
		m_Params.m_fPitch = UserToAngle(*(AkReal32*)in_pvValue);
		break;
	case AK_Roll_Param:
		m_Params.m_fRoll = UserToAngle(*(AkReal32*)in_pvValue);
		break;
	case AK_Heave_Param:	
		m_Params.m_fHeave = UserToHeave(*(AkReal32*)in_pvValue);
		break;
	case AK_FrontLeft_Param:
		m_Params.m_fFrontLeft = UserToHeave(*(AkReal32*)in_pvValue);
		break;
	case AK_FrontRight_Param:	
		m_Params.m_fFrontRight = UserToHeave(*(AkReal32*)in_pvValue);
		break;
	case AK_BackLeft_Param:	
		m_Params.m_fBackLeft = UserToHeave(*(AkReal32*)in_pvValue);
		break;
	case AK_BackRight_Param:	
		m_Params.m_fBackRight= UserToHeave(*(AkReal32*)in_pvValue);
		break;
	case AK_Duration_Param:		
		m_Params.m_fDuration = *(AkReal32*)in_pvValue;
		break;

#ifndef AK_OPTIMIZED
	case AK_Simulate_Param:
		SetSimulate(*(bool*)in_pvValue);
		break;
#endif

	case AK_Curves_Param:
		eResult = ReadAllCurves(in_pvValue);
		break;

	case AK::IAkPluginParam::ALL_PLUGIN_DATA_ID:	
#ifndef AK_OPTIMIZED
		SetSimulate(*(bool*)in_pvValue);
#endif
		void* pCurves = (char*)in_pvValue + sizeof(bool);
		eResult = ReadAllCurves(pCurves);
		break;
    }

    UnlockParams( );

    return eResult ;
}

AKRESULT CAkPitchRollHeaveParams::ReadCurve(AkUInt16 in_iIndex, void* &io_pData)
{
	AkCurve &rCurve = m_Params.m_Curves[in_iIndex];
	AkUInt32* pCount = (AkUInt32*)io_pData;
	AkRTPCGraphPoint *pPoints = (AkRTPCGraphPoint *)(pCount + 1);
	if (*pCount > 0 )
	{
		rCurve.Set(pPoints, *pCount, AkCurveScaling_None);
	}

	//Advance the pointer at the end of the structure we just read.
	io_pData = (void*)(pPoints + *pCount);
	return AK_Success;
}

AKRESULT CAkPitchRollHeaveParams::ReadAllCurves(void* &io_pData)
{
	AKRESULT eResult = AK_Success;
	for(int i = 0; i < 3 && eResult == AK_Success; i++)
		eResult = ReadCurve(i, io_pData);

	return eResult;
}

#ifndef AK_OPTIMIZED
void CAkPitchRollHeaveParams::SetSimulate(bool in_bEnabled)
{
	if (m_Params.m_bSimulate && !in_bEnabled)
	{
		//Also reset all other parameters to zero.  If we left them to their current value, there would
		//be a jolt the next time we started a playback.
		m_Params.m_fPitch		= 0.0f;
		m_Params.m_fRoll		= 0.0f;
		m_Params.m_fHeave		= 0.0f;
		m_Params.m_fFrontLeft	= 0.0f;
		m_Params.m_fFrontRight	= 0.0f;
		m_Params.m_fBackLeft	= 0.0f;
		m_Params.m_fBackRight	= 0.0f;
	}
	m_Params.m_bSimulate = in_bEnabled;
}
#endif

//-----------------------------------------------------------------------------
// Name: CreateEffect
// Desc: Plugin mechanism. Dynamic create function whose address must be 
//       registered to the FX manager.
//-----------------------------------------------------------------------------
IAkPlugin* CreateDBoxPitchRollHeave( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkPitchRollHeave( ) );
}

//-----------------------------------------------------------------------------
// Name: CAkPitchRollHeave
// Desc: Constructor.
//-----------------------------------------------------------------------------
CAkPitchRollHeave::CAkPitchRollHeave()
{
	// Initialize members.
	m_uSampleRate = 0;
	m_uBytesPerSample = 0;
	m_pSourceFXContext = NULL;
	m_pSharedParams = NULL;
	m_uDuration = 0;
	m_uSamplesProduced = 0;
	m_uLoops = 0;	

	m_fOldPitch = 0;
	m_fOldRoll = 0;
	m_fOldHeave = 0;
	m_fOldFrontLeft = 0;
	m_fOldFrontRight = 0;
	m_fOldBackLeft = 0;
	m_fOldBackRight = 0;
}

//-----------------------------------------------------------------------------
// Name: ~CAkPitchRollHeave
// Desc: Destructor.
//-----------------------------------------------------------------------------
CAkPitchRollHeave::~CAkPitchRollHeave()
{
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Init.
//-----------------------------------------------------------------------------
AKRESULT CAkPitchRollHeave::Init( IAkPluginMemAlloc *			in_pAllocator,		// Memory allocator interface.
								IAkSourcePluginContext *	in_pSourceFXContext,// Source FX context
								IAkPluginParam *			in_pParams,			// Effect parameters.
								AkAudioFormat &				io_rFormat			// Supported audio output format.
								)
{
	// Keep source FX context (looping etc.)
	m_pSourceFXContext = in_pSourceFXContext;
	m_pSharedParams = reinterpret_cast<CAkPitchRollHeaveParams*>(in_pParams);

	io_rFormat.uChannelMask = AK_SPEAKER_FRONT_LEFT | AK_SPEAKER_FRONT_RIGHT | AK_SPEAKER_BACK_LEFT | AK_SPEAKER_BACK_RIGHT;

	// Save audio format internally
	m_uSampleRate = io_rFormat.uSampleRate;
	m_uBytesPerSample = io_rFormat.GetBitsPerSample() / 8;
	m_fOneSample = 1.0f/m_uSampleRate;

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Term.
// Desc: Term. The effect must destroy itself herein
//-----------------------------------------------------------------------------
AKRESULT CAkPitchRollHeave::Term( IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Reset
// Desc: Reset or seek to start (looping).
//-----------------------------------------------------------------------------
AKRESULT CAkPitchRollHeave::Reset( void )
{
	m_uSamplesProduced = 0;
	m_uDuration = (AkUInt32)ceil(PARAM(m_fDuration) * m_uSampleRate);
	m_fTime = 0.0f;

	m_uLoops = m_pSourceFXContext->GetNumLoops();
	if (m_uLoops == 0 
#ifndef AK_OPTIMIZED
		|| PARAM(m_bSimulate)
#endif
		)
		m_uLoops = 0xFFFFFFFF;

	m_fOldPitch = PARAM(m_fPitch);
	m_fOldRoll = PARAM(m_fRoll);
	m_fOldHeave = PARAM(m_fHeave);
	m_fOldFrontLeft = PARAM(m_fFrontLeft);
	m_fOldFrontRight= PARAM(m_fFrontRight);
	m_fOldBackLeft  = PARAM(m_fBackLeft);
	m_fOldBackRight = PARAM(m_fBackRight); 

#ifndef AK_OPTIMIZED
	m_bOldSimulate = PARAM(m_bSimulate);
#endif

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: GetEffectType
// Desc: Effect type query.
//-----------------------------------------------------------------------------
// Info query:
// Effect type (source, monadic, mixer, ...)
// Buffer type: in-place, input(?), output (source), io.
AKRESULT CAkPitchRollHeave::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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
void CAkPitchRollHeave::Execute( AkAudioBuffer * io_pBufferOut )	// Output buffer interface
{
    assert( io_pBufferOut != NULL );
	assert(io_pBufferOut->NumChannels() == 4);

	if (io_pBufferOut->NumChannels() != 4)
	{
		io_pBufferOut->eState = AK_Fail;
		return;
	}

	io_pBufferOut->eState = AK_DataReady;

	//Check when we need to stop for one loop (m_fDuration)
	//And check when we definitely need to stop (m_fDuration * loop count)
	if (m_uSamplesProduced >= m_uDuration)
	{
		m_uSamplesProduced = m_uSamplesProduced - m_uDuration;
		m_fTime = m_uSamplesProduced * m_fOneSample;
		m_uLoops--;
	}

	if (m_uLoops == 0 
#ifndef AK_OPTIMIZED
		|| m_bOldSimulate != PARAM(m_bSimulate)
#endif
		)
	{
		io_pBufferOut->eState = AK_NoMoreData;
		return ;
	}

	//Eval the 3 curves for each samples and compute the 4 actuator position.
	AkReal32 *pData[4];
	pData[DBOX_FRONT_RIGHT] = io_pBufferOut->GetChannel(DBOX_FRONT_RIGHT);
	pData[DBOX_FRONT_LEFT] = io_pBufferOut->GetChannel(DBOX_FRONT_LEFT);
	pData[DBOX_BACK_RIGHT] = io_pBufferOut->GetChannel(DBOX_BACK_RIGHT);
	pData[DBOX_BACK_LEFT] = io_pBufferOut->GetChannel(DBOX_BACK_LEFT);

	//Compute RTPC driven transitions steps
	AkReal32 fStep = 1.f / io_pBufferOut->MaxFrames();
	AkReal32 fPitch = m_fOldPitch;
	AkReal32 fRoll = m_fOldRoll;
	AkReal32 fHeave = m_fOldHeave;
	AkReal32 fFrontLeft = m_fOldFrontLeft;
	AkReal32 fFrontRight =m_fOldFrontRight; 
	AkReal32 fBackLeft =  m_fOldBackLeft;
	AkReal32 fBackRight = m_fOldBackRight;

	AkReal32 fPitchStep = (PARAM(m_fPitch) - fPitch) * fStep ;
	AkReal32 fRollStep = (PARAM(m_fRoll) - fRoll ) * fStep ;
	AkReal32 fHeaveStep = (PARAM(m_fHeave) - fHeave) * fStep ;
	AkReal32 fFrontLeftStep  = (PARAM(m_fFrontLeft) - fFrontLeft) * fStep ;
	AkReal32 fFrontRightStep = (PARAM(m_fFrontRight)- fFrontRight) * fStep ;
	AkReal32 fBackLeftStep	 = (PARAM(m_fBackLeft)  - fBackLeft) * fStep ;
	AkReal32 fBackRightStep  = (PARAM(m_fBackRight) - fBackRight) * fStep ;

	for(AkUInt16 i = 0; i < io_pBufferOut->MaxFrames(); i++)
	{
		AkReal32 fCurvePitch = 0.0f;
		AkReal32 fCurveRoll = 0.0f;
		AkReal32 fCurveHeave = 0.0f;

#ifndef AK_OPTIMIZED
		if (!PARAM(m_bSimulate))
		{
			fCurvePitch = PARAM(m_Curves[CurvePitch].Convert(m_fTime));
			fCurveRoll  = PARAM(m_Curves[CurveRoll].Convert(m_fTime));
			fCurveHeave = PARAM(m_Curves[CurveHeave].Convert(m_fTime));
		}
#endif

		//Add the pitch component (Curve + RTPC value)
		AkReal32 fPitchComponent = c_fSide * AkMath::Sin(fCurvePitch + fPitch);
		*pData[DBOX_FRONT_LEFT]		= -fPitchComponent;
		*pData[DBOX_FRONT_RIGHT]	= -fPitchComponent;
		*pData[DBOX_BACK_RIGHT]		= fPitchComponent;
		*pData[DBOX_BACK_LEFT]		= fPitchComponent;

		//Add the roll component (Curve + RTPC value)
		AkReal32 fRollComponent = c_fSide * AkMath::Sin(fCurveRoll + fRoll);
		*pData[DBOX_FRONT_RIGHT]	+= -fRollComponent ;
		*pData[DBOX_BACK_RIGHT]		+= -fRollComponent;
		*pData[DBOX_BACK_LEFT]		+= fRollComponent;
		*pData[DBOX_FRONT_LEFT]		+= fRollComponent;

		//Add the vertical component (Curve + RTPC value)
		AkReal32 fHeaveComponent = fCurveHeave + fHeave;
		*pData[DBOX_FRONT_LEFT]		+= fHeaveComponent;
		*pData[DBOX_FRONT_RIGHT]	+= fHeaveComponent;
		*pData[DBOX_BACK_LEFT]		+= fHeaveComponent;
		*pData[DBOX_BACK_RIGHT]		+= fHeaveComponent;

		//Add each actuator RTPC.  Normally, it should not be used in conjunction with PRH).
		*pData[DBOX_FRONT_LEFT]		+= fFrontLeftStep;
		*pData[DBOX_FRONT_RIGHT]	+= fFrontRightStep;
		*pData[DBOX_BACK_LEFT]		+= fBackLeftStep;	
		*pData[DBOX_BACK_RIGHT]		+= fBackRightStep; 

		*pData[DBOX_FRONT_LEFT]		*= c_fNormalizeHeave;
		*pData[DBOX_FRONT_RIGHT]	*= c_fNormalizeHeave;
		*pData[DBOX_BACK_LEFT]		*= c_fNormalizeHeave;
		*pData[DBOX_BACK_RIGHT]		*= c_fNormalizeHeave;

		pData[DBOX_FRONT_LEFT]++;
		pData[DBOX_FRONT_RIGHT]++;
		pData[DBOX_BACK_LEFT]++;
		pData[DBOX_BACK_RIGHT]++; 

		m_fTime += m_fOneSample;
		if (m_fTime >= PARAM(m_fDuration))
			m_fTime -= PARAM(m_fDuration);
		
		fPitch += fPitchStep;
		fRoll +=  fRollStep;
		fHeave += fHeaveStep;
	}

	//Keep the RTPC offsets we just completed.
	m_fOldPitch = PARAM(m_fPitch);
	m_fOldRoll = PARAM(m_fRoll);
	m_fOldHeave = PARAM(m_fHeave);
	m_fOldFrontLeft = PARAM(m_fFrontLeft);
	m_fOldFrontRight= PARAM(m_fFrontRight);
	m_fOldBackLeft  = PARAM(m_fBackLeft);
	m_fOldBackRight = PARAM(m_fBackRight); 

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
AkTimeMs CAkPitchRollHeave::GetDuration( void ) const
{
	return static_cast<AkTimeMs>( PARAM(m_fDuration) * 1000.f );
}

AKRESULT CAkPitchRollHeave::StopLooping()
{
	m_uLoops = 1;

	return AK_Success;
}