//==============================================================================
// unitactionhonk.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionhonk.h"
#include "ability.h"
#include "protoobject.h"
#include "selectionmanager.h"
#include "syncmacros.h"
#include "tactic.h"
#include "unit.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionHonk, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionHonk::init()
{
   if (!BAction::init())
      return (false);

   mTarget.reset();
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionHonk::update(float elapsed)
{
   switch (mState)
   {
      case cStateNone:
      {
         setState(cStateWorking);
         break;
      }

      case cStateWorking:
      {
         honk();
         setState(cStateDone);
         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionHonk::honk()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   //XXXHalwes - 7/20/2007 - Is this still used?
   //if (pUnit->doUnattach())
   //   return;

//-- FIXING PREFIX BUG ID 1246
   const BAbility* pAbility=gDatabase.getAbilityFromID(mTarget.getAbilityID());
//--
   if (!pAbility || pAbility->getNumberObjects()==0)
      return;

   // AJL FIXME - Hard coded range should come from abilities.xml data
   float range=15.0f;
   if (range>0.0f)
   {
      BVector pos=mpOwner->getPosition();

      BEntityIDArray units(0, 50);
      BUnitQuery query(pos, range, false);
      for (long i=0; i<pAbility->getNumberObjects(); i++)
         query.addObjectTypeFilter(pAbility->getObject(i));
      query.setFlagIgnoreDead(true);
      query.addPlayerFilter(mpOwner->getPlayerID());
      gWorld->getUnitsInArea(&query, &units);

      if (units.getNumber()>0)
      {
         BUnit* pClosestUnit=NULL;
         float closestDist=cMaximumFloat;
         for (long i=0; i<units.getNumber(); i++)
         {
            BUnit* pTempUnit=gWorld->getUnit(units[i]);
            if (pTempUnit)
            {
               if (pTempUnit->isAttached())
                  continue;
               float dist=pTempUnit->getPosition().distanceSqr(pos);
               if (dist<closestDist)
               {
                  closestDist=dist;
                  pClosestUnit=pTempUnit;
               }
            }
         }
         if (pClosestUnit)
         {
//-- FIXING PREFIX BUG ID 1245
            const BSquad* pSquad=pClosestUnit->getParentSquad();
//--
            if (pSquad)
            {
               for (uint i=0; i<pSquad->getNumberChildren(); i++)
               {
                  BUnit* pTempUnit=gWorld->getUnit(pSquad->getChild(i));
                  if (pTempUnit)
                     pUnit->attachObject(pTempUnit->getID());
               }
            }
         }
      }
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionHonk::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionHonk::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   return true;
}
