//==============================================================================
// physicsobject.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _PHYSICS_LISTENER
#define _PHYSICS_LISTENER

//==============================================================================
// Includes
//==============================================================================
// Forward declarations
//==============================================================================
// Const declarations
//==============================================================================
class BPhysicsCollisionData
{
public:
   void *mpObjectA;
   void *mpObjectB;

   BVector    mNormal;
   BVector    mLocation;
   float       mfVelocity;
   long        mSurfaceID;
};

//==============================================================================
class BPhysicsCollisionListener
{
public:
   virtual bool collision(const BPhysicsCollisionData &collisionData)=0;
   virtual bool preCollision(const BPhysicsCollisionData &collisionData) { return false; }
};

#endif