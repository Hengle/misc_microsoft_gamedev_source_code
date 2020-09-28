//==============================================================================
// kbunitfilter.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "kbsquad.h"
#include "simtypes.h"

//=============================================================================
// class BKBSquadFilter
//=============================================================================
class BKBSquadFilter
{
public:
   enum
   {
      cFilterTypeCurrentlyVisible,
      cFilterTypeObjectTypes,
      cFilterTypePlayers,
      cFilterTypeInList,
      cFilterTypePlayerRelation,
      cFilterTypeMinStaleness,
      cFilterTypeMaxStaleness,
   };

   BYTE getType() const { return (mType); }
   bool isType(BYTE type) const { return (mType == type); }
   void setIsInverted(bool v) { mbIsInverted = v; }
   bool getIsInverted() const { return (mbIsInverted); }

   static BKBSquadFilter* allocateFilter(BYTE filterType);
   static void releaseFilter(BKBSquadFilter *pKBSquadFilter);
   virtual bool testKBSquad(BKBSquad *pKBSquad) const = 0;
   virtual BKBSquadFilter* clone() = 0;

   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BYTE, mType); GFWRITEBITBOOL(pStream, mbIsInverted); return true; }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BYTE, mType); GFREADBITBOOL(pStream, mbIsInverted); return true; }

protected:
   BYTE mType;
   bool mbIsInverted       : 1;
};


//=============================================================================
// class BKBSquadFilterCurrentlyVisible
//=============================================================================
class BKBSquadFilterCurrentlyVisible : public BKBSquadFilter, IPoolable
{
public:
   BKBSquadFilterCurrentlyVisible(){ mType = BKBSquadFilter::cFilterTypeCurrentlyVisible; }
   ~BKBSquadFilterCurrentlyVisible() {}
   virtual void onAcquire(){ setIsInverted(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BKBSquadFilterCurrentlyVisible, 4);
   virtual bool testKBSquad(BKBSquad* pKBSquad) const;
   virtual BKBSquadFilter* clone(){ BKBSquadFilterCurrentlyVisible* pClone = BKBSquadFilterCurrentlyVisible::getInstance(); pClone->mbIsInverted = mbIsInverted; return (pClone); }
protected:
};

//=============================================================================
//=============================================================================
class BKBSquadFilterInList : public BKBSquadFilter, IPoolable
{
public:
   BKBSquadFilterInList() { mType = BKBSquadFilter::cFilterTypeInList; }
   ~BKBSquadFilterInList() {}
   virtual void onAcquire(){ setIsInverted(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BKBSquadFilterInList, 4);
   virtual bool testKBSquad(BKBSquad *pKBSquad) const;
   void setKBSquadList(const BKBSquadIDArray &newKBSquadList) { mKBSquadList = newKBSquadList; }
   const BKBSquadIDArray& getKBSquadList() const { return (mKBSquadList); }
   virtual BKBSquadFilter* clone() { BKBSquadFilterInList *pClone = BKBSquadFilterInList::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mKBSquadList = mKBSquadList; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BKBSquadID,mKBSquadList,uint16,2000); return BKBSquadFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BKBSquadID,mKBSquadList,uint16,2000); return BKBSquadFilter::load(pStream, saveType); }
protected:
   BKBSquadIDArray mKBSquadList;
};


//=============================================================================
//=============================================================================
class BKBSquadFilterPlayers : public BKBSquadFilter, IPoolable
{
public:
   BKBSquadFilterPlayers() { mType = BKBSquadFilter::cFilterTypePlayers; }
   ~BKBSquadFilterPlayers() {}
   virtual void onAcquire(){ setIsInverted(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BKBSquadFilterPlayers, 4);
   virtual bool testKBSquad(BKBSquad *pKBSquad) const;
   void setPlayers(const BPlayerIDArray &newPlayers) { mPlayers = newPlayers; }
   const BPlayerIDArray& getPlayers() const { return (mPlayers); }
   virtual BKBSquadFilter* clone() { BKBSquadFilterPlayers *pClone = BKBSquadFilterPlayers::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mPlayers = mPlayers; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BPlayerID,mPlayers,uint8,20); return BKBSquadFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BPlayerID,mPlayers,uint8,20); return BKBSquadFilter::load(pStream, saveType); }
protected:
   BPlayerIDArray mPlayers;
};


//=============================================================================
//=============================================================================
class BKBSquadFilterObjectTypes : public BKBSquadFilter, IPoolable
{
public:
   BKBSquadFilterObjectTypes() { mType = BKBSquadFilter::cFilterTypeObjectTypes; }
   ~BKBSquadFilterObjectTypes() {}
   virtual void onAcquire(){ setIsInverted(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BKBSquadFilterObjectTypes, 4);
   virtual bool testKBSquad(BKBSquad *pKBSquad) const;
   void setObjectTypes(const BObjectTypeIDArray &newObjectTypes) { mObjectTypes = newObjectTypes; }
   const BObjectTypeIDArray& getObjectTypes() const { return (mObjectTypes); }
   virtual BKBSquadFilter* clone() { BKBSquadFilterObjectTypes *pClone = BKBSquadFilterObjectTypes::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mObjectTypes = mObjectTypes; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream,BObjectTypeID,mObjectTypes,uint16,1000); return BKBSquadFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream,BObjectTypeID,mObjectTypes,uint16,1000); gSaveGame.remapObjectTypes(mObjectTypes); return BKBSquadFilter::load(pStream, saveType); }
protected:
   BObjectTypeIDArray mObjectTypes;
};


//=============================================================================
//=============================================================================
class BKBSquadFilterPlayerRelation : public BKBSquadFilter, IPoolable
{
public:
   BKBSquadFilterPlayerRelation(){ mType = BKBSquadFilter::cFilterTypePlayerRelation; }
   ~BKBSquadFilterPlayerRelation() {}
   virtual void onAcquire();
   virtual void onRelease() {}
   DECLARE_FREELIST(BKBSquadFilterPlayerRelation, 4);
   virtual bool testKBSquad(BKBSquad* pKBSquad) const;
   void setPlayerRelationFilterData(BPlayerID playerID, BRelationType relationType, bool selfAsAlly) { mPlayerID = playerID; mRelationType = relationType; mbSelfAsAlly = selfAsAlly; }
   virtual BKBSquadFilter* clone(){ BKBSquadFilterPlayerRelation* pClone = BKBSquadFilterPlayerRelation::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mPlayerID = mPlayerID; pClone->mRelationType = mRelationType; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream,BPlayerID,mPlayerID); GFWRITEVAR(pStream,BRelationType,mRelationType); GFWRITEVAR(pStream,bool,mbSelfAsAlly); return BKBSquadFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream,BPlayerID,mPlayerID); GFREADVAR(pStream,BRelationType,mRelationType); GFREADVAR(pStream,bool,mbSelfAsAlly); return BKBSquadFilter::load(pStream, saveType); }

protected:
   BPlayerID mPlayerID;
   BRelationType mRelationType;
   bool mbSelfAsAlly;
};


//=============================================================================
//=============================================================================
class BKBSquadFilterMinStaleness : public BKBSquadFilter, IPoolable
{
public:
   BKBSquadFilterMinStaleness() { mType = BKBSquadFilter::cFilterTypeMinStaleness; }
   ~BKBSquadFilterMinStaleness() {}
   virtual void onAcquire(){ setIsInverted(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BKBSquadFilterMinStaleness, 4);
   virtual bool testKBSquad(BKBSquad *pKBSquad) const;
   void setMinStaleness(DWORD minStaleness) { mMinStaleness = minStaleness; }
   DWORD getMinStaleness() const { return (mMinStaleness); }
   virtual BKBSquadFilter* clone() { BKBSquadFilterMinStaleness *pClone = BKBSquadFilterMinStaleness::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mMinStaleness = mMinStaleness; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream,DWORD,mMinStaleness); return BKBSquadFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream,DWORD,mMinStaleness); return BKBSquadFilter::load(pStream, saveType); }
protected:
   DWORD mMinStaleness;
};


//=============================================================================
//=============================================================================
class BKBSquadFilterMaxStaleness : public BKBSquadFilter, IPoolable
{
public:
   BKBSquadFilterMaxStaleness() { mType = BKBSquadFilter::cFilterTypeMaxStaleness; }
   ~BKBSquadFilterMaxStaleness() {}
   virtual void onAcquire(){ setIsInverted(false); }
   virtual void onRelease(){}
   DECLARE_FREELIST(BKBSquadFilterMaxStaleness, 4);
   virtual bool testKBSquad(BKBSquad *pKBSquad) const;
   void setMaxStaleness(DWORD maxStaleness) { mMaxStaleness = maxStaleness; }
   DWORD getMaxStaleness() const { return (mMaxStaleness); }
   virtual BKBSquadFilter* clone() { BKBSquadFilterMaxStaleness *pClone = BKBSquadFilterMaxStaleness::getInstance(); pClone->mbIsInverted = mbIsInverted; pClone->mMaxStaleness = mMaxStaleness; return (pClone); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream,DWORD,mMaxStaleness); return BKBSquadFilter::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream,DWORD,mMaxStaleness); return BKBSquadFilter::load(pStream, saveType); }
protected:
   DWORD mMaxStaleness;
};



//=============================================================================
// class BKBSquadFilterSet
//=============================================================================
class BKBSquadFilterSet
{
public:
   enum
   {
      cFilterResultPassed,
      cFilterResultFailed,
      cFilterResultInvalid,
   };

   BKBSquadFilterSet() {}
   ~BKBSquadFilterSet() { clearFilters(); }

   // Functions for adding filters or clearing them
   void clearFilters();
   void addKBSquadFilterCurrentlyVisible(bool invertFilter);
   void addKBSquadFilterObjectTypes(bool invertFilter, const BObjectTypeIDArray &objectTypes);
   void addKBSquadFilterPlayers(bool invertFilter, const BPlayerIDArray &players);
   void addKBSquadFilterInList(bool invertFilter, const BKBSquadIDArray &kbUnitList);
   void addKBSquadFilterPlayerRelation(bool invertFilter, BPlayerID playerID, BRelationType relationType, bool selfAsAlly);
   void addKBSquadFilterMinStaleness(bool invertFilter, DWORD minStaleness);
   void addKBSquadFilterMaxStaleness(bool invertFilter, DWORD maxStaleness);
   void copyFilterSet(const BKBSquadFilterSet *pSourceKBSquadFilterSet);

   uint validateAndTestKBSquad(BKBSquadID kbSquadID) const;
   bool testKBSquad(BKBSquad* pKBSquad) const;
   void filterKBSquads(BKB* pKB, const BKBSquadIDArray &sourceKBSquadIDs, BKBSquadIDArray *pKBSquadsPassed, BKBSquadIDArray *pKBSquadsFailed, BKBSquadIDArray *pKBSquadsInvalid);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BSmallDynamicSimArray<BKBSquadFilter*> mKBSquadFilters;
};