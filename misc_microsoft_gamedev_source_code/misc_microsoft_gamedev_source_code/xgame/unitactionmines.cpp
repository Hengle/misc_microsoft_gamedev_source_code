//==============================================================================
// unitactionmines.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "Common.h"
#include "UnitActionMines.h"
#include "Ability.h"
#include "Database.h"
#include "Squad.h"
#include "Tactic.h"
#include "Unit.h"
#include "UnitQuery.h"
#include "World.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionMines, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionMines::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(mpProtoAction);
   //Validate our ability.
//-- FIXING PREFIX BUG ID 5026
   const BAbility* pAbility = gDatabase.getAbilityFromID(mTarget.getAbilityID());
//--
   BASSERT(pAbility);

   if (!BAction::connect(pOwner, pOrder) || !pAbility)
      return (false);

   //Take our controllers.  If we can't get them, fail.
   if (!grabControllers())
   {
      BAction::disconnect();
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMines::disconnect()
{
   //Release our controllers.
   //BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner); // mrh 8/8/07 - removed warning.
   //BASSERT(pUnit);
   BASSERT(mpOwner);
   releaseControllers();
   
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::init()
{
   if (!BAction::init())
      return(false);

   mFlagConflictsWithIdle=true;
      
   mTarget.reset();
   mOppID=BUnitOpp::cInvalidID;
   mPlacedAny=false;
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::setState(BActionState state)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch(state)
   {
      case cStateWorking:
      {
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateWork, mpProtoAction->getAnimType());
         pUnit->computeAnimation();
         BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateWork, mpProtoAction->getAnimType()));
         break;
      }

      case cStateDone:
      case cStateFailed:
      case cStateNone:
         if (state == cStateDone)
            pUnit->completeOpp(mOppID, true);
         else if (state == cStateFailed)
            pUnit->completeOpp(mOppID, false);
      
         releaseControllers();
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (mState)
   {
      case cStateNone:
         //Check range.
         if (!validateRange())
         {
            if (mPlacedAny)
               setState(cStateDone);
            else
               setState(cStateFailed);
            break;
         }
         //At this point, we need to have the controllers to do anything.
         if (!grabControllers())
            break;

         // Can't go on if the animation is locked
         if (pUnit->isAnimationLocked())
            break;

         setState(cStateWorking);
         break;

      case cStateWorking:
      {
         //If we've lost the controllers, go back to None.
         if (!validateControllers())
         {
            setState(cStateNone);
            break;
         }
         //Check range.
         if (!validateRange())
         {
            if (mPlacedAny)
               setState(cStateDone);
            else
               setState(cStateFailed);
            break;
         }
         //If this ability has an ammo cost, we pay it or fail.
//-- FIXING PREFIX BUG ID 5027
         const BAbility* pAbility=gDatabase.getAbilityFromID(mTarget.getAbilityID());
//--
         if (pAbility->getAmmoCost() > 0.0f)
         {
            if (pUnit->getAmmunition() < pAbility->getAmmoCost())
            {
               if (mPlacedAny)
                  setState(cStateDone);
               else
                  setState(cStateFailed);
               break;
            }
            pUnit->adjustAmmunition(-pAbility->getAmmoCost());
         }
         //Place a mine.
         if (!placeMine())
         {
            //Refund.
            pUnit->adjustAmmunition(pAbility->getAmmoCost());
            if (mPlacedAny)
               setState(cStateDone);
            else
               setState(cStateFailed);
            break;
            break;
         }
         else
            mPlacedAny=true;
         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMines::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));
   mTarget=target;
}

//==============================================================================
//==============================================================================
void BUnitActionMines::setOppID(BUnitOppID oppID)
{
   if (mpOwner)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }
   mOppID=oppID;
}

//==============================================================================
//==============================================================================
uint BUnitActionMines::getPriority() const
{
//-- FIXING PREFIX BUG ID 5028
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   const BUnitOpp* pOpp=pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);
   return (pOpp->getPriority());
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 5029
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   //We need them all.
   if (pUnit->getController(BActionController::cControllerOrient)->getActionID() != mID)
      return (false);
   if (pUnit->getController(BActionController::cControllerAnimation)->getActionID() != mID)
      return (false);
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::grabControllers()
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
void BUnitActionMines::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::validateRange() const
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5032
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   //Get our range as defined by our PA.
   float range;
   if (mTarget.isRangeValid())
      range=mTarget.getRange();
   else
      range=mpProtoAction->getMaxRange(pUnit);

   //See if our squad is in range of our target.
   if (mTarget.getID().isValid())
   {
      BUnit* pTarget=gWorld->getUnit(mTarget.getID());
      
      if(!pTarget)
         return false;

//-- FIXING PREFIX BUG ID 5031
      const BSquad* pTargetSquad = pTarget->getParentSquad();
//--
      if (!pTargetSquad)
         return (false);

      return (pSquad->calculateXZDistance(pTargetSquad) <= range);
   }
   if (!mTarget.isPositionValid())
      return (false);
   return (pSquad->calculateXZDistance(mTarget.getPosition()) < range);
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::placeMine()
{
//-- FIXING PREFIX BUG ID 5034
   const BAbility* pAbility=gDatabase.getAbilityFromID(mTarget.getAbilityID());
//--
   if(!pAbility || pAbility->getNumberObjects()==0)
      return false;

   long objectID=pAbility->getObject(0);
   BVector targetPos=mpOwner->getPosition();

   // Find the closest free spot to place the mine. Look for the spot going in a spiral direction from the target position.
   BVector pos=targetPos;

   // AJL FIXME - Hard coded spacing and range should come from abilities.xml data
   float spacing=5.0f;
   float range=20.0f;
   if(spacing>0.0f && range>0.0f)
   {
      float sx=targetPos.x/spacing;
      sx=(float)((int)sx);
      sx*=spacing;

      float sz=targetPos.z/spacing;
      sz=(float)((int)sz);
      sz*=spacing;

      pos.x=sx;
      pos.z=sz;

      BEntityIDArray mines(0, 100);
      BUnitQuery query(targetPos, range, false);
      query.addObjectTypeFilter(objectID);
      query.setFlagIgnoreDead(true);
      gWorld->getUnitsInArea(&query, &mines);
      uint numMines = (uint)mines.getNumber();
      BSmallDynamicSimArray<BVector> minePositions(0, 100);
      // Add default pos if necessary
      if (numMines == 0)
      {
         minePositions.add(pos);
      }
      for (uint i = 0; i < numMines; i++)
      {
//-- FIXING PREFIX BUG ID 5033
         const BObject* pObject=gWorld->getObject(mines[i]);
//--
         if (pObject)
            minePositions.add(pObject->getPosition());
      }

      bool foundSpot = false;
      if(minePositions.getNumber()>0)
      {
         long ringCount=(long)(range/spacing)+1;
         long stepCount=1;
         for(long ring=0; ring<ringCount; ring++)
         {
            // Top row from left to right
            float x=sx;
            float z=sz;
            for(long i=0; i<stepCount; i++)
            {
               if(checkMineSpot(minePositions, x, z))
               {
                  pos.x=x;
                  pos.z=z;
                  foundSpot=true;
                  break;
               }
               x+=spacing;
            }
            if(foundSpot)
               break;

            if(stepCount>1)
            {
               // Right column from top to bottom
               x-=spacing;
               z+=spacing;
               for(long i=0; i<stepCount-1; i++)
               {
                  if(checkMineSpot(minePositions, x, z))
                  {
                     pos.x=x;
                     pos.z=z;
                     foundSpot=true;
                     break;
                  }
                  z+=spacing;
               }
               if(foundSpot)
                  break;

               // Bottom row from right to left
               x-=spacing;
               z-=spacing;
               for(long i=0; i<stepCount-1; i++)
               {
                  if(checkMineSpot(minePositions, x, z))
                  {
                     pos.x=x;
                     pos.z=z;
                     foundSpot=true;
                     break;
                  }
                  x-=spacing;
               }
               if(foundSpot)
                  break;

               // Left column from bottom to top
               x+=spacing;
               z-=spacing;
               for(long i=0; i<stepCount-2; i++)
               {
                  if(checkMineSpot(minePositions, x, z))
                  {
                     pos.x=x;
                     pos.z=z;
                     foundSpot=true;
                     break;
                  }
                  z-=spacing;
               }
               if(foundSpot)
                  break;
            }

            sx-=spacing;
            sz-=spacing;
            stepCount+=2;
         }
      }
      if (!foundSpot)
      {
         return (false);
      }
   }

   BEntityID id=gWorld->createEntity(objectID, false, mpOwner->getPlayerID(), pos, cXAxisVector, cZAxisVector, true, false);
   if(id==cInvalidObjectID)
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::checkMineSpot(BSmallDynamicSimArray<BVector>& minePositions, float x, float z)
{
//-- FIXING PREFIX BUG ID 5035
   const BAbility* pAbility = gDatabase.getAbilityFromID(mTarget.getAbilityID());
//--
   if (!pAbility || pAbility->getNumberObjects() == 0)
   {
      return (false);
   }
   
   int objectID = pAbility->getObject(0);
   BPlayerID playerID = mpOwner->getPlayerID();
   long losType = BWorld::cCPLOSFullVisible;
   BVector placementSuggestion = cInvalidVector;
   DWORD flags = 0;
   flags = BWorld::cCPCheckObstructions | BWorld::cCPExpandHull;
   int searchScale = 1;

   uint mineCount = minePositions.getNumber();
   for (uint i = 0; i < mineCount; i++)
   {
      BVector pos = minePositions[i];
      bool validPos = gWorld->checkPlacement(objectID, playerID, pos, placementSuggestion, cZAxisVector, losType, flags, searchScale);
      if (((fabs(x - pos.x) < 0.01f) && (fabs(z - pos.z) < 0.01f)) || !validPos)
      {         
         return (false);
      }
   }
   return (true);
}


//==============================================================================
//==============================================================================
bool BUnitActionMines::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEBITBOOL(pStream, mPlacedAny);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionMines::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADBITBOOL(pStream, mPlacedAny);
   return true;
}
