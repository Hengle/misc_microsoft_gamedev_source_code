//==============================================================================
// unitActionThrown.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionthrown.h"
#include "unit.h"
#include "physics.h"
#include "world.h"
#include "pather.h"
#include "tactic.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionThrown, 10, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionThrown::init()
{
   if (!BAction::init())
      return(false);

   mOppID = BUnitOpp::cInvalidID;
   mThrowerID = cInvalidObjectID;
   mThrowerProtoActionID = -1;
   mRestoreCollisionFilter = -1;
   mThrowVelocityScalar = 1.0f;
   mCollided = false;
   mNewThrow = false;
   mReleasePhysicsObject = false;
   mCollisionListener = false;
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionThrown::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   // Connect.
   if (!BAction::connect(pOwner, pOrder))
   {
      return (false);
   }

   // Get the controllers
   if (!grabControllers())
      return false;

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionThrown::disconnect()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if(pUnit)
   {
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
bool BUnitActionThrown::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (mState)
   {
      case cStateNone:
      {
         // for all non-infantry objects with physics objects, we just do the impulse and don't wait for the collision
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if (pProtoObject && pUnit->getPhysicsObject() && !pProtoObject->isType(gDatabase.getOTIDInfantry()))
         {
            impulseUnit();
            setState(cStateDone);
            // Complete the opp
            pUnit->completeOpp(mOppID, true);
            return true;
         }

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

         // Add collision listener to receive ground collisions (if not already added)
         if (!mCollisionListener)
         {
            pUnit->getPhysicsObject()->addHavokCollisionListener(this);
            mCollisionListener=true;
         }

         // Impulse the unit
         impulseUnit();

         // Set the flail anim
         pUnit->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeFlail, true);

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

         // Add additional impulse if necessary
         if (mNewThrow)
         {
            impulseUnit();
            mNewThrow = false;
         }
         // If the physics has collided with the ground, thrown action is done and set
         // the unit back to sim control
         else if (mCollided)
         {
            releaseToSimControl();
            setState(cStateDone);
         }
         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionThrown::setOppID(BUnitOppID oppID)
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
void BUnitActionThrown::impulseUnit()
{
   // TODO - This isn't giving predictable throwing yet.  In fact, it's
   // a bunch of temporary hack crap.  Do more work

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BUnit* pThrowerUnit = gWorld->getUnit(mThrowerID);

   // VT: 4/29/08 Sadly, having and assert here is unsafe, since if a unit throws
   // another unit and immediately dies before our update, this will be null
   // BASSERT(pThrowerUnit);

//-- FIXING PREFIX BUG ID 1646
   const BProtoAction* pThrowerProtoAction  = NULL;
//--
   if(mThrowerProtoActionID != -1 && pThrowerUnit)
      pThrowerProtoAction  = pThrowerUnit->getTactic()->getProtoAction(mThrowerProtoActionID);

   // so, if we have no thrower, just toss with some randomness
   BVector dir, throwerVel;
   float landingDistance;
   if (pThrowerUnit)
   {
#ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionThrown::impulseUnit throwerUnit ID", pThrowerUnit->getID());
#endif
      dir = pUnit->getPosition() - pThrowerUnit->getPosition();
      throwerVel = pThrowerUnit->getVelocity();
      landingDistance = pThrowerUnit->getObstructionRadius() * 2.0f;
   }
   else
   {
      dir.x = getRandDistribution(cSimRand);
      dir.z = getRandDistribution(cSimRand);
      throwerVel.zero();
      landingDistance = pUnit->getObstructionRadius() * 2.0f;
   }

   // Calc direction to throw - for now this is just 45 degrees off
   // the throwing unit's velocity vector based on the direction of collision
   dir.y = 0.0f;
   dir.safeNormalize();
   throwerVel.y = 0.0f;
   throwerVel.safeNormalize();

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionThrown::impulseUnit dir", dir);
   syncUnitActionData("BUnitActionThrown::impulseUnit throwerVel", throwerVel);
   syncUnitActionData("BUnitActionThrown::impulseUnit landingDistance", landingDistance);
#endif

   if(pThrowerProtoAction )
   {
#ifdef SYNC_UnitAction
      if (pThrowerProtoAction->getName())
         syncUnitActionData("BUnitActionThrown::impulseUnit throwerProtoAction Name", pThrowerProtoAction->getName().getPtr());
#endif

      //-- Setup the dir
      float dirAdjustment = pThrowerProtoAction ->getThrowOffsetAngle();
      dir.normalize();
      dir.rotateXZ(dirAdjustment);
   }
   else
   {
      float crossResult = throwerVel.cross(dir).y;
      dir = throwerVel;
      if (crossResult >= 0.0f)
         dir.rotateXZ(cPiOver4);
      else
         dir.rotateXZ(-cPiOver4);
   }


   // Calc ideal landing position
   BVector landingPosition = pUnit->getPosition() + (dir * landingDistance);

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionThrown::impulseUnit dir", dir);
   syncUnitActionData("BUnitActionThrown::impulseUnit landingPosition", landingPosition);
#endif

   // Adjust position for passability
   BVector adjPosition(cOriginVector);
   if (!gPather.findClosestPassableTileEx(landingPosition, adjPosition))
   {
      // Throw straight up
      adjPosition = landingPosition;
#ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionThrown::impulseUnit adjPosition", adjPosition);
#endif
   }

   // Calc impulse to get to adjusted position
   float launchAngle = 30.0f * cRadiansPerDegree;
   float initialVel = 10.0f;

   if(pThrowerProtoAction )
   {
      initialVel = pThrowerProtoAction ->getThrowVelocity();
   }
   initialVel *= mThrowVelocityScalar;
   /*
   float timeInAir = 1.0f;
   float vx = (adjPosition.x - pUnit->getPosition().x) / timeInAir;
   float vz = (adjPosition.z - pUnit->getPosition().z) / timeInAir;
   float vy = (adjPosition.y - pUnit->getPosition().y + (9.8f * timeInAir * timeInAir * 0.5f)) / timeInAir;
   BVector impulse(vx, vy, vz);
   */
   dir.set(adjPosition.x - pUnit->getPosition().x, 0.0f, adjPosition.z - pUnit->getPosition().z);
   if (dir.safeNormalize())
   {
      dir *= cosf(launchAngle);
      dir.y = sinf(launchAngle);
   }
   else
      dir = cYAxisVector;

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionThrown::impulseUnit dir", dir);
   syncUnitActionData("BUnitActionThrown::impulseUnit initialVel", initialVel);
#endif

   // Apply impulse
   BVector velocity = dir * initialVel;
#ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionThrown::impulseUnit setLinearVelocity", velocity);
#endif
   BPhysicsObject* pPO = pUnit->getPhysicsObject();
   if (pPO->getType() == BPhysicsObject::cClamshell)
   {
      BClamshellPhysicsObject* pCPO = reinterpret_cast<BClamshellPhysicsObject*>(pPO);
      //impulse *= pCPO->getMass() * gWorld->getLastUpdateLengthFloat();
      //pCPO->applyImpulse(impulse);

      pCPO->setLinearVelocity(velocity);
      //pCPO->applyImpulse(dir * initialVel / gWorld->getLastUpdateLengthFloat());
   }
   else
   {
      //impulse *= pPO->getMass() * gWorld->getLastUpdateLengthFloat();
      //pPO->applyImpulse(impulse);
      pPO->setLinearVelocity(velocity);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionThrown::releaseToSimControl()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Release physics object if it was created by this action
   if (mReleasePhysicsObject)
   {
      releasePhysicsObject(pUnit);
   }
   // Otherwise restore physics object to previous state
   // (i.e. zero out the velocities, restore collision filter, and remove listener)
   else
   {
      pUnit->getPhysicsObject()->setLinearVelocity(cOriginVector);
      pUnit->getPhysicsObject()->setAngularVelocity(cOriginVector);
      pUnit->getPhysicsObject()->setCollisionFilterInfo(mRestoreCollisionFilter);
      pUnit->getPhysicsObject()->updateCollisionFilter();
      pUnit->getPhysicsObject()->removeHavokCollisionListener(this);
      mCollisionListener=false;
   }

   // make damn sure we're in a valid location
   BVector adjPosition(pUnit->getPosition());
   gPather.findClosestPassableTileEx(pUnit->getPosition(), adjPosition);
   pUnit->setPosition(adjPosition);

   // Reset orientation / position
   BVector fwd = pUnit->getForward();
   fwd.y = 0.0f;
   fwd.safeNormalize();
   BMatrix mtx;
   mtx.makeOrient(fwd, cYAxisVector, cYAxisVector.cross(fwd));
   pUnit->setRotation(mtx);
   pUnit->tieToGround();

   // Set idle animation
   pUnit->setAnimationState(BObjectAnimationState::cAnimationStateIdle, cAnimTypeIdle, true);

   // Complete the opp
   pUnit->completeOpp(mOppID, true);
}

//==============================================================================
//==============================================================================
bool BUnitActionThrown::validateControllers() const
{
   // Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 1647
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   bool valid = (pUnit->getController(BActionController::cControllerOrient)->getActionID() == mID);
   if (!valid)
      return (false);

   valid = (pUnit->getController(BActionController::cControllerAnimation)->getActionID() == mID);

   return (valid);
}

//==============================================================================
//==============================================================================
bool BUnitActionThrown::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Try to grab the orient controller.
   bool grabbed = pUnit->grabController(BActionController::cControllerOrient, this, getOppID());
   if (!grabbed)
   {
      return (false);
   }

   // Try to grab the animation controller
   grabbed = pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());

   return (grabbed);
}

//==============================================================================
//==============================================================================
void BUnitActionThrown::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Release the orientation controller
   pUnit->releaseController(BActionController::cControllerOrient, this);

   // Release the animation controller
   pUnit->releaseController(BActionController::cControllerAnimation, this);
}

//==============================================================================
//==============================================================================
void BUnitActionThrown::contactPointAddedCallback(hkpContactPointAddedEvent& event)
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
   if (event.m_projectedVelocity < 0.0f)
      mCollided = true;
}


//==============================================================================
//==============================================================================
bool BUnitActionThrown::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BEntityID, mThrowerID);
   GFWRITEVAR(pStream, long, mThrowerProtoActionID);
   GFWRITEVAR(pStream, long, mRestoreCollisionFilter);
   GFWRITEVAR(pStream, float, mThrowVelocityScalar);
   GFWRITEBITBOOL(pStream, mCollided);
   GFWRITEBITBOOL(pStream, mNewThrow);
   GFWRITEBITBOOL(pStream, mReleasePhysicsObject);
   GFWRITEBITBOOL(pStream, mCollisionListener);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionThrown::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BEntityID, mThrowerID);
   GFREADVAR(pStream, long, mThrowerProtoActionID);
   GFREADVAR(pStream, long, mRestoreCollisionFilter);
   if (BAction::mGameFileVersion >= 24)
   {
      GFREADVAR(pStream, float, mThrowVelocityScalar);
   }
   GFREADBITBOOL(pStream, mCollided);
   GFREADBITBOOL(pStream, mNewThrow);
   GFREADBITBOOL(pStream, mReleasePhysicsObject);
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
bool BUnitActionThrown::createPhysicsObject(BUnit* pUnit)
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
void BUnitActionThrown::releasePhysicsObject(BUnit* pUnit)
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