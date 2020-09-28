//==============================================================================
// unitactionplasmashieldgen.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionplasmashieldgen.h"
#include "player.h"
#include "unit.h"
#include "user.h"
#include "usermanager.h"
#include "grannyinstance.h"
#include "world.h"
#include "actionmanager.h"
#include "ParticleSystemManager.h"
#include "unitquery.h"
#include "grannymodel.h"
#include "grannymanager.h"
#include "soundmanager.h"
#include "tactic.h"
#include "team.h"
#include "techtree.h"
#include "unitactionbuilding.h"
#include "game.h"
#include "prototech.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionPlasmaShieldGen, 5, &gSimHeap);

const float         BUnitActionPlasmaShieldGen::mHitrate = 200.0f;            // Amount to ramp the shield intensity up per second
const DWORD         BUnitActionPlasmaShieldGen::mHitTime = 500;               // Number of seconds to ramp hit over
//const DWORD         BUnitActionPlasmaShieldGen::mHitBreak = 500;              // Number of seconds to wait before playing hit again
const float         BUnitActionPlasmaShieldGen::mMaxHitIntensity = 100.0f;    // Max intensity allowed

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::ShieldInfo::reset()
{
   mShieldProtoId = cInvalidProtoObjectID;
   mShieldID = cInvalidObjectID;
   mNumShields = 1;
   mFlagPrimary = false;
   mFlagShieldUp = false;
   mFlagRaising = false;
   mBldgToShieldMap.clear();
}

//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::connect(BEntity* pOwner, BSimOrder *pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return false;

   // get the correct proto id for the shield
   BASSERT(pOwner);
   mShieldInfo.mShieldProtoId = mpProtoAction->getProtoObject();

   // setup the mesh mask bits
   BPlayer* pPlayer = mpOwner->getPlayer();
   BASSERT(pPlayer);
//-- FIXING PREFIX BUG ID 5082
   const BProtoObject* pShieldProto = pPlayer->getProtoObject(mShieldInfo.mShieldProtoId);
//--
   BASSERT(pShieldProto);
//-- FIXING PREFIX BUG ID 5083
   const BProtoVisual* pShieldProtoVisual = gVisualManager.getProtoVisual(pShieldProto->getProtoVisualIndex(), true);
//--
   BASSERT(pShieldProtoVisual);
   BASSERT(pShieldProtoVisual->getDefaultModel());
//-- FIXING PREFIX BUG ID 5084
   const BGrannyModel* pGrannyModel = gGrannyManager.getModel(pShieldProtoVisual->getDefaultModel()->mAsset.mAssetIndex, true);
//--
   BASSERT(pGrannyModel);

   // Look for a primary shield generator if there is one
   mPrimary = getPrimaryShieldGen();
   if (mPrimary == cInvalidObjectID)
   {
      mShieldInfo.mFlagPrimary = true;
      mPrimary = mpOwner->getUnit()->getID();
   }
   // There is, tell it about us
   else
   {
      BUnit* pPrimary = gWorld->getUnit(mPrimary);
      BASSERT(pPrimary);

      BUnitActionPlasmaShieldGen* pAction = (BUnitActionPlasmaShieldGen*)pPrimary->getActionByType(BAction::cActionTypeUnitPlasmaShieldGen);
      BASSERT(pAction);

      mShieldInfo.mFlagPrimary = false;
      pAction->addSecondaryShieldGen();
   }

   BASSERT(mPrimary != cInvalidObjectID);

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::disconnect()
{
   // If there are other shield generators, mark one of them as the primary
   if (mShieldInfo.mFlagPrimary)
   {
      findNewPrimary();
   }
   // Tell the primary we're going away
   else if (mPrimary != cInvalidObjectID)
   {
      BUnit* pPrimary = gWorld->getUnit(mPrimary);
      
      // pPrimary can be NULL if we're resigning/restarting
      if (pPrimary)
      {
         BUnitActionPlasmaShieldGen* pAction = (BUnitActionPlasmaShieldGen*)pPrimary->getActionByType(BAction::cActionTypeUnitPlasmaShieldGen);
         if (pAction)
            pAction->removeSecondaryShieldGen();
      }
   }

   // The subshield is already removed if we're the primary
   BUnit* pOwner = mpOwner->getUnit();
   if (pOwner && pOwner->getParentSquad())
   {
      if (findSquad(pOwner->getParentSquad()->getID()) != -1)
      {
         removeSubShield(mpOwner->getID());
      }
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::init()
{
   if (!BAction::init()) 
      return(false);

   mCachedLocalPlayerId = -1;
   mAttackWaitTime = 0.0f;
   mPrimary = cInvalidObjectID;
   mShieldInfo.reset();

   // Pre-calculate shield anims
   mShieldAnims[3] = gVisualManager.getAnimType("100Health");
   mShieldAnims[2] = gVisualManager.getAnimType("75Health");
   mShieldAnims[1] = gVisualManager.getAnimType("50Health");
   mShieldAnims[0] = gVisualManager.getAnimType("25Health");

   mAnimIdx = -1;

   mCurrentHitVal = 1.0f;
   mHitStartTime = 0;
   mPlayingHit = false;

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::update(float elapsed)
{
   if (mShieldInfo.mFlagPrimary)
      return primaryUpdate(elapsed);

   return secondaryUpdate(elapsed);
}


//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::primaryUpdate(float elapsed)
{
   BASSERT(mpOwner);
   BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pShieldGen);
   BASSERT(mpProtoAction);

   if (!pShieldGen->isAlive())
      return false;

//-- FIXING PREFIX BUG ID 5087
   const BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
//--
   BASSERT(pBase);

   if (!pBase)
      return true;

   // Update ramp
   if (mPlayingHit)
   {
      if (gWorld->getGametime() - mHitStartTime < mHitTime)
         mCurrentHitVal += elapsed * mHitrate;
      else
         mCurrentHitVal -= elapsed * mHitrate;

      if (mCurrentHitVal > mMaxHitIntensity)
         mCurrentHitVal = mMaxHitIntensity;

      if (mCurrentHitVal < 1.0f)
         mCurrentHitVal = 1.0f;

      if (gWorld->getGametime() - mHitStartTime > mHitTime * 2)
      {
         mPlayingHit = false;
         mCurrentHitVal = 1.0f;
      }

      BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);
      if (pShield)
         pShield->setHighlightIntensity(mCurrentHitVal);

      long count = mShieldInfo.mBldgToShieldMap.getNumber();
      for (long i = 0; i < count; i++)
      {
         BSquad* pShieldSquad = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[i].second);
         if (pShieldSquad && pShieldSquad->getLeaderUnit())
         {
            pShieldSquad->getLeaderUnit()->setHighlightIntensity(mCurrentHitVal);
         }
      }
   }

   switch (mState)
   {
      // shield is down
      case cStateNone:
      {  
         // If the base is under attack, keep the timer reset
         BUnitActionBuilding* pBuildingAction=(BUnitActionBuilding*)pShieldGen->getActionByType(BAction::cActionTypeUnitBuilding);
         BASSERT(pBuildingAction);

         if (pBase->getNumberAttackingUnits() > 0)
         {
            BPlayer* pPlayer = mpOwner->getPlayer();
            BASSERT(pPlayer);
//-- FIXING PREFIX BUG ID 5085
            const BProtoObject* pShieldProto = pPlayer->getProtoObject(mShieldInfo.mShieldProtoId);
//--
            BASSERT(pShieldProto);
            pShieldGen->setRecharge(false, mShieldInfo.mShieldProtoId, pShieldProto->getBuildPoints());

            mAttackWaitTime = mpProtoAction->getDuration();
            break;
         }
         else if (mAttackWaitTime > 0.0f)
         {
            BPlayer* pPlayer = mpOwner->getPlayer();
            BASSERT(pPlayer);
//-- FIXING PREFIX BUG ID 5086
            const BProtoObject* pShieldProto = pPlayer->getProtoObject(mShieldInfo.mShieldProtoId);
//--
            BASSERT(pShieldProto);
            pShieldGen->setRecharge(false, mShieldInfo.mShieldProtoId, pShieldProto->getBuildPoints());

            mAttackWaitTime -= elapsed;
            break;
         }

         // check rebuild timer
         float remaining;
         if (!pShieldGen->getRecharging(false, mShieldInfo.mShieldProtoId, &remaining))
         {
            pBuildingAction->clearDescriptionOverride();
            if (pShieldGen->getPlayerID() == gUserManager.getPrimaryUser()->getPlayerID())
               gUserManager.getPrimaryUser()->setFlagCommandMenuRefresh(true);
            else
               gUserManager.getSecondaryUser()->setFlagCommandMenuRefresh(true);
            attemptBringUp();
         }
         else
         {
            BUString desc;
            BUString recharge = gDatabase.getLocStringFromID(mpProtoAction->getCount());

            // FIXES PHX - 18817
            BUString rechargeFmtString = gDatabase.getLocStringFromID(26006);
            desc.locFormat(rechargeFmtString, recharge.getPtr(), Math::FloatToIntTrunc(remaining) + 1);
            //desc.format(L"%s %ds", recharge.getPtr(), Math::FloatToIntTrunc(remaining) + 1);
            pBuildingAction->setDescriptionOverride(desc);
            if (pShieldGen->getPlayerID() == gUserManager.getPrimaryUser()->getPlayerID())
               gUserManager.getPrimaryUser()->setFlagCommandMenuRefresh(true);
            else
               gUserManager.getSecondaryUser()->setFlagCommandMenuRefresh(true);
         }
         break;
      }

      // shield is up
      case cStateWorking:
      {
         updatePlayerVisibilityAndSelection(mShieldInfo.mShieldID);

         // check that the shield is up. if not, kill it
         BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);
         if (!pShield || !pShield->isAlive())
         {
            takeShieldDown();
            break;
         }

         // quick and dirty way to guarantee new buildings will be invulnerable, check this each frame
         updateCoveredObjectsVulnerability(true);

         // Update HP of main shield and any subshield to reflect their underlying building's health
         BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
         BASSERT(pShieldGen);

         if (pShieldGen)
         {
            BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
            BASSERT(pBase);

            if (pBase)
               pShield->setHitpoints(pBase->getHPPercentage());
         }

         long count = mShieldInfo.mBldgToShieldMap.getNumber();
         for (long i = 0; i < count; i++)
         {
            BUnit* pBldgUnit = gWorld->getUnit(mShieldInfo.mBldgToShieldMap[i].first);
            BSquad* pShieldSquad = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[i].second);
            if (pShieldSquad)
            {
               BUnit* pShieldUnit = pShieldSquad->getLeaderUnit();
               if (pBldgUnit && pShieldUnit)
               {
                  pShieldUnit->setHitpoints(pBldgUnit->getHPPercentage());
               }
            }
         }

         updateSockets();

         // If we're playing the raise anim, don't recompute anim
         if (mShieldInfo.mFlagRaising)
            break;

         // Update anim to reflect current health
         int idx = Math::Max(Math::FloatToIntTrunc(4.0f * pShield->getShieldpoints() / pShield->getSPMax()) - 1, 0);

         if (idx != mAnimIdx)
         {
            long animType = mShieldAnims[idx];
            pShield->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, animType, true);

            long count = mShieldInfo.mBldgToShieldMap.getNumber();
            for (long i = 0; i < count; i++)
            {
               BSquad* pSquad = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[i].second);
               if (pSquad && pSquad->getLeaderUnit())
                  pSquad->getLeaderUnit()->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, animType, true);
            }
            mAnimIdx = idx;
         }
         break;
      }
   }

   return (true);
}


//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::secondaryUpdate(float elapsed)
{
   BASSERT(mPrimary != cInvalidObjectID);

   BUnit* pPrimary = gWorld->getUnit(mPrimary);
   BASSERT(pPrimary);

//-- FIXING PREFIX BUG ID 5090
   const BUnitActionPlasmaShieldGen* pAction = (BUnitActionPlasmaShieldGen*)pPrimary->getActionByType(BAction::cActionTypeUnitPlasmaShieldGen);
//--
   BASSERT(pAction);

   // Keep us in sync with primary shield
   if (mShieldInfo.mFlagShieldUp != pAction->isShieldUp())
   {
      if (!mShieldInfo.mFlagShieldUp)
         attemptBringUp();
      else
         takeShieldDown();
   }

   BUnit* pOwner = mpOwner->getUnit();
   BASSERT(pOwner);

   BUnitActionBuilding* pBuildingAction=(BUnitActionBuilding*)pOwner->getActionByType(BAction::cActionTypeUnitBuilding);
   BASSERT(pBuildingAction);

   // Update our recharge text if primary is recharging
   if (pAction->getState() == cStateNone)
   {
      // Notify that we've updated
      gWorld->notify(BEntity::cEventTrainPercent, mpOwner->getID(), 0, 0);

      float remaining;
      if (!pPrimary->getRecharging(false, mShieldInfo.mShieldProtoId, &remaining))
      {
         pBuildingAction->clearDescriptionOverride();
         if (pPrimary->getPlayerID() == gUserManager.getPrimaryUser()->getPlayerID())
            gUserManager.getPrimaryUser()->setFlagCommandMenuRefresh(true);
         else
            gUserManager.getSecondaryUser()->setFlagCommandMenuRefresh(true);
      }
      else
      {
         BUString desc;
         BUString recharge = gDatabase.getLocStringFromID(mpProtoAction->getCount());
         desc.format(L"%s %ds", recharge.getPtr(), Math::FloatToIntTrunc(remaining) + 1);
         pBuildingAction->setDescriptionOverride(desc);
         if (pPrimary->getPlayerID() == gUserManager.getPrimaryUser()->getPlayerID())
            gUserManager.getPrimaryUser()->setFlagCommandMenuRefresh(true);
         else
            gUserManager.getSecondaryUser()->setFlagCommandMenuRefresh(true);
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::setState(BActionState state)
{
   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!mpOwner)
      return;

//-- FIXING PREFIX BUG ID 5091
   const BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
//--

   switch (eventType)
   {
      case BEntity::cEventDamaged:
      {
         mShieldInfo.mFlagRaising = false;

         // our event listener gets purged before we get the killed event for the shield, so we need to check it here. sadness
         if (senderID == mShieldInfo.mShieldID && mShieldInfo.mFlagPrimary)
         {
            BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);
            if (!pShield || !pShield->isAlive() || pShield->getHitpoints() < cFloatCompareEpsilon)
               takeShieldDown();
         }
         else if (senderID != mShieldInfo.mShieldID)
         {
            // Subshield, apply damage to base shield
            BUnit* pAttacker = gWorld->getUnit(data);

            // Apply damage to base shield
            if (pAttacker && mShieldInfo.mShieldID != cInvalidObjectID)
            {
               float dmg = (float)data2;
               BDamageHelper::doDamageWithWeaponType(pShieldGen->getPlayerID(), pShieldGen->getTeamID(), mShieldInfo.mShieldID, NULL, dmg, -1, false, cInvalidVector, 1.0f, cInvalidVector, cInvalidObjectID);
            }
         }

         if (mShieldInfo.mFlagPrimary)
         {
            long count = mShieldInfo.mBldgToShieldMap.getNumber();
            for (long i = 0; i < count; i++)
            {
               BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);
               BSquad* pSubShield = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[i].second);
               if (pShield && pSubShield && pSubShield->getLeaderUnit())
               {
                  pSubShield->updateLastDamagedTime();

                  float newsp = pSubShield->getProtoObject()->getShieldpoints() * pShield->getSPPercentage();
                  if (newsp < 1.0f)
                     newsp = 1.0f;
                  pSubShield->getLeaderUnit()->setShieldpoints(newsp);
                  pSubShield->updateLastDamagedTime();
                  pSubShield->updateLastAttackedTime();
                  pSubShield->setFlagShieldDamaged(true);
               }
            }

            //if (!mPlayingHit && ((gWorld->getGametime() - mHitStartTime) >= (mHitBreak + mHitTime * 2)))
            {
               mPlayingHit = true;
               mHitStartTime = gWorld->getGametime();
            }
         }
         break;
      }
   }

   BAction::notify(eventType, senderID, data, data2);
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::updatePlayerVisibilityAndSelection(BEntityID shieldID, bool force)
{
   BUnit* pShield = gWorld->getUnit(shieldID);

   if (!pShield)
      return;

   BPlayer * pPlayer = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   BASSERT(pPlayer);
   if (mCachedLocalPlayerId == pPlayer->getID() && !force)
      return;

   mCachedLocalPlayerId = pPlayer->getID();

   // set the teams selection mask for the shield
   int numTeams = gWorld->getNumberTeams();
   BTeamID shieldTeamId = pShield->getTeamID();
   for (int i = 0; i < numTeams; ++i)
   {
      const BTeam* const pOtherTeam = gWorld->getTeam(i);
      if (pOtherTeam)
      {
         BRelationType teamRelation = gWorld->getTeamRelationType(shieldTeamId, pOtherTeam->getID());
         bool bIsAlly = (teamRelation == cRelationTypeAlly || teamRelation == cRelationTypeSelf);
         pShield->setTeamSelectionMask(pOtherTeam->getID(), !bIsAlly);
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::attemptBringUp()
{
   if (!mShieldInfo.mFlagPrimary)
      return;

   // we should never call this if we have a valid shield already
   BASSERT(mShieldInfo.mShieldID == cInvalidObjectID);

   // create the shield, then check for obstructions
//-- FIXING PREFIX BUG ID 5096
   const BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pShieldGen);

//-- FIXING PREFIX BUG ID 5097
   const BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
//--
   BASSERT(pBase);

   BEntityID shieldSquadId = gWorld->createEntity(mShieldInfo.mShieldProtoId, false, pShieldGen->getPlayerID(), pBase->getPosition(), pBase->getForward(), pBase->getRight(), true, false, false, cInvalidObjectID, cInvalidPlayerID, pShieldGen->getID());
//-- FIXING PREFIX BUG ID 5098
   const BSquad* pSquad = gWorld->getSquad(shieldSquadId);
//--
   if (pSquad && pSquad->getNumberChildren()>0)
      mShieldInfo.mShieldID = pSquad->getChild(0);
   BASSERT(mShieldInfo.mShieldID != cInvalidObjectID);
   
   // listen for its events
   BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);
   BASSERT(pShield);
   pShield->addEventListener(this);

   bringShieldUp();
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::bringShieldUp()
{
   if (!mShieldInfo.mFlagPrimary)
      return;

   BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);
   BASSERT(pShield);

//-- FIXING PREFIX BUG ID 5101
   const BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pShieldGen);

   BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
   BASSERT(pBase);

   // zero out the shield
   pShield->setShieldpoints(0);

   // Play create sound
   BPlayerID playerID = pShield->getPlayerID();
   if (playerID == gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() || (gGame.isSplitScreen() && playerID == gUserManager.getUser(BUserManager::cSecondaryUser)->getPlayerID()))
      gSoundManager.playCue(pShield->getProtoObject()->getSound(cObjectSoundCreate));

   // add an entity ref on the base
   if (mShieldInfo.mFlagPrimary)
      pBase->addEntityRef(BEntityRef::cTypeBaseShield, mShieldInfo.mShieldID, 0, 0);

   // Add entity ref to shield
   if (mShieldInfo.mFlagPrimary)
      pShield->addEntityRef(BEntityRef::cTypeAssociatedBase, pBase->getID(), 0, 0);

   pBase->setFlagCollidable(false);

   // Hide the base's HP bar
   pBase->getParentSquad()->setFlagHasHPBar(false);
   pBase->getParentSquad()->setDamageProxy(pShield->getParentSquad()->getID());

   pShield->getParentSquad()->setFlagHasHPBar(true);

   // Set our health percentage to the same as the base's (so we look visually like we just added a shield to the base)
   pShield->setHitpoints(pBase->getHPPercentage());

   pShield->setFlagCollidable(false); // Make sure that units can get inside the shield
   pShield->setFlagBuilt(true);
   pShield->updateObstruction();

   // set us into the correct state and make sure selection is setup correctly
   updatePlayerVisibilityAndSelection(mShieldInfo.mShieldID, true);
   setState(cStateWorking);

   mShieldInfo.mFlagShieldUp = true;

   updateSockets();

   // Do birth animation on all shields
   pShield->setAnimation(mID, BObjectAnimationState::cAnimationStateTrain, cAnimTypeTrain, true, false, -1, true);
   pShield->computeAnimation();
   BASSERT(pShield->isAnimationSet(BObjectAnimationState::cAnimationStateTrain, cAnimTypeTrain));

   long count = mShieldInfo.mBldgToShieldMap.getNumber();
   for (long i = 0; i < count; i++)
   {
      BSquad* pSquad = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[i].second);
      if (pSquad)
      {
         BUnit* pUnit = pSquad->getLeaderUnit();
         if (pUnit)
         {
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateTrain, cAnimTypeTrain, true, false, -1, true);
         }
      }
   }

   mShieldInfo.mFlagRaising = true;

   // Setup the render override callback
   BObject* pObject = pShield->getObject();
   BASSERT(pObject);
   
   if (pObject)
      pObject->setOverrideRenderCallback(BUnitActionPlasmaShieldGen::renderStatic);
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::updateSockets()
{
   BASSERT(mpOwner);
   BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pShieldGen);
   BASSERT(mpProtoAction);

   BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
   
   // We may not have a base if we're shutting down
   if (!pBase)
      return;

   // Update all sockets
   uint numEntityRefs = pBase->getNumberEntityRefs();
   for (uint i=0; i<numEntityRefs; i++)
   {
      BEntityRef* pRef = pBase->getEntityRefByIndex(i);
      if (pRef && pRef->mType == BEntityRef::cTypeAssociatedBuilding)
      {
         BEntity* pEntity = gWorld->getEntity(pRef->mID);
         if (pEntity)
         {
            // Get cTypeParentSocket ref of building and make sure it's valid--if it's not, it's the hotdrop or a turret or building right now, so no shield
            if (!pEntity->getProtoObject()->isType(gDatabase.getOTIDTurretSocket()) && !pEntity->getProtoObject()->isType(gDatabase.getOTIDTurretBuilding()) && 
                pEntity->getFirstEntityRefByType(BEntityRef::cTypeParentSocket) && pEntity->getFlagIsBuilt())
            {
               updateSocket(pRef->mID);
            }
         }         
      }
   }

   // Remove any defunct subshields
   for (long i = 0; i < mShieldInfo.mBldgToShieldMap.getNumber(); i++)
   {
      BuildingShieldPair p = mShieldInfo.mBldgToShieldMap[i];
      BEntity* pEntity = gWorld->getEntity(p.first);
      if (!pEntity)
      {
         removeSubShield(mShieldInfo.mBldgToShieldMap[i].first);
         i = 0;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::updateSocket(BEntityID targetID)
{
   BASSERT(mpOwner);
   BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pShieldGen);
   BASSERT(mpProtoAction);

   BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
   
   // We may not have a base if we're shutting down
   if (!pBase)
      return;

   if (findSquad(targetID) == -1)
   {
      addSubShield(targetID);
   }
   else
   {
      updateSubShield(targetID);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::addSubShield(BEntityID targetID)
{
   BASSERT(mPrimary != cInvalidObjectID);

   BASSERT(mShieldInfo.mFlagPrimary);

   BUnit* pPrimary = mpOwner->getUnit();
   BASSERT(pPrimary);

   BUnitActionPlasmaShieldGen* pAction = (BUnitActionPlasmaShieldGen*)pPrimary->getActionByType(BAction::cActionTypeUnitPlasmaShieldGen);
   BASSERT(pAction);

   // If primary shield is not up, bail
   if (!mShieldInfo.mFlagShieldUp)
      return;

   BUnit* pUnit = gWorld->getUnit(targetID);
   if (pUnit)
   {
      const BProtoObject* pProto = pUnit->getProtoObject();
      BASSERT(pProto);
      if (!pProto)
         return;

      long shieldType = pProto->getShieldType();
      BASSERT(shieldType != cInvalidObjectID);

      BASSERT(mpOwner);
      BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pShieldGen);

      BEntityID shieldID = gWorld->createEntity(shieldType, false, pShieldGen->getPlayerID(), pUnit->getPosition(), pUnit->getForward(), pUnit->getRight(), true, false, false, cInvalidObjectID, cInvalidPlayerID, pShieldGen->getID());

      mShieldInfo.mBldgToShieldMap.add(BuildingShieldPair(targetID, shieldID));

      // Add shield reference
      pUnit->addEntityRef(BEntityRef::cTypeBuildingShield, shieldID, 0, 0);

      pUnit->updateSimBoundingBox();
      pUnit->getParentSquad()->setFlagHasHPBar(false);
      pUnit->getParentSquad()->setDamageProxy(shieldID);

      BEntityID shieldUnitID = cInvalidObjectID;
      BSquad* pSquad = gWorld->getSquad(shieldID);
      BUnit* pShield = gWorld->getUnit(shieldUnitID);
      if (pSquad && pSquad->getNumberChildren()>0)
      {
         shieldUnitID = pSquad->getChild(0);
         pShield = pSquad->getLeaderUnit();

         // Set shield hp percentage
         pSquad->getLeaderUnit()->setHitpoints(pUnit->getHPPercentage());
      }
      BASSERT(shieldUnitID != cInvalidObjectID);

      // Add reference to base building to shield
      if (pShield)
         pShield->addEntityRef(BEntityRef::cTypeAssociatedBase, pUnit->getID(), 0, 0);

      updatePlayerVisibilityAndSelection(shieldUnitID, true);

      // Listen for damage on subshield
      BASSERT(pShield);
      if (!pShield)
         return;

      // Setup the render override callback
      BObject* pObject = pShield->getObject();
      BASSERT(pObject);
      
      if (pObject)
         pObject->setOverrideRenderCallback(BUnitActionPlasmaShieldGen::renderStatic);

      pShield->addEventListener(this);

      // Always have shieldpoints on subshields be the same as the main base (percentagewise)
      BUnit* pMainShield = gWorld->getUnit(mShieldInfo.mShieldID);
      if (pMainShield)
      {
         pShield->setShieldpoints(pShield->getProtoObject()->getShieldpoints() * pMainShield->getSPPercentage());
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::removeSubShield(BEntityID targetID)
{
   int index = findSquad(targetID);

   if (index != -1)
   {
      BSquad* pSquad = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[index].second);

      // pSquad can be gone if the building was destroyed
      if (pSquad)
      {
         pSquad->kill(false);

         // Remove the entity reference
         BEntity* pEntity = gWorld->getEntity(targetID);

         if (pEntity)
         {
            pEntity->removeEntityRef(BEntityRef::cTypeBuildingShield, mShieldInfo.mBldgToShieldMap[index].second);

            if (pEntity->getUnit() && pEntity->getUnit()->getParentSquad())
            {
               pEntity->getUnit()->getParentSquad()->setFlagHasHPBar(true);
               pEntity->getUnit()->getParentSquad()->setDamageProxy(cInvalidObjectID);
            }
         }
      }

      mShieldInfo.mBldgToShieldMap.erase(index);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::updateSubShield(BEntityID targetID)
{
   BObject* pObject = gWorld->getObject(targetID);
   if (!pObject)
      return;

   const BProtoObject* pProto = pObject->getProtoObject();
      BASSERT(pProto);
      if (!pProto)
         return;

   long shieldType = pProto->getShieldType();
   BASSERT(shieldType != cInvalidObjectID);

   BASSERT(mpOwner);
   BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pShieldGen);

   // Need to upgrade shield
   int index = findSquad(targetID);
   if (index != -1)
   {
      BSquad* pShield = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[index].second);
      if (pShield && pShield->getLeaderUnit() && shieldType != pShield->getLeaderUnit()->getProtoID())
      {
         BUnit* pUnit = gWorld->getUnit(targetID);

         // Remove reference for old object
         if (pUnit)
         {
            pUnit->removeEntityRef(BEntityRef::cTypeBuildingShield, pShield->getID());
         }

         // Remove event listener for old object
         pShield->removeEventListener(this);

         pShield->kill(false);
         
         if (pUnit)
         {
            BEntityID shieldID = gWorld->createEntity(shieldType, false, pShieldGen->getPlayerID(), pUnit->getPosition(), pUnit->getForward(), pUnit->getRight(), true, false, false, cInvalidObjectID, cInvalidPlayerID, pShieldGen->getID());

            int index = findSquad(targetID);
            if (index != -1)
               mShieldInfo.mBldgToShieldMap.erase(index);

            mShieldInfo.mBldgToShieldMap.add(BuildingShieldPair(targetID, shieldID));

            // Add reference for new object
            pUnit->addEntityRef(BEntityRef::cTypeBuildingShield, shieldID, 0, 0);

            // Add event listener for new object
            BEntity* pShield = gWorld->getEntity(shieldID);
            BASSERT(pShield);
            pShield->addEventListener(this);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::takeShieldDown()
{
   BASSERT(mPrimary != cInvalidObjectID);
   BUnit* pPrimary = gWorld->getUnit(mPrimary);

   //check to make sure the shield has not already been destroyed.
   if(mShieldInfo.mShieldID == cInvalidObjectID)
   {
      setState(cStateNone);
      return;
   }

   BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);

   // On occasion, the shield object is destroyed first, so check for that condition
   if (!pShield)
   {
      mShieldInfo.mShieldID = cInvalidObjectID;
      setState(cStateNone);
      return;
   }

   // make sure the shield is dead, kill immediately if we just finished fading
   // otherwise, we play the death animation
   if (pShield)
      pShield->kill(false);

   // unhide all the base objects
   if (mShieldInfo.mFlagPrimary)
      updateCoveredObjectsVulnerability(false);

   mShieldInfo.mFlagShieldUp = false;

   BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pShieldGen);

   BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
   if (!pBase)
      return;

   // clear the entity ref on the base
   if (mShieldInfo.mFlagPrimary)
      pBase->removeEntityRef(BEntityRef::cTypeBaseShield, mShieldInfo.mShieldID);

   // Restore the base's collision
   pBase->setFlagCollidable(pBase->getProtoObject()->getFlagCollidable());

   // Show the base's HP bar
   if (pBase->getParentSquad())
   {
      pBase->getParentSquad()->setFlagHasHPBar(true);
      pBase->getParentSquad()->setDamageProxy(cInvalidObjectID);
   }

   mShieldInfo.mShieldID = cInvalidObjectID;
   setState(cStateNone);

   // The shield generator can be destroyed first when shutting down, so check for that
   if (!mpOwner->isAlive())
      return;

   // reset the retry build timer
   const BProtoObject* pShieldProto = pShield->getProtoObject();
   BASSERT(pShieldProto);
   if (mShieldInfo.mFlagPrimary)
   {
      pShieldGen->setRecharge(false, mShieldInfo.mShieldProtoId, pShieldProto->getBuildPoints());
   }
   else if (pPrimary && pPrimary->isAlive())
   {
//-- FIXING PREFIX BUG ID 5099
      const BUnitActionPlasmaShieldGen* pAction = (BUnitActionPlasmaShieldGen*)pPrimary->getActionByType(BAction::cActionTypeUnitPlasmaShieldGen);
//--

      // [8-19-08 CJS] This can happen from ::disconnect, presumably because the actions have already been removed
      if (!pAction)
         return;

      float recharge;
      pPrimary->getRecharging(false, pAction->mShieldInfo.mShieldProtoId, &recharge);

      if (recharge > 0.0f)
         pShieldGen->setRecharge(false, mShieldInfo.mShieldProtoId, recharge);
   }

   // Take down any subshields
   while (mShieldInfo.mBldgToShieldMap.size() > 0)
   {
      removeSubShield(mShieldInfo.mBldgToShieldMap.begin()->first);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::updateCoveredObjectsVulnerability(bool hide)
{
   // create the shield, then check for obstructions
   BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pShieldGen);

   BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
   
   // We may not have a base if we're shutting down
   if (!pBase)
      return;

   // set all the building sockets and their plugs as invulnerable 
   uint numEntityRefs = pBase->getNumberEntityRefs();
   for (uint i=0; i<numEntityRefs; i++)
   {
      BEntityRef* pRef = pBase->getEntityRefByIndex(i);
      if (pRef && pRef->mType == BEntityRef::cTypeAssociatedSocket)
      {
         BObject* pSocket = gWorld->getUnit(pRef->mID);
         if (pSocket && pSocket->getProtoObject() && pSocket->getProtoObject()->isType(gDatabase.getOTIDBuildingSocket()))
         {
            // set the socket plug, if one exists
            BEntityRef* pPlugRef = pSocket->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug);
            if (pPlugRef)
            {
               BObject* pPlug = gWorld->getUnit(pPlugRef->mID);

               if (pPlug)
                  pPlug->setFlagInvulnerable(hide);
            }
         }
      }
   }

   // set the base 
   pBase->setFlagInvulnerable(hide);
}


//==============================================================================
//==============================================================================
BEntityID BUnitActionPlasmaShieldGen::getPrimaryShieldGen(void)
{
   BASSERT(mpOwner);
//-- FIXING PREFIX BUG ID 5108
   const BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pShieldGen);

//-- FIXING PREFIX BUG ID 5109
   const BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
//--
   
   if (!pBase)
      return cInvalidObjectID;

   uint numRefs = pBase->getNumberEntityRefs();

   // Search the building sockets for another shield generator and return it if it's the primary
   for (uint i = 0; i < numRefs; ++i)
   {
      const BEntityRef* pRef = pBase->getEntityRefByIndex(i);

      if (pRef->mType == BEntityRef::cTypeAssociatedBuilding)
      {
         BUnit* pUnit = gWorld->getUnit(pRef->mID);
         if (pUnit)
         {
//-- FIXING PREFIX BUG ID 5103
            const BUnitActionPlasmaShieldGen* pAction = (BUnitActionPlasmaShieldGen*)pUnit->getActionByType(BAction::cActionTypeUnitPlasmaShieldGen);
//--
            if (pAction && pAction->isPrimary() && pUnit->isAlive())
               return pUnit->getID();
         }
      }
   }

   return cInvalidObjectID;
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::addSecondaryShieldGen(void)
{
   ++mShieldInfo.mNumShields;

   upgradeShield();

   updateSockets();
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::removeSecondaryShieldGen(void)
{
   --mShieldInfo.mNumShields;

   downgradeShield();

   // Update shieldpoints
   BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);
   if (pShield)
   {
      if (pShield->getShieldpoints() > pShield->getSPMax())
         pShield->setShieldpoints(pShield->getSPMax());
   }

   updateSockets();
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::upgradeShield()
{
   //long techID = gDatabase.getPTIDCovenantShieldUpgrade();
      
   //BPlayer* pPlayer = mpOwner->getPlayer();
   //BASSERT(pPlayer);

   uint oldNumShields = mShieldInfo.mNumShields-1;
   uint newNumShields = mShieldInfo.mNumShields;
   BUnit* pShieldUnit = gWorld->getUnit(mShieldInfo.mShieldID);
   if (pShieldUnit && oldNumShields && newNumShields)
   {
      float damageTakenScalar = pShieldUnit->getDamageTakenScalar();
      float oldShieldDamageModifier = 1.0f / static_cast<float>(oldNumShields);
      float newShieldDamageModifier = 1.0f / static_cast<float>(newNumShields);
      damageTakenScalar /= oldShieldDamageModifier;
      damageTakenScalar *= newShieldDamageModifier;
      pShieldUnit->setDamageTakenScalar(damageTakenScalar);
   }      

   //pPlayer->getTechTree()->activateTech(techID, mShieldInfo.mShieldID/*-1*/, true);
}


//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::downgradeShield()
{
   //long techID = gDatabase.getPTIDCovenantShieldDowngrade();
      
   //BPlayer* pPlayer = mpOwner->getPlayer();
   //BASSERT(pPlayer);

   uint oldNumShields = mShieldInfo.mNumShields+1;
   uint newNumShields = mShieldInfo.mNumShields;
   BUnit* pShieldUnit = gWorld->getUnit(mShieldInfo.mShieldID);
   if (pShieldUnit && oldNumShields && newNumShields)
   {
      float damageTakenScalar = pShieldUnit->getDamageTakenScalar();
      float oldShieldDamageModifier = 1.0f / static_cast<float>(oldNumShields);
      float newShieldDamageModifier = 1.0f / static_cast<float>(newNumShields);
      damageTakenScalar /= oldShieldDamageModifier;
      damageTakenScalar *= newShieldDamageModifier;
      pShieldUnit->setDamageTakenScalar(damageTakenScalar);
   }      

   //pPlayer->getTechTree()->activateTech(techID, mShieldInfo.mShieldID/*-1*/, true);
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::findNewPrimary()
{
   BASSERT(mpOwner);
//-- FIXING PREFIX BUG ID 5110
   const BUnit* pShieldGen = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pShieldGen);

//-- FIXING PREFIX BUG ID 5111
   const BUnit* pBase = gWorld->getUnit(pShieldGen->getBaseBuilding());
//--

   // When the game shuts down, this could be called by disconnect after the base building has been deleted.
   if (pBase == NULL)
      return;

   // If we're the only shield, shutdown and bail
   if (mShieldInfo.mFlagPrimary && mShieldInfo.mNumShields == 1)
   {
      // Remove event listener on main shield
      BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);

      if (pShield)
      {
         pShield->removeEventListener(this);
      }

      // Remove event listenters on subshields
      long count = mShieldInfo.mBldgToShieldMap.getNumber();
      for (long i = 0; i < count; i++)
      {
         BSquad* pSquad = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[i].second);
         if (pSquad)
         {
            BEntityID shieldUnitID = cInvalidObjectID;
            if (pSquad && pSquad->getNumberChildren()>0)
               shieldUnitID = pSquad->getChild(0);

            BUnit* pUnit = gWorld->getUnit(shieldUnitID);
            if (pUnit)
            {
               pUnit->removeEventListener(this);
            }
         }
      }
      
      takeShieldDown();
      return;
   }

   uint numRefs = pBase->getNumberEntityRefs();

   // Search the building sockets for another available shield generator and set it as primary
   for (uint i = 0; i < numRefs; ++i)
   {
      const BEntityRef* pRef = pBase->getEntityRefByIndex(i);

      if (pRef->mType == BEntityRef::cTypeAssociatedBuilding)
      {
         BUnit* pUnit = gWorld->getUnit(pRef->mID);
         if (pUnit)
         {
            BUnitActionPlasmaShieldGen* pAction = (BUnitActionPlasmaShieldGen*)pUnit->getActionByType(BAction::cActionTypeUnitPlasmaShieldGen);
            if (pAction && !pAction->isPrimary() && pAction != this && pUnit->isAlive())
            {
               BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);

               // Reset shieldpoints appropriately
               if (pShield && pShield->getShieldpoints() > pShield->getSPMax())
                  pShield->setShieldpoints(pShield->getSPMax());

               mShieldInfo.mNumShields--;
               downgradeShield(); // mrh - moved to after we decrement the shields so we can modify the damage taken scalar

               pAction->mShieldInfo = mShieldInfo;
               mShieldInfo.mFlagPrimary = false;

               // Stop listening for shield events and tell the new action to listen
               if (pShield)
               {
                  pShield->removeEventListener(this);
                  pShield->addEventListener(pAction);
               }

               // Do the same for subshields
               long count = mShieldInfo.mBldgToShieldMap.getNumber();
               for (long j = 0; j < count; j++)
               {
                  BUnit* pUnit = gWorld->getUnit(mShieldInfo.mBldgToShieldMap[j].second);
                  if (pUnit)
                  {
                     pUnit->removeEventListener(this);
                     pUnit->addEventListener(pAction);
                  }
               }

               // Make sure new primary shield gen is not recharging (unless we were)
               float recharge = 0.0f;
               if (pShieldGen)
                  pShieldGen->getRecharging(false, mShieldInfo.mShieldProtoId, &recharge);

               BUnit* pNewShieldGen = pAction->getOwner()->getUnit();
               if (pNewShieldGen)
                  pNewShieldGen->setRecharge(false, mShieldInfo.mShieldProtoId, recharge);

               if (recharge == 0.0f)
                  pAction->setState(cStateWorking);
               
               break;
            }
         }
      }
   }

   // Tell all other shield generators to update their primary
   for (uint i = 0; i < numRefs; ++i)
   {
      const BEntityRef* pRef = pBase->getEntityRefByIndex(i);

      if (pRef->mType == BEntityRef::cTypeAssociatedBuilding)
      {
         BUnit* pUnit = gWorld->getUnit(pRef->mID);
         if (pUnit)
         {
            BUnitActionPlasmaShieldGen* pAction = (BUnitActionPlasmaShieldGen*)pUnit->getActionByType(BAction::cActionTypeUnitPlasmaShieldGen);
            if (pAction && !pAction->isPrimary())
               pAction->updatePrimary();
         }
      }
   }
}


//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::updatePrimary()
{
   mPrimary = getPrimaryShieldGen();
   BASSERT(mPrimary != cInvalidObjectID);
}

//==============================================================================
//==============================================================================
int BUnitActionPlasmaShieldGen::findSquad(BEntityID squadID)
{
   int count = mShieldInfo.mBldgToShieldMap.size();
   for (int i = 0; i < count; ++i)
   {
      if (mShieldInfo.mBldgToShieldMap[i].first == squadID)
         return i;
   }
   return -1;
}

//==============================================================================
//==============================================================================
void BUnitActionPlasmaShieldGen::renderStatic(BObject* pObject, BVisualRenderAttributes* pRenderAttributes, DWORD subUpdate, DWORD subUpdatesPerUpdate, float elapsedSubUpdateTime)
{
   if (!pObject)
      return;

   BVisual* pVisual = pObject->getVisual();
   if (!pVisual)
      return;
   BGrannyInstance* pGrannyInstance = pVisual->getGrannyInstance();
   if (!pGrannyInstance)
      return;

   // Render
   pRenderAttributes->mEmissiveIntensity = pObject->getHighlightIntensity();
   //pRenderAttributes->mHighlightIntensity = 2.0f;
   pVisual->render(pRenderAttributes);
}

//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BProtoObjectID, mShieldInfo.mShieldProtoId);
   GFWRITEVAR(pStream, BEntityID, mShieldInfo.mShieldID);
   GFWRITEVAR(pStream, long, mCachedLocalPlayerId);
   GFWRITEVAR(pStream, float,mAttackWaitTime);
   GFWRITEVAR(pStream, uint, mShieldInfo.mNumShields);
   GFWRITEVAL(pStream, BEntityID, mPrimary);
   GFWRITEBITBOOL(pStream, mShieldInfo.mFlagPrimary);
   GFWRITEBITBOOL(pStream, mShieldInfo.mFlagShieldUp);
   GFWRITEBITBOOL(pStream, mShieldInfo.mFlagRaising);

   long count = mShieldInfo.mBldgToShieldMap.getNumber();
   GFWRITEVAR(pStream, long, count);
   for (long i = 0; i < count; i++)
   {
      GFWRITEVAR(pStream, BEntityID, mShieldInfo.mBldgToShieldMap[i].first);
      GFWRITEVAR(pStream, BEntityID, mShieldInfo.mBldgToShieldMap[i].second);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, BProtoObjectID, mShieldInfo.mShieldProtoId);
   GFREADVAR(pStream, BEntityID, mShieldInfo.mShieldID);
   GFREADVAR(pStream, long, mCachedLocalPlayerId);
   GFVERIFYCOUNT(mCachedLocalPlayerId, 16);
   if (BAction::mGameFileVersion < 10)
   {
      GFREADTEMPVAL(pStream, byte);
      GFREADTEMPVAL(pStream, byte);
      GFREADTEMPVAL(pStream, int);
      GFREADTEMPVAL(pStream, float);
   }
   GFREADVAR(pStream, float,mAttackWaitTime);
   GFREADVAR(pStream, uint, mShieldInfo.mNumShields);
   GFREADVAR(pStream, BEntityID, mPrimary);
   GFREADBITBOOL(pStream, mShieldInfo.mFlagPrimary);
   GFREADBITBOOL(pStream, mShieldInfo.mFlagShieldUp);

   if (BAction::mGameFileVersion >= 14)
      GFREADBITBOOL(pStream, mShieldInfo.mFlagRaising);

   if (BAction::mGameFileVersion >= 23)
   {
      long count;
      GFREADVAR(pStream, long, count);

      BEntityID first, second;
      for (long i = 0; i < count; i++)
      {
         GFREADVAR(pStream, BEntityID, first);
         GFREADVAR(pStream, BEntityID, second);

         mShieldInfo.mBldgToShieldMap.add(BuildingShieldPair(first, second));
      }
   }

   gSaveGame.remapProtoObjectID(mShieldInfo.mShieldProtoId);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::postLoad(int saveType)
{
   if (!BAction::postLoad(saveType))
      return false;

   if (mShieldInfo.mFlagPrimary)
   {
      BUnit* pShield = gWorld->getUnit(mShieldInfo.mShieldID);
      if (pShield)
         pShield->addEventListener(this);

      long count = mShieldInfo.mBldgToShieldMap.getNumber();
      for (long i = 0; i < count; i++)
      {
         BSquad* pSquad = gWorld->getSquad(mShieldInfo.mBldgToShieldMap[i].second);
         if (pSquad)
         {
            BEntityID shieldUnitID = cInvalidObjectID;
            if (pSquad && pSquad->getNumberChildren()>0)
               shieldUnitID = pSquad->getChild(0);
            BUnit* pUnit = gWorld->getUnit(shieldUnitID);
            if (pUnit)
               pUnit->addEventListener(this);
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPlasmaShieldGen::savePtr(BStream* pStream) const
{
   GFWRITEACTIONPTR(pStream, this);
   return true;
}
