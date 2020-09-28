//==============================================================================
// squadactionhitch.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unitactionmove.h"
#include "squadactionunhitch.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "tactic.h"
#include "triggervar.h"
#include "usermanager.h"
#include "selectionmanager.h"
#include "user.h"
#include "SimOrderManager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionUnhitch, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionUnhitch::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // Check target so we can check range
   if (!validateTarget())
   {
      return (false);
   }

   // Figure our range.  This will end up setting the range into mTarget.
   if (!mTarget.isRangeValid())
   {
      pSquad->calculateRange(&mTarget, NULL);
   }

   // Set the unhitching squad flag
   pSquad->setFlagIsUnhitching(true);
   BSquad* pTarget = gWorld->getSquad(mTarget.getID());
   if (pTarget)
   {
      pTarget->setFlagIsUnhitching(true);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionUnhitch::disconnect()
{   
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // If still hitched on a failure then
   if (pSquad->hasHitchedSquad())

   // Reset the unhitching squad flag
   pSquad->setFlagIsUnhitching(false);
   BSquad* pTarget = gWorld->getSquad(mTarget.getID());
   if (pTarget)
   {
      pTarget->setFlagIsUnhitching(false);
   }

   // Stop our units.
   removeOpp();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionUnhitch::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mTarget.reset();
   mFlagAnyFailed = false;
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
   mFutureState = cStateNone;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionUnhitch::setState(BActionState state)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   switch (state)
   {
      // Unhitching.  Give our units an unhitch opp.
      case cStateWorking:
         {            
            BUnitOpp opp;
            opp.init();
            opp.setTarget(mTarget);
            opp.setType(BUnitOpp::cTypeUnhitch);
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
         // Remove the opp we gave the units.
         removeOpp();

         enableUserAbility();

         // Reset leashing    
         pSquad->setLeashPosition(pSquad->getPosition());
         pSquad->updateObstruction();
         BSquad* pTarget = gWorld->getSquad(mTarget.getID());
         if (pTarget)
         {
            pTarget->setLeashPosition(pTarget->getPosition());
            pTarget->updateObstruction();
         }
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionUnhitch::update(float elapsed)
{
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
         setState(cStateWorking);
         break;

      case cStateWorking:
         break;
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionUnhitch::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   switch (eventType)
   {
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
                     mFutureState = cStateFailed;
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
void BSquadActionUnhitch::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit) || (target.getID().getType() == BEntity::cClassTypeSquad));

   mTarget = target;
}

//==============================================================================
//==============================================================================
bool BSquadActionUnhitch::validateTarget()
{
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 1711
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
bool BSquadActionUnhitch::addOpp(BUnitOpp opp)
{
   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 1713
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
void BSquadActionUnhitch::removeOpp()
{
   if (mUnitOppID == BUnitOpp::cInvalidID)
   {
      return;
   }

   //Remove the opportunity that we've given the unit.  That's all we do here.
//-- FIXING PREFIX BUG ID 1714
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
}

//==============================================================================
// Enable the towing squad's user ability
//==============================================================================
void BSquadActionUnhitch::enableUserAbility()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
      return;   

   // If we still have a hitched squad then bail
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
bool BSquadActionUnhitch::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);  
   GFWRITEVAR(pStream, uint8, mUnitOppIDCount);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEBITBOOL(pStream, mFlagAnyFailed);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionUnhitch::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);  
   GFREADVAR(pStream, uint8, mUnitOppIDCount);
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADBITBOOL(pStream, mFlagAnyFailed);
   return true;
}
