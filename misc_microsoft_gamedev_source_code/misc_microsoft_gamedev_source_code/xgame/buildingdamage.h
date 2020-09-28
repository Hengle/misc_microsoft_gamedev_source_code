//==============================================================================
// buildingdamage.h
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "containers\staticArray.h"
#include "D3DTextureManager.h"

//==============================================================================
// Defines
//==============================================================================
#define BBUILDING_DAMAGE_ARRAY_SIZE 20

struct BBuildingDamageInfo
{
   BBuildingDamageInfo() { clear(); }
   
   void clear(void)
   {
      name.empty();
      damageBrushTextureID = cInvalidManagedTextureHandle;
      scorchBrushTextureID = cInvalidManagedTextureHandle;
      damageTextureID = cInvalidManagedTextureHandle;
      damageRadius = 0;
      scorchRadius = 0;
      hotIntensity = 0;
      coldIntensity = 0;
      duration = 0;
   }
   
   BSimString        name;
   BManagedTextureHandle damageBrushTextureID;
   BManagedTextureHandle scorchBrushTextureID;
   BManagedTextureHandle damageTextureID;
   float          damageRadius;
   float          scorchRadius;
   float          hotIntensity;
   float          coldIntensity;
   float          duration;
};
typedef BStaticArray<BBuildingDamageInfo, BBUILDING_DAMAGE_ARRAY_SIZE, true, false> BBuildingDamageInfoArray;

//==============================================================================
class BBuildingDamage
{
   public:

      // Constructor/Destructor
      BBuildingDamage();
      ~BBuildingDamage();

      // Initializer/Deinitializer
      void init(void);
      void deinit(void);

      // Public accessor functions
      inline const long getNumDamageTypes(void) const;
      inline const BBuildingDamageInfo *getDamageInfo(const long damageType) const;

   private:

      // Add new damage info to the list
      inline void addDamageInfo(const BBuildingDamageInfo &damageInfo);

      BBuildingDamageInfoArray mBuildingInfo;
};

extern BBuildingDamage gBuildingDamage;

//==============================================================================
// Inlines
//==============================================================================

inline const long BBuildingDamage::getNumDamageTypes(void) const
{
   return mBuildingInfo.getSize();
}

inline const BBuildingDamageInfo *BBuildingDamage::getDamageInfo(const long damageType) const
{
   return mBuildingInfo.getPtr(damageType);
}

inline void BBuildingDamage::addDamageInfo(const BBuildingDamageInfo &damageInfo)
{
   mBuildingInfo.pushBack(damageInfo);
}
