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
// AkRTPC.h
//
// Data structures used for RTPC.
//
//////////////////////////////////////////////////////////////////////
#ifndef _RTPC_H_
#define _RTPC_H_

#include "AkPrivateTypes.h"

// Every point of a conversion graph
template< class VALUE_TYPE >
struct AkRTPCGraphPointBase
{
	AkReal32				From;		// Representing X Axis
	VALUE_TYPE				To;	    	// Representing Y Axis
	AkCurveInterpolation	Interp;		// How to interpolate between this point and the next
};

// Every point of a conversion graph
typedef AkRTPCGraphPointBase<AkReal32> AkRTPCGraphPoint;

// Points in switch conversion curves
typedef AkRTPCGraphPointBase<AkUInt32> AkRTPCGraphPointInteger;

// Points in a Layer crossfade curve. Note that the value is actually 0-255
// but the way curves are loaded from banks requires both the From and To to
// to be on 32 bits, hence the use of AkReal32.
typedef AkRTPCGraphPointBase<AkReal32> AkRTPCCrossfadingPoint;

enum AkCurveScaling
{
	AkCurveScaling_None							= 0, // No special scaling
	AkCurveScaling_db_255						= 1, // dB scaling, range 0 to 255
	AkCurveScaling_dB_96_3						= 2, // dB scaling, range -96.3 to +96.3
};

// IDs of the AudioLib known RTPC capable parameters
enum AkRTPC_ParameterID
{
	// Main Audionode
	RTPC_Volume										= 0,
	RTPC_LFE										= 1,
	RTPC_Pitch										= 2,

	// Main LPF
	RTPC_LPF										= 3,

	// Positioning - Parameters common to Radius and Position
	RTPC_Positioning_Radius_LPF						= 10,
	RTPC_Positioning_Divergence_Center_PCT			= 11,
	RTPC_Positioning_Cone_Attenuation_ON_OFF		= 12,
	RTPC_Positioning_Cone_Attenuation				= 13,
	RTPC_Positioning_Cone_LPF						= 14,

	// Positioning - Position specific
	RTPC_Position_PAN_RL							= 20,
	RTPC_Position_PAN_FR							= 21,
	RTPC_Position_Radius_SIM_ON_OFF					= 22,
	RTPC_Position_Radius_SIM_Attenuation			= 23,

	// The 4 effect slots
	RTPC_BypassFX0									= 24,
	RTPC_BypassFX1									= 25,
	RTPC_BypassFX2									= 26,
	RTPC_BypassFX3									= 27,
	RTPC_BypassAllFX								= 28, // placed here to follow bit order in bitfields

	RTPC_FeedbackVolume								= 29,
	RTPC_FeedbackLowpass							= 30,

	RTPC_MaxNumRTPC									= 32

	// Should not exceed a value of 32, if it does, then the m_RTPCBitArrayMax32 optimization should be removed
	// 32 == MAX_BITFIELD_NUM
};

#endif //_RTPC_H_
