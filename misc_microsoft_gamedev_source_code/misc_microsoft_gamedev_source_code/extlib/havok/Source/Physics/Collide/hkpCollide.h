/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_COLLIDE2_COLLIDE2_H
#define HK_COLLIDE2_COLLIDE2_H

#include <Common/Base/hkBase.h>
#include <Common/Base/Monitor/hkMonitorStream.h>
#include <Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h>

#include <Common/Base/Types/Physics/MotionState/hkMotionState.h>
#include <Common/Base/Types/Geometry/Aabb/hkAabb.h>
#include <Common/Base/Types/Geometry/Sphere/hkSphere.h>
#include <Common/Base/Types/Physics/ContactPoint/hkContactPoint.h>


#include <Physics/Collide/Shape/Convex/hkpCdVertex.h>
#include <Physics/Collide/Shape/hkpShape.h>
#include <Physics/Collide/Shape/hkpShapeType.h>
#include <Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h>

#include <Physics/Collide/Dispatch/hkpCollisionDispatcher.h>

#include <Physics/Collide/Agent/Collidable/hkpCollidable.h>
#include <Physics/Collide/Agent/ContactMgr/hkpContactMgr.h>
#include <Physics/Collide/Agent/hkpProcessCdPoint.h>
#include <Physics/Collide/Agent/Collidable/hkpCdPoint.h>
#include <Physics/Collide/Agent/Query/hkpCdPointCollector.h>
#include <Physics/Collide/Agent/hkpCollisionAgent.h>
#include <Physics/Collide/Agent/hkpCollisionInput.h>
#include <Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h>
#include <Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgent.h>

#endif // HK_COLLIDE2_COLLIDE2_H

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
