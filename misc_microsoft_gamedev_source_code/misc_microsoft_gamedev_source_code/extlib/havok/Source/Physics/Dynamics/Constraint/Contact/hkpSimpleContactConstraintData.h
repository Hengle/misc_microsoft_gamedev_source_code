/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */


#ifndef HK_DYNAMICS2_SIMPLE_CONTACT_CONSTRAINT_H
#define HK_DYNAMICS2_SIMPLE_CONTACT_CONSTRAINT_H

#include <Common/Base/Types/Physics/ContactPoint/hkContactPoint.h>
#include <Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h>
#include <Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h>
#include <Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h>

#include <Physics/Dynamics/Constraint/hkpConstraintInstance.h>
#include <Physics/Dynamics/Constraint/Contact/hkpDynamicsCpIdMgr.h>

#include <Physics/Dynamics/Constraint/hkpConstraintData.h>
#include <Physics/Dynamics/Constraint/Atom/hkpConstraintAtomUtil.h>

class hkContactPoint;
class hkpSolverResults;
class hkpRigidBody;



/// A group of simple contacts.
class hkpSimpleContactConstraintData: public hkpConstraintData
{
	public:

		hkpSimpleContactConstraintData(hkpConstraintInstance* constraint, hkpRigidBody* bodyA, hkpRigidBody* bodyB);
		inline ~hkpSimpleContactConstraintData();

			/// Get number of contact points.
		inline int getNumContactPoints();

			/// Get contactPoint at position i.
		inline const hkContactPointId getContactPointIdAt( int index );

			/// Get the contact point for an id.
		inline hkContactPoint& getContactPoint( int /*hkContactPointId*/ id );


			/// Get the result for the same id.
		inline hkpContactPointProperties* getContactPointProperties( int /*hkContactPointId*/ id );

		hkContactPointId allocateContactPoint( hkpConstraintOwner& constraintOwner, hkContactPoint** cpOut, hkpContactPointProperties** cpPropsOut);

			// return the new number of contact points
#if defined(HK_PLATFORM_SPU)
		HK_FORCE_INLINE
#endif
		int freeContactPoint( hkpConstraintOwner& constraintOwner, hkContactPointId id );

		virtual hkBool isValid() const;

		// hkpConstraintData interface implementations
		virtual void getConstraintInfo( hkpConstraintData::ConstraintInfo& infoOut ) const;

			// hkpConstraintData interface implementation
		virtual void getRuntimeInfo( hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut ) const;

		virtual hkpSolverResults* getSolverResults( hkpConstraintRuntime* runtime );

		virtual int getType() const;
	
			/// This function is just called before a toi or a normal collision response.
			/// Those collision response functions are called for every new contact point
			/// where the objects have a colliding velocity > hkpWorldCinfo.m_contactRestingVelocity
		virtual void collisionResponseBeginCallback( const hkContactPoint& cp, struct hkpSimpleConstraintInfoInitInput& inA, struct hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB );

		virtual void collisionResponseEndCallback( const hkContactPoint& cp, hkReal impulseApplied, struct hkpSimpleConstraintInfoInitInput& inA, struct hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB);

#if !defined(HK_PLATFORM_SPU)
		inline hkpSimpleContactConstraintData( hkFinishLoadedObjectFlag f ) { }
#endif
	public:

		hkpDynamicsCpIdMgr		m_idMgrA;
		void*					m_clientData;
		hkpConstraintInstance*	m_constraint;

		hkpSimpleContactConstraintAtom*	m_atom;
		int								m_atomSize;
};



#include <Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.inl> 

#endif // HK_DYNAMICS2_SIMPLE_CONTACT_CONSTRAINT_H

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
