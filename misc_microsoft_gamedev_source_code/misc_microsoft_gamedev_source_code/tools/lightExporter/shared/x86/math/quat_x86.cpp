//-----------------------------------------------------------------------------
// File: quat_x86.cpp
// x86 optimized quaternion class.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "quat_x86.h"
#include "common/math/euler_angles.h"

namespace gr
{
	const Quat Quat::I(0.0f, 0.0f, 0.0f, 1.0f);
	const Quat Quat::Z(0.0f, 0.0f, 0.0f, 0.0f);

	Quat::Quat(const EulerAngles& eu)
	{
		eu.toQuat(*this);
	}

	Quat& Quat::operator= (const EulerAngles& eu)
	{
		return eu.toQuat(*this);
	}

	Quat& Quat::operator= (const Matrix44& m)
	{
		*this = makeFromMatrix(m);
		return *this;
	}

	Quat Quat::makeFromMatrix(const Matrix44& m)
	{
		Quat ret;

		if (!m.hasNoReflection3x3())
			return Quat(0.0f, 0.0f, 0.0f, 0.0f);

		// See "Quaternion Calculus and Fast Animation", Ken Shoemake 1987 SIGGRAPH course notes.

		double trace = m[0][0] + m[1][1] + m[2][2];

		if (trace > 0.0f) 
		{
			double s = sqrt(trace + 1.0f);
			
			ret.vec[3] = s * .5f;
			s = 0.5f / s;
			ret.vec[0] = (m[2][1] - m[1][2]) * s;
			ret.vec[1] = (m[0][2] - m[2][0]) * s;
			ret.vec[2] = (m[1][0] - m[0][1]) * s;
		}
		else 
		{
			int i = 0;
			if (m[1][1] > m[0][0])
				i = 1;
			if (m[2][2] > m[i][i])
				i = 2;
			
			const int j = Math::NextWrap(i, 3);  
			const int k = Math::NextWrap(j, 3);

			double s = sqrt(1.0f + m[i][i] - m[j][j] - m[k][k]);

			ret.vec[i] = s * 0.5f;
			Assert(s > 0.0f);
			s = 0.5f / s;
			ret.vec[3] = (m[k][j] - m[j][k]) * s;
			ret.vec[j] = (m[j][i] + m[i][j]) * s;
			ret.vec[k] = (m[k][i] + m[i][k]) * s;
		}
#if DEBUG
		Assert(ret.isUnit());
		Matrix44 temp;
		ret.toMatrix(temp);
		Assert(Matrix44::equalTol3x3(m, temp, Math::fSmallEpsilon));
#endif

		return ret;
	}

	Quat Quat::slerp(const Quat& a, const Quat& b, float t)
	{
		float d = a.vec * b.vec;
		
		Vec4 bvec(b.vec);

		if (d < 0.0f)
		{
			d = -d;
			bvec = -bvec;
		}

		if (d > (1.0f - fQuatEpsilon))
			return lerp(a, bvec, t);		

		float omega = acos(d);
		float l = sin(omega * (1.0f - t));
		float r = sin(omega * t);

		return (a.vec * l + bvec * r).normalize();
	}

	Quat Quat::slerpExtraSpins(const Quat& a, const Quat& b, float t, int extraSpins)
	{
		float d = a.vec * b.vec;
		
		Vec4 bvec(b.vec);

		if (d < 0.0f)
		{
			d = -d;
			bvec = -bvec;
		}

		if (d > (1.0f - fQuatEpsilon))
			return lerp(a, bvec, t);

		float omega = acos(d);
		float phase = Math::fPi * extraSpins * t;
		float l = sin(omega * (1.0f - t) - phase);
		float r = sin(omega * t + phase);

		return (a.vec * l + bvec * r).normalize();
	}

	Quat Quat::slerpNoInvert(const Quat& a, const Quat& b, float t)
	{
		float d = a.vec * b.vec;
		
		if (d > (1.0f - fQuatEpsilon))
			return lerp(a, b, t);

		// should probably check for d close to -1 here

		Vec4 bvec(b.vec);

		float omega = acos(d);
		float l = sin(omega * (1.0f - t));
		float r = sin(omega * t);

		return (a.vec * l + bvec * r).normalize();
	}

	Quat Quat::squad(const Quat& p0, const Quat& pa, const Quat& pb, const Quat& p1, float t)
	{
		return slerpNoInvert(
			slerpNoInvert(p0, p1, t), 
			slerpNoInvert(pa, pb, t), 
			2.0f * t * (1.0f - t));
	}

	void Quat::squadSetup(Quat& pa, Quat& pb,	const Quat& p0, const Quat& p1, const Quat& p2)
	{
		Quat temp(0.25f * (unitLog(p0.unitInverse() * p1) - unitLog(p1.unitInverse() * p2)));
    pa = p1 * unitExp(temp);
    pb = p1 * unitExp(-temp);
	}

	// 3x-4x faster than slerp(), if you can tolerate less accuracy
	Quat Quat::slerpFast(const Quat& a, const Quat& b, float t)
	{
		float d = a.vec * b.vec;
		if (d > (1.0f - fQuatEpsilon))
			return lerp(a, b, t);

		Vec4 bvec(b.vec);

		if (d < 0.0f)
		{
			d = -d;
			bvec = -bvec;
		}

		float omega = Math::fFastACos(d);
		float l = Math::fFastSin(omega * (1.0f - t));
		float r = Math::fFastSin(omega * t);

		return (a.vec * l + bvec * r).normalize();
	}

	Quat Quat::makeRotationArc(const Vec4& v0, const Vec4& v1)
	{
		Assert(v0.isVector());
		Assert(v1.isVector());
		Quat q;
		const Vec4 c(v0 % v1);
		const float d = v0 * v1;
		const float s = (float)sqrt((1 + d) * 2);
		// could multiply by 1/s instead
		q[0] = c[0] / s;
		q[1] = c[1] / s;
		q[2] = c[2] / s;
		q[3] = s * .5f;
		return q;
	}
	
	Quat Quat::unitLog(const Quat& q) 
	{
		Assert(q.isUnit());
		float scale = sqrt(Math::Sqr(q.vec[0]) + Math::Sqr(q.vec[1]) + Math::Sqr(q.vec[2]));
		float t = atan2(scale, q.vec[3]);
		if (scale > 0.0f) 
			scale = t / scale;
		else 
			scale = 1.0f;
		return Quat(scale * q.vec[0], scale * q.vec[1], scale * q.vec[2], 0.0f);
	}

	Quat Quat::unitExp(const Quat& q) 
	{
		Assert(q.isPure());

		float t = sqrt(Math::Sqr(q.vec[0]) + Math::Sqr(q.vec[1]) + Math::Sqr(q.vec[2]));
		float scale;
		if (t > Math::fSmallEpsilon) 
			scale = sin(t) / t;
		else
			scale = 1.0f;
		
		Quat ret(scale * q.vec[0], scale * q.vec[1], scale * q.vec[2], cos(t));

#if DEBUG
		Quat temp(unitLog(ret));
		Assert(Quat::equalTol(temp, q, Math::fLargeEpsilon));
#endif
		
		return ret;
	}

	// interpolates from identity 
	Quat Quat::unitPow(const Quat& q, float p) 
	{
		return unitExp(p * unitLog(q));
	}

	Quat Quat::slerpLogForm(const Quat& a, const Quat& b, float t)
	{
		return a * unitPow(a.unitInverse() * b, t);
	}

	// same as pow(.5)
	Quat Quat::squareRoot(const Quat& q) 
	{
		Assert(q.isUnit());

		const float k = Math::ClampLow(0.5f * (q.scaler() + 1.0f), 0.0f);
		const float s = sqrt(k);	
		const float d = 1.0f / (2.0f * s);
		
		Quat ret(q.vec[0] * d, q.vec[1] * d, q.vec[2] * d, s);

#if DEBUG
		Assert(ret.isUnit());
		Quat temp(ret * ret);
		Assert(Quat::equalTol(temp, q, Math::fLargeEpsilon));
#endif

		return ret;
	}

	Quat Quat::catmullRom(const Quat& q00, const Quat& q01, const Quat& q02, const Quat& q03, float t)
	{
		Quat q10(slerp(q00, q01, t + 1.0f));	Quat q11(slerp(q01, q02, t)); Quat q12(slerp(q02, q03, t - 1.0f));
		Quat q20(slerp(q10, q11, (t + 1.0f) * .5f));	Quat q21(slerp(q11, q12, t * .5f));
		return slerp(q20, q21, t);
	}

	Quat Quat::bezier(const Quat& q00, const Quat& q01, const Quat& q02, const Quat& q03, float t)
	{
		Quat q10(slerp(q00, q01, t)); Quat q11(slerp(q01, q02, t));	Quat q12(slerp(q02, q03, t));
		Quat q20(slerp(q10, q11, t)); Quat q21(slerp(q11, q12, t));
		return slerp(q20, q21, t);
	}

	Quat Quat::bSpline(const Quat& q00, const Quat& q01, const Quat& q02, const Quat& q03, float t)
	{
		Quat q10(slerp(q00, q01, (t + 2.0f) / 3.0f)); Quat q11(slerp(q01, q02, (t + 1.0f) / 3.0f)); Quat q12(slerp(q02, q03, t / 3.0f));
		Quat q20(slerp(q10, q11, (t + 1.0f) * .5f)); Quat q21(slerp(q11, q12, t * .5f));
		return slerp(q20, q21, t);
	}

	Quat Quat::makeRandom(float x1, float x2, float x3)
	{
		const float z = x1;
		const float o = Math::fTwoPi * x2;
		const float r = sqrt(1.0f - z * z);
		const float w = Math::fPi * x3;
		const float sw = sin(w);
		return Quat(sw * cos(o) * r, sw * sin(o) * r, sw * z, cos(w));
	}

	Quat& Quat::setFromEuler(const Vec4& euler)
	{
		const float a = euler[0];
		const float b = euler[1];
		const float c = euler[2];
		// not a particularly fast method
		const Quat qX(sin(a * .5f), 0, 0, cos(a * .5f)); // roll
		const Quat qY(0, sin(b * .5f), 0, cos(b * .5f)); // yaw
		const Quat qZ(0, 0, sin(c * .5f), cos(c * .5f)); // pitch
		*this = qX * qY * qZ;
		return *this;
	}

	Quat Quat::makeFromEuler(const Vec4& euler)
	{
		Quat ret;
		return ret.setFromEuler(euler);
	}

} // namespace gr
