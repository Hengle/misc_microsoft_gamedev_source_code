//-----------------------------------------------------------------------------
// File: vector_x86.h
// x86 optimized vector/matrix classes.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
// FIXME: Need to check methods to make sure I'm not breaking VC7's return value optimization.
#pragma once
#ifndef VECTOR_X86_H
#define VECTOR_X86_H

#include "common/math/math.h"
#include <fvec.h>

#include <vector>

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#define M128CAST(x) (reinterpret_cast<const __m128*>(x))

namespace gr
{
	struct Quat;
	struct EulerAngles;
	class Stream;

	template<int size> struct Vec;

	__declspec(align(16))
	struct Vec4
	{
		enum { numElements = 4 };
		enum { requiresAlignment = true };

		union
		{
			__m128 vec;
			struct
			{
				float x, y, z, w;
			};
			float element[numElements];
		};

		// constructors
		Vec4();
		explicit Vec4(float s);
		Vec4(const __m128 v);
		Vec4(const Vec4& b);
		Vec4(float sx, float sy, float sz);
		Vec4(float sx, float sy, float sz, float sw);
		Vec4(const Vec4& v, float sw);
		
		template<int size>
		Vec4(const Vec<size>& b);

		template<> Vec4(const Vec<4>& b);
		Vec4(const Vec<3>& b, float sw);
				
		Vec4& operator= (float s);
		
		Vec4& operator= (const Vec4& b);

		Vec4& operator= (const __m128 v);
		operator __m128() const;
		
		template<int size>
		Vec4& operator= (const Vec<size>& b);

		bool operator== (const Vec4& b) const;
		bool operator!= (const Vec4& b) const;

		// lexicographical less 
		bool operator<(const Vec4& b) const;
				
		Vec4& set(float sx, float sy, float sz, float sw);
		Vec4& setBroadcast(float s);
		Vec4& setW(float sw);
		Vec4& setZero(void);
		Vec4& clearW(void);
		
		// element access
		const float& operator[] (int i) const;
		float& operator[] (int i);
		
		// in place vec add
		Vec4& operator+= (const Vec4& b);
		// in place vec sub
		Vec4& operator-= (const Vec4& b);
		// in place vec mul
		Vec4& operator*= (const Vec4& b);
		// in place scaler mul
		Vec4& operator*= (float s);
		// in place scaler div
		Vec4& operator/= (float s);
		
		// positive
		Vec4 operator+ () const;
		// negative
		Vec4 operator- () const;

		// scale mul
		Vec4 operator* (float s) const;
		// scale mul
		friend Vec4 operator* (float s, const Vec4& a);
		// scale div
		Vec4 operator/ (float s) const;
		
		// vec add/sub
		friend Vec4 operator+ (const Vec4& a, const Vec4& b);
		friend Vec4 operator- (const Vec4& a, const Vec4& b);
		
		// dot product 4
		friend float operator* (const Vec4& a, const Vec4& b);
		
		// cross product
		friend Vec4 operator% (const Vec4& a, const Vec4& b);
		
		// vec division
		friend Vec4 operator/ (const Vec4& a, const Vec4& b);
		
		// add elements
		float horizontalAdd(void) const;
		// sum in first element
		Vec4 horizontalAddSingle(void) const;
		// sum in all elements
		Vec4 horizontalAddBroadcast(void) const;
		
		// dot product 4
		float dot(const Vec4& b) const;
		
		// dot product 3
		float dot3(const Vec4& b) const;
		
		// cross product
		Vec4 cross(const Vec4& b) const;
		
		// squared length
		float norm(void) const;
		
		float squaredLen(void) const { return norm(); }
		
		// length
		float len(void) const;
		
		// 1/length
		float oneOverLen(void) const;
		
		// In-place normalize.
		Vec4& normalize(void);

		// In-place normalize; only uses XYZ to calc length. W is modified!!
		Vec4& normalize3(void);
		
		// returns 0 or 1/sqrt(norm())
		float tryNormalize(void);
		
		// returns normalized vector
		Vec4 normalized(void) const;
				
		// component replication
		Vec4 broadcast(const int e) const;
		
		// true if w=00
		bool isVector(void) const;
		bool isPoint(void) const;
		
		// true if unit length or nearly so
		bool isUnit(void) const;

		Vec4& project(void);
				
		// clamp components between [low, high]
		Vec4& clampComponents(float low, float high);
		
		float minComponent(void) const;
		float maxComponent(void) const;

		int minorAxis(void) const;
		int majorAxis(void) const;
		void projectionAxes(int& uAxis, int& vAxis) const;

		// [-1,1] -> [0,1]
		Vec4 rangeCompressed(void) const;
		
		// [0,1] -> [-1,1]
		Vec4 rangeExpanded(void) const;

		// returns (x,y,z,0)
		Vec4 toVector(void) const;
		
		// returns (x,y,z,1)
		Vec4 toPoint(void) const;

		Vec4 negateXYZ(void) const;
		Vec4 negateW(void) const;

		Vec4& setX(const Vec4& a);
		Vec4& setY(const Vec4& a);
		Vec4& setZ(const Vec4& a);
		Vec4& setW(const Vec4& a);
		Vec4& setXY(const Vec4& a);
		Vec4& setZW(const Vec4& a);

		operator const float* () const	{ return &x; }
					operator float* ()				{ return &x; }
		
		const float* getPtr() const	{ return &x; }
					float* getPtr()				{ return &x; }

#if DEBUG
		void debugCheck(void) const
		{
			Assert(Math::IsValidFloat(x));
			Assert(Math::IsValidFloat(y));
			Assert(Math::IsValidFloat(z));
			Assert(Math::IsValidFloat(w));
		}
#else
		void debugCheck(void) const
		{
		}
#endif
	  	
		// remove projection of v on dir from v, dir must be unit length
		static Vec4 removeCompUnit(const Vec4& v, const Vec4& dir);
		
		// returns vector with a 1 in the selected component
		static Vec4 makeAxisVector(int axis, float s = 1.0f);

		// -pi   <= yaw (long)  < pi    
		// -pi/2 <= pitch (lat) < pi/2	
		// x=yaw (XZ) -180 to 180
		// y=pitch (Y) -90 to 90
		// z=mag/radius
		static Vec4 makeCartesian(const Vec4& v);
		// x=yaw,y=pitch,z=mag
		static Vec4 makeSpherical(const Vec4& v);

		// vec multiply
		static Vec4 multiply(const Vec4& a, const Vec4& b);

		// elementwise min
		static Vec4 min(const Vec4& a, const Vec4& b);

		// elementwise max
		static Vec4 max(const Vec4& a, const Vec4& b);

		static bool equalTol(const Vec4& a, const Vec4& b, float tol = Math::fSmallEpsilon);
		static bool equalTol3(const Vec4& a, const Vec4& b, float tol = Math::fSmallEpsilon);
		
		static Vec4 lerp(const Vec4& a, const Vec4& b, float t);
	};

	extern const uint32 __declspec(align(16)) gSignMask[4];
	extern const uint32 __declspec(align(16)) gXSignMask[4];
	extern const uint32 __declspec(align(16)) gYSignMask[4];
	extern const uint32 __declspec(align(16)) gZSignMask[4];
	extern const uint32 __declspec(align(16)) gWSignMask[4];
	extern const uint32 __declspec(align(16)) gZWSignMask[4];
	extern const uint32 __declspec(align(16)) gXWSignMask[4];
	extern const uint32 __declspec(align(16)) gYWSignMask[4];
	extern const uint32 __declspec(align(16)) gXYZSignMask[4];
	extern const uint32 __declspec(align(16)) gXMask[4];
	extern const uint32 __declspec(align(16)) gYMask[4];
	extern const uint32 __declspec(align(16)) gZMask[4];
	extern const uint32 __declspec(align(16)) gWMask[4];
	extern const uint32 __declspec(align(16)) gInvWMask[4];

	extern const Vec4 gVec4One;
	extern const Vec4 gVec4Point5;
	extern const Vec4 gVec4Three;
	extern const Vec4 gVec4XOne;
	extern const Vec4 gVec4YOne;
	extern const Vec4 gVec4ZOne;
	extern const Vec4 gVec4WOne;

	extern const __declspec(align(16)) uchar gPNPN[16];
	extern const __declspec(align(16)) uchar gNPNP[16];

	// Notes:
	// Some ops are row vector centric.
	// row vector: 1x4 times 4x4 = 1x4 
	// col vector: 4x4 times 4x1 = 4x1

	__declspec(align(16))
	struct Matrix44
	{
		Vec4 row[4];

		Matrix44();

		Matrix44(
			float e00, float e01, float e02, float e03, 
			float e10, float e11, float e12, float e13, 
			float e20, float e21, float e22, float e23, 
			float e30, float e31, float e32, float e33);

		Matrix44(const Vec4& row0, const Vec4& row1, const Vec4& row2, const Vec4& row3);
		
		Matrix44(const Matrix44& b);

		Matrix44(const Quat& q);
		Matrix44& operator= (const Quat& q);

		Matrix44(const EulerAngles& eu);
		Matrix44& operator= (const EulerAngles& eu);
    		
		Matrix44& operator= (const Matrix44& b);
				
		const float& operator() (int r, int c) const;
		float& operator() (int r, int c);
		
		const Vec4& operator[] (int r) const;
		Vec4& operator[] (int r);
		
		const Vec4& getRow(int r) const;
		Matrix44& setRow(int r, const Vec4& v);
		Vec4 getColumn(int c) const;
		Matrix44& setColumn(int c, const Vec4& v);
		
		Matrix44& operator+= (const Matrix44& b);
		Matrix44& operator-= (const Matrix44& b);
				
		Matrix44& operator*= (const Matrix44& b);
				
		Matrix44& operator*= (float s);
			
		friend Matrix44 operator+ (const Matrix44& a, const Matrix44& b);
		friend Matrix44 operator- (const Matrix44& a, const Matrix44& b);
		friend Matrix44 operator* (const Matrix44& a, const Matrix44& b);
		friend Vec4 operator* (const Vec4& a, const Matrix44& b);
		friend Matrix44 operator* (const Matrix44& a, float s);
		friend Matrix44 operator* (float s, const Matrix44& a);
		friend Matrix44 operator/ (const Matrix44& a, float s);
		
		Matrix44 operator+ () const;
		Matrix44 operator- () const;
				
		Matrix44& setElements(
			float e00, float e01, float e02, float e03, 
			float e10, float e11, float e12, float e13, 
			float e20, float e21, float e22, float e23, 
			float e30, float e31, float e32, float e33);

		Matrix44& setZero(void);
		Matrix44& setIdentity(void);
		
		// in-place transpose
		Matrix44& transpose(void);
		
		// in-place upper 3x3 transpose
		Matrix44& transpose3x3(void);
		
		// returns transpose of matrix
		Matrix44 transposed(void) const;
			
		// in-place invert, returns det
		float invert(void);

		Matrix44& invertSlow(void);
		
		Matrix44 inverseSlow(void) const
		{
			return Matrix44(*this).invertSlow();
		}
			
		// returns inverse of matrix
		Matrix44 inverse(void) const;

		// slightly hacked Gram-Schmidt
		void orthonormalize(void);

		float det(void) const;
		
		const Vec4& getTranslate(void) const;
		Matrix44& setTranslate(const Vec4& t);
		Matrix44& setTranslate(const Vec4& t, float w);
		Matrix44& setTranslate(const Vec<3>& t);
		
		bool isOrthonormal3x3(void) const;
		bool hasNoReflection3x3(void) const;

		Stream& writeText(Stream& stream) const;
		void debugDump(void) const;

		operator const	float* () const	{ return &row[0].x; }
		operator				float* ()				{ return &row[0].x; }

		const	float* getPtr() const	{ return &row[0].x; }
					float* getPtr()				{ return &row[0].x; }

#if DEBUG
		void debugCheck(void) const
		{
			for (int i = 0; i < 4; i++)
				row[i].debugCheck();
		}
#else
		void debugCheck(void) const
		{
		}
#endif
						
		static Matrix44& multiplyToDest(Matrix44& dest, const Matrix44& a, const Matrix44& b);
		static Matrix44& multiply3x4ToDest(Matrix44& dest, const Matrix44& a, const Matrix44& b);
		static Matrix44 multiply(const Matrix44& a, const Matrix44& b);

		// (row vector) 1x4 * 4x4 = 1x4
		static Vec4 transform(const Vec4& a, const Matrix44& b);
			
		// effectively transposes b before xforming
		// (a is treated as column vector)
		static Vec4 transformTransposed(const Matrix44& b, const Vec4& a);
		
		// assumes w = 0
		static Vec4 transformNormal(const Vec4& a, const Matrix44& b);
		
		// assumes w = 1
		static Vec4 transformPoint(const Vec4& a, const Matrix44& b);
		
		// assumes w = 0
		// (a is treated as column vector)
		static Vec4 transformNormalTransposed(const Matrix44& b, const Vec4& a);
		
		// assumes w = 1
		// (a is treated as column vector)
		static Vec4 transformPointTransposed(const Matrix44& b, const Vec4& a);

		// From Intel Appnote AP-928 "Streaming SIMD Extensions - Inverse of 4x4 Matrix"
		static float invertCramersRuleSSE(float* dst, const float* src);

		static Matrix44 makeScale(const Vec4& s);
		static Matrix44 makeIdentity(void);
		static Matrix44 makeZero(void);
		static Matrix44 makeTranslate(const Vec4& t);
		static Matrix44 makeRotate(const Vec4& axis, float ang);
		static Matrix44 makeRotate(int axis, float ang);
		static Matrix44 makeRotation(const Vec4& from, const Vec4& to);
		static Matrix44 makeRightToLeft(void);
		static Matrix44 makeTensorProduct(const Vec4& v, const Vec4& w);
		static Matrix44 makeCrossProduct(const Vec4& w);
		static Matrix44 makeReflection(const Vec4& n, const Vec4& q);
		static Matrix44 makeUniformScaling(const Vec4& q, float c);
		static Matrix44 makeNonuniformScaling(const Vec4& q, float c, const Vec4& w);
		// n = normal of plane, q = point on plane
		static Matrix44 makeOrthoProjection(const Vec4& n, const Vec4& q);
		static Matrix44 makeParallelProjection(const Vec4& n, const Vec4& q, const Vec4& w);
		static Matrix44 makeCamera(const Vec4& pos, const Vec4& at, const Vec4& up, float roll = 0.0f);
		static Matrix44 makeMaxToD3D(void);

		static bool equalTol(const Matrix44& a, const Matrix44& b, float tol = Math::fSmallEpsilon);
		static bool equalTol3x3(const Matrix44& a, const Matrix44& b, float tol = Math::fSmallEpsilon);

		static Matrix44 I;
	};
	
	typedef Vec<2> Vec2;
	typedef Vec<3> Vec3;
	typedef std::vector<Vec3> Vec3Vec;
	typedef Vec<9> Vec9;
} // namespace gr

#define GENMATRIX_DEF_PLATFORM_METHODS \
		Vec(const Vec4& b) \
		{ \
			*this = b; \
		} \
		Vec& operator= (const Vec4& b) \
		{ \
			set(b); \
			return *this; \
		} \
		Vec& set(const Vec4& b)	\
		{ \
			int n = Math::Min(size, 4); \
			for (int i = 0; i < n; i++)	\
				element[i] = b.element[i]; \
			for ( ; i < size; i++) \
				element[i] = 0; \
			return *this; \
		} 

#include "common/math/general_vector.h"

#include "vector_x86.inl"

#endif // VECTOR_X86_H
