//==============================================================================
// UnitActionBomb.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "UnitActionBomb.h"
#include "unit.h"
#include "physics.h"
#include "world.h"
#include "tactic.h"
#include "pather.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionBomb, 10, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionBomb::init()
{
   if (!BAction::init())
      return(false);

   mOppID = BUnitOpp::cInvalidID;
   mThrowerID = cInvalidObjectID;
   mRestoreCollisionFilter = -1;
   mCollided = false;
   mReleasePhysicsObject = false;
   mRoll=false;
   mCollisionListener=false;
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionBomb::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   // Connect.
   if (!BAction::connect(pOwner, pOrder))
   {
      return (false);
   }

   // Get the controllers
   if (!grabControllers())
      return false;

   BASSERT(mpProtoAction);

   float rollChance = getRandMax(cSimRand,1.0f);
   if (rollChance <= mpProtoAction->getWorkRange())
      mRoll = true;

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionBomb::disconnect()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if (pUnit)
   {
      pUnit->getPhysicsObject()->enableDeactivation(true);

      if (mReleasePhysicsObject)
      {
         releasePhysicsObject(pUnit);
      }

      // Remove collision listener if it hasn't already been removed above.  This case can happen
      // if this action didn't create the physics object (and thus mReleasePhysicsObject if false)
      if (mCollisionListener)
      {
         BPhysicsObject* pPO = pUnit->getPhysicsObject();
         if (pPO)
         {
            pPO->removeHavokCollisionListener(this);
            mCollisionListener = false;
         }
      }
   }

   //Release our controllers.
   releaseControllers();

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
bool BUnitActionBomb::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (mState)
   {
      case cStateNone:
      {
         // Create physics object if necessary
         if (!pUnit->getPhysicsObject())
         {
            if(!createPhysicsObject(pUnit))
            {
               setState(cStateFailed);
               pUnit->completeOpp(mOppID, false);
               return(false);
            }
         }

         // Set the collision filter so vehicles won't introduce additional, non-predictable
         // impulses.
         mRestoreCollisionFilter = pUnit->getPhysicsObject()->getCollisionFilterInfo();
         pUnit->getPhysicsObject()->setCollisionFilterInfo(gWorld->getPhysicsWorld()->getVehicleCollisionFilterInfo());
         pUnit->getPhysicsObject()->updateCollisionFilter();

         // Add collision listener to receive ground collisions
         if (!mCollisionListener)
         {
            pUnit->getPhysicsObject()->addHavokCollisionListener(this);
            mCollisionListener=true;
         }

         // Drop safely if we can
         pUnit->getPhysicsObject()->enableDeactivation(false);
         pUnit->getPhysicsObject()->forceActivate();
         // Impulse the unit if we're rolling (otherwise it will just drop straight down)
         if (mRoll)
            impulseUnit();

         setState(cStateWorking);
         break;
      }
         
      case cStateWorking:
      {
         if (!validateControllers())
         {
            releaseToSimControl();
            setState(cStateFailed);
            return false;
         }

         // If the physics has gotten a callback, check distance to ground.  Once we're close enough, release
         // control back to the sim
         if (mCollided)
         {
            float height;
            gTerrainSimRep.getHeightRaycast(pUnit->getPosition(), height, true);

            if (pUnit->getPosition().y - height < 1.0f)
            {
               releaseToSimControl();
               setState(cStateDone);
            }
         }
         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionBomb::setOppID(BUnitOppID oppID)
{
   if (mpOwner)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }
   mOppID=oppID;
}

//==============================================================================
//==============================================================================
void BUnitActionBomb::impulseUnit()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   static float initialVel = 20.0f;

   // Apply impulse
   BPhysicsObject* pPO = pUnit->getPhysicsObject();
   BVector velocity = mDir * initialVel;
#ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionBomb::impulseUnit setLinearVelocity", velocity);
#endif
   pPO->setLinearVelocity(velocity);
}

//==============================================================================
//==============================================================================
void BUnitActionBomb::releaseToSimControl()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Release physics object if it was created by this action
   if (mReleasePhysicsObject)
   {
      releasePhysicsObject(pUnit);
   }

   // Complete the opp
   pUnit->completeOpp(mOppID, true);
}

//==============================================================================
//==============================================================================
bool BUnitActionBomb::validateControllers() const
{
   // Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 4912
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   bool valid = (pUnit->getController(BActionController::cControllerOrient)->getActionID() == mID);
   if (!valid)
      return (false);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBomb::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Try to grab the orient controller.  
   bool grabbed = pUnit->grabController(BActionController::cControllerOrient, this, getOppID());
   if (!grabbed)
   {
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionBomb::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Release the orientation controller
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
void BUnitActionBomb::contactPointAddedCallback(hkpContactPointAddedEvent& event)
{
   hkpShapeType aType = event.m_bodyA->getShape()->getType();
   hkpShapeType bType = event.m_bodyB->getShape()->getType();

   // Make sure this is a ground collision
   if ((aType != HK_SHAPE_SAMPLED_HEIGHT_FIELD) &&
       (bType != HK_SHAPE_SAMPLED_HEIGHT_FIELD))
   {
      return;
   }

   // This needs to be a collision when the unit is falling and not on the way up
   // NOTE: Moving object A in the direction of the normal will separate the objects.  So
   // check that the contact normal is making the objects move closer together.

   // TODO - This isn't sufficient for detecting collision since objects like the warthog
   // with a suspension supporting it's physics object won't return a collision with
   // the ground
   if (event.m_projectedVelocity < 0.0f && mpOwner)
   {
      // If we're not rolling, die on hit so we immediately detonate
      if (!mRoll)
      {
         BUnit* pUnit = mpOwner->getUnit();
         pUnit->setHitpoints(0.0f);
      }

      mCollided = true;
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionBomb::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BEntityID, mThrowerID);
   GFWRITEVAR(pStream, long, mRestoreCollisionFilter);
   GFWRITEVECTOR(pStream, mDir);
   GFWRITEBITBOOL(pStream, mCollided);
   GFWRITEBITBOOL(pStream, mReleasePhysicsObject);
   GFWRITEBITBOOL(pStream, mRoll);
   GFWRITEBITBOOL(pStream, mCollisionListener);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBomb::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BEntityID, mThrowerID);
   GFREADVAR(pStream, long, mRestoreCollisionFilter);
   GFREADVECTOR(pStream, mDir);
   GFREADBITBOOL(pStream, mCollided);
   GFREADBITBOOL(pStream, mReleasePhysicsObject);
   GFREADBITBOOL(pStream, mRoll);
   bool collisionListener;
   GFREADBITBOOL(pStream, collisionListener);

   if (collisionListener && mpOwner->getPhysicsObject())
   {
      mpOwner->getPhysicsObject()->addHavokCollisionListener(this);
      mCollisionListener=true;
   }

   return true;
}



//==============================================================================
//==============================================================================
bool BUnitActionBomb::createPhysicsObject(BUnit* pUnit)
{
   long physicsInfoID = pUnit->getProtoObject()->getPhysicsReplacementInfoID();
   if (!pUnit->createPhysicsObject(physicsInfoID, NULL, false, false))
   {
      return false;
   }

   mReleasePhysicsObject = true; // flag to release physics when complete

   return true;
}


//==============================================================================
//==============================================================================
void BUnitActionBomb::releasePhysicsObject(BUnit* pUnit)
{
   BPhysicsObject* pPhysicsObject = pUnit->getPhysicsObject();
   BASSERT(pPhysicsObject);
   pPhysicsObject->removeHavokCollisionListener(this);
   mCollisionListener=false;

   pUnit->releasePhysicsObject();
   pUnit->setFlagPhysicsControl(false);
   pUnit->setFlagTiesToGround(true);

   // Re-create original physics
   if (pUnit->getProtoObject() && (pUnit->getProtoObject()->getPhysicsInfoID() != -1))
      pUnit->createPhysicsObject(pUnit->getProtoObject()->getPhysicsInfoID(), NULL, false, false);

   mReleasePhysicsObject = false;
}