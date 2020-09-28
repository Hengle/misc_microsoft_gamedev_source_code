//==============================================================================
// SquadActionWork.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#include "Common.h"
#include "ConfigsGame.h"
#include "Database.h"
#include "SquadActionWork.h"
#include "Squad.h"
#include "Unit.h"
#include "UnitQuery.h"
#include "World.h"
#include "ability.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionWork, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionWork::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
      return (false);

   //Figure our range.  This will end up setting the range into mTarget.
//-- FIXING PREFIX BUG ID 3557
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);
   if (!mTarget.isRangeValid())
      pSquad->calculateRange(&mTarget, NULL);

   if (mUnitOppType == BUnitOpp::cTypeCapture)
   {
      //Link this squad to the target and pay capture cost
      if (!captureConnect())
      {
         BAction::disconnect();
         return (false);
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionWork::disconnect()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   //Remove the child action.
   pSquad->removeActionByID(mChildActionID);
   //Stop our units.
   removeOpps();

   if (mUnitOppType == BUnitOpp::cTypeCapture)
   {
      //Unlink this squad from the target and refund capture cost if target not captured
      captureDisconnect();
   }

   BAction::disconnect();
}

//==============================================================================
// BSquadActionWork::init
//==============================================================================
bool BSquadActionWork::init()
{
   if (!BAction::init())
      return (false);
      
   mFlagConflictsWithIdle=true;
   mPotentialTargets.clear();
   mTarget.reset();
   mUnitOppType=BUnitOpp::cTypeNone;
   mUnitOppIDs.clear();
   mChildActionID=cInvalidActionID;
   mSearchType=cInvalidProtoObjectID;
   mFutureState=cStateNone;
   mUserData=0;
   mFlagDoneOnWorkOnce=false;
   mFlagDoneOnOppComplete=false;
   mFlagSearchForPotentialTargets=false;
   mFlagHasSearched=false;
   mFlagCapturePaid=false;
   mUserDataSet=false;
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionWork::setState(BActionState state)
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BDEBUG_ASSERT(pSquad);

   switch(state)  
   {
      //If we're going to Moving, tell our squad to move.
      case cStateMoving:
      {
         //Remove the child action.
         pSquad->removeActionByID(mChildActionID);
         mChildActionID=cInvalidActionID;
         //Stop our units.
         removeOpps();
         
         //Have our squad move.  If it can't, fail.  Platoon move is not valid
         //after we've searched.
         bool platoonMove=false;
         if (!mFlagHasSearched && mpOrder && (pSquad->getParentID() != cInvalidObjectID) && (mpOrder->getOwnerID() == pSquad->getParentID()))
            platoonMove=true;
         // Since capturing and gathering is based on unit range, the squad move needs to wait on (i.e. monitor opps of)
         // the child units before completing
         bool monitorOpps = false;
         if (mUnitOppType == BUnitOpp::cTypeCapture || mUnitOppType == BUnitOpp::cTypeGather || mUnitOppType == BUnitOpp::cTypeJoin)
            monitorOpps = true;
         // DLM 6/30/08 - Set ForceLeashUpdate to true, so the squad will update it's leash as it follows the target around..
         mChildActionID=pSquad->doMove(mpOrder, this, &mTarget, platoonMove, monitorOpps, false, true);
         if (mChildActionID == cInvalidActionID)
         {
            setState(cStateFailed);
            return (true);
         }
         break;
      }
         
      //Working is, um, working.
      case cStateWorking:
      {
         //Remove the child action.
         pSquad->removeActionByID(mChildActionID);
         mChildActionID=cInvalidActionID;
         //Stop our units.
         removeOpps();

         //Make sure capture is valid.
         if (mUnitOppType == BUnitOpp::cTypeCapture)
         {
            if (!captureValidate())
            {
               setState(cStateFailed);
               return (true);
            }
         }

         //Handle unpacking.
         if (mUnitOppType == BUnitOpp::cTypeUnpack)
         {
            setState(cStateDone);
            unpack();
            return (false);
         }

         //Figure out the actual target.  We want to pass down a squad target if
         //we have one.  We do make a copy of our target since the actions here
         //are specific (vs. attacking).
         BUnitOpp opp;
         opp.init();
         BSimTarget actualTarget;
         actualTarget=mTarget;
         //Pass down a squad target.
         if (mTarget.getID().isValid() && (mTarget.getID().getType() == BEntity::cClassTypeUnit))
         {
//-- FIXING PREFIX BUG ID 3558
            const BUnit* pTargetUnit=gWorld->getUnit(mTarget.getID());
//--
            if (!pTargetUnit || !pTargetUnit->getParentSquad())
            {
               setState(cStateFailed);
               return (true);
            }
            actualTarget.setID(pTargetUnit->getParentSquad()->getID());
         }
         opp.setTarget(actualTarget);
         opp.setType(mUnitOppType);
         opp.setSource(pSquad->getID());
         if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
            opp.setTrigger(true);
         else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
            opp.setPriority(BUnitOpp::cPriorityCommand);
         else
            opp.setPriority(BUnitOpp::cPrioritySquad);

         if (mUserDataSet)
            opp.setUserData(mUserData);
            
         //Fail if we can't do the opp.
         if (!addOpps(opp))
         {
            setState(cStateFailed);
            return (true);
         }
         break;
      }

      //Search.
      case cStateSearch:
      {
         //We've searched now.
         mFlagHasSearched=true;
         
         //Remove the child action.
         pSquad->removeActionByID(mChildActionID);
         mChildActionID=cInvalidActionID;
         //Stop our units.
         removeOpps();

         //Validate our potential target list.  Remove our current target
         //from that list first, since we're searching (which means it's
         //not something we want).
         if (mTarget.getID().isValid())
            mPotentialTargets.removeValue(mTarget.getID());
         validatePotentialTargets();

         //If there's nothing left, then bail.
         if (mPotentialTargets.getSize() == 0)
         {
            setState(cStateDone);
            return (true);
         }
         
         //Else, take the first thing.
         BSimTarget newTarget(mPotentialTargets[0]);
         pSquad->calculateRange(&newTarget, NULL);
         setTarget(newTarget);
         setState(cStateMoving);
         return (true);
      }

      //Done/Failed.
      case cStateDone:
      case cStateFailed:
      {
         //Remove the child action.
         pSquad->removeActionByID(mChildActionID);
         mChildActionID=cInvalidActionID;
         //Stop our units.
         removeOpps();
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionWork::update(float elapsed)
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (mUnitOppType == BUnitOpp::cTypeGather)
      pSquad->setLeashPosition(pSquad->getPosition());

   //If we're supposed to search for potential targets, do that.
   if (mFlagSearchForPotentialTargets)
      searchForPotentialTargets();

   //If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
         return (true);
      mFutureState=cStateNone;
   }

   //See if our target is gone.  If it is and we're gathering, search.  Else, go done.
   if (!validateTarget())
   {
      if (mUnitOppType == BUnitOpp::cTypeGather)
         setState(cStateSearch);
      else
         setState(cStateDone);
      return (true);
   }
   //Get range-ness.
   bool inRange=validateRange();
   
   //State logic.
   switch (mState)
   {
      case cStateNone:
      {
         setState(cStateMoving);
         break;
      }
      
      case cStateMoving:
      {
         // If our move action has been cancelled, re-move to our target
         if (mUnitOppType == BUnitOpp::cTypeJoin)
         {
//-- FIXING PREFIX BUG ID 3559
            const BAction* pAction = mpOwner->findActionByID(mChildActionID);
//--
            if (!pAction)
               setState(cStateWorking);
         }
         break;
      }

      case cStateWorking:
      {
         //If we're not in range, go back to moving.
         // MS 11/17/2008: unless this is a join opp and our target is garrisoned in a base or we're cryo'd
         BEntity* pTarget = NULL;
         if (mTarget.getID().isValid())
            pTarget = gWorld->getEntity(mTarget.getID());

         bool base = false;
         BSquad* pTargetSquad = pTarget->getSquad();

         if (pTargetSquad && pTargetSquad->getLeaderUnit())
         {
            BEntityRef* pRef = pTargetSquad->getLeaderUnit()->getFirstEntityRefByType(BEntityRef::cTypeContainingUnit);
            if (pRef)
            {
               BUnit* pContainerUnit = gWorld->getUnit(pRef->mID);
               if (pContainerUnit && pContainerUnit->isType(gDatabase.getOTIDBase()))
                  base = true;
            }
         }


         if (!inRange && !(mUnitOppType == BUnitOpp::cTypeJoin && pTarget &&
            ((pTarget->getFlagGarrisoned() && base) || pSquad->getFlagCryoFrozen())))
         {
            setState(cStateMoving);
            break;
         }

         //If we're only supposed to work once, go away.
         if (mFlagDoneOnWorkOnce)
         {
            setState(cStateDone);
            break;
         }
         break;
      }
   }

   return(true);
}

//==============================================================================
//==============================================================================
void BSquadActionWork::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   switch (eventType)
   {
      case BEntity::cEventOppComplete:
      //Data1:  OppID.
      //Data2:  Success.
      {
         //Validate this is one ours and remove it at the same time.
         if (mUnitOppIDs.remove(data1))
         {
            //If we're done on opp complete, this is defined as all of the opps
            //being done or failed.
            if (mFlagDoneOnOppComplete)
            {
               if (mUnitOppIDs.getSize() == 0)
                  mFutureState=cStateDone;
            }
            //Else, if we're gathering, then go search if all of the opps are done/failed.
            else if (mUnitOppType == BUnitOpp::cTypeGather)
            {
               if ((mUnitOppIDs.getSize() == 0) && (mFutureState == cStateNone))
                  mFutureState=cStateSearch;
            }
            // Else if joining and we failed, stop running
            else if (mUnitOppType == BUnitOpp::cTypeJoin && data2 == 0)
            {
               mFutureState = cStateFailed;
            }
         }
         break;
      }
         
      //If the move failed, then either search or fail.
      case BEntity::cEventActionFailed:
         if ((data1 == mChildActionID) && ((mState != cStateSearch) || (mState != cStateFailed)))
         {
            if (mUnitOppType == BUnitOpp::cTypeGather)
               mFutureState=cStateSearch;
            else if (mUnitOppType == BUnitOpp::cTypeJoin)
               mFutureState=cStateWorking;
            else
               mFutureState=cStateFailed;
         }
         break;
      //If the move succeeded, then go to work if we're in range, else search.
      case BEntity::cEventActionDone:
         if ((data1 == mChildActionID) && ((mState != cStateSearch) || (mState != cStateFailed) || (mState != cStateWorking)))
         {
            BEntity* pTarget = NULL;
            if (mTarget.getID().isValid())
               pTarget = gWorld->getEntity(mTarget.getID());

            // MS 11/17/2008: validate range, but stay working even if out of range if we're trying to join with a garrisoned target
            if (validateRange() || (mUnitOppType == BUnitOpp::cTypeJoin && pTarget && pTarget->getFlagGarrisoned()))
               mFutureState=cStateWorking;
            else if (mUnitOppType == BUnitOpp::cTypeGather)
               mFutureState=cStateSearch;
            else if (mUnitOppType == BUnitOpp::cTypeJoin)
               mFutureState=cStateMoving;
            else
               mFutureState=cStateFailed;
         }
         break;

      // A unit was added to the squad (probably through repair / reinforce).  Add any unit opps associated with this action to
      // the new unit.
      case BEntity::cEventSquadUnitAdded:
         {
//-- FIXING PREFIX BUG ID 3562
            const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
            if (pSquad && pSquad->containsChild(static_cast<BEntityID>(data1)))
            {
               BUnit* pUnit = gWorld->getUnit(data1);
               if (pUnit)
               {
                  if (mUnitOppIDs.getSize() > 0)
                  {
                     // Find a unit opp amongst the squad mate units that is in the
                     // list of unit opps associated with this squad action
//-- FIXING PREFIX BUG ID 3561
                     const BUnitOpp* pSquadmateOpp = NULL;
//--
                     for (uint i = 0; i < pSquad->getNumberChildren(); i++)
                     {
                        // Don't look at the opps for the unit just added
                        if (pSquad->getChild(i) == pUnit->getID())
                           continue;

//-- FIXING PREFIX BUG ID 3560
                        const BUnit* pSquadmate = gWorld->getUnit(pSquad->getChild(i));
//--
                        if (pSquadmate)
                        {
                           // Iterate through list of opps to find one in this actions
                           // opp id list
                           for (uint j = 0; j < pSquadmate->getNumberOpps(); j++)
                           {
                              const BUnitOpp* pOpp = pSquadmate->getOppByIndex(j);
                              if (mUnitOppIDs.contains(pOpp->getID()))
                              {
                                 pSquadmateOpp = const_cast<BUnitOpp*>(pOpp);
                                 break;
                              }
                           }
                        }
                        // If one was found, skip down to the section below
                        if (pSquadmateOpp)
                           break;
                     }

                     // If found a correct unit opp associated with this squad action,
                     // copy and add it to the new unit.
                     if (pSquadmateOpp)
                     {
                        // Copy new opp
                        BUnitOpp* pNewOpp = BUnitOpp::getInstance();
                        *pNewOpp = *pSquadmateOpp;
                        pNewOpp->generateID();

                        //Add it.
                        if (!pUnit->addOpp(pNewOpp))
                           BUnitOpp::releaseInstance(pNewOpp);
                        else
                           mUnitOppIDs.add(pNewOpp->getID());
                     }
                  }
               }
            }
         }
         break;
   }    
}

//==============================================================================
//==============================================================================
void BSquadActionWork::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() ||
      (target.getID().getType() == BEntity::cClassTypeUnit) ||
      (target.getID().getType() == BEntity::cClassTypeSquad));
      
   //Set the base target.
   mTarget=target;
}

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BSquadActionWork::getDebugLine(uint index, BSimString &string) const
{
   switch (index)
   {
      case 1:
      {
         string.format("Target: ID=%s, Pos=(%.2f, %.2f, %.2f)%s, Range=%.2f%s.", mTarget.getID().getDebugString().getPtr(),
            mTarget.getPosition().x, mTarget.getPosition().y, mTarget.getPosition().z, mTarget.isPositionValid() ? "(V)" : "",
            mTarget.getRange(), mTarget.isRangeValid() ? "(V)" : "");
         break;
      }
      default:
         BAction::getDebugLine(index, string);
         break;
   }
}
#endif

//==============================================================================
//==============================================================================
bool BSquadActionWork::validateTarget()
{
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 3563
      const BEntity* pEnt=gWorld->getEntity(mTarget.getID());
//--
      if (!pEnt || !pEnt->isAlive())
         return (false);
      if ((mUnitOppType == BUnitOpp::cTypeGather) && (pEnt->getResourceAmount() < cFloatCompareEpsilon))
         return (false);
      return (true);
   }
   return (mTarget.isPositionValid());
}

//==============================================================================
//==============================================================================
bool BSquadActionWork::validateRange()
{
//-- FIXING PREFIX BUG ID 3564
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BDEBUG_ASSERT(pSquad);
   BDEBUG_ASSERT(mTarget.isRangeValid());
   float distance=0.0f;
   BEntity* pTarget = NULL;
   if (mTarget.getID().isValid())
      pTarget = gWorld->getEntity(mTarget.getID());
   if (pTarget)
      distance=pSquad->calculateXZDistance(pTarget);
   else
      distance=pSquad->calculateXZDistance(mTarget.getPosition());

   if (distance > mTarget.getRange())
      return (false);
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionWork::addOpps(BUnitOpp opp)
{
   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 3566
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
//--
   BDEBUG_ASSERT(pSquad);
   BDEBUG_ASSERT(mUnitOppIDs.getSize() == 0);

   if (pSquad->addOppToChildren(opp, &mUnitOppIDs))
      return (true);
   return (false);
}

//==============================================================================
//==============================================================================
void BSquadActionWork::removeOpps()
{
   if (mUnitOppIDs.getSize() == 0)
      return;

   //Remove the opportunity that we've given the units.  That's all we do here.
//-- FIXING PREFIX BUG ID 3567
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
//--
   BDEBUG_ASSERT(pSquad);

   pSquad->removeOppFromChildren(mUnitOppIDs);
   mUnitOppIDs.clear();
}

//==============================================================================
//==============================================================================
void BSquadActionWork::searchForPotentialTargets()
{
//-- FIXING PREFIX BUG ID 3570
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BDEBUG_ASSERT(pSquad);

   //Clear our potential target list.
   mPotentialTargets.clear();
   mFlagSearchForPotentialTargets=false;

   //Figure out where to search.
   BVector searchPosition;
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 3568
      const BEntity* pTarget=gWorld->getEntity(mTarget.getID());
//--
      if (pTarget)
         searchPosition=pTarget->getPosition();
      else
         return;
   }
   else if (mTarget.isPositionValid())
      searchPosition=mTarget.getPosition();
   else
      searchPosition=pSquad->getPosition();

   //See what we can find.
   BUnitQuery query(searchPosition, 20.0f, true);
   query.addPlayerFilter(0);
   query.addObjectTypeFilter(mSearchType);
   gWorld->getSquadsInArea(&query, &mPotentialTargets);
   
   //If we didn't find anything, return.
   if (mPotentialTargets.getSize() == 0)
      return;

   //If we're gathering, loop through to make sure that we have things with resources.
   if (mUnitOppType == BUnitOpp::cTypeGather)
   {
      for (uint i=0; i < mPotentialTargets.getSize(); i++)
      {
//-- FIXING PREFIX BUG ID 3569
         const BSquad* pResourceSquad=gWorld->getSquad(mPotentialTargets[i]);
//--
         if (!pResourceSquad || (pResourceSquad->getResourceAmount() < cFloatCompareEpsilon))
         {
            mPotentialTargets.removeIndex(i);
            i--;
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadActionWork::validatePotentialTargets()
{
   //Remove any targets that are gone or don't have what we want.
   for (uint i=0; i < mPotentialTargets.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 3571
      const BEntity* pEntity=gWorld->getEntity(mPotentialTargets[i]);
//--
      if (!pEntity || !pEntity->isAlive())
      {
         mPotentialTargets.removeIndex(i);
         i--;
         continue;
      }
      if ((mUnitOppType == BUnitOpp::cTypeGather) && (pEntity->getResourceAmount() < cFloatCompareEpsilon))
      {
         mPotentialTargets.removeIndex(i);
         i--;
         continue;
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionWork::captureConnect()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);

   BSquad* pTarget = gWorld->getSquad(mTarget.getID());
   if (pTarget)
   {
      //Link player to the target.
      BEntityID playerID(pSquad->getPlayerID());
      BEntityRef* pRef = pTarget->getFirstEntityRefByID(BEntityRef::cTypeCapturingPlayer, playerID);
      if (!pRef)
         pRef = pTarget->addEntityRef(BEntityRef::cTypeCapturingPlayer, playerID, 0, 0);
      if (!pRef)
         return (false);

      //Increment link ref count for this squad.
      pRef->mData1++;

      //Pay the cost if not already paid.
      if (pRef->mData2 != 1)
      {
         int protoObjectID = pTarget->getProtoObjectID();
         if (protoObjectID != -1)
         {
            BPlayer* pPlayer = pSquad->getPlayer();
            const BProtoObject* pProtoObject = pPlayer->getProtoObject(protoObjectID);
            const BCost* pCost = pProtoObject->getCaptureCost(pPlayer);
            if (pCost)
            {
               if (pPlayer->payCost(pCost))
                  pRef->mData2 = 1;
            }
            else
               pRef->mData2 = 1;
         }
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionWork::captureDisconnect()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);

   BSquad* pTarget = gWorld->getSquad(mTarget.getID());
   if (pTarget)
   {
      //Get the player's link to the target.
      BEntityID playerID(pSquad->getPlayerID());
      BEntityRef* pRef = pTarget->getFirstEntityRefByID(BEntityRef::cTypeCapturingPlayer, playerID);
      if (pRef)
      {
         //Decrement link ref count for this squad.
         pRef->mData1--;

         //No other squads for this player linked to the target?
         if (pRef->mData1 == 0)
         {
            //Not captured?
            if (pTarget->getPlayerID() != pSquad->getPlayerID())
            {
               //Refund the cost.
               if (pRef->mData2 == 1)
               {
                  int protoObjectID = pTarget->getProtoObjectID();
                  if (protoObjectID != -1)
                  {
                     BPlayer* pPlayer = pSquad->getPlayer();
                     const BProtoObject* pProtoObject = pPlayer->getProtoObject(protoObjectID);
                     const BCost* pCost = pProtoObject->getCaptureCost(pPlayer);
                     if (pCost)
                        pPlayer->refundCost(pCost);
                  }
               }

               //Lose target capture progress.
               BUnit* pTargetUnit = pTarget->getLeaderUnit();
               if (pTargetUnit && !pTargetUnit->getFlagBeingCaptured())
                  pTargetUnit->setCapturePoints(0.0f, 0.0f);
            }

            //Remove the entity ref.
            pTarget->removeEntityRef(BEntityRef::cTypeCapturingPlayer, playerID);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionWork::captureValidate()
{
//-- FIXING PREFIX BUG ID 3572
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
//--

   BSquad* pTarget = gWorld->getSquad(mTarget.getID());
   if (!pTarget)
      return (false);

   //Get the player's link to the target.
   BEntityID playerID(pSquad->getPlayerID());
//-- FIXING PREFIX BUG ID 3573
   const BEntityRef* pRef = pTarget->getFirstEntityRefByID(BEntityRef::cTypeCapturingPlayer, playerID);
//--
   if (!pRef)
      return (false);

   //Didn't pay capture cost?
   if (pRef->mData2 != 1)
      return (false);

   //Target already being captured by another squad?
//-- FIXING PREFIX BUG ID 3574
   const BUnit* pTargetUnit = pTarget->getLeaderUnit();
//--
   if (pTargetUnit && pTargetUnit->getFlagBeingCaptured())
      return (false);

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionWork::unpack()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);

   if (!mTarget.isAbilityIDValid() || !mTarget.isPositionValid())
      return;

   int abilityID = gDatabase.getSquadAbilityID(pSquad, mTarget.getAbilityID());
   const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
   if (!pAbility)
      return;

   int protoObjectID = pAbility->getObject(0);
   if (protoObjectID == -1)
      return;

   const BPlayer* pPlayer = pSquad->getPlayer();
   if (!pPlayer)
      return;

   BPlayerID playerID = (pPlayer->getCoopID() != cInvalidPlayerID ? pPlayer->getCoopID() : pPlayer->getID());

   BVector forward(cZAxisVector);
   BVector right(cXAxisVector);
   float angle = (mpOrder ? mpOrder->getAngle() : 0.0f);
   if (angle != 0.0f)
   {
      forward.rotateXZ(Math::fDegToRad(angle));
      forward.normalize();
      right.assignCrossProduct(cYAxisVector, forward);
      right.normalize();
   }

   // Fail if object can't be built here
   const DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain;
   BVector suggestion;
   if (!gWorld->checkPlacement(protoObjectID,  playerID, mTarget.getPosition(), suggestion, forward, BWorld::cCPLOSDontCare, flags))
      return;

   BEntityID unpackedID = gWorld->createEntity(protoObjectID, false, playerID, mTarget.getPosition(), forward, right, false, false, false, cInvalidObjectID, pPlayer->getID(), cInvalidObjectID, cInvalidObjectID, true);
   if (unpackedID == cInvalidObjectID)
      return;

   pSquad->kill(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionWork::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEARRAY(pStream, BEntityID, mPotentialTargets, uint8, 200);
   GFWRITEARRAY(pStream, uint, mUnitOppIDs, uint8, 200);
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVAR(pStream, BProtoObjectID, mSearchType);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEVAR(pStream, BUnitOppType, mUnitOppType);
   GFWRITEVAR(pStream, uint16, mUserData);
   GFWRITEBITBOOL(pStream, mFlagDoneOnWorkOnce);
   GFWRITEBITBOOL(pStream, mFlagDoneOnOppComplete);
   GFWRITEBITBOOL(pStream, mFlagSearchForPotentialTargets);
   GFWRITEBITBOOL(pStream, mFlagHasSearched);
   GFWRITEBITBOOL(pStream, mFlagCapturePaid);
   GFWRITEBITBOOL(pStream, mUserDataSet);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionWork::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADARRAY(pStream, BEntityID, mPotentialTargets, uint8, 200);
   GFREADARRAY(pStream, uint, mUnitOppIDs, uint8, 200);
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVAR(pStream, BProtoObjectID, mSearchType);
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADVAR(pStream, BUnitOppType, mUnitOppType);

   if (BAction::mGameFileVersion >= 20)
   {
      GFREADVAR(pStream, uint16, mUserData);
   }
   else
   {
      uint8 userData;
      GFREADVAR(pStream, uint8, userData);
      mUserData = userData;
   }

   GFREADBITBOOL(pStream, mFlagDoneOnWorkOnce);
   GFREADBITBOOL(pStream, mFlagDoneOnOppComplete);
   GFREADBITBOOL(pStream, mFlagSearchForPotentialTargets);
   GFREADBITBOOL(pStream, mFlagHasSearched);
   GFREADBITBOOL(pStream, mFlagCapturePaid);
   GFREADBITBOOL(pStream, mUserDataSet);

   gSaveGame.remapProtoObjectID(mSearchType);

   return true;
}
