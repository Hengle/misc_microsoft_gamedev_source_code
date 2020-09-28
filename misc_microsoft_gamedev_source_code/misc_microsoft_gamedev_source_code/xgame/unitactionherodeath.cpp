//==============================================================================
// unitactionherodeath.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionherodeath.h"
#include "entity.h"
#include "protoobject.h"
#include "unit.h"
#include "world.h"
#include "game.h"
#include "usermanager.h"
#include "user.h"
#include "syncmacros.h"
#include "visualitem.h"
//XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
//#include "physicsInfo.h"
//#include "physicsInfoManager.h"
//#include "physics.h"
#include "visual.h"
#include "configsgame.h"
#include "worldsoundmanager.h"
#include "weapontype.h"
#include "unitquery.h"
#include "gamesettings.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionHeroDeath, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionHeroDeath::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   grabControllers();

//-- FIXING PREFIX BUG ID 5055
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   
   mRegenRate = pUnit->getHPMax() / gDatabase.getHeroHPRegenTime();

   // Halwes - 10/16/2008 - Add the unit's squad to a new army and thus a new platoon.
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   BEntityIDArray armySquads;
   armySquads.add(pSquad->getID());

   BObjectCreateParms objectParms;
   objectParms.mPlayerID = pUnit->getPlayerID();
   BArmy* pArmy = gWorld->createArmy(objectParms);
   BASSERT(pArmy);
   if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
      pArmy->addSquads(armySquads, false);
   else
      pArmy->addSquads(armySquads);         

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionHeroDeath::connect unitID", pOwner->getID().asLong());
   #endif

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionHeroDeath::disconnect()
{
   BASSERT(mpOwner);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionHeroDeath::disconnect unitID", mpOwner->getID().asLong());
   #endif

   releaseControllers();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionHeroDeath::init()
{
   if (!BAction::init())
      return (false);

   mFlagConflictsWithIdle = true;
   mKillingEntity = cInvalidObjectID;
   mKillingWeaponType = -1;
   mRegenRate = 0.0f;
   mDeathAnimType = -1;
   
   //XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
   //mCrashRollAccel = 0.0f;
   //mRollRate = 0.0f;
   //mCrashPitchAccel = 0.0f;
   //mPitchRate = 0.0f;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionHeroDeath::setState(BActionState state)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   #ifdef SYNC_UnitAction
      syncUnitData("BUnitActionHeroDeath::setState unitID", pUnit->getID().asLong());
      syncUnitData("BUnitActionHeroDeath::setState state", state);
   #endif

   switch (state)
   {
      case cStateWorking:
         break;

      case cStateWait:
         {
            //======================================================================================
            // SLB: Reset bounding box hack to work around a bug. Remove this when the bug is fixed.
            BVisual* pVisual = pUnit->getVisual();
            if (pVisual)
            {
               //XXXHalwes - 5/2/2008 - No fatalities against heroes?
               //if (!getFlagDoingFatality())
                  pUnit->computeAnimation();
               pVisual->resetCombinedBoundingBox();
               pVisual->computeCombinedBoundingBox();
               pUnit->updateBoundingBox();
            }
            //======================================================================================            
         }
         break;

      case cStateDone:
      case cStateFailed:
         {
            if (state == cStateDone)
            {
               pUnit->completeOpp(mOppID, true);
            }
            else if (state == cStateFailed)
            {
               pUnit->completeOpp(mOppID, false);
            }

            // If we're not flying, make sure we're on the ground
            if (!pUnit->isPhysicsAircraft())
            {
               // Make sure hero is on the ground
               BVector pos = pUnit->getPosition();
               gTerrainSimRep.getHeightRaycast(pos, pos.y, true);
               pos.y += 1.0f; // Push us above the ground so we will tie to it correctly
               pUnit->getParentSquad()->doTeleport(pos);

               pUnit->tieToGround();
            }

            // Release our controllers
            releaseControllers();

            pUnit->setFlagIgnoreUserInput(false);
            pUnit->setFlagDontAutoAttackMe(false);
            pUnit->setLOSScalar(1.0f);
            //XXXHalwes - 5/5/2008 - Remove once we get a downed hero animation.
            pUnit->setAnimationEnabled(true, true);

            // Idle the anim.
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);            

            pUnit->setFlagDown(false);

            playSPCHeroRevivedSound();
         }
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionHeroDeath::update(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   if (!pUnit)
      return false;
   BSquad* pSquad = pUnit->getParentSquad();

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionHeroDeath::update unitID", mpOwner->getID().asLong());
      syncUnitActionData("BUnitActionHeroDeath::update mState", mState);
   #endif

   switch (mState)
   {
      case cStateNone:
         {
            if (!grabControllers())
               break;

            //XXXHalwes - 5/2/2008 - No fatalities against heroes?
            //if (getFlagDoingFatality())
            //{
            //   pUnit->enableAnimation(false);
            //}
            //else
            {
               if (pUnit->isAnimationLocked())
                  pUnit->unlockAnimation(); // Death overrides everything

               //XXXHalwes - 5/5/2008 - For now do a normal death animation until hero specific animations are used
               mDeathAnimType = -1;
               if (mKillingWeaponType != -1)
               {
//-- FIXING PREFIX BUG ID 5056
                  const BWeaponType* pWeaponType = gDatabase.getWeaponTypeByID(mKillingWeaponType);
//--
                  if (pWeaponType != NULL)
                  {
                     mDeathAnimType = pWeaponType->getDeathAnimType();
                  }
               }

               BVisual* pVisual = pUnit->getVisual();
               if (pVisual)
               {
//-- FIXING PREFIX BUG ID 5057
                  const BVisualAnimationData& animData = pVisual->getAnimationData(cActionAnimationTrack, mDeathAnimType);
//--
                  if (animData.mDuration == 0.0f)
                  {
                     mDeathAnimType = -1;
                  }
               }
               pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateDeath, mDeathAnimType);
               pUnit->computeAnimation();
               BASSERT((pUnit->getAnimationState() == BObjectAnimationState::cAnimationStateDeath) && pUnit->isAnimationApplied());
            }

            //XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
            //// Set up air unit crash characteristics
            //if (pUnit->isPhysicsAircraft())
            //{
            //   mCrashRollAccel = getRandRangeFloat(cSimRand, -4.0f, 4.0f);
            //   mCrashPitchAccel = getRandRangeFloat(cSimRand, -1.0f, 1.0f);
            //}

            // If there is no death anim to play, then we're done
            DWORD animLen = pUnit->getAnimationDurationDWORD(cActionAnimationTrack);
            #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionHeroDeath::update animLen", animLen);
            #endif

            // Fire hero downed sound
            playSPCHeroDownedSound();

            //XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
            //if (animLen == 0.0f && !pUnit->isPhysicsAircraft())
            if (animLen == 0)
            {
               setState(cStateWorking);
            }
            else
            {
               pUnit->lockAnimation(animLen, false);
               setState(cStateWait);
            }
         }
         break;

      case cStateWorking:
         {
            // Has hero regenerated to a high enough HP
            bool revive = false;
            float hp = pUnit->getHitpoints();
            float hpMax = pUnit->getHPMax();
            if (pUnit->getHPPercentage() >= gDatabase.getHeroPercentHPRevivalThreshhold())
            {
               revive = true;               
            }

            // Regen HP
            if (hp < hpMax)
            {
               pUnit->setHitpoints(Math::Min(hp + (mRegenRate * elapsed), hpMax));
            }            

            if (revive)
            {
               // Detect friendlies in area
               int numUnitsToSearchFor = 60;
               BEntityIDArray results(0, numUnitsToSearchFor);       
               BUnitQuery query(pUnit->getPosition(), gDatabase.getHeroRevivalDistance(), true);
               gWorld->getSquadsInArea(&query, &results, false);            
               uint numSquads = results.getSize();
               for (uint i = 0; i < numSquads; i++)
               {
                  BSquad* pTestSquad = gWorld->getSquad(results[i]);
                  if (!pTestSquad)
                     continue;

                  // Skip ourselves
                  if (pSquad == pTestSquad)
                     continue;

                  // Only test against units that are not alive or downed
                  if (!pTestSquad->isAlive() || pTestSquad->isDown())
                     continue;

                  // Make sure they are allies
                  if (gWorld->getTeamRelationType(pUnit->getTeamID(), pTestSquad->getTeamID()) != cRelationTypeAlly)
                     continue;

                  // Ignore other downed heroes
                  //if (pTestUnit->isType(gDatabase.getOTIDHeroDeath()) && (BUnitActionHeroDeath*)pTestUnit->getActionByType(BAction::cActionTypeUnitHeroDeath))
                  //   continue;

                  // MSC: I don't think we're doing fatalities on heroes, but they can be marked as a fatality victim, so clear the flag after we revive them so they can be attacked again. BUnit::isAttackable checks this flag
                  pUnit->setFlagFatalityVictim(false);

                  // Make sure we are not different units in the same squad
                  if (pSquad)
                  {
                     setState(cStateDone);
                     break;
                  }
               }
            }            
         }
         break;

      case cStateWait:
         {
            //XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
            //if (pUnit->isPhysicsAircraft())
            //{
            //   float sizeSquared = pUnit->getProtoObject()->getObstructionRadius() * pUnit->getProtoObject()->getObstructionRadius();
            //   // Crashing to the ground (ballistic trajectory)
            //   BVector g = cYAxisVector * -1.0f * 9.8f;
            //   BVector incrementalVelocity = g * elapsed;
            //   BVector newVelocity = pUnit->getVelocity() + incrementalVelocity;
            //   BVector newPosition = pUnit->getPosition() + (pUnit->getVelocity() * elapsed) + (0.5f * g * elapsed * elapsed);

            //   pUnit->setVelocity(newVelocity);
            //   pUnit->setPosition(newPosition);

            //   // Alter orientation as necessary
            //   // Roll
            //   float maxRollRate = 40.0f / sizeSquared; // rad/sec
            //   float goalRollRate = maxRollRate;

            //   if (mCrashRollAccel < 0.0f) // Correct for direction of roll
            //      goalRollRate*= -1.0f;

            //   // Now that we have a goal roll rate, apply roll acceleration
            //   float goalRollRateDelta = goalRollRate - mRollRate;
            //   float maxRollRateDelta = fabs(mCrashRollAccel) * elapsed;

            //   if (fabs(goalRollRateDelta) < maxRollRateDelta) // We can match goal roll rate now
            //      mRollRate = goalRollRate;
            //   else // Otherwise, apply max roll rate acceleration (in the proper direction) to mRollRate
            //   {
            //      if (goalRollRateDelta < 0.0f)
            //         maxRollRateDelta *= -1.0f;
            //      mRollRate += maxRollRateDelta;
            //   }

            //   // Calculate new roll angle
            //   float rollAngle = mRollRate * elapsed;
            //   mpOwner->roll(rollAngle);

            //   // Pitch
            //   float maxPitchRate = 10.0f / sizeSquared; // rad/sec
            //   float goalPitchRate = maxPitchRate;

            //   if (mCrashPitchAccel < 0.0f) // Correct for direction of pitch
            //      goalPitchRate*= -1.0f;

            //   // Now that we have a goal pitch rate, apply pitch acceleration
            //   float goalPitchRateDelta = goalPitchRate - mPitchRate;
            //   float maxPitchRateDelta = fabs(mCrashPitchAccel) * elapsed;

            //   if (fabs(goalPitchRateDelta) < maxPitchRateDelta) // We can match goal pitch rate now
            //      mPitchRate = goalPitchRate;
            //   else // Otherwise, apply max pitch rate acceleration (in the proper direction) to mPitchRate
            //   {
            //      if (goalPitchRateDelta < 0.0f)
            //         maxPitchRateDelta *= -1.0f;
            //      mPitchRate += maxPitchRateDelta;
            //   }

            //   // Calculate new pitch angle
            //   float pitchAngle = mPitchRate * elapsed;
            //   mpOwner->pitch(pitchAngle);


            //   // Did we hit yet?
            //   checkForImpact();
            //}
         }
         break;

      default:
         break;
   }

   return (true);
} 

//==============================================================================
//==============================================================================
void BUnitActionHeroDeath::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (eventType)
   {
      case BEntity::cEventAnimChain:
      case BEntity::cEventAnimLoop:
      case BEntity::cEventAnimEnd:
         {
            // If our death anim just looped, then we're done
            //XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
            //bool bCrashingAirUnit = false;
            //if (pUnit->isPhysicsAircraft())
            //   bCrashingAirUnit = true;

            if ((senderID == mpOwner->getID()) &&
               (data2 == cActionAnimationTrack) &&
               //XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
               //(!bCrashingAirUnit) &&
               ((data == static_cast<DWORD>(mDeathAnimType)) || (data == static_cast<DWORD>(pUnit->getDeathAnim())) || (data == cAnimTypeDeath)))
            {
               #ifdef SYNC_UnitAction
                  syncUnitData("BUnitActionHeroDeath::notify animLoop", pUnit->getID().asLong());
               #endif
               //XXXHalwes - 5/5/2008 - Remove once we get a downed hero animation.
               pUnit->setAnimationEnabled(false, true);
               setState(cStateWorking);
            }
            break;
         }

      default:
         break;
   }
}

//XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
//==============================================================================
//==============================================================================
//void BUnitActionHeroDeath::checkForImpact()
//{
//   // Did we collide with the terrain?
//   BVector intersectionPt;
//   float terrainHeight;
//   BVector ownPos = mpOwner->getPosition();
//
//   bool blowUp = false;
//
//   gTerrainSimRep.getHeightRaycast(ownPos, terrainHeight, true);
//   blowUp = (terrainHeight >= ownPos.y);
//
//   // -- Blow up --
//   if (blowUp)
//   {
//      BUnit* pUnit = mpOwner->getUnit();
//      ownPos.y = terrainHeight;
//      mpOwner->setPosition(ownPos);
//
//      if ((pUnit->isPhysicsAircraft()) &&
//         (pUnit->getProtoObject()->getSurfaceType() == gDatabase.getSurfaceType("Metal")))
//      {
//         pUnit->explode();
//         pUnit->destroy();
//      }
//      else
//         setState(cStateWait);
//   }
//}

//==============================================================================
//==============================================================================
bool BUnitActionHeroDeath::validateControllers() const
{
   // Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 5060
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
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
bool BUnitActionHeroDeath::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
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
void BUnitActionHeroDeath::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
void BUnitActionHeroDeath::setOppID(BUnitOppID oppID)
{
   if (mpOwner)
   {
      BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }

   mOppID = oppID;
}

//==============================================================================
//==============================================================================
uint BUnitActionHeroDeath::getPriority() const
{
//-- FIXING PREFIX BUG ID 5061
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   const BUnitOpp* pOpp = pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);

   return (pOpp->getPriority());
}

//==============================================================================
//==============================================================================
bool BUnitActionHeroDeath::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BEntityID, mKillingEntity);
   GFWRITEVAR(pStream, long, mKillingWeaponType);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);      
   GFWRITEVAR(pStream, float, mRegenRate);
   GFWRITEVAR(pStream, int, mDeathAnimType);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionHeroDeath::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BEntityID, mKillingEntity);
   GFREADVAR(pStream, long, mKillingWeaponType);
   GFREADVAR(pStream, BUnitOppID, mOppID);      
   GFREADVAR(pStream, float, mRegenRate);
   GFREADVAR(pStream, int, mDeathAnimType);
   return true;
}

//==============================================================================
// Play the SPC VoG sound when a hero goes down
//==============================================================================
void BUnitActionHeroDeath::playSPCHeroDownedSound()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   if (!pUnit)
      return;

   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (pSettings)
   {
      long gameType = -1;
      pSettings->getLong(BGameSettings::cGameType, gameType);
      if (gameType == BGameSettings::cGameTypeCampaign)
      {
         BProtoObjectID protoID = pUnit->getProtoID();
         // Forge
         if ((protoID == gDatabase.getPOIDForge()) || (protoID == gDatabase.getPOIDForgeWarthog()))
         {
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGHeroDownForge);
         }
         // Anders
         else if (protoID == gDatabase.getPOIDAnders())
         {
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGHeroDownAnders);
         }
         // Spartans
         else if ((protoID == gDatabase.getPOIDSpartanMG()) || (protoID == gDatabase.getPOIDSpartanRocket()) || (protoID == gDatabase.getPOIDSpartanSniper()))
         {
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGHeroDownSpartan);
         }  
      }
   }
}

//==============================================================================
// Play the SPC VoG sound when a hero is revived
//==============================================================================
void BUnitActionHeroDeath::playSPCHeroRevivedSound()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   if (!pUnit)
      return;

   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (pSettings)
   {
      long gameType = -1;
      pSettings->getLong(BGameSettings::cGameType, gameType);
      if (gameType == BGameSettings::cGameTypeCampaign)
      {
         BProtoObjectID protoID = pUnit->getProtoID();
         // Forge
         if ((protoID == gDatabase.getPOIDForge()) || (protoID == gDatabase.getPOIDForgeWarthog()))
         {
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGHeroReviveForge);
         }
         // Anders
         else if (protoID == gDatabase.getPOIDAnders())
         {
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGHeroReviveAnders);
         }
         // Spartans
         else if ((protoID == gDatabase.getPOIDSpartanMG()) || (protoID == gDatabase.getPOIDSpartanRocket()) || (protoID == gDatabase.getPOIDSpartanSniper()))
         {
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGHeroReviveSpartan);
         }  
      }
   }
}
