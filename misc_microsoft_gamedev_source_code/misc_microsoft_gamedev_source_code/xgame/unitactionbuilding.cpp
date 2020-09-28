//==============================================================================
// unitactionbuilding.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionbuilding.h"
#include "Platoon.h"
#include "protoobject.h"
#include "prototech.h"
#include "techtree.h"
#include "unit.h"
#include "unitquery.h"
#include "usermanager.h"
#include "world.h"
#include "protosquad.h"
#include "squadplotter.h"
#include "command.h"
#include "database.h"
#include "user.h"
#include "usermanager.h"
#include "uigame.h"
#include "soundmanager.h"
#include "syncmacros.h"
#include "squadactiontransport.h"
#include "civ.h"
#include "tactic.h"
#include "team.h"
#include "vincehelper.h"
#include "physics.h"
#include "triggermanager.h"
#include "terrain.h"
#include "terrainSimRep.h"
#include "uimanager.h"
#include "SimOrderManager.h"
#include "configsgame.h"
#include "game.h"
#include "generaleventmanager.h"
#include "scenario.h"
#include "alert.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionBuilding, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::connect(BEntity* pOwner, BSimOrder *pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return false;

   int playerCount=1;
   if (pOwner->getProtoObject()->getFlagCommandableByAnyPlayer())
   {
      playerCount=gWorld->getNumberPlayers();
      mFlagCommandableByAnyPlayer=true;
   }
   else if (gWorld->getFlagCoop())
   {
//-- FIXING PREFIX BUG ID 3185
      const BTeam* pTeam=mpOwner->getPlayer()->getTeam();
//--
      playerCount=pTeam->getNumberPlayers();
      mFlagCoop=true;
   }
   mCurrentItems.resize(playerCount, false);
   for (int i=0; i<playerCount; i++)
      mCurrentItems[i].clear();
   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::disconnect()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
      return;

   if (mFlagResearchSetAnimRate || mFlagBuildSetAnimRate)
   {
      pUnit->setAnimationRate(1.0f);
      mFlagResearchSetAnimRate=false;
      mFlagBuildSetAnimRate=false;
   }

   clearQueue();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::init()
{
   if (!BAction::init())
      return(false);

   mFlagPersistent=true;
   mCreatedByPlayerID=cInvalidPlayerID;
   mBuildTime=gWorld->getGametime();
   mDestructTime=0;
   mTrainedSquadBirthTime=0.0f;
   mMoveSquadsCount=0;
   mMoveSquadsTimer=0.0f;
   mRebuildTimer=0.0f;
   mPrevBuildPct=0.0f;
   mLastBuildPointsForAnimRate=-1.0f;
   mDescriptionOverride.empty();
   mDelayedDoppleList.clear();
   mQueuedItems.clear();
   mRechargeList.clear();
   mTrainedSquads.clear();
   mFlagGatherLink=false;
   mFlagAutoBuild=true;
   mFlagCompleteBuildOnNextUpdate=false;
   mFlagStartedBuilt=false;
   mFlagCommandableByAnyPlayer=false;
   mFlagCoop=false;
   mFlagResearchSetAnimRate=false;
   mFlagBuildSetAnimRate=false;
   mFlagObstructionIsClear=false;
   mFlagDescriptionOverride=false;
   mFlagLastQuickBuild=false;
   mFlagHideParentSocket=false;
   mCurrentItems.clear();
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::update(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);

   // Process delayed dopples
   long numDelayedDopples = mDelayedDoppleList.getNumber();
   for (long i = 0; i < numDelayedDopples; i++)
   {
      BUBADelayedDopple& delayedDopple = mDelayedDoppleList[i];
      // Time's up!
      if (--delayedDopple.mCountDown <= 0)
      {
         // Dopple
         BUnit* pBuilding = gWorld->getUnit(delayedDopple.mEntityID);
         if (pBuilding)
         {
             for (int i = 1; i < gWorld->getNumberTeams(); i++)
                pBuilding->forceDoppleAllTeams();
         }

         // Remove from list
         mDelayedDoppleList.removeIndex(i, false);
      }
   }

   switch (mState)
   {
      case cStateNone:
      {  
         if (mFlagCompleteBuildOnNextUpdate)
         {
            mFlagCompleteBuildOnNextUpdate=false;
            completeBuild(mCreatedByPlayerID, true, false);
         }

//-- FIXING PREFIX BUG ID 3186
         const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(mpOwner->getPlayerID());
//--
         if (currentItem.mCurrentType == BProtoObjectCommand::cTypeBuild)
         {
            // Make sure we can build and move squads that are in the way
            if (pUnit->getFlagBuildingConstructionPaused())
            {
               if (!mFlagObstructionIsClear && updateMoveSquadsFromObstruction(elapsed))
                  mFlagObstructionIsClear = true;
               break;
            }
            else if (!updateMoveSquadsFromObstruction(elapsed))
               break;

            // Okay, we are ready to build.
            const BProtoObject* pBuildProto=pUnit->getProtoObject();
            if (pBuildProto && pBuildProto->getFlagManualBuild())
               mFlagAutoBuild=false;
            else
               mFlagAutoBuild=true;
            setState(cStateWorking);
            mpOwner->setFlagIsBuilt(true);

            updateBuildAnimRate();
         }
         else
            setState(cStateWorking);

         break;
      }
      
      case cStateWorking:
      {
         // Hide this parent socket after build other performed
         if (mFlagHideParentSocket)
         {
            if (pUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug))
               pUnit->setFlagNoRender(true);
            mFlagHideParentSocket=false;
         }

         // Update recharge list
         updateRechargeList(elapsed);

         // Update birth of trained squads
         updateTrainedSquadBirth(elapsed);

         // Handle our build other queue
         pUnit->updateBuildOtherQueue();

         // Update rebuild timer
         if (mRebuildTimer > 0.0f)
         {
            mRebuildTimer -= elapsed;
            if (mRebuildTimer < 0.0f)
               mRebuildTimer = 0.0f;
            gWorld->notify(BEntity::cEventRebuildTimer, mpOwner->getID(), 0, 0);
         }

         bool anyWork=false;
         uint currentItemCount=mCurrentItems.getSize();
         for (uint i=0; i<currentItemCount; i++)
         {
            BPlayerID playerID=getPlayerIDForIndex(i);
            BPlayer* pPlayer = gWorld->getPlayer(playerID);
            BUnitActionBuildingCurrentItem& currentItem=mCurrentItems[i];
            if (currentItem.mCurrentType==-1)
            {
               // Grab the next item from the queue
               for (int j=0; j<mQueuedItems.getNumber(); j++)
               {
                  BUnitActionBuildingQueuedItem& queuedItem = mQueuedItems[j];
                  if (queuedItem.mPlayerID!=playerID)
                     continue;

                  if (queuedItem.mType == BProtoObjectCommand::cTypeTrainUnit)
                  {
//-- FIXING PREFIX BUG ID 3187
                     const BProtoObject* pTrainProto = pPlayer->getProtoObject(queuedItem.mID);
//--
                     if (pTrainProto)
                     {
                        currentItem.mCurrentType = queuedItem.mType;
                        currentItem.mCurrentID = queuedItem.mID;
                        currentItem.mPlayerID = queuedItem.mPlayerID;
                        currentItem.mCurrentLinkedResource = queuedItem.mLinkedResource;
                        currentItem.mTrainLimitBucket = queuedItem.mTrainLimitBucket;
                        currentItem.mTriggerScriptID = queuedItem.mTriggerScriptID;
                        currentItem.mTriggerVarID = queuedItem.mTriggerVarID;
                        currentItem.mCurrentPoints = 0.0f;
                        currentItem.mTotalPoints = pTrainProto->getBuildPoints();
                        currentItem.mNextUpdatePercent = 0.01f;
                        queuedItem.mCount--;
                        if (queuedItem.mCount == 0)
                           mQueuedItems.removeIndex(j, true);
                        pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateResearch, pTrainProto->getTrainAnimType());
                        anyWork=true;
                        break;
                     }
                  }
                  else if (queuedItem.mType == BProtoObjectCommand::cTypeTrainSquad)
                  {
//-- FIXING PREFIX BUG ID 3188
                     const BProtoSquad* pTrainSquadProto = pPlayer->getProtoSquad(queuedItem.mID);
//--
                     if (pTrainSquadProto)
                     {
                        currentItem.mCurrentType = queuedItem.mType;
                        currentItem.mCurrentID = queuedItem.mID;
                        currentItem.mPlayerID = queuedItem.mPlayerID;
                        currentItem.mCurrentLinkedResource = queuedItem.mLinkedResource;
                        currentItem.mTrainLimitBucket = queuedItem.mTrainLimitBucket;
                        currentItem.mTriggerScriptID = queuedItem.mTriggerScriptID;
                        currentItem.mTriggerVarID = queuedItem.mTriggerVarID;
                        currentItem.mCurrentPoints = 0.0f;
                        currentItem.mTotalPoints = pTrainSquadProto->getBuildPoints();
                        currentItem.mNextUpdatePercent = 0.01f;
                        queuedItem.mCount--;
                        if (queuedItem.mCount == 0)
                           mQueuedItems.removeIndex(j, true);
                        pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateResearch, pTrainSquadProto->getTrainAnimType());
                        anyWork=true;
                        break;
                     }
                  }
                  else if (queuedItem.mType == BProtoObjectCommand::cTypeResearch)
                  {
//-- FIXING PREFIX BUG ID 3189
                     const BProtoTech* pProtoTech = pPlayer->getProtoTech(queuedItem.mID);
//--
                     if (pProtoTech)
                     {
                        currentItem.mCurrentType = queuedItem.mType;
                        currentItem.mCurrentID = queuedItem.mID;
                        currentItem.mPlayerID = queuedItem.mPlayerID;
                        currentItem.mCurrentLinkedResource = -1;
                        currentItem.mTrainLimitBucket = 0;
                        currentItem.mTriggerScriptID = queuedItem.mTriggerScriptID;
                        currentItem.mTriggerVarID = queuedItem.mTriggerVarID;
                        currentItem.mCurrentPoints = 0.0f;
                        currentItem.mTotalPoints = pProtoTech->getResearchPoints();
                        currentItem.mNextUpdatePercent = 0.01f;
                        mQueuedItems.removeIndex(j, true);
                        if (pUnit->isControllerFree(BActionController::cControllerAnimation))
                        {
                           pUnit->setAnimationRate(0.0f);
                           mFlagResearchSetAnimRate=true;
                           pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateResearch, pProtoTech->getAnimType(), true);
                        }
                        anyWork=true;
                        break;
                     }
                  }
                  else if (queuedItem.mType == BProtoObjectCommand::cTypeBuildOther)
                  {
//-- FIXING PREFIX BUG ID 3191
                     const BProtoObject* pBuildingProto = pPlayer->getProtoObject(queuedItem.mID);
//--
                     if (pBuildingProto)
                     {
//-- FIXING PREFIX BUG ID 3190
                        const BSquad* pSquad=pUnit->getParentSquad();
//--
                        // Don't let "build other" commands start while under attacked.
                        if (pBuildingProto->getFlagNoBuildUnderAttack() && pSquad && pSquad->wasAttackedRecently())
                           break;
                        else if (pSquad && pSquad->isFrozen())
                           break;
                        else
                        {
                           BPlayer* purchasingPlayer = gWorld->getPlayer(queuedItem.mPurchasingPlayerID);
                           BEntityID buildingID = startBuildOther(queuedItem.mPlayerID, queuedItem.mPurchasingPlayerID, queuedItem.mID, false);
                           if (buildingID != cInvalidObjectID)
                           {
                              currentItem.mCurrentType = queuedItem.mType;
                              currentItem.mCurrentID = queuedItem.mID;
                              currentItem.mPlayerID = queuedItem.mPlayerID;
                              currentItem.mPurchasingPlayerID = queuedItem.mPurchasingPlayerID;
                              currentItem.mCurrentBuildingID = buildingID;
                              currentItem.mCurrentLinkedResource = queuedItem.mLinkedResource;
                              currentItem.mTrainLimitBucket = queuedItem.mTrainLimitBucket;
                              currentItem.mTriggerScriptID = queuedItem.mTriggerScriptID;
                              currentItem.mTriggerVarID = queuedItem.mTriggerVarID;
                              currentItem.mCurrentPoints = 0.0f;
                              currentItem.mTotalPoints = pBuildingProto->getBuildPoints();
                              currentItem.mNextUpdatePercent = 0.01f;
                              pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateResearch, pBuildingProto->getTrainAnimType());
                           }
                           else
                           {
                              purchasingPlayer->refundCost(pBuildingProto->getCost());
                              checkTriggerState(queuedItem.mTriggerScriptID, queuedItem.mTriggerVarID, cInvalidObjectID);
                           }
                           purchasingPlayer->adjustFutureUnitCount(queuedItem.mID, -1);
                           queuedItem.mCount--;
                           if (queuedItem.mCount == 0)
                              mQueuedItems.removeIndex(j, true);
                        }
                        anyWork=true;
                        break;
                     }
                  }
                  else if (queuedItem.mType == BProtoObjectCommand::cTypeCustomCommand)
                  {
//-- FIXING PREFIX BUG ID 3192
                     const BCustomCommand* pCustomCommand = gWorld->getCustomCommand(queuedItem.mID);
//--
                     if (pCustomCommand)
                     {
                        currentItem.mCurrentType = queuedItem.mType;
                        currentItem.mCurrentID = queuedItem.mID;
                        currentItem.mPlayerID = queuedItem.mPlayerID;
                        currentItem.mCurrentLinkedResource = -1;
                        currentItem.mTrainLimitBucket = 0;
                        currentItem.mTriggerScriptID = queuedItem.mTriggerScriptID;
                        currentItem.mTriggerVarID = queuedItem.mTriggerVarID;
                        currentItem.mCurrentPoints = 0.0f;
                        currentItem.mTotalPoints = pCustomCommand->mTimer;
                        currentItem.mNextUpdatePercent = 0.01f;
                        mQueuedItems.removeIndex(j, true);
                        pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateResearch, -1);
                        anyWork=true;
                        break;
                     }
                  }
                  mQueuedItems.removeIndex(j, true);
                  j--;
               }
            }
            else
            {
               anyWork=true;

               // Update the points
               long unitID = mpOwner->getID().asLong();

               bool updatePoints=true;
               if(currentItem.mCurrentType==BProtoObjectCommand::cTypeBuild)
               {
                  updateBuildAnimRate();

                  if (!mFlagAutoBuild)
                     updatePoints = false;
                  else if (pUnit->getProtoObject()->getFlagNoBuildUnderAttack())
                  {
//-- FIXING PREFIX BUG ID 3193
                     const BSquad* pSquad=pUnit->getParentSquad();
//--
                     if (pSquad && pSquad->wasAttackedRecently())
                        updatePoints = false;
                  }
               }
               
               if (pUnit->getParentSquad() && pUnit->getParentSquad()->isFrozen())
                  updatePoints = false;

               if (updatePoints)
               {            
                  float points = elapsed;            

                  // Take unit's work rate into account
                  if (mpProtoAction)
                     points *= mpProtoAction->getWorkRate(mpOwner->getID());            
                  else
                     points *= pUnit->getWorkRateScalar();

                  // Quick build
                  if (gWorld->getFlagQuickBuild())
                     points *= 30.0f;
                  else
                  { 
                     BPlayer* pPlayer = pUnit->getPlayer();
                     if (pPlayer)      
                        points *= pPlayer->getAIBuildSpeedMultiplier();   // Sets speed multiplier for AI handicapping in deathmatch.  1.0 all other times.
                  }

                  currentItem.mCurrentPoints += points;
               }

               // Update mTotalPoints in case something modified the base value (such as from a tech)
               switch (currentItem.mCurrentType)
               {
                  case BProtoObjectCommand::cTypeBuild:
                  {
                     const BProtoObject* pProtoObject = pUnit->getProtoObject();
                     if (pProtoObject)
                        currentItem.mTotalPoints = pProtoObject->getBuildPoints();
                     break;
                  }

                  case BProtoObjectCommand::cTypeTrainUnit:
                  case BProtoObjectCommand::cTypeBuildOther:
                  {
                     const BProtoObject* pProtoObject = pPlayer->getProtoObject(currentItem.mCurrentID);
                     if (pProtoObject)
                        currentItem.mTotalPoints = pProtoObject->getBuildPoints();
                     break;
                  }

                  case BProtoObjectCommand::cTypeTrainSquad:
                  {
                     const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(currentItem.mCurrentID);
                     if (pProtoSquad)
                        currentItem.mTotalPoints = pProtoSquad->getBuildPoints();
                     break;
                  }

                  case BProtoObjectCommand::cTypeResearch:
                  {
                     const BProtoTech* pProtoTech = pPlayer->getProtoTech(currentItem.mCurrentID);
                     if (pProtoTech)
                        currentItem.mTotalPoints = pProtoTech->getResearchPoints();
                     break;
                  }
               }

               if (currentItem.mCurrentPoints>currentItem.mTotalPoints)
                  currentItem.mCurrentPoints = currentItem.mTotalPoints;

               if (updatePoints && (currentItem.mCurrentType == BProtoObjectCommand::cTypeBuild))
               {
                  BObject* pObject=gWorld->getObject(mpOwner->getID());//should this be owner ID?
                  if (pObject)
                  {
//-- FIXING PREFIX BUG ID 3196
                     const BVisual* pVisual=pObject->getVisual();
//--
                     if (pVisual)
                     {
//-- FIXING PREFIX BUG ID 3194
                        const BProtoVisual* pProtoVisual=pVisual->getProtoVisual();
//--
                        if (pProtoVisual)
                        {
                           const BProtoVisualLogicNode* pLogicNode=pProtoVisual->getLogicNode();
                           if (pLogicNode)
                           {
                              // Only call computeVisual() if the animation has crossed a logic node threshold since the last update
                              long valueCount = pLogicNode->mLogicValues.getNumber();
                              float buildPct = currentItem.mCurrentPoints / currentItem.mTotalPoints;
                              for (long i=valueCount-1; i>=0; i--)
                              {
                                 const BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
                                 if((mPrevBuildPct < pLogicValue->mValueFloat) && (buildPct >= pLogicValue->mValueFloat))
                                    pUnit->computeVisual();
                              }
                              mPrevBuildPct = currentItem.mCurrentPoints / currentItem.mTotalPoints;
                           }
                        }
                     }
                  }
               }

               if (currentItem.mCurrentType == BProtoObjectCommand::cTypeResearch)
               {
                  int techID = currentItem.mCurrentID;

                  pPlayer->getTechTree()->setResearchPoints(techID, unitID, currentItem.mCurrentPoints);

                  if (gWorld->getFlagCoop() && pPlayer->isHuman())
                  {
//-- FIXING PREFIX BUG ID 3195
                     const BTeam* pTeam=pPlayer->getTeam();
//--
                     int playerCount=pTeam->getNumberPlayers();
                     for (int j=0; j<playerCount; j++)
                     {
                        BPlayer* pTeamPlayer = gWorld->getPlayer(pTeam->getPlayerID(j));
                        if (pTeamPlayer != pPlayer)
                           pTeamPlayer->getTechTree()->setResearchPoints(techID, unitID, currentItem.mCurrentPoints);
                     }
                  }

                  BVisual *pVisual = pUnit->getVisual();
                  if (pVisual && pUnit->isControllerFree(BActionController::cControllerAnimation) && currentItem.mTotalPoints > 0.0f && elapsed > 0.0f)
                  {
                     float percent = currentItem.mCurrentPoints / currentItem.mTotalPoints;
                     float duration = pUnit->getAnimationDuration(cActionAnimationTrack);
                     if (duration > 0.0f)
                     {
                        float position = pVisual->getAnimationPosition(cActionAnimationTrack) / duration;
                        float change = percent - position;
                        float rate = Math::Max(change * duration / elapsed, 0.0f);
                        pUnit->setAnimationRate(rate);
                        mFlagResearchSetAnimRate=true;
                     }
                  }
               }

               bool done = false;
               if (currentItem.mCurrentType == BProtoObjectCommand::cTypeBuildOther)
               {
                  BUnit* pBuilding = gWorld->getUnit(currentItem.mCurrentBuildingID);
                  if (!pBuilding || pBuilding->getFlagBuilt() || !pBuilding->isAlive())
                     done = true;
               }
               else
                  done = (currentItem.mCurrentPoints == currentItem.mTotalPoints);
               if (done)
               {
                  // Finish the task
                  BEntityID builtEntity = cInvalidObjectID;
                  switch (currentItem.mCurrentType)
                  {
                     case BProtoObjectCommand::cTypeTrainUnit:
                     case BProtoObjectCommand::cTypeTrainSquad:
                        pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
                        builtEntity = completeTrain(playerID, false);
                        break;

                     case BProtoObjectCommand::cTypeResearch:
                        pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
                        completeResearch(playerID, false);
                        break;

                     case BProtoObjectCommand::cTypeBuild:
                        completeBuild(playerID, false, false);
                        builtEntity = pUnit->getID();
                        break;

                     case BProtoObjectCommand::cTypeBuildOther:
                        completeBuildOther(playerID, currentItem.mPurchasingPlayerID, false, false);
                        builtEntity = currentItem.mCurrentBuildingID;
                        break;

                     case BProtoObjectCommand::cTypeCustomCommand:
                        pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
                        completeCustomCommand(playerID);
                        break;
                  }
                  long currentType = currentItem.mCurrentType;
                  long currentID = currentItem.mCurrentID;
                  BTriggerScriptID currentTriggerScriptID=currentItem.mTriggerScriptID;
                  BTriggerVarID currentTriggerVarID=currentItem.mTriggerVarID;
                  currentItem.clear();
                  checkTriggerState(currentTriggerScriptID, currentTriggerVarID, builtEntity);
                  switch (currentType)
                  {
                     case BProtoObjectCommand::cTypeTrainUnit:    gWorld->notify(BEntity::cEventTrainPercent, mpOwner->getID(), currentID, 0); break;
                     case BProtoObjectCommand::cTypeTrainSquad:   gWorld->notify(BEntity::cEventTrainSquadPercent, mpOwner->getID(), currentID, 0); break;
                     case BProtoObjectCommand::cTypeResearch:     gWorld->notify(BEntity::cEventTechPercent, mpOwner->getID(), currentID, 0); break;
                     case BProtoObjectCommand::cTypeBuild:        gWorld->notify(BEntity::cEventBuildPercent, mpOwner->getID(), currentID, 0); break;
                     case BProtoObjectCommand::cTypeBuildOther:   gWorld->notify(BEntity::cEventTrainPercent, mpOwner->getID(), currentID, 0); break;
                     case BProtoObjectCommand::cTypeCustomCommand:gWorld->notify(BEntity::cEventCustomCommandPercent, mpOwner->getID(), currentID, 0); break;
                  }
               }
               else
               {
                  if ((currentItem.mCurrentPoints / currentItem.mTotalPoints) >= currentItem.mNextUpdatePercent)
                  {
                     switch (currentItem.mCurrentType)
                     {
                        case BProtoObjectCommand::cTypeTrainUnit:    gWorld->notify(BEntity::cEventTrainPercent, mpOwner->getID(), currentItem.mCurrentID, 0); break;
                        case BProtoObjectCommand::cTypeTrainSquad:   gWorld->notify(BEntity::cEventTrainSquadPercent, mpOwner->getID(), currentItem.mCurrentID, 0); break;
                        case BProtoObjectCommand::cTypeResearch:     gWorld->notify(BEntity::cEventTechPercent, mpOwner->getID(), currentItem.mCurrentID, 0); break;
                        case BProtoObjectCommand::cTypeBuild:        gWorld->notify(BEntity::cEventBuildPercent, mpOwner->getID(), currentItem.mCurrentID, 0); break;
                        case BProtoObjectCommand::cTypeBuildOther:   gWorld->notify(BEntity::cEventTrainPercent, mpOwner->getID(), currentItem.mCurrentID, 0); break;
                        case BProtoObjectCommand::cTypeCustomCommand:gWorld->notify(BEntity::cEventCustomCommandPercent, mpOwner->getID(), currentItem.mCurrentID, 0); break;
                     }
                     currentItem.mNextUpdatePercent += 0.01f;
                  }
               }
            }
         }

         if (!anyWork && !mFlagPersistent)
            setState(cStateDone);
         break;
      }

      case cStateFading:
      {
         gWorld->notify(BEntity::cEventSelfDestructTime, mpOwner->getID(), 0, 0);
         if (gWorld->getGametime()>=mDestructTime)
         {
            //if (!pUnit->getFlagBuilt() || pUnit->getAssociatedBase() != pUnit->getID())
            {
               // Refund cost
               BPlayer* pPlayer=gWorld->getPlayer(mCreatedByPlayerID);
               const BProtoObject* pBuildProto=pUnit->getProtoObject();
               if (pPlayer && pBuildProto)
               {
                  BCost cost;
                  pBuildProto->getCost(pPlayer, &cost, -1);
                  // SLB: Tim wants a penalty when canceling a building that's about to be hit by a volley of grenades
                  //if (pUnit->getFlagBuilt())
                     cost *= gDatabase.getRecyleRefundRate();
                  float curHp = pUnit->getHitpoints();
                  float maxHp = pBuildProto->getHitpoints();
                  if (curHp < maxHp)
                  {
                     float pctDamaged = curHp / maxHp;
                     cost *= pctDamaged;
                  }
                  pPlayer->refundCost(&cost);
               }
            }

            setState(cStateWait);
            gWorld->notify(BEntity::cEventBuildPercent, mpOwner->getID(), pUnit->getProtoID(), 0);
            pUnit->setFlagRecycled(true);
            pUnit->kill(false);
         }
         break;
      }
   }


/* DEBUG ONLY
   BEntityID buildingSquadID = mpOwner->getParentID();
   const BOPObstructionNode* pObstructionNode = mpOwner->getObstructionNode();
   BOPQuadHull* pHull = (BOPQuadHull*) pObstructionNode->getHull(); // Cast to remove const
   if (!pHull)
      return(true);
   BSquad* pObsSquad = gWorld->getSquad(buildingSquadID);
   if (!pObsSquad)
      return(true);
   BVector obsPos = pObsSquad->getPosition();
   BVector maxPoint(pHull->getMaxX() - obsPos.x, 10.0f, pHull->getMaxZ() - obsPos.z);
   BVector minPoint(pHull->getMinX() - obsPos.x, 00.0f, pHull->getMinZ() - obsPos.z);
   BMatrix xfrm;
   xfrm.makeOrient(pObsSquad->getForward(), pObsSquad->getUp(), pObsSquad->getRight());
   xfrm.setTranslation(obsPos);
   BBoundingBox boundingBox;
   boundingBox.initializeTransformed(minPoint, maxPoint, xfrm);
   boundingBox.draw(cDWORDPurple);
*/

   return (true);
}

//==============================================================================
// BUnitActionBuilding::addTrain
//==============================================================================
void BUnitActionBuilding::addTrain(BPlayerID playerID, long id, long count, bool squad, bool noCost, BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID, bool forcePlaySound)
{
   BUnit* pBuilding=reinterpret_cast<BUnit*>(mpOwner);
   const BProtoObject* pBuildingProto=pBuilding->getProtoObject();
   BPlayer* pPlayer=gWorld->getPlayer(playerID);

   BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);

//-- FIXING PREFIX BUG ID 3200
   const BProtoSquad* pTrainSquadProto = NULL;
//--
//-- FIXING PREFIX BUG ID 3201
   const BProtoObject* pTrainProto = NULL;
//--
   bool instantTrainWithRecharge = false;
   float rechargePoints = 0.0f;

   if (squad)
   {
      pTrainSquadProto = pPlayer->getProtoSquad(id);
      if(!pTrainSquadProto)
      {
         completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
         return;
      }
      instantTrainWithRecharge = pTrainSquadProto->getFlagInstantTrainWithRecharge();
      if (instantTrainWithRecharge)
      {
         const BProtoSquad* pProtoSquad = mpOwner->getPlayer()->getProtoSquad(id);
         if (pProtoSquad)
            rechargePoints = pProtoSquad->getBuildPoints();
      }
   }
   else
   {
      pTrainProto = pPlayer->getProtoObject(id);
      if(!pTrainProto)
      {
         completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
         return;
      }
      instantTrainWithRecharge = pTrainProto->getFlagInstantTrainWithRecharge();
      if (instantTrainWithRecharge)
      {
         const BProtoObject* pProtoObject = mpOwner->getPlayer()->getProtoObject(id);
         if (pProtoObject)
            rechargePoints = pProtoObject->getBuildPoints();
      }
   }

   const BPopArray* pPops;
   BPopArray pops;
   BCost cost;
   if(squad)
   {
      const BCost* pCost=pTrainSquadProto->getCost();
      cost=*pCost;
      pTrainSquadProto->getPops(pops);
      pPops=&pops;      
   }
   else
   {
      pPops=pTrainProto->getPops();
   }

   long itemCount=mQueuedItems.getNumber();

   if(count<0)
   {
      // Handle un-queue
      for(long i=itemCount-1; i>=0 && count<0; i--)
      {
         BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
         if((queuedItem.mType == (squad ? BProtoObjectCommand::cTypeTrainSquad : BProtoObjectCommand::cTypeTrainUnit)) && queuedItem.mID==id && queuedItem.mPlayerID==playerID)
         {
            long trainCount=queuedItem.mCount;
            for(long j=0; j<trainCount && count<0; j++)
            {
               // Refund cost
               if(!squad)
                  pTrainProto->getCost(pPlayer, &cost, -1);
               pPlayer->refundCost(&cost);
               pPlayer->refundPopFuture(pPops);
               if(!squad)
               {
                  pPlayer->adjustFutureUnitCount(id, -1);
               }
               else
               {
                  pPlayer->adjustFutureSquadCount(id, -1);
                  /*
                  BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(id);
                  for (long i=0; i<pProtoSquad->getNumberUnitNodes(); i++)
                     pPlayer->adjustFutureUnitCount(pProtoSquad->getUnitNode(i).mUnitType, -pProtoSquad->getUnitNode(i).mUnitCount);
                  */
               }

               // Un-reserve resource spot
               unreserveTrainResourceLink(queuedItem.mLinkedResource, queuedItem.mID);

               // Remove one from the train queue count
               queuedItem.mCount--;

               // Remove the item if the train queue count is now zero
               if(queuedItem.mCount==0)
               {
                  BTriggerScriptID queuedTriggerScriptID=queuedItem.mTriggerScriptID;
                  BTriggerVarID queuedTriggerVarID=queuedItem.mTriggerVarID;
                  mQueuedItems.erase(i);
                  checkTriggerState(queuedTriggerScriptID, queuedTriggerVarID, cInvalidObjectID);
               }

               count++;
               if(count==0)
                  break;
            }
         }
      }

      if(count != 0 && (currentItem.mCurrentType== (squad ? BProtoObjectCommand::cTypeTrainSquad : BProtoObjectCommand::cTypeTrainUnit)) && currentItem.mCurrentID==id)
      {
         // Refund the cost
         if(!squad)
            pTrainProto->getCost(pPlayer, &cost, -1);
         pPlayer->refundCost(&cost);
         pPlayer->refundPopFuture(pPops);
         if(!squad)
         {
            pPlayer->adjustFutureUnitCount(id, -1);
         }
         else
         {
            pPlayer->adjustFutureSquadCount(id, -1);
            /*
            BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(id);
            for (long i=0; i<pProtoSquad->getNumberUnitNodes(); i++)
               pPlayer->adjustFutureUnitCount(pProtoSquad->getUnitNode(i).mUnitType, -pProtoSquad->getUnitNode(i).mUnitCount);
            */
         }

         // Un-reserve resource spot
         unreserveTrainResourceLink(currentItem.mCurrentLinkedResource, currentItem.mCurrentID);

         // Stop training this unit
         BTriggerScriptID currentTriggerScriptID=currentItem.mTriggerScriptID;
         BTriggerVarID currentTriggerVarID=currentItem.mTriggerVarID;
         currentItem.clear();
         checkTriggerState(currentTriggerScriptID, currentTriggerVarID, currentItem.mCurrentBuildingID);
         pBuilding->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
      }
      gWorld->notify((squad ? BEntity::cEventTrainSquadQueued : BEntity::cEventTrainQueued), mpOwner->getID(), id, 0);
      return;
   }
   else
   {
      // Handle queuing new units

      // Check if not available because the unit/squad is recharging
      if (instantTrainWithRecharge)
      {
         for (int i=0; i<mRechargeList.getNumber(); i++)
         {
//-- FIXING PREFIX BUG ID 3198
            const BUnitActionBuildingRecharge& recharge=mRechargeList[i];
//--
            if (recharge.mID == id && recharge.mSquad == squad)
            {
               completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
               return;
            }
         }
      }

      // Calculate the current train limit
      const cMaxResources=16;
      BUBAResourceLink resourceLinks[cMaxResources];
      long resourceCount=0;
      uint8 trainLimitBucket=0;
      long trainLimit=getTrainLimitAndResourceLinks(playerID, id, resourceLinks, cMaxResources, &resourceCount, NULL, &trainLimitBucket, squad);

      if(trainLimit!=-1 && count>trainLimit)
         count=trainLimit;

      // Attempt to pay for the units
      long paidCount=0;
      if(noCost)
         paidCount=count;
      else
      {
         for(long i=0; i<count; i++)
         {
            if(!squad)
               pTrainProto->getCost(pPlayer, &cost, 0);
            if(!pPlayer->checkCost(&cost) || !pPlayer->checkPops(pPops))
               break;
            pPlayer->payCost(&cost);
            pPlayer->payPopFuture(pPops);
            if(!squad)
            {
               pPlayer->adjustFutureUnitCount(id, 1);
            }
            else
            {
               pPlayer->adjustFutureSquadCount(id, 1);
               /*
               BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(id);
               for (long i=0; i<pProtoSquad->getNumberUnitNodes(); i++)
                  pPlayer->adjustFutureUnitCount(pProtoSquad->getUnitNode(i).mUnitType, pProtoSquad->getUnitNode(i).mUnitCount);
               */
            }
            paidCount++;
         }
      }

      BEntityID trainID = cInvalidObjectID;
      if(paidCount>0)
      {
         if(resourceCount>0)
         {
            // Add train units to the queue and reserve spots on linked resources. Need to create a
            // separate train queue items because each one could reserve a spot on different resources.
            long buildingID=mpOwner->getID().asLong();
            long totalReserveCount=paidCount;
            long actualReservedCount=0;
            int gatherLinkObjectType=pBuildingProto->getGatherLinkObjectType();
            for(long i=0; i<resourceCount; i++)
            {
               BUBAResourceLink& link=resourceLinks[i];
               long reserveCount=min(totalReserveCount, link.mCount);
               long refCount=link.mpUnit->getNumberEntityRefs();
               for(long j=0; j<refCount; j++)
               {
                  BEntityRef *pEntityRef = link.mpUnit->getEntityRefByIndex(j);
                  if (!pEntityRef)
                     continue;

                  if(pEntityRef->mID == buildingID && pEntityRef->mType == BEntityRef::cTypeGatherBuilding && pEntityRef->mData1 == (short)gatherLinkObjectType)
                  {
                     // Reserve a spot on this resource
                     pEntityRef->mData2 = (BYTE)(reserveCount + pEntityRef->mData2);

                     if(noCost || instantTrainWithRecharge)
                     {
                        // Immediately train
                        BUnitActionBuildingCurrentItem saveCurrentItem=currentItem;
                        currentItem.mCurrentType=(BYTE)(squad ? BProtoObjectCommand::cTypeTrainSquad : BProtoObjectCommand::cTypeTrainUnit);
                        currentItem.mPlayerID = playerID;
                        currentItem.mCurrentID=id;
                        currentItem.mCurrentLinkedResource=link.mpUnit->getID().asLong();
                        currentItem.mTrainLimitBucket=trainLimitBucket;
                        currentItem.mTriggerScriptID=triggerScriptID;
                        currentItem.mTriggerVarID=triggerVarID;
                        for(long k=0; k<reserveCount; k++)
                           trainID = completeTrain(playerID, noCost, forcePlaySound);
                        currentItem=saveCurrentItem;
                        if (instantTrainWithRecharge)
                           setRecharge(squad, id, rechargePoints);
                     }
                     else
                     {
                        // Add the train item to the queue
                        BUnitActionBuildingQueuedItem queuedItem;
                        queuedItem.mType=(BYTE)(squad ? BProtoObjectCommand::cTypeTrainSquad : BProtoObjectCommand::cTypeTrainUnit);
                        queuedItem.mID=(short)id;
                        queuedItem.mCount=(BYTE)reserveCount;
                        queuedItem.mLinkedResource=link.mpUnit->getID().asLong();
                        queuedItem.mPlayerID=playerID;
                        queuedItem.mTrainLimitBucket=trainLimitBucket;
                        queuedItem.mTriggerScriptID=triggerScriptID;
                        queuedItem.mTriggerVarID=triggerVarID;
                        mQueuedItems.add(queuedItem);
                     }
                     actualReservedCount+=reserveCount;
                     break;
                  }
               }
               totalReserveCount-=reserveCount;
               if(totalReserveCount<=0)
                  break;
            }
            if(actualReservedCount<paidCount)
            {
               if(!noCost)
               {
                  // Refund for unreserved items
                  for(long i=actualReservedCount; i<paidCount; i++)
                  {
                     if(!squad)
                        pTrainProto->getCost(pPlayer, &cost, -1);
                     pPlayer->refundCost(&cost);
                     pPlayer->refundPopFuture(pPops);
                     if(!squad)
                     {
                        pPlayer->adjustFutureUnitCount(id, -1);
                     }
                     else
                     {
                        pPlayer->adjustFutureSquadCount(id, -1);
                        /*
                        BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(id);
                        for (long i=0; i<pProtoSquad->getNumberUnitNodes(); i++)
                           pPlayer->adjustFutureUnitCount(pProtoSquad->getUnitNode(i).mUnitType, -pProtoSquad->getUnitNode(i).mUnitCount);
                        */
                     }
                  }
               }
            }
         }
         else
         {
            if(noCost || instantTrainWithRecharge)
            {
               // Immediately train
               BUnitActionBuildingCurrentItem saveCurrentItem=currentItem;
               currentItem.mCurrentType=(BYTE)(squad ? BProtoObjectCommand::cTypeTrainSquad : BProtoObjectCommand::cTypeTrainUnit);
               currentItem.mCurrentID=id;
               currentItem.mPlayerID = playerID;
               currentItem.mCurrentLinkedResource=-1;
               currentItem.mTrainLimitBucket=trainLimitBucket;
               currentItem.mTriggerScriptID=triggerScriptID;
               currentItem.mTriggerVarID=triggerVarID;
               for(long k=0; k<paidCount; k++)
                  trainID = completeTrain(playerID, noCost, forcePlaySound);
               currentItem=saveCurrentItem;
               if (instantTrainWithRecharge)
                  setRecharge(squad, id, rechargePoints);
            }
            else
            {
               // Add train units to the queue
               bool add=true;
               if(itemCount>0)
               {
                  BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[itemCount-1];
                  if((queuedItem.mType==(squad ? BProtoObjectCommand::cTypeTrainSquad : BProtoObjectCommand::cTypeTrainUnit)) && queuedItem.mID==id && queuedItem.mPlayerID==playerID)
                  {
                     queuedItem.mCount=queuedItem.mCount+(BYTE)paidCount;
                     add=false;
                  }
               }
               if(add)
               {
                  BUnitActionBuildingQueuedItem queuedItem;
                  queuedItem.mType=(BYTE)(squad ? BProtoObjectCommand::cTypeTrainSquad : BProtoObjectCommand::cTypeTrainUnit);
                  queuedItem.mID=(short)id;
                  queuedItem.mCount=(BYTE)paidCount;
                  queuedItem.mLinkedResource=-1;
                  queuedItem.mPlayerID=playerID;
                  queuedItem.mTrainLimitBucket=trainLimitBucket;
                  queuedItem.mTriggerScriptID=triggerScriptID;
                  queuedItem.mTriggerVarID=triggerVarID;
                  mQueuedItems.add(queuedItem);
               }
            }
         }
      }

      checkTriggerState(triggerScriptID, triggerVarID, trainID);

      gWorld->notify((squad ? BEntity::cEventTrainSquadQueued : BEntity::cEventTrainQueued), mpOwner->getID(), id, 0);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::unreserveTrainResourceLink(long resourceID, long trainID)
{
   if(resourceID==-1)
      return;

   BUnit* pResource=gWorld->getUnit(resourceID);
   if(!pResource)
      return;

   BUnit* pBuilding=reinterpret_cast<BUnit*>(mpOwner);
   const BProtoObject* pBuildingProto=pBuilding->getProtoObject();
   int gatherLinkObjectType=pBuildingProto->getGatherLinkObjectType();

   const BProtoObject* pTrainProto = pBuilding->getPlayer()->getProtoObject(trainID);
   if (!pTrainProto || !pTrainProto->isType(gatherLinkObjectType))
      return;

   long buildingID=mpOwner->getID().asLong();

   long refCount=pResource->getNumberEntityRefs();
   for(long k=0; k<refCount; k++)
   {
      BEntityRef *pEntityRef = pResource->getEntityRefByIndex(k);
      if (pEntityRef && pEntityRef->mID == buildingID && pEntityRef->mType == BEntityRef::cTypeGatherBuilding && pEntityRef->mData1 == (short)gatherLinkObjectType)
      {
         long reserveCount = pEntityRef->mData2;
         if(reserveCount>0)
         {
            reserveCount--;
            pEntityRef->mData2 = (BYTE)reserveCount;
         }
         return;
      }
   }
}

//==============================================================================
//==============================================================================
long BUnitActionBuilding::getTrainCount(BPlayerID playerID, long id) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   long count=0;
   if(mState==cStateWorking && (currentItem.mPlayerID == playerID) && (currentItem.mCurrentType==BProtoObjectCommand::cTypeTrainUnit || currentItem.mCurrentType==BProtoObjectCommand::cTypeTrainSquad) && currentItem.mCurrentID==id)
      count=1;
   long itemCount=mQueuedItems.getNumber();
   for(long i=0; i<itemCount; i++)
   {
      const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
      if((queuedItem.mType==BProtoObjectCommand::cTypeTrainUnit || queuedItem.mType==BProtoObjectCommand::cTypeTrainSquad) && queuedItem.mID==id && (queuedItem.mPlayerID==playerID))
         count+=queuedItem.mCount;
   }
   return count;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::getTrainQueue(BPlayerID playerID, long*  pCount, float*  pTotalPoints) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   long count=0;
   float points=0;

   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   
   if(mState==cStateWorking && (currentItem.mCurrentType==BProtoObjectCommand::cTypeTrainUnit || currentItem.mCurrentType==BProtoObjectCommand::cTypeTrainSquad))
   {
      if(currentItem.mPlayerID==playerID)
      {
         //Not checking player for now
      }

      count=1;

      points = (currentItem.mTotalPoints - currentItem.mCurrentPoints);
   }

   long itemCount=mQueuedItems.getNumber();
   for(long i=0; i<itemCount; i++)
   {
      const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
      if((queuedItem.mType==BProtoObjectCommand::cTypeTrainUnit || queuedItem.mType==BProtoObjectCommand::cTypeTrainSquad))
      {
         if(queuedItem.mPlayerID==playerID)
         {
            //Not checking player for now
         }

         count+=queuedItem.mCount;
         
         if(queuedItem.mType==BProtoObjectCommand::cTypeTrainUnit)
         {
//-- FIXING PREFIX BUG ID 3202
            const BProtoObject* pObj = pPlayer ? pPlayer->getProtoObject(queuedItem.mID) : gDatabase.getGenericProtoObject(queuedItem.mID);
//--
            if(pObj != NULL)
               points += (queuedItem.mCount * pObj->getBuildPoints());
         }
         else if(queuedItem.mType==BProtoObjectCommand::cTypeTrainSquad)
         {
//-- FIXING PREFIX BUG ID 3203
            const BProtoSquad* pSqd = pPlayer ? pPlayer->getProtoSquad(queuedItem.mID) : gDatabase.getGenericProtoSquad(queuedItem.mID);
//--
            if(pSqd != NULL)
               points += (queuedItem.mCount * pSqd->getBuildPoints());
         }
      }
   }

//-- FIXING PREFIX BUG ID 3204
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--

   //adjust for work rate
   if (mpProtoAction)
      points /= mpProtoAction->getWorkRate(mpOwner->getID());            
   else
      points /= pUnit->getWorkRateScalar();


   *pCount = count;
   *pTotalPoints = points;
}

//==============================================================================
//==============================================================================
float BUnitActionBuilding::getTrainPercent(BPlayerID playerID, long id) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   if(mState==cStateWorking && (currentItem.mPlayerID == playerID) && (currentItem.mCurrentType==BProtoObjectCommand::cTypeTrainUnit || currentItem.mCurrentType==BProtoObjectCommand::cTypeTrainSquad) && currentItem.mCurrentID==id)
   {
      if(currentItem.mTotalPoints==0.0f)
         return 0.0f;
      else
         return currentItem.mCurrentPoints/currentItem.mTotalPoints;
   }
   return 0.0f;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::addResearch(BPlayerID playerID, long techID, long count, bool noCost, BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID)
{
   BPlayer* pPlayer=gWorld->getPlayer(playerID);

//-- FIXING PREFIX BUG ID 3207
   const BTeam* pTeam = pPlayer->getTeam();
//--
   int playerCount = pTeam->getNumberPlayers();

   BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);

//-- FIXING PREFIX BUG ID 3208
   const BProtoTech* pTechProto=pPlayer->getProtoTech(techID);
//--
   if(!pTechProto)
   {
      completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
      return;
   }

   const BCost*  pCost=pTechProto->getCost();

   long unitID=mpOwner->getID().asLong();
   long itemCount=mQueuedItems.getNumber();

   if(count<0)
   {
      for(long i=itemCount-1; i>=0; i--)
      {
//-- FIXING PREFIX BUG ID 3205
         const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
//--
         if(queuedItem.mType==BProtoObjectCommand::cTypeResearch && queuedItem.mID==techID && queuedItem.mPlayerID==playerID)
         {
            pPlayer->refundCost(pCost);

            BTriggerScriptID queuedTriggerScriptID=queuedItem.mTriggerScriptID;
            BTriggerVarID queuedTriggerVarID=queuedItem.mTriggerVarID;
            mQueuedItems.erase(i);
            checkTriggerState(queuedTriggerScriptID, queuedTriggerVarID, cInvalidObjectID);

            pPlayer->getTechTree()->unresearchTech(techID, unitID);

            if (gWorld->getFlagCoop() && pPlayer->isHuman())
            {
               for (int j=0; j<playerCount; j++)
               {
                  BPlayer* pTeamPlayer = gWorld->getPlayer(pTeam->getPlayerID(j));
                  if (pTeamPlayer != pPlayer)
                     pTeamPlayer->getTechTree()->unresearchTech(techID, unitID);
               }
            }

            gWorld->notify(BEntity::cEventTechQueued, mpOwner->getID(), techID, 0);
            return;
         }
      }
      if(currentItem.mCurrentType==BProtoObjectCommand::cTypeResearch && currentItem.mCurrentID==techID)
      {
         pPlayer->refundCost(pCost);
         pPlayer->getTechTree()->unresearchTech(techID, unitID);

         if (gWorld->getFlagCoop() && pPlayer->isHuman())
         {
            for (int j=0; j<playerCount; j++)
            {
               BPlayer* pTeamPlayer = gWorld->getPlayer(pTeam->getPlayerID(j));
               if (pTeamPlayer != pPlayer)
                  pTeamPlayer->getTechTree()->unresearchTech(techID, unitID);
            }
         }

         BTriggerScriptID currentTriggerScriptID=currentItem.mTriggerScriptID;
         BTriggerVarID currentTriggerVarID=currentItem.mTriggerVarID;
         currentItem.clear();
         checkTriggerState(currentTriggerScriptID, currentTriggerVarID, cInvalidObjectID);

         BUnit* pBuilding = getOwner()->getUnit();

         if (mFlagResearchSetAnimRate || mFlagBuildSetAnimRate)
         {
            pBuilding->setAnimationRate(1.0f);
            mFlagResearchSetAnimRate=false;
            mFlagBuildSetAnimRate=false;
         }

         pBuilding->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);

         gWorld->notify(BEntity::cEventTechQueued, mpOwner->getID(), techID, 0);
      }
      return;
   }
   else
   {
      int techStatus = pPlayer->getTechTree()->getTechStatus(techID, unitID);
      if (techStatus == BTechTree::cStatusResearching || techStatus == BTechTree::cStatusCoopResearching || techStatus == BTechTree::cStatusActive)
      {
         completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
         return;
      }

      // Don't allow queuing or researching of more than one unique tech at a time
      if (pTechProto->getFlagUnique() && hasUniqueTech())
      {
         completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
         return;
      }

      for(long i=0; i<itemCount; i++)
      {
//-- FIXING PREFIX BUG ID 3206
         const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
//--
         if(queuedItem.mType==BProtoObjectCommand::cTypeResearch && queuedItem.mID==techID && queuedItem.mPlayerID==playerID)
         {
            completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
            return;
         }
      }

      if (!noCost)
      {
         if(!pPlayer->payCost(pCost))
         {
            completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
            return;
         }
      }

      pPlayer->getTechTree()->researchTech(techID, unitID, false);

      if (gWorld->getFlagCoop() && pPlayer->isHuman())
      {
         for (int j=0; j<playerCount; j++)
         {
            BPlayer* pTeamPlayer = gWorld->getPlayer(pTeam->getPlayerID(j));
            if (pTeamPlayer != pPlayer)
               pTeamPlayer->getTechTree()->researchTech(techID, unitID, true);
         }
      }

      gWorld->notify(BEntity::cEventTechQueued, mpOwner->getID(), techID, 0);

      if (noCost || pTechProto->getFlagInstant())
      {
         // Immediately research
         BUnitActionBuildingCurrentItem saveCurrentItem=currentItem;
         currentItem.mCurrentType=BProtoObjectCommand::cTypeResearch;
         currentItem.mPlayerID = playerID;
         currentItem.mCurrentID=techID;
         currentItem.mTriggerScriptID=triggerScriptID;
         currentItem.mTriggerVarID=triggerVarID;
         completeResearch(playerID, noCost);
         currentItem=saveCurrentItem;
      }
      else
      {
         BUnitActionBuildingQueuedItem queuedItem;
         queuedItem.mType=BProtoObjectCommand::cTypeResearch;
         queuedItem.mID=(short)techID;
         queuedItem.mCount=1;
         queuedItem.mPlayerID=playerID;
         queuedItem.mTriggerScriptID=triggerScriptID;
         queuedItem.mTriggerVarID=triggerVarID;
         mQueuedItems.add(queuedItem);
      }

      checkTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
   }
}

//==============================================================================
//==============================================================================
long BUnitActionBuilding::getResearchCount(BPlayerID playerID, long id) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   if(mState==cStateWorking && (currentItem.mPlayerID == playerID) && currentItem.mCurrentType==BProtoObjectCommand::cTypeResearch && currentItem.mCurrentID==id)
      return 1;
   long itemCount=mQueuedItems.getNumber();
   for(long i=0; i<itemCount; i++)
   {
      const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
      if(queuedItem.mType==BProtoObjectCommand::cTypeResearch && queuedItem.mID==id && (queuedItem.mPlayerID == playerID))
         return 1;
   }
   return 0;
}

//==============================================================================
//==============================================================================
float BUnitActionBuilding::getResearchPercent(BPlayerID playerID, long id) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   if(mState==cStateWorking && (currentItem.mPlayerID == playerID) && currentItem.mCurrentType==BProtoObjectCommand::cTypeResearch && currentItem.mCurrentID==id)
   {
      if(currentItem.mTotalPoints==0.0f)
         return 0.0f;
      else
         return currentItem.mCurrentPoints/currentItem.mTotalPoints;
   }
   return 0.0f;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::getUniqueTechInfo(BPlayerID& playerID, long& techID, float& percentComplete) const
{
   // Check the current items first
   for (uint i=0; i<mCurrentItems.getSize(); i++)
   {
      const BUnitActionBuildingCurrentItem& currentItem=mCurrentItems[i];
      if (currentItem.mCurrentType!=BProtoObjectCommand::cTypeResearch)
         continue;
      const BPlayer* pPlayer=gWorld->getPlayer(getPlayerIDForIndex(i));
      const BProtoTech* pProtoTech=pPlayer->getProtoTech(currentItem.mCurrentID);
      if (!pProtoTech)
         continue;
      if (!pProtoTech->getFlagUnique())
         continue;
      playerID=pPlayer->getID();
      techID=currentItem.mCurrentID;
      percentComplete=getResearchPercent(playerID, techID);
      return true;
   }

   // Check the queue
   for (uint i=0; i<mQueuedItems.getSize(); i++)
   {
      const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
      if (queuedItem.mType!=BProtoObjectCommand::cTypeResearch)
         continue;
      const BPlayer* pPlayer=gWorld->getPlayer(queuedItem.mPlayerID);
      const BProtoTech* pProtoTech=pPlayer->getProtoTech(queuedItem.mID);
      if (!pProtoTech)
         continue;
      if (!pProtoTech->getFlagUnique())
         continue;
      playerID=pPlayer->getID();
      techID=queuedItem.mID;
      percentComplete=0.0f;
      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::hasUniqueTech() const
{
   BPlayerID uniqueTechPlayerID=cInvalidPlayerID;
   long uniqueTechID=-1;
   float uniqueTechPercentComplete=0.0f;
   bool hasUniqueTech=getUniqueTechInfo(uniqueTechPlayerID, uniqueTechID, uniqueTechPercentComplete);
   return hasUniqueTech;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::doBuild(BPlayerID playerID, bool cancel, bool noCost, bool fromSave)
{
   #ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionBuilding::doBuild ownerID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionBuilding::doBuild cancel", cancel);
   #endif

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);

   if(!cancel)
      mCreatedByPlayerID=(playerID==cInvalidPlayerID ? mpOwner->getPlayerID() : playerID);

   BPlayer* pPlayer=gWorld->getPlayer(mCreatedByPlayerID);
   const BProtoObject* pBuildProto=pPlayer->getProtoObject(pUnit->getProtoID());
   if (!pBuildProto)
      return;

   BCost cost;
   pBuildProto->getCost(pPlayer, &cost, -1);

   BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(mpOwner->getPlayerID());

   if(cancel)
   {
      if (currentItem.mCurrentType!=BProtoObjectCommand::cTypeBuild)
         return;

      // Refund the player
      pPlayer->refundCost(&cost);

      #ifdef SYNC_UnitAction
      syncUnitActionCode("BUnitActionBuilding::doBuild setHitpoints 0");
      #endif
      pUnit->setHitpoints(0.0f);
      setState(cStateDone);
   }
   else
   {
      if(mState!=cStateNone)
      {
         BASSERT(0);
         return;
      }

      if(pUnit->getFlagBuilt())
      {
         mFlagStartedBuilt=true;
         // If we are already built from a scenario load, then finish building on the next update instead of now. 
         // This will allow all the objects to be created first, such as objects that need to get auto-linked
         // to this building when the building finishes constructing.
         if (gScenarioLoad && pUnit->getFlagBuilt() && !fromSave)
            mFlagCompleteBuildOnNextUpdate=true;
         else
            completeBuild(playerID, true, fromSave);
      }
      else
      {
         if (!noCost)
         {
            // Pay the cost
            if(!pPlayer->payCost(&cost))
            {
               setState(cStateFailed);
               #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionBuilding::doBuild ownerID", mpOwner->getID().asLong());
               #endif
               mpOwner->kill(false);
               return;
            }
         }

         currentItem.mCurrentType=BProtoObjectCommand::cTypeBuild;
         currentItem.mCurrentID=mpOwner->getID().asLong();
         currentItem.mPlayerID = playerID;
         currentItem.mCurrentLinkedResource=-1;
         currentItem.mCurrentPoints=0.0f;
         currentItem.mTotalPoints=pBuildProto->getBuildPoints();
         currentItem.mNextUpdatePercent=0.01f;
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::isBuild()
{
//-- FIXING PREFIX BUG ID 3209
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(mpOwner->getPlayerID());
//--
   return (currentItem.mCurrentType==BProtoObjectCommand::cTypeBuild);
}

//==============================================================================
//==============================================================================
float BUnitActionBuilding::getBuildPercent() const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(mpOwner->getPlayerID());
   if(currentItem.mCurrentType!=BProtoObjectCommand::cTypeBuild)
      return 0.0f;
   if(currentItem.mTotalPoints==0.0f)
      return 0.0f;
   return currentItem.mCurrentPoints/currentItem.mTotalPoints;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::addBuildPoints(float points)
{
   BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(mpOwner->getPlayerID());
   currentItem.mCurrentPoints+=points;

   if (currentItem.mCurrentType == BProtoObjectCommand::cTypeBuild)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      pUnit->computeVisual();
   }
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::doBuildOther(BPlayerID playerID, BPlayerID purchasingPlayerID, int id, bool cancel, bool noCost, bool doppleOnStart, BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID)
{
//-- FIXING PREFIX BUG ID 3211
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   //BPlayer* pPlayer=gWorld->getPlayer(playerID);
   if(!cancel)
      mCreatedByPlayerID=(purchasingPlayerID==cInvalidPlayerID ? mpOwner->getPlayerID() : purchasingPlayerID);

   BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);

   BPlayer* pPurchasingPlayer = gWorld->getPlayer(purchasingPlayerID);
   if(!pPurchasingPlayer)
   {
      completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
      return;
   }

//-- FIXING PREFIX BUG ID 3212
   const BProtoObject* pBuildingProto=pPurchasingPlayer->getProtoObject(id);
//--
   if(!pBuildingProto)
   {
      completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
      return;
   }


   const BPopArray* pPops=pBuildingProto->getPops();

   long itemCount=mQueuedItems.getNumber();
   if(cancel)
   {
      for(long i=itemCount-1; i>=0; i--)
      {
//-- FIXING PREFIX BUG ID 3210
         const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
//--
         if(queuedItem.mType==BProtoObjectCommand::cTypeBuildOther && queuedItem.mID==id && queuedItem.mPlayerID==playerID)
         {
            //cost needs to match player stored in queue
            BPlayer* pPurchaserPlayer=gWorld->getPlayer(queuedItem.mPurchasingPlayerID);
            BCost purchasedCost;
            pBuildingProto->getCost(pPurchaserPlayer, &purchasedCost, 0);
            const BCost* pPurchasedCost=&purchasedCost;

            pPurchaserPlayer->refundCost(pPurchasedCost);
            pPurchaserPlayer->refundPopFuture(pPops);
            pPurchaserPlayer->adjustFutureUnitCount(id, -1);

            BTriggerScriptID queuedTriggerScriptID=queuedItem.mTriggerScriptID;
            BTriggerVarID queuedTriggerVarID=queuedItem.mTriggerVarID;
            mQueuedItems.erase(i);
            checkTriggerState(queuedTriggerScriptID, queuedTriggerVarID, cInvalidObjectID);

            gWorld->notify(BEntity::cEventTrainQueued, mpOwner->getID(), id, 0);
            return;
         }
      }
      if(currentItem.mCurrentType==BProtoObjectCommand::cTypeBuildOther && currentItem.mCurrentID==id)
      {
         //cost needs to match player stored in queue
         BPlayer* pPurchaserPlayer=gWorld->getPlayer(currentItem.mPurchasingPlayerID);
         BCost purchasedCost;
         pBuildingProto->getCost(pPurchaserPlayer, &purchasedCost, 0);
         const BCost* pPurchasedCost=&purchasedCost;

         pPurchaserPlayer->refundCost(pPurchasedCost);

         BUnit* pBuilding = gWorld->getUnit(currentItem.mCurrentBuildingID);
         if (pBuilding)
            pBuilding->kill(false);

         BTriggerScriptID currentTriggerScriptID=currentItem.mTriggerScriptID;
         BTriggerVarID currentTriggerVarID=currentItem.mTriggerVarID;
         currentItem.clear();
         checkTriggerState(currentTriggerScriptID, currentTriggerVarID, cInvalidObjectID);

         gWorld->notify(BEntity::cEventTrainQueued, mpOwner->getID(), id, 0);
      }
      return;
   }
   else
   {
      // ajl 12/1/08 - PHX-18786 - Don't allow queuing of build other if already queued or active
      if(currentItem.mCurrentType==BProtoObjectCommand::cTypeBuildOther)
      {
         completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
         return;
      }
      for(long i=0; i<itemCount; i++)
      {
         if(mQueuedItems[i].mType==BProtoObjectCommand::cTypeBuildOther)
         {
            completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
            return;
         }
      }

      BCost cost;
      pBuildingProto->getCost(pPurchasingPlayer, &cost, 0);
      const BCost* pCost=&cost;

      if (!noCost)
      {
         if (!gConfig.isDefined(cConfigBuildingQueue))
         {
            if (!pUnit->isNewBuildingConstructionAllowed(NULL, NULL, pUnit->getProtoObject()->getFlagSecondaryBuildingQueue()))
            {
               completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
               return;
            }
         }

         if (!pUnit->getTrainLimit(playerID, id, false) || !pPurchasingPlayer->checkPops(pPops) || !pPurchasingPlayer->checkCost(pCost))
         {
            completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
            return;
         }

         pPurchasingPlayer->payCost(pCost);
      }

      gWorld->notify(BEntity::cEventTrainQueued, mpOwner->getID(), id, 0);

      BEntityID buildingID = cInvalidObjectID;
      if (noCost)
      {
         // Immediately build
         buildingID = startBuildOther(playerID, purchasingPlayerID, id, true);
         if (buildingID == cInvalidObjectID)
         {
            // ajl 11/09/08 - Cost not payed so we should not refund it.
            //pPurchasingPlayer->refundCost(pCost);
            completeTriggerState(triggerScriptID, triggerVarID, cInvalidObjectID);
            return;
         }
         BUnitActionBuildingCurrentItem saveCurrentItem=currentItem;
         currentItem.mCurrentType=BProtoObjectCommand::cTypeBuildOther;
         currentItem.mCurrentID=id;
         currentItem.mPlayerID = playerID;
         currentItem.mPurchasingPlayerID = purchasingPlayerID;
         currentItem.mCurrentBuildingID=buildingID;
         currentItem.mTriggerScriptID=triggerScriptID;
         currentItem.mTriggerVarID=triggerVarID;
         completeBuildOther(playerID, purchasingPlayerID, noCost, doppleOnStart);
         currentItem=saveCurrentItem;
      }
      else
      {
         pPurchasingPlayer->adjustFutureUnitCount(id, 1);
         pPurchasingPlayer->payPopFuture(pPops);
         BUnitActionBuildingQueuedItem queuedItem;
         queuedItem.mType=BProtoObjectCommand::cTypeBuildOther;
         queuedItem.mID=(short)id;
         queuedItem.mCount=1;
         queuedItem.mPlayerID=playerID;
         queuedItem.mPurchasingPlayerID = purchasingPlayerID;
         queuedItem.mTriggerScriptID=triggerScriptID;
         queuedItem.mTriggerVarID=triggerVarID;
         mQueuedItems.add(queuedItem);
      }

      checkTriggerState(triggerScriptID, triggerVarID, buildingID);
   }
}


//==============================================================================
//==============================================================================
BEntityID BUnitActionBuilding::startBuildOther(BPlayerID playerID, BPlayerID purchasingPlayerID, int id, bool startBuilt)
{
//-- FIXING PREFIX BUG ID 3213
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BEntityIDArray sockets;
   gWorld->getBuildSockets(playerID, id, pUnit, cOriginVector, 0.0f, true, true, sockets);
   if (sockets.getNumber() == 0)
      return cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3214
   const BObject* pSocket = gWorld->getObject(sockets[0]);
//--
   if (!pSocket)
      return cInvalidObjectID;

   bool doQueue = false;
   bool secondaryQueue = false;
   if (gConfig.isDefined(cConfigBuildingQueue))
   {
      secondaryQueue = pUnit->getProtoObject()->getFlagSecondaryBuildingQueue();
      if (!pUnit->isNewBuildingConstructionAllowed(NULL, NULL, secondaryQueue))
         doQueue = true;
   }

   BEntityID squadID = gWorld->createEntity(id, false, playerID, pSocket->getPosition(), cZAxisVector, cXAxisVector, startBuilt, false, false, cInvalidObjectID, playerID, pUnit->getID(), pSocket->getID(), true, 1.0, false, 0, -1, pUnit->getExplorationGroup());
//-- FIXING PREFIX BUG ID 3215
   const BSquad* pSquad = gWorld->getSquad(squadID);         
//--
   if (pSquad && pSquad->getNumberChildren()>0)
   {    
      mFlagHideParentSocket = true;

      BEntityID buildingID = pSquad->getChild(0);
      if (doQueue)
      {
         BUnit* pBaseBuilding = gWorld->getUnit(pUnit->getBaseBuilding());
         if (pBaseBuilding)
            pBaseBuilding->queueBuildOther(buildingID, secondaryQueue);
      }
      //For coop campaign we need the building to be the color of whoever purchased it, even though player 1 always owns it.  mwc 6/13/2008
      for (uint i=0; i<pSquad->getNumberChildren(); i++)
      {
         BEntityID childID = pSquad->getChild(i);
         BObject *childObj = gWorld->getObject(childID);
         if (childObj)
            childObj->setColorPlayerID(purchasingPlayerID);
         BUnit *childUnit = gWorld->getUnit(childID);
         if (childUnit)
         {
            BUnitActionBuilding* pAction = (BUnitActionBuilding*)childUnit->getActionByType(BAction::cActionTypeUnitBuilding);
            if (pAction)
               pAction->overrideCreatedByPlayerID(purchasingPlayerID);
         }
      }

      if (pUnit->getProtoObject()->getFlagUseAutoParkingLot())
      {
         BUnit* pBuilding = gWorld->getUnit(buildingID);
         if (pBuilding)
         {
            BProtoObjectID autoParkingLotObject = pBuilding->getProtoObject()->getAutoParkingLotObject();
            if (autoParkingLotObject != cInvalidProtoObjectID)
            {
               BVector parkingLotForward = pBuilding->getForward();
               BVector parkingLotRight = pBuilding->getRight();
               float buildRotation = pBuilding->getProtoObject()->getAutoParkingLotRotation();
               if (buildRotation != 0.0f)
               {
                  BMatrix rotMat;
                  rotMat.makeRotateY(Math::fDegToRad(buildRotation));
                  rotMat.transformVector(pBuilding->getForward(), parkingLotForward);
                  parkingLotRight.assignCrossProduct(cYAxisVector, parkingLotForward);
                  parkingLotRight.normalize();
               }

               BMatrix posMat;
               posMat.makeOrient(pBuilding->getForward(), cYAxisVector, pBuilding->getRight());
               posMat.setTranslation(pBuilding->getPosition());
               BVector parkingLotPos;
               posMat.transformVectorAsPoint(pBuilding->getProtoObject()->getAutoParkingLotOffset(), parkingLotPos);
               BVector origin(parkingLotPos.x, parkingLotPos.y+1000.0f, parkingLotPos.z);
               gTerrainSimRep.getHeightRaycast(origin, parkingLotPos.y, true);

               BEntityID parkingLotSquadID = gWorld->createEntity(autoParkingLotObject, false, playerID, parkingLotPos, parkingLotForward, parkingLotRight, true, false, false, cInvalidObjectID, playerID, pUnit->getID(), cInvalidObjectID, true, 1.0, false, 0, -1, pUnit->getExplorationGroup());
               if (parkingLotSquadID != cInvalidObjectID)
               {
                  BSquad* pParkingLotSquad = gWorld->getSquad(parkingLotSquadID);
                  if (pParkingLotSquad)
                  {
                     BUnit* pParkingLotUnit = pParkingLotSquad->getLeaderUnit();
                     if (pParkingLotUnit)
                        pBuilding->setAssociatedParkingLot(pParkingLotUnit->getID());
                  }
               }
            }
         }
      }

      return buildingID;
   }

   return cInvalidObjectID;
}

//==============================================================================
//==============================================================================
long BUnitActionBuilding::getBuildOtherCount(BPlayerID playerID, int id) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   long count=0;
   if(mState==cStateWorking && (currentItem.mPlayerID == playerID) && currentItem.mCurrentType==BProtoObjectCommand::cTypeBuildOther && currentItem.mCurrentID==id)
      count=1;
   long itemCount=mQueuedItems.getNumber();
   for(long i=0; i<itemCount; i++)
   {
      const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
      if(queuedItem.mType==BProtoObjectCommand::cTypeBuildOther && queuedItem.mID==id && (queuedItem.mPlayerID == playerID))
         count+=queuedItem.mCount;
   }
   return count;
}

//==============================================================================
//==============================================================================
float BUnitActionBuilding::getBuildOtherPercent(BPlayerID playerID, int id) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   if(mState==cStateWorking && (currentItem.mPlayerID == playerID) && currentItem.mCurrentType==BProtoObjectCommand::cTypeBuildOther && currentItem.mCurrentID==id)
   {
      const BUnit* pBuilding = gWorld->getUnit(currentItem.mCurrentBuildingID);
      if (pBuilding)
         return pBuilding->getBuildPercent();
      else
         return 0.0f;
   }
   return 0.0f;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::doCustomCommand(BPlayerID playerID, int commandID, bool cancel)
{
   BPlayer* pPlayer=gWorld->getPlayer(playerID);

   BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);

   BCustomCommand* pCustomCommand=gWorld->getCustomCommand(commandID);
   if (!pCustomCommand)
      return;

   int itemCount=mQueuedItems.getNumber();

   if (cancel)
   {
      for (int i=itemCount-1; i>=0; i--)
      {
//-- FIXING PREFIX BUG ID 3147
         const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
//--
         if (queuedItem.mType==BProtoObjectCommand::cTypeCustomCommand && queuedItem.mID==commandID && queuedItem.mPlayerID==playerID)
         {
            BASSERT(pCustomCommand->mQueuedCount>0);
            pCustomCommand->mQueuedCount--;
            if(pPlayer)
            {
               pPlayer->refundCost(&pCustomCommand->mCost);
            }
            mQueuedItems.erase(i);
            gWorld->notify(BEntity::cEventCustomCommandQueued, mpOwner->getID(), commandID, 0);
            return;
         }
      }
      if (currentItem.mCurrentType==BProtoObjectCommand::cTypeCustomCommand && currentItem.mCurrentID==commandID)
      {
         BASSERT(pCustomCommand->mQueuedCount>0);
         pCustomCommand->mQueuedCount--;
         if(pPlayer)
         {
            pPlayer->refundCost(&pCustomCommand->mCost);
         }
         currentItem.clear();
         gWorld->notify(BEntity::cEventCustomCommandQueued, mpOwner->getID(), commandID, 0);
      }
      return;
   }
   else
   {
      if (pCustomCommand->mLimit!=0)
      {
         if (pCustomCommand->mQueuedCount + pCustomCommand->mFinishedCount >= pCustomCommand->mLimit)
            return;
      }

      if (pCustomCommand->mQueuedCount>0 && (!pCustomCommand->mFlagAllowMultiple || !pCustomCommand->mFlagPersistent))
         return;

      if (pPlayer && !pPlayer->payCost(&pCustomCommand->mCost))
         return;

      pCustomCommand->mQueuedCount++;

      gWorld->notify(BEntity::cEventCustomCommandQueued, mpOwner->getID(), commandID, 0);

      if (!pCustomCommand->mFlagQueue && pCustomCommand->mTimer==0.0f)
      {
         // Complete immediately
         BUnitActionBuildingCurrentItem saveCurrentItem=currentItem;
         currentItem.mCurrentType=BProtoObjectCommand::cTypeCustomCommand;
         currentItem.mCurrentID=commandID;
         currentItem.mPlayerID = playerID;
         completeCustomCommand(playerID);
         currentItem=saveCurrentItem;
      }
      else
      {
         // Clear the queue so this can start processing
         if (!pCustomCommand->mFlagQueue)
            clearQueue();

         // Queue it up.
         BUnitActionBuildingQueuedItem queuedItem;
         queuedItem.mType=BProtoObjectCommand::cTypeCustomCommand;
         queuedItem.mID=(short)commandID;
         queuedItem.mCount=1;
         queuedItem.mPlayerID=playerID;
         mQueuedItems.add(queuedItem);
      }
   }
}

//==============================================================================
//==============================================================================
int BUnitActionBuilding::getCustomCommandCount(BPlayerID playerID, int commandID) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   if(mState==cStateWorking && currentItem.mCurrentType==BProtoObjectCommand::cTypeCustomCommand && currentItem.mCurrentID==commandID)
      return 1;
   long itemCount=mQueuedItems.getNumber();
   for(long i=0; i<itemCount; i++)
   {
      const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
      if(queuedItem.mType==BProtoObjectCommand::cTypeCustomCommand && queuedItem.mID==commandID)
         return 1;
   }
   return 0;
}

//==============================================================================
//==============================================================================
float BUnitActionBuilding::getCustomCommandPercent(BPlayerID playerID, int commandID) const
{
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   if(mState==cStateWorking && currentItem.mCurrentType==BProtoObjectCommand::cTypeCustomCommand && currentItem.mCurrentID==commandID)
   {
      if(currentItem.mTotalPoints==0.0f)
         return 0.0f;
      else
         return currentItem.mCurrentPoints/currentItem.mTotalPoints;
   }
   return 0.0f;
}


//==============================================================================
//==============================================================================
void BUnitActionBuilding::doSelfDestruct(bool cancel)
{
   BUnit* pBuilding=reinterpret_cast<BUnit*>(mpOwner);

   if (cancel)
   {
      if (mState == cStateFading)
      {
         if (pBuilding->getFlagBuilt())
            setState(cStateNone);
         else
            setState(cStateWorking);
         gWorld->notify(BEntity::cEventSelfDestructTime, mpOwner->getID(), 0, 0);
      }
   }
   else
   {
      if (mState != cStateFading)
      {
         if (pBuilding->getFlagBuildingConstructionPaused())
         {
            // Refund cost
            BPlayer* pPlayer=gWorld->getPlayer(mCreatedByPlayerID);
            if (pPlayer)
            {
               const BProtoObject* pBuildProto=pPlayer->getProtoObject(pBuilding->getProtoID());
               if (pBuildProto)
               {
                  BCost cost;
                  pBuildProto->getCost(pPlayer, &cost, -1);
                  pPlayer->refundCost(&cost);
               }
            }

            // This is just a foundation, so immediately cancel
            setState(cStateWait);
            gWorld->notify(BEntity::cEventBuildPercent, pBuilding->getID(), pBuilding->getProtoID(), 0);
            pBuilding->setFlagRecycled(true);
            pBuilding->kill(false);
         }
         else
         {
            if (pBuilding->getFlagBuilt())
               clearQueue();
            //mDestructTime=gWorld->getGametime()+gDatabase.getBuildingSelfDestructTime();
            mDestructTime=0.0f;      //Now that we have a verification prompt we don't need the time out.  Leaving commented out in case minds change.
            setState(cStateFading);
            gWorld->notify(BEntity::cEventSelfDestructTime, mpOwner->getID(), 0, 0);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::isSelfDestructing(float& timeRemaining) const
{
   if (mState != cStateFading)
      return false;

   DWORD gameTime = gWorld->getGametime();
   if (gameTime >= mDestructTime)
      timeRemaining=0.0f;
   else
      timeRemaining = ((mDestructTime-gameTime)*0.001f);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::isQueuedItem(BPlayerID playerID, int id) const
{
   // is this the current item?
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
   if(mState==cStateWorking && (currentItem.mPlayerID == playerID) && (currentItem.mCurrentID==id))
      return false;

   long itemCount=mQueuedItems.getNumber();
   for(long i=0; i<itemCount; i++)
   {
      const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
      if((queuedItem.mID==id) && (queuedItem.mPlayerID == playerID))
         return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::clearQueue()
{
   // Undo current and queued up trains and researches
   if(mQueuedItems.getNumber()>0)
   {
      for(long i=mQueuedItems.getNumber()-1; i>=0; i--)
      {
         BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
         for(long j=queuedItem.mCount-1; j>=0; j--)
         {
            switch(queuedItem.mType)
            {
               case BProtoObjectCommand::cTypeTrainUnit:
                  addTrain(queuedItem.mPlayerID, queuedItem.mID, -1, false, false);
                  break;
               case BProtoObjectCommand::cTypeTrainSquad:
                  addTrain(queuedItem.mPlayerID, queuedItem.mID, -1, true, false);
                  break;
               case BProtoObjectCommand::cTypeResearch:
                  addResearch(queuedItem.mPlayerID, queuedItem.mID, -1);
                  break;
               case BProtoObjectCommand::cTypeCustomCommand:
                  doCustomCommand(queuedItem.mPlayerID, queuedItem.mID, true);
                  break;
               case BProtoObjectCommand::cTypeBuildOther:
                  completeBuildOther(queuedItem.mPlayerID, queuedItem.mPurchasingPlayerID, false, false);
                  break;
               default:
                  queuedItem.clear();
                  break;
            }
         }
      }
      mQueuedItems.clear();
   }

   if(mState==cStateWorking)
   {
      for (uint i=0; i<mCurrentItems.getSize(); i++)
      {
         BPlayerID playerID=getPlayerIDForIndex(i);
         BUnitActionBuildingCurrentItem& currentItem=mCurrentItems[i];
         switch(currentItem.mCurrentType)
         {
            case BProtoObjectCommand::cTypeTrainUnit:
               addTrain(playerID, currentItem.mCurrentID, -1, false, false);
               break;
            case BProtoObjectCommand::cTypeTrainSquad:
               addTrain(playerID, currentItem.mCurrentID, -1, true, false);
               break;
            case BProtoObjectCommand::cTypeResearch:
               addResearch(playerID, currentItem.mCurrentID, -1);
               break;
            case BProtoObjectCommand::cTypeCustomCommand:
               doCustomCommand(playerID, currentItem.mCurrentID, true);
               break;
            case BProtoObjectCommand::cTypeBuildOther:
               completeBuildOther(playerID, currentItem.mPurchasingPlayerID, false, false);
               break;
            default:
               currentItem.clear();
               break;
         }
      }
   }
}

//==============================================================================
//==============================================================================
long BUnitActionBuilding::getTrainLimit(BPlayerID playerID, long id, bool squad) const
{
   return getTrainLimitAndResourceLinks(playerID, id, NULL, 0, NULL, NULL, NULL, squad);
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::getTrainCounts(BPlayerID playerID, long id, long*  pCount, long*  pMax, bool squad, bool buildingOther) const
{
   uint8 trainLimitBucket=0;
   long limit=getTrainLimitAndResourceLinks(playerID, id, NULL, 0, NULL, pCount, &trainLimitBucket, squad);
   if(limit!=-1)
   {
      *pMax=limit+(*pCount);
      if (buildingOther)
      {
         long checkCount=mQueuedItems.getNumber();
         for(long i=0; i<checkCount; i++)
         {
            const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
            if(queuedItem.mTrainLimitBucket==trainLimitBucket && queuedItem.mType==BProtoObjectCommand::cTypeBuildOther)
               *pMax-=queuedItem.mCount;
         }
      }
      return true;
   }
   else
   {
      *pMax=-1;
      return false;
   }
}

//==============================================================================
//==============================================================================
long BUnitActionBuilding::getTrainLimitAndResourceLinks(BPlayerID playerID, long id, BUBAResourceLink*  pResourceLinks, long maxResources, long*  pResourceCountOut, long*  pTrainCountOut, uint8* pTrainLimitBucketOut, bool squad) const
{
   BUnit*  pBuilding=reinterpret_cast<BUnit*>(mpOwner);
   const BProtoObject*  pBuildingProto=pBuilding->getProtoObject();
   uint8 trainLimitBucket=0;
   long trainLimit = pBuildingProto->getTrainLimit(id, squad, &trainLimitBucket);

   if (pTrainLimitBucketOut)
      *pTrainLimitBucketOut=trainLimitBucket;

   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);

   // Calculate the current train limit based on this unit types train limit less the number
   // of items already trained, currently being trained, or in the queue.
   if(pTrainCountOut)
      *pTrainCountOut=-1;

   long trainedCount=0;
   if(trainLimit!=-1)
   {
      long type = squad ? BEntityRef::cTypeTrainLimitSquad : BEntityRef::cTypeTrainLimitUnit;
      long checkCount=pBuilding->getNumberEntityRefs();
      for(long i=0; i<checkCount; i++)
      {
         BEntityRef *pEntityRef = pBuilding->getEntityRefByIndex(i);
         if (pEntityRef && pEntityRef->mData2 == trainLimitBucket && pEntityRef->mType == type)
            trainedCount++;
      }
   }
   if(trainLimit!=-1 || pTrainCountOut)
   {
      if(currentItem.mTrainLimitBucket==trainLimitBucket && 
         ((!squad && currentItem.mCurrentType==BProtoObjectCommand::cTypeTrainUnit) || 
          (squad && currentItem.mCurrentType==BProtoObjectCommand::cTypeTrainSquad)))
         trainedCount++;

      long checkCount=mQueuedItems.getNumber();
      for(long i=0; i<checkCount; i++)
      {
         const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
         if(queuedItem.mTrainLimitBucket==trainLimitBucket && 
            ((!squad && queuedItem.mType==BProtoObjectCommand::cTypeTrainUnit) || 
             (squad && queuedItem.mType==BProtoObjectCommand::cTypeTrainSquad) ||
             queuedItem.mType==BProtoObjectCommand::cTypeBuildOther))
            trainedCount+=queuedItem.mCount;
      }
   }
   if(pTrainCountOut)
      *pTrainCountOut=trainedCount;
   if(trainLimit!=-1)
   {
      trainLimit-=trainedCount;
      if(trainLimit<0)
         trainLimit=0;
   }

   // Determine if we have a gatherer link for this type of unit
   BPlayer* pPlayer=gWorld->getPlayer(playerID);
//-- FIXING PREFIX BUG ID 3151
   const BProtoObject* pTrainProto=pPlayer->getProtoObject(id);
//--
   int gatherLinkObjectType=pBuildingProto->getGatherLinkObjectType();
   if(gatherLinkObjectType!=-1 && pTrainProto->isType(gatherLinkObjectType))
   {
      if(!mFlagGatherLink)
         return 0;

      // Calculate the number of available resource slots available for gatherers 
      // based on the resources that this building is linked to. Also build a list
      // of linked resources that's sorted by distance.
      long availCount=0;
      long checkCount=pBuilding->getNumberEntityRefs();
      BYTE bytePlayerID=(BYTE)pBuilding->getPlayerID();
      for(long i=0; i<checkCount; i++)
      {
         BEntityRef *pEntityRef = pBuilding->getEntityRefByIndex(i);
         if (pEntityRef && pEntityRef->mData1 == gatherLinkObjectType && pEntityRef->mType == BEntityRef::cTypeGatherResource)
         {
            BUnit* pResource=gWorld->getUnit(pEntityRef->mID);
            if(pResource)
            {
               long gathererLimit=pResource->getProtoObject()->getGathererLimit();
               long openSlots=0;
               if(gathererLimit==-1)
               {
                  if(!pResourceLinks && !pTrainCountOut)
                     return trainLimit;
                  openSlots=-1;
                  availCount=-1;
               }
               if(gathererLimit!=-1 || pTrainCountOut)
               {
                  long gathererCount=0;
                  long checkCount2=pResource->getNumberEntityRefs();
                  for(long j=0; j<checkCount2; j++)
                  {
                     BEntityRef *pEntityRef2 = pResource->getEntityRefByIndex(j);
                     if (pEntityRef2 && pEntityRef2->mType == BEntityRef::cTypeGatherUnit)
                     {
                        gathererCount++;
                        if (pTrainCountOut && pEntityRef2->mData2 == bytePlayerID)
                           (*pTrainCountOut)++;
                     }
                     else if(pEntityRef2->mType == BEntityRef::cTypeGatherBuilding)
                        gathererCount += pEntityRef2->mData2;
                  }
                  if(gathererCount<gathererLimit)
                  {
                     openSlots=gathererLimit-gathererCount;
                     if(availCount!=-1)
                        availCount+=openSlots;
                  }
               }
               if(openSlots!=0 && pResourceLinks && *pResourceCountOut<maxResources)
               {
                  float distSqr=pResource->getPosition().distanceSqr(pBuilding->getPosition());
                  BUBAResourceLink* pLink=NULL;
                  for(long j=0; j<*pResourceCountOut; j++)
                  {
                     if(distSqr<pResourceLinks[j].mDistSqr)
                     {
                        for(long k=*pResourceCountOut; k>=j; k--)
                           pResourceLinks[k]=pResourceLinks[k-1];
                        pLink=pResourceLinks+j;
                        break;
                     }
                  }
                  if(!pLink)
                     pLink=pResourceLinks+(*pResourceCountOut);
                  pLink->mpUnit=pResource;
                  pLink->mCount=openSlots;
                  pLink->mDistSqr=distSqr;
                  (*pResourceCountOut)++;
               }
            }
         }
      }

      if(availCount!=-1 && (trainLimit==-1 || availCount<trainLimit))
         trainLimit=availCount;
   }

   return trainLimit;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::completeBuild(BPlayerID playerID, bool noCost, bool fromSave)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (mFlagBuildSetAnimRate)
   {
      pUnit->setAnimationRate(1.0f);
      mFlagBuildSetAnimRate=false;
   }

   //Tell the unit it is built
   pUnit->onBuilt(false, false, true, fromSave);

   // Create gather resource links
   const BProtoObject* pProto=pUnit->getProtoObject();
   if (pProto->getGatherLinkObjectType()!=-1 || pProto->getGatherLinkTarget()!=-1)
   {
      BEntityIDArray results(0, 16);
      if(pProto->getFlagGatherLinkSelf())
         results.add(mpOwner->getID());
      else if(pProto->getGatherLinkTarget()!=-1)
      {
         BUnitQuery query(pUnit->getPosition(), pProto->getGatherLinkRadius(), true);
         query.addObjectTypeFilter(pProto->getGatherLinkTarget());
         gWorld->getUnitsInArea(&query, &results);
      }
      long numTargets = results.getNumber();
      if (numTargets > 0)
      {
         BEntityID buildingID = mpOwner->getID();
         short gathererObjectType=(short)pProto->getGatherLinkObjectType();
         for(long j=0; j<numTargets; j++)
         {
            BUnit* pResourceUnit=gWorld->getUnit(results[j]);
            if (pResourceUnit)
            {
               if (pUnit->getFirstEntityRefByID(BEntityRef::cTypeGatherResource, results[j]) == NULL)
               {
                  pResourceUnit->addEntityRef(BEntityRef::cTypeGatherBuilding, buildingID, gathererObjectType, 0);
                  pUnit->addEntityRef(BEntityRef::cTypeGatherResource, results[j], gathererObjectType, 0);
               }
               mFlagGatherLink=true;
            }
         }
      }
   }

   pUnit->doBuildingTerrainFlatten();
 
   if (mFlagStartedBuilt)
      pUnit->doBuildingTerrainAlpha();

   MVinceEventSync_EntityBuilt(pUnit);
   
   if(pUnit)
      gGeneralEventManager.eventTrigger(BEventDefinitions::cGameEntityBuilt, gUserManager.getPrimaryUser()->getPlayerID(), this->getOwner()->getID(), pUnit->getID()); 

   // Play create sound
   if (!noCost)
   {
      BPlayerID userPlayerID = gUserManager.getPrimaryUser()->getPlayerID();
      if (playerID==userPlayerID || mpOwner->getPlayerID()==userPlayerID || ((gWorld->getFlagCoop() && mpOwner->getPlayer()->isHuman())))
      {
         // Alert the player
         const BPlayer* pPlayer = pUnit->getPlayer();
         BASSERT(pPlayer);
         pPlayer->getAlertManager()->createTrainingCompleteAlert(pUnit->getPosition(), pUnit->getID());

         gSoundManager.playCue(pProto->getSound(cObjectSoundCreate));
         gUI.playRumbleEvent(BRumbleEvent::cTypeBuildComplete);
      }
      else if (gGame.isSplitScreen())
      {
         userPlayerID = gUserManager.getSecondaryUser()->getPlayerID();
         if (playerID==userPlayerID || mpOwner->getPlayerID()==userPlayerID)
         {
            // Alert the player
            const BPlayer* pPlayer = pUnit->getPlayer();
            BASSERT(pPlayer);
            pPlayer->getAlertManager()->createTrainingCompleteAlert(pUnit->getPosition(), pUnit->getID());

            gSoundManager.playCue(pProto->getSound(cObjectSoundCreate));
            gUI.playRumbleEvent(BRumbleEvent::cTypeBuildComplete);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::completeBuildOther(BPlayerID playerID, BPlayerID purchasingPlayerID, bool noCost, bool doppleOnStart)
{
//-- FIXING PREFIX BUG ID 3158
   const BUnitActionBuildingCurrentItem& currentItem = getCurrentItemForPlayer(playerID);
//--
   
//-- FIXING PREFIX BUG ID 3159
//    const BUnit* pBuilding = gWorld->getUnit(currentItem.mCurrentBuildingID);
//--
//    if (pBuilding)
   {
      if (doppleOnStart)
      {
         BUBADelayedDopple delayedDopple;
         delayedDopple.mEntityID = currentItem.mCurrentBuildingID;
         delayedDopple.mCountDown = 2;
         mDelayedDoppleList.add(delayedDopple);

         //pBuilding->setFlagGrayMapDopples(true);
//          for (int i = 1; i < gWorld->getNumberTeams(); i++)
//             gWorld->createRevealer(i, pBuilding->getPosition(), Math::Max(pBuilding->getObstructionRadius(), 1.0f), 0);
      }
   }

   if (!noCost)
   {
      // Unreserve pop
      BPlayer* pPlayer = gWorld->getPlayer(purchasingPlayerID);
      if (pPlayer)
      {
         const BProtoObject* pBuildingProto = pPlayer->getProtoObject(currentItem.mCurrentID);
         if (pBuildingProto)
            pPlayer->refundPopFuture(pBuildingProto->getPops());
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::completeResearch(BPlayerID playerID, bool noCost)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (mFlagResearchSetAnimRate)
   {
      pUnit->setAnimationRate(1.0f);
      mFlagResearchSetAnimRate=false;
   }

//-- FIXING PREFIX BUG ID 3162
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
//--

   long unitID=pUnit->getID().asLong();
   MVinceEventSync_TechResearched(currentItem.mCurrentID, unitID);

   BPlayer* pPlayer=gWorld->getPlayer(currentItem.mPlayerID);
//-- FIXING PREFIX BUG ID 3163
   const BTeam* pTeam=pPlayer->getTeam();
//--
   int playerCount=pTeam->getNumberPlayers();

   int techID=currentItem.mCurrentID;
   pPlayer->getTechTree()->activateTech(techID, unitID, noCost, false);

   if (gWorld->getFlagCoop() && pPlayer->isHuman())
   {
      for (int j=0; j<playerCount; j++)
      {
         BPlayer* pTeamPlayer = gWorld->getPlayer(pTeam->getPlayerID(j));
         if (pTeamPlayer != pPlayer)
            pTeamPlayer->getTechTree()->activateTech(techID, unitID, true);
      }
   }

   // Generate an alert
   pPlayer->getAlertManager()->createResearchCompleteAlert(pUnit->getPosition(), pUnit->getID());

   if(pPlayer->getID()==gUserManager.getPrimaryUser()->getPlayerID() || (gGame.isSplitScreen() && pPlayer->getID() == gUserManager.getSecondaryUser()->getPlayerID()) || ((gWorld->getFlagCoop() && pPlayer->isHuman())))
   {
      bool playSound=true;
//-- FIXING PREFIX BUG ID 3160
      const BProtoTech* pProtoTech=pPlayer->getProtoTech(techID);
//--
      if (pProtoTech && pProtoTech->getFlagNoSound())
         playSound=false;
      if (playSound)
      {
         BCueIndex cue = pProtoTech->getResearchCompleteSound();
         if(cue != cInvalidCueIndex)
         {
            gSoundManager.playCue(cue);
         }
         else
            gUIGame.playSound(BSoundManager::cSoundResearchComplete);
         gUI.playRumbleEvent(BRumbleEvent::cTypeResearchComplete);
      }

      gUserManager.getPrimaryUser()->getUIContext()->playTechNotification(techID);
   }
}

//==============================================================================
//==============================================================================
BEntityID BUnitActionBuilding::completeTrain(BPlayerID playerID, bool noCost, bool forcePlaySound)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   const BProtoObject* pBuildingProto=pUnit->getProtoObject();

   BEntityID trainID = cInvalidObjectID;
   BProtoObject*  pTrainProto = NULL;
   BProtoSquad*  pTrainSquadProto = NULL;
   bool squad = false;

   // ajl 1/4/08 - turn off squads inheriting level from building per Tim
   //BSquad* pBuildingSquad=pUnit->getParentSquad();
   //int level = (pBuildingSquad ? pBuildingSquad->getLevel() : 0);
   int level=0;

//-- FIXING PREFIX BUG ID 3168
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
//--

   BPlayer* pPlayer=gWorld->getPlayer(currentItem.mPlayerID);

   if (currentItem.mCurrentType == BProtoObjectCommand::cTypeTrainUnit)
      pTrainProto = pUnit->getPlayer()->getProtoObject(currentItem.mCurrentID);
   else if (currentItem.mCurrentType == BProtoObjectCommand::cTypeTrainSquad)
   {
      squad = true;
      pTrainSquadProto = pPlayer->getProtoSquad(currentItem.mCurrentID);
      pTrainProto = pPlayer->getProtoObject(pTrainSquadProto->getUnitNode(0).mUnitType);
   }

   if (pTrainProto)
   {
      if(!noCost)
      {
         // Unreserve pop
         if(pTrainSquadProto)
         {
            BPopArray pops;
            pTrainSquadProto->getPops(pops);
            pPlayer->refundPopFuture(&pops);
            pPlayer->adjustFutureSquadCount(pTrainSquadProto->getID(), -1);
            /*
            BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pTrainSquadProto->getID());
            for (long i=0; i<pProtoSquad->getNumberUnitNodes(); i++)
               pPlayer->adjustFutureUnitCount(pProtoSquad->getUnitNode(i).mUnitType, -pProtoSquad->getUnitNode(i).mUnitCount);
            */
         }
         else
         {
            pPlayer->refundPopFuture(pTrainProto->getPops());
            pPlayer->adjustFutureUnitCount(currentItem.mCurrentID, -1);
         }
      }

      BUnit* pResource=NULL;
      if(currentItem.mCurrentLinkedResource!=-1)
         pResource=gWorld->getUnit(currentItem.mCurrentLinkedResource);

//-- FIXING PREFIX BUG ID 3164
      const BEntityRef* pParkingLotRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeAssociatedParkingLot);
//--
      BUnit* pParkingLotUnit = NULL;
      if (pParkingLotRef)
         pParkingLotUnit = gWorld->getUnit(pParkingLotRef->mID);

      /*
      BVector pos, forward, right;
      if (pParkingLotUnit)
         pos = gWorld->getSquadPlacement(pParkingLotUnit, pResource, pTrainSquadProto, pTrainProto, &forward, &right);
      else
         pos = gWorld->getSquadPlacement(pUnit, pResource, pTrainSquadProto, pTrainProto, &forward, &right);
      */
//-- FIXING PREFIX BUG ID 3165
      const BUnit* pFromUnit = (pParkingLotUnit ? pParkingLotUnit : pUnit);
//--

      // Create the new unit
      //BEntityID trainID = gWorld->createEntity(currentItem.mCurrentID, squad, pPlayer->getID(), pos, forward, right, true, false, false, cInvalidObjectID, cInvalidPlayerID, cInvalidObjectID, cInvalidObjectID, false, 1.0f, false, level);
      trainID = gWorld->createEntity(currentItem.mCurrentID, squad, pPlayer->getID(), pFromUnit->getPosition(), pFromUnit->getForward(), pFromUnit->getRight(), true, false, false, cInvalidObjectID, cInvalidPlayerID, cInvalidObjectID, cInvalidObjectID, false, 1.0f, false, level, -1, -1, false);
      BEntity* pTrainEntity=gWorld->getEntity(trainID);
      if(pTrainEntity)
      {
         // Get the squad pointer
         BSquad* pTrainSquad = NULL;
         if (squad)
            pTrainSquad = pTrainEntity->getSquad();
         else
         {
            BUnit* pTrainUnit = pTrainEntity->getUnit();
            if (pTrainUnit)
               pTrainSquad = pTrainUnit->getParentSquad();
         }

         if (pTrainSquad)
         {
            // Link the building and unit if there's a train limit
            if (pBuildingProto->getTrainLimit(currentItem.mCurrentID, squad, NULL)!=-1)
            {
               pTrainEntity->addEntityRef(BEntityRef::cTypeTrainLimitBuilding, pUnit->getID(), 0, 0);
               pUnit->addEntityRef((short)(squad ? BEntityRef::cTypeTrainLimitSquad : BEntityRef::cTypeTrainLimitUnit), trainID, (short)currentItem.mCurrentID, currentItem.mTrainLimitBucket);
            }

            // Handle gather resource links
            if(pResource)
            {
               // Update the reserved count in the building to resource link
               unreserveTrainResourceLink(currentItem.mCurrentLinkedResource, currentItem.mCurrentID);

               // Add a link between the new unit and the resource
               int gatherLinkObjectType=pBuildingProto->getGatherLinkObjectType();
               pResource->addEntityRef(BEntityRef::cTypeGatherUnit, trainID, (short)gatherLinkObjectType, (BYTE)pUnit->getPlayerID());
               pTrainEntity->addEntityRef(BEntityRef::cTypeGatherTarget, pResource->getID(), 0, 0);

               // Add a link between the training building and the new unit
               pUnit->addEntityRef(BEntityRef::cTypeGatherChild, trainID, 0, 0);
               pTrainEntity->addEntityRef(BEntityRef::cTypeGatherParent, pUnit->getID(), 0, 0);

               // Tell the squad/platoon to work on the resource
               BSquad* pSquad=pTrainEntity->getSquad();
               if (pSquad)
               {
                  BPlatoon* pPlatoon=pSquad->getParentPlatoon();
                  if (pPlatoon)
                     pPlatoon->queueWork(BSimTarget(pResource->getID()));
                  else
                     pSquad->queueWork(BSimTarget(pResource->getID()));
               }
            }

            // Have this building or the parking lot play out the birth sequence (or garrison the squad if training is locked down).
            if (pParkingLotUnit)
               pParkingLotUnit->queueTrainedSquad(pTrainSquad, !noCost || forcePlaySound);
            else
               pUnit->queueTrainedSquad(pTrainSquad, !noCost || forcePlaySound);

            // Update trigger state with the newly trained item
            updateTriggerStateTrain(currentItem.mTriggerScriptID, currentItem.mTriggerVarID, squad, trainID);

		      MVinceEventSync_EntityBuilt(pTrainEntity);

            gGeneralEventManager.eventTrigger(BEventDefinitions::cGameEntityBuilt, gUserManager.getPrimaryUser()->getPlayerID(), this->getOwner()->getID(), trainID); //not 100% sure this is the best fitting player to use
         }
      }
   }

   return (trainID);
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::completeCustomCommand(BPlayerID playerID)
{
   BASSERT(mpOwner);

//-- FIXING PREFIX BUG ID 3169
   const BUnitActionBuildingCurrentItem& currentItem=getCurrentItemForPlayer(playerID);
//--

   BCustomCommand* pCustomCommand=gWorld->getCustomCommand(currentItem.mCurrentID);
   if (!pCustomCommand)
      return;

   BASSERT(pCustomCommand->mQueuedCount>0);
   pCustomCommand->mQueuedCount--;

   pCustomCommand->mFinishedCount++;

   if (!pCustomCommand->mFlagPersistent)
      gWorld->removeCustomCommand(pCustomCommand->mID);
}

//==============================================================================
//==============================================================================
uint BUnitActionBuilding::getIndexForPlayer(BPlayerID playerID) const
{
   if (mFlagCommandableByAnyPlayer)
   {
      if (playerID<0 || playerID>=gWorld->getNumberPlayers())
      {
         BASSERT(0);
         return 0;
      }
      return playerID;
   }
   else if (mFlagCoop)
   {
      const BPlayer* pPlayer=gWorld->getPlayer(playerID);
      const BTeam* pTeam=pPlayer->getTeam();
      int playerCount=pTeam->getNumberPlayers();
      for (int i=0; i<playerCount; i++)
      {
         if (pTeam->getPlayerID(i)==playerID)
            return i;
      }
      BASSERT(0);
      return 0;
   }
   else
      return 0;
}

//==============================================================================
//==============================================================================
BPlayerID BUnitActionBuilding::getPlayerIDForIndex(uint index) const
{
   if (mFlagCommandableByAnyPlayer)
      return ((BPlayerID)index);
   else if (mFlagCoop)
      return mpOwner->getTeam()->getPlayerID((long)index);
   return mpOwner->getPlayerID();
}

//==============================================================================
//==============================================================================
BUnitActionBuildingCurrentItem& BUnitActionBuilding::getCurrentItemForPlayer(BPlayerID playerID)
{
   return mCurrentItems[getIndexForPlayer(playerID)];
}

//==============================================================================
//==============================================================================
const BUnitActionBuildingCurrentItem& BUnitActionBuilding::getCurrentItemForPlayer(BPlayerID playerID) const
{
   return mCurrentItems[getIndexForPlayer(playerID)];
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::checkTriggerState(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID, BEntityID buildingID)
{
   if (triggerScriptID == cInvalidTriggerScriptID)
      return;

   for (uint i=0; i<mCurrentItems.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 3170
      const BUnitActionBuildingCurrentItem& currentItem=mCurrentItems[i];
//--
      if (currentItem.mTriggerScriptID == triggerScriptID && currentItem.mTriggerVarID == triggerVarID)
         return;
   }

   for (uint i=0; i<mQueuedItems.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 3171
      const BUnitActionBuildingQueuedItem& queuedItem=mQueuedItems[i];
//--
      if (queuedItem.mTriggerScriptID == triggerScriptID && queuedItem.mTriggerVarID == triggerVarID)
         return;
   }

   completeTriggerState(triggerScriptID, triggerVarID, buildingID);
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::completeTriggerState(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID, BEntityID buildingID)
{
   if (triggerScriptID == cInvalidTriggerScriptID)
      return;

   BTriggerScript* pTriggerScript = gTriggerManager.getTriggerScript(triggerScriptID);
   if (!pTriggerScript)
      return;

   BTriggerVar* pTriggerVar = pTriggerScript->getTriggerVar(triggerVarID);
   if (!pTriggerVar)
      return;

   BTriggerVarBuildingCommandState* pState=pTriggerVar->asBuildingCommandState();
   if (pState)
   {
      pState->writeResult(BTriggerVarBuildingCommandState::cResultDone);
      BEntityID squadID = cInvalidObjectID;
      if (buildingID != cInvalidObjectID)
      {
         uint idType = buildingID.getType();
         if (idType == BEntity::cClassTypeSquad)
         {
            squadID = buildingID;
         }
         else if (idType == BEntity::cClassTypeUnit)
         {
//-- FIXING PREFIX BUG ID 3172
            const BUnit* pUnit = gWorld->getUnit(buildingID);
//--
            if (pUnit)
            {
               squadID = pUnit->getParentID();
            }
         }
      }
      pState->writeTrainedSquadID(squadID);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::updateTriggerStateTrain(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID, bool squad, BEntityID trainID)
{
   if (triggerScriptID == cInvalidTriggerScriptID)
      return;

   BTriggerScript* pTriggerScript = gTriggerManager.getTriggerScript(triggerScriptID);
   if (!pTriggerScript)
      return;

   BTriggerVar* pTriggerVar = pTriggerScript->getTriggerVar(triggerVarID);
   if (!pTriggerVar)
      return;

   BTriggerVarBuildingCommandState* pState=pTriggerVar->asBuildingCommandState();
   if (pState)
   {
      if (squad)
         pState->writeTrainedSquadID(trainID);
      else
      {
//-- FIXING PREFIX BUG ID 3173
         const BUnit* pUnit = gWorld->getUnit(trainID);
//--
         if (pUnit)
         {
            BEntityID parentID = pUnit->getParentID();
            if (parentID != cInvalidObjectID)
               pState->writeTrainedSquadID(parentID);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::getRecharging(bool squad, int protoID, float* pTimeRemaining) const
{
   for (int i=0; i<mRechargeList.getNumber(); i++)
   {
      const BUnitActionBuildingRecharge& recharge = mRechargeList[i];
      if (recharge.mID == protoID && recharge.mSquad == squad)
      {
         if (pTimeRemaining)
            (*pTimeRemaining) = recharge.mTimer;
         return true;
      }
   }
   return false;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::setRecharge(bool squad, int protoID, float rechargeTime)
{
   // if it's already in the array, just update it
   for (int i=0; i<mRechargeList.getNumber(); i++)
   {
      BUnitActionBuildingRecharge& recharge = mRechargeList[i];
      if (recharge.mID == protoID && recharge.mSquad == squad)
      {
         recharge.mTimer = rechargeTime;
         return;
      }
   }

   // otherwise create a new one and add it
   BUnitActionBuildingRecharge recharge;
   recharge.mID = protoID;
   recharge.mSquad = squad;
   recharge.mTimer = rechargeTime;
   mRechargeList.add(recharge);
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::queueTrainedSquad(BSquad* pSquad, bool playSound)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);

   bool trainLock = (mpOwner->getFirstEntityRefByType(BEntityRef::cTypeTrainLock) != NULL);

   const BProtoObject* pProtoObject = NULL;
   for (uint i=0 ; i<pSquad->getNumberChildren(); i++)
   {
//-- FIXING PREFIX BUG ID 3174
      const BUnit* pChild = gWorld->getUnit(pSquad->getChild(i));
//--
      if (pChild)
      {
         pUnit->containUnit(pChild->getID(), true);
         pProtoObject = pChild->getProtoObject(); //-- takes the last protoobject in the squad for the sound
      }
   }

   if (trainLock && playSound)
   {
      // Alert the player
      const BPlayer* pPlayer = pSquad->getPlayer();
      BASSERT(pPlayer);
      pPlayer->getAlertManager()->createTrainingCompleteAlert(pSquad->getPosition(), pSquad->getID());

      BPlayerID playerID = pSquad->getPlayerID();
      if (playerID==gUserManager.getPrimaryUser()->getPlayerID() || (gGame.isSplitScreen() && playerID==gUserManager.getSecondaryUser()->getPlayerID()) || ((gWorld->getFlagCoop() && pPlayer->isHuman())))
      {         
         BCueIndex cue = (pProtoObject ? pProtoObject->getSound(cObjectSoundCreate) : cInvalidCueIndex);
         if (cue != cInvalidCueIndex)
            gSoundManager.playCue(cue);
         gUI.playRumbleEvent(BRumbleEvent::cTypeTrainComplete);
      }
      playSound = false;
   }

   BUnitActionBuildingTrainedSquad trainedSquad;
   trainedSquad.mSquadID = pSquad->getID();
   trainedSquad.mPosition = cInvalidVector;
   trainedSquad.mPlotted = false;
   trainedSquad.mPlaySound = playSound;
   mTrainedSquads.add(trainedSquad);
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::updateTrainedSquadBirth(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);

   if (mTrainedSquadBirthTime > 0.0f)
   {
      mTrainedSquadBirthTime -= elapsed;
      if (mTrainedSquadBirthTime < 0.0f)
         mTrainedSquadBirthTime = 0.0f;
      else
         return;
   }

   if (mTrainedSquads.getNumber() == 0)
      return;

   BEntityIDArray squadList;
   bool playSound = false;
   BPlayerID playerID = cInvalidPlayerID;
   bool allPlotted = true;

   for (int i=0; i<mTrainedSquads.getNumber(); i++)
   {
//-- FIXING PREFIX BUG ID 3176
      const BUnitActionBuildingTrainedSquad& trainedSquad = mTrainedSquads[i];
//--
//-- FIXING PREFIX BUG ID 3177
      const BSquad* pSquad = gWorld->getSquad(trainedSquad.mSquadID);
//--
      if (pSquad)
      {
         if (playerID == cInvalidPlayerID)
         {
            if (pUnit->getTrainLock(pSquad->getPlayerID()))
               continue;
            playerID = pSquad->getPlayerID();
         }
         else
         {
            if (pSquad->getPlayerID() != playerID)
               continue;
         }
         squadList.add(trainedSquad.mSquadID);
         if (trainedSquad.mPlaySound)
            playSound = true;
         if (!trainedSquad.mPlotted)
            allPlotted = false;
      }
      else
      {
         mTrainedSquads.removeIndex(i);
         i--;
      }
   }

   if (squadList.getNumber() == 0)
      return;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);

   // Get rally point
   bool useRallyPoint=false;
   BVector rallyPoint;
   BEntityID rallyPointEntityID = cInvalidObjectID;
   if (pUnit->haveRallyPoint(playerID))
   {
      useRallyPoint=true;
      pUnit->getRallyPoint(rallyPoint, rallyPointEntityID, playerID);
   }
   else if (pPlayer->haveRallyPoint())
   {
      useRallyPoint=true;
      pPlayer->getRallyPoint(rallyPoint, rallyPointEntityID);
   }

   if (!allPlotted)
   {
      // Calculate rally point if rally point entity not specified
      if (useRallyPoint && (rallyPointEntityID == cInvalidObjectID))
      {
         // Use location near near rally point instead of directly on top of it.
         BVector dir=rallyPoint-pUnit->getPosition();
         if (dir.length() > 4.0f)
         {
            dir.normalize();
            rallyPoint -= dir * 4.0f;
         }

         BCommand* pCommand=new BCommand(-1, BCommand::cNumberCommandFlags);
         pCommand->setPlayerID(pPlayer->getID());
         pCommand->addWaypoint(rallyPoint);
         gSquadPlotter.plotSquads(squadList, pCommand);
         delete pCommand;

         const BDynamicSimArray<BSquadPlotterResult>& plotterResults = gSquadPlotter.getResults();
         for (int i=0; i<squadList.getNumber(); i++)
         {
            BVector location = (i < plotterResults.getNumber() ? plotterResults[i].getDesiredPosition() : rallyPoint);
            for (int j=0; j<mTrainedSquads.getNumber(); j++)
            {
               BUnitActionBuildingTrainedSquad& trainedSquad = mTrainedSquads[j];
               if (trainedSquad.mSquadID == squadList[i])
               {
                  trainedSquad.mPosition = location;
                  trainedSquad.mPlotted = true;
                  break;
               }
            }
         }
      }
   }

   for (int j=0; j<mTrainedSquads.getNumber(); j++)
   {
      BUnitActionBuildingTrainedSquad& trainedSquad = mTrainedSquads[j];
      if (trainedSquad.mSquadID == squadList[0])
      {
         // Use rally point entity if given.  Otherwise use use plotted position.
         BSimTarget rallyPointTarget;
         bool plotted;
         if (useRallyPoint)
         {
            rallyPointTarget.setID(rallyPointEntityID);
            rallyPointTarget.setPosition(rallyPoint);
            plotted = true;
         }
         else
         {
            rallyPointTarget.setPosition(trainedSquad.mPosition);
            plotted = trainedSquad.mPlotted;
         }

         doTrainedSquadBirth(trainedSquad.mSquadID, rallyPointTarget, plotted, trainedSquad.mPlaySound);

         // Delay birth by train or fly in animation time if there is one.
         // For now just delaying by the time it takes the squad to move 2 times one of it's units size
         // if the squad has a rally point to goto.
         // VAT 10/29/2008: hook objects that birth on top need to force the delay here even though thye may not be moving to a rally point
         bool delayBirth = (plotted || (pUnit->isType(gDatabase.getOTIDHook()) && pUnit->isType(gDatabase.getOTIDBirthOnTop())));
         if (delayBirth)
         {
            BSquad* pSquad = gWorld->getSquad(trainedSquad.mSquadID);
            if (pSquad)
            {
               bool setBirthTime = false;

               const BProtoSquad* pTrainedFromProtoSquad = pSquad->getProtoSquad();
               if (pTrainedFromProtoSquad)
               {
                  long trainerAnimType = pTrainedFromProtoSquad->getBirthTrainerAnim();

                  if (trainerAnimType != -1 && pUnit->getAnimationType(cActionAnimationTrack) == trainerAnimType)
                  {
                     float duration = pUnit->getAnimationDuration(cActionAnimationTrack);
                     if (duration > 0.0f)
                     {
                        mTrainedSquadBirthTime = duration;
                        setBirthTime = true;
                     }
                  }
               }
               
               if (!setBirthTime)
               {
                  const BUnit* pChild = pSquad->getLeaderUnit();
                  if (pChild)
                  {
                     float speed = pChild->getProtoObject()->getDesiredVelocity();
                     float size = pSquad->getObstructionRadius();
                     if (speed > 0.0f && size > 0.0f)
                        mTrainedSquadBirthTime = 1.0f + ((size * 2.0f) / speed);
                  }
               }
            }
         }
            
         mTrainedSquads.removeIndex(j);
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::doTrainedSquadBirth(BEntityID squadID, BSimTarget rallyPoint, bool useRallyPoint, bool playSound)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);

   BSquad* pTrainedSquad = gWorld->getSquad(squadID);
   if (!pTrainedSquad)
      return;

   BUnit* pTrainedUnit = pTrainedSquad->getLeaderUnit();
   if (!pTrainedUnit)
      return;

   BProtoSquad* pTrainedProtoSquad = const_cast<BProtoSquad*>(pTrainedSquad->getProtoSquad());
   BProtoObject* pTrainedProtoObject = const_cast<BProtoObject*>(pTrainedUnit->getProtoObject());

   BPlayer* pPlayer = pTrainedSquad->getPlayer();

   BCueIndex cue = cInvalidCueIndex;
   if (playSound)
   {
      if (pPlayer->getID()==gUserManager.getPrimaryUser()->getPlayerID() || (gGame.isSplitScreen() && pPlayer->getID()==gUserManager.getSecondaryUser()->getPlayerID()))
      {
         //-- use the last dude on the squad for VO sound
         BUnit* pLastUnit = gWorld->getUnit(pTrainedSquad->getChild(pTrainedSquad->getNumberChildren()-1));
         if(pLastUnit && pLastUnit->getProtoObject())   
            cue = pLastUnit->getProtoObject()->getSound(cObjectSoundCreate);
      }
   }

   BUnit* pParkingLot = gWorld->getUnit(pUnit->getAssociatedParkingLot());
   BUnit* pSpawnFrom = (pParkingLot ? pParkingLot : pUnit);

   // Get end position of the birth anim so it can be tested for obstructions.
   long birthEndBone = -1;
   BVisual* pSpawnFromVisual = pSpawnFrom->getVisual();
   if (pSpawnFromVisual)
      birthEndBone = pSpawnFromVisual->getBoneHandle(pTrainedProtoSquad->getBirthEndBone());

   BVector forward, right;
   bool gotPreferredPosition = true;
   BVector pos = gWorld->getSquadPlacement(pUnit, NULL, pTrainedProtoSquad, pTrainedProtoObject, &forward, &right, birthEndBone, gotPreferredPosition);

   // If a birth end bone wasn't specified then assume we have a preferred position.  Units without birth end bones will be things
   // like flying vehicles and the Covenant who can spawn in at any unobstructed position.
   if (birthEndBone == -1)
      gotPreferredPosition = true;

   if (pUnit->getFlagHasGarrisoned())
   {
      for (uint j = 0; j < pTrainedSquad->getNumberChildren(); j++)
      {
//-- FIXING PREFIX BUG ID 3181
         const BUnit* pChild = gWorld->getUnit(pTrainedSquad->getChild(j));
//--
         if (pChild && pChild->getFlagGarrisoned())
            pUnit->unloadUnit(pChild->getID(), false);
      }
   }

   for (uint j = 0; j < pTrainedSquad->getNumberChildren(); j++)
   {
      BUnit* pChild = gWorld->getUnit(pTrainedSquad->getChild(j));
      if (pChild)
      {
         #ifdef SYNC_Unit
            syncUnitData("BUnitActionBuilding::doTrainedSquadBirth 1", pos);
         #endif
         pChild->setPosition(pos);

         /*
         pChild->setForward(forward);
         pChild->setRight(right);
         pChild->calcUp();
         */
         BVector up;
         up.assignCrossProduct(forward, right);
         up.normalize();         

         BMatrix rotation;
         rotation.makeOrient(forward, up, right);

         pChild->setRotation(rotation);
      }
   }

   pTrainedSquad->setPosition(pos);
   pTrainedSquad->setTurnRadiusPos(pos);
   pTrainedSquad->setLeashPosition(pos);
   pTrainedSquad->setForward(forward);
   pTrainedSquad->setTurnRadiusFwd(forward);
   pTrainedSquad->setRight(right);
   pTrainedSquad->calcUp();
   pTrainedSquad->settle();

   // Move the Platoon's position as well, cause it doesn't want to be stuck in the middle of the building either..
   #ifdef _MOVE4
   BPlatoon *pPlatoon = pTrainedSquad->getParentPlatoon();
   if (pPlatoon)
      pPlatoon->setPosition(pos);
   #endif

   pTrainedSquad->updateObstruction();
   for (uint j = 0; j < pTrainedSquad->getNumberChildren(); j++)
   {
      BUnit* pChild = gWorld->getUnit(pTrainedSquad->getChild(j));
      if (pChild)
         pChild->updateObstruction();
   }

   bool gotoRallyPoint = useRallyPoint;

   bool bTrainedFromMobileUnit = false;
   bool bTrainedFromDetachedBuilding = false;
   if((pUnit->getProtoObject() != NULL) && (pUnit->getProtoObject()->getObjectClass() != cObjectClassBuilding))
      bTrainedFromMobileUnit = true;
//   else if ((pUnit->getProtoObject() != NULL) && (pUnit->getAssociatedBase() == cInvalidObjectID))
//      bTrainedFromDetachedBuilding = true;

//   if (!gConfig.isDefined(cConfigNoBirthAnims) && !bTrainedFromMobileUnit && (pTrainedProtoSquad->isFlyInBirthType() || (!bTrainedFromDetachedBuilding && pTrainedProtoSquad->isTrainedBirthType())) )
   if (gotPreferredPosition && !gConfig.isDefined(cConfigNoBirthAnims) && (pTrainedProtoSquad->isFlyInBirthType() || (!bTrainedFromDetachedBuilding && pTrainedProtoSquad->isTrainedBirthType())) )
   {
      // Do the birth anim.
      BVisual *pVisual = pSpawnFrom->getVisual();
      if (pVisual)
      {
         // Use birth bone as birth anim origin
         long landingPoint = pVisual->getBoneHandle(pTrainedProtoSquad->getBirthBone());
         if (landingPoint != -1)
         {
            BMatrix worldMatrix;
            BMatrix boneMatrix;
            BVector tempPos, tempFwd, tempRight;

            pVisual->getBone(landingPoint, &tempPos, &boneMatrix, NULL, NULL);
            boneMatrix.getForward(tempFwd);
            boneMatrix.getRight(tempRight);
            pSpawnFrom->getWorldMatrix(worldMatrix);

            worldMatrix.transformVectorAsPoint(tempPos, pos);
            worldMatrix.transformVector(tempFwd, forward);
            worldMatrix.transformVector(tempRight, right);
         }
      }

      if (pTrainedProtoSquad->isFlyInBirthType())
      {
         // If the trained squad is an aircraft, let it fly itself in, otherwise bring it in a transport
         BUnit* pLeadUnit = gWorld->getUnit(pTrainedSquad->getChild(0));
         if (pLeadUnit->getProtoObject()->getMovementType() == cMovementTypeAir)
         {
            BVector startPos = pos;
            startPos.y += 100.0f;
            #ifdef SYNC_Unit
               syncUnitData("BUnitActionBuilding::doTrainedSquadBirth 2", startPos);
            #endif
            pLeadUnit->setPosition(startPos);
            pLeadUnit->setForward(forward);
            pLeadUnit->setRight(right);
            pLeadUnit->calcUp();
         }
         else
         {
            // Fly in squad
            BSimOrder* pOrder = gSimOrderManager.createOrder();
            if (pOrder)  
               pOrder->setPriority(BSimOrder::cPrioritySim);
            BSquadActionTransport::flyInSquad(pOrder, pTrainedSquad, NULL, pos, forward, right, NULL,
               pPlayer->getID(), pPlayer->getCiv()->getTransportProtoID(), useRallyPoint, rallyPoint.getPosition(), false, cue, true, false, cInvalidVector);
            gotoRallyPoint = false;
         }
      }
      else
      {
         bool bAnimateUnits = true;
         bool applyFormation = false;
         const BProtoObject* pSpawnFromProtoObject = pSpawnFrom->getProtoObject();
         if (pTrainedProtoSquad->getBirthAnim(pSpawnFromProtoObject->getTrainerType()) == -1)
            bAnimateUnits = false;
         if (pSpawnFromProtoObject->getFlagTrainerApplyFormation() && (pTrainedSquad->getNumberChildren() > 1))
            applyFormation = true;

         // Figure out offset from when squad was settled to new birth position.  This will be used to apply the
         // formation so units will remain spread out when running their birth anims.
         BVector positionOffset(0.0f);
         if (applyFormation)
            positionOffset.assignDifference(pos, pTrainedSquad->getPosition());

         // Train squad from building
         for (uint j = 0; j < pTrainedSquad->getNumberChildren(); j++)
         {
            BUnit* pChild = gWorld->getUnit(pTrainedSquad->getChild(j));
            if (pChild)
            {
               #ifdef SYNC_Unit
                  syncUnitData("BUnitActionBuilding::doTrainedSquadBirth 3", pos);
               #endif
               if (applyFormation)
                  pChild->setPosition(pChild->getPosition() + positionOffset);
               else
                  pChild->setPosition(pos);
               pChild->setForward(forward);
               pChild->setRight(right);
               pChild->calcUp();
               if (bAnimateUnits)
                  pChild->setPhysicsKeyFramed(true);

               // If subupdating enabled then reset the interpolation matrices so it doesn't briefly render at its
               // old temporary location when it was contained.
               if (gEnableSubUpdating)
               {
                  BMatrix matrix;
                  pChild->getWorldMatrix(matrix);
                  pChild->setInterpolationMatrices(matrix);
               }

               pChild->startExistSound();
            }
         }

         if (bAnimateUnits)
            pTrainedSquad->playBirthAnimation(pSpawnFrom->getProtoObject()->getTrainerType());
      }

      if ((pTrainedProtoSquad->getBirthTrainerAnim() != -1) && !pSpawnFrom->isAnimationLocked())
      {
         pSpawnFrom->setAnimation(mID, BObjectAnimationState::cAnimationStateTrain, pTrainedProtoSquad->getBirthTrainerAnim(), false, false, -1, true);
         pSpawnFrom->computeAnimation();
         pSpawnFrom->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
      }
   }
   else
   {
      // If subupdating enabled then reset the interpolation matrices so it doesn't briefly render at its
      // old temporary location when it was contained.
      if (gEnableSubUpdating)
      {
         for (uint j = 0; j < pTrainedSquad->getNumberChildren(); j++)
         {
            BMatrix matrix;
            BUnit* pChild = gWorld->getUnit(pTrainedSquad->getChild(j));
            if (pChild)
            {
               pChild->getWorldMatrix(matrix);
               pChild->setInterpolationMatrices(matrix);
            }
         }
      }
   }

   if (gotoRallyPoint)
   {
      //Go to rally point
      BPlatoon* pPlatoon=pTrainedSquad->getParentPlatoon();
      if (pPlatoon)
         pPlatoon->queueWork(rallyPoint, BSimOrder::cPriorityUser);
      else
         pTrainedSquad->queueWork(rallyPoint, BSimOrder::cPriorityUser);
   }
   else if (bTrainedFromMobileUnit)
   {
      // Find a clear spot near the back of the training unit and rally there
      bool bClearSpotFound = false;
      int8 numTries = 0;
      const BProtoObject* pProtoObj = pUnit->getProtoObject();
      float radiusX = 0.0f;
      float radiusZ = 0.0f;
      float spacing = 10.0f;
      if (pProtoObj)
      {
         radiusX = pProtoObj->getObstructionRadiusX();
         radiusZ = pProtoObj->getObstructionRadiusZ();
      }

      while (!bClearSpotFound && (numTries < 8))
      {
         BVector testLocation = pUnit->getPosition();
         switch (numTries)
         {
         case 0:
            testLocation += forward * (1.5f * spacing + radiusZ);
            break;
         case 1:
            testLocation += forward * (spacing + radiusZ);
            testLocation -= right * (spacing + radiusX);
            break;
         case 2:
            testLocation += forward * (spacing + radiusZ);
            testLocation += right * (spacing + radiusX);
            break;
         case 3:
            testLocation -= right * (spacing + radiusZ);
            break;
         case 4:
            testLocation += right * (spacing + radiusZ);
            break;
         case 5:
            testLocation -= forward * (spacing + radiusZ);
            testLocation -= right * (spacing + radiusX);
            break;
         case 6:
            testLocation -= forward * (spacing + radiusZ);
            testLocation += right * (spacing + radiusX);
            break;
         case 7:
            testLocation -= forward * (spacing + radiusZ);
            break;
         }

         long obstructionQuadTrees = 
            BObstructionManager::cIsNewTypeCollidableNonMovableUnit | // Unit that can't move                                                       
            BObstructionManager::cIsNewTypeBlockLandUnits;            // Terrain that blocks any combo of movement that includes land-based movement
         bool obstructions = gObsManager.testObstructions(testLocation, 1.0f, 0.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, pUnit->getPlayerID());
         if (!obstructions)
         {
            bClearSpotFound = true;
            rallyPoint = testLocation;
         }
         numTries++;
      }

      if (bClearSpotFound)
      {
         //Go to (makeshift) rally point
         BPlatoon* pPlatoon=pTrainedSquad->getParentPlatoon();
         if (pPlatoon)
            pPlatoon->queueWork(rallyPoint, BSimOrder::cPriorityUser);
         else
            pTrainedSquad->queueWork(rallyPoint, BSimOrder::cPriorityUser);
      }

   }

   // Play create sound
   if (playSound)
   {
      // Alert the player
      const BPlayer* pPlayer = pTrainedSquad->getPlayer();
      BASSERT(pPlayer);
      pPlayer->getAlertManager()->createTrainingCompleteAlert(pTrainedSquad->getPosition(), pTrainedSquad->getID());

      if (cue != cInvalidCueIndex)
         gSoundManager.playCue(cue);
      gUI.playRumbleEvent(BRumbleEvent::cTypeTrainComplete);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::updateRechargeList(float elapsed)
{
   for (int i=0; i<mRechargeList.getNumber(); i++)
   {
      BUnitActionBuildingRecharge& recharge = mRechargeList[i];
      int id = recharge.mID;
      bool squad = recharge.mSquad;
      recharge.mTimer -= elapsed;
      if (recharge.mTimer <= 0.0f)
      {
         squad = recharge.mSquad;
         mRechargeList.removeIndex(i);
         i--;
      }
      if (squad)
         gWorld->notify(BEntity::cEventTrainSquadPercent, mpOwner->getID(), id, 0);
      else
         gWorld->notify(BEntity::cEventTrainPercent, mpOwner->getID(), id, 0);
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::updateMoveSquadsFromObstruction(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);

   const BOPObstructionNode* pObsNode = mpOwner->getObstructionNode();
   if (!pObsNode || pObsNode->mType == BObstructionManager::cObsTypeNonCollidableUnit)
      return true;

   BEntityID buildingSquadID = mpOwner->getParentID();
   BOPQuadHull* pHull = (BOPQuadHull*) pObsNode->getHull(); // Cast to remove const
   if (!pHull)
      return true;

   mMoveSquadsTimer -= elapsed;
   if (mMoveSquadsTimer > 0.0f)
      return false;

   // Prevent obstructionManager from updating the LRP tree with the building's obstruction
   mpOwner->setFlagIsBuilt(false);

   // To get the obstruction manager to find the squads in the obstruction, we need a collidable obstruction - but we need 
   // to make it non-collidable after telling the squads to move so they will not collide with the building's foundation.
   pUnit->setFlagCollidable(true);
   pUnit->updateObstruction();

   bool ready = false;
   bool failNow = false;

   // Base sockets always have an obstruction so don't bother with the check.  Units shouldn't get in a position to block base buildings.
   // It's actually possible to get a unit in that position because the movement system does radius checks and can use relaxed obstructions.
   // So to avoid problems with that just early out for bases.
   if (pUnit->isType(gDatabase.getOTIDBase()))
      return true;

   if (mMoveSquadsCount == 0 || mMoveSquadsCount == 4 || mMoveSquadsCount == 8 || mMoveSquadsCount == 12)
   {
      // Attempt to move squads that are directly on this building.
      if (!gWorld->moveSquadsFromObstruction(buildingSquadID, pHull, false, mMoveSquadsCount%4, failNow))
         ready = true;
   }
   else
   {
      // Just check to see if we are still blocked.
      if (!gWorld->moveSquadsFromObstruction(buildingSquadID, pHull, true, 0, failNow))
         ready  = true;
   }

   if (ready)
      return true;

   pUnit->setFlagCollidable(false);
   pUnit->updateObstruction();

   mMoveSquadsCount++;
   if ((mMoveSquadsCount == 16) || failNow)
   {
      // Cancel build since we've been trying to move squads for too long.
      doBuild(mCreatedByPlayerID, true, false, false);
   }
   else
      mMoveSquadsTimer=1.0f;

   return false;
}

//==============================================================================
//==============================================================================
void BUnitActionBuilding::updateBuildAnimRate()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);

   const BProtoObject* pBuildProto=pUnit->getProtoObject();

   float buildPoints = pBuildProto->getBuildPoints();
   if (buildPoints == mLastBuildPointsForAnimRate && gWorld->getFlagQuickBuild() == mFlagLastQuickBuild)
      return;

   if (pUnit->isControllerFree(BActionController::cControllerAnimation))
   {
      float rate = 1.0f;
      if (pBuildProto->getFlagScaleBuildAnimRate())
      {
         float animationDuration = pUnit->getAnimationDuration(cActionAnimationTrack);
         float buildDuration = buildPoints / ((mpProtoAction) ? mpProtoAction->getWorkRate(mpOwner->getID()) : pUnit->getWorkRateScalar());
         rate = animationDuration / buildDuration;
      }
      else
      {
         if (buildPoints > 0.0f)
         {
            const BProtoObject* pBaseProto = gDatabase.getGenericProtoObject(pBuildProto->getID());
            if (pBaseProto)
               rate *= pBaseProto->getBuildPoints() / buildPoints;
         }
      }
      if (gWorld->getFlagQuickBuild())
         rate *= 30.0f;
      else
      {
         BPlayer* pPlayer = pUnit->getPlayer();
         if (pPlayer)      
            rate *= pPlayer->getAIBuildSpeedMultiplier();   // Sets speed multiplier for AI handicapping in deathmatch.  1.0 all other times.
      }
      if (rate != 1.0f || mLastBuildPointsForAnimRate != -1.0f)
      {
         pUnit->setAnimationRate(rate);
         mFlagBuildSetAnimRate=true;
         pUnit->computeAnimation();
      }
   }

   mLastBuildPointsForAnimRate = buildPoints;
   mFlagLastQuickBuild = gWorld->getFlagQuickBuild();
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASSARRAY(pStream, saveType, mQueuedItems, uint8, 200);
   GFWRITECLASSARRAY(pStream, saveType, mCurrentItems, uint8, 20);
   GFWRITECLASSARRAY(pStream, saveType, mRechargeList, uint8, 100);
   GFWRITECLASSARRAY(pStream, saveType, mTrainedSquads, uint8, 200);
   GFWRITEARRAY(pStream, BUBADelayedDopple, mDelayedDoppleList, uint8, 100);
   GFWRITEVAR(pStream, BPlayerID, mCreatedByPlayerID);
   GFWRITEVAR(pStream, DWORD, mBuildTime);
   GFWRITEVAR(pStream, DWORD, mDestructTime);
   GFWRITEVAR(pStream, float, mTrainedSquadBirthTime);
   GFWRITEVAR(pStream, int, mMoveSquadsCount);
   GFWRITEVAR(pStream, float, mMoveSquadsTimer);
   GFWRITEVAR(pStream, float, mRebuildTimer);
   GFWRITEVAR(pStream, float, mPrevBuildPct);
   GFWRITEVAR(pStream, float, mLastBuildPointsForAnimRate);
   GFWRITESTRING(pStream, BUString, mDescriptionOverride, 1000);
   GFWRITEBITBOOL(pStream, mFlagGatherLink);
   GFWRITEBITBOOL(pStream, mFlagAutoBuild);
   GFWRITEBITBOOL(pStream, mFlagCompleteBuildOnNextUpdate);
   GFWRITEBITBOOL(pStream, mFlagStartedBuilt);
   GFWRITEBITBOOL(pStream, mFlagCommandableByAnyPlayer);
   GFWRITEBITBOOL(pStream, mFlagCoop);
   GFWRITEBITBOOL(pStream, mFlagResearchSetAnimRate);
   GFWRITEBITBOOL(pStream, mFlagBuildSetAnimRate);
   GFWRITEBITBOOL(pStream, mFlagObstructionIsClear);
   GFWRITEBITBOOL(pStream, mFlagDescriptionOverride);
   GFWRITEBITBOOL(pStream, mFlagLastQuickBuild);
   GFWRITEBITBOOL(pStream, mFlagHideParentSocket);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuilding::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASSARRAY(pStream, saveType, mQueuedItems, uint8, 200);
   GFREADCLASSARRAY(pStream, saveType, mCurrentItems, uint8, 20);
   GFREADCLASSARRAY(pStream, saveType, mRechargeList, uint8, 100);
   GFREADCLASSARRAY(pStream, saveType, mTrainedSquads, uint8, 200);
   if (BAction::mGameFileVersion >= 5)
      GFREADARRAY(pStream, BUBADelayedDopple, mDelayedDoppleList, uint8, 100);
   GFREADVAR(pStream, BPlayerID, mCreatedByPlayerID);
   GFREADVAR(pStream, DWORD, mBuildTime);
   GFREADVAR(pStream, DWORD, mDestructTime);
   GFREADVAR(pStream, float, mTrainedSquadBirthTime);
   GFREADVAR(pStream, int, mMoveSquadsCount);
   GFREADVAR(pStream, float, mMoveSquadsTimer);
   GFREADVAR(pStream, float, mRebuildTimer);
   GFREADVAR(pStream, float, mPrevBuildPct);
   if (BAction::mGameFileVersion >= 15)
      GFREADVAR(pStream, float, mLastBuildPointsForAnimRate);
   GFREADSTRING(pStream, BUString, mDescriptionOverride, 1000);
   GFREADBITBOOL(pStream, mFlagGatherLink);
   GFREADBITBOOL(pStream, mFlagAutoBuild);
   GFREADBITBOOL(pStream, mFlagCompleteBuildOnNextUpdate);
   GFREADBITBOOL(pStream, mFlagStartedBuilt);
   GFREADBITBOOL(pStream, mFlagCommandableByAnyPlayer);
   GFREADBITBOOL(pStream, mFlagCoop);
   GFREADBITBOOL(pStream, mFlagResearchSetAnimRate);
   GFREADBITBOOL(pStream, mFlagBuildSetAnimRate);
   GFREADBITBOOL(pStream, mFlagObstructionIsClear);
   GFREADBITBOOL(pStream, mFlagDescriptionOverride);
   if (BAction::mGameFileVersion >= 15)
      GFREADBITBOOL(pStream, mFlagLastQuickBuild);
   if (BAction::mGameFileVersion >= 16)
      GFREADBITBOOL(pStream, mFlagHideParentSocket);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuildingQueuedItem::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BYTE, mType);
   GFWRITEVAR(pStream, BYTE, mCount);
   GFWRITEVAR(pStream, short, mID);
   GFWRITEVAR(pStream, long, mLinkedResource);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BPlayerID, mPurchasingPlayerID);
   GFWRITEVAR(pStream, BTriggerScriptID, mTriggerScriptID);
   GFWRITEVAR(pStream, BTriggerVarID, mTriggerVarID);
   GFWRITEVAR(pStream, uint8, mTrainLimitBucket);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuildingQueuedItem::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BYTE, mType);
   GFREADVAR(pStream, BYTE, mCount);
   GFREADVAR(pStream, short, mID);
   GFREADVAR(pStream, long, mLinkedResource);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BPlayerID, mPurchasingPlayerID);
   GFREADVAR(pStream, BTriggerScriptID, mTriggerScriptID);
   GFREADVAR(pStream, BTriggerVarID, mTriggerVarID);
   GFREADVAR(pStream, uint8, mTrainLimitBucket);

   switch (mType)
   {
      case BProtoObjectCommand::cTypeTrainUnit     : gSaveGame.remapProtoObjectID(mID); break;
      case BProtoObjectCommand::cTypeTrainSquad    : gSaveGame.remapProtoSquadID(mID); break;
      case BProtoObjectCommand::cTypeResearch      : gSaveGame.remapProtoTechID(mID); break;
      case BProtoObjectCommand::cTypeBuildOther    : gSaveGame.remapProtoObjectID(mID); break;
      case BProtoObjectCommand::cTypeCustomCommand : break;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuildingCurrentItem::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, long, mState);
   GFWRITEVAR(pStream, long, mCurrentType);
   GFWRITEVAR(pStream, long, mCurrentID);
   GFWRITEVAR(pStream, BEntityID, mCurrentBuildingID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BPlayerID, mPurchasingPlayerID);
   GFWRITEVAR(pStream, long, mCurrentLinkedResource);
   GFWRITEVAR(pStream, float, mCurrentPoints);
   GFWRITEVAR(pStream, float, mNextUpdatePercent);
   GFWRITEVAR(pStream, float, mTotalPoints);
   GFWRITEVAR(pStream, BTriggerScriptID, mTriggerScriptID);
   GFWRITEVAR(pStream, BTriggerVarID, mTriggerVarID);
   GFWRITEVAR(pStream, uint8, mTrainLimitBucket);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuildingCurrentItem::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, long, mState);
   GFREADVAR(pStream, long, mCurrentType);
   GFREADVAR(pStream, long, mCurrentID);
   GFREADVAR(pStream, BEntityID, mCurrentBuildingID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BPlayerID, mPurchasingPlayerID);
   GFREADVAR(pStream, long, mCurrentLinkedResource);
   GFREADVAR(pStream, float, mCurrentPoints);
   GFREADVAR(pStream, float, mNextUpdatePercent);
   GFREADVAR(pStream, float, mTotalPoints);
   GFREADVAR(pStream, BTriggerScriptID, mTriggerScriptID);
   GFREADVAR(pStream, BTriggerVarID, mTriggerVarID);
   GFREADVAR(pStream, uint8, mTrainLimitBucket);

   switch (mCurrentType)
   {
      case BProtoObjectCommand::cTypeTrainUnit     : gSaveGame.remapProtoObjectID(mCurrentID); break;
      case BProtoObjectCommand::cTypeTrainSquad    : gSaveGame.remapProtoSquadID(mCurrentID); break;
      case BProtoObjectCommand::cTypeResearch      : gSaveGame.remapProtoTechID(mCurrentID); break;
      case BProtoObjectCommand::cTypeBuildOther    : gSaveGame.remapProtoObjectID(mCurrentID); break;
      case BProtoObjectCommand::cTypeCustomCommand : break;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuildingRecharge::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, int, mID);
   GFWRITEVAR(pStream, float, mTimer);
   GFWRITEVAR(pStream, bool, mSquad);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuildingRecharge::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, int, mID);
   GFREADVAR(pStream, float, mTimer);
   GFREADVAR(pStream, bool, mSquad);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuildingTrainedSquad::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BEntityID, mSquadID);
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVAR(pStream, bool, mPlotted);
   GFWRITEVAR(pStream, bool, mPlaySound);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuildingTrainedSquad::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BEntityID, mSquadID);
   GFREADVECTOR(pStream, mPosition);
   GFREADVAR(pStream, bool, mPlotted);
   GFREADVAR(pStream, bool, mPlaySound);
   return true;
}
