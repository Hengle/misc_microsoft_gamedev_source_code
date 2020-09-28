/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_BLENDING_MOTOR_H
#define HK_DYNAMICS2_BLENDING_MOTOR_H

#include <Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h>

extern const hkClass hkpPositionConstraintMotorClass;

	/// a motor which tries to reach a desired position/angle given
	/// a maxForce and a recovery speed.
	/// Note:
	///  - this motor is a good choice for driving a ragdoll to a given pose
	///  - This motor is accumulates the error over time to be able to archive high forces
	///    This can lead to a small jitter, the first time the motor is activated.
	///    To reduce this jitter, you can either try to fade in the motor max force or
	///    by setting the maxForce to some reasonable value.
class hkpPositionConstraintMotor : public hkpLimitedForceConstraintMotor
{
	public:

		HK_DECLARE_REFLECTION();

			/// Will resets all values to 0. except m_constantRecoveryVelocity = 2, m_proportionalRecoveryVelocity = 1 
		hkpPositionConstraintMotor( hkReal currentPosition = 0.0f);

			/// Motor clone method
		virtual hkpConstraintMotor* clone() const;

	public:

			/// the relative stiffness between 0 and 1
		hkReal				m_tau;

			/// the relative damping between 0 and 1
		hkReal				m_damping;

			/// A factor to the current error to calculate the recovery velocity
		hkReal m_proportionalRecoveryVelocity;

			/// A constant velocity which is used to recover from errors
		hkReal m_constantRecoveryVelocity;


	public:
	
		hkpPositionConstraintMotor(hkFinishLoadedObjectFlag f) : hkpLimitedForceConstraintMotor(f) {}

};

#endif // HK_DYNAMICS2_BLENDING_MOTOR_H

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
