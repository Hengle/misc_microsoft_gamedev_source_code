//============================================================================
//
// File: math.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

namespace Math
{
   enum CompareType
   {
      cNotEqualTo           = 0,
      cLessThan             = 1,
      cLessThanOrEqualTo    = 2,
      cEqualTo              = 3,
      cGreaterThanOrEqualTo = 4,
      cGreaterThan          = 5,
   };

   enum OperatorType
   {
      cOpTypeAdd        = 0,
      cOpTypeSubtract   = 1,
      cOpTypeMultiply   = 2,
      cOpTypeDivide     = 3,
      cOpTypeModulus    = 4,
   };

   const float fNearlyInfinite = 1.0e+37f;
   const float fMinuteEpsilon  = 0.00000125f;
   const float fTinyEpsilon    = 0.000125f;
   const float fSmallEpsilon   = 0.00125f;
               
   const float fPi             = 3.1415926536f;          // 180
   const float fHalfPi         = 3.1415926536f * .5f;    // 90
   const float fTwoPi          = 3.1415926536f * 2.0f;   // 360
   const float fFourPi         = 3.1415926536f * 4.0f;   // 720
   const float fOOPi           = 0.31830988618379067f;

   const double dPi            = 3.1415926535897932384626433832795028841971693993751; 
   const double dTwoPi         = 6.2831853071795864769252867665590057683943387987502; 
   const double dOOPi          = 0.3183098861837906715377675267450287240689192914809;
   
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

   // Uniform distribution, half open interval [l,h)
   // l <= ret < h
   // rg [12/31/05] -Hack hack, just uses rand() internally
   float fRand(float l = 0.0f, float h = 1.0f);
   
   //int iRand(int l, int h);
   
   // Returns 32-bit psuedo random number.
   //uint uRand(void);
   
   //void seedRand(uint seed);
   
   template<class T> T Clamp(T i, T l, T h);
   template<class T> T ClampLow(T i, T l);
   template<class T> T ClampHigh(T i, T h);
   template<class T> bool IsInRange(T i, T l, T h);
   template<class T> bool IsInExclusiveRange(T i, T l, T h);
   
   // Clamps i to [0.0f, 1.0f]
   template<class T> T Saturate(T i);
   
   // Clamps t to [0,255]
   int iClampToByte(int t);

   template<class T> int Sign(T f);
   
   float fSign(float f);
   
   float fDegToRad(float f);
   float fRadToDeg(float f);
   
   template<class T> void OrderLowestToHighest(T& a, T& b);
   template<class T> void OrderHighestToLowest(T& a, T& b);

   template<class T> bool EqualTol(T a, T b, T tol = fTinyEpsilon);
   template<class T> bool EqualRelTol(T a, T b, T tol = fTinyEpsilon);

   double Eps(double a, double b);
         
   // square
   template<class T> T Sqr(T f);
   
   int BitMaskOffset(uint i);
   int BitMaskLength(uint i);
   
   int iPosMod(int x, int y);
   float fPosMod(float x, float y);
   
   int NextPowerOf2(uint num);
   
   // num must be > 0 to be considered a power of 2!
   bool IsPow2(int num);
   bool IsOdd(int num);
   bool IsEven(int num);
   uint iLog2(uint num);
   uint CodeSize(uint value);
   
   template<class T> T PrevWrap(T i, T n);
   template<class T> T NextWrap(T i, T n);
   template<class T> T NegWrap(T i, T n);
   template<class T> T PosWrap(T i, T n);

   // false if the float is a NaN, infinity, or denormal
   bool IsValidFloat(float i);
   bool IsValidFloat(double i);
   
   // linear congruential generator
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
   
   bool FloatPositiveOrZero(float x);
   
   float fSmoothStep(float x);
   float fSmoothStep(float a, float b, float t);
    
   float fCalcFOVXFromY(float fovY, float aspectXY);
   float fCalcFOVYFromX(float fovX, float aspectYX);
   float fCalcScaleFromFOV(float fov);
   
   uint Factorial(uint i);
   
   // Returns the number of permutations of n different things taken n at a time (n!).
   uint NumPermutations(uint n);
   
   // Returns the number of combinations of n things taken k at a time.
   uint NumCombinations(uint n, uint k);
   
   float fAbs(float f);
   
   float fSqrt(float f);
   
   // fSelect is equivalent to return (comp >= 0.0f) ? a : b, except uses __fsel on Xbox.
   // From Feb. 2005 XFest:
   // minFP(x, y) = fSelect(x - y, y, x)
   // maxFP(x, y) = fSelect(x - y, x, y)
   // a > b ? y : z = fSelect(b - a, z, y)
   // a >= b ? y : z = fSelect(a - b, y, z)
   double fSelect(double comp, double a, double b);
   
   double fSelectMin(double x, double y);
   double fSelectMax(double x, double y);
   
   double fSelectClamp(double i, double l, double h);
   
   // Returns true if the specified string looks numeric. 
   // Overflow/underflow is not detected, so the string still may fail conversion.
   bool isNumeric(const char* pStr, bool allowExponential = true, uint startOfs = 0, int* pFailOfs = NULL, uint* pIntegralDigits = NULL, uint* pFractionalDigits = NULL, uint* pSignificantDigits = NULL);
   
   bool isZero(const char* pStr, bool allowExponential = true, bool* pIsNegative = NULL);
   
   // 32-bit linear feedback shift register.
   // jsr should not be 0!
   inline uint LFSR32(uint jsr);
   
   // Converts a float to an unsigned integer using all-integer math (i.e. the FP unit is not used to avoid load hit stores).
   uint64 FloatToUInt64TruncIntALU(const float* pFloat);
                  
} // namespace Math

#include "math.inl"
