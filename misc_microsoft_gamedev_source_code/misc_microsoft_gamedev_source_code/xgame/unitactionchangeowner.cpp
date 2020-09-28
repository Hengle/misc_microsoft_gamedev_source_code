//==============================================================================
// unitactionchangeowner.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "protoobject.h"
#include "unit.h"
#include "unitactionchangeowner.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionChangeOwner, 5, &gSimHeap);


//==============================================================================
//==============================================================================
bool BUnitActionChangeOwner::init()
{
   if (!BAction::init())
      return(false);

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionChangeOwner::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   switch (mState)
   {
      case cStateNone:
         setState(cStateWorking);
         break;
         
      case cStateWorking:
      {
         //If we're fully owned by a non-GAIA player, bail (and make sure the capture
         //points are set back to 0).
         if (pUnit->getPlayerID() > 0)
         {
            pUnit->setCapturePoints(0.0f, 0.0f);
            break;
         }
         //If the capture player isn't set, bail (and make sure the capture
         //points are set back to 0).
         if (pUnit->getCapturePlayerID() == cInvalidPlayerID)
         {
            pUnit->setCapturePoints(0.0f, 0.0f);
            break;
         }
         //If we're actively being captured, bail.
         if (pUnit->getFlagBeingCaptured())
            break;
         /*long count=pUnit->getNumberEntityRefs();
         for(long i=0; i<count; i++)
         {
            BEntityRef *pEntityRef = pUnit->getEntityRefByIndex(i);
            if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeCapturingUnit)
            {
               if(pEntityRef->mData1 != (short)pUnit->getCapturePlayerID())
                  decay=true;
               break;
            }
         }*/

         //Else, we have a partially captured node that's not currently being
         //captured.  Thus, we decay.
         float capturePoints=pUnit->getCapturePoints();
         float points=gDatabase.getCaptureDecayRate()*elapsed;
         float newCapturePoints=capturePoints-points;
         //Unit handles the edge causes on the value here.
         pUnit->setCapturePoints(newCapturePoints, elapsed);
         gWorld->notify(BEntity::cEventCapturePercent, pUnit->getID(), 0, 0);
      }
   }

   return (true);
}
