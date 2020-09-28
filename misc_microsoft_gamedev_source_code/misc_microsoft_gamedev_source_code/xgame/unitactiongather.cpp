//==============================================================================
// unitactiongather.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactiongather.h"
#include "entity.h"
#include "player.h"
#include "protoobject.h"
#include "tactic.h"
#include "unit.h"
#include "world.h"
#include "usermanager.h"
#include "user.h"
#include "syncmacros.h"
#include "uigame.h"
#include "team.h"
#include "configsgame.h"
#include "game.h"
#include "SimOrderManager.h"
#include "skullmanager.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionGather, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionGather::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(mpProtoAction);
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   if (mFlagPersistent)
      mFlagAuto=true;

   mFlagSetGathering=false;

   //Take our controllers.  If we can't get them, fail.
   if (!grabControllers())
   {
      BAction::disconnect();
      return (false);
   }
      
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionGather::disconnect()
{
   //Release our controllers.
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Kick out another floaty text if we have left over resources in the pool
   handleFloatyText(pUnit->getPlayerID(), mCurrResourceID, 0.001f);

   releaseControllers();

   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   if (pTarget && mFlagSetGathering)
      pTarget->removeGatherer();

   //Remove any move opp we may have given the unit.  DO NOT remove the action
   //since we're in the middle of stuff with this one.
   pUnit->removeOpp(mMoveOppID, false);
   mMoveOppID=BUnitOpp::cInvalidID;
   
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionGather::init(void)
{
   if (!BAction::init())
      return(false);
      
   mFlagAuto=false;
   mFlagConflictsWithIdle=true;

   mTarget.reset();
   mOppID=BUnitOpp::cInvalidID;

   for (uint i=0; i < cMaxGather; i++)
      mElapsed[i]=0.0f;
   mResourceTotal=0.0f;
   mCurrResourceID=0;

   mMoveOppID=BUnitOpp::cInvalidID;
   mFutureState=cStateNone;
   mNewMode=BSquadAI::cModeNormal;
   mbSwitchAnims=false;

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionGather::setState(BActionState state)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 4929
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;

   //Validate our target.
   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   if (!mFlagAuto && !pTarget && (state != cStateFailed))
      return (setState(cStateFailed));

   switch (state)
   {
      //Moving.
      case cStateMoving:
      {
         BVector desiredLocation;
         if (!pSquad->getDesiredChildLocation(pUnit->getID(), desiredLocation))
         {
            setState(cStateFailed);
            return (false);
         }

         break;
      }

      case cStateWorking:
      {
         //Remove any move opp we may have given the unit.  DO NOT remove the action
         //since we're in the middle of stuff with this one.
         pUnit->removeOpp(mMoveOppID, false);
         mMoveOppID=BUnitOpp::cInvalidID;

         for (uint i=0; i < cMaxGather; i++)
            mElapsed[i]=0.0f;
         if (!mFlagAuto)
         {
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateWork, mpProtoAction->getAnimType());
            pUnit->computeAnimation();
            BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateWork, mpProtoAction->getAnimType()));
         }
         else
         {
            mbSwitchAnims = true;
            if(isResourceActive(mpProtoAction->getResourceType()))
               mNewMode = BSquadAI::cModeLockdown;
            else
               mNewMode = BSquadAI::cModeNormal;
         }

         //Face the target.
         //TODO: Fix the snap.
         if (pTarget && !pUnit->getFlagPhysicsControl())
         {
            BVector right, up;
            BVector targetDirection=pTarget->getPosition()-pUnit->getPosition();
            targetDirection.y=0.0f;
            targetDirection.safeNormalize();
            right.assignCrossProduct(pUnit->getUp(), targetDirection);
            right.safeNormalize();
            up.assignCrossProduct(targetDirection, right);
            up.normalize();

            BMatrix rot;
            rot.makeOrient(targetDirection, up, right);
            pUnit->setRotation(rot, true);
         }

         if (pTarget && !mFlagSetGathering)
         {
            pTarget->addGatherer();
            mFlagSetGathering=true;
         }
         break;
      }

      case cStateDone:
      case cStateFailed:
      case cStateNone:
      {
         if (pTarget && mFlagSetGathering)
         {
            pTarget->removeGatherer();
            mFlagSetGathering=false;
         }

         //Remove any move opp we may have given the unit.  DO NOT remove the action
         //since we're in the middle of stuff with this one.
         pUnit->removeOpp(mMoveOppID, false);
         mMoveOppID=BUnitOpp::cInvalidID;

         if (state == cStateDone)
            pUnit->completeOpp(mOppID, true);
         else if (state == cStateFailed)
            pUnit->completeOpp(mOppID, false);

         releaseControllers();
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionGather::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   const BSquad* pSquad=pUnit->getParentSquad();
   BASSERT(pSquad);

   //If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
         return (true);
      mFutureState=cStateNone;
   }

   //Validate our target.
   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   if (!mFlagAuto && !pTarget)
   {
      setState(cStateFailed);
      return (true);
   }

   switch (mState)
   {
      case cStateNone:
      {
         //Check range.
         if (!validateRange())
         {
            setState(cStateMoving);
            break;
         }
         //At this point, we need to have the controllers to do anything.
         if (!grabControllers())
            break;

         // Can't go on if the animation is locked
         if (pUnit->isAnimationLocked())
            break;

         //We're good, figure out what we're doing.
         if (mFlagAuto)
         {
            #ifdef SYNC_UnitAction
            syncUnitActionData("BUnitActionGather::update ownerID", pUnit->getID().asLong());
            #endif
            setState(cStateWorking);
         }
         else
         {
            long ownerPlayerID=pUnit->getPlayerID();
            long targetPlayerID=pTarget->getPlayerID();
            long capturePlayerID=pTarget->getCapturePlayerID();
            #ifdef SYNC_UnitAction
            syncUnitActionData("BUnitActionGather::update ownerID", pUnit->getID().asLong());
            syncUnitActionData("BUnitActionGather::update mTargetID", mTarget.getID().asLong());
            syncUnitActionData("BUnitActionGather::update ownerPlayerID", ownerPlayerID);
            syncUnitActionData("BUnitActionGather::update targetPlayerID", targetPlayerID);
            syncUnitActionData("BUnitActionGather::update capturePlayerID", capturePlayerID);
            #endif

            //If we can take this, great.  Else, fail.
            if (targetPlayerID == 0)
            {
               if ((capturePlayerID == -1) || (capturePlayerID == ownerPlayerID))
                  setState(cStateWorking);
            }
            else if (targetPlayerID == ownerPlayerID)
               setState(cStateWorking);
            else
               setState(cStateFailed);
         }
         break;
      }

      case cStateMoving:
      {
         //Check range.
         if (validateRange())
            setState(cStateNone);
         break;
      }

      case cStateWorking:
      {
         //If we're moving, then just wait.
         if (mMoveOppID != BUnitOpp::cInvalidID)
            break;
      
         //If we've lost the controllers or are frozen, go back to None.
         if (!validateControllers() || pSquad->isFrozen())
         {
            setState(cStateNone);
            break;
         }
         //Check range.
         if (!validateRange())
         {
            setState(cStateFailed);
            break;
         }

         if (!mpProtoAction)
            break;

         // Take unit's work rate scalar into account
         float gatherAmount = mpProtoAction->getWorkRate(mpOwner->getID()) * elapsed;

         if (mFlagAuto)
         {
            if(!isResourceActive(mpProtoAction->getResourceType()))
               break;
            #ifdef SYNC_UnitAction
            syncUnitActionData("BUnitActionGather::update unitID", pUnit->getID().asLong());
            syncUnitActionData("BUnitActionGather::update gatherAmount", gatherAmount);
            #endif
            addResource(mpProtoAction->getResourceType(), gatherAmount);
         }
         else
         {
            if (!isResourceActive(mpProtoAction->getResourceType()))
            {
               setState(cStateDone);
               break;
            }

            if (pTarget->getFlagUnlimitedResources())
            {
               #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionGather::update unitID", pUnit->getID().asLong());
               syncUnitActionData("BUnitActionGather::update gatherAmount", gatherAmount);
               #endif
               addResource(mpProtoAction->getResourceType(), gatherAmount);                  
            }
            else
            {
               float targetAmount=pTarget->getResourceAmount();
               bool empty=false;
               if (gatherAmount>=targetAmount)
               {
                  gatherAmount=targetAmount;
                  empty=true;
               }

               #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionGather::update unitID", pUnit->getID().asLong());
               syncUnitActionData("BUnitActionGather::update gatherAmount", gatherAmount);
               syncUnitActionData("BUnitActionGather::update targetAmount", targetAmount);
               #endif
               pTarget->adjustResourceAmount(-gatherAmount);
               addResource(mpProtoAction->getResourceType(), gatherAmount, BPlayer::cFlagFromGather);

               if (empty)
               {
                  //Report if a collectable is gathered
                  if (mpProtoAction->getResourceType() == 3)//3 = collectable.  No way to not hardcode this?  It is for optimization only.
                  {
                     if( pTarget->getProtoObject() && pTarget->getProtoObject()->isType(gDatabase.getOTIDBlackBox()) )
                     {
                        gCollectiblesManager.reportBlackBoxCollected(NULL, pTarget->getProtoObject());
                     }
                     else if( pTarget->getProtoObject() && pTarget->getProtoObject()->isType(gDatabase.getOTIDSkull()) )
                     {
                        gCollectiblesManager.reportSkullCollected(NULL, pTarget->getProtoObject()->getDBID());
                     }

                     gSoundManager.playCue( "play_ui_flare" );
                  }

                  setState(cStateDone);
                  break;
               }
            }
         }
         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionGather::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad=pUnit->getParentSquad();
   BDEBUG_ASSERT(pSquad);

   #ifdef DEBUGACTION
   mpOwner->debug("UnitActionGather::notify:: AID=%d, eventType=%d, senderID=%d, data1=%d, data2=%d.", mID, eventType, senderID, data1, data2);
   #endif
   //If we have a completed opp that's our move opp, deal with that.
   if ((eventType == BEntity::cEventOppComplete) &&
      (senderID == mpOwner->getID()) &&
      (data1 == mMoveOppID))
   {
      if (validateRange())
         mFutureState=cStateNone;
      else
         mFutureState=cStateFailed;
   }
   else if ((eventType == BEntity::cEventAnimLoop) && mbSwitchAnims)
   {
      pSquad->getSquadAI()->setMode(mNewMode);
      pUnit->computeVisual();
      mbSwitchAnims = false;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionGather::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));
   mTarget=target;
}

//==============================================================================
//==============================================================================
void BUnitActionGather::setOppID(BUnitOppID oppID)
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
uint BUnitActionGather::getPriority() const
{
//-- FIXING PREFIX BUG ID 4930
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
bool BUnitActionGather::validateControllers() const
{
   if (mFlagAuto)
      return (true);

   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 4931
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
bool BUnitActionGather::grabControllers()
{
   if (mFlagAuto)
      return (true);

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
void BUnitActionGather::releaseControllers()
{
   if (mFlagAuto)
      return;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
bool BUnitActionGather::validateRange() const
{
   if (mFlagAuto)
      return (true);

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 4932
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   BDEBUG_ASSERT(mpProtoAction);
   if (!pSquad)
      return false;

   //Determine our range.
   float range;
   if (mTarget.isRangeValid())
      range=mTarget.getRange();
   else
      range=mpProtoAction->getMaxRange(pUnit);
   //See if we're GTG.
   return (pSquad->isChildInRange(false, pUnit->getID(), mTarget, 0.0f, range, 1.0f));
}

//==============================================================================
//==============================================================================
bool BUnitActionGather::isResourceActive(long resourceID) const
{
   if (mpProtoAction->getFlagTeamShare() && gConfig.isDefined(cConfigEnableCapturePointResourceSharing))
   {
      // Return true if the resource is active for any player on the owner's team
      BTeam *pTeam = mpOwner->getTeam();
      int32 numPlayers = pTeam->getNumberPlayers();
      for (int32 i = 0; i < numPlayers; i++)
      {
         int32 playerID = pTeam->getPlayerID(i);
         BPlayer *pPlayer = gWorld->getPlayer(playerID);
         if (pPlayer->isResourceActive(resourceID))
            return true;
      }

      return false;
   }
   else
   {
      return mpOwner->getPlayer()->isResourceActive(resourceID);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionGather::addResource(long resourceID, float amount, uint flags)
{
   // Save resourceID so that we can refer to it on disconnect to see whether we need to round up to the next floaty text increment
   mCurrResourceID = resourceID;

   BUnit *pUnit = mpOwner->getUnit();

   BPlayer* pUnitPlayer = pUnit->getPlayer();
   if (gWorld->getFlagCoop() && pUnitPlayer->isHuman() && !gConfig.isDefined(cConfigCoopSharedResources))
   {
      // Co-op support: Give resource to all players on the team
      float splitAmount = amount;
      if (gDatabase.getResourceDeductable(resourceID))
         splitAmount *= gDatabase.getCoopResourceSplitRate();
//-- FIXING PREFIX BUG ID 4934
      const BTeam* pTeam=pUnit->getTeam();
//--
      int playerCount=pTeam->getNumberPlayers();
      for (int i=0; i<playerCount; i++)
      {
         BPlayer* pTeamPlayer=gWorld->getPlayer(pTeam->getPlayerID(i));
         handleFloatyText(pTeamPlayer->getID(), resourceID, splitAmount);
#ifdef SYNC_Player
         syncPlayerCode("BUnitActionGather::addResource1");
#endif
         pTeamPlayer->addResource(resourceID, splitAmount, flags);
      }
   }
   else if (mpProtoAction->getFlagTeamShare() && gConfig.isDefined(cConfigEnableCapturePointResourceSharing) && !gConfig.isDefined(cConfigCoopSharedResources))
   {
      // Get number of active players on the owner's team that use this resource
      BTeam *pTeam = mpOwner->getTeam();
      int32 numPlayers = pTeam->getNumberPlayers();
      float numActivePlayers = 0.0f;
      for (int32 i = 0; i < numPlayers; i++)
      {
         int32 playerID = pTeam->getPlayerID(i);
         BPlayer *pPlayer = gWorld->getPlayer(playerID);
         if (pPlayer->getPlayerState() == BPlayer::cPlayerStatePlaying)
         {
            numActivePlayers++;
         }
      }
      float recNumActivePlayers = 1.0f / numActivePlayers;

      // Divide amount by that number of players
      amount *= recNumActivePlayers;

      // Add resource
      for (int32 i = 0; i < numPlayers; i++)
      {
         int32 playerID = pTeam->getPlayerID(i);
         BPlayer *pPlayer = gWorld->getPlayer(playerID);
         if ((pPlayer->getPlayerState() == BPlayer::cPlayerStatePlaying) && (pPlayer->isResourceActive(resourceID)))
         {
            handleFloatyText(playerID, resourceID, amount);
#ifdef SYNC_Player
            syncPlayerCode("BUnitActionGather::addResource2");
#endif
            pPlayer->addResource(resourceID, amount, flags);
         }
      }
   }
   else
   {
      handleFloatyText(pUnit->getPlayerID(), resourceID, amount);
#ifdef SYNC_Player
      syncPlayerCode("BUnitActionGather::addResource3");
#endif
      pUnitPlayer->addResource(resourceID, amount, flags);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionGather::handleFloatyText(BPlayerID playerID, long resourceID, float amount)
{
   // Display floaty text if we reach the bucket size
   if (gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() == playerID || (gGame.isSplitScreen() && gUserManager.getUser(BUserManager::cSecondaryUser)->getPlayerID() == playerID))
   {
      BUnit *pUnit = mpOwner->getUnit();
      BSquad* pSquad = pUnit->getParentSquad();
      if (pUnit && pSquad)
      {
         // Only display floaty text if this is the leader unit - otherwise pass on amount to squad leader
         if (pUnit == pSquad->getLeaderUnit())
         {
            int32 resourceBucketSize = gUIGame.getResourceTextVisualBucketSize(resourceID);
            if (resourceBucketSize > 0)
            {
               mResourceTotal += amount;
               if (mResourceTotal >= resourceBucketSize)
               {
                  mpOwner->getUnit()->generateFloatingTextForResourceGather(resourceID);
                  mResourceTotal -= resourceBucketSize;
               }
            }
         }
         else
         {
            if (pSquad->getLeaderUnit())
            {
               BUnitActionGather* leadersGatherAction = reinterpret_cast<BUnitActionGather*>(pSquad->getLeaderUnit()->getActionByType(BAction::cActionTypeUnitGather));
               if (leadersGatherAction)
                  leadersGatherAction->handleFloatyText(playerID, resourceID, amount);
            }
         }
      }
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionGather::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BUnitOppID, mMoveOppID);
   GFWRITEVAR(pStream, float, mElapsed[cMaxGather]);
   GFWRITEVAR(pStream, float, mResourceTotal);
   GFWRITEVAR(pStream, long, mCurrResourceID);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEVAR(pStream, int, mNewMode);
   GFWRITEBITBOOL(pStream, mFlagAuto);
   GFWRITEBITBOOL(pStream, mFlagSetGathering);
   GFWRITEBITBOOL(pStream, mbSwitchAnims);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionGather::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BUnitOppID, mMoveOppID);
   GFREADVAR(pStream, float, mElapsed[cMaxGather]);
   GFREADVAR(pStream, float, mResourceTotal);
   if (BAction::mGameFileVersion >= 13)
   {
      GFREADVAR(pStream, long, mCurrResourceID);
      gSaveGame.remapResourceID(mCurrResourceID);
   }
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADVAR(pStream, int, mNewMode);
   GFREADBITBOOL(pStream, mFlagAuto);
   GFREADBITBOOL(pStream, mFlagSetGathering);
   GFREADBITBOOL(pStream, mbSwitchAnims);
   return true;
}
