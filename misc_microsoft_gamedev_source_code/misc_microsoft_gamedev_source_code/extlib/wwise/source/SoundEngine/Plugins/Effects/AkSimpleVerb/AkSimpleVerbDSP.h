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
// AkSimpleVerbDSP.h
//
// SimpleVerbFX implementation.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SIMPLEVERBDSP_H_
#define _AK_SIMPLEVERBDSP_H_

#include "AkSimpleVerbFX.h"
#include <assert.h>

#include "AkSimpleVerbFXParams.h"

#include "AkCrossPlatformSIMD.h"

#define SCALEREVERBCOMPONENTSSIMD( __Gain__, __DryLevel__, __IOPtr__, __ScaledWet__, __vDry__, __vRev__ ) \
	AKSIMD_LOADVEC( (__IOPtr__), (__vDry__) );						\
	(__vDry__) = AKSIMD_MUL( (__vDry__), (__DryLevel__) );			\
	(__vRev__) = AKSIMD_ADD( (__ScaledWet__), (__vDry__) );			\
	(__vRev__) = AKSIMD_MUL( (__vRev__), (__Gain__) );				\
	AKSIMD_STOREVEC( (__vRev__), (__IOPtr__) );

#define SCALEREVERBCOMPONENTS( __Gain__, __DryLevel__, __IOPtr__, __ScaledWet__ ) \
	*(__IOPtr__) = (__Gain__) * ((__DryLevel__) * *(__IOPtr__)) + (__ScaledWet__);

#define SCALEREVERBCOMPONENTSSENT( __Gain__, __IOPtr__, __Reverb__ ) \
	*(__IOPtr__) = (__Gain__) * ( __Reverb__);

// sqrt(2)/numchannels
#define FIVEPOINTZERONORMFACTOR		(0.2828427f)
#define FIVEPOINTONENORMFACTOR		(0.2357378f)
#define STEREONORMFACTOR			(0.7071067f)

static const AkReal32 ONEOVER_SIMPLEVERBFXPARAM_WETDRYMIX_MAX = 1.f/SIMPLEVERBFXPARAM_WETDRYMIX_MAX;

#endif
