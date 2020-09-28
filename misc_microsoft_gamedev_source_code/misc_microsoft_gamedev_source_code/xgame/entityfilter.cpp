//==============================================================================
// entityfilter.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

//xgame
#include "common.h"

#include "entityfilter.h"
#include "database.h"
#include "protoobject.h"
#include "protosquad.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "user.h"
#include "selectionmanager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BEntityFilterIsAlive, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterIsIdle, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterInList, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterPlayers, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterTeams, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterProtoObjects, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterProtoSquads, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterObjectTypes, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterRefCount, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterRelationType, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterMaxObjectType, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterIsSelected, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterCanChangeOwner, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEntityFilterJacking, 4, &gSimHeap);

//==============================================================================
//==============================================================================
GFIMPLEMENTVERSION(BEntityFilter, 1);
GFIMPLEMENTVERSION(BEntityFilterSet, 1);

//==============================================================================
// BEntityFilter::allocateFilter
//==============================================================================
BEntityFilter* BEntityFilter::allocateFilter(BYTE filterType)
{
   switch(filterType)
   {
      case cFilterTypeIsAlive: { return BEntityFilterIsAlive::getInstance(); }
      case cFilterTypeIsIdle: { return BEntityFilterIsIdle::getInstance(); }
      case cFilterTypeInList: { return BEntityFilterInList::getInstance(); }
      case cFilterTypePlayers: { return BEntityFilterPlayers::getInstance(); }
      case cFilterTypeTeams: { return BEntityFilterTeams::getInstance(); }
      case cFilterTypeProtoObjects: { return BEntityFilterProtoObjects::getInstance(); }
      case cFilterTypeProtoSquads: { return BEntityFilterProtoSquads::getInstance(); }
      case cFilterTypeObjectTypes: { return BEntityFilterObjectTypes::getInstance(); }
      case cFilterTypeRefCount: { return BEntityFilterRefCount::getInstance(); }
      case cFilterTypeRelationType: { return BEntityFilterRelationType::getInstance(); }
      case cFilterTypeMaxObjectType: { return BEntityFilterMaxObjectType::getInstance(); }
      case cFilterTypeIsSelected: { return BEntityFilterIsSelected::getInstance(); }
      case cFilterTypeCanChangeOwner: { return BEntityFilterCanChangeOwner::getInstance(); }
      case cFilterTypeJacking: { return BEntityFilterJacking::getInstance(); }
      default: BASSERTM(false, "You are trying to allocate an instance of an unknown entity filter type.");
   }
   return NULL;
}

//==============================================================================
// BEntityFilter::releaseFilter
//==============================================================================
void BEntityFilter::releaseFilter(BEntityFilter *pEntityFilter)
{
   BASSERT(pEntityFilter);
   BYTE filterType = pEntityFilter->getType();
   switch(filterType)
   {
      case cFilterTypeIsAlive: { BEntityFilterIsAlive::releaseInstance(reinterpret_cast<BEntityFilterIsAlive*>(pEntityFilter)); break; }
      case cFilterTypeIsIdle: { BEntityFilterIsIdle::releaseInstance(reinterpret_cast<BEntityFilterIsIdle*>(pEntityFilter)); break; }
      case cFilterTypeInList: { BEntityFilterInList::releaseInstance(reinterpret_cast<BEntityFilterInList*>(pEntityFilter)); break; }
      case cFilterTypePlayers: { BEntityFilterPlayers::releaseInstance(reinterpret_cast<BEntityFilterPlayers*>(pEntityFilter)); break; }
      case cFilterTypeTeams: { BEntityFilterTeams::releaseInstance(reinterpret_cast<BEntityFilterTeams*>(pEntityFilter)); break; }
      case cFilterTypeProtoObjects: { BEntityFilterProtoObjects::releaseInstance(reinterpret_cast<BEntityFilterProtoObjects*>(pEntityFilter)); break; }
      case cFilterTypeProtoSquads: { BEntityFilterProtoSquads::releaseInstance(reinterpret_cast<BEntityFilterProtoSquads*>(pEntityFilter)); break; }
      case cFilterTypeObjectTypes: { BEntityFilterObjectTypes::releaseInstance(reinterpret_cast<BEntityFilterObjectTypes*>(pEntityFilter)); break; }
      case cFilterTypeRefCount: { BEntityFilterRefCount::releaseInstance(reinterpret_cast<BEntityFilterRefCount*>(pEntityFilter)); break; }
      case cFilterTypeRelationType: { BEntityFilterRelationType::releaseInstance(reinterpret_cast<BEntityFilterRelationType*>(pEntityFilter)); break; }
      case cFilterTypeMaxObjectType: { BEntityFilterMaxObjectType::releaseInstance(reinterpret_cast<BEntityFilterMaxObjectType*>(pEntityFilter)); break; }
      case cFilterTypeIsSelected: { BEntityFilterIsSelected::releaseInstance(reinterpret_cast<BEntityFilterIsSelected*>(pEntityFilter)); break; }
      case cFilterTypeCanChangeOwner: { BEntityFilterCanChangeOwner::releaseInstance(reinterpret_cast<BEntityFilterCanChangeOwner*>(pEntityFilter)); break; }
      case cFilterTypeJacking: { BEntityFilterJacking::releaseInstance(reinterpret_cast<BEntityFilterJacking*>(pEntityFilter)); break; }
      default: BASSERTM(false, "You are trying to release an instance of an unknown entity filter type.  Botched.");
   }
}


//==============================================================================
// Clear the entity filter list
//==============================================================================
void BEntityFilterSet::clearFilters()
{
   int numEntityFilters = mEntityFilters.getNumber();
   for (int i = (numEntityFilters - 1); i >= 0; i--)
   {
      BEntityFilter::releaseFilter(mEntityFilters[i]);
   }
   mEntityFilters.clear();
}


//==============================================================================
// BEntityFilterSet::addEntityFilterIsAlive
//==============================================================================
void BEntityFilterSet::addEntityFilterIsAlive(bool invertFilter)
{
   BEntityFilterIsAlive *pFilter = BEntityFilterIsAlive::getInstance();
   pFilter->setIsInverted(invertFilter);
   mEntityFilters.add(pFilter);
}

//==============================================================================
// BEntityFilterSet::addEntityFilterIsIdle
//==============================================================================
void BEntityFilterSet::addEntityFilterIsIdle(bool invertFilter)
{
   BEntityFilterIsIdle* pFilter = BEntityFilterIsIdle::getInstance();
   pFilter->setIsInverted(invertFilter);
   mEntityFilters.add(pFilter);
}

//==============================================================================
// BEntityFilterSet::addEntityFilterInList
//==============================================================================
void BEntityFilterSet::addEntityFilterInList(bool invertFilter, const BEntityIDArray &entityList)
{
   BEntityFilterInList *pFilter = BEntityFilterInList::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setEntityList(entityList);
   mEntityFilters.add(pFilter);
}


//==============================================================================
// BEntityFilterSet::addEntityFilterPlayers
//==============================================================================
void BEntityFilterSet::addEntityFilterPlayers(bool invertFilter, const BPlayerIDArray &players)
{
   BEntityFilterPlayers *pFilter = BEntityFilterPlayers::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setPlayers(players);
   mEntityFilters.add(pFilter);
}


//==============================================================================
// BEntityFilterSet::addEntityFilterTeams
//==============================================================================
void BEntityFilterSet::addEntityFilterTeams(bool invertFilter, const BTeamIDArray &teams)
{
   BEntityFilterTeams *pFilter = BEntityFilterTeams::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setTeams(teams);
   mEntityFilters.add(pFilter);
}


//==============================================================================
// BEntityFilterSet::addEntityFilterProtoObjects
//==============================================================================
void BEntityFilterSet::addEntityFilterProtoObjects(bool invertFilter, const BProtoObjectIDArray &protoObjects)
{
   BEntityFilterProtoObjects *pFilter = BEntityFilterProtoObjects::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setProtoObjects(protoObjects);
   mEntityFilters.add(pFilter);
}


//==============================================================================
// BEntityFilterSet::addEntityFilterProtoSquads
//==============================================================================
void BEntityFilterSet::addEntityFilterProtoSquads(bool invertFilter, const BProtoSquadIDArray &protoSquads)
{
   BEntityFilterProtoSquads *pFilter = BEntityFilterProtoSquads::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setProtoSquads(protoSquads);
   mEntityFilters.add(pFilter);
}


//==============================================================================
// BEntityFilterSet::addEntityFilterObjectTypes
//==============================================================================
void BEntityFilterSet::addEntityFilterObjectTypes(bool invertFilter, const BObjectTypeIDArray &objectTypes)
{
   BEntityFilterObjectTypes *pFilter = BEntityFilterObjectTypes::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setObjectTypes(objectTypes);
   mEntityFilters.add(pFilter);
}


//==============================================================================
// BEntityFilterSet::addEntityFilterRefCount
//==============================================================================
void BEntityFilterSet::addEntityFilterRefCount(bool invertFilter, long refCountType, long compareType, long count)
{
   BEntityFilterRefCount *pFilter = BEntityFilterRefCount::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setRefCountFilterData(refCountType, compareType, count);
   mEntityFilters.add(pFilter);
}

//==============================================================================
//==============================================================================
void BEntityFilterSet::addEntityFilterRelationType(bool invertFilter, BRelationType relationType, BTeamID teamID)
{
   BEntityFilterRelationType* pFilter = BEntityFilterRelationType::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setRelationTypeFilterData(relationType, teamID);
   mEntityFilters.add(pFilter);
}

//==============================================================================
//==============================================================================
void BEntityFilterSet::addEntityFilterMaxObjectType(bool invertFilter, BObjectTypeID objectTypeID, uint maxCount, bool applyToSquads /*= true*/, bool applyToUnits /*= false*/)
{
   BEntityFilterMaxObjectType* pFilter = BEntityFilterMaxObjectType::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setMaxObjectTypeFilterData(objectTypeID, maxCount, applyToSquads, applyToUnits);
   mEntityFilters.add(pFilter);
}

//==============================================================================
//==============================================================================
void BEntityFilterSet::addEntityFilterIsSelected(bool invertFilter, BPlayerID player)
{
   BEntityFilterIsSelected* pFilter = BEntityFilterIsSelected::getInstance();
   pFilter->setIsInverted(invertFilter);
   pFilter->setPlayerID(player);
   mEntityFilters.add(pFilter);
}

//==============================================================================
//==============================================================================
void BEntityFilterSet::addEntityFilterCanChangeOwner(bool invertFilter)
{
   BEntityFilterCanChangeOwner* pFilter = BEntityFilterCanChangeOwner::getInstance();
   pFilter->setIsInverted(invertFilter);
   mEntityFilters.add(pFilter);
}

//==============================================================================
//==============================================================================
void BEntityFilterSet::addEntityFilterJacking(bool invertFilter)
{
   BEntityFilterJacking *pFilter = BEntityFilterJacking::getInstance();
   pFilter->setIsInverted(invertFilter);
   mEntityFilters.add(pFilter);
}

//==============================================================================
// BEntityFilterSet::copyFilterSet
//==============================================================================
void BEntityFilterSet::copyFilterSet(const BEntityFilterSet *pSourceEntityFilterSet)
{
   if (!pSourceEntityFilterSet)
      return;
   if (pSourceEntityFilterSet == this)
      return;

   clearFilters();

   long numSourceFilters = pSourceEntityFilterSet->mEntityFilters.getNumber();
   for (long i=0; i<numSourceFilters; i++)
   {
      BEntityFilter *pClonedFilter = pSourceEntityFilterSet->mEntityFilters[i]->clone();
      BASSERT(pClonedFilter);
      mEntityFilters.add(pClonedFilter);
   }
}

//==============================================================================
// BEntityFilterIsAlive::testEntity
//==============================================================================
bool BEntityFilterIsAlive::testEntity(BEntity *pEntity) const
{
   BASSERT(pEntity);
   bool testResult = pEntity->isAlive();
   BUnit* pUnit = pEntity->getUnit();
   if (pUnit)
      testResult &= (!pUnit->getFlagDown() && !pUnit->isHibernating());
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterIsIdle::testEntity
//==============================================================================
bool BEntityFilterIsIdle::testEntity(BEntity* pEntity) const
{
   BASSERT(pEntity);
   bool testResult = pEntity->isIdle();
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterInList::testEntity
//==============================================================================
bool BEntityFilterInList::testEntity(BEntity *pEntity) const
{
   BASSERT(pEntity);
   bool testResult = (mEntityList.find(pEntity->getID()) != cInvalidIndex);
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterPlayers::testEntity
//==============================================================================
bool BEntityFilterPlayers::testEntity(BEntity *pEntity) const
{
   BASSERT(pEntity);
   bool testResult = (mPlayers.find(pEntity->getPlayerID()) != cInvalidIndex);
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterTeams::testEntity
//==============================================================================
bool BEntityFilterTeams::testEntity(BEntity *pEntity) const
{
   BASSERT(pEntity);
   bool testResult = (mTeams.find(pEntity->getTeamID()) != cInvalidIndex);
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterProtoObjects::testEntity
//==============================================================================
bool BEntityFilterProtoObjects::testEntity(BEntity *pEntity) const
{
   BASSERT(pEntity);
   bool testResult = false;

   BUnit* pUnit = pEntity->getUnit();
   if (pUnit)
   {
      long numProtoObjects = mProtoObjects.getNumber();
      for (long i=0; i<numProtoObjects; i++)
      {
         if (pUnit->isType(mProtoObjects[i]))
         {
            testResult = true;
            break;
         }
      }
   }
   else if (pEntity->getSquad())
   {
      testResult = true;
      BSquad* pSquad = pEntity->getSquad();
      long numChildren = pSquad->getNumberChildren();
      for (long i=0; i<numChildren; i++)
      {
         BUnit *pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (!pUnit)
            continue;

         bool childPassedFilter = false;
         long numProtoObjects = mProtoObjects.getNumber();
         for (long o=0; o<numProtoObjects; o++)
         {
            if (pUnit->isType(mProtoObjects[o]))
            {
               childPassedFilter = true;
               break;
            }
         }

         if (!childPassedFilter)
         {
            testResult = false;
            break;
         }
      }
   }

   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterProtoSquads::testEntity
//==============================================================================
bool BEntityFilterProtoSquads::testEntity(BEntity *pEntity) const
{
   BASSERT(pEntity);
   bool testResult = false;
   if (pEntity->getSquad())
      testResult = (mProtoSquads.find(pEntity->getSquad()->getProtoSquadID()) != cInvalidIndex);
   else if (pEntity->getUnit())
   {
      BSquad *pParentSquad = pEntity->getUnit()->getParentSquad();
      testResult = pParentSquad ? (mProtoSquads.find(pParentSquad->getProtoSquadID()) != cInvalidIndex) : false;
   }
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterObjectTypes::testEntity
//==============================================================================
bool BEntityFilterObjectTypes::testEntity(BEntity *pEntity) const
{
   BASSERT(pEntity);
   bool testResult = false;

   BUnit* pUnit = pEntity->getUnit();
   if (pUnit)
   {
      long numObjectTypes = mObjectTypes.getNumber();
      for (long i=0; i<numObjectTypes; i++)
      {
         if (pUnit->isType(mObjectTypes[i]))
         {
            testResult = true;
            break;
         }
      }
   }
   else if (pEntity->getSquad())
   {
      testResult = true;
      BSquad* pSquad = pEntity->getSquad();
      long numChildren = pSquad->getNumberChildren();
      for (long i=0; i<numChildren; i++)
      {
         BUnit *pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (!pUnit)
            continue;

         bool childPassedFilter = false;
         long numObjectTypes = mObjectTypes.getNumber();
         for (long o=0; o<numObjectTypes; o++)
         {
            if (pUnit->isType(mObjectTypes[o]))
            {
               childPassedFilter = true;
               break;
            }
         }

         if (!childPassedFilter)
         {
            testResult = false;
            break;
         }
      }
   }

   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterRefCount::testEntity
//==============================================================================
bool BEntityFilterRefCount::testEntity(BEntity *pEntity) const
{
   BASSERT(pEntity);
   BEntityRef *pEntityRef = pEntity->getFirstEntityRefByType(mRefCountType);
   long refCountFound = pEntityRef ? pEntityRef->mData1 : 0;

   bool testResult;
   switch(mCompareType)
   {
   case Math::cNotEqualTo:
      testResult = (refCountFound != mCount);
      break;
   case Math::cLessThan:
      testResult = (refCountFound < mCount);
      break;
   case Math::cLessThanOrEqualTo:
      testResult = (refCountFound <= mCount);
      break;
   case Math::cEqualTo:
      testResult = (refCountFound == mCount);
      break;
   case Math::cGreaterThanOrEqualTo:
      testResult = (refCountFound >= mCount);
      break;
   case Math::cGreaterThan:
      testResult = (refCountFound > mCount);
      break;
   default:
      testResult = false;
      break;
   }

   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
//==============================================================================
void BEntityFilterRelationType::onAcquire()
{
   mRelationType = cRelationTypeAny;
   setIsInverted(false);
   setAppliesToUnits(true);
   setAppliesToSquads(true);
   setAppliesToEntities(true);
}

//==============================================================================
//==============================================================================
bool BEntityFilterRelationType::testEntity(BEntity* pEntity) const
{
   BASSERT(pEntity);
   bool testResult = (gWorld->getTeamRelationType(pEntity->getTeamID(), mTeamID) == mRelationType);
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
//==============================================================================
void BEntityFilterMaxObjectType::onAcquire()
{
   mObjectTypeID = cInvalidObjectTypeID;
   setIsInverted(false);
   setAppliesToUnits(false);
   setAppliesToSquads(true);
   setAppliesToEntities(false);
}

//==============================================================================
//==============================================================================
bool BEntityFilterMaxObjectType::testEntity(BEntity* pEntity) const
{
   BASSERT(pEntity);
   bool testResult = false;

   BUnit* pUnit = pEntity->getUnit();
   if (pUnit)
      testResult = pUnit->isType(mObjectTypeID);
   else if (pEntity->getSquad())
   {
      testResult = true;
      BSquad* pSquad = pEntity->getSquad();
      uint numChildren = pSquad->getNumberChildren();
      if (numChildren == 0)
         testResult = false;
      for (uint i = 0; i < numChildren; i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit && !pUnit->isType(mObjectTypeID))
         {
            testResult = false;
            break;
         }         
      }
   }

   if (getIsInverted())
      testResult = !testResult;

   return (testResult);
}

//==============================================================================
// BEntityFilterIsSelected::testEntity
//==============================================================================
bool BEntityFilterIsSelected::testEntity(BEntity* pEntity) const
{
   BASSERT(pEntity);
   BUser *pUser = gUserManager.getUserByPlayerID(mPlayerID);
   if (!pUser)
      return false;

   bool testResult = false;

   BUnit* pUnit = pEntity->getUnit();
   BSquad* pSquad = pEntity->getSquad();
   if (pUnit)
      testResult = pUser->getSelectionManager()->isUnitSelected(pUnit->getID());
   else if (pSquad)
      testResult = pUser->getSelectionManager()->isSquadSelected(pSquad->getID());
   
   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterCanChangeOwner::testEntity
//==============================================================================
bool BEntityFilterCanChangeOwner::testEntity(BEntity* pEntity) const
{
   BASSERT(pEntity);

   bool testResult = false;

   const BSquad* pSquad = NULL;
   const BUnit* pUnit = pEntity->getUnit();
   if (pUnit)
   {
      if (pUnit->getParent())
         pSquad = pUnit->getParent()->getSquad();
   }
   else
      pSquad = pEntity->getSquad();

   if (pSquad)
      testResult = pSquad->hasCompletedBirth() && !pSquad->getFlagIsTransporting();

   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterJacking::testEntity
//==============================================================================
bool BEntityFilterJacking::testEntity(BEntity* pEntity) const
{
   BASSERT(pEntity);

   bool testResult = false;
   const BSquad* pSquad = NULL;
   BUnit* pUnit = pEntity->getUnit();
   if (!pUnit)
   {
      pSquad = pEntity->getSquad();
      if (pSquad && pSquad->getLeaderUnit())
         pUnit = pSquad->getLeaderUnit();
   }

   if (pUnit)
      testResult = pUnit->getActionByType(BAction::cActionTypeUnitJoin) != NULL;

   if (getIsInverted())
      testResult = !testResult;
   return(testResult);
}

//==============================================================================
// BEntityFilterSet::validateAndTestUnit
//==============================================================================
long BEntityFilterSet::validateAndTestUnit(BEntityID unitID) const
{
   BUnit *pUnit = gWorld->getUnit(unitID);
   if (!pUnit)
      return(BEntityFilterSet::cFilterResultInvalid);

   bool testResult = testUnit(pUnit);
   if (testResult)
      return(BEntityFilterSet::cFilterResultPassed);
   else
      return(BEntityFilterSet::cFilterResultFailed);
}


//==============================================================================
// BEntityFilterSet::validateAndTestSquad
//==============================================================================
long BEntityFilterSet::validateAndTestSquad(BEntityID squadID) const
{
   BSquad *pSquad = gWorld->getSquad(squadID);
   if (!pSquad)
      return(BEntityFilterSet::cFilterResultInvalid);

   bool testResult = testSquad(pSquad);
   if (testResult)
      return(BEntityFilterSet::cFilterResultPassed);
   else
      return(BEntityFilterSet::cFilterResultFailed);
}

//==============================================================================
// BEntityFilterSet::validateAndTestProjectile
//==============================================================================
long BEntityFilterSet::validateAndTestProjectile(BEntityID projectileID) const
{
   BProjectile *pProjectile = gWorld->getProjectile(projectileID);
   if (!pProjectile)
      return(BEntityFilterSet::cFilterResultInvalid);

   bool testResult = testProjectile(pProjectile);
   if (testResult)
      return(BEntityFilterSet::cFilterResultPassed);
   else
      return(BEntityFilterSet::cFilterResultFailed);
}


//==============================================================================
// BEntityFilterSet::testUnit
//==============================================================================
bool BEntityFilterSet::testUnit(BUnit *pUnit) const
{
   BASSERT(pUnit);
   long numEntityFilters = mEntityFilters.getNumber();
   for (long i=0; i<numEntityFilters; i++)
   {
      BEntityFilter *pEntityFilter = mEntityFilters[i];
      if (pEntityFilter->getAppliesToUnits() && (pEntityFilter->getType() != BEntityFilter::cFilterTypeMaxObjectType))
      {
         if (!pEntityFilter->testEntity(pUnit))
            return(false);
      }
   }
   return(true);
}


//==============================================================================
// BEntityFilterSet::testSquad
//==============================================================================
bool BEntityFilterSet::testSquad(BSquad *pSquad) const
{
   BASSERT(pSquad);
   long numEntityFilters = mEntityFilters.getNumber();
   for (long i=0; i<numEntityFilters; i++)
   {
      BEntityFilter *pEntityFilter = mEntityFilters[i];
      if (pEntityFilter->getAppliesToSquads() && (pEntityFilter->getType() != BEntityFilter::cFilterTypeMaxObjectType))
      {
         if (!pEntityFilter->testEntity(pSquad))
            return(false);
      }
   }
   return(true);
}

//==============================================================================
// BEntityFilterSet::testProjectile
//==============================================================================
bool BEntityFilterSet::testProjectile(BProjectile *pProjectile) const
{
   BASSERT(pProjectile);
   long numEntityFilters = mEntityFilters.getNumber();
   for (long i=0; i<numEntityFilters; i++)
   {
      BEntityFilter *pEntityFilter = mEntityFilters[i];
      if (pEntityFilter->getAppliesToEntities() && (pEntityFilter->getType() != BEntityFilter::cFilterTypeMaxObjectType))
      {
         if (!pEntityFilter->testEntity(pProjectile))
            return(false);
      }
   }
   return(true);
}


//==============================================================================
// BEntityFilterSet::filterUnits
//==============================================================================
void BEntityFilterSet::filterUnits(const BEntityIDArray &sourceUnitIDs, BEntityIDArray *pUnitsPassed, BEntityIDArray *pUnitsFailed, BEntityIDArray *pUnitsInvalid)
{
   // Do early bail and get some filter query information.
   if (!pUnitsPassed && !pUnitsFailed && !pUnitsInvalid)
      return;

   if (pUnitsPassed)
      pUnitsPassed->setNumber(0);
   if (pUnitsFailed)
      pUnitsFailed->setNumber(0);
   if (pUnitsInvalid)
      pUnitsInvalid->setNumber(0);

   long numSourceUnitIDs = sourceUnitIDs.getNumber();
   for (long unitIdx=0; unitIdx<numSourceUnitIDs; unitIdx++)
   {
      BEntityID unitID = sourceUnitIDs[unitIdx];

      long filterResult = validateAndTestUnit(unitID);
      if (filterResult == BEntityFilterSet::cFilterResultInvalid)
      {
         if (pUnitsInvalid)
            pUnitsInvalid->add(unitID);
      }
      else if (filterResult == BEntityFilterSet::cFilterResultPassed)
      {
         if (pUnitsPassed)
            pUnitsPassed->add(unitID);
      }
      else if (filterResult == BEntityFilterSet::cFilterResultFailed)
      {
         if (pUnitsFailed)
            pUnitsFailed->add(unitID);
      }
   }

   // Second pass test for the max object type filter
   uint numEntityFilters = mEntityFilters.getSize();
   for (uint i = 0; i < numEntityFilters; i++)
   {
//-- FIXING PREFIX BUG ID 4553
      const BEntityFilter* pEntityFilter = mEntityFilters[i];
//--
      if (pEntityFilter->getAppliesToUnits() && (pEntityFilter->getType() == BEntityFilter::cFilterTypeMaxObjectType))
      {
         uint objectTypeCount = (reinterpret_cast<const BEntityFilterMaxObjectType*>(pEntityFilter))->getMaxCount();
         uint numUnitsPassed = (pUnitsPassed) ? pUnitsPassed->getSize() : 0;
         for (int j = ((int)numUnitsPassed - 1); j >= 0; j--)
         {
            BUnit* pUnit = gWorld->getUnit(pUnitsPassed->get(j));
            bool testUnitResult = pEntityFilter->testEntity(pUnit);
            if (testUnitResult && (objectTypeCount > 0))
            {
               objectTypeCount--;               
            }
            else if (testUnitResult && (objectTypeCount == 0))
            {               
               if (pUnitsFailed)
               {
                  pUnitsFailed->uniqueAdd(pUnitsPassed->get(j));
               }
               pUnitsPassed->removeIndex(j);
            }
         }
      }
   }
}


//==============================================================================
// BEntityFilterSet::filterSquads
//==============================================================================
void BEntityFilterSet::filterSquads(const BEntityIDArray &sourceSquadIDs, BEntityIDArray *pSquadsPassed, BEntityIDArray *pSquadsFailed, BEntityIDArray *pSquadsInvalid)
{
   // Do early bail and get some filter query information.
   if (!pSquadsPassed && !pSquadsFailed && !pSquadsInvalid)
      return;

   if (pSquadsPassed)
      pSquadsPassed->setNumber(0);
   if (pSquadsFailed)
      pSquadsFailed->setNumber(0);
   if (pSquadsInvalid)
      pSquadsInvalid->setNumber(0);

   long numSourceSquadIDs = sourceSquadIDs.getNumber();
   for (long squadIdx=0; squadIdx<numSourceSquadIDs; squadIdx++)
   {
      BEntityID squadID = sourceSquadIDs[squadIdx];

      long filterResult = validateAndTestSquad(squadID);
      if (filterResult == BEntityFilterSet::cFilterResultInvalid)
      {
         if (pSquadsInvalid)
            pSquadsInvalid->add(squadID);
      }
      else if (filterResult == BEntityFilterSet::cFilterResultPassed)
      {
         if (pSquadsPassed)
            pSquadsPassed->add(squadID);
      }
      else if (filterResult == BEntityFilterSet::cFilterResultFailed)
      {
         if (pSquadsFailed)
            pSquadsFailed->add(squadID);
      }
   }

   // Second pass test for the max object type filter
   uint numEntityFilters = mEntityFilters.getSize();
   for (uint i = 0; i < numEntityFilters; i++)
   {
//-- FIXING PREFIX BUG ID 4554
      const BEntityFilter* pEntityFilter = mEntityFilters[i];
//--
      if (pEntityFilter->getAppliesToSquads() && (pEntityFilter->getType() == BEntityFilter::cFilterTypeMaxObjectType))
      {
         uint objectTypeCount = (reinterpret_cast<const BEntityFilterMaxObjectType*>(pEntityFilter))->getMaxCount();
         uint numSquadsPassed = (pSquadsPassed) ? pSquadsPassed->getSize() : 0;
         for (int j = ((int)numSquadsPassed - 1); j >= 0; j--)
         {
            BSquad* pSquad = gWorld->getSquad(pSquadsPassed->get(j));
            bool testSquadResult = pEntityFilter->testEntity(pSquad);
            if (testSquadResult && (objectTypeCount > 0))
            {
               objectTypeCount--;               
            }
            else if (testSquadResult && (objectTypeCount == 0))
            {               
               if (pSquadsFailed)
               {
                  pSquadsFailed->uniqueAdd(pSquadsPassed->get(j));
               }
               pSquadsPassed->removeIndex(j);
            }
         }
      }
   }
}

//==============================================================================
// BEntityFilterSet::filterProjectiles
//==============================================================================
void BEntityFilterSet::filterProjectiles(const BEntityIDArray &sourceProjectileIDs, BEntityIDArray *pProjectilesPassed, BEntityIDArray *pProjectilesFailed, BEntityIDArray *pProjectilesInvalid)
{
   // Do early bail and get some filter query information.
   if (!pProjectilesPassed && !pProjectilesFailed && !pProjectilesInvalid)
      return;

   if (pProjectilesPassed)
      pProjectilesPassed->setNumber(0);
   if (pProjectilesFailed)
      pProjectilesFailed->setNumber(0);
   if (pProjectilesInvalid)
      pProjectilesInvalid->setNumber(0);

   long numSourceProjectileIDs = sourceProjectileIDs.getNumber();
   for (long projectileIdx=0; projectileIdx<numSourceProjectileIDs; projectileIdx++)
   {
      BEntityID projectileID = sourceProjectileIDs[projectileIdx];

      long filterResult = validateAndTestProjectile(projectileID);
      if (filterResult == BEntityFilterSet::cFilterResultInvalid)
      {
         if (pProjectilesInvalid)
            pProjectilesInvalid->add(projectileID);
      }
      else if (filterResult == BEntityFilterSet::cFilterResultPassed)
      {
         if (pProjectilesPassed)
            pProjectilesPassed->add(projectileID);
      }
      else if (filterResult == BEntityFilterSet::cFilterResultFailed)
      {
         if (pProjectilesFailed)
            pProjectilesFailed->add(projectileID);
      }
   }

   // Second pass test for the max object type filter
   uint numEntityFilters = mEntityFilters.getSize();
   for (uint i = 0; i < numEntityFilters; i++)
   {
      BEntityFilter* pEntityFilter = mEntityFilters[i];
      if (pEntityFilter->getAppliesToEntities() && (pEntityFilter->getType() == BEntityFilter::cFilterTypeMaxObjectType))
      {
         uint objectTypeCount = (reinterpret_cast<BEntityFilterMaxObjectType*>(pEntityFilter))->getMaxCount();
         uint numProjectilesPassed = (pProjectilesPassed) ? pProjectilesPassed->getSize() : 0;
         for (int j = ((int)numProjectilesPassed - 1); j >= 0; j--)
         {
            BProjectile* pProjectile = gWorld->getProjectile(pProjectilesPassed->get(j));
            bool testProjectileResult = pEntityFilter->testEntity(pProjectile);
            if (testProjectileResult && (objectTypeCount > 0))
            {
               objectTypeCount--;               
            }
            else if (testProjectileResult && (objectTypeCount == 0))
            {               
               if (pProjectilesFailed)
               {
                  pProjectilesFailed->uniqueAdd(pProjectilesPassed->get(j));
               }
               pProjectilesPassed->removeIndex(j);
            }
         }
      }
   }
}


//==============================================================================
//==============================================================================
bool BEntityFilterSet::save(BStream* pStream, int saveType) const 
{ 
   uint16 count = (uint16)mEntityFilters.size();
   GFVERIFYCOUNT(count,1000);
   GFWRITEVAR(pStream,uint16,count);
   for (uint16 i=0; i<count; i++)
   {
      BYTE filterType = mEntityFilters[i]->getType();
      GFWRITEVAR(pStream,BYTE,filterType);
      GFWRITECLASSPTR(pStream, saveType, mEntityFilters[i]);
   }
   return true; 
}

//==============================================================================
//==============================================================================
bool BEntityFilterSet::load(BStream* pStream, int saveType) 
{ 
   clearFilters();
   uint16 count;
   GFREADVAR(pStream,uint16,count);
   GFVERIFYCOUNT(count,1000);
   mEntityFilters.reserve(count);
   for (uint16 i=0; i<count; i++)
   {
      BYTE filterType;
      GFREADVAR(pStream,BYTE,filterType);
      BEntityFilter* pFilter = BEntityFilter::allocateFilter(filterType);
      if (!pFilter)
         return false;
      mEntityFilters.add(pFilter);
      GFREADCLASSPTR(pStream, saveType, pFilter);
   }
   return true; 
}
