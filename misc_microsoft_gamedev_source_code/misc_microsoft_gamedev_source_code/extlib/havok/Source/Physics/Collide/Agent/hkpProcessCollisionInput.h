/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_COLLIDE2_PROCESS_COLLISION_INPUT_H
#define HK_COLLIDE2_PROCESS_COLLISION_INPUT_H

#include <Common/Base/Types/Physics/hkStepInfo.h>
#include <Physics/Collide/Agent/hkpCollisionInput.h>

#if defined(HK_PLATFORM_HAS_SPU)
#	include <Physics/Internal/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h>
#endif

struct hkpAgent1nSector;
struct hkpCollisionQualityInfo;


	/// This structure is used for all process collision calls queries.
struct hkpProcessCollisionInput : public hkpCollisionInput
{
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_AGENT, hkpProcessCollisionInput);

	public:

			/// time step information
		hkStepInfo m_stepInfo;

			// Factor used to scale collision tolerance for toi events
		//hkPadSpu<hkReal>  m_toiToleranceFactor;

			/// A pointer to the collision quality information. See hkpCollisionDispatcher for more details
		mutable hkPadSpu<hkpCollisionQualityInfo*> m_collisionQualityInfo;

#if defined(HK_PLATFORM_HAS_SPU)
		hkPadSpu<struct hkpSpuAgentSectorJobCollisionObjects*> m_collisionObjects;
#endif

			/// a pointer to hkpWorldDynamicsStepInfo if you use the hkDynamics lib, otherwise this can be used as a user pointer
		void*	m_dynamicsInfo;

		bool m_enableDeprecatedWelding;

			/// A pointer to a structure containing internal collision tolerances etc.
		hkpCollisionAgentConfig* m_config;

};

#endif // HK_COLLIDE2_PROCESS_COLLISION_INPUT_H

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
