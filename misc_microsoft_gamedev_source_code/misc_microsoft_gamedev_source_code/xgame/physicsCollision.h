//============================================================================
// File: physicsCollision.h
//  
// Copyright (c) 2008, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "Physics\Dynamics\Phantom\hkpPhantomOverlapListener.h"
#include "Physics/Dynamics/Collide/hkpCollisionListener.h"

//============================================================================
// Phantom collision listener callbacks
//============================================================================
class BPhantomOverlapListener :  public hkpPhantomOverlapListener
{
   public:

      virtual void collidableAddedCallback (const hkpCollidableAddedEvent &event);
      virtual void collidableRemovedCallback (const hkpCollidableRemovedEvent &event);
};
extern BPhantomOverlapListener gPhantomListener;

//============================================================================
// Rigid body collision listener callbacks
//============================================================================
class BLayerFilterCollisionListener : public hkpCollisionListener
{
   public:

      virtual void contactPointAddedCallback(hkpContactPointAddedEvent& event);
      virtual void contactPointConfirmedCallback( hkpContactPointConfirmedEvent& event) {}
      virtual void contactPointRemovedCallback( hkpContactPointRemovedEvent& event ) {}
      virtual void contactProcessCallback( hkpContactProcessEvent& event ) {}
};
extern BLayerFilterCollisionListener gVehicleCollisionListener;

//============================================================================
// Global utility functions
//============================================================================
void batchUpdatePhantoms();