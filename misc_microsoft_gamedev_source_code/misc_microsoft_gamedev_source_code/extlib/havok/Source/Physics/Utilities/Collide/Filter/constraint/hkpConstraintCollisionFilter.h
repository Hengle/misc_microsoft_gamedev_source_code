/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_COLLIDE2_CONSTRAINT_COLLISION_FILTER_H
#define HK_COLLIDE2_CONSTRAINT_COLLISION_FILTER_H


#include <Physics/Utilities/Collide/Filter/pair/hkpPairCollisionFilter.h>
#include <Physics/Dynamics/Constraint/hkpConstraintListener.h>


class hkpWorld;



/// This filter allows to disable collisions between two entities if they are connected through a constraint (other than a contact constraint).
///
/// You can supply a child filter which will be queried first. If this child filter decides to disable the collision between the entities we will return that. Otherwise
/// the hkpConstraintCollisionFilter will check its internal table for any disabled pairs.
///
/// This filter also acts as a hkpConstraintListener and therefore automatically add and removes entity pairs upon addition and removal of constraints.
/// The filter provides a utility function updateFromWorld() which allows to sync itself with the world's current state.
class hkpConstraintCollisionFilter : public hkpPairCollisionFilter, public hkpConstraintListener
{
	public:

		//HK_DECLARE_REFLECTION();

			/// If you provide a child filter here it will be queried before using this filter's table to check for disabled pairs.
		 hkpConstraintCollisionFilter(const hkpCollisionFilter* otherFilter = HK_NULL);
		 hkpConstraintCollisionFilter( class hkFinishLoadedObjectFlag flag ) {}
		~hkpConstraintCollisionFilter();

			/// Sync the filter with the supplied world's current state, i.e. add all current constraints to its internal table of disabled entity pairs.
		void updateFromWorld(hkpWorld* world);

		//
		// Implementation of the hkpConstraintListener interface.
		//

			/// Called when a constraint is added to the world.
		virtual void constraintAddedCallback( hkpConstraintInstance* constraint );

			/// Called when a constraint is removed from the world.
		virtual void constraintRemovedCallback( hkpConstraintInstance* constraint );


};


#endif // HK_COLLIDE2_CONSTRAINT_COLLISION_FILTER_H

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
