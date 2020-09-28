//==============================================================================
// scalarconsts.h
//
// Copyright (c) 1999-2006, Ensemble Studios
//==============================================================================
#pragma once

#ifdef XBOX

const float cPi=XM_PI;
const float cTwoPi=XM_2PI;
const float cPiOver2=XM_PIDIV2;
const float cPiOver4=XM_PIDIV4;

#else

const float cPi=3.14159265359f;
const float cTwoPi=2.0f*cPi;
const float cPiOver2=0.5f*cPi;
const float cPiOver4=0.25f*cPi;

#endif

const float cPiOver8=0.125f*cPi;
const float cPiOver12=0.0833f*cPi;
const float cThreePiOver2=1.5f*cPi;
const float cThreePiOver4=0.75f*cPi;
const float cRadiansPerDegree=cPi/180.0f;//0.0174533f;
const float cDegreesPerRadian=180.0f/cPi;//57.295779f;
const float cSqrt2 = (float)sqrt(2.0f);
const float cOneOverSqrt2 = 1.0f/cSqrt2;
const float cOneThird = 1.0f/3.0f;
const float cOneHalf = 0.5f;
const float cFloatCompareEpsilon=0.000001f;
const float cMaximumFloat=FLT_MAX;
const float cMinimumFloat=FLT_MIN;
const long cMaximumLong=LONG_MAX;
const long cMinimumLong=LONG_MIN;
const long cMaximumWORD=32768;
const DWORD cMaximumDWORD=ULONG_MAX;
const __int64 cMaximumInt64=0xffffffffffffffff;

const float cGravityAcceleration = 9.8f;

const float c255        = 255.0f;
const float cOneOver255 = 1.0f/255.0f;
const DWORD cAlphaMask  = 0xFF000000;
const DWORD cRedMask    = 0x00FF0000;
const DWORD cGreenMask  = 0x0000FF00;
const DWORD cBlueMask   = 0x000000FF;
const DWORD cAlphaShift = 24;
const DWORD cRedShift   = 16;
const DWORD cGreenShift = 8;

const WCHAR UNICODE_TOKEN = 0xFEFF;

//-----------------------------------------------------------------------------
//  MACROS
//-----------------------------------------------------------------------------
#define DEGREES_TO_RADIANS(val) (val * cRadiansPerDegree)
#define RADIANS_TO_DEGREES(val) (val * cDegreesPerRadian)

