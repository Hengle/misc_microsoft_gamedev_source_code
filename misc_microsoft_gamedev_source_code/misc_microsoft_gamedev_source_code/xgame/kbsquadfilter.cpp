//==============================================================================
// kbunitfilter.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

//xgame
#include "common.h"
#include "aidebug.h"
#include "database.h"
#include "kb.h"
#include "kbsquadfilter.h"
#include "player.h"
#include "protoobject.h"
#include "protosquad.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BKBSquadFilterCurrentlyVisible, 4, &gSimHeap);
IMPLEMENT_FREELIST(BKBSquadFilterInList, 4, &gSimHeap);
IMPLEMENT_FREELIST(BKBSquadFilterPlayers, 4, &gSimHeap);
IMPLEMENT_FREELIST(BKBSquadFilterObjectTypes, 4, &gSimHeap);
IMPLEMENT_FREELIST(BKBSquadFilterPlayerRelation, 4, &gSimHeap);
IMPLEMENT_FREELIST(BKBSquadFilterMinStaleness, 4, &gSimHeap);
IMPLEMENT_FREELIST(BKBSquadFilterMaxStaleness, 4, &gSimHeap);

//==============================================================================
//==============================================================================
GFIMPLEMENTVERSION(BKBSquadFilter, 1);
GFIMPLEMENTVERSION(BKBSquadFilterSet, 1);

//==============================================================================
//==============================================================================
BKBSquadFilter* BKBSquadFilter::allocateFilter(BYTE filterType)
{
   switch(filterType)
   {
      case cFilterTypeCurrentlyVisible: { return BKBSquadFilterCurrentlyVisible::getInstance(); }
      case cFilterTypeObjectTypes: { return BKBSquadFilterObjectTypes::getInstance(); }
      case cFilterTypePlayers: { return BKBSquadFilterPlayers::getInstance(); }
      case cFilterTypeInList: { return BKBSquadFilterInList::getInstance(); }
      case cFilterTypePlayerRelation: { return BKBSquadFilterPlayerRelation::getInstance(); }
      case cFilterTypeMinStaleness: { return BKBSquadFilterMinStaleness::getInstance(); }
      case cFilterTypeMaxStaleness: { return BKBSquadFilterMaxStaleness::getInstance(); }
      default: BASSERTM(false, "You are trying to allocate an instance of an unknown kbunit filter type.");
   }
   return NULL;
}


//==============================================================================
//==============================================================================
void BKBSquadFilter::releaseFilter(BKBSquadFilter *pKBSquadFilter)
{
   BASSERT(pKBSquadFilter);
   BYTE filterType = pKBSquadFilter->getType();
   switch(filterType)
   {
      case cFilterTypeCurrentlyVisible: { BKBSquadFilterCurrentlyVisible::releaseInstance(reinterpret_cast<BKBSquadFilterCurrentlyVisible*>(pKBSquadFilter)); break; }
      case cFilterTypeObjectTypes: { BKBSquadFilterObjectTypes::releaseInstance(reinterpret_cast<BKBSquadFilterObjectTypes*>(pKBSquadFilter)); break; }
      case cFilterTypePlayers: { BKBSquadFilterPlayers::releaseInstance(reinterpret_cast<BKBSquadFilterPlayers*>(pKBSquadFilter)); break; }
      case cFilterTypeInList: { BKBSquadFilterInList::releaseInstance(reinterpret_cast<BKBSquadFilterInList*>(pKBSquadFilter)); break; }
      case cFilterTypePlayerRelation: { BKBSquadFilterPlayerRelation::releaseInstance(reinterpret_cast<BKBSquadFilterPlayerRelation*>(pKBSquadFilter)); break; }
      case cFilterTypeMinStaleness: { BKBSquadFilterMinStaleness::releaseInstance(reinterpret_cast<BKBSquadFilterMinStaleness*>(pKBSquadFilter)); break; }
      case cFilterTypeMaxStaleness: { BKBSquadFilterMaxStaleness::releaseInstance(reinterpret_cast<BKBSquadFilterMaxStaleness*>(pKBSquadFilter)); break; }
      default: BASSERTM(false, "You are trying to release an instance of an unknown kbunit filter type.  Botched.");
   }
}

//==============================================================================
//==============================================================================
bool BKBSquadFilterCurrentlyVisible::testKBSquad(BKBSquad* pKBSquad) const
{
   BASSERT(pKBSquad);
   bool testResult = pKBSquad->getCurrentlyVisible();
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}


//==============================================================================
//==============================================================================
bool BKBSquadFilterInList::testKBSquad(BKBSquad *pKBSquad) const
{
   BASSERT(pKBSquad);
   bool testResult = (mKBSquadList.find(pKBSquad->getID()) != cInvalidIndex);
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}


//==============================================================================
//==============================================================================
bool BKBSquadFilterPlayers::testKBSquad(BKBSquad *pKBSquad) const
{
   BASSERT(pKBSquad);
   bool testResult = (mPlayers.find(pKBSquad->getPlayerID()) != cInvalidIndex);
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}


//==============================================================================
//==============================================================================
bool BKBSquadFilterObjectTypes::testKBSquad(BKBSquad *pKBSquad) const
{
   //APF 8/30/07  Not a direct translation from units, but I would like to make this work somehow 
   //sub units all match, or one sub unit matches?

   //BASSERT(pKBSquad);
   //bool testResult = false;

   //BPlayer* pPlayer = gWorld->getPlayer(pKBSquad->getPlayerID());
   //if (!pPlayer)
   //   return (false);
//-- FIXING PREFIX BUG ID 4869
   //const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pKBSquad->getProtoSquadID());
//--
   //if (!pProtoSquad)
   //   return (false);

   //long numObjectTypes = mObjectTypes.getNumber();
   //for (long i=0; i<numObjectTypes; i++)
   //{
   //   if (pProtoSquad->isType(mObjectTypes[i]))
   //   {
   //      testResult = true;
   //      break;
   //   }
   //}
   //if (getIsInverted())
   //   testResult = !testResult;
   //return(testResult);



   BASSERT(pKBSquad);
   bool testResult = false;

   BPlayer* pPlayer = gWorld->getPlayer(pKBSquad->getPlayerID());
   if (!pPlayer)
      return (false);
   BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pKBSquad->getProtoSquadID());
   if (!pProtoSquad)
      return (false);

   long numObjectTypes = mObjectTypes.getNumber();
   for (long i=0; i<numObjectTypes; i++)
   {
      bool allChildrenQualify = true;

      long numUnitNodes = pProtoSquad->getNumberUnitNodes();
      for (long j=0; j<numUnitNodes; j++)
      {
         const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(j);
         const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
         if (pProtoObject && pProtoObject->isType(mObjectTypes[i]))
         {
            //good
         }
         else
         {
            allChildrenQualify = false;
         }
      }

      testResult = allChildrenQualify;

      if(testResult == true)
      {
         break;
      }
      //if(getIsInverted() && testResult = false)
      //{
      //   break;
      //}
   }
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);



   return true;
}


//==============================================================================
//==============================================================================
bool BKBSquadFilterMinStaleness::testKBSquad(BKBSquad *pKBSquad) const
{
   BASSERT(pKBSquad);
   bool testResult = pKBSquad->getStaleness() >= mMinStaleness;
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}


//==============================================================================
//==============================================================================
bool BKBSquadFilterMaxStaleness::testKBSquad(BKBSquad *pKBSquad) const
{
   BASSERT(pKBSquad);
   bool testResult = pKBSquad->getStaleness() <= mMaxStaleness;
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}


//==============================================================================
//==============================================================================
void BKBSquadFilterPlayerRelation::onAcquire()
{
   mRelationType = cRelationTypeAny;
   mbSelfAsAlly = false;
   setIsInverted(false);
}


//==============================================================================
//==============================================================================
bool BKBSquadFilterPlayerRelation::testKBSquad(BKBSquad* pKBSquad) const
{
   BASSERT(pKBSquad);
   bool testResult = false;

   if (mRelationType == cRelationTypeAny)
   {
      testResult = true;
   }
   else if (mRelationType == cRelationTypeSelf)
   {
      if (pKBSquad->getPlayerID() == mPlayerID)
         testResult = true;
   }
   else if (mRelationType == cRelationTypeAlly)
   {
      const BPlayer* pRelationPlayer = gWorld->getPlayer(mPlayerID);
      if (pRelationPlayer && pRelationPlayer->isAlly(pKBSquad->getPlayerID(), mbSelfAsAlly))
         testResult = true;
   }
   else if (mRelationType == cRelationTypeEnemy)
   {
      const BPlayer* pRelationPlayer = gWorld->getPlayer(mPlayerID);
      if (pRelationPlayer && pRelationPlayer->isEnemy(pKBSquad->getPlayerID()))
         testResult = true;
   }
   else if (mRelationType == cRelationTypeNeutral)
   {
      const BPlayer* pRelationPlayer = gWorld->getPlayer(mPlayerID);
      if (pRelationPlayer && pRelationPlayer->isNeutral(pKBSquad->getPlayerID()))
         testResult = true;
   }

   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}


//==============================================================================
//==============================================================================
void BKBSquadFilterSet::clearFilters(void)
{
   uint numKBSquadFilters = mKBSquadFilters.getSize();
   for (uint i=0; i<numKBSquadFilters; i++)
   {
      BKBSquadFilter::releaseFilter(mKBSquadFilters[i]);
   }
   mKBSquadFilters.setNumber(0);
}

//==============================================================================
//==============================================================================
void BKBSquadFilterSet::addKBSquadFilterCurrentlyVisible(bool invertFilter)
{
   BKBSquadFilterCurrentlyVisible *pFilter = BKBSquadFilterCurrentlyVisible::getInstance();
   pFilter->setIsInverted(invertFilter);
   mKBSquadFilters.add(pFilter);
}


//==============================================================================
//==============================================================================
void BKBSquadFilterSet::addKBSquadFilterInList(bool invertFilter, const BKBSquadIDArray &kbUnitList)
{
   BKBSquadFilterInList *pFilter = BKBSquadFilterInList::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setKBSquadList(kbUnitList);
   mKBSquadFilters.add(pFilter);
}


//==============================================================================
//==============================================================================
void BKBSquadFilterSet::addKBSquadFilterPlayers(bool invertFilter, const BPlayerIDArray &players)
{
   BKBSquadFilterPlayers *pFilter = BKBSquadFilterPlayers::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setPlayers(players);
   mKBSquadFilters.add(pFilter);
}

//==============================================================================
//==============================================================================
void BKBSquadFilterSet::addKBSquadFilterObjectTypes(bool invertFilter, const BObjectTypeIDArray &objectTypes)
{
   BKBSquadFilterObjectTypes *pFilter = BKBSquadFilterObjectTypes::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setObjectTypes(objectTypes);
   mKBSquadFilters.add(pFilter);
}

//==============================================================================
//==============================================================================
void BKBSquadFilterSet::addKBSquadFilterPlayerRelation(bool invertFilter, BPlayerID playerID, BRelationType relationType, bool selfAsAlly)
{
   BKBSquadFilterPlayerRelation* pFilter = BKBSquadFilterPlayerRelation::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setPlayerRelationFilterData(playerID, relationType, selfAsAlly);
   mKBSquadFilters.add(pFilter);
}


//==============================================================================
//==============================================================================
void BKBSquadFilterSet::addKBSquadFilterMinStaleness(bool invertFilter, DWORD minStaleness)
{
   BKBSquadFilterMinStaleness* pFilter = BKBSquadFilterMinStaleness::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setMinStaleness(minStaleness);
   mKBSquadFilters.add(pFilter);
}


//==============================================================================
//==============================================================================
void BKBSquadFilterSet::addKBSquadFilterMaxStaleness(bool invertFilter, DWORD maxStaleness)
{
   BKBSquadFilterMaxStaleness* pFilter = BKBSquadFilterMaxStaleness::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setMaxStaleness(maxStaleness);
   mKBSquadFilters.add(pFilter);
}



//==============================================================================
//==============================================================================
void BKBSquadFilterSet::copyFilterSet(const BKBSquadFilterSet *pSourceKBSquadFilterSet)
{
   if (!pSourceKBSquadFilterSet)
      return;
   if (pSourceKBSquadFilterSet == this)
      return;

   clearFilters();

   uint numSourceFilters = pSourceKBSquadFilterSet->mKBSquadFilters.getSize();
   for (uint i=0; i<numSourceFilters; i++)
   {
      BKBSquadFilter *pClonedFilter = pSourceKBSquadFilterSet->mKBSquadFilters[i]->clone();
      BASSERT(pClonedFilter);
      mKBSquadFilters.add(pClonedFilter);
   }
}


//==============================================================================
//==============================================================================
uint BKBSquadFilterSet::validateAndTestKBSquad(BKBSquadID kbSquadID) const
{
   BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadID);
   if (!pKBSquad)
      return (BEntityFilterSet::cFilterResultInvalid);

   bool testResult = testKBSquad(pKBSquad);
   if (testResult)
      return(BEntityFilterSet::cFilterResultPassed);
   else
      return(BEntityFilterSet::cFilterResultFailed);      
}

//==============================================================================
//==============================================================================
bool BKBSquadFilterSet::testKBSquad(BKBSquad *pKBSquad) const
{
   BASSERT(pKBSquad);
   uint numKBSquadFilters = mKBSquadFilters.getNumber();
   for (uint i=0; i<numKBSquadFilters; i++)
   {
      BKBSquadFilter *pKBSquadFilter = mKBSquadFilters[i];
      if (!pKBSquadFilter->testKBSquad(pKBSquad))
         return (false);
   }
   return(true);
}


//==============================================================================
//==============================================================================
void BKBSquadFilterSet::filterKBSquads(BKB* pKB, const BKBSquadIDArray &sourceKBSquadIDs, BKBSquadIDArray *pKBSquadsPassed, BKBSquadIDArray *pKBSquadsFailed, BKBSquadIDArray *pKBSquadsInvalid)
{
   // Do early bail and get some filter query information.
   if (!pKBSquadsPassed && !pKBSquadsFailed && !pKBSquadsInvalid)
      return;

   if (pKBSquadsPassed)
      pKBSquadsPassed->setNumber(0);
   if (pKBSquadsFailed)
      pKBSquadsFailed->setNumber(0);
   if (pKBSquadsInvalid)
      pKBSquadsInvalid->setNumber(0);

   uint numSourceKBSquadIDs = sourceKBSquadIDs.getSize();
   for (uint i=0; i<numSourceKBSquadIDs; i++)
   {
      BKBSquadID kbSquadID = sourceKBSquadIDs[i];

      uint filterResult = validateAndTestKBSquad(kbSquadID);
      if (filterResult == BKBSquadFilterSet::cFilterResultInvalid)
      {
         if (pKBSquadsInvalid)
            pKBSquadsInvalid->add(kbSquadID);
      }
      else if (filterResult == BKBSquadFilterSet::cFilterResultPassed)
      {
         if (pKBSquadsPassed)
            pKBSquadsPassed->add(kbSquadID);
      }
      else if (filterResult == BKBSquadFilterSet::cFilterResultFailed)
      {
         if (pKBSquadsFailed)
            pKBSquadsFailed->add(kbSquadID);
      }
   }
}

//==============================================================================
//==============================================================================
bool BKBSquadFilterSet::save(BStream* pStream, int saveType) const 
{ 
   uint16 count = (uint16)mKBSquadFilters.size();
   GFVERIFYCOUNT(count,1000);
   GFWRITEVAR(pStream,uint16,count);
   for (uint16 i=0; i<count; i++)
   {
      BYTE filterType = mKBSquadFilters[i]->getType();
      GFWRITEVAR(pStream,BYTE,filterType);
      GFWRITECLASSPTR(pStream, saveType, mKBSquadFilters[i]);
   }
   return true; 
}

//==============================================================================
//==============================================================================
bool BKBSquadFilterSet::load(BStream* pStream, int saveType) 
{ 
   clearFilters();
   uint16 count;
   GFREADVAR(pStream,uint16,count);
   GFVERIFYCOUNT(count,1000);
   mKBSquadFilters.reserve(count);
   for (uint16 i=0; i<count; i++)
   {
      BYTE filterType;
      GFREADVAR(pStream,BYTE,filterType);
      BKBSquadFilter* pFilter = BKBSquadFilter::allocateFilter(filterType);
      if (!pFilter)
         return false;
      mKBSquadFilters.add(pFilter);
      GFREADCLASSPTR(pStream, saveType, pFilter);
   }
   return true; 
}
