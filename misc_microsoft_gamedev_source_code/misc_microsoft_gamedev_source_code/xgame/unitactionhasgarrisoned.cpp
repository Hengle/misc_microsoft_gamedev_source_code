//==============================================================================
// unitactionhasgarrisoned.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "protoobject.h"
#include "unit.h"
#include "unitactionhasgarrisoned.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionHasGarrisoned, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionHasGarrisoned::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   setState(cStateWorking);
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionHasGarrisoned::init()
{
   if (!BAction::init())
      return(false);

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionHasGarrisoned::update(float elapsed)
{
   BUnit* pThisUnit = mpOwner->getUnit();

   bool moving = pThisUnit->getFlagMoving() || pThisUnit->getFlagTeleported();

   long count = pThisUnit->getNumberEntityRefs();

   if(!pThisUnit->getFlagHasGarrisoned())
      setState(cStateDone);

   if(!pThisUnit->getFlagHasGarrisoned() || !moving || count == 0)
      return (true);

   for(long i=0; i<count; i++)
   {
      BEntityRef ref=(*pThisUnit->getEntityRefByIndex(i));
      if(ref.mType!=BEntityRef::cTypeContainUnit)
         continue;
      BUnit* pContainedUnit=gWorld->getUnit(ref.mID);
      if(pContainedUnit)
      {
         BVector position = pThisUnit->getPosition();
         pContainedUnit->setPosition(position);
         pContainedUnit->updateObstruction();
         BSquad* pSquad=pContainedUnit->getParentSquad();
         if(pSquad)
         {
            pSquad->setPosition(position);
            pSquad->setLeashPosition(position);
            pSquad->updateObstruction();
         }
      }
   }

   return (true);
}
