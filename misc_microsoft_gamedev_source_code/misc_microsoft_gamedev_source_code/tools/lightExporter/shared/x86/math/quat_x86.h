//-----------------------------------------------------------------------------
// File: quat_x86.h
// x86 optimized quaternion class.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef QUAT_X86_H
#define QUAT_X86_H

#include "vector_x86.h"

#define fQuatEpsilon .00125f

namespace gr
{
	struct EulerAngles;

	struct Quat
	{
		Vec4 vec;

		Quat();

		// directly sets vector/scaler components
		Quat(const Vec4& v);

		// directly sets vector and scaler components
		Quat(const Vec4& v, float w);

		Quat(float x, float y, float z, float w);

		Quat(const Matrix44& m);
		Quat& operator= (const Matrix44& m);

		Quat(const EulerAngles& eu);
		Quat& operator= (const EulerAngles& eu);

		Quat(const Vec<4>& v);
		Quat& operator= (const Vec<4>& v);
			
		Quat& operator= (const Quat& b);
		
		const float& operator[] (int element) const;
		
		float& operator[] (int element);
		
		Quat& setIdentity(void);

		// returns 0 quat on error
		Quat& setFromMatrix(const Matrix44& m);

		Quat& setFromAxisAngle(const Vec4& axis, float angle);
		
		// XYZ rotation ordering
		Quat& setFromEuler(const Vec4& euler);
														
		Vec4 imaginary(void) const;
		
		float scaler(void) const;
		
		// broadcasts w
		Vec4 scalerBroadcast(void) const;
		
		// rotation axis
		Vec4 axis(void) const;
		
		// rotation angle
		float angle(void) const;
		
		// squared length
		float norm(void) const;
		
		// length
		float len(void) const;
		
		// in-place normalization
		Quat normalize(void);
		
		// returns 0 or 1/sqrt(norm())
		float tryNormalize(void);
		
		// returns normalized vector
		Quat normalized(void) const;
		
		// dot product 4
		float dot(const Quat& b) const;
		
		Quat inverse(void) const;
		Quat unitInverse(void) const;
		Quat conjugate(void) const;
	
		// concatenates a and b
		friend Quat operator* (const Quat& a, const Quat& b);

		// component ops
		friend Quat operator* (const Quat& a, float s);
		friend Quat operator* (float s, const Quat& b);
		friend Quat operator/ (const Quat& a, float s);
		friend Quat operator+ (const Quat& a, const Quat& b);
		friend Quat operator- (const Quat& a, const Quat& b);
						
		Quat operator- () const;
		Quat operator+ () const;
		
		Quat& operator += (const Quat& b);
		Quat& operator -= (const Quat& b);

		bool operator== (const Quat& b) const;
		bool operator!= (const Quat& b) const;
				
		// SIMD form
		Matrix44& toMatrixSIMD1(Matrix44& m) const;
		
		// an attempt at SIMD'izing the scaler form
		// fastest on P4
		Matrix44& toMatrixSIMD2(Matrix44& m) const;
				
		// scaler form
		Matrix44& toMatrixScaler(Matrix44& m) const;
		
		// SSE scaler form
		Matrix44& toMatrixSIMDScaler(Matrix44& m) const;
		
		// converts to homogeneous 3x3 matrix (element 3,3=1)
		Matrix44& toMatrix(Matrix44& m) const;
						
		bool isPure(void) const;
		
		bool isUnit(void) const;
		
		Vec4 rotateVec(const Vec4& v) const;
		Vec4 rotateVecTransposed(const Vec4& v) const;

		static Quat makeIdentity(void);

		// returns 0 quat on error
		static Quat makeFromMatrix(const Matrix44& m);

		// returns rotation that rotates v0 into v1
		static Quat makeRotationArc(const Vec4& v0, const Vec4& v1);

		static Quat makeRotation(const Quat& from, const Quat& to);
		
		static Quat makeRandom(float x1 = Math::fRand(0.0f, 1.0f), float x2 = Math::fRand(0.0f, 1.0f), float x3 = Math::fRand(0.0f, 1.0f));

		// XYZ rotation ordering
		static Quat makeFromEuler(const Vec4& euler);
			
		static Quat unitLog(const Quat& q);
		static Quat unitExp(const Quat& q);
		// interpolates from identity 
		static Quat unitPow(const Quat& q, float p);
		// same as pow(.5)
		static Quat squareRoot(const Quat& q);

		// fastest on P4
		static Quat multiplySIMD(const Quat& a, const Quat& b);
		
		static Quat multiplyScaler(const Quat& a, const Quat& b);
		
		// concatenates a and b
		static Quat multiply(const Quat& a, const Quat& b);

		static Quat lerp(const Quat& a, const Quat& b, float t);
		static Quat lerpNoNormalize(const Quat& a, const Quat& b, float t);
		
		static Quat slerp(const Quat& a, const Quat& b, float t);
		
		static Quat slerpExtraSpins(const Quat& a, const Quat& b, float t, int extraSpins);

		// used by squad()
		static Quat slerpNoInvert(const Quat& a, const Quat& b, float t);
				
		// 3x-4x faster than slerp(), if you can tolerate the loss of accuracy
		static Quat slerpFast(const Quat& a, const Quat& b, float t);

		static Quat slerpLogForm(const Quat& a, const Quat& b, float t);

		static Quat squad(const Quat& q1, const Quat& q2, const Quat& a, const Quat& b, float t);
		static void squadSetup(Quat& pa, Quat& pb, const Quat& p0, const Quat& p1, const Quat& p2);
    			
		static bool equalTol(const Quat& a, const Quat& b, float tol = Math::fSmallEpsilon);
	
		static Quat catmullRom(const Quat& q00, const Quat& q01, const Quat& q02, const Quat& q03, float t);
		static Quat bezier(const Quat& q00, const Quat& q01, const Quat& q02, const Quat& q03, float t);
		static Quat bSpline(const Quat& q00, const Quat& q01, const Quat& q02, const Quat& q03, float t);

		static Matrix44 createMatrix(const Quat& a);
										
		static const Quat I;
		static const Quat Z;
	};

} // namespace gr

#include "quat_x86.inl"

#endif // QUAT_X86_H









