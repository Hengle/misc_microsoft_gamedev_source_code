/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_CONSTRAINTSOLVER2_SIMPLE_CONSTRAINT_UTIL_H
#define HK_CONSTRAINTSOLVER2_SIMPLE_CONSTRAINT_UTIL_H

#include <Common/Base/hkBase.h>


/// struct to hold same caches for fast collision resolution
/// Note: the size is quit big (6*48 = 288 bytes)


	/// some cached values, which allow for fast solving of simple 3d constraints
	/// (e.g. contact constraints for toi handling)
struct hkpSimpleConstraintInfo
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT_SOLVER, hkpSimpleConstraintInfo );

	struct BodyInfo
	{
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT_SOLVER, hkpSimpleConstraintInfo::BodyInfo );

		hkRotation m_jacT;		// the transposed jacobians, used to get the velocities
		hkMatrix3 m_invIjac;	// used to apply forces	
		//hkMatrix3 m_inertia;
		hkReal m_invMass;
		hkReal m_mass;

		hkReal getMass() const { return m_mass; }
	};

	BodyInfo m_bodyInfo[2];

		/// first column is the normal direction, the other two columns are perpendicular
	hkRotation m_base;

	/// calculated by J^-1 * invI * J
	hkMatrix3 m_invMassMatrix;

		/// = m_invMassMatrix^-1
	hkMatrix3 m_massMatrix;

		/// store the inverted determinint of the invMassMatrix without row and column 1 and 2 
	hkReal& getMass00(){ return 	m_invMassMatrix.getColumn(0)(3); }
	hkReal getMass00() const{ return 	m_invMassMatrix.getColumn(0)(3); }

		/// store the inverted determinint of the invMassMatrix without row and column 1 and 2 
	hkReal& getInvDetM12(){ return 	m_invMassMatrix.getColumn(1)(3); }
	hkReal getInvDetM12() const { return 	m_invMassMatrix.getColumn(1)(3); }

};

struct hkpSimpleConstraintInfoInitInput
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT_SOLVER, hkpSimpleConstraintInfoInitInput );

	hkVector4 m_massRelPos;
	hkMatrix3 m_invInertia;
	hkReal m_invMass;
};

struct hkpSimpleConstraintUtilCollideParams
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT_SOLVER, hkpSimpleConstraintUtilCollideParams );

	hkVector4 m_direction;
	hkReal m_friction;
	hkReal m_restitution;
	hkReal m_maxImpulse;


		/// This is the approaching velocity calculated outside this function
		/// (typically the collision velocity)
		/// This allows to make sure that the function applies an impulse even if the
		/// dynamics projected velocities are > 0
	hkReal m_externalSeperatingVelocity;
	hkReal m_extraSeparatingVelocity;

		// This value will virtuall rotate the colliding surface towards the velocity
	hkReal m_extraSlope;

		//
		//	output
		//
	hkBool m_contactImpulseLimitBreached;	// this is an output variable
};

struct hkpBodyVelocity
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT_SOLVER, hkpBodyVelocity );

	hkVector4 m_linear;
	hkVector4 m_angular;
};

extern "C"
{
		/// Initialize a jacobian cache, directions specifies the direction space, directions.getColumn(0) must be the contact normal.
	void HK_CALL hkSimpleConstraintUtil_InitInfo(	const hkpSimpleConstraintInfoInitInput& bodyA, const hkpSimpleConstraintInfoInitInput& bodyB,
													const hkMatrix3& directions, hkpSimpleConstraintInfo& infoOut);

		/// Get the point velocity between two objects.
	void HK_CALL hkSimpleConstraintUtil_getPointVelocity( const hkpSimpleConstraintInfo& info, hkpBodyVelocity& velA, hkpBodyVelocity& velB, hkVector4& out );



		/// Apply an impulse at object a and -impulse at object b, impulse specified in direction space.
	void HK_CALL hkSimpleConstraintUtil_applyImpulse(	const hkpSimpleConstraintInfo& info, const hkVector4& impulse,
														hkpBodyVelocity& velA, hkpBodyVelocity& velB );

	hkReal HK_CALL hkSimpleConstraintUtil_calcTotalEnergy( const hkpSimpleConstraintInfo& info, hkpBodyVelocity& velA, hkpBodyVelocity& velB );
	
		/// Collide two objects, returns the impulse in the normal direction.
	hkReal  HK_CALL hkSimpleConstraintUtil_Collide(	const hkpSimpleConstraintInfo& info, hkpSimpleConstraintUtilCollideParams& params,
													hkpBodyVelocity& velA, hkpBodyVelocity& velB );
}




#endif // HK_CONSTRAINTSOLVER2_SIMPLE_CONSTRAINT_UTIL_H

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
