//==============================================================================
// squadactioncarpetbomb.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactioncarpetbomb.h"
#include "unitactionmoveair.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "SimOrderManager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionCarpetBomb, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionCarpetBomb::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionCarpetBomb::disconnect()
{
   BASSERT(mpOwner);

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
bool BSquadActionCarpetBomb::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mChildActionID = cInvalidActionID;
   mFutureState = cStateNone;
   mpChildOrder = NULL;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionCarpetBomb::setState(BActionState state)
{
   BASSERT(mpOwner);

   switch (state)
   {
      // Bombs away!
      case cStateWorking:
         if (!bombsAway())
         {
            setState(cStateFailed);
            return (false);
         }
         break;

      // Fly off bombers
      case cStateReturning:
         if (!flyOffTo())
         {
            setState(cStateFailed);
            return (false);
         }
         break;

      // Done/Failed.
      case cStateDone:
      case cStateFailed:
         removeChildActions();
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionCarpetBomb::update(float elapsed)
{
   BASSERT(mpOwner);

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

   switch (mState)
   {
      // First update
      case cStateNone:
         if (setupAttacks())
         {
            setState(cStateWorking);
         }
         else
         {
            setState(cStateDone);
         }            
         break;

      // Bombs away
      case cStateWorking:
         break;

      // Fly off bombers
      case cStateReturning:
         break;
   }

   #if !defined (BUILD_FINAL)
      if (gConfig.isDefined(cConfigRenderSimDebug))
      {      
         BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
         BASSERT(pSquad);

         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
            if (pUnit)
               pUnit->debugRender();
         }

         pSquad->debugRender();

         BPlatoon* pPlatoon = pSquad->getParentPlatoon();
         if (pPlatoon)
            pPlatoon->debugRender();
      }
   #endif

      BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
      BVector unitPos = pUnit->getPosition();
      BVector squadPos = pSquad->getPosition();
      unitPos.y = squadPos.y = mCarpetBombLocation.y;
      float unitSquadDist = unitPos.distance(squadPos);
      float squadTargetDist = squadPos.distance(mCarpetBombLocation);
      if (unitSquadDist < 10.0f && squadTargetDist < 10.0f)
      {
         setState(cStateReturning);
         mCarpetBombLocation = cInvalidVector; // Hack: BUnitActionMoveAir uses this to signal return to base
      }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionCarpetBomb::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   switch (eventType)
   {
      case BEntity::cEventActionFailed:
         if (actionsComplete((BActionID)data1))
         {
            switch (mState)
            {
               // Bombs away
               case cStateWorking:
                  if (mAttackLocations.getNumber() > 0)
                  {
                     mFutureState = cStateWorking;
                  }
                  else
                  {
                     mFutureState = cStateFailed;
                  }
                  break;

               // Fly off bombers
               case cStateFlyOff:
                  mFutureState = cStateFailed;
                  break;
            }            
         }
         break;
      
      case BEntity::cEventActionDone:

         if (actionsComplete((BActionID)data1))
         {
            switch (mState)
            {
               // Bombs away
               case cStateWorking:
                  if (mAttackLocations.getNumber() > 0)
                  {
                     mFutureState = cStateWorking;
                  }
                  break;

               // Fly off bombers
               case cStateReturning:
                  mFutureState = cStateDone;
                  break;
            }            
         }
         break;
   }    
}

//===========================================================================================================================
// Static function used to fly in bombers, drop bombs, and fly back to base
//===========================================================================================================================
bool BSquadActionCarpetBomb::carpetBomb(BSimOrder* pSimOrder, BEntityID bomberSquadID, BVector targetLocation, BVector launchLocation, float attackRunDistance /*= cDefaultAttackRunDistance*/, uint numAttacks /*= cDefaultAttackNumber*/)
{   
   BSquad* pBomberSquad = gWorld->getSquad(bomberSquadID);
   if (pBomberSquad)
   {
      pBomberSquad->setFlagIgnoreLeash(true);
      BSquadActionCarpetBomb* pAction = (BSquadActionCarpetBomb*)gActionManager.createAction(BAction::cActionTypeSquadCarpetBomb);
      if (pAction)
      {
         pAction->setCarpetBombLocation(targetLocation);
         pAction->setStartingLocation(launchLocation);
         pAction->setAttackRunDistance(attackRunDistance);
         pAction->setNumberAttacks(numAttacks);
         pBomberSquad->addAction(pAction, pSimOrder);

         return (true);
      }
   }

   return (false);
}

//==============================================================================
// Setup the attack locations
//==============================================================================
bool BSquadActionCarpetBomb::setupAttacks()
{
   if ((mCarpetBombLocation == cInvalidVector) || (mStartingLocation == cInvalidVector))
   {
      return (false);
   }

   if (mNumAttacks <= 0)
   {
      mNumAttacks = 1;
   }

   // Crappy straight shot direction for now.
   BVector vec = mCarpetBombLocation - mStartingLocation;
   vec.y = 0.0f;
   vec.normalize();

   uint numSpans = mNumAttacks - 1;
   if ((numSpans == 0) || (mAttackRunDistance == 0.0f))
   {
      mAttackLocations.setNumber(1);
      mAttackLocations[0] = mCarpetBombLocation;
   }
   else
   {
      mAttackLocations.setNumber(mNumAttacks);
      float spanDist = mAttackRunDistance / numSpans;
      float addDist = 0.0f;
      float halfRunDist = mAttackRunDistance * 0.5f;
      for (uint i = 0; i < mNumAttacks; i++)
      {
         mAttackLocations[i] = mCarpetBombLocation + (vec * addDist) - (vec * halfRunDist);
         addDist += spanDist;
      }
   }

   // Launch the aircraft
//-- FIXING PREFIX BUG ID 1686
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   uint numChildren = pSquad->getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         BUnitActionMoveAir* pMoveAction = reinterpret_cast<BUnitActionMoveAir*>(pUnit->getActionByType(BAction::cActionTypeUnitMoveAir));
         if (pMoveAction)
            pMoveAction->launch(true);
      }
   }

   return (true);
}

//==============================================================================
// Move the bombers from their current location to the target location
//==============================================================================
bool BSquadActionCarpetBomb::flyInFrom()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // Have our squad move.  If it can't, fail.
   bool platoonMove = false;
   if (mpOrder && (mpOrder->getOwnerID() == pSquad->getParentID()))
   {
      platoonMove = true;
   }

   uint numChildren = pSquad->getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         BUnitActionMoveAir* pMoveAction = reinterpret_cast<BUnitActionMoveAir*>(pUnit->getActionByType(BAction::cActionTypeUnitMoveAir));
         if (pMoveAction)
            pMoveAction->launch(true);
      }
   }

   BSimTarget target(mAttackLocations[0]);
   setupChildOrder(mpOrder);
   if (mpChildOrder)
   {
      mpChildOrder->setTarget(target);
   }
   mChildActionID = pSquad->doMove(mpChildOrder, this, &target, platoonMove, true);
   if (mChildActionID == cInvalidActionID)
   {
      return (false);
   }   

   return (true);
}

//==============================================================================
// Drop da bombs!
//==============================================================================
bool BSquadActionCarpetBomb::bombsAway()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (mAttackLocations.getNumber() == 0)
   {
      return (false);
   }

   // Have our squad attack.  If it can't, fail.
   BSimTarget target(mAttackLocations[0]);
   setupChildOrder(mpOrder);
   if (mpChildOrder)
   {
      mpChildOrder->setTarget(target);
   }
   mChildActionID = pSquad->doCarpetBomb(mpChildOrder, this, &target);
   if (mChildActionID == cInvalidActionID)
   {
      return (false);
   }

   mAttackLocations.removeIndex(0);

   return (true);
}

//==============================================================================
// Move the bombers from their current location to the launch location
//==============================================================================
bool BSquadActionCarpetBomb::flyOffTo()
{
   // Tell the units' move air actions they have to go home
   // This allows them to refuse further orders until they have landed
//-- FIXING PREFIX BUG ID 1687
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   uint numChildren = pSquad->getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         BUnitActionMoveAir* pMoveAction = reinterpret_cast<BUnitActionMoveAir*>(pUnit->getActionByType(BAction::cActionTypeUnitMoveAir));
         if (pMoveAction)
         {
            pMoveAction->returnToBase();
         }
      }
   }

   return (true);
}

//==============================================================================
// Remove cached child action
//==============================================================================
void BSquadActionCarpetBomb::removeChildActions()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // Remove the child action.
   pSquad->removeActionByID(mChildActionID);

   mChildActionID = cInvalidActionID;
}

//===================================================================================================================
// Check action against cached child action
//===================================================================================================================
bool BSquadActionCarpetBomb::actionsComplete(BActionID id)
{
   return (mChildActionID == id);
}

//==============================================================================
// Setup the child order based on the parent order
//==============================================================================
void BSquadActionCarpetBomb::setupChildOrder(BSimOrder* pOrder)
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
bool BSquadActionCarpetBomb::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVECTORARRAY(pStream, mAttackLocations, uint8, 100);
   GFWRITEVECTOR(pStream, mStartingLocation);
   GFWRITEVECTOR(pStream, mCarpetBombLocation);
   GFWRITEVAR(pStream, BEntityID, mBomberSquad);
   GFWRITEFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFWRITEVAR(pStream, float, mAttackRunDistance);      
   GFWRITEVAR(pStream, uint, mNumAttacks);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionCarpetBomb::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVECTORARRAY(pStream, mAttackLocations, uint8, 100);
   GFREADVECTOR(pStream, mStartingLocation);
   GFREADVECTOR(pStream, mCarpetBombLocation);
   GFREADVAR(pStream, BEntityID, mBomberSquad);
   GFREADFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFREADVAR(pStream, float, mAttackRunDistance);      
   GFREADVAR(pStream, uint, mNumAttacks);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVAR(pStream, BActionState, mFutureState);
   return true;
}
