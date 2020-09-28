//-----------------------------------------------------------------------------
// File: math.inl
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "xmmintrin.h"

namespace gr
{
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

		inline float fRand(float l, float h)
		{
			return l + (h - l) * rand() * (1.0f / float(RAND_MAX));
		}

		template<class T> inline T Clamp(T i, T l, T h)
		{
			if (i < l)
				return l;
			else if (i > h)
				return h;

			return i;
		}

		template<class T> inline T ClampLow(T i, T l)
		{
			return (i < l) ? l : i;
		}

		template<class T> inline T ClampHigh(T i, T h)
		{
			return (i > h) ? h : i;
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

		inline float fDegToRad(float f)
		{
			return f * 0.01745329252f;
		};

		inline float fRadToDeg(float f)
		{
			return f * 57.29577951f;
		};

		template<class T> inline bool EqualTol(T a, T b, T tol)
		{
			return fabs(a - b) <= tol;
		}

		inline double Eps(double a, double b)
		{
			b;
			const double aa = fabs(a) + 1;
			return fSmallEpsilon * aa;
		}

		inline bool FuzzyEq(double a, double b)
		{
			return fabs(a - b) <= Eps(a, b);
		}

		inline bool FuzzyNe(double a, double b)
		{
			return ! FuzzyEq(a, b);
		}

		inline bool FuzzyGt(double a, double b)
		{
			return a > b + Eps(a, b);
		}

		inline bool FuzzyGe(double a, double b)
		{
			return a > b - Eps(a, b);
		}

		inline bool FuzzyLt(double a, double b)
		{
			return a < b - Eps(a, b);
		}

		inline bool FuzzyLe(double a, double b)
		{
			return a < b + Eps(a, b);
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
			Assert(y > 0);
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
			Assert(y > 0.0f);

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
	  
		inline int iLog2(int num)
		{
			int result = 0;
			while (num > 1) 
			{
				num >>= 1;
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
			Assert(angle >= 0.0f);
			Assert(angle <= fHalfPi);

			const float asqr = angle * angle;
	 		return ((((7.61e-03f * asqr) - 1.6605e-01f) * asqr) + 1.0f) * angle;
		}

		inline float fFastCos(float angle)
		{
			Assert(angle >= 0.0f);
			Assert(angle <= fHalfPi);
			const float asqr = angle * angle;
			return (((3.705e-02f * asqr) - 4.967e-01f) * asqr) + 1.0f;
		}
		    
		inline float fFastACos(float value)
		{
			Assert(value >= 0.0f);
			Assert(value <= 1.0f);
			return ((((((-0.0187293f * value) + 0.0742610f) * value) - 0.2121144f) * value) + 1.5707288f) * sqrt(1.0f - value);
		}

		// with truncation
		inline int FloatToIntTrunc(float f)
		{
			return _mm_cvttss_si32(_mm_load_ss(&f));
		}
		
		// with rounding, nearest or even
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
		
		inline float fSmoothStep(float x)
		{
			return x * x * (3.0f - 2.0f * x);
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
	
} // namespace Math

 } // namespace gr
 
