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

#pragma once

#include <AkSettings.h>
#include <PlatformAudiolibDefs.h>
#include "dboxconst.h"

//Maximum samples computed per audio buffer received.  
#define DBOX_MAX_REFILL_FRAMES (DBOX_SAMPLERATE * AK_NUM_VOICE_REFILL_FRAMES / 24000)
#define DBOX_NUM_REFILL_FRAMES AK_FEEDBACK_MAX_FRAMES_PER_BUFFER

//Number of repetitions of samples for up-sampling.
#define SAMPLE_MULTIPLIER ((float)DBOX_OUTPUT_RATE/DBOX_SAMPLERATE)

//Number of samples sent to output per buffer
#define DBOX_NUM_OUTPUT_FRAMES (DBOX_NUM_REFILL_FRAMES * SAMPLE_MULTIPLIER)

//Maximum acceleration is 9.8 m/s2 (gravity) * 1.2 to allow small bumps.
//Maximum speed is 0.2 m/s
#define MAX_ACCELERATION 9.8f * 1.2f	// m/s2
#define MAX_SPEED 0.1f			// m/s
#define MAX_DISPLACEMENT 0.012f // m

//Those values are good only with real measurements for the DBOX platform (displacement of 0.012m)
//However the signal is normalized to +1, -1.  The normalized values are:
// Vnorm = Xnorm * Vmax / Xmax = 1 * 0.1 / 0.012 
// Anorm = Xnorm * Amax / Xmax = 1 * 9.8 / 0.012
#define NORM_DISPLACEMENT 2.0f
#define AUDIOSAMPLE_INT_MAX 32767.0f

// Additional SIMD macros
__declspec(align(16)) const unsigned long M128_SIGNS_MASK[4] = {0x80000000,0x80000000,0x80000000,0x80000000};
__declspec(align(16)) const unsigned long M128_VALUE_MASK[4] = {0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF};

//Absolute value
#define _mm_abs_ps(__val) _mm_and_ps(__val, *(__m128*)(&M128_VALUE_MASK))

//Copy the sign of all 4 values in 3 steps: clear the sign(and) of the destination, get the signs(and) and set the signs(or).
#define _mm_copysign_ps(__val, __valSign) _mm_or_ps( _mm_abs_ps(__val), _mm_and_ps(__valSign, *(__m128*)(&M128_SIGNS_MASK)) )

