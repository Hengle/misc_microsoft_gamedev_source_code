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
// AkReverbDSP.h
//
// ReverbFX implementation.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_REVERBDSP_H_
#define _AK_REVERBDSP_H_

#include "AkDSPUtils.h"
#include <assert.h>

#include "AkReverbFXParams.h"

#include "AkCrossPlatformSIMD.h"
#define SCALEREVERBCOMPONENTSSINGLESIMD( __Gain__, __DryLevel__, __IOPtr__, __WetLevel__ , __RefLevel__, __RefPtr__, __ReverbLevel__, __ReverbPtr__ ) \
	AkReal32Vector __vDry__, __vRef__, __vRev__;					\
	AKSIMD_LOADVEC( (__IOPtr__), (__vDry__) );						\
	AKSIMD_LOADVEC( (__RefPtr__), (__vRef__) );						\
	AKSIMD_LOADVEC( (__ReverbPtr__), (__vRev__) );					\
	__vDry__ = AKSIMD_MUL( (__vDry__), (__DryLevel__) );			\
	__vRef__ = AKSIMD_MUL( (__vRef__), (__RefLevel__) );			\
	__vRev__ = AKSIMD_MUL( (__vRev__), (__ReverbLevel__) );			\
	__vRev__ = AKSIMD_ADD( (__vRev__), (__vRef__) );				\
	__vRev__ = AKSIMD_MUL( (__vRev__), (__WetLevel__) );			\
	__vRev__ = AKSIMD_ADD( (__vRev__), (__vDry__) );				\
	__vRev__ = AKSIMD_MUL( (__vRev__), (__Gain__) );				\
	AKSIMD_STOREVEC( (__vRev__), (__IOPtr__) );

#define SCALEREVERBCOMPONENTSSINGLESIMDSENT( __OPtr__, __Gain__, __RefLevel__, __RefPtr__, __ReverbLevel__, __ReverbPtr__ ) \
	AkReal32Vector __vRef__, __vRev__;								\
	AKSIMD_LOADVEC( (__RefPtr__), (__vRef__) );						\
	AKSIMD_LOADVEC( (__ReverbPtr__), (__vRev__) );					\
	__vRef__ = AKSIMD_MUL( (__vRef__), (__RefLevel__) );			\
	__vRev__ = AKSIMD_MUL( (__vRev__), (__ReverbLevel__) );			\
	__vRev__ = AKSIMD_ADD( (__vRev__), (__vRef__) );				\
	__vRev__ = AKSIMD_MUL( (__vRev__), (__Gain__) );				\
	AKSIMD_STOREVEC( (__vRev__), (__OPtr__) );

#define SCALEREVERBCOMPONENTSDUALSIMD( __Gain__, __DryLevel__, __IOPtr__, __WetLevel__, __RefLevel__, __RefPtr__, __vReverbMix__ ) \
	AKSIMD_LOADVEC( (__IOPtr__), (__vDry__) );						\
	AKSIMD_LOADVEC( (__RefPtr__), (__vRef__) );						\
	(__vDry__) = AKSIMD_MUL( (__vDry__), (__DryLevel__) );			\
	(__vRef__) = AKSIMD_MUL( (__vRef__), (__RefLevel__) );			\
	(__vRev__) = AKSIMD_ADD( (__vReverbMix__), (__vRef__) );		\
	(__vRev__) = AKSIMD_MUL( (__vRev__), (__WetLevel__) );			\
	(__vRev__) = AKSIMD_ADD( (__vRev__), (__vDry__) );				\
	(__vRev__) = AKSIMD_MUL( (__vRev__), (__Gain__) );				\
	AKSIMD_STOREVEC( (__vRev__), (__IOPtr__) );

#define SCALEREVERBCOMPONENTSDUALSIMDSENT( __OPtr__, __Gain__, __RefLevel__, __RefPtr__, __vReverbMix__ ) \
	AKSIMD_LOADVEC( (__RefPtr__), (__vRef__) );						\
	(__vRef__) = AKSIMD_MUL( (__vRef__), (__RefLevel__) );			\
	(__vRev__) = AKSIMD_ADD( (__vReverbMix__), (__vRef__) );		\
	(__vRev__) = AKSIMD_MUL( (__vRev__), (__Gain__) );				\
	AKSIMD_STOREVEC( (__vRev__), (__OPtr__) );

#define COMPUTEREVERBUNITMIXSIMD( __ReverbLevel__, __ReverbBufPtr1__, __ReverbBufPtr2__, __RevUnitScale1__, __RevUnitScale2__, __ReverbMix1__, __ReverbMix2__ ) \
	AkReal32Vector __vRev1__, __vRev2__; \
	AKSIMD_LOADVEC( (__ReverbBufPtr1__), (__vRev1__) ); \
	AKSIMD_LOADVEC( (__ReverbBufPtr2__), (__vRev2__) );		\
	(__vRev1__) = AKSIMD_MUL((__ReverbLevel__),(__vRev1__));																				\
	(__vRev2__) = AKSIMD_MUL((__ReverbLevel__),(__vRev2__));																				\
	(__ReverbMix1__) = AKSIMD_ADD( AKSIMD_MUL((__RevUnitScale1__),(__vRev1__)), AKSIMD_MUL((__RevUnitScale2__),(__vRev2__)) );				\
	(__ReverbMix2__) = AKSIMD_ADD( AKSIMD_MUL((__RevUnitScale1__),(__vRev2__)), AKSIMD_MUL((__RevUnitScale2__),(__vRev1__)) );

#define COMPUTEREVERBUNITMIXSIMDSURROUND( __ReverbLevel__, __ReverbBufPtr1__, __ReverbBufPtr2__, __RevUnitScale1__, __RevUnitScale2__, __ReverbMix1__, __ReverbMix2__, __ReverbMix3__ ) \
	AkReal32Vector __vRev1__, __vRev2__, __POINTFIVE__; \
	AKSIMD_LOADVEC( (__ReverbBufPtr1__), (__vRev1__) ); \
	AKSIMD_LOADVEC( (__ReverbBufPtr2__), (__vRev2__) ); \
	AkReal32 __fZeroPointFive__ = 0.5f; \
	AKSIMD_LOAD1( __fZeroPointFive__, __POINTFIVE__ ); \
	(__vRev1__) = AKSIMD_MUL((__ReverbLevel__),(__vRev1__));																				\
	(__vRev2__) = AKSIMD_MUL((__ReverbLevel__),(__vRev2__));																				\
	(__ReverbMix1__) = AKSIMD_ADD( AKSIMD_MUL((__RevUnitScale1__),(__vRev1__)), AKSIMD_MUL((__RevUnitScale2__),(__vRev2__)) );				\
	(__ReverbMix2__) = AKSIMD_ADD( AKSIMD_MUL((__RevUnitScale1__),(__vRev2__)), AKSIMD_MUL((__RevUnitScale2__),(__vRev1__)) );				\
	(__ReverbMix3__) = AKSIMD_ADD( AKSIMD_MUL((__POINTFIVE__),(__vRev2__)), AKSIMD_MUL((__POINTFIVE__),(__vRev1__)) );


// To be used by single unit processing
#define SCALEREVERBCOMPONENTSSINGLE( __Gain__, __DryLevel__, __IOPtr__, __WetLevel__ , __RefLevel__, __RefPtr__, __ReverbLevel__, __ReverbPtr__ ) \
	*(__IOPtr__) = ((__Gain__) * (((__DryLevel__) * *(__IOPtr__)) + ((__WetLevel__) * (((__RefLevel__) * *(__RefPtr__)) + ((__ReverbLevel__) * *(__ReverbPtr__))))));

// To be used by single unit processing (Send Mode)
#define SCALEREVERBCOMPONENTSSINGLESENT( __OPtr__, __Gain__, __RefLevel__, __RefPtr__, __ReverbLevel__, __ReverbPtr__ ) \
	*(__OPtr__) = ((__Gain__) * ( ((__RefLevel__) * *(__RefPtr__)) + ((__ReverbLevel__) * *(__ReverbPtr__)) ));

#define COMPUTEREVERBUNITMIX( __ReverbLevel__, __ReverbBufPtr1__, __ReverbBufPtr2__, __RevUnitScale1__, __RevUnitScale2__, __pReverbMix1__, __pReverbMix2__ ) \
	(__Unit1__) = ((__ReverbLevel__) * *(__ReverbBufPtr1__));									\
	(__Unit2__) = ((__ReverbLevel__) * *(__ReverbBufPtr2__));									\
	*(__pReverbMix1__) = ((__RevUnitScale1__) * (__Unit1__)) + ((__RevUnitScale2__) * (__Unit2__));		\
	*(__pReverbMix2__) = ((__RevUnitScale1__) * (__Unit2__)) + ((__RevUnitScale2__) * (__Unit1__));	

#define SCALEREVERBCOMPONENTSDUAL( __Gain__, __DryLevel__, __IOPtr__, __WetLevel__ , __RefLevel__, __RefPtr__, __ScaledRev__ ) \
	*(__IOPtr__) = ((__Gain__) * (((__DryLevel__) * *(__IOPtr__)) + ((__WetLevel__) * (((__RefLevel__) * *(__RefPtr__)) + (__ScaledRev__)))));

#define SCALEREVERBCOMPONENTSDUALSENT( __OPtr__, __Gain__, __RefLevel__, __RefPtr__, __ScaledRev__ ) \
	*(__OPtr__) = ((__Gain__) * (((__RefLevel__) * *(__RefPtr__)) + (__ScaledRev__)));

#define COMPUTEREVERBUNITMIXSURROUND( __ReverbLevel__, __ReverbBufPtr1__, __ReverbBufPtr2__, __RevUnitScale1__, __RevUnitScale2__, __pReverbMix1__, __pReverbMix2__, __pReverbMix3__ ) \
	(__Unit1__) = ((__ReverbLevel__) * *(__ReverbBufPtr1__));									\
	(__Unit2__) = ((__ReverbLevel__) * *(__ReverbBufPtr2__));									\
	*(__pReverbMix1__) = ((__RevUnitScale1__) * (__Unit1__)) + ((__RevUnitScale2__) * (__Unit2__));		\
	*(__pReverbMix2__) = ((__RevUnitScale1__) * (__Unit2__)) + ((__RevUnitScale2__) * (__Unit1__));		\
	*(__pReverbMix3__) = ((0.5f) * (__Unit2__)) + ((0.5f) * (__Unit1__));	

// sqrt(2)/(numchannels*num reverb units)
#define FIVEPOINTONENORMFACTOR		(0.11785113f)
#define FIVEPOINTZERONORMFACTOR		(0.14142135f)	
#define STEREONORMFACTOR			(0.35355339f)

static const AkReal32 ONEOVER_REVERB_WETDRYMIX_MAX = 1.f/REVERB_WETDRYMIX_MAX;

#endif
