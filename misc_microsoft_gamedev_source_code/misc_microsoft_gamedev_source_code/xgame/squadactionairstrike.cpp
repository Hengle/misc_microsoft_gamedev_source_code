//==============================================================================
// squadactionairstrike.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactionairstrike.h"
#include "squadactionmove.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "unitactionrangedattack.h"
#include "unitactionmoveair.h"
#include "squadactionattack.h"
#include "army.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionAirStrike, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionAirStrike::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionAirStrike::disconnect()
{
   BASSERT(mpOwner);

   removeChildActions();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionAirStrike::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mChildActionID = cInvalidActionID;
   mFutureState = cStateNone;
   mLoiterTime = 0.0f;
   mbOnStation = false;
   mbAutoReturnToBase = true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionAirStrike::setState(BActionState state)
{
   BASSERT(mpOwner);

   switch (state)
   {
      // Bombs away!
      case cStateWorking:
         break;

      // Fly off strikers
      case cStateReturning:
/*
         if (!returnToBase())
         {
            setState(cStateFailed);
            return (false);
         }
*/
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
bool BSquadActionAirStrike::update(float elapsed)
{
   BASSERT(mpOwner);
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);

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
            if (!strike())
            {
               setState(cStateFailed);
               return (false);
            }
            setState(cStateWorking);
         }
         else
         {
            setState(cStateDone);
         }            
         break;

      // Bombs away
      case cStateWorking:
         if (!mbOnStation)
            checkOnStation();
         // Hack: to support flood swarm fliers until we have a design
         if (mbAutoReturnToBase)
            checkAmmoAndFuel(elapsed);
         break;

      // Fly off strikers
      case cStateReturning:
         if (mbOnStation && (mStartingLocation.distance(pSquad->getPosition()) < 10.0f))
            mFutureState = cStateNone;
         break;
   }

   #if !defined (BUILD_FINAL)
      if (gConfig.isDefined(cConfigRenderSimDebug))
      {      
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

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionAirStrike::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
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
                  if (mAirStrikeLocation != cInvalidVector)
                  {
                     mFutureState = cStateWorking;
                  }
                  else
                  {
                     mFutureState = cStateFailed;
                  }
                  break;

               // Fly off strikers
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
                  if (mAirStrikeLocation != cInvalidVector)
                  {
                     mFutureState = cStateWorking;
                  }
                  else
                  {
                     mFutureState = cStateReturning;
                  }
                  break;

               // Fly off strikers
               case cStateReturning:

                  mFutureState = cStateDone;
                  break;
            }            
         }
         break;
   }    
}

//===========================================================================================================================
// Static function used to fly in strikers, engage targets, and fly back to base
//===========================================================================================================================
bool BSquadActionAirStrike::airStrike(BSimOrder* pSimOrder, BEntityID strikerSquadID, BSimTarget* target, BVector launchLocation)
{
   BSquad* pStrikerSquad = gWorld->getSquad(strikerSquadID);
   if (pStrikerSquad)
   {
      pStrikerSquad->setFlagIgnoreLeash(true);
      BSquadActionAirStrike* pAction = (BSquadActionAirStrike*)gActionManager.createAction(BAction::cActionTypeSquadAirStrike);
      if (pAction)
      {
         //XXXHalwes - 9/18/2007 - SquadActionAirStrike should take a BSimTarget and be able to attack a moving target or a ground location.
         pAction->setAirStrikeLocation(target->getPosition());
         pAction->setStartingLocation(launchLocation);
         pStrikerSquad->addAction(pAction, pSimOrder);

         return (true);
      }
   }

   return (false);
}

//==============================================================================
// Setup the attack locations
//==============================================================================
bool BSquadActionAirStrike::setupAttacks()
{
//-- FIXING PREFIX BUG ID 4985
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--

   BObjectCreateParms objectParms;
   objectParms.mPlayerID = pSquad->getPlayerID();
   BArmy* pArmy = gWorld->createArmy(objectParms);
   if (pArmy)
   {
      BEntityIDArray squadList;
      squadList.uniqueAdd(pSquad->getID());
      if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
         pArmy->addSquads(squadList, false);
      else
         pArmy->addSquads(squadList);
   }

   if ((mAirStrikeLocation == cInvalidVector) || (mStartingLocation == cInvalidVector))
   {
      return (false);
   }
   mbOnStation = false;

   // FIXME: if this target is a squad, find out whether it is a ground unit. If so, set attack priority to ground units

   return (true);
}

//==============================================================================
// Sic 'em!
//==============================================================================
bool BSquadActionAirStrike::strike()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   BSimTarget target(mAirStrikeLocation);

   BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   if (pPlatoon)
   {
      pPlatoon->queueMove(target, BSimOrder::cPriorityUser);
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
   else
      return (false);
}

//==============================================================================
// Move the strikers from their current location to the launch location
//==============================================================================
bool BSquadActionAirStrike::returnToBase()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   BSimTarget target(mStartingLocation);   

   BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   if (pPlatoon)
   {
      pPlatoon->removeAllOrders();
      pPlatoon->queueMove(target, BSimOrder::cPriorityUser);

      // Tell the units' move air actions they have to go home
      // This allows them to refuse further orders until they have landed
      uint numChildren = pSquad->getNumberChildren();
      for (uint i = 0; i < numChildren; i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit)
         {
            BUnitActionMoveAir* pMoveAction = reinterpret_cast<BUnitActionMoveAir*>(pUnit->getActionByType(BAction::cActionTypeUnitMoveAir));
            if (pMoveAction)
               pMoveAction->returnToBase();
         }
      }
   }
   return (true);
}

//==============================================================================
// Remove cached child action
//==============================================================================
void BSquadActionAirStrike::removeChildActions()
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
bool BSquadActionAirStrike::actionsComplete(BActionID id)
{
   return (mChildActionID == id);
}

//===================================================================================================================
// Check ammo and fuel
//===================================================================================================================
void BSquadActionAirStrike::checkAmmoAndFuel(float elapsed)
{
   bool bReturnToBase = false;

//-- FIXING PREFIX BUG ID 4987
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   float onStationTime = gDatabase.getAirStrikeLoiterTime();

   // Find out if the squad members have the ammo they need in order to fire
   uint numChildren = pSquad->getNumberChildren();
   bool bOutOfAmmo = true;
   bool bAttacking = false;
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         BUnitActionRangedAttack* pAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack));
         if (pAttackAction)
         {
            bAttacking = true;
            if (pAttackAction->hasEnoughAmmoForFullAttack())
               bOutOfAmmo = false;
         }
      }
   }

   if ((bAttacking && bOutOfAmmo) || (mLoiterTime > onStationTime))
      bReturnToBase = true;

   if (!bAttacking && mbOnStation)
      mLoiterTime += elapsed;

   if (bReturnToBase)
   {
      mFutureState = cStateReturning;
      mAirStrikeLocation = cInvalidVector; // Hack: BUnitActionMoveAir uses this to signal return to base
      returnToBase();

   }
}

//===================================================================================================================
// Rearm the strikers
//===================================================================================================================
void BSquadActionAirStrike::rearm()
{
//-- FIXING PREFIX BUG ID 4988
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   uint numChildren = pSquad->getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         pUnit->setAmmunition(pUnit->getAmmoMax());
      }
   }
}

//===================================================================================================================
// Are we there yet?
//===================================================================================================================
void BSquadActionAirStrike::checkOnStation()
{
//-- FIXING PREFIX BUG ID 4989
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
//-- FIXING PREFIX BUG ID 4990
   const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
//--

   if (mAirStrikeLocation.distance(pUnit->getPosition()) < 30.0f)
      mbOnStation = true;
}

//==============================================================================
//==============================================================================
bool BSquadActionAirStrike::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVECTOR(pStream, mStartingLocation);
   GFWRITEVECTOR(pStream, mAirStrikeLocation);
   GFWRITEVAR(pStream, BEntityID, mStrikeSquad);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEVAR(pStream, float, mLoiterTime);
   GFWRITEVAR(pStream, bool, mbOnStation);
   GFWRITEVAR(pStream, bool, mbAutoReturnToBase);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionAirStrike::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVECTOR(pStream, mStartingLocation);
   GFREADVECTOR(pStream, mAirStrikeLocation);
   GFREADVAR(pStream, BEntityID, mStrikeSquad);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADVAR(pStream, float, mLoiterTime);
   GFREADVAR(pStream, bool, mbOnStation);
   GFREADVAR(pStream, bool, mbAutoReturnToBase);
   return true;
}
