//==============================================================================
// squadactiontransport.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "squadactiontransport.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "actionmanager.h"
#include "soundmanager.h"
#include "squadactiongarrison.h"
#include "squadactionungarrison.h"
#include "PlatoonActionMove.h"
#include "visualitem.h"
#include "database.h"
#include "protoobject.h"
#include "visual.h"
#include "EntityGrouper.h"
#include "config.h"
#include "configsgame.h"
#include "commands.h"
#include "squadplotter.h"
#include "SimOrderManager.h"
#include "ui.h"
#include "Formation2.h"
#include "protosquad.h"
#include "alert.h"
#include "simhelper.h"
#include "unitactionplayblockinganimation.h"
#include "physics.h"
#include "physicsobject.h"
#include "formationmanager.h"
#include "unitactionjoin.h"
#include "tactic.h"

//==============================================================================
//==============================================================================
bool BTransportChildActionInfo::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BActionID, mActionID);
   GFWRITEVAR(pStream, BEntityID, mEntityID);
   return true;
}

//==============================================================================
//==============================================================================
bool BTransportChildActionInfo::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BActionID, mActionID);
   GFREADVAR(pStream, BEntityID, mEntityID);
   return true;
}

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionTransport, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionTransport::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(pEntity);
   BASSERT(pSquad);

   if (mFlagControllableTransport)
   {      
      // Bail if target location is invalid
      mDropOffLocation = pOrder->getTarget().isPositionValid() ? pOrder->getTarget().getPosition() : cInvalidVector;      
      if ((mDropOffLocation == cInvalidVector))
         return (false);

      mFlagTransportSquads = true;
      mFlagUnloadSquads = true;
      mFlagMoveToRallyPoint = false;
      mSquadList = pSquad->getGarrisonedSquads();
   }
/*
   else if(pSquad->getSelectType(pSquad->getPlayer()->getTeamID()) != cSelectTypeTarget)
   {
      bool selectable = false;
      pSquad->setFlagSelectable(selectable);
      uint numChildren = pSquad->getNumberChildren();
      for (uint i = 0; i < numChildren; i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit)
         {
            pUnit->setFlagSelectable(selectable);
         }
      }
   }
*/

   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionTransport::disconnect()
{
   // MS 11/20/2008: PHX-18529
   for(long i = 0; i < mSquadList.getNumber(); i++)
   {
      BSquad* pSquad = gWorld->getSquad(mSquadList[i]);
      if(pSquad)
      {
         pSquad->setFlagIsTransporting(false);

         BSquadActionGarrison* pGarrisonAction = reinterpret_cast<BSquadActionGarrison*>(pSquad->getActionByType(BAction::cActionTypeSquadGarrison));
         if(pGarrisonAction && pGarrisonAction->getParentAction() == this)
            pGarrisonAction->setParentAction(NULL);

         BSquadActionUngarrison* pUngarrisonAction = reinterpret_cast<BSquadActionUngarrison*>(pSquad->getActionByType(BAction::cActionTypeSquadUngarrison));
         if(pUngarrisonAction && pUngarrisonAction->getParentAction() == this)
            pUngarrisonAction->setParentAction(NULL);
      }
   }

   removeOpp();
   removeChildActions();

   if (mpChildOrder)
   {
      mpChildOrder->decrementRefCount();
      if (mpChildOrder->getRefCount() == 0)
         gSimOrderManager.markForDelete(mpChildOrder);
      mpChildOrder = NULL;
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagLoadSquads = false;
   mFlagFlyInFrom = false;
   mFlagTransportSquads = false;
   mFlagUnloadSquads = false;
   mFlagMoveToRallyPoint = false;
   mFlagFlyOffTo = false;
   mFlagUseMaxHeight = false;
   mFlagConflictsWithIdle = true;
   mFlagActionFailed = false;
   mFlagTrainingAlert = false;
   mFlagNewDropOff = false;
   mFlagBlocked = false;
   mFlagNotifyReceived = false;
   mTarget.reset();   
   mChildActionInfos.clear();
   mFutureState = cStateNone;
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
   mFlagAnyFailed = false;
   mFlagAttackMove = false;
   mRallyPoint = cInvalidVector;
   mpChildOrder = NULL;
   mFlagUseFacing = false;
   mFlagControllableTransport = false;
   mFacing = cInvalidVector;
   mCompletionSoundCue = cInvalidCueIndex;
   mBlockedTimeStamp = 0;
   mMovementTimer = 0.0f;
   mMovementPos = cInvalidVector;
   mFlagIgnoreNotify = false;
   mFlagUseAnimations = false;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport::setState(BActionState state)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   switch (state)
   {
      // Fly in animation
      case cStateIncoming:
         if (!mFlagUseAnimations)
         {
            setState(cStateFlyIn);
            return (true);
         }

         if (!flyInTransports())
         {
            setState(cStateFailed);
            return (false);
         }
         break;

      // Fly in from location
      case cStateFlyIn:
         if (!mFlagFlyInFrom)
         {
            setState(cStateLoading);
            return (true);
         }

         mMovementTimer = 0.0f;
         if (!flyInFrom())
         {
            setState(cStateReturning);
            return (true);
         }         
         break;

      // Load squads
      case cStateLoading:
         if (!mFlagLoadSquads)
         {
            setState(cStateMoving);
            return (true);
         }
         
         mBlockedTimeStamp = gWorld->getGametime() + gDatabase.getTransportLoadBlockTime();         
         if (!loadSquads())
         {
            if(mFlagUseAnimations)
               setState(cStateReturning);
            else
               setState(cStateFlyOff);
            return (true);
         }
         break;

      // Transport squads
      case cStateMoving:
         if(pSquad->getLeaderUnit())
            pSquad->getLeaderUnit()->setFlagBlockContain(true); // can no longer garrison into this once we're on our way

         if (!mFlagTransportSquads && !mFlagNewDropOff)
         {
            setState(cStateUnloading);
            return (true);
         }

         mMovementTimer = 0.0f;
         if (!moveTransports())
         {
            setState(cStateReturning);
            return (true);
         }
         break;

      // Unloading of squads is blocked by movable squads
      case cStateBlocked:
         mBlockedTimeStamp = gWorld->getGametime() + gDatabase.getTransportBlockTime();
         mFlagBlocked = true;
         break;

      // Unload squads
      case cStateUnloading:
         {
            if (!mFlagUnloadSquads || !anySquadsLoaded())
            {
               setState(cStateFlyOff);
               return (true);
            }

            // Evaluate the drop-off location
            bool unload = false;
            bool blocked = false;
            BVector suggestedLocation = mDropOffLocation;
            if (BSimHelper::evaluateExitLocation(pSquad, mSquadList, mDropOffLocation, suggestedLocation))
            {
               if (mDropOffLocation.almostEqual(suggestedLocation))
               {
                  unload = true;
               }
            }
            // No valid drop-off location so try to move squads out of the way
            else if (BSimHelper::clearExitLocation(pSquad, mSquadList, suggestedLocation, blocked))
            {
               mDropOffLocation = suggestedLocation;
               if (blocked)
               {
                  setState(cStateBlocked);
               }
               else
               {
                  mFlagNewDropOff = true;
                  setState(cStateMoving);
               }
               return (true);
            }
            // No valid location and cannot move blocking squads out of the way
            else
            {
               BASSERTM(false, "No valid drop off location could be evaluated!");
               if (mFlagControllableTransport)
               {
                  setState(cStateFailed);
               }
               else
               {
                  setState(cStateReturning);
               }
               return (true);
            }

            // Move to new drop-off location
            if (!unload)
            {
               mDropOffLocation = suggestedLocation;
               mFlagNewDropOff = true;
               setState(cStateMoving);
               return (true);
            }

            // Unload the squads
            if (!unloadSquads())
            {
               if (mFlagControllableTransport)
               {
                  setState(cStateFailed);
               }
               else
               {
                  setState(cStateReturning);
               }
               return (true);
            }
         }
         break;

      // Fly off to location
      case cStateFlyOff:
         if (mFlagControllableTransport)
            return (setState(cStateDone));

         if (!mFlagFlyOffTo)
         {
            setState(cStateReturning);
            return (true);
         }

         mMovementTimer = 0.0f;
         if (!flyOffTo())
         {
            setState(cStateReturning);
            return (true);
         }
         break;

      // Transports leave area
      case cStateReturning:         

         // MS 10/24/2008: PHX-16295
         if(mFlagUseAnimations)
         {
            pSquad->setFlagSelectable(false);
            for(uint i = 0; i < pSquad->getNumberChildren(); i++)
            {
               BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(i));
               if(pChildUnit)
                  pChildUnit->setFlagInvulnerable(true);
            }
         }
         else
         {
            setState(cStateDone);
            return (true);
         }

         if (!flyOffTransports())
         {
            setState(cStateFailed);
            return (false);
         }
         break;
 
      // Done/Failed.      
      case cStateDone:
      case cStateFailed:
         //XXXHalwes - 7/30/2008 - Testing only, remove this.
         //BASSERT(state != cStateFailed);
         removeOpp();
         removeChildActions();            
         if (!mFlagControllableTransport)
            selfDestruct();
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport::update(float elapsed)
{
//-- FIXING PREFIX BUG ID 2173
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   // If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
      {
         return (true);
      }

      mFutureState = cStateNone;
   }

   // MS 11/17/2008: PHX-18237, get rid of child garrison actions that have a NULL or dead squad
   for (long i = 0; i < mChildActionInfos.getNumber(); i++)
   {
      BEntity* pEntity = gWorld->getEntity(mChildActionInfos[i].mEntityID);
      if(!pEntity)
      {
         mChildActionInfos.removeIndex(i);
         i--;
         continue;
      }

      if(!pEntity->isAlive() || (pEntity->getSquad() && pEntity->getSquad()->isDown()))
      {
         BAction* pChildAction = pEntity->findActionByID(mChildActionInfos[i].mActionID);
         if (pChildAction)
         {
            if(pEntity->getSquad())
               pEntity->getSquad()->removeOrder(mpChildOrder, true, false);
         }

         mChildActionInfos.removeIndex(i);
         i--;
      }
   }


   switch (mState)
   {
      // First update
      case cStateNone:
         if (mFlagControllableTransport)
            setState(cStateMoving);
         else
            setState(cStateIncoming);
         positionSquads();
         break;

      // Fly in animation
      case cStateIncoming:
         {
            if (pSquad)
            {
               BUnit* pUnit = pSquad->getLeaderUnit();
               if (pUnit && pUnit->getFlagOccluded())
               {
                  BUnitActionPlayBlockingAnimation* pAction = reinterpret_cast<BUnitActionPlayBlockingAnimation*>(pUnit->getActionByType(BAction::cActionTypeUnitPlayBlockingAnimation));
                  if (pAction && pAction->getFlagHasPlayedAnimation())
                  {
                     pUnit->setFlagOccluded(false);
                  }
               }
            }
         }
         break;

      // Fly in from location
      case cStateFlyIn:
         //XXXHalwes - 7/24/2208 - Debug
         //gpDebugPrimitives->addDebugSphere(mPickupLocation, 5.0f, cDWORDBlue);   
         if (!mFlagNotifyReceived && (mChildActionInfos.getSize() == 0))
         {
            BPlatoon* pPlatoon = pSquad->getParentPlatoon();
            BASSERT(pPlatoon);
            if (pPlatoon)
            {
               BPlatoonActionMove* pPlatoonActionMove = reinterpret_cast<BPlatoonActionMove*>(pPlatoon->getActionByType(BAction::cActionTypePlatoonMove));
               if (pPlatoonActionMove)
               {
                  BTransportChildActionInfo info;
                  info.mActionID = pPlatoonActionMove->getID();
                  info.mEntityID = pPlatoon->getID();
                  mChildActionInfos.add(info);
                  pPlatoonActionMove->setParentAction(this);
               }
            }            
         }
         else if (mFlagNotifyReceived && (validateUnitDist(mPickupLocation, 2.0f) || validateUnitMotion(elapsed, 1.0f, 1.0f)))
         {
            mFutureState = cStateLoading;
            mFlagNotifyReceived = false;
         }
         break;

      // Load squads
      case cStateLoading:
         if (mFlagNotifyReceived)
         {
            mFlagNotifyReceived = false;
            mBlockedTimeStamp = 0;
            mFutureState = cStateMoving;
         }
         else if (mChildActionInfos.getNumber() == 0)
         {
            mFutureState = cStateUnloading;
         }
         else if (gWorld->getGametime() >= mBlockedTimeStamp)
         {
            mFlagNotifyReceived = false;
            mBlockedTimeStamp = 0;
            if (forceLoadSquads())
            {               
               mFutureState = cStateMoving;            
            }
            else
            {
               mFutureState = cStateUnloading;
            }
         }
         break;

      // Transport squads
      case cStateMoving:
      {
         //XXXHalwes - 7/24/2208 - Debug
         //gpDebugPrimitives->addDebugSphere(mDropOffLocation, 5.0f, cDWORDGreen);   
         if (!mFlagNotifyReceived && (mChildActionInfos.getSize() == 0))
         {
            BPlatoon* pPlatoon = pSquad->getParentPlatoon();
            BASSERT(pPlatoon);
            if (pPlatoon)
            {
               BPlatoonActionMove* pPlatoonActionMove = reinterpret_cast<BPlatoonActionMove*>(pPlatoon->getActionByType(BAction::cActionTypePlatoonMove));
               if (pPlatoonActionMove)
               {
                  BTransportChildActionInfo info;
                  info.mActionID = pPlatoonActionMove->getID();
                  info.mEntityID = pPlatoon->getID();
                  mChildActionInfos.add(info);
                  pPlatoonActionMove->setParentAction(this);
               }
            }            
         }
         else if (mFlagNotifyReceived && (validateUnitDist(mDropOffLocation, 2.0f) || validateUnitMotion(elapsed, 1.0f, 1.0f)))
         {
            mFlagNewDropOff = false;
            mFutureState = cStateUnloading;
            mFlagNotifyReceived = false;
         }

         // while we're moving, tell the squads that we're transporting that they moved too. This fixes some bugs with optimizations around last move time.
         DWORD time = gWorld->getGametime();
         uint numSquadsTransporting = mSquadList.getSize();
         for (uint i = 0; i < numSquadsTransporting; i++)
         {
            BSquad* pTransportSquad = gWorld->getSquad(mSquadList[i]);
            if (!pTransportSquad)
               continue;

            pTransportSquad->setLastMoveTime(time);
         }

         break;
      }

      // Unloading of squads is blocked by movable squads
      case cStateBlocked:
         {
            // If blocked time and has expired re-evaluate
            if (mFlagBlocked && (gWorld->getGametime() > mBlockedTimeStamp))
            {
               mFlagBlocked = false;
               setState(cStateUnloading);
            }
         }
         break;

      // Unload squads
      case cStateUnloading:
         {
            if(mChildActionInfos.getNumber() == 0)
            {
               setState(cStateFlyOff);
            }
         }
         break;

      // Fly off to location
      case cStateFlyOff:
         //XXXHalwes - 7/24/2208 - Debug
         //gpDebugPrimitives->addDebugSphere(mFlyOffLocation, 5.0f, cDWORDRed);   
         if (!mFlagNotifyReceived && (mChildActionInfos.getSize() == 0))
         {
            BPlatoon* pPlatoon = pSquad->getParentPlatoon();
            BASSERT(pPlatoon);
            if (pPlatoon)
            {
               BPlatoonActionMove* pPlatoonActionMove = reinterpret_cast<BPlatoonActionMove*>(pPlatoon->getActionByType(BAction::cActionTypePlatoonMove));
               if (pPlatoonActionMove)
               {
                  BTransportChildActionInfo info;
                  info.mActionID = pPlatoonActionMove->getID();
                  info.mEntityID = pPlatoon->getID();
                  mChildActionInfos.add(info);
                  pPlatoonActionMove->setParentAction(this);
               }
            }            
         }
         else if (mFlagNotifyReceived && (validateUnitDist(mFlyOffLocation, 2.0f) || validateUnitMotion(elapsed, 1.0f, 1.0f)))
         {
            mFutureState = cStateReturning;
            mFlagNotifyReceived = false;
         }
         break;

      // Transports leave area
      case cStateReturning:
         break;
   }

   #if !defined (BUILD_FINAL)
      if (gConfig.isDefined(cConfigRenderSimDebug))
      {      
         BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
         if (pSquad)
         {
            pSquad->debugRender();
            BUnit* pUnit = pSquad->getLeaderUnit();
            if (pUnit)
            {
               pUnit->debugRender();
            }
            BPlatoon* pPlatoon = pSquad->getParentPlatoon();                                 
            if (pPlatoon)
            {
               pPlatoon->debugRender();
            }
         }
      }
   #endif

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionTransport::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   if (mFlagIgnoreNotify)
   {
      return;
   }

   bool actionsDone = false;
   switch (eventType)
   {
      case BEntity::cEventActionFailed:
         {
            for(long i = 0; i < mChildActionInfos.getNumber(); i++)
            {
               if(mChildActionInfos[i].mActionID == (BActionID)data1)
                  mFlagActionFailed = true;
            }

            actionsDone = actionsComplete((BActionID)data1);
         }
         break;

      case BEntity::cEventActionDone:         
         actionsDone = actionsComplete((BActionID)data1);
         break;

      // Check if this squad's units have completed their animation opportunities
      case BEntity::cEventOppComplete:
         {
            //Data1:  OppID.
            //Data2:  Success.
            if (data1 == mUnitOppID)
            {                  
               if (!data2)
               {                  
                  mFlagAnyFailed = true;                  
               }

               mUnitOppIDCount--;
               if (mUnitOppIDCount == 0)
               {
                  actionsDone = true;
                  if (mFlagAnyFailed)
                  {
                     mFlagActionFailed = true;
                  }
                  else
                  {
                     mFlagActionFailed = false;
                  }
                  mFlagAnyFailed = false;

                  mUnitOppID = BUnitOpp::cInvalidID;
               }
            }            
         }
         break;
   }    

   if (actionsDone && mFlagActionFailed)
   {
      //XXXHalwes - 7/30/2008 - Testing only, remove me.
      //BASSERT(false);
      switch (mState)
      {
         // Fly in animation
         case cStateIncoming:
            mFutureState = cStateFailed;
            break;

         // Fly in from location
         case cStateFlyIn:
            mFutureState = cStateReturning;
            break;

         // Load squads
         case cStateLoading:
            if (cleanUpLoadFailure())
            {
               if (anySquadsLoaded())
               {
                  mFutureState = cStateMoving;
               }
               else
               {
                  mFutureState = cStateUnloading;
               }
            }
            else
            {
               mFutureState = cStateUnloading;
            }
            break;

         // Transport squads
         case cStateMoving:
            mFlagNewDropOff = false;
            mFutureState = cStateUnloading;
            break;

         // Unload squads
         case cStateUnloading:
            if (mFlagControllableTransport)
            {
               mFutureState = cStateFailed;
            }
            else
            {
               mFutureState = cStateReturning;
            }
            break;

         // Fly off to location
         case cStateFlyOff:
            mFutureState = cStateReturning;
            break;

         // Transports leave area
         case cStateReturning:
            mFutureState = cStateFailed;
            break;
      } 
      mFlagActionFailed = false;
   }
   else if (actionsDone && !mFlagActionFailed)
   {
      switch (mState)
      {
         // Fly in animation
         case cStateIncoming:
            mFutureState = cStateFlyIn;
            break;

         // Fly in from location
         case cStateFlyIn:
            mFlagNotifyReceived = true;
            break;

         // Load squads
         case cStateLoading:
            mFlagNotifyReceived = true;            
            break;

         // Transport squads
         case cStateMoving:
            mFlagNotifyReceived = true;
            break;

         // Unload squads
         case cStateUnloading:
            mFutureState = cStateFlyOff;
            break;

         // Fly off to location
         case cStateFlyOff:
            mFlagNotifyReceived = true;
            break;

         // Transports leave area
         case cStateReturning:
            mFutureState = cStateDone;

            BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
            BASSERT(pSquad);
            if (pSquad)
            {
               BUnit* pUnit = pSquad->getLeaderUnit();
               if (pUnit)
               {        
                  // Occlude unit to hide pops until destroy occurs
                  if (!pUnit->getFlagOccluded())
                  {
                     pUnit->setFlagOccluded(true);
                  }

                  // Lock animation so that idle does not take over
                  pUnit->lockAnimation(5000, false);                  
               }
            }
            break;
      }       
   }
}

//===========================================================================================================================
// Return a list of squads that are valid for transporting
//===========================================================================================================================
void BSquadActionTransport::notifySquadWasCommanded(BEntityID squadID)
{
   // grab squad
   BSquad* pSquadToLose = gWorld->getSquad(squadID);
   if(!pSquadToLose)
      return;

   // if it's already garrisoned, just bail
   if(pSquadToLose->getFlagGarrisoned() || pSquadToLose->getFlagAttached())
      return;

   // if it's early enough, nuke from our squad list, otherwise bail
   switch(mState)
   {
      case cStateIncoming:
      case cStateFlyIn:
      case cStateLoading:
         {
            // Clear the transporting squad's current garrison order
            uint numChildActions = mChildActionInfos.getSize();
            for (uint i = 0; i < numChildActions; i++)
            {
               BAction* pChildAction = pSquadToLose->findActionByID(mChildActionInfos[i].mActionID);
               if (pChildAction)
               {
                  actionsComplete(mChildActionInfos[i].mActionID);            
                  pSquadToLose->removeOrder(mpChildOrder, true, false);
                  pChildAction->setState(cStateDone);
                  break;
               }
            }                           

            mSquadList.remove(squadID);
            pSquadToLose->setFlagIsTransporting(false);
         }
         break;

      case cStateMoving:
      case cStateBlocked:
      case cStateUnloading:
      case cStateFlyOff:
      case cStateReturning:         
      case cStateDone:
      case cStateFailed:
         return;
         break;
   }
}

//===========================================================================================================================
// Return a list of squads that are valid for transporting
//===========================================================================================================================
void BSquadActionTransport::filterTransportableSquads(const BEntityIDArray &squadsToTransport, BPlayerID playerID /*= cInvalidPlayerID*/, bool checkTransportingFlag /*= true*/, BEntityIDArray &results)
{
   BASSERT(&squadsToTransport != &results);

   results.resize(0);
   uint numSquadsToTransport = squadsToTransport.getSize();
   for (uint i = 0; i < numSquadsToTransport; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadsToTransport[i]);
      if (!pSquad)
         continue;

      // no proto object
      if (!pSquad->getProtoObject())
         continue;

      // Wrong player
      if ((playerID != cInvalidPlayerID) && (pSquad->getPlayerID() != playerID))
         continue;

      // Not transportable
      if (!pSquad->getProtoObject()->isType(gDatabase.getOTIDTransportable()))
         continue;

      // Dead or downed
      if (!pSquad->isAlive() || pSquad->isDown())
         continue;

      // Garrisoning or ungarrisoning or already garrisoned
      if (pSquad->getFlagGarrisoned() || pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsUngarrisoning())
         continue;

      // Hitching or unhitching
      if (pSquad->getFlagIsHitching() || pSquad->getFlagIsUnhitching())
         continue;

      // Locked down
      if (pSquad->isLockedDown())
         continue;

      // Frozen
      if (pSquad->isFrozen())
         continue;

      // ignore units that are boarding other units
      if (pSquad->getLeaderUnit())
      {
         const BUnitActionJoin* pJoin = static_cast<BUnitActionJoin*>(pSquad->getLeaderUnit()->getActionByType(BAction::cActionTypeUnitJoin));
         if (pJoin && pJoin->getProtoAction() && pJoin->getProtoAction()->getJoinType() == BUnitActionJoin::cJoinTypeBoard)
            continue;
      }

      // Being boarded (e.g., by a Spartan)
      bool bBeingBoarded = false;
      for(uint childIndex = 0; childIndex < pSquad->getNumberChildren(); childIndex++)
      {
         BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(childIndex));
         if(pChildUnit && pChildUnit->getFlagBeingBoarded())
         {
            bBeingBoarded = true;
            break;
         }
      }
      if(bBeingBoarded)
         continue;
     

      // Already being transported
      if (checkTransportingFlag && pSquad->getFlagIsTransporting())
         continue;

      // Keep the squad
      results.add(pSquad->getID());
   }
}

//===========================================================================================================================
// Calculate the number of transports required to transport the provided squads
//===========================================================================================================================
uint BSquadActionTransport::calculateNumTransports(const BEntityIDArray &transportingSquads)
{
   uint numTransportingSquads = transportingSquads.getSize();
   if (numTransportingSquads <= 0)
   {
      return (0);
   }

   // Find the transport proto object ID
   const BPlayer* pPlayer = NULL;
   BProtoObjectID transportProtoID = cInvalidProtoObjectID;   
   for (uint i = 0; i < numTransportingSquads; i++)
   {
//-- FIXING PREFIX BUG ID 2152
      const BSquad* pSquad = gWorld->getSquad(transportingSquads[i]);
//--
      if (pSquad)
      {
         pPlayer = pSquad->getPlayer();
         if (pPlayer)
         {
//-- FIXING PREFIX BUG ID 2151
            const BCiv* pCiv = pPlayer->getCiv();
//--
            if (pCiv)
            {
               transportProtoID = pCiv->getTransportProtoID();               
               if (transportProtoID != cInvalidProtoObjectID)
               {
                  break;
               }
            }
         }         
      }
   }
   if (transportProtoID == cInvalidProtoObjectID)
   {
      return (0);
   }

   // Get the transport ProtoObject
//-- FIXING PREFIX BUG ID 2153
   const BProtoObject* pTransportProtoObject = pPlayer ? pPlayer->getProtoObject(transportProtoID) : gDatabase.getGenericProtoObject(transportProtoID);
//--
   if (!pTransportProtoObject)
   {
      return (0);
   }

   // Calculate amount of pop per transport
   int popNeeded = 0;
   int maxPop = pTransportProtoObject->getMaxContained();
   if (maxPop == 0)
   {
      return (0);
   }

   // Get contains data
   const BObjectTypeIDArray& transportContains = pTransportProtoObject->getContains();
   uint numTransportContains = transportContains.getSize();

   // Iterate squads
   for (uint i = 0; i < numTransportingSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(transportingSquads[i]);
      if (pSquad)
      {
         const BProtoObject* pProtoObject = pSquad->getProtoObject();
         if (pProtoObject)
         {
            // Is this a transportable squad?
            bool transportable = false;
            for (uint j = 0; j < numTransportContains; j++)
            {
               if (pProtoObject->isType(transportContains[j]))
               {
                  transportable = true;
                  break;
               }
            }

            if (transportable)
            {                 
               const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
               if (pProtoSquad)
               {
                  BPopArray pops;
                  pProtoSquad->getPops(pops);
                  if (pops.getNumber() > 0)
                  {
                     float pop = pops[0].mCount;
                     popNeeded += Math::FloatToIntRound(pop);
                  }
               }
            }                     
         }
      }               
   }

   // Calculate appropriate number of transports for the whole group
   uint totalNumTransports = popNeeded / maxPop;
   if ((popNeeded % maxPop) > 0)
   {
      totalNumTransports++;
   }

   return (totalNumTransports);
}

//===========================================================================================================================
// Calculate the transport data
//===========================================================================================================================        
void BSquadActionTransport::calculateTransportData(const BEntityIDArray &squadsToTransport, BPlayerID playerID, const BEntityIDArray &transportSquads, BVector dropOffLocation, bool updateTransports, BTransportDataArray &transportData)
{
   uint totalNumTransports = transportSquads.getSize();
   transportData.resize(0);
   if (totalNumTransports == 0)
      return;
   for (uint i = 0; i < totalNumTransports; i++)
   {
      BTransportData tempTransportData;
      transportData.add(tempTransportData);
   }

   // Transport ProtoObject contains info
   BProtoObjectIDArray transportContains;
   uint numTransportContains = 0;
   int maxTransportPop = 0;
//-- FIXING PREFIX BUG ID 2158
   const BSquad* pTransportSquad = gWorld->getSquad(transportSquads[0]);
//--
   if (pTransportSquad)
   {
      const BProtoObject* pTransportProtoObject = pTransportSquad->getProtoObject();
      if (pTransportProtoObject)
      {
         transportContains = pTransportProtoObject->getContains();
         numTransportContains = transportContains.getSize();
         maxTransportPop = pTransportProtoObject->getMaxContained();
      }
   }

   // Find average location of squads to transport
   BVector groupAvgPos = cOriginVector;
   static BEntityIDArray validSquadsToTransport;
   validSquadsToTransport.resize(0);
   uint numSquadsToTransport = squadsToTransport.getSize();
   for (uint i = 0; i < numSquadsToTransport; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadsToTransport[i]);
      if (pSquad)
      {
         groupAvgPos += pSquad->getPosition();
         validSquadsToTransport.uniqueAdd(squadsToTransport[i]);
      }
   }
   uint numValidSquadsToTransport = validSquadsToTransport.getSize();
   if (numValidSquadsToTransport == 0)
   {
      return;
   }
   groupAvgPos /= (float)numValidSquadsToTransport;
   gTerrainSimRep.getHeight(groupAvgPos, false);

   // Calculate transport drop off directions
   BVector dropOffDir = cZAxisVector;
   BVector dropOffRight = cXAxisVector;
   if (!dropOffLocation.almostEqual(groupAvgPos))
   {
      dropOffDir = dropOffLocation - groupAvgPos;
      dropOffDir.y = 0.0f;
      dropOffDir.safeNormalize();
      dropOffRight.assignCrossProduct(cYAxisVector, dropOffDir);
      dropOffRight.y = 0.0f;
      dropOffRight.safeNormalize();
   }
   
   // Add ALL transport squads to a temporary single army in a single platoon
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = playerID;
   BArmy* pArmy = gWorld->createArmy(objectParms);
   BASSERT(pArmy);
   if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
      pArmy->addSquads(transportSquads, false);
   else
      pArmy->addSquads(transportSquads);         
   BPlatoon* pPlatoon = gWorld->getPlatoon(pArmy->getChild(0));
   BASSERT(pPlatoon);                  
   int numNewPlatoons = (int)pArmy->getNumberChildren();
   for (int i = (numNewPlatoons - 1); i >= 1; i--)
   {
      BPlatoon* pNextPlatoon = gWorld->getPlatoon(pArmy->getChild(i));
      if (pNextPlatoon)
      {
         pPlatoon->mergePlatoon(pNextPlatoon);
      }
   }   

   // Create a formation to calculate positional data
   BFormation2* pTransportsFormation = gFormationManager.createFormation2();
   pTransportsFormation->setOwner(pPlatoon);
   pTransportsFormation->setType(BFormation2::eTypeStandard);
   pTransportsFormation->addChildren(transportSquads);
   pTransportsFormation->makeFormation();
   for (uint i = 0; i < totalNumTransports; i++)
   {
      BSquad* pTransportSquad = gWorld->getSquad(transportSquads[i]);
      if (pTransportSquad)
      {         
         // Set positional and orientation data for the transport
         transportData[i].setTransportSquad(transportSquads[i]);
         transportData[i].setForward(dropOffDir);
         transportData[i].setRight(dropOffRight);

         BVector thisTransportsPickUpLoc = pTransportsFormation->getTransformedFormationPositionFast(transportSquads[i], groupAvgPos, dropOffDir, dropOffRight);
         gTerrainSimRep.getHeight(thisTransportsPickUpLoc, false);
         transportData[i].setPos(thisTransportsPickUpLoc);
         transportData[i].setPickUpLoc(thisTransportsPickUpLoc);

         BVector dirScale = dropOffDir * (groupAvgPos.xzDistance(dropOffLocation));
         BVector thisTransportDropOffLoc = thisTransportsPickUpLoc + dirScale;
         gTerrainSimRep.getHeight(thisTransportDropOffLoc, false);
         transportData[i].setDropOffLoc(thisTransportDropOffLoc);
      }
   }   
   gFormationManager.releaseFormation2(pTransportsFormation);

   // Place transport squads in unique armies and platoons and set the platoons to not merge
   for (uint i = 0; i < totalNumTransports; i++)
   {
      BSquad* pTransportSquad = gWorld->getSquad(transportSquads[i]);
      if (pTransportSquad)
      {
         pTransportSquad->setFlagNoPlatoonMerge(true);
      }

      static BEntityIDArray armySquads;
      armySquads.resize(0);
      armySquads.add(transportSquads[i]);
      BArmy* pArmy = gWorld->createArmy(objectParms);
      BASSERT(pArmy);
      if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
         pArmy->addSquads(armySquads, false);
      else
         pArmy->addSquads(armySquads);         
   }

   // Assign squads to transports
   for (uint i = 0; i < totalNumTransports; i++)
   {
      BEntityIDArray thisTransportsSquads;
      float transportsContainedPop = 0.0f;
      BSquad* pTransportSquad = gWorld->getSquad(transportData[i].getTransportSquad());
      if (pTransportSquad)
      {
         BUnit* pTransportUnit = pTransportSquad->getLeaderUnit();
         if (pTransportUnit)
         {
            // Sort the squads left to transport based on this transports pick up location
            static BEntityIDArray sortedValidSquads;
            sortedValidSquads.resize(0);
            BSimHelper::sortEntitiesBasedOnDistance(validSquadsToTransport, transportData[i].getPickUpLoc(), sortedValidSquads);

            // Add squads to this transport until it is full
            uint numSortedValidSquads = sortedValidSquads.getSize();
            for (uint j = 0; j < numSortedValidSquads; j++)
            {
               BSquad* pSquad = gWorld->getSquad(sortedValidSquads[j]);
               if (pSquad)
               {
                  if (pTransportUnit->canContain(pSquad, transportsContainedPop))
                  {
                     thisTransportsSquads.uniqueAdd(sortedValidSquads[j]);
                     validSquadsToTransport.remove(sortedValidSquads[j]);

                     pSquad->setFlagIsTransporting(true);

                     const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
                     if (pProtoSquad)
                     {
                        BPopArray pops;
                        pProtoSquad->getPops(pops);
                        float pop = (pops.getSize() > 0) ? pops[0].mCount : 0.0f;
                        transportsContainedPop += pop;
                     }
                  }
               }               
            }
         }
      }

      transportData[i].setSquadsToTransport(thisTransportsSquads);
   }
}

//===========================================================================================================================
// Static function used to fly in transports, pick up troops and drop em off at a different location before flying off again
//===========================================================================================================================
bool BSquadActionTransport::transportSquads(BSimOrder* pSimOrder, const BEntityIDArray &squadList, BVector dropOffLocation, BPlayerID playerID, int transportObjectID)
{  
   if (squadList.getSize() == 0)
   {
      return (false);
   }

   // Remove all invalid squads
   static BEntityIDArray filteredSquadList;
   filteredSquadList.resize(0);
   filterTransportableSquads(squadList, cInvalidPlayerID, true, filteredSquadList);

   // Get total number of transports needed
   uint totalNumTransports = Math::Min(calculateNumTransports(filteredSquadList), gDatabase.getTransportMax());

   // Setup parameters for transports   
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = playerID;
   objectParms.mStartBuilt = true;
   objectParms.mProtoObjectID = transportObjectID;
   objectParms.mProtoSquadID = -1;
   objectParms.mPosition = cOriginVector;
   objectParms.mForward = cZAxisVector;
   objectParms.mRight = XMVector3Cross(cYAxisVector, objectParms.mForward);

   // Create the transports
   BEntityIDArray transportSquads;
   for (uint i = 0; i < totalNumTransports; i++)
   {
      // Create transport squad
//-- FIXING PREFIX BUG ID 2162
      const BSquad* pTransportSquad = gWorld->createSquad(objectParms);
//--
      if (pTransportSquad)
      {
         transportSquads.add(pTransportSquad->getID());
      }
   }

   gTerrainSimRep.getHeight(dropOffLocation, false);
   BTransportDataArray transportData;
   calculateTransportData(filteredSquadList, playerID, transportSquads, dropOffLocation, true, transportData);

   // Create ignore list
   BEntityIDArray ignoreSquads;
   for (uint i = 0; i < totalNumTransports; i++)
   {
      ignoreSquads.append(transportData[i].getSquadsToTransport());
   }

   for (uint i = 0; i < totalNumTransports; i++)
   {
      BSquad* pTransportSquad = gWorld->getSquad(transportSquads[i]);
      if (pTransportSquad)
      {
         // Test the validity of the drop-off location
         BEntityIDArray flyInSquads = transportData[i].getSquadsToTransport();
         BVector testDropOffLocation = transportData[i].getDropOffLoc();
         BVector evaluatedDropOffLocation = dropOffLocation;
         bool blocked = false;
         if (!BSimHelper::evaluateExitLocation(pTransportSquad, ignoreSquads, testDropOffLocation, evaluatedDropOffLocation) && !BSimHelper::clearExitLocation(pTransportSquad, ignoreSquads, evaluatedDropOffLocation, blocked))
         {
            BASSERTM(false, "Invalid drop off area for squad fly-in!");            
            for (uint j = 0; j < totalNumTransports; j++)
            {
               BSquad* pKillTransportSquad = gWorld->getSquad(transportData[j].getTransportSquad());
               if (pKillTransportSquad)
               {
                  pKillTransportSquad->kill(true);
               }
            }

            return (false);
         }

         // Recalculate fly-in position and orientation
         if (!evaluatedDropOffLocation.almostEqual(testDropOffLocation))
         {
            transportData[i].setDropOffLoc(evaluatedDropOffLocation);
            BVector forward = evaluatedDropOffLocation - transportData[i].getPickUpLoc();
            forward.y = 0.0f;
            forward.safeNormalize();
            BVector right;
            right.assignCrossProduct(cYAxisVector, forward);
            right.y = 0.0f;
            right.safeNormalize();
            transportData[i].setForward(forward);
            transportData[i].setRight(right);
         }

         // Compute initial position for transport to fly in from
         BVector flyInPos = transportData[i].getPos();
         BVector offset = transportData[i].getForward() * -gDatabase.getTransportIncomingOffset();
         flyInPos += offset;
         flyInPos.y += gDatabase.getTransportIncomingHeight();

         // Update transport squad, unit, and physics object position and orientation         
         pTransportSquad->setPosition(flyInPos);
         pTransportSquad->setForward(transportData[i].getForward());
         pTransportSquad->setRight(transportData[i].getRight());
         pTransportSquad->setLeashPosition(flyInPos);

         BUnit* pTransportUnit = pTransportSquad->getLeaderUnit();
         if (pTransportUnit)
         {
            #ifdef SYNC_Unit
            syncUnitData("BSquadActionTransport::transportSquads", flyInPos);
            #endif
            pTransportUnit->setPosition(flyInPos, true);
            BMatrix rot;
            pTransportSquad->getWorldMatrix(rot);
            rot.setTranslation(cOriginVector);
            pTransportUnit->setRotation(rot, true);
         }

         // Create the transport actions for each transport squad
         BSquadActionTransport* pAction = reinterpret_cast<BSquadActionTransport*>(gActionManager.createAction(BAction::cActionTypeSquadTransport));
         if (pAction)
         {
            // Clear orders
            uint numFlyInSquads = flyInSquads.getSize();
            for (uint j = 0; j < numFlyInSquads; j++)
            {
               BSquad* pFlyInSquad = gWorld->getSquad(flyInSquads[j]);
               if (pFlyInSquad)
               {
                  pFlyInSquad->removeAllOrders();
               }
            }

            BVector pickUpLoc = transportData[i].getPickUpLoc();
            BVector dropOffLoc = transportData[i].getDropOffLoc();            
            BVector flyOffLoc = cOriginVector;
            if (gDatabase.getTransportOutgoingOffset() > 0.0f)
            {
               BVector dir = dropOffLoc - pickUpLoc;
               dir.y = 0.0f;
               dir.safeNormalize();
               flyOffLoc = dropOffLoc + (dir * gDatabase.getTransportOutgoingOffset());
               flyOffLoc.y += gDatabase.getTransportOutgoingHeight();
               gTerrainSimRep.clampWorldWithBuffer(flyOffLoc, pTransportSquad->getObstructionRadius());
            }            

            pAction->setFlyInFrom(true);
            pAction->setLoadSquads(true);
            pAction->setTransportSquads(true);
            pAction->setUnloadSquads(true);
            pAction->setFlyOffTo(true);            
            
            pAction->setPickupLocation(pickUpLoc);                     
            pAction->setDropOffLocation(dropOffLoc);                        
            pAction->setFlyOffLocation(flyOffLoc);

            pAction->setSquadList(transportData[i].getSquadsToTransport());
            pAction->setCompletionSoundCue(cInvalidCueIndex);            
            pAction->setUseAnimations(false);

            pTransportSquad->addAction(pAction, pSimOrder);                          
            pTransportSquad->settle();
         }         
      }
   }

   return (true);
}
      
//============================================================================================
// Static function used to fly in a transport to drop off a new squad before flying off again
//============================================================================================
bool BSquadActionTransport::flyInSquad(BSimOrder* pSimOrder, BSquad* pSquad, const BVector* pFlyInLocation, BVector dropoffLocation, BVector dropoffForward, BVector dropoffRight, const BVector* pFlyOffLocation, BPlayerID playerID, int transportObjectID, bool moveToRallyPoint, BVector rallyPoint, bool attackMove, BCueIndex soundCue, bool useMaxHeight, bool useFacing, BVector facing)
{   
   bool result = false;
   if (pSquad)
   {      
      BEntityIDArray squadList;
      squadList.add(pSquad->getID());

      result = flyInSquads(pSimOrder, squadList, pFlyInLocation, dropoffLocation, dropoffForward, dropoffRight, pFlyOffLocation, playerID, transportObjectID, moveToRallyPoint, rallyPoint, attackMove, soundCue, useMaxHeight, useFacing, facing);
   }

   return (result);
}

//============================================================================================
// Static function used to fly in a transport to drop off new squads before flying off again
//============================================================================================
bool BSquadActionTransport::flyInSquads(BSimOrder* pSimOrder, const BEntityIDArray &squadList, const BVector* pFlyInLocation, BVector dropoffLocation, BVector dropoffForward, BVector dropoffRight, const BVector* pFlyOffLocation, BPlayerID playerID, int transportObjectID, bool moveToRallyPoint, BVector rallyPoint, bool attackMove, BCueIndex soundCue, bool useMaxHeight, bool useFacing, BVector facing)
{   
   uint numNewSquads = squadList.getSize();
   if (!(numNewSquads > 0))
   {
      return (false);
   }

   // Get total number of transports needed
   uint totalNumTransports = calculateNumTransports(squadList);
   BASSERTM(totalNumTransports <= gDatabase.getTransportMax(), "Consider creating less squads at one time!");
   totalNumTransports = Math::Min(totalNumTransports, gDatabase.getTransportMax());

   // Fix up vector data
   BVector flyInLoc = cOriginVector;
   if (pFlyInLocation)
   {
      flyInLoc = *pFlyInLocation;
      gTerrainSimRep.getHeight(flyInLoc, false);
   }

   BVector flyOffLoc = cOriginVector;
   if (pFlyOffLocation)
   {
      flyOffLoc = *pFlyOffLocation;
      gTerrainSimRep.getHeight(flyOffLoc, false);
   }
   gTerrainSimRep.getHeight(dropoffLocation, false);
   dropoffForward.y = 0.0f;
   dropoffForward.safeNormalize();
   dropoffRight.y = 0.0f;
   dropoffRight.safeNormalize();
   if (useFacing)
   {
      facing.y = 0.0f;
      facing.safeNormalize();
   }
   
   bool flyInNotDropOff = (pFlyInLocation && !flyInLoc.almostEqual(dropoffLocation));

   // Setup parameters for transports   
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = playerID;
   objectParms.mStartBuilt = true;
   objectParms.mProtoObjectID = transportObjectID;
   objectParms.mProtoSquadID = -1;
   objectParms.mPosition = cOriginVector;
   objectParms.mForward = cZAxisVector;
   objectParms.mRight = cXAxisVector;
   
   // Create the transports
   BEntityIDArray transportSquads;
   for (uint i = 0; i < totalNumTransports; i++)
   {
      // Create transport squad
//-- FIXING PREFIX BUG ID 2167
      const BSquad* pTransportSquad = gWorld->createSquad(objectParms);
//--
      if (pTransportSquad)
      {
         // Occlude transport squads until fly-in animation starts
         BUnit* pUnit = pTransportSquad->getLeaderUnit();
         if (pUnit)
         {
            pUnit->setFlagOccluded(true);
         }

         transportSquads.add(pTransportSquad->getID());
      }
   }   

   // Setup data if using a fly-in location
   if (flyInNotDropOff)
   {
      uint numSquadsToTransport = squadList.getSize();
      for (uint i = 0; i < numSquadsToTransport; i++)
      {
         BSquad* pSquad = gWorld->getSquad(squadList[i]);
         if (pSquad)
         {
            pSquad->setPosition(flyInLoc);
         }
      }
   }
    
   // Calculate transport specific data
   BTransportDataArray transportData;
   calculateTransportData(squadList, playerID, transportSquads, dropoffLocation, false, transportData);

   // Create ignore list
   BEntityIDArray ignoreSquads;
   for (uint i = 0; i < totalNumTransports; i++)
   {
      ignoreSquads.append(transportData[i].getSquadsToTransport());
   }
   
   for (uint i = 0; i < totalNumTransports; i++)
   {
      BSquad* pTransportSquad = gWorld->getSquad(transportSquads[i]);
      if (pTransportSquad)
      {
         BEntityIDArray flyInSquads = transportData[i].getSquadsToTransport();
         BVector testDropOffLocation = transportData[i].getDropOffLoc();               
         BVector evaluatedDropOffLocation = dropoffLocation;
         bool blocked = false;
         if (!BSimHelper::evaluateExitLocation(pTransportSquad, ignoreSquads, testDropOffLocation, evaluatedDropOffLocation) && !BSimHelper::clearExitLocation(pTransportSquad, ignoreSquads, evaluatedDropOffLocation, blocked))
         {
            BASSERTM(false, "Invalid drop off area for squad fly-in!");            
            for (uint j = 0; j < totalNumTransports; j++)
            {
               BSquad* pKillTransportSquad = gWorld->getSquad(transportData[j].getTransportSquad());
               if (pKillTransportSquad)
               {
                  pKillTransportSquad->kill(true);
               }
            }

            return (false);
         }

         // Recalculate fly-in position and orientation
         if (!evaluatedDropOffLocation.almostEqual(testDropOffLocation))
         {   
            transportData[i].setDropOffLoc(evaluatedDropOffLocation);     
            if (flyInNotDropOff)
            {
               BVector forward = evaluatedDropOffLocation - flyInLoc;
               forward.y = 0.0f;
               forward.safeNormalize();
               BVector right;
               right.assignCrossProduct(cYAxisVector, forward);
               right.y = 0.0f;
               right.safeNormalize();
               transportData[i].setForward(forward);
               transportData[i].setRight(right);
            }
            else
            {               
               transportData[i].setForward(dropoffForward);
               transportData[i].setRight(dropoffRight);
            }
         }

         // Update transport squad, unit, and physics object position and orientation
         BVector flyInPos = transportData[i].getPos();
         BVector transportForward = transportData[i].getForward();
         BVector transportRight = transportData[i].getRight();

         // If a Pelican only flying in one attached vehicle then offset drop-off so attached vehicle is on target
         if (pTransportSquad->getProtoObject() && pTransportSquad->getProtoObject()->isType(gDatabase.getOTIDUnsc()) && (numNewSquads == 1))
         {
            BSquad* pTestSquad = gWorld->getSquad(flyInSquads[0]);
            if (pTestSquad && pTestSquad->getProtoObject() && pTestSquad->getProtoObject()->isType(gDatabase.getOTIDGroundVehicle()))
            {
               BVector offsetDir = transportForward;
               offsetDir.safeNormalize();
               offsetDir *= 12.0f;
               flyInPos += offsetDir;
            }
         }

         pTransportSquad->setPosition(flyInPos);
         pTransportSquad->setForward(transportForward);
         pTransportSquad->setRight(transportRight);
         pTransportSquad->setLeashPosition(flyInPos);

         BUnit* pTransportUnit = pTransportSquad->getLeaderUnit();
         if (pTransportUnit)
         {
            #ifdef SYNC_Unit
               syncUnitData("BSquadActionTransport::flyInSquads", flyInPos);
            #endif
            pTransportUnit->setPosition(flyInPos, true);
            BMatrix rot;
            rot.makeIdentity();
            pTransportSquad->getWorldMatrix(rot);
            rot.setTranslation(cOriginVector);
            pTransportUnit->setRotation(rot, true);
            pTransportUnit->setFlagBlockContain(true);      // Block units from garrisoning in the transport as it is flying in
         }

         // Create the transport actions for each transport squad
         BSquadActionTransport* pAction = reinterpret_cast<BSquadActionTransport*>(gActionManager.createAction(BAction::cActionTypeSquadTransport));
         if (pAction)
         {
            // Clear orders
            uint numFlyInSquads = flyInSquads.getSize();
            for (uint j = 0; j < numFlyInSquads; j++)
            {
               BSquad* pFlyInSquad = gWorld->getSquad(flyInSquads[j]);
               if (pFlyInSquad)
               {
                  pFlyInSquad->removeAllOrders();
               }
            }
            
            BVector thisTransportsDropOffLoc = transportData[i].getDropOffLoc();
            BVector thisTransportsFlyOffLoc = flyOffLoc;
            if (pFlyOffLocation)
            {
               BVector flyOffDir = flyOffLoc - dropoffLocation;               
               flyOffDir.y = 0.0f;
               flyOffDir.safeNormalize();               
               thisTransportsFlyOffLoc = thisTransportsDropOffLoc + (flyOffDir * flyOffLoc.xzDistance(dropoffLocation));
            }

            pAction->setFlyInFrom(false);            
            pAction->setLoadSquads(false);
            pAction->setTransportSquads((flyInNotDropOff) ? true : false);
            pAction->setUnloadSquads(true);
            pAction->setFlyOffTo((pFlyOffLocation) ? true : false);            

            pAction->setDropOffLocation(thisTransportsDropOffLoc);            
            pAction->setFlyOffLocation(thisTransportsFlyOffLoc);

            pAction->setSquadList(flyInSquads);
            pAction->setMoveToRallyPoint(moveToRallyPoint);
            pAction->setUseMaxHeight(useMaxHeight);                        
            pAction->setRallyPoint(rallyPoint);
            pAction->setAttackMove(attackMove);
            pAction->setCompletionSoundCue(soundCue);
            pAction->setUseFacing(useFacing);
            pAction->setFacing(facing);                                    
            pAction->setTrainingAlert(true);
            pAction->setUseAnimations(true);
            
            bool result = pTransportSquad->addAction(pAction, pSimOrder);                
            BASSERT(result);
            pTransportSquad->settle();
            if (result)
               pAction->preLoadSquads();            
         }         
      }
   }

   return (true);
}

//==============================================================================
// Destroy transports
//==============================================================================
void BSquadActionTransport::selfDestruct()
{
//-- FIXING PREFIX BUG ID 2175
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);
   
   uint numChildren = pSquad->getNumberChildren();
   for (uint i  = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
         pUnit->kill(true);
   }
}

//==============================================================================
// Garrison the squads into the transports
//==============================================================================
bool BSquadActionTransport::loadSquads()
{
//-- FIXING PREFIX BUG ID 2176
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   removeOpp();

   // Create a new army and add the transporting squads
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = pSquad->getPlayerID();
   BArmy* pArmy = gWorld->createArmy(objectParms);
   BASSERT(pArmy);
   if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
      pArmy->addSquads(mSquadList, false);   
   else
      pArmy->addSquads(mSquadList);

   BSimTarget target(pSquad->getLeader());
   setupChildOrder(mpOrder);
   if (mpChildOrder)
   {
      mpChildOrder->setTarget(target);
   }

   // filter again before loading, and unset the transporting flag on any now-invalid squads
   static BEntityIDArray tempSquads;
   tempSquads.resize(0);
   static BEntityIDArray leavingSquads;
   leavingSquads.resize(0);
   filterTransportableSquads(mSquadList, cInvalidPlayerID, false, tempSquads);
   BSimHelper::diffEntityIDArrays(mSquadList, tempSquads, &leavingSquads, NULL, NULL);
   for(long i = 0; i < leavingSquads.getNumber(); i++)
   {
      BSquad* pSquad = gWorld->getSquad(leavingSquads[i]);
      if (pSquad)
         pSquad->setFlagIsTransporting(false);
   }
   mSquadList.assignNoDealloc(tempSquads);

   uint numSquadsToTransport = mSquadList.getSize();
   for (uint i = 0; i < numSquadsToTransport; i++)
   {
      BSquad* pSquadToTransport = gWorld->getSquad(mSquadList[i]);
      if (!pSquadToTransport)
      {
         continue;
      }
      
      // Clear the transporting squad's current orders
      pSquadToTransport->removeOrders(true, true, true, false);

      BActionID childActionID = pSquadToTransport->doGarrison(mpChildOrder, this, &target, false, false);
      if (childActionID != cInvalidActionID)
      {
         BTransportChildActionInfo info;
         info.mActionID = childActionID;
         info.mEntityID = mSquadList[i];
         mChildActionInfos.add(info);
      }                  
   }   

   if (mChildActionInfos.getNumber() == 0)
   {
      return (false);
   }

   return (true);
}

//==============================================================================
// Pre-loads the squads in the transports directly.  No actions are used.
//==============================================================================
void BSquadActionTransport::preLoadSquads()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
      return;

   BUnit* pTransportUnit = pSquad->getLeaderUnit();
   BASSERT(pTransportUnit);
   if (!pTransportUnit)
      return;

   // Iterate through squads to garrison
   uint numSquadsToTransport = (uint)mSquadList.getNumber();
   for (uint i = 0; i < numSquadsToTransport; i++)
   {
//-- FIXING PREFIX BUG ID 2178
      const BSquad* pSquadToGarrison = gWorld->getSquad(mSquadList[i]);
//--
      if (pSquadToGarrison)
      {
         // Iterate through the squad's units
         uint numChildren = pSquadToGarrison->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            BEntityID unitID = pSquadToGarrison->getChild(j);
            BUnit* pUnitToGarrison = gWorld->getUnit(unitID);
            if (pUnitToGarrison)
            {
               attachOrContainUnit(pUnitToGarrison, true);
            }
         }
      }
   }

   mFlagLoadSquads = false;
}

//==============================================================================
// Move the transports from their spawn location to the pick up location
//==============================================================================
bool BSquadActionTransport::flyInFrom()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   removeOpp();

   BWorkCommand tempCommand;
   tempCommand.setWaypoints(&mPickupLocation, 1);

   // Set the command to be from a trigger to the army
   tempCommand.setRecipientType(BCommand::cArmy);
   tempCommand.setSenderType(BCommand::cGame);

   // Set the attack move
   tempCommand.setFlag(BWorkCommand::cFlagAttackMove, false);

   // Set the queue order
   tempCommand.setFlag(BCommand::cFlagAlternate, false);
   BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   BASSERT(pPlatoon);
   if (!pPlatoon)
      return (false);
   BArmy* pArmy = pPlatoon->getParentArmy();
   BASSERT(pArmy);
   if (!pArmy)
      return (false);
   pArmy->queueOrder(&tempCommand);

   return (true);
}

//===================================================================================================
// Move the transports with the garrisoned squads from the pick up location to the drop off location
//===================================================================================================
bool BSquadActionTransport::moveTransports()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   
   removeOpp();

   BWorkCommand tempCommand;
   tempCommand.setWaypoints(&mDropOffLocation, 1);

   // Set the command to be from a trigger to the army
   tempCommand.setRecipientType(BCommand::cArmy);
   tempCommand.setSenderType(BCommand::cGame);

   // Set the attack move
   tempCommand.setFlag(BWorkCommand::cFlagAttackMove, false);

   // Set the queue order
   tempCommand.setFlag(BCommand::cFlagAlternate, false);
   BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   BASSERT(pPlatoon);
   if (!pPlatoon)
      return (false);
   BArmy* pArmy = pPlatoon->getParentArmy();
   BASSERT(pArmy);
   if (!pArmy)
      return (false);
   pArmy->queueOrder(&tempCommand);

   return (true);
}

//==============================================================================
// Move the transports from the drop off location to the fly off location
//==============================================================================
bool BSquadActionTransport::flyOffTo()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   removeOpp();

   playCompletionSoundCue();

   BWorkCommand tempCommand;
   tempCommand.setWaypoints(&mFlyOffLocation, 1);

   // Set the command to be from a trigger to the army
   tempCommand.setRecipientType(BCommand::cArmy);
   tempCommand.setSenderType(BCommand::cGame);

   // Set the attack move
   tempCommand.setFlag(BWorkCommand::cFlagAttackMove, false);

   // Set the queue order
   tempCommand.setFlag(BCommand::cFlagAlternate, false);
   BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   BASSERT(pPlatoon);
   if (!pPlatoon)
      return (false);
   BArmy* pArmy = pPlatoon->getParentArmy();
   BASSERT(pArmy);
   if (!pArmy)
      return (false);
   pArmy->queueOrder(&tempCommand);

   return (true);
}

//==============================================================================
// Ungarrison the squads from the transports
//==============================================================================
bool BSquadActionTransport::unloadSquads()
{
//-- FIXING PREFIX BUG ID 2179
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   removeOpp();

   // Group the squads to be unloaded per player and put them in a new army
   BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(mSquadList);
   uint numPlayerEntityIDs = playerEntityIDs.getSize();
   for (uint i = 0; i < numPlayerEntityIDs; i++)
   {
      uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
      if (numEntities > 0)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (pArmy)
         {
            // Platoon the squads
            pArmy->addSquads(playerEntityIDs[i].mEntityIDs, false);
         }               
      }
   }

   BSimTarget target(pSquad->getLeader());
   setupChildOrder(mpOrder);
   if (mpChildOrder)
   {
      mpChildOrder->setTarget(target);
      mpChildOrder->setAttackMove(mFlagAttackMove);
   }
   int8 exitDirectionInfantry = -1;
   int8 exitDirectionVehicle = BProtoObjectStatic::cNumExitFrom;
   int8 exitDirection = BProtoObjectStatic::cExitFromFront;
   uint numSquadsToTransport = mSquadList.getSize();
   for (uint i = 0; i < numSquadsToTransport; i++)
   {
      BSquad* pSquadToTransport = gWorld->getSquad(mSquadList[i]);
      if (!pSquadToTransport)
      {
         continue;
      }

      // Setup the exit direction for the ungarrisoning infantry      
      const BProtoObject* pProtoObject = pSquadToTransport->getProtoObject();
      if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDInfantry()))
      {
         exitDirectionInfantry++;
         exitDirection = exitDirectionInfantry;
      }
      else if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDGroundVehicle()))
      {
         exitDirectionVehicle--;
         exitDirection = exitDirectionVehicle;
      }
      BActionID childActionID = pSquadToTransport->doUngarrison(mpChildOrder, this, &target, &target, mRallyPoint, exitDirection, mFlagUseFacing ? mFacing : cInvalidVector);
      if (childActionID != cInvalidActionID)
      {
         BTransportChildActionInfo info;
         info.mActionID = childActionID;
         info.mEntityID = mSquadList[i];
         mChildActionInfos.add(info);
      }                  

      if (mFlagTrainingAlert)
      {
         // Alert the player
         const BPlayer* pPlayer = pSquadToTransport->getPlayer();
         BASSERT(pPlayer);
         pPlayer->getAlertManager()->createTrainingCompleteAlert(pSquadToTransport->getPosition(), pSquadToTransport->getID());
      }

      //pSquadToTransport->setFlagIsTransporting(false);
   }   

   if (mChildActionInfos.getNumber() == 0)
   {
      return (false);
   }

   return (true);
}

//==============================================================================
// Send the transports out of the world
//==============================================================================
bool BSquadActionTransport::flyOffTransports()
{
//-- FIXING PREFIX BUG ID 2180
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);
   BUnit* pUnit = pSquad->getLeaderUnit();
   BASSERT(pUnit);
   if (pUnit)
   {
      // Halwes - 7/29/2008 - Deactivate physics object so no pop at end of animation.  This is fine since I am going to destroy this unit after the animation is done.
      BPhysicsObject* pPhysObject = pUnit->getPhysicsObject();
      if (pPhysObject)
      {
         pPhysObject->forceDeactivate();
      }
   }

   BUnitOpp opp;
   opp.init();
   opp.setTarget(mTarget);
   opp.setType(BUnitOpp::cTypeAnimation);
   opp.setSource(pSquad->getID());
   opp.setUserData(cAnimTypeOutgoing);   
   opp.generateID();
   if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
      opp.setTrigger(true);
   else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
      opp.setPriority(BUnitOpp::cPriorityCommand);
   else
      opp.setPriority(BUnitOpp::cPrioritySquad);

   bool result = addOpp(opp);

   return (result);
}

//==============================================================================
// Fly in the transports in from out of the world
//==============================================================================
bool BSquadActionTransport::flyInTransports()
{
//-- FIXING PREFIX BUG ID 2181
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);
   BUnit* pUnit = pSquad->getLeaderUnit();
   BASSERT(pUnit);

   BUnitOpp opp;
   opp.init();
   opp.setTarget(mTarget);
   opp.setType(BUnitOpp::cTypeAnimation);
   opp.setSource(pSquad->getID());
   opp.setUserData(cAnimTypeIncoming);
   opp.generateID();
   if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
      opp.setTrigger(true);
   else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
      opp.setPriority(BUnitOpp::cPriorityCommand);
   else
      opp.setPriority(BUnitOpp::cPrioritySquad);

   bool result = addOpp(opp);

   return (result);
}

//==============================================================================
//==============================================================================
void BSquadActionTransport::playCompletionSoundCue()
{
   if (mCompletionSoundCue != cInvalidCueIndex)
   {
      gSoundManager.playCue(mCompletionSoundCue);
      gUI.playRumbleEvent(BRumbleEvent::cTypeTrainComplete);
   }
}

//==============================================================================
// Position squads to get ready for transporting
//==============================================================================
bool BSquadActionTransport::positionSquads()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // Create a new army and add the transporting squads
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = pSquad->getPlayerID();
   BArmy* pArmy = gWorld->createArmy(objectParms);
   BASSERT(pArmy);
   pArmy->addSquads(mSquadList, false);   

   BWorkCommand tempCommand;
   tempCommand.setWaypoints(&mPickupLocation, 1);
   tempCommand.setRecipientType(BCommand::cArmy);
   tempCommand.setFlag(BWorkCommand::cFlagAlternate, false);
   tempCommand.setFlag(BWorkCommand::cFlagAttackMove, false);
   pArmy->queueOrder(&tempCommand);

   return (true);
}

//==============================================================================
// Validate the transport units distance to the test location
//==============================================================================
bool BSquadActionTransport::validateUnitDist(BVector testPos, float threshold)
{   
   bool result = false;
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (pSquad)
   {
      BUnit* pUnit = pSquad->getLeaderUnit();
      BASSERT(pUnit);
      if (pUnit)
      {
         //XXXHalwes - 7/24/2208 - Debug
         //gpDebugPrimitives->addDebugSphere(testPos, threshold, cDWORDGreen);   
         BVector unitPos = pUnit->getPosition();
         gTerrainSimRep.getHeight(unitPos, false);         
         //XXXHalwes - 7/24/2208 - Debug
         //gpDebugPrimitives->addDebugSphere(unitPos, pUnit->getObstructionRadius(), cDWORDRed);            
         float dist = pUnit->calculateXZDistance(testPos);
         if (dist <= threshold)
         {
            result = true;
         }
      }
   }

   return (result);
}

//==============================================================================
// Validate unit motion
//==============================================================================
bool BSquadActionTransport::validateUnitMotion(float elapsed, float maxStillTime, float minMovementDist)
{
   bool result = false;
   mMovementTimer += elapsed;
   if (mMovementTimer >= maxStillTime)
   {
      BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
      BASSERT(pSquad);
      if (pSquad)
      {
         BUnit* pUnit = pSquad->getLeaderUnit();
         BASSERT(pUnit);
         if (pUnit)
         {
            float distSq = mMovementPos.distanceSqr(pUnit->getPosition());
            float minDistSq = minMovementDist * minMovementDist;
            if (distSq < minDistSq)
            {
               result = true;
            }
            mMovementPos = pUnit->getPosition();
         }
      }
   }

   return (result);
}

//==============================================================================
// Hover offset for persistent physics action
//==============================================================================
bool BSquadActionTransport::getHoverOffset(float& offset) const
{
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (mFlagUseAnimations)
   {
      return (false);
   }

   switch (mState)
   {
      case cStateFlyIn:
      case cStateLoading:
         offset = gDatabase.getTransportPickupHeight();
         return (true);

      case cStateMoving:
         {
            const BUnit* pUnit = pSquad->getLeaderUnit();
            if (pUnit)
            {
               static float cApproachDist = 60.0f;
               float dist = pUnit->getPosition().xzDistance(mDropOffLocation);
               if (dist <= cApproachDist)
               {
                  offset = gDatabase.getTransportDropoffHeight();
                  return (true);
               }
            }
            break;
         }

      case cStateUnloading:
         offset = gDatabase.getTransportDropoffHeight();
         return (true);

      case cStateFlyOff:
         offset = gDatabase.getTransportOutgoingHeight();
         return (true);
   }

   return (false);
}

//==============================================================================
// Force load the squads
//==============================================================================
bool BSquadActionTransport::forceLoadSquads()
{
   BSquad* pTransportSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pTransportSquad);
   BUnit* pTransportUnit = pTransportSquad->getLeaderUnit();
   BASSERT(pTransportUnit);
   if (!pTransportUnit)
   {
      return (false);
   }

   mFlagIgnoreNotify = true;

   // Iterate through squads to load
   uint numSquadsToTransport = mSquadList.getSize();
   for (uint i = 0; i < numSquadsToTransport; i++)
   {
      // For any squad that is not already loaded
      BSquad* pSquadToTransport = gWorld->getSquad(mSquadList[i]);
      if (pSquadToTransport && !pSquadToTransport->getFlagGarrisoned() && !pSquadToTransport->getFlagAttached())
      {         
         // Clear the transporting squad's current orders         
         BSquadActionGarrison* pGarrisonAction = (BSquadActionGarrison*)pSquadToTransport->getActionByType(BAction::cActionTypeSquadGarrison);
         if (pGarrisonAction)
         {
            actionsComplete(pGarrisonAction->getID());            
         }
         pSquadToTransport->removeOrder(mpChildOrder, true, false);
         pSquadToTransport->removeOrders(true, true, true, false);
         pSquadToTransport->removeAllActionsOfType(BAction::cActionTypeSquadGarrison);

         // Iterate through the squads units
         uint numChildren = pSquadToTransport->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            // For any unit that is not already loaded
            BEntityID pUnitToLoadID = pSquadToTransport->getChild(j);
            BUnit* pUnitToLoad = gWorld->getUnit(pUnitToLoadID);
            if (pUnitToLoad && !pUnitToLoad->getFlagGarrisoned() && !pUnitToLoad->getFlagAttached())
            {
               attachOrContainUnit(pUnitToLoad);
            }
         }
      }
   }   

   BASSERT(mChildActionInfos.getSize() == 0);

   mFlagIgnoreNotify = false;

   return (true);
}

//==============================================================================
// Clean up after a load failure
//==============================================================================
bool BSquadActionTransport::cleanUpLoadFailure()
{
   BSquad* pTransportSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pTransportSquad);
   BUnit* pTransportUnit = pTransportSquad->getLeaderUnit();
   BASSERT(pTransportUnit);
   if (!pTransportUnit)
   {
      return (false);
   }

   mFlagIgnoreNotify = true;

   // Iterate through squads that should be loaded
   uint numSquadsToTransport = mSquadList.getSize();
   for (uint i = 0; i < numSquadsToTransport; i++)
   {
      // For any squad that is not already loaded
      BSquad* pSquadToTransport = gWorld->getSquad(mSquadList[i]);
      if (pSquadToTransport && !pSquadToTransport->getFlagGarrisoned() && !pSquadToTransport->getFlagAttached())
      {
         // Clear the transporting squad's current orders
         pSquadToTransport->removeOrders(true, true, true, false);

         pSquadToTransport->setFlagIsTransporting(false);
         
         // Iterate through the squads units
         uint numChildren = pSquadToTransport->getNumberChildren();
         for (uint j= 0 ; j < numChildren; j++)
         {
            // For any unit that is already loaded
            BEntityID pUnitToLoadID = pSquadToTransport->getChild(j);
            BUnit* pUnitToLoad = gWorld->getUnit(pUnitToLoadID);
            if (pUnitToLoad && pUnitToLoad->getFlagGarrisoned())
            {
               // Unload unit for partially loaded squad
               if (pUnitToLoad->getFlagGarrisoned())
               {
                  pTransportUnit->unloadUnit(pUnitToLoadID, false);
               }
            }
         }
      }
   }   

   mFlagIgnoreNotify = false;

   return (true);   
}

//================================================================================
// Detect if any of the squads we want to transport have been successfully loaded
//================================================================================
bool BSquadActionTransport::anySquadsLoaded()
{
   bool result = false;

   // Iterate through squads that should be loaded
   uint numSquadsToTransport = mSquadList.getSize();
   for (uint i = 0; i < numSquadsToTransport; i++)
   {
      // For any squad that is not already loaded
      BSquad* pSquadToTransport = gWorld->getSquad(mSquadList[i]);
      if (pSquadToTransport && (pSquadToTransport->getFlagGarrisoned() || pSquadToTransport->getFlagAttached()))
      {
         result = true;
         break;
      }
   }   

   return (result);
}

//==============================================================================
// Contain or attach the unit to be transported
//==============================================================================
void BSquadActionTransport::attachOrContainUnit(BUnit* pUnit, bool occlude /*= false*/)
{
   BSquad* pTransportSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pTransportSquad);
   BUnit* pTransportUnit = pTransportSquad->getLeaderUnit();
   BASSERT(pTransportUnit);
   if (!pTransportUnit)
   {
      return;
   }

   BASSERT(pUnit);
   if (!pUnit)
   {
      return;
   }

   BEntityID unitID = pUnit->getID();
   if (!pTransportUnit->getFlagHasAttached() && pTransportUnit->isType(gDatabase.getOTIDUnsc()) && pUnit->isType(gDatabase.getOTIDGroundVehicle()))
   {
//-- FIXING PREFIX BUG ID 2177
      const BVisual* pTargetVisual = pTransportUnit->getVisual();
//--
      if (pTargetVisual)
      {
         // Orient unit to Pelican because this is not handled by move action
         pUnit->setForward(pTransportUnit->getForward());
         pUnit->setRight(pTransportUnit->getRight());
         pUnit->calcUp();
         long boneHandle = pTargetVisual->getBoneHandle("bone_attachpoint");                     
         if (boneHandle != -1)
         {
            BVector bonePos;
            BMatrix worldMatrix;
            pTransportUnit->getWorldMatrix(worldMatrix);
            pTargetVisual->getBone(boneHandle, &bonePos, NULL, NULL, &worldMatrix, false);                        
#ifdef SYNC_Unit
            syncUnitData("BSquadActionTransport::preLoadSquads", bonePos);
#endif
            pUnit->setPosition(bonePos, true);
            pTransportUnit->attachObject(unitID, boneHandle);       

            // Occlude attached unit so there is no pop before fly-in animation begins
            if (occlude)
            {
               pUnit->setFlagOccluded(true);
            }            
         }
         else
         {
            pUnit->setFlagUseMaxHeight(mFlagUseMaxHeight);
            pTransportUnit->containUnit(unitID);
         }
      }                  
   }
   else
   {
      // Stop unit and garrison it
      pUnit->setFlagUseMaxHeight(mFlagUseMaxHeight);
      pTransportUnit->containUnit(unitID);
   }
}

//==============================================================================
// Remove any cached child actions from the squads and the transports
//==============================================================================
void BSquadActionTransport::removeChildActions()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   uint numChildActions = (uint)mChildActionInfos.getNumber();
   for (uint i = 0; i < numChildActions; i++)
   {
      // Remove the child action.
      pSquad->removeActionByID(mChildActionInfos[i].mActionID);
      
      uint numSquadsTransported = (uint)mSquadList.getNumber();
      for (uint j = 0; j < numSquadsTransported; j++)
      {
         BSquad* pSquadTransprted = gWorld->getSquad(mSquadList[i]);
         if (pSquadTransprted)
         {
            // Remove the child action.
            pSquadTransprted->removeActionByID(mChildActionInfos[i].mActionID);
         }
      }
   }
}

//===================================================================================================================
// Check action against cached child actions and remove it when found.  When all actions are gone they are complete.
//===================================================================================================================
bool BSquadActionTransport::actionsComplete(BActionID id)
{
   for(long i = 0; i < mChildActionInfos.getNumber(); i++)
   {
      if(mChildActionInfos[i].mActionID == id)
      {
         if(mChildActionInfos.removeIndex(i))
            return (mChildActionInfos.getNumber() == 0);
      }
   }

   return (false);
}

//==============================================================================
// Add the opportunity to the children units
//==============================================================================
bool BSquadActionTransport::addOpp(BUnitOpp opp)
{
   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 2171
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   if (pSquad->addOppToChildren(opp, mUnitOppIDCount))
   {      
      mUnitOppID = opp.getID();      
      return (true);
   }

   return (false);
}

//==============================================================================
// Remove the opportunity to the children units
//==============================================================================
void BSquadActionTransport::removeOpp()
{
   if (mUnitOppID == BUnitOpp::cInvalidID)
   {
      return;
   }

   //Remove the opportunity that we've given the units.  That's all we do here.
//-- FIXING PREFIX BUG ID 2172
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
}

//==============================================================================
// Setup the child order based on the parent order
//==============================================================================
void BSquadActionTransport::setupChildOrder(BSimOrder* pOrder)
{
   if (!mpChildOrder)
   {
      mpChildOrder = gSimOrderManager.createOrder();
      BASSERT(mpChildOrder);
      mpChildOrder->incrementRefCount();
      mpChildOrder->setOwnerID(pOrder->getOwnerID());
      mpChildOrder->setPriority(pOrder->getPriority());
   }
   mpChildOrder->setAttackMove(false);
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport::isInterruptible() const
{
   if (mFlagControllableTransport && mState == cStateMoving)
      return true;
   return false;
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITECLASS(pStream, saveType, mTarget);      
   GFWRITEARRAY(pStream, BEntityID, mSquadList, uint8, 100);
   GFWRITEARRAY(pStream, uint, mTransportIndex, uint8, 20);
   GFWRITEARRAY(pStream, BTransportChildActionInfo, mChildActionInfos, uint8, 20);
   GFWRITEVECTOR(pStream, mPickupLocation);
   GFWRITEVECTOR(pStream, mDropOffLocation);
   GFWRITEVECTOR(pStream, mFlyOffLocation);
   GFWRITEVECTOR(pStream, mRallyPoint);        
   GFWRITEVECTOR(pStream, mFacing);
   GFWRITEVECTOR(pStream, mMovementPos);
   GFWRITEFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFWRITEVAR(pStream, float, mMovementTimer);
   GFWRITEVAR(pStream, DWORD, mBlockedTimeStamp);      
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);  
   GFWRITEVAR(pStream, BCueIndex, mCompletionSoundCue);
   GFWRITEVAR(pStream, BActionState, mFutureState);      
   GFWRITEVAR(pStream, uint8, mUnitOppIDCount);
   GFWRITEBITBOOL(pStream, mFlagLoadSquads);
   GFWRITEBITBOOL(pStream, mFlagFlyInFrom);
   GFWRITEBITBOOL(pStream, mFlagTransportSquads);
   GFWRITEBITBOOL(pStream, mFlagUnloadSquads);
   GFWRITEBITBOOL(pStream, mFlagMoveToRallyPoint);
   GFWRITEBITBOOL(pStream, mFlagFlyOffTo);
   GFWRITEBITBOOL(pStream, mFlagUseMaxHeight);
   GFWRITEBITBOOL(pStream, mFlagActionFailed);
   GFWRITEBITBOOL(pStream, mFlagAnyFailed);      
   GFWRITEBITBOOL(pStream, mFlagAttackMove);
   GFWRITEBITBOOL(pStream, mFlagUseFacing);
   GFWRITEBITBOOL(pStream, mFlagControllableTransport);
   GFWRITEBITBOOL(pStream, mFlagTrainingAlert);
   GFWRITEBITBOOL(pStream, mFlagNewDropOff);
   GFWRITEBITBOOL(pStream, mFlagBlocked);
   GFWRITEBITBOOL(pStream, mFlagNotifyReceived);
   GFWRITEBITBOOL(pStream, mFlagUseAnimations);
   GFWRITEBITBOOL(pStream, mFlagIgnoreNotify);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADCLASS(pStream, saveType, mTarget);      
   GFREADARRAY(pStream, BEntityID, mSquadList, uint8, 100);
   GFREADARRAY(pStream, uint, mTransportIndex, uint8, 20);
   if(BAction::mGameFileVersion < 49)
   {
      BActionIDArray deprecatedArray;
      GFREADARRAY(pStream, BActionID, deprecatedArray, uint8, 20);
      mChildActionInfos.clear();
      for(long i = 0; i < deprecatedArray.getNumber(); i++)
      {
         BTransportChildActionInfo info;
         info.mActionID = deprecatedArray[i];
         info.mEntityID = cInvalidObjectID;
         mChildActionInfos.add(info);
      }
   }
   else
   {
      GFREADARRAY(pStream, BTransportChildActionInfo, mChildActionInfos, uint8, 20);
   }
   GFREADVECTOR(pStream, mPickupLocation);
   GFREADVECTOR(pStream, mDropOffLocation);
   GFREADVECTOR(pStream, mFlyOffLocation);
   GFREADVECTOR(pStream, mRallyPoint);        
   GFREADVECTOR(pStream, mFacing);
   GFREADVECTOR(pStream, mMovementPos);
   GFREADFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFREADVAR(pStream, float, mMovementTimer);
   GFREADVAR(pStream, DWORD, mBlockedTimeStamp);      
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);  
   GFREADVAR(pStream, BCueIndex, mCompletionSoundCue);
   GFREADVAR(pStream, BActionState, mFutureState);      
   GFREADVAR(pStream, uint8, mUnitOppIDCount);
   GFREADBITBOOL(pStream, mFlagLoadSquads);
   GFREADBITBOOL(pStream, mFlagFlyInFrom);
   GFREADBITBOOL(pStream, mFlagTransportSquads);
   GFREADBITBOOL(pStream, mFlagUnloadSquads);
   GFREADBITBOOL(pStream, mFlagMoveToRallyPoint);
   GFREADBITBOOL(pStream, mFlagFlyOffTo);
   GFREADBITBOOL(pStream, mFlagUseMaxHeight);
   GFREADBITBOOL(pStream, mFlagActionFailed);
   GFREADBITBOOL(pStream, mFlagAnyFailed);      
   GFREADBITBOOL(pStream, mFlagAttackMove);
   GFREADBITBOOL(pStream, mFlagUseFacing);
   GFREADBITBOOL(pStream, mFlagControllableTransport);
   GFREADBITBOOL(pStream, mFlagTrainingAlert);
   GFREADBITBOOL(pStream, mFlagNewDropOff);
   GFREADBITBOOL(pStream, mFlagBlocked);
   GFREADBITBOOL(pStream, mFlagNotifyReceived);
   GFREADBITBOOL(pStream, mFlagUseAnimations);
   GFREADBITBOOL(pStream, mFlagIgnoreNotify);

   return true;
}
