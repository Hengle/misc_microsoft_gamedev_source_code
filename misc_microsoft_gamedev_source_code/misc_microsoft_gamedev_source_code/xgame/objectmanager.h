//==============================================================================
// objectmanager.h
//
// objectmanager manages all objects (of all types)
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

#include "entity.h"
#include "object.h"
#include "unit.h"
#include "squad.h"
#include "platoon.h"
#include "army.h"
#include "dopple.h"
#include "projectile.h"
#include "containers\freelist.h"


#pragma once 

class BProtoObject;


#define cMaxAirSpotIndex   UINT16_MAX  // Max index allowed.

class BAirSpot
{
public:
   void init() { mClaimedPos=cInvalidVector; mAircraftID=cInvalidObjectID; };

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BVector     mClaimedPos;
   BEntityID   mAircraftID;
};


class BObjectCreateParms
{
   public:
      BObjectCreateParms(){ reset(); }

      BObjectCreateParms& operator=(const BObjectCreateParms& t)
      {
         mPosition=t.mPosition;
         mForward=t.mForward;
         mRight=t.mRight;
         mPlayerID=t.mPlayerID;
         mCreatedByPlayerID=t.mCreatedByPlayerID;
         mBuiltByUnitID=t.mBuiltByUnitID;
         mSocketUnitID=t.mSocketUnitID;
         mType=t.mType;
         mSourceVisual=t.mSourceVisual;
         mSaveEntityID=t.mSaveEntityID;
         mProtoObjectID=t.mProtoObjectID;
         mProtoSquadID=t.mProtoSquadID;
         mStartBuilt=t.mStartBuilt;
         mPhysicsReplacement=t.mPhysicsReplacement;
         mNoPhysics = t.mNoPhysics;
         mNoTieToGround=t.mNoTieToGround;
         mIgnorePop=t.mIgnorePop;
         mNoCost=t.mNoCost;
         mAOTintValue=t.mAOTintValue;
         mAppearsBelowDecals=t.mAppearsBelowDecals;
         mLevel=t.mLevel;
         mMultiframeTextureIndex=t.mMultiframeTextureIndex;
         mVisualVariationIndex=t.mVisualVariationIndex;
         mExploreGroup=t.mExploreGroup;
         mStartExistSound=t.mStartExistSound;
         mFromSave=t.mFromSave;
         return(*this);
      }

      BVector                             mPosition;
      BVector                             mForward;
      BVector                             mRight;
      float                               mAOTintValue;
      BPlayerID                           mPlayerID;
      BPlayerID                           mCreatedByPlayerID;
      BEntityID                           mBuiltByUnitID;
      BEntityID                           mSocketUnitID;
      BEntity::BClassType                 mType;
      BEntityID                           mSourceVisual;
      BEntityID                           mSaveEntityID;
      int                                 mProtoObjectID;
      int                                 mProtoSquadID;
      int                                 mLevel;
      int                                 mVisualVariationIndex;
      uint                                mMultiframeTextureIndex;
      int16                               mExploreGroup;
      bool                                mStartBuilt:1;
      bool                                mPhysicsReplacement:1;
      bool                                mNoTieToGround:1;
      bool                                mIgnorePop:1;
      bool                                mNoCost:1;
      bool                                mAppearsBelowDecals:1;
      bool                                mNoPhysics:1;
      bool                                mStartExistSound:1;
      bool                                mFromSave:1;

      inline void reset(){ mPlayerID = cInvalidPlayerID; mCreatedByPlayerID = cInvalidPlayerID; mBuiltByUnitID = cInvalidObjectID; mSocketUnitID = cInvalidObjectID; mType = BEntity::cClassTypeObject; mProtoObjectID = cInvalidProtoID; mProtoSquadID = cInvalidProtoID; mStartBuilt = true; mPhysicsReplacement = false; mNoTieToGround = false; mPosition = cOriginVector; mForward = cZAxisVector; mRight = cXAxisVector; mIgnorePop = false; mSourceVisual=cInvalidObjectID; mSaveEntityID=cInvalidObjectID; mNoCost = false; mAOTintValue = 1.0f; mAppearsBelowDecals = false; mLevel=0; mNoPhysics = false; mMultiframeTextureIndex = 0; mVisualVariationIndex = -1; mExploreGroup = -1; mStartExistSound = true; mFromSave=false; }
};

class BObjectManager
{
public:

   BObjectManager();
   virtual ~BObjectManager();

   //-- init
   bool                    init                    ( void );
   void                    reset                   ( void );
  
   //-- create
   BEntity*                createObject(const BObjectCreateParms &parms);

   //-- release
   void                    releaseObject           (BEntity* pObject);
   void                    releaseObject           (BEntityID id);
   
   //-- get
   bool                    validateEntityID        (BEntityID id) const;

   const BEntity*          getEntityConst          ( BEntityID id, bool anyCount = false ) const;
   BEntity*                getEntity               ( BEntityID id, bool anyCount = false );
   BEntity*                getEntity               ( long lower32ID, bool anyCount = false );
   uint                    getNumberEntities       (void) const;
   BEntity*                getNextEntity           (BEntityHandle &id);

   const BObject*          getObjectConst          ( BEntityID id, bool anyCount = false ) const;
   BObject*                getObject               ( BEntityID id, bool anyCount = false );
   uint                    getNumberObjects        (void) const         { return mObjects.getNumberAllocated(); }
   uint                    getObjectsHighWaterMark (void) const         { return mObjects.getHighWaterMark(); }
   BObject*                getNextObject           (BEntityHandle &handle);
   BObject*                getFirstUpdateObject    (uint& handle);
   BObject*                getNextUpdateObject     (uint& handle);
   BObject*                getObjectByIndex        (uint index, bool& inUse);
   uint                    getObjectFreeListSize   () const { return mObjects.getSize(); }

   const BUnit*            getUnitConst            ( BEntityID id, bool anyCount = false ) const;
   BUnit*                  getUnit                 ( BEntityID id, bool anyCount = false );
   BUnit*                  getUnit                 ( long lower32ID, bool anyCount = false );
   uint                    getNumberUnits          (void) const         { return mUnits.getNumberAllocated(); }
   uint                    getUnitsHighWaterMark   (void) const         { return mUnits.getHighWaterMark(); }
   BUnit*                  getNextUnit             (BEntityHandle &handle);
   BUnit*                  getUnitByIndex          (uint index, bool& inUse);
   uint                    getUnitFreeListSize     () const { return mUnits.getSize(); }

   const BDopple*          getDoppleConst          ( BEntityID id, bool anyCount = false ) const;
   BDopple*                getDopple               ( BEntityID id, bool anyCount = false );
   BDopple*                getDopple               ( long lower32ID, bool anyCount = false );
   uint                    getNumberDopples        (void) const         { return mDopples.getNumberAllocated(); }
   uint                    getDopplesHighWaterMark (void) const         { return mDopples.getHighWaterMark(); }
   BDopple*                getNextDopple           (BEntityHandle &handle);
   BDopple*                getDoppleByIndex        (uint index, bool& inUse);
   uint                    getDoppleFreeListSize   () const { return mDopples.getSize(); }

   const BProjectile*      getProjectileConst       ( BEntityID id, bool anyCount = false ) const;
   BProjectile*            getProjectile            ( BEntityID id, bool anyCount = false );
   BProjectile*            getProjectile            ( long lower32ID, bool anyCount = false );
   uint                    getNumberProjectiles     (void) const     { return mProjectiles.getNumberAllocated(); }
   uint                    getProjectilesHighWaterMark   (void) const         { return mProjectiles.getHighWaterMark(); }
   BProjectile*            getNextProjectile        (BEntityHandle &handle);
   BProjectile*            getProjectileByIndex     (uint index, bool& inUse);
   uint                    getProjectileFreeListSize() const { return mProjectiles.getSize(); }

   const BSquad*           getSquadConst           ( BEntityID id, bool anyCount = false ) const;
   BSquad*                 getSquad                ( BEntityID id, bool anyCount = false );
   uint                    getNumberSquads         (void) const         { return mSquads.getNumberAllocated(); }
   uint                    getSquadsHighWaterMark  (void) const         { return mSquads.getHighWaterMark(); }
   BSquad*                 getNextSquad            (BEntityHandle &handle);
   BSquad*                 getSquadByIndex         (uint index, bool& inUse);
   uint                    getSquadFreeListSize     () const { return mSquads.getSize(); }

   const BPlatoon*         getPlatoonConst           ( BEntityID id, bool anyCount = false ) const;
   BPlatoon*               getPlatoon                ( BEntityID id, bool anyCount = false );
   uint                    getNumberPlatoons         (void) const         { return mPlatoons.getNumberAllocated(); }
   uint                    getPlatoonsHighWaterMark  (void) const         { return mPlatoons.getHighWaterMark(); }
   BPlatoon*               getNextPlatoon            (BEntityHandle &handle);
   BPlatoon*               getPlatoonByIndex         (uint index, bool& inUse);
   uint                    getPlatoonFreeListSize    () const { return mPlatoons.getSize(); }

   const BArmy*            getArmyConst           ( BEntityID id, bool anyCount = false ) const;
   BArmy*                  getArmy                ( BEntityID id, bool anyCount = false );
   uint                    getNumberArmies        (void) const         { return mArmies.getNumberAllocated(); }
   uint                    getArmiesHighWaterMark (void) const         { return mArmies.getHighWaterMark(); }
   BArmy*                  getNextArmy            (BEntityHandle &handle);
   BArmy*                  getArmyByIndex         (uint index, bool& inUse);
   uint                    getArmyFreeListSize    () const { return mArmies.getSize(); }


   BEntityID               upconvertEntityID       ( long id ) const;

   BAirSpot*               createClaimedAirSpot    (uint &index);
   BAirSpot*               createClaimedAirSpotAtIndex (uint index);
   uint                    getNumClaimedAirSpots   (void) const         { return mAirSpots.getNumberAllocated(); }
   BAirSpot*               getClaimedAirSpot(      uint &index);
   BAirSpot*               getNextClaimedAirSpot   (uint &index);
   void                    releaseClaimedAirSpot   (uint &index)        { mAirSpots.release(index); }
   uint                    getAirSpotHighWaterMark (void) const         { return mAirSpots.getHighWaterMark(); }

   void                    initObjects             (uint count) { mObjects.init(count); }
   void                    initUnits               (uint count) { mUnits.init(count); }
   void                    initSquads              (uint count) { mSquads.init(count); }
   void                    initPlatoons            (uint count) { mPlatoons.init(count); }
   void                    initArmies              (uint count) { mArmies.init(count); }
   void                    initDopples             (uint count) { mDopples.init(count); }
   void                    initProjectiles         (uint count) { mProjectiles.init(count); }
   void                    initAirSpots            (uint count) { mAirSpots.init(count); }

   void                    setObjectNoUpdate(BEntityID id, bool val);
   BOOL                    getObjectNoUpdate(BEntityID id) const;
   BOOL                    getObjectNoUpdate(uint index) const;

protected:

   BEntity*                allocateObject          (BEntity::BClassType type);
   BEntity*                allocateObjectWithID    (BEntity::BClassType type, BEntityID id);
   BEntityID               getBumpUseCount         (BEntityID id);
   void                    verifyObjectState       (void) const;
   void                    removeFromVisibleLists  (BEntityID id);

   BFreeList<BObject,      3> mObjects;
   //bool*                      mpNoUpdateObjects;
   BDynamicArray<uint64, sizeof(uint64), BDynamicArrayHeapAllocator> mNoUpdateObjects; // Bit array indicating objects set to no update

   BFreeList<BUnit,        3> mUnits;
   BFreeList<BSquad,       3> mSquads;
   BFreeList<BPlatoon,     3> mPlatoons;
   BFreeList<BArmy,        3> mArmies;
   BFreeList<BDopple,      3> mDopples;
   BFreeList<BProjectile,  3> mProjectiles;

   BFreeList<BAirSpot,     3> mAirSpots; // Represents space claimed by aircraft (used for collision avoidance).

};

