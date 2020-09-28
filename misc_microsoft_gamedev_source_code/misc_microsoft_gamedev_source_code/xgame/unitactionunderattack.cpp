//==============================================================================
// unitactionunderattack.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "unit.h"
#include "unitactionunderattack.h"
#include "syncmacros.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionUnderAttack, 5, &gSimHeap);


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   BASSERT(mpOwner);
   //BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   //BASSERT(pUnit);

   // We should have no dudes attacking us yet, so go to the wait state.
   BASSERT(mAttackingUnits.getSize() == 0);
   setState(cStateWait);

   return (true);
}


//==============================================================================
//==============================================================================
void BUnitActionUnderAttack::disconnect(void)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   if (pUnit)
   {
      // Unreveal attackers and attackee
      BTeamID unitTeamID = pUnit->getTeamID();
      uint attackingUnitsCount = mAttackingUnits.getSize();
      for (uint i = 0; i < attackingUnitsCount; i++)
      {
         BUnit* pAttackingUnit = gWorld->getUnit(mAttackingUnits[i].mAttackingUnitID);
         if (pAttackingUnit)
         {
            // If unit is Gaia, but has garrisoned units
            if ((pUnit->getPlayerID() == 0) && (pUnit->getFlagHasGarrisoned() || pUnit->getFlagHasAttached()))
            {
               pAttackingUnit->removeReveal(pUnit->getGarrisonedTeam());
            }
            else
            {
               pAttackingUnit->removeReveal(unitTeamID);
            }                         
            pUnit->removeReveal(pAttackingUnit->getTeamID());
         }
      }

      uint damagedCount = mUnitDamagedBy.getSize();
      for (uint i = 0; i < damagedCount; i++)
      {
         const BUnitDamagedBy& damagedBy = mUnitDamagedBy[i];

         BUnit* pAttackingUnit = gWorld->getUnit(damagedBy.mAttackerUnitID);
         if (pAttackingUnit)
         {
            // If unit is Gaia, but has garrisoned units
            if ((pUnit->getPlayerID() == 0) && (pUnit->getFlagHasGarrisoned() || pUnit->getFlagHasAttached()))
            {
               pAttackingUnit->removeReveal(pUnit->getGarrisonedTeam());
            }
            else
            {
               pAttackingUnit->removeReveal(unitTeamID);
            }                
         }
         pUnit->removeReveal(damagedBy.mAttackingTeamID);
      }
   }
   return (BAction::disconnect());
}


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::init()
{
   if (!BAction::init())
      return false;

   mAttackingUnits.resize(0);
   mUnitDamagedBy.resize(0);

   return (true);
}


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::setState(BActionState state)
{
//-- FIXING PREFIX BUG ID 2433
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   pUnit;

   syncUnitActionData("BUnitActionUnderAttack::setState owner ID", pUnit->getID());
   syncUnitActionData("BUnitActionUnderAttack::setState state", state);

   switch (state)
   {
      case cStateWorking:
      {
         // Don't know what to do here yet...
         BASSERT((mAttackingUnits.getSize() > 0) || (mUnitDamagedBy.getSize() > 0));
         break;
      }
      case cStateWait:
      {
         // Don't know what to do here yet...
         BASSERT(mAttackingUnits.getSize() == 0);
         break;
      }
   }

   return (BAction::setState(state));
}


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   
   switch (mState)
   {
      case cStateWorking:
      {
         DWORD gameTime = gWorld->getGametime();
         BTeamID unitTeamID = pUnit->getTeamID();

         // We shouldn't need this. We're already checking for null pointers below.
// #ifndef BUILD_FINAL
//          // SLB: Paranoia
//          long attackingUnitsCount = mAttackingUnits.getSize();
//          for (long i = 0; i < attackingUnitsCount; i++)
//          {
//             BUnit *pAttackingUnit = gWorld->getUnit(mAttackingUnits[i].mAttackingUnitID);
//             if (!pAttackingUnit)
//             {
//                BASSERT(0); //Why would this ever happen!?
//             }
//          }
// #endif

         // Reveal units that damage us and reveal us to the teams that damage us
         long count = mUnitDamagedBy.getSize();
         for (long i = count - 1; i >= 0; i--)
         {
            const BUnitDamagedBy &damagedBy = mUnitDamagedBy[i];

            // Remove expired entries
            if (damagedBy.mExpires <= gameTime)
            {
               BUnit* pAttackingUnit = gWorld->getUnit(damagedBy.mAttackerUnitID);
               if (pAttackingUnit)
               {
                  // If unit is Gaia, but has garrisoned units
                  if ((pUnit->getPlayerID() == 0) && (pUnit->getFlagHasGarrisoned() || pUnit->getFlagHasAttached()))
                  {
                     pAttackingUnit->removeReveal(pUnit->getGarrisonedTeam());
                  }
                  else
                  {
                     pAttackingUnit->removeReveal(unitTeamID);
                  }                
               }
               pUnit->removeReveal(damagedBy.mAttackingTeamID);

               mUnitDamagedBy.removeIndex(i, false);

               if ((mAttackingUnits.getSize() == 0) && (mUnitDamagedBy.getSize() == 0))
               {
                  setState(cStateWait);
                  break;
               }
            }
         }

         break;
      }
      case cStateWait:
      {
         // Don't know what to do here yet...
         break;
      }
   }

   return (true);
}


//==============================================================================
//==============================================================================
void BUnitActionUnderAttack::addDamage(BEntityID attackerUnitID, BTeamID attackingTeamID, DWORD expires)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Look for existing entry
   uint index;
   if (wasDamagedBy(attackerUnitID, attackingTeamID, index))
   {
      // Found one. Update the expiration time.
      mUnitDamagedBy[index].mExpires = expires;
   }
   else
   {
      // Reveal
      BUnit* pAttackingUnit = gWorld->getUnit(attackerUnitID);
      if (pAttackingUnit)
      {
         // If unit is Gaia, but has garrisoned units
         if ((pUnit->getPlayerID() == 0) && (pUnit->getFlagHasGarrisoned() || pUnit->getFlagHasAttached()))
         {
            pAttackingUnit->addReveal(pUnit->getGarrisonedTeam());
         }
         else
         {
            pAttackingUnit->addReveal(pUnit->getTeamID());
         }
      }
      pUnit->addReveal(attackingTeamID);

      // Add a new entry.
      BUnitDamagedBy damagedBy;
      damagedBy.mAttackerUnitID = attackerUnitID;
      damagedBy.mAttackingTeamID = attackingTeamID;
      damagedBy.mExpires = expires;
      mUnitDamagedBy.add(damagedBy);

      setState(cStateWorking);
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::wasDamagedBy(BEntityID attackerUnitID, BTeamID attackingTeamID, uint &index) const
{
   BASSERT(mpOwner);

   for (uint i = 0; i < mUnitDamagedBy.getSize(); i++)
   {
      const BUnitDamagedBy& damagedBy = mUnitDamagedBy[i];
      if ((damagedBy.mAttackerUnitID == attackerUnitID) && (damagedBy.mAttackingTeamID == attackingTeamID))
      {
         index = i;
         return true;
      }
   }

   return false;
}


//==============================================================================
//==============================================================================
void BUnitActionUnderAttack::addAttackingUnit(BEntityID unitID, BActionID actionID)
{
   // If not already attacking
   uint index;
   if (!isBeingAttackedByUnit(unitID, actionID, index))
   {
      // Add
      BUnitAttackedBy unitAttackedBy;
      unitAttackedBy.mAttackingUnitID = unitID;
      unitAttackedBy.mAttackingActionID = actionID;
      mAttackingUnits.add(unitAttackedBy);

      // Reveal
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      BUnit *pAttackingUnit = gWorld->getUnit(unitID);
      BASSERT(pAttackingUnit);
      // If unit is Gaia, but has garrisoned units
      if ((pUnit->getPlayerID() == 0) && (pUnit->getFlagHasGarrisoned() || pUnit->getFlagHasAttached()))
      {
         pAttackingUnit->addReveal(pUnit->getGarrisonedTeam());
      }
      else
      {
         pAttackingUnit->addReveal(pUnit->getTeamID());
      }
      pUnit->addReveal(pAttackingUnit->getTeamID());
      // We have to force the unit to update the visible list here, because it is possible (edge case) to get the removeReveal before the list is ever updated. This causes dopple issues (dopple dies and never recreates).
      if (pUnit && pUnit->getFlagVisibility())
         pUnit->updateVisibleLists();
   }

   setState(cStateWorking);
}


//==============================================================================
//==============================================================================
void BUnitActionUnderAttack::removeAttackingUnit(BEntityID unitID, BActionID actionID)
{
   // If attacking
   uint index;
   if (isBeingAttackedByUnit(unitID, actionID, index))
   {
      mAttackingUnits.removeIndex(index, false);

      // Unreveal
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BUnit *pAttackingUnit = gWorld->getUnit(unitID);
      if (pUnit && pAttackingUnit)
      {
         // If unit is Gaia, but has garrisoned units
         if ((pUnit->getPlayerID() == 0) && (pUnit->getFlagHasGarrisoned() || pUnit->getFlagHasAttached()))
         {
            pAttackingUnit->removeReveal(pUnit->getGarrisonedTeam());
         }
         else
         {
            pAttackingUnit->removeReveal(pUnit->getTeamID());
         }      
         pUnit->removeReveal(pAttackingUnit->getTeamID());
      }

      if (pUnit && pUnit->getFlagVisibility())
         pUnit->updateVisibleLists();
   }

   if ((mAttackingUnits.getSize() == 0) && (mUnitDamagedBy.getSize() == 0))
   {
      setState(cStateWait);
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::isBeingAttackedByUnit(BEntityID unitID) const
{
   BASSERT(mpOwner);

   for (uint i = 0; i < mAttackingUnits.getSize(); i++)
   {
      if (mAttackingUnits[i].mAttackingUnitID == unitID)
         return true;
   }

   return false;
}


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::isBeingAttackedByUnit(BEntityID unitID, BActionID actionID, uint &index) const
{
   BASSERT(mpOwner);

   for (uint i = 0; i < mAttackingUnits.getSize(); i++)
   {
      const BUnitAttackedBy& attackedBy = mAttackingUnits[i];
      if ((attackedBy.mAttackingUnitID == unitID) && (attackedBy.mAttackingActionID == actionID))
      {
         index = i;
         return true;
      }
   }

   return false;
}


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::isBeingAttacked() const
{
   BASSERT(mpOwner);
   return (mAttackingUnits.getSize() > 0);
}


//==============================================================================
//==============================================================================
uint BUnitActionUnderAttack::getNumberAttackingUnits() const
{
   BASSERT(mpOwner);
   return (mAttackingUnits.getSize());
}


//==============================================================================
//==============================================================================
BEntityID BUnitActionUnderAttack::getAttackingUnitByIndex(uint index) const
{
   if (index < mAttackingUnits.getSize())
      return (mAttackingUnits[index].mAttackingUnitID);
   else
      return (cInvalidObjectID);
}


//==============================================================================
//==============================================================================
uint BUnitActionUnderAttack::getAttackingUnits(BEntityIDArray& attackingUnits) const
{
   attackingUnits.resize(0);

   for (uint i = 0; i < mAttackingUnits.getSize(); i++)
   {
      attackingUnits.add(mAttackingUnits[i].mAttackingUnitID);
   }

   return (attackingUnits.getSize());
}


//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEARRAY(pStream, BUnitAttackedBy, mAttackingUnits, uint8, 200);
   GFWRITEARRAY(pStream, BUnitAttackedBy, mUnitDamagedBy, uint8, 200);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionUnderAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADARRAY(pStream, BUnitAttackedBy, mAttackingUnits, uint8, 200);
   GFREADARRAY(pStream, BUnitAttackedBy, mUnitDamagedBy, uint8, 200);
   return true;
}
