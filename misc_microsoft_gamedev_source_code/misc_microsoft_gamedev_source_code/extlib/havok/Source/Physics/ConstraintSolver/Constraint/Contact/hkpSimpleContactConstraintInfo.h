/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_CONSTRAINTSOLVER2_SIMPLE_CONTACT_CONSTRAINT_INFO_H
#define HK_CONSTRAINTSOLVER2_SIMPLE_CONTACT_CONSTRAINT_INFO_H

#include <Physics/ConstraintSolver/Solve/hkpSolverResults.h>

HK_REFLECTION_CLASSFILE_DESTINATION("../../../Dynamics/Constraint/Contact");

class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkContactPoint;
class hkpContactPointProperties;
struct hkpSimpleContactConstraintAtom; 
class hkpSimpleContactConstraintDataInfo
{
	public:
		HK_DECLARE_REFLECTION();
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT_SOLVER, hkpSimpleContactConstraintDataInfo );

		enum {	HK_FLAG_OK = 0x00, HK_FLAG_POINT_REMOVED = 0x01, HK_FLAG_AREA_CHANGED = 0x04 };

		void init()
		{
			m_flags = HK_FLAG_OK;
			m_index = 3;
			for(int i = 0; i < 7; ++i)
			{
				m_data[i] = 0;
			}
		}

		hkpSimpleContactConstraintDataInfo() 
		{
			init();
		}

		HK_ALIGN16(hkUint16 m_flags);
		hkUint16 m_index;
		hkUint32 m_data[7];


		hkpSimpleContactConstraintDataInfo(hkFinishLoadedObjectFlag f) {}
};

/// This parameter helps to tune penetration recovery of contacts. This is
/// especially useful:
///   - For non standard sized worlds: set it to how quickly deep penetrating objects should separate 
///   - Or if you run the solver at a high number of iterations, you should set:
///         HK_CONTACT_LINEAR_ERROR_RECOVERY_VELOCITY = 10/solverIterations

extern hkReal HK_CONTACT_LINEAR_ERROR_RECOVERY_VELOCITY;
extern hkReal HK_CONTACT_EXPONENTIAL_ERROR_RECOVERY_VELOCITY;

extern "C"
{
		/// build the jacobians for a contact constraints, (Note: jitContactPoints are contact points which are externally created just in time
	void HK_CALL hkSimpleContactConstraintDataBuildJacobian( hkpSimpleContactConstraintAtom* atom, const hkpConstraintQueryIn& in, hkBool writeHeaderSchema, hkpConstraintQueryOut& out );

	void HK_CALL hkSimpleContactConstraintDataAddALittleSurfaceViscosity( hkpSimpleContactConstraintDataInfo* info );

}


#endif // HK_CONSTRAINTSOLVER2_SIMPLE_CONTACT_CONSTRAINT_INFO_H

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
