
/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */


#ifndef HK_CONSTRAINTSOLVER2_VELOCITY_ACCUMULATOR_H
#define HK_CONSTRAINTSOLVER2_VELOCITY_ACCUMULATOR_H

	/// This is a simplified Rigid body as it is used by the
	/// constraint solver.
	/// Note: This class has different purposes, whether it is used for setting
	/// up the constraint solver input (hkConstraintDate::buildJacobian()), or when it is 
	/// used as a solver output.
class hkpVelocityAccumulator 
{
	public:
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT_SOLVER, hkpVelocityAccumulator );

	    /// A list of possible types
	    enum hkpAccumulatorType
	    {
		    /// A rigid body
		    HK_RIGID_BODY,

		/// A fixed rigid body, which ignores gravity
		HK_KEYFRAMED_RIGID_BODY,

		/// A rigid body which has no gravity applied (used for character controller)
		HK_NO_GRAVITY_RIGID_BODY,

		HK_END
	};

	hkEnum<hkpAccumulatorType,hkUint8> m_type;	

		/// set to true if coreFromWorldMatrix is the identity matrix
	hkBool m_matrixIsIdentity;	

		/// An index into hkpSolverInfo::m_deactivationInfo which is taken from
		/// hkpRigidBodyCinfo::m_solverDeactivation
	hkUint32 m_deactivationClass;


		/// the linear velocity
	hkVector4 m_linearVel;

		/// the angular velocity in principle axis space 
	hkVector4 m_angularVel;

		/// the inverted diagonal of the inertia tensor in local space.
		/// plus the w component is the inverted mass
	hkVector4 m_invMasses;

	//
	//	Solver input accessors
	// 

	/// Get center of mass in world space. 
	/// Note: This only works before the constraint solver is called (e.g. in hkConstraintDate::buildJacobian())
	hkVector4& getCenterOfMassInWorld() { return m_scratch0; }

	/// Get center of mass in world space. 
	/// Note: This only works before the constraint solver is called (e.g. in hkConstraintDate::buildJacobian())
	const hkVector4& getCenterOfMassInWorld() const { return m_scratch0; }

	/// Get The matrix converting from world to principle axis space
	/// Note: This only works before the constraint solver is called (e.g. in hkConstraintDate::buildJacobian())
	hkRotation& getCoreFromWorldMatrix() { return reinterpret_cast<hkRotation&>(m_scratch1); }

	/// Get The matrix converting from world to principle axis space
	/// Note: This only works before the constraint solver is called (e.g. in hkConstraintDate::buildJacobian())
	const hkRotation& getCoreFromWorldMatrix() const { return reinterpret_cast<const hkRotation&>(m_scratch1); }

	/// Get the average velocity during constraint solving multiplied by the number of substeps
	/// Note: this function only works after the constraint solver is called
	hkVector4& getSumLinearVel() { return m_scratch0; }

	/// Get the average velocity in principle axis space during constraint solving multiplied by the number of substeps
	/// Note: this function only works after the constraint solver is called
	hkVector4& getSumAngularVel() { return m_scratch1; }

	/// Get the average velocity during constraint solving multiplied by the number of substeps
	/// Note: this function only works after the constraint solver is called
	const hkVector4& getSumLinearVel() const { return m_scratch0; }

	/// Get the average velocity during constraint solving multiplied by the number of substeps
	/// Note: this function only works after the constraint solver is called
	const hkVector4& getSumAngularVel() const { return m_scratch1; }


public:		// internal, public needed for HK_COMPILE_TIME_ASSERT
	hkVector4 m_scratch0;
	hkVector4 m_scratch1;
	hkVector4 m_scratch2;
	hkVector4 m_scratch3;

public:
	/// Set this velocity accumulator to the one of a fixed object
	HK_FORCE_INLINE void setFixed()
	{
		m_type = hkpVelocityAccumulator::HK_KEYFRAMED_RIGID_BODY;
		m_matrixIsIdentity = true;
		this->m_invMasses.setZero4();
		this->m_linearVel.setZero4();
		this->m_angularVel.setZero4();
		this->getCenterOfMassInWorld().setZero4();
		this->getCoreFromWorldMatrix().setZero();
	}
};

#endif // HK_CONSTRAINTSOLVER2_VELOCITY_ACCUMULATOR_H

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
