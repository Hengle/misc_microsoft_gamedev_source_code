
//============================================================================
// File: physicswarthogaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

//============================================================================
// Includes
#include "common.h"
#include "world.h"
#include "physics.h"
#include "physicsCollision.h"
#include "Physics\Collide\Filter\Group\hkpGroupFilter.h"
#include "Physics\Dynamics\Collide\hkpResponseModifier.h"
#include "Common\Base\Container\LocalArray\hkLocalArray.h"
#include "Physics\Dynamics\Phantom\hkpAabbPhantom.h"
#include "Physics\Collide\Dispatch\BroadPhase\hkpTypedBroadPhaseDispatcher.h"
#include "Physics\Collide\Dispatch\BroadPhase\hkpTypedBroadPhaseHandlePair.h"

//============================================================================
// Globals
BPhantomOverlapListener gPhantomListener;
BLayerFilterCollisionListener gVehicleCollisionListener;

//============================================================================
//============================================================================
// BPhantomOverlapListener
//============================================================================
//============================================================================

//============================================================================
// Note: This can be fired during phantom updating and from any physics update
// thread, so we need to send messages to the sim to handle these collisions
// in outside physics update
//============================================================================
void BPhantomOverlapListener::collidableAddedCallback (const hkpCollidableAddedEvent &event)
{
   if (!event.m_collidable || !event.m_phantom)
      return;

   BEntity* pPhantomEntity = reinterpret_cast<BEntity*>(event.m_phantom->getProperty(BPhysicsWorld::cPropertyEntityReference).getPtr());
   if (!pPhantomEntity)
      return;

   // Phantom-entity
   if (event.m_collidable->getType() == hkpWorldObject::BROAD_PHASE_ENTITY)
   {
      hkpEntity* pHKCollideEntity = reinterpret_cast<hkpEntity*>(event.m_collidable->getOwner());
      if (!pHKCollideEntity)
         return;
      BEntity* pCollideEntity = reinterpret_cast<BEntity*>(pHKCollideEntity->getProperty(BPhysicsWorld::cPropertyEntityReference).getPtr());
      if (!pCollideEntity)
         return;

      /*
      #ifdef DEBUG
         trace("phantom-entity collidable added [%s]-[%s]", pPhantomEntity->getName()->asNative(), pCollideEntity->getName()->asNative());
      #endif
      */
   }
   // Phantom-phantom
   else if (event.m_collidable->getType() == hkpWorldObject::BROAD_PHASE_PHANTOM)
   {
      //trace("phantom-phantom collidable added");
   }
}

//============================================================================
//============================================================================
void BPhantomOverlapListener::collidableRemovedCallback (const hkpCollidableRemovedEvent &event)
{
   //trace("phantom collidable removed");
}

//============================================================================
//============================================================================
// BLayerFilterCollisionListener
//============================================================================
//============================================================================

//============================================================================
//============================================================================
void BLayerFilterCollisionListener::contactPointAddedCallback(hkpContactPointAddedEvent& event)
{
   if ( static_cast<hkpEntity*>(event.m_bodyA->getRootCollidable()->getOwner())->getMaterial().getResponseType() != hkpMaterial::RESPONSE_SIMPLE_CONTACT )
   {
      return;
   }
   if ( static_cast<hkpEntity*>(event.m_bodyB->getRootCollidable()->getOwner())->getMaterial().getResponseType() != hkpMaterial::RESPONSE_SIMPLE_CONTACT )
   {
      return;
   }

   hkpRigidBody* bodyA = static_cast<hkpRigidBody*>(event.m_bodyA->getRootCollidable()->getOwner());
   hkpRigidBody* bodyB = static_cast<hkpRigidBody*>(event.m_bodyB->getRootCollidable()->getOwner());

   //float inverseMaskA = 1.0f;k
   //float inverseMaskB = 1.0f;
   static float mask = 1.0f;

   uint32 collisionFilterA = bodyA->getCollisionFilterInfo();
   uint32 collisionFilterB = bodyB->getCollisionFilterInfo();

   int layerA = hkpGroupFilter::getLayerFromFilterInfo(collisionFilterA);
   int layerB = hkpGroupFilter::getLayerFromFilterInfo(collisionFilterB);
   if (layerA == BPhysicsWorld::cLayerVehicles || layerA == BPhysicsWorld::cLayerNoTerrainVehicles)
   {
      if (hkpGroupFilter::getLayerFromFilterInfo(collisionFilterB) == BPhysicsWorld::cLayerObjects)
        hkpResponseModifier::setInvMassScalingForContact(event.m_internalContactMgr, bodyA, bodyB, *event.m_collisionOutput->m_constraintOwner.val(), 0.0f, mask);
   }
   else if (layerB == BPhysicsWorld::cLayerVehicles || layerB == BPhysicsWorld::cLayerNoTerrainVehicles)
   {
      if (hkpGroupFilter::getLayerFromFilterInfo(collisionFilterA) == BPhysicsWorld::cLayerObjects)
        hkpResponseModifier::setInvMassScalingForContact(event.m_internalContactMgr, bodyA, bodyB, *event.m_collisionOutput->m_constraintOwner.val(), mask, 0.0f);
   }

   // The bodies could be in either order so we have to check both cases
   /*
   if ( ( ( bodyA == m_bodyA ) && (bodyB == m_bodyB ) ) ||
      ( ( bodyB == m_bodyA) && (bodyA == m_bodyB ) ) )
   {
      hkpResponseModifier::setInvMassScalingForContact( event.m_internalContactMgr, bodyA, bodyB, *event.m_collisionOutput->m_constraintOwner.val(), inverseMassA, inverseMassB );
   }
   */
}

//============================================================================
// Batch updating function for phantoms.  This is copied / adapted from
// hkPhantomUtil::setPositionBatch
//============================================================================
void batchUpdatePhantoms()
{
   hkpWorld* pHKWorld = gWorld->getPhysicsWorld()->getHavokWorld();
   if (!pHKWorld)
      return;

   const hkArray<hkpPhantom*>& phantoms = pHKWorld->getPhantoms();
   int initialNumPhantoms = phantoms.getSize();

	hkLocalArray<hkAabb> aabbs(initialNumPhantoms);
	aabbs.setSize(initialNumPhantoms);

	hkLocalArray<hkpBroadPhaseHandle*> handles(initialNumPhantoms);
	handles.setSize(initialNumPhantoms);

   int numPhantoms = 0;
   for (int i = 0; i < phantoms.getSize(); i++)
   {
      hkpPhantom* pPhantom = phantoms[i];
      if (!pPhantom)
         continue;

		HK_ASSERT2(0x0, phantoms[i]->getWorld() == pHKWorld, "All phantoms in setPositionBatch must be in the same world");


      // Update AABB phantoms
      if (pPhantom->getType() == HK_PHANTOM_AABB)
      {
   		hkpCollidable* col = const_cast<hkpCollidable*>( phantoms[i]->getCollidable() ); // need the non-const handle accessor
         hkpEntity* pHKEntity = reinterpret_cast<hkpEntity*>(col->getOwner());
         BEntity* pEntity = reinterpret_cast<BEntity*>(pHKEntity->getProperty(BPhysicsWorld::cPropertyEntityReference).getPtr());
         if (!pEntity || !pEntity->getProtoObject())
            continue;

         // Update handle
   		handles[numPhantoms] = col->getBroadPhaseHandle();

         // Update aabb
         float minObsXZ = Math::Min(pEntity->getProtoObject()->getObstructionRadiusX(), pEntity->getProtoObject()->getObstructionRadiusZ());
         BVector min = BVector(-minObsXZ, 0.0f, -minObsXZ);
         BVector max = BVector(minObsXZ, pEntity->getProtoObject()->getObstructionRadiusY() * 2.0f, minObsXZ);
         aabbs[numPhantoms].m_min = pEntity->getPosition() + min;
         aabbs[numPhantoms].m_max = pEntity->getPosition() + max;

			hkpAabbPhantom* aabbPhantom = static_cast<hkpAabbPhantom*> (phantoms[i]);
         hkAabb& aabb = const_cast<hkAabb&>(aabbPhantom->getAabb());
         aabb = aabbs[numPhantoms];
			//aabbPhantom->m_aabb = aabbs[numPhantoms];

         // Update numPhantoms
         numPhantoms++;
      }
      else
      {
         // Non aabb phantoms unsupported currently
         BASSERT(0);
      }
   }

   // Resize to actual numPhantoms
   aabbs.setSize(numPhantoms);
   handles.setSize(numPhantoms);

   // Phantom setup stuff from original hkPhantomUtil::setPositionBatch.  Here for reference.
   /*
	hkLocalArray<hkAabb> aabbs(numPhantoms);
	aabbs.setSize(numPhantoms);

	hkLocalArray<hkBroadPhaseHandle*> handles(numPhantoms);
	handles.setSize(numPhantoms);

	hkWorld* world = phantoms[0]->getWorld();
	HK_ASSERT2(0x0, world, "All phantoms must be in the world");

	hkReal halfTolerance = 0.5f * world->getCollisionInput()->getTolerance();

	for (int i=0; i<numPhantoms; i++)
	{
		HK_ASSERT2(0x0, phantoms[i]->getWorld() == world, "All phantoms in setPositionBatch must be in the same world");

		hkCollidable* col = const_cast<hkCollidable*>( phantoms[i]->getCollidable() ); // need the non-const handle accessor
		handles[i] = col->getBroadPhaseHandle();

		if (phantoms[i]->getCollidable()->getShape()) // Shape phantoms
		{
			HK_ACCESS_CHECK_WITH_PARENT( world, HK_ACCESS_RW, phantoms[i], HK_ACCESS_RW );
			hkShapePhantom* shapePhantom  = static_cast<hkShapePhantom*>(phantoms[i]);
			//phantoms[i]->getMotionState()->getTransform().setTranslation(positions[i]);
			shapePhantom->getMotionState()->getTransform().setTranslation(positions[i]);
			shapePhantom->getCollidable()->getShape()->getAabb( phantoms[i]->getMotionState()->getTransform(), halfTolerance + extraTolerance, aabbs[i] );
		}
		else // AABB phantoms
		{
			hkAabb oldAabb;
			hkAabbPhantom* aabbPhantom = static_cast<hkAabbPhantom*> (phantoms[i]);
			oldAabb = aabbPhantom->getAabb();

			hkVector4 midpoint; midpoint.setInterpolate4(oldAabb.m_min, oldAabb.m_max, hkQuadRealHalf);
			hkVector4 offset; offset.setSub4(midpoint, positions[i]);
			aabbs[i] = oldAabb;
			aabbs[i].m_max.add4(offset);
			aabbs[i].m_min.add4(offset);

			aabbPhantom->m_aabb = aabbs[i];
		}
	}
   */

	// cribbed from updateAabb
	{
      hkpWorld* world = pHKWorld;

		// Check if the world is locked, if so bail out
		if (world->areCriticalOperationsLockedForPhantoms())
		{
			HK_ASSERT2(0x0, 0, "Can't queue  hkPhantomUtil::setPositionBatch; aborting.");
			return;
		}

		// Perform the actual operation
		HK_ACCESS_CHECK_OBJECT( world, HK_ACCESS_RW );

		world->lockCriticalOperations();

		hkLocalArray<hkpBroadPhaseHandlePair> newPairs( world->m_broadPhaseUpdateSize );
		hkLocalArray<hkpBroadPhaseHandlePair> delPairs( world->m_broadPhaseUpdateSize );

		//hkBroadPhaseHandle* thisObj = m_collidable.getBroadPhaseHandle();

		world->getBroadPhase()->lock();
		world->getBroadPhase()->updateAabbs( handles.begin(), aabbs.begin(), numPhantoms, newPairs, delPairs );

		// check for changes
		if ( newPairs.getSize() != 0 || delPairs.getSize() != 0)
		{
			hkpTypedBroadPhaseDispatcher::removeDuplicates( newPairs, delPairs );

			world->m_broadPhaseDispatcher->removePairs(static_cast<hkpTypedBroadPhaseHandlePair*>(delPairs.begin()), delPairs.getSize());
			world->m_broadPhaseDispatcher->addPairs(static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.begin()), newPairs.getSize(),  world->getCollisionFilter());
		}

		//for (int i=0; i<numPhantoms; i++)
		//{
		//	cleanupNullPointers<hkPhantomOverlapListener>( phantoms[i]->getPhantomOverlapListeners());
		//}

		world->getBroadPhase()->unlock();

		world->unlockAndAttemptToExecutePendingOperations();
	}
}
