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
// AkCommon.h
//
// AudioLib common defines, enums, and structs.
//
//////////////////////////////////////////////////////////////////////

#ifndef _COMMON_H_
#define _COMMON_H_

// Additional "AkCommon" definition made public.
#include <AK/SoundEngine/Common/AkCommonDefs.h>

#ifndef __SPU__
#include "AkLEngineDefs.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Tools/Common/AkObject.h>
#endif

#include "AkSIMDSpeakerVolumes.h"

struct AkSoundParams;
class  CAkPBI;


//-----------------------------------------------------------------------------
// AUDIO FILES AND MARKERS
//-----------------------------------------------------------------------------

/// Defines the header of a block of markers.
struct AkMarkersHeader
{
    AkUInt32        uNumMarkers;		    ///< Number of markers
};

/// Defines the parameters of a marker.
struct AkAudioMarker
{     
    AkUInt32        dwIdentifier;           ///< Identifier.
    AkUInt32        dwPosition;             ///< Position in the audio data in sample frames.
	char*			strLabel;				///< Label of the marker taken from the file.
};

// Structure contained in the buffer to carry markers
struct AkBufferMarker
{
	CAkPBI*			pContext;
	AkUInt32		dwPositionInBuffer;
	AkAudioMarker	marker;
};

//-----------------------------------------------------------------------------
// Name: struct AkAudioMix
// Desc: Defines the parameters of a buffer to be used by mixers.
//-----------------------------------------------------------------------------
struct AkAudioMix
{
	// all values : [0, 1]
	AkSIMDSpeakerVolumes Next;
	AkSIMDSpeakerVolumes Previous;
} AK_ALIGNED_16;
//-----------------------------------------------------------------------------
// Name: struct AkListenerData
// Desc: Defines the parameters of a listener.
//-----------------------------------------------------------------------------
struct AkListenerData
{
	AkListenerPosition Pos;
	AkSIMDSpeakerVolumes VolumeOffset;	// per-speaker volume offset
	bool			   bSpatialized;	// false: attenuation only
};

//-----------------------------------------------------------------------------
// Name: struct AkBufferPosInformation
// Desc: Defines the position info for a source used by GetSourcePlayPosition()
//-----------------------------------------------------------------------------
struct AkBufferPosInformation
{
	AkUInt32	uStartPos;		//start position for data contained in the buffer
	AkReal32	fLastRate;		//last known pitch rate
	AkUInt32	uFileEnd;		//file end position
	AkUInt32	uSampleRate;	//file sample rate
};

//-----------------------------------------------------------------------------
// Name: class AkPipelineBufferBase
//-----------------------------------------------------------------------------
class AkPipelineBufferBase : public AkAudioBuffer 
{
public:
	AkPipelineBufferBase() { ClearData(); }

	void ClearData()
	{
#ifdef RVL_OS
		arData[0] = arData[1] = NULL;
#else
		pData				= NULL;
#endif
	}
	void Clear()
	{
		ClearData();
		uValidFrames		= 0;
		uMaxFrames			= 0;
		eState				= AK_DataNeeded;
	}

#ifdef RVL_OS

	void AttachData( void * in_pDataL, void * in_pDataR, AkUInt16 in_uValidFrames, AkChannelMask in_uChannelMask ) 
	{ 
		arData[0] = in_pDataL;
		arData[1] = in_pDataR;
		uValidFrames = in_uValidFrames; 
		uChannelMask = in_uChannelMask; 
	}
	
	// Logic.
	void SetRequestSize( AkUInt16 in_uMaxFrames ) 
	{
		AKASSERT( !arData[0] );
		uMaxFrames = in_uMaxFrames; 
	}
	
	bool HasData() { return ( NULL != arData[0] ); }

#else

	bool HasData() { return ( NULL != pData ); }

	// Buffer management.
	// ----------------------------

	// Logical.
	void SetRequestSize( AkUInt16 in_uMaxFrames ) 
	{
		AKASSERT( !pData );
		uMaxFrames = in_uMaxFrames; 
	}

	// Interleaved. Allocation is performed outside.
	void AttachInterleavedData( void * in_pData, AkUInt16 in_uMaxFrames, AkUInt16 in_uValidFrames, AkChannelMask in_uChannelMask )
	{ 
		AKASSERT( !pData || !"Trying to attach a buffer before having detached the previous one" );
		pData = in_pData; 
		uMaxFrames = in_uMaxFrames; 
		uValidFrames = in_uValidFrames; 
		uChannelMask = in_uChannelMask; 
	}

	// Contiguous deinterleaved. Allocation is performed outside.
	void AttachContiguousDeinterleavedData( void * in_pData, AkUInt16 in_uMaxFrames, AkUInt16 in_uValidFrames, AkChannelMask in_uChannelMask )
	{ 
		AttachInterleavedData( in_pData, in_uMaxFrames, in_uValidFrames, in_uChannelMask );
	}

	// Detach data from buffer, after it has been freed from managing instance.
	void DetachData()
	{
		AKASSERT( pData && uMaxFrames > 0 && NumChannels() > 0 );
		pData = NULL;
		uMaxFrames = 0;
		uValidFrames = 0;
		uChannelMask = 0;
	}

	void * GetContiguousDeinterleavedData()
	{
		return pData;
	}

	// Deinterleaved (pipeline API). Allocation is performed inside.
	AKRESULT GetCachedBuffer( 
		AkUInt16		in_uMaxFrames, 
		AkChannelMask	in_uChannelMask );
	void ReleaseCachedBuffer();

#endif

	// Override SetChannelMask(): Constraint-free assignation.
	AKRESULT SetChannelMask( AkChannelMask in_uChannelMask );
};

class AkPipelineBuffer
	: public AkPipelineBufferBase
{
public:
	AkPipelineBuffer()
		: pMarkers(NULL)
		, uNumMarkers( 0 )
	{}
	void Clear()
	{
		AkPipelineBufferBase::Clear();

		uNumMarkers			= 0;
		pMarkers			= NULL;
		posInfo.uStartPos	= 0xFFFFFFFF;
		posInfo.fLastRate	= 1.0f;
		posInfo.uFileEnd	= 0xFFFFFFFF;
		posInfo.uSampleRate	= 1;
	}

	// Markers.
	AkUInt16        uNumMarkers;        // Number of markers present in this buffer
	AkBufferMarker* pMarkers;           // List of markers present in this buffer
	AkBufferPosInformation posInfo;		// Position information for GetSourcePlayPosition
};

class AkAudioBufferMix
	: public AkPipelineBuffer
{
public:
	AkAudioMix		AudioMix[AK_VOICE_MAX_NUM_CHANNELS];	// Direct path volumes
};

// For buffers going into the final mix: direct channel mapping.
class AkAudioBufferFinalMix
	: public AkPipelineBufferBase
{
public:
	AkAudioMix		AudioMix;
};

class AkAudioBufferCbx
	: public AkAudioBufferMix
{
public:
	AkAudioMix		EnvMix[AK_VOICE_MAX_NUM_CHANNELS];		// Environment path volumes
	AkReal32		fObsLPF;		// Obstruction LPF
};

#ifndef __SPU__

inline void ZeroPadBuffer( AkAudioBuffer * io_pAudioBuffer )
{ 
	// Extra frames should be padded with 0's
	AkUInt32 uPadFrames = io_pAudioBuffer->MaxFrames() - io_pAudioBuffer->uValidFrames;
	if ( uPadFrames )
	{
		// Do all channels
		AkUInt32 uNumChannels = io_pAudioBuffer->NumChannels();
		for ( unsigned int uChanIter = 0; uChanIter < uNumChannels; ++uChanIter )
		{
			AkSampleType * pPadStart = io_pAudioBuffer->GetChannel(uChanIter) + io_pAudioBuffer->uValidFrames;
			for ( unsigned int uFrameIter = 0; uFrameIter < uPadFrames;  ++uFrameIter )
			{
				pPadStart[uFrameIter] = 0.f;
			}
		}
	}
}

inline void ZeroPrePadBuffer( AkAudioBuffer * io_pAudioBuffer, AkUInt32 in_ulNumFramestoZero )
{ 
	if ( in_ulNumFramestoZero )
	{
		// Do all channels
		AkUInt32 uNumChannels = io_pAudioBuffer->NumChannels();
		for ( unsigned int uChanIter = 0; uChanIter < uNumChannels; ++uChanIter )
		{
			AkSampleType * pPadStart = io_pAudioBuffer->GetChannel(uChanIter);
			for ( unsigned int uFrameIter = 0; uFrameIter < in_ulNumFramestoZero;  ++uFrameIter )
			{
				pPadStart[uFrameIter] = 0.f;
			}
		}
	}
}

//-----------------------------------------------------------------------------
/// Helpers for AkAudioFormat bitfield stored in banks.
//-----------------------------------------------------------------------------

#define AKAUDIOFORMAT_CHANNELMASK_MASK		0x0003FFFF ///< Mask for ChannelMask
#define AKAUDIOFORMAT_BITSPERSAMPLE_MASK	0x0000003F ///< Mask for BitsPerSample
#define AKAUDIOFORMAT_BLOCKALIGN_MASK		0x0000001F ///< Mask for BlockAlign
#define AKAUDIOFORMAT_TYPEID_MASK			0x00000003 ///< Mask for TypeID
#define AKAUDIOFORMAT_INTERLEAVEID_MASK		0x00000001 ///< Mask for InterleaveID

#define AKAUDIOFORMAT_CHANNELMASK_SHIFT		0  ///< Shift for ChannelMask
#define AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT	18 ///< Shift for BitsPerSample
#define AKAUDIOFORMAT_BLOCKALIGN_SHIFT		24 ///< Shift for BlockAlign
#define AKAUDIOFORMAT_TYPEID_SHIFT			29 ///< Shift for TypeID
#define AKAUDIOFORMAT_INTERLEAVEID_SHIFT	31 ///< Shift for InterleaveID

// Parse bitfield stored in banks to initialize an AkAudioFormat structure.
inline void SetFormatFromBank( AkUInt32 in_uSampleRate, AkUInt32 in_uFormatBits, AkAudioFormat & out_audioFormat )
{
	out_audioFormat.SetAll( 
		in_uSampleRate, 
		((in_uFormatBits >> AKAUDIOFORMAT_CHANNELMASK_SHIFT)	& AKAUDIOFORMAT_CHANNELMASK_MASK),
		((in_uFormatBits >> AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT)	& AKAUDIOFORMAT_BITSPERSAMPLE_MASK),
		((in_uFormatBits >> AKAUDIOFORMAT_BLOCKALIGN_SHIFT)		& AKAUDIOFORMAT_BLOCKALIGN_MASK),
		((in_uFormatBits >> AKAUDIOFORMAT_TYPEID_SHIFT)			& AKAUDIOFORMAT_TYPEID_MASK),
		((in_uFormatBits >> AKAUDIOFORMAT_INTERLEAVEID_SHIFT)	& AKAUDIOFORMAT_INTERLEAVEID_MASK)
		);
}

// Create a bitfield from an AkAudioFormat structure to be stored in banks.
inline void GetFormatForBank( const AkAudioFormat & in_audioFormat, AkUInt32 & out_uSampleRate, AkUInt32 & out_uFormatBits )
{
	out_uSampleRate = in_audioFormat.uSampleRate;
	out_uFormatBits = 
		( in_audioFormat.GetChannelMask()	<< AKAUDIOFORMAT_CHANNELMASK_SHIFT ) |
		( in_audioFormat.GetBitsPerSample() << AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT ) |
		( in_audioFormat.GetBlockAlign()	<< AKAUDIOFORMAT_BLOCKALIGN_SHIFT ) |
		( in_audioFormat.GetTypeID()		<< AKAUDIOFORMAT_TYPEID_SHIFT ) |
		( in_audioFormat.GetInterleaveID()	<< AKAUDIOFORMAT_INTERLEAVEID_SHIFT );
}


//-----------------------------------------------------------------------------
// Looping constants.
//-----------------------------------------------------------------------------
const AkInt16 LOOPING_INFINITE  = 0;
const AkInt16 LOOPING_ONE_SHOT	= 1;

#define IS_LOOPING( in_Value ) ( in_Value == LOOPING_INFINITE || in_Value > LOOPING_ONE_SHOT )

#endif

#endif // _COMMON_H_


