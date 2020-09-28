//==============================================================================
// simpleraycastcollisionfilter.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================



#ifndef _SIMPLE_RAYCAST_COLLISION_FILTER_H_
#define _SIMPLE_RAYCAST_COLLISION_FILTER_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


//==============================================================================
class BSimpleRaycastCollisionFilter : public hkpGroupFilter
{
public:


   BEGIN_FLAG_MAP
      ADD_FLAG_ENTRY(cFlagRaycastIgnoreObjects,1)
      ADD_FLAG_ENTRY(cFlagRaycastIgnoreLevel,2)
   END_FLAG_MAP

   DECLARE_FLAGS;

   BSimpleRaycastCollisionFilter() : hkpGroupFilter()
   {
      memset(&mFlags, 0, sizeof(mFlags)); 
   };

   virtual ~BSimpleRaycastCollisionFilter()
   {
     
   };

   /// hkpRayCollidableFilter interface forwarding
   virtual  hkBool isCollisionEnabled( const hkpWorldRayCastInput& a, const hkpCollidable& collidableB ) const
   {

      //-- determine if we hit the level
      bool bLevelHit = false;

      const hkpCollidable *pHit = &collidableB;

      if (pHit->getType() == hkpWorldObject::BROAD_PHASE_ENTITY)
      {
         hkpRigidBody *phkRigidBody = (hkpRigidBody*) pHit->getOwner();
         if (phkRigidBody)
         {
            hkpPropertyValue &value = phkRigidBody->getProperty(BPhysicsWorld::cPropertyEntityReference);

            if (value.getPtr() == 0)
              bLevelHit = true;
         }
      }

      if (getFlag(cFlagRaycastIgnoreObjects) && !bLevelHit)
         return (false);

      if (getFlag(cFlagRaycastIgnoreLevel) && bLevelHit)
         return (false);
     

      return hkpGroupFilter::isCollisionEnabled(a, collidableB);
   }

   
   
protected:



}; 


//==============================================================================
#endif // _SIMPLE_RAYCAST_COLLISION_FILTER_H_

//==============================================================================
// eof: simpleraycastcollisionfilter.h
//==============================================================================