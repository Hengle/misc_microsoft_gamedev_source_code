/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_MATH_PSEUDORANDOMGENERATOR_H
#define HK_MATH_PSEUDORANDOMGENERATOR_H

#include <Common/Base/hkBase.h>

// This is a Pseudorandom Number generator from  Num. Recip. p284:  Knuth-Lewis "quick and dirty" rand,
// otherwise known as randq1(). It's not great, but it's fast. Don't use it for "serious" work.
class hkPseudoRandomGenerator
{
public:

	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_MATH, hkPseudoRandomGenerator);

		// Seed MUST be passed in on construction (to force you to think about it!)
	hkPseudoRandomGenerator(int s)
	{
		m_seed = hkUint32(s);
		m_current = m_seed;
	}

		// Can also reseed if desired. It wipes over  the last used, so effectively "restarts" the RNG
	void setSeed(int s)
	{
		m_seed = hkUint32(s);
		m_current = m_seed;
	}

		// Get seed used by generator. This may be useful to determine how the RNG was "started".
	int getSeed()
	{
		return int(m_seed);
	}

		// Get current value (NOT equal to getRand32), but will allow you to "monitor" the RNG so you
		// can reseed it later. 
	int getCurrent()
	{
		return int(m_current);
	}

		// Get random number as unsigned int
	hkUint32 getRand32()
	{
		m_current = 1664525U * m_current + 1013904223U;		
		return m_current;
	}

			// Get random integer in range [0-X] as unsigned int, suitable for small values
	hkUint32 getRandChar(int x)
	{
		HK_ASSERT(0x777504aa,  x > 0 && x <= 256 );
		m_current = 1664525L * m_current + 1013904223L;	
		
		hkUint32 temp = (hkUint32)m_current;
		temp >>= 13;
		temp = temp % x;

		HK_ASSERT(0x3443045a, temp < 256 );
		return temp;
	}

		// Get random real in range [0,1] using getRand32()
	hkReal getRandReal01()
	{	
		const hkReal v = getRand32()*( 1.0f / 0xffffffff);
		return v;
	}

		// Get random real in range [-1,1] using getRand32()
	hkReal getRandReal11()
	{	
		return 2*getRandReal01() - 1.0f;
	}

		// Get random real in range [min,max] using getRand01()
	hkReal getRandRange( hkReal min, hkReal max )
	{
		return getRandReal01() * ( max - min ) + min;
	}

	void getRandomRotation( hkRotation& rotOut )
	{
		hkVector4 v;
		v.set( getRandReal11(), getRandReal11(), getRandReal11(), getRandReal11() );
		v.normalize4();
		hkQuaternion q;
		q.m_vec = v;
		rotOut.set( q );
	}

	void getRandomRotation( hkQuaternion& rotOut )
	{
		hkVector4 v;
		v.set( getRandReal11(), getRandReal11(), getRandReal11(), getRandReal11() );
		v.normalize4();
		rotOut.m_vec = v;
	}

		/// sets xyzw randomly between -1 and 1
	void getRandomVector11( hkVector4& vecOut )
	{
		vecOut.set( getRandReal11(), getRandReal11(), getRandReal11(), getRandReal11() );
	}

		/// sets xyzw randomly between 0 and 1
	void getRandomVector01( hkVector4& vecOut )
	{
		vecOut.set( getRandReal01(), getRandReal01(), getRandReal01(), getRandReal01() );
	}


private:
	hkUint32 m_seed;
	hkUint32 m_current;

};

#endif // HK_MATH_PSEUDORANDOMGENERATOR_H

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
