//==============================================================================
// entityfilter.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "simtypes.h"
#include "savegame.h"

class BEntity;
class BUnit;
class BSquad;
class BProjectile;

//=============================================================================
// class BEntityFilter
//=============================================================================
class BEntityFilter
{
public:
   enum
   {
      cFilterTypeIsAlive,
      cFilterTypeIsIdle,
      cFilterTypeInList,
      cFilterTypePlayers,
      cFilterTypeTeams,
      cFilterTypeProtoObjects,
      cFilterTypeProtoSquads,
      cFilterTypeObjectTypes,
      cFilterTypeRefCount,
      cFilterTypeRelationType,
      cFilterTypeMaxObjectType,
      cFilterTypeIsSelected,
      cFilterTypeCanChangeOwner,
      cFilterTypeJacking,
   };

   BYTE getType() const { return (mType); }
   bool isType(BYTE type) const { return (mType == type); }
   void setIsInverted(bool v) { mbIsInverted = v; }
   void setAppliesToEntities(bool v) { mbAppliesToEntities = v; }
   void setAppliesToUnits(bool v) { mbAppliesToUnits = v; }
   void setAppliesToSquads(bool v) { mbAppliesToSquads = v; }
   bool getIsInverted() const { return (mbIsInverted); }
   bool getAppliesToEntities() const { return (mbAppliesToEntities); }
   bool getAppliesToUnits() const { return (mbAppliesToUnits); }
   bool getAppliesToSquads() const { return (mbAppliesToSquads); }

   static BEntityFilter* allocateFilter(BYTE filterType);
   static void releaseFilter(BEntityFilter *pEntityFilter);
   virtual bool testEntity(BEntity* pEntity) const = 0;
   virtual BEntityFilter* clone() = 0;

   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BYTE, mType); GFWRITEBITBOOL(pStream, mbIsInverted); GFWRITEBITBOOL(pStream, mbAppliesToUnits); GFWRITEBITBOOL(pStream, mbAppliesToSquads); GFWRITEBITBOOL(pStream, mbAppliesToEntities); return true; }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BYTE, mType); GFREADBITBOOL(pStream, mbIsInverted); GFREADBITBOOL(pStream, mbAppliesToUnits); GFREADBITBOOL(pStream, mbAppliesToSquads); GFREADBITBOOL(pStream, mbAppliesToEntities); return true; }

protected:
   BYTE mType;
   bool mbIsInverted       : 1;
   bool mbAppliesToUnits   : 1;
   bool mbAppliesToSquads  : 1;
   bool mbAppliesToEntities: 1;
};


//=============================================================================
// class BEntityFilterIsAlive
//=============================================================================
class BEntityFilterIsAlive : public BEntityFilter, IPoolable
{
public:
   BEntityFilterIsAlive() { mType = BEntityFilter::cFilterTypeIsAlive; }
   ~BEntityFilterIsAlive() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterIsAlive, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   virtual BEntityFilter* clone() { BEntityFilterIsAlive *pClone = BEntityFilterIsAlive::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; return (pClone); }
protected:
};

//=============================================================================
// class BEntityFilterIsIdle
//=============================================================================
class BEntityFilterIsIdle : public BEntityFilter, IPoolable
{
   public:
      BEntityFilterIsIdle(){ mType = BEntityFilter::cFilterTypeIsIdle; }
      ~BEntityFilterIsIdle() {}
      virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); }
      virtual void onRelease(){}
      DECLARE_FREELIST(BEntityFilterIsIdle, 4);
      virtual bool testEntity(BEntity* pEntity) const;
      virtual BEntityFilter* clone(){ BEntityFilterIsIdle* pClone = BEntityFilterIsIdle::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; return (pClone); }
   protected:
};

//=============================================================================
// class BEntityFilterInList
//=============================================================================
class BEntityFilterInList : public BEntityFilter, IPoolable
{
public:
   BEntityFilterInList() { mType = BEntityFilter::cFilterTypeInList; }
   ~BEntityFilterInList() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterInList, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   void setEntityList(const BEntityIDArray &newEntityList) { mEntityList = newEntityList; }
   const BEntityIDArray& getEntityList() const { return (mEntityList); }
   virtual BEntityFilter* clone() { BEntityFilterInList *pClone = BEntityFilterInList::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mEntityList = mEntityList; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BEntityID,mEntityList,uint16,2000); return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BEntityID,mEntityList,uint16,2000); return BEntityFilter::load(pStream, saveType); }
protected:
   BEntityIDArray mEntityList;
};


//=============================================================================
// class BEntityFilterPlayers
//=============================================================================
class BEntityFilterPlayers : public BEntityFilter, IPoolable
{
public:
   BEntityFilterPlayers() { mType = BEntityFilter::cFilterTypePlayers; }
   ~BEntityFilterPlayers() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterPlayers, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   void setPlayers(const BPlayerIDArray &newPlayers) { mPlayers = newPlayers; }
   const BPlayerIDArray& getPlayers() const { return (mPlayers); }
   virtual BEntityFilter* clone() { BEntityFilterPlayers *pClone = BEntityFilterPlayers::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mPlayers = mPlayers; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BPlayerID,mPlayers,uint8,20); return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BPlayerID,mPlayers,uint8,20); return BEntityFilter::load(pStream, saveType); }
protected:
   BPlayerIDArray mPlayers;
};


//=============================================================================
// class BEntityFilterTeams
//=============================================================================
class BEntityFilterTeams : public BEntityFilter, IPoolable
{
public:
   BEntityFilterTeams() { mType = BEntityFilter::cFilterTypeTeams; }
   ~BEntityFilterTeams() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterTeams, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   void setTeams(const BTeamIDArray &newTeams) { mTeams = newTeams; }
   const BTeamIDArray& getTeams() const { return (mTeams); }
   virtual BEntityFilter* clone() { BEntityFilterTeams *pClone = BEntityFilterTeams::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mTeams = mTeams; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BTeamID,mTeams,uint8,10); return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BTeamID,mTeams,uint8,10); return BEntityFilter::load(pStream, saveType); }
protected:
   BTeamIDArray mTeams;
};


//=============================================================================
// class BEntityFilterProtoObjects
//=============================================================================
class BEntityFilterProtoObjects : public BEntityFilter, IPoolable
{
public:
   BEntityFilterProtoObjects() { mType = BEntityFilter::cFilterTypeProtoObjects; }
   ~BEntityFilterProtoObjects() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterProtoObjects, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   void setProtoObjects(const BProtoObjectIDArray &newProtoObjects) { mProtoObjects = newProtoObjects; }
   const BProtoObjectIDArray& getProtoObjects() const { return (mProtoObjects); }
   virtual BEntityFilter* clone() { BEntityFilterProtoObjects *pClone = BEntityFilterProtoObjects::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mProtoObjects = mProtoObjects; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BProtoObjectID,mProtoObjects,uint16,1000); return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BProtoObjectID,mProtoObjects,uint16,1000); gSaveGame.remapProtoObjectIDs(mProtoObjects); return BEntityFilter::load(pStream, saveType); }
protected:
   BProtoObjectIDArray mProtoObjects;
};


//=============================================================================
// class BEntityFilterProtoSquads
//=============================================================================
class BEntityFilterProtoSquads : public BEntityFilter, IPoolable
{
public:
   BEntityFilterProtoSquads() { mType = BEntityFilter::cFilterTypeProtoSquads; }
   ~BEntityFilterProtoSquads() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterProtoSquads, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   void setProtoSquads(const BProtoSquadIDArray &newProtoSquads) { mProtoSquads = newProtoSquads; }
   const BProtoSquadIDArray& getProtoSquads() const { return (mProtoSquads); }
   virtual BEntityFilter* clone() { BEntityFilterProtoSquads *pClone = BEntityFilterProtoSquads::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mProtoSquads = mProtoSquads; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BProtoSquadID,mProtoSquads,uint16,1000); return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BProtoSquadID,mProtoSquads,uint16,1000); gSaveGame.remapProtoSquadIDs(mProtoSquads); return BEntityFilter::load(pStream, saveType); }
protected:
   BProtoSquadIDArray mProtoSquads;
};


//=============================================================================
// class BEntityFilterObjectTypes
//=============================================================================
class BEntityFilterObjectTypes : public BEntityFilter, IPoolable
{
public:
   BEntityFilterObjectTypes() { mType = BEntityFilter::cFilterTypeObjectTypes; }
   ~BEntityFilterObjectTypes() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterObjectTypes, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   void setObjectTypes(const BObjectTypeIDArray &newObjectTypes) { mObjectTypes = newObjectTypes; }
   const BObjectTypeIDArray& getObjectTypes() const { return (mObjectTypes); }
   virtual BEntityFilter* clone() { BEntityFilterObjectTypes *pClone = BEntityFilterObjectTypes::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mObjectTypes = mObjectTypes; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BObjectTypeID,mObjectTypes,uint16,1000); return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BObjectTypeID,mObjectTypes,uint16,1000); gSaveGame.remapObjectTypes(mObjectTypes); return BEntityFilter::load(pStream, saveType); }
protected:
   BObjectTypeIDArray mObjectTypes;
};


//=============================================================================
// class BEntityFilterRefCount
//=============================================================================
class BEntityFilterRefCount : public BEntityFilter, IPoolable
{
public:
   BEntityFilterRefCount() { mType = BEntityFilter::cFilterTypeRefCount; }
   ~BEntityFilterRefCount() {}
   virtual void onAcquire(){setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); mRefCountType = 0; mCompareType = 0; mCount = 0; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterRefCount, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   void setRefCountFilterData(long refCountType, long compareType, long compareCount) { mRefCountType = refCountType; mCompareType = compareType; mCount = compareCount; }
   virtual BEntityFilter* clone() { BEntityFilterRefCount *pClone = BEntityFilterRefCount::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mRefCountType = mRefCountType; pClone->mCompareType = mCompareType; pClone->mCount = mCount; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream,long,mRefCountType); GFWRITEVAR(pStream,long,mCompareType); GFWRITEVAR(pStream,long,mCount); return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream,long,mRefCountType); GFREADVAR(pStream,long,mCompareType); GFREADVAR(pStream,long,mCount); return BEntityFilter::load(pStream, saveType); }
protected:

   long mRefCountType;
   long mCompareType;
   long mCount;
};

//=============================================================================
// class BEntityFilterRelationType
//=============================================================================
class BEntityFilterRelationType : public BEntityFilter, IPoolable
{
   public:
      BEntityFilterRelationType(){ mType = BEntityFilter::cFilterTypeRelationType; }
      ~BEntityFilterRelationType() {}
      virtual void onAcquire();
      virtual void onRelease() {}
      DECLARE_FREELIST(BEntityFilterRelationType, 4);
      virtual bool testEntity(BEntity* pEntity) const;
      void setRelationTypeFilterData(BRelationType relationType, BTeamID teamID) { mRelationType = relationType; mTeamID = teamID; }
      virtual BEntityFilter* clone(){ BEntityFilterRelationType* pClone = BEntityFilterRelationType::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mRelationType = mRelationType; pClone->mTeamID = mTeamID; return (pClone); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream,BRelationType,mRelationType); GFWRITEVAR(pStream,BTeamID,mTeamID); return BEntityFilter::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream,BRelationType,mRelationType); GFREADVAR(pStream,BTeamID,mTeamID); return BEntityFilter::load(pStream, saveType); }

   protected:
      BRelationType mRelationType;
      BTeamID mTeamID;
};

//=============================================================================
// class BEntityFilterMaxObjectType
//=============================================================================
class BEntityFilterMaxObjectType : public BEntityFilter, IPoolable
{
   public:
      BEntityFilterMaxObjectType(){ mType = BEntityFilter::cFilterTypeMaxObjectType; }
      ~BEntityFilterMaxObjectType() {}
      virtual void onAcquire();
      virtual void onRelease() {}
      DECLARE_FREELIST(BEntityFilterMaxObjectType, 4);
      virtual bool testEntity(BEntity* pEntity) const;
      void setMaxObjectTypeFilterData(BObjectTypeID objectTypeID, uint maxCount, bool applyToSquads = true, bool applyToUnits = false, bool applyToEntities = false) { mObjectTypeID = objectTypeID; mMaxCount = maxCount; mbAppliesToSquads = applyToSquads; mbAppliesToEntities = applyToEntities; mbAppliesToUnits = applyToUnits; }
      virtual BEntityFilter* clone(){ BEntityFilterMaxObjectType* pClone = BEntityFilterMaxObjectType::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; pClone->mObjectTypeID = mObjectTypeID; pClone->mMaxCount = mMaxCount; return (pClone); }
      uint getMaxCount() const { return (mMaxCount); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream,BObjectTypeID,mObjectTypeID); GFWRITEVAR(pStream,uint,mMaxCount); return BEntityFilter::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream,BObjectTypeID,mObjectTypeID); GFREADVAR(pStream,uint,mMaxCount); gSaveGame.remapObjectType(mObjectTypeID); return BEntityFilter::load(pStream, saveType); }

   protected:
      BObjectTypeID mObjectTypeID;
      uint mMaxCount;
};

//=============================================================================
// class BEntityFilterIsSelected
//=============================================================================
class BEntityFilterIsSelected : public BEntityFilter, IPoolable
{
public:
   BEntityFilterIsSelected(){ mType = BEntityFilter::cFilterTypeIsSelected; }
   ~BEntityFilterIsSelected() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterIsSelected, 4);
   void setPlayerID(BPlayerID player) { mPlayerID = player; }
   virtual bool testEntity(BEntity* pEntity) const;
   virtual BEntityFilter* clone(){ BEntityFilterIsSelected* pClone = BEntityFilterIsSelected::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream,BPlayerID,mPlayerID); return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream,BPlayerID,mPlayerID); return BEntityFilter::load(pStream, saveType); }
protected:
   BPlayerID   mPlayerID;
};


//=============================================================================
// class BEntityFilterCanChangeOwner
//=============================================================================
class BEntityFilterCanChangeOwner : public BEntityFilter, IPoolable
{
public:
   BEntityFilterCanChangeOwner(){ mType = BEntityFilter::cFilterTypeCanChangeOwner; }
   ~BEntityFilterCanChangeOwner() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterCanChangeOwner, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   virtual BEntityFilter* clone(){ BEntityFilterCanChangeOwner* pClone = BEntityFilterCanChangeOwner::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { return BEntityFilter::load(pStream, saveType); }
protected:
};

//=============================================================================
// class BEntityFilterJacking
//=============================================================================
class BEntityFilterJacking : public BEntityFilter, IPoolable
{
public:
   BEntityFilterJacking(){ mType = BEntityFilter::cFilterTypeJacking; }
   ~BEntityFilterJacking() {}
   virtual void onAcquire(){ setIsInverted(false); setAppliesToUnits(true); setAppliesToSquads(true); setAppliesToEntities(true); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BEntityFilterJacking, 4);
   virtual bool testEntity(BEntity* pEntity) const;
   virtual BEntityFilter* clone(){ BEntityFilterJacking* pClone = BEntityFilterJacking::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mbAppliesToUnits = mbAppliesToUnits; pClone->mbAppliesToSquads = mbAppliesToSquads; pClone->mbAppliesToEntities = mbAppliesToEntities; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { return BEntityFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { return BEntityFilter::load(pStream, saveType); }
protected:
};

//=============================================================================
// class BEntityFilterSet
//=============================================================================
class BEntityFilterSet
{
public:
   enum
   {
      cFilterResultPassed,
      cFilterResultFailed,
      cFilterResultInvalid,
   };

   BEntityFilterSet() {}
   ~BEntityFilterSet() { clearFilters(); }
   
   // Functions for adding filters or clearing them
   void clearFilters();
   void addEntityFilterIsAlive(bool invertFilter);
   void addEntityFilterIsIdle(bool invertFilter);
   void addEntityFilterInList(bool invertFilter, const BEntityIDArray &entityList);
   void addEntityFilterPlayers(bool invertFilter, const BPlayerIDArray &players);
   void addEntityFilterTeams(bool invertFilter, const BTeamIDArray &teams);
   void addEntityFilterProtoObjects(bool invertFilter, const BProtoObjectIDArray &protoObjects);
   void addEntityFilterProtoSquads(bool invertFilter, const BProtoSquadIDArray &protoSquads);
   void addEntityFilterObjectTypes(bool invertFilter, const BObjectTypeIDArray &objectTypes);
   void addEntityFilterRefCount(bool invertFilter, long refCountType, long compareType, long count);
   void addEntityFilterRelationType(bool invertFilter, BRelationType relationType, BTeamID teamID);
   void addEntityFilterMaxObjectType(bool invertFilter, BObjectTypeID objectTypeID, uint maxCount, bool applyToSquads = true, bool applyToUnits = false);
   void addEntityFilterIsSelected(bool invertFilter, BPlayerID player);
   void addEntityFilterCanChangeOwner(bool invertFilter);
   void addEntityFilterJacking(bool invertFilter);

   void copyFilterSet(const BEntityFilterSet *pSourceEntityFilterSet);

   long validateAndTestUnit(BEntityID unitID) const;
   long validateAndTestSquad(BEntityID squadID) const;
   long validateAndTestProjectile(BEntityID projectileID) const;
   bool testUnit(BUnit *pUnit) const;
   bool testSquad(BSquad *pSquad) const;
   bool testProjectile(BProjectile *pProjectile) const;
   void filterUnits(const BEntityIDArray &sourceUnitIDs, BEntityIDArray *pUnitsPassed, BEntityIDArray *pUnitsFailed, BEntityIDArray *pUnitsInvalid);
   void filterSquads(const BEntityIDArray &sourceSquadIDs, BEntityIDArray *pSquadsPassed, BEntityIDArray *pSquadsFailed, BEntityIDArray *pSquadsInvalid);
   void filterProjectiles(const BEntityIDArray &sourceProjectileIDs, BEntityIDArray *pProjectilesPassed, BEntityIDArray *pProjectilesFailed, BEntityIDArray *pProjectilesInvalid);

   bool isEmpty() const { return (mEntityFilters.getSize() == 0); }
   int getNumFilters() const { return mEntityFilters.getSize(); }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BSmallDynamicSimArray<BEntityFilter*> mEntityFilters;
};