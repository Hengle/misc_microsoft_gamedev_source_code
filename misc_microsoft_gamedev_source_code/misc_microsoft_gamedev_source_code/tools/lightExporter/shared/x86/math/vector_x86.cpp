//-----------------------------------------------------------------------------
// File: vector_x86.cpp
// x86 optimized vector/matrix classes.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "vector_x86.h"
#include "quat_x86.h"
#include "common/math/euler_angles.h"
#include "common/utils/stream.h"

#pragma warning (disable:4700)    

namespace gr
{
	const uint32 __declspec(align(16)) gSignMask[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
	const uint32 __declspec(align(16)) gXSignMask[4] = { 0x80000000, 0x00000000, 0x00000000, 0x00000000 };
	const uint32 __declspec(align(16)) gYSignMask[4] = { 0x00000000, 0x80000000, 0x00000000, 0x00000000 };
	const uint32 __declspec(align(16)) gZSignMask[4] = { 0x00000000, 0x00000000, 0x80000000, 0x00000000 };
	const uint32 __declspec(align(16)) gWSignMask[4] = { 0x00000000, 0x00000000, 0x00000000, 0x80000000 };
	const uint32 __declspec(align(16)) gZWSignMask[4] = { 0x00000000, 0x00000000, 0x80000000, 0x80000000 };
	const uint32 __declspec(align(16)) gXWSignMask[4] = { 0x80000000, 0x00000000, 0x00000000, 0x80000000 };
	const uint32 __declspec(align(16)) gYWSignMask[4] = { 0x00000000, 0x80000000, 0x00000000, 0x80000000 };
	const uint32 __declspec(align(16)) gXYZSignMask[4] = { 0x80000000, 0x80000000, 0x80000000, 0x00000000 };
	const uint32 __declspec(align(16)) gXMask[4] = { 0xffffffff, 0x00000000, 0x00000000, 0x00000000 };
	const uint32 __declspec(align(16)) gYMask[4] = { 0x00000000, 0xffffffff, 0x00000000, 0x00000000 };
	const uint32 __declspec(align(16)) gZMask[4] = { 0x00000000, 0x00000000, 0xffffffff, 0x00000000 };
	const uint32 __declspec(align(16)) gWMask[4] = { 0x00000000, 0x00000000, 0x00000000, 0xffffffff };
	const uint32 __declspec(align(16)) gInvWMask[4] = { 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000 };

	const Vec4 gVec4One(1.0f);
	const Vec4 gVec4Point5(0.5f);
	const Vec4 gVec4Three(3.0f);
	const Vec4 gVec4XOne(1.0f, 0.0f, 0.0f, 0.0f);
	const Vec4 gVec4YOne(0.0f, 1.0f, 0.0f, 0.0f);
	const Vec4 gVec4ZOne(0.0f, 0.0f, 1.0f, 0.0f);
	const Vec4 gVec4WOne(0.0f, 0.0f, 0.0f, 1.0f);

	const __declspec(align(16)) uchar gPNPN[16] = {  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x80,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x80 };
	const __declspec(align(16)) uchar gNPNP[16] = {  0x00, 0x00, 0x00, 0x80,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x80,  0x00, 0x00, 0x00, 0x00 };

	Matrix44 Matrix44::I(Matrix44::makeIdentity());

	Matrix44::Matrix44(const Quat& q)
	{
		q.toMatrix(*this);
	}

	Matrix44& Matrix44::operator= (const Quat& q)
	{
		return q.toMatrix(*this);
	}

	Matrix44::Matrix44(const EulerAngles& eu)
	{
		eu.toMatrix(*this);
	}

	Matrix44& Matrix44::operator= (const EulerAngles& eu)
	{
		return eu.toMatrix(*this);
	}

	Matrix44 Matrix44::makeRotate(const Vec4& axis, float ang)
	{
		Matrix44 ret;
		ret.setZero();

		Vec4 nrm(axis.normalized());

		float sinA = (float)sin(ang);
		float cosA = (float)cos(ang);
		float invCosA = 1.0f - cosA;

		const float x = nrm.x;
		const float y = nrm.y;
		const float z = nrm.z;

		nrm *= nrm;

		const float xSq = nrm.x;
		const float ySq = nrm.y;
		const float zSq = nrm.z;
		
		ret[0][0] = (invCosA * xSq) + cosA;
		ret[0][1] = (invCosA * x * y) - (sinA * z);
		ret[0][2] = (invCosA * x * z) + (sinA * y);

		ret[1][0] = (invCosA * x * y) + (sinA * z);
		ret[1][1] = (invCosA * ySq) + cosA;
		ret[1][2] = (invCosA * y * z) - (sinA * x);

		ret[2][0] = (invCosA * x * z) - (sinA * y);
		ret[2][1] = (invCosA * y * z) + (sinA * x);
		ret[2][2] = (invCosA * zSq) + cosA;

		ret[3][3] = 1.0f;

		return ret;
	}
	
	Matrix44 Matrix44::makeRotate(int axis, float ang)
	{
		Matrix44 ret;
		ret.setZero();
		ret[3][3] = 1.0f;

		float sinA = (float)sin(ang);
		float cosA = (float)cos(ang);
		
		switch (axis)
		{
			case 0:
				ret[1][1] =  cosA; ret[1][2] = -sinA;
				ret[2][1] =  sinA; ret[2][2] =  cosA;
				break;
			case 1:
				ret[0][0] =  cosA; ret[0][2] =  sinA;
				ret[2][0] = -sinA; ret[2][2] =  cosA;
				break;
			case 2:
				ret[0][0] =  cosA; ret[0][1] = -sinA; 
				ret[1][0] =  sinA; ret[1][1] =  cosA; 
				break;
			default:
				Assert(false);
				__assume(0);
		}
		
		ret[axis][axis] =  1.0F; 

		return ret;
	}

	// Intel Appnote AP-928 "Streaming SIMD Extensions - Inverse of 4x4 Matrix"
	// Is this code OK to use??
	float Matrix44::invertCramersRuleSSE(float* dst, const float* src)
	{
		__m128 minor0, minor1, minor2, minor3;
		__m128 row0, row1, row2, row3;
		__m128 det, tmp1;
#if DEBUG
		tmp1 = row1 = row3 = _mm_setzero_ps();
#endif
		tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src+ 4));
		row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src+8)), (__m64*)(src+12));
		row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
		row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
		tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src+ 2)), (__m64*)(src+ 6));
		row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src+10)), (__m64*)(src+14));
		row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
		row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row2, row3);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor0 = _mm_mul_ps(row1, tmp1);
		minor1 = _mm_mul_ps(row0, tmp1);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
		minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
		minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row1, row2);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
		minor3 = _mm_mul_ps(row0, tmp1);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
		minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
		minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		row2 = _mm_shuffle_ps(row2, row2, 0x4E);
		minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
		minor2 = _mm_mul_ps(row0, tmp1);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
		minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
		minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row0, row1);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
		minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
		minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row0, row3);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
		minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
		minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));
		// -----------------------------------------------
		tmp1 = _mm_mul_ps(row0, row2);
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
		minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
		minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
		tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
		minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
		minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);
		// -----------------------------------------------
		det = _mm_mul_ps(row0, minor0);
		det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
		det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);

// not precise enough.. why?
//		tmp1 = _mm_rcp_ss(det);
//		det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
//		det = _mm_shuffle_ps(det, det, 0x00);

		const float ret = *(float*)&det;
		det = _mm_set_ps1(1.0f / ret);
		
		minor0 = _mm_mul_ps(det, minor0);
		_mm_storel_pi((__m64*)(dst), minor0);
		_mm_storeh_pi((__m64*)(dst+2), minor0);
		minor1 = _mm_mul_ps(det, minor1);
		_mm_storel_pi((__m64*)(dst+4), minor1);
		_mm_storeh_pi((__m64*)(dst+6), minor1);
		minor2 = _mm_mul_ps(det, minor2);
		_mm_storel_pi((__m64*)(dst+ 8), minor2);
		_mm_storeh_pi((__m64*)(dst+10), minor2);
		minor3 = _mm_mul_ps(det, minor3);
		_mm_storel_pi((__m64*)(dst+12), minor3);
		_mm_storeh_pi((__m64*)(dst+14), minor3);
		
		return ret;
	}

	void Matrix44::orthonormalize(void)
	{
		Vec4 x(row[0]);
		Vec4 y(row[1]);
		Vec4 z(row[2]);
		x.normalize();
		y = Vec4::removeCompUnit(y, x).normalize();
		Vec4 new_z(x % y);
		new_z.normalize();
		if ((new_z * z) < 0.0f)
			new_z *= -1.0f;
		row[0] = x;
		row[1] = y;
		row[2] = new_z;
	}

	float Matrix44::det(void) const
	{
		const __m128 temp = _mm_ror_ps(row[2], 1);

		__m128 a = _mm_mul_ps(temp, _mm_ror_ps(row[3], 2));					
		__m128 b = _mm_mul_ps(temp, _mm_ror_ps(row[3], 3));					
		__m128 c = _mm_mul_ps(temp, _mm_ror_ps(row[3], 0));					

		const __m128 r1 = _mm_sub_ps(_mm_ror_ps(a, 1),_mm_ror_ps(c, 2));		
		const __m128 r2 = _mm_sub_ps(_mm_ror_ps(b, 2),_mm_ror_ps(b, 0));		
		const __m128 r3 = _mm_sub_ps(_mm_ror_ps(a, 0),_mm_ror_ps(c, 1));		

		a = _mm_ror_ps(row[1], 1);		
		b = _mm_ror_ps(a, 1);		
		c = _mm_ror_ps(b, 1);		

		__m128 s = _mm_add_ps(_mm_add_ps(_mm_mul_ps(a, r1), _mm_mul_ps(b, r2)), _mm_mul_ps(c, r3));

		s = _mm_mul_ps(s, row[0]);
		s = _mm_add_ps(s, _mm_movehl_ps(s, s));
		s = _mm_sub_ss(s, _mm_shuffle_ps(s, s, 1));

		float ret;
		_mm_store_ss(&ret, s);
		return ret;
	}

	bool Matrix44::isOrthonormal3x3(void) const
	{
		Vec4 a(row[0].toVector());
		Vec4 b(row[1].toVector());
		Vec4 c(row[2].toVector());

		return 
			a.isUnit() &&	b.isUnit() && c.isUnit() &&
			Math::EqualTol(0.0f, a * b) && Math::EqualTol(0.0f, a * c) && Math::EqualTol(0.0f, b * c);
	}

	bool Matrix44::hasNoReflection3x3(void) const
	{
		Assert(isOrthonormal3x3());
		return Vec4::equalTol3(row[0] % row[1], row[2]);
  }

	// See: T. Moller and J. F. Hughes, "Efficiently Building a Matrix
	//      to Rotate One Vector to Another," Journal of Graphics Tools, 4(4):1-4, 1999.
	Matrix44 Matrix44::makeRotation(const Vec4& from, const Vec4& to)
	{
		Assert(from.isUnit() && from.isVector());
		Assert(to.isUnit() && to.isVector());

		Matrix44 res;
		
		res.setIdentity();

		float ang = from * to;
		if (Math::EqualTol(ang, 1.0f, Math::fTinyEpsilon))
		{
			
    }
		else if (Math::EqualTol(ang, -1.0f, Math::fTinyEpsilon))
		{
			Vec4 side(0.0f, from.z, -from.y, 0.0f);

			if (Math::EqualTol(side * side, 0.0f, Math::fTinyEpsilon))
			{
				side.x = -from.z;
				side.y = 0.f;
				side.z = from.x;
			}
			
			if (side.tryNormalize() == 0.0f)
			{
				Assert(false);
			}

			Vec4 up((side % from).normalize());
			
		  res.row[0][0] = (up.x * up.x) - (from.x * from.x) - (side.x * side.x);
			res.row[0][1] = (up.x * up.y) - (from.x * from.y) - (side.x * side.y);
			res.row[0][2] = (up.x * up.z) - (from.x * from.z) - (side.x * side.z);

			res.row[1][0] = (up.x * up.y) - (from.x * from.y) - (side.x * side.y);
			res.row[1][1] = (up.y * up.y) - (from.y * from.y) - (side.y * side.y);
			res.row[1][2] = (up.y * up.z) - (from.y * from.z) - (side.y * side.z);

			res.row[2][0] = (up.x * up.z) - (from.x * from.z) - (side.x * side.z);
			res.row[2][1] = (up.y * up.z) - (from.y * from.z) - (side.y * side.z);
			res.row[2][2] = (up.z * up.z) - (from.z * from.z) - (side.z * side.z);
		}
		else
		{
			Vec4 v(from % to);
		
			const float temp = (1.0f - ang) / v.norm();

			res.setRow(0, Vec4(ang + temp * v.x * v.x, temp * v.x * v.y + v.z, temp * v.x * v.z - v.y, 0));
			res.setRow(1, Vec4(temp * v.x * v.y - v.z, ang + temp * v.y * v.y, temp * v.y * v.z + v.x, 0));
			res.setRow(2, Vec4(temp * v.x * v.z + v.y, temp * v.y * v.z - v.x, ang + temp * v.z * v.z, 0));
		}

		return res;
	}

	Matrix44 Matrix44::makeCamera(const Vec4& pos, const Vec4& at, const Vec4& up, float roll)
	{
		Matrix44 rot;

		Assert(pos.isPoint());
		Assert(at.isPoint());

		Vec4 z(at - pos);
		if (z.tryNormalize() < Math::fSmallEpsilon)
			z = gVec4ZOne;

		Vec4 y(up);
		
		if (!z.x && !z.z)
			y = Vec4(-1.0f, 0.0f, 0.0f, 0.0f);

		// HACK HACK
		if ((y * z) >= (1.0f - Math::fTinyEpsilon))
			y = Vec4(0,1,0,0);

		Vec4 x((y % z).normalize());
		
		y = (z % x).normalize();

		rot.setColumn(0, x);
		rot.setColumn(1, y);
		rot.setColumn(2, z);
		rot.setColumn(3, gVec4WOne);
		return Matrix44::makeTranslate((-pos).toPoint()) * rot * Matrix44::makeRotate(2, roll);
	}

	Matrix44& Matrix44::invertSlow(void)
	{
	  Matrix44 a(*this);
		Matrix44 b(makeIdentity());

		for (int c = 0; c < 4; c++)
		{
			int rMax = c;
			for (int r = c + 1; r < 4; r++)
				if (fabs(a[r][c]) > fabs(a[rMax][c]))
					rMax = r;

			if (0.0f == a[rMax][c])
			{
				setIdentity();
				return *this;
			}

			std::swap(a[c], a[rMax]);
			std::swap(b[c], b[rMax]);
			
			b[c] /= a[c][c];
			a[c] /= a[c][c];

			for (int row = 0; row < 4; row++)
			{
				if (row != c)
				{
					const Vec4 temp(a[row][c]);
					a[row] -= Vec4::multiply(a[c], temp);
					b[row] -= Vec4::multiply(b[c], temp);
				}
			}
		}

		*this = b;
		
		return *this;
	}

	Matrix44 Matrix44::makeMaxToD3D(void)
	{
		Matrix44 ret;
		ret.setIdentity();
		ret[0][0] = -1.0f;
		ret[2][1] = 1.0f;
		ret[1][1] = 0.0f;
		ret[1][2] = -1.0f;
		ret[2][2] = 0.0f;
		return ret;
	}

	Stream& Matrix44::writeText(Stream& stream) const
	{
		for (int i = 0; i < 4; i++)
			stream.printf("%f %f %f %f\n", row[i][0], row[i][1], row[i][2], row[i][3]);
		return stream;
	}
	
	void Matrix44::debugDump(void) const
	{
		for (int i = 0; i < 4; i++)
			Trace("%f %f %f %f\n", row[i][0], row[i][1], row[i][2], row[i][3]);
	}
	
} // namespace gr














