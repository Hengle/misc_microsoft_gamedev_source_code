//-----------------------------------------------------------------------------
// File: vector_x86.inl
// x86 optimized vector/matrix classes.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

namespace gr 
{
	#define SignPNPN (*(F32vec4*)gPNPN)
	#define SignNPNP (*(F32vec4*)gNPNP)
	
	#define _mm_neg_ps(v) _mm_xor_ps(gSignMask, v)
	#define _mm_abs_ps(v) _mm_andnot_ps(gSignMask, v)
	#define _mm_ror_ps(v, i) _mm_shuffle_ps(v, v, _MM_SHUFFLE((uint8)((i)+3) & 3, (uint8)((i)+2) & 3, (uint8)((i)+1) & 3, (uint8)((i)+0) & 3))
	#define _mm_rol_ps(v, i) _mm_shuffle_ps(v, v, _MM_SHUFFLE((uint8)(7-(i)) & 3, (uint8)(6-(i)) & 3, (uint8)(5-(i)) & 3, (uint8)(4-(i)) & 3))
	#define _mm_mad_ps(a, b, c) _mm_add_ps(_mm_mul_ps(a, b), c)

	// Component arrangement:
	//127               0
	//3333 2222 1111 0000
	//WWWW ZZZZ YYYY XXXX
	#define SSE_SHUFFLE(x, y, z, w) ((w)<<6) | ((z)<<4) | ((y)<<2) | (x)

	inline __m128 rcpNR(__m128 a)
	{
		const __m128 Ra0 = _mm_rcp_ss(a);
		return _mm_sub_ss(_mm_add_ss(Ra0, Ra0), _mm_mul_ss(_mm_mul_ss(Ra0, a), Ra0));
	}

	inline __m128 rsqrtNR(__m128 a)
	{
		//const __m128 Ra0 = _mm_rsqrt_ss(a);
		//return _mm_mul_ss(_mm_mul_ss(gVec4Point5, Ra0), _mm_sub_ss(gVec4Three, _mm_mul_ss(_mm_mul_ss(a, Ra0), Ra0)));
		return _mm_set_ss(1.0f / sqrt(a.m128_f32[0]));
	}

	inline Vec4::Vec4()
	{
	}

	inline Vec4::Vec4(float s)
	{
		vec = _mm_load_ps1(&s);
	}

	inline Vec4::Vec4(const __m128 v) : vec(v)
	{
	}

	inline Vec4::Vec4(const Vec4& b)
	{
		vec = b.vec;
	}

	inline Vec4& Vec4::operator= (float s)
	{
		vec = _mm_load_ps1(&s);
		return *this;
	}
	
	inline Vec4& Vec4::operator= (const Vec4& b)
	{
		vec = b.vec;
		return *this;
	}

	inline Vec4::operator __m128() const
	{
		return vec;
	}

	inline Vec4& Vec4::operator= (const __m128 v)
	{
		vec = v;
		return *this;
	}

	template<int size>
	inline Vec4::Vec4(const Vec<size>& b)
	{
		int n = Math::Min(4, size);
		for (int i = 0; i < n; i++)
			element[i] = b[i];
		for ( ; i < 4; i++)
			element[i] = 0.0f;
	}

	template<>
	inline Vec4::Vec4(const Vec<4>& b) 
	{
		vec = _mm_loadu_ps(reinterpret_cast<const float*>(&b));
	}
	
	inline Vec4::Vec4(const Vec<3>& b, float sw) : x(b[0]), y(b[1]), z(b[2]), w(sw)
	{
	}

	template<int size> Vec4& Vec4::operator= (const Vec<size>& b)
	{
		switch (b.numElements)
		{
			case 2:
				set(b[0], b[1], 0, 0);
				break;
			case 3:
				set(b[0], b[1], b[2], 0);
				break;
			case 4:
				vec = _mm_loadu_ps(reinterpret_cast<const float*>(&b));
				break;
			default:
				setZero();
				int n = Math::Min<int>(4, b.numElements);
				for (int i = 0; i < n; i++)
					element[i] = b.element[i];
				break;
		}
		return *this;
	}

	inline Vec4::Vec4(float sx, float sy, float sz) :
		x(sx), y(sy), z(sz), w(0)
	{
	}

	inline Vec4::Vec4(float sx, float sy, float sz, float sw) :
		x(sx), y(sy), z(sz), w(sw)
	{
		//vec = _mm_set_ps(sw, sz, sy, sx);
	}

	inline Vec4::Vec4(const Vec4& v, float sw)
	{
		vec = v;
		w = sw;
	}

	inline Vec4& Vec4::set(float sx, float sy, float sz, float sw)  
	{
		//vec = _mm_set_ps(sw, sz, sy, sx);
		x = sx;
		y = sy;
		z = sz;
		w = sw;
		return *this;
	}

	inline Vec4& Vec4::clearW(void)
	{
		vec = _mm_and_ps(*(__m128*)gInvWMask, vec);
		return *this;
	}

	inline Vec4& Vec4::setBroadcast(float s)
	{
		vec = _mm_set_ps1(s);
		return *this;
	}

	inline Vec4& Vec4::setW(float sw)
	{
		w = sw;
		return *this;
	}

	inline const float& Vec4::operator[] (int i) const
	{
		return element[DebugRange<int>(i, numElements)];
	}

	inline float& Vec4::operator[] (int i) 
	{
		return element[DebugRange<int>(i, numElements)];
	}

	// in place vec add
	inline Vec4& Vec4::operator+= (const Vec4& b)
	{
		vec = _mm_add_ps(vec, b.vec);
		return *this;
	}

	// in place vec sub
	inline Vec4& Vec4::operator-= (const Vec4& b)
	{
		vec = _mm_sub_ps(vec, b.vec);
		return *this;
	}

	// in place vec mul
	inline Vec4& Vec4::operator*= (const Vec4& b)
	{
		vec = _mm_mul_ps(vec, b.vec);
		return *this;
	}

	// in place scaler mul
	inline Vec4& Vec4::operator*= (float s)
	{
		vec = _mm_mul_ps(vec, _mm_set_ps1(s));
		return *this;
	}

	// in place scaler div
	inline Vec4& Vec4::operator/= (float s)
	{
		vec = _mm_div_ps(vec, _mm_set_ps1(s));
		return *this;
	}

	// positive
	inline Vec4 Vec4::operator+ () const
	{
		return *this;
	}

	// negative
	inline Vec4 Vec4::operator- () const
	{
		return Vec4(_mm_xor_ps(vec, *(__m128*)gSignMask));
	}

	// scale mul
	inline Vec4 Vec4::operator* (float s) const
	{
		return Vec4(_mm_mul_ps(vec, _mm_set_ps1(s)));
	}

	// scale mul
	inline Vec4 operator* (float s, const Vec4& a) 
	{
		return Vec4(_mm_mul_ps(a.vec, _mm_set_ps1(s)));
	}

	// scale div
	inline Vec4 Vec4::operator/ (float s) const
	{
		return Vec4(_mm_div_ps(vec, _mm_set_ps1(s)));
	}

	inline Vec4 operator+ (const Vec4& a, const Vec4& b) 
	{
		return Vec4(_mm_add_ps(a.vec, b.vec));
	}

	inline Vec4 operator- (const Vec4& a, const Vec4& b) 
	{
		return Vec4(_mm_sub_ps(a.vec, b.vec));
	}

	inline float operator* (const Vec4& a, const Vec4& b) 
	{
		return a.dot(b);
	}

	inline Vec4 operator% (const Vec4& a, const Vec4& b)
	{
		return a.cross(b);
	}

	inline Vec4 operator/ (const Vec4& a, const Vec4& b) 
	{
		return Vec4(_mm_div_ps(a.vec, b.vec));
	}

	inline float Vec4::horizontalAdd(void) const
	{
		__m128 temp = _mm_add_ps(vec, _mm_movehl_ps(vec, vec));
		temp = _mm_add_ss(_mm_shuffle_ps(temp, temp, 1), temp);
		float ret;
		_mm_store_ss(&ret, temp);
		return ret;
	}

	inline Vec4 Vec4::horizontalAddSingle(void) const
	{
		__m128 temp = _mm_add_ps(vec, _mm_movehl_ps(vec, vec));
		return _mm_add_ss(_mm_shuffle_ps(temp, temp, 1), temp);
	}

	inline Vec4 Vec4::horizontalAddBroadcast(void) const
	{
		__m128 temp = _mm_add_ps(vec, _mm_movehl_ps(vec, vec));
		temp = _mm_add_ss(_mm_shuffle_ps(temp, temp, 1), temp);
		return _mm_shuffle_ps(temp, temp, SSE_SHUFFLE(0, 0, 0, 0));
	}

	inline float Vec4::dot(const Vec4& b) const
	{
		__m128 temp = _mm_mul_ps(vec, b.vec);
		temp = _mm_add_ps(temp, _mm_movehl_ps(temp, temp));
		temp = _mm_add_ss(_mm_shuffle_ps(temp, temp, 1), temp);
		float ret;
		_mm_store_ss(&ret, temp);
		return ret;
	}

	inline float Vec4::dot3(const Vec4& b) const
	{
		__m128 temp = _mm_mul_ps(_mm_and_ps(vec, *(__m128*)gInvWMask), b.vec);
		temp = _mm_add_ps(temp, _mm_movehl_ps(temp, temp));
		temp = _mm_add_ss(_mm_shuffle_ps(temp, temp, 1), temp);
		float ret;
		_mm_store_ss(&ret, temp);
		return ret;
	}

	inline Vec4 Vec4::cross(const Vec4& b) const
	{
		const __m128 temp1 = _mm_shuffle_ps(  vec,   vec, SSE_SHUFFLE(1, 2, 0, 3));
		const __m128 temp2 = _mm_shuffle_ps(b.vec, b.vec, SSE_SHUFFLE(2, 0, 1, 3));
		const __m128 temp3 = _mm_shuffle_ps(  vec,   vec, SSE_SHUFFLE(2, 0, 1, 3));
		const __m128 temp4 = _mm_shuffle_ps(b.vec, b.vec, SSE_SHUFFLE(1, 2, 0, 3));
		return Vec4(_mm_sub_ps(_mm_mul_ps(temp1, temp2), _mm_mul_ps(temp3, temp4)));
	}

	inline float Vec4::norm(void) const
	{
		return dot(*this);
	}

	inline float Vec4::len(void) const
	{
		return sqrt(norm());
	}

	inline float Vec4::oneOverLen(void) const
	{
		__m128 temp = _mm_mul_ps(vec, vec);
		temp = _mm_add_ps(_mm_movehl_ps(temp, temp), temp);
		temp = _mm_add_ss(_mm_shuffle_ps(temp, temp, 1), temp);
		temp = _mm_and_ps(_mm_cmpneq_ss(temp, _mm_setzero_ps()), rsqrtNR(temp));
		float ret;
		_mm_store_ss(&ret, temp);
		return ret;
	}

	inline Vec4& Vec4::normalize(void)
	{
		__m128 temp = _mm_mul_ps(vec, vec);
		temp = _mm_add_ps(_mm_movehl_ps(temp, temp), temp);
		temp = _mm_add_ss(_mm_shuffle_ps(temp, temp, 1), temp);
		temp = _mm_and_ps(_mm_cmpneq_ss(temp, _mm_setzero_ps()), rsqrtNR(temp));
		vec = _mm_mul_ps(vec, _mm_shuffle_ps(temp, temp, 0));
		Assert(isUnit());
		return *this;
	}

	inline Vec4& Vec4::normalize3(void)
	{
		float l = sqrt(x * x + y * y + z * z);
		if (l > 0)
			*this *= 1.0f / l;
		return *this;
	}

	// returns 0 or 1/sqrt(norm())
	inline float Vec4::tryNormalize(void)
	{
		__m128 temp = _mm_mul_ps(vec, vec);
		temp = _mm_add_ps(_mm_movehl_ps(temp, temp), temp);
		temp = _mm_add_ss(_mm_shuffle_ps(temp, temp, 1), temp);
		temp = _mm_and_ps(_mm_cmpneq_ss(temp, _mm_setzero_ps()), rsqrtNR(temp));
		vec = _mm_mul_ps(vec, _mm_shuffle_ps(temp, temp, 0));
		Assert(isUnit());
		float ret;
		_mm_store_ss(&ret, temp);
		return ret;
	}

	inline Vec4 Vec4::normalized(void) const
	{
		Vec4 ret(*this);
		return ret.normalize();
	}

	inline Vec4& Vec4::setZero(void)
	{
		vec = _mm_setzero_ps();
		return *this;
	}

	inline Vec4 Vec4::broadcast(const int e) const
	{
		DebugRange<int>(e, numElements);
		// Compiler can optimize this switch out when optimizing.
		switch (e)
		{
			case 0: return _mm_shuffle_ps(vec, vec, SSE_SHUFFLE(0,0,0,0));
			case 1: return _mm_shuffle_ps(vec, vec, SSE_SHUFFLE(1,1,1,1));
			case 2: return _mm_shuffle_ps(vec, vec, SSE_SHUFFLE(2,2,2,2));
			case 3: return _mm_shuffle_ps(vec, vec, SSE_SHUFFLE(3,3,3,3));
			default:
				__assume(0);
		}
	}

	inline bool Vec4::isVector(void) const
	{
		return w == 0.0f;
	}

	inline bool Vec4::isPoint(void) const
	{
		return w == 1.0f;
	}

	inline bool Vec4::isUnit(void) const
	{
		return Math::EqualTol(len(), 1.0f);
	}

	inline Vec4& Vec4::project(void) 
	{
		if (0.0f != w)
			*this /= w;
		return *this;
	}

	inline Vec4 Vec4::removeCompUnit(const Vec4& v, const Vec4& dir)
	{
		Assert(dir.isUnit());
		return v - (dir * (v * dir));
	}

	inline Vec4 Vec4::makeAxisVector(int axis, float s)
	{
		Vec4 ret;
		ret = _mm_setzero_ps();
		ret[DebugRange<int>(axis, numElements)] = s;
		return ret;
	}

	inline Vec4& Vec4::clampComponents(float low, float high)
	{
		vec = max(Vec4(low), min(Vec4(high), *this));
		return *this;
	}

	inline float Vec4::minComponent(void) const
	{
		return Math::Min4(x, y, z, w);
	}

	inline float Vec4::maxComponent(void) const
	{
		return Math::Max4(x, y, z, w);
	}

	inline Vec4 Vec4::multiply(const Vec4& a, const Vec4& b)
	{
		return _mm_mul_ps(a.vec, b.vec);
	}

	inline int Vec4::minorAxis(void) const
	{
		int i = 0; 
		float f = fabs(x);
		if (fabs(y) < f) { i = 1; f = fabs(y); }
		if (fabs(z) < f) { i = 2; f = fabs(z); }
		if (fabs(w) < f) i = 3; 
		return i;
	}

	inline int Vec4::majorAxis(void) const
	{
		int i = 0; 
		float f = fabs(x);
		if (fabs(y) > f) { i = 1; f = fabs(y); }
		if (fabs(z) > f) { i = 2; f = fabs(z); }
		if (fabs(w) > f) i = 3; 
		return i;
	}

	inline void Vec4::projectionAxes(int& uAxis, int& vAxis) const
	{
		const int axis = majorAxis();
		if (element[axis] < 0.0f)
		{
			vAxis = Math::NextWrap(axis, 3);
			uAxis = Math::NextWrap(vAxis, 3);
		}
		else
		{
			uAxis = Math::NextWrap(axis, 3);
			vAxis = Math::NextWrap(uAxis, 3);
		}
	}

	inline Vec4 Vec4::min(const Vec4& a, const Vec4& b)
	{
		return _mm_min_ps(a, b);
	}

	inline Vec4 Vec4::max(const Vec4& a, const Vec4& b)
	{
		return _mm_max_ps(a, b);
	}

	inline Vec4 Vec4::rangeCompressed(void) const
	{
		return *this * .5f + Vec4(.5f);
	}

	inline Vec4 Vec4::rangeExpanded(void) const
	{
		return (*this - Vec4(.5f)) * 2.0f;
	}

	// returns (x,y,z,0)
	inline Vec4 Vec4::toVector(void) const
	{
		return _mm_and_ps(vec, *reinterpret_cast<const __m128*>(gInvWMask));
	}
		
	// returns (x,y,z,1)
	inline Vec4 Vec4::toPoint(void) const
	{
		return _mm_or_ps(_mm_and_ps(vec, *reinterpret_cast<const __m128*>(gInvWMask)), gVec4WOne);
	}

	inline Vec4 Vec4::negateXYZ(void) const
	{
		return _mm_xor_ps(vec, *reinterpret_cast<const __m128*>(gXYZSignMask));
	}

	inline Vec4 Vec4::negateW(void) const
	{
		return _mm_xor_ps(vec, *reinterpret_cast<const __m128*>(gWSignMask));
	}

	inline bool Vec4::equalTol(const Vec4& a, const Vec4& b, float tol)
	{
		return 
			Math::EqualTol(a.x, b.x, tol) && 
			Math::EqualTol(a.y, b.y, tol) &&
			Math::EqualTol(a.z, b.z, tol) &&
			Math::EqualTol(a.w, b.w, tol);
	}
	
	inline bool Vec4::equalTol3(const Vec4& a, const Vec4& b, float tol)
	{
		return 
			Math::EqualTol(a.x, b.x, tol) && 
			Math::EqualTol(a.y, b.y, tol) &&
			Math::EqualTol(a.z, b.z, tol);
	}

	inline Vec4 Vec4::lerp(const Vec4& a, const Vec4& b, float t)
	{
		return a + (b - a) * t;
	}

	inline Vec4& Vec4::setX(const Vec4& a)
	{
		vec = _mm_move_ss(vec, a);
		return *this;
	}

	inline Vec4& Vec4::setY(const Vec4& a)
	{
		//vec = _mm_move_ss(_mm_shuffle_ps(vec, vec, SSE_SHUFFLE(1,2,3,0)), _mm_shuffle_ps(a, a, SSE_SHUFFLE(1, 1, 1, 1)));
		//vec = _mm_shuffle_ps(vec, vec, SSE_SHUFFLE(3,0,1,2));
		vec = _mm_or_ps(_mm_andnot_ps(*(const __m128*)gYMask, vec), _mm_and_ps(*(const __m128*)gYMask, a));
		return *this;
	}

	inline Vec4& Vec4::setZ(const Vec4& a)
	{
		//vec = _mm_move_ss(_mm_shuffle_ps(vec, vec, SSE_SHUFFLE(2,3,0,1)), _mm_shuffle_ps(a, a, SSE_SHUFFLE(2, 2, 2, 2)));
		//vec = _mm_shuffle_ps(vec, vec, SSE_SHUFFLE(2,3,0,1));
		vec = _mm_or_ps(_mm_andnot_ps(*(const __m128*)gZMask, vec), _mm_and_ps(*(const __m128*)gZMask, a));
		return *this;
	}

	inline Vec4& Vec4::setW(const Vec4& a)
	{
		//vec = _mm_move_ss(_mm_shuffle_ps(vec, vec, SSE_SHUFFLE(3,0,1,2)), _mm_shuffle_ps(a, a, SSE_SHUFFLE(3, 3, 3, 3)));
		//vec = _mm_shuffle_ps(vec, vec, SSE_SHUFFLE(1,2,3,0));
		vec = _mm_or_ps(_mm_andnot_ps(*(const __m128*)gWMask, vec), _mm_and_ps(*(const __m128*)gWMask, a));
		return *this;
	}
	
	inline Vec4& Vec4::setXY(const Vec4& a)
	{
		vec = _mm_movelh_ps(a, _mm_movehl_ps(vec, vec));
		return *this;
	}

	inline Vec4& Vec4::setZW(const Vec4& a)
	{
		vec = _mm_movelh_ps(vec, _mm_movehl_ps(a, a));
		return *this;
	}

	inline Vec4 Vec4::makeCartesian(const Vec4& v)
	{
		const float yaw = v[0];
		const float pitch = v[1];
		const float mag = v[2];
		return Vec4(
			sin(yaw)*cos(pitch)*mag,
			sin(pitch)*mag,
			cos(yaw)*cos(pitch)*mag);
	}

	inline Vec4 Vec4::makeSpherical(const Vec4& v)
	{
		//asin(z)=atan2(z,sqrt(x*x+y*y))
		return Vec4(atan2(v.x, v.z), asin(v.y),	v.len());
	}

	inline bool Vec4::operator== (const Vec4& b) const
	{
		return (x == b.x) && (y == b.y) && (z == b.z) && (w == b.w);
	}

	inline bool Vec4::operator!= (const Vec4& b) const
	{
		return !(*this == b);
	}

	// lexicographical less 
	inline bool Vec4::operator<(const Vec4& b) const
	{
		for (int i = 0; i < numElements; i++)
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
		
	//-----------------------------------------------------------------

	inline Matrix44::Matrix44() 
	{ 
	}

	inline Matrix44::Matrix44(
		float e00, float e01, float e02, float e03, 
		float e10, float e11, float e12, float e13, 
		float e20, float e21, float e22, float e23, 
		float e30, float e31, float e32, float e33)
	{
		setElements(
			e00, e01, e02, e03, 
			e10, e11, e12, e13, 
			e20, e21, e22, e23, 
			e30, e31, e32, e33);
	}

	inline Matrix44& Matrix44::setElements(
		float e00, float e01, float e02, float e03, 
		float e10, float e11, float e12, float e13, 
		float e20, float e21, float e22, float e23, 
		float e30, float e31, float e32, float e33)
	{
		row[0][0] = e00; row[0][1] = e01;	row[0][2] = e02; row[0][3] = e03;
		row[1][0] = e10; row[1][1] = e11;	row[1][2] = e12; row[1][3] = e13;
		row[2][0] = e20; row[2][1] = e21;	row[2][2] = e22; row[2][3] = e23;
		row[3][0] = e30; row[3][1] = e31;	row[3][2] = e32; row[3][3] = e33;
		return *this;
	}

	inline Matrix44::Matrix44(const Vec4& row0, const Vec4& row1, const Vec4& row2, const Vec4& row3) 
	{
		row[0] = row0;
		row[1] = row1;
		row[2] = row2;
		row[3] = row3;
	}

	inline Matrix44::Matrix44(const Matrix44& b)
	{
		row[0].vec = b.row[0].vec;
		row[1].vec = b.row[1].vec;
		row[2].vec = b.row[2].vec;
		row[3].vec = b.row[3].vec;
	}

	inline Matrix44& Matrix44::operator= (const Matrix44& b)
	{
		row[0].vec = b.row[0].vec;
		row[1].vec = b.row[1].vec;
		row[2].vec = b.row[2].vec;
		row[3].vec = b.row[3].vec;
		return *this;
	}

	inline const float& Matrix44::operator() (int r, int c) const
	{
		return row[DebugRange(r, 4)][DebugRange(c, 4)];
	}

	inline float& Matrix44::operator() (int r, int c)
	{
		return row[DebugRange(r, 4)][DebugRange(c, 4)];
	}

	inline const Vec4& Matrix44::operator[] (int r) const
	{
		return row[DebugRange(r, 4)];
	}

	inline Vec4& Matrix44::operator[] (int r) 
	{
		return row[DebugRange(r, 4)];
	}

	inline const Vec4& Matrix44::getRow(int r) const
	{
		DebugRange(r, 4);
		return row[r];
	}
	
	inline Matrix44& Matrix44::setRow(int r, const Vec4& v)
	{
		DebugRange(r, 4);
		row[r] = v;
		return *this;
	}

	inline Vec4 Matrix44::getColumn(int c) const
	{
		DebugRange(c, 4);
		return Vec4(row[0][c], row[1][c], row[2][c], row[3][c]);
	}

	inline Matrix44& Matrix44::setColumn(int c, const Vec4& v)
	{
		DebugRange(c, 4);
		row[0][c] = v.x;
		row[1][c] = v.y;
		row[2][c] = v.z;
		row[3][c] = v.w;
		return *this;
	}

	inline Matrix44& Matrix44::operator+= (const Matrix44& b)
	{
		row[0] += b.row[0];	
		row[1] += b.row[1];	
		row[2] += b.row[2];	
		row[3] += b.row[3];
		return *this;
	}

	inline Matrix44& Matrix44::operator-= (const Matrix44& b)
	{
		row[0] -= b.row[0];	
		row[1] -= b.row[1];	
		row[2] -= b.row[2];	
		row[3] -= b.row[3];
		return *this;
	}

	#pragma optimize( "a", on )
	inline Matrix44& Matrix44::multiplyToDest(Matrix44& dest, const Matrix44& a, const Matrix44& b)
	{
		Assert(&dest != &a);
		Assert(&dest != &b);
		dest.row[0] = _mm_mad_ps(b.row[0], a[0].broadcast(0), _mm_mad_ps(b.row[1], a[0].broadcast(1), _mm_mad_ps(b.row[2], a[0].broadcast(2), _mm_mul_ps(b.row[3], a[0].broadcast(3)))));
		dest.row[1] = _mm_mad_ps(b.row[0], a[1].broadcast(0), _mm_mad_ps(b.row[1], a[1].broadcast(1), _mm_mad_ps(b.row[2], a[1].broadcast(2), _mm_mul_ps(b.row[3], a[1].broadcast(3)))));
		dest.row[2] = _mm_mad_ps(b.row[0], a[2].broadcast(0), _mm_mad_ps(b.row[1], a[2].broadcast(1), _mm_mad_ps(b.row[2], a[2].broadcast(2), _mm_mul_ps(b.row[3], a[2].broadcast(3)))));
		dest.row[3] = _mm_mad_ps(b.row[0], a[3].broadcast(0), _mm_mad_ps(b.row[1], a[3].broadcast(1), _mm_mad_ps(b.row[2], a[3].broadcast(2), _mm_mul_ps(b.row[3], a[3].broadcast(3)))));
		return dest;
	}	
	// a[3] isn't needed
	inline Matrix44& Matrix44::multiply3x4ToDest(Matrix44& dest, const Matrix44& a, const Matrix44& b)
	{
		Assert(&dest != &a);
		Assert(&dest != &b);
		dest.row[0] = _mm_mad_ps(b.row[0], a[0].broadcast(0), _mm_mad_ps(b.row[1], a[0].broadcast(1), _mm_mad_ps(b.row[2], a[0].broadcast(2), _mm_mul_ps(b.row[3], a[0].broadcast(3)))));
		dest.row[1] = _mm_mad_ps(b.row[0], a[1].broadcast(0), _mm_mad_ps(b.row[1], a[1].broadcast(1), _mm_mad_ps(b.row[2], a[1].broadcast(2), _mm_mul_ps(b.row[3], a[1].broadcast(3)))));
		dest.row[2] = _mm_mad_ps(b.row[0], a[2].broadcast(0), _mm_mad_ps(b.row[1], a[2].broadcast(1), _mm_mad_ps(b.row[2], a[2].broadcast(2), _mm_mul_ps(b.row[3], a[2].broadcast(3)))));
		dest.row[3] = gVec4WOne;
		return dest;
	}	
	inline Matrix44 Matrix44::multiply(const Matrix44& a, const Matrix44& b)
	{
		Matrix44 dest;
		dest.row[0] = _mm_mad_ps(b.row[0], a[0].broadcast(0), _mm_mad_ps(b.row[1], a[0].broadcast(1), _mm_mad_ps(b.row[2], a[0].broadcast(2), _mm_mul_ps(b.row[3], a[0].broadcast(3)))));
		dest.row[1] = _mm_mad_ps(b.row[0], a[1].broadcast(0), _mm_mad_ps(b.row[1], a[1].broadcast(1), _mm_mad_ps(b.row[2], a[1].broadcast(2), _mm_mul_ps(b.row[3], a[1].broadcast(3)))));
		dest.row[2] = _mm_mad_ps(b.row[0], a[2].broadcast(0), _mm_mad_ps(b.row[1], a[2].broadcast(1), _mm_mad_ps(b.row[2], a[2].broadcast(2), _mm_mul_ps(b.row[3], a[2].broadcast(3)))));
		dest.row[3] = _mm_mad_ps(b.row[0], a[3].broadcast(0), _mm_mad_ps(b.row[1], a[3].broadcast(1), _mm_mad_ps(b.row[2], a[3].broadcast(2), _mm_mul_ps(b.row[3], a[3].broadcast(3)))));
		return dest;
	}	
	#pragma optimize( "", on )

	inline Matrix44& Matrix44::operator*= (const Matrix44& b)
	{
		multiplyToDest(*this, Matrix44(*this), b);
		return *this;
	}

	// (row vector) 1x4 * 4x4 = 1x4
	inline Vec4 Matrix44::transform(const Vec4& a, const Matrix44& b)
	{
		return _mm_mad_ps(a.broadcast(0), b.row[0], _mm_mad_ps(a.broadcast(1), b.row[1], _mm_mad_ps(a.broadcast(2), b.row[2], _mm_mul_ps(a.broadcast(3), b.row[3]))));
	}
	
	// effectively transposed b before xforming
	inline Vec4 Matrix44::transformTransposed(const Matrix44& b, const Vec4& a)
	{
		return a * b.transposed();
	}

	// assumes w = 0
	inline Vec4 Matrix44::transformNormal(const Vec4& a, const Matrix44& b)
	{
		return _mm_mad_ps(a.broadcast(0), b.row[0], _mm_mad_ps(a.broadcast(1), b.row[1], _mm_mul_ps(a.broadcast(2), b.row[2])));
	}

	// assumes w = 1
	inline Vec4 Matrix44::transformPoint(const Vec4& a, const Matrix44& b)
	{
		return _mm_mad_ps(a.broadcast(0), b.row[0], _mm_mad_ps(a.broadcast(1), b.row[1], _mm_mad_ps(a.broadcast(2), b.row[2], b.row[3])));
	}

	// assumes w = 0
	inline Vec4 Matrix44::transformNormalTransposed(const Matrix44& b, const Vec4& a)
	{
		return transformNormal(a, b.transposed());
	}

	// assumes w = 1
	inline Vec4 Matrix44::transformPointTransposed(const Matrix44& b, const Vec4& a)
	{
		return transformPoint(a, b.transposed());
	}

	inline Matrix44& Matrix44::operator*= (float s)
	{
		const __m128 v = _mm_set_ps1(s);
		row[0] *= v;
		row[1] *= v;
		row[2] *= v;
		row[3] *= v;
		return *this;
	}
	
	inline Matrix44 operator+ (const Matrix44& a, const Matrix44& b)
	{
		Matrix44 ret;
		ret[0] = a[0] + b[0];
		ret[1] = a[1] + b[1];
		ret[2] = a[2] + b[2];
		ret[3] = a[3] + b[3];
		return ret;
	}

	inline Matrix44 operator- (const Matrix44& a, const Matrix44& b)
	{
		Matrix44 ret;
		ret[0] = a[0] - b[0];
		ret[1] = a[1] - b[1];
		ret[2] = a[2] - b[2];
		ret[3] = a[3] - b[3];
		return ret;
	}

	inline Matrix44 operator* (const Matrix44& a, const Matrix44& b)
	{
		return Matrix44::multiply(a, b);
	}

	inline Vec4 operator* (const Vec4& a, const Matrix44& b)
	{
		return Matrix44::transform(a, b);
	}

	inline Matrix44 operator* (const Matrix44& a, float s)
	{
		const __m128 v = _mm_set_ps1(s);
		Matrix44 ret;
		ret[0] = _mm_mul_ps(a[0], v);
		ret[1] = _mm_mul_ps(a[1], v);
		ret[2] = _mm_mul_ps(a[2], v);
		ret[3] = _mm_mul_ps(a[3], v);
		return ret;
	}

	inline Matrix44 operator* (float s, const Matrix44& a)
	{
		const __m128 v = _mm_set_ps1(s);
		Matrix44 ret;
		ret[0] = _mm_mul_ps(a[0], v);
		ret[1] = _mm_mul_ps(a[1], v);
		ret[2] = _mm_mul_ps(a[2], v);
		ret[3] = _mm_mul_ps(a[3], v);
		return ret;
	}

	inline Matrix44 Matrix44::operator+ () const
	{
		return *this;
	}

	inline Matrix44 Matrix44::operator- () const
	{
		Matrix44 ret;
		ret[0] = -row[0];
		ret[1] = -row[1];
		ret[2] = -row[2];
		ret[3] = -row[3];
		return ret;
	}

	inline Matrix44 operator/ (const Matrix44& a, float s)
	{
		Matrix44 ret;
		ret[0] = a[0] / s;
		ret[1] = a[1] / s;
		ret[2] = a[2] / s;
		ret[3] = a[3] / s;
		return ret;
	}

	inline Matrix44& Matrix44::setZero(void)
	{
		Vec4 vec;
		vec.setZero();
		Assert(vec[0] == 0.0f);
		row[0] = vec;
		row[1] = vec;
		row[2] = vec;
		row[3] = vec;
		return *this;
	}

	inline Matrix44& Matrix44::setIdentity(void)
	{
		setZero();
		row[0][0] = 1.0f;
		row[1][1] = 1.0f;
		row[2][2] = 1.0f;
		row[3][3] = 1.0f;
		return *this;
	}

	// in-place transpose
	inline Matrix44& Matrix44::transpose(void)
	{
		_MM_TRANSPOSE4_PS(row[0].vec, row[1].vec, row[2].vec, row[3].vec);
		return *this;
	}

	// in-place upper 3x3 transpose
	inline Matrix44& Matrix44::transpose3x3(void)
	{
		Matrix44 temp(*this);
		
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				(*this)[i][j] = temp[j][i];

		return *this;
	}

	// returns transpose of matrix
	inline Matrix44 Matrix44::transposed(void) const
	{
		Matrix44 ret(*this);
		_MM_TRANSPOSE4_PS(ret.row[0].vec, ret.row[1].vec, ret.row[2].vec, ret.row[3].vec);
		return ret;
	}
	

	// in-place invert
	inline float Matrix44::invert(void)
	{
		return invertCramersRuleSSE(reinterpret_cast<float*>(row), reinterpret_cast<const float*>(row));
	}
	
	// returns inverse of matrix
	inline Matrix44 Matrix44::inverse(void) const
	{
		Matrix44 ret;
		invertCramersRuleSSE(reinterpret_cast<float*>(&ret), reinterpret_cast<const float*>(row));
		return ret;
	}

	inline const Vec4& Matrix44::getTranslate(void) const
	{
		return row[3];
	}

	inline Matrix44& Matrix44::setTranslate(const Vec4& t)
	{
		row[3] = t;
		return *this;
	}

	inline Matrix44& Matrix44::setTranslate(const Vec4& t, float w)
	{
		row[3] = t;
		row[3][3] = w;
		return *this;
	}

	inline Matrix44& Matrix44::setTranslate(const Vec<3>& t)
	{
		row[3][0] = t[0];
		row[3][1] = t[1];
		row[3][2] = t[2];
		return *this;
	}

	inline Matrix44 Matrix44::makeScale(const Vec4& s)
	{
		Matrix44 ret;
		ret.setZero();
		ret[0][0] = s.x;
		ret[1][1] = s.y;
		ret[2][2] = s.z;
		ret[3][3] = s.w;
		return ret;
	}

	inline Matrix44 Matrix44::makeIdentity(void)
	{
		Matrix44 ret;
		ret.setIdentity();
		return ret;
	}

	inline Matrix44 Matrix44::makeZero(void)
	{
		Matrix44 ret;
		ret.setZero();
		return ret;
	}

	inline Matrix44 Matrix44::makeTranslate(const Vec4& t)
	{
		Matrix44 ret;
		ret.setIdentity();
		ret.setTranslate(t);
		return ret;
	}

	inline Matrix44 Matrix44::makeRightToLeft(void)
	{
		Matrix44 ret(makeIdentity());
		ret[2][2] = -1.0f;
		return ret;
	}

	inline Matrix44 Matrix44::makeTensorProduct(const Vec4& v, const Vec4& w)
	{
		Matrix44 ret;
		ret[0] = _mm_mul_ps(v.broadcast(0), w);
		ret[1] = _mm_mul_ps(v.broadcast(1), w);
		ret[2] = _mm_mul_ps(v.broadcast(2), w);
		ret[3] = _mm_mul_ps(v.broadcast(3), w);
		return ret;
	}

	inline Matrix44 Matrix44::makeCrossProduct(const Vec4& w)
	{
		Matrix44 ret;
		ret[0] = Vec4(0.0f,  w.z, -w.y, 0.0f);
    ret[1] = Vec4(-w.z,    0,  w.x, 0.0f);
    ret[2] = Vec4( w.y, -w.x, 0.0f, 0.0f);
    ret[3] = Vec4(0.0f);
		return ret;
	}

	inline Matrix44 Matrix44::makeReflection(const Vec4& n, const Vec4& q)
	{
		Matrix44 ret;
		Assert(n.isVector());
		Assert(q.isVector());
		ret = makeIdentity() - 2.0f * makeTensorProduct(n, n);
		ret.setTranslate(2.0f * (q * n) * n, 1.0f);
		return ret;
	}

	inline Matrix44 Matrix44::makeUniformScaling(const Vec4& q, float c)
	{
		Matrix44 ret;
		Assert(q.isVector());
		ret = c * makeIdentity();
		ret.setTranslate((1.0f - c) * q, 1.0f);
		return ret;
	}

	inline Matrix44 Matrix44::makeNonuniformScaling(const Vec4& q, float c, const Vec4& w)
	{
		Matrix44 ret;
		Assert(q.isVector());
		Assert(w.isVector());
		ret = makeIdentity() - (1.0f - c) * makeTensorProduct(w, w);
		ret.setTranslate((1.0f - c) * (q * w) * w, 1.0f);
		return ret;
	}

	// n = normal of plane, q = point on plane
	inline Matrix44 Matrix44::makeOrthoProjection(const Vec4& n, const Vec4& q)
	{
		Matrix44 ret;
		Assert(n.isVector());
		Assert(q.isVector());
		ret = makeIdentity() - makeTensorProduct(n, n);
		ret.setTranslate((q * n) * n, 1.0f);
		return ret;
	}
  
	inline Matrix44 Matrix44::makeParallelProjection(const Vec4& n, const Vec4& q, const Vec4& w)
	{
		Matrix44 ret;
		Assert(n.isVector());
		Assert(q.isVector());
		Assert(w.isVector());
		ret = makeIdentity() - (makeTensorProduct(n, w) / (w * n));
		ret.setTranslate(((q * n) / (w * n)) * w, 1.0f);
		return ret;
	}

	inline bool Matrix44::equalTol(const Matrix44& a, const Matrix44& b, float tol)
	{
		return 
			Vec4::equalTol(a[0], b[0], tol) && 
			Vec4::equalTol(a[1], b[1], tol) && 
			Vec4::equalTol(a[2], b[2], tol) && 
			Vec4::equalTol(a[3], b[3], tol);
	}

	inline bool Matrix44::equalTol3x3(const Matrix44& a, const Matrix44& b, float tol)
	{
		return 
			Vec4::equalTol3(a[0], b[0], tol) && 
			Vec4::equalTol3(a[1], b[1], tol) && 
			Vec4::equalTol3(a[2], b[2], tol);
	}
	
} // namespace gr

