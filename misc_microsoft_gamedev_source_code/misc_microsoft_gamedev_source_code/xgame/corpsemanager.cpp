//============================================================================
// corpsemanager.cpp
//  
// Copyright (c) 2007 Ensemble Studios
//============================================================================

#include "common.h"
#include "corpsemanager.h"
#include "game.h"
#include "unit.h"
#include "database.h"
#include "protoobject.h"
#include "obstructionmanager.h"
#include "world.h"
#include "worldsoundmanager.h"

BCorpseManager gCorpseManager;

GFIMPLEMENTVERSION(BCorpseManager, 1);

enum
{
   cSaveMarkerCorpseManager=10000,
};

//==============================================================================
// BCorpse::init
//==============================================================================
void BCorpse::init(BUnit* pUnit)
{
   BASSERT(pUnit);
   const BProtoObject* pProto = pUnit->getProtoObject();
   BDEBUG_ASSERT(pProto);

   mTimeStamp = gGame.getGametime();
   mUnitID = pUnit->getID();
   mFlagIsInfantry = pProto->isType(gDatabase.getOTIDInfantry());
   mFlagRemove = false;
   mFlagIgnore = false;

   // Create obstruction entries for non-infantry corpses
   if (!mFlagIsInfantry)
      pUnit->createCorpseObstruction();
}

//==============================================================================
// BCorpse::setTimeStamp
//==============================================================================
void BCorpse::setTimeStamp()
{
   mTimeStamp = gGame.getGametime();
}

//==============================================================================
// BCorpse::getUnit
//==============================================================================
BUnit* BCorpse::getUnit() const
{
   return gWorld->getUnit(mUnitID);
}

//==============================================================================
// BCorpseManager::reset
//==============================================================================
void BCorpseManager::reset()
{
   mCorpses.clear();
   mEntityToCorpse.clear();
   mNumQueuedRemovals = 0;
   mTimeOfLastRemoval = 0;
   mRemoveCount = 0;
}

//==============================================================================
// BCorpseManager::registerCorpse
//==============================================================================
void BCorpseManager::registerCorpse(BEntityID unitID)
{
   BUnit* pUnit = gWorld->getUnit(unitID);
   BASSERT(pUnit);
   const BProtoObject* pProto = pUnit->getProtoObject();
   BDEBUG_ASSERT(pProto);

   bool registerMe = pProto->isType(gDatabase.getOTIDInfantry());

   // If we're registering non-infantry wreckage, make sure it's not in an obstructed position
   if (!registerMe)
   {
      long obstructionQuadTrees =
         BObstructionManager::cIsNewTypeNonCollidableUnit |          // Other corpses
         BObstructionManager::cIsNewTypeCollidableNonMovableUnit |   // Unit that can't move
         BObstructionManager::cIsNewTypeCollidableMovingUnit |       // Unit that can Move and *IS* currently in motion
         BObstructionManager::cIsNewTypeCollidableStationaryUnit;    // Unit that can Move and *IS NOT* currently in motion

      // Find obstructions
      BObstructionNodePtrArray obstructions;
      BVector position = pUnit->getPosition();
      float obstructionRadius = pUnit->getObstructionRadius();
      float x1 = position.x - obstructionRadius;
      float z1 = position.z - obstructionRadius;
      float x2 = position.x + obstructionRadius;
      float z2 = position.z + obstructionRadius;
      gObsManager.findObstructions(x1, z1, x2, z2, 0.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAllSolid | BObstructionManager::cObsNodeTypeCorpse, -1, false, obstructions);

      // Are we a big piece of wreckage?
      bool bigCorpse = (pUnit->getProtoID() != gDatabase.getPOIDPhysicsThrownObject());

      registerMe = true;

      // Iterate through them
      long numObstructions = obstructions.getNumber();
      for (long i = 0; i < numObstructions; i++)
      {
         const BOPObstructionNode* pNode = obstructions.get(i);
         BASSERT(pNode);

         // If we're co-habitating with another corpse, make the lesser or older one sink into the ground
         if ((pNode->mThisNodeQuadTree == BObstructionManager::cObsTypeNonCollidableUnit) && (pNode->mType & BObstructionManager::cObsNodeTypeCorpse))
         {
            BASSERT(pNode->mObject);

//-- FIXING PREFIX BUG ID 4254
            const BUnit* pObstructionUnit = pNode->mObject->getUnit();
//--

            if (pObstructionUnit)
            {
               // If we're big, make this sink into the ground
               if (bigCorpse)
               {
                  #ifdef SYNC_Unit
                     syncUnitCode("BCorpseManager::registerCorpse big corpse landing on existing corpse - remove existing corpse");
                  #endif
                  gCorpseManager.removeCorpse(pObstructionUnit->getID());
               }
               else
               {
                  // Is it a big obstruction?
                  bool bigObstruction = (pObstructionUnit->getProtoID() != gDatabase.getPOIDPhysicsThrownObject());

                  // If the obstruction is bigger, don't bother registering the corpse
                  if (bigObstruction)
                  {
                     #ifdef SYNC_Unit
                        syncUnitCode("BCorpseManager::registerCorpse small corpse landing on existing big corpse - remove new corpse");
                     #endif
                     registerMe = false;
                     break;
                  }
                  else
                  {
                     #ifdef SYNC_Unit
                        syncUnitCode("BCorpseManager::registerCorpse small corpse landing on existing small corpse - remove existing corpse");
                     #endif
                     // If we're both small, make the obstruction sink into the ground
                     gCorpseManager.removeCorpse(pObstructionUnit->getID());
                  }
               }
            }
         }
         // If we're really obstructed, don't bother registering the corpse
         else
         {
            #ifdef SYNC_Unit
            {
               BASSERT(pNode->mObject);
               #ifdef BUILD_DEBUG
                  BObject* pObstructionUnit = pNode->mObject->getUnit();
                  BASSERT(pObstructionUnit);
               #endif
               syncUnitCode("BCorpseManager::registerCorpse corpse landing on obstruction - remove new corpse");
            }
            #endif
            registerMe = false;
            break;
         }
      }
   }

   // Register me
   if (registerMe)
   {
      // Queue a corpse removal if we need to
      long numCorpses = mCorpses.getNumberAllocated();
      if ((numCorpses - mNumQueuedRemovals) >= gDatabase.getMaxNumCorpses())
         queueRemoveOldestCorpse();

      // Add new corpse
      uint index = 0;
      BCorpse* pCorpse = mCorpses.acquire(index, true);
      BASSERT(pCorpse);
      pCorpse->init(pUnit);

      mEntityToCorpse.insert(unitID, index);

      #ifdef SYNC_Unit
         syncUnitData("BCorpseManager::registerCorpse add corpseIndex", findCorpse(unitID));
         syncUnitData("BCorpseManager::registerCorpse add at position", pUnit->getPosition());
         syncUnitData("BCorpseManager::registerCorpse add unit ID", unitID);
      #endif
   }
   else
   {
      #ifdef SYNC_Unit
         syncUnitData("BCorpseManager::registerCorpse cEventCorpseRemove at position", pUnit->getPosition());
         syncUnitData("BCorpseManager::registerCorpse cEventCorpseRemove unit ID", unitID);
      #endif

      // You're obstructed. Go away.
      pUnit->notify(BEntity::cEventCorpseRemove, unitID, 0, 0);
   }
}

//==============================================================================
// BCorpseManager::queueRemoveOldestCorpse
//==============================================================================
void BCorpseManager::queueRemoveOldestCorpse()
{
   // Find the oldest corpse
   long oldestCorpse = findOldestCorpse(false);

   // Queue it
   if (oldestCorpse != -1)
   {
      #ifdef SYNC_Unit
         syncUnitData("BCorpseManager::queueRemoveOldestCorpse oldestCorpse", oldestCorpse);
      #endif
      queueRemoveCorpse(oldestCorpse);
   }
}

//==============================================================================
// BCorpseManager::queueRemoveCorpse
//==============================================================================
void BCorpseManager::queueRemoveCorpse(long corpseIndex)
{
   BCorpse* pCorpse = mCorpses.get(corpseIndex);
   BASSERT(pCorpse);
   pCorpse->setTimeStamp();
   pCorpse->setFlagRemove();
   mNumQueuedRemovals++;
}

//==============================================================================
// BCorpseManager::findOldestCorpse
//==============================================================================
long BCorpseManager::findOldestCorpse(bool queued) const
{
   // Scan through the list and find the oldest corpse
   long oldestCorpse = -1;
   DWORD oldestTime = gGame.getGametime() + 1;

   long highWaterMark = mCorpses.getHighWaterMark();
   for (long i = 0; i < highWaterMark; i++)
   {
      if (mCorpses.isInUse(i))
      {
         const BCorpse* pCorpse = mCorpses.getConst(i);
         BASSERT(pCorpse);
         if (pCorpse->getFlagRemove() == queued && !pCorpse->getFlagIgnore())
         {
            DWORD timeOfDeath = pCorpse->getTimeStamp();
            if (timeOfDeath < oldestTime)
            {
               oldestTime = timeOfDeath;
               oldestCorpse = i;
            }
         }
      }
   }

   return oldestCorpse;
}

//==============================================================================
// BCorpseManager::removeCorpse
//==============================================================================
void BCorpseManager::removeCorpse(long corpseIndex, bool doEffect)
{
   #ifdef SYNC_Unit
      syncUnitData("BCorpseManager::removeCorpse remove corpseIndex", corpseIndex);
   #endif

   BCorpse* pCorpse = mCorpses.get(corpseIndex);
   BASSERT(pCorpse);
   if (!pCorpse)
      return;
   BUnit* pUnit = pCorpse->getUnit();

   // Dec queue count if this was queued
   if (pCorpse->getFlagRemove() && !pCorpse->getFlagIgnore())
      mNumQueuedRemovals--;

   if (pUnit)
   {
      // Do effect
      if (doEffect && (pUnit->getProtoID() != gDatabase.getPOIDPhysicsThrownObject()))
      {
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         BDEBUG_ASSERT(pProtoObject);
         long corpseDeathVisualIndex = pProtoObject->getProtoCorpseDeathVisualIndex();

         if (corpseDeathVisualIndex != -1)
         {
            //-- Create impact effect
            BObjectCreateParms parms;
            parms.mPlayerID = pUnit->getPlayerID();
            parms.mPosition = pUnit->getPosition();
            parms.mRight = cXAxisVector;
            parms.mForward = cZAxisVector;      
            gWorld->createTempObject(parms, corpseDeathVisualIndex, 5.0f, -1, false, cVisualDisplayPriorityNormal);

            //-- Trigger sound
            BCueIndex cueIndex = pProtoObject->getSound(cObjectSoundCorpseDeath);
            gWorld->getWorldSoundManager()->addSound(parms.mPosition, cueIndex, true, cInvalidCueIndex, true, true);
         }
      }

      // Remove obstruction, if any
      pUnit->deleteObstruction();

      #ifdef SYNC_Unit
         syncUnitData("BCorpseManager::removeCorpse cEventCorpseRemove at position", pUnit->getPosition());
         syncUnitData("BCorpseManager::removeCorpse cEventCorpseRemove unit ID", pUnit->getID());
      #endif

      // Tell the corpse to go away
      pUnit->notify(BEntity::cEventCorpseRemove, pUnit->getID(), 0, 0);
   }

   // Remove it from the corpse manager
   mCorpses.release(corpseIndex);
   if (pUnit)
      mEntityToCorpse.erase(pUnit->getID());
}

//==============================================================================
// Get the corpse's unit ID from the corpse index
//==============================================================================
BEntityID BCorpseManager::getCorpseUnit(long corpseIndex)
{ 
   BEntityID result = cInvalidObjectID;
   if (mCorpses.isInUse(corpseIndex))
   {
//-- FIXING PREFIX BUG ID 4256
      const BCorpse* pCorpse = mCorpses.get(corpseIndex);
//--
      BASSERT(pCorpse);
      //if (pCorpse)
      {
         result = pCorpse->getUnitID();
      }
   }

   return (result);
}

//==============================================================================
// BCorpseManager::removeCorpse
//==============================================================================
void BCorpseManager::removeCorpse(BEntityID unitID, bool doEffect /*= true*/)
{
   #ifdef SYNC_Unit
      syncUnitData("BCorpseManager::removeCorpse unitID", unitID);
   #endif

   // Find the corpse associated with that unit
   long corpse = findCorpse(unitID);

   // Release it
   if (corpse != -1)
      removeCorpse(corpse, doEffect);
}

//==============================================================================
// BCorpseManager::findCorpse
//==============================================================================
long BCorpseManager::findCorpse(BEntityID unitID) const
{
   // Scan through the list and find the corpse associated with that unit
   long highWaterMark = mCorpses.getHighWaterMark();
   for (long i = 0; i < highWaterMark; i++)
   {
      if (mCorpses.isInUse(i))
      {
         const BCorpse* pCorpse = mCorpses.getConst(i);
         BASSERT(pCorpse);
         if (pCorpse->getUnitID() == unitID)
            return i;
      }
   }

   return -1;
}

//==============================================================================
// BCorpseManager::update
//==============================================================================
void BCorpseManager::update()
{
   DWORD decayTime = gGame.getGametime();

   // Scan through the list and queue decayed infantry corpses for removal
   long highWaterMark = mCorpses.getHighWaterMark();
   #ifdef SYNC_Unit
      syncUnitData("BCorpseManager::update highWaterMark", highWaterMark);
   #endif
   for (long i = 0; i < highWaterMark; i++)
   {
      if (mCorpses.isInUse(i))
      {
         const BCorpse* pCorpse = mCorpses.getConst(i);
         BASSERT(pCorpse);
         DWORD corpseTime = pCorpse->getTimeStamp() + (DWORD)(gDatabase.getInfantryCorpseDecayTime() * 1000.0f);
         if (!pCorpse->getFlagRemove() && pCorpse->getFlagIsInfantry() && (corpseTime <= decayTime))
         {
            #ifdef SYNC_Unit
               syncUnitData("BCorpseManager::update queueRemoveCorpse", i);
            #endif
            queueRemoveCorpse(i);
         }
      }
   }

   // If the last corpse removal was long enough ago, we can remove a queued corpse right now
   if (mNumQueuedRemovals > 0)
   {
      long corpseOverflow = mCorpses.getNumberAllocated() - gDatabase.getMaxNumCorpses();
      float corpseSinkingSpacingScale = (corpseOverflow > 0) ? (1.0f - (((float)corpseOverflow) / ((float)gDatabase.getMaxNumCorpses()))) : 1.0f;
      DWORD expiredTimeStamp = gGame.getGametime() - ((DWORD)(gDatabase.getCorpseSinkingSpacing() * 1000.0f * corpseSinkingSpacingScale));
      if (mTimeOfLastRemoval <= expiredTimeStamp)
      {
         while (gDatabase.getMaxCorpseDisposalCount() > mRemoveCount && mNumQueuedRemovals > 0)
         {
            long oldestCorpse = findOldestCorpse(true);
            #ifdef SYNC_Unit
               syncUnitData("BCorpseManager::update remove queued oldestCorpse", oldestCorpse);
            #endif
            if (mCorpses.isInUse(oldestCorpse))
            {
               BCorpse* pc = mCorpses.get(oldestCorpse);
               BASSERT(pc);
               //if (pc)
               {
                  removeCorpse(oldestCorpse, false);
                  pc->setFlagIgnore();
                  ++mRemoveCount;
               }
            }
         }
         if (mRemoveCount >= gDatabase.getMaxCorpseDisposalCount())
         {
            mRemoveCount = 0;

            // Time stamp it
            mTimeOfLastRemoval = gGame.getGametime();
         }
      }
   }
}

//==============================================================================
// BCorpseManager::getCorpseDelay
//==============================================================================
float BCorpseManager::getCorpseDecay(BEntityID id)
{
   EntityToCorpseCtnr::iterator iter;
   iter = mEntityToCorpse.find(id);

   if (iter == mEntityToCorpse.end())
      return 0.0f;

   long corpseID = iter->second;

   if (corpseID == -1)
      return 0.0f;
   else
   {
//-- FIXING PREFIX BUG ID 4257
      const BCorpse* pCorpse = mCorpses.get(corpseID);
//--
      BASSERT(pCorpse);
//       if (!pCorpse)
//          return 0.0f;

      float time = (float)(gGame.getGametime() - pCorpse->getTimeStamp()) / (gDatabase.getInfantryCorpseDecayTime() * 1000.0f);
      if (time < 0.0f)
         time = 0.0f;
      else if (time > 1.0f)
         time = 1.0f;

      return time;
   }
}

//==============================================================================
//==============================================================================
bool BCorpseManager::save(BStream* pStream, int saveType) const
{
   GFWRITEFREELIST(pStream, saveType, BCorpse, mCorpses, uint16, 10000);

   // mEntityToCorpse
   int count = mEntityToCorpse.size();
   GFWRITEVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   for (long i = 0; i < count; i++)
   {
      const std::pair<BEntityID, uint>& entityToCorpse = mEntityToCorpse.get(i);
      GFWRITEVAR(pStream, BEntityID, entityToCorpse.first);
      GFWRITEVAR(pStream, uint, entityToCorpse.second);
   }

   GFWRITEVAR(pStream, long, mNumQueuedRemovals);
   GFWRITEVAR(pStream, long, mRemoveCount);
   GFWRITEVAR(pStream, DWORD, mTimeOfLastRemoval);

   GFWRITEMARKER(pStream, cSaveMarkerCorpseManager);

   return true;
}

//==============================================================================
//==============================================================================
bool BCorpseManager::load(BStream* pStream, int saveType)
{
   GFREADFREELIST(pStream, saveType, BCorpse, mCorpses, uint16, 10000);

   // mEntityToCorpse
   int count;
   GFREADVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   for (int i=0; i<count; i++)
   {
      BEntityID entityID;
      uint index;
      GFREADVAR(pStream, BEntityID, entityID);
      GFREADVAR(pStream, uint, index);
      if (gWorld->getEntity(entityID))
         mEntityToCorpse.insert(entityID, index);
   }

   GFREADVAR(pStream, long, mNumQueuedRemovals);
   GFREADVAR(pStream, long, mRemoveCount);
   GFREADVAR(pStream, DWORD, mTimeOfLastRemoval);

   GFREADMARKER(pStream, cSaveMarkerCorpseManager);

   return true;
}

//==============================================================================
//==============================================================================
bool BCorpse::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, DWORD, mTimeStamp);
   GFWRITEVAR(pStream, BEntityID, mUnitID);
   GFWRITEBITBOOL(pStream, mFlagIsInfantry);
   GFWRITEBITBOOL(pStream, mFlagRemove);
   GFWRITEBITBOOL(pStream, mFlagIgnore);
   return true;
}

//==============================================================================
//==============================================================================
bool BCorpse::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, DWORD, mTimeStamp);
   GFREADVAR(pStream, BEntityID, mUnitID);
   GFREADBITBOOL(pStream, mFlagIsInfantry);
   GFREADBITBOOL(pStream, mFlagRemove);
   GFREADBITBOOL(pStream, mFlagIgnore);
   return true;
}
