//-----------------------------------------------------------------------------
// File: general_vector.h
// Note: Don't include this file directly, instead include vector.h!
// A generic, but slow vector class.
// Some customizations present for Vec1-Vec4's, but any size vector is supported.
//-----------------------------------------------------------------------------
#pragma once
#ifndef GENERAL_VECTOR_H
#define GENERAL_VECTOR_H

#include "common/utils/stream.h"
#include "common/utils/logfile.h"

#ifndef GENMATRIX_DEF_PLATFORM_METHODS
	#define GENMATRIX_DEF_PLATFORM_METHODS
#endif

namespace gr
{
	template<int size>
	struct Vec
	{
		enum { numElements = size };
		enum { requiresAlignment = false };

		float element[numElements];
		
		Vec() { }
		
		explicit Vec(float s) 
		{
			set(s);
		}
		
		Vec(float sx, float sy)
		{
			set(sx, sy);
		}
		
		Vec(float sx, float sy, float sz)
		{
			set(sx, sy, sz);
		}
		
		Vec(float sx, float sy, float sz, float sw)
		{
			set(sx, sy, sz, sw);
		}

		Vec(const Vec& b)
		{
			*this = b;
		}

		template<int otherSize>
		Vec(const Vec<otherSize>& b)
		{
			*this = b;
		}
		
		template<int otherSize>
		Vec(const Vec<otherSize>& b, float sw)
		{
			*this = b;
			if (size >= 4)
				element[3] = sw;
		}

		Vec& operator= (const Vec& b)
		{
			for (int i = 0; i < size; i++)
				element[i] = b.element[i];
			return *this;
		}

		template<int otherSize>
		Vec& operator= (const Vec<otherSize>& b)
		{
			int n = Math::Min<int>(size, b.numElements);
			for (int i = 0; i < n; i++)
				element[i] = b.element[i];
			for ( ; i < size; i++)
				element[i] = 0;
			return *this;
		}

		bool operator== (const Vec& b) const
		{
			for (int i = 0; i < size; i++)
				if (element[i] != b.element[i])
					return false;
			return true;
		}

		bool operator!= (const Vec& b) const
		{
			return !(*this == b);
		}

		// lexicographical less
		bool operator<(const Vec& b) const
		{
			for (int i = 0; i < size; i++)
			{
				if (element[i] != b.element[i])
				{
					if (element[i] < b.element[i])
						return true;
					
					return false;
				}
			}
			return false;
		}

		Vec& setW(float sw)
		{
			staticAssert(size >= 4);
			element[3] = sw;
			return *this;
		}

		Vec& clearW(void)
		{
			staticAssert(size >= 4);
			element[3] = 0.0f;
			return *this;
		}

		Vec& set(float s)
		{
			for (int i = 0; i < size; i++)
				element[i] = s;
			return *this;
		}
		
		Vec& set(float sx, float sy)
		{
			element[0] = sx;
			if (size >= 2)
			{
				element[1] = sy;
				for (int i = 2; i < size; i++)
					element[i] = 0;
			}
			return *this;
		}
		
		Vec& set(float sx, float sy, float sz)
		{
			element[0] = sx;
			if (size >= 2)
			{
				element[1] = sy;
				if (size >= 3)
				{
					element[2] = sz;
					for (int i = 3; i < size; i++)
						element[i] = 0;
				}
			}
			return *this;
		}
		
		Vec& set(float sx, float sy, float sz, float sw)
		{
			element[0] = sx;
			if (size >= 2)
			{
				element[1] = sy;
				if (size >= 3)
				{
					element[2] = sz;
					if (size >= 4)
					{
						element[3] = sw;
						for (int i = 4; i < size; i++)
							element[i] = 0;
					}
				}
			}
			return *this;
		}
		
		Vec& setZero(void)
		{
			for (int i = 0; i < size; i++)
				element[i] = 0;
			return *this;
		}
	
		// element access
		const float& operator[] (int i) const
		{
			return element[DebugRange(i, size)];
		}
		
		float& operator[] (int i)
		{
			return element[DebugRange(i, size)];
		}
		
		// in place vec add
		Vec& operator+= (const Vec& b)
		{
			for (int i = 0; i < size; i++)
				element[i] += b.element[i];
			return *this;
		}

		// in place vec sub
		Vec& operator-= (const Vec& b)
		{
			for (int i = 0; i < size; i++)
				element[i] -= b.element[i];
			return *this;
		}

		// in place vec mul
		Vec& operator*= (const Vec& b)
		{
			for (int i = 0; i < size; i++)
				element[i] *= b.element[i];
			return *this;
		}

		// in place scaler mul
		Vec& operator*= (float s)
		{
			for (int i = 0; i < size; i++)
				element[i] *= s;
			return *this;
		}

		// in place scaler div
		Vec& operator/= (float s)
		{
			for (int i = 0; i < size; i++)
				element[i] /= s;
			return *this;
		}
		
		// positive
		Vec operator+ () const
		{
			return *this;
		}

		// negative
		Vec operator- () const
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = -element[i];
			return ret;
		}

		// scale mul
		Vec operator* (float s) const
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = element[i] * s;
			return ret;
		}

		// scale mul
		friend Vec operator* (float s, const Vec& a)
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = s * a.element[i];
			return ret;
		}
		
		// scale div
		Vec operator/ (float s) const
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = element[i] / s;
			return ret;
		}
		
		// vec add/sub
		friend Vec operator+ (const Vec& a, const Vec& b)
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = a.element[i] + b.element[i];
			return ret;
		}

		friend Vec operator- (const Vec& a, const Vec& b)
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = a.element[i] - b.element[i];
			return ret;
		}
		
		// dot product
		friend float operator* (const Vec& a, const Vec& b)
		{
			float sum = 0;
			for (int i = 0; i < size; i++)
				sum += a.element[i] * b.element[i];
			return sum;
		}
						
		// vec division
		friend Vec operator/ (const Vec& a, const Vec& b)
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = a.element[i] / b.element[i];
			return ret;
		}

		friend Vec operator% (const Vec& a, const Vec& b)
		{
			switch (size)
			{
				case 2:
					return Vec<2>(a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2],	a[0]*b[1] - a[1]*b[0]);
				case 3:
					return Vec<3>(a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2],	a[0]*b[1] - a[1]*b[0]);
				case 4:
					return Vec<4>(a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2],	a[0]*b[1] - a[1]*b[0]);
			}
			StaticAssert(false);
			return Vec(0);
		}
		
		// add elements
		float horizontalAdd(void) const
		{
			float sum = 0;
			for (int i = 0; i < size; i++)
				sum += element[i];
			return sum;
		}

		// sum in first element
		Vec horizontalAddSingle(void) const
		{
			Vec ret(0);
			ret.element[0] = horizontalAdd();
			return ret;
		}

		// sum in all elements
		Vec horizontalAddBroadcast(void) const
		{
			return Vec(horizontalAdd());
		}
		
		// dot product 
		float dot(const Vec& b) const
		{
			return *this * b;
		}
	
		// squared length
		float norm(void) const
		{
			float sum = 0;
			for (int i = 0; i < size; i++)
				sum += element[i] * element[i];
			return sum;
		}

		float len2(void) const
		{
			return norm();
		}

		float squaredLen(void) const 
		{ 
			return norm(); 
		}
		
		// length
		float len(void) const
		{
			return sqrt(norm());
		}
		
		// 1/length
		float oneOverLen(void) const
		{
			const float l = len();
			if (l != 0.0f)
				return 1.0f / l;
			return 0.0f;
		}
		
		// in place normalize
		Vec& normalize(void)
		{
			*this *= oneOverLen();
			return *this;
		}

		// in place normalize
		// returns 0 or 1/sqrt(norm())
		float tryNormalize(void)
		{
			const float ret = oneOverLen();
			*this *= oneOverLen();
			return ret;
		}
		
		// returns normalized vector
		Vec normalized(void) const
		{
			return Vec(*this * oneOverLen());
		}
				
		// component replication
		Vec broadcast(const int e) const
		{
			DebugRange(e, size);

			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = element[e];
			
			return ret;
		}
		
		bool isVector(void) const
		{
			return element[size - 1] == 0.0f;
		}

		bool isPoint(void) const
		{
			return element[size - 1] == 1.0f;
		}
		
		// true if unit length or nearly so
		bool isUnit(void) const
		{
			return Math::EqualTol(len(), 1.0f);
		}
				
		// clamp components between [low, high]
		Vec& clampComponents(float low, float high)
		{
			for (int i = 0; i < size; i++)
				if (element[i] < low)
					element[i] = low;
				else if (element[i] > high)
					element[i] = high;
			return *this;
		}
		
		float minComponent(void) const
		{
			float m = element[0];
			for (int i = 1; i < size; i++)
				if (element[i] < m)
					m = element[i];
			return m;
		}

		float maxComponent(void) const
		{
			float m = element[0];
			for (int i = 1; i < size; i++)
				if (element[i] > m)
					m = element[i];
			return m;
		}

		float minAbsComponent(void) const
		{
			float m = fabs(element[0]);
			for (int i = 1; i < size; i++)
				if (fabs(element[i]) < m)
					m = fabs(element[i]);
			return m;
		}
				
		float maxAbsComponent(void) const
		{
			float m = fabs(element[0]);
			for (int i = 1; i < size; i++)
				if (fabs(element[i]) > m)
					m = fabs(element[i]);
			return m;
		}

		int minorAxis(void) const
		{
			float m = fabs(element[0]);
			int a = 0;
			for (int i = 1; i < size; i++)
				if (fabs(element[i]) < m)
				{
					m = fabs(element[i]);
					a = i;
				}
			return a;
		}
		
		int majorAxis(void) const
		{
			float m = fabs(element[0]);
			int a = 0;
			for (int i = 1; i < size; i++)
				if (fabs(element[i]) > m)
				{
					m = fabs(element[i]);
					a = i;
				}
			return a;
		}

		void projectionAxes(int& uAxis, int& vAxis) const
		{
			staticAssert(size == 3);
			const int axis = majorAxis();
			if (element[axis] < 0.0f)
			{
				vAxis = NextWrap(axis, size);
				uAxis = NextWrap(vAxis, size);
			}
			else
			{
				uAxis = NextWrap(axis, size);
				vAxis = NextWrap(uAxis, size);
			}
		}

		// [-1,1] -> [0,1]
		Vec rangeCompressed(void) const
		{
			return *this * .5f + Vec(.5f);
		}
		
		// [0,1] -> [-1,1]
		Vec rangeExpanded(void) const
		{
			return (*this - Vec(.5f)) * 2.0f;
		}

		Vec toVector(void) const
		{
			element[size - 1] = 0.0f;
			return *this;
		}

		Vec toPoint(void) const
		{
			element[size - 1] = 1.0f;
			return *this;
		}

		Vec floor(void) const
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = ::floor(element[i]);
			return ret;
		}

		Vec ceil(void) const
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = ::ceil(element[i]);
			return ret;
		}

		// true if all elements are 0
		bool isZero(void) const
		{
			for (int i = 0; i < size; i++)
				if (element[i] != 0.0f)
					return false;
			return true;
		}
	  	
		// remove projection of v on dir from v, dir must be unit length
		static Vec removeCompUnit(const Vec& v, const Vec& dir)
		{
			Assert(dir.isUnit());
			const Vec rhs(dir * (v * dir));
			return v - rhs;
		}
		
		// returns vector with a 1 in the selected component
		static Vec makeAxisVector(int axis, float s = 1.0f)
		{
			Vec ret;
			ret.setZero();
			ret.element[DebugRange(axis, size)] = s;
			return ret;
		}

		// vec multiply
		static Vec multiply(const Vec& a, const Vec& b)
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = a.element[i] * b.element[i];
			return ret;
		}

		// elementwise min
		static Vec elementMin(const Vec& a, const Vec& b)
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = Math::Min(a.element[i], b.element[i]);
			return ret;
		}

		// elementwise max
		static Vec elementMax(const Vec& a, const Vec& b)
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = Math::Max(a.element[i], b.element[i]);
			return ret;
		}

		// See "The Pleasures of Perp Dot Products", Graphics Gems IV, page 138.
		Vec perp(void) const
		{
			StaticAssert(size == 2);
			return Vec(-element[1], element[0]);
		}

		// this->perp() dot b
		float perpDot(const Vec& b) const
		{
			StaticAssert(size == 2);
			return element[0] * b.element[1] - element[1] * b.element[0];
		}

		static bool equalTol(const Vec& a, const Vec& b, float tol = fSmallTol)
		{
			for (int i = 0; i < size; i++)
				if (!Math::EqualTol(a, b, tol))
					return false;
			return true;
		}

		static Vec lerp(const Vec& a, const Vec& b, float t)
		{
			Vec ret;
			for (int i = 0; i < size; i++)
				ret.element[i] = a.element[i] + (b.element[i] - a.element[i]) * t;
			return ret;
		}

		float* setFloats(float* pDest) const
		{
			for (int i = 0; i < size; i++)
				*pDest++ = element[i];
			return pDest;
		}

		static Vec makeFromFloats(const float* pFloats, int n)
		{
			n = Math::Min(size, n);
			for (int i = 0; i < n; i++)
				element[i] = pFloats[i];
			for ( ; i < size; i++)
				element[i] = 0;
		}

		// false if any elements are invalid floats
		bool isValid(void) const
		{
			for (int i = 0; i < size; i++)
				if (!Math::IsValidFloat(element[i]))
					return false;
			return true;
		}

		bool allLess(const Vec& b) const
		{
			for (int i = 0; i < size; i++)
				if (element[i] >= b.element[i])
					return false;
			return true;
		}

		bool allLessEqual(const Vec& b) const
		{
			for (int i = 0; i < size; i++)
				if (element[i] > b.element[i])
					return false;
			return true;
		}

		bool allGreater(const Vec& b) const
		{
			for (int i = 0; i < size; i++)
				if (element[i] <= b.element[i])
					return false;
			return true;
		}

		bool allGreaterEqual(const Vec& b) const
		{
			for (int i = 0; i < size; i++)
				if (element[i] < b.element[i])
					return false;
			return true;
		}

// was causing weird problems in VS.Net 2003
//		operator const	float* () const	{ return &element[0]; }
//		operator				float* ()				{ return &element[0]; }
		const float* getPtr(void) const { return &element[0]; }
					float* getPtr(void)				{ return &element[0]; }

#if DEBUG
		void debugCheck(void) const
		{
			for (int i = 0; i < size; i++)
				Assert(Math::IsValidFloat(element[i]));
		}
#else
		void debugCheck(void) const
		{
		}
#endif

		void log(LogFile& l) const
		{
			for (int i = 0; i < size; i++)
			{
				l.printf("%f", element[i]);
				if (i < size - 1)
					l.printf(" ");
			}
			l.printf("\n");
		}

		friend Stream& operator<< (Stream& dst, const Vec& src)
		{
			for (int i = 0; i < size; i++)
				dst << src.element[i];
			return dst;
		}

		friend Stream& operator>> (Stream& src, Vec& dst)
		{
			for (int i = 0; i < size; i++)
				src >> dst.element[i];
			return src;
		}
		    
		// This is a hack so the platform specific conversion constructors, 
		// copy operators, etc. can be defined.
		GENMATRIX_DEF_PLATFORM_METHODS
  };
	
	template<class T>
	struct VecInterval
	{
		T extent[2];

		VecInterval()
		{
		}

		VecInterval(const T& v)
		{
			extent[0] = v;
			extent[1] = v;
		}

		VecInterval(const T& low, const T& high)
		{
			extent[0] = low;
			extent[1] = high;
		}

		VecInterval(float low, float high)
		{
			extent[0] = T(low);
			extent[1] = T(high);
		}

		VecInterval(float v)
		{
			extent[0] = T(v);
			extent[1] = T(v);
		}

		enum EInitExpand { eInitExpand };
		VecInterval(EInitExpand e)
		{
			e;
			initExpand();
		}
		
		VecInterval(EClear e)
		{
			e;
			clear();
		}

		void clear(void)
		{
			extent[0] = extent[1] = T(0);
		}

		const T& operator[] (int i) const { return extent[DebugRange(i, 2)]; }
					T& operator[] (int i)				{ return extent[DebugRange(i, 2)]; }

		const T& low(void) const		{ return extent[0]; }
					T& low(void)					{ return extent[0]; }

		const T& high(void) const		{ return extent[1]; }
					T& high(void)					{ return extent[1]; }

		bool contains(const T& p) const
		{
			return (extent[0].allLessEqual(p)) && (p.allLessEqual(extent[1]));
		}

		bool contains(const VecInterval& v) const
		{
			for (int i = 0; i < T::numElements; i++)
				if ((extent[0][i] > v.extent[0][i]) ||
					  (extent[1][i] < v.extent[1][i]))
					return false;
			return true;
		}

		VecInterval& normalize(void) const
		{
			for (int i = 0; i < T::numElements; i++)
        if (extent[0][i] > extent[1][i])
					std::swap(extent[0][i], extent[1][i]);
			return *this;
		}

		VecInterval& initExpand(void) 
		{
			extent[0] = T(+Math::fNearlyInfinite);
			extent[1] = T(-Math::fNearlyInfinite);
			return *this;
		}

		VecInterval& expand(const T& p) 
		{
			for (int i = 0; i < T::numElements; i++)
			{
				if (p[i] < extent[0][i])
					extent[0][i] = p[i];
				if (p[i] > extent[1][i])
					extent[1][i] = p[i];
			}
			return *this;
		}

		T diagonal(void) const
		{
			return extent[1] - extent[0];
		}

		T center(void) const
		{
			return T::lerp(extent[0], extent[1], .5f);
		}

		bool overlapsAxis(const VecInterval& b, int e)
		{
			return (extent[0][e] <= b.extent[1][e]) && (extent[1][e] >= b.extent[0][e]);
		}

		bool overlaps(const VecInterval& b) const
		{
			for (int i = 0; i < T::numElements; i++)
				if (!overlapsAxis(b, i))
					return false;
			return true;
		}

		float dimension(int e) const
		{
			return extent[1][e] - extent[0][e];
		}

		int minorDimension(void) const
		{
			int d = 0;
			float v = dimension(0);
			for (int i = 1; i < T::numElements; i++)
			{
				if (dimension(i) < v)
				{
					v = dimension(i);
					d = i;
				}
			}
			return d;
		}
		
		int majorDimension(void) const
		{
			int d = 0;
			float v = dimension(0);
			for (int i = 1; i < T::numElements; i++)
			{
				if (dimension(i) > v)
				{
					v = dimension(i);
					d = i;
				}
			}
			return d;
		}

		float volume(void) const
		{
			float ret = dimension(0);
			for (int i = 1; i < T::numElements; i++)
				ret *= dimension(i);
			return ret;
		}

		float minPossibleDist2(const T& v) const
		{
			float ret = 0.0f;
			for (int i = 0; i < T::numElements; i++)
			{
				if (v[i] < extent[0][i])
					ret += sqr(extent[0][i] - v[i]);
				else if (v[i] > extent[1][i])
					ret += sqr(v[i] - extent[1][i]);
			}
			return ret;
		}
		
		float maxPossibleDist2(const T& v) const
		{
			float ret = 0.0f;
			for (int i = 0; i < T::numElements; i++)
			{
				if (v[i] < lo[i])
					ret += sqr(extent[1][i] - v[i]);
				else if (v[i] > extent[1][i])
					ret += sqr(v[i] - extent[0][i]);
				else
					ret += Math::Max(sqr(extent[1][i] - v[i]), sqr(v[i] - extent[0][i]));
			}
			return ret;
		}
		
		T toNormPos(const T& v) const
		{
			T ret;
			for (int i = 0; i < T::numElements; i++)
				ret[i] = (v[i] - low()[i]) / dimension(i);
			return ret;
		}
		
		void log(LogFile& l) const
		{
			extent[0].log(l);
			extent[1].log(l);
		}
								
		// there are 2^T::numElements possible corner points
		T corner(int p) const
		{
			const int numCorners = 1 << T::numElements;
			
			DebugRange(p, numCorners);
									
			T ret;
			for (int i = 0; i < T::numElements; i++)
				ret[i] = extent[(p >> i) & 1][i];
									
			return ret;
		}
		
		friend Stream& operator<< (Stream& dst, const VecInterval<T>& src)
		{
			return dst << src.extent[0] << src.extent[1];
		}

		friend Stream& operator>> (Stream& src, VecInterval<T>& dst)
		{
			return src >> dst.extent[0] >> dst.extent[1];
		}
	};

	typedef VecInterval<Vec<1> >	Vec1Interval;
	
	typedef VecInterval<Vec<2> >	Vec2Interval;
	typedef Vec2Interval					Rect;

	typedef VecInterval<Vec<3> >	Vec3Interval;
	typedef Vec3Interval					AABB;
	typedef std::vector<AABB>			AABBVec;

	typedef VecInterval<Vec<4> >	Vec4Interval;

} // namespace gr

#endif // GENERAL_VECTOR_H
