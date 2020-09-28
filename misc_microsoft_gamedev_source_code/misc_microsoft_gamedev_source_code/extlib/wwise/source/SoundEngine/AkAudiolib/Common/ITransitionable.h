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
// ITransitionable.h
//
// Interface that allows being updated by the transition manager
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#ifndef _ITRANSITIONNABLE_H_
#define _ITRANSITIONNABLE_H_


enum TransitionTargets
{
	TransTarget_UndefinedTarget	= 0x00000000,
	TransTarget_Volume			= 0x00010000,
	TransTarget_Pitch			= 0x00020000,
	TransTarget_LPF				= 0x00040000,
	TransTarget_Lfe				= 0x00080000,
	TransTarget_PlayStop		= 0x00200000,// not used anymore, only there for compatibility
	TransTarget_PauseResume		= 0x00400000,// not used anymore, only there for compatibility
	TransTarget_Mute			= 0x00800000,
	TransTarget_Play			= 0x01000000,
	TransTarget_Stop			= 0x02000000,
	TransTarget_Pause			= 0x04000000,
	TransTarget_Resume			= 0x08000000,
	TransTarget_Path			= 0x10000000,// PhM : transition between path vertices
	TransTarget_FeedbackVolume  = 0x20000000,
	TransTarget_TargetMask		= 0xFFFF0000
};

enum TransitionTypes
{
	AkUndefinedType	= 0x00000000,
	AkTypeFloat		= 0x00000001,
	AkTypeLong		= 0x00000002,
	AkTypeWord		= 0x00000004,
	AkTypeShort		= 0x00000008,
	AkTypeByte		= 0x00000010,
	AkTypeMask		= 0x0000FFFF
};

// the supported ones
enum TransitionTargetTypes
{
	AkUndefinedTargetType = TransTarget_UndefinedTarget | AkUndefinedType,

	// volume
	AkVolumeFloat = TransTarget_Volume | AkTypeFloat,
	AkVolumeLong = TransTarget_Volume | AkTypeLong,
	AkVolumeWord = TransTarget_Volume | AkTypeWord,
	AkVolumeByte = TransTarget_Volume | AkTypeByte,

	// pitch
	AkPitchFloat = TransTarget_Pitch | AkTypeFloat,
	AkPitchLong =  TransTarget_Pitch | AkTypeLong,
	AkPitchWord = TransTarget_Pitch | AkTypeWord,
	AkPitchByte = TransTarget_Pitch | AkTypeByte,

	// LPF
	AkLPFFloat = TransTarget_LPF | AkTypeFloat,
	AkLPFLong = TransTarget_LPF | AkTypeLong,
	AkLPFWord =	TransTarget_LPF | AkTypeWord,
	AkLPFyte =	TransTarget_LPF | AkTypeByte,

	// lfe
	AkLfeFloat =  TransTarget_Lfe | AkTypeFloat,
	AkLfeLong =  TransTarget_Lfe | AkTypeLong,
	AkLfeWord = TransTarget_Lfe | AkTypeWord,
	AkLfeByte = TransTarget_Lfe | AkTypeByte,

	// Pay / stop
	AkPlayStopFloat = TransTarget_PlayStop | AkTypeFloat,
	AkPlayStopLong = TransTarget_PlayStop | AkTypeLong,
	AkPlayStopWord = TransTarget_PlayStop | AkTypeWord,
	AkPlayStopByte = TransTarget_PlayStop | AkTypeByte,

	AkPathFloat = TransTarget_Path | AkTypeFloat,

	// FeedbackVolume
	AkFeedbackVolumeFloat = TransTarget_FeedbackVolume | AkTypeFloat,
	AkFeedbackVolumeLong = TransTarget_FeedbackVolume | AkTypeLong,
	AkFeedbackVolumeWord = TransTarget_FeedbackVolume | AkTypeWord,
	AkFeedbackVolumeByte = TransTarget_FeedbackVolume | AkTypeByte,
};


union TransitionTarget
{
	AkReal32		fValue;
	AkInt32		lValue;
	AkUInt16	wValue;
	AkInt16		sValue;
	AkUInt8		ucValue;
};

// Interface that allows being updated by the transition manager
//
// Author:  alessard

class ITransitionable
{
public:

    virtual void TransUpdateValue(TransitionTargetTypes in_eTargetType, TransitionTarget in_unionValue, bool in_bIsTerminated) = 0;
};
#endif
