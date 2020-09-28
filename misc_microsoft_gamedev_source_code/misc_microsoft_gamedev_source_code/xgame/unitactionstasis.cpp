//==============================================================================
// unitactionstasis.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionstasis.h"
#include "unit.h"
#include "squad.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionStasis, 5, &gSimHeap);

//==============================================================================
//==============================================================================
BUnitActionStasis::BUnitActionStasis()
{
}


//==============================================================================
//==============================================================================
void BUnitActionStasis::disconnect()
{
//-- FIXING PREFIX BUG ID 1247
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   //Release our controllers.
   releaseControllers();

   BAction::disconnect();
}


//==============================================================================
//==============================================================================
bool BUnitActionStasis::init()
{
   if (!BAction::init())
      return(false);

   return(true);
}


//==============================================================================
//==============================================================================
bool BUnitActionStasis::update(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   BSquad *pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);

   if(pUnit && pSquad)
   {
      float stasisSpeedMult = 1.0f;
      int numStasisEffects = pSquad->getNumStasisEffects();

      int numStasisFieldsToStop = pUnit->getProtoObject()->getNumStasisFieldsToStop();
      if (numStasisFieldsToStop < 1)
         numStasisFieldsToStop = 1;

      if (numStasisEffects >= numStasisFieldsToStop)
         stasisSpeedMult = 0.0f;
      else if (numStasisEffects > 0)
         stasisSpeedMult = 1.0f - ((float)numStasisEffects / (float)numStasisFieldsToStop);

      if (stasisSpeedMult != pSquad->getStasisSpeedMult())
      {
         pSquad->setStasisSpeedMult(stasisSpeedMult);
         if (numStasisEffects >= numStasisFieldsToStop)
         {
            pSquad->setFlagAttackBlocked(true);
            pSquad->stop();
            pSquad->removeActions();

            grabControllers();
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, cAnimTypeStasis);
         }
         else
         {
            pSquad->setFlagAttackBlocked(false);
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, cAnimTypeIdle);
            releaseControllers();
         }

         if (numStasisEffects <= 0)
            return (false);
      }
   }

   return BAction::update(elapsed);
}


//==============================================================================
//==============================================================================
bool BUnitActionStasis::grabControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Take them all.
   if (!pUnit->grabController(BActionController::cControllerOrient, this, getOppID()))
      return (false);
   if (!pUnit->grabController(BActionController::cControllerAnimation, this, getOppID()))
   {
      pUnit->releaseController(BActionController::cControllerOrient, this);
      return (false);
   }

   return (true);
}


//==============================================================================
//==============================================================================
void BUnitActionStasis::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

