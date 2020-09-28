//==============================================================================
// platoonactionmove.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "ConfigsGame.h"
#include "Platoon.h"
#include "database.h"
#include "EntityScheduler.h"
#include "platoonactionmove.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "pather.h"
#include "pathingLimiter.h"
#include "obstructionmanager.h"
#include "commands.h"
#include "squadPlotter.h"
#include "triggervar.h"
#include "UnitOpportunity.h"
#include "protoobject.h"
#include "Formation2.h"
#include "protosquad.h"
#include "physics.h"
#include "physicsobject.h"
#include "tactic.h"
#include "weapontype.h"
#include "worldsoundmanager.h"
#include "physicsinfomanager.h"
#include "physicsinfo.h"
#include "physicswarthogaction.h"
#include "ability.h"
#include "unitactioncollisionattack.h"
#include "user.h"
#include "usermanager.h"
#include "squadactionattack.h"
#include "SimOrderManager.h"
#include "squadlosvalidator.h"
#include "squadactionmove.h"

#include "physicsinfo.h"
#include "physicsinfomanager.h"

#include "actionmanager.h"

#ifndef _MOVE4
#define _MOVE4
#endif


GFIMPLEMENTVERSION(BSquadActionEntry, 2);

#ifndef BUILD_FINAL
//     #define DEBUG_MOVE4
//     #define DEBUG_UPDATESQUADS4

#endif

#ifdef DEBUG_MOVE4
   #define debugMove4 sDebugPlatoonTempID=(reinterpret_cast<BPlatoon*>(mpOwner))->getID(), dbgPlatoonInternalTempID
#else
   #define debugMove4 __noop
#endif

#ifdef DEBUG_UPDATESQUADS4
#define debugUpdateSquads4 sDebugPlatoonTempID=(reinterpret_cast<BPlatoon*>(mpOwner))->getID(), dbgPlatoonInternalTempID
#else
#define debugUpdateSquads4 __noop
#endif


const int32 cMaxLLPathingAttemptsAllowed = 3;
const float cRepathNeededDistance = 16.0f;
// We're going to be metering canPath checks for squads that are trying to get
// to platoon positions.  
const int32 cMaxCanPathsPerUpdate = 10;   // No more than n squads can run a canPath on any given update..
const int32 cUpdatesBetweenCanPaths = 20; // No particular squad can run a canPath more often than n updates.

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BPlatoonActionMove, 5, &gSimHeap);

//==============================================================================
// connect
// I'm not really sure.  "connects" the action to the entity perhaps?
//==============================================================================
bool BPlatoonActionMove::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
      return(false);

   
   return(true);
}

//==============================================================================
// disconnect.  And.. detaches the action from the entity.  
//==============================================================================
void BPlatoonActionMove::disconnect()
{
   // DLM - 10/8/08
   // Remove actions for children squads..
   for (long m = 0; m < mSquadActionList.getNumber(); m++)
   {
      BSquad *pSquad = reinterpret_cast<BSquad *>(gWorld->getEntity(mSquadActionList[m].mSquad));
      if (pSquad)
      {
         long nNumActions = pSquad->getNumberActions();
         for (long n = 0; n < nNumActions; n++)
         {
            BAction *pAction = pSquad->getActionByIndex(n);
            if (pAction && pAction->getType() == BAction::cActionTypeSquadMove && pAction->getState() != BAction::cStatePaused)
            {
               BSquadActionMove *pMoveAction = reinterpret_cast<BSquadActionMove *>(pAction);
               if (pMoveAction && pMoveAction->getParentAction() == this)
               {
                  pMoveAction->setParentAction(NULL);
               }
               break;
            }
         }
      }
   }

   BAction::disconnect();
}

//==============================================================================
// init
//==============================================================================
bool BPlatoonActionMove::init()
{
   if (!BAction::init())
      return(false);

   mLLPathingAttempts = 0;
   mSquadActionList.clear();
   mUnpauseState = cStateNone;
   mPauseTimeRemaining = 0;
   mOrigTargetPos = cInvalidVector;
   mpParentAction = NULL;
   return (true);
}

//==============================================================================
// isInterruptable
//==============================================================================
bool BPlatoonActionMove::isInterruptible() const
{
   return true;
}

//==============================================================================
// Set the state
//==============================================================================
bool BPlatoonActionMove::setState(BActionState state)
{
   switch (state)
   {
      case cStatePaused:
         {
            if (mState != cStatePaused)
               mUnpauseState = mState;
         }
         break;

      case cStateUnpaused:
         {
            setState(mUnpauseState);
            mUnpauseState = cStateNone;
            return (true);
         }
         break;

      case cStateDone:
      case cStateFailed:
         //Notify our parent, if any.
         if (mpParentAction)
         {
            if (state == cStateDone)
            {
               mpParentAction->notify(BEntity::cEventActionDone, mpOwner->getID(), getID(), 0);
            }
            else
            {
               mpParentAction->notify(BEntity::cEventActionFailed, mpOwner->getID(), getID(), 0);
            }
         }
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
// update
//==============================================================================
bool BPlatoonActionMove::update(float elapsedTime)
{
   BPlatoon* pPlatoon = reinterpret_cast<BPlatoon*>(mpOwner);
   BASSERT(pPlatoon);

   // DLM 6/17/08 - If the platoon no longer has any children, remove all orders, so that
   // updatePreAsync will destroy it.     
   if (pPlatoon->getNumberChildren() == 0)
   {
      debugMove4("update -- Platoon no longer has any children.  Removing all orders and returning false.");
      pPlatoon->removeAllOrders();
      return false;
   }
   

   // Because I have to perpetuate the brutal hack that is playblockinganimation, if any of our squads
   // are in a playblockinganimation flag (which should be a state, wtf?) then just burn cycles until
   // they all come out of it.
   for (uint n = 0; n < pPlatoon->getNumberChildren(); n++)
   {
      BEntityID childID = pPlatoon->getChild(n);
      BSquad *pSquad = reinterpret_cast<BSquad*>(gWorld->getEntity(childID));
      if (pSquad && pSquad->getFlagPlayingBlockingAnimation())
      {
         debugMove4("update -- squad %d has a blocking animation", pSquad->getID());
         return true;
      }
   }

   switch (mState)
   {
      case cStateNone:
      {
         // Reset our LLPathingAttempts (low-level) count.  This is done whenever
         // we initially go into the working state, or whenever we successfully
         // retrieve a path.  
         mLLPathingAttempts = 0;

         // If no squads can move then end the platoon move action
         bool canAnySquadsMove = false;
         for (uint i = 0; i < pPlatoon->getNumberChildren(); i++)
         {
            BSquad *pSquad = gWorld->getSquad(pPlatoon->getChild(i));
            if (pSquad && pSquad->canMoveForOrder(mpOrder))
            {
               canAnySquadsMove = true;
               break;
            }
         }
         if (!canAnySquadsMove)
         {
            debugMove4("update -- no squads can move");
            setState(cStateFailed);
            return true;
         }
         // If we've been reset, then we should have a high level path to work from, and a target.  
         // Find the high level path waypoint just outside the low level pathing distance from the target.
         // this is the waypoin we'll use to activate squad plats. 
         if (pPlatoon && mTarget.isIDValid())
         {
            pPlatoon->setSquadPlotWaypoint();
         }

         setState(cStateWorking);
         break;
      }
      case cStateWorking:
      {
         BActionState newState = doWorkUpdate(elapsedTime);
         // After updating the platoon, if we're going to a new state,
         // do that.  
         if (newState != cStateWorking)
            setState(newState);
         break;
      }
      case cStatePaused:
      {
         // Query platoon's children for no paused move actions
         bool unPause = true;
         uint numChildren = pPlatoon->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            const BSquad* pSquad = gWorld->getSquad(pPlatoon->getChild(i));
            if (pSquad)
            {
               const BSquadActionMove* pSquadActionMove = reinterpret_cast<const BSquadActionMove*>(pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove));
               if (pSquadActionMove && (pSquadActionMove->getState() == cStatePaused))
               {
                  unPause = false;
                  break;                  
               }
            }
         }     

         if (unPause)
         {
            setState(cStateUnpaused);
         }
         return (true);
         break;
      }
      case cStateUnpaused:
      {
         // Spin
         return (true);
         break;
      }
      case cStateWaitOnChildren:
      {
         // We might hijack this state for something else later on, but for now it means
         // I'm waiting on my children to complete their movement. Besides
         // children being finished, we probably also want to put an absolute timer on this as well.
         bool bChildrenFinished = checkForChildrenComplete();
         if (bChildrenFinished)
         {
            debugMove4("update -- Platoon is in cStateWaitOnChildren and all children are done. Setting State to Done.");
            setState(cStateDone);
         }
         break;
      }
      case cStatePathing:
      {
         BActionState newState = incPathingAttempt();
         setState(newState);
         
         break;
      }
      case cStateAdvanceWP:
      {
         BActionState newState = advanceWP();
         if (newState != cStateAdvanceWP)
            setState(newState);
         break;
      }
      case cStateWait:
      {
         // Convert to milliseconds elapsed.
         DWORD elapsedMS = 1000.0f*elapsedTime;

         // Check time remaining.
         if (mPauseTimeRemaining <= (DWORD)elapsedMS)
         {
            // Time's up, back to work.
            // Reset timer to 0 here for good measure (we don't always just subtract because negative = giant unsigned number)
            debugMove4("Coming out of wait state.");
            mPauseTimeRemaining = 0;
            setState(cStateWorking);
         }
         else
         {
            // Update the timer
            mPauseTimeRemaining -= elapsedMS;
            debugMove4("In wait state, time remaining=%d.", mPauseTimeRemaining);
         }
         break;
      }
      default:
         break;
   }

   // DLM 5/9/8 - I think I want to update the squad's actions *every* update. 
   updateSquadActions();

   //Done.
   return(true);
}

//==============================================================================
// doWorkUpdate(float elapsedTime)
// this is where most of the real work in update is going to be done.  
// I'm sure this will grow.
// The end result of this is that we should have done a basic movement update.
// If we detect we need to move to a different state, then return the new
// state.  
//==============================================================================
BActionState BPlatoonActionMove::doWorkUpdate(float elapsedTime)
{

#ifndef _MOVE4
   elapsedTime;
   return cStateDone;
#else
   BPlatoon* pPlatoon = reinterpret_cast<BPlatoon*>(mpOwner);
   BASSERT(pPlatoon);
   // Despite the BASSERT, we should still do a sanity check here
   if (!pPlatoon)
      return cStateFailed;
   
   // If we're "close" to the target, (and we have a target) then compute valid plots.  Don't wait until we're "inRange" only, becuase
   // this is way too late to compute plot points for melee units.
   // DLM 10/7/08 - I'm removing this pre-check, and allowing the "inRangeofTarget" check below to handle the entire case.  This fixes 
   // several instances of guys not following their high level paths to their destination correctly, as well as allowing the platoon
   // to consider itself "done" as soon as it computes plots for the squads.
   /*
   if (mTarget.isIDValid() && !pPlatoon->hasValidPlots_4() && pPlatoon->inRangeOfTarget_4()) //pPlatoon->getCurrentHLWP_4() >= pPlatoon->getSquadPlotWaypoint())
   {
      BEntity *pEntity = gWorld->getEntity(mTarget.getID());
      if (pEntity)
      {
         BVector vTargetPos = pEntity->getPosition();
         float fDistSqr = vTargetPos.xzDistanceSqr(pPlatoon->getPosition());
         // DLM - 9/30/08 - We've increased this distance to go from .25 to .5 of short range limit dist
         // to accomodate some of our higher range units in the game -- aka vulture.  
         // DLM - 9/30/08 - We also don't check this until we're past the prerequesite waypoint..
         if (fDistSqr < (BPather::cShortRangeLimitDist * 0.5f))
         {
            debugMove4("BPlatoonActionMove::doWorkUpdate -- less than half the short range limit dist to target.. computing SquadPlots...");
            pPlatoon->computePlots_4();
         }
      }
   }
   */

   // jce [4/16/2008] -- If we've gotten into range of our target entity/point, we can just be done now.
   if(pPlatoon->inRangeOfTarget_4())
   {
      // Besides signaling we're done, we want to compute Squad Plots if we have a valid target. (and we haven't done so yet)
      if (mTarget.isIDValid() && !pPlatoon->hasValidPlots_4())
      {
         debugMove4("BPlatoonActionMove::doWorkUpdate -- in range of target, computing SquadPlots...");
         pPlatoon->computePlots_4();
      }
      debugMove4("BPlatoonActionMove::doWorkUpdate -- in range of target, so we're done.");
      // DLM - we no longer call ourselves done here.  The platoon continues along it's path until it
      // either reached the end of our path or until *all* our children are "Done".
      //return(cStateWaitOnChildren);
   }

   // More top of update processing.  See if target has moved substantially.  If so, we need to abandon our current
   // path, and repath at the high level path level. 
   if (mTarget.isIDValid() && (mOrigTargetPos != cInvalidVector) && pPlatoon != NULL)
   {
      BEntity *pEntity = gWorld->getEntity(mTarget.getID());
      if (pEntity)
      {
         float fDist = pEntity->getPosition().xzDistance(mOrigTargetPos);
         if (fDist > cRepathNeededDistance && pEntity->isVisible(pPlatoon->getPlayer()->getTeamID()))
         {
            debugMove4("BPlatoonActionMove::doWorkUpdate -- The target has moved substantially.. we're going to repath..");
            int32 pathResult = BPath::cFailed;
            bool bPathRun = pPlatoon->repathToTarget_4(pathResult);
            if (bPathRun)
            {
               // Update the  Target "Original" position
               mOrigTargetPos = pEntity->getPosition();
               if (pathResult == BPath::cInRangeAtStart || pathResult == BPath::cFailed)
               {
                  debugMove4("BPlatoonActionMove::doWorkUpdate -- repath returned cStateFailed, going into cStateWaitingOnChildren..");
                  return cStateWaitOnChildren;
               }
               // Otherwise, the paths have been reset, the new path is run.. return so we can start moving towards that..
               debugMove4("BPlatoonActionMove::doWorkUpdate -- repath didn't fail, returning now with cStateWorking..");
               return cStateWorking;
            }
            // if we couldn't run the path, just continu on like nothing happened.. maybe we can run the path next update.
            debugMove4("BPlatoonActionMove::doWorkUpdate -- Unable to run path (out of pathing attempts?), continueing (try again next udpate)..");
         }
      }
   }
   
   // See if we have a current low level path.  If we don't, then path to the next waypoint and we're done. 
   bool wantLLReset = false;
   const BPath &llp = pPlatoon->getCurrentLLPath_4();
   if (llp.getNumberWaypoints() == 0)
   {
      debugMove4("BPlatoonActionMove::doWorkUpdate -- no low-level path, trying to get one.");
      
      // Increment our pathing attempts.
      BActionState newState = incPathingAttempt();
      debugMove4("  incPathingAttempt gave us state %d", newState);
      
      // If we get a state other than working back from that, just get out now, returning that state to the caller.
      if(newState != cStateWorking)
      {
         debugMove4("  not going to work state, so bail", newState);
         return(newState);
      }
      
      int32 pathResult = BPath::cFailed;
      bool bPathAttempt = pPlatoon->pathToNextHLWP_4(pathResult);
      // If we couldn't path, path limiter choked us.  Just leave us in the working state, and try again next update. 
      if (bPathAttempt == false)
         return cStateWorking;

      // If we fail the path, go into pathing state, which will increment our path count and let us try again, if allowed.
      // jce [4/16/2008] -- we should follow a partial low level path from BPath::cOutsideHulledAreaFailed so as to get a close as possible to our goal even if we
      // aren't going to make it all the way.
      if (pathResult == BPath::cFailed /*|| pathResult == BPath::cOutsideHulledAreaFailed*/)
      {
         // jce [4/16/2008] -- maybe overly brute force here, but get a new HL path as well from our new somewhat-stuck position.
         // dlm 5/1/8 - don't do this.  It puts us in an endless loop of the low level path saying it can't get there and the high level path saying 
         // yes you can. If we're stuck we're stuck.
         // jce [9/22/2008] -- trying to put this back on now that the low-level path counter is working differently.  We theorize some ping-pong type
         // effects might be possible though...

         debugMove4("BPlatoonActionMove::doWorkUpdate -- repath to current high-level waypoint.");
         bPathAttempt = pPlatoon->pathToNextUserWP_4(pathResult);
         if(!bPathAttempt)
            return(cStateWorking);
         
         // See if we got some kind of valid path back (full/partial)
         if(pathResult == BPath::cFull || pathResult == BPath::cPartial || pathResult == BPath::cOutsideHulledAreaFailed || pathResult == BPath::cInRangeAtStart)
         {
            // Move to next waypoint (from invalid 0th) so we can start following it.
            debugMove4("BPlatoonActionMove::doWorkUpdate -- starting to follow new high level path.");
            BActionState adjustedState;
            pPlatoon->advanceHLWP_4(adjustedState);
         }
         else
         {
            // We failed to get a HLP, so just fail out now.
            debugMove4("BPlatoonActionMove::doWorkUpdate -- our high-level path completely failed, so just failing out.");
            return(cStateFailed);
         }
         
         // This is the version that just pauses US, not the squads which hopefully are figuring things out on their own.
         // jce [9/18/2008] -- pause in this case instead of immediately repathing since we just failed a path.
         pPlatoon->resetLLP_4();
         pauseMovement(1000);
         return cStateWait;
      }

      if (pathResult == BPath::cInRangeAtStart)
      {
         debugMove4("BPlatoonActionMove::doWorkUpdate -- we were in range at start for next HLWP.");
         return cStateAdvanceWP;
      }
      
      // All Others (Success, Partial, FailedOutsideHullAreaPartial), take the path and set our current
      // LLWP, and let's go. 
      debugMove4("BPlatoonActionMove::doWorkUpdate -- proceeding to next HLWP.");
      
      // Only reset pathingAttempts if we got a FULL, actual path.
      // jce [9/18/2008] -- actually, reset if we've made any progress like squads do
      // jce [9/23/2008] -- However, only do this if we make it successfully past advanceWP() below since otherwise
      // we might not be getting close enough to the HL wp and needing to repath
      wantLLReset = true;
      //if (pathResult == BPath::cFull)
      //   mLLPathingAttempts = 0;
      
      pPlatoon->initLLWP_4();
      // DLM 4/22/08 -- no reason to return here right away.  No since in not doing
      // our first update of movement as well.
      //return cStateWorking;
   }

   // Check to see if we're within advance range of our next LLWP.
   // jce [4/16/2008] -- check HL too since we might skip ahead.
   if (pPlatoon->inLLWPRange_4() || pPlatoon->inHLWPRange_4())
   {
      debugMove4("BPlatoonActionMove::doWorkUpdate -- close enough to next waypoint, advancing.");
      BActionState newState = advanceWP();
      if (newState != cStateWorking)
      {
         if (newState == cStateWaitOnChildren)
         {
            // DLM 10/30/08 - this code needs to do what we were doing up at the top too.  That is, if we detect we're done here,
            // and we haven't plotted spots yet, then plot some spots. 
            if (mTarget.isIDValid() && !pPlatoon->hasValidPlots_4())
            {
               debugMove4("BPlatoonActionMove::doWorkUpdate -- in range of target, computing SquadPlots...");
               pPlatoon->computePlots_4();
            }
         }

         return newState;
      }
   }
   
   // jce [9/23/2008] -- If we wanted to reset the LLP attempts and we successfully got past the advanceWP() without wanting to
   // repath again, then go ahead and reset.
   if(wantLLReset)
      mLLPathingAttempts = 0;
      
   //   return cStateAdvanceWP;
   
   // Get velocity factor which will slow us down when squads lag too far behind.
   // DLM - turning this back on, to try to keep platoons from running so far away from their squads.  
   // adjusted the factor up a bit, to prevent slowdown.
   float velocityFactor = computeVelocityFactor();
   //float velocityFactor = 1.0f;

   // Advance our Positions
   bool movedFully = pPlatoon->advancePosition_4(elapsedTime, velocityFactor);
   // Advancing position might have caused us to go into wait state.. only go to state pathing if we're
   // still working, but collided with something.
   if(!movedFully && mState == cStateWorking)
   {
      // We collided with something.  Re-path.
      debugMove4("BPlatoonActionMove::doWorkUpdate -- failed to move fully, re-pathing.");
      pPlatoon->resetLLP_4();
      return(cStatePathing);
   }

   // Tell the platoon to update it's formations..

   static bool usePath = false;
   if(usePath)
   {
      const BPath &path = pPlatoon->getCurrentLLPath_4();
      pPlatoon->updateFormation(&path);
   }
   else
      pPlatoon->updateFormation();

   // Continue to work
   return cStateWorking;
#endif
}

//==============================================================================
// advanceWP
// Advances the Waypoint, and looks up each level of pathing to see if the
// next WP should advance as well, setting states appropriately.
// bForce -- allows us to advance a HLWP even if we're not close enough.
//==============================================================================
BActionState BPlatoonActionMove::advanceWP(bool bForce)
{
#ifndef _MOVE4
   return cStateDone;
#else
   // Last LLWP?
   BPlatoon* pPlatoon = reinterpret_cast<BPlatoon*>(mpOwner);
   BASSERT(pPlatoon);
   // Despite the BASSERT, we should still do a sanity check here
   if (!pPlatoon)
      return cStateFailed;

   // Get the current llp
   const BPath &llp = pPlatoon->getCurrentLLPath_4();
   // Sanity check on the llp.. 
   // DLM - Removed this.  It's possible if we return InRangeAtStart for us to not have any llp waypoints, but still
   // want to advance our waypoint..
   /*
   if (llp.getNumberWaypoints() == 0)
   {
      debugMove4("BPlatoonActionMove::advanceWP -- no low level waypoints, failing.");
      return cStateFailed;
   }
   */

   // We're only here because we know it's time to advance the LLWP.  Bubble up the paths
   // and see if we're done.  
   if ((pPlatoon->getCurrentLLWP_4() == llp.getNumberWaypoints()-1) || (pPlatoon->getCurrentLLWP_4() == 0 && llp.getNumberWaypoints() == 0))
   {
      // Within Range of HLWP?
      float fDist = 0.0f;
      if (pPlatoon->inHLWPRange_4(fDist) || bForce)
      {
         debugMove4("BPlatoonActionMove::advanceWP -- we're in range of our HLWP.");

         // Are we at the end of our HLP?
         const BPath &hlp = pPlatoon->getCurrentHLPath_4();
         if (hlp.getNumberWaypoints() == 0)
         {
            debugMove4("BPlatoonActionMove::advanceWP -- no high level waypoints, failing.");
            return cStateFailed;
         }
         
         if (pPlatoon->getCurrentHLWP_4() == hlp.getNumberWaypoints()-1)
         {
            debugMove4("BPlatoonActionMove::advanceWP -- this is the last high level waypoint.");

            if (pPlatoon->inUserWPRange_4())
            {
               debugMove4("BPlatoonActionMove::advanceWP -- we're in range of our user waypoint.");
         
               const BPath &userPath = pPlatoon->getCurrentUserPath_4();
               if (userPath.getNumberWaypoints() == 0)
               {
                  debugMove4("BPlatoonActionMove::advanceWP -- user path had no waypoints.");
                  return cStateFailed;
               }
                  
               // Are we at the end of our UserPath?
               if (pPlatoon->getCurrentUserWP_4() == userPath.getNumberWaypoints() - 1)
               {
                  debugMove4("BPlatoonActionMove::advanceWP -- at our last user waypoint, so we're done.");

                  // We're done - just need to make sure all the children are done..
                  return cStateWaitOnChildren;
               }
               // No?  Advance the UserWaypoint.  This advances the waypoint, and runs
               // the high level path to that waypoint.  If we can't advance, we fail
               // now.  Otherwise, we go back to work.  
               if (!pPlatoon->advanceUserWP_4())
               {
                  debugMove4("BPlatoonActionMove::advanceWP -- could not advance user WP, failing.");
                  return cStateWaitOnChildren;
               }

               debugMove4("BPlatoonActionMove::advanceWP -- advanced user WP, moving on.");
               
               // We succeeded.
               return cStateWorking;
            }
            // DLM 5/30/08 - Even if we're not in range of the User WP, if we're at the end of our HLP, advance the User WP.  If we're at the
            // end of the User WP, we'll fail.             
            if (!pPlatoon->advanceUserWP_4())
            {
               debugMove4("BPlatoonActionMove::advanceWP -- could not advance user WP, failing.");
               return cStateWaitOnChildren;
            }
            return cStateWorking;
            
            /*
            // If we weren't in range of the user WP, but at the end of our HL rope,
            // we fail.  Don't repath, because it's just a HLP, and the likelihood of anything
            // changing at the HL scene is very small.
            debugMove4("BPlatoonActionMove::advanceWP -- end of HLP, not close enough to user waypoint, failing.");
            return cStateFailed;
            */
         
         }
         // Advance to the next HLP. 
         BActionState adjustedState;
         if (!pPlatoon->advanceHLWP_4(adjustedState))
         {
            debugMove4("BPlatoonActionMove::advanceWP -- could not advance HLWP, failing.");
            return cStateWaitOnChildren;
         }
         // Whenever we advance to the HLWP, reset our LLPathingAttempts
         if (adjustedState == cStateWorking)
            mLLPathingAttempts = 0;
         return adjustedState;
      }
      // If we're at the end of our LLP, but not within range of our next HLP,
      // clear our path, and put ourselves in cPathing, which will flag the pathingAttempts marker.
      debugMove4("BPlatoonActionMove::advanceWP -- reached the end of our LLP, but not close enough to HLWP.  Re-path. (%f)", fDist);
      pPlatoon->resetLLP_4();
      return cStatePathing;
   }
   // If we're not at the last LLWP, then just advance to the next one
   if (!pPlatoon->advanceLLWP_4())
   {
      debugMove4("BPlatoonActionMove::advanceWP -- could not advance low-level waypoint, failing.");
      return cStateFailed;
   }
   
   return cStateWorking;
#endif

}

//==============================================================================
// updateSquadMoveActions
// Walks through the list of squad actions assigned to our children.  If
// a child has no new action, assign it a move action with the current formation
// position as its destination.  If a child's current action is the one we
// gave it, then update it's destination with the updated formation position.
// If the child is doing some other action, let it handle that.  
//==============================================================================
void BPlatoonActionMove::updateSquadActions()
{
#ifndef _MOVE4
   return;
#else
   BPlatoon* pPlatoon = reinterpret_cast<BPlatoon*>(mpOwner);
   BASSERT(pPlatoon);
   // Despite the BASSERT, we should still do a sanity check here
   if (!pPlatoon)
      return;

   debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Evaluating squads...");
   debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Number of squads in Action List: %d", mSquadActionList.getNumber());
   debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Actual No. of Children Squads:   %d", pPlatoon->getNumberChildren());

   const BEntityIDArray &childList = pPlatoon->getChildList();
   long n = 0;
   long m = 0;
   for (n = 0; n < mSquadActionList.getNumber(); n++)
      mSquadActionList[n].mRefreshed = false;
   long nCanPathsThisUpdate = 0;
   for (n = 0; n < childList.getNumber(); n++)
   {
      // Try to find the squad in our list..
      bool bFound = false;
      for (m = 0; m < mSquadActionList.getNumber(); m++)
      {
         if (mSquadActionList[m].mSquad == childList[n])
         {
            bFound = true;
            break;
         }
      }
      BSquad *pSquad = reinterpret_cast<BSquad*>(gWorld->getEntity(childList[n]));

      // Sanity check here..
      if (!pSquad || pSquad->getNumberChildren() == 0)
         continue;

      // Get the squad's destination..
      debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- looking at squad: %d", childList[n].asLong());
      syncPlatoonData("BPlatoonActionMove::updateSquadActions -- looking at squad: ", childList[n].asLong());
      // Give the squad the platoon's target location, and an interim target location, which is 
      // simply the platoon's desired formation position for the squad on a per update basis.
      BVector squadInterimTarget;
      BSimTarget squadTarget = mTarget;

      // DLM 11/11/08 - Update the squadTarget with the appropriate range for this squad. This is important, as this
      // is the range that will be used with the generated unit actions for this squad, and should jive with what the squad
      // (and the platoon) think are the right range for this squad.
      float fSquadRange = pPlatoon->getSquadRange(m);
      if (fSquadRange > -cFloatCompareEpsilon)
         squadTarget.setRange(fSquadRange);
      // TRB 12/4/08 - If squad range not valid then don't pass along platoon range either.  This could be a problem for heterogeneous
      // platoons where some of the squads are just moving.  We don't want the movers to inherit the range from another squad.
      else
         squadTarget.invalidateRange();

      // DLM 10/30/08 - Get the formation position now.  
      const BFormation2 *pFormation = pPlatoon->getFormation();
      if (!pFormation)
         break;
      const BFormationPosition2 *formationPosition = pFormation->getFormationPosition(childList[n]);
      if (!formationPosition)
         break;               

      // Unless the platoon has calculated squad positions, then use that.
      bool bProceedingToPlot = false;
      BPath tempPath;
      if (pPlatoon->hasValidPlots_4())
      {
         bool bRetrieved = pPlatoon->getPlottedPosition_4(childList[n], squadInterimTarget);
         if (!bRetrieved)
         {
            syncPlatoonCode("platoonActionMove -- breaking out due to no plotted position even though hasValidPlots_4 is true");
            break;
         }
         // Don't run this canPath check if we're already done.
         if (bFound && mSquadActionList[m].mState == BSquadActionEntry::cActionEntryStateDone)
         {
            debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Have Valid Plots, & Squad %d found in list, But is DONE or already going to target.  Skipping Evaluation.",
               mSquadActionList[m].mSquad);
            mSquadActionList[m].mRefreshed = true;
            continue;
         }
         // DLM 10/30/08 - Run a canPath to the squad's plotted location.  IF the squad can't get to it,
         // then tell the squad to continue to move towards it's formation location..         
         // DLM 11/3/08 - Do some metering of how often we run canpath's..
         bool bOkayToCanPath = false;
         
         if (bFound && mSquadActionList[m].mState == BSquadActionEntry::cActionEntryStateWorking && nCanPathsThisUpdate < cMaxCanPathsPerUpdate)
         {
            DWORD currentUpdate = gWorld->getUpdateNumber();
            if ((currentUpdate - mSquadActionList[m].mLastCanPathUpdate) > cUpdatesBetweenCanPaths)
            {
               bOkayToCanPath = true;
               mSquadActionList[m].mLastCanPathUpdate = currentUpdate;
               ++nCanPathsThisUpdate;
            }
         }
         
         if (bOkayToCanPath)
         {
            long findPathResult = BPath::cFailed;
            int nCanPath = pSquad->getMoveActionPath_4(pSquad->getPosition(), squadInterimTarget, findPathResult, &tempPath);
            if (nCanPath != BSquadActionMove::cFindPathOk || findPathResult != BPath::cFull)
            {
               squadInterimTarget = formationPosition->getPosition();
               debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- platoonActionMove - We have plotted Positions, but can'tPath to plotted position, so using formation position: (%5.2f, %5.2f, %5.2f)", squadInterimTarget.x,
                  squadInterimTarget.y, squadInterimTarget.z);
               syncPlatoonData("platoonActionMove -- Had plotted positions, but used formation position because we couldn't path to plotted position:", squadInterimTarget);
            }
            else
            {
               debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- canPath succeeded to plotted Position.. using that as squad action target.");
               syncPlatoonData("platoonActionMove -- squadInterimTarget set by Plotted position:", squadInterimTarget);
               bProceedingToPlot = true;
            }
         }
         else
         {
            // If we couldn't canPath, but we're still in state working, then continue to head towards formation position..
            if (bFound && mSquadActionList[m].mState == BSquadActionEntry::cActionEntryStateWorking)
            {
               // If we can't run a canpath this update, then continue to follow the platoon..
               squadInterimTarget = formationPosition->getPosition();
               syncPlatoonData("platoonActionMove -- squadInterimTarget set by formation position:", squadInterimTarget);
            }
         }
         
         // TRB 10/2/08 - Mark squads as done if they've reached their plotted position.  This allows squads that
         // reach their positions to move on to attacking even if their platoon mates are still moving.
         if (bFound && pPlatoon->isSquadAtTarget_4(childList[n]))
            mSquadActionList[m].mState = BSquadActionEntry::cActionEntryStateDone;
      }
      else
      {
         squadInterimTarget = formationPosition->getPosition();
         syncPlatoonData("platoonActionMove -- squadInterimTarget set by formation position:", squadInterimTarget);
      }
      if (bFound)
      {
         debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Entity found in ActionList.", childList[n].asLong());
         // If we found the entry, but the squad no longer exists or is invalid, then remove the entry.
         if (!pSquad)
         {
            debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- But squad doesn't exist, so removing from ActionList.", childList[n].asLong());
            mSquadActionList.eraseUnordered(m);
            continue;
         }

         // If Squad is done, skip evaluation
         if (mSquadActionList[m].mState == BSquadActionEntry::cActionEntryStateDone)
         {
            debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Squad %d found in list, But is DONE.  Skipping Evaluation.",
               mSquadActionList[m].mSquad);
            mSquadActionList[m].mRefreshed = true;
            continue;
         }

         // Are we working?  If so, the squad will need an update.  
         if (mState == cStateWorking || mState == cStateWaitOnChildren || mState == cStateWait)
         {
            // See if the squad still has the action we gave it.. 
            BSquadActionMove *pMoveAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
            if (pMoveAction)
            {
               debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Move Action ID (%d) found for squad.", pMoveAction->getID());
               // Update the squad's interim target
               if (pMoveAction->getID() == mSquadActionList[m].mActionID)
               {
                  debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- This is the action we have recorded for this squad.  Updating its interimTarget.");
                  pMoveAction->setInterimTarget_4(squadInterimTarget);
                  if (bProceedingToPlot)
                  {
                     debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Setting squad as ProceedingToPlot..");
                     mSquadActionList[m].mState = BSquadActionEntry::cActionEntryStateProceedingToPlot;
                     pSquad->setMoveActionPath_4(tempPath);
                  }
                  mSquadActionList[m].mRefreshed = true;
                  continue;
               }
               else
               {
                  // if it had a move action, but it's not the one we gave it, then continue, and we'll update it next time. 
                  debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- The move action doesn't not match.  Skipping.");
                  mSquadActionList[m].mRefreshed = true;
                  continue;
               }
            }
            // If the squad doesn't have a move action, and we are still working, then we should create a move action. 
            // But only if we're still working.  Once we reach state wait on children, stop reassigning actions..
            if (mState == cStateWorking  || mState == cStateWait)
            {
               debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Squad %d found in list, but has no move action.  Calling doMove to add a move.",
                  mSquadActionList[m].mSquad);

               mSquadActionList[m].mActionID = pSquad->doMove_4(mpOrder, this, squadTarget, squadInterimTarget, true, true, true, true);
               if (mSquadActionList[m].mActionID == cInvalidActionID)
               {
                  debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- UNABLE TO CREATE ACTION!!");
                  continue;
               }
               debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Action created and recorded.");
            }
            mSquadActionList[m].mState = BSquadActionEntry::cActionEntryStateWorking;
            mSquadActionList[m].mRefreshed = true;
            continue;
         }
         // If we are not working, then as soon as any of our children don't have an action, or are waiting on children
         // themselves, mark them as done.  
         debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Platoon not working...");
         BSquadActionMove *pMoveAction2 = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
         if (pMoveAction2)
         {
            if (pMoveAction2->getState() == cStateWaitOnChildren || pMoveAction2->getState() == cStateDone)
            {
               debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Squad %d done or waiting on children, so setting is status to mDone.",
                  mSquadActionList[m].mSquad);
               mSquadActionList[m].mState = BSquadActionEntry::cActionEntryStateDone;
            }
         }
         else
         {
            debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Squad %d no longer has a move action, so setting is status to mDone.",
              mSquadActionList[m].mSquad);
            mSquadActionList[m].mState = BSquadActionEntry::cActionEntryStateDone;
         }
         // Either way, the squad has been examined, move on.  
         mSquadActionList[m].mRefreshed = true;
         continue;
      }

      // If we got to here, the squad is not yet in our action list. 
      // if the squad wasn't found, move on..
      BASSERT(pSquad);
      if (!pSquad)
         continue;

      // If we didn't find the squad in our list, then give it a move order, and add it to our list.
      // DLM 5/19/08 - If we're in work or wait state, then generate a move order for our child if we didn't find
      // him in the list at all. 
      if (mState == cStateWorking || mState == cStateWaitOnChildren || mState == cStateWait)
      {
         BSquadActionEntry entry;
         entry.mSquad = childList[n];

         debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Squad %d not found in list, calling doMove to add a move and recording it in our list.",
            entry.mSquad);

         // TRB 6/18/08 -  Don't add the squad to the action list if it already has a move action that can't be interrupted.
         // Once the uninterruptible move action finishes, a new move action will be created for the squad and it will be added to the list.
         // This fixes a problem with units that can change mode before or after moving.
         BSquadActionMove *pMoveAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
         if (pMoveAction && !pMoveAction->isInterruptible())
            continue;

         // TRB 7/29/08 - If the squad already has a move action tied to this order, then the squad has merged this order with an
         // existing one.  The platoon should use the existing move action since notifications may have already been set up between
         // the move action and other actions like a gather or attack action.  Fixing up all these notifications is trickier than
         // just using the existing move action here.
         if (pMoveAction && (pMoveAction->getOrder() == mpOrder))
            entry.mActionID = pMoveAction->getID();
         else
            entry.mActionID = pSquad->doMove_4(mpOrder, this, squadTarget, squadInterimTarget, true, true, true, true);

         if (entry.mActionID == cInvalidActionID)
         {
            debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- UNABLE TO CREATE MOVE ACTION!!!");
            continue;
         }
         entry.mRefreshed = true;
         entry.mState = BSquadActionEntry::cActionEntryStateWorking;
         entry.mLastCanPathUpdate = 0;
         mSquadActionList.pushBack(entry);         
         debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Action created and recorded.");
      }
   }

   // After walking through our squad list, remove any actionentries that didn't get refreshed. 
   // DLM 7/22/08 - Note, we're back to only removing the *action entries* from the action list
   // for any that didn't get updated -- meaning the squad has been removed from the platoon.
   // We no longer attempt to remove squads from platoons that are done. 
   for (m = 0; m < mSquadActionList.getNumber(); m++)
   {
      if (!mSquadActionList[m].mRefreshed)
      {
         debugUpdateSquads4("BPlatoonActionMove::updateSquadActions -- Squad %d not refreshed this update.  ActionList Entry removed.", mSquadActionList[m].mSquad.asLong());
         mSquadActionList.erase(m);
         m--;
      }
   }

#endif
}

//==============================================================================
// update
//==============================================================================
bool BPlatoonActionMove::checkForChildrenComplete()
{
   #ifdef _MOVE4
   BPlatoon* pPlatoon = reinterpret_cast<BPlatoon*>(mpOwner);
   BASSERT(pPlatoon);
   // Despite the BASSERT, we should still do a sanity check here
   if (!pPlatoon)
      return false;
   const BEntityIDArray &childList = pPlatoon->getChildList();
   for (long n = 0; n < childList.getNumber(); n++)
   {
      // Check to see if it has a move action..
      BSquad *pSquad = reinterpret_cast<BSquad*>(gWorld->getEntity(childList[n]));
      if (!pSquad)
         continue;
      BSquadActionMove *pMoveAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
      if (pMoveAction)
      {
         // IF so then is it at its destination yet..
         bool bComplete = pPlatoon->isSquadAtTarget_4(childList[n]);
         if (!bComplete)
         {
            return false;
         }
      }
   }
   #endif
   return true;
}


//==============================================================================
//==============================================================================
void BPlatoonActionMove::debug(char* v, ... ) const
{
   #ifndef BUILD_FINAL
   static char out[BLogManager::cMaxLogTextLength];
   va_list va;
   va_start(va, v);
   bvsnprintf(out, sizeof(out), v, va);
   static char out2[BLogManager::cMaxLogTextLength*2];

   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   bsnprintf(out2, sizeof(out2), "SQUAD ID#%5d: %s", pSquad->getID(), out);
   gConsole.output(cChannelSim, out2);
   #endif
   return;
}


//==============================================================================
//==============================================================================
float BPlatoonActionMove::computeVelocityFactor() const
{
   float factor = 0.0f;
   #ifdef _MOVE4
   // Get platoon.
   const BPlatoon* pPlatoon = reinterpret_cast<BPlatoon*>(mpOwner);
   BASSERT(pPlatoon);
   // Despite the BASSERT, we should still do a sanity check here
   if (!pPlatoon)
      return(1.0f);
   
   // Get formation.
   const BFormation2 *formation = pPlatoon->getFormation();
   if(!formation)
      return(1.0f);

   // Run through and compare formation positions to the squads' actual positions.
   float deltaMax = 0.0f;
   float deltaMin = cMaximumFloat;
   uint pausedCount = 0;
   uint listCount = mSquadActionList.getNumber();
   bool bDone = false;
   for(uint i=0; i<formation->getNumberPositions(); i++)
   {
      // Get formation position.
      const BFormationPosition2 *formationPosition = formation->getFormationPosition(i);
      if(!formationPosition)
         continue;

      // Don't allow done squads to hold us up..
      bDone = false;
      BEntityID entityID = formationPosition->getEntityID();
      for (uint n = 0; n < listCount; n++)
      {
         if (entityID == mSquadActionList[n].mSquad)
         {
            if (mSquadActionList[n].mState != BSquadActionEntry::cActionEntryStateWorking)
               bDone = true;
            break;
         }
      }
      if (bDone)
         continue;
        
      // Get entity (squad).
      const BEntity *entity = gWorld->getEntity(formationPosition->getEntityID());
      if(!entity)
         continue;
         
      // This should be a squad, but check for sanity's sake before casting.
      if(entity->getClassType() != BEntity::cClassTypeSquad)
         continue;

      // Cast this as a squad.
      const BSquad *squad = reinterpret_cast<const BSquad*>(entity);

      // Get position delta.
      //BVector delta = entity->getPosition()-formationPosition->getPosition();
      float deltaDist = entity->getPosition().xzDistance(formationPosition->getPosition());
      
      // Keep maximum.
      if(deltaDist>deltaMax)
         deltaMax = deltaDist;

      // Keep minimum.
      if(deltaDist<deltaMin)
         deltaMin = deltaDist;

      
      // See if it's paused.
      if(squad->isMovementPaused_4())
         pausedCount++;
   }

   // Scale into a slowdown factor based on magic numbers.
   // First, scale between full speed and slowest factor based on how far out the furthest guy is.
   
   // If the best delta is pretty bad, consider a more dramatic slowdown.
   const float cMinCutoff = 15.0f;
   float minFactor = 1.0f;
   if(deltaMin > cMinCutoff)
      minFactor = Math::Lerp(1.03f, 0.0f, Math::fSmoothStep(0.0f, 1.0f, (deltaMin-cMinCutoff)/50.0f));

   // Slow down based on the worst-off guy.
   float maxFactor = Math::Lerp(1.03f, 0.5f, Math::fSmoothStep(0.0f, 1.0f, deltaMax/50.0f));
   
   // Choose whichever distance factor is less.
   float distanceFactor = min(minFactor, maxFactor);

   // Second, figure it by number of paused guys.
   float percentPaused = float(pausedCount) / float(pPlatoon->getNumberChildren());
   
   // jce [9/25/2008] -- slow down to nothing if everyone is paused
   //float pausedFactor = Math::Lerp(1.03f, 0.5f, Math::fSmoothStep(0.0f, 1.0f, percentPaused));
   float pausedFactor = Math::Lerp(1.03f, 0.0f, Math::fSmoothStep(0.0f, 1.0f, percentPaused));
   
   // Pick the slowest of the two.
   factor = min(distanceFactor, pausedFactor);
   #endif
   return(factor);
}

//==============================================================================
//==============================================================================
void BPlatoonActionMove::pauseMovement(DWORD pauseTime)
{
   // Remember the timeout.  However, if we're already paused, don't reduce the timeout that
   // someone else has already asked for (only increase it)
   if(mState != cStateWait || pauseTime>mPauseTimeRemaining)
      mPauseTimeRemaining = pauseTime;

   // Go into wait state.
   setState(cStateWait);

}

//==============================================================================
//==============================================================================
void BPlatoonActionMove::getSquadActionEntry(const BEntityID squad, BSquadActionEntry &entry)
{
   for (long m = 0; m < mSquadActionList.getNumber(); m++)
   {
      if (mSquadActionList[m].mSquad == squad)
      {
         entry = mSquadActionList[m];
         return;
      }
   }
}

//==============================================================================
//==============================================================================
void BPlatoonActionMove::notifyThatSquadWillHandleItsOwnMovement(BEntityID squad)
{
   for (long m = 0; m < mSquadActionList.getNumber(); m++)
   {
      if (mSquadActionList[m].mSquad == squad)
      {
         mSquadActionList[m].mState = BSquadActionEntry::cActionEntryStateDone;
         return;
      }
   }
}

//==============================================================================
//==============================================================================
BActionState BPlatoonActionMove::incPathingAttempt()
{
   // Get platoon.
   BPlatoon* pPlatoon = reinterpret_cast<BPlatoon*>(mpOwner);
   BASSERT(pPlatoon);

   // Increment our pathing attempts, and try to go back to working.  If we've been here
   // three times, then we should fail.  
   mLLPathingAttempts++;
   
   if (mLLPathingAttempts > cMaxLLPathingAttemptsAllowed)
   {
      debugMove4("BPlatoonActionMove::update -- Ran out of low-level pathing attempts");
      debugMove4("BPlatoonActionMove::update -- See if we can advance the waypoint..");
      // Even if we fail, we should wait for the children to get to our location.
      // DLM 8/22/08 - Instead of outright failing, see if we can advance waypoint.  If we're not close enough to 
      // advance, we'll fail anyway.
      //setState(cStateWaitOnChildren);
      // Rather than failing immediately... see if we can advance to the next waypoint.  If not, well then
      // we really are done.
      BActionState tempState = advanceWP();
      if (tempState == cStatePathing)
      {
         // One more attempt.  Look to see if the *next* HLWP (if we have one) is within the Short Range Limit.  If so,
         // then advance anyway. 
         debugMove4("BPlatoonActionMove::update -- could not advance the waypoint, but maybe the next waypoint is close enough?");
         const BPath &hlpath = pPlatoon->getCurrentHLPath_4();
         long hlwp = pPlatoon->getCurrentHLWP_4();
         if (hlwp < hlpath.getNumberWaypoints() - 1)
         {
            const BVector &nextWP = hlpath.getWaypoint(hlwp+1);
            if (pPlatoon->getPosition().xzDistanceSqr(nextWP) < BPather::cShortRangeLimitDist)
            {
               debugMove4("BPlatoonActionMove::update -- We can safely skip to the next waypoint, so we're going to do that anyway.");
               BActionState newState = advanceWP(true);
               if (newState == cStatePathing)
                  return(cStateWaitOnChildren);
               else
                  return(newState);
            }
         }
         // Otherwise, okay we're done
         return(cStateWaitOnChildren);
      }
      else
         return(tempState);
   }

   return(cStateWorking);
}


#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BPlatoonActionMove::getDebugLine(uint index, BSimString &string) const
{
   if (index < 2)
      return (BAction::getDebugLine(index, string));
   switch (index)
   {
   case 2:
      {
         BPlatoon *pPlatoon =reinterpret_cast<BPlatoon *>(mpOwner);
         float platoonVelocity = pPlatoon->getPlatoonVelocity_4();
         float factor = computeVelocityFactor();
         string.format("Platoon Vel: %5.2f, Vel. Factor: %5.2f, Adj. Vel: %5.2f", platoonVelocity, factor, platoonVelocity * factor);
         break;
      }
   }
}
#endif

//==============================================================================
//==============================================================================
bool BPlatoonActionMove::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, int32, mLLPathingAttempts);
   GFWRITECLASSARRAY(pStream, saveType, mSquadActionList, uint8, 200);
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BActionState, mUnpauseState);
   GFWRITEVAR(pStream, DWORD, mPauseTimeRemaining);
   GFWRITEVECTOR(pStream, mOrigTargetPos);
   GFWRITEACTIONPTR(pStream, mpParentAction);
   return true;
}

//==============================================================================
//==============================================================================
bool BPlatoonActionMove::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, int32, mLLPathingAttempts);
   GFREADCLASSARRAY(pStream, saveType, mSquadActionList, uint8, 200);
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BActionState, mUnpauseState);
   GFREADVAR(pStream, DWORD, mPauseTimeRemaining);
   GFREADVECTOR(pStream, mOrigTargetPos);
   if (mGameFileVersion >= 16)
      GFREADACTIONPTR(pStream, mpParentAction);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionEntry::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BEntityID, mSquad);
   GFWRITEVAR(pStream, BActionID, mActionID);
   GFWRITEVAR(pStream, bool, mRefreshed);
   GFWRITEVAR(pStream, int, mState);
   GFWRITEVAR(pStream, DWORD, mLastCanPathUpdate)
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionEntry::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BEntityID, mSquad);
   GFREADVAR(pStream, BActionID, mActionID);
   GFREADVAR(pStream, bool, mRefreshed);
   if (mGameFileVersion < 1)
   {
      bool bDone;
      GFREADVAR(pStream, bool, bDone);
      if (bDone)
         mState = BSquadActionEntry::cActionEntryStateDone;
      else
         mState = BSquadActionEntry::cActionEntryStateWorking;
   }
   else
      GFREADVAR(pStream, int, mState);

   if (mGameFileVersion >= 2)
   {
      GFREADVAR(pStream, DWORD, mLastCanPathUpdate);
   }
   else
      mLastCanPathUpdate = 0;
   return true;
}
