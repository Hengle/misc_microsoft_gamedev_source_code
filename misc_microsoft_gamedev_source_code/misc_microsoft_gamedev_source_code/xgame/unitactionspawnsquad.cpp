//==============================================================================
// unitactionspawnsquad.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionspawnsquad.h"
#include "command.h"
#include "entity.h"
#include "Platoon.h"
#include "player.h"
#include "protoobject.h"
#include "protosquad.h"
#include "squadplotter.h"
#include "tactic.h"
#include "unit.h"
#include "world.h"
#include "usermanager.h"
#include "user.h"
#include "selectionmanager.h"
#include "soundmanager.h"
#include "game.h"
#include "unitactionthrown.h"
#include "SimOrderManager.h"
#include "SquadActionWork.h"
#include "actionmanager.h"
#include "simhelper.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionSpawnSquad, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionSpawnSquad::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(mpProtoAction);
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   mSquadType=mpProtoAction->getSquadType();
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionSpawnSquad::init()
{
   if (!BAction::init())
      return(false);

   mSquadType=-1;
   mCurrentPoints=0.0f;
   mTotalPoints=0.0f;
   mRandomWorkRateVariance=0.0f;
   mCount=0;
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionSpawnSquad::update(float elapsed)
{
   BASSERT(mpProtoAction);

   if (mpProtoAction && mpProtoAction->getFlagDisabled())
      return true;

   BASSERT(mpOwner);
   BUnit* pUnit = static_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (mState)
   {
      case cStateNone:
      {
         // Gaia doesn't spawn stuff
         if (mpOwner->getPlayerID() == 0)
            break;

         BPlayer*  pPlayer = mpOwner->getPlayer();
         BProtoSquad*  pTrainSquadProto = pPlayer->getProtoSquad(mSquadType);

         // If we're on spawn and auto-join, and the target squad already has joined units, we're done (a previous action already did our work for us)
         if (mpProtoAction->getFlagAutoJoin())
         {
            BUnit* pUnit = mpOwner->getUnit();
            BASSERT(pUnit);
            if (!pUnit)
               return (false);

//-- FIXING PREFIX BUG ID 4891
            const BSquad* pSquad = pUnit->getParentSquad();
//--
            BASSERT(pSquad);
            if (!pSquad)
               return (false);

            if (pSquad->getMergeCount() >= mpProtoAction->getCount())
            {
               setState(cStateDone);
               break;
            }
            else
               mCount = pSquad->getMergeCount();
         }

         mCurrentPoints = 0.0f;
         mTotalPoints   = pTrainSquadProto->getBuildPoints();

         // Spawn and attach squad
         long animType = mpProtoAction->getAnimType();

         if (animType != -1)
         {
//-- FIXING PREFIX BUG ID 4892
            const BSquad* pSquad = completeSpawn();
//--
            if (pSquad && pSquad->getLeaderUnit())
            {
               mSpawnedUnit = pSquad->getLeaderUnit()->getID();
               pUnit->attachObject(mSpawnedUnit);
               if (mpProtoAction->getFlagHideSpawnUntilRelease())
                  pSquad->getLeaderUnit()->setFlagNoRender(true);
            }
         }

         setState(cStateWorking);
         break;
      }

      case cStateWorking:
      {
         // Gaia doesn't spawn stuff
         if (mpOwner->getPlayerID() == 0)
         {
            setState(cStateNone);
            break;
         }

         // Update the points
         if (gWorld->getFlagQuickBuild())
            elapsed *= 30.0f;
         else
         {
            BPlayer* pPlayer = pUnit->getPlayer();
            if (pPlayer)      
               elapsed *= pPlayer->getAIBuildSpeedMultiplier();   // Sets speed multiplier for AI handicapping in deathmatch.  1.0 all other times.
         }
         mCurrentPoints += elapsed;

         // We're done
         float total = mTotalPoints;

         if (mpProtoAction->getWorkRate() > 0.0f)
            total = mpProtoAction->getWorkRate() + mRandomWorkRateVariance;

         if (mCurrentPoints >= total)
         {
            long animType = mpProtoAction->getAnimType();

            if (animType != -1)
            {
               pUnit->beginPlayBlockingAnimation(BObjectAnimationState::cAnimationStateMisc, animType, true, false, false, -1, NULL, false);

               setState(cStateWait);
            }
            else if (completeSpawn())
            {
               ++mCount;

               if (mpProtoAction->getCount() > 0 && mCount >= mpProtoAction->getCount())
                  setState(cStateDone);
               
               if (mFlagPersistent)
               {
                  BPlayer*  pPlayer = mpOwner->getPlayer();
                  BProtoSquad*  pTrainSquadProto = pPlayer->getProtoSquad(mSquadType);

                  mCurrentPoints = 0.0f;
                  mTotalPoints = pTrainSquadProto->getBuildPoints();
               }
               else
                  setState(cStateDone);
            }
            else
               mCurrentPoints = mTotalPoints;

            // Update random work rate variance
            float variance = mpProtoAction->getWorkRateVariance();
            mRandomWorkRateVariance = getRandRangeFloat(cSimRand, -0.5f * variance, 0.5f * variance);
         }
         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
BSquad* BUnitActionSpawnSquad::completeSpawn(void)
{
   BPlayer*  pPlayer = mpOwner->getPlayer();
   BProtoSquad*  pTrainSquadProto = pPlayer->getProtoSquad(mSquadType);
   BProtoObject*  pTrainProto = pPlayer->getProtoObject(pTrainSquadProto->getUnitNode(0).mUnitType);

   uint8 trainLimitBucket=0;
   const long trainLimit = getSquadTrainLimit(mSquadType, &trainLimitBucket);
   if (trainLimit == 0)
      return NULL;

   BVector pos, forward, right;

   // If we're air dropping, just spawn on top of ourselves
   if (mpProtoAction->getStationary())
   {
      pos = mpOwner->getPosition();
      forward = mpOwner->getForward();
      right = mpOwner->getRight();
   }
   else
   {
      //FIXME AJL 5/9/06 - Temp code to calculate random location for placing new unit
      float obstructionRadius = pTrainProto->getObstructionRadius();
      BVector dir = cZAxisVector;
      dir.rotateXZ(getRandRangeFloat(cSimRand, 0.0f, cTwoPi));
      dir.normalize();
      
      pos = mpOwner->getPosition() + (dir * (mpOwner->getObstructionRadius() + obstructionRadius * getRandRangeFloat(cSimRand, 2.0f, 4.0f)));
      forward = dir;
      right.assignCrossProduct(cYAxisVector, forward);
      right.normalize();

      BVector suggestedPos(0.0f);
      static BDynamicSimVectorArray instantiatePositions;
      instantiatePositions.setNumber(0);
      long closestDesiredPositionIndex;

      bool validExit = BSimHelper::findInstantiatePositions(1, instantiatePositions, 0.5f, pos,
         forward, right, obstructionRadius * 2.0f, pos, closestDesiredPositionIndex, 4, true, 0.8f);

      if(validExit && instantiatePositions.getNumber() > 0)
      {
         if(closestDesiredPositionIndex >= 0 && closestDesiredPositionIndex < instantiatePositions.getNumber())
            pos = instantiatePositions[closestDesiredPositionIndex];
         else
            pos = instantiatePositions[0];
      }
   }

   // Create the new squad
   BEntityID   spawnedSquadID = gWorld->createEntity(mSquadType, true, mpOwner->getPlayerID(), pos, forward, right, true);
   BSquad      *pSpawnedSquad = gWorld->getSquad(spawnedSquadID);
   BDEBUG_ASSERT(pSpawnedSquad);

   // Track build limit
   if (trainLimit != -1)
   {
      pSpawnedSquad->addEntityRef(BEntityRef::cTypeTrainLimitBuilding, mpOwner->getID(), 0, 0);
      mpOwner->addEntityRef(BEntityRef::cTypeTrainLimitSquad, spawnedSquadID, (short)mSquadType, trainLimitBucket);
   }

   // Play create sound
   BPlayerID playerID = pPlayer->getID();
   if (playerID == gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() || (gGame.isSplitScreen() && pPlayer->getID() == gUserManager.getUser(BUserManager::cSecondaryUser)->getPlayerID()))
      gSoundManager.playCue(pTrainProto->getSound(cObjectSoundCreate));

   // Goto rally point
   bool useRallyPoint=false;
   BVector rallyPoint;
   BUnit* pUnit=mpOwner->getUnit();
   if(pUnit && pUnit->haveRallyPoint(playerID) && pUnit->getProtoObject()->getRallyPointType()==pTrainProto->getRallyPointType())
   {
      useRallyPoint=true;
      rallyPoint=pUnit->getRallyPoint(playerID);
   }
   else if(pPlayer->haveRallyPoint())
   {
      if(pTrainProto->getRallyPointType()==cRallyPointMilitary)
      {
         useRallyPoint=true;
         rallyPoint=pPlayer->getRallyPoint();
      }
   }
   if(pUnit && useRallyPoint)
   {
      // Tell units to move near the rally point instead of right on top of it.
      BVector dir=rallyPoint-pUnit->getPosition();
      if (dir.length() > 4.0f)
      {
         dir.normalize();
         rallyPoint -= dir * 4.0f;
      }
      BCommand* pCommand=new BCommand(-1, BCommand::cNumberCommandFlags);
      pCommand->setPlayerID(pPlayer->getID());
      pCommand->addWaypoint(rallyPoint);
      BEntityIDArray ids;
      ids.add(pSpawnedSquad->getID());
      gSquadPlotter.plotSquads(ids, pCommand);
      delete pCommand;
      const BDynamicSimArray<BSquadPlotterResult>& plotterResults = gSquadPlotter.getResults();
      BSimTarget target;
      if(plotterResults.getNumber()>0)
         target.setPosition(plotterResults[0].getDesiredPosition());
      else
         target.setPosition(rallyPoint);

      BPlatoon* pPlatoon=pSpawnedSquad->getParentPlatoon();
      if (pPlatoon)
         pPlatoon->queueWork(target);
      else
         pSpawnedSquad->queueWork(target);
   }

   if (mpProtoAction->getFlagAutoJoin())
   {
      BSimOrder* pOrder = gSimOrderManager.createOrder();
      BASSERT(pOrder);
      if (pOrder)  
      {
         BUnit* pUnit = mpOwner->getUnit();
         BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 4898
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         BASSERT(pSquad);

         BSimTarget target;
         target.setID(pSquad->getID());
         target.setAbilityID(0);

         pOrder->setPriority(BSimOrder::cPrioritySim);
         pOrder->setTarget(target);

         BSquadActionWork* pAction=reinterpret_cast<BSquadActionWork*>(gActionManager.createAction(BAction::cActionTypeSquadWork));

         if (pAction)
         {
            pAction->setUserData(1);
            pAction->setTarget(target);
            pAction->setUnitOppType(BUnitOpp::cTypeJoin);
            pAction->setFlagDoneOnOppComplete(true);
            pAction->setFlagAIAction(pOrder->getPriority() == BSimOrder::cPrioritySim);

            pSpawnedSquad->addAction(pAction, pOrder);
         }
      }
   }

   return pSpawnedSquad;
}

//==============================================================================
//==============================================================================
long BUnitActionSpawnSquad::getSquadTrainLimit(long id, uint8* pTrainLimitBucketOut) const
{
   BUnit*  pBuilding = reinterpret_cast<BUnit *>(mpOwner);
   const BProtoObject*  pBuildingProto = pBuilding->getProtoObject();
   uint8 trainLimitBucket=0;
   long trainLimit = pBuildingProto->getTrainLimit(id, true, &trainLimitBucket);

   if (pTrainLimitBucketOut)
      *pTrainLimitBucketOut=trainLimitBucket;

   // Calculate the current train limit based on this unit types train limit less the number of items already trained
   long trainedCount = 0;
   if (trainLimit != -1)
   {
      long checkCount = pBuilding->getNumberEntityRefs();
      for (long i = 0; i < checkCount; i++)
      {
         BEntityRef *pEntityRef = pBuilding->getEntityRefByIndex(i);
         if (pEntityRef && pEntityRef->mData2 == trainLimitBucket && pEntityRef->mType == BEntityRef::cTypeTrainLimitSquad)
            trainedCount++;
      }

      trainLimit -= trainedCount;
      trainLimit = Math::Max(trainLimit, 0L);
   }

   return trainLimit;
}

//==============================================================================
//==============================================================================
void BUnitActionSpawnSquad::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BASSERT(mpProtoAction);
   
   if (mpProtoAction && mpProtoAction->getFlagDisabled())
      return;

   BASSERT(mpOwner);
   BUnit* pUnit = static_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (eventType == BEntity::cEventAnimAttackTag && mState == cStateWait)
   {
      // Detach squad
      BUnit* pSpawnedUnit = gWorld->getUnit(mSpawnedUnit);
      if (pSpawnedUnit && mpProtoAction->getFlagHideSpawnUntilRelease())
         pSpawnedUnit->setFlagNoRender(false);
      pUnit->unattachObject(mSpawnedUnit);
   }

   if ((eventType == BEntity::cEventAnimEnd || eventType == BEntity::cEventAnimLoop) && mState == cStateWait)
   {
      // Spawn another egg
      setState(cStateNone);
   }

   BAction::notify(eventType, senderID, data, data2);
}


//==============================================================================
//==============================================================================
bool BUnitActionSpawnSquad::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, long, mSquadType);
   GFWRITEVAR(pStream, float, mCurrentPoints);
   GFWRITEVAR(pStream, float, mTotalPoints);
   GFWRITEVAR(pStream, float, mRandomWorkRateVariance);
   GFWRITEVAR(pStream, BEntityID, mSpawnedUnit);
   GFWRITEVAR(pStream, long, mCount);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionSpawnSquad::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, long, mSquadType);
   GFREADVAR(pStream, float, mCurrentPoints);
   GFREADVAR(pStream, float, mTotalPoints);
   GFREADVAR(pStream, float, mRandomWorkRateVariance);
   GFREADVAR(pStream, BEntityID, mSpawnedUnit);
   GFREADVAR(pStream, long, mCount);

   gSaveGame.remapProtoSquadID(mSquadType);

   return true;
}
