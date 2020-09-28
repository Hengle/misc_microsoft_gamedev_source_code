//==============================================================================
// statsManager.cpp
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

// Includes
#include "common.h"
#include "configsgame.h"
#include "database.h"
#include "world.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "protoobject.h"
#include "prototech.h"
#include "protopower.h"
#include "protosquad.h"
#include "user.h"
#include "usermanager.h"
//#include "userprofilemanager.h"
#include "humanPlayerAITrackingData.h"
#include "statsManager.h"
#include "lspManager.h"
#include "scoreManager.h"
#include "gamemode.h"

// xsystem
#include "bfileStream.h"

// compression
#include "compressedStream.h"

// xnetwork
#include "winsockinc.h"

// Globals
BStatsManager gStatsManager;

GFIMPLEMENTVERSION(BStatsManager, 1);

enum
{
   cSaveMarkerStatsPlayer=10000,
   cSaveMarkerStatsRecorders,
   cSaveMarkerStatsPowers,
   cSaveMarkerStatsAbilities,
   cSaveMarkerStatsManager,
};

//==============================================================================
// 
//==============================================================================
IMPLEMENT_FREELIST(BStatDefinition, 4, &gSimHeap);
IMPLEMENT_FREELIST(BStatRecorderTotal, 4, &gSimHeap);
IMPLEMENT_FREELIST(BStatRecorderEvent, 4, &gSimHeap);
IMPLEMENT_FREELIST(BStatRecorderGraph, 4, &gSimHeap);
IMPLEMENT_FREELIST(BStatsPlayer, 2, &gSimHeap);

//==============================================================================
// 
//==============================================================================
static int __cdecl BStatResourcesCompareFunc(const void* pElem1, const void* pElem2)
{
   BStatRecorder::BStatResources* pStat1 = (BStatRecorder::BStatResources*)pElem1;
   BStatRecorder::BStatResources* pStat2 = (BStatRecorder::BStatResources*)pElem2;

   if (pStat1->mTimestamp < pStat2->mTimestamp)
      return -1;
   if (pStat1->mTimestamp > pStat2->mTimestamp)
      return 1;
   return 0;
}

//==============================================================================
// 
//==============================================================================
static int __cdecl BStatEventFloatCompareFunc(const void* pElem1, const void* pElem2)
{
   BStatRecorder::BStatEventFloat* pStat1 = (BStatRecorder::BStatEventFloat*)pElem1;
   BStatRecorder::BStatEventFloat* pStat2 = (BStatRecorder::BStatEventFloat*)pElem2;

   if (pStat1->mTimestamp < pStat2->mTimestamp)
      return -1;
   if (pStat1->mTimestamp > pStat2->mTimestamp)
      return 1;
   return 0;
}


//==============================================================================
// 
//==============================================================================
static int __cdecl BStatEventUInt32CompareFunc(const void* pElem1, const void* pElem2)
{
   BStatRecorder::BStatEventUInt32* pStat1 = (BStatRecorder::BStatEventUInt32*)pElem1;
   BStatRecorder::BStatEventUInt32* pStat2 = (BStatRecorder::BStatEventUInt32*)pElem2;

   if (pStat1->mTimestamp < pStat2->mTimestamp)
      return -1;
   if (pStat1->mTimestamp > pStat2->mTimestamp)
      return 1;
   return 0;
}

//==============================================================================
// 
//==============================================================================
static int __cdecl BStatEventCompareFunc(const void* pElem1, const void* pElem2)
{
   BStatRecorder::BStatEvent* pStat1 = (BStatRecorder::BStatEvent*)pElem1;
   BStatRecorder::BStatEvent* pStat2 = (BStatRecorder::BStatEvent*)pElem2;

   if (pStat1->mTimestamp < pStat2->mTimestamp)
      return -1;
   if (pStat1->mTimestamp > pStat2->mTimestamp)
      return 1;
   return 0;
}

//==============================================================================
// 
//==============================================================================
void BStatGraphDefinition::update()
{
   int16 prevIndex = mIndex;

   mIndex = mIndex + mStep;

   mSize = min(mSize+1, mSamples);

   if (mIndex < 0 || mIndex >= mSamples)
   {
      // step 2 for every other slot
      mStep = 2;

      // reverse directions
      if (mIndex >= mSamples)
         mStep = -mStep;

      // bump our recording interval out some to slow down the sample rate
      mInterval = mInterval + mOriginalInterval;

      // we ping-pong between the ends of our sample array
      // our step size will always remain 2.
      // this should degrade resolution at the upper end while
      // trying to preserve the resolution of the oldest data.
      if (prevIndex == (mSamples - 1))
         mIndex = prevIndex - 1;
      else if (prevIndex == 0)
         mIndex = 1;
      else if (mIndex < 0)
         mIndex = 0;
      else
         mIndex = mSamples - 1;
   }
}

//==============================================================================
// 
//==============================================================================
void BStatGraphDefinition::reset()
{
   mSize = 0;
   mSamples = 0;
   mInterval = 0;
   mOriginalInterval = 0;
   mStep = 0;
   mLastUpdateTime = 0;
   mIndex = 0;
   mUpload = true;
}

//==============================================================================
// 
//==============================================================================
BStatDefinition::BStatDefinition() :
   mpKillers(NULL),
   mDBID(-1),
   mObjectClass(-1),
   mType(eUnknown),
   mSize(0),
   mSamples(0),
   mInterval(5000),
   mID(0),
   mFilterObjectClass(false),
   mFilterObjectType(false),
   mFilterObjectTypeAND(false),
   mFilterObjectTypeOR(false),
   mFilterObjectTypeNOT(false),
   mFilterDBID(false),
   mFilterExcludeTechs(false),
   mTrackAllTechs(false),
   mTrackSquads(false),
   mTrackKillers(false),
   mTrackIndividual(false),
   mTrackFirstLastGameTime(false),
   mTrackTimeZero(false),
   mEventResearched(false),
   mEventBuilt(false),
   mEventLost(false),
   mEventDestroyed(false),
   mEventDestroyedTarget(false),
   mUpload(true)
{
   mAbstractTypesAND.setNumber(gDatabase.getNumberAbstractObjectTypes());
   mAbstractTypesOR.setNumber(gDatabase.getNumberAbstractObjectTypes());
   mAbstractTypesNOT.setNumber(gDatabase.getNumberAbstractObjectTypes());

   mAbstractTypesAND.clear();
   mAbstractTypesOR.clear();
   mAbstractTypesNOT.clear();
}

//==============================================================================
// 
//==============================================================================
BStatDefinition::~BStatDefinition()
{
   onRelease();
}

//==============================================================================
// 
//==============================================================================
void BStatDefinition::onRelease()
{
   if (mpKillers)
      BStatDefinition::releaseInstance(mpKillers);
   mpKillers = NULL;
}

//==============================================================================
// 
//==============================================================================
BStatRecorder::BStatRecorder() :
   mpDef(NULL)
{
}

//==============================================================================
// 
//==============================================================================
BStatRecorder::~BStatRecorder()
{
   onRelease();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorder::onRelease()
{
   // clean up any killers we may have been tracking
   uint count = mKillers.size();
   for (uint i=0; i < count; ++i)
   {
      BStatProtoIDHashMap* pKiller = mKillers[i];
      if (pKiller)
         delete pKiller;
   }
   mKillers.clear();

   count = mCombat.size();
   for (uint i=0; i < count; ++i)
   {
      BStatCombat* pCombat = mCombat[i];
      if (pCombat)
         delete pCombat;
   }
   mCombat.clear();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorder::init(BStatDefinition* pDef, BStatIDHashMap* pObjectIDMap, BStatIDHashMap* pSquadIDMap)
{
   mpDef = pDef;
   mpObjectIDMap = pObjectIDMap;
   mpSquadIDMap = pSquadIDMap;
}

//==============================================================================
// 
//==============================================================================
#ifndef BUILD_FINAL
uint32 BStatRecorder::getMemUsage()
{
   mMemUsage = sizeof(BStatRecorder);
   mMemUsage += mKillers.getSizeInBytes();
   mMemUsage += mCombat.getSizeInBytes();

   // loop through all killers
   uint numStats = mKillers.getSize();
   for (uint s=0; s < numStats; s++)
   {
//-- FIXING PREFIX BUG ID 3072
      const BStatProtoIDHashMap* pHash = mKillers[s];
//--
      if (pHash)
         mMemUsage += (sizeof(BStatProtoIDHashMap) + (pHash->getMaxEntries() * (sizeof(long) + sizeof(BStatLostDestroyed) + sizeof(int))));
   }

   numStats = mCombat.getSize();
   for (uint s=0; s < numStats; s++)
   {
//-- FIXING PREFIX BUG ID 3073
      const BStatCombat* pCombat = mCombat[s];
//--
      if (pCombat)
         mMemUsage += sizeof(BStatCombat) + (sizeof(long)*pCombat->mLevels.getSize());
   }

   return mMemUsage;
}
#endif

//==============================================================================
// If we've specified a specific ObjectType, this will filter for it
//==============================================================================
bool BStatRecorder::checkObjectType(BStatDefinition* pDef, const BProtoObject* pProtoObject) const
{
   const BBitArray& abstractTypes = pProtoObject->getAbstractTypes();
   const unsigned char* protoBits = abstractTypes.getBits();
   uint numBytes = abstractTypes.getNumberBytes();

   if (numBytes != static_cast<uint>(pDef->mAbstractTypesAND.getNumberBytes()))
      return false;

   const unsigned char* filterBits = NULL;

   if (pDef->mFilterObjectTypeNOT)
   {
      filterBits = pDef->mAbstractTypesNOT.getBits();
      for (uint i = 0; i < numBytes; ++i)
      {
         // if I have a filter and the proto object contains at least one matching bit, fail
         if (filterBits[i] > 0 && (filterBits[i] & protoBits[i]) > 0)
            return false;
      }
   }

   if (pDef->mFilterObjectTypeAND)
   {
      filterBits = pDef->mAbstractTypesAND.getBits();
      for (uint i = 0; i < numBytes; ++i)
      {
         // if I have a filter set and the proto bits does not match bit for bit, then fail
         if (filterBits[i] > 0 && (filterBits[i] & protoBits[i]) != filterBits[i])
            return false;
      }
   }

   if (pDef->mFilterObjectTypeOR)
   {
      filterBits = pDef->mAbstractTypesOR.getBits();
      for (uint i = 0; i < numBytes; ++i)
      {
         if (filterBits[i] > 0 && (filterBits[i] & protoBits[i]) > 0)
            return true;
      }

      // I need to match at least one bit in the OR's for the match to work
      return false;
   }

   return true;
}

//==============================================================================
// 
//==============================================================================
long BStatRecorder::getMappedProtoObjectID(long protoID)
{
   if (mpObjectIDMap == NULL)
      return protoID;

   BStatIDHashMap::iterator iter = mpObjectIDMap->find(protoID);
   if (iter != mpObjectIDMap->end())
   {
      return iter->second;
   }

   return protoID;
}

//==============================================================================
// 
//==============================================================================
long BStatRecorder::getMappedProtoSquadID(long protoID)
{
   if (mpSquadIDMap == NULL)
      return protoID;

   BStatIDHashMap::iterator iter = mpSquadIDMap->find(protoID);
   if (iter != mpSquadIDMap->end())
   {
      return iter->second;
   }

   return protoID;
}

//==============================================================================
// 
//==============================================================================
BStatRecorderTotal::BStatRecorderTotal() :
   mpCombat(NULL)
{
}

//==============================================================================
// 
//==============================================================================
BStatRecorderTotal::~BStatRecorderTotal()
{
   onRelease();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderTotal::onAcquire()
{
#ifndef BUILD_FINAL
   mMemDirty = true;
   mMemUsage = 0;
#endif

   mTotalsHash.clear();
   mTotal.reset();

   mpCombat = NULL;
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderTotal::onRelease()
{
   BStatRecorder::onRelease();

   mTotalsHash.clear();
   mTotal.reset();

   if (mpCombat != NULL)
   {
      delete mpCombat;
      mpCombat = NULL;
   }
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderTotal::init(BStatDefinition* pDef, BStatIDHashMap* pObjectIDMap, BStatIDHashMap* pSquadIDMap)
{
   BStatRecorder::init(pDef, pObjectIDMap, pSquadIDMap);

   mTotal.reset();
   mTotalsHash.clear();

   if (trackCombat())
   {
      // create a new BStatCombat and assign it to our pointer for fast lookups
      mpCombat = new BStatCombat();
   }
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderTotal::eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime)
{
   if (!mpDef->mTrackTimeZero && gameTime == 0)
      return;

   // first check against excluded techs
   if (mpDef->mFilterExcludeTechs && mpDef->mTechsExclude.find(pProtoTech->getID()) != cInvalidIndex)
      return;

   // then if we want to track all techs
   // then the explicit techs
   if (!mpDef->mTrackAllTechs && (mpDef->mTechs.find(pProtoTech->getID()) == cInvalidIndex))
      return;

   // [10/27/2008 xemu] filter out hidden techs
   if (pProtoTech->getHiddenFromStats())
      return;

   // update our totals?
   mTotal.mResearched += 1;

   if (mpDef->mTrackFirstLastGameTime)
   {
      if (mTotal.mFirstTime == 0)
         mTotal.mFirstTime = gameTime;
      mTotal.mLastTime = gameTime;
   }

   // check if we want to track each individual tech we've researched
   if (!mpDef->mTrackIndividual)
      return;

   BStatTotalHashMap::iterator it = mTotalsHash.find(pProtoTech->getID());
   if (it != mTotalsHash.end())
   {
      // found the protoID entry
      it->second.mResearched = it->second.mResearched + 1;

      if (mpDef->mTrackFirstLastGameTime)
      {
         if (it->second.mFirstTime == 0)
            it->second.mFirstTime = gameTime;
         it->second.mLastTime = gameTime;
      }
      return;
   }

   mTotalsHash.insert(pProtoTech->getID(), BStatTotal(1, 0, 0, gameTime));

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderTotal::eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime)
{
   if (!mpDef->mTrackTimeZero && gameTime == 0)
      return;

   // if we want to track squads and the proto squad is missing, skip event
   if (mpDef->mTrackSquads && !pProtoSquad)
      return;
   else if (pProtoSquad && !mpDef->mTrackSquads)
      return;

   // verify the ObjectClass, DBID and ObjectType
   if (mpDef->mFilterObjectClass && pProtoObject->getObjectClass() != mpDef->mObjectClass)
      return;
   if (mpDef->mFilterDBID && pProtoObject->getDBID() != mpDef->mDBID)
      return;
   if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pProtoObject))
      return;

   long protoID = cInvalidProtoID;
   if (mpDef->mTrackSquads)
      protoID = getMappedProtoSquadID(pProtoSquad->getID());
   else
      protoID = getMappedProtoObjectID(pProtoObject->getID());

   mTotal.mBuilt += 1;

   mTotal.mMax = Math::Max<uint32>(mTotal.mMax, mTotal.mBuilt - mTotal.mLost);

   if (mpDef->mTrackFirstLastGameTime)
   {
      if (mTotal.mFirstTime == 0)
         mTotal.mFirstTime = gameTime;
      mTotal.mLastTime = gameTime;
   }

   // check if we want to track each individual unit we've built
   if (!mpDef->mTrackIndividual)
      return;

   BStatTotalHashMap::iterator it = mTotalsHash.find(protoID);
   if (it != mTotalsHash.end())
   {
      // found the protoID entry
      it->second.mBuilt = it->second.mBuilt + 1;

      it->second.mMax = Math::Max<uint32>(it->second.mMax, it->second.mBuilt - it->second.mLost);

      if (mpDef->mTrackFirstLastGameTime)
      {
         if (it->second.mFirstTime == 0)
            it->second.mFirstTime = gameTime;
         it->second.mLastTime = gameTime;
      }
      return;
   }

   mTotalsHash.insert(protoID, BStatTotal(1, 0, 0, gameTime));

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderTotal::eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime)
{
   // if we want to track squads and the proto squad is missing, skip event
   if (mpDef->mTrackSquads && !pProtoSquad)
      return;

   // verify the ObjectClass, ObjectType and DBID
   if (mpDef->mFilterObjectClass && pProtoObject->getObjectClass() != mpDef->mObjectClass)
      return;
   if (mpDef->mFilterDBID && pProtoObject->getDBID() != mpDef->mDBID)
      return;
   if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pProtoObject))
      return;

   long protoID = cInvalidProtoID;
   if (mpDef->mTrackSquads)
      protoID = getMappedProtoSquadID(pProtoSquad->getID());
   else
      protoID = getMappedProtoObjectID(pProtoObject->getID());

   mTotal.mLost += 1;

   //if (mpDef->mTrackFirstLastGameTime)
   //{
   //   if (mTotal.mFirstTime == 0)
   //      mTotal.mFirstTime = gameTime;
   //   mTotal.mLastTime = gameTime;
   //}

   BStatTotal* pTotal = NULL;

   if (mpDef->mTrackIndividual)
   {
      BStatTotalHashMap::iterator it = mTotalsHash.find(protoID);
      if (it != mTotalsHash.end())
      {
         // found the protoID entry
         pTotal = &(it->second);
      }
      else
      {
         BStatTotalHashMap::InsertResult result = mTotalsHash.insert(protoID, BStatTotal());
         pTotal = &(result.first->second);

#ifndef BUILD_FINAL
         mMemDirty = true;
         gStatsManager.mMemDirty = true;
#endif
      }

      // increment the number of times I've lost the given pProtoObject
      pTotal->mLost += 1;

      //if (mpDef->mTrackFirstLastGameTime)
      //{
      //   if (pTotal->mFirstTime == 0)
      //      pTotal->mFirstTime = gameTime;
      //   pTotal->mLastTime = gameTime;
      //}
   }
   else
   {
      pTotal = &mTotal;
   }

   if (trackCombat() && mpDef->mTrackSquads && pProtoSquad)
   {
      eventCombat(*pTotal, xp, level);
   }

   // if we're tracking killers and we have a known killer...
   if (mpDef->mTrackKillers && pKillerProtoObject)
   {
      BStatDefinition* pDef = mpDef->mpKillers;

      if (pDef->mTrackSquads && !pKillerProtoSquad)
         return;

      // we can filter out killers by ObjectClass, DBID and ObjectType
      if (pDef->mFilterObjectClass && pKillerProtoObject->getObjectClass() != pDef->mObjectClass)
         return;
      if (pDef->mFilterDBID && pKillerProtoObject->getDBID() != pDef->mDBID)
         return;
      if (pDef->mFilterObjectType && !checkObjectType(pDef, pKillerProtoObject))
         return;

      long playerID = killerPlayerID;
      if (playerID < 0 || playerID >= BStats::cMaxPlayers)
         playerID = 0;

      BStatProtoIDHashMap* pKillers = NULL;
      if (pTotal->mKillerIDs[playerID] == -1)
      {
         pKillers = new BStatProtoIDHashMap();
         pTotal->mKillerIDs[playerID] = static_cast<int32>(mKillers.add(pKillers));

#ifndef BUILD_FINAL
         mMemDirty = true;
         gStatsManager.mMemDirty = true;
#endif
      }
      else
      {
         pKillers = mKillers[pTotal->mKillerIDs[playerID]];
      }

      long protoID = cInvalidProtoID;
      // try to track by squads if we can, it's not mandatory
      if (pDef->mTrackSquads)
         protoID = getMappedProtoSquadID(pKillerProtoSquad->getID());
      else
         protoID = getMappedProtoObjectID(pKillerProtoObject->getID());

      // throw the killers into the hash
      BStatProtoIDHashMap::iterator killerIter = pKillers->find(protoID);
      if (killerIter != pKillers->end())
      {
         // I lost X number of pProtoObjects to the given pKillerProtoObject
         killerIter->second.mLost = killerIter->second.mLost + 1;
      }
      else
      {
         pKillers->insert(protoID, BStatLostDestroyed(1, 0));

#ifndef BUILD_FINAL
         mMemDirty = true;
         gStatsManager.mMemDirty = true;
#endif
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderTotal::eventDestroyed(long playerID, BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, uint32 gameTime)
{
   // the thing that I killed is NOT what I'm interested in
   // what I'm tracking here is that I destroyed something with a given BProtoObject/BProtoSquad, that is the key

   // the BProtoObject/BProtoSquad that I killed could be anything
   // FIXME: check eventLost to insure that I'm tracking the building/turret/power that killed me

   // if we want to track squads, insure that we have a valid proto squad pointer
   // but only if we're not tracking the target
   if (mpDef->mTrackSquads && !mpDef->mEventDestroyedTarget && pKillerProtoSquad == NULL)
      return;

   // whether we track the killer or the target, we still need the target
   if (pKillerProtoObject == NULL)
      return;

   // verify the ObjectClass, ObjectType and DBID of the killer proto object
   BProtoObject* pTrackProtoObject = (mpDef->mEventDestroyedTarget ? pProtoObject : pKillerProtoObject);
   if (mpDef->mFilterObjectClass && pTrackProtoObject->getObjectClass() != mpDef->mObjectClass)
      return;
   if (mpDef->mFilterDBID && pTrackProtoObject->getDBID() != mpDef->mDBID)
      return;
   if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pTrackProtoObject))
      return;

   bool countIt = true;

   if (pProtoObject != NULL && pTrackProtoObject != pProtoObject)
   {
      if (mpDef->mFilterObjectClass && pProtoObject->getObjectClass() != mpDef->mObjectClass)
         countIt = false;
      if (mpDef->mFilterDBID && pProtoObject->getDBID() != mpDef->mDBID)
         countIt = false;
      if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pProtoObject))
         countIt = false;
   }

   if (countIt)
   {
      mTotal.mDestroyed += 1;

      //if (mpDef->mTrackFirstLastGameTime)
      //{
      //   if (mTotal.mFirstTime == 0)
      //      mTotal.mFirstTime = gameTime;
      //   mTotal.mLastTime = gameTime;
      //}
   }

   BStatTotal* pTotal = NULL;

   BProtoSquad* pTrackProtoSquad = (mpDef->mEventDestroyedTarget ? pProtoSquad : pKillerProtoSquad);

   if (mpDef->mTrackIndividual)
   {
      // need to look up the proto squad of the killer so I know what bucket to place us in
      long protoID = cInvalidProtoID;
      if (mpDef->mTrackSquads)
      {
         if (!pTrackProtoSquad)
            return;
      }
      else if (!pTrackProtoObject)
         return;

      if (mpDef->mTrackSquads)
         protoID = getMappedProtoSquadID(pTrackProtoSquad->getID());
      else
         protoID = getMappedProtoObjectID(pTrackProtoObject->getID());

      BStatTotalHashMap::iterator it = mTotalsHash.find(protoID);
      if (it != mTotalsHash.end())
      {
         // found the protoID entry
         pTotal = &(it->second);
      }
      else
      {
         BStatTotalHashMap::InsertResult result = mTotalsHash.insert(protoID, BStatTotal());
         pTotal = &(result.first->second);

#ifndef BUILD_FINAL
         mMemDirty = true;
         gStatsManager.mMemDirty = true;
#endif
      }

      // increment the number of times I've destroyed something with this killer proto object/squad
      if (countIt)
      {
         pTotal->mDestroyed += 1;

         //if (mpDef->mTrackFirstLastGameTime)
         //{
         //   if (pTotal->mFirstTime == 0)
         //      pTotal->mFirstTime = gameTime;
         //   pTotal->mLastTime = gameTime;
         //}
      }
   }
   else
   {
      pTotal = &mTotal;
   }

   if (mpDef->mTrackKillers && pProtoObject)
   {
      BStatDefinition* pDef = mpDef->mpKillers;

      // if we want to track squads and the proto squad is missing, skip event
      if (pDef->mTrackSquads && !pProtoSquad)
         return;

      if (pDef->mFilterObjectClass && pProtoObject->getObjectClass() != pDef->mObjectClass)
         return;
      if (pDef->mFilterDBID && pProtoObject->getDBID() != pDef->mDBID)
         return;
      if (pDef->mFilterObjectType && !checkObjectType(pDef, pProtoObject))
         return;

      if (playerID < 0 || playerID >= BStats::cMaxPlayers)
         playerID = 0;

      BStatProtoIDHashMap* pKillers = NULL;
      if (pTotal->mKillerIDs[playerID] == -1)
      {
         pKillers = new BStatProtoIDHashMap();
         pTotal->mKillerIDs[playerID] = static_cast<int32>(mKillers.add(pKillers));

#ifndef BUILD_FINAL
         mMemDirty = true;
         gStatsManager.mMemDirty = true;
#endif
      }
      else
      {
         pKillers = mKillers[pTotal->mKillerIDs[playerID]];
      }

      long protoID = cInvalidProtoID;
      if (pDef->mTrackSquads)
         protoID = getMappedProtoSquadID(pProtoSquad->getID());
      else
         protoID = getMappedProtoObjectID(pProtoObject->getID());

      // throw the killers into the hash
      BStatProtoIDHashMap::iterator killerIter = pKillers->find(protoID);
      if (killerIter != pKillers->end())
      {
         // I've destroyed pProtoObject with pKillerProtoObject
         killerIter->second.mDestroyed = killerIter->second.mDestroyed + 1;
      }
      else
      {
         pKillers->insert(protoID, BStatLostDestroyed(0, 1));

#ifndef BUILD_FINAL
         mMemDirty = true;
         gStatsManager.mMemDirty = true;
#endif
      }
   }
}

//==============================================================================
// Called when I lose a squad, need to add in the squad's xp
//==============================================================================
void BStatRecorderTotal::eventCombat(BStatTotal& total, float xp, long level)
{
   if (mpCombat == NULL)
      return;

   mpCombat->mXP += xp;

   if (mpCombat->mLevels.getNumber() <= level)
      mpCombat->mLevels.setNumber(level+1);

   mpCombat->mLevels[level] += 1;

   BStatCombat* pCombat = NULL;

   if (total.mCombatID == -1)
   {
      pCombat = new BStatCombat();
      total.mCombatID = static_cast<int32>(mCombat.add(pCombat));

#ifndef BUILD_FINAL
      mMemDirty = true;
      gStatsManager.mMemDirty = true;
#endif
   }
   else
   {
      pCombat = mCombat[total.mCombatID];
   }

   if(pCombat)
   {
      // 
      pCombat->mXP += xp;

      if (pCombat->mLevels.getNumber() <= level)
         pCombat->mLevels.setNumber(level+1);

      pCombat->mLevels[level] += 1;
   }
}

//==============================================================================
// XXX FIXME: DPM 3/11/2008 - if you don't win all your squads are killed,
//    but if you do, you need to accumulate all the remaining xp/levels
//==============================================================================
void BStatRecorderTotal::updateFinalStats(BPlayerID playerID)
{
   // if we're tracking combat stats and tracking squads
   // we need to iterate over all the remaining squads for
   // this player and add their xp and levels to our tracker
   if (trackCombat() && mpDef->mTrackSquads)
   {
      BEntityHandle handle = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3076
      const BSquad* pSquad = gWorld->getNextSquad(handle);
//--
      while (pSquad)
      {
         if (pSquad->getFlagAlive() && pSquad->getPlayerID() == playerID)
         {
            BStatTotalHashMap::iterator it = mTotalsHash.find(pSquad->getProtoSquadID());
            if (it != mTotalsHash.end())
            {
               BStatTotal& total = it->second;

               eventCombat(total, pSquad->getXP(), pSquad->getLevel());
            }
         }
         pSquad = gWorld->getNextSquad(handle);
      }
   }
}

//==============================================================================
// If you need this, see Doug
//==============================================================================
//bool BStatRecorderTotal::collectAIStats(BPlayer* pPlayer, BStatTotalHashMap& rolledupSquadTotals, BStatTotalHashMap& rolledupTechTotals, BStatTotalHashMap& rolledupObjectTotals) const
//{
//   for (BStatTotalHashMap::const_iterator it = mTotalsHash.begin(); it != mTotalsHash.end(); ++it)
//   {
//      long protoID = it->first; // or techid
//      const BStatTotal& total = it->second;
//      if (mpDef->mEventResearched)
//      {
//         //BProtoTech* pProtoTech = (pPlayer ? pPlayer->getProtoTech(protoID) : gDatabase.getProtoTech(protoID));
//         BProtoTech* pProtoTech = gDatabase.getProtoTech(protoID);
//         if (pProtoTech)
//            rolledupTechTotals.insert(protoID, total);
//      }
//      else if (mpDef->mTrackSquads)
//      {
//         //const BProtoSquad* pProtoSquad = (pPlayer ? pPlayer->getProtoSquad(protoID) : gDatabase.getGenericProtoSquad(protoID));
//         const BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoID);
//         if (pProtoSquad)
//            rolledupSquadTotals.insert(protoID, total);
//      }
//      else
//      {
//         //const BProtoObject* pProtoObject = (pPlayer ? pPlayer->getProtoObject(protoID) : gDatabase.getGenericProtoObject(protoID));
//         const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
//         if (pProtoObject)
//            rolledupObjectTotals.insert(protoID, total);
//      }
//   }
//
//   return(true);
//}

//==============================================================================
// 
//==============================================================================
bool BStatRecorderTotal::write(BPlayer* pPlayer, BDynamicStream& stream) const
{
   if (!mpDef->uploadStat())
      return true;

   stream.printf("<total id='%u'>", mpDef->mID);

   // built/lost/destroyed military units/squads/buildings
   stream.printf("((%u,%u,%u,%u,%u,%u),", mTotal.mBuilt, mTotal.mLost, mTotal.mDestroyed, mTotal.mMax, mTotal.mFirstTime, mTotal.mLastTime);

   // this particular mKillersID is never used
   /*
   if (mTotal.mKillersID != -1)
   {
      BStatProtoIDHashMap* pKillers = mKillers[mTotal.mKillersID];

      for (BStatProtoIDHashMap::const_iterator killerIt = pKillers->begin(); killerIt != pKillers->end(); ++killerIt)
      {
         const char* name = NULL;

//-- FIXING PREFIX BUG ID 3080
         const BProtoObject* pProtoObject = (pPlayer ? pPlayer->getProtoObject(killerIt->first) : gDatabase.getProtoObject(killerIt->first));
//--
         if (pProtoObject)
            name = pProtoObject->getName().asNative();

         if (name)
            stream.printf("('%s',%u,%u),", name, killerIt->second.mLost, killerIt->second.mDestroyed);
         else
            stream.printf("(%i,%u,%u),", killerIt->first, killerIt->second.mLost, killerIt->second.mDestroyed);
      }
   }
   */

   if (mpCombat != NULL)
   {
      stream.printf("(%0.4f,(", mpCombat->mXP);
      for (uint i=0; i < mpCombat->mLevels.getSize(); ++i)
      {
         stream.printf("%d,", mpCombat->mLevels[i]);
      }
      stream.printf(")),");
   }

   for (BStatTotalHashMap::const_iterator it = mTotalsHash.begin(); it != mTotalsHash.end(); ++it)
   {
      long protoID = it->first; // or techid
      const BStatTotal& total = it->second;

      const char* name = NULL;

      if (mpDef->mEventResearched)
      {
//-- FIXING PREFIX BUG ID 3078
         //const BProtoTech* pProtoTech = (pPlayer ? pPlayer->getProtoTech(protoID) : gDatabase.getProtoTech(protoID));
//--
         BProtoTech* pProtoTech = gDatabase.getProtoTech(protoID);
         if (pProtoTech)
            name = pProtoTech->getName().asNative();
      }
      else if (mpDef->mTrackSquads)
      {
//-- FIXING PREFIX BUG ID 3079
         //const BProtoSquad* pProtoSquad = (pPlayer ? pPlayer->getProtoSquad(protoID) : gDatabase.getGenericProtoSquad(protoID));
//--
//-- FIXING PREFIX BUG ID 3081
         const BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoID);
//--
         if (pProtoSquad)
            name = pProtoSquad->getName().asNative();
      }
      else
      {
//-- FIXING PREFIX BUG ID 3082
         //const BProtoObject* pProtoObject = (pPlayer ? pPlayer->getProtoObject(protoID) : gDatabase.getGenericProtoObject(protoID));
//--
         BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
         if (pProtoObject)
            name = pProtoObject->getName().asNative();
      }

      if (name)
         stream.printf("('%s',%u,%u,%u,%u,%u,%u,", name, total.mBuilt, total.mLost, total.mDestroyed, total.mMax, total.mFirstTime, total.mLastTime);
      else
         stream.printf("(%i,%u,%u,%u,%u,%u,%u,", protoID, total.mBuilt, total.mLost, total.mDestroyed, total.mMax, total.mFirstTime, total.mLastTime);

      // if we have combat information, we need to dump that now
      if (total.mCombatID != -1)
      {
         BStatCombat* pCombat = mCombat[total.mCombatID];

         stream.printf("(%0.4f,(", pCombat->mXP);
         for (uint i=0; i < pCombat->mLevels.getSize(); ++i)
         {
            stream.printf("%d,", pCombat->mLevels[i]);
         }
         stream.printf(")),");
      }

      for (uint i=0; i < BStats::cMaxPlayers; ++i)
      {
         if (total.mKillerIDs[i] == -1)
            continue;

         //const BPlayer* pKillerPlayer = gWorld->getPlayer(i);

         // output the player ID for this set of killers
         // (PlayerID, ('unit', lost, destroyed), ...), ...
         stream.printf("(%i,", i);

//-- FIXING PREFIX BUG ID 3083
         const BStatDefinition* pDef = mpDef->mpKillers;
//--

         BStatProtoIDHashMap* pKillers = mKillers[total.mKillerIDs[i]];

         for (BStatProtoIDHashMap::const_iterator killerIt = pKillers->begin(); killerIt != pKillers->end(); ++killerIt)
         {
            const char* name = NULL;

            if (pDef->mTrackSquads)
            {
               //BProtoSquad* pProtoSquad = (pKillerPlayer ? pKillerPlayer->getProtoSquad(killerIt->first) : gDatabase.getGenericProtoSquad(killerIt->first));
               BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(killerIt->first);
               if (pProtoSquad)
                  name = pProtoSquad->getName().asNative();
            }
            else
            {
               //BProtoObject* pProtoObject = (pKillerPlayer ? pKillerPlayer->getProtoObject(killerIt->first) : gDatabase.getGenericProtoObject(killerIt->first));
               BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(killerIt->first);
               if (pProtoObject)
                  name = pProtoObject->getName().asNative();
            }

            if (name != NULL)
               stream.printf("('%s',%u,%u),", name, killerIt->second.mLost, killerIt->second.mDestroyed);
            else
               stream.printf("(%i,%u,%u),", killerIt->first, killerIt->second.mLost, killerIt->second.mDestroyed);
         }

         stream.printf("),");
      }

      stream.printf("),");
   }

   stream.printf(")</total>");

   return true;
}

//==============================================================================
// 
//==============================================================================
long BStatRecorderTotal::getLargestTotal(BStatTotal& total)
{
   long largestProtoID = -1; 
//-- FIXING PREFIX BUG ID 3085
   const BStatTotal* pLargest = NULL;
//--

   for (BStatTotalHashMap::iterator it = mTotalsHash.begin(); it != mTotalsHash.end(); ++it)
   {
      long protoID = it->first;        // protoID
      BStatTotal& t = it->second;  // statTotal

      if ((pLargest == NULL) || (pLargest->mBuilt < t.mBuilt))
      {
         largestProtoID = protoID;
         pLargest = &t;        // value
      }
   }

   if (pLargest)
      total = *pLargest;

   return largestProtoID;
}

//==============================================================================
// 
//==============================================================================
#ifndef BUILD_FINAL
uint32 BStatRecorderTotal::getMemUsage()
{
   if (!mMemDirty)
      return mMemUsage;

   mMemUsage = BStatRecorder::getMemUsage();
   mMemUsage += sizeof(BStatRecorderTotal);
   mMemUsage += (mTotalsHash.getMaxEntries() * (sizeof(long) + sizeof(BStatTotal) + sizeof(int)));
   if (mpCombat != NULL)
      mMemUsage += sizeof(BStatCombat) + (sizeof(long)*mpCombat->mLevels.getSize());

   mMemDirty = false;

   return mMemUsage;
}
#endif

//==============================================================================
// 
//==============================================================================
BStatRecorderEvent::BStatRecorderEvent()
{
}

//==============================================================================
// 
//==============================================================================
BStatRecorderEvent::~BStatRecorderEvent()
{
   onRelease();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderEvent::onAcquire()
{
#ifndef BUILD_FINAL
   mMemDirty = true;
   mMemUsage = 0;
#endif

   mEvents.clear();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderEvent::onRelease()
{
   BStatRecorder::onRelease();

   mTotal.reset();
   mEvents.clear();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderEvent::init(BStatDefinition* pDef, BStatIDHashMap* pObjectIDMap, BStatIDHashMap* pSquadIDMap)
{
   BStatRecorder::init(pDef, pObjectIDMap, pSquadIDMap);

   mTotal.reset();

   mEvents.clear();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderEvent::eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime)
{
   if (!mpDef->mTrackTimeZero && gameTime == 0)
      return;

   // first check against excluded techs
   // then if we want to track all techs
   // then the explicit techs
   if (mpDef->mFilterExcludeTechs && mpDef->mTechsExclude.find(pProtoTech->getID()) != cInvalidIndex)
      return;

   if (!mpDef->mTrackAllTechs && (mpDef->mTechs.find(pProtoTech->getID()) == cInvalidIndex))
      return;

   // [10/27/2008 xemu] filter out hidden techs
   if (pProtoTech->getHiddenFromStats())
      return;

   mTotal.mResearched += 1;

   // add a new event
   BStatEvent statEvent;

   statEvent.mProtoID = pProtoTech->getID();
   statEvent.mResearched = 1;
   statEvent.mTimestamp = gameTime;

   mEvents.add(statEvent);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderEvent::eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime)
{
   if (!mpDef->mTrackTimeZero && gameTime == 0)
      return;

   // if we want to track squads and the proto squad is missing, skip event
   if (mpDef->mTrackSquads && !pProtoSquad)
      return;
   else if (pProtoSquad && !mpDef->mTrackSquads)
      return;

   // verify the ObjectClass, DBID and ObjectType
   if (mpDef->mFilterObjectClass && pProtoObject->getObjectClass() != mpDef->mObjectClass)
      return;
   if (mpDef->mFilterDBID && pProtoObject->getDBID() != mpDef->mDBID)
      return;
   if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pProtoObject))
      return;

   long protoID = cInvalidProtoID;
   if (mpDef->mTrackSquads)
      protoID = getMappedProtoSquadID(pProtoSquad->getID());
   else
      protoID = getMappedProtoObjectID(pProtoObject->getID());

   mTotal.mBuilt += 1;

   // add a new event
   BStatEvent statEvent;

   statEvent.mProtoID = protoID;
   statEvent.mBuilt = 1;
   statEvent.mTimestamp = gameTime;

   mEvents.add(statEvent);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderEvent::eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime)
{
   // if we want to track squads and the proto squad is missing, skip event
   if (mpDef->mTrackSquads && !pProtoSquad)
      return;

   // verify the ObjectClass, ObjectType and DBID
   if (mpDef->mFilterObjectClass && pProtoObject->getObjectClass() != mpDef->mObjectClass)
      return;
   if (mpDef->mFilterDBID && pProtoObject->getDBID() != mpDef->mDBID)
      return;
   if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pProtoObject))
      return;

   long protoID = cInvalidProtoID;
   if (mpDef->mTrackSquads)
      protoID = getMappedProtoSquadID(pProtoSquad->getID());
   else
      protoID = getMappedProtoObjectID(pProtoObject->getID());

   mTotal.mLost += 1;

   // add a new event
   BStatEvent statEvent;

   statEvent.mProtoID = protoID;
   statEvent.mLost = 1;
   statEvent.mTimestamp = gameTime;

   mEvents.add(statEvent);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
bool BStatRecorderEvent::write(BPlayer* pPlayer, BDynamicStream& stream) const
{
   if (!mpDef->uploadStat())
      return true;

   // researched techs
   uint count = mEvents.getSize();

   stream.printf("<event id='%u'>((%u,%u,%u,%u)", mpDef->mID, count, mTotal.mBuilt, mTotal.mLost, mTotal.mDestroyed);

   for (uint i=0; i < count; ++i)
   {
      const BStatEvent& statEvent = mEvents[i];
      const char* name = NULL;

      if (mpDef->mEventResearched)
      {
//-- FIXING PREFIX BUG ID 3086
         //const BProtoTech* pProtoTech = (pPlayer ? pPlayer->getProtoTech(statEvent.mProtoID) : gDatabase.getProtoTech(statEvent.mProtoID));
//--
         BProtoTech* pProtoTech = gDatabase.getProtoTech(statEvent.mProtoID);
         if (pProtoTech)
            name = pProtoTech->getName().asNative();
      }
      else
      {
//-- FIXING PREFIX BUG ID 3087
         //const BProtoObject* pProtoObject = (pPlayer ? pPlayer->getProtoObject(statEvent.mProtoID) : gDatabase.getGenericProtoObject(statEvent.mProtoID));
//--
         BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(statEvent.mProtoID);
         if (pProtoObject)
            name = pProtoObject->getName().asNative();
      }

      if (name)
         stream.printf(",(%u,'%s',%u,%u,%u)", statEvent.mTimestamp, name, statEvent.mBuilt, statEvent.mLost, statEvent.mDestroyed);
      else
         stream.printf(",(%u,%u,%u,%u,%u)", statEvent.mTimestamp, statEvent.mProtoID, statEvent.mBuilt, statEvent.mLost, statEvent.mDestroyed);
   }

   stream.printf(")</event>");

   return true;
}

//==============================================================================
// 
//==============================================================================
const BStatRecorder::BStatTotal& BStatRecorderEvent::getStatTotal() const
{
   return mTotal;
}

//==============================================================================
// 
//==============================================================================
const BSmallDynamicSimArray<BStatRecorder::BStatEvent>& BStatRecorderEvent::getEvents() const
{
   return mEvents;
}

//==============================================================================
// 
//==============================================================================
#ifndef BUILD_FINAL
uint32 BStatRecorderEvent::getMemUsage()
{
   if (!mMemDirty)
      return mMemUsage;

   mMemUsage = BStatRecorder::getMemUsage();
   mMemUsage += sizeof(BStatRecorderEvent);
   mMemUsage += mEvents.getSizeInBytes();

   mMemDirty = false;

   return mMemUsage;
}
#endif

//==============================================================================
// 
//==============================================================================
BStatRecorderGraph::BStatRecorderGraph() :
   mLastUpdateTime(0),
   mSize(0),
   mInterval(5000),
   mIndex(0),
   mStep(1)
{
}

//==============================================================================
// 
//==============================================================================
BStatRecorderGraph::~BStatRecorderGraph()
{
   onRelease();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::onAcquire()
{
#ifndef BUILD_FINAL
   mMemDirty = true;
   mMemUsage = 0;
#endif

   mCurrentEvent.mBuilt = 0;
   mCurrentEvent.mLost = 0;
   mCurrentEvent.mDestroyed = 0;

   mSize = 0;
   mStep = 1;
   mInterval = 5000;
   mLastUpdateTime = 0;
   mIndex = 0;

   mGraph.clear();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::onRelease()
{
   BStatRecorder::onRelease();

   mGraph.clear();
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::init(BStatDefinition* pDef, BStatIDHashMap* pObjectIDMap, BStatIDHashMap* pSquadIDMap)
{
   BStatRecorder::init(pDef, pObjectIDMap, pSquadIDMap);

   mSize = 0;
   mStep = 1;
   mLastUpdateTime = 0;
   mInterval = pDef->mInterval;
   mIndex = 0;

   mGraph.clear();
   mGraph.resize(pDef->mSamples);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::update(DWORD timeNow)
{
   if (mpDef->mSamples == 0)
      return;

   // update our graphing data based on the new time value
   // check if the appropriate amount of time has passed based on our interval
   // verify that we're still within the bounds of our samples
   if (timeNow - mLastUpdateTime >= mInterval)
   {
      mLastUpdateTime = timeNow;

      mCurrentEvent.mTimestamp = timeNow;

      mGraph[mIndex] = mCurrentEvent;

      int16 prevIndex = mIndex;

      mIndex = mIndex + mStep;

      mSize = min(mSize+1, mpDef->mSamples);

      if (mIndex < 0 || mIndex >= mpDef->mSamples)
      {
         // step 2 for every other slot
         mStep = 2;

         // reverse directions
         if (mIndex >= mpDef->mSamples)
            mStep = -mStep;

         // bump our recording interval out some to slow down the sample rate
         mInterval = mInterval + mpDef->mInterval;

         // we ping-pong between the ends of our sample array
         // our step size will always remain 2.
         // this should degrade resolution at the upper end while
         // trying to preserve the resolution of the oldest data.
         if (prevIndex == (mpDef->mSamples - 1))
            mIndex = prevIndex - 1;
         else if (prevIndex == 0)
            mIndex = 1;
         else if (mIndex < 0)
            mIndex = 0;
         else
            mIndex = mpDef->mSamples - 1;
      }

      // do not reset the numbers, make it cumulative
      //mCurrentEvent.mBuilt = 0;
      //mCurrentEvent.mLost = 0;
      //mCurrentEvent.mDestroyed = 0;
   }
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime)
{
   if (!mpDef->mTrackTimeZero && gameTime == 0)
      return;

   // first check against excluded techs
   // then if we want to track all techs
   // then the explicit techs
   if (mpDef->mFilterExcludeTechs && mpDef->mTechsExclude.find(pProtoTech->getID()) != cInvalidIndex)
      return;

   if (!mpDef->mTrackAllTechs && (mpDef->mTechs.find(pProtoTech->getID()) == cInvalidIndex))
      return;

   // [10/27/2008 xemu] filter out hidden techs
   if (pProtoTech->getHiddenFromStats())
      return;

   mCurrentEvent.mResearched += 1;
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime)
{
   if (!mpDef->mTrackTimeZero && gameTime == 0)
      return;

   if (mpDef->mTrackSquads && !pProtoSquad)
      return;
   else if (pProtoSquad && !mpDef->mTrackSquads)
      return;

   // verify the ObjectClass, DBID and ObjectType
   if (mpDef->mFilterObjectClass && pProtoObject->getObjectClass() != mpDef->mObjectClass)
      return;
   if (mpDef->mFilterDBID && pProtoObject->getDBID() != mpDef->mDBID)
      return;
   if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pProtoObject))
      return;

   mCurrentEvent.mBuilt += 1;
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime)
{
   if (mpDef->mTrackSquads && !pProtoSquad)
      return;

   // verify the ObjectClass, ObjectType and DBID
   if (mpDef->mFilterObjectClass && pProtoObject->getObjectClass() != mpDef->mObjectClass)
      return;
   if (mpDef->mFilterDBID && pProtoObject->getDBID() != mpDef->mDBID)
      return;
   if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pProtoObject))
      return;

   mCurrentEvent.mLost += 1;
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::eventDestroyed(long playerID, BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, uint32 gameTime)
{
   if (mpDef->mTrackSquads && !pProtoSquad)
      return;

   // verify the ObjectClass, ObjectType and DBID
   if (mpDef->mFilterObjectClass && pProtoObject->getObjectClass() != mpDef->mObjectClass)
      return;
   if (mpDef->mFilterDBID && pProtoObject->getDBID() != mpDef->mDBID)
      return;
   if (mpDef->mFilterObjectType && !checkObjectType(mpDef, pProtoObject))
      return;

   mCurrentEvent.mDestroyed += 1;
}

//==============================================================================
// 
//==============================================================================
void BStatRecorderGraph::query(uint32& minX, uint32& maxX, uint32& minY, uint32& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEvent>& points)
{
   points = mGraph;

   points.setNumber(mSize);

   minX = 0;
   maxX = 0;

   minY = 0;
   maxY = 0;

   points.sort(BStatEventCompareFunc);

   if (mSize == 0)
      return;

   const BStatRecorder::BStatEvent& begin = points[0];
   const BStatRecorder::BStatEvent& end = points[mSize-1];

   minX = begin.mTimestamp;
   maxX = end.mTimestamp;

   for (uint i = 0; i < mSize; ++i)
   {
      const BStatRecorder::BStatEvent& point = points[i];

      if (point.mBuilt >= point.mLost)
      {
         uint32 y = point.mBuilt - point.mLost;
         minY = min(minY, y);
         maxX = max(maxY, y);
      }
      else
         minY = 0;
   }
}

//==============================================================================
// 
//==============================================================================
bool BStatRecorderGraph::write(BPlayer* pPlayer, BDynamicStream& stream) const
{
   if (!mpDef->uploadStat())
      return true;

   stream.printf("<graph id='%u'>(", mpDef->mID);

   for (uint i = 0; i < mSize; ++i)
   {
      const BStatEvent& e = mGraph[i];

      stream.printf("(%u,%u,%u,%u),", e.mTimestamp, e.mBuilt, e.mLost, e.mDestroyed);
   }

   stream.printf(")</graph>");

   return true;
}

//==============================================================================
// 
//==============================================================================
#ifndef BUILD_FINAL
uint32 BStatRecorderGraph::getMemUsage()
{
   if (!mMemDirty)
      return mMemUsage;

   mMemUsage = BStatRecorder::getMemUsage();
   mMemUsage += sizeof(BStatRecorderGraph);
   mMemUsage += mGraph.getSizeInBytes();

   mMemDirty = false;

   return mMemUsage;
}
#endif

//==============================================================================
// 
//==============================================================================
BStatsPlayer::BStatsPlayer() :
   mXuid(0),
   mpPlayer(NULL),
   mPlayerID(-1),
   mTeamID(-1),
   mPlayerStateTime(0),
   mPopID(-1),
   mPlayerState(0),
   mResourcesUsed(0),
   mDifficultyType(0),
   mDifficulty(0.0f),
   mStrengthTime(0),
   mStrengthTimer(0),
   mCivID(-1),
   mLeaderID(-1),
   mPlayerType(BPlayer::cPlayerTypeNPC),
   mRandomCiv(false),
   mRandomLeader(false),
   mResigned(false),
   mDefeated(false),
   mDisconnected(false),
   mWon(false)
{
}

//==============================================================================
// 
//==============================================================================
BStatsPlayer::~BStatsPlayer()
{
   onRelease();
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::onAcquire()
{
#ifndef BUILD_FINAL
   mMemDirty = true;
   mMemUsage = 0;
   gStatsManager.mMemDirty = true;
#endif

   // initialize all the variables
   mXuid = 0;
   mpPlayer = NULL;
   mPlayerID = -1;
   mTeamID = -1;
   mPlayerStateTime = 0;
   mResourcesUsed = 0;
   mPlayerState = 0;
   mDifficultyType = 0;
   mDifficulty = 0.0f;
   mStrengthTime = 0;
   mStrengthTimer = 0;
   mCivID = -1;
   mLeaderID = -1;
   mPlayerType = BPlayer::cPlayerTypeNPC;
   mRandomCiv = false;
   mRandomLeader = false;
   mResigned = false;
   mDefeated = false;
   mDisconnected = false;
   mWon = false;

   mResGraphDef.reset();
   mPopGraphDef.reset();
   mBaseGraphDef.reset();
   mScoreGraphDef.reset();

   mTotalResources.setAll(0.0f);
   mMaxResources.setAll(0.0f);
   mGatheredResources.setAll(0.0f);
   mTributedResources.setAll(0.0f);

   mPopID = gDatabase.getPop("Unit");
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::onRelease()
{
   // free any resources
   uint count = mStatRecorders.getSize();
   for (uint i=0; i < count; ++i)
   {
      BStatRecorder* pRecorder = mStatRecorders[i];
      if (!pRecorder)
         continue;

      if (pRecorder->getType() == eTotal)
         BStatRecorderTotal::releaseInstance(reinterpret_cast<BStatRecorderTotal*>(pRecorder));
      else if (pRecorder->getType() == eEvent)
         BStatRecorderEvent::releaseInstance(reinterpret_cast<BStatRecorderEvent*>(pRecorder));
      else if (pRecorder->getType() == eGraph)
         BStatRecorderGraph::releaseInstance(reinterpret_cast<BStatRecorderGraph*>(pRecorder));
   }

   mStatRecorders.clear();
   mResourceGraph.clear();
   mPopGraph.clear();
   mBaseGraph.clear();
   mScoreGraph.clear();
   mPowersUsed.clear();
   mAbilitiesUsed.clear();
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::init(BPlayer* pPlayer)
{
   mpPlayer = pPlayer;
   mPlayerID = mpPlayer->getID();

   mCivID = mpPlayer->getCivID();
   mLeaderID = mpPlayer->getLeaderID();

   mPlayerType = mpPlayer->getPlayerType();

   mPowersUsed.clear();
   mAbilitiesUsed.clear();

   mTotalResources = mpPlayer->getTotalResources();
   mMaxResources = mpPlayer->getTotalResources();

//-- FIXING PREFIX BUG ID 3057
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   BDEBUG_ASSERT(pSettings);

   XUID xuid=0;
   if (pSettings->getUInt64(PSINDEX(mpPlayer->getMPID(), BGameSettings::cPlayerXUID), xuid))
      mXuid = xuid;

   pSettings->getFloat(PSINDEX(mpPlayer->getMPID(), BGameSettings::cPlayerDifficulty), mDifficulty);
   pSettings->getLong(PSINDEX(mpPlayer->getMPID(), BGameSettings::cPlayerDifficultyType), mDifficultyType);
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::initResourceGraph(BStatDefinition* pDef)
{
   mResourcesUsed = 0;
   mResGraphDef.mStep = 1;
   mResGraphDef.mLastUpdateTime = 0;
   mResGraphDef.mIndex = 0;
   mResGraphDef.mSize = 0;
   mResGraphDef.mSamples = pDef->mSamples;
   mResGraphDef.mInterval = mResGraphDef.mOriginalInterval = pDef->mInterval;

   mResourceGraph.clear();
   mResourceGraph.resize(mResGraphDef.mSamples);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::initScoreGraph(BStatDefinition* pDef)
{
   mScoreGraphDef.mStep = 1;
   mScoreGraphDef.mLastUpdateTime = 0;
   mScoreGraphDef.mIndex = 0;
   mScoreGraphDef.mSize = 0;
   mScoreGraphDef.mSamples = pDef->mSamples;
   mScoreGraphDef.mInterval = mScoreGraphDef.mOriginalInterval = pDef->mInterval;

   mScoreGraph.clear();
   mScoreGraph.resize(mScoreGraphDef.mSamples);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::initBaseGraph(BStatDefinition* pDef)
{
   mBaseGraphDef.mStep = 1;
   mBaseGraphDef.mLastUpdateTime = 0;
   mBaseGraphDef.mIndex = 0;
   mBaseGraphDef.mSize = 0;
   mBaseGraphDef.mSamples = pDef->mSamples;
   mBaseGraphDef.mInterval = mBaseGraphDef.mOriginalInterval = pDef->mInterval;
   mBaseGraphDef.mUpload = pDef->mUpload;

   mBaseGraph.clear();
   mBaseGraph.resize(mBaseGraphDef.mSamples);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::initPopGraph(BStatDefinition* pDef)
{
   mPopGraphDef.mStep = 1;
   mPopGraphDef.mLastUpdateTime = 0;
   mPopGraphDef.mIndex = 0;
   mPopGraphDef.mSize = 0;
   mPopGraphDef.mSamples = pDef->mSamples;
   mPopGraphDef.mInterval = mPopGraphDef.mOriginalInterval = pDef->mInterval;

   mPopGraph.clear();
   mPopGraph.resize(mPopGraphDef.mSamples);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::addResource(long resourceID, float amount, uint flags)
{
   if (mPlayerID == 0)
      return;

   mTotalResources.add(resourceID, amount);

   float current = mTotalResources.get(resourceID);
   float max = mMaxResources.get(resourceID);
   if (current > max)
      mMaxResources.set(resourceID, current);

   if (flags & BPlayer::cFlagFromGather)
      mGatheredResources.add(resourceID, amount);
   if (flags & BPlayer::cFlagFromTribute)
      mTributedResources.add(resourceID, amount);
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::addLeaderPower(BProtoPowerID protoPowerID)
{
   if (mPlayerID == 0)
      return;

   BStatProtoPowerIDHash::iterator it = mPowersUsed.find(protoPowerID);
   if (it == mPowersUsed.end())
   {
      mPowersUsed.insert(protoPowerID, 0);

#ifndef BUILD_FINAL
      mMemDirty = true;
      gStatsManager.mMemDirty = true;
#endif
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::usedLeaderPower(BProtoPowerID protoPowerID)
{
   if (mPlayerID == 0)
      return;

   // update our used powers list
   BStatProtoPowerIDHash::iterator it = mPowersUsed.find(protoPowerID);
   if (it != mPowersUsed.end())
   {
      // found the protoID entry
      it->second = it->second + 1;
      return;
   }

   mPowersUsed.insert(protoPowerID, 1);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::stateChanged()
{
   if (mPlayerState == mpPlayer->getPlayerState())
      return;

   if (mpPlayer)
   {
      switch (mpPlayer->getPlayerState())
      {
         case BPlayer::cPlayerStateResigned:
            mResigned = true;
            break;
         case BPlayer::cPlayerStateDefeated:
            mDefeated = true;
            break;
         case BPlayer::cPlayerStateDisconnected:
            mDisconnected = true;
            break;
         case BPlayer::cPlayerStateWon:
            mWon = true;
            break;
         default:
            return;
      }

      mPlayerState = mpPlayer->getPlayerState();
      mPlayerStateTime = gWorld->getGametime();
   }

   if (mpPlayer->getNetState() == BPlayer::cPlayerNetStateDisconnected)
      mDisconnected = true;

   // FIXME: DPM 3/11/2008
   // if you don't win, then don't call updateFinalStats
   //if (!mWon)
   //   return;

   // the player has either been defeated, resigned or won
   // need to update the xp/levels for the player's remaining squads
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder)
      {
         pRecorder->updateFinalStats(mPlayerID);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::update(DWORD timeNow, int32 maxStrength)
{
   if (mPlayerID == 0)
      return;

   if (mResGraphDef.mSamples > 0 && timeNow - mResGraphDef.mLastUpdateTime >= mResGraphDef.mInterval)
   {
      mResGraphDef.mLastUpdateTime = timeNow;

      BStatRecorder::BStatResources& resources = mResourceGraph[mResGraphDef.mIndex];

      resources.mCost = mpPlayer->getTotalResources();
      resources.mTimestamp = timeNow;

      mResGraphDef.update();

      // cache away the resources that we've used to save on serialization space
      long count = resources.mCost.getNumberResources();
      for (long i=0; i < count; ++i)
      {
         float r = resources.mCost.get(i);
         if (r > 0.0f)
            mResourcesUsed |= (1 << i);
      }
   }

   if (mScoreGraphDef.mSamples > 0 && timeNow - mScoreGraphDef.mLastUpdateTime >= mScoreGraphDef.mInterval)
   {
      mScoreGraphDef.mLastUpdateTime = timeNow;

      BStatRecorder::BStatEventInt32& score = mScoreGraph[mScoreGraphDef.mIndex];

      score.mValue = gScoreManager.getBaseScore(mPlayerID);
      score.mTimestamp = timeNow;
 
      mScoreGraphDef.update();
   }  

   if (mBaseGraphDef.mSamples > 0 && timeNow - mBaseGraphDef.mLastUpdateTime >= mBaseGraphDef.mInterval)
   {
      mBaseGraphDef.mLastUpdateTime = timeNow;

      BStatRecorder::BStatEventUInt32& base = mBaseGraph[mBaseGraphDef.mIndex];

      base.mValue = mpPlayer->getNumUnitsOfType(gDatabase.getOTIDBase());
      base.mTimestamp = timeNow;

      mBaseGraphDef.update();
   }

   if (mPopGraphDef.mSamples > 0 && timeNow - mPopGraphDef.mLastUpdateTime >= mPopGraphDef.mInterval)
   {
      mPopGraphDef.mLastUpdateTime = timeNow;

      BStatRecorder::BStatEventFloat& pop = mPopGraph[mPopGraphDef.mIndex];

      pop.mValue = mpPlayer->getPopCount(mPopID) + mpPlayer->getPopFuture(mPopID);
      pop.mTimestamp = timeNow;

      mPopGraphDef.update();
   }

   // also track how long a user is on top of the strength curve
   // in order to do that I'd need to know the current max strength
   // if my strength matches the max, then start recording time
   // when I'm no longer the max and I'm currently recording, stop recording
   // this way I'll have a total game time value that I was at the top of the curve
   // other game layers/components can divide this time by the game time to get the percentage
   if (mpPlayer->getStrength() == maxStrength)
   {
      if (!mStrengthTimer)
         mStrengthTimer = timeNow;
   }
   else if (mStrengthTimer)
   {
      mStrengthTime += (timeNow - mStrengthTimer);
      mStrengthTimer = 0;
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::eventTechResearched(long techID, uint32 gameTime)
{
   if (mPlayerID == 0)
      return;

   // find the stat recorder(s) interested in this tech
   BProtoTech* pProtoTech = gDatabase.getProtoTech(techID);
   if (!pProtoTech)
      return;

   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->trackResearched())
      {
         pRecorder->eventTechResearched(pProtoTech, gameTime);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime)
{
   if (mPlayerID == 0)
      return;

   if (!pProtoTech)
      return;

   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->trackResearched())
      {
         pRecorder->eventTechResearched(pProtoTech, gameTime);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime)
{
   if (mPlayerID == 0)
      return;

   // loop through all stat recorders and, well, record
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->trackBuilt())
      {
         pRecorder->eventBuilt(pProtoObject, pProtoSquad, gameTime);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::eventBuilt(BEntity* pEntity, uint32 gameTime)
{
   if (mPlayerID == 0)
      return;

   BProtoObject* pProtoObject = NULL;
   BProtoSquad* pProtoSquad = NULL;

   if (pEntity->getClassType() != BEntity::cClassTypeSquad)
   {
      pProtoObject = mpPlayer->getProtoObject(pEntity->getProtoID());
   }
   else
   {
      BSquad* pSquad = pEntity->getSquad();
      if (pSquad == NULL)
         return;

      pProtoObject = const_cast<BProtoObject*>(pSquad->getProtoObject());
      if (pProtoObject == NULL)
         return;

      pProtoSquad = const_cast<BProtoSquad*>(pSquad->getProtoSquad());
      if (pProtoSquad == NULL)
         return;
   }

   // loop through all stat recorders and, well, record
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->trackBuilt())
      {
         pRecorder->eventBuilt(pProtoObject, pProtoSquad, gameTime);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime)
{
   if (mPlayerID == 0)
      return;

   // loop through all stat recorders and, well, record
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->trackLost())
      {
         pRecorder->eventLost(pProtoObject, pProtoSquad, killerPlayerID, pKillerProtoObject, pKillerProtoSquad, xp, level, gameTime);
      }
   }
}

//==============================================================================
// This should be deprecated soon
//==============================================================================
void BStatsPlayer::eventLost(BEntity* pEntity, BEntity* pKillerEntity, uint32 gameTime)
{
   if (mPlayerID == 0)
      return;

   BProtoObject* pProtoObject = mpPlayer->getProtoObject(pEntity->getProtoID());
   if (pProtoObject == NULL)
      return;

   BProtoObject* pKillerProtoObject = (pKillerEntity ? mpPlayer->getProtoObject(pKillerEntity->getProtoID()) : NULL);

   // loop through all stat recorders and, well, record
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->trackLost())
      {
         pRecorder->eventLost(pProtoObject, NULL, -1, pKillerProtoObject, NULL, 0.0f, -1, gameTime);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::eventDestroyed(long playerID, BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, uint32 gameTime)
{
   if (mPlayerID == 0)
      return;

   // loop through all stat recorders and, well, record
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->trackDestroyed())
      {
         pRecorder->eventDestroyed(playerID, pProtoObject, pProtoSquad, pKillerProtoObject, pKillerProtoSquad, gameTime);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::eventDestroyed(BEntity* pEntity, BEntity* pKillerEntity, uint32 gameTime)
{
   if (mPlayerID == 0)
      return;

   BProtoObject* pProtoObject = mpPlayer->getProtoObject(pEntity->getProtoID());
   if (pProtoObject == NULL)
      return;

   BProtoObject* pKillerProtoObject = mpPlayer->getProtoObject(pKillerEntity->getProtoID());
   BProtoSquad* pKillerProtoSquad = NULL;
   BSquad* pKillerSquad = pKillerEntity->getSquad();
   if (pKillerSquad)
      pKillerProtoSquad = const_cast<BProtoSquad*>(pKillerSquad->getProtoSquad());

   // loop through all stat recorders and, well, record
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->trackDestroyed())
      {
         pRecorder->eventDestroyed(-1, pProtoObject, NULL, pKillerProtoObject, pKillerProtoSquad, gameTime);
      }
   }
}

//==============================================================================
// 
//==============================================================================
BStatRecorder* BStatsPlayer::getStatsRecorder(const char* pName) const
{
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (!pRecorder)
         continue;

      if (pRecorder->getStatDefiniion()->getName().compare(pName) == 0)
      {
         return pRecorder;
      }
   }

   return NULL;
}

//==============================================================================
// 
//==============================================================================
void BStatsPlayer::eventAbility(long abilityID)
{
   if (mPlayerID == 0 || abilityID < 0)
      return;

   BStatAbilityIDHashMap::iterator it = mAbilitiesUsed.find(abilityID);
   if (it != mAbilitiesUsed.end())
   {
      it->second = it->second + 1;
      return;
   }

   mAbilitiesUsed.insert(abilityID, 1);

#ifndef BUILD_FINAL
   mMemDirty = true;
   gStatsManager.mMemDirty = true;
#endif
}

//==============================================================================
// 
//==============================================================================
BStatRecorderGraph* BStatsPlayer::getGraphByID(uint8 statID) const
{
   // find the recorder graph that matches the statID
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder && pRecorder->getID() == statID && pRecorder->getType() == eGraph)
         return static_cast<BStatRecorderGraph*>(pRecorder);
   }

   return NULL;
}

//==============================================================================
// See queryResourceGraph.
//==============================================================================
const void BStatsPlayer::queryGraphByID(uint8 statID, uint32& minX, uint32& maxX, uint32& minY, uint32& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEvent>& points) const
{
   BStatRecorderGraph* pGraph = getGraphByID(statID);
   if (!pGraph)
      return;

   pGraph->query(minX, maxX, minY, maxY, points);
}

//==============================================================================
// Resources are collected with the X axis representing the game time and the
// Y axis representing a value from the BCost instance.
//
// resourcesUsed - bitmask indicating which resources a player used.
//                 Each bit corresponds to an index into the BCost instance.
//                 For example, if bit 0 is set to 1, then BCost.get(0) will
//                    return the amount of resources collected by that user
//                    for that resource type.
//                 The resource name is from gDatabase.getResourceName(index)
//                    where index is the bit position.
// minX - the minimum gametime value of all resource data collected
// maxX - the maximum gametime of all resource data collected
// minY - the minimum amount of a resource type collected.  The value may
//           represent power, supplies, favor, etc...  Depending on the
//           resource model we end up with.
// maxY - the maximum amount of a resource type collected
// resourcePoints - populated in sorted order from smallest to largest in terms
//                     of gametime (X-axis).  Contains only as many points as
//                     were collected.
//==============================================================================
const void BStatsPlayer::queryResourceGraph(uint8& resourcesUsed, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatResources>& resourcePoints) const
{
   resourcesUsed = mResourcesUsed;

   minX = 0;
   maxX = 0;

   minY = 0.0f;
   maxY = 0.0f;

   resourcePoints = mResourceGraph;

   // trip the array down to the actual number of samples collected
   resourcePoints.setNumber(mResGraphDef.mSize);

   // sort according to game time
   resourcePoints.sort(BStatResourcesCompareFunc);

   if (mResGraphDef.mSize == 0)
      return;

   // find the min/max values
   const BStatRecorder::BStatResources& begin = resourcePoints[0];
   const BStatRecorder::BStatResources& end = resourcePoints[mResGraphDef.mSize-1];

   minX = begin.mTimestamp;
   maxX = end.mTimestamp;

   long numResources = mTotalResources.getNumberResources();
   for (uint i = 0; i < mResGraphDef.mSize; ++i)
   {
      const BStatRecorder::BStatResources& resources = resourcePoints[i];

      for (long j=0; j < numResources; ++j)
      {
         if (mResourcesUsed & (1 << j))
         {
            float resource = resources.mCost.get(j);
            minY = min(minY, resource);
            maxY = max(maxY, resource);
         }
      }
   }
}

//==============================================================================
// See queryResourceGraph.
// The main difference is the lack of a resourcesUsed value.
//==============================================================================
const void BStatsPlayer::queryScoreGraph(uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventInt32>& scorePoints) const
{
   minX = 0;
   maxX = 0;

   minY = 0.0f;
   maxY = 0.0f;

   scorePoints = mScoreGraph;

   scorePoints.setNumber(mScoreGraphDef.mSize);

   scorePoints.sort(BStatEventFloatCompareFunc);

   if (mScoreGraphDef.mSize == 0)
      return;

   const BStatRecorder::BStatEventInt32& begin = scorePoints[0];
   const BStatRecorder::BStatEventInt32& end = scorePoints[mScoreGraphDef.mSize-1];

   minX = begin.mTimestamp;
   maxX = end.mTimestamp;

   for (uint i = 0; i < mScoreGraphDef.mSize; ++i)
   {
      const BStatRecorder::BStatEventInt32& score = scorePoints[i];

      minY = min(minY, score.mValue);
      maxY = max(maxY, score.mValue);
   }
}

//==============================================================================
// See queryResourceGraph.
// The main difference is the lack of a resourcesUsed value.
//==============================================================================
const void BStatsPlayer::queryBaseGraph(uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventUInt32>& basePoints) const
{
   minX = 0;
   maxX = 0;

   minY = 0.0f;
   maxY = 0.0f;

   basePoints = mBaseGraph;

   basePoints.setNumber(mBaseGraphDef.mSize);

   basePoints.sort(BStatEventUInt32CompareFunc);

   if (mBaseGraphDef.mSize == 0)
      return;

   const BStatRecorder::BStatEventUInt32& begin = basePoints[0];
   const BStatRecorder::BStatEventUInt32& end = basePoints[mBaseGraphDef.mSize-1];

   minX = begin.mTimestamp;
   maxX = end.mTimestamp;

   for (uint i = 0; i < mBaseGraphDef.mSize; ++i)
   {
      const BStatRecorder::BStatEventUInt32& base = basePoints[i];

      minY = min(minY, base.mValue);
      maxY = max(maxY, base.mValue);
   }
}

//==============================================================================
// See queryResourceGraph.
// The main difference is the lack of a resourcesUsed value.
// Population is tracked similar to the UI, meaning current + future pop.
// Y-axis is a float and requires rounding.
// For example, a population of 2 is stored as 1.99 in the array.
//==============================================================================
const void BStatsPlayer::queryPopGraph(uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventFloat>& popPoints) const
{
   minX = 0;
   maxX = 0;

   minY = 0.0f;
   maxY = 0.0f;

   popPoints = mPopGraph;

   popPoints.setNumber(mPopGraphDef.mSize);

   popPoints.sort(BStatEventFloatCompareFunc);

   if (mPopGraphDef.mSize == 0)
      return;

   const BStatRecorder::BStatEventFloat& begin = popPoints[0];
   const BStatRecorder::BStatEventFloat& end = popPoints[mPopGraphDef.mSize-1];

   minX = begin.mTimestamp;
   maxX = end.mTimestamp;

   for (uint i = 0; i < mPopGraphDef.mSize; ++i)
   {
      const BStatRecorder::BStatEventFloat& pop = popPoints[i];

      minY = min(minY, pop.mValue);
      maxY = max(maxY, pop.mValue);
   }
}

//==============================================================================
// If you need this, see Doug
//==============================================================================
//bool BStatsPlayer::collectAIStats(BStatRecorder::BStatTotalHashMap& rolledupSquadTotals, BStatRecorder::BStatTotalHashMap& rolledupTechTotals, BStatRecorder::BStatTotalHashMap& rolledupObjectTotals) const
//{
//   // loop through all the stat recorders and save
//   uint numStats = mStatRecorders.getSize();
//   for (uint s=0; s < numStats; s++)
//   {
//      BStatRecorder* pRecorder = mStatRecorders[s];
//      if (pRecorder)
//         pRecorder->collectAIStats(mpPlayer, rolledupSquadTotals, rolledupTechTotals, rolledupObjectTotals);
//   }
//
//   return(true);
//}

//==============================================================================
// 
//==============================================================================
bool BStatsPlayer::write(BDynamicStream& stream, ULONGLONG machineId) const
{
   if (mPlayerType != BPlayer::cPlayerTypeHuman && mPlayerType != BPlayer::cPlayerTypeComputerAI)
      return true;

   stream.printf("<name>%s</name>", mpPlayer->getName().getPtr());

   stream.printf("<pid>%d</pid>", mpPlayer->getID());

   // if we have a user, then it's a local player (or at least should be)
   if (mpPlayer && mpPlayer->getUser())
      stream.printf("<mid>%I64u</mid>", machineId);

   stream.printf("<xuid>%I64u</xuid>", mXuid);

   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser != NULL && pUser->getPlayerID() == mPlayerID)
   {
      // grab the user profile and extract the timeline, skulls, rank & level, campaign progress
      BUserProfile* pProfile = pUser->getProfile();
      if (pProfile != NULL)
      {
         // wrapping up the profile information
         // <pf>...</pf>
         // tl == timeline bits
         // sk == skull bits
         //
         // Campaign
         // <cp>...</cp>
         // mi == unlocked mission count
         // mo == unlocked movie count
         //
         // Rank
         // <rank...>
         // r == rank 0-8
         // l == level 0-50
         // w == won matchmade games
         //
         stream.printf("<pf><tl>%I64u</tl><sk>%u</sk>", pProfile->getTimeLineBits(), pProfile->getSkullBits());

         const BCampaignProgress& campaign = pProfile->getCampaignProgress();
         stream.printf("<cp mi='%d' mo='%d'/>", campaign.getUnlockedMissionCount(), campaign.getUnlockedMovieCount());

         const BProfileRank& rank = pProfile->getRank();
         stream.printf("<rank r='%u' l='%u' w='%u' s='%u'/></pf>", rank.mRank, rank.mLevel, rank.mWon, pProfile->mMatchmakingScore);
      }
   }

   stream.printf("<team>%d</team>", mpPlayer->getTeamID());

   stream.printf("<c>%08X</c>", gDatabase.getPlayerColor(gWorld->getPlayerColorCategory(), mpPlayer->getColorIndex()));

   const char* pType = (mPlayerType == BPlayer::cPlayerTypeHuman ? "human" : "ai");
   stream.printf("<type>%s</type>", pType);

   stream.printf("<diff t='%d' l='%f'/>", mDifficultyType, mDifficulty);

   stream.printf("<civ r='%d'>%s</civ>", mRandomCiv, mpPlayer->getCiv()->getCivName().getPtr());
   stream.printf("<leader r='%d'>%s</leader>", mRandomLeader, mpPlayer->getLeader()->mName.getPtr());

   const char* pState = "unknown";

   if (mWon)
      pState = "won";
   else if (mResigned)
      pState = "resigned";
   else if (mDefeated)
      pState = "defeated";
   else if (mDisconnected)
      pState = "disconnected";
   else
      pState = "unknown";
   stream.printf("<state t='%d'>%s</state>", mPlayerStateTime, pState);

   const BScorePlayer* pScorePlayer = gScoreManager.getPlayerByID(mpPlayer->getID());
   if (pScorePlayer)
   {
      long finalScore = gScoreManager.getFinalScore(mpPlayer->getID());
      stream.printf("<score c='%0.2f' m='%0.2f' b='%d'>%d</score>", pScorePlayer->mCombatBonus, pScorePlayer->mMPBonus, pScorePlayer->mFinalBaseScore, finalScore);
   }

   uint32 strengthTime = mStrengthTime;
   if (mStrengthTimer)
      strengthTime += (gWorld->getGametime() - mStrengthTimer);

   stream.printf("<strengthTime>%d</strengthTime>", strengthTime);

   stream.printf("<resources>");
   long numResources = mTotalResources.getNumberResources();
   for (long i=0; i < numResources; ++i)
   {
      if (mTotalResources.get(i) > 0.0f)
         stream.printf("<resource id='%d'>%0.2f</resource>", i, mTotalResources.get(i));
   }
   stream.printf("</resources>");

   stream.printf("<maxResources>");
   numResources = mMaxResources.getNumberResources();
   for (long i=0; i < numResources; ++i)
   {
      if (mMaxResources.get(i) > 0.0f)
         stream.printf("<resource id='%d'>%0.2f</resource>", i, mMaxResources.get(i));
   }
   stream.printf("</maxResources>");

   stream.printf("<gathered>");
   long numGathered = mGatheredResources.getNumberResources();
   for (long i=0; i < numGathered; ++i)
   {
      if (mGatheredResources.get(i) > 0.0f)
         stream.printf("<resource id='%d'>%0.2f</resource>", i, mGatheredResources.get(i));
   }
   stream.printf("</gathered>");

   stream.printf("<tributed>");
   long numTributed = mTributedResources.getNumberResources();
   for (long i=0; i < numTributed; ++i)
   {
      stream.printf("<resource id='%d'>%0.2f</resource>", i, mTributedResources.get(i));
   }
   stream.printf("</tributed>");

   stream.printf("<powers>");
   for (BStatProtoPowerIDHash::const_iterator it = mPowersUsed.begin(); it != mPowersUsed.end(); ++it)
   {
      BProtoPowerID protoPowerID = it->first;
      uint used = it->second;

      stream.printf("<p n='%s' u='%d'/>", gDatabase.getProtoPowerByID(protoPowerID)->getName().getPtr(), used);
   }
   stream.printf("</powers>");

   stream.printf("<abilities>");
   for (BStatAbilityIDHashMap::const_iterator it = mAbilitiesUsed.begin(); it != mAbilitiesUsed.end(); ++it)
   {
      stream.printf("<a n='%s' u='%d'/>", gDatabase.getAbilityName(it->first), it->second);
   }
   stream.printf("</abilities>");

   stream.printf("<resourceGraph u='%u'>", mResourcesUsed);
   for (uint i = 0; i < mResGraphDef.mSize; ++i)
   {
      const BStatRecorder::BStatResources& resources = mResourceGraph[i];

      if (i > 0)
         stream.printf(",(%u", resources.mTimestamp);
      else
         stream.printf("((%u", resources.mTimestamp);

      for (long j=0; j < numResources; ++j)
      {
         if (mResourcesUsed & (1 << j))
            stream.printf(",%0.2f", resources.mCost.get(j));
      }

      stream.printf(")");
   }
   if (mResGraphDef.mSize > 0)
      stream.printf(")");
   stream.printf("</resourceGraph>");

   stream.printf("<popGraph>");
   for (uint i = 0; i < mPopGraphDef.mSize; ++i)
   {
      const BStatRecorder::BStatEventFloat& pop = mPopGraph[i];

      if (i > 0)
         stream.printf(",(%u,%0.2f)", pop.mTimestamp, pop.mValue);
      else
         stream.printf("((%u,%0.2f)", pop.mTimestamp, pop.mValue);
   }
   if (mPopGraphDef.mSize > 0)
      stream.printf(")");
   stream.printf("</popGraph>");

   stream.printf("<scoreGraph>");
   for (uint i = 0; i < mScoreGraphDef.mSize; ++i)
   {
      const BStatRecorder::BStatEventInt32& score = mScoreGraph[i];

      if (i > 0)
         stream.printf(",(%u,%d)", score.mTimestamp, score.mValue);
      else
         stream.printf("((%u,%d)", score.mTimestamp, score.mValue);
   }
   if (mScoreGraphDef.mSize > 0)
      stream.printf(")");
   stream.printf("</scoreGraph>");

   // XXX we don't want to upload the baseGraph for production
   // in stats.xml, set the upload attribute on the BaseGraph node to false
   // <BaseGraph upload="0">
   if (mBaseGraphDef.mUpload)
   {
      stream.printf("<baseGraph>");
      for (uint i = 0; i < mBaseGraphDef.mSize; ++i)
      {
         const BStatRecorder::BStatEventUInt32& base = mBaseGraph[i];

         if (i > 0)
            stream.printf(",(%u,%d)", base.mTimestamp, base.mValue);
         else
            stream.printf("((%u,%d)", base.mTimestamp, base.mValue);
      }
      if (mBaseGraphDef.mSize > 0)
         stream.printf(")");
      stream.printf("</baseGraph>");
   }

   // loop through all the stat recorders and save
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
//-- FIXING PREFIX BUG ID 3059
      const BStatRecorder* pRecorder = mStatRecorders[s];
//--
      if (pRecorder)
         pRecorder->write(mpPlayer, stream);
   }

   return true;
}

//==============================================================================
// 
//==============================================================================
#ifndef BUILD_FINAL
uint32 BStatsPlayer::getMemUsage()
{
   if (!mMemDirty)
      return mMemUsage;

   mMemUsage = sizeof(BStatsPlayer);
   mMemUsage += mStatRecorders.getSizeInBytes();

   // loop through all stat recorders
   uint numStats = mStatRecorders.getSize();
   for (uint s=0; s < numStats; s++)
   {
      BStatRecorder* pRecorder = mStatRecorders[s];
      if (pRecorder)
         mMemUsage += pRecorder->getMemUsage();
   }

   mMemUsage += mResourceGraph.getSizeInBytes();
   mMemUsage += mPopGraph.getSizeInBytes();
   mMemUsage += mBaseGraph.getSizeInBytes();
   mMemUsage += mScoreGraph.getSizeInBytes();
   mMemUsage += (mPowersUsed.getMaxEntries() * (sizeof(long) + sizeof(uint) + sizeof(int)));
   mMemUsage += (mAbilitiesUsed.getMaxEntries() * (sizeof(long) + sizeof(uint) + sizeof(int)));

   return mMemUsage;
}
#endif

//==============================================================================
// 
//==============================================================================
BStatsManager::BStatsManager() :
   mXuid(0),
   mControllerID(0),
   mXNetGetTitleXnAddrFlags(XNET_GET_XNADDR_PENDING),
   mpObjectIDMap(NULL),
   mpSquadIDMap(NULL),
   mGameType(BStats::eUnknown),
   mGameMode(0),
   mSettingsGameType(BGameSettings::cGameTypeSkirmish),
   mDisabled(true),
   mLiveGame(false)
{
}

//==============================================================================
// 
//==============================================================================
BStatsManager::~BStatsManager()
{
   shutdown(true);
}

//==============================================================================
// 
//==============================================================================
bool BStatsManager::setup()
{
   mDisabled = !gConfig.isDefined(cConfigEnableGameStats);

   if (mDisabled)
      return true;

   return load();
}

//==============================================================================
// 
//==============================================================================
void BStatsManager::shutdown(bool destruct)
{
   if (mDisabled)
      return;

   reset();

   delete mpObjectIDMap;
   mpObjectIDMap = NULL;

   delete mpSquadIDMap;
   mpSquadIDMap = NULL;

   uint count = mStatDefinitions.getSize();
   for (uint i=0; i < count; ++i)
   {
      BStatDefinition* pDef = mStatDefinitions[i];

      if (!pDef)
         continue;

      BStatDefinition::releaseInstance(pDef);
   }

   mStatDefinitions.clear();

   if (!destruct)
   {
      BStatDefinition::mFreeList.clear();
      BStatRecorderTotal::mFreeList.clear();
      BStatRecorderEvent::mFreeList.clear();
      BStatRecorderGraph::mFreeList.clear();
      BStatsPlayer::mFreeList.clear();
   }
}

//==============================================================================
// Initialize all the players/stat structures for a new game
//==============================================================================
void BStatsManager::init(bool liveGame)
{
   if (mDisabled)
      return;

#ifndef BUILD_FINAL
   mMemDirty = true;
   mMemUsage = 0;
#endif

   // flush out all old stats and get ready to start recording new ones
   // release all BStatsPlayer instances
   reset(true);

   // start by storing off the game ID, map name and game mode
   gDatabase.getGameSettings()->getString(BGameSettings::cGameID, mGameID);
   gDatabase.getGameSettings()->getString(BGameSettings::cMapName, mMapName);
   gDatabase.getGameSettings()->getLong(BGameSettings::cGameMode, mGameMode);
   gDatabase.getGameSettings()->getLong(BGameSettings::cGameType, mSettingsGameType);

   mXNetGetTitleXnAddrFlags = XNET_GET_XNADDR_PENDING;
   Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR));

   mXNetGetTitleXnAddrFlags = XNetGetTitleXnAddr(&mXnAddr);

   mXuid = 0;
   mControllerID = 0;
   mLiveGame = liveGame;

//-- FIXING PREFIX BUG ID 3064
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (!pUser)
      return;

   if (!pUser->isSignedIn())
      return;

   // verify the user is valid for playing mp games on live
   if (!pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
      return;

   mXuid = pUser->getXuid();
   mControllerID = pUser->getPort();
   mGamertag = pUser->getName();

#ifndef BUILD_FINAL
   // fake out a live enabled game if we want to test stats
   if (gConfig.isDefined(cConfigForceGameStats))
      mLiveGame = true;
#endif
}

//==============================================================================
// Flush all existing stats and prepare for the next game
//==============================================================================
void BStatsManager::reset(bool newGame)
{
   if (mDisabled)
      return;

   uint count = mPlayers.getSize();
   for (uint i=0; i < count; ++i)
   {
      BStatsPlayer* pPlayer = mPlayers[i];

      if (!pPlayer)
         continue;

      BStatsPlayer::releaseInstance(pPlayer);
   }

   mPlayers.clear();
   mGraphs.clear();

   mGameID.empty();
   mMapName.empty();

   mOoSReports.clear();

   if (!newGame)
   {
      Utils::FastMemSet(mClientToPlayerMap, cInvalidPlayerID, sizeof(mClientToPlayerMap));
      Utils::FastMemSet(mClientDisconnect, 0, sizeof(mClientDisconnect));
   }
}

//============================================================================
// 
//============================================================================
void BStatsManager::submitUserProfileStats(DWORD gameTime, long winningTeamID)
{
   // mrh - wtf is this for.  We don't use any of it.  Gone.
   gameTime;
   winningTeamID;
}

//============================================================================
// 
//============================================================================
void BStatsManager::submit(DWORD gameTime, long winningTeamID)
{
   if (mDisabled)
      return;

   //-- Write out stats to the user profile is this a Human player with other (AI allies or opponents) AND gametype is Skirmish.
   //-- Dont want to write out for SPC games.
   submitUserProfileStats(gameTime, winningTeamID);

   // if we were not signed-in at the start of the game, then
   // we do not want to bother with submitting stats to the LSP
   //
   // if we attempt to submit stats to the LSP but then user has since disconnected from Live or signed-out
   // then the LSP code will attempt to at least try and request the user to sign back in
   // 
   if (mXuid == 0)
      return;

   // if this is not a LIVE game and it's also not a campaign game then don't upload
   // this allows us to send up campaign stats to track progress on the server
   if (!mLiveGame && mSettingsGameType != BGameSettings::cGameTypeCampaign)
      return;

   uint retry=0;
   while (mXNetGetTitleXnAddrFlags == XNET_GET_XNADDR_PENDING && retry++ < 5)
   {
      mXNetGetTitleXnAddrFlags = XNetGetTitleXnAddr(&mXnAddr);
      if (retry > 0)
         Sleep(1);
   }

   ULONGLONG machineId = 0;

   if ((mXNetGetTitleXnAddrFlags & XNET_GET_XNADDR_NONE) == 0)
   {
      // if this errors out, the server will treat a machineId of 0 as suspect
      XNetXnAddrToMachineId(&mXnAddr, &machineId);
   }

   BDynamicStream stream;

   stream.printf("<game v='%d'><mid>%I64u</mid><id>%s</id><map>%s</map>", BStats::cUploadVersion, machineId, mGameID.getPtr(), mMapName.getPtr());
   if (mGameType == BStats::eCustom)
      stream.printf("<t>custom</t>");
   else if (mGameType == BStats::eMatchMaking)
      stream.printf("<t>mm</t>");
   else
      stream.printf("<t>unknown</t>");

   // Game settings type
   // BGameSettings::cGameTypeSkirmish == 0
   // BGameSettings::cGameTypeCampaign == 1
   // BGameSettings::cGameTypeScenario == 2
   stream.printf("<st>%d</st>", mSettingsGameType);

   BGameMode* pGameMode = gDatabase.getGameModeByID(mGameMode);
   if (pGameMode != NULL)
      stream.printf("<md>%s</md>", pGameMode->getName().getPtr());
   else
      stream.printf("<md>unknown</md>", pGameMode->getName().getPtr());

   stream.printf("<resources>");

   long numResources = gDatabase.getNumberResources();
   for (long i=0; i < numResources; ++i)
   {
      stream.printf("<resource id='%d'>%s</resource>", i, gDatabase.getResourceName(i));
   }
   stream.printf("</resources>");

   SYSTEMTIME systemTime;
   GetSystemTime(&systemTime);

   BSimString time;
   time.format("%04u%02u%02uT%02u%02u%02uZ", systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
   stream.printf("<time>%s</time><gt>%d</gt><won>%d</won>", time.getPtr(), gameTime, winningTeamID);

   // remap clientIDs to the new playerIDs
   uint count = mPlayers.getSize();
   for (uint i=1; i < count; ++i)
   {
      BStatsPlayer* pStatsPlayer = mPlayers[i];

//-- FIXING PREFIX BUG ID 3067
      const BPlayer* pPlayer = pStatsPlayer->getPlayer();
//--
      if (pPlayer)
      {
         for (uint i=0; i < XNetwork::cMaxClients; ++i)
         {
            if (mClientToPlayerMap[i] == pPlayer->getMPID())
            {
               mClientToPlayerMap[i] = pPlayer->getID();
               break;
            }
         }
      }
   }

   stream.printf("<clients>");
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      stream.printf("<client id='%d' pid='%d' d='%d'/>", i, mClientToPlayerMap[i], mClientDisconnect[i]);
   }
   stream.printf("</clients>");

   if (mOoSReports.getSize() > 0)
   {
      stream.printf("<oos>");
      for (uint i=0; i < mOoSReports.getSize(); ++i)
      {
         const BOoS& oos = mOoSReports[i];
         stream.printf("<report from='%d' with='%d' checksumID='%d'/>", oos.mFromClientID, oos.mWithClientID, oos.mChecksumID);
      }
      stream.printf("</oos>");
   }

   for (uint i=1; i < count; ++i)
   {
//-- FIXING PREFIX BUG ID 3068
      const BStatsPlayer* pPlayer = mPlayers[i];
//--

      if (!pPlayer)
         continue;

      stream.printf("<player>");
      pPlayer->write(stream, machineId);
      stream.printf("</player>");
   }

   stream.printf("</game>");

   BDynamicStream* pStream = new BDynamicStream(); // <-- LSPManager owns this pointer (see below) and will clean it up

   BDeflateStream* pDeflStream = new BDeflateStream(*pStream);
   pDeflStream->writeBytes(stream.getBuf().getPtr(), stream.getBuf().getSizeInBytes());
   pDeflStream->close();
   delete pDeflStream;

   gLSPManager.uploadFile(mXuid, mGamertag, "statslog.xml.hwz", pStream);

   // debugging purposes only, writes out to the game's recordgame directory
   //{
   //   char systemName[256] = "Unknown";
   //   DWORD size = sizeof(systemName);
   //   DmGetXboxName(systemName, &size);

   //   BString fileName;
   //   BUser *user = gUserManager.getUser(BUserManager::cPrimaryUser);
   //   fileName.format("%s_%s_statslog_%04u%02u%02uT%02u%02u%02uZ.xml", systemName, user->getName().getPtr(), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

   //   BStream* pFileStream = gFileSystemStreamFactory.create(cDirRecordGame, fileName, cSFWritable);

   //   pFileStream->writeBytes(stream.getBuf().getPtr(), stream.getBuf().getSizeInBytes());

   //   delete pFileStream;
   //}
}

//============================================================================
// 
//============================================================================
BStatsPlayer* BStatsManager::createStatsPlayer(BPlayer* pPlayer)
{
   if (mDisabled)
      return NULL;

   if (!pPlayer)
      return NULL;

   BStatsPlayer* pStatsPlayer = BStatsPlayer::getInstance();
   if (!pStatsPlayer)
      return NULL;

   pStatsPlayer->init(pPlayer);

   // this only works because I'm inserting players in order of their player ID
   // if we ever create players out of order, this will assert
   mPlayers.insert(pPlayer->getID(), pStatsPlayer);

   // don't bother initializing stat recorders for Gaia
   if (pPlayer->getID() == 0)
      return pStatsPlayer;

   // also need to initialize all the stats collectors that we have definitions for
   uint count = mStatDefinitions.getSize();
   for (uint i=0; i < count; ++i)
   {
      BStatDefinition* pDef = mStatDefinitions[i];

      if (!pDef)
         continue;

      BStatRecorder* pRecorder = NULL;

      switch (pDef->mType)
      {
         case eTotal:
            {
               pRecorder = BStatRecorderTotal::getInstance();
            }
            break;
         case eEvent:
            {
               pRecorder = BStatRecorderEvent::getInstance();
            }
            break;
         case eGraph:
            {
               BStatRecorderGraph* pGraph = BStatRecorderGraph::getInstance();
               pRecorder = pGraph;
               mGraphs.add(pGraph);
            }
            break;
         case eResourceGraph:
            {
               pStatsPlayer->initResourceGraph(pDef);
            }
            break;
         case ePopGraph:
            {
               pStatsPlayer->initPopGraph(pDef);
            }
            break;
         case eBaseGraph:
            {
               pStatsPlayer->initBaseGraph(pDef);
            }
            break;
         case eScoreGraph:
            {
               pStatsPlayer->initScoreGraph(pDef);
            }
            break;
      }

      if (pRecorder)
      {
         pRecorder->init(pDef, mpObjectIDMap, mpSquadIDMap);
         pStatsPlayer->mStatRecorders.add(pRecorder);
      }
   }

#ifndef BUILD_FINAL
   mMemDirty = true;
#endif

   return pStatsPlayer;
}

//============================================================================
// 
//============================================================================
BStatsPlayer* BStatsManager::getStatsPlayer(BPlayerID playerID) const
{
   if (playerID < 0 || playerID >= mPlayers.getNumber())
      return NULL;
   else
      return mPlayers[playerID];
}

//============================================================================
// See BStatsPlayer::queryGraphByID
//============================================================================
const void BStatsManager::queryGraphByPlayerIDAndStatID(BPlayerID playerID, uint8 statID, uint32& minX, uint32& maxX, uint32& minY, uint32& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEvent>& points) const
{
   const BStatsPlayer* pPlayer = getStatsPlayer(playerID);
   if (!pPlayer)
      return;

   pPlayer->queryGraphByID(statID, minX, maxX, minY, maxY, points);
}

//============================================================================
// See BStatsPlayer::queryResourceGraph
//============================================================================
const void BStatsManager::queryResourceGraphByPlayerID(BPlayerID playerID, uint8& resourcesUsed, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatResources>& resourcePoints) const
{
   const BStatsPlayer* pPlayer = getStatsPlayer(playerID);
   if (!pPlayer)
      return;

   pPlayer->queryResourceGraph(resourcesUsed, minX, maxX, minY, maxY, resourcePoints);
}

//============================================================================
// See BStatsPlayer::queryScoreGraph
//============================================================================
const void BStatsManager::queryScoreGraphByPlayerID(BPlayerID playerID, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventInt32>& scorePoints) const
{
   const BStatsPlayer* pPlayer = getStatsPlayer(playerID);
   if (!pPlayer)
      return;

   pPlayer->queryScoreGraph(minX, maxX, minY, maxY, scorePoints);
}

//============================================================================
// See BStatsPlayer::queryBaseGraph
//============================================================================
const void BStatsManager::queryBaseGraphByPlayerID(BPlayerID playerID, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventUInt32>& basePoints) const
{
   const BStatsPlayer* pPlayer = getStatsPlayer(playerID);
   if (!pPlayer)
      return;

   pPlayer->queryBaseGraph(minX, maxX, minY, maxY, basePoints);
}

//============================================================================
// See BStatsPlayer::queryPopGraph
//============================================================================
const void BStatsManager::queryPopGraphByPlayerID(BPlayerID playerID, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventFloat>& popPoints) const
{
   const BStatsPlayer* pPlayer = getStatsPlayer(playerID);
   if (!pPlayer)
      return;

   pPlayer->queryPopGraph(minX, maxX, minY, maxY, popPoints);
}

//============================================================================
// 
//============================================================================
void BStatsManager::notify(DWORD eventType, long targetPlayerID, long targetProtoID, long targetProtoSquadID, long sourcePlayerID, long sourceProtoID, long sourceProtoSquadID, float xp, long level)
{
   if (mDisabled)
      return;

   if (targetPlayerID == 0 || targetPlayerID == cInvalidPlayerID)
      return;

   if (targetPlayerID >= mPlayers.getNumber())
      return;

   BStatsPlayer* pStatsPlayer = mPlayers[targetPlayerID];
   if (!pStatsPlayer)
      return;

   //BPlayer* pPlayer = pStatsPlayer->getPlayer();

   uint32 gameTime = (gWorld ? gWorld->getGametime() : 0);

   switch (eventType)
   {
      case BEntity::cEventTechResearched:
         {
            //BProtoTech* pProtoTech = (pPlayer ? pPlayer->getProtoTech(targetProtoID) : gDatabase.getProtoTech(targetProtoID));
            BProtoTech* pProtoTech = gDatabase.getProtoTech(targetProtoID);
            if (!pProtoTech)
               return;

            pStatsPlayer->eventTechResearched(pProtoTech, gameTime);
         }
         break;

      //case BEntity::cEventBuiltUnit:
      //   {
      //      // XXX units are built and then added to the squad, so at this point I don't know what squad I will be in
      //      //
      //      // if we have a valid targetProtoSquadID, lookup the BProtoSquad and pass in
      //      //BProtoSquad* pProtoSquad = NULL;
      //      //if (targetProtoSquadID != cInvalidProtoID)
      //      //   pProtoSquad = (pPlayer ? pPlayer->getProtoSquad(targetProtoSquadID) : gDatabase.getProtoSquad(targetProtoSquadID));

      //      BProtoObject* pProtoObject = (pPlayer ? pPlayer->getProtoObject(targetProtoID) : gDatabase.getProtoObject(targetProtoID));
      //      if (!pProtoObject)
      //         return;

      //      pStatsPlayer->eventBuilt(pProtoObject, NULL);
      //   }
      //   break;

      case BEntity::cEventKilled:
      case BEntity::cEventKilledUnit:
         {
            BProtoSquad* pProtoSquad = NULL;
            if (targetProtoSquadID != cInvalidProtoID)
               pProtoSquad = gDatabase.getGenericProtoSquad(targetProtoSquadID);
               //pProtoSquad = (pPlayer ? pPlayer->getProtoSquad(targetProtoSquadID) : gDatabase.getGenericProtoSquad(targetProtoSquadID));

            //BProtoObject* pProtoObject = (pPlayer ? pPlayer->getProtoObject(targetProtoID) : gDatabase.getGenericProtoObject(targetProtoID));
            BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(targetProtoID);
            if (!pProtoObject)
               return;

            BStatsPlayer* pStatsPlayerSource = NULL;
            BProtoObject* pKillerProtoObject = NULL;
            BProtoSquad* pKillerProtoSquad = NULL;

            // lookup the object ID that killed us
            if (sourcePlayerID != cInvalidPlayerID && sourcePlayerID != 0)
            {
               pStatsPlayerSource = mPlayers[sourcePlayerID];
               if (pStatsPlayerSource)
               {
                  BPlayer* pPlayerSource = pStatsPlayerSource->getPlayer();
                  if (pPlayerSource)
                  {
                     if (sourceProtoID != cInvalidProtoID)
                        pKillerProtoObject = (pPlayerSource ? pPlayerSource->getProtoObject(sourceProtoID) : gDatabase.getGenericProtoObject(sourceProtoID));
                        //pKillerProtoObject = gDatabase.getGenericProtoObject(sourceProtoID);
                     if (sourceProtoSquadID != cInvalidProtoID)
                        pKillerProtoSquad = gDatabase.getGenericProtoSquad(sourceProtoSquadID);
                        //pKillerProtoSquad = (pPlayerSource ? pPlayerSource->getProtoSquad(sourceProtoSquadID) : gDatabase.getGenericProtoSquad(sourceProtoSquadID));

                     // [5/6/2008 JRuediger] Save some AI Memory stats if our ptrs to date are valid.
                     //if (pPlayer && pPlayer->getUser() && pPlayer->getTrackingData())
                     //{
                     //   //-- if AI killed my unit and I am human, record the attack time.
                     //   if (pPlayerSource->getPlayerType() == BPlayer::cPlayerTypeComputerAI && pPlayer->getPlayerType() == BPlayer::cPlayerTypeHuman)
                     //   {
                     //      uint newTime = gameTime;
                     //      uint firstAttackTime = pPlayer->getTrackingData()->getFirstTimeAIAttackedMe();
                     //      if (newTime < firstAttackTime)
                     //      {
                     //         pPlayer->getTrackingData()->setFirstTimeAIAttackedMe(newTime);
                     //      }
                     //   }
                     //}

                     //-- if I killed the AI's unit and I am human, record the attack time.
                     //if (pPlayerSource && pPlayerSource->getUser() && pPlayerSource->getTrackingData())
                     //{
                     //   if (pPlayerSource->getPlayerType() == BPlayer::cPlayerTypeHuman && pPlayer->getPlayerType() == BPlayer::cPlayerTypeComputerAI)
                     //   {
                     //      uint newTime = gameTime;
                     //      uint firstAttackTime = pPlayerSource->getTrackingData()->getFirstTimeIAttackedAI();
                     //      if (newTime < firstAttackTime)
                     //      {
                     //         pPlayerSource->getTrackingData()->setFirstTimeIAttackedAI(newTime);
                     //      }
                     //   }
                     //}
                  }
               }
            }

            //BProtoObject* pKillerProtoObject = (sourceProtoID != cInvalidProtoID ? (pPlayer ? pPlayer->getProtoObject(sourceProtoID) : gDatabase.getProtoObject(sourceProtoID)) : NULL);

            pStatsPlayer->eventLost(pProtoObject, pProtoSquad, sourcePlayerID, pKillerProtoObject, pKillerProtoSquad, xp, level, gameTime);

            //if (pKillerProtoObject)
            //{
            //   if (sourcePlayerID != cInvalidPlayerID && sourcePlayerID != 0)
            //   {
            //      pStatsPlayer = mPlayers[sourcePlayerID];
            //      if (pStatsPlayer)
            //         pStatsPlayer->eventDestroyed(pProtoObject, pProtoSquad, pKillerProtoObject);
            //   }
            //}

            if (pStatsPlayerSource)
               pStatsPlayerSource->eventDestroyed(targetPlayerID, pProtoObject, pProtoSquad, pKillerProtoObject, pKillerProtoSquad, gameTime);
         }
         break;
   }
}

//============================================================================
// 
//============================================================================
void BStatsManager::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (mDisabled)
      return;

   // XXX testing...
   //return;

   uint32 gameTime = (gWorld ? gWorld->getGametime() : 0);

   switch (eventType)
   {
      //case BEntity::cEventTechResearched:
      //   {
      //      // senderID - entity that the research came from
      //      // data - techID researched
      //      // data2 - player ID
      //      BPlayerID playerID = static_cast<BPlayerID>(data2);
      //      if (playerID != cInvalidPlayerID)
      //      {
      //         BStatsPlayer* pPlayer = mPlayers[playerID];
      //         if (!pPlayer)
      //            return;
      //         pPlayer->eventTechResearched(static_cast<long>(data));
      //      }
      //   }
      //   break;

      //case BEntity::cEventBuilt:
      //   {
      //      // only track squads
      //      if (senderID.getType() != BEntity::cClassTypeSquad)
      //         return;

      //      // track the person who built it
      //      // senderID - EntityID of the item built
      //      // data - BPlayerID, needs cast
      //      // only need to apply this event to the owner
      //      BPlayerID playerID = static_cast<BPlayerID>(data);
      //      if (playerID == cInvalidPlayerID)
      //         return;

      //      BStatsPlayer* pStatsPlayer = mPlayers[playerID];
      //      if (!pStatsPlayer)
      //         return;

      //      BEntity* pEntity = gWorld->getEntity(senderID);
      //      if (!pEntity)
      //         return;

      //      BSquad* pSquad = gWorld->getSquad(senderID);
      //      if (!pSquad)
      //         return;

      //      BProtoObject* pProtoObject = const_cast<BProtoObject*>(pSquad->getProtoObject());
      //      if (!pProtoObject)
      //         return;

      //      BProtoSquad* pProtoSquad = const_cast<BProtoSquad*>(pSquad->getProtoSquad());
      //      if (!pProtoSquad)
      //         return;

      //      pStatsPlayer->eventBuilt(pProtoObject, pProtoSquad);

      //      break;
      //   }

      case BEntity::cEventBuilt:
         {
            // track the person who built it
            // senderID - EntityID of the item built
            // data - BPlayerID, needs cast
            // only need to apply this event to the owner
            BPlayerID playerID = static_cast<BPlayerID>(data);
            if (playerID != cInvalidPlayerID)
            {
               BStatsPlayer* pStatsPlayer = mPlayers[playerID];
               if (!pStatsPlayer)
                  return;

               BEntity* pEntity = gWorld->getEntity(senderID);
               if (!pEntity)
                  return;

               pStatsPlayer->eventBuilt(pEntity, gameTime);
            }
         }
         break;

      case BEntity::cEventSquadAbility:
         {
            // data == BPlayerID
            // data2 == ability ID
            BPlayerID playerID = static_cast<BPlayerID>(data);
            if (playerID != cInvalidPlayerID)
            {
               BStatsPlayer* pStatsPlayer = mPlayers[playerID];
               if (!pStatsPlayer)
                  return;

               long abilityID = static_cast<long>(data2);
               if (abilityID < 0)
                  return;

               const BEntity* pEntity = gWorld->getEntity(senderID);
               if (!pEntity)
                  return;

               const BSquad* pSquad = pEntity->getSquad();
               if (pSquad == NULL)
                  return;

               pStatsPlayer->eventAbility(gDatabase.getSquadAbilityID(pSquad, abilityID));
            }

            break;
         }

      //case BEntity::cEventKilled:
      //   {
      //      // track the person who lost it and the person who killed it
      //      // senderID - unit killed
      //      // data - EntityID of the killer
      //      // find the player that owns the killed unit
      //      BEntity *pEntity = gWorld->getEntity(senderID);
      //      if (!pEntity)
      //         return;
      //      BUnit* pUnit = pEntity->getUnit();
      //      if (!pUnit)
      //         return;

      //      BEntityID killerID = BEntityID(data);
      //      BEntity *pKillerEntity = gWorld->getEntity(killerID);

      //      BPlayerID playerID = pEntity->getPlayerID();
      //      if (playerID != cInvalidPlayerID)
      //      {
      //         BStatsPlayer* pPlayer = mPlayers[playerID];
      //         if (pPlayer)
      //            pPlayer->eventLost(pUnit, pKillerEntity);
      //      }

      //      if (pKillerEntity)
      //      {
      //         BPlayerID killerPlayerID = pKillerEntity->getPlayerID();
      //         if (killerPlayerID != cInvalidPlayerID)
      //         {
      //            BStatsPlayer* pPlayer = mPlayers[killerPlayerID];
      //            if (pPlayer)
      //               pPlayer->eventDestroyed(pUnit, pKillerEntity); // this player has destroyed pUnit with pKillerEntity
      //         }
      //      }
      //   }
      //   break;

      case BEntity::cEventPlayerResigned:
      case BEntity::cEventPlayerDefeated:
      case BEntity::cEventPlayerDisconnected:
      case BEntity::cEventPlayerWon:
         {
            BPlayerID playerID = static_cast<BPlayerID>(data);
            if (playerID != cInvalidPlayerID || playerID >= mPlayers.getNumber())
            {
               BStatsPlayer* pPlayer = mPlayers[playerID];
               if (!pPlayer)
                  return;

               pPlayer->stateChanged();
            }
         }
         break;
   }
}

//============================================================================
// 
//============================================================================
void BStatsManager::update()
{
   if (mDisabled)
      return;

   // all stats are recorded based on game time and not real time
   DWORD now = gWorld->getGametime();

   uint count = mGraphs.getSize();
   for (uint i=0; i < count; ++i)
   {
      BStatRecorderGraph* pGraph = mGraphs[i];

      if (!pGraph)
         continue;

      pGraph->update(now);
   }

   int32 maxStrength = 0;

   count = mPlayers.getSize();
   for (uint i=1; i < count; ++i)
   {
//-- FIXING PREFIX BUG ID 3070
      const BStatsPlayer* pPlayer = mPlayers[i];
//--

      if (!pPlayer)
         continue;

      maxStrength = max(maxStrength, pPlayer->getStrength());
   }

   for (uint i=1; i < count; ++i)
   {
      BStatsPlayer* pPlayer = mPlayers[i];

      if (!pPlayer)
         continue;

      pPlayer->update(now, maxStrength);
   }
}

//==============================================================================
// 
//==============================================================================
void BStatsManager::setGameType(BStats::BGameType gameType)
{
   mGameType = gameType;
}

//==============================================================================
// 
//==============================================================================
void BStatsManager::setPlayerID(uint clientID, BPlayerID playerID)
{
   if (clientID >= XNetwork::cMaxClients)
      return;

   mClientToPlayerMap[clientID] = playerID;
}

//==============================================================================
// 
//==============================================================================
void BStatsManager::setDisconnect(uint clientID)
{
   if (clientID >= XNetwork::cMaxClients)
      return;

   mClientDisconnect[clientID] = TRUE;
}

//==============================================================================
// 
//==============================================================================
void BStatsManager::setOoS(uint fromClientID, uint withClientID, long checksumID)
{
   BOoS oos(fromClientID, withClientID, checksumID);
   mOoSReports.add(oos);
}

//==============================================================================
// 
//==============================================================================
#ifndef BUILD_FINAL
uint32 BStatsManager::getMemUsage()
{
   if (!mMemDirty)
      return mMemUsage;

   // accumulate all the memory usage from the statsmanager and players
   mMemUsage = sizeof(BStatsManager);
   mMemUsage += mPlayers.getSizeInBytes();
   mMemUsage += mStatDefinitions.getSizeInBytes();
   mMemUsage += (mStatDefinitions.getSize() * sizeof(BStatDefinition));
   mMemUsage += mGraphs.getSizeInBytes();
   mMemUsage += mGameID.maxBytes();
   mMemUsage += mMapName.maxBytes();
   mMemUsage += mGamertag.maxBytes();

   // loop through all the players and do the same
   uint count = mPlayers.getSize();
   for (uint i=0; i < count; ++i)
   {
      BStatsPlayer* pPlayer = mPlayers[i];

      if (!pPlayer)
         continue;

      mMemUsage += pPlayer->getMemUsage();
   }

   mMemDirty = false;

   return mMemUsage;
}
#endif

//==============================================================================
// 
//==============================================================================
bool BStatsManager::load()
{
   BXMLReader reader;
   BXMLNode rootNode;

   if (!reader.load(cDirData, "stats.xml"))
      return false;

   rootNode = reader.getRootNode();
   long count = rootNode.getNumberChildren();

   mStatDefinitions.reserve(count);

   for (long i=0; i<count; i++)
   {
      BXMLNode node(rootNode.getChild(i));

      BStatDefinition* pDef = NULL;

      BStatType type = eUnknown;

      long statID=0;
      node.getAttribValueAsLong("id", statID);

      // need to instantiate the appropriate recorder for the definition
      if (node.getName().compare("Total") == 0)
         type = eTotal;
      else if (node.getName().compare("Event") == 0)
         type = eEvent;
      else if (node.getName().compare("Graph") == 0)
         type = eGraph;
      else if (node.getName().compare("ResourceGraph") == 0)
         type = eResourceGraph;
      else if (node.getName().compare("PopGraph") == 0)
         type = ePopGraph;
      else if (node.getName().compare("BaseGraph") == 0)
         type = eBaseGraph;
      else if (node.getName().compare("ScoreGraph") == 0)
         type = eScoreGraph;
      else if (node.getName().compare("ObjectMap") == 0)
      {
         if (mpObjectIDMap == NULL)
            mpObjectIDMap = new BStatIDHashMap();
         loadMap(*mpObjectIDMap, node);
         continue;
      }
      else if (node.getName().compare("SquadMap") == 0)
      {
         if (mpSquadIDMap == NULL)
            mpSquadIDMap = new BStatIDHashMap();
         loadMap(*mpSquadIDMap, node);
         continue;
      }

      pDef = BStatDefinition::getInstance();
      BDEBUG_ASSERT(pDef);

      pDef->init(statID, type);

      loadDefinition(*pDef, node);

      mStatDefinitions.add(pDef);
   }

#ifndef BUILD_FINAL
   mMemDirty = true;
#endif

   return true;
}

//============================================================================
// 
//============================================================================
void BStatsManager::loadMap(BStatIDHashMap& map, BXMLNode& root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      BSimString tempStr;
      BPackedString nodeText(node.getTextPtr(tempStr));

      if (name == "Object")
      {
         BProtoObjectID fromID = -1;
         BProtoObjectID toID = -1;

         BSimString data;
         if (node.getAttribValueAsString("from", data))
         {
            fromID = gDatabase.getProtoObject(data);
         }

         if (node.getAttribValueAsString("to", data))
         {
            toID = gDatabase.getProtoObject(data);
         }

         if (fromID != -1 && toID != -1)
            map.insert(fromID, toID);
      }
      else if (name == "Squad")
      {
         BProtoSquadID fromID = -1;
         BProtoSquadID toID = -1;

         BSimString data;
         if (node.getAttribValueAsString("from", data))
         {
            fromID = gDatabase.getProtoSquad(data);
         }

         if (node.getAttribValueAsString("to", data))
         {
            toID = gDatabase.getProtoSquad(data);
         }

         if (fromID != -1 && toID != -1)
            map.insert(fromID, toID);
      }
   }
}

//============================================================================
// 
//============================================================================
void BStatsManager::loadDefinition(BStatDefinition& statDef, BXMLNode& root)
{
   // get the name of this node
   root.getAttribValueAsString("name", statDef.mName);
   bool upload = true;
   if (root.getAttribValueAsBool("upload", upload))
      statDef.mUpload = upload;

   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      BSimString tempStr;
      BPackedString nodeText(node.getTextPtr(tempStr));

      if (name == "EventType")
      {
         if (nodeText == "Researched")
            statDef.mEventResearched = true;
         else if (nodeText == "Built")
            statDef.mEventBuilt = true;
         else if (nodeText == "Lost")
            statDef.mEventLost = true;
         else if (nodeText == "Destroyed")
         {
            BSimString track;
            if (node.getAttribValueAsString("t", track))
            {
               if (track == "target")
                  statDef.mEventDestroyedTarget = true;
            }
            statDef.mEventDestroyed = true;
         }
         else if (nodeText == "Combat")
            statDef.mEventCombat = true;
      }
      else if (name == "ObjectClass")
      {
         statDef.mObjectClass = gDatabase.getObjectClass(nodeText);
         statDef.mFilterObjectClass = true;
      }
      else if (name == "ObjectType")
      {
         BOOL AND = FALSE;
         BOOL OR = FALSE;
         BOOL NOT = FALSE;

         BSimString conditional;
         if (node.getAttribValueAsString("conditional", conditional))
         {
            if (conditional == "OR")
               OR = TRUE;
            else if (conditional == "NOT")
               NOT = TRUE;
            else if (conditional == "AND")
               AND = TRUE;
         }
         else
         {
            // conditionals are AND by default
            AND = TRUE;
         }

         long type = gDatabase.getObjectType(nodeText);
         long abstractType = type - gDatabase.getNumberBaseObjectTypes();
         if (abstractType >= 0 && abstractType < statDef.mAbstractTypesAND.getNumber())
         {
            if (AND)
            {
               statDef.mAbstractTypesAND.setBit(abstractType);
               statDef.mFilterObjectType = true;
               statDef.mFilterObjectTypeAND = true;
            }
            else if (OR)
            {
               statDef.mAbstractTypesOR.setBit(abstractType);
               statDef.mFilterObjectType = true;
               statDef.mFilterObjectTypeOR = true;
            }
            else if (NOT)
            {
               statDef.mAbstractTypesNOT.setBit(abstractType);
               statDef.mFilterObjectType = true;
               statDef.mFilterObjectTypeNOT = true;
            }
         }
      }
      else if (name == "DBID")
      {
         if (node.getTextAsLong(statDef.mDBID))
            statDef.mFilterDBID = true;
      }
      else if (name == "TrackSquads")
      {
         statDef.mTrackSquads = true;
      }
      else if (name == "TrackKillers")
      {
         statDef.mTrackKillers = true;
         statDef.mpKillers = BStatDefinition::getInstance();
         loadDefinition(*statDef.mpKillers, node);
      }
      else if (name == "TrackIndividual")
      {
         statDef.mTrackIndividual = true;
      }
      else if (name == "TrackFirstLastGameTime")
      {
         statDef.mTrackFirstLastGameTime = true;
      }
      else if (name == "TrackTimeZero")
      {
         statDef.mTrackTimeZero = true;
      }
      else if (name == "Tech")
      {
         // read the name and check for flags
         BSimString name;
         long techID = -1;
         if (node.getAttribValueAsString("name", name))
         {
            techID = gDatabase.getProtoTech(name.getPtr());
            if (techID == -1)
               continue;
         }

         bool exclude = false;
         long count = node.getNumberChildren();
         for (long i=0; i < count; ++i)
         {
            BXMLNode childNode(node.getChild(i));

            BPackedString childNodeText(childNode.getTextPtr(tempStr));
            if (childNodeText == "Exclude")
               exclude = true;
         }

         if (techID != -1)
         {
            if (exclude)
            {
               statDef.mFilterExcludeTechs = true;
               statDef.mTechsExclude.add(techID);
            }
            else
               statDef.mTechs.add(techID);
         }
         else
         {
            // having a blank Tech tag indicates that we wish to track all techs researched
            statDef.mTrackAllTechs = true;
         }
      }
      else if (name == "Samples")
      {
         long size = 0;
         if (node.getTextAsLong(size))
            statDef.mSamples = static_cast<uint16>(size);
      }
      else if (name == "Interval")
      {
         long interval = 0;
         if (node.getTextAsLong(interval))
            statDef.mInterval = static_cast<uint16>(interval);
      }
   }
}

//============================================================================
//============================================================================
bool BStatsManager::save(BStream* pStream, int saveType) const
{
   //XNADDR mXnAddr;
   //BPlayerID mClientToPlayerMap[XNetwork::cMaxClients];
   //BOOL mClientDisconnect[XNetwork::cMaxClients];

   // mPlayers
   int count = mPlayers.getNumber();
   GFWRITEVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 16);
   for (int i=0; i<count; i++)
   {
      if (mPlayers[i])
      {
         GFWRITEVAL(pStream, int8, i);
         GFWRITECLASSPTR(pStream, saveType, mPlayers[i]);
      }
   }
   GFWRITEVAL(pStream, int8, -1);

   //BSmallDynamicSimArray<BStatDefinition*> mStatDefinitions;
   //BSmallDynamicSimArray<BStatRecorderGraph*> mGraphs;
   //BSmallDynamicSimArray<BOoS> mOoSReports;
   //BSimString mGameID;
   //BSimString mMapName;
   //BSimString mGamertag;
   //XUID mXuid;
   //DWORD mControllerID;
   //DWORD mXNetGetTitleXnAddrFlags;
   //BStatIDHashMap* mpObjectIDMap;
   //BStatIDHashMap* mpSquadIDMap;
   //BStats::BGameType mGameType;
   //bool mDisabled : 1;
   //bool mLiveGame : 1;

   GFWRITEMARKER(pStream, cSaveMarkerStatsManager);
   return true;
}

//============================================================================
//============================================================================
bool BStatsManager::load(BStream* pStream, int saveType)
{
   //XNADDR mXnAddr;
   //BPlayerID mClientToPlayerMap[XNetwork::cMaxClients];
   //BOOL mClientDisconnect[XNetwork::cMaxClients];

   // mPlayers
   int count;
   GFREADVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 16);
   if (count != mPlayers.getNumber())
      return false;
   for (;;)
   {
      int index;
      GFREADVAL(pStream, int8, int, index);
      if (index == -1)
         break;
      GFVERIFYCOUNT(index, count-1);
      if (!mPlayers[index])
         return false;
      GFREADCLASSPTR(pStream, saveType, mPlayers[index]);
   }

   //BSmallDynamicSimArray<BStatDefinition*> mStatDefinitions;
   //BSmallDynamicSimArray<BStatRecorderGraph*> mGraphs;
   //BSmallDynamicSimArray<BOoS> mOoSReports;
   //BSimString mGameID;
   //BSimString mMapName;
   //BSimString mGamertag;
   //XUID mXuid;
   //DWORD mControllerID;
   //DWORD mXNetGetTitleXnAddrFlags;
   //BStatIDHashMap* mpObjectIDMap;
   //BStatIDHashMap* mpSquadIDMap;
   //BStats::BGameType mGameType;
   //bool mDisabled : 1;
   //bool mLiveGame : 1;

   GFREADMARKER(pStream, cSaveMarkerStatsManager);
   return true;
}

//============================================================================
//============================================================================
bool BStatsPlayer::save(BStream* pStream, int saveType) const
{
   // mStatRecorders
   uint recCount = mStatRecorders.getSize();
   GFWRITEVAL(pStream, uint16, recCount);
   GFVERIFYCOUNT(recCount, 10000);
   for (uint i=0; i < recCount; ++i)
   {
      BStatRecorder* pRecorder = mStatRecorders[i];
      if (!pRecorder)
         continue;
      BStatType statType = pRecorder->getType();
      if (statType == eTotal || statType == eEvent)
      {
         GFWRITEVAL(pStream, uint16, i);
         GFWRITEVAL(pStream, uint8, statType);
         GFWRITECLASSPTR(pStream, saveType, pRecorder);
      }
   }
   GFWRITEVAL(pStream, uint16, UINT16_MAX);
   GFWRITEMARKER(pStream, cSaveMarkerStatsRecorders);

   //BSmallDynamicSimArray<BStatRecorder::BStatResources> mResourceGraph;
   //BSmallDynamicSimArray<BStatRecorder::BStatEventFloat> mPopGraph;
   //BSmallDynamicSimArray<BStatRecorder::BStatEventUInt32> mBaseGraph;
   //BSmallDynamicSimArray<BStatRecorder::BStatEventInt32> mScoreGraph;

   // mPowersUsed
   int count = mPowersUsed.size();
   GFWRITEVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   BStatProtoPowerIDHash* pPowersUsed = const_cast<BStatProtoPowerIDHash*>(&mPowersUsed);
   BStatProtoPowerIDHash::iterator powerIter = pPowersUsed->begin();
   BStatProtoPowerIDHash::iterator powerEndIter = pPowersUsed->end();
   for (powerIter; powerIter != powerEndIter; ++powerIter)
   {
      GFWRITEVAR(pStream, BProtoPowerID, powerIter->first);
      GFWRITEVAR(pStream, uint, powerIter->second);
   }
   GFWRITEMARKER(pStream, cSaveMarkerStatsPowers);

   // mAbilitiesUsed
   count = mAbilitiesUsed.size();
   GFWRITEVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   BStatAbilityIDHashMap* pAbilitiesUsed = const_cast<BStatAbilityIDHashMap*>(&mAbilitiesUsed);
   BStatAbilityIDHashMap::iterator abilityIter = pAbilitiesUsed->begin();
   BStatAbilityIDHashMap::iterator abilityEndIter = pAbilitiesUsed->end();
   for (abilityIter; abilityIter != abilityEndIter; ++abilityIter)
   {
      GFWRITEVAR(pStream, long, abilityIter->first);
      GFWRITEVAR(pStream, uint, abilityIter->second);
   }
   GFWRITEMARKER(pStream, cSaveMarkerStatsAbilities);

   //BStatGraphDefinition mResGraphDef;
   //BStatGraphDefinition mPopGraphDef;
   //BStatGraphDefinition mBaseGraphDef;
   //BStatGraphDefinition mScoreGraphDef;

   GFWRITECLASS(pStream, saveType, mTotalResources);
   GFWRITECLASS(pStream, saveType, mMaxResources);
   GFWRITECLASS(pStream, saveType, mGatheredResources);
   GFWRITECLASS(pStream, saveType, mTributedResources);
   //XUID mXuid;
   //BPlayer* mpPlayer;
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BTeamID, mTeamID);
   GFWRITEVAR(pStream, DWORD, mPlayerStateTime);
   //long mPopID;
   GFWRITEVAR(pStream, BPlayerState, mPlayerState);
   GFWRITEVAR(pStream, uint32, mStrengthTime);
   GFWRITEVAR(pStream, uint32, mStrengthTimer);
   GFWRITEVAR(pStream, int32, mCivID);
   GFWRITEVAR(pStream, int32, mLeaderID);
   GFWRITEVAR(pStream, uint8, mResourcesUsed);
   GFWRITEVAR(pStream, uint8, mPlayerType);
   GFWRITEBITBOOL(pStream, mRandomCiv);
   GFWRITEBITBOOL(pStream, mRandomLeader);
   GFWRITEBITBOOL(pStream, mResigned);
   GFWRITEBITBOOL(pStream, mDefeated);
   GFWRITEBITBOOL(pStream, mDisconnected);
   GFWRITEBITBOOL(pStream, mWon);

   GFWRITEMARKER(pStream, cSaveMarkerStatsPlayer);
   return true;
}

//============================================================================
//============================================================================
bool BStatsPlayer::load(BStream* pStream, int saveType)
{
   // mStatRecorders
   uint recCount;
   GFREADVAL(pStream, uint16, uint, recCount);
   GFVERIFYCOUNT(recCount, 10000);
   bool validLoad = (recCount == mStatRecorders.getSize());
   uint loaded = 0;
   for (;;)
   {
      uint index;
      GFREADVAL(pStream, uint16, uint, index);
      if (index == UINT16_MAX)
         break;
      GFVERIFYCOUNT(index, recCount-1);
      BStatType statType;
      GFREADVAL(pStream, uint8, BStatType, statType);
      loaded++;
      GFVERIFYCOUNT(loaded, recCount);
      if (validLoad)
      {
         BStatRecorder* pRecorder = mStatRecorders[index];
         if (pRecorder && pRecorder->getType() == statType)
            GFREADCLASSPTR(pStream, saveType, pRecorder)
         else
            validLoad = false;
      }
      if (!validLoad)
      {
         switch (statType)
         {
            case eTotal: GFREADTEMPCLASS(pStream, saveType, BStatRecorderTotal); break;
            case eEvent: GFREADTEMPCLASS(pStream, saveType, BStatRecorderEvent); break;
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerStatsRecorders);

   //BSmallDynamicSimArray<BStatRecorder::BStatResources> mResourceGraph;
   //BSmallDynamicSimArray<BStatRecorder::BStatEventFloat> mPopGraph;
   //BSmallDynamicSimArray<BStatRecorder::BStatEventUInt32> mBaseGraph;
   //BSmallDynamicSimArray<BStatRecorder::BStatEventInt32> mScoreGraph;

   // mPowersUsed
   int count;
   GFREADVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   for (int i=0; i<count; i++)
   {
      BProtoPowerID id;
      uint index;
      GFREADVAR(pStream, BProtoPowerID, id);
      GFREADVAR(pStream, uint, index);
      gSaveGame.remapProtoPowerID(id);
      mPowersUsed.insert(id, index);
   }
   GFREADMARKER(pStream, cSaveMarkerStatsPowers);

   // mAbilitiesUsed
   GFREADVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   for (int i=0; i<count; i++)
   {
      long id;
      uint index;
      GFREADVAR(pStream, long, id);
      GFREADVAR(pStream, uint, index);
      gSaveGame.remapAbilityID(id);
      mAbilitiesUsed.insert(id, index);
   }
   GFREADMARKER(pStream, cSaveMarkerStatsAbilities);

   //BStatGraphDefinition mResGraphDef;
   //BStatGraphDefinition mPopGraphDef;
   //BStatGraphDefinition mBaseGraphDef;
   //BStatGraphDefinition mScoreGraphDef;

   GFREADCLASS(pStream, saveType, mTotalResources);
   GFREADCLASS(pStream, saveType, mMaxResources);
   GFREADCLASS(pStream, saveType, mGatheredResources);
   GFREADCLASS(pStream, saveType, mTributedResources);
   //XUID mXuid;
   //BPlayer* mpPlayer;
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BTeamID, mTeamID);
   GFREADVAR(pStream, DWORD, mPlayerStateTime);
   //long mPopID;
   GFREADVAR(pStream, BPlayerState, mPlayerState);
   GFREADVAR(pStream, uint32, mStrengthTime);
   GFREADVAR(pStream, uint32, mStrengthTimer);
   GFREADVAR(pStream, int32, mCivID);
   GFREADVAR(pStream, int32, mLeaderID);
   GFREADVAR(pStream, uint8, mResourcesUsed);
   GFREADVAR(pStream, uint8, mPlayerType);
   GFREADBITBOOL(pStream, mRandomCiv);
   GFREADBITBOOL(pStream, mRandomLeader);
   GFREADBITBOOL(pStream, mResigned);
   GFREADBITBOOL(pStream, mDefeated);
   GFREADBITBOOL(pStream, mDisconnected);
   GFREADBITBOOL(pStream, mWon);

   gSaveGame.remapCivID(mCivID);
   gSaveGame.remapLeaderID(mLeaderID);

   GFREADMARKER(pStream, cSaveMarkerStatsPlayer);
   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::save(BStream* pStream, int saveType) const
{
   // mKillers;
   uint killerCount = mKillers.getSize();
   GFWRITEVAL(pStream, uint16, killerCount);
   GFVERIFYCOUNT(killerCount, 10000);
   for (uint i=0; i < killerCount; ++i)
   {
      BStatProtoIDHashMap* pKiller = mKillers[i];
      if (!pKiller)
         continue;
      GFWRITEVAL(pStream, uint16, i);
      int count = pKiller->size();
      GFWRITEVAL(pStream, int16, count);
      GFVERIFYCOUNT(count, 10000);
      BStatProtoIDHashMap::iterator killerIter = pKiller->begin();
      BStatProtoIDHashMap::iterator killerEndIter = pKiller->end();
      for (killerIter; killerIter != killerEndIter; ++killerIter)
      {
         GFWRITEVAR(pStream, long, killerIter->first);
         GFWRITEVAR(pStream, BStatLostDestroyed, killerIter->second);
      }

   }
   GFWRITEVAL(pStream, uint16, UINT16_MAX);

   GFWRITECLASSPTRARRAY(pStream, saveType, BStatCombat, mCombat, uint16, 10000);

   //BStatDefinition* mpDef;
   //BStatIDHashMap* mpObjectIDMap;
   //BStatIDHashMap* mpSquadIDMap;

   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::load(BStream* pStream, int saveType)
{
   // mKillers;
   uint killerCount;
   GFREADVAL(pStream, uint16, uint, killerCount);
   GFVERIFYCOUNT(killerCount, 10000);
   mKillers.setNumber(killerCount);
   for (uint i=0; i<killerCount; i++)
      mKillers[i] = NULL;
   for (;;)
   {
      uint index;
      GFREADVAL(pStream, uint16, uint, index);
      if (index == UINT16_MAX)
         break;
      GFVERIFYCOUNT(index, killerCount-1);
      BStatProtoIDHashMap* pKillers = new BStatProtoIDHashMap();
      mKillers[index] = pKillers;
      int count;
      GFREADVAL(pStream, int16, int, count);
      GFVERIFYCOUNT(count, 10000);
      for (int j=0; j<count; j++)
      {
         long id;
         BStatLostDestroyed val(0, 0);
         GFREADVAR(pStream, long, id);
         GFREADVAR(pStream, BStatLostDestroyed, val);
         if (mpDef && mpDef->mpKillers->mTrackSquads)
            gSaveGame.remapProtoSquadID(id);
         else
            gSaveGame.remapProtoObjectID(id);
         pKillers->insert(id, val);
      }
   }

   GFREADCLASSPTRARRAY(pStream, saveType, BStatCombat, mCombat, uint16, 10000);

   //BStatDefinition* mpDef;
   //BStatIDHashMap* mpObjectIDMap;
   //BStatIDHashMap* mpSquadIDMap;

   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::BStatCombat::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, long, mLevels, uint16, 10000);
   GFWRITEVAR(pStream, float, mXP);
   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::BStatCombat::load(BStream* pStream, int saveType)
{
   GFREADARRAY(pStream, long, mLevels, uint16, 10000);
   GFREADVAR(pStream, float, mXP);
   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::BStatTotal::save(BStream* pStream, int saveType) const
{
   GFWRITEPTR(pStream, sizeof(int32)*BStats::cMaxPlayers, mKillerIDs);
   GFWRITEVAR(pStream, uint32, mBuilt);
   GFWRITEVAR(pStream, uint32, mLost);
   GFWRITEVAR(pStream, uint32, mDestroyed);
   GFWRITEVAR(pStream, uint32, mMax);
   GFWRITEVAR(pStream, int32, mCombatID);
   GFWRITEVAR(pStream, uint32, mFirstTime);
   GFWRITEVAR(pStream, uint32, mLastTime);
   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::BStatTotal::load(BStream* pStream, int saveType, BStatDefinition* pDef)
{
   GFREADPTR(pStream, sizeof(int32)*BStats::cMaxPlayers, mKillerIDs);
   GFREADVAR(pStream, uint32, mBuilt);
   GFREADVAR(pStream, uint32, mLost);
   GFREADVAR(pStream, uint32, mDestroyed);
   GFREADVAR(pStream, uint32, mMax);
   GFREADVAR(pStream, int32, mCombatID);
   GFREADVAR(pStream, uint32, mFirstTime);
   GFREADVAR(pStream, uint32, mLastTime);
   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::BStatEvent::save(BStream* pStream, int saveType) const
{
   if (!BStatTotal::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, uint32, mTimestamp);
   GFWRITEVAR(pStream, long, mProtoID);
   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::BStatEvent::load(BStream* pStream, int saveType, BStatDefinition* pDef)
{
   if (!BStatTotal::load(pStream, saveType, pDef))
      return false;
   GFREADVAR(pStream, uint32, mTimestamp);
   GFREADVAR(pStream, long, mProtoID);
   if (pDef && pDef->mTrackAllTechs)
      gSaveGame.remapProtoTechID(mProtoID);
   else if (pDef && pDef->mTrackSquads)
      gSaveGame.remapProtoSquadID(mProtoID);
   else
      gSaveGame.remapProtoObjectID(mProtoID);
   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::BStatResources::save(BStream* pStream, int saveType) const
{
   GFWRITECLASS(pStream, saveType, mCost);
   GFWRITEVAR(pStream, uint32, mTimestamp);
   return true;
}

//============================================================================
//============================================================================
bool BStatRecorder::BStatResources::load(BStream* pStream, int saveType)
{
   GFREADCLASS(pStream, saveType, mCost);
   GFREADVAR(pStream, uint32, mTimestamp);
   return true;
}


//============================================================================
//============================================================================
bool BStatRecorderTotal::save(BStream* pStream, int saveType) const
{
   if (!BStatRecorder::save(pStream, saveType))
      return false;

   // mTotalsHash
   int count = mTotalsHash.size();
   GFWRITEVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   BStatTotalHashMap* pTotalHash = const_cast<BStatTotalHashMap*>(&mTotalsHash);
   BStatTotalHashMap::iterator totalIter = pTotalHash->begin();
   BStatTotalHashMap::iterator totalEndIter = pTotalHash->end();
   for (totalIter; totalIter != totalEndIter; ++totalIter)
   {
      GFWRITEVAR(pStream, long, totalIter->first);
      GFWRITECLASS(pStream, saveType, totalIter->second);
   }

   GFWRITECLASS(pStream, saveType, mTotal);

   GFWRITEVAL(pStream, bool, (mpCombat != NULL));
   if (mpCombat)
      GFWRITECLASSPTR(pStream, saveType, mpCombat);

   return true;
}

//============================================================================
//============================================================================
bool BStatRecorderTotal::load(BStream* pStream, int saveType)
{
   if (!BStatRecorder::load(pStream, saveType))
      return false;

   // mTotalsHash
   int count;
   GFREADVAR(pStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   for (int i=0; i<count; i++)
   {
      long id;
      BStatTotal val;
      GFREADVAR(pStream, long, id);
      if (!val.load(pStream, saveType, mpDef))
         return false;
      if (mpDef && mpDef->mTrackAllTechs)
         gSaveGame.remapProtoTechID(id);
      else if (mpDef && mpDef->mTrackSquads)
         gSaveGame.remapProtoSquadID(id);
      else
         gSaveGame.remapProtoObjectID(id);
      mTotalsHash.insert(id, val);
   }

   if (!mTotal.load(pStream, saveType, mpDef))
      return false;
   
   bool combat;
   GFREADVAR(pStream, bool, combat);
   if (combat)
   {
      if (mpCombat)
         GFREADCLASSPTR(pStream, saveType, mpCombat)
      else
         GFREADTEMPCLASS(pStream, saveType, BStatCombat)
   }

   return true;
}

//============================================================================
//============================================================================
bool BStatRecorderEvent::save(BStream* pStream, int saveType) const
{
   if (!BStatRecorder::save(pStream, saveType))
      return false;

   GFWRITECLASS(pStream, saveType, mTotal);
   GFWRITECLASSARRAY(pStream, saveType, mEvents, uint16, 10000);

   return true;
}

//============================================================================
//============================================================================
bool BStatRecorderEvent::load(BStream* pStream, int saveType)
{
   if (!BStatRecorder::load(pStream, saveType))
      return false;

   if (!mTotal.load(pStream, saveType, mpDef))
      return false;

   uint16 itemCount;
   GFREADVAR(pStream, uint16, itemCount);
   GFVERIFYCOUNT(itemCount, 10000);
   if (!mEvents.setNumber(itemCount))
      return false;
   for (uint16 i=0; i<itemCount; i++)
   {
      if (!mEvents[i].load(pStream, saveType, mpDef))
         return false;
   }

   return true;
}
