//-----------------------------------------------------------------------------
// File: math.h
// Math constants and utilities.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef MATH_H
#define MATH_H

#include "common/core/core.h"

namespace gr
{
	namespace Math
	{
		const float fNearlyInfinite = 1.0e+37f;
		const float fTinyEpsilon    = 0.000125f;
		const float fSmallEpsilon   = 0.00125f;
		const float fLargeEpsilon   = 0.0125f;
		const double dPi            = 3.1415926535897932384626433832795028841971693993751; 
		const double dTwoPi         = 3.1415926535897932384626433832795028841971693993751*2.0; 
		const float fPi             = 3.1415926535f;				// 180
		const float fHalfPi         = 3.1415926535f*.5f;		// 90
		const float fTwoPi          = 3.1415926535f*2.0f;		// 360
		const float fFourPi         = 3.1415926535f*4.0f;		// 720

		template<class T, class F> T Lerp(T a, T b, F fract);
	  
		template<class T> T Min(T i, T j);
		template<class T> T Min3(T a, T b, T c);
		template<class T> T Min4(T a, T b, T c, T d);
		template<class T> int Min2Index(T a, T b);
		template<class T> int Min3Index(T a, T b, T c);  
	  
		template<class T> T Max(T i, T j);
		template<class T> T Max3(T a, T b, T c);
		template<class T> T Max4(T a, T b, T c, T d);
		template<class T> int Max2Index(T a, T b);
		template<class T> int Max3Index(T a, T b, T c);

		// l <= ret <= h
		float fRand(float l = 0.0f, float h = 1.0f);

		template<class T> T Clamp(T i, T l, T h);
		template<class T> T ClampLow(T i, T l);
		template<class T> T ClampHigh(T i, T h);
		int iClampToByte(int t);

		template<class T> int Sign(T f);
		
		float fDegToRad(float f);
		float fRadToDeg(float f);

		template<class T> bool EqualTol(T a, T b, T tol = fSmallEpsilon);

		double Eps(double a, double b);
		bool FuzzyEq(double a, double b);
		bool FuzzyNe(double a, double b);
		bool FuzzyGt(double a, double b);
		bool FuzzyGe(double a, double b);
		bool FuzzyLt(double a, double b);
		bool FuzzyLe(double a, double b);
	  
		template<class T> T Sqr(T f);
	  
		int BitMaskOffset(uint i);
		int BitMaskLength(uint i);
	  
		int iPosMod(int x, int y);
		float fPosMod(float x, float y);
	  
		int NextPowerOf2(uint num);
		bool IsPow2(int num);
		bool IsOdd(int num);
		bool IsEven(int num);
		int iLog2(int num);
	  
		template<class T> T PrevWrap(T i, T n);
		template<class T> T NextWrap(T i, T n);
		template<class T> T NegWrap(T i, T n);
		template<class T> T PosWrap(T i, T n);

		bool IsValidFloat(float i);
	  
		uint LCGNextRand(uint& curSeed);

		// 1/sqrt(t)
		float fRSqrt(float t);
		
		// s=sin(ang), c=cos(ang)
		void fSinCos(float ang, float& s, float& c);
			
		// From Magic Software's core library:
		// Angle must be within [0,PI/2]. Max. abs error is ~1.7e-04. 
		float fFastSin(float angle);

		// From Magic Software's core library:
		// Angle must be within [0,PI/2]. Max. abs error is 1.1880e-03. 
		float fFastCos(float angle);

		// The value must be in [0,1]. Max. abs error is ~6.8e-05.
		float fFastACos(float value);

		// FIXME: Should be in platform specific module!
		// with truncation
		int FloatToIntTrunc(float f);
		
		// FIXME: Should be in platform specific module!
		// with rounding, nearest or even
		int FloatToIntRound(float f);
		
		float fSmoothStep(float x);
		
		float fCalcFOVXFromY(float fovY, float aspectXY);
		float fCalcFOVYFromX(float fovX, float aspectYX);
		float fCalcScaleFromFOV(float fov);
							
	} // namespace Math

} // namespace gr  

#include "math.inl"
  
#endif // MATH_H

