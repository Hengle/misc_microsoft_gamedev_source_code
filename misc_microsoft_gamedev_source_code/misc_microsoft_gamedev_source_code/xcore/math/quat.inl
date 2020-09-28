//-----------------------------------------------------------------------------
// File: quat.inl
// Copyright (c) 2005-2006, Ensemble Studios
// rg [1/6/05] - Ported from xgrlib, SSE code removed, needs to be retested!
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// BQuat::BQuat
//-----------------------------------------------------------------------------
inline BQuat::BQuat()
{
}

//-----------------------------------------------------------------------------
// BQuat::BQuat
//-----------------------------------------------------------------------------
// directly sets vector/scalar components
inline BQuat::BQuat(const BVec4& v) : 
	mVec(v)
{
}

//-----------------------------------------------------------------------------
// BQuat::BQuat
//-----------------------------------------------------------------------------
// directly sets vector and scalar components
inline BQuat::BQuat(const BVec4& v, float w) : mVec(v, w)
{
}

//-----------------------------------------------------------------------------
// BQuat::BQuat
//-----------------------------------------------------------------------------
inline BQuat::BQuat(float x, float y, float z, float w) :
	mVec(x, y, z, w)
{
}

//-----------------------------------------------------------------------------
// BQuat::BQuat
//-----------------------------------------------------------------------------
inline BQuat::BQuat(const BMatrix44& m)
{
	setFromMatrix(m);
}

//-----------------------------------------------------------------------------
// BQuat::operator=
//-----------------------------------------------------------------------------
inline BQuat& BQuat::operator= (const BVec4& v)
{
	mVec[0] = v[0];
	mVec[1] = v[1];
	mVec[2] = v[2];
	mVec[3] = v[3];
	return *this;
}

//-----------------------------------------------------------------------------
// BQuat::operator=
//-----------------------------------------------------------------------------
inline BQuat& BQuat::operator= (const BQuat& b)
{
	mVec = b.mVec;
	return *this;
}

//-----------------------------------------------------------------------------
// BQuat::operator[]
//-----------------------------------------------------------------------------
inline const float BQuat::operator[] (int element) const
{
	return mVec[element];
}

//-----------------------------------------------------------------------------
// BQuat::operator[]
//-----------------------------------------------------------------------------
inline float& BQuat::operator[] (int element) 
{
	return mVec[element];
}

//-----------------------------------------------------------------------------
// BQuat::setFromAxisAngle
//-----------------------------------------------------------------------------
inline BQuat& BQuat::setFromAxisAngle(const BVec4& axis, float angle)
{
	BDEBUG_ASSERT(axis.isVector());
	if (axis.norm() < Math::fSmallEpsilon)
		setIdentity();
	else			
	{
		float s, c;
		Math::fSinCos(angle * .5f, s, c);

		mVec = (axis.normalized() * s).setW(c);
	}
      	
	return *this;
}

//-----------------------------------------------------------------------------
// BQuat::setIdentity
//-----------------------------------------------------------------------------
inline BQuat& BQuat::setIdentity(void)
{
	mVec.set(0.0f, 0.0f, 0.0f, 1.0f);
	return *this;
}

//-----------------------------------------------------------------------------
// BQuat::imaginary
//-----------------------------------------------------------------------------
inline BVec4 BQuat::imaginary(void) const
{
	return mVec.toVector();
}

//-----------------------------------------------------------------------------
// BQuat::scalar
//-----------------------------------------------------------------------------
inline float BQuat::scalar(void) const
{
	return mVec[3];
}

//-----------------------------------------------------------------------------
// BQuat::scalarBroadcast
// broadcasts w
//-----------------------------------------------------------------------------
inline BVec4 BQuat::scalarBroadcast(void) const
{
	return mVec.broadcast(3);
}

//-----------------------------------------------------------------------------
// BQuat::axis
// rotation axis
//-----------------------------------------------------------------------------
inline BVec4 BQuat::axis(void) const
{
	BDEBUG_ASSERT(isUnit());
	BVec4 ret(imaginary());
	if (ret.tryNormalize() == 0.0f)
		ret = BVec4(1, 0, 0, 0);
   return ret;
}

//-----------------------------------------------------------------------------
// BQuat::angle
// rotation angle
//-----------------------------------------------------------------------------
inline float BQuat::angle(void) const
{
	BDEBUG_ASSERT(isUnit());
	return acos(Math::Clamp(mVec[3], -1.0f, 1.0f)) * 2.0f;
}

//-----------------------------------------------------------------------------
// BQuat::norm
// squared length
//-----------------------------------------------------------------------------
inline float BQuat::norm(void) const
{
	return mVec.norm();
}

//-----------------------------------------------------------------------------
// BQuat::len
// length
//-----------------------------------------------------------------------------
inline float BQuat::len(void) const
{
	return sqrt(norm());
}

//-----------------------------------------------------------------------------
// BQuat::normalize
// in-place normalization
//-----------------------------------------------------------------------------
inline BQuat BQuat::normalize(void) 
{
	mVec.normalize();
	return *this;
}

//-----------------------------------------------------------------------------
// BQuat::tryNormalize
// returns 0 or 1/sqrt(norm())
//-----------------------------------------------------------------------------
inline float BQuat::tryNormalize(void)
{
	return mVec.tryNormalize();
}

//-----------------------------------------------------------------------------
// BQuat::normalized
// returns normalized vector
//-----------------------------------------------------------------------------
inline BQuat BQuat::normalized(void) const
{
	return BQuat(*this).normalize();
}

//-----------------------------------------------------------------------------
// BQuat::dot
// dot product 4
//-----------------------------------------------------------------------------
inline float BQuat::dot(const BQuat& b) const
{
	return mVec * b.mVec;
}

//-----------------------------------------------------------------------------
// BQuat::inverse
//-----------------------------------------------------------------------------
inline BQuat BQuat::inverse(void) const
{
	float n = norm();
	if (n == 0.0)
		return BQuat(0, 0, 0, 0);
	return BQuat(mVec.negateXYZ() / n);
}

//-----------------------------------------------------------------------------
// BQuat::unitInverse
//-----------------------------------------------------------------------------
inline BQuat BQuat::unitInverse(void) const
{
	BDEBUG_ASSERT(isUnit());
	return BQuat(mVec.negateXYZ());
}

//-----------------------------------------------------------------------------
// BQuat::conjugate
//-----------------------------------------------------------------------------
inline BQuat BQuat::conjugate(void) const
{
	return BQuat(mVec.negateXYZ());
}

//-----------------------------------------------------------------------------
// BQuat::multiplySIMD
//-----------------------------------------------------------------------------
inline BQuat BQuat::multiplySIMD(const BQuat& a, const BQuat& b) 
{
	return BQuat(
		BVec4(a.mVec % b.mVec) + BVec4::multiply(a.scalarBroadcast(), b.mVec) + BVec4::multiply(b.scalarBroadcast(), a.mVec),
		a.mVec.negateXYZ() * b.mVec
	);
}

//-----------------------------------------------------------------------------
// BQuat::multiplyScalar
//-----------------------------------------------------------------------------
inline BQuat BQuat::multiplyScalar(const BQuat& a, const BQuat& b) 
{
	return BQuat(
			a[1] * b[2]	- a[2] * b[1]	+ a[3] * b[0]	+ a[0] * b[3],
			a[2] * b[0]	- a[0] * b[2]	+ a[3] * b[1]	+ a[1] * b[3],
			a[0] * b[1]	- a[1] * b[0]	+ a[3] * b[2]	+ a[2] * b[3],
			a[3] * b[3]	- a[0] * b[0]	- a[1] * b[1]	- a[2] * b[2]
		);
}

//-----------------------------------------------------------------------------
// BQuat::multiply
// concatenates a and b
//-----------------------------------------------------------------------------
inline BQuat BQuat::multiply(const BQuat& a, const BQuat& b) 
{
	return multiplyScalar(a, b);
}

//-----------------------------------------------------------------------------
// operator*
// concatenates a and b
//-----------------------------------------------------------------------------
inline BQuat operator* (const BQuat& a, const BQuat& b) 
{
	return BQuat::multiply(a, b);
}
		
//-----------------------------------------------------------------------------
// operator*
//-----------------------------------------------------------------------------
inline BQuat operator* (const BQuat& a, float s) 
{
	return BQuat(a.mVec * s);
}

//-----------------------------------------------------------------------------
// operator/
//-----------------------------------------------------------------------------
inline BQuat operator/ (const BQuat& a, float s)
{
	return BQuat(a.mVec / s);
}

//-----------------------------------------------------------------------------
// BQuat::operator-
//-----------------------------------------------------------------------------
inline BQuat BQuat::operator- () const
{
	return BQuat(-mVec);
}

//-----------------------------------------------------------------------------
// BQuat::operator+
//-----------------------------------------------------------------------------
inline BQuat BQuat::operator+ () const
{
	return BQuat(mVec);
}

//-----------------------------------------------------------------------------
// BQuat::operator +=
//-----------------------------------------------------------------------------
inline BQuat& BQuat::operator += (const BQuat& b) 
{
	mVec += b.mVec;
	return *this;
}

//-----------------------------------------------------------------------------
// BQuat::operator -=
//-----------------------------------------------------------------------------
inline BQuat& BQuat::operator -= (const BQuat& b) 
{
	mVec -= b.mVec;
	return *this;
}

#if 0
//-----------------------------------------------------------------------------
// BQuat::toMatrixSIMD1
// SIMD form
//-----------------------------------------------------------------------------
inline BMatrix44& BQuat::toMatrixSIMD1(BMatrix44& m) const
{
	BMatrix44 l, r;

	r[0] = _mm_xor_ps(_mm_shuffle_ps(mVec, mVec, SSE_SHUFFLE(3, 2, 1, 0)), *(const __m128*)gYSignMask);
	r[1] = _mm_xor_ps(_mm_shuffle_ps(mVec, mVec, SSE_SHUFFLE(2, 3, 0, 1)), *(const __m128*)gZSignMask);
	r[2] = _mm_xor_ps(_mm_shuffle_ps(mVec, mVec, SSE_SHUFFLE(1, 0, 3, 2)), *(const __m128*)gXSignMask);
	//r[3] = _mm_xor_ps(mVec, *(const __m128*)gXYZSignMask);

	l[0] = _mm_xor_ps(r[0], *(const __m128*)gWSignMask);
	l[1] = _mm_xor_ps(r[1], *(const __m128*)gWSignMask);
	l[2] = _mm_xor_ps(r[2], *(const __m128*)gWSignMask);
	l[3] = mVec;
									
	BMatrix44::multiply3x4ToDest(m, r, l);

	return m;
}

//-----------------------------------------------------------------------------
// BQuat::toMatrixSIMD2
// an attempt at SIMD'izing the scalar form
// fastest on P4
//-----------------------------------------------------------------------------
inline BMatrix44& BQuat::toMatrixSIMD2(BMatrix44& m) const
{
	BVec4 x2(mVec + mVec);
	
	BMatrix44 t;
	
	// this is transposed the obvious way to simplify the later calcs
	
	t[0] = BVec4::multiply(x2.broadcast(0), mVec);
	t[1] = BVec4::multiply(x2.broadcast(1), mVec);
	t[2] = BVec4::multiply(x2.broadcast(2), mVec);
	//t[3] = BVec4::multiply(x2.broadcast(3), mVec);

	__m128 a = _mm_xor_ps(_mm_shuffle_ps(t[1], t[1], SSE_SHUFFLE(1, 0, 3, 0)), M128_CAST(gXSignMask));
	__m128 b = _mm_xor_ps(_mm_shuffle_ps(t[2], t[2], SSE_SHUFFLE(2, 3, 0, 0)), M128_CAST(gZSignMask));
	
	m[0] = _mm_and_ps(_mm_add_ps(_mm_sub_ps(a, b), gVec4XOne), M128_CAST(gInvWMask));
				
	__m128 c = _mm_xor_ps(_mm_shuffle_ps(t[2], t[2], SSE_SHUFFLE(3, 2, 1, 0)), M128_CAST(gYSignMask));
	__m128 d = _mm_xor_ps(BVec4(_mm_shuffle_ps(t[0], t[0], SSE_SHUFFLE(0, 0, 3, 0))).setX(t[1]), M128_CAST(gXSignMask)); 
	m[1] = _mm_and_ps(_mm_add_ps(_mm_sub_ps(c, d), gVec4YOne), M128_CAST(gInvWMask));
	
	m[2] = m[0] % m[1];
	m[3] = BVec4(0.0f, 0.0f, 0.0f, 1.0f);

	return m;
}
#endif

//-----------------------------------------------------------------------------
// BQuat::toMatrix
// scalar form
//-----------------------------------------------------------------------------
inline BMatrix44& BQuat::toMatrix(BMatrix44& m) const
{
	const BVec4 s(mVec * 2.0f);
	
	float wx, wy, wz, xx, xy, xz, yy, yz, zz;
	
	wx = mVec[3] * s[0];	wy = mVec[3] * s[1];	wz = mVec[3] * s[2];
	xx = mVec[0] * s[0];	xy = mVec[0] * s[1];	xz = mVec[0] * s[2];
	yy = mVec[1] * s[1];	yz = mVec[1] * s[2];	zz = mVec[2] * s[2];

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
	m[3] = BVec4(0.0f, 0.0f, 0.0f, 1.0f);
	
	return m;
}

#if 0
//-----------------------------------------------------------------------------
// BQuat::toMatrixSIMDScalar
// SSE scalar form
//-----------------------------------------------------------------------------
inline BMatrix44& BQuat::toMatrixSIMDScalar(BMatrix44& m) const
{
	const BVec4 a(mVec * 2.0f);
	const F32vec1 sx(a[0]);
	const F32vec1 sy(a[1]);
	const F32vec1 sz(a[2]);
	
	F32vec1 wx, wy, wz, xx, xy, xz, yy, yz, zz;
	
	wx = F32vec1(mVec[3]) * sx;	wy = F32vec1(mVec[3]) * sy;	wz = F32vec1(mVec[3]) * sz;
	xx = F32vec1(mVec[0]) * sx;	xy = F32vec1(mVec[0]) * sy;	xz = F32vec1(mVec[0]) * sz;
	yy = F32vec1(mVec[1]) * sy;	yz = F32vec1(mVec[1]) * sz;	zz = F32vec1(mVec[2]) * sz;

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
	m[3] = BVec4(0.0f, 0.0f, 0.0f, 1.0f);

	return m;
}
#endif

//-----------------------------------------------------------------------------
// BQuat::isPure
//-----------------------------------------------------------------------------
inline bool BQuat::isPure(void) const
{
	return mVec.isVector();
}

//-----------------------------------------------------------------------------
// BQuat::isUnit
//-----------------------------------------------------------------------------
inline bool BQuat::isUnit(void) const
{
	return Math::EqualTol(len(), 1.0f);
}

//-----------------------------------------------------------------------------
// BQuat::rotateVec
//-----------------------------------------------------------------------------
inline BVec4 BQuat::rotateVec(const BVec4& v) const
{
	return (unitInverse() * BQuat(v.toVector()) * *this).mVec;
}

//-----------------------------------------------------------------------------
// BQuat::rotateVecTransposed
//-----------------------------------------------------------------------------
inline BVec4 BQuat::rotateVecTransposed(const BVec4& v) const
{
	return (*this * BQuat(v.toVector()) * unitInverse()).mVec;
}

//-----------------------------------------------------------------------------
// BQuat::lerp
//-----------------------------------------------------------------------------
inline BQuat BQuat::lerp(const BQuat& a, const BQuat& b, float t)
{
	return BVec4::lerp(a.mVec, b.mVec, t).normalize();
}

//-----------------------------------------------------------------------------
// BQuat::lerpNoNormalize
//-----------------------------------------------------------------------------
inline BQuat BQuat::lerpNoNormalize(const BQuat& a, const BQuat& b, float t)
{
	return BVec4::lerp(a.mVec, b.mVec, t);
}

//-----------------------------------------------------------------------------
// BQuat::makeIdentity
//-----------------------------------------------------------------------------
inline BQuat BQuat::makeIdentity(void) 
{
	return I;
}

//-----------------------------------------------------------------------------
// BQuat::equalTol
//-----------------------------------------------------------------------------
inline bool BQuat::equalTol(const BQuat& a, const BQuat& b, float tol)
{
	return BVec4::equalTol(a.mVec, b.mVec, tol);
}

//-----------------------------------------------------------------------------
// operator*
//-----------------------------------------------------------------------------
inline BQuat operator* (float s, const BQuat& b)
{
	return b * s;
}

//-----------------------------------------------------------------------------
// operator+
//-----------------------------------------------------------------------------
inline BQuat operator+ (const BQuat& a, const BQuat& b)
{
	return BQuat(a) += b;
}

//-----------------------------------------------------------------------------
// operator-
//-----------------------------------------------------------------------------
inline BQuat operator- (const BQuat& a, const BQuat& b)
{
	return BQuat(a) -= b;
}

//-----------------------------------------------------------------------------
// BQuat::makeRotation
//-----------------------------------------------------------------------------
inline BQuat BQuat::makeRotation(const BQuat& from, const BQuat& to)
{
	return (from.unitInverse() * to).normalize();
}

//-----------------------------------------------------------------------------
// BQuat::setFromMatrix
//-----------------------------------------------------------------------------
inline BQuat& BQuat::setFromMatrix(const BMatrix44& m)
{
	return *this = makeFromMatrix(m);
}

//-----------------------------------------------------------------------------
// BQuat::createMatrix
//-----------------------------------------------------------------------------
inline BMatrix44 BQuat::createMatrix(const BQuat& a)
{
	BMatrix44 ret;
	a.toMatrix(ret);
	return ret;
}

//-----------------------------------------------------------------------------
// BQuat::operator==
//-----------------------------------------------------------------------------
inline bool BQuat::operator== (const BQuat& b) const
{
	return mVec == b.mVec;
}

//-----------------------------------------------------------------------------
// BQuat::operator!=
//-----------------------------------------------------------------------------
inline bool BQuat::operator!= (const BQuat& b) const
{
	return !(*this == b);
}
