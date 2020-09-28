//============================================================================
//
// File: math.inl
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#ifndef XBOX
   #include "xmmintrin.h"
#endif

namespace Math
{

	template<class T, class F> inline T Lerp(T a, T b, F fract)
	{
		return a + (b - a) * fract;
	}

	template<class T> inline T Min(T i, T j)
	{
		return (i < j) ? i : j;
	}

#ifdef XBOX	
   template<> inline float Min<float>(float i, float j) 
   {
      return (float)fSelectMin(i, j);
   }
   
   template<> inline double Min<double>(double i, double j) 
   {
      return fSelectMin(i, j);
   }
#endif   
	
	template<class T> inline T Min3(T a, T b, T c)
	{
		return Min(Min(a, b), c);
	}
	
	template<class T> inline T Min4(T a, T b, T c, T d)
	{
		return Min(Min(Min(a, b), c), d);
	}

	template<class T> inline int Min2Index(T a, T b)
	{
		if (a < b)
			return 0;

		return 1;
	}

	template<class T> inline int Min3Index(T a, T b, T c)
	{
		T min = a;
		int resultIndex = 0;
	   
		if (b < min)
		{
			min = b;
			resultIndex = 1;
		}

		if (c < min)
			resultIndex = 2;
	   
		return resultIndex;
	}

	template<class T> inline T Max(T i, T j)
	{
		return (i > j) ? i : j;
	}
	
#ifdef XBOX	
   template<> inline float Max<float>(float i, float j) 
   {
      return (float)fSelectMax(i, j);
   }
   
   template<> inline double Max<double>(double i, double j) 
   {
      return fSelectMax(i, j);
   }
#endif   

	template<class T> inline T Max3(T a, T b, T c)
	{
		return Max(Max(a, b), c);
	}

	template<class T> inline T Max4(T a, T b, T c, T d)
	{
		return Max(Max(Max(a, b), c), d);
	}
	   
	template<class T> inline int Max2Index(T a, T b)
	{
		if (a > b)
			return 0;

		return 1;
	}

	template<class T> inline int Max3Index(T a, T b, T c)
	{
		T max = a;
		int resultIndex = 0;
	   
		if (b > max)
		{
			max = b;
			resultIndex = 1;
		}

		if (c > max)
			resultIndex = 2;
	   
		return resultIndex;
	}
	
   template<class T> inline void OrderLowestToHighest(T& a, T& b)
   {
      if (a > b)
         std::swap(a, b);
   }
   
   template<class T> inline void OrderHighestToLowest(T& a, T& b)
   {
      if (a < b)
         std::swap(a, b);
   }
	         
	template<class T> inline T Clamp(T i, T l, T h)
	{
		if (i < l)
			return l;
		else if (i > h)
			return h;

		return i;
	}

#ifdef XBOX	
   template<> inline float Clamp(float i, float l, float h)
   {
      return (float)fSelectClamp(i, l, h);
   }
   
   template<> inline double Clamp(double i, double l, double h)
   {
      return fSelectClamp(i, l, h);
   }
#endif

	template<class T> inline T ClampLow(T i, T l)
	{
		return (i < l) ? l : i;
	}

	template<class T> inline T ClampHigh(T i, T h)
	{
		return (i > h) ? h : i;
	}
	
	template<class T> bool IsInRange(T i, T l, T h)
	{
	   return (i >= l) && (i <= h);
	}
	
   template<class T> bool IsInExclusiveRange(T i, T l, T h)
   {
      return (i >= l) && (i < h);
   }
	
	template<class T> inline T Saturate(T i) 
	{ 
	   return Clamp(i, 0.0f, 1.0f); 
	}

	inline int iClampToByte(int t) 
	{
		if (t & 0xFFFFFF00)
			t = ~(t >> 31) & 0xFF;
		return t;
	}

	template<class T> inline int Sign(T f)
	{ 
		return (f > 0) ? 1 : ((f < 0) ? -1 : 0); 
	}
	
	inline float fSign(float f)
	{
		if (f < 0.0f)
		   return -1.0f;
		else if (f > 0.0f)
		   return 1.0f;
		
		return 0.0f;
	}

	inline float fDegToRad(float f)
	{
		return f * 0.01745329252f;
	};

	inline float fRadToDeg(float f)
	{
		return f * 57.29577951f;
	};

   inline double Eps(double a, double b, double tol = fSmallEpsilon)
	{
		b;
		const double aa = fabs(a) + 1;
		return tol * aa;
	}
	
	template<class T> inline bool EqualTol(T a, T b, T tol)
	{
		return fabs(a - b) <= tol;
	}
			
	template<class T> inline bool EqualRelTol(T a, T b, T tol)
	{
		return fabs(a - b) <= Eps(a, b, tol);
	}
	
	template<class T> inline T Sqr(T f)
	{
		return f * f;
	}
	
	inline int BitMaskOffset(uint i)
	{
		if (!i)
			return (32);
		int offset = 0;
		while ((i & 1) == 0)
		{
			offset++;
			i >>= 1;
		}
		return offset;
	}

	inline int BitMaskLength(uint i)
	{
		int length = 0;
		i >>= BitMaskOffset(i);
		while (i)
		{
			length++;
			i >>= 1;
		}
		return length;
	}
	
	inline int iPosMod(int x, int y)
	{
		BDEBUG_ASSERT(y > 0);
		if (x >= 0)
			return x % y;
		else
		{
			int m = (-x) % y;

			if (m != 0)
				m = y - m;

			return m;
		}
	}

	inline float fPosMod(float x, float y)
	{
		BDEBUG_ASSERT(y > 0.0f);

		if (x >= 0.0f)
			return fmod(x, y);
		else
		{
			float m = fmod(-x, y);
			if (m != 0.0f)
				m = y - m;
			return m;
		}
	}
	
	inline int NextPowerOf2(uint num)
	{
		num -= 1;
		num |= num >> 16;
		num |= num >> 8;
		num |= num >> 4;
		num |= num >> 2;
		num |= num >> 1;
		return num + 1;
	}

	inline bool IsPow2(int num)
	{
		return (num > 0) && ((num & -num) == num);
	}

	inline bool IsOdd(int num)
	{
		return (num & 1) == 1;
	}

	inline bool IsEven(int num)
	{
		return (num & 1) == 0;
	}
	
	// floor(log2(num))
	inline uint iLog2(uint num)
	{
		uint result = 0;
		while (num > 1U) 
		{
			num >>= 1;
			result++;
		} 
		return result;
	}
	
   // Returns number of bits needed to encode value.
   inline uint CodeSize(uint value)
   {
      uint result = 0;
      while (value > 0U)
      {
         value >>= 1;
         result++;
      } 
      return result;
   }
	
	template<class T> inline T PrevWrap(T i, T n) 
	{ 
		T temp = i - 1;
		if (temp < 0)
			temp = n - 1;
		return temp;
	}

	template<class T> inline T NextWrap(T i, T n) 
	{ 
		T temp = i + 1;
		if (temp >= n)
			temp = 0;
		return temp;
	}

	template<class T> inline T NegWrap(T i, T n) 
	{ 
		T temp = i;
		if (temp < 0)
			temp += n;
		return temp;
	}   

	template<class T> inline T PosWrap(T i, T n) 
	{ 
		T temp = i;
		if (temp >= n)
			temp -= n;
		return temp;
	}

	inline bool IsValidFloat(float i)
	{
		const uint bits = *reinterpret_cast<uint*>(&i);
		const uint exponent = (bits >> 23) & 0xFF;
		if (exponent == 255) 
		{
			// If mantissa is 0 the value is infinity, otherwise it's a NaN.
			return false;
		}	  
		const uint mantissa = bits & ((1 << 23) - 1);
		if ((exponent == 0) && (mantissa != 0))
		{
			// The value is a denormal.
			return false; 
		}
		return true;
	}
	
   inline bool IsValidFloat(double i)
   {
      // FIXME - Should check the double's bits.
      return IsValidFloat(static_cast<float>(i));
   }
   
   inline float fRand(float l, float h)
   {
      return Lerp(l, h, rand() / float(RAND_MAX + 1));
   }
      
	inline uint LCGNextRand(uint& curSeed)
	{
		return curSeed = 12345 + curSeed * 1103515245UL;
	}

	inline float fRSqrt(float t)
	{
		return 1.0f / sqrt(t);
	}

	// FIXME: Optimize!
	inline void fSinCos(float ang, float& s, float& c)
	{
		s = sin(ang);
		c = cos(ang);
	}

	inline float fFastSin(float angle)
	{
		BDEBUG_ASSERT(angle >= 0.0f);
		BDEBUG_ASSERT(angle <= fHalfPi);

		const float asqr = angle * angle;
	 	return ((((7.61e-03f * asqr) - 1.6605e-01f) * asqr) + 1.0f) * angle;
	}

	inline float fFastCos(float angle)
	{
		BDEBUG_ASSERT(angle >= 0.0f);
		BDEBUG_ASSERT(angle <= fHalfPi);
		const float asqr = angle * angle;
		return (((3.705e-02f * asqr) - 4.967e-01f) * asqr) + 1.0f;
	}
		   
	inline float fFastACos(float value)
	{
		BDEBUG_ASSERT(value >= 0.0f);
		BDEBUG_ASSERT(value <= 1.0f);
		return ((((((-0.0187293f * value) + 0.0742610f) * value) - 0.2121144f) * value) + 1.5707288f) * sqrt(1.0f - value);
	}
	
	// With truncation. Result is pushed towards zero.
   inline int DoubleToIntTrunc(double f)
   {
      return static_cast<int>(f);
   }

   // With rounding, nearest or even. Result is pushed away from zero.
   inline int DoubleToIntRound(double f)
   {
      return static_cast<int>((f < 0.0f) ? -floor(-f + .5f) : floor(f + .5f));
   }


#ifdef XBOX
   // With truncation. Result is pushed towards zero.
   inline int FloatToIntTrunc(float f)
   {
      return static_cast<int>(f);
   }

   // With rounding, nearest or even. Result is pushed away from zero.
   inline int FloatToIntRound(float f)
   {
      return static_cast<int>((f < 0.0f) ? -floor(-f + .5f) : floor(f + .5f));
   }

   inline bool FloatPositiveOrZero(float x)
   {
      return (x >= 0.0f);
   }
#else
   // With truncation. Result is pushed towards zero.
	inline int FloatToIntTrunc(float f)
	{
		return static_cast<int>(f);
	}
	
	// With rounding, nearest or even. Result is pushed away from zero.
	inline int FloatToIntRound(float f)
	{
      int r;	
		__asm
		{
			fld f
			fistp r
		}
		return r;
	}
	
   inline bool FloatPositiveOrZero(float x)
   {
      return *(int *)&(x) >= 0;
   }
#endif	
	   	
	inline float fSmoothStep(float x)
	{
		return x * x * (3.0f - 2.0f * x);
	}

   inline float fSmoothStep(float a, float b, float t)
   {
      t = Math::Clamp<float>((t - a) / (b - a), 0.0, 1.0);
      return t * t * (3.0f - 2.0f * t);
   }
	
	inline float fCalcFOVXFromY(float fovY, float aspectXY)
	{
		return 2.0f * atan(tan(fovY * .5f) * aspectXY); 
	}
	
	inline float fCalcFOVYFromX(float fovX, float aspectYX)
	{
		return 2.0f * atan(tan(fovX * .5f) * aspectYX);
	}
	
	inline float fCalcScaleFromFOV(float fov)
	{
		return tan(Math::fPi * .5f - (fov * .5f)); 
	}
	
	inline uint Factorial(uint i)
	{
		BDEBUG_ASSERT(i <= 12);
		
		for (int j = i - 1; j >= 2; j--)
		   i *= j;
		   
		return i;
	}
	
   inline uint NumPermutations(uint n)
   {
      return Factorial(n);
   }
	
	inline uint NumCombinations(uint n, uint k)
	{
		BDEBUG_ASSERT(n >= k);
		BDEBUG_ASSERT(n > 0);
		BDEBUG_ASSERT(k > 0);
		if (n == k)
		   return 1;
		
		return Factorial(n) / (Factorial(n - k) * Factorial(k));
	}
	
	inline float fAbs(float f)
	{
		return fabs(f);
	}
	
	inline float fSqrt(float f)
	{
		return sqrt(f);
	}
	
	inline double fSelect(double comp, double a, double b)
	{
#ifdef XBOX	
      return __fsel(comp, a, b);
#else
      return (comp >= 0.0f) ? a : b;
#endif
	}
	
   inline double fSelectMin(double x, double y) 
   { 
      return fSelect(x - y, y, x); 
   }
   
   inline double fSelectMax(double x, double y) 
   { 
      return fSelect(x - y, x, y); 
   }
   
   inline double fSelectClamp(double i, double l, double h)
   {
      return fSelectMax(fSelectMin(i, h), l);
   }
   
   inline uint LFSR32(uint jsr)
   {
#define RND_SHR3  (jsr^=(jsr<<17), jsr^=(jsr>>13), jsr^=(jsr<<5))
      RND_SHR3;
#undef RND_SHR3
      return jsr;
   }
		    
   inline float fReciprocal(float a)
   {
#ifdef XBOX	
      return __fres(a);
#else
      return 1.0f / a;
#endif
   }

} // namespace Math

 
