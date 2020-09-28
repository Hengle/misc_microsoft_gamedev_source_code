//==============================================================================
// squadactionchangemode.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "squadactionchangemode.h"
#include "actionmanager.h"
#include "squad.h"
#include "tactic.h"
#include "unit.h"
#include "unitactionchangemode.h"
#include "world.h"
#include "visualitem.h"
#include "ability.h"
#include "selectionmanager.h"
#include "user.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionChangeMode, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionChangeMode::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return false;

   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   pSquad->setFlagChangingMode(true);
   pSquad->setChangingToSquadMode(mSquadMode);

   return true;
}

//==============================================================================
//==============================================================================
void BSquadActionChangeMode::disconnect()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (pSquad->getFlagChangingMode())
   {
      pSquad->setFlagChangingMode(false);
      pSquad->setChangingToSquadMode(-1);
   }
   removeOpp();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionChangeMode::init()
{
   if (!BAction::init())
      return false;
   mpParentAction = NULL;
   mUnitOppID = BUnitOpp::cInvalidID;
   mFutureState = cStateNone;
   mSquadMode = 0;
   mFlagConflictsWithIdle = true;
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionChangeMode::setState(BActionState state)
{
   BSquad* pSquad=mpOwner->getSquad();

   switch(state)
   {
      case cStateWorking:
      {
         BUnitOpp opp;
         opp.init();
         
         opp.setType(BUnitOpp::cTypeChangeMode);
         opp.setUserData(mSquadMode);
         opp.setSource(pSquad->getID());
         opp.generateID();
         if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
            opp.setTrigger(true);
         else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
            opp.setPriority(BUnitOpp::cPriorityCommand);
         else
            opp.setPriority(BUnitOpp::cPrioritySquad);
            
         //Fail if we can't do the opp.
         if (!addOpp(opp))
         {
            setState(cStateFailed);
            return (true);
         }

         // Let the other units/actions know that we're changing the squad mode now.
         for (uint i=0; i<mpOwner->getNumberChildren(); i++)
         {  
            BUnit* pUnit=gWorld->getUnit(pSquad->getChild(i));
            if (pUnit)
               pUnit->notify(BEntity::cEventSquadModeChanging, pUnit->getID(), mSquadMode, 0);
         }            

         // If this squad has a lockdown menu and it's going to lock down, remove it from the selection
         const BProtoObject* pProtoObject = mpOwner->getProtoObject();
         if ((pSquad->getChangingToSquadMode() == BSquadAI::cModeLockdown) && pProtoObject && pProtoObject->getFlagLockdownMenu())
         {
            BPlayer* pPlayer = (BPlayer*) mpOwner->getPlayer();
            if (pPlayer)
            {
               BUser* pUser = pPlayer->getUser();
               if (pUser)
               {
                  BEntityID squadID = mpOwner->getID();
                  BSelectionManager* pSelectionManager = pUser->getSelectionManager();
                  if (pSelectionManager && pSelectionManager->isSquadSelected(squadID))
                  {
                     pSelectionManager->unselectSquad(squadID);
                  }
               }
            }
         }

         // Stop the movement sound if there is one playing and we're going into lockdown
         if(pSquad->getChangingToSquadMode() == BSquadAI::cModeLockdown)
            pSquad->playMovementSound(false, true);

         break;
      }

      case cStateDone:
      {
         // Remove the unit opportunity
         removeOpp();

         //Notify our parent, if any.
         if (mpParentAction)
            mpParentAction->notify(BEntity::cEventActionDone, mpOwner->getID(), getID(), 0);

         // Update the unit obstructions.  If this change mode was preceded by a move then the moving
         // flag just got set back to false when notify was called above.  To make sure the unit has
         // a non-moving obstruction, update the obstructions.  This fixes a bug where units could
         // path through locked down units.
         if (pSquad)
         {
            for (uint i = 0; i < pSquad->getNumberChildren(); i++)
            {
               BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
               if (pUnit)
                  pUnit->updateObstruction();
            }
         }
         break;
      }

      case cStateFailed:
      {
         // Remove the unit opportunity
         removeOpp();

         //Notify our parent, if any.
         if (mpParentAction)
            mpParentAction->notify(BEntity::cEventActionFailed, mpOwner->getID(), getID(), 0);
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionChangeMode::update(float elapsed)
{
   //If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
         return (true);
      mFutureState=cStateNone;
   }

   switch (mState)
   {
      case cStateNone:
      {                
         setState(cStateWorking);
         break;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BSquadActionChangeMode::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   switch (eventType)
   {
      case BEntity::cEventOppComplete:
      {
         //Data1:  OppID.
         //Data2:  Success.
         if (data1 == mUnitOppID)
         {
            if (data2)
            {
               // Set the final squad mode.  Need to do this now before unit's recalculate their new animation,
               // otherwise they'll base it off of the old mode.
               BSquad* pSquad = mpOwner->getSquad();
               BASSERT(pSquad);
               pSquad->setFlagChangingMode(false);
               pSquad->setChangingToSquadMode(-1);

               // apply the mode modifier if there is one. 
               if (mTarget.isAbilityIDValid())
               {
                  int abilityID = gDatabase.getSquadAbilityID(pSquad, mTarget.getAbilityID());
//-- FIXING PREFIX BUG ID 4922
                  const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
//--
                  if (pAbility && pAbility->getMovementModifierType() == BAbility::cMovementModifierMode)
                     pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierMode, pAbility->getMovementSpeedModifier(), false);
               }

               pSquad->getSquadAI()->setMode(mSquadMode);

               mFutureState=cStateDone;
            }
            else
               mFutureState=cStateFailed;
         }
         break;
      }
   }    
}

//==============================================================================
//==============================================================================
bool BSquadActionChangeMode::addOpp(BUnitOpp opp)
{
   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 4924
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   if (pSquad->addOppToChildren(opp))
   {
      mUnitOppID=opp.getID();
      return (true);
   }
   return (false);
}

//==============================================================================
//==============================================================================
void BSquadActionChangeMode::removeOpp()
{
   if (mUnitOppID == BUnitOpp::cInvalidID)
      return;

   //Remove the opportunity that we've given the units.  That's all we do here.
//-- FIXING PREFIX BUG ID 4925
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID=BUnitOpp::cInvalidID;
}

//==============================================================================
//==============================================================================
bool BSquadActionChangeMode::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEACTIONPTR(pStream, mpParentAction);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEVAR(pStream, int8, mSquadMode);
   GFWRITECLASS(pStream, saveType, mTarget);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionChangeMode::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADACTIONPTR(pStream, mpParentAction);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADVAR(pStream, int8, mSquadMode);
   GFREADCLASS(pStream, saveType, mTarget);

   return true;
}
