//==============================================================================
// terraineffect.h
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================
#pragma once



//============================================================================
// Includes
//#include "impacttype.h"
#include "entityID.h"
#include "simtypes.h"
#include "TerrainSimRep.h"
#include "database.h"


//============================================================================
// Forward declarations
class BTerrainImpactDecalHandle;
class BObject;
class BRibbonCreateParams;

__declspec(selectany) extern const byte cInvalidSurfaceType = 255;

//============================================================================
// class BTerrainEffectItem
//============================================================================
class BTerrainEffectItem
{
   public:
                              BTerrainEffectItem(void);
                              ~BTerrainEffectItem();

      bool                    loadXML(const BXMLNode &root);

      void                    loadAllAssets();
      void                    unloadAllAssets();

      long                                getParticleIndexHandle(void) const { return(mParticleEffectHandle); }
      const BTerrainImpactDecalHandle*    getImpactDecalHandle(void) const { return(mpImpactDecal); }
      BRibbonCreateParams*                getTrailParams(void) const { return mDecalTrailParms;}
      //long                                getSoundHandle(void) const { return(mSoundCueIndex); }
      BSimString                          getSoundName(void) const { return(mSoundName); }
      long                                getProtoVisID() const { return mProtoVisID; }
      long                                getLightEffectHandle() const { return mLightEffectHandle; }
      float                               getLightEffectLifespan() const { return mLightEffectLifespan; }
/*
      const BString           &getParticleSet(void) const {return(mParticleSet);}
      const BString           &getSecondaryParticleSet(void) const {return(mSecondaryParticleSet);}
      const BString           &getSoundSet(long index) const;
      long                    getSoundSetCount(void) const {return(mSoundSets.getNumber());}
*/
      DWORD                   getWeightValue(void) const {return(mWeightValue);}
      void                    setWeightValue(DWORD val) {mWeightValue=val;}

   protected:

      BSimString                 mParticleEffectName;
      long                       mParticleEffectHandle;

      BTerrainImpactDecalHandle* mpImpactDecal;

      BRibbonCreateParams* mDecalTrailParms;

      BSimString                 mSoundName;

      BSimString                 mProtoVisName;
      long                       mProtoVisID;

      BSimString                 mLightEffectName;
      long                       mLightEffectHandle;
      float                      mLightEffectLifespan;

      DWORD                      mWeightValue;

      /*
      BString                 mParticleSet;        // jce [12/5/2003] -- need particle manager to give an ID here.
      BString                 mSecondaryParticleSet;
      BSimpleArray<BString>   mSoundSets;          // jce [12/9/2003] -- these should be handles, keeping a string for now since i don't trust it.
      

*/
};

//============================================================================
//============================================================================
class BTerrainEffectItemArray
{
   public:
      BTerrainEffectItemArray() {}
      virtual ~BTerrainEffectItemArray();

      void loadAllAssets();
      void unloadAllAssets();

      void cleanup();
      void addItem(uint sizeID, BTerrainEffectItem* pItem);
      void normalizeWeightValues(void);
      const BDynamicSimArray<BTerrainEffectItem*>& getItemsBySize(long sizeID) const;

   protected:
      BDynamicSimArray<BTerrainEffectItem*> mItemsBySize[cNumImpactEffectSizes + 1];
};


//============================================================================
// class BTerrainEffect
//============================================================================
class BTerrainEffect
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
   public:      
      enum
      {
         cFlagIsLoaded,
         cFlagAreAllAssetsLoaded,
         cFlagLoadFailed,
      };
                              BTerrainEffect(void);
                              ~BTerrainEffect();

      bool                    init(const BCHAR_T* pName, int id);

      bool                    load();
      bool                    reload(void);

      void                    loadAllAssets();
      void                    unloadAllAssets();

      const BSimString&       getName() const                       { return mName; }
      void                    setName(const BSimString &fileName)   { mName = fileName; }
      BTerrainEffectItem*     getItem(byte surfaceType, long sizeID, long randTag) const;
      BTerrainEffectItem*     getItemByIndex(byte surfaceType, long sizeID, long index) const;
      long                    getNumItems(byte surfaceType, long sizeID) const;
      long                    getRandomItemIndex(byte surfaceType, long sizeID, long randTag) const;

      XMFINLINE void          instantiateEffect(const BVector &pos, const BVector &dir, bool visible);
      void                    instantiateEffect(byte surfaceType, long sizeID, const BVector &pos, const BVector &dir, bool visible, BPlayerID playerID, float lifespan, bool visibleToAll, int displayPriority, BObject** ppReturnObject);//, bool usePrimaryEffect = true, BUnit* pAttachToUnit = NULL, long dummyIndex = 0);
      
      long                    getParticleEffectHandleForType(byte surfaceType, long sizeID);
      BRibbonCreateParams*    getTrailDecalHandleForType(byte surfaceType, long sizeID);

      int                     getID() const { return mID; }
      DWORD                   getMeterLength() const { return mMeterLength; }
      void                    setMeterLength(DWORD length) { mMeterLength = length; }
      uint                    getMeterCount() const { return mMeterCount; }
      void                    setMeterCount(uint count) { mMeterCount = count; }

      bool                    getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void                    setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

      bool                    isLoaded() const   { return getFlag(cFlagIsLoaded); }
      bool                    areAllAssetsLoaded() const   { return getFlag(cFlagAreAllAssetsLoaded); }
      bool                    loadFailed() const { return getFlag(cFlagLoadFailed); }

   protected:
      bool                    loadXML(const BXMLNode &root);

      void                    cleanup(void);

      void                    normalizeWeightValues(void);
      //bool                    checkMeter(BVector pos);

      BSimString              mName;
      BDynamicSimArray<BTerrainEffectItemArray> mItems;

      int                     mID;
      DWORD                   mMeterLength;
      uint                    mMeterCount;
      //DWORD                   mExitDelayMult;
      //float                   mExitImpulseMult;
      //float                   mExitUnitLifespan;
      //float                   mExitEffectSpawnChance;


      UTBitVector<16>         mFlags;      
#ifdef ENABLE_RELOAD_MANAGER
      virtual bool            receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
};

//============================================================================
//============================================================================
XMFINLINE void BTerrainEffect::instantiateEffect(const BVector &pos, const BVector &dir, bool visible)
{
   // Get terrain type
   BYTE surfaceType = gTerrainSimRep.getTileType(pos);
   instantiateEffect(surfaceType, -1, pos, dir, visible, cInvalidPlayerID, 0.0f, false, cVisualDisplayPriorityNormal, NULL);
}

