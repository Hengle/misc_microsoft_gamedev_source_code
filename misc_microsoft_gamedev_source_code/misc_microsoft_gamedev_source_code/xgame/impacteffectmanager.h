//============================================================================
// impacteffectmanager.h
// Ensemble Studios (C) 2008
//============================================================================

#pragma once

#include "objectmanager.h"
#include "debugTextDisplay.h"

//============================================================================
//============================================================================
typedef void (*CREATEIMPACTOBJECTCALLBACK)(const BObject* pObject, DWORD data);

//============================================================================
//============================================================================
class BImpactEffectData
{
   public:
      BObjectCreateParms         mObjectParams;
      CREATEIMPACTOBJECTCALLBACK mCallback;
      DWORD                      mCallbackData;
      int                        mProtoID;
      int                        mUserdata;
      bool                       mVisibleToAll : 1;
};

//============================================================================
//============================================================================
class BActiveImpactKey : public BBitHashable<BActiveImpactKey>
{
   public:
      BActiveImpactKey() { }
      
      BActiveImpactKey(int xx, int yy, int zz) { set (xx, yy, zz); }

      void set(int xx, int yy, int zz) { x = static_cast<int16>(xx); y = static_cast<int16>(yy); z = static_cast<int16>(zz); dummy = 0; }

      bool operator==(const BActiveImpactKey& rhs) const { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z); }
      
      int16 x;
      int16 y;
      int16 z;
      int16 dummy; // not use/for alignment
};

//============================================================================
//============================================================================
class BActiveImpactInstance
{
   public:
      int      mProtoID;
      BSmallDynamicSimArray<BObject*> mObjects;      
};

//============================================================================
//============================================================================
class BActiveImpactValue
{
   public:
      BSmallDynamicSimArray<BActiveImpactInstance> mValues;      
};

//============================================================================
//============================================================================
class BImpactEffectManager
{
   public:
      enum 
      {
         eVoxelXDim = 8,
         eVoxelYDim = 16,
         eVoxelZDim = 8
      };

      BImpactEffectManager();
     ~BImpactEffectManager();

      void init();
      void deinit();
      void update(float elapsedTime);
      void release(BObject* pObject);
      void addImpactEffect(const BImpactEffectData& data);
      void clearImpactEffects();
      void flushImpactEffects();
      void setEnableLimits(bool value) { mEnableLimits = value; };

#ifndef BUILD_FINAL

      struct BVoxelStat
      {
         public:
            int mProtoID;
      };
      struct BStats
      {
         public: 
         BStats(){clear(); clearMaxStats();};

         void clear(void)
         {
            mNumImpactsSimTriedToCreate = 0;
            mDroppedImpacts = 0;
            mAddedImpacts  = 0;
            mVoxelCount = 0;
         }

         void clearMaxStats()
         {
         }            

         BSmallDynamicSimArray<BVoxelStat> mVoxelStats;

         int mNumImpactsSimTriedToCreate;
         int mDroppedImpacts;
         int mAddedImpacts;

         int mVoxelCount;
      }; 

      

      void getStats(BStats& stats) { ASSERT_MAIN_THREAD; stats = mStats; } 
      void resetStats() { mStats.clear(); }
      BImpactEffectManager::BStats mStats;


      void dumpVoxelData(BDebugTextDisplay& textDisplay) const;
#endif   

   private:
      
      void createImpactEffect(const BImpactEffectData& data);
      void initKey(const BVector& pos, BActiveImpactKey& key) const;

      BDynamicArray<BImpactEffectData> mImpactEffects;
      int mImpactEffectObjectID;

      typedef BHashMap<BActiveImpactKey, BActiveImpactValue, BHasher<BActiveImpactKey>, BEqualTo<BActiveImpactKey>, true, BSimFixedHeapAllocator> BImpactEffectGridHashMap;      
      BImpactEffectGridHashMap mImpactGrid;

      bool mEnableLimits : 1;
};

extern BImpactEffectManager gImpactEffectManager;