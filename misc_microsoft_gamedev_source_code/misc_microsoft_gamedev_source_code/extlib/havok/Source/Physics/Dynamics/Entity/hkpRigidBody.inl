/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

inline hkpRigidBody* hkGetRigidBody( const hkpCollidable* collidable )
{
#if !defined(HK_PLATFORM_SPU)
	if ( collidable->getType() == hkpWorldObject::BROAD_PHASE_ENTITY )
	{
		return static_cast<hkpRigidBody*>( hkGetWorldObject(collidable) );
	}
	return HK_NULL;
#else
	return static_cast<hkpRigidBody*>( hkGetWorldObject(collidable) );
#endif
}

inline hkpMotion* hkpRigidBody::getRigidMotion() const
{
	return const_cast<hkpMaxSizeMotion*>(&m_motion);
}

// Get the mass of the rigid body.
inline hkReal hkpRigidBody::getMass() const
{
	return getRigidMotion()->getMass();
}

inline hkReal hkpRigidBody::getMassInv() const
{
	return getRigidMotion()->getMassInv();
}


// Get the inertia tensor in local space
inline void hkpRigidBody::getInertiaLocal(hkMatrix3& inertia) const
{
	getRigidMotion()->getInertiaLocal(inertia);
}

// Get the inertia tensor in world space
inline void hkpRigidBody::getInertiaWorld(hkMatrix3& inertia) const
{
	getRigidMotion()->getInertiaWorld(inertia);
}


// Get the inverse inertia tensor in local space
void hkpRigidBody::getInertiaInvLocal(hkMatrix3& inertiaInv) const 
{
	getRigidMotion()->getInertiaInvLocal(inertiaInv);
}

// Get the inverse inertia tensor in World space
void hkpRigidBody::getInertiaInvWorld(hkMatrix3& inertiaInv) const 
{
	getRigidMotion()->getInertiaInvWorld(inertiaInv);
}

// Explicit center of mass in local space
const hkVector4& hkpRigidBody::getCenterOfMassLocal() const
{
	return getRigidMotion()->getCenterOfMassLocal();
}

		
const hkVector4& hkpRigidBody::getCenterOfMassInWorld() const
{
	return getRigidMotion()->getCenterOfMassInWorld();
}

/*
** UTILITIY
*/



// Return the position (Local Space origin) for this rigid body, in World space.
// Note: The center of mass is no longer necessarily the local space origin
inline const hkVector4& hkpRigidBody::getPosition() const
{
	return getRigidMotion()->getPosition();
}

// Returns the rotation from Local to World space for this rigid body.
inline const hkQuaternion& hkpRigidBody::getRotation() const
{
	return getRigidMotion()->getRotation();
}

// Returns the rigid body (local) to world transformation.
inline const hkTransform& hkpRigidBody::getTransform() const
{
	return getRigidMotion()->getTransform();
}

// Used by the graphics engine.
// It only uses a simple lerp quaternion interpolation while 
// hkSweptTransformUtil::lerp2() used for collision detection
// uses the 'dobule-linear' interpolation 
void hkpRigidBody::approxTransformAt( hkTime time, hkTransform& transformOut ) const
{
	getRigidMotion()->approxTransformAt( time, transformOut );
}

void hkpRigidBody::approxCurrentTransform( hkTransform& transformOut ) const
{
	HK_ASSERT2(0x3dd87047, m_world, "This function requires the body to be in an hkpWorld, in order to retrieve the current time.");
	getRigidMotion()->approxTransformAt( m_world->getCurrentTime(), transformOut );
}

/*
** VELOCITY ACCESS
*/

// Return the linear velocity of the center of mass of the rigid body, in World space.
inline const hkVector4& hkpRigidBody::getLinearVelocity() const
{
	return getRigidMotion()->getLinearVelocity();
}


// Returns the angular velocity around the center of mass, in World space.
inline const hkVector4& hkpRigidBody::getAngularVelocity() const
{
	return getRigidMotion()->getAngularVelocity();
}


// Velocity of point p on the rigid body, in World space.
void hkpRigidBody::getPointVelocity(const hkVector4& p, hkVector4& vecOut) const
{
	HK_ASSERT2(0x49da7b54, p.isOk3(), "Point passed to hkpRigidBody::getPointVelocity is invalid.");
	getRigidMotion()->getPointVelocity(p, vecOut);
}


/*
** IMPULSE APPLICATION
*/

// Apply an impulse to the center of mass.
inline void hkpRigidBody::applyLinearImpulse(const hkVector4& imp)
{
	HK_ASSERT2(0x1f476204, imp.isOk3(), "Impulse passed to hkpRigidBody::applyLinearImpulse is invalid.");
	activate();
	getRigidMotion()->applyLinearImpulse(imp);
}


// Apply an impulse at the point p in world space.
inline void hkpRigidBody::applyPointImpulse(const hkVector4& imp, const hkVector4& p)
{
	HK_ASSERT2(0x69b4b47f, imp.isOk3(), "Impulse passed to hkpRigidBody::applyLinearImpulse is invalid.");
	HK_ASSERT2(0x39557915, p.isOk3(), "Point passed to hkpRigidBody::applyPointImpulse is invalid.");
	activate();
	getRigidMotion()->applyPointImpulse(imp, p);
}


// Apply an instantaneous change in angular velocity around center of mass.
inline void hkpRigidBody::applyAngularImpulse(const hkVector4& imp)
{
	HK_ASSERT2(0x7659db28, imp.isOk3(), "Impulse passed to hkpRigidBody::applyAngularImpulse is invalid.");
	activate();
	getRigidMotion()->applyAngularImpulse(imp);
}


/*
** FORCE AND TORQUE APPLICATION
*/

// Applies a force to the rigid body. The force is applied to the center of mass.
inline void hkpRigidBody::applyForce(const hkReal deltaTime, const hkVector4& force)
{
	HK_ASSERT2(0x5caf01a5, force.isOk3(), "Force passed to hkpRigidBody::applyForce is invalid.");
	activate();
	getRigidMotion()->applyForce(deltaTime, force);
}

// Applies a force to the rigid body at the point p in world space.
inline void hkpRigidBody::applyForce(const hkReal deltaTime, const hkVector4& force, const hkVector4& p)
{
	HK_ASSERT2(0x5817ffd7, force.isOk3(), "Force passed to hkpRigidBody::applyForce is invalid.");
	HK_ASSERT2(0x6f65e274, p.isOk3(), "Point passed to hkpRigidBody::applyForce is invalid.");
	activate();
	getRigidMotion()->applyForce(deltaTime, force, p);
}


// Applies the specified torque to the rigid body.
inline void hkpRigidBody::applyTorque(const hkReal deltaTime, const hkVector4& torque)
{
	HK_ASSERT2(0x32bce075, torque.isOk3(), "Torque passed to hkpRigidBody::applyTorque is invalid.");
	activate();
	getRigidMotion()->applyTorque(deltaTime, torque);
}



/*
** DAMPING
*/

// Naive momentum damping
inline hkReal hkpRigidBody::getLinearDamping() const
{
	return getRigidMotion()->getLinearDamping();
}

inline hkReal hkpRigidBody::getAngularDamping() const
{
	return getRigidMotion()->getAngularDamping();
}



inline void hkpRigidBody::setLinearDamping( hkReal d )
{
	getRigidMotion()->setLinearDamping( d );
}



inline void hkpRigidBody::setAngularDamping( hkReal d )
{
	getRigidMotion()->setAngularDamping( d );
}


inline hkReal hkpRigidBody::getMaxLinearVelocity() const
{
	return getRigidMotion()->getMotionState()->m_maxLinearVelocity;
}

inline hkReal hkpRigidBody::getMaxAngularVelocity() const
{
	return getRigidMotion()->getMotionState()->m_maxAngularVelocity;
}

inline void hkpRigidBody::setMaxLinearVelocity( hkReal maxVel )
{
	getRigidMotion()->getMotionState()->m_maxLinearVelocity = maxVel;
}

inline void hkpRigidBody::setMaxAngularVelocity( hkReal maxVel )
{
	getRigidMotion()->getMotionState()->m_maxAngularVelocity = maxVel;
}


inline enum hkpMotion::MotionType hkpRigidBody::getMotionType() const
{
	return getRigidMotion()->getType();
}

// Sets the linear velocity at the center of mass, in World space.
void hkpRigidBody::setLinearVelocity(const hkVector4& newVel)
{
	HK_ASSERT2(0x19142e8b, newVel.isOk3(), "Velocity passed to hkRigidbody::setLinearVelocity is invalid.");
	activate();
	getRigidMotion()->setLinearVelocity(newVel);
}

// Sets the angular velocity around the center of mass, in World space.
void hkpRigidBody::setAngularVelocity(const hkVector4& newVel)
{
	HK_ASSERT2(0x38c5ec40, newVel.isOk3(), "Velocity passed to hkRigidbody::setAngularVelocity is invalid.");
	activate();
	getRigidMotion()->setAngularVelocity(newVel);
}

#if !defined(HK_PLATFORM_SPU)
hkUint32 hkpRigidBody::getCollisionFilterInfo() const
{
	HK_ACCESS_CHECK_WITH_PARENT( m_world, HK_ACCESS_IGNORE, this, HK_ACCESS_RO );
	return getCollidable()->getBroadPhaseHandle()->getCollisionFilterInfo();
}

void hkpRigidBody::setCollisionFilterInfo( hkUint32 info )
{
	HK_ACCESS_CHECK_WITH_PARENT( m_world, HK_ACCESS_IGNORE, this, HK_ACCESS_RW );
	getCollidableRw()->getBroadPhaseHandle()->setCollisionFilterInfo(info);
}

HK_FORCE_INLINE hkpCollidableQualityType hkpRigidBody::getQualityType() const
{
	HK_ACCESS_CHECK_WITH_PARENT( m_world, HK_ACCESS_IGNORE, this, HK_ACCESS_RO );
	return hkpCollidableQualityType( getCollidable()->getBroadPhaseHandle()->m_objectQualityType );
}

HK_FORCE_INLINE void hkpRigidBody::setQualityType(hkpCollidableQualityType type)
{
	HK_ACCESS_CHECK_WITH_PARENT( m_world, HK_ACCESS_IGNORE, this, HK_ACCESS_RW );
	getCollidableRw()->getBroadPhaseHandle()->m_objectQualityType = hkUint16(type);
}

HK_FORCE_INLINE hkReal hkpRigidBody::getAllowedPenetrationDepth() const
{
	HK_ACCESS_CHECK_WITH_PARENT( m_world, HK_ACCESS_IGNORE, this, HK_ACCESS_RO );
	return getCollidable()->m_allowedPenetrationDepth;
}

HK_FORCE_INLINE void hkpRigidBody::setAllowedPenetrationDepth( hkReal val )
{
	HK_ACCESS_CHECK_WITH_PARENT( m_world, HK_ACCESS_IGNORE, this, HK_ACCESS_RW );
	HK_ASSERT2(0xad45bd3d, val > HK_REAL_EPSILON, "Must use a non-zero ( > epsilon ) value when setting allowed penetration depth of bodies.");
	getCollidableRw()->m_allowedPenetrationDepth = val;
}
#endif

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
