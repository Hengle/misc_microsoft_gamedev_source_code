//==============================================================================
// platoon.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#include "Common.h"
#include "Platoon.h"
#include "ActionManager.h"
#include "ConfigsGame.h"
#include "Formation2.h"
#include "FormationManager.h"
#include "SimOrderManager.h"
#include "SquadPlotter.h"
#include "EntityGrouper.h"

#include "ability.h"
#include "army.h"
#include "command.h"
#include "commands.h"
#include "objectmanager.h"
#include "protoobject.h"
#include "protosquad.h"
#include "squad.h"
#include "SquadActionMove.h"
#include "world.h"
#include "uimanager.h"
#include "pather.h"
#include "pathingLimiter.h"
#include "tactic.h"
#include "worldsoundmanager.h"
#include "usermanager.h"
#include "user.h"
#include "findpathhelper.h"
#include "platoonactionmove.h"


#ifndef BUILD_FINAL
   //#define DEBUGMANAGEMENT
   //#define DEBUGGPH
   //#define PATHING_HINTS_USE_PLOTTER
   //#define DEBUG_MOVE4
   
   BEntityID sDebugPlatoonTempID;
#endif


#define cDefaultMovementProjectionTime    1.0f


#ifndef _MOVE4
#define _MOVE4
#endif


#ifdef DEBUG_MOVE4
   #define debugMove4 sDebugPlatoonTempID=mID, dbgPlatoonInternalTempID
#else
   #define debugMove4 __noop
#endif

#ifdef _MOVE4
const float cActionCompleteEpsilon = 0.1f;
const unsigned long cDefaultPauseTimeout = 3000L;
//#define PLATOON_PAUSING
//#define PLATOON_SPLITTING

#endif

GFIMPLEMENTVERSION(BPlatoon,3);
enum
{
   cSaveMarkerPlatoon1=10000,
};

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void dbgPlatoonInternal(BEntityID platoonID, const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);
   
   // Output.
   gConsole.output(cChannelSim, "PLATOON %d (%d): %s", platoonID.asLong(), platoonID.getIndex(), buf);
   //syncPlatoonData("debugMove4 --", buf);
}


//==============================================================================
//==============================================================================
void dbgPlatoonInternalTempID(const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);

   // Call with preset ID.
   dbgPlatoonInternal(sDebugPlatoonTempID, buf);
}

#endif



// Fudge factor for when we're "close enough" to a waypoint to advance to the next
// one.
const float cInWPRangeConst = 0.02f;
// DLM 5/31/08 - Loosening this up even more, seen's how a single tank
// can be more than 10 meters in this game.
// jce [9/25/2008] -- uh yeah, new, um, scientifically-chosen, even larger amount
//const float cInHLWPLooseRangeConst = 12.0f;
const float cInHLWPLooseRangeConst = 20.0f;

//==============================================================================
//==============================================================================
void BPlatoon::onRelease()
{
   mChildren.clear();
   mPendingChildrenToRemove.clear();
   mSquadRanges.clear();

   if (!gWorldReset)
      removeAllOrders();
   else
      mOrders.clear();

   gFormationManager.releaseFormation2(mpFormation);

   BEntity::onRelease();
}

//==============================================================================
//==============================================================================
void BPlatoon::init()
{
   BEntity::init();

   mChildren.clear();
   mPendingChildrenToRemove.clear();
   // DLM 11/3/08 SquadRanges
   mSquadRanges.clear();

   mSlowestSquadVelocity=0.0f;
   mSlowestSquadVelocityValid=false;
   mSlowestSquadID=cInvalidObjectID;
    
   mpFormation=gFormationManager.createFormation2();
   mpFormation->setOwner(this);
   if (gConfig.isDefined(cConfigMoreNewMovement3))
      mpFormation->setType(BFormation2::eTypeMob);
   else
      mpFormation->setType(BFormation2::eTypePlatoon);

   mPathingHints.clear();
   mPathingHintForwards.clear();
   mPathingHintsSimOrderID = 0;
   mPathingHintsTime = 0;

   //Override the -1 obstruction radius so that distance checks work.
   setObstructionRadiusX(0.0f);
   setObstructionRadiusY(0.0f);
   setObstructionRadiusZ(0.0f);
   
   mOrders.clear();
   #ifdef _MOVE4
   mCurrentUserWP_4 = 0;
   mCurrentUserPath_4.reset();
   mCurrentHLWP_4 = 0;
   mCurrentHLPath_4.reset();
   mCurrentLLWP_4 = 0;
   mCurrentLLPath_4.reset();
   mValidPlots_4 = false;
   mSquadPlots_4.clear();

   mRelocationActionID = cInvalidActionID;
   mRelocationInProgress = false;
   mSquadToRelocate = 0;
   mSquadPlotWaypoint = -1;

   #endif

}

//==============================================================================
//==============================================================================
float BPlatoon::getMaxVelocity() const
{
   return(getDesiredVelocity());
}

//==============================================================================
//==============================================================================
float BPlatoon::getDesiredVelocity() const
{
   if (mSlowestSquadVelocityValid)
      return(mSlowestSquadVelocity);
   return(10.0f);
}

//==============================================================================
//==============================================================================
bool BPlatoon::canJump() const
{
   //We can jump if everyone can jump.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      if (pSquad && !pSquad->canJump())
         return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
float BPlatoon::getPathingRadius() const
{
   // Get the radius of the largest unit
   float pathingRadius = 0.0f;
   float squadPathingRadius = 0.0f;
   uint squadIndex;
   for (squadIndex = 0; squadIndex < getNumberChildren(); squadIndex++)
   {
      const BSquad* pSquad = gWorld->getSquad(getChild(squadIndex));
      if (pSquad != NULL)
      {
         squadPathingRadius = Math::Max(squadPathingRadius, pSquad->getObstructionRadius());
         
         // jce [4/24/2008] -- path with the biggest squad always since the collision checks, etc. that happen
         // when following the path are based on squad and not dude size.
         #ifndef _MOVE4
         uint unitIndex;
         for (unitIndex = 0; unitIndex < pSquad->getNumberChildren(); unitIndex++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(unitIndex));
            if (pUnit != NULL)
            {
               pathingRadius = Math::Max(pathingRadius, pUnit->getObstructionRadius());
            }
         }
         #endif
      }
   }

   // Fallback to squad obstruction pathing radius
   #ifndef _MOVE4
   if (pathingRadius < cFloatCompareEpsilon)
   #endif
      pathingRadius = squadPathingRadius;
   if (pathingRadius < cFloatCompareEpsilon)
      pathingRadius = 2.0f;

   return pathingRadius;
}

//==============================================================================
//==============================================================================
void BPlatoon::calculateRange(BSimTarget* pTarget) const
{
   float range=0.0f;
   bool hasRange=false;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      float tempRange;
      if (pSquad && pSquad->calculateRange(pTarget, tempRange, NULL, NULL))
      {
         if (!hasRange || (tempRange < range))
         {
            range=tempRange;
            hasRange=true;
         }
      }
   }

   if (hasRange)
      pTarget->setRange(range);
}

//==============================================================================
//==============================================================================
float BPlatoon::getPathingRange() const
{
   float range = 0.0f;
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
      const BSquad* pSquad = gWorld->getSquad(mChildren[i]);
      if (pSquad != NULL)
         range = Math::Max(range, pSquad->getPathingRange());
   }
   return range;
}

//==============================================================================
//==============================================================================
float BPlatoon::getDesiredMovementProgress() const
{
   SCOPEDSAMPLE(BPlatoon_getDesiredMovementProgress);

   // Find the squad that's furthest along and return its progress
   float percentComplete = -1.0f;
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
      const BSquad* pSquad = gWorld->getSquad(mChildren[i]);
      if (pSquad == NULL)
         continue;
      
      const BSquadActionMove* pAction = reinterpret_cast<const BSquadActionMove*>(pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove));
      if (pAction == NULL)
         continue;

      float actionPercentComplete = pAction->getPercentComplete();
      if (actionPercentComplete > percentComplete)
         percentComplete = actionPercentComplete;
   }

   if (percentComplete < 0.0f)
      return 1.0f;
   return percentComplete;
}
//==============================================================================
//==============================================================================
bool BPlatoon::updatePreAsync(float elapsedTime)
{
   // Remove children queued to be removed
   for (int i = 0; i < mPendingChildrenToRemove.getNumber(); i++)
   {
      BSquad* pSquad = gWorld->getSquad(mPendingChildrenToRemove[i]);
      if (pSquad != NULL)
         removeChild(pSquad);
   }
   mPendingChildrenToRemove.clear();

   if (mChildren.getSize() == 0)
   {
      // Don't go away while we have active orders. That can happen when a new command is 
      // issued to a squad that's in the middle of locking down or unlocking.
      for (uint i=0; i < mOrders.getSize(); i++)
      {
         if (mOrders[i].getState() != BSimOrderEntry::cStateQueued && mOrders[i].getOrder()->getRefCount() > 0)
         {
#ifndef BUILD_FINAL
#ifndef BUILD_PROFILE
            {
               uint index = UINT_MAX;
               if (!BSimOrder::mFreeList.getIndex(mOrders[i].getOrder(), index))
               {
                  char buffer[1024];
                  sprintf_s(buffer, sizeof(buffer), "BPlatoon::updatePreAsync - mOrders references a BSimOrder (%s) that has been released. ", mOrders[i].getTypeName());
                  BASSERTM(0, buffer);
               }
            }
#endif
#endif

            BEntity::update(elapsedTime);
            return true;
         }
      }
#ifdef DEBUGMANAGEMENT
      debug("Update: No children, going away.");
#endif
      return(false);
   }

   //Determine our max velocity.  Make this event-based eventually.
   mSlowestSquadVelocityValid=false;
   mSlowestSquadVelocity=0.0f;

   #ifndef _MOVE4
   // none of this needs to be done in the new movement
   BVector averagePosition=cOriginVector;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad *squad=gWorld->getSquad(mChildren[i]);
      if (!squad)
         continue;
      averagePosition+=squad->getPosition();

      if (!mSlowestSquadVelocityValid ||
         (mSlowestSquadVelocity > squad->getDesiredVelocity()))
      {
         mSlowestSquadVelocity=squad->getDesiredVelocity();
         mSlowestSquadVelocityValid=true;
         mSlowestSquadID=squad->getID();
      }
   }
   averagePosition/=static_cast<float>(mChildren.getSize());
   setPosition(averagePosition);

   // Update formation..
   // jce [5/8/2008] -- actually this is being done in platoonactionmove now anyway and was getting done twice.
   updateFormation();
   #endif

   
   // We may need to do this check every n updates, versus every update, if it tanks perf. 
   // DLM 7/2/08 - Scary platoon splitting code here.  
   // Submit our squads to entity grouper.  If it thinks we need multiple platoons now, then
   // create each of the new platoons.. move all of our children to the new platoons.. pass our orders
   // to them, and schedule ourself for termination.  
   #ifdef PLATOON_SPLITTING
   const BEntityIDArray &squads = getChildList();

   BEntityGrouper *egr=gWorld->getEntityGrouper();
   egr->reset();
   //Set the radius.
   float platoonRadius;
   gConfig.get(cConfigPlatoonRadius, &platoonRadius);
   // Expand platoon radius a tad, so that platoons just split aren't immediately merged back together.
   platoonRadius += (float)(platoonRadius * 0.25);
   egr->setRadius(platoonRadius);
   egr->groupBySpeed(true);
   egr->groupEntities(squads);
   BArmy *pParentArmy = reinterpret_cast<BArmy *>(getParent());
   if (!pParentArmy)
   {
      debugMove4("BPlatoon::preAsyncUpdate -- Platoon doesn't have a parent WTF?");
      return false;
   }
   // The requirements for merging are the same for splitting, so only attempt to split if we
   // have more than one group and canMerge. 
   if (egr->getNumberGroups() > 1 && canMerge())
   {
      // One of the new entity groups should be left with the existing platoon.  We need to run through the groups,
      // get the average location of each of them, and the one closest to the original platoon position is the one
      // we skip.  Create platoons for the rest. 
      long nClosestIndex = -1;
      float fSmallestDist = cMaximumFloat;
      for (uint i=0; i < egr->getNumberGroups(); i++)
      {
         const BEntityGroup *entityGroup=egr->getGroupByIndex(i);
         if (!entityGroup)
            continue;

         const BEntityIDArray& tempSquads = entityGroup->getEntities();
         BVector averagePosition=cOriginVector;
         for (uint j = 0; j < tempSquads.getSize(); j++)
         {
            BSquad* pSquad = gWorld->getSquad(tempSquads[j]);
            if (pSquad)
            {
               averagePosition += pSquad->getPosition();
            }
         }
         averagePosition/=static_cast<float>(j);
         float fDistance = mPosition.xzDistance(averagePosition);
         if (fDistance < fSmallestDist)
            nClosestIndex = i;
      }
      if (nClosestIndex < 0)
      {
         debugMove4("Somehow, wanted to split platoon, but couldn't figure out which one was closest.");
         updateOrders();
         BEntity::update(elapsedTime);
         return(true);
      }

      // Okay..go through and break off the new platoons..
      BObjectCreateParms objectParms;
      objectParms.mPlayerID=mPlayerID;
      for (uint i=0; i < egr->getNumberGroups(); i++)
      {
         // Skip the closest one
         if (i == nClosestIndex)
            continue;

         const BEntityGroup *entityGroup=egr->getGroupByIndex(i);
         if (!entityGroup)
            continue;

         BPlatoon *platoon=gWorld->createPlatoon(objectParms);
         if (!platoon)
            return(false);  
         pParentArmy->addChild(platoon);

         // Give the new platoon the orders
         for (uint k=0; k < mOrders.getSize(); k++)
            platoon->queueOrder(mOrders[k].getOrder(), BSimOrder::cTypeNone);

         const BEntityIDArray& tempSquads = entityGroup->getEntities();
         BVector averagePosition=cOriginVector;
         for (uint j = 0; j < tempSquads.getSize(); j++)
         {
            BSquad* pSquad = gWorld->getSquad(tempSquads[j]);
            if (pSquad)
            {
               // Remove that child from us FIRST, as this clobbers parentID's and resets orders and stuff too.
               removeChild(pSquad);

               // Add the child to the new platoon.. 
               platoon->addChild(pSquad);           
               averagePosition += pSquad->getPosition();
            }
         }
         // For this update only, set the platoon's position to the average of the children.
         averagePosition/=static_cast<float>(platoon->getNumberChildren());
         platoon->setPosition(averagePosition);
      }
   }
   #endif

   //Update our orders.
   updateOrders();

   /* For now, let's not update formations dynamically...
   //If we have a formation and more than one squad and we have orders, do the dynamic priority thing.
   if (gConfig.isDefined(cConfigMoreNewMovement3))
   {
   if (mpFormation && (mChildren.getSize() > 0) && (mOrders.getSize() > 0))
   {
   // Calculate current forward vector for the platoon based on squad progress
   BVector forward = cZAxisVector;
   BPathingHints* pDefaultPathingHints = getPathingHints(cInvalidObjectID, true);
   if (pDefaultPathingHints)
   {
   int currentPathingHint = -1;
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
   BSquad* pSquad = gWorld->getSquad(mChildren[i]);
   if (!pSquad)
   continue;
   const BSquadActionMove* pSquadAction = reinterpret_cast<const BSquadActionMove*>(pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove));
   if (pSquadAction && (pSquadAction->getNumUserLevelWaypoints() == pDefaultPathingHints->mWaypoints.getSize()) && (currentPathingHint < static_cast<int>(pSquadAction->getCurrentUserLevelWaypointIndex())))
   currentPathingHint = pSquadAction->getCurrentUserLevelWaypointIndex();
   }

   if (currentPathingHint == -1)
   currentPathingHint = mPathingHintForwards.getSize() - 1;
   if ((currentPathingHint >= 0) && (currentPathingHint < mPathingHintForwards.getNumber()))
   forward = mPathingHintForwards[currentPathingHint];
   }

   // Calculate new priorities         
   mpFormation->calculateDynamicPriorities(forward);

   }
   }
   */

   //Do our base update.  Do this after orders because that probably changes our actions.
   BEntity::update(elapsedTime);

   // DLM - This is probably not the best place to put this, but I want this logic to live at the Platoon Level.
   // we don't have any notion of PlatoonAI, so for now, performing this check in Platoon::update seems like
   // the most expedient thing to do.  

   // Perform Squad Relocation
   // Are we already relocating?  If so, are we done?   
   if (mRelocationInProgress)
   {
      // Check the squad that's being relocated.  If it's stopped, we can relocate another squad.
      debugMove4("BPlatoon::preAsyncUpdate -- Relocation in Progress for squad (%d)", mSquadToRelocate);
      // if the index is invalide, then just bail. 
      if (mSquadToRelocate < 0 || mSquadToRelocate > mChildren.getNumber() - 1)
      {
         mRelocationInProgress = false;
         return true;
      }
      BSquad *pSquad = gWorld->getSquad(mChildren[mSquadToRelocate]);
      if (!pSquad)
      {
         mRelocationInProgress = false;
         return true;
      }
      BSquadActionMove *pMoveAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
      if (pMoveAction && pMoveAction->getID() == mRelocationActionID)
      {
         return true;
      }
      mRelocationInProgress = false;
      // Increment the squad count.
      mSquadToRelocate++;
      debugMove4("BPlatoon::preAsyncUpdate -- Relocation in Progress reset.  New Squad to Relocate is: (%d)", mSquadToRelocate);
      return true;
   }

   if (mSquadToRelocate > mChildren.getNumber() - 1)
      return true;

   // Only relocate if we don't have any orders.
   if (getNumberOrders() != 0)
      return true;

   // Okay check for a squad to relocate
   // jce [10/9/2008] -- turned of for now... see more detailed comments in function
   //doSquadRelocation();

   // [6/27/2008 xemu] removed minimap update from here, moved to individual squads 
   return(true);
}
//==============================================================================
//==============================================================================
bool BPlatoon::updateAsync(float elapsedTime)
{
   return(true);
}
//==============================================================================
//==============================================================================
bool BPlatoon::update(float elapsedTime)
{
   return(true);
}

//==============================================================================
//==============================================================================
void BPlatoon::mergePlatoon(BPlatoon* pPlatoon)
{
   //Go through all of the squads in the parm platoon and move them to us.
   //Sync their command to us, as well.
   #ifdef DEBUGMANAGEMENT
   debug("Merging Platoon %s into us.", pPlatoon->getID().getDebugString().getPtr());
   #endif
   
   debugMove4("Merging platoon %d into %d", pPlatoon->getID(), mID);

   for (uint i=0; i < pPlatoon->getNumberChildren(); i++)
   {
      BSquad* pSquad=gWorld->getSquad(pPlatoon->getChild(i));
      if (pSquad)
      {
         //Make the squad clean up all of its orders, since they won't be from us.
         pSquad->removeOrders(true, true, true, false);
      
         //Add it.
         if (addChild(pSquad))
            i--;
         
         //Give it our orders.
         for (uint j=0; j < mOrders.getSize(); j++)
            pSquad->queueOrder(mOrders[j].getOrder(), BSimOrder::cTypeNone);
         
      }
   }
   
   // jce [9/22/2008] -- DUSTY MADE ME DO THIS... force a formation update since if the platoon is stopped it
   // won't be updating every frame.
   updateFormation();
}

//==============================================================================
// Test to see if this platoon is merge able
//==============================================================================
bool BPlatoon::canMerge()
{
   // Can all child squads merge?
   uint numSquads = getNumberChildren();
   for (uint i = 0; i < numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(getChild(i));
      if (pSquad && !pSquad->canMerge())
      {
         return (false);
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BPlatoon::updateOrders()
{
   //Check for orders that have gone away on their own. 
   const BSimOrderEntry* pMainOrderEntry=NULL;
   BSimOrderEntry* pNextOrderEntry=NULL;
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      BSimOrder* pTempOrder=mOrders[i].getOrder();

      if (mOrders[i].getState() == BSimOrderEntry::cStateQueued)
      {
         if (!pNextOrderEntry || (pNextOrderEntry->getOrder()->getPriority() < pTempOrder->getPriority()))
            pNextOrderEntry=&(mOrders[i]);
         continue;
      }

#ifndef BUILD_FINAL
#ifndef BUILD_PROFILE
      {
         uint index = UINT_MAX;
         if (!BSimOrder::mFreeList.getIndex(pTempOrder, index))
         {
            char buffer[1024];
            sprintf_s(buffer, sizeof(buffer), "BPlatoon::updateOrders - mOrders references a BSimOrder (%s) that has been released. ", mOrders[i].getTypeName());
            BASSERTM(0, buffer);
         }
      }
#endif
#endif
      //If this order is executing and has a 0 ref count, we're done
      //with it.  Take it out of our list and mark it for deletion if 
      //we own it.  We also need to remove it from our children in case they
      //have a queued order for it or something.
      if (pTempOrder->getRefCount() <= 0)
      {
         if (mPathingHintsSimOrderID == pTempOrder->getID())
         {
            mPathingHints.clear();
            mPathingHintForwards.clear();
            mPathingHintsSimOrderID = 0;
            mPathingHintsTime = 0;
         }
         // DLM 11/7/08 - If we're releasing the order, then also set to null the order entry associated with it, 
         // so we don't try to use it below. 
         if (pNextOrderEntry == &mOrders[i])
            pNextOrderEntry = NULL;

         mOrders.removeIndex(i);
         i--;
         removeOrderFromChildren(pTempOrder, true, false);
         if (pTempOrder->getOwnerID() == mID)
            gSimOrderManager.markForDelete(pTempOrder);
      }
      else if (!pMainOrderEntry || (pMainOrderEntry->getOrder()->getPriority() < pTempOrder->getPriority()))
         pMainOrderEntry=&(mOrders[i]);
   }
   
   //If we have a queued order to execute and it's a higher priority
   //than our current main order, execute it.
   if (pNextOrderEntry)
   {
      if (!pMainOrderEntry ||
         (pNextOrderEntry->getOrder()->getPriority() > pMainOrderEntry->getOrder()->getPriority()) )
         #ifdef _MOVE4
         executeOrder_4(pNextOrderEntry);
         #else
         executeOrder(pNextOrderEntry);
         #endif
   }
}

//==============================================================================
//==============================================================================
void BPlatoon::executeOrder(BSimOrderEntry* pOrderEntry)
{
   //debug("BPlatoon::executeOrder: time=%d.", timeGetTime());
   SCOPEDSAMPLE(BPlatoon_executeOrder);
   removeOrders(true, false, true);
   stopChildren();

   //Turn it to executing.
   pOrderEntry->setState(BSimOrderEntry::cStateExecute);

   BSimOrder* pOrder = pOrderEntry->getOrder();

   // Generate the move data for the platoon
   BVector targetPosition;
   if (!getTargetPosFromOrder(pOrder, targetPosition))
      return;

   //If this is a platoon of warthogs that will use jump pathing, check and see if the waypoint is in a jump obstruction.  If so, move it out.
   repositionTargetLocationIfInsidePlayerObstruction( targetPosition );

   // If there are multiple waypoints then use the order waypoint list.  Otherwise
   // use the target position.  If the target is an entity there should only be one
   // waypoint in the order's list so use the target position since it is more accurate.
   // DLM 2-26-08 - in the current incarnation of the code, there are *never* multiple waypoints,
   // as each successive move command is a separate order. 
   BDynamicSimVectorArray tempWaypoints;
   tempWaypoints.add(targetPosition);
   const BDynamicSimVectorArray* waypoints = &tempWaypoints;
   if ((pOrder != NULL) && (pOrder->getWaypointNumber() > 1))
      waypoints = &(pOrder->getWaypoints());

   //Append == false to gPH says "This is a new move".  GPH also now sets
   //the position/direction of the squad and other "init"-ish code.
   // this is deprecated code anyway.  Commenting out because limiter interface has changed.
   /*
   BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
   if (pathLimiter)
   {
      SCOPEDSAMPLE(BPlatoon_executeOrder_pathLimiter);
      pathLimiter->resetTmpPathCalls();

      if (!pathLimiter->getDenyPathing(mPlayerID) && (pathLimiter->getNumPlatoonPathingCallsThisFrame() < gDatabase.getMaxPlatoonPathingCallsPerFrame()))
      {
         pathLimiter->incNumPlatoonPathingCallsThisFrame();
         if (pathLimiter->getNumPlatoonPathingCallsThisFrame() > pathLimiter->getMaxPlatoonPathingCallsPerFrame())
            pathLimiter->setMaxPlatoonPathingCallsPerFrame(pathLimiter->getNumPlatoonPathingCallsThisFrame());

         pathLimiter->setInGenPathingHints(true);
         bool bGeneratedPathingHints = generatePathingHints(*waypoints, pOrder, false);
         pathLimiter->setInGenPathingHints(false);

         if (pathLimiter->getTmpPathCalls() > pathLimiter->getMaxPathCallsInGenPathingHints())
            pathLimiter->setMaxPathCallsInGenPathingHints(pathLimiter->getTmpPathCalls());

         if (!bGeneratedPathingHints)
            return;
      }
      else
      {
         pathLimiter->incPathsDeniedLastFrame(mPlayerID);
         pathLimiter->incPlatoonFramesDenied(mPlayerID);
         if (pathLimiter->getPlatoonFramesDenied(mPlayerID) > pathLimiter->getMaxPlatoonFramesDenied(mPlayerID))
            pathLimiter->setMaxPlatoonFramesDenied(mPlayerID, pathLimiter->getPlatoonFramesDenied(mPlayerID));
         pOrderEntry->setState(BSimOrderEntry::cStateQueued);
      }
   }
   else
   {
      bool bGeneratedPathingHints = generatePathingHints(*waypoints, pOrder, false);
      if (!bGeneratedPathingHints)
         return;
   }
   */

   // Send the order to the squads
   queueOrderToChildren(pOrder);
}

//==============================================================================
//==============================================================================
void BPlatoon::removeOrders(bool current, bool queued, bool delayChildOrderRemoval)
{
   BASSERT(current || queued);

   for (uint i=0; i < mOrders.getSize(); i++)
   {
      if ((mOrders[i].getState() == BSimOrderEntry::cStateQueued) && !queued)
         continue;
      if ((mOrders[i].getState() == BSimOrderEntry::cStateExecute) && !current)
         continue;
   
      BSimOrder* pOrder=mOrders[i].getOrder();
      //If this was active, do the action/children cleanup.
      if (mOrders[i].getState() != BSimOrderEntry::cStateQueued)
      {
         // TRB 4/10/08:  Don't remove all actions for order.  Removing the order
         // will do this as long as the order is interruptible.  This was removing
         // uninterruptible orders before when platoons didn't get remade.

         //Remove any child order entries that might be tied to this order.
         // TRB 7/22/08 - Delay the removal of the child order in case the newly queued order
         // is the same and should be merged to prevent unnecessary action churn.
         removeOrderFromChildren(pOrder, false, delayChildOrderRemoval);
      }
      //If we own this order, mark it for deletion.
      if (pOrder->getOwnerID() == mID)
         gSimOrderManager.markForDelete(pOrder);
      //Remove this index.
      mOrders.removeIndex(i);
      i--;
   }
}

//==============================================================================
//==============================================================================
void BPlatoon::removeAllOrders()
{
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      BSimOrder* pOrder=mOrders[i].getOrder();
      //If this was active, do the action/children cleanup.
      if (mOrders[i].getState() != BSimOrderEntry::cStateQueued)
      {
         //Remove any actions that are a result of this order.
         removeAllActionsForOrder(pOrder);
         //Remove any child order entries that might be tied to this order.
         removeOrderFromChildren(pOrder, true, false);
      }
      //If we own this order, mark it for deletion.
      if (pOrder->getOwnerID() == mID)
         gSimOrderManager.markForDelete(pOrder);
      //Remove this index.
      mOrders.removeIndex(i);
      i--;
   }
}

//==============================================================================
//==============================================================================
void BPlatoon::removeOrderFromChildren(BSimOrder* pOrder, bool uninterruptible, bool delayChildOrderRemoval)
{
   //Remove it from our children.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      if (pSquad)
      {
         pSquad->removeOrder(pOrder, uninterruptible, delayChildOrderRemoval);
      }
   }
}

//==============================================================================
//==============================================================================
BSimOrderType BPlatoon::getOrderType(const BCommand* pCommand) const
{
   BSimTarget target(pCommand->getTargetID());
   if (pCommand->getNumberWaypoints() > 0)
      target.setPosition(pCommand->getWaypoint(pCommand->getNumberWaypoints()-1));
   if (pCommand->getAbilityID() != -1)
      target.setAbilityID(pCommand->getAbilityID());
   return(getOrderType(target, pCommand));
}

//==============================================================================
//==============================================================================
BSimOrderType BPlatoon::getOrderType(BSimTarget target, const BCommand* pCommand) const
{
   //NOTE: This is supposed to use the BSimTarget target as much as possible.
   //It may NOT be the target presented in the command.

   //NOTE2: If a squad returns an order type of move, check the other squads to see if they
   //can do something more interesting.  Platoon does some special case things if it only
   // thinks the squads can move.

   BSimOrderType rVal = BSimOrder::cTypeNone;

   //Repeat this until we get something that will work.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      if (pSquad)
      {
         BSimOrderType orderType = pSquad->getOrderType(target, pCommand, NULL);
         if (orderType == BSimOrder::cTypeMove)
            rVal = orderType;
         else if (orderType != BSimOrder::cTypeNone)
            return orderType;
      }
   }
   return rVal;
}

//==============================================================================
//==============================================================================
bool BPlatoon::queueOrder(const BCommand* pCommand)
{
   if (!pCommand)
      return (false);

   // AJL 2/6/08 - Don't do any process if somehow all are children are gone (to possibly fix a crash Jira bug 3026)
   if (getNumberChildren() == 0)
      return (false);

   // Log in the user class that these squads were just commanded (so we can flash the UI)
   if (gConfig.isDefined(cConfigShowCommands))
   {
      BUser* pPrimaryUser = gUserManager.getPrimaryUser();
      if (pPrimaryUser)
      {
         uint numChildren = mChildren.getSize();
         for (uint i=0; i<numChildren; i++)
            pPrimaryUser->commandSquad(mChildren[i]);
      }
   }

   //Figure out what to do with this command.  In a few cases, we might just
   //pass it on directly to our squads.
   BSimOrderType orderType=getOrderType(pCommand);
   if (orderType == BSimOrder::cTypeRallyPoint)
      return (queueOrderToChildren(pCommand));
   else if (orderType == BSimOrder::cTypeRepair)
   {
      removeOrders();
      return (queueOrderToChildren(pCommand));
   }
   else if (orderType == BSimOrder::cTypeChangeMode)
   {
      removeOrders();
      return (queueChangeModeOrderToChildren(pCommand));
   }

   // Attempt to append move waypoints onto current move action
   if ((orderType == BSimOrder::cTypeMove) && pCommand->getFlag(BCommand::cFlagAlternate) && pCommand->getAbilityID() == -1)
   {
      #ifdef _MOVE4
      if (tackOnWaypoints_4(pCommand))
         return true;
      #else
      if (tackOnWaypoints(pCommand))
         return true;
      #endif
   }

   //Don't issue new order if already capturing the target.
   if (orderType == BSimOrder::cTypeCapture && pCommand->getTargetID().getType() == BEntity::cClassTypeUnit)
   {
      const BUnit* pTargetUnit = gWorld->getUnit(pCommand->getTargetID());
      if (pTargetUnit && pTargetUnit->getFlagBeingCaptured())
      {
         for (int i=mChildren.getNumber()-1; i >= 0; i--)
         {
            const BSquad* pSquad=gWorld->getSquad(mChildren[i]);
            if (pSquad)
            {
               for (uint j=0; j<pSquad->getNumberChildren(); j++)
               {
                  if (pTargetUnit->getFirstEntityRefByID(BEntityRef::cTypeCapturingUnit, pSquad->getChild(j)))
                     return (false);
               }
            }
         }
      }
   }

   //Create the order (which we may not use, but whatever).
   BSimOrder* pOrder=gSimOrderManager.createOrder();
   if (!pOrder)
      return (false);

   //Create the target.  Do a quick check to make sure we're not passing dopples in
   //as targets.  Squads or units are fine, though.
   BSimTarget target;
   switch (pCommand->getTargetID().getType())
   {
      //Dopples.
      case BEntity::cClassTypeDopple:
      {
         const BDopple* pDopple=gWorld->getDopple(pCommand->getTargetID());
         if (!pDopple || (pDopple->getParentID() == cInvalidObjectID))
            return (false);
         target.setID(pDopple->getParentID());
         break;
      }
      //Squad/Unit.
      case BEntity::cClassTypeSquad:
      case BEntity::cClassTypeUnit:
      default:
         target.setID(pCommand->getTargetID());
         break;
   }

   // Make sure the last waypoint corresponds to the target entity position
   BDynamicVectorArray waypoints;
   waypoints = pCommand->getWaypointList();
   if (target.isIDValid())
   {
      const BEntity* pEntity = gWorld->getEntity(target.getID());
      if (pEntity != NULL)
      {
         // TRB 6/23/08 - Make sure target's position set to entity position
         target.setPosition(pEntity->getPosition());

         if (waypoints.getSize() > 0)
            waypoints.setAt(waypoints.getSize() - 1, pEntity->getPosition());
      }
   }

   if (waypoints.getSize() > 0)
      target.setPosition(waypoints[waypoints.getSize()-1]);
   if (pCommand->getAbilityID() >= 0)
      target.setAbilityID(pCommand->getAbilityID());
   if (orderType == BSimOrder::cTypeUnpack)
      target.setRange(0.0f);

   // TRB 6/27/08 - For move orders with a target ID, clear out the target ID.  In this case
   // the target is probably an ally unit.  This keeps platoon from calling the squad plotter
   // when it doesn't need to and keeps squads from moving into the middle of the target.
   if (orderType == BSimOrder::cTypeMove)
      target.invalidateID();
      
   if (pCommand->getType() == cCommandWork)
   {
      const BWorkCommand* pWorkCommand = (BWorkCommand*)pCommand;
      BDynamicVectorArray workWaypoints;
      workWaypoints = pCommand->getWaypointList();
      uint numWaypoints = workWaypoints.getSize();
      if (pWorkCommand->getFlagOverridePosition() && (numWaypoints > 0))
      {
         target.setPosition(workWaypoints[0]);
         pOrder->setFlagOverridePosition(true);
      }

      if (pWorkCommand->getFlagOverrideRange())
      {
         target.setRange(pWorkCommand->getRange());
         pOrder->setFlagOverrideRange(true);
      }
   }

   //Init it.
   pOrder->setOwnerID(mID);
   pOrder->setTarget(target);
   pOrder->setWaypoints(waypoints);
   if (pCommand->getSenderType() == BCommand::cTrigger)
      pOrder->setPriority(BSimOrder::cPriorityTrigger);
   else
      pOrder->setPriority(BSimOrder::cPriorityUser);
   if ((pCommand->getType() == cCommandWork) && pCommand->getFlag(BWorkCommand::cFlagAttackMove))
      pOrder->setAttackMove(true);
   pOrder->setAngle(pCommand->getAngle());
   pOrder->setMode((int8)pCommand->getSquadMode());

   //-- Let the sound systems know that a command was issued. Yeah we want to know even if they're dupe orders.
   notifySound(pOrder);

   //Now that we have constructed the actual order, deal with it.  If it's "alternate", it's queued, so just
   //go.  If it's not queued, then we check for dupes.  If we don't have a dupe, we flush all current/queued orders.
   if (!pCommand->getFlag(BCommand::cFlagAlternate))
   {
      // TRB 7/23/08 - Added back dupe checking but just for move orders.  We want non-move orders to be sent
      // down to the squads so they can decide if they are really dupes since they can interpret the orders differently.
      if ((orderType == BSimOrder::cTypeMove) && !pOrder->getTarget().isIDValid())
      {
         //Generic dupe checking.  Ignore any order that's a dupe of something already in the list.
         for (uint i=0; i < mOrders.getSize(); i++)
         {
            BSimOrder *pTempOrder=mOrders[i].getOrder();
            if (*pTempOrder == *pOrder)
            {
               gSimOrderManager.markForDelete(pOrder);
               return (false);
            }
         }
      }

      //If we're here, we have a new immediate "go now" order, so nuke everything else we have to date.
      // TRB 7/22/08 - This will now flag the current order for removal at the squad level.  When the new order
      // is queued, the squad will determine if it really is a dupe order.
      removeOrders(true, true, true);
   }
      

   //Add it.
   if (!queueOrder(pOrder, orderType))
   {
      gSimOrderManager.markForDelete(pOrder);
      return (false);
   }   

   return (true);
}


//==============================================================================
//==============================================================================
bool BPlatoon::queueOrder(BSimOrder* pOrder)
{
   //Add this order.  It's already been filled out by the caller (which may or
   //may not be us)
   if (!pOrder)
      return (false);

   BSimOrderType orderType=getOrderType(pOrder->getTarget(), NULL);
   return (queueOrder(pOrder, orderType));
}

//==============================================================================
//==============================================================================
bool BPlatoon::queueOrder(BSimOrder* pOrder, BSimOrderType orderType)
{
//   BASSERT(pOrder->getTarget().getPosition().x > -10.0f); // This is insufficient as a safety check since it does not check upper bounds, and its also
//   BASSERT(pOrder->getTarget().getPosition().z > -10.0f); // unnecessary and obstructive since it blocks calling off-map units like covenant bombing run bombers

   //Add this order.  It's already been filled out by the caller (which may or
   //may not be us)
   if (!pOrder)
      return (false);

   //Do dupe checking.  Ignore any order that's a dupe of something already in the list.
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      BSimOrder *pTempOrder=mOrders[i].getOrder();
      if (pTempOrder == pOrder)
         return (false);
      if (*pTempOrder == *pOrder)
         return (false);
   }

   //Add it.
   BSimOrderEntry soe;
   soe.setOrder(pOrder);
   soe.setType(orderType);
   soe.setState(BSimOrderEntry::cStateQueued);
   mOrders.add(soe);
   return (true);
}

//==============================================================================
//==============================================================================
bool BPlatoon::queueOrderToChildren(const BCommand* pCommand)
{
   //Return true if anyone returns true.
   bool rVal=false;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      if (pSquad && pSquad->queueOrder(pCommand))
         rVal=true;
   }
   return (rVal);
}

//==============================================================================
//==============================================================================
bool BPlatoon::queueOrderToChildren(BSimOrder* pOrder)
{
   //Return true if anyone returns true.
   bool rVal=false;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      if (pSquad && pSquad->queueOrder(pOrder, BSimOrder::cTypeNone))
         rVal=true;
   }
   return (rVal);
}

//==============================================================================
//==============================================================================
bool BPlatoon::queueChangeModeOrderToChildren(const BCommand *pCommand)
{
   // When one of the children interprets the command as a change order command
   // it means that at least one child is locked down.  The desired behavior is for
   // all locked down ones to unlock and stay where they are.  Unlocked children should
   // do nothing.  So only send the command to children who are going to change modes.
   BSimTarget target(pCommand->getTargetID());
   if (pCommand->getNumberWaypoints() > 0)
      target.setPosition(pCommand->getWaypoint(pCommand->getNumberWaypoints()-1));
   if (pCommand->getAbilityID() != -1)
      target.setAbilityID(pCommand->getAbilityID());

   bool rVal = true;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      if (pSquad)
      {
         BSimOrderType orderType = pSquad->getOrderType(target, pCommand, NULL);
         if (orderType == BSimOrder::cTypeChangeMode)
         {
            if (!pSquad->queueOrder(pCommand))
               rVal = false;
         }
      }
   }
   return rVal;
}

//==============================================================================
//==============================================================================
bool BPlatoon::queueMove(BSimTarget target, uint priority)
{
   //Create the order.
   BSimOrder* pOrder=gSimOrderManager.createOrder();
   if (!pOrder)
      return (false);

   //Init it.
   pOrder->setOwnerID(mID);
   pOrder->setTarget(target);
   pOrder->setPriority(priority);

   //Add it.
   if (!queueOrder(pOrder, BSimOrder::cTypeMove))
   {
      gSimOrderManager.markForDelete(pOrder);
      return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
bool BPlatoon::queueWork(BSimTarget target, uint priority)
{
   //Create the order.
   BSimOrder* pOrder=gSimOrderManager.createOrder();
   if (!pOrder)
      return (false);

   BSimOrderType orderType=getOrderType(target, NULL);
   if (orderType == BSimOrder::cTypeNone)
      return (false);

   //Init it.
   pOrder->setOwnerID(mID);
   pOrder->setTarget(target);
   pOrder->setPriority(priority);

   //Add it.
   if (!queueOrder(pOrder, orderType))
   {
      gSimOrderManager.markForDelete(pOrder);
      return (false);
   }
   return (true);
}

//==============================================================================
//==============================================================================
void BPlatoon::propagateAttackMove()
{
   //This is uber creepy, but I have my reasons for doing it this way.
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      BSimOrder *pTempOrder=mOrders[i].getOrder();
      pTempOrder->setAttackMove(true);
   }
}

//==============================================================================
//==============================================================================
bool BPlatoon::removeAllActionsForOrder(BSimOrder* pOrder)
{
   //Remove it from ourself.
   mActions.removeAllActionsForOrder(pOrder);

   //Remove it from our children.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      if (pSquad)
         pSquad->removeAllActionsForOrder(pOrder);
   }
  
   return(true);
}

//==============================================================================
//==============================================================================
void BPlatoon::stopChildren()
{
   //DCPTODO: I hate this.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      if (pSquad)
      {
         // Remove all but the uninterruptible orders
         pSquad->removeOrders(true, true, true, false, true);
      }
   }
}

//==============================================================================
//==============================================================================
bool BPlatoon::applyFormationToPathingHints(uint startHintIndex, uint endHintIndex, bool useSquadPlotter, BEntityID squadPlotterTargetID, long abilityID, float overrideRange /*= -1.0f*/, BVector overrideTargetPos /*= cInvalidVector*/, float overrideRadius /*= -1.0f*/)
{
    SCOPEDSAMPLE(BPlatoon_applyFormationToPathingHints)
   #ifdef DEBUGGPH
   debug("ApplyFormToPH: SHT=%d EHT=%d.", startHintIndex, endHintIndex);
   #endif

   // Get the default pathing hints
   int defaultPathingHintsIndex = -1;
   for (int i = 0; i < mPathingHints.getNumber(); i++)
   {
      if (mPathingHints[i].mID == cInvalidObjectID)
      {
         defaultPathingHintsIndex = i;
         break;
      }
   }
   if (defaultPathingHintsIndex == -1)
      return false;

   if (mpFormation == NULL)
      return false;

   // Validate start and end hint indices
   uint realStartHintIndex = startHintIndex;
   if (realStartHintIndex >= mPathingHints[defaultPathingHintsIndex].mWaypoints.getSize())
      realStartHintIndex = 0;
   uint realEndHintIndex = Math::Min(endHintIndex, (mPathingHints[defaultPathingHintsIndex].mWaypoints.getSize() - 1));

   // Copy the pathing hints for each of the squads.  Offset the waypoints by the formation and make sure they are in valid locations.
   for (uint wpIndex = realStartHintIndex; wpIndex <= realEndHintIndex; wpIndex++)
   {
      #ifdef DEBUGGPH
      debug("wpIndex: %d.", wpIndex);
      #endif

      bool haveValidPlots = false;

      if (useSquadPlotter)
      {
         BDynamicSimVectorArray foo;
         foo.setNumber(1);
         foo[0] = mPathingHints[defaultPathingHintsIndex].mWaypoints[wpIndex];
         DWORD flags = BSquadPlotter::cSPFlagIgnorePlayerRestriction;
         if (!overrideTargetPos.almostEqual(cInvalidVector))
         {
            flags = (BSquadPlotter::cSPFlagIgnorePlayerRestriction | BSquadPlotter::cSPFlagIgnoreWeaponLOS | BSquadPlotter::cSPFlagForceMove);
         }

         haveValidPlots = gSquadPlotter.plotSquads(getChildList(), getPlayerID(), squadPlotterTargetID, foo, mPathingHints[defaultPathingHintsIndex].mWaypoints[wpIndex], abilityID, flags, overrideRange, overrideTargetPos, overrideRadius);
         if (haveValidPlots && (gSquadPlotter.getResults().getNumber() != mChildren.getNumber()))
            haveValidPlots = false;
      }

      // Position and orient the formation at the current hint waypoint
      if (!haveValidPlots /* && (wpIndex == mPathingHints[defaultPathingHintsIndex].mWaypoints.getSize() - 1) */)
      // if (!haveValidPlots && (wpIndex == mPathingHints[defaultPathingHintsIndex].mWaypoints.getSize() - 1))
      {
         // Forward vector precalculated in when pathing hints were generated
         BASSERT(wpIndex < mPathingHintForwards.getSize());
         BVector forward = mPathingHintForwards[wpIndex];
         BVector right;
         right.assignCrossProduct(cYAxisVector, forward);

         BMatrix worldMatrix;      
         worldMatrix.makeOrient(forward, cYAxisVector, right);  
         worldMatrix.setTranslation(mPathingHints[defaultPathingHintsIndex].mWaypoints[wpIndex]);
         mpFormation->update(worldMatrix);
         #ifdef DEBUGGPH
         debug("Formating Transformed...");
         #endif
      }

      // Add the point for each squad
      for (uint squadIndex = 0; squadIndex < getNumberChildren(); squadIndex++)
      {
         BEntityID squadID = getChild(squadIndex);
         const BSquad* pSquad = gWorld->getSquad(squadID);
         if (pSquad == NULL)
            continue;

         // Make sure squad has pathing hints data and the proper number of waypoints
         BPathingHints* pSquadHints = getPathingHints(squadID, false);
         if (pSquadHints == NULL)
         {
            SCOPEDSAMPLE(BPlatoon_pSquadHintsinit)
            mPathingHints.grow();
            pSquadHints = &(mPathingHints[mPathingHints.getSize() - 1]);
            pSquadHints->init();
            pSquadHints->mID = squadID;
         }

         // Resize the squad's waypoints array to match the default array.
         // Fill out the new part of the squad's waypoints with the default waypoints.
         if (pSquadHints->mWaypoints.getSize() != mPathingHints[defaultPathingHintsIndex].mWaypoints.getSize())
         {
            uint tempIndex = pSquadHints->mWaypoints.getSize();
            pSquadHints->mWaypoints.setNumber(mPathingHints[defaultPathingHintsIndex].mWaypoints.getSize());
            for ( ; tempIndex < mPathingHints[defaultPathingHintsIndex].mWaypoints.getSize(); tempIndex++)
            {
               pSquadHints->mWaypoints.setAt(tempIndex, mPathingHints[defaultPathingHintsIndex].mWaypoints[tempIndex]);
            }
         }

         pSquadHints->mWaypoints.setAt(wpIndex, mPathingHints[defaultPathingHintsIndex].mWaypoints[wpIndex]);

         #ifdef DEBUGGPH
         debug("Updating Squad %d...", squadIndex);
         #endif
         // Use squad plotter positions
         if (haveValidPlots)
         {
            pSquadHints->mWaypoints.setAt(wpIndex, gSquadPlotter.getResults()[squadIndex].getDesiredPosition());
         }
         // Use formation positions
         else /*if (wpIndex == mPathingHints[defaultPathingHintsIndex].mWaypoints.getSize() - 1)*/
         {
            
            // Get formation position
            const BFormationPosition2* pFP = mpFormation->getFormationPosition(squadID);

            if (pFP != NULL)
            {
               // Use the new position as long as it's not bad (off map, in an obstruction, etc.).
               BVector newPosition = pFP->getPosition();
               #ifdef DEBUGGPH
               debug("Checking formation position (%f, %f)...", newPosition.x, newPosition.z);
               #endif
               if (pSquad->isPassable(newPosition, getChildList()))
               {
                  pSquadHints->mWaypoints.setAt(wpIndex, newPosition);
               }
            }
         }
      }
   }      

   // Restore the formation since action probably changed it
   {
      SCOPEDSAMPLE(BPlatoon_updateFormation)
      updateFormation();
   }

   #ifdef DEBUGGPH
   // Debug info
   for (uint squadIndex = 0; squadIndex < getNumberChildren(); squadIndex++)
   {
      BEntityID squadID = getChild(squadIndex);
      BSquad* pSquad = gWorld->getSquad(squadID);
      BPathingHints* pSquadHints = getPathingHints(squadID, false);
      if ((pSquad == NULL) || (pSquadHints == NULL) || (pSquadHints->mWaypoints.getSize() <= 1))
         continue;

      debug("  First Pass: %d waypoints:", pSquadHints->mWaypoints.getSize());
      for (uint i=0; i < pSquadHints->mWaypoints.getSize(); i++)
      {
         float distance;
         if (i == 0)
            distance=pSquadHints->mWaypoints[i].xzDistance(pSquad->getPosition());
         else
            distance=pSquadHints->mWaypoints[i].xzDistance(pSquadHints->mWaypoints[i-1]);
         debug("    WP[%d]: (%5.2f, %5.2f), distance=%f.", i, pSquadHints->mWaypoints[i].x, pSquadHints->mWaypoints[i].z, distance);
      }
   }
   #endif

   return true;
}

//==============================================================================
//==============================================================================
bool BPlatoon::getInitialPathingHintsForChild(BEntityID childID, uint startingHintIndex, BDynamicSimVectorArray& resultPathingHints, uint& maxSiblingHintIndex)
{
   // Find the pathing hints for the child
   const BPathingHints* pPathingHints = getPathingHints(childID, true);
   if (pPathingHints == NULL)
      return false;

   const BDynamicSimVectorArray& pathingHintWaypoints = pPathingHints->mWaypoints;

   // See if starting hint index is valid
   uint sourceIndex = startingHintIndex;
   if (sourceIndex > pathingHintWaypoints.getSize())
      sourceIndex = 0;

   resultPathingHints.setNumber(pathingHintWaypoints.getSize() - sourceIndex);

   // Copy the path into the waypoint list
   uint wpIndex;
   for (wpIndex = 0; wpIndex < resultPathingHints.getSize(); wpIndex++)
   {
      resultPathingHints.setAt(wpIndex, pathingHintWaypoints[sourceIndex]);
      sourceIndex++;
   }

   // In the case of platoons merging check the other squads to see how far along the pathing
   // hints they are.  Return the max index so the squad can skip waypoints that have been passed.
   maxSiblingHintIndex = 0;
   bool foundMaxSiblingHintIndex = false;
   BSquad* pSlowestSquad = gWorld->getSquad(mSlowestSquadID);
   if ((pSlowestSquad != NULL) && (mSlowestSquadID != childID))
   {
      BSquadActionMove* pAction = reinterpret_cast<BSquadActionMove*>(pSlowestSquad->getActionByType(BAction::cActionTypeSquadMove));
      if ((pAction != NULL) && (pAction->getOrder() != NULL) && (mPathingHintsSimOrderID == pAction->getOrder()->getID()))
      {
         maxSiblingHintIndex = pAction->getCurrentUserLevelWaypointIndex();
         foundMaxSiblingHintIndex = true;
      }
   }
   else
   {
      uint childIndex;
      for (childIndex = 0; childIndex < mChildren.getSize(); childIndex++)
      {
         if (mChildren[childIndex] != childID)
         {
            BSquad* pSquad = gWorld->getSquad(mChildren[childIndex]);
            if (pSquad != NULL)
            {
               BSquadActionMove* pAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
               if ((pAction != NULL) && (pAction->getOrder() != NULL) && (mPathingHintsSimOrderID == pAction->getOrder()->getID()))
               {
                  uint hintIndex = pAction->getCurrentUserLevelWaypointIndex();
                  foundMaxSiblingHintIndex = true;
                  if (hintIndex > maxSiblingHintIndex)
                     maxSiblingHintIndex = hintIndex;
               }
            }
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPlatoon::getFinalPathingHintForChild(BEntityID childID, uint pathingHintIndex, bool useSquadPlotter, BEntityID squadPlotterTargetID, long abilityID, BVector& finalPathingHint, float overrideRange /*= -1.0f*/, BVector overrideTargetPos /*= cInvalidVector*/, float overrideRadius /*= -1.0f*/)
{
   // Validate index
   const BPathingHints* pDefaultPathingHints = getPathingHints(cInvalidObjectID, false);
   if (pDefaultPathingHints == NULL)
      return false;
   if ((pathingHintIndex < 0) || (pathingHintIndex >= pDefaultPathingHints->mWaypoints.getSize()))
      return false;

   // If final pathing hints haven't been found then apply the formation to them or use the squad plotter to find them.
   if (!getFinalPathingHintValid(pathingHintIndex) && (pDefaultPathingHints->mWaypoints.getSize() > 0))
   {
      if (applyFormationToPathingHints(pathingHintIndex, pathingHintIndex, useSquadPlotter, squadPlotterTargetID, abilityID, overrideRange, overrideTargetPos, overrideRadius))
         setFinalPathingHintValid(pathingHintIndex, true);
   }

   // Return the pathing hints for the child
   BPathingHints* pPathingHints = getPathingHints(childID, true);
   if ((pPathingHints == NULL) || (pPathingHints->mWaypoints.getSize() == 0) || (pathingHintIndex >= pPathingHints->mWaypoints.getSize()))
      return false;
   finalPathingHint = pPathingHints->mWaypoints[pathingHintIndex];
   return true;
}

//==============================================================================
//==============================================================================
bool BPlatoon::getFinalPathingHintValid(uint pathingHintIndex) const
{
   // Use the unused fourth component of the forward vector as the validity flag to save on memory.
   // 0 == not valid, 1 == valid
   if ((pathingHintIndex < 0) || (pathingHintIndex >= mPathingHintForwards.getSize()))
      return true;
   return (abs(mPathingHintForwards[pathingHintIndex].w - 1.0f) < cFloatCompareEpsilon);
}

//==============================================================================
//==============================================================================
void BPlatoon::setFinalPathingHintValid(uint pathingHintIndex, bool valid)
{
   // Use the unused fourth component of the forward vector as the validity flag to save on memory.
   // 0 == not valid, 1 == valid
   if ((pathingHintIndex < 0) || (pathingHintIndex >= mPathingHintForwards.getSize()))
      return;
   if (valid)
      mPathingHintForwards[pathingHintIndex].w = 1.0f;
   else
      mPathingHintForwards[pathingHintIndex].w = 0.0f;
}

//==============================================================================
//==============================================================================
bool BPlatoon::getPathingHintsComplete(BEntityID childID, BSimOrder* pOrder)
{
   // Make sure order IDs match
   if ((pOrder == NULL) || (pOrder->getID() != mPathingHintsSimOrderID))
      return false;

   const BPathingHints* pPathingHints = getPathingHints(childID, false);
   if (pPathingHints != NULL)
      return (pPathingHints->mComplete);
   return false;
}

//==============================================================================
//==============================================================================
void BPlatoon::setPathingHintsComplete(BEntityID childID, BSimOrder* pOrder)
{
   // Make sure order IDs match
   if ((pOrder == NULL) || (pOrder->getID() != mPathingHintsSimOrderID))
      return;

   BPathingHints* pPathingHints = getPathingHints(childID, false);
   if (pPathingHints != NULL)
      pPathingHints->mComplete = true;
}

//==============================================================================
//==============================================================================
void BPlatoon::updateFormation(const BPath *path)
{
   BDEBUG_ASSERT(mpFormation);

   BMatrix worldMatrix;
   getWorldMatrix(worldMatrix);
   mpFormation->update(worldMatrix, path);
}

//==============================================================================
//==============================================================================
void BPlatoon::kill(bool bKillImmediately)
{
}

//==============================================================================
//==============================================================================
void BPlatoon::destroy()
{
   //Remove us from our parent, if we have one.
   BArmy *army=gWorld->getArmy(mParentID);
   if (army)
      army->removeChild(this);
}

//=============================================================================
//=============================================================================
BArmy* BPlatoon::getParentArmy() const
{
   return(gWorld->getArmy(mParentID));
}

//==============================================================================
//==============================================================================
long BPlatoon::addChild(BSquad *squad)
{
   #ifdef DEBUGMANAGEMENT
   BSimString squadProtoObjectName=(squad->getProtoSquad() ? squad->getProtoSquad()->getName() : squad->getProtoObject() ? squad->getProtoObject()->getName() : "NoProto");
   debug("AddChild: SquadID=%s (%s).", squad->getID().getDebugString().getPtr(), squadProtoObjectName.getPtr());
   #endif

   //Remove us from our old platoon.
   BPlatoon *oldPlatoon=gWorld->getPlatoon(squad->getParentID());
   if (oldPlatoon)
      oldPlatoon->removeChild(squad);

   //If this is the first squad in the platoon, set the platoon's position to
   //the squad's position.
   if (mChildren.getSize() == 0)
      setPosition(squad->getPosition());

   //Set us as the parent to the squad and add it to our child list.
   squad->setParentID(mID);
   long rIndex=mChildren.uniqueAdd(squad->getID());
   if (rIndex < 0)
      return(rIndex);

   //Add the child to the formation.   
   mpFormation->addChild(squad->getID());

   // DLM 11/3/08 - Add a range for the child
   mSquadRanges.insertAtIndex(-1.0f, rIndex);

   #ifndef _MOVE4
   // DLMHACK - If we have more than 10 squads, then convert from mob to platoon formation
   // DLM - removing this hack for _MOVE4
   if (mChildren.getNumber() > 9)
      mpFormation->setType(BFormation2::eTypePlatoon);
   #endif

   return(rIndex);
}

//==============================================================================
//==============================================================================
bool BPlatoon::removeChild(BSquad* pSquad)
{
   #ifdef DEBUGMANAGEMENT
   debug("RemoveChild: SquadID=%s.", pSquad->getID().getDebugString().getPtr());
   #endif

   //Bail if we don't have this.
   long index=mChildren.find(pSquad->getID());
   if (index == -1)
      return(false);

   //Remove from the formation.
   mpFormation->removeChild(pSquad->getID());

   // DLM 11/3/08 - Remove the associated range..
   mSquadRanges.removeIndex(index);

   //Make the squad clean up all of its orders, since it's not part of us anymore.
   // TRB 7/19/07:  Don't remove orders that can't be interrupted.  This prevents canceling
   // change mode orders in the middle of execution.  Change mode groups squads in individual
   // platoons while other commands put them in one squad so there can be a lot of platoon churn
   // when commanding multiple moves and mode changes.
   pSquad->removeOrders(true, true, true, false, true);

   //Remove it from us.
   mChildren.removeIndex(index);
   pSquad->setParentID(cInvalidObjectID);
   return(true);
}

//==============================================================================
//==============================================================================
BEntityID BPlatoon::getChild(uint index) const
{
   if (index >= (unsigned int) mChildren.getNumber())
      return(cInvalidObjectID);
   return(mChildren[index]);
}

//==============================================================================
//==============================================================================
void BPlatoon::addPendingChildToRemove(BSquad* squad)
{
   BDEBUG_ASSERT(squad);
   mPendingChildrenToRemove.uniqueAdd(squad->getID());
}

//==============================================================================
//==============================================================================
float BPlatoon::getDefaultMovementProjectionTime() const
{
   // Return time units should look into the future for their movement target
   return (1.0f);
#if 0
   // TRB 9/19/07: Use the projection time config that squad projections use.
   float projectionTime = 0.0f;
   if (gConfig.get(cConfigProjectionTime, &projectionTime))
   {
      if (projectionTime > cFloatCompareEpsilon)
         return projectionTime;
   }

   // Fallback 1
   // Base this on the platoon's desired velocity and the default distance
   // between the platoon path's waypoints
   // TRB 7/18/07: Cut the time in half becuase of a doubling in the default pather distance to
   // fit the scale of the units vs. the terrain.
   // TRB 9/19/07: Making the time smaller again so guys don't cut corners as much.
   float velocity = getDesiredVelocity();
   if (velocity > cFloatCompareEpsilon)
      return (cMaximumLowLevelDist / velocity * 0.2f);

   // Fallback 2
   return cDefaultMovementProjectionTime;
#endif
}

//==============================================================================
//==============================================================================
BEntityID BPlatoon::getHigherMovementPriority(BEntityID child1, BEntityID child2) const
{
   if (gConfig.isDefined(cConfigMoreNewMovement3))
   {
      BSquad* pChild1 = gWorld->getSquad(child1);
      BSquad* pChild2 = gWorld->getSquad(child2);
      if (pChild1 && pChild2)
      {
         // If in different platoons, last commanded group has priority
         if (pChild1->getParentID() != pChild2->getParentID())
         {
            DWORD commandTime1 = getMoveActionStartTime(pChild1);
            DWORD commandTime2 = getMoveActionStartTime(pChild2);
            if (commandTime1 < commandTime2)
               return child2;
            else if (commandTime1 > commandTime2)
               return child1;
         }
         // If in same platoon, use formation priority
         else
         {
            uint child1Priority = mpFormation->getChildPriority(child1);
            uint child2Priority = mpFormation->getChildPriority(child2);
            if (child2Priority < child1Priority)
               return child2;
            else if (child2Priority > child1Priority)
               return child1;
         }

         // After that faster squads have priority
         float velocityDiff = pChild2->getDesiredVelocity() - pChild1->getDesiredVelocity();
         if (velocityDiff > cFloatCompareEpsilon)
            return child2;
         else if (velocityDiff < -cFloatCompareEpsilon)
            return child1;
      }

      // Use entity ID as tiebreaker for now
      if (child1.asLong() < child2.asLong())
         return child1;
      return child2;
   }

   else
   {
      uint child1Priority=mpFormation->getChildPriority(child1);
      uint child2Priority=mpFormation->getChildPriority(child2);

      // One of the children isn't a member of this platoon.
      // Do something smarter here.  Uses entity ID as tiebreaker for now.
      if ((child1Priority == BFormationPosition2::cInvalidPriority) || (child2Priority == BFormationPosition2::cInvalidPriority))
      {
         child1Priority = child1.asLong();
         child2Priority = child2.asLong();
      }

      if (child2Priority < child1Priority)
         return (child2);
      return (child1);
   }
}

//==============================================================================
//==============================================================================
uint BPlatoon::getDefaultMovementPriority(BEntityID childID) const
{
   return (mpFormation->getDefaultChildPriority(childID));
}

//==============================================================================
//==============================================================================
uint BPlatoon::getMovementPriority(BEntityID childID) const
{
   return (mpFormation->getChildPriority(childID));
}

//==============================================================================
// Check to see if the whole platoon is outside the playable bounds
//==============================================================================
bool BPlatoon::isOutsidePlayableBounds(bool forceCheckWorldBoundaries) const
{
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(getChild(i));
      if (pSquad && pSquad->isOutsidePlayableBounds(forceCheckWorldBoundaries))
      {
         return (true);
      }
   }

   return (BEntity::isOutsidePlayableBounds(forceCheckWorldBoundaries));
}

//==============================================================================
//==============================================================================
void BPlatoon::debugRender()
{
   BEntity::debugRender();

   // Show formation at current hover point
   /*
   if (gConfig.isDefined(cConfigDebugPlatoonFormations))
   {
      BUser* pUser = gUserManager.getPrimaryUser();
      if ((pUser != NULL) && pUser->getFlagHaveHoverPoint())
      {
         BVector forward = (pUser->getHoverPoint() - getPosition());
         forward.normalize();
         BVector right;
         right.assignCrossProduct(cYAxisVector, forward);

         BMatrix worldMatrix;      
         worldMatrix.makeOrient(forward, cYAxisVector, right);  
         worldMatrix.setTranslation(pUser->getHoverPoint());
         mpFormation->transformFormation(worldMatrix);

         mpFormation->debugRender();

         // Restore formation
         updateFormation();
      }
   }
   // Formation based on current position
   else
   {
      mpFormation->debugRender();
   }
   */
#ifdef _MOVE4
   BVector wp1 = cOriginVector;
   BVector wp2 = cOriginVector;
   if (mCurrentUserPath_4.getNumberWaypoints() >= 2)
   {
      wp1 = mCurrentUserPath_4.getWaypoint(0);
      gTerrainSimRep.addDebugPointOverTerrain(wp1, 2.0f, cDWORDWhite, 1.0f, BDebugPrimitives::cCategoryPathing);
      for (long l = 1; l < mCurrentUserPath_4.getNumberWaypoints(); l++)
      {
         wp2 = mCurrentUserPath_4.getWaypoint(l);
         gTerrainSimRep.addDebugLineOverTerrain(wp1, wp2, cDWORDWhite, cDWORDWhite, 1.0f, BDebugPrimitives::cCategoryPathing);
         gTerrainSimRep.addDebugPointOverTerrain(wp2, 2.0f, cDWORDWhite, 1.0f, BDebugPrimitives::cCategoryPathing);
         wp1 = wp2;
      }
   }
   if (mCurrentHLPath_4.getNumberWaypoints() >= 2)
   {
      wp1 = mCurrentHLPath_4.getWaypoint(0);
      gTerrainSimRep.addDebugPointOverTerrain(wp1, 2.0f, cDWORDYellow, 1.0f, BDebugPrimitives::cCategoryPathing);
      for (long l = 1; l < mCurrentHLPath_4.getNumberWaypoints(); l++)
      {
         wp2 = mCurrentHLPath_4.getWaypoint(l);
         gTerrainSimRep.addDebugLineOverTerrain(wp1, wp2, cDWORDYellow, cDWORDYellow, 1.0f, BDebugPrimitives::cCategoryPathing);
         gTerrainSimRep.addDebugPointOverTerrain(wp2, 2.0f, cDWORDYellow, 1.0f, BDebugPrimitives::cCategoryPathing);
         wp1 = wp2;
      }
   }

   if (mCurrentLLPath_4.getNumberWaypoints() >= 2)
   {
      wp1 = mCurrentLLPath_4.getWaypoint(0);
      gTerrainSimRep.addDebugPointOverTerrain(wp1, 2.0f, cDWORDGreen, 1.0f, BDebugPrimitives::cCategoryPathing);
      for (long l = 1; l < mCurrentLLPath_4.getNumberWaypoints(); l++)
      {
         wp2 = mCurrentLLPath_4.getWaypoint(l);
         gTerrainSimRep.addDebugLineOverTerrain(wp1, wp2, cDWORDGreen, cDWORDGreen, 1.0f, BDebugPrimitives::cCategoryPathing);
         gTerrainSimRep.addDebugPointOverTerrain(wp2, 2.0f, cDWORDGreen, 1.0f, BDebugPrimitives::cCategoryPathing);
         wp1 = wp2;
      }
   }
   // If we've plotted squad positions, I want to see those, rather than formation positions.. 
   if (mValidPlots_4)
   {
      // Display Squad Plot position..
      for (long i = 0; i < mSquadPlots_4.getNumber(); i++)
      {
         BDynamicSimVectorArray &waypoints = (BDynamicSimVectorArray&) mSquadPlots_4[i].getWaypoints();
         for (long j = 0; j < waypoints.getNumber() - 1; j++)
         {
            gTerrainSimRep.addDebugLineOverTerrain(waypoints[j], waypoints[j + 1], cDWORDBlue, cDWORDBlue, 0.5f);
         }

         BVector pts[4];
         if(mSquadPlots_4[i].getSquadCorners(pts))
         {
            DWORD color = cDWORDBlue;
            if (mSquadPlots_4[i].isObstructed())
               color = cDWORDRed;
            gTerrainSimRep.addDebugSquareOverTerrain(pts[0], pts[1], pts[2], pts[3], color, 0.5f);
         }

         // Add line to show who's going where
         BEntity* pSquad = gWorld->getEntity(mSquadPlots_4[i].getSquadID());
         if (pSquad)
         {
            gTerrainSimRep.addDebugLineOverTerrain(pSquad->getPosition(), mSquadPlots_4[i].getDesiredPosition(), cDWORDWhite, cDWORDWhite, 0.5f);
         }
      }
   }
   else
      mpFormation->debugRender();
   return;
#endif

   // Don't do the formation at cursor thing anymore.. just show the formations.. 
   mpFormation->debugRender();

   // Command Path
   if (mDebugCommandPath.getNumberWaypoints() > 0)
   {
      gTerrainSimRep.addDebugPointOverTerrain(mDebugCommandPath.getWaypoint(0), 2.0f, cDWORDYellow, 1.0f, BDebugPrimitives::cCategoryPathing);
      for (long l = 0; l < mDebugCommandPath.getNumberWaypoints() - 1; l++)
      {
         gTerrainSimRep.addDebugLineOverTerrain(mDebugCommandPath.getWaypoint(l), mDebugCommandPath.getWaypoint(l+1), cDWORDYellow, cDWORDYellow, 1.0f, BDebugPrimitives::cCategoryPathing);
         gTerrainSimRep.addDebugPointOverTerrain(mDebugCommandPath.getWaypoint(l+1), 2.0f, cDWORDYellow, 1.0f, BDebugPrimitives::cCategoryPathing);
      }
   }

   /*
   // Reduced Path
   if (mDebugReducedPath.getNumberWaypoints() > 0)
   {
      gTerrainSimRep.addDebugPointOverTerrain(mDebugReducedPath.getWaypoint(0), 2.0f, cDWORDRed, 1.0f, BDebugPrimitives::cCategoryPathing);
      for (long l = 0; l < mDebugReducedPath.getNumberWaypoints() - 1; l++)
      {
         gTerrainSimRep.addDebugLineOverTerrain(mDebugReducedPath.getWaypoint(l), mDebugReducedPath.getWaypoint(l+1), cDWORDRed, cDWORDRed, 1.0f, BDebugPrimitives::cCategoryPathing);
         gTerrainSimRep.addDebugPointOverTerrain(mDebugReducedPath.getWaypoint(l+1), 2.0f, cDWORDRed, 1.0f, BDebugPrimitives::cCategoryPathing);
      }
   }
   */

   // Forward Position
   gTerrainSimRep.addDebugPointOverTerrain(mDebugForwardPosition, 2.0f, cDWORDPurple, 2.0f, BDebugPrimitives::cCategoryPathing);

   // Pathing hints
   for (uint hintIndex = 0; hintIndex < mPathingHints.getSize(); hintIndex++)
   {
      // Umm.. what?
      /*
      DWORD pointColor = cDWORDGreen;
      if (mPathingHints[hintIndex].mID != cInvalidObjectID)
      {
         #ifdef PATHING_HINTS_USE_PLOTTER
            pointColor = packRGBA(BColor(0.0f, 0.5f, 0.0f), 1.0f);
         #else
            continue;
         #endif
      }
      */
      if (mPathingHints[hintIndex].mID == cInvalidObjectID)
         continue;

      if (mPathingHints[hintIndex].mWaypoints.getSize() > 0)
      {
         gTerrainSimRep.addDebugPointOverTerrain(mPathingHints[hintIndex].mWaypoints[0], 2.0f, cDWORDCyan, 2.0f, BDebugPrimitives::cCategoryPathing);
         for (uint i = 0; i < mPathingHints[hintIndex].mWaypoints.getSize() - 1; i++)
         {
            gTerrainSimRep.addDebugLineOverTerrain(mPathingHints[hintIndex].mWaypoints[i], mPathingHints[hintIndex].mWaypoints[i+1], cDWORDCyan, cDWORDCyan, 0.75f, BDebugPrimitives::cCategoryPathing);
            gTerrainSimRep.addDebugPointOverTerrain(mPathingHints[hintIndex].mWaypoints[i+1], 2.0f, cDWORDCyan, 2.0f, BDebugPrimitives::cCategoryPathing);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BPlatoon::isPathableAsFlyingUnit() const
{
   // Oh snap!  We're out of the playable bounds we better path like we have wings.
   if (getPlayer() && !getPlayer()->isHuman() && isOutsidePlayableBounds())
      return (true);

   // All of the platoon's squads should by flying for platoon to use air pather
   uint squadIndex;
   for (squadIndex = 0; squadIndex < getNumberChildren(); squadIndex++)
   {
      const BSquad* pSquad = gWorld->getSquad(getChild(squadIndex));
      if ((pSquad == NULL) || (pSquad->getProtoObject() == NULL) || (pSquad->getProtoObject()->getMovementType() != cMovementTypeAir))
         return false;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPlatoon::isPathableAsFloodUnit() const
{
   // All of the platoon's squads should by flood movers for platoon to use flood pather
   uint squadIndex;
   for (squadIndex = 0; squadIndex < getNumberChildren(); squadIndex++)
   {
      const BSquad* pSquad = gWorld->getSquad(getChild(squadIndex));
      if ((pSquad == NULL) || (pSquad->getProtoObject() == NULL) || (pSquad->getProtoObject()->getMovementType() != cMovementTypeFlood))
         return false;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPlatoon::isPathableAsScarabUnit() const
{
   // All of the platoon's squads should by scarab movers for platoon to use scarab pather
   uint squadIndex;
   for (squadIndex = 0; squadIndex < getNumberChildren(); squadIndex++)
   {
      const BSquad* pSquad = gWorld->getSquad(getChild(squadIndex));
      if ((pSquad == NULL) || (pSquad->getProtoObject() == NULL) || (pSquad->getProtoObject()->getMovementType() != cMovementTypeScarab))
         return false;
   }

   return true;
}

//==============================================================================
// getMovementType - returns the movment type of the platoon.
// This relies on the entity grouper basically assuring us that platoon's will
// only ever have a single movement type within them.  If that is no longer
// the case, this function will have to do something different. 
//==============================================================================
long BPlatoon::getMovementType()
{
   
   BSquad *pSquad = NULL;
   if (mChildren.getNumber() < 1)
      return -1;

   if (gWorld->getEntity(mChildren[0]))
   {
      pSquad = gWorld->getEntity(mChildren[0])->getSquad();
      if (pSquad && pSquad->getProtoObject())
      {
         return pSquad->getProtoObject()->getMovementType();
      }
   }
   return -1;
}
//==============================================================================
//==============================================================================
bool BPlatoon::getTargetPosFromOrder(BSimOrder* pOrder, BVector& targetPosition)
{
   // Get the target position if there is one
   bool haveTarget = false;
   if (pOrder != NULL)
   {
      BSimTarget target = pOrder->getTarget();
      if (target.isIDValid())
      {
         const BEntity* pEntity = gWorld->getEntity(target.getID());
         if (pEntity != NULL)
         {
            targetPosition = pEntity->getPosition();
            haveTarget = true;
         }
         else if (target.isPositionValid())
         {
            targetPosition = target.getPosition();
            haveTarget = true;
         }
      }
      else if (target.isPositionValid())
      {
         targetPosition = target.getPosition();
         haveTarget = true;
      }
   }
   return haveTarget;
}

//==============================================================================
//==============================================================================
BPathingHints* BPlatoon::getPathingHints(BEntityID childID, bool returnDefaultIfNotFound)
{
   // Find the pathing hints for the ID
   BPathingHints* pPathingHints = NULL;
   for (int i = 0; i < mPathingHints.getNumber(); i++)
   {
      if (mPathingHints[i].mID.getIndex() == childID.getIndex())
      {
         pPathingHints = &(mPathingHints[i]);
         break;
      }
      else if (returnDefaultIfNotFound && (mPathingHints[i].mID == cInvalidObjectID))
         pPathingHints = &(mPathingHints[i]);
   }
   return pPathingHints;
}

//==============================================================================
//==============================================================================
bool BPlatoon::generatePathingHints(const BDynamicSimVectorArray& waypoints, BSimOrder* pOrder, bool append)
{
   SCOPEDSAMPLE(BPlatoon_generatePathingHints);
   #ifdef DEBUGGPH
   debug("");
   debug("");
   debug("GPH:  %d waypoints:", waypoints.getSize());
   for (uint a=0; a < waypoints.getSize(); a++)
      debug("  WP[%d]: (%5.2f, %5.2f).", a, waypoints[a].x, waypoints[a].z);
   #endif

   BASSERT(pOrder);
   if (waypoints.getSize() == 0)
      return (false);

   if (!append)
   {
      mPathingHints.clear();
      mPathingHintForwards.clear();
   }

   // Get default pathing hints data
   BPathingHints* pDefaultPathingHints = getPathingHints(cInvalidObjectID, true);
   if (pDefaultPathingHints == NULL)
   {
      mPathingHints.grow();
      pDefaultPathingHints = &(mPathingHints[mPathingHints.getSize() - 1]);
      pDefaultPathingHints->init();
   }
   BDynamicSimVectorArray& defaultHintWaypoints = pDefaultPathingHints->mWaypoints;

   // Setup path finder
   BFindPathHelper pathFinder;
   BSimTarget target = pOrder->getTarget();
   pathFinder.setTarget(&target);
   pathFinder.setEntity(this);
   pathFinder.setPathingRadius(getPathingRadius());
   pathFinder.setPathableAsFlyingUnit(isPathableAsFlyingUnit());
   pathFinder.setPathableAsFloodUnit(isPathableAsFloodUnit());
   pathFinder.setPathableAsScarabUnit(isPathableAsScarabUnit());
   pathFinder.enableReducePath(true);
   pathFinder.enablePathAroundSquads(false);

   // Get target and range to pass to the pather since we want to path the platoon just within range
   long targetID = -1L;
   if (target.isIDValid() || target.isPositionValid())
   {
      targetID = target.getID();
      calculateRange(&(pathFinder.getTarget()));
   }

   static BPath pathToFirstWaypoint;
   pathToFirstWaypoint.reset();
   bool pathToFirstWaypointValid = false;

   //Figure out whether we need to create a new position from scratch or use
   //the last one in the existing pathing hints.
   BVector startPosition;
   if (!append || (mPathingHints.getSize() == 0))
   {
      syncPositionWithSquads(waypoints, target, pathToFirstWaypoint, pathToFirstWaypointValid);
      startPosition=getPosition();
   }
   else
      startPosition = defaultHintWaypoints[defaultHintWaypoints.getSize() - 1];


   // Find the path.
   static BPath path;
   path.reset();
   uint findPathResult = BPath::cFailed;
   if (pathToFirstWaypointValid)
   {
      // Reuse path found earlier.  Reduce it so it can be used as pathing hints.
      findPathResult = BPath::cFull;
      // DLM - reduce path produces "anomolous" results.  Removing it.      
      //pathFinder.reducePath(pathToFirstWaypoint);
      pathFinder.appendPath(pathToFirstWaypoint, path);
      BDynamicSimVectorArray realUserWaypoints(waypoints);
      realUserWaypoints.removeIndex(0, true);

      #ifdef DEBUGGPH
      debug("After \"reducing\" the path..: %d waypoints:", pathToFirstWaypoint.getNumberWaypoints());
      for (int a=0; a < pathToFirstWaypoint.getNumberWaypoints(); a++)
         debug("  WP[%d]: (%5.2f, %5.2f).", a, pathToFirstWaypoint.getWaypoint(a).x, pathToFirstWaypoint.getWaypoint(a).z);
      #endif
      BASSERT(mpFormation);

      #ifndef BUILD_FINAL
      long value = 0;
      gConfig.get(cConfigRenderSimDebug, &value);
      if (value == 2)
      {
         mDebugReducedPath.zeroWaypoints();
         for (long i=0; i < pathToFirstWaypoint.getNumberWaypoints(); i++)
            mDebugReducedPath.addWaypointAtEnd(pathToFirstWaypoint.getWaypoint(i));
      }
      #endif

      // Generate the rest of the paths if there are more waypoints
      if (realUserWaypoints.getSize() > 0)
      {
         static BPath tempPath;
         tempPath.reset();
         #ifdef DEBUGGPH
         debug("  More waypoints available, generating paths for the remaining ones..");
         #endif
         findPathResult = pathFinder.findPath(tempPath, realUserWaypoints, &(waypoints[0]), targetID, true);
         #ifdef DEBUGGPH
         debug("  Path Result was: %d", findPathResult);
         #endif
         if ((findPathResult != BPath::cFailed) && (tempPath.getNumberWaypoints() > 0))
         {
            if (!pathFinder.appendPath(tempPath, path))
               findPathResult = BPath::cFailed;
         }
      }
   }
   else
   {
      findPathResult = pathFinder.findPath(path, waypoints, &startPosition, targetID, true);
   }

   // Copy results into platoon waypoint array
   if (findPathResult != BPath::cFailed)
   {
      if(path.getNumberWaypoints() <= 0)
      {
         BUnit *pTarget = gWorld->getUnit(targetID);
         if(pTarget)
            path.addWaypointAtEnd(pTarget->getPosition());
      }

      SCOPEDSAMPLE(BPlatoon_generatePathingHints_copyResults)

      //If a warthog platoon is trying to go back over a one way jump, stop it.
      repositionWaypointIfOneWayBarrierIsCrossed(path);

      const BDynamicVectorArray& newWaypoints = path.getWaypointList();
      uint wpIndex = 0;

      // Skip duplicate waypoint at the beginning of new path
      if ((defaultHintWaypoints.getSize() > 0) && (newWaypoints.getSize() > 0))
      {
         if (defaultHintWaypoints[defaultHintWaypoints.getSize() - 1].almostEqualXZ(newWaypoints[0]))
            wpIndex++;
      }
      // Also skip it if the platoon is already at the first waypoint
      else if (newWaypoints.getSize() > 1)
      {
         if (newWaypoints[0].almostEqualXZ(getPosition()))
            wpIndex++;
      }

      #ifdef DEBUGGPH
      debug("  Copying PathingHints Path into defaultPathingHints for platoon..");
      #endif


      // Calculate and save forward vector
      {
         SCOPEDSAMPLE(Calculate_and_save_forward_vector)
         BVector forward;
         if (wpIndex >= 0 && wpIndex < (signed)newWaypoints.getSize())         
            forward = newWaypoints[wpIndex] - getPosition();
         else
            forward = cOriginVector;
         for ( ; wpIndex < newWaypoints.getSize(); wpIndex++)
         {
            defaultHintWaypoints.add(newWaypoints[wpIndex]);

            // DLM 2/25/08 - Instead of using the segment behind me as my current forward vector,
            // let's use the segment in front of me as my forward vector.  The last waypoint will just use
            // the previous waypoint's forward vector. 
            if (wpIndex < newWaypoints.getSize() - 1)
            {
               forward = newWaypoints[wpIndex+1] - newWaypoints[wpIndex];
            }
            /*
            uint newWaypointIndex = defaultHintWaypoints.getSize() - 1;
            if (newWaypointIndex == 0)
            {
            forward = defaultHintWaypoints[newWaypointIndex] - getPosition();
            if (forward.almostEqualXZ(cOriginVector))
            {
            if ((wpIndex + 1) < newWaypoints.getSize())
            {
            forward = newWaypoints[wpIndex + 1] - defaultHintWaypoints[newWaypointIndex];
            if (forward.almostEqualXZ(cOriginVector))
            forward = getForward();
            }
            else
            forward = getForward();
            }
            }
            else
            {
            forward = defaultHintWaypoints[newWaypointIndex] - defaultHintWaypoints[newWaypointIndex - 1];
            }
            */
            forward.y = 0.0f;
            forward.w = 0.0f;       // Final pathing hint validity uses w component.  0 == not valid.
            forward.normalize();
            mPathingHintForwards.add(forward);
         }
      }

      // If appending waypoints, make sure the squad's pathing hints array is the same size.
      // If the squad doesn't have pathing hints at all then it's ok to leave it that way.
      if (append)
      {
         for (uint squadIndex = 0; squadIndex < mPathingHints.getSize(); squadIndex++)
         {
            if (mPathingHints[squadIndex].mID == cInvalidObjectID)
               continue;

            if (mPathingHints[squadIndex].mWaypoints.getSize() != defaultHintWaypoints.getSize())
            {
               uint tempIndex = mPathingHints[squadIndex].mWaypoints.getSize();
               mPathingHints[squadIndex].mWaypoints.setNumber(defaultHintWaypoints.getSize());
               for ( ; tempIndex < defaultHintWaypoints.getSize(); tempIndex++)
               {
                  mPathingHints[squadIndex].mWaypoints.setAt(tempIndex, defaultHintWaypoints[tempIndex]);
               }
            }
         }
      }

      mPathingHintsSimOrderID = pOrder->getID();
      mPathingHintsTime = gWorld->getGametime();
   }

   return (findPathResult != BPath::cFailed);
}

//==============================================================================
//==============================================================================
void BPlatoon::syncPositionWithSquads(const BDynamicSimVectorArray& waypoints, BSimTarget target, BPath& pathToFirstWaypoint, bool& pathValid)
{
   pathToFirstWaypoint.reset();
   pathValid = false;

   #ifdef DEBUGGPH
   debug("SyncPosWithSquads2:  %d waypoints:", waypoints.getSize());
   for (uint a=0; a < waypoints.getSize(); a++)
      debug("  WP[%d]: (%5.2f, %5.2f).", a, waypoints[a].x, waypoints[a].z);
   #endif
   BASSERT(mpFormation);

   //TODO: Ensure that a new platoon of >1 unit has MakeNeeded on the formation.
   //Check the "remake formations" config.
   #ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigRemakePlatoonFormations))
   {
      #ifdef DEBUGGPH
      debug("  Setting Make and Reassign to true due to RemakeFormations.");
      #endif
      mpFormation->setMakeNeeded(true);
      mpFormation->setReassignNeeded(true);
   }
   else
   #endif
   if (mFlagFirstUpdate)
   {
      #ifdef DEBUGGPH
      debug("  Setting Make and Reassign to true since this is the first update.");
      #endif
      mpFormation->setMakeNeeded(true);
      mpFormation->setReassignNeeded(true);
   }
   
   //Get the average squad position.
   BVector averagePosition=cOriginVector;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      const BSquad* pSquad=gWorld->getSquad(mChildren[i]);
      BDEBUG_ASSERT(pSquad);
      if (pSquad)
      {
         averagePosition+=pSquad->getPosition();
         #ifdef DEBUGGPH
         debug("  SquadID=%d Position=(%5.2f, %5.2f).", pSquad->getID().asLong(), averagePosition.x, averagePosition.z);
         #endif
      }
   }
   averagePosition/=static_cast<float>(mChildren.getSize());
   #ifdef DEBUGGPH
   debug("    AveragePosition=(%5.2f, %5.2f).", averagePosition.x, averagePosition.z);
   #endif

   //See if that position is passable.  If not, we will bump it to a known valid
   //location of a squad.
   //TODO: Does this need to verify that the new position is actually pathable
   //from all of the squads?  Age did, but maybe we can get away without doing that.
   BVector newPosition=cInvalidVector;
   if (!isPassable(averagePosition, getChildList()))
   {
      //Now find the closest squad to that position.
      BEntityID closestSquadID=cInvalidObjectID;
      float closestSquadDistance=0.0f;
      for (uint i=0; i < mChildren.getSize(); i++)
      {
         const BSquad* pSquad=gWorld->getSquad(mChildren[i]);
         BDEBUG_ASSERT(pSquad);
         if (pSquad)
         {
            float distance=pSquad->getPosition().xzDistanceSqr(averagePosition);
            if ((closestSquadID == cInvalidObjectID) || (distance < closestSquadDistance))
            {
               closestSquadID=pSquad->getID();
               closestSquadDistance=distance;
               newPosition=pSquad->getPosition();
            }
         }
      }
      BDEBUG_ASSERT(closestSquadID != cInvalidObjectID);
      #ifdef DEBUGGPH
      debug("  Using SquadID=%d Position for NewPosition=(%5.2f, %5.2f).", closestSquadID.asLong(), newPosition.x, newPosition.z);
      #endif
   }
   else
   {
      #ifdef DEBUGGPH
      debug("  Using AveragePosition for NewPosition.");
      #endif
      newPosition=averagePosition;
   }

   //If we don't have any waypoints, just set the starting position and be done.
   if (waypoints.getSize() == 0)
   {
      setPosition(newPosition);
      return;
   }
      
   //Find a path.
   //TODO: Figure the right obstruction radius to call this path with.
   //TODO: Figure out the right range.  Range is accessed through the target.
   BFindPathHelper pathFinder;
   target.invalidateRange();
   pathFinder.setTarget(&target);
   pathFinder.setEntity(this);
   pathFinder.setPathingRadius(getPathingRadius());
   pathFinder.setPathableAsFlyingUnit(isPathableAsFlyingUnit());
   pathFinder.setPathableAsFloodUnit(isPathableAsFloodUnit());
   pathFinder.setPathableAsScarabUnit(isPathableAsScarabUnit());
   pathFinder.enableReducePath(false);                      // Don't reduce the path yet.  That will be done when it's turned into pathing hints.
   pathFinder.enablePathAroundSquads(false);

   // Get target and range to pass to the pather since we want to path the platoon just within range
   long targetID = -1L;
   if (target.isIDValid() || target.isPositionValid())
   {
      targetID = target.getID();
      calculateRange(&(pathFinder.getTarget()));
   }
   /* DLM 11/10/08 - no longer supported
   if (gWorld->getPathingLimiter())
      gWorld->getPathingLimiter()->incNumPlatoonMoves();
   */
   uint result = pathFinder.findPath(pathToFirstWaypoint, newPosition, waypoints[0], false, targetID, true);

   //If we're in range at the start or we flat-out fail, we're done.
   if ((result == BPath::cFailed) || (pathToFirstWaypoint.getNumberWaypoints() < 1))
   {
      #ifdef DEBUGGPH
      debug("  Invalid path results, NOT setting a new forward.");
      #endif
      return;
   }
   #ifdef DEBUGGPH
   debug("  PathToFirstWaypoint: %d waypoints:", pathToFirstWaypoint.getNumberWaypoints());
   for (long i=0; i < pathToFirstWaypoint.getNumberWaypoints(); i++)
      debug("    WP[%d]: (%5.2f, %5.2f).", i, pathToFirstWaypoint.getWaypoint(i).x, pathToFirstWaypoint.getWaypoint(i).z);
   #endif
   #ifndef BUILD_FINAL
   /*
   long value = 0;
   gConfig.get(cConfigRenderSimDebug, &value);
   if (value == 2)
   {
   */
#ifndef _MOVE4
      mDebugCommandPath.zeroWaypoints();
      for (long i=0; i < pathToFirstWaypoint.getNumberWaypoints(); i++)
         mDebugCommandPath.addWaypointAtEnd(pathToFirstWaypoint.getWaypoint(i));
#endif
   //}
   #endif

   pathValid = true;


   //Find the initial direction.  Handle whether or not the starting position
   //is in the path.  If we don't have enough to path with, just set the position
   //and bail.
   BVector newForward;
   if (!newPosition.almostEqualXZ(pathToFirstWaypoint.getWaypoint(0)))
   {
      #ifdef DEBUGGPH
      debug("  Using WP[0] and newPosition for newForward.");
      #endif
      newForward=pathToFirstWaypoint.getWaypoint(0)-newPosition;
   }
   else if (pathToFirstWaypoint.getNumberWaypoints() > 1)
   {
      #ifdef DEBUGGPH
      debug("  Using WP[1] and newPosition for newForward.");
      #endif
      newForward=pathToFirstWaypoint.getWaypoint(1)-newPosition;
   }
   else
   {
      #ifdef DEBUGGPH
      debug("  Nothing to use for newForward, returning.");
      #endif
      setPosition(newPosition);
      return;
   }

   //Finish the forward.
   newForward.y=0.0f;
   newForward.safeNormalize();
   #ifdef DEBUGGPH
   debug("  NewForward=(%5.2f, %5.2f).", newForward.x, newForward.z);
   #endif

   //Now, set them.
   setPosition(newPosition);
   setForward(newForward);
   calcRight();
   calcUp();   

   //Now, update the formation.  This gives us the default formation.
   updateFormation();

   // DLM - After we've set the new forward for the formation, let's use it to "dynamically" calculate priorities
   // one time.  
   mpFormation->calculateDynamicPriorities(newForward);


   // DLM 2/26/08 - The below code was there to keep formations of large,
   // slow moving tanks from trying to move to thier formation position *before*
   // moving to their first waypoint, which would cause them to go backwards before going
   // forwards.  As we're using a mob formation now, this isn't necessary.  If we
   // go back to using a box or line formations, and we see this kind of behavior, 
   // consider restoring this code. 
   /**** start clip here

   //Try to "Forward" the new position and move it far enough ahead of
   //the formation so that units are told to walk forward "more".
   // MPB [10/13/2007] - Changed this to find the point on the path that's
   // "forwardAmount" from the start of the path.  It then crops out
   // the part of the path before this point.
#ifdef DEBUGGPH
   debug("  Try to \"Forward\" the new position and move it far enough ahead of");
   debug("  the formation so that units are told to walk forward \"more\".");
#endif
   float forwardAmount=mpFormation->getForwardAmount();
   BVector forwardPosition;
   long newNextWaypoint = 1;
   float realDist = 0.0f;
   pathToFirstWaypoint.calculatePointAlongPath(forwardAmount, pathToFirstWaypoint.getWaypoint(0), 1, forwardPosition, realDist, newNextWaypoint);
   #ifdef DEBUGGPH
   debug("  forwardAmount=%5.2f.", forwardAmount);
   debug("  forwardPosition=(%5.2f, %5.2f).", forwardPosition.x, forwardPosition.z);
   #endif
   #ifndef BUILD_FINAL
      mDebugForwardPosition = forwardPosition;
   #endif
   if (newNextWaypoint > 0)
   {
      setPosition(forwardPosition);
      #ifdef DEBUGGPH
      debug("    That's passable, using it and redoing the formation.");
      #endif

      // Crop the path to remove the part before the forwardPosition
      pathToFirstWaypoint.setWaypoint(newNextWaypoint - 1, forwardPosition);
      for (int i = 0; i < newNextWaypoint - 1; i++)
         pathToFirstWaypoint.removeWaypoint(0);

#ifdef DEBUGGPH
      debug("  After \"Forward Point\" Massaging, updated path is: %d waypoints:", pathToFirstWaypoint.getNumberWaypoints());
      for (long i=0; i < pathToFirstWaypoint.getNumberWaypoints(); i++)
         debug("    WP[%d]: (%5.2f, %5.2f).", i, pathToFirstWaypoint.getWaypoint(i).x, pathToFirstWaypoint.getWaypoint(i).z);
#endif

      //Now that we have our formation in the right spot, reassign the units.
      mpFormation->setReassignNeeded(true);
      updateFormation();
   }
   #ifdef DEBUGGPH
   else
      debug("    That's NOT passable, using existing newPosition.");
   #endif
   *** end clip here */

}

//==============================================================================
//==============================================================================
bool BPlatoon::tackOnWaypoints(const BCommand* pCommand)
{
   // Make sure the command has waypoints rather than an entity target
   if ((pCommand->getTargetID() != cInvalidObjectID) || (pCommand->getNumberWaypoints() <= 0))
      return false;

   // Scan the current orders to see if there is an active move order.  Also make sure there
   // are no other orders this command should be queued behind.
   BSimOrderEntry* pMoveOrderEntry = NULL;
   uint i;
   for (i = 0; i < mOrders.getSize(); i++)
   {
      if ((mOrders[i].getType() == BSimOrder::cTypeMove) && (mOrders[i].getState() != BSimOrderEntry::cStateQueued))
      {
         // Confirm target doesn't have entity or an ability
         if (!mOrders[i].getOrder()->getTarget().isIDValid() && (mOrders[i].getOrder()->getWaypointNumber() > 0) && mOrders[i].getOrder()->getTarget().getAbilityID() == -1)
            pMoveOrderEntry = &(mOrders[i]);
      }

      // Must queue command behind any other queued orders
      if (mOrders[i].getState() == BSimOrderEntry::cStateQueued)
         return false;
   }

   // Move order not found
   if (pMoveOrderEntry == NULL)
      return false;

   //pointer to the waypoint list (so we can redirect it if we need to for the warthog jump)
   const BDynamicSimVectorArray *pWaypointArray = &pCommand->getWaypointList();

   //For warthog jumping, make sure the last waypoint is not in a user pathable area, if it is, reposition it.
   BDynamicSimVectorArray arraycopy;                                                  //copy has to be in this scope whether we use it or not.
   BVector lastWaypoint = pCommand->getWaypoint(pCommand->getNumberWaypoints() - 1);  //Check the last waypoint
   if ( repositionTargetLocationIfInsidePlayerObstruction(lastWaypoint) )
   {
      //This is a warthog platoon and we need to modify that last waypoint.
      //Since the pCommand is const, a copy of the waypoint list is made and then modified.
      arraycopy = pCommand->getWaypointList();
      arraycopy.get(pCommand->getNumberWaypoints() - 1).set(lastWaypoint);

      //The pWapointArray pointer then is reassigned to this new modified waypoint list.
      pWaypointArray = &arraycopy;
   }

   BSimOrder* pMoveOrder = pMoveOrderEntry->getOrder();

   // Add waypoints to existing order
   int wpIndex;
   for (wpIndex = 0; wpIndex < pWaypointArray->getNumber(); wpIndex++)
   {
      const_cast<BDynamicSimVectorArray&>(pMoveOrder->getWaypoints()).add(pWaypointArray->get(wpIndex));
   }

   // Update target for order
   BSimTarget tempTarget = pMoveOrder->getTarget();
   tempTarget.setPosition(pWaypointArray->get(pWaypointArray->getNumber() - 1));
   pMoveOrder->setTarget(tempTarget);

   // Generate new waypoints and add to platoon's list of waypoints for the squad to use
   if (!generatePathingHints(*pWaypointArray, pMoveOrder, true))
      return false;

   return true;
}

#ifdef _MOVE4
//==============================================================================
// tackOnWaypoints - If a new move order has come in, and we already have an
// existing move order in progress, (and it's the last order in the queue)
// we don't queue the new move order.  Instead, we just grab the existing move 
// order and tack the waypoints on to that move order.  Also note that in the 
// current implementation, no single command should ever have more than a single 
// waypoint.
// return code:  
// true - waypoints copied, order discarded.
// false - we can't just tack on the waypoints, so we'll have to actually queue
// this order. 
//==============================================================================
bool BPlatoon::tackOnWaypoints_4(const BCommand* pCommand)
{
   // Make sure the command has waypoints rather than an entity target
   if ((pCommand->getTargetID() != cInvalidObjectID) || (pCommand->getNumberWaypoints() <= 0))
      return false;

   // Scan the current orders to see if there is an active move order.  Also make sure there
   // are no other orders this command should be queued behind.
   uint i;

   BSimOrder* pMoveOrder = NULL;

   for (i = 0; i < mOrders.getSize(); i++)
   {
      BSimOrder * pTempOrder = mOrders[i].getOrder();

      // Must queue command behind any other queued orders
      if (mOrders[i].getState() == BSimOrderEntry::cStateQueued)
         return false;

      if ((mOrders[i].getType() == BSimOrder::cTypeMove))
      {
         // Confirm target doesn't have entity or an ability
         if (!pTempOrder->getTarget().isIDValid() && (pTempOrder->getWaypointNumber() > 0) && pTempOrder->getTarget().getAbilityID() == -1)
            pMoveOrder = pTempOrder;
      }

   }

   // Move order not found
   if (pMoveOrder == NULL)
      return false;

   //pointer to the waypoint list (so we can redirect it if we need to for the warthog jump)
   const BDynamicSimVectorArray *pWaypointArray = &pCommand->getWaypointList();

   //For warthog jumping, make sure the last waypoint is not in a user pathable area, if it is, reposition it.
   BDynamicSimVectorArray arraycopy;                                                  //copy has to be in this scope whether we use it or not.
   BVector lastWaypoint = pCommand->getWaypoint(pCommand->getNumberWaypoints() - 1);  //Check the last waypoint
   if ( repositionTargetLocationIfInsidePlayerObstruction(lastWaypoint) )
   {
      //This is a warthog platoon and we need to modify that last waypoint.
      //Since the pCommand is const, a copy of the waypoint list is made and then modified.
      arraycopy = pCommand->getWaypointList();
      arraycopy.get(pCommand->getNumberWaypoints() - 1).set(lastWaypoint);

      //The pWapointArray pointer then is reassigned to this new modified waypoint list.
      pWaypointArray = &arraycopy;
   }


   // Add waypoints to existing order, and place the waypoints in our user path.  
   int wpIndex;
   for (wpIndex = 0; wpIndex < pWaypointArray->getNumber(); wpIndex++)
   {
      const_cast<BDynamicSimVectorArray&>(pMoveOrder->getWaypoints()).add(pWaypointArray->get(wpIndex));
      mCurrentUserPath_4.addWaypointAtEnd(pWaypointArray->get(wpIndex));
      mDebugCommandPath.addWaypointAtEnd(pWaypointArray->get(wpIndex));
   }

   // Update target for order
   BSimTarget tempTarget = pMoveOrder->getTarget();
   tempTarget.setPosition(pWaypointArray->get(pWaypointArray->getNumber() - 1));
   pMoveOrder->setTarget(tempTarget);

   return true;
}
#endif


//==============================================================================
//==============================================================================
DWORD BPlatoon::getMoveActionStartTime(BSquad* pSquad) const
{
   BASSERT(pSquad);

   // Get the last time the move action's user level waypoints changed.  This is probably a good
   // approximation for now when the squad was commanded to move.  Do we need to do anything special
   // if the move action doesn't exist?  Should we check when sim orders are executed instead?
   DWORD moveTime = 0;
   const BSquadActionMove* pMoveAction=reinterpret_cast <BSquadActionMove*> (pSquad->getActionByType(BAction::cActionTypeSquadMove));
   if (pMoveAction != NULL)
      moveTime = pMoveAction->getUserLevelWaypointUpdateTime();
   return moveTime;
}

//==============================================================================
//==============================================================================
void BPlatoon::notifySound(const BSimOrder* pOrder)
{
   if(!pOrder)
      return;

   BEntityID targetID = pOrder->getTarget().getID();
   
   gWorld->getWorldSoundManager()->getMusicManager()->squadsCommandedToAttack(mChildren, getPlayerID(), mPosition, pOrder->getTarget().getID());
}


//==============================================================================
//BPlatoon::repositionTargetLocationIfInsidePlayerObstruction
//Special function for warthog jumping 
//==============================================================================
bool BPlatoon::repositionTargetLocationIfInsidePlayerObstruction(BVector &targetPosition)
{
   //We need the DesignObjectManager, and it should have some lines in it. (quickest test, so it's first)
   BDesignObjectManager* pDOM=gWorld->getDesignObjectManager();
   if (pDOM && (pDOM->getDesignLineCount() > 0))                
   {
      //make sure this is a platoon of jumpers
      if( canJump() )                                           
      {
         //Regular ground type obstructions
         long lObOptions=
            BObstructionManager::cIsNewTypeBlockLandUnits |
            BObstructionManager::cIsNewTypeAllCollidableUnits |
            BObstructionManager::cIsNewTypeAllCollidableSquads;
         long lObNodeType = BObstructionManager::cObsNodeTypeAll;

         //Get the obstructions that this point is within
         BObstructionNodePtrArray obsList;
         gObsManager.findObstructionsOnPoint(targetPosition, 0.0f, lObOptions, lObNodeType, getPlayerID(), false, obsList);

         //Check each of the obstructions looking for a player obstruction node
         for (uint obsIndex=0; obsIndex < obsList.getSize(); obsIndex++)
         {
            //If we have a playerID, we are a player obstruction
            BOPObstructionNode *obsnode = obsList.get(obsIndex);
            if( obsnode && (obsnode->mPlayerID > 0) )
            {
               //gConsoleOutput.debug("Placing warthog waypoint in player obstruction:");
               //gConsoleOutput.debug("      target point: (%f, %f, %f)", targetPosition.x, targetPosition.y, targetPosition.z);
               //gConsoleOutput.debug("      obst. points: (%f, %f) (%f, %f) (%f, %f) (%f, %f)", obsnode->mX1, obsnode->mZ1, obsnode->mX2, obsnode->mZ2, obsnode->mX3, obsnode->mZ3, obsnode->mX4, obsnode->mZ4 );

               //Go through the design lines, looking for ones designated as jump landing lines (currently mValue==999 and mValues[1]==2)  or oneway jump lines
               float minDistance = 999999.0f;
               BVector minLocation;
               uint lineID = 0;
               uint lineSubID = 0;
               for (uint i=0; i < pDOM->getDesignLineCount(); i++)
               {
                  //Get the line.  Ignore non-physics lines.
                  BDesignLine& line=pDOM->getDesignLine(i);
                  if ((!line.mDesignData.isPhysicsLine() || (line.mDesignData.mValues[1] != 2)) && !line.mDesignData.isOneWayBarrierLine()) //1 = designation for a jump line, 2 = land line
                     continue;

                  //Find the closest point on the landing lines.  These design lines can have multiple segments, so test each one, along with the mid point.
                  float dist = 0.0f;
                  long j=0;
                  for (j=0; j<(line.mPoints.getNumber()-1); j++ )
                  {
                     //test first point and center point (of the segment) for the distance to targetposition
                     BVector firstPoint = line.mPoints.get(j);
                     BVector centerPoint = firstPoint + line.mPoints.get(j+1);
                     centerPoint.scale( 0.5f );

                     dist = targetPosition.distanceEstimate( firstPoint );
                     if( dist < minDistance )
                     {
                        minDistance = dist;
                        minLocation = firstPoint;
                        lineID = i;
                        lineSubID = j;
                     }
                     dist = targetPosition.distanceEstimate( centerPoint );
                     if( dist < minDistance )
                     {
                        minDistance = dist;
                        minLocation = centerPoint;
                        lineID = i;
                        lineSubID = j;
                     }
                  }
                  //test the last point
                  dist = targetPosition.distanceEstimate( line.mPoints.get(j) );
                  if( dist < minDistance )
                  {
                     minDistance = dist;
                     minLocation = line.mPoints.get(j);
                     lineID = i;
                     lineSubID = j-1;
                  }
               } //end for design lines

              //change the target location!
              if (pDOM->getDesignLine(lineID).mDesignData.isOneWayBarrierLine())               //move out a little if this is a barrier line
              {
                  BDesignLine& line=pDOM->getDesignLine(lineID);
                  BVector orientDirection;
                  orientDirection = line.mPoints.get(lineSubID+1) - line.mPoints.get(lineSubID);
                  orientDirection.y = 0.0f;
                  orientDirection.safeNormalize();
                  orientDirection.assignCrossProduct(orientDirection, cYAxisVector);
                  orientDirection.scale( getBoundingRadius() );

                  targetPosition = minLocation + orientDirection;
              }
              else
              {
                 targetPosition = minLocation;
              }
              //gConsoleOutput.debug("      closest line point: (%f, %f, %f)", minLocation.x, minLocation.y, minLocation.z );
              return true;

            } //end obsnode
         } //end objsize
      }  //end canJump
   } //end pDom

   return false;
}


//==============================================================================
//BPlatoon::repositionWaypointIfOneWayBarrierIsCrossed
//Special function for warthog jumping 
//==============================================================================
bool BPlatoon::repositionWaypointIfOneWayBarrierIsCrossed(BPath &path)
{
   if( !canJump() )
      return false;

   //Check path for intersections with lines
   BDesignObjectManager* pDOM=gWorld->getDesignObjectManager();
   if (pDOM && (pDOM->getDesignLineCount() > 0))
   {
      for( int j=0; j<path.getNumberWaypoints()-1; j++ )
      {
         BVector pointA = path.getWaypoint(j);
         BVector pointB = path.getWaypoint(j+1);
         BVector forward = pointB - pointA;
         //gConsoleOutput.debug("waypoint position %d (%f, %f, %f)   %d (%f, %f, %f)", j, pointA.x, pointA.y, pointA.z, j+1, pointB.x, pointB.y, pointB.z );

         //gConsoleOutput.debug("Platoon position (%f, %f, %f)", getPosition().x, getPosition().y, getPosition().z );
         for (uint i=0; i < pDOM->getDesignLineCount(); i++)
         {
            //Get the line.  Ignore non-physics lines.
            BDesignLine& line=pDOM->getDesignLine(i);
            if (line.mDesignData.isOneWayBarrierLine())
            {
               //See if we intersect.
               BVector intersectionPoint;
               if (line.imbeddedIncidenceIntersects(forward, pointA, pointB, intersectionPoint))
               {
                  float platoonRadius = getBoundingRadius();
                  //gConsoleOutput.debug("Crossed Line: %d  with Waypoint: %d / %d\n", i, j, path.getNumberWaypoints());
                  //gConsoleOutput.debug("Truncating");

                  //move the newpoint off the line (so repeated clicks don't put us past it)
                  BVector newpoint = forward;
                  newpoint.safeNormalize();
                  newpoint.scale(-platoonRadius);
                  newpoint.assignSum( newpoint, intersectionPoint );

                  //Now change the waypoint and get rid of any further ones.
                  path.setWaypoint( j+1, newpoint );
                  long numwaypoints = path.getNumberWaypoints();
                  for( int k=numwaypoints-1; k>j+1; k-- )
                     path.removeWaypoint(k);
                  return true;
               }
            }
         }
      }
   }
   return false;
}

//==============================================================================
//==============================================================================
float BPlatoon::getBoundingRadius()
{
   float platoonRadius = 0.0f;
   BVector platoonPosition = getPosition();

   for( uint i=0; i<getNumberChildren(); i++ )
   {
      BEntityID eid = getChild(i);
      const BEntity* pChildEntity = gWorld->getEntity(eid);
      if( !pChildEntity )
         continue;
      const BVector &position = pChildEntity->getPosition();
      float radius = pChildEntity->getObstructionRadius();

      float maxDistFromPlatoonPos = position.distance(platoonPosition) + radius;
      if( maxDistFromPlatoonPos > platoonRadius )
         platoonRadius = maxDistFromPlatoonPos;
   }
   return platoonRadius;
}


#ifdef _MOVE4
//==============================================================================
// executeOrder
//==============================================================================
void BPlatoon::executeOrder_4(BSimOrderEntry* pOrderEntry)
{
   SCOPEDSAMPLE(BPlatoon_executeOrder);
   
   debugMove4("executeOrder_4: %s", pOrderEntry->getTypeName());
   
   removeOrders(true, false, true);
   //stopChildren();
   // Reset SquadToRelocate.. 
   mSquadToRelocate = 0;

   // Reset Plots
   mSquadPlots_4.resize(0);
   mValidPlots_4 = false;

   #ifdef DEBUG_MOVE4
   if (mChildren.getNumber() == 0)
      debugMove4("executeOrder_4 -- Platoon with No Children receiving order.");
   #endif

   //Turn it to executing.
   pOrderEntry->setState(BSimOrderEntry::cStateExecute);

   BSimOrder* pOrder = pOrderEntry->getOrder();
   BASSERT(pOrder);
   if (pOrder == NULL)
      return;

   // mrh/dlm 6/20/08 - Moved this up above getTargetPosFromOrder()
   // Assign the platoon's current target
   mCurrentTarget_4 = pOrder->getTarget();

   // Generate the move data for the platoon
   BVector targetPosition;
   if (!getTargetPosFromOrder(pOrder, targetPosition))
   {
      debugMove4("executeOrder_4: could not get target position from order");
      return;
   }
   // mrh/dlm/trb 6/20/08 - The assumption that a valid target ID == a valid target position was not true.
   // Update the target position with a valid position, to ensure if the ID was valid but the position was not, we aren't screwed.
   else if (!mCurrentTarget_4.isPositionValid())
   {
      mCurrentTarget_4.setPosition(targetPosition);
   }

   // If the target has an ID, then calculate a valid range to go with it..
   if (mCurrentTarget_4.isIDValid() || mCurrentTarget_4.isPositionValid())
   {
      getPlatoonRange_4(mCurrentTarget_4);
      // At the same time, go through our squads and reset their ranges.. 
      for (uint i=0; i < mChildren.getSize(); i++)
      {
         BSquad *squad=gWorld->getSquad(mChildren[i]);
         if (!squad)
            continue;
         mSquadRanges[i] = -1.0f;
         if (mCurrentTarget_4.isIDValid())
            squad->calculateRange(&mCurrentTarget_4, mSquadRanges[i], NULL, NULL);
      }
   }

   //If this is a platoon of warthogs that will use jump pathing, check and see if the waypoint is in a jump obstruction.  If so, move it out.
   repositionTargetLocationIfInsidePlayerObstruction( targetPosition );

   // Reset the platoon's position to the average of the children, before we run paths.  
   BVector averagePosition=cOriginVector;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad *squad=gWorld->getSquad(mChildren[i]);
      if (!squad)
         continue;
      averagePosition+=squad->getPosition();

      if (!mSlowestSquadVelocityValid ||
         (mSlowestSquadVelocity > squad->getDesiredVelocity()))
      {
         mSlowestSquadVelocity=squad->getDesiredVelocity();
         mSlowestSquadVelocityValid=true;
         mSlowestSquadID=squad->getID();
      }
   }
   averagePosition/=static_cast<float>(mChildren.getSize());
   
   // jce [9/4/2008] -- Find the closest unit to the average position and use HIS position for the platoon.
   // Otherwise the platoon's position can be placed in an obstructed location, causing badness.
   BVector closestPos = averagePosition;        // As a precaution, assume average position here though it should be overwritten
   float closestDistSqr = cMaximumFloat;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      // Get squad.
      BSquad *squad=gWorld->getSquad(mChildren[i]);
      if (!squad)
         continue;
         
      // Get distance to average position.
      BVector pos = squad->getPosition();
      float distSqr = averagePosition.xzDistanceSqr(pos);
      if(distSqr < closestDistSqr)
      {
         // Remember this position as closest so far.
         closestPos = pos;
         closestDistSqr = distSqr;
      }
   }
   closestPos.y = 0.0f;
   
   // Set the position.
   setPosition(closestPos);
   debugMove4("Setting platoon position to %0.2f, %0.2f", mPosition.x, mPosition.z);

   
   // jce [8/27/2008] -- Force formation to re-make.
   if(mpFormation)
      mpFormation->setMakeNeeded(true); 

   // Reset all of the paths..
   mCurrentUserPath_4.reset();
   mCurrentHLPath_4.reset();
   mCurrentLLPath_4.reset();

   // Before we execute the order, see if any of our children are playing a blocking animation.  If so, then belay this order.
   // perpetration of the brutal playblockinganimation hack.  Also make sure it isn't executing an uninterruptible action.
   for (uint n = 0; n < (uint)mChildren.getNumber(); n++)
   {
      BEntityID childID = mChildren[n];
      BSquad *pSquad = reinterpret_cast<BSquad*>(gWorld->getEntity(childID));
      if (pSquad)
      {
         // Playing blocking anim
         if (pSquad->getFlagPlayingBlockingAnimation())
         {
            pOrderEntry->setState(BSimOrderEntry::cStateQueued);
            debugMove4("Squad %d is in blocking anim, bailing", pSquad->getID());
            return;
         }

         // Uninterruptible action
         uint numActions = pSquad->getNumberActions();
         for (uint actionIndex = 0; actionIndex < numActions; actionIndex++)
         {
            const BAction* pAction = pSquad->getActionByIndexConst(actionIndex);
            // Halwes - 9/29/2008 - Special case where we want to interrupt a the non-interruptible transport action.
            if (pAction && !pAction->isInterruptible() && (pAction->getType() != BAction::cActionTypeSquadTransport))
            {
               pOrderEntry->setState(BSimOrderEntry::cStateQueued);
               debugMove4("Squad %d has uninterruptable action, bailing", pSquad->getID());
               return;
            }
         }
      }
   }

   // From the order, copy the waypoints to our user WP list. 
   int32 nIndex = 0;
   const BDynamicSimVectorArray &orderWaypoints = pOrder->getWaypoints();
   // A valid path must always have two waypoints.. so we always stuff our current position
   // into the first waypoint when we execute an order.
   mCurrentUserPath_4.addWaypointAtEnd(mPosition);
   syncPlatoonData("mPosition", mPosition);

   for (nIndex = 0; nIndex < orderWaypoints.getNumber(); nIndex++)
   {
      mCurrentUserPath_4.addWaypointAtEnd(orderWaypoints[nIndex]);
      syncPlatoonData("Order waypoint -- ", orderWaypoints[nIndex]);
   }
   // If we did not have an order, or if there were no waypoints in the order, then see if we have a target position.
   if (!pOrder || orderWaypoints.getNumber() == 0)
   {
      // If we have a target position, then we should add that as well.
      debugMove4("executeOrder_4: no order, or order had no waypoints.. looking for a target.");
      if (mCurrentTarget_4.isPositionValid())
         mCurrentUserPath_4.addWaypointAtEnd(mCurrentTarget_4.getPosition());
   }

   // If by this point, we have an order, but we only have one waypoint, don't move at all.  But send the order
   // on down to our children, whom might want to take actions -- like garrisoning. 
   if (pOrder && mCurrentUserPath_4.getNumberWaypoints() < 2)
   {
      debugMove4("executeOrder_4: We had an order, but only 1 (or fewer) waypoints, so just passing the order on to our children.");
      // Notify sound system that an order was issued.
      notifySound(pOrder);
      // Send the order to the squads
      queueOrderToChildren(pOrder);
      return;
   }

   // Reset our current Waypoints.  Current UserWP is 1, cause we're progressing
   // to waypoint one from waypoint zero (our start).  
   // Current HLWP and LLWP are 0, as we've not yet run those paths.
   mCurrentUserWP_4 = 1;
   mCurrentHLWP_4 = 0;
   mCurrentHLPath_4.reset();
   mCurrentLLWP_4 = 0;
   mCurrentLLPath_4.reset();
   int32 pathResult = BPath::cFailed;

   // If we can't run the path, then pathing limiter stopped us - return this order to queued.  
   // If our path fails, or we're in range to begin with, then we're done already. 
   bool bPathRun = pathToNextUserWP_4(pathResult);
   if (!bPathRun)
   {
      debugMove4("executeOrder_4: could not run a path, going into queued state");
      pOrderEntry->setState(BSimOrderEntry::cStateQueued);
      return;
   }

   // DLM 5/16/08 - If we fail, then queue it back up.  For inRangeAtStart, allow it to pass through and to execute the
   // squad orders, because lockdown-in-place actually uses this code path.  Yeah.. I know. 
   if (pathResult == BPath::cFailed /* || pathResult == BPath::cInRangeAtStart*/)
   {
      pOrderEntry->setState(BSimOrderEntry::cStateQueued);
      debugMove4("executeOrder_4: Going back to queued state, path %s", (pathResult==BPath::cFailed)?"failed":"in range at start");
      return;
   }

   // We successfully got a HLP, so all we have to do is make advance the WP, to start moving towards it.
   mCurrentHLWP_4 = 1;

   // Update Formation for squads.. 
   updateFormation();

   // Start moving towards whatever it is if we're not an action type that doesn't want that
   // (Yes, this is completely hacktastic :( -CJS)
   /*if (pOrderEntry->getType() != BSimOrder::cTypeJump && 
       pOrderEntry->getType() != BSimOrder::cTypeJumpGather && 
       pOrderEntry->getType() != BSimOrder::cTypeJumpGarrison && 
       pOrderEntry->getType() != BSimOrder::cTypeJumpAttack &&
       pOrderEntry->getType() != BSimOrder::cTypeJumpPull)*/
   {
      BActionID moveAction = doMove_4(pOrder, NULL, &mCurrentTarget_4);
      if (moveAction == cInvalidActionID)
      {
         debugMove4("executeOrder_4: failed to create move action");
         return;
      }
   }
      
   // Notify sound system that an order was issued.
   notifySound(pOrder);

   // Send the order to the squads
   queueOrderToChildren(pOrder);

#ifndef BUILD_FINAL
   // Trigger timings
   if (pOrder->getPriority() == BSimOrder::cPriorityTrigger)
   {
      setFlagIsTriggered(true);
   }
   else
   {
      setFlagIsTriggered(false);
   }
#endif
}

//==============================================================================
// repathToTarget
//==============================================================================
bool BPlatoon::repathToTarget_4(int32 &pathResult)
{
   // This is called when the platoonActionMove detects the target has moved
   // "substantially" from it's original location.  We re-stuff the target
   // position into the last user waypoint, and if we're on the segment
   // that leads to that waypoint, we repath. 
   // Theoretically we shouldn't need to do target checks, but better safe than
   // sorry. 
   syncPlatoonData("repathToTarget, mCurrentTarget4", mCurrentTarget_4.getID());
   if (mCurrentTarget_4.isIDValid())
   {
      BEntity *pEntity = gWorld->getEntity(mCurrentTarget_4.getID());
      if (pEntity)
      {
         syncPlatoonData("mCurrentUserPath num waypoints", mCurrentUserPath_4.getNumberWaypoints());
         syncPlatoonData("mCurrentUserWP_4", mCurrentUserWP_4);
         
         long nLastWPIndex = mCurrentUserPath_4.getNumberWaypoints() - 1;
         mCurrentUserPath_4.setWaypoint(nLastWPIndex, pEntity->getPosition());
         // IF we're moving towards that waypoint.. let's REPATH!!
         if (mCurrentUserWP_4 == nLastWPIndex)
         {
            syncPlatoonCode("repathing");

            mCurrentHLWP_4 = 0;
            mCurrentHLPath_4.reset();
            mCurrentLLWP_4 = 0;
            mCurrentLLPath_4.reset();

            // If we can't run the path, then pathing limiter stopped us - return this order to queued.  
            // If our path fails, or we're in range to begin with, then we're done already. 
            bool bPathRun = pathToNextUserWP_4(pathResult);
            // If we're good to do, then set our current HLWP marker.. 
            if (bPathRun && (pathResult != BPath::cFailed && BPath::cInRangeAtStart))
            {
               syncPlatoonCode("using new path");
               mCurrentHLWP_4 = 1;
            }
            return bPathRun;
         }
      }
   }
   return false;
}


//==============================================================================
// pathToNextUserWP
//==============================================================================
bool BPlatoon::pathToNextUserWP_4(int32 &pathResult)
{
   // jce [4/16/2008] -- Path is invalid til proven otherwise, so reset to 0th waypoint.
   mCurrentHLWP_4 = 0;
   
   // Use the pathing limiter.. wtf! 
   BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
   if (!pathLimiter)
      return false;

   bool bAllowed = pathLimiter->requestPlatoonLRP(mPlayerID);
   if (!bAllowed)
   {
      debugMove4("pathToNextUserWP_4: pathing limiter prevented us from pathing");
      return false;
   }

   // Sanity check on the currentWP.  
   if (mCurrentUserWP_4 < 1 || mCurrentUserWP_4 > mCurrentUserPath_4.getNumberWaypoints() - 1)
   {
      debugMove4("pathToNextUserWP_4: bailing since current user waypoint is bogus");
      return false;
   }

   // Run the path..

   // Set up the class..
   int pathClass = BPather::cSquadLandPath;
   if (isPathableAsFloodUnit())
      pathClass = BPather::cFloodPath;
   else if (isPathableAsScarabUnit())
      pathClass = BPather::cScarabPath;
   else if (isPathableAsFlyingUnit())
      pathClass = BPather::cAirPath;

   // If we're pathing to the last waypoint in our user list, see if we also have a target.  If so, we'll
   // want to accomodate it in our pathing attempt
   long targetID = -1;
   if (mCurrentUserWP_4 == (long)mCurrentUserPath_4.getNumberWaypoints() - 1)
   {
      targetID = (long)mCurrentTarget_4.getID();
      syncPlatoonData("using target", targetID);
   }

   // If we have a target range, use that
   float fRange = 0;
   if (mCurrentTarget_4.isRangeValid())
   {
      fRange = mCurrentTarget_4.getRange();
      debugMove4("pathToNextUserWP_4: pathing with target ID: %d, range: %0.2f", targetID, fRange);
   }
   else
   {
      debugMove4("pathToNextUserWP_4: pathing without target ID");
   }

   // And the radius.. 
   float fRadius = getPathingRadius();

   // Figure out canJump
   bool jumping = canJump();

   // Clear the ignore list.
   mIgnoreList_4.clear();

   // and Path
   BVector goal = mCurrentUserPath_4.getWaypoint(mCurrentUserWP_4);
   pathResult = gPather.findPath(getID(), // EntityID
                                 BEntity::cClassTypePlatoon,         // EntityType
                                 &gObsManager,                       // ObstructionManager
                                 mPosition,                          // jce [4/15/2008] -- start from current position so as not to go backwards towards and old waypoint
                                 //mCurrentUserPath_4.getWaypoint(mCurrentUserWP_4-1), // Start Point
                                 goal,   // Goal
                                 fRadius,                            // Radius
                                 fRange,                             // Range
                                 mIgnoreList_4,                      // Ignore List
                                 &mCurrentHLPath_4,                  // Path to be filled in
                                 false,                              // Skip BeginPathing
                                 true,                               // Full Path Only
                                 jumping,                            // Can Unit Jump?
                                 targetID,                           // Target ID if we have one
                                 BPather::cLongRange,                // Path Type
                                 pathClass);                         // Pathing Class

   debugMove4("pathToNextUserWP_4: LRP from (%0.2f, %0.2f, %0.2f) to (%0.2f, %0.2f, %0.2f)", mPosition.x, mPosition.y, mPosition.z, goal.x, goal.y, goal.z);
   debugMove4("   pathResult=%s, %d waypoints", BPath::getPathResultName(pathResult), mCurrentHLPath_4.getNumberWaypoints());
      
   repositionWaypointIfOneWayBarrierIsCrossed(mCurrentHLPath_4);
   
   BASSERT(pathResult == BPath::cFailed || mCurrentHLPath_4.getNumberWaypoints() > 1);

   // Save the creation time.
   mCurrentHLPath_4.setCreationTime(gWorld->getGametime());

   // And that's pretty much it
   return true;
}

//==============================================================================
// pathToNextHLWP
// You'll find this routine remarkably similar to the one above it, cept
// we're running a LLP, rather than a HLP
//==============================================================================
bool BPlatoon::pathToNextHLWP_4(int32 &pathResult)
{
   // Use the pathing limiter.. wtf! 
   BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
   if (!pathLimiter)
      return false;

   bool bAllowed = pathLimiter->requestPlatoonSRP(mPlayerID);
   if (!bAllowed)
   {
      debugMove4("pathToNextHLWP_4: pathing limiter prevented us from pathing");
      return false;
   }

   // Sanity check on the currentWP.  
   if (mCurrentHLWP_4 < 1 || mCurrentHLWP_4 > mCurrentHLPath_4.getNumberWaypoints() - 1)
   {
      debugMove4("pathToNextHLWP_4: bailing since current user waypoint is bogus");
      
      // jce [10/9/2008] -- Not sure yet why this ever happens, but I'm adding code here to force a repath in this case
      // since we've seen it happen.  If we don't reset the path here, the platoon will endlessly loop not doing anything.
      mCurrentHLWP_4 = 0;
      mCurrentHLPath_4.reset();
      mCurrentLLWP_4 = 0;
      mCurrentLLPath_4.reset();
      
      return false;
   }

   // Run the path..

   // Set up the class..
   int pathClass = BPather::cSquadLandPath;
   if (isPathableAsFloodUnit())
      pathClass = BPather::cFloodPath;
   else if (isPathableAsScarabUnit())
      pathClass = BPather::cScarabPath;
   else if (isPathableAsFlyingUnit())
      pathClass = BPather::cAirPath;

   // Note -- we only use the target if we're pathing to the last HLWP AND the last UserWP
   long targetID = -1;
   if (mCurrentUserWP_4 == (long)mCurrentUserPath_4.getNumberWaypoints() - 1 && mCurrentHLWP_4 == (long)mCurrentHLPath_4.getNumberWaypoints() - 1)
   {
      targetID = (long)mCurrentTarget_4.getID();
      syncPlatoonData("using target", targetID);
   }

   // If we have a range in our target value, use that.
   float fRange = 0;
   if (mCurrentTarget_4.isRangeValid())
   {
      fRange = mCurrentTarget_4.getRange();
      debugMove4("pathToNextHLWP_4: pathing with target ID: %d, range: %0.2f", targetID, fRange);
   }
   else
   {
      debugMove4("pathToNextHLWP_4: pathing without target ID");
   }


   // And the radius.. 
   float fRadius = getPathingRadius();

   // Figure out canJump
   bool jumping = canJump();

   // Build an ignore list.
   buildIgnoreList_4();

   // and Path   
   BVector goal = mCurrentHLPath_4.getWaypoint(mCurrentHLWP_4);
   pathResult = gPather.findPath(getID(), // EntityID
                                 BEntity::cClassTypePlatoon,         // EntityType
                                 &gObsManager,        // Pointer to obstruction mgr
                                 mPosition,                          // jce [4/15/2008] -- start from current position so as not to go backwards towards and old waypoint
                                 //mCurrentHLPath_4.getWaypoint(mCurrentHLWP_4-1), // Start Point
                                 goal,   // Goal
                                 fRadius,                            // Radius
                                 fRange,                             // Range
                                 mIgnoreList_4,                      // Ignore List
                                 &mCurrentLLPath_4,                  // Path to be filled in
                                 false,                              // Skip BeginPathing
                                 false,                              // Full Path Only
                                 jumping,                            // Can Unit Jump?
                                 targetID,                           // Target ID if we have one
                                 BPather::cShortRange,               // Path Type
                                 pathClass);                         // Pathing Class

   debugMove4("pathToNextHLWP_4: LLP from (%0.2f, %0.2f, %0.2f) to (%0.2f, %0.2f, %0.2f)", mPosition.x, mPosition.y, mPosition.z, goal.x, goal.y, goal.z);
   debugMove4("   pathResult=%s, %d waypoints", BPath::getPathResultName(pathResult), mCurrentLLPath_4.getNumberWaypoints());

   // Save the creation time.
   mCurrentLLPath_4.setCreationTime(gWorld->getGametime());
    
   // And that's still pretty much it
   return true;
}

//==============================================================================
// doMove
// Creates our move action
//==============================================================================
BActionID BPlatoon::doMove_4(BSimOrder* pOrder, BAction* pParentAction, BSimTarget *pTarget)
{
   syncPlatoonData("doMove_4 for platoon", mID);
   syncPlatoonData("   orderID", (DWORD)pOrder->getID());

   BPlatoonActionMove* pAction=reinterpret_cast<BPlatoonActionMove*>(gActionManager.createAction(BAction::cActionTypePlatoonMove));
   pAction->setParentAction(pParentAction);

   // If we pass in a target, it has priority.
   // If we didn't pass in a target, and we have an order, and it has a target, then use it.
   // There's really no opportunity for confusion here, really. 
   if (pTarget)
   {
      pAction->setTarget(*pTarget);
      syncPlatoonData("passed in target", pTarget->getID());
   }
   else if (pOrder)
   {
      pAction->setTarget(pOrder->getTarget());
      syncPlatoonData("action target", (DWORD)pAction->getID());
   }

   if (!addAction(pAction, pOrder))
      return (cInvalidActionID);

   return (pAction->getID());
}


//==============================================================================
//==============================================================================
void BPlatoon::buildIgnoreList_4()
{
   // The LLP should ignore all squad and unit obstructions in my platoon.
   mIgnoreList_4.resize(0);

   // If I have a target, put it on the ignore list.
   if (mCurrentTarget_4.isIDValid())
   {
      mIgnoreList_4.add(mCurrentTarget_4.getID());
      // If the thing is a squad, we need to ignore it's children, really.  (as well as the squad)
      BEntity *pEntity = gWorld->getEntity(mCurrentTarget_4.getID());
      if (pEntity && pEntity->getClassType() == BEntity::cClassTypeSquad)
      {
         const BSquad* pTargetSquad=reinterpret_cast<BSquad*>(pEntity);
         const BEntityIDArray &myUnitList = pTargetSquad->getChildList();
         mIgnoreList_4.add(myUnitList.getPtr(), myUnitList.size());
      }
      // Conversely, if this is a unit, please add it's squad ID to the list as well..
      if (pEntity && (pEntity->getClassType() == BEntity::cClassTypeUnit) && pEntity->getParent())
      {
         BSquad *pTargetSquad = reinterpret_cast<BSquad*>(pEntity->getParent());
         mIgnoreList_4.add(pTargetSquad->getID());
      }
   }

   for (uint n = 0; n < getNumberChildren(); n++)
   {
      const BSquad* pSquad = gWorld->getSquad(getChild(n));
      if (pSquad != NULL)
      {
         mIgnoreList_4.add(pSquad->getID());
         for (uint u = 0; u < pSquad->getNumberChildren(); u++)
         {
            const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(u));
            if (pUnit != NULL)
            {
               mIgnoreList_4.add(pUnit->getID());
            }
         }
      }
   }
}


//==============================================================================
// resetLLP
// reset the LLP and the currentLLWP
//==============================================================================
void BPlatoon::resetLLP_4()
{
   mCurrentLLWP_4 = 0;
   mCurrentLLPath_4.reset();
   
   debugMove4("resetLLP_4");
}

//==============================================================================
// advanceLLWP
// sure this one's simple.. but each one does a bit more! 
//==============================================================================
bool BPlatoon::advanceLLWP_4()
{
   mCurrentLLWP_4++;
   
   debugMove4("advanceLLWP_4: %d to %d", mCurrentLLWP_4-1, mCurrentLLWP_4);
   
   return true;
}

//==============================================================================
// advanceHLWP
// Advance the HLWP, and reset the LLP
//==============================================================================
bool BPlatoon::advanceHLWP_4(BActionState &adjustedState)
{
   mCurrentHLWP_4++;
   resetLLP_4();

   debugMove4("advanceHLWP_4: %d to %d", mCurrentHLWP_4-1, mCurrentHLWP_4);

   // Run the path to the next waypoint right now, we can continue
   // movement in the same frame. 
   int32 pathResult = BPath::cFailed;
   bool bPathAttempt = pathToNextHLWP_4(pathResult);
   // If we couldn't path, path limiter choked us.  return false, no advancement was made.
   if (bPathAttempt == false)
   {
      debugMove4("advanceHLWP_4: pathing limiter stopped us from pathing");
      return false;
   }

   // Adjust the state, based on our result
   // jce [4/17/2008] -- allow cOutsideHulledAreaFailed since it's really a partial
   if (pathResult == BPath::cFailed /*|| pathResult == BPath::cOutsideHulledAreaFailed*/)
   {
      debugMove4("advanceHLWP_4: going to pathing state");
      adjustedState = BAction::cStatePathing;
      return true;
   }
   if (pathResult == BPath::cInRangeAtStart)
   {
      debugMove4("advanceHLWP_4: going to advanceWP state");
      adjustedState = BAction::cStateAdvanceWP;
      return true;
   }
   
   // Otherwise, we're good to go.
   mCurrentLLWP_4 = 1;
   adjustedState = BAction::cStateWorking;
   debugMove4("advanceHLWP_4: going to working state");
   
   return true;
}

//==============================================================================
// advanceUserWP
// Advance the User WP, run the HLP, and return the appropriate value if
// we succeed.
//==============================================================================
bool BPlatoon::advanceUserWP_4()
{

   mCurrentUserWP_4++;

   debugMove4("advanceUserWP_4: %d to %d", mCurrentUserWP_4, mCurrentUserWP_4);

   int32 pathResult = BPath::cFailed;
   mCurrentHLPath_4.reset();
   mCurrentLLPath_4.reset();
   mCurrentHLWP_4 = 0;
   mCurrentLLWP_4 = 0;

   // If we can't run the path, then pathing limiter stopped us
   // If our path fails, or we're in range to begin with, then we're done already. 
   bool bPathRun = pathToNextUserWP_4(pathResult);

   if (!bPathRun || pathResult == BPath::cFailed || pathResult == BPath::cInRangeAtStart)
   {
      #ifdef DEBUG_MOVE4
      if(!bPathRun)
         debugMove4("advanceUserWP_4: failing, limiter prevented us from pathing");
      else if(pathResult == BPath::cFailed)
         debugMove4("advanceUserWP_4: failing, path failed");
      else if(pathResult == BPath::cInRangeAtStart)
         debugMove4("advanceUserWP_4: failing, in range at start");
      #endif
      
      return false;
   }

   // We successfully got a HLP, so all we have to do is make advance the WP, to start moving towards it.
   mCurrentHLWP_4 = 1;

   return true;
}

//==============================================================================
// inLLWPRange
//==============================================================================
bool BPlatoon::inLLWPRange_4()
{
   syncPlatoonData("inLLWPRange_4", mID);

   // Some sanity checks to make sure we're not insane.
   if (mCurrentLLWP_4 < 1 || mCurrentLLWP_4 > mCurrentLLPath_4.getNumberWaypoints() -1)
      return false;
   if (mCurrentHLWP_4 < 1 || mCurrentHLWP_4 > mCurrentHLPath_4.getNumberWaypoints() -1)
      return false;
   if (mCurrentUserWP_4 < 1 || mCurrentUserWP_4 > mCurrentUserPath_4.getNumberWaypoints() - 1)
      return false;

   // If we're at the last waypoint of the llp, and the last waypoint of the hlp, and the last waypoint
   // of the userpath, and we have a target, use the target.  Otherwise, use the point.
   if (mCurrentLLWP_4 == mCurrentLLPath_4.getNumberWaypoints() - 1 && 
       mCurrentHLWP_4 == mCurrentHLPath_4.getNumberWaypoints() - 1 &&
       mCurrentUserWP_4 == mCurrentUserPath_4.getNumberWaypoints() -1)
   {
      BEntity *pTarget = gWorld->getEntity(mCurrentTarget_4.getID());
      if (pTarget && pTarget->isClassType(BEntity::cClassTypeUnit))
         pTarget = gWorld->getEntity(pTarget->getParentID());
      if (pTarget)
      {
         syncPlatoonData("targetID", pTarget->getID());
         
         float fDist = calculateXZDistance(pTarget);
         if (fDist < cActionCompleteEpsilon)
         {
            syncPlatoonData("fDist < cActionCompleteEpsilon", fDist);
            return true;
         }
         // TRB 6/25/08 - Commented these lines out so the platoon will stop moving if the target moved but the platoon reached the
         // end of its path.  Before if the target moved, the platoon would be stuck at the target's original position forever and
         // never try to repath.  Eventually the platoon needs to monitor the target and follow it if it remains in sight.
         //else
         //   return false;
      }
   }
   // Okay, otherwise, use our next waypoint on the path.
   float fDist = calculateXZDistance(mCurrentLLPath_4.getWaypoint(mCurrentLLWP_4));
   if (fDist < cActionCompleteEpsilon)
   {
      syncPlatoonData("fDist < cActionCompleteEpsilon 2", fDist);
      return true;
   }

   return false;
}

//==============================================================================
// inHLWPRange
//==============================================================================
bool BPlatoon::inHLWPRange_4()
{
   float fDist;
   return inHLWPRange_4(fDist);
}

//==============================================================================
// inHLWPRange
//==============================================================================
bool BPlatoon::inHLWPRange_4(float &fDist)
{
   if (mCurrentHLWP_4 < 1 || mCurrentHLWP_4 > mCurrentHLPath_4.getNumberWaypoints() -1)
      return false;
   if (mCurrentUserWP_4 < 1 || mCurrentUserWP_4 > mCurrentUserPath_4.getNumberWaypoints() - 1)
      return false;

   // If we're at the last waypoint of the hlp, and the last waypoint
   // of the userpath, and we have a target, use the target.  Otherwise, use the point. 
   if (mCurrentHLWP_4 == mCurrentHLPath_4.getNumberWaypoints() - 1 &&
       mCurrentUserWP_4 == mCurrentUserPath_4.getNumberWaypoints() -1)
   {
      BEntity *pTarget = gWorld->getEntity(mCurrentTarget_4.getID());
      if (pTarget && pTarget->isClassType(BEntity::cClassTypeUnit))
         pTarget = gWorld->getEntity(pTarget->getParentID());
      if (pTarget)
      {
         syncPlatoonData("targetID", pTarget->getID());
         
         fDist = calculateXZDistance(pTarget);
         // Factor out the radius, if we're using a target
         fDist -= getPathingRadius();
         if (mCurrentTarget_4.isRangeValid())
         {
            fDist -= mCurrentTarget_4.getRange();
         }
         if (fDist < cActionCompleteEpsilon)
         {
            syncPlatoonData("fDist < cActionCompleteEpsilon", fDist);
            return true;
         }
         // TRB 6/25/08 - Commented these lines out so the platoon will stop moving if the target moved but the platoon reached the
         // end of its path.  Before if the target moved, the platoon would be stuck at the target's original position forever and
         // never try to repath.  Eventually the platoon needs to monitor the target and follow it if it remains in sight.
         //else
         //   return false;
      }
   
      // Otherwise, use the waypoint, but with the very close distance.
      fDist = calculateXZDistance(mCurrentHLPath_4.getWaypoint(mCurrentHLWP_4));
      if (fDist < cActionCompleteEpsilon)
      {
         syncPlatoonData("fDist < cActionCompleteEpsilon 2", fDist);
         return true;
      }
      else
      {
         syncPlatoonData("fDist >= cActionCompleteEpsilon", fDist);
         return false;
      }
   }
   
   // Okay, otherwise, use our next waypoint on the path.
   // jce [4/16/2008] -- made this use a loser distance for waypoints in the middle of the path since
   // these will often be blocked by moveable obstructions that the HL pather didn't account for.
   fDist = calculateXZDistance(mCurrentHLPath_4.getWaypoint(mCurrentHLWP_4));
   if (fDist < cInHLWPLooseRangeConst)
   {
      syncPlatoonData("fDist < cInHLWPLooseRangeConst", fDist);
      return true;
   }

   return false;
}

//==============================================================================
// inUserWPRange
//==============================================================================
bool BPlatoon::inUserWPRange_4()
{
   if (mCurrentUserWP_4 < 1 || mCurrentUserWP_4 > mCurrentUserPath_4.getNumberWaypoints() - 1)
      return false;

   // If we're at the last waypoint of the userpath, and we have a target, use the target.  Otherwise, use the point. 
   if (mCurrentUserWP_4 == mCurrentUserPath_4.getNumberWaypoints() - 1)
   {
      BEntity *pTarget = gWorld->getEntity(mCurrentTarget_4.getID());
      if (pTarget && pTarget->isClassType(BEntity::cClassTypeUnit))
         pTarget = gWorld->getEntity(pTarget->getParentID());
      if (pTarget)
      {
         float fDist = calculateXZDistance(pTarget);
         // Use the pathing radius to determine if we're close enough to the target.
         fDist -= getPathingRadius();
         if (mCurrentTarget_4.isRangeValid())
         {
            fDist -= mCurrentTarget_4.getRange();
         }
         if (fDist < cActionCompleteEpsilon)
         {
            syncPlatoonData("fDist < cActionCompleteEpsilon", fDist);
            return true;
         }
         // TRB 6/25/08 - Commented these lines out so the platoon will stop moving if the target moved but the platoon reached the
         // end of its path.  Before if the target moved, the platoon would be stuck at the target's original position forever and
         // never try to repath.  Eventually the platoon needs to monitor the target and follow it if it remains in sight.
         //else
         //   return false;
      }
   }
   // Okay, otherwise, use our next waypoint on the path.
   float fDist = calculateXZDistance(mCurrentUserPath_4.getWaypoint(mCurrentUserWP_4));
   if (fDist < cActionCompleteEpsilon)
   {
      syncPlatoonData("fDist < cActionCompleteEpsilon 2", fDist);
      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BPlatoon::inRangeOfTarget_4()
{
   // jce [4/16/2008] -- simple check of whether we're in range of our target.

   // If we have a target, use that.   
   BEntity *pTarget = gWorld->getEntity(mCurrentTarget_4.getID());
   if (pTarget && pTarget->isClassType(BEntity::cClassTypeUnit))
      pTarget = gWorld->getEntity(pTarget->getParentID());
   if (pTarget)
   {
      // DLM - Instead of using the platoon's position, we'll cycle throug the squads 
      // in a perf friendly fashion and see if any squad is in range.  
      // DLMTODO - do these range calculations for the squads once, when we get the order,
      // and save them.
      long numChildren = mChildren.getNumber();
      for (long m = 0; m < numChildren; m++)
      {
         BSquad *pSquad = reinterpret_cast<BSquad *>(gWorld->getEntity(mChildren[m]));
         if (pSquad)
         {
            float fDist = pSquad->calculateXZDistance(pTarget);
            float fSquadRange = 0.0f;
            // If we haven't already calculated a range for this squad, do so now.
            if (mSquadRanges[m] < 0.0f)
                pSquad->calculateRange(&mCurrentTarget_4, fSquadRange, NULL, NULL);
            if (mSquadRanges[m] >= 0.0f)
               fDist -= mSquadRanges[m];
            if (fDist < cActionCompleteEpsilon)
               return true;
         }
      }
      /*
      float fDist = calculateXZDistance(pTarget);
      fDist -= getPathingRadius();
      // Account for our range, held in Target.. 
      if (mCurrentTarget_4.isRangeValid())
      {
         fDist -= mCurrentTarget_4.getRange();
      }
      if (fDist < cActionCompleteEpsilon)
      {
         syncPlatoonData("fDist < cActionCompleteEpsilon", fDist);
         return(true);
      }
      */
      return(false);
   }


   // If we got here we have no target, so use our last user waypoint.
   long lastWaypoint = mCurrentUserPath_4.getNumberWaypoints()-1;
   // DLM - Doh!  We also have to make sure we're currently on our next to last waypoing.. otherwise
   // we'll quit right away if we've been ordered to move in a circle. 
   if(lastWaypoint >= 0 && mCurrentUserWP_4 == lastWaypoint)
   {
      float fDist = calculateXZDistance(mCurrentUserPath_4.getWaypoint(lastWaypoint));
      if (fDist < cActionCompleteEpsilon)
      {
         syncPlatoonData("fDist < cActionCompleteEpsilon 2", fDist);
         return(true);
      }
      else
      {
         syncPlatoonData("fDist >= cActionCompleteEpsilon", fDist);
         return(false);
      }
   }

   // Epic fail -- somehow you're checking target distance with no target entity or point.
   syncPlatoonCode("epic fail");
   return false;
}

//==============================================================================
// isSquadAtTarget
//==============================================================================
bool BPlatoon::isSquadAtTarget_4(BEntityID child)
{
   // DLM 6/17/08 - So forcing the "atTarget" check to wait on whether or not the platoon
   // is on the last leg of it's path is bogus.  We could be within range of our target
   // after having computed valid plots, but still not be on our the last leg of our path.
   debugMove4("isSquadAtTarget -- checking Squad ID: %ld", child.asLong());

   // If we have valid squadplots, we should check that.. 
   if (mValidPlots_4)
   {
      debugMove4("isSquadAtTarget -- mValidPlots is set, checking against plotted Position.");
      // Find this child..
      for (long n = 0; n < mSquadPlots_4.getNumber(); n++)
      {
         if (mSquadPlots_4[n].getSquadID() == child)
         {
            BSquad *squad = gWorld->getSquad(child);
            if (!squad)
               return false;
            BVector destination = mSquadPlots_4[n].getDesiredPosition();
            BVector loc = squad->getPosition();
            BVector direction = destination - loc;
            direction.y = 0.0f;
            // TRB 7/1/08 - Removed subtraction of the pathing radius.  The squad should get as near to the
            // plotted position as possible since it could be as little as just 1.0m within attack range.
            // Subtracting the pathing radius meant it could stop without ever getting into range.
            float fLen = direction.length();
            bool bReturn = false;
            if (fLen < cActionCompleteEpsilon)
               bReturn = true;
            debugMove4("isSquadAtTarget: plotted pos: (%6.2f, %6.2f, %6.2f), location: (%6.2f, %6.2f, %6.2f), distance: (%6.2f), atTarget: %d",
               destination.x, destination.y, destination.z, loc.x, loc.y, loc.z, fLen, (int)bReturn);
            return bReturn;
         }            
      }
      // If somehow we got here, we didn't find our child.  REturn false.
      debugMove4("isSquadAtTarget -- failed to find child %d in squad plot positions.", child.asLong());
      return false;
   }
   
   debugMove4("isSquadAtTarget -- no mValidPlots, just comparing to platoon formation positions.");
   // No waypoints/paths, etc.  Just compare squad's location to our formation position for that squad.
   BSquad *squad = gWorld->getSquad(child);
   if (!squad)
      return false;
   if (!mpFormation)
      return false;
   const BFormationPosition2 *pPos = mpFormation->getFormationPosition(child);
   if (pPos)
   {
      BVector formationPos = pPos->getPosition();
      BVector direction = formationPos - squad->getPosition();
      direction.y = 0.0f;
      float fLen = direction.length() - getPathingRadius();
      if (fLen < cActionCompleteEpsilon)
      {
         debugMove4("isSquadAtTarget -- %d Within range of platoon position.  Returning true.", child.asLong());
         return true;
      }
   }
   debugMove4("isSquadAtTarget -- %d Squad too far away fromp platoon position.  Returning false.", child.asLong());
   return false;
}

//==============================================================================
// getPlottedPosition_4 - returns a squadploter position for the given child,
// if we've calculated positions.
//==============================================================================
bool BPlatoon::getPlottedPosition_4(const BEntityID child, BVector &position)
{
   if (mValidPlots_4)
   {
      // Find this child..
      for (long n = 0; n < mSquadPlots_4.getNumber(); n++)
      {
         if (mSquadPlots_4[n].getSquadID() == child)
         {
            position = mSquadPlots_4[n].getDesiredPosition();
            syncPlatoonData("squad plot pos", position);
            return true;
         }            
      }
      return false;
   }
   return false;
}

//==============================================================================
// hasDefaultSquadPlot_4 - returns whether the squad's plotted position is the default
// one meaning that the squad probably just has the target position and needs to check range
//==============================================================================
bool BPlatoon::hasDefaultSquadPlot_4(const BEntityID child)
{
   if (mValidPlots_4)
   {
      // Find this child..
      for (long n = 0; n < mSquadPlots_4.getNumber(); n++)
      {
         if (mSquadPlots_4[n].getSquadID() == child)
         {
            return (mSquadPlots_4[n].getDefaultDesiredPos());
         }            
      }
      return false;
   }
   return false;
}

//==============================================================================
// getPlatoonVelocity
// platoonVelocity is the velocity of the slowest squad in the platoon.
// Eventually, sub this routine into getDesiredVelocity.
// Always recalc this, as children come and go. 
//==============================================================================
float BPlatoon::getPlatoonVelocity_4()
{
   float platoonVelocity = cMaximumFloat;
   bool bSet = false;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad *squad = gWorld->getSquad(mChildren[i]);
      if (!squad)
         continue;
      if (squad->getDesiredVelocity() < platoonVelocity)
      {
         platoonVelocity = squad->getDesiredVelocity();
         bSet = true;
      }
   }
   // checksafe
   if (!bSet)
      return 0.0f;

   return platoonVelocity;

}

//==============================================================================
// advancePosition
//==============================================================================
bool BPlatoon::advancePosition_4(float elapsedTime, float velocityFactor)
{
   // jce [5/2/2008] -- Scale velocity by passed in factor, which is used to slow us down if squads 
   // are starting to lag too far behind.
   float velocity = getPlatoonVelocity_4() * velocityFactor;
   
   if (mCurrentLLWP_4 < 1 || mCurrentLLWP_4 > mCurrentLLPath_4.getNumberWaypoints() -1)
   {
      syncPlatoonCode("No LL waypoints");
      return(false);
   }

   debugMove4("Advancing at velocity %0.1f scaled by velocityFactor: %f", velocity, velocityFactor);

   // Get our destination, and the direction to our destination..
   BVector destination = mCurrentLLPath_4.getWaypoint(mCurrentLLWP_4);
   BVector direction = destination - mPosition;
   direction.y = 0;
   direction.safeNormalize();

   float distCovered = velocity * elapsedTime;
   BVector newPosition = mPosition + (direction * distCovered);
   // Check to see if we overshot the position.
   BVector newDirection = destination - newPosition;
   newDirection.y = 0;
   newDirection.safeNormalize();

   if (direction.dot(newDirection) < 0)
   {
      // We'll overshoot, so just set our destination as our position
      newPosition = destination;

      debugMove4("   We were going to overshoot, snapping to destination (%0.2f, %0.2f, %0.2f)", destination.x, destination.y, destination.z);
   }
   
   // Check for collisions.
   
   // First, figure out what type of obstructions we care about.
   // jce [4/17/2008] -- I ripped this logic out of the pather for when a platoon, so hopefully it's setting the right flags.
   // It might need some tweaking though?
   // jce [9/5/2008] -- updated with flying units
   // First, figure out what type of obstructions we care about.
   long quadtrees=BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeAllCollidableSquads;
   if (isPathableAsFlyingUnit())
      quadtrees = BObstructionManager::cIsNewTypeBlockAirMovement;
   else if (isPathableAsFloodUnit())
      quadtrees |= BObstructionManager::cIsNewTypeBlockFloodUnits | BObstructionManager::cIsNewTypeBlockAllMovement;
   else if (isPathableAsScarabUnit())
      quadtrees |= BObstructionManager::cIsNewTypeBlockScarabUnits | BObstructionManager::cIsNewTypeBlockAllMovement;
   else
      quadtrees |= BObstructionManager::cIsNewTypeBlockLandUnits | BObstructionManager::cIsNewTypeBlockAllMovement;

   // Build the ignore list.   
   // jce [4/17/2008] -- maybe this can be changed so it happens only when composition of platoon changes?
   buildIgnoreList_4(); 

   // Set up obstruction manager.
   gObsManager.begin(BObstructionManager::cBeginNone, getPathingRadius(), quadtrees, BObstructionManager::cObsNodeTypeAll, mPlayerID, cDefaultRadiusSofteningFactor, &mIgnoreList_4, canJump());
   
   // See if we intersect anything.
   BVector iPoint(newPosition);     // jce [4/17/2008] -- setting this to newPosition is probably being overly cautious since it should always be initialized if there is an intersection
   BObstructionNodePtrArray obstructions;
   bool intersects = gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, mPosition, newPosition, true, iPoint, obstructions);

   // Start by assuming everything is ok, until proven otherwise.   
   bool fullMovement = true;

   // If we did intersect, we need to check whether it's a collision we care about.
   if(intersects)
   {
      debugMove4("   Found %d intersections --", obstructions.getNumber());
      // Run through the obstructions
      for(long i=0; i<obstructions.getNumber(); i++)
      {
         // Get obstruction node.
         BOPObstructionNode *ob = obstructions[i];
         if(!ob)
            continue;
         
         // jce [4/18/2008] -- as far as I know in HW, only units/squads can move after the game begins (so things like
         // terrain are never valid collisions here).
         if(ob->mType == BObstructionManager::cObsNodeTypeUnit || ob->mType == BObstructionManager::cObsNodeTypeSquad)
         {
            // jce [9/8/2008] -- Try skipping anything that's moving.  There is a big issue with the platoon's colliding
            // in general since they collide with things but nothing collides with them ... so they can locked into a cycle
            // where they are colliding every update (like something is moving on top of them).  Hopefully skipping the moving
            // things will be enough to make that better without breaking the ability of the platoon to route around things it
            // really needs to.
            if(ob->mObject->isMoving())
            {
               debugMove4("   skipping this collision since the object is moving");
               continue;
            }
         
            // Look for parent squad if it's a unit.
            if(ob->mType == BObstructionManager::cObsNodeTypeUnit && ob->mObject->getParent())
            {
               // Get parent.
               BEntity *parent = ob->mObject->getParent();
               
               // If it's a squad parent and the squad actually has an obstruction, we ignore this collision.
               if(parent->getClassType() == BEntity::cClassTypeSquad && parent->getObstructionNode() != NULL)
               {
                  debugMove4("   this was a unit collision and the unit has a squad, so skipping it.");
                  continue;
               }
            }

            // Look at the last time the entity we collided with has moved.
            DWORD lastMoveTime = ob->mObject->getLastMoveTime();
            
            // If we moved since the path was made, then this is a valid collision.
            if(mCurrentLLPath_4.getCreationTime() < lastMoveTime)
            {
               // jce [4/18/2008] -- note that this iPoint is overly conservative because it is the closest intersection point of any collision
               // and not the one on the actual obstruction in question (or necessarily one that has moved since last path).  If we wanted to fix that, 
               // we'd need to not bail out here but instead examine all collisions that were valid and keep the closest intersection.  
               // I think this will be good enough though since we're going to have to re-path anyway and the difference is probably only centimeters.
               newPosition = iPoint;
               fullMovement = false;
               
               debugMove4("   we collided with %d and need to repath", ob->mEntityID);
               #ifdef PLATOON_PAUSING
               // Okay, highly experimental stuff going on here.  Check to see if the unit we're pathing around is an allied squad.  If so,
               // get it's platoon, and compare it's platoon speed to our speed.  If we're faster, pause it.  If we're slower, pause ourselves.  
               if (ob->mType == BObstructionManager::cObsNodeTypeSquad)
               {
                  BSquad *pCollidedSquad = (BSquad *)gWorld->getEntity(ob->mEntityID);
                  if (!pCollidedSquad)
                     continue;
                  BPlatoon *pCollidedPlatoon = reinterpret_cast<BPlatoon *>(pCollidedSquad->getParent());
                  if (!pCollidedPlatoon)
                     continue;
                  // Are we allies?
                  BPlayer *player=gWorld->getPlayer(getPlayerID());
                  if (!player)
                     continue;
                  bool allies = player->isAlly(pCollidedPlatoon->getPlayerID());
                  if (allies)
                  {
                     if (getPlatoonVelocity_4() > pCollidedPlatoon->getPlatoonVelocity_4())
                     {
                        debugMove4("   platoon %d is slower than us, so we are pausing that platoon.", pCollidedPlatoon->getID());
                        pCollidedPlatoon->pauseMovement_4(cDefaultPauseTimeout);
                     }
                     else
                     {
                        debugMove4("   platoon %d is faster than us, so we are going to pause, and not repath..", pCollidedPlatoon->getID());
                        pauseMovement_4(cDefaultPauseTimeout);
                     }
                  }
               }
               #endif
               break;
            }
            else
            {
               debugMove4("   we collided with %d but it hasn't moved since our path was created, so skipping", ob->mEntityID);
            }
         }
         else
         {
            debugMove4("   ignoring obstruction for entity %d", ob->mEntityID);
         }
      }
   }
   else
   {
      debugMove4("   No intersections");
   }
      
   // Done with obstruction manager.
   gObsManager.end();
   
   // Set our new position/orientation
   BVector forward = newPosition - getPosition();
   // jce [8/29/2008] -- project onto xz plane, otherwise we could get something like a tiny upwards vector that then produces
   // a bogus right vector since we're assuming cYAxisVector as our up.
   forward.y = 0.0f;
   if(forward.safeNormalize())
   {
      // and forward and right.. 
      setForward(forward);
      BVector right;
      right.assignCrossProduct(cYAxisVector, forward);
      setRight(right);
   }
   
   setPosition(newPosition);
   debugMove4("   resulting new position (%0.2f, %0.2f, %0.2f)", newPosition.x, newPosition.y, newPosition.z);
   
   // Return whether we moved fully or not.
   return(fullMovement);
}

//==============================================================================
//==============================================================================
void BPlatoon::notifyThatSquadHasPaused_4(BEntityID child)
{
   // Eventually we might want the platoon to keep track of which children are paused and which one's are not.  
   // For now, nothing.
}

//==============================================================================
//==============================================================================
void BPlatoon::notifyThatSquadWillHandleItsOwnMovement_4(BEntityID child)
{
   BPlatoonActionMove* pMoveAction = reinterpret_cast <BPlatoonActionMove*>(getActionByType(BAction::cActionTypePlatoonMove));
   if (!pMoveAction)
      return;

   // Set the state for the squad to done so the platoon doesn't try to control its movement
   pMoveAction->notifyThatSquadWillHandleItsOwnMovement(child);
}


//==============================================================================
//==============================================================================
bool BPlatoon::isPlatoonDoneMoving_4(bool *isPaused /*=NULL*/)
{
   if(isPaused)
      *isPaused = false;
      
   // DLM 5/6/8 - does platoon have a move action?  If so is it waiting?  If
   // so, the platoon is done movint. 
   const BPlatoonActionMove* pMoveAction=reinterpret_cast <BPlatoonActionMove*> (getActionByType(BAction::cActionTypePlatoonMove));
   if (!pMoveAction)
      return true;
   if (pMoveAction->getState() == BAction::cStateWaitOnChildren)
      return true;
   // Paused can mean either the wait state or the paused state..
   if (isPaused && (pMoveAction->getState() == BAction::cStateWait ||  pMoveAction->getState() == BAction::cStatePaused))
      *isPaused = true;
   return false;
}

//==============================================================================
// DLM 5/7/8 - pass it in a simtarget, and it will set the target's range with the 
// shortest range in the squad.  This is probably not so great for mixed
// squad platoons, but I suspect we'll soon be using homogeneous platoons anyway.
//==============================================================================
bool BPlatoon::getPlatoonRange_4(BSimTarget &target)
{
   bool bValidRange;
   bool bSet = false;
   float minRange = cMaximumFloat;
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad *squad = gWorld->getSquad(mChildren[i]);
      if (!squad)
         continue;
      float range;
      bValidRange = squad->calculateRange(&target, range, NULL, NULL);
      if (bValidRange)
      {
         if (range < minRange)
         {
            minRange = range;
            bSet = true;
         }
      }
   }
   // checksafe
   if (!bSet)
      return false;

   // Set the range and return
   target.setRange(minRange);
   return true;

}

//==============================================================================
// DLM 5/7/8 - Called by the platoon Action when the action wants plots.
// there's actually no reason this couldn't be called as soon as the target it
// set, but as it checks for obstructions, it's probably best to call it just
// before it's needed
//==============================================================================
void BPlatoon::computePlots_4()
{
   if (mCurrentTarget_4.isIDValid())
   {
      BDynamicSimVectorArray temp;     // Empty waypoint list.  Let squad plotter calculate best side of target as destination.
      // DLM 9/2/08 - The squad plotter really doesn't want waypoints from us.  Giving it the target location as the first
      // waypoint only makes it place waypoint positions in whacky positions.
      //temp.setNumber(1);
      //temp[0] = mCurrentTarget_4.getPosition();

      const BEntityIDArray &childList = getChildList();
      long abilityID = -1;
      if (mCurrentTarget_4.isAbilityIDValid())
         abilityID = mCurrentTarget_4.getAbilityID();
      mValidPlots_4 = gSquadPlotter.plotSquads(childList, getPlayerID(), mCurrentTarget_4.getID(), temp, mCurrentTarget_4.getPosition(), abilityID, BSquadPlotter::cSPFlagIgnorePlayerRestriction);
      if (mValidPlots_4 && (gSquadPlotter.getResults().getNumber() != childList.getNumber()))
         mValidPlots_4 = false;
      if (mValidPlots_4)
      {
         const BDynamicSimArray<BSquadPlotterResult> &results = gSquadPlotter.getResults();
         mSquadPlots_4.resize(0);
         for (long n = 0; n < results.getNumber(); n++)
            mSquadPlots_4.add(results[n]);
      }
   }
   return;
}

//==============================================================================
// DLM 5/27/08 - Similar to the version on squad -- gets the first executing
// or paused order.
//==============================================================================
const BSimOrderEntry* BPlatoon::getOrderEntry_4(bool execute, bool paused) const
{
   //Return the first order that matches.
   for (uint i=0; i < mOrders.getSize(); i++)
   {
      if (execute && (mOrders[i].getState() == BSimOrderEntry::cStateExecute))
         return ( &(mOrders[i]));
      if (paused && (mOrders[i].getState() == BSimOrderEntry::cStatePaused))
         return ( &(mOrders[i]));
   }

   return (NULL);
}


//==============================================================================
// pauseMovement_4
// Called (by other platoons, most likely, when we want to pause an entire platoon.
void BPlatoon::pauseMovement_4(DWORD pauseTime)
{
   BPlatoonActionMove* pMoveAction=reinterpret_cast <BPlatoonActionMove*> (getActionByType(BAction::cActionTypePlatoonMove));
   if (!pMoveAction)
      return;
   pMoveAction->pauseMovement(pauseTime);

   // Go through, and pause all of the children.  Immediately.  Where they are. 
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BSquad *squad = gWorld->getSquad(mChildren[i]);
      if (!squad)
         continue;
      squad->pauseMovement_4(pauseTime);
   }

}

//==============================================================================
// isSquadDoneMoving_4
// Used by external actions (including squad action move) to see if a particular squad is done moving
// during a particular move action.  NOTE -- don't confused this with isSquadAtTarget.  isSquadAtTarget
// might be used by the move action to determine *if* a squad should be done moving, but a squad can 
// be done moving without ever being at its target. 
bool BPlatoon::isSquadDoneMoving_4(const BEntityID child)
{
   BPlatoonActionMove* pMoveAction=reinterpret_cast <BPlatoonActionMove*> (getActionByType(BAction::cActionTypePlatoonMove));
   if (!pMoveAction)
      return true;

   BSquadActionEntry entry;
   pMoveAction->getSquadActionEntry(child, entry);
   return (entry.mState == BSquadActionEntry::cActionEntryStateDone);
}

bool BPlatoon::isSquadProceedingToPlot_4(const BEntityID child)
{
   BPlatoonActionMove* pMoveAction=reinterpret_cast <BPlatoonActionMove*> (getActionByType(BAction::cActionTypePlatoonMove));
   if (!pMoveAction)
      return true;

   BSquadActionEntry entry;
   pMoveAction->getSquadActionEntry(child, entry);
   return (entry.mState == BSquadActionEntry::cActionEntryStateProceedingToPlot);
}

//==============================================================================
// setSquadPlotWaypoint()
// New routine to see if any of my squads are standing around on top of each other.  
// if they are, attempt to move them to a location that is clear.  
void BPlatoon::setSquadPlotWaypoint()
{
   mSquadPlotWaypoint = -1;
   long nNumberHLWaypoints = mCurrentHLPath_4.getNumberWaypoints();
   if (nNumberHLWaypoints < 2)
      return;

   // The destination will be the last waypoint location, not the target location.  The target may move, and we'll
   // have other code that deals with repathing if the target has moved far enough. 
   BVector vDestination = mCurrentHLPath_4.getWaypoint(nNumberHLWaypoints - 2);
   long nTestIndex = nNumberHLWaypoints - 2;

   bool bDone = false;
   float fTotalDistance = 0.0f;
   while (!bDone)
   {
      fTotalDistance += mCurrentHLPath_4.getWaypoint(nTestIndex).xzDistanceSqr(mCurrentHLPath_4.getWaypoint(nTestIndex+1)); 
      if (fTotalDistance > (BPather::cShortRangeLimitDist * 0.5f))
         bDone = true;
      else
      {
         if (--nTestIndex < 0)
            bDone = true;
      }                 
   }
   mSquadPlotWaypoint = nTestIndex + 1;
   return;
}


//==============================================================================
// doSquadRelocation()
// New routine to see if any of my squads are standing around on top of each other.  
// if they are, attempt to move them to a location that is clear.  
void BPlatoon::doSquadRelocation()
{
   // jce [10/9/2008] -- commenting this out for now.  We didn't think it was necessarily helping too much right
   // now and it has some problems like constantly causing vehicles to spin by commanding them short distances.  Plus
   // it ends up sucking up paths, etc. to do this.  If we decide we want it back, we'll want to tweak that stuff.
   
/*
   // Get the next squad to look at. 
   // IF the squad to relocate is invalid, reset it, and look next time.  
   if (mSquadToRelocate < 0 || mSquadToRelocate > mChildren.getNumber() - 1)
   {
      return;
   }
   BSquad* pSquad = gWorld->getSquad(mChildren[mSquadToRelocate]);
   if (pSquad == NULL)
   {
      ++mSquadToRelocate;
      return;
   }
   // Squad not doing anything?
   if (pSquad->getNumberOrders() != 0)
   {
      debugMove4("Squad (%d) is busy. Can't relocate.", mSquadToRelocate);
      ++mSquadToRelocate;
      return;
   }
   if (pSquad->getSquadMode() == BSquadAI::cModeHitAndRun)
      return;

   // Early out if squad is under cinematic control, that is the squad is controlled by the wow moment
   if(pSquad->isUnderCinematicControl())
      return;

   // Don't do this for hitch or jumping stuff
   if (pSquad->getFlagIsHitching() || pSquad->getFlagIsUnhitching() || pSquad->isHitched() || pSquad->getFlagJumping())
   {
      return;
   }

   // Okay so lets see if he's obstructed.  Time to crack open the obstruction manager. 
   // All the easy checks are done.  We need to see if we're standing on anything. 
   long quadtrees= BObstructionManager::cIsNewTypeCollidableStationarySquad;
   if (isPathableAsFlyingUnit())
      quadtrees = BObstructionManager::cIsNewTypeBlockAirMovement;
   else if (isPathableAsFloodUnit())
      quadtrees |= BObstructionManager::cIsNewTypeBlockFloodUnits;
   else if (isPathableAsScarabUnit())
      quadtrees |= BObstructionManager::cIsNewTypeBlockScarabUnits;
   else
      quadtrees |= BObstructionManager::cIsNewTypeBlockLandUnits;

   // Set up ignore list.
   BEntityIDArray ignoreList;
   // Don't use the standard ignoreList for movement.  In this case, we want to check against EVERY ONE, 
   // including our platoon mates.  So only add myself and my children.
   ignoreList.add(pSquad->getID());
   const BEntityIDArray &myUnitList = pSquad->getChildList();
   ignoreList.add(myUnitList.getPtr(), myUnitList.size());

   // Set up obstruction manager.
   gObsManager.begin(BObstructionManager::cBeginNone, pSquad->getObstructionRadius(), quadtrees, BObstructionManager::cObsNodeTypeAll, pSquad->getPlayerID(), cDefaultRadiusSofteningFactor, &ignoreList, pSquad->canJump());

   // See if we are obstructed.
   BVector iPoint;
   BObstructionNodePtrArray obstructionList;
   gObsManager.findObstructions(pSquad->getPosition(), false, false, obstructionList);
   // If not, then we're done.
   if (obstructionList.getNumber() == 0)
   {
      debugMove4("Squad (%d) is not obstructed. Don't need to relocate.", mSquadToRelocate);
      ++mSquadToRelocate;
      gObsManager.end();
      return;
   }
   // Okay, let's try to find a spot to move to..
   BVector vRelocationPoint;
   if (gPather.findUnobstructedPoint_3(pSquad->getPosition(), vRelocationPoint) == false)
   {
      // If we couldn't find an unobstructed position, then buh bye. 
      debugMove4("Squad (%d) is obstructed, but I couldn't find a clear spot. Not Relocating.", mSquadToRelocate);
      ++mSquadToRelocate;
      gObsManager.end();
      return;
   }

   // done with the obs manager
   gObsManager.end();

   // We have a location!  Tell the squad to move there!
   mRelocationTarget.setPosition(vRelocationPoint);
   // Even though we're a platoon, and we're issuing a move, this is NOT a platoon move, per se.  It's just
   // like an action move, and so the platoonMove parm is false.
    mRelocationActionID = pSquad->doMove(NULL, NULL, &mRelocationTarget, false, true);
   if (mRelocationActionID == cInvalidActionID)
   {
      // If we couldn't generate the action, well try again later.
      debugMove4("Squad (%d) was unable to generate a move action. Not Relocating.", mSquadToRelocate);
      ++mSquadToRelocate;
      return;
   }

   debugMove4("Squad (%d) is commencing Relocation!.", mSquadToRelocate);
   ++mSquadToRelocate;
   mRelocationInProgress = true;
   return;
   */
}

//==============================================================================
void BPlatoon::attackMoveNotify(BEntityID child, BSimTarget target)
{
   // Okay, a child is informing us it's queued up an attack from attack move.  Walk through any other
   // children we have.  If they are not attack/moving towards the target, then send them to do so. 
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      // Don't recheck ourselves. 
      if (mChildren[i] == child)
         continue;
      BSquad *squad = gWorld->getSquad(mChildren[i]);
      if (!squad)
         continue;
      // Easiest way to tell if a unit is doing the attack move thing is if it's paused a move order.  
      bool bQueue = true;
      for (int i = 0; i < squad->getNumberActions(); i++)
      {
         const BAction* pAction = squad->getActionByIndexConst(i);
         if (pAction && (pAction->getType() == BAction::cActionTypeSquadMove) && (pAction->getState() == BAction::cStatePaused))
         {
            bQueue = false;
            break;
         }
      }
      if (bQueue)
         squad->queueAttack(target, true, false, true);
   }

}

#endif

//==============================================================================
//==============================================================================
bool BPlatoon::save(BStream* pStream, int saveType) const
{
   if (!BEntity::save(pStream, saveType))
      return false;

   GFWRITECLASSARRAY(pStream, saveType, mPathingHints, uint8, 200);
   GFWRITEVECTORARRAY(pStream, mPathingHintForwards, uint8, 200);
   GFWRITEARRAY(pStream, BEntityID, mChildren, uint8, 200);
   GFWRITEARRAY(pStream, BEntityID, mPendingChildrenToRemove, uint8, 200);
   GFWRITECLASSARRAY(pStream, saveType, mOrders, uint8, 200);
   GFWRITECLASSPTR(pStream, saveType, mpFormation);
   GFWRITEVAR(pStream, BEntityID, mSlowestSquadID);
   GFWRITEVAR(pStream, float, mSlowestSquadVelocity);
   GFWRITEVAR(pStream, uint, mPathingHintsSimOrderID);
   GFWRITEVAR(pStream, DWORD, mPathingHintsTime);
   GFWRITEBITBOOL(pStream, mSlowestSquadVelocityValid);
   GFWRITECLASS(pStream, saveType, mCurrentUserPath_4);
   GFWRITECLASS(pStream, saveType, mCurrentHLPath_4);
   GFWRITECLASS(pStream, saveType, mCurrentLLPath_4);
   GFWRITEVAR(pStream, int32, mCurrentUserWP_4);
   GFWRITEVAR(pStream, int32, mCurrentHLWP_4);
   GFWRITEVAR(pStream, int32, mCurrentLLWP_4);
   GFWRITECLASS(pStream, saveType, mCurrentTarget_4);
   GFWRITEARRAY(pStream, BEntityID, mIgnoreList_4, uint16, 2000);
   GFWRITECLASSARRAY(pStream, saveType, mSquadPlots_4, uint16, 2000);
   GFWRITEVAR(pStream, bool, mValidPlots_4);
   GFWRITECLASS(pStream, saveType, mRelocationTarget);
   GFWRITEVAR(pStream, long, mSquadToRelocate);
   GFWRITEVAR(pStream, bool, mRelocationInProgress);
   GFWRITEVAR(pStream, BActionID, mRelocationActionID);
   GFWRITEVAR(pStream, long, mSquadPlotWaypoint);
   GFWRITEARRAY(pStream, float, mSquadRanges, uint8, 200);

   GFWRITEMARKER(pStream, cSaveMarkerPlatoon1);
   return true;
}

//==============================================================================
//==============================================================================
bool BPlatoon::load(BStream* pStream, int saveType)
{
   if (!BEntity::load(pStream, saveType))
      return false;

   GFREADCLASSARRAY(pStream, saveType, mPathingHints, uint8, 200);
   GFREADVECTORARRAY(pStream, mPathingHintForwards, uint8, 200);
   GFREADARRAY(pStream, BEntityID, mChildren, uint8, 200);
   GFREADARRAY(pStream, BEntityID, mPendingChildrenToRemove, uint8, 200);
   GFREADCLASSARRAY(pStream, saveType, mOrders, uint8, 200);
   GFREADCLASSPTR(pStream, saveType, mpFormation);
   GFREADVAR(pStream, BEntityID, mSlowestSquadID);
   GFREADVAR(pStream, float, mSlowestSquadVelocity);
   GFREADVAR(pStream, uint, mPathingHintsSimOrderID);
   GFREADVAR(pStream, DWORD, mPathingHintsTime);
   GFREADBITBOOL(pStream, mSlowestSquadVelocityValid);
   GFREADCLASS(pStream, saveType, mCurrentUserPath_4);
   GFREADCLASS(pStream, saveType, mCurrentHLPath_4);
   GFREADCLASS(pStream, saveType, mCurrentLLPath_4);
   GFREADVAR(pStream, int32, mCurrentUserWP_4);
   GFREADVAR(pStream, int32, mCurrentHLWP_4);
   GFREADVAR(pStream, int32, mCurrentLLWP_4);
   GFREADCLASS(pStream, saveType, mCurrentTarget_4);
   GFREADARRAY(pStream, BEntityID, mIgnoreList_4, uint16, 2000);
   GFREADCLASSARRAY(pStream, saveType, mSquadPlots_4, uint16, 2000);
   GFREADVAR(pStream, bool, mValidPlots_4);
   GFREADCLASS(pStream, saveType, mRelocationTarget);
   GFREADVAR(pStream, long, mSquadToRelocate);
   GFREADVAR(pStream, bool, mRelocationInProgress);
   GFREADVAR(pStream, BActionID, mRelocationActionID);
   if (BPlatoon::mGameFileVersion >= 2)
      GFREADVAR(pStream, long, mSquadPlotWaypoint);
   if (BPlatoon::mGameFileVersion >= 3)
   {
      GFREADARRAY(pStream, float, mSquadRanges, uint8, 200);
   }
   else
   {
      mSquadRanges.clear();
      for (long n = 0; n < mChildren.getNumber(); n++)
         mSquadRanges.add(-1.0f);
   }

   GFREADMARKER(pStream, cSaveMarkerPlatoon1);

   // Remove order entries with NULL orders.  This probably points to an issue where the order
   // wasn't cleaned up properly before saving, but this will at least prevent future crashes.
   for (int i = (mOrders.getNumber() - 1); i >= 0; i--)
   {
      BSimOrder *pTempOrder = mOrders[i].getOrder();
      if (pTempOrder == NULL)
         mOrders.removeIndex(i);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPathingHints::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTORARRAY(pStream, mWaypoints, uint8, 200);
   GFWRITEVAR(pStream, BEntityID, mID);
   GFWRITEBITBOOL(pStream, mComplete);
   return true;
}

//==============================================================================
//==============================================================================
bool BPathingHints::load(BStream* pStream, int saveType)
{
   GFREADVECTORARRAY(pStream, mWaypoints, uint8, 200);
   GFREADVAR(pStream, BEntityID, mID);
   GFREADBITBOOL(pStream, mComplete);
   return true;
}
