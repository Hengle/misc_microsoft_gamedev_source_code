/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_PHANTOM_H
#define HK_DYNAMICS2_PHANTOM_H

#include <Physics/Dynamics/World/hkpWorldObject.h>
#include <Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h>
#include <Physics/Dynamics/Phantom/hkpPhantomType.h>

class hkAabb;
class hkpCollidable;
class hkPhantomCinfo;
class hkpPhantomListener;

extern const hkClass hkpPhantomClass;

/// Helper function that returns a hkpPhantom if the collidable's broad phase handle is of type hkpWorldObject::BROAD_PHASE_PHANTOM
inline hkpPhantom* HK_CALL hkGetPhantom(const hkpCollidable* collidable);

/// A phantom is simply an aabb entry in the world's broad phase. It can be moved around,
/// and receives events when a new aabb overlap occurs with another phantom or entity,
/// or an existing overlap ends. These events can occur either when you move the phantom around, or another
/// object moves into or out of the phantom's aabb. <br>
/// One major reason for using phantoms is to exploit frame coherency the broadphase. Moving objects
/// around within the broadphase can be 5-100 times faster than querying the broadphase from scratch.<br>
/// Therefore phantoms should be is used for collision queries where you wish to make several queries for
/// an object that is relatively persistent in location:
/// <ul>
///     <li>use hkpShapePhantom if your query involves a shape (e.g find all objects penetrating a sphere)
///     <li>use hkpAabbPhantom if your query does not use a shape (e.g. ray casts or simple AABB checks)
///      You can also an hkpAabbPhantom to create areas where events occur if the area is entered.
/// </ul>
/// <br>
/// Phantoms can raise a number of events:
/// <ul>
///     <li>phantom added/removed from the world ( received by hkpPhantomListener)
///     <li>broadphase overlaps created/destroyed (received by hkpPhantomOverlapListener)
///      Note: the hkpPhantomOverlapListener can actually reject a new overlap, thereby acting as 
///      a user collision filter.
/// </ul>
/// <br>
/// Notes:
/// <ul>
///   <li>All broad phase objects in the havok world are actually hkpCollidable objects.
///   <li>The addition and removal callback functions are addOverlappingCollidable and removeOverlappingCollidable (with the cast done for you).
///   <li>All hkpPhantom objects use the hkpWorld::getCollisionFilter as the filter for broad phase updates, and shape queries. The one notable
///       exception is that removeOverlappingCollidable() callbacks are fired independent of the current filter. See the UserGuide for more details.
/// </ul>
/// Attention: You only get good broadphase CPU, if the aabb is not jumping randomly around in your world. If it is,
/// the hkpPhantom can only do a poor job of caching the broadphase and can result in a slower performance than calling the 
/// some functions in hkpWorld.<br>
class hkpPhantom : public hkpWorldObject
{
	public:
	
		HK_DECLARE_REFLECTION();

		HK_DECLARE_CLASS_ALLOCATOR( HK_MEMORY_CLASS_PHANTOM );

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		~hkpPhantom();
			
			/// Gets the hkpPhantom type.
		virtual hkpPhantomType getType() const = 0;

			/// Get the current aabb
		virtual void calcAabb( hkAabb& aabb ) = 0;
		
			/// Called by the broadphase for every new overlapping collidable
		virtual void addOverlappingCollidable( hkpCollidable* collidable ) = 0;

			/// Tests whether a collidable is known in the phantom
		virtual hkBool isOverlappingCollidableAdded( hkpCollidable* collidable ) = 0;


			/// Called by the broadphase for every removed collidable - This may get called even if there was
			/// no corresponding addOverlappingCollidable() if the bodies are filtered not to collide.
			/// See the UserGuide for more details.
		virtual void removeOverlappingCollidable( hkpCollidable* collidable ) = 0;

			/// Given a phantom (this), return a new phantom that shares all static data
			/// such as the shape, but clones any dynamic runtime data.
		virtual hkpPhantom* clone() const = 0; 

		//
		// Listeners
		//

			/// Adds a phantom listener to the phantom
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void addPhantomListener( hkpPhantomListener* el);

			/// Removes an phantom listener from the phantom
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void removePhantomListener( hkpPhantomListener* el);

			/// Adds a phantom listener to the phantom
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void addPhantomOverlapListener( hkpPhantomOverlapListener* el);

			/// Removes an phantom listener from the phantom
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void removePhantomOverlapListener( hkpPhantomOverlapListener* el);

			/// Get const access to the array of phantom listeners
		inline const hkArray<hkpPhantomListener*>& getPhantomListeners() const;

			/// Get const access to the array of phantom overlap listeners
		inline const hkArray<hkpPhantomOverlapListener*>& getPhantomOverlapListeners() const;

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RO] );
		void calcStatistics( hkStatisticsCollector* collector ) const;

	public:

			// Called internally by hkpWorld, this function updates all cached agents (if there are any)
			// to make sure they correspond correctly with the world's shape collection filter
		virtual void updateShapeCollectionFilter() {}

			// Fires phantom-added callbacks through all attached hkPhantomListeners
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		void firePhantomAdded( );
			// Fires phantom-removed callbacks through all attached hkPhantomListeners
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		void firePhantomRemoved( );
			// Fires phantom's shape set callbacks through all attached hkPhantomListeners
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		void firePhantomShapeSet( );


	protected:

		inline hkpPhantom( const hkpShape* shape );

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		void firePhantomDeleted( );

	protected:
			/// fires the callbacks and returns hkpCollidableAccept
		inline hkpCollidableAccept fireCollidableAdded( const hkpCollidable* collidable );

			/// fires the callbacks and returns hkpCollidableAccept
		inline void fireCollidableRemoved( const hkpCollidable* collidable, hkBool collidableWasAdded );

	public:
			// used by hkpWorldOperationUtil
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RW] );
		void updateBroadPhase( const hkAabb& aabb );
		
	protected:

		hkArray<hkpPhantomOverlapListener*> m_overlapListeners; //+nosave
		hkArray<hkpPhantomListener*>	       m_phantomListeners; //+nosave


	public:

		hkpPhantom( class hkFinishLoadedObjectFlag flag ) : hkpWorldObject( flag ) {}

		//
		// INTERNAL USE ONLY
		//

		// Called when a phantom is removed from the world.
		// In the case of an object being loaded from a packfile
		// This method ensures that any arrays that have grown dynamically 
		// After loading are correctly deallocated.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		virtual void deallocateInternalArrays();
};



#include <Physics/Dynamics/Phantom/hkpPhantom.inl>


#endif	//HK_DYNAMICS2_PHANTOM_H

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
