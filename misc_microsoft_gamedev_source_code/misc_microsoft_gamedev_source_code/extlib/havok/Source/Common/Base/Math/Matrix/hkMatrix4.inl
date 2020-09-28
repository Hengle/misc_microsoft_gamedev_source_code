/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

inline hkVector4& hkMatrix4::getColumn(int x)
{
	return (&m_col0)[x];
}

inline const hkVector4& hkMatrix4::getColumn(int x) const
{
	return (&m_col0)[x];
}

inline hkReal& hkMatrix4::operator() (int r, int c)
{
	return getColumn(c)(r);
}

inline const hkReal& hkMatrix4::operator() (int r, int c) const
{
	return getColumn(c)(r);
}


inline void hkMatrix4::setRows( const hkVector4& r0,
								const hkVector4& r1,
								const hkVector4& r2,
								const hkVector4& r3)
{
	getColumn(0).set( r0(0), r1(0), r2(0), r3(0) );
	getColumn(1).set( r0(1), r1(1), r2(1), r3(1) );
	getColumn(2).set( r0(2), r1(2), r2(2), r3(2) );
	getColumn(3).set( r0(3), r1(3), r2(3), r3(3) );
}


inline void hkMatrix4::getRows( hkVector4& r0,
								hkVector4& r1,
								hkVector4& r2,
								hkVector4& r3) const
{
	r0.set( getColumn(0)(0), getColumn(1)(0), getColumn(2)(0), getColumn(3)(0) );
	r1.set( getColumn(0)(1), getColumn(1)(1), getColumn(2)(1), getColumn(3)(1) );
	r2.set( getColumn(0)(2), getColumn(1)(2), getColumn(2)(2), getColumn(3)(2) );
	r3.set( getColumn(0)(3), getColumn(1)(3), getColumn(2)(3), getColumn(3)(3) );
}

inline void hkMatrix4::setCols( const hkVector4& r0,
								const hkVector4& r1,
								const hkVector4& r2,
								const hkVector4& r3)
{
	m_col0 = r0;
	m_col1 = r1;
	m_col2 = r2;
	m_col3 = r3;
}

inline void hkMatrix4::operator= ( const hkMatrix4& a )
{
	m_col0 = a.getColumn(0);
	m_col1 = a.getColumn(1);
	m_col2 = a.getColumn(2);
	m_col3 = a.getColumn(3);
}

inline void hkMatrix4::getCols( hkVector4& r0,
								hkVector4& r1,
								hkVector4& r2,
								hkVector4& r3) const
{
	r0 = getColumn(0);
	r1 = getColumn(1);
	r2 = getColumn(2);
	r3 = getColumn(3);
}

inline void hkMatrix4::getRow( int row, hkVector4& r) const
{
	r.set( getColumn(0)(row), getColumn(1)(row), getColumn(2)(row), getColumn(3)(row) );
}

inline void hkMatrix4::setZero()
{
	getColumn(0).setZero4();
	getColumn(1).setZero4();
	getColumn(2).setZero4();
	getColumn(3).setZero4();
}

inline void hkMatrix4::setDiagonal( hkReal m00, hkReal m11, hkReal m22, hkReal m33 )
{
	setZero();
	(*this)(0,0) = m00;
	(*this)(1,1) = m11;
	(*this)(2,2) = m22;
	(*this)(3,3) = m33;
}

inline void hkMatrix4::setIdentity()
{
	setZero();
	hkReal one = 1.0f;
	(*this)(0,0) = one;
	(*this)(1,1) = one;
	(*this)(2,2) = one;
	(*this)(3,3) = one;
}

inline const hkMatrix4& HK_CALL hkMatrix4::getIdentity()
{
	extern hkReal hkMatrix4Identity[];
	return reinterpret_cast<const hkMatrix4&>(hkMatrix4Identity);
}

/*
* Havok SDK - PUBLIC RELEASE, BUILD(#20070919)
*
* Confidential Information of Havok.  (C) Copyright 1999-2007 
* Telekinesys Research Limited t/a Havok. All Rights Reserved. The Havok
* Logo, and the Havok buzzsaw logo are trademarks of Havok.  Title, ownership
* rights, and intellectual property rights in the Havok software remain in
* Havok and/or its suppliers.
*
* Use of this software for evaluation purposes is subject to and indicates 
* acceptance of the End User licence Agreement for this product. A copy of 
* the license is included with this software and is also available from salesteam@havok.com.
*
*/
