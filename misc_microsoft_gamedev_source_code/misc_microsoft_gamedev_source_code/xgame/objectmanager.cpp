//==============================================================================
// objectmanager.cpp
//
// objectmanager
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================


#include "common.h"
#include "protoobject.h"
#include "object.h"
#include "objectmanager.h"
#include "player.h"
#include "triggermanager.h"
#include "world.h"
#include "team.h"

//#define DEBUG_VERIFY_OBJECT_NO_UPDATE

//const unsigned long cNumObjects = 1000;
//const unsigned long cNumSquads = 2000;
//const unsigned long cNumUnits = 6000;
//const unsigned long cNumDopples = 6000;
//const unsigned long cNumProjectiles = 2000;

static DWORD cNextArray = 0xFEFEFEFE;

//==============================================================================
// BObjectManager::BObjectManager
//==============================================================================
BObjectManager::BObjectManager( void ) :
   mObjects(&gSimHeap),
   mUnits(&gSimHeap),
   mSquads(&gSimHeap),
   mPlatoons(&gSimHeap),
   mArmies(&gSimHeap),
   mDopples(&gSimHeap),
   mProjectiles(&gSimHeap)
   //,mpNoUpdateObjects(NULL)
{
}

//==============================================================================
// BObjectManager::~BObjectManager
//==============================================================================
BObjectManager::~BObjectManager( void )
{
   verifyObjectState();
}

//==============================================================================
// BObjectManager::init
//==============================================================================
bool BObjectManager::init( void )
{
   //-- create space for each of our object types
   //-- WMJ - data drive this
   bool success = mObjects.init();
   BASSERT(success);
   
   success = mUnits.init();
   BASSERT(success);
   
   success = mSquads.init();
   BASSERT(success);

   success = mPlatoons.init();
   BASSERT(success);

   success = mArmies.init();
   BASSERT(success);
   
   success = mDopples.init();
   BASSERT(success);
   
   success = mProjectiles.init();
   BASSERT(success);
   
   //AJL FIXME - Just like in team.cpp, the max # objects is hard-coded.
   //mpNoUpdateObjects = HEAP_NEW_ARRAY(bool, 16000, gSimHeap);
   mNoUpdateObjects.getAllocator().setHeap(&gSimHeap);
   uint newBits = 16000;
   mNoUpdateObjects.resize((newBits + 63U) >> 6U);

   return success;

}

//==============================================================================
// BObjectManager::reset
//==============================================================================
void BObjectManager::reset( void )
{
   BEntityHandle handle = cInvalidObjectID;
   BEntity* pEntity = getNextArmy(handle);
   while (pEntity)
   {
      releaseObject(pEntity);
      pEntity = getNextArmy(handle);
   }
   mArmies.clear();

   handle = cInvalidObjectID;
   pEntity = getNextPlatoon(handle);
   while (pEntity)
   {
      releaseObject(pEntity);
      pEntity = getNextPlatoon(handle);
   }
   mPlatoons.clear();

   handle = cInvalidObjectID;
   pEntity = getNextSquad(handle);
   while (pEntity)
   {
      releaseObject(pEntity);
      pEntity = getNextSquad(handle);
   }
   mSquads.clear();

   handle = cInvalidObjectID;
   pEntity = getNextUnit(handle);
   while (pEntity)
   {
      releaseObject(pEntity);
      pEntity = getNextUnit(handle);
   }
   mUnits.clear();

   handle = cInvalidObjectID;
   pEntity = getNextProjectile(handle);
   while (pEntity)
   {
      releaseObject(pEntity);
      pEntity = getNextProjectile(handle);
   }
   mProjectiles.clear();

   handle = cInvalidObjectID;
   pEntity = getNextDopple(handle);
   while (pEntity)
   {
      releaseObject(pEntity);
      pEntity = getNextDopple(handle);
   }
   mDopples.clear();

   handle = cInvalidObjectID;
   pEntity = getNextObject(handle);
   while (pEntity)
   {
      releaseObject(pEntity);
      pEntity = getNextObject(handle);
   }
   mObjects.clear();

   /*
   if (mpNoUpdateObjects)
   {
      HEAP_DELETE_ARRAY(mpNoUpdateObjects, gSimHeap);
      mpNoUpdateObjects = NULL;
   }
   */
   mNoUpdateObjects.setAll(0);

   uint airSpotIndex=0;
   BAirSpot* pAirSpot = getNextClaimedAirSpot(airSpotIndex);
   while (pAirSpot)
   {
      airSpotIndex++;
      releaseClaimedAirSpot(airSpotIndex);
      pAirSpot = getNextClaimedAirSpot(airSpotIndex);
   }
   mAirSpots.clear();

   verifyObjectState();
}


//==============================================================================
// BObjectManager::createObject
//==============================================================================
BEntity* BObjectManager::createObject(const BObjectCreateParms &parms)
{
   BEntity* pEnt = NULL;
   if (parms.mFromSave)
      pEnt = allocateObjectWithID(parms.mType, parms.mSaveEntityID);
   else
      pEnt = allocateObject(parms.mType);
   if (!pEnt)
      return pEnt;

   pEnt->setPlayerID(parms.mPlayerID);  
   #ifdef SYNC_Unit
      if (pEnt->isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BObjectManager::createObject", parms.mPosition);
   #endif
   pEnt->setPosition(parms.mPosition);
   pEnt->setForward(parms.mForward);
   pEnt->setRight(parms.mRight);
   pEnt->calcUp();

   return pEnt;
}




//==============================================================================
// BObjectManager::allocateObject
//==============================================================================
BEntity* BObjectManager::allocateObject(BEntity::BClassType type)
{
   BEntity *pEnt = NULL;   // will point to the new entry after acquire
   uint index = 0;         // will contain the new index after acquire
   switch (type)
   {
   case BEntity::cClassTypeObject: 
         pEnt = mObjects.acquire(index);
         break;
         
   case BEntity::cClassTypeUnit:
         pEnt = mUnits.acquire(index);
         break;

   case BEntity::cClassTypeSquad:
         pEnt = mSquads.acquire(index);
         break;

   case BEntity::cClassTypePlatoon:
         pEnt = mPlatoons.acquire(index);
         break;

   case BEntity::cClassTypeArmy:
         pEnt = mArmies.acquire(index);
         break;

   case BEntity::cClassTypeDopple:
      pEnt = mDopples.acquire(index);
      break;

   case BEntity::cClassTypeProjectile:
      pEnt = mProjectiles.acquire(index);
      break;
   }

   if (!pEnt)
      return NULL;

   // Do our onAcquire initialization.
   pEnt->onAcquire();

   // Set up our ID here.
   uint count = pEnt->getID().getUseCount();
   pEnt->setID(BEntityID(type, count, index));

   // Return our thing.
   return pEnt;
}

//==============================================================================
// BObjectManager::allocateObjectWithID
//==============================================================================
BEntity* BObjectManager::allocateObjectWithID(BEntity::BClassType type, BEntityID id)
{
   BEntity *pEnt = NULL;   // will point to the new entry after acquire
   uint index = id.getIndex();
   switch (type)
   {
      case BEntity::cClassTypeObject: 
         pEnt = mObjects.acquireAtIndex(index);
         break;
         
      case BEntity::cClassTypeUnit:
         pEnt = mUnits.acquireAtIndex(index);
         break;

      case BEntity::cClassTypeSquad:
         pEnt = mSquads.acquireAtIndex(index);
         break;

      case BEntity::cClassTypePlatoon:
         pEnt = mPlatoons.acquireAtIndex(index);
         break;

      case BEntity::cClassTypeArmy:
         pEnt = mArmies.acquireAtIndex(index);
         break;

      case BEntity::cClassTypeDopple:
         pEnt = mDopples.acquireAtIndex(index);
         break;

      case BEntity::cClassTypeProjectile:
         pEnt = mProjectiles.acquireAtIndex(index);
         break;
   }

   if (!pEnt)
      return NULL;

   // Do our onAcquire initialization.
   pEnt->onAcquire();

   // Set up our ID here.
   pEnt->setID(id);

   // Return our thing.
   return pEnt;
}

//==============================================================================
// BObjectManager::releaseObject
//==============================================================================
void BObjectManager::releaseObject(BEntityID id)
{
   BObject* pObject = getObject(id);
   if (!pObject)
      return;

   releaseObject(pObject);
}

//==============================================================================
// BObjectManager::removeFromVisibleLists
//==============================================================================
void BObjectManager::removeFromVisibleLists(BEntityID id)
{
   // Remove object from team visibility lists
   int32 numTeams = gWorld->getNumberTeams();
   for (int32 i = 1; i < numTeams; i++)
   {
      BTeam* pTeam = gWorld->getTeam(i);
      BASSERT(pTeam);
      pTeam->onRelease(id);
   }
}

//==============================================================================
// BObjectManager::releaseObject
//==============================================================================
void BObjectManager::releaseObject(BEntity* pObject)
{
   if (!pObject)   // Don't do this.
      return;

   // Release our object and bump the refcount
   BEntityID id = pObject->getID();
   pObject->onRelease();
   pObject->setID(getBumpUseCount(id));
   uint index = id.getIndex();

   // Actually release it back to the freelist.
   uint type = id.getType();
   switch (type)
   {
   case BEntity::cClassTypeObject:
      {
         removeFromVisibleLists(id);
         mObjects.release(index);
         gTriggerManager.invalidateEntityID(id);
         break;
      }
   case BEntity::cClassTypeUnit:
      {
         removeFromVisibleLists(id);
         mUnits.release(index);
         gTriggerManager.invalidateEntityID(id);
         break;
      }
   case BEntity::cClassTypeSquad:
      {
         mSquads.release(index);
         gTriggerManager.invalidateEntityID(id);
         break;
      }
   case BEntity::cClassTypePlatoon:
      {
         mPlatoons.release(index);
         //DCP 03/30/07: Not hooking this equivalent up until we decide we need to.
         //gTriggerManager.invalidatePlatoonID(id);
         break;
      }
   case BEntity::cClassTypeArmy:
      {
         mArmies.release(index);
         //DCP 03/30/07: Not hooking this equivalent up until we decide we need to.
         //gTriggerManager.invalidateArmyID(id);
         break;
      }
   case BEntity::cClassTypeDopple:
      {
         mDopples.release(index);
         break;
      }
   case BEntity::cClassTypeProjectile:
      {
         removeFromVisibleLists(id);
         mProjectiles.release(index);
         break;
      }
   default:
      {
         BFAIL("unknown type in BObjectManager::releaseObject()");
         return;
      }
   }
}



//==============================================================================
// BObjectManager::getEntityConst
//==============================================================================
const BEntity* BObjectManager::getEntityConst( BEntityID id, bool anyCount ) const
{

   if (!id.isValid())
      return NULL;
      

   uint index = (uint) id.getIndex();
   uint type = (uint) id.getType();

   const BEntity *pEnt = NULL;

   switch (type)
   {
   case BEntity::cClassTypeObject:
      
      if (index >= mObjects.getSize())
         return NULL;
         
      if (!mObjects.isInUse(index))
         return NULL;         

      pEnt = mObjects.getConst(index);
      break;

   case BEntity::cClassTypeUnit:

      if (index >= mUnits.getSize())
         return NULL;

      if (!mUnits.isInUse(index))
         return NULL;
         
      pEnt = mUnits.getConst(index);
      break;

   case BEntity::cClassTypeSquad:

      if (index >= mSquads.getSize())
         return NULL;

      if (!mSquads.isInUse(index))
         return NULL;
         
      pEnt = mSquads.getConst(index);
      break;

   case BEntity::cClassTypePlatoon:

      if (index >= mPlatoons.getSize())
         return NULL;

      if (!mPlatoons.isInUse(index))
         return NULL;
         
      pEnt = mPlatoons.getConst(index);
      break;

   case BEntity::cClassTypeArmy:

      if (index >= mArmies.getSize())
         return NULL;

      if (!mArmies.isInUse(index))
         return NULL;
         
      pEnt = mArmies.getConst(index);
      break;

   case BEntity::cClassTypeDopple:

      if (index >= mDopples.getSize())
         return NULL;
         
      if (!mDopples.isInUse(index))
         return NULL;

      pEnt = mDopples.getConst(index);
      break;

   case BEntity::cClassTypeProjectile:

      if (index >= mProjectiles.getSize())
         return NULL;
         
      if (!mProjectiles.isInUse(index))
         return NULL;

      pEnt = mProjectiles.getConst(index);
      break;

   default:
      BFAIL("unknown type in BObjectManager::getEntityConst()");
      return NULL;
   }

   if (anyCount)
      return pEnt;

   if (!pEnt)
      return NULL;

   long requestedRef = (long)id.getUseCount();
   long currentRef = (long) pEnt->getID().getUseCount();

   if (requestedRef == currentRef)
      return pEnt;

   //-- if we get here, then the usecount is old.
   return NULL;
}

//==============================================================================
// BObjectManager::validateEntityID
//==============================================================================
bool BObjectManager::validateEntityID(BEntityID id) const
{
   uint index = (uint) id.getIndex();
   uint type = (uint) id.getType();

   switch (type)
   {
      case BEntity::cClassTypeObject:
         if (index >= mObjects.getHighWaterMark() || !mObjects.isInUse(index))
            return false;
         break;

      case BEntity::cClassTypeUnit:
         if (index >= mUnits.getHighWaterMark() || !mUnits.isInUse(index))
            return false;
         break;

      case BEntity::cClassTypeSquad:
         if (index >= mSquads.getHighWaterMark() || !mSquads.isInUse(index))
            return false;
         break;

      case BEntity::cClassTypePlatoon:
         if (index >= mPlatoons.getHighWaterMark() || !mPlatoons.isInUse(index))
            return false;
         break;

      case BEntity::cClassTypeArmy:
         if (index >= mArmies.getHighWaterMark() || !mArmies.isInUse(index))
            return false;
         break;

      case BEntity::cClassTypeDopple:
         if (index >= mDopples.getHighWaterMark() || !mDopples.isInUse(index))
            return false;
         break;

      case BEntity::cClassTypeProjectile:
         if (index >= mProjectiles.getHighWaterMark() || !mProjectiles.isInUse(index))
            return false;
         break;

      default:
         return false;
   }

   const BEntity* pEnt=getEntityConst(id, false);
   if (!pEnt)
      return false;

   return true;
}

//==============================================================================
// BObjectManager::getEntity
//==============================================================================
BEntity* BObjectManager::getEntity( BEntityID id, bool anyCount ) 
{
   return const_cast<BEntity*> (getEntityConst(id, anyCount));
}

//==============================================================================
// BObjectManager::getEntity
//==============================================================================
BEntity * BObjectManager::getEntity( long lower32ID, bool anyCount )
{
   BEntityID entityID = BEntityID(lower32ID);
   return getEntity(entityID, anyCount);
};

//==============================================================================
// BObjectManager:getEntity
//
// MUST BE UPDATED WHEN ADDING A NEW ARRAY TO THIS MANAGER
//==============================================================================
uint BObjectManager::getNumberEntities(void) const
{
   uint count = 0;
   count += mObjects.getNumberAllocated();
   count += mSquads.getNumberAllocated();
   count += mPlatoons.getNumberAllocated();
   count += mArmies.getNumberAllocated();
   count += mDopples.getNumberAllocated();
   count += mUnits.getNumberAllocated();
   count += mProjectiles.getNumberAllocated();

   return count;
}

//==============================================================================
// BObjectManager:getEntity
//
// MUST BE UPDATED WHEN ADDING A NEW ARRAY TO THIS MANAGER
//==============================================================================
BEntity* BObjectManager::getNextEntity(BEntityHandle& handle) 
{
   bool nextArray = false;
   while (true)
   { 
      //-- if this is the invalid object id, then we force the type to the first array
      //-- we want to search.  We search until we reach the end of that array.
      //-- After that, we bump this value and search the next array
      //-- when we are at the end of the last array, we return NULL;
      long type = handle.getType();
      if (!handle.isValid())
      {
         type = BEntity::cClassTypeObject;
      }
      //else if (handle.getData() == cNextArray)
      else if (nextArray)
      {
         //-- look for the sentinel that says we are wrapping
         type = handle.getType();
         handle.invalidate();
         nextArray = false;        
      }

      //-- order matters here.  These are in the order that they are seached.
      //-- for sanity sake, make sure you don't change this.  This will allow for 
      //-- future optimization.
      //-- get an item
      BEntity *pEnt = NULL;
      switch (type)
      {

         case BEntity::cClassTypeObject:
            pEnt = (BEntity*) getNextObject(handle);
            break;

         case BEntity::cClassTypeUnit:
            pEnt = (BEntity*) getNextUnit(handle);
            break;

         case BEntity::cClassTypeSquad:
            pEnt = (BEntity*) getNextSquad(handle);
            break;

         case BEntity::cClassTypePlatoon:
            pEnt = (BEntity*) getNextPlatoon(handle);
            break;

         case BEntity::cClassTypeArmy:
            pEnt = (BEntity*) getNextArmy(handle);
            break;

         case BEntity::cClassTypeDopple:
            pEnt = (BEntity*) getNextDopple(handle);
            break;

         case BEntity::cClassTypeProjectile:
            pEnt = (BEntity*) getNextProjectile(handle);
            break;

         default:
            BFAIL("Invalid object type in GetNextEntity().");
      }


      
      //-- do the check for end of a particular array
      if (!pEnt && type < BEntity::cClassTypeProjectile)
      {
         handle.set(type+1, handle.getUseCount(), 0);
         nextArray = true;
      }
      else
         return pEnt;
   }

      
   
}


//==============================================================================
// BObjectManager::getObjectConst
//==============================================================================
const BObject* BObjectManager::getObjectConst( BEntityID id, bool anyCount ) const
{
   long type = (long) id.getType();
   if ((type != BEntity::cClassTypeObject) 
      && (type != BEntity::cClassTypeUnit) 
      && (type != BEntity::cClassTypeDopple)
      && (type != BEntity::cClassTypeProjectile))
      return NULL;

   return reinterpret_cast<const BObject*>(getEntityConst(id, anyCount));
}

//==============================================================================
// BObjectManager::getObject
//==============================================================================
BObject* BObjectManager::getObject( BEntityID id, bool anyCount ) 
{
   long type = (long)id.getType();

   if ((type != BEntity::cClassTypeObject) && 
      (type != BEntity::cClassTypeUnit) && 
      (type != BEntity::cClassTypeDopple) &&
      (type != BEntity::cClassTypeProjectile))
      return NULL;

   return reinterpret_cast<BObject*>(getEntity(id, anyCount));
}

//==============================================================================
// BObjectManager::getNextObject
//==============================================================================
BObject* BObjectManager::getNextObject(BEntityHandle &handle)
{
 
   uint index = 0;
   if (handle.isValid())
      index = (uint)handle.getIndex()+1;

   if (index >= mObjects.getSize())
      return NULL;

   while (index < mObjects.getHighWaterMark())
   {
      if (mObjects.isInUse(index))
      {
         //-- adjust the handle
         handle.set(BEntity::cClassTypeObject, handle.getUseCount(), index);
         return mObjects.get(index);
      }

      //-- bump
      index++;
   }
  
   return NULL;
}

//==============================================================================
//==============================================================================
BObject* BObjectManager::getFirstUpdateObject(uint& handle)
{
   handle = 0;
   while (handle < mObjects.getHighWaterMark())
   {
      if (mObjects.isInUse(handle) && !getObjectNoUpdate(handle))
         return mObjects.get(handle);
      handle++;
   }
   return NULL;
}

//==============================================================================
//==============================================================================
BObject* BObjectManager::getNextUpdateObject(uint& handle)
{
   handle++;
   while (handle < mObjects.getHighWaterMark())
   {
      if (mObjects.isInUse(handle) && !getObjectNoUpdate(handle))
         return mObjects.get(handle);
      handle++;
   }
   return NULL;
}

//==============================================================================
// BObjectManager::getDoppleConst
//==============================================================================
const BDopple* BObjectManager::getDoppleConst( BEntityID id, bool anyCount ) const
{
   if (!id.isValid())
      return NULL;

   uint index = (uint) id.getIndex();
   uint type = (uint) id.getType();

   if (type != BEntity::cClassTypeDopple)
      return NULL;
      
   if (index >= mDopples.getSize())
      return NULL;
      
   if (!mDopples.isInUse(index))
      return NULL;

   const BDopple *pEnt = reinterpret_cast<const BDopple*>(mDopples.getConst(index));

   if (anyCount)
      return pEnt;

   if (!pEnt)
      return NULL;

   long requestedRef = (long)id.getUseCount();
   long currentRef = (long) pEnt->getID().getUseCount();

   if (requestedRef == currentRef)
      return pEnt;

   //-- if we get here, then the usecount is old.
   return NULL;
}

//==============================================================================
// BObjectManager::getDopple
//==============================================================================
BDopple* BObjectManager::getDopple( BEntityID id, bool anyCount ) 
{
   return const_cast<BDopple*>(getDoppleConst(id, anyCount));
}

//==============================================================================
// BObjectManager::getDopple
//==============================================================================
BDopple * BObjectManager::getDopple( long lower32ID, bool anyCount )
{
   BEntityID entityID = BEntityID(lower32ID);
   return getDopple(entityID, anyCount);
}

//==============================================================================
// BObjectManager::getNextDopple
//==============================================================================
BDopple* BObjectManager::getNextDopple(BEntityHandle &handle)
{

   uint index = 0;
   if (handle.isValid())
      index = (uint)handle.getIndex()+1;

   if (index >= mDopples.getSize())
      return NULL;

   while (index < mDopples.getHighWaterMark())
   {
      if (mDopples.isInUse(index))
      {
         //-- adjust the handle
         handle.set(BEntity::cClassTypeDopple, handle.getUseCount(), index);
         return mDopples.get(index);
      }

      //-- bump
      index++;
   }

   return NULL;
}




//==============================================================================
// BObjectManager::getProjectile
//==============================================================================
BProjectile* BObjectManager::getProjectile( BEntityID id, bool anyCount ) 
{
   if (!id.isValid())
      return NULL;

   uint index = (uint) id.getIndex();
   uint type = (uint) id.getType();

   if (type != BEntity::cClassTypeProjectile)
      return NULL;

   if (index >= mProjectiles.getSize())
      return NULL;
      
   if (!mProjectiles.isInUse(index))
      return NULL;

   BProjectile *pEnt = const_cast<BProjectile*>(mProjectiles.getConst(index));

   if (anyCount)
      return pEnt;

   if (!pEnt)
      return NULL;

   long requestedRef = (long)id.getUseCount();
   long currentRef = (long) pEnt->getID().getUseCount();

   if (requestedRef == currentRef)
      return pEnt;

   //-- if we get here, then the usecount is old.
   return NULL;
}

//==============================================================================
// BObjectManager::getProjectile
//==============================================================================
BProjectile * BObjectManager::getProjectile( long lower32ID, bool anyCount )
{
   BEntityID entityID = BEntityID(lower32ID);
   return getProjectile(entityID, anyCount);
}


//==============================================================================
// BObjectManager::getNextProjectile
//==============================================================================
BProjectile* BObjectManager::getNextProjectile(BEntityHandle &handle)
{

   uint index = 0;
   if (handle.isValid())
      index = (uint)handle.getIndex()+1;

   if (index >= mProjectiles.getSize())
      return NULL;

   while (index < mProjectiles.getHighWaterMark())
   {
      if (mProjectiles.isInUse(index))
      {
         //-- adjust the handle
         handle.set(BEntity::cClassTypeProjectile, handle.getUseCount(), index);
         return mProjectiles.get(index);
      }

      //-- bump
      index++;
   }

   return NULL;
}

//==============================================================================
// BObjectManager::getUnitConst
//==============================================================================
const BUnit* BObjectManager::getUnitConst( BEntityID id, bool anyCount ) const
{
   if (!id.isValid())
      return NULL;

   uint index = (uint) id.getIndex();
   uint type = (uint) id.getType();

   if (type != BEntity::cClassTypeUnit)
      return NULL;

   if (index >= mUnits.getSize())
      return NULL;

   if (!mUnits.isInUse(index))
      return NULL;
      
   const BUnit *pEnt = reinterpret_cast<const BUnit*>(mUnits.getConst(index));

   if (anyCount)
      return pEnt;

   if (!pEnt)
      return NULL;

   long requestedRef = (long)id.getUseCount();
   long currentRef = (long) pEnt->getID().getUseCount();

   if (requestedRef == currentRef)
      return pEnt;

   //-- if we get here, then the usecount is old.
   return NULL;
}

//==============================================================================
// BObjectManager::getUnit
//==============================================================================
BUnit* BObjectManager::getUnit( BEntityID id, bool anyCount ) 
{
    return const_cast<BUnit*>(getUnitConst(id, anyCount));
}

//==============================================================================
// BObjectManager::getUnit
//==============================================================================
BUnit * BObjectManager::getUnit( long lower32ID, bool anyCount )
{
   BEntityID entityID = BEntityID(lower32ID);
   return getUnit(entityID, anyCount);
};

//==============================================================================
// BObjectManager::getNextUnit
//==============================================================================
BUnit* BObjectManager::getNextUnit(BEntityHandle &handle)
{

   uint index = 0;
   if (handle.isValid())
      index = (uint)handle.getIndex()+1;

   if (index >= mUnits.getSize())
      return NULL;

   while (index < mUnits.getHighWaterMark())
   {
      if (mUnits.isInUse(index))
      {
         //-- adjust the handle
         handle.set(BEntity::cClassTypeUnit, handle.getUseCount(), index);
         return mUnits.get(index);
      }

      //-- bump
      index++;
   }

   return NULL;
}

//==============================================================================
// BObjectManager::getSquadConst
//==============================================================================
const BSquad* BObjectManager::getSquadConst( BEntityID id, bool anyCount ) const
{
   if (!id.isValid())
      return NULL;

   uint index = (uint) id.getIndex();
   uint type = (uint) id.getType();

   if (type != BEntity::cClassTypeSquad)
      return NULL;

   if (index >= mSquads.getSize())
      return NULL;

   if (!mSquads.isInUse(index))
      return NULL;
      
   const BSquad *pEnt = reinterpret_cast<const BSquad*>(mSquads.getConst(index));

   if (anyCount)
      return pEnt;

   if (!pEnt)
      return NULL;

   long requestedRef = (long)id.getUseCount();
   long currentRef = (long) pEnt->getID().getUseCount();

   if (requestedRef == currentRef)
      return pEnt;

   //-- if we get here, then the usecount is old.
   return NULL;
}

//==============================================================================
// BObjectManager::getSquad
//==============================================================================
BSquad* BObjectManager::getSquad( BEntityID id, bool anyCount )
{
   return const_cast<BSquad*>(getSquadConst(id, anyCount));
}

//==============================================================================
// BObjectManager::getNextSquad
//==============================================================================
BSquad* BObjectManager::getNextSquad(BEntityHandle &handle)
{

   uint index = 0;
   if (handle.isValid())
      index = (uint)handle.getIndex()+1;

   if (index >= mSquads.getSize())
      return NULL;

   while (index < mSquads.getHighWaterMark())
   {
      if (mSquads.isInUse(index))
      {
         //-- adjust the handle
         handle.set(BEntity::cClassTypeSquad, handle.getUseCount(), index);
         return mSquads.get(index);
      }

      //-- bump
      index++;
   }

   return NULL;
}

//==============================================================================
// BObjectManager::getPlatoonConst
//==============================================================================
const BPlatoon* BObjectManager::getPlatoonConst( BEntityID id, bool anyCount ) const
{
   if (!id.isValid())
      return NULL;

   uint index = (uint) id.getIndex();
   uint type = (uint) id.getType();

   if (type != BEntity::cClassTypePlatoon)
      return NULL;

   if (index >= mPlatoons.getSize())
      return NULL;

   if (!mPlatoons.isInUse(index))
      return NULL;
      
   const BPlatoon *pEnt = reinterpret_cast<const BPlatoon*>(mPlatoons.getConst(index));

   if (anyCount)
      return pEnt;

   if (!pEnt)
      return NULL;

   long requestedRef = (long)id.getUseCount();
   long currentRef = (long) pEnt->getID().getUseCount();

   if (requestedRef == currentRef)
      return pEnt;

   //-- if we get here, then the usecount is old.
   return NULL;
}

//==============================================================================
// BObjectManager::getPlatoon
//==============================================================================
BPlatoon* BObjectManager::getPlatoon( BEntityID id, bool anyCount )
{
   return const_cast<BPlatoon*>(getPlatoonConst(id, anyCount));
}

//==============================================================================
// BObjectManager::getNextPlatoon
//==============================================================================
BPlatoon* BObjectManager::getNextPlatoon(BEntityHandle &handle)
{

   uint index = 0;
   if (handle.isValid())
      index = (uint)handle.getIndex()+1;

   if (index >= mPlatoons.getSize())
      return NULL;

   while (index < mPlatoons.getHighWaterMark())
   {
      if (mPlatoons.isInUse(index))
      {
         //-- adjust the handle
         handle.set(BEntity::cClassTypePlatoon, handle.getUseCount(), index);
         return mPlatoons.get(index);
      }

      //-- bump
      index++;
   }

   return NULL;
}


//==============================================================================
// BObjectManager::getArmyConst
//==============================================================================
const BArmy* BObjectManager::getArmyConst( BEntityID id, bool anyCount ) const
{
   if (!id.isValid())
      return NULL;

   uint index = (uint) id.getIndex();
   uint type = (uint) id.getType();

   if (type != BEntity::cClassTypeArmy)
      return NULL;

   if (index >= mArmies.getSize())
      return NULL;

   if (!mArmies.isInUse(index))
      return NULL;
      
   const BArmy *pEnt = reinterpret_cast<const BArmy*>(mArmies.getConst(index));

   if (anyCount)
      return pEnt;

   if (!pEnt)
      return NULL;

   long requestedRef = (long)id.getUseCount();
   long currentRef = (long) pEnt->getID().getUseCount();

   if (requestedRef == currentRef)
      return pEnt;

   //-- if we get here, then the usecount is old.
   return NULL;
}

//==============================================================================
// BObjectManager::getArmy
//==============================================================================
BArmy* BObjectManager::getArmy( BEntityID id, bool anyCount )
{
   return const_cast<BArmy*>(getArmyConst(id, anyCount));
}

//==============================================================================
// BObjectManager::getNextArmy
//==============================================================================
BArmy* BObjectManager::getNextArmy(BEntityHandle &handle)
{

   uint index = 0;
   if (handle.isValid())
      index = (uint)handle.getIndex()+1;

   if (index >= mArmies.getSize())
      return NULL;

   while (index < mArmies.getHighWaterMark())
   {
      if (mArmies.isInUse(index))
      {
         //-- adjust the handle
         handle.set(BEntity::cClassTypeArmy, handle.getUseCount(), index);
         return mArmies.get(index);
      }

      //-- bump
      index++;
   }

   return NULL;
}

//==============================================================================
// BObjectManager::getBumpUseCount
// mrh 8/22/06 - Note: This returns the bumped use count handle, it doesn't modify the
// input handle.
//==============================================================================
BEntityID BObjectManager::getBumpUseCount( BEntityID id ) 
{
   return(id.getBumpUseCount());
}


//==============================================================================
// BObjectManager::upconvertEntityID
//==============================================================================
BEntityID BObjectManager::upconvertEntityID( long id ) const
{
   BEntityID id64 = BEntityID(id);
   const BEntity *pEntity = getEntityConst(id64);
   if (pEntity)
      return pEntity->getID();
   
   //BFAIL("Upconvert failed.");
   id64.invalidate();   
   return id64;
}

//==============================================================================
// BObjectManager::verifyObjectState
//==============================================================================
void BObjectManager::verifyObjectState( void ) const
{
   //-- objects
   if (mObjects.getNumberAllocated() != 0)
   {
      blogtrace("ObjectManager - object array still has %d entries!", mObjects.getNumberAllocated());
      BASSERTM(0, "ObjectManager - object array still has entries");
   }
   //-- units
   if (mUnits.getNumberAllocated() != 0)
   {
      blogtrace("ObjectManager - unit array still has %d entries!", mUnits.getNumberAllocated());
      BASSERTM(0, "ObjectManager - unit array still has entries");
   }
   
   //-- squads
   if (mSquads.getNumberAllocated() != 0)
   {
      blogtrace("ObjectManager - squad array still has %d entries!", mSquads.getNumberAllocated());
      BASSERTM(0, "ObjectManager - squad array still has entries");
   }

   //-- platoons
   if (mPlatoons.getNumberAllocated() != 0)
   {
      blogtrace("ObjectManager - platoon array still has %d entries!", mPlatoons.getNumberAllocated());
      BASSERTM(0, "ObjectManager - platoon array still has entries");
   }

   //-- armies
   if (mArmies.getNumberAllocated() != 0)
   {
      blogtrace("ObjectManager - army array still has %d entries!", mArmies.getNumberAllocated());
      BASSERTM(0, "ObjectManager - army array still has entries");
   }

   //-- dopples
   if (mDopples.getNumberAllocated() != 0)
   {
      blogtrace("ObjectManager - dopple array still has %d entries!", mDopples.getNumberAllocated());
      BASSERTM(0, "ObjectManager - dopple array still has entries");
   }

   //-- projectiles
   if (mProjectiles.getNumberAllocated() != 0)
   {
      blogtrace("ObjectManager - projectile array still has %d entries!", mProjectiles.getNumberAllocated());
      BASSERTM(0, "ObjectManager - projectile array still has entries");
   }

   //-- airspots
   if (mAirSpots.getNumberAllocated() != 0)
   {
      blogtrace("ObjectManager - airspots array still has %d entries!", mAirSpots.getNumberAllocated());
      BASSERTM(0, "ObjectManager - airspot array still has entries");
   }
}


//==============================================================================
// BObjectManager::createClaimedAirSpot
//==============================================================================
BAirSpot* BObjectManager::createClaimedAirSpot(uint &index)
{
   index = cMaxAirSpotIndex;
   BAirSpot* pNewClaimedAirSpot = NULL;

   pNewClaimedAirSpot = mAirSpots.acquire(index, true);
   BASSERT(index < cMaxAirSpotIndex);

   return (pNewClaimedAirSpot);
}

//==============================================================================
// BObjectManager::createClaimedAirSpotAtIndex
//==============================================================================
BAirSpot* BObjectManager::createClaimedAirSpotAtIndex(uint index)
{
   BAirSpot* pNewClaimedAirSpot = mAirSpots.acquireAtIndex(index);
   return (pNewClaimedAirSpot);
}

//==============================================================================
// BObjectManager::getClaimedAirSpot
//==============================================================================
BAirSpot* BObjectManager::getClaimedAirSpot(uint &index)
{
   BAirSpot* pAirSpot = NULL;
   pAirSpot = &mAirSpots[index];

   return (pAirSpot);
}

//==============================================================================
// BObjectManager::getNextClaimedAirSpot
//==============================================================================
BAirSpot* BObjectManager::getNextClaimedAirSpot(uint &index)
{
   if (index >= mAirSpots.getSize())
      return NULL;

   while (index < mAirSpots.getHighWaterMark())
   {
      if (mAirSpots.isInUse(index))
         return mAirSpots.get(index);

      //-- bump
      index++;
   }

   return NULL;
}

//==============================================================================
//==============================================================================
void BObjectManager::setObjectNoUpdate(BEntityID id, bool val)
{
   uint index = id.getIndex();
   //mpNoUpdateObjects[index] = val;
   if (val)
      mNoUpdateObjects[index >> 6U] |= Utils::BBitMasks::get64(63U - (index & 63U));
   else
      mNoUpdateObjects[index >> 6U] &= Utils::BBitMasks::getInverted64(63U - (index & 63U));
   #ifdef DEBUG_VERIFY_OBJECT_NO_UPDATE
      BObject* pObject = mObjects.get(index);
      bool noUpdate1, noUpdate2;
      noUpdate1 = (getObjectNoUpdate(index) ? true : false);
      noUpdate2 = pObject->getFlagNoUpdate();
      BASSERT(noUpdate1 == noUpdate2);
   #endif
}

//==============================================================================
//==============================================================================
BOOL BObjectManager::getObjectNoUpdate(BEntityID id) const
{
   uint index = id.getIndex();
   //return mpNoUpdateObjects[index];
   return getObjectNoUpdate(index);
}

//==============================================================================
//==============================================================================
BOOL BObjectManager::getObjectNoUpdate(uint index) const
{
   //return mpNoUpdateObjects[index];
   return (0U != (mNoUpdateObjects[index >> 6U] & Utils::BBitMasks::get64(63U - (index & 63U))));
}

//==============================================================================
//==============================================================================
bool BAirSpot::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mClaimedPos);
   GFWRITEVAL(pStream, BEntityID, mAircraftID);
   return true;
}

//==============================================================================
//==============================================================================
bool BAirSpot::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mClaimedPos);
   BEntityID entityID;
   GFREADVAR(pStream, BEntityID, mAircraftID);
   return true;
}

//==============================================================================
//==============================================================================
BObject* BObjectManager::getObjectByIndex(uint index, bool& inUse)
{
   if (index >= mObjects.getSize())
   {
      inUse = false;
      return NULL;
   }
   inUse = (mObjects.isInUse(index) != 0);
   return &(mObjects.getElement(index));
}

//==============================================================================
//==============================================================================
BUnit* BObjectManager::getUnitByIndex(uint index, bool& inUse)
{
   if (index >= mUnits.getSize())
   {
      inUse = false;
      return NULL;
   }
   inUse = (mUnits.isInUse(index) != 0);
   return &(mUnits.getElement(index));
}

//==============================================================================
//==============================================================================
BDopple* BObjectManager::getDoppleByIndex(uint index, bool& inUse)
{
   if (index >= mDopples.getSize())
   {
      inUse = false;
      return NULL;
   }
   inUse = (mDopples.isInUse(index) != 0);
   return &(mDopples.getElement(index));
}

//==============================================================================
//==============================================================================
BProjectile* BObjectManager::getProjectileByIndex(uint index, bool& inUse)
{
   if (index >= mProjectiles.getSize())
   {
      inUse = false;
      return NULL;
   }
   inUse = (mProjectiles.isInUse(index) != 0);
   return &(mProjectiles.getElement(index));
}

//==============================================================================
//==============================================================================
BSquad* BObjectManager::getSquadByIndex(uint index, bool& inUse)
{
   if (index >= mSquads.getSize())
   {
      inUse = false;
      return NULL;
   }
   inUse = (mSquads.isInUse(index) != 0);
   return &(mSquads.getElement(index));
}

//==============================================================================
//==============================================================================
BPlatoon* BObjectManager::getPlatoonByIndex(uint index, bool& inUse)
{
   if (index >= mPlatoons.getSize())
   {
      inUse = false;
      return NULL;
   }
   inUse = (mPlatoons.isInUse(index) != 0);
   return &(mPlatoons.getElement(index));
}

//==============================================================================
//==============================================================================
BArmy* BObjectManager::getArmyByIndex(uint index, bool& inUse)
{
   if (index >= mArmies.getSize())
   {
      inUse = false;
      return NULL;
   }
   inUse = (mArmies.isInUse(index) != 0);
   return &(mArmies.getElement(index));
}
