/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

void hkQsTransform::setIdentity()
{
	m_translation.setZero4();
	m_rotation.setIdentity();
	m_scale.setAll(1.0f);
}

void hkQsTransform::setZero()
{
	m_translation.setZero4();
	m_rotation.m_vec.setZero4();
	m_scale.setZero4();
}

const hkVector4& hkQsTransform::getTranslation() const
{
	return m_translation;
}

void hkQsTransform::setTranslation(const hkVector4& t)
{
	m_translation = t;
}

const hkQuaternion& hkQsTransform::getRotation() const
{
	return m_rotation;
}

void hkQsTransform::setRotation(const hkRotation& rotation)
{	
	m_rotation.set(rotation);
}

void hkQsTransform::setRotation(const hkQuaternion& rotation)
{
	m_rotation = rotation;
}

const hkVector4& hkQsTransform::getScale() const
{
	return m_scale;
}

void hkQsTransform::setScale(const hkVector4& s)
{
	m_scale = s;
}

hkQsTransform::hkQsTransform(const hkQsTransform& t)
{
	m_translation = t.m_translation;
	m_rotation = t.m_rotation;
	m_scale = t.m_scale;
}

hkQsTransform::hkQsTransform( IdentityInitializer /*init*/ )
{
	setIdentity();
}

hkQsTransform::hkQsTransform( ZeroInitializer /*init*/ )
{
	setZero();
}

hkQsTransform::hkQsTransform(const hkVector4& translation, const hkQuaternion& rotation, const hkVector4& scale)
	:	m_translation(translation), m_rotation(rotation), m_scale(scale)
{
}

hkQsTransform::hkQsTransform(const hkVector4& translation, const hkQuaternion& rotation)
:	m_translation(translation), m_rotation(rotation)
{
	m_scale.setAll(1.0f);
}


void hkQsTransform::set(const hkVector4& translation, const hkQuaternion& rotation, const hkVector4& scale)
{
	m_translation = translation;
	m_rotation = rotation;
	m_scale = scale;
}

const hkQsTransform& HK_CALL hkQsTransform::getIdentity()
{
	extern hkReal hkQsTransformIdentity[];
	return reinterpret_cast<const hkQsTransform&>(hkQsTransformIdentity);
}


void hkQsTransform::setInterpolate4( const hkQsTransform& a, const hkQsTransform& b, hkSimdRealParameter t)
{
	m_scale.setInterpolate4(a.m_scale, b.m_scale, t);
	m_translation.setInterpolate4(a.m_translation, b.m_translation, t);

	const hkReal oneMinusT = 1.0f - hkReal(t);

	// Check quaternion polarity
	const hkReal signedT = ( a.m_rotation.m_vec.dot4(b.m_rotation.m_vec) < 0.0f ) ? -t : t;
	
	hkQuaternion out;
	out.m_vec.setMul4( HK_SIMD_REAL(signedT), b.m_rotation.m_vec);
	out.m_vec.addMul4( HK_SIMD_REAL(oneMinusT), a.m_rotation.m_vec);
	out.normalize();

	m_rotation = out;
}

/*
** Blending stuff
*/

void hkQsTransform::blendAddMul(const hkQsTransform& other, hkSimdRealParameter weight)
{
	m_translation.addMul4(weight, other.m_translation);
	m_scale.addMul4(weight, other.m_scale);

	// Check quaternion polarity
#if defined(HK_PLATFORM_XBOX360) || ( defined(HK_PLATFORM_PS3) && !defined(HK_PLATFORM_SPU) )
	hkVector4 negRotation; negRotation.setNeg4(other.m_rotation.m_vec);
	hkVector4 dotVec; dotVec.getQuad() = m_rotation.m_vec.dot4(other.m_rotation.m_vec).getQuad();
	hkVector4Comparison mask = dotVec.compareLessThanZero4();
	hkVector4 polarityRotation; polarityRotation.select32( other.m_rotation.m_vec, negRotation, mask );
	m_rotation.m_vec.addMul4(weight, polarityRotation);
#else
	const hkSimdReal signedWeight = ( m_rotation.m_vec.dot4(other.m_rotation.m_vec) < 0.0f )? -weight : weight;
	m_rotation.m_vec.addMul4( signedWeight, other.m_rotation.m_vec );
#endif
}


void hkQsTransform::blendNormalize( hkSimdRealParameter totalWeight )
{
	// If weight is almost zero, blend is identity
	if (hkMath::fabs(totalWeight) < HK_REAL_EPSILON )
	{
		setIdentity();
		return;
	}

	// Weight all accumulators by inverse 
	{
		const hkSimdReal invWeight = HK_SIMD_REAL(1.0f) / totalWeight;

		m_translation.mul4(invWeight);
	    
		m_scale.mul4(invWeight);
		
	}
	
	// Now, check for the special cases

	// Rotation
	{
		const hkReal quatNorm = m_rotation.m_vec.lengthSquared4();
		if (quatNorm < HK_REAL_EPSILON)
		{
			// no rotations blended (or cancelled each other) -> set to identity
			m_rotation.setIdentity();
		}
		else
		{
			// normalize
			m_rotation.normalize();
		}
	}

	// Scale
	{
		const hkReal scaleNorm = m_scale.lengthSquared3();
		if (scaleNorm < HK_REAL_EPSILON)
		{
			// no scale blended (or scale cancelled each other) -> set to identity
			m_scale.setAll(1.0f);
		}
	}

}

void hkQsTransform::fastRenormalize( hkSimdRealParameter totalWeight )
{
	const hkSimdReal invWeight = HK_SIMD_REAL(1.0f) / totalWeight;

	m_translation.mul4(invWeight);
	
	m_scale.mul4(invWeight);
	
	m_rotation.normalize();
}

// aTc = aTb * bTc
void hkQsTransform::setMul( const hkQsTransform& aTb, const hkQsTransform& bTc )
{
	// Rotation and position go together
	hkVector4 extraTrans;	
	extraTrans.setRotatedDir(aTb.m_rotation, bTc.m_translation);
	m_translation.setAdd4(aTb.m_translation, extraTrans);
	m_rotation.setMul(aTb.m_rotation, bTc.m_rotation);

	// Scale goes apart
	m_scale.setMul4(aTb.m_scale, bTc.m_scale);
}

void hkQsTransform::setMulInverseMul( const hkQsTransform& a, const hkQsTransform &b )
{
	hkQsTransform h;
	h.setInverse( a );
	setMul(h, b);
}

void hkQsTransform::setMulMulInverse( const hkQsTransform &a, const hkQsTransform &b )
{
	hkQsTransform h;
	h.setInverse( b );
	setMul( a, h);	
}

void hkQsTransform::setMulEq( const hkQsTransform& b )
{
	// We are not "reference-safe" because quaternion multiplication is not reference-safe
	HK_ASSERT2(0x1acceb8d, (&b!=this) , "Using unsafe references in math operation");

	// Rotation and position go together
	hkVector4 extraTrans;
	extraTrans.setRotatedDir(m_rotation, b.m_translation);
	m_translation.add4(extraTrans);
	m_rotation.mul(b.m_rotation);

	// Scale goes apart
	m_scale.mul4(b.m_scale);

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
