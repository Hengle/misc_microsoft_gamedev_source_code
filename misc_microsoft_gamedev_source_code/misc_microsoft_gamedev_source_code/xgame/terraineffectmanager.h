//==============================================================================
// terraineffectmanager.h
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================

#pragma once

#include "debugTextDisplay.h"

//============================================================================
//  Forward Declarations
class BTerrainEffect;

//============================================================================
//============================================================================
class BActiveTerrainEffectKey : public BBitHashable<BActiveTerrainEffectKey>
{
   public:
      BActiveTerrainEffectKey() { }
      
      BActiveTerrainEffectKey(int xx, int yy, int zz) { set (xx, yy, zz); }

      void set(int xx, int yy, int zz) { x = static_cast<int16>(xx); y = static_cast<int16>(yy); z = static_cast<int16>(zz); dummy = 0; }

      bool operator==(const BActiveTerrainEffectKey& rhs) const { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z); }
      
      int16 x;
      int16 y;
      int16 z;
      int16 dummy; // not use/for alignment
};

//============================================================================
//============================================================================
class BActiveTerrainEffectInstance
{
   public:
      int                           mTerrainEffectID;
      BSmallDynamicSimArray<DWORD>  mTimes;
};

//============================================================================
//============================================================================
class BActiveTerrainEffectValue
{
   public:
      BSmallDynamicSimArray<BActiveTerrainEffectInstance> mValues;      
};

//============================================================================
// BTerrainEffectManager
//============================================================================
class BTerrainEffectManager
{
   public:

      enum 
      {
         eVoxelXDim = 8,
         eVoxelYDim = 16,
         eVoxelZDim = 8
      };

                              BTerrainEffectManager();
                              ~BTerrainEffectManager();

      void                    reset( void );
      void                    clear();
      void                    unloadAll();

      long                    findTerrainEffect(const BCHAR_T* pFileName);
      long                    createTerrainEffect(const BCHAR_T* pFileName);
      long                    getOrCreateTerrainEffect(const BCHAR_T* pFileName, bool loadFile);
      BTerrainEffect*         getTerrainEffect(long index, bool ensureLoaded);
      long                    getNumberTerrainEffects() const { return mTerrainEffectNameTable.getTags().getNumber(); }

      bool                    checkMeter(int terrainEffectID, BVector pos);

      // Debug stats
      #ifndef BUILD_FINAL
         struct BStats
         {
            public: 
            BStats(){clear(); clearMaxStats();};

            void clear(void)
            {
               mNumImpactsSimTriedToCreate = 0;
               mDroppedImpacts = 0;
               mAddedImpacts  = 0;
            }

            void clearMaxStats()
            {
            }            

            int mNumImpactsSimTriedToCreate;
            int mDroppedImpacts;
            int mAddedImpacts;
         };

         void getStats(BStats& stats) { stats = mStats; } 
         void resetStats() { mStats.clear(); }
         void dumpVoxelData(BDebugTextDisplay& textDisplay);

         BTerrainEffectManager::BStats mStats;
      #endif

   protected:

      void initKey(const BVector& pos, BActiveTerrainEffectKey& key) const;

      BSmallDynamicSimArray<BTerrainEffect*>      mTerrainEffects;
      BStringTable<short, false>                  mTerrainEffectNameTable;

      typedef BHashMap<BActiveTerrainEffectKey, BActiveTerrainEffectValue, BHasher<BActiveTerrainEffectKey>, BEqualTo<BActiveTerrainEffectKey>, true, BSimFixedHeapAllocator> BTerrainEffectEffectGridHashMap;      
      BTerrainEffectEffectGridHashMap mTerrainEffectGrid;
};

// gTerrainEffectManager
extern BTerrainEffectManager gTerrainEffectManager;
