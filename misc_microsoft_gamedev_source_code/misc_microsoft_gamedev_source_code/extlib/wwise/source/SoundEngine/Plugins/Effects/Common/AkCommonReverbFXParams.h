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
// AkCommonReverbFXParams.h
//
// Parameters common to the environmental & non-environmental reverb effect.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_COMMONREVERBFXPARAMS_H_
#define _AK_COMMONREVERBFXPARAMS_H_

#include "AK\SoundEngine\Common\AkTypes.h"

// Parameters IDs for Wwise or RTPC.
static const AkPluginParamID AK_REVERBFXPARAM_WETDRYMIX_ID			= 0;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_REFLECTIONSLEVEL_ID	= 1;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_REVERBLEVEL_ID		= 2;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_REFLECTIONSDELAY_ID	= 3;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_REVERBDELAY_ID		= 4;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_DECAYTIME_ID			= 5;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_LPCUTOFFFREQ_ID		= 6;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_REVERBWIDTH_ID		= 7;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_MAINLEVEL_ID			= 8;	// RTPC
static const AkPluginParamID AK_REVERBFXPARAM_PROCESSLFE_ID			= 9;

static const AkPluginParamID AK_REVERBFXPARAM_NUM					= 10;

// Valid parameter ranges
#define REVERB_LEVEL_MIN			0.0f									// linear
#define REVERB_LEVEL_MAX			1.0f									// linear
#define REVERB_MAINLEVEL_MAX		3.981071705534972507702523050878f		// linear
#define REVERB_WETDRYMIX_MIN		0.0f									// Percent
#define REVERB_WETDRYMIX_DEF		50.0f									// Percent
#define REVERB_WETDRYMIX_MAX		100.0f									// Percent
#define REVERB_REFLECTIONSLEVEL_DEF	0.70794578438413791080221494218931f		// linear
#define REVERB_REVERBLEVEL_DEF		0.25118864315095801110850320677993f		// linear
#define REVERB_REFLECTIONSDELAY_MIN	0										// sec
#define REVERB_REFLECTIONSDELAY_DEF	0.01f									// sec
#define REVERB_REFLECTIONSDELAY_MAX	0.5f									// sec
#define REVERB_REVERBDELAY_MIN		0										// sec
#define REVERB_REVERBDELAY_DEF		0.04f									// sec
#define REVERB_REVERBDELAY_MAX		0.5f									// sec
#define REVERB_DECAYTIME_MIN		0.2f									// sec
#define REVERB_DECAYTIME_DEF		2.0f									// sec
#define REVERB_DECAYTIME_MAX		10.0f									// sec
#define REVERB_LPCUTOFFFREQ_MIN		20.0f									// Hz
#define REVERB_LPCUTOFFFREQ_DEF		7000.0f									// Hz
#define REVERB_LPCUTOFFFREQ_MAX		20000.0f								// Hz
#define REVERB_REVERBWIDTH_MIN		0.0f 									// No channel crosstalk
#define REVERB_REVERBWIDTH_DEF		0.0f									// Default value
#define REVERB_REVERBWIDTH_MAX		1.0f									// Full stereo width
#define REVERB_MAINLEVEL_DEF		1.0f									// linear
#define REVERB_PROCESSLFE_DEF		(true)

#endif // _AK_COMMONREVERBFXPARAMS_H_
