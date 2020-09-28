//==============================================================================
// unitactiondeath.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactiondeath.h"
#include "entity.h"
#include "protoobject.h"
#include "protosquad.h"
#include "unitactionmove.h"
#include "unit.h"
#include "world.h"
#include "game.h"
#include "usermanager.h"
#include "user.h"
#include "syncmacros.h"
#include "visualitem.h"
#include "physicsInfo.h"
#include "physicsInfoManager.h"
#include "physics.h"
#include "visual.h"
#include "corpsemanager.h"
#include "configsgame.h"
#include "worldsoundmanager.h"
#include "weapontype.h"
#include "pather.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionDeath, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionDeath::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   grabControllers();

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   // SLB: We don't allow this object to become invisible while it's animating. Let players see the entire death animation.
   pUnit->setFlagRemainVisible(true);

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionDeath::connect unitID", pOwner->getID().asLong());
   #endif
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionDeath::disconnect()
{
   //BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner); // mrh 8/8/07 - removed warning.
   //BASSERT(pUnit);
   BASSERT(mpOwner);
   #ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionDeath::disconnect unitID", mpOwner->getID().asLong());
   #endif

   releaseControllers();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionDeath::init()
{
   if (!BAction::init())
      return (false);

   mFlagConflictsWithIdle = true;
   mbSetToDetonate = false;
   mKillingEntity = cInvalidObjectID;
   mKillingWeaponType = -1;
   mDeathAnimType = -1;
   mCrashRollAccel = 0.0f;
   mRollRate = 0.0f;
   mCrashPitchAccel = 0.0f;
   mPitchRate = 0.0f;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionDeath::setState(BActionState state)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   #ifdef SYNC_UnitAction
   syncUnitData("BUnitActionDeath::setState unitID", pUnit->getID().asLong());
   syncUnitData("BUnitActionDeath::setState state", state);
   #endif

   switch (state)
   {
      case cStateDone:
      {
         //DCP 07/24/07: It's kosher to call ::destroy here because the unit is already
         //dead.  Plus, it may break things to not do that.
         pUnit->destroy();
         break;
      }

      case cStateWait:
      {
         //XXXHalwes - 8/13/2007 - First pass flood infection on death.
         // If killing entity is a flood infector form
         /*BEntity* pKillingEntity = gWorld->getUnit(mKillingEntity);
         if (pKillingEntity && (pKillingEntity->getProtoID() == gDatabase.getPOIDFloodInfector()))
         {
            // If we're an infantry unit then we can be infected
            const BProtoObject* pDyingProtoObject = pUnit->getProtoObject();
            if (pDyingProtoObject && pDyingProtoObject->isType(gDatabase.getOTIDInfantry()))
            {               
               // We're being killed by a flood infector so we need to spawn the correct infected form
               BObjectCreateParms infectedSquadParms;
               infectedSquadParms.mPlayerID = pKillingEntity->getPlayerID();
               infectedSquadParms.mStartBuilt = true; //?
               //infectedSquadParms.mProtoObjectID = ;               
               infectedSquadParms.mPosition = pUnit->getPosition();
               infectedSquadParms.mForward = pUnit->getForward();
               infectedSquadParms.mForward.y = 0.0f;
               if (!infectedSquadParms.mForward.safeNormalize())
                  infectedSquadParms.mForward = cZAxisVector;
               infectedSquadParms.mRight = XMVector3Cross(cYAxisVector, infectedSquadParms.mForward);

               const BPlayer* pDyingPlayer = pUnit->getPlayer();
               if (pDyingPlayer && (pDyingPlayer->getCivID() == gDatabase.getCivID("UNSC")))
               {
                  infectedSquadParms.mProtoSquadID = gDatabase.getProtoSquad("fld_inf_infectedMarineSingle_01");                  
               }
               else if (pDyingPlayer && (pDyingPlayer->getCivID() == gDatabase.getCivID("Covenant")))
               {
                  infectedSquadParms.mProtoSquadID = gDatabase.getProtoSquad("fld_inf_infectedEliteSingle_01");
               }

               // If infected squad created correctly then kill the infection form
               BSquad* pInfectedSquad = gWorld->createSquad(infectedSquadParms);
               if (pInfectedSquad)
               {
                  pKillingEntity->kill(true);
               }
            }
         }*/

         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if (pProtoObject)
         {
            BProtoSquadID deathSpawnSquad = pProtoObject->getDeathSpawnSquad();
            if (deathSpawnSquad != cInvalidProtoSquadID)
            {
               pUnit->doDeathSpawnSquad();
            }
         }

         //======================================================================================
         // SLB: Reset bounding box hack to work around a bug. Remove this when the bug is fixed.
         BVisual *pVisual = pUnit->getVisual();
         if (pVisual)
         {
            if (!getFlagDoingFatality())
               pUnit->computeAnimation();
            pVisual->resetCombinedBoundingBox();
            pVisual->computeCombinedBoundingBox();
            pUnit->updateBoundingBox();
         }
         //======================================================================================

         pUnit->setFlagObscurable(false);

         // If a building has either DeathFadeTime or DeathFadeDelay time specified - start the fading timer
         if ((pUnit->getProtoObject()->getObjectClass() == cObjectClassBuilding) &&
            ((pUnit->getProtoObject()->getDeathFadeTime() != cDefaultDeathFadeTime) || (pUnit->getProtoObject()->getDeathFadeDelayTime() != cDefaultDeathFadeDelayTime)))
         {
            setState(cStateFading);
         }
         else if (gConfig.isDefined(cConfigEnableCorpses) && (pUnit->getProtoObject()->getObjectClass() != cObjectClassBuilding) &&
             !pUnit->getProtoObject()->isType(gDatabase.getOTIDGatherable()) && !pUnit->getProtoObject()->getFlagNoCorpse() && !pUnit->getFlagNoCorpse())
         {
            // We don't want corpses to remain visible, we want them to dopple when appropriate, otherwise it could give away enemy troop positions.
            pUnit->setFlagRemainVisible(false);
            //pUnit->setFlagDopples(true);
            //pUnit->changeOwner(0);
            #ifdef SYNC_UnitAction
            syncUnitData("BUnitActionDeath::setState registerCorpse", pUnit->getID().asLong());
            #endif
            // Register the unit with the corpse manager. It'll tell us when it's time to bury it.
            gCorpseManager.registerCorpse(pUnit->getID());
            //pUnit->revealPosition(2.0f);
         }
         else
         {
            #ifdef SYNC_UnitAction
            syncUnitData("BUnitActionDeath::setState", pUnit->getID().asLong());
            #endif
            setState(cStateDone);
         }
         break;
      }

      case cStateFading:
      {
         pUnit->setFlagFadeOnDeath(true);
         pUnit->setLifespan(0, true);
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionDeath::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   #ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionDeath::update unitID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionDeath::update mState", mState);
   #endif

   switch (mState)
   {
      case cStateNone:
      {
         // SLB: This is to prevent objects masquerading as units from triggering the assert a few lines below (I'm looking at you "sys_thrownobject").
         if (!pUnit->getProtoObject() || (pUnit->getProtoObject()->getProtoVisualIndex() == -1))
         {
            setState(cStateDone);
            break;
         }

         if (!grabControllers())
            break;

         if ((pUnit->getProtoObject()->getObjectClass() == cObjectClassBuilding) && (pUnit->getBuildPercent() < 1.0f))
         {
            pUnit->undoBuildingTerrainAlpha();
            setState(cStateDone);
            break;
         }

         if (getFlagDoingFatality())
         {
            pUnit->setAnimationEnabled(false, true);
            mDeathAnimType = pUnit->getAnimationType(cActionAnimationTrack);
         }
         else
         {
            if (pUnit->isAnimationLocked())
               pUnit->unlockAnimation(); // Death overrides everything

            long deathAnimType = -1;
            if (mKillingWeaponType != -1)
            {
               BWeaponType *pWeaponType = gDatabase.getWeaponTypeByID(mKillingWeaponType);
               if (pWeaponType != NULL)
               {
                  deathAnimType = pWeaponType->getDeathAnimType();

                  BVisual* pVisual = pUnit->getVisual();
                  if (pVisual)
                  {
//-- FIXING PREFIX BUG ID 4963
                     const BVisualAnimationData& animData = pVisual->getAnimationData(cActionAnimationTrack, deathAnimType);
//--
                     if (animData.mDuration <= cFloatCompareEpsilon)
                     {
                        deathAnimType = -1;
                     }
                  }
               }
            }

            pUnit->setAnimationEnabled(true, true);
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateDeath, deathAnimType);
            pUnit->computeAnimation();
            BASSERT((pUnit->getAnimationState() == BObjectAnimationState::cAnimationStateDeath) && pUnit->isAnimationApplied());
            mDeathAnimType = pUnit->getAnimationType(cActionAnimationTrack);
         }

         // Set up air unit crash characteristics
         if (pUnit->isType(gDatabase.getObjectType("Flying")))
         {
            mCrashRollAccel = getRandRangeFloat(cSimRand, -4.0f, 4.0f);
            mCrashPitchAccel = getRandRangeFloat(cSimRand, -1.0f, 1.0f);
         }

         //--If there is no death anim to play, then we're done
         float animLen=getFlagDoingFatality() ? 0.0f : pUnit->getAnimationDuration(cActionAnimationTrack);
         #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionDeath::update animLen", animLen);
         #endif
         // If doing fatality, the fatality anim is already complete, so just go to wait state
         if (getFlagDoingFatality())
         {
            setState(cStateWait);
         }
         // If there is no death anim (just make the unit vanish)
         else if((animLen == 0.0f && !pUnit->isType(gDatabase.getObjectType("Flying"))))
         {
            setState(cStateDone);
         }
         else
         {
            pUnit->lockAnimation(animLen, false);
            setState(cStateWorking);
         }

         break;
      }

      case cStateWorking:
      {

         if (pUnit->isType(gDatabase.getObjectType("Flying")))
         {
            float sizeSquared = pUnit->getProtoObject()->getObstructionRadius() * pUnit->getProtoObject()->getObstructionRadius();
            // Crashing to the ground (ballistic trajectory)
            BVector g = cYAxisVector * -1.0f * 9.8f;
            BVector incrementalVelocity = g * elapsed;
            BVector newVelocity = pUnit->getVelocity() + incrementalVelocity;
            BVector newPosition = pUnit->getPosition() + (pUnit->getVelocity() * elapsed) + (0.5f * g * elapsed * elapsed);

            pUnit->setVelocity(newVelocity);
            #ifdef SYNC_Unit
               syncUnitData("BUnitActionDeath::update", newPosition);
            #endif
            pUnit->setPosition(newPosition);

            // Alter orientation as necessary
            // Roll
            float maxRollRate = 40.0f / sizeSquared; // rad/sec
            float goalRollRate = maxRollRate;

            if (mCrashRollAccel < 0.0f) // Correct for direction of roll
               goalRollRate*= -1.0f;

            // Now that we have a goal roll rate, apply roll acceleration
            float goalRollRateDelta = goalRollRate - mRollRate;
            float maxRollRateDelta = fabs(mCrashRollAccel) * elapsed;

            if (fabs(goalRollRateDelta) < maxRollRateDelta) // We can match goal roll rate now
               mRollRate = goalRollRate;
            else // Otherwise, apply max roll rate acceleration (in the proper direction) to mRollRate
            {
               if (goalRollRateDelta < 0.0f)
                  maxRollRateDelta *= -1.0f;
               mRollRate += maxRollRateDelta;
            }

            // Calculate new roll angle
            float rollAngle = mRollRate * elapsed;
            mpOwner->roll(rollAngle);

            // Pitch
            float maxPitchRate = 10.0f / sizeSquared; // rad/sec
            float goalPitchRate = maxPitchRate;

            if (mCrashPitchAccel < 0.0f) // Correct for direction of pitch
               goalPitchRate*= -1.0f;

            // Now that we have a goal pitch rate, apply pitch acceleration
            float goalPitchRateDelta = goalPitchRate - mPitchRate;
            float maxPitchRateDelta = fabs(mCrashPitchAccel) * elapsed;

            if (fabs(goalPitchRateDelta) < maxPitchRateDelta) // We can match goal pitch rate now
               mPitchRate = goalPitchRate;
            else // Otherwise, apply max pitch rate acceleration (in the proper direction) to mPitchRate
            {
               if (goalPitchRateDelta < 0.0f)
                  maxPitchRateDelta *= -1.0f;
               mPitchRate += maxPitchRateDelta;
            }

            // Calculate new pitch angle
            float pitchAngle = mPitchRate * elapsed;
            mpOwner->pitch(pitchAngle);


            // Did we hit yet?
            checkForImpact();
         }

         break;
      }

      case cStateWait:
         break;

      case cStateFading:
      {
         //=================================================================================

         if (pUnit->getFlagIsFading() && pUnit->isFullyFaded())
         {
            #ifdef SYNC_UnitAction
               syncUnitActionCode("BUnitActionDeath::update fullyFaded");
            #endif
            setState(cStateDone);
         }
         break;
      }

      default:
         break;
   }
   
   return (true);
} 

//==============================================================================
//==============================================================================
void BUnitActionDeath::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (eventType)
   {
      case BEntity::cEventAnimChain:
      case BEntity::cEventAnimLoop:
      case BEntity::cEventAnimEnd:
      {
         //-- If our death anim just looped, then we're done
         //E3: DCP 07/01/07: Make sure we catch the death anim.
         bool bCrashingAirUnit = false;
         if (pUnit->getProtoObject()->getMovementType() == cMovementTypeAir)
            bCrashingAirUnit = true;

         if ((senderID == mpOwner->getID()) &&
            (data2 == cActionAnimationTrack) &&
            (!bCrashingAirUnit) &&
            (((long)data) == mDeathAnimType))
         {
            #ifdef SYNC_UnitAction
            syncUnitData("BUnitActionDeath::notify animLoop", pUnit->getID().asLong());
            #endif
            pUnit->setAnimationEnabled(false, true);
            setState(cStateWait);
         }
         break;
      }

      case BEntity::cEventCorpseRemove:
         {
            #ifdef SYNC_UnitAction
               syncUnitData("BUnitActionDeath::notify corpseRemove", pUnit->getID().asLong());
            #endif

            pUnit->computeDopple();
            pUnit->setFlagDopples(false);
            setState(cStateFading); // Corpse manager is telling us to go away
            break;
         }
        

      default:
         break;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionDeath::checkForImpact()
{
   // Did we collide with the terrain?
   BVector intersectionPt;
   float terrainHeight;
   BVector ownPos = mpOwner->getPosition();

   bool blowUp = false;

   gTerrainSimRep.getHeightRaycast(ownPos, terrainHeight, true);
   blowUp = (terrainHeight >= ownPos.y);

   // -- Blow up --
   if (blowUp)
   {
      BUnit* pUnit = mpOwner->getUnit();
      ownPos.y = terrainHeight;
      #ifdef SYNC_Unit
         syncUnitData("BUnitActionDeath::checkForImpact", ownPos);
      #endif
      mpOwner->setPosition(ownPos);

      // air units that are metal, or set to shatter get destroyed here
      if ((pUnit->getProtoObject()->getMovementType() == cMovementTypeAir) &&
         (pUnit->getProtoObject()->getSurfaceType() == gDatabase.getSurfaceType("Metal")) || pUnit->getFlagShatterOnDeath())
      {
//         pUnit->explode();
         pUnit->destroy();
      }
      else
         setState(cStateWait);
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionDeath::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 4958
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   //We need them all.
   if (pUnit->getController(BActionController::cControllerOrient)->getActionID() != mID)
      return (false);
   if (pUnit->getController(BActionController::cControllerAnimation)->getActionID() != mID)
      return (false);

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionDeath::grabControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Take them all.
   if (!pUnit->grabController(BActionController::cControllerOrient, this, getOppID()))
      return (false);
   if (!pUnit->grabController(BActionController::cControllerAnimation, this, getOppID()))
   {
      pUnit->releaseController(BActionController::cControllerOrient, this);
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionDeath::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
void BUnitActionDeath::setOppID(BUnitOppID oppID)
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
uint BUnitActionDeath::getPriority() const
{
//-- FIXING PREFIX BUG ID 4959
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   const BUnitOpp* pOpp=pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);
   return (pOpp->getPriority());
}

//==============================================================================
//==============================================================================
bool BUnitActionDeath::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mCrashRollAccel);
   GFWRITEVAR(pStream, float, mRollRate);
   GFWRITEVAR(pStream, float, mCrashPitchAccel);
   GFWRITEVAR(pStream, float, mPitchRate);
   GFWRITEVAR(pStream, BEntityID, mKillingEntity);
   GFWRITEVAR(pStream, long, mKillingWeaponType);
   GFWRITEVAR(pStream, long, mDeathAnimType);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEBITBOOL(pStream, mbSetToDetonate);
   GFWRITEBITBOOL(pStream, mFlagDoingFatality);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDeath::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mCrashRollAccel);
   GFREADVAR(pStream, float, mRollRate);
   GFREADVAR(pStream, float, mCrashPitchAccel);
   GFREADVAR(pStream, float, mPitchRate);
   GFREADVAR(pStream, BEntityID, mKillingEntity);
   GFREADVAR(pStream, long, mKillingWeaponType);
   GFREADVAR(pStream, long, mDeathAnimType);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADBITBOOL(pStream, mbSetToDetonate);
   GFREADBITBOOL(pStream, mFlagDoingFatality);
   return true;
}
