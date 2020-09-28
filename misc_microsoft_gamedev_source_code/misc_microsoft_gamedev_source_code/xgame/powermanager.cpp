//==============================================================================
// powermanager.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "powercleansing.h"
#include "powerorbital.h"
#include "powerrepair.h"
#include "powercarpetbombing.h"
#include "powercryo.h"
#include "powerwave.h"
#include "powerrage.h"
#include "powerdisruption.h"
#include "powerodst.h"
#include "powertransport.h"
#include "powermanager.h"
#include "database.h"
#include "protoobject.h"
#include "protopower.h"
#include "squad.h"
#include "world.h"
#include "unitactionavoidcollisionair.h"
#include "user.h"
#include "usermanager.h"

GFIMPLEMENTVERSION(BPowerManager, 1);

//==============================================================================
//==============================================================================
BPowerManager::BPowerManager()
{
   mNextPowerID = 0;
}


//==============================================================================
//==============================================================================
BPowerManager::~BPowerManager()
{
   // Reset this.
   mNextPowerID = 0;

   // Free all the powers.
   uint numPowers = mPowers.getSize();
   for (uint i=0; i<numPowers; i++)
   {
      BASSERTM(mPowers[i], "BPowerManager has a crap BPower* in the list.");
      if (mPowers[i])
      {
         HEAP_DELETE(mPowers[i], gSimHeap);
      }
   }
}

//==============================================================================
// Allocate the power of appropriate type
//==============================================================================
BPower* BPowerManager::allocatePower(BPowerType powerType)
{
   BPower* pNewPower = NULL;
   switch(powerType)
   {
      case PowerType::cCleansing:
      {
         pNewPower = HEAP_NEW(BPowerCleansing, gSimHeap);
         break;
      }
      case PowerType::cOrbital:
      {
         pNewPower = HEAP_NEW(BPowerOrbital, gSimHeap);
         break;
      }
      case PowerType::cCarpetBombing:
      {
         pNewPower = HEAP_NEW(BPowerCarpetBombing, gSimHeap);
         break;
      }
      case PowerType::cCryo:
      {
         pNewPower = HEAP_NEW(BPowerCryo, gSimHeap);
         break;
      }
      case PowerType::cRage:
      {
         pNewPower = HEAP_NEW(BPowerRage, gSimHeap);
         break;
      }
      case PowerType::cWave:
      {
         pNewPower = HEAP_NEW(BPowerWave, gSimHeap);
         break;
      }
      case PowerType::cDisruption:
      {
         pNewPower = HEAP_NEW(BPowerDisruption, gSimHeap);
         break;
      }
      case PowerType::cRepair:
      {
         pNewPower = HEAP_NEW(BPowerRepair, gSimHeap);
         break;
      }
      case PowerType::cTransport:
      {
         pNewPower = HEAP_NEW(BPowerTransport, gSimHeap);
         break;
      }
      case PowerType::cODST:
      {
         pNewPower = HEAP_NEW(BPowerODST, gSimHeap);
         break;
      }
      default:
      {
         BFAIL("BPowerManager::createNewPower() - Called with unsupported BPowerType.");
      }
   }
   return (pNewPower);
}

//==============================================================================
// Create the power of appropriate type and assign the ID
//==============================================================================
BPower* BPowerManager::createNewPower(long protoPowerID)
{
//-- FIXING PREFIX BUG ID 2658
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
   if (!pProtoPower)
      return NULL;

   BPower* pNewPower = allocatePower(pProtoPower->getPowerType());

   // We got a valid power, so set up the ID.
   if (pNewPower)
   {
      pNewPower->setProtoPowerID(protoPowerID);
      pNewPower->setID(mNextPowerID);
      pNewPower->setType(pProtoPower->getPowerType());
      mNextPowerID++;
      BASSERT(mNextPowerID != cInvalidPowerID);
      mPowers.add(pNewPower);
   }

   return (pNewPower);
}


//==============================================================================
//==============================================================================
BPower* BPowerManager::getPowerByID(BPowerID powerID)
{
   uint numPowers = mPowers.getSize();
   for (uint i=0; i<numPowers; i++)
   {
      if (mPowers[i] && mPowers[i]->getID() == powerID)
         return (mPowers[i]);
   }
   return (NULL);
}


//==============================================================================
//==============================================================================
const BPower* BPowerManager::getPowerByIndex(uint powerIndex) const
{
   if (powerIndex < mPowers.getSize())
      return (mPowers[powerIndex]);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
BPower* BPowerManager::getPowerByUserID(BPowerUserID powerUserID)
{
   uint numPowers = mPowers.getSize();
   for (uint i=0; i<numPowers; i++)
   {
      if (mPowers[i] && mPowers[i]->getPowerUserID() == powerUserID)
         return (mPowers[i]);
   }
   return (NULL);
}

//==============================================================================
//==============================================================================
void BPowerManager::cancelPowerUser(BPowerUserID powerUserID)
{
   // look up our user - if we have this power user, then cancel it
   BUser* pUser = gUserManager.getUserByPlayerID(powerUserID.getPlayerID());
   if (pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == powerUserID)
      pUser->getPowerUser()->cancelPower();
}

//==============================================================================
//==============================================================================
const BPower* BPowerManager::getPowerByID(BPowerID powerID) const
{
   uint numPowers = mPowers.getSize();
   for (uint i=0; i<numPowers; i++)
   {
      if (mPowers[i] && mPowers[i]->getID() == powerID)
         return (mPowers[i]);
   }
   return (NULL);
}


//==============================================================================
//==============================================================================
void BPowerManager::update(DWORD currentGameTime, float lastUpdateLength)
{
   uint numPowers = mPowers.getSize();
   uint numDeleted = 0;

   // Iterate through our live powers and update.
   // If they need to be deleted, do that too, but clean up the NULL pointers outside the loop
   for (uint i=0; i<numPowers; i++)
   {
      mPowers[i]->update(currentGameTime, lastUpdateLength);

      if (mPowers[i]->getFlagDestroy())
      {
         // Manually delete our power user if it's on this machine.
         mPowers[i]->deletePowerUser();

         numDeleted++;
         HEAP_DELETE(mPowers[i], gSimHeap);
         mPowers[i] = NULL;
      }
   }

   // If we deleted stuff, clear out the NULL pointers here.
   if (numDeleted > 0)
      mPowers.removeValueAllInstances(NULL);
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerLocation(const BVector& location, BPowerDisruption** pOutDisruptPower)
{
   // quick and dirty right now, search through the powers for a disruption power
   BPower* pPower = NULL;
   for (long i = 0; i < mPowers.getNumber(); ++i)
   {
      pPower = mPowers[i];
      if (pPower && pPower->getType() == PowerType::cDisruption)
      {
         BPowerDisruption* pDisruptionPower = reinterpret_cast<BPowerDisruption*>(pPower);
         if (!pDisruptionPower->isActive())
            continue;

         if (location.xzDistanceSqr(pDisruptionPower->getTargetLocation()) < pDisruptionPower->getRadiusSqr())
         {
            if (pOutDisruptPower)
               *pOutDisruptPower = pDisruptionPower;
            return false;
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerLocation(const BVector& location, const BPower* pSourcePower, BPowerDisruption** pOutDisruptPower)
{
   // bail if this power doesn't need to validate power locations
   if (pSourcePower && !pSourcePower->getFlagCheckPowerLocation())
      return true;
   return isValidPowerLocation(location, pOutDisruptPower);
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerLocation(const BVector& location, const BPowerUser* pSourcePowerUser, BPowerDisruption** pOutDisruptPower)
{
   // bail if this power doesn't need to validate power locations
   if (pSourcePowerUser && !pSourcePowerUser->getFlagCheckPowerLocation())
      return true;
   return isValidPowerLocation(location, pOutDisruptPower);
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerLineSegment(const BVector& start, const BVector& end, BPowerDisruption** pOutDisruptPower)
{
   // quick and dirty right now, search through the powers for a disruption power
   BPower* pPower = NULL;
   for (long i = 0; i < mPowers.getNumber(); ++i)
   {
      pPower = mPowers[i];
      if (pPower && pPower->getType() == PowerType::cDisruption)
      {
         BPowerDisruption* pDisruptionPower = reinterpret_cast<BPowerDisruption*>(pPower);
         if (!pDisruptionPower->isActive())
            continue;

         // now we need to check if this line segment crosses this circle
         BVector disruptCenter = pDisruptionPower->getTargetLocation();

         // use parametrics to find the closest point on the line to the circle center
         BVector dir = end - start;
         BVector toCenter = disruptCenter - start;
         float t = toCenter.dot(dir) / dir.dot(dir);
         t = Math::Clamp(t, 0.0f, 1.0f);

         BVector closestPoint = start + t * dir;
         BVector distance = disruptCenter - closestPoint;
         float distanceSqr = distance.dot(distance);
         if (distanceSqr < pDisruptionPower->getRadiusSqr())
         {
            if (pOutDisruptPower)
               *pOutDisruptPower = pDisruptionPower;
            return false;
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerLineSegment(const BVector& start, const BVector& end, const BPower* pSourcePower, BPowerDisruption** pOutDisruptPower)
{
   // bail if this power doesn't need to validate power locations
   if (pSourcePower && !pSourcePower->getFlagCheckPowerLocation())
      return true;
   return isValidPowerLineSegment(start, end, pOutDisruptPower);
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerLineSegment(const BVector& start, const BVector& end, const BPowerUser* pSourcePowerUser, BPowerDisruption** pOutDisruptPower)
{
   // bail if this power doesn't need to validate power locations
   if (pSourcePowerUser && !pSourcePowerUser->getFlagCheckPowerLocation())
      return true;
   return isValidPowerLineSegment(start, end, pOutDisruptPower);
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerCircle(const BVector& location, float radius, BPowerDisruption** pOutDisruptPower)
{
   // quick and dirty right now, search through the powers for a disruption power
   BPower* pPower = NULL;
   for (long i = 0; i < mPowers.getNumber(); ++i)
   {
      pPower = mPowers[i];
      if (pPower && pPower->getType() == PowerType::cDisruption)
      {
         BPowerDisruption* pDisruptionPower = reinterpret_cast<BPowerDisruption*>(pPower);
         if (!pDisruptionPower->isActive())
            continue;

         float radiusSqr = (pDisruptionPower->getRadius() + radius) * (pDisruptionPower->getRadius() + radius);
         if (location.xzDistanceSqr(pDisruptionPower->getTargetLocation()) < radiusSqr)
         {
            if (pOutDisruptPower)
               *pOutDisruptPower = pDisruptionPower;
            return false;
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerCircle(const BVector& location, float radius, const BPower* pSourcePower, BPowerDisruption** pOutDisruptPower)
{
   // bail if this power doesn't need to validate power locations
   if (pSourcePower && !pSourcePower->getFlagCheckPowerLocation())
      return true;
   return isValidPowerCircle(location, radius, pOutDisruptPower);
}

//==============================================================================
//==============================================================================
bool BPowerManager::isValidPowerCircle(const BVector& location, float radius, const BPowerUser* pSourcePowerUser, BPowerDisruption** pOutDisruptPower)
{
   // bail if this power doesn't need to validate power locations
   if (pSourcePowerUser && !pSourcePowerUser->getFlagCheckPowerLocation())
      return true;
   return isValidPowerCircle(location, radius, pOutDisruptPower);
}

//==============================================================================
//==============================================================================
bool BPowerManager::isSquadPowerValid(BPlayerID playerId, const BSquad& squad, const BProtoPower& protoPower, const BVector& targetLocation)
{
   // only unit powers with valid squads will get in here
   // check the squad's position and the target location's position
   BUnit* pLeaderUnit = squad.getLeaderUnit();
   if (!pLeaderUnit)
      return false;

   if (squad.getFlagAttackBlocked())
      return false;

   if (!squad.isAlive() || squad.isGarrisoned())
      return false;

   BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pLeaderUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
   if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
      return false;

   const BVector& ownerPos = squad.getPosition();
   BVector tempVec;
   if(!gWorld->checkPlacement(-1,  playerId, ownerPos, tempVec, cZAxisVector, BWorld::cCPLOSFullVisible, BWorld::cCPLOSCenterOnly) ||
      !isValidPowerLocation(ownerPos))
      return false;

   if(!gWorld->checkPlacement(-1,  playerId, targetLocation, tempVec, cZAxisVector, BWorld::cCPLOSFullVisible, BWorld::cCPLOSCenterOnly) ||
      !isValidPowerLocation(targetLocation))
      return false;

   // check range from squad
   float xzDistance = ownerPos.xzDistance(targetLocation);
   if (xzDistance > protoPower.getMaxDistanceToSquad() || xzDistance < protoPower.getMinDistanceToSquad())
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerManager::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BPowerID, mNextPowerID);

   // mPowers
   uint count = mPowers.getNumber();
   GFWRITEVAL(pStream, uint8, count);
   GFVERIFYCOUNT(count, 60);
   for (uint i=0; i<count; i++)
   {
      BPower* pPower = mPowers[i];
      if (pPower)
      {
         GFWRITEVAL(pStream, uint8, i);
         BPowerType powerType = pPower->getType();
         BPowerID powerID = pPower->getID();
         GFWRITEVAL(pStream, uint8, powerType);
         GFWRITEVAL(pStream, int16, powerID);
         GFVERIFYCOUNT(powerID, mNextPowerID);
         GFWRITECLASSPTR(pStream, saveType, pPower);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerManager::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BPowerID, mNextPowerID);

   // mPowers
   uint count;
   GFREADVAL(pStream, uint8, uint, count);
   GFVERIFYCOUNT(count, 60);
   if (!mPowers.setNumber(count))
      return false;
   for (uint i=0; i<count; i++)
      mPowers[i] = NULL;
   for (uint i=0; i<count; i++)
   {
      uint index;
      GFREADVAL(pStream, uint8, uint, index);
      GFVERIFYCOUNT(index, count-1);
      BPowerType powerType;
      BPowerID powerID;
      GFREADVAL(pStream, uint8, BPowerType, powerType);
      GFREADVAL(pStream, int16, BPowerID, powerID);
      GFVERIFYCOUNT(powerID, mNextPowerID);
      BPower* pPower = allocatePower(powerType);
      if (!pPower)
         return false;
      pPower->setID(powerID);
      pPower->setType(powerType);
      mPowers[index] = pPower;
      GFREADCLASSPTR(pStream, saveType, pPower);
   }

   return true;
}
