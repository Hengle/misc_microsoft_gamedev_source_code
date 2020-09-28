//==============================================================================
// squadactionhitch.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unitactionmove.h"
#include "squadactionhitch.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "tactic.h"
#include "triggervar.h"
#include "usermanager.h"
#include "selectionmanager.h"
#include "user.h"
#include "SimOrderManager.h"
#include "techtree.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionHitch, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionHitch::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
   {
      return (false);
   }

   // Check target so we can check range
   if (!validateTarget())
   {
      return (false);
   }

   BEntityID targetID = mTarget.getID();
   BSquad* pTargetSquad = NULL;
   if (targetID.getType() == BEntity::cClassTypeSquad)
   {
      pTargetSquad = gWorld->getSquad(targetID);
   }
   else if (targetID.getType() == BEntity::cClassTypeUnit)
   {
      BUnit* pTargetUnit = gWorld->getUnit(targetID);
      if (pTargetUnit)
      {
         pTargetSquad = pTargetUnit->getParentSquad();
      }
   }
   BASSERT(pTargetSquad);
   if (!pTargetSquad)
   {
      return (false);
   }

   // Calculate range
   if (!mTarget.isRangeValid())
   {
      mTarget.setRange(1.0f);
   }   

   // Add the trailer to its own army and platoon
   BObjectCreateParms targetObjectParms;
   targetObjectParms.mPlayerID = pTargetSquad->getPlayerID();
   BArmy* pTargetArmy = gWorld->createArmy(targetObjectParms);
   BASSERT(pTargetArmy);
   if (!pTargetArmy)
   {
      return (false);
   }

   BEntityIDArray targetSquads;
   targetSquads.add(targetID);
   pTargetArmy->addSquads(targetSquads, false);            

   // Setup all the positions for sanity check
   BPlatoon* pTargetPlatoon = pTargetSquad->getParentPlatoon();
   if (pTargetPlatoon)
   {
      pTargetPlatoon->setPosition(pTargetSquad->getPosition());
      pTargetPlatoon->setForward(pTargetSquad->getForward());
      pTargetPlatoon->setRight(pTargetSquad->getRight());
      pTargetPlatoon->setUp(pTargetSquad->getUp());
   }

   // Set the hitching squad flag
   pSquad->setFlagIsHitching(true);   
   pTargetSquad->setFlagIsHitching(true);

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionHitch::disconnect()
{   
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // Make squad selectable
   pSquad->setFlagSelectable(true);
   pSquad->setFlagMatchFacing(false);

   // Reset the hitching squad flag
   pSquad->setFlagIsHitching(false);
   BEntityID targetID = mTarget.getID();
   BSquad* pTargetSquad = NULL;
   if (targetID.getType() == BEntity::cClassTypeSquad)
   {
      pTargetSquad = gWorld->getSquad(targetID);
   }
   else if (targetID.getType() == BEntity::cClassTypeUnit)
   {
      BUnit* pTargetUnit = gWorld->getUnit(targetID);
      if (pTargetUnit)
      {
         pTargetSquad = pTargetUnit->getParentSquad();
      }
   }

   if (pTargetSquad)
   {
      pTargetSquad->setFlagIsHitching(false);
   }

   // Stop our units.
   removeOpp();

   // Remove the child action.
   pSquad->removeActionByID(mChildActionID);

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
bool BSquadActionHitch::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mTarget.reset();
   mFlagAnyFailed = false;
   mFlagPlatoonMove = false;
   mFlagPlatoonHasMoved = false;
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
   mChildActionID = cInvalidActionID;
   mFutureState = cStateNone;
   mpChildOrder = NULL;
   mHitchPos = cInvalidVector;
   mHitchDir = cInvalidVector;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionHitch::setState(BActionState state)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   switch (state)
   {
      // Moving.
      case cStateMoving:
         {
            // Remove any opp we might have given our units.
            removeOpp();

            // Force platoon move to false
            mFlagPlatoonMove = false;

            // Set destination location appropriately in front of trailer
            BEntity* pEntity = gWorld->getEntity(mTarget.getID());
            BASSERT(pEntity);
            mHitchDir = pEntity->getForward();
//-- FIXING PREFIX BUG ID 2079
            const BUnit* pUnit = pSquad->getLeaderUnit();
//--
            BASSERT(pUnit);            
            float obsRadius = pEntity->getObstructionRadiusZ();
            if (pUnit)
               obsRadius += pUnit->getObstructionRadiusZ();
            float offset = gDatabase.getHitchOffset();
            BVector finalOffset = mHitchDir * (obsRadius + offset);
            mHitchPos = pEntity->getPosition() + finalOffset;
            gTerrainSimRep.getHeight(mHitchPos, false);

            // Add the tow truck into a unique army and platoon
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = pSquad->getPlayerID();
            BArmy* pArmy = gWorld->createArmy(objectParms);
            BASSERT(pArmy);
            if (!pArmy)
            {
               setState(cStateFailed);
               return (true);
            }

            BEntityIDArray squads;
            squads.add(pSquad->getID());
            pArmy->addSquads(squads, false);

            BPlatoon* pPlatoon = pSquad->getParentPlatoon();

            // VAT: 11/15/08: we did all kinds of horrible things like setup our current
            // order for cancellation by removing ourselves from our old platoon when we added
            // ourselves to a new army, so we need a fresh order here
            // scary scary hack!
            BSimOrder* pOrder = gSimOrderManager.createOrder();
            if (pOrder && mpOrder)
            {
               // copy the order so it's all the same
               // and then update our local copy of the order
               pOrder->copyFromTemplate(*mpOrder);
               mpOrder->decrementRefCount();
               pOrder->incrementRefCount();
               setOrder(pOrder);
               pSquad->queueOrder(pOrder, BSimOrder::cTypeHitch);
            }

            // Setup all the positions for sanity check
            if (pPlatoon)
            {
               pPlatoon->setPosition(pSquad->getPosition());
               pPlatoon->setForward(pSquad->getForward());
               pPlatoon->setRight(pSquad->getRight());
               pPlatoon->setUp(pSquad->getUp());
            }

            // Add way points
            BDynamicVectorArray waypoints;            
            waypoints.add(mHitchPos);

            // Setup target and order
            BSimTarget target(mHitchPos);
            setupChildOrder(mpOrder);
            mpChildOrder->setTarget(target);
            mpChildOrder->setWaypoints(waypoints);            

            // Have our squad move.  If it can't, fail.
            mChildActionID = pSquad->doMove(mpChildOrder, this, mFlagPlatoonMove, true);
            if (mChildActionID == cInvalidActionID)
            {
               setState(cStateFailed);
               return (true);
            }            
         }
         break;

      // Rotate to line up with tow
      case cStateTurning:
         {
            // Set squad up for the turn
            pSquad->setFlagSelectable(false);
            pSquad->setFlagMatchFacing(true);
            BUser* pUser = gUserManager.getUserByPlayerID(pSquad->getPlayerID());
            // SLB: This assert gives false positives. The "if" check is enough.
            //BASSERT(pUser);
            if (pUser)
            {
               BSelectionManager* pSelectionManager = pUser->getSelectionManager();
               if (pSelectionManager)
               {
                  pSelectionManager->unselectSquad(pSquad->getID());
               }
            }   

            // Set the position and orientation
            BVector right;
            right.assignCrossProduct(cYAxisVector, mHitchDir);
            right.y = 0.0f;
            right.safeNormalize();
            BPlatoon* pPlatoon = pSquad->getParentPlatoon();
            if (pPlatoon)
            {
               pPlatoon->setPosition(mHitchPos);
               pPlatoon->setForward(mHitchDir);
               pPlatoon->setRight(right);
               pPlatoon->calcUp();
            }
            pSquad->setPosition(mHitchPos);
            pSquad->setLeashPosition(mHitchPos);
            pSquad->setForward(mHitchDir);
            pSquad->setRight(right);
            pSquad->calcUp();
         }
         break;

      // Hitching.  Give our units a hitch Opp.
      case cStateWorking:
         {            
            BUnitOpp opp;
            opp.init();
            opp.setTarget(mTarget);
            opp.setType(BUnitOpp::cTypeHitch);
            opp.setSource(pSquad->getID());
            opp.generateID();
            if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
               opp.setTrigger(true);
            else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
               opp.setPriority(BUnitOpp::cPriorityCommand);
            else
               opp.setPriority(BUnitOpp::cPrioritySquad);

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
         if (state == cStateDone)
         {
            disableUserAbility();
         }
         else
         {
            enableUserAbility();
         }

         // Set squad back to a normal mode
         pSquad->setFlagSelectable(true);
         pSquad->setFlagMatchFacing(false);

         // Remove the child action.
         pSquad->removeActionByID(mChildActionID);

         // Remove the opp we gave the units.
         removeOpp();

         // Reset positions and orientations
         BUnit* pUnit = pSquad->getLeaderUnit();
         BASSERT(pUnit);
         if (pUnit)
         {
            BVector unitPos = pUnit->getPosition();
            BVector unitDir = pUnit->getForward();
            BVector unitRight = pUnit->getRight();
            BVector unitUp = pUnit->getUp();

            BPlatoon* pPlatoon = pSquad->getParentPlatoon();
            BASSERT(pPlatoon);
            if (pPlatoon)
            {
               pPlatoon->setPosition(unitPos);
               pPlatoon->setForward(unitDir);
               pPlatoon->setRight(unitRight);
               pPlatoon->setUp(unitUp);
            }

            pSquad->setPosition(unitPos);
            pSquad->setLeashPosition(unitPos);
            pSquad->setForward(unitDir);
            pSquad->setRight(pUnit->getRight());
            pSquad->setUp(pUnit->getUp());
            pSquad->setTurnRadiusPos(unitPos);
            pSquad->setTurnRadiusFwd(unitDir);
         }
         else
         {
            BPlatoon* pPlatoon = pSquad->getParentPlatoon();
            BASSERT(pPlatoon);
            if (pPlatoon)
            {
               pPlatoon->setPosition(pSquad->getPosition());
               pPlatoon->setForward(pSquad->getForward());
               pPlatoon->setRight(pSquad->getRight());
               pPlatoon->setUp(pSquad->getUp());
            }
         }
         pSquad->updateObstruction();
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionHitch::update(float elapsed)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
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
      setState(cStateDone);
      return (true);
   }

   switch (mState)
   {
      case cStateNone:
         {
            setState(cStateMoving);
         }
         break;

      case cStateMoving:
         {
            const BAction* pAction = mpOwner->findActionByID(mChildActionID);

            // We've lost our child action for some reason.  Fail.
            if (!pAction)
            {
               setState(cStateFailed);
            }
            break;
         }

      case cStateTurning:
         {
            if (pSquad->getTurnRadiusState() == cStateDone)
            {
               setState(cStateWorking);
            }
         }
         break;

      case cStateWorking:
         break;
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionHitch::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   switch (eventType)
   {
      case BEntity::cEventActionFailed:
         if (data1 == mChildActionID)
         {
            mFutureState = cStateFailed;
         }
         break;

      case BEntity::cEventActionDone:
         if (data1 == mChildActionID)
         {
            if (mState == cStateMoving)
            {
               if (validateRange())
               {
                  mFutureState = cStateTurning;
               }
               else
               {                  
                  mFutureState = cStateFailed;
               }
            }
            else 
            {
               mFutureState = cStateDone;
            }
         }
         break;

      // Check if this squad's unit has completed its hitch opportunities
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
                  if (mFlagAnyFailed)
                  {
                     // Did we fail because we were out of range?
                     if (!validateRange())
                     {
                        mFutureState = cStateMoving;
                     }
                     else
                     {
                        mFutureState = cStateFailed;
                     }
                  }
                  else
                  {
                     mFutureState = cStateDone;
                  }
                  mFlagAnyFailed = false;
               }
            }            
         }
         break;
   }    
}

//==============================================================================
//==============================================================================
void BSquadActionHitch::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit) || (target.getID().getType() == BEntity::cClassTypeSquad));

   mTarget = target;
}

//==============================================================================
//==============================================================================
bool BSquadActionHitch::validateTarget()
{
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 2080
      const BEntity* pEnt = gWorld->getEntity(mTarget.getID());
//--
      if (!pEnt)
      {
         return (false);
      }

      return (pEnt->isAlive());
   }

   return (false);
}

//==============================================================================
//==============================================================================
bool BSquadActionHitch::validateRange()
{
//-- FIXING PREFIX BUG ID 2081
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);
   if (!pSquad)
   {
      return (false); 
   }

   if (mHitchPos.almostEqual(cInvalidVector))
   {
      return (false);
   }

   if (!mTarget.isRangeValid())
   {
      return (false);
   }

   if (pSquad->calculateXZDistance(mHitchPos) > mTarget.getRange())
   {
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionHitch::addOpp(BUnitOpp opp)
{
   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 2083
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
void BSquadActionHitch::removeOpp()
{
   if (mUnitOppID == BUnitOpp::cInvalidID)
   {
      return;
   }

   //Remove the opportunity that we've given the unit.  That's all we do here.
//-- FIXING PREFIX BUG ID 2084
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
void BSquadActionHitch::setupChildOrder(BSimOrder* pOrder)
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
// Disable the towing squad's user ability
//==============================================================================
void BSquadActionHitch::disableUserAbility()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
      return;   

   // If we are not hitched then don't disable
   if (!pSquad->hasHitchedSquad())
      return;

   BUnit* pUnit = pSquad->getLeaderUnit();
   BASSERT(pUnit);
   if (!pUnit)
      return;

   BProtoObject* pProtoObject = (BProtoObject*)pUnit->getProtoObject();
   BASSERT(pProtoObject);
   if (!pProtoObject)
      return;

   // Are we already a unique instance?
   if (pProtoObject->getFlagUniqueInstance())
   {
      pProtoObject->setFlagAbilityDisabled(true);
   }
   // Make a unique instance
   else
   {
      BPlayer* pPlayer = (BPlayer*)pUnit->getPlayer();
      BASSERT(pPlayer);
      if (!pPlayer)
         return;

      BProtoObjectID protoObjectID = pProtoObject->getID();
      pPlayer->removeUnitFromProtoObject(pUnit, protoObjectID);
      pPlayer->getTechTree()->checkUnitPrereq(protoObjectID); // THIS MODIFIES STUFF

      BEntityID unitID = pUnit->getID();
      // Goto base tracking
      if (pProtoObject->getGotoType() == cGotoTypeBase)
      {
         pPlayer->removeGotoBase(unitID);
      }
      else if (pProtoObject->getFlagLockdownMenu())
      {
         if (pSquad && (pSquad->getSquadMode() == BSquadAI::cModeLockdown))
         {
            pPlayer->removeGotoBase(unitID);
         }
      }
      
      BProtoObject* pUniquePO = pPlayer->allocateUniqueProtoObject(pProtoObject, pPlayer, unitID);
      BASSERT(pUniquePO);
      if (!pUniquePO)
         return;

      BProtoObjectID uniqueProtoID = pUniquePO->getID();
      pUnit->setProtoID(uniqueProtoID);
      pPlayer->addUnitToProtoObject(pUnit, uniqueProtoID);
      pPlayer->getTechTree()->checkUnitPrereq(uniqueProtoID); // THIS MODIFIED STUFF   

      pUniquePO->setFlagAbilityDisabled(true);
   }
}

//==============================================================================
// Enable the towing squad's user ability
//==============================================================================
void BSquadActionHitch::enableUserAbility()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
      return;   

   // If still hitched forget about it
   if (pSquad->hasHitchedSquad())
      return;

   BUnit* pUnit = pSquad->getLeaderUnit();
   BASSERT(pUnit);
   if (!pUnit)
      return;

   BProtoObject* pProtoObject = (BProtoObject*)pUnit->getProtoObject();
   BASSERT(pProtoObject);
   if (!pProtoObject)
      return;

   if (pProtoObject->getFlagAbilityDisabled())
      pProtoObject->setFlagAbilityDisabled(false);
}

//==============================================================================
//==============================================================================
bool BSquadActionHitch::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVECTOR(pStream, mHitchPos);
   GFWRITEVECTOR(pStream, mHitchDir);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);  
   GFWRITEFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVAR(pStream, uint8, mUnitOppIDCount);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEBITBOOL(pStream, mFlagAnyFailed);
   GFWRITEBITBOOL(pStream, mFlagPlatoonMove);
   GFWRITEBITBOOL(pStream, mFlagPlatoonHasMoved);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionHitch::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVECTOR(pStream, mHitchPos);
   GFREADVECTOR(pStream, mHitchDir);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);  
   GFREADFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVAR(pStream, uint8, mUnitOppIDCount);
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADBITBOOL(pStream, mFlagAnyFailed);
   GFREADBITBOOL(pStream, mFlagPlatoonMove);
   GFREADBITBOOL(pStream, mFlagPlatoonHasMoved);
   return true;
}
