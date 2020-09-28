//-----------------------------------------------------------------------------
// File: quat_x86.inl
// x86 optimized quaternion class.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

namespace gr 
{
	inline Quat::Quat()
	{
	}

	// directly sets vector/scaler components
	inline Quat::Quat(const Vec4& v) : 
		vec(v)
	{
	}

	// directly sets vector and scaler components
	inline Quat::Quat(const Vec4& v, float w) : vec(v, w)
	{
	}

	inline Quat::Quat(float x, float y, float z, float w) :
		vec(x, y, z, w)
	{
	}

	inline Quat::Quat(const Matrix44& m)
	{
		setFromMatrix(m);
	}

	inline Quat::Quat(const Vec<4>& v) :
		vec(v[0], v[1], v[2], v[3])
	{
	}

	inline Quat& Quat::operator= (const Vec<4>& v)
	{
		vec.x = v[0];
		vec.y = v[1];
		vec.z = v[2];
		vec.w = v[3];
		return *this;
	}

	inline Quat& Quat::operator= (const Quat& b)
	{
		vec = b.vec;
		return *this;
  }

	inline const float& Quat::operator[] (int element) const
	{
		return vec[element];
	}

	inline float& Quat::operator[] (int element) 
	{
		return vec[element];
	}

	inline Quat& Quat::setFromAxisAngle(const Vec4& axis, float angle)
	{
		Assert(axis.isVector());
		if (axis.norm() < Math::fSmallEpsilon)
			setIdentity();
		else			
		{
			float s, c;
			Math::fSinCos(angle * .5f, s, c);

			vec = (axis.normalized() * s).setW(c);
		}
      		
		return *this;
	}

	inline Quat& Quat::setIdentity(void)
	{
		vec.set(0.0f, 0.0f, 0.0f, 1.0f);
		return *this;
	}
	
	inline Vec4 Quat::imaginary(void) const
	{
		return vec.toVector();
	}

	inline float Quat::scaler(void) const
	{
		return vec.w;
	}

	// broadcasts w
	inline Vec4 Quat::scalerBroadcast(void) const
	{
		return vec.broadcast(3);
	}

	// rotation axis
	inline Vec4 Quat::axis(void) const
	{
		Assert(isUnit());
		Vec4 ret(imaginary());
		if (ret.tryNormalize() == 0.0f)
			ret = Vec4(1, 0, 0, 0);
    return ret;
	}

	// rotation angle
	inline float Quat::angle(void) const
	{
		Assert(isUnit());
		return acos(Math::Clamp(vec.w, -1.0f, 1.0f)) * 2.0f;
	}

	// squared length
	inline float Quat::norm(void) const
	{
		return vec.norm();
	}

	// length
	inline float Quat::len(void) const
	{
		return sqrt(norm());
	}

	// in-place normalization
	inline Quat Quat::normalize(void) 
	{
		vec.normalize();
		return *this;
	}

	// returns 0 or 1/sqrt(norm())
	inline float Quat::tryNormalize(void)
	{
		return vec.tryNormalize();
	}

	// returns normalized vector
	inline Quat Quat::normalized(void) const
	{
		return Quat(*this).normalize();
	}

	// dot product 4
	inline float Quat::dot(const Quat& b) const
	{
		return vec * b.vec;
	}

	inline Quat Quat::inverse(void) const
	{
		float n = norm();
		if (n == 0.0)
			return Quat(0, 0, 0, 0);
		return Quat(vec.negateXYZ() / n);
	}

	inline Quat Quat::unitInverse(void) const
	{
		Assert(isUnit());
		return Quat(vec.negateXYZ());
	}

	inline Quat Quat::conjugate(void) const
	{
		return Quat(vec.negateXYZ());
	}

	// fastest on P4
	inline Quat Quat::multiplySIMD(const Quat& a, const Quat& b) 
	{
		return Quat(
			(a.vec % b.vec) + Vec4::multiply(a.scalerBroadcast(), b.vec) + Vec4::multiply(b.scalerBroadcast(), a.vec),
			a.vec.negateXYZ() * b.vec
		);
  }

	inline Quat Quat::multiplyScaler(const Quat& a, const Quat& b) 
	{
		return Quat(
				a[1] * b[2]	- a[2] * b[1]	+ a[3] * b[0]	+ a[0] * b[3],
				a[2] * b[0]	- a[0] * b[2]	+ a[3] * b[1]	+ a[1] * b[3],
				a[0] * b[1]	- a[1] * b[0]	+ a[3] * b[2]	+ a[2] * b[3],
				a[3] * b[3]	- a[0] * b[0]	- a[1] * b[1]	- a[2] * b[2]
			);
  }

	// concatenates a and b
	inline Quat Quat::multiply(const Quat& a, const Quat& b) 
	{
		return multiplySIMD(a, b);
	}

	// concatenates a and b
	inline Quat operator* (const Quat& a, const Quat& b) 
	{
		return Quat::multiply(a, b);
  }
			
	inline Quat operator* (const Quat& a, float s) 
	{
		return Quat(a.vec * s);
	}
	
	inline Quat operator/ (const Quat& a, float s)
	{
		return Quat(a.vec / s);
	}
	
	inline Quat Quat::operator- () const
	{
		return Quat(-vec);
	}

	inline Quat Quat::operator+ () const
	{
		return Quat(vec);
	}

	inline Quat& Quat::operator += (const Quat& b) 
	{
		vec += b.vec;
		return *this;
	}

	inline Quat& Quat::operator -= (const Quat& b) 
	{
		vec -= b.vec;
		return *this;
	}

	// SIMD form
	inline Matrix44& Quat::toMatrixSIMD1(Matrix44& m) const
	{
		Matrix44 l, r;

		r[0] = _mm_xor_ps(_mm_shuffle_ps(vec, vec, SSE_SHUFFLE(3, 2, 1, 0)), *(const __m128*)gYSignMask);
		r[1] = _mm_xor_ps(_mm_shuffle_ps(vec, vec, SSE_SHUFFLE(2, 3, 0, 1)), *(const __m128*)gZSignMask);
		r[2] = _mm_xor_ps(_mm_shuffle_ps(vec, vec, SSE_SHUFFLE(1, 0, 3, 2)), *(const __m128*)gXSignMask);
		//r[3] = _mm_xor_ps(vec, *(const __m128*)gXYZSignMask);

		l[0] = _mm_xor_ps(r[0], *(const __m128*)gWSignMask);
		l[1] = _mm_xor_ps(r[1], *(const __m128*)gWSignMask);
		l[2] = _mm_xor_ps(r[2], *(const __m128*)gWSignMask);
		l[3] = vec;
										
		Matrix44::multiply3x4ToDest(m, r, l);

		return m;
	}

	// an attempt at SIMD'izing the scaler form
	// fastest on P4
	inline Matrix44& Quat::toMatrixSIMD2(Matrix44& m) const
	{
		Vec4 x2(vec + vec);
		
		Matrix44 t;
		
		// this is transposed the obvious way to simplify the later calcs
		
		t[0] = Vec4::multiply(x2.broadcast(0), vec);
		t[1] = Vec4::multiply(x2.broadcast(1), vec);
		t[2] = Vec4::multiply(x2.broadcast(2), vec);
		//t[3] = Vec4::multiply(x2.broadcast(3), vec);

		__m128 a = _mm_xor_ps(_mm_shuffle_ps(t[1], t[1], SSE_SHUFFLE(1, 0, 3, 0)), *M128CAST(&gXSignMask));
		__m128 b = _mm_xor_ps(_mm_shuffle_ps(t[2], t[2], SSE_SHUFFLE(2, 3, 0, 0)), *M128CAST(&gZSignMask));
		
		m[0] = _mm_and_ps(_mm_add_ps(_mm_sub_ps(a, b), gVec4XOne), *M128CAST(&gInvWMask));
					
		__m128 c = _mm_xor_ps(_mm_shuffle_ps(t[2], t[2], SSE_SHUFFLE(3, 2, 1, 0)), *M128CAST(&gYSignMask));
		__m128 d = _mm_xor_ps(Vec4(_mm_shuffle_ps(t[0], t[0], SSE_SHUFFLE(0, 0, 3, 0))).setX(t[1]), *M128CAST(&gXSignMask)); 
		m[1] = _mm_and_ps(_mm_add_ps(_mm_sub_ps(c, d), gVec4YOne), *M128CAST(&gInvWMask));
		
		m[2] = m[0] % m[1];
		m[3] = gVec4WOne;

		return m;
	}
	
	// scaler form
	inline Matrix44& Quat::toMatrixScaler(Matrix44& m) const
	{
		const Vec4 s(vec * 2.0f);
		
		float wx, wy, wz, xx, xy, xz, yy, yz, zz;
		
		wx = vec[3] * s[0];	wy = vec[3] * s[1];	wz = vec[3] * s[2];
		xx = vec[0] * s[0];	xy = vec[0] * s[1];	xz = vec[0] * s[2];
		yy = vec[1] * s[1];	yz = vec[1] * s[2];	zz = vec[2] * s[2];

		m[0][0] = 1.0f - (yy + zz);
		m[0][1] = xy - wz;
		m[0][2] = xz + wy;

		m[1][0] = xy + wz;
		m[1][1] = 1.0f - (xx + zz);
		m[1][2] = yz - wx;

		m[2][0] = xz - wy;
		m[2][1] = yz + wx;
		m[2][2] = 1.0f - (xx + yy);

		m[0][3] = m[1][3] = m[2][3] = 0.0f;
		m[3] = gVec4WOne;
		
		return m;
	}

	// SSE scaler form
	inline Matrix44& Quat::toMatrixSIMDScaler(Matrix44& m) const
	{
		const Vec4 a(vec * 2.0f);
		const F32vec1 sx(a[0]);
		const F32vec1 sy(a[1]);
		const F32vec1 sz(a[2]);
		
		F32vec1 wx, wy, wz, xx, xy, xz, yy, yz, zz;
		
		wx = F32vec1(vec[3]) * sx;	wy = F32vec1(vec[3]) * sy;	wz = F32vec1(vec[3]) * sz;
		xx = F32vec1(vec[0]) * sx;	xy = F32vec1(vec[0]) * sy;	xz = F32vec1(vec[0]) * sz;
		yy = F32vec1(vec[1]) * sy;	yz = F32vec1(vec[1]) * sz;	zz = F32vec1(vec[2]) * sz;

		_mm_store_ss(&m[0][0], F32vec1(1.0f) - (yy + zz));
		_mm_store_ss(&m[0][1], xy - wz);
		_mm_store_ss(&m[0][2], xz + wy);

		_mm_store_ss(&m[1][0], xy + wz);
		_mm_store_ss(&m[1][1], F32vec1(1.0f) - (xx + zz));
		_mm_store_ss(&m[1][2], yz - wx);

		_mm_store_ss(&m[2][0], xz - wy);
		_mm_store_ss(&m[2][1], yz + wx);
		_mm_store_ss(&m[2][2], F32vec1(1.0f) - (xx + yy));

		m[0][3] = m[1][3] = m[2][3] = 0.0f;
		m[3] = gVec4WOne;

		return m;
	}

	// converts to homogeneous 3x3 matrix (element 3,3=1)
	inline Matrix44& Quat::toMatrix(Matrix44& m) const
	{
		return toMatrixSIMD2(m);
	}

	inline bool Quat::isPure(void) const
	{
		return vec.isVector();
	}

	inline bool Quat::isUnit(void) const
	{
		return Math::EqualTol(len(), 1.0f);
	}

	inline Vec4 Quat::rotateVec(const Vec4& v) const
	{
		return (unitInverse() * Quat(v.toVector()) * *this).vec;
	}

	inline Vec4 Quat::rotateVecTransposed(const Vec4& v) const
	{
		return (*this * Quat(v.toVector()) * unitInverse()).vec;
	}

	inline Quat Quat::lerp(const Quat& a, const Quat& b, float t)
	{
		return Vec4::lerp(a.vec, b.vec, t).normalize();
	}

	inline Quat Quat::lerpNoNormalize(const Quat& a, const Quat& b, float t)
	{
		return Vec4::lerp(a.vec, b.vec, t);
	}
	
	inline Quat Quat::makeIdentity(void) 
	{
		return I;
	}

	inline bool Quat::equalTol(const Quat& a, const Quat& b, float tol)
	{
		return Vec4::equalTol(a.vec, b.vec, tol);
	}

	inline Quat operator* (float s, const Quat& b)
	{
		return b * s;
	}

	inline Quat operator+ (const Quat& a, const Quat& b)
	{
		return Quat(a) += b;
	}

	inline Quat operator- (const Quat& a, const Quat& b)
	{
		return Quat(a) -= b;
	}

	inline Quat Quat::makeRotation(const Quat& from, const Quat& to)
	{
		return (from.unitInverse() * to).normalize();
	}

	inline Quat& Quat::setFromMatrix(const Matrix44& m)
	{
		return *this = makeFromMatrix(m);
	}

	inline Matrix44 Quat::createMatrix(const Quat& a)
	{
		Matrix44 ret;
		a.toMatrix(ret);
		return ret;
	}

	inline bool Quat::operator== (const Quat& b) const
	{
		return vec == b.vec;
	}
	
	inline bool Quat::operator!= (const Quat& b) const
	{
		return !(*this == b);
	}

} // namespace gr



