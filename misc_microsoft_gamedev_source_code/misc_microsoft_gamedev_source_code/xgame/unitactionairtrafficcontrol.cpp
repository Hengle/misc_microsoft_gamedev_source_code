//==============================================================================
// unitactionairtrafficcontrol.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unit.h"
#include "unitactionairtrafficcontrol.h"
#include "unitactionmoveair.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionAirTrafficControl, 5, &gSimHeap);


//==============================================================================
//==============================================================================
bool BUnitActionAirTrafficControl::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionAirTrafficControl::disconnect()
{
   releaseAllAircraft();
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionAirTrafficControl::init()
{
   if (!BAction::init())
      return(false);

   for (int i=0; i<MAX_LANDING_SPOTS; i++)
   {
      mSpot[i].init();
   }
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionAirTrafficControl::update(float elapsed)
{
   switch (mState)
   {
      case cStateNone:
         initSpotCoords();
         setState(cStateWorking);
         break;
         
      case cStateWorking:
      {
         break;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionAirTrafficControl::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
}

//==============================================================================
//==============================================================================
void BUnitActionAirTrafficControl::initSpotCoords()
{
   BMatrix matrix;
   mpOwner->getWorldMatrix(matrix);

   // FIXME BSR 9/20/07 temporary hack until bones are placed to designate spots
//-- FIXING PREFIX BUG ID 1819
   const BPlayer* pPlayer = mpOwner->getPlayer();
//--
   if (pPlayer && (pPlayer->getCivID() == gDatabase.getCivID("UNSC")))
   {
      for (int i=0; i<4; i++)
      {
         matrix.transformVectorAsPoint(BVector(3.0f, 3.0f, -7.0f*i), mSpot[i].worldPos);
         mSpot[i].forward.x = 1.0f;
         mSpot[i].forward.z = -1.0f;
         mSpot[i].forward.normalize();
      }
      for (int i=4; i<MAX_LANDING_SPOTS; i++)
      {
         matrix.transformVectorAsPoint(BVector(-6.0f, 3.0f, -7.0f*i + 18.0f), mSpot[i].worldPos);
         mSpot[i].forward.x = -1.0f;
         mSpot[i].forward.z = -1.0f;
         mSpot[i].forward.normalize();
      }
   }
   else
   {
      BVector offset[MAX_LANDING_SPOTS];
      offset[0].x = 0.0f; offset[0].z = 20.0f;
      offset[1].x = 14.14f; offset[1].z = 14.14f;
      offset[2].x = 20.0f; offset[2].z = 0.0f;
      offset[3].x = 14.14f; offset[3].z = -14.14f;
      offset[4].x = 0.0f; offset[4].z = -24.0f;
      offset[5].x = -14.14f; offset[5].z = -14.14f;
      offset[6].x = -20.0f; offset[6].z = 0.0f;
      offset[7].x = -14.14f; offset[7].z = 14.14f;
      for (int i=0; i<MAX_LANDING_SPOTS; i++)
      {
         offset[i].y = 3.0f;
         matrix.transformVectorAsPoint(offset[i], mSpot[i].worldPos);
         mSpot[i].forward.x = mSpot[i].worldPos.x - mpOwner->getPosition().x;
         mSpot[i].forward.z = mSpot[i].worldPos.z - mpOwner->getPosition().z;
         mSpot[i].forward.normalize();
      }
   }
}

//==============================================================================
//==============================================================================
BVector BUnitActionAirTrafficControl::requestLandingSpot(BUnit* aircraft, BVector& forward)
{
   BVector spotPos = cInvalidVector;

   if (!aircraft)
      return (spotPos);

   for (int i=0; i<MAX_LANDING_SPOTS; i++)
   {
      if (mSpot[i].aircraftID == cInvalidObjectID) // spot is available
      {
         spotPos = mSpot[i].worldPos;
         forward = mSpot[i].forward;
         mSpot[i].aircraftID = aircraft->getID();
         break;
      }
   }

   return (spotPos);
}

//==============================================================================
//==============================================================================
void BUnitActionAirTrafficControl::releaseSpot(BUnit* aircraft)
{
   if (!aircraft)
      return;

   BEntityID aircraftID = aircraft->getID();
   for (int i=0; i<MAX_LANDING_SPOTS; i++)
   {
      if (mSpot[i].aircraftID == aircraftID) // that's the one
      {
         mSpot[i].aircraftID = cInvalidObjectID; // free it up
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionAirTrafficControl::releaseAllAircraft()
{
   for (int i=0; i<MAX_LANDING_SPOTS; i++)
   {
      if (mSpot[i].aircraftID != cInvalidObjectID) // spot is taken
      {
         // Tell assigned aircraft they're on their own (through their move actions
         BUnit* aircraft = gWorld->getUnit(mSpot[i].aircraftID);
         if (aircraft)
         {
            BUnitActionMoveAir* pAirAction=(BUnitActionMoveAir*)aircraft->getActionByType(BAction::cActionTypeUnitMoveAir);
            if (pAirAction)
               pAirAction->landingSiteDestroyed();
         }
         mSpot[i].aircraftID = cInvalidObjectID;
      }
   }

}


//==============================================================================
//==============================================================================
bool BUnitActionAirTrafficControl::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAL(pStream, int, MAX_LANDING_SPOTS);
   for (int i=0; i<MAX_LANDING_SPOTS; i++)
      GFWRITECLASS(pStream, saveType, mSpot[i]);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionAirTrafficControl::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   int count;
   GFREADVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 20);
   for (int i=0; i<count; i++)
   {
      if (i < MAX_LANDING_SPOTS)
         GFREADCLASS(pStream, saveType, mSpot[i])
      else
         GFREADTEMPCLASS(pStream, saveType, BLandingSpot)
   }
   return true;
}


//==============================================================================
//==============================================================================
bool BLandingSpot::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, worldPos);
   GFWRITEVECTOR(pStream, forward);
   GFWRITEVAL(pStream, BEntityID, aircraftID);
   return true;
}

//==============================================================================
//==============================================================================
bool BLandingSpot::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, worldPos);
   GFREADVECTOR(pStream, forward);
   GFREADVAR(pStream, BEntityID, aircraftID);
   return true;
}
