//==============================================================================
// squadactionungarrison.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unitactionmove.h"
#include "squadactionungarrison.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "tactic.h"
#include "triggervar.h"
#include "usermanager.h"
#include "selectionmanager.h"
#include "user.h"
#include "protoobject.h"
#include "commands.h"
#include "SimOrderManager.h"
#include "simhelper.h"
#include "alert.h"
#include "actionmanager.h"
#include "configsgame.h"
//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionUngarrison, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionUngarrison::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }
   
   // Set the ungarrisoning squad flag
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
   BSquad* pSourceSquad = gWorld->getSquad(mSource.getID());

   if (!pTargetSquad)
   {
      BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
      if (!pTargetUnit)
         return false;

      pTargetSquad = pTargetUnit->getParentSquad();

      if (!pTargetSquad)
         return false;
   }

   if (!pSourceSquad)
   {
      BUnit* pSourceUnit = gWorld->getUnit(mSource.getID());
      if (!pSourceUnit)
         return false;

      pSourceSquad = pSourceUnit->getParentSquad();

      if (!pSourceSquad)
         return false;
   }

   BUnit* pSourceUnit = pSourceSquad->getLeaderUnit();
   BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();

   // Doesn't contain squad, bail
   // Halwes - 10/14/2008 - Do not do this check for transporters since it will fail with attached squads
   if (pTargetUnit && pSourceUnit && !pTargetUnit->isType(gDatabase.getOTIDTransporter()) && !pSourceUnit->doesContain(pSquad))
      return false;

   pSquad->setFlagIsUngarrisoning(true);

   // Turn off interpolation on all of our units
   int count = pSquad->getNumberChildren();
   for (int i = 0; i < count; ++i)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         pUnit->setFlagDontInterpolate(true);
         pUnit->setFlagIKDisabled(true);
      }
   }

   if (pTargetUnit)
      pTargetUnit->notify(BEntity::cEventSquadUngarrisonStart, pSquad->getID(), 0, 0);

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionUngarrison::disconnect()
{
   BASSERT(mpOwner);
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);
   BEntity* pTarget = gWorld->getEntity(mTarget.getID());
   //BASSERT(pTarget);

   // Allow target to ungarrison other squads
   if (pTarget && !pTarget->getFlagUngarrisonValid())
   {
      pTarget->setFlagUngarrisonValid(true);            
   }
   mFlagOwnTarget = false;

   // MSC: This is pretty terrible isn't it? We don't want to clear the transporting flag until this thing is on the ground.
   if (pSquad->getFlagIsTransporting())
      pSquad->setFlagIsTransporting(false);

   // Reset the ungarrisoning squad flag
   pSquad->setFlagIsUngarrisoning(false);

   // Stop our units.
   removeOpp();

   // Turn interpolation back on for all of our units
   int count = pSquad->getNumberChildren();
   for (int i = 0; i < count; ++i)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         pUnit->setFlagDontInterpolate(false);
         pUnit->setFlagIKDisabled(false);
      }
   }

   // Tell the target squad's cActionTypeSquadTransport (if that's what created this action) that we're done   
   if (pTarget)
   {
      BEntity* pTargetSquad = gWorld->getEntity(pTarget->getParentID());
      if (pTargetSquad)
      {
         pTargetSquad->notify(BEntity::cEventActionDone, pSquad->getID(), BAction::cActionTypeSquadReinforce, 0);
      }
   }

   if(mFlagAlertWhenComplete)
   {  
      pSquad->getPlayer()->getAlertManager()->createTransportCompleteAlert(pSquad->getPosition(), pSquad->getID());
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionUngarrison::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mTarget.reset();
   mSource.reset();
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
   mFutureState = cStateNone;
   mExitDirection = BProtoObjectStatic::cExitFromFront;
   mRallyPoint = cInvalidVector;
   mFlagUseMaxHeight = false;
   mpParentAction = NULL;
   mFlagAnyFailed = false;
   mFlagOwnTarget = false;
   mFacing = cInvalidVector;
   mSpawnPoint = cInvalidVector;
   mFlagAllowMovingSquadsFromUngarrisonPoint = true;
   mFlagBlocked = false;
   mBlockedTimeStamp = 0;
   mNumBlockedAttempts = 0;
   mFlagAlertWhenComplete = false;
   mIgnoreSpawnPoint = false;
   mInterruptable = false;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionUngarrison::setState(BActionState state)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BEntity* pTarget = gWorld->getEntity(mTarget.getID());
   BASSERT(pTarget);

   switch (state)
   {
      // Ungarrisoning.  Give our units an ungarrison Opp.
      case cStateWorking:
         {
            BSimTarget target = mTarget;
            BVector loc;

            // Calculate the ungarrison position once so that each unit doesn't have to recalculate it.
            BUnit* pTargetUnit = pTarget->getUnit();
            if (!pTargetUnit)
            {
               BSquad* pTargetSquad = pTarget->getSquad();
               if (pTargetSquad)
                  pTargetUnit = pTargetSquad->getLeaderUnit();
            }

            BVector suggestedPos(0.0f);
            static BDynamicSimVectorArray instantiatePositions;
            instantiatePositions.setNumber(0);
            long closestDesiredPositionIndex = -1;

            bool validExit = false;
            if (pTargetUnit)
            {
               validExit = BSimHelper::findInstantiatePositions(1, instantiatePositions, pTargetUnit->getObstructionRadius(), pTargetUnit->getPosition(),
                              pTargetUnit->getForward(), pTargetUnit->getRight(), pSquad->getObstructionRadius(), pTargetUnit->getPosition(), closestDesiredPositionIndex, 4);
            }

            if (validExit && instantiatePositions.size() > 0)
            {
               if(closestDesiredPositionIndex >= 0 && closestDesiredPositionIndex < instantiatePositions.getNumber())
                  target.setPosition(instantiatePositions[closestDesiredPositionIndex]);
               else
                  target.setPosition(instantiatePositions[0]);
            }
            else
            {
               // Default to target's position
               target.setPosition(pTarget->getPosition());
            }

            //if (BSimHelper::calculateExitLocation(pTargetUnit, pSquad, loc))
               //target.setPosition(loc);

            //gpDebugPrimitives->addDebugSphere(target.getPosition(), 9.0f, cDWORDGreen, BDebugPrimitives::cCategoryNone, 10.0f);

            BUnitOpp opp;
            opp.init();
            opp.setTarget(target);
            opp.setType(BUnitOpp::cTypeUngarrison);
            opp.setSource(pSquad->getID());
            opp.generateID();
            opp.setTrigger(true);
            opp.setUserData(mExitDirection);
            opp.setUserData2(mIgnoreSpawnPoint);

            // Override spawn position so it doesn't use the target position
            if (!mSpawnPoint.almostEqual(cInvalidVector))
               opp.addPointToPath(mSpawnPoint);

            if (!addOpp(opp))
            {
               setState(cStateFailed);
               return (true);
            }
         }
         break;

      // Done/Failed.
      case cStateDone:
      case cStateFailed:
         // Remove the opp we gave the units.
         removeOpp();

         // Ungarrison animations may have motion extracted the units so reset the positions based on the units new positions
         BVector avgPos = pSquad->getAveragePosition();
         pSquad->setPosition(avgPos);
         //pSquad->setForward(pTargetSquad->getForward());
         //pSquad->setRight(pTargetSquad->getRight());
         pSquad->setLeashPosition(avgPos);
         pSquad->updateObstruction();
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
            if (pUnit)
            {
               pUnit->updateObstruction();
            }
         }

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
         if (!pSquad->getFlagInCover())
         {
            BSquadAI* pSquadAI = pSquad->getSquadAI();
            if (pSquadAI)
            {
               pSquadAI->setMode(BSquadAI::cModeNormal);
            }
         }

         // Reset the ungarrisoning squad flag
         pSquad->setFlagIsUngarrisoning(false);
        
         // Move to rally point or set facing
         if (!mRallyPoint.almostEqual(cInvalidVector) || !mFacing.almostEqual(cInvalidVector))
         {
            moveToRallyPoint();
         }
         else if (pTargetSquad)
         {            
            const BProtoObject* pTargetProtoObject = pTargetSquad->getProtoObject();
            if (pTargetProtoObject && (!pTargetProtoObject->isType(gDatabase.getOTIDTransporter()) ||pTargetProtoObject->isType(gDatabase.getOTIDCovenant())))
            {
               pSquad->settle();
            }
         }

         // Allow target to ungarrison other squads
         if (!pTarget->getFlagUngarrisonValid())
         {
            pTarget->setFlagUngarrisonValid(true);            
         }
         mFlagOwnTarget = false;

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

         // Notify the unit
         BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());

         if (state == cStateDone)
         {
            if (pTargetUnit)
               pTargetUnit->notify(BEntity::cEventSquadUngarrisonEnd, pSquad->getID(), 0, 0);
         }
         else if (state == cStateFailed)
         {
            if (pTargetUnit)
               pTargetUnit->notify(BEntity::cEventSquadUngarrisonFail, pSquad->getID(), 0, 0);
         }
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionUngarrison::update(float elapsed)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BEntity* pTarget = gWorld->getEntity(mTarget.getID());
   BASSERT(pTarget);

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

   bool transporter = false;
   const BProtoObject* pTargetProtoObject = pTarget->getProtoObject();
   if (pTargetProtoObject && pTargetProtoObject->isType(gDatabase.getOTIDTransporter()))
   {
      transporter = true;
   }   

   BSquad* pTargetSquad = NULL;
   if (mTarget.getID().getType() == BEntity::cClassTypeUnit)
   {
      BUnit* pUnit = gWorld->getUnit(mTarget.getID());
      if (pUnit)
      {
         pTargetSquad = pUnit->getParentSquad();
      }
   }
   else if (mTarget.getID().getType() == BEntity::cClassTypeSquad)
   {
      pTargetSquad = gWorld->getSquad(mTarget.getID());
   }

   switch (mState)
   {
      case cStateNone:
         {
            if (!transporter)
            {
               // If target can ungarrison then reserve our spot otherwise wait
               if (!mFlagOwnTarget && pTarget->getFlagUngarrisonValid())
               {
                  pTarget->setFlagUngarrisonValid(false);
                  mFlagOwnTarget = true;

                  // Should we change the squad mode back to normal?
                  if (pTargetSquad && pTargetProtoObject && (pTargetProtoObject->getGarrisonSquadMode() != -1) && (pTargetSquad->getSquadMode() != BSquadAI::cModeNormal) && !pTargetSquad->getFlagChangingMode())
                  {
                     // Change squad mode to normal
                     BSimOrder* pOrder = gSimOrderManager.createOrder();
                     BASSERT(pOrder);
                     pOrder->setOwnerID(pTargetSquad->getID());
                     pOrder->setPriority(BSimOrder::cPrioritySim);
                     pOrder->setMode((int8)BSquadAI::cModeNormal);
                     pTargetSquad->doChangeMode(pOrder, NULL);                                          
                     break;
                  }                                    
               }
               else if (!mFlagOwnTarget)
               {
                  break;
               }
               else if (pTargetSquad && pTargetSquad->getFlagChangingMode())
               {
                  break;
               }

               // Move squads out of the way
               if (mFlagAllowMovingSquadsFromUngarrisonPoint)
               {
                  // Evaluate the calculated ungarrison location
                  BVector testedUnloadPos = cInvalidVector;
                  BVector unloadPos = calculateUngarrisonPosition();
                  BEntityIDArray ignoreSquads;                 
                  ignoreSquads.add(pTargetSquad->getID());
                  bool blocked = mFlagBlocked;
                  if (!mFlagBlocked && BSimHelper::evaluateExitLocation(pSquad, ignoreSquads, unloadPos, testedUnloadPos))
                  {
                     setSpawnPoint(testedUnloadPos);
                  }
                  // No valid ungarrison location so try to move squads out of the way
                  else if (!mFlagBlocked && (mNumBlockedAttempts < gDatabase.getTransportMaxBlockAttempts()) && BSimHelper::clearExitLocation(pSquad, ignoreSquads, testedUnloadPos, blocked))
                  {
                     setSpawnPoint(testedUnloadPos);
                     // If blocked set time stamp
                     if (blocked)
                     {
                        mBlockedTimeStamp = gWorld->getGametime() + gDatabase.getTransportBlockTime();
                     }
                  }
                  // No valid location and cannot move blocking squads out of the way so just dump them
                  else if (!mFlagBlocked)
                  {
                     BASSERTM(false, "No valid drop off location could be evaluated!");
                     setState(cStateWorking);
                     break;
                  }

                  // Waiting for moving squads to clear blocked location
                  mFlagBlocked = blocked;
                  if (mFlagBlocked)
                  {
                     // If blocked time has expired re-evaluate
                     if (gWorld->getGametime() > mBlockedTimeStamp)
                     {
                        mFlagBlocked = false;
                        mNumBlockedAttempts++;
                     }
                     break;
                  }
               }
            }

            setState(cStateWorking);
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
void BSquadActionUngarrison::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BUnit* pTargetUnit = getTargetUnit();

   switch (eventType)
   {
      // This should only get triggered at the moment by unitactionhotdrop
      case BEntity::cEventActionDone:
         {
            // If we are here then it means that our parent action is no longer valid
            if (mpParentAction && (BActionID)data1 == mpParentAction->getID())
            {
               mpParentAction = NULL;
            }
         }
         break;
      // Check if this squad's units have completed their ungarrison opportunities
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
                  // This is a total bonk so we should just dump the squad out
                  if (mFlagAnyFailed)
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

                              BVector pos = calculateUngarrisonPosition();
                              if (pos.almostEqual(cInvalidVector))
                              {
                                 pos = pSquad->getPosition();
                              }                           
                              // Reset the unit position and orientation
#ifdef SYNC_Unit
                              syncUnitData("BSquadActionUngarrison::notify", pos);
#endif

                              pChildUnit->setPosition(pos);
                              pChildUnit->setForward(pSquad->getForward());
                              pChildUnit->setUp(cYAxisVector);
                              pChildUnit->calcRight();
                              pChildUnit->clearGoalVector();
                              if (pChildUnit->getProtoObject()->getFlagOrientUnitWithGround())
                                 pChildUnit->orientWithGround();
                              pChildUnit->updateObstruction();
                           }
                        }

                        pSquad->settle();
                     }
                     
                     mFutureState = cStateFailed;
                  }
                  else
                  {
                     mFutureState = cStateDone;

                     mInterruptable = true;
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
void BSquadActionUngarrison::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit) || (target.getID().getType() == BEntity::cClassTypeSquad));

   mTarget = target;
}

//==============================================================================
//==============================================================================
bool BSquadActionUngarrison::validateTarget()
{
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 5044
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
bool BSquadActionUngarrison::addOpp(BUnitOpp opp)
{
   // Give our opp to our units.
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (pSquad->addOppToChildren(opp, mUnitOppIDCount))
   {      
      mUnitOppID = opp.getID();      
      pSquad->setFlagUseMaxHeight(mFlagUseMaxHeight);
      return (true);
   }

   return (false);
}

//==============================================================================
//==============================================================================
void BSquadActionUngarrison::removeOpp()
{
   if (mUnitOppID == BUnitOpp::cInvalidID)
   {
      return;
   }

   // Remove the opportunity that we've given the units.  That's all we do here.
//-- FIXING PREFIX BUG ID 5046
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
}

//==============================================================================
//==============================================================================
void BSquadActionUngarrison::moveToRallyPoint()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   BVectorArray wayPoints;
   BVector rallyPoint = pSquad->getPosition();
   if (mRallyPoint != cInvalidVector)
   {
      rallyPoint = mRallyPoint;
      wayPoints.add(rallyPoint);
   }

   if (mFacing != cInvalidVector)
   {  
      BVector offsetPoint = mFacing;
      offsetPoint.normalize();
      offsetPoint *= (2.0f * pSquad->getObstructionRadius());
      offsetPoint += rallyPoint;  
      wayPoints.add(offsetPoint);
   }

   // Build the move command.
   BWorkCommand tempCommand;
   tempCommand.setWaypoints(wayPoints.getPtr(), wayPoints.getNumber());
   tempCommand.setFlag(BWorkCommand::cFlagAlternate, true);

   // Put the squad in its own army
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = pSquad->getPlayerID();
   BArmy* pArmy = gWorld->createArmy(objectParms);
   if (!pArmy)
      return;
   BEntityIDArray workingSquads;
   workingSquads.add(pSquad->getID());
   if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
      pArmy->addSquads(workingSquads, false);
   else
      pArmy->addSquads(workingSquads, true);


   // Set the command to the army
   tempCommand.setRecipientType(BCommand::cArmy);
   if (mpOrder)
   {
      // Set the appropriate sender type
      switch (mpOrder->getPriority())
      {
         case BSimOrder::cPriorityUser:
            tempCommand.setSenderType(BCommand::cPlayer);
            break;

         case BSimOrder::cPriorityTrigger:
            tempCommand.setSenderType(BCommand::cTrigger);
            break;

         default:
            tempCommand.setSenderType(BCommand::cSquad);
            break;
      }

      // Set the attack move
      tempCommand.setFlag(BWorkCommand::cFlagAttackMove, mpOrder->getAttackMove());
   }
   else
   {
      // Set the appropriate sender type
      tempCommand.setSenderType(BCommand::cSquad);

      // Set the attack move
      tempCommand.setFlag(BWorkCommand::cFlagAttackMove, false);
   }
   
   //XXXHalwes - 11/13/2007 - If a transporter, then remove the transporter's obstruction so that it doesn't interfere with the ungarrisoned squad
   BUnit* pTargetUnit = getTargetUnit();
   if (pTargetUnit)
   {
      const BProtoObject* pProtoObject = pTargetUnit->getProtoObject();
      if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDTransporter()))
      {
         pTargetUnit->deleteObstruction();
      }
   }      

   //Give the command to the army.
   pArmy->queueOrder(&tempCommand);  
}

//==============================================================================
// Calculate the ungarrison position
//==============================================================================
BVector BSquadActionUngarrison::calculateUngarrisonPosition()
{
   // Use spawn point override if specified
   if (!mSpawnPoint.almostEqual(cInvalidVector))
      return mSpawnPoint;

   BVector result = cInvalidVector;
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   
   BVector targetPos = pSquad->getPosition();
   BVector targetDir = pSquad->getForward();
   float targetObs = pSquad->getObstructionRadius();
   BSquad* pTargetSquad = NULL;
   const BUnit* pTargetUnit = NULL;
   if (mTarget.getID().getType() == BEntity::cClassTypeSquad)
   {
      pTargetSquad = gWorld->getSquad(mTarget.getID());
      if (pTargetSquad)
      {
         pTargetUnit = pTargetSquad->getLeaderUnit();
         if (pTargetUnit)
         {
            targetPos = pTargetUnit->getPosition();
            targetDir = pTargetUnit->getForward();
            targetObs = pTargetUnit->getObstructionRadius();
         }
      }
   }
   else
   {
//-- FIXING PREFIX BUG ID 5047
      const BEntity* pTargetEntity = gWorld->getEntity(mTarget.getID());
//--
      if (pTargetEntity)
      {
         targetPos = pTargetEntity->getPosition();
         targetDir = pTargetEntity->getForward();
         targetObs = pTargetEntity->getObstructionRadius();

         pTargetUnit = pTargetEntity->getUnit();
         if (pTargetUnit)
            pTargetSquad = pTargetUnit->getParentSquad();
      }
   }

   targetDir.normalize();
   targetDir *= (targetObs + (pSquad->getObstructionRadius() * 2.0f) + gTerrainSimRep.getDataTileScale());
   result = targetPos + targetDir;      

   // If UBL position is blocked calculate a new position
   BEntityIDArray ignoreSquads;
   if (pTargetSquad)
      ignoreSquads.add(pTargetSquad->getID());
   BVector suggestedLoc = cInvalidVector;
   if (!BSimHelper::verifyExitLocation(pSquad, ignoreSquads, result) && pTargetUnit && BSimHelper::calculateExitLocation(pTargetUnit, pSquad, suggestedLoc))
   {
      result = suggestedLoc;
   }

   return (result);
}

//==============================================================================
// Resolve the target unit
//==============================================================================
BUnit* BSquadActionUngarrison::getTargetUnit()
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

      // [10-28-08 CJS] Putting this back, as it is here to catch a NULL target unit, which should never happen.
      BASSERT(pTargetUnit);
   }   

   return (pTargetUnit);
}

//==============================================================================
//==============================================================================
bool BSquadActionUngarrison::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVECTOR(pStream, mRallyPoint);
   GFWRITEVECTOR(pStream, mFacing);
   GFWRITEVECTOR(pStream, mSpawnPoint);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);      
   GFWRITEACTIONPTR(pStream, mpParentAction);
   GFWRITEVAR(pStream, DWORD, mBlockedTimeStamp);
   GFWRITEVAR(pStream, uint, mNumBlockedAttempts);
   GFWRITEVAR(pStream, uint8, mUnitOppIDCount);
   GFWRITEVAR(pStream, uint8, mExitDirection);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEBITBOOL(pStream, mFlagUseMaxHeight);
   GFWRITEBITBOOL(pStream, mFlagAnyFailed);
   GFWRITEBITBOOL(pStream, mFlagOwnTarget);
   GFWRITEBITBOOL(pStream, mFlagAllowMovingSquadsFromUngarrisonPoint);
   GFWRITEBITBOOL(pStream, mFlagBlocked);
   GFWRITEBITBOOL(pStream, mFlagAlertWhenComplete);
   GFWRITEBITBOOL(pStream, mIgnoreSpawnPoint);
   GFWRITEBITBOOL(pStream, mInterruptable);

   GFWRITECLASS(pStream, saveType, mSource);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionUngarrison::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVECTOR(pStream, mRallyPoint);
   GFREADVECTOR(pStream, mFacing);
   GFREADVECTOR(pStream, mSpawnPoint);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);      
   GFREADACTIONPTR(pStream, mpParentAction);
   GFREADVAR(pStream, DWORD, mBlockedTimeStamp);
   GFREADVAR(pStream, uint, mNumBlockedAttempts);
   GFREADVAR(pStream, uint8, mUnitOppIDCount);
   GFREADVAR(pStream, uint8, mExitDirection);
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADBITBOOL(pStream, mFlagUseMaxHeight);
   GFREADBITBOOL(pStream, mFlagAnyFailed);
   GFREADBITBOOL(pStream, mFlagOwnTarget);
   GFREADBITBOOL(pStream, mFlagAllowMovingSquadsFromUngarrisonPoint);
   GFREADBITBOOL(pStream, mFlagBlocked);
   GFREADBITBOOL(pStream, mFlagAlertWhenComplete);
   GFREADBITBOOL(pStream, mIgnoreSpawnPoint);
   
   if (BAction::mGameFileVersion >= 36)
   {
      GFREADBITBOOL(pStream, mInterruptable);
   }

   if (BAction::mGameFileVersion >= 26)
   {
      GFREADCLASS(pStream, saveType, mSource);
   }

   return true;
}
