//==============================================================================
// squadactiongarrison.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unitactionmove.h"
#include "squadactiongarrison.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "tactic.h"
#include "triggervar.h"
#include "usermanager.h"
#include "selectionmanager.h"
#include "config.h"
#include "configsgame.h"
#include "user.h"
#include "unitactionhotdrop.h"
#include "actionmanager.h"


#ifndef _MOVE4
#define _MOVE4
#endif


#ifndef BUILD_FINAL
//#define DEBUG_MOVE4
#endif

#ifdef DEBUG_MOVE4
   #define debugMove4 sDebugSquadTempID=(reinterpret_cast<BSquad*>(mpOwner))->getID(), dbgSquadInternalTempID
#else
   #define debugMove4 __noop
#endif

static const uint cMaxTeleporterRepaths = 15;

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionGarrison, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionGarrison::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(pEntity);
   BASSERT(pSquad);

   // Hot drop garrisons where the hotdrop target is the same as the unit we're garrisoning only work for reverse hot drops
   BUnit* pTargetUnit = getTargetUnit();
   if (pTargetUnit)
   {
      BUnitActionHotDrop* pHDA = reinterpret_cast<BUnitActionHotDrop*>(pTargetUnit->getActionByType(BAction::cActionTypeUnitHotDrop));
      if (pHDA && (pHDA->getHotdropTarget() == pSquad->getLeaderUnit()) && !mReverseHotDropGarrison)
      {
         return (false);
      }
   }

   // Base class connect
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   //Figure our range.  This will end up setting the range into mTarget.
   if (!mTarget.isRangeValid())
   {
      pSquad->calculateRange(&mTarget, NULL);
   }

   // Set the garrisoning squad flag
   pSquad->setFlagIsGarrisoning(true);

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionGarrison::disconnect()
{   
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BEntity* pTarget = gWorld->getEntity(mTarget.getID());

   // Allow target to garrison other squads
   if (pTarget && !pTarget->getFlagGarrisonValid())
   {
      pTarget->setFlagGarrisonValid(true);            
   }
   mFlagOwnTarget = false;

   // Reset the garrisoning squad flag
   pSquad->setFlagIsGarrisoning(false);

   // Remove the child action.
   pSquad->removeActionByID(mChildActionID);

   // Stop our units.
   removeOpp();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionGarrison::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mTarget.reset();
   mFlagAnyFailed = false;
   mFlagAnySucceed = false;
   mFlagPlatoonMove = false;
   mFlagPlatoonHasMoved = false;
   mFlagOwnTarget = false;
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
   mChildActionID = cInvalidActionID;
   mFutureState = cStateNone;
   mpParentAction = NULL;
   mIgnoreRange = false;
   mReverseHotDropGarrison = false;
   mPathCount = 0;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionGarrison::setState(BActionState state)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BEntity* pTarget = gWorld->getEntity(mTarget.getID());

   switch (state)
   {
      // Moving.
      case cStateMoving:
         {
            // Remove any opp we might have given our units.
            removeOpp();
            
            BEntityID parentID = pSquad->getParentID();
            if (!mFlagPlatoonHasMoved && mpOrder && (parentID != cInvalidObjectID) && (mpOrder->getOwnerID() == parentID))
            {
               mFlagPlatoonMove = true;
            }

            // Have our squad move.  If it can't, fail.
            ++mPathCount;
            BSimTarget tar = mTarget;
            tar.setRange(1.0f);
            mChildActionID = pSquad->doMove(mpOrder, this, &tar, mFlagPlatoonMove, true);
            if (mChildActionID == cInvalidActionID)
            {
               debugMove4("setState: Unable to move squad.  Action failed.");
               setState(cStateFailed);
               return (true);
            }            
         }
         break;

      // Garrisoning.  Give our units a garrison Opp.
      case cStateWorking:
         {            
//-- FIXING PREFIX BUG ID 2036
            const BUnit* pTargetUnit = getTargetUnit();
//--
            if (!pTargetUnit)
            {
               setState(cStateFailed);
               return (true);
            }

            // If target can garrison then reserve our spot otherwise wait
            if (!mFlagOwnTarget && pTarget && pTarget->getFlagGarrisonValid())
            {
               pTarget->setFlagGarrisonValid(false);
               mFlagOwnTarget = true;
            }
            else if (!mFlagOwnTarget)
            {
               setState(cStateWait);
               return (true);
            }

            // Have enough room?
            if (!pTargetUnit->canContain(pSquad))
            {
               if (pTargetUnit->getProtoObject() && (pTargetUnit->getProtoObject()->getFlagIsTeleporter() || pTargetUnit->isType(gDatabase.getOTIDHotDropPickup())))
                  setState(cStateWait);
               else
                  setState(cStateFailed);
               return (true);
            }

            BUnitOpp opp;
            opp.init();
            opp.setTarget(mTarget);
            opp.setType(BUnitOpp::cTypeGarrison);
            opp.setSource(pSquad->getID());
            opp.generateID();
            opp.setTrigger(true);
            if (mIgnoreRange)
               opp.setUserData(mIgnoreRange);

            if (!addOpp(opp))
            {
               setState(cStateMoving);
               return (true);
            }
         }
         break;

      // Done/Failed.
      case cStateDone:
      case cStateFailed:
         // Reset the garrisoning squad flag
         pSquad->setFlagIsGarrisoning(false);

         // Remove the child action.
         pSquad->removeActionByID(mChildActionID);

         // Remove the opp we gave the units.
         removeOpp();

         // Force update on target squad       
         BEntityID targetID = mTarget.getID();
         BSquad* pTargetSquad = NULL;
         if (targetID.getType() == BEntity::cClassTypeUnit)
         {
            BUnit* pUnit = gWorld->getUnit(targetID);
            if (pUnit)
            {
               pTargetSquad = pUnit->getParentSquad();
            }
         }
         else if (targetID.getType() == BEntity::cClassTypeSquad)
         {
            pTargetSquad = gWorld->getSquad(targetID);
         }
         if (pTargetSquad)
         {
            pTargetSquad->setFlagForceUpdateGarrisoned(true);
         }

         // Check cover
         if (pSquad->getFlagInCover())
         {
            BSquadAI* pSquadAI = pSquad->getSquadAI();
            if (pSquadAI)
            {
               pSquadAI->setMode(BSquadAI::cModeCover);
            }
         }

         // Remove the squad from selection
         BPlayer* pPlayer = (BPlayer*)pSquad->getPlayer();
         if (pPlayer && (state == cStateDone))
         {
            BUser* pUser = pPlayer->getUser();
            if (pUser)
            {
               BEntityID squadID = pSquad->getID();
               BSelectionManager* pSelectionManager = pUser->getSelectionManager();
               if (pSelectionManager && pSelectionManager->isSquadSelected(squadID))
               {
                  pSelectionManager->unselectSquad(squadID);
               }
            }
         }

         // Allow target to garrison other squads
         if (pTarget && !pTarget->getFlagGarrisonValid())
         {
            pTarget->setFlagGarrisonValid(true);            
         }
         mFlagOwnTarget = false;

         // Notify our parent, if any.
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
//==============================================================================
bool BSquadActionGarrison::update(float elapsed)
{
//-- FIXING PREFIX BUG ID 2038
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

   // If our target is gone, we're done.
   if (!validateTarget())
   {
      debugMove4("update: invalid target.  setting state to Done");
      setState(cStateDone);
      return (true);
   }

   BUnit* pTargetUnit = getTargetUnit();
   BASSERT(pTargetUnit);
   // If no room and not a teleporter then fail
   if (!pTargetUnit || (!mFlagOwnTarget && !pTargetUnit->canContain(pSquad) && (!pTargetUnit->getProtoObject() || !pTargetUnit->getProtoObject()->getFlagIsTeleporter()) && !pTargetUnit->isType(gDatabase.getOTIDHotDropPickup())))
   {
      setState(cStateFailed);
      return (true);
   }

   // Get range-ness.
   bool inRange = validateRange();

   switch (mState)
   {
      case cStateNone:
         {
            if (inRange)
            {
               debugMove4("update: inRange of target - setting state to Working.");
               setState(cStateWorking);
            }
            else
            {
               debugMove4("update: not in Range, setting state to Moving..");
               setState(cStateMoving);
            }
         }
         break;

      case cStateMoving:
         break;

      case cStateWorking:
         {
            //If we're not in range, go back to moving.
            if (!inRange)
            {
               setState(cStateMoving);
               break;
            }
         }
         break;

      case cStateWait:
         {
            // If target can garrison then reserve our spot otherwise wait
            if (!mFlagOwnTarget && pTargetUnit->getFlagGarrisonValid())
            {
               pTargetUnit->setFlagGarrisonValid(false);
               mFlagOwnTarget = true;
               setState(cStateNone);
               return (true);
            }

            // Has timer has reset and we can go through now?
            if (mFlagOwnTarget && pTargetUnit->canContain(pSquad))
            {
               setState(cStateNone);
               return (true);
            }
         }
         break;
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionGarrison::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BUnit* pTargetUnit = getTargetUnit();

   switch (eventType)
   {
      case BEntity::cEventActionFailed:
         if (data1 == mChildActionID)
         {
            // We got close enough to garrison anyway, so go ahead and garrison
            if (validateRange())
               mFutureState = cStateWorking;
         	// [6-19-08 CJS] Try pathing to the objective again in case it was blocked (hotdrop) or in use (teleporter)
            else if (mPathCount < cMaxTeleporterRepaths && pTargetUnit->getProtoObject() && (pTargetUnit->getProtoObject()->getFlagIsTeleporter() || pTargetUnit->isType(gDatabase.getOTIDHotDropPickup())))
               mFutureState = cStateMoving;
            else
               mFutureState = cStateFailed;
         }
         break;

      case BEntity::cEventActionDone:
         if (data1 == mChildActionID)
         {
            float dist = distanceToTarget();

            // Try to get us as close as possible--if we fail on the move, we'll still garrison in the fail state above.
            if ((mState == cStateMoving) && validateRange())
            {
               if (mPathCount < cMaxTeleporterRepaths && mTarget.isRangeValid() && dist > mTarget.getRange() / 3.0f)
                  mFutureState = cStateMoving;
               else
                  mFutureState = cStateWorking;
            }
            else if ((mState == cStateMoving) && !validateRange() && mFlagPlatoonMove)
            {
               mFlagPlatoonMove = false;
               mFlagPlatoonHasMoved = true;
               mFutureState = cStateMoving;
            }
            else
            {
               // [7-25-08 CJS] Try pathing to the objective again in case it was blocked (hotdrop) or in use (teleporter)
               if (mPathCount < cMaxTeleporterRepaths && pTargetUnit->getProtoObject() && (pTargetUnit->getProtoObject()->getFlagIsTeleporter() || pTargetUnit->isType(gDatabase.getOTIDHotDropPickup())))
                  mFutureState = cStateMoving;
               else
                  mFutureState = cStateDone;
            }
         }
         break;

      // Check if this squad's units have completed their garrison opportunities
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
               else
               {
                  mFlagAnySucceed = true;
               }

               mUnitOppIDCount--;
               if (mUnitOppIDCount == 0)
               {
                  if (mFlagAnyFailed)
                  {
                     // This is bad.  We have some units that have garrisoned and some that have not.                        
                     if (mFlagAnySucceed)
                     {                        
                        if (pSquad)
                        {
                           uint numUnits = pSquad->getNumberChildren();
                           for (uint i = 0; i < numUnits; i++)
                           {
                              BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(i));
                              if (pChildUnit)
                              {
                                 // We still have a valid target so lets unload any garrisoned units
                                 if (pTargetUnit)
                                 {
                                    if (pChildUnit->isGarrisoned())
                                    {
                                       pTargetUnit->unloadUnit(pChildUnit->getID(), false);
                                    }
                                    else if (pChildUnit->isAttached())
                                    {
                                       pTargetUnit->unattachObject(pChildUnit->getID());
                                    }
                                 }
                                 // Invalid target so clean up units as best as possible and fail
                                 else
                                 {
                                    if (pChildUnit->isGarrisoned())
                                    {
                                       pChildUnit->setFlagGarrisoned(false);
                                       pChildUnit->setFlagPassiveGarrisoned(false);      
                                       pChildUnit->setAnimationEnabled(true, true);
                                       const BProtoObject* pProtoObject = pChildUnit->getProtoObject();
                                       if (pProtoObject)
                                       {
                                          pChildUnit->setFlagCollidable(pProtoObject->getFlagCollidable());
                                          pChildUnit->setFlagNonMobile(pProtoObject->getFlagNonMobile());
                                          if (!pProtoObject->getFlagNoTieToGround())
                                          {
                                             pChildUnit->tieToGround();
                                          }
                                       }
                                       pChildUnit->updateObstruction(); 
                                       pChildUnit->setPhysicsKeyFramed(false);

                                       if (pChildUnit->getFlagLOS() && !pChildUnit->getFlagLOSMarked() && !pChildUnit->getFlagInCover())
                                       {
                                          pChildUnit->markLOSOn();
                                       }

                                       pChildUnit->setFlagInCover(false);
                                       pChildUnit->removeEntityRef(BEntityRef::cTypeContainingUnit, mTarget.getID());

                                       // DLM 5/29/08
                                       // The Squad comes with the unit.. 
#ifdef _MOVE4
                                       // Use the unit's collidable state as the squad's collidable status.  
                                       // This could cause problems if we mix collidable and noncollidable units in the same squad, lord help us.
                                       pSquad->setFlagCollidable(pProtoObject->getFlagCollidable());
                                       pSquad->updateObstruction();
#endif
                                       pSquad->updateGarrisonedFlag();
                                    }                                   
                                    else if (pChildUnit->isAttached())
                                    {
                                       pChildUnit->setFlagAttached(false);
                                       pChildUnit->setFlagNoWorldUpdate(false);
                                       const BProtoObject* pProtoObject = pChildUnit->getProtoObject();
                                       if (pProtoObject)
                                       {
                                          pChildUnit->setFlagCollidable(pProtoObject->getFlagCollidable());
                                          // [5/15/2008 xemu] make this selectable again now that it is no longer a child object 
                                          if (pProtoObject->getFlagNotSelectableWhenChildObject())
                                          {
                                             pChildUnit->setFlagSelectable(true);
                                          }
                                       }                                       
                                       pChildUnit->updateObstruction();
                                       pChildUnit->setPhysicsKeyFramed(false);
                                       pChildUnit->setFlagPassiveGarrisoned(false);
                                       pChildUnit->removeEntityRef(BEntityRef::cTypeAttachedToObject, mTarget.getID());

                                       pSquad->updateAttachedFlag();
                                    }
                                 }

                                 // Reset the unit position and orientation
#ifdef SYNC_Unit
                                 syncUnitData("BSquadActionGarrison::notify", pSquad->getPosition());
#endif
                                 pChildUnit->setPosition(pSquad->getPosition());
                                 pChildUnit->setForward(pSquad->getForward());
                                 pChildUnit->setUp(cYAxisVector);
                                 pChildUnit->calcRight();
                                 pChildUnit->clearGoalVector();
                              }
                           }
                           pSquad->settle();
                        }

                        mFutureState = cStateFailed;
                     }
                     else
                     {
                        // Did we fail because we were out of range?
                        if (!validateRange())
                        {
                           mFutureState = cStateMoving;
                        }
                        else
                        {
                           // If teleporter then reset don't assume we where in the wait state
                           if (pTargetUnit && pTargetUnit->getProtoObject() && ((pTargetUnit->getProtoObject()->getFlagIsTeleporter() || pTargetUnit->isType(gDatabase.getOTIDHotDropPickup())) && !pTargetUnit->getFlagBlockContain()))
                           {
                              mFutureState = cStateNone;                           
                           }
                           else
                           {
                              mFutureState = cStateFailed;
                           }
                        }
                     }                        
                  }
                  else
                  {
                     mFutureState = cStateDone;
                  }
                  mFlagAnyFailed = false;
                  mFlagAnySucceed = false;
               }
            }            
         }
         break;
   }    

   //XXXHalwes - 7/30/2008 - Testing only, remove me.
   //BASSERT(mFutureState != cStateFailed);
}

//==============================================================================
//==============================================================================
void BSquadActionGarrison::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit) || (target.getID().getType() == BEntity::cClassTypeSquad));

   mTarget = target;
}

//==============================================================================
//==============================================================================
bool BSquadActionGarrison::validateTarget()
{
   if (!getTargetUnit())
      return (false);

   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 2040
      const BEntity* pEnt = gWorld->getEntity(mTarget.getID());
//--
      if (!pEnt)
      {
         return (false);
      }

      return (pEnt->isAlive());
   }

   return (mTarget.isPositionValid());
}

//==============================================================================
//==============================================================================
bool BSquadActionGarrison::validateRange()
{
//-- FIXING PREFIX BUG ID 2041
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   if (mIgnoreRange)
      return true;

   bool inRange = true;
   float distance = distanceToTarget();

   if (mTarget.isRangeValid() && (distance > mTarget.getRange()))
      inRange = false;

   return (inRange);
}

//==============================================================================
//==============================================================================
bool BSquadActionGarrison::addOpp(BUnitOpp opp)
{
   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 2043
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
//==============================================================================
void BSquadActionGarrison::removeOpp()
{
   if (mUnitOppID == BUnitOpp::cInvalidID)
   {
      return;
   }

   //Remove the opportunity that we've given the units.  That's all we do here.
//-- FIXING PREFIX BUG ID 2044
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
}

//==============================================================================
// Resolve the target unit
//==============================================================================
BUnit* BSquadActionGarrison::getTargetUnit()
{
   BUnit* pTargetUnit = NULL;
   if (mTarget.isIDValid())
   {
      BEntityID target = mTarget.getID();      
      if (target.getType() == BEntity::cClassTypeSquad)
      {
         BSquad* pSquad = gWorld->getSquad(target);
         if (pSquad)
         {
            pTargetUnit = pSquad->getLeaderUnit();
         }
      }
      else if (target.getType() == BEntity::cClassTypeUnit)
      {
         pTargetUnit = gWorld->getUnit(target);
      }
   }   

   return (pTargetUnit);
}


//==============================================================================
// Get the distance from the owner squad to the target
//==============================================================================
float BSquadActionGarrison::distanceToTarget()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   float distance = 0.0f;
   if (mTarget.getID().isValid())
   {
      distance = pSquad->calculateXZDistance(gWorld->getEntity(mTarget.getID()));
   }
   else
   {
      distance = pSquad->calculateXZDistance(mTarget.getPosition());
   }

   return distance;
}

//==============================================================================
//==============================================================================
bool BSquadActionGarrison::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);
   GFWRITEACTIONPTR(pStream, mpParentAction);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVAR(pStream, uint8, mUnitOppIDCount);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEVAR(pStream, uint8, mPathCount);
   GFWRITEBITBOOL(pStream, mFlagAnyFailed);   
   GFWRITEBITBOOL(pStream, mFlagAnySucceed);
   GFWRITEBITBOOL(pStream, mFlagPlatoonMove);
   GFWRITEBITBOOL(pStream, mFlagPlatoonHasMoved);
   GFWRITEBITBOOL(pStream, mFlagOwnTarget);
   GFWRITEBITBOOL(pStream, mIgnoreRange);
   GFWRITEBITBOOL(pStream, mReverseHotDropGarrison);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionGarrison::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);
   GFREADACTIONPTR(pStream, mpParentAction);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVAR(pStream, uint8, mUnitOppIDCount);
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADVAR(pStream, uint8, mPathCount);
   GFREADBITBOOL(pStream, mFlagAnyFailed);   
   GFREADBITBOOL(pStream, mFlagAnySucceed);
   GFREADBITBOOL(pStream, mFlagPlatoonMove);
   GFREADBITBOOL(pStream, mFlagPlatoonHasMoved);
   GFREADBITBOOL(pStream, mFlagOwnTarget);
   GFREADBITBOOL(pStream, mIgnoreRange);
   GFREADBITBOOL(pStream, mReverseHotDropGarrison);

   return true;
}
