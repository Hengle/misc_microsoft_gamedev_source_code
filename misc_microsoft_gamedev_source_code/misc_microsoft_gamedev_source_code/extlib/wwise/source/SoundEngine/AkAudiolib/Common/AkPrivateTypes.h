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
// AkPrivateTypes.h
//
// Audiokinetic Data Type Definition (internal)
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKPRIVATETYPES_H
#define _AKPRIVATETYPES_H

// Moved to SDK.
#include <AK/SoundEngine/Common/AkTypes.h>

// Below: Internal only definitions.
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
#include "AudioLibLimitations.h"

//----------------------------------------------------------------------------------------------------
// Structs.
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// Macros.
//----------------------------------------------------------------------------------------------------

#define DECLARE_BASECLASS( baseClass )	\
	private:							\
		typedef baseClass __base		\

//----------------------------------------------------------------------------------------------------
// Enums
//----------------------------------------------------------------------------------------------------

enum AkVirtualQueueBehavior
{
	// See documentation for the definition of the behaviors.
	AkVirtualQueueBehavior_FromBeginning 	= 0,
	AkVirtualQueueBehavior_FromElapsedTime  = 1,
	AkVirtualQueueBehavior_Resume 			= 2
};

enum AkBelowThresholdBehavior
{
	// See documentation for the definition of the behaviors.
	AkBelowThresholdBehavior_ContinueToPlay 	= 0,
	AkBelowThresholdBehavior_KillVoice			= 1,
	AkBelowThresholdBehavior_SetAsVirtualVoice 	= 2
};

enum AkCurveInterpolation
{
//DONT GO BEYOND 15 HERE! (see below for details)
    AkCurveInterpolation_Log3			= 0,
    AkCurveInterpolation_Log2			= 1,
    AkCurveInterpolation_Log1			= 2,
    AkCurveInterpolation_InvSCurve		= 3,
    AkCurveInterpolation_Linear			= 4,
    AkCurveInterpolation_SCurve			= 5,
    AkCurveInterpolation_Exp1			= 6,
    AkCurveInterpolation_Exp2			= 7,
    AkCurveInterpolation_Exp3			= 8,
	AkCurveInterpolation_Constant		= 9
//DONT GO BEYOND 15 HERE! The value is stored on 5 bits,
//but we can use only 4 bits for the actual values, keeping
//the 5th bit at 0 to void problems when the value is
//expanded to 32 bits.
#define AKCURVEINTERPOLATION_NUM_STORAGE_BIT 5
};

///Transition mode selection
enum AkTransitionMode
{
//DONT ADD ANYTHING HERE, STORED ON 4 BITS!
	Transition_Disabled				= 0,	// Sounds are followed without any delay
	Transition_CrossFade			= 1,	// Sound 2 starts before sound 1 finished
	Transition_Delay				= 2,	// There is a delay before starting sound 2 once sound 1 terminated
	Transition_SampleAccurate		= 3,	// Next sound is prepared in advance and uses the same pipeline than the previous one
	Transition_TriggerRate			= 4		// Sound 2 starts after a fixed delay
//DONT ADD ANYTHING HERE, STORED ON 4 BITS!
#define TRANSITION_MODE_NUM_STORAGE_BIT 4
};

//----------------------------------------------------------------------------------------------------
// Common defines
//----------------------------------------------------------------------------------------------------

#define UNMUTED_LVL 255
#define MUTED_LVL	  0

#define AK_MAXIMUM_VOLUME_LEVEL (0.0f)
#define AK_MINIMUM_VOLUME_LEVEL (-96.3f)
#define AK_SAFE_MINIMUM_VOLUME_LEVEL (-97.0f) // Used for volume initialization and comparison to avoid problems with volume threshold.
#define AK_INVALID_PITCH_VALUE	(0x80000000)

#define AK_EVENTWITHCOOKIE_RESERVED_BIT 0x00000001
#define AK_EVENTFROMWWISE_RESERVED_BIT	0x40000000

#define DEFAULT_RANDOM_WEIGHT	50
#define NO_PLAYING_ID			0

#define AK_DEFAULT_PITCH						(0)
#define AK_LOWER_MIN_DISTANCE					(0.001f)
#define AK_UPPER_MAX_DISTANCE					(10000000000.0f)

#define AK_DEFAULT_DISTANCE_FACTOR				(1.0f)

#define	AK_DEFAULT_USE_DISTANCE_LOPASS_FILTER	false

// cone
#define	AK_DEFAULT_USE_CONE_LOPASS_FILTER		false

#define AK_DEFAULT_INSIDE_CONE_ANGLE			(PI / 4.0f)
#define AK_DEFAULT_OUTSIDE_CONE_ANGLE			(PI / 2.0f)
#define AK_OMNI_INSIDE_CONE_ANGLE				(TWOPI)
#define AK_OMNI_OUTSIDE_CONE_ANGLE				(TWOPI)
#define AK_DEFAULT_OUTSIDE_VOLUME				(-10.0f)

//(<255) Stored on AkUInt8 
#define	AK_MIN_LOPASS_VALUE						(0)
#define	AK_MAX_LOPASS_VALUE						(100) 
#define	AK_DEFAULT_LOPASS_VALUE					(0) 

#define AK_MIN_PAN_RL_VALUE						(-100)
#define AK_MAX_PAN_RL_VALUE						(100)
#define AK_DEFAULT_PAN_RL_VALUE					(0)

#define AK_MIN_PAN_FR_VALUE						(-100)
#define AK_MAX_PAN_FR_VALUE						(100)
#define AK_DEFAULT_PAN_FR_VALUE					(0)

#define AK_PAN_RIGHT							(100.0f)
#define AK_PAN_CENTER							(0.0f)
#define AK_PAN_LEFT								(-100.0f)
#define AK_DEFAULT_PAN_VALUE					(AK_PAN_CENTER)

#define	AK_MIN_LOPASS_VALUE						(0)
#define	AK_MAX_LOPASS_VALUE						(100) 
#define	AK_DEFAULT_LOPASS_VALUE					(0)

#define AK_DEFAULT_PATH_LOOP					(false)

//----------------------------------------------------------------------------------------------------
// Platform-specific defines
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// windows
//----------------------------------------------------------------------------------------------------
#ifdef WIN32
#endif //WIN32

//----------------------------------------------------------------------------------------------------
// X360
//----------------------------------------------------------------------------------------------------
#ifdef XBOX360
#endif //XBOX360

//----------------------------------------------------------------------------------------------------
// PS3
//----------------------------------------------------------------------------------------------------
#ifdef AK_PS3

#ifndef NULL
#define NULL (0)
#endif

#include <wchar.h>

#endif

//----------------------------------------------------------------------------------------------------
// Wii
//----------------------------------------------------------------------------------------------------
#ifdef RVL_OS
#include <wchar.h>
#endif

#endif // _AKPRIVATETYPES_H
